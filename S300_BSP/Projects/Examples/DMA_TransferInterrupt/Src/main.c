#include "s300.h"
#include "board.h"
#include "s300_uart.h"
#include "s300_rcc.h"
#include "s300_dma.h"
#include "s300_intctrl.h"
#include <string.h>
#include <stdio.h>

/* 本示例默认走板载 UART3 作为调试串口；如未在 board.h 定义，则使用本地默认 */
#ifndef BOARD_UART_DEBUG_ID
    #define BOARD_UART_DEBUG_ID 3u
#endif
#ifndef BOARD_UART_DEBUG_BAUD
    #define BOARD_UART_DEBUG_BAUD 115200u
#endif

#define LEN 512
/* 选择 IRQ 演示模式：0=M2M；1=M2P(UART3 TX) */
#ifndef DMA_IRQ_DEMO_MODE
    #define DMA_IRQ_DEMO_MODE 0
#endif
/* 重复次数：用于验证多次中断是否正常触发 */
#ifndef DMA_REPEAT_TIMES
    #define DMA_REPEAT_TIMES 10
#endif
static uint8_t a[LEN];
static uint8_t b[LEN];
static const char irq_msg[] = "IRQ DEMO via DMA UART3\r\n";
volatile uint32_t tfr_cnt = 0;
volatile uint32_t isr_enter_cnt = 0;

void DMA0_IRQHandler(void)
{
    isr_enter_cnt++;
    /* 先快照 StatusTfr，避免清 pending 后读到 0 */
    uint32_t st_tfr = S300_DMA_GetStatusTfr(S300_DMA0);
    (void)S300_DMA_GetStatusInt(S300_DMA0); /* 旁路采样 */
    /* 仅清除 CH0 的 TFR pending（只启用 TFR 中断） */
    S300_DMA_ClearPending(S300_DMA0, 0x01u, S300_DMA_INT_TFR);
    /* 仅当 StatusTfr 指示 CH0 有 pending 时计数，避免误触发 */
    if (st_tfr & 0x1u) tfr_cnt++;
}

int main(void)
{
    Board_Clock_Init();
    Board_Pinmux_Init();
    S300_UART_Init(BOARD_UART_DEBUG_ID, BOARD_UART_DEBUG_BAUD);
    __enable_irq(); /* 确保全局中断开启 */
    printf("[DMA][IRQ] start...\r\n");
    for (unsigned i = 0; i < LEN; ++i) a[i] = (uint8_t)(i);
    memset(b, 0, sizeof(b));
    /* 使能 INT_CTRL 与 DMA 时钟，并解除 INT_CTRL 对 DMA0 的屏蔽（路由到 CM4）*/
    S300_RCC_EnableAPB0Mask(S300_APB0_INT_CTRL);
    S300_RCC_ReleaseAPB0ResetBits(S300_APB0_INT_CTRL);
    /* 将 DMA0 中断(irq27)路由到 CM4 */
    S300_INTCTRL_RouteToCM4(27u);
    S300_DMA_EnableClock(S300_DMA0, 1);
    S300_DMA_ClearAllInterrupts(S300_DMA0);
    S300_DMA_GlobalEnable(S300_DMA0, 1);
    int rc;
    /* 先明确屏蔽 CH0 的 Src/DstTran/Err，避免“全开”造成中断风暴 */
    S300_DMA_SetInterrupts(S300_DMA0, S300_DMA_CH0,
                           (S300_DMA_INT_SRCTRAN | S300_DMA_INT_DSTTRAN | S300_DMA_INT_ERR), 0);
    /* 使能 CH0 的中断：仅开启 TFR，避免 BLOCK 与 TFR 同时触发造成处理复杂
         注意：最后一个参数传 1 代表 unmask（允许触发）。 */
     S300_DMA_SetInterrupts(S300_DMA0, S300_DMA_CH0, S300_DMA_INT_TFR, 1);
    NVIC_ClearPendingIRQ(DMA0_IRQn);
    NVIC_EnableIRQ(DMA0_IRQn);
    /* 多次传输循环：每轮重新配置并启动，等待 ISR 计数递增来判定完成 */
    for (unsigned iter = 0; iter < (unsigned)DMA_REPEAT_TIMES; ++iter)
    {
        /* 准备数据：更换源数据图案以便可视验证 */
        for (unsigned i = 0; i < LEN; ++i) a[i] = (uint8_t)(iter + i);
        memset(b, 0, sizeof(b));
        /* 清 pending，避免上一轮残留影响 */
        S300_DMA_ClearAllInterrupts(S300_DMA0);
#if DMA_IRQ_DEMO_MODE == 0
        rc = S300_DMA_ConfigStd(S300_DMA0, S300_DMA_CH0,
                                (uint32_t)a, (uint32_t)b, LEN,
                                S300_DMA_TR_WIDTH_8);
        if (rc != 0)
        {
            printf("config err=%d\r\n", rc);
            break;
        }
        S300_DMA_SetIncrements(S300_DMA0, S300_DMA_CH0, S300_DMA_ADDR_INC, S300_DMA_ADDR_INC);
        S300_DMA_SetTransferType(S300_DMA0, S300_DMA_CH0, S300_DMA_TT_M2M_FD);
#else
        /* UART3 DMA TX：切换到 DMA 模式，目的写 STHR 阴影窗口 */
        while (!S300_UART_TxIdle(3)) { }
        {
            S300_UartFifoConfig fc = { .enable = 1, .rx_trig = S300_UART_RX_TRIG_1CHAR, .tx_trig = S300_UART_TX_TRIG_EMPTY, .dma_mode = 1 };
            (void)S300_UART_SetFIFO(3, &fc);
        }
        rc = S300_DMA_ConfigStd(S300_DMA0, S300_DMA_CH0,
                                (uint32_t)irq_msg,
                                (uint32_t)(UARTn_BASE(3) + 0x30u),
                                (uint32_t)(sizeof(irq_msg) - 1),
                                S300_DMA_TR_WIDTH_8);
        if (rc != 0)
        {
            printf("config err=%d\r\n", rc);
            break;
        }
        S300_DMA_SetIncrements(S300_DMA0, S300_DMA_CH0, S300_DMA_ADDR_INC, S300_DMA_ADDR_FIX);
        S300_DMA_SetTransferType(S300_DMA0, S300_DMA_CH0, S300_DMA_TT_M2P_FD);
        S300_DMA_SetHandshake(S300_DMA0, S300_DMA_CH0, S300_DMA_HS_NONE, S300_DMA_HS_UART3_TX);
#endif
        uint32_t prev = tfr_cnt;
        S300_DMA_Start(S300_DMA0, S300_DMA_CH0);
        /* 等待 ISR 计数递增或超时（不依赖忙位，以免 ISR 稍后到来） */
        volatile uint32_t spins = 20000000u; /* 适度放宽超时 */
        while ((tfr_cnt == prev) && --spins) { }
        if (spins == 0)
        {
            printf("[DMA][IRQ] iter %u timeout: no IRQ? tfr=%lu\r\n", iter, (unsigned long)tfr_cnt);
            break;
        }
#if DMA_IRQ_DEMO_MODE == 0
        /* 简单一致性校验 */
        if (b[LEN - 1] != a[LEN - 1])
        {
            printf("[DMA][IRQ] iter %u copy mismatch: b[%u]=%u vs a[%u]=%u\r\n",
                   iter, LEN - 1, b[LEN - 1], LEN - 1, a[LEN - 1]);
            break;
        }
        /* 本轮 OK 日志 */
        printf("[DMA][IRQ] iter %u OK, tfr=%lu isr=%lu, last b[%u]=%u\r\n",
               iter, (unsigned long)tfr_cnt, (unsigned long)isr_enter_cnt, LEN - 1, b[LEN - 1]);
#else
        /* 确保 UART 将 FIFO 数据发完，退出 DMA 模式，避免影响后续 printf */
        while (!S300_UART_TxIdle(3)) { }
        {
            S300_UartFifoConfig fc = { .enable = 1, .rx_trig = S300_UART_RX_TRIG_1CHAR, .tx_trig = S300_UART_TX_TRIG_EMPTY, .dma_mode = 0 };
            (void)S300_UART_SetFIFO(3, &fc);
        }
        /* 本轮 OK 日志（发送长度 = irq_msg 长度） */
        printf("[DMA][IRQ] iter %u OK, tfr=%lu isr=%lu, sent=%u bytes\r\n",
               iter, (unsigned long)tfr_cnt, (unsigned long)isr_enter_cnt, (unsigned)(sizeof(irq_msg) - 1));
#endif
    }
    /* 快照 DMA 全局状态位与掩码寄存器、CTLL.int_en，辅助判断是否有中断产生但未进入 NVIC */
    {
        volatile uint32_t *base = (volatile uint32_t *)S300_DMA_GetBase(S300_DMA0);
        uint32_t raw_tfr = base[0x02C0 / 4];
        uint32_t raw_blk = base[0x02C8 / 4];
        uint32_t st_tfr  = base[0x02E8 / 4];
        uint32_t st_blk  = base[0x02F0 / 4];
        uint32_t m_tfr  = base[0x0310 / 4];
        uint32_t m_blk  = base[0x0318 / 4];
        uint32_t m_s    = base[0x0320 / 4];
        uint32_t m_d    = base[0x0328 / 4];
        uint32_t m_err  = base[0x0330 / 4];
        uint32_t ctll   = base[(0x0018 + 0x58 * 0) / 4];
        /* 额外快照 INT_CTRL 和 NVIC 寄存器，核查路由与 NVIC 使能 */
        // uint32_t ic_mask0 = ARM_INT_MASK0;
        uint32_t nvic_iser0 = *((volatile uint32_t *)0xE000E100u);
        uint32_t nvic_icpr0 = *((volatile uint32_t *)0xE000E280u);
        printf("[DMA][IRQ] done, IRQ cnt=%lu(isr=%lu), last b[%u]=%u, RawTfr=0x%02lX, RawBlk=0x%02lX, StatusTfr=0x%02lX, StatusBlock=0x%02lX, MaskTfr=0x%04lX, MaskBlk=0x%04lX, MaskSrc=0x%04lX, MaskDst=0x%04lX, MaskErr=0x%04lX, CTLL=0x%08lX, INT_MASK0=0x%08lX, NVIC_ISER0=0x%08lX, NVIC_ICPR0=0x%08lX\r\n",
               (unsigned long)tfr_cnt, (unsigned long)isr_enter_cnt, LEN - 1, b[LEN - 1],
               (unsigned long)raw_tfr, (unsigned long)raw_blk,
               (unsigned long)st_tfr, (unsigned long)st_blk,
               (unsigned long)m_tfr, (unsigned long)m_blk, (unsigned long)m_s, (unsigned long)m_d, (unsigned long)m_err,
               (unsigned long)ctll, (unsigned long)0, (unsigned long)nvic_iser0, (unsigned long)nvic_icpr0);
    }
    while (1)
    {
        __NOP();
    }
}
