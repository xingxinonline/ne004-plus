#ifndef __MFCC_TOOLS_H__
#define __MFCC_TOOLS_H__
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "mfcc_type.h"
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) < (Y)) ? (Y) : (X))

/*FFT radix stages 计算*/
int16_t GetStage(int32_t frame_len);

/*向量求和*/
int32_t Sum_Int16(int16_t* input, int16_t input_len);

/*向量点加*/
void Add_Int16(int16_t* input, int16_t input_len, int32_t val2add);

/*内积 res += a[i] * b[i] make sure length is the same*/
BaseFloat InnerProduct_BF(BaseFloat* input_01, BaseFloat* input_02, int32_t length);
int64_t   InnerProduct_Int64(int64_t* input_01, int64_t* input_02, int32_t length);
int64_t   InnerProduct_Int64_ShiftRight(int64_t* input_01, int64_t* input_02, int32_t data_length, int32_t shift_length);
int32_t   InnerProduct_Int32_ShiftRight(uint16_t* input_01, int32_t* input_02, int32_t data_length, int32_t shift_length);

/*乘法 dest[i] *= from[i]*/
void Multiply_Int16_ShiftRight(int16_t* dest, uint16_t* from, int16_t data_length, int16_t shift_length);

/*找到最接近的2的指数*/
int32_t PaddedWindowSize(int32_t n);

/*计算DCT映射矩阵*/
void ComputeDct_Int32(int32_t* input, int32_t input_row, int32_t input_col);
void ComputeDct_Int16(int16_t* input, int32_t input_row, int32_t input_col);
void ComputeDct_BF(BaseFloat* input, int32_t input_row, int32_t input_col);
/*最小值边界控制*/
void ApplyFloor_Int64(int64_t* vec, int32_t vec_len, int64_t floor_val);
void ApplyFloor_Int32(int32_t* vec, int32_t vec_len, int32_t floor_val);
void ApplyFloor_BF(BaseFloat* vec, int32_t vec_len, BaseFloat floor_val);
/*log(x)*/
void ApplyLog_Int64(int64_t* vec, int32_t vec_len);
void ApplyLog_Int32(int32_t* vec, int32_t vec_len);
void ApplyLog_BF(BaseFloat* vec, int32_t vec_len);
void ApplyFloorLog_BF(int32_t* vec, int32_t vec_len, BaseFloat floor_val);

/*BaseFloat类型对比*/
inline bool CheckEqualBF(BaseFloat a, BaseFloat b) {
  if (fabs(a - b) < PX_EPSILON) {
    return true;
  }
  return false;
}

/*逆mel映射*/
inline BaseFloat InverseMelScale(BaseFloat mel_freq) {
  return 700.0f * (expf(mel_freq / 1127.0f) - 1.0f);
}

/*mel映射*/
inline BaseFloat MelScale(BaseFloat freq) {
  return 1127.0f * logf(1.0f + freq / 700.0f);
}

/*读文件*/
#ifdef MAC
void ReadInputs(const char* input_file, int16_t* output);
#endif

/*溢出性检查*/
bool CheckOverFlowADD(int32_t addend_01, int32_t addend_02);
bool CheckOverFlowMul(int64_t addend_01, int64_t addend_02);

/*大于此数字的下个2的幂次数*/
int16_t nextpow2(int64_t nums);
#endif