#ifndef __LAYERAVEPOOL_H__
#define __LAYERAVEPOOL_H__
#include <stdint.h>

#include "Tensor.h"

typedef struct {
  const int16_t* output_size_;
} TLayerGlobAvePool;

void GlobAvePoolForward(Tensor* input, Tensor* output,
                        TLayerGlobAvePool* pool);
#endif
