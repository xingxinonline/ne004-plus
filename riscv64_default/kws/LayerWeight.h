#ifndef __LAYERWEIGHT_H__
#define __LAYERWEIGHT_H__
#include <stdint.h>

//=================quant==================
extern const float quant_output_scale;
extern const uint8_t quant_output_zero_point;


//=================conv==================
extern const int32_t conv_bias[64];
extern const int8_t conv_weight[576];
extern const uint8_t conv_output_zero_point;
extern const uint8_t conv_input_zero_point;
extern const int16_t conv_M[64];
//extern const int16_t conv_MQ;
//extern const int32_t conv_MBits;
//extern const float conv_MF[64];
extern const int16_t conv_weight_size[4];
extern const int16_t conv_stride_size[2];
extern const int16_t conv_padding_size[2];
extern const int16_t conv_groups;


//=================depth_wise_separable_conv_01.0==================
extern const int32_t dwcnn_01_0_bias[64];
extern const int8_t dwcnn_01_0_weight[576];
extern const uint8_t dwcnn_01_0_output_zero_point;
extern const uint8_t dwcnn_01_0_input_zero_point;
extern const int16_t dwcnn_01_0_M[64];
//extern const int16_t dwcnn_01_0_MQ;
//extern const int32_t dwcnn_01_0_MBits;
//extern const float dwcnn_01_0_MF[64];
extern const int16_t dwcnn_01_0_weight_size[4];
extern const int16_t dwcnn_01_0_stride_size[2];
extern const int16_t dwcnn_01_0_padding_size[2];
extern const int16_t dwcnn_01_0_groups;


//=================depth_wise_separable_conv_01.3==================
extern const int32_t dwcnn_01_3_bias[64];
extern const int8_t dwcnn_01_3_weight[4096];
extern const uint8_t dwcnn_01_3_output_zero_point;
extern const uint8_t dwcnn_01_3_input_zero_point;
extern const int16_t dwcnn_01_3_M[64];
//extern const int16_t dwcnn_01_3_MQ;
//extern const int32_t dwcnn_01_3_MBits;
//extern const float dwcnn_01_3_MF[64];
extern const int16_t dwcnn_01_3_weight_size[4];
extern const int16_t dwcnn_01_3_stride_size[2];
extern const int16_t dwcnn_01_3_padding_size[2];
extern const int16_t dwcnn_01_3_groups;


//=================depth_wise_separable_conv_02.0==================
extern const int32_t dwcnn_02_0_bias[64];
extern const int8_t dwcnn_02_0_weight[576];
extern const uint8_t dwcnn_02_0_output_zero_point;
extern const uint8_t dwcnn_02_0_input_zero_point;
extern const int16_t dwcnn_02_0_M[64];
//extern const int16_t dwcnn_02_0_MQ;
//extern const int32_t dwcnn_02_0_MBits;
//extern const float dwcnn_02_0_MF[64];
extern const int16_t dwcnn_02_0_weight_size[4];
extern const int16_t dwcnn_02_0_stride_size[2];
extern const int16_t dwcnn_02_0_padding_size[2];
extern const int16_t dwcnn_02_0_groups;


//=================depth_wise_separable_conv_02.3==================
extern const int32_t dwcnn_02_3_bias[64];
extern const int8_t dwcnn_02_3_weight[4096];
extern const uint8_t dwcnn_02_3_output_zero_point;
extern const uint8_t dwcnn_02_3_input_zero_point;
extern const int16_t dwcnn_02_3_M[64];
//extern const int16_t dwcnn_02_3_MQ;
//extern const int32_t dwcnn_02_3_MBits;
//extern const float dwcnn_02_3_MF[64];
extern const int16_t dwcnn_02_3_weight_size[4];
extern const int16_t dwcnn_02_3_stride_size[2];
extern const int16_t dwcnn_02_3_padding_size[2];
extern const int16_t dwcnn_02_3_groups;


//=================depth_wise_separable_conv_03.0==================
extern const int32_t dwcnn_03_0_bias[64];
extern const int8_t dwcnn_03_0_weight[576];
extern const uint8_t dwcnn_03_0_output_zero_point;
extern const uint8_t dwcnn_03_0_input_zero_point;
extern const int16_t dwcnn_03_0_M[64];
//extern const int16_t dwcnn_03_0_MQ;
//extern const int32_t dwcnn_03_0_MBits;
//extern const float dwcnn_03_0_MF[64];
extern const int16_t dwcnn_03_0_weight_size[4];
extern const int16_t dwcnn_03_0_stride_size[2];
extern const int16_t dwcnn_03_0_padding_size[2];
extern const int16_t dwcnn_03_0_groups;


//=================depth_wise_separable_conv_03.3==================
extern const int32_t dwcnn_03_3_bias[64];
extern const int8_t dwcnn_03_3_weight[4096];
extern const uint8_t dwcnn_03_3_output_zero_point;
extern const uint8_t dwcnn_03_3_input_zero_point;
extern const int16_t dwcnn_03_3_M[64];
//extern const int16_t dwcnn_03_3_MQ;
//extern const int32_t dwcnn_03_3_MBits;
//extern const float dwcnn_03_3_MF[64];
extern const int16_t dwcnn_03_3_weight_size[4];
extern const int16_t dwcnn_03_3_stride_size[2];
extern const int16_t dwcnn_03_3_padding_size[2];
extern const int16_t dwcnn_03_3_groups;


//=================depth_wise_separable_conv_04.0==================
extern const int32_t dwcnn_04_0_bias[64];
extern const int8_t dwcnn_04_0_weight[576];
extern const uint8_t dwcnn_04_0_output_zero_point;
extern const uint8_t dwcnn_04_0_input_zero_point;
extern const int16_t dwcnn_04_0_M[64];
//extern const int16_t dwcnn_04_0_MQ;
//extern const int32_t dwcnn_04_0_MBits;
//extern const float dwcnn_04_0_MF[64];
extern const int16_t dwcnn_04_0_weight_size[4];
extern const int16_t dwcnn_04_0_stride_size[2];
extern const int16_t dwcnn_04_0_padding_size[2];
extern const int16_t dwcnn_04_0_groups;


//=================depth_wise_separable_conv_04.3==================
extern const int32_t dwcnn_04_3_bias[64];
extern const int8_t dwcnn_04_3_weight[4096];
extern const uint8_t dwcnn_04_3_output_zero_point;
extern const uint8_t dwcnn_04_3_input_zero_point;
extern const int16_t dwcnn_04_3_M[64];
//extern const int16_t dwcnn_04_3_MQ;
//extern const int32_t dwcnn_04_3_MBits;
//extern const float dwcnn_04_3_MF[64];
extern const int16_t dwcnn_04_3_weight_size[4];
extern const int16_t dwcnn_04_3_stride_size[2];
extern const int16_t dwcnn_04_3_padding_size[2];
extern const int16_t dwcnn_04_3_groups;


//=================depth_wise_separable_conv_05.0==================
extern const int32_t dwcnn_05_0_bias[64];
extern const int8_t dwcnn_05_0_weight[576];
extern const uint8_t dwcnn_05_0_output_zero_point;
extern const uint8_t dwcnn_05_0_input_zero_point;
extern const int16_t dwcnn_05_0_M[64];
//extern const int16_t dwcnn_05_0_MQ;
//extern const int32_t dwcnn_05_0_MBits;
//extern const float dwcnn_05_0_MF[64];
extern const int16_t dwcnn_05_0_weight_size[4];
extern const int16_t dwcnn_05_0_stride_size[2];
extern const int16_t dwcnn_05_0_padding_size[2];
extern const int16_t dwcnn_05_0_groups;


//=================depth_wise_separable_conv_05.3==================
extern const int32_t dwcnn_05_3_bias[64];
extern const int8_t dwcnn_05_3_weight[4096];
extern const uint8_t dwcnn_05_3_output_zero_point;
extern const uint8_t dwcnn_05_3_input_zero_point;
extern const int16_t dwcnn_05_3_M[64];
//extern const int16_t dwcnn_05_3_MQ;
//extern const int32_t dwcnn_05_3_MBits;
//extern const float dwcnn_05_3_MF[64];
extern const int16_t dwcnn_05_3_weight_size[4];
extern const int16_t dwcnn_05_3_stride_size[2];
extern const int16_t dwcnn_05_3_padding_size[2];
extern const int16_t dwcnn_05_3_groups;


//=================global_average_pool==================
extern const int16_t global_average_pool_output_size[2];


//=================linear==================
extern const int32_t linear_bias[12];
extern const int8_t linear_weights[768];
extern const int16_t linear_weight_size[2];
extern const uint8_t linear_output_zero_point;
extern const int16_t linear_M[12];
//extern const int16_t linear_MQ;
//extern const int32_t linear_MBits;
//extern const float linear_MF[12];


#endif
