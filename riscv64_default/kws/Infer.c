#include "Infer.h"

#include <stdio.h>

#include "LayerWeight.h"

uint8_t tensor_buf_0[13*4*64];
uint8_t tensor_buf_1[25*4*64];
Tensor tensor0 = {tensor_buf_0, 13*4*64, {13,4,64}, 13*4*64};
Tensor tensor1 = {tensor_buf_1, 25*4*64, {25,4,64}, 25*4*64};
//uint8_t tensor_buf_0[21*4*64];
//uint8_t tensor_buf_1[42*4*64];
//Tensor tensor0 = {tensor_buf_0, 21*4*64, {21,4,64}, 21*4*64};
//Tensor tensor1 = {tensor_buf_1, 42*4*64, {42,4,64}, 42*4*64};


void LayerInit(TKWS* kws) {
  // quant
  kws->quant_.scales_     = quant_output_scale;
  kws->quant_.zero_point_ = quant_output_zero_point;
  // conv
  kws->conv_.groups_       = conv_groups;
  kws->conv_.padding_size_ = conv_padding_size;
  kws->conv_.stride_size_  = conv_stride_size;
  kws->conv_.weight_size_  = conv_weight_size;
  kws->conv_.M_            = conv_M;
//  kws->conv_.MQ_           = conv_MQ;
//  kws->conv_.MBits_        = conv_MBits;
//  kws->conv_.MF_           = conv_MF;
  kws->conv_.weights_        = conv_weight;
  kws->conv_.bias_           = conv_bias;
  kws->conv_.out_zero_point_ = conv_output_zero_point;
  kws->conv_.in_zero_point_  = conv_input_zero_point;

//  float *mf = conv_MF;
//  float max = 0.0f;
//  for(int i = 0; i < 64; i++){
//    if(mf[i] > max)
//      max = mf[i];
//  }
//  printf("max:%f\n",max);
//
//  for(int i = 0; i < 64; i++){
//    printf("%d,", (int32_t)((mf[i] * ((1 << (15+6)) - 1)+0.5)));
//  }
//  printf("\n");

  // dwconv_01_0
  kws->dwcnn_01_0_.groups_       = dwcnn_01_0_groups;
  kws->dwcnn_01_0_.padding_size_ = dwcnn_01_0_padding_size;
  kws->dwcnn_01_0_.stride_size_  = dwcnn_01_0_stride_size;
  kws->dwcnn_01_0_.weight_size_  = dwcnn_01_0_weight_size;
  kws->dwcnn_01_0_.M_            = dwcnn_01_0_M;
//  kws->dwcnn_01_0_.MQ_           = dwcnn_01_0_MQ;
//  kws->dwcnn_01_0_.MBits_        = dwcnn_01_0_MBits;
//  kws->dwcnn_01_0_.MF_           = dwcnn_01_0_MF;
  kws->dwcnn_01_0_.weights_        = dwcnn_01_0_weight;
  kws->dwcnn_01_0_.bias_           = dwcnn_01_0_bias;
  kws->dwcnn_01_0_.out_zero_point_ = dwcnn_01_0_output_zero_point;
  kws->dwcnn_01_0_.in_zero_point_  = dwcnn_01_0_input_zero_point;
  // dwconv_01_3
  kws->dwcnn_01_3_.groups_       = dwcnn_01_3_groups;
  kws->dwcnn_01_3_.padding_size_ = dwcnn_01_3_padding_size;
  kws->dwcnn_01_3_.stride_size_  = dwcnn_01_3_stride_size;
  kws->dwcnn_01_3_.weight_size_  = dwcnn_01_3_weight_size;
  kws->dwcnn_01_3_.M_            = dwcnn_01_3_M;
//  kws->dwcnn_01_3_.MQ_           = dwcnn_01_3_MQ;
//  kws->dwcnn_01_3_.MBits_        = dwcnn_01_3_MBits;
//  kws->dwcnn_01_3_.MF_           = dwcnn_01_3_MF;
  kws->dwcnn_01_3_.weights_        = dwcnn_01_3_weight;
  kws->dwcnn_01_3_.bias_           = dwcnn_01_3_bias;
  kws->dwcnn_01_3_.out_zero_point_ = dwcnn_01_3_output_zero_point;
  kws->dwcnn_01_3_.in_zero_point_  = dwcnn_01_3_input_zero_point;

  // dwconv_02_0
  kws->dwcnn_02_0_.groups_       = dwcnn_02_0_groups;
  kws->dwcnn_02_0_.padding_size_ = dwcnn_02_0_padding_size;
  kws->dwcnn_02_0_.stride_size_  = dwcnn_02_0_stride_size;
  kws->dwcnn_02_0_.weight_size_  = dwcnn_02_0_weight_size;
  kws->dwcnn_02_0_.M_            = dwcnn_02_0_M;
//  kws->dwcnn_02_0_.MQ_           = dwcnn_02_0_MQ;
//  kws->dwcnn_02_0_.MBits_        = dwcnn_02_0_MBits;
//  kws->dwcnn_02_0_.MF_           = dwcnn_02_0_MF;
  kws->dwcnn_02_0_.weights_        = dwcnn_02_0_weight;
  kws->dwcnn_02_0_.bias_           = dwcnn_02_0_bias;
  kws->dwcnn_02_0_.out_zero_point_ = dwcnn_02_0_output_zero_point;
  kws->dwcnn_02_0_.in_zero_point_  = dwcnn_02_0_input_zero_point;
  // dwconv_02_3
  kws->dwcnn_02_3_.groups_       = dwcnn_02_3_groups;
  kws->dwcnn_02_3_.padding_size_ = dwcnn_02_3_padding_size;
  kws->dwcnn_02_3_.stride_size_  = dwcnn_02_3_stride_size;
  kws->dwcnn_02_3_.weight_size_  = dwcnn_02_3_weight_size;
  kws->dwcnn_02_3_.M_            = dwcnn_02_3_M;
//  kws->dwcnn_02_3_.MQ_           = dwcnn_02_3_MQ;
//  kws->dwcnn_02_3_.MBits_        = dwcnn_02_3_MBits;
//  kws->dwcnn_02_3_.MF_           = dwcnn_02_3_MF;
  kws->dwcnn_02_3_.weights_        = dwcnn_02_3_weight;
  kws->dwcnn_02_3_.bias_           = dwcnn_02_3_bias;
  kws->dwcnn_02_3_.out_zero_point_ = dwcnn_02_3_output_zero_point;
  kws->dwcnn_02_3_.in_zero_point_  = dwcnn_02_3_input_zero_point;

  // dwconv_03_0
  kws->dwcnn_03_0_.groups_       = dwcnn_03_0_groups;
  kws->dwcnn_03_0_.padding_size_ = dwcnn_03_0_padding_size;
  kws->dwcnn_03_0_.stride_size_  = dwcnn_03_0_stride_size;
  kws->dwcnn_03_0_.weight_size_  = dwcnn_03_0_weight_size;
  kws->dwcnn_03_0_.M_            = dwcnn_03_0_M;
//  kws->dwcnn_03_0_.MQ_           = dwcnn_03_0_MQ;
//  kws->dwcnn_03_0_.MBits_        = dwcnn_03_0_MBits;
//  kws->dwcnn_03_0_.MF_           = dwcnn_03_0_MF;
  kws->dwcnn_03_0_.weights_        = dwcnn_03_0_weight;
  kws->dwcnn_03_0_.bias_           = dwcnn_03_0_bias;
  kws->dwcnn_03_0_.out_zero_point_ = dwcnn_03_0_output_zero_point;
  kws->dwcnn_03_0_.in_zero_point_  = dwcnn_03_0_input_zero_point;
  // dwconv_03_3
  kws->dwcnn_03_3_.groups_       = dwcnn_03_3_groups;
  kws->dwcnn_03_3_.padding_size_ = dwcnn_03_3_padding_size;
  kws->dwcnn_03_3_.stride_size_  = dwcnn_03_3_stride_size;
  kws->dwcnn_03_3_.weight_size_  = dwcnn_03_3_weight_size;
  kws->dwcnn_03_3_.M_            = dwcnn_03_3_M;
//  kws->dwcnn_03_3_.MQ_           = dwcnn_03_3_MQ;
//  kws->dwcnn_03_3_.MBits_        = dwcnn_03_3_MBits;
//  kws->dwcnn_03_3_.MF_           = dwcnn_03_3_MF;
  kws->dwcnn_03_3_.weights_        = dwcnn_03_3_weight;
  kws->dwcnn_03_3_.bias_           = dwcnn_03_3_bias;
  kws->dwcnn_03_3_.out_zero_point_ = dwcnn_03_3_output_zero_point;
  kws->dwcnn_03_3_.in_zero_point_  = dwcnn_03_3_input_zero_point;

  // dwconv_04_0
  kws->dwcnn_04_0_.groups_       = dwcnn_04_0_groups;
  kws->dwcnn_04_0_.padding_size_ = dwcnn_04_0_padding_size;
  kws->dwcnn_04_0_.stride_size_  = dwcnn_04_0_stride_size;
  kws->dwcnn_04_0_.weight_size_  = dwcnn_04_0_weight_size;
  kws->dwcnn_04_0_.M_            = dwcnn_04_0_M;
//  kws->dwcnn_04_0_.MQ_           = dwcnn_04_0_MQ;
//  kws->dwcnn_04_0_.MBits_        = dwcnn_04_0_MBits;
//  kws->dwcnn_04_0_.MF_           = dwcnn_04_0_MF;
  kws->dwcnn_04_0_.weights_        = dwcnn_04_0_weight;
  kws->dwcnn_04_0_.bias_           = dwcnn_04_0_bias;
  kws->dwcnn_04_0_.out_zero_point_ = dwcnn_04_0_output_zero_point;
  kws->dwcnn_04_0_.in_zero_point_  = dwcnn_04_0_input_zero_point;
  // dwconv_04_3
  kws->dwcnn_04_3_.groups_       = dwcnn_04_3_groups;
  kws->dwcnn_04_3_.padding_size_ = dwcnn_04_3_padding_size;
  kws->dwcnn_04_3_.stride_size_  = dwcnn_04_3_stride_size;
  kws->dwcnn_04_3_.weight_size_  = dwcnn_04_3_weight_size;
  kws->dwcnn_04_3_.M_            = dwcnn_04_3_M;
//  kws->dwcnn_04_3_.MQ_           = dwcnn_04_3_MQ;
//  kws->dwcnn_04_3_.MBits_        = dwcnn_04_3_MBits;
//  kws->dwcnn_04_3_.MF_           = dwcnn_04_3_MF;
  kws->dwcnn_04_3_.weights_        = dwcnn_04_3_weight;
  kws->dwcnn_04_3_.bias_           = dwcnn_04_3_bias;
  kws->dwcnn_04_3_.out_zero_point_ = dwcnn_04_3_output_zero_point;
  kws->dwcnn_04_3_.in_zero_point_  = dwcnn_04_3_input_zero_point;

  // dwconv_05_0
  kws->dwcnn_05_0_.groups_       = dwcnn_05_0_groups;
  kws->dwcnn_05_0_.padding_size_ = dwcnn_05_0_padding_size;
  kws->dwcnn_05_0_.stride_size_  = dwcnn_05_0_stride_size;
  kws->dwcnn_05_0_.weight_size_  = dwcnn_05_0_weight_size;
  kws->dwcnn_05_0_.M_            = dwcnn_05_0_M;
//  kws->dwcnn_05_0_.MQ_           = dwcnn_05_0_MQ;
//  kws->dwcnn_05_0_.MBits_        = dwcnn_05_0_MBits;
//  kws->dwcnn_05_0_.MF_           = dwcnn_05_0_MF;
  kws->dwcnn_05_0_.weights_        = dwcnn_05_0_weight;
  kws->dwcnn_05_0_.bias_           = dwcnn_05_0_bias;
  kws->dwcnn_05_0_.out_zero_point_ = dwcnn_05_0_output_zero_point;
  kws->dwcnn_05_0_.in_zero_point_  = dwcnn_05_0_input_zero_point;
  // dwconv_05_3
  kws->dwcnn_05_3_.groups_       = dwcnn_05_3_groups;
  kws->dwcnn_05_3_.padding_size_ = dwcnn_05_3_padding_size;
  kws->dwcnn_05_3_.stride_size_  = dwcnn_05_3_stride_size;
  kws->dwcnn_05_3_.weight_size_  = dwcnn_05_3_weight_size;
  kws->dwcnn_05_3_.M_            = dwcnn_05_3_M;
//  kws->dwcnn_05_3_.MQ_           = dwcnn_05_3_MQ;
//  kws->dwcnn_05_3_.MBits_        = dwcnn_05_3_MBits;
//  kws->dwcnn_05_3_.MF_           = dwcnn_05_3_MF;
  kws->dwcnn_05_3_.weights_        = dwcnn_05_3_weight;
  kws->dwcnn_05_3_.bias_           = dwcnn_05_3_bias;
  kws->dwcnn_05_3_.out_zero_point_ = dwcnn_05_3_output_zero_point;
  kws->dwcnn_05_3_.in_zero_point_  = dwcnn_05_3_input_zero_point;

  // glob_ave_pool_
  kws->glob_ave_pool_.output_size_ = global_average_pool_output_size;

  // linear
  kws->linear_.bias_              = linear_bias;
  kws->linear_.weights_           = linear_weights;
  kws->linear_.weight_size_       = linear_weight_size;
  kws->linear_.output_zero_point_ = linear_output_zero_point;
  kws->linear_.M_                 = linear_M;
//  kws->linear_.MQ_                = linear_MQ;
//  kws->linear_.MBits_             = linear_MBits;
//  kws->linear_.MF_                = linear_MF;

//  printf("tensor0:0x%08x, %d, [%d, %d, %d], %d\n", (uint32_t)tensor0.data_, tensor0.data_len_, tensor0.data_shape_[0],tensor0.data_shape_[1],tensor0.data_shape_[2],tensor0.capacity_);
//  printf("tensor1:0x%08x, %d, [%d, %d, %d], %d\n", (uint32_t)tensor1.data_, tensor1.data_len_, tensor1.data_shape_[0],tensor1.data_shape_[1],tensor1.data_shape_[2],tensor1.capacity_);
}

void LayerForward(Tensor* input, Tensor* output, TKWS* kws) {

  QuantForward(input, &tensor0, &(kws->quant_));
//  uint8_t *data_p = tensor0.data_;
//  for (int i = 0; i < tensor0.data_len_; ++i) {
//    printf("%u,", data_p[i]);
//  }
//  printf("\n");
  GeneralCNNForward3X3Fast(&tensor0, &tensor1, &(kws->conv_));
//  uint8_t *data_p = tensor1.data_;
//  for (int i = 0; i < tensor1.data_len_; ++i) {
//    printf("%u,", data_p[i]);
//  }
//  printf("\n");
  DSCNNForward3X3(&tensor1, &tensor0, &(kws->dwcnn_01_0_));
//  uint8_t *data_p = tensor0.data_;
//  for (int i = 0; i < tensor0.data_len_; ++i) {
//    printf("%u,", data_p[i]);
//  }
//  printf("\n");
  PointCNNForwardFast(&tensor0, &tensor1, &(kws->dwcnn_01_3_));
//  uint8_t *data_p = tensor1.data_;
//  for (int i = 0; i < tensor1.data_len_; ++i) {
//    printf("%u,", data_p[i]);
//  }
//  printf("\n");

  DSCNNForward3X3(&tensor1, &tensor0, &(kws->dwcnn_02_0_));
  PointCNNForwardFast(&tensor0, &tensor1, &(kws->dwcnn_02_3_));

  DSCNNForward3X3(&tensor1, &tensor0, &(kws->dwcnn_03_0_));
  PointCNNForwardFast(&tensor0, &tensor1, &(kws->dwcnn_03_3_));

  DSCNNForward3X3(&tensor1, &tensor0, &(kws->dwcnn_04_0_));
  PointCNNForwardFast(&tensor0, &tensor1, &(kws->dwcnn_04_3_));

  DSCNNForward3X3(&tensor1, &tensor0, &(kws->dwcnn_05_0_));
  PointCNNForwardFast(&tensor0, &tensor1, &(kws->dwcnn_05_3_));

  GlobAvePoolForward(&tensor1, &tensor0, &(kws->glob_ave_pool_));
  LinearForward(&tensor0, output, &(kws->linear_));
}
