#include "flash_if.h"
#include "s300.h"
#include "s300_bsp.h"
#include <string.h>

/* External NOR map assumed at 0x0800_0000 via XIP */
#define XIP_FLASH_BASE (0x08000000u)

/* Winbond W25Q128 instructions */
#define W25_CMD_WREN   0x06
#define W25_CMD_WRDI   0x04
#define W25_CMD_RDSR1  0x05
#define W25_CMD_PP     0x02   /* Page Program (up to 256B) */
#define W25_CMD_SECTOR_ERASE 0x20 /* 4KB */
#define W25_CMD_BLOCK_ERASE  0xD8 /* 64KB */
#define W25_CMD_CHIP_ERASE   0xC7

#define W25_SR1_WIP    (1u << 0)
#define W25_SR1_WEL    (1u << 1)

/* Cadence QSPI register access */
#define QSPI_REG(off) (*(volatile uint32_t *)(QSPI_CFG_BASE + (off)))

static int qspi_exec_cmd(uint32_t reg)
{
    /* write reg to CMDCTRL, set EXECUTE, poll INPROGRESS clear */
    QSPI_REG(CQSPI_REG_CMDCTRL) = reg;
    QSPI_REG(CQSPI_REG_CMDCTRL) = reg | CQSPI_REG_CMDCTRL_EXECUTE;
    /* simple timeout */
    for (uint32_t t = 0; t < 100000; ++t)
    {
        uint32_t v = QSPI_REG(CQSPI_REG_CMDCTRL);
        if ((v & CQSPI_REG_CMDCTRL_INPROGRESS) == 0) break;
    }
    /* flush */
    QSPI_REG(CQSPI_REG_CMDCTRL) = 0;
    /* wait idle */
    for (uint32_t t = 0; t < 100000; ++t)
    {
        if (CQSPI_IS_IDLE()) return 0;
    }
    return -1;
}

static int qspi_cmd_only(uint8_t opcode)
{
    uint32_t reg = ((uint32_t)opcode) << CQSPI_REG_CMDCTRL_OPCODE_LSB;
    return qspi_exec_cmd(reg);
}

static int qspi_cmd_addr(uint8_t opcode, uint32_t addr, int addr_bytes)
{
    QSPI_REG(CQSPI_REG_CMDADDRESS) = addr;
    uint32_t reg = ((uint32_t)opcode) << CQSPI_REG_CMDCTRL_OPCODE_LSB;
    reg |= (uint32_t)((addr_bytes - 1) & CQSPI_REG_CMDCTRL_ADD_BYTES_MASK) << CQSPI_REG_CMDCTRL_ADD_BYTES_LSB;
    reg |= (1u << CQSPI_REG_CMDCTRL_ADDR_EN_LSB);
    return qspi_exec_cmd(reg);
}

static int qspi_cmd_addr_write(uint8_t opcode, uint32_t addr, int addr_bytes, const uint8_t *data, uint32_t len)
{
    QSPI_REG(CQSPI_REG_CMDADDRESS) = addr;
    uint32_t reg = ((uint32_t)opcode) << CQSPI_REG_CMDCTRL_OPCODE_LSB;
    reg |= (uint32_t)((addr_bytes - 1) & CQSPI_REG_CMDCTRL_ADD_BYTES_MASK) << CQSPI_REG_CMDCTRL_ADD_BYTES_LSB;
    reg |= (1u << CQSPI_REG_CMDCTRL_ADDR_EN_LSB);
    if (len)
    {
        uint32_t wr = 0;
        uint32_t wr_len = (len > 4) ? 4 : len;
        /* lower */
        memcpy(&wr, data, wr_len);
        QSPI_REG(CQSPI_REG_CMDWRITEDATALOWER) = wr;
        if (len > 4)
        {
            wr = 0;
            wr_len = len - wr_len; /* remaining <=4 expected in STIG */
            memcpy(&wr, data + 4, wr_len);
            QSPI_REG(CQSPI_REG_CMDWRITEDATAUPPER) = wr;
        }
        reg |= (1u << CQSPI_REG_CMDCTRL_WR_EN_LSB);
        reg |= (uint32_t)((len - 1) & CQSPI_REG_CMDCTRL_WR_BYTES_MASK) << CQSPI_REG_CMDCTRL_WR_BYTES_LSB;
    }
    return qspi_exec_cmd(reg);
}

static int w25_wait_wip_clear(uint32_t timeout_ms)
{
    uint32_t start = S300_SysTick_Millis();
    for (;;)
    {
        uint32_t reg = ((uint32_t)W25_CMD_RDSR1) << CQSPI_REG_CMDCTRL_OPCODE_LSB;
        reg |= (1u << CQSPI_REG_CMDCTRL_RD_EN_LSB);
        reg |= (0 /* 1 byte */) << CQSPI_REG_CMDCTRL_RD_BYTES_LSB;
        if (qspi_exec_cmd(reg) != 0) return -1;
        uint32_t val = QSPI_REG(CQSPI_REG_CMDREADDATALOWER) & 0xFFu;
        if ((val & W25_SR1_WIP) == 0) return 0;
        if (timeout_ms && (S300_SysTick_Millis() - start) > timeout_ms) return -1;
    }
}

static uint32_t crc32_update(uint32_t crc, uint8_t byte)
{
    crc ^= ((uint32_t)byte) << 24;
    for (int b = 0; b < 8; ++b)
    {
        if (crc & 0x80000000u) crc = (crc << 1) ^ 0x04C11DB7u;
        else crc <<= 1;
    }
    return crc;
}

int flash_if_init(void)
{
    /* Minimal bring-up: ensure controller enabled, 24MHz safe, 3-byte addr mode. */
    /* Assume clocks/mux set by ROM; ensure enable bit set */
    uint32_t cfg = QSPI_REG(CQSPI_REG_CONFIG);
    cfg |= CQSPI_REG_CONFIG_ENABLE;
    /* Single CS0 without decoder */
    cfg &= ~(CQSPI_REG_CONFIG_DECODE);
    cfg &= ~(CQSPI_REG_CONFIG_CHIPSELECT_MASK << CQSPI_REG_CONFIG_CHIPSELECT_LSB);
    cfg |= (0xE & CQSPI_REG_CONFIG_CHIPSELECT_MASK) << CQSPI_REG_CONFIG_CHIPSELECT_LSB; /* CS0 active */
    /* leave DIRECT off for STIG, XiP mapping still valid via system */
    QSPI_REG(CQSPI_REG_CONFIG) = cfg;
    /* Set address size = 3 bytes (0..3 means 1..4 bytes) */
    uint32_t sz = QSPI_REG(CQSPI_REG_SIZE);
    sz &= ~CQSPI_REG_SIZE_ADDRESS_MASK;
    sz |= (3 - 1);
    QSPI_REG(CQSPI_REG_SIZE) = sz;
    return 0;
}

static inline uint32_t to_dev_off(uint32_t addr)
{
    /* Accept absolute XIP address or offset */
    return (addr >= XIP_FLASH_BASE) ? (addr - XIP_FLASH_BASE) : addr;
}

int flash_if_read(uint32_t addr, void *buf, uint32_t len)
{
    uint32_t off = to_dev_off(addr);
    const uint8_t *src = (const uint8_t *)(XIP_FLASH_BASE + off);
    uint8_t *dst = (uint8_t *)buf;
    for (uint32_t i = 0; i < len; ++i) dst[i] = src[i];
    return 0;
}

int flash_if_erase(uint32_t addr, uint32_t len)
{
    addr = to_dev_off(addr);
    /* Align to 4KB sectors */
    uint32_t mis = addr & 0xFFFu;
    if (mis)
    {
        addr -= mis;
        len += mis;
    }
    /* Enable write */
    if (qspi_cmd_only(W25_CMD_WREN) != 0) return -1;
    /* Erase each 4KB sector covering length */
    uint32_t end = addr + len;
    for (uint32_t a = addr; a < end; a += 0x1000u)
    {
        if (qspi_cmd_addr(W25_CMD_SECTOR_ERASE, a, 3) != 0) return -1;
        if (w25_wait_wip_clear(2000) != 0) return -1;
        /* WREN again for next operation per datasheet */
        if (a + 0x1000u < end)
        {
            if (qspi_cmd_only(W25_CMD_WREN) != 0) return -1;
        }
    }
    return 0;
}

int flash_if_write(uint32_t addr, const void *data, uint32_t len)
{
    addr = to_dev_off(addr);
    const uint8_t *p = (const uint8_t *)data;
    uint32_t remaining = len;
    while (remaining)
    {
        uint32_t page_off = addr & 0xFFu;
        uint32_t chunk = 256u - page_off;
        if (chunk > remaining) chunk = remaining;
        /* STIG write supports up to 8 bytes via CMDWRITEDATA regs; for more, we must use indirect write.
           For simplicity, do up to 8 bytes per STIG; loop within page. */
        uint32_t done = 0;
        while (done < chunk)
        {
            uint32_t n = chunk - done;
            if (n > 8) n = 8; /* CMDWRITEDATA lower+upper support 8 bytes */
            /* WREN before each program operation */
            if (qspi_cmd_only(W25_CMD_WREN) != 0) return -1;
            if (qspi_cmd_addr_write(W25_CMD_PP, addr + done, 3, p + done, n) != 0) return -1;
            if (w25_wait_wip_clear(5) != 0) return -1;
            done += n;
        }
        addr += chunk;
        p += chunk;
        remaining -= chunk;
    }
    return 0;
}

uint32_t flash_if_crc32(uint32_t addr, uint32_t len)
{
    uint32_t off = to_dev_off(addr);
    const uint8_t *p = (const uint8_t *)(XIP_FLASH_BASE + off);
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < len; ++i)
    {
        crc = crc32_update(crc, p[i]);
    }
    return crc;
}
