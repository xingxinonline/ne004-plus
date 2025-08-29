/**
 * @file sbl_main.c
 * @brief SBL主程序实现 - ESP32兼容的二级引导程序
 */

#include "sbl_main.h"
#include "sbl_config.h"
#include "sbl_partition.h"
#include "sbl_ota.h"
#include "s300.h"
#include "uart.h"
#include "rcc.h"
#include <stdio.h>
#include <string.h>

/* 全局变量 */
static volatile uint32_t g_system_tick = 0;
static sbl_system_info_t g_system_info = {0};

/* 函数声明 */
extern void SystemInit(void);

/**
 * @brief SBL主函数
 */
int sbl_main(void)
{
    sbl_boot_result_t result = SBL_BOOT_SUCCESS;
    sbl_boot_env_t boot_env = {0};
    const esp_partition_info_t* boot_partition = NULL;
    
    /* 1. 系统初始化 */
    if (sbl_system_init() != 0) {
        result = SBL_BOOT_UNKNOWN_ERROR;
        goto error_exit;
    }
    
    /* 2. 打印启动信息 */
    sbl_print_banner();
    
    /* 3. 初始化分区表系统 */
    SBL_LOGI("BOOT", "Initializing partition table...");
    if (sbl_partition_init() != 0) {
        SBL_LOGE("BOOT", "Partition table initialization failed");
        result = SBL_BOOT_PARTITION_ERROR;
        goto error_exit;
    }
    
    /* 4. 打印分区表信息 */
    sbl_partition_table_print();
    
    /* 5. 初始化OTA系统 */
    SBL_LOGI("BOOT", "Initializing OTA system...");
    if (sbl_ota_init() != 0) {
        SBL_LOGE("BOOT", "OTA system initialization failed");
        result = SBL_BOOT_OTA_ERROR;
        goto error_exit;
    }
    
    /* 6. 读取启动环境 */
    if (sbl_boot_read_env(&boot_env) != 0) {
        SBL_LOGW("BOOT", "Failed to read boot environment, using defaults");
        memset(&boot_env, 0, sizeof(boot_env));
    }
    
    SBL_LOGI("BOOT", "Boot environment: slot=%d, pending=%d, count=%d, retry=%d",
             boot_env.active_slot, boot_env.update_pending, 
             boot_env.boot_count, boot_env.retry_count);
    
    /* 7. 确定启动分区 */
    boot_partition = sbl_ota_get_boot_partition();
    if (boot_partition == NULL) {
        SBL_LOGE("BOOT", "No valid boot partition found");
        result = SBL_BOOT_OTA_ERROR;
        goto error_exit;
    }
    
    SBL_LOGI("BOOT", "Selected boot partition: %s (offset=0x%X, size=0x%X)",
             boot_partition->label, boot_partition->offset, boot_partition->size);
    
    /* 8. 验证镜像 */
    SBL_LOGI("BOOT", "Verifying application image...");
    if (!sbl_ota_verify_image(boot_partition)) {
        SBL_LOGE("BOOT", "Application image verification failed");
        
        /* 检查是否需要回滚 */
        if (sbl_boot_should_rollback(&boot_env)) {
            SBL_LOGW("BOOT", "Performing rollback...");
            if (sbl_boot_perform_rollback(&boot_env) == 0) {
                boot_partition = sbl_ota_get_boot_partition();
                if (boot_partition && sbl_ota_verify_image(boot_partition)) {
                    SBL_LOGI("BOOT", "Rollback successful, booting from %s", boot_partition->label);
                    goto boot_app;
                }
            }
        }
        
        result = SBL_BOOT_VERIFY_ERROR;
        goto error_exit;
    }
    
boot_app:
    /* 9. 更新启动计数 */
    boot_env.boot_count++;
    if (boot_env.update_pending) {
        boot_env.retry_count++;
        SBL_LOGI("BOOT", "Update pending, retry count: %d", boot_env.retry_count);
    }
    sbl_boot_write_env(&boot_env);
    
    /* 10. 启动看门狗 */
    if (boot_env.update_pending && boot_env.retry_count > 0) {
        SBL_LOGI("BOOT", "Starting watchdog for new firmware validation");
        sbl_watchdog_start(SBL_WATCHDOG_TIMEOUT_MS);
    }
    
    /* 11. 计算应用程序地址 */
    uint32_t app_address = SBL_FLASH_BASE_ADDR + boot_partition->offset;
    uint32_t app_entry = app_address + sizeof(esp_image_header_t);
    
    SBL_LOGI("BOOT", "Loading app from partition %s at offset 0x%x", 
             boot_partition->label, boot_partition->offset);
    SBL_LOGI("BOOT", "Application load address: 0x%08X", app_address);
    SBL_LOGI("BOOT", "Application entry point: 0x%08X", app_entry);
    
    /* 12. 跳转到应用程序 */
    SBL_LOGI("BOOT", "Starting application...");
    sbl_delay_ms(100); /* 确保日志输出完成 */
    
    sbl_jump_to_app(app_address, app_entry);
    
    /* 不应该到达这里 */
    return SBL_BOOT_UNKNOWN_ERROR;
    
error_exit:
    sbl_error_handler(result, "SBL boot failed");
    return result;
}

/**
 * @brief 系统初始化
 */
int sbl_system_init(void)
{
    /* 系统时钟初始化 */
    SystemInit();
    
    /* 配置系统时钟为192MHz */
    rcc_config_system_clock_192mhz();
    
    /* 初始化SysTick - 1ms */
    SysTick_Config(SystemCoreClock / 1000);
    
    /* 初始化调试串口 */
    uart_config_t uart_config = {
        .baudrate = SBL_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    
    if (uart_param_config(SBL_UART_DEBUG_PORT, &uart_config) != 0) {
        return -1;
    }
    
    if (uart_driver_install(SBL_UART_DEBUG_PORT) != 0) {
        return -1;
    }
    
    /* 初始化系统信息 */
    g_system_info.magic = 0x5300534C; /* "SL00" */
    g_system_info.version = (SBL_VERSION_MAJOR << 16) | 
                           (SBL_VERSION_MINOR << 8) | 
                           SBL_VERSION_PATCH;
    g_system_info.build_time = 0; /* TODO: 从编译时间获取 */
    
    return 0;
}

/**
 * @brief 打印启动横幅
 */
void sbl_print_banner(void)
{
    printf("\r\n");
    printf("========================================\r\n");
    printf("S300 SBL v%s - ESP32 Compatible Bootloader\r\n", SBL_VERSION_STRING);
    printf("Build: %s %s\r\n", __DATE__, __TIME__);
    printf("========================================\r\n");
    printf("CPU: ARM Cortex-M4F @ %lu MHz\r\n", SystemCoreClock / 1000000);
    printf("SBL: Flash XIP Mode (Base: 0x%08X)\r\n", SBL_FLASH_BASE_ADDR + SBL_FLASH_OFFSET);
    printf("APP SRAM: %d KB Available (0x%08X - 0x%08X)\r\n", 
           SBL_APP_SRAM_SIZE / 1024, SBL_APP_SRAM_START, 
           SBL_APP_SRAM_START + SBL_APP_SRAM_SIZE);
    printf("========================================\r\n");
}

/**
 * @brief 获取SBL版本字符串
 */
const char* sbl_get_version_string(void)
{
    return SBL_VERSION_STRING;
}

/**
 * @brief 获取系统信息
 */
int sbl_get_system_info(sbl_system_info_t* info)
{
    if (info == NULL) {
        return -1;
    }
    
    *info = g_system_info;
    info->boot_count = g_system_tick;
    
    return 0;
}

/**
 * @brief 跳转到应用程序
 */
void sbl_jump_to_app(uint32_t app_address, uint32_t app_entry)
{
    /* 禁用中断 */
    __disable_irq();
    
    /* 停止看门狗 */
    sbl_watchdog_stop();
    
    /* 停止SysTick */
    SysTick->CTRL = 0;
    
    /* 读取应用程序的栈指针和入口点 */
    uint32_t* app_vector_table = (uint32_t*)app_address;
    uint32_t app_stack_ptr = app_vector_table[0];
    uint32_t app_reset_handler = app_vector_table[1];
    
    /* 验证栈指针和入口点的有效性 */
    if (app_stack_ptr < 0x20000000 || app_stack_ptr > 0x20060000) {
        SBL_LOGE("BOOT", "Invalid stack pointer: 0x%08X", app_stack_ptr);
        sbl_system_reset();
    }
    
    if ((app_reset_handler & 1) == 0 || app_reset_handler < app_address) {
        SBL_LOGE("BOOT", "Invalid reset handler: 0x%08X", app_reset_handler);
        sbl_system_reset();
    }
    
    SBL_LOGI("BOOT", "App stack pointer: 0x%08X", app_stack_ptr);
    SBL_LOGI("BOOT", "App reset handler: 0x%08X", app_reset_handler);
    
    /* 设置向量表基地址 */
    SCB->VTOR = app_address;
    
    /* 设置主栈指针 */
    __set_MSP(app_stack_ptr);
    
    /* 跳转到应用程序 */
    void (*app_main)(void) = (void(*)(void))app_reset_handler;
    app_main();
    
    /* 不应该到达这里 */
    while(1);
}

/**
 * @brief 系统复位
 */
void sbl_system_reset(void)
{
    SBL_LOGI("BOOT", "System reset requested");
    
    /* 等待串口发送完成 */
    sbl_delay_ms(100);
    
    /* 执行系统复位 */
    NVIC_SystemReset();
    
    /* 不应该到达这里 */
    while(1);
}

/**
 * @brief 错误处理
 */
void sbl_error_handler(sbl_boot_result_t error_code, const char* error_msg)
{
    SBL_LOGE("BOOT", "Fatal error: %d - %s", error_code, error_msg ? error_msg : "Unknown");
    
    /* 更新系统信息 */
    g_system_info.last_error = error_code;
    
    /* 进入死循环或复位 */
    SBL_LOGE("BOOT", "System will reset in 5 seconds...");
    for (int i = 5; i > 0; i--) {
        SBL_LOGE("BOOT", "Reset countdown: %d", i);
        sbl_delay_ms(1000);
    }
    
    sbl_system_reset();
}

/**
 * @brief 启动看门狗
 */
void sbl_watchdog_start(uint32_t timeout_ms)
{
    /* TODO: 实现看门狗启动 */
    SBL_LOGI("WDT", "Watchdog started with timeout: %lu ms", timeout_ms);
}

/**
 * @brief 停止看门狗
 */
void sbl_watchdog_stop(void)
{
    /* TODO: 实现看门狗停止 */
    SBL_LOGI("WDT", "Watchdog stopped");
}

/**
 * @brief 喂看门狗
 */
void sbl_watchdog_feed(void)
{
    /* TODO: 实现看门狗喂狗 */
}

/**
 * @brief 延时函数
 */
void sbl_delay_ms(uint32_t ms)
{
    uint32_t start = g_system_tick;
    while ((g_system_tick - start) < ms) {
        __WFI(); /* 等待中断 */
    }
}

/**
 * @brief 获取系统时钟
 */
uint32_t sbl_get_tick_ms(void)
{
    return g_system_tick;
}

/**
 * @brief SysTick中断处理函数
 */
void SysTick_Handler(void)
{
    g_system_tick++;
}

/**
 * @brief SBL入口点
 */
int main(void)
{
    return sbl_main();
}
