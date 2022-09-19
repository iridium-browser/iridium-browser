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

#include "src/dsp/obmc.h"
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
#include "src/utils/constants.h"

namespace libgav1 {
namespace dsp {
namespace low_bitdepth {
namespace {

#include "src/dsp/obmc.inc"

inline void OverlapBlendFromLeft2xH_SSE4_1(
    uint8_t* LIBGAV1_RESTRICT const prediction,
    const ptrdiff_t prediction_stride, const int height,
    const uint8_t* LIBGAV1_RESTRICT const obmc_prediction) {
  constexpr int obmc_prediction_stride = 2;
  uint8_t* pred = prediction;
  const uint8_t* obmc_pred = obmc_prediction;
  const __m128i mask_inverter = _mm_cvtsi32_si128(0x40404040);
  const __m128i mask_val = _mm_shufflelo_epi16(Load4(kObmcMask), 0);
  // 64 - mask
  const __m128i obmc_mask_val = _mm_sub_epi8(mask_inverter, mask_val);
  const __m128i masks = _mm_unpacklo_epi8(mask_val, obmc_mask_val);
  int y = height;
  do {
    const __m128i pred_val = Load2x2(pred, pred + prediction_stride);
    const __m128i obmc_pred_val = Load4(obmc_pred);

    const __m128i terms = _mm_unpacklo_epi8(pred_val, obmc_pred_val);
    const __m128i result =
        RightShiftWithRounding_U16(_mm_maddubs_epi16(terms, masks), 6);
    const __m128i packed_result = _mm_packus_epi16(result, result);
    Store2(pred, packed_result);
    pred += prediction_stride;
    const int16_t second_row_result = _mm_extract_epi16(packed_result, 1);
    memcpy(pred, &second_row_result, sizeof(second_row_result));
    pred += prediction_stride;
    obmc_pred += obmc_prediction_stride << 1;
    y -= 2;
  } while (y != 0);
}

inline void OverlapBlendFromLeft4xH_SSE4_1(
    uint8_t* LIBGAV1_RESTRICT const prediction,
    const ptrdiff_t prediction_stride, const int height,
    const uint8_t* LIBGAV1_RESTRICT const obmc_prediction) {
  constexpr int obmc_prediction_stride = 4;
  uint8_t* pred = prediction;
  const uint8_t* obmc_pred = obmc_prediction;
  const __m128i mask_inverter = _mm_cvtsi32_si128(0x40404040);
  const __m128i mask_val = Load4(kObmcMask + 2);
  // 64 - mask
  const __m128i obmc_mask_val = _mm_sub_epi8(mask_inverter, mask_val);
  // Duplicate first half of vector.
  const __m128i masks =
      _mm_shuffle_epi32(_mm_unpacklo_epi8(mask_val, obmc_mask_val), 0x44);
  int y = height;
  do {
    const __m128i pred_val0 = Load4(pred);
    pred += prediction_stride;

    // Place the second row of each source in the second four bytes.
    const __m128i pred_val =
        _mm_alignr_epi8(Load4(pred), _mm_slli_si128(pred_val0, 12), 12);
    const __m128i obmc_pred_val = LoadLo8(obmc_pred);
    const __m128i terms = _mm_unpacklo_epi8(pred_val, obmc_pred_val);
    const __m128i result =
        RightShiftWithRounding_U16(_mm_maddubs_epi16(terms, masks), 6);
    const __m128i packed_result = _mm_packus_epi16(result, result);
    Store4(pred - prediction_stride, packed_result);
    const int second_row_result = _mm_extract_epi32(packed_result, 1);
    memcpy(pred, &second_row_result, sizeof(second_row_result));
    pred += prediction_stride;
    obmc_pred += obmc_prediction_stride << 1;
    y -= 2;
  } while (y != 0);
}

inline void OverlapBlendFromLeft8xH_SSE4_1(
    uint8_t* LIBGAV1_RESTRICT const prediction,
    const ptrdiff_t prediction_stride, const int height,
    const uint8_t* LIBGAV1_RESTRICT const obmc_prediction) {
  constexpr int obmc_prediction_stride = 8;
  uint8_t* pred = prediction;
  const uint8_t* obmc_pred = obmc_prediction;
  const __m128i mask_inverter = _mm_set1_epi8(64);
  const __m128i mask_val = LoadLo8(kObmcMask + 6);
  // 64 - mask
  const __m128i obmc_mask_val = _mm_sub_epi8(mask_inverter, mask_val);
  const __m128i masks = _mm_unpacklo_epi8(mask_val, obmc_mask_val);
  int y = height;
  do {
    const __m128i pred_val = LoadHi8(LoadLo8(pred), pred + prediction_stride);
    const __m128i obmc_pred_val = LoadUnaligned16(obmc_pred);

    const __m128i terms_lo = _mm_unpacklo_epi8(pred_val, obmc_pred_val);
    const __m128i result_lo =
        RightShiftWithRounding_U16(_mm_maddubs_epi16(terms_lo, masks), 6);

    const __m128i terms_hi = _mm_unpackhi_epi8(pred_val, obmc_pred_val);
    const __m128i result_hi =
        RightShiftWithRounding_U16(_mm_maddubs_epi16(terms_hi, masks), 6);

    const __m128i result = _mm_packus_epi16(result_lo, result_hi);
    StoreLo8(pred, result);
    pred += prediction_stride;
    StoreHi8(pred, result);
    pred += prediction_stride;
    obmc_pred += obmc_prediction_stride << 1;
    y -= 2;
  } while (y != 0);
}

void OverlapBlendFromLeft_SSE4_1(
    void* LIBGAV1_RESTRICT const prediction, const ptrdiff_t prediction_stride,
    const int width, const int height,
    const void* LIBGAV1_RESTRICT const obmc_prediction,
    const ptrdiff_t obmc_prediction_stride) {
  auto* pred = static_cast<uint8_t*>(prediction);
  const auto* obmc_pred = static_cast<const uint8_t*>(obmc_prediction);
  assert(width >= 2);
  assert(height >= 4);

  if (width == 2) {
    OverlapBlendFromLeft2xH_SSE4_1(pred, prediction_stride, height, obmc_pred);
    return;
  }
  if (width == 4) {
    OverlapBlendFromLeft4xH_SSE4_1(pred, prediction_stride, height, obmc_pred);
    return;
  }
  if (width == 8) {
    OverlapBlendFromLeft8xH_SSE4_1(pred, prediction_stride, height, obmc_pred);
    return;
  }
  const __m128i mask_inverter = _mm_set1_epi8(64);
  const uint8_t* mask = kObmcMask + width - 2;
  int x = 0;
  do {
    pred = static_cast<uint8_t*>(prediction) + x;
    obmc_pred = static_cast<const uint8_t*>(obmc_prediction) + x;
    const __m128i mask_val = LoadUnaligned16(mask + x);
    // 64 - mask
    const __m128i obmc_mask_val = _mm_sub_epi8(mask_inverter, mask_val);
    const __m128i masks_lo = _mm_unpacklo_epi8(mask_val, obmc_mask_val);
    const __m128i masks_hi = _mm_unpackhi_epi8(mask_val, obmc_mask_val);

    int y = 0;
    do {
      const __m128i pred_val = LoadUnaligned16(pred);
      const __m128i obmc_pred_val = LoadUnaligned16(obmc_pred);
      const __m128i terms_lo = _mm_unpacklo_epi8(pred_val, obmc_pred_val);
      const __m128i result_lo =
          RightShiftWithRounding_U16(_mm_maddubs_epi16(terms_lo, masks_lo), 6);
      const __m128i terms_hi = _mm_unpackhi_epi8(pred_val, obmc_pred_val);
      const __m128i result_hi =
          RightShiftWithRounding_U16(_mm_maddubs_epi16(terms_hi, masks_hi), 6);
      StoreUnaligned16(pred, _mm_packus_epi16(result_lo, result_hi));

      pred += prediction_stride;
      obmc_pred += obmc_prediction_stride;
    } while (++y < height);
    x += 16;
  } while (x < width);
}

inline void OverlapBlendFromTop4xH_SSE4_1(
    uint8_t* LIBGAV1_RESTRICT const prediction,
    const ptrdiff_t prediction_stride, const int height,
    const uint8_t* LIBGAV1_RESTRICT const obmc_prediction) {
  constexpr int obmc_prediction_stride = 4;
  uint8_t* pred = prediction;
  const uint8_t* obmc_pred = obmc_prediction;
  const __m128i mask_inverter = _mm_set1_epi16(64);
  const __m128i mask_shuffler = _mm_set_epi32(0x01010101, 0x01010101, 0, 0);
  const __m128i mask_preinverter = _mm_set1_epi16(-256 | 1);

  const uint8_t* mask = kObmcMask + height - 2;
  const int compute_height = height - (height >> 2);
  int y = 0;
  do {
    // First mask in the first half, second mask in the second half.
    const __m128i mask_val = _mm_shuffle_epi8(
        _mm_cvtsi32_si128(*reinterpret_cast<const uint16_t*>(mask + y)),
        mask_shuffler);
    const __m128i masks =
        _mm_sub_epi8(mask_inverter, _mm_sign_epi8(mask_val, mask_preinverter));
    const __m128i pred_val0 = Load4(pred);

    const __m128i obmc_pred_val = LoadLo8(obmc_pred);
    pred += prediction_stride;
    const __m128i pred_val =
        _mm_alignr_epi8(Load4(pred), _mm_slli_si128(pred_val0, 12), 12);
    const __m128i terms = _mm_unpacklo_epi8(obmc_pred_val, pred_val);
    const __m128i result =
        RightShiftWithRounding_U16(_mm_maddubs_epi16(terms, masks), 6);

    const __m128i packed_result = _mm_packus_epi16(result, result);
    Store4(pred - prediction_stride, packed_result);
    Store4(pred, _mm_srli_si128(packed_result, 4));
    pred += prediction_stride;
    obmc_pred += obmc_prediction_stride << 1;
    y += 2;
  } while (y < compute_height);
}

inline void OverlapBlendFromTop8xH_SSE4_1(
    uint8_t* LIBGAV1_RESTRICT const prediction,
    const ptrdiff_t prediction_stride, const int height,
    const uint8_t* LIBGAV1_RESTRICT const obmc_prediction) {
  constexpr int obmc_prediction_stride = 8;
  uint8_t* pred = prediction;
  const uint8_t* obmc_pred = obmc_prediction;
  const uint8_t* mask = kObmcMask + height - 2;
  const __m128i mask_inverter = _mm_set1_epi8(64);
  const int compute_height = height - (height >> 2);
  int y = compute_height;
  do {
    const __m128i mask_val0 = _mm_set1_epi8(mask[compute_height - y]);
    // 64 - mask
    const __m128i obmc_mask_val0 = _mm_sub_epi8(mask_inverter, mask_val0);
    const __m128i masks0 = _mm_unpacklo_epi8(mask_val0, obmc_mask_val0);

    const __m128i pred_val = LoadHi8(LoadLo8(pred), pred + prediction_stride);
    const __m128i obmc_pred_val = LoadUnaligned16(obmc_pred);

    const __m128i terms_lo = _mm_unpacklo_epi8(pred_val, obmc_pred_val);
    const __m128i result_lo =
        RightShiftWithRounding_U16(_mm_maddubs_epi16(terms_lo, masks0), 6);

    --y;
    const __m128i mask_val1 = _mm_set1_epi8(mask[compute_height - y]);
    // 64 - mask
    const __m128i obmc_mask_val1 = _mm_sub_epi8(mask_inverter, mask_val1);
    const __m128i masks1 = _mm_unpacklo_epi8(mask_val1, obmc_mask_val1);

    const __m128i terms_hi = _mm_unpackhi_epi8(pred_val, obmc_pred_val);
    const __m128i result_hi =
        RightShiftWithRounding_U16(_mm_maddubs_epi16(terms_hi, masks1), 6);

    const __m128i result = _mm_packus_epi16(result_lo, result_hi);
    StoreLo8(pred, result);
    pred += prediction_stride;
    StoreHi8(pred, result);
    pred += prediction_stride;
    obmc_pred += obmc_prediction_stride << 1;
  } while (--y > 0);
}

void OverlapBlendFromTop_SSE4_1(
    void* LIBGAV1_RESTRICT const prediction, const ptrdiff_t prediction_stride,
    const int width, const int height,
    const void* LIBGAV1_RESTRICT const obmc_prediction,
    const ptrdiff_t obmc_prediction_stride) {
  auto* pred = static_cast<uint8_t*>(prediction);
  const auto* obmc_pred = static_cast<const uint8_t*>(obmc_prediction);
  assert(width >= 4);
  assert(height >= 2);

  if (width == 4) {
    OverlapBlendFromTop4xH_SSE4_1(pred, prediction_stride, height, obmc_pred);
    return;
  }
  if (width == 8) {
    OverlapBlendFromTop8xH_SSE4_1(pred, prediction_stride, height, obmc_pred);
    return;
  }

  // Stop when mask value becomes 64.
  const int compute_height = height - (height >> 2);
  const __m128i mask_inverter = _mm_set1_epi8(64);
  int y = 0;
  const uint8_t* mask = kObmcMask + height - 2;
  do {
    const __m128i mask_val = _mm_set1_epi8(mask[y]);
    // 64 - mask
    const __m128i obmc_mask_val = _mm_sub_epi8(mask_inverter, mask_val);
    const __m128i masks = _mm_unpacklo_epi8(mask_val, obmc_mask_val);
    int x = 0;
    do {
      const __m128i pred_val = LoadUnaligned16(pred + x);
      const __m128i obmc_pred_val = LoadUnaligned16(obmc_pred + x);
      const __m128i terms_lo = _mm_unpacklo_epi8(pred_val, obmc_pred_val);
      const __m128i result_lo =
          RightShiftWithRounding_U16(_mm_maddubs_epi16(terms_lo, masks), 6);
      const __m128i terms_hi = _mm_unpackhi_epi8(pred_val, obmc_pred_val);
      const __m128i result_hi =
          RightShiftWithRounding_U16(_mm_maddubs_epi16(terms_hi, masks), 6);
      StoreUnaligned16(pred + x, _mm_packus_epi16(result_lo, result_hi));
      x += 16;
    } while (x < width);
    pred += prediction_stride;
    obmc_pred += obmc_prediction_stride;
  } while (++y < compute_height);
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
#if DSP_ENABLED_8BPP_SSE4_1(ObmcVertical)
  dsp->obmc_blend[kObmcDirectionVertical] = OverlapBlendFromTop_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(ObmcHorizontal)
  dsp->obmc_blend[kObmcDirectionHorizontal] = OverlapBlendFromLeft_SSE4_1;
#endif
}

}  // namespace
}  // namespace low_bitdepth

#if LIBGAV1_MAX_BITDEPTH >= 10
namespace high_bitdepth {
namespace {

#include "src/dsp/obmc.inc"

constexpr int kRoundBitsObmcBlend = 6;

inline void OverlapBlendFromLeft2xH_SSE4_1(
    uint16_t* LIBGAV1_RESTRICT const prediction, const ptrdiff_t pred_stride,
    const int height, const uint16_t* LIBGAV1_RESTRICT const obmc_prediction) {
  constexpr int obmc_pred_stride = 2;
  uint16_t* pred = prediction;
  const uint16_t* obmc_pred = obmc_prediction;
  const ptrdiff_t pred_stride2 = pred_stride << 1;
  const ptrdiff_t obmc_pred_stride2 = obmc_pred_stride << 1;
  const __m128i mask_inverter = _mm_cvtsi32_si128(0x40404040);
  const __m128i mask_val = _mm_shufflelo_epi16(Load2(kObmcMask), 0x00);
  // 64 - mask.
  const __m128i obmc_mask_val = _mm_sub_epi8(mask_inverter, mask_val);
  const __m128i masks =
      _mm_cvtepi8_epi16(_mm_unpacklo_epi8(mask_val, obmc_mask_val));
  int y = height;
  do {
    const __m128i pred_val = Load4x2(pred, pred + pred_stride);
    const __m128i obmc_pred_val = LoadLo8(obmc_pred);
    const __m128i terms = _mm_unpacklo_epi16(pred_val, obmc_pred_val);
    const __m128i result = RightShiftWithRounding_U32(
        _mm_madd_epi16(terms, masks), kRoundBitsObmcBlend);
    const __m128i packed_result = _mm_packus_epi32(result, result);
    Store4(pred, packed_result);
    Store4(pred + pred_stride, _mm_srli_si128(packed_result, 4));
    pred += pred_stride2;
    obmc_pred += obmc_pred_stride2;
    y -= 2;
  } while (y != 0);
}

inline void OverlapBlendFromLeft4xH_SSE4_1(
    uint16_t* LIBGAV1_RESTRICT const prediction, const ptrdiff_t pred_stride,
    const int height, const uint16_t* LIBGAV1_RESTRICT const obmc_prediction) {
  constexpr int obmc_pred_stride = 4;
  uint16_t* pred = prediction;
  const uint16_t* obmc_pred = obmc_prediction;
  const ptrdiff_t pred_stride2 = pred_stride << 1;
  const ptrdiff_t obmc_pred_stride2 = obmc_pred_stride << 1;
  const __m128i mask_inverter = _mm_cvtsi32_si128(0x40404040);
  const __m128i mask_val = Load4(kObmcMask + 2);
  // 64 - mask.
  const __m128i obmc_mask_val = _mm_sub_epi8(mask_inverter, mask_val);
  const __m128i masks =
      _mm_cvtepi8_epi16(_mm_unpacklo_epi8(mask_val, obmc_mask_val));
  int y = height;
  do {
    const __m128i pred_val = LoadHi8(LoadLo8(pred), pred + pred_stride);
    const __m128i obmc_pred_val = LoadUnaligned16(obmc_pred);
    const __m128i terms_lo = _mm_unpacklo_epi16(pred_val, obmc_pred_val);
    const __m128i terms_hi = _mm_unpackhi_epi16(pred_val, obmc_pred_val);
    const __m128i result_lo = RightShiftWithRounding_U32(
        _mm_madd_epi16(terms_lo, masks), kRoundBitsObmcBlend);
    const __m128i result_hi = RightShiftWithRounding_U32(
        _mm_madd_epi16(terms_hi, masks), kRoundBitsObmcBlend);
    const __m128i packed_result = _mm_packus_epi32(result_lo, result_hi);
    StoreLo8(pred, packed_result);
    StoreHi8(pred + pred_stride, packed_result);
    pred += pred_stride2;
    obmc_pred += obmc_pred_stride2;
    y -= 2;
  } while (y != 0);
}

void OverlapBlendFromLeft10bpp_SSE4_1(
    void* LIBGAV1_RESTRICT const prediction, const ptrdiff_t prediction_stride,
    const int width, const int height,
    const void* LIBGAV1_RESTRICT const obmc_prediction,
    const ptrdiff_t obmc_prediction_stride) {
  auto* pred = static_cast<uint16_t*>(prediction);
  const auto* obmc_pred = static_cast<const uint16_t*>(obmc_prediction);
  const ptrdiff_t pred_stride = prediction_stride / sizeof(pred[0]);
  const ptrdiff_t obmc_pred_stride =
      obmc_prediction_stride / sizeof(obmc_pred[0]);
  assert(width >= 2);
  assert(height >= 4);

  if (width == 2) {
    OverlapBlendFromLeft2xH_SSE4_1(pred, pred_stride, height, obmc_pred);
    return;
  }
  if (width == 4) {
    OverlapBlendFromLeft4xH_SSE4_1(pred, pred_stride, height, obmc_pred);
    return;
  }
  const __m128i mask_inverter = _mm_set1_epi8(64);
  const uint8_t* mask = kObmcMask + width - 2;
  int x = 0;
  do {
    pred = static_cast<uint16_t*>(prediction) + x;
    obmc_pred = static_cast<const uint16_t*>(obmc_prediction) + x;
    const __m128i mask_val = LoadLo8(mask + x);
    // 64 - mask
    const __m128i obmc_mask_val = _mm_sub_epi8(mask_inverter, mask_val);
    const __m128i masks = _mm_unpacklo_epi8(mask_val, obmc_mask_val);
    const __m128i masks_lo = _mm_cvtepi8_epi16(masks);
    const __m128i masks_hi = _mm_cvtepi8_epi16(_mm_srli_si128(masks, 8));
    int y = height;
    do {
      const __m128i pred_val = LoadUnaligned16(pred);
      const __m128i obmc_pred_val = LoadUnaligned16(obmc_pred);
      const __m128i terms_lo = _mm_unpacklo_epi16(pred_val, obmc_pred_val);
      const __m128i terms_hi = _mm_unpackhi_epi16(pred_val, obmc_pred_val);
      const __m128i result_lo = RightShiftWithRounding_U32(
          _mm_madd_epi16(terms_lo, masks_lo), kRoundBitsObmcBlend);
      const __m128i result_hi = RightShiftWithRounding_U32(
          _mm_madd_epi16(terms_hi, masks_hi), kRoundBitsObmcBlend);
      StoreUnaligned16(pred, _mm_packus_epi32(result_lo, result_hi));

      pred += pred_stride;
      obmc_pred += obmc_pred_stride;
    } while (--y != 0);
    x += 8;
  } while (x < width);
}

inline void OverlapBlendFromTop4xH_SSE4_1(
    uint16_t* LIBGAV1_RESTRICT const prediction, const ptrdiff_t pred_stride,
    const int height, const uint16_t* LIBGAV1_RESTRICT const obmc_prediction) {
  constexpr int obmc_pred_stride = 4;
  uint16_t* pred = prediction;
  const uint16_t* obmc_pred = obmc_prediction;
  const __m128i mask_inverter = _mm_set1_epi16(64);
  const __m128i mask_shuffler = _mm_set_epi32(0x01010101, 0x01010101, 0, 0);
  const __m128i mask_preinverter = _mm_set1_epi16(-256 | 1);
  const uint8_t* mask = kObmcMask + height - 2;
  const int compute_height = height - (height >> 2);
  const ptrdiff_t pred_stride2 = pred_stride << 1;
  const ptrdiff_t obmc_pred_stride2 = obmc_pred_stride << 1;
  int y = 0;
  do {
    // First mask in the first half, second mask in the second half.
    const __m128i mask_val = _mm_shuffle_epi8(Load4(mask + y), mask_shuffler);
    const __m128i masks =
        _mm_sub_epi8(mask_inverter, _mm_sign_epi8(mask_val, mask_preinverter));
    const __m128i masks_lo = _mm_cvtepi8_epi16(masks);
    const __m128i masks_hi = _mm_cvtepi8_epi16(_mm_srli_si128(masks, 8));

    const __m128i pred_val = LoadHi8(LoadLo8(pred), pred + pred_stride);
    const __m128i obmc_pred_val = LoadUnaligned16(obmc_pred);
    const __m128i terms_lo = _mm_unpacklo_epi16(obmc_pred_val, pred_val);
    const __m128i terms_hi = _mm_unpackhi_epi16(obmc_pred_val, pred_val);
    const __m128i result_lo = RightShiftWithRounding_U32(
        _mm_madd_epi16(terms_lo, masks_lo), kRoundBitsObmcBlend);
    const __m128i result_hi = RightShiftWithRounding_U32(
        _mm_madd_epi16(terms_hi, masks_hi), kRoundBitsObmcBlend);
    const __m128i packed_result = _mm_packus_epi32(result_lo, result_hi);

    StoreLo8(pred, packed_result);
    StoreHi8(pred + pred_stride, packed_result);
    pred += pred_stride2;
    obmc_pred += obmc_pred_stride2;
    y += 2;
  } while (y < compute_height);
}

void OverlapBlendFromTop10bpp_SSE4_1(
    void* LIBGAV1_RESTRICT const prediction, const ptrdiff_t prediction_stride,
    const int width, const int height,
    const void* LIBGAV1_RESTRICT const obmc_prediction,
    const ptrdiff_t obmc_prediction_stride) {
  auto* pred = static_cast<uint16_t*>(prediction);
  const auto* obmc_pred = static_cast<const uint16_t*>(obmc_prediction);
  const ptrdiff_t pred_stride = prediction_stride / sizeof(pred[0]);
  const ptrdiff_t obmc_pred_stride =
      obmc_prediction_stride / sizeof(obmc_pred[0]);
  assert(width >= 4);
  assert(height >= 2);

  if (width == 4) {
    OverlapBlendFromTop4xH_SSE4_1(pred, pred_stride, height, obmc_pred);
    return;
  }

  const __m128i mask_inverter = _mm_set1_epi8(64);
  const int compute_height = height - (height >> 2);
  const uint8_t* mask = kObmcMask + height - 2;
  pred = static_cast<uint16_t*>(prediction);
  obmc_pred = static_cast<const uint16_t*>(obmc_prediction);
  int y = 0;
  do {
    const __m128i mask_val = _mm_set1_epi8(mask[y]);
    // 64 - mask
    const __m128i obmc_mask_val = _mm_sub_epi8(mask_inverter, mask_val);
    const __m128i masks = _mm_unpacklo_epi8(mask_val, obmc_mask_val);
    const __m128i masks_lo = _mm_cvtepi8_epi16(masks);
    const __m128i masks_hi = _mm_cvtepi8_epi16(_mm_srli_si128(masks, 8));
    int x = 0;
    do {
      const __m128i pred_val = LoadUnaligned16(pred + x);
      const __m128i obmc_pred_val = LoadUnaligned16(obmc_pred + x);
      const __m128i terms_lo = _mm_unpacklo_epi16(pred_val, obmc_pred_val);
      const __m128i terms_hi = _mm_unpackhi_epi16(pred_val, obmc_pred_val);
      const __m128i result_lo = RightShiftWithRounding_U32(
          _mm_madd_epi16(terms_lo, masks_lo), kRoundBitsObmcBlend);
      const __m128i result_hi = RightShiftWithRounding_U32(
          _mm_madd_epi16(terms_hi, masks_hi), kRoundBitsObmcBlend);
      StoreUnaligned16(pred + x, _mm_packus_epi32(result_lo, result_hi));
      x += 8;
    } while (x < width);
    pred += pred_stride;
    obmc_pred += obmc_pred_stride;
  } while (++y < compute_height);
}

void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth10);
  assert(dsp != nullptr);
#if DSP_ENABLED_10BPP_SSE4_1(ObmcVertical)
  dsp->obmc_blend[kObmcDirectionVertical] = OverlapBlendFromTop10bpp_SSE4_1;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(ObmcHorizontal)
  dsp->obmc_blend[kObmcDirectionHorizontal] = OverlapBlendFromLeft10bpp_SSE4_1;
#endif
}

}  // namespace
}  // namespace high_bitdepth
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

void ObmcInit_SSE4_1() {
  low_bitdepth::Init8bpp();
#if LIBGAV1_MAX_BITDEPTH >= 10
  high_bitdepth::Init10bpp();
#endif  // LIBGAV1_MAX_BITDEPTH >= 10
}

}  // namespace dsp
}  // namespace libgav1

#else   // !LIBGAV1_TARGETING_SSE4_1

namespace libgav1 {
namespace dsp {

void ObmcInit_SSE4_1() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_TARGETING_SSE4_1
