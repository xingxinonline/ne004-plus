#ifndef S300_FLASH_IF_H
#define S300_FLASH_IF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 Minimal flash interface for bootloader
 - Address space is the XIP mapping starting at 0x0800_0000
 - erase/write are placeholders until the SPI NOR driver is integrated
*/

/* Initialize flash interface (clock/mux if needed) */
int flash_if_init(void);

/* Read from XIP-mapped flash into buffer */
int flash_if_read(uint32_t addr, void *buf, uint32_t len);

/* Erase flash region [addr, addr+len), sector aligned as required by device */
int flash_if_erase(uint32_t addr, uint32_t len);

/* Write raw payload to flash (handles page alignment inside) */
int flash_if_write(uint32_t addr, const void *data, uint32_t len);

/* Compute CRC32 over flash region */
uint32_t flash_if_crc32(uint32_t addr, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* S300_FLASH_IF_H */
