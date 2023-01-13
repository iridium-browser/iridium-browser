/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <arm_neon.h>
#include "config/aom_config.h"
#include "config/aom_dsp_rtcd.h"
#include "aom/aom_integer.h"
#include "aom_dsp/arm/sum_neon.h"

#if defined(__ARM_FEATURE_DOTPROD)

static INLINE unsigned int sadwxh_neon(const uint8_t *src_ptr, int src_stride,
                                       const uint8_t *ref_ptr, int ref_stride,
                                       int w, int h) {
  // Only two accumulators are required for optimal instruction throughput of
  // the ABD, UDOT sequence on CPUs with either 2 or 4 Neon pipes.
  uint32x4_t sum[2] = { vdupq_n_u32(0), vdupq_n_u32(0) };

  int i = 0;
  do {
    int j = 0;
    do {
      uint8x16_t s0, s1, r0, r1, diff0, diff1;

      s0 = vld1q_u8(src_ptr + j);
      r0 = vld1q_u8(ref_ptr + j);
      diff0 = vabdq_u8(s0, r0);
      sum[0] = vdotq_u32(sum[0], diff0, vdupq_n_u8(1));

      s1 = vld1q_u8(src_ptr + j + 16);
      r1 = vld1q_u8(ref_ptr + j + 16);
      diff1 = vabdq_u8(s1, r1);
      sum[1] = vdotq_u32(sum[1], diff1, vdupq_n_u8(1));

      j += 32;
    } while (j < w);

    src_ptr += src_stride;
    ref_ptr += ref_stride;
    i++;
  } while (i < h);

  return horizontal_add_u32x4(vaddq_u32(sum[0], sum[1]));
}

static INLINE unsigned int sad128xh_neon(const uint8_t *src_ptr, int src_stride,
                                         const uint8_t *ref_ptr, int ref_stride,
                                         int h) {
  return sadwxh_neon(src_ptr, src_stride, ref_ptr, ref_stride, 128, h);
}

static INLINE unsigned int sad64xh_neon(const uint8_t *src_ptr, int src_stride,
                                        const uint8_t *ref_ptr, int ref_stride,
                                        int h) {
  return sadwxh_neon(src_ptr, src_stride, ref_ptr, ref_stride, 64, h);
}

static INLINE unsigned int sad32xh_neon(const uint8_t *src_ptr, int src_stride,
                                        const uint8_t *ref_ptr, int ref_stride,
                                        int h) {
  return sadwxh_neon(src_ptr, src_stride, ref_ptr, ref_stride, 32, h);
}

static INLINE unsigned int sad16xh_neon(const uint8_t *src_ptr, int src_stride,
                                        const uint8_t *ref_ptr, int ref_stride,
                                        int h) {
  uint32x4_t sum[2] = { vdupq_n_u32(0), vdupq_n_u32(0) };

  int i = 0;
  do {
    uint8x16_t s0, s1, r0, r1, diff0, diff1;

    s0 = vld1q_u8(src_ptr);
    r0 = vld1q_u8(ref_ptr);
    diff0 = vabdq_u8(s0, r0);
    sum[0] = vdotq_u32(sum[0], diff0, vdupq_n_u8(1));

    src_ptr += src_stride;
    ref_ptr += ref_stride;

    s1 = vld1q_u8(src_ptr);
    r1 = vld1q_u8(ref_ptr);
    diff1 = vabdq_u8(s1, r1);
    sum[1] = vdotq_u32(sum[1], diff1, vdupq_n_u8(1));

    src_ptr += src_stride;
    ref_ptr += ref_stride;
    i++;
  } while (i < h / 2);

  return horizontal_add_u32x4(vaddq_u32(sum[0], sum[1]));
}

#else  // !defined(__ARM_FEATURE_DOTPROD)

static INLINE unsigned int sad128xh_neon(const uint8_t *src_ptr, int src_stride,
                                         const uint8_t *ref_ptr, int ref_stride,
                                         int h) {
  // We use 8 accumulators to prevent overflow for large values of 'h', as well
  // as enabling optimal UADALP instruction throughput on CPUs that have either
  // 2 or 4 Neon pipes.
  uint16x8_t sum[8] = { vdupq_n_u16(0), vdupq_n_u16(0), vdupq_n_u16(0),
                        vdupq_n_u16(0), vdupq_n_u16(0), vdupq_n_u16(0),
                        vdupq_n_u16(0), vdupq_n_u16(0) };

  int i = 0;
  do {
    uint8x16_t s0, s1, s2, s3, s4, s5, s6, s7;
    uint8x16_t r0, r1, r2, r3, r4, r5, r6, r7;
    uint8x16_t diff0, diff1, diff2, diff3, diff4, diff5, diff6, diff7;

    s0 = vld1q_u8(src_ptr);
    r0 = vld1q_u8(ref_ptr);
    diff0 = vabdq_u8(s0, r0);
    sum[0] = vpadalq_u8(sum[0], diff0);

    s1 = vld1q_u8(src_ptr + 16);
    r1 = vld1q_u8(ref_ptr + 16);
    diff1 = vabdq_u8(s1, r1);
    sum[1] = vpadalq_u8(sum[1], diff1);

    s2 = vld1q_u8(src_ptr + 32);
    r2 = vld1q_u8(ref_ptr + 32);
    diff2 = vabdq_u8(s2, r2);
    sum[2] = vpadalq_u8(sum[2], diff2);

    s3 = vld1q_u8(src_ptr + 48);
    r3 = vld1q_u8(ref_ptr + 48);
    diff3 = vabdq_u8(s3, r3);
    sum[3] = vpadalq_u8(sum[3], diff3);

    s4 = vld1q_u8(src_ptr + 64);
    r4 = vld1q_u8(ref_ptr + 64);
    diff4 = vabdq_u8(s4, r4);
    sum[4] = vpadalq_u8(sum[4], diff4);

    s5 = vld1q_u8(src_ptr + 80);
    r5 = vld1q_u8(ref_ptr + 80);
    diff5 = vabdq_u8(s5, r5);
    sum[5] = vpadalq_u8(sum[5], diff5);

    s6 = vld1q_u8(src_ptr + 96);
    r6 = vld1q_u8(ref_ptr + 96);
    diff6 = vabdq_u8(s6, r6);
    sum[6] = vpadalq_u8(sum[6], diff6);

    s7 = vld1q_u8(src_ptr + 112);
    r7 = vld1q_u8(ref_ptr + 112);
    diff7 = vabdq_u8(s7, r7);
    sum[7] = vpadalq_u8(sum[7], diff7);

    src_ptr += src_stride;
    ref_ptr += ref_stride;
    i++;
  } while (i < h);

  uint32x4_t sum_u32 = vpaddlq_u16(sum[0]);
  sum_u32 = vpadalq_u16(sum_u32, sum[1]);
  sum_u32 = vpadalq_u16(sum_u32, sum[2]);
  sum_u32 = vpadalq_u16(sum_u32, sum[3]);
  sum_u32 = vpadalq_u16(sum_u32, sum[4]);
  sum_u32 = vpadalq_u16(sum_u32, sum[5]);
  sum_u32 = vpadalq_u16(sum_u32, sum[6]);
  sum_u32 = vpadalq_u16(sum_u32, sum[7]);

  return horizontal_add_u32x4(sum_u32);
}

static INLINE unsigned int sad64xh_neon(const uint8_t *src_ptr, int src_stride,
                                        const uint8_t *ref_ptr, int ref_stride,
                                        int h) {
  uint16x8_t sum[4] = { vdupq_n_u16(0), vdupq_n_u16(0), vdupq_n_u16(0),
                        vdupq_n_u16(0) };

  int i = 0;
  do {
    uint8x16_t s0, s1, s2, s3, r0, r1, r2, r3;
    uint8x16_t diff0, diff1, diff2, diff3;

    s0 = vld1q_u8(src_ptr);
    r0 = vld1q_u8(ref_ptr);
    diff0 = vabdq_u8(s0, r0);
    sum[0] = vpadalq_u8(sum[0], diff0);

    s1 = vld1q_u8(src_ptr + 16);
    r1 = vld1q_u8(ref_ptr + 16);
    diff1 = vabdq_u8(s1, r1);
    sum[1] = vpadalq_u8(sum[1], diff1);

    s2 = vld1q_u8(src_ptr + 32);
    r2 = vld1q_u8(ref_ptr + 32);
    diff2 = vabdq_u8(s2, r2);
    sum[2] = vpadalq_u8(sum[2], diff2);

    s3 = vld1q_u8(src_ptr + 48);
    r3 = vld1q_u8(ref_ptr + 48);
    diff3 = vabdq_u8(s3, r3);
    sum[3] = vpadalq_u8(sum[3], diff3);

    src_ptr += src_stride;
    ref_ptr += ref_stride;
    i++;
  } while (i < h);

  uint32x4_t sum_u32 = vpaddlq_u16(sum[0]);
  sum_u32 = vpadalq_u16(sum_u32, sum[1]);
  sum_u32 = vpadalq_u16(sum_u32, sum[2]);
  sum_u32 = vpadalq_u16(sum_u32, sum[3]);

  return horizontal_add_u32x4(sum_u32);
}

static INLINE unsigned int sad32xh_neon(const uint8_t *src_ptr, int src_stride,
                                        const uint8_t *ref_ptr, int ref_stride,
                                        int h) {
  uint32x4_t sum = vdupq_n_u32(0);

  int i = 0;
  do {
    uint8x16_t s0 = vld1q_u8(src_ptr);
    uint8x16_t r0 = vld1q_u8(ref_ptr);
    uint8x16_t diff0 = vabdq_u8(s0, r0);
    uint16x8_t sum0 = vpaddlq_u8(diff0);

    uint8x16_t s1 = vld1q_u8(src_ptr + 16);
    uint8x16_t r1 = vld1q_u8(ref_ptr + 16);
    uint8x16_t diff1 = vabdq_u8(s1, r1);
    uint16x8_t sum1 = vpaddlq_u8(diff1);

    sum = vpadalq_u16(sum, sum0);
    sum = vpadalq_u16(sum, sum1);

    src_ptr += src_stride;
    ref_ptr += ref_stride;
    i++;
  } while (i < h);

  return horizontal_add_u32x4(sum);
}

static INLINE unsigned int sad16xh_neon(const uint8_t *src_ptr, int src_stride,
                                        const uint8_t *ref_ptr, int ref_stride,
                                        int h) {
  uint16x8_t sum = vdupq_n_u16(0);

  int i = 0;
  do {
    uint8x16_t s = vld1q_u8(src_ptr);
    uint8x16_t r = vld1q_u8(ref_ptr);

    uint8x16_t diff = vabdq_u8(s, r);
    sum = vpadalq_u8(sum, diff);

    src_ptr += src_stride;
    ref_ptr += ref_stride;
    i++;
  } while (i < h);

  return horizontal_add_u16x8(sum);
}

#endif  // defined(__ARM_FEATURE_DOTPROD)

static INLINE unsigned int sad8xh_neon(const uint8_t *src_ptr, int src_stride,
                                       const uint8_t *ref_ptr, int ref_stride,
                                       int h) {
  uint16x8_t sum = vdupq_n_u16(0);

  int i = 0;
  do {
    uint8x8_t s = vld1_u8(src_ptr);
    uint8x8_t r = vld1_u8(ref_ptr);

    sum = vabal_u8(sum, s, r);

    src_ptr += src_stride;
    ref_ptr += ref_stride;
    i++;
  } while (i < h);

  return horizontal_add_u16x8(sum);
}

static INLINE unsigned int sad4xh_neon(const uint8_t *src_ptr, int src_stride,
                                       const uint8_t *ref_ptr, int ref_stride,
                                       int h) {
  uint16x8_t sum = vdupq_n_u16(0);

  int i = 0;
  do {
    uint32x2_t s, r;
    uint32_t s0, s1, r0, r1;

    memcpy(&s0, src_ptr, 4);
    memcpy(&r0, ref_ptr, 4);
    s = vdup_n_u32(s0);
    r = vdup_n_u32(r0);
    src_ptr += src_stride;
    ref_ptr += ref_stride;

    memcpy(&s1, src_ptr, 4);
    memcpy(&r1, ref_ptr, 4);
    s = vset_lane_u32(s1, s, 1);
    r = vset_lane_u32(r1, r, 1);
    src_ptr += src_stride;
    ref_ptr += ref_stride;

    sum = vabal_u8(sum, vreinterpret_u8_u32(s), vreinterpret_u8_u32(r));
    i++;
  } while (i < h / 2);

  return horizontal_add_u16x8(sum);
}

#define SAD_WXH_NEON(w, h)                                                   \
  unsigned int aom_sad##w##x##h##_neon(const uint8_t *src, int src_stride,   \
                                       const uint8_t *ref, int ref_stride) { \
    return sad##w##xh_neon(src, src_stride, ref, ref_stride, (h));           \
  }

SAD_WXH_NEON(4, 4)
SAD_WXH_NEON(4, 8)
SAD_WXH_NEON(4, 16)

SAD_WXH_NEON(8, 4)
SAD_WXH_NEON(8, 8)
SAD_WXH_NEON(8, 16)
SAD_WXH_NEON(8, 32)

SAD_WXH_NEON(16, 4)
SAD_WXH_NEON(16, 8)
SAD_WXH_NEON(16, 16)
SAD_WXH_NEON(16, 32)
SAD_WXH_NEON(16, 64)

SAD_WXH_NEON(32, 8)
SAD_WXH_NEON(32, 16)
SAD_WXH_NEON(32, 32)
SAD_WXH_NEON(32, 64)

SAD_WXH_NEON(64, 16)
SAD_WXH_NEON(64, 32)
SAD_WXH_NEON(64, 64)
SAD_WXH_NEON(64, 128)

SAD_WXH_NEON(128, 64)
SAD_WXH_NEON(128, 128)

#undef SAD_WXH_NEON

#define SAD_SKIP_WXH_NEON(w, h)                                                \
  unsigned int aom_sad_skip_##w##x##h##_neon(                                  \
      const uint8_t *src, int src_stride, const uint8_t *ref,                  \
      int ref_stride) {                                                        \
    return 2 *                                                                 \
           sad##w##xh_neon(src, 2 * src_stride, ref, 2 * ref_stride, (h) / 2); \
  }

SAD_SKIP_WXH_NEON(4, 8)
SAD_SKIP_WXH_NEON(4, 16)

SAD_SKIP_WXH_NEON(8, 8)
SAD_SKIP_WXH_NEON(8, 16)
SAD_SKIP_WXH_NEON(8, 32)

SAD_SKIP_WXH_NEON(16, 8)
SAD_SKIP_WXH_NEON(16, 16)
SAD_SKIP_WXH_NEON(16, 32)
SAD_SKIP_WXH_NEON(16, 64)

SAD_SKIP_WXH_NEON(32, 8)
SAD_SKIP_WXH_NEON(32, 16)
SAD_SKIP_WXH_NEON(32, 32)
SAD_SKIP_WXH_NEON(32, 64)

SAD_SKIP_WXH_NEON(64, 16)
SAD_SKIP_WXH_NEON(64, 32)
SAD_SKIP_WXH_NEON(64, 64)
SAD_SKIP_WXH_NEON(64, 128)

SAD_SKIP_WXH_NEON(128, 64)
SAD_SKIP_WXH_NEON(128, 128)

#undef SAD_SKIP_WXH_NEON
