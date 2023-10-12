/*
 * Copyright (c) 2023, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <assert.h>
#include <arm_neon.h>

#include "config/aom_config.h"
#include "config/av1_rtcd.h"

#include "aom_dsp/aom_dsp_common.h"
#include "aom_dsp/arm/mem_neon.h"
#include "aom_dsp/arm/transpose_neon.h"
#include "aom_ports/mem.h"
#include "av1/common/convolve.h"
#include "av1/common/filter.h"
#include "av1/common/arm/highbd_convolve_neon.h"

static INLINE void highbd_convolve_dist_wtd_x_8tap_neon(
    const uint16_t *src_ptr, int src_stride, uint16_t *dst_ptr, int dst_stride,
    int w, int h, const int16_t *x_filter_ptr, ConvolveParams *conv_params,
    const int offset) {
  const int16x8_t x_filter = vld1q_s16(x_filter_ptr);
  const int32x4_t shift_s32 = vdupq_n_s32(-conv_params->round_0);
  const int weight_bits = FILTER_BITS - conv_params->round_1;
  const int32x4_t zero_s32 = vdupq_n_s32(0);
  const int32x4_t weight_s32 = vdupq_n_s32(1 << weight_bits);
  const int32x4_t offset_s32 = vdupq_n_s32(offset);

  if (w <= 4) {
    const int16_t *s = (const int16_t *)src_ptr;
    uint16_t *d = dst_ptr;

    do {
      int16x8_t s0, s1, s2, s3;
      load_s16_8x2(s, src_stride, &s0, &s2);
      load_s16_8x2(s + 8, src_stride, &s1, &s3);

      uint16x4_t d0 = highbd_convolve8_wtd_horiz4_s32_s16(
          s0, s1, x_filter, shift_s32, zero_s32, weight_s32, offset_s32);
      uint16x4_t d1 = highbd_convolve8_wtd_horiz4_s32_s16(
          s2, s3, x_filter, shift_s32, zero_s32, weight_s32, offset_s32);
      uint16x8_t d01 = vcombine_u16(d0, d1);

      if (w == 2) {
        store_u16q_2x1(d + 0 * dst_stride, d01, 0);
        store_u16q_2x1(d + 1 * dst_stride, d01, 2);
      } else {
        vst1_u16(d + 0 * dst_stride, vget_low_u16(d01));
        vst1_u16(d + 1 * dst_stride, vget_high_u16(d01));
      }

      s += 2 * src_stride;
      d += 2 * dst_stride;
      h -= 2;
    } while (h > 0);
  } else {
    int height = h;

    do {
      int width = w;
      const int16_t *s = (const int16_t *)src_ptr;
      uint16_t *d = dst_ptr;

      int16x8_t s0, s2;
      load_s16_8x2(s, src_stride, &s0, &s2);
      s += 8;

      do {
        int16x8_t s1, s3;
        load_s16_8x2(s, src_stride, &s1, &s3);

        uint16x8_t d0 = highbd_convolve8_wtd_horiz8_s32_s16(
            s0, s1, x_filter, shift_s32, zero_s32, weight_s32, offset_s32);
        uint16x8_t d1 = highbd_convolve8_wtd_horiz8_s32_s16(
            s2, s3, x_filter, shift_s32, zero_s32, weight_s32, offset_s32);

        store_u16_8x2(d, dst_stride, d0, d1);

        s0 = s1;
        s2 = s3;
        s += 8;
        d += 8;
        width -= 8;
      } while (width > 0);
      src_ptr += 2 * src_stride;
      dst_ptr += 2 * dst_stride;
      height -= 2;
    } while (height > 0);
  }
}

static INLINE void highbd_convolve_dist_wtd_y_8tap_neon(
    const uint16_t *src_ptr, int src_stride, uint16_t *dst_ptr, int dst_stride,
    int w, int h, const int16_t *y_filter_ptr, ConvolveParams *conv_params,
    const int offset) {
  const int16x8_t y_filter = vld1q_s16(y_filter_ptr);
  const int32x4_t shift_s32 = vdupq_n_s32(-conv_params->round_0);
  const int weight_bits = FILTER_BITS - conv_params->round_1;
  const int32x4_t zero_s32 = vdupq_n_s32(0);
  const int32x4_t weight_s32 = vdupq_n_s32(1 << weight_bits);
  const int32x4_t offset_s32 = vdupq_n_s32(offset);

  if (w <= 4) {
    const int16_t *s = (const int16_t *)src_ptr;
    uint16_t *d = dst_ptr;

    int16x4_t s0, s1, s2, s3, s4, s5, s6;
    load_s16_4x7(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6);
    s += 7 * src_stride;

    do {
      int16x4_t s7, s8, s9, s10;
      load_s16_4x4(s, src_stride, &s7, &s8, &s9, &s10);

      uint16x4_t d0 = highbd_convolve8_wtd_4_s32_s16(
          s0, s1, s2, s3, s4, s5, s6, s7, y_filter, shift_s32, zero_s32,
          weight_s32, offset_s32);
      uint16x4_t d1 = highbd_convolve8_wtd_4_s32_s16(
          s1, s2, s3, s4, s5, s6, s7, s8, y_filter, shift_s32, zero_s32,
          weight_s32, offset_s32);
      uint16x8_t d01 = vcombine_u16(d0, d1);

      if (w == 2) {
        store_u16q_2x1(d + 0 * dst_stride, d01, 0);
        store_u16q_2x1(d + 1 * dst_stride, d01, 2);
      } else {
        vst1_u16(d + 0 * dst_stride, vget_low_u16(d01));
        vst1_u16(d + 1 * dst_stride, vget_high_u16(d01));
      }

      s0 = s2;
      s1 = s3;
      s2 = s4;
      s3 = s5;
      s4 = s6;
      s5 = s7;
      s6 = s8;
      s += 2 * src_stride;
      d += 2 * dst_stride;
      h -= 2;
    } while (h > 0);
  } else {
    do {
      int height = h;
      const int16_t *s = (const int16_t *)src_ptr;
      uint16_t *d = dst_ptr;

      int16x8_t s0, s1, s2, s3, s4, s5, s6;
      load_s16_8x7(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6);
      s += 7 * src_stride;

      do {
        int16x8_t s7, s8;
        load_s16_8x2(s, src_stride, &s7, &s8);

        uint16x8_t d0 = highbd_convolve8_wtd_8_s32_s16(
            s0, s1, s2, s3, s4, s5, s6, s7, y_filter, shift_s32, zero_s32,
            weight_s32, offset_s32);
        uint16x8_t d1 = highbd_convolve8_wtd_8_s32_s16(
            s1, s2, s3, s4, s5, s6, s7, s8, y_filter, shift_s32, zero_s32,
            weight_s32, offset_s32);

        store_u16_8x2(d, dst_stride, d0, d1);

        s0 = s2;
        s1 = s3;
        s2 = s4;
        s3 = s5;
        s4 = s6;
        s5 = s7;
        s6 = s8;
        s += 2 * src_stride;
        d += 2 * dst_stride;
        height -= 2;
      } while (height > 0);
      src_ptr += 8;
      dst_ptr += 8;
      w -= 8;
    } while (w > 0);
  }
}

void av1_highbd_dist_wtd_convolve_x_neon(
    const uint16_t *src, int src_stride, uint16_t *dst, int dst_stride, int w,
    int h, const InterpFilterParams *filter_params_x, const int subpel_x_qn,
    ConvolveParams *conv_params, int bd) {
  DECLARE_ALIGNED(16, uint16_t,
                  im_block[(MAX_SB_SIZE + MAX_FILTER_TAP) * MAX_SB_SIZE]);
  CONV_BUF_TYPE *dst16 = conv_params->dst;
  int dst16_stride = conv_params->dst_stride;
  const int im_stride = MAX_SB_SIZE;
  const int horiz_offset = filter_params_x->taps / 2 - 1;
  const int offset_bits = bd + 2 * FILTER_BITS - conv_params->round_0;
  const int round_offset = (1 << (offset_bits - conv_params->round_1)) +
                           (1 << (offset_bits - conv_params->round_1 - 1));
  const int round_bits =
      2 * FILTER_BITS - conv_params->round_0 - conv_params->round_1;
  assert(round_bits >= 0);

  const int16_t *x_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_x, subpel_x_qn & SUBPEL_MASK);

  src -= horiz_offset;

  // horizontal filter
  if (conv_params->do_average) {
    highbd_convolve_dist_wtd_x_8tap_neon(src, src_stride, im_block, im_stride,
                                         w, h, x_filter_ptr, conv_params,
                                         round_offset);
  } else {
    highbd_convolve_dist_wtd_x_8tap_neon(src, src_stride, dst16, dst16_stride,
                                         w, h, x_filter_ptr, conv_params,
                                         round_offset);
  }

  if (conv_params->do_average) {
    if (conv_params->use_dist_wtd_comp_avg) {
      highbd_dist_wtd_comp_avg_neon(im_block, im_stride, dst, dst_stride, w, h,
                                    conv_params, round_bits, round_offset, bd);
    } else {
      highbd_comp_avg_neon(im_block, im_stride, dst, dst_stride, w, h,
                           conv_params, round_bits, round_offset, bd);
    }
  }
}

void av1_highbd_dist_wtd_convolve_y_neon(
    const uint16_t *src, int src_stride, uint16_t *dst, int dst_stride, int w,
    int h, const InterpFilterParams *filter_params_y, const int subpel_y_qn,
    ConvolveParams *conv_params, int bd) {
  DECLARE_ALIGNED(16, uint16_t,
                  im_block[(MAX_SB_SIZE + MAX_FILTER_TAP) * MAX_SB_SIZE]);
  CONV_BUF_TYPE *dst16 = conv_params->dst;
  int dst16_stride = conv_params->dst_stride;
  const int im_stride = MAX_SB_SIZE;
  const int vert_offset = filter_params_y->taps / 2 - 1;
  const int offset_bits = bd + 2 * FILTER_BITS - conv_params->round_0;
  const int round_offset = (1 << (offset_bits - conv_params->round_1)) +
                           (1 << (offset_bits - conv_params->round_1 - 1));
  const int round_bits =
      2 * FILTER_BITS - conv_params->round_0 - conv_params->round_1;
  assert(round_bits >= 0);

  const int16_t *y_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_y, subpel_y_qn & SUBPEL_MASK);

  src -= vert_offset * src_stride;

  // vertical filter
  if (conv_params->do_average) {
    highbd_convolve_dist_wtd_y_8tap_neon(src, src_stride, im_block, im_stride,
                                         w, h, y_filter_ptr, conv_params,
                                         round_offset);
  } else {
    highbd_convolve_dist_wtd_y_8tap_neon(src, src_stride, dst16, dst16_stride,
                                         w, h, y_filter_ptr, conv_params,
                                         round_offset);
  }

  if (conv_params->do_average) {
    if (conv_params->use_dist_wtd_comp_avg) {
      highbd_dist_wtd_comp_avg_neon(im_block, im_stride, dst, dst_stride, w, h,
                                    conv_params, round_bits, round_offset, bd);
    } else {
      highbd_comp_avg_neon(im_block, im_stride, dst, dst_stride, w, h,
                           conv_params, round_bits, round_offset, bd);
    }
  }
}

static INLINE void highbd_2d_copy_neon(const uint16_t *src_ptr, int src_stride,
                                       uint16_t *dst_ptr, int dst_stride, int w,
                                       int h, const int round_bits,
                                       const int offset) {
  if (w <= 4) {
    const int16x4_t round_shift_s16 = vdup_n_s16(round_bits);
    const uint16x4_t offset_u16 = vdup_n_u16(offset);

    for (int y = 0; y < h; ++y) {
      const uint16x4_t s = vld1_u16(src_ptr + y * src_stride);
      uint16x4_t d = vshl_u16(s, round_shift_s16);
      d = vadd_u16(d, offset_u16);
      if (w == 2) {
        store_u16_2x1(dst_ptr + y * dst_stride, d, 0);
      } else {
        vst1_u16(dst_ptr + y * dst_stride, d);
      }
    }
  } else {
    const int16x8_t round_shift_s16 = vdupq_n_s16(round_bits);
    const uint16x8_t offset_u16 = vdupq_n_u16(offset);

    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; x += 8) {
        const uint16x8_t s = vld1q_u16(src_ptr + y * src_stride + x);
        uint16x8_t d = vshlq_u16(s, round_shift_s16);
        d = vaddq_u16(d, offset_u16);
        vst1q_u16(dst_ptr + y * dst_stride + x, d);
      }
    }
  }
}

void av1_highbd_dist_wtd_convolve_2d_copy_neon(const uint16_t *src,
                                               int src_stride, uint16_t *dst,
                                               int dst_stride, int w, int h,
                                               ConvolveParams *conv_params,
                                               int bd) {
  DECLARE_ALIGNED(16, uint16_t,
                  im_block[(MAX_SB_SIZE + MAX_FILTER_TAP) * MAX_SB_SIZE]);

  const int im_stride = MAX_SB_SIZE;
  CONV_BUF_TYPE *dst16 = conv_params->dst;
  int dst16_stride = conv_params->dst_stride;
  const int offset_bits = bd + 2 * FILTER_BITS - conv_params->round_0;
  const int round_offset = (1 << (offset_bits - conv_params->round_1)) +
                           (1 << (offset_bits - conv_params->round_1 - 1));
  const int round_bits =
      2 * FILTER_BITS - conv_params->round_0 - conv_params->round_1;
  assert(round_bits >= 0);

  if (conv_params->do_average) {
    highbd_2d_copy_neon(src, src_stride, im_block, im_stride, w, h, round_bits,
                        round_offset);
  } else {
    highbd_2d_copy_neon(src, src_stride, dst16, dst16_stride, w, h, round_bits,
                        round_offset);
  }

  if (conv_params->do_average) {
    if (conv_params->use_dist_wtd_comp_avg) {
      highbd_dist_wtd_comp_avg_neon(im_block, im_stride, dst, dst_stride, w, h,
                                    conv_params, round_bits, round_offset, bd);
    } else {
      highbd_comp_avg_neon(im_block, im_stride, dst, dst_stride, w, h,
                           conv_params, round_bits, round_offset, bd);
    }
  }
}

static INLINE void highbd_convolve_y_8tap_neon(
    const uint16_t *src_ptr, int src_stride, uint16_t *dst_ptr, int dst_stride,
    int w, int h, const int16_t *y_filter_ptr, ConvolveParams *conv_params,
    int offset) {
  const int16x8_t y_filter = vld1q_s16(y_filter_ptr);
  const int32x4_t offset_s32 = vdupq_n_s32(offset);
  const int32x4_t shift_s32 = vdupq_n_s32(-conv_params->round_1);

  if (w <= 4) {
    const int16_t *s = (const int16_t *)src_ptr;
    uint16_t *d = dst_ptr;

    int16x4_t s0, s1, s2, s3, s4, s5, s6;
    load_s16_4x7(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6);
    s += 7 * src_stride;

    do {
      int16x4_t s7, s8, s9, s10;
      load_s16_4x4(s, src_stride, &s7, &s8, &s9, &s10);

      uint16x4_t d0 = highbd_convolve8_4_sr_s32_s16(
          s0, s1, s2, s3, s4, s5, s6, s7, y_filter, shift_s32, offset_s32);
      uint16x4_t d1 = highbd_convolve8_4_sr_s32_s16(
          s1, s2, s3, s4, s5, s6, s7, s8, y_filter, shift_s32, offset_s32);
      uint16x4_t d2 = highbd_convolve8_4_sr_s32_s16(
          s2, s3, s4, s5, s6, s7, s8, s9, y_filter, shift_s32, offset_s32);
      uint16x4_t d3 = highbd_convolve8_4_sr_s32_s16(
          s3, s4, s5, s6, s7, s8, s9, s10, y_filter, shift_s32, offset_s32);

      uint16x8_t d01 = vcombine_u16(d0, d1);
      uint16x8_t d23 = vcombine_u16(d2, d3);

      if (w == 2) {
        store_u16q_2x1(d + 0 * dst_stride, d01, 0);
        store_u16q_2x1(d + 1 * dst_stride, d01, 2);
        if (h != 2) {
          store_u16q_2x1(d + 2 * dst_stride, d23, 0);
          store_u16q_2x1(d + 3 * dst_stride, d23, 2);
        }
      } else {
        vst1_u16(d + 0 * dst_stride, vget_low_u16(d01));
        vst1_u16(d + 1 * dst_stride, vget_high_u16(d01));
        if (h != 2) {
          vst1_u16(d + 2 * dst_stride, vget_low_u16(d23));
          vst1_u16(d + 3 * dst_stride, vget_high_u16(d23));
        }
      }

      s0 = s4;
      s1 = s5;
      s2 = s6;
      s3 = s7;
      s4 = s8;
      s5 = s9;
      s6 = s10;
      s += 4 * src_stride;
      d += 4 * dst_stride;
      h -= 4;
    } while (h > 0);
  } else {
    do {
      int height = h;
      const int16_t *s = (const int16_t *)src_ptr;
      uint16_t *d = dst_ptr;

      int16x8_t s0, s1, s2, s3, s4, s5, s6;
      load_s16_8x7(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6);
      s += 7 * src_stride;

      do {
        int16x8_t s7, s8, s9, s10;
        load_s16_8x4(s, src_stride, &s7, &s8, &s9, &s10);

        uint16x8_t d0 = highbd_convolve8_8_s32_s16(s0, s1, s2, s3, s4, s5, s6,
                                                   s7, y_filter, offset_s32);
        uint16x8_t d1 = highbd_convolve8_8_s32_s16(s1, s2, s3, s4, s5, s6, s7,
                                                   s8, y_filter, offset_s32);
        uint16x8_t d2 = highbd_convolve8_8_s32_s16(s2, s3, s4, s5, s6, s7, s8,
                                                   s9, y_filter, offset_s32);
        uint16x8_t d3 = highbd_convolve8_8_s32_s16(s3, s4, s5, s6, s7, s8, s9,
                                                   s10, y_filter, offset_s32);

        if (h == 2) {
          store_u16_8x2(d, dst_stride, d0, d1);
        } else {
          store_u16_8x4(d, dst_stride, d0, d1, d2, d3);
        }

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
      } while (height > 0);
      src_ptr += 8;
      dst_ptr += 8;
      w -= 8;
    } while (w > 0);
  }
}

static INLINE uint16x4_t highbd_convolve8_4_2d_h(const int16x8_t s[2],
                                                 const int16x8_t x_filter,
                                                 const int32x4_t shift_s32,
                                                 const int32x4_t offset) {
  const int16x4_t s0 = vget_low_s16(vextq_s16(s[0], s[1], 0));
  const int16x4_t s1 = vget_low_s16(vextq_s16(s[0], s[1], 1));
  const int16x4_t s2 = vget_low_s16(vextq_s16(s[0], s[1], 2));
  const int16x4_t s3 = vget_low_s16(vextq_s16(s[0], s[1], 3));
  const int16x4_t s4 = vget_high_s16(vextq_s16(s[0], s[1], 0));
  const int16x4_t s5 = vget_high_s16(vextq_s16(s[0], s[1], 1));
  const int16x4_t s6 = vget_high_s16(vextq_s16(s[0], s[1], 2));
  const int16x4_t s7 = vget_high_s16(vextq_s16(s[0], s[1], 3));

  const int16x4_t x_filter_0_3 = vget_low_s16(x_filter);
  const int16x4_t x_filter_4_7 = vget_high_s16(x_filter);

  int32x4_t sum = vmlal_lane_s16(offset, s0, x_filter_0_3, 0);
  sum = vmlal_lane_s16(sum, s1, x_filter_0_3, 1);
  sum = vmlal_lane_s16(sum, s2, x_filter_0_3, 2);
  sum = vmlal_lane_s16(sum, s3, x_filter_0_3, 3);
  sum = vmlal_lane_s16(sum, s4, x_filter_4_7, 0);
  sum = vmlal_lane_s16(sum, s5, x_filter_4_7, 1);
  sum = vmlal_lane_s16(sum, s6, x_filter_4_7, 2);
  sum = vmlal_lane_s16(sum, s7, x_filter_4_7, 3);

  sum = vqrshlq_s32(sum, shift_s32);
  return vqmovun_s32(sum);
}

static INLINE uint16x8_t highbd_convolve8_8_2d_h(const int16x8_t s[2],
                                                 const int16x8_t x_filter,
                                                 const int32x4_t shift_s32,
                                                 const int32x4_t offset) {
  const int16x8_t s0 = vextq_s16(s[0], s[1], 0);
  const int16x8_t s1 = vextq_s16(s[0], s[1], 1);
  const int16x8_t s2 = vextq_s16(s[0], s[1], 2);
  const int16x8_t s3 = vextq_s16(s[0], s[1], 3);
  const int16x8_t s4 = vextq_s16(s[0], s[1], 4);
  const int16x8_t s5 = vextq_s16(s[0], s[1], 5);
  const int16x8_t s6 = vextq_s16(s[0], s[1], 6);
  const int16x8_t s7 = vextq_s16(s[0], s[1], 7);

  const int16x4_t x_filter_0_3 = vget_low_s16(x_filter);
  const int16x4_t x_filter_4_7 = vget_high_s16(x_filter);

  int32x4_t sum0 = vmlal_lane_s16(offset, vget_low_s16(s0), x_filter_0_3, 0);
  sum0 = vmlal_lane_s16(sum0, vget_low_s16(s1), x_filter_0_3, 1);
  sum0 = vmlal_lane_s16(sum0, vget_low_s16(s2), x_filter_0_3, 2);
  sum0 = vmlal_lane_s16(sum0, vget_low_s16(s3), x_filter_0_3, 3);
  sum0 = vmlal_lane_s16(sum0, vget_low_s16(s4), x_filter_4_7, 0);
  sum0 = vmlal_lane_s16(sum0, vget_low_s16(s5), x_filter_4_7, 1);
  sum0 = vmlal_lane_s16(sum0, vget_low_s16(s6), x_filter_4_7, 2);
  sum0 = vmlal_lane_s16(sum0, vget_low_s16(s7), x_filter_4_7, 3);

  int32x4_t sum1 = vmlal_lane_s16(offset, vget_high_s16(s0), x_filter_0_3, 0);
  sum1 = vmlal_lane_s16(sum1, vget_high_s16(s1), x_filter_0_3, 1);
  sum1 = vmlal_lane_s16(sum1, vget_high_s16(s2), x_filter_0_3, 2);
  sum1 = vmlal_lane_s16(sum1, vget_high_s16(s3), x_filter_0_3, 3);
  sum1 = vmlal_lane_s16(sum1, vget_high_s16(s4), x_filter_4_7, 0);
  sum1 = vmlal_lane_s16(sum1, vget_high_s16(s5), x_filter_4_7, 1);
  sum1 = vmlal_lane_s16(sum1, vget_high_s16(s6), x_filter_4_7, 2);
  sum1 = vmlal_lane_s16(sum1, vget_high_s16(s7), x_filter_4_7, 3);

  sum0 = vqrshlq_s32(sum0, shift_s32);
  sum1 = vqrshlq_s32(sum1, shift_s32);

  return vcombine_u16(vqmovun_s32(sum0), vqmovun_s32(sum1));
}

static INLINE void highbd_convolve_x_8tap_neon(
    const uint16_t *src_ptr, int src_stride, uint16_t *dst_ptr, int dst_stride,
    int w, int h, const int16_t *x_filter_ptr, ConvolveParams *conv_params,
    const int offset) {
  const int16x8_t x_filter = vld1q_s16(x_filter_ptr);
  const int32x4_t shift_s32 = vdupq_n_s32(-conv_params->round_0);
  const int32x4_t offset_s32 = vdupq_n_s32(offset);

  if (w <= 4) {
    const int16_t *s = (const int16_t *)src_ptr;
    uint16_t *d = dst_ptr;

    do {
      int16x8_t s0[2], s1[2];
      load_s16_8x2(s, src_stride, &s0[0], &s1[0]);
      load_s16_8x2(s + 8, src_stride, &s0[1], &s1[1]);

      uint16x4_t d0 =
          highbd_convolve8_4_2d_h(s0, x_filter, shift_s32, offset_s32);
      uint16x4_t d1 =
          highbd_convolve8_4_2d_h(s1, x_filter, shift_s32, offset_s32);

      // Store 4 elements to avoid additional branches. This is safe if the
      // actual block width is < 4 because the intermediate buffer is large
      // enough to accommodate 128x128 blocks.
      vst1_u16(d + 0 * dst_stride, d0);
      vst1_u16(d + 1 * dst_stride, d1);

      s += 2 * src_stride;
      d += 2 * dst_stride;
      h -= 2;
    } while (h > 0);
  } else {
    int height = h;

    do {
      int width = w;
      const int16_t *s = (const int16_t *)src_ptr;
      uint16_t *d = dst_ptr;

      int16x8_t s0[2], s1[2], s2[2], s3[2];
      load_s16_8x4(s, src_stride, &s0[0], &s1[0], &s2[0], &s3[0]);
      s += 8;

      do {
        load_s16_8x4(s, src_stride, &s0[1], &s1[1], &s2[1], &s3[1]);

        uint16x8_t d0 =
            highbd_convolve8_8_2d_h(s0, x_filter, shift_s32, offset_s32);
        uint16x8_t d1 =
            highbd_convolve8_8_2d_h(s1, x_filter, shift_s32, offset_s32);
        uint16x8_t d2 =
            highbd_convolve8_8_2d_h(s2, x_filter, shift_s32, offset_s32);
        uint16x8_t d3 =
            highbd_convolve8_8_2d_h(s3, x_filter, shift_s32, offset_s32);

        if (h == 2) {
          store_u16_8x2(d, dst_stride, d0, d1);
        } else {
          store_u16_8x4(d, dst_stride, d0, d1, d2, d3);
        }

        s0[0] = s0[1];
        s1[0] = s1[1];
        s2[0] = s2[1];
        s3[0] = s3[1];
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

void av1_highbd_dist_wtd_convolve_2d_neon(
    const uint16_t *src, int src_stride, uint16_t *dst, int dst_stride, int w,
    int h, const InterpFilterParams *filter_params_x,
    const InterpFilterParams *filter_params_y, const int subpel_x_qn,
    const int subpel_y_qn, ConvolveParams *conv_params, int bd) {
  DECLARE_ALIGNED(16, uint16_t,
                  im_block[(MAX_SB_SIZE + MAX_FILTER_TAP) * MAX_SB_SIZE]);
  DECLARE_ALIGNED(16, uint16_t,
                  im_block2[(MAX_SB_SIZE + MAX_FILTER_TAP) * MAX_SB_SIZE]);

  CONV_BUF_TYPE *dst16 = conv_params->dst;
  int dst16_stride = conv_params->dst_stride;

  const int im_h = h + filter_params_y->taps - 1;
  const int im_stride = MAX_SB_SIZE;
  const int vert_offset = filter_params_y->taps / 2 - 1;
  const int horiz_offset = filter_params_x->taps / 2 - 1;
  const int round_bits =
      2 * FILTER_BITS - conv_params->round_0 - conv_params->round_1;
  const int x_offset_initial = (1 << (bd + FILTER_BITS - 1));
  const int y_offset_bits = bd + 2 * FILTER_BITS - conv_params->round_0;
  const int y_offset_initial = (1 << y_offset_bits);
  const int y_offset_correction =
      ((1 << (y_offset_bits - conv_params->round_1)) +
       (1 << (y_offset_bits - conv_params->round_1 - 1)));

  const uint16_t *src_ptr = src - vert_offset * src_stride - horiz_offset;

  const int16_t *x_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_x, subpel_x_qn & SUBPEL_MASK);
  const int16_t *y_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_y, subpel_y_qn & SUBPEL_MASK);

  // horizontal filter
  highbd_convolve_x_8tap_neon(src_ptr, src_stride, im_block, im_stride, w, im_h,
                              x_filter_ptr, conv_params, x_offset_initial);
  // vertical filter
  if (conv_params->do_average) {
    highbd_convolve_y_8tap_neon(im_block, im_stride, im_block2, im_stride, w, h,
                                y_filter_ptr, conv_params, y_offset_initial);
  } else {
    highbd_convolve_y_8tap_neon(im_block, im_stride, dst16, dst16_stride, w, h,
                                y_filter_ptr, conv_params, y_offset_initial);
  }

  // Do the compound averaging outside the loop, avoids branching within the
  // main loop
  if (conv_params->do_average) {
    if (conv_params->use_dist_wtd_comp_avg) {
      highbd_dist_wtd_comp_avg_neon(im_block2, im_stride, dst, dst_stride, w, h,
                                    conv_params, round_bits,
                                    y_offset_correction, bd);
    } else {
      highbd_comp_avg_neon(im_block2, im_stride, dst, dst_stride, w, h,
                           conv_params, round_bits, y_offset_correction, bd);
    }
  }
}
