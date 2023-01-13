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

#include "src/dsp/intrapred.h"

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
#include "src/utils/memory.h"

namespace libgav1 {
namespace dsp {
namespace {

template <int block_width, int block_height, typename Pixel>
struct IntraPredFuncs_C {
  IntraPredFuncs_C() = delete;

  static void DcTop(void* dest, ptrdiff_t stride, const void* top_row,
                    const void* left_column);
  static void DcLeft(void* dest, ptrdiff_t stride, const void* top_row,
                     const void* left_column);
  static void Dc(void* dest, ptrdiff_t stride, const void* top_row,
                 const void* left_column);
  static void Vertical(void* dest, ptrdiff_t stride, const void* top_row,
                       const void* left_column);
  static void Horizontal(void* dest, ptrdiff_t stride, const void* top_row,
                         const void* left_column);
  static void Paeth(void* dest, ptrdiff_t stride, const void* top_row,
                    const void* left_column);
};

// Intra-predictors that require bitdepth.
template <int block_width, int block_height, int bitdepth, typename Pixel>
struct IntraPredBppFuncs_C {
  IntraPredBppFuncs_C() = delete;

  static void DcFill(void* dest, ptrdiff_t stride, const void* top_row,
                     const void* left_column);
};

//------------------------------------------------------------------------------
// IntraPredFuncs_C::DcPred

template <int block_width, int block_height, typename Pixel>
void IntraPredFuncs_C<block_width, block_height, Pixel>::DcTop(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row, const void* /*left_column*/) {
  int sum = block_width >> 1;  // rounder
  const auto* const top = static_cast<const Pixel*>(top_row);
  for (int x = 0; x < block_width; ++x) sum += top[x];
  const int dc = sum >> FloorLog2(block_width);

  auto* dst = static_cast<Pixel*>(dest);
  stride /= sizeof(Pixel);
  for (int y = 0; y < block_height; ++y) {
    Memset(dst, dc, block_width);
    dst += stride;
  }
}

template <int block_width, int block_height, typename Pixel>
void IntraPredFuncs_C<block_width, block_height, Pixel>::DcLeft(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* /*top_row*/, const void* LIBGAV1_RESTRICT const left_column) {
  int sum = block_height >> 1;  // rounder
  const auto* const left = static_cast<const Pixel*>(left_column);
  for (int y = 0; y < block_height; ++y) sum += left[y];
  const int dc = sum >> FloorLog2(block_height);

  auto* dst = static_cast<Pixel*>(dest);
  stride /= sizeof(Pixel);
  for (int y = 0; y < block_height; ++y) {
    Memset(dst, dc, block_width);
    dst += stride;
  }
}

// Note for square blocks the divide in the Dc() function reduces to a shift.
// For rectangular block sizes the following multipliers can be used with the
// corresponding shifts.
// 8-bit
//  1:2 (e.g,, 4x8):  scale = 0x5556
//  1:4 (e.g., 4x16): scale = 0x3334
//  final_descale = 16
// 10/12-bit
//  1:2: scale = 0xaaab
//  1:4: scale = 0x6667
//  final_descale = 17
//  Note these may be halved to the values used in 8-bit in all cases except
//  when bitdepth == 12 and block_width + block_height is divisible by 5 (as
//  opposed to 3).
//
// The calculation becomes:
//  (dc_sum >> intermediate_descale) * scale) >> final_descale
// where intermediate_descale is:
// sum = block_width + block_height
// intermediate_descale =
//     (sum <= 20) ? 2 : (sum <= 40) ? 3 : (sum <= 80) ? 4 : 5
//
// The constants (multiplier and shifts) for a given block size are obtained
// as follows:
// - Let sum = block width + block height
// - Shift 'sum' right until we reach an odd number
// - Let the number of shifts for that block size be called 'intermediate_scale'
//   and let the odd number be 'd' (d has only 2 possible values: d = 3 for a
//   1:2 rectangular block and d = 5 for a 1:4 rectangular block).
// - Find multipliers by dividing by 'd' using "Algorithm 1" in:
//   http://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=1467632
//   by ensuring that m + n = 16 (in that algorithm). This ensures that our 2nd
//   shift will be 16, regardless of the block size.
// TODO(jzern): the base implementation could be updated to use this method.

template <int block_width, int block_height, typename Pixel>
void IntraPredFuncs_C<block_width, block_height, Pixel>::Dc(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const int divisor = block_width + block_height;
  int sum = divisor >> 1;  // rounder

  const auto* const top = static_cast<const Pixel*>(top_row);
  const auto* const left = static_cast<const Pixel*>(left_column);
  for (int x = 0; x < block_width; ++x) sum += top[x];
  for (int y = 0; y < block_height; ++y) sum += left[y];

  const int dc = sum / divisor;

  auto* dst = static_cast<Pixel*>(dest);
  stride /= sizeof(Pixel);
  for (int y = 0; y < block_height; ++y) {
    Memset(dst, dc, block_width);
    dst += stride;
  }
}

//------------------------------------------------------------------------------
// IntraPredFuncs_C directional predictors

// IntraPredFuncs_C::Vertical -- apply top row vertically
template <int block_width, int block_height, typename Pixel>
void IntraPredFuncs_C<block_width, block_height, Pixel>::Vertical(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row, const void* /*left_column*/) {
  auto* dst = static_cast<uint8_t*>(dest);
  for (int y = 0; y < block_height; ++y) {
    memcpy(dst, top_row, block_width * sizeof(Pixel));
    dst += stride;
  }
}

// IntraPredFuncs_C::Horizontal -- apply left column horizontally
template <int block_width, int block_height, typename Pixel>
void IntraPredFuncs_C<block_width, block_height, Pixel>::Horizontal(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* /*top_row*/, const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left = static_cast<const Pixel*>(left_column);
  auto* dst = static_cast<Pixel*>(dest);
  stride /= sizeof(Pixel);
  for (int y = 0; y < block_height; ++y) {
    Memset(dst, left[y], block_width);
    dst += stride;
  }
}

// IntraPredFuncs_C::Paeth
template <int block_width, int block_height, typename Pixel>
void IntraPredFuncs_C<block_width, block_height, Pixel>::Paeth(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const Pixel*>(top_row);
  const auto* const left = static_cast<const Pixel*>(left_column);
  const Pixel top_left = top[-1];
  const int top_left_x2 = top_left + top_left;
  auto* dst = static_cast<Pixel*>(dest);
  stride /= sizeof(Pixel);

  for (int y = 0; y < block_height; ++y) {
    const int left_pixel = left[y];
    for (int x = 0; x < block_width; ++x) {
      // The Paeth filter selects the value closest to:
      // top[x] + left[y] - top_left
      // To calculate the absolute distance for the left value this would be:
      // abs((top[x] + left[y] - top_left) - left[y])
      // or, because left[y] cancels out:
      // abs(top[x] - top_left)
      const int left_dist = std::abs(top[x] - top_left);
      const int top_dist = std::abs(left_pixel - top_left);
      const int top_left_dist = std::abs(top[x] + left_pixel - top_left_x2);

      // Select the closest value to the initial estimate of 'T + L - TL'.
      if (left_dist <= top_dist && left_dist <= top_left_dist) {
        dst[x] = left_pixel;
      } else if (top_dist <= top_left_dist) {
        dst[x] = top[x];
      } else {
        dst[x] = top_left;
      }
    }
    dst += stride;
  }
}

//------------------------------------------------------------------------------
// IntraPredBppFuncs_C
template <int fill, typename Pixel>
inline void DcFill_C(void* const dest, ptrdiff_t stride, const int block_width,
                     const int block_height) {
  static_assert(sizeof(Pixel) == 1 || sizeof(Pixel) == 2,
                "Only 1 & 2 byte pixels are supported");

  auto* dst = static_cast<Pixel*>(dest);
  stride /= sizeof(Pixel);
  for (int y = 0; y < block_height; ++y) {
    Memset(dst, fill, block_width);
    dst += stride;
  }
}

template <int block_width, int block_height, int bitdepth, typename Pixel>
void IntraPredBppFuncs_C<block_width, block_height, bitdepth, Pixel>::DcFill(
    void* const dest, ptrdiff_t stride, const void* /*top_row*/,
    const void* /*left_column*/) {
  DcFill_C<0x80 << (bitdepth - 8), Pixel>(dest, stride, block_width,
                                          block_height);
}

// -----------------------------------------------------------------------------

template <typename Pixel>
struct IntraPredDefs {
  IntraPredDefs() = delete;

  using _4x4 = IntraPredFuncs_C<4, 4, Pixel>;
  using _4x8 = IntraPredFuncs_C<4, 8, Pixel>;
  using _4x16 = IntraPredFuncs_C<4, 16, Pixel>;
  using _8x4 = IntraPredFuncs_C<8, 4, Pixel>;
  using _8x8 = IntraPredFuncs_C<8, 8, Pixel>;
  using _8x16 = IntraPredFuncs_C<8, 16, Pixel>;
  using _8x32 = IntraPredFuncs_C<8, 32, Pixel>;
  using _16x4 = IntraPredFuncs_C<16, 4, Pixel>;
  using _16x8 = IntraPredFuncs_C<16, 8, Pixel>;
  using _16x16 = IntraPredFuncs_C<16, 16, Pixel>;
  using _16x32 = IntraPredFuncs_C<16, 32, Pixel>;
  using _16x64 = IntraPredFuncs_C<16, 64, Pixel>;
  using _32x8 = IntraPredFuncs_C<32, 8, Pixel>;
  using _32x16 = IntraPredFuncs_C<32, 16, Pixel>;
  using _32x32 = IntraPredFuncs_C<32, 32, Pixel>;
  using _32x64 = IntraPredFuncs_C<32, 64, Pixel>;
  using _64x16 = IntraPredFuncs_C<64, 16, Pixel>;
  using _64x32 = IntraPredFuncs_C<64, 32, Pixel>;
  using _64x64 = IntraPredFuncs_C<64, 64, Pixel>;
};

template <int bitdepth, typename Pixel>
struct IntraPredBppDefs {
  IntraPredBppDefs() = delete;

  using _4x4 = IntraPredBppFuncs_C<4, 4, bitdepth, Pixel>;
  using _4x8 = IntraPredBppFuncs_C<4, 8, bitdepth, Pixel>;
  using _4x16 = IntraPredBppFuncs_C<4, 16, bitdepth, Pixel>;
  using _8x4 = IntraPredBppFuncs_C<8, 4, bitdepth, Pixel>;
  using _8x8 = IntraPredBppFuncs_C<8, 8, bitdepth, Pixel>;
  using _8x16 = IntraPredBppFuncs_C<8, 16, bitdepth, Pixel>;
  using _8x32 = IntraPredBppFuncs_C<8, 32, bitdepth, Pixel>;
  using _16x4 = IntraPredBppFuncs_C<16, 4, bitdepth, Pixel>;
  using _16x8 = IntraPredBppFuncs_C<16, 8, bitdepth, Pixel>;
  using _16x16 = IntraPredBppFuncs_C<16, 16, bitdepth, Pixel>;
  using _16x32 = IntraPredBppFuncs_C<16, 32, bitdepth, Pixel>;
  using _16x64 = IntraPredBppFuncs_C<16, 64, bitdepth, Pixel>;
  using _32x8 = IntraPredBppFuncs_C<32, 8, bitdepth, Pixel>;
  using _32x16 = IntraPredBppFuncs_C<32, 16, bitdepth, Pixel>;
  using _32x32 = IntraPredBppFuncs_C<32, 32, bitdepth, Pixel>;
  using _32x64 = IntraPredBppFuncs_C<32, 64, bitdepth, Pixel>;
  using _64x16 = IntraPredBppFuncs_C<64, 16, bitdepth, Pixel>;
  using _64x32 = IntraPredBppFuncs_C<64, 32, bitdepth, Pixel>;
  using _64x64 = IntraPredBppFuncs_C<64, 64, bitdepth, Pixel>;
};

using Defs = IntraPredDefs<uint8_t>;
using Defs8bpp = IntraPredBppDefs<8, uint8_t>;

// Initializes dsp entries for kTransformSize|W|x|H| from |DEFS|/|DEFSBPP| of
// the same size.
#define INIT_INTRAPREDICTORS_WxH(DEFS, DEFSBPP, W, H)                         \
  dsp->intra_predictors[kTransformSize##W##x##H][kIntraPredictorDcFill] =     \
      DEFSBPP::_##W##x##H::DcFill;                                            \
  dsp->intra_predictors[kTransformSize##W##x##H][kIntraPredictorDcTop] =      \
      DEFS::_##W##x##H::DcTop;                                                \
  dsp->intra_predictors[kTransformSize##W##x##H][kIntraPredictorDcLeft] =     \
      DEFS::_##W##x##H::DcLeft;                                               \
  dsp->intra_predictors[kTransformSize##W##x##H][kIntraPredictorDc] =         \
      DEFS::_##W##x##H::Dc;                                                   \
  dsp->intra_predictors[kTransformSize##W##x##H][kIntraPredictorVertical] =   \
      DEFS::_##W##x##H::Vertical;                                             \
  dsp->intra_predictors[kTransformSize##W##x##H][kIntraPredictorHorizontal] = \
      DEFS::_##W##x##H::Horizontal;                                           \
  dsp->intra_predictors[kTransformSize##W##x##H][kIntraPredictorPaeth] =      \
      DEFS::_##W##x##H::Paeth

#define INIT_INTRAPREDICTORS(DEFS, DEFSBPP)        \
  INIT_INTRAPREDICTORS_WxH(DEFS, DEFSBPP, 4, 4);   \
  INIT_INTRAPREDICTORS_WxH(DEFS, DEFSBPP, 4, 8);   \
  INIT_INTRAPREDICTORS_WxH(DEFS, DEFSBPP, 4, 16);  \
  INIT_INTRAPREDICTORS_WxH(DEFS, DEFSBPP, 8, 4);   \
  INIT_INTRAPREDICTORS_WxH(DEFS, DEFSBPP, 8, 8);   \
  INIT_INTRAPREDICTORS_WxH(DEFS, DEFSBPP, 8, 16);  \
  INIT_INTRAPREDICTORS_WxH(DEFS, DEFSBPP, 8, 32);  \
  INIT_INTRAPREDICTORS_WxH(DEFS, DEFSBPP, 16, 4);  \
  INIT_INTRAPREDICTORS_WxH(DEFS, DEFSBPP, 16, 8);  \
  INIT_INTRAPREDICTORS_WxH(DEFS, DEFSBPP, 16, 16); \
  INIT_INTRAPREDICTORS_WxH(DEFS, DEFSBPP, 16, 32); \
  INIT_INTRAPREDICTORS_WxH(DEFS, DEFSBPP, 16, 64); \
  INIT_INTRAPREDICTORS_WxH(DEFS, DEFSBPP, 32, 8);  \
  INIT_INTRAPREDICTORS_WxH(DEFS, DEFSBPP, 32, 16); \
  INIT_INTRAPREDICTORS_WxH(DEFS, DEFSBPP, 32, 32); \
  INIT_INTRAPREDICTORS_WxH(DEFS, DEFSBPP, 32, 64); \
  INIT_INTRAPREDICTORS_WxH(DEFS, DEFSBPP, 64, 16); \
  INIT_INTRAPREDICTORS_WxH(DEFS, DEFSBPP, 64, 32); \
  INIT_INTRAPREDICTORS_WxH(DEFS, DEFSBPP, 64, 64)

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(8);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  INIT_INTRAPREDICTORS(Defs, Defs8bpp);
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x4_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDcFill] =
      Defs8bpp::_4x4::DcFill;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x4_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDcTop] =
      Defs::_4x4::DcTop;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x4_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDcLeft] =
      Defs::_4x4::DcLeft;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x4_IntraPredictorDc
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDc] = Defs::_4x4::Dc;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x4_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorVertical] =
      Defs::_4x4::Vertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x4_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorHorizontal] =
      Defs::_4x4::Horizontal;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x4_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorPaeth] =
      Defs::_4x4::Paeth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x8_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorDcFill] =
      Defs8bpp::_4x8::DcFill;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x8_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorDcTop] =
      Defs::_4x8::DcTop;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x8_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorDcLeft] =
      Defs::_4x8::DcLeft;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x8_IntraPredictorDc
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorDc] = Defs::_4x8::Dc;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x8_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorVertical] =
      Defs::_4x8::Vertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x8_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorHorizontal] =
      Defs::_4x8::Horizontal;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x8_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorPaeth] =
      Defs::_4x8::Paeth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x16_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorDcFill] =
      Defs8bpp::_4x16::DcFill;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x16_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorDcTop] =
      Defs::_4x16::DcTop;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x16_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorDcLeft] =
      Defs::_4x16::DcLeft;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x16_IntraPredictorDc
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorDc] =
      Defs::_4x16::Dc;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x16_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorVertical] =
      Defs::_4x16::Vertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x16_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorHorizontal] =
      Defs::_4x16::Horizontal;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize4x16_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorPaeth] =
      Defs::_4x16::Paeth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x4_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorDcFill] =
      Defs8bpp::_8x4::DcFill;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x4_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorDcTop] =
      Defs::_8x4::DcTop;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x4_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorDcLeft] =
      Defs::_8x4::DcLeft;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x4_IntraPredictorDc
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorDc] = Defs::_8x4::Dc;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x4_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorVertical] =
      Defs::_8x4::Vertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x4_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorHorizontal] =
      Defs::_8x4::Horizontal;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x4_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorPaeth] =
      Defs::_8x4::Paeth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x8_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorDcFill] =
      Defs8bpp::_8x8::DcFill;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x8_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorDcTop] =
      Defs::_8x8::DcTop;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x8_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorDcLeft] =
      Defs::_8x8::DcLeft;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x8_IntraPredictorDc
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorDc] = Defs::_8x8::Dc;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x8_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorVertical] =
      Defs::_8x8::Vertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x8_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorHorizontal] =
      Defs::_8x8::Horizontal;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x8_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorPaeth] =
      Defs::_8x8::Paeth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x16_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorDcFill] =
      Defs8bpp::_8x16::DcFill;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x16_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorDcTop] =
      Defs::_8x16::DcTop;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x16_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorDcLeft] =
      Defs::_8x16::DcLeft;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x16_IntraPredictorDc
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorDc] =
      Defs::_8x16::Dc;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x16_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorVertical] =
      Defs::_8x16::Vertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x16_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorHorizontal] =
      Defs::_8x16::Horizontal;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x16_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorPaeth] =
      Defs::_8x16::Paeth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x32_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorDcFill] =
      Defs8bpp::_8x32::DcFill;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x32_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorDcTop] =
      Defs::_8x32::DcTop;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x32_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorDcLeft] =
      Defs::_8x32::DcLeft;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x32_IntraPredictorDc
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorDc] =
      Defs::_8x32::Dc;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x32_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorVertical] =
      Defs::_8x32::Vertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x32_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorHorizontal] =
      Defs::_8x32::Horizontal;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize8x32_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorPaeth] =
      Defs::_8x32::Paeth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x4_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorDcFill] =
      Defs8bpp::_16x4::DcFill;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x4_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorDcTop] =
      Defs::_16x4::DcTop;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x4_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorDcLeft] =
      Defs::_16x4::DcLeft;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x4_IntraPredictorDc
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorDc] =
      Defs::_16x4::Dc;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x4_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorVertical] =
      Defs::_16x4::Vertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x4_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorHorizontal] =
      Defs::_16x4::Horizontal;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x4_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorPaeth] =
      Defs::_16x4::Paeth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x8_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorDcFill] =
      Defs8bpp::_16x8::DcFill;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x8_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorDcTop] =
      Defs::_16x8::DcTop;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x8_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorDcLeft] =
      Defs::_16x8::DcLeft;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x8_IntraPredictorDc
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorDc] =
      Defs::_16x8::Dc;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x8_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorVertical] =
      Defs::_16x8::Vertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x8_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorHorizontal] =
      Defs::_16x8::Horizontal;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x8_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorPaeth] =
      Defs::_16x8::Paeth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x16_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorDcFill] =
      Defs8bpp::_16x16::DcFill;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x16_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorDcTop] =
      Defs::_16x16::DcTop;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x16_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorDcLeft] =
      Defs::_16x16::DcLeft;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x16_IntraPredictorDc
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorDc] =
      Defs::_16x16::Dc;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x16_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorVertical] =
      Defs::_16x16::Vertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x16_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorHorizontal] =
      Defs::_16x16::Horizontal;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x16_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorPaeth] =
      Defs::_16x16::Paeth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x32_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorDcFill] =
      Defs8bpp::_16x32::DcFill;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x32_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorDcTop] =
      Defs::_16x32::DcTop;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x32_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorDcLeft] =
      Defs::_16x32::DcLeft;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x32_IntraPredictorDc
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorDc] =
      Defs::_16x32::Dc;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x32_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorVertical] =
      Defs::_16x32::Vertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x32_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorHorizontal] =
      Defs::_16x32::Horizontal;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x32_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorPaeth] =
      Defs::_16x32::Paeth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x64_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorDcFill] =
      Defs8bpp::_16x64::DcFill;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x64_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorDcTop] =
      Defs::_16x64::DcTop;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x64_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorDcLeft] =
      Defs::_16x64::DcLeft;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x64_IntraPredictorDc
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorDc] =
      Defs::_16x64::Dc;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x64_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorVertical] =
      Defs::_16x64::Vertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x64_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorHorizontal] =
      Defs::_16x64::Horizontal;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize16x64_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorPaeth] =
      Defs::_16x64::Paeth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x8_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorDcFill] =
      Defs8bpp::_32x8::DcFill;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x8_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorDcTop] =
      Defs::_32x8::DcTop;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x8_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorDcLeft] =
      Defs::_32x8::DcLeft;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x8_IntraPredictorDc
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorDc] =
      Defs::_32x8::Dc;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x8_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorVertical] =
      Defs::_32x8::Vertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x8_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorHorizontal] =
      Defs::_32x8::Horizontal;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x8_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorPaeth] =
      Defs::_32x8::Paeth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x16_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorDcFill] =
      Defs8bpp::_32x16::DcFill;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x16_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorDcTop] =
      Defs::_32x16::DcTop;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x16_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorDcLeft] =
      Defs::_32x16::DcLeft;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x16_IntraPredictorDc
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorDc] =
      Defs::_32x16::Dc;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x16_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorVertical] =
      Defs::_32x16::Vertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x16_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorHorizontal] =
      Defs::_32x16::Horizontal;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x16_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorPaeth] =
      Defs::_32x16::Paeth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x32_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorDcFill] =
      Defs8bpp::_32x32::DcFill;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x32_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorDcTop] =
      Defs::_32x32::DcTop;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x32_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorDcLeft] =
      Defs::_32x32::DcLeft;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x32_IntraPredictorDc
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorDc] =
      Defs::_32x32::Dc;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x32_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorVertical] =
      Defs::_32x32::Vertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x32_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorHorizontal] =
      Defs::_32x32::Horizontal;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x32_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorPaeth] =
      Defs::_32x32::Paeth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x64_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorDcFill] =
      Defs8bpp::_32x64::DcFill;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x64_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorDcTop] =
      Defs::_32x64::DcTop;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x64_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorDcLeft] =
      Defs::_32x64::DcLeft;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x64_IntraPredictorDc
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorDc] =
      Defs::_32x64::Dc;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x64_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorVertical] =
      Defs::_32x64::Vertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x64_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorHorizontal] =
      Defs::_32x64::Horizontal;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize32x64_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorPaeth] =
      Defs::_32x64::Paeth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x16_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorDcFill] =
      Defs8bpp::_64x16::DcFill;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x16_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorDcTop] =
      Defs::_64x16::DcTop;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x16_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorDcLeft] =
      Defs::_64x16::DcLeft;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x16_IntraPredictorDc
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorDc] =
      Defs::_64x16::Dc;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x16_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorVertical] =
      Defs::_64x16::Vertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x16_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorHorizontal] =
      Defs::_64x16::Horizontal;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x16_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorPaeth] =
      Defs::_64x16::Paeth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x32_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorDcFill] =
      Defs8bpp::_64x32::DcFill;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x32_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorDcTop] =
      Defs::_64x32::DcTop;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x32_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorDcLeft] =
      Defs::_64x32::DcLeft;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x32_IntraPredictorDc
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorDc] =
      Defs::_64x32::Dc;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x32_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorVertical] =
      Defs::_64x32::Vertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x32_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorHorizontal] =
      Defs::_64x32::Horizontal;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x32_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorPaeth] =
      Defs::_64x32::Paeth;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x64_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorDcFill] =
      Defs8bpp::_64x64::DcFill;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x64_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorDcTop] =
      Defs::_64x64::DcTop;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x64_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorDcLeft] =
      Defs::_64x64::DcLeft;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x64_IntraPredictorDc
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorDc] =
      Defs::_64x64::Dc;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x64_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorVertical] =
      Defs::_64x64::Vertical;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x64_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorHorizontal] =
      Defs::_64x64::Horizontal;
#endif
#ifndef LIBGAV1_Dsp8bpp_TransformSize64x64_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorPaeth] =
      Defs::_64x64::Paeth;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}  // NOLINT(readability/fn_size)

#if LIBGAV1_MAX_BITDEPTH >= 10
using DefsHbd = IntraPredDefs<uint16_t>;
using Defs10bpp = IntraPredBppDefs<10, uint16_t>;

void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(10);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  INIT_INTRAPREDICTORS(DefsHbd, Defs10bpp);
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x4_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDcFill] =
      Defs10bpp::_4x4::DcFill;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x4_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDcTop] =
      DefsHbd::_4x4::DcTop;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x4_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDcLeft] =
      DefsHbd::_4x4::DcLeft;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x4_IntraPredictorDc
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDc] =
      DefsHbd::_4x4::Dc;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x4_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorVertical] =
      DefsHbd::_4x4::Vertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x4_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorHorizontal] =
      DefsHbd::_4x4::Horizontal;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x4_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorPaeth] =
      DefsHbd::_4x4::Paeth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x8_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorDcFill] =
      Defs10bpp::_4x8::DcFill;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x8_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorDcTop] =
      DefsHbd::_4x8::DcTop;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x8_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorDcLeft] =
      DefsHbd::_4x8::DcLeft;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x8_IntraPredictorDc
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorDc] =
      DefsHbd::_4x8::Dc;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x8_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorVertical] =
      DefsHbd::_4x8::Vertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x8_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorHorizontal] =
      DefsHbd::_4x8::Horizontal;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x8_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorPaeth] =
      DefsHbd::_4x8::Paeth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x16_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorDcFill] =
      Defs10bpp::_4x16::DcFill;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x16_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorDcTop] =
      DefsHbd::_4x16::DcTop;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x16_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorDcLeft] =
      DefsHbd::_4x16::DcLeft;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x16_IntraPredictorDc
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorDc] =
      DefsHbd::_4x16::Dc;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x16_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorVertical] =
      DefsHbd::_4x16::Vertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x16_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorHorizontal] =
      DefsHbd::_4x16::Horizontal;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize4x16_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorPaeth] =
      DefsHbd::_4x16::Paeth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x4_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorDcFill] =
      Defs10bpp::_8x4::DcFill;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x4_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorDcTop] =
      DefsHbd::_8x4::DcTop;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x4_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorDcLeft] =
      DefsHbd::_8x4::DcLeft;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x4_IntraPredictorDc
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorDc] =
      DefsHbd::_8x4::Dc;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x4_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorVertical] =
      DefsHbd::_8x4::Vertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x4_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorHorizontal] =
      DefsHbd::_8x4::Horizontal;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x4_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorPaeth] =
      DefsHbd::_8x4::Paeth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x8_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorDcFill] =
      Defs10bpp::_8x8::DcFill;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x8_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorDcTop] =
      DefsHbd::_8x8::DcTop;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x8_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorDcLeft] =
      DefsHbd::_8x8::DcLeft;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x8_IntraPredictorDc
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorDc] =
      DefsHbd::_8x8::Dc;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x8_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorVertical] =
      DefsHbd::_8x8::Vertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x8_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorHorizontal] =
      DefsHbd::_8x8::Horizontal;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x8_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorPaeth] =
      DefsHbd::_8x8::Paeth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x16_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorDcFill] =
      Defs10bpp::_8x16::DcFill;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x16_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorDcTop] =
      DefsHbd::_8x16::DcTop;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x16_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorDcLeft] =
      DefsHbd::_8x16::DcLeft;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x16_IntraPredictorDc
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorDc] =
      DefsHbd::_8x16::Dc;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x16_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorVertical] =
      DefsHbd::_8x16::Vertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x16_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorHorizontal] =
      DefsHbd::_8x16::Horizontal;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x16_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorPaeth] =
      DefsHbd::_8x16::Paeth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x32_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorDcFill] =
      Defs10bpp::_8x32::DcFill;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x32_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorDcTop] =
      DefsHbd::_8x32::DcTop;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x32_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorDcLeft] =
      DefsHbd::_8x32::DcLeft;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x32_IntraPredictorDc
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorDc] =
      DefsHbd::_8x32::Dc;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x32_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorVertical] =
      DefsHbd::_8x32::Vertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x32_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorHorizontal] =
      DefsHbd::_8x32::Horizontal;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize8x32_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorPaeth] =
      DefsHbd::_8x32::Paeth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x4_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorDcFill] =
      Defs10bpp::_16x4::DcFill;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x4_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorDcTop] =
      DefsHbd::_16x4::DcTop;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x4_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorDcLeft] =
      DefsHbd::_16x4::DcLeft;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x4_IntraPredictorDc
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorDc] =
      DefsHbd::_16x4::Dc;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x4_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorVertical] =
      DefsHbd::_16x4::Vertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x4_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorHorizontal] =
      DefsHbd::_16x4::Horizontal;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x4_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorPaeth] =
      DefsHbd::_16x4::Paeth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x8_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorDcFill] =
      Defs10bpp::_16x8::DcFill;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x8_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorDcTop] =
      DefsHbd::_16x8::DcTop;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x8_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorDcLeft] =
      DefsHbd::_16x8::DcLeft;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x8_IntraPredictorDc
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorDc] =
      DefsHbd::_16x8::Dc;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x8_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorVertical] =
      DefsHbd::_16x8::Vertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x8_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorHorizontal] =
      DefsHbd::_16x8::Horizontal;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x8_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorPaeth] =
      DefsHbd::_16x8::Paeth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x16_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorDcFill] =
      Defs10bpp::_16x16::DcFill;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x16_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorDcTop] =
      DefsHbd::_16x16::DcTop;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x16_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorDcLeft] =
      DefsHbd::_16x16::DcLeft;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x16_IntraPredictorDc
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorDc] =
      DefsHbd::_16x16::Dc;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x16_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorVertical] =
      DefsHbd::_16x16::Vertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x16_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorHorizontal] =
      DefsHbd::_16x16::Horizontal;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x16_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorPaeth] =
      DefsHbd::_16x16::Paeth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x32_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorDcFill] =
      Defs10bpp::_16x32::DcFill;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x32_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorDcTop] =
      DefsHbd::_16x32::DcTop;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x32_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorDcLeft] =
      DefsHbd::_16x32::DcLeft;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x32_IntraPredictorDc
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorDc] =
      DefsHbd::_16x32::Dc;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x32_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorVertical] =
      DefsHbd::_16x32::Vertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x32_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorHorizontal] =
      DefsHbd::_16x32::Horizontal;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x32_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorPaeth] =
      DefsHbd::_16x32::Paeth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x64_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorDcFill] =
      Defs10bpp::_16x64::DcFill;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x64_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorDcTop] =
      DefsHbd::_16x64::DcTop;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x64_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorDcLeft] =
      DefsHbd::_16x64::DcLeft;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x64_IntraPredictorDc
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorDc] =
      DefsHbd::_16x64::Dc;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x64_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorVertical] =
      DefsHbd::_16x64::Vertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x64_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorHorizontal] =
      DefsHbd::_16x64::Horizontal;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize16x64_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorPaeth] =
      DefsHbd::_16x64::Paeth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x8_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorDcFill] =
      Defs10bpp::_32x8::DcFill;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x8_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorDcTop] =
      DefsHbd::_32x8::DcTop;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x8_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorDcLeft] =
      DefsHbd::_32x8::DcLeft;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x8_IntraPredictorDc
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorDc] =
      DefsHbd::_32x8::Dc;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x8_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorVertical] =
      DefsHbd::_32x8::Vertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x8_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorHorizontal] =
      DefsHbd::_32x8::Horizontal;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x8_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorPaeth] =
      DefsHbd::_32x8::Paeth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x16_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorDcFill] =
      Defs10bpp::_32x16::DcFill;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x16_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorDcTop] =
      DefsHbd::_32x16::DcTop;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x16_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorDcLeft] =
      DefsHbd::_32x16::DcLeft;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x16_IntraPredictorDc
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorDc] =
      DefsHbd::_32x16::Dc;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x16_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorVertical] =
      DefsHbd::_32x16::Vertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x16_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorHorizontal] =
      DefsHbd::_32x16::Horizontal;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x16_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorPaeth] =
      DefsHbd::_32x16::Paeth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x32_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorDcFill] =
      Defs10bpp::_32x32::DcFill;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x32_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorDcTop] =
      DefsHbd::_32x32::DcTop;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x32_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorDcLeft] =
      DefsHbd::_32x32::DcLeft;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x32_IntraPredictorDc
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorDc] =
      DefsHbd::_32x32::Dc;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x32_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorVertical] =
      DefsHbd::_32x32::Vertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x32_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorHorizontal] =
      DefsHbd::_32x32::Horizontal;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x32_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorPaeth] =
      DefsHbd::_32x32::Paeth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x64_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorDcFill] =
      Defs10bpp::_32x64::DcFill;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x64_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorDcTop] =
      DefsHbd::_32x64::DcTop;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x64_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorDcLeft] =
      DefsHbd::_32x64::DcLeft;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x64_IntraPredictorDc
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorDc] =
      DefsHbd::_32x64::Dc;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x64_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorVertical] =
      DefsHbd::_32x64::Vertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x64_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorHorizontal] =
      DefsHbd::_32x64::Horizontal;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize32x64_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorPaeth] =
      DefsHbd::_32x64::Paeth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x16_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorDcFill] =
      Defs10bpp::_64x16::DcFill;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x16_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorDcTop] =
      DefsHbd::_64x16::DcTop;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x16_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorDcLeft] =
      DefsHbd::_64x16::DcLeft;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x16_IntraPredictorDc
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorDc] =
      DefsHbd::_64x16::Dc;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x16_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorVertical] =
      DefsHbd::_64x16::Vertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x16_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorHorizontal] =
      DefsHbd::_64x16::Horizontal;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x16_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorPaeth] =
      DefsHbd::_64x16::Paeth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x32_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorDcFill] =
      Defs10bpp::_64x32::DcFill;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x32_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorDcTop] =
      DefsHbd::_64x32::DcTop;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x32_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorDcLeft] =
      DefsHbd::_64x32::DcLeft;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x32_IntraPredictorDc
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorDc] =
      DefsHbd::_64x32::Dc;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x32_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorVertical] =
      DefsHbd::_64x32::Vertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x32_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorHorizontal] =
      DefsHbd::_64x32::Horizontal;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x32_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorPaeth] =
      DefsHbd::_64x32::Paeth;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x64_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorDcFill] =
      Defs10bpp::_64x64::DcFill;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x64_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorDcTop] =
      DefsHbd::_64x64::DcTop;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x64_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorDcLeft] =
      DefsHbd::_64x64::DcLeft;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x64_IntraPredictorDc
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorDc] =
      DefsHbd::_64x64::Dc;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x64_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorVertical] =
      DefsHbd::_64x64::Vertical;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x64_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorHorizontal] =
      DefsHbd::_64x64::Horizontal;
#endif
#ifndef LIBGAV1_Dsp10bpp_TransformSize64x64_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorPaeth] =
      DefsHbd::_64x64::Paeth;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}  // NOLINT(readability/fn_size)
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using Defs12bpp = IntraPredBppDefs<12, uint16_t>;

void Init12bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(12);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  INIT_INTRAPREDICTORS(DefsHbd, Defs12bpp);
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x4_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDcFill] =
      Defs12bpp::_4x4::DcFill;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x4_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDcTop] =
      DefsHbd::_4x4::DcTop;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x4_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDcLeft] =
      DefsHbd::_4x4::DcLeft;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x4_IntraPredictorDc
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDc] =
      DefsHbd::_4x4::Dc;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x4_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorVertical] =
      DefsHbd::_4x4::Vertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x4_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorHorizontal] =
      DefsHbd::_4x4::Horizontal;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x4_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorPaeth] =
      DefsHbd::_4x4::Paeth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x8_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorDcFill] =
      Defs12bpp::_4x8::DcFill;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x8_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorDcTop] =
      DefsHbd::_4x8::DcTop;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x8_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorDcLeft] =
      DefsHbd::_4x8::DcLeft;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x8_IntraPredictorDc
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorDc] =
      DefsHbd::_4x8::Dc;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x8_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorVertical] =
      DefsHbd::_4x8::Vertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x8_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorHorizontal] =
      DefsHbd::_4x8::Horizontal;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x8_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorPaeth] =
      DefsHbd::_4x8::Paeth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x16_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorDcFill] =
      Defs12bpp::_4x16::DcFill;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x16_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorDcTop] =
      DefsHbd::_4x16::DcTop;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x16_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorDcLeft] =
      DefsHbd::_4x16::DcLeft;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x16_IntraPredictorDc
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorDc] =
      DefsHbd::_4x16::Dc;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x16_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorVertical] =
      DefsHbd::_4x16::Vertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x16_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorHorizontal] =
      DefsHbd::_4x16::Horizontal;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize4x16_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorPaeth] =
      DefsHbd::_4x16::Paeth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x4_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorDcFill] =
      Defs12bpp::_8x4::DcFill;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x4_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorDcTop] =
      DefsHbd::_8x4::DcTop;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x4_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorDcLeft] =
      DefsHbd::_8x4::DcLeft;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x4_IntraPredictorDc
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorDc] =
      DefsHbd::_8x4::Dc;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x4_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorVertical] =
      DefsHbd::_8x4::Vertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x4_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorHorizontal] =
      DefsHbd::_8x4::Horizontal;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x4_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorPaeth] =
      DefsHbd::_8x4::Paeth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x8_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorDcFill] =
      Defs12bpp::_8x8::DcFill;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x8_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorDcTop] =
      DefsHbd::_8x8::DcTop;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x8_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorDcLeft] =
      DefsHbd::_8x8::DcLeft;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x8_IntraPredictorDc
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorDc] =
      DefsHbd::_8x8::Dc;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x8_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorVertical] =
      DefsHbd::_8x8::Vertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x8_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorHorizontal] =
      DefsHbd::_8x8::Horizontal;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x8_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorPaeth] =
      DefsHbd::_8x8::Paeth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x16_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorDcFill] =
      Defs12bpp::_8x16::DcFill;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x16_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorDcTop] =
      DefsHbd::_8x16::DcTop;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x16_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorDcLeft] =
      DefsHbd::_8x16::DcLeft;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x16_IntraPredictorDc
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorDc] =
      DefsHbd::_8x16::Dc;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x16_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorVertical] =
      DefsHbd::_8x16::Vertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x16_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorHorizontal] =
      DefsHbd::_8x16::Horizontal;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x16_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorPaeth] =
      DefsHbd::_8x16::Paeth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x32_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorDcFill] =
      Defs12bpp::_8x32::DcFill;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x32_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorDcTop] =
      DefsHbd::_8x32::DcTop;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x32_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorDcLeft] =
      DefsHbd::_8x32::DcLeft;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x32_IntraPredictorDc
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorDc] =
      DefsHbd::_8x32::Dc;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x32_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorVertical] =
      DefsHbd::_8x32::Vertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x32_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorHorizontal] =
      DefsHbd::_8x32::Horizontal;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize8x32_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorPaeth] =
      DefsHbd::_8x32::Paeth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x4_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorDcFill] =
      Defs12bpp::_16x4::DcFill;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x4_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorDcTop] =
      DefsHbd::_16x4::DcTop;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x4_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorDcLeft] =
      DefsHbd::_16x4::DcLeft;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x4_IntraPredictorDc
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorDc] =
      DefsHbd::_16x4::Dc;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x4_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorVertical] =
      DefsHbd::_16x4::Vertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x4_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorHorizontal] =
      DefsHbd::_16x4::Horizontal;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x4_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorPaeth] =
      DefsHbd::_16x4::Paeth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x8_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorDcFill] =
      Defs12bpp::_16x8::DcFill;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x8_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorDcTop] =
      DefsHbd::_16x8::DcTop;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x8_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorDcLeft] =
      DefsHbd::_16x8::DcLeft;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x8_IntraPredictorDc
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorDc] =
      DefsHbd::_16x8::Dc;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x8_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorVertical] =
      DefsHbd::_16x8::Vertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x8_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorHorizontal] =
      DefsHbd::_16x8::Horizontal;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x8_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorPaeth] =
      DefsHbd::_16x8::Paeth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x16_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorDcFill] =
      Defs12bpp::_16x16::DcFill;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x16_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorDcTop] =
      DefsHbd::_16x16::DcTop;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x16_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorDcLeft] =
      DefsHbd::_16x16::DcLeft;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x16_IntraPredictorDc
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorDc] =
      DefsHbd::_16x16::Dc;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x16_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorVertical] =
      DefsHbd::_16x16::Vertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x16_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorHorizontal] =
      DefsHbd::_16x16::Horizontal;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x16_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorPaeth] =
      DefsHbd::_16x16::Paeth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x32_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorDcFill] =
      Defs12bpp::_16x32::DcFill;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x32_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorDcTop] =
      DefsHbd::_16x32::DcTop;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x32_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorDcLeft] =
      DefsHbd::_16x32::DcLeft;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x32_IntraPredictorDc
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorDc] =
      DefsHbd::_16x32::Dc;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x32_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorVertical] =
      DefsHbd::_16x32::Vertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x32_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorHorizontal] =
      DefsHbd::_16x32::Horizontal;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x32_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorPaeth] =
      DefsHbd::_16x32::Paeth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x64_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorDcFill] =
      Defs12bpp::_16x64::DcFill;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x64_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorDcTop] =
      DefsHbd::_16x64::DcTop;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x64_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorDcLeft] =
      DefsHbd::_16x64::DcLeft;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x64_IntraPredictorDc
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorDc] =
      DefsHbd::_16x64::Dc;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x64_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorVertical] =
      DefsHbd::_16x64::Vertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x64_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorHorizontal] =
      DefsHbd::_16x64::Horizontal;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize16x64_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorPaeth] =
      DefsHbd::_16x64::Paeth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x8_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorDcFill] =
      Defs12bpp::_32x8::DcFill;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x8_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorDcTop] =
      DefsHbd::_32x8::DcTop;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x8_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorDcLeft] =
      DefsHbd::_32x8::DcLeft;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x8_IntraPredictorDc
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorDc] =
      DefsHbd::_32x8::Dc;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x8_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorVertical] =
      DefsHbd::_32x8::Vertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x8_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorHorizontal] =
      DefsHbd::_32x8::Horizontal;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x8_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorPaeth] =
      DefsHbd::_32x8::Paeth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x16_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorDcFill] =
      Defs12bpp::_32x16::DcFill;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x16_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorDcTop] =
      DefsHbd::_32x16::DcTop;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x16_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorDcLeft] =
      DefsHbd::_32x16::DcLeft;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x16_IntraPredictorDc
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorDc] =
      DefsHbd::_32x16::Dc;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x16_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorVertical] =
      DefsHbd::_32x16::Vertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x16_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorHorizontal] =
      DefsHbd::_32x16::Horizontal;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x16_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorPaeth] =
      DefsHbd::_32x16::Paeth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x32_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorDcFill] =
      Defs12bpp::_32x32::DcFill;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x32_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorDcTop] =
      DefsHbd::_32x32::DcTop;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x32_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorDcLeft] =
      DefsHbd::_32x32::DcLeft;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x32_IntraPredictorDc
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorDc] =
      DefsHbd::_32x32::Dc;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x32_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorVertical] =
      DefsHbd::_32x32::Vertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x32_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorHorizontal] =
      DefsHbd::_32x32::Horizontal;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x32_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorPaeth] =
      DefsHbd::_32x32::Paeth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x64_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorDcFill] =
      Defs12bpp::_32x64::DcFill;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x64_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorDcTop] =
      DefsHbd::_32x64::DcTop;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x64_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorDcLeft] =
      DefsHbd::_32x64::DcLeft;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x64_IntraPredictorDc
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorDc] =
      DefsHbd::_32x64::Dc;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x64_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorVertical] =
      DefsHbd::_32x64::Vertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x64_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorHorizontal] =
      DefsHbd::_32x64::Horizontal;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize32x64_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorPaeth] =
      DefsHbd::_32x64::Paeth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x16_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorDcFill] =
      Defs12bpp::_64x16::DcFill;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x16_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorDcTop] =
      DefsHbd::_64x16::DcTop;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x16_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorDcLeft] =
      DefsHbd::_64x16::DcLeft;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x16_IntraPredictorDc
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorDc] =
      DefsHbd::_64x16::Dc;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x16_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorVertical] =
      DefsHbd::_64x16::Vertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x16_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorHorizontal] =
      DefsHbd::_64x16::Horizontal;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x16_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorPaeth] =
      DefsHbd::_64x16::Paeth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x32_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorDcFill] =
      Defs12bpp::_64x32::DcFill;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x32_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorDcTop] =
      DefsHbd::_64x32::DcTop;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x32_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorDcLeft] =
      DefsHbd::_64x32::DcLeft;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x32_IntraPredictorDc
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorDc] =
      DefsHbd::_64x32::Dc;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x32_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorVertical] =
      DefsHbd::_64x32::Vertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x32_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorHorizontal] =
      DefsHbd::_64x32::Horizontal;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x32_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorPaeth] =
      DefsHbd::_64x32::Paeth;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x64_IntraPredictorDcFill
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorDcFill] =
      Defs12bpp::_64x64::DcFill;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x64_IntraPredictorDcTop
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorDcTop] =
      DefsHbd::_64x64::DcTop;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x64_IntraPredictorDcLeft
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorDcLeft] =
      DefsHbd::_64x64::DcLeft;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x64_IntraPredictorDc
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorDc] =
      DefsHbd::_64x64::Dc;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x64_IntraPredictorVertical
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorVertical] =
      DefsHbd::_64x64::Vertical;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x64_IntraPredictorHorizontal
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorHorizontal] =
      DefsHbd::_64x64::Horizontal;
#endif
#ifndef LIBGAV1_Dsp12bpp_TransformSize64x64_IntraPredictorPaeth
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorPaeth] =
      DefsHbd::_64x64::Paeth;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}  // NOLINT(readability/fn_size)
#endif  // LIBGAV1_MAX_BITDEPTH == 12

#undef INIT_INTRAPREDICTORS_WxH
#undef INIT_INTRAPREDICTORS
}  // namespace

void IntraPredInit_C() {
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
