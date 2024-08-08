#include "LayerAvePool.h"
#include "Utils.h"

void GlobAvePoolForward(Tensor* input, Tensor* output,
                        TLayerGlobAvePool* pool) {
  // input: H,W,C => H,1,1
  const int16_t* output_size = pool->output_size_;
  int16_t        ih          = input->data_shape_[0];
  int16_t        iw          = input->data_shape_[1];
  int16_t        cin         = input->data_shape_[2];
  int16_t        oh          = output_size[0];
  int16_t        ow          = output_size[1];
  int16_t        cout        = cin;

  SetTensorHWC(output, oh, ow, cout);

  int16_t stride_h   = ih / oh;
  int16_t stride_w   = iw / ow;
  int16_t kernel_h   = stride_h;
  int16_t kernel_w   = stride_w;
  int32_t kernel_len = kernel_h * kernel_w;
  int16_t padding_h  = 0;
  int16_t padding_w  = 0;

  uint8_t *data_in = (uint8_t *)input->data_;
  uint8_t *data_out = (uint8_t *)output->data_;
  for (int16_t co = 0; co < cout; ++co) {
    for (int16_t ho = 0; ho < oh; ++ho) {
      for (int16_t wo = 0; wo < ow; ++wo) {
        int32_t conv_out = 0;
        for (int16_t n = 0; n < kernel_h; ++n) {
          for (int16_t m = 0; m < kernel_w; ++m) {
            int32_t in_row = stride_h * ho + n - padding_h;
            int32_t in_col = stride_w * wo + m - padding_w;
            if (in_row >= 0 && in_col >= 0 && in_row < ih && in_col < iw) {
              conv_out +=
                  data_in[(in_row * iw + in_col) * cin + co];
            }
          }
        }
        conv_out = FloatRound((float)conv_out / (float)kernel_len);
        data_out[co + cout * (ho * ow + wo)] = ClampUInt8(conv_out);
      }
    }
  }
}
