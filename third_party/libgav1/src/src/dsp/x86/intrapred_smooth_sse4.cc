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

#include "src/dsp/intrapred_smooth.h"
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

// Note these constants are duplicated from intrapred.cc to allow the compiler
// to have visibility of the values. This helps reduce loads and in the
// creation of the inverse weights.
constexpr uint8_t kSmoothWeights[] = {
#include "src/dsp/smooth_weights.inc"
};

template <int y_mask>
inline void WriteSmoothHorizontalSum4(void* LIBGAV1_RESTRICT const dest,
                                      const __m128i& left,
                                      const __m128i& weights,
                                      const __m128i& scaled_top_right,
                                      const __m128i& round) {
  const __m128i left_y = _mm_shuffle_epi32(left, y_mask);
  const __m128i weighted_left_y = _mm_mullo_epi16(left_y, weights);
  const __m128i pred_sum = _mm_add_epi32(scaled_top_right, weighted_left_y);
  // Equivalent to RightShiftWithRounding(pred[x][y], 8).
  const __m128i pred = _mm_srli_epi32(_mm_add_epi32(pred_sum, round), 8);
  const __m128i cvtepi32_epi8 = _mm_set1_epi32(0x0C080400);
  Store4(dest, _mm_shuffle_epi8(pred, cvtepi32_epi8));
}

// For SMOOTH_H, |pixels| is the repeated left value for the row. For SMOOTH_V,
// |pixels| is a segment of the top row or the whole top row, and |weights| is
// repeated.
inline __m128i SmoothDirectionalSum8(const __m128i& pixels,
                                     const __m128i& weights,
                                     const __m128i& scaled_corner) {
  const __m128i weighted_px = _mm_mullo_epi16(pixels, weights);
  return _mm_add_epi16(scaled_corner, weighted_px);
}

inline void WriteSmoothDirectionalSum8(uint8_t* LIBGAV1_RESTRICT dest,
                                       const __m128i& pixels,
                                       const __m128i& weights,
                                       const __m128i& scaled_corner,
                                       const __m128i& round) {
  const __m128i pred_sum =
      SmoothDirectionalSum8(pixels, weights, scaled_corner);
  // Equivalent to RightShiftWithRounding(pred[x][y], 8).
  const __m128i pred = _mm_srli_epi16(_mm_add_epi16(pred_sum, round), 8);
  StoreLo8(dest, _mm_packus_epi16(pred, pred));
}

// For Horizontal, pixels1 and pixels2 are the same repeated value. For
// Vertical, weights1 and weights2 are the same, and scaled_corner1 and
// scaled_corner2 are the same.
inline void WriteSmoothDirectionalSum16(
    uint8_t* LIBGAV1_RESTRICT dest, const __m128i& pixels1,
    const __m128i& pixels2, const __m128i& weights1, const __m128i& weights2,
    const __m128i& scaled_corner1, const __m128i& scaled_corner2,
    const __m128i& round) {
  const __m128i weighted_px1 = _mm_mullo_epi16(pixels1, weights1);
  const __m128i weighted_px2 = _mm_mullo_epi16(pixels2, weights2);
  const __m128i pred_sum1 = _mm_add_epi16(scaled_corner1, weighted_px1);
  const __m128i pred_sum2 = _mm_add_epi16(scaled_corner2, weighted_px2);
  // Equivalent to RightShiftWithRounding(pred[x][y], 8).
  const __m128i pred1 = _mm_srli_epi16(_mm_add_epi16(pred_sum1, round), 8);
  const __m128i pred2 = _mm_srli_epi16(_mm_add_epi16(pred_sum2, round), 8);
  StoreUnaligned16(dest, _mm_packus_epi16(pred1, pred2));
}

template <int y_mask>
inline void WriteSmoothPredSum4(uint8_t* LIBGAV1_RESTRICT const dest,
                                const __m128i& top, const __m128i& left,
                                const __m128i& weights_x,
                                const __m128i& weights_y,
                                const __m128i& scaled_bottom_left,
                                const __m128i& scaled_top_right,
                                const __m128i& round) {
  const __m128i left_y = _mm_shuffle_epi32(left, y_mask);
  const __m128i weighted_left_y = _mm_mullo_epi32(left_y, weights_x);
  const __m128i weight_y = _mm_shuffle_epi32(weights_y, y_mask);
  const __m128i weighted_top = _mm_mullo_epi32(weight_y, top);
  const __m128i scaled_bottom_left_y =
      _mm_shuffle_epi32(scaled_bottom_left, y_mask);
  const __m128i col_pred = _mm_add_epi32(scaled_bottom_left_y, weighted_left_y);
  const __m128i row_pred = _mm_add_epi32(scaled_top_right, weighted_top);
  const __m128i pred_sum = _mm_add_epi32(row_pred, col_pred);

  // Equivalent to RightShiftWithRounding(pred[x][y], 9).
  const __m128i pred = _mm_srli_epi32(_mm_add_epi32(pred_sum, round), 9);

  const __m128i cvtepi32_epi8 = _mm_set1_epi32(0x0C080400);
  Store4(dest, _mm_shuffle_epi8(pred, cvtepi32_epi8));
}

// pixels[0]: above and below_pred interleave vector
// pixels[1]: left vector
// pixels[2]: right_pred vector
inline void LoadSmoothPixels4(const uint8_t* LIBGAV1_RESTRICT above,
                              const uint8_t* LIBGAV1_RESTRICT left,
                              const int height, __m128i* pixels) {
  if (height == 4) {
    pixels[1] = Load4(left);
  } else if (height == 8) {
    pixels[1] = LoadLo8(left);
  } else {
    pixels[1] = LoadUnaligned16(left);
  }

  const __m128i bottom_left = _mm_set1_epi16(left[height - 1]);
  const __m128i top = _mm_cvtepu8_epi16(Load4(above));
  pixels[0] = _mm_unpacklo_epi16(top, bottom_left);
  pixels[2] = _mm_set1_epi16(above[3]);
}

// weight_h[0]: weight_h vector
// weight_h[1]: scale - weight_h vector
// weight_h[2]: same as [0], second half for height = 16 only
// weight_h[3]: same as [1], second half for height = 16 only
// weight_w[0]: weights_w and scale - weights_w interleave vector
inline void LoadSmoothWeights4(const uint8_t* LIBGAV1_RESTRICT weight_array,
                               const int height, __m128i* weight_h,
                               __m128i* weight_w) {
  const __m128i scale = _mm_set1_epi16(256);
  const __m128i x_weights = Load4(weight_array);
  weight_h[0] = _mm_cvtepu8_epi16(x_weights);
  weight_h[1] = _mm_sub_epi16(scale, weight_h[0]);
  weight_w[0] = _mm_unpacklo_epi16(weight_h[0], weight_h[1]);

  if (height == 8) {
    const __m128i y_weights = LoadLo8(weight_array + 4);
    weight_h[0] = _mm_cvtepu8_epi16(y_weights);
    weight_h[1] = _mm_sub_epi16(scale, weight_h[0]);
  } else if (height == 16) {
    const __m128i zero = _mm_setzero_si128();
    const __m128i y_weights = LoadUnaligned16(weight_array + 12);
    weight_h[0] = _mm_cvtepu8_epi16(y_weights);
    weight_h[1] = _mm_sub_epi16(scale, weight_h[0]);
    weight_h[2] = _mm_unpackhi_epi8(y_weights, zero);
    weight_h[3] = _mm_sub_epi16(scale, weight_h[2]);
  }
}

inline void WriteSmoothPred4x8(const __m128i* pixel, const __m128i* weights_y,
                               const __m128i* weight_x,
                               uint8_t* LIBGAV1_RESTRICT dst,
                               const ptrdiff_t stride,
                               const bool use_second_half) {
  const __m128i round = _mm_set1_epi32(256);
  const __m128i mask_increment = _mm_set1_epi16(0x0202);
  const __m128i cvtepi32_epi8 = _mm_set1_epi32(0x0C080400);
  const __m128i zero = _mm_setzero_si128();
  const __m128i left = use_second_half ? _mm_unpackhi_epi8(pixel[1], zero)
                                       : _mm_unpacklo_epi8(pixel[1], zero);
  __m128i y_select = _mm_set1_epi16(0x0100);

  for (int i = 0; i < 8; ++i) {
    const __m128i weight_y = _mm_shuffle_epi8(weights_y[0], y_select);
    const __m128i inverted_weight_y = _mm_shuffle_epi8(weights_y[1], y_select);
    const __m128i interleaved_weights =
        _mm_unpacklo_epi16(weight_y, inverted_weight_y);
    __m128i vertical_pred = _mm_madd_epi16(pixel[0], interleaved_weights);

    __m128i horizontal_vect = _mm_shuffle_epi8(left, y_select);
    horizontal_vect = _mm_unpacklo_epi16(horizontal_vect, pixel[2]);
    __m128i sum = _mm_madd_epi16(horizontal_vect, weight_x[0]);

    sum = _mm_add_epi32(vertical_pred, sum);
    sum = _mm_add_epi32(sum, round);
    sum = _mm_srai_epi32(sum, 9);

    sum = _mm_shuffle_epi8(sum, cvtepi32_epi8);
    Store4(dst, sum);
    dst += stride;

    y_select = _mm_add_epi16(y_select, mask_increment);
  }
}

// The interleaving approach has some overhead that causes it to underperform in
// the 4x4 case.
void Smooth4x4_SSE4_1(void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
                      const void* LIBGAV1_RESTRICT top_row,
                      const void* LIBGAV1_RESTRICT left_column) {
  const __m128i top = _mm_cvtepu8_epi32(Load4(top_row));
  const __m128i left = _mm_cvtepu8_epi32(Load4(left_column));
  const __m128i weights = _mm_cvtepu8_epi32(Load4(kSmoothWeights));
  const __m128i scale = _mm_set1_epi32(256);
  // Fourth short is top_row[3].
  const __m128i top_right = _mm_shuffle_epi32(top, 0xFF);
  // Fourth short is left_column[3].
  const __m128i bottom_left = _mm_shuffle_epi32(left, 0xFF);
  const __m128i inverted_weights = _mm_sub_epi32(scale, weights);
  const __m128i scaled_top_right = _mm_mullo_epi16(inverted_weights, top_right);
  const __m128i scaled_bottom_left =
      _mm_mullo_epi16(inverted_weights, bottom_left);
  auto* dst = static_cast<uint8_t*>(dest);
  // AV1 spec 7.11.2.6 (3) describes the sum:
  // smoothPred[y][x:x+3] = weighted_top + scaled_right + weighted_left[y] +
  // scaled_bottom[y] This could be a loop, but for the immediate value in the
  // shuffles.
  WriteSmoothPredSum4<0>(dst, top, left, weights, weights, scaled_bottom_left,
                         scaled_top_right, scale);
  dst += stride;
  WriteSmoothPredSum4<0x55>(dst, top, left, weights, weights,
                            scaled_bottom_left, scaled_top_right, scale);
  dst += stride;
  WriteSmoothPredSum4<0xAA>(dst, top, left, weights, weights,
                            scaled_bottom_left, scaled_top_right, scale);
  dst += stride;
  WriteSmoothPredSum4<0xFF>(dst, top, left, weights, weights,
                            scaled_bottom_left, scaled_top_right, scale);
}

void Smooth4x8_SSE4_1(void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
                      const void* LIBGAV1_RESTRICT top_row,
                      const void* LIBGAV1_RESTRICT left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  __m128i weights_x[1];
  __m128i weights_y[2];
  LoadSmoothWeights4(kSmoothWeights, 8, weights_y, weights_x);
  __m128i pixels[3];
  LoadSmoothPixels4(top_ptr, left_ptr, 8, pixels);
  auto* dst = static_cast<uint8_t*>(dest);
  WriteSmoothPred4x8(pixels, weights_y, weights_x, dst, stride, false);
}

void Smooth4x16_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                       const ptrdiff_t stride,
                       const void* LIBGAV1_RESTRICT top_row,
                       const void* LIBGAV1_RESTRICT left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  __m128i weights_x[1];
  __m128i weights_y[4];
  LoadSmoothWeights4(kSmoothWeights, 16, weights_y, weights_x);
  __m128i pixels[3];
  LoadSmoothPixels4(top_ptr, left_ptr, 16, pixels);
  auto* dst = static_cast<uint8_t*>(dest);
  WriteSmoothPred4x8(pixels, weights_y, weights_x, dst, stride, false);
  dst += stride << 3;
  WriteSmoothPred4x8(pixels, &weights_y[2], weights_x, dst, stride, true);
}

// pixels[0]: above and below_pred interleave vector, first half
// pixels[1]: above and below_pred interleave vector, second half
// pixels[2]: left vector
// pixels[3]: right_pred vector
// pixels[4]: above and below_pred interleave vector, first half
// pixels[5]: above and below_pred interleave vector, second half
// pixels[6]: left vector + 16
// pixels[7]: right_pred vector
inline void LoadSmoothPixels8(const uint8_t* LIBGAV1_RESTRICT above,
                              const uint8_t* LIBGAV1_RESTRICT left,
                              const int height, __m128i* pixels) {
  const __m128i bottom_left = _mm_set1_epi16(left[height - 1]);
  __m128i top_row = _mm_cvtepu8_epi16(LoadLo8(above));
  pixels[0] = _mm_unpacklo_epi16(top_row, bottom_left);
  pixels[1] = _mm_unpackhi_epi16(top_row, bottom_left);

  pixels[3] = _mm_set1_epi16(above[7]);

  if (height == 4) {
    pixels[2] = Load4(left);
  } else if (height == 8) {
    pixels[2] = LoadLo8(left);
  } else if (height == 16) {
    pixels[2] = LoadUnaligned16(left);
  } else {
    pixels[2] = LoadUnaligned16(left);
    pixels[4] = pixels[0];
    pixels[5] = pixels[1];
    pixels[6] = LoadUnaligned16(left + 16);
    pixels[7] = pixels[3];
  }
}

// weight_h[0]: weight_h vector
// weight_h[1]: scale - weight_h vector
// weight_h[2]: same as [0], offset 8
// weight_h[3]: same as [1], offset 8
// weight_h[4]: same as [0], offset 16
// weight_h[5]: same as [1], offset 16
// weight_h[6]: same as [0], offset 24
// weight_h[7]: same as [1], offset 24
// weight_w[0]: weights_w and scale - weights_w interleave vector, first half
// weight_w[1]: weights_w and scale - weights_w interleave vector, second half
inline void LoadSmoothWeights8(const uint8_t* LIBGAV1_RESTRICT weight_array,
                               const int height, __m128i* weight_w,
                               __m128i* weight_h) {
  const int offset = (height < 8) ? 0 : 4;
  __m128i loaded_weights = LoadUnaligned16(&weight_array[offset]);
  weight_h[0] = _mm_cvtepu8_epi16(loaded_weights);
  const __m128i inverter = _mm_set1_epi16(256);
  weight_h[1] = _mm_sub_epi16(inverter, weight_h[0]);

  if (height == 4) {
    loaded_weights = _mm_srli_si128(loaded_weights, 4);
    __m128i weights_x = _mm_cvtepu8_epi16(loaded_weights);
    __m128i inverted_weights_x = _mm_sub_epi16(inverter, weights_x);
    weight_w[0] = _mm_unpacklo_epi16(weights_x, inverted_weights_x);
    weight_w[1] = _mm_unpackhi_epi16(weights_x, inverted_weights_x);
  } else {
    weight_w[0] = _mm_unpacklo_epi16(weight_h[0], weight_h[1]);
    weight_w[1] = _mm_unpackhi_epi16(weight_h[0], weight_h[1]);
  }

  if (height == 16) {
    const __m128i zero = _mm_setzero_si128();
    loaded_weights = LoadUnaligned16(weight_array + 12);
    weight_h[0] = _mm_cvtepu8_epi16(loaded_weights);
    weight_h[1] = _mm_sub_epi16(inverter, weight_h[0]);
    weight_h[2] = _mm_unpackhi_epi8(loaded_weights, zero);
    weight_h[3] = _mm_sub_epi16(inverter, weight_h[2]);
  } else if (height == 32) {
    const __m128i zero = _mm_setzero_si128();
    const __m128i weight_lo = LoadUnaligned16(weight_array + 28);
    weight_h[0] = _mm_cvtepu8_epi16(weight_lo);
    weight_h[1] = _mm_sub_epi16(inverter, weight_h[0]);
    weight_h[2] = _mm_unpackhi_epi8(weight_lo, zero);
    weight_h[3] = _mm_sub_epi16(inverter, weight_h[2]);
    const __m128i weight_hi = LoadUnaligned16(weight_array + 44);
    weight_h[4] = _mm_cvtepu8_epi16(weight_hi);
    weight_h[5] = _mm_sub_epi16(inverter, weight_h[4]);
    weight_h[6] = _mm_unpackhi_epi8(weight_hi, zero);
    weight_h[7] = _mm_sub_epi16(inverter, weight_h[6]);
  }
}

inline void WriteSmoothPred8xH(const __m128i* pixels, const __m128i* weights_x,
                               const __m128i* weights_y, const int height,
                               uint8_t* LIBGAV1_RESTRICT dst,
                               const ptrdiff_t stride,
                               const bool use_second_half) {
  const __m128i round = _mm_set1_epi32(256);
  const __m128i mask_increment = _mm_set1_epi16(0x0202);
  const __m128i cvt_epu16_epi8 = _mm_set_epi32(0, 0, 0xe0c0a08, 0x6040200);

  const __m128i zero = _mm_setzero_si128();
  const __m128i left = use_second_half ? _mm_unpackhi_epi8(pixels[2], zero)
                                       : _mm_unpacklo_epi8(pixels[2], zero);
  __m128i y_select = _mm_set1_epi16(0x100);

  for (int i = 0; i < height; ++i) {
    const __m128i weight_y = _mm_shuffle_epi8(weights_y[0], y_select);
    const __m128i inverted_weight_y = _mm_shuffle_epi8(weights_y[1], y_select);
    const __m128i interleaved_weights =
        _mm_unpacklo_epi16(weight_y, inverted_weight_y);
    const __m128i vertical_sum0 =
        _mm_madd_epi16(pixels[0], interleaved_weights);
    const __m128i vertical_sum1 =
        _mm_madd_epi16(pixels[1], interleaved_weights);

    __m128i horizontal_pixels = _mm_shuffle_epi8(left, y_select);
    horizontal_pixels = _mm_unpacklo_epi16(horizontal_pixels, pixels[3]);
    const __m128i horizontal_sum0 =
        _mm_madd_epi16(horizontal_pixels, weights_x[0]);
    const __m128i horizontal_sum1 =
        _mm_madd_epi16(horizontal_pixels, weights_x[1]);

    __m128i sum0 = _mm_add_epi32(vertical_sum0, horizontal_sum0);
    sum0 = _mm_add_epi32(sum0, round);
    sum0 = _mm_srai_epi32(sum0, 9);

    __m128i sum1 = _mm_add_epi32(vertical_sum1, horizontal_sum1);
    sum1 = _mm_add_epi32(sum1, round);
    sum1 = _mm_srai_epi32(sum1, 9);

    sum0 = _mm_packus_epi16(sum0, sum1);
    sum0 = _mm_shuffle_epi8(sum0, cvt_epu16_epi8);
    StoreLo8(dst, sum0);
    dst += stride;

    y_select = _mm_add_epi16(y_select, mask_increment);
  }
}

void Smooth8x4_SSE4_1(void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
                      const void* LIBGAV1_RESTRICT top_row,
                      const void* LIBGAV1_RESTRICT left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  __m128i pixels[4];
  LoadSmoothPixels8(top_ptr, left_ptr, 4, pixels);

  __m128i weights_x[2], weights_y[2];
  LoadSmoothWeights8(kSmoothWeights, 4, weights_x, weights_y);

  auto* dst = static_cast<uint8_t*>(dest);
  WriteSmoothPred8xH(pixels, weights_x, weights_y, 4, dst, stride, false);
}

void Smooth8x8_SSE4_1(void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
                      const void* LIBGAV1_RESTRICT top_row,
                      const void* LIBGAV1_RESTRICT left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);

  __m128i pixels[4];
  LoadSmoothPixels8(top_ptr, left_ptr, 8, pixels);

  __m128i weights_x[2], weights_y[2];
  LoadSmoothWeights8(kSmoothWeights, 8, weights_x, weights_y);

  auto* dst = static_cast<uint8_t*>(dest);
  WriteSmoothPred8xH(pixels, weights_x, weights_y, 8, dst, stride, false);
}

void Smooth8x16_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                       const ptrdiff_t stride,
                       const void* LIBGAV1_RESTRICT top_row,
                       const void* LIBGAV1_RESTRICT left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  __m128i pixels[4];
  LoadSmoothPixels8(top_ptr, left_ptr, 16, pixels);

  __m128i weights_x[2], weights_y[4];
  LoadSmoothWeights8(kSmoothWeights, 16, weights_x, weights_y);

  auto* dst = static_cast<uint8_t*>(dest);
  WriteSmoothPred8xH(pixels, weights_x, weights_y, 8, dst, stride, false);
  dst += stride << 3;
  WriteSmoothPred8xH(pixels, weights_x, &weights_y[2], 8, dst, stride, true);
}

void Smooth8x32_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                       const ptrdiff_t stride,
                       const void* LIBGAV1_RESTRICT top_row,
                       const void* LIBGAV1_RESTRICT left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  __m128i pixels[8];
  LoadSmoothPixels8(top_ptr, left_ptr, 32, pixels);

  __m128i weights_x[2], weights_y[8];
  LoadSmoothWeights8(kSmoothWeights, 32, weights_x, weights_y);

  auto* dst = static_cast<uint8_t*>(dest);
  WriteSmoothPred8xH(pixels, weights_x, weights_y, 8, dst, stride, false);
  dst += stride << 3;
  WriteSmoothPred8xH(pixels, weights_x, &weights_y[2], 8, dst, stride, true);
  dst += stride << 3;
  WriteSmoothPred8xH(&pixels[4], weights_x, &weights_y[4], 8, dst, stride,
                     false);
  dst += stride << 3;
  WriteSmoothPred8xH(&pixels[4], weights_x, &weights_y[6], 8, dst, stride,
                     true);
}

template <int width, int height>
void SmoothWxH(void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
               const void* LIBGAV1_RESTRICT const top_row,
               const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const uint8_t* const sm_weights_h = kSmoothWeights + height - 4;
  const uint8_t* const sm_weights_w = kSmoothWeights + width - 4;
  const __m128i zero = _mm_setzero_si128();
  const __m128i scale_value = _mm_set1_epi16(256);
  const __m128i bottom_left = _mm_cvtsi32_si128(left_ptr[height - 1]);
  const __m128i top_right = _mm_set1_epi16(top_ptr[width - 1]);
  const __m128i round = _mm_set1_epi32(256);
  auto* dst = static_cast<uint8_t*>(dest);
  for (int y = 0; y < height; ++y) {
    const __m128i weights_y = _mm_cvtsi32_si128(sm_weights_h[y]);
    const __m128i left_y = _mm_cvtsi32_si128(left_ptr[y]);
    const __m128i scale_m_weights_y = _mm_sub_epi16(scale_value, weights_y);
    __m128i scaled_bottom_left =
        _mm_mullo_epi16(scale_m_weights_y, bottom_left);
    const __m128i weight_left_y =
        _mm_shuffle_epi32(_mm_unpacklo_epi16(weights_y, left_y), 0);
    scaled_bottom_left = _mm_add_epi32(scaled_bottom_left, round);
    scaled_bottom_left = _mm_shuffle_epi32(scaled_bottom_left, 0);
    for (int x = 0; x < width; x += 8) {
      const __m128i top_x = LoadLo8(top_ptr + x);
      const __m128i weights_x = LoadLo8(sm_weights_w + x);
      const __m128i top_weights_x = _mm_unpacklo_epi8(top_x, weights_x);
      const __m128i top_weights_x_lo = _mm_cvtepu8_epi16(top_weights_x);
      const __m128i top_weights_x_hi = _mm_unpackhi_epi8(top_weights_x, zero);

      // Here opposite weights and pixels are multiplied, where the order of
      // interleaving is indicated in the names.
      __m128i pred_lo = _mm_madd_epi16(top_weights_x_lo, weight_left_y);
      __m128i pred_hi = _mm_madd_epi16(top_weights_x_hi, weight_left_y);

      // |scaled_bottom_left| is always scaled by the same weight each row, so
      // we only derive |scaled_top_right| values here.
      const __m128i inverted_weights_x =
          _mm_sub_epi16(scale_value, _mm_cvtepu8_epi16(weights_x));
      const __m128i scaled_top_right =
          _mm_mullo_epi16(inverted_weights_x, top_right);
      const __m128i scaled_top_right_lo = _mm_cvtepu16_epi32(scaled_top_right);
      const __m128i scaled_top_right_hi =
          _mm_unpackhi_epi16(scaled_top_right, zero);
      pred_lo = _mm_add_epi32(pred_lo, scaled_bottom_left);
      pred_hi = _mm_add_epi32(pred_hi, scaled_bottom_left);
      pred_lo = _mm_add_epi32(pred_lo, scaled_top_right_lo);
      pred_hi = _mm_add_epi32(pred_hi, scaled_top_right_hi);

      // The round value for RightShiftWithRounding was added with
      // |scaled_bottom_left|.
      pred_lo = _mm_srli_epi32(pred_lo, 9);
      pred_hi = _mm_srli_epi32(pred_hi, 9);
      const __m128i pred = _mm_packus_epi16(pred_lo, pred_hi);
      StoreLo8(dst + x, _mm_packus_epi16(pred, pred));
    }
    dst += stride;
  }
}

void SmoothHorizontal4x4_SSE4_1(void* LIBGAV1_RESTRICT dest,
                                const ptrdiff_t stride,
                                const void* LIBGAV1_RESTRICT top_row,
                                const void* LIBGAV1_RESTRICT left_column) {
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const __m128i top_right = _mm_set1_epi32(top_ptr[3]);
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  const __m128i left = _mm_cvtepu8_epi32(Load4(left_ptr));
  const __m128i weights = _mm_cvtepu8_epi32(Load4(kSmoothWeights));
  __m128i scale = _mm_set1_epi32(256);
  const __m128i inverted_weights = _mm_sub_epi32(scale, weights);
  const __m128i scaled_top_right = _mm_mullo_epi16(inverted_weights, top_right);
  scale = _mm_set1_epi32(128);
  auto* dst = static_cast<uint8_t*>(dest);
  WriteSmoothHorizontalSum4<0>(dst, left, weights, scaled_top_right, scale);
  dst += stride;
  WriteSmoothHorizontalSum4<0x55>(dst, left, weights, scaled_top_right, scale);
  dst += stride;
  WriteSmoothHorizontalSum4<0xAA>(dst, left, weights, scaled_top_right, scale);
  dst += stride;
  WriteSmoothHorizontalSum4<0xFF>(dst, left, weights, scaled_top_right, scale);
}

void SmoothHorizontal4x8_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const __m128i top_right = _mm_set1_epi32(top[3]);
  const __m128i weights = _mm_cvtepu8_epi32(Load4(kSmoothWeights));
  __m128i scale = _mm_set1_epi32(256);
  const __m128i inverted_weights = _mm_sub_epi32(scale, weights);
  const __m128i scaled_top_right = _mm_mullo_epi16(inverted_weights, top_right);
  scale = _mm_set1_epi32(128);
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  __m128i left = _mm_cvtepu8_epi32(Load4(left_column));
  auto* dst = static_cast<uint8_t*>(dest);
  WriteSmoothHorizontalSum4<0>(dst, left, weights, scaled_top_right, scale);
  dst += stride;
  WriteSmoothHorizontalSum4<0x55>(dst, left, weights, scaled_top_right, scale);
  dst += stride;
  WriteSmoothHorizontalSum4<0xAA>(dst, left, weights, scaled_top_right, scale);
  dst += stride;
  WriteSmoothHorizontalSum4<0xFF>(dst, left, weights, scaled_top_right, scale);
  dst += stride;

  left = _mm_cvtepu8_epi32(Load4(left_ptr + 4));
  WriteSmoothHorizontalSum4<0>(dst, left, weights, scaled_top_right, scale);
  dst += stride;
  WriteSmoothHorizontalSum4<0x55>(dst, left, weights, scaled_top_right, scale);
  dst += stride;
  WriteSmoothHorizontalSum4<0xAA>(dst, left, weights, scaled_top_right, scale);
  dst += stride;
  WriteSmoothHorizontalSum4<0xFF>(dst, left, weights, scaled_top_right, scale);
}

void SmoothHorizontal4x16_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const __m128i top_right = _mm_set1_epi32(top[3]);
  const __m128i weights = _mm_cvtepu8_epi32(Load4(kSmoothWeights));
  __m128i scale = _mm_set1_epi32(256);
  const __m128i inverted_weights = _mm_sub_epi32(scale, weights);
  const __m128i scaled_top_right = _mm_mullo_epi16(inverted_weights, top_right);
  scale = _mm_set1_epi32(128);
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  __m128i left = _mm_cvtepu8_epi32(Load4(left_column));
  auto* dst = static_cast<uint8_t*>(dest);
  WriteSmoothHorizontalSum4<0>(dst, left, weights, scaled_top_right, scale);
  dst += stride;
  WriteSmoothHorizontalSum4<0x55>(dst, left, weights, scaled_top_right, scale);
  dst += stride;
  WriteSmoothHorizontalSum4<0xAA>(dst, left, weights, scaled_top_right, scale);
  dst += stride;
  WriteSmoothHorizontalSum4<0xFF>(dst, left, weights, scaled_top_right, scale);
  dst += stride;

  left = _mm_cvtepu8_epi32(Load4(left_ptr + 4));
  WriteSmoothHorizontalSum4<0>(dst, left, weights, scaled_top_right, scale);
  dst += stride;
  WriteSmoothHorizontalSum4<0x55>(dst, left, weights, scaled_top_right, scale);
  dst += stride;
  WriteSmoothHorizontalSum4<0xAA>(dst, left, weights, scaled_top_right, scale);
  dst += stride;
  WriteSmoothHorizontalSum4<0xFF>(dst, left, weights, scaled_top_right, scale);
  dst += stride;

  left = _mm_cvtepu8_epi32(Load4(left_ptr + 8));
  WriteSmoothHorizontalSum4<0>(dst, left, weights, scaled_top_right, scale);
  dst += stride;
  WriteSmoothHorizontalSum4<0x55>(dst, left, weights, scaled_top_right, scale);
  dst += stride;
  WriteSmoothHorizontalSum4<0xAA>(dst, left, weights, scaled_top_right, scale);
  dst += stride;
  WriteSmoothHorizontalSum4<0xFF>(dst, left, weights, scaled_top_right, scale);
  dst += stride;

  left = _mm_cvtepu8_epi32(Load4(left_ptr + 12));
  WriteSmoothHorizontalSum4<0>(dst, left, weights, scaled_top_right, scale);
  dst += stride;
  WriteSmoothHorizontalSum4<0x55>(dst, left, weights, scaled_top_right, scale);
  dst += stride;
  WriteSmoothHorizontalSum4<0xAA>(dst, left, weights, scaled_top_right, scale);
  dst += stride;
  WriteSmoothHorizontalSum4<0xFF>(dst, left, weights, scaled_top_right, scale);
}

void SmoothHorizontal8x4_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const __m128i top_right = _mm_set1_epi16(top[7]);
  const __m128i left = _mm_cvtepu8_epi16(Load4(left_column));
  const __m128i weights = _mm_cvtepu8_epi16(LoadLo8(kSmoothWeights + 4));
  __m128i scale = _mm_set1_epi16(256);
  const __m128i inverted_weights = _mm_sub_epi16(scale, weights);
  const __m128i scaled_top_right = _mm_mullo_epi16(inverted_weights, top_right);
  scale = _mm_set1_epi16(128);
  __m128i y_select = _mm_set1_epi32(0x01000100);
  __m128i left_y = _mm_shuffle_epi8(left, y_select);
  auto* dst = static_cast<uint8_t*>(dest);
  WriteSmoothDirectionalSum8(dst, left_y, weights, scaled_top_right, scale);
  dst += stride;
  y_select = _mm_set1_epi32(0x03020302);
  left_y = _mm_shuffle_epi8(left, y_select);
  WriteSmoothDirectionalSum8(dst, left_y, weights, scaled_top_right, scale);
  dst += stride;
  y_select = _mm_set1_epi32(0x05040504);
  left_y = _mm_shuffle_epi8(left, y_select);
  WriteSmoothDirectionalSum8(dst, left_y, weights, scaled_top_right, scale);
  dst += stride;
  y_select = _mm_set1_epi32(0x07060706);
  left_y = _mm_shuffle_epi8(left, y_select);
  WriteSmoothDirectionalSum8(dst, left_y, weights, scaled_top_right, scale);
}

void SmoothHorizontal8x8_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const __m128i top_right = _mm_set1_epi16(top[7]);
  const __m128i left = _mm_cvtepu8_epi16(LoadLo8(left_column));
  const __m128i weights = _mm_cvtepu8_epi16(LoadLo8(kSmoothWeights + 4));
  __m128i scale = _mm_set1_epi16(256);
  const __m128i inverted_weights = _mm_sub_epi16(scale, weights);
  const __m128i scaled_top_right = _mm_mullo_epi16(inverted_weights, top_right);
  scale = _mm_set1_epi16(128);
  auto* dst = static_cast<uint8_t*>(dest);
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i left_y = _mm_shuffle_epi8(left, y_select);
    WriteSmoothDirectionalSum8(dst, left_y, weights, scaled_top_right, scale);
    dst += stride;
  }
}

void SmoothHorizontal8x16_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const __m128i top_right = _mm_set1_epi16(top[7]);
  const __m128i weights = _mm_cvtepu8_epi16(LoadLo8(kSmoothWeights + 4));
  __m128i scale = _mm_set1_epi16(256);
  const __m128i inverted_weights = _mm_sub_epi16(scale, weights);
  const __m128i scaled_top_right = _mm_mullo_epi16(inverted_weights, top_right);
  scale = _mm_set1_epi16(128);
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  __m128i left = _mm_cvtepu8_epi16(LoadLo8(left_column));
  auto* dst = static_cast<uint8_t*>(dest);
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i left_y = _mm_shuffle_epi8(left, y_select);
    WriteSmoothDirectionalSum8(dst, left_y, weights, scaled_top_right, scale);
    dst += stride;
  }
  left = _mm_cvtepu8_epi16(LoadLo8(left_ptr + 8));
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i left_y = _mm_shuffle_epi8(left, y_select);
    WriteSmoothDirectionalSum8(dst, left_y, weights, scaled_top_right, scale);
    dst += stride;
  }
}

void SmoothHorizontal8x32_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const __m128i top_right = _mm_set1_epi16(top[7]);
  const __m128i weights = _mm_cvtepu8_epi16(LoadLo8(kSmoothWeights + 4));
  __m128i scale = _mm_set1_epi16(256);
  const __m128i inverted_weights = _mm_sub_epi16(scale, weights);
  const __m128i scaled_top_right = _mm_mullo_epi16(inverted_weights, top_right);
  scale = _mm_set1_epi16(128);
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  __m128i left = _mm_cvtepu8_epi16(LoadLo8(left_column));
  auto* dst = static_cast<uint8_t*>(dest);
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i left_y = _mm_shuffle_epi8(left, y_select);
    WriteSmoothDirectionalSum8(dst, left_y, weights, scaled_top_right, scale);
    dst += stride;
  }
  left = _mm_cvtepu8_epi16(LoadLo8(left_ptr + 8));
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i left_y = _mm_shuffle_epi8(left, y_select);
    WriteSmoothDirectionalSum8(dst, left_y, weights, scaled_top_right, scale);
    dst += stride;
  }
  left = _mm_cvtepu8_epi16(LoadLo8(left_ptr + 16));
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i left_y = _mm_shuffle_epi8(left, y_select);
    WriteSmoothDirectionalSum8(dst, left_y, weights, scaled_top_right, scale);
    dst += stride;
  }
  left = _mm_cvtepu8_epi16(LoadLo8(left_ptr + 24));
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i left_y = _mm_shuffle_epi8(left, y_select);
    WriteSmoothDirectionalSum8(dst, left_y, weights, scaled_top_right, scale);
    dst += stride;
  }
}

void SmoothHorizontal16x4_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const __m128i top_right = _mm_set1_epi16(top[15]);
  const __m128i left = _mm_cvtepu8_epi16(Load4(left_column));
  const __m128i weights = LoadUnaligned16(kSmoothWeights + 12);
  __m128i scale = _mm_set1_epi16(256);
  const __m128i weights1 = _mm_cvtepu8_epi16(weights);
  const __m128i weights2 = _mm_cvtepu8_epi16(_mm_srli_si128(weights, 8));
  const __m128i inverted_weights1 = _mm_sub_epi16(scale, weights1);
  const __m128i inverted_weights2 = _mm_sub_epi16(scale, weights2);
  const __m128i scaled_top_right1 =
      _mm_mullo_epi16(inverted_weights1, top_right);
  const __m128i scaled_top_right2 =
      _mm_mullo_epi16(inverted_weights2, top_right);
  scale = _mm_set1_epi16(128);
  __m128i y_mask = _mm_set1_epi32(0x01000100);
  __m128i left_y = _mm_shuffle_epi8(left, y_mask);
  auto* dst = static_cast<uint8_t*>(dest);
  WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                              scaled_top_right1, scaled_top_right2, scale);
  dst += stride;
  y_mask = _mm_set1_epi32(0x03020302);
  left_y = _mm_shuffle_epi8(left, y_mask);
  WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                              scaled_top_right1, scaled_top_right2, scale);
  dst += stride;
  y_mask = _mm_set1_epi32(0x05040504);
  left_y = _mm_shuffle_epi8(left, y_mask);
  WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                              scaled_top_right1, scaled_top_right2, scale);
  dst += stride;
  y_mask = _mm_set1_epi32(0x07060706);
  left_y = _mm_shuffle_epi8(left, y_mask);
  WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                              scaled_top_right1, scaled_top_right2, scale);
}

void SmoothHorizontal16x8_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const __m128i top_right = _mm_set1_epi16(top[15]);
  const __m128i left = _mm_cvtepu8_epi16(LoadLo8(left_column));
  const __m128i weights = LoadUnaligned16(kSmoothWeights + 12);
  __m128i scale = _mm_set1_epi16(256);
  const __m128i weights1 = _mm_cvtepu8_epi16(weights);
  const __m128i weights2 = _mm_cvtepu8_epi16(_mm_srli_si128(weights, 8));
  const __m128i inverted_weights1 = _mm_sub_epi16(scale, weights1);
  const __m128i inverted_weights2 = _mm_sub_epi16(scale, weights2);
  const __m128i scaled_top_right1 =
      _mm_mullo_epi16(inverted_weights1, top_right);
  const __m128i scaled_top_right2 =
      _mm_mullo_epi16(inverted_weights2, top_right);
  scale = _mm_set1_epi16(128);
  auto* dst = static_cast<uint8_t*>(dest);
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i left_y = _mm_shuffle_epi8(left, y_select);
    WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                scaled_top_right1, scaled_top_right2, scale);
    dst += stride;
  }
}

void SmoothHorizontal16x16_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const __m128i top_right = _mm_set1_epi16(top[15]);
  const __m128i weights = LoadUnaligned16(kSmoothWeights + 12);
  __m128i scale = _mm_set1_epi16(256);
  const __m128i weights1 = _mm_cvtepu8_epi16(weights);
  const __m128i weights2 = _mm_cvtepu8_epi16(_mm_srli_si128(weights, 8));
  const __m128i inverted_weights1 = _mm_sub_epi16(scale, weights1);
  const __m128i inverted_weights2 = _mm_sub_epi16(scale, weights2);
  const __m128i scaled_top_right1 =
      _mm_mullo_epi16(inverted_weights1, top_right);
  const __m128i scaled_top_right2 =
      _mm_mullo_epi16(inverted_weights2, top_right);
  scale = _mm_set1_epi16(128);
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  __m128i left = _mm_cvtepu8_epi16(LoadLo8(left_column));
  auto* dst = static_cast<uint8_t*>(dest);
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i left_y = _mm_shuffle_epi8(left, y_select);
    WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                scaled_top_right1, scaled_top_right2, scale);
    dst += stride;
  }
  left = _mm_cvtepu8_epi16(LoadLo8(left_ptr + 8));
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i left_y = _mm_shuffle_epi8(left, y_select);
    WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                scaled_top_right1, scaled_top_right2, scale);
    dst += stride;
  }
}

void SmoothHorizontal16x32_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const __m128i top_right = _mm_set1_epi16(top[15]);
  const __m128i weights = LoadUnaligned16(kSmoothWeights + 12);
  __m128i scale = _mm_set1_epi16(256);
  const __m128i weights1 = _mm_cvtepu8_epi16(weights);
  const __m128i weights2 = _mm_cvtepu8_epi16(_mm_srli_si128(weights, 8));
  const __m128i inverted_weights1 = _mm_sub_epi16(scale, weights1);
  const __m128i inverted_weights2 = _mm_sub_epi16(scale, weights2);
  const __m128i scaled_top_right1 =
      _mm_mullo_epi16(inverted_weights1, top_right);
  const __m128i scaled_top_right2 =
      _mm_mullo_epi16(inverted_weights2, top_right);
  scale = _mm_set1_epi16(128);
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  __m128i left = _mm_cvtepu8_epi16(LoadLo8(left_column));
  auto* dst = static_cast<uint8_t*>(dest);
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i left_y = _mm_shuffle_epi8(left, y_select);
    WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                scaled_top_right1, scaled_top_right2, scale);
    dst += stride;
  }
  left = _mm_cvtepu8_epi16(LoadLo8(left_ptr + 8));
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i left_y = _mm_shuffle_epi8(left, y_select);
    WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                scaled_top_right1, scaled_top_right2, scale);
    dst += stride;
  }
  left = _mm_cvtepu8_epi16(LoadLo8(left_ptr + 16));
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i left_y = _mm_shuffle_epi8(left, y_select);
    WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                scaled_top_right1, scaled_top_right2, scale);
    dst += stride;
  }
  left = _mm_cvtepu8_epi16(LoadLo8(left_ptr + 24));
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i left_y = _mm_shuffle_epi8(left, y_select);
    WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                scaled_top_right1, scaled_top_right2, scale);
    dst += stride;
  }
}

void SmoothHorizontal16x64_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const __m128i top_right = _mm_set1_epi16(top[15]);
  const __m128i weights = LoadUnaligned16(kSmoothWeights + 12);
  __m128i scale = _mm_set1_epi16(256);
  const __m128i weights1 = _mm_cvtepu8_epi16(weights);
  const __m128i weights2 = _mm_cvtepu8_epi16(_mm_srli_si128(weights, 8));
  const __m128i inverted_weights1 = _mm_sub_epi16(scale, weights1);
  const __m128i inverted_weights2 = _mm_sub_epi16(scale, weights2);
  const __m128i scaled_top_right1 =
      _mm_mullo_epi16(inverted_weights1, top_right);
  const __m128i scaled_top_right2 =
      _mm_mullo_epi16(inverted_weights2, top_right);
  scale = _mm_set1_epi16(128);
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  auto* dst = static_cast<uint8_t*>(dest);
  for (int left_offset = 0; left_offset < 64; left_offset += 8) {
    const __m128i left = _mm_cvtepu8_epi16(LoadLo8(left_ptr + left_offset));
    for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
      const __m128i y_select = _mm_set1_epi32(y_mask);
      const __m128i left_y = _mm_shuffle_epi8(left, y_select);
      WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                  scaled_top_right1, scaled_top_right2, scale);
      dst += stride;
    }
  }
}

void SmoothHorizontal32x8_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const __m128i top_right = _mm_set1_epi16(top[31]);
  const __m128i left = _mm_cvtepu8_epi16(LoadLo8(left_column));
  const __m128i weights_lo = LoadUnaligned16(kSmoothWeights + 28);
  const __m128i weights_hi = LoadUnaligned16(kSmoothWeights + 44);
  __m128i scale = _mm_set1_epi16(256);
  const __m128i weights1 = _mm_cvtepu8_epi16(weights_lo);
  const __m128i weights2 = _mm_cvtepu8_epi16(_mm_srli_si128(weights_lo, 8));
  const __m128i weights3 = _mm_cvtepu8_epi16(weights_hi);
  const __m128i weights4 = _mm_cvtepu8_epi16(_mm_srli_si128(weights_hi, 8));
  const __m128i inverted_weights1 = _mm_sub_epi16(scale, weights1);
  const __m128i inverted_weights2 = _mm_sub_epi16(scale, weights2);
  const __m128i inverted_weights3 = _mm_sub_epi16(scale, weights3);
  const __m128i inverted_weights4 = _mm_sub_epi16(scale, weights4);
  const __m128i scaled_top_right1 =
      _mm_mullo_epi16(inverted_weights1, top_right);
  const __m128i scaled_top_right2 =
      _mm_mullo_epi16(inverted_weights2, top_right);
  const __m128i scaled_top_right3 =
      _mm_mullo_epi16(inverted_weights3, top_right);
  const __m128i scaled_top_right4 =
      _mm_mullo_epi16(inverted_weights4, top_right);
  scale = _mm_set1_epi16(128);
  auto* dst = static_cast<uint8_t*>(dest);
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    __m128i y_select = _mm_set1_epi32(y_mask);
    __m128i left_y = _mm_shuffle_epi8(left, y_select);
    WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                scaled_top_right1, scaled_top_right2, scale);
    WriteSmoothDirectionalSum16(dst + 16, left_y, left_y, weights3, weights4,
                                scaled_top_right3, scaled_top_right4, scale);
    dst += stride;
  }
}

void SmoothHorizontal32x16_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const __m128i top_right = _mm_set1_epi16(top[31]);
  const __m128i left1 = _mm_cvtepu8_epi16(LoadLo8(left_column));
  const __m128i weights_lo = LoadUnaligned16(kSmoothWeights + 28);
  const __m128i weights_hi = LoadUnaligned16(kSmoothWeights + 44);
  __m128i scale = _mm_set1_epi16(256);
  const __m128i weights1 = _mm_cvtepu8_epi16(weights_lo);
  const __m128i weights2 = _mm_cvtepu8_epi16(_mm_srli_si128(weights_lo, 8));
  const __m128i weights3 = _mm_cvtepu8_epi16(weights_hi);
  const __m128i weights4 = _mm_cvtepu8_epi16(_mm_srli_si128(weights_hi, 8));
  const __m128i inverted_weights1 = _mm_sub_epi16(scale, weights1);
  const __m128i inverted_weights2 = _mm_sub_epi16(scale, weights2);
  const __m128i inverted_weights3 = _mm_sub_epi16(scale, weights3);
  const __m128i inverted_weights4 = _mm_sub_epi16(scale, weights4);
  const __m128i scaled_top_right1 =
      _mm_mullo_epi16(inverted_weights1, top_right);
  const __m128i scaled_top_right2 =
      _mm_mullo_epi16(inverted_weights2, top_right);
  const __m128i scaled_top_right3 =
      _mm_mullo_epi16(inverted_weights3, top_right);
  const __m128i scaled_top_right4 =
      _mm_mullo_epi16(inverted_weights4, top_right);
  scale = _mm_set1_epi16(128);
  auto* dst = static_cast<uint8_t*>(dest);
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    __m128i y_select = _mm_set1_epi32(y_mask);
    __m128i left_y = _mm_shuffle_epi8(left1, y_select);
    WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                scaled_top_right1, scaled_top_right2, scale);
    WriteSmoothDirectionalSum16(dst + 16, left_y, left_y, weights3, weights4,
                                scaled_top_right3, scaled_top_right4, scale);
    dst += stride;
  }
  const __m128i left2 =
      _mm_cvtepu8_epi16(LoadLo8(static_cast<const uint8_t*>(left_column) + 8));
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    __m128i y_select = _mm_set1_epi32(y_mask);
    __m128i left_y = _mm_shuffle_epi8(left2, y_select);
    WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                scaled_top_right1, scaled_top_right2, scale);
    WriteSmoothDirectionalSum16(dst + 16, left_y, left_y, weights3, weights4,
                                scaled_top_right3, scaled_top_right4, scale);
    dst += stride;
  }
}

void SmoothHorizontal32x32_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const __m128i top_right = _mm_set1_epi16(top[31]);
  const __m128i weights_lo = LoadUnaligned16(kSmoothWeights + 28);
  const __m128i weights_hi = LoadUnaligned16(kSmoothWeights + 44);
  __m128i scale = _mm_set1_epi16(256);
  const __m128i weights1 = _mm_cvtepu8_epi16(weights_lo);
  const __m128i weights2 = _mm_cvtepu8_epi16(_mm_srli_si128(weights_lo, 8));
  const __m128i weights3 = _mm_cvtepu8_epi16(weights_hi);
  const __m128i weights4 = _mm_cvtepu8_epi16(_mm_srli_si128(weights_hi, 8));
  const __m128i inverted_weights1 = _mm_sub_epi16(scale, weights1);
  const __m128i inverted_weights2 = _mm_sub_epi16(scale, weights2);
  const __m128i inverted_weights3 = _mm_sub_epi16(scale, weights3);
  const __m128i inverted_weights4 = _mm_sub_epi16(scale, weights4);
  const __m128i scaled_top_right1 =
      _mm_mullo_epi16(inverted_weights1, top_right);
  const __m128i scaled_top_right2 =
      _mm_mullo_epi16(inverted_weights2, top_right);
  const __m128i scaled_top_right3 =
      _mm_mullo_epi16(inverted_weights3, top_right);
  const __m128i scaled_top_right4 =
      _mm_mullo_epi16(inverted_weights4, top_right);
  scale = _mm_set1_epi16(128);
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  __m128i left = _mm_cvtepu8_epi16(LoadLo8(left_column));
  auto* dst = static_cast<uint8_t*>(dest);
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    __m128i y_select = _mm_set1_epi32(y_mask);
    __m128i left_y = _mm_shuffle_epi8(left, y_select);
    WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                scaled_top_right1, scaled_top_right2, scale);
    WriteSmoothDirectionalSum16(dst + 16, left_y, left_y, weights3, weights4,
                                scaled_top_right3, scaled_top_right4, scale);
    dst += stride;
  }
  left = _mm_cvtepu8_epi16(LoadLo8(left_ptr + 8));
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    __m128i y_select = _mm_set1_epi32(y_mask);
    __m128i left_y = _mm_shuffle_epi8(left, y_select);
    WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                scaled_top_right1, scaled_top_right2, scale);
    WriteSmoothDirectionalSum16(dst + 16, left_y, left_y, weights3, weights4,
                                scaled_top_right3, scaled_top_right4, scale);
    dst += stride;
  }
  left = _mm_cvtepu8_epi16(LoadLo8(left_ptr + 16));
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    __m128i y_select = _mm_set1_epi32(y_mask);
    __m128i left_y = _mm_shuffle_epi8(left, y_select);
    WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                scaled_top_right1, scaled_top_right2, scale);
    WriteSmoothDirectionalSum16(dst + 16, left_y, left_y, weights3, weights4,
                                scaled_top_right3, scaled_top_right4, scale);
    dst += stride;
  }
  left = _mm_cvtepu8_epi16(LoadLo8(left_ptr + 24));
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    __m128i y_select = _mm_set1_epi32(y_mask);
    __m128i left_y = _mm_shuffle_epi8(left, y_select);
    WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                scaled_top_right1, scaled_top_right2, scale);
    WriteSmoothDirectionalSum16(dst + 16, left_y, left_y, weights3, weights4,
                                scaled_top_right3, scaled_top_right4, scale);
    dst += stride;
  }
}

void SmoothHorizontal32x64_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const __m128i top_right = _mm_set1_epi16(top[31]);
  const __m128i weights_lo = LoadUnaligned16(kSmoothWeights + 28);
  const __m128i weights_hi = LoadUnaligned16(kSmoothWeights + 44);
  __m128i scale = _mm_set1_epi16(256);
  const __m128i weights1 = _mm_cvtepu8_epi16(weights_lo);
  const __m128i weights2 = _mm_cvtepu8_epi16(_mm_srli_si128(weights_lo, 8));
  const __m128i weights3 = _mm_cvtepu8_epi16(weights_hi);
  const __m128i weights4 = _mm_cvtepu8_epi16(_mm_srli_si128(weights_hi, 8));
  const __m128i inverted_weights1 = _mm_sub_epi16(scale, weights1);
  const __m128i inverted_weights2 = _mm_sub_epi16(scale, weights2);
  const __m128i inverted_weights3 = _mm_sub_epi16(scale, weights3);
  const __m128i inverted_weights4 = _mm_sub_epi16(scale, weights4);
  const __m128i scaled_top_right1 =
      _mm_mullo_epi16(inverted_weights1, top_right);
  const __m128i scaled_top_right2 =
      _mm_mullo_epi16(inverted_weights2, top_right);
  const __m128i scaled_top_right3 =
      _mm_mullo_epi16(inverted_weights3, top_right);
  const __m128i scaled_top_right4 =
      _mm_mullo_epi16(inverted_weights4, top_right);
  scale = _mm_set1_epi16(128);
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  auto* dst = static_cast<uint8_t*>(dest);
  for (int left_offset = 0; left_offset < 64; left_offset += 8) {
    const __m128i left = _mm_cvtepu8_epi16(LoadLo8(left_ptr + left_offset));
    for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
      const __m128i y_select = _mm_set1_epi32(y_mask);
      const __m128i left_y = _mm_shuffle_epi8(left, y_select);
      WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                  scaled_top_right1, scaled_top_right2, scale);
      WriteSmoothDirectionalSum16(dst + 16, left_y, left_y, weights3, weights4,
                                  scaled_top_right3, scaled_top_right4, scale);
      dst += stride;
    }
  }
}

void SmoothHorizontal64x16_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const __m128i top_right = _mm_set1_epi16(top[63]);
  const __m128i left1 = _mm_cvtepu8_epi16(LoadLo8(left_column));
  const __m128i weights_lolo = LoadUnaligned16(kSmoothWeights + 60);
  const __m128i weights_lohi = LoadUnaligned16(kSmoothWeights + 76);
  __m128i scale = _mm_set1_epi16(256);
  const __m128i weights1 = _mm_cvtepu8_epi16(weights_lolo);
  const __m128i weights2 = _mm_cvtepu8_epi16(_mm_srli_si128(weights_lolo, 8));
  const __m128i weights3 = _mm_cvtepu8_epi16(weights_lohi);
  const __m128i weights4 = _mm_cvtepu8_epi16(_mm_srli_si128(weights_lohi, 8));
  const __m128i inverted_weights1 = _mm_sub_epi16(scale, weights1);
  const __m128i inverted_weights2 = _mm_sub_epi16(scale, weights2);
  const __m128i inverted_weights3 = _mm_sub_epi16(scale, weights3);
  const __m128i inverted_weights4 = _mm_sub_epi16(scale, weights4);
  const __m128i scaled_top_right1 =
      _mm_mullo_epi16(inverted_weights1, top_right);
  const __m128i scaled_top_right2 =
      _mm_mullo_epi16(inverted_weights2, top_right);
  const __m128i scaled_top_right3 =
      _mm_mullo_epi16(inverted_weights3, top_right);
  const __m128i scaled_top_right4 =
      _mm_mullo_epi16(inverted_weights4, top_right);
  const __m128i weights_hilo = LoadUnaligned16(kSmoothWeights + 92);
  const __m128i weights_hihi = LoadUnaligned16(kSmoothWeights + 108);
  const __m128i weights5 = _mm_cvtepu8_epi16(weights_hilo);
  const __m128i weights6 = _mm_cvtepu8_epi16(_mm_srli_si128(weights_hilo, 8));
  const __m128i weights7 = _mm_cvtepu8_epi16(weights_hihi);
  const __m128i weights8 = _mm_cvtepu8_epi16(_mm_srli_si128(weights_hihi, 8));
  const __m128i inverted_weights5 = _mm_sub_epi16(scale, weights5);
  const __m128i inverted_weights6 = _mm_sub_epi16(scale, weights6);
  const __m128i inverted_weights7 = _mm_sub_epi16(scale, weights7);
  const __m128i inverted_weights8 = _mm_sub_epi16(scale, weights8);
  const __m128i scaled_top_right5 =
      _mm_mullo_epi16(inverted_weights5, top_right);
  const __m128i scaled_top_right6 =
      _mm_mullo_epi16(inverted_weights6, top_right);
  const __m128i scaled_top_right7 =
      _mm_mullo_epi16(inverted_weights7, top_right);
  const __m128i scaled_top_right8 =
      _mm_mullo_epi16(inverted_weights8, top_right);
  scale = _mm_set1_epi16(128);
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  auto* dst = static_cast<uint8_t*>(dest);
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    __m128i y_select = _mm_set1_epi32(y_mask);
    __m128i left_y = _mm_shuffle_epi8(left1, y_select);
    WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                scaled_top_right1, scaled_top_right2, scale);
    WriteSmoothDirectionalSum16(dst + 16, left_y, left_y, weights3, weights4,
                                scaled_top_right3, scaled_top_right4, scale);
    WriteSmoothDirectionalSum16(dst + 32, left_y, left_y, weights5, weights6,
                                scaled_top_right5, scaled_top_right6, scale);
    WriteSmoothDirectionalSum16(dst + 48, left_y, left_y, weights7, weights8,
                                scaled_top_right7, scaled_top_right8, scale);
    dst += stride;
  }
  const __m128i left2 = _mm_cvtepu8_epi16(LoadLo8(left_ptr + 8));
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    __m128i y_select = _mm_set1_epi32(y_mask);
    __m128i left_y = _mm_shuffle_epi8(left2, y_select);
    WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                scaled_top_right1, scaled_top_right2, scale);
    WriteSmoothDirectionalSum16(dst + 16, left_y, left_y, weights3, weights4,
                                scaled_top_right3, scaled_top_right4, scale);
    WriteSmoothDirectionalSum16(dst + 32, left_y, left_y, weights5, weights6,
                                scaled_top_right5, scaled_top_right6, scale);
    WriteSmoothDirectionalSum16(dst + 48, left_y, left_y, weights7, weights8,
                                scaled_top_right7, scaled_top_right8, scale);
    dst += stride;
  }
}

void SmoothHorizontal64x32_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const __m128i top_right = _mm_set1_epi16(top[63]);
  const __m128i left1 = _mm_cvtepu8_epi16(LoadLo8(left_column));
  const __m128i weights_lolo = LoadUnaligned16(kSmoothWeights + 60);
  const __m128i weights_lohi = LoadUnaligned16(kSmoothWeights + 76);
  __m128i scale = _mm_set1_epi16(256);
  const __m128i weights1 = _mm_cvtepu8_epi16(weights_lolo);
  const __m128i weights2 = _mm_cvtepu8_epi16(_mm_srli_si128(weights_lolo, 8));
  const __m128i weights3 = _mm_cvtepu8_epi16(weights_lohi);
  const __m128i weights4 = _mm_cvtepu8_epi16(_mm_srli_si128(weights_lohi, 8));
  const __m128i inverted_weights1 = _mm_sub_epi16(scale, weights1);
  const __m128i inverted_weights2 = _mm_sub_epi16(scale, weights2);
  const __m128i inverted_weights3 = _mm_sub_epi16(scale, weights3);
  const __m128i inverted_weights4 = _mm_sub_epi16(scale, weights4);
  const __m128i scaled_top_right1 =
      _mm_mullo_epi16(inverted_weights1, top_right);
  const __m128i scaled_top_right2 =
      _mm_mullo_epi16(inverted_weights2, top_right);
  const __m128i scaled_top_right3 =
      _mm_mullo_epi16(inverted_weights3, top_right);
  const __m128i scaled_top_right4 =
      _mm_mullo_epi16(inverted_weights4, top_right);
  const __m128i weights_hilo = LoadUnaligned16(kSmoothWeights + 92);
  const __m128i weights_hihi = LoadUnaligned16(kSmoothWeights + 108);
  const __m128i weights5 = _mm_cvtepu8_epi16(weights_hilo);
  const __m128i weights6 = _mm_cvtepu8_epi16(_mm_srli_si128(weights_hilo, 8));
  const __m128i weights7 = _mm_cvtepu8_epi16(weights_hihi);
  const __m128i weights8 = _mm_cvtepu8_epi16(_mm_srli_si128(weights_hihi, 8));
  const __m128i inverted_weights5 = _mm_sub_epi16(scale, weights5);
  const __m128i inverted_weights6 = _mm_sub_epi16(scale, weights6);
  const __m128i inverted_weights7 = _mm_sub_epi16(scale, weights7);
  const __m128i inverted_weights8 = _mm_sub_epi16(scale, weights8);
  const __m128i scaled_top_right5 =
      _mm_mullo_epi16(inverted_weights5, top_right);
  const __m128i scaled_top_right6 =
      _mm_mullo_epi16(inverted_weights6, top_right);
  const __m128i scaled_top_right7 =
      _mm_mullo_epi16(inverted_weights7, top_right);
  const __m128i scaled_top_right8 =
      _mm_mullo_epi16(inverted_weights8, top_right);
  scale = _mm_set1_epi16(128);
  auto* dst = static_cast<uint8_t*>(dest);
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i left_y = _mm_shuffle_epi8(left1, y_select);
    WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                scaled_top_right1, scaled_top_right2, scale);
    WriteSmoothDirectionalSum16(dst + 16, left_y, left_y, weights3, weights4,
                                scaled_top_right3, scaled_top_right4, scale);
    WriteSmoothDirectionalSum16(dst + 32, left_y, left_y, weights5, weights6,
                                scaled_top_right5, scaled_top_right6, scale);
    WriteSmoothDirectionalSum16(dst + 48, left_y, left_y, weights7, weights8,
                                scaled_top_right7, scaled_top_right8, scale);
    dst += stride;
  }
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  const __m128i left2 = _mm_cvtepu8_epi16(LoadLo8(left_ptr + 8));
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i left_y = _mm_shuffle_epi8(left2, y_select);
    WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                scaled_top_right1, scaled_top_right2, scale);
    WriteSmoothDirectionalSum16(dst + 16, left_y, left_y, weights3, weights4,
                                scaled_top_right3, scaled_top_right4, scale);
    WriteSmoothDirectionalSum16(dst + 32, left_y, left_y, weights5, weights6,
                                scaled_top_right5, scaled_top_right6, scale);
    WriteSmoothDirectionalSum16(dst + 48, left_y, left_y, weights7, weights8,
                                scaled_top_right7, scaled_top_right8, scale);
    dst += stride;
  }
  const __m128i left3 = _mm_cvtepu8_epi16(LoadLo8(left_ptr + 16));
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i left_y = _mm_shuffle_epi8(left3, y_select);
    WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                scaled_top_right1, scaled_top_right2, scale);
    WriteSmoothDirectionalSum16(dst + 16, left_y, left_y, weights3, weights4,
                                scaled_top_right3, scaled_top_right4, scale);
    WriteSmoothDirectionalSum16(dst + 32, left_y, left_y, weights5, weights6,
                                scaled_top_right5, scaled_top_right6, scale);
    WriteSmoothDirectionalSum16(dst + 48, left_y, left_y, weights7, weights8,
                                scaled_top_right7, scaled_top_right8, scale);
    dst += stride;
  }
  const __m128i left4 = _mm_cvtepu8_epi16(LoadLo8(left_ptr + 24));
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i left_y = _mm_shuffle_epi8(left4, y_select);
    WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                scaled_top_right1, scaled_top_right2, scale);
    WriteSmoothDirectionalSum16(dst + 16, left_y, left_y, weights3, weights4,
                                scaled_top_right3, scaled_top_right4, scale);
    WriteSmoothDirectionalSum16(dst + 32, left_y, left_y, weights5, weights6,
                                scaled_top_right5, scaled_top_right6, scale);
    WriteSmoothDirectionalSum16(dst + 48, left_y, left_y, weights7, weights8,
                                scaled_top_right7, scaled_top_right8, scale);
    dst += stride;
  }
}

void SmoothHorizontal64x64_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const __m128i top_right = _mm_set1_epi16(top[63]);
  const __m128i weights_lolo = LoadUnaligned16(kSmoothWeights + 60);
  const __m128i weights_lohi = LoadUnaligned16(kSmoothWeights + 76);
  __m128i scale = _mm_set1_epi16(256);
  const __m128i weights1 = _mm_cvtepu8_epi16(weights_lolo);
  const __m128i weights2 = _mm_cvtepu8_epi16(_mm_srli_si128(weights_lolo, 8));
  const __m128i weights3 = _mm_cvtepu8_epi16(weights_lohi);
  const __m128i weights4 = _mm_cvtepu8_epi16(_mm_srli_si128(weights_lohi, 8));
  const __m128i inverted_weights1 = _mm_sub_epi16(scale, weights1);
  const __m128i inverted_weights2 = _mm_sub_epi16(scale, weights2);
  const __m128i inverted_weights3 = _mm_sub_epi16(scale, weights3);
  const __m128i inverted_weights4 = _mm_sub_epi16(scale, weights4);
  const __m128i scaled_top_right1 =
      _mm_mullo_epi16(inverted_weights1, top_right);
  const __m128i scaled_top_right2 =
      _mm_mullo_epi16(inverted_weights2, top_right);
  const __m128i scaled_top_right3 =
      _mm_mullo_epi16(inverted_weights3, top_right);
  const __m128i scaled_top_right4 =
      _mm_mullo_epi16(inverted_weights4, top_right);
  const __m128i weights_hilo = LoadUnaligned16(kSmoothWeights + 92);
  const __m128i weights_hihi = LoadUnaligned16(kSmoothWeights + 108);
  const __m128i weights5 = _mm_cvtepu8_epi16(weights_hilo);
  const __m128i weights6 = _mm_cvtepu8_epi16(_mm_srli_si128(weights_hilo, 8));
  const __m128i weights7 = _mm_cvtepu8_epi16(weights_hihi);
  const __m128i weights8 = _mm_cvtepu8_epi16(_mm_srli_si128(weights_hihi, 8));
  const __m128i inverted_weights5 = _mm_sub_epi16(scale, weights5);
  const __m128i inverted_weights6 = _mm_sub_epi16(scale, weights6);
  const __m128i inverted_weights7 = _mm_sub_epi16(scale, weights7);
  const __m128i inverted_weights8 = _mm_sub_epi16(scale, weights8);
  const __m128i scaled_top_right5 =
      _mm_mullo_epi16(inverted_weights5, top_right);
  const __m128i scaled_top_right6 =
      _mm_mullo_epi16(inverted_weights6, top_right);
  const __m128i scaled_top_right7 =
      _mm_mullo_epi16(inverted_weights7, top_right);
  const __m128i scaled_top_right8 =
      _mm_mullo_epi16(inverted_weights8, top_right);
  scale = _mm_set1_epi16(128);
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  auto* dst = static_cast<uint8_t*>(dest);
  for (int left_offset = 0; left_offset < 64; left_offset += 8) {
    const __m128i left = _mm_cvtepu8_epi16(LoadLo8(left_ptr + left_offset));
    for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
      const __m128i y_select = _mm_set1_epi32(y_mask);
      const __m128i left_y = _mm_shuffle_epi8(left, y_select);
      WriteSmoothDirectionalSum16(dst, left_y, left_y, weights1, weights2,
                                  scaled_top_right1, scaled_top_right2, scale);
      WriteSmoothDirectionalSum16(dst + 16, left_y, left_y, weights3, weights4,
                                  scaled_top_right3, scaled_top_right4, scale);
      WriteSmoothDirectionalSum16(dst + 32, left_y, left_y, weights5, weights6,
                                  scaled_top_right5, scaled_top_right6, scale);
      WriteSmoothDirectionalSum16(dst + 48, left_y, left_y, weights7, weights8,
                                  scaled_top_right7, scaled_top_right8, scale);
      dst += stride;
    }
  }
}

inline void LoadSmoothVerticalPixels4(const uint8_t* LIBGAV1_RESTRICT above,
                                      const uint8_t* LIBGAV1_RESTRICT left,
                                      const int height, __m128i* pixels) {
  __m128i top = Load4(above);
  const __m128i bottom_left = _mm_set1_epi16(left[height - 1]);
  top = _mm_cvtepu8_epi16(top);
  pixels[0] = _mm_unpacklo_epi16(top, bottom_left);
}

// |weight_array| alternates weight vectors from the table with their inverted
// (256-w) counterparts. This is precomputed by the compiler when the weights
// table is visible to this module. Removing this visibility can cut speed by up
// to half in both 4xH and 8xH transforms.
inline void LoadSmoothVerticalWeights4(const uint8_t* LIBGAV1_RESTRICT
                                           weight_array,
                                       const int height, __m128i* weights) {
  const __m128i inverter = _mm_set1_epi16(256);

  if (height == 4) {
    const __m128i weight = Load4(weight_array);
    weights[0] = _mm_cvtepu8_epi16(weight);
    weights[1] = _mm_sub_epi16(inverter, weights[0]);
  } else if (height == 8) {
    const __m128i weight = LoadLo8(weight_array + 4);
    weights[0] = _mm_cvtepu8_epi16(weight);
    weights[1] = _mm_sub_epi16(inverter, weights[0]);
  } else {
    const __m128i weight = LoadUnaligned16(weight_array + 12);
    const __m128i zero = _mm_setzero_si128();
    weights[0] = _mm_cvtepu8_epi16(weight);
    weights[1] = _mm_sub_epi16(inverter, weights[0]);
    weights[2] = _mm_unpackhi_epi8(weight, zero);
    weights[3] = _mm_sub_epi16(inverter, weights[2]);
  }
}

inline void WriteSmoothVertical4xH(const __m128i* pixel, const __m128i* weight,
                                   const int height,
                                   uint8_t* LIBGAV1_RESTRICT dst,
                                   const ptrdiff_t stride) {
  const __m128i pred_round = _mm_set1_epi32(128);
  const __m128i mask_increment = _mm_set1_epi16(0x0202);
  const __m128i cvtepu8_epi32 = _mm_set1_epi32(0xC080400);
  __m128i y_select = _mm_set1_epi16(0x0100);

  for (int y = 0; y < height; ++y) {
    const __m128i weight_y = _mm_shuffle_epi8(weight[0], y_select);
    const __m128i inverted_weight_y = _mm_shuffle_epi8(weight[1], y_select);
    const __m128i alternate_weights =
        _mm_unpacklo_epi16(weight_y, inverted_weight_y);
    // Here the pixel vector is top_row[0], corner, top_row[1], corner, ...
    // The madd instruction yields four results of the form:
    // (top_row[x] * weight[y] + corner * inverted_weight[y])
    __m128i sum = _mm_madd_epi16(pixel[0], alternate_weights);
    sum = _mm_add_epi32(sum, pred_round);
    sum = _mm_srai_epi32(sum, 8);
    sum = _mm_shuffle_epi8(sum, cvtepu8_epi32);
    Store4(dst, sum);
    dst += stride;
    y_select = _mm_add_epi16(y_select, mask_increment);
  }
}

void SmoothVertical4x4_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                              const ptrdiff_t stride,
                              const void* LIBGAV1_RESTRICT const top_row,
                              const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left = static_cast<const uint8_t*>(left_column);
  const auto* const above = static_cast<const uint8_t*>(top_row);
  auto* dst = static_cast<uint8_t*>(dest);
  __m128i pixels;
  LoadSmoothVerticalPixels4(above, left, 4, &pixels);

  __m128i weights[2];
  LoadSmoothVerticalWeights4(kSmoothWeights, 4, weights);

  WriteSmoothVertical4xH(&pixels, weights, 4, dst, stride);
}

void SmoothVertical4x8_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                              const ptrdiff_t stride,
                              const void* LIBGAV1_RESTRICT const top_row,
                              const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left = static_cast<const uint8_t*>(left_column);
  const auto* const above = static_cast<const uint8_t*>(top_row);
  auto* dst = static_cast<uint8_t*>(dest);
  __m128i pixels;
  LoadSmoothVerticalPixels4(above, left, 8, &pixels);

  __m128i weights[2];
  LoadSmoothVerticalWeights4(kSmoothWeights, 8, weights);

  WriteSmoothVertical4xH(&pixels, weights, 8, dst, stride);
}

void SmoothVertical4x16_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                               const ptrdiff_t stride,
                               const void* LIBGAV1_RESTRICT const top_row,
                               const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left = static_cast<const uint8_t*>(left_column);
  const auto* const above = static_cast<const uint8_t*>(top_row);
  auto* dst = static_cast<uint8_t*>(dest);
  __m128i pixels;
  LoadSmoothVerticalPixels4(above, left, 16, &pixels);

  __m128i weights[4];
  LoadSmoothVerticalWeights4(kSmoothWeights, 16, weights);

  WriteSmoothVertical4xH(&pixels, weights, 8, dst, stride);
  dst += stride << 3;
  WriteSmoothVertical4xH(&pixels, &weights[2], 8, dst, stride);
}

void SmoothVertical8x4_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                              const ptrdiff_t stride,
                              const void* LIBGAV1_RESTRICT const top_row,
                              const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  const __m128i bottom_left = _mm_set1_epi16(left_ptr[3]);
  const __m128i weights = _mm_cvtepu8_epi16(Load4(kSmoothWeights));
  __m128i scale = _mm_set1_epi16(256);
  const __m128i inverted_weights = _mm_sub_epi16(scale, weights);
  const __m128i scaled_bottom_left =
      _mm_mullo_epi16(inverted_weights, bottom_left);
  scale = _mm_set1_epi16(128);

  auto* dst = static_cast<uint8_t*>(dest);
  __m128i y_select = _mm_set1_epi32(0x01000100);
  const __m128i top = _mm_cvtepu8_epi16(LoadLo8(top_row));
  __m128i weights_y = _mm_shuffle_epi8(weights, y_select);
  __m128i scaled_bottom_left_y = _mm_shuffle_epi8(scaled_bottom_left, y_select);
  WriteSmoothDirectionalSum8(dst, top, weights_y, scaled_bottom_left_y, scale);
  dst += stride;
  y_select = _mm_set1_epi32(0x03020302);
  weights_y = _mm_shuffle_epi8(weights, y_select);
  scaled_bottom_left_y = _mm_shuffle_epi8(scaled_bottom_left, y_select);
  WriteSmoothDirectionalSum8(dst, top, weights_y, scaled_bottom_left_y, scale);
  dst += stride;
  y_select = _mm_set1_epi32(0x05040504);
  weights_y = _mm_shuffle_epi8(weights, y_select);
  scaled_bottom_left_y = _mm_shuffle_epi8(scaled_bottom_left, y_select);
  WriteSmoothDirectionalSum8(dst, top, weights_y, scaled_bottom_left_y, scale);
  dst += stride;
  y_select = _mm_set1_epi32(0x07060706);
  weights_y = _mm_shuffle_epi8(weights, y_select);
  scaled_bottom_left_y = _mm_shuffle_epi8(scaled_bottom_left, y_select);
  WriteSmoothDirectionalSum8(dst, top, weights_y, scaled_bottom_left_y, scale);
}

void SmoothVertical8x8_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                              const ptrdiff_t stride,
                              const void* LIBGAV1_RESTRICT const top_row,
                              const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  const __m128i bottom_left = _mm_set1_epi16(left_ptr[7]);
  const __m128i weights = _mm_cvtepu8_epi16(LoadLo8(kSmoothWeights + 4));
  __m128i scale = _mm_set1_epi16(256);
  const __m128i inverted_weights = _mm_sub_epi16(scale, weights);
  const __m128i scaled_bottom_left =
      _mm_mullo_epi16(inverted_weights, bottom_left);
  scale = _mm_set1_epi16(128);
  const __m128i top = _mm_cvtepu8_epi16(LoadLo8(top_row));
  auto* dst = static_cast<uint8_t*>(dest);
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left, y_select);
    WriteSmoothDirectionalSum8(dst, top, weights_y, scaled_bottom_left_y,
                               scale);
    dst += stride;
  }
}

void SmoothVertical8x16_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                               const ptrdiff_t stride,
                               const void* LIBGAV1_RESTRICT const top_row,
                               const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  const __m128i bottom_left = _mm_set1_epi16(left_ptr[15]);
  const __m128i weights = LoadUnaligned16(kSmoothWeights + 12);

  const __m128i weights1 = _mm_cvtepu8_epi16(weights);
  const __m128i weights2 = _mm_cvtepu8_epi16(_mm_srli_si128(weights, 8));
  __m128i scale = _mm_set1_epi16(256);
  const __m128i inverted_weights1 = _mm_sub_epi16(scale, weights1);
  const __m128i inverted_weights2 = _mm_sub_epi16(scale, weights2);
  const __m128i scaled_bottom_left1 =
      _mm_mullo_epi16(inverted_weights1, bottom_left);
  const __m128i scaled_bottom_left2 =
      _mm_mullo_epi16(inverted_weights2, bottom_left);
  scale = _mm_set1_epi16(128);
  const __m128i top = _mm_cvtepu8_epi16(LoadLo8(top_row));
  auto* dst = static_cast<uint8_t*>(dest);
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights1, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left1, y_select);
    WriteSmoothDirectionalSum8(dst, top, weights_y, scaled_bottom_left_y,
                               scale);
    dst += stride;
  }
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights2, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left2, y_select);
    WriteSmoothDirectionalSum8(dst, top, weights_y, scaled_bottom_left_y,
                               scale);
    dst += stride;
  }
}

void SmoothVertical8x32_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                               const ptrdiff_t stride,
                               const void* LIBGAV1_RESTRICT const top_row,
                               const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  const __m128i zero = _mm_setzero_si128();
  const __m128i bottom_left = _mm_set1_epi16(left_ptr[31]);
  const __m128i weights_lo = LoadUnaligned16(kSmoothWeights + 28);
  const __m128i weights_hi = LoadUnaligned16(kSmoothWeights + 44);
  const __m128i weights1 = _mm_cvtepu8_epi16(weights_lo);
  const __m128i weights2 = _mm_unpackhi_epi8(weights_lo, zero);
  const __m128i weights3 = _mm_cvtepu8_epi16(weights_hi);
  const __m128i weights4 = _mm_unpackhi_epi8(weights_hi, zero);
  __m128i scale = _mm_set1_epi16(256);
  const __m128i inverted_weights1 = _mm_sub_epi16(scale, weights1);
  const __m128i inverted_weights2 = _mm_sub_epi16(scale, weights2);
  const __m128i inverted_weights3 = _mm_sub_epi16(scale, weights3);
  const __m128i inverted_weights4 = _mm_sub_epi16(scale, weights4);
  const __m128i scaled_bottom_left1 =
      _mm_mullo_epi16(inverted_weights1, bottom_left);
  const __m128i scaled_bottom_left2 =
      _mm_mullo_epi16(inverted_weights2, bottom_left);
  const __m128i scaled_bottom_left3 =
      _mm_mullo_epi16(inverted_weights3, bottom_left);
  const __m128i scaled_bottom_left4 =
      _mm_mullo_epi16(inverted_weights4, bottom_left);
  scale = _mm_set1_epi16(128);
  auto* dst = static_cast<uint8_t*>(dest);
  const __m128i top = _mm_cvtepu8_epi16(LoadLo8(top_row));
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights1, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left1, y_select);
    WriteSmoothDirectionalSum8(dst, top, weights_y, scaled_bottom_left_y,
                               scale);
    dst += stride;
  }
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights2, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left2, y_select);
    WriteSmoothDirectionalSum8(dst, top, weights_y, scaled_bottom_left_y,
                               scale);
    dst += stride;
  }
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights3, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left3, y_select);
    WriteSmoothDirectionalSum8(dst, top, weights_y, scaled_bottom_left_y,
                               scale);
    dst += stride;
  }
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights4, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left4, y_select);
    WriteSmoothDirectionalSum8(dst, top, weights_y, scaled_bottom_left_y,
                               scale);
    dst += stride;
  }
}

void SmoothVertical16x4_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                               const ptrdiff_t stride,
                               const void* LIBGAV1_RESTRICT const top_row,
                               const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  auto* dst = static_cast<uint8_t*>(dest);
  const __m128i bottom_left = _mm_set1_epi16(left_ptr[3]);
  const __m128i weights = _mm_cvtepu8_epi16(Load4(kSmoothWeights));
  __m128i scale = _mm_set1_epi16(256);
  const __m128i inverted_weights = _mm_sub_epi16(scale, weights);
  const __m128i scaled_bottom_left =
      _mm_mullo_epi16(inverted_weights, bottom_left);
  scale = _mm_set1_epi16(128);
  const __m128i top = LoadUnaligned16(top_row);
  const __m128i top_lo = _mm_cvtepu8_epi16(top);
  const __m128i top_hi = _mm_cvtepu8_epi16(_mm_srli_si128(top, 8));

  __m128i y_select = _mm_set1_epi32(0x01000100);
  __m128i weights_y = _mm_shuffle_epi8(weights, y_select);
  __m128i scaled_bottom_left_y = _mm_shuffle_epi8(scaled_bottom_left, y_select);
  WriteSmoothDirectionalSum16(dst, top_lo, top_hi, weights_y, weights_y,
                              scaled_bottom_left_y, scaled_bottom_left_y,
                              scale);
  dst += stride;
  y_select = _mm_set1_epi32(0x03020302);
  weights_y = _mm_shuffle_epi8(weights, y_select);
  scaled_bottom_left_y = _mm_shuffle_epi8(scaled_bottom_left, y_select);
  WriteSmoothDirectionalSum16(dst, top_lo, top_hi, weights_y, weights_y,
                              scaled_bottom_left_y, scaled_bottom_left_y,
                              scale);
  dst += stride;
  y_select = _mm_set1_epi32(0x05040504);
  weights_y = _mm_shuffle_epi8(weights, y_select);
  scaled_bottom_left_y = _mm_shuffle_epi8(scaled_bottom_left, y_select);
  WriteSmoothDirectionalSum16(dst, top_lo, top_hi, weights_y, weights_y,
                              scaled_bottom_left_y, scaled_bottom_left_y,
                              scale);
  dst += stride;
  y_select = _mm_set1_epi32(0x07060706);
  weights_y = _mm_shuffle_epi8(weights, y_select);
  scaled_bottom_left_y = _mm_shuffle_epi8(scaled_bottom_left, y_select);
  WriteSmoothDirectionalSum16(dst, top_lo, top_hi, weights_y, weights_y,
                              scaled_bottom_left_y, scaled_bottom_left_y,
                              scale);
}

void SmoothVertical16x8_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                               const ptrdiff_t stride,
                               const void* LIBGAV1_RESTRICT const top_row,
                               const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  auto* dst = static_cast<uint8_t*>(dest);
  const __m128i bottom_left = _mm_set1_epi16(left_ptr[7]);
  const __m128i weights = _mm_cvtepu8_epi16(LoadLo8(kSmoothWeights + 4));
  __m128i scale = _mm_set1_epi16(256);
  const __m128i inverted_weights = _mm_sub_epi16(scale, weights);
  const __m128i scaled_bottom_left =
      _mm_mullo_epi16(inverted_weights, bottom_left);
  scale = _mm_set1_epi16(128);

  const __m128i top = LoadUnaligned16(top_row);
  const __m128i top_lo = _mm_cvtepu8_epi16(top);
  const __m128i top_hi = _mm_cvtepu8_epi16(_mm_srli_si128(top, 8));
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left, y_select);
    WriteSmoothDirectionalSum16(dst, top_lo, top_hi, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    dst += stride;
  }
}

void SmoothVertical16x16_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  auto* dst = static_cast<uint8_t*>(dest);
  const __m128i bottom_left = _mm_set1_epi16(left_ptr[15]);
  const __m128i zero = _mm_setzero_si128();
  __m128i scale = _mm_set1_epi16(256);
  const __m128i weights = LoadUnaligned16(kSmoothWeights + 12);
  const __m128i weights_lo = _mm_cvtepu8_epi16(weights);
  const __m128i weights_hi = _mm_unpackhi_epi8(weights, zero);
  const __m128i inverted_weights_lo = _mm_sub_epi16(scale, weights_lo);
  const __m128i inverted_weights_hi = _mm_sub_epi16(scale, weights_hi);
  const __m128i scaled_bottom_left_lo =
      _mm_mullo_epi16(inverted_weights_lo, bottom_left);
  const __m128i scaled_bottom_left_hi =
      _mm_mullo_epi16(inverted_weights_hi, bottom_left);
  scale = _mm_set1_epi16(128);

  const __m128i top = LoadUnaligned16(top_row);
  const __m128i top_lo = _mm_cvtepu8_epi16(top);
  const __m128i top_hi = _mm_unpackhi_epi8(top, zero);
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights_lo, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left_lo, y_select);
    WriteSmoothDirectionalSum16(dst, top_lo, top_hi, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    dst += stride;
  }
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights_hi, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left_hi, y_select);
    WriteSmoothDirectionalSum16(dst, top_lo, top_hi, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    dst += stride;
  }
}

void SmoothVertical16x32_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  auto* dst = static_cast<uint8_t*>(dest);
  const __m128i bottom_left = _mm_set1_epi16(left_ptr[31]);
  const __m128i weights_lo = LoadUnaligned16(kSmoothWeights + 28);
  const __m128i weights_hi = LoadUnaligned16(kSmoothWeights + 44);
  __m128i scale = _mm_set1_epi16(256);
  const __m128i zero = _mm_setzero_si128();
  const __m128i weights1 = _mm_cvtepu8_epi16(weights_lo);
  const __m128i weights2 = _mm_unpackhi_epi8(weights_lo, zero);
  const __m128i weights3 = _mm_cvtepu8_epi16(weights_hi);
  const __m128i weights4 = _mm_unpackhi_epi8(weights_hi, zero);
  const __m128i inverted_weights1 = _mm_sub_epi16(scale, weights1);
  const __m128i inverted_weights2 = _mm_sub_epi16(scale, weights2);
  const __m128i inverted_weights3 = _mm_sub_epi16(scale, weights3);
  const __m128i inverted_weights4 = _mm_sub_epi16(scale, weights4);
  const __m128i scaled_bottom_left1 =
      _mm_mullo_epi16(inverted_weights1, bottom_left);
  const __m128i scaled_bottom_left2 =
      _mm_mullo_epi16(inverted_weights2, bottom_left);
  const __m128i scaled_bottom_left3 =
      _mm_mullo_epi16(inverted_weights3, bottom_left);
  const __m128i scaled_bottom_left4 =
      _mm_mullo_epi16(inverted_weights4, bottom_left);
  scale = _mm_set1_epi16(128);

  const __m128i top = LoadUnaligned16(top_row);
  const __m128i top_lo = _mm_cvtepu8_epi16(top);
  const __m128i top_hi = _mm_unpackhi_epi8(top, zero);
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights1, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left1, y_select);
    WriteSmoothDirectionalSum16(dst, top_lo, top_hi, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    dst += stride;
  }
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights2, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left2, y_select);
    WriteSmoothDirectionalSum16(dst, top_lo, top_hi, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    dst += stride;
  }
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights3, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left3, y_select);
    WriteSmoothDirectionalSum16(dst, top_lo, top_hi, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    dst += stride;
  }
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights4, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left4, y_select);
    WriteSmoothDirectionalSum16(dst, top_lo, top_hi, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    dst += stride;
  }
}

void SmoothVertical16x64_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  auto* dst = static_cast<uint8_t*>(dest);
  const __m128i bottom_left = _mm_set1_epi16(left_ptr[63]);
  const __m128i scale = _mm_set1_epi16(256);
  const __m128i round = _mm_set1_epi16(128);
  const __m128i zero = _mm_setzero_si128();

  const __m128i top = LoadUnaligned16(top_row);
  const __m128i top_lo = _mm_cvtepu8_epi16(top);
  const __m128i top_hi = _mm_unpackhi_epi8(top, zero);
  const uint8_t* weights_base_ptr = kSmoothWeights + 60;
  for (int left_offset = 0; left_offset < 64; left_offset += 16) {
    const __m128i weights = LoadUnaligned16(weights_base_ptr + left_offset);
    const __m128i weights_lo = _mm_cvtepu8_epi16(weights);
    const __m128i weights_hi = _mm_unpackhi_epi8(weights, zero);
    const __m128i inverted_weights_lo = _mm_sub_epi16(scale, weights_lo);
    const __m128i inverted_weights_hi = _mm_sub_epi16(scale, weights_hi);
    const __m128i scaled_bottom_left_lo =
        _mm_mullo_epi16(inverted_weights_lo, bottom_left);
    const __m128i scaled_bottom_left_hi =
        _mm_mullo_epi16(inverted_weights_hi, bottom_left);

    for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
      const __m128i y_select = _mm_set1_epi32(y_mask);
      const __m128i weights_y = _mm_shuffle_epi8(weights_lo, y_select);
      const __m128i scaled_bottom_left_y =
          _mm_shuffle_epi8(scaled_bottom_left_lo, y_select);
      WriteSmoothDirectionalSum16(dst, top_lo, top_hi, weights_y, weights_y,
                                  scaled_bottom_left_y, scaled_bottom_left_y,
                                  round);
      dst += stride;
    }
    for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
      const __m128i y_select = _mm_set1_epi32(y_mask);
      const __m128i weights_y = _mm_shuffle_epi8(weights_hi, y_select);
      const __m128i scaled_bottom_left_y =
          _mm_shuffle_epi8(scaled_bottom_left_hi, y_select);
      WriteSmoothDirectionalSum16(dst, top_lo, top_hi, weights_y, weights_y,
                                  scaled_bottom_left_y, scaled_bottom_left_y,
                                  round);
      dst += stride;
    }
  }
}

void SmoothVertical32x8_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                               const ptrdiff_t stride,
                               const void* LIBGAV1_RESTRICT const top_row,
                               const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  auto* dst = static_cast<uint8_t*>(dest);
  const __m128i zero = _mm_setzero_si128();
  const __m128i bottom_left = _mm_set1_epi16(left_ptr[7]);
  const __m128i top_lo = LoadUnaligned16(top_ptr);
  const __m128i top_hi = LoadUnaligned16(top_ptr + 16);
  const __m128i top1 = _mm_cvtepu8_epi16(top_lo);
  const __m128i top2 = _mm_unpackhi_epi8(top_lo, zero);
  const __m128i top3 = _mm_cvtepu8_epi16(top_hi);
  const __m128i top4 = _mm_unpackhi_epi8(top_hi, zero);
  __m128i scale = _mm_set1_epi16(256);
  const __m128i weights = _mm_cvtepu8_epi16(LoadLo8(kSmoothWeights + 4));
  const __m128i inverted_weights = _mm_sub_epi16(scale, weights);
  const __m128i scaled_bottom_left =
      _mm_mullo_epi16(inverted_weights, bottom_left);
  scale = _mm_set1_epi16(128);
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left, y_select);
    WriteSmoothDirectionalSum16(dst, top1, top2, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 16, top3, top4, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    dst += stride;
  }
}

void SmoothVertical32x16_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  auto* dst = static_cast<uint8_t*>(dest);
  const __m128i zero = _mm_setzero_si128();
  const __m128i bottom_left = _mm_set1_epi16(left_ptr[15]);
  const __m128i top_lo = LoadUnaligned16(top_ptr);
  const __m128i top_hi = LoadUnaligned16(top_ptr + 16);
  const __m128i top1 = _mm_cvtepu8_epi16(top_lo);
  const __m128i top2 = _mm_unpackhi_epi8(top_lo, zero);
  const __m128i top3 = _mm_cvtepu8_epi16(top_hi);
  const __m128i top4 = _mm_unpackhi_epi8(top_hi, zero);
  const __m128i weights = LoadUnaligned16(kSmoothWeights + 12);
  const __m128i weights1 = _mm_cvtepu8_epi16(weights);
  const __m128i weights2 = _mm_unpackhi_epi8(weights, zero);
  __m128i scale = _mm_set1_epi16(256);
  const __m128i inverted_weights1 = _mm_sub_epi16(scale, weights1);
  const __m128i inverted_weights2 = _mm_sub_epi16(scale, weights2);
  const __m128i scaled_bottom_left1 =
      _mm_mullo_epi16(inverted_weights1, bottom_left);
  const __m128i scaled_bottom_left2 =
      _mm_mullo_epi16(inverted_weights2, bottom_left);
  scale = _mm_set1_epi16(128);
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights1, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left1, y_select);
    WriteSmoothDirectionalSum16(dst, top1, top2, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 16, top3, top4, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    dst += stride;
  }
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights2, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left2, y_select);
    WriteSmoothDirectionalSum16(dst, top1, top2, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 16, top3, top4, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    dst += stride;
  }
}

void SmoothVertical32x32_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  auto* dst = static_cast<uint8_t*>(dest);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const __m128i bottom_left = _mm_set1_epi16(left_ptr[31]);
  const __m128i weights_lo = LoadUnaligned16(kSmoothWeights + 28);
  const __m128i weights_hi = LoadUnaligned16(kSmoothWeights + 44);
  const __m128i zero = _mm_setzero_si128();
  __m128i scale = _mm_set1_epi16(256);
  const __m128i top_lo = LoadUnaligned16(top_ptr);
  const __m128i top_hi = LoadUnaligned16(top_ptr + 16);
  const __m128i top1 = _mm_cvtepu8_epi16(top_lo);
  const __m128i top2 = _mm_unpackhi_epi8(top_lo, zero);
  const __m128i top3 = _mm_cvtepu8_epi16(top_hi);
  const __m128i top4 = _mm_unpackhi_epi8(top_hi, zero);
  const __m128i weights1 = _mm_cvtepu8_epi16(weights_lo);
  const __m128i weights2 = _mm_unpackhi_epi8(weights_lo, zero);
  const __m128i weights3 = _mm_cvtepu8_epi16(weights_hi);
  const __m128i weights4 = _mm_unpackhi_epi8(weights_hi, zero);
  const __m128i inverted_weights1 = _mm_sub_epi16(scale, weights1);
  const __m128i inverted_weights2 = _mm_sub_epi16(scale, weights2);
  const __m128i inverted_weights3 = _mm_sub_epi16(scale, weights3);
  const __m128i inverted_weights4 = _mm_sub_epi16(scale, weights4);
  const __m128i scaled_bottom_left1 =
      _mm_mullo_epi16(inverted_weights1, bottom_left);
  const __m128i scaled_bottom_left2 =
      _mm_mullo_epi16(inverted_weights2, bottom_left);
  const __m128i scaled_bottom_left3 =
      _mm_mullo_epi16(inverted_weights3, bottom_left);
  const __m128i scaled_bottom_left4 =
      _mm_mullo_epi16(inverted_weights4, bottom_left);
  scale = _mm_set1_epi16(128);
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights1, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left1, y_select);
    WriteSmoothDirectionalSum16(dst, top1, top2, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 16, top3, top4, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    dst += stride;
  }
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights2, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left2, y_select);
    WriteSmoothDirectionalSum16(dst, top1, top2, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 16, top3, top4, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    dst += stride;
  }
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights3, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left3, y_select);
    WriteSmoothDirectionalSum16(dst, top1, top2, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 16, top3, top4, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    dst += stride;
  }
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights4, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left4, y_select);
    WriteSmoothDirectionalSum16(dst, top1, top2, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 16, top3, top4, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    dst += stride;
  }
}

void SmoothVertical32x64_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  auto* dst = static_cast<uint8_t*>(dest);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const __m128i zero = _mm_setzero_si128();
  const __m128i bottom_left = _mm_set1_epi16(left_ptr[63]);
  const __m128i top_lo = LoadUnaligned16(top_ptr);
  const __m128i top_hi = LoadUnaligned16(top_ptr + 16);
  const __m128i top1 = _mm_cvtepu8_epi16(top_lo);
  const __m128i top2 = _mm_unpackhi_epi8(top_lo, zero);
  const __m128i top3 = _mm_cvtepu8_epi16(top_hi);
  const __m128i top4 = _mm_unpackhi_epi8(top_hi, zero);
  const __m128i scale = _mm_set1_epi16(256);
  const __m128i round = _mm_set1_epi16(128);
  const uint8_t* weights_base_ptr = kSmoothWeights + 60;
  for (int left_offset = 0; left_offset < 64; left_offset += 16) {
    const __m128i weights = LoadUnaligned16(weights_base_ptr + left_offset);
    const __m128i weights_lo = _mm_cvtepu8_epi16(weights);
    const __m128i weights_hi = _mm_unpackhi_epi8(weights, zero);
    const __m128i inverted_weights_lo = _mm_sub_epi16(scale, weights_lo);
    const __m128i inverted_weights_hi = _mm_sub_epi16(scale, weights_hi);
    const __m128i scaled_bottom_left_lo =
        _mm_mullo_epi16(inverted_weights_lo, bottom_left);
    const __m128i scaled_bottom_left_hi =
        _mm_mullo_epi16(inverted_weights_hi, bottom_left);

    for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
      const __m128i y_select = _mm_set1_epi32(y_mask);
      const __m128i weights_y = _mm_shuffle_epi8(weights_lo, y_select);
      const __m128i scaled_bottom_left_y =
          _mm_shuffle_epi8(scaled_bottom_left_lo, y_select);
      WriteSmoothDirectionalSum16(dst, top1, top2, weights_y, weights_y,
                                  scaled_bottom_left_y, scaled_bottom_left_y,
                                  round);
      WriteSmoothDirectionalSum16(dst + 16, top3, top4, weights_y, weights_y,
                                  scaled_bottom_left_y, scaled_bottom_left_y,
                                  round);
      dst += stride;
    }
    for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
      const __m128i y_select = _mm_set1_epi32(y_mask);
      const __m128i weights_y = _mm_shuffle_epi8(weights_hi, y_select);
      const __m128i scaled_bottom_left_y =
          _mm_shuffle_epi8(scaled_bottom_left_hi, y_select);
      WriteSmoothDirectionalSum16(dst, top1, top2, weights_y, weights_y,
                                  scaled_bottom_left_y, scaled_bottom_left_y,
                                  round);
      WriteSmoothDirectionalSum16(dst + 16, top3, top4, weights_y, weights_y,
                                  scaled_bottom_left_y, scaled_bottom_left_y,
                                  round);
      dst += stride;
    }
  }
}

void SmoothVertical64x16_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  auto* dst = static_cast<uint8_t*>(dest);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const __m128i bottom_left = _mm_set1_epi16(left_ptr[15]);
  __m128i scale = _mm_set1_epi16(256);
  const __m128i zero = _mm_setzero_si128();
  const __m128i top_lolo = LoadUnaligned16(top_ptr);
  const __m128i top_lohi = LoadUnaligned16(top_ptr + 16);
  const __m128i top1 = _mm_cvtepu8_epi16(top_lolo);
  const __m128i top2 = _mm_unpackhi_epi8(top_lolo, zero);
  const __m128i top3 = _mm_cvtepu8_epi16(top_lohi);
  const __m128i top4 = _mm_unpackhi_epi8(top_lohi, zero);

  const __m128i weights = LoadUnaligned16(kSmoothWeights + 12);
  const __m128i weights1 = _mm_cvtepu8_epi16(weights);
  const __m128i weights2 = _mm_unpackhi_epi8(weights, zero);
  const __m128i inverted_weights1 = _mm_sub_epi16(scale, weights1);
  const __m128i inverted_weights2 = _mm_sub_epi16(scale, weights2);
  const __m128i top_hilo = LoadUnaligned16(top_ptr + 32);
  const __m128i top_hihi = LoadUnaligned16(top_ptr + 48);
  const __m128i top5 = _mm_cvtepu8_epi16(top_hilo);
  const __m128i top6 = _mm_unpackhi_epi8(top_hilo, zero);
  const __m128i top7 = _mm_cvtepu8_epi16(top_hihi);
  const __m128i top8 = _mm_unpackhi_epi8(top_hihi, zero);
  const __m128i scaled_bottom_left1 =
      _mm_mullo_epi16(inverted_weights1, bottom_left);
  const __m128i scaled_bottom_left2 =
      _mm_mullo_epi16(inverted_weights2, bottom_left);
  scale = _mm_set1_epi16(128);
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights1, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left1, y_select);
    WriteSmoothDirectionalSum16(dst, top1, top2, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 16, top3, top4, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 32, top5, top6, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 48, top7, top8, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    dst += stride;
  }
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights2, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left2, y_select);
    WriteSmoothDirectionalSum16(dst, top1, top2, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 16, top3, top4, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 32, top5, top6, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 48, top7, top8, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    dst += stride;
  }
}

void SmoothVertical64x32_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  auto* dst = static_cast<uint8_t*>(dest);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const __m128i zero = _mm_setzero_si128();
  const __m128i bottom_left = _mm_set1_epi16(left_ptr[31]);
  const __m128i top_lolo = LoadUnaligned16(top_ptr);
  const __m128i top_lohi = LoadUnaligned16(top_ptr + 16);
  const __m128i top1 = _mm_cvtepu8_epi16(top_lolo);
  const __m128i top2 = _mm_unpackhi_epi8(top_lolo, zero);
  const __m128i top3 = _mm_cvtepu8_epi16(top_lohi);
  const __m128i top4 = _mm_unpackhi_epi8(top_lohi, zero);
  const __m128i top_hilo = LoadUnaligned16(top_ptr + 32);
  const __m128i top_hihi = LoadUnaligned16(top_ptr + 48);
  const __m128i top5 = _mm_cvtepu8_epi16(top_hilo);
  const __m128i top6 = _mm_unpackhi_epi8(top_hilo, zero);
  const __m128i top7 = _mm_cvtepu8_epi16(top_hihi);
  const __m128i top8 = _mm_unpackhi_epi8(top_hihi, zero);
  const __m128i weights_lo = LoadUnaligned16(kSmoothWeights + 28);
  const __m128i weights_hi = LoadUnaligned16(kSmoothWeights + 44);
  const __m128i weights1 = _mm_cvtepu8_epi16(weights_lo);
  const __m128i weights2 = _mm_unpackhi_epi8(weights_lo, zero);
  const __m128i weights3 = _mm_cvtepu8_epi16(weights_hi);
  const __m128i weights4 = _mm_unpackhi_epi8(weights_hi, zero);
  __m128i scale = _mm_set1_epi16(256);
  const __m128i inverted_weights1 = _mm_sub_epi16(scale, weights1);
  const __m128i inverted_weights2 = _mm_sub_epi16(scale, weights2);
  const __m128i inverted_weights3 = _mm_sub_epi16(scale, weights3);
  const __m128i inverted_weights4 = _mm_sub_epi16(scale, weights4);
  const __m128i scaled_bottom_left1 =
      _mm_mullo_epi16(inverted_weights1, bottom_left);
  const __m128i scaled_bottom_left2 =
      _mm_mullo_epi16(inverted_weights2, bottom_left);
  const __m128i scaled_bottom_left3 =
      _mm_mullo_epi16(inverted_weights3, bottom_left);
  const __m128i scaled_bottom_left4 =
      _mm_mullo_epi16(inverted_weights4, bottom_left);
  scale = _mm_set1_epi16(128);

  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights1, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left1, y_select);
    WriteSmoothDirectionalSum16(dst, top1, top2, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 16, top3, top4, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 32, top5, top6, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 48, top7, top8, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    dst += stride;
  }
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights2, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left2, y_select);
    WriteSmoothDirectionalSum16(dst, top1, top2, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 16, top3, top4, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 32, top5, top6, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 48, top7, top8, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    dst += stride;
  }
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights3, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left3, y_select);
    WriteSmoothDirectionalSum16(dst, top1, top2, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 16, top3, top4, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 32, top5, top6, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 48, top7, top8, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    dst += stride;
  }
  for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
    const __m128i y_select = _mm_set1_epi32(y_mask);
    const __m128i weights_y = _mm_shuffle_epi8(weights4, y_select);
    const __m128i scaled_bottom_left_y =
        _mm_shuffle_epi8(scaled_bottom_left4, y_select);
    WriteSmoothDirectionalSum16(dst, top1, top2, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 16, top3, top4, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 32, top5, top6, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    WriteSmoothDirectionalSum16(dst + 48, top7, top8, weights_y, weights_y,
                                scaled_bottom_left_y, scaled_bottom_left_y,
                                scale);
    dst += stride;
  }
}

void SmoothVertical64x64_SSE4_1(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  auto* dst = static_cast<uint8_t*>(dest);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const __m128i zero = _mm_setzero_si128();
  const __m128i bottom_left = _mm_set1_epi16(left_ptr[63]);
  const __m128i top_lolo = LoadUnaligned16(top_ptr);
  const __m128i top_lohi = LoadUnaligned16(top_ptr + 16);
  const __m128i top1 = _mm_cvtepu8_epi16(top_lolo);
  const __m128i top2 = _mm_unpackhi_epi8(top_lolo, zero);
  const __m128i top3 = _mm_cvtepu8_epi16(top_lohi);
  const __m128i top4 = _mm_unpackhi_epi8(top_lohi, zero);
  const __m128i top_hilo = LoadUnaligned16(top_ptr + 32);
  const __m128i top_hihi = LoadUnaligned16(top_ptr + 48);
  const __m128i top5 = _mm_cvtepu8_epi16(top_hilo);
  const __m128i top6 = _mm_unpackhi_epi8(top_hilo, zero);
  const __m128i top7 = _mm_cvtepu8_epi16(top_hihi);
  const __m128i top8 = _mm_unpackhi_epi8(top_hihi, zero);
  const __m128i scale = _mm_set1_epi16(256);
  const __m128i round = _mm_set1_epi16(128);
  const uint8_t* weights_base_ptr = kSmoothWeights + 60;
  for (int left_offset = 0; left_offset < 64; left_offset += 16) {
    const __m128i weights = LoadUnaligned16(weights_base_ptr + left_offset);
    const __m128i weights_lo = _mm_cvtepu8_epi16(weights);
    const __m128i weights_hi = _mm_unpackhi_epi8(weights, zero);
    const __m128i inverted_weights_lo = _mm_sub_epi16(scale, weights_lo);
    const __m128i inverted_weights_hi = _mm_sub_epi16(scale, weights_hi);
    const __m128i scaled_bottom_left_lo =
        _mm_mullo_epi16(inverted_weights_lo, bottom_left);
    const __m128i scaled_bottom_left_hi =
        _mm_mullo_epi16(inverted_weights_hi, bottom_left);
    for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
      const __m128i y_select = _mm_set1_epi32(y_mask);
      const __m128i weights_y = _mm_shuffle_epi8(weights_lo, y_select);
      const __m128i scaled_bottom_left_y =
          _mm_shuffle_epi8(scaled_bottom_left_lo, y_select);
      WriteSmoothDirectionalSum16(dst, top1, top2, weights_y, weights_y,
                                  scaled_bottom_left_y, scaled_bottom_left_y,
                                  round);
      WriteSmoothDirectionalSum16(dst + 16, top3, top4, weights_y, weights_y,
                                  scaled_bottom_left_y, scaled_bottom_left_y,
                                  round);
      WriteSmoothDirectionalSum16(dst + 32, top5, top6, weights_y, weights_y,
                                  scaled_bottom_left_y, scaled_bottom_left_y,
                                  round);
      WriteSmoothDirectionalSum16(dst + 48, top7, top8, weights_y, weights_y,
                                  scaled_bottom_left_y, scaled_bottom_left_y,
                                  round);
      dst += stride;
    }
    for (int y_mask = 0x01000100; y_mask < 0x0F0E0F0F; y_mask += 0x02020202) {
      const __m128i y_select = _mm_set1_epi32(y_mask);
      const __m128i weights_y = _mm_shuffle_epi8(weights_hi, y_select);
      const __m128i scaled_bottom_left_y =
          _mm_shuffle_epi8(scaled_bottom_left_hi, y_select);
      WriteSmoothDirectionalSum16(dst, top1, top2, weights_y, weights_y,
                                  scaled_bottom_left_y, scaled_bottom_left_y,
                                  round);
      WriteSmoothDirectionalSum16(dst + 16, top3, top4, weights_y, weights_y,
                                  scaled_bottom_left_y, scaled_bottom_left_y,
                                  round);
      WriteSmoothDirectionalSum16(dst + 32, top5, top6, weights_y, weights_y,
                                  scaled_bottom_left_y, scaled_bottom_left_y,
                                  round);
      WriteSmoothDirectionalSum16(dst + 48, top7, top8, weights_y, weights_y,
                                  scaled_bottom_left_y, scaled_bottom_left_y,
                                  round);
      dst += stride;
    }
  }
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x4_IntraPredictorSmooth)
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorSmooth] =
      Smooth4x4_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x8_IntraPredictorSmooth)
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorSmooth] =
      Smooth4x8_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x16_IntraPredictorSmooth)
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorSmooth] =
      Smooth4x16_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x4_IntraPredictorSmooth)
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorSmooth] =
      Smooth8x4_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x8_IntraPredictorSmooth)
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorSmooth] =
      Smooth8x8_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x16_IntraPredictorSmooth)
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorSmooth] =
      Smooth8x16_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x32_IntraPredictorSmooth)
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorSmooth] =
      Smooth8x32_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x4_IntraPredictorSmooth)
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorSmooth] =
      SmoothWxH<16, 4>;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x8_IntraPredictorSmooth)
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorSmooth] =
      SmoothWxH<16, 8>;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x16_IntraPredictorSmooth)
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorSmooth] =
      SmoothWxH<16, 16>;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x32_IntraPredictorSmooth)
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorSmooth] =
      SmoothWxH<16, 32>;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x64_IntraPredictorSmooth)
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorSmooth] =
      SmoothWxH<16, 64>;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x8_IntraPredictorSmooth)
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorSmooth] =
      SmoothWxH<32, 8>;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x16_IntraPredictorSmooth)
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorSmooth] =
      SmoothWxH<32, 16>;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x32_IntraPredictorSmooth)
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorSmooth] =
      SmoothWxH<32, 32>;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x64_IntraPredictorSmooth)
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorSmooth] =
      SmoothWxH<32, 64>;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x16_IntraPredictorSmooth)
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorSmooth] =
      SmoothWxH<64, 16>;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x32_IntraPredictorSmooth)
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorSmooth] =
      SmoothWxH<64, 32>;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x64_IntraPredictorSmooth)
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorSmooth] =
      SmoothWxH<64, 64>;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x4_IntraPredictorSmoothVertical)
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorSmoothVertical] =
      SmoothVertical4x4_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x8_IntraPredictorSmoothVertical)
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorSmoothVertical] =
      SmoothVertical4x8_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x16_IntraPredictorSmoothVertical)
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorSmoothVertical] =
      SmoothVertical4x16_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x4_IntraPredictorSmoothVertical)
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorSmoothVertical] =
      SmoothVertical8x4_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x8_IntraPredictorSmoothVertical)
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorSmoothVertical] =
      SmoothVertical8x8_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x16_IntraPredictorSmoothVertical)
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorSmoothVertical] =
      SmoothVertical8x16_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x32_IntraPredictorSmoothVertical)
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorSmoothVertical] =
      SmoothVertical8x32_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x4_IntraPredictorSmoothVertical)
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorSmoothVertical] =
      SmoothVertical16x4_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x8_IntraPredictorSmoothVertical)
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorSmoothVertical] =
      SmoothVertical16x8_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x16_IntraPredictorSmoothVertical)
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorSmoothVertical] =
      SmoothVertical16x16_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x32_IntraPredictorSmoothVertical)
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorSmoothVertical] =
      SmoothVertical16x32_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x64_IntraPredictorSmoothVertical)
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorSmoothVertical] =
      SmoothVertical16x64_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x8_IntraPredictorSmoothVertical)
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorSmoothVertical] =
      SmoothVertical32x8_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x16_IntraPredictorSmoothVertical)
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorSmoothVertical] =
      SmoothVertical32x16_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x32_IntraPredictorSmoothVertical)
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorSmoothVertical] =
      SmoothVertical32x32_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x64_IntraPredictorSmoothVertical)
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorSmoothVertical] =
      SmoothVertical32x64_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x16_IntraPredictorSmoothVertical)
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorSmoothVertical] =
      SmoothVertical64x16_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x32_IntraPredictorSmoothVertical)
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorSmoothVertical] =
      SmoothVertical64x32_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x64_IntraPredictorSmoothVertical)
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorSmoothVertical] =
      SmoothVertical64x64_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x4_IntraPredictorSmoothHorizontal)
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal4x4_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x8_IntraPredictorSmoothHorizontal)
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal4x8_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x16_IntraPredictorSmoothHorizontal)
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal4x16_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x4_IntraPredictorSmoothHorizontal)
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal8x4_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x8_IntraPredictorSmoothHorizontal)
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal8x8_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x16_IntraPredictorSmoothHorizontal)
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal8x16_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x32_IntraPredictorSmoothHorizontal)
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal8x32_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x4_IntraPredictorSmoothHorizontal)
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal16x4_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x8_IntraPredictorSmoothHorizontal)
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal16x8_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x16_IntraPredictorSmoothHorizontal)
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal16x16_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x32_IntraPredictorSmoothHorizontal)
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal16x32_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x64_IntraPredictorSmoothHorizontal)
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal16x64_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x8_IntraPredictorSmoothHorizontal)
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal32x8_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x16_IntraPredictorSmoothHorizontal)
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal32x16_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x32_IntraPredictorSmoothHorizontal)
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal32x32_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x64_IntraPredictorSmoothHorizontal)
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal32x64_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x16_IntraPredictorSmoothHorizontal)
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal64x16_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x32_IntraPredictorSmoothHorizontal)
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal64x32_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x64_IntraPredictorSmoothHorizontal)
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal64x64_SSE4_1;
#endif
}

}  // namespace
}  // namespace low_bitdepth

void IntraPredSmoothInit_SSE4_1() { low_bitdepth::Init8bpp(); }

}  // namespace dsp
}  // namespace libgav1

#else  // !LIBGAV1_TARGETING_SSE4_1

namespace libgav1 {
namespace dsp {

void IntraPredSmoothInit_SSE4_1() {}

}  // namespace dsp
}  // namespace libgav1

#endif  // LIBGAV1_TARGETING_SSE4_1
