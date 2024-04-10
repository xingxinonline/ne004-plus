#include "layer_cnn_weights.h"
#include "layer_cnn_wrap.h"
#include "layer_tools.h"
#include "layer_input.h"
#include "layer_types.h"
#ifdef MAC
#include <stdio.h>
#elif RISCV
#include "layer_fix.h"
#endif


extern TINPUT  layer_0;
extern TCNN2D  layer_1;
extern TCNN2D  layer_2;
extern TCNN2D  layer_3;
extern TCNN2D  layer_4;
extern TCNN2D  layer_5;
extern TCNN2D  layer_6;
extern TCNN2D  layer_7;
extern TCNN2D  layer_8;
extern TCNN2D  layer_9;
extern TCNN2D  layer_10;
extern TCNN2D  layer_11;
extern TCNN2D  layer_12;
extern TLINEAR layer_13;
extern IOFeature ifeature;
extern IOFeature ofeature;

// 定制化
void inference_init() {
  layer_0.forward = &layer_input_forward;
  layer_1.forward = &layer_conv_forward;
  layer_2.forward = &layer_conv_forward;
  layer_3.forward = &layer_conv_forward;
  layer_4.forward = &layer_conv_forward;
  layer_5.forward = &layer_conv_forward;
  layer_6.forward = &layer_conv_forward;
  layer_7.forward = &layer_conv_forward;
  layer_8.forward = &layer_conv_forward;
  layer_9.forward = &layer_conv_forward;
  layer_10.forward = &layer_conv_forward;
  layer_11.forward = &layer_conv_forward;
  layer_12.forward = &layer_conv_forward;
  layer_13.forward = &layer_linear_forward;

  layer_weight_init();
}

void inference_stop() {
  layer_weight_destroy();
}

// 定制化版本，不通用 不同结构需要更改
void inference_start(BaseFloat* input) {
  layer_0.forward(input, &ifeature, &layer_0);
  layer_1.forward(&ifeature, &ofeature, &layer_1);
  layer_2.forward(&ofeature, &ifeature, &layer_2);
  layer_3.forward(&ifeature, &ofeature, &layer_3);
  layer_4.forward(&ofeature, &ifeature, &layer_4);
  layer_5.forward(&ifeature, &ofeature, &layer_5);
  layer_6.forward(&ofeature, &ifeature, &layer_6);
  layer_7.forward(&ifeature, &ofeature, &layer_7);
  layer_8.forward(&ofeature, &ifeature, &layer_8);
  layer_9.forward(&ifeature, &ofeature, &layer_9);
  layer_10.forward(&ofeature, &ifeature, &layer_10);
  layer_11.forward(&ifeature, &ofeature, &layer_11);
  layer_12.forward(&ofeature, &ifeature, &layer_12);
  layer_13.forward(&ifeature, &ofeature, &layer_13);
#ifndef RISCV
  ShowLinearLayerData(&ofeature, &layer_13);
#endif
}
