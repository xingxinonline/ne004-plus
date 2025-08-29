/**
 * @file rbl_flash.h
 * @brief RBL Flash高级操作头文件
 */

#ifndef __RBL_FLASH_H__
#define __RBL_FLASH_H__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Flash扇区大小 */
#define RBL_FLASH_SECTOR_SIZE       4096

/* 恢复模式相关 */
bool rbl_flash_check_recovery_flag(void);
int rbl_flash_set_recovery_flag(void);
int rbl_flash_clear_recovery_flag(void);

/* CRC计算和验证 */
int rbl_flash_calculate_crc32(uint32_t addr, uint32_t length, uint32_t *crc);
int rbl_flash_verify_region(uint32_t addr, uint32_t length, uint32_t expected_crc);

/* 安全操作 */
int rbl_flash_write_safe(uint32_t addr, const uint8_t *data, uint32_t length);
int rbl_flash_erase_region(uint32_t addr, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* __RBL_FLASH_H__ */
