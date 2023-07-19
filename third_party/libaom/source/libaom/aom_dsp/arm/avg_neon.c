/*
 *  Copyright (c) 2019, Alliance for Open Media. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <arm_neon.h>
#include <assert.h>

#include "config/aom_config.h"
#include "config/aom_dsp_rtcd.h"
#include "aom/aom_integer.h"
#include "aom_dsp/arm/mem_neon.h"
#include "aom_dsp/arm/sum_neon.h"
#include "aom_dsp/arm/transpose_neon.h"
#include "aom_ports/mem.h"

#if !AOM_ARCH_AARCH64
static INLINE uint32x2_t horizontal_add_u16x8_v(const uint16x8_t a) {
  const uint32x4_t b = vpaddlq_u16(a);
  const uint64x2_t c = vpaddlq_u32(b);
  return vadd_u32(vreinterpret_u32_u64(vget_low_u64(c)),
                  vreinterpret_u32_u64(vget_high_u64(c)));
}
#endif

unsigned int aom_avg_4x4_neon(const uint8_t *a, int a_stride) {
  const uint8x16_t b = load_unaligned_u8q(a, a_stride);
  const uint16x8_t c = vaddl_u8(vget_low_u8(b), vget_high_u8(b));
#if AOM_ARCH_AARCH64
  const uint32_t d = vaddlvq_u16(c);
  return (d + 8) >> 4;
#else
  const uint32x2_t d = horizontal_add_u16x8_v(c);
  return vget_lane_u32(vrshr_n_u32(d, 4), 0);
#endif
}

unsigned int aom_avg_8x8_neon(const uint8_t *a, int a_stride) {
  uint16x8_t sum;
  uint8x8_t b = vld1_u8(a);
  a += a_stride;
  uint8x8_t c = vld1_u8(a);
  a += a_stride;
  sum = vaddl_u8(b, c);

  for (int i = 0; i < 6; ++i) {
    const uint8x8_t e = vld1_u8(a);
    a += a_stride;
    sum = vaddw_u8(sum, e);
  }

#if AOM_ARCH_AARCH64
  const uint32_t d = vaddlvq_u16(sum);
  return (d + 32) >> 6;
#else
  const uint32x2_t d = horizontal_add_u16x8_v(sum);
  return vget_lane_u32(vrshr_n_u32(d, 6), 0);
#endif
}

void aom_avg_8x8_quad_neon(const uint8_t *s, int p, int x16_idx, int y16_idx,
                           int *avg) {
  for (int k = 0; k < 4; k++) {
    const int x8_idx = x16_idx + ((k & 1) << 3);
    const int y8_idx = y16_idx + ((k >> 1) << 3);
    const uint8_t *s_tmp = s + y8_idx * p + x8_idx;
    avg[k] = aom_avg_8x8_neon(s_tmp, p);
  }
}

int aom_satd_lp_neon(const int16_t *coeff, int length) {
  const int16x4_t zero = vdup_n_s16(0);
  int32x4_t accum = vdupq_n_s32(0);

  do {
    const int16x8_t src0 = vld1q_s16(coeff);
    const int16x8_t src8 = vld1q_s16(coeff + 8);
    accum = vabal_s16(accum, vget_low_s16(src0), zero);
    accum = vabal_s16(accum, vget_high_s16(src0), zero);
    accum = vabal_s16(accum, vget_low_s16(src8), zero);
    accum = vabal_s16(accum, vget_high_s16(src8), zero);
    length -= 16;
    coeff += 16;
  } while (length != 0);

  return horizontal_add_s32x4(accum);
}

void aom_int_pro_row_neon(int16_t *hbuf, const uint8_t *ref,
                          const int ref_stride, const int width,
                          const int height, int norm_factor) {
  assert(width % 16 == 0);
  assert(height % 4 == 0);

  const int16x8_t neg_norm_factor = vdupq_n_s16(-norm_factor);
  uint16x8_t sum_lo[2], sum_hi[2];

  int w = 0;
  do {
    const uint8_t *r = ref + w;
    uint8x16_t r0 = vld1q_u8(r + 0 * ref_stride);
    uint8x16_t r1 = vld1q_u8(r + 1 * ref_stride);
    uint8x16_t r2 = vld1q_u8(r + 2 * ref_stride);
    uint8x16_t r3 = vld1q_u8(r + 3 * ref_stride);

    sum_lo[0] = vaddl_u8(vget_low_u8(r0), vget_low_u8(r1));
    sum_hi[0] = vaddl_u8(vget_high_u8(r0), vget_high_u8(r1));
    sum_lo[1] = vaddl_u8(vget_low_u8(r2), vget_low_u8(r3));
    sum_hi[1] = vaddl_u8(vget_high_u8(r2), vget_high_u8(r3));

    r += 4 * ref_stride;

    for (int h = height - 4; h != 0; h -= 4) {
      r0 = vld1q_u8(r + 0 * ref_stride);
      r1 = vld1q_u8(r + 1 * ref_stride);
      r2 = vld1q_u8(r + 2 * ref_stride);
      r3 = vld1q_u8(r + 3 * ref_stride);

      uint16x8_t tmp0_lo = vaddl_u8(vget_low_u8(r0), vget_low_u8(r1));
      uint16x8_t tmp0_hi = vaddl_u8(vget_high_u8(r0), vget_high_u8(r1));
      uint16x8_t tmp1_lo = vaddl_u8(vget_low_u8(r2), vget_low_u8(r3));
      uint16x8_t tmp1_hi = vaddl_u8(vget_high_u8(r2), vget_high_u8(r3));

      sum_lo[0] = vaddq_u16(sum_lo[0], tmp0_lo);
      sum_hi[0] = vaddq_u16(sum_hi[0], tmp0_hi);
      sum_lo[1] = vaddq_u16(sum_lo[1], tmp1_lo);
      sum_hi[1] = vaddq_u16(sum_hi[1], tmp1_hi);

      r += 4 * ref_stride;
    }

    sum_lo[0] = vaddq_u16(sum_lo[0], sum_lo[1]);
    sum_hi[0] = vaddq_u16(sum_hi[0], sum_hi[1]);

    const int16x8_t avg0 =
        vshlq_s16(vreinterpretq_s16_u16(sum_lo[0]), neg_norm_factor);
    const int16x8_t avg1 =
        vshlq_s16(vreinterpretq_s16_u16(sum_hi[0]), neg_norm_factor);

    vst1q_s16(hbuf + w, avg0);
    vst1q_s16(hbuf + w + 8, avg1);
    w += 16;
  } while (w < width);
}

void aom_int_pro_col_neon(int16_t *vbuf, const uint8_t *ref,
                          const int ref_stride, const int width,
                          const int height, int norm_factor) {
  assert(width % 16 == 0);
  assert(height % 4 == 0);

  const int16x4_t neg_norm_factor = vdup_n_s16(-norm_factor);
  uint16x8_t sum[4];

  int h = 0;
  do {
    sum[0] = vpaddlq_u8(vld1q_u8(ref + 0 * ref_stride));
    sum[1] = vpaddlq_u8(vld1q_u8(ref + 1 * ref_stride));
    sum[2] = vpaddlq_u8(vld1q_u8(ref + 2 * ref_stride));
    sum[3] = vpaddlq_u8(vld1q_u8(ref + 3 * ref_stride));

    for (int w = 16; w < width; w += 16) {
      sum[0] = vpadalq_u8(sum[0], vld1q_u8(ref + 0 * ref_stride + w));
      sum[1] = vpadalq_u8(sum[1], vld1q_u8(ref + 1 * ref_stride + w));
      sum[2] = vpadalq_u8(sum[2], vld1q_u8(ref + 2 * ref_stride + w));
      sum[3] = vpadalq_u8(sum[3], vld1q_u8(ref + 3 * ref_stride + w));
    }

    uint16x4_t sum_4d = vmovn_u32(horizontal_add_4d_u16x8(sum));
    int16x4_t avg = vshl_s16(vreinterpret_s16_u16(sum_4d), neg_norm_factor);
    vst1_s16(vbuf + h, avg);

    ref += 4 * ref_stride;
    h += 4;
  } while (h < height);
}

// coeff: 16 bits, dynamic range [-32640, 32640].
// length: value range {16, 64, 256, 1024}.
int aom_satd_neon(const tran_low_t *coeff, int length) {
  const int32x4_t zero = vdupq_n_s32(0);
  int32x4_t accum = zero;
  do {
    const int32x4_t src0 = vld1q_s32(&coeff[0]);
    const int32x4_t src8 = vld1q_s32(&coeff[4]);
    const int32x4_t src16 = vld1q_s32(&coeff[8]);
    const int32x4_t src24 = vld1q_s32(&coeff[12]);
    accum = vabaq_s32(accum, src0, zero);
    accum = vabaq_s32(accum, src8, zero);
    accum = vabaq_s32(accum, src16, zero);
    accum = vabaq_s32(accum, src24, zero);
    length -= 16;
    coeff += 16;
  } while (length != 0);

  // satd: 26 bits, dynamic range [-32640 * 1024, 32640 * 1024]
  return horizontal_add_s32x4(accum);
}

int aom_vector_var_neon(const int16_t *ref, const int16_t *src, int bwl) {
  int32x4_t v_mean = vdupq_n_s32(0);
  int32x4_t v_sse = v_mean;
  int16x8_t v_ref, v_src;
  int16x4_t v_low;

  int i, width = 4 << bwl;
  for (i = 0; i < width; i += 8) {
    v_ref = vld1q_s16(&ref[i]);
    v_src = vld1q_s16(&src[i]);
    const int16x8_t diff = vsubq_s16(v_ref, v_src);
    // diff: dynamic range [-510, 510], 10 bits.
    v_mean = vpadalq_s16(v_mean, diff);
    v_low = vget_low_s16(diff);
    v_sse = vmlal_s16(v_sse, v_low, v_low);
#if AOM_ARCH_AARCH64
    v_sse = vmlal_high_s16(v_sse, diff, diff);
#else
    const int16x4_t v_high = vget_high_s16(diff);
    v_sse = vmlal_s16(v_sse, v_high, v_high);
#endif
  }
  const int mean = horizontal_add_s32x4(v_mean);
  const int sse = horizontal_add_s32x4(v_sse);
  const unsigned int mean_abs = mean >= 0 ? mean : -mean;
  // (mean * mean): dynamic range 31 bits.
  const int var = sse - ((mean_abs * mean_abs) >> (bwl + 2));
  return var;
}

void aom_minmax_8x8_neon(const uint8_t *a, int a_stride, const uint8_t *b,
                         int b_stride, int *min, int *max) {
  // Load and concatenate.
  const uint8x16_t a01 = load_u8_8x2(a + 0 * a_stride, a_stride);
  const uint8x16_t a23 = load_u8_8x2(a + 2 * a_stride, a_stride);
  const uint8x16_t a45 = load_u8_8x2(a + 4 * a_stride, a_stride);
  const uint8x16_t a67 = load_u8_8x2(a + 6 * a_stride, a_stride);

  const uint8x16_t b01 = load_u8_8x2(b + 0 * b_stride, b_stride);
  const uint8x16_t b23 = load_u8_8x2(b + 2 * b_stride, b_stride);
  const uint8x16_t b45 = load_u8_8x2(b + 4 * b_stride, b_stride);
  const uint8x16_t b67 = load_u8_8x2(b + 6 * b_stride, b_stride);

  // Absolute difference.
  const uint8x16_t ab01_diff = vabdq_u8(a01, b01);
  const uint8x16_t ab23_diff = vabdq_u8(a23, b23);
  const uint8x16_t ab45_diff = vabdq_u8(a45, b45);
  const uint8x16_t ab67_diff = vabdq_u8(a67, b67);

  // Max values between the Q vectors.
  const uint8x16_t ab0123_max = vmaxq_u8(ab01_diff, ab23_diff);
  const uint8x16_t ab4567_max = vmaxq_u8(ab45_diff, ab67_diff);
  const uint8x16_t ab0123_min = vminq_u8(ab01_diff, ab23_diff);
  const uint8x16_t ab4567_min = vminq_u8(ab45_diff, ab67_diff);

  const uint8x16_t ab07_max = vmaxq_u8(ab0123_max, ab4567_max);
  const uint8x16_t ab07_min = vminq_u8(ab0123_min, ab4567_min);

#if AOM_ARCH_AARCH64
  *min = *max = 0;  // Clear high bits
  *((uint8_t *)max) = vmaxvq_u8(ab07_max);
  *((uint8_t *)min) = vminvq_u8(ab07_min);
#else
  // Split into 64-bit vectors and execute pairwise min/max.
  uint8x8_t ab_max = vmax_u8(vget_high_u8(ab07_max), vget_low_u8(ab07_max));
  uint8x8_t ab_min = vmin_u8(vget_high_u8(ab07_min), vget_low_u8(ab07_min));

  // Enough runs of vpmax/min propagate the max/min values to every position.
  ab_max = vpmax_u8(ab_max, ab_max);
  ab_min = vpmin_u8(ab_min, ab_min);

  ab_max = vpmax_u8(ab_max, ab_max);
  ab_min = vpmin_u8(ab_min, ab_min);

  ab_max = vpmax_u8(ab_max, ab_max);
  ab_min = vpmin_u8(ab_min, ab_min);

  *min = *max = 0;  // Clear high bits
  // Store directly to avoid costly neon->gpr transfer.
  vst1_lane_u8((uint8_t *)max, ab_max, 0);
  vst1_lane_u8((uint8_t *)min, ab_min, 0);
#endif
}
