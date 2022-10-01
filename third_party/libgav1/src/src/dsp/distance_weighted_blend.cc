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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "src/dsp/dsp.h"
#include "src/utils/common.h"

namespace libgav1 {
namespace dsp {
namespace {

template <int bitdepth, typename Pixel>
void DistanceWeightedBlend_C(const void* LIBGAV1_RESTRICT prediction_0,
                             const void* LIBGAV1_RESTRICT prediction_1,
                             const uint8_t weight_0, const uint8_t weight_1,
                             const int width, const int height,
                             void* LIBGAV1_RESTRICT const dest,
                             const ptrdiff_t dest_stride) {
  // 7.11.3.2 Rounding variables derivation process
  //   2 * FILTER_BITS(7) - (InterRound0(3|5) + InterRound1(7))
  constexpr int inter_post_round_bits = (bitdepth == 12) ? 2 : 4;
  using PredType =
      typename std::conditional<bitdepth == 8, int16_t, uint16_t>::type;
  const auto* pred_0 = static_cast<const PredType*>(prediction_0);
  const auto* pred_1 = static_cast<const PredType*>(prediction_1);
  auto* dst = static_cast<Pixel*>(dest);
  const ptrdiff_t dst_stride = dest_stride / sizeof(Pixel);

  int y = 0;
  do {
    int x = 0;
    do {
      // See warp.cc and convolve.cc for detailed prediction ranges.
      // weight_0 + weight_1 = 16.
      int res = pred_0[x] * weight_0 + pred_1[x] * weight_1;
      res -= (bitdepth == 8) ? 0 : kCompoundOffset * 16;
      dst[x] = static_cast<Pixel>(
          Clip3(RightShiftWithRounding(res, inter_post_round_bits + 4), 0,
                (1 << bitdepth) - 1));
    } while (++x < width);

    dst += dst_stride;
    pred_0 += width;
    pred_1 += width;
  } while (++y < height);
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(8);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->distance_weighted_blend = DistanceWeightedBlend_C<8, uint8_t>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp8bpp_DistanceWeightedBlend
  dsp->distance_weighted_blend = DistanceWeightedBlend_C<8, uint8_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}

#if LIBGAV1_MAX_BITDEPTH >= 10
void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(10);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->distance_weighted_blend = DistanceWeightedBlend_C<10, uint16_t>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp10bpp_DistanceWeightedBlend
  dsp->distance_weighted_blend = DistanceWeightedBlend_C<10, uint16_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
void Init12bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(12);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->distance_weighted_blend = DistanceWeightedBlend_C<12, uint16_t>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp12bpp_DistanceWeightedBlend
  dsp->distance_weighted_blend = DistanceWeightedBlend_C<12, uint16_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace

void DistanceWeightedBlendInit_C() {
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
