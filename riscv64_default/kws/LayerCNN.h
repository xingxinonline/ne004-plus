#ifndef __CNNLAYER_H__
#define __CNNLAYER_H__
#include <stdint.h>

#include "Tensor.h"

typedef struct {
  const int16_t* M_;
//  int16_t  MQ_;
//  int32_t  MBits_;
//  const float*   MF_;
  int16_t        groups_;
  const int16_t* weight_size_;
  const int16_t* kernel_size_;
  const int16_t* stride_size_;
  const int16_t* padding_size_;
  const int32_t* bias_;
  const int8_t*  weights_;
  uint8_t        out_zero_point_;
  uint8_t        in_zero_point_;
} TLayerCNN;

void GeneralCNNForward(Tensor* input, Tensor* output, TLayerCNN* cnn);
void DSCNNForward(Tensor* input, Tensor* output, TLayerCNN* cnn);

void GeneralCNNForward3X3(Tensor* input, Tensor* output, TLayerCNN* cnn);
void GeneralCNNForward3X3Fast(Tensor* input, Tensor* output, TLayerCNN* cnn);
void PointCNNForward(Tensor* input, Tensor* output, TLayerCNN* cnn);
void PointCNNForwardFast(Tensor* input, Tensor* output, TLayerCNN* cnn);
void DSCNNForward3X3(Tensor* input, Tensor* output, TLayerCNN* cnn);

void CNNForward(Tensor* input, Tensor* output, TLayerCNN* cnn);

#endif
