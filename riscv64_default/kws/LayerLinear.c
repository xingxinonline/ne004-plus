#include "LayerLinear.h"
#include "Utils.h"

/*NN Forward*/
void LinearForward(Tensor* input, Tensor* output, TLayerLinear* linear) {
  int16_t ih    = input->data_shape_[2];  // D
  int16_t oh    = linear->weight_size_[0];
//  int16_t mq    = linear->MQ_;
//  int32_t mbits = linear->MBits_;

  SetTensorHWC(output, oh, 1, 1);

  uint8_t *data_in = (uint8_t *)input->data_;
  uint8_t *data_out = (uint8_t *)output->data_;
  for (int16_t i = 0; i < oh; i++) {
    int32_t conv_out = linear->bias_[i];
    for (int16_t j = 0; j < ih; j++) {
      conv_out += data_in[j] * linear->weights_[i * ih + j];
    }

    data_out[i] =
       ClampUInt8(RoundRShift(conv_out * linear->M_[i], GLOBAL_PRECISION_BITS, GLOBAL_PRECISION) + linear->output_zero_point_);
//       ClampUInt8(RoundRShift(conv_out * linear->M_[i], mbits, mq) + linear->output_zero_point_);
//       ClampUInt8(FloatRound(conv_out * linear->MF_[i] + linear->output_zero_point_));
  }
}
