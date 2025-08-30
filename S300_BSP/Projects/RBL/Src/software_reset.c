/**
 * @file software_reset.c
 * @brief S300软件复位解决方案实现
 * @version 1.0
 * @date 2024
 * 
 * @copyright Copyright (c) 2024 S300 BSP Team
 */

#include "software_reset.h"
#include "partition_layout.h"
#include "qspi_flash.h"
#include "rcc.h"
#include "uart.h"
#include <string.h>
#include <stdio.h>

/* ================================ 私有变量 ================================ */

static software_reset_status_t g_reset_status = {0};
static bool g_module_initialized = false;

/* ================================ 私有函数 ================================ */

/**
 * @brief 计算CRC32校验值
 * @param data 数据指针
 * @param length 数据长度
 * @return CRC32值
 */
static uint32_t calculate_crc32(const uint8_t *data, uint32_t length) {
    uint32_t crc = 0xFFFFFFFF;
    const uint32_t polynomial = 0xEDB88320;
    
    for (uint32_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ polynomial;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return ~crc;
}

/**
 * @brief 获取系统时间戳(毫秒)
 * @return 时间戳
 */
static uint32_t get_system_time_ms(void) {
    // 使用系统滴答计数器，假设1ms一次中断
    extern volatile uint32_t g_system_tick_ms;
    return g_system_tick_ms;
}

/**
 * @brief 安全读取Flash数据
 * @param address Flash地址
 * @param buffer 数据缓冲区
 * @param size 数据大小
 * @return 0=成功, <0=失败
 */
static int safe_flash_read(uint32_t address, void *buffer, uint32_t size) {
    if (!buffer || size == 0) {
        return -1;
    }
    
    // 调用QSPI Flash读取函数
    if (qspi_flash_read(address, (uint8_t*)buffer, size) != 0) {
        return -2;
    }
    
    return 0;
}

/**
 * @brief 安全写入Flash数据
 * @param address Flash地址
 * @param buffer 数据缓冲区
 * @param size 数据大小
 * @return 0=成功, <0=失败
 */
static int safe_flash_write(uint32_t address, const void *buffer, uint32_t size) {
    if (!buffer || size == 0) {
        return -1;
    }
    
    // 先擦除整个扇区(4KB)
    if (qspi_flash_erase_sector(address & ~0xFFF) != 0) {
        return -2;
    }
    
    // 写入数据
    if (qspi_flash_write(address, (const uint8_t*)buffer, size) != 0) {
        return -3;
    }
    
    return 0;
}

/* ================================ 公共函数 ================================ */

/**
 * @brief 初始化软件复位模块
 */
int software_reset_init(void) {
    if (g_module_initialized) {
        return 0;
    }
    
    printf("[SWRST] Software Reset Module v1.0\\n");
    printf("[SWRST] NVS base addr: 0x%08X (60KB)\\n", NVS_BASE_ADDR);
    printf("[SWRST] Reset flags region: 0x%08X (4KB)\\n", RESET_FLAG_SECTOR_ADDR);
    
    // 清除状态
    memset(&g_reset_status, 0, sizeof(g_reset_status));
    
    // 检查各种标志状态
    reset_reason_t reason;
    g_reset_status.download_flag_active = software_reset_check_download_flag(&reason);
    if (g_reset_status.download_flag_active) {
        g_reset_status.last_reason = reason;
    }
    
    g_reset_status.double_reset_detected = software_reset_check_double_reset();
    
    // 读取启动计数
    boot_counter_t counter;
    if (safe_flash_read(RESET_FLAG_SECTOR_ADDR + BOOT_COUNT_OFFSET, 
                       &counter, sizeof(counter)) == 0) {
        if (counter.magic == BOOT_COUNTER_MAGIC) {
            uint32_t calc_crc = calculate_crc32((uint8_t*)&counter, 
                                               sizeof(counter) - 4);
            if (calc_crc == counter.crc32) {
                g_reset_status.total_boots = counter.boot_count;
                g_reset_status.failure_count = counter.failure_count;
            }
        }
    }
    
    g_module_initialized = true;
    
    printf("[SWRST] Module initialized successfully\\n");
    software_reset_print_status();
    
    return 0;
}

/**
 * @brief 获取软件复位状态信息
 */
int software_reset_get_status(software_reset_status_t *status) {
    if (!status || !g_module_initialized) {
        return -1;
    }
    
    *status = g_reset_status;
    return 0;
}

/**
 * @brief 设置下载模式标志并软件复位
 */
int software_reset_enter_download_mode(reset_reason_t reason) {
    download_flag_t flag = {
        .magic = DOWNLOAD_FLAG_MAGIC,
        .reason = reason,
        .timestamp = get_system_time_ms(),
        .retry_count = 0,
        .user_data = 0,
    };
    
    // 计算CRC
    flag.crc32 = calculate_crc32((uint8_t*)&flag, sizeof(flag) - 4);
    
    printf("[SWRST] Entering download mode (reason: %s)\\n", 
           software_reset_get_reason_string(reason));
    
    // 写入Flash
    if (safe_flash_write(RESET_FLAG_SECTOR_ADDR + DOWNLOAD_FLAG_OFFSET, 
                        &flag, sizeof(flag)) != 0) {
        printf("[SWRST] Error: Failed to write download flag\\n");
        return -1;
    }
    
    printf("[SWRST] Download flag set, resetting system...\\n");
    
    // 等待串口输出完成
    uart_flush();
    
    // 执行软件复位
    software_reset_system_now();
    
    // 不会执行到这里
    return 0;
}

/**
 * @brief 检查是否有下载模式标志
 */
bool software_reset_check_download_flag(reset_reason_t *reason) {
    download_flag_t flag;
    
    // 读取标志
    if (safe_flash_read(RESET_FLAG_SECTOR_ADDR + DOWNLOAD_FLAG_OFFSET, 
                       &flag, sizeof(flag)) != 0) {
        return false;
    }
    
    // 检查魔数
    if (flag.magic != DOWNLOAD_FLAG_MAGIC) {
        return false;
    }
    
    // 验证CRC
    uint32_t calc_crc = calculate_crc32((uint8_t*)&flag, sizeof(flag) - 4);
    if (calc_crc != flag.crc32) {
        printf("[SWRST] Warning: Download flag CRC mismatch\\n");
        return false;
    }
    
    // 检查重试次数，防止死循环
    if (flag.retry_count >= MAX_RETRY_COUNT) {
        printf("[SWRST] Too many download retries (%lu), clearing flag\\n", 
               flag.retry_count);
        software_reset_clear_download_flag();
        return false;
    }
    
    // 增加重试次数并保存
    flag.retry_count++;
    flag.crc32 = calculate_crc32((uint8_t*)&flag, sizeof(flag) - 4);
    safe_flash_write(RESET_FLAG_SECTOR_ADDR + DOWNLOAD_FLAG_OFFSET, 
                    &flag, sizeof(flag));
    
    if (reason) {
        *reason = (reset_reason_t)flag.reason;
    }
    
    printf("[SWRST] Download flag detected (reason: %s, retry: %lu)\\n",
           software_reset_get_reason_string((reset_reason_t)flag.reason),
           flag.retry_count);
    
    return true;
}

/**
 * @brief 清除下载模式标志
 */
void software_reset_clear_download_flag(void) {
    uint32_t clear_data[sizeof(download_flag_t) / 4] = {0};
    
    safe_flash_write(RESET_FLAG_SECTOR_ADDR + DOWNLOAD_FLAG_OFFSET,
                    clear_data, sizeof(clear_data));
    
    printf("[SWRST] Download flag cleared\\n");
    g_reset_status.download_flag_active = false;
}

/**
 * @brief 检查双重启动模式
 */
bool software_reset_check_double_reset(void) {
    double_reset_flag_t flag;
    uint32_t current_time = get_system_time_ms();
    bool is_double_reset = false;
    
    // 读取双重启标志
    if (safe_flash_read(RESET_FLAG_SECTOR_ADDR + DOUBLE_RESET_FLAG_OFFSET, 
                       &flag, sizeof(flag)) != 0) {
        memset(&flag, 0, sizeof(flag));
    }
    
    if (flag.magic == DOUBLE_RESET_MAGIC) {
        // 验证CRC
        uint32_t calc_crc = calculate_crc32((uint8_t*)&flag, sizeof(flag) - 4);
        if (calc_crc == flag.crc32) {
            uint32_t elapsed = current_time - flag.first_reset_time;
            
            if (elapsed < DOUBLE_RESET_TIMEOUT_MS) {
                // 在时间窗口内，增加计数
                flag.reset_count++;
                if (flag.reset_count >= 2) {
                    printf("[SWRST] Double reset detected! (%.1fs interval)\\n", 
                           elapsed / 1000.0f);
                    is_double_reset = true;
                    
                    // 清除标志并触发下载模式
                    software_reset_clear_double_reset_flag();
                    software_reset_enter_download_mode(RESET_REASON_DOUBLE_RESET);
                    return true; // 不会执行到这里
                }
            } else {
                // 超时，重置计数
                flag.reset_count = 1;
                flag.first_reset_time = current_time;
                printf("[SWRST] Double reset timeout, restarting detection\\n");
            }
        } else {
            // CRC错误，重新开始
            flag.magic = DOUBLE_RESET_MAGIC;
            flag.first_reset_time = current_time;
            flag.reset_count = 1;
        }
    } else {
        // 第一次复位或无效数据
        flag.magic = DOUBLE_RESET_MAGIC;
        flag.first_reset_time = current_time;
        flag.reset_count = 1;
        printf("[SWRST] First reset detected, starting double reset detection\\n");
    }
    
    // 更新标志
    flag.crc32 = calculate_crc32((uint8_t*)&flag, sizeof(flag) - 4);
    safe_flash_write(RESET_FLAG_SECTOR_ADDR + DOUBLE_RESET_FLAG_OFFSET, 
                    &flag, sizeof(flag));
    
    return is_double_reset;
}

/**
 * @brief 清除双重启标志
 */
void software_reset_clear_double_reset_flag(void) {
    uint32_t clear_data[sizeof(double_reset_flag_t) / 4] = {0};
    
    safe_flash_write(RESET_FLAG_SECTOR_ADDR + DOUBLE_RESET_FLAG_OFFSET,
                    clear_data, sizeof(clear_data));
    
    printf("[SWRST] Double reset flag cleared\\n");
    g_reset_status.double_reset_detected = false;
}

/**
 * @brief 更新启动计数器
 */
void software_reset_update_boot_counter(bool success) {
    boot_counter_t counter;
    
    // 读取启动计数
    if (safe_flash_read(RESET_FLAG_SECTOR_ADDR + BOOT_COUNT_OFFSET, 
                       &counter, sizeof(counter)) != 0) {
        memset(&counter, 0, sizeof(counter));
        counter.magic = BOOT_COUNTER_MAGIC;
    }
    
    if (counter.magic != BOOT_COUNTER_MAGIC) {
        memset(&counter, 0, sizeof(counter));
        counter.magic = BOOT_COUNTER_MAGIC;
    } else {
        // 验证CRC
        uint32_t calc_crc = calculate_crc32((uint8_t*)&counter, 
                                           sizeof(counter) - 4);
        if (calc_crc != counter.crc32) {
            // CRC错误，重新初始化
            memset(&counter, 0, sizeof(counter));
            counter.magic = BOOT_COUNTER_MAGIC;
        }
    }
    
    counter.boot_count++;
    
    if (success) {
        // 启动成功，清除故障计数
        counter.failure_count = 0;
        counter.last_success_time = get_system_time_ms();
        printf("[SWRST] Boot success reported (total: %lu)\\n", 
               counter.boot_count);
    } else {
        // 启动失败
        counter.failure_count++;
        printf("[SWRST] Boot failure detected (%lu/%d)\\n", 
               counter.failure_count, MAX_BOOT_FAILURES);
        
        if (counter.failure_count >= MAX_BOOT_FAILURES) {
            printf("[SWRST] Too many failures, triggering recovery mode\\n");
            
            // 保存计数器
            counter.crc32 = calculate_crc32((uint8_t*)&counter, 
                                           sizeof(counter) - 4);
            safe_flash_write(RESET_FLAG_SECTOR_ADDR + BOOT_COUNT_OFFSET, 
                            &counter, sizeof(counter));
            
            // 触发恢复模式
            software_reset_enter_download_mode(RESET_REASON_APP_FAILURE);
            return; // 不会执行到这里
        }
    }
    
    // 更新Flash
    counter.crc32 = calculate_crc32((uint8_t*)&counter, sizeof(counter) - 4);
    safe_flash_write(RESET_FLAG_SECTOR_ADDR + BOOT_COUNT_OFFSET, 
                    &counter, sizeof(counter));
    
    // 更新全局状态
    g_reset_status.total_boots = counter.boot_count;
    g_reset_status.failure_count = counter.failure_count;
}

/**
 * @brief 检查启动失败状态
 */
bool software_reset_check_boot_failure(void) {
    boot_counter_t counter;
    
    if (safe_flash_read(RESET_FLAG_SECTOR_ADDR + BOOT_COUNT_OFFSET, 
                       &counter, sizeof(counter)) != 0) {
        return false;
    }
    
    if (counter.magic != BOOT_COUNTER_MAGIC) {
        return false;
    }
    
    uint32_t calc_crc = calculate_crc32((uint8_t*)&counter, 
                                       sizeof(counter) - 4);
    if (calc_crc != counter.crc32) {
        return false;
    }
    
    if (counter.failure_count > 0) {
        printf("[SWRST] Previous boot failure detected (%lu failures)\\n", 
               counter.failure_count);
        
        // 报告失败状态
        software_reset_update_boot_counter(false);
        return true;
    }
    
    return false;
}

/**
 * @brief 清除启动计数器
 */
void software_reset_clear_boot_counter(void) {
    uint32_t clear_data[sizeof(boot_counter_t) / 4] = {0};
    
    safe_flash_write(RESET_FLAG_SECTOR_ADDR + BOOT_COUNT_OFFSET,
                    clear_data, sizeof(clear_data));
    
    printf("[SWRST] Boot counter cleared\\n");
    g_reset_status.total_boots = 0;
    g_reset_status.failure_count = 0;
}

/**
 * @brief 执行立即系统复位
 */
void software_reset_system_now(void) {
    printf("[SWRST] System reset NOW...\\n");
    uart_flush();
    
    __disable_irq();
    
    // 使用ARM Cortex-M4系统控制寄存器进行复位
    // SCB->AIRCR = 0x05FA0004; // 系统复位请求
    *((volatile uint32_t*)0xE000ED0C) = 0x05FA0004;
    
    // 确保复位指令执行
    __DSB();
    
    // 无限循环，等待复位生效
    while(1) {
        __NOP();
    }
}

/**
 * @brief 延时系统复位
 */
void software_reset_system_delayed(uint32_t delay_ms) {
    printf("[SWRST] System will reset in %lu ms...\\n", delay_ms);
    
    // 简单延时实现
    volatile uint32_t count = delay_ms * 1000; // 假设1MHz循环
    while(count--) {
        __NOP();
    }
    
    software_reset_system_now();
}

/**
 * @brief 打印软件复位状态信息
 */
void software_reset_print_status(void) {
    printf("\\n[SWRST] ===== Software Reset Status =====\\n");
    printf("Module Initialized: %s\\n", g_module_initialized ? "Yes" : "No");
    printf("Last Reset Reason: %s\\n", 
           software_reset_get_reason_string(g_reset_status.last_reason));
    printf("Total Boot Count: %lu\\n", g_reset_status.total_boots);
    printf("Failure Count: %lu\\n", g_reset_status.failure_count);
    printf("Download Flag Active: %s\\n", 
           g_reset_status.download_flag_active ? "Yes" : "No");
    printf("Double Reset Detected: %s\\n", 
           g_reset_status.double_reset_detected ? "Yes" : "No");
    printf("Flash Flag Region: 0x%08X (NVS tail 4KB)\\n", RESET_FLAG_SECTOR_ADDR);
    printf("===================================\\n\\n");
}

/**
 * @brief 获取复位原因字符串
 */
const char* software_reset_get_reason_string(reset_reason_t reason) {
    switch (reason) {
        case RESET_REASON_NONE:         return "None";
        case RESET_REASON_USER_CMD:     return "User Command";
        case RESET_REASON_SERIAL_CMD:   return "Serial Command";
        case RESET_REASON_DOUBLE_RESET: return "Double Reset";
        case RESET_REASON_APP_FAILURE:  return "App Failure";
        case RESET_REASON_WATCHDOG:     return "Watchdog";
        case RESET_REASON_REMOTE_CMD:   return "Remote Command";
        case RESET_REASON_POWER_ON:     return "Power On";
        case RESET_REASON_UNKNOWN:      return "Unknown";
        default:                        return "Invalid";
    }
}

/**
 * @brief 复位所有软件标志
 */
void software_reset_factory_reset_flags(void) {
    printf("[SWRST] WARNING: Performing factory reset of all flags!\\n");
    
    // 擦除整个标志扇区
    if (qspi_flash_erase_sector(RESET_FLAG_SECTOR_ADDR) == 0) {
        printf("[SWRST] All reset flags cleared\\n");
        
        // 清除全局状态
        memset(&g_reset_status, 0, sizeof(g_reset_status));
    } else {
        printf("[SWRST] Error: Failed to clear flags\\n");
    }
}

/**
 * @brief 处理软件复位相关命令
 */
int software_reset_handle_command(const char *cmd) {
    if (!cmd || !g_module_initialized) {
        return -1;
    }
    
    if (strcmp(cmd, "reset") == 0) {
        software_reset_system_now();
        return 0; // 不会执行到这里
    }
    else if (strcmp(cmd, "download") == 0 || strcmp(cmd, "bootloader") == 0) {
        software_reset_enter_download_mode(RESET_REASON_USER_CMD);
        return 0; // 不会执行到这里
    }
    else if (strcmp(cmd, "dfu") == 0) {
        software_reset_enter_download_mode(RESET_REASON_USER_CMD);
        return 0; // 不会执行到这里
    }
    else if (strcmp(cmd, "status") == 0) {
        software_reset_print_status();
        return 0;
    }
    else if (strcmp(cmd, "clear") == 0) {
        software_reset_clear_download_flag();
        return 0;
    }
    else if (strcmp(cmd, "factory_reset") == 0) {
        software_reset_factory_reset_flags();
        return 0;
    }
    else if (strcmp(cmd, "help") == 0) {
        software_reset_print_help();
        return 0;
    }
    
    return -1; // 无效命令
}

/**
 * @brief 打印命令帮助
 */
void software_reset_print_help(void) {
    printf("\\n[SWRST] Available Commands:\\n");
    printf("  reset          - Immediate system reset\\n");
    printf("  download       - Enter download mode\\n");
    printf("  bootloader     - Enter bootloader mode\\n");
    printf("  dfu            - Enter DFU mode\\n");
    printf("  status         - Show reset status\\n");
    printf("  clear          - Clear download flag\\n");
    printf("  factory_reset  - Clear all flags (dangerous!)\\n");
    printf("  help           - Show this help\\n\\n");
}
