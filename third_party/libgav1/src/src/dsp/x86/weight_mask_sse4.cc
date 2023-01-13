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

#include "src/dsp/x86/weight_mask_sse4.h"

#include "src/utils/cpu.h"

#if LIBGAV1_TARGETING_SSE4_1

#include <smmintrin.h>

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/dsp/x86/common_sse4.h"
#include "src/utils/common.h"

namespace libgav1 {
namespace dsp {
namespace low_bitdepth {
namespace {

constexpr int kRoundingBits8bpp = 4;

template <bool mask_is_inverse, bool is_store_16>
inline void WeightMask16_SSE4_1(const int16_t* LIBGAV1_RESTRICT prediction_0,
                                const int16_t* LIBGAV1_RESTRICT prediction_1,
                                uint8_t* LIBGAV1_RESTRICT mask,
                                ptrdiff_t mask_stride) {
  const __m128i pred_00 = LoadAligned16(prediction_0);
  const __m128i pred_10 = LoadAligned16(prediction_1);
  const __m128i difference_0 = RightShiftWithRounding_U16(
      _mm_abs_epi16(_mm_sub_epi16(pred_00, pred_10)), kRoundingBits8bpp);
  const __m128i scaled_difference_0 = _mm_srli_epi16(difference_0, 4);

  const __m128i pred_01 = LoadAligned16(prediction_0 + 8);
  const __m128i pred_11 = LoadAligned16(prediction_1 + 8);
  const __m128i difference_1 = RightShiftWithRounding_U16(
      _mm_abs_epi16(_mm_sub_epi16(pred_01, pred_11)), kRoundingBits8bpp);
  const __m128i scaled_difference_1 = _mm_srli_epi16(difference_1, 4);

  const __m128i difference_offset = _mm_set1_epi8(38);
  const __m128i adjusted_difference =
      _mm_adds_epu8(_mm_packus_epi16(scaled_difference_0, scaled_difference_1),
                    difference_offset);
  const __m128i mask_ceiling = _mm_set1_epi8(64);
  const __m128i mask_value = _mm_min_epi8(adjusted_difference, mask_ceiling);
  if (mask_is_inverse) {
    const __m128i inverted_mask_value = _mm_sub_epi8(mask_ceiling, mask_value);
    if (is_store_16) {
      StoreAligned16(mask, inverted_mask_value);
    } else {
      StoreLo8(mask, inverted_mask_value);
      StoreHi8(mask + mask_stride, inverted_mask_value);
    }
  } else {
    if (is_store_16) {
      StoreAligned16(mask, mask_value);
    } else {
      StoreLo8(mask, mask_value);
      StoreHi8(mask + mask_stride, mask_value);
    }
  }
}

#define WEIGHT8_PAIR_WITHOUT_STRIDE \
  WeightMask16_SSE4_1<mask_is_inverse, false>(pred_0, pred_1, mask, mask_stride)

#define WEIGHT8_PAIR_AND_STRIDE \
  WEIGHT8_PAIR_WITHOUT_STRIDE;  \
  pred_0 += 8 << 1;             \
  pred_1 += 8 << 1;             \
  mask += mask_stride << 1

template <bool mask_is_inverse>
void WeightMask8x8_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                          const void* LIBGAV1_RESTRICT prediction_1,
                          uint8_t* LIBGAV1_RESTRICT mask,
                          ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);

  WEIGHT8_PAIR_AND_STRIDE;
  WEIGHT8_PAIR_AND_STRIDE;
  WEIGHT8_PAIR_AND_STRIDE;
  WEIGHT8_PAIR_WITHOUT_STRIDE;
}

template <bool mask_is_inverse>
void WeightMask8x16_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                           const void* LIBGAV1_RESTRICT prediction_1,
                           uint8_t* LIBGAV1_RESTRICT mask,
                           ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y3 = 3;
  do {
    WEIGHT8_PAIR_AND_STRIDE;
    WEIGHT8_PAIR_AND_STRIDE;
  } while (--y3 != 0);
  WEIGHT8_PAIR_AND_STRIDE;
  WEIGHT8_PAIR_WITHOUT_STRIDE;
}

template <bool mask_is_inverse>
void WeightMask8x32_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                           const void* LIBGAV1_RESTRICT prediction_1,
                           uint8_t* LIBGAV1_RESTRICT mask,
                           ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y5 = 5;
  do {
    WEIGHT8_PAIR_AND_STRIDE;
    WEIGHT8_PAIR_AND_STRIDE;
    WEIGHT8_PAIR_AND_STRIDE;
  } while (--y5 != 0);
  WEIGHT8_PAIR_WITHOUT_STRIDE;
}

#define WEIGHT16_WITHOUT_STRIDE \
  WeightMask16_SSE4_1<mask_is_inverse, true>(pred_0, pred_1, mask, mask_stride)

#define WEIGHT16_AND_STRIDE \
  WEIGHT16_WITHOUT_STRIDE;  \
  pred_0 += 16;             \
  pred_1 += 16;             \
  mask += mask_stride

template <bool mask_is_inverse>
void WeightMask16x8_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                           const void* LIBGAV1_RESTRICT prediction_1,
                           uint8_t* LIBGAV1_RESTRICT mask,
                           ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y = 7;
  do {
    WEIGHT16_AND_STRIDE;
  } while (--y != 0);
  WEIGHT16_WITHOUT_STRIDE;
}

template <bool mask_is_inverse>
void WeightMask16x16_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                            const void* LIBGAV1_RESTRICT prediction_1,
                            uint8_t* LIBGAV1_RESTRICT mask,
                            ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y3 = 5;
  do {
    WEIGHT16_AND_STRIDE;
    WEIGHT16_AND_STRIDE;
    WEIGHT16_AND_STRIDE;
  } while (--y3 != 0);
  WEIGHT16_WITHOUT_STRIDE;
}

template <bool mask_is_inverse>
void WeightMask16x32_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                            const void* LIBGAV1_RESTRICT prediction_1,
                            uint8_t* LIBGAV1_RESTRICT mask,
                            ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y5 = 6;
  do {
    WEIGHT16_AND_STRIDE;
    WEIGHT16_AND_STRIDE;
    WEIGHT16_AND_STRIDE;
    WEIGHT16_AND_STRIDE;
    WEIGHT16_AND_STRIDE;
  } while (--y5 != 0);
  WEIGHT16_AND_STRIDE;
  WEIGHT16_WITHOUT_STRIDE;
}

template <bool mask_is_inverse>
void WeightMask16x64_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                            const void* LIBGAV1_RESTRICT prediction_1,
                            uint8_t* LIBGAV1_RESTRICT mask,
                            ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y3 = 21;
  do {
    WEIGHT16_AND_STRIDE;
    WEIGHT16_AND_STRIDE;
    WEIGHT16_AND_STRIDE;
  } while (--y3 != 0);
  WEIGHT16_WITHOUT_STRIDE;
}

#define WEIGHT32_WITHOUT_STRIDE                                        \
  WeightMask16_SSE4_1<mask_is_inverse, true>(pred_0, pred_1, mask,     \
                                             mask_stride);             \
  WeightMask16_SSE4_1<mask_is_inverse, true>(pred_0 + 16, pred_1 + 16, \
                                             mask + 16, mask_stride)

#define WEIGHT32_AND_STRIDE \
  WEIGHT32_WITHOUT_STRIDE;  \
  pred_0 += 32;             \
  pred_1 += 32;             \
  mask += mask_stride

template <bool mask_is_inverse>
void WeightMask32x8_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                           const void* LIBGAV1_RESTRICT prediction_1,
                           uint8_t* LIBGAV1_RESTRICT mask,
                           ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  WEIGHT32_AND_STRIDE;
  WEIGHT32_AND_STRIDE;
  WEIGHT32_AND_STRIDE;
  WEIGHT32_AND_STRIDE;
  WEIGHT32_AND_STRIDE;
  WEIGHT32_AND_STRIDE;
  WEIGHT32_AND_STRIDE;
  WEIGHT32_WITHOUT_STRIDE;
}

template <bool mask_is_inverse>
void WeightMask32x16_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                            const void* LIBGAV1_RESTRICT prediction_1,
                            uint8_t* LIBGAV1_RESTRICT mask,
                            ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y3 = 5;
  do {
    WEIGHT32_AND_STRIDE;
    WEIGHT32_AND_STRIDE;
    WEIGHT32_AND_STRIDE;
  } while (--y3 != 0);
  WEIGHT32_WITHOUT_STRIDE;
}

template <bool mask_is_inverse>
void WeightMask32x32_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                            const void* LIBGAV1_RESTRICT prediction_1,
                            uint8_t* LIBGAV1_RESTRICT mask,
                            ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y5 = 6;
  do {
    WEIGHT32_AND_STRIDE;
    WEIGHT32_AND_STRIDE;
    WEIGHT32_AND_STRIDE;
    WEIGHT32_AND_STRIDE;
    WEIGHT32_AND_STRIDE;
  } while (--y5 != 0);
  WEIGHT32_AND_STRIDE;
  WEIGHT32_WITHOUT_STRIDE;
}

template <bool mask_is_inverse>
void WeightMask32x64_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                            const void* LIBGAV1_RESTRICT prediction_1,
                            uint8_t* LIBGAV1_RESTRICT mask,
                            ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y3 = 21;
  do {
    WEIGHT32_AND_STRIDE;
    WEIGHT32_AND_STRIDE;
    WEIGHT32_AND_STRIDE;
  } while (--y3 != 0);
  WEIGHT32_WITHOUT_STRIDE;
}

#define WEIGHT64_WITHOUT_STRIDE                                        \
  WeightMask16_SSE4_1<mask_is_inverse, true>(pred_0, pred_1, mask,     \
                                             mask_stride);             \
  WeightMask16_SSE4_1<mask_is_inverse, true>(pred_0 + 16, pred_1 + 16, \
                                             mask + 16, mask_stride);  \
  WeightMask16_SSE4_1<mask_is_inverse, true>(pred_0 + 32, pred_1 + 32, \
                                             mask + 32, mask_stride);  \
  WeightMask16_SSE4_1<mask_is_inverse, true>(pred_0 + 48, pred_1 + 48, \
                                             mask + 48, mask_stride)

#define WEIGHT64_AND_STRIDE \
  WEIGHT64_WITHOUT_STRIDE;  \
  pred_0 += 64;             \
  pred_1 += 64;             \
  mask += mask_stride

template <bool mask_is_inverse>
void WeightMask64x16_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                            const void* LIBGAV1_RESTRICT prediction_1,
                            uint8_t* LIBGAV1_RESTRICT mask,
                            ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y3 = 0;
  do {
    WEIGHT64_AND_STRIDE;
    WEIGHT64_AND_STRIDE;
    WEIGHT64_AND_STRIDE;
  } while (++y3 < 5);
  WEIGHT64_WITHOUT_STRIDE;
}

template <bool mask_is_inverse>
void WeightMask64x32_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                            const void* LIBGAV1_RESTRICT prediction_1,
                            uint8_t* LIBGAV1_RESTRICT mask,
                            ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y5 = 0;
  do {
    WEIGHT64_AND_STRIDE;
    WEIGHT64_AND_STRIDE;
    WEIGHT64_AND_STRIDE;
    WEIGHT64_AND_STRIDE;
    WEIGHT64_AND_STRIDE;
  } while (++y5 < 6);
  WEIGHT64_AND_STRIDE;
  WEIGHT64_WITHOUT_STRIDE;
}

template <bool mask_is_inverse>
void WeightMask64x64_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                            const void* LIBGAV1_RESTRICT prediction_1,
                            uint8_t* LIBGAV1_RESTRICT mask,
                            ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y3 = 0;
  do {
    WEIGHT64_AND_STRIDE;
    WEIGHT64_AND_STRIDE;
    WEIGHT64_AND_STRIDE;
  } while (++y3 < 21);
  WEIGHT64_WITHOUT_STRIDE;
}

template <bool mask_is_inverse>
void WeightMask64x128_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                             const void* LIBGAV1_RESTRICT prediction_1,
                             uint8_t* LIBGAV1_RESTRICT mask,
                             ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y3 = 0;
  do {
    WEIGHT64_AND_STRIDE;
    WEIGHT64_AND_STRIDE;
    WEIGHT64_AND_STRIDE;
  } while (++y3 < 42);
  WEIGHT64_AND_STRIDE;
  WEIGHT64_WITHOUT_STRIDE;
}

template <bool mask_is_inverse>
void WeightMask128x64_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                             const void* LIBGAV1_RESTRICT prediction_1,
                             uint8_t* LIBGAV1_RESTRICT mask,
                             ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y3 = 0;
  const ptrdiff_t adjusted_mask_stride = mask_stride - 64;
  do {
    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += 64;
    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += adjusted_mask_stride;

    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += 64;
    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += adjusted_mask_stride;

    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += 64;
    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += adjusted_mask_stride;
  } while (++y3 < 21);
  WEIGHT64_WITHOUT_STRIDE;
  pred_0 += 64;
  pred_1 += 64;
  mask += 64;
  WEIGHT64_WITHOUT_STRIDE;
}

template <bool mask_is_inverse>
void WeightMask128x128_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                              const void* LIBGAV1_RESTRICT prediction_1,
                              uint8_t* LIBGAV1_RESTRICT mask,
                              ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y3 = 0;
  const ptrdiff_t adjusted_mask_stride = mask_stride - 64;
  do {
    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += 64;
    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += adjusted_mask_stride;

    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += 64;
    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += adjusted_mask_stride;

    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += 64;
    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += adjusted_mask_stride;
  } while (++y3 < 42);
  WEIGHT64_WITHOUT_STRIDE;
  pred_0 += 64;
  pred_1 += 64;
  mask += 64;
  WEIGHT64_WITHOUT_STRIDE;
  pred_0 += 64;
  pred_1 += 64;
  mask += adjusted_mask_stride;

  WEIGHT64_WITHOUT_STRIDE;
  pred_0 += 64;
  pred_1 += 64;
  mask += 64;
  WEIGHT64_WITHOUT_STRIDE;
}

#define INIT_WEIGHT_MASK_8BPP(width, height, w_index, h_index) \
  dsp->weight_mask[w_index][h_index][0] =                      \
      WeightMask##width##x##height##_SSE4_1<0>;                \
  dsp->weight_mask[w_index][h_index][1] =                      \
      WeightMask##width##x##height##_SSE4_1<1>
void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
  INIT_WEIGHT_MASK_8BPP(8, 8, 0, 0);
  INIT_WEIGHT_MASK_8BPP(8, 16, 0, 1);
  INIT_WEIGHT_MASK_8BPP(8, 32, 0, 2);
  INIT_WEIGHT_MASK_8BPP(16, 8, 1, 0);
  INIT_WEIGHT_MASK_8BPP(16, 16, 1, 1);
  INIT_WEIGHT_MASK_8BPP(16, 32, 1, 2);
  INIT_WEIGHT_MASK_8BPP(16, 64, 1, 3);
  INIT_WEIGHT_MASK_8BPP(32, 8, 2, 0);
  INIT_WEIGHT_MASK_8BPP(32, 16, 2, 1);
  INIT_WEIGHT_MASK_8BPP(32, 32, 2, 2);
  INIT_WEIGHT_MASK_8BPP(32, 64, 2, 3);
  INIT_WEIGHT_MASK_8BPP(64, 16, 3, 1);
  INIT_WEIGHT_MASK_8BPP(64, 32, 3, 2);
  INIT_WEIGHT_MASK_8BPP(64, 64, 3, 3);
  INIT_WEIGHT_MASK_8BPP(64, 128, 3, 4);
  INIT_WEIGHT_MASK_8BPP(128, 64, 4, 3);
  INIT_WEIGHT_MASK_8BPP(128, 128, 4, 4);
}

}  // namespace
}  // namespace low_bitdepth

#if LIBGAV1_MAX_BITDEPTH >= 10
namespace high_bitdepth {
namespace {

constexpr int kRoundingBits10bpp = 6;
constexpr int kScaledDiffShift = 4;

template <bool mask_is_inverse, bool is_store_16>
inline void WeightMask16_10bpp_SSE4_1(
    const uint16_t* LIBGAV1_RESTRICT prediction_0,
    const uint16_t* LIBGAV1_RESTRICT prediction_1,
    uint8_t* LIBGAV1_RESTRICT mask, ptrdiff_t mask_stride) {
  const __m128i diff_offset = _mm_set1_epi8(38);
  const __m128i mask_ceiling = _mm_set1_epi8(64);
  const __m128i zero = _mm_setzero_si128();

  // Range of prediction: [3988, 61532].
  const __m128i pred_00 = LoadAligned16(prediction_0);
  const __m128i pred_10 = LoadAligned16(prediction_1);
  const __m128i pred_lo_00 = _mm_cvtepu16_epi32(pred_00);
  const __m128i pred_lo_10 = _mm_cvtepu16_epi32(pred_10);
  const __m128i diff_lo_0 = RightShiftWithRounding_U32(
      _mm_abs_epi32(_mm_sub_epi32(pred_lo_00, pred_lo_10)), kRoundingBits10bpp);

  const __m128i pred_hi_00 = _mm_unpackhi_epi16(pred_00, zero);
  const __m128i pred_hi_10 = _mm_unpackhi_epi16(pred_10, zero);
  const __m128i diff_hi_0 = RightShiftWithRounding_U32(
      _mm_abs_epi32(_mm_sub_epi32(pred_hi_00, pred_hi_10)), kRoundingBits10bpp);

  const __m128i diff_0 = _mm_packus_epi32(diff_lo_0, diff_hi_0);
  const __m128i scaled_diff_0 = _mm_srli_epi16(diff_0, kScaledDiffShift);

  const __m128i pred_01 = LoadAligned16(prediction_0 + 8);
  const __m128i pred_11 = LoadAligned16(prediction_1 + 8);
  const __m128i pred_lo_01 = _mm_cvtepu16_epi32(pred_01);
  const __m128i pred_lo_11 = _mm_cvtepu16_epi32(pred_11);
  const __m128i diff_lo_1 = RightShiftWithRounding_U32(
      _mm_abs_epi32(_mm_sub_epi32(pred_lo_01, pred_lo_11)), kRoundingBits10bpp);

  const __m128i pred_hi_01 = _mm_unpackhi_epi16(pred_01, zero);
  const __m128i pred_hi_11 = _mm_unpackhi_epi16(pred_11, zero);
  const __m128i diff_hi_1 = RightShiftWithRounding_U32(
      _mm_abs_epi32(_mm_sub_epi32(pred_hi_01, pred_hi_11)), kRoundingBits10bpp);

  const __m128i diff_1 = _mm_packus_epi32(diff_lo_1, diff_hi_1);
  const __m128i scaled_diff_1 = _mm_srli_epi16(diff_1, kScaledDiffShift);

  const __m128i adjusted_diff = _mm_adds_epu8(
      _mm_packus_epi16(scaled_diff_0, scaled_diff_1), diff_offset);
  const __m128i mask_value = _mm_min_epi8(adjusted_diff, mask_ceiling);

  if (mask_is_inverse) {
    const __m128i inverted_mask_value = _mm_sub_epi8(mask_ceiling, mask_value);
    if (is_store_16) {
      StoreAligned16(mask, inverted_mask_value);
    } else {
      StoreLo8(mask, inverted_mask_value);
      StoreHi8(mask + mask_stride, inverted_mask_value);
    }
  } else {
    if (is_store_16) {
      StoreAligned16(mask, mask_value);
    } else {
      StoreLo8(mask, mask_value);
      StoreHi8(mask + mask_stride, mask_value);
    }
  }
}

#define WEIGHT8_PAIR_WITHOUT_STRIDE_10BPP                                 \
  WeightMask16_10bpp_SSE4_1<mask_is_inverse, false>(pred_0, pred_1, mask, \
                                                    mask_stride)

#define WEIGHT8_PAIR_AND_STRIDE_10BPP \
  WEIGHT8_PAIR_WITHOUT_STRIDE_10BPP;  \
  pred_0 += 8 << 1;                   \
  pred_1 += 8 << 1;                   \
  mask += mask_stride << 1

template <bool mask_is_inverse>
void WeightMask8x8_10bpp_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                                const void* LIBGAV1_RESTRICT prediction_1,
                                uint8_t* LIBGAV1_RESTRICT mask,
                                ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const uint16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const uint16_t*>(prediction_1);

  WEIGHT8_PAIR_AND_STRIDE_10BPP;
  WEIGHT8_PAIR_AND_STRIDE_10BPP;
  WEIGHT8_PAIR_AND_STRIDE_10BPP;
  WEIGHT8_PAIR_WITHOUT_STRIDE_10BPP;
}

template <bool mask_is_inverse>
void WeightMask8x16_10bpp_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                                 const void* LIBGAV1_RESTRICT prediction_1,
                                 uint8_t* LIBGAV1_RESTRICT mask,
                                 ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const uint16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const uint16_t*>(prediction_1);
  int y3 = 3;
  do {
    WEIGHT8_PAIR_AND_STRIDE_10BPP;
    WEIGHT8_PAIR_AND_STRIDE_10BPP;
  } while (--y3 != 0);
  WEIGHT8_PAIR_AND_STRIDE_10BPP;
  WEIGHT8_PAIR_WITHOUT_STRIDE_10BPP;
}

template <bool mask_is_inverse>
void WeightMask8x32_10bpp_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                                 const void* LIBGAV1_RESTRICT prediction_1,
                                 uint8_t* LIBGAV1_RESTRICT mask,
                                 ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const uint16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const uint16_t*>(prediction_1);
  int y5 = 5;
  do {
    WEIGHT8_PAIR_AND_STRIDE_10BPP;
    WEIGHT8_PAIR_AND_STRIDE_10BPP;
    WEIGHT8_PAIR_AND_STRIDE_10BPP;
  } while (--y5 != 0);
  WEIGHT8_PAIR_WITHOUT_STRIDE_10BPP;
}

#define WEIGHT16_WITHOUT_STRIDE_10BPP                                    \
  WeightMask16_10bpp_SSE4_1<mask_is_inverse, true>(pred_0, pred_1, mask, \
                                                   mask_stride)

#define WEIGHT16_AND_STRIDE_10BPP \
  WEIGHT16_WITHOUT_STRIDE_10BPP;  \
  pred_0 += 16;                   \
  pred_1 += 16;                   \
  mask += mask_stride

template <bool mask_is_inverse>
void WeightMask16x8_10bpp_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                                 const void* LIBGAV1_RESTRICT prediction_1,
                                 uint8_t* LIBGAV1_RESTRICT mask,
                                 ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const uint16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const uint16_t*>(prediction_1);
  int y = 7;
  do {
    WEIGHT16_AND_STRIDE_10BPP;
  } while (--y != 0);
  WEIGHT16_WITHOUT_STRIDE_10BPP;
}

template <bool mask_is_inverse>
void WeightMask16x16_10bpp_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                                  const void* LIBGAV1_RESTRICT prediction_1,
                                  uint8_t* LIBGAV1_RESTRICT mask,
                                  ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const uint16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const uint16_t*>(prediction_1);
  int y3 = 5;
  do {
    WEIGHT16_AND_STRIDE_10BPP;
    WEIGHT16_AND_STRIDE_10BPP;
    WEIGHT16_AND_STRIDE_10BPP;
  } while (--y3 != 0);
  WEIGHT16_WITHOUT_STRIDE_10BPP;
}

template <bool mask_is_inverse>
void WeightMask16x32_10bpp_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                                  const void* LIBGAV1_RESTRICT prediction_1,
                                  uint8_t* LIBGAV1_RESTRICT mask,
                                  ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const uint16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const uint16_t*>(prediction_1);
  int y5 = 6;
  do {
    WEIGHT16_AND_STRIDE_10BPP;
    WEIGHT16_AND_STRIDE_10BPP;
    WEIGHT16_AND_STRIDE_10BPP;
    WEIGHT16_AND_STRIDE_10BPP;
    WEIGHT16_AND_STRIDE_10BPP;
  } while (--y5 != 0);
  WEIGHT16_AND_STRIDE_10BPP;
  WEIGHT16_WITHOUT_STRIDE_10BPP;
}

template <bool mask_is_inverse>
void WeightMask16x64_10bpp_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                                  const void* LIBGAV1_RESTRICT prediction_1,
                                  uint8_t* LIBGAV1_RESTRICT mask,
                                  ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const uint16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const uint16_t*>(prediction_1);
  int y3 = 21;
  do {
    WEIGHT16_AND_STRIDE_10BPP;
    WEIGHT16_AND_STRIDE_10BPP;
    WEIGHT16_AND_STRIDE_10BPP;
  } while (--y3 != 0);
  WEIGHT16_WITHOUT_STRIDE_10BPP;
}

#define WEIGHT32_WITHOUT_STRIDE_10BPP                                        \
  WeightMask16_10bpp_SSE4_1<mask_is_inverse, true>(pred_0, pred_1, mask,     \
                                                   mask_stride);             \
  WeightMask16_10bpp_SSE4_1<mask_is_inverse, true>(pred_0 + 16, pred_1 + 16, \
                                                   mask + 16, mask_stride)

#define WEIGHT32_AND_STRIDE_10BPP \
  WEIGHT32_WITHOUT_STRIDE_10BPP;  \
  pred_0 += 32;                   \
  pred_1 += 32;                   \
  mask += mask_stride

template <bool mask_is_inverse>
void WeightMask32x8_10bpp_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                                 const void* LIBGAV1_RESTRICT prediction_1,
                                 uint8_t* LIBGAV1_RESTRICT mask,
                                 ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const uint16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const uint16_t*>(prediction_1);
  WEIGHT32_AND_STRIDE_10BPP;
  WEIGHT32_AND_STRIDE_10BPP;
  WEIGHT32_AND_STRIDE_10BPP;
  WEIGHT32_AND_STRIDE_10BPP;
  WEIGHT32_AND_STRIDE_10BPP;
  WEIGHT32_AND_STRIDE_10BPP;
  WEIGHT32_AND_STRIDE_10BPP;
  WEIGHT32_WITHOUT_STRIDE_10BPP;
}

template <bool mask_is_inverse>
void WeightMask32x16_10bpp_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                                  const void* LIBGAV1_RESTRICT prediction_1,
                                  uint8_t* LIBGAV1_RESTRICT mask,
                                  ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const uint16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const uint16_t*>(prediction_1);
  int y3 = 5;
  do {
    WEIGHT32_AND_STRIDE_10BPP;
    WEIGHT32_AND_STRIDE_10BPP;
    WEIGHT32_AND_STRIDE_10BPP;
  } while (--y3 != 0);
  WEIGHT32_WITHOUT_STRIDE_10BPP;
}

template <bool mask_is_inverse>
void WeightMask32x32_10bpp_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                                  const void* LIBGAV1_RESTRICT prediction_1,
                                  uint8_t* LIBGAV1_RESTRICT mask,
                                  ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const uint16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const uint16_t*>(prediction_1);
  int y5 = 6;
  do {
    WEIGHT32_AND_STRIDE_10BPP;
    WEIGHT32_AND_STRIDE_10BPP;
    WEIGHT32_AND_STRIDE_10BPP;
    WEIGHT32_AND_STRIDE_10BPP;
    WEIGHT32_AND_STRIDE_10BPP;
  } while (--y5 != 0);
  WEIGHT32_AND_STRIDE_10BPP;
  WEIGHT32_WITHOUT_STRIDE_10BPP;
}

template <bool mask_is_inverse>
void WeightMask32x64_10bpp_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                                  const void* LIBGAV1_RESTRICT prediction_1,
                                  uint8_t* LIBGAV1_RESTRICT mask,
                                  ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const uint16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const uint16_t*>(prediction_1);
  int y3 = 21;
  do {
    WEIGHT32_AND_STRIDE_10BPP;
    WEIGHT32_AND_STRIDE_10BPP;
    WEIGHT32_AND_STRIDE_10BPP;
  } while (--y3 != 0);
  WEIGHT32_WITHOUT_STRIDE_10BPP;
}

#define WEIGHT64_WITHOUT_STRIDE_10BPP                                        \
  WeightMask16_10bpp_SSE4_1<mask_is_inverse, true>(pred_0, pred_1, mask,     \
                                                   mask_stride);             \
  WeightMask16_10bpp_SSE4_1<mask_is_inverse, true>(pred_0 + 16, pred_1 + 16, \
                                                   mask + 16, mask_stride);  \
  WeightMask16_10bpp_SSE4_1<mask_is_inverse, true>(pred_0 + 32, pred_1 + 32, \
                                                   mask + 32, mask_stride);  \
  WeightMask16_10bpp_SSE4_1<mask_is_inverse, true>(pred_0 + 48, pred_1 + 48, \
                                                   mask + 48, mask_stride)

#define WEIGHT64_AND_STRIDE_10BPP \
  WEIGHT64_WITHOUT_STRIDE_10BPP;  \
  pred_0 += 64;                   \
  pred_1 += 64;                   \
  mask += mask_stride

template <bool mask_is_inverse>
void WeightMask64x16_10bpp_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                                  const void* LIBGAV1_RESTRICT prediction_1,
                                  uint8_t* LIBGAV1_RESTRICT mask,
                                  ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const uint16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const uint16_t*>(prediction_1);
  int y3 = 5;
  do {
    WEIGHT64_AND_STRIDE_10BPP;
    WEIGHT64_AND_STRIDE_10BPP;
    WEIGHT64_AND_STRIDE_10BPP;
  } while (--y3 != 0);
  WEIGHT64_WITHOUT_STRIDE_10BPP;
}

template <bool mask_is_inverse>
void WeightMask64x32_10bpp_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                                  const void* LIBGAV1_RESTRICT prediction_1,
                                  uint8_t* LIBGAV1_RESTRICT mask,
                                  ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const uint16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const uint16_t*>(prediction_1);
  int y5 = 6;
  do {
    WEIGHT64_AND_STRIDE_10BPP;
    WEIGHT64_AND_STRIDE_10BPP;
    WEIGHT64_AND_STRIDE_10BPP;
    WEIGHT64_AND_STRIDE_10BPP;
    WEIGHT64_AND_STRIDE_10BPP;
  } while (--y5 != 0);
  WEIGHT64_AND_STRIDE_10BPP;
  WEIGHT64_WITHOUT_STRIDE_10BPP;
}

template <bool mask_is_inverse>
void WeightMask64x64_10bpp_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                                  const void* LIBGAV1_RESTRICT prediction_1,
                                  uint8_t* LIBGAV1_RESTRICT mask,
                                  ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const uint16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const uint16_t*>(prediction_1);
  int y3 = 21;
  do {
    WEIGHT64_AND_STRIDE_10BPP;
    WEIGHT64_AND_STRIDE_10BPP;
    WEIGHT64_AND_STRIDE_10BPP;
  } while (--y3 != 0);
  WEIGHT64_WITHOUT_STRIDE_10BPP;
}

template <bool mask_is_inverse>
void WeightMask64x128_10bpp_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                                   const void* LIBGAV1_RESTRICT prediction_1,
                                   uint8_t* LIBGAV1_RESTRICT mask,
                                   ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const uint16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const uint16_t*>(prediction_1);
  int y3 = 42;
  do {
    WEIGHT64_AND_STRIDE_10BPP;
    WEIGHT64_AND_STRIDE_10BPP;
    WEIGHT64_AND_STRIDE_10BPP;
  } while (--y3 != 0);
  WEIGHT64_AND_STRIDE_10BPP;
  WEIGHT64_WITHOUT_STRIDE_10BPP;
}

template <bool mask_is_inverse>
void WeightMask128x64_10bpp_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                                   const void* LIBGAV1_RESTRICT prediction_1,
                                   uint8_t* LIBGAV1_RESTRICT mask,
                                   ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const uint16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const uint16_t*>(prediction_1);
  int y3 = 21;
  const ptrdiff_t adjusted_mask_stride = mask_stride - 64;
  do {
    WEIGHT64_WITHOUT_STRIDE_10BPP;
    pred_0 += 64;
    pred_1 += 64;
    mask += 64;
    WEIGHT64_WITHOUT_STRIDE_10BPP;
    pred_0 += 64;
    pred_1 += 64;
    mask += adjusted_mask_stride;

    WEIGHT64_WITHOUT_STRIDE_10BPP;
    pred_0 += 64;
    pred_1 += 64;
    mask += 64;
    WEIGHT64_WITHOUT_STRIDE_10BPP;
    pred_0 += 64;
    pred_1 += 64;
    mask += adjusted_mask_stride;

    WEIGHT64_WITHOUT_STRIDE_10BPP;
    pred_0 += 64;
    pred_1 += 64;
    mask += 64;
    WEIGHT64_WITHOUT_STRIDE_10BPP;
    pred_0 += 64;
    pred_1 += 64;
    mask += adjusted_mask_stride;
  } while (--y3 != 0);
  WEIGHT64_WITHOUT_STRIDE_10BPP;
  pred_0 += 64;
  pred_1 += 64;
  mask += 64;
  WEIGHT64_WITHOUT_STRIDE_10BPP;
}

template <bool mask_is_inverse>
void WeightMask128x128_10bpp_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                                    const void* LIBGAV1_RESTRICT prediction_1,
                                    uint8_t* LIBGAV1_RESTRICT mask,
                                    ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const uint16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const uint16_t*>(prediction_1);
  int y3 = 42;
  const ptrdiff_t adjusted_mask_stride = mask_stride - 64;
  do {
    WEIGHT64_WITHOUT_STRIDE_10BPP;
    pred_0 += 64;
    pred_1 += 64;
    mask += 64;
    WEIGHT64_WITHOUT_STRIDE_10BPP;
    pred_0 += 64;
    pred_1 += 64;
    mask += adjusted_mask_stride;

    WEIGHT64_WITHOUT_STRIDE_10BPP;
    pred_0 += 64;
    pred_1 += 64;
    mask += 64;
    WEIGHT64_WITHOUT_STRIDE_10BPP;
    pred_0 += 64;
    pred_1 += 64;
    mask += adjusted_mask_stride;

    WEIGHT64_WITHOUT_STRIDE_10BPP;
    pred_0 += 64;
    pred_1 += 64;
    mask += 64;
    WEIGHT64_WITHOUT_STRIDE_10BPP;
    pred_0 += 64;
    pred_1 += 64;
    mask += adjusted_mask_stride;
  } while (--y3 != 0);
  WEIGHT64_WITHOUT_STRIDE_10BPP;
  pred_0 += 64;
  pred_1 += 64;
  mask += 64;
  WEIGHT64_WITHOUT_STRIDE_10BPP;
  pred_0 += 64;
  pred_1 += 64;
  mask += adjusted_mask_stride;

  WEIGHT64_WITHOUT_STRIDE_10BPP;
  pred_0 += 64;
  pred_1 += 64;
  mask += 64;
  WEIGHT64_WITHOUT_STRIDE_10BPP;
}

#define INIT_WEIGHT_MASK_10BPP(width, height, w_index, h_index) \
  dsp->weight_mask[w_index][h_index][0] =                       \
      WeightMask##width##x##height##_10bpp_SSE4_1<0>;           \
  dsp->weight_mask[w_index][h_index][1] =                       \
      WeightMask##width##x##height##_10bpp_SSE4_1<1>
void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth10);
  assert(dsp != nullptr);
  INIT_WEIGHT_MASK_10BPP(8, 8, 0, 0);
  INIT_WEIGHT_MASK_10BPP(8, 16, 0, 1);
  INIT_WEIGHT_MASK_10BPP(8, 32, 0, 2);
  INIT_WEIGHT_MASK_10BPP(16, 8, 1, 0);
  INIT_WEIGHT_MASK_10BPP(16, 16, 1, 1);
  INIT_WEIGHT_MASK_10BPP(16, 32, 1, 2);
  INIT_WEIGHT_MASK_10BPP(16, 64, 1, 3);
  INIT_WEIGHT_MASK_10BPP(32, 8, 2, 0);
  INIT_WEIGHT_MASK_10BPP(32, 16, 2, 1);
  INIT_WEIGHT_MASK_10BPP(32, 32, 2, 2);
  INIT_WEIGHT_MASK_10BPP(32, 64, 2, 3);
  INIT_WEIGHT_MASK_10BPP(64, 16, 3, 1);
  INIT_WEIGHT_MASK_10BPP(64, 32, 3, 2);
  INIT_WEIGHT_MASK_10BPP(64, 64, 3, 3);
  INIT_WEIGHT_MASK_10BPP(64, 128, 3, 4);
  INIT_WEIGHT_MASK_10BPP(128, 64, 4, 3);
  INIT_WEIGHT_MASK_10BPP(128, 128, 4, 4);
}

}  // namespace
}  // namespace high_bitdepth
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

void WeightMaskInit_SSE4_1() {
  low_bitdepth::Init8bpp();
#if LIBGAV1_MAX_BITDEPTH >= 10
  high_bitdepth::Init10bpp();
#endif
}

}  // namespace dsp
}  // namespace libgav1

#else   // !LIBGAV1_TARGETING_SSE4_1

namespace libgav1 {
namespace dsp {

void WeightMaskInit_SSE4_1() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_TARGETING_SSE4_1
