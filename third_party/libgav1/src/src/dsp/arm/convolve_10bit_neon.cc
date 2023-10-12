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

#include "src/dsp/convolve.h"
#include "src/utils/cpu.h"

#if LIBGAV1_ENABLE_NEON && LIBGAV1_MAX_BITDEPTH >= 10
#include <arm_neon.h>

#include <algorithm>
#include <cassert>
#include <cstdint>

#include "src/dsp/arm/common_neon.h"
#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/utils/common.h"
#include "src/utils/compiler_attributes.h"
#include "src/utils/constants.h"

namespace libgav1 {
namespace dsp {
namespace {

// Include the constants and utility functions inside the anonymous namespace.
#include "src/dsp/convolve.inc"

// Output of ConvolveTest.ShowRange below.
// Bitdepth: 10 Input range:            [       0,     1023]
//   Horizontal base upscaled range:    [  -28644,    94116]
//   Horizontal halved upscaled range:  [  -14322,    47085]
//   Horizontal downscaled range:       [   -7161,    23529]
//   Vertical upscaled range:           [-1317624,  2365176]
//   Pixel output range:                [       0,     1023]
//   Compound output range:             [    3988,    61532]

template <int num_taps>
int32x4x2_t SumOnePassTaps(const uint16x8_t* const src,
                           const int16x4_t* const taps) {
  const auto* ssrc = reinterpret_cast<const int16x8_t*>(src);
  int32x4x2_t sum;
  if (num_taps == 6) {
    // 6 taps.
    sum.val[0] = vmull_s16(vget_low_s16(ssrc[0]), taps[0]);
    sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(ssrc[1]), taps[1]);
    sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(ssrc[2]), taps[2]);
    sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(ssrc[3]), taps[3]);
    sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(ssrc[4]), taps[4]);
    sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(ssrc[5]), taps[5]);

    sum.val[1] = vmull_s16(vget_high_s16(ssrc[0]), taps[0]);
    sum.val[1] = vmlal_s16(sum.val[1], vget_high_s16(ssrc[1]), taps[1]);
    sum.val[1] = vmlal_s16(sum.val[1], vget_high_s16(ssrc[2]), taps[2]);
    sum.val[1] = vmlal_s16(sum.val[1], vget_high_s16(ssrc[3]), taps[3]);
    sum.val[1] = vmlal_s16(sum.val[1], vget_high_s16(ssrc[4]), taps[4]);
    sum.val[1] = vmlal_s16(sum.val[1], vget_high_s16(ssrc[5]), taps[5]);
  } else if (num_taps == 8) {
    // 8 taps.
    sum.val[0] = vmull_s16(vget_low_s16(ssrc[0]), taps[0]);
    sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(ssrc[1]), taps[1]);
    sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(ssrc[2]), taps[2]);
    sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(ssrc[3]), taps[3]);
    sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(ssrc[4]), taps[4]);
    sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(ssrc[5]), taps[5]);
    sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(ssrc[6]), taps[6]);
    sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(ssrc[7]), taps[7]);

    sum.val[1] = vmull_s16(vget_high_s16(ssrc[0]), taps[0]);
    sum.val[1] = vmlal_s16(sum.val[1], vget_high_s16(ssrc[1]), taps[1]);
    sum.val[1] = vmlal_s16(sum.val[1], vget_high_s16(ssrc[2]), taps[2]);
    sum.val[1] = vmlal_s16(sum.val[1], vget_high_s16(ssrc[3]), taps[3]);
    sum.val[1] = vmlal_s16(sum.val[1], vget_high_s16(ssrc[4]), taps[4]);
    sum.val[1] = vmlal_s16(sum.val[1], vget_high_s16(ssrc[5]), taps[5]);
    sum.val[1] = vmlal_s16(sum.val[1], vget_high_s16(ssrc[6]), taps[6]);
    sum.val[1] = vmlal_s16(sum.val[1], vget_high_s16(ssrc[7]), taps[7]);
  } else if (num_taps == 2) {
    // 2 taps.
    sum.val[0] = vmull_s16(vget_low_s16(ssrc[0]), taps[0]);
    sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(ssrc[1]), taps[1]);

    sum.val[1] = vmull_s16(vget_high_s16(ssrc[0]), taps[0]);
    sum.val[1] = vmlal_s16(sum.val[1], vget_high_s16(ssrc[1]), taps[1]);
  } else {
    // 4 taps.
    sum.val[0] = vmull_s16(vget_low_s16(ssrc[0]), taps[0]);
    sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(ssrc[1]), taps[1]);
    sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(ssrc[2]), taps[2]);
    sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(ssrc[3]), taps[3]);

    sum.val[1] = vmull_s16(vget_high_s16(ssrc[0]), taps[0]);
    sum.val[1] = vmlal_s16(sum.val[1], vget_high_s16(ssrc[1]), taps[1]);
    sum.val[1] = vmlal_s16(sum.val[1], vget_high_s16(ssrc[2]), taps[2]);
    sum.val[1] = vmlal_s16(sum.val[1], vget_high_s16(ssrc[3]), taps[3]);
  }
  return sum;
}

template <int num_taps>
int32x4_t SumOnePassTaps(const uint16x4_t* const src,
                         const int16x4_t* const taps) {
  const auto* ssrc = reinterpret_cast<const int16x4_t*>(src);
  int32x4_t sum;
  if (num_taps == 6) {
    // 6 taps.
    sum = vmull_s16(ssrc[0], taps[0]);
    sum = vmlal_s16(sum, ssrc[1], taps[1]);
    sum = vmlal_s16(sum, ssrc[2], taps[2]);
    sum = vmlal_s16(sum, ssrc[3], taps[3]);
    sum = vmlal_s16(sum, ssrc[4], taps[4]);
    sum = vmlal_s16(sum, ssrc[5], taps[5]);
  } else if (num_taps == 8) {
    // 8 taps.
    sum = vmull_s16(ssrc[0], taps[0]);
    sum = vmlal_s16(sum, ssrc[1], taps[1]);
    sum = vmlal_s16(sum, ssrc[2], taps[2]);
    sum = vmlal_s16(sum, ssrc[3], taps[3]);
    sum = vmlal_s16(sum, ssrc[4], taps[4]);
    sum = vmlal_s16(sum, ssrc[5], taps[5]);
    sum = vmlal_s16(sum, ssrc[6], taps[6]);
    sum = vmlal_s16(sum, ssrc[7], taps[7]);
  } else if (num_taps == 2) {
    // 2 taps.
    sum = vmull_s16(ssrc[0], taps[0]);
    sum = vmlal_s16(sum, ssrc[1], taps[1]);
  } else {
    // 4 taps.
    sum = vmull_s16(ssrc[0], taps[0]);
    sum = vmlal_s16(sum, ssrc[1], taps[1]);
    sum = vmlal_s16(sum, ssrc[2], taps[2]);
    sum = vmlal_s16(sum, ssrc[3], taps[3]);
  }
  return sum;
}

template <int num_taps, bool is_compound, bool is_2d>
void FilterHorizontalWidth8AndUp(const uint16_t* LIBGAV1_RESTRICT src,
                                 const ptrdiff_t src_stride,
                                 void* LIBGAV1_RESTRICT const dest,
                                 const ptrdiff_t pred_stride, const int width,
                                 const int height,
                                 const int16x4_t* const v_tap) {
  auto* dest16 = static_cast<uint16_t*>(dest);
  const uint16x4_t v_max_bitdepth = vdup_n_u16((1 << kBitdepth10) - 1);
  if (is_2d) {
    int x = 0;
    do {
      const uint16_t* s = src + x;
      int y = height;
      do {  // Increasing loop counter x is better.
        const uint16x8_t src_long = vld1q_u16(s);
        const uint16x8_t src_long_hi = vld1q_u16(s + 8);
        uint16x8_t v_src[8];
        int32x4x2_t v_sum;
        if (num_taps == 6) {
          v_src[0] = src_long;
          v_src[1] = vextq_u16(src_long, src_long_hi, 1);
          v_src[2] = vextq_u16(src_long, src_long_hi, 2);
          v_src[3] = vextq_u16(src_long, src_long_hi, 3);
          v_src[4] = vextq_u16(src_long, src_long_hi, 4);
          v_src[5] = vextq_u16(src_long, src_long_hi, 5);
          v_sum = SumOnePassTaps<num_taps>(v_src, v_tap + 1);
        } else if (num_taps == 8) {
          v_src[0] = src_long;
          v_src[1] = vextq_u16(src_long, src_long_hi, 1);
          v_src[2] = vextq_u16(src_long, src_long_hi, 2);
          v_src[3] = vextq_u16(src_long, src_long_hi, 3);
          v_src[4] = vextq_u16(src_long, src_long_hi, 4);
          v_src[5] = vextq_u16(src_long, src_long_hi, 5);
          v_src[6] = vextq_u16(src_long, src_long_hi, 6);
          v_src[7] = vextq_u16(src_long, src_long_hi, 7);
          v_sum = SumOnePassTaps<num_taps>(v_src, v_tap);
        } else if (num_taps == 2) {
          v_src[0] = src_long;
          v_src[1] = vextq_u16(src_long, src_long_hi, 1);
          v_sum = SumOnePassTaps<num_taps>(v_src, v_tap + 3);
        } else {  // 4 taps
          v_src[0] = src_long;
          v_src[1] = vextq_u16(src_long, src_long_hi, 1);
          v_src[2] = vextq_u16(src_long, src_long_hi, 2);
          v_src[3] = vextq_u16(src_long, src_long_hi, 3);
          v_sum = SumOnePassTaps<num_taps>(v_src, v_tap + 2);
        }

        const int16x4_t d0 =
            vqrshrn_n_s32(v_sum.val[0], kInterRoundBitsHorizontal - 1);
        const int16x4_t d1 =
            vqrshrn_n_s32(v_sum.val[1], kInterRoundBitsHorizontal - 1);
        vst1_u16(&dest16[0], vreinterpret_u16_s16(d0));
        vst1_u16(&dest16[4], vreinterpret_u16_s16(d1));
        s += src_stride;
        dest16 += 8;
      } while (--y != 0);
      x += 8;
    } while (x < width);
    return;
  }
  int y = height;
  do {
    int x = 0;
    do {
      const uint16x8_t src_long = vld1q_u16(src + x);
      const uint16x8_t src_long_hi = vld1q_u16(src + x + 8);
      uint16x8_t v_src[8];
      int32x4x2_t v_sum;
      if (num_taps == 6) {
        v_src[0] = src_long;
        v_src[1] = vextq_u16(src_long, src_long_hi, 1);
        v_src[2] = vextq_u16(src_long, src_long_hi, 2);
        v_src[3] = vextq_u16(src_long, src_long_hi, 3);
        v_src[4] = vextq_u16(src_long, src_long_hi, 4);
        v_src[5] = vextq_u16(src_long, src_long_hi, 5);
        v_sum = SumOnePassTaps<num_taps>(v_src, v_tap + 1);
      } else if (num_taps == 8) {
        v_src[0] = src_long;
        v_src[1] = vextq_u16(src_long, src_long_hi, 1);
        v_src[2] = vextq_u16(src_long, src_long_hi, 2);
        v_src[3] = vextq_u16(src_long, src_long_hi, 3);
        v_src[4] = vextq_u16(src_long, src_long_hi, 4);
        v_src[5] = vextq_u16(src_long, src_long_hi, 5);
        v_src[6] = vextq_u16(src_long, src_long_hi, 6);
        v_src[7] = vextq_u16(src_long, src_long_hi, 7);
        v_sum = SumOnePassTaps<num_taps>(v_src, v_tap);
      } else if (num_taps == 2) {
        v_src[0] = src_long;
        v_src[1] = vextq_u16(src_long, src_long_hi, 1);
        v_sum = SumOnePassTaps<num_taps>(v_src, v_tap + 3);
      } else {  // 4 taps
        v_src[0] = src_long;
        v_src[1] = vextq_u16(src_long, src_long_hi, 1);
        v_src[2] = vextq_u16(src_long, src_long_hi, 2);
        v_src[3] = vextq_u16(src_long, src_long_hi, 3);
        v_sum = SumOnePassTaps<num_taps>(v_src, v_tap + 2);
      }
      if (is_compound) {
        const int16x4_t v_compound_offset = vdup_n_s16(kCompoundOffset);
        const int16x4_t d0 =
            vqrshrn_n_s32(v_sum.val[0], kInterRoundBitsHorizontal - 1);
        const int16x4_t d1 =
            vqrshrn_n_s32(v_sum.val[1], kInterRoundBitsHorizontal - 1);
        vst1_u16(&dest16[x],
                 vreinterpret_u16_s16(vadd_s16(d0, v_compound_offset)));
        vst1_u16(&dest16[x + 4],
                 vreinterpret_u16_s16(vadd_s16(d1, v_compound_offset)));
      } else {
        // Normally the Horizontal pass does the downshift in two passes:
        // kInterRoundBitsHorizontal - 1 and then (kFilterBits -
        // kInterRoundBitsHorizontal). Each one uses a rounding shift.
        // Combining them requires adding the rounding offset from the skipped
        // shift.
        const int32x4_t v_first_shift_rounding_bit =
            vdupq_n_s32(1 << (kInterRoundBitsHorizontal - 2));
        v_sum.val[0] = vaddq_s32(v_sum.val[0], v_first_shift_rounding_bit);
        v_sum.val[1] = vaddq_s32(v_sum.val[1], v_first_shift_rounding_bit);
        const uint16x4_t d0 = vmin_u16(
            vqrshrun_n_s32(v_sum.val[0], kFilterBits - 1), v_max_bitdepth);
        const uint16x4_t d1 = vmin_u16(
            vqrshrun_n_s32(v_sum.val[1], kFilterBits - 1), v_max_bitdepth);
        vst1_u16(&dest16[x], d0);
        vst1_u16(&dest16[x + 4], d1);
      }
      x += 8;
    } while (x < width);
    src += src_stride;
    dest16 += pred_stride;
  } while (--y != 0);
}

template <int num_taps, bool is_compound, bool is_2d>
void FilterHorizontalWidth4(const uint16_t* LIBGAV1_RESTRICT src,
                            const ptrdiff_t src_stride,
                            void* LIBGAV1_RESTRICT const dest,
                            const ptrdiff_t pred_stride, const int height,
                            const int16x4_t* const v_tap) {
  auto* dest16 = static_cast<uint16_t*>(dest);
  const uint16x4_t v_max_bitdepth = vdup_n_u16((1 << kBitdepth10) - 1);
  int y = height;
  do {
    const uint16x8_t v_zero = vdupq_n_u16(0);
    uint16x4_t v_src[4];
    int32x4_t v_sum;
    const uint16x8_t src_long = vld1q_u16(src);
    v_src[0] = vget_low_u16(src_long);
    if (num_taps == 2) {
      v_src[1] = vget_low_u16(vextq_u16(src_long, v_zero, 1));
      v_sum = SumOnePassTaps<num_taps>(v_src, v_tap + 3);
    } else {
      v_src[1] = vget_low_u16(vextq_u16(src_long, v_zero, 1));
      v_src[2] = vget_low_u16(vextq_u16(src_long, v_zero, 2));
      v_src[3] = vget_low_u16(vextq_u16(src_long, v_zero, 3));
      v_sum = SumOnePassTaps<num_taps>(v_src, v_tap + 2);
    }
    if (is_compound || is_2d) {
      const int16x4_t d0 = vqrshrn_n_s32(v_sum, kInterRoundBitsHorizontal - 1);
      if (is_compound && !is_2d) {
        vst1_u16(&dest16[0], vreinterpret_u16_s16(
                                 vadd_s16(d0, vdup_n_s16(kCompoundOffset))));
      } else {
        vst1_u16(&dest16[0], vreinterpret_u16_s16(d0));
      }
    } else {
      const int32x4_t v_first_shift_rounding_bit =
          vdupq_n_s32(1 << (kInterRoundBitsHorizontal - 2));
      v_sum = vaddq_s32(v_sum, v_first_shift_rounding_bit);
      const uint16x4_t d0 =
          vmin_u16(vqrshrun_n_s32(v_sum, kFilterBits - 1), v_max_bitdepth);
      vst1_u16(&dest16[0], d0);
    }
    src += src_stride;
    dest16 += pred_stride;
  } while (--y != 0);
}

template <int num_taps, bool is_2d>
void FilterHorizontalWidth2(const uint16_t* LIBGAV1_RESTRICT src,
                            const ptrdiff_t src_stride,
                            void* LIBGAV1_RESTRICT const dest,
                            const ptrdiff_t pred_stride, const int height,
                            const int16x4_t* const v_tap) {
  auto* dest16 = static_cast<uint16_t*>(dest);
  const uint16x4_t v_max_bitdepth = vdup_n_u16((1 << kBitdepth10) - 1);
  int y = height >> 1;
  do {
    const int16x8_t v_zero = vdupq_n_s16(0);
    const int16x8_t input0 = vreinterpretq_s16_u16(vld1q_u16(src));
    const int16x8_t input1 = vreinterpretq_s16_u16(vld1q_u16(src + src_stride));
    const int16x8x2_t input = vzipq_s16(input0, input1);
    int32x4_t v_sum;
    if (num_taps == 2) {
      v_sum = vmull_s16(vget_low_s16(input.val[0]), v_tap[3]);
      v_sum = vmlal_s16(v_sum,
                        vget_low_s16(vextq_s16(input.val[0], input.val[1], 2)),
                        v_tap[4]);
    } else {
      v_sum = vmull_s16(vget_low_s16(input.val[0]), v_tap[2]);
      v_sum = vmlal_s16(v_sum, vget_low_s16(vextq_s16(input.val[0], v_zero, 2)),
                        v_tap[3]);
      v_sum = vmlal_s16(v_sum, vget_low_s16(vextq_s16(input.val[0], v_zero, 4)),
                        v_tap[4]);
      v_sum = vmlal_s16(v_sum,
                        vget_low_s16(vextq_s16(input.val[0], input.val[1], 6)),
                        v_tap[5]);
    }
    if (is_2d) {
      const uint16x4_t d0 = vreinterpret_u16_s16(
          vqrshrn_n_s32(v_sum, kInterRoundBitsHorizontal - 1));
      dest16[0] = vget_lane_u16(d0, 0);
      dest16[1] = vget_lane_u16(d0, 2);
      dest16 += pred_stride;
      dest16[0] = vget_lane_u16(d0, 1);
      dest16[1] = vget_lane_u16(d0, 3);
      dest16 += pred_stride;
    } else {
      // Normally the Horizontal pass does the downshift in two passes:
      // kInterRoundBitsHorizontal - 1 and then (kFilterBits -
      // kInterRoundBitsHorizontal). Each one uses a rounding shift.
      // Combining them requires adding the rounding offset from the skipped
      // shift.
      const int32x4_t v_first_shift_rounding_bit =
          vdupq_n_s32(1 << (kInterRoundBitsHorizontal - 2));
      v_sum = vaddq_s32(v_sum, v_first_shift_rounding_bit);
      const uint16x4_t d0 =
          vmin_u16(vqrshrun_n_s32(v_sum, kFilterBits - 1), v_max_bitdepth);
      dest16[0] = vget_lane_u16(d0, 0);
      dest16[1] = vget_lane_u16(d0, 2);
      dest16 += pred_stride;
      dest16[0] = vget_lane_u16(d0, 1);
      dest16[1] = vget_lane_u16(d0, 3);
      dest16 += pred_stride;
    }
    src += src_stride << 1;
  } while (--y != 0);

  // The 2d filters have an odd |height| because the horizontal pass
  // generates context for the vertical pass.
  if (is_2d) {
    assert(height % 2 == 1);
    const int16x8_t input = vreinterpretq_s16_u16(vld1q_u16(src));
    int32x4_t v_sum;
    if (num_taps == 2) {
      v_sum = vmull_s16(vget_low_s16(input), v_tap[3]);
      v_sum =
          vmlal_s16(v_sum, vget_low_s16(vextq_s16(input, input, 1)), v_tap[4]);
    } else {
      v_sum = vmull_s16(vget_low_s16(input), v_tap[2]);
      v_sum =
          vmlal_s16(v_sum, vget_low_s16(vextq_s16(input, input, 1)), v_tap[3]);
      v_sum =
          vmlal_s16(v_sum, vget_low_s16(vextq_s16(input, input, 2)), v_tap[4]);
      v_sum =
          vmlal_s16(v_sum, vget_low_s16(vextq_s16(input, input, 3)), v_tap[5]);
    }
    const uint16x4_t d0 = vreinterpret_u16_s16(
        vqrshrn_n_s32(v_sum, kInterRoundBitsHorizontal - 1));
    Store2<0>(dest16, d0);
  }
}

template <int num_taps, bool is_compound, bool is_2d>
void FilterHorizontal(const uint16_t* LIBGAV1_RESTRICT const src,
                      const ptrdiff_t src_stride,
                      void* LIBGAV1_RESTRICT const dest,
                      const ptrdiff_t pred_stride, const int width,
                      const int height, const int16x4_t* const v_tap) {
  // Horizontal passes only needs to account for number of taps 2 and 4 when
  // |width| <= 4.
  assert(width <= 4);
  assert(num_taps == 2 || num_taps == 4);
  if (num_taps == 2 || num_taps == 4) {
    if (width == 2 && !is_compound) {
      FilterHorizontalWidth2<num_taps, is_2d>(src, src_stride, dest,
                                              pred_stride, height, v_tap);
      return;
    }
    assert(width == 4);
    FilterHorizontalWidth4<num_taps, is_compound, is_2d>(
        src, src_stride, dest, pred_stride, height, v_tap);
  } else {
    assert(false);
  }
}

template <bool is_compound = false, bool is_2d = false>
LIBGAV1_ALWAYS_INLINE void DoHorizontalPass(
    const uint16_t* LIBGAV1_RESTRICT const src, const ptrdiff_t src_stride,
    void* LIBGAV1_RESTRICT const dst, const ptrdiff_t dst_stride,
    const int width, const int height, const int filter_id,
    const int filter_index) {
  // Duplicate the absolute value for each tap.  Negative taps are corrected
  // by using the vmlsl_u8 instruction.  Positive taps use vmlal_u8.
  int16x4_t v_tap[kSubPixelTaps];
  assert(filter_id != 0);

  for (int k = 0; k < kSubPixelTaps; ++k) {
    v_tap[k] = vdup_n_s16(kHalfSubPixelFilters[filter_index][filter_id][k]);
  }

  // Horizontal filter.
  // Filter types used for width <= 4 are different from those for width > 4.
  // When width > 4, the valid filter index range is always [0, 3].
  // When width <= 4, the valid filter index range is always [4, 5].
  if (width >= 8) {
    if (filter_index == 2) {  // 8 tap.
      FilterHorizontalWidth8AndUp<8, is_compound, is_2d>(
          src, src_stride, dst, dst_stride, width, height, v_tap);
    } else if (filter_index < 2) {  // 6 tap.
      FilterHorizontalWidth8AndUp<6, is_compound, is_2d>(
          src + 1, src_stride, dst, dst_stride, width, height, v_tap);
    } else {  // 2 tap.
      assert(filter_index == 3);
      FilterHorizontalWidth8AndUp<2, is_compound, is_2d>(
          src + 3, src_stride, dst, dst_stride, width, height, v_tap);
    }
  } else {
    if ((filter_index & 0x4) != 0) {  // 4 tap.
      // ((filter_index == 4) | (filter_index == 5))
      FilterHorizontal<4, is_compound, is_2d>(src + 2, src_stride, dst,
                                              dst_stride, width, height, v_tap);
    } else {  // 2 tap.
      assert(filter_index == 3);
      FilterHorizontal<2, is_compound, is_2d>(src + 3, src_stride, dst,
                                              dst_stride, width, height, v_tap);
    }
  }
}

void ConvolveHorizontal_NEON(
    const void* LIBGAV1_RESTRICT const reference,
    const ptrdiff_t reference_stride, const int horizontal_filter_index,
    const int /*vertical_filter_index*/, const int horizontal_filter_id,
    const int /*vertical_filter_id*/, const int width, const int height,
    void* LIBGAV1_RESTRICT const prediction, const ptrdiff_t pred_stride) {
  const int filter_index = GetFilterIndex(horizontal_filter_index, width);
  // Set |src| to the outermost tap.
  const auto* const src =
      static_cast<const uint16_t*>(reference) - kHorizontalOffset;
  auto* const dest = static_cast<uint16_t*>(prediction);
  const ptrdiff_t src_stride = reference_stride >> 1;
  const ptrdiff_t dst_stride = pred_stride >> 1;

  DoHorizontalPass(src, src_stride, dest, dst_stride, width, height,
                   horizontal_filter_id, filter_index);
}

void ConvolveCompoundHorizontal_NEON(
    const void* LIBGAV1_RESTRICT const reference,
    const ptrdiff_t reference_stride, const int horizontal_filter_index,
    const int /*vertical_filter_index*/, const int horizontal_filter_id,
    const int /*vertical_filter_id*/, const int width, const int height,
    void* LIBGAV1_RESTRICT const prediction, const ptrdiff_t /*pred_stride*/) {
  const int filter_index = GetFilterIndex(horizontal_filter_index, width);
  const auto* const src =
      static_cast<const uint16_t*>(reference) - kHorizontalOffset;
  auto* const dest = static_cast<uint16_t*>(prediction);
  const ptrdiff_t src_stride = reference_stride >> 1;

  DoHorizontalPass</*is_compound=*/true>(src, src_stride, dest, width, width,
                                         height, horizontal_filter_id,
                                         filter_index);
}

template <int num_taps, bool is_compound = false>
void FilterVertical(const uint16_t* LIBGAV1_RESTRICT const src,
                    const ptrdiff_t src_stride,
                    void* LIBGAV1_RESTRICT const dst,
                    const ptrdiff_t dst_stride, const int width,
                    const int height, const int16x4_t* const taps) {
  const int next_row = num_taps - 1;
  const uint16x4_t v_max_bitdepth = vdup_n_u16((1 << kBitdepth10) - 1);
  auto* const dst16 = static_cast<uint16_t*>(dst);
  assert(width >= 8);

  int x = 0;
  do {
    const uint16_t* src_x = src + x;
    uint16x8_t srcs[8];
    srcs[0] = vld1q_u16(src_x);
    src_x += src_stride;
    if (num_taps >= 4) {
      srcs[1] = vld1q_u16(src_x);
      src_x += src_stride;
      srcs[2] = vld1q_u16(src_x);
      src_x += src_stride;
      if (num_taps >= 6) {
        srcs[3] = vld1q_u16(src_x);
        src_x += src_stride;
        srcs[4] = vld1q_u16(src_x);
        src_x += src_stride;
        if (num_taps == 8) {
          srcs[5] = vld1q_u16(src_x);
          src_x += src_stride;
          srcs[6] = vld1q_u16(src_x);
          src_x += src_stride;
        }
      }
    }

    // Decreasing the y loop counter produces worse code with clang.
    // Don't unroll this loop since it generates too much code and the decoder
    // is even slower.
    int y = 0;
    do {
      srcs[next_row] = vld1q_u16(src_x);
      src_x += src_stride;

      const int32x4x2_t v_sum = SumOnePassTaps<num_taps>(srcs, taps);
      if (is_compound) {
        const int16x4_t v_compound_offset = vdup_n_s16(kCompoundOffset);
        const int16x4_t d0 =
            vqrshrn_n_s32(v_sum.val[0], kInterRoundBitsHorizontal - 1);
        const int16x4_t d1 =
            vqrshrn_n_s32(v_sum.val[1], kInterRoundBitsHorizontal - 1);
        vst1_u16(dst16 + x + y * dst_stride,
                 vreinterpret_u16_s16(vadd_s16(d0, v_compound_offset)));
        vst1_u16(dst16 + x + 4 + y * dst_stride,
                 vreinterpret_u16_s16(vadd_s16(d1, v_compound_offset)));
      } else {
        const uint16x4_t d0 = vmin_u16(
            vqrshrun_n_s32(v_sum.val[0], kFilterBits - 1), v_max_bitdepth);
        const uint16x4_t d1 = vmin_u16(
            vqrshrun_n_s32(v_sum.val[1], kFilterBits - 1), v_max_bitdepth);
        vst1_u16(dst16 + x + y * dst_stride, d0);
        vst1_u16(dst16 + x + 4 + y * dst_stride, d1);
      }

      srcs[0] = srcs[1];
      if (num_taps >= 4) {
        srcs[1] = srcs[2];
        srcs[2] = srcs[3];
        if (num_taps >= 6) {
          srcs[3] = srcs[4];
          srcs[4] = srcs[5];
          if (num_taps == 8) {
            srcs[5] = srcs[6];
            srcs[6] = srcs[7];
          }
        }
      }
    } while (++y < height);
    x += 8;
  } while (x < width);
}

template <int num_taps, bool is_compound = false>
void FilterVertical4xH(const uint16_t* LIBGAV1_RESTRICT src,
                       const ptrdiff_t src_stride,
                       void* LIBGAV1_RESTRICT const dst,
                       const ptrdiff_t dst_stride, const int height,
                       const int16x4_t* const taps) {
  const int next_row = num_taps - 1;
  const uint16x4_t v_max_bitdepth = vdup_n_u16((1 << kBitdepth10) - 1);
  auto* dst16 = static_cast<uint16_t*>(dst);

  uint16x4_t srcs[9];
  srcs[0] = vld1_u16(src);
  src += src_stride;
  if (num_taps >= 4) {
    srcs[1] = vld1_u16(src);
    src += src_stride;
    srcs[2] = vld1_u16(src);
    src += src_stride;
    if (num_taps >= 6) {
      srcs[3] = vld1_u16(src);
      src += src_stride;
      srcs[4] = vld1_u16(src);
      src += src_stride;
      if (num_taps == 8) {
        srcs[5] = vld1_u16(src);
        src += src_stride;
        srcs[6] = vld1_u16(src);
        src += src_stride;
      }
    }
  }

  int y = height;
  do {
    srcs[next_row] = vld1_u16(src);
    src += src_stride;
    srcs[num_taps] = vld1_u16(src);
    src += src_stride;

    const int32x4_t v_sum = SumOnePassTaps<num_taps>(srcs, taps);
    const int32x4_t v_sum_1 = SumOnePassTaps<num_taps>(srcs + 1, taps);
    if (is_compound) {
      const int16x4_t d0 = vqrshrn_n_s32(v_sum, kInterRoundBitsHorizontal - 1);
      const int16x4_t d1 =
          vqrshrn_n_s32(v_sum_1, kInterRoundBitsHorizontal - 1);
      vst1_u16(dst16,
               vreinterpret_u16_s16(vadd_s16(d0, vdup_n_s16(kCompoundOffset))));
      dst16 += dst_stride;
      vst1_u16(dst16,
               vreinterpret_u16_s16(vadd_s16(d1, vdup_n_s16(kCompoundOffset))));
      dst16 += dst_stride;
    } else {
      const uint16x4_t d0 =
          vmin_u16(vqrshrun_n_s32(v_sum, kFilterBits - 1), v_max_bitdepth);
      const uint16x4_t d1 =
          vmin_u16(vqrshrun_n_s32(v_sum_1, kFilterBits - 1), v_max_bitdepth);
      vst1_u16(dst16, d0);
      dst16 += dst_stride;
      vst1_u16(dst16, d1);
      dst16 += dst_stride;
    }

    srcs[0] = srcs[2];
    if (num_taps >= 4) {
      srcs[1] = srcs[3];
      srcs[2] = srcs[4];
      if (num_taps >= 6) {
        srcs[3] = srcs[5];
        srcs[4] = srcs[6];
        if (num_taps == 8) {
          srcs[5] = srcs[7];
          srcs[6] = srcs[8];
        }
      }
    }
    y -= 2;
  } while (y != 0);
}

template <int num_taps>
void FilterVertical2xH(const uint16_t* LIBGAV1_RESTRICT src,
                       const ptrdiff_t src_stride,
                       void* LIBGAV1_RESTRICT const dst,
                       const ptrdiff_t dst_stride, const int height,
                       const int16x4_t* const taps) {
  const int next_row = num_taps - 1;
  const uint16x4_t v_max_bitdepth = vdup_n_u16((1 << kBitdepth10) - 1);
  auto* dst16 = static_cast<uint16_t*>(dst);
  const uint16x4_t v_zero = vdup_n_u16(0);

  uint16x4_t srcs[9];
  srcs[0] = Load2<0>(src, v_zero);
  src += src_stride;
  if (num_taps >= 4) {
    srcs[0] = Load2<1>(src, srcs[0]);
    src += src_stride;
    srcs[2] = Load2<0>(src, v_zero);
    src += src_stride;
    srcs[1] = vext_u16(srcs[0], srcs[2], 2);
    if (num_taps >= 6) {
      srcs[2] = Load2<1>(src, srcs[2]);
      src += src_stride;
      srcs[4] = Load2<0>(src, v_zero);
      src += src_stride;
      srcs[3] = vext_u16(srcs[2], srcs[4], 2);
      if (num_taps == 8) {
        srcs[4] = Load2<1>(src, srcs[4]);
        src += src_stride;
        srcs[6] = Load2<0>(src, v_zero);
        src += src_stride;
        srcs[5] = vext_u16(srcs[4], srcs[6], 2);
      }
    }
  }

  int y = height;
  do {
    srcs[next_row - 1] = Load2<1>(src, srcs[next_row - 1]);
    src += src_stride;
    srcs[num_taps] = Load2<0>(src, v_zero);
    src += src_stride;
    srcs[next_row] = vext_u16(srcs[next_row - 1], srcs[num_taps], 2);

    const int32x4_t v_sum = SumOnePassTaps<num_taps>(srcs, taps);
    const uint16x4_t d0 =
        vmin_u16(vqrshrun_n_s32(v_sum, kFilterBits - 1), v_max_bitdepth);
    Store2<0>(dst16, d0);
    dst16 += dst_stride;
    Store2<1>(dst16, d0);
    dst16 += dst_stride;

    srcs[0] = srcs[2];
    if (num_taps >= 4) {
      srcs[1] = srcs[3];
      srcs[2] = srcs[4];
      if (num_taps >= 6) {
        srcs[3] = srcs[5];
        srcs[4] = srcs[6];
        if (num_taps == 8) {
          srcs[5] = srcs[7];
          srcs[6] = srcs[8];
        }
      }
    }
    y -= 2;
  } while (y != 0);
}

template <int num_taps, bool is_compound>
int16x8_t SimpleSum2DVerticalTaps(const int16x8_t* const src,
                                  const int16x8_t taps) {
  const int16x4_t taps_lo = vget_low_s16(taps);
  const int16x4_t taps_hi = vget_high_s16(taps);
  int32x4_t sum_lo, sum_hi;
  if (num_taps == 8) {
    sum_lo = vmull_lane_s16(vget_low_s16(src[0]), taps_lo, 0);
    sum_hi = vmull_lane_s16(vget_high_s16(src[0]), taps_lo, 0);
    sum_lo = vmlal_lane_s16(sum_lo, vget_low_s16(src[1]), taps_lo, 1);
    sum_hi = vmlal_lane_s16(sum_hi, vget_high_s16(src[1]), taps_lo, 1);
    sum_lo = vmlal_lane_s16(sum_lo, vget_low_s16(src[2]), taps_lo, 2);
    sum_hi = vmlal_lane_s16(sum_hi, vget_high_s16(src[2]), taps_lo, 2);
    sum_lo = vmlal_lane_s16(sum_lo, vget_low_s16(src[3]), taps_lo, 3);
    sum_hi = vmlal_lane_s16(sum_hi, vget_high_s16(src[3]), taps_lo, 3);

    sum_lo = vmlal_lane_s16(sum_lo, vget_low_s16(src[4]), taps_hi, 0);
    sum_hi = vmlal_lane_s16(sum_hi, vget_high_s16(src[4]), taps_hi, 0);
    sum_lo = vmlal_lane_s16(sum_lo, vget_low_s16(src[5]), taps_hi, 1);
    sum_hi = vmlal_lane_s16(sum_hi, vget_high_s16(src[5]), taps_hi, 1);
    sum_lo = vmlal_lane_s16(sum_lo, vget_low_s16(src[6]), taps_hi, 2);
    sum_hi = vmlal_lane_s16(sum_hi, vget_high_s16(src[6]), taps_hi, 2);
    sum_lo = vmlal_lane_s16(sum_lo, vget_low_s16(src[7]), taps_hi, 3);
    sum_hi = vmlal_lane_s16(sum_hi, vget_high_s16(src[7]), taps_hi, 3);
  } else if (num_taps == 6) {
    sum_lo = vmull_lane_s16(vget_low_s16(src[0]), taps_lo, 1);
    sum_hi = vmull_lane_s16(vget_high_s16(src[0]), taps_lo, 1);
    sum_lo = vmlal_lane_s16(sum_lo, vget_low_s16(src[1]), taps_lo, 2);
    sum_hi = vmlal_lane_s16(sum_hi, vget_high_s16(src[1]), taps_lo, 2);
    sum_lo = vmlal_lane_s16(sum_lo, vget_low_s16(src[2]), taps_lo, 3);
    sum_hi = vmlal_lane_s16(sum_hi, vget_high_s16(src[2]), taps_lo, 3);

    sum_lo = vmlal_lane_s16(sum_lo, vget_low_s16(src[3]), taps_hi, 0);
    sum_hi = vmlal_lane_s16(sum_hi, vget_high_s16(src[3]), taps_hi, 0);
    sum_lo = vmlal_lane_s16(sum_lo, vget_low_s16(src[4]), taps_hi, 1);
    sum_hi = vmlal_lane_s16(sum_hi, vget_high_s16(src[4]), taps_hi, 1);
    sum_lo = vmlal_lane_s16(sum_lo, vget_low_s16(src[5]), taps_hi, 2);
    sum_hi = vmlal_lane_s16(sum_hi, vget_high_s16(src[5]), taps_hi, 2);
  } else if (num_taps == 4) {
    sum_lo = vmull_lane_s16(vget_low_s16(src[0]), taps_lo, 2);
    sum_hi = vmull_lane_s16(vget_high_s16(src[0]), taps_lo, 2);
    sum_lo = vmlal_lane_s16(sum_lo, vget_low_s16(src[1]), taps_lo, 3);
    sum_hi = vmlal_lane_s16(sum_hi, vget_high_s16(src[1]), taps_lo, 3);

    sum_lo = vmlal_lane_s16(sum_lo, vget_low_s16(src[2]), taps_hi, 0);
    sum_hi = vmlal_lane_s16(sum_hi, vget_high_s16(src[2]), taps_hi, 0);
    sum_lo = vmlal_lane_s16(sum_lo, vget_low_s16(src[3]), taps_hi, 1);
    sum_hi = vmlal_lane_s16(sum_hi, vget_high_s16(src[3]), taps_hi, 1);
  } else if (num_taps == 2) {
    sum_lo = vmull_lane_s16(vget_low_s16(src[0]), taps_lo, 3);
    sum_hi = vmull_lane_s16(vget_high_s16(src[0]), taps_lo, 3);

    sum_lo = vmlal_lane_s16(sum_lo, vget_low_s16(src[1]), taps_hi, 0);
    sum_hi = vmlal_lane_s16(sum_hi, vget_high_s16(src[1]), taps_hi, 0);
  }

  if (is_compound) {
    // Output is compound, so leave signed and do not saturate. Offset will
    // accurately bring the value back into positive range.
    return vcombine_s16(
        vrshrn_n_s32(sum_lo, kInterRoundBitsCompoundVertical - 1),
        vrshrn_n_s32(sum_hi, kInterRoundBitsCompoundVertical - 1));
  }

  // Output is pixel, so saturate to clip at 0.
  return vreinterpretq_s16_u16(
      vcombine_u16(vqrshrun_n_s32(sum_lo, kInterRoundBitsVertical - 1),
                   vqrshrun_n_s32(sum_hi, kInterRoundBitsVertical - 1)));
}

template <int num_taps, bool is_compound = false>
void Filter2DVerticalWidth8AndUp(const int16_t* LIBGAV1_RESTRICT src,
                                 void* LIBGAV1_RESTRICT const dst,
                                 const ptrdiff_t dst_stride, const int width,
                                 const int height, const int16x8_t taps) {
  assert(width >= 8);
  constexpr int next_row = num_taps - 1;
  const uint16x8_t v_max_bitdepth = vdupq_n_u16((1 << kBitdepth10) - 1);
  auto* const dst16 = static_cast<uint16_t*>(dst);

  int x = 0;
  do {
    int16x8_t srcs[9];
    srcs[0] = vld1q_s16(src);
    src += 8;
    if (num_taps >= 4) {
      srcs[1] = vld1q_s16(src);
      src += 8;
      srcs[2] = vld1q_s16(src);
      src += 8;
      if (num_taps >= 6) {
        srcs[3] = vld1q_s16(src);
        src += 8;
        srcs[4] = vld1q_s16(src);
        src += 8;
        if (num_taps == 8) {
          srcs[5] = vld1q_s16(src);
          src += 8;
          srcs[6] = vld1q_s16(src);
          src += 8;
        }
      }
    }

    uint16_t* d16 = dst16 + x;
    int y = height;
    do {
      srcs[next_row] = vld1q_s16(src);
      src += 8;
      srcs[next_row + 1] = vld1q_s16(src);
      src += 8;
      const int16x8_t sum0 =
          SimpleSum2DVerticalTaps<num_taps, is_compound>(srcs + 0, taps);
      const int16x8_t sum1 =
          SimpleSum2DVerticalTaps<num_taps, is_compound>(srcs + 1, taps);
      if (is_compound) {
        const int16x8_t v_compound_offset = vdupq_n_s16(kCompoundOffset);
        vst1q_u16(d16,
                  vreinterpretq_u16_s16(vaddq_s16(sum0, v_compound_offset)));
        d16 += dst_stride;
        vst1q_u16(d16,
                  vreinterpretq_u16_s16(vaddq_s16(sum1, v_compound_offset)));
        d16 += dst_stride;
      } else {
        vst1q_u16(d16, vminq_u16(vreinterpretq_u16_s16(sum0), v_max_bitdepth));
        d16 += dst_stride;
        vst1q_u16(d16, vminq_u16(vreinterpretq_u16_s16(sum1), v_max_bitdepth));
        d16 += dst_stride;
      }
      srcs[0] = srcs[2];
      if (num_taps >= 4) {
        srcs[1] = srcs[3];
        srcs[2] = srcs[4];
        if (num_taps >= 6) {
          srcs[3] = srcs[5];
          srcs[4] = srcs[6];
          if (num_taps == 8) {
            srcs[5] = srcs[7];
            srcs[6] = srcs[8];
          }
        }
      }
      y -= 2;
    } while (y != 0);
    x += 8;
  } while (x < width);
}

// Take advantage of |src_stride| == |width| to process two rows at a time.
template <int num_taps, bool is_compound = false>
void Filter2DVerticalWidth4(const int16_t* LIBGAV1_RESTRICT src,
                            void* LIBGAV1_RESTRICT const dst,
                            const ptrdiff_t dst_stride, const int height,
                            const int16x8_t taps) {
  const uint16x8_t v_max_bitdepth = vdupq_n_u16((1 << kBitdepth10) - 1);
  auto* dst16 = static_cast<uint16_t*>(dst);

  int16x8_t srcs[9];
  srcs[0] = vld1q_s16(src);
  src += 8;
  if (num_taps >= 4) {
    srcs[2] = vld1q_s16(src);
    src += 8;
    srcs[1] = vcombine_s16(vget_high_s16(srcs[0]), vget_low_s16(srcs[2]));
    if (num_taps >= 6) {
      srcs[4] = vld1q_s16(src);
      src += 8;
      srcs[3] = vcombine_s16(vget_high_s16(srcs[2]), vget_low_s16(srcs[4]));
      if (num_taps == 8) {
        srcs[6] = vld1q_s16(src);
        src += 8;
        srcs[5] = vcombine_s16(vget_high_s16(srcs[4]), vget_low_s16(srcs[6]));
      }
    }
  }

  int y = height;
  do {
    srcs[num_taps] = vld1q_s16(src);
    src += 8;
    srcs[num_taps - 1] = vcombine_s16(vget_high_s16(srcs[num_taps - 2]),
                                      vget_low_s16(srcs[num_taps]));

    const int16x8_t sum =
        SimpleSum2DVerticalTaps<num_taps, is_compound>(srcs, taps);
    if (is_compound) {
      const int16x8_t v_compound_offset = vdupq_n_s16(kCompoundOffset);
      vst1q_u16(dst16,
                vreinterpretq_u16_s16(vaddq_s16(sum, v_compound_offset)));
      dst16 += 4 << 1;
    } else {
      const uint16x8_t d0 =
          vminq_u16(vreinterpretq_u16_s16(sum), v_max_bitdepth);
      vst1_u16(dst16, vget_low_u16(d0));
      dst16 += dst_stride;
      vst1_u16(dst16, vget_high_u16(d0));
      dst16 += dst_stride;
    }

    srcs[0] = srcs[2];
    if (num_taps >= 4) {
      srcs[1] = srcs[3];
      srcs[2] = srcs[4];
      if (num_taps >= 6) {
        srcs[3] = srcs[5];
        srcs[4] = srcs[6];
        if (num_taps == 8) {
          srcs[5] = srcs[7];
          srcs[6] = srcs[8];
        }
      }
    }
    y -= 2;
  } while (y != 0);
}

// Take advantage of |src_stride| == |width| to process four rows at a time.
template <int num_taps>
void Filter2DVerticalWidth2(const int16_t* LIBGAV1_RESTRICT src,
                            void* LIBGAV1_RESTRICT const dst,
                            const ptrdiff_t dst_stride, const int height,
                            const int16x8_t taps) {
  constexpr int next_row = (num_taps < 6) ? 4 : 8;
  const uint16x8_t v_max_bitdepth = vdupq_n_u16((1 << kBitdepth10) - 1);
  auto* dst16 = static_cast<uint16_t*>(dst);

  int16x8_t srcs[9];
  srcs[0] = vld1q_s16(src);
  src += 8;
  if (num_taps >= 6) {
    srcs[4] = vld1q_s16(src);
    src += 8;
    srcs[1] = vextq_s16(srcs[0], srcs[4], 2);
    if (num_taps == 8) {
      srcs[2] = vcombine_s16(vget_high_s16(srcs[0]), vget_low_s16(srcs[4]));
      srcs[3] = vextq_s16(srcs[0], srcs[4], 6);
    }
  }

  int y = height;
  do {
    srcs[next_row] = vld1q_s16(src);
    src += 8;
    if (num_taps == 2) {
      srcs[1] = vextq_s16(srcs[0], srcs[4], 2);
    } else if (num_taps == 4) {
      srcs[1] = vextq_s16(srcs[0], srcs[4], 2);
      srcs[2] = vcombine_s16(vget_high_s16(srcs[0]), vget_low_s16(srcs[4]));
      srcs[3] = vextq_s16(srcs[0], srcs[4], 6);
    } else if (num_taps == 6) {
      srcs[2] = vcombine_s16(vget_high_s16(srcs[0]), vget_low_s16(srcs[4]));
      srcs[3] = vextq_s16(srcs[0], srcs[4], 6);
      srcs[5] = vextq_s16(srcs[4], srcs[8], 2);
    } else if (num_taps == 8) {
      srcs[5] = vextq_s16(srcs[4], srcs[8], 2);
      srcs[6] = vcombine_s16(vget_high_s16(srcs[4]), vget_low_s16(srcs[8]));
      srcs[7] = vextq_s16(srcs[4], srcs[8], 6);
    }
    const int16x8_t sum =
        SimpleSum2DVerticalTaps<num_taps, /*is_compound=*/false>(srcs, taps);
    const uint16x8_t d0 = vminq_u16(vreinterpretq_u16_s16(sum), v_max_bitdepth);
    Store2<0>(dst16, d0);
    dst16 += dst_stride;
    Store2<1>(dst16, d0);
    // When |height| <= 4 the taps are restricted to 2 and 4 tap variants.
    // Therefore we don't need to check this condition when |height| > 4.
    if (num_taps <= 4 && height == 2) return;
    dst16 += dst_stride;
    Store2<2>(dst16, d0);
    dst16 += dst_stride;
    Store2<3>(dst16, d0);
    dst16 += dst_stride;

    srcs[0] = srcs[4];
    if (num_taps == 6) {
      srcs[1] = srcs[5];
      srcs[4] = srcs[8];
    } else if (num_taps == 8) {
      srcs[1] = srcs[5];
      srcs[2] = srcs[6];
      srcs[3] = srcs[7];
      srcs[4] = srcs[8];
    }

    y -= 4;
  } while (y != 0);
}

template <int vertical_taps>
void Filter2DVertical(const int16_t* LIBGAV1_RESTRICT const intermediate_result,
                      const int width, const int height, const int16x8_t taps,
                      void* LIBGAV1_RESTRICT const prediction,
                      const ptrdiff_t pred_stride) {
  auto* const dest = static_cast<uint16_t*>(prediction);
  if (width >= 8) {
    Filter2DVerticalWidth8AndUp<vertical_taps>(
        intermediate_result, dest, pred_stride, width, height, taps);
  } else if (width == 4) {
    Filter2DVerticalWidth4<vertical_taps>(intermediate_result, dest,
                                          pred_stride, height, taps);
  } else {
    assert(width == 2);
    Filter2DVerticalWidth2<vertical_taps>(intermediate_result, dest,
                                          pred_stride, height, taps);
  }
}

void Convolve2D_NEON(const void* LIBGAV1_RESTRICT const reference,
                     const ptrdiff_t reference_stride,
                     const int horizontal_filter_index,
                     const int vertical_filter_index,
                     const int horizontal_filter_id,
                     const int vertical_filter_id, const int width,
                     const int height, void* LIBGAV1_RESTRICT const prediction,
                     const ptrdiff_t pred_stride) {
  const int horiz_filter_index = GetFilterIndex(horizontal_filter_index, width);
  const int vert_filter_index = GetFilterIndex(vertical_filter_index, height);
  const int vertical_taps = GetNumTapsInFilter(vert_filter_index);
  // The output of the horizontal filter is guaranteed to fit in 16 bits.
  int16_t intermediate_result[kMaxSuperBlockSizeInPixels *
                              (kMaxSuperBlockSizeInPixels + kSubPixelTaps - 1)];
#if LIBGAV1_MSAN
  // Quiet msan warnings. Set with random non-zero value to aid in debugging.
  memset(intermediate_result, 0x43, sizeof(intermediate_result));
#endif
  const int intermediate_height = height + vertical_taps - 1;
  const ptrdiff_t src_stride = reference_stride >> 1;
  const auto* const src = static_cast<const uint16_t*>(reference) -
                          (vertical_taps / 2 - 1) * src_stride -
                          kHorizontalOffset;
  const ptrdiff_t dest_stride = pred_stride >> 1;

  DoHorizontalPass</*is_compound=*/false, /*is_2d=*/true>(
      src, src_stride, intermediate_result, width, width, intermediate_height,
      horizontal_filter_id, horiz_filter_index);

  assert(vertical_filter_id != 0);
  const int16x8_t taps = vmovl_s8(
      vld1_s8(kHalfSubPixelFilters[vert_filter_index][vertical_filter_id]));
  if (vertical_taps == 8) {
    Filter2DVertical<8>(intermediate_result, width, height, taps, prediction,
                        dest_stride);
  } else if (vertical_taps == 6) {
    Filter2DVertical<6>(intermediate_result, width, height, taps, prediction,
                        dest_stride);
  } else if (vertical_taps == 4) {
    Filter2DVertical<4>(intermediate_result, width, height, taps, prediction,
                        dest_stride);
  } else {  // |vertical_taps| == 2
    Filter2DVertical<2>(intermediate_result, width, height, taps, prediction,
                        dest_stride);
  }
}

template <int vertical_taps>
void Compound2DVertical(
    const int16_t* LIBGAV1_RESTRICT const intermediate_result, const int width,
    const int height, const int16x8_t taps,
    void* LIBGAV1_RESTRICT const prediction) {
  auto* const dest = static_cast<uint16_t*>(prediction);
  if (width == 4) {
    Filter2DVerticalWidth4<vertical_taps, /*is_compound=*/true>(
        intermediate_result, dest, width, height, taps);
  } else {
    Filter2DVerticalWidth8AndUp<vertical_taps, /*is_compound=*/true>(
        intermediate_result, dest, width, width, height, taps);
  }
}

void ConvolveCompound2D_NEON(
    const void* LIBGAV1_RESTRICT const reference,
    const ptrdiff_t reference_stride, const int horizontal_filter_index,
    const int vertical_filter_index, const int horizontal_filter_id,
    const int vertical_filter_id, const int width, const int height,
    void* LIBGAV1_RESTRICT const prediction, const ptrdiff_t /*pred_stride*/) {
  // The output of the horizontal filter, i.e. the intermediate_result, is
  // guaranteed to fit in int16_t.
  int16_t
      intermediate_result[(kMaxSuperBlockSizeInPixels *
                           (kMaxSuperBlockSizeInPixels + kSubPixelTaps - 1))];

  // Horizontal filter.
  // Filter types used for width <= 4 are different from those for width > 4.
  // When width > 4, the valid filter index range is always [0, 3].
  // When width <= 4, the valid filter index range is always [4, 5].
  // Similarly for height.
  const int horiz_filter_index = GetFilterIndex(horizontal_filter_index, width);
  const int vert_filter_index = GetFilterIndex(vertical_filter_index, height);
  const int vertical_taps = GetNumTapsInFilter(vert_filter_index);
  const int intermediate_height = height + vertical_taps - 1;
  const ptrdiff_t src_stride = reference_stride >> 1;
  const auto* const src = static_cast<const uint16_t*>(reference) -
                          (vertical_taps / 2 - 1) * src_stride -
                          kHorizontalOffset;

  DoHorizontalPass</*is_2d=*/true, /*is_compound=*/true>(
      src, src_stride, intermediate_result, width, width, intermediate_height,
      horizontal_filter_id, horiz_filter_index);

  // Vertical filter.
  assert(vertical_filter_id != 0);
  const int16x8_t taps = vmovl_s8(
      vld1_s8(kHalfSubPixelFilters[vert_filter_index][vertical_filter_id]));
  if (vertical_taps == 8) {
    Compound2DVertical<8>(intermediate_result, width, height, taps, prediction);
  } else if (vertical_taps == 6) {
    Compound2DVertical<6>(intermediate_result, width, height, taps, prediction);
  } else if (vertical_taps == 4) {
    Compound2DVertical<4>(intermediate_result, width, height, taps, prediction);
  } else {  // |vertical_taps| == 2
    Compound2DVertical<2>(intermediate_result, width, height, taps, prediction);
  }
}

void ConvolveVertical_NEON(
    const void* LIBGAV1_RESTRICT const reference,
    const ptrdiff_t reference_stride, const int /*horizontal_filter_index*/,
    const int vertical_filter_index, const int /*horizontal_filter_id*/,
    const int vertical_filter_id, const int width, const int height,
    void* LIBGAV1_RESTRICT const prediction, const ptrdiff_t pred_stride) {
  const int filter_index = GetFilterIndex(vertical_filter_index, height);
  const int vertical_taps = GetNumTapsInFilter(filter_index);
  const ptrdiff_t src_stride = reference_stride >> 1;
  const auto* src = static_cast<const uint16_t*>(reference) -
                    (vertical_taps / 2 - 1) * src_stride;
  auto* const dest = static_cast<uint16_t*>(prediction);
  const ptrdiff_t dest_stride = pred_stride >> 1;
  assert(vertical_filter_id != 0);

  int16x4_t taps[8];
  for (int k = 0; k < kSubPixelTaps; ++k) {
    taps[k] =
        vdup_n_s16(kHalfSubPixelFilters[filter_index][vertical_filter_id][k]);
  }

  if (filter_index == 0) {  // 6 tap.
    if (width == 2) {
      FilterVertical2xH<6>(src, src_stride, dest, dest_stride, height,
                           taps + 1);
    } else if (width == 4) {
      FilterVertical4xH<6>(src, src_stride, dest, dest_stride, height,
                           taps + 1);
    } else {
      FilterVertical<6>(src, src_stride, dest, dest_stride, width, height,
                        taps + 1);
    }
  } else if ((static_cast<int>(filter_index == 1) &
              (static_cast<int>(vertical_filter_id == 1) |
               static_cast<int>(vertical_filter_id == 7) |
               static_cast<int>(vertical_filter_id == 8) |
               static_cast<int>(vertical_filter_id == 9) |
               static_cast<int>(vertical_filter_id == 15))) != 0) {  // 6 tap.
    if (width == 2) {
      FilterVertical2xH<6>(src, src_stride, dest, dest_stride, height,
                           taps + 1);
    } else if (width == 4) {
      FilterVertical4xH<6>(src, src_stride, dest, dest_stride, height,
                           taps + 1);
    } else {
      FilterVertical<6>(src, src_stride, dest, dest_stride, width, height,
                        taps + 1);
    }
  } else if (filter_index == 2) {  // 8 tap.
    if (width == 2) {
      FilterVertical2xH<8>(src, src_stride, dest, dest_stride, height, taps);
    } else if (width == 4) {
      FilterVertical4xH<8>(src, src_stride, dest, dest_stride, height, taps);
    } else {
      FilterVertical<8>(src, src_stride, dest, dest_stride, width, height,
                        taps);
    }
  } else if (filter_index == 3) {  // 2 tap.
    if (width == 2) {
      FilterVertical2xH<2>(src, src_stride, dest, dest_stride, height,
                           taps + 3);
    } else if (width == 4) {
      FilterVertical4xH<2>(src, src_stride, dest, dest_stride, height,
                           taps + 3);
    } else {
      FilterVertical<2>(src, src_stride, dest, dest_stride, width, height,
                        taps + 3);
    }
  } else {
    // 4 tap. When |filter_index| == 1 the |vertical_filter_id| values listed
    // below map to 4 tap filters.
    assert(filter_index == 5 || filter_index == 4 ||
           (filter_index == 1 &&
            (vertical_filter_id == 0 || vertical_filter_id == 2 ||
             vertical_filter_id == 3 || vertical_filter_id == 4 ||
             vertical_filter_id == 5 || vertical_filter_id == 6 ||
             vertical_filter_id == 10 || vertical_filter_id == 11 ||
             vertical_filter_id == 12 || vertical_filter_id == 13 ||
             vertical_filter_id == 14)));
    // According to GetNumTapsInFilter() this has 6 taps but here we are
    // treating it as though it has 4.
    if (filter_index == 1) src += src_stride;
    if (width == 2) {
      FilterVertical2xH<4>(src, src_stride, dest, dest_stride, height,
                           taps + 2);
    } else if (width == 4) {
      FilterVertical4xH<4>(src, src_stride, dest, dest_stride, height,
                           taps + 2);
    } else {
      FilterVertical<4>(src, src_stride, dest, dest_stride, width, height,
                        taps + 2);
    }
  }
}

void ConvolveCompoundVertical_NEON(
    const void* LIBGAV1_RESTRICT const reference,
    const ptrdiff_t reference_stride, const int /*horizontal_filter_index*/,
    const int vertical_filter_index, const int /*horizontal_filter_id*/,
    const int vertical_filter_id, const int width, const int height,
    void* LIBGAV1_RESTRICT const prediction, const ptrdiff_t /*pred_stride*/) {
  const int filter_index = GetFilterIndex(vertical_filter_index, height);
  const int vertical_taps = GetNumTapsInFilter(filter_index);
  const ptrdiff_t src_stride = reference_stride >> 1;
  const auto* src = static_cast<const uint16_t*>(reference) -
                    (vertical_taps / 2 - 1) * src_stride;
  auto* const dest = static_cast<uint16_t*>(prediction);
  assert(vertical_filter_id != 0);

  int16x4_t taps[8];
  for (int k = 0; k < kSubPixelTaps; ++k) {
    taps[k] =
        vdup_n_s16(kHalfSubPixelFilters[filter_index][vertical_filter_id][k]);
  }

  if (filter_index == 0) {  // 6 tap.
    if (width == 4) {
      FilterVertical4xH<6, /*is_compound=*/true>(src, src_stride, dest, 4,
                                                 height, taps + 1);
    } else {
      FilterVertical<6, /*is_compound=*/true>(src, src_stride, dest, width,
                                              width, height, taps + 1);
    }
  } else if ((static_cast<int>(filter_index == 1) &
              (static_cast<int>(vertical_filter_id == 1) |
               static_cast<int>(vertical_filter_id == 7) |
               static_cast<int>(vertical_filter_id == 8) |
               static_cast<int>(vertical_filter_id == 9) |
               static_cast<int>(vertical_filter_id == 15))) != 0) {  // 6 tap.
    if (width == 4) {
      FilterVertical4xH<6, /*is_compound=*/true>(src, src_stride, dest, 4,
                                                 height, taps + 1);
    } else {
      FilterVertical<6, /*is_compound=*/true>(src, src_stride, dest, width,
                                              width, height, taps + 1);
    }
  } else if (filter_index == 2) {  // 8 tap.
    if (width == 4) {
      FilterVertical4xH<8, /*is_compound=*/true>(src, src_stride, dest, 4,
                                                 height, taps);
    } else {
      FilterVertical<8, /*is_compound=*/true>(src, src_stride, dest, width,
                                              width, height, taps);
    }
  } else if (filter_index == 3) {  // 2 tap.
    if (width == 4) {
      FilterVertical4xH<2, /*is_compound=*/true>(src, src_stride, dest, 4,
                                                 height, taps + 3);
    } else {
      FilterVertical<2, /*is_compound=*/true>(src, src_stride, dest, width,
                                              width, height, taps + 3);
    }
  } else {
    // 4 tap. When |filter_index| == 1 the |filter_id| values listed below map
    // to 4 tap filters.
    assert(filter_index == 5 || filter_index == 4 ||
           (filter_index == 1 &&
            (vertical_filter_id == 2 || vertical_filter_id == 3 ||
             vertical_filter_id == 4 || vertical_filter_id == 5 ||
             vertical_filter_id == 6 || vertical_filter_id == 10 ||
             vertical_filter_id == 11 || vertical_filter_id == 12 ||
             vertical_filter_id == 13 || vertical_filter_id == 14)));
    // According to GetNumTapsInFilter() this has 6 taps but here we are
    // treating it as though it has 4.
    if (filter_index == 1) src += src_stride;
    if (width == 4) {
      FilterVertical4xH<4, /*is_compound=*/true>(src, src_stride, dest, 4,
                                                 height, taps + 2);
    } else {
      FilterVertical<4, /*is_compound=*/true>(src, src_stride, dest, width,
                                              width, height, taps + 2);
    }
  }
}

void ConvolveCompoundCopy_NEON(
    const void* const reference, const ptrdiff_t reference_stride,
    const int /*horizontal_filter_index*/, const int /*vertical_filter_index*/,
    const int /*horizontal_filter_id*/, const int /*vertical_filter_id*/,
    const int width, const int height, void* const prediction,
    const ptrdiff_t /*pred_stride*/) {
  const auto* src = static_cast<const uint16_t*>(reference);
  const ptrdiff_t src_stride = reference_stride >> 1;
  auto* dest = static_cast<uint16_t*>(prediction);
  constexpr int final_shift =
      kInterRoundBitsVertical - kInterRoundBitsCompoundVertical;
  const uint16x8_t offset =
      vdupq_n_u16((1 << kBitdepth10) + (1 << (kBitdepth10 - 1)));

  if (width >= 16) {
    int y = height;
    do {
      int x = 0;
      int w = width;
      do {
        const uint16x8_t v_src_lo = vld1q_u16(&src[x]);
        const uint16x8_t v_src_hi = vld1q_u16(&src[x + 8]);
        const uint16x8_t v_sum_lo = vaddq_u16(v_src_lo, offset);
        const uint16x8_t v_sum_hi = vaddq_u16(v_src_hi, offset);
        const uint16x8_t v_dest_lo = vshlq_n_u16(v_sum_lo, final_shift);
        const uint16x8_t v_dest_hi = vshlq_n_u16(v_sum_hi, final_shift);
        vst1q_u16(&dest[x], v_dest_lo);
        vst1q_u16(&dest[x + 8], v_dest_hi);
        x += 16;
        w -= 16;
      } while (w != 0);
      src += src_stride;
      dest += width;
    } while (--y != 0);
  } else if (width == 8) {
    int y = height;
    do {
      const uint16x8_t v_src_lo = vld1q_u16(&src[0]);
      const uint16x8_t v_src_hi = vld1q_u16(&src[src_stride]);
      const uint16x8_t v_sum_lo = vaddq_u16(v_src_lo, offset);
      const uint16x8_t v_sum_hi = vaddq_u16(v_src_hi, offset);
      const uint16x8_t v_dest_lo = vshlq_n_u16(v_sum_lo, final_shift);
      const uint16x8_t v_dest_hi = vshlq_n_u16(v_sum_hi, final_shift);
      vst1q_u16(&dest[0], v_dest_lo);
      vst1q_u16(&dest[8], v_dest_hi);
      src += src_stride << 1;
      dest += 16;
      y -= 2;
    } while (y != 0);
  } else {  // width == 4
    int y = height;
    do {
      const uint16x4_t v_src_lo = vld1_u16(&src[0]);
      const uint16x4_t v_src_hi = vld1_u16(&src[src_stride]);
      const uint16x4_t v_sum_lo = vadd_u16(v_src_lo, vget_low_u16(offset));
      const uint16x4_t v_sum_hi = vadd_u16(v_src_hi, vget_low_u16(offset));
      const uint16x4_t v_dest_lo = vshl_n_u16(v_sum_lo, final_shift);
      const uint16x4_t v_dest_hi = vshl_n_u16(v_sum_hi, final_shift);
      vst1_u16(&dest[0], v_dest_lo);
      vst1_u16(&dest[4], v_dest_hi);
      src += src_stride << 1;
      dest += 8;
      y -= 2;
    } while (y != 0);
  }
}

inline void HalfAddHorizontal(const uint16_t* LIBGAV1_RESTRICT const src,
                              uint16_t* LIBGAV1_RESTRICT const dst) {
  const uint16x8_t left = vld1q_u16(src);
  const uint16x8_t right = vld1q_u16(src + 1);
  vst1q_u16(dst, vrhaddq_u16(left, right));
}

inline void HalfAddHorizontal16(const uint16_t* LIBGAV1_RESTRICT const src,
                                uint16_t* LIBGAV1_RESTRICT const dst) {
  HalfAddHorizontal(src, dst);
  HalfAddHorizontal(src + 8, dst + 8);
}

template <int width>
inline void IntraBlockCopyHorizontal(const uint16_t* LIBGAV1_RESTRICT src,
                                     const ptrdiff_t src_stride,
                                     const int height,
                                     uint16_t* LIBGAV1_RESTRICT dst,
                                     const ptrdiff_t dst_stride) {
  const ptrdiff_t src_remainder_stride = src_stride - (width - 16);
  const ptrdiff_t dst_remainder_stride = dst_stride - (width - 16);

  int y = height;
  do {
    HalfAddHorizontal16(src, dst);
    if (width >= 32) {
      src += 16;
      dst += 16;
      HalfAddHorizontal16(src, dst);
      if (width >= 64) {
        src += 16;
        dst += 16;
        HalfAddHorizontal16(src, dst);
        src += 16;
        dst += 16;
        HalfAddHorizontal16(src, dst);
        if (width == 128) {
          src += 16;
          dst += 16;
          HalfAddHorizontal16(src, dst);
          src += 16;
          dst += 16;
          HalfAddHorizontal16(src, dst);
          src += 16;
          dst += 16;
          HalfAddHorizontal16(src, dst);
          src += 16;
          dst += 16;
          HalfAddHorizontal16(src, dst);
        }
      }
    }
    src += src_remainder_stride;
    dst += dst_remainder_stride;
  } while (--y != 0);
}

void ConvolveIntraBlockCopyHorizontal_NEON(
    const void* LIBGAV1_RESTRICT const reference,
    const ptrdiff_t reference_stride, const int /*horizontal_filter_index*/,
    const int /*vertical_filter_index*/, const int /*subpixel_x*/,
    const int /*subpixel_y*/, const int width, const int height,
    void* LIBGAV1_RESTRICT const prediction, const ptrdiff_t pred_stride) {
  assert(width >= 4 && width <= kMaxSuperBlockSizeInPixels);
  assert(height >= 4 && height <= kMaxSuperBlockSizeInPixels);
  const auto* src = static_cast<const uint16_t*>(reference);
  auto* dest = static_cast<uint16_t*>(prediction);
  const ptrdiff_t src_stride = reference_stride >> 1;
  const ptrdiff_t dst_stride = pred_stride >> 1;

  if (width == 128) {
    IntraBlockCopyHorizontal<128>(src, src_stride, height, dest, dst_stride);
  } else if (width == 64) {
    IntraBlockCopyHorizontal<64>(src, src_stride, height, dest, dst_stride);
  } else if (width == 32) {
    IntraBlockCopyHorizontal<32>(src, src_stride, height, dest, dst_stride);
  } else if (width == 16) {
    IntraBlockCopyHorizontal<16>(src, src_stride, height, dest, dst_stride);
  } else if (width == 8) {
    int y = height;
    do {
      HalfAddHorizontal(src, dest);
      src += src_stride;
      dest += dst_stride;
    } while (--y != 0);
  } else {  // width == 4
    int y = height;
    do {
      uint16x4x2_t left;
      uint16x4x2_t right;
      left.val[0] = vld1_u16(src);
      right.val[0] = vld1_u16(src + 1);
      src += src_stride;
      left.val[1] = vld1_u16(src);
      right.val[1] = vld1_u16(src + 1);
      src += src_stride;

      vst1_u16(dest, vrhadd_u16(left.val[0], right.val[0]));
      dest += dst_stride;
      vst1_u16(dest, vrhadd_u16(left.val[1], right.val[1]));
      dest += dst_stride;
      y -= 2;
    } while (y != 0);
  }
}

template <int width>
inline void IntraBlockCopyVertical(const uint16_t* LIBGAV1_RESTRICT src,
                                   const ptrdiff_t src_stride, const int height,
                                   uint16_t* LIBGAV1_RESTRICT dst,
                                   const ptrdiff_t dst_stride) {
  const ptrdiff_t src_remainder_stride = src_stride - (width - 8);
  const ptrdiff_t dst_remainder_stride = dst_stride - (width - 8);
  uint16x8_t row[8], below[8];

  row[0] = vld1q_u16(src);
  if (width >= 16) {
    src += 8;
    row[1] = vld1q_u16(src);
    if (width >= 32) {
      src += 8;
      row[2] = vld1q_u16(src);
      src += 8;
      row[3] = vld1q_u16(src);
      if (width == 64) {
        src += 8;
        row[4] = vld1q_u16(src);
        src += 8;
        row[5] = vld1q_u16(src);
        src += 8;
        row[6] = vld1q_u16(src);
        src += 8;
        row[7] = vld1q_u16(src);
      }
    }
  }
  src += src_remainder_stride;

  int y = height;
  do {
    below[0] = vld1q_u16(src);
    if (width >= 16) {
      src += 8;
      below[1] = vld1q_u16(src);
      if (width >= 32) {
        src += 8;
        below[2] = vld1q_u16(src);
        src += 8;
        below[3] = vld1q_u16(src);
        if (width == 64) {
          src += 8;
          below[4] = vld1q_u16(src);
          src += 8;
          below[5] = vld1q_u16(src);
          src += 8;
          below[6] = vld1q_u16(src);
          src += 8;
          below[7] = vld1q_u16(src);
        }
      }
    }
    src += src_remainder_stride;

    vst1q_u16(dst, vrhaddq_u16(row[0], below[0]));
    row[0] = below[0];
    if (width >= 16) {
      dst += 8;
      vst1q_u16(dst, vrhaddq_u16(row[1], below[1]));
      row[1] = below[1];
      if (width >= 32) {
        dst += 8;
        vst1q_u16(dst, vrhaddq_u16(row[2], below[2]));
        row[2] = below[2];
        dst += 8;
        vst1q_u16(dst, vrhaddq_u16(row[3], below[3]));
        row[3] = below[3];
        if (width >= 64) {
          dst += 8;
          vst1q_u16(dst, vrhaddq_u16(row[4], below[4]));
          row[4] = below[4];
          dst += 8;
          vst1q_u16(dst, vrhaddq_u16(row[5], below[5]));
          row[5] = below[5];
          dst += 8;
          vst1q_u16(dst, vrhaddq_u16(row[6], below[6]));
          row[6] = below[6];
          dst += 8;
          vst1q_u16(dst, vrhaddq_u16(row[7], below[7]));
          row[7] = below[7];
        }
      }
    }
    dst += dst_remainder_stride;
  } while (--y != 0);
}

void ConvolveIntraBlockCopyVertical_NEON(
    const void* LIBGAV1_RESTRICT const reference,
    const ptrdiff_t reference_stride, const int /*horizontal_filter_index*/,
    const int /*vertical_filter_index*/, const int /*horizontal_filter_id*/,
    const int /*vertical_filter_id*/, const int width, const int height,
    void* LIBGAV1_RESTRICT const prediction, const ptrdiff_t pred_stride) {
  assert(width >= 4 && width <= kMaxSuperBlockSizeInPixels);
  assert(height >= 4 && height <= kMaxSuperBlockSizeInPixels);
  const auto* src = static_cast<const uint16_t*>(reference);
  auto* dest = static_cast<uint16_t*>(prediction);
  const ptrdiff_t src_stride = reference_stride >> 1;
  const ptrdiff_t dst_stride = pred_stride >> 1;

  if (width == 128) {
    // Due to register pressure, process two 64xH.
    for (int i = 0; i < 2; ++i) {
      IntraBlockCopyVertical<64>(src, src_stride, height, dest, dst_stride);
      src += 64;
      dest += 64;
    }
  } else if (width == 64) {
    IntraBlockCopyVertical<64>(src, src_stride, height, dest, dst_stride);
  } else if (width == 32) {
    IntraBlockCopyVertical<32>(src, src_stride, height, dest, dst_stride);
  } else if (width == 16) {
    IntraBlockCopyVertical<16>(src, src_stride, height, dest, dst_stride);
  } else if (width == 8) {
    IntraBlockCopyVertical<8>(src, src_stride, height, dest, dst_stride);
  } else {  // width == 4
    uint16x4_t row = vld1_u16(src);
    src += src_stride;
    int y = height;
    do {
      const uint16x4_t below = vld1_u16(src);
      src += src_stride;
      vst1_u16(dest, vrhadd_u16(row, below));
      dest += dst_stride;
      row = below;
    } while (--y != 0);
  }
}

template <int width>
inline void IntraBlockCopy2D(const uint16_t* LIBGAV1_RESTRICT src,
                             const ptrdiff_t src_stride, const int height,
                             uint16_t* LIBGAV1_RESTRICT dst,
                             const ptrdiff_t dst_stride) {
  const ptrdiff_t src_remainder_stride = src_stride - (width - 8);
  const ptrdiff_t dst_remainder_stride = dst_stride - (width - 8);
  uint16x8_t row[16];
  row[0] = vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
  if (width >= 16) {
    src += 8;
    row[1] = vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
    if (width >= 32) {
      src += 8;
      row[2] = vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
      src += 8;
      row[3] = vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
      if (width >= 64) {
        src += 8;
        row[4] = vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
        src += 8;
        row[5] = vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
        src += 8;
        row[6] = vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
        src += 8;
        row[7] = vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
        if (width == 128) {
          src += 8;
          row[8] = vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
          src += 8;
          row[9] = vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
          src += 8;
          row[10] = vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
          src += 8;
          row[11] = vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
          src += 8;
          row[12] = vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
          src += 8;
          row[13] = vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
          src += 8;
          row[14] = vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
          src += 8;
          row[15] = vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
        }
      }
    }
  }
  src += src_remainder_stride;

  int y = height;
  do {
    const uint16x8_t below_0 = vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
    vst1q_u16(dst, vrshrq_n_u16(vaddq_u16(row[0], below_0), 2));
    row[0] = below_0;
    if (width >= 16) {
      src += 8;
      dst += 8;

      const uint16x8_t below_1 = vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
      vst1q_u16(dst, vrshrq_n_u16(vaddq_u16(row[1], below_1), 2));
      row[1] = below_1;
      if (width >= 32) {
        src += 8;
        dst += 8;

        const uint16x8_t below_2 =
            vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
        vst1q_u16(dst, vrshrq_n_u16(vaddq_u16(row[2], below_2), 2));
        row[2] = below_2;
        src += 8;
        dst += 8;

        const uint16x8_t below_3 =
            vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
        vst1q_u16(dst, vrshrq_n_u16(vaddq_u16(row[3], below_3), 2));
        row[3] = below_3;
        if (width >= 64) {
          src += 8;
          dst += 8;

          const uint16x8_t below_4 =
              vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
          vst1q_u16(dst, vrshrq_n_u16(vaddq_u16(row[4], below_4), 2));
          row[4] = below_4;
          src += 8;
          dst += 8;

          const uint16x8_t below_5 =
              vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
          vst1q_u16(dst, vrshrq_n_u16(vaddq_u16(row[5], below_5), 2));
          row[5] = below_5;
          src += 8;
          dst += 8;

          const uint16x8_t below_6 =
              vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
          vst1q_u16(dst, vrshrq_n_u16(vaddq_u16(row[6], below_6), 2));
          row[6] = below_6;
          src += 8;
          dst += 8;

          const uint16x8_t below_7 =
              vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
          vst1q_u16(dst, vrshrq_n_u16(vaddq_u16(row[7], below_7), 2));
          row[7] = below_7;
          if (width == 128) {
            src += 8;
            dst += 8;

            const uint16x8_t below_8 =
                vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
            vst1q_u16(dst, vrshrq_n_u16(vaddq_u16(row[8], below_8), 2));
            row[8] = below_8;
            src += 8;
            dst += 8;

            const uint16x8_t below_9 =
                vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
            vst1q_u16(dst, vrshrq_n_u16(vaddq_u16(row[9], below_9), 2));
            row[9] = below_9;
            src += 8;
            dst += 8;

            const uint16x8_t below_10 =
                vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
            vst1q_u16(dst, vrshrq_n_u16(vaddq_u16(row[10], below_10), 2));
            row[10] = below_10;
            src += 8;
            dst += 8;

            const uint16x8_t below_11 =
                vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
            vst1q_u16(dst, vrshrq_n_u16(vaddq_u16(row[11], below_11), 2));
            row[11] = below_11;
            src += 8;
            dst += 8;

            const uint16x8_t below_12 =
                vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
            vst1q_u16(dst, vrshrq_n_u16(vaddq_u16(row[12], below_12), 2));
            row[12] = below_12;
            src += 8;
            dst += 8;

            const uint16x8_t below_13 =
                vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
            vst1q_u16(dst, vrshrq_n_u16(vaddq_u16(row[13], below_13), 2));
            row[13] = below_13;
            src += 8;
            dst += 8;

            const uint16x8_t below_14 =
                vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
            vst1q_u16(dst, vrshrq_n_u16(vaddq_u16(row[14], below_14), 2));
            row[14] = below_14;
            src += 8;
            dst += 8;

            const uint16x8_t below_15 =
                vaddq_u16(vld1q_u16(src), vld1q_u16(src + 1));
            vst1q_u16(dst, vrshrq_n_u16(vaddq_u16(row[15], below_15), 2));
            row[15] = below_15;
          }
        }
      }
    }
    src += src_remainder_stride;
    dst += dst_remainder_stride;
  } while (--y != 0);
}

void ConvolveIntraBlockCopy2D_NEON(
    const void* LIBGAV1_RESTRICT const reference,
    const ptrdiff_t reference_stride, const int /*horizontal_filter_index*/,
    const int /*vertical_filter_index*/, const int /*horizontal_filter_id*/,
    const int /*vertical_filter_id*/, const int width, const int height,
    void* LIBGAV1_RESTRICT const prediction, const ptrdiff_t pred_stride) {
  assert(width >= 4 && width <= kMaxSuperBlockSizeInPixels);
  assert(height >= 4 && height <= kMaxSuperBlockSizeInPixels);
  const auto* src = static_cast<const uint16_t*>(reference);
  auto* dest = static_cast<uint16_t*>(prediction);
  const ptrdiff_t src_stride = reference_stride >> 1;
  const ptrdiff_t dst_stride = pred_stride >> 1;

  // Note: allow vertical access to height + 1. Because this function is only
  // for u/v plane of intra block copy, such access is guaranteed to be within
  // the prediction block.

  if (width == 128) {
    IntraBlockCopy2D<128>(src, src_stride, height, dest, dst_stride);
  } else if (width == 64) {
    IntraBlockCopy2D<64>(src, src_stride, height, dest, dst_stride);
  } else if (width == 32) {
    IntraBlockCopy2D<32>(src, src_stride, height, dest, dst_stride);
  } else if (width == 16) {
    IntraBlockCopy2D<16>(src, src_stride, height, dest, dst_stride);
  } else if (width == 8) {
    IntraBlockCopy2D<8>(src, src_stride, height, dest, dst_stride);
  } else {  // width == 4
    uint16x4_t row0 = vadd_u16(vld1_u16(src), vld1_u16(src + 1));
    src += src_stride;

    int y = height;
    do {
      const uint16x4_t row1 = vadd_u16(vld1_u16(src), vld1_u16(src + 1));
      src += src_stride;
      const uint16x4_t row2 = vadd_u16(vld1_u16(src), vld1_u16(src + 1));
      src += src_stride;
      const uint16x4_t result_01 = vrshr_n_u16(vadd_u16(row0, row1), 2);
      const uint16x4_t result_12 = vrshr_n_u16(vadd_u16(row1, row2), 2);
      vst1_u16(dest, result_01);
      dest += dst_stride;
      vst1_u16(dest, result_12);
      dest += dst_stride;
      row0 = row2;
      y -= 2;
    } while (y != 0);
  }
}

// -----------------------------------------------------------------------------
// Scaled Convolve

// There are many opportunities for overreading in scaled convolve, because the
// range of starting points for filter windows is anywhere from 0 to 16 for 8
// destination pixels, and the window sizes range from 2 to 8. To accommodate
// this range concisely, we use |grade_x| to mean the most steps in src that can
// be traversed in a single |step_x| increment, i.e. 1 or 2. When grade_x is 2,
// we are guaranteed to exceed 8 whole steps in src for every 8 |step_x|
// increments. The first load covers the initial elements of src_x, while the
// final load covers the taps.
template <int grade_x>
inline uint8x16x3_t LoadSrcVals(const uint16_t* const src_x) {
  uint8x16x3_t ret;
  // When fractional step size is less than or equal to 1, the rightmost
  // starting value for a filter may be at position 7. For an 8-tap filter, the
  // rightmost value for the final tap may be at position 14. Therefore we load
  // 2 vectors of eight 16-bit values.
  ret.val[0] = vreinterpretq_u8_u16(vld1q_u16(src_x));
  ret.val[1] = vreinterpretq_u8_u16(vld1q_u16(src_x + 8));
#if LIBGAV1_MSAN
  // Initialize to quiet msan warnings when grade_x <= 1.
  ret.val[2] = vdupq_n_u8(0);
#endif
  if (grade_x > 1) {
    // When fractional step size is greater than 1 (up to 2), the rightmost
    // starting value for a filter may be at position 15. For an 8-tap filter,
    // the rightmost value for the final tap may be at position 22. Therefore we
    // load 3 vectors of eight 16-bit values.
    ret.val[2] = vreinterpretq_u8_u16(vld1q_u16(src_x + 16));
  }
  return ret;
}

// Assemble 4 values corresponding to one tap position across multiple filters.
// This is a simple case because maximum offset is 8 and only smaller filters
// work on 4xH.
inline uint16x4_t PermuteSrcVals(const uint8x16x3_t src_bytes,
                                 const uint8x8_t indices) {
  const uint8x16x2_t src_bytes2 = {src_bytes.val[0], src_bytes.val[1]};
  return vreinterpret_u16_u8(VQTbl2U8(src_bytes2, indices));
}

// Assemble 8 values corresponding to one tap position across multiple filters.
// This requires a lot of workaround on A32 architectures, so it may be worth
// using an overall different algorithm for that architecture.
template <int grade_x>
inline uint16x8_t PermuteSrcVals(const uint8x16x3_t src_bytes,
                                 const uint8x16_t indices) {
  if (grade_x == 1) {
    const uint8x16x2_t src_bytes2 = {src_bytes.val[0], src_bytes.val[1]};
    return vreinterpretq_u16_u8(VQTbl2QU8(src_bytes2, indices));
  }
  return vreinterpretq_u16_u8(VQTbl3QU8(src_bytes, indices));
}

// Pre-transpose the 2 tap filters in |kAbsHalfSubPixelFilters|[3]
// Although the taps need to be converted to 16-bit values, they must be
// arranged by table lookup, which is more expensive for larger types than
// lengthening in-loop. |tap_index| refers to the index within a kernel applied
// to a single value.
inline int8x16_t GetPositive2TapFilter(const int tap_index) {
  assert(tap_index < 2);
  alignas(
      16) static constexpr int8_t kAbsHalfSubPixel2TapFilterColumns[2][16] = {
      {64, 60, 56, 52, 48, 44, 40, 36, 32, 28, 24, 20, 16, 12, 8, 4},
      {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60}};

  return vld1q_s8(kAbsHalfSubPixel2TapFilterColumns[tap_index]);
}

template <int grade_x>
inline void ConvolveKernelHorizontal2Tap(
    const uint16_t* LIBGAV1_RESTRICT const src, const ptrdiff_t src_stride,
    const int width, const int subpixel_x, const int step_x,
    const int intermediate_height, int16_t* LIBGAV1_RESTRICT intermediate) {
  // Account for the 0-taps that precede the 2 nonzero taps in the spec.
  const int kernel_offset = 3;
  const int ref_x = subpixel_x >> kScaleSubPixelBits;
  const int step_x8 = step_x << 3;
  const int8x16_t filter_taps0 = GetPositive2TapFilter(0);
  const int8x16_t filter_taps1 = GetPositive2TapFilter(1);
  const uint16x8_t index_steps = vmulq_n_u16(
      vmovl_u8(vcreate_u8(0x0706050403020100)), static_cast<uint16_t>(step_x));
  const uint8x8_t filter_index_mask = vdup_n_u8(kSubPixelMask);

  int p = subpixel_x;
  if (width <= 4) {
    const uint16_t* src_y = src;
    // Only add steps to the 10-bit truncated p to avoid overflow.
    const uint16x8_t p_fraction = vdupq_n_u16(p & 1023);
    const uint16x8_t subpel_index_offsets = vaddq_u16(index_steps, p_fraction);
    const uint8x8_t filter_indices =
        vand_u8(vshrn_n_u16(subpel_index_offsets, 6), filter_index_mask);
    // Each lane of lane of taps[k] corresponds to one output value along the
    // row, containing kSubPixelFilters[filter_index][filter_id][k], where
    // filter_id depends on x.
    const int16x4_t taps[2] = {
        vget_low_s16(vmovl_s8(VQTbl1S8(filter_taps0, filter_indices))),
        vget_low_s16(vmovl_s8(VQTbl1S8(filter_taps1, filter_indices)))};
    // Lower byte of Nth value is at position 2*N.
    // Narrowing shift is not available here because the maximum shift
    // parameter is 8.
    const uint8x8_t src_indices0 = vshl_n_u8(
        vmovn_u16(vshrq_n_u16(subpel_index_offsets, kScaleSubPixelBits)), 1);
    // Upper byte of Nth value is at position 2*N+1.
    const uint8x8_t src_indices1 = vadd_u8(src_indices0, vdup_n_u8(1));
    // Only 4 values needed.
    const uint8x8_t src_indices = InterleaveLow8(src_indices0, src_indices1);
    const uint8x8_t src_lookup[2] = {src_indices,
                                     vadd_u8(src_indices, vdup_n_u8(2))};

    int y = intermediate_height;
    do {
      const uint16_t* src_x =
          src_y + (p >> kScaleSubPixelBits) - ref_x + kernel_offset;
      // Load a pool of samples to select from using stepped indices.
      const uint8x16x3_t src_bytes = LoadSrcVals<1>(src_x);
      // Each lane corresponds to a different filter kernel.
      const uint16x4_t src[2] = {PermuteSrcVals(src_bytes, src_lookup[0]),
                                 PermuteSrcVals(src_bytes, src_lookup[1])};

      vst1_s16(intermediate,
               vrshrn_n_s32(SumOnePassTaps</*num_taps=*/2>(src, taps),
                            kInterRoundBitsHorizontal - 1));
      src_y = AddByteStride(src_y, src_stride);
      intermediate += kIntermediateStride;
    } while (--y != 0);
    return;
  }

  // |width| >= 8
  int16_t* intermediate_x = intermediate;
  int x = 0;
  do {
    const uint16_t* src_x =
        src + (p >> kScaleSubPixelBits) - ref_x + kernel_offset;
    // Only add steps to the 10-bit truncated p to avoid overflow.
    const uint16x8_t p_fraction = vdupq_n_u16(p & 1023);
    const uint16x8_t subpel_index_offsets = vaddq_u16(index_steps, p_fraction);
    const uint8x8_t filter_indices =
        vand_u8(vshrn_n_u16(subpel_index_offsets, kFilterIndexShift),
                filter_index_mask);
    // Each lane of lane of taps[k] corresponds to one output value along the
    // row, containing kSubPixelFilters[filter_index][filter_id][k], where
    // filter_id depends on x.
    const int16x8_t taps[2] = {
        vmovl_s8(VQTbl1S8(filter_taps0, filter_indices)),
        vmovl_s8(VQTbl1S8(filter_taps1, filter_indices))};
    const int16x4_t taps_low[2] = {vget_low_s16(taps[0]),
                                   vget_low_s16(taps[1])};
    const int16x4_t taps_high[2] = {vget_high_s16(taps[0]),
                                    vget_high_s16(taps[1])};
    // Lower byte of Nth value is at position 2*N.
    const uint8x8_t src_indices0 = vshl_n_u8(
        vmovn_u16(vshrq_n_u16(subpel_index_offsets, kScaleSubPixelBits)), 1);
    // Upper byte of Nth value is at position 2*N+1.
    const uint8x8_t src_indices1 = vadd_u8(src_indices0, vdup_n_u8(1));
    const uint8x8x2_t src_indices_zip = vzip_u8(src_indices0, src_indices1);
    const uint8x16_t src_indices =
        vcombine_u8(src_indices_zip.val[0], src_indices_zip.val[1]);
    const uint8x16_t src_lookup[2] = {src_indices,
                                      vaddq_u8(src_indices, vdupq_n_u8(2))};

    int y = intermediate_height;
    do {
      // Load a pool of samples to select from using stepped indices.
      const uint8x16x3_t src_bytes = LoadSrcVals<grade_x>(src_x);
      // Each lane corresponds to a different filter kernel.
      const uint16x8_t src[2] = {
          PermuteSrcVals<grade_x>(src_bytes, src_lookup[0]),
          PermuteSrcVals<grade_x>(src_bytes, src_lookup[1])};
      const uint16x4_t src_low[2] = {vget_low_u16(src[0]),
                                     vget_low_u16(src[1])};
      const uint16x4_t src_high[2] = {vget_high_u16(src[0]),
                                      vget_high_u16(src[1])};

      vst1_s16(intermediate_x,
               vrshrn_n_s32(SumOnePassTaps</*num_taps=*/2>(src_low, taps_low),
                            kInterRoundBitsHorizontal - 1));
      vst1_s16(intermediate_x + 4,
               vrshrn_n_s32(SumOnePassTaps</*num_taps=*/2>(src_high, taps_high),
                            kInterRoundBitsHorizontal - 1));
      // Avoid right shifting the stride.
      src_x = AddByteStride(src_x, src_stride);
      intermediate_x += kIntermediateStride;
    } while (--y != 0);
    x += 8;
    p += step_x8;
  } while (x < width);
}

// Pre-transpose the 4 tap filters in |kAbsHalfSubPixelFilters|[5].
inline int8x16_t GetPositive4TapFilter(const int tap_index) {
  assert(tap_index < 4);
  alignas(
      16) static constexpr int8_t kSubPixel4TapPositiveFilterColumns[4][16] = {
      {0, 15, 13, 11, 10, 9, 8, 7, 6, 6, 5, 4, 3, 2, 2, 1},
      {64, 31, 31, 31, 30, 29, 28, 27, 26, 24, 23, 22, 21, 20, 18, 17},
      {0, 17, 18, 20, 21, 22, 23, 24, 26, 27, 28, 29, 30, 31, 31, 31},
      {0, 1, 2, 2, 3, 4, 5, 6, 6, 7, 8, 9, 10, 11, 13, 15}};

  return vld1q_s8(kSubPixel4TapPositiveFilterColumns[tap_index]);
}

// This filter is only possible when width <= 4.
inline void ConvolveKernelHorizontalPositive4Tap(
    const uint16_t* LIBGAV1_RESTRICT const src, const ptrdiff_t src_stride,
    const int subpixel_x, const int step_x, const int intermediate_height,
    int16_t* LIBGAV1_RESTRICT intermediate) {
  // Account for the 0-taps that precede the 2 nonzero taps in the spec.
  const int kernel_offset = 2;
  const int ref_x = subpixel_x >> kScaleSubPixelBits;
  const int8x16_t filter_taps0 = GetPositive4TapFilter(0);
  const int8x16_t filter_taps1 = GetPositive4TapFilter(1);
  const int8x16_t filter_taps2 = GetPositive4TapFilter(2);
  const int8x16_t filter_taps3 = GetPositive4TapFilter(3);
  const uint16x8_t index_steps = vmulq_n_u16(
      vmovl_u8(vcreate_u8(0x0706050403020100)), static_cast<uint16_t>(step_x));
  const uint8x8_t filter_index_mask = vdup_n_u8(kSubPixelMask);

  int p = subpixel_x;
  // Only add steps to the 10-bit truncated p to avoid overflow.
  const uint16x8_t p_fraction = vdupq_n_u16(p & 1023);
  const uint16x8_t subpel_index_offsets = vaddq_u16(index_steps, p_fraction);
  const uint8x8_t filter_indices =
      vand_u8(vshrn_n_u16(subpel_index_offsets, 6), filter_index_mask);
  // Each lane of lane of taps[k] corresponds to one output value along the row,
  // containing kSubPixelFilters[filter_index][filter_id][k], where filter_id
  // depends on x.
  const int16x4_t taps[4] = {
      vget_low_s16(vmovl_s8(VQTbl1S8(filter_taps0, filter_indices))),
      vget_low_s16(vmovl_s8(VQTbl1S8(filter_taps1, filter_indices))),
      vget_low_s16(vmovl_s8(VQTbl1S8(filter_taps2, filter_indices))),
      vget_low_s16(vmovl_s8(VQTbl1S8(filter_taps3, filter_indices)))};
  // Lower byte of Nth value is at position 2*N.
  // Narrowing shift is not available here because the maximum shift
  // parameter is 8.
  const uint8x8_t src_indices0 = vshl_n_u8(
      vmovn_u16(vshrq_n_u16(subpel_index_offsets, kScaleSubPixelBits)), 1);
  // Upper byte of Nth value is at position 2*N+1.
  const uint8x8_t src_indices1 = vadd_u8(src_indices0, vdup_n_u8(1));
  // Only 4 values needed.
  const uint8x8_t src_indices_base = InterleaveLow8(src_indices0, src_indices1);

  uint8x8_t src_lookup[4];
  const uint8x8_t two = vdup_n_u8(2);
  src_lookup[0] = src_indices_base;
  for (int i = 1; i < 4; ++i) {
    src_lookup[i] = vadd_u8(src_lookup[i - 1], two);
  }

  const uint16_t* src_y =
      src + (p >> kScaleSubPixelBits) - ref_x + kernel_offset;
  int y = intermediate_height;
  do {
    // Load a pool of samples to select from using stepped indices.
    const uint8x16x3_t src_bytes = LoadSrcVals<1>(src_y);
    // Each lane corresponds to a different filter kernel.
    const uint16x4_t src[4] = {PermuteSrcVals(src_bytes, src_lookup[0]),
                               PermuteSrcVals(src_bytes, src_lookup[1]),
                               PermuteSrcVals(src_bytes, src_lookup[2]),
                               PermuteSrcVals(src_bytes, src_lookup[3])};

    vst1_s16(intermediate,
             vrshrn_n_s32(SumOnePassTaps</*num_taps=*/4>(src, taps),
                          kInterRoundBitsHorizontal - 1));
    src_y = AddByteStride(src_y, src_stride);
    intermediate += kIntermediateStride;
  } while (--y != 0);
}

// Pre-transpose the 4 tap filters in |kAbsHalfSubPixelFilters|[4].
inline int8x16_t GetSigned4TapFilter(const int tap_index) {
  assert(tap_index < 4);
  alignas(16) static constexpr int8_t
      kAbsHalfSubPixel4TapSignedFilterColumns[4][16] = {
          {-0, -2, -4, -5, -6, -6, -7, -6, -6, -5, -5, -5, -4, -3, -2, -1},
          {64, 63, 61, 58, 55, 51, 47, 42, 38, 33, 29, 24, 19, 14, 9, 4},
          {0, 4, 9, 14, 19, 24, 29, 33, 38, 42, 47, 51, 55, 58, 61, 63},
          {-0, -1, -2, -3, -4, -5, -5, -5, -6, -6, -7, -6, -6, -5, -4, -2}};

  return vld1q_s8(kAbsHalfSubPixel4TapSignedFilterColumns[tap_index]);
}

// This filter is only possible when width <= 4.
inline void ConvolveKernelHorizontalSigned4Tap(
    const uint16_t* LIBGAV1_RESTRICT const src, const ptrdiff_t src_stride,
    const int subpixel_x, const int step_x, const int intermediate_height,
    int16_t* LIBGAV1_RESTRICT intermediate) {
  const int kernel_offset = 2;
  const int ref_x = subpixel_x >> kScaleSubPixelBits;
  const uint8x8_t filter_index_mask = vdup_n_u8(kSubPixelMask);
  const int8x16_t filter_taps0 = GetSigned4TapFilter(0);
  const int8x16_t filter_taps1 = GetSigned4TapFilter(1);
  const int8x16_t filter_taps2 = GetSigned4TapFilter(2);
  const int8x16_t filter_taps3 = GetSigned4TapFilter(3);
  const uint16x8_t index_steps = vmulq_n_u16(
      vmovl_u8(vcreate_u8(0x0706050403020100)), static_cast<uint16_t>(step_x));

  const int p = subpixel_x;
  // Only add steps to the 10-bit truncated p to avoid overflow.
  const uint16x8_t p_fraction = vdupq_n_u16(p & 1023);
  const uint16x8_t subpel_index_offsets = vaddq_u16(index_steps, p_fraction);
  const uint8x8_t filter_indices =
      vand_u8(vshrn_n_u16(subpel_index_offsets, 6), filter_index_mask);
  // Each lane of lane of taps[k] corresponds to one output value along the row,
  // containing kSubPixelFilters[filter_index][filter_id][k], where filter_id
  // depends on x.
  const int16x4_t taps[4] = {
      vget_low_s16(vmovl_s8(VQTbl1S8(filter_taps0, filter_indices))),
      vget_low_s16(vmovl_s8(VQTbl1S8(filter_taps1, filter_indices))),
      vget_low_s16(vmovl_s8(VQTbl1S8(filter_taps2, filter_indices))),
      vget_low_s16(vmovl_s8(VQTbl1S8(filter_taps3, filter_indices)))};
  // Lower byte of Nth value is at position 2*N.
  // Narrowing shift is not available here because the maximum shift
  // parameter is 8.
  const uint8x8_t src_indices0 = vshl_n_u8(
      vmovn_u16(vshrq_n_u16(subpel_index_offsets, kScaleSubPixelBits)), 1);
  // Upper byte of Nth value is at position 2*N+1.
  const uint8x8_t src_indices1 = vadd_u8(src_indices0, vdup_n_u8(1));
  // Only 4 values needed.
  const uint8x8_t src_indices_base = InterleaveLow8(src_indices0, src_indices1);

  uint8x8_t src_lookup[4];
  const uint8x8_t two = vdup_n_u8(2);
  src_lookup[0] = src_indices_base;
  for (int i = 1; i < 4; ++i) {
    src_lookup[i] = vadd_u8(src_lookup[i - 1], two);
  }

  const uint16_t* src_y =
      src + (p >> kScaleSubPixelBits) - ref_x + kernel_offset;
  int y = intermediate_height;
  do {
    // Load a pool of samples to select from using stepped indices.
    const uint8x16x3_t src_bytes = LoadSrcVals<1>(src_y);
    // Each lane corresponds to a different filter kernel.
    const uint16x4_t src[4] = {PermuteSrcVals(src_bytes, src_lookup[0]),
                               PermuteSrcVals(src_bytes, src_lookup[1]),
                               PermuteSrcVals(src_bytes, src_lookup[2]),
                               PermuteSrcVals(src_bytes, src_lookup[3])};

    vst1_s16(intermediate,
             vrshrn_n_s32(SumOnePassTaps</*num_taps=*/4>(src, taps),
                          kInterRoundBitsHorizontal - 1));
    src_y = AddByteStride(src_y, src_stride);
    intermediate += kIntermediateStride;
  } while (--y != 0);
}

// Pre-transpose the 6 tap filters in |kAbsHalfSubPixelFilters|[0].
inline int8x16_t GetSigned6TapFilter(const int tap_index) {
  assert(tap_index < 6);
  alignas(16) static constexpr int8_t
      kAbsHalfSubPixel6TapSignedFilterColumns[6][16] = {
          {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
          {-0, -3, -5, -6, -7, -7, -8, -7, -7, -6, -6, -6, -5, -4, -2, -1},
          {64, 63, 61, 58, 55, 51, 47, 42, 38, 33, 29, 24, 19, 14, 9, 4},
          {0, 4, 9, 14, 19, 24, 29, 33, 38, 42, 47, 51, 55, 58, 61, 63},
          {-0, -1, -2, -4, -5, -6, -6, -6, -7, -7, -8, -7, -7, -6, -5, -3},
          {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

  return vld1q_s8(kAbsHalfSubPixel6TapSignedFilterColumns[tap_index]);
}

// This filter is only possible when width >= 8.
template <int grade_x>
inline void ConvolveKernelHorizontalSigned6Tap(
    const uint16_t* LIBGAV1_RESTRICT const src, const ptrdiff_t src_stride,
    const int width, const int subpixel_x, const int step_x,
    const int intermediate_height,
    int16_t* LIBGAV1_RESTRICT const intermediate) {
  const int kernel_offset = 1;
  const uint8x8_t filter_index_mask = vdup_n_u8(kSubPixelMask);
  const int ref_x = subpixel_x >> kScaleSubPixelBits;
  const int step_x8 = step_x << 3;
  int8x16_t filter_taps[6];
  for (int i = 0; i < 6; ++i) {
    filter_taps[i] = GetSigned6TapFilter(i);
  }
  const uint16x8_t index_steps = vmulq_n_u16(
      vmovl_u8(vcreate_u8(0x0706050403020100)), static_cast<uint16_t>(step_x));

  int16_t* intermediate_x = intermediate;
  int x = 0;
  int p = subpixel_x;
  do {
    const uint16_t* src_x =
        src + (p >> kScaleSubPixelBits) - ref_x + kernel_offset;
    // Only add steps to the 10-bit truncated p to avoid overflow.
    const uint16x8_t p_fraction = vdupq_n_u16(p & 1023);
    const uint16x8_t subpel_index_offsets = vaddq_u16(index_steps, p_fraction);
    const uint8x8_t filter_indices =
        vand_u8(vshrn_n_u16(subpel_index_offsets, kFilterIndexShift),
                filter_index_mask);

    // Each lane of lane of taps_(low|high)[k] corresponds to one output value
    // along the row, containing kSubPixelFilters[filter_index][filter_id][k],
    // where filter_id depends on x.
    int16x4_t taps_low[6];
    int16x4_t taps_high[6];
    for (int i = 0; i < 6; ++i) {
      const int16x8_t taps_i =
          vmovl_s8(VQTbl1S8(filter_taps[i], filter_indices));
      taps_low[i] = vget_low_s16(taps_i);
      taps_high[i] = vget_high_s16(taps_i);
    }

    // Lower byte of Nth value is at position 2*N.
    const uint8x8_t src_indices0 = vshl_n_u8(
        vmovn_u16(vshrq_n_u16(subpel_index_offsets, kScaleSubPixelBits)), 1);
    // Upper byte of Nth value is at position 2*N+1.
    const uint8x8_t src_indices1 = vadd_u8(src_indices0, vdup_n_u8(1));
    const uint8x8x2_t src_indices_zip = vzip_u8(src_indices0, src_indices1);
    const uint8x16_t src_indices_base =
        vcombine_u8(src_indices_zip.val[0], src_indices_zip.val[1]);

    uint8x16_t src_lookup[6];
    const uint8x16_t two = vdupq_n_u8(2);
    src_lookup[0] = src_indices_base;
    for (int i = 1; i < 6; ++i) {
      src_lookup[i] = vaddq_u8(src_lookup[i - 1], two);
    }

    int y = intermediate_height;
    do {
      // Load a pool of samples to select from using stepped indices.
      const uint8x16x3_t src_bytes = LoadSrcVals<grade_x>(src_x);

      uint16x4_t src_low[6];
      uint16x4_t src_high[6];
      for (int i = 0; i < 6; ++i) {
        const uint16x8_t src_i =
            PermuteSrcVals<grade_x>(src_bytes, src_lookup[i]);
        src_low[i] = vget_low_u16(src_i);
        src_high[i] = vget_high_u16(src_i);
      }

      vst1_s16(intermediate_x,
               vrshrn_n_s32(SumOnePassTaps</*num_taps=*/6>(src_low, taps_low),
                            kInterRoundBitsHorizontal - 1));
      vst1_s16(intermediate_x + 4,
               vrshrn_n_s32(SumOnePassTaps</*num_taps=*/6>(src_high, taps_high),
                            kInterRoundBitsHorizontal - 1));
      // Avoid right shifting the stride.
      src_x = AddByteStride(src_x, src_stride);
      intermediate_x += kIntermediateStride;
    } while (--y != 0);
    x += 8;
    p += step_x8;
  } while (x < width);
}

// Pre-transpose the 6 tap filters in |kAbsHalfSubPixelFilters|[1]. This filter
// has mixed positive and negative outer taps depending on the filter id.
inline int8x16_t GetMixed6TapFilter(const int tap_index) {
  assert(tap_index < 6);
  alignas(16) static constexpr int8_t
      kAbsHalfSubPixel6TapMixedFilterColumns[6][16] = {
          {0, 1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0},
          {0, 14, 13, 11, 10, 9, 8, 8, 7, 6, 5, 4, 3, 2, 2, 1},
          {64, 31, 31, 31, 30, 29, 28, 27, 26, 24, 23, 22, 21, 20, 18, 17},
          {0, 17, 18, 20, 21, 22, 23, 24, 26, 27, 28, 29, 30, 31, 31, 31},
          {0, 1, 2, 2, 3, 4, 5, 6, 7, 8, 8, 9, 10, 11, 13, 14},
          {0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 1}};

  return vld1q_s8(kAbsHalfSubPixel6TapMixedFilterColumns[tap_index]);
}

// This filter is only possible when width >= 8.
template <int grade_x>
inline void ConvolveKernelHorizontalMixed6Tap(
    const uint16_t* LIBGAV1_RESTRICT const src, const ptrdiff_t src_stride,
    const int width, const int subpixel_x, const int step_x,
    const int intermediate_height,
    int16_t* LIBGAV1_RESTRICT const intermediate) {
  const int kernel_offset = 1;
  const uint8x8_t filter_index_mask = vdup_n_u8(kSubPixelMask);
  const int ref_x = subpixel_x >> kScaleSubPixelBits;
  const int step_x8 = step_x << 3;
  int8x16_t filter_taps[6];
  for (int i = 0; i < 6; ++i) {
    filter_taps[i] = GetMixed6TapFilter(i);
  }
  const uint16x8_t index_steps = vmulq_n_u16(
      vmovl_u8(vcreate_u8(0x0706050403020100)), static_cast<uint16_t>(step_x));

  int16_t* intermediate_x = intermediate;
  int x = 0;
  int p = subpixel_x;
  do {
    const uint16_t* src_x =
        src + (p >> kScaleSubPixelBits) - ref_x + kernel_offset;
    // Only add steps to the 10-bit truncated p to avoid overflow.
    const uint16x8_t p_fraction = vdupq_n_u16(p & 1023);
    const uint16x8_t subpel_index_offsets = vaddq_u16(index_steps, p_fraction);

    const uint8x8_t filter_indices =
        vand_u8(vshrn_n_u16(subpel_index_offsets, kFilterIndexShift),
                filter_index_mask);
    // Each lane of lane of taps_(low|high)[k] corresponds to one output value
    // along the row, containing kSubPixelFilters[filter_index][filter_id][k],
    // where filter_id depends on x.
    int16x4_t taps_low[6];
    int16x4_t taps_high[6];
    for (int i = 0; i < 6; ++i) {
      const int16x8_t taps = vmovl_s8(VQTbl1S8(filter_taps[i], filter_indices));
      taps_low[i] = vget_low_s16(taps);
      taps_high[i] = vget_high_s16(taps);
    }

    // Lower byte of Nth value is at position 2*N.
    const uint8x8_t src_indices0 = vshl_n_u8(
        vmovn_u16(vshrq_n_u16(subpel_index_offsets, kScaleSubPixelBits)), 1);
    // Upper byte of Nth value is at position 2*N+1.
    const uint8x8_t src_indices1 = vadd_u8(src_indices0, vdup_n_u8(1));
    const uint8x8x2_t src_indices_zip = vzip_u8(src_indices0, src_indices1);
    const uint8x16_t src_indices_base =
        vcombine_u8(src_indices_zip.val[0], src_indices_zip.val[1]);

    uint8x16_t src_lookup[6];
    const uint8x16_t two = vdupq_n_u8(2);
    src_lookup[0] = src_indices_base;
    for (int i = 1; i < 6; ++i) {
      src_lookup[i] = vaddq_u8(src_lookup[i - 1], two);
    }

    int y = intermediate_height;
    do {
      // Load a pool of samples to select from using stepped indices.
      const uint8x16x3_t src_bytes = LoadSrcVals<grade_x>(src_x);

      uint16x4_t src_low[6];
      uint16x4_t src_high[6];
      for (int i = 0; i < 6; ++i) {
        const uint16x8_t src_i =
            PermuteSrcVals<grade_x>(src_bytes, src_lookup[i]);
        src_low[i] = vget_low_u16(src_i);
        src_high[i] = vget_high_u16(src_i);
      }

      vst1_s16(intermediate_x,
               vrshrn_n_s32(SumOnePassTaps</*num_taps=*/6>(src_low, taps_low),
                            kInterRoundBitsHorizontal - 1));
      vst1_s16(intermediate_x + 4,
               vrshrn_n_s32(SumOnePassTaps</*num_taps=*/6>(src_high, taps_high),
                            kInterRoundBitsHorizontal - 1));
      // Avoid right shifting the stride.
      src_x = AddByteStride(src_x, src_stride);
      intermediate_x += kIntermediateStride;
    } while (--y != 0);
    x += 8;
    p += step_x8;
  } while (x < width);
}

// Pre-transpose the 8 tap filters in |kAbsHalfSubPixelFilters|[2].
inline int8x16_t GetSigned8TapFilter(const int tap_index) {
  assert(tap_index < 8);
  alignas(16) static constexpr int8_t
      kAbsHalfSubPixel8TapSignedFilterColumns[8][16] = {
          {-0, -1, -1, -1, -2, -2, -2, -2, -2, -1, -1, -1, -1, -1, -1, -0},
          {0, 1, 3, 4, 5, 5, 5, 5, 6, 5, 4, 4, 3, 3, 2, 1},
          {-0, -3, -6, -9, -11, -11, -12, -12, -12, -11, -10, -9, -7, -5, -3,
           -1},
          {64, 63, 62, 60, 58, 54, 50, 45, 40, 35, 30, 24, 19, 13, 8, 4},
          {0, 4, 8, 13, 19, 24, 30, 35, 40, 45, 50, 54, 58, 60, 62, 63},
          {-0, -1, -3, -5, -7, -9, -10, -11, -12, -12, -12, -11, -11, -9, -6,
           -3},
          {0, 1, 2, 3, 3, 4, 4, 5, 6, 5, 5, 5, 5, 4, 3, 1},
          {-0, -0, -1, -1, -1, -1, -1, -1, -2, -2, -2, -2, -2, -1, -1, -1}};

  return vld1q_s8(kAbsHalfSubPixel8TapSignedFilterColumns[tap_index]);
}

// This filter is only possible when width >= 8.
template <int grade_x>
inline void ConvolveKernelHorizontalSigned8Tap(
    const uint16_t* LIBGAV1_RESTRICT const src, const ptrdiff_t src_stride,
    const int width, const int subpixel_x, const int step_x,
    const int intermediate_height,
    int16_t* LIBGAV1_RESTRICT const intermediate) {
  const uint8x8_t filter_index_mask = vdup_n_u8(kSubPixelMask);
  const int ref_x = subpixel_x >> kScaleSubPixelBits;
  const int step_x8 = step_x << 3;
  int8x16_t filter_taps[8];
  for (int i = 0; i < 8; ++i) {
    filter_taps[i] = GetSigned8TapFilter(i);
  }
  const uint16x8_t index_steps = vmulq_n_u16(
      vmovl_u8(vcreate_u8(0x0706050403020100)), static_cast<uint16_t>(step_x));
  int16_t* intermediate_x = intermediate;
  int x = 0;
  int p = subpixel_x;
  do {
    const uint16_t* src_x = src + (p >> kScaleSubPixelBits) - ref_x;
    // Only add steps to the 10-bit truncated p to avoid overflow.
    const uint16x8_t p_fraction = vdupq_n_u16(p & 1023);
    const uint16x8_t subpel_index_offsets = vaddq_u16(index_steps, p_fraction);

    const uint8x8_t filter_indices =
        vand_u8(vshrn_n_u16(subpel_index_offsets, kFilterIndexShift),
                filter_index_mask);

    // Lower byte of Nth value is at position 2*N.
    const uint8x8_t src_indices0 = vshl_n_u8(
        vmovn_u16(vshrq_n_u16(subpel_index_offsets, kScaleSubPixelBits)), 1);
    // Upper byte of Nth value is at position 2*N+1.
    const uint8x8_t src_indices1 = vadd_u8(src_indices0, vdup_n_u8(1));
    const uint8x8x2_t src_indices_zip = vzip_u8(src_indices0, src_indices1);
    const uint8x16_t src_indices_base =
        vcombine_u8(src_indices_zip.val[0], src_indices_zip.val[1]);

    uint8x16_t src_lookup[8];
    const uint8x16_t two = vdupq_n_u8(2);
    src_lookup[0] = src_indices_base;
    for (int i = 1; i < 8; ++i) {
      src_lookup[i] = vaddq_u8(src_lookup[i - 1], two);
    }
    // Each lane of lane of taps_(low|high)[k] corresponds to one output value
    // along the row, containing kSubPixelFilters[filter_index][filter_id][k],
    // where filter_id depends on x.
    int16x4_t taps_low[8];
    int16x4_t taps_high[8];
    for (int i = 0; i < 8; ++i) {
      const int16x8_t taps = vmovl_s8(VQTbl1S8(filter_taps[i], filter_indices));
      taps_low[i] = vget_low_s16(taps);
      taps_high[i] = vget_high_s16(taps);
    }

    int y = intermediate_height;
    do {
      // Load a pool of samples to select from using stepped indices.
      const uint8x16x3_t src_bytes = LoadSrcVals<grade_x>(src_x);

      uint16x4_t src_low[8];
      uint16x4_t src_high[8];
      for (int i = 0; i < 8; ++i) {
        const uint16x8_t src_i =
            PermuteSrcVals<grade_x>(src_bytes, src_lookup[i]);
        src_low[i] = vget_low_u16(src_i);
        src_high[i] = vget_high_u16(src_i);
      }

      vst1_s16(intermediate_x,
               vrshrn_n_s32(SumOnePassTaps</*num_taps=*/8>(src_low, taps_low),
                            kInterRoundBitsHorizontal - 1));
      vst1_s16(intermediate_x + 4,
               vrshrn_n_s32(SumOnePassTaps</*num_taps=*/8>(src_high, taps_high),
                            kInterRoundBitsHorizontal - 1));
      // Avoid right shifting the stride.
      src_x = AddByteStride(src_x, src_stride);
      intermediate_x += kIntermediateStride;
    } while (--y != 0);
    x += 8;
    p += step_x8;
  } while (x < width);
}

// Process 16 bit inputs and output 32 bits.
template <int num_taps, bool is_compound>
inline int16x4_t Sum2DVerticalTaps4(const int16x4_t* const src,
                                    const int16x8_t taps) {
  const int16x4_t taps_lo = vget_low_s16(taps);
  const int16x4_t taps_hi = vget_high_s16(taps);
  int32x4_t sum;
  if (num_taps == 8) {
    sum = vmull_lane_s16(src[0], taps_lo, 0);
    sum = vmlal_lane_s16(sum, src[1], taps_lo, 1);
    sum = vmlal_lane_s16(sum, src[2], taps_lo, 2);
    sum = vmlal_lane_s16(sum, src[3], taps_lo, 3);
    sum = vmlal_lane_s16(sum, src[4], taps_hi, 0);
    sum = vmlal_lane_s16(sum, src[5], taps_hi, 1);
    sum = vmlal_lane_s16(sum, src[6], taps_hi, 2);
    sum = vmlal_lane_s16(sum, src[7], taps_hi, 3);
  } else if (num_taps == 6) {
    sum = vmull_lane_s16(src[0], taps_lo, 1);
    sum = vmlal_lane_s16(sum, src[1], taps_lo, 2);
    sum = vmlal_lane_s16(sum, src[2], taps_lo, 3);
    sum = vmlal_lane_s16(sum, src[3], taps_hi, 0);
    sum = vmlal_lane_s16(sum, src[4], taps_hi, 1);
    sum = vmlal_lane_s16(sum, src[5], taps_hi, 2);
  } else if (num_taps == 4) {
    sum = vmull_lane_s16(src[0], taps_lo, 2);
    sum = vmlal_lane_s16(sum, src[1], taps_lo, 3);
    sum = vmlal_lane_s16(sum, src[2], taps_hi, 0);
    sum = vmlal_lane_s16(sum, src[3], taps_hi, 1);
  } else if (num_taps == 2) {
    sum = vmull_lane_s16(src[0], taps_lo, 3);
    sum = vmlal_lane_s16(sum, src[1], taps_hi, 0);
  }

  if (is_compound) {
    return vrshrn_n_s32(sum, kInterRoundBitsCompoundVertical - 1);
  }

  return vreinterpret_s16_u16(vqrshrun_n_s32(sum, kInterRoundBitsVertical - 1));
}

template <int num_taps, int grade_y, int width, bool is_compound>
void ConvolveVerticalScale2Or4xH(const int16_t* LIBGAV1_RESTRICT const src,
                                 const int subpixel_y, const int filter_index,
                                 const int step_y, const int height,
                                 void* LIBGAV1_RESTRICT const dest,
                                 const ptrdiff_t dest_stride) {
  static_assert(width == 2 || width == 4, "");
  // We increment stride with the 8-bit pointer and then reinterpret to avoid
  // shifting |dest_stride|.
  auto* dest_y = static_cast<uint16_t*>(dest);
  // In compound mode, |dest_stride| is based on the size of uint16_t, rather
  // than bytes.
  auto* compound_dest_y = static_cast<uint16_t*>(dest);
  // This stride always corresponds to int16_t.
  constexpr ptrdiff_t src_stride = kIntermediateStride;
  const int16_t* src_y = src;
  int16x4_t s[num_taps + grade_y];

  int p = subpixel_y & 1023;
  int prev_p = p;
  int y = height;
  do {
    for (int i = 0; i < num_taps; ++i) {
      s[i] = vld1_s16(src_y + i * src_stride);
    }
    int filter_id = (p >> 6) & kSubPixelMask;
    int16x8_t filter =
        vmovl_s8(vld1_s8(kHalfSubPixelFilters[filter_index][filter_id]));
    int16x4_t sums = Sum2DVerticalTaps4<num_taps, is_compound>(s, filter);
    if (is_compound) {
      assert(width != 2);
      // This offset potentially overflows into the sign bit, but should yield
      // the correct unsigned value.
      const uint16x4_t result =
          vreinterpret_u16_s16(vadd_s16(sums, vdup_n_s16(kCompoundOffset)));
      vst1_u16(compound_dest_y, result);
      compound_dest_y += dest_stride;
    } else {
      const uint16x4_t result = vmin_u16(vreinterpret_u16_s16(sums),
                                         vdup_n_u16((1 << kBitdepth10) - 1));
      if (width == 2) {
        Store2<0>(dest_y, result);
      } else {
        vst1_u16(dest_y, result);
      }
      dest_y = AddByteStride(dest_y, dest_stride);
    }
    p += step_y;
    const int p_diff =
        (p >> kScaleSubPixelBits) - (prev_p >> kScaleSubPixelBits);
    prev_p = p;
    // Here we load extra source in case it is needed. If |p_diff| == 0, these
    // values will be unused, but it's faster to load than to branch.
    s[num_taps] = vld1_s16(src_y + num_taps * src_stride);
    if (grade_y > 1) {
      s[num_taps + 1] = vld1_s16(src_y + (num_taps + 1) * src_stride);
    }

    filter_id = (p >> 6) & kSubPixelMask;
    filter = vmovl_s8(vld1_s8(kHalfSubPixelFilters[filter_index][filter_id]));
    sums = Sum2DVerticalTaps4<num_taps, is_compound>(&s[p_diff], filter);
    if (is_compound) {
      assert(width != 2);
      const uint16x4_t result =
          vreinterpret_u16_s16(vadd_s16(sums, vdup_n_s16(kCompoundOffset)));
      vst1_u16(compound_dest_y, result);
      compound_dest_y += dest_stride;
    } else {
      const uint16x4_t result = vmin_u16(vreinterpret_u16_s16(sums),
                                         vdup_n_u16((1 << kBitdepth10) - 1));
      if (width == 2) {
        Store2<0>(dest_y, result);
      } else {
        vst1_u16(dest_y, result);
      }
      dest_y = AddByteStride(dest_y, dest_stride);
    }
    p += step_y;
    src_y = src + (p >> kScaleSubPixelBits) * src_stride;
    prev_p = p;
    y -= 2;
  } while (y != 0);
}

template <int num_taps, int grade_y, bool is_compound>
void ConvolveVerticalScale(const int16_t* LIBGAV1_RESTRICT const source,
                           const int intermediate_height, const int width,
                           const int subpixel_y, const int filter_index,
                           const int step_y, const int height,
                           void* LIBGAV1_RESTRICT const dest,
                           const ptrdiff_t dest_stride) {
  // This stride always corresponds to int16_t.
  constexpr ptrdiff_t src_stride = kIntermediateStride;

  int16x8_t s[num_taps + 2];

  const int16_t* src = source;
  int x = 0;
  do {
    const int16_t* src_y = src;
    int p = subpixel_y & 1023;
    int prev_p = p;
    // We increment stride with the 8-bit pointer and then reinterpret to avoid
    // shifting |dest_stride|.
    auto* dest_y = static_cast<uint16_t*>(dest) + x;
    // In compound mode, |dest_stride| is based on the size of uint16_t, rather
    // than bytes.
    auto* compound_dest_y = static_cast<uint16_t*>(dest) + x;
    int y = height;
    do {
      for (int i = 0; i < num_taps; ++i) {
        s[i] = vld1q_s16(src_y + i * src_stride);
      }
      int filter_id = (p >> 6) & kSubPixelMask;
      int16x8_t filter =
          vmovl_s8(vld1_s8(kHalfSubPixelFilters[filter_index][filter_id]));
      int16x8_t sums =
          SimpleSum2DVerticalTaps<num_taps, is_compound>(s, filter);
      if (is_compound) {
        // This offset potentially overflows int16_t, but should yield the
        // correct unsigned value.
        const uint16x8_t result = vreinterpretq_u16_s16(
            vaddq_s16(sums, vdupq_n_s16(kCompoundOffset)));
        vst1q_u16(compound_dest_y, result);
        compound_dest_y += dest_stride;
      } else {
        const uint16x8_t result = vminq_u16(
            vreinterpretq_u16_s16(sums), vdupq_n_u16((1 << kBitdepth10) - 1));
        vst1q_u16(dest_y, result);
        dest_y = AddByteStride(dest_y, dest_stride);
      }
      p += step_y;
      const int p_diff =
          (p >> kScaleSubPixelBits) - (prev_p >> kScaleSubPixelBits);
      prev_p = p;
      // Here we load extra source in case it is needed. If |p_diff| == 0, these
      // values will be unused, but it's faster to load than to branch.
      s[num_taps] = vld1q_s16(src_y + num_taps * src_stride);
      if (grade_y > 1) {
        s[num_taps + 1] = vld1q_s16(src_y + (num_taps + 1) * src_stride);
      }

      filter_id = (p >> 6) & kSubPixelMask;
      filter = vmovl_s8(vld1_s8(kHalfSubPixelFilters[filter_index][filter_id]));
      sums = SimpleSum2DVerticalTaps<num_taps, is_compound>(&s[p_diff], filter);
      if (is_compound) {
        assert(width != 2);
        const uint16x8_t result = vreinterpretq_u16_s16(
            vaddq_s16(sums, vdupq_n_s16(kCompoundOffset)));
        vst1q_u16(compound_dest_y, result);
        compound_dest_y += dest_stride;
      } else {
        const uint16x8_t result = vminq_u16(
            vreinterpretq_u16_s16(sums), vdupq_n_u16((1 << kBitdepth10) - 1));
        vst1q_u16(dest_y, result);
        dest_y = AddByteStride(dest_y, dest_stride);
      }
      p += step_y;
      src_y = src + (p >> kScaleSubPixelBits) * src_stride;
      prev_p = p;

      y -= 2;
    } while (y != 0);
    src += kIntermediateStride * intermediate_height;
    x += 8;
  } while (x < width);
}

template <bool is_compound>
void ConvolveScale2D_NEON(const void* LIBGAV1_RESTRICT const reference,
                          const ptrdiff_t reference_stride,
                          const int horizontal_filter_index,
                          const int vertical_filter_index, const int subpixel_x,
                          const int subpixel_y, const int step_x,
                          const int step_y, const int width, const int height,
                          void* LIBGAV1_RESTRICT const prediction,
                          const ptrdiff_t pred_stride) {
  const int horiz_filter_index = GetFilterIndex(horizontal_filter_index, width);
  const int vert_filter_index = GetFilterIndex(vertical_filter_index, height);
  assert(step_x <= 2048);
  assert(step_y <= 2048);
  const int num_vert_taps = GetNumTapsInFilter(vert_filter_index);
  const int intermediate_height =
      (((height - 1) * step_y + (1 << kScaleSubPixelBits) - 1) >>
       kScaleSubPixelBits) +
      num_vert_taps;
  int16_t intermediate_result[kIntermediateAllocWidth *
                              (2 * kIntermediateAllocWidth + 8)];
#if LIBGAV1_MSAN
  // Quiet msan warnings. Set with random non-zero value to aid in debugging.
  memset(intermediate_result, 0x54, sizeof(intermediate_result));
#endif
  // Horizontal filter.
  // Filter types used for width <= 4 are different from those for width > 4.
  // When width > 4, the valid filter index range is always [0, 3].
  // When width <= 4, the valid filter index range is always [3, 5].
  // The same applies to height and vertical filter index.
  int filter_index = GetFilterIndex(horizontal_filter_index, width);
  int16_t* intermediate = intermediate_result;
  const ptrdiff_t src_stride = reference_stride;
  const auto* src = static_cast<const uint16_t*>(reference);
  const int vert_kernel_offset = (8 - num_vert_taps) / 2;
  src = AddByteStride(src, vert_kernel_offset * src_stride);

  // Derive the maximum value of |step_x| at which all source values fit in one
  // 16-byte (8-value) load. Final index is src_x + |num_taps| - 1 < 16
  // step_x*7 is the final base subpel index for the shuffle mask for filter
  // inputs in each iteration on large blocks. When step_x is large, we need a
  // larger structure and use a larger table lookup in order to gather all
  // filter inputs.
  const int num_horiz_taps = GetNumTapsInFilter(horiz_filter_index);
  // |num_taps| - 1 is the shuffle index of the final filter input.
  const int kernel_start_ceiling = 16 - num_horiz_taps;
  // This truncated quotient |grade_x_threshold| selects |step_x| such that:
  // (step_x * 7) >> kScaleSubPixelBits < single load limit
  const int grade_x_threshold =
      (kernel_start_ceiling << kScaleSubPixelBits) / 7;

  switch (filter_index) {
    case 0:
      if (step_x > grade_x_threshold) {
        ConvolveKernelHorizontalSigned6Tap<2>(
            src, src_stride, width, subpixel_x, step_x, intermediate_height,
            intermediate);
      } else {
        ConvolveKernelHorizontalSigned6Tap<1>(
            src, src_stride, width, subpixel_x, step_x, intermediate_height,
            intermediate);
      }
      break;
    case 1:
      if (step_x > grade_x_threshold) {
        ConvolveKernelHorizontalMixed6Tap<2>(src, src_stride, width, subpixel_x,
                                             step_x, intermediate_height,
                                             intermediate);

      } else {
        ConvolveKernelHorizontalMixed6Tap<1>(src, src_stride, width, subpixel_x,
                                             step_x, intermediate_height,
                                             intermediate);
      }
      break;
    case 2:
      if (step_x > grade_x_threshold) {
        ConvolveKernelHorizontalSigned8Tap<2>(
            src, src_stride, width, subpixel_x, step_x, intermediate_height,
            intermediate);
      } else {
        ConvolveKernelHorizontalSigned8Tap<1>(
            src, src_stride, width, subpixel_x, step_x, intermediate_height,
            intermediate);
      }
      break;
    case 3:
      if (step_x > grade_x_threshold) {
        ConvolveKernelHorizontal2Tap<2>(src, src_stride, width, subpixel_x,
                                        step_x, intermediate_height,
                                        intermediate);
      } else {
        ConvolveKernelHorizontal2Tap<1>(src, src_stride, width, subpixel_x,
                                        step_x, intermediate_height,
                                        intermediate);
      }
      break;
    case 4:
      assert(width <= 4);
      ConvolveKernelHorizontalSigned4Tap(src, src_stride, subpixel_x, step_x,
                                         intermediate_height, intermediate);
      break;
    default:
      assert(filter_index == 5);
      ConvolveKernelHorizontalPositive4Tap(src, src_stride, subpixel_x, step_x,
                                           intermediate_height, intermediate);
  }

  // Vertical filter.
  filter_index = GetFilterIndex(vertical_filter_index, height);
  intermediate = intermediate_result;
  switch (filter_index) {
    case 0:
    case 1:
      if (step_y <= 1024) {
        if (!is_compound && width == 2) {
          ConvolveVerticalScale2Or4xH<6, 1, 2, is_compound>(
              intermediate, subpixel_y, filter_index, step_y, height,
              prediction, pred_stride);
        } else if (width == 4) {
          ConvolveVerticalScale2Or4xH<6, 1, 4, is_compound>(
              intermediate, subpixel_y, filter_index, step_y, height,
              prediction, pred_stride);
        } else {
          ConvolveVerticalScale<6, 1, is_compound>(
              intermediate, intermediate_height, width, subpixel_y,
              filter_index, step_y, height, prediction, pred_stride);
        }
      } else {
        if (!is_compound && width == 2) {
          ConvolveVerticalScale2Or4xH<6, 2, 2, is_compound>(
              intermediate, subpixel_y, filter_index, step_y, height,
              prediction, pred_stride);
        } else if (width == 4) {
          ConvolveVerticalScale2Or4xH<6, 2, 4, is_compound>(
              intermediate, subpixel_y, filter_index, step_y, height,
              prediction, pred_stride);
        } else {
          ConvolveVerticalScale<6, 2, is_compound>(
              intermediate, intermediate_height, width, subpixel_y,
              filter_index, step_y, height, prediction, pred_stride);
        }
      }
      break;
    case 2:
      if (step_y <= 1024) {
        if (!is_compound && width == 2) {
          ConvolveVerticalScale2Or4xH<8, 1, 2, is_compound>(
              intermediate, subpixel_y, filter_index, step_y, height,
              prediction, pred_stride);
        } else if (width == 4) {
          ConvolveVerticalScale2Or4xH<8, 1, 4, is_compound>(
              intermediate, subpixel_y, filter_index, step_y, height,
              prediction, pred_stride);
        } else {
          ConvolveVerticalScale<8, 1, is_compound>(
              intermediate, intermediate_height, width, subpixel_y,
              filter_index, step_y, height, prediction, pred_stride);
        }
      } else {
        if (!is_compound && width == 2) {
          ConvolveVerticalScale2Or4xH<8, 2, 2, is_compound>(
              intermediate, subpixel_y, filter_index, step_y, height,
              prediction, pred_stride);
        } else if (width == 4) {
          ConvolveVerticalScale2Or4xH<8, 2, 4, is_compound>(
              intermediate, subpixel_y, filter_index, step_y, height,
              prediction, pred_stride);
        } else {
          ConvolveVerticalScale<8, 2, is_compound>(
              intermediate, intermediate_height, width, subpixel_y,
              filter_index, step_y, height, prediction, pred_stride);
        }
      }
      break;
    case 3:
      if (step_y <= 1024) {
        if (!is_compound && width == 2) {
          ConvolveVerticalScale2Or4xH<2, 1, 2, is_compound>(
              intermediate, subpixel_y, filter_index, step_y, height,
              prediction, pred_stride);
        } else if (width == 4) {
          ConvolveVerticalScale2Or4xH<2, 1, 4, is_compound>(
              intermediate, subpixel_y, filter_index, step_y, height,
              prediction, pred_stride);
        } else {
          ConvolveVerticalScale<2, 1, is_compound>(
              intermediate, intermediate_height, width, subpixel_y,
              filter_index, step_y, height, prediction, pred_stride);
        }
      } else {
        if (!is_compound && width == 2) {
          ConvolveVerticalScale2Or4xH<2, 2, 2, is_compound>(
              intermediate, subpixel_y, filter_index, step_y, height,
              prediction, pred_stride);
        } else if (width == 4) {
          ConvolveVerticalScale2Or4xH<2, 2, 4, is_compound>(
              intermediate, subpixel_y, filter_index, step_y, height,
              prediction, pred_stride);
        } else {
          ConvolveVerticalScale<2, 2, is_compound>(
              intermediate, intermediate_height, width, subpixel_y,
              filter_index, step_y, height, prediction, pred_stride);
        }
      }
      break;
    default:
      assert(filter_index == 4 || filter_index == 5);
      assert(height <= 4);
      if (step_y <= 1024) {
        if (!is_compound && width == 2) {
          ConvolveVerticalScale2Or4xH<4, 1, 2, is_compound>(
              intermediate, subpixel_y, filter_index, step_y, height,
              prediction, pred_stride);
        } else if (width == 4) {
          ConvolveVerticalScale2Or4xH<4, 1, 4, is_compound>(
              intermediate, subpixel_y, filter_index, step_y, height,
              prediction, pred_stride);
        } else {
          ConvolveVerticalScale<4, 1, is_compound>(
              intermediate, intermediate_height, width, subpixel_y,
              filter_index, step_y, height, prediction, pred_stride);
        }
      } else {
        if (!is_compound && width == 2) {
          ConvolveVerticalScale2Or4xH<4, 2, 2, is_compound>(
              intermediate, subpixel_y, filter_index, step_y, height,
              prediction, pred_stride);
        } else if (width == 4) {
          ConvolveVerticalScale2Or4xH<4, 2, 4, is_compound>(
              intermediate, subpixel_y, filter_index, step_y, height,
              prediction, pred_stride);
        } else {
          ConvolveVerticalScale<4, 2, is_compound>(
              intermediate, intermediate_height, width, subpixel_y,
              filter_index, step_y, height, prediction, pred_stride);
        }
      }
  }
}

void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth10);
  assert(dsp != nullptr);
  dsp->convolve[0][0][0][1] = ConvolveHorizontal_NEON;
  dsp->convolve[0][0][1][0] = ConvolveVertical_NEON;
  dsp->convolve[0][0][1][1] = Convolve2D_NEON;

  dsp->convolve[0][1][0][0] = ConvolveCompoundCopy_NEON;
  dsp->convolve[0][1][0][1] = ConvolveCompoundHorizontal_NEON;
  dsp->convolve[0][1][1][0] = ConvolveCompoundVertical_NEON;
  dsp->convolve[0][1][1][1] = ConvolveCompound2D_NEON;

  dsp->convolve[1][0][0][1] = ConvolveIntraBlockCopyHorizontal_NEON;
  dsp->convolve[1][0][1][0] = ConvolveIntraBlockCopyVertical_NEON;
  dsp->convolve[1][0][1][1] = ConvolveIntraBlockCopy2D_NEON;

  dsp->convolve_scale[0] = ConvolveScale2D_NEON<false>;
  dsp->convolve_scale[1] = ConvolveScale2D_NEON<true>;
}

}  // namespace

void ConvolveInit10bpp_NEON() { Init10bpp(); }

}  // namespace dsp
}  // namespace libgav1

#else   // !(LIBGAV1_ENABLE_NEON && LIBGAV1_MAX_BITDEPTH >= 10)

namespace libgav1 {
namespace dsp {

void ConvolveInit10bpp_NEON() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_ENABLE_NEON && LIBGAV1_MAX_BITDEPTH >= 10
