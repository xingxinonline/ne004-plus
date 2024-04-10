#ifndef __MFCC_H__
#define __MFCC_H__
#include "mfcc_mel.h"
#include "mfcc_type.h"

/*mfcc初始化*/
TMFCC* MfccInit();
/*mfcc特征提取*/
void MfccStart(TMFCC* mfcc, int16_t* wave, uint32_t wav_len, BaseFloat** output,
               int16_t* row_out, int16_t* col_out);
/*mfcc销毁*/
void MfccDestroy(TMFCC* mfcc);
#endif