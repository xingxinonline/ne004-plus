/*
 * @Author       : panxinhao
 * @Date         : 2023-08-10 10:09:42
 * @LastEditors  : panxinhao
 * @LastEditTime : 2023-08-16 11:15:37
 * @FilePath     : \\testbench_base\\riscv64_default\\mfcc\\mfcc_core.c
 * @Description  : 
 * 
 * Copyright (c) 2023 by xinhao.pan@pimchip.cn, All Rights Reserved. 
 */

#include <stdbool.h>
#include <stdint.h>
#ifdef MAC
    #include <stddef.h>
    #include <stdlib.h>
    #include <string.h>
#elif RISCV
    #include "malloc.h"
#elif ARM_MATH_DSP
    #include <stddef.h>
    #include <stdlib.h>
    #include <string.h>
#endif
#include <assert.h>
#include <stdio.h>

#include "mfcc_core.h"
// #include "mfcc_fft.h"
#include "mfcc_tools.h"

// int16_t STAGE = 0;
int16_t FRAME_LEN = 0;

// extern int64_t* wn_sin;
// extern int64_t* wn_cos;
// extern int32_t* fft_re;
// extern int32_t* fft_im;
// extern int32_t* wavform;
// extern int32_t* signal_frame_re;
// extern int32_t* signal_frame_im;

void ComputePowerSpectrum(int32_t *fft_frame, int32_t frame_len,
                          int32_t *waveform)
{
    waveform[0] = fft_frame[0] * fft_frame[0];
    for (int16_t i = 1; i < frame_len / 2; i++)
    {
        waveform[i] = fft_frame[2 * i] * fft_frame[2 * i] +
                      fft_frame[2 * i + 1] * fft_frame[2 * i + 1];
    }
    waveform[frame_len / 2] = fft_frame[1] * fft_frame[1];
}

void ComputeMfccCore(TMFCC *mfcc, BaseFloat *feature)
{
    int16_t *signal_frame = mfcc->window;
    extern void fft_hardware(int32_t *src_addr);
    // printf("frame_len:%d\n", FRAME_LEN);
    for (int16_t i = 0; i < FRAME_LEN; i++)
    {
        /* code */
        mfcc->fft_out[i] = (int32_t)signal_frame[i];
    }
    fft_hardware(mfcc->fft_out);
//   for (int16_t i = 0; i < FRAME_LEN; i++) {
//     signal_frame_re[i] = signal_frame[i];
//     signal_frame_im[i] = 0;
//   }
//   int16_t start_param = 0;
//   FFTFixPoint(signal_frame_re, signal_frame_im, wn_sin, wn_cos, start_param, 0,
//               FRAME_LEN, 0, fft_re, fft_im);
//   for (int16_t i = 0; i < FRAME_LEN / 2; ++i) {
//     mfcc->fft_out[2 * i] = fft_re[i];
//     mfcc->fft_out[2 * i + 1] = fft_im[i];
//   }
    // 功率谱
    ComputePowerSpectrum(mfcc->fft_out, FRAME_LEN, mfcc->wavform);
    MelBankCompute(mfcc, mfcc->wavform, mfcc->mel_energies, &(mfcc->mel_len));
    ApplyFloor_Int32(mfcc->mel_energies, mfcc->mel_len, 1);
    ApplyLog_Int32(mfcc->mel_energies, mfcc->mel_len);
#ifdef MAC
    memset(feature, 0, sizeof(BaseFloat) * mfcc->dct_row_len);
#elif RISCV
    // TOFIX
    mymemset(feature, 0, sizeof(BaseFloat) * mfcc->dct_row_len);
#elif ARM_MATH_DSP
    memset(feature, 0, sizeof(BaseFloat) * mfcc->dct_row_len);
#endif
    for (int32_t i = 0; i < mfcc->dct_row_len; ++i)    // 问题出在这里
    {
        int32_t row_sum = 0;
        for (int32_t j = 0; j < mfcc->dct_col_len; ++j)
        {
            row_sum +=
                mfcc->dct_weights[i * mfcc->dct_col_len + j] * mfcc->mel_energies[j];
        }
        feature[i] = row_sum;
    }
    for (int32_t i = 0; i < mfcc->dct_row_len; ++i)
    {
        feature[i] = feature[i] * mfcc->lifter_coeffs[i] / 32767.0f;
    }
}
