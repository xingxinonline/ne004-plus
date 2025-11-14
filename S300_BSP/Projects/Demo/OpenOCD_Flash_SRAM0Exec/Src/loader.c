#include "loader.h"
#include "qspi_cadence.h"
#include <stdint.h>

#define LOADER_TEXT   __attribute__((section(".loader.text"))) __attribute__((noinline))
#define LOADER_DATA   __attribute__((section(".loader.data")))
#define LOADER_BSS    __attribute__((section(".loader.bss")))
#define LOADER_SHARED __attribute__((section(".loader.shared")))

#define QSPI_REG32(offset) (*(volatile uint32_t *)(QSPI_CFG_BASE + (offset)))

#define LOADER_CMD_TIMEOUT  200000u
#define LOADER_WIP_TIMEOUT  1500000u
#define LOADER_PAGE_FALLBACK 256u

LOADER_SHARED loader_mailbox_t g_loader_mailbox = {
    .cmd = LOADER_CMD_NONE,
    .status = LOADER_STATUS_IDLE,
    .addr_bytes = 3u,
    .page_size = LOADER_PAGE_FALLBACK,
    .fifo_wp = 0u,
    .fifo_rp = 0u,
    .fifo_size = LOADER_FIFO_SIZE,
};

LOADER_BSS static uint8_t s_loader_chunk[256];

static LOADER_TEXT void loader_spin(void)
{
    for (volatile uint32_t i = 0; i < 256u; ++i) {
        __asm volatile ("nop");
    }
}

static LOADER_TEXT int loader_wait_idle(void)
{
    for (uint32_t t = 0; t < 500000u; ++t) {
        if ((QSPI_REG32(CQSPI_REG_CONFIG) >> CQSPI_CFG_IDLE_LSB) & 0x1u) {
            return 0;
        }
    }
    return -10;
}

static LOADER_TEXT int loader_exec_cmd(uint32_t cmd)
{
    QSPI_REG32(CQSPI_REG_CMDCTRL) = cmd;
    QSPI_REG32(CQSPI_REG_CMDCTRL) = cmd | CQSPI_CMDCTRL_EXECUTE;
    for (uint32_t t = 0; t < LOADER_CMD_TIMEOUT; ++t) {
        if ((QSPI_REG32(CQSPI_REG_CMDCTRL) & CQSPI_CMDCTRL_INPROGRESS) == 0u) {
            return loader_wait_idle();
        }
    }
    return -11;
}

static LOADER_TEXT uint32_t loader_mask_addr_bytes(uint32_t addr_bytes)
{
    if (addr_bytes < 3u) return 3u;
    if (addr_bytes > 4u) return 4u;
    return addr_bytes;
}

static LOADER_TEXT int loader_read_status(uint8_t opcode, uint8_t *val)
{
    if (!val) return -12;
    uint32_t cmd = ((uint32_t)opcode << CQSPI_CMDCTRL_OPCODE_LSB) |
                   (1u << CQSPI_CMDCTRL_RD_EN_LSB) |
                   ((((1u - 1u) & CQSPI_CMDCTRL_RD_BYTES_MASK) << CQSPI_CMDCTRL_RD_BYTES_LSB));
    int rc = loader_exec_cmd(cmd);
    if (rc != 0) return rc;
    *val = (uint8_t)(QSPI_REG32(CQSPI_REG_CMDREADDATALOWER) & 0xFFu);
    return 0;
}

static LOADER_TEXT int loader_wait_wip_clear(void)
{
    for (uint32_t t = 0; t < LOADER_WIP_TIMEOUT; ++t) {
        uint8_t sr1 = 0u;
        int rc = loader_read_status(W25Q_CMD_RDSR1, &sr1);
        if (rc != 0) return rc;
        if ((sr1 & 0x01u) == 0u) {
            return 0;
        }
        loader_spin();
    }
    return -13;
}

static LOADER_TEXT int loader_write_enable(void)
{
    uint32_t cmd = ((uint32_t)W25Q_CMD_WREN << CQSPI_CMDCTRL_OPCODE_LSB);
    return loader_exec_cmd(cmd);
}

static LOADER_TEXT void loader_pack_u32(uint32_t *dst, const uint8_t *src, uint32_t len)
{
    uint32_t v = 0u;
    for (uint32_t i = 0; i < len; ++i) {
        v |= ((uint32_t)src[i]) << (8u * i);
    }
    *dst = v;
}

static LOADER_TEXT void loader_fifo_pop(loader_mailbox_t *mb, uint8_t *dst, uint32_t len)
{
    uint32_t rp = mb->fifo_rp;
    for (uint32_t i = 0; i < len; ++i) {
        dst[i] = mb->fifo[rp & LOADER_FIFO_MASK];
        ++rp;
    }
    mb->fifo_rp = rp;
}

static LOADER_TEXT void loader_fifo_push(loader_mailbox_t *mb, const uint8_t *src, uint32_t len)
{
    uint32_t wp = mb->fifo_wp;
    for (uint32_t i = 0; i < len; ++i) {
        mb->fifo[wp & LOADER_FIFO_MASK] = src[i];
        ++wp;
    }
    mb->fifo_wp = wp;
}

static LOADER_TEXT uint32_t loader_fifo_used(const loader_mailbox_t *mb)
{
    return mb->fifo_wp - mb->fifo_rp;
}

static LOADER_TEXT uint32_t loader_fifo_space(const loader_mailbox_t *mb)
{
    return mb->fifo_size - loader_fifo_used(mb);
}

static LOADER_TEXT int loader_program_body(uint32_t addr, const uint8_t *src, uint32_t len, uint32_t addr_bytes)
{
    uint32_t done = 0u;
    uint32_t ab = loader_mask_addr_bytes(addr_bytes);
    while (done < len) {
        uint32_t chunk = len - done;
        if (chunk > 8u) chunk = 8u;
        int rc = loader_write_enable();
        if (rc != 0) return rc;
        QSPI_REG32(CQSPI_REG_CMDADDRESS) = addr + done;
        uint32_t lower = 0u;
        uint32_t upper = 0u;
        uint32_t first_part = (chunk > 4u) ? 4u : chunk;
        loader_pack_u32(&lower, src + done, first_part);
        if (chunk > 4u) {
            loader_pack_u32(&upper, src + done + 4u, chunk - 4u);
        }
        QSPI_REG32(CQSPI_REG_CMDWRITEDATALOWER) = lower;
        QSPI_REG32(CQSPI_REG_CMDWRITEDATAUPPER) = upper;
        uint32_t cmd = ((uint32_t)W25Q_CMD_PP << CQSPI_CMDCTRL_OPCODE_LSB) |
                       (1u << CQSPI_CMDCTRL_ADDR_EN_LSB) |
                       ((((ab - 1u) & CQSPI_CMDCTRL_ADD_BYTES_MASK) << CQSPI_CMDCTRL_ADD_BYTES_LSB)) |
                       (1u << CQSPI_CMDCTRL_WR_EN_LSB) |
                       ((((chunk - 1u) & CQSPI_CMDCTRL_WR_BYTES_MASK) << CQSPI_CMDCTRL_WR_BYTES_LSB));
        rc = loader_exec_cmd(cmd);
        if (rc != 0) return rc;
        rc = loader_wait_wip_clear();
        if (rc != 0) return rc;
        done += chunk;
    }
    return 0;
}

static LOADER_TEXT int loader_program_stream(uint32_t addr, const uint8_t *src, uint32_t len,
                                             uint32_t page_size, uint32_t addr_bytes)
{
    uint32_t ps = page_size ? page_size : LOADER_PAGE_FALLBACK;
    uint32_t mask = ps - 1u;
    uint32_t written = 0u;
    while (written < len) {
        uint32_t page_off = (addr + written) & mask;
        uint32_t room = ps - page_off;
        uint32_t chunk = len - written;
        if (chunk > room) chunk = room;
        int rc = loader_program_body(addr + written, src + written, chunk, addr_bytes);
        if (rc != 0) return rc;
        written += chunk;
    }
    return 0;
}

static LOADER_TEXT void loader_unpack32(uint8_t *dst, uint32_t value, uint32_t bytes)
{
    for (uint32_t i = 0; i < bytes; ++i) {
        dst[i] = (uint8_t)((value >> (8u * i)) & 0xFFu);
    }
}

static LOADER_TEXT int loader_read_body(uint32_t addr, uint8_t *dst, uint32_t len, uint32_t addr_bytes)
{
    uint32_t ab = loader_mask_addr_bytes(addr_bytes);
    uint32_t done = 0u;
    while (done < len) {
        uint32_t chunk = len - done;
        if (chunk > 8u) chunk = 8u;
        QSPI_REG32(CQSPI_REG_CMDADDRESS) = addr + done;
        uint32_t cmd = ((uint32_t)W25Q_CMD_FAST << CQSPI_CMDCTRL_OPCODE_LSB) |
                       (1u << CQSPI_CMDCTRL_ADDR_EN_LSB) |
                       ((((ab - 1u) & CQSPI_CMDCTRL_ADD_BYTES_MASK) << CQSPI_CMDCTRL_ADD_BYTES_LSB)) |
                       (1u << CQSPI_CMDCTRL_RD_EN_LSB) |
                       ((((chunk - 1u) & CQSPI_CMDCTRL_RD_BYTES_MASK) << CQSPI_CMDCTRL_RD_BYTES_LSB)) |
                       (8u << CQSPI_CMDCTRL_DUMMY_LSB);
        int rc = loader_exec_cmd(cmd);
        if (rc != 0) return rc;
        uint32_t lower = QSPI_REG32(CQSPI_REG_CMDREADDATALOWER);
        uint32_t to_copy = (chunk > 4u) ? 4u : chunk;
        loader_unpack32(dst + done, lower, to_copy);
        if (chunk > 4u) {
            uint32_t upper = QSPI_REG32(CQSPI_REG_CMDREADDATAUPPER);
            loader_unpack32(dst + done + 4u, upper, chunk - 4u);
        }
        done += chunk;
    }
    return 0;
}

static LOADER_TEXT int loader_handle_write(loader_mailbox_t *mb)
{
    uint32_t addr = mb->flash_addr;
    uint32_t remain = mb->length;
    while (remain) {
        while (loader_fifo_used(mb) == 0u) {
            loader_spin();
        }
        uint32_t available = loader_fifo_used(mb);
        uint32_t chunk = (remain < available) ? remain : available;
        if (chunk > sizeof(s_loader_chunk)) {
            chunk = sizeof(s_loader_chunk);
        }
        loader_fifo_pop(mb, s_loader_chunk, chunk);
        int rc = loader_program_stream(addr, s_loader_chunk, chunk, mb->page_size, mb->addr_bytes);
        if (rc != 0) return rc;
        addr += chunk;
        remain -= chunk;
        mb->progress += chunk;
    }
    return 0;
}

static LOADER_TEXT int loader_handle_read(loader_mailbox_t *mb)
{
    uint32_t addr = mb->flash_addr;
    uint32_t remain = mb->length;
    while (remain) {
        while (loader_fifo_space(mb) == 0u) {
            loader_spin();
        }
        uint32_t room = loader_fifo_space(mb);
        uint32_t chunk = (remain < room) ? remain : room;
        if (chunk > sizeof(s_loader_chunk)) {
            chunk = sizeof(s_loader_chunk);
        }
        int rc = loader_read_body(addr, s_loader_chunk, chunk, mb->addr_bytes);
        if (rc != 0) return rc;
        loader_fifo_push(mb, s_loader_chunk, chunk);
        addr += chunk;
        remain -= chunk;
        mb->progress += chunk;
    }
    return 0;
}

LOADER_TEXT void loader_task(void *param)
{
    (void)param;
    loader_mailbox_t *mb = &g_loader_mailbox;
    for (;;) {
        if (mb->cmd == LOADER_CMD_NONE) {
            loader_spin();
            continue;
        }
        mb->status = LOADER_STATUS_BUSY;
        mb->progress = 0u;
        mb->error_code = 0u;
        int rc = 0;
        if (mb->cmd == LOADER_CMD_WRITE) {
            rc = loader_handle_write(mb);
        } else if (mb->cmd == LOADER_CMD_READ) {
            rc = loader_handle_read(mb);
        } else {
            rc = -50;
        }
        if (rc == 0) {
            mb->status = LOADER_STATUS_DONE;
        } else {
            mb->status = LOADER_STATUS_ERROR;
            mb->error_code = (uint32_t)rc;
        }
        mb->cmd = LOADER_CMD_NONE;
    }
}
