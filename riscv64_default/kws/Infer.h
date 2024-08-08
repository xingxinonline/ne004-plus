#ifndef __INFER_H__
#define __INFER_H__
#include "LayerAvePool.h"
#include "LayerCNN.h"
#include "LayerLinear.h"
#include "LayerQuant.h"
#include "Tensor.h"

typedef struct {
  TLayerQuant       quant_;
  TLayerCNN         conv_;
  TLayerCNN         dwcnn_01_0_;
  TLayerCNN         dwcnn_01_3_;
  TLayerCNN         dwcnn_02_0_;
  TLayerCNN         dwcnn_02_3_;
  TLayerCNN         dwcnn_03_0_;
  TLayerCNN         dwcnn_03_3_;
  TLayerCNN         dwcnn_04_0_;
  TLayerCNN         dwcnn_04_3_;
  TLayerCNN         dwcnn_05_0_;
  TLayerCNN         dwcnn_05_3_;
  TLayerGlobAvePool glob_ave_pool_;
  TLayerLinear      linear_;
} TKWS;

/* Init Layer Parameters Like: Weights,Bias ... */
void LayerInit(TKWS* kws);
/* Given Input Compute Each Layer */
void LayerForward(Tensor* input, Tensor* output, TKWS* kws);

#endif
