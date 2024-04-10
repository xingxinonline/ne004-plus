#ifndef __LAYER_CNN_FUNCTIONS_H__
#define __LAYER_CNN_FUNCTIONS_H__
#include "layer_types.h"
#include "layer_cnn_supports.h"

/**
 * @brief 1xn convolution
 *
 * @param[in, out] ctx            Function context that contains the additional buffer if required by the function.
                                  arm_convolve_1_x_n_s8_get_buffer_size will return the buffer_size if required
 * @param[in]      conv_params    Convolution parameters (e.g. strides, dilations, pads,...).
 *                                Range of conv_params->input_offset  : [-127, 128]
 *                                Range of conv_params->output_offset : [-128, 127]
 * @param[in]      quant_params   Per-channel quantization info.
 *                                It contains the multiplier(abandoned) and shift values to be applied to each output channel
 * @param[in]      input_dims     Input (activation) tensor dimensions. Format: [N, H, W, C_IN]
 * @param[in]      input_data     Input (activation) data pointer. Data type: int8
 * @param[in]      filter_dims    Filter tensor dimensions. Format: [C_OUT, 1, WK, C_IN] where WK is the horizontal
 *                                spatial filter dimension
 * @param[in]      filter_data    Filter data pointer. Data type: int8
 * @param[in]      bias_dims      Bias tensor dimensions. Format: [C_OUT]
 * @param[in]      bias_data      Optional bias data pointer. Data type: int32
 * @param[in]      output_dims    Output tensor dimensions. Format: [N, H, W, C_OUT]
 * @param[out]     output_data    Output data pointer. Data type: int8
 *
 * @return     The function returns either
 *                  <code>ARM_MATH_SIZE_MISMATCH</code> if argument constraints fail. or,
 *                  <code>ARM_MATH_SUCCESS</code> on successful completion.
 *
 * @details
 *   - Supported framework : TensorFlow Lite Micro
 *   - The following constrains on the arguments apply
 *      -# input_dims->n equals 1
 *      -# ouput_dims->w is a multiple of 4
 *      -# Explicit constraints(since it is for 1xN convolution)
 *      -## input_dims->h equals 1
 *      -## output_dims->h equals 1
 *      -## filter_dims->h equals 1
 *@todo  Remove constraint on output_dims->w to make the function generic.
 *
 */
arm_status arm_convolve_1_x_n(const int8_t input_type,
                              const cmsis_nn_context *ctx,
                              const cmsis_nn_conv_params *conv_params,
                              const cmsis_nn_per_channel_quant_params *quant_params,
                              const cmsis_nn_dims *input_dims,
                              void *input_data,
                              const cmsis_nn_dims *filter_dims,
                              const q7_t *filter_data,
                              const cmsis_nn_dims *bias_dims,
                              const int32_t *bias_data,
                              const cmsis_nn_dims *output_dims,
                              q7_t *output_data);

/**
 * @brief Fast s8 version for 1x1 convolution (non-square shape)
 *
 * @param[in, out] ctx            Function context that contains the additional buffer if required by the function.
                                  arm_convolve_1x1_s8_fast_get_buffer_size will return the buffer_size if required
 * @param[in]      conv_params    Convolution parameters (e.g. strides, dilations, pads,...).
 *                                Range of conv_params->input_offset  : [-127, 128]
 *                                Range of conv_params->output_offset : [-128, 127]
 * @param[in]      quant_params   Per-channel quantization info.
 *                                It contains the multiplier and shift values to be applied to each output channel
 * @param[in]      input_dims     Input (activation) tensor dimensions. Format: [N, H, W, C_IN]
 * @param[in]      input_data     Input (activation) data pointer. Data type: int8
 * @param[in]      filter_dims    Filter tensor dimensions. Format: [C_OUT, 1, 1, C_IN]
 * @param[in]      filter_data    Filter data pointer. Data type: int8
 * @param[in]      bias_dims      Bias tensor dimensions. Format: [C_OUT]
 * @param[in]      bias_data      Optional bias data pointer. Data type: int32
 * @param[in]      output_dims    Output tensor dimensions. Format: [N, H, W, C_OUT]
 * @param[out]     output_data    Output data pointer. Data type: int8
 *
 * @return     The function returns either
 *                  <code>ARM_MATH_SIZE_MISMATCH</code> if argument constraints fail. or,
 *                  <code>ARM_MATH_SUCCESS</code> on successful completion.
 *
 * @details
 *   - Supported framework : TensorFlow Lite Micro
 *   - The following constrains on the arguments apply
 *      -# input_dims->c is a multiple of 4
 *      -# conv_params->padding.w = conv_params->padding.h = 0
 *      -# conv_params->stride.w = conv_params->stride.h = 1
 *
 */
arm_status arm_convolve_1x1_fast(const int8_t input_type,
                                 const cmsis_nn_context *ctx,
                                 const cmsis_nn_conv_params *conv_params,
                                 const cmsis_nn_per_channel_quant_params *quant_params,
                                 const cmsis_nn_dims *input_dims,
                                 void  *input_data,
                                 const cmsis_nn_dims *filter_dims,
                                 const q7_t *filter_data,
                                 const cmsis_nn_dims *bias_dims,
                                 const int32_t *bias_data,
                                 const cmsis_nn_dims *output_dims,
                                 q7_t *output_data);

/**
 * @brief Basic s8 convolution function
 * @param[in, out] ctx            Function context that contains the additional buffer if required by the function.
                                  arm_convolve_s8_get_buffer_size will return the buffer_size if required
 * @param[in]      conv_params    Convolution parameters (e.g. strides, dilations, pads,...).
 *                                Range of conv_params->input_offset  : [-127, 128]
 *                                Range of conv_params->output_offset : [-128, 127]
 * @param[in]      quant_params   Per-channel quantization info.
 *                                It contains the multiplier and shift values to be applied to each output channel
 * @param[in]      input_dims     Input (activation) tensor dimensions. Format: [N, H, W, C_IN]
 * @param[in]      input_data     Input (activation) data pointer. Data type: int8
 * @param[in]      filter_dims    Filter tensor dimensions. Format: [C_OUT, HK, WK, C_IN] where HK and WK are the
 *                                spatial filter dimensions
 * @param[in]      filter_data    Filter data pointer. Data type: int8
 * @param[in]      bias_dims      Bias tensor dimensions. Format: [C_OUT]
 * @param[in]      bias_data      Optional bias data pointer. Data type: int32
 * @param[in]      output_dims    Output tensor dimensions. Format: [N, H, W, C_OUT]
 * @param[out]     output_data    Output data pointer. Data type: int8

 * @return     The function returns <code>ARM_MATH_SUCCESS</code>
 *
 * @details
 *    1. Supported framework: TensorFlow Lite micro
 *    2. q7 is used as data type eventhough it is s8 data. It is done so to be consistent with existing APIs.
 *    3. Additional memory is required for optimization. Refer to argument 'ctx' for details.
 *
 */
arm_status arm_convolve(const int8_t input_type,
                        const cmsis_nn_context *ctx,
                        const cmsis_nn_conv_params *conv_params,
                        const cmsis_nn_per_channel_quant_params *quant_params,
                        const cmsis_nn_dims *input_dims,
                        void *input_data,
                        const cmsis_nn_dims *filter_dims,
                        const q7_t *filter_data,
                        const cmsis_nn_dims *bias_dims,
                        const int32_t *bias_data,
                        const cmsis_nn_dims *output_dims,
                        q7_t *output_data);

/**
 * @brief Basic s8 depthwise convolution function that doesn't have any constraints on the input dimensions.
 *
 * @param[in, out] ctx            Function context (e.g. temporary buffer). Check the function
 *                                definition file to see if an additional buffer is required.
 *                                Optional function {API}_get_buffer_size() provides the buffer
 *                                size if an additional buffer is required.
 *                                exists if additional memory is.
 * @param[in]      dw_conv_params Depthwise convolution parameters (e.g. strides, dilations, pads,...)
 *                                dw_conv_params->dilation is not used.
 *                                Range of dw_conv_params->input_offset : [-127, 128]
 *                                Range of dw_conv_params->input_offset : [-128, 127]
 * @param[in]      quant_params   Per-channel quantization info.
 *                               It contains the multiplier and shift values to be applied to each
 *                               output channel
 * @param[in]      input_dims     Input (activation) tensor dimensions. Format: [N, H, W, C_IN]
 *                                Batch argument N is not used.
 * @param[in]      input_data     Input (activation) data pointer. Data type: int8
 * @param[in]      filter_dims    Filter tensor dimensions. Format: [1, H, W, C_OUT]
 * @param[in]      filter_data    Filter data pointer. Data type: int8
 * @param[in]      bias_dims      Bias tensor dimensions. Format: [C_OUT]
 * @param[in]      bias_data      Bias data pointer. Data type: int32
 * @param[in]      output_dims    Output tensor dimensions. Format: [N, H, W, C_OUT]
 * @param[in, out] output_data    Output data pointer. Data type: int8
 * @return     The function returns <code>ARM_MATH_SUCCESS</code>
 *
 * @details
 *    - Supported framework: TensorFlow Lite
 *    - q7 is used as data type eventhough it is s8 data. It is done so to be consistent with existing APIs.
 */
arm_status arm_depthwise_conv(const int8_t input_type,
                              const cmsis_nn_context *ctx,
                              const cmsis_nn_dw_conv_params *dw_conv_params,
                              const cmsis_nn_per_channel_quant_params *quant_params,
                              const cmsis_nn_dims *input_dims,
                              void  *input,
                              const cmsis_nn_dims *filter_dims,
                              const q7_t *kernel,
                              const cmsis_nn_dims *bias_dims,
                              const int32_t *bias,
                              const cmsis_nn_dims *output_dims,
                              q7_t *output);

/**
 * @brief Optimized s8/u8 depthwise convolution function for 3x3 kernel size with some constraints on
 *        the input arguments(documented below). Refer arm_depthwise_conv_s8() for function
 *        argument details.
 *
 * @return     The function returns one of the following
 *                <code>ARM_MATH_SIZE_MISMATCH</code> - Unsupported dimension of tensors
 *                <code>ARM_MATH_ARGUMENT_ERROR</code> - Unsupported pad size along the x axis
 *                <code>ARM_MATH_SUCCESS</code> - Successful operation
 *
 * @details
 *   - Supported framework : TensorFlow Lite Micro
 *   - The following constrains on the arguments apply
 *      -# Number of input channel equals number of output channels
 *      -# Filter height and width equals 3
 *      -# Padding along x is either 0 or 1.
 *
 */
arm_status arm_depthwise_conv_3x3(const int8_t input_type,
                                  const cmsis_nn_context *ctx,
                                  const cmsis_nn_dw_conv_params *dw_conv_params,
                                  const cmsis_nn_per_channel_quant_params *quant_params,
                                  const cmsis_nn_dims *input_dims,
                                  void  *input,
                                  const cmsis_nn_dims *filter_dims,
                                  const q7_t *kernel,
                                  const cmsis_nn_dims *bias_dims,
                                  const int32_t *bias,
                                  const cmsis_nn_dims *output_dims,
                                  q7_t *output);

/**
 * @brief Optimized s8/u8 depthwise convolution function with constraint that in_channel equals out_channel.
 *        Refer arm_depthwise_conv_s8() for function argument details.
 *
 * @return     The function returns one of the following
 *                <code>ARM_MATH_SIZE_MISMATCH</code> - input channel != output channel or
 *                                                      ch_mult != 1
 *                <code>ARM_MATH_SUCCESS</code> - Successful operation
 *
 * @note       If number of channels is not a multiple of 4, upto 3 elements outside the boundary will be read out
 *             for the following if MVE optimizations(Arm Helium Technology) are used.
 *               - Output shift
 *               - Output multiplier
 *               - Output bias
 *               - kernel
 * @details
 *    - Supported framework: TensorFlow Lite
 *    - The following constrains on the arguments apply
 *        -# Number of input channel equals number of output channels or ch_mult equals 1
 *    - q7 is used as data type eventhough it is s8 data. It is done so to be consistent with existing APIs.
 *    - Reccomended when number of channels is 4 or greater.
 *
 */
arm_status arm_depthwise_conv_opt(const int8_t input_type,
                                  const cmsis_nn_context *ctx,
                                  const cmsis_nn_dw_conv_params *dw_conv_params,
                                  const cmsis_nn_per_channel_quant_params *quant_params,
                                  const cmsis_nn_dims *input_dims,
                                  void  *input,
                                  const cmsis_nn_dims *filter_dims,
                                  const q7_t *kernel,
                                  const cmsis_nn_dims *bias_dims,
                                  const int32_t *bias,
                                  const cmsis_nn_dims *output_dims,
                                  q7_t *output) ;


void arm_q7_to_q15_with_offset(const q7_t *src, q15_t *dst, uint32_t block_size, q15_t offset);
void arm_u7_to_q15_with_offset(const u7_t *src, q15_t *dst, uint32_t block_size, q15_t offset);
#endif
