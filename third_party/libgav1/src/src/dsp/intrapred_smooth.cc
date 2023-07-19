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

#include "src/dsp/intrapred_smooth.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>

#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"

namespace libgav1 {
namespace dsp {
namespace {

template <int block_width, int block_height, typename Pixel>
struct SmoothFuncs_C {
  SmoothFuncs_C() = delete;

  static void Smooth(void* dest, ptrdiff_t stride, const void* top_row,
                     const void* left_column);
  static void SmoothVertical(void* dest, ptrdiff_t stride, const void* top_row,
                             const void* left_column);
  static void SmoothHorizontal(void* dest, ptrdiff_t stride,
                               const void* top_row, const void* left_column);
};

constexpr uint8_t kSmoothWeights[] = {
#include "src/dsp/smooth_weights.inc"
};

// SmoothFuncs_C::Smooth
template <int block_width, int block_height, typename Pixel>
void SmoothFuncs_C<block_width, block_height, Pixel>::Smooth(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const Pixel*>(top_row);
  const auto* const left = static_cast<const Pixel*>(left_column);
  const Pixel top_right = top[block_width - 1];
  const Pixel bottom_left = left[block_height - 1];
  static_assert(
      block_width >= 4 && block_height >= 4,
      "Weights for smooth predictor undefined for block width/height < 4");
  const uint8_t* const weights_x = kSmoothWeights + block_width - 4;
  const uint8_t* const weights_y = kSmoothWeights + block_height - 4;
  const uint16_t scale_value = (1 << kSmoothWeightScale);
  auto* dst = static_cast<Pixel*>(dest);
  stride /= sizeof(Pixel);

  for (int y = 0; y < block_height; ++y) {
    for (int x = 0; x < block_width; ++x) {
      assert(scale_value >= weights_y[y] && scale_value >= weights_x[x]);
      uint32_t pred = weights_y[y] * top[x];
      pred += weights_x[x] * left[y];
      pred += static_cast<uint8_t>(scale_value - weights_y[y]) * bottom_left;
      pred += static_cast<uint8_t>(scale_value - weights_x[x]) * top_right;
      // The maximum value of pred with the rounder is 2^9 * (2^bitdepth - 1)
      // + 256. With the descale there's no need for saturation.
      dst[x] = static_cast<Pixel>(
          RightShiftWithRounding(pred, kSmoothWeightScale + 1));
    }
    dst += stride;
  }
}

// SmoothFuncs_C::SmoothVertical
template <int block_width, int block_height, typename Pixel>
void SmoothFuncs_C<block_width, block_height, Pixel>::SmoothVertical(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const Pixel*>(top_row);
  const auto* const left = static_cast<const Pixel*>(left_column);
  const Pixel bottom_left = left[block_height - 1];
  static_assert(block_height >= 4,
                "Weights for smooth predictor undefined for block height < 4");
  const uint8_t* const weights_y = kSmoothWeights + block_height - 4;
  const uint16_t scale_value = (1 << kSmoothWeightScale);
  auto* dst = static_cast<Pixel*>(dest);
  stride /= sizeof(Pixel);

  for (int y = 0; y < block_height; ++y) {
    for (int x = 0; x < block_width; ++x) {
      assert(scale_value >= weights_y[y]);
      uint32_t pred = weights_y[y] * top[x];
      pred += static_cast<uint8_t>(scale_value - weights_y[y]) * bottom_left;
      dst[x] =
          static_cast<Pixel>(RightShiftWithRounding(pred, kSmoothWeightScale));
    }
    dst += stride;
  }
}

// SmoothFuncs_C::SmoothHorizontal
template <int block_width, int block_height, typename Pixel>
void SmoothFuncs_C<block_width, block_height, Pixel>::SmoothHorizontal(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const Pixel*>(top_row);
  const auto* const left = static_cast<const Pixel*>(left_column);
  const Pixel top_right = top[block_width - 1];
  static_assert(block_width >= 4,
                "Weights for smooth predictor undefined for block width < 4");
  const uint8_t* const weights_x = kSmoothWeights + block_width - 4;
  const uint16_t scale_value = (1 << kSmoothWeightScale);
  auto* dst = static_cast<Pixel*>(dest);
  stride /= sizeof(Pixel);

  for (int y = 0; y < block_height; ++y) {
    for (int x = 0; x < block_width; ++x) {
      assert(scale_value >= weights_x[x]);
      uint32_t pred = weights_x[x] * left[y];
      pred += static_cast<uint8_t>(scale_value - weights_x[x]) * top_right;
      dst[x] =
          static_cast<Pixel>(RightShiftWithRounding(pred, kSmoothWeightScale));
    }
    dst += stride;
  }
}

// -----------------------------------------------------------------------------

template <typename Pixel>
struct SmoothDefs {
  SmoothDefs() = delete;

  using _4x4 = SmoothFuncs_C<4, 4, Pixel>;
  using _4x8 = SmoothFuncs_C<4, 8, Pixel>;
  using _4x16 = SmoothFuncs_C<4, 16, Pixel>;
  using _8x4 = SmoothFuncs_C<8, 4, Pixel>;
  using _8x8 = SmoothFuncs_C<8, 8, Pixel>;
  using _8x16 = SmoothFuncs_C<8, 16, Pixel>;
  using _8x32 = SmoothFuncs_C<8, 32, Pixel>;
  using _16x4 = SmoothFuncs_C<16, 4, Pixel>;
  using _16x8 = SmoothFuncs_C<16, 8, Pixel>;
  using _16x16 = SmoothFuncs_C<16, 16, Pixel>;
  using _16x32 = SmoothFuncs_C<16, 32, Pixel>;
  using _16x64 = SmoothFuncs_C<16, 64, Pixel>;
  using _32x8 = SmoothFuncs_C<32, 8, Pixel>;
  using _32x16 = SmoothFuncs_C<32, 16, Pixel>;
  using _32x32 = SmoothFuncs_C<32, 32, Pixel>;
  using _32x64 = SmoothFuncs_C<32, 64, Pixel>;
  using _64x16 = SmoothFuncs_C<64, 16, Pixel>;
  using _64x32 = SmoothFuncs_C<64, 32, Pixel>;
  using _64x64 = SmoothFuncs_C<64, 64, Pixel>;
};

using Defs = SmoothDefs<uint8_t>;

// Initializes dsp entries for kTransformSize|W|x|H| from |DEFS| of
// the same size.
#define INIT_SMOOTH_WxH(DEFS, W, H)                                       \
  dsp->intra_predictors[kTransformSize##W##x##H][kIntraPredictorSmooth] = \
      DEFS::_##W##x##H::Smooth;                                           \
  dsp->intra_predictors[kTransformSize##W##x##H]                          \
                       [kIntraPredictorSmoothVertical] =                  \
      DEFS::_##W##x##H::SmoothVertical;                                   \
  dsp->intra_predictors[kTransformSize##W##x##H]                          \
                       [kIntraPredictorSmoothHorizontal] =                \
      DEFS::_##W##x##H::SmoothHorizontal

#define INIT_SMOOTH(DEFS)        \
  INIT_SMOOTH_WxH(DEFS, 4, 4);   \
  INIT_SMOOTH_WxH(DEFS, 4, 8);   \
  INIT_SMOOTH_WxH(DEFS, 4, 16);  \
  INIT_SMOOTH_WxH(DEFS, 8, 4);   \
  INIT_SMOOTH_WxH(DEFS, 8, 8);   \
  INIT_SMOOTH_WxH(DEFS, 8, 16);  \
  INIT_SMOOTH_WxH(DEFS, 8, 32);  \
  INIT_SMOOTH_WxH(DEFS, 16, 4);  \
  INIT_SMOOTH_WxH(DEFS, 16, 8);  \
  INIT_SMOOTH_WxH(DEFS, 16, 16); \
  INIT_SMOOTH_WxH(DEFS, 16, 32); \
  INIT_SMOOTH_WxH(DEFS, 16, 64); \
  INIT_SMOOTH_WxH(DEFS, 32, 8);  \
  INIT_SMOOTH_WxH(DEFS, 32, 16); \
  INIT_SMOOTH_WxH(DEFS, 32, 32); \
  INIT_SMOOTH_WxH(DEFS, 32, 64); \
  INIT_SMOOTH_WxH(DEFS, 64, 16); \
  INIT_SMOOTH_WxH(DEFS, 64, 32); \
  INIT_SMOOTH_WxH(DEFS, 64, 64)

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(8);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  INIT_SMOOTH(Defs);
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x4_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorSmooth] =
      Defs::_4x4::Smooth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x4_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorSmoothVertical] =
      Defs::_4x4::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x4_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorSmoothHorizontal] =
      Defs::_4x4::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize4x8_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorSmooth] =
      Defs::_4x8::Smooth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x8_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorSmoothVertical] =
      Defs::_4x8::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x8_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorSmoothHorizontal] =
      Defs::_4x8::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize4x16_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorSmooth] =
      Defs::_4x16::Smooth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x16_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorSmoothVertical] =
      Defs::_4x16::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x16_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorSmoothHorizontal] =
      Defs::_4x16::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize8x4_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorSmooth] =
      Defs::_8x4::Smooth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x4_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorSmoothVertical] =
      Defs::_8x4::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x4_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorSmoothHorizontal] =
      Defs::_8x4::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize8x8_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorSmooth] =
      Defs::_8x8::Smooth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x8_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorSmoothVertical] =
      Defs::_8x8::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x8_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorSmoothHorizontal] =
      Defs::_8x8::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize8x16_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorSmooth] =
      Defs::_8x16::Smooth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x16_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorSmoothVertical] =
      Defs::_8x16::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x16_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorSmoothHorizontal] =
      Defs::_8x16::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize8x32_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorSmooth] =
      Defs::_8x32::Smooth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x32_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorSmoothVertical] =
      Defs::_8x32::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x32_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorSmoothHorizontal] =
      Defs::_8x32::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize16x4_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorSmooth] =
      Defs::_16x4::Smooth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x4_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorSmoothVertical] =
      Defs::_16x4::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x4_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorSmoothHorizontal] =
      Defs::_16x4::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize16x8_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorSmooth] =
      Defs::_16x8::Smooth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x8_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorSmoothVertical] =
      Defs::_16x8::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x8_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorSmoothHorizontal] =
      Defs::_16x8::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize16x16_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorSmooth] =
      Defs::_16x16::Smooth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x16_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorSmoothVertical] =
      Defs::_16x16::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x16_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorSmoothHorizontal] =
      Defs::_16x16::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize16x32_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorSmooth] =
      Defs::_16x32::Smooth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x32_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorSmoothVertical] =
      Defs::_16x32::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x32_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorSmoothHorizontal] =
      Defs::_16x32::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize16x64_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorSmooth] =
      Defs::_16x64::Smooth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x64_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorSmoothVertical] =
      Defs::_16x64::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x64_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorSmoothHorizontal] =
      Defs::_16x64::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize32x8_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorSmooth] =
      Defs::_32x8::Smooth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x8_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorSmoothVertical] =
      Defs::_32x8::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x8_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorSmoothHorizontal] =
      Defs::_32x8::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize32x16_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorSmooth] =
      Defs::_32x16::Smooth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x16_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorSmoothVertical] =
      Defs::_32x16::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x16_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorSmoothHorizontal] =
      Defs::_32x16::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize32x32_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorSmooth] =
      Defs::_32x32::Smooth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x32_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorSmoothVertical] =
      Defs::_32x32::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x32_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorSmoothHorizontal] =
      Defs::_32x32::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize32x64_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorSmooth] =
      Defs::_32x64::Smooth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x64_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorSmoothVertical] =
      Defs::_32x64::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x64_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorSmoothHorizontal] =
      Defs::_32x64::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize64x16_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorSmooth] =
      Defs::_64x16::Smooth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x16_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorSmoothVertical] =
      Defs::_64x16::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x16_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorSmoothHorizontal] =
      Defs::_64x16::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize64x32_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorSmooth] =
      Defs::_64x32::Smooth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x32_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorSmoothVertical] =
      Defs::_64x32::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x32_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorSmoothHorizontal] =
      Defs::_64x32::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize64x64_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorSmooth] =
      Defs::_64x64::Smooth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x64_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorSmoothVertical] =
      Defs::_64x64::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x64_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorSmoothHorizontal] =
      Defs::_64x64::SmoothHorizontal;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}  // NOLINT(readability/fn_size)

#if LIBGAV1_MAX_BITDEPTH >= 10
using DefsHbd = SmoothDefs<uint16_t>;

void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(10);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  INIT_SMOOTH(DefsHbd);
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x4_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorSmooth] =
      DefsHbd::_4x4::Smooth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x4_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorSmoothVertical] =
      DefsHbd::_4x4::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x4_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_4x4::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize4x8_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorSmooth] =
      DefsHbd::_4x8::Smooth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x8_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorSmoothVertical] =
      DefsHbd::_4x8::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x8_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_4x8::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize4x16_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorSmooth] =
      DefsHbd::_4x16::Smooth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x16_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorSmoothVertical] =
      DefsHbd::_4x16::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x16_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_4x16::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize8x4_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorSmooth] =
      DefsHbd::_8x4::Smooth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x4_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorSmoothVertical] =
      DefsHbd::_8x4::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x4_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_8x4::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize8x8_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorSmooth] =
      DefsHbd::_8x8::Smooth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x8_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorSmoothVertical] =
      DefsHbd::_8x8::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x8_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_8x8::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize8x16_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorSmooth] =
      DefsHbd::_8x16::Smooth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x16_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorSmoothVertical] =
      DefsHbd::_8x16::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x16_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_8x16::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize8x32_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorSmooth] =
      DefsHbd::_8x32::Smooth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x32_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorSmoothVertical] =
      DefsHbd::_8x32::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x32_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_8x32::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize16x4_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorSmooth] =
      DefsHbd::_16x4::Smooth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x4_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorSmoothVertical] =
      DefsHbd::_16x4::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x4_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_16x4::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize16x8_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorSmooth] =
      DefsHbd::_16x8::Smooth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x8_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorSmoothVertical] =
      DefsHbd::_16x8::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x8_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_16x8::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize16x16_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorSmooth] =
      DefsHbd::_16x16::Smooth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x16_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorSmoothVertical] =
      DefsHbd::_16x16::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x16_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_16x16::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize16x32_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorSmooth] =
      DefsHbd::_16x32::Smooth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x32_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorSmoothVertical] =
      DefsHbd::_16x32::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x32_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_16x32::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize16x64_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorSmooth] =
      DefsHbd::_16x64::Smooth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x64_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorSmoothVertical] =
      DefsHbd::_16x64::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x64_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_16x64::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize32x8_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorSmooth] =
      DefsHbd::_32x8::Smooth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x8_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorSmoothVertical] =
      DefsHbd::_32x8::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x8_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_32x8::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize32x16_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorSmooth] =
      DefsHbd::_32x16::Smooth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x16_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorSmoothVertical] =
      DefsHbd::_32x16::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x16_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_32x16::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize32x32_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorSmooth] =
      DefsHbd::_32x32::Smooth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x32_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorSmoothVertical] =
      DefsHbd::_32x32::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x32_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_32x32::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize32x64_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorSmooth] =
      DefsHbd::_32x64::Smooth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x64_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorSmoothVertical] =
      DefsHbd::_32x64::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x64_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_32x64::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize64x16_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorSmooth] =
      DefsHbd::_64x16::Smooth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x16_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorSmoothVertical] =
      DefsHbd::_64x16::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x16_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_64x16::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize64x32_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorSmooth] =
      DefsHbd::_64x32::Smooth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x32_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorSmoothVertical] =
      DefsHbd::_64x32::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x32_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_64x32::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize64x64_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorSmooth] =
      DefsHbd::_64x64::Smooth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x64_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorSmoothVertical] =
      DefsHbd::_64x64::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x64_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_64x64::SmoothHorizontal;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}  // NOLINT(readability/fn_size)
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using DefsHbd = SmoothDefs<uint16_t>;

void Init12bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(12);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  INIT_SMOOTH(DefsHbd);
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x4_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorSmooth] =
      DefsHbd::_4x4::Smooth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x4_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorSmoothVertical] =
      DefsHbd::_4x4::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x4_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_4x4::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize4x8_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorSmooth] =
      DefsHbd::_4x8::Smooth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x8_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorSmoothVertical] =
      DefsHbd::_4x8::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x8_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_4x8::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize4x16_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorSmooth] =
      DefsHbd::_4x16::Smooth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x16_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorSmoothVertical] =
      DefsHbd::_4x16::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x16_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_4x16::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize8x4_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorSmooth] =
      DefsHbd::_8x4::Smooth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x4_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorSmoothVertical] =
      DefsHbd::_8x4::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x4_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_8x4::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize8x8_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorSmooth] =
      DefsHbd::_8x8::Smooth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x8_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorSmoothVertical] =
      DefsHbd::_8x8::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x8_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_8x8::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize8x16_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorSmooth] =
      DefsHbd::_8x16::Smooth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x16_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorSmoothVertical] =
      DefsHbd::_8x16::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x16_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_8x16::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize8x32_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorSmooth] =
      DefsHbd::_8x32::Smooth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x32_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorSmoothVertical] =
      DefsHbd::_8x32::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x32_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_8x32::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize16x4_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorSmooth] =
      DefsHbd::_16x4::Smooth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x4_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorSmoothVertical] =
      DefsHbd::_16x4::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x4_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_16x4::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize16x8_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorSmooth] =
      DefsHbd::_16x8::Smooth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x8_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorSmoothVertical] =
      DefsHbd::_16x8::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x8_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_16x8::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize16x16_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorSmooth] =
      DefsHbd::_16x16::Smooth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x16_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorSmoothVertical] =
      DefsHbd::_16x16::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x16_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_16x16::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize16x32_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorSmooth] =
      DefsHbd::_16x32::Smooth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x32_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorSmoothVertical] =
      DefsHbd::_16x32::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x32_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_16x32::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize16x64_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorSmooth] =
      DefsHbd::_16x64::Smooth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x64_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorSmoothVertical] =
      DefsHbd::_16x64::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x64_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_16x64::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize32x8_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorSmooth] =
      DefsHbd::_32x8::Smooth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x8_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorSmoothVertical] =
      DefsHbd::_32x8::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x8_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_32x8::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize32x16_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorSmooth] =
      DefsHbd::_32x16::Smooth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x16_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorSmoothVertical] =
      DefsHbd::_32x16::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x16_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_32x16::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize32x32_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorSmooth] =
      DefsHbd::_32x32::Smooth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x32_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorSmoothVertical] =
      DefsHbd::_32x32::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x32_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_32x32::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize32x64_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorSmooth] =
      DefsHbd::_32x64::Smooth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x64_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorSmoothVertical] =
      DefsHbd::_32x64::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x64_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_32x64::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize64x16_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorSmooth] =
      DefsHbd::_64x16::Smooth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x16_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorSmoothVertical] =
      DefsHbd::_64x16::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x16_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_64x16::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize64x32_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorSmooth] =
      DefsHbd::_64x32::Smooth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x32_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorSmoothVertical] =
      DefsHbd::_64x32::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x32_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_64x32::SmoothHorizontal;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize64x64_IntraPredictorSmooth
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorSmooth] =
      DefsHbd::_64x64::Smooth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x64_IntraPredictorSmoothVertical
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorSmoothVertical] =
      DefsHbd::_64x64::SmoothVertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x64_IntraPredictorSmoothHorizontal
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorSmoothHorizontal] =
      DefsHbd::_64x64::SmoothHorizontal;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}  // NOLINT(readability/fn_size)
#endif  // LIBGAV1_MAX_BITDEPTH == 12

#undef INIT_SMOOTH_WxH
#undef INIT_SMOOTH
}  // namespace

void IntraPredSmoothInit_C() {
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
