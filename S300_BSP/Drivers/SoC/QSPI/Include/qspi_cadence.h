#ifndef S300_BSP_QSPI_CADENCE_H
#define S300_BSP_QSPI_CADENCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "s300_memmap.h"

/* Cadence QSPI APB register map (minimal subset) */
#define CQSPI_REG_CONFIG                 0x00u
#define   CQSPI_CFG_ENABLE               (1u << 0)
#define   CQSPI_CFG_CLK_POL              (1u << 1)
#define   CQSPI_CFG_CLK_PHA              (1u << 2)
#define   CQSPI_CFG_DIRECT               (1u << 7)
#define   CQSPI_CFG_XIP_IMM              (1u << 18)
#define   CQSPI_CFG_DECODE               (1u << 9)
#define   CQSPI_CFG_CHIPSELECT_LSB       10u
#define   CQSPI_CFG_CHIPSELECT_MASK      0xFu
#define   CQSPI_CFG_BAUD_LSB             19u
#define   CQSPI_CFG_BAUD_MASK            0xFu
#define   CQSPI_CFG_IDLE_LSB             31u

#define CQSPI_REG_RD_INSTR               0x04u
#define   CQSPI_RD_OPCODE_LSB            0u
#define   CQSPI_RD_TYPE_INSTR_LSB        8u
#define   CQSPI_RD_TYPE_ADDR_LSB         12u
#define   CQSPI_RD_TYPE_DATA_LSB         16u
#define   CQSPI_RD_DUMMY_LSB             24u
#define   CQSPI_RD_MODE_EN_LSB           20u

#define CQSPI_REG_WR_INSTR               0x08u
#define   CQSPI_WR_OPCODE_LSB            0u
#define   CQSPI_WR_TYPE_ADDR_LSB         12u
#define   CQSPI_WR_TYPE_DATA_LSB         16u

#define CQSPI_REG_DELAY                  0x0Cu
#define   CQSPI_DELAY_TSLCH_LSB          0u
#define   CQSPI_DELAY_TCHSH_LSB          8u
#define   CQSPI_DELAY_TSD2D_LSB          16u
#define   CQSPI_DELAY_TSHSL_LSB          24u

#define CQSPI_REG_RD_DATA_CAPTURE        0x10u
#define   CQSPI_RD_CAPTURE_BYPASS        (1u << 0)
#define   CQSPI_RD_CAPTURE_DELAY_LSB     1u
#define   CQSPI_RD_CAPTURE_DELAY_MASK    0xFu

#define CQSPI_REG_SIZE                   0x14u
#define   CQSPI_SIZE_ADDR_LSB            0u      /* program as (addr_bytes-1) */
#define   CQSPI_SIZE_ADDR_MASK           0xFu
#define   CQSPI_SIZE_PAGE_LSB            4u      /* page size in bytes */
#define   CQSPI_SIZE_PAGE_MASK           0xFFFu
#define   CQSPI_SIZE_BLOCK_LSB           16u     /* block size in 4KB units */
#define   CQSPI_SIZE_BLOCK_MASK          0x3Fu

#define CQSPI_REG_SRAMPARTITION          0x18u
#define CQSPI_REG_INDIRECTTRIGGER        0x1Cu
#define CQSPI_REG_REMAP                  0x24u
#define CQSPI_REG_MODE_BIT               0x28u
#define CQSPI_REG_SDRAMLEVEL             0x2Cu
#define   CQSPI_SDRAMLEVEL_RD_LSB        0u
#define   CQSPI_SDRAMLEVEL_RD_MASK       0xFFFFu
#define   CQSPI_SDRAMLEVEL_WR_LSB        16u
#define   CQSPI_SDRAMLEVEL_WR_MASK       0xFFFFu

#define CQSPI_REG_IRQSTATUS              0x40u
#define CQSPI_REG_IRQMASK                0x44u

#define CQSPI_REG_INDIRECTRD             0x60u
#define   CQSPI_INDIRECTRD_START         (1u << 2)
#define   CQSPI_INDIRECTRD_CANCEL        (1u << 1)
#define   CQSPI_INDIRECTRD_DONE          (1u << 5)
#define CQSPI_REG_INDIRECTRDWATERMARK    0x64u
#define CQSPI_REG_INDIRECTRDSTARTADDR    0x68u
#define CQSPI_REG_INDIRECTRDBYTES        0x6Cu

#define CQSPI_REG_INDIRECTWR             0x70u
#define   CQSPI_INDIRECTWR_START         (1u << 2)
#define   CQSPI_INDIRECTWR_CANCEL        (1u << 1)
#define   CQSPI_INDIRECTWR_DONE          (1u << 5)
#define CQSPI_REG_INDIRECTWRWATERMARK    0x74u
#define CQSPI_REG_INDIRECTWRSTARTADDR    0x78u
#define CQSPI_REG_INDIRECTWRBYTES        0x7Cu

#define CQSPI_REG_CMDCTRL                0x90u
#define   CQSPI_CMDCTRL_EXECUTE          (1u << 0)
#define   CQSPI_CMDCTRL_INPROGRESS       (1u << 1)
#define   CQSPI_CMDCTRL_DUMMY_LSB        7u
#define   CQSPI_CMDCTRL_WR_BYTES_LSB     12u
#define   CQSPI_CMDCTRL_WR_EN_LSB        15u
#define   CQSPI_CMDCTRL_ADD_BYTES_LSB    16u
#define   CQSPI_CMDCTRL_ADDR_EN_LSB      19u
#define   CQSPI_CMDCTRL_RD_BYTES_LSB     20u
#define   CQSPI_CMDCTRL_RD_EN_LSB        23u
#define   CQSPI_CMDCTRL_OPCODE_LSB       24u
#define   CQSPI_CMDCTRL_DUMMY_MASK       0x1Fu
#define   CQSPI_CMDCTRL_WR_BYTES_MASK    0x7u
#define   CQSPI_CMDCTRL_ADD_BYTES_MASK   0x3u
#define   CQSPI_CMDCTRL_RD_BYTES_MASK    0x7u
#define   CQSPI_CMDCTRL_OPCODE_MASK      0xFFu

#define CQSPI_REG_CMDADDRESS             0x94u
#define CQSPI_REG_CMDREADDATALOWER       0xA0u
#define CQSPI_REG_CMDREADDATAUPPER       0xA4u
#define CQSPI_REG_CMDWRITEDATALOWER      0xA8u
#define CQSPI_REG_CMDWRITEDATAUPPER      0xACu

/* Transfer width encodings */
#define CQSPI_INST_TYPE_SINGLE           0u
#define CQSPI_INST_TYPE_DUAL             1u
#define CQSPI_INST_TYPE_QUAD             2u

/* Common SPI flash opcodes (W25Qxx) */
#define W25Q_CMD_RDID    0x9Fu
#define W25Q_CMD_RDSR1   0x05u
#define W25Q_CMD_RDSR2   0x35u
#define W25Q_CMD_RDSR3   0x15u
#define W25Q_CMD_WREN    0x06u
#define W25Q_CMD_PP      0x02u
#define W25Q_CMD_READ    0x03u
#define W25Q_CMD_FAST    0x0Bu
#define W25Q_CMD_SE_4K   0x20u
#define W25Q_CMD_BE_64K  0xD8u
#define W25Q_CMD_CE      0xC7u
/* Addressing mode */
#define W25Q_CMD_EN4B    0xB7u
#define W25Q_CMD_EX4B    0xE9u
/* Status register write ops (Winbond) */
#define W25Q_CMD_WRSR12  0x01u  /* write SR1 then SR2 */
#define W25Q_CMD_WRSR2   0x31u  /* write SR2 only */
#define W25Q_CMD_WRSR3   0x11u  /* write SR3 only */

int qspi_unlock_all(void);
/* 控制驱动内部调试打印（默认开启）。带宽测试时建议关闭以避免串口开销影响计时。 */
void qspi_set_verbose(bool enable);

typedef struct
{
    volatile uint8_t *reg;   /* APB config base: QSPI_CFG_BASE */
    volatile uint8_t *ahb;   /* AHB mapped flash window: M4_SLV_FLASH_BASE */
    uint32_t ahb_size;       /* Window size in bytes */
    uint32_t ref_clk_hz;     /* Reference clock to controller (AHB clock) */
    uint32_t sclk_hz;        /* Desired SPI SCLK */
    uint32_t fifo_depth;     /* words */
    uint32_t fifo_width;     /* bytes per word (4) */
    uint32_t page_size;      /* 256 */
    uint32_t block_4k_units; /* 16 => 64KB blocks (for SIZE reg) */
} qspi_cadence_t;

/* Global singleton for simplicity */
extern qspi_cadence_t g_qspi;

void qspi_cadence_init(uint32_t ref_clk_hz, uint32_t sclk_hz);

int qspi_read_id(uint8_t *id, uint32_t len);
int qspi_read(uint32_t addr, void *buf, uint32_t len);
int qspi_page_program(uint32_t addr, const void *buf, uint32_t len);
int qspi_erase_4k(uint32_t addr);
int qspi_erase_64k(uint32_t addr);
int qspi_chip_erase(void);
int qspi_wait_ready(uint32_t timeout_ms);
/* Feature helpers */
int qspi_set_quad_enable(bool enable);
int qspi_set_address_mode_4byte(bool enable);

/* Debug helpers */
void qspi_dump_regs(const char *tag);
int qspi_read_status(uint8_t *sr1, uint8_t *sr2, uint8_t *sr3);

#ifdef __cplusplus
}
#endif

#endif /* S300_BSP_QSPI_CADENCE_H */
