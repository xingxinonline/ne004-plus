/**
 * @file rbl_download_simple.c
 * @brief RBL下载协议实现 - 支持Ymodem和简单命令
 */

#include "rbl_download.h"
#include "rbl_uart.h"
#include "rbl_system.h"
#include "rbl_ymodem.h"
#include "rbl_qspi.h"
#include "rbl_flash.h"
#include <string.h>

/* 下载状态 */
static struct {
    bool active;
    uint32_t total_bytes;
    uint32_t written_bytes;
} g_download_state;

/**
 * @brief Flash写入回调函数
 */
static ymodem_result_t flash_write_callback(uint32_t addr, const uint8_t *data, uint32_t size)
{
    /* 检查地址范围 */
    if (addr < 0x10000 || addr >= 0x1000000) {
        printf("[RBL] Invalid flash address: 0x%08lX\r\n", (unsigned long)addr);
        return YMODEM_FLASH_ERROR;
    }
    
    /* 写入Flash */
    if (rbl_qspi_write(addr, data, size) != 0) {
        printf("[RBL] Flash write failed at 0x%08lX\r\n", (unsigned long)addr);
        return YMODEM_FLASH_ERROR;
    }
    
    return YMODEM_OK;
}

/**
 * @brief 进度回调函数
 */
static void progress_callback(uint32_t received, uint32_t total)
{
    if (total > 0) {
        uint32_t percent = (received * 100) / total;
        printf("[RBL] Progress: %lu/%lu bytes (%lu%%)\r\n", 
               (unsigned long)received, (unsigned long)total, (unsigned long)percent);
    } else {
        printf("[RBL] Received: %lu bytes\r\n", (unsigned long)received);
    }
}

/**
 * @brief 擦除Flash扇区
 */
static int erase_flash_range(uint32_t start_addr, uint32_t size)
{
    uint32_t sector_size = 4096;  /* 4KB扇区 */
    uint32_t addr = start_addr & ~(sector_size - 1);  /* 对齐到扇区边界 */
    uint32_t end_addr = start_addr + size;
    
    printf("[RBL] Erasing flash from 0x%08lX to 0x%08lX...\r\n", 
           (unsigned long)addr, (unsigned long)end_addr);
    
    while (addr < end_addr) {
        if (rbl_qspi_erase_sector(addr) != 0) {
            printf("[RBL] Failed to erase sector at 0x%08lX\r\n", (unsigned long)addr);
            return -1;
        }
        addr += sector_size;
        
        /* 显示擦除进度 */
        if ((addr % (64 * 1024)) == 0) {
            printf("[RBL] Erased: 0x%08lX\r\n", (unsigned long)addr);
        }
    }
    
    printf("[RBL] Flash erase completed\r\n");
    return 0;
}

/**
 * @brief 下载模式主函数
 */
int rbl_download_mode(void)
{
    printf("\r\n");
    printf("=================================================\r\n");
    printf("S300 RBL Download Mode - Ymodem Protocol Support\r\n");
    printf("=================================================\r\n");
    printf("Commands:\r\n");
    printf("  INFO     - Show chip information\r\n");
    printf("  YMODEM   - Start Ymodem file reception\r\n");
    printf("  ERASE    - Erase application area\r\n");
    printf("  QUIT     - Exit download mode\r\n");
    printf("=================================================\r\n");
    
    /* 初始化下载状态 */
    g_download_state.active = true;
    g_download_state.total_bytes = 0;
    g_download_state.written_bytes = 0;
    
    /* 发送就绪信号 */
    rbl_send_ready_signal();
    
    /* 命令处理循环 */
    char cmd_buffer[64];
    int cmd_len = 0;
    
    while (g_download_state.active) {
        uint8_t ch;
        if (rbl_uart_read_nonblock(&ch, 1) > 0) {
            if (ch == '\r' || ch == '\n') {
                if (cmd_len > 0) {
                    cmd_buffer[cmd_len] = '\0';
                    
                    if (strcmp(cmd_buffer, "QUIT") == 0) {
                        printf("[RBL] Exiting download mode\r\n");
                        g_download_state.active = false;
                    } 
                    else if (strcmp(cmd_buffer, "INFO") == 0) {
                        /* 获取系统信息 */
                        rbl_system_info_t sys_info;
                        uint32_t flash_id;
                        
                        printf("[RBL] ================== System Information ==================\r\n");
                        
                        /* 基本芯片信息 */
                        printf("[RBL] Chip: PiMCHIP S300 (ARM Cortex-M4)\r\n");
                        
                        if (rbl_get_system_info(&sys_info) == 0) {
                            printf("[RBL] CPU ID: 0x%08lX\r\n", (unsigned long)sys_info.cpu_id);
                            printf("[RBL] System Clock: %lu MHz\r\n", (unsigned long)(sys_info.system_clock / 1000000));
                            printf("[RBL] SRAM Size: %lu KB\r\n", (unsigned long)(sys_info.sram_size / 1024));
                            printf("[RBL] Flash Size: %lu MB\r\n", (unsigned long)(sys_info.flash_size / 1024 / 1024));
                            printf("[RBL] Uptime: %lu ms\r\n", (unsigned long)sys_info.tick_count);
                        }
                        
                        /* Flash信息 */
                        if (rbl_qspi_read_id(&flash_id) == 0) {
                            printf("[RBL] Flash ID: 0x%06lX\r\n", (unsigned long)flash_id);
                            if (flash_id == 0xEF4018) {
                                printf("[RBL] Flash Type: Winbond W25Q128JW\r\n");
                            } else {
                                printf("[RBL] Flash Type: Unknown\r\n");
                            }
                        }
                        
                        /* 内存布局 */
                        printf("[RBL] ================== Memory Layout ==================\r\n");
                        printf("[RBL] RBL Area:       0x000000 - 0x00FFFF (64KB)\r\n");
                        printf("[RBL] SBL Area:       0x010000 - 0x02FFFF (128KB)\r\n");
                        printf("[RBL] App Area:       0x030000 - 0xFFFFFF (~15.8MB)\r\n");
                        printf("[RBL] SRAM Exec:      0x80000000 - 0x8003FFFF (256KB)\r\n");
                        
                        /* 版本信息 */
                        printf("[RBL] ================== Version Information ================\r\n");
                        printf("[RBL] RBL Version: v1.0.0\r\n");
                        printf("[RBL] Build Date: %s %s\r\n", __DATE__, __TIME__);
                        printf("[RBL] Features: Ymodem, CRC32, Flash Protection\r\n");
                        printf("[RBL] =====================================================\r\n");
                    }
                    else if (strcmp(cmd_buffer, "YMODEM") == 0) {
                        printf("[RBL] Starting Ymodem reception...\r\n");
                        printf("[RBL] Please send file using Ymodem protocol\r\n");
                        
                        ymodem_result_t result = ymodem_receive(0x10000, /* SBL起始地址 */
                                                               flash_write_callback,
                                                               progress_callback);
                        
                        switch (result) {
                            case YMODEM_OK:
                                printf("[RBL] File received successfully!\r\n");
                                break;
                            case YMODEM_CANCEL:
                                printf("[RBL] Transfer cancelled\r\n");
                                break;
                            case YMODEM_TIMEOUT:
                                printf("[RBL] Transfer timeout\r\n");
                                break;
                            case YMODEM_FLASH_ERROR:
                                printf("[RBL] Flash write error\r\n");
                                break;
                            default:
                                printf("[RBL] Transfer failed: %d\r\n", result);
                                break;
                        }
                    }
                    else if (strcmp(cmd_buffer, "ERASE") == 0) {
                        printf("[RBL] Erasing application area...\r\n");
                        if (erase_flash_range(0x10000, 512 * 1024) == 0) {
                            printf("[RBL] Erase completed\r\n");
                        } else {
                            printf("[RBL] Erase failed\r\n");
                        }
                    }
                    else {
                        printf("[RBL] Unknown command: %s\r\n", cmd_buffer);
                        printf("[RBL] Available: INFO, YMODEM, ERASE, QUIT\r\n");
                    }
                    
                    cmd_len = 0;
                    printf("\r\n> ");
                }
            } else if (ch >= 32 && ch < 127 && cmd_len < 63) {
                cmd_buffer[cmd_len++] = ch;
                /* 回显字符 */
                rbl_uart_putchar(ch);
            } else if (ch == 8 || ch == 127) { /* 退格键 */
                if (cmd_len > 0) {
                    cmd_len--;
                    printf("\b \b");  /* 删除字符 */
                }
            }
        }
        
        rbl_system_delay_ms(10);
    }
    
    printf("[RBL] Download mode finished\r\n");
    return 0;
}

/**
 * @brief 发送就绪信号
 */
void rbl_send_ready_signal(void)
{
    const char *ready_msg = "\r\n[RBL READY] Type commands or use Ymodem to upload firmware\r\n> ";
    rbl_uart_write((uint8_t*)ready_msg, strlen(ready_msg));
    rbl_uart_flush();
}
