/*
 * @Author       : panxinhao
 * @Date         : 2023-07-05 19:08:38
 * @LastEditors  : xingxinonline
 * @LastEditTime : 2024-08-07 18:31:56
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

#include "agc.h"
#include "vad_core.h"
#include "mfcc_10_float.h"
#include "Infer.h"

#include "malloc.h"
#include "ringbuffer.h"

#define FFT_HARDWARE_START      (1U << 0)
#define FFT_HARDWARE_END        (1U << 1)

#define ARM_RISCV_IPCM_KWS      0x622000F0U
#define ARM_RISCV_IPCM_PDM      0x622000F4U
#define ARM_RISCV_IPCM_FFT      0x622000F8U
#define ARM_RISCV_IPCM_END      0x622000FCU

#define ARM_RISCV_STOP_VALUE    0x5A5A5A5AU

#define FFT_DMA_USE             DMA1
#define FFT_DMA_READ_CHANNEL    DMA_CH7
#define FFT_DMA_WRITE_CHANNEL   DMA_CH8

void user_interrupt_handler(void);

static uint32_t SystemCoreClock = 400000000U;

static volatile uint32_t riscv_task_flag = 0;
static volatile size_t dma_cnt = 0xA5A50000;
static ring_buffer_t *npu_buffer = (struct ring_buffer_t *)0x60000000U;

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

#if 0
static int32_t output_value[1024] = {0};
void fft_hardware(int32_t *src_addr)
{
    /* ?????DMA????????????FFT SRAM */
    // asm volatile("csrr %0, mcycle" : "=r"(start_cycles));
    dma_start(FFT_DMA_USE, FFT_DMA_WRITE_CHANNEL, (uint64_t)src_addr, 0x44000000);
    //TODO ???DMA ????
    while (DMA_CHEN(FFT_DMA_USE) & (DMA_CHEN_CH_EN & (0x01ULL << (FFT_DMA_WRITE_CHANNEL - 1))));
    // asm volatile("csrr %0, mcycle" : "=r"(end_cycles));
    // nop_cycles = end_cycles - start_cycles;
    // printf("fft_hardware write 2KB data time taken: %ldus\n", nop_cycles / 50);
    /* ?????FFT */
    // asm volatile("csrr %0, mcycle" : "=r"(start_cycles));
    REG32(ARM_RISCV_IPCM_FFT) |= FFT_HARDWARE_START;
    REG8(0x6400200CU) |= 0x8U;
    REG8(0x6400200CU) &= ~0x8U;
    riscv_task_flag |= FFT_HARDWARE_START;
    //TODO ???FFT????
    while (!(riscv_task_flag & FFT_HARDWARE_END));
    riscv_task_flag &= ~FFT_HARDWARE_END;
    // asm volatile("csrr %0, mcycle" : "=r"(end_cycles));
    // nop_cycles = end_cycles - start_cycles;
    // printf("fft_hardware compute time taken: %ldus\n", nop_cycles / 50);
    /* ?????DMA??FFT SRAM???????????? */
    // asm volatile("csrr %0, mcycle" : "=r"(start_cycles));
    dma_start(FFT_DMA_USE, FFT_DMA_READ_CHANNEL, (uint64_t)0x44000000, (uint64_t)output_value);
    //TODO ???DMA ????
    while (DMA_CHEN(FFT_DMA_USE) & (DMA_CHEN_CH_EN & (0x01ULL << (FFT_DMA_READ_CHANNEL - 1))));
    // asm volatile("csrr %0, mcycle" : "=r"(end_cycles));
    // nop_cycles = end_cycles - start_cycles;
    // printf("fft_hardware read 4KB data time taken: %ldus\n", nop_cycles / 50);
    /* ????FFT??? */
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
#endif

#define FFT_SIZE        512
#define FRM_SHIFT       256
#define FEATURE_BLOCK   74
#define FEATURE_NUM     10
#define COMMAND_NUM     13

int16_t frame_s16[FRM_SHIFT];
float fft_block[FFT_SIZE];
int16_t inbuf[FRM_SHIFT];
float feat_block[FEATURE_NUM];
float mfcc_feature[FEATURE_BLOCK * FEATURE_NUM];
uint8_t output[COMMAND_NUM];

uint8_t th[COMMAND_NUM] =
{
    90,      //未知
    105,      //机械臂归零
    105,      //机械臂前进
    105,      //机械臂后退
    105,      //机械臂左转
    105,      //机械臂右转
    105,      //机械臂上升
    105,      //机械臂下降
    105,      //机械臂抓取
    95,       //机械臂松开
    105,      //机械臂加速
    105,      //机械臂减速
    105,      //err
};

void show_res(Tensor *output)
{
    uint8_t max_val = 0;
    int16_t idx_val = 0;
    uint8_t *data_out = (uint8_t *)output->data_;
    for (int32_t i = 0; i < output->data_len_; ++i)
    {
        // printf("%u,", data_out[i]);
        if (max_val < data_out[i])
        {
            max_val = data_out[i];
            idx_val = i;
        }
        data_out[i] = 0;
    }
    
    
    switch (idx_val)
    {
    case 0:
        //   printf("未知:%u\r\n", max_val);
        break;
    case 1:
        if (max_val > th[idx_val])
            printf("机械臂归零:%u\r\n", max_val);
        break;
    case 2:
        if (max_val > th[idx_val])
            printf("机械臂前进:%u\r\n", max_val);
        break;
    case 3:
        if (max_val > th[idx_val])
            printf("机械臂后退:%u\r\n", max_val);
        break;
    case 4:
        if (max_val > th[idx_val])
            printf("机械臂左转:%u\r\n", max_val);
        break;
    case 5:
        if (max_val > th[idx_val])
            printf("机械臂右转:%u\r\n", max_val);
        break;
    case 6:
        if (max_val > th[idx_val])
            printf("机械臂上升:%u\r\n", max_val);
        break;
    case 7:
        if (max_val > th[idx_val])
            printf("机械臂下降:%u\r\n", max_val);
        break;
    case 8:
        if (max_val > th[idx_val])
            printf("机械臂抓取:%u\r\n", max_val);
        break;
    case 9:
        if (max_val > th[idx_val])
            printf("机械臂松开:%u\r\n", max_val);
        break;
    case 10:
        if (max_val > th[idx_val])
            printf("机械臂加速:%u\r\n", max_val);
        break;
    case 11:
        if (max_val > th[idx_val])
            printf("机械臂减速:%u\r\n", max_val);
        break;
    case 12:
        if (max_val > th[idx_val])
            printf("err:%u\r\n", max_val);
        break;
    default:
        printf("Error Key\r\n");
        break;
    }
    if (idx_val && (max_val > th[idx_val]))
    {
        /* code */
        REG32(ARM_RISCV_IPCM_KWS) = idx_val;
        REG8(0x6400200CU) |= 0x8U;
        REG8(0x6400200CU) &= ~0x8U;
        printf("机械臂cmd:%u %u\r\n", idx_val, max_val);
    }
}


int main(void)
{
    // 13.  RV??GPIO2??GPIO3??????????????8??h0x64002008???2??b10
    REG8(0x64002008U) = 0x8U | 0x4U;
    plic_init();
    __asm volatile("csrs mie, %0" :: "r"(0x800));
    __asm volatile("csrs mstatus, 8");
    // f.   ??baud_div??????(addr=0x6400_0018)锟斤拷??????0x364.
    REG16(0x64000018U) = SystemCoreClock / 921600U;
    // g.   ??rx_ctrl??????(addr=0x6400_000C)锟斤拷??????0x1.
    REG8(0x6400000CU) = 0x1U;
    // h.   ??tx_ctrl??????(addr=0x6400_0008)锟斤拷??????0x1.
    REG8(0x64000008U) = 0x1U;
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Hello from NE004-PLUS riscv64 core!\r\n");
    // //TODO heap init
    my_mem_init(SRAMIN);
    /*ringbuffer init*/
    npu_buffer->buffer = (int16_t *)0x60100000;
    // npu_buffer->head_index = 0; //??arm???????
    npu_buffer->tail_index = 0;
    /* hardware FFT init */
    // dma_enable(FFT_DMA_USE);
    // dma_init(FFT_DMA_USE, FFT_DMA_WRITE_CHANNEL, 0x800, 4, 4);
    // dma_init(FFT_DMA_USE, FFT_DMA_READ_CHANNEL, 0x1000, 4, 4);
    // riscv_stop();
    /* agc init */
    ag_set_th(&l_agc_inst, 2, 5, 1);  //target: -9db, gate: -60db, gate_en:1
    agc_set_gain(&l_agc_inst, 0x20, 0x40);  // max_gain:12db, def_gain:0db
    agc_set_time(&l_agc_inst, 16000, 32, 1, 2); //sr:16kHz, frmLen:32ms, attack_frm_num:1, decay_frm_num:2
    /* vad init */
    WebRtcVad_InitCore(&l_vad_inst);
    WebRtcVad_set_mode_core(&l_vad_inst, 0);
    /* kws init */
    TKWS kws;
    Tensor tinput;
    Tensor toutput;
    CreateTensorFromData(&tinput, mfcc_feature, FEATURE_BLOCK, FEATURE_NUM, 1);
    CreateTensorFromData(&toutput, output, COMMAND_NUM, 1, 1);
    LayerInit(&kws);
#define UART_DATAx
#ifdef UART_DATA
    int16_t pcmdata[FRM_SHIFT];
    int8_t *pcm_ptr = pcmdata;
    int8_t header[8] = {'a', 'a', '5', '5', 'l', '2', '5', '6'};
    while (1)
    {
        if (ring_buffer_num_items(npu_buffer) >= FRM_SHIFT)
        {
            // asm volatile("csrr %0, mcycle" : "=r"(start_cycles));
            ring_buffer_dequeue_arr(npu_buffer, pcmdata, FRM_SHIFT);
            for (int i = 0; i < FRM_SHIFT; i++)
            {
                pcmdata[i] = agc_process(&l_agc_inst, pcmdata[i]);
            }
            for (int i = 0; i < 8; i++)
            {
                __io_putchar(header[i]);
            }
            for (int i = 0; i < FRM_SHIFT * 2; i++)
            {
                __io_putchar(*pcm_ptr++);
            }
            pcm_ptr = pcmdata;
        }
    }
#else
    printf("kws start\r\n");
    uint64_t FrmIdx = 0;
    while (1)
    {
        // uint64_t volatile mfcc_start_cycles;
        // uint64_t volatile mfcc_end_cycles;
        // uint64_t volatile mfcc_nop_cycles;
        // uint64_t volatile kws_start_cycles;
        // uint64_t volatile kws_end_cycles;
        // uint64_t volatile kws_nop_cycles;
        int16_t vad_flag = 0;
        uint32_t num_speech = 0;
        uint32_t num_silence = 0;
        uint32_t kws_state = 0;
        if (ring_buffer_num_items(npu_buffer) >= FRM_SHIFT)
        {
            ring_buffer_dequeue_arr(npu_buffer, frame_s16, FRM_SHIFT);
            // //AGC
            // for(int i = 0; i < FRM_SHIFT; i++)
            // {
            //     inbuf[i] = agc_process(&l_agc_inst, frame_s16[i]);
            // }
            // // VAD
            // vad_flag = WebRtcVad_CalcVad16khz(&l_vad_inst, inbuf, FRM_SHIFT);
            // if(vad_flag == 0)
            // {
            //     num_speech = 0;
            //     num_silence++;
            // }
            // else
            // {
            //     num_speech++;
            //     num_silence = 0;
            // }
            // MFCC
            int16_t tmp16;
            float tmp;
            for (int i = 0; i < FRM_SHIFT; i++)
            {
                tmp16 = frame_s16[i];
                tmp = (float)tmp16 / 32767.0f;
                fft_block[i + FRM_SHIFT] = tmp * 2.0;
            }
            // asm volatile("csrr %0, mcycle" : "=r"(mfcc_start_cycles));
            get_mfcc_feature(fft_block, feat_block);
            for (int i = 0; i < FRM_SHIFT; i++)
            {
                tmp16 = frame_s16[i];
                tmp = (float)tmp16 / 32767.0f;
                fft_block[i] = tmp * 2.0;
            }
            // asm volatile("csrr %0, mcycle" : "=r"(mfcc_end_cycles));
            // mfcc_nop_cycles = mfcc_end_cycles - mfcc_start_cycles;
            // printf("mfcc time taken = %ldus ", mfcc_nop_cycles/400);
            memcpy(mfcc_feature, mfcc_feature + FEATURE_NUM, sizeof(float) * (FEATURE_BLOCK - 1) * FEATURE_NUM);
            memcpy(mfcc_feature + (FEATURE_BLOCK - 1) * FEATURE_NUM, feat_block, sizeof(float) * FEATURE_NUM);
            // KWS
            // if(num_speech == 30)
            // {
            //     kws_state = 1;
            //     printf("kws...\r\n");
            // }
            // if(num_silence == 20)
            // {
            //     kws_state = 0;
            //     printf("sil...\r\n");
            // }
            // if(kws_state == 1)
            {
                // asm volatile("csrr %0, mcycle" : "=r"(kws_start_cycles));
                if (FrmIdx % 2 == 0)
                    LayerForward(&tinput, &toutput, &kws);
                // asm volatile("csrr %0, mcycle" : "=r"(kws_end_cycles));
                // kws_nop_cycles = kws_end_cycles - kws_start_cycles;
                // printf("kws time taken = %ldus\r\n", kws_nop_cycles/400);
                show_res(&toutput);
            }
            FrmIdx++;
        }
    }
#endif
    while (1);
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
            // 14.  RV??GPIO3????????????arm???fft??????
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
