#ifndef __LAYERLINEAR_H__
#define __LAYERLINEAR_H__
#include <stdint.h>

#include "Tensor.h"

typedef struct {
  const int32_t* bias_;
  const int8_t*  weights_;
  const int16_t* weight_size_;
  uint8_t        output_zero_point_;
  const int16_t* M_;
//  int16_t  MQ_;
//  int32_t  MBits_;
//  const float* MF_;
} TLayerLinear;

void LinearForward(Tensor* input, Tensor* output, TLayerLinear* linear);
#endif
