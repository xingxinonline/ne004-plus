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

/* System Core Clock */
#define HSE_CLOCK_HZ    (24000000UL)

/* UART registers (DW-apb-uart like) */
#define UART_RBRn(b)   (*(volatile uint32_t *)((b) + 0x00))
#define UART_THRn(b)   (*(volatile uint32_t *)((b) + 0x00))
#define UART_DLLn(b)   (*(volatile uint32_t *)((b) + 0x00))
#define UART_IERn(b)   (*(volatile uint32_t *)((b) + 0x04))
#define UART_DLHn(b)   (*(volatile uint32_t *)((b) + 0x04))
#define UART_IIRn(b)   (*(volatile uint32_t *)((b) + 0x08))
#define UART_FCRn(b)   (*(volatile uint32_t *)((b) + 0x08))
#define UART_LCRn(b)   (*(volatile uint32_t *)((b) + 0x0C))
#define UART_MCRn(b)   (*(volatile uint32_t *)((b) + 0x10))
#define UART_LSRn(b)   (*(volatile uint32_t *)((b) + 0x14))
#define UART_USRn(b)   (*(volatile uint32_t *)((b) + 0x7C))
#define UART_DLFn(b)   (*(volatile uint32_t *)((b) + 0xC0))

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
