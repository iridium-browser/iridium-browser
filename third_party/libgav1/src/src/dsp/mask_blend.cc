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

#include "src/dsp/mask_blend.h"

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "src/dsp/dsp.h"
#include "src/utils/common.h"

namespace libgav1 {
namespace dsp {
namespace {

uint8_t GetMaskValue(const uint8_t* LIBGAV1_RESTRICT mask,
                     const uint8_t* LIBGAV1_RESTRICT mask_next_row, int x,
                     int subsampling_x, int subsampling_y) {
  if ((subsampling_x | subsampling_y) == 0) {
    return mask[x];
  }
  if (subsampling_x == 1 && subsampling_y == 0) {
    return static_cast<uint8_t>(RightShiftWithRounding(
        mask[MultiplyBy2(x)] + mask[MultiplyBy2(x) + 1], 1));
  }
  assert(subsampling_x == 1 && subsampling_y == 1);
  return static_cast<uint8_t>(RightShiftWithRounding(
      mask[MultiplyBy2(x)] + mask[MultiplyBy2(x) + 1] +
          mask_next_row[MultiplyBy2(x)] + mask_next_row[MultiplyBy2(x) + 1],
      2));
}

template <int bitdepth, typename Pixel, bool is_inter_intra, int subsampling_x,
          int subsampling_y>
void MaskBlend_C(const void* LIBGAV1_RESTRICT prediction_0,
                 const void* LIBGAV1_RESTRICT prediction_1,
                 const ptrdiff_t prediction_stride_1,
                 const uint8_t* LIBGAV1_RESTRICT mask,
                 const ptrdiff_t mask_stride, const int width, const int height,
                 void* LIBGAV1_RESTRICT dest, const ptrdiff_t dest_stride) {
  static_assert(!(bitdepth == 8 && is_inter_intra), "");
  assert(mask != nullptr);
  using PredType =
      typename std::conditional<bitdepth == 8, int16_t, uint16_t>::type;
  const auto* pred_0 = static_cast<const PredType*>(prediction_0);
  const auto* pred_1 = static_cast<const PredType*>(prediction_1);
  auto* dst = static_cast<Pixel*>(dest);
  const ptrdiff_t dst_stride = dest_stride / sizeof(Pixel);
  constexpr int step_y = subsampling_y ? 2 : 1;
  const uint8_t* mask_next_row = mask + mask_stride;
  // 7.11.3.2 Rounding variables derivation process
  //   2 * FILTER_BITS(7) - (InterRound0(3|5) + InterRound1(7))
  constexpr int inter_post_round_bits = (bitdepth == 12) ? 2 : 4;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const uint8_t mask_value =
          GetMaskValue(mask, mask_next_row, x, subsampling_x, subsampling_y);
      if (is_inter_intra) {
        dst[x] = static_cast<Pixel>(RightShiftWithRounding(
            mask_value * pred_1[x] + (64 - mask_value) * pred_0[x], 6));
      } else {
        assert(prediction_stride_1 == width);
        int res = (mask_value * pred_0[x] + (64 - mask_value) * pred_1[x]) >> 6;
        res -= (bitdepth == 8) ? 0 : kCompoundOffset;
        dst[x] = static_cast<Pixel>(
            Clip3(RightShiftWithRounding(res, inter_post_round_bits), 0,
                  (1 << bitdepth) - 1));
      }
    }
    dst += dst_stride;
    mask += mask_stride * step_y;
    mask_next_row += mask_stride * step_y;
    pred_0 += width;
    pred_1 += prediction_stride_1;
  }
}

template <int subsampling_x, int subsampling_y>
void InterIntraMaskBlend8bpp_C(const uint8_t* LIBGAV1_RESTRICT prediction_0,
                               uint8_t* LIBGAV1_RESTRICT prediction_1,
                               const ptrdiff_t prediction_stride_1,
                               const uint8_t* LIBGAV1_RESTRICT mask,
                               const ptrdiff_t mask_stride, const int width,
                               const int height) {
  assert(mask != nullptr);
  constexpr int step_y = subsampling_y ? 2 : 1;
  const uint8_t* mask_next_row = mask + mask_stride;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const uint8_t mask_value =
          GetMaskValue(mask, mask_next_row, x, subsampling_x, subsampling_y);
      prediction_1[x] = static_cast<uint8_t>(RightShiftWithRounding(
          mask_value * prediction_1[x] + (64 - mask_value) * prediction_0[x],
          6));
    }
    mask += mask_stride * step_y;
    mask_next_row += mask_stride * step_y;
    prediction_0 += width;
    prediction_1 += prediction_stride_1;
  }
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(8);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->mask_blend[0][0] = MaskBlend_C<8, uint8_t, false, 0, 0>;
  dsp->mask_blend[1][0] = MaskBlend_C<8, uint8_t, false, 1, 0>;
  dsp->mask_blend[2][0] = MaskBlend_C<8, uint8_t, false, 1, 1>;
  // The is_inter_intra index of mask_blend[][] is replaced by
  // inter_intra_mask_blend_8bpp[] in 8-bit.
  dsp->mask_blend[0][1] = nullptr;
  dsp->mask_blend[1][1] = nullptr;
  dsp->mask_blend[2][1] = nullptr;
  dsp->inter_intra_mask_blend_8bpp[0] = InterIntraMaskBlend8bpp_C<0, 0>;
  dsp->inter_intra_mask_blend_8bpp[1] = InterIntraMaskBlend8bpp_C<1, 0>;
  dsp->inter_intra_mask_blend_8bpp[2] = InterIntraMaskBlend8bpp_C<1, 1>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp8bpp_MaskBlend444
  dsp->mask_blend[0][0] = MaskBlend_C<8, uint8_t, false, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_MaskBlend422
  dsp->mask_blend[1][0] = MaskBlend_C<8, uint8_t, false, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_MaskBlend420
  dsp->mask_blend[2][0] = MaskBlend_C<8, uint8_t, false, 1, 1>;
#endif
  // The is_inter_intra index of mask_blend[][] is replaced by
  // inter_intra_mask_blend_8bpp[] in 8-bit.
  dsp->mask_blend[0][1] = nullptr;
  dsp->mask_blend[1][1] = nullptr;
  dsp->mask_blend[2][1] = nullptr;
#ifndef LIBGAV1_Dsp8bpp_InterIntraMaskBlend8bpp444
  dsp->inter_intra_mask_blend_8bpp[0] = InterIntraMaskBlend8bpp_C<0, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_InterIntraMaskBlend8bpp422
  dsp->inter_intra_mask_blend_8bpp[1] = InterIntraMaskBlend8bpp_C<1, 0>;
#endif
#ifndef LIBGAV1_Dsp8bpp_InterIntraMaskBlend8bpp420
  dsp->inter_intra_mask_blend_8bpp[2] = InterIntraMaskBlend8bpp_C<1, 1>;
#endif
  static_cast<void>(GetMaskValue);
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}

#if LIBGAV1_MAX_BITDEPTH >= 10
void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(10);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->mask_blend[0][0] = MaskBlend_C<10, uint16_t, false, 0, 0>;
  dsp->mask_blend[1][0] = MaskBlend_C<10, uint16_t, false, 1, 0>;
  dsp->mask_blend[2][0] = MaskBlend_C<10, uint16_t, false, 1, 1>;
  dsp->mask_blend[0][1] = MaskBlend_C<10, uint16_t, true, 0, 0>;
  dsp->mask_blend[1][1] = MaskBlend_C<10, uint16_t, true, 1, 0>;
  dsp->mask_blend[2][1] = MaskBlend_C<10, uint16_t, true, 1, 1>;
  // These are only used with 8-bit.
  dsp->inter_intra_mask_blend_8bpp[0] = nullptr;
  dsp->inter_intra_mask_blend_8bpp[1] = nullptr;
  dsp->inter_intra_mask_blend_8bpp[2] = nullptr;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp10bpp_MaskBlend444
  dsp->mask_blend[0][0] = MaskBlend_C<10, uint16_t, false, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_MaskBlend422
  dsp->mask_blend[1][0] = MaskBlend_C<10, uint16_t, false, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_MaskBlend420
  dsp->mask_blend[2][0] = MaskBlend_C<10, uint16_t, false, 1, 1>;
#endif
#ifndef LIBGAV1_Dsp10bpp_MaskBlendInterIntra444
  dsp->mask_blend[0][1] = MaskBlend_C<10, uint16_t, true, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_MaskBlendInterIntra422
  dsp->mask_blend[1][1] = MaskBlend_C<10, uint16_t, true, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp10bpp_MaskBlendInterIntra420
  dsp->mask_blend[2][1] = MaskBlend_C<10, uint16_t, true, 1, 1>;
#endif
  // These are only used with 8-bit.
  dsp->inter_intra_mask_blend_8bpp[0] = nullptr;
  dsp->inter_intra_mask_blend_8bpp[1] = nullptr;
  dsp->inter_intra_mask_blend_8bpp[2] = nullptr;
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
void Init12bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(12);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->mask_blend[0][0] = MaskBlend_C<12, uint16_t, false, 0, 0>;
  dsp->mask_blend[1][0] = MaskBlend_C<12, uint16_t, false, 1, 0>;
  dsp->mask_blend[2][0] = MaskBlend_C<12, uint16_t, false, 1, 1>;
  dsp->mask_blend[0][1] = MaskBlend_C<12, uint16_t, true, 0, 0>;
  dsp->mask_blend[1][1] = MaskBlend_C<12, uint16_t, true, 1, 0>;
  dsp->mask_blend[2][1] = MaskBlend_C<12, uint16_t, true, 1, 1>;
  // These are only used with 8-bit.
  dsp->inter_intra_mask_blend_8bpp[0] = nullptr;
  dsp->inter_intra_mask_blend_8bpp[1] = nullptr;
  dsp->inter_intra_mask_blend_8bpp[2] = nullptr;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp12bpp_MaskBlend444
  dsp->mask_blend[0][0] = MaskBlend_C<12, uint16_t, false, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_MaskBlend422
  dsp->mask_blend[1][0] = MaskBlend_C<12, uint16_t, false, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_MaskBlend420
  dsp->mask_blend[2][0] = MaskBlend_C<12, uint16_t, false, 1, 1>;
#endif
#ifndef LIBGAV1_Dsp12bpp_MaskBlendInterIntra444
  dsp->mask_blend[0][1] = MaskBlend_C<12, uint16_t, true, 0, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_MaskBlendInterIntra422
  dsp->mask_blend[1][1] = MaskBlend_C<12, uint16_t, true, 1, 0>;
#endif
#ifndef LIBGAV1_Dsp12bpp_MaskBlendInterIntra420
  dsp->mask_blend[2][1] = MaskBlend_C<12, uint16_t, true, 1, 1>;
#endif
  // These are only used with 8-bit.
  dsp->inter_intra_mask_blend_8bpp[0] = nullptr;
  dsp->inter_intra_mask_blend_8bpp[1] = nullptr;
  dsp->inter_intra_mask_blend_8bpp[2] = nullptr;
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace

void MaskBlendInit_C() {
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
