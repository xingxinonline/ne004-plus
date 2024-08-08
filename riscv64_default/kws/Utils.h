#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>

#define GLOBAL_PRECISION        15
#define GLOBAL_PRECISION_BITS   ((uint32_t)16384)

int8_t  ClampQInt8(int32_t input);
uint8_t ClampUInt8(int32_t input);
int32_t RoundRShift(int32_t x, int32_t bits, int32_t bit);
int32_t FloatRound(float x);

#endif
