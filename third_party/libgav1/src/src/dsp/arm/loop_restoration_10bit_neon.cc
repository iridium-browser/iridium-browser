// Copyright 2021 The libgav1 Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/dsp/loop_restoration.h"
#include "src/utils/cpu.h"

#if LIBGAV1_ENABLE_NEON && LIBGAV1_MAX_BITDEPTH >= 10
#include <arm_neon.h>

#include <algorithm>
#include <cassert>
#include <cstdint>

#include "src/dsp/arm/common_neon.h"
#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/utils/common.h"
#include "src/utils/compiler_attributes.h"
#include "src/utils/constants.h"

namespace libgav1 {
namespace dsp {
namespace {

//------------------------------------------------------------------------------
// Wiener

// Must make a local copy of coefficients to help compiler know that they have
// no overlap with other buffers. Using 'const' keyword is not enough. Actually
// compiler doesn't make a copy, since there is enough registers in this case.
inline void PopulateWienerCoefficients(
    const RestorationUnitInfo& restoration_info, const int direction,
    int16_t filter[4]) {
  for (int i = 0; i < 4; ++i) {
    filter[i] = restoration_info.wiener_info.filter[direction][i];
  }
}

inline int32x4x2_t WienerHorizontal2(const uint16x8_t s0, const uint16x8_t s1,
                                     const int16_t filter,
                                     const int32x4x2_t sum) {
  const int16x8_t ss = vreinterpretq_s16_u16(vaddq_u16(s0, s1));
  int32x4x2_t res;
  res.val[0] = vmlal_n_s16(sum.val[0], vget_low_s16(ss), filter);
  res.val[1] = vmlal_n_s16(sum.val[1], vget_high_s16(ss), filter);
  return res;
}

inline void WienerHorizontalSum(const uint16x8_t s[3], const int16_t filter[4],
                                int32x4x2_t sum, int16_t* const wiener_buffer) {
  constexpr int offset =
      1 << (kBitdepth10 + kWienerFilterBits - kInterRoundBitsHorizontal - 1);
  constexpr int limit = (offset << 2) - 1;
  const int16x8_t s_0_2 = vreinterpretq_s16_u16(vaddq_u16(s[0], s[2]));
  const int16x8_t s_1 = vreinterpretq_s16_u16(s[1]);
  int16x4x2_t sum16;
  sum.val[0] = vmlal_n_s16(sum.val[0], vget_low_s16(s_0_2), filter[2]);
  sum.val[0] = vmlal_n_s16(sum.val[0], vget_low_s16(s_1), filter[3]);
  sum16.val[0] = vqshrn_n_s32(sum.val[0], kInterRoundBitsHorizontal);
  sum16.val[0] = vmax_s16(sum16.val[0], vdup_n_s16(-offset));
  sum16.val[0] = vmin_s16(sum16.val[0], vdup_n_s16(limit - offset));
  vst1_s16(wiener_buffer, sum16.val[0]);
  sum.val[1] = vmlal_n_s16(sum.val[1], vget_high_s16(s_0_2), filter[2]);
  sum.val[1] = vmlal_n_s16(sum.val[1], vget_high_s16(s_1), filter[3]);
  sum16.val[1] = vqshrn_n_s32(sum.val[1], kInterRoundBitsHorizontal);
  sum16.val[1] = vmax_s16(sum16.val[1], vdup_n_s16(-offset));
  sum16.val[1] = vmin_s16(sum16.val[1], vdup_n_s16(limit - offset));
  vst1_s16(wiener_buffer + 4, sum16.val[1]);
}

inline void WienerHorizontalTap7(const uint16_t* src,
                                 const ptrdiff_t src_stride,
                                 const ptrdiff_t wiener_stride,
                                 const ptrdiff_t width, const int height,
                                 const int16_t filter[4],
                                 int16_t** const wiener_buffer) {
  const ptrdiff_t src_width =
      width + ((kRestorationHorizontalBorder - 1) * sizeof(*src));
  for (int y = height; y != 0; --y) {
    const uint16_t* src_ptr = src;
    uint16x8_t s[8];
    s[0] = vld1q_u16(src_ptr);
    ptrdiff_t x = wiener_stride;
    ptrdiff_t valid_bytes = src_width * 2;
    do {
      src_ptr += 8;
      valid_bytes -= 16;
      s[7] = Load1QMsanU16(src_ptr, 16 - valid_bytes);
      s[1] = vextq_u16(s[0], s[7], 1);
      s[2] = vextq_u16(s[0], s[7], 2);
      s[3] = vextq_u16(s[0], s[7], 3);
      s[4] = vextq_u16(s[0], s[7], 4);
      s[5] = vextq_u16(s[0], s[7], 5);
      s[6] = vextq_u16(s[0], s[7], 6);
      int32x4x2_t sum;
      sum.val[0] = sum.val[1] =
          vdupq_n_s32(1 << (kInterRoundBitsHorizontal - 1));
      sum = WienerHorizontal2(s[0], s[6], filter[0], sum);
      sum = WienerHorizontal2(s[1], s[5], filter[1], sum);
      WienerHorizontalSum(s + 2, filter, sum, *wiener_buffer);
      s[0] = s[7];
      *wiener_buffer += 8;
      x -= 8;
    } while (x != 0);
    src += src_stride;
  }
}

inline void WienerHorizontalTap5(const uint16_t* src,
                                 const ptrdiff_t src_stride,
                                 const ptrdiff_t wiener_stride,
                                 const ptrdiff_t width, const int height,
                                 const int16_t filter[4],
                                 int16_t** const wiener_buffer) {
  const ptrdiff_t src_width =
      width + ((kRestorationHorizontalBorder - 1) * sizeof(*src));
  for (int y = height; y != 0; --y) {
    const uint16_t* src_ptr = src;
    uint16x8_t s[6];
    s[0] = vld1q_u16(src_ptr);
    ptrdiff_t x = wiener_stride;
    ptrdiff_t valid_bytes = src_width * 2;
    do {
      src_ptr += 8;
      valid_bytes -= 16;
      s[5] = Load1QMsanU16(src_ptr, 16 - valid_bytes);
      s[1] = vextq_u16(s[0], s[5], 1);
      s[2] = vextq_u16(s[0], s[5], 2);
      s[3] = vextq_u16(s[0], s[5], 3);
      s[4] = vextq_u16(s[0], s[5], 4);

      int32x4x2_t sum;
      sum.val[0] = sum.val[1] =
          vdupq_n_s32(1 << (kInterRoundBitsHorizontal - 1));
      sum = WienerHorizontal2(s[0], s[4], filter[1], sum);
      WienerHorizontalSum(s + 1, filter, sum, *wiener_buffer);
      s[0] = s[5];
      *wiener_buffer += 8;
      x -= 8;
    } while (x != 0);
    src += src_stride;
  }
}

inline void WienerHorizontalTap3(const uint16_t* src,
                                 const ptrdiff_t src_stride,
                                 const ptrdiff_t width, const int height,
                                 const int16_t filter[4],
                                 int16_t** const wiener_buffer) {
  for (int y = height; y != 0; --y) {
    const uint16_t* src_ptr = src;
    uint16x8_t s[3];
    ptrdiff_t x = width;
    do {
      s[0] = vld1q_u16(src_ptr);
      s[1] = vld1q_u16(src_ptr + 1);
      s[2] = vld1q_u16(src_ptr + 2);

      int32x4x2_t sum;
      sum.val[0] = sum.val[1] =
          vdupq_n_s32(1 << (kInterRoundBitsHorizontal - 1));
      WienerHorizontalSum(s, filter, sum, *wiener_buffer);
      src_ptr += 8;
      *wiener_buffer += 8;
      x -= 8;
    } while (x != 0);
    src += src_stride;
  }
}

inline void WienerHorizontalTap1(const uint16_t* src,
                                 const ptrdiff_t src_stride,
                                 const ptrdiff_t width, const int height,
                                 int16_t** const wiener_buffer) {
  for (int y = height; y != 0; --y) {
    ptrdiff_t x = 0;
    do {
      const uint16x8_t s = vld1q_u16(src + x);
      const int16x8_t d = vreinterpretq_s16_u16(vshlq_n_u16(s, 4));
      vst1q_s16(*wiener_buffer + x, d);
      x += 8;
    } while (x < width);
    src += src_stride;
    *wiener_buffer += width;
  }
}

inline int32x4x2_t WienerVertical2(const int16x8_t a0, const int16x8_t a1,
                                   const int16_t filter,
                                   const int32x4x2_t sum) {
  int32x4x2_t d;
  d.val[0] = vmlal_n_s16(sum.val[0], vget_low_s16(a0), filter);
  d.val[1] = vmlal_n_s16(sum.val[1], vget_high_s16(a0), filter);
  d.val[0] = vmlal_n_s16(d.val[0], vget_low_s16(a1), filter);
  d.val[1] = vmlal_n_s16(d.val[1], vget_high_s16(a1), filter);
  return d;
}

inline uint16x8_t WienerVertical(const int16x8_t a[3], const int16_t filter[4],
                                 const int32x4x2_t sum) {
  int32x4x2_t d = WienerVertical2(a[0], a[2], filter[2], sum);
  d.val[0] = vmlal_n_s16(d.val[0], vget_low_s16(a[1]), filter[3]);
  d.val[1] = vmlal_n_s16(d.val[1], vget_high_s16(a[1]), filter[3]);
  const uint16x4_t sum_lo_16 = vqrshrun_n_s32(d.val[0], 11);
  const uint16x4_t sum_hi_16 = vqrshrun_n_s32(d.val[1], 11);
  return vcombine_u16(sum_lo_16, sum_hi_16);
}

inline uint16x8_t WienerVerticalTap7Kernel(const int16_t* const wiener_buffer,
                                           const ptrdiff_t wiener_stride,
                                           const int16_t filter[4],
                                           int16x8_t a[7]) {
  int32x4x2_t sum;
  a[0] = vld1q_s16(wiener_buffer + 0 * wiener_stride);
  a[1] = vld1q_s16(wiener_buffer + 1 * wiener_stride);
  a[5] = vld1q_s16(wiener_buffer + 5 * wiener_stride);
  a[6] = vld1q_s16(wiener_buffer + 6 * wiener_stride);
  sum.val[0] = sum.val[1] = vdupq_n_s32(0);
  sum = WienerVertical2(a[0], a[6], filter[0], sum);
  sum = WienerVertical2(a[1], a[5], filter[1], sum);
  a[2] = vld1q_s16(wiener_buffer + 2 * wiener_stride);
  a[3] = vld1q_s16(wiener_buffer + 3 * wiener_stride);
  a[4] = vld1q_s16(wiener_buffer + 4 * wiener_stride);
  return WienerVertical(a + 2, filter, sum);
}

inline uint16x8x2_t WienerVerticalTap7Kernel2(
    const int16_t* const wiener_buffer, const ptrdiff_t wiener_stride,
    const int16_t filter[4]) {
  int16x8_t a[8];
  int32x4x2_t sum;
  uint16x8x2_t d;
  d.val[0] = WienerVerticalTap7Kernel(wiener_buffer, wiener_stride, filter, a);
  a[7] = vld1q_s16(wiener_buffer + 7 * wiener_stride);
  sum.val[0] = sum.val[1] = vdupq_n_s32(0);
  sum = WienerVertical2(a[1], a[7], filter[0], sum);
  sum = WienerVertical2(a[2], a[6], filter[1], sum);
  d.val[1] = WienerVertical(a + 3, filter, sum);
  return d;
}

inline void WienerVerticalTap7(const int16_t* wiener_buffer,
                               const ptrdiff_t width, const int height,
                               const int16_t filter[4], uint16_t* dst,
                               const ptrdiff_t dst_stride) {
  const uint16x8_t v_max_bitdepth = vdupq_n_u16((1 << kBitdepth10) - 1);
  for (int y = height >> 1; y != 0; --y) {
    uint16_t* dst_ptr = dst;
    ptrdiff_t x = width;
    do {
      uint16x8x2_t d[2];
      d[0] = WienerVerticalTap7Kernel2(wiener_buffer + 0, width, filter);
      d[1] = WienerVerticalTap7Kernel2(wiener_buffer + 8, width, filter);
      vst1q_u16(dst_ptr, vminq_u16(d[0].val[0], v_max_bitdepth));
      vst1q_u16(dst_ptr + 8, vminq_u16(d[1].val[0], v_max_bitdepth));
      vst1q_u16(dst_ptr + dst_stride, vminq_u16(d[0].val[1], v_max_bitdepth));
      vst1q_u16(dst_ptr + 8 + dst_stride,
                vminq_u16(d[1].val[1], v_max_bitdepth));
      wiener_buffer += 16;
      dst_ptr += 16;
      x -= 16;
    } while (x != 0);
    wiener_buffer += width;
    dst += 2 * dst_stride;
  }

  if ((height & 1) != 0) {
    ptrdiff_t x = width;
    do {
      int16x8_t a[7];
      const uint16x8_t d0 =
          WienerVerticalTap7Kernel(wiener_buffer + 0, width, filter, a);
      const uint16x8_t d1 =
          WienerVerticalTap7Kernel(wiener_buffer + 8, width, filter, a);
      vst1q_u16(dst, vminq_u16(d0, v_max_bitdepth));
      vst1q_u16(dst + 8, vminq_u16(d1, v_max_bitdepth));
      wiener_buffer += 16;
      dst += 16;
      x -= 16;
    } while (x != 0);
  }
}

inline uint16x8_t WienerVerticalTap5Kernel(const int16_t* const wiener_buffer,
                                           const ptrdiff_t wiener_stride,
                                           const int16_t filter[4],
                                           int16x8_t a[5]) {
  a[0] = vld1q_s16(wiener_buffer + 0 * wiener_stride);
  a[1] = vld1q_s16(wiener_buffer + 1 * wiener_stride);
  a[2] = vld1q_s16(wiener_buffer + 2 * wiener_stride);
  a[3] = vld1q_s16(wiener_buffer + 3 * wiener_stride);
  a[4] = vld1q_s16(wiener_buffer + 4 * wiener_stride);
  int32x4x2_t sum;
  sum.val[0] = sum.val[1] = vdupq_n_s32(0);
  sum = WienerVertical2(a[0], a[4], filter[1], sum);
  return WienerVertical(a + 1, filter, sum);
}

inline uint16x8x2_t WienerVerticalTap5Kernel2(
    const int16_t* const wiener_buffer, const ptrdiff_t wiener_stride,
    const int16_t filter[4]) {
  int16x8_t a[6];
  int32x4x2_t sum;
  uint16x8x2_t d;
  d.val[0] = WienerVerticalTap5Kernel(wiener_buffer, wiener_stride, filter, a);
  a[5] = vld1q_s16(wiener_buffer + 5 * wiener_stride);
  sum.val[0] = sum.val[1] = vdupq_n_s32(0);
  sum = WienerVertical2(a[1], a[5], filter[1], sum);
  d.val[1] = WienerVertical(a + 2, filter, sum);
  return d;
}

inline void WienerVerticalTap5(const int16_t* wiener_buffer,
                               const ptrdiff_t width, const int height,
                               const int16_t filter[4], uint16_t* dst,
                               const ptrdiff_t dst_stride) {
  const uint16x8_t v_max_bitdepth = vdupq_n_u16((1 << kBitdepth10) - 1);
  for (int y = height >> 1; y != 0; --y) {
    uint16_t* dst_ptr = dst;
    ptrdiff_t x = width;
    do {
      uint16x8x2_t d[2];
      d[0] = WienerVerticalTap5Kernel2(wiener_buffer + 0, width, filter);
      d[1] = WienerVerticalTap5Kernel2(wiener_buffer + 8, width, filter);
      vst1q_u16(dst_ptr, vminq_u16(d[0].val[0], v_max_bitdepth));
      vst1q_u16(dst_ptr + 8, vminq_u16(d[1].val[0], v_max_bitdepth));
      vst1q_u16(dst_ptr + dst_stride, vminq_u16(d[0].val[1], v_max_bitdepth));
      vst1q_u16(dst_ptr + 8 + dst_stride,
                vminq_u16(d[1].val[1], v_max_bitdepth));
      wiener_buffer += 16;
      dst_ptr += 16;
      x -= 16;
    } while (x != 0);
    wiener_buffer += width;
    dst += 2 * dst_stride;
  }

  if ((height & 1) != 0) {
    ptrdiff_t x = width;
    do {
      int16x8_t a[5];
      const uint16x8_t d0 =
          WienerVerticalTap5Kernel(wiener_buffer + 0, width, filter, a);
      const uint16x8_t d1 =
          WienerVerticalTap5Kernel(wiener_buffer + 8, width, filter, a);
      vst1q_u16(dst, vminq_u16(d0, v_max_bitdepth));
      vst1q_u16(dst + 8, vminq_u16(d1, v_max_bitdepth));
      wiener_buffer += 16;
      dst += 16;
      x -= 16;
    } while (x != 0);
  }
}

inline uint16x8_t WienerVerticalTap3Kernel(const int16_t* const wiener_buffer,
                                           const ptrdiff_t wiener_stride,
                                           const int16_t filter[4],
                                           int16x8_t a[3]) {
  a[0] = vld1q_s16(wiener_buffer + 0 * wiener_stride);
  a[1] = vld1q_s16(wiener_buffer + 1 * wiener_stride);
  a[2] = vld1q_s16(wiener_buffer + 2 * wiener_stride);
  int32x4x2_t sum;
  sum.val[0] = sum.val[1] = vdupq_n_s32(0);
  return WienerVertical(a, filter, sum);
}

inline uint16x8x2_t WienerVerticalTap3Kernel2(
    const int16_t* const wiener_buffer, const ptrdiff_t wiener_stride,
    const int16_t filter[4]) {
  int16x8_t a[4];
  int32x4x2_t sum;
  uint16x8x2_t d;
  d.val[0] = WienerVerticalTap3Kernel(wiener_buffer, wiener_stride, filter, a);
  a[3] = vld1q_s16(wiener_buffer + 3 * wiener_stride);
  sum.val[0] = sum.val[1] = vdupq_n_s32(0);
  d.val[1] = WienerVertical(a + 1, filter, sum);
  return d;
}

inline void WienerVerticalTap3(const int16_t* wiener_buffer,
                               const ptrdiff_t width, const int height,
                               const int16_t filter[4], uint16_t* dst,
                               const ptrdiff_t dst_stride) {
  const uint16x8_t v_max_bitdepth = vdupq_n_u16((1 << kBitdepth10) - 1);

  for (int y = height >> 1; y != 0; --y) {
    uint16_t* dst_ptr = dst;
    ptrdiff_t x = width;
    do {
      uint16x8x2_t d[2];
      d[0] = WienerVerticalTap3Kernel2(wiener_buffer + 0, width, filter);
      d[1] = WienerVerticalTap3Kernel2(wiener_buffer + 8, width, filter);

      vst1q_u16(dst_ptr, vminq_u16(d[0].val[0], v_max_bitdepth));
      vst1q_u16(dst_ptr + 8, vminq_u16(d[1].val[0], v_max_bitdepth));
      vst1q_u16(dst_ptr + dst_stride, vminq_u16(d[0].val[1], v_max_bitdepth));
      vst1q_u16(dst_ptr + 8 + dst_stride,
                vminq_u16(d[1].val[1], v_max_bitdepth));

      wiener_buffer += 16;
      dst_ptr += 16;
      x -= 16;
    } while (x != 0);
    wiener_buffer += width;
    dst += 2 * dst_stride;
  }

  if ((height & 1) != 0) {
    ptrdiff_t x = width;
    do {
      int16x8_t a[3];
      const uint16x8_t d0 =
          WienerVerticalTap3Kernel(wiener_buffer + 0, width, filter, a);
      const uint16x8_t d1 =
          WienerVerticalTap3Kernel(wiener_buffer + 8, width, filter, a);
      vst1q_u16(dst, vminq_u16(d0, v_max_bitdepth));
      vst1q_u16(dst + 8, vminq_u16(d1, v_max_bitdepth));
      wiener_buffer += 16;
      dst += 16;
      x -= 16;
    } while (x != 0);
  }
}

inline void WienerVerticalTap1Kernel(const int16_t* const wiener_buffer,
                                     uint16_t* const dst) {
  const uint16x8_t v_max_bitdepth = vdupq_n_u16((1 << kBitdepth10) - 1);
  const int16x8_t a0 = vld1q_s16(wiener_buffer + 0);
  const int16x8_t a1 = vld1q_s16(wiener_buffer + 8);
  const int16x8_t d0 = vrshrq_n_s16(a0, 4);
  const int16x8_t d1 = vrshrq_n_s16(a1, 4);
  vst1q_u16(dst, vminq_u16(vreinterpretq_u16_s16(vmaxq_s16(d0, vdupq_n_s16(0))),
                           v_max_bitdepth));
  vst1q_u16(dst + 8,
            vminq_u16(vreinterpretq_u16_s16(vmaxq_s16(d1, vdupq_n_s16(0))),
                      v_max_bitdepth));
}

inline void WienerVerticalTap1(const int16_t* wiener_buffer,
                               const ptrdiff_t width, const int height,
                               uint16_t* dst, const ptrdiff_t dst_stride) {
  for (int y = height >> 1; y != 0; --y) {
    uint16_t* dst_ptr = dst;
    ptrdiff_t x = width;
    do {
      WienerVerticalTap1Kernel(wiener_buffer, dst_ptr);
      WienerVerticalTap1Kernel(wiener_buffer + width, dst_ptr + dst_stride);
      wiener_buffer += 16;
      dst_ptr += 16;
      x -= 16;
    } while (x != 0);
    wiener_buffer += width;
    dst += 2 * dst_stride;
  }

  if ((height & 1) != 0) {
    ptrdiff_t x = width;
    do {
      WienerVerticalTap1Kernel(wiener_buffer, dst);
      wiener_buffer += 16;
      dst += 16;
      x -= 16;
    } while (x != 0);
  }
}

// For width 16 and up, store the horizontal results, and then do the vertical
// filter row by row. This is faster than doing it column by column when
// considering cache issues.
void WienerFilter_NEON(
    const RestorationUnitInfo& LIBGAV1_RESTRICT restoration_info,
    const void* LIBGAV1_RESTRICT const source, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_border,
    const ptrdiff_t top_border_stride,
    const void* LIBGAV1_RESTRICT const bottom_border,
    const ptrdiff_t bottom_border_stride, const int width, const int height,
    RestorationBuffer* LIBGAV1_RESTRICT const restoration_buffer,
    void* LIBGAV1_RESTRICT const dest) {
  const int16_t* const number_leading_zero_coefficients =
      restoration_info.wiener_info.number_leading_zero_coefficients;
  const int number_rows_to_skip = std::max(
      static_cast<int>(number_leading_zero_coefficients[WienerInfo::kVertical]),
      1);
  const ptrdiff_t wiener_stride = Align(width, 16);
  int16_t* const wiener_buffer_vertical = restoration_buffer->wiener_buffer;
  // The values are saturated to 13 bits before storing.
  int16_t* wiener_buffer_horizontal =
      wiener_buffer_vertical + number_rows_to_skip * wiener_stride;
  int16_t filter_horizontal[(kWienerFilterTaps + 1) / 2];
  int16_t filter_vertical[(kWienerFilterTaps + 1) / 2];
  PopulateWienerCoefficients(restoration_info, WienerInfo::kHorizontal,
                             filter_horizontal);
  PopulateWienerCoefficients(restoration_info, WienerInfo::kVertical,
                             filter_vertical);
  // horizontal filtering.
  const int height_horizontal =
      height + kWienerFilterTaps - 1 - 2 * number_rows_to_skip;
  const int height_extra = (height_horizontal - height) >> 1;
  assert(height_extra <= 2);
  const auto* const src = static_cast<const uint16_t*>(source);
  const auto* const top = static_cast<const uint16_t*>(top_border);
  const auto* const bottom = static_cast<const uint16_t*>(bottom_border);
  if (number_leading_zero_coefficients[WienerInfo::kHorizontal] == 0) {
    WienerHorizontalTap7(top + (2 - height_extra) * top_border_stride - 3,
                         top_border_stride, wiener_stride, width, height_extra,
                         filter_horizontal, &wiener_buffer_horizontal);
    WienerHorizontalTap7(src - 3, stride, wiener_stride, width, height,
                         filter_horizontal, &wiener_buffer_horizontal);
    WienerHorizontalTap7(bottom - 3, bottom_border_stride, wiener_stride, width,
                         height_extra, filter_horizontal,
                         &wiener_buffer_horizontal);
  } else if (number_leading_zero_coefficients[WienerInfo::kHorizontal] == 1) {
    WienerHorizontalTap5(top + (2 - height_extra) * top_border_stride - 2,
                         top_border_stride, wiener_stride, width, height_extra,
                         filter_horizontal, &wiener_buffer_horizontal);
    WienerHorizontalTap5(src - 2, stride, wiener_stride, width, height,
                         filter_horizontal, &wiener_buffer_horizontal);
    WienerHorizontalTap5(bottom - 2, bottom_border_stride, wiener_stride, width,
                         height_extra, filter_horizontal,
                         &wiener_buffer_horizontal);
  } else if (number_leading_zero_coefficients[WienerInfo::kHorizontal] == 2) {
    WienerHorizontalTap3(top + (2 - height_extra) * top_border_stride - 1,
                         top_border_stride, wiener_stride, height_extra,
                         filter_horizontal, &wiener_buffer_horizontal);
    WienerHorizontalTap3(src - 1, stride, wiener_stride, height,
                         filter_horizontal, &wiener_buffer_horizontal);
    WienerHorizontalTap3(bottom - 1, bottom_border_stride, wiener_stride,
                         height_extra, filter_horizontal,
                         &wiener_buffer_horizontal);
  } else {
    assert(number_leading_zero_coefficients[WienerInfo::kHorizontal] == 3);
    WienerHorizontalTap1(top + (2 - height_extra) * top_border_stride,
                         top_border_stride, wiener_stride, height_extra,
                         &wiener_buffer_horizontal);
    WienerHorizontalTap1(src, stride, wiener_stride, height,
                         &wiener_buffer_horizontal);
    WienerHorizontalTap1(bottom, bottom_border_stride, wiener_stride,
                         height_extra, &wiener_buffer_horizontal);
  }

  // vertical filtering.
  auto* dst = static_cast<uint16_t*>(dest);
  if (number_leading_zero_coefficients[WienerInfo::kVertical] == 0) {
    // Because the top row of |source| is a duplicate of the second row, and the
    // bottom row of |source| is a duplicate of its above row, we can duplicate
    // the top and bottom row of |wiener_buffer| accordingly.
    memcpy(wiener_buffer_horizontal, wiener_buffer_horizontal - wiener_stride,
           sizeof(*wiener_buffer_horizontal) * wiener_stride);
    memcpy(restoration_buffer->wiener_buffer,
           restoration_buffer->wiener_buffer + wiener_stride,
           sizeof(*restoration_buffer->wiener_buffer) * wiener_stride);
    WienerVerticalTap7(wiener_buffer_vertical, wiener_stride, height,
                       filter_vertical, dst, stride);
  } else if (number_leading_zero_coefficients[WienerInfo::kVertical] == 1) {
    WienerVerticalTap5(wiener_buffer_vertical + wiener_stride, wiener_stride,
                       height, filter_vertical, dst, stride);
  } else if (number_leading_zero_coefficients[WienerInfo::kVertical] == 2) {
    WienerVerticalTap3(wiener_buffer_vertical + 2 * wiener_stride,
                       wiener_stride, height, filter_vertical, dst, stride);
  } else {
    assert(number_leading_zero_coefficients[WienerInfo::kVertical] == 3);
    WienerVerticalTap1(wiener_buffer_vertical + 3 * wiener_stride,
                       wiener_stride, height, dst, stride);
  }
}

//------------------------------------------------------------------------------
// SGR

// SIMD overreads 8 - (width % 8) - 2 * padding pixels, where padding is 3 for
// Pass 1 and 2 for Pass 2.
constexpr int kOverreadInBytesPass1 = 4;
constexpr int kOverreadInBytesPass2 = 8;

inline void LoadAligned16x2U16(const uint16_t* const src[2], const ptrdiff_t x,
                               uint16x8_t dst[2]) {
  dst[0] = vld1q_u16(src[0] + x);
  dst[1] = vld1q_u16(src[1] + x);
}

inline void LoadAligned16x2U16Msan(const uint16_t* const src[2],
                                   const ptrdiff_t x, const ptrdiff_t border,
                                   uint16x8_t dst[2]) {
  dst[0] = Load1QMsanU16(src[0] + x, sizeof(**src) * (x + 8 - border));
  dst[1] = Load1QMsanU16(src[1] + x, sizeof(**src) * (x + 8 - border));
}

inline void LoadAligned16x3U16(const uint16_t* const src[3], const ptrdiff_t x,
                               uint16x8_t dst[3]) {
  dst[0] = vld1q_u16(src[0] + x);
  dst[1] = vld1q_u16(src[1] + x);
  dst[2] = vld1q_u16(src[2] + x);
}

inline void LoadAligned16x3U16Msan(const uint16_t* const src[3],
                                   const ptrdiff_t x, const ptrdiff_t border,
                                   uint16x8_t dst[3]) {
  dst[0] = Load1QMsanU16(src[0] + x, sizeof(**src) * (x + 8 - border));
  dst[1] = Load1QMsanU16(src[1] + x, sizeof(**src) * (x + 8 - border));
  dst[2] = Load1QMsanU16(src[2] + x, sizeof(**src) * (x + 8 - border));
}

inline void LoadAligned32U32(const uint32_t* const src, uint32x4_t dst[2]) {
  dst[0] = vld1q_u32(src + 0);
  dst[1] = vld1q_u32(src + 4);
}

inline void LoadAligned32U32Msan(const uint32_t* const src, const ptrdiff_t x,
                                 const ptrdiff_t border, uint32x4_t dst[2]) {
  dst[0] = Load1QMsanU32(src + x + 0, sizeof(*src) * (x + 4 - border));
  dst[1] = Load1QMsanU32(src + x + 4, sizeof(*src) * (x + 8 - border));
}

inline void LoadAligned32x2U32(const uint32_t* const src[2], const ptrdiff_t x,
                               uint32x4_t dst[2][2]) {
  LoadAligned32U32(src[0] + x, dst[0]);
  LoadAligned32U32(src[1] + x, dst[1]);
}

inline void LoadAligned32x2U32Msan(const uint32_t* const src[2],
                                   const ptrdiff_t x, const ptrdiff_t border,
                                   uint32x4_t dst[2][2]) {
  LoadAligned32U32Msan(src[0], x, border, dst[0]);
  LoadAligned32U32Msan(src[1], x, border, dst[1]);
}

inline void LoadAligned32x3U32(const uint32_t* const src[3], const ptrdiff_t x,
                               uint32x4_t dst[3][2]) {
  LoadAligned32U32(src[0] + x, dst[0]);
  LoadAligned32U32(src[1] + x, dst[1]);
  LoadAligned32U32(src[2] + x, dst[2]);
}

inline void LoadAligned32x3U32Msan(const uint32_t* const src[3],
                                   const ptrdiff_t x, const ptrdiff_t border,
                                   uint32x4_t dst[3][2]) {
  LoadAligned32U32Msan(src[0], x, border, dst[0]);
  LoadAligned32U32Msan(src[1], x, border, dst[1]);
  LoadAligned32U32Msan(src[2], x, border, dst[2]);
}

inline void StoreAligned32U16(uint16_t* const dst, const uint16x8_t src[2]) {
  vst1q_u16(dst + 0, src[0]);
  vst1q_u16(dst + 8, src[1]);
}

inline void StoreAligned32U32(uint32_t* const dst, const uint32x4_t src[2]) {
  vst1q_u32(dst + 0, src[0]);
  vst1q_u32(dst + 4, src[1]);
}

inline void StoreAligned64U32(uint32_t* const dst, const uint32x4_t src[4]) {
  StoreAligned32U32(dst + 0, src + 0);
  StoreAligned32U32(dst + 8, src + 2);
}

inline uint16x8_t VaddwLo8(const uint16x8_t src0, const uint8x16_t src1) {
  const uint8x8_t s1 = vget_low_u8(src1);
  return vaddw_u8(src0, s1);
}

inline uint16x8_t VaddwHi8(const uint16x8_t src0, const uint8x16_t src1) {
  const uint8x8_t s1 = vget_high_u8(src1);
  return vaddw_u8(src0, s1);
}

inline uint32x4_t VmullLo16(const uint16x8_t src0, const uint16x8_t src1) {
  return vmull_u16(vget_low_u16(src0), vget_low_u16(src1));
}

inline uint32x4_t VmullHi16(const uint16x8_t src0, const uint16x8_t src1) {
  return vmull_u16(vget_high_u16(src0), vget_high_u16(src1));
}

template <int bytes>
inline uint8x8_t VshrU128(const uint8x8x2_t src) {
  return vext_u8(src.val[0], src.val[1], bytes);
}

template <int bytes>
inline uint8x8_t VshrU128(const uint8x8_t src[2]) {
  return vext_u8(src[0], src[1], bytes);
}

template <int bytes>
inline uint8x16_t VshrU128(const uint8x16_t src[2]) {
  return vextq_u8(src[0], src[1], bytes);
}

template <int bytes>
inline uint16x8_t VshrU128(const uint16x8x2_t src) {
  return vextq_u16(src.val[0], src.val[1], bytes / 2);
}

template <int bytes>
inline uint16x8_t VshrU128(const uint16x8_t src[2]) {
  return vextq_u16(src[0], src[1], bytes / 2);
}

inline uint32x4_t Square(uint16x4_t s) { return vmull_u16(s, s); }

inline void Square(const uint16x8_t src, uint32x4_t dst[2]) {
  const uint16x4_t s_lo = vget_low_u16(src);
  const uint16x4_t s_hi = vget_high_u16(src);
  dst[0] = Square(s_lo);
  dst[1] = Square(s_hi);
}

template <int offset>
inline void Prepare3_8(const uint8x16_t src[2], uint8x16_t dst[3]) {
  dst[0] = VshrU128<offset + 0>(src);
  dst[1] = VshrU128<offset + 1>(src);
  dst[2] = VshrU128<offset + 2>(src);
}

inline void Prepare3_16(const uint16x8_t src[2], uint16x8_t dst[3]) {
  dst[0] = src[0];
  dst[1] = vextq_u16(src[0], src[1], 1);
  dst[2] = vextq_u16(src[0], src[1], 2);
}

template <int offset>
inline void Prepare5_8(const uint8x16_t src[2], uint8x16_t dst[5]) {
  dst[0] = VshrU128<offset + 0>(src);
  dst[1] = VshrU128<offset + 1>(src);
  dst[2] = VshrU128<offset + 2>(src);
  dst[3] = VshrU128<offset + 3>(src);
  dst[4] = VshrU128<offset + 4>(src);
}

inline void Prepare5_16(const uint16x8_t src[2], uint16x8_t dst[5]) {
  dst[0] = src[0];
  dst[1] = vextq_u16(src[0], src[1], 1);
  dst[2] = vextq_u16(src[0], src[1], 2);
  dst[3] = vextq_u16(src[0], src[1], 3);
  dst[4] = vextq_u16(src[0], src[1], 4);
}

inline void Prepare3_32(const uint32x4_t src[2], uint32x4_t dst[3]) {
  dst[0] = src[0];
  dst[1] = vextq_u32(src[0], src[1], 1);
  dst[2] = vextq_u32(src[0], src[1], 2);
}

inline void Prepare5_32(const uint32x4_t src[2], uint32x4_t dst[5]) {
  Prepare3_32(src, dst);
  dst[3] = vextq_u32(src[0], src[1], 3);
  dst[4] = src[1];
}

inline uint16x8_t Sum3WLo16(const uint8x16_t src[3]) {
  const uint16x8_t sum = vaddl_u8(vget_low_u8(src[0]), vget_low_u8(src[1]));
  return vaddw_u8(sum, vget_low_u8(src[2]));
}

inline uint16x8_t Sum3WHi16(const uint8x16_t src[3]) {
  const uint16x8_t sum = vaddl_u8(vget_high_u8(src[0]), vget_high_u8(src[1]));
  return vaddw_u8(sum, vget_high_u8(src[2]));
}

inline uint16x8_t Sum3_16(const uint16x8_t src0, const uint16x8_t src1,
                          const uint16x8_t src2) {
  const uint16x8_t sum = vaddq_u16(src0, src1);
  return vaddq_u16(sum, src2);
}

inline uint16x8_t Sum3_16(const uint16x8_t src[3]) {
  return Sum3_16(src[0], src[1], src[2]);
}

inline uint32x4_t Sum3_32(const uint32x4_t src0, const uint32x4_t src1,
                          const uint32x4_t src2) {
  const uint32x4_t sum = vaddq_u32(src0, src1);
  return vaddq_u32(sum, src2);
}

inline uint32x4_t Sum3_32(const uint32x4_t src[3]) {
  return Sum3_32(src[0], src[1], src[2]);
}

inline void Sum3_32(const uint32x4_t src[3][2], uint32x4_t dst[2]) {
  dst[0] = Sum3_32(src[0][0], src[1][0], src[2][0]);
  dst[1] = Sum3_32(src[0][1], src[1][1], src[2][1]);
}

inline uint16x8_t Sum5_16(const uint16x8_t src[5]) {
  const uint16x8_t sum01 = vaddq_u16(src[0], src[1]);
  const uint16x8_t sum23 = vaddq_u16(src[2], src[3]);
  const uint16x8_t sum = vaddq_u16(sum01, sum23);
  return vaddq_u16(sum, src[4]);
}

inline uint32x4_t Sum5_32(const uint32x4_t* src0, const uint32x4_t* src1,
                          const uint32x4_t* src2, const uint32x4_t* src3,
                          const uint32x4_t* src4) {
  const uint32x4_t sum01 = vaddq_u32(*src0, *src1);
  const uint32x4_t sum23 = vaddq_u32(*src2, *src3);
  const uint32x4_t sum = vaddq_u32(sum01, sum23);
  return vaddq_u32(sum, *src4);
}

inline uint32x4_t Sum5_32(const uint32x4_t src[5]) {
  return Sum5_32(&src[0], &src[1], &src[2], &src[3], &src[4]);
}

inline void Sum5_32(const uint32x4_t src[5][2], uint32x4_t dst[2]) {
  dst[0] = Sum5_32(&src[0][0], &src[1][0], &src[2][0], &src[3][0], &src[4][0]);
  dst[1] = Sum5_32(&src[0][1], &src[1][1], &src[2][1], &src[3][1], &src[4][1]);
}

inline uint16x8_t Sum3Horizontal16(const uint16x8_t src[2]) {
  uint16x8_t s[3];
  Prepare3_16(src, s);
  return Sum3_16(s);
}

inline void Sum3Horizontal32(const uint32x4_t src[3], uint32x4_t dst[2]) {
  uint32x4_t s[3];
  Prepare3_32(src + 0, s);
  dst[0] = Sum3_32(s);
  Prepare3_32(src + 1, s);
  dst[1] = Sum3_32(s);
}

inline uint16x8_t Sum5Horizontal16(const uint16x8_t src[2]) {
  uint16x8_t s[5];
  Prepare5_16(src, s);
  return Sum5_16(s);
}

inline void Sum5Horizontal32(const uint32x4_t src[3], uint32x4_t dst[2]) {
  uint32x4_t s[5];
  Prepare5_32(src + 0, s);
  dst[0] = Sum5_32(s);
  Prepare5_32(src + 1, s);
  dst[1] = Sum5_32(s);
}

void SumHorizontal16(const uint16x8_t src[2], uint16x8_t* const row3,
                     uint16x8_t* const row5) {
  uint16x8_t s[5];
  Prepare5_16(src, s);
  const uint16x8_t sum04 = vaddq_u16(s[0], s[4]);
  *row3 = Sum3_16(s + 1);
  *row5 = vaddq_u16(sum04, *row3);
}

inline void SumHorizontal16(const uint16x8_t src[3], uint16x8_t* const row3_0,
                            uint16x8_t* const row3_1, uint16x8_t* const row5_0,
                            uint16x8_t* const row5_1) {
  SumHorizontal16(src + 0, row3_0, row5_0);
  SumHorizontal16(src + 1, row3_1, row5_1);
}

void SumHorizontal32(const uint32x4_t src[5], uint32x4_t* const row_sq3,
                     uint32x4_t* const row_sq5) {
  const uint32x4_t sum04 = vaddq_u32(src[0], src[4]);
  *row_sq3 = Sum3_32(src + 1);
  *row_sq5 = vaddq_u32(sum04, *row_sq3);
}

inline void SumHorizontal32(const uint32x4_t src[3],
                            uint32x4_t* const row_sq3_0,
                            uint32x4_t* const row_sq3_1,
                            uint32x4_t* const row_sq5_0,
                            uint32x4_t* const row_sq5_1) {
  uint32x4_t s[5];
  Prepare5_32(src + 0, s);
  SumHorizontal32(s, row_sq3_0, row_sq5_0);
  Prepare5_32(src + 1, s);
  SumHorizontal32(s, row_sq3_1, row_sq5_1);
}

inline uint16x8_t Sum343Lo(const uint8x16_t ma3[3]) {
  const uint16x8_t sum = Sum3WLo16(ma3);
  const uint16x8_t sum3 = Sum3_16(sum, sum, sum);
  return VaddwLo8(sum3, ma3[1]);
}

inline uint16x8_t Sum343Hi(const uint8x16_t ma3[3]) {
  const uint16x8_t sum = Sum3WHi16(ma3);
  const uint16x8_t sum3 = Sum3_16(sum, sum, sum);
  return VaddwHi8(sum3, ma3[1]);
}

inline uint32x4_t Sum343(const uint32x4_t src[3]) {
  const uint32x4_t sum = Sum3_32(src);
  const uint32x4_t sum3 = Sum3_32(sum, sum, sum);
  return vaddq_u32(sum3, src[1]);
}

inline void Sum343(const uint32x4_t src[3], uint32x4_t dst[2]) {
  uint32x4_t s[3];
  Prepare3_32(src + 0, s);
  dst[0] = Sum343(s);
  Prepare3_32(src + 1, s);
  dst[1] = Sum343(s);
}

inline uint16x8_t Sum565Lo(const uint8x16_t src[3]) {
  const uint16x8_t sum = Sum3WLo16(src);
  const uint16x8_t sum4 = vshlq_n_u16(sum, 2);
  const uint16x8_t sum5 = vaddq_u16(sum4, sum);
  return VaddwLo8(sum5, src[1]);
}

inline uint16x8_t Sum565Hi(const uint8x16_t src[3]) {
  const uint16x8_t sum = Sum3WHi16(src);
  const uint16x8_t sum4 = vshlq_n_u16(sum, 2);
  const uint16x8_t sum5 = vaddq_u16(sum4, sum);
  return VaddwHi8(sum5, src[1]);
}

inline uint32x4_t Sum565(const uint32x4_t src[3]) {
  const uint32x4_t sum = Sum3_32(src);
  const uint32x4_t sum4 = vshlq_n_u32(sum, 2);
  const uint32x4_t sum5 = vaddq_u32(sum4, sum);
  return vaddq_u32(sum5, src[1]);
}

inline void Sum565(const uint32x4_t src[3], uint32x4_t dst[2]) {
  uint32x4_t s[3];
  Prepare3_32(src + 0, s);
  dst[0] = Sum565(s);
  Prepare3_32(src + 1, s);
  dst[1] = Sum565(s);
}

inline void BoxSum(const uint16_t* src, const ptrdiff_t src_stride,
                   const ptrdiff_t width, const ptrdiff_t sum_stride,
                   const ptrdiff_t sum_width, uint16_t* sum3, uint16_t* sum5,
                   uint32_t* square_sum3, uint32_t* square_sum5) {
  const ptrdiff_t overread_in_bytes =
      kOverreadInBytesPass1 - sizeof(*src) * width;
  int y = 2;
  do {
    uint16x8_t s[3];
    uint32x4_t sq[6];
    s[0] = Load1QMsanU16(src, overread_in_bytes);
    Square(s[0], sq);
    ptrdiff_t x = sum_width;
    do {
      uint16x8_t row3[2], row5[2];
      uint32x4_t row_sq3[2], row_sq5[2];
      s[1] = Load1QMsanU16(
          src + 8, overread_in_bytes + sizeof(*src) * (sum_width - x + 8));
      x -= 16;
      src += 16;
      s[2] = Load1QMsanU16(src,
                           overread_in_bytes + sizeof(*src) * (sum_width - x));
      Square(s[1], sq + 2);
      Square(s[2], sq + 4);
      SumHorizontal16(s, &row3[0], &row3[1], &row5[0], &row5[1]);
      StoreAligned32U16(sum3, row3);
      StoreAligned32U16(sum5, row5);
      SumHorizontal32(sq + 0, &row_sq3[0], &row_sq3[1], &row_sq5[0],
                      &row_sq5[1]);
      StoreAligned32U32(square_sum3 + 0, row_sq3);
      StoreAligned32U32(square_sum5 + 0, row_sq5);
      SumHorizontal32(sq + 2, &row_sq3[0], &row_sq3[1], &row_sq5[0],
                      &row_sq5[1]);
      StoreAligned32U32(square_sum3 + 8, row_sq3);
      StoreAligned32U32(square_sum5 + 8, row_sq5);
      s[0] = s[2];
      sq[0] = sq[4];
      sq[1] = sq[5];
      sum3 += 16;
      sum5 += 16;
      square_sum3 += 16;
      square_sum5 += 16;
    } while (x != 0);
    src += src_stride - sum_width;
    sum3 += sum_stride - sum_width;
    sum5 += sum_stride - sum_width;
    square_sum3 += sum_stride - sum_width;
    square_sum5 += sum_stride - sum_width;
  } while (--y != 0);
}

template <int size>
inline void BoxSum(const uint16_t* src, const ptrdiff_t src_stride,
                   const ptrdiff_t width, const ptrdiff_t sum_stride,
                   const ptrdiff_t sum_width, uint16_t* sums,
                   uint32_t* square_sums) {
  static_assert(size == 3 || size == 5, "");
  const ptrdiff_t overread_in_bytes =
      ((size == 5) ? kOverreadInBytesPass1 : kOverreadInBytesPass2) -
      sizeof(*src) * width;
  int y = 2;
  do {
    uint16x8_t s[3];
    uint32x4_t sq[6];
    s[0] = Load1QMsanU16(src, overread_in_bytes);
    Square(s[0], sq);
    ptrdiff_t x = sum_width;
    do {
      uint16x8_t row[2];
      uint32x4_t row_sq[4];
      s[1] = Load1QMsanU16(
          src + 8, overread_in_bytes + sizeof(*src) * (sum_width - x + 8));
      x -= 16;
      src += 16;
      s[2] = Load1QMsanU16(src,
                           overread_in_bytes + sizeof(*src) * (sum_width - x));
      Square(s[1], sq + 2);
      Square(s[2], sq + 4);
      if (size == 3) {
        row[0] = Sum3Horizontal16(s + 0);
        row[1] = Sum3Horizontal16(s + 1);
        Sum3Horizontal32(sq + 0, row_sq + 0);
        Sum3Horizontal32(sq + 2, row_sq + 2);
      } else {
        row[0] = Sum5Horizontal16(s + 0);
        row[1] = Sum5Horizontal16(s + 1);
        Sum5Horizontal32(sq + 0, row_sq + 0);
        Sum5Horizontal32(sq + 2, row_sq + 2);
      }
      StoreAligned32U16(sums, row);
      StoreAligned64U32(square_sums, row_sq);
      s[0] = s[2];
      sq[0] = sq[4];
      sq[1] = sq[5];
      sums += 16;
      square_sums += 16;
    } while (x != 0);
    src += src_stride - sum_width;
    sums += sum_stride - sum_width;
    square_sums += sum_stride - sum_width;
  } while (--y != 0);
}

template <int n>
inline uint16x4_t CalculateMa(const uint16x4_t sum, const uint32x4_t sum_sq,
                              const uint32_t scale) {
  // a = |sum_sq|
  // d = |sum|
  // p = (a * n < d * d) ? 0 : a * n - d * d;
  const uint32x4_t dxd = vmull_u16(sum, sum);
  const uint32x4_t axn = vmulq_n_u32(sum_sq, n);
  // Ensure |p| does not underflow by using saturating subtraction.
  const uint32x4_t p = vqsubq_u32(axn, dxd);
  const uint32x4_t pxs = vmulq_n_u32(p, scale);
  // vrshrn_n_u32() (narrowing shift) can only shift by 16 and kSgrProjScaleBits
  // is 20.
  const uint32x4_t shifted = vrshrq_n_u32(pxs, kSgrProjScaleBits);
  return vmovn_u32(shifted);
}

template <int n>
inline uint16x8_t CalculateMa(const uint16x8_t sum, const uint32x4_t sum_sq[2],
                              const uint32_t scale) {
  static_assert(n == 9 || n == 25, "");
  const uint16x8_t b = vrshrq_n_u16(sum, 2);
  const uint16x4_t sum_lo = vget_low_u16(b);
  const uint16x4_t sum_hi = vget_high_u16(b);
  const uint16x4_t z0 =
      CalculateMa<n>(sum_lo, vrshrq_n_u32(sum_sq[0], 4), scale);
  const uint16x4_t z1 =
      CalculateMa<n>(sum_hi, vrshrq_n_u32(sum_sq[1], 4), scale);
  return vcombine_u16(z0, z1);
}

inline void CalculateB5(const uint16x8_t sum, const uint16x8_t ma,
                        uint32x4_t b[2]) {
  // one_over_n == 164.
  constexpr uint32_t one_over_n =
      ((1 << kSgrProjReciprocalBits) + (25 >> 1)) / 25;
  // one_over_n_quarter == 41.
  constexpr uint32_t one_over_n_quarter = one_over_n >> 2;
  static_assert(one_over_n == one_over_n_quarter << 2, "");
  // |ma| is in range [0, 255].
  const uint32x4_t m2 = VmullLo16(ma, sum);
  const uint32x4_t m3 = VmullHi16(ma, sum);
  const uint32x4_t m0 = vmulq_n_u32(m2, one_over_n_quarter);
  const uint32x4_t m1 = vmulq_n_u32(m3, one_over_n_quarter);
  b[0] = vrshrq_n_u32(m0, kSgrProjReciprocalBits - 2);
  b[1] = vrshrq_n_u32(m1, kSgrProjReciprocalBits - 2);
}

inline void CalculateB3(const uint16x8_t sum, const uint16x8_t ma,
                        uint32x4_t b[2]) {
  // one_over_n == 455.
  constexpr uint32_t one_over_n =
      ((1 << kSgrProjReciprocalBits) + (9 >> 1)) / 9;
  const uint32x4_t m0 = VmullLo16(ma, sum);
  const uint32x4_t m1 = VmullHi16(ma, sum);
  const uint32x4_t m2 = vmulq_n_u32(m0, one_over_n);
  const uint32x4_t m3 = vmulq_n_u32(m1, one_over_n);
  b[0] = vrshrq_n_u32(m2, kSgrProjReciprocalBits);
  b[1] = vrshrq_n_u32(m3, kSgrProjReciprocalBits);
}

inline void CalculateSumAndIndex3(const uint16x8_t s3[3],
                                  const uint32x4_t sq3[3][2],
                                  const uint32_t scale, uint16x8_t* const sum,
                                  uint16x8_t* const index) {
  uint32x4_t sum_sq[2];
  *sum = Sum3_16(s3);
  Sum3_32(sq3, sum_sq);
  *index = CalculateMa<9>(*sum, sum_sq, scale);
}

inline void CalculateSumAndIndex5(const uint16x8_t s5[5],
                                  const uint32x4_t sq5[5][2],
                                  const uint32_t scale, uint16x8_t* const sum,
                                  uint16x8_t* const index) {
  uint32x4_t sum_sq[2];
  *sum = Sum5_16(s5);
  Sum5_32(sq5, sum_sq);
  *index = CalculateMa<25>(*sum, sum_sq, scale);
}

template <int n, int offset>
inline void LookupIntermediate(const uint16x8_t sum, const uint16x8_t index,
                               uint8x16_t* const ma, uint32x4_t b[2]) {
  static_assert(n == 9 || n == 25, "");
  static_assert(offset == 0 || offset == 8, "");

  const uint8x8_t idx = vqmovn_u16(index);
  uint8_t temp[8];
  vst1_u8(temp, idx);
  // offset == 0 is assumed to be the first call to this function. The value is
  // duplicated to avoid -Wuninitialized warnings under gcc.
  if (offset == 0) {
    *ma = vdupq_n_u8(kSgrMaLookup[temp[0]]);
  } else {
    *ma = vsetq_lane_u8(kSgrMaLookup[temp[0]], *ma, offset + 0);
  }
  *ma = vsetq_lane_u8(kSgrMaLookup[temp[1]], *ma, offset + 1);
  *ma = vsetq_lane_u8(kSgrMaLookup[temp[2]], *ma, offset + 2);
  *ma = vsetq_lane_u8(kSgrMaLookup[temp[3]], *ma, offset + 3);
  *ma = vsetq_lane_u8(kSgrMaLookup[temp[4]], *ma, offset + 4);
  *ma = vsetq_lane_u8(kSgrMaLookup[temp[5]], *ma, offset + 5);
  *ma = vsetq_lane_u8(kSgrMaLookup[temp[6]], *ma, offset + 6);
  *ma = vsetq_lane_u8(kSgrMaLookup[temp[7]], *ma, offset + 7);
  // b = ma * b * one_over_n
  // |ma| = [0, 255]
  // |sum| is a box sum with radius 1 or 2.
  // For the first pass radius is 2. Maximum value is 5x5x255 = 6375.
  // For the second pass radius is 1. Maximum value is 3x3x255 = 2295.
  // |one_over_n| = ((1 << kSgrProjReciprocalBits) + (n >> 1)) / n
  // When radius is 2 |n| is 25. |one_over_n| is 164.
  // When radius is 1 |n| is 9. |one_over_n| is 455.
  // |kSgrProjReciprocalBits| is 12.
  // Radius 2: 255 * 6375 * 164 >> 12 = 65088 (16 bits).
  // Radius 1: 255 * 2295 * 455 >> 12 = 65009 (16 bits).
  const uint16x8_t maq =
      vmovl_u8((offset == 0) ? vget_low_u8(*ma) : vget_high_u8(*ma));
  if (n == 9) {
    CalculateB3(sum, maq, b);
  } else {
    CalculateB5(sum, maq, b);
  }
}

inline uint8x8_t AdjustValue(const uint8x8_t value, const uint8x8_t index,
                             const int threshold) {
  const uint8x8_t thresholds = vdup_n_u8(threshold);
  const uint8x8_t offset = vcgt_u8(index, thresholds);
  // Adding 255 is equivalent to subtracting 1 for 8-bit data.
  return vadd_u8(value, offset);
}

inline uint8x8_t MaLookupAndAdjust(const uint8x8x4_t table0,
                                   const uint8x8x2_t table1,
                                   const uint16x8_t index) {
  const uint8x8_t idx = vqmovn_u16(index);
  // All elements whose indices are out of range [0, 47] are set to 0.
  uint8x8_t val = vtbl4_u8(table0, idx);  // Range [0, 31].
  // Subtract 8 to shuffle the next index range.
  const uint8x8_t sub_idx = vsub_u8(idx, vdup_n_u8(32));
  const uint8x8_t res = vtbl2_u8(table1, sub_idx);  // Range [32, 47].
  // Use OR instruction to combine shuffle results together.
  val = vorr_u8(val, res);

  // For elements whose indices are larger than 47, since they seldom change
  // values with the increase of the index, we use comparison and arithmetic
  // operations to calculate their values.
  // Elements whose indices are larger than 47 (with value 0) are set to 5.
  val = vmax_u8(val, vdup_n_u8(5));
  val = AdjustValue(val, idx, 55);   // 55 is the last index which value is 5.
  val = AdjustValue(val, idx, 72);   // 72 is the last index which value is 4.
  val = AdjustValue(val, idx, 101);  // 101 is the last index which value is 3.
  val = AdjustValue(val, idx, 169);  // 169 is the last index which value is 2.
  val = AdjustValue(val, idx, 254);  // 254 is the last index which value is 1.
  return val;
}

inline void CalculateIntermediate(const uint16x8_t sum[2],
                                  const uint16x8_t index[2],
                                  uint8x16_t* const ma, uint32x4_t b0[2],
                                  uint32x4_t b1[2]) {
  // Use table lookup to read elements whose indices are less than 48.
  // Using one uint8x8x4_t vector and one uint8x8x2_t vector is faster than
  // using two uint8x8x3_t vectors.
  uint8x8x4_t table0;
  uint8x8x2_t table1;
  table0.val[0] = vld1_u8(kSgrMaLookup + 0 * 8);
  table0.val[1] = vld1_u8(kSgrMaLookup + 1 * 8);
  table0.val[2] = vld1_u8(kSgrMaLookup + 2 * 8);
  table0.val[3] = vld1_u8(kSgrMaLookup + 3 * 8);
  table1.val[0] = vld1_u8(kSgrMaLookup + 4 * 8);
  table1.val[1] = vld1_u8(kSgrMaLookup + 5 * 8);
  const uint8x8_t ma_lo = MaLookupAndAdjust(table0, table1, index[0]);
  const uint8x8_t ma_hi = MaLookupAndAdjust(table0, table1, index[1]);
  *ma = vcombine_u8(ma_lo, ma_hi);
  // b = ma * b * one_over_n
  // |ma| = [0, 255]
  // |sum| is a box sum with radius 1 or 2.
  // For the first pass radius is 2. Maximum value is 5x5x255 = 6375.
  // For the second pass radius is 1. Maximum value is 3x3x255 = 2295.
  // |one_over_n| = ((1 << kSgrProjReciprocalBits) + (n >> 1)) / n
  // When radius is 2 |n| is 25. |one_over_n| is 164.
  // When radius is 1 |n| is 9. |one_over_n| is 455.
  // |kSgrProjReciprocalBits| is 12.
  // Radius 2: 255 * 6375 * 164 >> 12 = 65088 (16 bits).
  // Radius 1: 255 * 2295 * 455 >> 12 = 65009 (16 bits).
  const uint16x8_t maq0 = vmovl_u8(vget_low_u8(*ma));
  CalculateB3(sum[0], maq0, b0);
  const uint16x8_t maq1 = vmovl_u8(vget_high_u8(*ma));
  CalculateB3(sum[1], maq1, b1);
}

inline void CalculateIntermediate(const uint16x8_t sum[2],
                                  const uint16x8_t index[2], uint8x16_t ma[2],
                                  uint32x4_t b[4]) {
  uint8x16_t mas;
  CalculateIntermediate(sum, index, &mas, b + 0, b + 2);
  ma[0] = vcombine_u8(vget_low_u8(ma[0]), vget_low_u8(mas));
  ma[1] = vextq_u8(mas, vdupq_n_u8(0), 8);
}

template <int offset>
inline void CalculateIntermediate5(const uint16x8_t s5[5],
                                   const uint32x4_t sq5[5][2],
                                   const uint32_t scale, uint8x16_t* const ma,
                                   uint32x4_t b[2]) {
  static_assert(offset == 0 || offset == 8, "");
  uint16x8_t sum, index;
  CalculateSumAndIndex5(s5, sq5, scale, &sum, &index);
  LookupIntermediate<25, offset>(sum, index, ma, b);
}

inline void CalculateIntermediate3(const uint16x8_t s3[3],
                                   const uint32x4_t sq3[3][2],
                                   const uint32_t scale, uint8x16_t* const ma,
                                   uint32x4_t b[2]) {
  uint16x8_t sum, index;
  CalculateSumAndIndex3(s3, sq3, scale, &sum, &index);
  LookupIntermediate<9, 0>(sum, index, ma, b);
}

inline void Store343_444(const uint32x4_t b3[3], const ptrdiff_t x,
                         uint32x4_t sum_b343[2], uint32x4_t sum_b444[2],
                         uint32_t* const b343, uint32_t* const b444) {
  uint32x4_t b[3], sum_b111[2];
  Prepare3_32(b3 + 0, b);
  sum_b111[0] = Sum3_32(b);
  sum_b444[0] = vshlq_n_u32(sum_b111[0], 2);
  sum_b343[0] = vsubq_u32(sum_b444[0], sum_b111[0]);
  sum_b343[0] = vaddq_u32(sum_b343[0], b[1]);
  Prepare3_32(b3 + 1, b);
  sum_b111[1] = Sum3_32(b);
  sum_b444[1] = vshlq_n_u32(sum_b111[1], 2);
  sum_b343[1] = vsubq_u32(sum_b444[1], sum_b111[1]);
  sum_b343[1] = vaddq_u32(sum_b343[1], b[1]);
  StoreAligned32U32(b444 + x, sum_b444);
  StoreAligned32U32(b343 + x, sum_b343);
}

inline void Store343_444Lo(const uint8x16_t ma3[3], const uint32x4_t b3[3],
                           const ptrdiff_t x, uint16x8_t* const sum_ma343,
                           uint16x8_t* const sum_ma444, uint32x4_t sum_b343[2],
                           uint32x4_t sum_b444[2], uint16_t* const ma343,
                           uint16_t* const ma444, uint32_t* const b343,
                           uint32_t* const b444) {
  const uint16x8_t sum_ma111 = Sum3WLo16(ma3);
  *sum_ma444 = vshlq_n_u16(sum_ma111, 2);
  vst1q_u16(ma444 + x, *sum_ma444);
  const uint16x8_t sum333 = vsubq_u16(*sum_ma444, sum_ma111);
  *sum_ma343 = VaddwLo8(sum333, ma3[1]);
  vst1q_u16(ma343 + x, *sum_ma343);
  Store343_444(b3, x, sum_b343, sum_b444, b343, b444);
}

inline void Store343_444Hi(const uint8x16_t ma3[3], const uint32x4_t b3[2],
                           const ptrdiff_t x, uint16x8_t* const sum_ma343,
                           uint16x8_t* const sum_ma444, uint32x4_t sum_b343[2],
                           uint32x4_t sum_b444[2], uint16_t* const ma343,
                           uint16_t* const ma444, uint32_t* const b343,
                           uint32_t* const b444) {
  const uint16x8_t sum_ma111 = Sum3WHi16(ma3);
  *sum_ma444 = vshlq_n_u16(sum_ma111, 2);
  vst1q_u16(ma444 + x, *sum_ma444);
  const uint16x8_t sum333 = vsubq_u16(*sum_ma444, sum_ma111);
  *sum_ma343 = VaddwHi8(sum333, ma3[1]);
  vst1q_u16(ma343 + x, *sum_ma343);
  Store343_444(b3, x, sum_b343, sum_b444, b343, b444);
}

inline void Store343_444Lo(const uint8x16_t ma3[3], const uint32x4_t b3[2],
                           const ptrdiff_t x, uint16x8_t* const sum_ma343,
                           uint32x4_t sum_b343[2], uint16_t* const ma343,
                           uint16_t* const ma444, uint32_t* const b343,
                           uint32_t* const b444) {
  uint16x8_t sum_ma444;
  uint32x4_t sum_b444[2];
  Store343_444Lo(ma3, b3, x, sum_ma343, &sum_ma444, sum_b343, sum_b444, ma343,
                 ma444, b343, b444);
}

inline void Store343_444Hi(const uint8x16_t ma3[3], const uint32x4_t b3[2],
                           const ptrdiff_t x, uint16x8_t* const sum_ma343,
                           uint32x4_t sum_b343[2], uint16_t* const ma343,
                           uint16_t* const ma444, uint32_t* const b343,
                           uint32_t* const b444) {
  uint16x8_t sum_ma444;
  uint32x4_t sum_b444[2];
  Store343_444Hi(ma3, b3, x, sum_ma343, &sum_ma444, sum_b343, sum_b444, ma343,
                 ma444, b343, b444);
}

inline void Store343_444Lo(const uint8x16_t ma3[3], const uint32x4_t b3[2],
                           const ptrdiff_t x, uint16_t* const ma343,
                           uint16_t* const ma444, uint32_t* const b343,
                           uint32_t* const b444) {
  uint16x8_t sum_ma343;
  uint32x4_t sum_b343[2];
  Store343_444Lo(ma3, b3, x, &sum_ma343, sum_b343, ma343, ma444, b343, b444);
}

inline void Store343_444Hi(const uint8x16_t ma3[3], const uint32x4_t b3[2],
                           const ptrdiff_t x, uint16_t* const ma343,
                           uint16_t* const ma444, uint32_t* const b343,
                           uint32_t* const b444) {
  uint16x8_t sum_ma343;
  uint32x4_t sum_b343[2];
  Store343_444Hi(ma3, b3, x, &sum_ma343, sum_b343, ma343, ma444, b343, b444);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPreProcess5Lo(
    const uint16x8_t s[2][4], const uint32_t scale, uint16_t* const sum5[5],
    uint32_t* const square_sum5[5], uint32x4_t sq[2][8], uint8x16_t* const ma,
    uint32x4_t b[2]) {
  uint16x8_t s5[2][5];
  uint32x4_t sq5[5][2];
  Square(s[0][1], sq[0] + 2);
  Square(s[1][1], sq[1] + 2);
  s5[0][3] = Sum5Horizontal16(s[0]);
  vst1q_u16(sum5[3], s5[0][3]);
  s5[0][4] = Sum5Horizontal16(s[1]);
  vst1q_u16(sum5[4], s5[0][4]);
  Sum5Horizontal32(sq[0], sq5[3]);
  StoreAligned32U32(square_sum5[3], sq5[3]);
  Sum5Horizontal32(sq[1], sq5[4]);
  StoreAligned32U32(square_sum5[4], sq5[4]);
  LoadAligned16x3U16(sum5, 0, s5[0]);
  LoadAligned32x3U32(square_sum5, 0, sq5);
  CalculateIntermediate5<0>(s5[0], sq5, scale, ma, b);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPreProcess5(
    const uint16x8_t s[2][4], const ptrdiff_t sum_width, const ptrdiff_t x,
    const uint32_t scale, uint16_t* const sum5[5],
    uint32_t* const square_sum5[5], uint32x4_t sq[2][8], uint8x16_t ma[2],
    uint32x4_t b[6]) {
  uint16x8_t s5[2][5];
  uint32x4_t sq5[5][2];
  Square(s[0][2], sq[0] + 4);
  Square(s[1][2], sq[1] + 4);
  s5[0][3] = Sum5Horizontal16(s[0] + 1);
  s5[1][3] = Sum5Horizontal16(s[0] + 2);
  vst1q_u16(sum5[3] + x + 0, s5[0][3]);
  vst1q_u16(sum5[3] + x + 8, s5[1][3]);
  s5[0][4] = Sum5Horizontal16(s[1] + 1);
  s5[1][4] = Sum5Horizontal16(s[1] + 2);
  vst1q_u16(sum5[4] + x + 0, s5[0][4]);
  vst1q_u16(sum5[4] + x + 8, s5[1][4]);
  Sum5Horizontal32(sq[0] + 2, sq5[3]);
  StoreAligned32U32(square_sum5[3] + x, sq5[3]);
  Sum5Horizontal32(sq[1] + 2, sq5[4]);
  StoreAligned32U32(square_sum5[4] + x, sq5[4]);
  LoadAligned16x3U16(sum5, x, s5[0]);
  LoadAligned32x3U32(square_sum5, x, sq5);
  CalculateIntermediate5<8>(s5[0], sq5, scale, &ma[0], b + 2);

  Square(s[0][3], sq[0] + 6);
  Square(s[1][3], sq[1] + 6);
  Sum5Horizontal32(sq[0] + 4, sq5[3]);
  StoreAligned32U32(square_sum5[3] + x + 8, sq5[3]);
  Sum5Horizontal32(sq[1] + 4, sq5[4]);
  StoreAligned32U32(square_sum5[4] + x + 8, sq5[4]);
  LoadAligned16x3U16Msan(sum5, x + 8, sum_width, s5[1]);
  LoadAligned32x3U32Msan(square_sum5, x + 8, sum_width, sq5);
  CalculateIntermediate5<0>(s5[1], sq5, scale, &ma[1], b + 4);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPreProcess5LastRowLo(
    const uint16x8_t s[2], const uint32_t scale, const uint16_t* const sum5[5],
    const uint32_t* const square_sum5[5], uint32x4_t sq[4],
    uint8x16_t* const ma, uint32x4_t b[2]) {
  uint16x8_t s5[5];
  uint32x4_t sq5[5][2];
  Square(s[1], sq + 2);
  s5[3] = s5[4] = Sum5Horizontal16(s);
  Sum5Horizontal32(sq, sq5[3]);
  sq5[4][0] = sq5[3][0];
  sq5[4][1] = sq5[3][1];
  LoadAligned16x3U16(sum5, 0, s5);
  LoadAligned32x3U32(square_sum5, 0, sq5);
  CalculateIntermediate5<0>(s5, sq5, scale, ma, b);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPreProcess5LastRow(
    const uint16x8_t s[4], const ptrdiff_t sum_width, const ptrdiff_t x,
    const uint32_t scale, const uint16_t* const sum5[5],
    const uint32_t* const square_sum5[5], uint32x4_t sq[8], uint8x16_t ma[2],
    uint32x4_t b[6]) {
  uint16x8_t s5[2][5];
  uint32x4_t sq5[5][2];
  Square(s[2], sq + 4);
  s5[0][3] = Sum5Horizontal16(s + 1);
  s5[1][3] = Sum5Horizontal16(s + 2);
  s5[0][4] = s5[0][3];
  s5[1][4] = s5[1][3];
  Sum5Horizontal32(sq + 2, sq5[3]);
  sq5[4][0] = sq5[3][0];
  sq5[4][1] = sq5[3][1];
  LoadAligned16x3U16(sum5, x, s5[0]);
  LoadAligned32x3U32(square_sum5, x, sq5);
  CalculateIntermediate5<8>(s5[0], sq5, scale, &ma[0], b + 2);

  Square(s[3], sq + 6);
  Sum5Horizontal32(sq + 4, sq5[3]);
  sq5[4][0] = sq5[3][0];
  sq5[4][1] = sq5[3][1];
  LoadAligned16x3U16Msan(sum5, x + 8, sum_width, s5[1]);
  LoadAligned32x3U32Msan(square_sum5, x + 8, sum_width, sq5);
  CalculateIntermediate5<0>(s5[1], sq5, scale, &ma[1], b + 4);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPreProcess3Lo(
    const uint16x8_t s[2], const uint32_t scale, uint16_t* const sum3[3],
    uint32_t* const square_sum3[3], uint32x4_t sq[4], uint8x16_t* const ma,
    uint32x4_t b[2]) {
  uint16x8_t s3[3];
  uint32x4_t sq3[3][2];
  Square(s[1], sq + 2);
  s3[2] = Sum3Horizontal16(s);
  vst1q_u16(sum3[2], s3[2]);
  Sum3Horizontal32(sq, sq3[2]);
  StoreAligned32U32(square_sum3[2], sq3[2]);
  LoadAligned16x2U16(sum3, 0, s3);
  LoadAligned32x2U32(square_sum3, 0, sq3);
  CalculateIntermediate3(s3, sq3, scale, ma, b);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPreProcess3(
    const uint16x8_t s[4], const ptrdiff_t x, const ptrdiff_t sum_width,
    const uint32_t scale, uint16_t* const sum3[3],
    uint32_t* const square_sum3[3], uint32x4_t sq[8], uint8x16_t ma[2],
    uint32x4_t b[6]) {
  uint16x8_t s3[4], sum[2], index[2];
  uint32x4_t sq3[3][2];

  Square(s[2], sq + 4);
  s3[2] = Sum3Horizontal16(s + 1);
  s3[3] = Sum3Horizontal16(s + 2);
  StoreAligned32U16(sum3[2] + x, s3 + 2);
  Sum3Horizontal32(sq + 2, sq3[2]);
  StoreAligned32U32(square_sum3[2] + x + 0, sq3[2]);
  LoadAligned16x2U16(sum3, x, s3);
  LoadAligned32x2U32(square_sum3, x, sq3);
  CalculateSumAndIndex3(s3, sq3, scale, &sum[0], &index[0]);

  Square(s[3], sq + 6);
  Sum3Horizontal32(sq + 4, sq3[2]);
  StoreAligned32U32(square_sum3[2] + x + 8, sq3[2]);
  LoadAligned16x2U16Msan(sum3, x + 8, sum_width, s3 + 1);
  LoadAligned32x2U32Msan(square_sum3, x + 8, sum_width, sq3);
  CalculateSumAndIndex3(s3 + 1, sq3, scale, &sum[1], &index[1]);
  CalculateIntermediate(sum, index, ma, b + 2);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPreProcessLo(
    const uint16x8_t s[2][4], const uint16_t scales[2], uint16_t* const sum3[4],
    uint16_t* const sum5[5], uint32_t* const square_sum3[4],
    uint32_t* const square_sum5[5], uint32x4_t sq[2][8], uint8x16_t ma3[2][2],
    uint32x4_t b3[2][6], uint8x16_t* const ma5, uint32x4_t b5[2]) {
  uint16x8_t s3[4], s5[5], sum[2], index[2];
  uint32x4_t sq3[4][2], sq5[5][2];

  Square(s[0][1], sq[0] + 2);
  Square(s[1][1], sq[1] + 2);
  SumHorizontal16(s[0], &s3[2], &s5[3]);
  SumHorizontal16(s[1], &s3[3], &s5[4]);
  vst1q_u16(sum3[2], s3[2]);
  vst1q_u16(sum3[3], s3[3]);
  vst1q_u16(sum5[3], s5[3]);
  vst1q_u16(sum5[4], s5[4]);
  SumHorizontal32(sq[0], &sq3[2][0], &sq3[2][1], &sq5[3][0], &sq5[3][1]);
  StoreAligned32U32(square_sum3[2], sq3[2]);
  StoreAligned32U32(square_sum5[3], sq5[3]);
  SumHorizontal32(sq[1], &sq3[3][0], &sq3[3][1], &sq5[4][0], &sq5[4][1]);
  StoreAligned32U32(square_sum3[3], sq3[3]);
  StoreAligned32U32(square_sum5[4], sq5[4]);
  LoadAligned16x2U16(sum3, 0, s3);
  LoadAligned32x2U32(square_sum3, 0, sq3);
  LoadAligned16x3U16(sum5, 0, s5);
  LoadAligned32x3U32(square_sum5, 0, sq5);
  CalculateSumAndIndex3(s3 + 0, sq3 + 0, scales[1], &sum[0], &index[0]);
  CalculateSumAndIndex3(s3 + 1, sq3 + 1, scales[1], &sum[1], &index[1]);
  CalculateIntermediate(sum, index, &ma3[0][0], b3[0], b3[1]);
  ma3[1][0] = vextq_u8(ma3[0][0], vdupq_n_u8(0), 8);
  CalculateIntermediate5<0>(s5, sq5, scales[0], ma5, b5);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPreProcess(
    const uint16x8_t s[2][4], const ptrdiff_t x, const uint16_t scales[2],
    uint16_t* const sum3[4], uint16_t* const sum5[5],
    uint32_t* const square_sum3[4], uint32_t* const square_sum5[5],
    const ptrdiff_t sum_width, uint32x4_t sq[2][8], uint8x16_t ma3[2][2],
    uint32x4_t b3[2][6], uint8x16_t ma5[2], uint32x4_t b5[6]) {
  uint16x8_t s3[2][4], s5[2][5], sum[2][2], index[2][2];
  uint32x4_t sq3[4][2], sq5[5][2];

  SumHorizontal16(s[0] + 1, &s3[0][2], &s3[1][2], &s5[0][3], &s5[1][3]);
  vst1q_u16(sum3[2] + x + 0, s3[0][2]);
  vst1q_u16(sum3[2] + x + 8, s3[1][2]);
  vst1q_u16(sum5[3] + x + 0, s5[0][3]);
  vst1q_u16(sum5[3] + x + 8, s5[1][3]);
  SumHorizontal16(s[1] + 1, &s3[0][3], &s3[1][3], &s5[0][4], &s5[1][4]);
  vst1q_u16(sum3[3] + x + 0, s3[0][3]);
  vst1q_u16(sum3[3] + x + 8, s3[1][3]);
  vst1q_u16(sum5[4] + x + 0, s5[0][4]);
  vst1q_u16(sum5[4] + x + 8, s5[1][4]);
  Square(s[0][2], sq[0] + 4);
  Square(s[1][2], sq[1] + 4);
  SumHorizontal32(sq[0] + 2, &sq3[2][0], &sq3[2][1], &sq5[3][0], &sq5[3][1]);
  StoreAligned32U32(square_sum3[2] + x, sq3[2]);
  StoreAligned32U32(square_sum5[3] + x, sq5[3]);
  SumHorizontal32(sq[1] + 2, &sq3[3][0], &sq3[3][1], &sq5[4][0], &sq5[4][1]);
  StoreAligned32U32(square_sum3[3] + x, sq3[3]);
  StoreAligned32U32(square_sum5[4] + x, sq5[4]);
  LoadAligned16x2U16(sum3, x, s3[0]);
  LoadAligned32x2U32(square_sum3, x, sq3);
  CalculateSumAndIndex3(s3[0], sq3, scales[1], &sum[0][0], &index[0][0]);
  CalculateSumAndIndex3(s3[0] + 1, sq3 + 1, scales[1], &sum[1][0],
                        &index[1][0]);
  LoadAligned16x3U16(sum5, x, s5[0]);
  LoadAligned32x3U32(square_sum5, x, sq5);
  CalculateIntermediate5<8>(s5[0], sq5, scales[0], &ma5[0], b5 + 2);

  Square(s[0][3], sq[0] + 6);
  Square(s[1][3], sq[1] + 6);
  SumHorizontal32(sq[0] + 4, &sq3[2][0], &sq3[2][1], &sq5[3][0], &sq5[3][1]);
  StoreAligned32U32(square_sum3[2] + x + 8, sq3[2]);
  StoreAligned32U32(square_sum5[3] + x + 8, sq5[3]);
  SumHorizontal32(sq[1] + 4, &sq3[3][0], &sq3[3][1], &sq5[4][0], &sq5[4][1]);
  StoreAligned32U32(square_sum3[3] + x + 8, sq3[3]);
  StoreAligned32U32(square_sum5[4] + x + 8, sq5[4]);
  LoadAligned16x2U16Msan(sum3, x + 8, sum_width, s3[1]);
  LoadAligned32x2U32Msan(square_sum3, x + 8, sum_width, sq3);
  CalculateSumAndIndex3(s3[1], sq3, scales[1], &sum[0][1], &index[0][1]);
  CalculateSumAndIndex3(s3[1] + 1, sq3 + 1, scales[1], &sum[1][1],
                        &index[1][1]);
  CalculateIntermediate(sum[0], index[0], ma3[0], b3[0] + 2);
  CalculateIntermediate(sum[1], index[1], ma3[1], b3[1] + 2);
  LoadAligned16x3U16Msan(sum5, x + 8, sum_width, s5[1]);
  LoadAligned32x3U32Msan(square_sum5, x + 8, sum_width, sq5);
  CalculateIntermediate5<0>(s5[1], sq5, scales[0], &ma5[1], b5 + 4);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPreProcessLastRowLo(
    const uint16x8_t s[2], const uint16_t scales[2],
    const uint16_t* const sum3[4], const uint16_t* const sum5[5],
    const uint32_t* const square_sum3[4], const uint32_t* const square_sum5[5],
    uint32x4_t sq[4], uint8x16_t* const ma3, uint8x16_t* const ma5,
    uint32x4_t b3[2], uint32x4_t b5[2]) {
  uint16x8_t s3[3], s5[5];
  uint32x4_t sq3[3][2], sq5[5][2];

  Square(s[1], sq + 2);
  SumHorizontal16(s, &s3[2], &s5[3]);
  SumHorizontal32(sq, &sq3[2][0], &sq3[2][1], &sq5[3][0], &sq5[3][1]);
  LoadAligned16x3U16(sum5, 0, s5);
  s5[4] = s5[3];
  LoadAligned32x3U32(square_sum5, 0, sq5);
  sq5[4][0] = sq5[3][0];
  sq5[4][1] = sq5[3][1];
  CalculateIntermediate5<0>(s5, sq5, scales[0], ma5, b5);
  LoadAligned16x2U16(sum3, 0, s3);
  LoadAligned32x2U32(square_sum3, 0, sq3);
  CalculateIntermediate3(s3, sq3, scales[1], ma3, b3);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPreProcessLastRow(
    const uint16x8_t s[4], const ptrdiff_t sum_width, const ptrdiff_t x,
    const uint16_t scales[2], const uint16_t* const sum3[4],
    const uint16_t* const sum5[5], const uint32_t* const square_sum3[4],
    const uint32_t* const square_sum5[5], uint32x4_t sq[8], uint8x16_t ma3[2],
    uint8x16_t ma5[2], uint32x4_t b3[6], uint32x4_t b5[6]) {
  uint16x8_t s3[2][3], s5[2][5], sum[2], index[2];
  uint32x4_t sq3[3][2], sq5[5][2];

  Square(s[2], sq + 4);
  SumHorizontal16(s + 1, &s3[0][2], &s3[1][2], &s5[0][3], &s5[1][3]);
  SumHorizontal32(sq + 2, &sq3[2][0], &sq3[2][1], &sq5[3][0], &sq5[3][1]);
  LoadAligned16x3U16(sum5, x, s5[0]);
  s5[0][4] = s5[0][3];
  LoadAligned32x3U32(square_sum5, x, sq5);
  sq5[4][0] = sq5[3][0];
  sq5[4][1] = sq5[3][1];
  CalculateIntermediate5<8>(s5[0], sq5, scales[0], ma5, b5 + 2);
  LoadAligned16x2U16(sum3, x, s3[0]);
  LoadAligned32x2U32(square_sum3, x, sq3);
  CalculateSumAndIndex3(s3[0], sq3, scales[1], &sum[0], &index[0]);

  Square(s[3], sq + 6);
  SumHorizontal32(sq + 4, &sq3[2][0], &sq3[2][1], &sq5[3][0], &sq5[3][1]);
  LoadAligned16x3U16Msan(sum5, x + 8, sum_width, s5[1]);
  s5[1][4] = s5[1][3];
  LoadAligned32x3U32Msan(square_sum5, x + 8, sum_width, sq5);
  sq5[4][0] = sq5[3][0];
  sq5[4][1] = sq5[3][1];
  CalculateIntermediate5<0>(s5[1], sq5, scales[0], ma5 + 1, b5 + 4);
  LoadAligned16x2U16Msan(sum3, x + 8, sum_width, s3[1]);
  LoadAligned32x2U32Msan(square_sum3, x + 8, sum_width, sq3);
  CalculateSumAndIndex3(s3[1], sq3, scales[1], &sum[1], &index[1]);
  CalculateIntermediate(sum, index, ma3, b3 + 2);
}

inline void BoxSumFilterPreProcess5(const uint16_t* const src0,
                                    const uint16_t* const src1, const int width,
                                    const uint32_t scale,
                                    uint16_t* const sum5[5],
                                    uint32_t* const square_sum5[5],
                                    const ptrdiff_t sum_width, uint16_t* ma565,
                                    uint32_t* b565) {
  const ptrdiff_t overread_in_bytes =
      kOverreadInBytesPass1 - sizeof(*src0) * width;
  uint16x8_t s[2][4];
  uint8x16_t mas[2];
  uint32x4_t sq[2][8], bs[6];

  s[0][0] = Load1QMsanU16(src0 + 0, overread_in_bytes + 0);
  s[0][1] = Load1QMsanU16(src0 + 8, overread_in_bytes + 16);
  s[1][0] = Load1QMsanU16(src1 + 0, overread_in_bytes + 0);
  s[1][1] = Load1QMsanU16(src1 + 8, overread_in_bytes + 16);
  Square(s[0][0], sq[0]);
  Square(s[1][0], sq[1]);
  BoxFilterPreProcess5Lo(s, scale, sum5, square_sum5, sq, &mas[0], bs);

  int x = 0;
  do {
    uint8x16_t ma5[3];
    uint16x8_t ma[2];
    uint32x4_t b[4];

    s[0][2] = Load1QMsanU16(src0 + x + 16,
                            overread_in_bytes + sizeof(*src0) * (x + 16));
    s[0][3] = Load1QMsanU16(src0 + x + 24,
                            overread_in_bytes + sizeof(*src0) * (x + 24));
    s[1][2] = Load1QMsanU16(src1 + x + 16,
                            overread_in_bytes + sizeof(*src1) * (x + 16));
    s[1][3] = Load1QMsanU16(src1 + x + 24,
                            overread_in_bytes + sizeof(*src1) * (x + 24));

    BoxFilterPreProcess5(s, sum_width, x + 8, scale, sum5, square_sum5, sq, mas,
                         bs);
    Prepare3_8<0>(mas, ma5);
    ma[0] = Sum565Lo(ma5);
    ma[1] = Sum565Hi(ma5);
    StoreAligned32U16(ma565, ma);
    Sum565(bs + 0, b + 0);
    Sum565(bs + 2, b + 2);
    StoreAligned64U32(b565, b);
    s[0][0] = s[0][2];
    s[0][1] = s[0][3];
    s[1][0] = s[1][2];
    s[1][1] = s[1][3];
    sq[0][2] = sq[0][6];
    sq[0][3] = sq[0][7];
    sq[1][2] = sq[1][6];
    sq[1][3] = sq[1][7];
    mas[0] = mas[1];
    bs[0] = bs[4];
    bs[1] = bs[5];
    ma565 += 16;
    b565 += 16;
    x += 16;
  } while (x < width);
}

template <bool calculate444>
LIBGAV1_ALWAYS_INLINE void BoxSumFilterPreProcess3(
    const uint16_t* const src, const int width, const uint32_t scale,
    uint16_t* const sum3[3], uint32_t* const square_sum3[3],
    const ptrdiff_t sum_width, uint16_t* ma343, uint16_t* ma444, uint32_t* b343,
    uint32_t* b444) {
  const ptrdiff_t overread_in_bytes =
      kOverreadInBytesPass2 - sizeof(*src) * width;
  uint16x8_t s[4];
  uint8x16_t mas[2];
  uint32x4_t sq[8], bs[6];

  s[0] = Load1QMsanU16(src + 0, overread_in_bytes + 0);
  s[1] = Load1QMsanU16(src + 8, overread_in_bytes + 16);
  Square(s[0], sq);
  BoxFilterPreProcess3Lo(s, scale, sum3, square_sum3, sq, &mas[0], bs);

  int x = 0;
  do {
    s[2] = Load1QMsanU16(src + x + 16,
                         overread_in_bytes + sizeof(*src) * (x + 16));
    s[3] = Load1QMsanU16(src + x + 24,
                         overread_in_bytes + sizeof(*src) * (x + 24));
    BoxFilterPreProcess3(s, x + 8, sum_width, scale, sum3, square_sum3, sq, mas,
                         bs);
    uint8x16_t ma3[3];
    Prepare3_8<0>(mas, ma3);
    if (calculate444) {  // NOLINT(readability-simplify-boolean-expr)
      Store343_444Lo(ma3, bs + 0, 0, ma343, ma444, b343, b444);
      Store343_444Hi(ma3, bs + 2, 8, ma343, ma444, b343, b444);
      ma444 += 16;
      b444 += 16;
    } else {
      uint16x8_t ma[2];
      uint32x4_t b[4];
      ma[0] = Sum343Lo(ma3);
      ma[1] = Sum343Hi(ma3);
      StoreAligned32U16(ma343, ma);
      Sum343(bs + 0, b + 0);
      Sum343(bs + 2, b + 2);
      StoreAligned64U32(b343, b);
    }
    s[1] = s[3];
    sq[2] = sq[6];
    sq[3] = sq[7];
    mas[0] = mas[1];
    bs[0] = bs[4];
    bs[1] = bs[5];
    ma343 += 16;
    b343 += 16;
    x += 16;
  } while (x < width);
}

inline void BoxSumFilterPreProcess(
    const uint16_t* const src0, const uint16_t* const src1, const int width,
    const uint16_t scales[2], uint16_t* const sum3[4], uint16_t* const sum5[5],
    uint32_t* const square_sum3[4], uint32_t* const square_sum5[5],
    const ptrdiff_t sum_width, uint16_t* const ma343[4], uint16_t* const ma444,
    uint16_t* ma565, uint32_t* const b343[4], uint32_t* const b444,
    uint32_t* b565) {
  const ptrdiff_t overread_in_bytes =
      kOverreadInBytesPass1 - sizeof(*src0) * width;
  uint16x8_t s[2][4];
  uint8x16_t ma3[2][2], ma5[2];
  uint32x4_t sq[2][8], b3[2][6], b5[6];

  s[0][0] = Load1QMsanU16(src0 + 0, overread_in_bytes + 0);
  s[0][1] = Load1QMsanU16(src0 + 8, overread_in_bytes + 16);
  s[1][0] = Load1QMsanU16(src1 + 0, overread_in_bytes + 0);
  s[1][1] = Load1QMsanU16(src1 + 8, overread_in_bytes + 16);
  Square(s[0][0], sq[0]);
  Square(s[1][0], sq[1]);
  BoxFilterPreProcessLo(s, scales, sum3, sum5, square_sum3, square_sum5, sq,
                        ma3, b3, &ma5[0], b5);

  int x = 0;
  do {
    uint16x8_t ma[2];
    uint32x4_t b[4];
    uint8x16_t ma3x[3], ma5x[3];

    s[0][2] = Load1QMsanU16(src0 + x + 16,
                            overread_in_bytes + sizeof(*src0) * (x + 16));
    s[0][3] = Load1QMsanU16(src0 + x + 24,
                            overread_in_bytes + sizeof(*src0) * (x + 24));
    s[1][2] = Load1QMsanU16(src1 + x + 16,
                            overread_in_bytes + sizeof(*src1) * (x + 16));
    s[1][3] = Load1QMsanU16(src1 + x + 24,
                            overread_in_bytes + sizeof(*src1) * (x + 24));
    BoxFilterPreProcess(s, x + 8, scales, sum3, sum5, square_sum3, square_sum5,
                        sum_width, sq, ma3, b3, ma5, b5);

    Prepare3_8<0>(ma3[0], ma3x);
    ma[0] = Sum343Lo(ma3x);
    ma[1] = Sum343Hi(ma3x);
    StoreAligned32U16(ma343[0] + x, ma);
    Sum343(b3[0] + 0, b + 0);
    Sum343(b3[0] + 2, b + 2);
    StoreAligned64U32(b343[0] + x, b);
    Sum565(b5 + 0, b + 0);
    Sum565(b5 + 2, b + 2);
    StoreAligned64U32(b565, b);
    Prepare3_8<0>(ma3[1], ma3x);
    Store343_444Lo(ma3x, b3[1], x, ma343[1], ma444, b343[1], b444);
    Store343_444Hi(ma3x, b3[1] + 2, x + 8, ma343[1], ma444, b343[1], b444);
    Prepare3_8<0>(ma5, ma5x);
    ma[0] = Sum565Lo(ma5x);
    ma[1] = Sum565Hi(ma5x);
    StoreAligned32U16(ma565, ma);
    s[0][0] = s[0][2];
    s[0][1] = s[0][3];
    s[1][0] = s[1][2];
    s[1][1] = s[1][3];
    sq[0][2] = sq[0][6];
    sq[0][3] = sq[0][7];
    sq[1][2] = sq[1][6];
    sq[1][3] = sq[1][7];
    ma3[0][0] = ma3[0][1];
    ma3[1][0] = ma3[1][1];
    ma5[0] = ma5[1];
    b3[0][0] = b3[0][4];
    b3[0][1] = b3[0][5];
    b3[1][0] = b3[1][4];
    b3[1][1] = b3[1][5];
    b5[0] = b5[4];
    b5[1] = b5[5];
    ma565 += 16;
    b565 += 16;
    x += 16;
  } while (x < width);
}

template <int shift>
inline int16x4_t FilterOutput(const uint32x4_t ma_x_src, const uint32x4_t b) {
  // ma: 255 * 32 = 8160 (13 bits)
  // b: 65088 * 32 = 2082816 (21 bits)
  // v: b - ma * 255 (22 bits)
  const int32x4_t v = vreinterpretq_s32_u32(vsubq_u32(b, ma_x_src));
  // kSgrProjSgrBits = 8
  // kSgrProjRestoreBits = 4
  // shift = 4 or 5
  // v >> 8 or 9 (13 bits)
  return vqrshrn_n_s32(v, kSgrProjSgrBits + shift - kSgrProjRestoreBits);
}

template <int shift>
inline int16x8_t CalculateFilteredOutput(const uint16x8_t src,
                                         const uint16x8_t ma,
                                         const uint32x4_t b[2]) {
  const uint32x4_t ma_x_src_lo = VmullLo16(ma, src);
  const uint32x4_t ma_x_src_hi = VmullHi16(ma, src);
  const int16x4_t dst_lo = FilterOutput<shift>(ma_x_src_lo, b[0]);
  const int16x4_t dst_hi = FilterOutput<shift>(ma_x_src_hi, b[1]);
  return vcombine_s16(dst_lo, dst_hi);  // 13 bits
}

inline int16x8_t CalculateFilteredOutputPass1(const uint16x8_t src,
                                              const uint16x8_t ma[2],
                                              const uint32x4_t b[2][2]) {
  const uint16x8_t ma_sum = vaddq_u16(ma[0], ma[1]);
  uint32x4_t b_sum[2];
  b_sum[0] = vaddq_u32(b[0][0], b[1][0]);
  b_sum[1] = vaddq_u32(b[0][1], b[1][1]);
  return CalculateFilteredOutput<5>(src, ma_sum, b_sum);
}

inline int16x8_t CalculateFilteredOutputPass2(const uint16x8_t src,
                                              const uint16x8_t ma[3],
                                              const uint32x4_t b[3][2]) {
  const uint16x8_t ma_sum = Sum3_16(ma);
  uint32x4_t b_sum[2];
  Sum3_32(b, b_sum);
  return CalculateFilteredOutput<5>(src, ma_sum, b_sum);
}

inline int16x8_t SelfGuidedFinal(const uint16x8_t src, const int32x4_t v[2]) {
  const int16x4_t v_lo =
      vqrshrn_n_s32(v[0], kSgrProjRestoreBits + kSgrProjPrecisionBits);
  const int16x4_t v_hi =
      vqrshrn_n_s32(v[1], kSgrProjRestoreBits + kSgrProjPrecisionBits);
  const int16x8_t vv = vcombine_s16(v_lo, v_hi);
  return vaddq_s16(vreinterpretq_s16_u16(src), vv);
}

inline int16x8_t SelfGuidedDoubleMultiplier(const uint16x8_t src,
                                            const int16x8_t filter[2],
                                            const int w0, const int w2) {
  int32x4_t v[2];
  v[0] = vmull_n_s16(vget_low_s16(filter[0]), w0);
  v[1] = vmull_n_s16(vget_high_s16(filter[0]), w0);
  v[0] = vmlal_n_s16(v[0], vget_low_s16(filter[1]), w2);
  v[1] = vmlal_n_s16(v[1], vget_high_s16(filter[1]), w2);
  return SelfGuidedFinal(src, v);
}

inline int16x8_t SelfGuidedSingleMultiplier(const uint16x8_t src,
                                            const int16x8_t filter,
                                            const int w0) {
  // weight: -96 to 96 (Sgrproj_Xqd_Min/Max)
  int32x4_t v[2];
  v[0] = vmull_n_s16(vget_low_s16(filter), w0);
  v[1] = vmull_n_s16(vget_high_s16(filter), w0);
  return SelfGuidedFinal(src, v);
}

inline void ClipAndStore(uint16_t* const dst, const int16x8_t val) {
  const uint16x8_t val0 = vreinterpretq_u16_s16(vmaxq_s16(val, vdupq_n_s16(0)));
  const uint16x8_t val1 = vminq_u16(val0, vdupq_n_u16((1 << kBitdepth10) - 1));
  vst1q_u16(dst, val1);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPass1(
    const uint16_t* const src, const uint16_t* const src0,
    const uint16_t* const src1, const ptrdiff_t stride, uint16_t* const sum5[5],
    uint32_t* const square_sum5[5], const int width, const ptrdiff_t sum_width,
    const uint32_t scale, const int16_t w0, uint16_t* const ma565[2],
    uint32_t* const b565[2], uint16_t* const dst) {
  const ptrdiff_t overread_in_bytes =
      kOverreadInBytesPass1 - sizeof(*src0) * width;
  uint16x8_t s[2][4];
  uint8x16_t mas[2];
  uint32x4_t sq[2][8], bs[6];

  s[0][0] = Load1QMsanU16(src0 + 0, overread_in_bytes + 0);
  s[0][1] = Load1QMsanU16(src0 + 8, overread_in_bytes + 16);
  s[1][0] = Load1QMsanU16(src1 + 0, overread_in_bytes + 0);
  s[1][1] = Load1QMsanU16(src1 + 8, overread_in_bytes + 16);

  Square(s[0][0], sq[0]);
  Square(s[1][0], sq[1]);
  BoxFilterPreProcess5Lo(s, scale, sum5, square_sum5, sq, &mas[0], bs);

  int x = 0;
  do {
    uint16x8_t ma[2];
    uint32x4_t b[2][2];
    uint8x16_t ma5[3];
    int16x8_t p[2];

    s[0][2] = Load1QMsanU16(src0 + x + 16,
                            overread_in_bytes + sizeof(*src0) * (x + 16));
    s[0][3] = Load1QMsanU16(src0 + x + 24,
                            overread_in_bytes + sizeof(*src0) * (x + 24));
    s[1][2] = Load1QMsanU16(src1 + x + 16,
                            overread_in_bytes + sizeof(*src1) * (x + 16));
    s[1][3] = Load1QMsanU16(src1 + x + 24,
                            overread_in_bytes + sizeof(*src1) * (x + 24));
    BoxFilterPreProcess5(s, sum_width, x + 8, scale, sum5, square_sum5, sq, mas,
                         bs);
    Prepare3_8<0>(mas, ma5);
    ma[1] = Sum565Lo(ma5);
    vst1q_u16(ma565[1] + x, ma[1]);
    Sum565(bs, b[1]);
    StoreAligned32U32(b565[1] + x, b[1]);
    const uint16x8_t sr0_lo = vld1q_u16(src + x + 0);
    const uint16x8_t sr1_lo = vld1q_u16(src + stride + x + 0);
    ma[0] = vld1q_u16(ma565[0] + x);
    LoadAligned32U32(b565[0] + x, b[0]);
    p[0] = CalculateFilteredOutputPass1(sr0_lo, ma, b);
    p[1] = CalculateFilteredOutput<4>(sr1_lo, ma[1], b[1]);
    const int16x8_t d00 = SelfGuidedSingleMultiplier(sr0_lo, p[0], w0);
    const int16x8_t d10 = SelfGuidedSingleMultiplier(sr1_lo, p[1], w0);

    ma[1] = Sum565Hi(ma5);
    vst1q_u16(ma565[1] + x + 8, ma[1]);
    Sum565(bs + 2, b[1]);
    StoreAligned32U32(b565[1] + x + 8, b[1]);
    const uint16x8_t sr0_hi = vld1q_u16(src + x + 8);
    const uint16x8_t sr1_hi = vld1q_u16(src + stride + x + 8);
    ma[0] = vld1q_u16(ma565[0] + x + 8);
    LoadAligned32U32(b565[0] + x + 8, b[0]);
    p[0] = CalculateFilteredOutputPass1(sr0_hi, ma, b);
    p[1] = CalculateFilteredOutput<4>(sr1_hi, ma[1], b[1]);
    const int16x8_t d01 = SelfGuidedSingleMultiplier(sr0_hi, p[0], w0);
    ClipAndStore(dst + x + 0, d00);
    ClipAndStore(dst + x + 8, d01);
    const int16x8_t d11 = SelfGuidedSingleMultiplier(sr1_hi, p[1], w0);
    ClipAndStore(dst + stride + x + 0, d10);
    ClipAndStore(dst + stride + x + 8, d11);
    s[0][0] = s[0][2];
    s[0][1] = s[0][3];
    s[1][0] = s[1][2];
    s[1][1] = s[1][3];
    sq[0][2] = sq[0][6];
    sq[0][3] = sq[0][7];
    sq[1][2] = sq[1][6];
    sq[1][3] = sq[1][7];
    mas[0] = mas[1];
    bs[0] = bs[4];
    bs[1] = bs[5];
    x += 16;
  } while (x < width);
}

inline void BoxFilterPass1LastRow(
    const uint16_t* const src, const uint16_t* const src0, const int width,
    const ptrdiff_t sum_width, const uint32_t scale, const int16_t w0,
    uint16_t* const sum5[5], uint32_t* const square_sum5[5], uint16_t* ma565,
    uint32_t* b565, uint16_t* const dst) {
  const ptrdiff_t overread_in_bytes =
      kOverreadInBytesPass1 - sizeof(*src0) * width;
  uint16x8_t s[4];
  uint8x16_t mas[2];
  uint32x4_t sq[8], bs[6];

  s[0] = Load1QMsanU16(src0 + 0, overread_in_bytes + 0);
  s[1] = Load1QMsanU16(src0 + 8, overread_in_bytes + 16);
  Square(s[0], sq);
  BoxFilterPreProcess5LastRowLo(s, scale, sum5, square_sum5, sq, &mas[0], bs);

  int x = 0;
  do {
    uint16x8_t ma[2];
    uint32x4_t b[2][2];
    uint8x16_t ma5[3];

    s[2] = Load1QMsanU16(src0 + x + 16,
                         overread_in_bytes + sizeof(*src0) * (x + 16));
    s[3] = Load1QMsanU16(src0 + x + 24,
                         overread_in_bytes + sizeof(*src0) * (x + 24));
    BoxFilterPreProcess5LastRow(s, sum_width, x + 8, scale, sum5, square_sum5,
                                sq, mas, bs);
    Prepare3_8<0>(mas, ma5);
    ma[1] = Sum565Lo(ma5);
    Sum565(bs, b[1]);
    ma[0] = vld1q_u16(ma565);
    LoadAligned32U32(b565, b[0]);
    const uint16x8_t sr_lo = vld1q_u16(src + x + 0);
    int16x8_t p = CalculateFilteredOutputPass1(sr_lo, ma, b);
    const int16x8_t d0 = SelfGuidedSingleMultiplier(sr_lo, p, w0);

    ma[1] = Sum565Hi(ma5);
    Sum565(bs + 2, b[1]);
    ma[0] = vld1q_u16(ma565 + 8);
    LoadAligned32U32(b565 + 8, b[0]);
    const uint16x8_t sr_hi = vld1q_u16(src + x + 8);
    p = CalculateFilteredOutputPass1(sr_hi, ma, b);
    const int16x8_t d1 = SelfGuidedSingleMultiplier(sr_hi, p, w0);
    ClipAndStore(dst + x + 0, d0);
    ClipAndStore(dst + x + 8, d1);
    s[1] = s[3];
    sq[2] = sq[6];
    sq[3] = sq[7];
    mas[0] = mas[1];
    bs[0] = bs[4];
    bs[1] = bs[5];
    ma565 += 16;
    b565 += 16;
    x += 16;
  } while (x < width);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPass2(
    const uint16_t* const src, const uint16_t* const src0, const int width,
    const ptrdiff_t sum_width, const uint32_t scale, const int16_t w0,
    uint16_t* const sum3[3], uint32_t* const square_sum3[3],
    uint16_t* const ma343[3], uint16_t* const ma444[2], uint32_t* const b343[3],
    uint32_t* const b444[2], uint16_t* const dst) {
  const ptrdiff_t overread_in_bytes =
      kOverreadInBytesPass2 - sizeof(*src0) * width;
  uint16x8_t s[4];
  uint8x16_t mas[2];
  uint32x4_t sq[8], bs[6];

  s[0] = Load1QMsanU16(src0 + 0, overread_in_bytes + 0);
  s[1] = Load1QMsanU16(src0 + 8, overread_in_bytes + 16);
  Square(s[0], sq);
  BoxFilterPreProcess3Lo(s, scale, sum3, square_sum3, sq, &mas[0], bs);

  int x = 0;
  do {
    s[2] = Load1QMsanU16(src0 + x + 16,
                         overread_in_bytes + sizeof(*src0) * (x + 16));
    s[3] = Load1QMsanU16(src0 + x + 24,
                         overread_in_bytes + sizeof(*src0) * (x + 24));
    BoxFilterPreProcess3(s, x + 8, sum_width, scale, sum3, square_sum3, sq, mas,
                         bs);
    uint16x8_t ma[3];
    uint32x4_t b[3][2];
    uint8x16_t ma3[3];

    Prepare3_8<0>(mas, ma3);
    Store343_444Lo(ma3, bs + 0, x, &ma[2], b[2], ma343[2], ma444[1], b343[2],
                   b444[1]);
    const uint16x8_t sr_lo = vld1q_u16(src + x + 0);
    ma[0] = vld1q_u16(ma343[0] + x);
    ma[1] = vld1q_u16(ma444[0] + x);
    LoadAligned32U32(b343[0] + x, b[0]);
    LoadAligned32U32(b444[0] + x, b[1]);
    const int16x8_t p0 = CalculateFilteredOutputPass2(sr_lo, ma, b);

    Store343_444Hi(ma3, bs + 2, x + 8, &ma[2], b[2], ma343[2], ma444[1],
                   b343[2], b444[1]);
    const uint16x8_t sr_hi = vld1q_u16(src + x + 8);
    ma[0] = vld1q_u16(ma343[0] + x + 8);
    ma[1] = vld1q_u16(ma444[0] + x + 8);
    LoadAligned32U32(b343[0] + x + 8, b[0]);
    LoadAligned32U32(b444[0] + x + 8, b[1]);
    const int16x8_t p1 = CalculateFilteredOutputPass2(sr_hi, ma, b);
    const int16x8_t d0 = SelfGuidedSingleMultiplier(sr_lo, p0, w0);
    const int16x8_t d1 = SelfGuidedSingleMultiplier(sr_hi, p1, w0);
    ClipAndStore(dst + x + 0, d0);
    ClipAndStore(dst + x + 8, d1);
    s[1] = s[3];
    sq[2] = sq[6];
    sq[3] = sq[7];
    mas[0] = mas[1];
    bs[0] = bs[4];
    bs[1] = bs[5];
    x += 16;
  } while (x < width);
}

LIBGAV1_ALWAYS_INLINE void BoxFilter(
    const uint16_t* const src, const uint16_t* const src0,
    const uint16_t* const src1, const ptrdiff_t stride, const int width,
    const uint16_t scales[2], const int16_t w0, const int16_t w2,
    uint16_t* const sum3[4], uint16_t* const sum5[5],
    uint32_t* const square_sum3[4], uint32_t* const square_sum5[5],
    const ptrdiff_t sum_width, uint16_t* const ma343[4],
    uint16_t* const ma444[3], uint16_t* const ma565[2], uint32_t* const b343[4],
    uint32_t* const b444[3], uint32_t* const b565[2], uint16_t* const dst) {
  const ptrdiff_t overread_in_bytes =
      kOverreadInBytesPass1 - sizeof(*src0) * width;
  uint16x8_t s[2][4];
  uint8x16_t ma3[2][2], ma5[2];
  uint32x4_t sq[2][8], b3[2][6], b5[6];

  s[0][0] = Load1QMsanU16(src0 + 0, overread_in_bytes + 0);
  s[0][1] = Load1QMsanU16(src0 + 8, overread_in_bytes + 16);
  s[1][0] = Load1QMsanU16(src1 + 0, overread_in_bytes + 0);
  s[1][1] = Load1QMsanU16(src1 + 8, overread_in_bytes + 16);
  Square(s[0][0], sq[0]);
  Square(s[1][0], sq[1]);
  BoxFilterPreProcessLo(s, scales, sum3, sum5, square_sum3, square_sum5, sq,
                        ma3, b3, &ma5[0], b5);

  int x = 0;
  do {
    uint16x8_t ma[3][3];
    uint32x4_t b[3][3][2];
    uint8x16_t ma3x[2][3], ma5x[3];
    int16x8_t p[2][2];

    s[0][2] = Load1QMsanU16(src0 + x + 16,
                            overread_in_bytes + sizeof(*src0) * (x + 16));
    s[0][3] = Load1QMsanU16(src0 + x + 24,
                            overread_in_bytes + sizeof(*src0) * (x + 24));
    s[1][2] = Load1QMsanU16(src1 + x + 16,
                            overread_in_bytes + sizeof(*src1) * (x + 16));
    s[1][3] = Load1QMsanU16(src1 + x + 24,
                            overread_in_bytes + sizeof(*src1) * (x + 24));

    BoxFilterPreProcess(s, x + 8, scales, sum3, sum5, square_sum3, square_sum5,
                        sum_width, sq, ma3, b3, ma5, b5);
    Prepare3_8<0>(ma3[0], ma3x[0]);
    Prepare3_8<0>(ma3[1], ma3x[1]);
    Prepare3_8<0>(ma5, ma5x);
    Store343_444Lo(ma3x[0], b3[0], x, &ma[1][2], &ma[2][1], b[1][2], b[2][1],
                   ma343[2], ma444[1], b343[2], b444[1]);
    Store343_444Lo(ma3x[1], b3[1], x, &ma[2][2], b[2][2], ma343[3], ma444[2],
                   b343[3], b444[2]);
    ma[0][1] = Sum565Lo(ma5x);
    vst1q_u16(ma565[1] + x, ma[0][1]);
    Sum565(b5, b[0][1]);
    StoreAligned32U32(b565[1] + x, b[0][1]);
    const uint16x8_t sr0_lo = vld1q_u16(src + x);
    const uint16x8_t sr1_lo = vld1q_u16(src + stride + x);
    ma[0][0] = vld1q_u16(ma565[0] + x);
    LoadAligned32U32(b565[0] + x, b[0][0]);
    p[0][0] = CalculateFilteredOutputPass1(sr0_lo, ma[0], b[0]);
    p[1][0] = CalculateFilteredOutput<4>(sr1_lo, ma[0][1], b[0][1]);
    ma[1][0] = vld1q_u16(ma343[0] + x);
    ma[1][1] = vld1q_u16(ma444[0] + x);
    LoadAligned32U32(b343[0] + x, b[1][0]);
    LoadAligned32U32(b444[0] + x, b[1][1]);
    p[0][1] = CalculateFilteredOutputPass2(sr0_lo, ma[1], b[1]);
    const int16x8_t d00 = SelfGuidedDoubleMultiplier(sr0_lo, p[0], w0, w2);
    ma[2][0] = vld1q_u16(ma343[1] + x);
    LoadAligned32U32(b343[1] + x, b[2][0]);
    p[1][1] = CalculateFilteredOutputPass2(sr1_lo, ma[2], b[2]);
    const int16x8_t d10 = SelfGuidedDoubleMultiplier(sr1_lo, p[1], w0, w2);

    Store343_444Hi(ma3x[0], b3[0] + 2, x + 8, &ma[1][2], &ma[2][1], b[1][2],
                   b[2][1], ma343[2], ma444[1], b343[2], b444[1]);
    Store343_444Hi(ma3x[1], b3[1] + 2, x + 8, &ma[2][2], b[2][2], ma343[3],
                   ma444[2], b343[3], b444[2]);
    ma[0][1] = Sum565Hi(ma5x);
    vst1q_u16(ma565[1] + x + 8, ma[0][1]);
    Sum565(b5 + 2, b[0][1]);
    StoreAligned32U32(b565[1] + x + 8, b[0][1]);
    const uint16x8_t sr0_hi = Load1QMsanU16(
        src + x + 8, overread_in_bytes + 4 + sizeof(*src) * (x + 8));
    const uint16x8_t sr1_hi = Load1QMsanU16(
        src + stride + x + 8, overread_in_bytes + 4 + sizeof(*src) * (x + 8));
    ma[0][0] = vld1q_u16(ma565[0] + x + 8);
    LoadAligned32U32(b565[0] + x + 8, b[0][0]);
    p[0][0] = CalculateFilteredOutputPass1(sr0_hi, ma[0], b[0]);
    p[1][0] = CalculateFilteredOutput<4>(sr1_hi, ma[0][1], b[0][1]);
    ma[1][0] = vld1q_u16(ma343[0] + x + 8);
    ma[1][1] = vld1q_u16(ma444[0] + x + 8);
    LoadAligned32U32(b343[0] + x + 8, b[1][0]);
    LoadAligned32U32(b444[0] + x + 8, b[1][1]);
    p[0][1] = CalculateFilteredOutputPass2(sr0_hi, ma[1], b[1]);
    const int16x8_t d01 = SelfGuidedDoubleMultiplier(sr0_hi, p[0], w0, w2);
    ClipAndStore(dst + x + 0, d00);
    ClipAndStore(dst + x + 8, d01);
    ma[2][0] = vld1q_u16(ma343[1] + x + 8);
    LoadAligned32U32(b343[1] + x + 8, b[2][0]);
    p[1][1] = CalculateFilteredOutputPass2(sr1_hi, ma[2], b[2]);
    const int16x8_t d11 = SelfGuidedDoubleMultiplier(sr1_hi, p[1], w0, w2);
    ClipAndStore(dst + stride + x + 0, d10);
    ClipAndStore(dst + stride + x + 8, d11);
    s[0][0] = s[0][2];
    s[0][1] = s[0][3];
    s[1][0] = s[1][2];
    s[1][1] = s[1][3];
    sq[0][2] = sq[0][6];
    sq[0][3] = sq[0][7];
    sq[1][2] = sq[1][6];
    sq[1][3] = sq[1][7];
    ma3[0][0] = ma3[0][1];
    ma3[1][0] = ma3[1][1];
    ma5[0] = ma5[1];
    b3[0][0] = b3[0][4];
    b3[0][1] = b3[0][5];
    b3[1][0] = b3[1][4];
    b3[1][1] = b3[1][5];
    b5[0] = b5[4];
    b5[1] = b5[5];
    x += 16;
  } while (x < width);
}

inline void BoxFilterLastRow(
    const uint16_t* const src, const uint16_t* const src0, const int width,
    const ptrdiff_t sum_width, const uint16_t scales[2], const int16_t w0,
    const int16_t w2, uint16_t* const sum3[4], uint16_t* const sum5[5],
    uint32_t* const square_sum3[4], uint32_t* const square_sum5[5],
    uint16_t* const ma343, uint16_t* const ma444, uint16_t* const ma565,
    uint32_t* const b343, uint32_t* const b444, uint32_t* const b565,
    uint16_t* const dst) {
  const ptrdiff_t overread_in_bytes =
      kOverreadInBytesPass1 - sizeof(*src0) * width;
  uint16x8_t s[4];
  uint8x16_t ma3[2], ma5[2];
  uint32x4_t sq[8], b3[6], b5[6];
  uint16x8_t ma[3];
  uint32x4_t b[3][2];

  s[0] = Load1QMsanU16(src0 + 0, overread_in_bytes + 0);
  s[1] = Load1QMsanU16(src0 + 8, overread_in_bytes + 16);
  Square(s[0], sq);
  BoxFilterPreProcessLastRowLo(s, scales, sum3, sum5, square_sum3, square_sum5,
                               sq, &ma3[0], &ma5[0], b3, b5);

  int x = 0;
  do {
    uint8x16_t ma3x[3], ma5x[3];
    int16x8_t p[2];

    s[2] = Load1QMsanU16(src0 + x + 16,
                         overread_in_bytes + sizeof(*src0) * (x + 16));
    s[3] = Load1QMsanU16(src0 + x + 24,
                         overread_in_bytes + sizeof(*src0) * (x + 24));
    BoxFilterPreProcessLastRow(s, sum_width, x + 8, scales, sum3, sum5,
                               square_sum3, square_sum5, sq, ma3, ma5, b3, b5);
    Prepare3_8<0>(ma3, ma3x);
    Prepare3_8<0>(ma5, ma5x);
    ma[1] = Sum565Lo(ma5x);
    Sum565(b5, b[1]);
    ma[2] = Sum343Lo(ma3x);
    Sum343(b3, b[2]);
    const uint16x8_t sr_lo = vld1q_u16(src + x + 0);
    ma[0] = vld1q_u16(ma565 + x);
    LoadAligned32U32(b565 + x, b[0]);
    p[0] = CalculateFilteredOutputPass1(sr_lo, ma, b);
    ma[0] = vld1q_u16(ma343 + x);
    ma[1] = vld1q_u16(ma444 + x);
    LoadAligned32U32(b343 + x, b[0]);
    LoadAligned32U32(b444 + x, b[1]);
    p[1] = CalculateFilteredOutputPass2(sr_lo, ma, b);
    const int16x8_t d0 = SelfGuidedDoubleMultiplier(sr_lo, p, w0, w2);

    ma[1] = Sum565Hi(ma5x);
    Sum565(b5 + 2, b[1]);
    ma[2] = Sum343Hi(ma3x);
    Sum343(b3 + 2, b[2]);
    const uint16x8_t sr_hi = Load1QMsanU16(
        src + x + 8, overread_in_bytes + 4 + sizeof(*src) * (x + 8));
    ma[0] = vld1q_u16(ma565 + x + 8);
    LoadAligned32U32(b565 + x + 8, b[0]);
    p[0] = CalculateFilteredOutputPass1(sr_hi, ma, b);
    ma[0] = vld1q_u16(ma343 + x + 8);
    ma[1] = vld1q_u16(ma444 + x + 8);
    LoadAligned32U32(b343 + x + 8, b[0]);
    LoadAligned32U32(b444 + x + 8, b[1]);
    p[1] = CalculateFilteredOutputPass2(sr_hi, ma, b);
    const int16x8_t d1 = SelfGuidedDoubleMultiplier(sr_hi, p, w0, w2);
    ClipAndStore(dst + x + 0, d0);
    ClipAndStore(dst + x + 8, d1);
    s[1] = s[3];
    sq[2] = sq[6];
    sq[3] = sq[7];
    ma3[0] = ma3[1];
    ma5[0] = ma5[1];
    b3[0] = b3[4];
    b3[1] = b3[5];
    b5[0] = b5[4];
    b5[1] = b5[5];
    x += 16;
  } while (x < width);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterProcess(
    const RestorationUnitInfo& restoration_info, const uint16_t* src,
    const ptrdiff_t stride, const uint16_t* const top_border,
    const ptrdiff_t top_border_stride, const uint16_t* bottom_border,
    const ptrdiff_t bottom_border_stride, const int width, const int height,
    SgrBuffer* const sgr_buffer, uint16_t* dst) {
  const auto temp_stride = Align<ptrdiff_t>(width, 16);
  const auto sum_width = Align<ptrdiff_t>(width + 8, 16);
  const auto sum_stride = temp_stride + 16;
  const int sgr_proj_index = restoration_info.sgr_proj_info.index;
  const uint16_t* const scales = kSgrScaleParameter[sgr_proj_index];  // < 2^12.
  const int16_t w0 = restoration_info.sgr_proj_info.multiplier[0];
  const int16_t w1 = restoration_info.sgr_proj_info.multiplier[1];
  const int16_t w2 = (1 << kSgrProjPrecisionBits) - w0 - w1;
  uint16_t *sum3[4], *sum5[5], *ma343[4], *ma444[3], *ma565[2];
  uint32_t *square_sum3[4], *square_sum5[5], *b343[4], *b444[3], *b565[2];
  sum3[0] = sgr_buffer->sum3;
  square_sum3[0] = sgr_buffer->square_sum3;
  ma343[0] = sgr_buffer->ma343;
  b343[0] = sgr_buffer->b343;
  for (int i = 1; i <= 3; ++i) {
    sum3[i] = sum3[i - 1] + sum_stride;
    square_sum3[i] = square_sum3[i - 1] + sum_stride;
    ma343[i] = ma343[i - 1] + temp_stride;
    b343[i] = b343[i - 1] + temp_stride;
  }
  sum5[0] = sgr_buffer->sum5;
  square_sum5[0] = sgr_buffer->square_sum5;
  for (int i = 1; i <= 4; ++i) {
    sum5[i] = sum5[i - 1] + sum_stride;
    square_sum5[i] = square_sum5[i - 1] + sum_stride;
  }
  ma444[0] = sgr_buffer->ma444;
  b444[0] = sgr_buffer->b444;
  for (int i = 1; i <= 2; ++i) {
    ma444[i] = ma444[i - 1] + temp_stride;
    b444[i] = b444[i - 1] + temp_stride;
  }
  ma565[0] = sgr_buffer->ma565;
  ma565[1] = ma565[0] + temp_stride;
  b565[0] = sgr_buffer->b565;
  b565[1] = b565[0] + temp_stride;
  assert(scales[0] != 0);
  assert(scales[1] != 0);
  BoxSum(top_border, top_border_stride, width, sum_stride, sum_width, sum3[0],
         sum5[1], square_sum3[0], square_sum5[1]);
  sum5[0] = sum5[1];
  square_sum5[0] = square_sum5[1];
  const uint16_t* const s = (height > 1) ? src + stride : bottom_border;
  BoxSumFilterPreProcess(src, s, width, scales, sum3, sum5, square_sum3,
                         square_sum5, sum_width, ma343, ma444[0], ma565[0],
                         b343, b444[0], b565[0]);
  sum5[0] = sgr_buffer->sum5;
  square_sum5[0] = sgr_buffer->square_sum5;

  for (int y = (height >> 1) - 1; y > 0; --y) {
    Circulate4PointersBy2<uint16_t>(sum3);
    Circulate4PointersBy2<uint32_t>(square_sum3);
    Circulate5PointersBy2<uint16_t>(sum5);
    Circulate5PointersBy2<uint32_t>(square_sum5);
    BoxFilter(src + 3, src + 2 * stride, src + 3 * stride, stride, width,
              scales, w0, w2, sum3, sum5, square_sum3, square_sum5, sum_width,
              ma343, ma444, ma565, b343, b444, b565, dst);
    src += 2 * stride;
    dst += 2 * stride;
    Circulate4PointersBy2<uint16_t>(ma343);
    Circulate4PointersBy2<uint32_t>(b343);
    std::swap(ma444[0], ma444[2]);
    std::swap(b444[0], b444[2]);
    std::swap(ma565[0], ma565[1]);
    std::swap(b565[0], b565[1]);
  }

  Circulate4PointersBy2<uint16_t>(sum3);
  Circulate4PointersBy2<uint32_t>(square_sum3);
  Circulate5PointersBy2<uint16_t>(sum5);
  Circulate5PointersBy2<uint32_t>(square_sum5);
  if ((height & 1) == 0 || height > 1) {
    const uint16_t* sr[2];
    if ((height & 1) == 0) {
      sr[0] = bottom_border;
      sr[1] = bottom_border + bottom_border_stride;
    } else {
      sr[0] = src + 2 * stride;
      sr[1] = bottom_border;
    }
    BoxFilter(src + 3, sr[0], sr[1], stride, width, scales, w0, w2, sum3, sum5,
              square_sum3, square_sum5, sum_width, ma343, ma444, ma565, b343,
              b444, b565, dst);
  }
  if ((height & 1) != 0) {
    if (height > 1) {
      src += 2 * stride;
      dst += 2 * stride;
      Circulate4PointersBy2<uint16_t>(sum3);
      Circulate4PointersBy2<uint32_t>(square_sum3);
      Circulate5PointersBy2<uint16_t>(sum5);
      Circulate5PointersBy2<uint32_t>(square_sum5);
      Circulate4PointersBy2<uint16_t>(ma343);
      Circulate4PointersBy2<uint32_t>(b343);
      std::swap(ma444[0], ma444[2]);
      std::swap(b444[0], b444[2]);
      std::swap(ma565[0], ma565[1]);
      std::swap(b565[0], b565[1]);
    }
    BoxFilterLastRow(src + 3, bottom_border + bottom_border_stride, width,
                     sum_width, scales, w0, w2, sum3, sum5, square_sum3,
                     square_sum5, ma343[0], ma444[0], ma565[0], b343[0],
                     b444[0], b565[0], dst);
  }
}

inline void BoxFilterProcessPass1(const RestorationUnitInfo& restoration_info,
                                  const uint16_t* src, const ptrdiff_t stride,
                                  const uint16_t* const top_border,
                                  const ptrdiff_t top_border_stride,
                                  const uint16_t* bottom_border,
                                  const ptrdiff_t bottom_border_stride,
                                  const int width, const int height,
                                  SgrBuffer* const sgr_buffer, uint16_t* dst) {
  const auto temp_stride = Align<ptrdiff_t>(width, 16);
  const auto sum_width = Align<ptrdiff_t>(width + 8, 16);
  const auto sum_stride = temp_stride + 16;
  const int sgr_proj_index = restoration_info.sgr_proj_info.index;
  const uint32_t scale = kSgrScaleParameter[sgr_proj_index][0];  // < 2^12.
  const int16_t w0 = restoration_info.sgr_proj_info.multiplier[0];
  uint16_t *sum5[5], *ma565[2];
  uint32_t *square_sum5[5], *b565[2];
  sum5[0] = sgr_buffer->sum5;
  square_sum5[0] = sgr_buffer->square_sum5;
  for (int i = 1; i <= 4; ++i) {
    sum5[i] = sum5[i - 1] + sum_stride;
    square_sum5[i] = square_sum5[i - 1] + sum_stride;
  }
  ma565[0] = sgr_buffer->ma565;
  ma565[1] = ma565[0] + temp_stride;
  b565[0] = sgr_buffer->b565;
  b565[1] = b565[0] + temp_stride;
  assert(scale != 0);

  BoxSum<5>(top_border, top_border_stride, width, sum_stride, sum_width,
            sum5[1], square_sum5[1]);
  sum5[0] = sum5[1];
  square_sum5[0] = square_sum5[1];
  const uint16_t* const s = (height > 1) ? src + stride : bottom_border;
  BoxSumFilterPreProcess5(src, s, width, scale, sum5, square_sum5, sum_width,
                          ma565[0], b565[0]);
  sum5[0] = sgr_buffer->sum5;
  square_sum5[0] = sgr_buffer->square_sum5;

  for (int y = (height >> 1) - 1; y > 0; --y) {
    Circulate5PointersBy2<uint16_t>(sum5);
    Circulate5PointersBy2<uint32_t>(square_sum5);
    BoxFilterPass1(src + 3, src + 2 * stride, src + 3 * stride, stride, sum5,
                   square_sum5, width, sum_width, scale, w0, ma565, b565, dst);
    src += 2 * stride;
    dst += 2 * stride;
    std::swap(ma565[0], ma565[1]);
    std::swap(b565[0], b565[1]);
  }

  Circulate5PointersBy2<uint16_t>(sum5);
  Circulate5PointersBy2<uint32_t>(square_sum5);
  if ((height & 1) == 0 || height > 1) {
    const uint16_t* sr[2];
    if ((height & 1) == 0) {
      sr[0] = bottom_border;
      sr[1] = bottom_border + bottom_border_stride;
    } else {
      sr[0] = src + 2 * stride;
      sr[1] = bottom_border;
    }
    BoxFilterPass1(src + 3, sr[0], sr[1], stride, sum5, square_sum5, width,
                   sum_width, scale, w0, ma565, b565, dst);
  }
  if ((height & 1) != 0) {
    src += 3;
    if (height > 1) {
      src += 2 * stride;
      dst += 2 * stride;
      std::swap(ma565[0], ma565[1]);
      std::swap(b565[0], b565[1]);
      Circulate5PointersBy2<uint16_t>(sum5);
      Circulate5PointersBy2<uint32_t>(square_sum5);
    }
    BoxFilterPass1LastRow(src, bottom_border + bottom_border_stride, width,
                          sum_width, scale, w0, sum5, square_sum5, ma565[0],
                          b565[0], dst);
  }
}

inline void BoxFilterProcessPass2(const RestorationUnitInfo& restoration_info,
                                  const uint16_t* src, const ptrdiff_t stride,
                                  const uint16_t* const top_border,
                                  const ptrdiff_t top_border_stride,
                                  const uint16_t* bottom_border,
                                  const ptrdiff_t bottom_border_stride,
                                  const int width, const int height,
                                  SgrBuffer* const sgr_buffer, uint16_t* dst) {
  assert(restoration_info.sgr_proj_info.multiplier[0] == 0);
  const auto temp_stride = Align<ptrdiff_t>(width, 16);
  const auto sum_width = Align<ptrdiff_t>(width + 8, 16);
  const auto sum_stride = temp_stride + 16;
  const int16_t w1 = restoration_info.sgr_proj_info.multiplier[1];
  const int16_t w0 = (1 << kSgrProjPrecisionBits) - w1;
  const int sgr_proj_index = restoration_info.sgr_proj_info.index;
  const uint32_t scale = kSgrScaleParameter[sgr_proj_index][1];  // < 2^12.
  uint16_t *sum3[3], *ma343[3], *ma444[2];
  uint32_t *square_sum3[3], *b343[3], *b444[2];
  sum3[0] = sgr_buffer->sum3;
  square_sum3[0] = sgr_buffer->square_sum3;
  ma343[0] = sgr_buffer->ma343;
  b343[0] = sgr_buffer->b343;
  for (int i = 1; i <= 2; ++i) {
    sum3[i] = sum3[i - 1] + sum_stride;
    square_sum3[i] = square_sum3[i - 1] + sum_stride;
    ma343[i] = ma343[i - 1] + temp_stride;
    b343[i] = b343[i - 1] + temp_stride;
  }
  ma444[0] = sgr_buffer->ma444;
  ma444[1] = ma444[0] + temp_stride;
  b444[0] = sgr_buffer->b444;
  b444[1] = b444[0] + temp_stride;
  assert(scale != 0);
  BoxSum<3>(top_border, top_border_stride, width, sum_stride, sum_width,
            sum3[0], square_sum3[0]);
  BoxSumFilterPreProcess3<false>(src, width, scale, sum3, square_sum3,
                                 sum_width, ma343[0], nullptr, b343[0],
                                 nullptr);
  Circulate3PointersBy1<uint16_t>(sum3);
  Circulate3PointersBy1<uint32_t>(square_sum3);
  const uint16_t* s;
  if (height > 1) {
    s = src + stride;
  } else {
    s = bottom_border;
    bottom_border += bottom_border_stride;
  }
  BoxSumFilterPreProcess3<true>(s, width, scale, sum3, square_sum3, sum_width,
                                ma343[1], ma444[0], b343[1], b444[0]);

  for (int y = height - 2; y > 0; --y) {
    Circulate3PointersBy1<uint16_t>(sum3);
    Circulate3PointersBy1<uint32_t>(square_sum3);
    BoxFilterPass2(src + 2, src + 2 * stride, width, sum_width, scale, w0, sum3,
                   square_sum3, ma343, ma444, b343, b444, dst);
    src += stride;
    dst += stride;
    Circulate3PointersBy1<uint16_t>(ma343);
    Circulate3PointersBy1<uint32_t>(b343);
    std::swap(ma444[0], ma444[1]);
    std::swap(b444[0], b444[1]);
  }

  int y = std::min(height, 2);
  src += 2;
  do {
    Circulate3PointersBy1<uint16_t>(sum3);
    Circulate3PointersBy1<uint32_t>(square_sum3);
    BoxFilterPass2(src, bottom_border, width, sum_width, scale, w0, sum3,
                   square_sum3, ma343, ma444, b343, b444, dst);
    src += stride;
    dst += stride;
    bottom_border += bottom_border_stride;
    Circulate3PointersBy1<uint16_t>(ma343);
    Circulate3PointersBy1<uint32_t>(b343);
    std::swap(ma444[0], ma444[1]);
    std::swap(b444[0], b444[1]);
  } while (--y != 0);
}

// If |width| is non-multiple of 8, up to 7 more pixels are written to |dest| in
// the end of each row. It is safe to overwrite the output as it will not be
// part of the visible frame.
void SelfGuidedFilter_NEON(
    const RestorationUnitInfo& LIBGAV1_RESTRICT restoration_info,
    const void* LIBGAV1_RESTRICT const source, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_border,
    const ptrdiff_t top_border_stride,
    const void* LIBGAV1_RESTRICT const bottom_border,
    const ptrdiff_t bottom_border_stride, const int width, const int height,
    RestorationBuffer* LIBGAV1_RESTRICT const restoration_buffer,
    void* LIBGAV1_RESTRICT const dest) {
  const int index = restoration_info.sgr_proj_info.index;
  const int radius_pass_0 = kSgrProjParams[index][0];  // 2 or 0
  const int radius_pass_1 = kSgrProjParams[index][2];  // 1 or 0
  const auto* const src = static_cast<const uint16_t*>(source);
  const auto* top = static_cast<const uint16_t*>(top_border);
  const auto* bottom = static_cast<const uint16_t*>(bottom_border);
  auto* const dst = static_cast<uint16_t*>(dest);
  SgrBuffer* const sgr_buffer = &restoration_buffer->sgr_buffer;
  if (radius_pass_1 == 0) {
    // |radius_pass_0| and |radius_pass_1| cannot both be 0, so we have the
    // following assertion.
    assert(radius_pass_0 != 0);
    BoxFilterProcessPass1(restoration_info, src - 3, stride, top - 3,
                          top_border_stride, bottom - 3, bottom_border_stride,
                          width, height, sgr_buffer, dst);
  } else if (radius_pass_0 == 0) {
    BoxFilterProcessPass2(restoration_info, src - 2, stride, top - 2,
                          top_border_stride, bottom - 2, bottom_border_stride,
                          width, height, sgr_buffer, dst);
  } else {
    BoxFilterProcess(restoration_info, src - 3, stride, top - 3,
                     top_border_stride, bottom - 3, bottom_border_stride, width,
                     height, sgr_buffer, dst);
  }
}

void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth10);
  assert(dsp != nullptr);
  dsp->loop_restorations[0] = WienerFilter_NEON;
  dsp->loop_restorations[1] = SelfGuidedFilter_NEON;
}

}  // namespace

void LoopRestorationInit10bpp_NEON() { Init10bpp(); }

}  // namespace dsp
}  // namespace libgav1

#else   // !(LIBGAV1_ENABLE_NEON && LIBGAV1_MAX_BITDEPTH >= 10)
namespace libgav1 {
namespace dsp {

void LoopRestorationInit10bpp_NEON() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_ENABLE_NEON && LIBGAV1_MAX_BITDEPTH >= 10
