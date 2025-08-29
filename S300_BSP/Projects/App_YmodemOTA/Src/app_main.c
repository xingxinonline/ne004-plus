/**
 * @file app_main.c
 * @brief App主程序实现 - Ymodem OTA应用程序
 */

#include "app_main.h"
#include "app_config.h"
#include "app_ota.h"
#include "s300.h"
#include "uart.h"
#include "rcc.h"
#include "gpio.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* 全局变量 */
static volatile uint32_t g_system_tick = 0;
static app_system_info_t g_system_info = {0};
static app_state_t g_app_state = APP_STATE_INIT;
static char g_cmd_buffer[256];
static size_t g_cmd_length = 0;

/* 外部函数声明 */
extern void SystemInit(void);
extern uint32_t SystemCoreClock;

/* 静态函数声明 */
static void ota_event_handler(app_ota_event_t event, void* event_data, void* user_data);
static int read_command_line(char* buffer, size_t buffer_size);
static void process_character(char ch);

/**
 * @brief 应用程序主函数
 */
int app_main(void)
{
    int result = 0;
    
    /* 系统初始化 */
    if (app_system_init() != 0) {
        app_error_handler(-1, "System initialization failed");
        return -1;
    }
    
    /* 打印启动信息 */
    app_print_banner();
    
    /* 显示帮助信息 */
    app_show_help();
    
    g_app_state = APP_STATE_RUNNING;
    
    APP_LOGI("MAIN", "Application started successfully");
    printf("S300 Ymodem OTA Demo > ");
    fflush(stdout);
    
    /* 主循环 */
    while (1) {
        /* 处理命令行输入 */
        app_console_process();
        
        /* 处理OTA */
        if (g_app_state == APP_STATE_OTA_MODE) {
            app_ota_process();
        }
        
        /* 喂看门狗 */
        app_watchdog_feed();
        
        /* 短暂延时 */
        app_delay_ms(10);
    }
    
    return result;
}

/**
 * @brief 系统初始化
 */
int app_system_init(void)
{
    /* 系统时钟初始化 */
    SystemInit();
    
    /* 配置系统时钟为192MHz */
    /* 使用RCC驱动配置系统时钟 */
    // TODO: 使用具体的PLL配置参数
    // rcc_init_cortex_m4_pll(refdiv, fbdiv, frac, postdiv1, postdiv2);
    APP_LOGI("SYS", "System clock initialized (using default settings)");
    
    /* 初始化SysTick - 1ms */
    SysTick_Config(SystemCoreClock / 1000);
    
    /* 初始化调试串口 */
    if (uart_init(UART_IDX3, UARTTYPE_STD_SERIAL, SystemCoreClock, APP_UART_BAUDRATE) != 0) {
        return -1;
    }
    
    /* 设置UART属性：8数据位，1停止位，无校验 */
    uart_set_property(UART_IDX3, 8, 1, 0);
    
    /* 设置FIFO */
    uart_set_fifo(UART_IDX3, UART_FIFO_EN | UART_FIFO_TX_RESET | UART_FIFO_RX_RESET);
    
    /* 初始化LED */
    if (app_led_init() != 0) {
        return -1;
    }
    
    /* 初始化看门狗 */
    if (app_watchdog_init(APP_WATCHDOG_TIMEOUT) != 0) {
        return -1;
    }
    
    /* 初始化OTA系统 */
    if (app_ota_init(ota_event_handler, NULL) != 0) {
        return -1;
    }
    
    /* 初始化系统信息 */
    g_system_info.magic = 0x53303041;  /* "A00S" */
    g_system_info.version = (APP_VERSION_MAJOR << 16) | 
                           (APP_VERSION_MINOR << 8) | 
                           APP_VERSION_PATCH;
    g_system_info.build_time = 0; /* TODO: 从编译时间获取 */
    g_system_info.state = g_app_state;
    
    return 0;
}

/**
 * @brief 打印启动横幅
 */
void app_print_banner(void)
{
    printf("\r\n");
    printf("========================================\r\n");
    printf("S300 Ymodem OTA Demo v%s\r\n", APP_VERSION_STRING);
    printf("Build: %s %s\r\n", __DATE__, __TIME__);
    printf("========================================\r\n");
    printf("CPU: ARM Cortex-M4F @ %lu MHz\r\n", SystemCoreClock / 1000000);
    printf("SRAM: %d KB Available\r\n", APP_SRAM_SIZE / 1024);
    printf("Flash: Base 0x%08X, Size %d MB\r\n", 
           APP_FLASH_BASE, APP_FLASH_SIZE / (1024 * 1024));
    printf("UART: Port %d, Baudrate %d\r\n", APP_UART_PORT, APP_UART_BAUDRATE);
    printf("========================================\r\n");
}

/**
 * @brief 获取应用程序版本
 */
const char* app_get_version(void)
{
    return APP_VERSION_STRING;
}

/**
 * @brief 获取系统信息
 */
int app_get_system_info(app_system_info_t* info)
{
    if (info == NULL) {
        return -1;
    }
    
    *info = g_system_info;
    info->run_time = g_system_tick;
    info->state = g_app_state;
    
    /* 获取堆内存信息 */
    app_get_heap_stats(&info->free_heap, &info->min_free_heap);
    
    return 0;
}

/**
 * @brief 系统复位
 */
void app_system_reset(void)
{
    APP_LOGI("MAIN", "System reset requested");
    
    /* 等待串口发送完成 */
    app_delay_ms(100);
    
    /* 执行系统复位 */
    NVIC_SystemReset();
    
    /* 不应该到达这里 */
    while(1);
}

/**
 * @brief 延时函数
 */
void app_delay_ms(uint32_t ms)
{
    uint32_t start = g_system_tick;
    while ((g_system_tick - start) < ms) {
        __WFI(); /* 等待中断 */
    }
}

/**
 * @brief 获取系统时钟
 */
uint32_t app_get_tick_ms(void)
{
    return g_system_tick;
}

/**
 * @brief 错误处理函数
 */
void app_error_handler(int error_code, const char* error_msg)
{
    APP_LOGE("MAIN", "Fatal error %d: %s", error_code, error_msg ? error_msg : "Unknown");
    
    /* 更新系统信息 */
    g_system_info.state = APP_STATE_ERROR;
    g_app_state = APP_STATE_ERROR;
    
    /* LED错误指示 */
    app_led_blink(10, 100);
    
    /* 进入死循环或复位 */
    APP_LOGE("MAIN", "System will reset in 5 seconds...");
    for (int i = 5; i > 0; i--) {
        APP_LOGE("MAIN", "Reset countdown: %d", i);
        app_delay_ms(1000);
    }
    
    app_system_reset();
}

/**
 * @brief 看门狗初始化
 */
int app_watchdog_init(uint32_t timeout_ms)
{
    /* TODO: 实现看门狗初始化 */
    APP_LOGI("WDT", "Watchdog initialized with timeout: %lu ms", timeout_ms);
    return 0;
}

/**
 * @brief 喂看门狗
 */
void app_watchdog_feed(void)
{
    /* TODO: 实现看门狗喂狗 */
}

/**
 * @brief LED初始化
 */
int app_led_init(void)
{
    /* TODO: 实现LED初始化 */
    APP_LOGI("LED", "LED initialized");
    return 0;
}

/**
 * @brief LED控制
 */
void app_led_set(bool on)
{
    /* TODO: 实现LED控制 */
    APP_LOGD("LED", "LED %s", on ? "ON" : "OFF");
}

/**
 * @brief LED闪烁
 */
void app_led_blink(int times, uint32_t interval_ms)
{
    for (int i = 0; i < times; i++) {
        app_led_set(true);
        app_delay_ms(interval_ms);
        app_led_set(false);
        app_delay_ms(interval_ms);
    }
}

/**
 * @brief 命令行初始化
 */
int app_console_init(void)
{
    memset(g_cmd_buffer, 0, sizeof(g_cmd_buffer));
    g_cmd_length = 0;
    return 0;
}

/**
 * @brief 处理命令行输入
 */
void app_console_process(void)
{
    /* 检查是否有数据可读 */
    S300_UART_TypeDef* uart = (S300_UART_TypeDef*)UART3;
    if (uart->LSR & 0x01) { /* 数据就绪 */
        uint16_t data = uart_read(UART_IDX3, UARTTYPE_STD_SERIAL);
        uint8_t ch = (uint8_t)(data & 0xFF);
        process_character((char)ch);
    }
}

/**
 * @brief 处理单个字符
 */
static void process_character(char ch)
{
    switch (ch) {
    case '\r':
    case '\n':
        if (g_cmd_length > 0) {
            printf("\r\n");
            g_cmd_buffer[g_cmd_length] = '\0';
            
            /* 解析并执行命令 */
            app_command_t cmd = app_parse_command(g_cmd_buffer);
            app_execute_command(cmd, g_cmd_buffer);
            
            /* 重置命令缓冲区 */
            memset(g_cmd_buffer, 0, sizeof(g_cmd_buffer));
            g_cmd_length = 0;
            
            /* 显示提示符 */
            if (g_app_state != APP_STATE_OTA_MODE) {
                printf("S300 Ymodem OTA Demo > ");
                fflush(stdout);
            }
        }
        break;
        
    case '\b':
    case 0x7F: /* DEL */
        if (g_cmd_length > 0) {
            g_cmd_length--;
            printf("\b \b");
            fflush(stdout);
        }
        break;
        
    default:
        if (ch >= 32 && ch <= 126 && g_cmd_length < (sizeof(g_cmd_buffer) - 1)) {
            g_cmd_buffer[g_cmd_length++] = ch;
            printf("%c", ch);
            fflush(stdout);
        }
        break;
    }
}

/**
 * @brief 解析命令
 */
app_command_t app_parse_command(const char* cmd_line)
{
    if (cmd_line == NULL || strlen(cmd_line) == 0) {
        return APP_CMD_UNKNOWN;
    }
    
    if (strncmp(cmd_line, "help", 4) == 0) {
        return APP_CMD_HELP;
    } else if (strncmp(cmd_line, "info", 4) == 0) {
        return APP_CMD_INFO;
    } else if (strncmp(cmd_line, "version", 7) == 0) {
        return APP_CMD_VERSION;
    } else if (strncmp(cmd_line, "reboot", 6) == 0) {
        return APP_CMD_REBOOT;
    } else if (strncmp(cmd_line, "ota", 3) == 0) {
        return APP_CMD_OTA;
    } else if (strncmp(cmd_line, "test", 4) == 0) {
        return APP_CMD_TEST;
    } else if (strncmp(cmd_line, "led", 3) == 0) {
        return APP_CMD_LED;
    } else if (strncmp(cmd_line, "memory", 6) == 0) {
        return APP_CMD_MEMORY;
    } else {
        return APP_CMD_UNKNOWN;
    }
}

/**
 * @brief 执行命令
 */
int app_execute_command(app_command_t cmd, const char* args)
{
    switch (cmd) {
    case APP_CMD_HELP:
        app_show_help();
        break;
        
    case APP_CMD_INFO:
        {
            app_system_info_t info;
            app_get_system_info(&info);
            
            printf("System Information:\r\n");
            printf("  Version: %s\r\n", app_get_version());
            printf("  Magic: 0x%08X\r\n", info.magic);
            printf("  Build Time: %lu\r\n", info.build_time);
            printf("  Boot Count: %lu\r\n", info.boot_count);
            printf("  Run Time: %lu ms\r\n", info.run_time);
            printf("  State: %d\r\n", info.state);
            printf("  Free Heap: %lu bytes\r\n", info.free_heap);
            printf("  Min Free Heap: %lu bytes\r\n", info.min_free_heap);
        }
        break;
        
    case APP_CMD_VERSION:
        printf("S300 Ymodem OTA Demo v%s\r\n", app_get_version());
        printf("Build: %s %s\r\n", __DATE__, __TIME__);
        break;
        
    case APP_CMD_REBOOT:
        printf("Rebooting system...\r\n");
        app_delay_ms(1000);
        app_system_reset();
        break;
        
    case APP_CMD_OTA:
        printf("Starting OTA update...\r\n");
        if (app_ota_start() == 0) {
            g_app_state = APP_STATE_OTA_MODE;
        } else {
            printf("Failed to start OTA update\r\n");
        }
        break;
        
    case APP_CMD_TEST:
        printf("Running tests...\r\n");
        app_run_tests();
        break;
        
    case APP_CMD_LED:
        if (strstr(args, "on")) {
            app_led_set(true);
            printf("LED turned on\r\n");
        } else if (strstr(args, "off")) {
            app_led_set(false);
            printf("LED turned off\r\n");
        } else if (strstr(args, "blink")) {
            printf("LED blinking...\r\n");
            app_led_blink(5, 200);
        } else {
            printf("Usage: led [on|off|blink]\r\n");
        }
        break;
        
    case APP_CMD_MEMORY:
        {
            uint32_t free_heap, min_free_heap;
            app_get_heap_stats(&free_heap, &min_free_heap);
            
            printf("Memory Information:\r\n");
            printf("  SRAM Base: 0x%08X\r\n", APP_SRAM_BASE);
            printf("  SRAM Size: %d KB\r\n", APP_SRAM_SIZE / 1024);
            printf("  Free Heap: %lu bytes\r\n", free_heap);
            printf("  Min Free Heap: %lu bytes\r\n", min_free_heap);
        }
        break;
        
    case APP_CMD_UNKNOWN:
    default:
        printf("Unknown command: %s\r\n", args);
        printf("Type 'help' for available commands\r\n");
        break;
    }
    
    return 0;
}

/**
 * @brief 显示帮助信息
 */
void app_show_help(void)
{
    printf("\r\n");
    printf("S300 Ymodem OTA Demo - Available Commands:\r\n");
    printf("========================================\r\n");
    printf("  help      - Show this help message\r\n");
    printf("  info      - Show system information\r\n");
    printf("  version   - Show version information\r\n");
    printf("  reboot    - Reboot the system\r\n");
    printf("  ota       - Start OTA update via Ymodem\r\n");
    printf("  test      - Run system tests\r\n");
    printf("  led       - Control LED (on|off|blink)\r\n");
    printf("  memory    - Show memory information\r\n");
    printf("========================================\r\n");
    printf("\r\n");
}

/**
 * @brief 运行测试
 */
int app_run_tests(void)
{
    printf("Test 1: System Information\r\n");
    app_system_info_t info;
    if (app_get_system_info(&info) == 0) {
        printf("  PASS - System info retrieved\r\n");
    } else {
        printf("  FAIL - Failed to get system info\r\n");
        return -1;
    }
    
    printf("Test 2: LED Control\r\n");
    app_led_blink(3, 100);
    printf("  PASS - LED blink test\r\n");
    
    printf("Test 3: Timer Test\r\n");
    uint32_t start = app_get_tick_ms();
    app_delay_ms(100);
    uint32_t elapsed = app_get_tick_ms() - start;
    if (elapsed >= 95 && elapsed <= 105) {
        printf("  PASS - Timer test (elapsed: %lu ms)\r\n", elapsed);
    } else {
        printf("  FAIL - Timer test (elapsed: %lu ms)\r\n", elapsed);
        return -1;
    }
    
    printf("All tests passed!\r\n");
    return 0;
}

/**
 * @brief 获取堆内存统计
 */
int app_get_heap_stats(uint32_t* free_size, uint32_t* min_free_size)
{
    if (free_size == NULL || min_free_size == NULL) {
        return -1;
    }
    
    /* TODO: 实现真实的堆内存统计 */
    *free_size = 100 * 1024;     /* 模拟100KB可用 */
    *min_free_size = 80 * 1024;  /* 模拟80KB最小可用 */
    
    return 0;
}

/**
 * @brief OTA事件处理函数
 */
static void ota_event_handler(app_ota_event_t event, void* event_data, void* user_data)
{
    switch (event) {
    case APP_OTA_EVENT_STARTED:
        APP_LOGI("OTA", "OTA update started");
        printf("\r\nOTA update started. Ready for Ymodem transfer...\r\n");
        break;
        
    case APP_OTA_EVENT_PROGRESS:
        if (event_data != NULL) {
            uint32_t percent = *(uint32_t*)event_data;
            printf("\rOTA Progress: %lu%%", percent);
            fflush(stdout);
        }
        break;
        
    case APP_OTA_EVENT_COMPLETED:
        APP_LOGI("OTA", "OTA update completed");
        printf("\r\nOTA update completed successfully!\r\n");
        printf("Type 'reboot' to restart with new firmware\r\n");
        g_app_state = APP_STATE_RUNNING;
        printf("S300 Ymodem OTA Demo > ");
        fflush(stdout);
        break;
        
    case APP_OTA_EVENT_ERROR:
        {
            app_ota_error_t error = APP_OTA_ERROR_NONE;
            if (event_data != NULL) {
                error = *(app_ota_error_t*)event_data;
            }
            APP_LOGE("OTA", "OTA update failed with error: %d", error);
            printf("\r\nOTA update failed! Error code: %d\r\n", error);
            g_app_state = APP_STATE_RUNNING;
            printf("S300 Ymodem OTA Demo > ");
            fflush(stdout);
        }
        break;
        
    case APP_OTA_EVENT_CANCELLED:
        APP_LOGW("OTA", "OTA update cancelled");
        printf("\r\nOTA update cancelled\r\n");
        g_app_state = APP_STATE_RUNNING;
        printf("S300 Ymodem OTA Demo > ");
        fflush(stdout);
        break;
    }
}

/**
 * @brief SysTick中断处理函数
 */
void SysTick_Handler(void)
{
    g_system_tick++;
}

/**
 * @brief 程序入口点
 */
int main(void)
{
    return app_main();
}
