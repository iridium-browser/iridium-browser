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

#include "src/dsp/weight_mask.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

#include "src/dsp/dsp.h"
#include "src/utils/common.h"

namespace libgav1 {
namespace dsp {
namespace {

template <int width, int height, int bitdepth, bool mask_is_inverse>
void WeightMask_C(const void* LIBGAV1_RESTRICT prediction_0,
                  const void* LIBGAV1_RESTRICT prediction_1,
                  uint8_t* LIBGAV1_RESTRICT mask, ptrdiff_t mask_stride) {
  using PredType =
      typename std::conditional<bitdepth == 8, int16_t, uint16_t>::type;
  const auto* pred_0 = static_cast<const PredType*>(prediction_0);
  const auto* pred_1 = static_cast<const PredType*>(prediction_1);
  static_assert(width >= 8, "");
  static_assert(height >= 8, "");
  constexpr int rounding_bits = bitdepth - 8 + ((bitdepth == 12) ? 2 : 4);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const int difference = RightShiftWithRounding(
          std::abs(pred_0[x] - pred_1[x]), rounding_bits);
      const auto mask_value =
          static_cast<uint8_t>(std::min(DivideBy16(difference) + 38, 64));
      mask[x] = mask_is_inverse ? 64 - mask_value : mask_value;
    }
    pred_0 += width;
    pred_1 += width;
    mask += mask_stride;
  }
}

#define INIT_WEIGHT_MASK(width, height, bitdepth, w_index, h_index) \
  dsp->weight_mask[w_index][h_index][0] =                           \
      WeightMask_C<width, height, bitdepth, 0>;                     \
  dsp->weight_mask[w_index][h_index][1] =                           \
      WeightMask_C<width, height, bitdepth, 1>

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(8);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  INIT_WEIGHT_MASK(8, 8, 8, 0, 0);
  INIT_WEIGHT_MASK(8, 16, 8, 0, 1);
  INIT_WEIGHT_MASK(8, 32, 8, 0, 2);
  INIT_WEIGHT_MASK(16, 8, 8, 1, 0);
  INIT_WEIGHT_MASK(16, 16, 8, 1, 1);
  INIT_WEIGHT_MASK(16, 32, 8, 1, 2);
  INIT_WEIGHT_MASK(16, 64, 8, 1, 3);
  INIT_WEIGHT_MASK(32, 8, 8, 2, 0);
  INIT_WEIGHT_MASK(32, 16, 8, 2, 1);
  INIT_WEIGHT_MASK(32, 32, 8, 2, 2);
  INIT_WEIGHT_MASK(32, 64, 8, 2, 3);
  INIT_WEIGHT_MASK(64, 16, 8, 3, 1);
  INIT_WEIGHT_MASK(64, 32, 8, 3, 2);
  INIT_WEIGHT_MASK(64, 64, 8, 3, 3);
  INIT_WEIGHT_MASK(64, 128, 8, 3, 4);
  INIT_WEIGHT_MASK(128, 64, 8, 4, 3);
  INIT_WEIGHT_MASK(128, 128, 8, 4, 4);
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp8bpp_WeightMask_8x8
  INIT_WEIGHT_MASK(8, 8, 8, 0, 0);
#endif
#ifndef LIBGAV1_Dsp8bpp_WeightMask_8x16
  INIT_WEIGHT_MASK(8, 16, 8, 0, 1);
#endif
#ifndef LIBGAV1_Dsp8bpp_WeightMask_8x32
  INIT_WEIGHT_MASK(8, 32, 8, 0, 2);
#endif
#ifndef LIBGAV1_Dsp8bpp_WeightMask_16x8
  INIT_WEIGHT_MASK(16, 8, 8, 1, 0);
#endif
#ifndef LIBGAV1_Dsp8bpp_WeightMask_16x16
  INIT_WEIGHT_MASK(16, 16, 8, 1, 1);
#endif
#ifndef LIBGAV1_Dsp8bpp_WeightMask_16x32
  INIT_WEIGHT_MASK(16, 32, 8, 1, 2);
#endif
#ifndef LIBGAV1_Dsp8bpp_WeightMask_16x64
  INIT_WEIGHT_MASK(16, 64, 8, 1, 3);
#endif
#ifndef LIBGAV1_Dsp8bpp_WeightMask_32x8
  INIT_WEIGHT_MASK(32, 8, 8, 2, 0);
#endif
#ifndef LIBGAV1_Dsp8bpp_WeightMask_32x16
  INIT_WEIGHT_MASK(32, 16, 8, 2, 1);
#endif
#ifndef LIBGAV1_Dsp8bpp_WeightMask_32x32
  INIT_WEIGHT_MASK(32, 32, 8, 2, 2);
#endif
#ifndef LIBGAV1_Dsp8bpp_WeightMask_32x64
  INIT_WEIGHT_MASK(32, 64, 8, 2, 3);
#endif
#ifndef LIBGAV1_Dsp8bpp_WeightMask_64x16
  INIT_WEIGHT_MASK(64, 16, 8, 3, 1);
#endif
#ifndef LIBGAV1_Dsp8bpp_WeightMask_64x32
  INIT_WEIGHT_MASK(64, 32, 8, 3, 2);
#endif
#ifndef LIBGAV1_Dsp8bpp_WeightMask_64x64
  INIT_WEIGHT_MASK(64, 64, 8, 3, 3);
#endif
#ifndef LIBGAV1_Dsp8bpp_WeightMask_64x128
  INIT_WEIGHT_MASK(64, 128, 8, 3, 4);
#endif
#ifndef LIBGAV1_Dsp8bpp_WeightMask_128x64
  INIT_WEIGHT_MASK(128, 64, 8, 4, 3);
#endif
#ifndef LIBGAV1_Dsp8bpp_WeightMask_128x128
  INIT_WEIGHT_MASK(128, 128, 8, 4, 4);
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}

#if LIBGAV1_MAX_BITDEPTH >= 10
void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(10);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  INIT_WEIGHT_MASK(8, 8, 10, 0, 0);
  INIT_WEIGHT_MASK(8, 16, 10, 0, 1);
  INIT_WEIGHT_MASK(8, 32, 10, 0, 2);
  INIT_WEIGHT_MASK(16, 8, 10, 1, 0);
  INIT_WEIGHT_MASK(16, 16, 10, 1, 1);
  INIT_WEIGHT_MASK(16, 32, 10, 1, 2);
  INIT_WEIGHT_MASK(16, 64, 10, 1, 3);
  INIT_WEIGHT_MASK(32, 8, 10, 2, 0);
  INIT_WEIGHT_MASK(32, 16, 10, 2, 1);
  INIT_WEIGHT_MASK(32, 32, 10, 2, 2);
  INIT_WEIGHT_MASK(32, 64, 10, 2, 3);
  INIT_WEIGHT_MASK(64, 16, 10, 3, 1);
  INIT_WEIGHT_MASK(64, 32, 10, 3, 2);
  INIT_WEIGHT_MASK(64, 64, 10, 3, 3);
  INIT_WEIGHT_MASK(64, 128, 10, 3, 4);
  INIT_WEIGHT_MASK(128, 64, 10, 4, 3);
  INIT_WEIGHT_MASK(128, 128, 10, 4, 4);
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp10bpp_WeightMask_8x8
  INIT_WEIGHT_MASK(8, 8, 10, 0, 0);
#endif
#ifndef LIBGAV1_Dsp10bpp_WeightMask_8x16
  INIT_WEIGHT_MASK(8, 16, 10, 0, 1);
#endif
#ifndef LIBGAV1_Dsp10bpp_WeightMask_8x32
  INIT_WEIGHT_MASK(8, 32, 10, 0, 2);
#endif
#ifndef LIBGAV1_Dsp10bpp_WeightMask_16x8
  INIT_WEIGHT_MASK(16, 8, 10, 1, 0);
#endif
#ifndef LIBGAV1_Dsp10bpp_WeightMask_16x16
  INIT_WEIGHT_MASK(16, 16, 10, 1, 1);
#endif
#ifndef LIBGAV1_Dsp10bpp_WeightMask_16x32
  INIT_WEIGHT_MASK(16, 32, 10, 1, 2);
#endif
#ifndef LIBGAV1_Dsp10bpp_WeightMask_16x64
  INIT_WEIGHT_MASK(16, 64, 10, 1, 3);
#endif
#ifndef LIBGAV1_Dsp10bpp_WeightMask_32x8
  INIT_WEIGHT_MASK(32, 8, 10, 2, 0);
#endif
#ifndef LIBGAV1_Dsp10bpp_WeightMask_32x16
  INIT_WEIGHT_MASK(32, 16, 10, 2, 1);
#endif
#ifndef LIBGAV1_Dsp10bpp_WeightMask_32x32
  INIT_WEIGHT_MASK(32, 32, 10, 2, 2);
#endif
#ifndef LIBGAV1_Dsp10bpp_WeightMask_32x64
  INIT_WEIGHT_MASK(32, 64, 10, 2, 3);
#endif
#ifndef LIBGAV1_Dsp10bpp_WeightMask_64x16
  INIT_WEIGHT_MASK(64, 16, 10, 3, 1);
#endif
#ifndef LIBGAV1_Dsp10bpp_WeightMask_64x32
  INIT_WEIGHT_MASK(64, 32, 10, 3, 2);
#endif
#ifndef LIBGAV1_Dsp10bpp_WeightMask_64x64
  INIT_WEIGHT_MASK(64, 64, 10, 3, 3);
#endif
#ifndef LIBGAV1_Dsp10bpp_WeightMask_64x128
  INIT_WEIGHT_MASK(64, 128, 10, 3, 4);
#endif
#ifndef LIBGAV1_Dsp10bpp_WeightMask_128x64
  INIT_WEIGHT_MASK(128, 64, 10, 4, 3);
#endif
#ifndef LIBGAV1_Dsp10bpp_WeightMask_128x128
  INIT_WEIGHT_MASK(128, 128, 10, 4, 4);
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
void Init12bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(12);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  INIT_WEIGHT_MASK(8, 8, 12, 0, 0);
  INIT_WEIGHT_MASK(8, 16, 12, 0, 1);
  INIT_WEIGHT_MASK(8, 32, 12, 0, 2);
  INIT_WEIGHT_MASK(16, 8, 12, 1, 0);
  INIT_WEIGHT_MASK(16, 16, 12, 1, 1);
  INIT_WEIGHT_MASK(16, 32, 12, 1, 2);
  INIT_WEIGHT_MASK(16, 64, 12, 1, 3);
  INIT_WEIGHT_MASK(32, 8, 12, 2, 0);
  INIT_WEIGHT_MASK(32, 16, 12, 2, 1);
  INIT_WEIGHT_MASK(32, 32, 12, 2, 2);
  INIT_WEIGHT_MASK(32, 64, 12, 2, 3);
  INIT_WEIGHT_MASK(64, 16, 12, 3, 1);
  INIT_WEIGHT_MASK(64, 32, 12, 3, 2);
  INIT_WEIGHT_MASK(64, 64, 12, 3, 3);
  INIT_WEIGHT_MASK(64, 128, 12, 3, 4);
  INIT_WEIGHT_MASK(128, 64, 12, 4, 3);
  INIT_WEIGHT_MASK(128, 128, 12, 4, 4);
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp12bpp_WeightMask_8x8
  INIT_WEIGHT_MASK(8, 8, 12, 0, 0);
#endif
#ifndef LIBGAV1_Dsp12bpp_WeightMask_8x16
  INIT_WEIGHT_MASK(8, 16, 12, 0, 1);
#endif
#ifndef LIBGAV1_Dsp12bpp_WeightMask_8x32
  INIT_WEIGHT_MASK(8, 32, 12, 0, 2);
#endif
#ifndef LIBGAV1_Dsp12bpp_WeightMask_16x8
  INIT_WEIGHT_MASK(16, 8, 12, 1, 0);
#endif
#ifndef LIBGAV1_Dsp12bpp_WeightMask_16x16
  INIT_WEIGHT_MASK(16, 16, 12, 1, 1);
#endif
#ifndef LIBGAV1_Dsp12bpp_WeightMask_16x32
  INIT_WEIGHT_MASK(16, 32, 12, 1, 2);
#endif
#ifndef LIBGAV1_Dsp12bpp_WeightMask_16x64
  INIT_WEIGHT_MASK(16, 64, 12, 1, 3);
#endif
#ifndef LIBGAV1_Dsp12bpp_WeightMask_32x8
  INIT_WEIGHT_MASK(32, 8, 12, 2, 0);
#endif
#ifndef LIBGAV1_Dsp12bpp_WeightMask_32x16
  INIT_WEIGHT_MASK(32, 16, 12, 2, 1);
#endif
#ifndef LIBGAV1_Dsp12bpp_WeightMask_32x32
  INIT_WEIGHT_MASK(32, 32, 12, 2, 2);
#endif
#ifndef LIBGAV1_Dsp12bpp_WeightMask_32x64
  INIT_WEIGHT_MASK(32, 64, 12, 2, 3);
#endif
#ifndef LIBGAV1_Dsp12bpp_WeightMask_64x16
  INIT_WEIGHT_MASK(64, 16, 12, 3, 1);
#endif
#ifndef LIBGAV1_Dsp12bpp_WeightMask_64x32
  INIT_WEIGHT_MASK(64, 32, 12, 3, 2);
#endif
#ifndef LIBGAV1_Dsp12bpp_WeightMask_64x64
  INIT_WEIGHT_MASK(64, 64, 12, 3, 3);
#endif
#ifndef LIBGAV1_Dsp12bpp_WeightMask_64x128
  INIT_WEIGHT_MASK(64, 128, 12, 3, 4);
#endif
#ifndef LIBGAV1_Dsp12bpp_WeightMask_128x64
  INIT_WEIGHT_MASK(128, 64, 12, 4, 3);
#endif
#ifndef LIBGAV1_Dsp12bpp_WeightMask_128x128
  INIT_WEIGHT_MASK(128, 128, 12, 4, 4);
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace

void WeightMaskInit_C() {
  Init8bpp();
#if LIBGAV1_MAX_BITDEPTH >= 10
  Init10bpp();
#endif
#if LIBGAV1_MAX_BITDEPTH == 12
  Init12bpp();
#endif
}

}  // namespace dsp
}  // namespace libgav1
