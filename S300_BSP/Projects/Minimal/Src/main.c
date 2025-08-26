/* Minimal CMSIS app: print "Hello, world!" via UART and idle */
#include "s300.h"
#include <stdio.h>

#ifndef UART_DEBUG_IDX
    #define UART_DEBUG_IDX 3u /* default use UART3 per existing examples */
#endif

/* Minimal UART3 GPIO mux + clock enable (APB1: GPIO, UART3) */
static void minimal_uart3_gpio_init(void)
{
    /* Base addresses from platform (kept local to avoid extra headers) */
    const uint32_t RCC_BASE        = 0x4000A000u; /* RCC */
    const uint32_t APB1_CLK_EN_OFF = 0x000Cu;     /* CM4_APB1_CLK_EN_REG */
    volatile uint32_t * const APB1_CLK_EN = (uint32_t *)(RCC_BASE + APB1_CLK_EN_OFF);

    /* Enable APB1 clocks: bit8=GPIO, bit3=UART3 */
    *APB1_CLK_EN |= (1u << 8) | (1u << 3);

    /* IO Matrix: set GPIOA26/27 to FUNCTION_3 (0b11) with one RMW on index 1 */
    const uint32_t IO_MATRIX_BASE = 0x40008000u;
    volatile uint32_t * const IO_MATRIX_CFG1 = (uint32_t *)(IO_MATRIX_BASE + 4u); /* index=1 */
    uint32_t v = *IO_MATRIX_CFG1;
    /* pin 26 -> shift 20, pin 27 -> shift 22; clear then set to 0b11 */
    v &= ~((0x3u << 20) | (0x3u << 22));
    v |=  ((0x3u << 20) | (0x3u << 22));
    *IO_MATRIX_CFG1 = v;
}

static void uart_set_baud(uint32_t base, uint32_t baud)
{
    /* DW-apb-uart divisor: baud_div = clk / (16*baud), fractional via DLF (1/16) */
    uint32_t clk = SystemCoreClock; /* 24MHz */
    if (baud == 0u || clk == 0u) return;
    uint32_t denom = baud * 16u;
    uint32_t div = clk / denom;
    uint32_t rem = clk % denom;
    /* round fractional to 1/16 */
    uint32_t dlf = (uint32_t)((uint64_t)rem * 16u + (denom / 2u)) / denom;
    if (dlf >= 16u)
    {
        dlf = 0u;
        div += 1u;
    }
    if (div == 0u) div = 1u;
    /* program divisor latch */
    UART_LCRn(base) |= 0x80; /* DLAB */
    UART_DLLn(base) = div & 0xFFu;
    UART_DLHn(base) = (div >> 8) & 0xFFu;
    UART_LCRn(base) &= ~0x80u; /* clear DLAB */
    UART_DLFn(base) = dlf & 0x0Fu; /* 1/16 steps */
}

static void uart_init_poll(uint32_t idx, uint32_t baud)
{
    uint32_t base = UARTn_BASE(idx);
    /* Disable interrupts */
    UART_IERn(base) = 0x0u;
    /* Enable FIFO, reset RX/TX */
    UART_FCRn(base) = 0x07u;
    /* 8N1 */
    UART_LCRn(base) = 0x03u;
    /* No modem features */
    UART_MCRn(base) = 0x00u;
    /* Baud */
    uart_set_baud(base, baud);
}

int main(void)
{
    /* SystemInit() runs before main; SystemCoreClock=24MHz; VTOR set. */
    minimal_uart3_gpio_init();
    uart_init_poll(UART_DEBUG_IDX, 115200u);
    /* Unbuffer stdout to avoid waiting for newline/flush */
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Hello, world!\n");
    /* Idle forever */
    for (;;)
    {
        __WFI();
    }
}
