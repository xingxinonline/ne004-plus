#include "s300.h"
#include "board.h"
#include "s300_uart.h"
#include "s300_rcc.h"
#include "s300_dma.h"
#include <string.h>
#include <stdio.h>

static const char msg[] = "Hello from DMA M2P UART3 TX!\r\n";

int main(void)
{
    Board_Clock_Init();
    Board_Pinmux_Init();
    /* Board_Pinmux_Init() 已按 BOARD_UART_DEBUG_ID=3 配好 UART3(IO26/27)，直接使用 */
    S300_UART_Init(3, 115200u);
    printf("[DMA][M2P][UART3 TX] start...\r\n");
    /* 为避免切换到 DMA 模式时截断上一条 printf，这里先等待 UART3 完全空闲 */
    while (!S300_UART_TxIdle(3)) { /* wait TX idle */ }
    S300_DMA_EnableClock(S300_DMA0, 1);
    S300_DMA_GlobalEnable(S300_DMA0, 1);
    /* 打开 UART 的 DMA 模式，TX 触发阈值设为 EMPTY 以便握手持续触发 */
    S300_UartFifoConfig fc = { .enable = 1, .rx_trig = S300_UART_RX_TRIG_1CHAR, .tx_trig = S300_UART_TX_TRIG_EMPTY, .dma_mode = 1 };
    (void)S300_UART_SetFIFO(3, &fc);
    int rc = S300_DMA_ConfigStd(S300_DMA0, S300_DMA_CH1,
                                (uint32_t)msg,
                                (uint32_t)(UARTn_BASE(3) + 0x30u) /* UART STHR 阴影窗口首地址 */,
                                sizeof(msg) - 1,
                                S300_DMA_TR_WIDTH_8);
    if (rc != 0)
    {
        printf("config err=%d\r\n", rc);
        for (;;);
    }
    S300_DMA_SetIncrements(S300_DMA0, S300_DMA_CH1, S300_DMA_ADDR_INC, S300_DMA_ADDR_FIX);
    S300_DMA_SetTransferType(S300_DMA0, S300_DMA_CH1, S300_DMA_TT_M2P_FD);
    S300_DMA_SetHandshake(S300_DMA0, S300_DMA_CH1, S300_DMA_HS_NONE, S300_DMA_HS_UART3_TX);
    S300_DMA_Start(S300_DMA0, S300_DMA_CH1);
    while (S300_DMA_IsBusy(S300_DMA0, S300_DMA_CH1)) { /* wait DMA done */ }
    /* 等待 UART 把 FIFO 中数据发送完成，再退出 DMA 模式，恢复普通 printf */
    while (!S300_UART_TxIdle(3)) { /* wait TX idle */ }
    fc.dma_mode = 0; /* 退出 DMA 模式 */
    (void)S300_UART_SetFIFO(3, &fc);
    printf("[DMA][M2P] done.\r\n");
    while (1)
    {
        __NOP();
    }
}
