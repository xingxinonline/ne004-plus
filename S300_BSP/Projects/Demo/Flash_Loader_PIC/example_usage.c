/*
 * Example: How to integrate flash_ops PIC code
 * 
 * This shows the typical usage pattern for the flash loader
 */

#include <stdint.h>
#include <string.h>
#include "flash_ops.h"

/* Auto-generated from flash_ops.bin */
#include "flash_ops.inc"

#define SRAM0_LOAD_ADDR  0x1FFF1000u
#define QSPI_BASE        0x50000000u  /* Example address */

/* Example 1: Simple read operation */
void example_flash_read(void)
{
    uint8_t buffer[256];
    
    /* 1. Copy PIC code to SRAM0 */
    memcpy((void*)SRAM0_LOAD_ADDR, flash_ops_code, sizeof(flash_ops_code));
    
    /* Ensure code is written before execution */
    __DSB();
    __ISB();
    
    /* 2. Get function pointer (with Thumb bit set) */
    flash_read_fn_t flash_read = (flash_read_fn_t)(SRAM0_LOAD_ADDR | 1);
    
    /* 3. Call the function */
    int result = flash_read(
        0x100000,       /* Flash address to read */
        buffer,         /* Destination buffer */
        256,            /* Number of bytes */
        QSPI_BASE       /* QSPI controller base */
    );
    
    if (result == 0) {
        /* Success - buffer contains data */
    }
}

/* Example 2: Using the function table */
void example_function_table(void)
{
    flash_ops_table_t *ops;
    uint32_t crc;
    uint8_t data[512];
    
    /* Load code */
    memcpy((void*)SRAM0_LOAD_ADDR, flash_ops_code, sizeof(flash_ops_code));
    __DSB();
    __ISB();
    
    /* Get function table (offset 0x150 in flash_ops.S) */
    ops = (flash_ops_table_t *)(SRAM0_LOAD_ADDR + 0x150);
    
    /* Read data */
    ops->read(0x100000, data, 512, QSPI_BASE);
    
    /* Calculate CRC */
    crc = ops->crc32(data, 512);
    
    /* Erase sector */
    ops->erase(0x100000, FLASH_ERASE_4K, QSPI_BASE);
    
    /* Write data */
    ops->write(0x100000, data, 256, QSPI_BASE);
}

/* Example 3: Testing PIC property */
void example_test_pic(void)
{
    uint8_t buffer1[32], buffer2[32];
    int result1, result2;
    
    /* Test 1: Load at address A */
    uint32_t addr1 = SRAM0_LOAD_ADDR;
    memcpy((void*)addr1, flash_ops_code, sizeof(flash_ops_code));
    __DSB(); __ISB();
    
    flash_read_fn_t fn1 = (flash_read_fn_t)(addr1 | 1);
    result1 = fn1(0x100000, buffer1, 32, QSPI_BASE);
    
    /* Test 2: Load at different address B */
    uint32_t addr2 = SRAM0_LOAD_ADDR + 0x1000;
    memcpy((void*)addr2, flash_ops_code, sizeof(flash_ops_code));
    __DSB(); __ISB();
    
    flash_read_fn_t fn2 = (flash_read_fn_t)(addr2 | 1);
    result2 = fn2(0x100000, buffer2, 32, QSPI_BASE);
    
    /* Both should produce identical results */
    if (result1 == result2 && memcmp(buffer1, buffer2, 32) == 0) {
        /* PIC property verified! */
    }
}

/* Example 4: OpenOCD-style loader stub */
typedef struct {
    uint32_t flash_addr;
    uint32_t buffer_addr;
    uint32_t count;
    volatile uint32_t status;  /* 0=busy, 1=done, -1=error */
} loader_params_t;

void example_openocd_stub(void)
{
    /* This would be uploaded by OpenOCD to SRAM */
    static loader_params_t params;
    
    /* Load flash ops to SRAM0 */
    memcpy((void*)SRAM0_LOAD_ADDR, flash_ops_code, sizeof(flash_ops_code));
    __DSB();
    __ISB();
    
    flash_read_fn_t flash_read = (flash_read_fn_t)(SRAM0_LOAD_ADDR | 1);
    
    /* Wait for parameters from OpenOCD */
    params.status = 0;  /* busy */
    
    /* Execute operation */
    int result = flash_read(
        params.flash_addr,
        (uint8_t*)params.buffer_addr,
        params.count,
        QSPI_BASE
    );
    
    /* Signal completion to OpenOCD */
    params.status = (result == 0) ? 1 : -1;
    
    /* OpenOCD polls params.status and reads buffer when done */
}
