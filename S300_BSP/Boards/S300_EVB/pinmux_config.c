#include "s300.h"
#include "s300_iomux.h"
#include "s300_rcc.h"
#include "board.h"

void Board_Pinmux_Init(void)
{
    /* Enable clocks for IO_MUX & IO_MATRIX on APB0 */
    S300_RCC_EnableAPB0((1u << 9) | (1u << 8));
    /* For debug UART route (BOARD_UART_DEBUG_ID) enable APB1 clock and release reset */
    uint32_t uart_idx = BOARD_UART_DEBUG_ID;
    uint32_t uart_clk_bit = (uart_idx == 0u) ? 0u : (uart_idx == 1u) ? 1u : (uart_idx == 2u) ? 2u : 3u;
    S300_RCC_EnableAPB1((1u << uart_clk_bit) | (1u << 8)); /* UARTx + GPIO */
    S300_RCC_ReleaseAPB1Reset((1u << uart_clk_bit) | (1u << 8));
    /* Configure pad/matrix for UARTx */
    if (uart_idx == 3u)
    {
        /* IO26/IO27 function3 in IO_MATRIX */
        S300_IOMAT_SetIoFunc(26u, 3u);
        S300_IOMAT_SetIoFunc(27u, 3u);
        /* PAD26/PAD27 mode=1 in IO_MUX (as per reference) */
        S300_IOMUX_SetPadFunc(26u, 1u);
        S300_IOMUX_SetPadFunc(27u, 1u);
    }
    else if (uart_idx == 0u)
    {
        /* UART0: PAD1 TXD, PAD3 RXD -> Function3 in IO_MATRIX, mode=1 in IO_MUX */
        S300_IOMAT_SetIoFunc(1u, 3u);
        S300_IOMAT_SetIoFunc(3u, 3u);
        S300_IOMUX_SetPadFunc(1u, 1u);
        S300_IOMUX_SetPadFunc(3u, 1u);
    }
}
