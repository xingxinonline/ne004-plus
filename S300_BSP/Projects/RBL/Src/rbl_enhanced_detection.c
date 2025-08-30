/**
 * @file rbl_enhanced_detection.c
 * @brief RBL增强启动模式检测
 * @version 1.0
 * @date 2025-08-30
 */

#include "rbl_main.h"
#include "rbl_download_flags.h"
#include "rbl_system.h"
#include "rbl_uart.h"
#include <string.h>

/**
 * @brief 增强版启动模式检测
 */
boot_mode_t rbl_detect_boot_mode_enhanced(void)
{
    printf("[RBL] Enhanced boot mode detection starting...\r\n");
    
    /* 1. 检查软件下载标志（最高优先级） */
    if (rbl_flash_check_download_flag()) {
        printf("[RBL] Software download flag detected\r\n");
        return BOOT_MODE_DOWNLOAD_SOFTWARE;
    }
    
    /* 2. 检查双重启动 */
    if (rbl_check_double_reset()) {
        printf("[RBL] Double reset detected\r\n");
        return BOOT_MODE_DOWNLOAD_DOUBLE_RESET;
    }
    
    /* 3. 检查启动故障 */
    if (rbl_check_boot_failure()) {
        printf("[RBL] Boot failure detected\r\n");
        return BOOT_MODE_BOOT_FAILURE;
    }
    
    /* 4. 更新启动计数 */
    rbl_increment_boot_attempts();
    
    /* 5. 检查传统Flash恢复标志（兼容性） */
    if (rbl_flash_check_recovery_flag()) {
        printf("[RBL] Legacy recovery flag detected\r\n");
        return BOOT_MODE_RECOVERY;
    }
    
    /* 6. 检查增强串口下载窗口 */
    if (rbl_check_enhanced_serial_window()) {
        printf("[RBL] Enhanced serial download activated\r\n");
        return BOOT_MODE_DOWNLOAD_SERIAL;
    }
    
    /* 7. 清除成功启动的计数（正常模式才清除） */
    rbl_clear_boot_attempts();
    
    printf("[RBL] Normal boot mode selected\r\n");
    return BOOT_MODE_NORMAL;
}

/**
 * @brief 增强版串口检测窗口
 */
bool rbl_check_enhanced_serial_window(void)
{
    uint32_t start_time = rbl_system_get_tick_ms();
    const uint32_t timeout_ms = 5000;  // 5秒超时
    uint8_t rx_data;
    char command_buffer[32] = {0};
    int cmd_index = 0;
    uint32_t last_second = 0;
    bool key_pressed = false;
    
    printf("\r\n");
    printf("╔════════════════════════════════════════════════════════╗\r\n");
    printf("║             S300 RBL - Download Mode Window            ║\r\n");
    printf("╠════════════════════════════════════════════════════════╣\r\n");
    printf("║  Press ANY KEY within 5 seconds to enter download     ║\r\n");
    printf("║  Send 'DOWNLOAD' command to force download mode       ║\r\n");
    printf("║  Double-press reset button for emergency recovery     ║\r\n");
    printf("╚════════════════════════════════════════════════════════╝\r\n");
    
    while ((rbl_system_get_tick_ms() - start_time) < timeout_ms) {
        /* 检查串口输入 */
        if (rbl_uart_read_nonblock(&rx_data, 1) > 0) {
            
            /* 首次按键立即进入下载模式 */
            if (!key_pressed) {
                printf("\r\n[RBL] Key detected (0x%02X)! Entering download mode...\r\n", rx_data);
                return true;
            }
            
            /* 收集命令字符 */
            if (rx_data >= ' ' && rx_data <= '~' && cmd_index < 31) {
                command_buffer[cmd_index++] = rx_data;
                command_buffer[cmd_index] = '\0';
                
                /* 检查特定命令 */
                if (strstr(command_buffer, "DOWNLOAD")) {
                    printf("\r\n[RBL] DOWNLOAD command received!\r\n");
                    return true;
                }
                
                if (strstr(command_buffer, "RECOVERY")) {
                    printf("\r\n[RBL] RECOVERY command received!\r\n");
                    /* 设置恢复标志并重启 */
                    rbl_flash_set_download_flag(DOWNLOAD_REASON_USER_REQUEST);
                    rbl_system_delay_ms(100);
                    rbl_system_reset();
                }
            }
            
            key_pressed = true;
        }
        
        /* 倒计时显示 */
        uint32_t elapsed = rbl_system_get_tick_ms() - start_time;
        uint32_t remaining_sec = (timeout_ms - elapsed) / 1000;
        
        if (remaining_sec != last_second) {
            printf("\r[INFO] Countdown: %lu seconds remaining", 
                   (unsigned long)remaining_sec);
            last_second = remaining_sec;
        }
        
        /* 短暂延时 */
        rbl_system_delay_ms(50);
    }
    
    printf("\r\n[INFO] Timeout reached. Continuing normal boot...\r\n");
    return false;
}

/**
 * @brief 获取增强版启动模式字符串
 */
const char *rbl_get_boot_mode_string_enhanced(boot_mode_t mode)
{
    switch (mode) {
        case BOOT_MODE_NORMAL:              return "Normal Boot";
        case BOOT_MODE_DOWNLOAD_GPIO:       return "GPIO Download (Legacy)";
        case BOOT_MODE_DOWNLOAD_SERIAL:     return "Serial Download";
        case BOOT_MODE_DOWNLOAD_SOFTWARE:   return "Software Download";
        case BOOT_MODE_DOWNLOAD_DOUBLE_RESET: return "Double Reset Download";
        case BOOT_MODE_RECOVERY:            return "Flash Recovery";
        case BOOT_MODE_BOOT_FAILURE:        return "Boot Failure Recovery";
        default:                            return "Unknown Mode";
    }
}

/**
 * @brief 处理增强下载模式
 */
int rbl_handle_enhanced_download_mode(boot_mode_t mode)
{
    int ret;
    
    printf("[RBL] Entering enhanced download mode: %s\r\n", 
           rbl_get_boot_mode_string_enhanced(mode));
    
    /* 清除触发标志，避免重复进入 */
    switch (mode) {
        case BOOT_MODE_DOWNLOAD_SOFTWARE:
        case BOOT_MODE_DOWNLOAD_DOUBLE_RESET:
            rbl_flash_clear_download_flag();
            break;
            
        case BOOT_MODE_BOOT_FAILURE:
            rbl_clear_boot_attempts();
            break;
            
        default:
            break;
    }
    
    /* 执行下载流程 */
    ret = rbl_download_mode();
    
    if (ret == 0) {
        printf("[RBL] Download completed successfully\r\n");
        printf("[RBL] System will restart in 2 seconds...\r\n");
        rbl_system_delay_ms(2000);
        rbl_system_reset();
    } else {
        printf("[RBL] Download failed with error: %d\r\n", ret);
        
        /* 下载失败时的处理 */
        printf("[RBL] Falling back to normal boot attempt...\r\n");
        return ret;
    }
    
    return 0;
}

/**
 * @brief 应用程序API：触发下载模式
 * 这个函数可以被应用程序调用来触发下载模式
 */
void app_trigger_download_mode(download_reason_t reason)
{
    printf("[APP] Triggering download mode: %s\r\n", 
           rbl_get_download_reason_string(reason));
    
    /* 设置下载标志 */
    rbl_flash_set_download_flag(reason);
    
    /* 短暂延时确保Flash写入完成 */
    rbl_system_delay_ms(100);
    
    /* 软件复位 */
    rbl_system_reset();
}

/**
 * @brief 应用程序API：报告应用成功启动
 * 应用程序启动完成后应该调用此函数
 */
void app_report_successful_boot(void)
{
    /* 清除启动失败计数 */
    rbl_clear_boot_attempts();
    
    printf("[APP] Successful boot reported to RBL\r\n");
}
