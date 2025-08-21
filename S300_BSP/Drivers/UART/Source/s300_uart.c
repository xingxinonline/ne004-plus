#include "s300.h"
#include "s300_uart.h"

/* Simple UART init for 24MHz, 115200, 8N1, polling */
void S300_UART_Init_115200(uint32_t idx)
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

void S300_UART_PutCharI(uint32_t idx, char c)
{
    uint32_t base = UARTn_BASE(idx);
    uint32_t to = 1000000u;
    while (!((UART_USRn(base) & (1u << 1)) || (UART_LSRn(base) & (1u << 5))))
    {
        if (--to == 0u) break;
    }
    UART_THRn(base) = (uint32_t)c;
}

void S300_UART_PutStringI(uint32_t idx, const char *s)
{
    while (*s)
    {
        if (*s == '\n') S300_UART_PutCharI(idx, '\r');
        S300_UART_PutCharI(idx, *s++);
    }
}
