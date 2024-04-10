#ifdef MAC
#include <stdio.h>
#endif
#include "layer_cnn_functions.h"

#define DIM_KER_X (1U)
#define DIM_KER_Y (1U)

/*
 * 1xN s8 convolution function.
 *
 * Refer header file for details.
 *
 */
arm_status arm_convolve_1_x_n(const int8_t input_type,
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
  arm_status status = ARM_MATH_SUCCESS;
  if (output_dims->w % 4 != 0) {
    status = ARM_MATH_SIZE_MISMATCH;
    goto out;
  }

  status = arm_convolve(input_type, ctx, conv_params, quant_params, input_dims,
                        (q7_t*)input_data, filter_dims, filter_data,
                        bias_dims, bias_data, output_dims, output_data);

out:
  /* Return to application */
  return status;
}

/*
 * Fast s8 version for 1x1 convolution (non-square shape)
 *
 * Refer header file for details.
 *
 */
arm_status arm_convolve_1x1_fast(const int8_t input_type,
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
                                 q7_t *output_data) {
  (void)ctx;
  (void)filter_dims;
  (void)bias_dims;
  if (input_dims->c % 4 != 0 || conv_params->padding.w != 0 || conv_params->padding.h != 0 ||
      conv_params->stride.w != 1 || conv_params->stride.h != 1) {
    return ARM_MATH_SIZE_MISMATCH;
  }

  /* Run the following code as reference implementation for Cortex-M processors with or without DSP extension */
  const int32_t lhs_rows = input_dims->w * input_dims->h * input_dims->n;
  const int32_t rhs_rows = output_dims->c;
  const int32_t rhs_cols = input_dims->c;

  if (input_type == 1) { // q7
    arm_nn_mat_mult_nt_t_s8((q7_t*)input_data, filter_data, bias_data, output_data,
                            quant_params->shift, lhs_rows, rhs_rows, rhs_cols,
                            conv_params->input_offset, conv_params->output_offset,
                            conv_params->activation.min, conv_params->activation.max);
  } else { // u7
    arm_nn_mat_mult_nt_t_u8((u7_t*)input_data, filter_data, bias_data, output_data,
                            quant_params->shift, lhs_rows, rhs_rows, rhs_cols,
                            conv_params->input_offset, conv_params->output_offset,
                            conv_params->activation.min, conv_params->activation.max);
  }

  /* Return to application */
  return ARM_MATH_SUCCESS;
}

/*
 * Basic s8 convolution function.
 *
 * Refer header file for details. Optimal use case for the DSP/MVE implementation is when input and output channels
 * are multiples of 4 or atleast greater than 4.
 *
 */
arm_status arm_convolve(const int8_t input_type,
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
                        q7_t *output_data) {
  (void)bias_dims;
  q15_t *buffer_a = (q15_t *)ctx->buf;

  const int32_t input_batches = input_dims->n;
  const uint16_t input_x = input_dims->w;
  const uint16_t input_y = input_dims->h;
  const uint16_t input_ch = input_dims->c;
  const uint16_t kernel_x = filter_dims->w;
  const uint16_t kernel_y = filter_dims->h;
  const uint16_t output_x = output_dims->w;
  const uint16_t output_y = output_dims->h;
  const uint16_t output_ch = output_dims->c;

  const uint16_t pad_x = conv_params->padding.w;
  const uint16_t pad_y = conv_params->padding.h;
  const uint16_t stride_x = conv_params->stride.w;
  const uint16_t stride_y = conv_params->stride.h;

  const int32_t input_offset = conv_params->input_offset;
  const int32_t out_offset = conv_params->output_offset;
  const int32_t out_activation_min = conv_params->activation.min;
  const int32_t out_activation_max = conv_params->activation.max;
  int8_t *output_shift = quant_params->shift;

  int i_batch;
  for (i_batch = 0; i_batch < input_batches; i_batch++) {

    const uint16_t dilation_x = conv_params->dilation.w;
    const uint16_t dilation_y = conv_params->dilation.h;

    int32_t i_out_y, i_out_x, i_ker_y, i_ker_x;

    /* Generate two columns from the input tensor a GEMM computation */
    q15_t *two_column_buf = buffer_a;
    q7_t *out = output_data;

    /* This part implements the im2col function */
    for (i_out_y = 0; i_out_y < output_y; i_out_y++) {
      for (i_out_x = 0; i_out_x < output_x; i_out_x++) {
        const int32_t base_idx_y = stride_y * i_out_y - pad_y;
        const int32_t base_idx_x = stride_x * i_out_x - pad_x;

        for (i_ker_y = 0; i_ker_y < kernel_y; i_ker_y++) {
          for (i_ker_x = 0; i_ker_x < kernel_x; i_ker_x++) {
            const int32_t k_y = base_idx_y + dilation_y * i_ker_y;
            const int32_t k_x = base_idx_x + dilation_x * i_ker_x;

            if (k_y < 0 || k_y >= input_y || k_x < 0 || k_x >= input_x) {
              /* Filling 0 for out-of-bound paddings */
#ifdef MAC
              memset(two_column_buf, 0, sizeof(q15_t) * input_ch);
#elif RISCV
              mymemset(two_column_buf, 0, sizeof(q15_t) * input_ch);
#elif ARM_MATH_DSP
              memset(two_column_buf, 0, sizeof(q15_t) * input_ch);
#endif
            } else {
              /* Copying the pixel data to column */
              if (input_type) { // q7
                arm_q7_to_q15_with_offset(
                  (q7_t*)input_data + (k_y * input_x + k_x) * input_ch, two_column_buf, input_ch, input_offset);
              } else { // u7
                arm_u7_to_q15_with_offset(
                  (u7_t*)input_data + (k_y * input_x + k_x) * input_ch, two_column_buf, input_ch, input_offset);
              }
            }
            two_column_buf += input_ch;
          }
        }

        /* Computation is filed for every 2 columns */
        if (two_column_buf == buffer_a + 2 * input_ch * kernel_y * kernel_x) {
          out = arm_nn_mat_mult_kernel_s8_s16(filter_data,
                                              buffer_a,
                                              output_ch,
                                              output_shift,
                                              out_offset,
                                              out_activation_min,
                                              out_activation_max,
                                              input_ch * kernel_y * kernel_x,
                                              bias_data,
                                              out);

          /* counter reset */
          two_column_buf = buffer_a;
        }
      }
    }

    /* left-over because odd number of output pixels */
    if (two_column_buf != buffer_a) {
      const q7_t *ker_a = filter_data;
      int i;

      for (i = 0; i < output_ch; i++) {
        /* Load the accumulator with bias first */
        q31_t sum = 0;
        if (bias_data) {
          sum = bias_data[i];
        }

        /* Point to the beginning of the im2col buffer where the input is available as a rearranged column */
        const q15_t *ip_as_col = buffer_a;

        /* 4 multiply and accumulates are done in one loop. */
#if defined(ARM_MATH_DSP)
        uint16_t col_count = (input_ch * kernel_y * kernel_x) >> 2;

        while (col_count) {
          q31_t ker_a1, ker_a2;
          q31_t ip_b1, ip_b2;

          ker_a = read_and_pad(ker_a, &ker_a1, &ker_a2);

          ip_b1 = arm_nn_read_q15x2_ia(&ip_as_col);
          sum = __SMLAD(ker_a1, ip_b1, sum);
          ip_b2 = arm_nn_read_q15x2_ia(&ip_as_col);
          sum = __SMLAD(ker_a2, ip_b2, sum);

          col_count--;
        }
        /* Handle left over mac */
        col_count = input_ch * kernel_y * kernel_x & 0x3;
#else
        uint16_t col_count = input_ch * kernel_y * kernel_x;
#endif
        while (col_count) {
          q7_t ker_a1 = *ker_a++;
          q15_t ip_b1 = *ip_as_col++;
          sum += ker_a1 * ip_b1;
          col_count--;
        }

        sum = arm_nn_requantize(sum, output_shift[i]);
        sum += out_offset;
        sum = MAX(sum, out_activation_min);
        sum = MIN(sum, out_activation_max);
        *out++ = (q7_t)sum;
      }
    }
    /* Advance to the next batch */
    if (input_type) {
      input_data = (void*)((q7_t*)input_data + (input_x * input_y * input_ch));
    } else {
      input_data = (void*)((u7_t*)input_data + (input_x * input_y * input_ch));
    }
    output_data += (output_x * output_y * output_ch);
  }

  /* Return to application */
  return ARM_MATH_SUCCESS;
}


/*
 * Optimized s8 depthwise convolution function with constraint that
 * in_channel == out_channel and kernel_x == kernel_y == 3 with pads at most 1
 *
 *  Refer prototype header file for details.
 *
 */
arm_status arm_depthwise_conv_3x3(const int8_t input_type,
                                  const cmsis_nn_context *ctx,
                                  const cmsis_nn_dw_conv_params *dw_conv_params,
                                  const cmsis_nn_per_channel_quant_params *quant_params,
                                  const cmsis_nn_dims *input_dims,
                                  void *input,
                                  const cmsis_nn_dims *filter_dims,
                                  const q7_t *kernel,
                                  const cmsis_nn_dims *bias_dims,
                                  const int32_t *bias,
                                  const cmsis_nn_dims *output_dims,
                                  q7_t *output) {
  (void)ctx;
  (void)bias_dims;
  const int32_t input_x = input_dims->w;
  const int32_t input_y = input_dims->h;
  const int32_t input_ch = input_dims->c;
  const int32_t output_ch = output_dims->c;
  const int32_t pad_x = dw_conv_params->padding.w;
  const int32_t pad_y = dw_conv_params->padding.h;
  const int32_t stride_x = dw_conv_params->stride.w;
  const int32_t stride_y = dw_conv_params->stride.h;
  const int8_t  *output_shift = quant_params->shift;
  const int32_t output_x = output_dims->w;
  const int32_t output_y = output_dims->h;
  const int32_t output_offset = dw_conv_params->output_offset;
  const int32_t input_offset = dw_conv_params->input_offset;
  const int32_t output_activation_min = dw_conv_params->activation.min;
  const int32_t output_activation_max = dw_conv_params->activation.max;

  /* Check input constraints input_ch == output_ch */
  if (input_ch != output_ch) {
    return ARM_MATH_SIZE_MISMATCH;
  }
  /* Check input constraints pad_x <= 1 */
  if (pad_x > 1 || filter_dims->w != 3 || filter_dims->h != 3) {
    return ARM_MATH_ARGUMENT_ERROR;
  }

  if (input_type) { // q7
    for (int32_t in_h = -pad_y, out_h = 0, out_idx = 0; out_h < output_y; in_h += stride_y, ++out_h) {
      for (int32_t in_w = -pad_x, out_w = 0, ker_h_start = MAX(0, -in_h); out_w < output_x; in_w += stride_x, ++out_w) {
        int32_t in_ch = 0;
        int32_t ker_w_start = MAX(0, -in_w);

        for (; in_ch <= (input_ch - 4); in_ch += 4) {
          int32_t out_buff0 = bias[in_ch + 0];
          int32_t out_buff1 = bias[in_ch + 1];
          int32_t out_buff2 = bias[in_ch + 2];
          int32_t out_buff3 = bias[in_ch + 3];

          const int8_t *input_ptr = (int8_t*)input + (in_h + ker_h_start) * (input_ch * input_x) + in_w * input_ch + in_ch;
          const int8_t *kernel_ptr = kernel + ker_h_start * (input_ch * 3) + in_ch;

          for (int32_t ker_h = ker_h_start; ker_h < MIN(3, input_y - in_h); ++ker_h) {
            int32_t in_val = 0;
            int32_t ker_val = 0;

            if (ker_w_start == 0) {
              in_val = arm_nn_read_q7x4(input_ptr);
              ker_val = arm_nn_read_q7x4(kernel_ptr);

              out_buff0 += ((int8_t)in_val + input_offset) * (int8_t)ker_val;
              out_buff1 += ((int8_t)(in_val >> 8) + input_offset) * (int8_t)(ker_val >> 8);
              out_buff2 += ((int8_t)(in_val >> 16) + input_offset) * (int8_t)(ker_val >> 16);
              out_buff3 += ((int8_t)(in_val >> 24) + input_offset) * (int8_t)(ker_val >> 24);
            }

            in_val = arm_nn_read_q7x4(input_ptr + input_ch);
            ker_val = arm_nn_read_q7x4(kernel_ptr + input_ch);

            out_buff0 += ((int8_t)in_val + input_offset) * (int8_t)ker_val;
            out_buff1 += ((int8_t)(in_val >> 8) + input_offset) * (int8_t)(ker_val >> 8);
            out_buff2 += ((int8_t)(in_val >> 16) + input_offset) * (int8_t)(ker_val >> 16);
            out_buff3 += ((int8_t)(in_val >> 24) + input_offset) * (int8_t)(ker_val >> 24);

            if ((input_x - in_w) >= 3) {
              in_val = arm_nn_read_q7x4(input_ptr + (input_ch << 1));
              ker_val = arm_nn_read_q7x4(kernel_ptr + (input_ch << 1));

              out_buff0 += ((int8_t)in_val + input_offset) * (int8_t)ker_val;
              out_buff1 += ((int8_t)(in_val >> 8) + input_offset) * (int8_t)(ker_val >> 8);
              out_buff2 += ((int8_t)(in_val >> 16) + input_offset) * (int8_t)(ker_val >> 16);
              out_buff3 += ((int8_t)(in_val >> 24) + input_offset) * (int8_t)(ker_val >> 24);
            }

            input_ptr += (input_ch * input_x);
            kernel_ptr += (input_ch * 3);
          }

          out_buff0 = arm_nn_requantize(out_buff0, output_shift[in_ch + 0]);
          out_buff1 = arm_nn_requantize(out_buff1, output_shift[in_ch + 1]);
          out_buff2 = arm_nn_requantize(out_buff2, output_shift[in_ch + 2]);
          out_buff3 = arm_nn_requantize(out_buff3, output_shift[in_ch + 3]);

          out_buff0 += output_offset;
          out_buff1 += output_offset;
          out_buff2 += output_offset;
          out_buff3 += output_offset;

          out_buff0 = MIN(MAX(out_buff0, output_activation_min), output_activation_max);
          out_buff1 = MIN(MAX(out_buff1, output_activation_min), output_activation_max);
          out_buff2 = MIN(MAX(out_buff2, output_activation_min), output_activation_max);
          out_buff3 = MIN(MAX(out_buff3, output_activation_min), output_activation_max);

          output[out_idx++] = (int8_t)out_buff0;
          output[out_idx++] = (int8_t)out_buff1;
          output[out_idx++] = (int8_t)out_buff2;
          output[out_idx++] = (int8_t)out_buff3;
        }

        // Leftover
        for (; in_ch < input_ch; ++in_ch) {
          int32_t out_buff = bias[in_ch];

          const int8_t *input_ptr = (int8_t*)input + (in_h + ker_h_start) * (input_ch * input_x) + in_w * input_ch + in_ch;
          const int8_t *kernel_ptr = kernel + ker_h_start * (input_ch * 3) + in_ch;

          for (int32_t ker_h = ker_h_start; ker_h < MIN(3, input_y - in_h); ++ker_h) {
            if (ker_w_start == 0) {
              out_buff += (*(input_ptr) + input_offset) * *(kernel_ptr);
            }

            out_buff += (*(input_ptr + input_ch) + input_offset) * *(kernel_ptr + input_ch);

            if ((input_x - in_w) >= 3) {
              out_buff += (*(input_ptr + (input_ch << 1)) + input_offset) * *(kernel_ptr + (input_ch << 1));
            }

            input_ptr += (input_ch * input_x);
            kernel_ptr += (input_ch * 3);
          }

          out_buff = arm_nn_requantize(out_buff, output_shift[in_ch]);
          out_buff += output_offset;
          out_buff = MIN(MAX(out_buff, output_activation_min), output_activation_max);
          output[out_idx++] = (int8_t)out_buff;
        }
      }
    }
  } else { // u7
    for (int32_t in_h = -pad_y, out_h = 0, out_idx = 0; out_h < output_y; in_h += stride_y, ++out_h) {
      for (int32_t in_w = -pad_x, out_w = 0, ker_h_start = MAX(0, -in_h); out_w < output_x; in_w += stride_x, ++out_w) {
        int32_t in_ch = 0;
        int32_t ker_w_start = MAX(0, -in_w);

        for (; in_ch <= (input_ch - 4); in_ch += 4) {
          int32_t out_buff0 = bias[in_ch + 0];
          int32_t out_buff1 = bias[in_ch + 1];
          int32_t out_buff2 = bias[in_ch + 2];
          int32_t out_buff3 = bias[in_ch + 3];

          const uint8_t *input_ptr = (uint8_t*)input + (in_h + ker_h_start) * (input_ch * input_x) + in_w * input_ch + in_ch;
          const int8_t *kernel_ptr = kernel + ker_h_start * (input_ch * 3) + in_ch;

          for (int32_t ker_h = ker_h_start; ker_h < MIN(3, input_y - in_h); ++ker_h) {
            uint32_t in_val = 0;
            int32_t ker_val = 0;

            if (ker_w_start == 0) {
              in_val = arm_nn_read_u7x4(input_ptr);
              ker_val = arm_nn_read_q7x4(kernel_ptr);

              out_buff0 += ((uint8_t)in_val + input_offset) * (int8_t)ker_val;
              out_buff1 += ((uint8_t)(in_val >> 8) + input_offset) * (int8_t)(ker_val >> 8);
              out_buff2 += ((uint8_t)(in_val >> 16) + input_offset) * (int8_t)(ker_val >> 16);
              out_buff3 += ((uint8_t)(in_val >> 24) + input_offset) * (int8_t)(ker_val >> 24);
            }

            in_val = arm_nn_read_u7x4(input_ptr + input_ch);
            ker_val = arm_nn_read_q7x4(kernel_ptr + input_ch);

            out_buff0 += ((uint8_t)in_val + input_offset) * (int8_t)ker_val;
            out_buff1 += ((uint8_t)(in_val >> 8) + input_offset) * (int8_t)(ker_val >> 8);
            out_buff2 += ((uint8_t)(in_val >> 16) + input_offset) * (int8_t)(ker_val >> 16);
            out_buff3 += ((uint8_t)(in_val >> 24) + input_offset) * (int8_t)(ker_val >> 24);

            if ((input_x - in_w) >= 3) {
              in_val = arm_nn_read_u7x4(input_ptr + (input_ch << 1));
              ker_val = arm_nn_read_q7x4(kernel_ptr + (input_ch << 1));

              out_buff0 += ((uint8_t)in_val + input_offset) * (int8_t)ker_val;
              out_buff1 += ((uint8_t)(in_val >> 8) + input_offset) * (int8_t)(ker_val >> 8);
              out_buff2 += ((uint8_t)(in_val >> 16) + input_offset) * (int8_t)(ker_val >> 16);
              out_buff3 += ((uint8_t)(in_val >> 24) + input_offset) * (int8_t)(ker_val >> 24);
            }

            input_ptr += (input_ch * input_x);
            kernel_ptr += (input_ch * 3);
          }

          out_buff0 = arm_nn_requantize(out_buff0, output_shift[in_ch + 0]);
          out_buff1 = arm_nn_requantize(out_buff1, output_shift[in_ch + 1]);
          out_buff2 = arm_nn_requantize(out_buff2, output_shift[in_ch + 2]);
          out_buff3 = arm_nn_requantize(out_buff3, output_shift[in_ch + 3]);

          out_buff0 += output_offset;
          out_buff1 += output_offset;
          out_buff2 += output_offset;
          out_buff3 += output_offset;

          out_buff0 = MIN(MAX(out_buff0, output_activation_min), output_activation_max);
          out_buff1 = MIN(MAX(out_buff1, output_activation_min), output_activation_max);
          out_buff2 = MIN(MAX(out_buff2, output_activation_min), output_activation_max);
          out_buff3 = MIN(MAX(out_buff3, output_activation_min), output_activation_max);

          output[out_idx++] = (int8_t)out_buff0;
          output[out_idx++] = (int8_t)out_buff1;
          output[out_idx++] = (int8_t)out_buff2;
          output[out_idx++] = (int8_t)out_buff3;
        }

        // Leftover
        for (; in_ch < input_ch; ++in_ch) {
          int32_t out_buff = bias[in_ch];

          const uint8_t *input_ptr = (uint8_t*)input + (in_h + ker_h_start) * (input_ch * input_x) + in_w * input_ch + in_ch;
          const int8_t *kernel_ptr = kernel + ker_h_start * (input_ch * 3) + in_ch;

          for (int32_t ker_h = ker_h_start; ker_h < MIN(3, input_y - in_h); ++ker_h) {
            if (ker_w_start == 0) {
              out_buff += (*(input_ptr) + input_offset) * *(kernel_ptr);
            }

            out_buff += (*(input_ptr + input_ch) + input_offset) * *(kernel_ptr + input_ch);

            if ((input_x - in_w) >= 3) {
              out_buff += (*(input_ptr + (input_ch << 1)) + input_offset) * *(kernel_ptr + (input_ch << 1));
            }

            input_ptr += (input_ch * input_x);
            kernel_ptr += (input_ch * 3);
          }

          out_buff = arm_nn_requantize(out_buff, output_shift[in_ch]);
          out_buff += output_offset;
          out_buff = MIN(MAX(out_buff, output_activation_min), output_activation_max);
          output[out_idx++] = (int8_t)out_buff;
        }
      }
    }

  }

  /* Return to application */
  return ARM_MATH_SUCCESS;
}


static void depthwise_conv_mult_4(const int8_t input_type,
                                  void *input,
                                  const int32_t input_x,
                                  const int32_t input_y,
                                  const int32_t input_ch,
                                  const int8_t *kernel,
                                  const int32_t output_ch,
                                  const int32_t ch_mult,
                                  const int32_t kernel_x,
                                  const int32_t kernel_y,
                                  const int32_t pad_x,
                                  const int32_t pad_y,
                                  const int32_t stride_x,
                                  const int32_t stride_y,
                                  const int32_t *bias,
                                  int8_t *output,
                                  const int8_t *output_shift,
                                  const int32_t output_x,
                                  const int32_t output_y,
                                  const int32_t output_offset,
                                  const int32_t input_offset,
                                  const int32_t output_activation_min,
                                  const int32_t output_activation_max) {
  for (int32_t in_h = -pad_y, out_h = 0, out_idx = 0; out_h < output_y; in_h += stride_y, ++out_h) {
    for (int32_t in_w = -pad_x, out_w = 0, ker_h_start = MAX(0, -in_h); out_w < output_x; in_w += stride_x, ++out_w) {
      for (int32_t in_ch = 0, out_ch = 0, ker_w_start = MAX(0, -in_w); out_ch < output_ch;
           ++in_ch, out_ch += ch_mult) {
        for (int mult_tile = 0; mult_tile < ch_mult; mult_tile += 4) {
          int32_t out_buff[4] = {0, 0, 0, 0};
          if (bias) {
            out_buff[0] = bias[out_ch + 0 + mult_tile];
            out_buff[1] = bias[out_ch + 1 + mult_tile];
            out_buff[2] = bias[out_ch + 2 + mult_tile];
            out_buff[3] = bias[out_ch + 3 + mult_tile];
          }

          for (int32_t ker_h = ker_h_start; ker_h < MIN(kernel_y, input_y - in_h); ++ker_h) {
            int32_t ker_idx = ker_h * (output_ch * kernel_x) + ker_w_start * output_ch + out_ch;
            int32_t in_idx = (in_h + ker_h) * (input_ch * input_x) + in_w * input_ch + in_ch;

            for (int32_t ker_w = ker_w_start; ker_w < MIN(kernel_x, input_x - in_w);
                 ++ker_w, ker_idx += output_ch) {
              int32_t in_val = (input_type ? ((q7_t*)input)[in_idx + ker_w * input_ch]  : ((u7_t*)input)[in_idx + ker_w * input_ch]) + input_offset;
              out_buff[0] += in_val * kernel[ker_idx + 0 + mult_tile];
              out_buff[1] += in_val * kernel[ker_idx + 1 + mult_tile];
              out_buff[2] += in_val * kernel[ker_idx + 2 + mult_tile];
              out_buff[3] += in_val * kernel[ker_idx + 3 + mult_tile];
            }
          }

          out_buff[0] = arm_nn_requantize(out_buff[0], output_shift[out_ch + 0 + mult_tile]);
          out_buff[1] = arm_nn_requantize(out_buff[1], output_shift[out_ch + 1 + mult_tile]);
          out_buff[2] = arm_nn_requantize(out_buff[2], output_shift[out_ch + 2 + mult_tile]);
          out_buff[3] = arm_nn_requantize(out_buff[3], output_shift[out_ch + 3 + mult_tile]);

          out_buff[0] += output_offset;
          out_buff[1] += output_offset;
          out_buff[2] += output_offset;
          out_buff[3] += output_offset;

          out_buff[0] = MIN(MAX(out_buff[0], output_activation_min), output_activation_max);
          out_buff[1] = MIN(MAX(out_buff[1], output_activation_min), output_activation_max);
          out_buff[2] = MIN(MAX(out_buff[2], output_activation_min), output_activation_max);
          out_buff[3] = MIN(MAX(out_buff[3], output_activation_min), output_activation_max);

          output[out_idx++] = (int8_t)out_buff[0];
          output[out_idx++] = (int8_t)out_buff[1];
          output[out_idx++] = (int8_t)out_buff[2];
          output[out_idx++] = (int8_t)out_buff[3];

        }
      }
    }
  }
}

static void depthwise_conv_generic(const int8_t input_type,
                                   void  *input,
                                   const uint16_t input_batches,
                                   const uint16_t input_x,
                                   const uint16_t input_y,
                                   const uint16_t input_ch,
                                   const q7_t *kernel,
                                   const uint16_t output_ch,
                                   const uint16_t ch_mult,
                                   const uint16_t kernel_x,
                                   const uint16_t kernel_y,
                                   const uint16_t pad_x,
                                   const uint16_t pad_y,
                                   const uint16_t stride_x,
                                   const uint16_t stride_y,
                                   const int32_t *bias,
                                   q7_t *output,
                                   const int8_t *output_shift,
                                   const uint16_t output_x,
                                   const uint16_t output_y,
                                   const int32_t output_offset,
                                   const int32_t input_offset,
                                   const int32_t output_activation_min,
                                   const int32_t output_activation_max,
                                   const uint16_t dilation_x,
                                   const uint16_t dilation_y) {
  (void)output_ch;
  int i_out = 0;
  int i_batch;

  for (i_batch = 0; i_batch < input_batches; i_batch++) {
    for (int i_out_y = 0; i_out_y < output_y; i_out_y++) {
      const int16_t base_idx_y = (i_out_y * stride_y) - pad_y;
      for (int i_out_x = 0; i_out_x < output_x; i_out_x++) {
        const int16_t base_idx_x = (i_out_x * stride_x) - pad_x;
        for (int i_input_ch = 0; i_input_ch < input_ch; i_input_ch++) {
          for (int i_ch_mult = 0; i_ch_mult < ch_mult; i_ch_mult++) {
            const int idx_out_ch = i_ch_mult + i_input_ch * ch_mult;
            int32_t acc_0 = 0;

            int ker_y_start;
            int ker_x_start;
            int ker_y_end;
            int ker_x_end;

            if (dilation_x > 1) {
              const int32_t start_x_max = (-base_idx_x + dilation_x - 1) / dilation_x;
              ker_x_start = MAX(0, start_x_max);
              const int32_t end_min_x = (input_x - base_idx_x + dilation_x - 1) / dilation_x;
              ker_x_end = MIN(kernel_x, end_min_x);
            } else {
              ker_x_start = MAX(0, -base_idx_x);
              ker_x_end = MIN(kernel_x, input_x - base_idx_x);
            }

            if (dilation_y > 1) {
              const int32_t start_y_max = (-base_idx_y + dilation_y - 1) / dilation_y;
              ker_y_start = MAX(0, start_y_max);
              const int32_t end_min_y = (input_y - base_idx_y + dilation_y - 1) / dilation_y;
              ker_y_end = MIN(kernel_y, end_min_y);
            } else {
              ker_y_start = MAX(0, -base_idx_y);
              ker_y_end = MIN(kernel_y, input_y - base_idx_y);
            }

            if (bias) {
              acc_0 = bias[idx_out_ch];
            }

            for (int i_ker_y = ker_y_start; i_ker_y < ker_y_end; i_ker_y++) {
              const int32_t idx_y = base_idx_y + dilation_y * i_ker_y;
              for (int i_ker_x = ker_x_start; i_ker_x < ker_x_end; i_ker_x++) {
                const int32_t idx_x = base_idx_x + dilation_x * i_ker_x;
                int32_t idx_0 = (idx_y * input_x + idx_x) * input_ch + i_input_ch;
                int32_t ker_idx_0 = (i_ker_y * kernel_x + i_ker_x) * (input_ch * ch_mult) + idx_out_ch;

                acc_0 += ((input_type ? ((q7_t*)input)[idx_0] : ((u7_t*)input)[idx_0]) + input_offset) * kernel[ker_idx_0];
              }
            }

            /* Requantize and clamp output to provided range */
            acc_0 = arm_nn_requantize(acc_0, output_shift[idx_out_ch]);
            acc_0 += output_offset;
            acc_0 = MAX(acc_0, output_activation_min);
            acc_0 = MIN(acc_0, output_activation_max);

            output[i_out++] = acc_0;
          }
        }
      }
    }
    /* Advance to the next batch */
    if (input_type) {
      input = (void*)((q7_t*)input + (input_x * input_y * input_ch));
    } else {
      input = (void*)((u7_t*)input + (input_x * input_y * input_ch));
    }
  }
}

/*
 *  Basic s8 depthwise convolution function.
 *
 *  Refer header file for details.
 *  Optimization using DSP extension is not available for the generic case where channel multiplier is > 1.
 *
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
                              q7_t *output) {
  (void)ctx;
  (void)bias_dims;
  const uint16_t dilation_x = dw_conv_params->dilation.w;
  const uint16_t dilation_y = dw_conv_params->dilation.h;

  if (dw_conv_params->ch_mult % 4 == 0 && input_dims->n == 1 && dw_conv_params->dilation.w == 1 &&
      dw_conv_params->dilation.h == 1) {
    depthwise_conv_mult_4(input_type,
                          (void*)input,
                          input_dims->w,
                          input_dims->h,
                          input_dims->c,
                          kernel,
                          output_dims->c,
                          dw_conv_params->ch_mult,
                          filter_dims->w,
                          filter_dims->h,
                          dw_conv_params->padding.w,
                          dw_conv_params->padding.h,
                          dw_conv_params->stride.w,
                          dw_conv_params->stride.h,
                          bias,
                          output,
                          quant_params->shift,
                          output_dims->w,
                          output_dims->h,
                          dw_conv_params->output_offset,
                          dw_conv_params->input_offset,
                          dw_conv_params->activation.min,
                          dw_conv_params->activation.max);
  } else {
    depthwise_conv_generic(input_type,
                           (void*)input,
                           input_dims->n,
                           input_dims->w,
                           input_dims->h,
                           input_dims->c,
                           kernel,
                           output_dims->c,
                           dw_conv_params->ch_mult,
                           filter_dims->w,
                           filter_dims->h,
                           dw_conv_params->padding.w,
                           dw_conv_params->padding.h,
                           dw_conv_params->stride.w,
                           dw_conv_params->stride.h,
                           bias,
                           output,
                           quant_params->shift,
                           output_dims->w,
                           output_dims->h,
                           dw_conv_params->output_offset,
                           dw_conv_params->input_offset,
                           dw_conv_params->activation.min,
                           dw_conv_params->activation.max,
                           dilation_x,
                           dilation_y);
  }

  /* Return to application */
  return ARM_MATH_SUCCESS;
}

/*
 * Optimized s8 depthwise convolution function with constraint that in_channel equals out_channel
 *
 *  Refer prototype header file for details.
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
                                  q7_t *output) {

  const int32_t input_ch = input_dims->c;
  const int32_t output_ch = output_dims->c;

  /* Check input constraints input_ch == output_ch */
  if (input_ch != output_ch) {
    return ARM_MATH_SIZE_MISMATCH;
  }


#ifdef ARM_MATH_DSP
  const int32_t input_x = input_dims->w;
  const int32_t input_y = input_dims->h;
  const int32_t kernel_x = filter_dims->w;
  const int32_t kernel_y = filter_dims->h;
  const int32_t pad_x = dw_conv_params->padding.w;
  const int32_t pad_y = dw_conv_params->padding.h;
  const int32_t stride_x = dw_conv_params->stride.w;
  const int32_t stride_y = dw_conv_params->stride.h;
  const int8_t *output_shift = quant_params->shift;
  const int32_t output_x = output_dims->w;
  const int32_t output_y = output_dims->h;
  const int32_t output_offset = dw_conv_params->output_offset;
  const int32_t input_offset = dw_conv_params->input_offset;
  const int32_t output_activation_min = dw_conv_params->activation.min;
  const int32_t output_activation_max = dw_conv_params->activation.max;
  q15_t *buffer_a = (q15_t *)ctx->buf;

  (void)bias_dims;
  /* Run the following code in cores using DSP extension */
  q15_t *const col_buffer_start = buffer_a;
  q15_t *col_buffer = col_buffer_start;
  const int32_t *const bias_start_pos = bias;
  const q7_t *const out_shift_start_pos = output_shift;
  uint16_t row_count;
  uint16_t row_shift;

  for (int i_out_y = 0; i_out_y < output_y; i_out_y++) {
    const int16_t base_idx_y = (i_out_y * stride_y) - pad_y;
    for (int i_out_x = 0; i_out_x < output_x; i_out_x++) {
      const int16_t base_idx_x = (i_out_x * stride_x) - pad_x;

      /* Out of bounds is only considered for the y axis as it provides a contiguous zero'ing opportunity than
         along the x axis */
      const int ker_y_start = MAX(0, -base_idx_y);
      /* Condition for kernel end dimension: (base_idx_y + ker_y_end) < input_y */
      const int ker_y_end = MIN(kernel_y, input_y - base_idx_y);

      int32_t index = 0;
      if (ker_y_start != 0) {
        memset(&col_buffer[index], 0, (kernel_x * input_ch) * ker_y_start * sizeof(q15_t));
        index += (kernel_x * input_ch) * ker_y_start;
      }

      for (int i_ker_y = ker_y_start; i_ker_y < ker_y_end; i_ker_y++) {
        const int32_t idx_y = base_idx_y + i_ker_y;

        for (int i_ker_x = 0; i_ker_x < kernel_x; i_ker_x++) {
          const int32_t idx_x = base_idx_x + i_ker_x;
          if (idx_x < 0 || idx_x >= input_x) {
            memset(&col_buffer[index], 0, input_ch * sizeof(q15_t));
          } else {
            if (input_type) { // q7
              arm_q7_to_q15_with_offset((q7_t *)input + (idx_y * input_x + idx_x) * input_ch,
                                        &col_buffer[index], input_ch, input_offset);
            } else { // u7
              arm_u7_to_q15_with_offset((u7_t *)input + (idx_y * input_x + idx_x) * input_ch,
                                        &col_buffer[index], input_ch, input_offset);
            }
          }
          index += input_ch;
        }
      }

      const int diff = kernel_y - ker_y_end;
      if (diff != 0) {
        memset(&col_buffer[index], 0, (kernel_x * input_ch) * diff * sizeof(q15_t));
      }

      row_count = output_ch / 4;
      row_shift = 0;
      bias = bias_start_pos;
      output_shift = out_shift_start_pos;

      while (row_count) {
        q31_t sum = *bias++;
        q31_t sum_2 = *bias++;
        q31_t sum_3 = *bias++;
        q31_t sum_4 = *bias++;

        uint16_t col_count = (kernel_x * kernel_y) / 2;
        q15_t *col_pos = col_buffer_start + row_shift;
        const q7_t *row_pos = kernel + row_shift;
        row_shift += 4;

        while (col_count) {
          /* General idea is to read 4 + 4 (input, kernel) pair and re-arrange them in the right order to
          use in a SMLAD instruction . One run of this loop produces 4 partial outputs with 8 MACs. */
          /* Note: variable names can be improved here to align with rows and columns. */
          q31_t ip_a1, ip_a2, ip_b1, ip_b2, op_a, op_b, op_c;
          /* Read 4 weights */
          ip_b1 = arm_nn_read_q7x4(row_pos);
          ip_a1 = arm_nn_read_q7x4(row_pos + input_ch);
          op_a = arm_nn_read_q15x2(col_pos);
          op_b = arm_nn_read_q15x2(col_pos + input_ch);

          ip_a2 = __SXTB16(ip_b1);
          ip_b1 = __SXTB16(__ROR(ip_b1, 8));

          ip_b2 = __SXTB16(ip_a1);
          ip_a1 = __SXTB16(__ROR(ip_a1, 8));

          op_c = __PKHBT(op_b, op_a, 16);
          op_a = __PKHTB(op_b, op_a, 16);
          op_b = __PKHBT(ip_b2, ip_a2, 16);
          sum = __SMLAD(op_c, op_b, sum);

          op_b = __PKHBT(ip_b1, ip_a1, 16);
          sum_2 = __SMLAD(op_a, op_b, sum_2);

          op_a = arm_nn_read_q15x2(col_pos + 2);
          op_b = arm_nn_read_q15x2(col_pos + input_ch + 2);

          op_c = __PKHBT(op_b, op_a, 16);
          op_a = __PKHTB(op_b, op_a, 16);
          op_b = __PKHTB(ip_a2, ip_b2, 16);
          sum_3 = __SMLAD(op_c, op_b, sum_3);

          op_b = __PKHTB(ip_a1, ip_b1, 16);
          sum_4 = __SMLAD(op_a, op_b, sum_4);

          row_pos += input_ch << 1;
          col_pos += input_ch << 1;
          col_count--;
        }

        col_count = (kernel_x * kernel_y) & 0x1;
        while (col_count) {
          sum += row_pos[0] * col_pos[0];
          sum_2 += row_pos[1] * col_pos[1];
          sum_3 += row_pos[2] * col_pos[2];
          sum_4 += row_pos[3] * col_pos[3];

          row_pos += input_ch;
          col_pos += input_ch;

          col_count--;
        }
        sum = arm_nn_requantize(sum, *output_shift++);
        sum += output_offset;
        sum = MAX(sum, output_activation_min);
        sum = MIN(sum, output_activation_max);
        *output++ = (q7_t)sum;

        sum_2 = arm_nn_requantize(sum_2, *output_shift++);
        sum_2 += output_offset;
        sum_2 = MAX(sum_2, output_activation_min);
        sum_2 = MIN(sum_2, output_activation_max);
        *output++ = (q7_t)sum_2;
        sum_3 = arm_nn_requantize(sum_3, *output_shift++);
        sum_3 += output_offset;
        sum_3 = MAX(sum_3, output_activation_min);
        sum_3 = MIN(sum_3, output_activation_max);
        *output++ = (q7_t)sum_3;

        sum_4 = arm_nn_requantize(sum_4, *output_shift++);
        sum_4 += output_offset;
        sum_4 = MAX(sum_4, output_activation_min);
        sum_4 = MIN(sum_4, output_activation_max);
        *output++ = (q7_t)sum_4;

        row_count--;
      }

      row_count = output_ch & 0x3;
      while (row_count) {
        q15_t *col_pos = col_buffer_start + row_shift;
        const q7_t *row_pos = kernel + row_shift;
        q31_t sum = *bias++;
        const uint16_t col_count = (kernel_x * kernel_y);
        row_shift += 1;

        for (int i = 0; i < col_count; i++) {
          sum += row_pos[i * input_ch] * col_pos[i * input_ch];
        }
        sum = arm_nn_requantize(sum, *output_shift++);
        sum += output_offset;
        sum = MAX(sum, output_activation_min);
        sum = MIN(sum, output_activation_max);
        *output++ = (q7_t)sum;

        row_count--;
      }

      // clear counter and pointers
      col_buffer = col_buffer_start;
    }
  }
#else
  /* Run the following code as reference implementation for Cortex-M0 and Cortex-M3 */
  return arm_depthwise_conv(input_type,
                            ctx,
                            dw_conv_params,
                            quant_params,
                            input_dims,
                            (void*)input,
                            filter_dims,
                            kernel,
                            bias_dims,
                            bias,
                            output_dims,
                            output);
#endif /* ARM_MATH_MVEI | ARM_MATH_DSP */

  /* Return to application */
  return ARM_MATH_SUCCESS;
}
