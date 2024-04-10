#ifdef MAC
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#elif RISCV
#include "malloc.h"
#elif ARM_MATH_DSP
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#endif

#include <limits.h>
#include "mfcc_tools.h"

int16_t GetStage(int32_t frame_len) {
  int16_t out = 0, i = 0;
  while (frame_len > (1 << (i++))) {
    ++out;
  }
  return out;
}

int32_t Sum_Int16(int16_t* input, int16_t input_len) {
  int32_t output = 0;
  for (int32_t i = 0; i < input_len; ++i) {
    output += input[i];
  }
  return output;
}

void Add_Int16(int16_t* input, int16_t input_len, int32_t val2add) {
  for (int16_t i = 0; i < input_len; ++i) {
    input[i] += val2add;
  }
}

BaseFloat InnerProduct_BF(BaseFloat* input_01, BaseFloat* input_02, int32_t length) {
  BaseFloat result = 0;
  for (int32_t i = 0; i < length; ++i) {
    result += (input_01[i] * input_02[i]);
  }
  return result;
}

int64_t InnerProduct_Int64(int64_t* input_01, int64_t* input_02, int32_t length) {
  int64_t result = 0;
  for (int32_t i = 0; i < length; ++i) {
    result += (input_01[i] * input_02[i]);
  }
  return result;
}

int64_t InnerProduct_Int64_ShiftRight(int64_t* input_01, int64_t* input_02, int32_t data_length, int32_t shift_length) {
  int64_t result = 0;
  for (int32_t i = 0; i < data_length; ++i) {
    result += (input_01[i] * input_02[i] >> shift_length);
  }
  return result;
}

int32_t InnerProduct_Int32_ShiftRight(uint16_t* input_01, int32_t* input_02, int32_t data_length, int32_t shift_length) {
  int32_t result = 0;
  for (int32_t i = 0; i < data_length; ++i) {
    result += input_01[i] * input_02[i] >> shift_length;
  }

  return result > INT32_MAX ? INT32_MAX : (result < INT32_MIN ? INT32_MIN : (int32_t)result);
}

void Multiply_Int16_ShiftRight(int16_t* dest, uint16_t* from, int16_t data_length, int16_t shift_length) {
  for (int16_t i = 0; i < data_length; ++i) {
    dest[i] = (dest[i] * from[i]) >> shift_length;
  }
}

int32_t PaddedWindowSize(int32_t n) {
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  return n + 1;
}
void ComputeDct_Int32(int32_t* input, int32_t input_row, int32_t input_col) {
  int32_t scales = 1 << BINARY_POINT;
  int32_t normalizer = (int32_t)(sqrt(1.0f / (BaseFloat)input_col) * scales); // 32, 16
  for (int32_t j = 0; j < input_col; j++) { input[j] = (int32_t)normalizer; }
  normalizer = (int32_t)(sqrt(2.0f / (BaseFloat)input_col) * scales); // normalizer for other 32, 16
  for (int32_t k = 1; k < input_row; k++)
    for (int32_t n = 0; n < input_col; n++)
      input[k * input_col + n] = normalizer * cos((double)M_PI / input_col * (n + 0.5f) * k);
}

void ComputeDct_Int16(int16_t* input, int32_t input_row, int32_t input_col) {
  int32_t   scales = 1 << BINARY_POINT;
  BaseFloat normalizer = sqrt(1.0f / (BaseFloat)input_col); // 32, 16
  for (int32_t j = 0; j < input_col; j++) {
    input[j] = normalizer * scales;
  }
  normalizer = sqrt(2.0f / (BaseFloat)input_col);
  for (int32_t k = 1; k < input_row; k++)
    for (int32_t n = 0; n < input_col; n++) {
      input[k * input_col + n] = normalizer * cos((double)M_PI / input_col * (n + 0.5f) * k) * scales;
    }
}

void ComputeDct_BF(BaseFloat* input, int32_t input_row, int32_t input_col) {
  BaseFloat normalizer = sqrt(1.0f / (BaseFloat)input_col);
  for (int32_t j = 0; j < input_col; j++) { input[j] = normalizer; }
  normalizer = sqrt(2.0f / (BaseFloat)input_col); // normalizer for other
  for (int32_t k = 1; k < input_row; k++)
    for (int32_t n = 0; n < input_col; n++) {
      input[k * input_col + n] = normalizer * cos((double)M_PI / input_col * (n + 0.5f) * k);
    }
}

void ApplyFloor_Int64(int64_t* vec, int32_t vec_len, int64_t floor_val) {
  for (int32_t i = 0; i < vec_len; i++) {
    vec[i] = MAX(vec[i], floor_val);
  }
}

void ApplyFloor_Int32(int32_t* vec, int32_t vec_len, int32_t floor_val) {
  for (int32_t i = 0; i < vec_len; i++) {
    vec[i] = MAX(vec[i], floor_val);
  }
}

void ApplyFloor_BF(BaseFloat* vec, int32_t vec_len, BaseFloat floor_val) {
  for (int32_t i = 0; i < vec_len; i++) {
    vec[i] = MAX(vec[i], floor_val);
  }
}

void ApplyFloorLog_BF(int32_t* vec, int32_t vec_len, BaseFloat floor_val) {
  for (int32_t i = 0; i < vec_len; i++) {
    BaseFloat max_val = MAX((BaseFloat)vec[i] / 16.0f, floor_val);
    vec[i] = (int32_t)logf(max_val);
  }
}

void ApplyLog_Int64(int64_t* vec, int32_t vec_len) {
  for (int32_t i = 0; i < vec_len; i++) {
    vec[i] = (int64_t)logf((BaseFloat)vec[i]);
  }
}

void ApplyLog_Int32(int32_t* vec, int32_t vec_len) {
  for (int32_t i = 0; i < vec_len; i++) {
    vec[i] = (int32_t)(round(logf((BaseFloat)vec[i]) - 8.0f));
  }
}

void ApplyLog_BF(BaseFloat* vec, int32_t vec_len) {
  for (int32_t i = 0; i < vec_len; i++) {
    vec[i] = logf(vec[i]);
  }
}

#ifdef MAC
void SplitInt16(char* input, const char* delimit, int16_t* out) {
  char*    token = strtok(input, delimit);
  uint32_t sidx = 0;
  while (token) {
    out[sidx++] = atoi(token);
    token = strtok(NULL, delimit);
  }
}

void ReadInputs(const char* input_file, int16_t* output) {
  FILE*  file = fopen(input_file, "r");
  size_t len = 0, read = 0;
  char*  line = NULL;
  while ((read = getline(&line, &len, file)) != -1) {
    SplitInt16(line, ",", output);
  }
  free(line);
  fclose(file);
}
#endif

bool CheckOverFlowADD(int32_t addend_01, int32_t addend_02) {
  if (addend_01 > 0 && addend_02 > 0) {
    int64_t c = (int64_t)addend_01 + (int64_t)addend_02;
    if (c < 0) return true;
  }
  return false;
}

bool CheckOverFlowMul(int64_t addend_01, int64_t addend_02) {
  int64_t c = addend_01 * addend_02;
  if (addend_01 != 0 && c / addend_01 != addend_02) {
    return true;
  }

  return false;
}

int16_t nextpow2(int64_t nums) {
  int64_t sums = 1;
  int16_t out = 0;
  if (sums == nums) {
    return out;
  }
  while (sums < nums) {
    sums = sums << 1;
    ++out;
  }
  return out;
}