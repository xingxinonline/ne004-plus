#ifndef __MFCC_WIN_H__
#define __MFCC_WIN_H__
#include "mfcc.h"

/*窗移长度*/
inline int32_t WindowShift(TMFCCOPT* opts) {
  return (int32_t)(opts->samp_freq * 0.001f * opts->frame_shift_ms);
}

/*窗口长度*/
inline int32_t WindowSize(TMFCCOPT* opts) {
  return (int32_t)(opts->samp_freq * 0.001f * opts->frame_length_ms);
}

/*创建窗函数*/
void MfccWindowInit(TMFCC* mfcc);

/*分帧加窗预加重*/
void ExtractWindow(int16_t* wave,
                   uint32_t wave_len,
                   int16_t  f,
                   TMFCC*   mfcc);

/*获取帧数*/
int16_t NumFrames(int32_t num_samples, TMFCC* mfcc);
#endif