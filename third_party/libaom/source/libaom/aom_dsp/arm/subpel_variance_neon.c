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

#include "config/aom_dsp_rtcd.h"
#include "config/aom_config.h"

#include "aom_ports/mem.h"
#include "aom/aom_integer.h"

#include "aom_dsp/variance.h"
#include "aom_dsp/arm/mem_neon.h"

static void var_filter_block2d_bil_w4(const uint8_t *src_ptr, uint8_t *dst_ptr,
                                      int src_stride, int pixel_step,
                                      int dst_height, int filter_offset) {
  const uint8x8_t f0 = vdup_n_u8(8 - filter_offset);
  const uint8x8_t f1 = vdup_n_u8(filter_offset);

  int i = 0;
  do {
    uint8x8_t s0 = load_unaligned_u8(src_ptr, src_stride);
    uint8x8_t s1 = load_unaligned_u8(src_ptr + pixel_step, src_stride);
    uint16x8_t blend = vmull_u8(s0, f0);
    blend = vmlal_u8(blend, s1, f1);
    uint8x8_t blend_u8 = vrshrn_n_u16(blend, 3);
    vst1_u8(dst_ptr, blend_u8);

    src_ptr += 2 * src_stride;
    dst_ptr += 2 * 4;
    i += 2;
  } while (i < dst_height);
}

static void var_filter_block2d_bil_w8(const uint8_t *src_ptr, uint8_t *dst_ptr,
                                      int src_stride, int pixel_step,
                                      int dst_height, int filter_offset) {
  const uint8x8_t f0 = vdup_n_u8(8 - filter_offset);
  const uint8x8_t f1 = vdup_n_u8(filter_offset);

  int i = 0;
  do {
    uint8x8_t s0 = vld1_u8(src_ptr);
    uint8x8_t s1 = vld1_u8(src_ptr + pixel_step);
    uint16x8_t blend = vmull_u8(s0, f0);
    blend = vmlal_u8(blend, s1, f1);
    uint8x8_t blend_u8 = vrshrn_n_u16(blend, 3);
    vst1_u8(dst_ptr, blend_u8);

    src_ptr += src_stride;
    dst_ptr += 8;
    i++;
  } while (i < dst_height);
}

static void var_filter_block2d_bil_large(const uint8_t *src_ptr,
                                         uint8_t *dst_ptr, int src_stride,
                                         int pixel_step, int dst_width,
                                         int dst_height, int filter_offset) {
  const uint8x8_t f0 = vdup_n_u8(8 - filter_offset);
  const uint8x8_t f1 = vdup_n_u8(filter_offset);

  int i = 0;
  do {
    int j = 0;
    do {
      uint8x16_t s0 = vld1q_u8(src_ptr + j);
      uint8x16_t s1 = vld1q_u8(src_ptr + j + pixel_step);
      uint16x8_t blend_l = vmull_u8(vget_low_u8(s0), f0);
      blend_l = vmlal_u8(blend_l, vget_low_u8(s1), f1);
      uint16x8_t blend_h = vmull_u8(vget_high_u8(s0), f0);
      blend_h = vmlal_u8(blend_h, vget_high_u8(s1), f1);
      uint8x16_t blend_u8 =
          vcombine_u8(vrshrn_n_u16(blend_l, 3), vrshrn_n_u16(blend_h, 3));
      vst1q_u8(dst_ptr + j, blend_u8);

      j += 16;
    } while (j < dst_width);

    src_ptr += src_stride;
    dst_ptr += dst_width;
    i++;
  } while (i < dst_height);
}

static void var_filter_block2d_bil_w16(const uint8_t *src_ptr, uint8_t *dst_ptr,
                                       int src_stride, int pixel_step,
                                       int dst_height, int filter_offset) {
  var_filter_block2d_bil_large(src_ptr, dst_ptr, src_stride, pixel_step, 16,
                               dst_height, filter_offset);
}

static void var_filter_block2d_bil_w32(const uint8_t *src_ptr, uint8_t *dst_ptr,
                                       int src_stride, int pixel_step,
                                       int dst_height, int filter_offset) {
  var_filter_block2d_bil_large(src_ptr, dst_ptr, src_stride, pixel_step, 32,
                               dst_height, filter_offset);
}

static void var_filter_block2d_bil_w64(const uint8_t *src_ptr, uint8_t *dst_ptr,
                                       int src_stride, int pixel_step,
                                       int dst_height, int filter_offset) {
  var_filter_block2d_bil_large(src_ptr, dst_ptr, src_stride, pixel_step, 64,
                               dst_height, filter_offset);
}

static void var_filter_block2d_bil_w128(const uint8_t *src_ptr,
                                        uint8_t *dst_ptr, int src_stride,
                                        int pixel_step, int dst_height,
                                        int filter_offset) {
  var_filter_block2d_bil_large(src_ptr, dst_ptr, src_stride, pixel_step, 128,
                               dst_height, filter_offset);
}

static void var_filter_block2d_avg(const uint8_t *src_ptr, uint8_t *dst_ptr,
                                   int src_stride, int pixel_step,
                                   int dst_width, int dst_height) {
  // We only specialise on the filter values for large block sizes (>= 16x16.)
  assert(dst_width >= 16 && dst_width % 16 == 0);

  int i = 0;
  do {
    int j = 0;
    do {
      uint8x16_t s0 = vld1q_u8(src_ptr + j);
      uint8x16_t s1 = vld1q_u8(src_ptr + j + pixel_step);
      uint8x16_t avg = vrhaddq_u8(s0, s1);
      vst1q_u8(dst_ptr + j, avg);

      j += 16;
    } while (j < dst_width);

    src_ptr += src_stride;
    dst_ptr += dst_width;
    i++;
  } while (i < dst_height);
}

#define SUBPEL_VARIANCE_WXH_NEON(w, h, padding)                          \
  unsigned int aom_sub_pixel_variance##w##x##h##_neon(                   \
      const uint8_t *src, int src_stride, int xoffset, int yoffset,      \
      const uint8_t *ref, int ref_stride, uint32_t *sse) {               \
    uint8_t tmp0[w * (h + padding)];                                     \
    uint8_t tmp1[w * h];                                                 \
    var_filter_block2d_bil_w##w(src, tmp0, src_stride, 1, (h + padding), \
                                xoffset);                                \
    var_filter_block2d_bil_w##w(tmp0, tmp1, w, w, h, yoffset);           \
    return aom_variance##w##x##h(tmp1, w, ref, ref_stride, sse);         \
  }

#define SPECIALIZED_SUBPEL_VARIANCE_WXH_NEON(w, h, padding)                   \
  unsigned int aom_sub_pixel_variance##w##x##h##_neon(                        \
      const uint8_t *src, int src_stride, int xoffset, int yoffset,           \
      const uint8_t *ref, int ref_stride, unsigned int *sse) {                \
    if (xoffset == 0) {                                                       \
      if (yoffset == 0) {                                                     \
        return aom_variance##w##x##h##_neon(src, src_stride, ref, ref_stride, \
                                            sse);                             \
      } else if (yoffset == 4) {                                              \
        uint8_t tmp[w * h];                                                   \
        var_filter_block2d_avg(src, tmp, src_stride, src_stride, w, h);       \
        return aom_variance##w##x##h##_neon(tmp, w, ref, ref_stride, sse);    \
      } else {                                                                \
        uint8_t tmp[w * h];                                                   \
        var_filter_block2d_bil_w##w(src, tmp, src_stride, src_stride, h,      \
                                    yoffset);                                 \
        return aom_variance##w##x##h##_neon(tmp, w, ref, ref_stride, sse);    \
      }                                                                       \
    } else if (xoffset == 4) {                                                \
      uint8_t tmp0[w * (h + padding)];                                        \
      var_filter_block2d_avg(src, tmp0, src_stride, 1, w, h + padding);       \
      if (yoffset == 0) {                                                     \
        return aom_variance##w##x##h##_neon(tmp0, w, ref, ref_stride, sse);   \
      } else if (yoffset == 4) {                                              \
        uint8_t tmp1[w * (h + padding)];                                      \
        var_filter_block2d_avg(tmp0, tmp1, w, w, w, h);                       \
        return aom_variance##w##x##h##_neon(tmp1, w, ref, ref_stride, sse);   \
      } else {                                                                \
        uint8_t tmp1[w * (h + padding)];                                      \
        var_filter_block2d_bil_w##w(tmp0, tmp1, w, w, h, yoffset);            \
        return aom_variance##w##x##h##_neon(tmp1, w, ref, ref_stride, sse);   \
      }                                                                       \
    } else {                                                                  \
      uint8_t tmp0[w * (h + padding)];                                        \
      var_filter_block2d_bil_w##w(src, tmp0, src_stride, 1, (h + padding),    \
                                  xoffset);                                   \
      if (yoffset == 0) {                                                     \
        return aom_variance##w##x##h##_neon(tmp0, w, ref, ref_stride, sse);   \
      } else if (yoffset == 4) {                                              \
        uint8_t tmp1[w * h];                                                  \
        var_filter_block2d_avg(tmp0, tmp1, w, w, w, h);                       \
        return aom_variance##w##x##h##_neon(tmp1, w, ref, ref_stride, sse);   \
      } else {                                                                \
        uint8_t tmp1[w * h];                                                  \
        var_filter_block2d_bil_w##w(tmp0, tmp1, w, w, h, yoffset);            \
        return aom_variance##w##x##h##_neon(tmp1, w, ref, ref_stride, sse);   \
      }                                                                       \
    }                                                                         \
  }

SUBPEL_VARIANCE_WXH_NEON(4, 4, 2)
SUBPEL_VARIANCE_WXH_NEON(4, 8, 2)

SUBPEL_VARIANCE_WXH_NEON(8, 4, 1)
SUBPEL_VARIANCE_WXH_NEON(8, 8, 1)
SUBPEL_VARIANCE_WXH_NEON(8, 16, 1)

SUBPEL_VARIANCE_WXH_NEON(16, 8, 1)
SPECIALIZED_SUBPEL_VARIANCE_WXH_NEON(16, 16, 1)
SPECIALIZED_SUBPEL_VARIANCE_WXH_NEON(16, 32, 1)

SPECIALIZED_SUBPEL_VARIANCE_WXH_NEON(32, 16, 1)
SPECIALIZED_SUBPEL_VARIANCE_WXH_NEON(32, 32, 1)
SPECIALIZED_SUBPEL_VARIANCE_WXH_NEON(32, 64, 1)

SPECIALIZED_SUBPEL_VARIANCE_WXH_NEON(64, 32, 1)
SPECIALIZED_SUBPEL_VARIANCE_WXH_NEON(64, 64, 1)
SPECIALIZED_SUBPEL_VARIANCE_WXH_NEON(64, 128, 1)

SPECIALIZED_SUBPEL_VARIANCE_WXH_NEON(128, 64, 1)
SPECIALIZED_SUBPEL_VARIANCE_WXH_NEON(128, 128, 1)

// Realtime mode doesn't use 4x rectangular blocks.
#if !CONFIG_REALTIME_ONLY

SUBPEL_VARIANCE_WXH_NEON(4, 16, 2)

SUBPEL_VARIANCE_WXH_NEON(8, 32, 1)

SUBPEL_VARIANCE_WXH_NEON(16, 4, 1)
SPECIALIZED_SUBPEL_VARIANCE_WXH_NEON(16, 64, 1)

SPECIALIZED_SUBPEL_VARIANCE_WXH_NEON(32, 8, 1)

SPECIALIZED_SUBPEL_VARIANCE_WXH_NEON(64, 16, 1)

#endif  // !CONFIG_REALTIME_ONLY

#undef SUBPEL_VARIANCE_WXH_NEON
#undef SPECIALIZED_SUBPEL_VARIANCE_WXH_NEON
