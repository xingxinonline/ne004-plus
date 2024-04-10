/*
 * @Author       : panxinhao
 * @Date         : 2023-07-25 11:04:26
 * @LastEditors  : panxinhao
 * @LastEditTime : 2023-08-16 15:05:21
 * @FilePath     : \\testbench_base\\cortexm4_timer\\main.c
 * @Description  :
 *
 * Copyright (c) 2023 by xinhao.pan@pimchip.cn, All Rights Reserved.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ne004xx.h"
#include "startriscv.h"

#include "riscv_dma.h"
#include "ne004xx_uart.h"

#define ARM_RISCV_IPCM_FFT      0x622000F8U
#define ARM_RISCV_IPCM_END      0x622000FCU

#define ARM_RISCV_STOP_VALUE    0x5A5A5A5AU

#define PDM_DMA_USE             DMA1
#define PDM_DMA_WRITE_CHANNEL   DMA_CH1

void Init_DWT_Clock(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; //Enable DWT
    DWT->CYCCNT = 0; //Clear the cycle counter
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; //Enable the cycle counter
}

uint32_t volatile start_cycles;
uint32_t volatile end_cycles;
uint32_t volatile nop_cycles;

static void __attribute__((optimize("O0"))) arm_stop(void)
{
    REG32(ARM_RISCV_IPCM_END) = ARM_RISCV_STOP_VALUE;
}

struct ring_buffer_t
{
    /** Index of head. */
    volatile uint32_t head_index;
    /** Index of tail. */
    volatile uint32_t tail_index;
    /** Buffer memory. */
    uint64_t buffer;
};

#define RING_BUFFER_SIZE 0x10000
#define RING_BUFFER_MASK (RING_BUFFER_SIZE - 1)

typedef struct ring_buffer_t ring_buffer_t;


static volatile ring_buffer_t *npu_buffer = (struct ring_buffer_t *)0x60000000U;
static volatile uint8_t pdm_avail_flag = 0;

#define PDM_DATA_WIDTH          0x100U //number of 16bit
#define PDM_BYTES               (PDM_DATA_WIDTH * 2)    //number of bytes when pdm avail
#define PDM_AVAIL_REPEAT        (0x2000U / PDM_DATA_WIDTH)  //number of pdm repeat
#define PDM_SRAM_REPEAT         (0x8000U / PDM_BYTES)        //32KB sram
#define PDM_NPU_SRAM_REPEAT     (0x20000U / PDM_BYTES)    //128KB sram

int main(void)
{
    uint32_t temp = 0x0U;
    Init_DWT_Clock();
    start_cycles = DWT->CYCCNT;
    // NOP指令
    __NOP();
    end_cycles = DWT->CYCCNT;
    nop_cycles = end_cycles - start_cycles;
    arm_stop();
    /* disable pdm */
    REG32(0x4000D08CU) = 0x0U;
    REG32(0x4000D05CU) = 0x2U;
    /* disable all interrupt */
    REG32(0x4000D0F8U) = 0x0U;  //屏蔽riscv所有外部中断
    REG32(0x4000D0FCU) = 0x0U;  //屏蔽riscv2arm所有中断
    REG64(0x4000D174U) = 0x0U;  //屏蔽arm所有个中断
    REG64(0x4000D17CU) = 0x0U;  //屏蔽arm2riscv所有中断
    npu_buffer->head_index = 0;
    /* config IO */
    PAD_GP7_FUNCSEL  |= (0x3 << 28); // JTAG
    PAD_GP8_FUNCSEL  |= (0x3);
    PAD_GP9_FUNCSEL  |= (0x3 << 4);
    PAD_GP10_FUNCSEL |= (0x03 << 8);
    PAD_GP13_FUNCSEL |= (0x01 << 20); // riscv UART
    PAD_GP14_FUNCSEL |= (0x1 << 24);
    PAD_GP15_FUNCSEL |= (0x02 << 28); // ARM UART
    PAD_GP16_FUNCSEL |= (0x2);
    start_riscv(1);
    /* enable riscv interrupt parameters */
    REG32(0x4000D0F8U) |= 0x1U << RISCV_ARM2RISCV_IRQ;
    REG32(0x4000D0F8U) |= 0x1U << RISCV_DMA1_IRQ;
    REG32(0x4000D0F8U) |= 0x1U << RISCV_ARM_NOTICE_IRQ;
    /* enable arm interrupt parameters */
    REG64(0x4000D174U) |= 1ULL << PDM_AVAIL_IRQn;   //允许pdm2pcm中断
    REG64(0x4000D174U) |= 1ULL << RISCV_NOTICE_IRQn; //允许riscv_notice中断
    /* enable arm2riscv interrupt */
    REG64(0x4000D17CU) |= 1ULL << REQ_FFT_DONE_IRQn; //发送FFT中断到riscv处理
    /* enable arm nvic irq */
    NVIC_EnableIRQ(PDM_AVAIL_IRQn);
    NVIC_EnableIRQ(RISCV_NOTICE_IRQn);
    UART_BAUD(UART0) = UART_BAUD_DIV & (12500000U / 460800);
    UART_CTRL(UART0) |= UART_CTRL_TEN | UART_CTRL_REN;
    //发送测试字节
    UART_DATA(UART0) = 0xAA;
    riscv_dma_enable(PDM_DMA_USE);
    riscv_dma_init(PDM_DMA_USE, PDM_DMA_WRITE_CHANNEL, 0x1000, 8, 8);
    riscv_dma_interrupt_disable(PDM_DMA_USE, PDM_DMA_WRITE_CHANNEL);
    /* enable pdm */
    REG32(0x4000D100U) = PDM_DATA_WIDTH;// 配置params_pdm_width
    REG32(0x4000D08CU) = 0x1U;
    while (1)
    {
        if (pdm_avail_flag)
        {
            static size_t pdm_avail_cnt = 0;
            pdm_avail_flag = 0;
            riscv_dma_start(PDM_DMA_USE, PDM_DMA_WRITE_CHANNEL, 0x46000000 + PDM_BYTES * (pdm_avail_cnt % PDM_BYTES), 0x60100000 + PDM_BYTES * (pdm_avail_cnt % PDM_NPU_SRAM_REPEAT));
            while (DMA_CHEN(PDM_DMA_USE) & (DMA_CHEN_CH_EN & (0x01ULL << (PDM_DMA_WRITE_CHANNEL - 1))));
            npu_buffer->head_index = ((npu_buffer->head_index + PDM_DATA_WIDTH) & RING_BUFFER_MASK);
            pdm_avail_cnt++;
            if ((pdm_avail_cnt % 8) == 0)
            {
                /* code */
                arm_stop();
            }
        }
    }
    (void)temp;
    return 0;
}
void PDM_AVAIL_IRQHandler(void)
{
    static size_t pdm_isr_cnt = 0;
    // ARM清pdm_avail中断, 配置地址32‘h4000_d104, 配置数据32’h0000_0001。
    REG32(0x4000D104U) = 0x00000001U;
    // ARM清pdm_avail中断, 配置地址32‘h4000_d104, 配置数据32’h0000_0000。
    REG32(0x4000D104U) = 0x00000000U;
    pdm_isr_cnt++;
    REG32(0x4000D100U) = PDM_DATA_WIDTH + PDM_DATA_WIDTH * (pdm_isr_cnt % PDM_AVAIL_REPEAT);// 配置params_pdm_width
    pdm_avail_flag = 1;
}

void RISCV_NOTICE_IRQHandler(void)
{
#define FFT_HARDWARE_START      (1U << 0)
#define FFT_HARDWARE_END        (1U << 1)
    //TODO 清除riscv notice中断
    REG32(0x4000D194U) = 0x1U;
    REG32(0x4000D194U) = 0x0U;
    uint32_t temp = REG32(ARM_RISCV_IPCM_FFT);
    if (temp & FFT_HARDWARE_START)
    {
        //TODO 启动fft
        REG32(0x4000D0ACU) = 0x1U;
        //TODO 清除IPCM
        temp &= ~FFT_HARDWARE_START;
        REG32(ARM_RISCV_IPCM_FFT) = temp;
    }
    else if (temp & FFT_HARDWARE_END)
    {
        //TODO 停止fft
        REG32(0x4000D0ACU) = 0x0U;
        //TODO 清除IPCM
        temp &= ~FFT_HARDWARE_END;
        REG32(ARM_RISCV_IPCM_FFT) = temp;
    }
}

void arm_delay_us(uint32_t us)
{
    volatile uint32_t i = 0, j = 10;
    for (i = 0; i < us; i++)//18+5 = 23cycle
    {
        while (j)//18cycle 100/20
        {
            j--;
        }
    }
}
