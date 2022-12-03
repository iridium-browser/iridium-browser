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

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "src/dsp/dsp.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"

namespace libgav1 {
namespace dsp {
namespace {

#include "src/dsp/obmc.inc"

// 7.11.3.10 (from top samples).
template <typename Pixel>
void OverlapBlendVertical_C(void* LIBGAV1_RESTRICT const prediction,
                            const ptrdiff_t prediction_stride, const int width,
                            const int height,
                            const void* LIBGAV1_RESTRICT const obmc_prediction,
                            const ptrdiff_t obmc_prediction_stride) {
  auto* pred = static_cast<Pixel*>(prediction);
  const ptrdiff_t pred_stride = prediction_stride / sizeof(Pixel);
  const auto* obmc_pred = static_cast<const Pixel*>(obmc_prediction);
  const ptrdiff_t obmc_pred_stride = obmc_prediction_stride / sizeof(Pixel);
  const uint8_t* const mask = kObmcMask + height - 2;
  assert(width >= 4);
  assert(height >= 2);

  for (int y = 0; y < height; ++y) {
    const uint8_t mask_value = mask[y];
    for (int x = 0; x < width; ++x) {
      pred[x] = static_cast<Pixel>(RightShiftWithRounding(
          mask_value * pred[x] + (64 - mask_value) * obmc_pred[x], 6));
    }
    pred += pred_stride;
    obmc_pred += obmc_pred_stride;
  }
}

// 7.11.3.10 (from left samples).
template <typename Pixel>
void OverlapBlendHorizontal_C(
    void* LIBGAV1_RESTRICT const prediction, const ptrdiff_t prediction_stride,
    const int width, const int height,
    const void* LIBGAV1_RESTRICT const obmc_prediction,
    const ptrdiff_t obmc_prediction_stride) {
  auto* pred = static_cast<Pixel*>(prediction);
  const ptrdiff_t pred_stride = prediction_stride / sizeof(Pixel);
  const auto* obmc_pred = static_cast<const Pixel*>(obmc_prediction);
  const ptrdiff_t obmc_pred_stride = obmc_prediction_stride / sizeof(Pixel);
  const uint8_t* const mask = kObmcMask + width - 2;
  assert(width >= 2);
  assert(height >= 4);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const uint8_t mask_value = mask[x];
      pred[x] = static_cast<Pixel>(RightShiftWithRounding(
          mask_value * pred[x] + (64 - mask_value) * obmc_pred[x], 6));
    }
    pred += pred_stride;
    obmc_pred += obmc_pred_stride;
  }
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(8);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->obmc_blend[kObmcDirectionVertical] = OverlapBlendVertical_C<uint8_t>;
  dsp->obmc_blend[kObmcDirectionHorizontal] = OverlapBlendHorizontal_C<uint8_t>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp8bpp_ObmcVertical
  dsp->obmc_blend[kObmcDirectionVertical] = OverlapBlendVertical_C<uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_ObmcHorizontal
  dsp->obmc_blend[kObmcDirectionHorizontal] = OverlapBlendHorizontal_C<uint8_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}

#if LIBGAV1_MAX_BITDEPTH >= 10
void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(10);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->obmc_blend[kObmcDirectionVertical] = OverlapBlendVertical_C<uint16_t>;
  dsp->obmc_blend[kObmcDirectionHorizontal] =
      OverlapBlendHorizontal_C<uint16_t>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp10bpp_ObmcVertical
  dsp->obmc_blend[kObmcDirectionVertical] = OverlapBlendVertical_C<uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_ObmcHorizontal
  dsp->obmc_blend[kObmcDirectionHorizontal] =
      OverlapBlendHorizontal_C<uint16_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
void Init12bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(12);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->obmc_blend[kObmcDirectionVertical] = OverlapBlendVertical_C<uint16_t>;
  dsp->obmc_blend[kObmcDirectionHorizontal] =
      OverlapBlendHorizontal_C<uint16_t>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp12bpp_ObmcVertical
  dsp->obmc_blend[kObmcDirectionVertical] = OverlapBlendVertical_C<uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_ObmcHorizontal
  dsp->obmc_blend[kObmcDirectionHorizontal] =
      OverlapBlendHorizontal_C<uint16_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace

void ObmcInit_C() {
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
