#ifndef PIMCHIP_S300_H
#define PIMCHIP_S300_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "device.h"

/* Memory map (only what minimal BSP needs) */
#define SRAM0_BASE      (0x10000000UL) /* 8KB */
#define SRAM1_BASE      (0x20000000UL) /* 384KB */

#define UART0_BASE      (0x40010000UL)
#define UARTn_BASE(n)   (UART0_BASE + ((uint32_t)(n) * 0x1000UL))

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

/* UART inline implementations moved to Drivers/UART. (not used in minimal) */

/* Minimal SysTick helpers (implemented in system_S300.c) */
void S300_SysTick_Init(void);
uint32_t S300_SysTick_Millis(void);
void S300_DelayMs(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif
