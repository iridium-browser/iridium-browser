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

#include "src/dsp/intrapred_directional.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
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
// 7.11.2.4. Directional intra prediction process

template <typename Pixel>
void DirectionalIntraPredictorZone1_C(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row, const int width,
    const int height, const int xstep, const bool upsampled_top) {
  const auto* const top = static_cast<const Pixel*>(top_row);
  auto* dst = static_cast<Pixel*>(dest);
  stride /= sizeof(Pixel);

  assert(xstep > 0);

  // If xstep == 64 then |shift| always evaluates to 0 which sets |val| to
  // |top[top_base_x]|. This corresponds to a 45 degree prediction.
  if (xstep == 64) {
    // 7.11.2.10. Intra edge upsample selection process
    // if ( d <= 0 || d >= 40 ) useUpsample = 0
    // For |upsampled_top| the delta is |predictor_angle - 90|. Since the
    // |predictor_angle| is 45 the delta is also 45.
    assert(!upsampled_top);
    const Pixel* top_ptr = top + 1;
    for (int y = 0; y < height; ++y, dst += stride, ++top_ptr) {
      memcpy(dst, top_ptr, sizeof(*top_ptr) * width);
    }
    return;
  }

  const int upsample_shift = static_cast<int>(upsampled_top);
  const int max_base_x = ((width + height) - 1) << upsample_shift;
  const int scale_bits = 6 - upsample_shift;
  const int base_step = 1 << upsample_shift;
  int top_x = xstep;
  int y = 0;
  do {
    int top_base_x = top_x >> scale_bits;

    if (top_base_x >= max_base_x) {
      for (int i = y; i < height; ++i) {
        Memset(dst, top[max_base_x], width);
        dst += stride;
      }
      return;
    }

    const int shift = ((top_x << upsample_shift) & 0x3F) >> 1;
    int x = 0;
    do {
      if (top_base_x >= max_base_x) {
        Memset(dst + x, top[max_base_x], width - x);
        break;
      }

      const int val =
          top[top_base_x] * (32 - shift) + top[top_base_x + 1] * shift;
      dst[x] = RightShiftWithRounding(val, 5 /*log2(32)*/);
      top_base_x += base_step;
    } while (++x < width);

    dst += stride;
    top_x += xstep;
  } while (++y < height);
}

// clang 14.0.0 produces incorrect code with LIBGAV1_RESTRICT.
// https://github.com/llvm/llvm-project/issues/54427
#if defined(__clang__) && __clang_major__ == 14
#define LOCAL_RESTRICT
#else
#define LOCAL_RESTRICT LIBGAV1_RESTRICT
#endif

template <typename Pixel>
void DirectionalIntraPredictorZone2_C(
    void* LOCAL_RESTRICT const dest, ptrdiff_t stride,
    const void* LOCAL_RESTRICT const top_row,
    const void* LOCAL_RESTRICT const left_column, const int width,
    const int height, const int xstep, const int ystep,
    const bool upsampled_top, const bool upsampled_left) {
  const auto* const top = static_cast<const Pixel*>(top_row);
  const auto* const left = static_cast<const Pixel*>(left_column);
  auto* dst = static_cast<Pixel*>(dest);
  stride /= sizeof(Pixel);

  assert(xstep > 0);
  assert(ystep > 0);

  const int upsample_top_shift = static_cast<int>(upsampled_top);
  const int upsample_left_shift = static_cast<int>(upsampled_left);
  const int scale_bits_x = 6 - upsample_top_shift;
  const int scale_bits_y = 6 - upsample_left_shift;
  const int min_base_x = -(1 << upsample_top_shift);
  const int base_step_x = 1 << upsample_top_shift;
  int y = 0;
  int top_x = -xstep;
  do {
    int top_base_x = top_x >> scale_bits_x;
    int left_y = (y << 6) - ystep;
    int x = 0;
    do {
      int val;
      if (top_base_x >= min_base_x) {
        const int shift = ((top_x * (1 << upsample_top_shift)) & 0x3F) >> 1;
        val = top[top_base_x] * (32 - shift) + top[top_base_x + 1] * shift;
      } else {
        // Note this assumes an arithmetic shift to handle negative values.
        const int left_base_y = left_y >> scale_bits_y;
        const int shift = ((left_y * (1 << upsample_left_shift)) & 0x3F) >> 1;
        assert(left_base_y >= -(1 << upsample_left_shift));
        val = left[left_base_y] * (32 - shift) + left[left_base_y + 1] * shift;
      }
      dst[x] = RightShiftWithRounding(val, 5);
      top_base_x += base_step_x;
      left_y -= ystep;
    } while (++x < width);

    top_x -= xstep;
    dst += stride;
  } while (++y < height);
}

#undef LOCAL_RESTRICT

template <typename Pixel>
void DirectionalIntraPredictorZone3_C(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const left_column, const int width,
    const int height, const int ystep, const bool upsampled_left) {
  const auto* const left = static_cast<const Pixel*>(left_column);
  stride /= sizeof(Pixel);

  assert(ystep > 0);

  const int upsample_shift = static_cast<int>(upsampled_left);
  const int scale_bits = 6 - upsample_shift;
  const int base_step = 1 << upsample_shift;
  // Zone3 never runs out of left_column values.
  assert((width + height - 1) << upsample_shift >  // max_base_y
         ((ystep * width) >> scale_bits) +
             base_step * (height - 1));  // left_base_y

  int left_y = ystep;
  int x = 0;
  do {
    auto* dst = static_cast<Pixel*>(dest);

    int left_base_y = left_y >> scale_bits;
    int y = 0;
    do {
      const int shift = ((left_y << upsample_shift) & 0x3F) >> 1;
      const int val =
          left[left_base_y] * (32 - shift) + left[left_base_y + 1] * shift;
      dst[x] = RightShiftWithRounding(val, 5);
      dst += stride;
      left_base_y += base_step;
    } while (++y < height);

    left_y += ystep;
  } while (++x < width);
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(8);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->directional_intra_predictor_zone1 =
      DirectionalIntraPredictorZone1_C<uint8_t>;
  dsp->directional_intra_predictor_zone2 =
      DirectionalIntraPredictorZone2_C<uint8_t>;
  dsp->directional_intra_predictor_zone3 =
      DirectionalIntraPredictorZone3_C<uint8_t>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp8bpp_DirectionalIntraPredictorZone1
  dsp->directional_intra_predictor_zone1 =
      DirectionalIntraPredictorZone1_C<uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_DirectionalIntraPredictorZone2
  dsp->directional_intra_predictor_zone2 =
      DirectionalIntraPredictorZone2_C<uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_DirectionalIntraPredictorZone3
  dsp->directional_intra_predictor_zone3 =
      DirectionalIntraPredictorZone3_C<uint8_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}

#if LIBGAV1_MAX_BITDEPTH >= 10
void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(10);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->directional_intra_predictor_zone1 =
      DirectionalIntraPredictorZone1_C<uint16_t>;
  dsp->directional_intra_predictor_zone2 =
      DirectionalIntraPredictorZone2_C<uint16_t>;
  dsp->directional_intra_predictor_zone3 =
      DirectionalIntraPredictorZone3_C<uint16_t>;
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp10bpp_DirectionalIntraPredictorZone1
  dsp->directional_intra_predictor_zone1 =
      DirectionalIntraPredictorZone1_C<uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_DirectionalIntraPredictorZone2
  dsp->directional_intra_predictor_zone2 =
      DirectionalIntraPredictorZone2_C<uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_DirectionalIntraPredictorZone3
  dsp->directional_intra_predictor_zone3 =
      DirectionalIntraPredictorZone3_C<uint16_t>;
#endif
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
void Init12bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(12);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->directional_intra_predictor_zone1 =
      DirectionalIntraPredictorZone1_C<uint16_t>;
  dsp->directional_intra_predictor_zone2 =
      DirectionalIntraPredictorZone2_C<uint16_t>;
  dsp->directional_intra_predictor_zone3 =
      DirectionalIntraPredictorZone3_C<uint16_t>;
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp12bpp_DirectionalIntraPredictorZone1
  dsp->directional_intra_predictor_zone1 =
      DirectionalIntraPredictorZone1_C<uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_DirectionalIntraPredictorZone2
  dsp->directional_intra_predictor_zone2 =
      DirectionalIntraPredictorZone2_C<uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_DirectionalIntraPredictorZone3
  dsp->directional_intra_predictor_zone3 =
      DirectionalIntraPredictorZone3_C<uint16_t>;
#endif
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace

void IntraPredDirectionalInit_C() {
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
