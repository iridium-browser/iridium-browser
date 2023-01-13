/*
 * Copyright (c) 2020, Alliance for Open Media. All rights reserved
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

#include "aom_dsp/arm/mem_neon.h"
#include "aom_dsp/arm/sum_neon.h"
#include "config/aom_dsp_rtcd.h"

static INLINE uint64_t aom_sum_squares_2d_i16_4x4_neon(const int16_t *src,
                                                       int stride) {
  int16x4_t s0 = vld1_s16(src + 0 * stride);
  int16x4_t s1 = vld1_s16(src + 1 * stride);
  int16x4_t s2 = vld1_s16(src + 2 * stride);
  int16x4_t s3 = vld1_s16(src + 3 * stride);

  int32x4_t sum_squares = vmull_s16(s0, s0);
  sum_squares = vmlal_s16(sum_squares, s1, s1);
  sum_squares = vmlal_s16(sum_squares, s2, s2);
  sum_squares = vmlal_s16(sum_squares, s3, s3);

  return horizontal_long_add_u32x4(vreinterpretq_u32_s32(sum_squares));
}

static INLINE uint64_t aom_sum_squares_2d_i16_4xn_neon(const int16_t *src,
                                                       int stride, int height) {
  int32x4_t sum_squares[2] = { vdupq_n_s32(0), vdupq_n_s32(0) };

  int h = height;
  do {
    int16x4_t s0 = vld1_s16(src + 0 * stride);
    int16x4_t s1 = vld1_s16(src + 1 * stride);
    int16x4_t s2 = vld1_s16(src + 2 * stride);
    int16x4_t s3 = vld1_s16(src + 3 * stride);

    sum_squares[0] = vmlal_s16(sum_squares[0], s0, s0);
    sum_squares[0] = vmlal_s16(sum_squares[0], s1, s1);
    sum_squares[1] = vmlal_s16(sum_squares[1], s2, s2);
    sum_squares[1] = vmlal_s16(sum_squares[1], s3, s3);

    src += 4 * stride;
    h -= 4;
  } while (h != 0);

  return horizontal_long_add_u32x4(
      vreinterpretq_u32_s32(vaddq_s32(sum_squares[0], sum_squares[1])));
}

static INLINE uint64_t aom_sum_squares_2d_i16_nxn_neon(const int16_t *src,
                                                       int stride, int width,
                                                       int height) {
  uint64x2_t sum_squares = vdupq_n_u64(0);

  int h = height;
  do {
    int32x4_t ss_row[2] = { vdupq_n_s32(0), vdupq_n_s32(0) };
    int w = 0;
    do {
      const int16_t *s = src + w;
      int16x8_t s0 = vld1q_s16(s + 0 * stride);
      int16x8_t s1 = vld1q_s16(s + 1 * stride);
      int16x8_t s2 = vld1q_s16(s + 2 * stride);
      int16x8_t s3 = vld1q_s16(s + 3 * stride);

      ss_row[0] = vmlal_s16(ss_row[0], vget_low_s16(s0), vget_low_s16(s0));
      ss_row[0] = vmlal_s16(ss_row[0], vget_low_s16(s1), vget_low_s16(s1));
      ss_row[0] = vmlal_s16(ss_row[0], vget_low_s16(s2), vget_low_s16(s2));
      ss_row[0] = vmlal_s16(ss_row[0], vget_low_s16(s3), vget_low_s16(s3));
      ss_row[1] = vmlal_s16(ss_row[1], vget_high_s16(s0), vget_high_s16(s0));
      ss_row[1] = vmlal_s16(ss_row[1], vget_high_s16(s1), vget_high_s16(s1));
      ss_row[1] = vmlal_s16(ss_row[1], vget_high_s16(s2), vget_high_s16(s2));
      ss_row[1] = vmlal_s16(ss_row[1], vget_high_s16(s3), vget_high_s16(s3));
      w += 8;
    } while (w < width);

    sum_squares = vpadalq_u32(
        sum_squares, vreinterpretq_u32_s32(vaddq_s32(ss_row[0], ss_row[1])));

    src += 4 * stride;
    h -= 4;
  } while (h != 0);

  return horizontal_add_u64x2(sum_squares);
}

uint64_t aom_sum_squares_2d_i16_neon(const int16_t *src, int stride, int width,
                                     int height) {
  // 4 elements per row only requires half an SIMD register, so this
  // must be a special case, but also note that over 75% of all calls
  // are with size == 4, so it is also the common case.
  if (LIKELY(width == 4 && height == 4)) {
    return aom_sum_squares_2d_i16_4x4_neon(src, stride);
  } else if (LIKELY(width == 4 && (height & 3) == 0)) {
    return aom_sum_squares_2d_i16_4xn_neon(src, stride, height);
  } else if (LIKELY((width & 7) == 0 && (height & 3) == 0)) {
    // Generic case
    return aom_sum_squares_2d_i16_nxn_neon(src, stride, width, height);
  } else {
    return aom_sum_squares_2d_i16_c(src, stride, width, height);
  }
}

static INLINE uint64_t aom_sum_sse_2d_i16_4x4_neon(const int16_t *src,
                                                   int stride, int *sum) {
  int16x4_t s0 = vld1_s16(src + 0 * stride);
  int16x4_t s1 = vld1_s16(src + 1 * stride);
  int16x4_t s2 = vld1_s16(src + 2 * stride);
  int16x4_t s3 = vld1_s16(src + 3 * stride);

  int32x4_t sse = vmull_s16(s0, s0);
  sse = vmlal_s16(sse, s1, s1);
  sse = vmlal_s16(sse, s2, s2);
  sse = vmlal_s16(sse, s3, s3);

  int32x4_t sum_01 = vaddl_s16(s0, s1);
  int32x4_t sum_23 = vaddl_s16(s2, s3);
  *sum += horizontal_add_s32x4(vaddq_s32(sum_01, sum_23));

  return horizontal_long_add_u32x4(vreinterpretq_u32_s32(sse));
}

static INLINE uint64_t aom_sum_sse_2d_i16_4xn_neon(const int16_t *src,
                                                   int stride, int height,
                                                   int *sum) {
  int32x4_t sse[2] = { vdupq_n_s32(0), vdupq_n_s32(0) };
  int32x2_t sum_acc[2] = { vdup_n_s32(0), vdup_n_s32(0) };

  int h = height;
  do {
    int16x4_t s0 = vld1_s16(src + 0 * stride);
    int16x4_t s1 = vld1_s16(src + 1 * stride);
    int16x4_t s2 = vld1_s16(src + 2 * stride);
    int16x4_t s3 = vld1_s16(src + 3 * stride);

    sse[0] = vmlal_s16(sse[0], s0, s0);
    sse[0] = vmlal_s16(sse[0], s1, s1);
    sse[1] = vmlal_s16(sse[1], s2, s2);
    sse[1] = vmlal_s16(sse[1], s3, s3);

    sum_acc[0] = vpadal_s16(sum_acc[0], s0);
    sum_acc[0] = vpadal_s16(sum_acc[0], s1);
    sum_acc[1] = vpadal_s16(sum_acc[1], s2);
    sum_acc[1] = vpadal_s16(sum_acc[1], s3);

    src += 4 * stride;
    h -= 4;
  } while (h != 0);

  *sum += horizontal_add_s32x4(vcombine_s32(sum_acc[0], sum_acc[1]));
  return horizontal_long_add_u32x4(
      vreinterpretq_u32_s32(vaddq_s32(sse[0], sse[1])));
}

static INLINE uint64_t aom_sum_sse_2d_i16_nxn_neon(const int16_t *src,
                                                   int stride, int width,
                                                   int height, int *sum) {
  uint64x2_t sse = vdupq_n_u64(0);
  int32x4_t sum_acc = vdupq_n_s32(0);

  int h = height;
  do {
    int32x4_t sse_row[2] = { vdupq_n_s32(0), vdupq_n_s32(0) };
    int w = 0;
    do {
      const int16_t *s = src + w;
      int16x8_t s0 = vld1q_s16(s + 0 * stride);
      int16x8_t s1 = vld1q_s16(s + 1 * stride);
      int16x8_t s2 = vld1q_s16(s + 2 * stride);
      int16x8_t s3 = vld1q_s16(s + 3 * stride);

      sse_row[0] = vmlal_s16(sse_row[0], vget_low_s16(s0), vget_low_s16(s0));
      sse_row[0] = vmlal_s16(sse_row[0], vget_low_s16(s1), vget_low_s16(s1));
      sse_row[0] = vmlal_s16(sse_row[0], vget_low_s16(s2), vget_low_s16(s2));
      sse_row[0] = vmlal_s16(sse_row[0], vget_low_s16(s3), vget_low_s16(s3));
      sse_row[1] = vmlal_s16(sse_row[1], vget_high_s16(s0), vget_high_s16(s0));
      sse_row[1] = vmlal_s16(sse_row[1], vget_high_s16(s1), vget_high_s16(s1));
      sse_row[1] = vmlal_s16(sse_row[1], vget_high_s16(s2), vget_high_s16(s2));
      sse_row[1] = vmlal_s16(sse_row[1], vget_high_s16(s3), vget_high_s16(s3));

      sum_acc = vpadalq_s16(sum_acc, s0);
      sum_acc = vpadalq_s16(sum_acc, s1);
      sum_acc = vpadalq_s16(sum_acc, s2);
      sum_acc = vpadalq_s16(sum_acc, s3);

      w += 8;
    } while (w < width);

    sse = vpadalq_u32(sse,
                      vreinterpretq_u32_s32(vaddq_s32(sse_row[0], sse_row[1])));

    src += 4 * stride;
    h -= 4;
  } while (h != 0);

  *sum += horizontal_add_s32x4(sum_acc);
  return horizontal_add_u64x2(sse);
}

uint64_t aom_sum_sse_2d_i16_neon(const int16_t *src, int stride, int width,
                                 int height, int *sum) {
  uint64_t sse;

  if (LIKELY(width == 4 && height == 4)) {
    sse = aom_sum_sse_2d_i16_4x4_neon(src, stride, sum);
  } else if (LIKELY(width == 4 && (height & 3) == 0)) {
    // width = 4, height is a multiple of 4.
    sse = aom_sum_sse_2d_i16_4xn_neon(src, stride, height, sum);
  } else if (LIKELY((width & 7) == 0 && (height & 3) == 0)) {
    // Generic case - width is multiple of 8, height is multiple of 4.
    sse = aom_sum_sse_2d_i16_nxn_neon(src, stride, width, height, sum);
  } else {
    sse = aom_sum_sse_2d_i16_c(src, stride, width, height, sum);
  }

  return sse;
}
