/**
 * @file rbl_main.h
 * @brief S300 RBL主头文件
 */

#ifndef RBL_MAIN_H
#define RBL_MAIN_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* RBL配置 */
#include "rbl_config.h"

/* CMSIS头文件 */
#include "core_cm4.h"

/* 启动模式定义 - 扩展支持软件触发模式 */
typedef enum {
    BOOT_MODE_NORMAL = 0,            // 正常启动模式
    BOOT_MODE_DOWNLOAD_GPIO,         // GPIO强制下载模式（保留兼容）
    BOOT_MODE_DOWNLOAD_SERIAL,       // 串口下载模式
    BOOT_MODE_DOWNLOAD_SOFTWARE,     // 软件标志下载模式
    BOOT_MODE_DOWNLOAD_DOUBLE_RESET, // 双重启下载模式
    BOOT_MODE_RECOVERY,              // Flash恢复模式
    BOOT_MODE_BOOT_FAILURE,          // 启动故障恢复模式
} boot_mode_t;

/* SBL信息结构 */
typedef struct {
    uint32_t addr;               // SBL在Flash中的地址
    uint32_t size;               // SBL大小
    uint32_t crc32;              // SBL CRC32校验值
    uint32_t entry;              // SBL入口地址(XIP)
} sbl_info_t;

/* 函数声明 */
void rbl_print_banner(void);
boot_mode_t rbl_detect_boot_mode(void);
boot_mode_t rbl_detect_boot_mode_enhanced(void);  // 新的增强检测函数
int rbl_normal_boot(void);
int rbl_get_sbl_info(sbl_info_t *sbl_info);
int rbl_verify_sbl(const sbl_info_t *sbl_info);
void rbl_jump_to_sbl(uint32_t entry_addr);
bool rbl_check_gpio_download_mode(void);
bool rbl_check_serial_download_window(void);
const char *rbl_get_reset_reason_string(void);
const char *rbl_get_boot_mode_string(boot_mode_t mode);

#endif // RBL_MAIN_H
