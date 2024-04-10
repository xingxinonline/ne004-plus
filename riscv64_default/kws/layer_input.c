#include <math.h>
#ifdef MAC
#include <string.h>
#elif RISCV
#include "malloc.h"
#elif ARM_MATH_DSP
#include <string.h>
#endif
#include "layer_types.h"
#include "layer_tools.h"


void layer_input_forward(BaseFloat* input, IOFeature* ofeature, TINPUT* tinput) {
  int8_t  min_val = -pow(2, 7);
  int8_t  max_val = pow(2, 7) - 1;
  int8_t* output = (int8_t*)ofeature->feature_buffer;

  uint32_t output_len = tinput->layer_output_shape[0] * tinput->layer_output_shape[1] * tinput->layer_output_shape[2];
  for (uint32_t i = 0; i < output_len; ++i) {
    output[i] = clampfloat(round(input[i] / tinput->layer_output_scale), min_val, max_val);
  }
#ifdef MAC
  ofeature->feature_type = strcmp(tinput->layer_output_type, "int8") == 0 ? 1 : 0;
#elif RISCV
  ofeature->feature_type = mystrcmp(tinput->layer_output_type, "int8") == 0 ? 1 : 0;
#elif ARM_MATH_DSP
  ofeature->feature_type = strcmp(tinput->layer_output_type, "int8") == 0 ? 1 : 0;
#endif
}