#ifdef MAC
#include <string.h>
#elif RISCV
#include "malloc.h"
#elif ARM_MATH_DSP
#include <string.h>
#endif
#include "layer_types.h"

void fully_connected_int8_to_int8(IOFeature* ifeature, IOFeature* ofeature, TLINEAR* linear) {
  uint16_t num_of_rows = linear->layer_output_shape[0] * linear->layer_output_shape[1] * linear->layer_output_shape[2]; // 13
  uint16_t dim_vec = linear->layer_input_shape[0] * linear->layer_input_shape[1] * linear->layer_input_shape[2];        // 156

  int8_t*  pV = (int8_t*)ifeature->feature_buffer;
  int8_t*  pM = linear->layer_weight;
  int8_t*  out_shift = linear->layer_weight_shift;
  int32_t* bias = linear->layer_bias;

  int8_t* output = (int8_t*)(ofeature->feature_buffer);
  int16_t i, j;

  /* Run the following code as reference implementation for Cortex-M0 and Cortex-M3 */
  for (i = 0; i < num_of_rows; i++) {
    int ip_out = bias[i];
    for (j = 0; j < dim_vec; j++) {
      ip_out += pV[j] * pM[i * dim_vec + j];
    }
    ip_out = ((ip_out + (1 << (out_shift[i] - 1))) >> out_shift[i]);
    ip_out = MIN(ip_out, MAXINT8);
    ip_out = MAX(ip_out, MININT8);
    output[i] = ip_out;
  }
#ifdef MAC
  ofeature->feature_type = strcmp(linear->layer_output_type, "int8") == 0;
#elif RISCV
  ofeature->feature_type = mystrcmp(linear->layer_output_type, "int8") == 0;
#elif ARM_MATH_DSP
  ofeature->feature_type = strcmp(linear->layer_output_type, "int8") == 0;
#endif
}

void fully_connected_uint8_to_int8(IOFeature* ifeature, IOFeature* ofeature, TLINEAR* linear) {
  uint16_t num_of_rows = linear->layer_output_shape[0] * linear->layer_output_shape[1] * linear->layer_output_shape[2]; // 13
  uint16_t dim_vec = linear->layer_input_shape[0] * linear->layer_input_shape[1] * linear->layer_input_shape[2];        // 156

  uint8_t* pV = (uint8_t*)ifeature->feature_buffer;
  int8_t*  pM = linear->layer_weight;
  int8_t*  out_shift = linear->layer_weight_shift;
  int32_t* bias = linear->layer_bias;

  int8_t* output = (int8_t*)(ofeature->feature_buffer);
  int16_t i, j;

  /* Run the following code as reference implementation for Cortex-M0 and Cortex-M3 */
  for (i = 0; i < num_of_rows; i++) {
    int ip_out = bias[i];
    for (j = 0; j < dim_vec; j++) {
      ip_out += pV[j] * pM[i * dim_vec + j];
    }
    ip_out = ((ip_out + (1 << (out_shift[i] - 1))) >> out_shift[i]);
    ip_out = MIN(ip_out, MAXINT8);
    ip_out = MAX(ip_out, MININT8);
    output[i] = ip_out;
  }
#ifdef MAC
  ofeature->feature_type = strcmp(linear->layer_output_type, "int8") == 0;
#elif RISCV
  ofeature->feature_type = mystrcmp(linear->layer_output_type, "int8") == 0;
#elif ARM_MATH_DSP
  ofeature->feature_type = strcmp(linear->layer_output_type, "int8") == 0;
#endif
}

void layer_linear_forward(IOFeature* ifeature, IOFeature* ofeature, TLINEAR* linear) {
#ifdef MAC
  if (ifeature->feature_type == 1 && strcmp(linear->layer_output_type, "int8") == 0) { // int8 -> int8
#elif RISCV
  if (ifeature->feature_type == 1 && mystrcmp(linear->layer_output_type, "int8") == 0) { // int8 -> int8
#elif ARM_MATH_DSP
  if (ifeature->feature_type == 1 && strcmp(linear->layer_output_type, "int8") == 0) { // int8 -> int8
#endif
    fully_connected_int8_to_int8(ifeature, ofeature, linear);
#ifdef MAC
  } else if (ifeature->feature_type == 0 && strcmp(linear->layer_output_type, "int8") == 0) { // uint8 -> int8
#elif RISCV
  } else if (ifeature->feature_type == 0 && mystrcmp(linear->layer_output_type, "int8") == 0) { // uint8 -> int8
#elif ARM_MATH_DSP
  } else if (ifeature->feature_type == 0 && strcmp(linear->layer_output_type, "int8") == 0) { // uint8 -> int8
#endif
    fully_connected_uint8_to_int8(ifeature, ofeature, linear);
  } else {
    //printf("ERR: LINEAR don't support this now\n");
    return;
  }
}

