/* Header for flash operations PIC code */

#ifndef FLASH_OPS_H
#define FLASH_OPS_H

#include <stdint.h>

/* Function types matching flash_ops.S */
typedef int (*flash_read_fn_t)(uint32_t addr, uint8_t *buf, uint32_t len, uint32_t qspi_base);
typedef int (*flash_write_fn_t)(uint32_t addr, const uint8_t *buf, uint32_t len, uint32_t qspi_base);
typedef int (*flash_erase_fn_t)(uint32_t addr, uint32_t type, uint32_t qspi_base);
typedef uint32_t (*crc32_fn_t)(const uint8_t *data, uint32_t len);

/* Erase types */
#define FLASH_ERASE_4K      0
#define FLASH_ERASE_32K     1
#define FLASH_ERASE_64K     2

/* Function table structure */
typedef struct {
    flash_read_fn_t read;
    flash_write_fn_t write;
    flash_erase_fn_t erase;
    crc32_fn_t crc32;
} flash_ops_table_t;

/* This will be included from generated flash_ops.inc */
#ifndef FLASH_OPS_CODE_DEFINED
extern const uint8_t flash_ops_code[];
extern const uint32_t flash_ops_code_size;
#endif

#endif /* FLASH_OPS_H */
