// Copyright 2019 The libgav1 Authors
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

#include "src/dsp/distance_weighted_blend.h"
#include "src/utils/cpu.h"

#if LIBGAV1_TARGETING_SSE4_1

#include <xmmintrin.h>

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

constexpr int kInterPostRoundBit = 4;
constexpr int kInterPostRhsAdjust = 1 << (16 - kInterPostRoundBit - 1);

inline __m128i ComputeWeightedAverage8(const __m128i& pred0,
                                       const __m128i& pred1,
                                       const __m128i& weight) {
  // Given: p0,p1 in range [-5132,9212] and w0 = 16 - w1, w1 = 16 - w0
  // Output: (p0 * w0 + p1 * w1 + 128(=rounding bit)) >>
  //    8(=kInterPostRoundBit + 4)
  // The formula is manipulated to avoid lengthening to 32 bits.
  // p0 * w0 + p1 * w1 = p0 * w0 + (16 - w0) * p1
  // = (p0 - p1) * w0 + 16 * p1
  // Maximum value of p0 - p1 is 9212 + 5132 = 0x3808.
  const __m128i diff = _mm_slli_epi16(_mm_sub_epi16(pred0, pred1), 1);
  // (((p0 - p1) * (w0 << 12) >> 16) + ((16 * p1) >> 4)
  const __m128i weighted_diff = _mm_mulhi_epi16(diff, weight);
  // ((p0 - p1) * w0 >> 4) + p1
  const __m128i upscaled_average = _mm_add_epi16(weighted_diff, pred1);
  // (x << 11) >> 15 == x >> 4
  const __m128i right_shift_prep = _mm_set1_epi16(kInterPostRhsAdjust);
  // (((p0 - p1) * w0 >> 4) + p1 + (128 >> 4)) >> 4
  return _mm_mulhrs_epi16(upscaled_average, right_shift_prep);
}

template <int height>
inline void DistanceWeightedBlend4xH_SSE4_1(
    const int16_t* LIBGAV1_RESTRICT pred_0,
    const int16_t* LIBGAV1_RESTRICT pred_1, const uint8_t weight,
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t dest_stride) {
  auto* dst = static_cast<uint8_t*>(dest);
  // Upscale the weight for mulhi.
  const __m128i weights = _mm_set1_epi16(weight << 11);

  for (int y = 0; y < height; y += 4) {
    const __m128i src_00 = LoadAligned16(pred_0);
    const __m128i src_10 = LoadAligned16(pred_1);
    pred_0 += 8;
    pred_1 += 8;
    const __m128i res0 = ComputeWeightedAverage8(src_00, src_10, weights);

    const __m128i src_01 = LoadAligned16(pred_0);
    const __m128i src_11 = LoadAligned16(pred_1);
    pred_0 += 8;
    pred_1 += 8;
    const __m128i res1 = ComputeWeightedAverage8(src_01, src_11, weights);

    const __m128i result_pixels = _mm_packus_epi16(res0, res1);
    Store4(dst, result_pixels);
    dst += dest_stride;
    const int result_1 = _mm_extract_epi32(result_pixels, 1);
    memcpy(dst, &result_1, sizeof(result_1));
    dst += dest_stride;
    const int result_2 = _mm_extract_epi32(result_pixels, 2);
    memcpy(dst, &result_2, sizeof(result_2));
    dst += dest_stride;
    const int result_3 = _mm_extract_epi32(result_pixels, 3);
    memcpy(dst, &result_3, sizeof(result_3));
    dst += dest_stride;
  }
}

template <int height>
inline void DistanceWeightedBlend8xH_SSE4_1(
    const int16_t* LIBGAV1_RESTRICT pred_0,
    const int16_t* LIBGAV1_RESTRICT pred_1, const uint8_t weight,
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t dest_stride) {
  auto* dst = static_cast<uint8_t*>(dest);
  // Upscale the weight for mulhi.
  const __m128i weights = _mm_set1_epi16(weight << 11);

  for (int y = 0; y < height; y += 2) {
    const __m128i src_00 = LoadAligned16(pred_0);
    const __m128i src_10 = LoadAligned16(pred_1);
    pred_0 += 8;
    pred_1 += 8;
    const __m128i res0 = ComputeWeightedAverage8(src_00, src_10, weights);

    const __m128i src_01 = LoadAligned16(pred_0);
    const __m128i src_11 = LoadAligned16(pred_1);
    pred_0 += 8;
    pred_1 += 8;
    const __m128i res1 = ComputeWeightedAverage8(src_01, src_11, weights);

    const __m128i result_pixels = _mm_packus_epi16(res0, res1);
    StoreLo8(dst, result_pixels);
    dst += dest_stride;
    StoreHi8(dst, result_pixels);
    dst += dest_stride;
  }
}

inline void DistanceWeightedBlendLarge_SSE4_1(
    const int16_t* LIBGAV1_RESTRICT pred_0,
    const int16_t* LIBGAV1_RESTRICT pred_1, const uint8_t weight,
    const int width, const int height, void* LIBGAV1_RESTRICT const dest,
    const ptrdiff_t dest_stride) {
  auto* dst = static_cast<uint8_t*>(dest);
  // Upscale the weight for mulhi.
  const __m128i weights = _mm_set1_epi16(weight << 11);

  int y = height;
  do {
    int x = 0;
    do {
      const __m128i src_0_lo = LoadAligned16(pred_0 + x);
      const __m128i src_1_lo = LoadAligned16(pred_1 + x);
      const __m128i res_lo =
          ComputeWeightedAverage8(src_0_lo, src_1_lo, weights);

      const __m128i src_0_hi = LoadAligned16(pred_0 + x + 8);
      const __m128i src_1_hi = LoadAligned16(pred_1 + x + 8);
      const __m128i res_hi =
          ComputeWeightedAverage8(src_0_hi, src_1_hi, weights);

      StoreUnaligned16(dst + x, _mm_packus_epi16(res_lo, res_hi));
      x += 16;
    } while (x < width);
    dst += dest_stride;
    pred_0 += width;
    pred_1 += width;
  } while (--y != 0);
}

void DistanceWeightedBlend_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                                  const void* LIBGAV1_RESTRICT prediction_1,
                                  const uint8_t weight_0,
                                  const uint8_t /*weight_1*/, const int width,
                                  const int height,
                                  void* LIBGAV1_RESTRICT const dest,
                                  const ptrdiff_t dest_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  const uint8_t weight = weight_0;
  if (width == 4) {
    if (height == 4) {
      DistanceWeightedBlend4xH_SSE4_1<4>(pred_0, pred_1, weight, dest,
                                         dest_stride);
    } else if (height == 8) {
      DistanceWeightedBlend4xH_SSE4_1<8>(pred_0, pred_1, weight, dest,
                                         dest_stride);
    } else {
      assert(height == 16);
      DistanceWeightedBlend4xH_SSE4_1<16>(pred_0, pred_1, weight, dest,
                                          dest_stride);
    }
    return;
  }

  if (width == 8) {
    switch (height) {
      case 4:
        DistanceWeightedBlend8xH_SSE4_1<4>(pred_0, pred_1, weight, dest,
                                           dest_stride);
        return;
      case 8:
        DistanceWeightedBlend8xH_SSE4_1<8>(pred_0, pred_1, weight, dest,
                                           dest_stride);
        return;
      case 16:
        DistanceWeightedBlend8xH_SSE4_1<16>(pred_0, pred_1, weight, dest,
                                            dest_stride);
        return;
      default:
        assert(height == 32);
        DistanceWeightedBlend8xH_SSE4_1<32>(pred_0, pred_1, weight, dest,
                                            dest_stride);

        return;
    }
  }

  DistanceWeightedBlendLarge_SSE4_1(pred_0, pred_1, weight, width, height, dest,
                                    dest_stride);
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
#if DSP_ENABLED_8BPP_SSE4_1(DistanceWeightedBlend)
  dsp->distance_weighted_blend = DistanceWeightedBlend_SSE4_1;
#endif
}

}  // namespace
}  // namespace low_bitdepth

#if LIBGAV1_MAX_BITDEPTH >= 10
namespace high_bitdepth {
namespace {

constexpr int kMax10bppSample = (1 << 10) - 1;
constexpr int kInterPostRoundBit = 4;

inline __m128i ComputeWeightedAverage8(const __m128i& pred0,
                                       const __m128i& pred1,
                                       const __m128i& weight0,
                                       const __m128i& weight1) {
  // This offset is a combination of round_factor and round_offset
  // which are to be added and subtracted respectively.
  // Here kInterPostRoundBit + 4 is considering bitdepth=10.
  constexpr int offset =
      (1 << ((kInterPostRoundBit + 4) - 1)) - (kCompoundOffset << 4);
  const __m128i zero = _mm_setzero_si128();
  const __m128i bias = _mm_set1_epi32(offset);
  const __m128i clip_high = _mm_set1_epi16(kMax10bppSample);

  __m128i prediction0 = _mm_cvtepu16_epi32(pred0);
  __m128i mult0 = _mm_mullo_epi32(prediction0, weight0);
  __m128i prediction1 = _mm_cvtepu16_epi32(pred1);
  __m128i mult1 = _mm_mullo_epi32(prediction1, weight1);
  __m128i sum = _mm_add_epi32(mult0, mult1);
  sum = _mm_add_epi32(sum, bias);
  const __m128i result0 = _mm_srai_epi32(sum, kInterPostRoundBit + 4);

  prediction0 = _mm_unpackhi_epi16(pred0, zero);
  mult0 = _mm_mullo_epi32(prediction0, weight0);
  prediction1 = _mm_unpackhi_epi16(pred1, zero);
  mult1 = _mm_mullo_epi32(prediction1, weight1);
  sum = _mm_add_epi32(mult0, mult1);
  sum = _mm_add_epi32(sum, bias);
  const __m128i result1 = _mm_srai_epi32(sum, kInterPostRoundBit + 4);
  const __m128i pack = _mm_packus_epi32(result0, result1);

  return _mm_min_epi16(pack, clip_high);
}

template <int height>
inline void DistanceWeightedBlend4xH_SSE4_1(
    const uint16_t* LIBGAV1_RESTRICT pred_0,
    const uint16_t* LIBGAV1_RESTRICT pred_1, const uint8_t weight_0,
    const uint8_t weight_1, void* LIBGAV1_RESTRICT const dest,
    const ptrdiff_t dest_stride) {
  auto* dst = static_cast<uint16_t*>(dest);
  const __m128i weight0 = _mm_set1_epi32(weight_0);
  const __m128i weight1 = _mm_set1_epi32(weight_1);

  int y = height;
  do {
    const __m128i src_00 = LoadAligned16(pred_0);
    const __m128i src_10 = LoadAligned16(pred_1);
    pred_0 += 8;
    pred_1 += 8;
    const __m128i res0 =
        ComputeWeightedAverage8(src_00, src_10, weight0, weight1);

    const __m128i src_01 = LoadAligned16(pred_0);
    const __m128i src_11 = LoadAligned16(pred_1);
    pred_0 += 8;
    pred_1 += 8;
    const __m128i res1 =
        ComputeWeightedAverage8(src_01, src_11, weight0, weight1);

    StoreLo8(dst, res0);
    dst += dest_stride;
    StoreHi8(dst, res0);
    dst += dest_stride;
    StoreLo8(dst, res1);
    dst += dest_stride;
    StoreHi8(dst, res1);
    dst += dest_stride;
    y -= 4;
  } while (y != 0);
}

template <int height>
inline void DistanceWeightedBlend8xH_SSE4_1(
    const uint16_t* LIBGAV1_RESTRICT pred_0,
    const uint16_t* LIBGAV1_RESTRICT pred_1, const uint8_t weight_0,
    const uint8_t weight_1, void* LIBGAV1_RESTRICT const dest,
    const ptrdiff_t dest_stride) {
  auto* dst = static_cast<uint16_t*>(dest);
  const __m128i weight0 = _mm_set1_epi32(weight_0);
  const __m128i weight1 = _mm_set1_epi32(weight_1);

  int y = height;
  do {
    const __m128i src_00 = LoadAligned16(pred_0);
    const __m128i src_10 = LoadAligned16(pred_1);
    pred_0 += 8;
    pred_1 += 8;
    const __m128i res0 =
        ComputeWeightedAverage8(src_00, src_10, weight0, weight1);

    const __m128i src_01 = LoadAligned16(pred_0);
    const __m128i src_11 = LoadAligned16(pred_1);
    pred_0 += 8;
    pred_1 += 8;
    const __m128i res1 =
        ComputeWeightedAverage8(src_01, src_11, weight0, weight1);

    StoreUnaligned16(dst, res0);
    dst += dest_stride;
    StoreUnaligned16(dst, res1);
    dst += dest_stride;
    y -= 2;
  } while (y != 0);
}

inline void DistanceWeightedBlendLarge_SSE4_1(
    const uint16_t* LIBGAV1_RESTRICT pred_0,
    const uint16_t* LIBGAV1_RESTRICT pred_1, const uint8_t weight_0,
    const uint8_t weight_1, const int width, const int height,
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t dest_stride) {
  auto* dst = static_cast<uint16_t*>(dest);
  const __m128i weight0 = _mm_set1_epi32(weight_0);
  const __m128i weight1 = _mm_set1_epi32(weight_1);

  int y = height;
  do {
    int x = 0;
    do {
      const __m128i src_0_lo = LoadAligned16(pred_0 + x);
      const __m128i src_1_lo = LoadAligned16(pred_1 + x);
      const __m128i res_lo =
          ComputeWeightedAverage8(src_0_lo, src_1_lo, weight0, weight1);

      const __m128i src_0_hi = LoadAligned16(pred_0 + x + 8);
      const __m128i src_1_hi = LoadAligned16(pred_1 + x + 8);
      const __m128i res_hi =
          ComputeWeightedAverage8(src_0_hi, src_1_hi, weight0, weight1);

      StoreUnaligned16(dst + x, res_lo);
      x += 8;
      StoreUnaligned16(dst + x, res_hi);
      x += 8;
    } while (x < width);
    dst += dest_stride;
    pred_0 += width;
    pred_1 += width;
  } while (--y != 0);
}

void DistanceWeightedBlend_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                                  const void* LIBGAV1_RESTRICT prediction_1,
                                  const uint8_t weight_0,
                                  const uint8_t weight_1, const int width,
                                  const int height,
                                  void* LIBGAV1_RESTRICT const dest,
                                  const ptrdiff_t dest_stride) {
  const auto* pred_0 = static_cast<const uint16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const uint16_t*>(prediction_1);
  const ptrdiff_t dst_stride = dest_stride / sizeof(*pred_0);
  if (width == 4) {
    if (height == 4) {
      DistanceWeightedBlend4xH_SSE4_1<4>(pred_0, pred_1, weight_0, weight_1,
                                         dest, dst_stride);
    } else if (height == 8) {
      DistanceWeightedBlend4xH_SSE4_1<8>(pred_0, pred_1, weight_0, weight_1,
                                         dest, dst_stride);
    } else {
      assert(height == 16);
      DistanceWeightedBlend4xH_SSE4_1<16>(pred_0, pred_1, weight_0, weight_1,
                                          dest, dst_stride);
    }
    return;
  }

  if (width == 8) {
    switch (height) {
      case 4:
        DistanceWeightedBlend8xH_SSE4_1<4>(pred_0, pred_1, weight_0, weight_1,
                                           dest, dst_stride);
        return;
      case 8:
        DistanceWeightedBlend8xH_SSE4_1<8>(pred_0, pred_1, weight_0, weight_1,
                                           dest, dst_stride);
        return;
      case 16:
        DistanceWeightedBlend8xH_SSE4_1<16>(pred_0, pred_1, weight_0, weight_1,
                                            dest, dst_stride);
        return;
      default:
        assert(height == 32);
        DistanceWeightedBlend8xH_SSE4_1<32>(pred_0, pred_1, weight_0, weight_1,
                                            dest, dst_stride);

        return;
    }
  }

  DistanceWeightedBlendLarge_SSE4_1(pred_0, pred_1, weight_0, weight_1, width,
                                    height, dest, dst_stride);
}

void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth10);
  assert(dsp != nullptr);
#if DSP_ENABLED_10BPP_SSE4_1(DistanceWeightedBlend)
  dsp->distance_weighted_blend = DistanceWeightedBlend_SSE4_1;
#endif
}

}  // namespace
}  // namespace high_bitdepth
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

void DistanceWeightedBlendInit_SSE4_1() {
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

void DistanceWeightedBlendInit_SSE4_1() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_TARGETING_SSE4_1
