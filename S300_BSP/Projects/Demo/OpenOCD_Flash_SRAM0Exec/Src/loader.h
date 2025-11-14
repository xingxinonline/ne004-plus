#ifndef OPENOCD_FLASH_SRAM0EXEC_LOADER_H
#define OPENOCD_FLASH_SRAM0EXEC_LOADER_H

#include <stdint.h>

#define LOADER_FIFO_SIZE 1024u
#define LOADER_FIFO_MASK (LOADER_FIFO_SIZE - 1u)

enum {
    LOADER_CMD_NONE  = 0u,
    LOADER_CMD_READ  = 1u,
    LOADER_CMD_WRITE = 2u,
};

enum {
    LOADER_STATUS_IDLE  = 0u,
    LOADER_STATUS_BUSY  = 1u,
    LOADER_STATUS_DONE  = 2u,
    LOADER_STATUS_ERROR = 3u,
};

typedef struct {
    volatile uint32_t cmd;
    volatile uint32_t status;
    volatile uint32_t error_code;
    volatile uint32_t flash_addr;
    volatile uint32_t length;
    volatile uint32_t progress;
    volatile uint32_t addr_bytes;
    volatile uint32_t page_size;
    volatile uint32_t fifo_wp;
    volatile uint32_t fifo_rp;
    uint32_t fifo_size;
    uint8_t fifo[LOADER_FIFO_SIZE];
} loader_mailbox_t;

extern loader_mailbox_t g_loader_mailbox;

void loader_task(void *param);

#endif /* OPENOCD_FLASH_SRAM0EXEC_LOADER_H */
