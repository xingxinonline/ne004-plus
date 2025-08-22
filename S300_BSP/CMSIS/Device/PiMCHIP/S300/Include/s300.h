#ifndef PIMCHIP_S300_H
#define PIMCHIP_S300_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "device.h"

/* Memory map */
#define SRAM0_BASE      (0x10000000UL) /* 8KB */
#define SRAM1_BASE      (0x20000000UL) /* 384KB */

#define APB0_BASE       (0x40000000UL)
#define APB1_BASE       (0x40010000UL)
#define AHB_BASE        (0x41000000UL)

#define UART0_BASE      (0x40010000UL)
#define UARTn_BASE(n)   (UART0_BASE + ((uint32_t)(n) * 0x1000UL))
#define GPIO_BASE       (0x40018000UL)
#define RCC_BASE        (0x4000A000UL)
#define IO_MATRIX_BASE  (0x40008000UL)
#define IO_MUX_BASE     (0x40009000UL)

/* QSPI (Cadence) register base */
#define QSPI_CFG_BASE   (0x4000D000UL)

/* Cadence QSPI minimal register set (offsets per cadence_qspi.h) */
#define CQSPI_REG_CONFIG                        0x00
#define CQSPI_REG_CONFIG_ENABLE                 (1u << 0)
#define CQSPI_REG_CONFIG_DIRECT                 (1u << 7)
#define CQSPI_REG_CONFIG_DECODE                 (1u << 9)
#define CQSPI_REG_CONFIG_CHIPSELECT_LSB         10
#define CQSPI_REG_CONFIG_CHIPSELECT_MASK        0xF
#define CQSPI_REG_CONFIG_IDLE_LSB               31

#define CQSPI_REG_RD_INSTR                      0x04
#define CQSPI_REG_RD_INSTR_OPCODE_LSB           0
#define CQSPI_REG_RD_INSTR_TYPE_DATA_LSB        16
#define CQSPI_REG_RD_INSTR_DUMMY_LSB            24
#define CQSPI_REG_RD_INSTR_DUMMY_MASK           0x1F

#define CQSPI_REG_WR_INSTR                      0x08
#define CQSPI_REG_WR_INSTR_OPCODE_LSB           0
#define CQSPI_REG_WR_INSTR_TYPE_ADDR_LSB        12
#define CQSPI_REG_WR_INSTR_TYPE_DATA_LSB        16

#define CQSPI_REG_SIZE                          0x14
#define CQSPI_REG_SIZE_ADDRESS_LSB              0
#define CQSPI_REG_SIZE_ADDRESS_MASK             0xF

#define CQSPI_REG_SDRAMLEVEL                    0x2C
#define CQSPI_REG_SDRAMLEVEL_WR_LSB             16
#define CQSPI_REG_SDRAMLEVEL_WR_MASK            0xFFFF

#define CQSPI_REG_CMDCTRL                       0x90
#define CQSPI_REG_CMDCTRL_EXECUTE               (1u << 0)
#define CQSPI_REG_CMDCTRL_INPROGRESS            (1u << 1)
#define CQSPI_REG_CMDCTRL_DUMMY_LSB             7
#define CQSPI_REG_CMDCTRL_WR_BYTES_LSB          12
#define CQSPI_REG_CMDCTRL_WR_EN_LSB             15
#define CQSPI_REG_CMDCTRL_ADD_BYTES_LSB         16
#define CQSPI_REG_CMDCTRL_ADDR_EN_LSB           19
#define CQSPI_REG_CMDCTRL_RD_BYTES_LSB          20
#define CQSPI_REG_CMDCTRL_RD_EN_LSB             23
#define CQSPI_REG_CMDCTRL_OPCODE_LSB            24
#define CQSPI_REG_CMDCTRL_DUMMY_MASK            0x1F
#define CQSPI_REG_CMDCTRL_WR_BYTES_MASK         0x7
#define CQSPI_REG_CMDCTRL_ADD_BYTES_MASK        0x3
#define CQSPI_REG_CMDCTRL_RD_BYTES_MASK         0x7

#define CQSPI_REG_CMDADDRESS                    0x94
#define CQSPI_REG_CMDREADDATALOWER              0xA0
#define CQSPI_REG_CMDREADDATAUPPER              0xA4
#define CQSPI_REG_CMDWRITEDATALOWER             0xA8
#define CQSPI_REG_CMDWRITEDATAUPPER             0xAC

/* Helpers */
#define CQSPI_IS_IDLE() \
	(((*(volatile uint32_t *)(QSPI_CFG_BASE + CQSPI_REG_CONFIG)) >> CQSPI_REG_CONFIG_IDLE_LSB) & 0x1u)

/* System Core Clock */
#define HSE_CLOCK_HZ    (24000000UL)

/* UART registers (DW_apb_uart) */
/* 0x00/0x04/0x08 dual-mapped with DLAB & access type */
#define UART_RBRn(b)    (*(volatile uint32_t *)((b) + 0x00)) /* RO */
#define UART_THRn(b)    (*(volatile uint32_t *)((b) + 0x00)) /* WO */
#define UART_DLLn(b)    (*(volatile uint32_t *)((b) + 0x00)) /* RW (when LCR[7]=1) */
#define UART_IERn(b)    (*(volatile uint32_t *)((b) + 0x04)) /* RW */
#define UART_DLHn(b)    (*(volatile uint32_t *)((b) + 0x04)) /* RW (when LCR[7]=1) */
#define UART_IIRn(b)    (*(volatile uint32_t *)((b) + 0x08)) /* RO */
#define UART_FCRn(b)    (*(volatile uint32_t *)((b) + 0x08)) /* WO */
#define UART_LCRn(b)    (*(volatile uint32_t *)((b) + 0x0C)) /* RW */
#define UART_MCRn(b)    (*(volatile uint32_t *)((b) + 0x10)) /* RW */
#define UART_LSRn(b)    (*(volatile uint32_t *)((b) + 0x14)) /* RO */
#define UART_MSRn(b)    (*(volatile uint32_t *)((b) + 0x18)) /* RO */
#define UART_SCRn(b)    (*(volatile uint32_t *)((b) + 0x1C)) /* RW */
#define UART_LPDLLn(b)  (*(volatile uint32_t *)((b) + 0x20)) /* RW */
#define UART_LPDLHn(b)  (*(volatile uint32_t *)((b) + 0x24)) /* RW */
/* 0x30..0x6C SRBR/STHR shadow FIFOs not defined per-entry here */
#define UART_USRn(b)    (*(volatile uint32_t *)((b) + 0x7C)) /* RO */
#define UART_TFLn(b)    (*(volatile uint32_t *)((b) + 0x80)) /* RO */
#define UART_RFLn(b)    (*(volatile uint32_t *)((b) + 0x84)) /* RO */
#define UART_SRRn(b)    (*(volatile uint32_t *)((b) + 0x88)) /* WO */
#define UART_SRTSn(b)   (*(volatile uint32_t *)((b) + 0x8C)) /* RW */
#define UART_SBCRn(b)   (*(volatile uint32_t *)((b) + 0x90)) /* RW */
#define UART_SDMAMn(b)  (*(volatile uint32_t *)((b) + 0x94)) /* RW */
#define UART_SFEn(b)    (*(volatile uint32_t *)((b) + 0x98)) /* RW */
#define UART_SRTn(b)    (*(volatile uint32_t *)((b) + 0x9C)) /* RW */
#define UART_STETn(b)   (*(volatile uint32_t *)((b) + 0xA0)) /* RO */
#define UART_HTXn(b)    (*(volatile uint32_t *)((b) + 0xA4)) /* RW */
#define UART_DMASAn(b)  (*(volatile uint32_t *)((b) + 0xA8)) /* WO */
#define UART_TCRn(b)    (*(volatile uint32_t *)((b) + 0xAC)) /* RW */
#define UART_DE_ENn(b)  (*(volatile uint32_t *)((b) + 0xB0)) /* RW */
#define UART_RE_ENn(b)  (*(volatile uint32_t *)((b) + 0xB4)) /* RW */
#define UART_DETn(b)    (*(volatile uint32_t *)((b) + 0xB8)) /* RW */
#define UART_TATn(b)    (*(volatile uint32_t *)((b) + 0xBC)) /* RW */
#define UART_DLFn(b)    (*(volatile uint32_t *)((b) + 0xC0)) /* RW */
#define UART_RARn(b)    (*(volatile uint32_t *)((b) + 0xC4)) /* RW */
#define UART_TARn(b)    (*(volatile uint32_t *)((b) + 0xC8)) /* RW */
#define UART_LCR_EXTn(b) (*(volatile uint32_t *)((b) + 0xCC)) /* RW */
#define UART_REG_TIMEOUT_RSTn(b) (*(volatile uint32_t *)((b) + 0xD4)) /* RW */
#define UART_CPRn(b)    (*(volatile uint32_t *)((b) + 0xF4)) /* RO */
#define UART_UCVn(b)    (*(volatile uint32_t *)((b) + 0xF8)) /* RO */
#define UART_CTRn(b)    (*(volatile uint32_t *)((b) + 0xFC)) /* RO */

/* RCC minimal fields used */
#define RCC_APB0_CLK_EN (*(volatile uint32_t *)(RCC_BASE + 0x08))
#define RCC_APB1_CLK_EN (*(volatile uint32_t *)(RCC_BASE + 0x0C))
#define RCC_APB1_RST_CTL (*(volatile uint32_t *)(RCC_BASE + 0x24))

/* IO Matrix minimal fields used (set IO to input/output mode) */
#define IO_MAT_CFG0     (*(volatile uint32_t *)(IO_MATRIX_BASE + 0x00)) /* IO0..15 */
#define IO_MAT_CFG1     (*(volatile uint32_t *)(IO_MATRIX_BASE + 0x04)) /* IO16..31 */
#define IO_MAT_CFG2     (*(volatile uint32_t *)(IO_MATRIX_BASE + 0x08)) /* IO32..47 */

/* IO MUX function select registers: 2 bits per pad, 00/01/10/11 => Function0..3 */
#define IO_MUX_CFG0     (*(volatile uint32_t *)(IO_MUX_BASE + 0x00))  /* PAD0..15 */
#define IO_MUX_CFG1     (*(volatile uint32_t *)(IO_MUX_BASE + 0x04))  /* PAD16..31 */
#define IO_MUX_CFG2     (*(volatile uint32_t *)(IO_MUX_BASE + 0x08))  /* PAD32..47 */

/* UART inline implementations moved to Drivers/UART. */

#ifdef __cplusplus
}
#endif

#endif
