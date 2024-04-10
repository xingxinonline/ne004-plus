/* ----------------------------------------------------------------------
 * Project:      CMSIS NN Library
 * Title:        arm_convolve_wrapper_s8.c
 * Description:  s8 convolution layer wrapper function with the main purpose to call the optimal kernel available in
 * cmsis-nn to perform the convolution.
 *
 * $Date:        02. December 2021
 * $Revision:    V.1.1.0
 *
 * Target Processor:  Cortex-M cores
 *
 * -------------------------------------------------------------------- */
// #include <assert.h>
#include "layer_cnn_wrap.h"
#include "layer_cnn_functions.h"

arm_status arm_convolve_wrapper(const int8_t input_type,
                                const cmsis_nn_context *ctx,
                                const cmsis_nn_conv_params *conv_params,
                                const cmsis_nn_per_channel_quant_params *quant_params,
                                const cmsis_nn_dims *input_dims,
                                void* input_data,
                                const cmsis_nn_dims *filter_dims,
                                const q7_t *filter_data,
                                const cmsis_nn_dims *bias_dims,
                                const int32_t *bias_data,
                                const cmsis_nn_dims *output_dims,
                                q7_t *output_data) {
  if ((conv_params->padding.w == 0) && (conv_params->padding.h == 0) && (input_dims->c % 4 == 0) &&
      (conv_params->stride.w == 1) && (conv_params->stride.h == 1) && (filter_dims->w == 1) &&
      (filter_dims->h == 1) && (conv_params->dilation.w == 1 && conv_params->dilation.h == 1)) {
    return arm_convolve_1x1_fast(input_type, ctx, conv_params, quant_params, input_dims,
                                 (void*)input_data, filter_dims, filter_data,
                                 bias_dims, bias_data, output_dims, output_data);
  } else if ((output_dims->h == 1) && (input_dims->h == 1) && (filter_dims->h == 1) && (output_dims->w % 4 == 0) &&
             (input_dims->n == 1) && (conv_params->dilation.w == 1 && conv_params->dilation.h == 1)) {
    return arm_convolve_1_x_n(input_type, ctx, conv_params, quant_params, input_dims, (q7_t*)input_data,
                              filter_dims, filter_data, bias_dims, bias_data, output_dims, output_data);
  } else {
    return arm_convolve(input_type, ctx, conv_params, quant_params, input_dims, (q7_t*)input_data,
                        filter_dims, filter_data, bias_dims, bias_data, output_dims, output_data);
  }
}

arm_status arm_depthwise_conv_wrapper(const int8_t input_type,
                                      const cmsis_nn_context *ctx,
                                      const cmsis_nn_dw_conv_params *dw_conv_params,
                                      const cmsis_nn_per_channel_quant_params *quant_params,
                                      const cmsis_nn_dims *input_dims,
                                      void* input,
                                      const cmsis_nn_dims *filter_dims,
                                      const q7_t *filter,
                                      const cmsis_nn_dims *bias_dims,
                                      const int32_t *bias,
                                      const cmsis_nn_dims *output_dims,
                                      q7_t *output) {

  arm_status status = ARM_MATH_SUCCESS;

  if (1 == dw_conv_params->ch_mult && input_dims->n == 1 && dw_conv_params->dilation.w == 1 &&
      dw_conv_params->dilation.h == 1) {
#if !defined(ARM_MATH_MVEI)
    if ((filter_dims->w == 3) && (filter_dims->h == 3) && (dw_conv_params->padding.h <= 1) &&
        (dw_conv_params->padding.w <= 1)) {
      status = arm_depthwise_conv_3x3(input_type, ctx, dw_conv_params, quant_params, input_dims,
                                      (q7_t*)input, filter_dims, filter, bias_dims,
                                      bias, output_dims, output);
    } else
#endif
    {
      status = arm_depthwise_conv_opt(input_type, ctx, dw_conv_params, quant_params, input_dims,
                                      (q7_t*)input, filter_dims, filter, bias_dims,
                                      bias, output_dims, output);
    }
  } else {
    status = arm_depthwise_conv(input_type, ctx, dw_conv_params, quant_params, input_dims,
                                (q7_t*)input, filter_dims, filter, bias_dims,
                                bias, output_dims, output);
  }
  /* Return to application */
  return status;
}

void layer_conv_forward(IOFeature* ifeature, IOFeature* ofeature, TCNN2D* tcnn2d) {
  if (tcnn2d->layer_groups == 1) { // convolution
    arm_convolve_wrapper(ifeature->feature_type == 1 ? 1 : 0,
                         &(tcnn2d->ctx),
                         &(tcnn2d->conv_params),
                         &(tcnn2d->quant_params),
                         &(tcnn2d->input_dims),
                         (void*)ifeature->feature_buffer,
                         &(tcnn2d->filter_dims),
                         tcnn2d->layer_weight,
                         &(tcnn2d->bias_dims),
                         tcnn2d->layer_bias,
                         &(tcnn2d->output_dims),
                         (q7_t*)(ofeature->feature_buffer));
#ifdef MAC
    ofeature->feature_type = strcmp(tcnn2d->layer_output_type, "int8") == 0;
#elif RISCV
    ofeature->feature_type = mystrcmp(tcnn2d->layer_output_type, "int8") == 0;
#elif ARM_MATH_DSP
    ofeature->feature_type = strcmp(tcnn2d->layer_output_type, "int8") == 0;
#endif
  } else { // depth_wise convolution
    arm_depthwise_conv_wrapper(ifeature->feature_type == 1 ? 1 : 0,
                               &(tcnn2d->ctx),
                               &(tcnn2d->dw_conv_params),
                               &(tcnn2d->quant_params),
                               &(tcnn2d->input_dims),
                               (void*)ifeature->feature_buffer,
                               &(tcnn2d->filter_dims),
                               tcnn2d->layer_weight,
                               &(tcnn2d->bias_dims),
                               tcnn2d->layer_bias,
                               &(tcnn2d->output_dims),
                               (q7_t*)(ofeature->feature_buffer));
#ifdef MAC
    ofeature->feature_type = strcmp(tcnn2d->layer_output_type, "int8") == 0;
#elif RISCV
    ofeature->feature_type = mystrcmp(tcnn2d->layer_output_type, "int8") == 0;
#elif ARM_MATH_DSP
    ofeature->feature_type = strcmp(tcnn2d->layer_output_type, "int8") == 0;
#endif
  }
}