#ifndef __QUANTLAYER_H__
#define __QUANTLAYER_H__
#include <stdint.h>

#include "Tensor.h"

typedef struct {
  float   scales_;
  uint8_t zero_point_;
} TLayerQuant;

void QuantForward(Tensor* input, Tensor* output, TLayerQuant* quant);
#endif
