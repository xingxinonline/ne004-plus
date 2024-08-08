#include "LayerCNN.h"
#include "Utils.h"

void GeneralCNNForward3X3(Tensor* input, Tensor* output, TLayerCNN* cnn) {
  int16_t i, j, k, m, n;
  int32_t conv_out;
  int32_t in_row, in_col, in_row_1, in_col_1;
  int16_t cout       = cnn->weight_size_[0];
  int16_t ih         = input->data_shape_[0];
  int16_t iw         = input->data_shape_[1];
  int16_t padding_h  = cnn->padding_size_[0];
  int16_t padding_w  = cnn->padding_size_[1];
  int16_t dilation_h = 1;  // 暂不支持空洞卷积
  int16_t dilation_w = 1;  // 暂不支持空洞卷积
  int16_t kernel_h   = cnn->weight_size_[1];
  int16_t kernel_w   = cnn->weight_size_[2];
  int16_t stride_h   = cnn->stride_size_[0];
  int16_t stride_w   = cnn->stride_size_[1];

  int16_t hout = (ih + 2 * padding_h - dilation_h * (kernel_h - 1) - 1) / stride_h + 1;
  int16_t wout = (iw + 2 * padding_w - dilation_w * (kernel_w - 1) - 1) / stride_w + 1;

  SetTensorHWC(output, hout, wout, cout);

  uint8_t *data_in = (uint8_t *)input->data_;
  uint8_t *data_out = (uint8_t *)output->data_;
  int8_t const *weight = 0;
  int8_t w00, w01, w02, w10, w11, w12, w20, w21, w22;
  int32_t data, in_zero;
  in_zero = cnn->in_zero_point_;

  for (i = 0; i < cout; i++) {  // C
    weight = cnn->weights_ + i*9;
    w00 = *weight++;
    w01 = *weight++;
    w02 = *weight++;
    w10 = *weight++;
    w11 = *weight++;
    w12 = *weight++;
    w20 = *weight++;
    w21 = *weight++;
    w22 = *weight++;
    for (k = 0; k < hout; k++) {    // H
      in_row = stride_h * k - padding_h;
      for (j = 0; j < wout; j++) {  // W
        conv_out = cnn->bias_[i];
        in_col = stride_w * j - padding_w;

        if ((in_row>=0)&&(in_col>=0)&&((in_row + 3 - 1)<ih)&&((in_col + 3 - 1)<iw))
        {
          data = (int32_t)data_in[in_row * iw + in_col];
          data = data - in_zero;
          conv_out += data * w00;
          data = (int32_t)data_in[in_row * iw + in_col + 1];
          data = data - in_zero;
          conv_out += data * w01;
          data = (int32_t)data_in[in_row * iw + in_col + 2];
          data = data - in_zero;
          conv_out += data * w02;
          data = (int32_t)data_in[(in_row + 1) * iw + in_col];
          data = data - in_zero;
          conv_out += data * w10;
          data = (int32_t)data_in[(in_row + 1) * iw + in_col + 1];
          data = data - in_zero;
          conv_out += data * w11;
          data = (int32_t)data_in[(in_row + 1) * iw + in_col + 2];
          data = data - in_zero;
          conv_out += data * w12;
          data = (int32_t)data_in[(in_row + 2) * iw + in_col];
          data = data - in_zero;
          conv_out += data * w20;
          data = (int32_t)data_in[(in_row + 2) * iw + in_col + 1];
          data = data - in_zero;
          conv_out += data * w21;
          data = (int32_t)data_in[(in_row + 2) * iw + in_col + 2];
          data = data - in_zero;
          conv_out += data * w22;
        }
        else
        {
          weight = cnn->weights_ + i*9;
          for (n = 0; n < 3; n++) {    // kernel H
            in_row_1 = in_row + n;
            for (m = 0; m < 3; m++) {  // kernel W
              in_col_1 = in_col + m;

              if (in_row_1 >= 0 && in_col_1 >= 0 && in_row_1 < ih && in_col_1 < iw) {
                data = data_in[in_row_1 * iw + in_col_1] - in_zero;
                conv_out += data * weight[n*3 + m];
              }
            }
          }
        }
        data_out[i + (k * wout + j) * cout] =
          ClampUInt8(RoundRShift(conv_out * cnn->M_[k], GLOBAL_PRECISION_BITS, GLOBAL_PRECISION));
//          ClampUInt8(RoundRShift(conv_out * cnn->M_[k], mbits, mq));
//            ClampUInt8(FloatRound(conv_out * cnn->MF_[i]));
      }
    }
  }
}


int8_t fast_tensor_buf[75*12*1] = {0};
//int8_t fast_tensor_buf[125*12*1] = {0};
void GeneralCNNForward3X3Fast(Tensor* input, Tensor* output, TLayerCNN* cnn) {
  int16_t i, j, k;
  int32_t conv_out;
  int16_t cout       = cnn->weight_size_[0];
  int16_t ih         = input->data_shape_[0];
  int16_t iw         = input->data_shape_[1];
  int16_t padding_h  = cnn->padding_size_[0];
  int16_t padding_w  = cnn->padding_size_[1];
  int16_t dilation_h = 1;  // 暂不支持空洞卷积
  int16_t dilation_w = 1;  // 暂不支持空洞卷积
  int16_t kernel_h   = cnn->weight_size_[1];
  int16_t kernel_w   = cnn->weight_size_[2];
  int16_t stride_h   = cnn->stride_size_[0];
  int16_t stride_w   = cnn->stride_size_[1];
//  int16_t mq         = cnn->MQ_;
//  int32_t mbits      = cnn->MBits_;

  int16_t hout = (ih + 2 * padding_h - dilation_h * (kernel_h - 1) - 1) / stride_h + 1;
  int16_t wout = (iw + 2 * padding_w - dilation_w * (kernel_w - 1) - 1) / stride_w + 1;

  SetTensorHWC(output, hout, wout, cout);

  uint8_t *data_in = (uint8_t *)input->data_;
  for(i = 1; i < 75; i++) {
//  for(i = 1; i < 125; i++) {
    for(j = 1; j < 11; j++) {
      fast_tensor_buf[i*12 + j] = *data_in++ - 94;
    }
  }

  uint8_t *data_out = (uint8_t *)output->data_;
  int8_t const *weight = cnn->weights_;
  int8_t x00, x01, x02, x10, x11, x12, x20, x21, x22;

  for(i = 0; i < hout; i++) {
    for(j = 0; j < wout; j++) {
      x00 = fast_tensor_buf[3*i*12 + 3*j];
      x01 = fast_tensor_buf[3*i*12 + 3*j + 1];
      x02 = fast_tensor_buf[3*i*12 + 3*j + 2];
      x10 = fast_tensor_buf[3*i*12 + 3*j + 12];
      x11 = fast_tensor_buf[3*i*12 + 3*j + 12 + 1];
      x12 = fast_tensor_buf[3*i*12 + 3*j + 12 + 2];
      x20 = fast_tensor_buf[3*i*12 + 3*j + 24];
      x21 = fast_tensor_buf[3*i*12 + 3*j + 24 + 1];
      x22 = fast_tensor_buf[3*i*12 + 3*j + 24 + 2];
      weight = cnn->weights_;
      for(k = 0; k < cout; k++) {
        conv_out = cnn->bias_[k];
        conv_out += x00 * (*weight++);
        conv_out += x01 * (*weight++);
        conv_out += x02 * (*weight++);
        conv_out += x10 * (*weight++);
        conv_out += x11 * (*weight++);
        conv_out += x12 * (*weight++);
        conv_out += x20 * (*weight++);
        conv_out += x21 * (*weight++);
        conv_out += x22 * (*weight++);
        data_out[(i*wout+j)*cout+k] =
          ClampUInt8(RoundRShift(conv_out * cnn->M_[k], GLOBAL_PRECISION_BITS, GLOBAL_PRECISION));
//          ClampUInt8(RoundRShift(conv_out * cnn->M_[k], mbits, mq));
//        ClampUInt8(FloatRound(conv_out * cnn->MF_[k]));
      }
    }
  }
}

void PointCNNForward(Tensor* input, Tensor* output, TLayerCNN* cnn) {
  int16_t i, j, k, l;
  int32_t conv_out;
  int16_t cout       = cnn->weight_size_[0];
  int16_t ih         = input->data_shape_[0];
  int16_t iw         = input->data_shape_[1];
  int16_t ic         = input->data_shape_[2];
//  int16_t mq         = cnn->MQ_;
//  int32_t mbits      = cnn->MBits_;

//  printf("PointCNN\n");
//  printf("in@h:%d, w:%d, c:%d\n", ih, iw, ic);
//  printf("out@h:%d, w:%d, c:%d\n", ih, iw, cout);

  SetTensorHWC(output, ih, iw, cout);

  uint8_t *data_ptr = 0;
  int8_t const *weight_ptr = 0;

  uint8_t *data_in = (uint8_t *)input->data_;
  uint8_t *data_out = (uint8_t *)output->data_;
  for (i = 0; i < cout; i++) {  // C
    for (k = 0; k < ih; k++) {    // H
      for (j = 0; j < iw; j++) {  // W
        conv_out = cnn->bias_[i];
        data_ptr = data_in + ic * (k * iw + j);
        weight_ptr = cnn->weights_ + ic * i;
        for (l = 0; l < ic / 8; l++) {
            conv_out += (*data_ptr++) * (*weight_ptr++);
            conv_out += (*data_ptr++) * (*weight_ptr++);
            conv_out += (*data_ptr++) * (*weight_ptr++);
            conv_out += (*data_ptr++) * (*weight_ptr++);
            conv_out += (*data_ptr++) * (*weight_ptr++);
            conv_out += (*data_ptr++) * (*weight_ptr++);
            conv_out += (*data_ptr++) * (*weight_ptr++);
            conv_out += (*data_ptr++) * (*weight_ptr++);
        }
        data_out[i + (k * iw + j) * cout] =
          ClampUInt8(RoundRShift(conv_out * cnn->M_[i], GLOBAL_PRECISION_BITS, GLOBAL_PRECISION));
//          ClampUInt8(RoundRShift(conv_out * cnn->M_[i], mbits, mq));
//          ClampUInt8(FloatRound(conv_out * cnn->MF_[i]));
      }
    }
  }
}
int32_t pointbuf[13*4] = {0}; //ih*iw
//int32_t pointbuf[21*4] = {0}; //ih*iw
void PointCNNForwardFast(Tensor* input, Tensor* output, TLayerCNN* cnn) {
  int16_t i, j, k, m;
  int32_t conv_out;
  int16_t cout       = cnn->weight_size_[0];
  int16_t ih         = input->data_shape_[0];
  int16_t iw         = input->data_shape_[1];
  int16_t ic         = input->data_shape_[2];
//  int16_t mq         = cnn->MQ_;
//  int32_t mbits      = cnn->MBits_;

//  printf("PointCNN\n");
//  printf("in@h:%d, w:%d, c:%d\n", ih, iw, ic);
//  printf("out@h:%d, w:%d, c:%d\n", ih, iw, cout);

  SetTensorHWC(output, ih, iw, cout);

  uint8_t *data_ptr = 0;

  uint8_t *data_in = (uint8_t *)input->data_;
  uint8_t *data_out = (uint8_t *)output->data_;
  int8_t const *weight_ptr = cnn->weights_;
  int8_t w0, w1, w2, w3, w4, w5, w6, w7;

  for (i = 0; i < cout; i++) {  // C
    conv_out = cnn->bias_[i];
    for(m = 0; m < ih*iw; m++) {
      pointbuf[m] = conv_out;
    }
    for (m = 0; m < 8; m++) {
      w0 = *weight_ptr++;
      w1 = *weight_ptr++;
      w2 = *weight_ptr++;
      w3 = *weight_ptr++;
      w4 = *weight_ptr++;
      w5 = *weight_ptr++;
      w6 = *weight_ptr++;
      w7 = *weight_ptr++;
      for (k = 0; k < ih; k++) {    // H
        for (j = 0; j < iw; j++) {  // W
          data_ptr = data_in + ic * (k * iw + j) + 8*m;
          conv_out = pointbuf[k*iw+j];
          conv_out += (*data_ptr++) * w0;
          conv_out += (*data_ptr++) * w1;
          conv_out += (*data_ptr++) * w2;
          conv_out += (*data_ptr++) * w3;
          conv_out += (*data_ptr++) * w4;
          conv_out += (*data_ptr++) * w5;
          conv_out += (*data_ptr++) * w6;
          conv_out += (*data_ptr++) * w7;
          pointbuf[k*iw+j] = conv_out;
        }
      }
    }

    for (k = 0; k < ih; k++) {    // H
      for (j = 0; j < iw; j++) {  // W
        conv_out = pointbuf[k*iw+j];
        data_out[i + (k * iw + j) * cout] =
          ClampUInt8(RoundRShift(conv_out * cnn->M_[i], GLOBAL_PRECISION_BITS, GLOBAL_PRECISION));
//          ClampUInt8(RoundRShift(conv_out * cnn->M_[i], mbits, mq));
//          ClampUInt8(FloatRound(conv_out * cnn->MF_[i]));
      }
    }
  }
}

void DSCNNForward3X3(Tensor* input, Tensor* output, TLayerCNN* cnn) {
  int16_t i, j, k, m, n;
  int32_t conv_out;
  int32_t in_row, in_col, in_row_1, in_col_1;
  int16_t cout       = cnn->weight_size_[0];
  int16_t ih         = input->data_shape_[0];
  int16_t iw         = input->data_shape_[1];
  int16_t padding_h  = cnn->padding_size_[0];
  int16_t padding_w  = cnn->padding_size_[1];
  int16_t dilation_h = 1;  // 暂不支持空洞卷积
  int16_t dilation_w = 1;  // 暂不支持空洞卷积
  int16_t kernel_h   = cnn->weight_size_[1];
  int16_t kernel_w   = cnn->weight_size_[2];
  int16_t stride_h   = cnn->stride_size_[0];
  int16_t stride_w   = cnn->stride_size_[1];
//  int16_t mq         = cnn->MQ_;
//  int32_t mbits      = cnn->MBits_;

  int16_t hout = (ih + 2 * padding_h - dilation_h * (kernel_h - 1) - 1) / stride_h + 1;
  int16_t wout = (iw + 2 * padding_w - dilation_w * (kernel_w - 1) - 1) / stride_w + 1;

  SetTensorHWC(output, hout, wout, cout);

//  printf("DSCNN\n");
//  printf("in@h:%d, w:%d, c:%d\n", ih, iw, input->data_shape_[2]);
//  printf("out@h:%d, w:%d, c:%d\n", hout, wout, cout);

  uint8_t *data_in = (uint8_t *)input->data_;
  uint8_t *data_out = (uint8_t *)output->data_;
  int8_t const *weight = 0;
  int8_t w00, w01, w02, w10, w11, w12, w20, w21, w22;
  int32_t data;
  for (i = 0; i < cout; i++) {      // C
    weight = cnn->weights_ + i*9;
    w00 = *weight++;
    w01 = *weight++;
    w02 = *weight++;
    w10 = *weight++;
    w11 = *weight++;
    w12 = *weight++;
    w20 = *weight++;
    w21 = *weight++;
    w22 = *weight++;
    for (k = 0; k < hout; k++) {    // H
      in_row = stride_h * k - padding_h;
      for (j = 0; j < wout; j++) {  // W
        conv_out = cnn->bias_[i];
        in_col = stride_w * j - padding_w;

        if ((in_row>=0)&&(in_col>=0)&&((in_row + 3 - 1)<ih)&&((in_col + 3 - 1)<iw))
        {
          data = (int32_t)data_in[(in_row * iw + in_col)*64 + i];
          conv_out += data * w00;
          data = (int32_t)data_in[(in_row * iw + in_col + 1)*64 + i];
          conv_out += data * w01;
          data = (int32_t)data_in[(in_row * iw + in_col + 2)*64 + i];
          conv_out += data * w02;
          data = (int32_t)data_in[((in_row + 1) * iw + in_col)*64 + i];
          conv_out += data * w10;
          data = (int32_t)data_in[((in_row + 1) * iw + in_col + 1)*64 + i];
          conv_out += data * w11;
          data = (int32_t)data_in[((in_row + 1) * iw + in_col + 2)*64 + i];
          conv_out += data * w12;
          data = (int32_t)data_in[((in_row + 2) * iw + in_col)*64 + i];
          conv_out += data * w20;
          data = (int32_t)data_in[((in_row + 2) * iw + in_col + 1)*64 + i];
          conv_out += data * w21;
          data = (int32_t)data_in[((in_row + 2) * iw + in_col + 2)*64 + i];
          conv_out += data * w22;
        }
        else
        {
          weight = cnn->weights_ + i*9;
          for (n = 0; n < 3; n++) {    // kernel H
            in_row_1 = in_row + n;
            for (m = 0; m < 3; m++) {  // kernel W
              in_col_1 = in_col + m;
              if (in_row_1 >= 0 && in_col_1 >= 0 && in_row_1 < ih && in_col_1 < iw) {
                data = data_in[(in_row_1 * iw + in_col_1) * 64 + i];
                conv_out += data * weight[n*3 + m];
              }
            }
          }
        }
        data_out[i + (k * wout + j) * cout] =
            ClampUInt8(RoundRShift(conv_out * cnn->M_[i], GLOBAL_PRECISION_BITS, GLOBAL_PRECISION));
//            ClampUInt8(RoundRShift(conv_out * cnn->M_[i], mbits, mq));
//            ClampUInt8(FloatRound(conv_out * cnn->MF_[i]));
      }
    }
  }
}

/*Normal CNN Layer*/
void GeneralCNNForward(Tensor* input, Tensor* output, TLayerCNN* cnn) {
  int16_t i, j, k, l, m, n;
  int32_t conv_out;
  int32_t in_row, in_col;
  int16_t cout       = cnn->weight_size_[0];
  int16_t ih         = input->data_shape_[0];
  int16_t iw         = input->data_shape_[1];
  int16_t ic         = input->data_shape_[2];
  int16_t padding_h  = cnn->padding_size_[0];
  int16_t padding_w  = cnn->padding_size_[1];
  int16_t dilation_h = 1;  // 暂不支持空洞卷积
  int16_t dilation_w = 1;  // 暂不支持空洞卷积
  int16_t kernel_h   = cnn->weight_size_[1];
  int16_t kernel_w   = cnn->weight_size_[2];
  int16_t stride_h   = cnn->stride_size_[0];
  int16_t stride_w   = cnn->stride_size_[1];

  int16_t hout = (ih + 2 * padding_h - dilation_h * (kernel_h - 1) - 1) / stride_h + 1;
  int16_t wout = (iw + 2 * padding_w - dilation_w * (kernel_w - 1) - 1) / stride_w + 1;

  SetTensorHWC(output, hout, wout, cout);

//  printf("CNN\n");
//  printf("in@h:%d, w:%d, c:%d\n", ih, iw, ic);
//  printf("out@h:%d, w:%d, c:%d\n", hout, wout, cout);

  uint8_t *data_in = (uint8_t *)input->data_;
  uint8_t *data_out = (uint8_t *)output->data_;
  for (i = 0; i < cout; i++) {  // C
    for (k = 0; k < hout; k++) {    // H
      for (j = 0; j < wout; j++) {  // W
        conv_out = cnn->bias_[i];
        for (n = 0; n < kernel_h; n++) {    // kernel H
          for (m = 0; m < kernel_w; m++) {  // kernel W
            // if-for implementation
            in_row = stride_h * k + n - padding_h;
            in_col = stride_w * j + m - padding_w;
            if (in_row >= 0 && in_col >= 0 && in_row < ih && in_col < iw) {
              for (l = 0; l < ic; l++) {
                conv_out +=
                    (data_in[(in_row * iw + in_col) * ic + l] -
                     cnn->in_zero_point_) *
                    (cnn->weights_[i * ic * kernel_h * kernel_w +
                                   (n * kernel_w + m) * ic + l] -
                     0);  // 默认weight 的zero_point 为0
              }
            }
          }
        }

        data_out[i + (k * wout + j) * cout] =
         ClampUInt8(RoundRShift(conv_out * cnn->M_[i], GLOBAL_PRECISION_BITS, GLOBAL_PRECISION) + cnn->out_zero_point_);
//         ClampUInt8(FloatRound(conv_out * cnn->MF_[i]) + cnn->out_zero_point_);
      }
    }
  }
}
/*Depth-Wise CNN Layer*/
void DSCNNForward(Tensor* input, Tensor* output, TLayerCNN* cnn) {
  int16_t i, j, k, m, n;
  int32_t conv_out;
  int32_t in_row, in_col;
  int16_t cout       = cnn->weight_size_[0];
  int16_t ih         = input->data_shape_[0];
  int16_t iw         = input->data_shape_[1];
  int16_t ic         = input->data_shape_[2];
  int16_t padding_h  = cnn->padding_size_[0];
  int16_t padding_w  = cnn->padding_size_[1];
  int16_t dilation_h = 1;  // 暂不支持空洞卷积
  int16_t dilation_w = 1;  // 暂不支持空洞卷积
  int16_t kernel_h   = cnn->weight_size_[1];
  int16_t kernel_w   = cnn->weight_size_[2];
  int16_t stride_h   = cnn->stride_size_[0];
  int16_t stride_w   = cnn->stride_size_[1];

  int16_t hout = (ih + 2 * padding_h - dilation_h * (kernel_h - 1) - 1) / stride_h + 1;
  int16_t wout = (iw + 2 * padding_w - dilation_w * (kernel_w - 1) - 1) / stride_w + 1;

  SetTensorHWC(output, hout, wout, cout);

//  printf("DSCNN\n");
//  printf("in@h:%d, w:%d, c:%d\n", ih, iw, ic);
//  printf("out@h:%d, w:%d, c:%d\n", hout, wout, cout);

  uint8_t *data_in = (uint8_t *)input->data_;
  uint8_t *data_out = (uint8_t *)output->data_;
  for (i = 0; i < cout; i++) {      // C
    for (k = 0; k < hout; k++) {    // H
      for (j = 0; j < wout; j++) {  // W
        conv_out = cnn->bias_[i];
        for (n = 0; n < kernel_h; n++) {    // kernel H
          for (m = 0; m < kernel_w; m++) {  // kernel W
            // if-for implementation
            in_row = stride_h * k + n - padding_h;
            in_col = stride_w * j + m - padding_w;
            if (in_row >= 0 && in_col >= 0 && in_row < ih && in_col < iw) {
              conv_out +=
                  (data_in[(in_row * iw + in_col) * ic + i] -
                   cnn->in_zero_point_) *
                  (cnn->weights_[i * kernel_h * kernel_w + n * kernel_w + m] -
                   0);  // 默认weight 的zero_point 为0
            }
          }
        }
        data_out[i + (k * wout + j) * cout] =
//            ClampUInt8(
//            (round((conv_out * cnn->M_[i] * 1.0) / (1 << GLOBAL_PRECISION))) +
//            cnn->out_zero_point_);

         ClampUInt8(RoundRShift(conv_out * cnn->M_[i], GLOBAL_PRECISION_BITS, GLOBAL_PRECISION) + cnn->out_zero_point_);
//         ClampUInt8(FloatRound(conv_out * cnn->MF_[i]) + cnn->out_zero_point_);
         //ClampUInt8((round(conv_out * cnn->MF_[i])) + cnn->out_zero_point_);
        // // 验证使用
      }
    }
  }
}

void CNNForward(Tensor* input, Tensor* output, TLayerCNN* cnn) {
  if (cnn->groups_ == 1) {  // General
    GeneralCNNForward(input, output, cnn);
  } else {  // DSCNN
    DSCNNForward(input, output, cnn);
  }
}
