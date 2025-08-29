/**
 * @file rbl_qspi.h
 * @brief RBL QSPI Flash驱动头文件
 */

#ifndef RBL_QSPI_H
#define RBL_QSPI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Flash信息结构体 */
typedef struct {
    uint32_t id;            /* Flash ID */
    uint32_t size;          /* Flash大小 */
    uint32_t sector_size;   /* 扇区大小 */
    uint32_t page_size;     /* 页大小 */
} rbl_flash_info_t;

/* 初始化和配置 */
int rbl_qspi_init(void);
void rbl_qspi_deinit(void);

/* Flash信息 */
int rbl_qspi_read_id(uint32_t *id);
int rbl_qspi_get_info(rbl_flash_info_t *info);

/* 状态检查 */
bool rbl_qspi_is_busy(void);
int rbl_qspi_wait_ready(uint32_t timeout_ms);

/* 数据操作 */
int rbl_qspi_read(uint32_t addr, uint8_t *buffer, uint32_t length);
int rbl_qspi_write(uint32_t addr, const uint8_t *data, uint32_t length);

/* 擦除操作 */
int rbl_qspi_erase_sector(uint32_t addr);
int rbl_qspi_erase_block(uint32_t addr);
int rbl_qspi_erase_chip(void);

/* 高级配置 */
int rbl_qspi_set_quad_mode(bool enable);

#ifdef __cplusplus
}
#endif

#endif // RBL_QSPI_H
