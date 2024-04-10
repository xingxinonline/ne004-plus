/*
 * @Author       : panxinhao
 * @Date         : 2023-07-05 19:08:38
 * @LastEditors  : panxinhao
 * @LastEditTime : 2024-04-10 18:45:18
 * @FilePath     : \\ne004-plus\\riscv64_default\\main.c
 * @Description  :
 *
 * Copyright (c) 2023 by xinhao.pan@pimchip.cn, All Rights Reserved.
 */

#include <stdio.h>
#include <string.h>

#include "reg.h"
#include "plic.h"
#include "dma.h"

#include "vad.h"
#include "mfcc.h"
#include "malloc.h"

#include "ringbuffer.h"
#include "layer_infer.h"
#include "layer_tools.h"

#include "legacy/gain_control.h"


#define FFT_HARDWARE_START      (1U << 0)
#define FFT_HARDWARE_END        (1U << 1)

#define ARM_RISCV_IPCM_PDM      0x622000F4U
#define ARM_RISCV_IPCM_FFT      0x622000F8U
#define ARM_RISCV_IPCM_END      0x622000FCU

#define ARM_RISCV_STOP_VALUE    0x5A5A5A5AU

#define FFT_DMA_USE             DMA1
#define FFT_DMA_READ_CHANNEL    DMA_CH7
#define FFT_DMA_WRITE_CHANNEL   DMA_CH8

void user_interrupt_handler(void);

static uint32_t SystemCoreClock = 400000000U;

static int32_t output_value[1024] = {0};

static volatile uint32_t riscv_task_flag = 0;
static volatile size_t dma_cnt = 0xA5A50000;

uint64_t volatile start_cycles;
uint64_t volatile end_cycles;
uint64_t volatile nop_cycles;

#include <stdio.h>

#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)

PUTCHAR_PROTOTYPE
{
    while (REG8(0x64000003U) & 0x80);
    REG8(0x64000000U) = ch;
    return ch;
}

int _write(int fd, char *pBuffer, int size)
{
    (void)fd;
    for (int i = 0; i < size; i++)
    {
        /* code */
        __io_putchar(*pBuffer++);
    }
    return size;
}

static void __attribute__((optimize("O0"))) riscv_stop(void)
{
    REG32(ARM_RISCV_IPCM_END) = ARM_RISCV_STOP_VALUE;
}

void fft_hardware(int32_t *src_addr)
{
    /* 启动DMA搬运数据至FFT SRAM */
    // asm volatile("csrr %0, mcycle" : "=r"(start_cycles));
    dma_start(FFT_DMA_USE, FFT_DMA_WRITE_CHANNEL, (uint64_t)src_addr, 0x44000000);
    //TODO 等待DMA 结束
    while (DMA_CHEN(FFT_DMA_USE) & (DMA_CHEN_CH_EN & (0x01ULL << (FFT_DMA_WRITE_CHANNEL - 1))));
    // asm volatile("csrr %0, mcycle" : "=r"(end_cycles));
    // nop_cycles = end_cycles - start_cycles;
    // printf("fft_hardware write 2KB data time taken: %ldus\n", nop_cycles / 50);
    /* 启动FFT */
    // asm volatile("csrr %0, mcycle" : "=r"(start_cycles));
    REG32(ARM_RISCV_IPCM_FFT) |= FFT_HARDWARE_START;
    REG8(0x6400200CU) |= 0x8U;
    REG8(0x6400200CU) &= ~0x8U;
    riscv_task_flag |= FFT_HARDWARE_START;
    //TODO 等待FFT结束
    while (!(riscv_task_flag & FFT_HARDWARE_END));
    riscv_task_flag &= ~FFT_HARDWARE_END;
    // asm volatile("csrr %0, mcycle" : "=r"(end_cycles));
    // nop_cycles = end_cycles - start_cycles;
    // printf("fft_hardware compute time taken: %ldus\n", nop_cycles / 50);
    /* 启动DMA从FFT SRAM搬运计算结果 */
    // asm volatile("csrr %0, mcycle" : "=r"(start_cycles));
    dma_start(FFT_DMA_USE, FFT_DMA_READ_CHANNEL, (uint64_t)0x44000000, (uint64_t)output_value);
    //TODO 等待DMA 结束
    while (DMA_CHEN(FFT_DMA_USE) & (DMA_CHEN_CH_EN & (0x01ULL << (FFT_DMA_READ_CHANNEL - 1))));
    // asm volatile("csrr %0, mcycle" : "=r"(end_cycles));
    // nop_cycles = end_cycles - start_cycles;
    // printf("fft_hardware read 4KB data time taken: %ldus\n", nop_cycles / 50);
    /* 处理FFT结果 */
    // asm volatile("csrr %0, mcycle" : "=r"(start_cycles));
    for (size_t i = 0; i < 256; i++)
    {
        /* code */
        src_addr[2 * i] = output_value[4 * i] >> 8;
        src_addr[2 * i + 1] = output_value[4 * i + 1] >> 8;
        src_addr[2 * i + 512] = output_value[4 * i + 2] >> 8;
        src_addr[2 * i + 512 + 1] = output_value[4 * i + 3] >> 8;
    }
    // asm volatile("csrr %0, mcycle" : "=r"(end_cycles));
    // nop_cycles = end_cycles - start_cycles;
    // printf("fft_hardware all time taken: %ldus\n", nop_cycles / 50);
    // riscv_stop();
}

static ring_buffer_t *npu_buffer = (struct ring_buffer_t *)0x60000000U;
static int16_t vad_buffer[19200];
extern IOFeature ofeature;
BaseFloat *mfcc_feature;
int16_t out_row, out_col;
uint64_t volatile mfcc_start_cycles;
uint64_t volatile mfcc_end_cycles;
uint64_t volatile mfcc_nop_cycles;
uint64_t volatile kws_start_cycles;
uint64_t volatile kws_end_cycles;
uint64_t volatile kws_nop_cycles;

int main(void)
{
#define SAMPLE_RATE         16000 //采样率16k
#define CACHE_LEN           19200
#define MINLEVEL 12
#define MAXLEVEL 255
#define VAD_FRAME_LEN       512
#define VAD_SHIFT_LEN       256
#define LEFT_CXT_LEN        (VAD_FRAME_LEN * 6)
#define POP_POSITION        (CACHE_LEN - VAD_SHIFT_LEN)
#define PAD_DATA_LEN        (CACHE_LEN - LEFT_CXT_LEN - VAD_FRAME_LEN)
#define COMPUTE_POSITION    (CACHE_LEN - VAD_FRAME_LEN)
#define SHIFT_DATA_LEN      (CACHE_LEN - VAD_SHIFT_LEN)
    TMFCC *mfcc = NULL;
    uint64_t temp = 0;
    uint16_t shifted_len = 0;
    // 13.  RV将GPIO2、GPIO3设置为输出，配置8‘h0x64002008为2’b10
    REG8(0x64002008U) = 0x8U | 0x4U;
    plic_init();
    __asm volatile("csrs mie, %0" :: "r"(0x800));
    __asm volatile("csrs mstatus, 8");
    // f.   向baud_div寄存器(addr=0x6400_0018)写入数据0x364.
    REG16(0x64000018U) = SystemCoreClock / 2000000;
    // g.   向rx_ctrl寄存器(addr=0x6400_000C)写入数据0x1.
    REG8(0x6400000CU) = 0x1U;
    // h.   向tx_ctrl寄存器(addr=0x6400_0008)写入数据0x1.
    REG8(0x64000008U) = 0x1U;
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Hello from NE004-PLUS riscv64 core!\r\n"); 
    // //TODO heap init
    my_mem_init(SRAMIN);
    /*ringbuffer init*/
    npu_buffer->buffer = (int16_t *)0x60100000;
    // npu_buffer->head_index = 0; //由arm初始化
    npu_buffer->tail_index = 0;
    /*kws init*/
    inference_init();
    /*mfcc init*/
    mfcc = MfccInit();
    /*feature init*/
    mfcc_feature = (BaseFloat *)mymalloc(0, sizeof(BaseFloat) * 590) ;
    uint8_t rmse = 0;
    uint8_t threshold = 0;
    VAD     *vad  = NULL;
    Context *cxt  = NULL;
    /*vad init*/
    init_cxt(&cxt);
    init_vad(&vad);
    dma_enable(FFT_DMA_USE);
    dma_init(FFT_DMA_USE, FFT_DMA_WRITE_CHANNEL, 0x800, 4, 4);
    dma_init(FFT_DMA_USE, FFT_DMA_READ_CHANNEL, 0x1000, 4, 4);
    /* agc init */
    // int16_t *pcm_data = vad_buffer;
    // read_pcm_binary("data/low.pcm", &pcm_data, &kSamples);
    // printf("frist element is: %d\tkSamples is: %d\n", pcm_data[0], kSamples);
    // Init
    void *hAGC = WebRtcAgc_Create();
    int agcMode = kAgcModeFixedDigital;
    WebRtcAgc_Init(hAGC, MINLEVEL, MAXLEVEL, agcMode, SAMPLE_RATE);
    WebRtcAgcConfig agcConfig;
    agcConfig.targetLevelDbfs = 9;     // 3 default
    agcConfig.compressionGaindB = 15;  // 9 default
    agcConfig.limiterEnable = 1;
    WebRtcAgc_set_config(hAGC, agcConfig);
    // Analyse
#define AGC_FRAMES  (SAMPLE_RATE / 100)  // 160
    int32_t gains[11];
    int32_t inMicLevel = 0;
    int32_t outMicLevel = 0;
    uint8_t saturationWarning = 0;
    int16_t buffer_in[AGC_FRAMES];
    int16_t buffer_out[AGC_FRAMES];
    int16_t *buffer_in_channel[1] = {buffer_in};
    int16_t *buffer_out_channel[1] = {buffer_out};
    // riscv_stop();
    while (1)
    {
        if (ring_buffer_num_items(npu_buffer) >= VAD_SHIFT_LEN)
        {
            // asm volatile("csrr %0, mcycle" : "=r"(start_cycles));
            ring_buffer_dequeue_arr(npu_buffer, vad_buffer + POP_POSITION, VAD_SHIFT_LEN);
            int npu_buffer_size = ring_buffer_num_items(npu_buffer) * 2;
            float npu_buffer_usage = npu_buffer_size * 100.0 / (128 * 1024);
            char buffer[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            snprintf(buffer, sizeof(buffer), "%.2f%%", npu_buffer_usage);
            // if (npu_buffer_size)
            // {
            //     /* code */
            //     // printf("npu_buffer->head_index = %d, npu_buffer->tail_index: %d\n", npu_buffer->head_index, npu_buffer->tail_index);
            //     printf("npu_buffer->buffer_size = %d, npu_buffer usage: %s\n", npu_buffer_size, buffer);
            // }
            // printf("npu_buffer->buffer_size = %d, npu_buffer usage: %s\n", npu_buffer_size, buffer);
            // asm volatile("csrr %0, mcycle" : "=r"(end_cycles));
            // nop_cycles = end_cycles - start_cycles;
            // printf("vad read 0.5KB time taken: %ldus\n", nop_cycles / 50);
            // asm volatile("csrr %0, mcycle" : "=r"(start_cycles));
            OSTATE cur_state = dynamic_energy_vad(vad_buffer + COMPUTE_POSITION, VAD_FRAME_LEN, cxt, vad, &rmse, &threshold);
            // asm volatile("csrr %0, mcycle" : "=r"(end_cycles));
            // nop_cycles = end_cycles - start_cycles;
            // printf("vad compute time taken: %ldus\n", nop_cycles / 50);
            /* code */
            if (1)
            {
                // if (shifted_len == 0)
                // {
                //     /* code */
                //     asm volatile("csrr %0, mcycle" : "=r"(start_cycles));
                // }
                shifted_len += VAD_SHIFT_LEN;
                if (shifted_len > PAD_DATA_LEN)
                {
                    // asm volatile("csrr %0, mcycle" : "=r"(end_cycles));
                    // nop_cycles = end_cycles - start_cycles;
                    // printf("vad compute 37.5KB time taken: %ldus\n", nop_cycles / 50);
                    /* agc */
                    asm volatile("csrr %0, mcycle" : "=r"(start_cycles));
                    for (size_t n = 0; n < (CACHE_LEN / AGC_FRAMES); n++)
                    {
                        memcpy(buffer_in, vad_buffer + n * AGC_FRAMES, sizeof(int16_t) * AGC_FRAMES);
                        // printf("agc buffer in addr is: %p, %p\n", buffer_in, (int16_t *)&buffer_in);
                        WebRtcAgc_Analyze(hAGC, (const int16_t *const *)buffer_in_channel, 1,
                                          AGC_FRAMES, inMicLevel, &outMicLevel, 0, &saturationWarning,
                                          gains);
                        WebRtcAgc_Process(hAGC, gains, (const int16_t *const *)buffer_in_channel, 1,
                                          (int16_t *const *)buffer_out_channel);
                        // write to pcm
                        // fwrite(buffer_out, 2, nFrames, pcm_data + n * nFrames);
                        memcpy(vad_buffer + n * AGC_FRAMES, buffer_out, sizeof(int16_t) * AGC_FRAMES);
                    }
                    asm volatile("csrr %0, mcycle" : "=r"(end_cycles));
                    nop_cycles = end_cycles - start_cycles;
                    printf("agc time taken: %ldus, ", nop_cycles / 50);
                    /* mfcc */
                    asm volatile("csrr %0, mcycle" : "=r"(mfcc_start_cycles));
                    MfccStart(mfcc, vad_buffer, CACHE_LEN, &mfcc_feature, &out_row, &out_col);
                    asm volatile("csrr %0, mcycle" : "=r"(mfcc_end_cycles));
                    // for (size_t i = 0; i < 590; i++)
                    // {
                    //     /* code */
                    //     printf("mfcc[%d]= 0x%08x\r\n", i, *((uint32_t *)mfcc_feature + i));
                    // }
                    /* kws */
                    asm volatile("csrr %0, mcycle" : "=r"(kws_start_cycles));
                    inference_start(mfcc_feature);
                    int8_t  res_idx = get_max_id((int8_t *)ofeature.feature_buffer, 13, 30, 50);
                    asm volatile("csrr %0, mcycle" : "=r"(end_cycles));
                    nop_cycles = end_cycles - start_cycles;
                    
                    asm volatile("csrr %0, mcycle" : "=r"(kws_end_cycles));
                    // printf("\n");
                    // for (size_t i = 0; i < 13; i++)
                    // {
                    //     /* code */
                    //     if ((int8_t)ofeature.feature_buffer[i] >= 0)
                    //     {
                    //         /* code */
                    //         printf("ofeature[%ld]= %d\r\n", i, (int8_t)ofeature.feature_buffer[i]);
                    //     }
                    // }
                    mfcc_nop_cycles = mfcc_end_cycles - mfcc_start_cycles;
                    kws_nop_cycles = kws_end_cycles - kws_start_cycles;
                    printf("mfcc time taken = %ldus, kws time taken = %ldus ", mfcc_nop_cycles / 50, kws_nop_cycles / 50);
                    printf("1.2s compute time taken: %ldus, ", nop_cycles / 50);
                    printf("ofeature[%d] = %d, result : ", res_idx, (int8_t)ofeature.feature_buffer[res_idx]);
                    print_word(res_idx);
                    // riscv_stop();
                    shifted_len = 0;
                    cur_state = 0;
                }
            }
            // asm volatile("csrr %0, mcycle" : "=r"(start_cycles));
            my_memmove(vad_buffer, vad_buffer + VAD_SHIFT_LEN, SHIFT_DATA_LEN * sizeof(int16_t));
            // asm volatile("csrr %0, mcycle" : "=r"(end_cycles));
            // nop_cycles = end_cycles - start_cycles;
            // printf("vad memmove 37.5KB time taken: %ldus\n", nop_cycles / 50);
            // riscv_stop();
        }
    }
    (void)temp;
    return 0;
}

void handle_trap(int64_t ulMCAUSE, uint64_t ulMEPC)
{
    if (ulMCAUSE & 0x8000000000000000UL)
    {
        if ((ulMCAUSE & 0xffUL) == 0x0B)
        {
            user_interrupt_handler();
        }
        return;
    }
    (void)ulMCAUSE;
    (void)ulMEPC;
    while (1);
}

void risc_v_application_interrupt_handler(int64_t ulMCAUSE, uint64_t ulMEPC)
{
    handle_trap(ulMCAUSE, ulMEPC);
}

void risc_v_application_exception_handler(int64_t ulMCAUSE, uint64_t ulMEPC)
{
    handle_trap(ulMCAUSE, ulMEPC);
}

void user_interrupt_handler(void)
{
    int source = plic_claim();
    /*do anything you need according to interrupt source number*/
    if (source == ARM2RISCV_IRQ)
    {
        if (riscv_task_flag & FFT_HARDWARE_START)
        {
            // 14.  RV将GPIO3拉高再拉低通知arm清除fft中断
            REG32(ARM_RISCV_IPCM_FFT) |= FFT_HARDWARE_END;
            REG8(0x6400200CU) |= 0x8U;
            REG8(0x6400200CU) &= ~0x8U;
            riscv_task_flag &= ~FFT_HARDWARE_START;
            riscv_task_flag |= FFT_HARDWARE_END;
        }
    }
    else if (source == ARM_NOTICE_IRQ)
    {
        REG8(0x6400200CU) |= 0x4U;
        REG8(0x6400200CU) &= ~0x4U;
    }
    else if (source == DMA1_IRQ)
    {
        if (DMA_CHX_INTSTATUSREG(DMA1, DMA_CH1) & DMA_CHX_INTSTATUS_BLOCK_TRF_DONE)
        {
            DMA_CHX_INTCLEAR(DMA1, DMA_CH1) = 0xFFFFFFFFUL;
        }
        else if (DMA_CHX_INTSTATUSREG(DMA1, DMA_CH2) & DMA_CHX_INTSTATUS_BLOCK_TRF_DONE)
        {
            DMA_CHX_INTCLEAR(DMA1, DMA_CH2) = 0xFFFFFFFFUL;
        }
        else if (DMA_CHX_INTSTATUSREG(DMA1, DMA_CH3) & DMA_CHX_INTSTATUS_BLOCK_TRF_DONE)
        {
            DMA_CHX_INTCLEAR(DMA1, DMA_CH3) = 0xFFFFFFFFUL;
        }
    }
    plic_complete(source);
    return;
}
