// Copyright 2020 The libgav1 Authors
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

#include "src/dsp/super_res.h"

#include <cassert>

#include "src/dsp/dsp.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"

namespace libgav1 {
namespace dsp {
namespace {

template <int bitdepth, typename Pixel>
void SuperRes_C(const void* /*coefficients*/,
                void* LIBGAV1_RESTRICT const source,
                const ptrdiff_t source_stride, const int height,
                const int downscaled_width, const int upscaled_width,
                const int initial_subpixel_x, const int step,
                void* LIBGAV1_RESTRICT const dest, ptrdiff_t dest_stride) {
  assert(step <= 1 << kSuperResScaleBits);
  auto* src = static_cast<Pixel*>(source) - DivideBy2(kSuperResFilterTaps);
  auto* dst = static_cast<Pixel*>(dest);
  int y = height;
  do {
    ExtendLine<Pixel>(src + DivideBy2(kSuperResFilterTaps), downscaled_width,
                      kSuperResHorizontalBorder, kSuperResHorizontalBorder);
    // If (original) upscaled_width is <= 9, the downscaled_width may be
    // upscaled_width - 1 (i.e. 8, 9), and become the same (i.e. 4) when
    // subsampled via RightShiftWithRounding. This leads to an edge case where
    // |step| == 1 << 14.
    int subpixel_x = initial_subpixel_x;
    int x = 0;
    do {
      int sum = 0;
      const Pixel* const src_x = &src[subpixel_x >> kSuperResScaleBits];
      const int src_x_subpixel =
          (subpixel_x & kSuperResScaleMask) >> kSuperResExtraBits;
      // The sign of each tap is: - + - + + - + -
      sum -= src_x[0] * kUpscaleFilterUnsigned[src_x_subpixel][0];
      sum += src_x[1] * kUpscaleFilterUnsigned[src_x_subpixel][1];
      sum -= src_x[2] * kUpscaleFilterUnsigned[src_x_subpixel][2];
      sum += src_x[3] * kUpscaleFilterUnsigned[src_x_subpixel][3];
      sum += src_x[4] * kUpscaleFilterUnsigned[src_x_subpixel][4];
      sum -= src_x[5] * kUpscaleFilterUnsigned[src_x_subpixel][5];
      sum += src_x[6] * kUpscaleFilterUnsigned[src_x_subpixel][6];
      sum -= src_x[7] * kUpscaleFilterUnsigned[src_x_subpixel][7];
      dst[x] = Clip3(RightShiftWithRounding(sum, kFilterBits), 0,
                     (1 << bitdepth) - 1);
      subpixel_x += step;
    } while (++x < upscaled_width);
    src += source_stride;
    dst += dest_stride;
  } while (--y != 0);
}

void Init8bpp() {
  Dsp* dsp = dsp_internal::GetWritableDspTable(8);
  assert(dsp != nullptr);
  dsp->super_res_coefficients = nullptr;
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->super_res = SuperRes_C<8, uint8_t>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp8bpp_SuperRes
  dsp->super_res = SuperRes_C<8, uint8_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}

#if LIBGAV1_MAX_BITDEPTH >= 10
void Init10bpp() {
  Dsp* dsp = dsp_internal::GetWritableDspTable(10);
  assert(dsp != nullptr);
  dsp->super_res_coefficients = nullptr;
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->super_res = SuperRes_C<10, uint16_t>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp10bpp_SuperRes
  dsp->super_res = SuperRes_C<10, uint16_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
void Init12bpp() {
  Dsp* dsp = dsp_internal::GetWritableDspTable(12);
  assert(dsp != nullptr);
  dsp->super_res_coefficients = nullptr;
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->super_res = SuperRes_C<12, uint16_t>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp12bpp_SuperRes
  dsp->super_res = SuperRes_C<12, uint16_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace

void SuperResInit_C() {
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
