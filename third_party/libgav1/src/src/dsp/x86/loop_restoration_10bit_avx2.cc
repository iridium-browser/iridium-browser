// Copyright 2020 The libgav1 Authors
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

#if LIBGAV1_TARGETING_AVX2 && LIBGAV1_MAX_BITDEPTH >= 10
#include <immintrin.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "src/dsp/common.h"
#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/dsp/x86/common_avx2.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"

namespace libgav1 {
namespace dsp {
namespace {

inline void WienerHorizontalClip(const __m256i s[2],
                                 int16_t* const wiener_buffer) {
  constexpr int offset =
      1 << (10 + kWienerFilterBits - kInterRoundBitsHorizontal - 1);
  constexpr int limit = (offset << 2) - 1;
  const __m256i offsets = _mm256_set1_epi16(-offset);
  const __m256i limits = _mm256_set1_epi16(limit - offset);
  const __m256i round = _mm256_set1_epi32(1 << (kInterRoundBitsHorizontal - 1));
  const __m256i sum0 = _mm256_add_epi32(s[0], round);
  const __m256i sum1 = _mm256_add_epi32(s[1], round);
  const __m256i rounded_sum0 =
      _mm256_srai_epi32(sum0, kInterRoundBitsHorizontal);
  const __m256i rounded_sum1 =
      _mm256_srai_epi32(sum1, kInterRoundBitsHorizontal);
  const __m256i rounded_sum = _mm256_packs_epi32(rounded_sum0, rounded_sum1);
  const __m256i d0 = _mm256_max_epi16(rounded_sum, offsets);
  const __m256i d1 = _mm256_min_epi16(d0, limits);
  StoreAligned32(wiener_buffer, d1);
}

inline void WienerHorizontalTap7Kernel(const __m256i s[7],
                                       const __m256i filter[2],
                                       int16_t* const wiener_buffer) {
  const __m256i s06 = _mm256_add_epi16(s[0], s[6]);
  const __m256i s15 = _mm256_add_epi16(s[1], s[5]);
  const __m256i s24 = _mm256_add_epi16(s[2], s[4]);
  const __m256i ss0 = _mm256_unpacklo_epi16(s06, s15);
  const __m256i ss1 = _mm256_unpackhi_epi16(s06, s15);
  const __m256i ss2 = _mm256_unpacklo_epi16(s24, s[3]);
  const __m256i ss3 = _mm256_unpackhi_epi16(s24, s[3]);
  __m256i madds[4];
  madds[0] = _mm256_madd_epi16(ss0, filter[0]);
  madds[1] = _mm256_madd_epi16(ss1, filter[0]);
  madds[2] = _mm256_madd_epi16(ss2, filter[1]);
  madds[3] = _mm256_madd_epi16(ss3, filter[1]);
  madds[0] = _mm256_add_epi32(madds[0], madds[2]);
  madds[1] = _mm256_add_epi32(madds[1], madds[3]);
  WienerHorizontalClip(madds, wiener_buffer);
}

inline void WienerHorizontalTap5Kernel(const __m256i s[5], const __m256i filter,
                                       int16_t* const wiener_buffer) {
  const __m256i s04 = _mm256_add_epi16(s[0], s[4]);
  const __m256i s13 = _mm256_add_epi16(s[1], s[3]);
  const __m256i s2d = _mm256_add_epi16(s[2], s[2]);
  const __m256i s0m = _mm256_sub_epi16(s04, s2d);
  const __m256i s1m = _mm256_sub_epi16(s13, s2d);
  const __m256i ss0 = _mm256_unpacklo_epi16(s0m, s1m);
  const __m256i ss1 = _mm256_unpackhi_epi16(s0m, s1m);
  __m256i madds[2];
  madds[0] = _mm256_madd_epi16(ss0, filter);
  madds[1] = _mm256_madd_epi16(ss1, filter);
  const __m256i s2_lo = _mm256_unpacklo_epi16(s[2], _mm256_setzero_si256());
  const __m256i s2_hi = _mm256_unpackhi_epi16(s[2], _mm256_setzero_si256());
  const __m256i s2x128_lo = _mm256_slli_epi32(s2_lo, 7);
  const __m256i s2x128_hi = _mm256_slli_epi32(s2_hi, 7);
  madds[0] = _mm256_add_epi32(madds[0], s2x128_lo);
  madds[1] = _mm256_add_epi32(madds[1], s2x128_hi);
  WienerHorizontalClip(madds, wiener_buffer);
}

inline void WienerHorizontalTap3Kernel(const __m256i s[3], const __m256i filter,
                                       int16_t* const wiener_buffer) {
  const __m256i s02 = _mm256_add_epi16(s[0], s[2]);
  const __m256i ss0 = _mm256_unpacklo_epi16(s02, s[1]);
  const __m256i ss1 = _mm256_unpackhi_epi16(s02, s[1]);
  __m256i madds[2];
  madds[0] = _mm256_madd_epi16(ss0, filter);
  madds[1] = _mm256_madd_epi16(ss1, filter);
  WienerHorizontalClip(madds, wiener_buffer);
}

inline void WienerHorizontalTap7(const uint16_t* src,
                                 const ptrdiff_t src_stride,
                                 const ptrdiff_t width, const int height,
                                 const __m256i* const coefficients,
                                 int16_t** const wiener_buffer) {
  __m256i filter[2];
  filter[0] = _mm256_shuffle_epi32(*coefficients, 0x0);
  filter[1] = _mm256_shuffle_epi32(*coefficients, 0x55);
  for (int y = height; y != 0; --y) {
    ptrdiff_t x = 0;
    do {
      __m256i s[7];
      s[0] = LoadUnaligned32(src + x + 0);
      s[1] = LoadUnaligned32(src + x + 1);
      s[2] = LoadUnaligned32(src + x + 2);
      s[3] = LoadUnaligned32(src + x + 3);
      s[4] = LoadUnaligned32(src + x + 4);
      s[5] = LoadUnaligned32(src + x + 5);
      s[6] = LoadUnaligned32(src + x + 6);
      WienerHorizontalTap7Kernel(s, filter, *wiener_buffer + x);
      x += 16;
    } while (x < width);
    src += src_stride;
    *wiener_buffer += width;
  }
}

inline void WienerHorizontalTap5(const uint16_t* src,
                                 const ptrdiff_t src_stride,
                                 const ptrdiff_t width, const int height,
                                 const __m256i* const coefficients,
                                 int16_t** const wiener_buffer) {
  const __m256i filter =
      _mm256_shuffle_epi8(*coefficients, _mm256_set1_epi32(0x05040302));
  for (int y = height; y != 0; --y) {
    ptrdiff_t x = 0;
    do {
      __m256i s[5];
      s[0] = LoadUnaligned32(src + x + 0);
      s[1] = LoadUnaligned32(src + x + 1);
      s[2] = LoadUnaligned32(src + x + 2);
      s[3] = LoadUnaligned32(src + x + 3);
      s[4] = LoadUnaligned32(src + x + 4);
      WienerHorizontalTap5Kernel(s, filter, *wiener_buffer + x);
      x += 16;
    } while (x < width);
    src += src_stride;
    *wiener_buffer += width;
  }
}

inline void WienerHorizontalTap3(const uint16_t* src,
                                 const ptrdiff_t src_stride,
                                 const ptrdiff_t width, const int height,
                                 const __m256i* const coefficients,
                                 int16_t** const wiener_buffer) {
  const auto filter = _mm256_shuffle_epi32(*coefficients, 0x55);
  for (int y = height; y != 0; --y) {
    ptrdiff_t x = 0;
    do {
      __m256i s[3];
      s[0] = LoadUnaligned32(src + x + 0);
      s[1] = LoadUnaligned32(src + x + 1);
      s[2] = LoadUnaligned32(src + x + 2);
      WienerHorizontalTap3Kernel(s, filter, *wiener_buffer + x);
      x += 16;
    } while (x < width);
    src += src_stride;
    *wiener_buffer += width;
  }
}

inline void WienerHorizontalTap1(const uint16_t* src,
                                 const ptrdiff_t src_stride,
                                 const ptrdiff_t width, const int height,
                                 int16_t** const wiener_buffer) {
  for (int y = height; y != 0; --y) {
    ptrdiff_t x = 0;
    do {
      const __m256i s0 = LoadUnaligned32(src + x);
      const __m256i d0 = _mm256_slli_epi16(s0, 4);
      StoreAligned32(*wiener_buffer + x, d0);
      x += 16;
    } while (x < width);
    src += src_stride;
    *wiener_buffer += width;
  }
}

inline __m256i WienerVertical7(const __m256i a[4], const __m256i filter[4]) {
  const __m256i madd0 = _mm256_madd_epi16(a[0], filter[0]);
  const __m256i madd1 = _mm256_madd_epi16(a[1], filter[1]);
  const __m256i madd2 = _mm256_madd_epi16(a[2], filter[2]);
  const __m256i madd3 = _mm256_madd_epi16(a[3], filter[3]);
  const __m256i madd01 = _mm256_add_epi32(madd0, madd1);
  const __m256i madd23 = _mm256_add_epi32(madd2, madd3);
  const __m256i sum = _mm256_add_epi32(madd01, madd23);
  return _mm256_srai_epi32(sum, kInterRoundBitsVertical);
}

inline __m256i WienerVertical5(const __m256i a[3], const __m256i filter[3]) {
  const __m256i madd0 = _mm256_madd_epi16(a[0], filter[0]);
  const __m256i madd1 = _mm256_madd_epi16(a[1], filter[1]);
  const __m256i madd2 = _mm256_madd_epi16(a[2], filter[2]);
  const __m256i madd01 = _mm256_add_epi32(madd0, madd1);
  const __m256i sum = _mm256_add_epi32(madd01, madd2);
  return _mm256_srai_epi32(sum, kInterRoundBitsVertical);
}

inline __m256i WienerVertical3(const __m256i a[2], const __m256i filter[2]) {
  const __m256i madd0 = _mm256_madd_epi16(a[0], filter[0]);
  const __m256i madd1 = _mm256_madd_epi16(a[1], filter[1]);
  const __m256i sum = _mm256_add_epi32(madd0, madd1);
  return _mm256_srai_epi32(sum, kInterRoundBitsVertical);
}

inline __m256i WienerVerticalClip(const __m256i s[2]) {
  const __m256i d = _mm256_packus_epi32(s[0], s[1]);
  return _mm256_min_epu16(d, _mm256_set1_epi16(1023));
}

inline __m256i WienerVerticalFilter7(const __m256i a[7],
                                     const __m256i filter[2]) {
  const __m256i round = _mm256_set1_epi16(1 << (kInterRoundBitsVertical - 1));
  __m256i b[4], c[2];
  b[0] = _mm256_unpacklo_epi16(a[0], a[1]);
  b[1] = _mm256_unpacklo_epi16(a[2], a[3]);
  b[2] = _mm256_unpacklo_epi16(a[4], a[5]);
  b[3] = _mm256_unpacklo_epi16(a[6], round);
  c[0] = WienerVertical7(b, filter);
  b[0] = _mm256_unpackhi_epi16(a[0], a[1]);
  b[1] = _mm256_unpackhi_epi16(a[2], a[3]);
  b[2] = _mm256_unpackhi_epi16(a[4], a[5]);
  b[3] = _mm256_unpackhi_epi16(a[6], round);
  c[1] = WienerVertical7(b, filter);
  return WienerVerticalClip(c);
}

inline __m256i WienerVerticalFilter5(const __m256i a[5],
                                     const __m256i filter[3]) {
  const __m256i round = _mm256_set1_epi16(1 << (kInterRoundBitsVertical - 1));
  __m256i b[3], c[2];
  b[0] = _mm256_unpacklo_epi16(a[0], a[1]);
  b[1] = _mm256_unpacklo_epi16(a[2], a[3]);
  b[2] = _mm256_unpacklo_epi16(a[4], round);
  c[0] = WienerVertical5(b, filter);
  b[0] = _mm256_unpackhi_epi16(a[0], a[1]);
  b[1] = _mm256_unpackhi_epi16(a[2], a[3]);
  b[2] = _mm256_unpackhi_epi16(a[4], round);
  c[1] = WienerVertical5(b, filter);
  return WienerVerticalClip(c);
}

inline __m256i WienerVerticalFilter3(const __m256i a[3],
                                     const __m256i filter[2]) {
  const __m256i round = _mm256_set1_epi16(1 << (kInterRoundBitsVertical - 1));
  __m256i b[2], c[2];
  b[0] = _mm256_unpacklo_epi16(a[0], a[1]);
  b[1] = _mm256_unpacklo_epi16(a[2], round);
  c[0] = WienerVertical3(b, filter);
  b[0] = _mm256_unpackhi_epi16(a[0], a[1]);
  b[1] = _mm256_unpackhi_epi16(a[2], round);
  c[1] = WienerVertical3(b, filter);
  return WienerVerticalClip(c);
}

inline __m256i WienerVerticalTap7Kernel(const int16_t* wiener_buffer,
                                        const ptrdiff_t wiener_stride,
                                        const __m256i filter[2], __m256i a[7]) {
  a[0] = LoadAligned32(wiener_buffer + 0 * wiener_stride);
  a[1] = LoadAligned32(wiener_buffer + 1 * wiener_stride);
  a[2] = LoadAligned32(wiener_buffer + 2 * wiener_stride);
  a[3] = LoadAligned32(wiener_buffer + 3 * wiener_stride);
  a[4] = LoadAligned32(wiener_buffer + 4 * wiener_stride);
  a[5] = LoadAligned32(wiener_buffer + 5 * wiener_stride);
  a[6] = LoadAligned32(wiener_buffer + 6 * wiener_stride);
  return WienerVerticalFilter7(a, filter);
}

inline __m256i WienerVerticalTap5Kernel(const int16_t* wiener_buffer,
                                        const ptrdiff_t wiener_stride,
                                        const __m256i filter[3], __m256i a[5]) {
  a[0] = LoadAligned32(wiener_buffer + 0 * wiener_stride);
  a[1] = LoadAligned32(wiener_buffer + 1 * wiener_stride);
  a[2] = LoadAligned32(wiener_buffer + 2 * wiener_stride);
  a[3] = LoadAligned32(wiener_buffer + 3 * wiener_stride);
  a[4] = LoadAligned32(wiener_buffer + 4 * wiener_stride);
  return WienerVerticalFilter5(a, filter);
}

inline __m256i WienerVerticalTap3Kernel(const int16_t* wiener_buffer,
                                        const ptrdiff_t wiener_stride,
                                        const __m256i filter[2], __m256i a[3]) {
  a[0] = LoadAligned32(wiener_buffer + 0 * wiener_stride);
  a[1] = LoadAligned32(wiener_buffer + 1 * wiener_stride);
  a[2] = LoadAligned32(wiener_buffer + 2 * wiener_stride);
  return WienerVerticalFilter3(a, filter);
}

inline void WienerVerticalTap7Kernel2(const int16_t* wiener_buffer,
                                      const ptrdiff_t wiener_stride,
                                      const __m256i filter[2], __m256i d[2]) {
  __m256i a[8];
  d[0] = WienerVerticalTap7Kernel(wiener_buffer, wiener_stride, filter, a);
  a[7] = LoadAligned32(wiener_buffer + 7 * wiener_stride);
  d[1] = WienerVerticalFilter7(a + 1, filter);
}

inline void WienerVerticalTap5Kernel2(const int16_t* wiener_buffer,
                                      const ptrdiff_t wiener_stride,
                                      const __m256i filter[3], __m256i d[2]) {
  __m256i a[6];
  d[0] = WienerVerticalTap5Kernel(wiener_buffer, wiener_stride, filter, a);
  a[5] = LoadAligned32(wiener_buffer + 5 * wiener_stride);
  d[1] = WienerVerticalFilter5(a + 1, filter);
}

inline void WienerVerticalTap3Kernel2(const int16_t* wiener_buffer,
                                      const ptrdiff_t wiener_stride,
                                      const __m256i filter[2], __m256i d[2]) {
  __m256i a[4];
  d[0] = WienerVerticalTap3Kernel(wiener_buffer, wiener_stride, filter, a);
  a[3] = LoadAligned32(wiener_buffer + 3 * wiener_stride);
  d[1] = WienerVerticalFilter3(a + 1, filter);
}

inline void WienerVerticalTap7(const int16_t* wiener_buffer,
                               const ptrdiff_t width, const int height,
                               const int16_t coefficients[4], uint16_t* dst,
                               const ptrdiff_t dst_stride) {
  const __m256i c = _mm256_broadcastq_epi64(LoadLo8(coefficients));
  __m256i filter[4];
  filter[0] = _mm256_shuffle_epi32(c, 0x0);
  filter[1] = _mm256_shuffle_epi32(c, 0x55);
  filter[2] = _mm256_shuffle_epi8(c, _mm256_set1_epi32(0x03020504));
  filter[3] =
      _mm256_set1_epi32((1 << 16) | static_cast<uint16_t>(coefficients[0]));
  for (int y = height >> 1; y > 0; --y) {
    ptrdiff_t x = 0;
    do {
      __m256i d[2];
      WienerVerticalTap7Kernel2(wiener_buffer + x, width, filter, d);
      StoreUnaligned32(dst + x, d[0]);
      StoreUnaligned32(dst + dst_stride + x, d[1]);
      x += 16;
    } while (x < width);
    dst += 2 * dst_stride;
    wiener_buffer += 2 * width;
  }

  if ((height & 1) != 0) {
    ptrdiff_t x = 0;
    do {
      __m256i a[7];
      const __m256i d =
          WienerVerticalTap7Kernel(wiener_buffer + x, width, filter, a);
      StoreUnaligned32(dst + x, d);
      x += 16;
    } while (x < width);
  }
}

inline void WienerVerticalTap5(const int16_t* wiener_buffer,
                               const ptrdiff_t width, const int height,
                               const int16_t coefficients[3], uint16_t* dst,
                               const ptrdiff_t dst_stride) {
  const __m256i c = _mm256_broadcastq_epi64(LoadLo8(coefficients));
  __m256i filter[3];
  filter[0] = _mm256_shuffle_epi32(c, 0x0);
  filter[1] = _mm256_shuffle_epi8(c, _mm256_set1_epi32(0x03020504));
  filter[2] =
      _mm256_set1_epi32((1 << 16) | static_cast<uint16_t>(coefficients[0]));
  for (int y = height >> 1; y > 0; --y) {
    ptrdiff_t x = 0;
    do {
      __m256i d[2];
      WienerVerticalTap5Kernel2(wiener_buffer + x, width, filter, d);
      StoreUnaligned32(dst + x, d[0]);
      StoreUnaligned32(dst + dst_stride + x, d[1]);
      x += 16;
    } while (x < width);
    dst += 2 * dst_stride;
    wiener_buffer += 2 * width;
  }

  if ((height & 1) != 0) {
    ptrdiff_t x = 0;
    do {
      __m256i a[5];
      const __m256i d =
          WienerVerticalTap5Kernel(wiener_buffer + x, width, filter, a);
      StoreUnaligned32(dst + x, d);
      x += 16;
    } while (x < width);
  }
}

inline void WienerVerticalTap3(const int16_t* wiener_buffer,
                               const ptrdiff_t width, const int height,
                               const int16_t coefficients[2], uint16_t* dst,
                               const ptrdiff_t dst_stride) {
  __m256i filter[2];
  filter[0] =
      _mm256_set1_epi32(*reinterpret_cast<const int32_t*>(coefficients));
  filter[1] =
      _mm256_set1_epi32((1 << 16) | static_cast<uint16_t>(coefficients[0]));
  for (int y = height >> 1; y > 0; --y) {
    ptrdiff_t x = 0;
    do {
      __m256i d[2][2];
      WienerVerticalTap3Kernel2(wiener_buffer + x, width, filter, d[0]);
      StoreUnaligned32(dst + x, d[0][0]);
      StoreUnaligned32(dst + dst_stride + x, d[0][1]);
      x += 16;
    } while (x < width);
    dst += 2 * dst_stride;
    wiener_buffer += 2 * width;
  }

  if ((height & 1) != 0) {
    ptrdiff_t x = 0;
    do {
      __m256i a[3];
      const __m256i d =
          WienerVerticalTap3Kernel(wiener_buffer + x, width, filter, a);
      StoreUnaligned32(dst + x, d);
      x += 16;
    } while (x < width);
  }
}

inline void WienerVerticalTap1Kernel(const int16_t* const wiener_buffer,
                                     uint16_t* const dst) {
  const __m256i a = LoadAligned32(wiener_buffer);
  const __m256i b = _mm256_add_epi16(a, _mm256_set1_epi16(8));
  const __m256i c = _mm256_srai_epi16(b, 4);
  const __m256i d = _mm256_max_epi16(c, _mm256_setzero_si256());
  const __m256i e = _mm256_min_epi16(d, _mm256_set1_epi16(1023));
  StoreUnaligned32(dst, e);
}

inline void WienerVerticalTap1(const int16_t* wiener_buffer,
                               const ptrdiff_t width, const int height,
                               uint16_t* dst, const ptrdiff_t dst_stride) {
  for (int y = height >> 1; y > 0; --y) {
    ptrdiff_t x = 0;
    do {
      WienerVerticalTap1Kernel(wiener_buffer + x, dst + x);
      WienerVerticalTap1Kernel(wiener_buffer + width + x, dst + dst_stride + x);
      x += 16;
    } while (x < width);
    dst += 2 * dst_stride;
    wiener_buffer += 2 * width;
  }

  if ((height & 1) != 0) {
    ptrdiff_t x = 0;
    do {
      WienerVerticalTap1Kernel(wiener_buffer + x, dst + x);
      x += 16;
    } while (x < width);
  }
}

void WienerFilter_AVX2(
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

  // horizontal filtering.
  // Over-reads up to 15 - |kRestorationHorizontalBorder| values.
  const int height_horizontal =
      height + kWienerFilterTaps - 1 - 2 * number_rows_to_skip;
  const int height_extra = (height_horizontal - height) >> 1;
  assert(height_extra <= 2);
  const auto* const src = static_cast<const uint16_t*>(source);
  const auto* const top = static_cast<const uint16_t*>(top_border);
  const auto* const bottom = static_cast<const uint16_t*>(bottom_border);
  const __m128i c =
      LoadLo8(restoration_info.wiener_info.filter[WienerInfo::kHorizontal]);
  const __m256i coefficients_horizontal = _mm256_broadcastq_epi64(c);
  if (number_leading_zero_coefficients[WienerInfo::kHorizontal] == 0) {
    WienerHorizontalTap7(top + (2 - height_extra) * top_border_stride - 3,
                         top_border_stride, wiener_stride, height_extra,
                         &coefficients_horizontal, &wiener_buffer_horizontal);
    WienerHorizontalTap7(src - 3, stride, wiener_stride, height,
                         &coefficients_horizontal, &wiener_buffer_horizontal);
    WienerHorizontalTap7(bottom - 3, bottom_border_stride, wiener_stride,
                         height_extra, &coefficients_horizontal,
                         &wiener_buffer_horizontal);
  } else if (number_leading_zero_coefficients[WienerInfo::kHorizontal] == 1) {
    WienerHorizontalTap5(top + (2 - height_extra) * top_border_stride - 2,
                         top_border_stride, wiener_stride, height_extra,
                         &coefficients_horizontal, &wiener_buffer_horizontal);
    WienerHorizontalTap5(src - 2, stride, wiener_stride, height,
                         &coefficients_horizontal, &wiener_buffer_horizontal);
    WienerHorizontalTap5(bottom - 2, bottom_border_stride, wiener_stride,
                         height_extra, &coefficients_horizontal,
                         &wiener_buffer_horizontal);
  } else if (number_leading_zero_coefficients[WienerInfo::kHorizontal] == 2) {
    // The maximum over-reads happen here.
    WienerHorizontalTap3(top + (2 - height_extra) * top_border_stride - 1,
                         top_border_stride, wiener_stride, height_extra,
                         &coefficients_horizontal, &wiener_buffer_horizontal);
    WienerHorizontalTap3(src - 1, stride, wiener_stride, height,
                         &coefficients_horizontal, &wiener_buffer_horizontal);
    WienerHorizontalTap3(bottom - 1, bottom_border_stride, wiener_stride,
                         height_extra, &coefficients_horizontal,
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
  // Over-writes up to 15 values.
  const int16_t* const filter_vertical =
      restoration_info.wiener_info.filter[WienerInfo::kVertical];
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
                       height, filter_vertical + 1, dst, stride);
  } else if (number_leading_zero_coefficients[WienerInfo::kVertical] == 2) {
    WienerVerticalTap3(wiener_buffer_vertical + 2 * wiener_stride,
                       wiener_stride, height, filter_vertical + 2, dst, stride);
  } else {
    assert(number_leading_zero_coefficients[WienerInfo::kVertical] == 3);
    WienerVerticalTap1(wiener_buffer_vertical + 3 * wiener_stride,
                       wiener_stride, height, dst, stride);
  }
}

//------------------------------------------------------------------------------
// SGR

constexpr int kSumOffset = 24;

// SIMD overreads the number of pixels in SIMD registers - (width % 8) - 2 *
// padding pixels, where padding is 3 for Pass 1 and 2 for Pass 2. The number of
// bytes in SIMD registers is 16 for SSE4.1 and 32 for AVX2.
constexpr int kOverreadInBytesPass1_128 = 4;
constexpr int kOverreadInBytesPass2_128 = 8;
constexpr int kOverreadInBytesPass1_256 = kOverreadInBytesPass1_128 + 16;
constexpr int kOverreadInBytesPass2_256 = kOverreadInBytesPass2_128 + 16;

inline void LoadAligned16x2U16(const uint16_t* const src[2], const ptrdiff_t x,
                               __m128i dst[2]) {
  dst[0] = LoadAligned16(src[0] + x);
  dst[1] = LoadAligned16(src[1] + x);
}

inline void LoadAligned32x2U16(const uint16_t* const src[2], const ptrdiff_t x,
                               __m256i dst[2]) {
  dst[0] = LoadAligned32(src[0] + x);
  dst[1] = LoadAligned32(src[1] + x);
}

inline void LoadAligned32x2U16Msan(const uint16_t* const src[2],
                                   const ptrdiff_t x, const ptrdiff_t border,
                                   __m256i dst[2]) {
  dst[0] = LoadAligned32Msan(src[0] + x, sizeof(**src) * (x + 16 - border));
  dst[1] = LoadAligned32Msan(src[1] + x, sizeof(**src) * (x + 16 - border));
}

inline void LoadAligned16x3U16(const uint16_t* const src[3], const ptrdiff_t x,
                               __m128i dst[3]) {
  dst[0] = LoadAligned16(src[0] + x);
  dst[1] = LoadAligned16(src[1] + x);
  dst[2] = LoadAligned16(src[2] + x);
}

inline void LoadAligned32x3U16(const uint16_t* const src[3], const ptrdiff_t x,
                               __m256i dst[3]) {
  dst[0] = LoadAligned32(src[0] + x);
  dst[1] = LoadAligned32(src[1] + x);
  dst[2] = LoadAligned32(src[2] + x);
}

inline void LoadAligned32x3U16Msan(const uint16_t* const src[3],
                                   const ptrdiff_t x, const ptrdiff_t border,
                                   __m256i dst[3]) {
  dst[0] = LoadAligned32Msan(src[0] + x, sizeof(**src) * (x + 16 - border));
  dst[1] = LoadAligned32Msan(src[1] + x, sizeof(**src) * (x + 16 - border));
  dst[2] = LoadAligned32Msan(src[2] + x, sizeof(**src) * (x + 16 - border));
}

inline void LoadAligned32U32(const uint32_t* const src, __m128i dst[2]) {
  dst[0] = LoadAligned16(src + 0);
  dst[1] = LoadAligned16(src + 4);
}

inline void LoadAligned32x2U32(const uint32_t* const src[2], const ptrdiff_t x,
                               __m128i dst[2][2]) {
  LoadAligned32U32(src[0] + x, dst[0]);
  LoadAligned32U32(src[1] + x, dst[1]);
}

inline void LoadAligned64x2U32(const uint32_t* const src[2], const ptrdiff_t x,
                               __m256i dst[2][2]) {
  LoadAligned64(src[0] + x, dst[0]);
  LoadAligned64(src[1] + x, dst[1]);
}

inline void LoadAligned64x2U32Msan(const uint32_t* const src[2],
                                   const ptrdiff_t x, const ptrdiff_t border,
                                   __m256i dst[2][2]) {
  LoadAligned64Msan(src[0] + x, sizeof(**src) * (x + 16 - border), dst[0]);
  LoadAligned64Msan(src[1] + x, sizeof(**src) * (x + 16 - border), dst[1]);
}

inline void LoadAligned32x3U32(const uint32_t* const src[3], const ptrdiff_t x,
                               __m128i dst[3][2]) {
  LoadAligned32U32(src[0] + x, dst[0]);
  LoadAligned32U32(src[1] + x, dst[1]);
  LoadAligned32U32(src[2] + x, dst[2]);
}

inline void LoadAligned64x3U32(const uint32_t* const src[3], const ptrdiff_t x,
                               __m256i dst[3][2]) {
  LoadAligned64(src[0] + x, dst[0]);
  LoadAligned64(src[1] + x, dst[1]);
  LoadAligned64(src[2] + x, dst[2]);
}

inline void LoadAligned64x3U32Msan(const uint32_t* const src[3],
                                   const ptrdiff_t x, const ptrdiff_t border,
                                   __m256i dst[3][2]) {
  LoadAligned64Msan(src[0] + x, sizeof(**src) * (x + 16 - border), dst[0]);
  LoadAligned64Msan(src[1] + x, sizeof(**src) * (x + 16 - border), dst[1]);
  LoadAligned64Msan(src[2] + x, sizeof(**src) * (x + 16 - border), dst[2]);
}

inline void StoreAligned32U32(uint32_t* const dst, const __m128i src[2]) {
  StoreAligned16(dst + 0, src[0]);
  StoreAligned16(dst + 4, src[1]);
}

// The AVX2 ymm register holds ma[0], ma[1], ..., ma[7], and ma[16], ma[17],
// ..., ma[23].
// There is an 8 pixel gap between the first half and the second half.
constexpr int kMaStoreOffset = 8;

inline void StoreAligned32_ma(uint16_t* src, const __m256i v) {
  StoreAligned16(src + 0 * 8, _mm256_extracti128_si256(v, 0));
  StoreAligned16(src + 2 * 8, _mm256_extracti128_si256(v, 1));
}

inline void StoreAligned64_ma(uint16_t* src, const __m256i v[2]) {
  // The next 4 lines are much faster than:
  // StoreAligned32(src + 0, _mm256_permute2x128_si256(v[0], v[1], 0x20));
  // StoreAligned32(src + 16, _mm256_permute2x128_si256(v[0], v[1], 0x31));
  StoreAligned16(src + 0 * 8, _mm256_extracti128_si256(v[0], 0));
  StoreAligned16(src + 1 * 8, _mm256_extracti128_si256(v[1], 0));
  StoreAligned16(src + 2 * 8, _mm256_extracti128_si256(v[0], 1));
  StoreAligned16(src + 3 * 8, _mm256_extracti128_si256(v[1], 1));
}

// Don't use _mm_cvtepu8_epi16() or _mm_cvtepu16_epi32() in the following
// functions. Some compilers may generate super inefficient code and the whole
// decoder could be 15% slower.

inline __m256i VaddlLo8(const __m256i src0, const __m256i src1) {
  const __m256i s0 = _mm256_unpacklo_epi8(src0, _mm256_setzero_si256());
  const __m256i s1 = _mm256_unpacklo_epi8(src1, _mm256_setzero_si256());
  return _mm256_add_epi16(s0, s1);
}

inline __m256i VaddlHi8(const __m256i src0, const __m256i src1) {
  const __m256i s0 = _mm256_unpackhi_epi8(src0, _mm256_setzero_si256());
  const __m256i s1 = _mm256_unpackhi_epi8(src1, _mm256_setzero_si256());
  return _mm256_add_epi16(s0, s1);
}

inline __m256i VaddwLo8(const __m256i src0, const __m256i src1) {
  const __m256i s1 = _mm256_unpacklo_epi8(src1, _mm256_setzero_si256());
  return _mm256_add_epi16(src0, s1);
}

inline __m256i VaddwHi8(const __m256i src0, const __m256i src1) {
  const __m256i s1 = _mm256_unpackhi_epi8(src1, _mm256_setzero_si256());
  return _mm256_add_epi16(src0, s1);
}

inline __m256i VmullNLo8(const __m256i src0, const int src1) {
  const __m256i s0 = _mm256_unpacklo_epi16(src0, _mm256_setzero_si256());
  return _mm256_madd_epi16(s0, _mm256_set1_epi32(src1));
}

inline __m256i VmullNHi8(const __m256i src0, const int src1) {
  const __m256i s0 = _mm256_unpackhi_epi16(src0, _mm256_setzero_si256());
  return _mm256_madd_epi16(s0, _mm256_set1_epi32(src1));
}

inline __m128i VmullLo16(const __m128i src0, const __m128i src1) {
  const __m128i s0 = _mm_unpacklo_epi16(src0, _mm_setzero_si128());
  const __m128i s1 = _mm_unpacklo_epi16(src1, _mm_setzero_si128());
  return _mm_madd_epi16(s0, s1);
}

inline __m256i VmullLo16(const __m256i src0, const __m256i src1) {
  const __m256i s0 = _mm256_unpacklo_epi16(src0, _mm256_setzero_si256());
  const __m256i s1 = _mm256_unpacklo_epi16(src1, _mm256_setzero_si256());
  return _mm256_madd_epi16(s0, s1);
}

inline __m128i VmullHi16(const __m128i src0, const __m128i src1) {
  const __m128i s0 = _mm_unpackhi_epi16(src0, _mm_setzero_si128());
  const __m128i s1 = _mm_unpackhi_epi16(src1, _mm_setzero_si128());
  return _mm_madd_epi16(s0, s1);
}

inline __m256i VmullHi16(const __m256i src0, const __m256i src1) {
  const __m256i s0 = _mm256_unpackhi_epi16(src0, _mm256_setzero_si256());
  const __m256i s1 = _mm256_unpackhi_epi16(src1, _mm256_setzero_si256());
  return _mm256_madd_epi16(s0, s1);
}

inline __m128i VrshrU16(const __m128i src0, const int src1) {
  const __m128i sum = _mm_add_epi16(src0, _mm_set1_epi16(1 << (src1 - 1)));
  return _mm_srli_epi16(sum, src1);
}

inline __m256i VrshrU16(const __m256i src0, const int src1) {
  const __m256i sum =
      _mm256_add_epi16(src0, _mm256_set1_epi16(1 << (src1 - 1)));
  return _mm256_srli_epi16(sum, src1);
}

inline __m256i VrshrS32(const __m256i src0, const int src1) {
  const __m256i sum =
      _mm256_add_epi32(src0, _mm256_set1_epi32(1 << (src1 - 1)));
  return _mm256_srai_epi32(sum, src1);
}

inline __m128i VrshrU32(const __m128i src0, const int src1) {
  const __m128i sum = _mm_add_epi32(src0, _mm_set1_epi32(1 << (src1 - 1)));
  return _mm_srli_epi32(sum, src1);
}

inline __m256i VrshrU32(const __m256i src0, const int src1) {
  const __m256i sum =
      _mm256_add_epi32(src0, _mm256_set1_epi32(1 << (src1 - 1)));
  return _mm256_srli_epi32(sum, src1);
}

inline void Square(const __m128i src, __m128i dst[2]) {
  const __m128i s0 = _mm_unpacklo_epi16(src, _mm_setzero_si128());
  const __m128i s1 = _mm_unpackhi_epi16(src, _mm_setzero_si128());
  dst[0] = _mm_madd_epi16(s0, s0);
  dst[1] = _mm_madd_epi16(s1, s1);
}

inline void Square(const __m256i src, __m256i dst[2]) {
  const __m256i s0 = _mm256_unpacklo_epi16(src, _mm256_setzero_si256());
  const __m256i s1 = _mm256_unpackhi_epi16(src, _mm256_setzero_si256());
  dst[0] = _mm256_madd_epi16(s0, s0);
  dst[1] = _mm256_madd_epi16(s1, s1);
}

inline void Prepare3_8(const __m256i src[2], __m256i dst[3]) {
  dst[0] = _mm256_alignr_epi8(src[1], src[0], 0);
  dst[1] = _mm256_alignr_epi8(src[1], src[0], 1);
  dst[2] = _mm256_alignr_epi8(src[1], src[0], 2);
}

inline void Prepare3_16(const __m128i src[2], __m128i dst[3]) {
  dst[0] = src[0];
  dst[1] = _mm_alignr_epi8(src[1], src[0], 2);
  dst[2] = _mm_alignr_epi8(src[1], src[0], 4);
}

inline void Prepare3_32(const __m128i src[2], __m128i dst[3]) {
  dst[0] = src[0];
  dst[1] = _mm_alignr_epi8(src[1], src[0], 4);
  dst[2] = _mm_alignr_epi8(src[1], src[0], 8);
}

inline void Prepare3_32(const __m256i src[2], __m256i dst[3]) {
  dst[0] = src[0];
  dst[1] = _mm256_alignr_epi8(src[1], src[0], 4);
  dst[2] = _mm256_alignr_epi8(src[1], src[0], 8);
}

inline void Prepare5_16(const __m128i src[2], __m128i dst[5]) {
  Prepare3_16(src, dst);
  dst[3] = _mm_alignr_epi8(src[1], src[0], 6);
  dst[4] = _mm_alignr_epi8(src[1], src[0], 8);
}

inline void Prepare5_32(const __m128i src[2], __m128i dst[5]) {
  Prepare3_32(src, dst);
  dst[3] = _mm_alignr_epi8(src[1], src[0], 12);
  dst[4] = src[1];
}

inline void Prepare5_32(const __m256i src[2], __m256i dst[5]) {
  Prepare3_32(src, dst);
  dst[3] = _mm256_alignr_epi8(src[1], src[0], 12);
  dst[4] = src[1];
}

inline __m128i Sum3_16(const __m128i src0, const __m128i src1,
                       const __m128i src2) {
  const __m128i sum = _mm_add_epi16(src0, src1);
  return _mm_add_epi16(sum, src2);
}

inline __m256i Sum3_16(const __m256i src0, const __m256i src1,
                       const __m256i src2) {
  const __m256i sum = _mm256_add_epi16(src0, src1);
  return _mm256_add_epi16(sum, src2);
}

inline __m128i Sum3_16(const __m128i src[3]) {
  return Sum3_16(src[0], src[1], src[2]);
}

inline __m256i Sum3_16(const __m256i src[3]) {
  return Sum3_16(src[0], src[1], src[2]);
}

inline __m128i Sum3_32(const __m128i src0, const __m128i src1,
                       const __m128i src2) {
  const __m128i sum = _mm_add_epi32(src0, src1);
  return _mm_add_epi32(sum, src2);
}

inline __m256i Sum3_32(const __m256i src0, const __m256i src1,
                       const __m256i src2) {
  const __m256i sum = _mm256_add_epi32(src0, src1);
  return _mm256_add_epi32(sum, src2);
}

inline __m128i Sum3_32(const __m128i src[3]) {
  return Sum3_32(src[0], src[1], src[2]);
}

inline __m256i Sum3_32(const __m256i src[3]) {
  return Sum3_32(src[0], src[1], src[2]);
}

inline void Sum3_32(const __m128i src[3][2], __m128i dst[2]) {
  dst[0] = Sum3_32(src[0][0], src[1][0], src[2][0]);
  dst[1] = Sum3_32(src[0][1], src[1][1], src[2][1]);
}

inline void Sum3_32(const __m256i src[3][2], __m256i dst[2]) {
  dst[0] = Sum3_32(src[0][0], src[1][0], src[2][0]);
  dst[1] = Sum3_32(src[0][1], src[1][1], src[2][1]);
}

inline __m256i Sum3WLo16(const __m256i src[3]) {
  const __m256i sum = VaddlLo8(src[0], src[1]);
  return VaddwLo8(sum, src[2]);
}

inline __m256i Sum3WHi16(const __m256i src[3]) {
  const __m256i sum = VaddlHi8(src[0], src[1]);
  return VaddwHi8(sum, src[2]);
}

inline __m128i Sum5_16(const __m128i src[5]) {
  const __m128i sum01 = _mm_add_epi16(src[0], src[1]);
  const __m128i sum23 = _mm_add_epi16(src[2], src[3]);
  const __m128i sum = _mm_add_epi16(sum01, sum23);
  return _mm_add_epi16(sum, src[4]);
}

inline __m256i Sum5_16(const __m256i src[5]) {
  const __m256i sum01 = _mm256_add_epi16(src[0], src[1]);
  const __m256i sum23 = _mm256_add_epi16(src[2], src[3]);
  const __m256i sum = _mm256_add_epi16(sum01, sum23);
  return _mm256_add_epi16(sum, src[4]);
}

inline __m128i Sum5_32(const __m128i* const src0, const __m128i* const src1,
                       const __m128i* const src2, const __m128i* const src3,
                       const __m128i* const src4) {
  const __m128i sum01 = _mm_add_epi32(*src0, *src1);
  const __m128i sum23 = _mm_add_epi32(*src2, *src3);
  const __m128i sum = _mm_add_epi32(sum01, sum23);
  return _mm_add_epi32(sum, *src4);
}

inline __m256i Sum5_32(const __m256i* const src0, const __m256i* const src1,
                       const __m256i* const src2, const __m256i* const src3,
                       const __m256i* const src4) {
  const __m256i sum01 = _mm256_add_epi32(*src0, *src1);
  const __m256i sum23 = _mm256_add_epi32(*src2, *src3);
  const __m256i sum = _mm256_add_epi32(sum01, sum23);
  return _mm256_add_epi32(sum, *src4);
}

inline __m128i Sum5_32(const __m128i src[5]) {
  return Sum5_32(&src[0], &src[1], &src[2], &src[3], &src[4]);
}

inline __m256i Sum5_32(const __m256i src[5]) {
  return Sum5_32(&src[0], &src[1], &src[2], &src[3], &src[4]);
}

inline void Sum5_32(const __m128i src[5][2], __m128i dst[2]) {
  dst[0] = Sum5_32(&src[0][0], &src[1][0], &src[2][0], &src[3][0], &src[4][0]);
  dst[1] = Sum5_32(&src[0][1], &src[1][1], &src[2][1], &src[3][1], &src[4][1]);
}

inline void Sum5_32(const __m256i src[5][2], __m256i dst[2]) {
  dst[0] = Sum5_32(&src[0][0], &src[1][0], &src[2][0], &src[3][0], &src[4][0]);
  dst[1] = Sum5_32(&src[0][1], &src[1][1], &src[2][1], &src[3][1], &src[4][1]);
}

inline __m128i Sum3Horizontal16(const __m128i src[2]) {
  __m128i s[3];
  Prepare3_16(src, s);
  return Sum3_16(s);
}

inline __m256i Sum3Horizontal16(const uint16_t* const src,
                                const ptrdiff_t over_read_in_bytes) {
  __m256i s[3];
  s[0] = LoadUnaligned32Msan(src + 0, over_read_in_bytes + 0);
  s[1] = LoadUnaligned32Msan(src + 1, over_read_in_bytes + 2);
  s[2] = LoadUnaligned32Msan(src + 2, over_read_in_bytes + 4);
  return Sum3_16(s);
}

inline __m128i Sum5Horizontal16(const __m128i src[2]) {
  __m128i s[5];
  Prepare5_16(src, s);
  return Sum5_16(s);
}

inline __m256i Sum5Horizontal16(const uint16_t* const src,
                                const ptrdiff_t over_read_in_bytes) {
  __m256i s[5];
  s[0] = LoadUnaligned32Msan(src + 0, over_read_in_bytes + 0);
  s[1] = LoadUnaligned32Msan(src + 1, over_read_in_bytes + 2);
  s[2] = LoadUnaligned32Msan(src + 2, over_read_in_bytes + 4);
  s[3] = LoadUnaligned32Msan(src + 3, over_read_in_bytes + 6);
  s[4] = LoadUnaligned32Msan(src + 4, over_read_in_bytes + 8);
  return Sum5_16(s);
}

inline void SumHorizontal16(const uint16_t* const src,
                            const ptrdiff_t over_read_in_bytes,
                            __m256i* const row3, __m256i* const row5) {
  __m256i s[5];
  s[0] = LoadUnaligned32Msan(src + 0, over_read_in_bytes + 0);
  s[1] = LoadUnaligned32Msan(src + 1, over_read_in_bytes + 2);
  s[2] = LoadUnaligned32Msan(src + 2, over_read_in_bytes + 4);
  s[3] = LoadUnaligned32Msan(src + 3, over_read_in_bytes + 6);
  s[4] = LoadUnaligned32Msan(src + 4, over_read_in_bytes + 8);
  const __m256i sum04 = _mm256_add_epi16(s[0], s[4]);
  *row3 = Sum3_16(s + 1);
  *row5 = _mm256_add_epi16(sum04, *row3);
}

inline void SumHorizontal16(const uint16_t* const src,
                            const ptrdiff_t over_read_in_bytes,
                            __m256i* const row3_0, __m256i* const row3_1,
                            __m256i* const row5_0, __m256i* const row5_1) {
  SumHorizontal16(src + 0, over_read_in_bytes + 0, row3_0, row5_0);
  SumHorizontal16(src + 16, over_read_in_bytes + 32, row3_1, row5_1);
}

inline void SumHorizontal32(const __m128i src[5], __m128i* const row_sq3,
                            __m128i* const row_sq5) {
  const __m128i sum04 = _mm_add_epi32(src[0], src[4]);
  *row_sq3 = Sum3_32(src + 1);
  *row_sq5 = _mm_add_epi32(sum04, *row_sq3);
}

inline void SumHorizontal32(const __m256i src[5], __m256i* const row_sq3,
                            __m256i* const row_sq5) {
  const __m256i sum04 = _mm256_add_epi32(src[0], src[4]);
  *row_sq3 = Sum3_32(src + 1);
  *row_sq5 = _mm256_add_epi32(sum04, *row_sq3);
}

inline void SumHorizontal32(const __m128i src[3], __m128i* const row_sq3_0,
                            __m128i* const row_sq3_1, __m128i* const row_sq5_0,
                            __m128i* const row_sq5_1) {
  __m128i s[5];
  Prepare5_32(src + 0, s);
  SumHorizontal32(s, row_sq3_0, row_sq5_0);
  Prepare5_32(src + 1, s);
  SumHorizontal32(s, row_sq3_1, row_sq5_1);
}

inline void SumHorizontal32(const __m256i src[3], __m256i* const row_sq3_0,
                            __m256i* const row_sq3_1, __m256i* const row_sq5_0,
                            __m256i* const row_sq5_1) {
  __m256i s[5];
  Prepare5_32(src + 0, s);
  SumHorizontal32(s, row_sq3_0, row_sq5_0);
  Prepare5_32(src + 1, s);
  SumHorizontal32(s, row_sq3_1, row_sq5_1);
}

inline void Sum3Horizontal32(const __m128i src[3], __m128i dst[2]) {
  __m128i s[3];
  Prepare3_32(src + 0, s);
  dst[0] = Sum3_32(s);
  Prepare3_32(src + 1, s);
  dst[1] = Sum3_32(s);
}

inline void Sum3Horizontal32(const __m256i src[3], __m256i dst[2]) {
  __m256i s[3];
  Prepare3_32(src + 0, s);
  dst[0] = Sum3_32(s);
  Prepare3_32(src + 1, s);
  dst[1] = Sum3_32(s);
}

inline void Sum5Horizontal32(const __m128i src[3], __m128i dst[2]) {
  __m128i s[5];
  Prepare5_32(src + 0, s);
  dst[0] = Sum5_32(s);
  Prepare5_32(src + 1, s);
  dst[1] = Sum5_32(s);
}

inline void Sum5Horizontal32(const __m256i src[3], __m256i dst[2]) {
  __m256i s[5];
  Prepare5_32(src + 0, s);
  dst[0] = Sum5_32(s);
  Prepare5_32(src + 1, s);
  dst[1] = Sum5_32(s);
}

void SumHorizontal16(const __m128i src[2], __m128i* const row3,
                     __m128i* const row5) {
  __m128i s[5];
  Prepare5_16(src, s);
  const __m128i sum04 = _mm_add_epi16(s[0], s[4]);
  *row3 = Sum3_16(s + 1);
  *row5 = _mm_add_epi16(sum04, *row3);
}

inline __m256i Sum343Lo(const __m256i ma3[3]) {
  const __m256i sum = Sum3WLo16(ma3);
  const __m256i sum3 = Sum3_16(sum, sum, sum);
  return VaddwLo8(sum3, ma3[1]);
}

inline __m256i Sum343Hi(const __m256i ma3[3]) {
  const __m256i sum = Sum3WHi16(ma3);
  const __m256i sum3 = Sum3_16(sum, sum, sum);
  return VaddwHi8(sum3, ma3[1]);
}

inline __m256i Sum343(const __m256i src[3]) {
  const __m256i sum = Sum3_32(src);
  const __m256i sum3 = Sum3_32(sum, sum, sum);
  return _mm256_add_epi32(sum3, src[1]);
}

inline void Sum343(const __m256i src[3], __m256i dst[2]) {
  __m256i s[3];
  Prepare3_32(src + 0, s);
  dst[0] = Sum343(s);
  Prepare3_32(src + 1, s);
  dst[1] = Sum343(s);
}

inline __m256i Sum565Lo(const __m256i src[3]) {
  const __m256i sum = Sum3WLo16(src);
  const __m256i sum4 = _mm256_slli_epi16(sum, 2);
  const __m256i sum5 = _mm256_add_epi16(sum4, sum);
  return VaddwLo8(sum5, src[1]);
}

inline __m256i Sum565Hi(const __m256i src[3]) {
  const __m256i sum = Sum3WHi16(src);
  const __m256i sum4 = _mm256_slli_epi16(sum, 2);
  const __m256i sum5 = _mm256_add_epi16(sum4, sum);
  return VaddwHi8(sum5, src[1]);
}

inline __m256i Sum565(const __m256i src[3]) {
  const __m256i sum = Sum3_32(src);
  const __m256i sum4 = _mm256_slli_epi32(sum, 2);
  const __m256i sum5 = _mm256_add_epi32(sum4, sum);
  return _mm256_add_epi32(sum5, src[1]);
}

inline void Sum565(const __m256i src[3], __m256i dst[2]) {
  __m256i s[3];
  Prepare3_32(src + 0, s);
  dst[0] = Sum565(s);
  Prepare3_32(src + 1, s);
  dst[1] = Sum565(s);
}

inline void BoxSum(const uint16_t* src, const ptrdiff_t src_stride,
                   const ptrdiff_t width, const ptrdiff_t sum_stride,
                   const ptrdiff_t sum_width, uint16_t* sum3, uint16_t* sum5,
                   uint32_t* square_sum3, uint32_t* square_sum5) {
  const ptrdiff_t overread_in_bytes_128 =
      kOverreadInBytesPass1_128 - sizeof(*src) * width;
  const ptrdiff_t overread_in_bytes_256 =
      kOverreadInBytesPass1_256 - sizeof(*src) * width;
  int y = 2;
  do {
    __m128i s0[2], sq_128[4], s3, s5, sq3[2], sq5[2];
    __m256i sq[8];
    s0[0] = LoadUnaligned16Msan(src + 0, overread_in_bytes_128 + 0);
    s0[1] = LoadUnaligned16Msan(src + 8, overread_in_bytes_128 + 16);
    Square(s0[0], sq_128 + 0);
    Square(s0[1], sq_128 + 2);
    SumHorizontal16(s0, &s3, &s5);
    StoreAligned16(sum3, s3);
    StoreAligned16(sum5, s5);
    SumHorizontal32(sq_128, &sq3[0], &sq3[1], &sq5[0], &sq5[1]);
    StoreAligned32U32(square_sum3, sq3);
    StoreAligned32U32(square_sum5, sq5);
    src += 8;
    sum3 += 8;
    sum5 += 8;
    square_sum3 += 8;
    square_sum5 += 8;
    sq[0] = SetrM128i(sq_128[2], sq_128[2]);
    sq[1] = SetrM128i(sq_128[3], sq_128[3]);
    ptrdiff_t x = sum_width;
    do {
      __m256i s[2], row3[2], row5[2], row_sq3[2], row_sq5[2];
      s[0] = LoadUnaligned32Msan(
          src + 8, overread_in_bytes_256 + sizeof(*src) * (sum_width - x + 8));
      s[1] = LoadUnaligned32Msan(
          src + 24,
          overread_in_bytes_256 + sizeof(*src) * (sum_width - x + 24));
      Square(s[0], sq + 2);
      Square(s[1], sq + 6);
      sq[0] = _mm256_permute2x128_si256(sq[0], sq[2], 0x21);
      sq[1] = _mm256_permute2x128_si256(sq[1], sq[3], 0x21);
      sq[4] = _mm256_permute2x128_si256(sq[2], sq[6], 0x21);
      sq[5] = _mm256_permute2x128_si256(sq[3], sq[7], 0x21);
      SumHorizontal16(
          src, overread_in_bytes_256 + sizeof(*src) * (sum_width - x + 8),
          &row3[0], &row3[1], &row5[0], &row5[1]);
      StoreAligned64(sum3, row3);
      StoreAligned64(sum5, row5);
      SumHorizontal32(sq + 0, &row_sq3[0], &row_sq3[1], &row_sq5[0],
                      &row_sq5[1]);
      StoreAligned64(square_sum3 + 0, row_sq3);
      StoreAligned64(square_sum5 + 0, row_sq5);
      SumHorizontal32(sq + 4, &row_sq3[0], &row_sq3[1], &row_sq5[0],
                      &row_sq5[1]);
      StoreAligned64(square_sum3 + 16, row_sq3);
      StoreAligned64(square_sum5 + 16, row_sq5);
      sq[0] = sq[6];
      sq[1] = sq[7];
      src += 32;
      sum3 += 32;
      sum5 += 32;
      square_sum3 += 32;
      square_sum5 += 32;
      x -= 32;
    } while (x != 0);
    src += src_stride - sum_width - 8;
    sum3 += sum_stride - sum_width - 8;
    sum5 += sum_stride - sum_width - 8;
    square_sum3 += sum_stride - sum_width - 8;
    square_sum5 += sum_stride - sum_width - 8;
  } while (--y != 0);
}

template <int size>
inline void BoxSum(const uint16_t* src, const ptrdiff_t src_stride,
                   const ptrdiff_t width, const ptrdiff_t sum_stride,
                   const ptrdiff_t sum_width, uint16_t* sums,
                   uint32_t* square_sums) {
  static_assert(size == 3 || size == 5, "");
  int overread_in_bytes_128, overread_in_bytes_256;
  if (size == 3) {
    overread_in_bytes_128 = kOverreadInBytesPass2_128;
    overread_in_bytes_256 = kOverreadInBytesPass2_256;
  } else {
    overread_in_bytes_128 = kOverreadInBytesPass1_128;
    overread_in_bytes_256 = kOverreadInBytesPass1_256;
  }
  overread_in_bytes_128 -= sizeof(*src) * width;
  overread_in_bytes_256 -= sizeof(*src) * width;
  int y = 2;
  do {
    __m128i s_128[2], ss, sq_128[4], sqs[2];
    __m256i sq[8];
    s_128[0] = LoadUnaligned16Msan(src + 0, overread_in_bytes_128);
    s_128[1] = LoadUnaligned16Msan(src + 8, overread_in_bytes_128 + 16);
    Square(s_128[0], sq_128 + 0);
    Square(s_128[1], sq_128 + 2);
    if (size == 3) {
      ss = Sum3Horizontal16(s_128);
      Sum3Horizontal32(sq_128, sqs);
    } else {
      ss = Sum5Horizontal16(s_128);
      Sum5Horizontal32(sq_128, sqs);
    }
    StoreAligned16(sums, ss);
    StoreAligned32U32(square_sums, sqs);
    src += 8;
    sums += 8;
    square_sums += 8;
    sq[0] = SetrM128i(sq_128[2], sq_128[2]);
    sq[1] = SetrM128i(sq_128[3], sq_128[3]);
    ptrdiff_t x = sum_width;
    do {
      __m256i s[2], row[2], row_sq[4];
      s[0] = LoadUnaligned32Msan(
          src + 8, overread_in_bytes_256 + sizeof(*src) * (sum_width - x + 8));
      s[1] = LoadUnaligned32Msan(
          src + 24,
          overread_in_bytes_256 + sizeof(*src) * (sum_width - x + 24));
      Square(s[0], sq + 2);
      Square(s[1], sq + 6);
      sq[0] = _mm256_permute2x128_si256(sq[0], sq[2], 0x21);
      sq[1] = _mm256_permute2x128_si256(sq[1], sq[3], 0x21);
      sq[4] = _mm256_permute2x128_si256(sq[2], sq[6], 0x21);
      sq[5] = _mm256_permute2x128_si256(sq[3], sq[7], 0x21);
      if (size == 3) {
        row[0] = Sum3Horizontal16(
            src, overread_in_bytes_256 + sizeof(*src) * (sum_width - x + 8));
        row[1] =
            Sum3Horizontal16(src + 16, overread_in_bytes_256 +
                                           sizeof(*src) * (sum_width - x + 24));
        Sum3Horizontal32(sq + 0, row_sq + 0);
        Sum3Horizontal32(sq + 4, row_sq + 2);
      } else {
        row[0] = Sum5Horizontal16(
            src, overread_in_bytes_256 + sizeof(*src) * (sum_width - x + 8));
        row[1] =
            Sum5Horizontal16(src + 16, overread_in_bytes_256 +
                                           sizeof(*src) * (sum_width - x + 24));
        Sum5Horizontal32(sq + 0, row_sq + 0);
        Sum5Horizontal32(sq + 4, row_sq + 2);
      }
      StoreAligned64(sums, row);
      StoreAligned64(square_sums + 0, row_sq + 0);
      StoreAligned64(square_sums + 16, row_sq + 2);
      sq[0] = sq[6];
      sq[1] = sq[7];
      src += 32;
      sums += 32;
      square_sums += 32;
      x -= 32;
    } while (x != 0);
    src += src_stride - sum_width - 8;
    sums += sum_stride - sum_width - 8;
    square_sums += sum_stride - sum_width - 8;
  } while (--y != 0);
}

template <int n>
inline __m128i CalculateMa(const __m128i sum, const __m128i sum_sq,
                           const uint32_t scale) {
  static_assert(n == 9 || n == 25, "");
  // a = |sum_sq|
  // d = |sum|
  // p = (a * n < d * d) ? 0 : a * n - d * d;
  const __m128i dxd = _mm_madd_epi16(sum, sum);
  // _mm_mullo_epi32() has high latency. Using shifts and additions instead.
  // Some compilers could do this for us but we make this explicit.
  // return _mm_mullo_epi32(sum_sq, _mm_set1_epi32(n));
  __m128i axn = _mm_add_epi32(sum_sq, _mm_slli_epi32(sum_sq, 3));
  if (n == 25) axn = _mm_add_epi32(axn, _mm_slli_epi32(sum_sq, 4));
  const __m128i sub = _mm_sub_epi32(axn, dxd);
  const __m128i p = _mm_max_epi32(sub, _mm_setzero_si128());
  const __m128i pxs = _mm_mullo_epi32(p, _mm_set1_epi32(scale));
  return VrshrU32(pxs, kSgrProjScaleBits);
}

template <int n>
inline __m128i CalculateMa(const __m128i sum, const __m128i sum_sq[2],
                           const uint32_t scale) {
  static_assert(n == 9 || n == 25, "");
  const __m128i b = VrshrU16(sum, 2);
  const __m128i sum_lo = _mm_unpacklo_epi16(b, _mm_setzero_si128());
  const __m128i sum_hi = _mm_unpackhi_epi16(b, _mm_setzero_si128());
  const __m128i z0 = CalculateMa<n>(sum_lo, VrshrU32(sum_sq[0], 4), scale);
  const __m128i z1 = CalculateMa<n>(sum_hi, VrshrU32(sum_sq[1], 4), scale);
  return _mm_packus_epi32(z0, z1);
}

template <int n>
inline __m256i CalculateMa(const __m256i sum, const __m256i sum_sq,
                           const uint32_t scale) {
  static_assert(n == 9 || n == 25, "");
  // a = |sum_sq|
  // d = |sum|
  // p = (a * n < d * d) ? 0 : a * n - d * d;
  const __m256i dxd = _mm256_madd_epi16(sum, sum);
  // _mm256_mullo_epi32() has high latency. Using shifts and additions instead.
  // Some compilers could do this for us but we make this explicit.
  // return _mm256_mullo_epi32(sum_sq, _mm256_set1_epi32(n));
  __m256i axn = _mm256_add_epi32(sum_sq, _mm256_slli_epi32(sum_sq, 3));
  if (n == 25) axn = _mm256_add_epi32(axn, _mm256_slli_epi32(sum_sq, 4));
  const __m256i sub = _mm256_sub_epi32(axn, dxd);
  const __m256i p = _mm256_max_epi32(sub, _mm256_setzero_si256());
  const __m256i pxs = _mm256_mullo_epi32(p, _mm256_set1_epi32(scale));
  return VrshrU32(pxs, kSgrProjScaleBits);
}

template <int n>
inline __m256i CalculateMa(const __m256i sum, const __m256i sum_sq[2],
                           const uint32_t scale) {
  static_assert(n == 9 || n == 25, "");
  const __m256i b = VrshrU16(sum, 2);
  const __m256i sum_lo = _mm256_unpacklo_epi16(b, _mm256_setzero_si256());
  const __m256i sum_hi = _mm256_unpackhi_epi16(b, _mm256_setzero_si256());
  const __m256i z0 = CalculateMa<n>(sum_lo, VrshrU32(sum_sq[0], 4), scale);
  const __m256i z1 = CalculateMa<n>(sum_hi, VrshrU32(sum_sq[1], 4), scale);
  return _mm256_packus_epi32(z0, z1);
}

inline void CalculateB5(const __m128i sum, const __m128i ma, __m128i b[2]) {
  // one_over_n == 164.
  constexpr uint32_t one_over_n =
      ((1 << kSgrProjReciprocalBits) + (25 >> 1)) / 25;
  // one_over_n_quarter == 41.
  constexpr uint32_t one_over_n_quarter = one_over_n >> 2;
  static_assert(one_over_n == one_over_n_quarter << 2, "");
  // |ma| is in range [0, 255].
  const __m128i m = _mm_maddubs_epi16(ma, _mm_set1_epi16(one_over_n_quarter));
  const __m128i m0 = VmullLo16(m, sum);
  const __m128i m1 = VmullHi16(m, sum);
  b[0] = VrshrU32(m0, kSgrProjReciprocalBits - 2);
  b[1] = VrshrU32(m1, kSgrProjReciprocalBits - 2);
}

inline void CalculateB5(const __m256i sum, const __m256i ma, __m256i b[2]) {
  // one_over_n == 164.
  constexpr uint32_t one_over_n =
      ((1 << kSgrProjReciprocalBits) + (25 >> 1)) / 25;
  // one_over_n_quarter == 41.
  constexpr uint32_t one_over_n_quarter = one_over_n >> 2;
  static_assert(one_over_n == one_over_n_quarter << 2, "");
  // |ma| is in range [0, 255].
  const __m256i m =
      _mm256_maddubs_epi16(ma, _mm256_set1_epi16(one_over_n_quarter));
  const __m256i m0 = VmullLo16(m, sum);
  const __m256i m1 = VmullHi16(m, sum);
  b[0] = VrshrU32(m0, kSgrProjReciprocalBits - 2);
  b[1] = VrshrU32(m1, kSgrProjReciprocalBits - 2);
}

inline void CalculateB3(const __m128i sum, const __m128i ma, __m128i b[2]) {
  // one_over_n == 455.
  constexpr uint32_t one_over_n =
      ((1 << kSgrProjReciprocalBits) + (9 >> 1)) / 9;
  const __m128i m0 = VmullLo16(ma, sum);
  const __m128i m1 = VmullHi16(ma, sum);
  const __m128i m2 = _mm_mullo_epi32(m0, _mm_set1_epi32(one_over_n));
  const __m128i m3 = _mm_mullo_epi32(m1, _mm_set1_epi32(one_over_n));
  b[0] = VrshrU32(m2, kSgrProjReciprocalBits);
  b[1] = VrshrU32(m3, kSgrProjReciprocalBits);
}

inline void CalculateB3(const __m256i sum, const __m256i ma, __m256i b[2]) {
  // one_over_n == 455.
  constexpr uint32_t one_over_n =
      ((1 << kSgrProjReciprocalBits) + (9 >> 1)) / 9;
  const __m256i m0 = VmullLo16(ma, sum);
  const __m256i m1 = VmullHi16(ma, sum);
  const __m256i m2 = _mm256_mullo_epi32(m0, _mm256_set1_epi32(one_over_n));
  const __m256i m3 = _mm256_mullo_epi32(m1, _mm256_set1_epi32(one_over_n));
  b[0] = VrshrU32(m2, kSgrProjReciprocalBits);
  b[1] = VrshrU32(m3, kSgrProjReciprocalBits);
}

inline void CalculateSumAndIndex5(const __m128i s5[5], const __m128i sq5[5][2],
                                  const uint32_t scale, __m128i* const sum,
                                  __m128i* const index) {
  __m128i sum_sq[2];
  *sum = Sum5_16(s5);
  Sum5_32(sq5, sum_sq);
  *index = CalculateMa<25>(*sum, sum_sq, scale);
}

inline void CalculateSumAndIndex5(const __m256i s5[5], const __m256i sq5[5][2],
                                  const uint32_t scale, __m256i* const sum,
                                  __m256i* const index) {
  __m256i sum_sq[2];
  *sum = Sum5_16(s5);
  Sum5_32(sq5, sum_sq);
  *index = CalculateMa<25>(*sum, sum_sq, scale);
}

inline void CalculateSumAndIndex3(const __m128i s3[3], const __m128i sq3[3][2],
                                  const uint32_t scale, __m128i* const sum,
                                  __m128i* const index) {
  __m128i sum_sq[2];
  *sum = Sum3_16(s3);
  Sum3_32(sq3, sum_sq);
  *index = CalculateMa<9>(*sum, sum_sq, scale);
}

inline void CalculateSumAndIndex3(const __m256i s3[3], const __m256i sq3[3][2],
                                  const uint32_t scale, __m256i* const sum,
                                  __m256i* const index) {
  __m256i sum_sq[2];
  *sum = Sum3_16(s3);
  Sum3_32(sq3, sum_sq);
  *index = CalculateMa<9>(*sum, sum_sq, scale);
}

template <int n>
inline void LookupIntermediate(const __m128i sum, const __m128i index,
                               __m128i* const ma, __m128i b[2]) {
  static_assert(n == 9 || n == 25, "");
  const __m128i idx = _mm_packus_epi16(index, index);
  // Actually it's not stored and loaded. The compiler will use a 64-bit
  // general-purpose register to process. Faster than using _mm_extract_epi8().
  uint8_t temp[8];
  StoreLo8(temp, idx);
  *ma = _mm_cvtsi32_si128(kSgrMaLookup[temp[0]]);
  *ma = _mm_insert_epi8(*ma, kSgrMaLookup[temp[1]], 1);
  *ma = _mm_insert_epi8(*ma, kSgrMaLookup[temp[2]], 2);
  *ma = _mm_insert_epi8(*ma, kSgrMaLookup[temp[3]], 3);
  *ma = _mm_insert_epi8(*ma, kSgrMaLookup[temp[4]], 4);
  *ma = _mm_insert_epi8(*ma, kSgrMaLookup[temp[5]], 5);
  *ma = _mm_insert_epi8(*ma, kSgrMaLookup[temp[6]], 6);
  *ma = _mm_insert_epi8(*ma, kSgrMaLookup[temp[7]], 7);
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
  const __m128i maq = _mm_unpacklo_epi8(*ma, _mm_setzero_si128());
  if (n == 9) {
    CalculateB3(sum, maq, b);
  } else {
    CalculateB5(sum, maq, b);
  }
}

// Repeat the first 48 elements in kSgrMaLookup with a period of 16.
alignas(32) constexpr uint8_t kSgrMaLookupAvx2[96] = {
    255, 128, 85, 64, 51, 43, 37, 32, 28, 26, 23, 21, 20, 18, 17, 16,
    255, 128, 85, 64, 51, 43, 37, 32, 28, 26, 23, 21, 20, 18, 17, 16,
    15,  14,  13, 13, 12, 12, 11, 11, 10, 10, 9,  9,  9,  9,  8,  8,
    15,  14,  13, 13, 12, 12, 11, 11, 10, 10, 9,  9,  9,  9,  8,  8,
    8,   8,   7,  7,  7,  7,  7,  6,  6,  6,  6,  6,  6,  6,  5,  5,
    8,   8,   7,  7,  7,  7,  7,  6,  6,  6,  6,  6,  6,  6,  5,  5};

// Set the shuffle control mask of indices out of range [0, 15] to (1xxxxxxx)b
// to get value 0 as the shuffle result. The most significiant bit 1 comes
// either from the comparison instruction, or from the sign bit of the index.
inline __m128i ShuffleIndex(const __m128i table, const __m128i index) {
  __m128i mask;
  mask = _mm_cmpgt_epi8(index, _mm_set1_epi8(15));
  mask = _mm_or_si128(mask, index);
  return _mm_shuffle_epi8(table, mask);
}

inline __m256i ShuffleIndex(const __m256i table, const __m256i index) {
  __m256i mask;
  mask = _mm256_cmpgt_epi8(index, _mm256_set1_epi8(15));
  mask = _mm256_or_si256(mask, index);
  return _mm256_shuffle_epi8(table, mask);
}

inline __m128i AdjustValue(const __m128i value, const __m128i index,
                           const int threshold) {
  const __m128i thresholds = _mm_set1_epi8(threshold - 128);
  const __m128i offset = _mm_cmpgt_epi8(index, thresholds);
  return _mm_add_epi8(value, offset);
}

inline __m256i AdjustValue(const __m256i value, const __m256i index,
                           const int threshold) {
  const __m256i thresholds = _mm256_set1_epi8(threshold - 128);
  const __m256i offset = _mm256_cmpgt_epi8(index, thresholds);
  return _mm256_add_epi8(value, offset);
}

inline void CalculateIntermediate(const __m128i sum[2], const __m128i index[2],
                                  __m128i* const ma, __m128i b0[2],
                                  __m128i b1[2]) {
  // Use table lookup to read elements whose indices are less than 48.
  const __m128i c0 = LoadAligned16(kSgrMaLookup + 0 * 16);
  const __m128i c1 = LoadAligned16(kSgrMaLookup + 1 * 16);
  const __m128i c2 = LoadAligned16(kSgrMaLookup + 2 * 16);
  const __m128i indices = _mm_packus_epi16(index[0], index[1]);
  __m128i idx;
  // Clip idx to 127 to apply signed comparison instructions.
  idx = _mm_min_epu8(indices, _mm_set1_epi8(127));
  // All elements whose indices are less than 48 are set to 0.
  // Get shuffle results for indices in range [0, 15].
  *ma = ShuffleIndex(c0, idx);
  // Get shuffle results for indices in range [16, 31].
  // Subtract 16 to utilize the sign bit of the index.
  idx = _mm_sub_epi8(idx, _mm_set1_epi8(16));
  const __m128i res1 = ShuffleIndex(c1, idx);
  // Use OR instruction to combine shuffle results together.
  *ma = _mm_or_si128(*ma, res1);
  // Get shuffle results for indices in range [32, 47].
  // Subtract 16 to utilize the sign bit of the index.
  idx = _mm_sub_epi8(idx, _mm_set1_epi8(16));
  const __m128i res2 = ShuffleIndex(c2, idx);
  *ma = _mm_or_si128(*ma, res2);

  // For elements whose indices are larger than 47, since they seldom change
  // values with the increase of the index, we use comparison and arithmetic
  // operations to calculate their values.
  // Add -128 to apply signed comparison instructions.
  idx = _mm_add_epi8(indices, _mm_set1_epi8(-128));
  // Elements whose indices are larger than 47 (with value 0) are set to 5.
  *ma = _mm_max_epu8(*ma, _mm_set1_epi8(5));
  *ma = AdjustValue(*ma, idx, 55);   // 55 is the last index which value is 5.
  *ma = AdjustValue(*ma, idx, 72);   // 72 is the last index which value is 4.
  *ma = AdjustValue(*ma, idx, 101);  // 101 is the last index which value is 3.
  *ma = AdjustValue(*ma, idx, 169);  // 169 is the last index which value is 2.
  *ma = AdjustValue(*ma, idx, 254);  // 254 is the last index which value is 1.

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
  const __m128i maq0 = _mm_unpacklo_epi8(*ma, _mm_setzero_si128());
  CalculateB3(sum[0], maq0, b0);
  const __m128i maq1 = _mm_unpackhi_epi8(*ma, _mm_setzero_si128());
  CalculateB3(sum[1], maq1, b1);
}

template <int n>
inline void CalculateIntermediate(const __m256i sum[2], const __m256i index[2],
                                  __m256i ma[3], __m256i b0[2], __m256i b1[2]) {
  static_assert(n == 9 || n == 25, "");
  // Use table lookup to read elements whose indices are less than 48.
  const __m256i c0 = LoadAligned32(kSgrMaLookupAvx2 + 0 * 32);
  const __m256i c1 = LoadAligned32(kSgrMaLookupAvx2 + 1 * 32);
  const __m256i c2 = LoadAligned32(kSgrMaLookupAvx2 + 2 * 32);
  const __m256i indices = _mm256_packus_epi16(index[0], index[1]);  // 0 2 1 3
  __m256i idx, mas;
  // Clip idx to 127 to apply signed comparison instructions.
  idx = _mm256_min_epu8(indices, _mm256_set1_epi8(127));
  // All elements whose indices are less than 48 are set to 0.
  // Get shuffle results for indices in range [0, 15].
  mas = ShuffleIndex(c0, idx);
  // Get shuffle results for indices in range [16, 31].
  // Subtract 16 to utilize the sign bit of the index.
  idx = _mm256_sub_epi8(idx, _mm256_set1_epi8(16));
  const __m256i res1 = ShuffleIndex(c1, idx);
  // Use OR instruction to combine shuffle results together.
  mas = _mm256_or_si256(mas, res1);
  // Get shuffle results for indices in range [32, 47].
  // Subtract 16 to utilize the sign bit of the index.
  idx = _mm256_sub_epi8(idx, _mm256_set1_epi8(16));
  const __m256i res2 = ShuffleIndex(c2, idx);
  mas = _mm256_or_si256(mas, res2);

  // For elements whose indices are larger than 47, since they seldom change
  // values with the increase of the index, we use comparison and arithmetic
  // operations to calculate their values.
  // Add -128 to apply signed comparison instructions.
  idx = _mm256_add_epi8(indices, _mm256_set1_epi8(-128));
  // Elements whose indices are larger than 47 (with value 0) are set to 5.
  mas = _mm256_max_epu8(mas, _mm256_set1_epi8(5));
  mas = AdjustValue(mas, idx, 55);   // 55 is the last index which value is 5.
  mas = AdjustValue(mas, idx, 72);   // 72 is the last index which value is 4.
  mas = AdjustValue(mas, idx, 101);  // 101 is the last index which value is 3.
  mas = AdjustValue(mas, idx, 169);  // 169 is the last index which value is 2.
  mas = AdjustValue(mas, idx, 254);  // 254 is the last index which value is 1.

  ma[2] = _mm256_permute4x64_epi64(mas, 0x63);     // 32-39 8-15 16-23 24-31
  ma[0] = _mm256_blend_epi32(ma[0], ma[2], 0xfc);  //  0-7  8-15 16-23 24-31
  ma[1] = _mm256_permute2x128_si256(ma[0], ma[2], 0x21);

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
  const __m256i maq0 = _mm256_unpackhi_epi8(ma[0], _mm256_setzero_si256());
  const __m256i maq1 = _mm256_unpacklo_epi8(ma[1], _mm256_setzero_si256());
  __m256i sums[2];
  sums[0] = _mm256_permute2x128_si256(sum[0], sum[1], 0x20);
  sums[1] = _mm256_permute2x128_si256(sum[0], sum[1], 0x31);
  if (n == 9) {
    CalculateB3(sums[0], maq0, b0);
    CalculateB3(sums[1], maq1, b1);
  } else {
    CalculateB5(sums[0], maq0, b0);
    CalculateB5(sums[1], maq1, b1);
  }
}

inline void CalculateIntermediate5(const __m128i s5[5], const __m128i sq5[5][2],
                                   const uint32_t scale, __m128i* const ma,
                                   __m128i b[2]) {
  __m128i sum, index;
  CalculateSumAndIndex5(s5, sq5, scale, &sum, &index);
  LookupIntermediate<25>(sum, index, ma, b);
}

inline void CalculateIntermediate3(const __m128i s3[3], const __m128i sq3[3][2],
                                   const uint32_t scale, __m128i* const ma,
                                   __m128i b[2]) {
  __m128i sum, index;
  CalculateSumAndIndex3(s3, sq3, scale, &sum, &index);
  LookupIntermediate<9>(sum, index, ma, b);
}

inline void Store343_444(const __m256i b3[3], const ptrdiff_t x,
                         __m256i sum_b343[2], __m256i sum_b444[2],
                         uint32_t* const b343, uint32_t* const b444) {
  __m256i b[3], sum_b111[2];
  Prepare3_32(b3 + 0, b);
  sum_b111[0] = Sum3_32(b);
  sum_b444[0] = _mm256_slli_epi32(sum_b111[0], 2);
  sum_b343[0] = _mm256_sub_epi32(sum_b444[0], sum_b111[0]);
  sum_b343[0] = _mm256_add_epi32(sum_b343[0], b[1]);
  Prepare3_32(b3 + 1, b);
  sum_b111[1] = Sum3_32(b);
  sum_b444[1] = _mm256_slli_epi32(sum_b111[1], 2);
  sum_b343[1] = _mm256_sub_epi32(sum_b444[1], sum_b111[1]);
  sum_b343[1] = _mm256_add_epi32(sum_b343[1], b[1]);
  StoreAligned64(b444 + x, sum_b444);
  StoreAligned64(b343 + x, sum_b343);
}

inline void Store343_444Lo(const __m256i ma3[3], const __m256i b3[2],
                           const ptrdiff_t x, __m256i* const sum_ma343,
                           __m256i* const sum_ma444, __m256i sum_b343[2],
                           __m256i sum_b444[2], uint16_t* const ma343,
                           uint16_t* const ma444, uint32_t* const b343,
                           uint32_t* const b444) {
  const __m256i sum_ma111 = Sum3WLo16(ma3);
  *sum_ma444 = _mm256_slli_epi16(sum_ma111, 2);
  StoreAligned32_ma(ma444 + x, *sum_ma444);
  const __m256i sum333 = _mm256_sub_epi16(*sum_ma444, sum_ma111);
  *sum_ma343 = VaddwLo8(sum333, ma3[1]);
  StoreAligned32_ma(ma343 + x, *sum_ma343);
  Store343_444(b3, x, sum_b343, sum_b444, b343, b444);
}

inline void Store343_444Hi(const __m256i ma3[3], const __m256i b3[2],
                           const ptrdiff_t x, __m256i* const sum_ma343,
                           __m256i* const sum_ma444, __m256i sum_b343[2],
                           __m256i sum_b444[2], uint16_t* const ma343,
                           uint16_t* const ma444, uint32_t* const b343,
                           uint32_t* const b444) {
  const __m256i sum_ma111 = Sum3WHi16(ma3);
  *sum_ma444 = _mm256_slli_epi16(sum_ma111, 2);
  StoreAligned32_ma(ma444 + x, *sum_ma444);
  const __m256i sum333 = _mm256_sub_epi16(*sum_ma444, sum_ma111);
  *sum_ma343 = VaddwHi8(sum333, ma3[1]);
  StoreAligned32_ma(ma343 + x, *sum_ma343);
  Store343_444(b3, x + kMaStoreOffset, sum_b343, sum_b444, b343, b444);
}

inline void Store343_444Lo(const __m256i ma3[3], const __m256i b3[2],
                           const ptrdiff_t x, __m256i* const sum_ma343,
                           __m256i sum_b343[2], uint16_t* const ma343,
                           uint16_t* const ma444, uint32_t* const b343,
                           uint32_t* const b444) {
  __m256i sum_ma444, sum_b444[2];
  Store343_444Lo(ma3, b3, x, sum_ma343, &sum_ma444, sum_b343, sum_b444, ma343,
                 ma444, b343, b444);
}

inline void Store343_444Hi(const __m256i ma3[3], const __m256i b3[2],
                           const ptrdiff_t x, __m256i* const sum_ma343,
                           __m256i sum_b343[2], uint16_t* const ma343,
                           uint16_t* const ma444, uint32_t* const b343,
                           uint32_t* const b444) {
  __m256i sum_ma444, sum_b444[2];
  Store343_444Hi(ma3, b3, x, sum_ma343, &sum_ma444, sum_b343, sum_b444, ma343,
                 ma444, b343, b444);
}

inline void Store343_444Lo(const __m256i ma3[3], const __m256i b3[2],
                           const ptrdiff_t x, uint16_t* const ma343,
                           uint16_t* const ma444, uint32_t* const b343,
                           uint32_t* const b444) {
  __m256i sum_ma343, sum_b343[2];
  Store343_444Lo(ma3, b3, x, &sum_ma343, sum_b343, ma343, ma444, b343, b444);
}

inline void Store343_444Hi(const __m256i ma3[3], const __m256i b3[2],
                           const ptrdiff_t x, uint16_t* const ma343,
                           uint16_t* const ma444, uint32_t* const b343,
                           uint32_t* const b444) {
  __m256i sum_ma343, sum_b343[2];
  Store343_444Hi(ma3, b3, x, &sum_ma343, sum_b343, ma343, ma444, b343, b444);
}

// Don't combine the following 2 functions, which would be slower.
inline void Store343_444(const __m256i ma3[3], const __m256i b3[6],
                         const ptrdiff_t x, __m256i* const sum_ma343_lo,
                         __m256i* const sum_ma343_hi,
                         __m256i* const sum_ma444_lo,
                         __m256i* const sum_ma444_hi, __m256i sum_b343_lo[2],
                         __m256i sum_b343_hi[2], __m256i sum_b444_lo[2],
                         __m256i sum_b444_hi[2], uint16_t* const ma343,
                         uint16_t* const ma444, uint32_t* const b343,
                         uint32_t* const b444) {
  __m256i sum_mat343[2], sum_mat444[2];
  const __m256i sum_ma111_lo = Sum3WLo16(ma3);
  sum_mat444[0] = _mm256_slli_epi16(sum_ma111_lo, 2);
  const __m256i sum333_lo = _mm256_sub_epi16(sum_mat444[0], sum_ma111_lo);
  sum_mat343[0] = VaddwLo8(sum333_lo, ma3[1]);
  Store343_444(b3, x, sum_b343_lo, sum_b444_lo, b343, b444);
  const __m256i sum_ma111_hi = Sum3WHi16(ma3);
  sum_mat444[1] = _mm256_slli_epi16(sum_ma111_hi, 2);
  *sum_ma444_lo = _mm256_permute2x128_si256(sum_mat444[0], sum_mat444[1], 0x20);
  *sum_ma444_hi = _mm256_permute2x128_si256(sum_mat444[0], sum_mat444[1], 0x31);
  StoreAligned32(ma444 + x + 0, *sum_ma444_lo);
  StoreAligned32(ma444 + x + 16, *sum_ma444_hi);
  const __m256i sum333_hi = _mm256_sub_epi16(sum_mat444[1], sum_ma111_hi);
  sum_mat343[1] = VaddwHi8(sum333_hi, ma3[1]);
  *sum_ma343_lo = _mm256_permute2x128_si256(sum_mat343[0], sum_mat343[1], 0x20);
  *sum_ma343_hi = _mm256_permute2x128_si256(sum_mat343[0], sum_mat343[1], 0x31);
  StoreAligned32(ma343 + x + 0, *sum_ma343_lo);
  StoreAligned32(ma343 + x + 16, *sum_ma343_hi);
  Store343_444(b3 + 3, x + 16, sum_b343_hi, sum_b444_hi, b343, b444);
}

inline void Store343_444(const __m256i ma3[3], const __m256i b3[6],
                         const ptrdiff_t x, __m256i* const sum_ma343_lo,
                         __m256i* const sum_ma343_hi, __m256i sum_b343_lo[2],
                         __m256i sum_b343_hi[2], uint16_t* const ma343,
                         uint16_t* const ma444, uint32_t* const b343,
                         uint32_t* const b444) {
  __m256i sum_ma444[2], sum_b444[2], sum_mat343[2];
  const __m256i sum_ma111_lo = Sum3WLo16(ma3);
  sum_ma444[0] = _mm256_slli_epi16(sum_ma111_lo, 2);
  const __m256i sum333_lo = _mm256_sub_epi16(sum_ma444[0], sum_ma111_lo);
  sum_mat343[0] = VaddwLo8(sum333_lo, ma3[1]);
  Store343_444(b3, x, sum_b343_lo, sum_b444, b343, b444);
  const __m256i sum_ma111_hi = Sum3WHi16(ma3);
  sum_ma444[1] = _mm256_slli_epi16(sum_ma111_hi, 2);
  StoreAligned64_ma(ma444 + x, sum_ma444);
  const __m256i sum333_hi = _mm256_sub_epi16(sum_ma444[1], sum_ma111_hi);
  sum_mat343[1] = VaddwHi8(sum333_hi, ma3[1]);
  *sum_ma343_lo = _mm256_permute2x128_si256(sum_mat343[0], sum_mat343[1], 0x20);
  *sum_ma343_hi = _mm256_permute2x128_si256(sum_mat343[0], sum_mat343[1], 0x31);
  StoreAligned32(ma343 + x + 0, *sum_ma343_lo);
  StoreAligned32(ma343 + x + 16, *sum_ma343_hi);
  Store343_444(b3 + 3, x + 16, sum_b343_hi, sum_b444, b343, b444);
}

inline void PermuteB(const __m256i t[4], __m256i b[7]) {
  // Input:
  //                             0     1      2     3  // b[0]
  //                             4     5      6     7  // b[1]
  //  8     9     10    11      24    25     26    27  // t[0]
  // 12    13     14    15      28    29     30    31  // t[1]
  // 16    17     18    19      32    33     34    35  // t[2]
  // 20    21     22    23      36    37     38    39  // t[3]

  // Output:
  //  0     1      2     3       8     9     10    11  // b[0]
  //  4     5      6     7      12    13     14    15  // b[1]
  //  8     9     10    11      16    17     18    19  // b[2]
  // 16    17     18    19      24    25     26    27  // b[3]
  // 20    21     22    23      28    29     30    31  // b[4]
  // 24    25     26    27      32    33     34    35  // b[5]
  // 20    21     22    23      36    37     38    39  // b[6]
  b[0] = _mm256_permute2x128_si256(b[0], t[0], 0x21);
  b[1] = _mm256_permute2x128_si256(b[1], t[1], 0x21);
  b[2] = _mm256_permute2x128_si256(t[0], t[2], 0x20);
  b[3] = _mm256_permute2x128_si256(t[2], t[0], 0x30);
  b[4] = _mm256_permute2x128_si256(t[3], t[1], 0x30);
  b[5] = _mm256_permute2x128_si256(t[0], t[2], 0x31);
  b[6] = t[3];
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPreProcess5Lo(
    const __m128i s[2][2], const uint32_t scale, uint16_t* const sum5[5],
    uint32_t* const square_sum5[5], __m128i sq[2][4], __m128i* const ma,
    __m128i b[2]) {
  __m128i s5[2][5], sq5[5][2];
  Square(s[0][1], sq[0] + 2);
  Square(s[1][1], sq[1] + 2);
  s5[0][3] = Sum5Horizontal16(s[0]);
  StoreAligned16(sum5[3], s5[0][3]);
  s5[0][4] = Sum5Horizontal16(s[1]);
  StoreAligned16(sum5[4], s5[0][4]);
  Sum5Horizontal32(sq[0], sq5[3]);
  StoreAligned32U32(square_sum5[3], sq5[3]);
  Sum5Horizontal32(sq[1], sq5[4]);
  StoreAligned32U32(square_sum5[4], sq5[4]);
  LoadAligned16x3U16(sum5, 0, s5[0]);
  LoadAligned32x3U32(square_sum5, 0, sq5);
  CalculateIntermediate5(s5[0], sq5, scale, ma, b);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPreProcess5(
    const uint16_t* const src0, const uint16_t* const src1,
    const ptrdiff_t over_read_in_bytes, const ptrdiff_t sum_width,
    const ptrdiff_t x, const uint32_t scale, uint16_t* const sum5[5],
    uint32_t* const square_sum5[5], __m256i sq[2][8], __m256i ma[3],
    __m256i b[3]) {
  __m256i s[2], s5[2][5], sq5[5][2], sum[2], index[2], t[4];
  s[0] = LoadUnaligned32Msan(src0 + 8, over_read_in_bytes + 16);
  s[1] = LoadUnaligned32Msan(src1 + 8, over_read_in_bytes + 16);
  Square(s[0], sq[0] + 2);
  Square(s[1], sq[1] + 2);
  sq[0][0] = _mm256_permute2x128_si256(sq[0][0], sq[0][2], 0x21);
  sq[0][1] = _mm256_permute2x128_si256(sq[0][1], sq[0][3], 0x21);
  sq[1][0] = _mm256_permute2x128_si256(sq[1][0], sq[1][2], 0x21);
  sq[1][1] = _mm256_permute2x128_si256(sq[1][1], sq[1][3], 0x21);
  s5[0][3] = Sum5Horizontal16(src0 + 0, over_read_in_bytes + 0);
  s5[1][3] = Sum5Horizontal16(src0 + 16, over_read_in_bytes + 32);
  s5[0][4] = Sum5Horizontal16(src1 + 0, over_read_in_bytes + 0);
  s5[1][4] = Sum5Horizontal16(src1 + 16, over_read_in_bytes + 32);
  StoreAligned32(sum5[3] + x + 0, s5[0][3]);
  StoreAligned32(sum5[3] + x + 16, s5[1][3]);
  StoreAligned32(sum5[4] + x + 0, s5[0][4]);
  StoreAligned32(sum5[4] + x + 16, s5[1][4]);
  Sum5Horizontal32(sq[0], sq5[3]);
  StoreAligned64(square_sum5[3] + x, sq5[3]);
  Sum5Horizontal32(sq[1], sq5[4]);
  StoreAligned64(square_sum5[4] + x, sq5[4]);
  LoadAligned32x3U16(sum5, x, s5[0]);
  LoadAligned64x3U32(square_sum5, x, sq5);
  CalculateSumAndIndex5(s5[0], sq5, scale, &sum[0], &index[0]);

  s[0] = LoadUnaligned32Msan(src0 + 24, over_read_in_bytes + 48);
  s[1] = LoadUnaligned32Msan(src1 + 24, over_read_in_bytes + 48);
  Square(s[0], sq[0] + 6);
  Square(s[1], sq[1] + 6);
  sq[0][4] = _mm256_permute2x128_si256(sq[0][2], sq[0][6], 0x21);
  sq[0][5] = _mm256_permute2x128_si256(sq[0][3], sq[0][7], 0x21);
  sq[1][4] = _mm256_permute2x128_si256(sq[1][2], sq[1][6], 0x21);
  sq[1][5] = _mm256_permute2x128_si256(sq[1][3], sq[1][7], 0x21);
  Sum5Horizontal32(sq[0] + 4, sq5[3]);
  StoreAligned64(square_sum5[3] + x + 16, sq5[3]);
  Sum5Horizontal32(sq[1] + 4, sq5[4]);
  StoreAligned64(square_sum5[4] + x + 16, sq5[4]);
  LoadAligned32x3U16Msan(sum5, x + 16, sum_width, s5[1]);
  LoadAligned64x3U32Msan(square_sum5, x + 16, sum_width, sq5);
  CalculateSumAndIndex5(s5[1], sq5, scale, &sum[1], &index[1]);
  CalculateIntermediate<25>(sum, index, ma, t, t + 2);
  PermuteB(t, b);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPreProcess5LastRowLo(
    const __m128i s[2], const uint32_t scale, const uint16_t* const sum5[5],
    const uint32_t* const square_sum5[5], __m128i sq[4], __m128i* const ma,
    __m128i b[2]) {
  __m128i s5[5], sq5[5][2];
  Square(s[1], sq + 2);
  s5[3] = s5[4] = Sum5Horizontal16(s);
  Sum5Horizontal32(sq, sq5[3]);
  sq5[4][0] = sq5[3][0];
  sq5[4][1] = sq5[3][1];
  LoadAligned16x3U16(sum5, 0, s5);
  LoadAligned32x3U32(square_sum5, 0, sq5);
  CalculateIntermediate5(s5, sq5, scale, ma, b);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPreProcess5LastRow(
    const uint16_t* const src, const ptrdiff_t over_read_in_bytes,
    const ptrdiff_t sum_width, const ptrdiff_t x, const uint32_t scale,
    const uint16_t* const sum5[5], const uint32_t* const square_sum5[5],
    __m256i sq[3], __m256i ma[3], __m256i b[3]) {
  const __m256i s0 = LoadUnaligned32Msan(src + 8, over_read_in_bytes + 16);
  __m256i s5[2][5], sq5[5][2], sum[2], index[2], t[4];
  Square(s0, sq + 2);
  sq[0] = _mm256_permute2x128_si256(sq[0], sq[2], 0x21);
  sq[1] = _mm256_permute2x128_si256(sq[1], sq[3], 0x21);
  s5[0][3] = Sum5Horizontal16(src + 0, over_read_in_bytes + 0);
  s5[1][3] = Sum5Horizontal16(src + 16, over_read_in_bytes + 32);
  s5[0][4] = s5[0][3];
  s5[1][4] = s5[1][3];
  Sum5Horizontal32(sq, sq5[3]);
  sq5[4][0] = sq5[3][0];
  sq5[4][1] = sq5[3][1];
  LoadAligned32x3U16(sum5, x, s5[0]);
  LoadAligned64x3U32(square_sum5, x, sq5);
  CalculateSumAndIndex5(s5[0], sq5, scale, &sum[0], &index[0]);

  const __m256i s1 = LoadUnaligned32Msan(src + 24, over_read_in_bytes + 48);
  Square(s1, sq + 6);
  sq[4] = _mm256_permute2x128_si256(sq[2], sq[6], 0x21);
  sq[5] = _mm256_permute2x128_si256(sq[3], sq[7], 0x21);
  Sum5Horizontal32(sq + 4, sq5[3]);
  sq5[4][0] = sq5[3][0];
  sq5[4][1] = sq5[3][1];
  LoadAligned32x3U16Msan(sum5, x + 16, sum_width, s5[1]);
  LoadAligned64x3U32Msan(square_sum5, x + 16, sum_width, sq5);
  CalculateSumAndIndex5(s5[1], sq5, scale, &sum[1], &index[1]);
  CalculateIntermediate<25>(sum, index, ma, t, t + 2);
  PermuteB(t, b);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPreProcess3Lo(
    const __m128i s[2], const uint32_t scale, uint16_t* const sum3[3],
    uint32_t* const square_sum3[3], __m128i sq[4], __m128i* const ma,
    __m128i b[2]) {
  __m128i s3[3], sq3[3][2];
  Square(s[1], sq + 2);
  s3[2] = Sum3Horizontal16(s);
  StoreAligned16(sum3[2], s3[2]);
  Sum3Horizontal32(sq, sq3[2]);
  StoreAligned32U32(square_sum3[2], sq3[2]);
  LoadAligned16x2U16(sum3, 0, s3);
  LoadAligned32x2U32(square_sum3, 0, sq3);
  CalculateIntermediate3(s3, sq3, scale, ma, b);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPreProcess3(
    const uint16_t* const src, const ptrdiff_t over_read_in_bytes,
    const ptrdiff_t x, const ptrdiff_t sum_width, const uint32_t scale,
    uint16_t* const sum3[3], uint32_t* const square_sum3[3], __m256i sq[8],
    __m256i ma[3], __m256i b[7]) {
  __m256i s[2], s3[4], sq3[3][2], sum[2], index[2], t[4];
  s[0] = LoadUnaligned32Msan(src + 8, over_read_in_bytes + 16);
  s[1] = LoadUnaligned32Msan(src + 24, over_read_in_bytes + 48);
  Square(s[0], sq + 2);
  sq[0] = _mm256_permute2x128_si256(sq[0], sq[2], 0x21);
  sq[1] = _mm256_permute2x128_si256(sq[1], sq[3], 0x21);
  s3[2] = Sum3Horizontal16(src, over_read_in_bytes);
  s3[3] = Sum3Horizontal16(src + 16, over_read_in_bytes + 32);
  StoreAligned64(sum3[2] + x, s3 + 2);
  Sum3Horizontal32(sq + 0, sq3[2]);
  StoreAligned64(square_sum3[2] + x, sq3[2]);
  LoadAligned32x2U16(sum3, x, s3);
  LoadAligned64x2U32(square_sum3, x, sq3);
  CalculateSumAndIndex3(s3, sq3, scale, &sum[0], &index[0]);

  Square(s[1], sq + 6);
  sq[4] = _mm256_permute2x128_si256(sq[2], sq[6], 0x21);
  sq[5] = _mm256_permute2x128_si256(sq[3], sq[7], 0x21);
  Sum3Horizontal32(sq + 4, sq3[2]);
  StoreAligned64(square_sum3[2] + x + 16, sq3[2]);
  LoadAligned32x2U16Msan(sum3, x + 16, sum_width, s3 + 1);
  LoadAligned64x2U32Msan(square_sum3, x + 16, sum_width, sq3);
  CalculateSumAndIndex3(s3 + 1, sq3, scale, &sum[1], &index[1]);
  CalculateIntermediate<9>(sum, index, ma, t, t + 2);
  PermuteB(t, b);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPreProcessLo(
    const __m128i s[2][4], const uint16_t scales[2], uint16_t* const sum3[4],
    uint16_t* const sum5[5], uint32_t* const square_sum3[4],
    uint32_t* const square_sum5[5], __m128i sq[2][8], __m128i ma3[2][3],
    __m128i b3[2][10], __m128i* const ma5, __m128i b5[2]) {
  __m128i s3[4], s5[5], sq3[4][2], sq5[5][2], sum[2], index[2];
  Square(s[0][1], sq[0] + 2);
  Square(s[1][1], sq[1] + 2);
  SumHorizontal16(s[0], &s3[2], &s5[3]);
  SumHorizontal16(s[1], &s3[3], &s5[4]);
  StoreAligned16(sum3[2], s3[2]);
  StoreAligned16(sum3[3], s3[3]);
  StoreAligned16(sum5[3], s5[3]);
  StoreAligned16(sum5[4], s5[4]);
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
  ma3[1][0] = _mm_srli_si128(ma3[0][0], 8);
  CalculateIntermediate5(s5, sq5, scales[0], ma5, b5);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPreProcess(
    const uint16_t* const src0, const uint16_t* const src1,
    const ptrdiff_t over_read_in_bytes, const ptrdiff_t x,
    const uint16_t scales[2], uint16_t* const sum3[4], uint16_t* const sum5[5],
    uint32_t* const square_sum3[4], uint32_t* const square_sum5[5],
    const ptrdiff_t sum_width, __m256i sq[2][8], __m256i ma3[2][3],
    __m256i b3[2][7], __m256i ma5[3], __m256i b5[5]) {
  __m256i s[2], s3[2][4], s5[2][5], sq3[4][2], sq5[5][2], sum_3[2][2],
      index_3[2][2], sum_5[2], index_5[2], t[4];
  s[0] = LoadUnaligned32Msan(src0 + 8, over_read_in_bytes + 16);
  s[1] = LoadUnaligned32Msan(src1 + 8, over_read_in_bytes + 16);
  Square(s[0], sq[0] + 2);
  Square(s[1], sq[1] + 2);
  sq[0][0] = _mm256_permute2x128_si256(sq[0][0], sq[0][2], 0x21);
  sq[0][1] = _mm256_permute2x128_si256(sq[0][1], sq[0][3], 0x21);
  sq[1][0] = _mm256_permute2x128_si256(sq[1][0], sq[1][2], 0x21);
  sq[1][1] = _mm256_permute2x128_si256(sq[1][1], sq[1][3], 0x21);
  SumHorizontal16(src0, over_read_in_bytes, &s3[0][2], &s3[1][2], &s5[0][3],
                  &s5[1][3]);
  SumHorizontal16(src1, over_read_in_bytes, &s3[0][3], &s3[1][3], &s5[0][4],
                  &s5[1][4]);
  StoreAligned32(sum3[2] + x + 0, s3[0][2]);
  StoreAligned32(sum3[2] + x + 16, s3[1][2]);
  StoreAligned32(sum3[3] + x + 0, s3[0][3]);
  StoreAligned32(sum3[3] + x + 16, s3[1][3]);
  StoreAligned32(sum5[3] + x + 0, s5[0][3]);
  StoreAligned32(sum5[3] + x + 16, s5[1][3]);
  StoreAligned32(sum5[4] + x + 0, s5[0][4]);
  StoreAligned32(sum5[4] + x + 16, s5[1][4]);
  SumHorizontal32(sq[0], &sq3[2][0], &sq3[2][1], &sq5[3][0], &sq5[3][1]);
  SumHorizontal32(sq[1], &sq3[3][0], &sq3[3][1], &sq5[4][0], &sq5[4][1]);
  StoreAligned64(square_sum3[2] + x, sq3[2]);
  StoreAligned64(square_sum5[3] + x, sq5[3]);
  StoreAligned64(square_sum3[3] + x, sq3[3]);
  StoreAligned64(square_sum5[4] + x, sq5[4]);
  LoadAligned32x2U16(sum3, x, s3[0]);
  LoadAligned64x2U32(square_sum3, x, sq3);
  CalculateSumAndIndex3(s3[0], sq3, scales[1], &sum_3[0][0], &index_3[0][0]);
  CalculateSumAndIndex3(s3[0] + 1, sq3 + 1, scales[1], &sum_3[1][0],
                        &index_3[1][0]);
  LoadAligned32x3U16(sum5, x, s5[0]);
  LoadAligned64x3U32(square_sum5, x, sq5);
  CalculateSumAndIndex5(s5[0], sq5, scales[0], &sum_5[0], &index_5[0]);

  s[0] = LoadUnaligned32Msan(src0 + 24, over_read_in_bytes + 48);
  s[1] = LoadUnaligned32Msan(src1 + 24, over_read_in_bytes + 48);
  Square(s[0], sq[0] + 6);
  Square(s[1], sq[1] + 6);
  sq[0][4] = _mm256_permute2x128_si256(sq[0][2], sq[0][6], 0x21);
  sq[0][5] = _mm256_permute2x128_si256(sq[0][3], sq[0][7], 0x21);
  sq[1][4] = _mm256_permute2x128_si256(sq[1][2], sq[1][6], 0x21);
  sq[1][5] = _mm256_permute2x128_si256(sq[1][3], sq[1][7], 0x21);
  SumHorizontal32(sq[0] + 4, &sq3[2][0], &sq3[2][1], &sq5[3][0], &sq5[3][1]);
  SumHorizontal32(sq[1] + 4, &sq3[3][0], &sq3[3][1], &sq5[4][0], &sq5[4][1]);
  StoreAligned64(square_sum3[2] + x + 16, sq3[2]);
  StoreAligned64(square_sum5[3] + x + 16, sq5[3]);
  StoreAligned64(square_sum3[3] + x + 16, sq3[3]);
  StoreAligned64(square_sum5[4] + x + 16, sq5[4]);
  LoadAligned32x2U16Msan(sum3, x + 16, sum_width, s3[1]);
  LoadAligned64x2U32Msan(square_sum3, x + 16, sum_width, sq3);
  CalculateSumAndIndex3(s3[1], sq3, scales[1], &sum_3[0][1], &index_3[0][1]);
  CalculateSumAndIndex3(s3[1] + 1, sq3 + 1, scales[1], &sum_3[1][1],
                        &index_3[1][1]);
  CalculateIntermediate<9>(sum_3[0], index_3[0], ma3[0], t, t + 2);
  PermuteB(t, b3[0]);
  CalculateIntermediate<9>(sum_3[1], index_3[1], ma3[1], t, t + 2);
  PermuteB(t, b3[1]);
  LoadAligned32x3U16Msan(sum5, x + 16, sum_width, s5[1]);
  LoadAligned64x3U32Msan(square_sum5, x + 16, sum_width, sq5);
  CalculateSumAndIndex5(s5[1], sq5, scales[0], &sum_5[1], &index_5[1]);
  CalculateIntermediate<25>(sum_5, index_5, ma5, t, t + 2);
  PermuteB(t, b5);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPreProcessLastRowLo(
    const __m128i s[2], const uint16_t scales[2], const uint16_t* const sum3[4],
    const uint16_t* const sum5[5], const uint32_t* const square_sum3[4],
    const uint32_t* const square_sum5[5], __m128i sq[4], __m128i* const ma3,
    __m128i* const ma5, __m128i b3[2], __m128i b5[2]) {
  __m128i s3[3], s5[5], sq3[3][2], sq5[5][2];
  Square(s[1], sq + 2);
  SumHorizontal16(s, &s3[2], &s5[3]);
  SumHorizontal32(sq, &sq3[2][0], &sq3[2][1], &sq5[3][0], &sq5[3][1]);
  LoadAligned16x3U16(sum5, 0, s5);
  s5[4] = s5[3];
  LoadAligned32x3U32(square_sum5, 0, sq5);
  sq5[4][0] = sq5[3][0];
  sq5[4][1] = sq5[3][1];
  CalculateIntermediate5(s5, sq5, scales[0], ma5, b5);
  LoadAligned16x2U16(sum3, 0, s3);
  LoadAligned32x2U32(square_sum3, 0, sq3);
  CalculateIntermediate3(s3, sq3, scales[1], ma3, b3);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPreProcessLastRow(
    const uint16_t* const src, const ptrdiff_t over_read_in_bytes,
    const ptrdiff_t sum_width, const ptrdiff_t x, const uint16_t scales[2],
    const uint16_t* const sum3[4], const uint16_t* const sum5[5],
    const uint32_t* const square_sum3[4], const uint32_t* const square_sum5[5],
    __m256i sq[6], __m256i ma3[2], __m256i ma5[2], __m256i b3[5],
    __m256i b5[5]) {
  const __m256i s0 = LoadUnaligned32Msan(src + 8, over_read_in_bytes + 16);
  __m256i s3[2][3], s5[2][5], sq3[4][2], sq5[5][2], sum_3[2], index_3[2],
      sum_5[2], index_5[2], t[4];
  Square(s0, sq + 2);
  sq[0] = _mm256_permute2x128_si256(sq[0], sq[2], 0x21);
  sq[1] = _mm256_permute2x128_si256(sq[1], sq[3], 0x21);
  SumHorizontal16(src, over_read_in_bytes, &s3[0][2], &s3[1][2], &s5[0][3],
                  &s5[1][3]);
  SumHorizontal32(sq, &sq3[2][0], &sq3[2][1], &sq5[3][0], &sq5[3][1]);
  LoadAligned32x2U16(sum3, x, s3[0]);
  LoadAligned64x2U32(square_sum3, x, sq3);
  CalculateSumAndIndex3(s3[0], sq3, scales[1], &sum_3[0], &index_3[0]);
  LoadAligned32x3U16(sum5, x, s5[0]);
  s5[0][4] = s5[0][3];
  LoadAligned64x3U32(square_sum5, x, sq5);
  sq5[4][0] = sq5[3][0];
  sq5[4][1] = sq5[3][1];
  CalculateSumAndIndex5(s5[0], sq5, scales[0], &sum_5[0], &index_5[0]);

  const __m256i s1 = LoadUnaligned32Msan(src + 24, over_read_in_bytes + 48);
  Square(s1, sq + 6);
  sq[4] = _mm256_permute2x128_si256(sq[2], sq[6], 0x21);
  sq[5] = _mm256_permute2x128_si256(sq[3], sq[7], 0x21);
  SumHorizontal32(sq + 4, &sq3[2][0], &sq3[2][1], &sq5[3][0], &sq5[3][1]);
  LoadAligned32x2U16Msan(sum3, x + 16, sum_width, s3[1]);
  LoadAligned64x2U32Msan(square_sum3, x + 16, sum_width, sq3);
  CalculateSumAndIndex3(s3[1], sq3, scales[1], &sum_3[1], &index_3[1]);
  CalculateIntermediate<9>(sum_3, index_3, ma3, t, t + 2);
  PermuteB(t, b3);
  LoadAligned32x3U16Msan(sum5, x + 16, sum_width, s5[1]);
  s5[1][4] = s5[1][3];
  LoadAligned64x3U32Msan(square_sum5, x + 16, sum_width, sq5);
  sq5[4][0] = sq5[3][0];
  sq5[4][1] = sq5[3][1];
  CalculateSumAndIndex5(s5[1], sq5, scales[0], &sum_5[1], &index_5[1]);
  CalculateIntermediate<25>(sum_5, index_5, ma5, t, t + 2);
  PermuteB(t, b5);
}

inline void BoxSumFilterPreProcess5(const uint16_t* const src0,
                                    const uint16_t* const src1, const int width,
                                    const uint32_t scale,
                                    uint16_t* const sum5[5],
                                    uint32_t* const square_sum5[5],
                                    const ptrdiff_t sum_width, uint16_t* ma565,
                                    uint32_t* b565) {
  const ptrdiff_t overread_in_bytes =
      kOverreadInBytesPass1_128 - sizeof(*src0) * width;
  __m128i s[2][2], ma0, sq_128[2][4], b0[2];
  __m256i mas[3], sq[2][8], bs[10];
  s[0][0] = LoadUnaligned16Msan(src0 + 0, overread_in_bytes + 0);
  s[0][1] = LoadUnaligned16Msan(src0 + 8, overread_in_bytes + 16);
  s[1][0] = LoadUnaligned16Msan(src1 + 0, overread_in_bytes + 0);
  s[1][1] = LoadUnaligned16Msan(src1 + 8, overread_in_bytes + 16);
  Square(s[0][0], sq_128[0]);
  Square(s[1][0], sq_128[1]);
  BoxFilterPreProcess5Lo(s, scale, sum5, square_sum5, sq_128, &ma0, b0);
  sq[0][0] = SetrM128i(sq_128[0][2], sq_128[0][2]);
  sq[0][1] = SetrM128i(sq_128[0][3], sq_128[0][3]);
  sq[1][0] = SetrM128i(sq_128[1][2], sq_128[1][2]);
  sq[1][1] = SetrM128i(sq_128[1][3], sq_128[1][3]);
  mas[0] = SetrM128i(ma0, ma0);
  bs[0] = SetrM128i(b0[0], b0[0]);
  bs[1] = SetrM128i(b0[1], b0[1]);

  int x = 0;
  do {
    __m256i ma5[3], ma[2], b[4];
    BoxFilterPreProcess5(
        src0 + x + 8, src1 + x + 8,
        kOverreadInBytesPass1_256 + sizeof(*src0) * (x + 8 - width), sum_width,
        x + 8, scale, sum5, square_sum5, sq, mas, bs);
    Prepare3_8(mas, ma5);
    ma[0] = Sum565Lo(ma5);
    ma[1] = Sum565Hi(ma5);
    StoreAligned64_ma(ma565, ma);
    Sum565(bs + 0, b + 0);
    Sum565(bs + 3, b + 2);
    StoreAligned64(b565, b + 0);
    StoreAligned64(b565 + 16, b + 2);
    sq[0][0] = sq[0][6];
    sq[0][1] = sq[0][7];
    sq[1][0] = sq[1][6];
    sq[1][1] = sq[1][7];
    mas[0] = mas[2];
    bs[0] = bs[5];
    bs[1] = bs[6];
    ma565 += 32;
    b565 += 32;
    x += 32;
  } while (x < width);
}

template <bool calculate444>
LIBGAV1_ALWAYS_INLINE void BoxSumFilterPreProcess3(
    const uint16_t* const src, const int width, const uint32_t scale,
    uint16_t* const sum3[3], uint32_t* const square_sum3[3],
    const ptrdiff_t sum_width, uint16_t* ma343, uint16_t* ma444, uint32_t* b343,
    uint32_t* b444) {
  const ptrdiff_t overread_in_bytes_128 =
      kOverreadInBytesPass2_128 - sizeof(*src) * width;
  __m128i s[2], ma0, sq_128[4], b0[2];
  __m256i mas[3], sq[8], bs[7];
  s[0] = LoadUnaligned16Msan(src + 0, overread_in_bytes_128 + 0);
  s[1] = LoadUnaligned16Msan(src + 8, overread_in_bytes_128 + 16);
  Square(s[0], sq_128);
  BoxFilterPreProcess3Lo(s, scale, sum3, square_sum3, sq_128, &ma0, b0);
  sq[0] = SetrM128i(sq_128[2], sq_128[2]);
  sq[1] = SetrM128i(sq_128[3], sq_128[3]);
  mas[0] = SetrM128i(ma0, ma0);
  bs[0] = SetrM128i(b0[0], b0[0]);
  bs[1] = SetrM128i(b0[1], b0[1]);

  int x = 0;
  do {
    __m256i ma3[3];
    BoxFilterPreProcess3(
        src + x + 8, kOverreadInBytesPass2_256 + sizeof(*src) * (x + 8 - width),
        x + 8, sum_width, scale, sum3, square_sum3, sq, mas, bs);
    Prepare3_8(mas, ma3);
    if (calculate444) {  // NOLINT(readability-simplify-boolean-expr)
      Store343_444Lo(ma3, bs + 0, 0, ma343, ma444, b343, b444);
      Store343_444Hi(ma3, bs + 3, kMaStoreOffset, ma343, ma444, b343, b444);
      ma444 += 32;
      b444 += 32;
    } else {
      __m256i ma[2], b[4];
      ma[0] = Sum343Lo(ma3);
      ma[1] = Sum343Hi(ma3);
      StoreAligned64_ma(ma343, ma);
      Sum343(bs + 0, b + 0);
      Sum343(bs + 3, b + 2);
      StoreAligned64(b343 + 0, b + 0);
      StoreAligned64(b343 + 16, b + 2);
    }
    sq[0] = sq[6];
    sq[1] = sq[7];
    mas[0] = mas[2];
    bs[0] = bs[5];
    bs[1] = bs[6];
    ma343 += 32;
    b343 += 32;
    x += 32;
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
      kOverreadInBytesPass1_128 - sizeof(*src0) * width;
  __m128i s[2][4], ma3_128[2][3], ma5_128[3], sq_128[2][8], b3_128[2][10],
      b5_128[10];
  __m256i ma3[2][3], ma5[3], sq[2][8], b3[2][7], b5[7];
  s[0][0] = LoadUnaligned16Msan(src0 + 0, overread_in_bytes + 0);
  s[0][1] = LoadUnaligned16Msan(src0 + 8, overread_in_bytes + 16);
  s[1][0] = LoadUnaligned16Msan(src1 + 0, overread_in_bytes + 0);
  s[1][1] = LoadUnaligned16Msan(src1 + 8, overread_in_bytes + 16);
  Square(s[0][0], sq_128[0]);
  Square(s[1][0], sq_128[1]);
  BoxFilterPreProcessLo(s, scales, sum3, sum5, square_sum3, square_sum5, sq_128,
                        ma3_128, b3_128, &ma5_128[0], b5_128);
  sq[0][0] = SetrM128i(sq_128[0][2], sq_128[0][2]);
  sq[0][1] = SetrM128i(sq_128[0][3], sq_128[0][3]);
  sq[1][0] = SetrM128i(sq_128[1][2], sq_128[1][2]);
  sq[1][1] = SetrM128i(sq_128[1][3], sq_128[1][3]);
  ma3[0][0] = SetrM128i(ma3_128[0][0], ma3_128[0][0]);
  ma3[1][0] = SetrM128i(ma3_128[1][0], ma3_128[1][0]);
  ma5[0] = SetrM128i(ma5_128[0], ma5_128[0]);
  b3[0][0] = SetrM128i(b3_128[0][0], b3_128[0][0]);
  b3[0][1] = SetrM128i(b3_128[0][1], b3_128[0][1]);
  b3[1][0] = SetrM128i(b3_128[1][0], b3_128[1][0]);
  b3[1][1] = SetrM128i(b3_128[1][1], b3_128[1][1]);
  b5[0] = SetrM128i(b5_128[0], b5_128[0]);
  b5[1] = SetrM128i(b5_128[1], b5_128[1]);

  int x = 0;
  do {
    __m256i ma[2], b[4], ma3x[3], ma5x[3];
    BoxFilterPreProcess(
        src0 + x + 8, src1 + x + 8,
        kOverreadInBytesPass1_256 + sizeof(*src0) * (x + 8 - width), x + 8,
        scales, sum3, sum5, square_sum3, square_sum5, sum_width, sq, ma3, b3,
        ma5, b5);
    Prepare3_8(ma3[0], ma3x);
    ma[0] = Sum343Lo(ma3x);
    ma[1] = Sum343Hi(ma3x);
    StoreAligned64_ma(ma343[0] + x, ma);
    Sum343(b3[0], b);
    Sum343(b3[0] + 3, b + 2);
    StoreAligned64(b343[0] + x, b);
    StoreAligned64(b343[0] + x + 16, b + 2);
    Prepare3_8(ma3[1], ma3x);
    Store343_444Lo(ma3x, b3[1], x, ma343[1], ma444, b343[1], b444);
    Store343_444Hi(ma3x, b3[1] + 3, x + kMaStoreOffset, ma343[1], ma444,
                   b343[1], b444);
    Prepare3_8(ma5, ma5x);
    ma[0] = Sum565Lo(ma5x);
    ma[1] = Sum565Hi(ma5x);
    StoreAligned64_ma(ma565, ma);
    Sum565(b5, b);
    StoreAligned64(b565, b);
    Sum565(b5 + 3, b);
    StoreAligned64(b565 + 16, b);
    sq[0][0] = sq[0][6];
    sq[0][1] = sq[0][7];
    sq[1][0] = sq[1][6];
    sq[1][1] = sq[1][7];
    ma3[0][0] = ma3[0][2];
    ma3[1][0] = ma3[1][2];
    ma5[0] = ma5[2];
    b3[0][0] = b3[0][5];
    b3[0][1] = b3[0][6];
    b3[1][0] = b3[1][5];
    b3[1][1] = b3[1][6];
    b5[0] = b5[5];
    b5[1] = b5[6];
    ma565 += 32;
    b565 += 32;
    x += 32;
  } while (x < width);
}

template <int shift>
inline __m256i FilterOutput(const __m256i ma_x_src, const __m256i b) {
  // ma: 255 * 32 = 8160 (13 bits)
  // b: 65088 * 32 = 2082816 (21 bits)
  // v: b - ma * 255 (22 bits)
  const __m256i v = _mm256_sub_epi32(b, ma_x_src);
  // kSgrProjSgrBits = 8
  // kSgrProjRestoreBits = 4
  // shift = 4 or 5
  // v >> 8 or 9 (13 bits)
  return VrshrS32(v, kSgrProjSgrBits + shift - kSgrProjRestoreBits);
}

template <int shift>
inline __m256i CalculateFilteredOutput(const __m256i src, const __m256i ma,
                                       const __m256i b[2]) {
  const __m256i ma_x_src_lo = VmullLo16(ma, src);
  const __m256i ma_x_src_hi = VmullHi16(ma, src);
  const __m256i dst_lo = FilterOutput<shift>(ma_x_src_lo, b[0]);
  const __m256i dst_hi = FilterOutput<shift>(ma_x_src_hi, b[1]);
  return _mm256_packs_epi32(dst_lo, dst_hi);  // 13 bits
}

inline __m256i CalculateFilteredOutputPass1(const __m256i src,
                                            const __m256i ma[2],
                                            const __m256i b[2][2]) {
  const __m256i ma_sum = _mm256_add_epi16(ma[0], ma[1]);
  __m256i b_sum[2];
  b_sum[0] = _mm256_add_epi32(b[0][0], b[1][0]);
  b_sum[1] = _mm256_add_epi32(b[0][1], b[1][1]);
  return CalculateFilteredOutput<5>(src, ma_sum, b_sum);
}

inline __m256i CalculateFilteredOutputPass2(const __m256i src,
                                            const __m256i ma[3],
                                            const __m256i b[3][2]) {
  const __m256i ma_sum = Sum3_16(ma);
  __m256i b_sum[2];
  Sum3_32(b, b_sum);
  return CalculateFilteredOutput<5>(src, ma_sum, b_sum);
}

inline __m256i SelfGuidedFinal(const __m256i src, const __m256i v[2]) {
  const __m256i v_lo =
      VrshrS32(v[0], kSgrProjRestoreBits + kSgrProjPrecisionBits);
  const __m256i v_hi =
      VrshrS32(v[1], kSgrProjRestoreBits + kSgrProjPrecisionBits);
  const __m256i vv = _mm256_packs_epi32(v_lo, v_hi);
  return _mm256_add_epi16(src, vv);
}

inline __m256i SelfGuidedDoubleMultiplier(const __m256i src,
                                          const __m256i filter[2], const int w0,
                                          const int w2) {
  __m256i v[2];
  const __m256i w0_w2 =
      _mm256_set1_epi32((w2 << 16) | static_cast<uint16_t>(w0));
  const __m256i f_lo = _mm256_unpacklo_epi16(filter[0], filter[1]);
  const __m256i f_hi = _mm256_unpackhi_epi16(filter[0], filter[1]);
  v[0] = _mm256_madd_epi16(w0_w2, f_lo);
  v[1] = _mm256_madd_epi16(w0_w2, f_hi);
  return SelfGuidedFinal(src, v);
}

inline __m256i SelfGuidedSingleMultiplier(const __m256i src,
                                          const __m256i filter, const int w0) {
  // weight: -96 to 96 (Sgrproj_Xqd_Min/Max)
  __m256i v[2];
  v[0] = VmullNLo8(filter, w0);
  v[1] = VmullNHi8(filter, w0);
  return SelfGuidedFinal(src, v);
}

inline void ClipAndStore(uint16_t* const dst, const __m256i val) {
  const __m256i val0 = _mm256_max_epi16(val, _mm256_setzero_si256());
  const __m256i val1 = _mm256_min_epi16(val0, _mm256_set1_epi16(1023));
  StoreUnaligned32(dst, val1);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPass1(
    const uint16_t* const src, const uint16_t* const src0,
    const uint16_t* const src1, const ptrdiff_t stride, uint16_t* const sum5[5],
    uint32_t* const square_sum5[5], const int width, const ptrdiff_t sum_width,
    const uint32_t scale, const int16_t w0, uint16_t* const ma565[2],
    uint32_t* const b565[2], uint16_t* const dst) {
  const ptrdiff_t overread_in_bytes =
      kOverreadInBytesPass1_128 - sizeof(*src0) * width;
  __m128i s[2][2], ma0, sq_128[2][4], b0[2];
  __m256i mas[3], sq[2][8], bs[7];
  s[0][0] = LoadUnaligned16Msan(src0 + 0, overread_in_bytes + 0);
  s[0][1] = LoadUnaligned16Msan(src0 + 8, overread_in_bytes + 16);
  s[1][0] = LoadUnaligned16Msan(src1 + 0, overread_in_bytes + 0);
  s[1][1] = LoadUnaligned16Msan(src1 + 8, overread_in_bytes + 16);
  Square(s[0][0], sq_128[0]);
  Square(s[1][0], sq_128[1]);
  BoxFilterPreProcess5Lo(s, scale, sum5, square_sum5, sq_128, &ma0, b0);
  sq[0][0] = SetrM128i(sq_128[0][2], sq_128[0][2]);
  sq[0][1] = SetrM128i(sq_128[0][3], sq_128[0][3]);
  sq[1][0] = SetrM128i(sq_128[1][2], sq_128[1][2]);
  sq[1][1] = SetrM128i(sq_128[1][3], sq_128[1][3]);
  mas[0] = SetrM128i(ma0, ma0);
  bs[0] = SetrM128i(b0[0], b0[0]);
  bs[1] = SetrM128i(b0[1], b0[1]);

  int x = 0;
  do {
    __m256i ma5[3], ma[4], b[4][2];
    BoxFilterPreProcess5(
        src0 + x + 8, src1 + x + 8,
        kOverreadInBytesPass1_256 + sizeof(*src0) * (x + 8 - width), sum_width,
        x + 8, scale, sum5, square_sum5, sq, mas, bs);
    Prepare3_8(mas, ma5);
    ma[2] = Sum565Lo(ma5);
    ma[3] = Sum565Hi(ma5);
    ma[1] = _mm256_permute2x128_si256(ma[2], ma[3], 0x20);
    ma[3] = _mm256_permute2x128_si256(ma[2], ma[3], 0x31);
    StoreAligned32(ma565[1] + x + 0, ma[1]);
    StoreAligned32(ma565[1] + x + 16, ma[3]);
    Sum565(bs + 0, b[1]);
    Sum565(bs + 3, b[3]);
    StoreAligned64(b565[1] + x, b[1]);
    StoreAligned64(b565[1] + x + 16, b[3]);
    const __m256i sr0_lo = LoadUnaligned32(src + x + 0);
    ma[0] = LoadAligned32(ma565[0] + x);
    LoadAligned64(b565[0] + x, b[0]);
    const __m256i p0 = CalculateFilteredOutputPass1(sr0_lo, ma, b);
    const __m256i d0 = SelfGuidedSingleMultiplier(sr0_lo, p0, w0);
    ClipAndStore(dst + x + 0, d0);
    const __m256i sr0_hi = LoadUnaligned32(src + x + 16);
    ma[2] = LoadAligned32(ma565[0] + x + 16);
    LoadAligned64(b565[0] + x + 16, b[2]);
    const __m256i p1 = CalculateFilteredOutputPass1(sr0_hi, ma + 2, b + 2);
    const __m256i d1 = SelfGuidedSingleMultiplier(sr0_hi, p1, w0);
    ClipAndStore(dst + x + 16, d1);
    const __m256i sr1_lo = LoadUnaligned32(src + stride + x + 0);
    const __m256i p10 = CalculateFilteredOutput<4>(sr1_lo, ma[1], b[1]);
    const __m256i d10 = SelfGuidedSingleMultiplier(sr1_lo, p10, w0);
    ClipAndStore(dst + stride + x + 0, d10);
    const __m256i sr1_hi = LoadUnaligned32(src + stride + x + 16);
    const __m256i p11 = CalculateFilteredOutput<4>(sr1_hi, ma[3], b[3]);
    const __m256i d11 = SelfGuidedSingleMultiplier(sr1_hi, p11, w0);
    ClipAndStore(dst + stride + x + 16, d11);
    sq[0][0] = sq[0][6];
    sq[0][1] = sq[0][7];
    sq[1][0] = sq[1][6];
    sq[1][1] = sq[1][7];
    mas[0] = mas[2];
    bs[0] = bs[5];
    bs[1] = bs[6];
    x += 32;
  } while (x < width);
}

inline void BoxFilterPass1LastRow(
    const uint16_t* const src, const uint16_t* const src0, const int width,
    const ptrdiff_t sum_width, const uint32_t scale, const int16_t w0,
    uint16_t* const sum5[5], uint32_t* const square_sum5[5], uint16_t* ma565,
    uint32_t* b565, uint16_t* const dst) {
  const ptrdiff_t overread_in_bytes =
      kOverreadInBytesPass1_128 - sizeof(*src0) * width;
  __m128i s[2], ma0[2], sq_128[8], b0[6];
  __m256i mas[3], sq[8], bs[7];
  s[0] = LoadUnaligned16Msan(src0 + 0, overread_in_bytes + 0);
  s[1] = LoadUnaligned16Msan(src0 + 8, overread_in_bytes + 16);
  Square(s[0], sq_128);
  BoxFilterPreProcess5LastRowLo(s, scale, sum5, square_sum5, sq_128, &ma0[0],
                                b0);
  sq[0] = SetrM128i(sq_128[2], sq_128[2]);
  sq[1] = SetrM128i(sq_128[3], sq_128[3]);
  mas[0] = SetrM128i(ma0[0], ma0[0]);
  bs[0] = SetrM128i(b0[0], b0[0]);
  bs[1] = SetrM128i(b0[1], b0[1]);

  int x = 0;
  do {
    __m256i ma5[3], ma[4], b[4][2];
    BoxFilterPreProcess5LastRow(
        src0 + x + 8,
        kOverreadInBytesPass1_256 + sizeof(*src0) * (x + 8 - width), sum_width,
        x + 8, scale, sum5, square_sum5, sq, mas, bs);
    Prepare3_8(mas, ma5);
    ma[2] = Sum565Lo(ma5);
    ma[3] = Sum565Hi(ma5);
    Sum565(bs + 0, b[1]);
    Sum565(bs + 3, b[3]);
    const __m256i sr0_lo = LoadUnaligned32(src + x + 0);
    ma[0] = LoadAligned32(ma565 + x);
    ma[1] = _mm256_permute2x128_si256(ma[2], ma[3], 0x20);
    LoadAligned64(b565 + x, b[0]);
    const __m256i p0 = CalculateFilteredOutputPass1(sr0_lo, ma, b);
    const __m256i d0 = SelfGuidedSingleMultiplier(sr0_lo, p0, w0);
    ClipAndStore(dst + x + 0, d0);
    const __m256i sr0_hi = LoadUnaligned32(src + x + 16);
    ma[0] = LoadAligned32(ma565 + x + 16);
    ma[1] = _mm256_permute2x128_si256(ma[2], ma[3], 0x31);
    LoadAligned64(b565 + x + 16, b[2]);
    const __m256i p1 = CalculateFilteredOutputPass1(sr0_hi, ma, b + 2);
    const __m256i d1 = SelfGuidedSingleMultiplier(sr0_hi, p1, w0);
    ClipAndStore(dst + x + 16, d1);
    sq[0] = sq[6];
    sq[1] = sq[7];
    mas[0] = mas[2];
    bs[0] = bs[5];
    bs[1] = bs[6];
    x += 32;
  } while (x < width);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterPass2(
    const uint16_t* const src, const uint16_t* const src0, const int width,
    const ptrdiff_t sum_width, const uint32_t scale, const int16_t w0,
    uint16_t* const sum3[3], uint32_t* const square_sum3[3],
    uint16_t* const ma343[3], uint16_t* const ma444[2], uint32_t* const b343[3],
    uint32_t* const b444[2], uint16_t* const dst) {
  const ptrdiff_t overread_in_bytes_128 =
      kOverreadInBytesPass2_128 - sizeof(*src0) * width;
  __m128i s0[2], ma0, sq_128[4], b0[2];
  __m256i mas[3], sq[8], bs[7];
  s0[0] = LoadUnaligned16Msan(src0 + 0, overread_in_bytes_128 + 0);
  s0[1] = LoadUnaligned16Msan(src0 + 8, overread_in_bytes_128 + 16);
  Square(s0[0], sq_128);
  BoxFilterPreProcess3Lo(s0, scale, sum3, square_sum3, sq_128, &ma0, b0);
  sq[0] = SetrM128i(sq_128[2], sq_128[2]);
  sq[1] = SetrM128i(sq_128[3], sq_128[3]);
  mas[0] = SetrM128i(ma0, ma0);
  bs[0] = SetrM128i(b0[0], b0[0]);
  bs[1] = SetrM128i(b0[1], b0[1]);

  int x = 0;
  do {
    __m256i ma[4], b[4][2], ma3[3];
    BoxFilterPreProcess3(
        src0 + x + 8,
        kOverreadInBytesPass2_256 + sizeof(*src0) * (x + 8 - width), x + 8,
        sum_width, scale, sum3, square_sum3, sq, mas, bs);
    Prepare3_8(mas, ma3);
    Store343_444(ma3, bs, x, &ma[2], &ma[3], b[2], b[3], ma343[2], ma444[1],
                 b343[2], b444[1]);
    const __m256i sr_lo = LoadUnaligned32(src + x + 0);
    const __m256i sr_hi = LoadUnaligned32(src + x + 16);
    ma[0] = LoadAligned32(ma343[0] + x);
    ma[1] = LoadAligned32(ma444[0] + x);
    LoadAligned64(b343[0] + x, b[0]);
    LoadAligned64(b444[0] + x, b[1]);
    const __m256i p0 = CalculateFilteredOutputPass2(sr_lo, ma, b);
    ma[1] = LoadAligned32(ma343[0] + x + 16);
    ma[2] = LoadAligned32(ma444[0] + x + 16);
    LoadAligned64(b343[0] + x + 16, b[1]);
    LoadAligned64(b444[0] + x + 16, b[2]);
    const __m256i p1 = CalculateFilteredOutputPass2(sr_hi, ma + 1, b + 1);
    const __m256i d0 = SelfGuidedSingleMultiplier(sr_lo, p0, w0);
    const __m256i d1 = SelfGuidedSingleMultiplier(sr_hi, p1, w0);
    ClipAndStore(dst + x + 0, d0);
    ClipAndStore(dst + x + 16, d1);
    sq[0] = sq[6];
    sq[1] = sq[7];
    mas[0] = mas[2];
    bs[0] = bs[5];
    bs[1] = bs[6];
    x += 32;
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
      kOverreadInBytesPass1_128 - sizeof(*src0) * width;
  __m128i s[2][4], ma3_128[2][3], ma5_0, sq_128[2][8], b3_128[2][10], b5_128[2];
  __m256i ma3[2][3], ma5[3], sq[2][8], b3[2][7], b5[7];
  s[0][0] = LoadUnaligned16Msan(src0 + 0, overread_in_bytes + 0);
  s[0][1] = LoadUnaligned16Msan(src0 + 8, overread_in_bytes + 16);
  s[1][0] = LoadUnaligned16Msan(src1 + 0, overread_in_bytes + 0);
  s[1][1] = LoadUnaligned16Msan(src1 + 8, overread_in_bytes + 16);
  Square(s[0][0], sq_128[0]);
  Square(s[1][0], sq_128[1]);
  BoxFilterPreProcessLo(s, scales, sum3, sum5, square_sum3, square_sum5, sq_128,
                        ma3_128, b3_128, &ma5_0, b5_128);
  sq[0][0] = SetrM128i(sq_128[0][2], sq_128[0][2]);
  sq[0][1] = SetrM128i(sq_128[0][3], sq_128[0][3]);
  sq[1][0] = SetrM128i(sq_128[1][2], sq_128[1][2]);
  sq[1][1] = SetrM128i(sq_128[1][3], sq_128[1][3]);
  ma3[0][0] = SetrM128i(ma3_128[0][0], ma3_128[0][0]);
  ma3[1][0] = SetrM128i(ma3_128[1][0], ma3_128[1][0]);
  ma5[0] = SetrM128i(ma5_0, ma5_0);
  b3[0][0] = SetrM128i(b3_128[0][0], b3_128[0][0]);
  b3[0][1] = SetrM128i(b3_128[0][1], b3_128[0][1]);
  b3[1][0] = SetrM128i(b3_128[1][0], b3_128[1][0]);
  b3[1][1] = SetrM128i(b3_128[1][1], b3_128[1][1]);
  b5[0] = SetrM128i(b5_128[0], b5_128[0]);
  b5[1] = SetrM128i(b5_128[1], b5_128[1]);

  int x = 0;
  do {
    __m256i ma[3][4], mat[3][3], b[3][3][2], bt[3][3][2], p[2][2], ma3x[2][3],
        ma5x[3];
    BoxFilterPreProcess(
        src0 + x + 8, src1 + x + 8,
        kOverreadInBytesPass1_256 + sizeof(*src0) * (x + 8 - width), x + 8,
        scales, sum3, sum5, square_sum3, square_sum5, sum_width, sq, ma3, b3,
        ma5, b5);
    Prepare3_8(ma3[0], ma3x[0]);
    Prepare3_8(ma3[1], ma3x[1]);
    Prepare3_8(ma5, ma5x);
    Store343_444(ma3x[0], b3[0], x, &ma[1][2], &mat[1][2], &ma[2][1],
                 &mat[2][1], b[1][2], bt[1][2], b[2][1], bt[2][1], ma343[2],
                 ma444[1], b343[2], b444[1]);
    Store343_444(ma3x[1], b3[1], x, &ma[2][2], &mat[2][2], b[2][2], bt[2][2],
                 ma343[3], ma444[2], b343[3], b444[2]);

    ma[0][2] = Sum565Lo(ma5x);
    ma[0][3] = Sum565Hi(ma5x);
    ma[0][1] = _mm256_permute2x128_si256(ma[0][2], ma[0][3], 0x20);
    ma[0][3] = _mm256_permute2x128_si256(ma[0][2], ma[0][3], 0x31);
    StoreAligned32(ma565[1] + x + 0, ma[0][1]);
    StoreAligned32(ma565[1] + x + 16, ma[0][3]);
    Sum565(b5, b[0][1]);
    StoreAligned64(b565[1] + x, b[0][1]);
    const __m256i sr0_lo = LoadUnaligned32(src + x);
    const __m256i sr1_lo = LoadUnaligned32(src + stride + x);
    ma[0][0] = LoadAligned32(ma565[0] + x);
    LoadAligned64(b565[0] + x, b[0][0]);
    p[0][0] = CalculateFilteredOutputPass1(sr0_lo, ma[0], b[0]);
    p[1][0] = CalculateFilteredOutput<4>(sr1_lo, ma[0][1], b[0][1]);
    ma[1][0] = LoadAligned32(ma343[0] + x);
    ma[1][1] = LoadAligned32(ma444[0] + x);
    // Keeping the following 4 redundant lines is faster. The reason is that
    // there are not enough registers available, and these values could be saved
    // and loaded which is even slower.
    ma[1][2] = LoadAligned32(ma343[2] + x);  // Redundant line 1.
    LoadAligned64(b343[0] + x, b[1][0]);
    LoadAligned64(b444[0] + x, b[1][1]);
    p[0][1] = CalculateFilteredOutputPass2(sr0_lo, ma[1], b[1]);
    ma[2][0] = LoadAligned32(ma343[1] + x);
    ma[2][1] = LoadAligned32(ma444[1] + x);  // Redundant line 2.
    LoadAligned64(b343[1] + x, b[2][0]);
    p[1][1] = CalculateFilteredOutputPass2(sr1_lo, ma[2], b[2]);
    const __m256i d00 = SelfGuidedDoubleMultiplier(sr0_lo, p[0], w0, w2);
    ClipAndStore(dst + x, d00);
    const __m256i d10x = SelfGuidedDoubleMultiplier(sr1_lo, p[1], w0, w2);
    ClipAndStore(dst + stride + x, d10x);

    Sum565(b5 + 3, bt[0][1]);
    StoreAligned64(b565[1] + x + 16, bt[0][1]);
    const __m256i sr0_hi = LoadUnaligned32(src + x + 16);
    const __m256i sr1_hi = LoadUnaligned32(src + stride + x + 16);
    ma[0][2] = LoadAligned32(ma565[0] + x + 16);
    LoadAligned64(b565[0] + x + 16, bt[0][0]);
    p[0][0] = CalculateFilteredOutputPass1(sr0_hi, ma[0] + 2, bt[0]);
    p[1][0] = CalculateFilteredOutput<4>(sr1_hi, ma[0][3], bt[0][1]);
    mat[1][0] = LoadAligned32(ma343[0] + x + 16);
    mat[1][1] = LoadAligned32(ma444[0] + x + 16);
    mat[1][2] = LoadAligned32(ma343[2] + x + 16);  // Redundant line 3.
    LoadAligned64(b343[0] + x + 16, bt[1][0]);
    LoadAligned64(b444[0] + x + 16, bt[1][1]);
    p[0][1] = CalculateFilteredOutputPass2(sr0_hi, mat[1], bt[1]);
    mat[2][0] = LoadAligned32(ma343[1] + x + 16);
    mat[2][1] = LoadAligned32(ma444[1] + x + 16);  // Redundant line 4.
    LoadAligned64(b343[1] + x + 16, bt[2][0]);
    p[1][1] = CalculateFilteredOutputPass2(sr1_hi, mat[2], bt[2]);
    const __m256i d01 = SelfGuidedDoubleMultiplier(sr0_hi, p[0], w0, w2);
    ClipAndStore(dst + x + 16, d01);
    const __m256i d11 = SelfGuidedDoubleMultiplier(sr1_hi, p[1], w0, w2);
    ClipAndStore(dst + stride + x + 16, d11);

    sq[0][0] = sq[0][6];
    sq[0][1] = sq[0][7];
    sq[1][0] = sq[1][6];
    sq[1][1] = sq[1][7];
    ma3[0][0] = ma3[0][2];
    ma3[1][0] = ma3[1][2];
    ma5[0] = ma5[2];
    b3[0][0] = b3[0][5];
    b3[0][1] = b3[0][6];
    b3[1][0] = b3[1][5];
    b3[1][1] = b3[1][6];
    b5[0] = b5[5];
    b5[1] = b5[6];
    x += 32;
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
      kOverreadInBytesPass1_128 - sizeof(*src0) * width;
  __m128i s[2], ma3_0, ma5_0, sq_128[4], b3_128[2], b5_128[2];
  __m256i ma3[3], ma5[3], sq[8], b3[7], b5[7];
  s[0] = LoadUnaligned16Msan(src0 + 0, overread_in_bytes + 0);
  s[1] = LoadUnaligned16Msan(src0 + 8, overread_in_bytes + 16);
  Square(s[0], sq_128);
  BoxFilterPreProcessLastRowLo(s, scales, sum3, sum5, square_sum3, square_sum5,
                               sq_128, &ma3_0, &ma5_0, b3_128, b5_128);
  sq[0] = SetrM128i(sq_128[2], sq_128[2]);
  sq[1] = SetrM128i(sq_128[3], sq_128[3]);
  ma3[0] = SetrM128i(ma3_0, ma3_0);
  ma5[0] = SetrM128i(ma5_0, ma5_0);
  b3[0] = SetrM128i(b3_128[0], b3_128[0]);
  b3[1] = SetrM128i(b3_128[1], b3_128[1]);
  b5[0] = SetrM128i(b5_128[0], b5_128[0]);
  b5[1] = SetrM128i(b5_128[1], b5_128[1]);

  int x = 0;
  do {
    __m256i ma[4], mat[4], b[3][2], bt[3][2], ma3x[3], ma5x[3], p[2];
    BoxFilterPreProcessLastRow(
        src0 + x + 8,
        kOverreadInBytesPass1_256 + sizeof(*src0) * (x + 8 - width), sum_width,
        x + 8, scales, sum3, sum5, square_sum3, square_sum5, sq, ma3, ma5, b3,
        b5);
    Prepare3_8(ma3, ma3x);
    Prepare3_8(ma5, ma5x);
    ma[2] = Sum565Lo(ma5x);
    Sum565(b5, b[1]);
    mat[1] = Sum565Hi(ma5x);
    Sum565(b5 + 3, bt[1]);
    ma[3] = Sum343Lo(ma3x);
    Sum343(b3, b[2]);
    mat[2] = Sum343Hi(ma3x);
    Sum343(b3 + 3, bt[2]);

    const __m256i sr_lo = LoadUnaligned32(src + x);
    ma[0] = LoadAligned32(ma565 + x);
    ma[1] = _mm256_permute2x128_si256(ma[2], mat[1], 0x20);
    mat[1] = _mm256_permute2x128_si256(ma[2], mat[1], 0x31);
    LoadAligned64(b565 + x, b[0]);
    p[0] = CalculateFilteredOutputPass1(sr_lo, ma, b);
    ma[0] = LoadAligned32(ma343 + x);
    ma[1] = LoadAligned32(ma444 + x);
    ma[2] = _mm256_permute2x128_si256(ma[3], mat[2], 0x20);
    LoadAligned64(b343 + x, b[0]);
    LoadAligned64(b444 + x, b[1]);
    p[1] = CalculateFilteredOutputPass2(sr_lo, ma, b);
    const __m256i d0 = SelfGuidedDoubleMultiplier(sr_lo, p, w0, w2);

    const __m256i sr_hi = LoadUnaligned32(src + x + 16);
    mat[0] = LoadAligned32(ma565 + x + 16);
    LoadAligned64(b565 + x + 16, bt[0]);
    p[0] = CalculateFilteredOutputPass1(sr_hi, mat, bt);
    mat[0] = LoadAligned32(ma343 + x + 16);
    mat[1] = LoadAligned32(ma444 + x + 16);
    mat[2] = _mm256_permute2x128_si256(ma[3], mat[2], 0x31);
    LoadAligned64(b343 + x + 16, bt[0]);
    LoadAligned64(b444 + x + 16, bt[1]);
    p[1] = CalculateFilteredOutputPass2(sr_hi, mat, bt);
    const __m256i d1 = SelfGuidedDoubleMultiplier(sr_hi, p, w0, w2);
    ClipAndStore(dst + x + 0, d0);
    ClipAndStore(dst + x + 16, d1);

    sq[0] = sq[6];
    sq[1] = sq[7];
    ma3[0] = ma3[2];
    ma5[0] = ma5[2];
    b3[0] = b3[5];
    b3[1] = b3[6];
    b5[0] = b5[5];
    b5[1] = b5[6];
    x += 32;
  } while (x < width);
}

LIBGAV1_ALWAYS_INLINE void BoxFilterProcess(
    const RestorationUnitInfo& restoration_info, const uint16_t* src,
    const ptrdiff_t stride, const uint16_t* const top_border,
    const ptrdiff_t top_border_stride, const uint16_t* bottom_border,
    const ptrdiff_t bottom_border_stride, const int width, const int height,
    SgrBuffer* const sgr_buffer, uint16_t* dst) {
  const auto temp_stride = Align<ptrdiff_t>(width, 32);
  const auto sum_width = temp_stride + 8;
  const auto sum_stride = temp_stride + 32;
  const int sgr_proj_index = restoration_info.sgr_proj_info.index;
  const uint16_t* const scales = kSgrScaleParameter[sgr_proj_index];  // < 2^12.
  const int16_t w0 = restoration_info.sgr_proj_info.multiplier[0];
  const int16_t w1 = restoration_info.sgr_proj_info.multiplier[1];
  const int16_t w2 = (1 << kSgrProjPrecisionBits) - w0 - w1;
  uint16_t *sum3[4], *sum5[5], *ma343[4], *ma444[3], *ma565[2];
  uint32_t *square_sum3[4], *square_sum5[5], *b343[4], *b444[3], *b565[2];
  sum3[0] = sgr_buffer->sum3 + kSumOffset;
  square_sum3[0] = sgr_buffer->square_sum3 + kSumOffset;
  ma343[0] = sgr_buffer->ma343;
  b343[0] = sgr_buffer->b343;
  for (int i = 1; i <= 3; ++i) {
    sum3[i] = sum3[i - 1] + sum_stride;
    square_sum3[i] = square_sum3[i - 1] + sum_stride;
    ma343[i] = ma343[i - 1] + temp_stride;
    b343[i] = b343[i - 1] + temp_stride;
  }
  sum5[0] = sgr_buffer->sum5 + kSumOffset;
  square_sum5[0] = sgr_buffer->square_sum5 + kSumOffset;
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
  BoxSum(top_border, top_border_stride, width, sum_stride, temp_stride, sum3[0],
         sum5[1], square_sum3[0], square_sum5[1]);
  sum5[0] = sum5[1];
  square_sum5[0] = square_sum5[1];
  const uint16_t* const s = (height > 1) ? src + stride : bottom_border;
  BoxSumFilterPreProcess(src, s, width, scales, sum3, sum5, square_sum3,
                         square_sum5, sum_width, ma343, ma444[0], ma565[0],
                         b343, b444[0], b565[0]);
  sum5[0] = sgr_buffer->sum5 + kSumOffset;
  square_sum5[0] = sgr_buffer->square_sum5 + kSumOffset;

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
  const auto temp_stride = Align<ptrdiff_t>(width, 32);
  const auto sum_width = temp_stride + 8;
  const auto sum_stride = temp_stride + 32;
  const int sgr_proj_index = restoration_info.sgr_proj_info.index;
  const uint32_t scale = kSgrScaleParameter[sgr_proj_index][0];  // < 2^12.
  const int16_t w0 = restoration_info.sgr_proj_info.multiplier[0];
  uint16_t *sum5[5], *ma565[2];
  uint32_t *square_sum5[5], *b565[2];
  sum5[0] = sgr_buffer->sum5 + kSumOffset;
  square_sum5[0] = sgr_buffer->square_sum5 + kSumOffset;
  for (int i = 1; i <= 4; ++i) {
    sum5[i] = sum5[i - 1] + sum_stride;
    square_sum5[i] = square_sum5[i - 1] + sum_stride;
  }
  ma565[0] = sgr_buffer->ma565;
  ma565[1] = ma565[0] + temp_stride;
  b565[0] = sgr_buffer->b565;
  b565[1] = b565[0] + temp_stride;
  assert(scale != 0);
  BoxSum<5>(top_border, top_border_stride, width, sum_stride, temp_stride,
            sum5[1], square_sum5[1]);
  sum5[0] = sum5[1];
  square_sum5[0] = square_sum5[1];
  const uint16_t* const s = (height > 1) ? src + stride : bottom_border;
  BoxSumFilterPreProcess5(src, s, width, scale, sum5, square_sum5, sum_width,
                          ma565[0], b565[0]);
  sum5[0] = sgr_buffer->sum5 + kSumOffset;
  square_sum5[0] = sgr_buffer->square_sum5 + kSumOffset;

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
  const auto temp_stride = Align<ptrdiff_t>(width, 32);
  const auto sum_width = temp_stride + 8;
  const auto sum_stride = temp_stride + 32;
  const int16_t w1 = restoration_info.sgr_proj_info.multiplier[1];
  const int16_t w0 = (1 << kSgrProjPrecisionBits) - w1;
  const int sgr_proj_index = restoration_info.sgr_proj_info.index;
  const uint32_t scale = kSgrScaleParameter[sgr_proj_index][1];  // < 2^12.
  uint16_t *sum3[3], *ma343[3], *ma444[2];
  uint32_t *square_sum3[3], *b343[3], *b444[2];
  sum3[0] = sgr_buffer->sum3 + kSumOffset;
  square_sum3[0] = sgr_buffer->square_sum3 + kSumOffset;
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
  BoxSum<3>(top_border, top_border_stride, width, sum_stride, temp_stride,
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

// If |width| is non-multiple of 32, up to 31 more pixels are written to |dest|
// in the end of each row. It is safe to overwrite the output as it will not be
// part of the visible frame.
void SelfGuidedFilter_AVX2(
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
  const auto* const top = static_cast<const uint16_t*>(top_border);
  const auto* const bottom = static_cast<const uint16_t*>(bottom_border);
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
#if DSP_ENABLED_10BPP_AVX2(WienerFilter)
  dsp->loop_restorations[0] = WienerFilter_AVX2;
#endif
#if DSP_ENABLED_10BPP_AVX2(SelfGuidedFilter)
  dsp->loop_restorations[1] = SelfGuidedFilter_AVX2;
#endif
}

}  // namespace

void LoopRestorationInit10bpp_AVX2() { Init10bpp(); }

}  // namespace dsp
}  // namespace libgav1

#else   // !(LIBGAV1_TARGETING_AVX2 && LIBGAV1_MAX_BITDEPTH >= 10)
namespace libgav1 {
namespace dsp {

void LoopRestorationInit10bpp_AVX2() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_TARGETING_AVX2 && LIBGAV1_MAX_BITDEPTH >= 10
