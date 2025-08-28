#ifndef S300_BSP_W25QXX_H
#define S300_BSP_W25QXX_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 简单的 W25Qxx 抽象，复用底层 qspi_cadence 提供的 API */

typedef struct
{
    uint8_t manuf_id;   /* 0xEF for Winbond */
    uint8_t memory_type;/* 0x40 for W25Q series */
    uint8_t capacity;   /* 0x18 for 128Mbit */
    uint32_t size_bytes;/* 1<<capacity when capacity>=0x14, else 0 */
    uint32_t page_size; /* 256 */
    uint32_t sector_size; /* 4KB */
    uint32_t block_size; /* 64KB */
    bool quad_enabled;
    bool addr4b;
} w25qxx_info_t;

/* 初始化并读取 ID，必要时配置 QE/4B */
int w25qxx_init(w25qxx_info_t *info, bool want_quad, bool want_4byte_addr);

/* 基础操作 */
int w25qxx_read(uint32_t addr, void *buf, uint32_t len);
int w25qxx_write_page(uint32_t addr, const void *buf, uint32_t len); /* len<=256, 不跨页 */
int w25qxx_erase_4k(uint32_t addr);
int w25qxx_erase_64k(uint32_t addr);
int w25qxx_chip_erase(void);

#ifdef __cplusplus
}
#endif

#endif /* S300_BSP_W25QXX_H */
