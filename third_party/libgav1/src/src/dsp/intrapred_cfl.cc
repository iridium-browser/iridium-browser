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

#include "src/dsp/intrapred_cfl.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"

namespace libgav1 {
namespace dsp {
namespace {

constexpr TransformSize kTransformSizesLargerThan32x32[] = {
    kTransformSize16x64, kTransformSize32x64, kTransformSize64x16,
    kTransformSize64x32, kTransformSize64x64};

//------------------------------------------------------------------------------
// CflIntraPredictor_C

// |luma| can be within +/-(((1 << bitdepth) - 1) << 3), inclusive.
// |alpha| can be -16 to 16 (inclusive).
template <int block_width, int block_height, int bitdepth, typename Pixel>
void CflIntraPredictor_C(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
    const int alpha) {
  auto* dst = static_cast<Pixel*>(dest);
  const int dc = dst[0];
  stride /= sizeof(Pixel);
  const int max_value = (1 << bitdepth) - 1;
  for (int y = 0; y < block_height; ++y) {
    for (int x = 0; x < block_width; ++x) {
      assert(luma[y][x] >= -(((1 << bitdepth) - 1) << 3));
      assert(luma[y][x] <= ((1 << bitdepth) - 1) << 3);
      dst[x] = Clip3(dc + RightShiftWithRoundingSigned(alpha * luma[y][x], 6),
                     0, max_value);
    }
    dst += stride;
  }
}

//------------------------------------------------------------------------------
// CflSubsampler_C

template <int block_width, int block_height, int bitdepth, typename Pixel,
          int subsampling_x, int subsampling_y>
void CflSubsampler_C(int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
                     const int max_luma_width, const int max_luma_height,
                     const void* LIBGAV1_RESTRICT const source,
                     ptrdiff_t stride) {
  assert(max_luma_width >= 4);
  assert(max_luma_height >= 4);
  const auto* src = static_cast<const Pixel*>(source);
  stride /= sizeof(Pixel);
  int sum = 0;
  for (int y = 0; y < block_height; ++y) {
    for (int x = 0; x < block_width; ++x) {
      const ptrdiff_t luma_x =
          std::min(x << subsampling_x, max_luma_width - (1 << subsampling_x));
      const ptrdiff_t luma_x_next = luma_x + stride;
      luma[y][x] =
          (src[luma_x] + ((subsampling_x != 0) ? src[luma_x + 1] : 0) +
           ((subsampling_y != 0) ? (src[luma_x_next] + src[luma_x_next + 1])
                                 : 0))
          << (3 - subsampling_x - subsampling_y);
      sum += luma[y][x];
    }
    if ((y << subsampling_y) < (max_luma_height - (1 << subsampling_y))) {
      src += stride << subsampling_y;
    }
  }
  const int average = RightShiftWithRounding(
      sum, FloorLog2(block_width) + FloorLog2(block_height));
  for (int y = 0; y < block_height; ++y) {
    for (int x = 0; x < block_width; ++x) {
      luma[y][x] -= average;
    }
  }
}

//------------------------------------------------------------------------------

// Initializes dsp entries for kTransformSize|W|x|H|.
#define INIT_CFL_INTRAPREDICTOR_WxH(W, H, BITDEPTH, PIXEL)             \
  dsp->cfl_intra_predictors[kTransformSize##W##x##H] =                 \
      CflIntraPredictor_C<W, H, BITDEPTH, PIXEL>;                      \
  dsp->cfl_subsamplers[kTransformSize##W##x##H][kSubsamplingType444] = \
      CflSubsampler_C<W, H, BITDEPTH, PIXEL, 0, 0>;                    \
  dsp->cfl_subsamplers[kTransformSize##W##x##H][kSubsamplingType422] = \
      CflSubsampler_C<W, H, BITDEPTH, PIXEL, 1, 0>;                    \
  dsp->cfl_subsamplers[kTransformSize##W##x##H][kSubsamplingType420] = \
      CflSubsampler_C<W, H, BITDEPTH, PIXEL, 1, 1>

#define INIT_CFL_INTRAPREDICTORS(BITDEPTH, PIXEL)       \
  INIT_CFL_INTRAPREDICTOR_WxH(4, 4, BITDEPTH, PIXEL);   \
  INIT_CFL_INTRAPREDICTOR_WxH(4, 8, BITDEPTH, PIXEL);   \
  INIT_CFL_INTRAPREDICTOR_WxH(4, 16, BITDEPTH, PIXEL);  \
  INIT_CFL_INTRAPREDICTOR_WxH(8, 4, BITDEPTH, PIXEL);   \
  INIT_CFL_INTRAPREDICTOR_WxH(8, 8, BITDEPTH, PIXEL);   \
  INIT_CFL_INTRAPREDICTOR_WxH(8, 16, BITDEPTH, PIXEL);  \
  INIT_CFL_INTRAPREDICTOR_WxH(8, 32, BITDEPTH, PIXEL);  \
  INIT_CFL_INTRAPREDICTOR_WxH(16, 4, BITDEPTH, PIXEL);  \
  INIT_CFL_INTRAPREDICTOR_WxH(16, 8, BITDEPTH, PIXEL);  \
  INIT_CFL_INTRAPREDICTOR_WxH(16, 16, BITDEPTH, PIXEL); \
  INIT_CFL_INTRAPREDICTOR_WxH(16, 32, BITDEPTH, PIXEL); \
  INIT_CFL_INTRAPREDICTOR_WxH(32, 8, BITDEPTH, PIXEL);  \
  INIT_CFL_INTRAPREDICTOR_WxH(32, 16, BITDEPTH, PIXEL); \
  INIT_CFL_INTRAPREDICTOR_WxH(32, 32, BITDEPTH, PIXEL)

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(8);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  INIT_CFL_INTRAPREDICTORS(8, uint8_t);
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x4_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize4x4] =
      CflIntraPredictor_C<4, 4, 8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x4_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize4x4][kSubsamplingType444] =
      CflSubsampler_C<4, 4, 8, uint8_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x4_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize4x4][kSubsamplingType422] =
      CflSubsampler_C<4, 4, 8, uint8_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x4_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize4x4][kSubsamplingType420] =
      CflSubsampler_C<4, 4, 8, uint8_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize4x8_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize4x8] =
      CflIntraPredictor_C<4, 8, 8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x8_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize4x8][kSubsamplingType444] =
      CflSubsampler_C<4, 8, 8, uint8_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x8_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize4x8][kSubsamplingType422] =
      CflSubsampler_C<4, 8, 8, uint8_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x8_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize4x8][kSubsamplingType420] =
      CflSubsampler_C<4, 8, 8, uint8_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize4x16_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize4x16] =
      CflIntraPredictor_C<4, 16, 8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x16_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize4x16][kSubsamplingType444] =
      CflSubsampler_C<4, 16, 8, uint8_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x16_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize4x16][kSubsamplingType422] =
      CflSubsampler_C<4, 16, 8, uint8_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x16_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize4x16][kSubsamplingType420] =
      CflSubsampler_C<4, 16, 8, uint8_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize8x4_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize8x4] =
      CflIntraPredictor_C<8, 4, 8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x4_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize8x4][kSubsamplingType444] =
      CflSubsampler_C<8, 4, 8, uint8_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x4_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize8x4][kSubsamplingType422] =
      CflSubsampler_C<8, 4, 8, uint8_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x4_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize8x4][kSubsamplingType420] =
      CflSubsampler_C<8, 4, 8, uint8_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize8x8_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize8x8] =
      CflIntraPredictor_C<8, 8, 8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x8_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize8x8][kSubsamplingType444] =
      CflSubsampler_C<8, 8, 8, uint8_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x8_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize8x8][kSubsamplingType422] =
      CflSubsampler_C<8, 8, 8, uint8_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x8_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize8x8][kSubsamplingType420] =
      CflSubsampler_C<8, 8, 8, uint8_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize8x16_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize8x16] =
      CflIntraPredictor_C<8, 16, 8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x16_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize8x16][kSubsamplingType444] =
      CflSubsampler_C<8, 16, 8, uint8_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x16_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize8x16][kSubsamplingType422] =
      CflSubsampler_C<8, 16, 8, uint8_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x16_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize8x16][kSubsamplingType420] =
      CflSubsampler_C<8, 16, 8, uint8_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize8x32_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize8x32] =
      CflIntraPredictor_C<8, 32, 8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x32_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize8x32][kSubsamplingType444] =
      CflSubsampler_C<8, 32, 8, uint8_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x32_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize8x32][kSubsamplingType422] =
      CflSubsampler_C<8, 32, 8, uint8_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x32_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize8x32][kSubsamplingType420] =
      CflSubsampler_C<8, 32, 8, uint8_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize16x4_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize16x4] =
      CflIntraPredictor_C<16, 4, 8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x4_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize16x4][kSubsamplingType444] =
      CflSubsampler_C<16, 4, 8, uint8_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x4_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize16x4][kSubsamplingType422] =
      CflSubsampler_C<16, 4, 8, uint8_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x4_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize16x4][kSubsamplingType420] =
      CflSubsampler_C<16, 4, 8, uint8_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize16x8_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize16x8] =
      CflIntraPredictor_C<16, 8, 8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x8_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize16x8][kSubsamplingType444] =
      CflSubsampler_C<16, 8, 8, uint8_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x8_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize16x8][kSubsamplingType422] =
      CflSubsampler_C<16, 8, 8, uint8_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x8_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize16x8][kSubsamplingType420] =
      CflSubsampler_C<16, 8, 8, uint8_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize16x16_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize16x16] =
      CflIntraPredictor_C<16, 16, 8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x16_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize16x16][kSubsamplingType444] =
      CflSubsampler_C<16, 16, 8, uint8_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x16_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize16x16][kSubsamplingType422] =
      CflSubsampler_C<16, 16, 8, uint8_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x16_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize16x16][kSubsamplingType420] =
      CflSubsampler_C<16, 16, 8, uint8_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize16x32_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize16x32] =
      CflIntraPredictor_C<16, 32, 8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x32_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize16x32][kSubsamplingType444] =
      CflSubsampler_C<16, 32, 8, uint8_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x32_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize16x32][kSubsamplingType422] =
      CflSubsampler_C<16, 32, 8, uint8_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x32_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize16x32][kSubsamplingType420] =
      CflSubsampler_C<16, 32, 8, uint8_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize32x8_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize32x8] =
      CflIntraPredictor_C<32, 8, 8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x8_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize32x8][kSubsamplingType444] =
      CflSubsampler_C<32, 8, 8, uint8_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x8_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize32x8][kSubsamplingType422] =
      CflSubsampler_C<32, 8, 8, uint8_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x8_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize32x8][kSubsamplingType420] =
      CflSubsampler_C<32, 8, 8, uint8_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize32x16_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize32x16] =
      CflIntraPredictor_C<32, 16, 8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x16_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize32x16][kSubsamplingType444] =
      CflSubsampler_C<32, 16, 8, uint8_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x16_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize32x16][kSubsamplingType422] =
      CflSubsampler_C<32, 16, 8, uint8_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x16_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize32x16][kSubsamplingType420] =
      CflSubsampler_C<32, 16, 8, uint8_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp8bpp_TransformSize32x32_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize32x32] =
      CflIntraPredictor_C<32, 32, 8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x32_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize32x32][kSubsamplingType444] =
      CflSubsampler_C<32, 32, 8, uint8_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x32_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize32x32][kSubsamplingType422] =
      CflSubsampler_C<32, 32, 8, uint8_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x32_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize32x32][kSubsamplingType420] =
      CflSubsampler_C<32, 32, 8, uint8_t, 1, 1>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  // Cfl predictors are available only for transform sizes with max(width,
  // height) <= 32. Set all others to nullptr.
  for (const auto i : kTransformSizesLargerThan32x32) {
    dsp->cfl_intra_predictors[i] = nullptr;
    for (int j = 0; j < kNumSubsamplingTypes; ++j) {
      dsp->cfl_subsamplers[i][j] = nullptr;
    }
  }
}  // NOLINT(readability/fn_size)

#if LIBGAV1_MAX_BITDEPTH >= 10
void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(10);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  INIT_CFL_INTRAPREDICTORS(10, uint16_t);
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x4_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize4x4] =
      CflIntraPredictor_C<4, 4, 10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x4_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize4x4][kSubsamplingType444] =
      CflSubsampler_C<4, 4, 10, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x4_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize4x4][kSubsamplingType422] =
      CflSubsampler_C<4, 4, 10, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x4_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize4x4][kSubsamplingType420] =
      CflSubsampler_C<4, 4, 10, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize4x8_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize4x8] =
      CflIntraPredictor_C<4, 8, 10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x8_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize4x8][kSubsamplingType444] =
      CflSubsampler_C<4, 8, 10, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x8_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize4x8][kSubsamplingType422] =
      CflSubsampler_C<4, 8, 10, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x8_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize4x8][kSubsamplingType420] =
      CflSubsampler_C<4, 8, 10, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize4x16_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize4x16] =
      CflIntraPredictor_C<4, 16, 10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x16_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize4x16][kSubsamplingType444] =
      CflSubsampler_C<4, 16, 10, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x16_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize4x16][kSubsamplingType422] =
      CflSubsampler_C<4, 16, 10, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x16_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize4x16][kSubsamplingType420] =
      CflSubsampler_C<4, 16, 10, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize8x4_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize8x4] =
      CflIntraPredictor_C<8, 4, 10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x4_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize8x4][kSubsamplingType444] =
      CflSubsampler_C<8, 4, 10, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x4_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize8x4][kSubsamplingType422] =
      CflSubsampler_C<8, 4, 10, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x4_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize8x4][kSubsamplingType420] =
      CflSubsampler_C<8, 4, 10, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize8x8_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize8x8] =
      CflIntraPredictor_C<8, 8, 10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x8_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize8x8][kSubsamplingType444] =
      CflSubsampler_C<8, 8, 10, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x8_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize8x8][kSubsamplingType422] =
      CflSubsampler_C<8, 8, 10, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x8_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize8x8][kSubsamplingType420] =
      CflSubsampler_C<8, 8, 10, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize8x16_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize8x16] =
      CflIntraPredictor_C<8, 16, 10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x16_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize8x16][kSubsamplingType444] =
      CflSubsampler_C<8, 16, 10, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x16_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize8x16][kSubsamplingType422] =
      CflSubsampler_C<8, 16, 10, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x16_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize8x16][kSubsamplingType420] =
      CflSubsampler_C<8, 16, 10, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize8x32_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize8x32] =
      CflIntraPredictor_C<8, 32, 10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x32_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize8x32][kSubsamplingType444] =
      CflSubsampler_C<8, 32, 10, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x32_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize8x32][kSubsamplingType422] =
      CflSubsampler_C<8, 32, 10, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x32_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize8x32][kSubsamplingType420] =
      CflSubsampler_C<8, 32, 10, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize16x4_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize16x4] =
      CflIntraPredictor_C<16, 4, 10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x4_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize16x4][kSubsamplingType444] =
      CflSubsampler_C<16, 4, 10, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x4_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize16x4][kSubsamplingType422] =
      CflSubsampler_C<16, 4, 10, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x4_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize16x4][kSubsamplingType420] =
      CflSubsampler_C<16, 4, 10, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize16x8_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize16x8] =
      CflIntraPredictor_C<16, 8, 10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x8_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize16x8][kSubsamplingType444] =
      CflSubsampler_C<16, 8, 10, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x8_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize16x8][kSubsamplingType422] =
      CflSubsampler_C<16, 8, 10, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x8_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize16x8][kSubsamplingType420] =
      CflSubsampler_C<16, 8, 10, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize16x16_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize16x16] =
      CflIntraPredictor_C<16, 16, 10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x16_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize16x16][kSubsamplingType444] =
      CflSubsampler_C<16, 16, 10, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x16_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize16x16][kSubsamplingType422] =
      CflSubsampler_C<16, 16, 10, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x16_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize16x16][kSubsamplingType420] =
      CflSubsampler_C<16, 16, 10, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize16x32_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize16x32] =
      CflIntraPredictor_C<16, 32, 10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x32_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize16x32][kSubsamplingType444] =
      CflSubsampler_C<16, 32, 10, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x32_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize16x32][kSubsamplingType422] =
      CflSubsampler_C<16, 32, 10, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x32_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize16x32][kSubsamplingType420] =
      CflSubsampler_C<16, 32, 10, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize32x8_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize32x8] =
      CflIntraPredictor_C<32, 8, 10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x8_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize32x8][kSubsamplingType444] =
      CflSubsampler_C<32, 8, 10, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x8_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize32x8][kSubsamplingType422] =
      CflSubsampler_C<32, 8, 10, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x8_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize32x8][kSubsamplingType420] =
      CflSubsampler_C<32, 8, 10, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize32x16_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize32x16] =
      CflIntraPredictor_C<32, 16, 10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x16_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize32x16][kSubsamplingType444] =
      CflSubsampler_C<32, 16, 10, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x16_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize32x16][kSubsamplingType422] =
      CflSubsampler_C<32, 16, 10, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x16_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize32x16][kSubsamplingType420] =
      CflSubsampler_C<32, 16, 10, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp10bpp_TransformSize32x32_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize32x32] =
      CflIntraPredictor_C<32, 32, 10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x32_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize32x32][kSubsamplingType444] =
      CflSubsampler_C<32, 32, 10, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x32_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize32x32][kSubsamplingType422] =
      CflSubsampler_C<32, 32, 10, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x32_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize32x32][kSubsamplingType420] =
      CflSubsampler_C<32, 32, 10, uint16_t, 1, 1>;
#endif

#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  // Cfl predictors are available only for transform sizes with max(width,
  // height) <= 32. Set all others to nullptr.
  for (const auto i : kTransformSizesLargerThan32x32) {
    dsp->cfl_intra_predictors[i] = nullptr;
    for (int j = 0; j < kNumSubsamplingTypes; ++j) {
      dsp->cfl_subsamplers[i][j] = nullptr;
    }
  }
}  // NOLINT(readability/fn_size)
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
void Init12bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(12);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  INIT_CFL_INTRAPREDICTORS(12, uint16_t);
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x4_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize4x4] =
      CflIntraPredictor_C<4, 4, 12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x4_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize4x4][kSubsamplingType444] =
      CflSubsampler_C<4, 4, 12, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x4_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize4x4][kSubsamplingType422] =
      CflSubsampler_C<4, 4, 12, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x4_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize4x4][kSubsamplingType420] =
      CflSubsampler_C<4, 4, 12, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize4x8_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize4x8] =
      CflIntraPredictor_C<4, 8, 12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x8_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize4x8][kSubsamplingType444] =
      CflSubsampler_C<4, 8, 12, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x8_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize4x8][kSubsamplingType422] =
      CflSubsampler_C<4, 8, 12, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x8_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize4x8][kSubsamplingType420] =
      CflSubsampler_C<4, 8, 12, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize4x16_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize4x16] =
      CflIntraPredictor_C<4, 16, 12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x16_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize4x16][kSubsamplingType444] =
      CflSubsampler_C<4, 16, 12, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x16_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize4x16][kSubsamplingType422] =
      CflSubsampler_C<4, 16, 12, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x16_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize4x16][kSubsamplingType420] =
      CflSubsampler_C<4, 16, 12, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize8x4_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize8x4] =
      CflIntraPredictor_C<8, 4, 12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x4_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize8x4][kSubsamplingType444] =
      CflSubsampler_C<8, 4, 12, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x4_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize8x4][kSubsamplingType422] =
      CflSubsampler_C<8, 4, 12, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x4_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize8x4][kSubsamplingType420] =
      CflSubsampler_C<8, 4, 12, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize8x8_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize8x8] =
      CflIntraPredictor_C<8, 8, 12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x8_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize8x8][kSubsamplingType444] =
      CflSubsampler_C<8, 8, 12, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x8_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize8x8][kSubsamplingType422] =
      CflSubsampler_C<8, 8, 12, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x8_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize8x8][kSubsamplingType420] =
      CflSubsampler_C<8, 8, 12, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize8x16_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize8x16] =
      CflIntraPredictor_C<8, 16, 12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x16_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize8x16][kSubsamplingType444] =
      CflSubsampler_C<8, 16, 12, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x16_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize8x16][kSubsamplingType422] =
      CflSubsampler_C<8, 16, 12, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x16_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize8x16][kSubsamplingType420] =
      CflSubsampler_C<8, 16, 12, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize8x32_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize8x32] =
      CflIntraPredictor_C<8, 32, 12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x32_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize8x32][kSubsamplingType444] =
      CflSubsampler_C<8, 32, 12, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x32_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize8x32][kSubsamplingType422] =
      CflSubsampler_C<8, 32, 12, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x32_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize8x32][kSubsamplingType420] =
      CflSubsampler_C<8, 32, 12, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize16x4_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize16x4] =
      CflIntraPredictor_C<16, 4, 12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x4_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize16x4][kSubsamplingType444] =
      CflSubsampler_C<16, 4, 12, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x4_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize16x4][kSubsamplingType422] =
      CflSubsampler_C<16, 4, 12, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x4_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize16x4][kSubsamplingType420] =
      CflSubsampler_C<16, 4, 12, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize16x8_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize16x8] =
      CflIntraPredictor_C<16, 8, 12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x8_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize16x8][kSubsamplingType444] =
      CflSubsampler_C<16, 8, 12, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x8_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize16x8][kSubsamplingType422] =
      CflSubsampler_C<16, 8, 12, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x8_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize16x8][kSubsamplingType420] =
      CflSubsampler_C<16, 8, 12, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize16x16_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize16x16] =
      CflIntraPredictor_C<16, 16, 12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x16_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize16x16][kSubsamplingType444] =
      CflSubsampler_C<16, 16, 12, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x16_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize16x16][kSubsamplingType422] =
      CflSubsampler_C<16, 16, 12, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x16_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize16x16][kSubsamplingType420] =
      CflSubsampler_C<16, 16, 12, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize16x32_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize16x32] =
      CflIntraPredictor_C<16, 32, 12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x32_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize16x32][kSubsamplingType444] =
      CflSubsampler_C<16, 32, 12, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x32_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize16x32][kSubsamplingType422] =
      CflSubsampler_C<16, 32, 12, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x32_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize16x32][kSubsamplingType420] =
      CflSubsampler_C<16, 32, 12, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize32x8_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize32x8] =
      CflIntraPredictor_C<32, 8, 12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x8_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize32x8][kSubsamplingType444] =
      CflSubsampler_C<32, 8, 12, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x8_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize32x8][kSubsamplingType422] =
      CflSubsampler_C<32, 8, 12, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x8_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize32x8][kSubsamplingType420] =
      CflSubsampler_C<32, 8, 12, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize32x16_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize32x16] =
      CflIntraPredictor_C<32, 16, 12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x16_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize32x16][kSubsamplingType444] =
      CflSubsampler_C<32, 16, 12, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x16_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize32x16][kSubsamplingType422] =
      CflSubsampler_C<32, 16, 12, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x16_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize32x16][kSubsamplingType420] =
      CflSubsampler_C<32, 16, 12, uint16_t, 1, 1>;
#endif

#ifndef LIBGAV1_Dsp12bpp_TransformSize32x32_CflIntraPredictor
  dsp->cfl_intra_predictors[kTransformSize32x32] =
      CflIntraPredictor_C<32, 32, 12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x32_CflSubsampler444
  dsp->cfl_subsamplers[kTransformSize32x32][kSubsamplingType444] =
      CflSubsampler_C<32, 32, 12, uint16_t, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x32_CflSubsampler422
  dsp->cfl_subsamplers[kTransformSize32x32][kSubsamplingType422] =
      CflSubsampler_C<32, 32, 12, uint16_t, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x32_CflSubsampler420
  dsp->cfl_subsamplers[kTransformSize32x32][kSubsamplingType420] =
      CflSubsampler_C<32, 32, 12, uint16_t, 1, 1>;
#endif

#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  // Cfl predictors are available only for transform sizes with max(width,
  // height) <= 32. Set all others to nullptr.
  for (const auto i : kTransformSizesLargerThan32x32) {
    dsp->cfl_intra_predictors[i] = nullptr;
    for (int j = 0; j < kNumSubsamplingTypes; ++j) {
      dsp->cfl_subsamplers[i][j] = nullptr;
    }
  }
}  // NOLINT(readability/fn_size)
#endif  // LIBGAV1_MAX_BITDEPTH == 12

#undef INIT_CFL_INTRAPREDICTOR_WxH
#undef INIT_CFL_INTRAPREDICTORS

}  // namespace

void IntraPredCflInit_C() {
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
