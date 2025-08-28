#include "s300.h"
#include "rcc.h"
#include "gpio.h"
#include "dma.h"
#include "i2s.h"
#include "uart.h"
#include "i2c_soft.h"
#include "wm8978.h"
#include "board.h"
#include <stdio.h>
/* Demo: I2S1 record and play in DMA mode, using WM8978 via I2C1 soft (legacy order) */

#define DMA_BUFFER_LEN   (4096u)
#define DMA_BUFFER_COUNT (8u)
/* 与旧源码一致：录音/播放缓冲基址相同，按块索引推进 */
#define DMA_REC_BUFFER(n) (DSP_SRAM0_BASE + ((n) * DMA_BUFFER_LEN))
#define DMA_PLA_BUFFER(n) (DSP_SRAM0_BASE + ((n) * DMA_BUFFER_LEN))

volatile uint32_t record_running = 0;
volatile uint8_t rec_buf_id;
volatile uint8_t pla_buf_id;

/* no UART debug in legacy-aligned demo */

static void i2s1_pins_init(void)
{
    /* A3: MCLK, A6: BCLK, A7: LRCLK, A8: SDOUT, A9: SDIN (per legacy demo) */
    int port = GPIOA;
    uint8_t pins[5] = {3, 6, 7, 8, 9};
    int funcs[5] = {FUNCTION_1, FUNCTION_3, FUNCTION_3, FUNCTION_3, FUNCTION_3};
    uint32_t modes[5] = {GPIO_DOWN, GPIO_DOWN, GPIO_DOWN, GPIO_DOWN, GPIO_DOWN};
    uint32_t isout[5] = {1, 1, 1, 1, 0};
    /* 确保 GPIO 时钟打开 */
    set_cortex_m4_apb1_clock(RCC_CM4_APB1_GPIO, true);
    for (int i = 0; i < 5; ++i)
    {
        set_gpio_function(port, pins[i], funcs[i]);
        set_gpio_direction(port, pins[i], isout[i]);
        gpio_set_mode(port, pins[i], modes[i]);
    }
}

static int wm8978_basic_cfg(i2c_soft_t *i2c)
{
    int ret = wm8978_init(i2c);
    if (ret) return ret;
    wm8978_set_hp_vol(i2c, 40, 40);
    wm8978_set_spk_vol(i2c, 50);
    wm8978_set_adda(i2c, true, true);
    wm8978_set_input(i2c, true, true, false);
    wm8978_set_output(i2c, true, false);
    wm8978_set_mic_gain(i2c, 46);
    wm8978_i2s_cfg(i2c, 2, 0); /* 2=I2S 模式, 0=16-bit，与旧 demo 对齐 */
    return 0;
}

/* Simple NVIC enable for DMA0 (legacy: set_interrupt + set_cortex_m4_interrupt_switch) */
static void enable_dma0_irq(void)
{
    NVIC_ClearPendingIRQ(DMA0_IRQn);
    NVIC_SetPriority(DMA0_IRQn, 5);
    NVIC_EnableIRQ(DMA0_IRQn);
}

int main(void)
{
    /* 统一板级初始化：先时钟后 UART */
    board_init();
    printf("hello world from bsp!\n");
    printf("CoreClk=%lu APB1=%lu\n", (unsigned long)SystemCoreClock, (unsigned long)rcc_get_clock(RCC_CLOCK_APB1));
    /* 1) I2S1 pins */
    i2s1_pins_init();
    printf("I2S1 pins configured.\n");
    /* 2) WM8978 via soft I2C1 */
    i2c_soft_t i2c1;
    if (i2c_soft_init_default_idx(&i2c1, 1, 100000))
    {
        for (;;) { /* i2c init failed */ }
    }
    i2c_soft_bus_recover(&i2c1);
    if (wm8978_basic_cfg(&i2c1))
    {
        for (;;) { /* wm8978 init failed */ }
    }
    printf("WM8978 configured (HP=40,SPK=50,MIC=46, I2S fmt=2 len=16).\n");
    wm8978_dump(&i2c1);
    /* 3) Audio PLL and I2S clock for 12MHz MCLK (from legacy params) */
    while (init_audio_pll(3, 129, 500000, 7, 6) != 0) {}
    set_audio_clock(1, true);
    /* 打开 I2S1 外设 APB 时钟 */
    set_cortex_m4_apb1_clock(RCC_CM4_APB1_I2S1, true);
    set_i2s_basic_config(1, 12000000, I2S_WORD_16);
    set_i2s_en(1, 1);
    printf("Audio PLL set, I2S1 MCLK=12MHz, 16-bit enabled.\n");
    /* ---- RCC/PLL/I2S 时钟相关寄存器一次性打印 ---- */
    printf("RCC: SYS_CLK_SEL=%08lx APB_CLK_DIV=%08lx\n",
           (unsigned long)RCC->CM4_SYS_CLK_SEL,
           (unsigned long)RCC->CM4_APB_CLK_DIV);
    printf("RCC: AUDIO_PLL_CTL=%08lx AUDIO_PLL_CTL2=%08lx PLL_LOCK=%08lx\n",
           (unsigned long)RCC->CM4_AUDIO_PLL_CTL,
           (unsigned long)RCC->CM4_AUDIO_PLL_CTL2,
           (unsigned long)RCC->CM4_PLL_LOCK_STATUS);
    printf("RCC: I2S_CLK_DIV=%08lx APB1_EN=%08lx AUDIO_CLK_EN=%08lx AUDIO_RSTN=%08lx\n",
           (unsigned long)RCC->CM4_I2S_CLK_DIV,
           (unsigned long)RCC->CM4_APB1_CLK_EN,
           (unsigned long)RCC->CM4_AUDIO_PERF_CLK_EN,
           (unsigned long)RCC->CM4_AUDIO_RSTN_CTL);
    /* 对齐正常 demo：在启用 DMA 模式之前，确保 I2S1 DMACR 为 0 */
    I2S1->DMACR = 0;
    printf("I2S1: CER=%08lx IER=%08lx TER0=%08lx RER0=%08lx CCR=%08lx DMACR=%08lx SR=%08lx\n",
           (unsigned long)I2S1->CER,
           (unsigned long)I2S1->IER,
           (unsigned long)I2S1->TER0,
           (unsigned long)I2S1->RER0,
           (unsigned long)I2S1->CCR,
           (unsigned long)I2S1->DMACR,
           (unsigned long)I2S1->SR);
    printf("I2S1: TCR0=%08lx RCR0=%08lx IMR0=%08lx\n",
           (unsigned long)I2S1->TCR0,
           (unsigned long)I2S1->RCR0,
           (unsigned long)I2S1->IMR0);
    /* 保持与参考相同：此时尚未启用 DMA，使得 DMACR=0x0 */
    /* 4) DMA: open DMA0 clock, init controller */
    rcc_set_cortex_m4_sys_clock(1, 0, 1, true);
    init_dma(EM_DMA0);
    /* 打印 rcc_set_cortex_m4_sys_clock 生效的寄存器（CM4_SYS_CLK_EN） */
    printf("RCC: CM4_SYS_CLK_EN=%08lx (AON/DMA1/DMA0 bits)\n", (unsigned long)RCC->CM4_SYS_CLK_EN);
    /* 打印 DMA 控制器全局寄存器状态 */
    printf("DMA0: DmaCfg=%08lx ChEn=%08lx MaskTfr=%08lx MaskBlock=%08lx MaskSrc=%08lx MaskDst=%08lx MaskErr=%08lx\n",
           (unsigned long)DMAC0->DmaCfgReg,
           (unsigned long)DMAC0->ChEnReg,
           (unsigned long)DMAC0->MaskTfr,
           (unsigned long)DMAC0->MaskBlock,
           (unsigned long)DMAC0->MaskSrcTran,
           (unsigned long)DMAC0->MaskDstTran,
           (unsigned long)DMAC0->MaskErr);
    /* 5) Enable DMA0 IRQ（打印顺序与参考保持一致，先启用 IRQ 再打印 SYS_CLK_EN） */
    enable_dma0_irq();
    printf("DMA0 IRQ enabled.\n");
    /* 6) I2S1 DMA mode: rec ch0, play ch1 (legacy order) */
    rec_buf_id = 0;
    pla_buf_id = 0;
    set_i2s_dma_mode(1, 0, 1, (uint8_t *)DMA_REC_BUFFER(rec_buf_id), (uint8_t *)DMA_PLA_BUFFER(pla_buf_id), DMA_BUFFER_LEN);
    /* 与参考打印对齐：修正 CH0/CH1 的 CFG_L，仅设置对应 HS_SEL 位 */
    {
        S300_DMA_TypeDef *D = DMAC0;
        uint32_t v0 = D->CH[0].CFG_L;
        v0 &= ~0x00000600u; /* 清 0x600（避免多出 0x200） */
        v0 |= 0x000004E0u;  /* 设为 0x004E0 */
        D->CH[0].CFG_L = v0;
        uint32_t v1 = D->CH[1].CFG_L;
        v1 &= ~0x00000C00u; /* 清 0xC00 */
        v1 |= 0x000008E0u;  /* 设为 0x008E0 */
        D->CH[1].CFG_L = v1;
    }
    /* 打印 DMA0 通道 0/1 基本配置寄存器 */
    {
        S300_DMA_TypeDef *D = DMAC0;
        printf("DMA0 CH0: SAR=%08lx DAR=%08lx CTL_L=%08lx CTL_H=%08lx CFG_L=%08lx CFG_H=%08lx\n",
               (unsigned long)D->CH[0].SAR,
               (unsigned long)D->CH[0].DAR,
               (unsigned long)D->CH[0].CTL_L,
               (unsigned long)D->CH[0].CTL_H,
               (unsigned long)D->CH[0].CFG_L,
               (unsigned long)D->CH[0].CFG_H);
        printf("DMA0 CH1: SAR=%08lx DAR=%08lx CTL_L=%08lx CTL_H=%08lx CFG_L=%08lx CFG_H=%08lx\n",
               (unsigned long)D->CH[1].SAR,
               (unsigned long)D->CH[1].DAR,
               (unsigned long)D->CH[1].CTL_L,
               (unsigned long)D->CH[1].CTL_H,
               (unsigned long)D->CH[1].CFG_L,
               (unsigned long)D->CH[1].CFG_H);
    }
    printf("DMA mode started. rec_buf=%u pla_buf=%u len=%u\n", rec_buf_id, pla_buf_id, (unsigned)DMA_BUFFER_LEN);
    printf("I2S1 DMACR after start: %08lx\n", (unsigned long)I2S1->DMACR);
    while (1)
    {
        /* 原始 demo：主循环不做额外处理 */
    }
}

void DMA0_IRQHandler(void)
{
    /* 与原始 demo 一致：清所有 DMA 中断标志，不判断具体通道完成位 */
    DMAC0->ClearTfr = 0xFFu;
    DMAC0->ClearBlock = 0xFFu;
    DMAC0->ClearSrcTran = 0xFFu;
    DMAC0->ClearDstTran = 0xFFu;
    DMAC0->ClearErr = 0xFFu;
    record_running = 1;
    /* 通道0（录音）空闲则旋转缓冲并重装地址后启动 */
    if (is_dma_busy(EM_DMA0, 0) == 0)
    {
        rec_buf_id = (uint8_t)((rec_buf_id + 1) % DMA_BUFFER_COUNT);
        set_dma_std_address(EM_DMA0, 0, (uint32_t)&I2S1->RXDMA, (uint32_t)DMA_REC_BUFFER(rec_buf_id));
        set_dma_start(EM_DMA0, 0);
    }
    /* 通道1（播放）空闲则旋转缓冲并重装地址后启动 */
    if (is_dma_busy(EM_DMA0, 1) == 0)
    {
        pla_buf_id = (uint8_t)((pla_buf_id + 1) % DMA_BUFFER_COUNT);
        set_dma_std_address(EM_DMA0, 1, (uint32_t)DMA_PLA_BUFFER(pla_buf_id), (uint32_t)&I2S1->TXDMA);
        set_dma_start(EM_DMA0, 1);
    }
}
