#include "s300.h"
#include "board.h"
#include "s300_uart.h"
#include "s300_rcc.h"
#include "s300_dma.h"
#include <string.h>
#include <stdio.h>

#define LEN 1024
static uint8_t src[LEN];
static uint8_t dst[LEN];

int main(void)
{
    Board_Clock_Init();
    Board_Pinmux_Init();
    S300_UART_Init(BOARD_UART_DEBUG_ID, BOARD_UART_DEBUG_BAUD);
    printf("[DMA][RAM2RAM] start...\r\n");
    for (unsigned i = 0; i < LEN; ++i) src[i] = (uint8_t)(i ^ 0x5Au);
    memset(dst, 0, sizeof(dst));
    S300_DMA_EnableClock(S300_DMA0, 1);
    S300_DMA_GlobalEnable(S300_DMA0, 1);
    int rc = S300_DMA_ConfigStd(S300_DMA0, S300_DMA_CH0,
                                (uint32_t)src, (uint32_t)dst, LEN,
                                S300_DMA_TR_WIDTH_8);
    if (rc != 0)
    {
        printf("config err=%d\r\n", rc);
        for (;;);
    }
    S300_DMA_SetIncrements(S300_DMA0, S300_DMA_CH0, S300_DMA_ADDR_INC, S300_DMA_ADDR_INC);
    S300_DMA_SetTransferType(S300_DMA0, S300_DMA_CH0, S300_DMA_TT_M2M_FD);
    S300_DMA_Start(S300_DMA0, S300_DMA_CH0);
    while (S300_DMA_IsBusy(S300_DMA0, S300_DMA_CH0)) { /* wait */ }
    /* verify */
    unsigned diff = 0;
    for (unsigned i = 0; i < LEN; ++i) if (dst[i] != src[i])
        {
            diff++;
            break;
        }
    printf("[DMA][RAM2RAM] done, verify %s, dst[%u]=0x%02X\r\n", diff ? "FAIL" : "OK", LEN - 1, dst[LEN - 1]);
    while (1)
    {
        __NOP();
    }
}
