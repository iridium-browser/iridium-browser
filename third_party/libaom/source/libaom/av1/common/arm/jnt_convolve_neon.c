/*
 * Copyright (c) 2018, Alliance for Open Media. All rights reserved
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

#include "config/aom_config.h"
#include "config/av1_rtcd.h"

#include "aom_dsp/txfm_common.h"
#include "aom_dsp/arm/mem_neon.h"
#include "aom_dsp/arm/transpose_neon.h"
#include "aom_ports/mem.h"
#include "av1/common/common.h"
#include "av1/common/arm/convolve_neon.h"

#if !AOM_ARCH_AARCH64
static INLINE void compute_dist_wtd_avg_4x1(uint16x4_t dd0, uint16x4_t d0,
                                            const uint16_t fwd_offset,
                                            const uint16_t bck_offset,
                                            const int16x4_t round_offset,
                                            uint8x8_t *d0_u8) {
  uint32x4_t blend0 = vmull_n_u16(dd0, fwd_offset);
  blend0 = vmlal_n_u16(blend0, d0, bck_offset);

  uint16x4_t avg0 = vshrn_n_u32(blend0, DIST_PRECISION_BITS);

  int16x4_t dst0 = vsub_s16(vreinterpret_s16_u16(avg0), round_offset);

  int16x8_t dst0q = vcombine_s16(dst0, vdup_n_s16(0));

  *d0_u8 = vqrshrun_n_s16(dst0q, FILTER_BITS - ROUND0_BITS);
}

static INLINE void compute_basic_avg_4x1(uint16x4_t dd0, uint16x4_t d0,
                                         const int16x4_t round_offset,
                                         uint8x8_t *d0_u8) {
  uint16x4_t avg0 = vhadd_u16(dd0, d0);

  int16x4_t dst0 = vsub_s16(vreinterpret_s16_u16(avg0), round_offset);

  int16x8_t dst0q = vcombine_s16(dst0, vdup_n_s16(0));

  *d0_u8 = vqrshrun_n_s16(dst0q, FILTER_BITS - ROUND0_BITS);
}

static INLINE void compute_dist_wtd_avg_8x1(uint16x8_t dd0, uint16x8_t d0,
                                            const uint16_t fwd_offset,
                                            const uint16_t bck_offset,
                                            const int16x8_t round_offset,
                                            uint8x8_t *d0_u8) {
  uint32x4_t blend0_lo = vmull_n_u16(vget_low_u16(dd0), fwd_offset);
  blend0_lo = vmlal_n_u16(blend0_lo, vget_low_u16(d0), bck_offset);
  uint32x4_t blend0_hi = vmull_n_u16(vget_high_u16(dd0), fwd_offset);
  blend0_hi = vmlal_n_u16(blend0_hi, vget_high_u16(d0), bck_offset);

  uint16x8_t avg0 = vcombine_u16(vshrn_n_u32(blend0_lo, DIST_PRECISION_BITS),
                                 vshrn_n_u32(blend0_hi, DIST_PRECISION_BITS));

  int16x8_t dst0 = vsubq_s16(vreinterpretq_s16_u16(avg0), round_offset);

  *d0_u8 = vqrshrun_n_s16(dst0, FILTER_BITS - ROUND0_BITS);
}

static INLINE void compute_basic_avg_8x1(uint16x8_t dd0, uint16x8_t d0,
                                         const int16x8_t round_offset,
                                         uint8x8_t *d0_u8) {
  uint16x8_t avg0 = vhaddq_u16(dd0, d0);

  int16x8_t dst0 = vsubq_s16(vreinterpretq_s16_u16(avg0), round_offset);

  *d0_u8 = vqrshrun_n_s16(dst0, FILTER_BITS - ROUND0_BITS);
}

#endif  // !AOM_ARCH_AARCH64

static INLINE void compute_dist_wtd_avg_4x4(
    uint16x4_t dd0, uint16x4_t dd1, uint16x4_t dd2, uint16x4_t dd3,
    uint16x4_t d0, uint16x4_t d1, uint16x4_t d2, uint16x4_t d3,
    const uint16_t fwd_offset, const uint16_t bck_offset,
    const int16x8_t round_offset, uint8x8_t *d01_u8, uint8x8_t *d23_u8) {
  uint32x4_t blend0 = vmull_n_u16(dd0, fwd_offset);
  blend0 = vmlal_n_u16(blend0, d0, bck_offset);
  uint32x4_t blend1 = vmull_n_u16(dd1, fwd_offset);
  blend1 = vmlal_n_u16(blend1, d1, bck_offset);
  uint32x4_t blend2 = vmull_n_u16(dd2, fwd_offset);
  blend2 = vmlal_n_u16(blend2, d2, bck_offset);
  uint32x4_t blend3 = vmull_n_u16(dd3, fwd_offset);
  blend3 = vmlal_n_u16(blend3, d3, bck_offset);

  uint16x4_t avg0 = vshrn_n_u32(blend0, DIST_PRECISION_BITS);
  uint16x4_t avg1 = vshrn_n_u32(blend1, DIST_PRECISION_BITS);
  uint16x4_t avg2 = vshrn_n_u32(blend2, DIST_PRECISION_BITS);
  uint16x4_t avg3 = vshrn_n_u32(blend3, DIST_PRECISION_BITS);

  int16x8_t dst_01 = vreinterpretq_s16_u16(vcombine_u16(avg0, avg1));
  int16x8_t dst_23 = vreinterpretq_s16_u16(vcombine_u16(avg2, avg3));

  dst_01 = vsubq_s16(dst_01, round_offset);
  dst_23 = vsubq_s16(dst_23, round_offset);

  *d01_u8 = vqrshrun_n_s16(dst_01, FILTER_BITS - ROUND0_BITS);
  *d23_u8 = vqrshrun_n_s16(dst_23, FILTER_BITS - ROUND0_BITS);
}

static INLINE void compute_basic_avg_4x4(uint16x4_t dd0, uint16x4_t dd1,
                                         uint16x4_t dd2, uint16x4_t dd3,
                                         uint16x4_t d0, uint16x4_t d1,
                                         uint16x4_t d2, uint16x4_t d3,
                                         const int16x8_t round_offset,
                                         uint8x8_t *d01_u8, uint8x8_t *d23_u8) {
  uint16x4_t avg0 = vhadd_u16(dd0, d0);
  uint16x4_t avg1 = vhadd_u16(dd1, d1);
  uint16x4_t avg2 = vhadd_u16(dd2, d2);
  uint16x4_t avg3 = vhadd_u16(dd3, d3);

  int16x8_t dst_01 = vreinterpretq_s16_u16(vcombine_u16(avg0, avg1));
  int16x8_t dst_23 = vreinterpretq_s16_u16(vcombine_u16(avg2, avg3));

  dst_01 = vsubq_s16(dst_01, round_offset);
  dst_23 = vsubq_s16(dst_23, round_offset);

  *d01_u8 = vqrshrun_n_s16(dst_01, FILTER_BITS - ROUND0_BITS);
  *d23_u8 = vqrshrun_n_s16(dst_23, FILTER_BITS - ROUND0_BITS);
}

static INLINE void compute_dist_wtd_avg_8x4(
    uint16x8_t dd0, uint16x8_t dd1, uint16x8_t dd2, uint16x8_t dd3,
    uint16x8_t d0, uint16x8_t d1, uint16x8_t d2, uint16x8_t d3,
    const uint16_t fwd_offset, const uint16_t bck_offset,
    const int16x8_t round_offset, uint8x8_t *d0_u8, uint8x8_t *d1_u8,
    uint8x8_t *d2_u8, uint8x8_t *d3_u8) {
  uint32x4_t blend0_lo = vmull_n_u16(vget_low_u16(dd0), fwd_offset);
  blend0_lo = vmlal_n_u16(blend0_lo, vget_low_u16(d0), bck_offset);
  uint32x4_t blend0_hi = vmull_n_u16(vget_high_u16(dd0), fwd_offset);
  blend0_hi = vmlal_n_u16(blend0_hi, vget_high_u16(d0), bck_offset);

  uint32x4_t blend1_lo = vmull_n_u16(vget_low_u16(dd1), fwd_offset);
  blend1_lo = vmlal_n_u16(blend1_lo, vget_low_u16(d1), bck_offset);
  uint32x4_t blend1_hi = vmull_n_u16(vget_high_u16(dd1), fwd_offset);
  blend1_hi = vmlal_n_u16(blend1_hi, vget_high_u16(d1), bck_offset);

  uint32x4_t blend2_lo = vmull_n_u16(vget_low_u16(dd2), fwd_offset);
  blend2_lo = vmlal_n_u16(blend2_lo, vget_low_u16(d2), bck_offset);
  uint32x4_t blend2_hi = vmull_n_u16(vget_high_u16(dd2), fwd_offset);
  blend2_hi = vmlal_n_u16(blend2_hi, vget_high_u16(d2), bck_offset);

  uint32x4_t blend3_lo = vmull_n_u16(vget_low_u16(dd3), fwd_offset);
  blend3_lo = vmlal_n_u16(blend3_lo, vget_low_u16(d3), bck_offset);
  uint32x4_t blend3_hi = vmull_n_u16(vget_high_u16(dd3), fwd_offset);
  blend3_hi = vmlal_n_u16(blend3_hi, vget_high_u16(d3), bck_offset);

  uint16x8_t avg0 = vcombine_u16(vshrn_n_u32(blend0_lo, DIST_PRECISION_BITS),
                                 vshrn_n_u32(blend0_hi, DIST_PRECISION_BITS));
  uint16x8_t avg1 = vcombine_u16(vshrn_n_u32(blend1_lo, DIST_PRECISION_BITS),
                                 vshrn_n_u32(blend1_hi, DIST_PRECISION_BITS));
  uint16x8_t avg2 = vcombine_u16(vshrn_n_u32(blend2_lo, DIST_PRECISION_BITS),
                                 vshrn_n_u32(blend2_hi, DIST_PRECISION_BITS));
  uint16x8_t avg3 = vcombine_u16(vshrn_n_u32(blend3_lo, DIST_PRECISION_BITS),
                                 vshrn_n_u32(blend3_hi, DIST_PRECISION_BITS));

  int16x8_t dst0 = vsubq_s16(vreinterpretq_s16_u16(avg0), round_offset);
  int16x8_t dst1 = vsubq_s16(vreinterpretq_s16_u16(avg1), round_offset);
  int16x8_t dst2 = vsubq_s16(vreinterpretq_s16_u16(avg2), round_offset);
  int16x8_t dst3 = vsubq_s16(vreinterpretq_s16_u16(avg3), round_offset);

  *d0_u8 = vqrshrun_n_s16(dst0, FILTER_BITS - ROUND0_BITS);
  *d1_u8 = vqrshrun_n_s16(dst1, FILTER_BITS - ROUND0_BITS);
  *d2_u8 = vqrshrun_n_s16(dst2, FILTER_BITS - ROUND0_BITS);
  *d3_u8 = vqrshrun_n_s16(dst3, FILTER_BITS - ROUND0_BITS);
}

static INLINE void compute_basic_avg_8x4(uint16x8_t dd0, uint16x8_t dd1,
                                         uint16x8_t dd2, uint16x8_t dd3,
                                         uint16x8_t d0, uint16x8_t d1,
                                         uint16x8_t d2, uint16x8_t d3,
                                         const int16x8_t round_offset,
                                         uint8x8_t *d0_u8, uint8x8_t *d1_u8,
                                         uint8x8_t *d2_u8, uint8x8_t *d3_u8) {
  uint16x8_t avg0, avg1, avg2, avg3;

  avg0 = vhaddq_u16(dd0, d0);
  avg1 = vhaddq_u16(dd1, d1);
  avg2 = vhaddq_u16(dd2, d2);
  avg3 = vhaddq_u16(dd3, d3);

  int16x8_t dst0 = vsubq_s16(vreinterpretq_s16_u16(avg0), round_offset);
  int16x8_t dst1 = vsubq_s16(vreinterpretq_s16_u16(avg1), round_offset);
  int16x8_t dst2 = vsubq_s16(vreinterpretq_s16_u16(avg2), round_offset);
  int16x8_t dst3 = vsubq_s16(vreinterpretq_s16_u16(avg3), round_offset);

  *d0_u8 = vqrshrun_n_s16(dst0, FILTER_BITS - ROUND0_BITS);
  *d1_u8 = vqrshrun_n_s16(dst1, FILTER_BITS - ROUND0_BITS);
  *d2_u8 = vqrshrun_n_s16(dst2, FILTER_BITS - ROUND0_BITS);
  *d3_u8 = vqrshrun_n_s16(dst3, FILTER_BITS - ROUND0_BITS);
}

#if AOM_ARCH_AARCH64 && defined(__ARM_FEATURE_MATMUL_INT8)

static INLINE int16x4_t convolve8_4_2d_h(uint8x16_t samples,
                                         const int8x8_t x_filter,
                                         const uint8x16x2_t permute_tbl,
                                         const int32x4_t horiz_const) {
  uint8x16_t permuted_samples[2];
  int32x4_t sum;

  // Permute samples ready for dot product.
  // { 0,  1,  2,  3,  1,  2,  3,  4,  2,  3,  4,  5,  3,  4,  5,  6 }
  permuted_samples[0] = vqtbl1q_u8(samples, permute_tbl.val[0]);
  // { 4,  5,  6,  7,  5,  6,  7,  8,  6,  7,  8,  9,  7,  8,  9, 10 }
  permuted_samples[1] = vqtbl1q_u8(samples, permute_tbl.val[1]);

  // First 4 output values.
  sum = vusdotq_lane_s32(horiz_const, permuted_samples[0], x_filter, 0);
  sum = vusdotq_lane_s32(sum, permuted_samples[1], x_filter, 1);

  // We halved the convolution filter values so -1 from the right shift.
  return vshrn_n_s32(sum, ROUND0_BITS - 1);
}

static INLINE int16x8_t convolve8_8_2d_h(uint8x16_t samples,
                                         const int8x8_t x_filter,
                                         const uint8x16x3_t permute_tbl,
                                         const int32x4_t horiz_const) {
  uint8x16_t permuted_samples[3];
  int32x4_t sum[2];

  // Permute samples ready for dot product.
  // { 0,  1,  2,  3,  1,  2,  3,  4,  2,  3,  4,  5,  3,  4,  5,  6 }
  permuted_samples[0] = vqtbl1q_u8(samples, permute_tbl.val[0]);
  // { 4,  5,  6,  7,  5,  6,  7,  8,  6,  7,  8,  9,  7,  8,  9, 10 }
  permuted_samples[1] = vqtbl1q_u8(samples, permute_tbl.val[1]);
  // { 8,  9, 10, 11,  9, 10, 11, 12, 10, 11, 12, 13, 11, 12, 13, 14 }
  permuted_samples[2] = vqtbl1q_u8(samples, permute_tbl.val[2]);

  // First 4 output values.
  sum[0] = vusdotq_lane_s32(horiz_const, permuted_samples[0], x_filter, 0);
  sum[0] = vusdotq_lane_s32(sum[0], permuted_samples[1], x_filter, 1);
  // Second 4 output values.
  sum[1] = vusdotq_lane_s32(horiz_const, permuted_samples[1], x_filter, 0);
  sum[1] = vusdotq_lane_s32(sum[1], permuted_samples[2], x_filter, 1);

  // Narrow and re-pack.
  // We halved the convolution filter values so -1 from the right shift.
  return vcombine_s16(vshrn_n_s32(sum[0], ROUND0_BITS - 1),
                      vshrn_n_s32(sum[1], ROUND0_BITS - 1));
}

static INLINE void dist_wtd_convolve_2d_horiz_8tap_neon(
    const uint8_t *src, int src_stride, int16_t *im_block, const int im_stride,
    const int16x8_t x_filter_s16, const int im_h, int w) {
  const int bd = 8;
  // A shim of 1 << ((ROUND0_BITS - 1) - 1) enables us to use non-rounding
  // shifts - which are generally faster than rounding shifts on modern CPUs.
  // (The extra -1 is needed because we halved the filter values.)
  const int32x4_t horiz_const = vdupq_n_s32((1 << (bd + FILTER_BITS - 2)) +
                                            (1 << ((ROUND0_BITS - 1) - 1)));
  // Horizontal filter.
  const int8x8_t x_filter = vmovn_s16(x_filter_s16);

  const uint8_t *src_ptr = src;
  int16_t *dst_ptr = im_block;
  int dst_stride = im_stride;
  int height = im_h;

  if (w == 4) {
    const uint8x16x2_t permute_tbl = vld1q_u8_x2(dot_prod_permute_tbl);

    do {
      uint8x16_t s0, s1, s2, s3;
      load_u8_16x4(src_ptr, src_stride, &s0, &s1, &s2, &s3);

      int16x4_t d0 = convolve8_4_2d_h(s0, x_filter, permute_tbl, horiz_const);
      int16x4_t d1 = convolve8_4_2d_h(s1, x_filter, permute_tbl, horiz_const);
      int16x4_t d2 = convolve8_4_2d_h(s2, x_filter, permute_tbl, horiz_const);
      int16x4_t d3 = convolve8_4_2d_h(s3, x_filter, permute_tbl, horiz_const);

      store_s16_4x4(dst_ptr, dst_stride, d0, d1, d2, d3);

      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      height -= 4;
    } while (height > 0);
  } else {
    const uint8x16x3_t permute_tbl = vld1q_u8_x3(dot_prod_permute_tbl);

    do {
      const uint8_t *s = src_ptr;
      int16_t *d = dst_ptr;
      int width = w;

      do {
        uint8x16_t s0, s1, s2, s3;
        load_u8_16x4(s, src_stride, &s0, &s1, &s2, &s3);

        int16x8_t d0 = convolve8_8_2d_h(s0, x_filter, permute_tbl, horiz_const);
        int16x8_t d1 = convolve8_8_2d_h(s1, x_filter, permute_tbl, horiz_const);
        int16x8_t d2 = convolve8_8_2d_h(s2, x_filter, permute_tbl, horiz_const);
        int16x8_t d3 = convolve8_8_2d_h(s3, x_filter, permute_tbl, horiz_const);

        store_s16_8x4(d, dst_stride, d0, d1, d2, d3);

        s += 8;
        d += 8;
        width -= 8;
      } while (width > 0);
      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      height -= 4;
    } while (height > 0);
  }
}

#elif AOM_ARCH_AARCH64 && defined(__ARM_FEATURE_DOTPROD)

static INLINE int16x4_t convolve8_4_2d_h(uint8x16_t samples,
                                         const int8x8_t x_filter,
                                         const int32x4_t correction,
                                         const uint8x16_t range_limit,
                                         const uint8x16x2_t permute_tbl) {
  int8x16_t clamped_samples, permuted_samples[2];
  int32x4_t sum;

  // Clamp sample range to [-128, 127] for 8-bit signed dot product.
  clamped_samples = vreinterpretq_s8_u8(vsubq_u8(samples, range_limit));

  // Permute samples ready for dot product.
  // { 0,  1,  2,  3,  1,  2,  3,  4,  2,  3,  4,  5,  3,  4,  5,  6 }
  permuted_samples[0] = vqtbl1q_s8(clamped_samples, permute_tbl.val[0]);
  // { 4,  5,  6,  7,  5,  6,  7,  8,  6,  7,  8,  9,  7,  8,  9, 10 }
  permuted_samples[1] = vqtbl1q_s8(clamped_samples, permute_tbl.val[1]);

  // Accumulate dot product into 'correction' to account for range clamp.
  sum = vdotq_lane_s32(correction, permuted_samples[0], x_filter, 0);
  sum = vdotq_lane_s32(sum, permuted_samples[1], x_filter, 1);

  // We halved the convolution filter values so -1 from the right shift.
  return vshrn_n_s32(sum, ROUND0_BITS - 1);
}

static INLINE int16x8_t convolve8_8_2d_h(uint8x16_t samples,
                                         const int8x8_t x_filter,
                                         const int32x4_t correction,
                                         const uint8x16_t range_limit,
                                         const uint8x16x3_t permute_tbl) {
  int8x16_t clamped_samples, permuted_samples[3];
  int32x4_t sum[2];

  // Clamp sample range to [-128, 127] for 8-bit signed dot product.
  clamped_samples = vreinterpretq_s8_u8(vsubq_u8(samples, range_limit));

  // Permute samples ready for dot product. */
  // { 0,  1,  2,  3,  1,  2,  3,  4,  2,  3,  4,  5,  3,  4,  5,  6 }
  permuted_samples[0] = vqtbl1q_s8(clamped_samples, permute_tbl.val[0]);
  // { 4,  5,  6,  7,  5,  6,  7,  8,  6,  7,  8,  9,  7,  8,  9, 10 }
  permuted_samples[1] = vqtbl1q_s8(clamped_samples, permute_tbl.val[1]);
  // { 8,  9, 10, 11,  9, 10, 11, 12, 10, 11, 12, 13, 11, 12, 13, 14 }
  permuted_samples[2] = vqtbl1q_s8(clamped_samples, permute_tbl.val[2]);

  // Accumulate dot product into 'correction' to account for range clamp.
  // First 4 output values.
  sum[0] = vdotq_lane_s32(correction, permuted_samples[0], x_filter, 0);
  sum[0] = vdotq_lane_s32(sum[0], permuted_samples[1], x_filter, 1);
  // Second 4 output values.
  sum[1] = vdotq_lane_s32(correction, permuted_samples[1], x_filter, 0);
  sum[1] = vdotq_lane_s32(sum[1], permuted_samples[2], x_filter, 1);

  // Narrow and re-pack.
  // We halved the convolution filter values so -1 from the right shift.
  return vcombine_s16(vshrn_n_s32(sum[0], ROUND0_BITS - 1),
                      vshrn_n_s32(sum[1], ROUND0_BITS - 1));
}

static INLINE void dist_wtd_convolve_2d_horiz_8tap_neon(
    const uint8_t *src, int src_stride, int16_t *im_block, const int im_stride,
    const int16x8_t x_filter_s16, const int im_h, int w) {
  const int bd = 8;
  const int32_t horiz_const = (1 << (bd + FILTER_BITS - 2));
  // Dot product constants and other shims.
  const int32_t correction_s32 = vaddlvq_s16(vshlq_n_s16(x_filter_s16, 7));
  // Fold horiz_const into the dot-product filter correction constant. The
  // additional shim of 1 << ((ROUND0_BITS - 1) - 1) enables us to use non-
  // rounding shifts - which are generally faster than rounding shifts on
  // modern CPUs. (The extra -1 is needed because we halved the filter values.)
  const int32x4_t correction = vdupq_n_s32(correction_s32 + horiz_const +
                                           (1 << ((ROUND0_BITS - 1) - 1)));
  const uint8x16_t range_limit = vdupq_n_u8(128);
  // Horizontal filter.
  const int8x8_t x_filter = vmovn_s16(x_filter_s16);

  const uint8_t *src_ptr = src;
  int16_t *dst_ptr = im_block;
  int dst_stride = im_stride;
  int height = im_h;

  if (w == 4) {
    const uint8x16x2_t permute_tbl = vld1q_u8_x2(dot_prod_permute_tbl);

    do {
      uint8x16_t s0, s1, s2, s3;
      load_u8_16x4(src_ptr, src_stride, &s0, &s1, &s2, &s3);

      int16x4_t d0 =
          convolve8_4_2d_h(s0, x_filter, correction, range_limit, permute_tbl);
      int16x4_t d1 =
          convolve8_4_2d_h(s1, x_filter, correction, range_limit, permute_tbl);
      int16x4_t d2 =
          convolve8_4_2d_h(s2, x_filter, correction, range_limit, permute_tbl);
      int16x4_t d3 =
          convolve8_4_2d_h(s3, x_filter, correction, range_limit, permute_tbl);

      store_s16_4x4(dst_ptr, dst_stride, d0, d1, d2, d3);

      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      height -= 4;
    } while (height > 0);
  } else {
    const uint8x16x3_t permute_tbl = vld1q_u8_x3(dot_prod_permute_tbl);

    do {
      const uint8_t *s = src_ptr;
      int16_t *d = dst_ptr;
      int width = w;

      do {
        uint8x16_t s0, s1, s2, s3;
        load_u8_16x4(s, src_stride, &s0, &s1, &s2, &s3);

        int16x8_t d0 = convolve8_8_2d_h(s0, x_filter, correction, range_limit,
                                        permute_tbl);
        int16x8_t d1 = convolve8_8_2d_h(s1, x_filter, correction, range_limit,
                                        permute_tbl);
        int16x8_t d2 = convolve8_8_2d_h(s2, x_filter, correction, range_limit,
                                        permute_tbl);
        int16x8_t d3 = convolve8_8_2d_h(s3, x_filter, correction, range_limit,
                                        permute_tbl);

        store_s16_8x4(d, dst_stride, d0, d1, d2, d3);

        s += 8;
        d += 8;
        width -= 8;
      } while (width > 0);
      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      height -= 4;
    } while (height > 0);
  }
}

#else  // !(AOM_ARCH_AARCH64 && defined(__ARM_FEATURE_DOTPROD))

static INLINE int16x4_t convolve8_4_2d_h(const int16x4_t s0, const int16x4_t s1,
                                         const int16x4_t s2, const int16x4_t s3,
                                         const int16x4_t s4, const int16x4_t s5,
                                         const int16x4_t s6, const int16x4_t s7,
                                         const int16x8_t x_filter,
                                         const int16x4_t horiz_const) {
  const int16x4_t x_filter_0_3 = vget_low_s16(x_filter);
  const int16x4_t x_filter_4_7 = vget_high_s16(x_filter);

  int16x4_t sum = horiz_const;
  sum = vmla_lane_s16(sum, s0, x_filter_0_3, 0);
  sum = vmla_lane_s16(sum, s1, x_filter_0_3, 1);
  sum = vmla_lane_s16(sum, s2, x_filter_0_3, 2);
  sum = vmla_lane_s16(sum, s3, x_filter_0_3, 3);
  sum = vmla_lane_s16(sum, s4, x_filter_4_7, 0);
  sum = vmla_lane_s16(sum, s5, x_filter_4_7, 1);
  sum = vmla_lane_s16(sum, s6, x_filter_4_7, 2);
  sum = vmla_lane_s16(sum, s7, x_filter_4_7, 3);

  // We halved the convolution filter values so -1 from the right shift.
  return vshr_n_s16(sum, ROUND0_BITS - 1);
}

static INLINE int16x8_t convolve8_8_2d_h(const int16x8_t s0, const int16x8_t s1,
                                         const int16x8_t s2, const int16x8_t s3,
                                         const int16x8_t s4, const int16x8_t s5,
                                         const int16x8_t s6, const int16x8_t s7,
                                         const int16x8_t x_filter,
                                         const int16x8_t horiz_const) {
  const int16x4_t x_filter_0_3 = vget_low_s16(x_filter);
  const int16x4_t x_filter_4_7 = vget_high_s16(x_filter);

  int16x8_t sum = horiz_const;
  sum = vmlaq_lane_s16(sum, s0, x_filter_0_3, 0);
  sum = vmlaq_lane_s16(sum, s1, x_filter_0_3, 1);
  sum = vmlaq_lane_s16(sum, s2, x_filter_0_3, 2);
  sum = vmlaq_lane_s16(sum, s3, x_filter_0_3, 3);
  sum = vmlaq_lane_s16(sum, s4, x_filter_4_7, 0);
  sum = vmlaq_lane_s16(sum, s5, x_filter_4_7, 1);
  sum = vmlaq_lane_s16(sum, s6, x_filter_4_7, 2);
  sum = vmlaq_lane_s16(sum, s7, x_filter_4_7, 3);

  // We halved the convolution filter values so -1 from the right shift.
  return vshrq_n_s16(sum, ROUND0_BITS - 1);
}

static INLINE void dist_wtd_convolve_2d_horiz_8tap_neon(
    const uint8_t *src, int src_stride, int16_t *im_block, const int im_stride,
    const int16x8_t x_filter, const int im_h, int w) {
  const int bd = 8;

  const uint8_t *src_ptr = src;
  int16_t *dst_ptr = im_block;
  int dst_stride = im_stride;
  int height = im_h;

  if (w == 4) {
    // A shim of 1 << ((ROUND0_BITS - 1) - 1) enables us to use non-rounding
    // shifts - which are generally faster than rounding shifts on modern CPUs.
    // (The extra -1 is needed because we halved the filter values.)
    const int16x4_t horiz_const = vdup_n_s16((1 << (bd + FILTER_BITS - 2)) +
                                             (1 << ((ROUND0_BITS - 1) - 1)));
    do {
      __builtin_prefetch(src_ptr + 0 * src_stride);
#if AOM_ARCH_AARCH64
      __builtin_prefetch(src_ptr + 1 * src_stride);
      __builtin_prefetch(src_ptr + 2 * src_stride);
      __builtin_prefetch(src_ptr + 3 * src_stride);

      uint8x8_t t0, t1, t2, t3;
      load_u8_8x4(src_ptr, src_stride, &t0, &t1, &t2, &t3);
      transpose_u8_8x4(&t0, &t1, &t2, &t3);

      int16x4_t s0 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
      int16x4_t s1 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t1)));
      int16x4_t s2 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t2)));
      int16x4_t s3 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t3)));
      int16x4_t s4 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
      int16x4_t s5 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t1)));
      int16x4_t s6 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t2)));

      __builtin_prefetch(dst_ptr + 0 * dst_stride);
      __builtin_prefetch(dst_ptr + 1 * dst_stride);
      __builtin_prefetch(dst_ptr + 2 * dst_stride);
      __builtin_prefetch(dst_ptr + 3 * dst_stride);

      load_u8_8x4(src_ptr + 7, src_stride, &t0, &t1, &t2, &t3);
      transpose_u8_8x4(&t0, &t1, &t2, &t3);

      int16x4_t s7 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
      int16x4_t s8 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t1)));
      int16x4_t s9 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t2)));
      int16x4_t s10 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t3)));

      int16x4_t d0 = convolve8_4_2d_h(s0, s1, s2, s3, s4, s5, s6, s7, x_filter,
                                      horiz_const);
      int16x4_t d1 = convolve8_4_2d_h(s1, s2, s3, s4, s5, s6, s7, s8, x_filter,
                                      horiz_const);
      int16x4_t d2 = convolve8_4_2d_h(s2, s3, s4, s5, s6, s7, s8, s9, x_filter,
                                      horiz_const);
      int16x4_t d3 = convolve8_4_2d_h(s3, s4, s5, s6, s7, s8, s9, s10, x_filter,
                                      horiz_const);

      transpose_s16_4x4d(&d0, &d1, &d2, &d3);
      store_s16_4x4(dst_ptr, dst_stride, d0, d1, d2, d3);

      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      height -= 4;
#else   // !AOM_ARCH_AARCH64
      uint8x8_t t0 = vld1_u8(src_ptr);  // a0 a1 a2 a3 a4 a5 a6 a7
      int16x4_t s0 =
          vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));  // a0 a1 a2 a3
      int16x4_t s4 =
          vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));  // a4 a5 a6 a7

      __builtin_prefetch(dst_ptr);

      t0 = vld1_u8(src_ptr + 8);  // a8 a9 a10 a11
      int16x4_t s7 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));

      int16x4_t s1 = vext_s16(s0, s4, 1);  // a1 a2 a3 a4
      int16x4_t s2 = vext_s16(s0, s4, 2);  // a2 a3 a4 a5
      int16x4_t s3 = vext_s16(s0, s4, 3);  // a3 a4 a5 a6
      int16x4_t s5 = vext_s16(s4, s7, 1);  // a5 a6 a7 a8
      int16x4_t s6 = vext_s16(s4, s7, 2);  // a6 a7 a8 a9
      s7 = vext_s16(s4, s7, 3);            // a7 a8 a9 a10

      int16x4_t d0 = convolve8_4_2d_h(s0, s1, s2, s3, s4, s5, s6, s7, x_filter,
                                      horiz_const);
      vst1_s16(dst_ptr, d0);

      src_ptr += src_stride;
      dst_ptr += dst_stride;
      height--;
#endif  // AOM_ARCH_AARCH64
    } while (height > 0);
  } else {
    // A shim of 1 << ((ROUND0_BITS - 1) - 1) enables us to use non-rounding
    // shifts - which are generally faster than rounding shifts on modern CPUs.
    // (The extra -1 is needed because we halved the filter values.)
    const int16x8_t horiz_const = vdupq_n_s16((1 << (bd + FILTER_BITS - 2)) +
                                              (1 << ((ROUND0_BITS - 1) - 1)));
    do {
      const uint8_t *s;
      int16_t *d = dst_ptr;
      int width = w;

#if AOM_ARCH_AARCH64
      __builtin_prefetch(src_ptr + 0 * src_stride);
      __builtin_prefetch(src_ptr + 1 * src_stride);
      __builtin_prefetch(src_ptr + 2 * src_stride);
      __builtin_prefetch(src_ptr + 3 * src_stride);
      __builtin_prefetch(src_ptr + 4 * src_stride);
      __builtin_prefetch(src_ptr + 5 * src_stride);
      __builtin_prefetch(src_ptr + 6 * src_stride);
      __builtin_prefetch(src_ptr + 7 * src_stride);

      uint8x8_t t0, t1, t2, t3, t4, t5, t6, t7;
      load_u8_8x8(src_ptr, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);
      transpose_u8_8x8(&t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);

      int16x8_t s0 = vreinterpretq_s16_u16(vmovl_u8(t0));
      int16x8_t s1 = vreinterpretq_s16_u16(vmovl_u8(t1));
      int16x8_t s2 = vreinterpretq_s16_u16(vmovl_u8(t2));
      int16x8_t s3 = vreinterpretq_s16_u16(vmovl_u8(t3));
      int16x8_t s4 = vreinterpretq_s16_u16(vmovl_u8(t4));
      int16x8_t s5 = vreinterpretq_s16_u16(vmovl_u8(t5));
      int16x8_t s6 = vreinterpretq_s16_u16(vmovl_u8(t6));

      s = src_ptr + 7;

      __builtin_prefetch(dst_ptr + 0 * dst_stride);
      __builtin_prefetch(dst_ptr + 1 * dst_stride);
      __builtin_prefetch(dst_ptr + 2 * dst_stride);
      __builtin_prefetch(dst_ptr + 3 * dst_stride);
      __builtin_prefetch(dst_ptr + 4 * dst_stride);
      __builtin_prefetch(dst_ptr + 5 * dst_stride);
      __builtin_prefetch(dst_ptr + 6 * dst_stride);
      __builtin_prefetch(dst_ptr + 7 * dst_stride);

      do {
        load_u8_8x8(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);
        transpose_u8_8x8(&t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);

        int16x8_t s7 = vreinterpretq_s16_u16(vmovl_u8(t0));
        int16x8_t s8 = vreinterpretq_s16_u16(vmovl_u8(t1));
        int16x8_t s9 = vreinterpretq_s16_u16(vmovl_u8(t2));
        int16x8_t s10 = vreinterpretq_s16_u16(vmovl_u8(t3));
        int16x8_t s11 = vreinterpretq_s16_u16(vmovl_u8(t4));
        int16x8_t s12 = vreinterpretq_s16_u16(vmovl_u8(t5));
        int16x8_t s13 = vreinterpretq_s16_u16(vmovl_u8(t6));
        int16x8_t s14 = vreinterpretq_s16_u16(vmovl_u8(t7));

        int16x8_t d0 = convolve8_8_2d_h(s0, s1, s2, s3, s4, s5, s6, s7,
                                        x_filter, horiz_const);
        int16x8_t d1 = convolve8_8_2d_h(s1, s2, s3, s4, s5, s6, s7, s8,
                                        x_filter, horiz_const);
        int16x8_t d2 = convolve8_8_2d_h(s2, s3, s4, s5, s6, s7, s8, s9,
                                        x_filter, horiz_const);
        int16x8_t d3 = convolve8_8_2d_h(s3, s4, s5, s6, s7, s8, s9, s10,
                                        x_filter, horiz_const);
        int16x8_t d4 = convolve8_8_2d_h(s4, s5, s6, s7, s8, s9, s10, s11,
                                        x_filter, horiz_const);
        int16x8_t d5 = convolve8_8_2d_h(s5, s6, s7, s8, s9, s10, s11, s12,
                                        x_filter, horiz_const);
        int16x8_t d6 = convolve8_8_2d_h(s6, s7, s8, s9, s10, s11, s12, s13,
                                        x_filter, horiz_const);
        int16x8_t d7 = convolve8_8_2d_h(s7, s8, s9, s10, s11, s12, s13, s14,
                                        x_filter, horiz_const);

        transpose_s16_8x8(&d0, &d1, &d2, &d3, &d4, &d5, &d6, &d7);
        store_s16_8x8(d, dst_stride, d0, d1, d2, d3, d4, d5, d6, d7);

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
      } while (width > 0);
      src_ptr += 8 * src_stride;
      dst_ptr += 8 * dst_stride;
      height -= 8;
#else   // !AOM_ARCH_AARCH64
      uint8x8_t t0 = vld1_u8(src_ptr);
      int16x8_t s0 =
          vreinterpretq_s16_u16(vmovl_u8(t0));  // a0 a1 a2 a3 a4 a5 a6 a7

      s = src_ptr + 8;
      __builtin_prefetch(dst_ptr);

      do {
        t0 = vld1_u8(s);  // a8 a9 a10 a11 a12 a13 a14 a15
        int16x8_t s8 = vreinterpretq_s16_u16(vmovl_u8(t0));

        int16x8_t s1 = vextq_s16(s0, s8, 1);  // a1 a2 a3 a4 a5 a6 a7 a8
        int16x8_t s2 = vextq_s16(s0, s8, 2);  // a2 a3 a4 a5 a6 a7 a8 a9
        int16x8_t s3 = vextq_s16(s0, s8, 3);  // a3 a4 a5 a6 a7 a8 a9 a10
        int16x8_t s4 = vextq_s16(s0, s8, 4);  // a4 a5 a6 a7 a8 a9 a10 a11
        int16x8_t s5 = vextq_s16(s0, s8, 5);  // a5 a6 a7 a8 a9 a10 a11 a12
        int16x8_t s6 = vextq_s16(s0, s8, 6);  // a6 a7 a8 a9 a10 a11 a12 a13
        int16x8_t s7 = vextq_s16(s0, s8, 7);  // a7 a8 a9 a10 a11 a12 a13 a14

        int16x8_t d0 = convolve8_8_2d_h(s0, s1, s2, s3, s4, s5, s6, s7,
                                        x_filter, horiz_const);
        vst1q_s16(d, d0);

        s0 = s8;
        s += 8;
        d += 8;
        width -= 8;
      } while (width > 0);
      src_ptr += src_stride;
      dst_ptr += dst_stride;
      height--;
#endif  // AOM_ARCH_AARCH64
    } while (height > 0);
  }
}

#endif  // AOM_ARCH_AARCH64 && defined(__ARM_FEATURE_DOTPROD)

static INLINE uint16x4_t
convolve6_4_2d_v(const int16x4_t s0, const int16x4_t s1, const int16x4_t s2,
                 const int16x4_t s3, const int16x4_t s4, const int16x4_t s5,
                 const int16x8_t y_filter, const int32x4_t offset_const) {
  const int16x4_t y_filter_0_3 = vget_low_s16(y_filter);
  const int16x4_t y_filter_4_7 = vget_high_s16(y_filter);

  int32x4_t sum = offset_const;
  // Filter values at indices 0 and 7 are 0.
  sum = vmlal_lane_s16(sum, s0, y_filter_0_3, 1);
  sum = vmlal_lane_s16(sum, s1, y_filter_0_3, 2);
  sum = vmlal_lane_s16(sum, s2, y_filter_0_3, 3);
  sum = vmlal_lane_s16(sum, s3, y_filter_4_7, 0);
  sum = vmlal_lane_s16(sum, s4, y_filter_4_7, 1);
  sum = vmlal_lane_s16(sum, s5, y_filter_4_7, 2);

  return vqrshrun_n_s32(sum, COMPOUND_ROUND1_BITS);
}

static INLINE uint16x8_t
convolve6_8_2d_v(const int16x8_t s0, const int16x8_t s1, const int16x8_t s2,
                 const int16x8_t s3, const int16x8_t s4, const int16x8_t s5,
                 const int16x8_t y_filter, const int32x4_t offset_const) {
  const int16x4_t y_filter_0_3 = vget_low_s16(y_filter);
  const int16x4_t y_filter_4_7 = vget_high_s16(y_filter);

  int32x4_t sum0 = offset_const;
  // Filter values at indices 0 and 7 are 0.
  sum0 = vmlal_lane_s16(sum0, vget_low_s16(s0), y_filter_0_3, 1);
  sum0 = vmlal_lane_s16(sum0, vget_low_s16(s1), y_filter_0_3, 2);
  sum0 = vmlal_lane_s16(sum0, vget_low_s16(s2), y_filter_0_3, 3);
  sum0 = vmlal_lane_s16(sum0, vget_low_s16(s3), y_filter_4_7, 0);
  sum0 = vmlal_lane_s16(sum0, vget_low_s16(s4), y_filter_4_7, 1);
  sum0 = vmlal_lane_s16(sum0, vget_low_s16(s5), y_filter_4_7, 2);

  int32x4_t sum1 = offset_const;
  sum1 = vmlal_lane_s16(sum1, vget_high_s16(s0), y_filter_0_3, 1);
  sum1 = vmlal_lane_s16(sum1, vget_high_s16(s1), y_filter_0_3, 2);
  sum1 = vmlal_lane_s16(sum1, vget_high_s16(s2), y_filter_0_3, 3);
  sum1 = vmlal_lane_s16(sum1, vget_high_s16(s3), y_filter_4_7, 0);
  sum1 = vmlal_lane_s16(sum1, vget_high_s16(s4), y_filter_4_7, 1);
  sum1 = vmlal_lane_s16(sum1, vget_high_s16(s5), y_filter_4_7, 2);

  return vcombine_u16(vqrshrun_n_s32(sum0, COMPOUND_ROUND1_BITS),
                      vqrshrun_n_s32(sum1, COMPOUND_ROUND1_BITS));
}

static INLINE void dist_wtd_convolve_2d_vert_6tap_dist_wtd_avg_neon(
    int16_t *src_ptr, const int src_stride, uint8_t *dst8_ptr, int dst8_stride,
    ConvolveParams *conv_params, const int16x8_t y_filter, int h, int w) {
  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int32x4_t offset_const = vdupq_n_s32(1 << offset_bits);
  const int16_t round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                               (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const int16x8_t round_offset_vec = vdupq_n_s16(round_offset);

  const uint16_t fwd_offset = conv_params->fwd_offset;
  const uint16_t bck_offset = conv_params->bck_offset;

  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  const int dst_stride = conv_params->dst_stride;

  if (w == 4) {
    int16x4_t s0, s1, s2, s3, s4;
    load_s16_4x5(src_ptr, src_stride, &s0, &s1, &s2, &s3, &s4);
    src_ptr += 5 * src_stride;

    do {
#if AOM_ARCH_AARCH64
      int16x4_t s5, s6, s7, s8;
      load_s16_4x4(src_ptr, src_stride, &s5, &s6, &s7, &s8);

      uint16x4_t d0 =
          convolve6_4_2d_v(s0, s1, s2, s3, s4, s5, y_filter, offset_const);
      uint16x4_t d1 =
          convolve6_4_2d_v(s1, s2, s3, s4, s5, s6, y_filter, offset_const);
      uint16x4_t d2 =
          convolve6_4_2d_v(s2, s3, s4, s5, s6, s7, y_filter, offset_const);
      uint16x4_t d3 =
          convolve6_4_2d_v(s3, s4, s5, s6, s7, s8, y_filter, offset_const);

      uint16x4_t dd0, dd1, dd2, dd3;
      load_u16_4x4(dst_ptr, dst_stride, &dd0, &dd1, &dd2, &dd3);

      uint8x8_t d01_u8, d23_u8;
      compute_dist_wtd_avg_4x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3, fwd_offset,
                               bck_offset, round_offset_vec, &d01_u8, &d23_u8);

      store_u8_4x1(dst8_ptr + 0 * dst8_stride, d01_u8, 0);
      store_u8_4x1(dst8_ptr + 1 * dst8_stride, d01_u8, 1);
      store_u8_4x1(dst8_ptr + 2 * dst8_stride, d23_u8, 0);
      store_u8_4x1(dst8_ptr + 3 * dst8_stride, d23_u8, 1);
      dst8_ptr += 4 * dst8_stride;

      s0 = s4;
      s1 = s5;
      s2 = s6;
      s3 = s7;
      s4 = s8;
      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      h -= 4;
#else   // !AOM_ARCH_AARCH64
      int16x4_t s5 = vld1_s16(src_ptr);

      uint16x4_t d0 =
          convolve6_4_2d_v(s0, s1, s2, s3, s4, s5, y_filter, offset_const);

      uint16x4_t dd0 = vld1_u16(dst_ptr);

      uint8x8_t d01_u8;
      compute_dist_wtd_avg_4x1(dd0, d0, fwd_offset, bck_offset,
                               vget_low_s16(round_offset_vec), &d01_u8);

      store_u8_4x1(dst8_ptr, d01_u8, 0);
      dst8_ptr += dst8_stride;

      s0 = s1;
      s1 = s2;
      s2 = s3;
      s3 = s4;
      s4 = s5;
      src_ptr += src_stride;
      dst_ptr += dst_stride;
      h--;
#endif  // AOM_ARCH_AARCH64
    } while (h != 0);
  } else {
    do {
      int16_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      uint8_t *d_u8 = dst8_ptr;
      int height = h;

      int16x8_t s0, s1, s2, s3, s4;
      load_s16_8x5(s, src_stride, &s0, &s1, &s2, &s3, &s4);
      s += 5 * src_stride;

      do {
#if AOM_ARCH_AARCH64
        int16x8_t s5, s6, s7, s8;
        load_s16_8x4(s, src_stride, &s5, &s6, &s7, &s8);

        uint16x8_t d0 =
            convolve6_8_2d_v(s0, s1, s2, s3, s4, s5, y_filter, offset_const);
        uint16x8_t d1 =
            convolve6_8_2d_v(s1, s2, s3, s4, s5, s6, y_filter, offset_const);
        uint16x8_t d2 =
            convolve6_8_2d_v(s2, s3, s4, s5, s6, s7, y_filter, offset_const);
        uint16x8_t d3 =
            convolve6_8_2d_v(s3, s4, s5, s6, s7, s8, y_filter, offset_const);

        uint16x8_t dd0, dd1, dd2, dd3;
        load_u16_8x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

        uint8x8_t d0_u8, d1_u8, d2_u8, d3_u8;
        compute_dist_wtd_avg_8x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3, fwd_offset,
                                 bck_offset, round_offset_vec, &d0_u8, &d1_u8,
                                 &d2_u8, &d3_u8);

        store_u8_8x4(d_u8, dst8_stride, d0_u8, d1_u8, d2_u8, d3_u8);
        d_u8 += 4 * dst8_stride;

        s0 = s4;
        s1 = s5;
        s2 = s6;
        s3 = s7;
        s4 = s8;
        s += 4 * src_stride;
        d += 4 * dst_stride;
        height -= 4;
#else   // !AOM_ARCH_AARCH64
        int16x8_t s5 = vld1q_s16(s);

        uint16x8_t d0 =
            convolve6_8_2d_v(s0, s1, s2, s3, s4, s5, y_filter, offset_const);

        uint16x8_t dd0 = vld1q_u16(d);

        uint8x8_t d0_u8;
        compute_dist_wtd_avg_8x1(dd0, d0, fwd_offset, bck_offset,
                                 round_offset_vec, &d0_u8);

        vst1_u8(d_u8, d0_u8);
        d_u8 += dst8_stride;

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;
        s += src_stride;
        d += dst_stride;
        height--;
#endif  // AOM_ARCH_AARCH64
      } while (height != 0);
      src_ptr += 8;
      dst_ptr += 8;
      dst8_ptr += 8;
      w -= 8;
    } while (w != 0);
  }
}

static INLINE void dist_wtd_convolve_2d_vert_6tap_avg_neon(
    int16_t *src_ptr, const int src_stride, uint8_t *dst8_ptr, int dst8_stride,
    ConvolveParams *conv_params, const int16x8_t y_filter, int h, int w) {
  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int32x4_t offset_const = vdupq_n_s32(1 << offset_bits);
  const int16_t round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                               (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const int16x8_t round_offset_vec = vdupq_n_s16(round_offset);

  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  const int dst_stride = conv_params->dst_stride;

  if (w == 4) {
    int16x4_t s0, s1, s2, s3, s4;
    load_s16_4x5(src_ptr, src_stride, &s0, &s1, &s2, &s3, &s4);
    src_ptr += 5 * src_stride;

    do {
#if AOM_ARCH_AARCH64
      int16x4_t s5, s6, s7, s8;
      load_s16_4x4(src_ptr, src_stride, &s5, &s6, &s7, &s8);

      uint16x4_t d0 =
          convolve6_4_2d_v(s0, s1, s2, s3, s4, s5, y_filter, offset_const);
      uint16x4_t d1 =
          convolve6_4_2d_v(s1, s2, s3, s4, s5, s6, y_filter, offset_const);
      uint16x4_t d2 =
          convolve6_4_2d_v(s2, s3, s4, s5, s6, s7, y_filter, offset_const);
      uint16x4_t d3 =
          convolve6_4_2d_v(s3, s4, s5, s6, s7, s8, y_filter, offset_const);

      uint16x4_t dd0, dd1, dd2, dd3;
      load_u16_4x4(dst_ptr, dst_stride, &dd0, &dd1, &dd2, &dd3);

      uint8x8_t d01_u8, d23_u8;
      compute_basic_avg_4x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3,
                            round_offset_vec, &d01_u8, &d23_u8);

      store_u8_4x1(dst8_ptr + 0 * dst8_stride, d01_u8, 0);
      store_u8_4x1(dst8_ptr + 1 * dst8_stride, d01_u8, 1);
      store_u8_4x1(dst8_ptr + 2 * dst8_stride, d23_u8, 0);
      store_u8_4x1(dst8_ptr + 3 * dst8_stride, d23_u8, 1);
      dst8_ptr += 4 * dst8_stride;

      s0 = s4;
      s1 = s5;
      s2 = s6;
      s3 = s7;
      s4 = s8;
      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      h -= 4;
#else   // !AOM_ARCH_AARCH64
      int16x4_t s5 = vld1_s16(src_ptr);

      uint16x4_t d0 =
          convolve6_4_2d_v(s0, s1, s2, s3, s4, s5, y_filter, offset_const);

      uint16x4_t dd0 = vld1_u16(dst_ptr);

      uint8x8_t d01_u8;
      compute_basic_avg_4x1(dd0, d0, vget_low_s16(round_offset_vec), &d01_u8);

      store_u8_4x1(dst8_ptr, d01_u8, 0);
      dst8_ptr += dst8_stride;

      s0 = s1;
      s1 = s2;
      s2 = s3;
      s3 = s4;
      s4 = s5;
      src_ptr += src_stride;
      dst_ptr += dst_stride;
      h--;
#endif  // AOM_ARCH_AARCH64
    } while (h != 0);
  } else {
    do {
      int16_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      uint8_t *d_u8 = dst8_ptr;
      int height = h;

      int16x8_t s0, s1, s2, s3, s4;
      load_s16_8x5(s, src_stride, &s0, &s1, &s2, &s3, &s4);
      s += 5 * src_stride;

      do {
#if AOM_ARCH_AARCH64
        int16x8_t s5, s6, s7, s8;
        load_s16_8x4(s, src_stride, &s5, &s6, &s7, &s8);

        uint16x8_t d0 =
            convolve6_8_2d_v(s0, s1, s2, s3, s4, s5, y_filter, offset_const);
        uint16x8_t d1 =
            convolve6_8_2d_v(s1, s2, s3, s4, s5, s6, y_filter, offset_const);
        uint16x8_t d2 =
            convolve6_8_2d_v(s2, s3, s4, s5, s6, s7, y_filter, offset_const);
        uint16x8_t d3 =
            convolve6_8_2d_v(s3, s4, s5, s6, s7, s8, y_filter, offset_const);

        uint16x8_t dd0, dd1, dd2, dd3;
        load_u16_8x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

        uint8x8_t d0_u8, d1_u8, d2_u8, d3_u8;
        compute_basic_avg_8x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3,
                              round_offset_vec, &d0_u8, &d1_u8, &d2_u8, &d3_u8);

        store_u8_8x4(d_u8, dst8_stride, d0_u8, d1_u8, d2_u8, d3_u8);
        d_u8 += 4 * dst8_stride;

        s0 = s4;
        s1 = s5;
        s2 = s6;
        s3 = s7;
        s4 = s8;
        s += 4 * src_stride;
        d += 4 * dst_stride;
        height -= 4;
#else   // !AOM_ARCH_AARCH64
        int16x8_t s5 = vld1q_s16(s);

        uint16x8_t d0 =
            convolve6_8_2d_v(s0, s1, s2, s3, s4, s5, y_filter, offset_const);

        uint16x8_t dd0 = vld1q_u16(d);

        uint8x8_t d0_u8;
        compute_basic_avg_8x1(dd0, d0, round_offset_vec, &d0_u8);

        vst1_u8(d_u8, d0_u8);
        d_u8 += dst8_stride;

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;
        s += src_stride;
        d += dst_stride;
        height--;
#endif  // AOM_ARCH_AARCH64
      } while (height != 0);
      src_ptr += 8;
      dst_ptr += 8;
      dst8_ptr += 8;
      w -= 8;
    } while (w != 0);
  }
}

static INLINE void dist_wtd_convolve_2d_vert_6tap_neon(
    int16_t *src_ptr, const int src_stride, ConvolveParams *conv_params,
    const int16x8_t y_filter, int h, int w) {
  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int32x4_t offset_const = vdupq_n_s32(1 << offset_bits);

  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  const int dst_stride = conv_params->dst_stride;

  if (w == 4) {
    int16x4_t s0, s1, s2, s3, s4;
    load_s16_4x5(src_ptr, src_stride, &s0, &s1, &s2, &s3, &s4);
    src_ptr += 5 * src_stride;

    do {
#if AOM_ARCH_AARCH64
      int16x4_t s5, s6, s7, s8;
      load_s16_4x4(src_ptr, src_stride, &s5, &s6, &s7, &s8);

      uint16x4_t d0 =
          convolve6_4_2d_v(s0, s1, s2, s3, s4, s5, y_filter, offset_const);
      uint16x4_t d1 =
          convolve6_4_2d_v(s1, s2, s3, s4, s5, s6, y_filter, offset_const);
      uint16x4_t d2 =
          convolve6_4_2d_v(s2, s3, s4, s5, s6, s7, y_filter, offset_const);
      uint16x4_t d3 =
          convolve6_4_2d_v(s3, s4, s5, s6, s7, s8, y_filter, offset_const);

      store_u16_4x4(dst_ptr, dst_stride, d0, d1, d2, d3);

      s0 = s4;
      s1 = s5;
      s2 = s6;
      s3 = s7;
      s4 = s8;
      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      h -= 4;
#else   // !AOM_ARCH_AARCH64
      int16x4_t s5 = vld1_s16(src_ptr);

      uint16x4_t d0 =
          convolve6_4_2d_v(s0, s1, s2, s3, s4, s5, y_filter, offset_const);

      vst1_u16(dst_ptr, d0);

      s0 = s1;
      s1 = s2;
      s2 = s3;
      s3 = s4;
      s4 = s5;
      src_ptr += src_stride;
      dst_ptr += dst_stride;
      h--;
#endif  // AOM_ARCH_AARCH64
    } while (h != 0);
  } else {
    do {
      int16_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      int height = h;

      int16x8_t s0, s1, s2, s3, s4;
      load_s16_8x5(s, src_stride, &s0, &s1, &s2, &s3, &s4);
      s += 5 * src_stride;

      do {
#if AOM_ARCH_AARCH64
        int16x8_t s5, s6, s7, s8;
        load_s16_8x4(s, src_stride, &s5, &s6, &s7, &s8);

        uint16x8_t d0 =
            convolve6_8_2d_v(s0, s1, s2, s3, s4, s5, y_filter, offset_const);
        uint16x8_t d1 =
            convolve6_8_2d_v(s1, s2, s3, s4, s5, s6, y_filter, offset_const);
        uint16x8_t d2 =
            convolve6_8_2d_v(s2, s3, s4, s5, s6, s7, y_filter, offset_const);
        uint16x8_t d3 =
            convolve6_8_2d_v(s3, s4, s5, s6, s7, s8, y_filter, offset_const);

        store_u16_8x4(d, dst_stride, d0, d1, d2, d3);

        s0 = s4;
        s1 = s5;
        s2 = s6;
        s3 = s7;
        s4 = s8;
        s += 4 * src_stride;
        d += 4 * dst_stride;
        height -= 4;
#else   // !AOM_ARCH_AARCH64
        int16x8_t s5 = vld1q_s16(s);

        uint16x8_t d0 =
            convolve6_8_2d_v(s0, s1, s2, s3, s4, s5, y_filter, offset_const);

        vst1q_u16(d, d0);

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;
        s += src_stride;
        d += dst_stride;
        height--;
#endif  // AOM_ARCH_AARCH64
      } while (height != 0);
      src_ptr += 8;
      dst_ptr += 8;
      w -= 8;
    } while (w != 0);
  }
}

static INLINE uint16x4_t
convolve8_4_2d_v(const int16x4_t s0, const int16x4_t s1, const int16x4_t s2,
                 const int16x4_t s3, const int16x4_t s4, const int16x4_t s5,
                 const int16x4_t s6, const int16x4_t s7,
                 const int16x8_t y_filter, const int32x4_t offset_const) {
  const int16x4_t y_filter_0_3 = vget_low_s16(y_filter);
  const int16x4_t y_filter_4_7 = vget_high_s16(y_filter);

  int32x4_t sum = offset_const;
  sum = vmlal_lane_s16(sum, s0, y_filter_0_3, 0);
  sum = vmlal_lane_s16(sum, s1, y_filter_0_3, 1);
  sum = vmlal_lane_s16(sum, s2, y_filter_0_3, 2);
  sum = vmlal_lane_s16(sum, s3, y_filter_0_3, 3);
  sum = vmlal_lane_s16(sum, s4, y_filter_4_7, 0);
  sum = vmlal_lane_s16(sum, s5, y_filter_4_7, 1);
  sum = vmlal_lane_s16(sum, s6, y_filter_4_7, 2);
  sum = vmlal_lane_s16(sum, s7, y_filter_4_7, 3);

  return vqrshrun_n_s32(sum, COMPOUND_ROUND1_BITS);
}

static INLINE uint16x8_t
convolve8_8_2d_v(const int16x8_t s0, const int16x8_t s1, const int16x8_t s2,
                 const int16x8_t s3, const int16x8_t s4, const int16x8_t s5,
                 const int16x8_t s6, const int16x8_t s7,
                 const int16x8_t y_filter, const int32x4_t offset_const) {
  const int16x4_t y_filter_0_3 = vget_low_s16(y_filter);
  const int16x4_t y_filter_4_7 = vget_high_s16(y_filter);

  int32x4_t sum0 = offset_const;
  sum0 = vmlal_lane_s16(sum0, vget_low_s16(s0), y_filter_0_3, 0);
  sum0 = vmlal_lane_s16(sum0, vget_low_s16(s1), y_filter_0_3, 1);
  sum0 = vmlal_lane_s16(sum0, vget_low_s16(s2), y_filter_0_3, 2);
  sum0 = vmlal_lane_s16(sum0, vget_low_s16(s3), y_filter_0_3, 3);
  sum0 = vmlal_lane_s16(sum0, vget_low_s16(s4), y_filter_4_7, 0);
  sum0 = vmlal_lane_s16(sum0, vget_low_s16(s5), y_filter_4_7, 1);
  sum0 = vmlal_lane_s16(sum0, vget_low_s16(s6), y_filter_4_7, 2);
  sum0 = vmlal_lane_s16(sum0, vget_low_s16(s7), y_filter_4_7, 3);

  int32x4_t sum1 = offset_const;
  sum1 = vmlal_lane_s16(sum1, vget_high_s16(s0), y_filter_0_3, 0);
  sum1 = vmlal_lane_s16(sum1, vget_high_s16(s1), y_filter_0_3, 1);
  sum1 = vmlal_lane_s16(sum1, vget_high_s16(s2), y_filter_0_3, 2);
  sum1 = vmlal_lane_s16(sum1, vget_high_s16(s3), y_filter_0_3, 3);
  sum1 = vmlal_lane_s16(sum1, vget_high_s16(s4), y_filter_4_7, 0);
  sum1 = vmlal_lane_s16(sum1, vget_high_s16(s5), y_filter_4_7, 1);
  sum1 = vmlal_lane_s16(sum1, vget_high_s16(s6), y_filter_4_7, 2);
  sum1 = vmlal_lane_s16(sum1, vget_high_s16(s7), y_filter_4_7, 3);

  return vcombine_u16(vqrshrun_n_s32(sum0, COMPOUND_ROUND1_BITS),
                      vqrshrun_n_s32(sum1, COMPOUND_ROUND1_BITS));
}

static INLINE void dist_wtd_convolve_2d_vert_8tap_dist_wtd_avg_neon(
    int16_t *src_ptr, const int src_stride, uint8_t *dst8_ptr, int dst8_stride,
    ConvolveParams *conv_params, const int16x8_t y_filter, int h, int w) {
  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int32x4_t offset_const = vdupq_n_s32(1 << offset_bits);
  const int16_t round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                               (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const int16x8_t round_offset_vec = vdupq_n_s16(round_offset);

  const uint16_t fwd_offset = conv_params->fwd_offset;
  const uint16_t bck_offset = conv_params->bck_offset;

  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  const int dst_stride = conv_params->dst_stride;

  if (w == 4) {
    int16x4_t s0, s1, s2, s3, s4, s5, s6;
    load_s16_4x7(src_ptr, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6);
    src_ptr += 7 * src_stride;

    do {
#if AOM_ARCH_AARCH64
      int16x4_t s7, s8, s9, s10;
      load_s16_4x4(src_ptr, src_stride, &s7, &s8, &s9, &s10);

      uint16x4_t d0 = convolve8_4_2d_v(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                       offset_const);
      uint16x4_t d1 = convolve8_4_2d_v(s1, s2, s3, s4, s5, s6, s7, s8, y_filter,
                                       offset_const);
      uint16x4_t d2 = convolve8_4_2d_v(s2, s3, s4, s5, s6, s7, s8, s9, y_filter,
                                       offset_const);
      uint16x4_t d3 = convolve8_4_2d_v(s3, s4, s5, s6, s7, s8, s9, s10,
                                       y_filter, offset_const);

      uint16x4_t dd0, dd1, dd2, dd3;
      load_u16_4x4(dst_ptr, dst_stride, &dd0, &dd1, &dd2, &dd3);

      uint8x8_t d01_u8, d23_u8;
      compute_dist_wtd_avg_4x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3, fwd_offset,
                               bck_offset, round_offset_vec, &d01_u8, &d23_u8);

      store_u8_4x1(dst8_ptr + 0 * dst8_stride, d01_u8, 0);
      store_u8_4x1(dst8_ptr + 1 * dst8_stride, d01_u8, 1);
      store_u8_4x1(dst8_ptr + 2 * dst8_stride, d23_u8, 0);
      store_u8_4x1(dst8_ptr + 3 * dst8_stride, d23_u8, 1);
      dst8_ptr += 4 * dst8_stride;

      s0 = s4;
      s1 = s5;
      s2 = s6;
      s3 = s7;
      s4 = s8;
      s5 = s9;
      s6 = s10;
      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      h -= 4;
#else   // !AOM_ARCH_AARCH64
      int16x4_t s7 = vld1_s16(src_ptr);

      uint16x4_t d0 = convolve8_4_2d_v(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                       offset_const);

      uint16x4_t dd0 = vld1_u16(dst_ptr);

      uint8x8_t d01_u8;
      compute_dist_wtd_avg_4x1(dd0, d0, fwd_offset, bck_offset,
                               vget_low_s16(round_offset_vec), &d01_u8);

      store_u8_4x1(dst8_ptr, d01_u8, 0);
      dst8_ptr += dst8_stride;

      s0 = s1;
      s1 = s2;
      s2 = s3;
      s3 = s4;
      s4 = s5;
      s5 = s6;
      s6 = s7;
      src_ptr += src_stride;
      dst_ptr += dst_stride;
      h--;
#endif  // AOM_ARCH_AARCH64
    } while (h != 0);
  } else {
    do {
      int16_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      uint8_t *d_u8 = dst8_ptr;
      int height = h;

      int16x8_t s0, s1, s2, s3, s4, s5, s6;
      load_s16_8x7(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6);
      s += 7 * src_stride;

      do {
#if AOM_ARCH_AARCH64
        int16x8_t s7, s8, s9, s10;
        load_s16_8x4(s, src_stride, &s7, &s8, &s9, &s10);

        uint16x8_t d0 = convolve8_8_2d_v(s0, s1, s2, s3, s4, s5, s6, s7,
                                         y_filter, offset_const);
        uint16x8_t d1 = convolve8_8_2d_v(s1, s2, s3, s4, s5, s6, s7, s8,
                                         y_filter, offset_const);
        uint16x8_t d2 = convolve8_8_2d_v(s2, s3, s4, s5, s6, s7, s8, s9,
                                         y_filter, offset_const);
        uint16x8_t d3 = convolve8_8_2d_v(s3, s4, s5, s6, s7, s8, s9, s10,
                                         y_filter, offset_const);

        uint16x8_t dd0, dd1, dd2, dd3;
        load_u16_8x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

        uint8x8_t d0_u8, d1_u8, d2_u8, d3_u8;
        compute_dist_wtd_avg_8x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3, fwd_offset,
                                 bck_offset, round_offset_vec, &d0_u8, &d1_u8,
                                 &d2_u8, &d3_u8);

        store_u8_8x4(d_u8, dst8_stride, d0_u8, d1_u8, d2_u8, d3_u8);
        d_u8 += 4 * dst8_stride;

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
#else   // !AOM_ARCH_AARCH64
        int16x8_t s7 = vld1q_s16(s);

        uint16x8_t d0 = convolve8_8_2d_v(s0, s1, s2, s3, s4, s5, s6, s7,
                                         y_filter, offset_const);

        uint16x8_t dd0 = vld1q_u16(d);

        uint8x8_t d0_u8;
        compute_dist_wtd_avg_8x1(dd0, d0, fwd_offset, bck_offset,
                                 round_offset_vec, &d0_u8);

        vst1_u8(d_u8, d0_u8);
        d_u8 += dst8_stride;

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;
        s5 = s6;
        s6 = s7;
        s += src_stride;
        d += dst_stride;
        height--;
#endif  // AOM_ARCH_AARCH64
      } while (height != 0);
      src_ptr += 8;
      dst_ptr += 8;
      dst8_ptr += 8;
      w -= 8;
    } while (w != 0);
  }
}

static INLINE void dist_wtd_convolve_2d_vert_8tap_avg_neon(
    int16_t *src_ptr, const int src_stride, uint8_t *dst8_ptr, int dst8_stride,
    ConvolveParams *conv_params, const int16x8_t y_filter, int h, int w) {
  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int32x4_t offset_const = vdupq_n_s32(1 << offset_bits);
  const int16_t round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                               (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const int16x8_t round_offset_vec = vdupq_n_s16(round_offset);

  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  const int dst_stride = conv_params->dst_stride;

  if (w == 4) {
    int16x4_t s0, s1, s2, s3, s4, s5, s6;
    load_s16_4x7(src_ptr, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6);
    src_ptr += 7 * src_stride;

    do {
#if AOM_ARCH_AARCH64
      int16x4_t s7, s8, s9, s10;
      load_s16_4x4(src_ptr, src_stride, &s7, &s8, &s9, &s10);

      uint16x4_t d0 = convolve8_4_2d_v(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                       offset_const);
      uint16x4_t d1 = convolve8_4_2d_v(s1, s2, s3, s4, s5, s6, s7, s8, y_filter,
                                       offset_const);
      uint16x4_t d2 = convolve8_4_2d_v(s2, s3, s4, s5, s6, s7, s8, s9, y_filter,
                                       offset_const);
      uint16x4_t d3 = convolve8_4_2d_v(s3, s4, s5, s6, s7, s8, s9, s10,
                                       y_filter, offset_const);

      uint16x4_t dd0, dd1, dd2, dd3;
      load_u16_4x4(dst_ptr, dst_stride, &dd0, &dd1, &dd2, &dd3);

      uint8x8_t d01_u8, d23_u8;
      compute_basic_avg_4x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3,
                            round_offset_vec, &d01_u8, &d23_u8);

      store_u8_4x1(dst8_ptr + 0 * dst8_stride, d01_u8, 0);
      store_u8_4x1(dst8_ptr + 1 * dst8_stride, d01_u8, 1);
      store_u8_4x1(dst8_ptr + 2 * dst8_stride, d23_u8, 0);
      store_u8_4x1(dst8_ptr + 3 * dst8_stride, d23_u8, 1);
      dst8_ptr += 4 * dst8_stride;

      s0 = s4;
      s1 = s5;
      s2 = s6;
      s3 = s7;
      s4 = s8;
      s5 = s9;
      s6 = s10;
      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      h -= 4;
#else   // !AOM_ARCH_AARCH64
      int16x4_t s7 = vld1_s16(src_ptr);

      uint16x4_t d0 = convolve8_4_2d_v(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                       offset_const);

      uint16x4_t dd0 = vld1_u16(dst_ptr);

      uint8x8_t d01_u8;
      compute_basic_avg_4x1(dd0, d0, vget_low_s16(round_offset_vec), &d01_u8);

      store_u8_4x1(dst8_ptr, d01_u8, 0);
      dst8_ptr += dst8_stride;

      s0 = s1;
      s1 = s2;
      s2 = s3;
      s3 = s4;
      s4 = s5;
      s5 = s6;
      s6 = s7;
      src_ptr += src_stride;
      dst_ptr += dst_stride;
      h--;
#endif  // AOM_ARCH_AARCH64
    } while (h != 0);
  } else {
    do {
      int16_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      uint8_t *d_u8 = dst8_ptr;
      int height = h;

      int16x8_t s0, s1, s2, s3, s4, s5, s6;
      load_s16_8x7(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6);
      s += 7 * src_stride;

      do {
#if AOM_ARCH_AARCH64
        int16x8_t s7, s8, s9, s10;
        load_s16_8x4(s, src_stride, &s7, &s8, &s9, &s10);

        uint16x8_t d0 = convolve8_8_2d_v(s0, s1, s2, s3, s4, s5, s6, s7,
                                         y_filter, offset_const);
        uint16x8_t d1 = convolve8_8_2d_v(s1, s2, s3, s4, s5, s6, s7, s8,
                                         y_filter, offset_const);
        uint16x8_t d2 = convolve8_8_2d_v(s2, s3, s4, s5, s6, s7, s8, s9,
                                         y_filter, offset_const);
        uint16x8_t d3 = convolve8_8_2d_v(s3, s4, s5, s6, s7, s8, s9, s10,
                                         y_filter, offset_const);

        uint16x8_t dd0, dd1, dd2, dd3;
        load_u16_8x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

        uint8x8_t d0_u8, d1_u8, d2_u8, d3_u8;
        compute_basic_avg_8x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3,
                              round_offset_vec, &d0_u8, &d1_u8, &d2_u8, &d3_u8);

        store_u8_8x4(d_u8, dst8_stride, d0_u8, d1_u8, d2_u8, d3_u8);
        d_u8 += 4 * dst8_stride;

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
#else   // !AOM_ARCH_AARCH64
        int16x8_t s7 = vld1q_s16(s);

        uint16x8_t d0 = convolve8_8_2d_v(s0, s1, s2, s3, s4, s5, s6, s7,
                                         y_filter, offset_const);

        uint16x8_t dd0 = vld1q_u16(d);

        uint8x8_t d0_u8;
        compute_basic_avg_8x1(dd0, d0, round_offset_vec, &d0_u8);

        vst1_u8(d_u8, d0_u8);
        d_u8 += dst8_stride;

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;
        s5 = s6;
        s6 = s7;
        s += src_stride;
        d += dst_stride;
        height--;
#endif  // AOM_ARCH_AARCH64
      } while (height != 0);
      src_ptr += 8;
      dst_ptr += 8;
      dst8_ptr += 8;
      w -= 8;
    } while (w != 0);
  }
}

static INLINE void dist_wtd_convolve_2d_vert_8tap_neon(
    int16_t *src_ptr, const int src_stride, ConvolveParams *conv_params,
    const int16x8_t y_filter, int h, int w) {
  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int32x4_t offset_const = vdupq_n_s32(1 << offset_bits);

  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  const int dst_stride = conv_params->dst_stride;

  if (w == 4) {
    int16x4_t s0, s1, s2, s3, s4, s5, s6;
    load_s16_4x7(src_ptr, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6);
    src_ptr += 7 * src_stride;

    do {
#if AOM_ARCH_AARCH64
      int16x4_t s7, s8, s9, s10;
      load_s16_4x4(src_ptr, src_stride, &s7, &s8, &s9, &s10);

      uint16x4_t d0 = convolve8_4_2d_v(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                       offset_const);
      uint16x4_t d1 = convolve8_4_2d_v(s1, s2, s3, s4, s5, s6, s7, s8, y_filter,
                                       offset_const);
      uint16x4_t d2 = convolve8_4_2d_v(s2, s3, s4, s5, s6, s7, s8, s9, y_filter,
                                       offset_const);
      uint16x4_t d3 = convolve8_4_2d_v(s3, s4, s5, s6, s7, s8, s9, s10,
                                       y_filter, offset_const);

      store_u16_4x4(dst_ptr, dst_stride, d0, d1, d2, d3);

      s0 = s4;
      s1 = s5;
      s2 = s6;
      s3 = s7;
      s4 = s8;
      s5 = s9;
      s6 = s10;
      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      h -= 4;
#else   // !AOM_ARCH_AARCH64
      int16x4_t s7 = vld1_s16(src_ptr);

      uint16x4_t d0 = convolve8_4_2d_v(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                       offset_const);

      vst1_u16(dst_ptr, d0);

      s0 = s1;
      s1 = s2;
      s2 = s3;
      s3 = s4;
      s4 = s5;
      s5 = s6;
      s6 = s7;
      src_ptr += src_stride;
      dst_ptr += dst_stride;
      h--;
#endif  // AOM_ARCH_AARCH64
    } while (h != 0);
  } else {
    do {
      int16_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      int height = h;

      int16x8_t s0, s1, s2, s3, s4, s5, s6;
      load_s16_8x7(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6);
      s += 7 * src_stride;

      do {
#if AOM_ARCH_AARCH64
        int16x8_t s7, s8, s9, s10;
        load_s16_8x4(s, src_stride, &s7, &s8, &s9, &s10);

        uint16x8_t d0 = convolve8_8_2d_v(s0, s1, s2, s3, s4, s5, s6, s7,
                                         y_filter, offset_const);
        uint16x8_t d1 = convolve8_8_2d_v(s1, s2, s3, s4, s5, s6, s7, s8,
                                         y_filter, offset_const);
        uint16x8_t d2 = convolve8_8_2d_v(s2, s3, s4, s5, s6, s7, s8, s9,
                                         y_filter, offset_const);
        uint16x8_t d3 = convolve8_8_2d_v(s3, s4, s5, s6, s7, s8, s9, s10,
                                         y_filter, offset_const);

        store_u16_8x4(d, dst_stride, d0, d1, d2, d3);

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
#else   // !AOM_ARCH_AARCH64
        int16x8_t s7 = vld1q_s16(s);

        uint16x8_t d0 = convolve8_8_2d_v(s0, s1, s2, s3, s4, s5, s6, s7,
                                         y_filter, offset_const);

        vst1q_u16(d, d0);

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;
        s5 = s6;
        s6 = s7;
        s += src_stride;
        d += dst_stride;
        height--;
#endif  // AOM_ARCH_AARCH64
      } while (height != 0);
      src_ptr += 8;
      dst_ptr += 8;
      w -= 8;
    } while (w != 0);
  }
}

void av1_dist_wtd_convolve_2d_neon(const uint8_t *src, int src_stride,
                                   uint8_t *dst8, int dst8_stride, int w, int h,
                                   const InterpFilterParams *filter_params_x,
                                   const InterpFilterParams *filter_params_y,
                                   const int subpel_x_qn, const int subpel_y_qn,
                                   ConvolveParams *conv_params) {
  assert(w % 4 == 0);
  assert(h % 4 == 0);

  DECLARE_ALIGNED(16, int16_t,
                  im_block[(MAX_SB_SIZE + HORIZ_EXTRA_ROWS) * MAX_SB_SIZE]);

  const int y_filter_taps = get_filter_tap(filter_params_y, subpel_y_qn);
  const int clamped_y_taps = y_filter_taps < 6 ? 6 : y_filter_taps;

  const int im_h = h + filter_params_y->taps - 1;
  const int im_stride = MAX_SB_SIZE;
  const int vert_offset = filter_params_y->taps / 2 - 1;
  const int horiz_offset = filter_params_x->taps / 2 - 1;
  const uint8_t *src_ptr = src - vert_offset * src_stride - horiz_offset;
  const int16_t *x_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_x, subpel_x_qn & SUBPEL_MASK);
  const int16_t *y_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_y, subpel_y_qn & SUBPEL_MASK);

  // Filter values are even, so downshift by 1 to reduce intermediate precision
  // requirements.
  const int16x8_t x_filter = vshrq_n_s16(vld1q_s16(x_filter_ptr), 1);
  const int16x8_t y_filter = vld1q_s16(y_filter_ptr);

  dist_wtd_convolve_2d_horiz_8tap_neon(src_ptr, src_stride, im_block, im_stride,
                                       x_filter, im_h, w);

  if (clamped_y_taps == 6) {
    if (conv_params->do_average) {
      if (UNLIKELY(conv_params->use_dist_wtd_comp_avg)) {
        dist_wtd_convolve_2d_vert_6tap_dist_wtd_avg_neon(
            im_block + im_stride, im_stride, dst8, dst8_stride, conv_params,
            y_filter, h, w);
      } else {
        dist_wtd_convolve_2d_vert_6tap_avg_neon(im_block + im_stride, im_stride,
                                                dst8, dst8_stride, conv_params,
                                                y_filter, h, w);
      }
    } else {
      dist_wtd_convolve_2d_vert_6tap_neon(im_block + im_stride, im_stride,
                                          conv_params, y_filter, h, w);
    }
  } else {
    if (conv_params->do_average) {
      if (UNLIKELY(conv_params->use_dist_wtd_comp_avg)) {
        dist_wtd_convolve_2d_vert_8tap_dist_wtd_avg_neon(
            im_block, im_stride, dst8, dst8_stride, conv_params, y_filter, h,
            w);
      } else {
        dist_wtd_convolve_2d_vert_8tap_avg_neon(im_block, im_stride, dst8,
                                                dst8_stride, conv_params,
                                                y_filter, h, w);
      }
    } else {
      dist_wtd_convolve_2d_vert_8tap_neon(im_block, im_stride, conv_params,
                                          y_filter, h, w);
    }
  }
}

static INLINE void dist_wtd_convolve_2d_copy_dist_wtd_avg_neon(
    const uint8_t *src, int src_stride, uint8_t *dst8, int dst8_stride, int w,
    int h, ConvolveParams *conv_params) {
  assert(w % 4 == 0);
  assert(h % 4 == 0);

  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const uint16_t round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                                (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const uint16x8_t round_offset_vec = vdupq_n_u16(round_offset);
  const uint8x8_t shift_by_bits = vdup_n_u8(1 << (FILTER_BITS - ROUND0_BITS));

  const uint16_t fwd_offset = conv_params->fwd_offset;
  const uint16_t bck_offset = conv_params->bck_offset;

  CONV_BUF_TYPE *dst = conv_params->dst;
  const int dst_stride = conv_params->dst_stride;
  int height = h;

  if (w == 4) {
    do {
      uint8x8_t s0, s1, s2, s3;
      load_u8_8x4(src, src_stride, &s0, &s1, &s2, &s3);

      uint16x4_t d0 =
          vget_low_u16(vmlal_u8(round_offset_vec, s0, shift_by_bits));
      uint16x4_t d1 =
          vget_low_u16(vmlal_u8(round_offset_vec, s1, shift_by_bits));
      uint16x4_t d2 =
          vget_low_u16(vmlal_u8(round_offset_vec, s2, shift_by_bits));
      uint16x4_t d3 =
          vget_low_u16(vmlal_u8(round_offset_vec, s3, shift_by_bits));

      uint16x4_t dd0, dd1, dd2, dd3;
      load_u16_4x4(dst, dst_stride, &dd0, &dd1, &dd2, &dd3);

      uint8x8_t d01, d23;
      compute_dist_wtd_avg_4x4(
          dd0, dd1, dd2, dd3, d0, d1, d2, d3, fwd_offset, bck_offset,
          vreinterpretq_s16_u16(round_offset_vec), &d01, &d23);

      store_u8_4x1(dst8 + 0 * dst8_stride, d01, 0);
      store_u8_4x1(dst8 + 1 * dst8_stride, d01, 1);
      store_u8_4x1(dst8 + 2 * dst8_stride, d23, 0);
      store_u8_4x1(dst8 + 3 * dst8_stride, d23, 1);

      src += 4 * src_stride;
      dst += 4 * dst_stride;
      dst8 += 4 * dst8_stride;
      height -= 4;
    } while (height != 0);
  } else {
    do {
      const uint8_t *s = src;
      CONV_BUF_TYPE *d = dst;
      uint8_t *d_u8 = dst8;
      int width = w;

      do {
        uint8x8_t s0, s1, s2, s3;
        load_u8_8x4(s, src_stride, &s0, &s1, &s2, &s3);

        uint16x8_t d0 = vmlal_u8(round_offset_vec, s0, shift_by_bits);
        uint16x8_t d1 = vmlal_u8(round_offset_vec, s1, shift_by_bits);
        uint16x8_t d2 = vmlal_u8(round_offset_vec, s2, shift_by_bits);
        uint16x8_t d3 = vmlal_u8(round_offset_vec, s3, shift_by_bits);

        uint16x8_t dd0, dd1, dd2, dd3;
        load_u16_8x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

        uint8x8_t d0_u8, d1_u8, d2_u8, d3_u8;
        compute_dist_wtd_avg_8x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3, fwd_offset,
                                 bck_offset,
                                 vreinterpretq_s16_u16(round_offset_vec),
                                 &d0_u8, &d1_u8, &d2_u8, &d3_u8);

        store_u8_8x4(d_u8, dst8_stride, d0_u8, d1_u8, d2_u8, d3_u8);

        s += 8;
        d += 8;
        d_u8 += 8;
        width -= 8;
      } while (width != 0);
      src += 4 * src_stride;
      dst += 4 * dst_stride;
      dst8 += 4 * dst8_stride;
      height -= 4;
    } while (height != 0);
  }
}

static INLINE void dist_wtd_convolve_2d_copy_avg_neon(
    const uint8_t *src, int src_stride, uint8_t *dst8, int dst8_stride, int w,
    int h, ConvolveParams *conv_params) {
  assert(w % 4 == 0);
  assert(h % 4 == 0);

  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const uint16_t round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                                (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const uint16x8_t round_offset_vec = vdupq_n_u16(round_offset);
  const uint8x8_t shift_by_bits = vdup_n_u8(1 << (FILTER_BITS - ROUND0_BITS));

  CONV_BUF_TYPE *dst = conv_params->dst;
  const int dst_stride = conv_params->dst_stride;
  int height = h;

  if (w == 4) {
    do {
      uint8x8_t s0, s1, s2, s3;
      load_u8_8x4(src, src_stride, &s0, &s1, &s2, &s3);

      uint16x4_t d0 =
          vget_low_u16(vmlal_u8(round_offset_vec, s0, shift_by_bits));
      uint16x4_t d1 =
          vget_low_u16(vmlal_u8(round_offset_vec, s1, shift_by_bits));
      uint16x4_t d2 =
          vget_low_u16(vmlal_u8(round_offset_vec, s2, shift_by_bits));
      uint16x4_t d3 =
          vget_low_u16(vmlal_u8(round_offset_vec, s3, shift_by_bits));

      uint16x4_t dd0, dd1, dd2, dd3;
      load_u16_4x4(dst, dst_stride, &dd0, &dd1, &dd2, &dd3);

      uint8x8_t d01, d23;
      compute_basic_avg_4x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3,
                            vreinterpretq_s16_u16(round_offset_vec), &d01,
                            &d23);

      store_u8_4x1(dst8 + 0 * dst8_stride, d01, 0);
      store_u8_4x1(dst8 + 1 * dst8_stride, d01, 1);
      store_u8_4x1(dst8 + 2 * dst8_stride, d23, 0);
      store_u8_4x1(dst8 + 3 * dst8_stride, d23, 1);

      src += 4 * src_stride;
      dst += 4 * dst_stride;
      dst8 += 4 * dst8_stride;
      height -= 4;
    } while (height != 0);
  } else {
    do {
      const uint8_t *s = src;
      CONV_BUF_TYPE *d = dst;
      uint8_t *d_u8 = dst8;
      int width = w;

      do {
        uint8x8_t s0, s1, s2, s3;
        load_u8_8x4(s, src_stride, &s0, &s1, &s2, &s3);

        uint16x8_t d0 = vmlal_u8(round_offset_vec, s0, shift_by_bits);
        uint16x8_t d1 = vmlal_u8(round_offset_vec, s1, shift_by_bits);
        uint16x8_t d2 = vmlal_u8(round_offset_vec, s2, shift_by_bits);
        uint16x8_t d3 = vmlal_u8(round_offset_vec, s3, shift_by_bits);

        uint16x8_t dd0, dd1, dd2, dd3;
        load_u16_8x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

        uint8x8_t d0_u8, d1_u8, d2_u8, d3_u8;
        compute_basic_avg_8x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3,
                              vreinterpretq_s16_u16(round_offset_vec), &d0_u8,
                              &d1_u8, &d2_u8, &d3_u8);

        store_u8_8x4(d_u8, dst8_stride, d0_u8, d1_u8, d2_u8, d3_u8);

        s += 8;
        d += 8;
        d_u8 += 8;
        width -= 8;
      } while (width != 0);
      src += 4 * src_stride;
      dst += 4 * dst_stride;
      dst8 += 4 * dst8_stride;
      height -= 4;
    } while (height != 0);
  }
}

static INLINE void dist_wtd_convolve_2d_copy_neon(const uint8_t *src,
                                                  int src_stride, int w, int h,
                                                  ConvolveParams *conv_params) {
  assert(w % 4 == 0);
  assert(h % 4 == 0);

  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const uint16_t round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                                (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const uint16x8_t round_offset_vec = vdupq_n_u16(round_offset);
  const uint8x8_t shift_by_bits = vdup_n_u8(1 << (FILTER_BITS - ROUND0_BITS));

  CONV_BUF_TYPE *dst = conv_params->dst;
  const int dst_stride = conv_params->dst_stride;
  int height = h;

  if (w == 4) {
    do {
      uint8x8_t s0, s1, s2, s3;
      load_u8_8x4(src, src_stride, &s0, &s1, &s2, &s3);

      uint16x4_t d0 =
          vget_low_u16(vmlal_u8(round_offset_vec, s0, shift_by_bits));
      uint16x4_t d1 =
          vget_low_u16(vmlal_u8(round_offset_vec, s1, shift_by_bits));
      uint16x4_t d2 =
          vget_low_u16(vmlal_u8(round_offset_vec, s2, shift_by_bits));
      uint16x4_t d3 =
          vget_low_u16(vmlal_u8(round_offset_vec, s3, shift_by_bits));

      store_u16_4x4(dst, dst_stride, d0, d1, d2, d3);

      src += 4 * src_stride;
      dst += 4 * dst_stride;
      height -= 4;
    } while (height != 0);
  } else {
    do {
      const uint8_t *s = src;
      CONV_BUF_TYPE *d = dst;
      int width = w;

      do {
        uint8x8_t s0, s1, s2, s3;
        load_u8_8x4(s, src_stride, &s0, &s1, &s2, &s3);

        uint16x8_t d0 = vmlal_u8(round_offset_vec, s0, shift_by_bits);
        uint16x8_t d1 = vmlal_u8(round_offset_vec, s1, shift_by_bits);
        uint16x8_t d2 = vmlal_u8(round_offset_vec, s2, shift_by_bits);
        uint16x8_t d3 = vmlal_u8(round_offset_vec, s3, shift_by_bits);

        store_u16_8x4(d, dst_stride, d0, d1, d2, d3);

        s += 8;
        d += 8;
        width -= 8;
      } while (width != 0);
      src += 4 * src_stride;
      dst += 4 * dst_stride;
      height -= 4;
    } while (height != 0);
  }
}

void av1_dist_wtd_convolve_2d_copy_neon(const uint8_t *src, int src_stride,
                                        uint8_t *dst8, int dst8_stride, int w,
                                        int h, ConvolveParams *conv_params) {
  if (conv_params->do_average) {
    if (UNLIKELY(conv_params->use_dist_wtd_comp_avg)) {
      dist_wtd_convolve_2d_copy_dist_wtd_avg_neon(
          src, src_stride, dst8, dst8_stride, w, h, conv_params);
    } else {
      dist_wtd_convolve_2d_copy_avg_neon(src, src_stride, dst8, dst8_stride, w,
                                         h, conv_params);
    }
  } else {
    dist_wtd_convolve_2d_copy_neon(src, src_stride, w, h, conv_params);
  }
}

#if AOM_ARCH_AARCH64 && defined(__ARM_FEATURE_MATMUL_INT8)

static INLINE uint16x4_t convolve8_4_x(uint8x16_t samples,
                                       const int8x8_t x_filter,
                                       const uint8x16x2_t permute_tbl,
                                       const int32x4_t round_offset) {
  uint8x16_t permuted_samples[2];
  int32x4_t sum;

  // Permute samples ready for dot product.
  // { 0,  1,  2,  3,  1,  2,  3,  4,  2,  3,  4,  5,  3,  4,  5,  6 }
  permuted_samples[0] = vqtbl1q_u8(samples, permute_tbl.val[0]);
  // { 4,  5,  6,  7,  5,  6,  7,  8,  6,  7,  8,  9,  7,  8,  9, 10 }
  permuted_samples[1] = vqtbl1q_u8(samples, permute_tbl.val[1]);

  // First 4 output values.
  sum = vusdotq_lane_s32(round_offset, permuted_samples[0], x_filter, 0);
  sum = vusdotq_lane_s32(sum, permuted_samples[1], x_filter, 1);

  // We halved the convolution filter values so -1 from the right shift.
  return vreinterpret_u16_s16(vshrn_n_s32(sum, ROUND0_BITS - 1));
}

static INLINE uint16x8_t convolve8_8_x(uint8x16_t samples,
                                       const int8x8_t x_filter,
                                       const uint8x16x3_t permute_tbl,
                                       const int32x4_t round_offset) {
  uint8x16_t permuted_samples[3];
  int32x4_t sum[2];

  // Permute samples ready for dot product.
  // { 0,  1,  2,  3,  1,  2,  3,  4,  2,  3,  4,  5,  3,  4,  5,  6 }
  permuted_samples[0] = vqtbl1q_u8(samples, permute_tbl.val[0]);
  // { 4,  5,  6,  7,  5,  6,  7,  8,  6,  7,  8,  9,  7,  8,  9, 10 }
  permuted_samples[1] = vqtbl1q_u8(samples, permute_tbl.val[1]);
  // { 8,  9, 10, 11,  9, 10, 11, 12, 10, 11, 12, 13, 11, 12, 13, 14 }
  permuted_samples[2] = vqtbl1q_u8(samples, permute_tbl.val[2]);

  // First 4 output values.
  sum[0] = vusdotq_lane_s32(round_offset, permuted_samples[0], x_filter, 0);
  sum[0] = vusdotq_lane_s32(sum[0], permuted_samples[1], x_filter, 1);
  // Second 4 output values.
  sum[1] = vusdotq_lane_s32(round_offset, permuted_samples[1], x_filter, 0);
  sum[1] = vusdotq_lane_s32(sum[1], permuted_samples[2], x_filter, 1);

  // Narrow and re-pack.
  // We halved the convolution filter values so -1 from the right shift.
  int16x8_t res = vcombine_s16(vshrn_n_s32(sum[0], ROUND0_BITS - 1),
                               vshrn_n_s32(sum[1], ROUND0_BITS - 1));
  return vreinterpretq_u16_s16(res);
}

static INLINE void dist_wtd_convolve_x_dist_wtd_avg_neon(
    const uint8_t *src, int src_stride, uint8_t *dst8, int dst8_stride, int w,
    int h, const InterpFilterParams *filter_params_x, const int subpel_x_qn,
    ConvolveParams *conv_params) {
  assert(w % 4 == 0);
  assert(h % 4 == 0);

  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int16_t round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                               (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const int16x8_t round_offset_vec = vdupq_n_s16(round_offset);
  // A shim of 1 << ((ROUND0_BITS - 1) - 1) enables us to use non-rounding
  // shifts - which are generally faster than rounding shifts on modern CPUs.
  // (The extra -1 is needed because we halved the filter values.)
  const int32x4_t round_offset_shim = vdupq_n_s32(
      (round_offset << (ROUND0_BITS - 1)) + (1 << ((ROUND0_BITS - 1) - 1)));

  const uint16_t fwd_offset = conv_params->fwd_offset;
  const uint16_t bck_offset = conv_params->bck_offset;

  // Horizontal filter.
  const int16_t *x_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_x, subpel_x_qn & SUBPEL_MASK);
  // Filter values are even, so downshift by 1 to reduce intermediate precision
  // requirements.
  const int8x8_t x_filter = vshrn_n_s16(vld1q_s16(x_filter_ptr), 1);

  const int horiz_offset = filter_params_x->taps / 2 - 1;
  const uint8_t *src_ptr = src - horiz_offset;
  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  uint8_t *dst8_ptr = dst8;
  int dst_stride = conv_params->dst_stride;
  int height = h;

  if (w == 4) {
    const uint8x16x2_t permute_tbl = vld1q_u8_x2(dot_prod_permute_tbl);

    do {
      uint8x16_t s0, s1, s2, s3;
      load_u8_16x4(src_ptr, src_stride, &s0, &s1, &s2, &s3);

      uint16x4_t d0 =
          convolve8_4_x(s0, x_filter, permute_tbl, round_offset_shim);
      uint16x4_t d1 =
          convolve8_4_x(s1, x_filter, permute_tbl, round_offset_shim);
      uint16x4_t d2 =
          convolve8_4_x(s2, x_filter, permute_tbl, round_offset_shim);
      uint16x4_t d3 =
          convolve8_4_x(s3, x_filter, permute_tbl, round_offset_shim);

      uint16x4_t dd0, dd1, dd2, dd3;
      load_u16_4x4(dst_ptr, dst_stride, &dd0, &dd1, &dd2, &dd3);

      uint8x8_t d01_u8, d23_u8;
      compute_dist_wtd_avg_4x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3, fwd_offset,
                               bck_offset, round_offset_vec, &d01_u8, &d23_u8);

      store_u8_4x1(dst8_ptr + 0 * dst8_stride, d01_u8, 0);
      store_u8_4x1(dst8_ptr + 1 * dst8_stride, d01_u8, 1);
      store_u8_4x1(dst8_ptr + 2 * dst8_stride, d23_u8, 0);
      store_u8_4x1(dst8_ptr + 3 * dst8_stride, d23_u8, 1);

      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      dst8_ptr += 4 * dst8_stride;
      height -= 4;
    } while (height != 0);
  } else {
    const uint8x16x3_t permute_tbl = vld1q_u8_x3(dot_prod_permute_tbl);

    do {
      const uint8_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      uint8_t *d_u8 = dst8_ptr;
      int width = w;

      do {
        uint8x16_t s0, s1, s2, s3;
        load_u8_16x4(s, src_stride, &s0, &s1, &s2, &s3);

        uint16x8_t d0 =
            convolve8_8_x(s0, x_filter, permute_tbl, round_offset_shim);
        uint16x8_t d1 =
            convolve8_8_x(s1, x_filter, permute_tbl, round_offset_shim);
        uint16x8_t d2 =
            convolve8_8_x(s2, x_filter, permute_tbl, round_offset_shim);
        uint16x8_t d3 =
            convolve8_8_x(s3, x_filter, permute_tbl, round_offset_shim);

        uint16x8_t dd0, dd1, dd2, dd3;
        load_u16_8x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

        uint8x8_t d0_u8, d1_u8, d2_u8, d3_u8;
        compute_dist_wtd_avg_8x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3, fwd_offset,
                                 bck_offset, round_offset_vec, &d0_u8, &d1_u8,
                                 &d2_u8, &d3_u8);

        store_u8_8x4(d_u8, dst8_stride, d0_u8, d1_u8, d2_u8, d3_u8);

        s += 8;
        d += 8;
        d_u8 += 8;
        width -= 8;
      } while (width != 0);
      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      dst8_ptr += 4 * dst8_stride;
      height -= 4;
    } while (height != 0);
  }
}

static INLINE void dist_wtd_convolve_x_avg_neon(
    const uint8_t *src, int src_stride, uint8_t *dst8, int dst8_stride, int w,
    int h, const InterpFilterParams *filter_params_x, const int subpel_x_qn,
    ConvolveParams *conv_params) {
  assert(w % 4 == 0);
  assert(h % 4 == 0);

  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int16_t round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                               (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const int16x8_t round_offset_vec = vdupq_n_s16(round_offset);
  // A shim of 1 << ((ROUND0_BITS - 1) - 1) enables us to use non-rounding
  // shifts - which are generally faster than rounding shifts on modern CPUs.
  // (The extra -1 is needed because we halved the filter values.)
  const int32x4_t round_offset_shim = vdupq_n_s32(
      (round_offset << (ROUND0_BITS - 1)) + (1 << ((ROUND0_BITS - 1) - 1)));

  // Horizontal filter.
  const int16_t *x_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_x, subpel_x_qn & SUBPEL_MASK);
  // Filter values are even, so downshift by 1 to reduce intermediate precision
  // requirements.
  const int8x8_t x_filter = vshrn_n_s16(vld1q_s16(x_filter_ptr), 1);

  const int horiz_offset = filter_params_x->taps / 2 - 1;
  const uint8_t *src_ptr = src - horiz_offset;
  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  uint8_t *dst8_ptr = dst8;
  int dst_stride = conv_params->dst_stride;
  int height = h;

  if (w == 4) {
    const uint8x16x2_t permute_tbl = vld1q_u8_x2(dot_prod_permute_tbl);

    do {
      uint8x16_t s0, s1, s2, s3;
      load_u8_16x4(src_ptr, src_stride, &s0, &s1, &s2, &s3);

      uint16x4_t d0 =
          convolve8_4_x(s0, x_filter, permute_tbl, round_offset_shim);
      uint16x4_t d1 =
          convolve8_4_x(s1, x_filter, permute_tbl, round_offset_shim);
      uint16x4_t d2 =
          convolve8_4_x(s2, x_filter, permute_tbl, round_offset_shim);
      uint16x4_t d3 =
          convolve8_4_x(s3, x_filter, permute_tbl, round_offset_shim);

      uint16x4_t dd0, dd1, dd2, dd3;
      load_u16_4x4(dst_ptr, dst_stride, &dd0, &dd1, &dd2, &dd3);

      uint8x8_t d01_u8, d23_u8;
      compute_basic_avg_4x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3,
                            round_offset_vec, &d01_u8, &d23_u8);

      store_u8_4x1(dst8_ptr + 0 * dst8_stride, d01_u8, 0);
      store_u8_4x1(dst8_ptr + 1 * dst8_stride, d01_u8, 1);
      store_u8_4x1(dst8_ptr + 2 * dst8_stride, d23_u8, 0);
      store_u8_4x1(dst8_ptr + 3 * dst8_stride, d23_u8, 1);

      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      dst8_ptr += 4 * dst8_stride;
      height -= 4;
    } while (height != 0);
  } else {
    const uint8x16x3_t permute_tbl = vld1q_u8_x3(dot_prod_permute_tbl);

    do {
      const uint8_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      uint8_t *d_u8 = dst8_ptr;
      int width = w;

      do {
        uint8x16_t s0, s1, s2, s3;
        load_u8_16x4(s, src_stride, &s0, &s1, &s2, &s3);

        uint16x8_t d0 =
            convolve8_8_x(s0, x_filter, permute_tbl, round_offset_shim);
        uint16x8_t d1 =
            convolve8_8_x(s1, x_filter, permute_tbl, round_offset_shim);
        uint16x8_t d2 =
            convolve8_8_x(s2, x_filter, permute_tbl, round_offset_shim);
        uint16x8_t d3 =
            convolve8_8_x(s3, x_filter, permute_tbl, round_offset_shim);

        uint16x8_t dd0, dd1, dd2, dd3;
        load_u16_8x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

        uint8x8_t d0_u8, d1_u8, d2_u8, d3_u8;
        compute_basic_avg_8x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3,
                              round_offset_vec, &d0_u8, &d1_u8, &d2_u8, &d3_u8);

        store_u8_8x4(d_u8, dst8_stride, d0_u8, d1_u8, d2_u8, d3_u8);

        s += 8;
        d += 8;
        d_u8 += 8;
        width -= 8;
      } while (width != 0);
      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      dst8_ptr += 4 * dst8_stride;
      height -= 4;
    } while (height != 0);
  }
}

static INLINE void dist_wtd_convolve_x_neon(
    const uint8_t *src, int src_stride, int w, int h,
    const InterpFilterParams *filter_params_x, const int subpel_x_qn,
    ConvolveParams *conv_params) {
  assert(w % 4 == 0);
  assert(h % 4 == 0);

  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int16_t round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                               (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  // A shim of 1 << ((ROUND0_BITS - 1) - 1) enables us to use non-rounding
  // shifts - which are generally faster than rounding shifts on modern CPUs.
  // (The extra -1 is needed because we halved the filter values.)
  const int32x4_t round_offset_shim = vdupq_n_s32(
      (round_offset << (ROUND0_BITS - 1)) + (1 << ((ROUND0_BITS - 1) - 1)));

  // Horizontal filter.
  const int16_t *x_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_x, subpel_x_qn & SUBPEL_MASK);
  // Filter values are even, so downshift by 1 to reduce intermediate precision
  // requirements.
  const int8x8_t x_filter = vshrn_n_s16(vld1q_s16(x_filter_ptr), 1);

  const int horiz_offset = filter_params_x->taps / 2 - 1;
  const uint8_t *src_ptr = src - horiz_offset;
  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  int dst_stride = conv_params->dst_stride;
  int height = h;

  if (w == 4) {
    const uint8x16x2_t permute_tbl = vld1q_u8_x2(dot_prod_permute_tbl);

    do {
      uint8x16_t s0, s1, s2, s3;
      load_u8_16x4(src_ptr, src_stride, &s0, &s1, &s2, &s3);

      uint16x4_t d0 =
          convolve8_4_x(s0, x_filter, permute_tbl, round_offset_shim);
      uint16x4_t d1 =
          convolve8_4_x(s1, x_filter, permute_tbl, round_offset_shim);
      uint16x4_t d2 =
          convolve8_4_x(s2, x_filter, permute_tbl, round_offset_shim);
      uint16x4_t d3 =
          convolve8_4_x(s3, x_filter, permute_tbl, round_offset_shim);

      store_u16_4x4(dst_ptr, dst_stride, d0, d1, d2, d3);

      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      height -= 4;
    } while (height != 0);
  } else {
    const uint8x16x3_t permute_tbl = vld1q_u8_x3(dot_prod_permute_tbl);

    do {
      const uint8_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      int width = w;

      do {
        uint8x16_t s0, s1, s2, s3;
        load_u8_16x4(s, src_stride, &s0, &s1, &s2, &s3);

        uint16x8_t d0 =
            convolve8_8_x(s0, x_filter, permute_tbl, round_offset_shim);
        uint16x8_t d1 =
            convolve8_8_x(s1, x_filter, permute_tbl, round_offset_shim);
        uint16x8_t d2 =
            convolve8_8_x(s2, x_filter, permute_tbl, round_offset_shim);
        uint16x8_t d3 =
            convolve8_8_x(s3, x_filter, permute_tbl, round_offset_shim);

        store_u16_8x4(d, dst_stride, d0, d1, d2, d3);

        s += 8;
        d += 8;
        width -= 8;
      } while (width != 0);
      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      height -= 4;
    } while (height != 0);
  }
}

#elif AOM_ARCH_AARCH64 && defined(__ARM_FEATURE_DOTPROD)

static INLINE uint16x4_t convolve8_4_x(uint8x16_t samples,
                                       const int8x8_t x_filter,
                                       const int32x4_t correction,
                                       const uint8x16_t range_limit,
                                       const uint8x16x2_t permute_tbl) {
  int8x16_t clamped_samples, permuted_samples[2];
  int32x4_t sum;

  // Clamp sample range to [-128, 127] for 8-bit signed dot product.
  clamped_samples = vreinterpretq_s8_u8(vsubq_u8(samples, range_limit));

  // Permute samples ready for dot product.
  // { 0,  1,  2,  3,  1,  2,  3,  4,  2,  3,  4,  5,  3,  4,  5,  6 }
  permuted_samples[0] = vqtbl1q_s8(clamped_samples, permute_tbl.val[0]);
  // { 4,  5,  6,  7,  5,  6,  7,  8,  6,  7,  8,  9,  7,  8,  9, 10 }
  permuted_samples[1] = vqtbl1q_s8(clamped_samples, permute_tbl.val[1]);

  // Accumulate dot product into 'correction' to account for range clamp.
  sum = vdotq_lane_s32(correction, permuted_samples[0], x_filter, 0);
  sum = vdotq_lane_s32(sum, permuted_samples[1], x_filter, 1);

  // We halved the convolution filter values so -1 from the right shift.
  return vreinterpret_u16_s16(vshrn_n_s32(sum, ROUND0_BITS - 1));
}

static INLINE uint16x8_t convolve8_8_x(uint8x16_t samples,
                                       const int8x8_t x_filter,
                                       const int32x4_t correction,
                                       const uint8x16_t range_limit,
                                       const uint8x16x3_t permute_tbl) {
  int8x16_t clamped_samples, permuted_samples[3];
  int32x4_t sum[2];

  // Clamp sample range to [-128, 127] for 8-bit signed dot product.
  clamped_samples = vreinterpretq_s8_u8(vsubq_u8(samples, range_limit));

  // Permute samples ready for dot product. */
  // { 0,  1,  2,  3,  1,  2,  3,  4,  2,  3,  4,  5,  3,  4,  5,  6 }
  permuted_samples[0] = vqtbl1q_s8(clamped_samples, permute_tbl.val[0]);
  // { 4,  5,  6,  7,  5,  6,  7,  8,  6,  7,  8,  9,  7,  8,  9, 10 }
  permuted_samples[1] = vqtbl1q_s8(clamped_samples, permute_tbl.val[1]);
  // { 8,  9, 10, 11,  9, 10, 11, 12, 10, 11, 12, 13, 11, 12, 13, 14 }
  permuted_samples[2] = vqtbl1q_s8(clamped_samples, permute_tbl.val[2]);

  // Accumulate dot product into 'correction' to account for range clamp.
  // First 4 output values.
  sum[0] = vdotq_lane_s32(correction, permuted_samples[0], x_filter, 0);
  sum[0] = vdotq_lane_s32(sum[0], permuted_samples[1], x_filter, 1);
  // Second 4 output values.
  sum[1] = vdotq_lane_s32(correction, permuted_samples[1], x_filter, 0);
  sum[1] = vdotq_lane_s32(sum[1], permuted_samples[2], x_filter, 1);

  // Narrow and re-pack.
  // We halved the convolution filter values so -1 from the right shift.
  int16x8_t res = vcombine_s16(vshrn_n_s32(sum[0], ROUND0_BITS - 1),
                               vshrn_n_s32(sum[1], ROUND0_BITS - 1));
  return vreinterpretq_u16_s16(res);
}

static INLINE void dist_wtd_convolve_x_dist_wtd_avg_neon(
    const uint8_t *src, int src_stride, uint8_t *dst8, int dst8_stride, int w,
    int h, const InterpFilterParams *filter_params_x, const int subpel_x_qn,
    ConvolveParams *conv_params) {
  assert(w % 4 == 0);
  assert(h % 4 == 0);

  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int16_t round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                               (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const int16x8_t round_offset_vec = vdupq_n_s16(round_offset);

  const uint16_t fwd_offset = conv_params->fwd_offset;
  const uint16_t bck_offset = conv_params->bck_offset;

  // Horizontal filter.
  const int16_t *x_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_x, subpel_x_qn & SUBPEL_MASK);
  // Filter values are even, so downshift by 1 to reduce intermediate precision
  // requirements.
  const int8x8_t x_filter = vshrn_n_s16(vld1q_s16(x_filter_ptr), 1);

  // Dot-product constants and other shims.
  const uint8x16_t range_limit = vdupq_n_u8(128);
  const int32_t correction_s32 = vaddlvq_s16(vshll_n_s8(x_filter, 7));
  // Fold round_offset into the dot-product filter correction constant. The
  // additional shim of 1 << ((ROUND0_BITS - 1) - 1) enables us to use non-
  // rounding shifts - which are generally faster than rounding shifts on
  // modern CPUs. (The extra -1 is needed because we halved the filter values.)
  int32x4_t correction =
      vdupq_n_s32(correction_s32 + (round_offset << (ROUND0_BITS - 1)) +
                  (1 << ((ROUND0_BITS - 1) - 1)));

  const int horiz_offset = filter_params_x->taps / 2 - 1;
  const uint8_t *src_ptr = src - horiz_offset;
  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  uint8_t *dst8_ptr = dst8;
  int dst_stride = conv_params->dst_stride;
  int height = h;

  if (w == 4) {
    const uint8x16x2_t permute_tbl = vld1q_u8_x2(dot_prod_permute_tbl);

    do {
      uint8x16_t s0, s1, s2, s3;
      load_u8_16x4(src_ptr, src_stride, &s0, &s1, &s2, &s3);

      uint16x4_t d0 =
          convolve8_4_x(s0, x_filter, correction, range_limit, permute_tbl);
      uint16x4_t d1 =
          convolve8_4_x(s1, x_filter, correction, range_limit, permute_tbl);
      uint16x4_t d2 =
          convolve8_4_x(s2, x_filter, correction, range_limit, permute_tbl);
      uint16x4_t d3 =
          convolve8_4_x(s3, x_filter, correction, range_limit, permute_tbl);

      uint16x4_t dd0, dd1, dd2, dd3;
      load_u16_4x4(dst_ptr, dst_stride, &dd0, &dd1, &dd2, &dd3);

      uint8x8_t d01_u8, d23_u8;
      compute_dist_wtd_avg_4x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3, fwd_offset,
                               bck_offset, round_offset_vec, &d01_u8, &d23_u8);

      store_u8_4x1(dst8_ptr + 0 * dst8_stride, d01_u8, 0);
      store_u8_4x1(dst8_ptr + 1 * dst8_stride, d01_u8, 1);
      store_u8_4x1(dst8_ptr + 2 * dst8_stride, d23_u8, 0);
      store_u8_4x1(dst8_ptr + 3 * dst8_stride, d23_u8, 1);

      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      dst8_ptr += 4 * dst8_stride;
      height -= 4;
    } while (height != 0);
  } else {
    const uint8x16x3_t permute_tbl = vld1q_u8_x3(dot_prod_permute_tbl);

    do {
      const uint8_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      uint8_t *d_u8 = dst8_ptr;
      int width = w;

      do {
        uint8x16_t s0, s1, s2, s3;
        load_u8_16x4(s, src_stride, &s0, &s1, &s2, &s3);

        uint16x8_t d0 =
            convolve8_8_x(s0, x_filter, correction, range_limit, permute_tbl);
        uint16x8_t d1 =
            convolve8_8_x(s1, x_filter, correction, range_limit, permute_tbl);
        uint16x8_t d2 =
            convolve8_8_x(s2, x_filter, correction, range_limit, permute_tbl);
        uint16x8_t d3 =
            convolve8_8_x(s3, x_filter, correction, range_limit, permute_tbl);

        uint16x8_t dd0, dd1, dd2, dd3;
        load_u16_8x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

        uint8x8_t d0_u8, d1_u8, d2_u8, d3_u8;
        compute_dist_wtd_avg_8x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3, fwd_offset,
                                 bck_offset, round_offset_vec, &d0_u8, &d1_u8,
                                 &d2_u8, &d3_u8);

        store_u8_8x4(d_u8, dst8_stride, d0_u8, d1_u8, d2_u8, d3_u8);

        s += 8;
        d += 8;
        d_u8 += 8;
        width -= 8;
      } while (width != 0);
      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      dst8_ptr += 4 * dst8_stride;
      height -= 4;
    } while (height != 0);
  }
}

static INLINE void dist_wtd_convolve_x_avg_neon(
    const uint8_t *src, int src_stride, uint8_t *dst8, int dst8_stride, int w,
    int h, const InterpFilterParams *filter_params_x, const int subpel_x_qn,
    ConvolveParams *conv_params) {
  assert(w % 4 == 0);
  assert(h % 4 == 0);

  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int16_t round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                               (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const int16x8_t round_offset_vec = vdupq_n_s16(round_offset);

  // Horizontal filter.
  const int16_t *x_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_x, subpel_x_qn & SUBPEL_MASK);
  // Filter values are even, so downshift by 1 to reduce intermediate precision
  // requirements.
  const int8x8_t x_filter = vshrn_n_s16(vld1q_s16(x_filter_ptr), 1);

  // Dot-product constants and other shims.
  const uint8x16_t range_limit = vdupq_n_u8(128);
  const int32_t correction_s32 = vaddlvq_s16(vshll_n_s8(x_filter, 7));
  // Fold round_offset into the dot-product filter correction constant. The
  // additional shim of 1 << ((ROUND0_BITS - 1) - 1) enables us to use non-
  // rounding shifts - which are generally faster than rounding shifts on
  // modern CPUs. (The extra -1 is needed because we halved the filter values.)
  int32x4_t correction =
      vdupq_n_s32(correction_s32 + (round_offset << (ROUND0_BITS - 1)) +
                  (1 << ((ROUND0_BITS - 1) - 1)));

  const int horiz_offset = filter_params_x->taps / 2 - 1;
  const uint8_t *src_ptr = src - horiz_offset;
  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  uint8_t *dst8_ptr = dst8;
  int dst_stride = conv_params->dst_stride;
  int height = h;

  if (w == 4) {
    const uint8x16x2_t permute_tbl = vld1q_u8_x2(dot_prod_permute_tbl);

    do {
      uint8x16_t s0, s1, s2, s3;
      load_u8_16x4(src_ptr, src_stride, &s0, &s1, &s2, &s3);

      uint16x4_t d0 =
          convolve8_4_x(s0, x_filter, correction, range_limit, permute_tbl);
      uint16x4_t d1 =
          convolve8_4_x(s1, x_filter, correction, range_limit, permute_tbl);
      uint16x4_t d2 =
          convolve8_4_x(s2, x_filter, correction, range_limit, permute_tbl);
      uint16x4_t d3 =
          convolve8_4_x(s3, x_filter, correction, range_limit, permute_tbl);

      uint16x4_t dd0, dd1, dd2, dd3;
      load_u16_4x4(dst_ptr, dst_stride, &dd0, &dd1, &dd2, &dd3);

      uint8x8_t d01_u8, d23_u8;
      compute_basic_avg_4x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3,
                            round_offset_vec, &d01_u8, &d23_u8);

      store_u8_4x1(dst8_ptr + 0 * dst8_stride, d01_u8, 0);
      store_u8_4x1(dst8_ptr + 1 * dst8_stride, d01_u8, 1);
      store_u8_4x1(dst8_ptr + 2 * dst8_stride, d23_u8, 0);
      store_u8_4x1(dst8_ptr + 3 * dst8_stride, d23_u8, 1);

      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      dst8_ptr += 4 * dst8_stride;
      height -= 4;
    } while (height != 0);
  } else {
    const uint8x16x3_t permute_tbl = vld1q_u8_x3(dot_prod_permute_tbl);

    do {
      const uint8_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      uint8_t *d_u8 = dst8_ptr;
      int width = w;

      do {
        uint8x16_t s0, s1, s2, s3;
        load_u8_16x4(s, src_stride, &s0, &s1, &s2, &s3);

        uint16x8_t d0 =
            convolve8_8_x(s0, x_filter, correction, range_limit, permute_tbl);
        uint16x8_t d1 =
            convolve8_8_x(s1, x_filter, correction, range_limit, permute_tbl);
        uint16x8_t d2 =
            convolve8_8_x(s2, x_filter, correction, range_limit, permute_tbl);
        uint16x8_t d3 =
            convolve8_8_x(s3, x_filter, correction, range_limit, permute_tbl);

        uint16x8_t dd0, dd1, dd2, dd3;
        load_u16_8x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

        uint8x8_t d0_u8, d1_u8, d2_u8, d3_u8;
        compute_basic_avg_8x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3,
                              round_offset_vec, &d0_u8, &d1_u8, &d2_u8, &d3_u8);

        store_u8_8x4(d_u8, dst8_stride, d0_u8, d1_u8, d2_u8, d3_u8);

        s += 8;
        d += 8;
        d_u8 += 8;
        width -= 8;
      } while (width != 0);
      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      dst8_ptr += 4 * dst8_stride;
      height -= 4;
    } while (height != 0);
  }
}

static INLINE void dist_wtd_convolve_x_neon(
    const uint8_t *src, int src_stride, int w, int h,
    const InterpFilterParams *filter_params_x, const int subpel_x_qn,
    ConvolveParams *conv_params) {
  assert(w % 4 == 0);
  assert(h % 4 == 0);

  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int16_t round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                               (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));

  // Horizontal filter.
  const int16_t *x_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_x, subpel_x_qn & SUBPEL_MASK);
  // Filter values are even, so downshift by 1 to reduce intermediate precision
  // requirements.
  const int8x8_t x_filter = vshrn_n_s16(vld1q_s16(x_filter_ptr), 1);

  // Dot-product constants and other shims.
  const uint8x16_t range_limit = vdupq_n_u8(128);
  const int32_t correction_s32 = vaddlvq_s16(vshll_n_s8(x_filter, 7));
  // Fold round_offset into the dot-product filter correction constant. The
  // additional shim of 1 << ((ROUND0_BITS - 1) - 1) enables us to use non-
  // rounding shifts - which are generally faster than rounding shifts on
  // modern CPUs. (The extra -1 is needed because we halved the filter values.)
  int32x4_t correction =
      vdupq_n_s32(correction_s32 + (round_offset << (ROUND0_BITS - 1)) +
                  (1 << ((ROUND0_BITS - 1) - 1)));

  const int horiz_offset = filter_params_x->taps / 2 - 1;
  const uint8_t *src_ptr = src - horiz_offset;
  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  int dst_stride = conv_params->dst_stride;
  int height = h;

  if (w == 4) {
    const uint8x16x2_t permute_tbl = vld1q_u8_x2(dot_prod_permute_tbl);

    do {
      uint8x16_t s0, s1, s2, s3;
      load_u8_16x4(src_ptr, src_stride, &s0, &s1, &s2, &s3);

      uint16x4_t d0 =
          convolve8_4_x(s0, x_filter, correction, range_limit, permute_tbl);
      uint16x4_t d1 =
          convolve8_4_x(s1, x_filter, correction, range_limit, permute_tbl);
      uint16x4_t d2 =
          convolve8_4_x(s2, x_filter, correction, range_limit, permute_tbl);
      uint16x4_t d3 =
          convolve8_4_x(s3, x_filter, correction, range_limit, permute_tbl);

      store_u16_4x4(dst_ptr, dst_stride, d0, d1, d2, d3);

      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      height -= 4;
    } while (height != 0);
  } else {
    const uint8x16x3_t permute_tbl = vld1q_u8_x3(dot_prod_permute_tbl);

    do {
      const uint8_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      int width = w;

      do {
        uint8x16_t s0, s1, s2, s3;
        load_u8_16x4(s, src_stride, &s0, &s1, &s2, &s3);

        uint16x8_t d0 =
            convolve8_8_x(s0, x_filter, correction, range_limit, permute_tbl);
        uint16x8_t d1 =
            convolve8_8_x(s1, x_filter, correction, range_limit, permute_tbl);
        uint16x8_t d2 =
            convolve8_8_x(s2, x_filter, correction, range_limit, permute_tbl);
        uint16x8_t d3 =
            convolve8_8_x(s3, x_filter, correction, range_limit, permute_tbl);

        store_u16_8x4(d, dst_stride, d0, d1, d2, d3);

        s += 8;
        d += 8;
        width -= 8;
      } while (width != 0);
      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      height -= 4;
    } while (height != 0);
  }
}

#else  // !(AOM_ARCH_AARCH64 && defined(__ARM_FEATURE_DOTPROD))

static INLINE uint16x4_t convolve8_4_x(const int16x4_t s0, const int16x4_t s1,
                                       const int16x4_t s2, const int16x4_t s3,
                                       const int16x4_t s4, const int16x4_t s5,
                                       const int16x4_t s6, const int16x4_t s7,
                                       const int16x8_t x_filter,
                                       const int16x4_t round_offset) {
  const int16x4_t x_filter_0_3 = vget_low_s16(x_filter);
  const int16x4_t x_filter_4_7 = vget_high_s16(x_filter);

  int16x4_t sum = vmul_lane_s16(s0, x_filter_0_3, 0);
  sum = vmla_lane_s16(sum, s1, x_filter_0_3, 1);
  sum = vmla_lane_s16(sum, s2, x_filter_0_3, 2);
  sum = vmla_lane_s16(sum, s3, x_filter_0_3, 3);
  sum = vmla_lane_s16(sum, s4, x_filter_4_7, 0);
  sum = vmla_lane_s16(sum, s5, x_filter_4_7, 1);
  sum = vmla_lane_s16(sum, s6, x_filter_4_7, 2);
  sum = vmla_lane_s16(sum, s7, x_filter_4_7, 3);

  // We halved the convolution filter values so -1 from the right shift.
  int16x4_t res = vrsra_n_s16(round_offset, sum, ROUND0_BITS - 1);
  return vreinterpret_u16_s16(res);
}

static INLINE uint16x8_t convolve8_8_x(const int16x8_t s0, const int16x8_t s1,
                                       const int16x8_t s2, const int16x8_t s3,
                                       const int16x8_t s4, const int16x8_t s5,
                                       const int16x8_t s6, const int16x8_t s7,
                                       const int16x8_t x_filter,
                                       const int16x8_t round_offset) {
  const int16x4_t x_filter_0_3 = vget_low_s16(x_filter);
  const int16x4_t x_filter_4_7 = vget_high_s16(x_filter);

  int16x8_t sum = vmulq_lane_s16(s0, x_filter_0_3, 0);
  sum = vmlaq_lane_s16(sum, s1, x_filter_0_3, 1);
  sum = vmlaq_lane_s16(sum, s2, x_filter_0_3, 2);
  sum = vmlaq_lane_s16(sum, s3, x_filter_0_3, 3);
  sum = vmlaq_lane_s16(sum, s4, x_filter_4_7, 0);
  sum = vmlaq_lane_s16(sum, s5, x_filter_4_7, 1);
  sum = vmlaq_lane_s16(sum, s6, x_filter_4_7, 2);
  sum = vmlaq_lane_s16(sum, s7, x_filter_4_7, 3);

  // We halved the convolution filter values so -1 from the right shift.
  int16x8_t res = vrsraq_n_s16(round_offset, sum, ROUND0_BITS - 1);
  return vreinterpretq_u16_s16(res);
}

static INLINE void dist_wtd_convolve_x_dist_wtd_avg_neon(
    const uint8_t *src, int src_stride, uint8_t *dst8, int dst8_stride, int w,
    int h, const InterpFilterParams *filter_params_x, const int subpel_x_qn,
    ConvolveParams *conv_params) {
  assert(w % 4 == 0);
  assert(h % 4 == 0);

  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int16_t round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                               (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const int16x8_t round_offset_vec = vdupq_n_s16(round_offset);

  const uint16_t fwd_offset = conv_params->fwd_offset;
  const uint16_t bck_offset = conv_params->bck_offset;

  // Horizontal filter.
  const int16_t *x_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_x, subpel_x_qn & SUBPEL_MASK);
  // Filter values are even, so downshift by 1 to reduce intermediate precision
  // requirements.
  const int16x8_t x_filter = vshrq_n_s16(vld1q_s16(x_filter_ptr), 1);

  const int horiz_offset = filter_params_x->taps / 2 - 1;
  const uint8_t *src_ptr = src - horiz_offset;
  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  uint8_t *dst8_ptr = dst8;
  int dst_stride = conv_params->dst_stride;
  const uint8_t *s;
  uint8_t *d_u8;
  CONV_BUF_TYPE *d;
  int width;
  int height = h;

  if (w == 4 || h == 4) {
    do {
      d = dst_ptr;
      d_u8 = dst8_ptr;
      width = w;

      __builtin_prefetch(src_ptr + 0 * src_stride);
#if AOM_ARCH_AARCH64
      __builtin_prefetch(src_ptr + 1 * src_stride);
      __builtin_prefetch(src_ptr + 2 * src_stride);
      __builtin_prefetch(src_ptr + 3 * src_stride);

      uint8x8_t t0, t1, t2, t3;
      load_u8_8x4(src_ptr, src_stride, &t0, &t1, &t2, &t3);
      transpose_u8_8x4(&t0, &t1, &t2, &t3);

      int16x4_t s0 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
      int16x4_t s1 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t1)));
      int16x4_t s2 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t2)));
      int16x4_t s3 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t3)));
      int16x4_t s4 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
      int16x4_t s5 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t1)));
      int16x4_t s6 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t2)));

      __builtin_prefetch(d + 0 * dst_stride);
      __builtin_prefetch(d + 1 * dst_stride);
      __builtin_prefetch(d + 2 * dst_stride);
      __builtin_prefetch(d + 3 * dst_stride);

      s = src_ptr + 7;

      do {
        load_unaligned_u8_4x4(s, src_stride, &t0, &t1);
        transpose_u8_4x4(&t0, &t1);

        int16x4_t s7 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
        int16x4_t s8 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t1)));
        int16x4_t s9 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
        int16x4_t s10 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t1)));

        uint16x4_t d0 = convolve8_4_x(s0, s1, s2, s3, s4, s5, s6, s7, x_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d1 = convolve8_4_x(s1, s2, s3, s4, s5, s6, s7, s8, x_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d2 = convolve8_4_x(s2, s3, s4, s5, s6, s7, s8, s9, x_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d3 = convolve8_4_x(s3, s4, s5, s6, s7, s8, s9, s10, x_filter,
                                      vget_low_s16(round_offset_vec));

        transpose_u16_4x4d(&d0, &d1, &d2, &d3);

        __builtin_prefetch(d + 0 * dst_stride);
        __builtin_prefetch(d + 1 * dst_stride);
        __builtin_prefetch(d + 2 * dst_stride);
        __builtin_prefetch(d + 3 * dst_stride);

        __builtin_prefetch(d_u8 + 0 * dst8_stride);
        __builtin_prefetch(d_u8 + 1 * dst8_stride);
        __builtin_prefetch(d_u8 + 2 * dst8_stride);
        __builtin_prefetch(d_u8 + 3 * dst8_stride);

        uint16x4_t dd0, dd1, dd2, dd3;
        load_u16_4x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

        uint8x8_t d01, d23;
        compute_dist_wtd_avg_4x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3, fwd_offset,
                                 bck_offset, round_offset_vec, &d01, &d23);

        store_u8_4x1(d_u8 + 0 * dst8_stride, d01, 0);
        store_u8_4x1(d_u8 + 1 * dst8_stride, d01, 1);
        store_u8_4x1(d_u8 + 2 * dst8_stride, d23, 0);
        store_u8_4x1(d_u8 + 3 * dst8_stride, d23, 1);

        s0 = s4;
        s1 = s5;
        s2 = s6;
        s3 = s7;
        s4 = s8;
        s5 = s9;
        s6 = s10;
        s += 4;
        d += 4;
        d_u8 += 4;
        width -= 4;
      } while (width != 0);
      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      dst8_ptr += 4 * dst8_stride;
      height -= 4;
#else   // !AOM_ARCH_AARCH64
      uint8x8_t t0 = vld1_u8(src_ptr);  // a0 a1 a2 a3 a4 a5 a6 a7
      int16x4_t s0 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
      int16x4_t s4 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));

      __builtin_prefetch(d);

      s = src_ptr + 8;

      do {
        t0 = vld1_u8(s);  // a8 a9 a10 a11
        int16x4_t s8 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));

        int16x4_t s1 = vext_s16(s0, s4, 1);  // a1 a2 a3 a4
        int16x4_t s2 = vext_s16(s0, s4, 2);  // a2 a3 a4 a5
        int16x4_t s3 = vext_s16(s0, s4, 3);  // a3 a4 a5 a6
        int16x4_t s5 = vext_s16(s4, s8, 1);  // a5 a6 a7 a8
        int16x4_t s6 = vext_s16(s4, s8, 2);  // a6 a7 a8 a9
        int16x4_t s7 = vext_s16(s4, s8, 3);  // a7 a8 a9 a10

        uint16x4_t d0 = convolve8_4_x(s0, s1, s2, s3, s4, s5, s6, s7, x_filter,
                                      vget_low_s16(round_offset_vec));

        __builtin_prefetch(d);
        __builtin_prefetch(d_u8);

        uint16x4_t dd0 = vld1_u16(d);

        uint8x8_t d01;
        compute_dist_wtd_avg_4x1(dd0, d0, fwd_offset, bck_offset,
                                 vget_low_s16(round_offset_vec), &d01);

        store_u8_4x1(d_u8, d01, 0);

        s0 = s4;
        s4 = s8;
        s += 4;
        d += 4;
        d_u8 += 4;
        width -= 4;
      } while (width != 0);
      src_ptr += src_stride;
      dst_ptr += dst_stride;
      dst8_ptr += dst8_stride;
      height--;
#endif  // AOM_ARCH_AARCH64
    } while (height != 0);
  } else {
    do {
      d = dst_ptr;
      d_u8 = dst8_ptr;
      width = w;

#if AOM_ARCH_AARCH64
      __builtin_prefetch(src_ptr + 0 * src_stride);
      __builtin_prefetch(src_ptr + 1 * src_stride);
      __builtin_prefetch(src_ptr + 2 * src_stride);
      __builtin_prefetch(src_ptr + 3 * src_stride);
      __builtin_prefetch(src_ptr + 4 * src_stride);
      __builtin_prefetch(src_ptr + 5 * src_stride);
      __builtin_prefetch(src_ptr + 6 * src_stride);
      __builtin_prefetch(src_ptr + 7 * src_stride);

      uint8x8_t t0, t1, t2, t3, t4, t5, t6, t7;
      load_u8_8x8(src_ptr, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);
      transpose_u8_8x8(&t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);

      int16x8_t s0 = vreinterpretq_s16_u16(vmovl_u8(t0));
      int16x8_t s1 = vreinterpretq_s16_u16(vmovl_u8(t1));
      int16x8_t s2 = vreinterpretq_s16_u16(vmovl_u8(t2));
      int16x8_t s3 = vreinterpretq_s16_u16(vmovl_u8(t3));
      int16x8_t s4 = vreinterpretq_s16_u16(vmovl_u8(t4));
      int16x8_t s5 = vreinterpretq_s16_u16(vmovl_u8(t5));
      int16x8_t s6 = vreinterpretq_s16_u16(vmovl_u8(t6));

      __builtin_prefetch(dst_ptr + 0 * dst_stride);
      __builtin_prefetch(dst_ptr + 1 * dst_stride);
      __builtin_prefetch(dst_ptr + 2 * dst_stride);
      __builtin_prefetch(dst_ptr + 3 * dst_stride);
      __builtin_prefetch(dst_ptr + 4 * dst_stride);
      __builtin_prefetch(dst_ptr + 5 * dst_stride);
      __builtin_prefetch(dst_ptr + 6 * dst_stride);
      __builtin_prefetch(dst_ptr + 7 * dst_stride);

      s = src_ptr + 7;

      do {
        load_u8_8x8(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);
        transpose_u8_8x8(&t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);

        int16x8_t s7 = vreinterpretq_s16_u16(vmovl_u8(t0));
        int16x8_t s8 = vreinterpretq_s16_u16(vmovl_u8(t1));
        int16x8_t s9 = vreinterpretq_s16_u16(vmovl_u8(t2));
        int16x8_t s10 = vreinterpretq_s16_u16(vmovl_u8(t3));
        int16x8_t s11 = vreinterpretq_s16_u16(vmovl_u8(t4));
        int16x8_t s12 = vreinterpretq_s16_u16(vmovl_u8(t5));
        int16x8_t s13 = vreinterpretq_s16_u16(vmovl_u8(t6));
        int16x8_t s14 = vreinterpretq_s16_u16(vmovl_u8(t7));

        uint16x8_t d0 = convolve8_8_x(s0, s1, s2, s3, s4, s5, s6, s7, x_filter,
                                      round_offset_vec);
        uint16x8_t d1 = convolve8_8_x(s1, s2, s3, s4, s5, s6, s7, s8, x_filter,
                                      round_offset_vec);
        uint16x8_t d2 = convolve8_8_x(s2, s3, s4, s5, s6, s7, s8, s9, x_filter,
                                      round_offset_vec);
        uint16x8_t d3 = convolve8_8_x(s3, s4, s5, s6, s7, s8, s9, s10, x_filter,
                                      round_offset_vec);
        uint16x8_t d4 = convolve8_8_x(s4, s5, s6, s7, s8, s9, s10, s11,
                                      x_filter, round_offset_vec);
        uint16x8_t d5 = convolve8_8_x(s5, s6, s7, s8, s9, s10, s11, s12,
                                      x_filter, round_offset_vec);
        uint16x8_t d6 = convolve8_8_x(s6, s7, s8, s9, s10, s11, s12, s13,
                                      x_filter, round_offset_vec);
        uint16x8_t d7 = convolve8_8_x(s7, s8, s9, s10, s11, s12, s13, s14,
                                      x_filter, round_offset_vec);

        transpose_u16_8x8(&d0, &d1, &d2, &d3, &d4, &d5, &d6, &d7);

        uint16x8_t dd0, dd1, dd2, dd3;
        load_u16_8x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

        uint8x8_t d0_u8, d1_u8, d2_u8, d3_u8;
        compute_dist_wtd_avg_8x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3, fwd_offset,
                                 bck_offset, round_offset_vec, &d0_u8, &d1_u8,
                                 &d2_u8, &d3_u8);

        store_u8_8x4(d_u8, dst8_stride, d0_u8, d1_u8, d2_u8, d3_u8);

        uint16x8_t dd4, dd5, dd6, dd7;
        load_u16_8x4(d + 4 * dst_stride, dst_stride, &dd4, &dd5, &dd6, &dd7);

        uint8x8_t d4_u8, d5_u8, d6_u8, d7_u8;
        compute_dist_wtd_avg_8x4(dd4, dd5, dd6, dd7, d4, d5, d6, d7, fwd_offset,
                                 bck_offset, round_offset_vec, &d4_u8, &d5_u8,
                                 &d6_u8, &d7_u8);

        store_u8_8x4(d_u8 + 4 * dst8_stride, dst8_stride, d4_u8, d5_u8, d6_u8,
                     d7_u8);

        s0 = s8;
        s1 = s9;
        s2 = s10;
        s3 = s11;
        s4 = s12;
        s5 = s13;
        s6 = s14;
        s += 8;
        d += 8;
        d_u8 += 8;
        width -= 8;
      } while (width != 0);
      src_ptr += 8 * src_stride;
      dst_ptr += 8 * dst_stride;
      dst8_ptr += 8 * dst8_stride;
      height -= 8;
#else   // !AOM_ARCH_AARCH64
      __builtin_prefetch(src_ptr);

      uint8x8_t t0 = vld1_u8(src_ptr);
      int16x8_t s0 =
          vreinterpretq_s16_u16(vmovl_u8(t0));  // a0 a1 a2 a3 a4 a5 a6 a7

      __builtin_prefetch(dst_ptr);

      s = src_ptr + 8;

      do {
        t0 = vld1_u8(s);  // a8 a9 a10 a11 a12 a13 a14 a15
        int16x8_t s8 = vreinterpretq_s16_u16(vmovl_u8(t0));

        int16x8_t s1 = vextq_s16(s0, s8, 1);  // a1 a2 a3 a4 a5 a6 a7 a8
        int16x8_t s2 = vextq_s16(s0, s8, 2);  // a2 a3 a4 a5 a6 a7 a8 a9
        int16x8_t s3 = vextq_s16(s0, s8, 3);  // a3 a4 a5 a6 a7 a8 a9 a10
        int16x8_t s4 = vextq_s16(s0, s8, 4);  // a4 a5 a6 a7 a8 a9 a10 a11
        int16x8_t s5 = vextq_s16(s0, s8, 5);  // a5 a6 a7 a8 a9 a10 a11 a12
        int16x8_t s6 = vextq_s16(s0, s8, 6);  // a6 a7 a8 a9 a10 a11 a12 a13
        int16x8_t s7 = vextq_s16(s0, s8, 7);  // a7 a8 a9 a10 a11 a12 a13 a14

        uint16x8_t d0 = convolve8_8_x(s0, s1, s2, s3, s4, s5, s6, s7, x_filter,
                                      round_offset_vec);

        uint16x8_t dd0 = vld1q_u16(d);

        uint8x8_t d0_u8;
        compute_dist_wtd_avg_8x1(dd0, d0, fwd_offset, bck_offset,
                                 round_offset_vec, &d0_u8);

        vst1_u8(d_u8, d0_u8);

        s0 = s8;
        s += 8;
        d += 8;
        d_u8 += 8;
        width -= 8;
      } while (width != 0);
      src_ptr += src_stride;
      dst_ptr += dst_stride;
      dst8_ptr += dst8_stride;
      height--;
#endif  // AOM_ARCH_AARCH64
    } while (height != 0);
  }
}

static INLINE void dist_wtd_convolve_x_avg_neon(
    const uint8_t *src, int src_stride, uint8_t *dst8, int dst8_stride, int w,
    int h, const InterpFilterParams *filter_params_x, const int subpel_x_qn,
    ConvolveParams *conv_params) {
  assert(w % 4 == 0);
  assert(h % 4 == 0);

  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int16_t round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                               (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const int16x8_t round_offset_vec = vdupq_n_s16(round_offset);

  // Horizontal filter.
  const int16_t *x_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_x, subpel_x_qn & SUBPEL_MASK);
  // Filter values are even, so downshift by 1 to reduce intermediate precision
  // requirements.
  const int16x8_t x_filter = vshrq_n_s16(vld1q_s16(x_filter_ptr), 1);

  const int horiz_offset = filter_params_x->taps / 2 - 1;
  const uint8_t *src_ptr = src - horiz_offset;
  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  uint8_t *dst8_ptr = dst8;
  int dst_stride = conv_params->dst_stride;
  const uint8_t *s;
  uint8_t *d_u8;
  CONV_BUF_TYPE *d;
  int width;
  int height = h;

  if (w == 4 || h == 4) {
    do {
      d = dst_ptr;
      d_u8 = dst8_ptr;
      width = w;

      __builtin_prefetch(src_ptr + 0 * src_stride);
#if AOM_ARCH_AARCH64
      __builtin_prefetch(src_ptr + 1 * src_stride);
      __builtin_prefetch(src_ptr + 2 * src_stride);
      __builtin_prefetch(src_ptr + 3 * src_stride);

      uint8x8_t t0, t1, t2, t3;
      load_u8_8x4(src_ptr, src_stride, &t0, &t1, &t2, &t3);
      transpose_u8_8x4(&t0, &t1, &t2, &t3);

      int16x4_t s0 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
      int16x4_t s1 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t1)));
      int16x4_t s2 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t2)));
      int16x4_t s3 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t3)));
      int16x4_t s4 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
      int16x4_t s5 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t1)));
      int16x4_t s6 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t2)));

      __builtin_prefetch(d + 0 * dst_stride);
      __builtin_prefetch(d + 1 * dst_stride);
      __builtin_prefetch(d + 2 * dst_stride);
      __builtin_prefetch(d + 3 * dst_stride);

      s = src_ptr + 7;

      do {
        load_unaligned_u8_4x4(s, src_stride, &t0, &t1);
        transpose_u8_4x4(&t0, &t1);

        int16x4_t s7 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
        int16x4_t s8 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t1)));
        int16x4_t s9 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
        int16x4_t s10 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t1)));

        uint16x4_t d0 = convolve8_4_x(s0, s1, s2, s3, s4, s5, s6, s7, x_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d1 = convolve8_4_x(s1, s2, s3, s4, s5, s6, s7, s8, x_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d2 = convolve8_4_x(s2, s3, s4, s5, s6, s7, s8, s9, x_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d3 = convolve8_4_x(s3, s4, s5, s6, s7, s8, s9, s10, x_filter,
                                      vget_low_s16(round_offset_vec));

        transpose_u16_4x4d(&d0, &d1, &d2, &d3);

        __builtin_prefetch(d + 0 * dst_stride);
        __builtin_prefetch(d + 1 * dst_stride);
        __builtin_prefetch(d + 2 * dst_stride);
        __builtin_prefetch(d + 3 * dst_stride);

        __builtin_prefetch(d_u8 + 0 * dst8_stride);
        __builtin_prefetch(d_u8 + 1 * dst8_stride);
        __builtin_prefetch(d_u8 + 2 * dst8_stride);
        __builtin_prefetch(d_u8 + 3 * dst8_stride);

        uint16x4_t dd0, dd1, dd2, dd3;
        load_u16_4x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

        uint8x8_t d01, d23;
        compute_basic_avg_4x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3,
                              round_offset_vec, &d01, &d23);

        store_u8_4x1(d_u8 + 0 * dst8_stride, d01, 0);
        store_u8_4x1(d_u8 + 1 * dst8_stride, d01, 1);
        store_u8_4x1(d_u8 + 2 * dst8_stride, d23, 0);
        store_u8_4x1(d_u8 + 3 * dst8_stride, d23, 1);

        s0 = s4;
        s1 = s5;
        s2 = s6;
        s3 = s7;
        s4 = s8;
        s5 = s9;
        s6 = s10;
        s += 4;
        d += 4;
        d_u8 += 4;
        width -= 4;
      } while (width != 0);
      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      dst8_ptr += 4 * dst8_stride;
      height -= 4;
#else   // !AOM_ARCH_AARCH64
      uint8x8_t t0 = vld1_u8(src_ptr);  // a0 a1 a2 a3 a4 a5 a6 a7
      int16x4_t s0 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
      int16x4_t s4 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));

      __builtin_prefetch(d);

      s = src_ptr + 8;

      do {
        t0 = vld1_u8(s);  // a8 a9 a10 a11
        int16x4_t s8 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));

        int16x4_t s1 = vext_s16(s0, s4, 1);  // a1 a2 a3 a4
        int16x4_t s2 = vext_s16(s0, s4, 2);  // a2 a3 a4 a5
        int16x4_t s3 = vext_s16(s0, s4, 3);  // a3 a4 a5 a6
        int16x4_t s5 = vext_s16(s4, s8, 1);  // a5 a6 a7 a8
        int16x4_t s6 = vext_s16(s4, s8, 2);  // a6 a7 a8 a9
        int16x4_t s7 = vext_s16(s4, s8, 3);  // a7 a8 a9 a10

        uint16x4_t d0 = convolve8_4_x(s0, s1, s2, s3, s4, s5, s6, s7, x_filter,
                                      vget_low_s16(round_offset_vec));

        __builtin_prefetch(d);
        __builtin_prefetch(d_u8);

        uint16x4_t dd0 = vld1_u16(d);

        uint8x8_t d01;
        compute_basic_avg_4x1(dd0, d0, vget_low_s16(round_offset_vec), &d01);

        store_u8_4x1(d_u8, d01, 0);

        s0 = s4;
        s4 = s8;
        s += 4;
        d += 4;
        d_u8 += 4;
        width -= 4;
      } while (width != 0);
      src_ptr += src_stride;
      dst_ptr += dst_stride;
      dst8_ptr += dst8_stride;
      height--;
#endif  // AOM_ARCH_AARCH64
    } while (height != 0);
  } else {
    do {
      d = dst_ptr;
      d_u8 = dst8_ptr;
      width = w;

#if AOM_ARCH_AARCH64
      __builtin_prefetch(src_ptr + 0 * src_stride);
      __builtin_prefetch(src_ptr + 1 * src_stride);
      __builtin_prefetch(src_ptr + 2 * src_stride);
      __builtin_prefetch(src_ptr + 3 * src_stride);
      __builtin_prefetch(src_ptr + 4 * src_stride);
      __builtin_prefetch(src_ptr + 5 * src_stride);
      __builtin_prefetch(src_ptr + 6 * src_stride);
      __builtin_prefetch(src_ptr + 7 * src_stride);

      uint8x8_t t0, t1, t2, t3, t4, t5, t6, t7;
      load_u8_8x8(src_ptr, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);
      transpose_u8_8x8(&t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);

      int16x8_t s0 = vreinterpretq_s16_u16(vmovl_u8(t0));
      int16x8_t s1 = vreinterpretq_s16_u16(vmovl_u8(t1));
      int16x8_t s2 = vreinterpretq_s16_u16(vmovl_u8(t2));
      int16x8_t s3 = vreinterpretq_s16_u16(vmovl_u8(t3));
      int16x8_t s4 = vreinterpretq_s16_u16(vmovl_u8(t4));
      int16x8_t s5 = vreinterpretq_s16_u16(vmovl_u8(t5));
      int16x8_t s6 = vreinterpretq_s16_u16(vmovl_u8(t6));

      __builtin_prefetch(dst_ptr + 0 * dst_stride);
      __builtin_prefetch(dst_ptr + 1 * dst_stride);
      __builtin_prefetch(dst_ptr + 2 * dst_stride);
      __builtin_prefetch(dst_ptr + 3 * dst_stride);
      __builtin_prefetch(dst_ptr + 4 * dst_stride);
      __builtin_prefetch(dst_ptr + 5 * dst_stride);
      __builtin_prefetch(dst_ptr + 6 * dst_stride);
      __builtin_prefetch(dst_ptr + 7 * dst_stride);

      s = src_ptr + 7;

      do {
        load_u8_8x8(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);
        transpose_u8_8x8(&t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);

        int16x8_t s7 = vreinterpretq_s16_u16(vmovl_u8(t0));
        int16x8_t s8 = vreinterpretq_s16_u16(vmovl_u8(t1));
        int16x8_t s9 = vreinterpretq_s16_u16(vmovl_u8(t2));
        int16x8_t s10 = vreinterpretq_s16_u16(vmovl_u8(t3));
        int16x8_t s11 = vreinterpretq_s16_u16(vmovl_u8(t4));
        int16x8_t s12 = vreinterpretq_s16_u16(vmovl_u8(t5));
        int16x8_t s13 = vreinterpretq_s16_u16(vmovl_u8(t6));
        int16x8_t s14 = vreinterpretq_s16_u16(vmovl_u8(t7));

        uint16x8_t d0 = convolve8_8_x(s0, s1, s2, s3, s4, s5, s6, s7, x_filter,
                                      round_offset_vec);
        uint16x8_t d1 = convolve8_8_x(s1, s2, s3, s4, s5, s6, s7, s8, x_filter,
                                      round_offset_vec);
        uint16x8_t d2 = convolve8_8_x(s2, s3, s4, s5, s6, s7, s8, s9, x_filter,
                                      round_offset_vec);
        uint16x8_t d3 = convolve8_8_x(s3, s4, s5, s6, s7, s8, s9, s10, x_filter,
                                      round_offset_vec);
        uint16x8_t d4 = convolve8_8_x(s4, s5, s6, s7, s8, s9, s10, s11,
                                      x_filter, round_offset_vec);
        uint16x8_t d5 = convolve8_8_x(s5, s6, s7, s8, s9, s10, s11, s12,
                                      x_filter, round_offset_vec);
        uint16x8_t d6 = convolve8_8_x(s6, s7, s8, s9, s10, s11, s12, s13,
                                      x_filter, round_offset_vec);
        uint16x8_t d7 = convolve8_8_x(s7, s8, s9, s10, s11, s12, s13, s14,
                                      x_filter, round_offset_vec);

        transpose_u16_8x8(&d0, &d1, &d2, &d3, &d4, &d5, &d6, &d7);

        uint16x8_t dd0, dd1, dd2, dd3;
        load_u16_8x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

        uint8x8_t d0_u8, d1_u8, d2_u8, d3_u8;
        compute_basic_avg_8x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3,
                              round_offset_vec, &d0_u8, &d1_u8, &d2_u8, &d3_u8);

        store_u8_8x4(d_u8, dst8_stride, d0_u8, d1_u8, d2_u8, d3_u8);

        uint16x8_t dd4, dd5, dd6, dd7;
        load_u16_8x4(d + 4 * dst_stride, dst_stride, &dd4, &dd5, &dd6, &dd7);

        uint8x8_t d4_u8, d5_u8, d6_u8, d7_u8;
        compute_basic_avg_8x4(dd4, dd5, dd6, dd7, d4, d5, d6, d7,
                              round_offset_vec, &d4_u8, &d5_u8, &d6_u8, &d7_u8);

        store_u8_8x4(d_u8 + 4 * dst8_stride, dst8_stride, d4_u8, d5_u8, d6_u8,
                     d7_u8);

        s0 = s8;
        s1 = s9;
        s2 = s10;
        s3 = s11;
        s4 = s12;
        s5 = s13;
        s6 = s14;
        s += 8;
        d += 8;
        d_u8 += 8;
        width -= 8;
      } while (width != 0);
      src_ptr += 8 * src_stride;
      dst_ptr += 8 * dst_stride;
      dst8_ptr += 8 * dst8_stride;
      height -= 8;
#else   // !AOM_ARCH_AARCH64
      __builtin_prefetch(src_ptr);

      uint8x8_t t0 = vld1_u8(src_ptr);
      int16x8_t s0 =
          vreinterpretq_s16_u16(vmovl_u8(t0));  // a0 a1 a2 a3 a4 a5 a6 a7

      __builtin_prefetch(dst_ptr);

      s = src_ptr + 8;

      do {
        t0 = vld1_u8(s);  // a8 a9 a10 a11 a12 a13 a14 a15
        int16x8_t s8 = vreinterpretq_s16_u16(vmovl_u8(t0));

        int16x8_t s1 = vextq_s16(s0, s8, 1);  // a1 a2 a3 a4 a5 a6 a7 a8
        int16x8_t s2 = vextq_s16(s0, s8, 2);  // a2 a3 a4 a5 a6 a7 a8 a9
        int16x8_t s3 = vextq_s16(s0, s8, 3);  // a3 a4 a5 a6 a7 a8 a9 a10
        int16x8_t s4 = vextq_s16(s0, s8, 4);  // a4 a5 a6 a7 a8 a9 a10 a11
        int16x8_t s5 = vextq_s16(s0, s8, 5);  // a5 a6 a7 a8 a9 a10 a11 a12
        int16x8_t s6 = vextq_s16(s0, s8, 6);  // a6 a7 a8 a9 a10 a11 a12 a13
        int16x8_t s7 = vextq_s16(s0, s8, 7);  // a7 a8 a9 a10 a11 a12 a13 a14

        uint16x8_t d0 = convolve8_8_x(s0, s1, s2, s3, s4, s5, s6, s7, x_filter,
                                      round_offset_vec);

        uint16x8_t dd0 = vld1q_u16(d);

        uint8x8_t d0_u8;
        compute_basic_avg_8x1(dd0, d0, round_offset_vec, &d0_u8);

        vst1_u8(d_u8, d0_u8);

        s0 = s8;
        s += 8;
        d += 8;
        d_u8 += 8;
        width -= 8;
      } while (width != 0);
      src_ptr += src_stride;
      dst_ptr += dst_stride;
      dst8_ptr += dst8_stride;
      height--;
#endif  // AOM_ARCH_AARCH64
    } while (height != 0);
  }
}

static INLINE void dist_wtd_convolve_x_neon(
    const uint8_t *src, int src_stride, int w, int h,
    const InterpFilterParams *filter_params_x, const int subpel_x_qn,
    ConvolveParams *conv_params) {
  assert(w % 4 == 0);
  assert(h % 4 == 0);

  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int16_t round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                               (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const int16x8_t round_offset_vec = vdupq_n_s16(round_offset);

  // Horizontal filter.
  const int16_t *x_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_x, subpel_x_qn & SUBPEL_MASK);
  // Filter values are even, so downshift by 1 to reduce intermediate precision
  // requirements.
  const int16x8_t x_filter = vshrq_n_s16(vld1q_s16(x_filter_ptr), 1);

  const int horiz_offset = filter_params_x->taps / 2 - 1;
  const uint8_t *src_ptr = src - horiz_offset;
  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  int dst_stride = conv_params->dst_stride;
  const uint8_t *s;
  CONV_BUF_TYPE *d;
  int width;
  int height = h;

  if (w == 4 || h == 4) {
    do {
      d = dst_ptr;
      width = w;

      __builtin_prefetch(src_ptr + 0 * src_stride);
#if AOM_ARCH_AARCH64
      __builtin_prefetch(src_ptr + 1 * src_stride);
      __builtin_prefetch(src_ptr + 2 * src_stride);
      __builtin_prefetch(src_ptr + 3 * src_stride);

      uint8x8_t t0, t1, t2, t3;
      load_u8_8x4(src_ptr, src_stride, &t0, &t1, &t2, &t3);
      transpose_u8_8x4(&t0, &t1, &t2, &t3);

      int16x4_t s0 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
      int16x4_t s1 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t1)));
      int16x4_t s2 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t2)));
      int16x4_t s3 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t3)));
      int16x4_t s4 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
      int16x4_t s5 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t1)));
      int16x4_t s6 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t2)));

      __builtin_prefetch(d + 0 * dst_stride);
      __builtin_prefetch(d + 1 * dst_stride);
      __builtin_prefetch(d + 2 * dst_stride);
      __builtin_prefetch(d + 3 * dst_stride);

      s = src_ptr + 7;

      do {
        load_unaligned_u8_4x4(s, src_stride, &t0, &t1);
        transpose_u8_4x4(&t0, &t1);

        int16x4_t s7 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
        int16x4_t s8 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t1)));
        int16x4_t s9 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
        int16x4_t s10 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t1)));

        uint16x4_t d0 = convolve8_4_x(s0, s1, s2, s3, s4, s5, s6, s7, x_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d1 = convolve8_4_x(s1, s2, s3, s4, s5, s6, s7, s8, x_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d2 = convolve8_4_x(s2, s3, s4, s5, s6, s7, s8, s9, x_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d3 = convolve8_4_x(s3, s4, s5, s6, s7, s8, s9, s10, x_filter,
                                      vget_low_s16(round_offset_vec));

        transpose_u16_4x4d(&d0, &d1, &d2, &d3);

        store_u16_4x4(d, dst_stride, d0, d1, d2, d3);

        s0 = s4;
        s1 = s5;
        s2 = s6;
        s3 = s7;
        s4 = s8;
        s5 = s9;
        s6 = s10;
        s += 4;
        d += 4;
        width -= 4;
      } while (width != 0);
      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      height -= 4;
#else   // !AOM_ARCH_AARCH64
      uint8x8_t t0 = vld1_u8(src_ptr);  // a0 a1 a2 a3 a4 a5 a6 a7
      int16x4_t s0 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
      int16x4_t s4 = vget_high_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));

      __builtin_prefetch(d);

      s = src_ptr + 8;

      do {
        t0 = vld1_u8(s);  // a8 a9 a10 a11
        int16x4_t s8 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));

        int16x4_t s1 = vext_s16(s0, s4, 1);  // a1 a2 a3 a4
        int16x4_t s2 = vext_s16(s0, s4, 2);  // a2 a3 a4 a5
        int16x4_t s3 = vext_s16(s0, s4, 3);  // a3 a4 a5 a6
        int16x4_t s5 = vext_s16(s4, s8, 1);  // a5 a6 a7 a8
        int16x4_t s6 = vext_s16(s4, s8, 2);  // a6 a7 a8 a9
        int16x4_t s7 = vext_s16(s4, s8, 3);  // a7 a8 a9 a10

        uint16x4_t d0 = convolve8_4_x(s0, s1, s2, s3, s4, s5, s6, s7, x_filter,
                                      vget_low_s16(round_offset_vec));

        vst1_u16(d, d0);

        s0 = s4;
        s4 = s8;
        s += 4;
        d += 4;
        width -= 4;
      } while (width != 0);
      src_ptr += src_stride;
      dst_ptr += dst_stride;
      height--;
#endif  // AOM_ARCH_AARCH64
    } while (height != 0);
  } else {
    do {
      d = dst_ptr;
      width = w;

#if AOM_ARCH_AARCH64
      __builtin_prefetch(src_ptr + 0 * src_stride);
      __builtin_prefetch(src_ptr + 1 * src_stride);
      __builtin_prefetch(src_ptr + 2 * src_stride);
      __builtin_prefetch(src_ptr + 3 * src_stride);
      __builtin_prefetch(src_ptr + 4 * src_stride);
      __builtin_prefetch(src_ptr + 5 * src_stride);
      __builtin_prefetch(src_ptr + 6 * src_stride);
      __builtin_prefetch(src_ptr + 7 * src_stride);

      uint8x8_t t0, t1, t2, t3, t4, t5, t6, t7;
      load_u8_8x8(src_ptr, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);
      transpose_u8_8x8(&t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);

      int16x8_t s0 = vreinterpretq_s16_u16(vmovl_u8(t0));
      int16x8_t s1 = vreinterpretq_s16_u16(vmovl_u8(t1));
      int16x8_t s2 = vreinterpretq_s16_u16(vmovl_u8(t2));
      int16x8_t s3 = vreinterpretq_s16_u16(vmovl_u8(t3));
      int16x8_t s4 = vreinterpretq_s16_u16(vmovl_u8(t4));
      int16x8_t s5 = vreinterpretq_s16_u16(vmovl_u8(t5));
      int16x8_t s6 = vreinterpretq_s16_u16(vmovl_u8(t6));

      __builtin_prefetch(dst_ptr + 0 * dst_stride);
      __builtin_prefetch(dst_ptr + 1 * dst_stride);
      __builtin_prefetch(dst_ptr + 2 * dst_stride);
      __builtin_prefetch(dst_ptr + 3 * dst_stride);
      __builtin_prefetch(dst_ptr + 4 * dst_stride);
      __builtin_prefetch(dst_ptr + 5 * dst_stride);
      __builtin_prefetch(dst_ptr + 6 * dst_stride);
      __builtin_prefetch(dst_ptr + 7 * dst_stride);

      s = src_ptr + 7;

      do {
        load_u8_8x8(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);
        transpose_u8_8x8(&t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);

        int16x8_t s7 = vreinterpretq_s16_u16(vmovl_u8(t0));
        int16x8_t s8 = vreinterpretq_s16_u16(vmovl_u8(t1));
        int16x8_t s9 = vreinterpretq_s16_u16(vmovl_u8(t2));
        int16x8_t s10 = vreinterpretq_s16_u16(vmovl_u8(t3));
        int16x8_t s11 = vreinterpretq_s16_u16(vmovl_u8(t4));
        int16x8_t s12 = vreinterpretq_s16_u16(vmovl_u8(t5));
        int16x8_t s13 = vreinterpretq_s16_u16(vmovl_u8(t6));
        int16x8_t s14 = vreinterpretq_s16_u16(vmovl_u8(t7));

        uint16x8_t d0 = convolve8_8_x(s0, s1, s2, s3, s4, s5, s6, s7, x_filter,
                                      round_offset_vec);
        uint16x8_t d1 = convolve8_8_x(s1, s2, s3, s4, s5, s6, s7, s8, x_filter,
                                      round_offset_vec);
        uint16x8_t d2 = convolve8_8_x(s2, s3, s4, s5, s6, s7, s8, s9, x_filter,
                                      round_offset_vec);
        uint16x8_t d3 = convolve8_8_x(s3, s4, s5, s6, s7, s8, s9, s10, x_filter,
                                      round_offset_vec);
        uint16x8_t d4 = convolve8_8_x(s4, s5, s6, s7, s8, s9, s10, s11,
                                      x_filter, round_offset_vec);
        uint16x8_t d5 = convolve8_8_x(s5, s6, s7, s8, s9, s10, s11, s12,
                                      x_filter, round_offset_vec);
        uint16x8_t d6 = convolve8_8_x(s6, s7, s8, s9, s10, s11, s12, s13,
                                      x_filter, round_offset_vec);
        uint16x8_t d7 = convolve8_8_x(s7, s8, s9, s10, s11, s12, s13, s14,
                                      x_filter, round_offset_vec);

        transpose_u16_8x8(&d0, &d1, &d2, &d3, &d4, &d5, &d6, &d7);

        store_u16_8x8(d, dst_stride, d0, d1, d2, d3, d4, d5, d6, d7);

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
      src_ptr += 8 * src_stride;
      dst_ptr += 8 * dst_stride;
      height -= 8;
#else   // !AOM_ARCH_AARCH64
      __builtin_prefetch(src_ptr);

      uint8x8_t t0 = vld1_u8(src_ptr);
      int16x8_t s0 =
          vreinterpretq_s16_u16(vmovl_u8(t0));  // a0 a1 a2 a3 a4 a5 a6 a7

      __builtin_prefetch(dst_ptr);

      s = src_ptr + 8;

      do {
        t0 = vld1_u8(s);  // a8 a9 a10 a11 a12 a13 a14 a15
        int16x8_t s8 = vreinterpretq_s16_u16(vmovl_u8(t0));

        int16x8_t s1 = vextq_s16(s0, s8, 1);  // a1 a2 a3 a4 a5 a6 a7 a8
        int16x8_t s2 = vextq_s16(s0, s8, 2);  // a2 a3 a4 a5 a6 a7 a8 a9
        int16x8_t s3 = vextq_s16(s0, s8, 3);  // a3 a4 a5 a6 a7 a8 a9 a10
        int16x8_t s4 = vextq_s16(s0, s8, 4);  // a4 a5 a6 a7 a8 a9 a10 a11
        int16x8_t s5 = vextq_s16(s0, s8, 5);  // a5 a6 a7 a8 a9 a10 a11 a12
        int16x8_t s6 = vextq_s16(s0, s8, 6);  // a6 a7 a8 a9 a10 a11 a12 a13
        int16x8_t s7 = vextq_s16(s0, s8, 7);  // a7 a8 a9 a10 a11 a12 a13 a14

        uint16x8_t d0 = convolve8_8_x(s0, s1, s2, s3, s4, s5, s6, s7, x_filter,
                                      round_offset_vec);

        vst1q_u16(d, d0);

        s0 = s8;
        s += 8;
        d += 8;
        width -= 8;
      } while (width != 0);
      src_ptr += src_stride;
      dst_ptr += dst_stride;
      height--;
#endif  // AOM_ARCH_AARCH64
    } while (height != 0);
  }
}

#endif  // AOM_ARCH_AARCH64 && defined(__ARM_FEATURE_DOTPROD)

void av1_dist_wtd_convolve_x_neon(const uint8_t *src, int src_stride,
                                  uint8_t *dst8, int dst8_stride, int w, int h,
                                  const InterpFilterParams *filter_params_x,
                                  const int subpel_x_qn,
                                  ConvolveParams *conv_params) {
  if (conv_params->do_average) {
    if (UNLIKELY(conv_params->use_dist_wtd_comp_avg)) {
      dist_wtd_convolve_x_dist_wtd_avg_neon(src, src_stride, dst8, dst8_stride,
                                            w, h, filter_params_x, subpel_x_qn,
                                            conv_params);
    } else {
      dist_wtd_convolve_x_avg_neon(src, src_stride, dst8, dst8_stride, w, h,
                                   filter_params_x, subpel_x_qn, conv_params);
    }
  } else {
    dist_wtd_convolve_x_neon(src, src_stride, w, h, filter_params_x,
                             subpel_x_qn, conv_params);
  }
}

static INLINE uint16x4_t convolve6_4_y(const int16x4_t s0, const int16x4_t s1,
                                       const int16x4_t s2, const int16x4_t s3,
                                       const int16x4_t s4, const int16x4_t s5,
                                       const int16x8_t y_filter,
                                       const int16x4_t round_offset) {
  const int16x4_t y_filter_0_3 = vget_low_s16(y_filter);
  const int16x4_t y_filter_4_7 = vget_high_s16(y_filter);

  // Filter values at indices 0 and 7 are 0.
  int16x4_t sum = vmul_lane_s16(s0, y_filter_0_3, 1);
  sum = vmla_lane_s16(sum, s1, y_filter_0_3, 2);
  sum = vmla_lane_s16(sum, s2, y_filter_0_3, 3);
  sum = vmla_lane_s16(sum, s3, y_filter_4_7, 0);
  sum = vmla_lane_s16(sum, s4, y_filter_4_7, 1);
  sum = vmla_lane_s16(sum, s5, y_filter_4_7, 2);

  // We halved the convolution filter values so -1 from the right shift.
  int16x4_t res = vrsra_n_s16(round_offset, sum, ROUND0_BITS - 1);
  return vreinterpret_u16_s16(res);
}

static INLINE uint16x8_t convolve6_8_y(const int16x8_t s0, const int16x8_t s1,
                                       const int16x8_t s2, const int16x8_t s3,
                                       const int16x8_t s4, const int16x8_t s5,
                                       const int16x8_t y_filter,
                                       const int16x8_t round_offset) {
  const int16x4_t y_filter_0_3 = vget_low_s16(y_filter);
  const int16x4_t y_filter_4_7 = vget_high_s16(y_filter);

  // Filter values at indices 0 and 7 are 0.
  int16x8_t sum = vmulq_lane_s16(s0, y_filter_0_3, 1);
  sum = vmlaq_lane_s16(sum, s1, y_filter_0_3, 2);
  sum = vmlaq_lane_s16(sum, s2, y_filter_0_3, 3);
  sum = vmlaq_lane_s16(sum, s3, y_filter_4_7, 0);
  sum = vmlaq_lane_s16(sum, s4, y_filter_4_7, 1);
  sum = vmlaq_lane_s16(sum, s5, y_filter_4_7, 2);

  // We halved the convolution filter values so -1 from the right shift.
  int16x8_t res = vrsraq_n_s16(round_offset, sum, ROUND0_BITS - 1);
  return vreinterpretq_u16_s16(res);
}

static INLINE void dist_wtd_convolve_y_6tap_dist_wtd_avg_neon(
    const uint8_t *src_ptr, int src_stride, uint8_t *dst8_ptr,
    const int dst8_stride, int w, int h, const int16x8_t y_filter,
    ConvolveParams *conv_params) {
  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int16_t round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                               (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const int16x8_t round_offset_vec = vdupq_n_s16(round_offset);

  const uint16_t fwd_offset = conv_params->fwd_offset;
  const uint16_t bck_offset = conv_params->bck_offset;

  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  const int dst_stride = conv_params->dst_stride;
  int width = w;

  if (w == 4 || h == 4) {
    do {
      const uint8_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      uint8_t *d_u8 = dst8_ptr;
      int height = h;

      uint8x8_t t0 = load_unaligned_u8_4x1(s + 0 * src_stride);
      uint8x8_t t1 = load_unaligned_u8_4x1(s + 1 * src_stride);
      uint8x8_t t2 = load_unaligned_u8_4x1(s + 2 * src_stride);
      uint8x8_t t3 = load_unaligned_u8_4x1(s + 3 * src_stride);
      uint8x8_t t4 = load_unaligned_u8_4x1(s + 4 * src_stride);

      int16x4_t s0 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t0)));
      int16x4_t s1 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t1)));
      int16x4_t s2 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t2)));
      int16x4_t s3 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t3)));
      int16x4_t s4 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t4)));

      s += 5 * src_stride;

      do {
#if AOM_ARCH_AARCH64
        t0 = load_unaligned_u8_4x1(s + 0 * src_stride);
        t1 = load_unaligned_u8_4x1(s + 1 * src_stride);
        t2 = load_unaligned_u8_4x1(s + 2 * src_stride);
        t3 = load_unaligned_u8_4x1(s + 3 * src_stride);

        int16x4_t s5 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t0)));
        int16x4_t s6 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t1)));
        int16x4_t s7 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t2)));
        int16x4_t s8 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t3)));

        uint16x4_t d0 = convolve6_4_y(s0, s1, s2, s3, s4, s5, y_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d1 = convolve6_4_y(s1, s2, s3, s4, s5, s6, y_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d2 = convolve6_4_y(s2, s3, s4, s5, s6, s7, y_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d3 = convolve6_4_y(s3, s4, s5, s6, s7, s8, y_filter,
                                      vget_low_s16(round_offset_vec));

        uint16x4_t dd0, dd1, dd2, dd3;
        load_u16_4x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

        uint8x8_t d01, d23;
        compute_dist_wtd_avg_4x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3, fwd_offset,
                                 bck_offset, round_offset_vec, &d01, &d23);

        store_u8_4x1(d_u8 + 0 * dst8_stride, d01, 0);
        store_u8_4x1(d_u8 + 1 * dst8_stride, d01, 1);
        store_u8_4x1(d_u8 + 2 * dst8_stride, d23, 0);
        store_u8_4x1(d_u8 + 3 * dst8_stride, d23, 1);

        s0 = s4;
        s1 = s5;
        s2 = s6;
        s3 = s7;
        s4 = s8;
        s += 4 * src_stride;
        d += 4 * dst_stride;
        d_u8 += 4 * dst8_stride;
        height -= 4;
#else   // !AOM_ARCH_AARCH64
        t0 = load_unaligned_u8_4x1(s);
        int16x4_t s5 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t0)));

        uint16x4_t d0 = convolve6_4_y(s0, s1, s2, s3, s4, s5, y_filter,
                                      vget_low_s16(round_offset_vec));

        uint16x4_t dd0 = vld1_u16(d);

        uint8x8_t d01;
        compute_dist_wtd_avg_4x1(dd0, d0, fwd_offset, bck_offset,
                                 vget_low_s16(round_offset_vec), &d01);

        store_u8_4x1(d_u8, d01, 0);

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;
        s += src_stride;
        d += dst_stride;
        d_u8 += dst8_stride;
        height--;
#endif  // AOM_ARCH_AARCH64
      } while (height != 0);
      src_ptr += 4;
      dst_ptr += 4;
      dst8_ptr += 4;
      width -= 4;
    } while (width != 0);
  } else {
    do {
      const uint8_t *s = src_ptr + (5 * src_stride);
      CONV_BUF_TYPE *d = dst_ptr;
      uint8_t *d_u8 = dst8_ptr;
      int height = h;

      uint8x8_t t0, t1, t2, t3, t4;
      load_u8_8x5(src_ptr, src_stride, &t0, &t1, &t2, &t3, &t4);

      int16x8_t s0 = vreinterpretq_s16_u16(vmovl_u8(t0));
      int16x8_t s1 = vreinterpretq_s16_u16(vmovl_u8(t1));
      int16x8_t s2 = vreinterpretq_s16_u16(vmovl_u8(t2));
      int16x8_t s3 = vreinterpretq_s16_u16(vmovl_u8(t3));
      int16x8_t s4 = vreinterpretq_s16_u16(vmovl_u8(t4));

      do {
#if AOM_ARCH_AARCH64
        uint8x8_t t5, t6, t7;
        load_u8_8x8(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);

        int16x8_t s5 = vreinterpretq_s16_u16(vmovl_u8(t0));
        int16x8_t s6 = vreinterpretq_s16_u16(vmovl_u8(t1));
        int16x8_t s7 = vreinterpretq_s16_u16(vmovl_u8(t2));
        int16x8_t s8 = vreinterpretq_s16_u16(vmovl_u8(t3));
        int16x8_t s9 = vreinterpretq_s16_u16(vmovl_u8(t4));
        int16x8_t s10 = vreinterpretq_s16_u16(vmovl_u8(t5));
        int16x8_t s11 = vreinterpretq_s16_u16(vmovl_u8(t6));
        int16x8_t s12 = vreinterpretq_s16_u16(vmovl_u8(t7));

        uint16x8_t d0 =
            convolve6_8_y(s0, s1, s2, s3, s4, s5, y_filter, round_offset_vec);
        uint16x8_t d1 =
            convolve6_8_y(s1, s2, s3, s4, s5, s6, y_filter, round_offset_vec);
        uint16x8_t d2 =
            convolve6_8_y(s2, s3, s4, s5, s6, s7, y_filter, round_offset_vec);
        uint16x8_t d3 =
            convolve6_8_y(s3, s4, s5, s6, s7, s8, y_filter, round_offset_vec);
        uint16x8_t d4 =
            convolve6_8_y(s4, s5, s6, s7, s8, s9, y_filter, round_offset_vec);
        uint16x8_t d5 =
            convolve6_8_y(s5, s6, s7, s8, s9, s10, y_filter, round_offset_vec);
        uint16x8_t d6 =
            convolve6_8_y(s6, s7, s8, s9, s10, s11, y_filter, round_offset_vec);
        uint16x8_t d7 = convolve6_8_y(s7, s8, s9, s10, s11, s12, y_filter,
                                      round_offset_vec);

        uint16x8_t dd0, dd1, dd2, dd3;
        load_u16_8x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

        uint8x8_t d0_u8, d1_u8, d2_u8, d3_u8;
        compute_dist_wtd_avg_8x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3, fwd_offset,
                                 bck_offset, round_offset_vec, &d0_u8, &d1_u8,
                                 &d2_u8, &d3_u8);

        store_u8_8x4(d_u8, dst8_stride, d0_u8, d1_u8, d2_u8, d3_u8);
        d_u8 += 4 * dst8_stride;

        uint16x8_t dd4, dd5, dd6, dd7;
        load_u16_8x4(d + 4 * dst_stride, dst_stride, &dd4, &dd5, &dd6, &dd7);

        uint8x8_t d4_u8, d5_u8, d6_u8, d7_u8;
        compute_dist_wtd_avg_8x4(dd4, dd5, dd6, dd7, d4, d5, d6, d7, fwd_offset,
                                 bck_offset, round_offset_vec, &d4_u8, &d5_u8,
                                 &d6_u8, &d7_u8);

        store_u8_8x4(d_u8, dst8_stride, d4_u8, d5_u8, d6_u8, d7_u8);
        d_u8 += 4 * dst8_stride;

        s0 = s8;
        s1 = s9;
        s2 = s10;
        s3 = s11;
        s4 = s12;
        s += 8 * src_stride;
        d += 8 * dst_stride;
        height -= 8;
#else   // !AOM_ARCH_AARCH64
        int16x8_t s5 = vreinterpretq_s16_u16(vmovl_u8(vld1_u8(s)));

        uint16x8_t d0 =
            convolve6_8_y(s0, s1, s2, s3, s4, s5, y_filter, round_offset_vec);

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;

        uint16x8_t dd0 = vld1q_u16(d);

        uint8x8_t d0_u8;
        compute_dist_wtd_avg_8x1(dd0, d0, fwd_offset, bck_offset,
                                 round_offset_vec, &d0_u8);

        vst1_u8(d_u8, d0_u8);
        d_u8 += dst8_stride;

        s += src_stride;
        d += dst_stride;
        height--;
#endif  // AOM_ARCH_AARCH64
      } while (height != 0);
      src_ptr += 8;
      dst_ptr += 8;
      dst8_ptr += 8;
      width -= 8;
    } while (width != 0);
  }
}

static INLINE void dist_wtd_convolve_y_6tap_avg_neon(
    const uint8_t *src_ptr, int src_stride, uint8_t *dst8_ptr,
    const int dst8_stride, int w, int h, const int16x8_t y_filter,
    ConvolveParams *conv_params) {
  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int16_t round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                               (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const int16x8_t round_offset_vec = vdupq_n_s16(round_offset);

  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  const int dst_stride = conv_params->dst_stride;
  int width = w;

  if (w == 4 || h == 4) {
    do {
      const uint8_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      uint8_t *d_u8 = dst8_ptr;
      int height = h;

      uint8x8_t t0 = load_unaligned_u8_4x1(s + 0 * src_stride);
      uint8x8_t t1 = load_unaligned_u8_4x1(s + 1 * src_stride);
      uint8x8_t t2 = load_unaligned_u8_4x1(s + 2 * src_stride);
      uint8x8_t t3 = load_unaligned_u8_4x1(s + 3 * src_stride);
      uint8x8_t t4 = load_unaligned_u8_4x1(s + 4 * src_stride);

      int16x4_t s0 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t0)));
      int16x4_t s1 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t1)));
      int16x4_t s2 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t2)));
      int16x4_t s3 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t3)));
      int16x4_t s4 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t4)));

      s += 5 * src_stride;

      do {
#if AOM_ARCH_AARCH64
        t0 = load_unaligned_u8_4x1(s + 0 * src_stride);
        t1 = load_unaligned_u8_4x1(s + 1 * src_stride);
        t2 = load_unaligned_u8_4x1(s + 2 * src_stride);
        t3 = load_unaligned_u8_4x1(s + 3 * src_stride);

        int16x4_t s5 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t0)));
        int16x4_t s6 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t1)));
        int16x4_t s7 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t2)));
        int16x4_t s8 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t3)));

        uint16x4_t d0 = convolve6_4_y(s0, s1, s2, s3, s4, s5, y_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d1 = convolve6_4_y(s1, s2, s3, s4, s5, s6, y_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d2 = convolve6_4_y(s2, s3, s4, s5, s6, s7, y_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d3 = convolve6_4_y(s3, s4, s5, s6, s7, s8, y_filter,
                                      vget_low_s16(round_offset_vec));

        uint16x4_t dd0, dd1, dd2, dd3;
        load_u16_4x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

        uint8x8_t d01, d23;
        compute_basic_avg_4x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3,
                              round_offset_vec, &d01, &d23);

        store_u8_4x1(d_u8 + 0 * dst8_stride, d01, 0);
        store_u8_4x1(d_u8 + 1 * dst8_stride, d01, 1);
        store_u8_4x1(d_u8 + 2 * dst8_stride, d23, 0);
        store_u8_4x1(d_u8 + 3 * dst8_stride, d23, 1);

        s0 = s4;
        s1 = s5;
        s2 = s6;
        s3 = s7;
        s4 = s8;
        s += 4 * src_stride;
        d += 4 * dst_stride;
        d_u8 += 4 * dst8_stride;
        height -= 4;
#else   // !AOM_ARCH_AARCH64
        t0 = load_unaligned_u8_4x1(s);
        int16x4_t s5 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t0)));

        uint16x4_t d0 = convolve6_4_y(s0, s1, s2, s3, s4, s5, y_filter,
                                      vget_low_s16(round_offset_vec));

        uint16x4_t dd0 = vld1_u16(d);

        uint8x8_t d01;
        compute_basic_avg_4x1(dd0, d0, vget_low_s16(round_offset_vec), &d01);

        store_u8_4x1(d_u8, d01, 0);

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;
        s += src_stride;
        d += dst_stride;
        d_u8 += dst8_stride;
        height--;
#endif  // AOM_ARCH_AARCH64
      } while (height != 0);
      src_ptr += 4;
      dst_ptr += 4;
      dst8_ptr += 4;
      width -= 4;
    } while (width != 0);
  } else {
    do {
      const uint8_t *s = src_ptr + (5 * src_stride);
      CONV_BUF_TYPE *d = dst_ptr;
      uint8_t *d_u8 = dst8_ptr;
      int height = h;

      uint8x8_t t0, t1, t2, t3, t4;
      load_u8_8x5(src_ptr, src_stride, &t0, &t1, &t2, &t3, &t4);

      int16x8_t s0 = vreinterpretq_s16_u16(vmovl_u8(t0));
      int16x8_t s1 = vreinterpretq_s16_u16(vmovl_u8(t1));
      int16x8_t s2 = vreinterpretq_s16_u16(vmovl_u8(t2));
      int16x8_t s3 = vreinterpretq_s16_u16(vmovl_u8(t3));
      int16x8_t s4 = vreinterpretq_s16_u16(vmovl_u8(t4));

      do {
#if AOM_ARCH_AARCH64
        uint8x8_t t5, t6, t7;
        load_u8_8x8(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);

        int16x8_t s5 = vreinterpretq_s16_u16(vmovl_u8(t0));
        int16x8_t s6 = vreinterpretq_s16_u16(vmovl_u8(t1));
        int16x8_t s7 = vreinterpretq_s16_u16(vmovl_u8(t2));
        int16x8_t s8 = vreinterpretq_s16_u16(vmovl_u8(t3));
        int16x8_t s9 = vreinterpretq_s16_u16(vmovl_u8(t4));
        int16x8_t s10 = vreinterpretq_s16_u16(vmovl_u8(t5));
        int16x8_t s11 = vreinterpretq_s16_u16(vmovl_u8(t6));
        int16x8_t s12 = vreinterpretq_s16_u16(vmovl_u8(t7));

        uint16x8_t d0 =
            convolve6_8_y(s0, s1, s2, s3, s4, s5, y_filter, round_offset_vec);
        uint16x8_t d1 =
            convolve6_8_y(s1, s2, s3, s4, s5, s6, y_filter, round_offset_vec);
        uint16x8_t d2 =
            convolve6_8_y(s2, s3, s4, s5, s6, s7, y_filter, round_offset_vec);
        uint16x8_t d3 =
            convolve6_8_y(s3, s4, s5, s6, s7, s8, y_filter, round_offset_vec);
        uint16x8_t d4 =
            convolve6_8_y(s4, s5, s6, s7, s8, s9, y_filter, round_offset_vec);
        uint16x8_t d5 =
            convolve6_8_y(s5, s6, s7, s8, s9, s10, y_filter, round_offset_vec);
        uint16x8_t d6 =
            convolve6_8_y(s6, s7, s8, s9, s10, s11, y_filter, round_offset_vec);
        uint16x8_t d7 = convolve6_8_y(s7, s8, s9, s10, s11, s12, y_filter,
                                      round_offset_vec);

        uint16x8_t dd0, dd1, dd2, dd3;
        load_u16_8x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

        uint8x8_t d0_u8, d1_u8, d2_u8, d3_u8;
        compute_basic_avg_8x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3,
                              round_offset_vec, &d0_u8, &d1_u8, &d2_u8, &d3_u8);

        store_u8_8x4(d_u8, dst8_stride, d0_u8, d1_u8, d2_u8, d3_u8);
        d_u8 += 4 * dst8_stride;

        uint16x8_t dd4, dd5, dd6, dd7;
        load_u16_8x4(d + 4 * dst_stride, dst_stride, &dd4, &dd5, &dd6, &dd7);

        uint8x8_t d4_u8, d5_u8, d6_u8, d7_u8;
        compute_basic_avg_8x4(dd4, dd5, dd6, dd7, d4, d5, d6, d7,
                              round_offset_vec, &d4_u8, &d5_u8, &d6_u8, &d7_u8);

        store_u8_8x4(d_u8, dst8_stride, d4_u8, d5_u8, d6_u8, d7_u8);
        d_u8 += 4 * dst8_stride;

        s0 = s8;
        s1 = s9;
        s2 = s10;
        s3 = s11;
        s4 = s12;
        s += 8 * src_stride;
        d += 8 * dst_stride;
        height -= 8;
#else   // !AOM_ARCH_AARCH64
        int16x8_t s5 = vreinterpretq_s16_u16(vmovl_u8(vld1_u8(s)));

        uint16x8_t d0 =
            convolve6_8_y(s0, s1, s2, s3, s4, s5, y_filter, round_offset_vec);

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;

        uint16x8_t dd0 = vld1q_u16(d);

        uint8x8_t d0_u8;
        compute_basic_avg_8x1(dd0, d0, round_offset_vec, &d0_u8);

        vst1_u8(d_u8, d0_u8);
        d_u8 += dst8_stride;

        s += src_stride;
        d += dst_stride;
        height--;
#endif  // AOM_ARCH_AARCH64
      } while (height != 0);
      src_ptr += 8;
      dst_ptr += 8;
      dst8_ptr += 8;
      width -= 8;
    } while (width != 0);
  }
}

static INLINE void dist_wtd_convolve_y_6tap_neon(const uint8_t *src_ptr,
                                                 int src_stride, int w, int h,
                                                 const int16x8_t y_filter,
                                                 ConvolveParams *conv_params) {
  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int16_t round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                               (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const int16x8_t round_offset_vec = vdupq_n_s16(round_offset);

  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  const int dst_stride = conv_params->dst_stride;
  int width = w;

  if (w == 4 || h == 4) {
    do {
      const uint8_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      int height = h;

      uint8x8_t t0 = load_unaligned_u8_4x1(s + 0 * src_stride);
      uint8x8_t t1 = load_unaligned_u8_4x1(s + 1 * src_stride);
      uint8x8_t t2 = load_unaligned_u8_4x1(s + 2 * src_stride);
      uint8x8_t t3 = load_unaligned_u8_4x1(s + 3 * src_stride);
      uint8x8_t t4 = load_unaligned_u8_4x1(s + 4 * src_stride);

      int16x4_t s0 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t0)));
      int16x4_t s1 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t1)));
      int16x4_t s2 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t2)));
      int16x4_t s3 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t3)));
      int16x4_t s4 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t4)));

      s += 5 * src_stride;

      do {
#if AOM_ARCH_AARCH64
        t0 = load_unaligned_u8_4x1(s + 0 * src_stride);
        t1 = load_unaligned_u8_4x1(s + 1 * src_stride);
        t2 = load_unaligned_u8_4x1(s + 2 * src_stride);
        t3 = load_unaligned_u8_4x1(s + 3 * src_stride);

        int16x4_t s5 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t0)));
        int16x4_t s6 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t1)));
        int16x4_t s7 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t2)));
        int16x4_t s8 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t3)));

        uint16x4_t d0 = convolve6_4_y(s0, s1, s2, s3, s4, s5, y_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d1 = convolve6_4_y(s1, s2, s3, s4, s5, s6, y_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d2 = convolve6_4_y(s2, s3, s4, s5, s6, s7, y_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d3 = convolve6_4_y(s3, s4, s5, s6, s7, s8, y_filter,
                                      vget_low_s16(round_offset_vec));

        store_u16_4x4(d, dst_stride, d0, d1, d2, d3);

        s0 = s4;
        s1 = s5;
        s2 = s6;
        s3 = s7;
        s4 = s8;
        s += 4 * src_stride;
        d += 4 * dst_stride;
        height -= 4;
#else   // !AOM_ARCH_AARCH64
        t0 = load_unaligned_u8_4x1(s);
        int16x4_t s5 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t0)));

        uint16x4_t d0 = convolve6_4_y(s0, s1, s2, s3, s4, s5, y_filter,
                                      vget_low_s16(round_offset_vec));

        vst1_u16(d, d0);

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;
        s += src_stride;
        d += dst_stride;
        height--;
#endif  // AOM_ARCH_AARCH64
      } while (height != 0);
      src_ptr += 4;
      dst_ptr += 4;
      width -= 4;
    } while (width != 0);
  } else {
    do {
      const uint8_t *s = src_ptr + (5 * src_stride);
      CONV_BUF_TYPE *d = dst_ptr;
      int height = h;

      uint8x8_t t0, t1, t2, t3, t4;
      load_u8_8x5(src_ptr, src_stride, &t0, &t1, &t2, &t3, &t4);

      int16x8_t s0 = vreinterpretq_s16_u16(vmovl_u8(t0));
      int16x8_t s1 = vreinterpretq_s16_u16(vmovl_u8(t1));
      int16x8_t s2 = vreinterpretq_s16_u16(vmovl_u8(t2));
      int16x8_t s3 = vreinterpretq_s16_u16(vmovl_u8(t3));
      int16x8_t s4 = vreinterpretq_s16_u16(vmovl_u8(t4));

      do {
#if AOM_ARCH_AARCH64
        uint8x8_t t5, t6, t7;
        load_u8_8x8(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);

        int16x8_t s5 = vreinterpretq_s16_u16(vmovl_u8(t0));
        int16x8_t s6 = vreinterpretq_s16_u16(vmovl_u8(t1));
        int16x8_t s7 = vreinterpretq_s16_u16(vmovl_u8(t2));
        int16x8_t s8 = vreinterpretq_s16_u16(vmovl_u8(t3));
        int16x8_t s9 = vreinterpretq_s16_u16(vmovl_u8(t4));
        int16x8_t s10 = vreinterpretq_s16_u16(vmovl_u8(t5));
        int16x8_t s11 = vreinterpretq_s16_u16(vmovl_u8(t6));
        int16x8_t s12 = vreinterpretq_s16_u16(vmovl_u8(t7));

        uint16x8_t d0 =
            convolve6_8_y(s0, s1, s2, s3, s4, s5, y_filter, round_offset_vec);
        uint16x8_t d1 =
            convolve6_8_y(s1, s2, s3, s4, s5, s6, y_filter, round_offset_vec);
        uint16x8_t d2 =
            convolve6_8_y(s2, s3, s4, s5, s6, s7, y_filter, round_offset_vec);
        uint16x8_t d3 =
            convolve6_8_y(s3, s4, s5, s6, s7, s8, y_filter, round_offset_vec);
        uint16x8_t d4 =
            convolve6_8_y(s4, s5, s6, s7, s8, s9, y_filter, round_offset_vec);
        uint16x8_t d5 =
            convolve6_8_y(s5, s6, s7, s8, s9, s10, y_filter, round_offset_vec);
        uint16x8_t d6 =
            convolve6_8_y(s6, s7, s8, s9, s10, s11, y_filter, round_offset_vec);
        uint16x8_t d7 = convolve6_8_y(s7, s8, s9, s10, s11, s12, y_filter,
                                      round_offset_vec);

        store_u16_8x8(d, dst_stride, d0, d1, d2, d3, d4, d5, d6, d7);

        s0 = s8;
        s1 = s9;
        s2 = s10;
        s3 = s11;
        s4 = s12;
        s += 8 * src_stride;
        d += 8 * dst_stride;
        height -= 8;
#else   // !AOM_ARCH_AARCH64
        int16x8_t s5 = vreinterpretq_s16_u16(vmovl_u8(vld1_u8(s)));

        uint16x8_t d0 =
            convolve6_8_y(s0, s1, s2, s3, s4, s5, y_filter, round_offset_vec);

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;

        vst1q_u16(d, d0);

        s += src_stride;
        d += dst_stride;
        height--;
#endif  // AOM_ARCH_AARCH64
      } while (height != 0);
      src_ptr += 8;
      dst_ptr += 8;
      width -= 8;
    } while (width != 0);
  }
}

static INLINE uint16x4_t convolve8_4_y(const int16x4_t s0, const int16x4_t s1,
                                       const int16x4_t s2, const int16x4_t s3,
                                       const int16x4_t s4, const int16x4_t s5,
                                       const int16x4_t s6, const int16x4_t s7,
                                       const int16x8_t y_filter,
                                       const int16x4_t round_offset) {
  const int16x4_t y_filter_0_3 = vget_low_s16(y_filter);
  const int16x4_t y_filter_4_7 = vget_high_s16(y_filter);

  int16x4_t sum = vmul_lane_s16(s0, y_filter_0_3, 0);
  sum = vmla_lane_s16(sum, s1, y_filter_0_3, 1);
  sum = vmla_lane_s16(sum, s2, y_filter_0_3, 2);
  sum = vmla_lane_s16(sum, s3, y_filter_0_3, 3);
  sum = vmla_lane_s16(sum, s4, y_filter_4_7, 0);
  sum = vmla_lane_s16(sum, s5, y_filter_4_7, 1);
  sum = vmla_lane_s16(sum, s6, y_filter_4_7, 2);
  sum = vmla_lane_s16(sum, s7, y_filter_4_7, 3);

  // We halved the convolution filter values so -1 from the right shift.
  int16x4_t res = vrsra_n_s16(round_offset, sum, ROUND0_BITS - 1);
  return vreinterpret_u16_s16(res);
}

static INLINE uint16x8_t convolve8_8_y(const int16x8_t s0, const int16x8_t s1,
                                       const int16x8_t s2, const int16x8_t s3,
                                       const int16x8_t s4, const int16x8_t s5,
                                       const int16x8_t s6, const int16x8_t s7,
                                       const int16x8_t y_filter,
                                       const int16x8_t round_offset) {
  const int16x4_t y_filter_0_3 = vget_low_s16(y_filter);
  const int16x4_t y_filter_4_7 = vget_high_s16(y_filter);

  int16x8_t sum = vmulq_lane_s16(s0, y_filter_0_3, 0);
  sum = vmlaq_lane_s16(sum, s1, y_filter_0_3, 1);
  sum = vmlaq_lane_s16(sum, s2, y_filter_0_3, 2);
  sum = vmlaq_lane_s16(sum, s3, y_filter_0_3, 3);
  sum = vmlaq_lane_s16(sum, s4, y_filter_4_7, 0);
  sum = vmlaq_lane_s16(sum, s5, y_filter_4_7, 1);
  sum = vmlaq_lane_s16(sum, s6, y_filter_4_7, 2);
  sum = vmlaq_lane_s16(sum, s7, y_filter_4_7, 3);

  // We halved the convolution filter values so -1 from the right shift.
  int16x8_t res = vrsraq_n_s16(round_offset, sum, ROUND0_BITS - 1);
  return vreinterpretq_u16_s16(res);
}

static INLINE void dist_wtd_convolve_y_8tap_dist_wtd_avg_neon(
    const uint8_t *src_ptr, int src_stride, uint8_t *dst8_ptr,
    const int dst8_stride, int w, int h, const int16x8_t y_filter,
    ConvolveParams *conv_params) {
  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int16_t round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                               (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const int16x8_t round_offset_vec = vdupq_n_s16(round_offset);

  const uint16_t fwd_offset = conv_params->fwd_offset;
  const uint16_t bck_offset = conv_params->bck_offset;

  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  const int dst_stride = conv_params->dst_stride;
  int width = w;

  if (w == 4 || h == 4) {
    do {
      const uint8_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      uint8_t *d_u8 = dst8_ptr;
      int height = h;

      __builtin_prefetch(s + 0 * src_stride);
      __builtin_prefetch(s + 1 * src_stride);
      __builtin_prefetch(s + 2 * src_stride);
      __builtin_prefetch(s + 3 * src_stride);

      uint8x8_t t0 = load_unaligned_u8_4x1(s + 0 * src_stride);
      uint8x8_t t1 = load_unaligned_u8_4x1(s + 1 * src_stride);
      uint8x8_t t2 = load_unaligned_u8_4x1(s + 2 * src_stride);
      uint8x8_t t3 = load_unaligned_u8_4x1(s + 3 * src_stride);
      uint8x8_t t4 = load_unaligned_u8_4x1(s + 4 * src_stride);
      uint8x8_t t5 = load_unaligned_u8_4x1(s + 5 * src_stride);
      uint8x8_t t6 = load_unaligned_u8_4x1(s + 6 * src_stride);

      int16x4_t s0 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t0)));
      int16x4_t s1 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t1)));
      int16x4_t s2 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t2)));
      int16x4_t s3 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t3)));
      int16x4_t s4 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t4)));
      int16x4_t s5 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t5)));
      int16x4_t s6 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t6)));

      __builtin_prefetch(d + 0 * dst_stride);
      __builtin_prefetch(d + 1 * dst_stride);
      __builtin_prefetch(d + 2 * dst_stride);
      __builtin_prefetch(d + 3 * dst_stride);

      s += 7 * src_stride;

      do {
#if AOM_ARCH_AARCH64
        t0 = load_unaligned_u8_4x1(s + 0 * src_stride);
        t1 = load_unaligned_u8_4x1(s + 1 * src_stride);
        t2 = load_unaligned_u8_4x1(s + 2 * src_stride);
        t3 = load_unaligned_u8_4x1(s + 3 * src_stride);

        int16x4_t s7 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t0)));
        int16x4_t s8 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t1)));
        int16x4_t s9 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t2)));
        int16x4_t s10 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t3)));

        uint16x4_t d0 = convolve8_4_y(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d1 = convolve8_4_y(s1, s2, s3, s4, s5, s6, s7, s8, y_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d2 = convolve8_4_y(s2, s3, s4, s5, s6, s7, s8, s9, y_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d3 = convolve8_4_y(s3, s4, s5, s6, s7, s8, s9, s10, y_filter,
                                      vget_low_s16(round_offset_vec));

        __builtin_prefetch(d + 0 * dst_stride);
        __builtin_prefetch(d + 1 * dst_stride);
        __builtin_prefetch(d + 2 * dst_stride);
        __builtin_prefetch(d + 3 * dst_stride);

        __builtin_prefetch(d_u8 + 0 * dst8_stride);
        __builtin_prefetch(d_u8 + 1 * dst8_stride);
        __builtin_prefetch(d_u8 + 2 * dst8_stride);
        __builtin_prefetch(d_u8 + 3 * dst8_stride);

        uint16x4_t dd0, dd1, dd2, dd3;
        load_u16_4x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

        uint8x8_t d01, d23;
        compute_dist_wtd_avg_4x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3, fwd_offset,
                                 bck_offset, round_offset_vec, &d01, &d23);

        store_u8_4x1(d_u8 + 0 * dst8_stride, d01, 0);
        store_u8_4x1(d_u8 + 1 * dst8_stride, d01, 1);
        store_u8_4x1(d_u8 + 2 * dst8_stride, d23, 0);
        store_u8_4x1(d_u8 + 3 * dst8_stride, d23, 1);

        s0 = s4;
        s1 = s5;
        s2 = s6;
        s3 = s7;
        s4 = s8;
        s5 = s9;
        s6 = s10;
        s += 4 * src_stride;
        d += 4 * dst_stride;
        d_u8 += 4 * dst8_stride;
        height -= 4;
#else   // !AOM_ARCH_AARCH64
        t0 = load_unaligned_u8_4x1(s);
        int16x4_t s7 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t0)));

        uint16x4_t d0 = convolve8_4_y(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                      vget_low_s16(round_offset_vec));

        __builtin_prefetch(d);

        uint16x4_t dd0 = vld1_u16(d);

        uint8x8_t d01;
        compute_dist_wtd_avg_4x1(dd0, d0, fwd_offset, bck_offset,
                                 vget_low_s16(round_offset_vec), &d01);

        store_u8_4x1(d_u8, d01, 0);

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;
        s5 = s6;
        s6 = s7;
        s += src_stride;
        d += dst_stride;
        d_u8 += dst8_stride;
        height--;
#endif  // AOM_ARCH_AARCH64
      } while (height != 0);
      src_ptr += 4;
      dst_ptr += 4;
      dst8_ptr += 4;
      width -= 4;
    } while (width != 0);
  } else {
    do {
      const uint8_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      uint8_t *d_u8 = dst8_ptr;
      int height = h;

      __builtin_prefetch(s + 0 * src_stride);
      __builtin_prefetch(s + 1 * src_stride);
      __builtin_prefetch(s + 2 * src_stride);
      __builtin_prefetch(s + 3 * src_stride);
      __builtin_prefetch(s + 4 * src_stride);
      __builtin_prefetch(s + 5 * src_stride);
      __builtin_prefetch(s + 6 * src_stride);
      __builtin_prefetch(s + 7 * src_stride);

      uint8x8_t t0, t1, t2, t3, t4, t5, t6;
      load_u8_8x7(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6);

      int16x8_t s0 = vreinterpretq_s16_u16(vmovl_u8(t0));
      int16x8_t s1 = vreinterpretq_s16_u16(vmovl_u8(t1));
      int16x8_t s2 = vreinterpretq_s16_u16(vmovl_u8(t2));
      int16x8_t s3 = vreinterpretq_s16_u16(vmovl_u8(t3));
      int16x8_t s4 = vreinterpretq_s16_u16(vmovl_u8(t4));
      int16x8_t s5 = vreinterpretq_s16_u16(vmovl_u8(t5));
      int16x8_t s6 = vreinterpretq_s16_u16(vmovl_u8(t6));

      s += 7 * src_stride;

      do {
#if AOM_ARCH_AARCH64
        uint8x8_t t7;
        load_u8_8x8(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);

        int16x8_t s7 = vreinterpretq_s16_u16(vmovl_u8(t0));
        int16x8_t s8 = vreinterpretq_s16_u16(vmovl_u8(t1));
        int16x8_t s9 = vreinterpretq_s16_u16(vmovl_u8(t2));
        int16x8_t s10 = vreinterpretq_s16_u16(vmovl_u8(t3));
        int16x8_t s11 = vreinterpretq_s16_u16(vmovl_u8(t4));
        int16x8_t s12 = vreinterpretq_s16_u16(vmovl_u8(t5));
        int16x8_t s13 = vreinterpretq_s16_u16(vmovl_u8(t6));
        int16x8_t s14 = vreinterpretq_s16_u16(vmovl_u8(t7));

        __builtin_prefetch(dst_ptr + 0 * dst_stride);
        __builtin_prefetch(dst_ptr + 1 * dst_stride);
        __builtin_prefetch(dst_ptr + 2 * dst_stride);
        __builtin_prefetch(dst_ptr + 3 * dst_stride);

        uint16x8_t d0 = convolve8_8_y(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                      round_offset_vec);
        uint16x8_t d1 = convolve8_8_y(s1, s2, s3, s4, s5, s6, s7, s8, y_filter,
                                      round_offset_vec);
        uint16x8_t d2 = convolve8_8_y(s2, s3, s4, s5, s6, s7, s8, s9, y_filter,
                                      round_offset_vec);
        uint16x8_t d3 = convolve8_8_y(s3, s4, s5, s6, s7, s8, s9, s10, y_filter,
                                      round_offset_vec);
        uint16x8_t d4 = convolve8_8_y(s4, s5, s6, s7, s8, s9, s10, s11,
                                      y_filter, round_offset_vec);
        uint16x8_t d5 = convolve8_8_y(s5, s6, s7, s8, s9, s10, s11, s12,
                                      y_filter, round_offset_vec);
        uint16x8_t d6 = convolve8_8_y(s6, s7, s8, s9, s10, s11, s12, s13,
                                      y_filter, round_offset_vec);
        uint16x8_t d7 = convolve8_8_y(s7, s8, s9, s10, s11, s12, s13, s14,
                                      y_filter, round_offset_vec);

        __builtin_prefetch(d + 0 * dst8_stride);
        __builtin_prefetch(d + 1 * dst8_stride);
        __builtin_prefetch(d + 2 * dst8_stride);
        __builtin_prefetch(d + 3 * dst8_stride);

        uint16x8_t dd0, dd1, dd2, dd3;
        load_u16_8x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

        uint8x8_t d0_u8, d1_u8, d2_u8, d3_u8;
        compute_dist_wtd_avg_8x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3, fwd_offset,
                                 bck_offset, round_offset_vec, &d0_u8, &d1_u8,
                                 &d2_u8, &d3_u8);

        store_u8_8x4(d_u8, dst8_stride, d0_u8, d1_u8, d2_u8, d3_u8);
        d_u8 += 4 * dst8_stride;

        uint16x8_t dd4, dd5, dd6, dd7;
        load_u16_8x4(d + 4 * dst_stride, dst_stride, &dd4, &dd5, &dd6, &dd7);

        uint8x8_t d4_u8, d5_u8, d6_u8, d7_u8;
        compute_dist_wtd_avg_8x4(dd4, dd5, dd6, dd7, d4, d5, d6, d7, fwd_offset,
                                 bck_offset, round_offset_vec, &d4_u8, &d5_u8,
                                 &d6_u8, &d7_u8);

        store_u8_8x4(d_u8, dst8_stride, d4_u8, d5_u8, d6_u8, d7_u8);
        d_u8 += 4 * dst8_stride;

        s0 = s8;
        s1 = s9;
        s2 = s10;
        s3 = s11;
        s4 = s12;
        s5 = s13;
        s6 = s14;
        s += 8 * src_stride;
        d += 8 * dst_stride;
        height -= 8;
#else   // !AOM_ARCH_AARCH64
        int16x8_t s7 = vreinterpretq_s16_u16(vmovl_u8(vld1_u8(s)));

        __builtin_prefetch(dst_ptr);

        uint16x8_t d0 = convolve8_8_y(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                      round_offset_vec);

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;
        s5 = s6;
        s6 = s7;

        __builtin_prefetch(d);

        uint16x8_t dd0 = vld1q_u16(d);

        uint8x8_t d0_u8;
        compute_dist_wtd_avg_8x1(dd0, d0, fwd_offset, bck_offset,
                                 round_offset_vec, &d0_u8);

        vst1_u8(d_u8, d0_u8);
        d_u8 += dst8_stride;

        s += src_stride;
        d += dst_stride;
        height--;
#endif  // AOM_ARCH_AARCH64
      } while (height != 0);
      src_ptr += 8;
      dst_ptr += 8;
      dst8_ptr += 8;
      width -= 8;
    } while (width != 0);
  }
}

static INLINE void dist_wtd_convolve_y_8tap_avg_neon(
    const uint8_t *src_ptr, int src_stride, uint8_t *dst8_ptr,
    const int dst8_stride, int w, int h, const int16x8_t y_filter,
    ConvolveParams *conv_params) {
  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int16_t round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                               (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const int16x8_t round_offset_vec = vdupq_n_s16(round_offset);

  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  const int dst_stride = conv_params->dst_stride;
  int width = w;

  if (w == 4 || h == 4) {
    do {
      const uint8_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      uint8_t *d_u8 = dst8_ptr;
      int height = h;

      __builtin_prefetch(s + 0 * src_stride);
      __builtin_prefetch(s + 1 * src_stride);
      __builtin_prefetch(s + 2 * src_stride);
      __builtin_prefetch(s + 3 * src_stride);

      uint8x8_t t0 = load_unaligned_u8_4x1(s + 0 * src_stride);
      uint8x8_t t1 = load_unaligned_u8_4x1(s + 1 * src_stride);
      uint8x8_t t2 = load_unaligned_u8_4x1(s + 2 * src_stride);
      uint8x8_t t3 = load_unaligned_u8_4x1(s + 3 * src_stride);
      uint8x8_t t4 = load_unaligned_u8_4x1(s + 4 * src_stride);
      uint8x8_t t5 = load_unaligned_u8_4x1(s + 5 * src_stride);
      uint8x8_t t6 = load_unaligned_u8_4x1(s + 6 * src_stride);

      int16x4_t s0 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t0)));
      int16x4_t s1 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t1)));
      int16x4_t s2 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t2)));
      int16x4_t s3 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t3)));
      int16x4_t s4 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t4)));
      int16x4_t s5 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t5)));
      int16x4_t s6 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t6)));

      __builtin_prefetch(d + 0 * dst_stride);
      __builtin_prefetch(d + 1 * dst_stride);
      __builtin_prefetch(d + 2 * dst_stride);
      __builtin_prefetch(d + 3 * dst_stride);

      s += 7 * src_stride;

      do {
#if AOM_ARCH_AARCH64
        t0 = load_unaligned_u8_4x1(s + 0 * src_stride);
        t1 = load_unaligned_u8_4x1(s + 1 * src_stride);
        t2 = load_unaligned_u8_4x1(s + 2 * src_stride);
        t3 = load_unaligned_u8_4x1(s + 3 * src_stride);

        int16x4_t s7 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t0)));
        int16x4_t s8 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t1)));
        int16x4_t s9 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t2)));
        int16x4_t s10 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t3)));

        uint16x4_t d0 = convolve8_4_y(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d1 = convolve8_4_y(s1, s2, s3, s4, s5, s6, s7, s8, y_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d2 = convolve8_4_y(s2, s3, s4, s5, s6, s7, s8, s9, y_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d3 = convolve8_4_y(s3, s4, s5, s6, s7, s8, s9, s10, y_filter,
                                      vget_low_s16(round_offset_vec));

        __builtin_prefetch(d + 0 * dst_stride);
        __builtin_prefetch(d + 1 * dst_stride);
        __builtin_prefetch(d + 2 * dst_stride);
        __builtin_prefetch(d + 3 * dst_stride);

        __builtin_prefetch(d_u8 + 0 * dst8_stride);
        __builtin_prefetch(d_u8 + 1 * dst8_stride);
        __builtin_prefetch(d_u8 + 2 * dst8_stride);
        __builtin_prefetch(d_u8 + 3 * dst8_stride);

        uint16x4_t dd0, dd1, dd2, dd3;
        load_u16_4x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

        uint8x8_t d01, d23;
        compute_basic_avg_4x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3,
                              round_offset_vec, &d01, &d23);

        store_u8_4x1(d_u8 + 0 * dst8_stride, d01, 0);
        store_u8_4x1(d_u8 + 1 * dst8_stride, d01, 1);
        store_u8_4x1(d_u8 + 2 * dst8_stride, d23, 0);
        store_u8_4x1(d_u8 + 3 * dst8_stride, d23, 1);

        s0 = s4;
        s1 = s5;
        s2 = s6;
        s3 = s7;
        s4 = s8;
        s5 = s9;
        s6 = s10;
        s += 4 * src_stride;
        d += 4 * dst_stride;
        d_u8 += 4 * dst8_stride;
        height -= 4;
#else   // !AOM_ARCH_AARCH64
        t0 = load_unaligned_u8_4x1(s);
        int16x4_t s7 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t0)));

        uint16x4_t d0 = convolve8_4_y(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                      vget_low_s16(round_offset_vec));

        __builtin_prefetch(d);

        uint16x4_t dd0 = vld1_u16(d);

        uint8x8_t d01;
        compute_basic_avg_4x1(dd0, d0, vget_low_s16(round_offset_vec), &d01);

        store_u8_4x1(d_u8, d01, 0);

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;
        s5 = s6;
        s6 = s7;
        s += src_stride;
        d += dst_stride;
        d_u8 += dst8_stride;
        height--;
#endif  // AOM_ARCH_AARCH64
      } while (height != 0);
      src_ptr += 4;
      dst_ptr += 4;
      dst8_ptr += 4;
      width -= 4;
    } while (width != 0);
  } else {
    do {
      const uint8_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      uint8_t *d_u8 = dst8_ptr;
      int height = h;

      __builtin_prefetch(s + 0 * src_stride);
      __builtin_prefetch(s + 1 * src_stride);
      __builtin_prefetch(s + 2 * src_stride);
      __builtin_prefetch(s + 3 * src_stride);
      __builtin_prefetch(s + 4 * src_stride);
      __builtin_prefetch(s + 5 * src_stride);
      __builtin_prefetch(s + 6 * src_stride);
      __builtin_prefetch(s + 7 * src_stride);

      uint8x8_t t0, t1, t2, t3, t4, t5, t6;
      load_u8_8x7(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6);

      int16x8_t s0 = vreinterpretq_s16_u16(vmovl_u8(t0));
      int16x8_t s1 = vreinterpretq_s16_u16(vmovl_u8(t1));
      int16x8_t s2 = vreinterpretq_s16_u16(vmovl_u8(t2));
      int16x8_t s3 = vreinterpretq_s16_u16(vmovl_u8(t3));
      int16x8_t s4 = vreinterpretq_s16_u16(vmovl_u8(t4));
      int16x8_t s5 = vreinterpretq_s16_u16(vmovl_u8(t5));
      int16x8_t s6 = vreinterpretq_s16_u16(vmovl_u8(t6));

      s += 7 * src_stride;

      do {
#if AOM_ARCH_AARCH64
        uint8x8_t t7;
        load_u8_8x8(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);

        int16x8_t s7 = vreinterpretq_s16_u16(vmovl_u8(t0));
        int16x8_t s8 = vreinterpretq_s16_u16(vmovl_u8(t1));
        int16x8_t s9 = vreinterpretq_s16_u16(vmovl_u8(t2));
        int16x8_t s10 = vreinterpretq_s16_u16(vmovl_u8(t3));
        int16x8_t s11 = vreinterpretq_s16_u16(vmovl_u8(t4));
        int16x8_t s12 = vreinterpretq_s16_u16(vmovl_u8(t5));
        int16x8_t s13 = vreinterpretq_s16_u16(vmovl_u8(t6));
        int16x8_t s14 = vreinterpretq_s16_u16(vmovl_u8(t7));

        __builtin_prefetch(dst_ptr + 0 * dst_stride);
        __builtin_prefetch(dst_ptr + 1 * dst_stride);
        __builtin_prefetch(dst_ptr + 2 * dst_stride);
        __builtin_prefetch(dst_ptr + 3 * dst_stride);

        uint16x8_t d0 = convolve8_8_y(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                      round_offset_vec);
        uint16x8_t d1 = convolve8_8_y(s1, s2, s3, s4, s5, s6, s7, s8, y_filter,
                                      round_offset_vec);
        uint16x8_t d2 = convolve8_8_y(s2, s3, s4, s5, s6, s7, s8, s9, y_filter,
                                      round_offset_vec);
        uint16x8_t d3 = convolve8_8_y(s3, s4, s5, s6, s7, s8, s9, s10, y_filter,
                                      round_offset_vec);
        uint16x8_t d4 = convolve8_8_y(s4, s5, s6, s7, s8, s9, s10, s11,
                                      y_filter, round_offset_vec);
        uint16x8_t d5 = convolve8_8_y(s5, s6, s7, s8, s9, s10, s11, s12,
                                      y_filter, round_offset_vec);
        uint16x8_t d6 = convolve8_8_y(s6, s7, s8, s9, s10, s11, s12, s13,
                                      y_filter, round_offset_vec);
        uint16x8_t d7 = convolve8_8_y(s7, s8, s9, s10, s11, s12, s13, s14,
                                      y_filter, round_offset_vec);

        __builtin_prefetch(d + 0 * dst8_stride);
        __builtin_prefetch(d + 1 * dst8_stride);
        __builtin_prefetch(d + 2 * dst8_stride);
        __builtin_prefetch(d + 3 * dst8_stride);

        uint16x8_t dd0, dd1, dd2, dd3;
        load_u16_8x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

        uint8x8_t d0_u8, d1_u8, d2_u8, d3_u8;
        compute_basic_avg_8x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3,
                              round_offset_vec, &d0_u8, &d1_u8, &d2_u8, &d3_u8);

        store_u8_8x4(d_u8, dst8_stride, d0_u8, d1_u8, d2_u8, d3_u8);
        d_u8 += 4 * dst8_stride;

        uint16x8_t dd4, dd5, dd6, dd7;
        load_u16_8x4(d + 4 * dst_stride, dst_stride, &dd4, &dd5, &dd6, &dd7);

        uint8x8_t d4_u8, d5_u8, d6_u8, d7_u8;
        compute_basic_avg_8x4(dd4, dd5, dd6, dd7, d4, d5, d6, d7,
                              round_offset_vec, &d4_u8, &d5_u8, &d6_u8, &d7_u8);

        store_u8_8x4(d_u8, dst8_stride, d4_u8, d5_u8, d6_u8, d7_u8);
        d_u8 += 4 * dst8_stride;

        s0 = s8;
        s1 = s9;
        s2 = s10;
        s3 = s11;
        s4 = s12;
        s5 = s13;
        s6 = s14;
        s += 8 * src_stride;
        d += 8 * dst_stride;
        height -= 8;
#else   // !AOM_ARCH_AARCH64
        int16x8_t s7 = vreinterpretq_s16_u16(vmovl_u8(vld1_u8(s)));

        __builtin_prefetch(dst_ptr);

        uint16x8_t d0 = convolve8_8_y(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                      round_offset_vec);

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;
        s5 = s6;
        s6 = s7;

        __builtin_prefetch(d);

        uint16x8_t dd0 = vld1q_u16(d);

        uint8x8_t d0_u8;
        compute_basic_avg_8x1(dd0, d0, round_offset_vec, &d0_u8);

        vst1_u8(d_u8, d0_u8);
        d_u8 += dst8_stride;

        s += src_stride;
        d += dst_stride;
        height--;
#endif  // AOM_ARCH_AARCH64
      } while (height != 0);
      src_ptr += 8;
      dst_ptr += 8;
      dst8_ptr += 8;
      width -= 8;
    } while (width != 0);
  }
}

static INLINE void dist_wtd_convolve_y_8tap_neon(const uint8_t *src_ptr,
                                                 int src_stride, int w, int h,
                                                 const int16x8_t y_filter,
                                                 ConvolveParams *conv_params) {
  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int16_t round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                               (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const int16x8_t round_offset_vec = vdupq_n_s16(round_offset);

  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  const int dst_stride = conv_params->dst_stride;
  int width = w;

  if (w == 4 || h == 4) {
    do {
      const uint8_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      int height = h;

      __builtin_prefetch(s + 0 * src_stride);
      __builtin_prefetch(s + 1 * src_stride);
      __builtin_prefetch(s + 2 * src_stride);
      __builtin_prefetch(s + 3 * src_stride);

      uint8x8_t t0 = load_unaligned_u8_4x1(s + 0 * src_stride);
      uint8x8_t t1 = load_unaligned_u8_4x1(s + 1 * src_stride);
      uint8x8_t t2 = load_unaligned_u8_4x1(s + 2 * src_stride);
      uint8x8_t t3 = load_unaligned_u8_4x1(s + 3 * src_stride);
      uint8x8_t t4 = load_unaligned_u8_4x1(s + 4 * src_stride);
      uint8x8_t t5 = load_unaligned_u8_4x1(s + 5 * src_stride);
      uint8x8_t t6 = load_unaligned_u8_4x1(s + 6 * src_stride);

      int16x4_t s0 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t0)));
      int16x4_t s1 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t1)));
      int16x4_t s2 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t2)));
      int16x4_t s3 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t3)));
      int16x4_t s4 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t4)));
      int16x4_t s5 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t5)));
      int16x4_t s6 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t6)));

      __builtin_prefetch(d + 0 * dst_stride);
      __builtin_prefetch(d + 1 * dst_stride);
      __builtin_prefetch(d + 2 * dst_stride);
      __builtin_prefetch(d + 3 * dst_stride);

      s += 7 * src_stride;

      do {
#if AOM_ARCH_AARCH64
        t0 = load_unaligned_u8_4x1(s + 0 * src_stride);
        t1 = load_unaligned_u8_4x1(s + 1 * src_stride);
        t2 = load_unaligned_u8_4x1(s + 2 * src_stride);
        t3 = load_unaligned_u8_4x1(s + 3 * src_stride);

        int16x4_t s7 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t0)));
        int16x4_t s8 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t1)));
        int16x4_t s9 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t2)));
        int16x4_t s10 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t3)));

        uint16x4_t d0 = convolve8_4_y(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d1 = convolve8_4_y(s1, s2, s3, s4, s5, s6, s7, s8, y_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d2 = convolve8_4_y(s2, s3, s4, s5, s6, s7, s8, s9, y_filter,
                                      vget_low_s16(round_offset_vec));
        uint16x4_t d3 = convolve8_4_y(s3, s4, s5, s6, s7, s8, s9, s10, y_filter,
                                      vget_low_s16(round_offset_vec));

        store_u16_4x4(d, dst_stride, d0, d1, d2, d3);

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
#else   // !AOM_ARCH_AARCH64
        t0 = load_unaligned_u8_4x1(s);
        int16x4_t s7 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(t0)));

        uint16x4_t d0 = convolve8_4_y(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                      vget_low_s16(round_offset_vec));

        vst1_u16(d, d0);

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;
        s5 = s6;
        s6 = s7;
        s += src_stride;
        d += dst_stride;
        height--;
#endif  // AOM_ARCH_AARCH64
      } while (height != 0);
      src_ptr += 4;
      dst_ptr += 4;
      width -= 4;
    } while (width != 0);
  } else {
    do {
      const uint8_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      int height = h;

      __builtin_prefetch(s + 0 * src_stride);
      __builtin_prefetch(s + 1 * src_stride);
      __builtin_prefetch(s + 2 * src_stride);
      __builtin_prefetch(s + 3 * src_stride);
      __builtin_prefetch(s + 4 * src_stride);
      __builtin_prefetch(s + 5 * src_stride);
      __builtin_prefetch(s + 6 * src_stride);
      __builtin_prefetch(s + 7 * src_stride);

      uint8x8_t t0, t1, t2, t3, t4, t5, t6;
      load_u8_8x7(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6);

      int16x8_t s0 = vreinterpretq_s16_u16(vmovl_u8(t0));
      int16x8_t s1 = vreinterpretq_s16_u16(vmovl_u8(t1));
      int16x8_t s2 = vreinterpretq_s16_u16(vmovl_u8(t2));
      int16x8_t s3 = vreinterpretq_s16_u16(vmovl_u8(t3));
      int16x8_t s4 = vreinterpretq_s16_u16(vmovl_u8(t4));
      int16x8_t s5 = vreinterpretq_s16_u16(vmovl_u8(t5));
      int16x8_t s6 = vreinterpretq_s16_u16(vmovl_u8(t6));

      s += 7 * src_stride;

      do {
#if AOM_ARCH_AARCH64
        uint8x8_t t7;
        load_u8_8x8(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);

        int16x8_t s7 = vreinterpretq_s16_u16(vmovl_u8(t0));
        int16x8_t s8 = vreinterpretq_s16_u16(vmovl_u8(t1));
        int16x8_t s9 = vreinterpretq_s16_u16(vmovl_u8(t2));
        int16x8_t s10 = vreinterpretq_s16_u16(vmovl_u8(t3));
        int16x8_t s11 = vreinterpretq_s16_u16(vmovl_u8(t4));
        int16x8_t s12 = vreinterpretq_s16_u16(vmovl_u8(t5));
        int16x8_t s13 = vreinterpretq_s16_u16(vmovl_u8(t6));
        int16x8_t s14 = vreinterpretq_s16_u16(vmovl_u8(t7));

        __builtin_prefetch(dst_ptr + 0 * dst_stride);
        __builtin_prefetch(dst_ptr + 1 * dst_stride);
        __builtin_prefetch(dst_ptr + 2 * dst_stride);
        __builtin_prefetch(dst_ptr + 3 * dst_stride);

        uint16x8_t d0 = convolve8_8_y(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                      round_offset_vec);
        uint16x8_t d1 = convolve8_8_y(s1, s2, s3, s4, s5, s6, s7, s8, y_filter,
                                      round_offset_vec);
        uint16x8_t d2 = convolve8_8_y(s2, s3, s4, s5, s6, s7, s8, s9, y_filter,
                                      round_offset_vec);
        uint16x8_t d3 = convolve8_8_y(s3, s4, s5, s6, s7, s8, s9, s10, y_filter,
                                      round_offset_vec);
        uint16x8_t d4 = convolve8_8_y(s4, s5, s6, s7, s8, s9, s10, s11,
                                      y_filter, round_offset_vec);
        uint16x8_t d5 = convolve8_8_y(s5, s6, s7, s8, s9, s10, s11, s12,
                                      y_filter, round_offset_vec);
        uint16x8_t d6 = convolve8_8_y(s6, s7, s8, s9, s10, s11, s12, s13,
                                      y_filter, round_offset_vec);
        uint16x8_t d7 = convolve8_8_y(s7, s8, s9, s10, s11, s12, s13, s14,
                                      y_filter, round_offset_vec);

        store_u16_8x8(d, dst_stride, d0, d1, d2, d3, d4, d5, d6, d7);

        s0 = s8;
        s1 = s9;
        s2 = s10;
        s3 = s11;
        s4 = s12;
        s5 = s13;
        s6 = s14;
        s += 8 * src_stride;
        d += 8 * dst_stride;
        height -= 8;
#else   // !AOM_ARCH_AARCH64
        int16x8_t s7 = vreinterpretq_s16_u16(vmovl_u8(vld1_u8(s)));

        __builtin_prefetch(dst_ptr);

        uint16x8_t d0 = convolve8_8_y(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                      round_offset_vec);

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;
        s5 = s6;
        s6 = s7;

        vst1q_u16(d, d0);

        s += src_stride;
        d += dst_stride;
        height--;
#endif  // AOM_ARCH_AARCH64
      } while (height != 0);
      src_ptr += 8;
      dst_ptr += 8;
      width -= 8;
    } while (width != 0);
  }
}

void av1_dist_wtd_convolve_y_neon(const uint8_t *src, int src_stride,
                                  uint8_t *dst8, int dst8_stride, int w, int h,
                                  const InterpFilterParams *filter_params_y,
                                  const int subpel_y_qn,
                                  ConvolveParams *conv_params) {
  assert(w % 4 == 0);
  assert(h % 4 == 0);

  // Vertical filter.
  const int16_t *y_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_y, subpel_y_qn & SUBPEL_MASK);
  // Filter values are even, so downshift by 1 to reduce intermediate
  // precision requirements.
  const int16x8_t y_filter = vshrq_n_s16(vld1q_s16(y_filter_ptr), 1);

  const int vert_offset = filter_params_y->taps / 2 - 1;
  const uint8_t *src_ptr = src - (vert_offset * src_stride);

  if (get_filter_tap(filter_params_y, subpel_y_qn) <= 6) {
    if (conv_params->do_average) {
      if (UNLIKELY(conv_params->use_dist_wtd_comp_avg)) {
        dist_wtd_convolve_y_6tap_dist_wtd_avg_neon(
            src_ptr + src_stride, src_stride, dst8, dst8_stride, w, h, y_filter,
            conv_params);
      } else {
        dist_wtd_convolve_y_6tap_avg_neon(src_ptr + src_stride, src_stride,
                                          dst8, dst8_stride, w, h, y_filter,
                                          conv_params);
      }
    } else {
      dist_wtd_convolve_y_6tap_neon(src_ptr + src_stride, src_stride, w, h,
                                    y_filter, conv_params);
    }
  } else {
    if (conv_params->do_average) {
      if (UNLIKELY(conv_params->use_dist_wtd_comp_avg)) {
        dist_wtd_convolve_y_8tap_dist_wtd_avg_neon(src_ptr, src_stride, dst8,
                                                   dst8_stride, w, h, y_filter,
                                                   conv_params);
      } else {
        dist_wtd_convolve_y_8tap_avg_neon(src_ptr, src_stride, dst8,
                                          dst8_stride, w, h, y_filter,
                                          conv_params);
      }
    } else {
      dist_wtd_convolve_y_8tap_neon(src_ptr, src_stride, w, h, y_filter,
                                    conv_params);
    }
  }
}
