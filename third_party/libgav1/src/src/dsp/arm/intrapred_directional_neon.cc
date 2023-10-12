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

#include "src/dsp/intrapred_directional.h"
#include "src/utils/cpu.h"

#if LIBGAV1_ENABLE_NEON

#include <arm_neon.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "src/dsp/arm/common_neon.h"
#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/utils/common.h"
#include "src/utils/compiler_attributes.h"

namespace libgav1 {
namespace dsp {
namespace low_bitdepth {
namespace {

// Blend two values based on weights that sum to 32.
inline uint8x8_t WeightedBlend(const uint8x8_t a, const uint8x8_t b,
                               const uint8x8_t a_weight,
                               const uint8x8_t b_weight) {
  const uint16x8_t a_product = vmull_u8(a, a_weight);
  const uint16x8_t sum = vmlal_u8(a_product, b, b_weight);

  return vrshrn_n_u16(sum, 5 /*log2(32)*/);
}

// For vertical operations the weights are one constant value.
inline uint8x8_t WeightedBlend(const uint8x8_t a, const uint8x8_t b,
                               const uint8_t weight) {
  return WeightedBlend(a, b, vdup_n_u8(32 - weight), vdup_n_u8(weight));
}

// Fill |left| and |right| with the appropriate values for a given |base_step|.
inline void LoadStepwise(const uint8_t* LIBGAV1_RESTRICT const source,
                         const uint8x8_t left_step, const uint8x8_t right_step,
                         uint8x8_t* left, uint8x8_t* right) {
  const uint8x16_t mixed = vld1q_u8(source);
  *left = VQTbl1U8(mixed, left_step);
  *right = VQTbl1U8(mixed, right_step);
}

// Handle signed step arguments by ignoring the sign. Negative values are
// considered out of range and overwritten later.
inline void LoadStepwise(const uint8_t* LIBGAV1_RESTRICT const source,
                         const int8x8_t left_step, const int8x8_t right_step,
                         uint8x8_t* left, uint8x8_t* right) {
  LoadStepwise(source, vreinterpret_u8_s8(left_step),
               vreinterpret_u8_s8(right_step), left, right);
}

// Process 4 or 8 |width| by any |height|.
template <int width>
inline void DirectionalZone1_WxH(uint8_t* LIBGAV1_RESTRICT dst,
                                 const ptrdiff_t stride, const int height,
                                 const uint8_t* LIBGAV1_RESTRICT const top,
                                 const int xstep, const bool upsampled) {
  assert(width == 4 || width == 8);

  const int upsample_shift = static_cast<int>(upsampled);
  const int scale_bits = 6 - upsample_shift;

  const int max_base_x = (width + height - 1) << upsample_shift;
  const int8x8_t max_base = vdup_n_s8(max_base_x);
  const uint8x8_t top_max_base = vdup_n_u8(top[max_base_x]);

  const int8x8_t all = vcreate_s8(0x0706050403020100);
  const int8x8_t even = vcreate_s8(0x0e0c0a0806040200);
  const int8x8_t base_step = upsampled ? even : all;
  const int8x8_t right_step = vadd_s8(base_step, vdup_n_s8(1));

  int top_x = xstep;
  int y = 0;
  do {
    const int top_base_x = top_x >> scale_bits;

    if (top_base_x >= max_base_x) {
      for (int i = y; i < height; ++i) {
        memset(dst, top[max_base_x], 4 /* width */);
        dst += stride;
      }
      return;
    }

    const uint8_t shift = ((top_x << upsample_shift) & 0x3F) >> 1;

    // Zone2 uses negative values for xstep. Use signed values to compare
    // |top_base_x| to |max_base_x|.
    const int8x8_t base_v = vadd_s8(vdup_n_s8(top_base_x), base_step);

    const uint8x8_t max_base_mask = vclt_s8(base_v, max_base);

    // 4 wide subsamples the output. 8 wide subsamples the input.
    if (width == 4) {
      const uint8x8_t left_values = vld1_u8(top + top_base_x);
      const uint8x8_t right_values = RightShiftVector<8>(left_values);
      const uint8x8_t value = WeightedBlend(left_values, right_values, shift);

      // If |upsampled| is true then extract every other value for output.
      const uint8x8_t value_stepped =
          vtbl1_u8(value, vreinterpret_u8_s8(base_step));
      const uint8x8_t masked_value =
          vbsl_u8(max_base_mask, value_stepped, top_max_base);

      StoreLo4(dst, masked_value);
    } else /* width == 8 */ {
      uint8x8_t left_values, right_values;
      // WeightedBlend() steps up to Q registers. Downsample the input to avoid
      // doing extra calculations.
      LoadStepwise(top + top_base_x, base_step, right_step, &left_values,
                   &right_values);

      const uint8x8_t value = WeightedBlend(left_values, right_values, shift);
      const uint8x8_t masked_value =
          vbsl_u8(max_base_mask, value, top_max_base);

      vst1_u8(dst, masked_value);
    }
    dst += stride;
    top_x += xstep;
  } while (++y < height);
}

// Process a multiple of 8 |width| by any |height|. Processes horizontally
// before vertically in the hopes of being a little more cache friendly.
inline void DirectionalZone1_WxH(uint8_t* LIBGAV1_RESTRICT dst,
                                 const ptrdiff_t stride, const int width,
                                 const int height,
                                 const uint8_t* LIBGAV1_RESTRICT const top,
                                 const int xstep, const bool upsampled) {
  assert(width % 8 == 0);
  const int upsample_shift = static_cast<int>(upsampled);
  const int scale_bits = 6 - upsample_shift;

  const int max_base_x = (width + height - 1) << upsample_shift;
  const int8x8_t max_base = vdup_n_s8(max_base_x);
  const uint8x8_t top_max_base = vdup_n_u8(top[max_base_x]);

  const int8x8_t all = vcreate_s8(0x0706050403020100);
  const int8x8_t even = vcreate_s8(0x0e0c0a0806040200);
  const int8x8_t base_step = upsampled ? even : all;
  const int8x8_t right_step = vadd_s8(base_step, vdup_n_s8(1));
  const int8x8_t block_step = vdup_n_s8(8 << upsample_shift);

  int top_x = xstep;
  int y = 0;
  do {
    const int top_base_x = top_x >> scale_bits;

    if (top_base_x >= max_base_x) {
      for (int i = y; i < height; ++i) {
        memset(dst, top[max_base_x], 4 /* width */);
        dst += stride;
      }
      return;
    }

    const uint8_t shift = ((top_x << upsample_shift) & 0x3F) >> 1;

    // Zone2 uses negative values for xstep. Use signed values to compare
    // |top_base_x| to |max_base_x|.
    int8x8_t base_v = vadd_s8(vdup_n_s8(top_base_x), base_step);

    int x = 0;
    do {
      const uint8x8_t max_base_mask = vclt_s8(base_v, max_base);

      // Extract the input values based on |upsampled| here to avoid doing twice
      // as many calculations.
      uint8x8_t left_values, right_values;
      LoadStepwise(top + top_base_x + x, base_step, right_step, &left_values,
                   &right_values);

      const uint8x8_t value = WeightedBlend(left_values, right_values, shift);
      const uint8x8_t masked_value =
          vbsl_u8(max_base_mask, value, top_max_base);

      vst1_u8(dst + x, masked_value);

      base_v = vadd_s8(base_v, block_step);
      x += 8;
    } while (x < width);
    top_x += xstep;
    dst += stride;
  } while (++y < height);
}

void DirectionalIntraPredictorZone1_NEON(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row, const int width,
    const int height, const int xstep, const bool upsampled_top) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  auto* dst = static_cast<uint8_t*>(dest);

  assert(xstep > 0);

  const int upsample_shift = static_cast<int>(upsampled_top);

  const uint8x8_t all = vcreate_u8(0x0706050403020100);

  if (xstep == 64) {
    assert(!upsampled_top);
    const uint8_t* top_ptr = top + 1;
    int y = 0;
    do {
      memcpy(dst, top_ptr, width);
      memcpy(dst + stride, top_ptr + 1, width);
      memcpy(dst + 2 * stride, top_ptr + 2, width);
      memcpy(dst + 3 * stride, top_ptr + 3, width);
      dst += 4 * stride;
      top_ptr += 4;
      y += 4;
    } while (y < height);
  } else if (width == 4) {
    DirectionalZone1_WxH<4>(dst, stride, height, top, xstep, upsampled_top);
  } else if (xstep > 51) {
    // 7.11.2.10. Intra edge upsample selection process
    // if ( d <= 0 || d >= 40 ) useUpsample = 0
    // For |upsample_top| the delta is from vertical so |prediction_angle - 90|.
    // In |kDirectionalIntraPredictorDerivative[]| angles less than 51 will meet
    // this criteria. The |xstep| value for angle 51 happens to be 51 as well.
    // Shallower angles have greater xstep values.
    assert(!upsampled_top);
    const int max_base_x = ((width + height) - 1);
    const uint8x8_t max_base = vdup_n_u8(max_base_x);
    const uint8x8_t top_max_base = vdup_n_u8(top[max_base_x]);
    const uint8x8_t block_step = vdup_n_u8(8);

    int top_x = xstep;
    int y = 0;
    do {
      const int top_base_x = top_x >> 6;
      const uint8_t shift = ((top_x << upsample_shift) & 0x3F) >> 1;
      uint8x8_t base_v = vadd_u8(vdup_n_u8(top_base_x), all);
      int x = 0;
      // Only calculate a block of 8 when at least one of the output values is
      // within range. Otherwise it can read off the end of |top|.
      const int must_calculate_width =
          std::min(width, max_base_x - top_base_x + 7) & ~7;
      for (; x < must_calculate_width; x += 8) {
        const uint8x8_t max_base_mask = vclt_u8(base_v, max_base);

        // Since these |xstep| values can not be upsampled the load is
        // simplified.
        const uint8x8_t left_values = vld1_u8(top + top_base_x + x);
        const uint8x8_t right_values = vld1_u8(top + top_base_x + x + 1);
        const uint8x8_t value = WeightedBlend(left_values, right_values, shift);
        const uint8x8_t masked_value =
            vbsl_u8(max_base_mask, value, top_max_base);

        vst1_u8(dst + x, masked_value);
        base_v = vadd_u8(base_v, block_step);
      }
      memset(dst + x, top[max_base_x], width - x);
      dst += stride;
      top_x += xstep;
    } while (++y < height);
  } else {
    DirectionalZone1_WxH(dst, stride, width, height, top, xstep, upsampled_top);
  }
}

// Process 4 or 8 |width| by 4 or 8 |height|.
template <int width>
inline void DirectionalZone3_WxH(
    uint8_t* LIBGAV1_RESTRICT dest, const ptrdiff_t stride, const int height,
    const uint8_t* LIBGAV1_RESTRICT const left_column, const int base_left_y,
    const int ystep, const int upsample_shift) {
  assert(width == 4 || width == 8);
  assert(height == 4 || height == 8);
  const int scale_bits = 6 - upsample_shift;

  // Zone3 never runs out of left_column values.
  assert((width + height - 1) << upsample_shift >  // max_base_y
         ((ystep * width) >> scale_bits) +
             (/* base_step */ 1 << upsample_shift) *
                 (height - 1));  // left_base_y

  // Limited improvement for 8x8. ~20% faster for 64x64.
  const uint8x8_t all = vcreate_u8(0x0706050403020100);
  const uint8x8_t even = vcreate_u8(0x0e0c0a0806040200);
  const uint8x8_t base_step = upsample_shift ? even : all;
  const uint8x8_t right_step = vadd_u8(base_step, vdup_n_u8(1));

  uint8_t* dst = dest;
  uint8x8_t left_v[8], right_v[8], value_v[8];
  const uint8_t* const left = left_column;

  const int index_0 = base_left_y;
  LoadStepwise(left + (index_0 >> scale_bits), base_step, right_step,
               &left_v[0], &right_v[0]);
  value_v[0] = WeightedBlend(left_v[0], right_v[0],
                             ((index_0 << upsample_shift) & 0x3F) >> 1);

  const int index_1 = base_left_y + ystep;
  LoadStepwise(left + (index_1 >> scale_bits), base_step, right_step,
               &left_v[1], &right_v[1]);
  value_v[1] = WeightedBlend(left_v[1], right_v[1],
                             ((index_1 << upsample_shift) & 0x3F) >> 1);

  const int index_2 = base_left_y + ystep * 2;
  LoadStepwise(left + (index_2 >> scale_bits), base_step, right_step,
               &left_v[2], &right_v[2]);
  value_v[2] = WeightedBlend(left_v[2], right_v[2],
                             ((index_2 << upsample_shift) & 0x3F) >> 1);

  const int index_3 = base_left_y + ystep * 3;
  LoadStepwise(left + (index_3 >> scale_bits), base_step, right_step,
               &left_v[3], &right_v[3]);
  value_v[3] = WeightedBlend(left_v[3], right_v[3],
                             ((index_3 << upsample_shift) & 0x3F) >> 1);

  const int index_4 = base_left_y + ystep * 4;
  LoadStepwise(left + (index_4 >> scale_bits), base_step, right_step,
               &left_v[4], &right_v[4]);
  value_v[4] = WeightedBlend(left_v[4], right_v[4],
                             ((index_4 << upsample_shift) & 0x3F) >> 1);

  const int index_5 = base_left_y + ystep * 5;
  LoadStepwise(left + (index_5 >> scale_bits), base_step, right_step,
               &left_v[5], &right_v[5]);
  value_v[5] = WeightedBlend(left_v[5], right_v[5],
                             ((index_5 << upsample_shift) & 0x3F) >> 1);

  const int index_6 = base_left_y + ystep * 6;
  LoadStepwise(left + (index_6 >> scale_bits), base_step, right_step,
               &left_v[6], &right_v[6]);
  value_v[6] = WeightedBlend(left_v[6], right_v[6],
                             ((index_6 << upsample_shift) & 0x3F) >> 1);

  const int index_7 = base_left_y + ystep * 7;
  LoadStepwise(left + (index_7 >> scale_bits), base_step, right_step,
               &left_v[7], &right_v[7]);
  value_v[7] = WeightedBlend(left_v[7], right_v[7],
                             ((index_7 << upsample_shift) & 0x3F) >> 1);

  // 8x8 transpose.
  const uint8x16x2_t b0 = vtrnq_u8(vcombine_u8(value_v[0], value_v[4]),
                                   vcombine_u8(value_v[1], value_v[5]));
  const uint8x16x2_t b1 = vtrnq_u8(vcombine_u8(value_v[2], value_v[6]),
                                   vcombine_u8(value_v[3], value_v[7]));

  const uint16x8x2_t c0 = vtrnq_u16(vreinterpretq_u16_u8(b0.val[0]),
                                    vreinterpretq_u16_u8(b1.val[0]));
  const uint16x8x2_t c1 = vtrnq_u16(vreinterpretq_u16_u8(b0.val[1]),
                                    vreinterpretq_u16_u8(b1.val[1]));

  const uint32x4x2_t d0 = vuzpq_u32(vreinterpretq_u32_u16(c0.val[0]),
                                    vreinterpretq_u32_u16(c1.val[0]));
  const uint32x4x2_t d1 = vuzpq_u32(vreinterpretq_u32_u16(c0.val[1]),
                                    vreinterpretq_u32_u16(c1.val[1]));

  if (width == 4) {
    StoreLo4(dst, vreinterpret_u8_u32(vget_low_u32(d0.val[0])));
    dst += stride;
    StoreLo4(dst, vreinterpret_u8_u32(vget_high_u32(d0.val[0])));
    dst += stride;
    StoreLo4(dst, vreinterpret_u8_u32(vget_low_u32(d1.val[0])));
    dst += stride;
    StoreLo4(dst, vreinterpret_u8_u32(vget_high_u32(d1.val[0])));
    if (height == 4) return;
    dst += stride;
    StoreLo4(dst, vreinterpret_u8_u32(vget_low_u32(d0.val[1])));
    dst += stride;
    StoreLo4(dst, vreinterpret_u8_u32(vget_high_u32(d0.val[1])));
    dst += stride;
    StoreLo4(dst, vreinterpret_u8_u32(vget_low_u32(d1.val[1])));
    dst += stride;
    StoreLo4(dst, vreinterpret_u8_u32(vget_high_u32(d1.val[1])));
  } else {
    vst1_u8(dst, vreinterpret_u8_u32(vget_low_u32(d0.val[0])));
    dst += stride;
    vst1_u8(dst, vreinterpret_u8_u32(vget_high_u32(d0.val[0])));
    dst += stride;
    vst1_u8(dst, vreinterpret_u8_u32(vget_low_u32(d1.val[0])));
    dst += stride;
    vst1_u8(dst, vreinterpret_u8_u32(vget_high_u32(d1.val[0])));
    if (height == 4) return;
    dst += stride;
    vst1_u8(dst, vreinterpret_u8_u32(vget_low_u32(d0.val[1])));
    dst += stride;
    vst1_u8(dst, vreinterpret_u8_u32(vget_high_u32(d0.val[1])));
    dst += stride;
    vst1_u8(dst, vreinterpret_u8_u32(vget_low_u32(d1.val[1])));
    dst += stride;
    vst1_u8(dst, vreinterpret_u8_u32(vget_high_u32(d1.val[1])));
  }
}

// Because the source values "move backwards" as the row index increases, the
// indices derived from ystep are generally negative. This is accommodated by
// making sure the relative indices are within [-15, 0] when the function is
// called, and sliding them into the inclusive range [0, 15], relative to a
// lower base address.
constexpr int kPositiveIndexOffset = 15;

// Process 4 or 8 |width| by any |height|.
template <int width>
inline void DirectionalZone2FromLeftCol_WxH(
    uint8_t* LIBGAV1_RESTRICT dst, const ptrdiff_t stride, const int height,
    const uint8_t* LIBGAV1_RESTRICT const left_column, const int16x8_t left_y,
    const int upsample_shift) {
  assert(width == 4 || width == 8);

  // The shift argument must be a constant.
  int16x8_t offset_y, shift_upsampled = left_y;
  if (upsample_shift) {
    offset_y = vshrq_n_s16(left_y, 5);
    shift_upsampled = vshlq_n_s16(shift_upsampled, 1);
  } else {
    offset_y = vshrq_n_s16(left_y, 6);
  }

  // Select values to the left of the starting point.
  // The 15th element (and 16th) will be all the way at the end, to the right.
  // With a negative ystep everything else will be "left" of them.
  // This supports cumulative steps up to 15. We could support up to 16 by doing
  // separate loads for |left_values| and |right_values|. vtbl supports 2 Q
  // registers as input which would allow for cumulative offsets of 32.
  const int16x8_t sampler =
      vaddq_s16(offset_y, vdupq_n_s16(kPositiveIndexOffset));
  const uint8x8_t left_values = vqmovun_s16(sampler);
  const uint8x8_t right_values = vadd_u8(left_values, vdup_n_u8(1));

  const int16x8_t shift_masked = vandq_s16(shift_upsampled, vdupq_n_s16(0x3f));
  const uint8x8_t shift_mul = vreinterpret_u8_s8(vshrn_n_s16(shift_masked, 1));
  const uint8x8_t inv_shift_mul = vsub_u8(vdup_n_u8(32), shift_mul);

  int y = 0;
  do {
    uint8x8_t src_left, src_right;
    LoadStepwise(left_column - kPositiveIndexOffset + (y << upsample_shift),
                 left_values, right_values, &src_left, &src_right);
    const uint8x8_t val =
        WeightedBlend(src_left, src_right, inv_shift_mul, shift_mul);

    if (width == 4) {
      StoreLo4(dst, val);
    } else {
      vst1_u8(dst, val);
    }
    dst += stride;
  } while (++y < height);
}

// Process 4 or 8 |width| by any |height|.
template <int width>
inline void DirectionalZone1Blend_WxH(
    uint8_t* LIBGAV1_RESTRICT dest, const ptrdiff_t stride, const int height,
    const uint8_t* LIBGAV1_RESTRICT const top_row, int zone_bounds, int top_x,
    const int xstep, const int upsample_shift) {
  assert(width == 4 || width == 8);

  const int scale_bits_x = 6 - upsample_shift;

  const uint8x8_t all = vcreate_u8(0x0706050403020100);
  const uint8x8_t even = vcreate_u8(0x0e0c0a0806040200);
  const uint8x8_t base_step = upsample_shift ? even : all;
  const uint8x8_t right_step = vadd_u8(base_step, vdup_n_u8(1));

  int y = 0;
  do {
    const uint8_t* const src = top_row + (top_x >> scale_bits_x);
    uint8x8_t left, right;
    LoadStepwise(src, base_step, right_step, &left, &right);

    const uint8_t shift = ((top_x << upsample_shift) & 0x3f) >> 1;
    const uint8x8_t val = WeightedBlend(left, right, shift);

    uint8x8_t dst_blend = vld1_u8(dest);
    // |zone_bounds| values can be negative.
    uint8x8_t blend =
        vcge_s8(vreinterpret_s8_u8(all), vdup_n_s8((zone_bounds >> 6)));
    uint8x8_t output = vbsl_u8(blend, val, dst_blend);

    if (width == 4) {
      StoreLo4(dest, output);
    } else {
      vst1_u8(dest, output);
    }
    dest += stride;
    zone_bounds += xstep;
    top_x -= xstep;
  } while (++y < height);
}

//  7.11.2.4 (8) 90 < angle > 180
//  The strategy for these functions (4xH and 8+xH) is to know how many blocks
//  can be processed with just pixels from |top_ptr|, then handle mixed blocks,
//  then handle only blocks that take from |left_ptr|. Additionally, a fast
//  index-shuffle approach is used for pred values from |left_column| in
//  sections that permit it.
inline void DirectionalZone2_4xH(
    uint8_t* LIBGAV1_RESTRICT dst, const ptrdiff_t stride,
    const uint8_t* LIBGAV1_RESTRICT const top_row,
    const uint8_t* LIBGAV1_RESTRICT const left_column, const int height,
    const int xstep, const int ystep, const bool upsampled_top,
    const bool upsampled_left) {
  const int upsample_left_shift = static_cast<int>(upsampled_left);
  const int upsample_top_shift = static_cast<int>(upsampled_top);

  // Helper vector.
  const int16x8_t zero_to_seven = {0, 1, 2, 3, 4, 5, 6, 7};

  // Loop incrementers for moving by block (4xN). Vertical still steps by 8. If
  // it's only 4, it will be finished in the first iteration.
  const ptrdiff_t stride8 = stride << 3;
  const int xstep8 = xstep << 3;

  const int min_height = (height == 4) ? 4 : 8;

  // All columns from |min_top_only_x| to the right will only need |top_row| to
  // compute and can therefore call the Zone1 functions. This assumes |xstep| is
  // at least 3.
  assert(xstep >= 3);
  const int min_top_only_x = std::min((height * xstep) >> 6, /* width */ 4);

  // Offsets the original zone bound value to simplify x < (y+1)*xstep/64 -1
  int xstep_bounds_base = (xstep == 64) ? 0 : xstep - 1;

  const int left_base_increment = ystep >> 6;
  const int ystep_remainder = ystep & 0x3F;

  // If the 64 scaling is regarded as a decimal point, the first value of the
  // left_y vector omits the portion which is covered under the left_column
  // offset. The following values need the full ystep as a relative offset.
  const int16x8_t remainder = vdupq_n_s16(-ystep_remainder);
  const int16x8_t left_y = vmlaq_n_s16(remainder, zero_to_seven, -ystep);

  // This loop treats each set of 4 columns in 3 stages with y-value boundaries.
  // The first stage, before the first y-loop, covers blocks that are only
  // computed from the top row. The second stage, comprising two y-loops, covers
  // blocks that have a mixture of values computed from top or left. The final
  // stage covers blocks that are only computed from the left.
  if (min_top_only_x > 0) {
    // Round down to the nearest multiple of 8 (or 4, if height is 4).
    const int max_top_only_y =
        std::min((1 << 6) / xstep, height) & ~(min_height - 1);
    DirectionalZone1_WxH<4>(dst, stride, max_top_only_y, top_row, -xstep,
                            upsampled_top);

    if (max_top_only_y == height) return;

    int y = max_top_only_y;
    dst += stride * y;
    const int xstep_y = xstep * y;

    // All rows from |min_left_only_y| down for this set of columns only need
    // |left_column| to compute.
    const int min_left_only_y = std::min((4 << 6) / xstep, height);
    int xstep_bounds = xstep_bounds_base + xstep_y;
    int top_x = -xstep - xstep_y;

    // +8 increment is OK because if height is 4 this only goes once.
    for (; y < min_left_only_y;
         y += 8, dst += stride8, xstep_bounds += xstep8, top_x -= xstep8) {
      DirectionalZone2FromLeftCol_WxH<4>(
          dst, stride, min_height,
          left_column + ((y - left_base_increment) << upsample_left_shift),
          left_y, upsample_left_shift);

      DirectionalZone1Blend_WxH<4>(dst, stride, min_height, top_row,
                                   xstep_bounds, top_x, xstep,
                                   upsample_top_shift);
    }

    // Loop over y for left_only rows.
    const int16_t base_left_y = vgetq_lane_s16(left_y, 0);
    for (; y < height; y += 8, dst += stride8) {
      DirectionalZone3_WxH<4>(
          dst, stride, min_height,
          left_column + ((y - left_base_increment) << upsample_left_shift),
          base_left_y, -ystep, upsample_left_shift);
    }
  } else {
    DirectionalZone1_WxH<4>(dst, stride, height, top_row, -xstep,
                            upsampled_top);
  }
}

template <bool shuffle_left_column>
inline void DirectionalZone2_8xH(
    uint8_t* LIBGAV1_RESTRICT const dst, const ptrdiff_t stride,
    const uint8_t* LIBGAV1_RESTRICT const top_row,
    const uint8_t* LIBGAV1_RESTRICT const left_column, const int height,
    const int xstep, const int ystep, const int x, const int left_offset,
    const int xstep_bounds_base, const int16x8_t left_y,
    const bool upsampled_top, const bool upsampled_left) {
  const int upsample_left_shift = static_cast<int>(upsampled_left);
  const int upsample_top_shift = static_cast<int>(upsampled_top);

  // Loop incrementers for moving by block (8x8). This function handles blocks
  // with height 4 as well. They are calculated in one pass so these variables
  // do not get used.
  const ptrdiff_t stride8 = stride << 3;
  const int xstep8 = xstep << 3;

  // Cover 8x4 case.
  const int min_height = (height == 4) ? 4 : 8;

  // The first stage, before the first y-loop, covers blocks that are only
  // computed from the top row. The second stage, comprising two y-loops, covers
  // blocks that have a mixture of values computed from top or left. The final
  // stage covers blocks that are only computed from the left.
  uint8_t* dst_x = dst + x;
  // Round down to the nearest multiple of 8 (or 4, if height is 4).
  const int max_top_only_y =
      std::min((1 << 6) / xstep, height) & ~(min_height - 1);
  DirectionalZone1_WxH<8>(dst_x, stride, max_top_only_y,
                          top_row + (x << upsample_top_shift), -xstep,
                          upsampled_top);

  if (max_top_only_y == height) return;

  int y = max_top_only_y;
  dst_x += stride * y;
  const int xstep_y = xstep * y;

  // All rows from |min_left_only_y| down for this set of columns only need
  // |left_column| to compute. Round up to the nearest 8.
  const int min_left_only_y =
      Align(std::min(((x + 8) << 6) / xstep, height), 8);
  int xstep_bounds = xstep_bounds_base + xstep_y;
  int top_x = -xstep - xstep_y;

  const int16_t base_left_y = vgetq_lane_s16(left_y, 0);
  for (; y < min_left_only_y;
       y += 8, dst_x += stride8, xstep_bounds += xstep8, top_x -= xstep8) {
    if (shuffle_left_column) {
      DirectionalZone2FromLeftCol_WxH<8>(
          dst_x, stride, min_height,
          left_column + ((left_offset + y) << upsample_left_shift), left_y,
          upsample_left_shift);
    } else {
      DirectionalZone3_WxH<8>(
          dst_x, stride, min_height,
          left_column + ((left_offset + y) << upsample_left_shift), base_left_y,
          -ystep, upsample_left_shift);
    }

    DirectionalZone1Blend_WxH<8>(
        dst_x, stride, min_height, top_row + (x << upsample_top_shift),
        xstep_bounds, top_x, xstep, upsample_top_shift);
  }

  // Loop over y for left_only rows.
  for (; y < height; y += 8, dst_x += stride8) {
    DirectionalZone3_WxH<8>(
        dst_x, stride, min_height,
        left_column + ((left_offset + y) << upsample_left_shift), base_left_y,
        -ystep, upsample_left_shift);
  }
}

// Process a multiple of 8 |width|.
inline void DirectionalZone2_WxH(
    uint8_t* LIBGAV1_RESTRICT const dst, const ptrdiff_t stride,
    const uint8_t* LIBGAV1_RESTRICT const top_row,
    const uint8_t* LIBGAV1_RESTRICT const left_column, const int width,
    const int height, const int xstep, const int ystep,
    const bool upsampled_top, const bool upsampled_left) {
  const int ystep8 = ystep << 3;

  // Offsets the original zone bound value to simplify x < (y+1)*xstep/64 -1
  int xstep_bounds_base = (xstep == 64) ? 0 : xstep - 1;

  const int left_base_increment = ystep >> 6;
  const int ystep_remainder = ystep & 0x3F;

  const int left_base_increment8 = ystep8 >> 6;
  const int ystep_remainder8 = ystep8 & 0x3F;
  const int16x8_t increment_left8 = vdupq_n_s16(ystep_remainder8);

  // If the 64 scaling is regarded as a decimal point, the first value of the
  // left_y vector omits the portion which is covered under the left_column
  // offset. Following values need the full ystep as a relative offset.
  const int16x8_t remainder = vdupq_n_s16(-ystep_remainder);
  const int16x8_t zero_to_seven = {0, 1, 2, 3, 4, 5, 6, 7};
  int16x8_t left_y = vmlaq_n_s16(remainder, zero_to_seven, -ystep);

  // For ystep > 90, at least two sets of 8 columns can be fully computed from
  // top_row only.
  const int min_top_only_x = std::min((height * xstep) >> 6, width);
  // Analysis finds that, for most angles (ystep < 132), all segments that use
  // both top_row and left_column can compute from left_column using byte
  // shuffles from a single vector. For steeper angles, the shuffle is also
  // fully reliable when x >= 32.
  const int shuffle_left_col_x = (ystep < 132) ? 0 : 32;
  const int min_shuffle_x = std::min(min_top_only_x, shuffle_left_col_x);

  // This loop treats each set of 4 columns in 3 stages with y-value boundaries.
  // The first stage, before the first y-loop, covers blocks that are only
  // computed from the top row. The second stage, comprising two y-loops, covers
  // blocks that have a mixture of values computed from top or left. The final
  // stage covers blocks that are only computed from the left.
  int x = 0;
  for (int left_offset = -left_base_increment; x < min_shuffle_x; x += 8,
           xstep_bounds_base -= (8 << 6),
           left_y = vsubq_s16(left_y, increment_left8),
           left_offset -= left_base_increment8) {
    DirectionalZone2_8xH<false>(dst, stride, top_row, left_column, height,
                                xstep, ystep, x, left_offset, xstep_bounds_base,
                                left_y, upsampled_top, upsampled_left);
  }
  for (int left_offset = -left_base_increment; x < min_top_only_x; x += 8,
           xstep_bounds_base -= (8 << 6),
           left_y = vsubq_s16(left_y, increment_left8),
           left_offset -= left_base_increment8) {
    DirectionalZone2_8xH<true>(dst, stride, top_row, left_column, height, xstep,
                               ystep, x, left_offset, xstep_bounds_base, left_y,
                               upsampled_top, upsampled_left);
  }
  if (x < width) {
    const int upsample_top_shift = static_cast<int>(upsampled_top);
    DirectionalZone1_WxH(dst + x, stride, width - x, height,
                         top_row + (x << upsample_top_shift), -xstep,
                         upsampled_top);
  }
}

void DirectionalIntraPredictorZone2_NEON(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column, const int width,
    const int height, const int xstep, const int ystep,
    const bool upsampled_top, const bool upsampled_left) {
  // Increasing the negative buffer for this function allows more rows to be
  // processed at a time without branching in an inner loop to check the base.
  uint8_t top_buffer[288];
  uint8_t left_buffer[288];
#if LIBGAV1_MSAN
  memset(top_buffer, 0, sizeof(top_buffer));
  memset(left_buffer, 0, sizeof(left_buffer));
#endif  // LIBGAV1_MSAN

  memcpy(top_buffer + 128, static_cast<const uint8_t*>(top_row) - 16, 160);
  memcpy(left_buffer + 128, static_cast<const uint8_t*>(left_column) - 16, 160);
  const uint8_t* top_ptr = top_buffer + 144;
  const uint8_t* left_ptr = left_buffer + 144;
  auto* dst = static_cast<uint8_t*>(dest);

  if (width == 4) {
    DirectionalZone2_4xH(dst, stride, top_ptr, left_ptr, height, xstep, ystep,
                         upsampled_top, upsampled_left);
  } else {
    DirectionalZone2_WxH(dst, stride, top_ptr, left_ptr, width, height, xstep,
                         ystep, upsampled_top, upsampled_left);
  }
}

void DirectionalIntraPredictorZone3_NEON(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const left_column, const int width,
    const int height, const int ystep, const bool upsampled_left) {
  const auto* const left = static_cast<const uint8_t*>(left_column);

  assert(ystep > 0);

  const int upsample_shift = static_cast<int>(upsampled_left);
  const int scale_bits = 6 - upsample_shift;
  const int base_step = 1 << upsample_shift;

  if (width == 4 || height == 4) {
    // This block can handle all sizes but the specializations for other sizes
    // are faster.
    const uint8x8_t all = vcreate_u8(0x0706050403020100);
    const uint8x8_t even = vcreate_u8(0x0e0c0a0806040200);
    const uint8x8_t base_step_v = upsampled_left ? even : all;
    const uint8x8_t right_step = vadd_u8(base_step_v, vdup_n_u8(1));

    int y = 0;
    do {
      int x = 0;
      do {
        auto* dst = static_cast<uint8_t*>(dest);
        dst += y * stride + x;
        uint8x8_t left_v[4], right_v[4], value_v[4];
        const int ystep_base = ystep * x;
        const int offset = y * base_step;

        const int index_0 = ystep_base + ystep * 1;
        LoadStepwise(left + offset + (index_0 >> scale_bits), base_step_v,
                     right_step, &left_v[0], &right_v[0]);
        value_v[0] = WeightedBlend(left_v[0], right_v[0],
                                   ((index_0 << upsample_shift) & 0x3F) >> 1);

        const int index_1 = ystep_base + ystep * 2;
        LoadStepwise(left + offset + (index_1 >> scale_bits), base_step_v,
                     right_step, &left_v[1], &right_v[1]);
        value_v[1] = WeightedBlend(left_v[1], right_v[1],
                                   ((index_1 << upsample_shift) & 0x3F) >> 1);

        const int index_2 = ystep_base + ystep * 3;
        LoadStepwise(left + offset + (index_2 >> scale_bits), base_step_v,
                     right_step, &left_v[2], &right_v[2]);
        value_v[2] = WeightedBlend(left_v[2], right_v[2],
                                   ((index_2 << upsample_shift) & 0x3F) >> 1);

        const int index_3 = ystep_base + ystep * 4;
        LoadStepwise(left + offset + (index_3 >> scale_bits), base_step_v,
                     right_step, &left_v[3], &right_v[3]);
        value_v[3] = WeightedBlend(left_v[3], right_v[3],
                                   ((index_3 << upsample_shift) & 0x3F) >> 1);

        // 8x4 transpose.
        const uint8x8x2_t b0 = vtrn_u8(value_v[0], value_v[1]);
        const uint8x8x2_t b1 = vtrn_u8(value_v[2], value_v[3]);

        const uint16x4x2_t c0 = vtrn_u16(vreinterpret_u16_u8(b0.val[0]),
                                         vreinterpret_u16_u8(b1.val[0]));
        const uint16x4x2_t c1 = vtrn_u16(vreinterpret_u16_u8(b0.val[1]),
                                         vreinterpret_u16_u8(b1.val[1]));

        StoreLo4(dst, vreinterpret_u8_u16(c0.val[0]));
        dst += stride;
        StoreLo4(dst, vreinterpret_u8_u16(c1.val[0]));
        dst += stride;
        StoreLo4(dst, vreinterpret_u8_u16(c0.val[1]));
        dst += stride;
        StoreLo4(dst, vreinterpret_u8_u16(c1.val[1]));

        if (height > 4) {
          dst += stride;
          StoreHi4(dst, vreinterpret_u8_u16(c0.val[0]));
          dst += stride;
          StoreHi4(dst, vreinterpret_u8_u16(c1.val[0]));
          dst += stride;
          StoreHi4(dst, vreinterpret_u8_u16(c0.val[1]));
          dst += stride;
          StoreHi4(dst, vreinterpret_u8_u16(c1.val[1]));
        }
        x += 4;
      } while (x < width);
      y += 8;
    } while (y < height);
  } else {  // 8x8 at a time.
    // Limited improvement for 8x8. ~20% faster for 64x64.
    int y = 0;
    do {
      int x = 0;
      do {
        auto* dst = static_cast<uint8_t*>(dest);
        dst += y * stride + x;
        const int ystep_base = ystep * (x + 1);

        DirectionalZone3_WxH<8>(dst, stride, 8, left + (y << upsample_shift),
                                ystep_base, ystep, upsample_shift);
        x += 8;
      } while (x < width);
      y += 8;
    } while (y < height);
  }
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
  dsp->directional_intra_predictor_zone1 = DirectionalIntraPredictorZone1_NEON;
  dsp->directional_intra_predictor_zone2 = DirectionalIntraPredictorZone2_NEON;
  dsp->directional_intra_predictor_zone3 = DirectionalIntraPredictorZone3_NEON;
}

}  // namespace
}  // namespace low_bitdepth

#if LIBGAV1_MAX_BITDEPTH >= 10
namespace high_bitdepth {
namespace {

// Blend two values based on weights that sum to 32.
inline uint16x4_t WeightedBlend(const uint16x4_t a, const uint16x4_t b,
                                const int a_weight, const int b_weight) {
  const uint16x4_t a_product = vmul_n_u16(a, a_weight);
  const uint16x4_t sum = vmla_n_u16(a_product, b, b_weight);

  return vrshr_n_u16(sum, 5 /*log2(32)*/);
}

// Blend two values based on weights that sum to 32.
inline uint16x8_t WeightedBlend(const uint16x8_t a, const uint16x8_t b,
                                const uint16_t a_weight,
                                const uint16_t b_weight) {
  const uint16x8_t a_product = vmulq_n_u16(a, a_weight);
  const uint16x8_t sum = vmlaq_n_u16(a_product, b, b_weight);

  return vrshrq_n_u16(sum, 5 /*log2(32)*/);
}

// Blend two values based on weights that sum to 32.
inline uint16x8_t WeightedBlend(const uint16x8_t a, const uint16x8_t b,
                                const uint16x8_t a_weight,
                                const uint16x8_t b_weight) {
  const uint16x8_t a_product = vmulq_u16(a, a_weight);
  const uint16x8_t sum = vmlaq_u16(a_product, b, b_weight);

  return vrshrq_n_u16(sum, 5 /*log2(32)*/);
}

// Each element of |dest| contains values associated with one weight value.
inline void LoadEdgeVals(uint16x4x2_t* dest,
                         const uint16_t* LIBGAV1_RESTRICT const source,
                         const bool upsampled) {
  if (upsampled) {
    *dest = vld2_u16(source);
  } else {
    dest->val[0] = vld1_u16(source);
    dest->val[1] = vld1_u16(source + 1);
  }
}

// Each element of |dest| contains values associated with one weight value.
inline void LoadEdgeVals(uint16x8x2_t* dest,
                         const uint16_t* LIBGAV1_RESTRICT const source,
                         const bool upsampled) {
  if (upsampled) {
    *dest = vld2q_u16(source);
  } else {
    dest->val[0] = vld1q_u16(source);
    dest->val[1] = vld1q_u16(source + 1);
  }
}

// For Wx4 blocks, load the source for 2 columns. The source for the second
// column is held in the high half of each vector.
inline void LoadEdgeVals2x4(uint16x8x2_t* dest,
                            const uint16_t* LIBGAV1_RESTRICT const source_low,
                            const uint16_t* LIBGAV1_RESTRICT const source_high,
                            const bool upsampled) {
  if (upsampled) {
    const uint16x4x2_t low = vld2_u16(source_low);
    const uint16x4x2_t high = vld2_u16(source_high);
    dest->val[0] = vcombine_u16(low.val[0], high.val[0]);
    dest->val[1] = vcombine_u16(low.val[1], high.val[1]);
  } else {
    dest->val[0] = vcombine_u16(vld1_u16(source_low), vld1_u16(source_high));
    dest->val[1] =
        vcombine_u16(vld1_u16(source_low + 1), vld1_u16(source_high + 1));
  }
}

template <bool upsampled>
inline void DirectionalZone1_4xH(uint16_t* LIBGAV1_RESTRICT dst,
                                 const ptrdiff_t stride, const int height,
                                 const uint16_t* LIBGAV1_RESTRICT const top,
                                 const int xstep) {
  const int upsample_shift = static_cast<int>(upsampled);
  const int index_scale_bits = 6 - upsample_shift;

  const int max_base_x = (4 + height - 1) << upsample_shift;
  const int16x4_t max_base = vdup_n_s16(max_base_x);
  const uint16x4_t final_top_val = vdup_n_u16(top[max_base_x]);
  const int16x4_t index_offset = {0, 1, 2, 3};

  // All rows from |min_corner_only_y| down will simply use Memset.
  // |max_base_x| is always greater than |height|, so clipping the denominator
  // to 1 is enough to make the logic work.
  const int xstep_units = std::max(xstep >> index_scale_bits, 1);
  const int min_corner_only_y = std::min(max_base_x / xstep_units, height);

  int top_x = xstep;
  int y = 0;
  for (; y < min_corner_only_y; ++y, dst += stride, top_x += xstep) {
    const int top_base_x = top_x >> index_scale_bits;

    // To accommodate reuse of this function in Zone2, permit negative values
    // for |xstep|.
    const uint16_t shift_0 = (LeftShift(top_x, upsample_shift) & 0x3F) >> 1;
    const uint16_t shift_1 = 32 - shift_0;

    // Use signed values to compare |top_base_x| to |max_base_x|.
    const int16x4_t base_x = vadd_s16(vdup_n_s16(top_base_x), index_offset);
    const uint16x4_t max_base_mask = vclt_s16(base_x, max_base);

    uint16x4x2_t sampled_top_row;
    LoadEdgeVals(&sampled_top_row, top + top_base_x, upsampled);
    const uint16x4_t combined = WeightedBlend(
        sampled_top_row.val[0], sampled_top_row.val[1], shift_1, shift_0);

    // If |upsampled| is true then extract every other value for output.
    const uint16x4_t masked_result =
        vbsl_u16(max_base_mask, combined, final_top_val);

    vst1_u16(dst, masked_result);
  }
  for (; y < height; ++y) {
    Memset(dst, top[max_base_x], 4 /* width */);
    dst += stride;
  }
}

// Process a multiple of 8 |width| by any |height|. Processes horizontally
// before vertically in the hopes of being a little more cache friendly.
template <bool upsampled>
inline void DirectionalZone1_WxH(uint16_t* LIBGAV1_RESTRICT dst,
                                 const ptrdiff_t stride, const int width,
                                 const int height,
                                 const uint16_t* LIBGAV1_RESTRICT const top,
                                 const int xstep) {
  assert(width % 8 == 0);
  const int upsample_shift = static_cast<int>(upsampled);
  const int index_scale_bits = 6 - upsample_shift;

  const int max_base_index = (width + height - 1) << upsample_shift;
  const int16x8_t max_base_x = vdupq_n_s16(max_base_index);
  const uint16x8_t final_top_val = vdupq_n_u16(top[max_base_index]);
  const int16x8_t index_offset = {0, 1, 2, 3, 4, 5, 6, 7};

  const int base_step = 1 << upsample_shift;
  const int base_step8 = base_step << 3;
  const int16x8_t block_step = vdupq_n_s16(base_step8);

  // All rows from |min_corner_only_y| down will simply use Memset.
  // |max_base_x| is always greater than |height|, so clipping the denominator
  // to 1 is enough to make the logic work.
  const int xstep_units = std::max(xstep >> index_scale_bits, 1);
  const int min_corner_only_y = std::min(max_base_index / xstep_units, height);

  int top_x = xstep;
  int y = 0;
  for (; y < min_corner_only_y; ++y, dst += stride, top_x += xstep) {
    int top_base_x = top_x >> index_scale_bits;

    // To accommodate reuse of this function in Zone2, permit negative values
    // for |xstep|.
    const uint16_t shift_0 = (LeftShift(top_x, upsample_shift) & 0x3F) >> 1;
    const uint16_t shift_1 = 32 - shift_0;

    // Use signed values to compare |top_base_x| to |max_base_x|.
    int16x8_t base_x = vaddq_s16(vdupq_n_s16(top_base_x), index_offset);

    int x = 0;
    do {
      const uint16x8_t max_base_mask = vcltq_s16(base_x, max_base_x);

      uint16x8x2_t sampled_top_row;
      LoadEdgeVals(&sampled_top_row, top + top_base_x, upsampled);
      const uint16x8_t combined = WeightedBlend(
          sampled_top_row.val[0], sampled_top_row.val[1], shift_1, shift_0);

      const uint16x8_t masked_result =
          vbslq_u16(max_base_mask, combined, final_top_val);
      vst1q_u16(dst + x, masked_result);

      base_x = vaddq_s16(base_x, block_step);
      top_base_x += base_step8;
      x += 8;
    } while (x < width);
  }
  for (int i = y; i < height; ++i) {
    Memset(dst, top[max_base_index], width);
    dst += stride;
  }
}

// Process a multiple of 8 |width| by any |height|. Processes horizontally
// before vertically in the hopes of being a little more cache friendly.
inline void DirectionalZone1_Large(uint16_t* LIBGAV1_RESTRICT dst,
                                   const ptrdiff_t stride, const int width,
                                   const int height,
                                   const uint16_t* LIBGAV1_RESTRICT const top,
                                   const int xstep, const bool upsampled) {
  assert(width % 8 == 0);
  const int upsample_shift = static_cast<int>(upsampled);
  const int index_scale_bits = 6 - upsample_shift;

  const int max_base_index = (width + height - 1) << upsample_shift;
  const int16x8_t max_base_x = vdupq_n_s16(max_base_index);
  const uint16x8_t final_top_val = vdupq_n_u16(top[max_base_index]);
  const int16x8_t index_offset = {0, 1, 2, 3, 4, 5, 6, 7};

  const int base_step = 1 << upsample_shift;
  const int base_step8 = base_step << 3;
  const int16x8_t block_step = vdupq_n_s16(base_step8);

  // All rows from |min_corner_only_y| down will simply use Memset.
  // |max_base_x| is always greater than |height|, so clipping the denominator
  // to 1 is enough to make the logic work.
  const int xstep_units = std::max(xstep >> index_scale_bits, 1);
  const int min_corner_only_y = std::min(max_base_index / xstep_units, height);

  // Rows up to this y-value can be computed without checking for bounds.
  const int max_no_corner_y = std::min(
      ((max_base_index - (base_step * width)) << index_scale_bits) / xstep,
      height);
  // No need to check for exceeding |max_base_x| in the first loop.
  int y = 0;
  int top_x = xstep;
  for (; y < max_no_corner_y; ++y, dst += stride, top_x += xstep) {
    int top_base_x = top_x >> index_scale_bits;
    // To accommodate reuse of this function in Zone2, permit negative values
    // for |xstep|.
    const uint16_t shift_0 = (LeftShift(top_x, upsample_shift) & 0x3F) >> 1;
    const uint16_t shift_1 = 32 - shift_0;

    int x = 0;
    do {
      uint16x8x2_t sampled_top_row;
      LoadEdgeVals(&sampled_top_row, top + top_base_x, upsampled);
      const uint16x8_t combined = WeightedBlend(
          sampled_top_row.val[0], sampled_top_row.val[1], shift_1, shift_0);

      vst1q_u16(dst + x, combined);

      top_base_x += base_step8;
      x += 8;
    } while (x < width);
  }

  for (; y < min_corner_only_y; ++y, dst += stride, top_x += xstep) {
    int top_base_x = top_x >> index_scale_bits;

    // To accommodate reuse of this function in Zone2, permit negative values
    // for |xstep|.
    const uint16_t shift_0 = (LeftShift(top_x, upsample_shift) & 0x3F) >> 1;
    const uint16_t shift_1 = 32 - shift_0;

    // Use signed values to compare |top_base_x| to |max_base_x|.
    int16x8_t base_x = vaddq_s16(vdupq_n_s16(top_base_x), index_offset);

    int x = 0;
    const int min_corner_only_x =
        std::min(width, ((max_base_index - top_base_x) >> upsample_shift) + 7) &
        ~7;
    for (; x < min_corner_only_x; x += 8, top_base_x += base_step8,
                                  base_x = vaddq_s16(base_x, block_step)) {
      const uint16x8_t max_base_mask = vcltq_s16(base_x, max_base_x);

      uint16x8x2_t sampled_top_row;
      LoadEdgeVals(&sampled_top_row, top + top_base_x, upsampled);
      const uint16x8_t combined = WeightedBlend(
          sampled_top_row.val[0], sampled_top_row.val[1], shift_1, shift_0);

      const uint16x8_t masked_result =
          vbslq_u16(max_base_mask, combined, final_top_val);
      vst1q_u16(dst + x, masked_result);
    }
    // Corner-only section of the row.
    Memset(dst + x, top[max_base_index], width - x);
  }
  for (; y < height; ++y) {
    Memset(dst, top[max_base_index], width);
    dst += stride;
  }
}

void DirectionalIntraPredictorZone1_NEON(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row, const int width,
    const int height, const int xstep, const bool upsampled_top) {
  const auto* const top = static_cast<const uint16_t*>(top_row);
  auto* dst = static_cast<uint16_t*>(dest);
  stride /= sizeof(top[0]);

  assert(xstep > 0);

  if (xstep == 64) {
    assert(!upsampled_top);
    const uint16_t* top_ptr = top + 1;
    const int width_bytes = width * sizeof(top[0]);
    int y = height;
    do {
      memcpy(dst, top_ptr, width_bytes);
      memcpy(dst + stride, top_ptr + 1, width_bytes);
      memcpy(dst + 2 * stride, top_ptr + 2, width_bytes);
      memcpy(dst + 3 * stride, top_ptr + 3, width_bytes);
      dst += 4 * stride;
      top_ptr += 4;
      y -= 4;
    } while (y != 0);
  } else {
    if (width == 4) {
      if (upsampled_top) {
        DirectionalZone1_4xH<true>(dst, stride, height, top, xstep);
      } else {
        DirectionalZone1_4xH<false>(dst, stride, height, top, xstep);
      }
    } else if (width >= 32) {
      if (upsampled_top) {
        DirectionalZone1_Large(dst, stride, width, height, top, xstep, true);
      } else {
        DirectionalZone1_Large(dst, stride, width, height, top, xstep, false);
      }
    } else if (upsampled_top) {
      DirectionalZone1_WxH<true>(dst, stride, width, height, top, xstep);
    } else {
      DirectionalZone1_WxH<false>(dst, stride, width, height, top, xstep);
    }
  }
}

// -----------------------------------------------------------------------------
// Zone 3
// This can be considered "the transpose of Zone 1." In Zone 1, the fractional
// step applies when moving vertically in the destination block, connected to
// the change in |y|, whereas in this mode, the step applies when moving
// horizontally, connected to the change in |x|. This makes vectorization very
// complicated in row-order, because a given vector may need source pixels that
// span 16 or 32 pixels in steep angles, requiring multiple expensive table
// lookups and checked loads. Rather than work in row order, it is simpler to
// compute |dest| in column order, and then store the transposed results.

// Compute 4x4 sub-blocks.
// Example of computed sub-blocks of a 4x8 block before and after transpose:
// 00 10 20 30             00 01 02 03
// 01 11 21 31             10 11 12 13
// 02 12 22 32             20 21 22 23
// 03 13 23 33             30 31 32 33
// -----------     -->     -----------
// 40 50 60 70             40 41 42 43
// 41 51 61 71             50 51 52 53
// 42 52 62 72             60 61 62 63
// 43 53 63 73             70 71 72 73
template <bool upsampled>
inline void DirectionalZone3_4x4(uint8_t* LIBGAV1_RESTRICT dst,
                                 const ptrdiff_t stride,
                                 const uint16_t* LIBGAV1_RESTRICT const left,
                                 const int ystep, const int base_left_y = 0) {
  const int upsample_shift = static_cast<int>(upsampled);
  const int index_scale_bits = 6 - upsample_shift;

  // Compute one column at a time, then transpose for storage.
  uint16x4_t result[4];

  int left_y = base_left_y + ystep;
  int left_offset = left_y >> index_scale_bits;
  int shift_0 = (LeftShift(left_y, upsample_shift) & 0x3F) >> 1;
  int shift_1 = 32 - shift_0;
  uint16x4x2_t sampled_left_col;
  LoadEdgeVals(&sampled_left_col, &left[left_offset], upsampled);
  result[0] = WeightedBlend(sampled_left_col.val[0], sampled_left_col.val[1],
                            shift_1, shift_0);

  left_y += ystep;
  left_offset = left_y >> index_scale_bits;
  shift_0 = (LeftShift(left_y, upsample_shift) & 0x3F) >> 1;
  shift_1 = 32 - shift_0;
  LoadEdgeVals(&sampled_left_col, &left[left_offset], upsampled);
  result[1] = WeightedBlend(sampled_left_col.val[0], sampled_left_col.val[1],
                            shift_1, shift_0);

  left_y += ystep;
  left_offset = left_y >> index_scale_bits;
  shift_0 = (LeftShift(left_y, upsample_shift) & 0x3F) >> 1;
  shift_1 = 32 - shift_0;
  LoadEdgeVals(&sampled_left_col, &left[left_offset], upsampled);
  result[2] = WeightedBlend(sampled_left_col.val[0], sampled_left_col.val[1],
                            shift_1, shift_0);

  left_y += ystep;
  left_offset = left_y >> index_scale_bits;
  shift_0 = (LeftShift(left_y, upsample_shift) & 0x3F) >> 1;
  shift_1 = 32 - shift_0;
  LoadEdgeVals(&sampled_left_col, &left[left_offset], upsampled);
  result[3] = WeightedBlend(sampled_left_col.val[0], sampled_left_col.val[1],
                            shift_1, shift_0);

  Transpose4x4(result);
  Store4(dst, result[0]);
  dst += stride;
  Store4(dst, result[1]);
  dst += stride;
  Store4(dst, result[2]);
  dst += stride;
  Store4(dst, result[3]);
}

template <bool upsampled>
inline void DirectionalZone3_8x4(uint8_t* LIBGAV1_RESTRICT dst,
                                 const ptrdiff_t stride,
                                 const uint16_t* LIBGAV1_RESTRICT const left,
                                 const int ystep, const int base_left_y = 0) {
  const int upsample_shift = static_cast<int>(upsampled);
  const int index_scale_bits = 6 - upsample_shift;
  const uint16x8_t inverter = vdupq_n_u16(32);

  uint16x8x2_t sampled_left_col;
  // Compute two columns at a time, then transpose for storage.
  uint16x8_t result[4];

  // The low half of pre-transpose vectors contains columns 0 through 3.
  int left_y_low = base_left_y + ystep;
  int left_offset_low = left_y_low >> index_scale_bits;
  int shift_low = (LeftShift(left_y_low, upsample_shift) & 0x3F) >> 1;

  // The high half of pre-transpose vectors contains columns 4 through 7.
  int left_y_high = left_y_low + (ystep << 2);
  int left_offset_high = left_y_high >> index_scale_bits;
  int shift_high = (LeftShift(left_y_high, upsample_shift) & 0x3F) >> 1;
  uint16x8_t weights_0 =
      vcombine_u16(vdup_n_u16(shift_low), vdup_n_u16(shift_high));
  uint16x8_t weights_1 = vsubq_u16(inverter, weights_0);
  LoadEdgeVals2x4(&sampled_left_col, &left[left_offset_low],
                  &left[left_offset_high], upsampled);
  result[0] = WeightedBlend(sampled_left_col.val[0], sampled_left_col.val[1],
                            weights_1, weights_0);

  left_y_low += ystep;
  left_offset_low = left_y_low >> index_scale_bits;
  shift_low = (LeftShift(left_y_low, upsample_shift) & 0x3F) >> 1;

  left_y_high += ystep;
  left_offset_high = left_y_high >> index_scale_bits;
  shift_high = (LeftShift(left_y_high, upsample_shift) & 0x3F) >> 1;
  weights_0 = vcombine_u16(vdup_n_u16(shift_low), vdup_n_u16(shift_high));
  weights_1 = vsubq_u16(inverter, weights_0);
  LoadEdgeVals2x4(&sampled_left_col, &left[left_offset_low],
                  &left[left_offset_high], upsampled);
  result[1] = WeightedBlend(sampled_left_col.val[0], sampled_left_col.val[1],
                            weights_1, weights_0);

  left_y_high += ystep;
  left_y_low += ystep;
  left_offset_low = left_y_low >> index_scale_bits;
  shift_low = (LeftShift(left_y_low, upsample_shift) & 0x3F) >> 1;

  left_offset_high = left_y_high >> index_scale_bits;
  shift_high = (LeftShift(left_y_high, upsample_shift) & 0x3F) >> 1;
  weights_0 = vcombine_u16(vdup_n_u16(shift_low), vdup_n_u16(shift_high));
  weights_1 = vsubq_u16(inverter, weights_0);
  LoadEdgeVals2x4(&sampled_left_col, &left[left_offset_low],
                  &left[left_offset_high], upsampled);
  result[2] = WeightedBlend(sampled_left_col.val[0], sampled_left_col.val[1],
                            weights_1, weights_0);

  left_y_low += ystep;
  left_offset_low = left_y_low >> index_scale_bits;
  shift_low = (LeftShift(left_y_low, upsample_shift) & 0x3F) >> 1;

  left_y_high += ystep;
  left_offset_high = left_y_high >> index_scale_bits;
  shift_high = (LeftShift(left_y_high, upsample_shift) & 0x3F) >> 1;
  weights_0 = vcombine_u16(vdup_n_u16(shift_low), vdup_n_u16(shift_high));
  weights_1 = vsubq_u16(inverter, weights_0);
  LoadEdgeVals2x4(&sampled_left_col, &left[left_offset_low],
                  &left[left_offset_high], upsampled);
  result[3] = WeightedBlend(sampled_left_col.val[0], sampled_left_col.val[1],
                            weights_1, weights_0);

  Transpose4x8(result);
  Store8(dst, result[0]);
  dst += stride;
  Store8(dst, result[1]);
  dst += stride;
  Store8(dst, result[2]);
  dst += stride;
  Store8(dst, result[3]);
}

template <bool upsampled>
inline void DirectionalZone3_4x8(uint8_t* LIBGAV1_RESTRICT dst,
                                 const ptrdiff_t stride,
                                 const uint16_t* LIBGAV1_RESTRICT const left,
                                 const int ystep, const int base_left_y = 0) {
  const int upsample_shift = static_cast<int>(upsampled);
  const int index_scale_bits = 6 - upsample_shift;

  // Compute one column at a time, then transpose for storage.
  uint16x8_t result[4];

  int left_y = base_left_y + ystep;
  int left_offset = left_y >> index_scale_bits;
  int shift_0 = (LeftShift(left_y, upsample_shift) & 0x3F) >> 1;
  int shift_1 = 32 - shift_0;
  uint16x8x2_t sampled_left_col;
  LoadEdgeVals(&sampled_left_col, &left[left_offset], upsampled);
  result[0] = WeightedBlend(sampled_left_col.val[0], sampled_left_col.val[1],
                            shift_1, shift_0);

  left_y += ystep;
  left_offset = left_y >> index_scale_bits;
  shift_0 = (LeftShift(left_y, upsample_shift) & 0x3F) >> 1;
  shift_1 = 32 - shift_0;
  LoadEdgeVals(&sampled_left_col, &left[left_offset], upsampled);
  result[1] = WeightedBlend(sampled_left_col.val[0], sampled_left_col.val[1],
                            shift_1, shift_0);

  left_y += ystep;
  left_offset = left_y >> index_scale_bits;
  shift_0 = (LeftShift(left_y, upsample_shift) & 0x3F) >> 1;
  shift_1 = 32 - shift_0;
  LoadEdgeVals(&sampled_left_col, &left[left_offset], upsampled);
  result[2] = WeightedBlend(sampled_left_col.val[0], sampled_left_col.val[1],
                            shift_1, shift_0);

  left_y += ystep;
  left_offset = left_y >> index_scale_bits;
  shift_0 = (LeftShift(left_y, upsample_shift) & 0x3F) >> 1;
  shift_1 = 32 - shift_0;
  LoadEdgeVals(&sampled_left_col, &left[left_offset], upsampled);
  result[3] = WeightedBlend(sampled_left_col.val[0], sampled_left_col.val[1],
                            shift_1, shift_0);

  Transpose4x8(result);
  Store4(dst, vget_low_u16(result[0]));
  dst += stride;
  Store4(dst, vget_low_u16(result[1]));
  dst += stride;
  Store4(dst, vget_low_u16(result[2]));
  dst += stride;
  Store4(dst, vget_low_u16(result[3]));
  dst += stride;
  Store4(dst, vget_high_u16(result[0]));
  dst += stride;
  Store4(dst, vget_high_u16(result[1]));
  dst += stride;
  Store4(dst, vget_high_u16(result[2]));
  dst += stride;
  Store4(dst, vget_high_u16(result[3]));
}

template <bool upsampled>
inline void DirectionalZone3_4xH(uint8_t* LIBGAV1_RESTRICT dest,
                                 const ptrdiff_t stride, const int height,
                                 const uint16_t* LIBGAV1_RESTRICT const left,
                                 const int ystep) {
  assert(height == 8 || height == 16);
  const int upsample_shift = static_cast<int>(upsampled);
  DirectionalZone3_4x8<upsampled>(dest, stride, left, ystep);
  if (height == 16) {
    dest += stride << 3;
    DirectionalZone3_4x8<upsampled>(dest, stride, left + (8 << upsample_shift),
                                    ystep);
  }
}

template <bool upsampled>
inline void DirectionalZone3_Wx4(uint8_t* LIBGAV1_RESTRICT dest,
                                 const ptrdiff_t stride, const int width,
                                 const uint16_t* LIBGAV1_RESTRICT const left,
                                 const int ystep) {
  assert(width <= 16);
  if (width == 4) {
    DirectionalZone3_4x4<upsampled>(dest, stride, left, ystep);
    return;
  }
  DirectionalZone3_8x4<upsampled>(dest, stride, left, ystep);
  if (width == 16) {
    const int base_left_y = ystep << 3;
    DirectionalZone3_8x4<upsampled>(dest + 8 * sizeof(uint16_t), stride, left,
                                    ystep, base_left_y);
  }
}

template <bool upsampled>
inline void DirectionalZone3_8x8(uint8_t* LIBGAV1_RESTRICT dest,
                                 const ptrdiff_t stride,
                                 const uint16_t* LIBGAV1_RESTRICT const left,
                                 const int ystep, const int base_left_y = 0) {
  const int upsample_shift = static_cast<int>(upsampled);
  const int index_scale_bits = 6 - upsample_shift;

  // Compute one column at a time, then transpose for storage.
  uint16x8_t result[8];

  int left_y = base_left_y + ystep;
  uint16x8x2_t sampled_left_col;
  int left_offset = left_y >> index_scale_bits;
  int shift_0 = (LeftShift(left_y, upsample_shift) & 0x3F) >> 1;
  int shift_1 = 32 - shift_0;
  LoadEdgeVals(&sampled_left_col, &left[left_offset], upsampled);
  result[0] = WeightedBlend(sampled_left_col.val[0], sampled_left_col.val[1],
                            shift_1, shift_0);
  left_y += ystep;
  left_offset = left_y >> index_scale_bits;
  shift_0 = (LeftShift(left_y, upsample_shift) & 0x3F) >> 1;
  shift_1 = 32 - shift_0;
  LoadEdgeVals(&sampled_left_col, &left[left_offset], upsampled);
  result[1] = WeightedBlend(sampled_left_col.val[0], sampled_left_col.val[1],
                            shift_1, shift_0);

  left_y += ystep;
  left_offset = left_y >> index_scale_bits;
  shift_0 = (LeftShift(left_y, upsample_shift) & 0x3F) >> 1;
  shift_1 = 32 - shift_0;
  LoadEdgeVals(&sampled_left_col, &left[left_offset], upsampled);
  result[2] = WeightedBlend(sampled_left_col.val[0], sampled_left_col.val[1],
                            shift_1, shift_0);

  left_y += ystep;
  left_offset = left_y >> index_scale_bits;
  shift_0 = (LeftShift(left_y, upsample_shift) & 0x3F) >> 1;
  shift_1 = 32 - shift_0;
  LoadEdgeVals(&sampled_left_col, &left[left_offset], upsampled);
  result[3] = WeightedBlend(sampled_left_col.val[0], sampled_left_col.val[1],
                            shift_1, shift_0);

  left_y += ystep;
  left_offset = left_y >> index_scale_bits;
  shift_0 = (LeftShift(left_y, upsample_shift) & 0x3F) >> 1;
  shift_1 = 32 - shift_0;
  LoadEdgeVals(&sampled_left_col, &left[left_offset], upsampled);
  result[4] = WeightedBlend(sampled_left_col.val[0], sampled_left_col.val[1],
                            shift_1, shift_0);

  left_y += ystep;
  left_offset = left_y >> index_scale_bits;
  shift_0 = (LeftShift(left_y, upsample_shift) & 0x3F) >> 1;
  shift_1 = 32 - shift_0;
  LoadEdgeVals(&sampled_left_col, &left[left_offset], upsampled);
  result[5] = WeightedBlend(sampled_left_col.val[0], sampled_left_col.val[1],
                            shift_1, shift_0);

  left_y += ystep;
  left_offset = left_y >> index_scale_bits;
  shift_0 = (LeftShift(left_y, upsample_shift) & 0x3F) >> 1;
  shift_1 = 32 - shift_0;
  LoadEdgeVals(&sampled_left_col, &left[left_offset], upsampled);
  result[6] = WeightedBlend(sampled_left_col.val[0], sampled_left_col.val[1],
                            shift_1, shift_0);

  left_y += ystep;
  left_offset = left_y >> index_scale_bits;
  shift_0 = (LeftShift(left_y, upsample_shift) & 0x3F) >> 1;
  shift_1 = 32 - shift_0;
  LoadEdgeVals(&sampled_left_col, &left[left_offset], upsampled);
  result[7] = WeightedBlend(sampled_left_col.val[0], sampled_left_col.val[1],
                            shift_1, shift_0);

  Transpose8x8(result);
  Store8(dest, result[0]);
  dest += stride;
  Store8(dest, result[1]);
  dest += stride;
  Store8(dest, result[2]);
  dest += stride;
  Store8(dest, result[3]);
  dest += stride;
  Store8(dest, result[4]);
  dest += stride;
  Store8(dest, result[5]);
  dest += stride;
  Store8(dest, result[6]);
  dest += stride;
  Store8(dest, result[7]);
}

template <bool upsampled>
inline void DirectionalZone3_WxH(uint8_t* LIBGAV1_RESTRICT dest,
                                 const ptrdiff_t stride, const int width,
                                 const int height,
                                 const uint16_t* LIBGAV1_RESTRICT const left,
                                 const int ystep) {
  const int upsample_shift = static_cast<int>(upsampled);
  // Zone3 never runs out of left_column values.
  assert((width + height - 1) << upsample_shift >  // max_base_y
         ((ystep * width) >> (6 - upsample_shift)) +
             (/* base_step */ 1 << upsample_shift) *
                 (height - 1));  // left_base_y
  int y = 0;
  do {
    int x = 0;
    uint8_t* dst_x = dest + y * stride;
    do {
      const int base_left_y = ystep * x;
      DirectionalZone3_8x8<upsampled>(
          dst_x, stride, left + (y << upsample_shift), ystep, base_left_y);
      dst_x += 8 * sizeof(uint16_t);
      x += 8;
    } while (x < width);
    y += 8;
  } while (y < height);
}

void DirectionalIntraPredictorZone3_NEON(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const left_column, const int width,
    const int height, const int ystep, const bool upsampled_left) {
  const auto* const left = static_cast<const uint16_t*>(left_column);
  auto* dst = static_cast<uint8_t*>(dest);

  if (ystep == 64) {
    assert(!upsampled_left);
    const int width_bytes = width * sizeof(left[0]);
    int y = height;
    do {
      const uint16_t* left_ptr = left + 1;
      memcpy(dst, left_ptr, width_bytes);
      memcpy(dst + stride, left_ptr + 1, width_bytes);
      memcpy(dst + 2 * stride, left_ptr + 2, width_bytes);
      memcpy(dst + 3 * stride, left_ptr + 3, width_bytes);
      dst += 4 * stride;
      left_ptr += 4;
      y -= 4;
    } while (y != 0);
    return;
  }
  if (height == 4) {
    if (upsampled_left) {
      DirectionalZone3_Wx4<true>(dst, stride, width, left, ystep);
    } else {
      DirectionalZone3_Wx4<false>(dst, stride, width, left, ystep);
    }
  } else if (width == 4) {
    if (upsampled_left) {
      DirectionalZone3_4xH<true>(dst, stride, height, left, ystep);
    } else {
      DirectionalZone3_4xH<false>(dst, stride, height, left, ystep);
    }
  } else {
    if (upsampled_left) {
      // |upsampled_left| can only be true if |width| + |height| <= 16,
      // therefore this is 8x8.
      DirectionalZone3_8x8<true>(dst, stride, left, ystep);
    } else {
      DirectionalZone3_WxH<false>(dst, stride, width, height, left, ystep);
    }
  }
}

// -----------------------------------------------------------------------------
// Zone2
// This function deals with cases not found in zone 1 or zone 3. The extreme
// angles are 93, which makes for sharp ascents along |left_column| with each
// successive dest row element until reaching |top_row|, and 177, with a shallow
// ascent up |left_column| until reaching large jumps along |top_row|. In the
// extremely steep cases, source vectors can only be loaded one lane at a time.

// Fill |left| and |right| with the appropriate values for a given |base_step|.
inline void LoadStepwise(const void* LIBGAV1_RESTRICT const source,
                         const uint8x8_t left_step, const uint8x8_t right_step,
                         uint16x4_t* left, uint16x4_t* right) {
  const uint8x16x2_t mixed = {
      vld1q_u8(static_cast<const uint8_t*>(source)),
      vld1q_u8(static_cast<const uint8_t*>(source) + 16)};
  *left = vreinterpret_u16_u8(VQTbl2U8(mixed, left_step));
  *right = vreinterpret_u16_u8(VQTbl2U8(mixed, right_step));
}

inline void LoadStepwise(const void* LIBGAV1_RESTRICT const source,
                         const uint8x8_t left_step_0,
                         const uint8x8_t right_step_0,
                         const uint8x8_t left_step_1,
                         const uint8x8_t right_step_1, uint16x8_t* left,
                         uint16x8_t* right) {
  const uint8x16x2_t mixed = {
      vld1q_u8(static_cast<const uint8_t*>(source)),
      vld1q_u8(static_cast<const uint8_t*>(source) + 16)};
  const uint16x4_t left_low = vreinterpret_u16_u8(VQTbl2U8(mixed, left_step_0));
  const uint16x4_t left_high =
      vreinterpret_u16_u8(VQTbl2U8(mixed, left_step_1));
  *left = vcombine_u16(left_low, left_high);
  const uint16x4_t right_low =
      vreinterpret_u16_u8(VQTbl2U8(mixed, right_step_0));
  const uint16x4_t right_high =
      vreinterpret_u16_u8(VQTbl2U8(mixed, right_step_1));
  *right = vcombine_u16(right_low, right_high);
}

// Blend two values based on weight pairs that each sum to 32.
inline uint16x4_t WeightedBlend(const uint16x4_t a, const uint16x4_t b,
                                const uint16x4_t a_weight,
                                const uint16x4_t b_weight) {
  const uint16x4_t a_product = vmul_u16(a, a_weight);
  const uint16x4_t sum = vmla_u16(a_product, b, b_weight);

  return vrshr_n_u16(sum, 5 /*log2(32)*/);
}

// Because the source values "move backwards" as the row index increases, the
// indices derived from ystep are generally negative in localized functions.
// This is accommodated by making sure the relative indices are within [-15, 0]
// when the function is called, and sliding them into the inclusive range
// [0, 15], relative to a lower base address. 15 is the Pixel offset, so 30 is
// the byte offset for table lookups.

constexpr int kPositiveIndexOffsetPixels = 15;
constexpr int kPositiveIndexOffsetBytes = 30;

inline void DirectionalZone2FromLeftCol_4xH(
    uint8_t* LIBGAV1_RESTRICT dst, const ptrdiff_t stride, const int height,
    const uint16_t* LIBGAV1_RESTRICT const left_column, const int16x4_t left_y,
    const bool upsampled) {
  const int upsample_shift = static_cast<int>(upsampled);

  const int index_scale_bits = 6;
  // The values in |offset_y| are negative, except for the first element, which
  // is zero.
  int16x4_t offset_y;
  int16x4_t shift_upsampled = left_y;
  // The shift argument must be a constant, otherwise use upsample_shift
  // directly.
  if (upsampled) {
    offset_y = vshr_n_s16(left_y, index_scale_bits - 1 /*upsample_shift*/);
    shift_upsampled = vshl_n_s16(shift_upsampled, 1);
  } else {
    offset_y = vshr_n_s16(left_y, index_scale_bits);
  }
  offset_y = vshl_n_s16(offset_y, 1);

  // Select values to the left of the starting point.
  // The 15th element (and 16th) will be all the way at the end, to the
  // right. With a negative ystep everything else will be "left" of them.
  // This supports cumulative steps up to 15. We could support up to 16 by
  // doing separate loads for |left_values| and |right_values|. vtbl
  // supports 2 Q registers as input which would allow for cumulative
  // offsets of 32.
  // |sampler_0| indexes the first byte of each 16-bit value.
  const int16x4_t sampler_0 =
      vadd_s16(offset_y, vdup_n_s16(kPositiveIndexOffsetBytes));
  // |sampler_1| indexes the second byte of each 16-bit value.
  const int16x4_t sampler_1 = vadd_s16(sampler_0, vdup_n_s16(1));
  const int16x4x2_t sampler = vzip_s16(sampler_0, sampler_1);
  const uint8x8_t left_indices =
      vqmovun_s16(vcombine_s16(sampler.val[0], sampler.val[1]));
  const uint8x8_t right_indices =
      vadd_u8(left_indices, vdup_n_u8(sizeof(uint16_t)));

  const int16x4_t shift_masked = vand_s16(shift_upsampled, vdup_n_s16(0x3f));
  const uint16x4_t shift_0 = vreinterpret_u16_s16(vshr_n_s16(shift_masked, 1));
  const uint16x4_t shift_1 = vsub_u16(vdup_n_u16(32), shift_0);

  int y = 0;
  do {
    uint16x4_t src_left, src_right;
    LoadStepwise(
        left_column - kPositiveIndexOffsetPixels + (y << upsample_shift),
        left_indices, right_indices, &src_left, &src_right);
    const uint16x4_t val = WeightedBlend(src_left, src_right, shift_1, shift_0);

    Store4(dst, val);
    dst += stride;
  } while (++y < height);
}

inline void DirectionalZone2FromLeftCol_8x8(
    uint8_t* LIBGAV1_RESTRICT dst, const ptrdiff_t stride,
    const uint16_t* LIBGAV1_RESTRICT const left_column, const int16x8_t left_y,
    const bool upsampled) {
  const int upsample_shift = static_cast<int>(upsampled);

  const int index_scale_bits = 6;
  // The values in |offset_y| are negative, except for the first element, which
  // is zero.
  int16x8_t offset_y;
  int16x8_t shift_upsampled = left_y;
  // The shift argument must be a constant, otherwise use upsample_shift
  // directly.
  if (upsampled) {
    offset_y = vshrq_n_s16(left_y, index_scale_bits - 1);
    shift_upsampled = vshlq_n_s16(shift_upsampled, 1);
  } else {
    offset_y = vshrq_n_s16(left_y, index_scale_bits);
  }
  offset_y = vshlq_n_s16(offset_y, 1);

  // Select values to the left of the starting point.
  // The 15th element (and 16th) will be all the way at the end, to the right.
  // With a negative ystep everything else will be "left" of them.
  // This supports cumulative steps up to 15. We could support up to 16 by doing
  // separate loads for |left_values| and |right_values|. vtbl supports 2 Q
  // registers as input which would allow for cumulative offsets of 32.
  // |sampler_0| indexes the first byte of each 16-bit value.
  const int16x8_t sampler_0 =
      vaddq_s16(offset_y, vdupq_n_s16(kPositiveIndexOffsetBytes));
  // |sampler_1| indexes the second byte of each 16-bit value.
  const int16x8_t sampler_1 = vaddq_s16(sampler_0, vdupq_n_s16(1));
  const int16x8x2_t sampler = vzipq_s16(sampler_0, sampler_1);
  const uint8x8_t left_values_0 = vqmovun_s16(sampler.val[0]);
  const uint8x8_t left_values_1 = vqmovun_s16(sampler.val[1]);
  const uint8x8_t right_values_0 =
      vadd_u8(left_values_0, vdup_n_u8(sizeof(uint16_t)));
  const uint8x8_t right_values_1 =
      vadd_u8(left_values_1, vdup_n_u8(sizeof(uint16_t)));

  const int16x8_t shift_masked = vandq_s16(shift_upsampled, vdupq_n_s16(0x3f));
  const uint16x8_t shift_0 =
      vreinterpretq_u16_s16(vshrq_n_s16(shift_masked, 1));
  const uint16x8_t shift_1 = vsubq_u16(vdupq_n_u16(32), shift_0);

  for (int y = 0; y < 8; ++y) {
    uint16x8_t src_left, src_right;
    LoadStepwise(
        left_column - kPositiveIndexOffsetPixels + (y << upsample_shift),
        left_values_0, right_values_0, left_values_1, right_values_1, &src_left,
        &src_right);
    const uint16x8_t val = WeightedBlend(src_left, src_right, shift_1, shift_0);

    Store8(dst, val);
    dst += stride;
  }
}

template <bool upsampled>
inline void DirectionalZone1Blend_4xH(
    uint8_t* LIBGAV1_RESTRICT dest, const ptrdiff_t stride, const int height,
    const uint16_t* LIBGAV1_RESTRICT const top_row, int zone_bounds, int top_x,
    const int xstep) {
  const int upsample_shift = static_cast<int>(upsampled);
  const int scale_bits_x = 6 - upsample_shift;

  // Representing positions along the row, which |zone_bounds| will target for
  // the blending boundary.
  const int16x4_t indices = {0, 1, 2, 3};

  uint16x4x2_t top_vals;
  int y = height;
  do {
    const uint16_t* const src = top_row + (top_x >> scale_bits_x);
    LoadEdgeVals(&top_vals, src, upsampled);

    const uint16_t shift_0 = ((top_x << upsample_shift) & 0x3f) >> 1;
    const uint16_t shift_1 = 32 - shift_0;

    const uint16x4_t val =
        WeightedBlend(top_vals.val[0], top_vals.val[1], shift_1, shift_0);

    const uint16x4_t dst_blend = Load4U16(dest);
    // |zone_bounds| values can be negative.
    const uint16x4_t blend = vcge_s16(indices, vdup_n_s16(zone_bounds >> 6));
    const uint16x4_t output = vbsl_u16(blend, val, dst_blend);

    Store4(dest, output);
    dest += stride;
    zone_bounds += xstep;
    top_x -= xstep;
  } while (--y != 0);
}

template <bool upsampled>
inline void DirectionalZone1Blend_8x8(
    uint8_t* LIBGAV1_RESTRICT dest, const ptrdiff_t stride,
    const uint16_t* LIBGAV1_RESTRICT const top_row, int zone_bounds, int top_x,
    const int xstep) {
  const int upsample_shift = static_cast<int>(upsampled);
  const int scale_bits_x = 6 - upsample_shift;

  // Representing positions along the row, which |zone_bounds| will target for
  // the blending boundary.
  const int16x8_t indices = {0, 1, 2, 3, 4, 5, 6, 7};

  uint16x8x2_t top_vals;
  for (int y = 0; y < 8; ++y) {
    const uint16_t* const src = top_row + (top_x >> scale_bits_x);
    LoadEdgeVals(&top_vals, src, upsampled);

    const uint16_t shift_0 = ((top_x << upsample_shift) & 0x3f) >> 1;
    const uint16_t shift_1 = 32 - shift_0;

    const uint16x8_t val =
        WeightedBlend(top_vals.val[0], top_vals.val[1], shift_1, shift_0);

    const uint16x8_t dst_blend = Load8U16(dest);
    // |zone_bounds| values can be negative.
    const uint16x8_t blend = vcgeq_s16(indices, vdupq_n_s16(zone_bounds >> 6));
    const uint16x8_t output = vbslq_u16(blend, val, dst_blend);

    Store8(dest, output);
    dest += stride;
    zone_bounds += xstep;
    top_x -= xstep;
  }
}

// 7.11.2.4 (8) 90 < angle > 180
// The strategy for these functions (4xH and 8+xH) is to know how many blocks
// can be processed with just pixels from |top_ptr|, then handle mixed blocks,
// then handle only blocks that take from |left_ptr|. Additionally, a fast
// index-shuffle approach is used for pred values from |left_column| in sections
// that permit it.
template <bool upsampled_top, bool upsampled_left>
inline void DirectionalZone2_4xH(
    uint8_t* LIBGAV1_RESTRICT dst, const ptrdiff_t stride,
    const uint16_t* LIBGAV1_RESTRICT const top_row,
    const uint16_t* LIBGAV1_RESTRICT const left_column, const int height,
    const int xstep, const int ystep) {
  const int upsample_left_shift = static_cast<int>(upsampled_left);

  // Helper vector for index computation.
  const int16x4_t zero_to_three = {0, 1, 2, 3};

  // Loop increments for moving by block (4xN). Vertical still steps by 8. If
  // it's only 4, it will be finished in the first iteration.
  const ptrdiff_t stride8 = stride << 3;
  const int xstep8 = xstep << 3;

  const int min_height = (height == 4) ? 4 : 8;

  // All columns from |min_top_only_x| to the right will only need |top_row| to
  // compute and can therefore call the Zone1 functions. This assumes |xstep| is
  // at least 3.
  assert(xstep >= 3);

  // Offsets the original zone bound value to simplify x < (y+1)*xstep/64 -1
  int xstep_bounds_base = (xstep == 64) ? 0 : xstep - 1;

  const int left_base_increment = ystep >> 6;
  const int ystep_remainder = ystep & 0x3F;

  // If the 64 scaling is regarded as a decimal point, the first value of the
  // left_y vector omits the portion which is covered under the left_column
  // offset. The following values need the full ystep as a relative offset.
  const int16x4_t left_y =
      vmla_n_s16(vdup_n_s16(-ystep_remainder), zero_to_three, -ystep);

  // This loop treats the 4 columns in 3 stages with y-value boundaries.
  // The first stage, before the first y-loop, covers blocks that are only
  // computed from the top row. The second stage, comprising two y-loops, covers
  // blocks that have a mixture of values computed from top or left. The final
  // stage covers blocks that are only computed from the left.
  // Round down to the nearest multiple of 8 (or 4, if height is 4).
  const int max_top_only_y =
      std::min((1 << 6) / xstep, height) & ~(min_height - 1);
  DirectionalZone1_4xH<upsampled_top>(reinterpret_cast<uint16_t*>(dst),
                                      stride >> 1, max_top_only_y, top_row,
                                      -xstep);

  if (max_top_only_y == height) return;

  int y = max_top_only_y;
  dst += stride * y;
  const int xstep_y = xstep * y;

  // All rows from |min_left_only_y| down for this set of columns only need
  // |left_column| to compute.
  const int min_left_only_y = std::min((4 /*width*/ << 6) / xstep, height);
  int xstep_bounds = xstep_bounds_base + xstep_y;
  int top_x = -xstep - xstep_y;

  // +8 increment is OK because if height is 4 this only runs once.
  for (; y < min_left_only_y;
       y += 8, dst += stride8, xstep_bounds += xstep8, top_x -= xstep8) {
    DirectionalZone2FromLeftCol_4xH(
        dst, stride, min_height,
        left_column + ((y - left_base_increment) << upsample_left_shift),
        left_y, upsampled_left);

    DirectionalZone1Blend_4xH<upsampled_top>(dst, stride, min_height, top_row,
                                             xstep_bounds, top_x, xstep);
  }

  // Left-only section. |height| - |y| is assumed equivalent to:
  // (y == 0) && (height == 4)
  if (height - y == 4) {
    DirectionalZone3_4x4<upsampled_left>(dst, stride, left_column, -ystep);
    return;
  }
  if (y < height) {
    DirectionalZone3_4xH<upsampled_left>(
        dst, stride, height - y, left_column + (y << upsample_left_shift),
        -ystep);
  }
}

// Process 8x4 and 16x4 blocks. This avoids a lot of overhead and simplifies
// address safety.
template <bool upsampled_top, bool upsampled_left>
inline void DirectionalZone2_Wx4(
    uint8_t* LIBGAV1_RESTRICT const dst, const ptrdiff_t stride,
    const uint16_t* LIBGAV1_RESTRICT const top_row,
    const uint16_t* LIBGAV1_RESTRICT const left_column, const int width,
    const int xstep, const int ystep) {
  const int upsample_top_shift = static_cast<int>(upsampled_top);
  // Offsets the original zone bound value to simplify x < (y+1)*xstep/64 -1
  int xstep_bounds_base = (xstep == 64) ? 0 : xstep - 1;

  const int min_top_only_x = std::min((4 * xstep) >> 6, width);
  int x = 0;
  for (; x < min_top_only_x; x += 4, xstep_bounds_base -= (4 << 6)) {
    uint8_t* dst_x = dst + x * sizeof(uint16_t);

    // Round down to the nearest multiple of 4.
    const int max_top_only_y = (((x + 1) << 6) / xstep) & ~3;
    if (max_top_only_y != 0) {
      DirectionalZone1_4xH<upsampled_top>(
          reinterpret_cast<uint16_t*>(dst_x), stride >> 1, 4,
          top_row + (x << upsample_top_shift), -xstep);
      continue;
    }

    DirectionalZone3_4x4<upsampled_left>(dst_x, stride, left_column, -ystep,
                                         -ystep * x);

    const int min_left_only_y = ((x + 4) << 6) / xstep;
    if (min_left_only_y != 0) {
      const int top_x = -xstep;
      DirectionalZone1Blend_4xH<upsampled_top>(
          dst_x, stride, 4, top_row + (x << upsample_top_shift),
          xstep_bounds_base, top_x, xstep);
    }
  }
  // Reached |min_top_only_x|.
  for (; x < width; x += 4) {
    DirectionalZone1_4xH<upsampled_top>(
        reinterpret_cast<uint16_t*>(dst) + x, stride >> 1, 4,
        top_row + (x << upsample_top_shift), -xstep);
  }
}

template <bool shuffle_left_column, bool upsampled_top, bool upsampled_left>
inline void DirectionalZone2_8xH(
    uint8_t* LIBGAV1_RESTRICT const dst, const ptrdiff_t stride,
    const uint16_t* LIBGAV1_RESTRICT const top_row,
    const uint16_t* LIBGAV1_RESTRICT const left_column, const int height,
    const int xstep, const int ystep, const int x, const int left_offset,
    const int xstep_bounds_base, const int16x8_t left_y) {
  const int upsample_left_shift = static_cast<int>(upsampled_left);
  const int upsample_top_shift = static_cast<int>(upsampled_top);

  // Loop incrementers for moving by block (8x8). This function handles blocks
  // with height 4 as well. They are calculated in one pass so these variables
  // do not get used.
  const ptrdiff_t stride8 = stride << 3;
  const int xstep8 = xstep << 3;

  // The first stage, before the first y-loop, covers blocks that are only
  // computed from the top row. The second stage, comprising two y-loops, covers
  // blocks that have a mixture of values computed from top or left. The final
  // stage covers blocks that are only computed from the left.
  uint8_t* dst_x = dst + x * sizeof(uint16_t);
  // Round down to the nearest multiple of 8.
  const int max_top_only_y = std::min(((x + 1) << 6) / xstep, height) & ~7;
  DirectionalZone1_WxH<upsampled_top>(
      reinterpret_cast<uint16_t*>(dst_x), stride >> 1, 8, max_top_only_y,
      top_row + (x << upsample_top_shift), -xstep);

  if (max_top_only_y == height) return;

  int y = max_top_only_y;
  dst_x += stride * y;
  const int xstep_y = xstep * y;

  // All rows from |min_left_only_y| down for this set of columns only need
  // |left_column| to compute. Round up to the nearest 8.
  const int min_left_only_y =
      Align(std::min(((x + 8) << 6) / xstep, height), 8);
  int xstep_bounds = xstep_bounds_base + xstep_y;
  int top_x = -xstep - xstep_y;

  for (; y < min_left_only_y;
       y += 8, dst_x += stride8, xstep_bounds += xstep8, top_x -= xstep8) {
    if (shuffle_left_column) {
      DirectionalZone2FromLeftCol_8x8(
          dst_x, stride,
          left_column + ((left_offset + y) << upsample_left_shift), left_y,
          upsampled_left);
    } else {
      DirectionalZone3_8x8<upsampled_left>(
          dst_x, stride, left_column + (y << upsample_left_shift), -ystep,
          -ystep * x);
    }

    DirectionalZone1Blend_8x8<upsampled_top>(
        dst_x, stride, top_row + (x << upsample_top_shift), xstep_bounds, top_x,
        xstep);
  }

  // Loop over y for left_only rows.
  for (; y < height; y += 8, dst_x += stride8) {
    DirectionalZone3_8x8<upsampled_left>(
        dst_x, stride, left_column + (y << upsample_left_shift), -ystep,
        -ystep * x);
  }
}

// Process a multiple of 8 |width|.
template <bool upsampled_top, bool upsampled_left>
inline void DirectionalZone2_NEON(
    uint8_t* LIBGAV1_RESTRICT const dst, const ptrdiff_t stride,
    const uint16_t* LIBGAV1_RESTRICT const top_row,
    const uint16_t* LIBGAV1_RESTRICT const left_column, const int width,
    const int height, const int xstep, const int ystep) {
  if (height == 4) {
    DirectionalZone2_Wx4<upsampled_top, upsampled_left>(
        dst, stride, top_row, left_column, width, xstep, ystep);
    return;
  }
  const int upsample_top_shift = static_cast<int>(upsampled_top);

  // Helper vector.
  const int16x8_t zero_to_seven = {0, 1, 2, 3, 4, 5, 6, 7};

  const int ystep8 = ystep << 3;

  // All columns from |min_top_only_x| to the right will only need |top_row| to
  // compute and can therefore call the Zone1 functions. This assumes |xstep| is
  // at least 3.
  assert(xstep >= 3);
  const int min_top_only_x = Align(std::min((height * xstep) >> 6, width), 8);
  // Analysis finds that, for most angles (ystep < 132), all segments that use
  // both top_row and left_column can compute from left_column using byte
  // shuffles from a single vector. For steeper angles, the shuffle is also
  // fully reliable when x >= 32.
  const int shuffle_left_col_x = (ystep < 132) ? 0 : 32;
  const int min_shuffle_x = std::min(min_top_only_x, shuffle_left_col_x);

  // Offsets the original zone bound value to simplify x < (y+1)*xstep/64 -1
  int xstep_bounds_base = (xstep == 64) ? 0 : xstep - 1;

  const int left_base_increment = ystep >> 6;
  const int ystep_remainder = ystep & 0x3F;

  const int left_base_increment8 = ystep8 >> 6;
  const int ystep_remainder8 = ystep8 & 0x3F;
  const int16x8_t increment_left8 = vdupq_n_s16(ystep_remainder8);

  // If the 64 scaling is regarded as a decimal point, the first value of the
  // left_y vector omits the portion which is covered under the left_column
  // offset. Following values need the full ystep as a relative offset.
  int16x8_t left_y =
      vmlaq_n_s16(vdupq_n_s16(-ystep_remainder), zero_to_seven, -ystep);

  int x = 0;
  for (int left_offset = -left_base_increment; x < min_shuffle_x; x += 8,
           xstep_bounds_base -= (8 << 6),
           left_y = vsubq_s16(left_y, increment_left8),
           left_offset -= left_base_increment8) {
    DirectionalZone2_8xH<false, upsampled_top, upsampled_left>(
        dst, stride, top_row, left_column, height, xstep, ystep, x, left_offset,
        xstep_bounds_base, left_y);
  }
  for (int left_offset = -left_base_increment; x < min_top_only_x; x += 8,
           xstep_bounds_base -= (8 << 6),
           left_y = vsubq_s16(left_y, increment_left8),
           left_offset -= left_base_increment8) {
    DirectionalZone2_8xH<true, upsampled_top, upsampled_left>(
        dst, stride, top_row, left_column, height, xstep, ystep, x, left_offset,
        xstep_bounds_base, left_y);
  }
  // Reached |min_top_only_x|.
  if (x < width) {
    DirectionalZone1_WxH<upsampled_top>(
        reinterpret_cast<uint16_t*>(dst) + x, stride >> 1, width - x, height,
        top_row + (x << upsample_top_shift), -xstep);
  }
}

// At this angle, neither edges are upsampled.
// |min_width| is either 4 or 8.
template <int min_width>
void DirectionalAngle135(uint8_t* LIBGAV1_RESTRICT dst, const ptrdiff_t stride,
                         const uint16_t* LIBGAV1_RESTRICT const top,
                         const uint16_t* LIBGAV1_RESTRICT const left,
                         const int width, const int height) {
  // y = 0 is more trivial than the other rows.
  memcpy(dst, top - 1, width * sizeof(top[0]));
  dst += stride;

  // If |height| > |width|, then there is a point at which top_row is no longer
  // used in each row.
  const int min_left_only_y = std::min(width, height);

  int y = 1;
  do {
    // Example: If y is 4 (min_width), the dest row starts with left[3],
    // left[2], left[1], left[0], because the angle points up. Therefore, load
    // starts at left[0] and is then reversed. If y is 2, the load starts at
    // left[-2], and is reversed to store left[1], left[0], with negative values
    // overwritten from |top_row|.
    const uint16_t* const load_left = left + y - min_width;
    auto* dst16 = reinterpret_cast<uint16_t*>(dst);

    // Some values will be overwritten when |y| is not a multiple of
    // |min_width|.
    if (min_width == 4) {
      const uint16x4_t left_toward_corner = vrev64_u16(vld1_u16(load_left));
      vst1_u16(dst16, left_toward_corner);
    } else {
      int x = 0;
      do {
        const uint16x8_t left_toward_corner =
            vrev64q_u16(vld1q_u16(load_left - x));
        vst1_u16(dst16 + x, vget_high_u16(left_toward_corner));
        vst1_u16(dst16 + x + 4, vget_low_u16(left_toward_corner));
        x += 8;
      } while (x < y);
    }
    // Entering |top|.
    memcpy(dst16 + y, top - 1, (width - y) * sizeof(top[0]));
    dst += stride;
  } while (++y < min_left_only_y);

  // Left only.
  for (; y < height; ++y, dst += stride) {
    auto* dst16 = reinterpret_cast<uint16_t*>(dst);
    const uint16_t* const load_left = left + y - min_width;

    int x = 0;
    if (min_width == 4) {
      const uint16x4_t left_toward_corner = vrev64_u16(vld1_u16(load_left - x));
      vst1_u16(dst16 + x, left_toward_corner);
    } else {
      do {
        const uint16x8_t left_toward_corner =
            vrev64q_u16(vld1q_u16(load_left - x));
        vst1_u16(dst16 + x, vget_high_u16(left_toward_corner));
        vst1_u16(dst16 + x + 4, vget_low_u16(left_toward_corner));
        x += 8;
      } while (x < width);
    }
  }
}

void DirectionalIntraPredictorZone2_NEON(
    void* LIBGAV1_RESTRICT dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column, const int width,
    const int height, const int xstep, const int ystep,
    const bool upsampled_top, const bool upsampled_left) {
  // Increasing the negative buffer for this function allows more rows to be
  // processed at a time without branching in an inner loop to check the base.
  uint16_t top_buffer[288];
  uint16_t left_buffer[288];
#if LIBGAV1_MSAN
  memset(top_buffer, 0, sizeof(top_buffer));
  memset(left_buffer, 0, sizeof(left_buffer));
#endif  // LIBGAV1_MSAN
  memcpy(top_buffer + 128, static_cast<const uint16_t*>(top_row) - 16, 160);
  memcpy(left_buffer + 128, static_cast<const uint16_t*>(left_column) - 16,
         160);
  const uint16_t* top_ptr = top_buffer + 144;
  const uint16_t* left_ptr = left_buffer + 144;
  auto* dst = static_cast<uint8_t*>(dest);

  if (width == 4) {
    if (xstep == 64) {
      assert(ystep == 64);
      DirectionalAngle135<4>(dst, stride, top_ptr, left_ptr, width, height);
      return;
    }
    if (upsampled_top) {
      if (upsampled_left) {
        DirectionalZone2_4xH<true, true>(dst, stride, top_ptr, left_ptr, height,
                                         xstep, ystep);
      } else {
        DirectionalZone2_4xH<true, false>(dst, stride, top_ptr, left_ptr,
                                          height, xstep, ystep);
      }
    } else if (upsampled_left) {
      DirectionalZone2_4xH<false, true>(dst, stride, top_ptr, left_ptr, height,
                                        xstep, ystep);
    } else {
      DirectionalZone2_4xH<false, false>(dst, stride, top_ptr, left_ptr, height,
                                         xstep, ystep);
    }
    return;
  }

  if (xstep == 64) {
    assert(ystep == 64);
    DirectionalAngle135<8>(dst, stride, top_ptr, left_ptr, width, height);
    return;
  }
  if (upsampled_top) {
    if (upsampled_left) {
      DirectionalZone2_NEON<true, true>(dst, stride, top_ptr, left_ptr, width,
                                        height, xstep, ystep);
    } else {
      DirectionalZone2_NEON<true, false>(dst, stride, top_ptr, left_ptr, width,
                                         height, xstep, ystep);
    }
  } else if (upsampled_left) {
    DirectionalZone2_NEON<false, true>(dst, stride, top_ptr, left_ptr, width,
                                       height, xstep, ystep);
  } else {
    DirectionalZone2_NEON<false, false>(dst, stride, top_ptr, left_ptr, width,
                                        height, xstep, ystep);
  }
}

void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth10);
  assert(dsp != nullptr);
  dsp->directional_intra_predictor_zone1 = DirectionalIntraPredictorZone1_NEON;
  dsp->directional_intra_predictor_zone2 = DirectionalIntraPredictorZone2_NEON;
  dsp->directional_intra_predictor_zone3 = DirectionalIntraPredictorZone3_NEON;
}

}  // namespace
}  // namespace high_bitdepth
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

void IntraPredDirectionalInit_NEON() {
  low_bitdepth::Init8bpp();
#if LIBGAV1_MAX_BITDEPTH >= 10
  high_bitdepth::Init10bpp();
#endif  // LIBGAV1_MAX_BITDEPTH >= 10
}

}  // namespace dsp
}  // namespace libgav1

#else   // !LIBGAV1_ENABLE_NEON
namespace libgav1 {
namespace dsp {

void IntraPredDirectionalInit_NEON() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_ENABLE_NEON
