/**
 * @file rbl_main.c
 * @brief S300 ROM Bootloader (RBL) - ESP32 Compatible
 * @version 1.0
 * @date 2025-08-29
 * 
 * RBL是S300启动链的第二级，运行在SRAM中，实现ESP32 ROM Bootloader等效功能
 */

#include "rbl_main.h"
#include "rbl_system.h"
#include "rbl_uart.h"
#include "rbl_qspi.h"
#include "rbl_download.h"
#include "rbl_flash.h"
#include "rbl_crc.h"
#include "rbl_download_flags.h"  // 新增：下载标志管理
#include "gpio.h"
#include "rcc.h"

/* 声明增强检测函数 */
boot_mode_t rbl_detect_boot_mode_enhanced(void);
bool rbl_check_enhanced_serial_window(void);
int rbl_handle_enhanced_download_mode(boot_mode_t mode);
const char *rbl_get_boot_mode_string_enhanced(boot_mode_t mode);

/* RBL版本信息 */
#define RBL_VERSION_MAJOR    1
#define RBL_VERSION_MINOR    0
#define RBL_VERSION_PATCH    0

/* RBL运行时信息 */
static struct {
    uint32_t boot_count;
    uint32_t last_boot_mode;
    uint32_t reset_reason;
} g_rbl_runtime;

/**
 * @brief RBL入口函数
 */
int main(void)
{
    boot_mode_t boot_mode;
    int ret;

    /* 系统初始化 */
    rbl_system_early_init();
    
    /* 初始化调试串口 */
    rbl_uart_init();
    
    /* 打印启动Banner */
    rbl_print_banner();
    
    /* 分析复位原因 */
    g_rbl_runtime.reset_reason = rbl_system_get_reset_reason();
    g_rbl_runtime.boot_count++;
    
    printf("[RBL] Reset reason: %s\r\n", rbl_get_reset_reason_string());
    printf("[RBL] Boot count: %lu\r\n", (unsigned long)g_rbl_runtime.boot_count);
    
    /* 完整系统初始化 */
    ret = rbl_system_init();
    if (ret != 0) {
        printf("[RBL] System init failed: %d\r\n", ret);
        goto error_handler;
    }
    
    /* 初始化QSPI Flash */
    ret = rbl_qspi_init();
    if (ret != 0) {
        printf("[RBL] QSPI init failed: %d\r\n", ret);
        goto error_handler;
    }
    
    /* 检测启动模式 - 使用增强版检测 */
    boot_mode = rbl_detect_boot_mode_enhanced();
    g_rbl_runtime.last_boot_mode = boot_mode;
    
    printf("[RBL] Boot mode: %s\r\n", rbl_get_boot_mode_string_enhanced(boot_mode));
    
    /* 根据启动模式执行相应流程 */
    switch (boot_mode) {
        case BOOT_MODE_DOWNLOAD_GPIO:
        case BOOT_MODE_DOWNLOAD_SERIAL:
        case BOOT_MODE_DOWNLOAD_SOFTWARE:
        case BOOT_MODE_DOWNLOAD_DOUBLE_RESET:
        case BOOT_MODE_BOOT_FAILURE:
        case BOOT_MODE_RECOVERY:
            /* 使用增强版下载模式处理 */
            ret = rbl_handle_enhanced_download_mode(boot_mode);
            if (ret != 0) {
                printf("[RBL] Enhanced download mode failed: %d\r\n", ret);
                goto error_handler;
            }
            break;
            
        case BOOT_MODE_NORMAL:
        default:
            /* 正常启动流程 */
            ret = rbl_normal_boot();
            if (ret != 0) {
                printf("[RBL] Normal boot failed: %d\r\n", ret);
                goto error_handler;
            }
            break;
    }
    
    /* 不应该到达这里 */
    printf("[RBL] Unexpected return from boot process\r\n");
    
error_handler:
    printf("[RBL] Error handler, entering infinite loop\r\n");
    while (1) {
        rbl_system_delay_ms(1000);
    }
    
    return -1;
}

/**
 * @brief 打印RBL启动Banner
 */
void rbl_print_banner(void)
{
    printf("\r\n");
    printf("================================================\r\n");
    printf("S300 RBL v%d.%d.%d - ROM Bootloader (ESP32 Compatible)\r\n", 
           RBL_VERSION_MAJOR, RBL_VERSION_MINOR, RBL_VERSION_PATCH);
    printf("Build: %s %s\r\n", __DATE__, __TIME__);
    printf("Chip: PiMCHIP S300, SRAM: 384KB, Running at: 0x%08lX\r\n", 
           (unsigned long)main);
    printf("================================================\r\n");
}

/**
 * @brief 检测启动模式
 */
boot_mode_t rbl_detect_boot_mode(void)
{
    /* 检查GPIO强制下载模式 */
    if (rbl_check_gpio_download_mode()) {
        printf("[RBL] GPIO download mode detected\r\n");
        return BOOT_MODE_DOWNLOAD_GPIO;
    }
    
    /* 检查Flash恢复标志 */
    if (rbl_flash_check_recovery_flag()) {
        printf("[RBL] Recovery mode flag detected\r\n");
        return BOOT_MODE_RECOVERY;
    }
    
    /* 检查串口下载窗口期 */
    printf("[RBL] Checking serial download window (3s)...\r\n");
    if (rbl_check_serial_download_window()) {
        printf("[RBL] Serial download mode activated\r\n");
        return BOOT_MODE_DOWNLOAD_SERIAL;
    }
    
    printf("[RBL] Normal boot mode\r\n");
    return BOOT_MODE_NORMAL;
}

/**
 * @brief 正常启动流程
 */
int rbl_normal_boot(void)
{
    sbl_info_t sbl_info;
    int ret;
    int retry_count = 0;
    const int max_retries = 3;
    
    printf("[RBL] Starting normal boot process...\r\n");
    
verify_retry:
    /* 获取SBL信息 */
    ret = rbl_get_sbl_info(&sbl_info);
    if (ret != 0) {
        printf("[RBL] Failed to get SBL info: %d\r\n", ret);
        goto sbl_verification_failed;
    }
    
    printf("[RBL] SBL Info:\r\n");
    printf("  Flash Addr: 0x%08lX\r\n", (unsigned long)sbl_info.addr);
    printf("  Size: %lu bytes\r\n", (unsigned long)sbl_info.size);
    printf("  CRC32: 0x%08lX\r\n", (unsigned long)sbl_info.crc32);
    printf("  Entry: 0x%08lX\r\n", (unsigned long)sbl_info.entry);
    
    /* 验证SBL完整性 */
    ret = rbl_verify_sbl(&sbl_info);
    if (ret != 0) {
        printf("[RBL] SBL verification failed: %d\r\n", ret);
        
        /* 重试机制 */
        retry_count++;
        if (retry_count < max_retries) {
            printf("[RBL] Retrying SBL verification (%d/%d)...\r\n", retry_count, max_retries);
            rbl_system_delay_ms(1000);
            goto verify_retry;
        }
        
        goto sbl_verification_failed;
    }
    
    printf("[RBL] SBL verification passed\r\n");
    
    /* 跳转到SBL */
    printf("[RBL] Jumping to SBL at 0x%08lX...\r\n", (unsigned long)sbl_info.entry);
    rbl_jump_to_sbl(sbl_info.entry);
    
    /* 不应该返回 */
    return -1;

sbl_verification_failed:
    printf("[RBL] ========================================\r\n");
    printf("[RBL] CRITICAL: SBL verification failed!\r\n");
    printf("[RBL] System cannot boot normally.\r\n");
    printf("[RBL] ========================================\r\n");
    printf("[RBL] Possible causes:\r\n");
    printf("[RBL] 1. SBL not programmed or corrupted\r\n");
    printf("[RBL] 2. Flash read error\r\n");
    printf("[RBL] 3. Incomplete firmware programming\r\n");
    printf("[RBL] ========================================\r\n");
    printf("[RBL] Entering download mode for recovery...\r\n");
    
    /* 设置恢复标志 */
    rbl_flash_set_recovery_flag();
    
    /* 进入下载模式进行恢复 */
    return rbl_download_mode();
}

/**
 * @brief 获取SBL信息
 */
int rbl_get_sbl_info(sbl_info_t *sbl_info)
{
    /* SBL固定位置和大小 */
    sbl_info->addr = 0x10000;          // SBL在Flash中的地址
    sbl_info->size = 128 * 1024;       // SBL大小 128KB
    sbl_info->entry = 0x80010000;      // SBL XIP入口地址
    
    /* 计算SBL的CRC32 */
    uint32_t calculated_crc;
    int ret = rbl_flash_calculate_crc32(sbl_info->addr, sbl_info->size, &calculated_crc);
    if (ret != 0) {
        return ret;
    }
    
    sbl_info->crc32 = calculated_crc;
    return 0;
}

/**
 * @brief 验证SBL完整性
 */
int rbl_verify_sbl(const sbl_info_t *sbl_info)
{
    uint32_t calculated_crc;
    int ret;
    uint8_t test_buffer[256];
    
    printf("[RBL] Verifying SBL integrity...\r\n");
    
    /* 1. 检查SBL头部是否存在 */
    ret = rbl_qspi_read(sbl_info->addr, test_buffer, 16);
    if (ret != 0) {
        printf("[RBL] Failed to read SBL header: %d\r\n", ret);
        return -1;
    }
    
    /* 检查ARM Cortex-M向量表签名 */
    uint32_t *vector_table = (uint32_t *)test_buffer;
    uint32_t initial_sp = vector_table[0];
    uint32_t reset_handler = vector_table[1];
    
    /* 栈指针应该在SRAM范围内 */
    if (initial_sp < 0x80000000 || initial_sp > 0x80060000) {
        printf("[RBL] Invalid SBL stack pointer: 0x%08lX\r\n", (unsigned long)initial_sp);
        return -2;
    }
    
    /* Reset Handler应该在XIP地址范围内 */
    if ((reset_handler & 0xFFFF0000) != 0x80010000) {
        printf("[RBL] Invalid SBL reset handler: 0x%08lX\r\n", (unsigned long)reset_handler);
        return -3;
    }
    
    /* 2. 检查SBL是否完全写入(非全0xFF) */
    uint32_t empty_count = 0;
    uint32_t sample_points = 32; /* 采样点数量 */
    
    for (uint32_t i = 0; i < sample_points; i++) {
        uint32_t sample_addr = sbl_info->addr + (i * sbl_info->size / sample_points);
        ret = rbl_qspi_read(sample_addr, test_buffer, 16);
        if (ret != 0) {
            printf("[RBL] Failed to read SBL at 0x%08lX\r\n", (unsigned long)sample_addr);
            return -4;
        }
        
        /* 检查是否为空白区域(全0xFF) */
        bool is_empty = true;
        for (int j = 0; j < 16; j++) {
            if (test_buffer[j] != 0xFF) {
                is_empty = false;
                break;
            }
        }
        
        if (is_empty) {
            empty_count++;
        }
    }
    
    /* 如果超过80%的采样点为空，认为SBL未正确烧录 */
    if (empty_count > (sample_points * 4 / 5)) {
        printf("[RBL] SBL appears to be empty or incomplete (%lu/%lu empty)\r\n", 
               (unsigned long)empty_count, (unsigned long)sample_points);
        return -5;
    }
    
    /* 3. 计算并验证CRC32 */
    ret = rbl_flash_calculate_crc32(sbl_info->addr, sbl_info->size, &calculated_crc);
    if (ret != 0) {
        printf("[RBL] Failed to calculate SBL CRC32: %d\r\n", ret);
        return -6;
    }
    
    /* 如果SBL头部包含预期的CRC32，则进行比较 */
    /* 这里假设SBL在固定位置包含自己的CRC32值 */
    uint32_t expected_crc = 0;
    ret = rbl_qspi_read(sbl_info->addr + sbl_info->size - 4, (uint8_t*)&expected_crc, 4);
    if (ret == 0 && expected_crc != 0xFFFFFFFF && expected_crc != 0) {
        if (calculated_crc != expected_crc) {
            printf("[RBL] SBL CRC32 mismatch: expected 0x%08lX, got 0x%08lX\r\n", 
                   (unsigned long)expected_crc, (unsigned long)calculated_crc);
            return -7;
        }
        printf("[RBL] SBL CRC32 verification passed: 0x%08lX\r\n", (unsigned long)calculated_crc);
    } else {
        printf("[RBL] SBL CRC32 calculated: 0x%08lX (no expected value found)\r\n", (unsigned long)calculated_crc);
    }
    
    printf("[RBL] SBL integrity verification completed successfully\r\n");
    return 0;
}

/**
 * @brief 跳转到SBL
 */
void rbl_jump_to_sbl(uint32_t entry_addr)
{
    /* 设置向量表到SBL地址 */
    SCB->VTOR = entry_addr;
    __DSB();
    
    /* 获取SBL的栈指针和入口地址 */
    uint32_t *sbl_vector_table = (uint32_t *)entry_addr;
    uint32_t sbl_sp = sbl_vector_table[0];
    uint32_t sbl_pc = sbl_vector_table[1];
    
    printf("[RBL] SBL SP: 0x%08lX, PC: 0x%08lX\r\n", 
           (unsigned long)sbl_sp, (unsigned long)sbl_pc);
    
    /* 关闭中断 */
    __disable_irq();
    
    /* 设置栈指针并跳转 */
    __asm volatile (
        "msr msp, %0\n"
        "bx %1\n"
        :
        : "r" (sbl_sp), "r" (sbl_pc)
        : "memory"
    );
}

/**
 * @brief 检查GPIO下载模式
 */
bool rbl_check_gpio_download_mode(void)
{
    /* 检查GPIO0引脚状态作为下载模式触发 */
    /* GPIO0 = 低电平时进入下载模式 */
    
    /* 使能GPIO时钟(在APB1总线上) */
    rcc_set_cortex_m4_apb1_clock(RCC_CM4_APB1_GPIO, true);
    
    /* 配置GPIO0为输入模式，启用内部上拉 */
    /* 这里需要使用S300的GPIO寄存器操作 */
    
    /* GPIO基地址 - 需要根据S300实际地址调整 */
    volatile uint32_t *gpio_base = (volatile uint32_t *)0x40020000; // 假设的GPIO基地址
    
    /* 配置GPIO0 (假设在GPIOA的Pin0) */
    /* 设置为输入模式 (00) */
    gpio_base[0] &= ~(3U << (0 * 2));  // GPIOA_MODER
    
    /* 启用内部上拉 (01) */
    gpio_base[3] &= ~(3U << (0 * 2));  // GPIOA_PUPDR
    gpio_base[3] |= (1U << (0 * 2));   // 上拉
    
    /* 短暂延时让电平稳定 */
    rbl_system_delay_ms(10);
    
    /* 读取GPIO状态 */
    uint32_t gpio_idr = gpio_base[4];  // GPIOA_IDR
    bool gpio0_low = !(gpio_idr & (1U << 0));
    
    if (gpio0_low) {
        printf("[RBL] GPIO0 pulled low - Download mode triggered\r\n");
        
        /* 多次采样确保稳定 */
        int low_count = 0;
        for (int i = 0; i < 5; i++) {
            rbl_system_delay_ms(10);
            gpio_idr = gpio_base[4];
            if (!(gpio_idr & (1U << 0))) {
                low_count++;
            }
        }
        
        if (low_count >= 3) {
            printf("[RBL] GPIO download mode confirmed (%d/5 samples low)\r\n", low_count);
            return true;
        } else {
            printf("[RBL] GPIO state unstable, ignoring\r\n");
        }
    }
    
    printf("[RBL] GPIO download mode not detected\r\n");
    return false;
}

/**
 * @brief 检查串口下载窗口期
 */
bool rbl_check_serial_download_window(void)
{
    uint32_t start_time = rbl_system_get_tick_ms();
    const uint32_t timeout_ms = 3000;  // 3秒超时
    uint8_t rx_data;
    
    printf("[RBL] Press any key within 3s for download mode");
    
    while ((rbl_system_get_tick_ms() - start_time) < timeout_ms) {
        /* 检查串口是否有数据 */
        if (rbl_uart_read_nonblock(&rx_data, 1) > 0) {
            printf("\r\n[RBL] Download request detected (0x%02X)\r\n", rx_data);
            return true;
        }
        
        /* 每500ms打印一个点 */
        static uint32_t last_dot = 0;
        if ((rbl_system_get_tick_ms() - last_dot) >= 500) {
            printf(".");
            last_dot = rbl_system_get_tick_ms();
        }
        
        /* 短暂延时避免CPU占用过高 */
        rbl_system_delay_ms(10);
    }
    
    printf("\r\n[RBL] Download window timeout\r\n");
    return false;
}

/**
 * @brief 获取复位原因字符串
 */
const char *rbl_get_reset_reason_string(void)
{
    uint32_t reason = g_rbl_runtime.reset_reason;
    static char reason_str[128];
    
    /* 构建复位原因字符串 */
    reason_str[0] = '\0';
    
    if (reason & 0x01) strcat(reason_str, "LPWR ");
    if (reason & 0x02) strcat(reason_str, "WWDG ");
    if (reason & 0x04) strcat(reason_str, "IWDG ");
    if (reason & 0x08) strcat(reason_str, "SFT ");
    if (reason & 0x10) strcat(reason_str, "POR ");
    if (reason & 0x20) strcat(reason_str, "PIN ");
    if (reason & 0x40) strcat(reason_str, "BOR ");
    
    /* 如果字符串为空，返回未知 */
    if (reason_str[0] == '\0') {
        return "Unknown";
    }
    
    /* 移除末尾空格 */
    int len = strlen(reason_str);
    if (len > 0 && reason_str[len-1] == ' ') {
        reason_str[len-1] = '\0';
    }
    
    return reason_str;
}

/**
 * @brief 获取启动模式字符串
 */
const char *rbl_get_boot_mode_string(boot_mode_t mode)
{
    switch (mode) {
        case BOOT_MODE_NORMAL:              return "Normal";
        case BOOT_MODE_DOWNLOAD_GPIO:       return "GPIO Download";
        case BOOT_MODE_DOWNLOAD_SERIAL:     return "Serial Download";
        case BOOT_MODE_DOWNLOAD_SOFTWARE:   return "Software Download";
        case BOOT_MODE_DOWNLOAD_DOUBLE_RESET: return "Double Reset";
        case BOOT_MODE_RECOVERY:            return "Recovery";
        case BOOT_MODE_BOOT_FAILURE:        return "Boot Failure";
        default:                            return "Unknown";
    }
}
