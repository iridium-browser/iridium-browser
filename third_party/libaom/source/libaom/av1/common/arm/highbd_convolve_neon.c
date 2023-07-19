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

static INLINE void highbd_convolve_y_sr_6tap_neon(
    const uint16_t *src_ptr, int src_stride, uint16_t *dst_ptr, int dst_stride,
    int w, int h, const int16_t *y_filter_ptr, const int bd) {
  const uint16x8_t max = vdupq_n_u16((1 << bd) - 1);
  const int16x8_t y_filter_0_7 = vld1q_s16(y_filter_ptr);
  const int32x4_t zero_s32 = vdupq_n_s32(0);

  if (w <= 4) {
    int16x4_t s0, s1, s2, s3, s4, s5, s6, s7, s8;
    uint16x4_t d0, d1, d2, d3;
    uint16x8_t d01, d23;
    const int16_t *s = (const int16_t *)(src_ptr + src_stride);
    uint16_t *d = dst_ptr;

    load_s16_4x5(s, src_stride, &s0, &s1, &s2, &s3, &s4);
    s += 5 * src_stride;

    do {
      load_s16_4x4(s, src_stride, &s5, &s6, &s7, &s8);

      d0 = highbd_convolve6_4_s32_s16(s0, s1, s2, s3, s4, s5, y_filter_0_7,
                                      zero_s32);
      d1 = highbd_convolve6_4_s32_s16(s1, s2, s3, s4, s5, s6, y_filter_0_7,
                                      zero_s32);
      d2 = highbd_convolve6_4_s32_s16(s2, s3, s4, s5, s6, s7, y_filter_0_7,
                                      zero_s32);
      d3 = highbd_convolve6_4_s32_s16(s3, s4, s5, s6, s7, s8, y_filter_0_7,
                                      zero_s32);

      d01 = vcombine_u16(d0, d1);
      d23 = vcombine_u16(d2, d3);

      d01 = vminq_u16(d01, max);
      d23 = vminq_u16(d23, max);

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
      s += 4 * src_stride;
      d += 4 * dst_stride;
      h -= 4;
    } while (h > 0);
  } else {
    // if width is a multiple of 8 & height is a multiple of 4
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7, s8;
    uint16x8_t d0, d1, d2, d3;

    do {
      int height = h;
      const int16_t *s = (const int16_t *)(src_ptr + src_stride);
      uint16_t *d = dst_ptr;

      load_s16_8x5(s, src_stride, &s0, &s1, &s2, &s3, &s4);
      s += 5 * src_stride;

      do {
        load_s16_8x4(s, src_stride, &s5, &s6, &s7, &s8);

        d0 = highbd_convolve6_8_s32_s16(s0, s1, s2, s3, s4, s5, y_filter_0_7,
                                        zero_s32);
        d1 = highbd_convolve6_8_s32_s16(s1, s2, s3, s4, s5, s6, y_filter_0_7,
                                        zero_s32);
        d2 = highbd_convolve6_8_s32_s16(s2, s3, s4, s5, s6, s7, y_filter_0_7,
                                        zero_s32);
        d3 = highbd_convolve6_8_s32_s16(s3, s4, s5, s6, s7, s8, y_filter_0_7,
                                        zero_s32);

        d0 = vminq_u16(d0, max);
        d1 = vminq_u16(d1, max);
        d2 = vminq_u16(d2, max);
        d3 = vminq_u16(d3, max);

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

static INLINE void highbd_convolve_y_sr_8tap_neon(
    const uint16_t *src_ptr, int src_stride, uint16_t *dst_ptr, int dst_stride,
    int w, int h, const int16_t *y_filter_ptr, int bd) {
  const uint16x8_t max = vdupq_n_u16((1 << bd) - 1);
  const int16x8_t y_filter = vld1q_s16(y_filter_ptr);
  const int32x4_t zero_s32 = vdupq_n_s32(0);

  if (w <= 4) {
    int16x4_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10;
    uint16x4_t d0, d1, d2, d3;
    uint16x8_t d01, d23;

    const int16_t *s = (const int16_t *)src_ptr;
    uint16_t *d = dst_ptr;

    load_s16_4x7(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6);
    s += 7 * src_stride;

    do {
      load_s16_4x4(s, src_stride, &s7, &s8, &s9, &s10);

      d0 = highbd_convolve8_4_s32_s16(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                      zero_s32);
      d1 = highbd_convolve8_4_s32_s16(s1, s2, s3, s4, s5, s6, s7, s8, y_filter,
                                      zero_s32);
      d2 = highbd_convolve8_4_s32_s16(s2, s3, s4, s5, s6, s7, s8, s9, y_filter,
                                      zero_s32);
      d3 = highbd_convolve8_4_s32_s16(s3, s4, s5, s6, s7, s8, s9, s10, y_filter,
                                      zero_s32);

      d01 = vcombine_u16(d0, d1);
      d23 = vcombine_u16(d2, d3);

      d01 = vminq_u16(d01, max);
      d23 = vminq_u16(d23, max);

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
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10;
    uint16x8_t d0, d1, d2, d3;
    do {
      int height = h;
      const int16_t *s = (const int16_t *)src_ptr;
      uint16_t *d = dst_ptr;

      load_s16_8x7(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6);
      s += 7 * src_stride;

      do {
        load_s16_8x4(s, src_stride, &s7, &s8, &s9, &s10);

        d0 = highbd_convolve8_8_s32_s16(s0, s1, s2, s3, s4, s5, s6, s7,
                                        y_filter, zero_s32);
        d1 = highbd_convolve8_8_s32_s16(s1, s2, s3, s4, s5, s6, s7, s8,
                                        y_filter, zero_s32);
        d2 = highbd_convolve8_8_s32_s16(s2, s3, s4, s5, s6, s7, s8, s9,
                                        y_filter, zero_s32);
        d3 = highbd_convolve8_8_s32_s16(s3, s4, s5, s6, s7, s8, s9, s10,
                                        y_filter, zero_s32);

        d0 = vminq_u16(d0, max);
        d1 = vminq_u16(d1, max);
        d2 = vminq_u16(d2, max);
        d3 = vminq_u16(d3, max);

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

static INLINE void highbd_convolve_y_sr_12tap_neon(
    const uint16_t *src_ptr, int src_stride, uint16_t *dst_ptr, int dst_stride,
    int w, int h, const int16_t *y_filter_ptr, int bd) {
  const uint16x8_t max = vdupq_n_u16((1 << bd) - 1);
  const int16x8_t y_filter_0_7 = vld1q_s16(y_filter_ptr);
  const int16x4_t y_filter_8_11 = vld1_s16(y_filter_ptr + 8);
  const int32x4_t zero_s32 = vdupq_n_s32(0);

  if (w <= 4) {
    int16x4_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14;
    uint16x4_t d0, d1, d2, d3;
    uint16x8_t d01, d23;

    const int16_t *s = (const int16_t *)src_ptr;
    uint16_t *d = dst_ptr;

    load_s16_4x11(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6, &s7, &s8,
                  &s9, &s10);
    s += 11 * src_stride;

    do {
      load_s16_4x4(s, src_stride, &s11, &s12, &s13, &s14);

      d0 = highbd_convolve12_y_4_s32_s16(s0, s1, s2, s3, s4, s5, s6, s7, s8, s9,
                                         s10, s11, y_filter_0_7, y_filter_8_11,
                                         zero_s32);
      d1 = highbd_convolve12_y_4_s32_s16(s1, s2, s3, s4, s5, s6, s7, s8, s9,
                                         s10, s11, s12, y_filter_0_7,
                                         y_filter_8_11, zero_s32);
      d2 = highbd_convolve12_y_4_s32_s16(s2, s3, s4, s5, s6, s7, s8, s9, s10,
                                         s11, s12, s13, y_filter_0_7,
                                         y_filter_8_11, zero_s32);
      d3 = highbd_convolve12_y_4_s32_s16(s3, s4, s5, s6, s7, s8, s9, s10, s11,
                                         s12, s13, s14, y_filter_0_7,
                                         y_filter_8_11, zero_s32);

      d01 = vcombine_u16(d0, d1);
      d23 = vcombine_u16(d2, d3);

      d01 = vminq_u16(d01, max);
      d23 = vminq_u16(d23, max);

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
      s7 = s11;
      s8 = s12;
      s9 = s13;
      s10 = s14;
      s += 4 * src_stride;
      d += 4 * dst_stride;
      h -= 4;
    } while (h > 0);
  } else {
    uint16x8_t d0, d1, d2, d3;
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14;

    do {
      int height = h;
      const int16_t *s = (const int16_t *)src_ptr;
      uint16_t *d = dst_ptr;

      load_s16_8x11(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6, &s7, &s8,
                    &s9, &s10);
      s += 11 * src_stride;

      do {
        load_s16_8x4(s, src_stride, &s11, &s12, &s13, &s14);

        d0 = highbd_convolve12_y_8_s32_s16(s0, s1, s2, s3, s4, s5, s6, s7, s8,
                                           s9, s10, s11, y_filter_0_7,
                                           y_filter_8_11, zero_s32);
        d1 = highbd_convolve12_y_8_s32_s16(s1, s2, s3, s4, s5, s6, s7, s8, s9,
                                           s10, s11, s12, y_filter_0_7,
                                           y_filter_8_11, zero_s32);
        d2 = highbd_convolve12_y_8_s32_s16(s2, s3, s4, s5, s6, s7, s8, s9, s10,
                                           s11, s12, s13, y_filter_0_7,
                                           y_filter_8_11, zero_s32);
        d3 = highbd_convolve12_y_8_s32_s16(s3, s4, s5, s6, s7, s8, s9, s10, s11,
                                           s12, s13, s14, y_filter_0_7,
                                           y_filter_8_11, zero_s32);

        d0 = vminq_u16(d0, max);
        d1 = vminq_u16(d1, max);
        d2 = vminq_u16(d2, max);
        d3 = vminq_u16(d3, max);

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
        s7 = s11;
        s8 = s12;
        s9 = s13;
        s10 = s14;
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

void av1_highbd_convolve_y_sr_neon(const uint16_t *src, int src_stride,
                                   uint16_t *dst, int dst_stride, int w, int h,
                                   const InterpFilterParams *filter_params_y,
                                   const int subpel_y_qn, int bd) {
  const int y_filter_taps = get_filter_tap(filter_params_y, subpel_y_qn);
  const int vert_offset = filter_params_y->taps / 2 - 1;
  const int16_t *y_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_y, subpel_y_qn & SUBPEL_MASK);

  src -= vert_offset * src_stride;

  if (y_filter_taps > 8) {
    highbd_convolve_y_sr_12tap_neon(src, src_stride, dst, dst_stride, w, h,
                                    y_filter_ptr, bd);
    return;
  }
  if (y_filter_taps < 8) {
    highbd_convolve_y_sr_6tap_neon(src, src_stride, dst, dst_stride, w, h,
                                   y_filter_ptr, bd);
    return;
  }

  highbd_convolve_y_sr_8tap_neon(src, src_stride, dst, dst_stride, w, h,
                                 y_filter_ptr, bd);
}

static INLINE void highbd_convolve_x_sr_8tap_neon(
    const uint16_t *src_ptr, int src_stride, uint16_t *dst_ptr, int dst_stride,
    int w, int h, const int16_t *x_filter_ptr, ConvolveParams *conv_params,
    int bd) {
  const int16x8_t x_filter = vld1q_s16(x_filter_ptr);
  const uint16x8_t max = vdupq_n_u16((1 << bd) - 1);
  const int32x4_t shift_s32 = vdupq_n_s32(-conv_params->round_0);
  const int bits = FILTER_BITS - conv_params->round_0;
  const int16x8_t bits_s16 = vdupq_n_s16(-bits);
  const int32x4_t zero_s32 = vdupq_n_s32(0);

  if (w <= 4) {
    int16x8_t s0, s1, s2, s3;
    uint16x4_t d0, d1;
    uint16x8_t d01;

    const int16_t *s = (const int16_t *)src_ptr;
    uint16_t *d = dst_ptr;

    do {
      load_s16_8x2(s, src_stride, &s0, &s2);
      load_s16_8x2(s + 8, src_stride, &s1, &s3);

      d0 = highbd_convolve8_horiz4_s32_s16(s0, s1, x_filter, shift_s32,
                                           zero_s32);
      d1 = highbd_convolve8_horiz4_s32_s16(s2, s3, x_filter, shift_s32,
                                           zero_s32);

      d01 = vcombine_u16(d0, d1);
      d01 = vqrshlq_u16(d01, bits_s16);
      d01 = vminq_u16(d01, max);

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
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7;
    uint16x8_t d0, d1, d2, d3;
    do {
      int width = w;
      const int16_t *s = (const int16_t *)src_ptr;
      uint16_t *d = dst_ptr;

      load_s16_8x4(s, src_stride, &s0, &s2, &s4, &s6);
      s += 8;

      do {
        load_s16_8x4(s, src_stride, &s1, &s3, &s5, &s7);

        d0 = highbd_convolve8_horiz8_s32_s16(s0, s1, x_filter, shift_s32,
                                             zero_s32);
        d1 = highbd_convolve8_horiz8_s32_s16(s2, s3, x_filter, shift_s32,
                                             zero_s32);
        d2 = highbd_convolve8_horiz8_s32_s16(s4, s5, x_filter, shift_s32,
                                             zero_s32);
        d3 = highbd_convolve8_horiz8_s32_s16(s6, s7, x_filter, shift_s32,
                                             zero_s32);

        d0 = vqrshlq_u16(d0, bits_s16);
        d1 = vqrshlq_u16(d1, bits_s16);
        d2 = vqrshlq_u16(d2, bits_s16);
        d3 = vqrshlq_u16(d3, bits_s16);

        d0 = vminq_u16(d0, max);
        d1 = vminq_u16(d1, max);
        d2 = vminq_u16(d2, max);
        d3 = vminq_u16(d3, max);

        if (h == 2) {
          store_u16_8x2(d, dst_stride, d0, d1);
        } else {
          store_u16_8x4(d, dst_stride, d0, d1, d2, d3);
        }

        s0 = s1;
        s2 = s3;
        s4 = s5;
        s6 = s7;
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

static INLINE void highbd_convolve_x_sr_12tap_neon(
    const uint16_t *src_ptr, int src_stride, uint16_t *dst_ptr, int dst_stride,
    int w, int h, const int16_t *x_filter_ptr, ConvolveParams *conv_params,
    int bd) {
  const uint16x8_t max = vdupq_n_u16((1 << bd) - 1);
  const int32x4_t shift_s32 = vdupq_n_s32(-conv_params->round_0);
  const int bits = FILTER_BITS - conv_params->round_0;
  const int16x8_t bits_s16 = vdupq_n_s16(-bits);
  const int16x8_t x_filter_0_7 = vld1q_s16(x_filter_ptr);
  const int16x4_t x_filter_8_11 = vld1_s16(x_filter_ptr + 8);
  const int32x4_t zero_s32 = vdupq_n_s32(0);

  if (w <= 4) {
    int16x8_t s0, s1, s2, s3;
    uint16x4_t d0, d1;
    uint16x8_t d01;

    const int16_t *s = (const int16_t *)src_ptr;
    uint16_t *d = dst_ptr;

    do {
      load_s16_8x2(s, src_stride, &s0, &s2);
      load_s16_8x2(s + 8, src_stride, &s1, &s3);

      d0 = highbd_convolve12_horiz4_s32_s16(s0, s1, x_filter_0_7, x_filter_8_11,
                                            shift_s32, zero_s32);
      d1 = highbd_convolve12_horiz4_s32_s16(s2, s3, x_filter_0_7, x_filter_8_11,
                                            shift_s32, zero_s32);

      d01 = vcombine_u16(d0, d1);
      d01 = vqrshlq_u16(d01, bits_s16);
      d01 = vminq_u16(d01, max);

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
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
    uint16x8_t d0, d1, d2, d3;
    do {
      int width = w;
      const int16_t *s = (const int16_t *)src_ptr;
      uint16_t *d = dst_ptr;

      load_s16_8x4(s, src_stride, &s0, &s3, &s6, &s9);
      s += 8;

      do {
        load_s16_8x4(s, src_stride, &s1, &s4, &s7, &s10);
        load_s16_8x4(s + 8, src_stride, &s2, &s5, &s8, &s11);

        d0 = highbd_convolve12_horiz8_s32_s16(
            s0, s1, s2, x_filter_0_7, x_filter_8_11, shift_s32, zero_s32);
        d1 = highbd_convolve12_horiz8_s32_s16(
            s3, s4, s5, x_filter_0_7, x_filter_8_11, shift_s32, zero_s32);
        d2 = highbd_convolve12_horiz8_s32_s16(
            s6, s7, s8, x_filter_0_7, x_filter_8_11, shift_s32, zero_s32);
        d3 = highbd_convolve12_horiz8_s32_s16(
            s9, s10, s11, x_filter_0_7, x_filter_8_11, shift_s32, zero_s32);

        d0 = vqrshlq_u16(d0, bits_s16);
        d1 = vqrshlq_u16(d1, bits_s16);
        d2 = vqrshlq_u16(d2, bits_s16);
        d3 = vqrshlq_u16(d3, bits_s16);

        d0 = vminq_u16(d0, max);
        d1 = vminq_u16(d1, max);
        d2 = vminq_u16(d2, max);
        d3 = vminq_u16(d3, max);

        if (h == 2) {
          store_u16_8x2(d, dst_stride, d0, d1);
        } else {
          store_u16_8x4(d, dst_stride, d0, d1, d2, d3);
        }

        s0 = s1;
        s1 = s2;
        s3 = s4;
        s4 = s5;
        s6 = s7;
        s7 = s8;
        s9 = s10;
        s10 = s11;
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

void av1_highbd_convolve_x_sr_neon(const uint16_t *src, int src_stride,
                                   uint16_t *dst, int dst_stride, int w, int h,
                                   const InterpFilterParams *filter_params_x,
                                   const int subpel_x_qn,
                                   ConvolveParams *conv_params, int bd) {
  const int x_filter_taps = get_filter_tap(filter_params_x, subpel_x_qn);
  const int horiz_offset = filter_params_x->taps / 2 - 1;
  const int16_t *x_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_x, subpel_x_qn & SUBPEL_MASK);

  src -= horiz_offset;

  if (x_filter_taps > 8) {
    highbd_convolve_x_sr_12tap_neon(src, src_stride, dst, dst_stride, w, h,
                                    x_filter_ptr, conv_params, bd);
    return;
  }

  highbd_convolve_x_sr_8tap_neon(src, src_stride, dst, dst_stride, w, h,
                                 x_filter_ptr, conv_params, bd);
}

static INLINE void highbd_convolve_2d_y_sr_8tap_neon(
    const uint16_t *src_ptr, int src_stride, uint16_t *dst_ptr, int dst_stride,
    int w, int h, const int16_t *y_filter_ptr, ConvolveParams *conv_params,
    int bd, const int offset, const int correction) {
  const uint16x8_t max = vdupq_n_u16((1 << bd) - 1);
  const int16x8_t y_filter = vld1q_s16(y_filter_ptr);
  const int32x4_t offset_s32 = vdupq_n_s32(offset);
  const int round1_shift = conv_params->round_1;
  const int32x4_t round1_shift_s32 = vdupq_n_s32(-round1_shift);
  const int32x4_t correction_s32 = vdupq_n_s32(correction);

  if (w <= 4) {
    int16x4_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10;
    uint16x4_t d0, d1, d2, d3;
    uint16x8_t d01, d23;

    const int16_t *s = (const int16_t *)src_ptr;
    uint16_t *d = dst_ptr;

    load_s16_4x7(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6);
    s += 7 * src_stride;

    do {
      load_s16_4x4(s, src_stride, &s7, &s8, &s9, &s10);

      d0 = highbd_convolve8_4_sr_s32_s16(s0, s1, s2, s3, s4, s5, s6, s7,
                                         y_filter, round1_shift_s32, offset_s32,
                                         correction_s32);
      d1 = highbd_convolve8_4_sr_s32_s16(s1, s2, s3, s4, s5, s6, s7, s8,
                                         y_filter, round1_shift_s32, offset_s32,
                                         correction_s32);
      d2 = highbd_convolve8_4_sr_s32_s16(s2, s3, s4, s5, s6, s7, s8, s9,
                                         y_filter, round1_shift_s32, offset_s32,
                                         correction_s32);
      d3 = highbd_convolve8_4_sr_s32_s16(s3, s4, s5, s6, s7, s8, s9, s10,
                                         y_filter, round1_shift_s32, offset_s32,
                                         correction_s32);

      d01 = vcombine_u16(d0, d1);
      d23 = vcombine_u16(d2, d3);

      d01 = vminq_u16(d01, max);
      d23 = vminq_u16(d23, max);

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
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10;
    uint16x8_t d0, d1, d2, d3;
    do {
      int height = h;
      const int16_t *s = (const int16_t *)src_ptr;
      uint16_t *d = dst_ptr;

      load_s16_8x7(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6);
      s += 7 * src_stride;

      do {
        load_s16_8x4(s, src_stride, &s7, &s8, &s9, &s10);

        d0 = highbd_convolve8_8_sr_s32_s16(s0, s1, s2, s3, s4, s5, s6, s7,
                                           y_filter, round1_shift_s32,
                                           offset_s32, correction_s32);
        d1 = highbd_convolve8_8_sr_s32_s16(s1, s2, s3, s4, s5, s6, s7, s8,
                                           y_filter, round1_shift_s32,
                                           offset_s32, correction_s32);
        d2 = highbd_convolve8_8_sr_s32_s16(s2, s3, s4, s5, s6, s7, s8, s9,
                                           y_filter, round1_shift_s32,
                                           offset_s32, correction_s32);
        d3 = highbd_convolve8_8_sr_s32_s16(s3, s4, s5, s6, s7, s8, s9, s10,
                                           y_filter, round1_shift_s32,
                                           offset_s32, correction_s32);

        d0 = vminq_u16(d0, max);
        d1 = vminq_u16(d1, max);
        d2 = vminq_u16(d2, max);
        d3 = vminq_u16(d3, max);

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

static INLINE void highbd_convolve_2d_y_sr_12tap_neon(
    const uint16_t *src_ptr, int src_stride, uint16_t *dst_ptr, int dst_stride,
    int w, int h, const int16_t *y_filter_ptr, ConvolveParams *conv_params,
    const int bd, const int offset, const int correction) {
  const uint16x8_t max = vdupq_n_u16((1 << bd) - 1);
  const int16x8_t y_filter_0_7 = vld1q_s16(y_filter_ptr);
  const int16x4_t y_filter_8_11 = vld1_s16(y_filter_ptr + 8);
  const int32x4_t offset_s32 = vdupq_n_s32(offset);
  const int round1_shift = conv_params->round_1;
  const int32x4_t round1_shift_s32 = vdupq_n_s32(-round1_shift);
  const int32x4_t correction_s32 = vdupq_n_s32(correction);

  if (w <= 4) {
    int16x4_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14;
    uint16x4_t d0, d1, d2, d3;
    uint16x8_t d01, d23;

    const int16_t *s = (const int16_t *)src_ptr;
    uint16_t *d = dst_ptr;

    load_s16_4x11(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6, &s7, &s8,
                  &s9, &s10);
    s += 11 * src_stride;

    do {
      load_s16_4x4(s, src_stride, &s11, &s12, &s13, &s14);

      d0 = highbd_convolve12_y_4_sr_s32_s16(
          s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, y_filter_0_7,
          y_filter_8_11, round1_shift_s32, offset_s32, correction_s32);
      d1 = highbd_convolve12_y_4_sr_s32_s16(
          s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, y_filter_0_7,
          y_filter_8_11, round1_shift_s32, offset_s32, correction_s32);
      d2 = highbd_convolve12_y_4_sr_s32_s16(
          s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, y_filter_0_7,
          y_filter_8_11, round1_shift_s32, offset_s32, correction_s32);
      d3 = highbd_convolve12_y_4_sr_s32_s16(
          s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, y_filter_0_7,
          y_filter_8_11, round1_shift_s32, offset_s32, correction_s32);

      d01 = vcombine_u16(d0, d1);
      d23 = vcombine_u16(d2, d3);

      d01 = vminq_u16(d01, max);
      d23 = vminq_u16(d23, max);

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
      s7 = s11;
      s8 = s12;
      s9 = s13;
      s10 = s14;
      s += 4 * src_stride;
      d += 4 * dst_stride;
      h -= 4;
    } while (h > 0);
  } else {
    uint16x8_t d0, d1, d2, d3;
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14;

    do {
      int height = h;
      const int16_t *s = (const int16_t *)src_ptr;
      uint16_t *d = dst_ptr;

      load_s16_8x11(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6, &s7, &s8,
                    &s9, &s10);
      s += 11 * src_stride;

      do {
        load_s16_8x4(s, src_stride, &s11, &s12, &s13, &s14);

        d0 = highbd_convolve12_y_8_sr_s32_s16(
            s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, y_filter_0_7,
            y_filter_8_11, round1_shift_s32, offset_s32, correction_s32);
        d1 = highbd_convolve12_y_8_sr_s32_s16(
            s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, y_filter_0_7,
            y_filter_8_11, round1_shift_s32, offset_s32, correction_s32);
        d2 = highbd_convolve12_y_8_sr_s32_s16(
            s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, y_filter_0_7,
            y_filter_8_11, round1_shift_s32, offset_s32, correction_s32);
        d3 = highbd_convolve12_y_8_sr_s32_s16(
            s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, y_filter_0_7,
            y_filter_8_11, round1_shift_s32, offset_s32, correction_s32);

        d0 = vminq_u16(d0, max);
        d1 = vminq_u16(d1, max);
        d2 = vminq_u16(d2, max);
        d3 = vminq_u16(d3, max);

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
        s7 = s11;
        s8 = s12;
        s9 = s13;
        s10 = s14;
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

static INLINE void highbd_convolve_x_8tap_neon(
    const uint16_t *src_ptr, int src_stride, uint16_t *dst_ptr, int dst_stride,
    int w, int h, const int16_t *x_filter_ptr, ConvolveParams *conv_params,
    const int offset) {
  const int16x8_t x_filter = vld1q_s16(x_filter_ptr);
  const int32x4_t shift_s32 = vdupq_n_s32(-conv_params->round_0);
  const int32x4_t offset_s32 = vdupq_n_s32(offset);

  if (w <= 4) {
    int16x8_t s0, s1, s2, s3;
    uint16x4_t d0, d1;
    uint16x8_t d01;

    const int16_t *s = (const int16_t *)src_ptr;
    uint16_t *d = dst_ptr;

    do {
      load_s16_8x2(s, src_stride, &s0, &s2);
      load_s16_8x2(s + 8, src_stride, &s1, &s3);

      d0 = highbd_convolve8_horiz4_s32_s16(s0, s1, x_filter, shift_s32,
                                           offset_s32);
      d1 = highbd_convolve8_horiz4_s32_s16(s2, s3, x_filter, shift_s32,
                                           offset_s32);

      d01 = vcombine_u16(d0, d1);

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
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7;
    uint16x8_t d0, d1, d2, d3;
    do {
      int width = w;
      const int16_t *s = (const int16_t *)src_ptr;
      uint16_t *d = dst_ptr;

      load_s16_8x4(s, src_stride, &s0, &s2, &s4, &s6);
      s += 8;

      do {
        load_s16_8x4(s, src_stride, &s1, &s3, &s5, &s7);

        d0 = highbd_convolve8_horiz8_s32_s16(s0, s1, x_filter, shift_s32,
                                             offset_s32);
        d1 = highbd_convolve8_horiz8_s32_s16(s2, s3, x_filter, shift_s32,
                                             offset_s32);
        d2 = highbd_convolve8_horiz8_s32_s16(s4, s5, x_filter, shift_s32,
                                             offset_s32);
        d3 = highbd_convolve8_horiz8_s32_s16(s6, s7, x_filter, shift_s32,
                                             offset_s32);

        if (h == 2) {
          store_u16_8x2(d, dst_stride, d0, d1);
        } else {
          store_u16_8x4(d, dst_stride, d0, d1, d2, d3);
        }

        s0 = s1;
        s2 = s3;
        s4 = s5;
        s6 = s7;
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

static INLINE void highbd_convolve_2d_x_sr_12tap_neon(
    const uint16_t *src_ptr, int src_stride, uint16_t *dst_ptr, int dst_stride,
    int w, int h, const int16_t *x_filter_ptr, ConvolveParams *conv_params,
    const int offset) {
  const int32x4_t shift_s32 = vdupq_n_s32(-conv_params->round_0);
  const int16x8_t x_filter_0_7 = vld1q_s16(x_filter_ptr);
  const int16x4_t x_filter_8_11 = vld1_s16(x_filter_ptr + 8);
  const int32x4_t offset_s32 = vdupq_n_s32(offset);

  if (w <= 4) {
    int16x8_t s0, s1, s2, s3;
    uint16x4_t d0, d1;
    uint16x8_t d01;

    const int16_t *s = (const int16_t *)src_ptr;
    uint16_t *d = dst_ptr;

    do {
      load_s16_8x2(s, src_stride, &s0, &s2);
      load_s16_8x2(s + 8, src_stride, &s1, &s3);

      d0 = highbd_convolve12_horiz4_s32_s16(s0, s1, x_filter_0_7, x_filter_8_11,
                                            shift_s32, offset_s32);
      d1 = highbd_convolve12_horiz4_s32_s16(s2, s3, x_filter_0_7, x_filter_8_11,
                                            shift_s32, offset_s32);

      d01 = vcombine_u16(d0, d1);

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
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
    uint16x8_t d0, d1, d2, d3;
    do {
      int width = w;
      const int16_t *s = (const int16_t *)src_ptr;
      uint16_t *d = dst_ptr;

      load_s16_8x4(s, src_stride, &s0, &s3, &s6, &s9);
      s += 8;

      do {
        load_s16_8x4(s, src_stride, &s1, &s4, &s7, &s10);
        load_s16_8x4(s + 8, src_stride, &s2, &s5, &s8, &s11);

        d0 = highbd_convolve12_horiz8_s32_s16(
            s0, s1, s2, x_filter_0_7, x_filter_8_11, shift_s32, offset_s32);
        d1 = highbd_convolve12_horiz8_s32_s16(
            s3, s4, s5, x_filter_0_7, x_filter_8_11, shift_s32, offset_s32);
        d2 = highbd_convolve12_horiz8_s32_s16(
            s6, s7, s8, x_filter_0_7, x_filter_8_11, shift_s32, offset_s32);
        d3 = highbd_convolve12_horiz8_s32_s16(
            s9, s10, s11, x_filter_0_7, x_filter_8_11, shift_s32, offset_s32);

        if (h == 2) {
          store_u16_8x2(d, dst_stride, d0, d1);
        } else {
          store_u16_8x4(d, dst_stride, d0, d1, d2, d3);
        }

        s0 = s1;
        s1 = s2;
        s3 = s4;
        s4 = s5;
        s6 = s7;
        s7 = s8;
        s9 = s10;
        s10 = s11;
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

void av1_highbd_convolve_2d_sr_neon(const uint16_t *src, int src_stride,
                                    uint16_t *dst, int dst_stride, int w, int h,
                                    const InterpFilterParams *filter_params_x,
                                    const InterpFilterParams *filter_params_y,
                                    const int subpel_x_qn,
                                    const int subpel_y_qn,
                                    ConvolveParams *conv_params, int bd) {
  DECLARE_ALIGNED(16, uint16_t,
                  im_block[(MAX_SB_SIZE + MAX_FILTER_TAP) * MAX_SB_SIZE]);
  const int im_h = h + filter_params_y->taps - 1;
  const int im_stride = MAX_SB_SIZE;
  const int vert_offset = filter_params_y->taps / 2 - 1;
  const int horiz_offset = filter_params_x->taps / 2 - 1;
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

  if (filter_params_x->taps > 8) {
    highbd_convolve_2d_x_sr_12tap_neon(src_ptr, src_stride, im_block, im_stride,
                                       w, im_h, x_filter_ptr, conv_params,
                                       x_offset_initial);

    highbd_convolve_2d_y_sr_12tap_neon(im_block, im_stride, dst, dst_stride, w,
                                       h, y_filter_ptr, conv_params, bd,
                                       y_offset_initial, y_offset_correction);
  } else {
    highbd_convolve_x_8tap_neon(src_ptr, src_stride, im_block, im_stride, w,
                                im_h, x_filter_ptr, conv_params,
                                x_offset_initial);

    highbd_convolve_2d_y_sr_8tap_neon(im_block, im_stride, dst, dst_stride, w,
                                      h, y_filter_ptr, conv_params, bd,
                                      y_offset_initial, y_offset_correction);
  }
}

static INLINE void highbd_convolve_2d_x_scale_8tap_neon(
    const uint16_t *src_ptr, int src_stride, uint16_t *dst_ptr, int dst_stride,
    int w, int h, const int subpel_x_qn, const int x_step_qn,
    const InterpFilterParams *filter_params, ConvolveParams *conv_params,
    const int offset) {
  const uint32x4_t idx = { 0, 1, 2, 3 };
  const uint32x4_t subpel_mask = vdupq_n_u32(SCALE_SUBPEL_MASK);
  const int32x4_t shift_s32 = vdupq_n_s32(-conv_params->round_0);
  const int32x4_t offset_s32 = vdupq_n_s32(offset);

  if (w <= 4) {
    int height = h;
    int16x8_t s0, s1, s2, s3;
    uint16x4_t d0;

    uint16_t *d = dst_ptr;

    do {
      int x_qn = subpel_x_qn;

      // Load 4 src vectors at a time, they might be the same, but we have to
      // calculate the indices anyway. Doing it in SIMD and then storing the
      // indices is faster than having to calculate the expression
      // &src_ptr[((x_qn + 0*x_step_qn) >> SCALE_SUBPEL_BITS)] 4 times
      // Ideally this should be a gather using the indices, but NEON does not
      // have that, so have to emulate
      const uint32x4_t xqn_idx = vmlaq_n_u32(vdupq_n_u32(x_qn), idx, x_step_qn);
      // We have to multiply x2 to get the actual pointer as sizeof(uint16_t) =
      // 2
      const uint32x4_t src_idx_u32 =
          vshlq_n_u32(vshrq_n_u32(xqn_idx, SCALE_SUBPEL_BITS), 1);
#if AOM_ARCH_AARCH64
      uint64x2_t src4[2];
      src4[0] = vaddw_u32(vdupq_n_u64((const uint64_t)src_ptr),
                          vget_low_u32(src_idx_u32));
      src4[1] = vaddw_u32(vdupq_n_u64((const uint64_t)src_ptr),
                          vget_high_u32(src_idx_u32));
      int16_t *src4_ptr[4];
      uint64_t *tmp_ptr = (uint64_t *)&src4_ptr;
      vst1q_u64(tmp_ptr, src4[0]);
      vst1q_u64(tmp_ptr + 2, src4[1]);
#else
      uint32x4_t src4;
      src4 = vaddq_u32(vdupq_n_u32((const uint32_t)src_ptr), src_idx_u32);
      int16_t *src4_ptr[4];
      uint32_t *tmp_ptr = (uint32_t *)&src4_ptr;
      vst1q_u32(tmp_ptr, src4);
#endif  // AOM_ARCH_AARCH64
      // Same for the filter vectors
      const int32x4_t filter_idx_s32 = vreinterpretq_s32_u32(
          vshrq_n_u32(vandq_u32(xqn_idx, subpel_mask), SCALE_EXTRA_BITS));
      int32_t x_filter4_idx[4];
      vst1q_s32(x_filter4_idx, filter_idx_s32);
      const int16_t *x_filter4_ptr[4];

      // Load source
      s0 = vld1q_s16(src4_ptr[0]);
      s1 = vld1q_s16(src4_ptr[1]);
      s2 = vld1q_s16(src4_ptr[2]);
      s3 = vld1q_s16(src4_ptr[3]);

      // We could easily do this using SIMD as well instead of calling the
      // inline function 4 times.
      x_filter4_ptr[0] =
          av1_get_interp_filter_subpel_kernel(filter_params, x_filter4_idx[0]);
      x_filter4_ptr[1] =
          av1_get_interp_filter_subpel_kernel(filter_params, x_filter4_idx[1]);
      x_filter4_ptr[2] =
          av1_get_interp_filter_subpel_kernel(filter_params, x_filter4_idx[2]);
      x_filter4_ptr[3] =
          av1_get_interp_filter_subpel_kernel(filter_params, x_filter4_idx[3]);

      // Actually load the filters
      const int16x8_t x_filter0 = vld1q_s16(x_filter4_ptr[0]);
      const int16x8_t x_filter1 = vld1q_s16(x_filter4_ptr[1]);
      const int16x8_t x_filter2 = vld1q_s16(x_filter4_ptr[2]);
      const int16x8_t x_filter3 = vld1q_s16(x_filter4_ptr[3]);

      // Group low and high parts and transpose
      int16x4_t filters_lo[] = { vget_low_s16(x_filter0),
                                 vget_low_s16(x_filter1),
                                 vget_low_s16(x_filter2),
                                 vget_low_s16(x_filter3) };
      int16x4_t filters_hi[] = { vget_high_s16(x_filter0),
                                 vget_high_s16(x_filter1),
                                 vget_high_s16(x_filter2),
                                 vget_high_s16(x_filter3) };
      transpose_u16_4x4((uint16x4_t *)filters_lo);
      transpose_u16_4x4((uint16x4_t *)filters_hi);

      // Run the 2D Scale convolution
      d0 = highbd_convolve8_2d_scale_horiz4x8_s32_s16(
          s0, s1, s2, s3, filters_lo, filters_hi, shift_s32, offset_s32);

      if (w == 2) {
        store_u16_2x1(d + 0 * dst_stride, d0, 0);
      } else {
        vst1_u16(d + 0 * dst_stride, d0);
      }

      src_ptr += src_stride;
      d += dst_stride;
      height--;
    } while (height > 0);
  } else {
    int height = h;
    int16x8_t s0, s1, s2, s3;
    uint16x4_t d0;

    do {
      int width = w;
      int x_qn = subpel_x_qn;
      uint16_t *d = dst_ptr;
      const uint16_t *s = src_ptr;

      do {
        // Load 4 src vectors at a time, they might be the same, but we have to
        // calculate the indices anyway. Doing it in SIMD and then storing the
        // indices is faster than having to calculate the expression
        // &src_ptr[((x_qn + 0*x_step_qn) >> SCALE_SUBPEL_BITS)] 4 times
        // Ideally this should be a gather using the indices, but NEON does not
        // have that, so have to emulate
        const uint32x4_t xqn_idx =
            vmlaq_n_u32(vdupq_n_u32(x_qn), idx, x_step_qn);
        // We have to multiply x2 to get the actual pointer as sizeof(uint16_t)
        // = 2
        const uint32x4_t src_idx_u32 =
            vshlq_n_u32(vshrq_n_u32(xqn_idx, SCALE_SUBPEL_BITS), 1);
#if AOM_ARCH_AARCH64
        uint64x2_t src4[2];
        src4[0] = vaddw_u32(vdupq_n_u64((const uint64_t)s),
                            vget_low_u32(src_idx_u32));
        src4[1] = vaddw_u32(vdupq_n_u64((const uint64_t)s),
                            vget_high_u32(src_idx_u32));
        int16_t *src4_ptr[4];
        uint64_t *tmp_ptr = (uint64_t *)&src4_ptr;
        vst1q_u64(tmp_ptr, src4[0]);
        vst1q_u64(tmp_ptr + 2, src4[1]);
#else
        uint32x4_t src4;
        src4 = vaddq_u32(vdupq_n_u32((const uint32_t)s), src_idx_u32);
        int16_t *src4_ptr[4];
        uint32_t *tmp_ptr = (uint32_t *)&src4_ptr;
        vst1q_u32(tmp_ptr, src4);
#endif  // AOM_ARCH_AARCH64
        // Same for the filter vectors
        const int32x4_t filter_idx_s32 = vreinterpretq_s32_u32(
            vshrq_n_u32(vandq_u32(xqn_idx, subpel_mask), SCALE_EXTRA_BITS));
        int32_t x_filter4_idx[4];
        vst1q_s32(x_filter4_idx, filter_idx_s32);
        const int16_t *x_filter4_ptr[4];

        // Load source
        s0 = vld1q_s16(src4_ptr[0]);
        s1 = vld1q_s16(src4_ptr[1]);
        s2 = vld1q_s16(src4_ptr[2]);
        s3 = vld1q_s16(src4_ptr[3]);

        // We could easily do this using SIMD as well instead of calling the
        // inline function 4 times.
        x_filter4_ptr[0] = av1_get_interp_filter_subpel_kernel(
            filter_params, x_filter4_idx[0]);
        x_filter4_ptr[1] = av1_get_interp_filter_subpel_kernel(
            filter_params, x_filter4_idx[1]);
        x_filter4_ptr[2] = av1_get_interp_filter_subpel_kernel(
            filter_params, x_filter4_idx[2]);
        x_filter4_ptr[3] = av1_get_interp_filter_subpel_kernel(
            filter_params, x_filter4_idx[3]);

        // Actually load the filters
        const int16x8_t x_filter0 = vld1q_s16(x_filter4_ptr[0]);
        const int16x8_t x_filter1 = vld1q_s16(x_filter4_ptr[1]);
        const int16x8_t x_filter2 = vld1q_s16(x_filter4_ptr[2]);
        const int16x8_t x_filter3 = vld1q_s16(x_filter4_ptr[3]);

        // Group low and high parts and transpose
        int16x4_t filters_lo[] = { vget_low_s16(x_filter0),
                                   vget_low_s16(x_filter1),
                                   vget_low_s16(x_filter2),
                                   vget_low_s16(x_filter3) };
        int16x4_t filters_hi[] = { vget_high_s16(x_filter0),
                                   vget_high_s16(x_filter1),
                                   vget_high_s16(x_filter2),
                                   vget_high_s16(x_filter3) };
        transpose_u16_4x4((uint16x4_t *)filters_lo);
        transpose_u16_4x4((uint16x4_t *)filters_hi);

        // Run the 2D Scale X convolution
        d0 = highbd_convolve8_2d_scale_horiz4x8_s32_s16(
            s0, s1, s2, s3, filters_lo, filters_hi, shift_s32, offset_s32);

        vst1_u16(d, d0);

        x_qn += 4 * x_step_qn;
        d += 4;
        width -= 4;
      } while (width > 0);

      src_ptr += src_stride;
      dst_ptr += dst_stride;
      height--;
    } while (height > 0);
  }
}

static INLINE void highbd_convolve_2d_y_scale_8tap_neon(
    const uint16_t *src_ptr, int src_stride, uint16_t *dst_ptr, int dst_stride,
    int w, int h, const int subpel_y_qn, const int y_step_qn,
    const InterpFilterParams *filter_params, const int round1_bits,
    const int offset) {
  const int32x4_t offset_s32 = vdupq_n_s32(1 << offset);

  const int32x4_t round1_shift_s32 = vdupq_n_s32(-round1_bits);
  if (w <= 4) {
    int height = h;
    int16x4_t s0, s1, s2, s3, s4, s5, s6, s7;
    uint16x4_t d0;

    uint16_t *d = dst_ptr;

    int y_qn = subpel_y_qn;
    do {
      const int16_t *s =
          (const int16_t *)&src_ptr[(y_qn >> SCALE_SUBPEL_BITS) * src_stride];

      load_s16_4x8(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6, &s7);

      const int y_filter_idx = (y_qn & SCALE_SUBPEL_MASK) >> SCALE_EXTRA_BITS;
      const int16_t *y_filter_ptr =
          av1_get_interp_filter_subpel_kernel(filter_params, y_filter_idx);
      const int16x8_t y_filter = vld1q_s16(y_filter_ptr);

      d0 = highbd_convolve8_4_sr_s32_s16(s0, s1, s2, s3, s4, s5, s6, s7,
                                         y_filter, round1_shift_s32, offset_s32,
                                         vdupq_n_s32(0));

      if (w == 2) {
        store_u16_2x1(d, d0, 0);
      } else {
        vst1_u16(d, d0);
      }

      y_qn += y_step_qn;
      d += dst_stride;
      height--;
    } while (height > 0);
  } else {
    int width = w;
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7;
    uint16x8_t d0;

    do {
      int height = h;
      int y_qn = subpel_y_qn;

      uint16_t *d = dst_ptr;

      do {
        const int16_t *s =
            (const int16_t *)&src_ptr[(y_qn >> SCALE_SUBPEL_BITS) * src_stride];
        load_s16_8x8(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6, &s7);

        const int y_filter_idx = (y_qn & SCALE_SUBPEL_MASK) >> SCALE_EXTRA_BITS;
        const int16_t *y_filter_ptr =
            av1_get_interp_filter_subpel_kernel(filter_params, y_filter_idx);
        const int16x8_t y_filter = vld1q_s16(y_filter_ptr);

        d0 = highbd_convolve8_8_sr_s32_s16(s0, s1, s2, s3, s4, s5, s6, s7,
                                           y_filter, round1_shift_s32,
                                           offset_s32, vdupq_n_s32(0));
        vst1q_u16(d, d0);

        y_qn += y_step_qn;
        d += dst_stride;
        height--;
      } while (height > 0);
      src_ptr += 8;
      dst_ptr += 8;
      width -= 8;
    } while (width > 0);
  }
}

static INLINE void highbd_dist_wtd_comp_avg_neon(
    const uint16_t *src_ptr, int src_stride, uint16_t *dst_ptr, int dst_stride,
    int w, int h, ConvolveParams *conv_params, const int round_bits,
    const int offset, const int bd) {
  CONV_BUF_TYPE *dst16 = conv_params->dst;
  const int dst16_stride = conv_params->dst_stride;
  const int32x4_t round_shift_s32 = vdupq_n_s32(-round_bits);
  const int16x4_t offset_s16 = vdup_n_s16(offset);
  const uint16x8_t max = vdupq_n_u16((1 << bd) - 1);
  uint16x4_t fwd_offset_u16 = vdup_n_u16(conv_params->fwd_offset);
  uint16x4_t bck_offset_u16 = vdup_n_u16(conv_params->bck_offset);

  // Weighted averaging
  if (w <= 4) {
    for (int y = 0; y < h; ++y) {
      const uint16x4_t s = vld1_u16(src_ptr + y * src_stride);
      const uint16x4_t d16 = vld1_u16(dst16 + y * dst16_stride);
      // We use vmull_u16/vmlal_u16 instead of of vmull_s16/vmlal_s16
      // because the latter sign-extend and the values are non-negative.
      // However, d0/d1 are signed-integers and we use vqmovun
      // to do saturated narrowing to unsigned.
      int32x4_t d0 = vreinterpretq_s32_u32(vmull_u16(d16, fwd_offset_u16));
      d0 = vreinterpretq_s32_u32(
          vmlal_u16(vreinterpretq_u32_s32(d0), s, bck_offset_u16));
      d0 = vshrq_n_s32(d0, DIST_PRECISION_BITS);
      // Subtract round offset and convolve round
      d0 = vqrshlq_s32(vsubw_s16(d0, offset_s16), round_shift_s32);
      uint16x4_t d = vqmovun_s32(d0);
      d = vmin_u16(d, vget_low_u16(max));
      if (w == 2) {
        store_u16_2x1(dst_ptr + y * dst_stride, d, 0);
      } else {
        vst1_u16(dst_ptr + y * dst_stride, d);
      }
    }
  } else {
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; x += 8) {
        const uint16x8_t s = vld1q_u16(src_ptr + y * src_stride + x);
        const uint16x8_t d16 = vld1q_u16(dst16 + y * dst16_stride + x);
        // We use vmull_u16/vmlal_u16 instead of of vmull_s16/vmlal_s16
        // because the latter sign-extend and the values are non-negative.
        // However, d0/d1 are signed-integers and we use vqmovun
        // to do saturated narrowing to unsigned.
        int32x4_t d0 =
            vreinterpretq_s32_u32(vmull_u16(vget_low_u16(d16), fwd_offset_u16));
        int32x4_t d1 = vreinterpretq_s32_u32(
            vmull_u16(vget_high_u16(d16), fwd_offset_u16));
        d0 = vreinterpretq_s32_u32(vmlal_u16(vreinterpretq_u32_s32(d0),
                                             vget_low_u16(s), bck_offset_u16));
        d1 = vreinterpretq_s32_u32(vmlal_u16(vreinterpretq_u32_s32(d1),
                                             vget_high_u16(s), bck_offset_u16));
        d0 = vshrq_n_s32(d0, DIST_PRECISION_BITS);
        d1 = vshrq_n_s32(d1, DIST_PRECISION_BITS);
        d0 = vqrshlq_s32(vsubw_s16(d0, offset_s16), round_shift_s32);
        d1 = vqrshlq_s32(vsubw_s16(d1, offset_s16), round_shift_s32);
        uint16x8_t d01 = vcombine_u16(vqmovun_s32(d0), vqmovun_s32(d1));
        d01 = vminq_u16(d01, max);
        vst1q_u16(dst_ptr + y * dst_stride + x, d01);
      }
    }
  }
}

static INLINE void highbd_comp_avg_neon(const uint16_t *src_ptr, int src_stride,
                                        uint16_t *dst_ptr, int dst_stride,
                                        int w, int h,
                                        ConvolveParams *conv_params,
                                        const int round_bits, const int offset,
                                        const int bd) {
  CONV_BUF_TYPE *dst16 = conv_params->dst;
  const int dst16_stride = conv_params->dst_stride;
  const int32x4_t round_shift_s32 = vdupq_n_s32(-round_bits);
  const int16x4_t offset_s16 = vdup_n_s16(offset);
  const uint16x8_t max = vdupq_n_u16((1 << bd) - 1);

  if (w <= 4) {
    for (int y = 0; y < h; ++y) {
      const uint16x4_t s = vld1_u16(src_ptr + y * src_stride);
      const uint16x4_t d16 = vld1_u16(dst16 + y * dst16_stride);
      int32x4_t s_s32 = vreinterpretq_s32_u32(vmovl_u16(s));
      int32x4_t d16_s32 = vreinterpretq_s32_u32(vmovl_u16(d16));
      int32x4_t d0 = vhaddq_s32(s_s32, d16_s32);
      d0 = vsubw_s16(d0, offset_s16);
      d0 = vqrshlq_s32(d0, round_shift_s32);
      uint16x4_t d = vqmovun_s32(d0);
      d = vmin_u16(d, vget_low_u16(max));
      if (w == 2) {
        store_u16_2x1(dst_ptr + y * dst_stride, d, 0);
      } else {
        vst1_u16(dst_ptr + y * dst_stride, d);
      }
    }
  } else {
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; x += 8) {
        const uint16x8_t s = vld1q_u16(src_ptr + y * src_stride + x);
        const uint16x8_t d16 = vld1q_u16(dst16 + y * dst16_stride + x);
        int32x4_t s_lo = vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(s)));
        int32x4_t s_hi = vreinterpretq_s32_u32(vmovl_u16(vget_high_u16(s)));
        int32x4_t d16_lo = vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(d16)));
        int32x4_t d16_hi = vreinterpretq_s32_u32(vmovl_u16(vget_high_u16(d16)));
        int32x4_t d0 = vhaddq_s32(s_lo, d16_lo);
        int32x4_t d1 = vhaddq_s32(s_hi, d16_hi);
        d0 = vsubw_s16(d0, offset_s16);
        d1 = vsubw_s16(d1, offset_s16);
        d0 = vqrshlq_s32(d0, round_shift_s32);
        d1 = vqrshlq_s32(d1, round_shift_s32);
        uint16x8_t d01 = vcombine_u16(vqmovun_s32(d0), vqmovun_s32(d1));
        d01 = vminq_u16(d01, max);
        vst1q_u16(dst_ptr + y * dst_stride + x, d01);
      }
    }
  }
}

static INLINE void highbd_convolve_correct_offset_neon(
    const uint16_t *src_ptr, int src_stride, uint16_t *dst_ptr, int dst_stride,
    int w, int h, const int round_bits, const int offset, const int bd) {
  const int32x4_t round_shift_s32 = vdupq_n_s32(-round_bits);
  const int16x4_t offset_s16 = vdup_n_s16(offset);
  const uint16x8_t max = vdupq_n_u16((1 << bd) - 1);

  if (w <= 4) {
    for (int y = 0; y < h; ++y) {
      const int16x4_t s = vld1_s16((const int16_t *)src_ptr + y * src_stride);
      const int32x4_t d0 =
          vqrshlq_s32(vsubl_s16(s, offset_s16), round_shift_s32);
      uint16x4_t d = vqmovun_s32(d0);
      d = vmin_u16(d, vget_low_u16(max));
      if (w == 2) {
        store_u16_2x1(dst_ptr + y * dst_stride, d, 0);
      } else {
        vst1_u16(dst_ptr + y * dst_stride, d);
      }
    }
  } else {
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; x += 8) {
        // Subtract round offset and convolve round
        const int16x8_t s =
            vld1q_s16((const int16_t *)src_ptr + y * src_stride + x);
        const int32x4_t d0 = vqrshlq_s32(vsubl_s16(vget_low_s16(s), offset_s16),
                                         round_shift_s32);
        const int32x4_t d1 = vqrshlq_s32(
            vsubl_s16(vget_high_s16(s), offset_s16), round_shift_s32);
        uint16x8_t d01 = vcombine_u16(vqmovun_s32(d0), vqmovun_s32(d1));
        d01 = vminq_u16(d01, max);
        vst1q_u16(dst_ptr + y * dst_stride + x, d01);
      }
    }
  }
}

void av1_highbd_convolve_2d_scale_neon(
    const uint16_t *src, int src_stride, uint16_t *dst, int dst_stride, int w,
    int h, const InterpFilterParams *filter_params_x,
    const InterpFilterParams *filter_params_y, const int subpel_x_qn,
    const int x_step_qn, const int subpel_y_qn, const int y_step_qn,
    ConvolveParams *conv_params, int bd) {
  uint16_t *im_block = (uint16_t *)aom_memalign(
      16, 2 * sizeof(uint16_t) * MAX_SB_SIZE * (MAX_SB_SIZE + MAX_FILTER_TAP));
  if (!im_block) return;
  uint16_t *im_block2 = (uint16_t *)aom_memalign(
      16, 2 * sizeof(uint16_t) * MAX_SB_SIZE * (MAX_SB_SIZE + MAX_FILTER_TAP));
  if (!im_block2) {
    aom_free(im_block);  // free the first block and return.
    return;
  }

  int im_h = (((h - 1) * y_step_qn + subpel_y_qn) >> SCALE_SUBPEL_BITS) +
             filter_params_y->taps;
  const int im_stride = MAX_SB_SIZE;
  const int bits =
      FILTER_BITS * 2 - conv_params->round_0 - conv_params->round_1;
  assert(bits >= 0);

  const int vert_offset = filter_params_y->taps / 2 - 1;
  const int horiz_offset = filter_params_x->taps / 2 - 1;
  const int x_offset_bits = (1 << (bd + FILTER_BITS - 1));
  const int y_offset_bits = bd + 2 * FILTER_BITS - conv_params->round_0;
  const int y_offset_correction =
      ((1 << (y_offset_bits - conv_params->round_1)) +
       (1 << (y_offset_bits - conv_params->round_1 - 1)));

  CONV_BUF_TYPE *dst16 = conv_params->dst;
  const int dst16_stride = conv_params->dst_stride;

  const uint16_t *src_ptr = src - vert_offset * src_stride - horiz_offset;

  highbd_convolve_2d_x_scale_8tap_neon(
      src_ptr, src_stride, im_block, im_stride, w, im_h, subpel_x_qn, x_step_qn,
      filter_params_x, conv_params, x_offset_bits);
  if (conv_params->is_compound && !conv_params->do_average) {
    highbd_convolve_2d_y_scale_8tap_neon(
        im_block, im_stride, dst16, dst16_stride, w, h, subpel_y_qn, y_step_qn,
        filter_params_y, conv_params->round_1, y_offset_bits);
  } else {
    highbd_convolve_2d_y_scale_8tap_neon(
        im_block, im_stride, im_block2, im_stride, w, h, subpel_y_qn, y_step_qn,
        filter_params_y, conv_params->round_1, y_offset_bits);
  }

  // Do the compound averaging outside the loop, avoids branching within the
  // main loop
  if (conv_params->is_compound) {
    if (conv_params->do_average) {
      if (conv_params->use_dist_wtd_comp_avg) {
        highbd_dist_wtd_comp_avg_neon(im_block2, im_stride, dst, dst_stride, w,
                                      h, conv_params, bits, y_offset_correction,
                                      bd);
      } else {
        highbd_comp_avg_neon(im_block2, im_stride, dst, dst_stride, w, h,
                             conv_params, bits, y_offset_correction, bd);
      }
    }
  } else {
    highbd_convolve_correct_offset_neon(im_block2, im_stride, dst, dst_stride,
                                        w, h, bits, y_offset_correction, bd);
  }
  aom_free(im_block);
  aom_free(im_block2);
}

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
    int16x8_t s0, s1, s2, s3;
    uint16x4_t d0, d1;
    uint16x8_t d01;

    const int16_t *s = (const int16_t *)src_ptr;
    uint16_t *d = dst_ptr;

    do {
      load_s16_8x2(s, src_stride, &s0, &s2);
      load_s16_8x2(s + 8, src_stride, &s1, &s3);

      d0 = highbd_convolve8_wtd_horiz4_s32_s16(
          s0, s1, x_filter, shift_s32, zero_s32, weight_s32, offset_s32);
      d1 = highbd_convolve8_wtd_horiz4_s32_s16(
          s2, s3, x_filter, shift_s32, zero_s32, weight_s32, offset_s32);
      d01 = vcombine_u16(d0, d1);

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
    int16x8_t s0, s1, s2, s3;
    uint16x8_t d0, d1;

    do {
      int width = w;
      const int16_t *s = (const int16_t *)src_ptr;
      uint16_t *d = dst_ptr;

      load_s16_8x2(s, src_stride, &s0, &s2);
      s += 8;

      do {
        load_s16_8x2(s, src_stride, &s1, &s3);

        d0 = highbd_convolve8_wtd_horiz8_s32_s16(
            s0, s1, x_filter, shift_s32, zero_s32, weight_s32, offset_s32);
        d1 = highbd_convolve8_wtd_horiz8_s32_s16(
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
    int16x4_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10;
    uint16x4_t d0, d1;
    uint16x8_t d01;

    const int16_t *s = (const int16_t *)src_ptr;
    uint16_t *d = dst_ptr;

    load_s16_4x7(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6);
    s += 7 * src_stride;

    do {
      load_s16_4x4(s, src_stride, &s7, &s8, &s9, &s10);

      d0 = highbd_convolve8_wtd_4_s32_s16(s0, s1, s2, s3, s4, s5, s6, s7,
                                          y_filter, shift_s32, zero_s32,
                                          weight_s32, offset_s32);
      d1 = highbd_convolve8_wtd_4_s32_s16(s1, s2, s3, s4, s5, s6, s7, s8,
                                          y_filter, shift_s32, zero_s32,
                                          weight_s32, offset_s32);
      d01 = vcombine_u16(d0, d1);

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
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7, s8;
    uint16x8_t d0, d1;

    do {
      int height = h;
      const int16_t *s = (const int16_t *)src_ptr;
      uint16_t *d = dst_ptr;

      load_s16_8x7(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6);
      s += 7 * src_stride;

      do {
        load_s16_8x2(s, src_stride, &s7, &s8);

        d0 = highbd_convolve8_wtd_8_s32_s16(s0, s1, s2, s3, s4, s5, s6, s7,
                                            y_filter, shift_s32, zero_s32,
                                            weight_s32, offset_s32);
        d1 = highbd_convolve8_wtd_8_s32_s16(s1, s2, s3, s4, s5, s6, s7, s8,
                                            y_filter, shift_s32, zero_s32,
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
    int16x4_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10;
    uint16x4_t d0, d1, d2, d3;
    uint16x8_t d01, d23;

    const int16_t *s = (const int16_t *)src_ptr;
    uint16_t *d = dst_ptr;

    load_s16_4x7(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6);
    s += 7 * src_stride;

    do {
      load_s16_4x4(s, src_stride, &s7, &s8, &s9, &s10);

      d0 = highbd_convolve8_sr_4_s32_s16(s0, s1, s2, s3, s4, s5, s6, s7,
                                         y_filter, shift_s32, offset_s32);
      d1 = highbd_convolve8_sr_4_s32_s16(s1, s2, s3, s4, s5, s6, s7, s8,
                                         y_filter, shift_s32, offset_s32);
      d2 = highbd_convolve8_sr_4_s32_s16(s2, s3, s4, s5, s6, s7, s8, s9,
                                         y_filter, shift_s32, offset_s32);
      d3 = highbd_convolve8_sr_4_s32_s16(s3, s4, s5, s6, s7, s8, s9, s10,
                                         y_filter, shift_s32, offset_s32);

      d01 = vcombine_u16(d0, d1);
      d23 = vcombine_u16(d2, d3);

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
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10;
    uint16x8_t d0, d1, d2, d3;
    do {
      int height = h;
      const int16_t *s = (const int16_t *)src_ptr;
      uint16_t *d = dst_ptr;

      load_s16_8x7(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6);
      s += 7 * src_stride;

      do {
        load_s16_8x4(s, src_stride, &s7, &s8, &s9, &s10);

        d0 = highbd_convolve8_8_s32_s16(s0, s1, s2, s3, s4, s5, s6, s7,
                                        y_filter, offset_s32);
        d1 = highbd_convolve8_8_s32_s16(s1, s2, s3, s4, s5, s6, s7, s8,
                                        y_filter, offset_s32);
        d2 = highbd_convolve8_8_s32_s16(s2, s3, s4, s5, s6, s7, s8, s9,
                                        y_filter, offset_s32);
        d3 = highbd_convolve8_8_s32_s16(s3, s4, s5, s6, s7, s8, s9, s10,
                                        y_filter, offset_s32);

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

#define UPSCALE_NORMATIVE_TAPS 8

void av1_highbd_convolve_horiz_rs_neon(const uint16_t *src, int src_stride,
                                       uint16_t *dst, int dst_stride, int w,
                                       int h, const int16_t *x_filters,
                                       int x0_qn, int x_step_qn, int bd) {
  const int horiz_offset = UPSCALE_NORMATIVE_TAPS / 2 - 1;

  const int32x4_t idx = { 0, 1, 2, 3 };
  const int32x4_t subpel_mask = vdupq_n_s32(RS_SCALE_SUBPEL_MASK);
  const int32x4_t shift_s32 = vdupq_n_s32(-FILTER_BITS);
  const int32x4_t offset_s32 = vdupq_n_s32(0);
  const uint16x4_t max = vdup_n_u16((1 << bd) - 1);

  const uint16_t *src_ptr = src - horiz_offset;
  uint16_t *dst_ptr = dst;

  if (w <= 4) {
    int height = h;
    int16x8_t s0, s1, s2, s3;
    uint16x4_t d0;

    uint16_t *d = dst_ptr;
    do {
      int x_qn = x0_qn;

      // Load 4 src vectors at a time, they might be the same, but we have to
      // calculate the indices anyway. Doing it in SIMD and then storing the
      // indices is faster than having to calculate the expression
      // &src_ptr[((x_qn + 0*x_step_qn) >> RS_SCALE_SUBPEL_BITS)] 4 times
      // Ideally this should be a gather using the indices, but NEON does not
      // have that, so have to emulate
      const int32x4_t xqn_idx = vmlaq_n_s32(vdupq_n_s32(x_qn), idx, x_step_qn);
      // We have to multiply x2 to get the actual pointer as sizeof(uint16_t) =
      // 2
      const int32x4_t src_idx =
          vshlq_n_s32(vshrq_n_s32(xqn_idx, RS_SCALE_SUBPEL_BITS), 1);
      // Similarly for the filter vector indices, we calculate the filter
      // indices for 4 columns. First we calculate the indices:
      // x_qn & RS_SCALE_SUBPEL_MASK) >> RS_SCALE_EXTRA_BITS
      // Then we calculate the actual pointers, multiplying with
      // UPSCALE_UPSCALE_NORMATIVE_TAPS
      // again shift left by 1
      const int32x4_t x_filter4_idx = vshlq_n_s32(
          vshrq_n_s32(vandq_s32(xqn_idx, subpel_mask), RS_SCALE_EXTRA_BITS), 1);
      // Even though pointers are unsigned 32/64-bit ints we do signed
      // addition The reason for this is that x_qn can be negative, leading to
      // negative offsets. Argon test
      // profile0_core/streams/test10573_11003.obu was failing because of
      // this.
#if AOM_ARCH_AARCH64
      uint64x2_t tmp4[2];
      tmp4[0] = vreinterpretq_u64_s64(vaddw_s32(
          vdupq_n_s64((const int64_t)src_ptr), vget_low_s32(src_idx)));
      tmp4[1] = vreinterpretq_u64_s64(vaddw_s32(
          vdupq_n_s64((const int64_t)src_ptr), vget_high_s32(src_idx)));
      int16_t *src4_ptr[4];
      uint64_t *tmp_ptr = (uint64_t *)&src4_ptr;
      vst1q_u64(tmp_ptr, tmp4[0]);
      vst1q_u64(tmp_ptr + 2, tmp4[1]);

      // filter vectors
      tmp4[0] = vreinterpretq_u64_s64(vmlal_s32(
          vdupq_n_s64((const int64_t)x_filters), vget_low_s32(x_filter4_idx),
          vdup_n_s32(UPSCALE_NORMATIVE_TAPS)));
      tmp4[1] = vreinterpretq_u64_s64(vmlal_s32(
          vdupq_n_s64((const int64_t)x_filters), vget_high_s32(x_filter4_idx),
          vdup_n_s32(UPSCALE_NORMATIVE_TAPS)));

      const int16_t *x_filter4_ptr[4];
      tmp_ptr = (uint64_t *)&x_filter4_ptr;
      vst1q_u64(tmp_ptr, tmp4[0]);
      vst1q_u64(tmp_ptr + 2, tmp4[1]);
#else
      uint32x4_t tmp4;
      tmp4 = vreinterpretq_u32_s32(
          vaddq_s32(vdupq_n_s32((const int32_t)src_ptr), src_idx));
      int16_t *src4_ptr[4];
      uint32_t *tmp_ptr = (uint32_t *)&src4_ptr;
      vst1q_u32(tmp_ptr, tmp4);
      // filter vectors
      tmp4 = vreinterpretq_u32_s32(
          vmlaq_s32(vdupq_n_s32((const int32_t)x_filters), x_filter4_idx,
                    vdupq_n_s32(UPSCALE_NORMATIVE_TAPS)));

      const int16_t *x_filter4_ptr[4];
      tmp_ptr = (uint32_t *)&x_filter4_ptr;
      vst1q_u32(tmp_ptr, tmp4);
#endif  // AOM_ARCH_AARCH64
      // Load source
      s0 = vld1q_s16(src4_ptr[0]);
      s1 = vld1q_s16(src4_ptr[1]);
      s2 = vld1q_s16(src4_ptr[2]);
      s3 = vld1q_s16(src4_ptr[3]);

      // Actually load the filters
      const int16x8_t x_filter0 = vld1q_s16(x_filter4_ptr[0]);
      const int16x8_t x_filter1 = vld1q_s16(x_filter4_ptr[1]);
      const int16x8_t x_filter2 = vld1q_s16(x_filter4_ptr[2]);
      const int16x8_t x_filter3 = vld1q_s16(x_filter4_ptr[3]);

      // Group low and high parts and transpose
      int16x4_t filters_lo[] = { vget_low_s16(x_filter0),
                                 vget_low_s16(x_filter1),
                                 vget_low_s16(x_filter2),
                                 vget_low_s16(x_filter3) };
      int16x4_t filters_hi[] = { vget_high_s16(x_filter0),
                                 vget_high_s16(x_filter1),
                                 vget_high_s16(x_filter2),
                                 vget_high_s16(x_filter3) };
      transpose_u16_4x4((uint16x4_t *)filters_lo);
      transpose_u16_4x4((uint16x4_t *)filters_hi);

      // Run the 2D Scale convolution
      d0 = highbd_convolve8_2d_scale_horiz4x8_s32_s16(
          s0, s1, s2, s3, filters_lo, filters_hi, shift_s32, offset_s32);

      d0 = vmin_u16(d0, max);

      if (w == 2) {
        store_u16_2x1(d + 0 * dst_stride, d0, 0);
      } else {
        vst1_u16(d + 0 * dst_stride, d0);
      }

      src_ptr += src_stride;
      d += dst_stride;
      height--;
    } while (height > 0);
  } else {
    int height = h;
    int16x8_t s0, s1, s2, s3;
    uint16x4_t d0;

    do {
      int width = w;
      int x_qn = x0_qn;
      uint16_t *d = dst_ptr;
      const uint16_t *s = src_ptr;

      do {
        // Load 4 src vectors at a time, they might be the same, but we have to
        // calculate the indices anyway. Doing it in SIMD and then storing the
        // indices is faster than having to calculate the expression
        // &src_ptr[((x_qn + 0*x_step_qn) >> RS_SCALE_SUBPEL_BITS)] 4 times
        // Ideally this should be a gather using the indices, but NEON does not
        // have that, so have to emulate
        const int32x4_t xqn_idx =
            vmlaq_n_s32(vdupq_n_s32(x_qn), idx, x_step_qn);
        // We have to multiply x2 to get the actual pointer as sizeof(uint16_t)
        // = 2
        const int32x4_t src_idx =
            vshlq_n_s32(vshrq_n_s32(xqn_idx, RS_SCALE_SUBPEL_BITS), 1);

        // Similarly for the filter vector indices, we calculate the filter
        // indices for 4 columns. First we calculate the indices:
        // x_qn & RS_SCALE_SUBPEL_MASK) >> RS_SCALE_EXTRA_BITS
        // Then we calculate the actual pointers, multiplying with
        // UPSCALE_UPSCALE_NORMATIVE_TAPS
        // again shift left by 1
        const int32x4_t x_filter4_idx = vshlq_n_s32(
            vshrq_n_s32(vandq_s32(xqn_idx, subpel_mask), RS_SCALE_EXTRA_BITS),
            1);
        // Even though pointers are unsigned 32/64-bit ints we do signed
        // addition The reason for this is that x_qn can be negative, leading to
        // negative offsets. Argon test
        // profile0_core/streams/test10573_11003.obu was failing because of
        // this.
#if AOM_ARCH_AARCH64
        uint64x2_t tmp4[2];
        tmp4[0] = vreinterpretq_u64_s64(
            vaddw_s32(vdupq_n_s64((const int64_t)s), vget_low_s32(src_idx)));
        tmp4[1] = vreinterpretq_u64_s64(
            vaddw_s32(vdupq_n_s64((const int64_t)s), vget_high_s32(src_idx)));
        int16_t *src4_ptr[4];
        uint64_t *tmp_ptr = (uint64_t *)&src4_ptr;
        vst1q_u64(tmp_ptr, tmp4[0]);
        vst1q_u64(tmp_ptr + 2, tmp4[1]);

        // filter vectors
        tmp4[0] = vreinterpretq_u64_s64(vmlal_s32(
            vdupq_n_s64((const int64_t)x_filters), vget_low_s32(x_filter4_idx),
            vdup_n_s32(UPSCALE_NORMATIVE_TAPS)));
        tmp4[1] = vreinterpretq_u64_s64(vmlal_s32(
            vdupq_n_s64((const int64_t)x_filters), vget_high_s32(x_filter4_idx),
            vdup_n_s32(UPSCALE_NORMATIVE_TAPS)));

        const int16_t *x_filter4_ptr[4];
        tmp_ptr = (uint64_t *)&x_filter4_ptr;
        vst1q_u64(tmp_ptr, tmp4[0]);
        vst1q_u64(tmp_ptr + 2, tmp4[1]);
#else
        uint32x4_t tmp4;
        tmp4 = vreinterpretq_u32_s32(
            vaddq_s32(vdupq_n_s32((const int32_t)s), src_idx));
        int16_t *src4_ptr[4];
        uint32_t *tmp_ptr = (uint32_t *)&src4_ptr;
        vst1q_u32(tmp_ptr, tmp4);
        // filter vectors
        tmp4 = vreinterpretq_u32_s32(
            vmlaq_s32(vdupq_n_s32((const int32_t)x_filters), x_filter4_idx,
                      vdupq_n_s32(UPSCALE_NORMATIVE_TAPS)));

        const int16_t *x_filter4_ptr[4];
        tmp_ptr = (uint32_t *)&x_filter4_ptr;
        vst1q_u32(tmp_ptr, tmp4);
#endif  // AOM_ARCH_AARCH64

        // Load source
        s0 = vld1q_s16(src4_ptr[0]);
        s1 = vld1q_s16(src4_ptr[1]);
        s2 = vld1q_s16(src4_ptr[2]);
        s3 = vld1q_s16(src4_ptr[3]);

        // Actually load the filters
        const int16x8_t x_filter0 = vld1q_s16(x_filter4_ptr[0]);
        const int16x8_t x_filter1 = vld1q_s16(x_filter4_ptr[1]);
        const int16x8_t x_filter2 = vld1q_s16(x_filter4_ptr[2]);
        const int16x8_t x_filter3 = vld1q_s16(x_filter4_ptr[3]);

        // Group low and high parts and transpose
        int16x4_t filters_lo[] = { vget_low_s16(x_filter0),
                                   vget_low_s16(x_filter1),
                                   vget_low_s16(x_filter2),
                                   vget_low_s16(x_filter3) };
        int16x4_t filters_hi[] = { vget_high_s16(x_filter0),
                                   vget_high_s16(x_filter1),
                                   vget_high_s16(x_filter2),
                                   vget_high_s16(x_filter3) };
        transpose_u16_4x4((uint16x4_t *)filters_lo);
        transpose_u16_4x4((uint16x4_t *)filters_hi);

        // Run the 2D Scale X convolution
        d0 = highbd_convolve8_2d_scale_horiz4x8_s32_s16(
            s0, s1, s2, s3, filters_lo, filters_hi, shift_s32, offset_s32);

        d0 = vmin_u16(d0, max);
        vst1_u16(d, d0);

        x_qn += 4 * x_step_qn;
        d += 4;
        width -= 4;
      } while (width > 0);

      src_ptr += src_stride;
      dst_ptr += dst_stride;
      height--;
    } while (height > 0);
  }
}
