/*
 * Copyright (c) 2014 The WebM project authors. All Rights Reserved.
 * Copyright (c) 2023, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <arm_neon.h>
#include <assert.h>
#include <string.h>

#include "config/aom_config.h"
#include "config/aom_dsp_rtcd.h"

#include "aom/aom_integer.h"
#include "aom_dsp/aom_dsp_common.h"
#include "aom_dsp/aom_filter.h"
#include "aom_dsp/arm/mem_neon.h"
#include "aom_dsp/arm/transpose_neon.h"
#include "aom_ports/mem.h"

#if AOM_ARCH_AARCH64 && \
    (defined(__ARM_FEATURE_DOTPROD) || defined(__ARM_FEATURE_MATMUL_INT8))

DECLARE_ALIGNED(16, static const uint8_t, dot_prod_permute_tbl[48]) = {
  0, 1, 2,  3,  1, 2,  3,  4,  2,  3,  4,  5,  3,  4,  5,  6,
  4, 5, 6,  7,  5, 6,  7,  8,  6,  7,  8,  9,  7,  8,  9,  10,
  8, 9, 10, 11, 9, 10, 11, 12, 10, 11, 12, 13, 11, 12, 13, 14
};

DECLARE_ALIGNED(16, static const uint8_t, dot_prod_tran_concat_tbl[32]) = {
  0, 8,  16, 24, 1, 9,  17, 25, 2, 10, 18, 26, 3, 11, 19, 27,
  4, 12, 20, 28, 5, 13, 21, 29, 6, 14, 22, 30, 7, 15, 23, 31
};

DECLARE_ALIGNED(16, static const uint8_t, dot_prod_merge_block_tbl[48]) = {
  /* Shift left and insert new last column in transposed 4x4 block. */
  1, 2, 3, 16, 5, 6, 7, 20, 9, 10, 11, 24, 13, 14, 15, 28,
  /* Shift left and insert two new columns in transposed 4x4 block. */
  2, 3, 16, 17, 6, 7, 20, 21, 10, 11, 24, 25, 14, 15, 28, 29,
  /* Shift left and insert three new columns in transposed 4x4 block. */
  3, 16, 17, 18, 7, 20, 21, 22, 11, 24, 25, 26, 15, 28, 29, 30
};

#if defined(__ARM_FEATURE_MATMUL_INT8)

static INLINE int16x4_t convolve8_4_usdot(uint8x16_t samples,
                                          const int8x8_t filter,
                                          const uint8x16x2_t permute_tbl) {
  uint8x16_t permuted_samples[2];
  int32x4_t sum;

  /* Permute samples ready for dot product. */
  /* { 0,  1,  2,  3,  1,  2,  3,  4,  2,  3,  4,  5,  3,  4,  5,  6 } */
  permuted_samples[0] = vqtbl1q_u8(samples, permute_tbl.val[0]);
  /* { 4,  5,  6,  7,  5,  6,  7,  8,  6,  7,  8,  9,  7,  8,  9, 10 } */
  permuted_samples[1] = vqtbl1q_u8(samples, permute_tbl.val[1]);

  /* Accumulate dot product into 'correction' to account for range clamp. */
  sum = vusdotq_lane_s32(vdupq_n_s32(0), permuted_samples[0], filter, 0);
  sum = vusdotq_lane_s32(sum, permuted_samples[1], filter, 1);

  /* Further narrowing and packing is performed by the caller. */
  return vqmovn_s32(sum);
}

static INLINE uint8x8_t convolve8_8_usdot(uint8x16_t samples,
                                          const int8x8_t filter,
                                          const uint8x16x3_t permute_tbl) {
  uint8x16_t permuted_samples[3];
  int32x4_t sum0, sum1;
  int16x8_t sum;

  /* Permute samples ready for dot product. */
  /* { 0,  1,  2,  3,  1,  2,  3,  4,  2,  3,  4,  5,  3,  4,  5,  6 } */
  permuted_samples[0] = vqtbl1q_u8(samples, permute_tbl.val[0]);
  /* { 4,  5,  6,  7,  5,  6,  7,  8,  6,  7,  8,  9,  7,  8,  9, 10 } */
  permuted_samples[1] = vqtbl1q_u8(samples, permute_tbl.val[1]);
  /* { 8,  9, 10, 11,  9, 10, 11, 12, 10, 11, 12, 13, 11, 12, 13, 14 } */
  permuted_samples[2] = vqtbl1q_u8(samples, permute_tbl.val[2]);

  /* First 4 output values. */
  sum0 = vusdotq_lane_s32(vdupq_n_s32(0), permuted_samples[0], filter, 0);
  sum0 = vusdotq_lane_s32(sum0, permuted_samples[1], filter, 1);
  /* Second 4 output values. */
  sum1 = vusdotq_lane_s32(vdupq_n_s32(0), permuted_samples[1], filter, 0);
  sum1 = vusdotq_lane_s32(sum1, permuted_samples[2], filter, 1);

  /* Narrow and re-pack. */
  sum = vcombine_s16(vqmovn_s32(sum0), vqmovn_s32(sum1));
  return vqrshrun_n_s16(sum, FILTER_BITS);
}

void aom_convolve8_horiz_neon(const uint8_t *src, ptrdiff_t src_stride,
                              uint8_t *dst, ptrdiff_t dst_stride,
                              const int16_t *filter_x, int x_step_q4,
                              const int16_t *filter_y, int y_step_q4, int w,
                              int h) {
  const int8x8_t filter = vmovn_s16(vld1q_s16(filter_x));
  uint8x16_t s0, s1, s2, s3;

  assert((intptr_t)dst % 4 == 0);
  assert(dst_stride % 4 == 0);

  (void)x_step_q4;
  (void)filter_y;
  (void)y_step_q4;

  src -= ((SUBPEL_TAPS / 2) - 1);

  if (w == 4) {
    const uint8x16x2_t perm_tbl = vld1q_u8_x2(dot_prod_permute_tbl);
    do {
      int16x4_t t0, t1, t2, t3;
      uint8x8_t d01, d23;

      load_u8_16x4(src, src_stride, &s0, &s1, &s2, &s3);

      t0 = convolve8_4_usdot(s0, filter, perm_tbl);
      t1 = convolve8_4_usdot(s1, filter, perm_tbl);
      t2 = convolve8_4_usdot(s2, filter, perm_tbl);
      t3 = convolve8_4_usdot(s3, filter, perm_tbl);
      d01 = vqrshrun_n_s16(vcombine_s16(t0, t1), FILTER_BITS);
      d23 = vqrshrun_n_s16(vcombine_s16(t2, t3), FILTER_BITS);

      store_u8_4x1(dst + 0 * dst_stride, d01, 0);
      store_u8_4x1(dst + 1 * dst_stride, d01, 1);
      store_u8_4x1(dst + 2 * dst_stride, d23, 0);
      store_u8_4x1(dst + 3 * dst_stride, d23, 1);

      src += 4 * src_stride;
      dst += 4 * dst_stride;
      h -= 4;
    } while (h > 0);
  } else {
    const uint8x16x3_t perm_tbl = vld1q_u8_x3(dot_prod_permute_tbl);
    const uint8_t *s;
    uint8_t *d;
    int width;
    uint8x8_t d0, d1, d2, d3;

    do {
      width = w;
      s = src;
      d = dst;
      do {
        load_u8_16x4(s, src_stride, &s0, &s1, &s2, &s3);

        d0 = convolve8_8_usdot(s0, filter, perm_tbl);
        d1 = convolve8_8_usdot(s1, filter, perm_tbl);
        d2 = convolve8_8_usdot(s2, filter, perm_tbl);
        d3 = convolve8_8_usdot(s3, filter, perm_tbl);

        store_u8_8x4(d, dst_stride, d0, d1, d2, d3);

        s += 8;
        d += 8;
        width -= 8;
      } while (width != 0);
      src += 4 * src_stride;
      dst += 4 * dst_stride;
      h -= 4;
    } while (h > 0);
  }
}

static INLINE void transpose_concat_4x4(uint8x8_t a0, uint8x8_t a1,
                                        uint8x8_t a2, uint8x8_t a3,
                                        uint8x16_t *b,
                                        const uint8x16_t permute_tbl) {
  /* Transpose 8-bit elements and concatenate result rows as follows:
   * a0: 00, 01, 02, 03, XX, XX, XX, XX
   * a1: 10, 11, 12, 13, XX, XX, XX, XX
   * a2: 20, 21, 22, 23, XX, XX, XX, XX
   * a3: 30, 31, 32, 33, XX, XX, XX, XX
   *
   * b: 00, 10, 20, 30, 01, 11, 21, 31, 02, 12, 22, 32, 03, 13, 23, 33
   *
   * The 'permute_tbl' is always 'dot_prod_tran_concat_tbl' above. Passing it
   * as an argument is preferable to loading it directly from memory as this
   * inline helper is called many times from the same parent function.
   */

  uint8x16x2_t samples = { { vcombine_u8(a0, a1), vcombine_u8(a2, a3) } };
  *b = vqtbl2q_u8(samples, permute_tbl);
}

static INLINE void transpose_concat_8x4(uint8x8_t a0, uint8x8_t a1,
                                        uint8x8_t a2, uint8x8_t a3,
                                        uint8x16_t *b0, uint8x16_t *b1,
                                        const uint8x16x2_t permute_tbl) {
  /* Transpose 8-bit elements and concatenate result rows as follows:
   * a0: 00, 01, 02, 03, 04, 05, 06, 07
   * a1: 10, 11, 12, 13, 14, 15, 16, 17
   * a2: 20, 21, 22, 23, 24, 25, 26, 27
   * a3: 30, 31, 32, 33, 34, 35, 36, 37
   *
   * b0: 00, 10, 20, 30, 01, 11, 21, 31, 02, 12, 22, 32, 03, 13, 23, 33
   * b1: 04, 14, 24, 34, 05, 15, 25, 35, 06, 16, 26, 36, 07, 17, 27, 37
   *
   * The 'permute_tbl' is always 'dot_prod_tran_concat_tbl' above. Passing it
   * as an argument is preferable to loading it directly from memory as this
   * inline helper is called many times from the same parent function.
   */

  uint8x16x2_t samples = { { vcombine_u8(a0, a1), vcombine_u8(a2, a3) } };
  *b0 = vqtbl2q_u8(samples, permute_tbl.val[0]);
  *b1 = vqtbl2q_u8(samples, permute_tbl.val[1]);
}

static INLINE int16x4_t convolve8_4_usdot_partial(const uint8x16_t samples_lo,
                                                  const uint8x16_t samples_hi,
                                                  const int8x8_t filter) {
  /* Sample permutation is performed by the caller. */
  int32x4_t sum;

  sum = vusdotq_lane_s32(vdupq_n_s32(0), samples_lo, filter, 0);
  sum = vusdotq_lane_s32(sum, samples_hi, filter, 1);

  /* Further narrowing and packing is performed by the caller. */
  return vqmovn_s32(sum);
}

static INLINE uint8x8_t convolve8_8_usdot_partial(const uint8x16_t samples0_lo,
                                                  const uint8x16_t samples0_hi,
                                                  const uint8x16_t samples1_lo,
                                                  const uint8x16_t samples1_hi,
                                                  const int8x8_t filter) {
  /* Sample permutation is performed by the caller. */
  int32x4_t sum0, sum1;
  int16x8_t sum;

  /* First 4 output values. */
  sum0 = vusdotq_lane_s32(vdupq_n_s32(0), samples0_lo, filter, 0);
  sum0 = vusdotq_lane_s32(sum0, samples0_hi, filter, 1);
  /* Second 4 output values. */
  sum1 = vusdotq_lane_s32(vdupq_n_s32(0), samples1_lo, filter, 0);
  sum1 = vusdotq_lane_s32(sum1, samples1_hi, filter, 1);

  /* Narrow and re-pack. */
  sum = vcombine_s16(vqmovn_s32(sum0), vqmovn_s32(sum1));
  return vqrshrun_n_s16(sum, FILTER_BITS);
}

void aom_convolve8_vert_neon(const uint8_t *src, ptrdiff_t src_stride,
                             uint8_t *dst, ptrdiff_t dst_stride,
                             const int16_t *filter_x, int x_step_q4,
                             const int16_t *filter_y, int y_step_q4, int w,
                             int h) {
  const int8x8_t filter = vmovn_s16(vld1q_s16(filter_y));
  const uint8x16x3_t merge_block_tbl = vld1q_u8_x3(dot_prod_merge_block_tbl);
  uint8x8_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10;
  uint8x16x2_t samples_LUT;

  assert((intptr_t)dst % 4 == 0);
  assert(dst_stride % 4 == 0);

  (void)filter_x;
  (void)x_step_q4;
  (void)y_step_q4;

  src -= ((SUBPEL_TAPS / 2) - 1) * src_stride;

  if (w == 4) {
    const uint8x16_t tran_concat_tbl = vld1q_u8(dot_prod_tran_concat_tbl);
    uint8x16_t s0123, s1234, s2345, s3456, s4567, s5678, s6789, s78910;
    int16x4_t d0, d1, d2, d3;
    uint8x8_t d01, d23;

    load_u8_8x7(src, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6);
    src += 7 * src_stride;

    s7 = vdup_n_u8(0);
    s8 = vdup_n_u8(0);
    s9 = vdup_n_u8(0);

    /* This operation combines a conventional transpose and the sample permute
     * (see horizontal case) required before computing the dot product.
     */
    transpose_concat_4x4(s0, s1, s2, s3, &s0123, tran_concat_tbl);
    transpose_concat_4x4(s1, s2, s3, s4, &s1234, tran_concat_tbl);
    transpose_concat_4x4(s2, s3, s4, s5, &s2345, tran_concat_tbl);
    transpose_concat_4x4(s3, s4, s5, s6, &s3456, tran_concat_tbl);
    transpose_concat_4x4(s4, s5, s6, s7, &s4567, tran_concat_tbl);
    transpose_concat_4x4(s5, s6, s7, s8, &s5678, tran_concat_tbl);
    transpose_concat_4x4(s6, s7, s8, s9, &s6789, tran_concat_tbl);

    do {
      load_u8_8x4(src, src_stride, &s7, &s8, &s9, &s10);

      transpose_concat_4x4(s7, s8, s9, s10, &s78910, tran_concat_tbl);

      /* Merge new data into block from previous iteration. */
      samples_LUT.val[0] = s3456;
      samples_LUT.val[1] = s78910;
      s4567 = vqtbl2q_u8(samples_LUT, merge_block_tbl.val[0]);
      s5678 = vqtbl2q_u8(samples_LUT, merge_block_tbl.val[1]);
      s6789 = vqtbl2q_u8(samples_LUT, merge_block_tbl.val[2]);

      d0 = convolve8_4_usdot_partial(s0123, s4567, filter);
      d1 = convolve8_4_usdot_partial(s1234, s5678, filter);
      d2 = convolve8_4_usdot_partial(s2345, s6789, filter);
      d3 = convolve8_4_usdot_partial(s3456, s78910, filter);
      d01 = vqrshrun_n_s16(vcombine_s16(d0, d1), FILTER_BITS);
      d23 = vqrshrun_n_s16(vcombine_s16(d2, d3), FILTER_BITS);

      store_u8_4x1(dst + 0 * dst_stride, d01, 0);
      store_u8_4x1(dst + 1 * dst_stride, d01, 1);
      store_u8_4x1(dst + 2 * dst_stride, d23, 0);
      store_u8_4x1(dst + 3 * dst_stride, d23, 1);

      /* Prepare block for next iteration - re-using as much as possible. */
      /* Shuffle everything up four rows. */
      s0123 = s4567;
      s1234 = s5678;
      s2345 = s6789;
      s3456 = s78910;

      src += 4 * src_stride;
      dst += 4 * dst_stride;
      h -= 4;
    } while (h != 0);
  } else {
    const uint8x16x2_t tran_concat_tbl = vld1q_u8_x2(dot_prod_tran_concat_tbl);
    uint8x16_t s0123_lo, s0123_hi, s1234_lo, s1234_hi, s2345_lo, s2345_hi,
        s3456_lo, s3456_hi, s4567_lo, s4567_hi, s5678_lo, s5678_hi, s6789_lo,
        s6789_hi, s78910_lo, s78910_hi;
    uint8x8_t d0, d1, d2, d3;
    const uint8_t *s;
    uint8_t *d;
    int height;

    do {
      height = h;
      s = src;
      d = dst;

      load_u8_8x7(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6);
      s += 7 * src_stride;

      s7 = vdup_n_u8(0);
      s8 = vdup_n_u8(0);
      s9 = vdup_n_u8(0);

      /* This operation combines a conventional transpose and the sample permute
       * (see horizontal case) required before computing the dot product.
       */
      transpose_concat_8x4(s0, s1, s2, s3, &s0123_lo, &s0123_hi,
                           tran_concat_tbl);
      transpose_concat_8x4(s1, s2, s3, s4, &s1234_lo, &s1234_hi,
                           tran_concat_tbl);
      transpose_concat_8x4(s2, s3, s4, s5, &s2345_lo, &s2345_hi,
                           tran_concat_tbl);
      transpose_concat_8x4(s3, s4, s5, s6, &s3456_lo, &s3456_hi,
                           tran_concat_tbl);
      transpose_concat_8x4(s4, s5, s6, s7, &s4567_lo, &s4567_hi,
                           tran_concat_tbl);
      transpose_concat_8x4(s5, s6, s7, s8, &s5678_lo, &s5678_hi,
                           tran_concat_tbl);
      transpose_concat_8x4(s6, s7, s8, s9, &s6789_lo, &s6789_hi,
                           tran_concat_tbl);

      do {
        load_u8_8x4(s, src_stride, &s7, &s8, &s9, &s10);

        transpose_concat_8x4(s7, s8, s9, s10, &s78910_lo, &s78910_hi,
                             tran_concat_tbl);

        /* Merge new data into block from previous iteration. */
        samples_LUT.val[0] = s3456_lo;
        samples_LUT.val[1] = s78910_lo;
        s4567_lo = vqtbl2q_u8(samples_LUT, merge_block_tbl.val[0]);
        s5678_lo = vqtbl2q_u8(samples_LUT, merge_block_tbl.val[1]);
        s6789_lo = vqtbl2q_u8(samples_LUT, merge_block_tbl.val[2]);

        samples_LUT.val[0] = s3456_hi;
        samples_LUT.val[1] = s78910_hi;
        s4567_hi = vqtbl2q_u8(samples_LUT, merge_block_tbl.val[0]);
        s5678_hi = vqtbl2q_u8(samples_LUT, merge_block_tbl.val[1]);
        s6789_hi = vqtbl2q_u8(samples_LUT, merge_block_tbl.val[2]);

        d0 = convolve8_8_usdot_partial(s0123_lo, s4567_lo, s0123_hi, s4567_hi,
                                       filter);
        d1 = convolve8_8_usdot_partial(s1234_lo, s5678_lo, s1234_hi, s5678_hi,
                                       filter);
        d2 = convolve8_8_usdot_partial(s2345_lo, s6789_lo, s2345_hi, s6789_hi,
                                       filter);
        d3 = convolve8_8_usdot_partial(s3456_lo, s78910_lo, s3456_hi, s78910_hi,
                                       filter);

        store_u8_8x4(d, dst_stride, d0, d1, d2, d3);

        /* Prepare block for next iteration - re-using as much as possible. */
        /* Shuffle everything up four rows. */
        s0123_lo = s4567_lo;
        s0123_hi = s4567_hi;
        s1234_lo = s5678_lo;
        s1234_hi = s5678_hi;
        s2345_lo = s6789_lo;
        s2345_hi = s6789_hi;
        s3456_lo = s78910_lo;
        s3456_hi = s78910_hi;

        s += 4 * src_stride;
        d += 4 * dst_stride;
        height -= 4;
      } while (height != 0);
      src += 8;
      dst += 8;
      w -= 8;
    } while (w != 0);
  }
}

#else  // !defined(__ARM_FEATURE_MATMUL_INT8)

static INLINE int16x4_t convolve8_4_sdot(uint8x16_t samples,
                                         const int8x8_t filter,
                                         const int32x4_t correction,
                                         const uint8x16_t range_limit,
                                         const uint8x16x2_t permute_tbl) {
  int8x16_t clamped_samples, permuted_samples[2];
  int32x4_t sum;

  /* Clamp sample range to [-128, 127] for 8-bit signed dot product. */
  clamped_samples = vreinterpretq_s8_u8(vsubq_u8(samples, range_limit));

  /* Permute samples ready for dot product. */
  /* { 0,  1,  2,  3,  1,  2,  3,  4,  2,  3,  4,  5,  3,  4,  5,  6 } */
  permuted_samples[0] = vqtbl1q_s8(clamped_samples, permute_tbl.val[0]);
  /* { 4,  5,  6,  7,  5,  6,  7,  8,  6,  7,  8,  9,  7,  8,  9, 10 } */
  permuted_samples[1] = vqtbl1q_s8(clamped_samples, permute_tbl.val[1]);

  /* Accumulate dot product into 'correction' to account for range clamp. */
  sum = vdotq_lane_s32(correction, permuted_samples[0], filter, 0);
  sum = vdotq_lane_s32(sum, permuted_samples[1], filter, 1);

  /* Further narrowing and packing is performed by the caller. */
  return vqmovn_s32(sum);
}

static INLINE uint8x8_t convolve8_8_sdot(uint8x16_t samples,
                                         const int8x8_t filter,
                                         const int32x4_t correction,
                                         const uint8x16_t range_limit,
                                         const uint8x16x3_t permute_tbl) {
  int8x16_t clamped_samples, permuted_samples[3];
  int32x4_t sum0, sum1;
  int16x8_t sum;

  /* Clamp sample range to [-128, 127] for 8-bit signed dot product. */
  clamped_samples = vreinterpretq_s8_u8(vsubq_u8(samples, range_limit));

  /* Permute samples ready for dot product. */
  /* { 0,  1,  2,  3,  1,  2,  3,  4,  2,  3,  4,  5,  3,  4,  5,  6 } */
  permuted_samples[0] = vqtbl1q_s8(clamped_samples, permute_tbl.val[0]);
  /* { 4,  5,  6,  7,  5,  6,  7,  8,  6,  7,  8,  9,  7,  8,  9, 10 } */
  permuted_samples[1] = vqtbl1q_s8(clamped_samples, permute_tbl.val[1]);
  /* { 8,  9, 10, 11,  9, 10, 11, 12, 10, 11, 12, 13, 11, 12, 13, 14 } */
  permuted_samples[2] = vqtbl1q_s8(clamped_samples, permute_tbl.val[2]);

  /* Accumulate dot product into 'correction' to account for range clamp. */
  /* First 4 output values. */
  sum0 = vdotq_lane_s32(correction, permuted_samples[0], filter, 0);
  sum0 = vdotq_lane_s32(sum0, permuted_samples[1], filter, 1);
  /* Second 4 output values. */
  sum1 = vdotq_lane_s32(correction, permuted_samples[1], filter, 0);
  sum1 = vdotq_lane_s32(sum1, permuted_samples[2], filter, 1);

  /* Narrow and re-pack. */
  sum = vcombine_s16(vqmovn_s32(sum0), vqmovn_s32(sum1));
  return vqrshrun_n_s16(sum, FILTER_BITS);
}

void aom_convolve8_horiz_neon(const uint8_t *src, ptrdiff_t src_stride,
                              uint8_t *dst, ptrdiff_t dst_stride,
                              const int16_t *filter_x, int x_step_q4,
                              const int16_t *filter_y, int y_step_q4, int w,
                              int h) {
  const int8x8_t filter = vmovn_s16(vld1q_s16(filter_x));
  const int16x8_t correct_tmp = vmulq_n_s16(vld1q_s16(filter_x), 128);
  const int32x4_t correction = vdupq_n_s32((int32_t)vaddvq_s16(correct_tmp));
  const uint8x16_t range_limit = vdupq_n_u8(128);
  uint8x16_t s0, s1, s2, s3;

  assert((intptr_t)dst % 4 == 0);
  assert(dst_stride % 4 == 0);

  (void)x_step_q4;
  (void)filter_y;
  (void)y_step_q4;

  src -= ((SUBPEL_TAPS / 2) - 1);

  if (w == 4) {
    const uint8x16x2_t perm_tbl = vld1q_u8_x2(dot_prod_permute_tbl);
    do {
      int16x4_t t0, t1, t2, t3;
      uint8x8_t d01, d23;

      load_u8_16x4(src, src_stride, &s0, &s1, &s2, &s3);

      t0 = convolve8_4_sdot(s0, filter, correction, range_limit, perm_tbl);
      t1 = convolve8_4_sdot(s1, filter, correction, range_limit, perm_tbl);
      t2 = convolve8_4_sdot(s2, filter, correction, range_limit, perm_tbl);
      t3 = convolve8_4_sdot(s3, filter, correction, range_limit, perm_tbl);
      d01 = vqrshrun_n_s16(vcombine_s16(t0, t1), FILTER_BITS);
      d23 = vqrshrun_n_s16(vcombine_s16(t2, t3), FILTER_BITS);

      store_u8_4x1(dst + 0 * dst_stride, d01, 0);
      store_u8_4x1(dst + 1 * dst_stride, d01, 1);
      store_u8_4x1(dst + 2 * dst_stride, d23, 0);
      store_u8_4x1(dst + 3 * dst_stride, d23, 1);

      src += 4 * src_stride;
      dst += 4 * dst_stride;
      h -= 4;
    } while (h > 0);
  } else {
    const uint8x16x3_t perm_tbl = vld1q_u8_x3(dot_prod_permute_tbl);
    const uint8_t *s;
    uint8_t *d;
    int width;
    uint8x8_t d0, d1, d2, d3;

    do {
      width = w;
      s = src;
      d = dst;
      do {
        load_u8_16x4(s, src_stride, &s0, &s1, &s2, &s3);

        d0 = convolve8_8_sdot(s0, filter, correction, range_limit, perm_tbl);
        d1 = convolve8_8_sdot(s1, filter, correction, range_limit, perm_tbl);
        d2 = convolve8_8_sdot(s2, filter, correction, range_limit, perm_tbl);
        d3 = convolve8_8_sdot(s3, filter, correction, range_limit, perm_tbl);

        store_u8_8x4(d, dst_stride, d0, d1, d2, d3);

        s += 8;
        d += 8;
        width -= 8;
      } while (width != 0);
      src += 4 * src_stride;
      dst += 4 * dst_stride;
      h -= 4;
    } while (h > 0);
  }
}

static INLINE void transpose_concat_4x4(int8x8_t a0, int8x8_t a1, int8x8_t a2,
                                        int8x8_t a3, int8x16_t *b,
                                        const uint8x16_t permute_tbl) {
  /* Transpose 8-bit elements and concatenate result rows as follows:
   * a0: 00, 01, 02, 03, XX, XX, XX, XX
   * a1: 10, 11, 12, 13, XX, XX, XX, XX
   * a2: 20, 21, 22, 23, XX, XX, XX, XX
   * a3: 30, 31, 32, 33, XX, XX, XX, XX
   *
   * b: 00, 10, 20, 30, 01, 11, 21, 31, 02, 12, 22, 32, 03, 13, 23, 33
   *
   * The 'permute_tbl' is always 'dot_prod_tran_concat_tbl' above. Passing it
   * as an argument is preferable to loading it directly from memory as this
   * inline helper is called many times from the same parent function.
   */

  int8x16x2_t samples = { { vcombine_s8(a0, a1), vcombine_s8(a2, a3) } };
  *b = vqtbl2q_s8(samples, permute_tbl);
}

static INLINE void transpose_concat_8x4(int8x8_t a0, int8x8_t a1, int8x8_t a2,
                                        int8x8_t a3, int8x16_t *b0,
                                        int8x16_t *b1,
                                        const uint8x16x2_t permute_tbl) {
  /* Transpose 8-bit elements and concatenate result rows as follows:
   * a0: 00, 01, 02, 03, 04, 05, 06, 07
   * a1: 10, 11, 12, 13, 14, 15, 16, 17
   * a2: 20, 21, 22, 23, 24, 25, 26, 27
   * a3: 30, 31, 32, 33, 34, 35, 36, 37
   *
   * b0: 00, 10, 20, 30, 01, 11, 21, 31, 02, 12, 22, 32, 03, 13, 23, 33
   * b1: 04, 14, 24, 34, 05, 15, 25, 35, 06, 16, 26, 36, 07, 17, 27, 37
   *
   * The 'permute_tbl' is always 'dot_prod_tran_concat_tbl' above. Passing it
   * as an argument is preferable to loading it directly from memory as this
   * inline helper is called many times from the same parent function.
   */

  int8x16x2_t samples = { { vcombine_s8(a0, a1), vcombine_s8(a2, a3) } };
  *b0 = vqtbl2q_s8(samples, permute_tbl.val[0]);
  *b1 = vqtbl2q_s8(samples, permute_tbl.val[1]);
}

static INLINE int16x4_t convolve8_4_sdot_partial(const int8x16_t samples_lo,
                                                 const int8x16_t samples_hi,
                                                 const int32x4_t correction,
                                                 const int8x8_t filter) {
  /* Sample range-clamping and permutation are performed by the caller. */
  int32x4_t sum;

  /* Accumulate dot product into 'correction' to account for range clamp. */
  sum = vdotq_lane_s32(correction, samples_lo, filter, 0);
  sum = vdotq_lane_s32(sum, samples_hi, filter, 1);

  /* Further narrowing and packing is performed by the caller. */
  return vqmovn_s32(sum);
}

static INLINE uint8x8_t convolve8_8_sdot_partial(const int8x16_t samples0_lo,
                                                 const int8x16_t samples0_hi,
                                                 const int8x16_t samples1_lo,
                                                 const int8x16_t samples1_hi,
                                                 const int32x4_t correction,
                                                 const int8x8_t filter) {
  /* Sample range-clamping and permutation are performed by the caller. */
  int32x4_t sum0, sum1;
  int16x8_t sum;

  /* Accumulate dot product into 'correction' to account for range clamp. */
  /* First 4 output values. */
  sum0 = vdotq_lane_s32(correction, samples0_lo, filter, 0);
  sum0 = vdotq_lane_s32(sum0, samples0_hi, filter, 1);
  /* Second 4 output values. */
  sum1 = vdotq_lane_s32(correction, samples1_lo, filter, 0);
  sum1 = vdotq_lane_s32(sum1, samples1_hi, filter, 1);

  /* Narrow and re-pack. */
  sum = vcombine_s16(vqmovn_s32(sum0), vqmovn_s32(sum1));
  return vqrshrun_n_s16(sum, FILTER_BITS);
}

void aom_convolve8_vert_neon(const uint8_t *src, ptrdiff_t src_stride,
                             uint8_t *dst, ptrdiff_t dst_stride,
                             const int16_t *filter_x, int x_step_q4,
                             const int16_t *filter_y, int y_step_q4, int w,
                             int h) {
  const int8x8_t filter = vmovn_s16(vld1q_s16(filter_y));
  const int16x8_t correct_tmp = vmulq_n_s16(vld1q_s16(filter_y), 128);
  const int32x4_t correction = vdupq_n_s32((int32_t)vaddvq_s16(correct_tmp));
  const uint8x8_t range_limit = vdup_n_u8(128);
  const uint8x16x3_t merge_block_tbl = vld1q_u8_x3(dot_prod_merge_block_tbl);
  uint8x8_t t0, t1, t2, t3, t4, t5, t6;
  int8x8_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10;
  int8x16x2_t samples_LUT;

  assert((intptr_t)dst % 4 == 0);
  assert(dst_stride % 4 == 0);

  (void)filter_x;
  (void)x_step_q4;
  (void)y_step_q4;

  src -= ((SUBPEL_TAPS / 2) - 1) * src_stride;

  if (w == 4) {
    const uint8x16_t tran_concat_tbl = vld1q_u8(dot_prod_tran_concat_tbl);
    int8x16_t s0123, s1234, s2345, s3456, s4567, s5678, s6789, s78910;
    int16x4_t d0, d1, d2, d3;
    uint8x8_t d01, d23;

    load_u8_8x7(src, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6);
    src += 7 * src_stride;

    /* Clamp sample range to [-128, 127] for 8-bit signed dot product. */
    s0 = vreinterpret_s8_u8(vsub_u8(t0, range_limit));
    s1 = vreinterpret_s8_u8(vsub_u8(t1, range_limit));
    s2 = vreinterpret_s8_u8(vsub_u8(t2, range_limit));
    s3 = vreinterpret_s8_u8(vsub_u8(t3, range_limit));
    s4 = vreinterpret_s8_u8(vsub_u8(t4, range_limit));
    s5 = vreinterpret_s8_u8(vsub_u8(t5, range_limit));
    s6 = vreinterpret_s8_u8(vsub_u8(t6, range_limit));
    s7 = vdup_n_s8(0);
    s8 = vdup_n_s8(0);
    s9 = vdup_n_s8(0);

    /* This operation combines a conventional transpose and the sample permute
     * (see horizontal case) required before computing the dot product.
     */
    transpose_concat_4x4(s0, s1, s2, s3, &s0123, tran_concat_tbl);
    transpose_concat_4x4(s1, s2, s3, s4, &s1234, tran_concat_tbl);
    transpose_concat_4x4(s2, s3, s4, s5, &s2345, tran_concat_tbl);
    transpose_concat_4x4(s3, s4, s5, s6, &s3456, tran_concat_tbl);
    transpose_concat_4x4(s4, s5, s6, s7, &s4567, tran_concat_tbl);
    transpose_concat_4x4(s5, s6, s7, s8, &s5678, tran_concat_tbl);
    transpose_concat_4x4(s6, s7, s8, s9, &s6789, tran_concat_tbl);

    do {
      uint8x8_t t7, t8, t9, t10;

      load_u8_8x4(src, src_stride, &t7, &t8, &t9, &t10);

      s7 = vreinterpret_s8_u8(vsub_u8(t7, range_limit));
      s8 = vreinterpret_s8_u8(vsub_u8(t8, range_limit));
      s9 = vreinterpret_s8_u8(vsub_u8(t9, range_limit));
      s10 = vreinterpret_s8_u8(vsub_u8(t10, range_limit));

      transpose_concat_4x4(s7, s8, s9, s10, &s78910, tran_concat_tbl);

      /* Merge new data into block from previous iteration. */
      samples_LUT.val[0] = s3456;
      samples_LUT.val[1] = s78910;
      s4567 = vqtbl2q_s8(samples_LUT, merge_block_tbl.val[0]);
      s5678 = vqtbl2q_s8(samples_LUT, merge_block_tbl.val[1]);
      s6789 = vqtbl2q_s8(samples_LUT, merge_block_tbl.val[2]);

      d0 = convolve8_4_sdot_partial(s0123, s4567, correction, filter);
      d1 = convolve8_4_sdot_partial(s1234, s5678, correction, filter);
      d2 = convolve8_4_sdot_partial(s2345, s6789, correction, filter);
      d3 = convolve8_4_sdot_partial(s3456, s78910, correction, filter);
      d01 = vqrshrun_n_s16(vcombine_s16(d0, d1), FILTER_BITS);
      d23 = vqrshrun_n_s16(vcombine_s16(d2, d3), FILTER_BITS);

      store_u8_4x1(dst + 0 * dst_stride, d01, 0);
      store_u8_4x1(dst + 1 * dst_stride, d01, 1);
      store_u8_4x1(dst + 2 * dst_stride, d23, 0);
      store_u8_4x1(dst + 3 * dst_stride, d23, 1);

      /* Prepare block for next iteration - re-using as much as possible. */
      /* Shuffle everything up four rows. */
      s0123 = s4567;
      s1234 = s5678;
      s2345 = s6789;
      s3456 = s78910;

      src += 4 * src_stride;
      dst += 4 * dst_stride;
      h -= 4;
    } while (h != 0);
  } else {
    const uint8x16x2_t tran_concat_tbl = vld1q_u8_x2(dot_prod_tran_concat_tbl);
    int8x16_t s0123_lo, s0123_hi, s1234_lo, s1234_hi, s2345_lo, s2345_hi,
        s3456_lo, s3456_hi, s4567_lo, s4567_hi, s5678_lo, s5678_hi, s6789_lo,
        s6789_hi, s78910_lo, s78910_hi;
    uint8x8_t d0, d1, d2, d3;
    const uint8_t *s;
    uint8_t *d;
    int height;

    do {
      height = h;
      s = src;
      d = dst;

      load_u8_8x7(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6);
      s += 7 * src_stride;

      /* Clamp sample range to [-128, 127] for 8-bit signed dot product. */
      s0 = vreinterpret_s8_u8(vsub_u8(t0, range_limit));
      s1 = vreinterpret_s8_u8(vsub_u8(t1, range_limit));
      s2 = vreinterpret_s8_u8(vsub_u8(t2, range_limit));
      s3 = vreinterpret_s8_u8(vsub_u8(t3, range_limit));
      s4 = vreinterpret_s8_u8(vsub_u8(t4, range_limit));
      s5 = vreinterpret_s8_u8(vsub_u8(t5, range_limit));
      s6 = vreinterpret_s8_u8(vsub_u8(t6, range_limit));
      s7 = vdup_n_s8(0);
      s8 = vdup_n_s8(0);
      s9 = vdup_n_s8(0);

      /* This operation combines a conventional transpose and the sample permute
       * (see horizontal case) required before computing the dot product.
       */
      transpose_concat_8x4(s0, s1, s2, s3, &s0123_lo, &s0123_hi,
                           tran_concat_tbl);
      transpose_concat_8x4(s1, s2, s3, s4, &s1234_lo, &s1234_hi,
                           tran_concat_tbl);
      transpose_concat_8x4(s2, s3, s4, s5, &s2345_lo, &s2345_hi,
                           tran_concat_tbl);
      transpose_concat_8x4(s3, s4, s5, s6, &s3456_lo, &s3456_hi,
                           tran_concat_tbl);
      transpose_concat_8x4(s4, s5, s6, s7, &s4567_lo, &s4567_hi,
                           tran_concat_tbl);
      transpose_concat_8x4(s5, s6, s7, s8, &s5678_lo, &s5678_hi,
                           tran_concat_tbl);
      transpose_concat_8x4(s6, s7, s8, s9, &s6789_lo, &s6789_hi,
                           tran_concat_tbl);

      do {
        uint8x8_t t7, t8, t9, t10;

        load_u8_8x4(s, src_stride, &t7, &t8, &t9, &t10);

        s7 = vreinterpret_s8_u8(vsub_u8(t7, range_limit));
        s8 = vreinterpret_s8_u8(vsub_u8(t8, range_limit));
        s9 = vreinterpret_s8_u8(vsub_u8(t9, range_limit));
        s10 = vreinterpret_s8_u8(vsub_u8(t10, range_limit));

        transpose_concat_8x4(s7, s8, s9, s10, &s78910_lo, &s78910_hi,
                             tran_concat_tbl);

        /* Merge new data into block from previous iteration. */
        samples_LUT.val[0] = s3456_lo;
        samples_LUT.val[1] = s78910_lo;
        s4567_lo = vqtbl2q_s8(samples_LUT, merge_block_tbl.val[0]);
        s5678_lo = vqtbl2q_s8(samples_LUT, merge_block_tbl.val[1]);
        s6789_lo = vqtbl2q_s8(samples_LUT, merge_block_tbl.val[2]);

        samples_LUT.val[0] = s3456_hi;
        samples_LUT.val[1] = s78910_hi;
        s4567_hi = vqtbl2q_s8(samples_LUT, merge_block_tbl.val[0]);
        s5678_hi = vqtbl2q_s8(samples_LUT, merge_block_tbl.val[1]);
        s6789_hi = vqtbl2q_s8(samples_LUT, merge_block_tbl.val[2]);

        d0 = convolve8_8_sdot_partial(s0123_lo, s4567_lo, s0123_hi, s4567_hi,
                                      correction, filter);
        d1 = convolve8_8_sdot_partial(s1234_lo, s5678_lo, s1234_hi, s5678_hi,
                                      correction, filter);
        d2 = convolve8_8_sdot_partial(s2345_lo, s6789_lo, s2345_hi, s6789_hi,
                                      correction, filter);
        d3 = convolve8_8_sdot_partial(s3456_lo, s78910_lo, s3456_hi, s78910_hi,
                                      correction, filter);

        store_u8_8x4(d, dst_stride, d0, d1, d2, d3);

        /* Prepare block for next iteration - re-using as much as possible. */
        /* Shuffle everything up four rows. */
        s0123_lo = s4567_lo;
        s0123_hi = s4567_hi;
        s1234_lo = s5678_lo;
        s1234_hi = s5678_hi;
        s2345_lo = s6789_lo;
        s2345_hi = s6789_hi;
        s3456_lo = s78910_lo;
        s3456_hi = s78910_hi;

        s += 4 * src_stride;
        d += 4 * dst_stride;
        height -= 4;
      } while (height != 0);
      src += 8;
      dst += 8;
      w -= 8;
    } while (w != 0);
  }
}

#endif  // defined(__ARM_FEATURE_MATMUL_INT8)

#else  // !(AOM_ARCH_AARCH64 &&
       //   (defined(__ARM_FEATURE_DOTPROD) ||
       //    defined(__ARM_FEATURE_MATMUL_INT8)))

static INLINE int16x4_t convolve8_4(const int16x4_t s0, const int16x4_t s1,
                                    const int16x4_t s2, const int16x4_t s3,
                                    const int16x4_t s4, const int16x4_t s5,
                                    const int16x4_t s6, const int16x4_t s7,
                                    const int16x8_t filter) {
  const int16x4_t filter_lo = vget_low_s16(filter);
  const int16x4_t filter_hi = vget_high_s16(filter);
  int16x4_t sum;

  sum = vmul_lane_s16(s0, filter_lo, 0);
  sum = vmla_lane_s16(sum, s1, filter_lo, 1);
  sum = vmla_lane_s16(sum, s2, filter_lo, 2);
  sum = vmla_lane_s16(sum, s5, filter_hi, 1);
  sum = vmla_lane_s16(sum, s6, filter_hi, 2);
  sum = vmla_lane_s16(sum, s7, filter_hi, 3);
  sum = vqadd_s16(sum, vmul_lane_s16(s3, filter_lo, 3));
  sum = vqadd_s16(sum, vmul_lane_s16(s4, filter_hi, 0));
  return sum;
}

static INLINE uint8x8_t convolve8_8(const int16x8_t s0, const int16x8_t s1,
                                    const int16x8_t s2, const int16x8_t s3,
                                    const int16x8_t s4, const int16x8_t s5,
                                    const int16x8_t s6, const int16x8_t s7,
                                    const int16x8_t filter) {
  const int16x4_t filter_lo = vget_low_s16(filter);
  const int16x4_t filter_hi = vget_high_s16(filter);
  int16x8_t sum;

  sum = vmulq_lane_s16(s0, filter_lo, 0);
  sum = vmlaq_lane_s16(sum, s1, filter_lo, 1);
  sum = vmlaq_lane_s16(sum, s2, filter_lo, 2);
  sum = vmlaq_lane_s16(sum, s5, filter_hi, 1);
  sum = vmlaq_lane_s16(sum, s6, filter_hi, 2);
  sum = vmlaq_lane_s16(sum, s7, filter_hi, 3);
  sum = vqaddq_s16(sum, vmulq_lane_s16(s3, filter_lo, 3));
  sum = vqaddq_s16(sum, vmulq_lane_s16(s4, filter_hi, 0));
  return vqrshrun_n_s16(sum, FILTER_BITS);
}

void aom_convolve8_horiz_neon(const uint8_t *src, ptrdiff_t src_stride,
                              uint8_t *dst, ptrdiff_t dst_stride,
                              const int16_t *filter_x, int x_step_q4,
                              const int16_t *filter_y, int y_step_q4, int w,
                              int h) {
  const int16x8_t filter = vld1q_s16(filter_x);

  assert((intptr_t)dst % 4 == 0);
  assert(dst_stride % 4 == 0);

  (void)x_step_q4;
  (void)filter_y;
  (void)y_step_q4;

  src -= ((SUBPEL_TAPS / 2) - 1);

  if (h == 4) {
    uint8x8_t t0, t1, t2, t3, d01, d23;
    int16x4_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, d0, d1, d2, d3;

    load_u8_8x4(src, src_stride, &t0, &t1, &t2, &t3);
    transpose_u8_8x4(&t0, &t1, &t2, &t3);
    s0 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
    s1 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t1)));
    s2 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t2)));
    s3 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t3)));
    s4 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
    s5 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t1)));
    s6 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t2)));

    src += 7;

    do {
      load_u8_8x4(src, src_stride, &t0, &t1, &t2, &t3);
      transpose_u8_8x4(&t0, &t1, &t2, &t3);
      s7 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
      s8 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t1)));
      s9 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t2)));
      s10 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t3)));

      d0 = convolve8_4(s0, s1, s2, s3, s4, s5, s6, s7, filter);
      d1 = convolve8_4(s1, s2, s3, s4, s5, s6, s7, s8, filter);
      d2 = convolve8_4(s2, s3, s4, s5, s6, s7, s8, s9, filter);
      d3 = convolve8_4(s3, s4, s5, s6, s7, s8, s9, s10, filter);
      d01 = vqrshrun_n_s16(vcombine_s16(d0, d1), FILTER_BITS);
      d23 = vqrshrun_n_s16(vcombine_s16(d2, d3), FILTER_BITS);

      transpose_u8_4x4(&d01, &d23);

      store_u8_4x1(dst + 0 * dst_stride, d01, 0);
      store_u8_4x1(dst + 1 * dst_stride, d23, 0);
      store_u8_4x1(dst + 2 * dst_stride, d01, 1);
      store_u8_4x1(dst + 3 * dst_stride, d23, 1);

      s0 = s4;
      s1 = s5;
      s2 = s6;
      s3 = s7;
      s4 = s8;
      s5 = s9;
      s6 = s10;
      src += 4;
      dst += 4;
      w -= 4;
    } while (w != 0);
  } else {
    uint8x8_t t0, t1, t2, t3, t4, t5, t6, t7, d0, d1, d2, d3;
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10;

    if (w == 4) {
      do {
        load_u8_8x8(src, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);
        transpose_u8_8x8(&t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);
        s0 = vreinterpretq_s16_u16(vmovl_u8(t0));
        s1 = vreinterpretq_s16_u16(vmovl_u8(t1));
        s2 = vreinterpretq_s16_u16(vmovl_u8(t2));
        s3 = vreinterpretq_s16_u16(vmovl_u8(t3));
        s4 = vreinterpretq_s16_u16(vmovl_u8(t4));
        s5 = vreinterpretq_s16_u16(vmovl_u8(t5));
        s6 = vreinterpretq_s16_u16(vmovl_u8(t6));

        load_u8_8x8(src + 7, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6,
                    &t7);
        transpose_u8_4x8(&t0, &t1, &t2, &t3, t4, t5, t6, t7);
        s7 = vreinterpretq_s16_u16(vmovl_u8(t0));
        s8 = vreinterpretq_s16_u16(vmovl_u8(t1));
        s9 = vreinterpretq_s16_u16(vmovl_u8(t2));
        s10 = vreinterpretq_s16_u16(vmovl_u8(t3));

        d0 = convolve8_8(s0, s1, s2, s3, s4, s5, s6, s7, filter);
        d1 = convolve8_8(s1, s2, s3, s4, s5, s6, s7, s8, filter);
        d2 = convolve8_8(s2, s3, s4, s5, s6, s7, s8, s9, filter);
        d3 = convolve8_8(s3, s4, s5, s6, s7, s8, s9, s10, filter);

        transpose_u8_8x4(&d0, &d1, &d2, &d3);

        store_u8_4x1(dst + 0 * dst_stride, d0, 0);
        store_u8_4x1(dst + 1 * dst_stride, d1, 0);
        store_u8_4x1(dst + 2 * dst_stride, d2, 0);
        store_u8_4x1(dst + 3 * dst_stride, d3, 0);
        store_u8_4x1(dst + 4 * dst_stride, d0, 1);
        store_u8_4x1(dst + 5 * dst_stride, d1, 1);
        store_u8_4x1(dst + 6 * dst_stride, d2, 1);
        store_u8_4x1(dst + 7 * dst_stride, d3, 1);

        src += 8 * src_stride;
        dst += 8 * dst_stride;
        h -= 8;
      } while (h > 0);
    } else {
      uint8x8_t d4, d5, d6, d7;
      int16x8_t s11, s12, s13, s14;
      int width;
      const uint8_t *s;
      uint8_t *d;

      do {
        load_u8_8x8(src, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);
        transpose_u8_8x8(&t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);
        s0 = vreinterpretq_s16_u16(vmovl_u8(t0));
        s1 = vreinterpretq_s16_u16(vmovl_u8(t1));
        s2 = vreinterpretq_s16_u16(vmovl_u8(t2));
        s3 = vreinterpretq_s16_u16(vmovl_u8(t3));
        s4 = vreinterpretq_s16_u16(vmovl_u8(t4));
        s5 = vreinterpretq_s16_u16(vmovl_u8(t5));
        s6 = vreinterpretq_s16_u16(vmovl_u8(t6));

        width = w;
        s = src + 7;
        d = dst;

        do {
          load_u8_8x8(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);
          transpose_u8_8x8(&t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);
          s7 = vreinterpretq_s16_u16(vmovl_u8(t0));
          s8 = vreinterpretq_s16_u16(vmovl_u8(t1));
          s9 = vreinterpretq_s16_u16(vmovl_u8(t2));
          s10 = vreinterpretq_s16_u16(vmovl_u8(t3));
          s11 = vreinterpretq_s16_u16(vmovl_u8(t4));
          s12 = vreinterpretq_s16_u16(vmovl_u8(t5));
          s13 = vreinterpretq_s16_u16(vmovl_u8(t6));
          s14 = vreinterpretq_s16_u16(vmovl_u8(t7));

          d0 = convolve8_8(s0, s1, s2, s3, s4, s5, s6, s7, filter);
          d1 = convolve8_8(s1, s2, s3, s4, s5, s6, s7, s8, filter);
          d2 = convolve8_8(s2, s3, s4, s5, s6, s7, s8, s9, filter);
          d3 = convolve8_8(s3, s4, s5, s6, s7, s8, s9, s10, filter);
          d4 = convolve8_8(s4, s5, s6, s7, s8, s9, s10, s11, filter);
          d5 = convolve8_8(s5, s6, s7, s8, s9, s10, s11, s12, filter);
          d6 = convolve8_8(s6, s7, s8, s9, s10, s11, s12, s13, filter);
          d7 = convolve8_8(s7, s8, s9, s10, s11, s12, s13, s14, filter);

          transpose_u8_8x8(&d0, &d1, &d2, &d3, &d4, &d5, &d6, &d7);

          store_u8_8x8(d, dst_stride, d0, d1, d2, d3, d4, d5, d6, d7);

          s0 = s8;
          s1 = s9;
          s2 = s10;
          s3 = s11;
          s4 = s12;
          s5 = s13;
          s6 = s14;
          s += 8;
          d += 8;
          width -= 8;
        } while (width != 0);
        src += 8 * src_stride;
        dst += 8 * dst_stride;
        h -= 8;
      } while (h > 0);
    }
  }
}

void aom_convolve8_vert_neon(const uint8_t *src, ptrdiff_t src_stride,
                             uint8_t *dst, ptrdiff_t dst_stride,
                             const int16_t *filter_x, int x_step_q4,
                             const int16_t *filter_y, int y_step_q4, int w,
                             int h) {
  const int16x8_t filter = vld1q_s16(filter_y);

  assert((intptr_t)dst % 4 == 0);
  assert(dst_stride % 4 == 0);

  (void)filter_x;
  (void)x_step_q4;
  (void)y_step_q4;

  src -= ((SUBPEL_TAPS / 2) - 1) * src_stride;

  if (w == 4) {
    uint8x8_t t0, t1, t2, t3, t4, t5, t6, d01, d23;
    int16x4_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, d0, d1, d2, d3;

    load_u8_8x7(src, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6);
    s0 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
    s1 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t1)));
    s2 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t2)));
    s3 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t3)));
    s4 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t4)));
    s5 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t5)));
    s6 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t6)));

    src += 7 * src_stride;

    do {
      load_u8_8x4(src, src_stride, &t0, &t1, &t2, &t3);
      s7 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
      s8 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t1)));
      s9 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t2)));
      s10 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t3)));

      d0 = convolve8_4(s0, s1, s2, s3, s4, s5, s6, s7, filter);
      d1 = convolve8_4(s1, s2, s3, s4, s5, s6, s7, s8, filter);
      d2 = convolve8_4(s2, s3, s4, s5, s6, s7, s8, s9, filter);
      d3 = convolve8_4(s3, s4, s5, s6, s7, s8, s9, s10, filter);
      d01 = vqrshrun_n_s16(vcombine_s16(d0, d1), FILTER_BITS);
      d23 = vqrshrun_n_s16(vcombine_s16(d2, d3), FILTER_BITS);

      store_u8_4x1(dst + 0 * dst_stride, d01, 0);
      store_u8_4x1(dst + 1 * dst_stride, d01, 1);
      store_u8_4x1(dst + 2 * dst_stride, d23, 0);
      store_u8_4x1(dst + 3 * dst_stride, d23, 1);

      s0 = s4;
      s1 = s5;
      s2 = s6;
      s3 = s7;
      s4 = s8;
      s5 = s9;
      s6 = s10;
      src += 4 * src_stride;
      dst += 4 * dst_stride;
      h -= 4;
    } while (h != 0);
  } else {
    uint8x8_t t0, t1, t2, t3, t4, t5, t6, d0, d1, d2, d3;
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10;
    int height;
    const uint8_t *s;
    uint8_t *d;

    do {
      load_u8_8x7(src, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6);
      s0 = vreinterpretq_s16_u16(vmovl_u8(t0));
      s1 = vreinterpretq_s16_u16(vmovl_u8(t1));
      s2 = vreinterpretq_s16_u16(vmovl_u8(t2));
      s3 = vreinterpretq_s16_u16(vmovl_u8(t3));
      s4 = vreinterpretq_s16_u16(vmovl_u8(t4));
      s5 = vreinterpretq_s16_u16(vmovl_u8(t5));
      s6 = vreinterpretq_s16_u16(vmovl_u8(t6));

      height = h;
      s = src + 7 * src_stride;
      d = dst;

      do {
        load_u8_8x4(s, src_stride, &t0, &t1, &t2, &t3);
        s7 = vreinterpretq_s16_u16(vmovl_u8(t0));
        s8 = vreinterpretq_s16_u16(vmovl_u8(t1));
        s9 = vreinterpretq_s16_u16(vmovl_u8(t2));
        s10 = vreinterpretq_s16_u16(vmovl_u8(t3));

        d0 = convolve8_8(s0, s1, s2, s3, s4, s5, s6, s7, filter);
        d1 = convolve8_8(s1, s2, s3, s4, s5, s6, s7, s8, filter);
        d2 = convolve8_8(s2, s3, s4, s5, s6, s7, s8, s9, filter);
        d3 = convolve8_8(s3, s4, s5, s6, s7, s8, s9, s10, filter);

        store_u8_8x4(d, dst_stride, d0, d1, d2, d3);

        s0 = s4;
        s1 = s5;
        s2 = s6;
        s3 = s7;
        s4 = s8;
        s5 = s9;
        s6 = s10;
        s += 4 * src_stride;
        d += 4 * dst_stride;
        height -= 4;
      } while (height != 0);
      src += 8;
      dst += 8;
      w -= 8;
    } while (w != 0);
  }
}

#endif  // AOM_ARCH_AARCH64 && defined(__ARM_FEATURE_DOTPROD)
