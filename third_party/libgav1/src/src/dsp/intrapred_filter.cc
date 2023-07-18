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

#include "src/dsp/intrapred_filter.h"

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

//------------------------------------------------------------------------------
// FilterIntraPredictor_C

// The recursive filter applies a different filter to the top 4 and 2 left
// pixels to produce each pixel in a 4x2 sub-block. Each successive 4x2 uses the
// prediction output of the blocks above and to the left, unless they are
// adjacent to the |top_row| or |left_column|. The set of 8 filters is selected
// according to |pred|.
template <int bitdepth, typename Pixel>
void FilterIntraPredictor_C(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                            const void* LIBGAV1_RESTRICT const top_row,
                            const void* LIBGAV1_RESTRICT const left_column,
                            const FilterIntraPredictor pred, const int width,
                            const int height) {
  const int kMaxPixel = (1 << bitdepth) - 1;
  const auto* const top = static_cast<const Pixel*>(top_row);
  const auto* const left = static_cast<const Pixel*>(left_column);

  assert(width <= 32 && height <= 32);

  Pixel buffer[3][33];  // cache 2 rows + top & left boundaries
  memcpy(buffer[0], &top[-1], (width + 1) * sizeof(top[0]));

  auto* dst = static_cast<Pixel*>(dest);
  stride /= sizeof(Pixel);
  int row0 = 0, row2 = 2;
  int ystep = 1;
  int y = 0;
  do {
    buffer[1][0] = left[y];
    buffer[row2][0] = left[y + 1];
    int x = 1;
    do {
      const Pixel p0 = buffer[row0][x - 1];  // top-left
      const Pixel p1 = buffer[row0][x + 0];  // top 0
      const Pixel p2 = buffer[row0][x + 1];  // top 1
      const Pixel p3 = buffer[row0][x + 2];  // top 2
      const Pixel p4 = buffer[row0][x + 3];  // top 3
      const Pixel p5 = buffer[1][x - 1];     // left 0
      const Pixel p6 = buffer[row2][x - 1];  // left 1
      for (int i = 0; i < 8; ++i) {
        const int xoffset = i & 0x03;
        const int yoffset = (i >> 2) * ystep;
        const int value = kFilterIntraTaps[pred][i][0] * p0 +
                          kFilterIntraTaps[pred][i][1] * p1 +
                          kFilterIntraTaps[pred][i][2] * p2 +
                          kFilterIntraTaps[pred][i][3] * p3 +
                          kFilterIntraTaps[pred][i][4] * p4 +
                          kFilterIntraTaps[pred][i][5] * p5 +
                          kFilterIntraTaps[pred][i][6] * p6;
        // Section 7.11.2.3 specifies the right-hand side of the assignment as
        //   Clip1( Round2Signed( pr, INTRA_FILTER_SCALE_BITS ) ).
        // Since Clip1() clips a negative value to 0, it is safe to replace
        // Round2Signed() with Round2().
        buffer[1 + yoffset][x + xoffset] = static_cast<Pixel>(
            Clip3(RightShiftWithRounding(value, 4), 0, kMaxPixel));
      }
      x += 4;
    } while (x < width);
    memcpy(dst, &buffer[1][1], width * sizeof(dst[0]));
    dst += stride;
    memcpy(dst, &buffer[row2][1], width * sizeof(dst[0]));
    dst += stride;

    // The final row becomes the top for the next pass.
    row0 ^= 2;
    row2 ^= 2;
    ystep = -ystep;
    y += 2;
  } while (y < height);
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(8);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->filter_intra_predictor = FilterIntraPredictor_C<8, uint8_t>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp8bpp_FilterIntraPredictor
  dsp->filter_intra_predictor = FilterIntraPredictor_C<8, uint8_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}

#if LIBGAV1_MAX_BITDEPTH >= 10
void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(10);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->filter_intra_predictor = FilterIntraPredictor_C<10, uint16_t>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp10bpp_FilterIntraPredictor
  dsp->filter_intra_predictor = FilterIntraPredictor_C<10, uint16_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
void Init12bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(12);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->filter_intra_predictor = FilterIntraPredictor_C<12, uint16_t>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp12bpp_FilterIntraPredictor
  dsp->filter_intra_predictor = FilterIntraPredictor_C<12, uint16_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace

void IntraPredFilterInit_C() {
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
