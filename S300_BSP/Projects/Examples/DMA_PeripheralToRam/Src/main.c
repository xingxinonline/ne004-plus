#include "s300.h"
#include "board.h"
#include "s300_uart.h"
#include "s300_rcc.h"
#include "s300_iomux.h"
#include "s300_dma.h"
#include <string.h>
#include <stdio.h>

#define RX_LEN 16
static uint8_t rxbuf[RX_LEN + 1];

static void uart1_pins(void)
{
    /* Ensure IO_MUX/MATRIX enabled */
    S300_RCC_EnableAPB0((1u << 9) | (1u << 8));
    /* UART1 on IO16/IO17 as function3 per board ref */
    S300_RCC_EnableAPB1Mask(S300_APB1_UART1 | S300_APB1_GPIO);
    S300_RCC_ReleaseAPB1ResetBits(S300_APB1_UART1 | S300_APB1_GPIO);
    S300_IOMAT_SetIoFunc(16u, 3u); /* UART1_TX */
    S300_IOMUX_SetPadFunc(16u, 1u);
    S300_IOMAT_SetIoFunc(17u, 3u); /* UART1_RX */
    S300_IOMUX_SetPadFunc(17u, 1u);
}

int main(void)
{
    Board_Clock_Init();
    Board_Pinmux_Init();
    uart1_pins();
    S300_UART_Init(1, 115200u);
    printf("[DMA][P2M][UART1 RX] Send %u bytes to UART1 to trigger DMA...\r\n", (unsigned)RX_LEN);
    S300_DMA_EnableClock(S300_DMA0, 1);
    S300_DMA_GlobalEnable(S300_DMA0, 1);
    memset(rxbuf, 0, sizeof(rxbuf));
    /* Configure UART1 for DMA mode if required via FIFO/DMA bit; our UART driver uses DW_apb_uart: set FCR.DMAM=1 */
    S300_UartFifoConfig fc = { .enable = 1, .rx_trig = S300_UART_RX_TRIG_1CHAR, .tx_trig = S300_UART_TX_TRIG_EMPTY, .dma_mode = 1 };
    (void)S300_UART_SetFIFO(1, &fc);
    /* DMA ch0: UART1 RBR -> rxbuf */
    int rc = S300_DMA_ConfigStd(S300_DMA0, S300_DMA_CH0,
                                (uint32_t)(UART_RBRn(UARTn_BASE(1))),
                                (uint32_t)rxbuf, RX_LEN,
                                S300_DMA_TR_WIDTH_8);
    if (rc != 0)
    {
        printf("config err=%d\r\n", rc);
        for (;;);
    }
    S300_DMA_SetIncrements(S300_DMA0, S300_DMA_CH0, S300_DMA_ADDR_FIX, S300_DMA_ADDR_INC);
    S300_DMA_SetTransferType(S300_DMA0, S300_DMA_CH0, S300_DMA_TT_P2M_FD);
    S300_DMA_SetHandshake(S300_DMA0, S300_DMA_CH0, S300_DMA_HS_UART1_RX, S300_DMA_HS_NONE);
    S300_DMA_Start(S300_DMA0, S300_DMA_CH0);
    while (S300_DMA_IsBusy(S300_DMA0, S300_DMA_CH0)) { /* wait */ }
    rxbuf[RX_LEN] = '\0';
    printf("[DMA][P2M] Done. RX: %s\r\n", rxbuf);
    while (1)
    {
        __NOP();
    }
}
