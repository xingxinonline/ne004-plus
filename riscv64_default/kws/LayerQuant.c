#include "LayerQuant.h"
#include "Utils.h"

void QuantForward(Tensor* input, Tensor* output, TLayerQuant* quant) {
  float*  finputs = (float*)(input->data_);
  int16_t h       = input->data_shape_[0];
  int16_t w       = input->data_shape_[1];
  int16_t c       = input->data_shape_[2];

  SetTensorHWC(output, h, w, c);

  uint8_t* uoutput = (uint8_t*)(output->data_);

  float inscales = 1.0 / quant->scales_;
  for (int32_t i = 0; i < input->data_len_; ++i) {
    uoutput[i] = ClampUInt8(FloatRound(finputs[i] * inscales + quant->zero_point_));
  }
}
