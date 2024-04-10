#ifndef __LAYER_CNN_SUPPORTS_H__
#define __LAYER_CNN_SUPPORTS_H__
#ifdef MAC
#include <string.h>
#elif RISCV
#include "malloc.h"
#elif ARM_MATH_DSP
#include <string.h>
#endif
#include "layer_types.h"

#ifdef ARM_MATH_DSP
#include "layer_arm_cc.h"
#endif

#ifndef   __STATIC_FORCEINLINE
#ifdef ARM_MATH_DSP
#define __STATIC_FORCEINLINE                   static __forceinline
#else
#define __STATIC_FORCEINLINE                   static inline
#endif // end dsp
#endif // end static_forceline

#ifndef LEFT_SHIFT
#define LEFT_SHIFT(_shift) (_shift > 0 ? _shift : 0)
#endif

#ifndef RIGHT_SHIFT
#define RIGHT_SHIFT(_shift) (_shift > 0 ? 0 : -_shift)
#endif

#ifndef MAX
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#endif

#ifndef MIN
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#endif

#ifndef CLAMP
#define CLAMP(x, h, l) MAX(MIN((x), (h)), (l))
#endif


/**
  @brief         Read 2 q15 from q15 pointer.
  @param[in]     in_q15   pointer to address of input.
  @return        q31 value
 */
__STATIC_FORCEINLINE q31_t arm_nn_read_q15x2(const q15_t *in_q15) {
  q31_t val;
#ifdef MAC
  memcpy(&val, in_q15, 4);
#elif RISCV
  mymemcpy(&val, (void*)in_q15, 4);
#elif ARM_MATH_DSP
  memcpy(&val, in_q15, 4);
#endif
  return (val);
}

/**
  @brief         Read 2 u15 from u15 pointer.
  @param[in]     in_q15   pointer to address of input.
  @return        q31 value
 */
__STATIC_FORCEINLINE u31_t arm_nn_read_u15x2(const u15_t *in_u15) {
  u31_t val;
#ifdef MAC
  memcpy(&val, in_u15, 4);
#elif RISCV
  mymemcpy(&val, (void*)in_u15, 4);
#elif ARM_MATH_DSP
  memcpy(&val, in_u15, 4);
#endif

  return (val);
}

/**
  @brief         Read 4 q7 values.
  @param[in]     in_q7       pointer to address of input.
  @return        q31 value
 */
__STATIC_FORCEINLINE q31_t arm_nn_read_q7x4(const q7_t *in_q7) {
  q31_t val;
#ifdef MAC
  memcpy(&val, in_q7, 4);
#elif RISCV
  mymemcpy(&val, (void*)in_q7, 4);
#elif ARM_MATH_DSP
  memcpy(&val, in_q7, 4);
#endif
  return (val);
}


/**
  @brief         Read 4 q7 values.
  @param[in]     in_q7       pointer to address of input.
  @return        q31 value
 */
__STATIC_FORCEINLINE u31_t arm_nn_read_u7x4(const u7_t *in_u7) {
  u31_t val;
#ifdef MAC
  memcpy(&val, in_u7, 4);
#elif RISCV
  mymemcpy(&val, (void*)in_u7, 4);
#elif ARM_MATH_DSP
  memcpy(&val, in_u7, 4);
#endif

  return (val);
}

/**
  @brief         Read 4 q7 from q7 pointer and post increment pointer.
  @param[in]     in_q7       Pointer to pointer that holds address of input.
  @return        q31 value
 */
__STATIC_FORCEINLINE q31_t arm_nn_read_q7x4_ia(const q7_t **in_q7) {
  q31_t val;
#ifdef MAC
  memcpy(&val, *in_q7, 4);
#elif RISCV
  mymemcpy(&val, (void*)*in_q7, 4);
#elif ARM_MATH_DSP
  memcpy(&val, *in_q7, 4);
#endif
  *in_q7 += 4;

  return (val);
}

__STATIC_FORCEINLINE u31_t arm_nn_read_u7x4_ia(const u7_t **in_u7) {
  u31_t val;
#ifdef MAC
  memcpy(&val, *in_u7, 4);
#elif RISCV
  mymemcpy(&val, (void*)*in_u7, 4);
#elif ARM_MATH_DSP
  memcpy(&val, *in_u7, 4);
#endif
  *in_u7 += 4;

  return (val);
}

/**
 * @brief           Requantize a given value.
 * @param[in]       val         Value to be requantized
 * @param[in]       multiplier  multiplier. Range {NN_Q31_MIN + 1, Q32_MAX}
 * @param[in]       shift       left or right shift for 'val * multiplier'
 *
 * @return          Returns (val * multiplier)/(2 ^ shift)
 *
 */
__STATIC_FORCEINLINE q31_t arm_nn_requantize(const q31_t val, const q7_t shift) {
  if (shift > 0) {
    return (val + (1 << (shift - 1))) >> shift;
  }
  return val;
}

__STATIC_FORCEINLINE u31_t arm_nn_requantize_u32(const u31_t val, const q7_t shift) {
  if (shift > 0) {
    return (val + (1 << (shift - 1))) >> shift;
  }
  return val;
}


/**
 * @brief General Matrix-multiplication function with per-channel requantization.
 *        This function assumes:
 *        - LHS input matrix NOT transposed (nt)
 *        - RHS input matrix transposed (t)
 *
 *  @note This operation also performs the broadcast bias addition before the requantization
 *
 * @param[in]  lhs                Pointer to the LHS input matrix
 * @param[in]  rhs                Pointer to the RHS input matrix
 * @param[in]  bias               Pointer to the bias vector. The length of this vector is equal to the number of
 * output columns (or RHS input rows)
 * @param[out] dst                Pointer to the output matrix with "m" rows and "n" columns
 * @param[in]  dst_multipliers    Pointer to the multipliers vector needed for the per-channel requantization.
 *                                The length of this vector is equal to the number of output columns (or RHS input
 * rows)
 * @param[in]  dst_shifts         Pointer to the shifts vector needed for the per-channel requantization. The length
 * of this vector is equal to the number of output columns (or RHS input rows)
 * @param[in]  lhs_rows           Number of LHS input rows
 * @param[in]  rhs_rows           Number of RHS input rows
 * @param[in]  rhs_cols           Number of LHS/RHS input columns
 * @param[in]  lhs_offset         Offset to be applied to the LHS input value
 * @param[in]  dst_offset         Offset to be applied the output result
 * @param[in]  activation_min     Minimum value to clamp down the output. Range : int8
 * @param[in]  activation_max     Maximum value to clamp up the output. Range : int8
 *
 * @return     The function returns <code>ARM_MATH_SUCCESS</code>
 *
 */
arm_status arm_nn_mat_mult_nt_t_s8(const q7_t *lhs,
                                   const q7_t *rhs,
                                   const q31_t *bias,
                                   q7_t *dst,
                                   const int8_t  *dst_shifts,
                                   const int32_t lhs_rows,
                                   const int32_t rhs_rows,
                                   const int32_t rhs_cols,
                                   const int32_t lhs_offset,
                                   const int32_t dst_offset,
                                   const int32_t activation_min,
                                   const int32_t activation_max);

arm_status arm_nn_mat_mult_nt_t_u8(const u7_t *lhs,
                                   const q7_t *rhs,
                                   const q31_t *bias,
                                   q7_t *dst,
                                   const int8_t  *dst_shifts,
                                   const int32_t lhs_rows,
                                   const int32_t rhs_rows,
                                   const int32_t rhs_cols,
                                   const int32_t lhs_offset,
                                   const int32_t dst_offset,
                                   const int32_t activation_min,
                                   const int32_t activation_max);

/**
 * @brief Matrix-multiplication function for convolution with per-channel requantization.
 * @param[in]       input_a     pointer to operand A
 * @param[in]       input_b     pointer to operand B, always consists of 2 vectors.
 * @param[in]       output_ch   number of rows of A
 * @param[in]       out_shift  pointer to per output channel requantization shift parameter.
 * @param[in]       out_offset      output tensor offset.
 * @param[in]       activation_min   minimum value to clamp the output to. Range : int8
 * @param[in]       activation_max   maximum value to clamp the output to. Range : int8
 * @param[in]       num_col_a   number of columns of A
 * @param[in]       output_bias per output channel bias. Range : int32
 * @param[in,out]   out_0       pointer to output
 * @return     The function returns one of the two
 *              1. The incremented output pointer for a successful operation or
 *              2. NULL if implementation is not available.
 *
 * @details   This function does the matrix multiplication of weight matrix for all output channels
 *            with 2 columns from im2col and produces two elements/output_channel. The outputs are
 *            clamped in the range provided by activation min and max.
 *            Supported framework: TensorFlow Lite micro.
 */
q7_t *arm_nn_mat_mult_kernel_s8_s16(const q7_t *input_a,
                                    const q15_t *input_b,
                                    const uint16_t output_ch,
                                    const int8_t *out_shift,
                                    const int32_t out_offset,
                                    const int16_t activation_min,
                                    const int16_t activation_max,
                                    const uint16_t num_col_a,
                                    const int32_t *const output_bias,
                                    q7_t *out_0);

q7_t *arm_nn_mat_mult_kernel_u8_s16(const u7_t *input_a,
                                    const q15_t *input_b,
                                    const uint16_t output_ch,
                                    const int8_t *out_shift,
                                    const int32_t out_offset,
                                    const int16_t activation_min,
                                    const int16_t activation_max,
                                    const uint16_t num_col_a,
                                    const int32_t *const output_bias,
                                    q7_t *out_0);
/**
 * @brief Converts the elements from a q7 vector to a q15 vector with an added offset
 * @param[in]    src        pointer to the q7 input vector
 * @param[out]   dst        pointer to the q15 output vector
 * @param[in]    block_size length of the input vector
 * @param[in]    offset     q7 offset to be added to each input vector element.
 *
 * \par Description:
 *
 * The equation used for the conversion process is:
 *
 * <pre>
 *  dst[n] = (q15_t) src[n] + offset;   0 <= n < block_size.
 * </pre>
 *
 */
void arm_q7_to_q15_with_offset(const q7_t *src, q15_t *dst, uint32_t block_size, q15_t offset);


#ifdef ARM_MATH_DSP
__STATIC_FORCEINLINE const q7_t *read_and_pad(const q7_t *source, q31_t *out1, q31_t *out2) {
  q31_t inA = arm_nn_read_q7x4_ia(&source);
  q31_t inAbuf1 = __SXTB16_RORn((uint32_t)inA, 8);
  q31_t inAbuf2 = __SXTB16(inA); // 这两行操作用来把32位中的4个8位word分别扩展成了16位

#ifndef ARM_MATH_BIG_ENDIAN
  *out2 = (int32_t)(__PKHTB(inAbuf1, inAbuf2, 16));
  *out1 = (int32_t)(__PKHBT(inAbuf2, inAbuf1, 16));
#else
  /* 相当于一个封装，把原来一个word中分为两个区间的部分变成了一个区间，低位的进行了位移操作 */
  *out1 = (int32_t)(__PKHTB(inAbuf1, inAbuf2, 16));
  *out2 = (int32_t)(__PKHBT(inAbuf2, inAbuf1, 16));
#endif

  return source;
}

__STATIC_FORCEINLINE const u7_t *read_and_pad_u7(const u7_t *source, u31_t *out1, u31_t *out2) {
  u31_t inA = arm_nn_read_u7x4_ia(&source);
  u31_t inAbuf1 = __UXTB16_RORn((uint32_t)inA, 8);
  u31_t inAbuf2 = __UXTB16(inA); // 这两行操作用来把32位中的4个8位word分别扩展成了16位

#ifndef ARM_MATH_BIG_ENDIAN
  *out2 = (uint32_t)(__PKHTB(inAbuf1, inAbuf2, 16));
  *out1 = (uint32_t)(__PKHBT(inAbuf2, inAbuf1, 16));
#else
  /* 相当于一个封装，把原来一个word中分为两个区间的部分变成了一个区间，低位的进行了位移操作 */
  *out1 = (uint32_t)(__PKHTB(inAbuf1, inAbuf2, 16));
  *out2 = (uint32_t)(__PKHBT(inAbuf2, inAbuf1, 16));
#endif

  return source;
}

/**
  @brief         Write 2 q15 elements and post increment pointer.
  @param[in]     dest_q15  Pointer to pointer that holds address of destination.
  @param[in]     src_q31   Input value to be written.
 */
__STATIC_FORCEINLINE void arm_nn_write_q15x2_ia(q15_t **dest_q15, q31_t src_q31) {
  q31_t val = src_q31;
#ifdef MAC
  memcpy(*dest_q15, &val, 4);
#elif RISCV
  mymemcpy(*dest_q15, &val, 4);
#elif ARM_MATH_DSP
  memcpy(*dest_q15, &val, 4);
#endif

  *dest_q15 += 2;
}

/**
  @brief         Write 2 u15 elements and post increment pointer.
  @param[in]     dest_q15  Pointer to pointer that holds address of destination.
  @param[in]     src_q31   Input value to be written.
 */
__STATIC_FORCEINLINE void arm_nn_write_u15x2_ia(u15_t **dest_u15, u31_t src_u31) {
  u31_t val = src_u31;
#ifdef MAC
  memcpy(*dest_u15, &val, 4);
#elif RISCV
  mymemcpy(*dest_u15, &val, 4);
#elif ARM_MATH_DSP
  memcpy(*dest_u15, &val, 4);
#endif
  *dest_u15 += 2;
}

__STATIC_FORCEINLINE q31_t arm_nn_read_q15x2_ia(const q15_t **in_q15) {
  q31_t val;
#ifdef MAC
  memcpy(&val, *in_q15, 4);
#elif RISCV
  mymemcpy(&val, *in_q15, 4);
#elif ARM_MATH_DSP
  memcpy(&val, *in_q15, 4);
#endif

  *in_q15 += 2;

  return (val);
}

#endif
#endif