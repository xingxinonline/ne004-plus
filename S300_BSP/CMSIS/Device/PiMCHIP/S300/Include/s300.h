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

/* Simple UART init for 24MHz, 115200, 8N1, polling */
static inline void S300_UART_Init_115200(uint32_t idx)
{
    /* Ensure IO Matrix/IO MUX clocks on (APB0) before pin config */
    RCC_APB0_CLK_EN |= (1u << 9) | (1u << 8); /* Io_mux, Io_matrix */
    /* Enable APB1 clock for UARTx and GPIO */
    uint32_t uart_clk_bit = (idx == 0u) ? 0u : (idx == 1u) ? 1u : (idx == 2u) ? 2u : 3u;
    RCC_APB1_CLK_EN |= (1u << uart_clk_bit) | (1u << 8);
    /* Release APB1 reset for UARTx and GPIO: per SoC, set bit = deassert reset */
    RCC_APB1_RST_CTL |= ((1u << uart_clk_bit) | (1u << 8));
    /* Configure IO Matrix & IO MUX for TX/RX on the chosen UART */
    if (idx == 0u)
    {
        /* UART0: PAD1 TXD, PAD3 RXD -> Function3 in IO_MATRIX, mode=1 in IO_MUX */
        uint32_t mat0 = IO_MAT_CFG0;
        mat0 &= ~((0x3u << 2) | (0x3u << 6));
        mat0 |= ((0x3u << 2) | (0x3u << 6));  /* func3 */
        IO_MAT_CFG0 = mat0;
        uint32_t mux0 = IO_MUX_CFG0;
        mux0 &= ~((0x3u << 2) | (0x3u << 6));
        mux0 |= ((0x1u << 2) | (0x1u << 6));  /* mode=1 per ref */
        IO_MUX_CFG0 = mux0;
    }
    else if (idx == 3u)
    {
        /* UART3: PAD27 TXD, PAD26 RXD -> Function3 in IO_MATRIX, mode=1 in IO_MUX */
        uint32_t mat1 = IO_MAT_CFG1;
        /* IO26 => bits[21:20], IO27 => bits[23:22] */
        mat1 &= ~((0x3u << 20) | (0x3u << 22));
        mat1 |= ((0x3u << 20) | (0x3u << 22));  /* func3 */
        IO_MAT_CFG1 = mat1;
        uint32_t mux1 = IO_MUX_CFG1;
        mux1 &= ~((0x3u << 20) | (0x3u << 22));
        mux1 |= ((0x1u << 20) | (0x1u << 22));  /* mode=1 */
        IO_MUX_CFG1 = mux1;
    }
    uint32_t base = UARTn_BASE(idx);
    /* Disable UART interrupts */
    UART_IERn(base) = 0x0;
    /* FIFO: enable and reset */
    UART_FCRn(base) = 0x07;
    /* 8N1 */
    UART_LCRn(base) = 0x03;
    /* Baud 115200 @24MHz */
    UART_LCRn(base) |= 0x80;              /* DLAB */
    UART_DLLn(base) = 13 & 0xFF;
    UART_DLHn(base) = (13 >> 8) & 0xFF;
    UART_LCRn(base) &= ~0x80;             /* clear DLAB */
    UART_DLFn(base) = 0;                  /* fractional off (integer divisor 13) */
    /* Wait TX FIFO ready */
    (void)UART_USRn(base);
}

static inline void S300_UART_PutCharI(uint32_t idx, char c)
{
    uint32_t base = UARTn_BASE(idx);
    uint32_t to = 1000000u;
    while (!((UART_USRn(base) & (1u << 1)) || (UART_LSRn(base) & (1u << 5))))
    {
        if (--to == 0u) break;
    }
    UART_THRn(base) = (uint32_t)c;
}

static inline void S300_UART_PutStringI(uint32_t idx, const char *s)
{
    while (*s)
    {
        if (*s == '\n') S300_UART_PutCharI(idx, '\r');
        S300_UART_PutCharI(idx, *s++);
    }
}

#ifdef __cplusplus
}
#endif

#endif
