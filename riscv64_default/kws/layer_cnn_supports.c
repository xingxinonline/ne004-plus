#include "layer_cnn_supports.h"

//=====================START===========================
/*
 * s8 matrix multiplication with the right-hand-side matrix transposed
 *
 * Refer header file for details.
 *
 */
arm_status arm_nn_mat_mult_nt_t_s8(const q7_t *lhs,
                                   const q7_t *rhs,
                                   const q31_t *bias,
                                   q7_t *dst,
                                   const int8_t *dst_shifts,
                                   const int32_t lhs_rows,
                                   const int32_t rhs_rows,
                                   const int32_t rhs_cols,
                                   const int32_t lhs_offset,
                                   const int32_t dst_offset,
                                   const int32_t activation_min,
                                   const int32_t activation_max) {
#if defined(ARM_MATH_DSP)
  const int32_t off0 = rhs_cols - 4;

  for (int32_t rhs_rows_idx = 0; rhs_rows_idx <= (rhs_rows - 2); rhs_rows_idx += 2) {
    const q7_t *lhs_ptr = &lhs[0];
    q7_t *dst_ptr = &dst[0];

    q31_t lhs_offset_contribution0 = 0;
    q31_t lhs_offset_contribution1 = 0;

    for (int32_t x = 0; x < rhs_cols; ++x) {
      lhs_offset_contribution0 += rhs[x];
      lhs_offset_contribution1 += rhs[x + rhs_cols];
    }

    lhs_offset_contribution0 *= lhs_offset;
    lhs_offset_contribution1 *= lhs_offset;
    if (bias) {
      lhs_offset_contribution0 += bias[rhs_rows_idx];
      lhs_offset_contribution1 += bias[rhs_rows_idx + 1];
    }

    int32_t lhs_rows_idx = lhs_rows >> 1;

    while (lhs_rows_idx) {
      const q7_t *rhs_ptr = &rhs[0];

      q31_t res00 = lhs_offset_contribution0;
      q31_t res01 = lhs_offset_contribution1;
      q31_t res10 = lhs_offset_contribution0;
      q31_t res11 = lhs_offset_contribution1;

      int32_t rhs_cols_idx = 0;

      q31_t val0, val1, val2, val3, val4, val5;

      for (; rhs_cols_idx <= (rhs_cols - 16); rhs_cols_idx += 16) {
        val1 = arm_nn_read_q7x4_ia((const q7_t **)&rhs_ptr);
        val2 = __SXTB16(val1);
        val0 = arm_nn_read_q7x4_ia((const q7_t **)&lhs_ptr);
        val3 = __SXTB16(val0);
        val4 = arm_nn_read_q7x4((const q7_t *)&rhs_ptr[off0]);
        val1 = __SXTB16_RORn(val1, 8);
        val0 = __SXTB16_RORn(val0, 8);

        // 4 x MAC res00, res01
        res00 = __SMLAD(val3, val2, res00);
        val5 = __SXTB16(val4);
        res00 = __SMLAD(val0, val1, res00);
        val4 = __SXTB16_RORn(val4, 8);
        res01 = __SMLAD(val3, val5, res01);
        res01 = __SMLAD(val0, val4, res01);

        // 4 x MAC res10, res11
        val0 = arm_nn_read_q7x4((const q7_t *)&lhs_ptr[off0]);
        val3 = __SXTB16(val0);
        val0 = __SXTB16_RORn(val0, 8);
        res10 = __SMLAD(val3, val2, res10);
        res11 = __SMLAD(val3, val5, res11);
        res10 = __SMLAD(val0, val1, res10);
        val1 = arm_nn_read_q7x4_ia((const q7_t **)&rhs_ptr);
        res11 = __SMLAD(val0, val4, res11);

        val4 = arm_nn_read_q7x4((const q7_t *)&rhs_ptr[off0]);
        val2 = __SXTB16(val1);
        val0 = arm_nn_read_q7x4_ia((const q7_t **)&lhs_ptr);
        val3 = __SXTB16(val0);
        val1 = __SXTB16_RORn(val1, 8);
        val0 = __SXTB16_RORn(val0, 8);

        // 4 x MAC res00, res01
        res00 = __SMLAD(val3, val2, res00);
        val5 = __SXTB16(val4);
        res00 = __SMLAD(val0, val1, res00);
        val4 = __SXTB16_RORn(val4, 8);
        res01 = __SMLAD(val3, val5, res01);
        res01 = __SMLAD(val0, val4, res01);

        // 4 x MAC res10, res11
        val0 = arm_nn_read_q7x4((const q7_t *)&lhs_ptr[off0]);
        val3 = __SXTB16(val0);
        val0 = __SXTB16_RORn(val0, 8);
        res10 = __SMLAD(val3, val2, res10);
        res11 = __SMLAD(val3, val5, res11);
        res10 = __SMLAD(val0, val1, res10);
        val1 = arm_nn_read_q7x4_ia((const q7_t **)&rhs_ptr);
        res11 = __SMLAD(val0, val4, res11);

        val4 = arm_nn_read_q7x4((const q7_t *)&rhs_ptr[off0]);
        val2 = __SXTB16(val1);
        val0 = arm_nn_read_q7x4_ia((const q7_t **)&lhs_ptr);
        val3 = __SXTB16(val0);
        val1 = __SXTB16_RORn(val1, 8);
        val0 = __SXTB16_RORn(val0, 8);

        // 4 x MAC res00, res01
        res00 = __SMLAD(val3, val2, res00);
        val5 = __SXTB16(val4);
        res00 = __SMLAD(val0, val1, res00);
        val4 = __SXTB16_RORn(val4, 8);
        res01 = __SMLAD(val3, val5, res01);
        res01 = __SMLAD(val0, val4, res01);

        // 4 x MAC res10, res11
        val0 = arm_nn_read_q7x4((const q7_t *)&lhs_ptr[off0]);
        val3 = __SXTB16(val0);
        val0 = __SXTB16_RORn(val0, 8);
        res10 = __SMLAD(val3, val2, res10);
        res11 = __SMLAD(val3, val5, res11);
        res10 = __SMLAD(val0, val1, res10);
        val1 = arm_nn_read_q7x4_ia((const q7_t **)&rhs_ptr);
        res11 = __SMLAD(val0, val4, res11);

        val4 = arm_nn_read_q7x4((const q7_t *)&rhs_ptr[off0]);
        val2 = __SXTB16(val1);
        val0 = arm_nn_read_q7x4_ia((const q7_t **)&lhs_ptr);
        val3 = __SXTB16(val0);
        val1 = __SXTB16_RORn(val1, 8);
        val0 = __SXTB16_RORn(val0, 8);

        // 4 x MAC res00, res01
        res00 = __SMLAD(val3, val2, res00);
        val5 = __SXTB16(val4);
        res00 = __SMLAD(val0, val1, res00);
        val4 = __SXTB16_RORn(val4, 8);
        res01 = __SMLAD(val3, val5, res01);
        res01 = __SMLAD(val0, val4, res01);

        // 4 x MAC res10, res11
        val0 = arm_nn_read_q7x4((const q7_t *)&lhs_ptr[off0]);
        val3 = __SXTB16(val0);
        val0 = __SXTB16_RORn(val0, 8);
        res10 = __SMLAD(val3, val2, res10);
        res11 = __SMLAD(val3, val5, res11);
        res10 = __SMLAD(val0, val1, res10);
        res11 = __SMLAD(val0, val4, res11);
      }

      for (; rhs_cols_idx < rhs_cols; ++rhs_cols_idx) {
        q7_t rhs_value0 = rhs_ptr[0];
        q7_t rhs_value1 = rhs_ptr[rhs_cols];
        q7_t lhs_value = lhs_ptr[0];

        res00 += lhs_value * rhs_value0;
        res01 += lhs_value * rhs_value1;

        lhs_value = lhs_ptr[rhs_cols];
        res10 += lhs_value * rhs_value0;
        res11 += lhs_value * rhs_value1;

        ++rhs_ptr;
        ++lhs_ptr;
      }

      // Quantize down
      res00 = arm_nn_requantize(res00, dst_shifts[rhs_rows_idx]);
      res01 = arm_nn_requantize(res01, dst_shifts[rhs_rows_idx + 1]);
      res10 = arm_nn_requantize(res10, dst_shifts[rhs_rows_idx]);
      res11 = arm_nn_requantize(res11, dst_shifts[rhs_rows_idx + 1]);

      // Add offset
      res00 += dst_offset;
      res01 += dst_offset;
      res10 += dst_offset;
      res11 += dst_offset;

      // Clamp the result
      res00 = MAX(res00, activation_min);
      res00 = MIN(res00, activation_max);
      res01 = MAX(res01, activation_min);
      res01 = MIN(res01, activation_max);
      res10 = MAX(res10, activation_min);
      res10 = MIN(res10, activation_max);
      res11 = MAX(res11, activation_min);
      res11 = MIN(res11, activation_max);

      dst_ptr[0] = (q7_t)res00;
      dst_ptr[1] = (q7_t)res01;
      dst_ptr += rhs_rows;
      dst_ptr[0] = (q7_t)res10;
      dst_ptr[1] = (q7_t)res11;
      dst_ptr += rhs_rows;

      lhs_ptr += rhs_cols;

      lhs_rows_idx--;
    }

    // Left-over rows
    if (lhs_rows % 2) {
      const q7_t *rhs_ptr = &rhs[0];

      q31_t res00 = lhs_offset_contribution0;
      q31_t res01 = lhs_offset_contribution1;

      int32_t rhs_cols_idx = 0;

      q31_t val0, val1, val2, val3, val4, val5;
      for (; rhs_cols_idx <= (rhs_cols - 16); rhs_cols_idx += 16) {
        val0 = arm_nn_read_q7x4_ia((const q7_t **)&rhs_ptr);
        val1 = arm_nn_read_q7x4((const q7_t *)&rhs_ptr[off0]);
        val2 = arm_nn_read_q7x4_ia((const q7_t **)&lhs_ptr);
        val3 = __SXTB16(val0);
        val5 = __SXTB16(val2);
        val4 = __SXTB16(val1);
        val0 = __SXTB16_RORn(val0, 8);
        val2 = __SXTB16_RORn(val2, 8);
        val1 = __SXTB16_RORn(val1, 8);

        // 4 x MAC res00, res01
        res00 = __SMLAD(val5, val3, res00);
        res00 = __SMLAD(val2, val0, res00);
        res01 = __SMLAD(val5, val4, res01);
        res01 = __SMLAD(val2, val1, res01);

        val0 = arm_nn_read_q7x4_ia((const q7_t **)&rhs_ptr);
        val1 = arm_nn_read_q7x4((const q7_t *)&rhs_ptr[off0]);
        val2 = arm_nn_read_q7x4_ia((const q7_t **)&lhs_ptr);
        val3 = __SXTB16(val0);
        val5 = __SXTB16(val2);
        val4 = __SXTB16(val1);
        val0 = __SXTB16_RORn(val0, 8);
        val2 = __SXTB16_RORn(val2, 8);
        val1 = __SXTB16_RORn(val1, 8);

        // 4 x MAC res00, res01
        res00 = __SMLAD(val5, val3, res00);
        res00 = __SMLAD(val2, val0, res00);
        res01 = __SMLAD(val5, val4, res01);
        res01 = __SMLAD(val2, val1, res01);

        val0 = arm_nn_read_q7x4_ia((const q7_t **)&rhs_ptr);
        val1 = arm_nn_read_q7x4((const q7_t *)&rhs_ptr[off0]);
        val2 = arm_nn_read_q7x4_ia((const q7_t **)&lhs_ptr);
        val3 = __SXTB16(val0);
        val5 = __SXTB16(val2);
        val4 = __SXTB16(val1);
        val0 = __SXTB16_RORn(val0, 8);
        val2 = __SXTB16_RORn(val2, 8);
        val1 = __SXTB16_RORn(val1, 8);

        // 4 x MAC res00, res01
        res00 = __SMLAD(val5, val3, res00);
        res00 = __SMLAD(val2, val0, res00);
        res01 = __SMLAD(val5, val4, res01);
        res01 = __SMLAD(val2, val1, res01);

        val0 = arm_nn_read_q7x4_ia((const q7_t **)&rhs_ptr);
        val1 = arm_nn_read_q7x4((const q7_t *)&rhs_ptr[off0]);
        val2 = arm_nn_read_q7x4_ia((const q7_t **)&lhs_ptr);
        val3 = __SXTB16(val0);
        val5 = __SXTB16(val2);
        val4 = __SXTB16(val1);
        val0 = __SXTB16_RORn(val0, 8);
        val2 = __SXTB16_RORn(val2, 8);
        val1 = __SXTB16_RORn(val1, 8);

        // 4 x MAC res00, res01
        res00 = __SMLAD(val5, val3, res00);
        res00 = __SMLAD(val2, val0, res00);
        res01 = __SMLAD(val5, val4, res01);
        res01 = __SMLAD(val2, val1, res01);
      }

      // Left-over accumulations
      for (; rhs_cols_idx < rhs_cols; ++rhs_cols_idx) {
        q7_t rhs_value0 = rhs_ptr[0];
        q7_t rhs_value1 = rhs_ptr[rhs_cols];
        q7_t lhs_value = lhs_ptr[0];

        res00 += lhs_value * rhs_value0;
        res01 += lhs_value * rhs_value1;

        ++rhs_ptr;
        ++lhs_ptr;
      }

      // Quantize down
      res00 = arm_nn_requantize(res00, dst_shifts[rhs_rows_idx]);
      res01 = arm_nn_requantize(res01, dst_shifts[rhs_rows_idx + 1]);

      // Add offset
      res00 += dst_offset;
      res01 += dst_offset;

      // Clamp the result
      res00 = MAX(res00, activation_min);
      res00 = MIN(res00, activation_max);
      res01 = MAX(res01, activation_min);
      res01 = MIN(res01, activation_max);

      dst_ptr[0] = (q7_t)res00;
      dst_ptr[1] = (q7_t)res01;
    }

    rhs += 2 * rhs_cols;
    dst += 2;
  }

  if (rhs_rows % 2) {
    const q7_t *lhs_ptr = &lhs[0];
    q7_t *dst_ptr = &dst[0];

    for (int32_t lhs_rows_idx = 0; lhs_rows_idx < lhs_rows; ++lhs_rows_idx) {
      const q7_t *rhs_ptr = &rhs[0];
      q31_t res00 = 0;
      if (bias) {
        res00 = bias[rhs_rows - 1];
      }

      for (int32_t rhs_cols_idx = 0; rhs_cols_idx < rhs_cols; ++rhs_cols_idx) {
        q31_t rhs_value = rhs_ptr[0];
        q31_t lhs_value = lhs_ptr[0] + lhs_offset;

        res00 += lhs_value * rhs_value;

        ++rhs_ptr;
        ++lhs_ptr;
      }

      // Quantize down
      res00 = arm_nn_requantize(res00, dst_shifts[rhs_rows - 1]);

      // Add offset
      res00 += dst_offset;

      // Clamp the result
      res00 = MAX(res00, activation_min);
      res00 = MIN(res00, activation_max);

      dst_ptr[0] = (q7_t)res00;
      dst_ptr += rhs_rows;
    }
  }
#else
  for (int32_t rhs_rows_idx = 0; rhs_rows_idx <= (rhs_rows - 2); rhs_rows_idx += 2) {
    const q7_t *lhs_ptr = &lhs[0];
    q7_t *dst_ptr = &dst[0];

    q31_t lhs_offset_contribution0 = 0;
    q31_t lhs_offset_contribution1 = 0;

    for (int32_t x = 0; x < rhs_cols; ++x) {
      lhs_offset_contribution0 += rhs[x];
      lhs_offset_contribution1 += rhs[x + rhs_cols];
    }

    lhs_offset_contribution0 *= lhs_offset;
    lhs_offset_contribution1 *= lhs_offset;
    if (bias) {
      lhs_offset_contribution0 += bias[rhs_rows_idx];
      lhs_offset_contribution1 += bias[rhs_rows_idx + 1];
    }

    int32_t lhs_rows_idx = lhs_rows >> 1;

    while (lhs_rows_idx) {
      const q7_t *rhs_ptr = &rhs[0];

      q31_t res00 = lhs_offset_contribution0;
      q31_t res01 = lhs_offset_contribution1;
      q31_t res10 = lhs_offset_contribution0;
      q31_t res11 = lhs_offset_contribution1;

      for (int32_t rhs_cols_idx = rhs_cols; rhs_cols_idx != 0; rhs_cols_idx--) {
        q7_t rhs_value0 = rhs_ptr[0];
        q7_t rhs_value1 = rhs_ptr[rhs_cols];
        q7_t lhs_value = lhs_ptr[0];

        res00 += lhs_value * rhs_value0;
        res01 += lhs_value * rhs_value1;

        lhs_value = lhs_ptr[rhs_cols];
        res10 += lhs_value * rhs_value0;
        res11 += lhs_value * rhs_value1;

        ++rhs_ptr;
        ++lhs_ptr;
      }

      // Quantize down
      res00 = arm_nn_requantize(res00, dst_shifts[rhs_rows_idx]);
      res01 = arm_nn_requantize(res01, dst_shifts[rhs_rows_idx + 1]);
      res10 = arm_nn_requantize(res10, dst_shifts[rhs_rows_idx]);
      res11 = arm_nn_requantize(res11, dst_shifts[rhs_rows_idx + 1]);

      // Add offset
      res00 += dst_offset;
      res01 += dst_offset;
      res10 += dst_offset;
      res11 += dst_offset;

      // Clamp the result
      res00 = MAX(res00, activation_min);
      res00 = MIN(res00, activation_max);
      res01 = MAX(res01, activation_min);
      res01 = MIN(res01, activation_max);
      res10 = MAX(res10, activation_min);
      res10 = MIN(res10, activation_max);
      res11 = MAX(res11, activation_min);
      res11 = MIN(res11, activation_max);

      dst_ptr[0] = (q7_t)res00;
      dst_ptr[1] = (q7_t)res01;
      dst_ptr += rhs_rows;
      dst_ptr[0] = (q7_t)res10;
      dst_ptr[1] = (q7_t)res11;
      dst_ptr += rhs_rows;

      lhs_ptr += rhs_cols;

      lhs_rows_idx--;
    }

    // Left-over rows
    if (lhs_rows % 2) {
      const q7_t *rhs_ptr = &rhs[0];

      q31_t res00 = lhs_offset_contribution0;
      q31_t res01 = lhs_offset_contribution1;

      for (int32_t rhs_cols_idx = rhs_cols; rhs_cols_idx != 0; rhs_cols_idx--) {
        q7_t rhs_value0 = rhs_ptr[0];
        q7_t rhs_value1 = rhs_ptr[rhs_cols];
        q7_t lhs_value = lhs_ptr[0];

        res00 += lhs_value * rhs_value0;
        res01 += lhs_value * rhs_value1;

        ++rhs_ptr;
        ++lhs_ptr;
      }

      // Quantize down
      res00 = arm_nn_requantize(res00, dst_shifts[rhs_rows_idx]);
      res01 = arm_nn_requantize(res01, dst_shifts[rhs_rows_idx + 1]);

      // Add offset
      res00 += dst_offset;
      res01 += dst_offset;

      // Clamp the result
      res00 = MAX(res00, activation_min);
      res00 = MIN(res00, activation_max);
      res01 = MAX(res01, activation_min);
      res01 = MIN(res01, activation_max);

      dst_ptr[0] = (q7_t)res00;
      dst_ptr[1] = (q7_t)res01;
    }

    rhs += 2 * rhs_cols;
    dst += 2;
  }

  if (rhs_rows % 2) {
    const q7_t *lhs_ptr = &lhs[0];
    q7_t *dst_ptr = &dst[0];

    for (int32_t lhs_rows_idx = 0; lhs_rows_idx < lhs_rows; ++lhs_rows_idx) {
      const q7_t *rhs_ptr = &rhs[0];
      q31_t res00 = 0;
      if (bias) {
        res00 = bias[rhs_rows - 1];
      }

      for (int32_t rhs_cols_idx = rhs_cols; rhs_cols_idx != 0; rhs_cols_idx--) {
        q31_t rhs_value = rhs_ptr[0];
        q31_t lhs_value = lhs_ptr[0] + lhs_offset;

        res00 += lhs_value * rhs_value;

        ++rhs_ptr;
        ++lhs_ptr;
      }

      // Quantize down
      res00 = arm_nn_requantize(res00, dst_shifts[rhs_rows - 1]);

      // Add offset
      res00 += dst_offset;

      // Clamp the result
      res00 = MAX(res00, activation_min);
      res00 = MIN(res00, activation_max);

      dst_ptr[0] = (q7_t)res00;
      dst_ptr += rhs_rows;
    }
  }
#endif
  return ARM_MATH_SUCCESS;
}

arm_status arm_nn_mat_mult_nt_t_u8(const u7_t *lhs,
                                   const q7_t *rhs,
                                   const q31_t *bias,
                                   q7_t *dst,
                                   const int8_t *dst_shifts,
                                   const int32_t lhs_rows,
                                   const int32_t rhs_rows,
                                   const int32_t rhs_cols,
                                   const int32_t lhs_offset,
                                   const int32_t dst_offset,
                                   const int32_t activation_min,
                                   const int32_t activation_max) {
#if defined(ARM_MATH_DSP)
  const int32_t off0 = rhs_cols - 4;
  // Debug from here
  for (int32_t rhs_rows_idx = 0; rhs_rows_idx <= (rhs_rows - 2); rhs_rows_idx += 2) {
    const u7_t *lhs_ptr = &lhs[0];
    q7_t *dst_ptr = &dst[0];

    q31_t lhs_offset_contribution0 = 0;
    q31_t lhs_offset_contribution1 = 0;

    for (int32_t x = 0; x < rhs_cols; ++x) {
      lhs_offset_contribution0 += rhs[x];
      lhs_offset_contribution1 += rhs[x + rhs_cols];
    }

    lhs_offset_contribution0 *= lhs_offset;
    lhs_offset_contribution1 *= lhs_offset;
    if (bias) {
      lhs_offset_contribution0 += bias[rhs_rows_idx];
      lhs_offset_contribution1 += bias[rhs_rows_idx + 1];
    }

    int32_t lhs_rows_idx = lhs_rows >> 1;

    while (lhs_rows_idx) {
      const q7_t *rhs_ptr = &rhs[0];

      q31_t res00 = lhs_offset_contribution0;
      q31_t res01 = lhs_offset_contribution1;
      q31_t res10 = lhs_offset_contribution0;
      q31_t res11 = lhs_offset_contribution1;

      int32_t rhs_cols_idx = 0;

      u31_t val0, val3;
      q31_t val1, val2, val4, val5;

      for (; rhs_cols_idx <= (rhs_cols - 16); rhs_cols_idx += 16) {
        val1 = arm_nn_read_q7x4_ia((const q7_t **)&rhs_ptr);
        val2 = __SXTB16(val1);
        val0 = arm_nn_read_u7x4_ia((const u7_t **)&lhs_ptr);
        val3 = __UXTB16(val0);
        val4 = arm_nn_read_q7x4((const q7_t *)&rhs_ptr[off0]);
        val1 = __SXTB16_RORn(val1, 8);
        val0 = __UXTB16_RORn(val0, 8);

        // 4 x MAC res00, res01
        res00 = __SMLAD(val3, val2, res00);
        val5 = __SXTB16(val4);
        res00 = __SMLAD(val0, val1, res00);
        val4 = __SXTB16_RORn(val4, 8);
        res01 = __SMLAD(val3, val5, res01);
        res01 = __SMLAD(val0, val4, res01);

        // 4 x MAC res10, res11
        val0 = arm_nn_read_u7x4((const u7_t *)&lhs_ptr[off0]);
        val3 = __UXTB16(val0);
        val0 = __UXTB16_RORn(val0, 8);
        res10 = __SMLAD(val3, val2, res10);
        res11 = __SMLAD(val3, val5, res11);
        res10 = __SMLAD(val0, val1, res10);
        val1 = arm_nn_read_q7x4_ia((const q7_t **)&rhs_ptr);
        res11 = __SMLAD(val0, val4, res11);

        val4 = arm_nn_read_q7x4((const q7_t *)&rhs_ptr[off0]);
        val2 = __SXTB16(val1);
        val0 = arm_nn_read_u7x4_ia((const u7_t **)&lhs_ptr);
        val3 = __UXTB16(val0);
        val1 = __SXTB16_RORn(val1, 8);
        val0 = __UXTB16_RORn(val0, 8);

        // 4 x MAC res00, res01
        res00 = __SMLAD(val3, val2, res00);
        val5 = __SXTB16(val4);
        res00 = __SMLAD(val0, val1, res00);
        val4 = __SXTB16_RORn(val4, 8);
        res01 = __SMLAD(val3, val5, res01);
        res01 = __SMLAD(val0, val4, res01);

        // 4 x MAC res10, res11
        val0 = arm_nn_read_u7x4((const u7_t *)&lhs_ptr[off0]);
        val3 = __UXTB16(val0);
        val0 = __UXTB16_RORn(val0, 8);
        res10 = __SMLAD(val3, val2, res10);
        res11 = __SMLAD(val3, val5, res11);
        res10 = __SMLAD(val0, val1, res10);
        val1 = arm_nn_read_q7x4_ia((const q7_t **)&rhs_ptr);
        res11 = __SMLAD(val0, val4, res11);

        val4 = arm_nn_read_q7x4((const q7_t *)&rhs_ptr[off0]);
        val2 = __SXTB16(val1);
        val0 = arm_nn_read_u7x4_ia((const u7_t **)&lhs_ptr);
        val3 = __UXTB16(val0);
        val1 = __SXTB16_RORn(val1, 8);
        val0 = __UXTB16_RORn(val0, 8);

        // 4 x MAC res00, res01
        res00 = __SMLAD(val3, val2, res00);
        val5 = __SXTB16(val4);
        res00 = __SMLAD(val0, val1, res00);
        val4 = __SXTB16_RORn(val4, 8);
        res01 = __SMLAD(val3, val5, res01);
        res01 = __SMLAD(val0, val4, res01);

        // 4 x MAC res10, res11
        val0 = arm_nn_read_u7x4((const u7_t *)&lhs_ptr[off0]);
        val3 = __UXTB16(val0);
        val0 = __UXTB16_RORn(val0, 8);
        res10 = __SMLAD(val3, val2, res10);
        res11 = __SMLAD(val3, val5, res11);
        res10 = __SMLAD(val0, val1, res10);
        val1 = arm_nn_read_q7x4_ia((const q7_t **)&rhs_ptr);
        res11 = __SMLAD(val0, val4, res11);

        val4 = arm_nn_read_q7x4((const q7_t *)&rhs_ptr[off0]);
        val2 = __SXTB16(val1);
        val0 = arm_nn_read_u7x4_ia((const u7_t **)&lhs_ptr);
        val3 = __UXTB16(val0);
        val1 = __SXTB16_RORn(val1, 8);
        val0 = __UXTB16_RORn(val0, 8);

        // 4 x MAC res00, res01
        res00 = __SMLAD(val3, val2, res00);
        val5 = __SXTB16(val4);
        res00 = __SMLAD(val0, val1, res00);
        val4 = __SXTB16_RORn(val4, 8);
        res01 = __SMLAD(val3, val5, res01);
        res01 = __SMLAD(val0, val4, res01);

        // 4 x MAC res10, res11
        val0 = arm_nn_read_u7x4((const u7_t *)&lhs_ptr[off0]);
        val3 = __UXTB16(val0);
        val0 = __UXTB16_RORn(val0, 8);
        res10 = __SMLAD(val3, val2, res10);
        res11 = __SMLAD(val3, val5, res11);
        res10 = __SMLAD(val0, val1, res10);
        res11 = __SMLAD(val0, val4, res11);
      }

      for (; rhs_cols_idx < rhs_cols; ++rhs_cols_idx) {
        q7_t rhs_value0 = rhs_ptr[0];
        q7_t rhs_value1 = rhs_ptr[rhs_cols];
        u7_t lhs_value = lhs_ptr[0];

        res00 += lhs_value * rhs_value0;
        res01 += lhs_value * rhs_value1;

        lhs_value = lhs_ptr[rhs_cols];
        res10 += lhs_value * rhs_value0;
        res11 += lhs_value * rhs_value1;

        ++rhs_ptr;
        ++lhs_ptr;
      }

      // Quantize down
      res00 = arm_nn_requantize(res00, dst_shifts[rhs_rows_idx]);
      res01 = arm_nn_requantize(res01, dst_shifts[rhs_rows_idx + 1]);
      res10 = arm_nn_requantize(res10, dst_shifts[rhs_rows_idx]);
      res11 = arm_nn_requantize(res11, dst_shifts[rhs_rows_idx + 1]);

      // Add offset
      res00 += dst_offset;
      res01 += dst_offset;
      res10 += dst_offset;
      res11 += dst_offset;

      // Clamp the result
      res00 = MAX(res00, activation_min);
      res00 = MIN(res00, activation_max);
      res01 = MAX(res01, activation_min);
      res01 = MIN(res01, activation_max);
      res10 = MAX(res10, activation_min);
      res10 = MIN(res10, activation_max);
      res11 = MAX(res11, activation_min);
      res11 = MIN(res11, activation_max);

      dst_ptr[0] = (q7_t)res00;
      dst_ptr[1] = (q7_t)res01;
      dst_ptr += rhs_rows;
      dst_ptr[0] = (q7_t)res10;
      dst_ptr[1] = (q7_t)res11;
      dst_ptr += rhs_rows;

      lhs_ptr += rhs_cols;

      lhs_rows_idx--;
    }

    // Left-over rows
    if (lhs_rows % 2) {
      const q7_t *rhs_ptr = &rhs[0];

      q31_t res00 = lhs_offset_contribution0;
      q31_t res01 = lhs_offset_contribution1;

      int32_t rhs_cols_idx = 0;

      u31_t val2;
      q31_t val0, val1, val3, val4, val5;

      for (; rhs_cols_idx <= (rhs_cols - 16); rhs_cols_idx += 16) {
        val0 = arm_nn_read_q7x4_ia((const q7_t **)&rhs_ptr);
        val1 = arm_nn_read_q7x4((const q7_t *)&rhs_ptr[off0]);
        val2 = arm_nn_read_u7x4_ia((const u7_t **)&lhs_ptr);
        val3 = __SXTB16(val0);
        val5 = __UXTB16(val2);
        val4 = __SXTB16(val1);
        val0 = __SXTB16_RORn(val0, 8);
        val2 = __UXTB16_RORn(val2, 8);
        val1 = __SXTB16_RORn(val1, 8);

        // 4 x MAC res00, res01
        res00 = __SMLAD(val5, val3, res00);
        res00 = __SMLAD(val2, val0, res00);
        res01 = __SMLAD(val5, val4, res01);
        res01 = __SMLAD(val2, val1, res01);

        val0 = arm_nn_read_q7x4_ia((const q7_t **)&rhs_ptr);
        val1 = arm_nn_read_q7x4((const q7_t *)&rhs_ptr[off0]);
        val2 = arm_nn_read_u7x4_ia((const u7_t **)&lhs_ptr);
        val3 = __SXTB16(val0);
        val5 = __UXTB16(val2);
        val4 = __SXTB16(val1);
        val0 = __SXTB16_RORn(val0, 8);
        val2 = __UXTB16_RORn(val2, 8);
        val1 = __SXTB16_RORn(val1, 8);

        // 4 x MAC res00, res01
        res00 = __SMLAD(val5, val3, res00);
        res00 = __SMLAD(val2, val0, res00);
        res01 = __SMLAD(val5, val4, res01);
        res01 = __SMLAD(val2, val1, res01);

        val0 = arm_nn_read_q7x4_ia((const q7_t **)&rhs_ptr);
        val1 = arm_nn_read_q7x4((const q7_t *)&rhs_ptr[off0]);
        val2 = arm_nn_read_u7x4_ia((const u7_t **)&lhs_ptr);
        val3 = __SXTB16(val0);
        val5 = __UXTB16(val2);
        val4 = __SXTB16(val1);
        val0 = __SXTB16_RORn(val0, 8);
        val2 = __UXTB16_RORn(val2, 8);
        val1 = __SXTB16_RORn(val1, 8);

        // 4 x MAC res00, res01
        res00 = __SMLAD(val5, val3, res00);
        res00 = __SMLAD(val2, val0, res00);
        res01 = __SMLAD(val5, val4, res01);
        res01 = __SMLAD(val2, val1, res01);

        val0 = arm_nn_read_q7x4_ia((const q7_t **)&rhs_ptr);
        val1 = arm_nn_read_q7x4((const q7_t *)&rhs_ptr[off0]);
        val2 = arm_nn_read_u7x4_ia((const u7_t **)&lhs_ptr);
        val3 = __SXTB16(val0);
        val5 = __UXTB16(val2);
        val4 = __SXTB16(val1);
        val0 = __SXTB16_RORn(val0, 8);
        val2 = __UXTB16_RORn(val2, 8);
        val1 = __SXTB16_RORn(val1, 8);

        // 4 x MAC res00, res01
        res00 = __SMLAD(val5, val3, res00);
        res00 = __SMLAD(val2, val0, res00);
        res01 = __SMLAD(val5, val4, res01);
        res01 = __SMLAD(val2, val1, res01);
      }

      // Left-over accumulations
      for (; rhs_cols_idx < rhs_cols; ++rhs_cols_idx) {
        q7_t rhs_value0 = rhs_ptr[0];
        q7_t rhs_value1 = rhs_ptr[rhs_cols];
        u7_t lhs_value = lhs_ptr[0];

        res00 += lhs_value * rhs_value0;
        res01 += lhs_value * rhs_value1;

        ++rhs_ptr;
        ++lhs_ptr;
      }

      // Quantize down
      res00 = arm_nn_requantize(res00, dst_shifts[rhs_rows_idx]);
      res01 = arm_nn_requantize(res01, dst_shifts[rhs_rows_idx + 1]);

      // Add offset
      res00 += dst_offset;
      res01 += dst_offset;

      // Clamp the result
      res00 = MAX(res00, activation_min);
      res00 = MIN(res00, activation_max);
      res01 = MAX(res01, activation_min);
      res01 = MIN(res01, activation_max);

      dst_ptr[0] = (q7_t)res00;
      dst_ptr[1] = (q7_t)res01;
    }

    rhs += 2 * rhs_cols;
    dst += 2;
  }

  if (rhs_rows % 2) {
    const u7_t *lhs_ptr = &lhs[0];
    q7_t *dst_ptr = &dst[0];

    for (int32_t lhs_rows_idx = 0; lhs_rows_idx < lhs_rows; ++lhs_rows_idx) {
      const q7_t *rhs_ptr = &rhs[0];
      q31_t res00 = 0;
      if (bias) {
        res00 = bias[rhs_rows - 1];
      }

      for (int32_t rhs_cols_idx = 0; rhs_cols_idx < rhs_cols; ++rhs_cols_idx) {
        q31_t rhs_value = rhs_ptr[0];
        u31_t lhs_value = lhs_ptr[0] + lhs_offset;

        res00 += lhs_value * rhs_value;

        ++rhs_ptr;
        ++lhs_ptr;
      }

      // Quantize down
      res00 = arm_nn_requantize(res00, dst_shifts[rhs_rows - 1]);

      // Add offset
      res00 += dst_offset;

      // Clamp the result
      res00 = MAX(res00, activation_min);
      res00 = MIN(res00, activation_max);

      dst_ptr[0] = (q7_t)res00;
      dst_ptr += rhs_rows;
    }
  }
#else
  for (int32_t rhs_rows_idx = 0; rhs_rows_idx <= (rhs_rows - 2); rhs_rows_idx += 2) {
    const u7_t *lhs_ptr = &lhs[0];
    q7_t *dst_ptr = &dst[0];

    q31_t lhs_offset_contribution0 = 0;
    q31_t lhs_offset_contribution1 = 0;

    for (int32_t x = 0; x < rhs_cols; ++x) {
      lhs_offset_contribution0 += rhs[x];
      lhs_offset_contribution1 += rhs[x + rhs_cols];
    }

    lhs_offset_contribution0 *= lhs_offset;
    lhs_offset_contribution1 *= lhs_offset;
    if (bias) {
      lhs_offset_contribution0 += bias[rhs_rows_idx];
      lhs_offset_contribution1 += bias[rhs_rows_idx + 1];
    }

    int32_t lhs_rows_idx = lhs_rows >> 1;

    while (lhs_rows_idx) {
      const q7_t *rhs_ptr = &rhs[0];

      q31_t res00 = lhs_offset_contribution0;
      q31_t res01 = lhs_offset_contribution1;
      q31_t res10 = lhs_offset_contribution0;
      q31_t res11 = lhs_offset_contribution1;

      for (int32_t rhs_cols_idx = rhs_cols; rhs_cols_idx != 0; rhs_cols_idx--) {
        q7_t rhs_value0 = rhs_ptr[0];
        q7_t rhs_value1 = rhs_ptr[rhs_cols];
        u7_t lhs_value = lhs_ptr[0];

        res00 += lhs_value * rhs_value0;
        res01 += lhs_value * rhs_value1;

        lhs_value = lhs_ptr[rhs_cols];
        res10 += lhs_value * rhs_value0;
        res11 += lhs_value * rhs_value1;

        ++rhs_ptr;
        ++lhs_ptr;
      }

      // Quantize down
      res00 = arm_nn_requantize(res00, dst_shifts[rhs_rows_idx]);
      res01 = arm_nn_requantize(res01, dst_shifts[rhs_rows_idx + 1]);
      res10 = arm_nn_requantize(res10, dst_shifts[rhs_rows_idx]);
      res11 = arm_nn_requantize(res11, dst_shifts[rhs_rows_idx + 1]);

      // Add offset
      res00 += dst_offset;
      res01 += dst_offset;
      res10 += dst_offset;
      res11 += dst_offset;

      // Clamp the result
      res00 = MAX(res00, activation_min);
      res00 = MIN(res00, activation_max);
      res01 = MAX(res01, activation_min);
      res01 = MIN(res01, activation_max);
      res10 = MAX(res10, activation_min);
      res10 = MIN(res10, activation_max);
      res11 = MAX(res11, activation_min);
      res11 = MIN(res11, activation_max);

      dst_ptr[0] = (q7_t)res00;
      dst_ptr[1] = (q7_t)res01;
      dst_ptr += rhs_rows;
      dst_ptr[0] = (q7_t)res10;
      dst_ptr[1] = (q7_t)res11;
      dst_ptr += rhs_rows;

      lhs_ptr += rhs_cols;

      lhs_rows_idx--;
    }

    // Left-over rows
    if (lhs_rows % 2) {
      const q7_t *rhs_ptr = &rhs[0];

      q31_t res00 = lhs_offset_contribution0;
      q31_t res01 = lhs_offset_contribution1;

      for (int32_t rhs_cols_idx = rhs_cols; rhs_cols_idx != 0; rhs_cols_idx--) {
        q7_t rhs_value0 = rhs_ptr[0];
        q7_t rhs_value1 = rhs_ptr[rhs_cols];
        u7_t lhs_value = lhs_ptr[0];

        res00 += lhs_value * rhs_value0;
        res01 += lhs_value * rhs_value1;

        ++rhs_ptr;
        ++lhs_ptr;
      }

      // Quantize down
      res00 = arm_nn_requantize(res00, dst_shifts[rhs_rows_idx]);
      res01 = arm_nn_requantize(res01, dst_shifts[rhs_rows_idx + 1]);

      // Add offset
      res00 += dst_offset;
      res01 += dst_offset;

      // Clamp the result
      res00 = MAX(res00, activation_min);
      res00 = MIN(res00, activation_max);
      res01 = MAX(res01, activation_min);
      res01 = MIN(res01, activation_max);

      dst_ptr[0] = (q7_t)res00;
      dst_ptr[1] = (q7_t)res01;
    }

    rhs += 2 * rhs_cols;
    dst += 2;
  }

  if (rhs_rows % 2) {
    const u7_t *lhs_ptr = &lhs[0];
    q7_t *dst_ptr = &dst[0];

    for (int32_t lhs_rows_idx = 0; lhs_rows_idx < lhs_rows; ++lhs_rows_idx) {
      const q7_t *rhs_ptr = &rhs[0];
      q31_t res00 = 0;
      if (bias) {
        res00 = bias[rhs_rows - 1];
      }

      for (int32_t rhs_cols_idx = rhs_cols; rhs_cols_idx != 0; rhs_cols_idx--) {
        q31_t rhs_value = rhs_ptr[0];
        u31_t lhs_value = lhs_ptr[0] + lhs_offset;

        res00 += lhs_value * rhs_value;

        ++rhs_ptr;
        ++lhs_ptr;
      }

      // Quantize down
      res00 = arm_nn_requantize(res00, dst_shifts[rhs_rows - 1]);

      // Add offset
      res00 += dst_offset;

      // Clamp the result
      res00 = MAX(res00, activation_min);
      res00 = MIN(res00, activation_max);

      dst_ptr[0] = (q7_t)res00;
      dst_ptr += rhs_rows;
    }
  }
#endif
  return ARM_MATH_SUCCESS;
}

//=====================START===========================

/*
 * Matrix-multiplication function for convolution with per-channel requantization.
 *
 * Refer header file for details.
 *
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
                                    q7_t *out_0) {
  /* set up the second output pointers */
  q7_t *out_1 = out_0 + output_ch;
  const int32_t *bias = output_bias;

  uint16_t row_count = output_ch / 2;
  const q7_t *ip_a0 = input_a;
  /* this loop over rows in A */
  while (row_count) {
    /* setup pointers for B */
    const q15_t *ip_b0 = input_b;
    const q15_t *ip_b1 = ip_b0 + num_col_a;

    /* align the second pointer for A */
    const q7_t *ip_a1 = ip_a0 + num_col_a;

    q31_t ch_0_out_0 = 0;
    q31_t ch_0_out_1 = 0;
    q31_t ch_1_out_0 = 0;
    q31_t ch_1_out_1 = 0;
    /* Init accumulator with bias for channel N and N + 1 */
    if (bias) {
      ch_0_out_0 = *bias;
      ch_0_out_1 = *bias++;
      ch_1_out_0 = *bias;
      ch_1_out_1 = *bias++;
    }

#if defined(ARM_MATH_DSP)
    uint16_t col_count = num_col_a / 4;
    /* accumulate over the vector */
    while (col_count) {
      q31_t a01, a02, a11, a12;
      q31_t b0 = arm_nn_read_q15x2_ia(&ip_b0);
      q31_t b1 = arm_nn_read_q15x2_ia(&ip_b1);

      ip_a0 = read_and_pad(ip_a0, &a01, &a02);
      ip_a1 = read_and_pad(ip_a1, &a11, &a12);

      ch_0_out_0 = __SMLAD(a01, b0, ch_0_out_0);
      ch_0_out_1 = __SMLAD(a01, b1, ch_0_out_1);
      ch_1_out_0 = __SMLAD(a11, b0, ch_1_out_0);
      ch_1_out_1 = __SMLAD(a11, b1, ch_1_out_1);

      b0 = arm_nn_read_q15x2_ia(&ip_b0);
      b1 = arm_nn_read_q15x2_ia(&ip_b1);

      ch_0_out_0 = __SMLAD(a02, b0, ch_0_out_0);
      ch_0_out_1 = __SMLAD(a02, b1, ch_0_out_1);
      ch_1_out_0 = __SMLAD(a12, b0, ch_1_out_0);
      ch_1_out_1 = __SMLAD(a12, b1, ch_1_out_1);

      col_count--;
    } /* while over col_count */
    col_count = num_col_a & 0x3;
#else
    uint16_t col_count = num_col_a;
#endif
    while (col_count) {
      q7_t a0 = *ip_a0++;
      q15_t b0 = *ip_b0++;
      q7_t a1 = *ip_a1++;
      q15_t b1 = *ip_b1++;

      ch_0_out_0 += a0 * b0;
      ch_0_out_1 += a0 * b1;
      ch_1_out_0 += a1 * b0;
      ch_1_out_1 += a1 * b1;
      col_count--;
    } /* while over col_count */

    ch_0_out_0 = arm_nn_requantize(ch_0_out_0, *out_shift);
    ch_0_out_0 += out_offset;
    ch_0_out_0 = MAX(ch_0_out_0, activation_min);
    ch_0_out_0 = MIN(ch_0_out_0, activation_max);
    *out_0++ = (q7_t)ch_0_out_0;

    ch_0_out_1 = arm_nn_requantize(ch_0_out_1, *out_shift);
    ch_0_out_1 += out_offset;
    ch_0_out_1 = MAX(ch_0_out_1, activation_min);
    ch_0_out_1 = MIN(ch_0_out_1, activation_max);
    *out_1++ = (q7_t)ch_0_out_1;
    out_shift++;

    ch_1_out_0 = arm_nn_requantize(ch_1_out_0, *out_shift);
    ch_1_out_0 += out_offset;
    ch_1_out_0 = MAX(ch_1_out_0, activation_min);
    ch_1_out_0 = MIN(ch_1_out_0, activation_max);
    *out_0++ = (q7_t)ch_1_out_0;

    ch_1_out_1 = arm_nn_requantize(ch_1_out_1, *out_shift);
    ch_1_out_1 += out_offset;
    ch_1_out_1 = MAX(ch_1_out_1, activation_min);
    ch_1_out_1 = MIN(ch_1_out_1, activation_max);
    *out_1++ = (q7_t)ch_1_out_1;
    out_shift++;

    /* skip row */
    ip_a0 += num_col_a;
    row_count--;
  }

  /* compute the last odd numbered row if any */
  if (output_ch & 0x1) {
    /* setup pointers for B */
    const q15_t *ip_b0 = input_b;
    const q15_t *ip_b1 = ip_b0 + num_col_a;

    q31_t ch_0_out_0 = 0;
    q31_t ch_0_out_1 = 0;

    /* load the bias */
    if (bias) {
      ch_0_out_0 = *bias;
      ch_0_out_1 = *bias++;
    }

#if defined(ARM_MATH_DSP)
    uint16_t col_count = num_col_a >> 2;
    while (col_count) {
      q31_t a01, a02;
      q31_t b0 = arm_nn_read_q15x2_ia(&ip_b0);
      q31_t b1 = arm_nn_read_q15x2_ia(&ip_b1);

      ip_a0 = read_and_pad(ip_a0, &a01, &a02);

      ch_0_out_0 = __SMLAD(a01, b0, ch_0_out_0);
      ch_0_out_1 = __SMLAD(a01, b1, ch_0_out_1);

      b0 = arm_nn_read_q15x2_ia(&ip_b0);
      b1 = arm_nn_read_q15x2_ia(&ip_b1);
      ch_0_out_0 = __SMLAD(a02, b0, ch_0_out_0);
      ch_0_out_1 = __SMLAD(a02, b1, ch_0_out_1);

      col_count--;
    }
    col_count = num_col_a & 0x3;
#else
    uint16_t col_count = num_col_a;
#endif
    while (col_count) {
      q7_t a0 = *ip_a0++;
      q15_t b0 = *ip_b0++;
      q15_t b1 = *ip_b1++;

      ch_0_out_0 += a0 * b0;
      ch_0_out_1 += a0 * b1;
      col_count--;
    }
    ch_0_out_0 = arm_nn_requantize(ch_0_out_0, *out_shift);
    ch_0_out_0 += out_offset;
    ch_0_out_0 = MAX(ch_0_out_0, activation_min);
    ch_0_out_0 = MIN(ch_0_out_0, activation_max);
    *out_0++ = (q7_t)ch_0_out_0;

    ch_0_out_1 = arm_nn_requantize(ch_0_out_1, *out_shift);
    ch_0_out_1 += out_offset;
    ch_0_out_1 = MAX(ch_0_out_1, activation_min);
    ch_0_out_1 = MIN(ch_0_out_1, activation_max);
    *out_1++ = (q7_t)ch_0_out_1;
    out_shift++;
  }

  out_0 += output_ch;

  /* return the new output pointer with offset */
  return out_0;
}

q7_t *arm_nn_mat_mult_kernel_u8_s16(const u7_t *input_a,
                                    const q15_t *input_b,
                                    const uint16_t output_ch,
                                    const int8_t *out_shift,
                                    const int32_t out_offset,
                                    const int16_t activation_min,
                                    const int16_t activation_max,
                                    const uint16_t num_col_a,
                                    const int32_t *const output_bias,
                                    q7_t *out_0) {
  /* set up the second output pointers */
  q7_t *out_1 = out_0 + output_ch;
  const int32_t *bias = output_bias;

  uint16_t row_count = output_ch / 2;
  const u7_t *ip_a0 = input_a;
  /* this loop over rows in A */
  while (row_count) {
    /* setup pointers for B */
    const q15_t *ip_b0 = input_b;
    const q15_t *ip_b1 = ip_b0 + num_col_a;

    /* align the second pointer for A */
    const u7_t *ip_a1 = ip_a0 + num_col_a;

    q31_t ch_0_out_0 = 0;
    q31_t ch_0_out_1 = 0;
    q31_t ch_1_out_0 = 0;
    q31_t ch_1_out_1 = 0;
    /* Init accumulator with bias for channel N and N + 1 */
    if (bias) {
      ch_0_out_0 = *bias;
      ch_0_out_1 = *bias++;
      ch_1_out_0 = *bias;
      ch_1_out_1 = *bias++;
    }

#if defined(ARM_MATH_DSP)
    uint16_t col_count = num_col_a / 4;
    /* accumulate over the vector */
    while (col_count) {
      u31_t a01, a02, a11, a12;
      q31_t b0 = arm_nn_read_q15x2_ia(&ip_b0);
      q31_t b1 = arm_nn_read_q15x2_ia(&ip_b1);

      ip_a0 = read_and_pad_u7(ip_a0, &a01, &a02);
      ip_a1 = read_and_pad_u7(ip_a1, &a11, &a12);

      ch_0_out_0 = __SMLAD(a01, b0, ch_0_out_0);
      ch_0_out_1 = __SMLAD(a01, b1, ch_0_out_1);
      ch_1_out_0 = __SMLAD(a11, b0, ch_1_out_0);
      ch_1_out_1 = __SMLAD(a11, b1, ch_1_out_1);

      b0 = arm_nn_read_q15x2_ia(&ip_b0);
      b1 = arm_nn_read_q15x2_ia(&ip_b1);

      ch_0_out_0 = __SMLAD(a02, b0, ch_0_out_0);
      ch_0_out_1 = __SMLAD(a02, b1, ch_0_out_1);
      ch_1_out_0 = __SMLAD(a12, b0, ch_1_out_0);
      ch_1_out_1 = __SMLAD(a12, b1, ch_1_out_1);

      col_count--;
    } /* while over col_count */
    col_count = num_col_a & 0x3;
#else
    uint16_t col_count = num_col_a;
#endif
    while (col_count) {
      u7_t a0 = *ip_a0++;
      q15_t b0 = *ip_b0++;
      u7_t a1 = *ip_a1++;
      q15_t b1 = *ip_b1++;

      ch_0_out_0 += a0 * b0;
      ch_0_out_1 += a0 * b1;
      ch_1_out_0 += a1 * b0;
      ch_1_out_1 += a1 * b1;
      col_count--;
    } /* while over col_count */

    ch_0_out_0 = arm_nn_requantize(ch_0_out_0, *out_shift);
    ch_0_out_0 += out_offset;
    ch_0_out_0 = MAX(ch_0_out_0, activation_min);
    ch_0_out_0 = MIN(ch_0_out_0, activation_max);
    *out_0++ = (q7_t)ch_0_out_0;

    ch_0_out_1 = arm_nn_requantize(ch_0_out_1, *out_shift);
    ch_0_out_1 += out_offset;
    ch_0_out_1 = MAX(ch_0_out_1, activation_min);
    ch_0_out_1 = MIN(ch_0_out_1, activation_max);
    *out_1++ = (q7_t)ch_0_out_1;
    out_shift++;

    ch_1_out_0 = arm_nn_requantize(ch_1_out_0, *out_shift);
    ch_1_out_0 += out_offset;
    ch_1_out_0 = MAX(ch_1_out_0, activation_min);
    ch_1_out_0 = MIN(ch_1_out_0, activation_max);
    *out_0++ = (q7_t)ch_1_out_0;

    ch_1_out_1 = arm_nn_requantize(ch_1_out_1, *out_shift);
    ch_1_out_1 += out_offset;
    ch_1_out_1 = MAX(ch_1_out_1, activation_min);
    ch_1_out_1 = MIN(ch_1_out_1, activation_max);
    *out_1++ = (q7_t)ch_1_out_1;
    out_shift++;

    /* skip row */
    ip_a0 += num_col_a;
    row_count--;
  }

  /* compute the last odd numbered row if any */
  if (output_ch & 0x1) {
    /* setup pointers for B */
    const q15_t *ip_b0 = input_b;
    const q15_t *ip_b1 = ip_b0 + num_col_a;

    q31_t ch_0_out_0 = 0;
    q31_t ch_0_out_1 = 0;

    /* load the bias */
    if (bias) {
      ch_0_out_0 = *bias;
      ch_0_out_1 = *bias++;
    }

#if defined(ARM_MATH_DSP)
    uint16_t col_count = num_col_a >> 2;
    while (col_count) {
      u31_t a01, a02;
      q31_t b0 = arm_nn_read_q15x2_ia(&ip_b0);
      q31_t b1 = arm_nn_read_q15x2_ia(&ip_b1);

      ip_a0 = read_and_pad_u7(ip_a0, &a01, &a02);

      ch_0_out_0 = __SMLAD(a01, b0, ch_0_out_0);
      ch_0_out_1 = __SMLAD(a01, b1, ch_0_out_1);

      b0 = arm_nn_read_q15x2_ia(&ip_b0);
      b1 = arm_nn_read_q15x2_ia(&ip_b1);
      ch_0_out_0 = __SMLAD(a02, b0, ch_0_out_0);
      ch_0_out_1 = __SMLAD(a02, b1, ch_0_out_1);

      col_count--;
    }
    col_count = num_col_a & 0x3;
#else
    uint16_t col_count = num_col_a;
#endif
    while (col_count) {
      u7_t a0 = *ip_a0++;
      q15_t b0 = *ip_b0++;
      q15_t b1 = *ip_b1++;

      ch_0_out_0 += a0 * b0;
      ch_0_out_1 += a0 * b1;
      col_count--;
    }
    ch_0_out_0 = arm_nn_requantize(ch_0_out_0, *out_shift);
    ch_0_out_0 += out_offset;
    ch_0_out_0 = MAX(ch_0_out_0, activation_min);
    ch_0_out_0 = MIN(ch_0_out_0, activation_max);
    *out_0++ = (q7_t)ch_0_out_0;

    ch_0_out_1 = arm_nn_requantize(ch_0_out_1, *out_shift);
    ch_0_out_1 += out_offset;
    ch_0_out_1 = MAX(ch_0_out_1, activation_min);
    ch_0_out_1 = MIN(ch_0_out_1, activation_max);
    *out_1++ = (q7_t)ch_0_out_1;
    out_shift++;
  }

  out_0 += output_ch;

  /* return the new output pointer with offset */
  return out_0;
}

void arm_q7_to_q15_with_offset(const q7_t *src, q15_t *dst, uint32_t block_size, q15_t offset) {
  int block_cnt;
#if defined(ARM_MATH_DSP)
  /* Run the below code for cores that support SIMD instructions  */
  q31_t in_q7x4;
  q31_t in_q15x2_1;
  q31_t in_q15x2_2;
  q31_t out_q15x2_1;
  q31_t out_q15x2_2;

  /*loop unrolling */
  block_cnt = block_size >> 2;

  /* First part of the processing with loop unrolling.  Compute 4 outputs at a time. */
  const q31_t offset_q15x2 = __PKHBT(offset, offset, 16);
  while (block_cnt > 0) {
    /* convert from q7 to q15 and then store the results in the destination buffer */
    in_q7x4 = arm_nn_read_q7x4_ia(&src);

    /* Extract and sign extend each of the four q7 values to q15 */
    in_q15x2_1 = __SXTAB16(offset_q15x2, __ROR(in_q7x4, 8));
    in_q15x2_2 = __SXTAB16(offset_q15x2, in_q7x4);

    out_q15x2_2 = __PKHTB(in_q15x2_1, in_q15x2_2, 16);
    out_q15x2_1 = __PKHBT(in_q15x2_2, in_q15x2_1, 16);

    arm_nn_write_q15x2_ia(&dst, out_q15x2_1);
    arm_nn_write_q15x2_ia(&dst, out_q15x2_2);

    block_cnt--;
  }
  /* Handle left over samples */
  block_cnt = block_size % 0x4;

#else
  /* Run the below code for Cortex-M0 */
  /* Loop over block_size number of values */
  block_cnt = block_size;
#endif

  while (block_cnt > 0) {
    *dst++ = (q15_t) * src++ + offset;

    /* Decrement the loop counter */
    block_cnt--;
  }
}

void arm_u7_to_q15_with_offset(const u7_t *src, u15_t *dst, uint32_t block_size, q15_t offset) {
  int block_cnt;
#if defined(ARM_MATH_DSP)
  /* Run the below code for cores that support SIMD instructions  */
  u31_t in_u7x4;
  u31_t in_u15x2_1;
  u31_t in_u15x2_2;
  u31_t out_u15x2_1;
  u31_t out_u15x2_2;

  /*loop unrolling */
  block_cnt = block_size >> 2;

  /* First part of the processing with loop unrolling.  Compute 4 outputs at a time. */
  const u31_t offset_u15x2 = __PKHBT(offset, offset, 16);
  while (block_cnt > 0) {
    /* convert from q7 to q15 and then store the results in the destination buffer */
    in_u7x4 = arm_nn_read_u7x4_ia(&src);

    /* Extract and sign extend each of the four q7 values to q15 */
    in_u15x2_1 = __UXTAB16(offset_u15x2, __ROR(in_u7x4, 8));
    in_u15x2_2 = __UXTAB16(offset_u15x2, in_u7x4);

    out_u15x2_2 = __PKHTB(in_u15x2_1, in_u15x2_2, 16);
    out_u15x2_1 = __PKHBT(in_u15x2_2, in_u15x2_1, 16);

    arm_nn_write_u15x2_ia(&dst, out_u15x2_1);
    arm_nn_write_u15x2_ia(&dst, out_u15x2_2);

    block_cnt--;
  }
  /* Handle left over samples */
  block_cnt = block_size % 0x4;

#else
  /* Run the below code for Cortex-M0 */
  /* Loop over block_size number of values */
  block_cnt = block_size;
#endif

  while (block_cnt > 0) {
    *dst++ = (u15_t) * src++ + offset;

    /* Decrement the loop counter */
    block_cnt--;
  }
}