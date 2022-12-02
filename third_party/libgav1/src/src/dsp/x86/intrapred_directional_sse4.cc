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
#include "src/utils/cpu.h"

#if LIBGAV1_TARGETING_SSE4_1

#include <smmintrin.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/dsp/x86/common_sse4.h"
#include "src/dsp/x86/transpose_sse4.h"
#include "src/utils/common.h"
#include "src/utils/memory.h"

namespace libgav1 {
namespace dsp {
namespace low_bitdepth {
namespace {

//------------------------------------------------------------------------------
// 7.11.2.4. Directional intra prediction process

// Special case: An |xstep| of 64 corresponds to an angle delta of 45, meaning
// upsampling is ruled out. In addition, the bits masked by 0x3F for
// |shift_val| are 0 for all multiples of 64, so the formula
// val = top[top_base_x]*shift + top[top_base_x+1]*(32-shift), reduces to
// val = top[top_base_x+1] << 5, meaning only the second set of pixels is
// involved in the output. Hence |top| is offset by 1.
inline void DirectionalZone1_Step64(uint8_t* dst, ptrdiff_t stride,
                                    const uint8_t* const top, const int width,
                                    const int height) {
  ptrdiff_t offset = 1;
  if (height == 4) {
    memcpy(dst, top + offset, width);
    dst += stride;
    memcpy(dst, top + offset + 1, width);
    dst += stride;
    memcpy(dst, top + offset + 2, width);
    dst += stride;
    memcpy(dst, top + offset + 3, width);
    return;
  }
  int y = 0;
  do {
    memcpy(dst, top + offset, width);
    dst += stride;
    memcpy(dst, top + offset + 1, width);
    dst += stride;
    memcpy(dst, top + offset + 2, width);
    dst += stride;
    memcpy(dst, top + offset + 3, width);
    dst += stride;
    memcpy(dst, top + offset + 4, width);
    dst += stride;
    memcpy(dst, top + offset + 5, width);
    dst += stride;
    memcpy(dst, top + offset + 6, width);
    dst += stride;
    memcpy(dst, top + offset + 7, width);
    dst += stride;

    offset += 8;
    y += 8;
  } while (y < height);
}

inline void DirectionalZone1_4xH(uint8_t* dst, ptrdiff_t stride,
                                 const uint8_t* const top, const int height,
                                 const int xstep, const bool upsampled) {
  const int upsample_shift = static_cast<int>(upsampled);
  const int scale_bits = 6 - upsample_shift;
  const __m128i max_shift = _mm_set1_epi8(32);
  // Downscaling for a weighted average whose weights sum to 32 (max_shift).
  const int rounding_bits = 5;
  const int max_base_x = (height + 3 /* width - 1 */) << upsample_shift;
  const __m128i final_top_val = _mm_set1_epi16(top[max_base_x]);
  const __m128i sampler = upsampled ? _mm_set_epi64x(0, 0x0706050403020100)
                                    : _mm_set_epi64x(0, 0x0403030202010100);
  // Each 16-bit value here corresponds to a position that may exceed
  // |max_base_x|. When added to the top_base_x, it is used to mask values
  // that pass the end of |top|. Starting from 1 to simulate "cmpge" which is
  // not supported for packed integers.
  const __m128i offsets =
      _mm_set_epi32(0x00080007, 0x00060005, 0x00040003, 0x00020001);

  // All rows from |min_corner_only_y| down will simply use memcpy. |max_base_x|
  // is always greater than |height|, so clipping to 1 is enough to make the
  // logic work.
  const int xstep_units = std::max(xstep >> scale_bits, 1);
  const int min_corner_only_y = std::min(max_base_x / xstep_units, height);

  // Rows up to this y-value can be computed without checking for bounds.
  int y = 0;
  int top_x = xstep;

  for (; y < min_corner_only_y; ++y, dst += stride, top_x += xstep) {
    const int top_base_x = top_x >> scale_bits;

    // Permit negative values of |top_x|.
    const int shift_val = (LeftShift(top_x, upsample_shift) & 0x3F) >> 1;
    const __m128i shift = _mm_set1_epi8(shift_val);
    const __m128i opposite_shift = _mm_sub_epi8(max_shift, shift);
    const __m128i shifts = _mm_unpacklo_epi8(opposite_shift, shift);
    __m128i top_index_vect = _mm_set1_epi16(top_base_x);
    top_index_vect = _mm_add_epi16(top_index_vect, offsets);
    const __m128i max_base_x_vect = _mm_set1_epi16(max_base_x);

    // Load 8 values because we will select the sampled values based on
    // |upsampled|.
    const __m128i values = LoadLo8(top + top_base_x);
    const __m128i sampled_values = _mm_shuffle_epi8(values, sampler);
    const __m128i past_max = _mm_cmpgt_epi16(top_index_vect, max_base_x_vect);
    __m128i prod = _mm_maddubs_epi16(sampled_values, shifts);
    prod = RightShiftWithRounding_U16(prod, rounding_bits);
    // Replace pixels from invalid range with top-right corner.
    prod = _mm_blendv_epi8(prod, final_top_val, past_max);
    Store4(dst, _mm_packus_epi16(prod, prod));
  }

  // Fill in corner-only rows.
  for (; y < height; ++y) {
    memset(dst, top[max_base_x], /* width */ 4);
    dst += stride;
  }
}

// 7.11.2.4 (7) angle < 90
inline void DirectionalZone1_Large(uint8_t* dest, ptrdiff_t stride,
                                   const uint8_t* const top_row,
                                   const int width, const int height,
                                   const int xstep, const bool upsampled) {
  const int upsample_shift = static_cast<int>(upsampled);
  const __m128i sampler =
      upsampled ? _mm_set_epi32(0x0F0E0D0C, 0x0B0A0908, 0x07060504, 0x03020100)
                : _mm_set_epi32(0x08070706, 0x06050504, 0x04030302, 0x02010100);
  const int scale_bits = 6 - upsample_shift;
  const int max_base_x = ((width + height) - 1) << upsample_shift;

  const __m128i max_shift = _mm_set1_epi8(32);
  // Downscaling for a weighted average whose weights sum to 32 (max_shift).
  const int rounding_bits = 5;
  const int base_step = 1 << upsample_shift;
  const int base_step8 = base_step << 3;

  // All rows from |min_corner_only_y| down will simply use memcpy. |max_base_x|
  // is always greater than |height|, so clipping to 1 is enough to make the
  // logic work.
  const int xstep_units = std::max(xstep >> scale_bits, 1);
  const int min_corner_only_y = std::min(max_base_x / xstep_units, height);

  // Rows up to this y-value can be computed without checking for bounds.
  const int max_no_corner_y = std::min(
      LeftShift((max_base_x - (base_step * width)), scale_bits) / xstep,
      height);
  // No need to check for exceeding |max_base_x| in the first loop.
  int y = 0;
  int top_x = xstep;
  for (; y < max_no_corner_y; ++y, dest += stride, top_x += xstep) {
    int top_base_x = top_x >> scale_bits;
    // Permit negative values of |top_x|.
    const int shift_val = (LeftShift(top_x, upsample_shift) & 0x3F) >> 1;
    const __m128i shift = _mm_set1_epi8(shift_val);
    const __m128i opposite_shift = _mm_sub_epi8(max_shift, shift);
    const __m128i shifts = _mm_unpacklo_epi8(opposite_shift, shift);
    int x = 0;
    do {
      const __m128i top_vals = LoadUnaligned16(top_row + top_base_x);
      __m128i vals = _mm_shuffle_epi8(top_vals, sampler);
      vals = _mm_maddubs_epi16(vals, shifts);
      vals = RightShiftWithRounding_U16(vals, rounding_bits);
      StoreLo8(dest + x, _mm_packus_epi16(vals, vals));
      top_base_x += base_step8;
      x += 8;
    } while (x < width);
  }

  // Each 16-bit value here corresponds to a position that may exceed
  // |max_base_x|. When added to the top_base_x, it is used to mask values
  // that pass the end of |top|. Starting from 1 to simulate "cmpge" which is
  // not supported for packed integers.
  const __m128i offsets =
      _mm_set_epi32(0x00080007, 0x00060005, 0x00040003, 0x00020001);

  const __m128i max_base_x_vect = _mm_set1_epi16(max_base_x);
  const __m128i final_top_val = _mm_set1_epi16(top_row[max_base_x]);
  const __m128i base_step8_vect = _mm_set1_epi16(base_step8);
  for (; y < min_corner_only_y; ++y, dest += stride, top_x += xstep) {
    int top_base_x = top_x >> scale_bits;

    const int shift_val = (LeftShift(top_x, upsample_shift) & 0x3F) >> 1;
    const __m128i shift = _mm_set1_epi8(shift_val);
    const __m128i opposite_shift = _mm_sub_epi8(max_shift, shift);
    const __m128i shifts = _mm_unpacklo_epi8(opposite_shift, shift);
    __m128i top_index_vect = _mm_set1_epi16(top_base_x);
    top_index_vect = _mm_add_epi16(top_index_vect, offsets);

    int x = 0;
    const int min_corner_only_x =
        std::min(width, ((max_base_x - top_base_x) >> upsample_shift) + 7) & ~7;
    for (; x < min_corner_only_x;
         x += 8, top_base_x += base_step8,
         top_index_vect = _mm_add_epi16(top_index_vect, base_step8_vect)) {
      const __m128i past_max = _mm_cmpgt_epi16(top_index_vect, max_base_x_vect);
      // Assuming a buffer zone of 8 bytes at the end of top_row, this prevents
      // reading out of bounds. If all indices are past max and we don't need to
      // use the loaded bytes at all, |top_base_x| becomes 0. |top_base_x| will
      // reset for the next |y|.
      top_base_x &= ~_mm_cvtsi128_si32(past_max);
      const __m128i top_vals = LoadUnaligned16(top_row + top_base_x);
      __m128i vals = _mm_shuffle_epi8(top_vals, sampler);
      vals = _mm_maddubs_epi16(vals, shifts);
      vals = RightShiftWithRounding_U16(vals, rounding_bits);
      vals = _mm_blendv_epi8(vals, final_top_val, past_max);
      StoreLo8(dest + x, _mm_packus_epi16(vals, vals));
    }
    // Corner-only section of the row.
    memset(dest + x, top_row[max_base_x], width - x);
  }
  // Fill in corner-only rows.
  for (; y < height; ++y) {
    memset(dest, top_row[max_base_x], width);
    dest += stride;
  }
}

// 7.11.2.4 (7) angle < 90
inline void DirectionalZone1_SSE4_1(uint8_t* dest, ptrdiff_t stride,
                                    const uint8_t* const top_row,
                                    const int width, const int height,
                                    const int xstep, const bool upsampled) {
  const int upsample_shift = static_cast<int>(upsampled);
  if (xstep == 64) {
    DirectionalZone1_Step64(dest, stride, top_row, width, height);
    return;
  }
  if (width == 4) {
    DirectionalZone1_4xH(dest, stride, top_row, height, xstep, upsampled);
    return;
  }
  if (width >= 32) {
    DirectionalZone1_Large(dest, stride, top_row, width, height, xstep,
                           upsampled);
    return;
  }
  const __m128i sampler =
      upsampled ? _mm_set_epi32(0x0F0E0D0C, 0x0B0A0908, 0x07060504, 0x03020100)
                : _mm_set_epi32(0x08070706, 0x06050504, 0x04030302, 0x02010100);
  const int scale_bits = 6 - upsample_shift;
  const int max_base_x = ((width + height) - 1) << upsample_shift;

  const __m128i max_shift = _mm_set1_epi8(32);
  // Downscaling for a weighted average whose weights sum to 32 (max_shift).
  const int rounding_bits = 5;
  const int base_step = 1 << upsample_shift;
  const int base_step8 = base_step << 3;

  // No need to check for exceeding |max_base_x| in the loops.
  if (((xstep * height) >> scale_bits) + base_step * width < max_base_x) {
    int top_x = xstep;
    int y = 0;
    do {
      int top_base_x = top_x >> scale_bits;
      // Permit negative values of |top_x|.
      const int shift_val = (LeftShift(top_x, upsample_shift) & 0x3F) >> 1;
      const __m128i shift = _mm_set1_epi8(shift_val);
      const __m128i opposite_shift = _mm_sub_epi8(max_shift, shift);
      const __m128i shifts = _mm_unpacklo_epi8(opposite_shift, shift);
      int x = 0;
      do {
        const __m128i top_vals = LoadUnaligned16(top_row + top_base_x);
        __m128i vals = _mm_shuffle_epi8(top_vals, sampler);
        vals = _mm_maddubs_epi16(vals, shifts);
        vals = RightShiftWithRounding_U16(vals, rounding_bits);
        StoreLo8(dest + x, _mm_packus_epi16(vals, vals));
        top_base_x += base_step8;
        x += 8;
      } while (x < width);
      dest += stride;
      top_x += xstep;
    } while (++y < height);
    return;
  }

  // Each 16-bit value here corresponds to a position that may exceed
  // |max_base_x|. When added to the top_base_x, it is used to mask values
  // that pass the end of |top|. Starting from 1 to simulate "cmpge" which is
  // not supported for packed integers.
  const __m128i offsets =
      _mm_set_epi32(0x00080007, 0x00060005, 0x00040003, 0x00020001);

  const __m128i max_base_x_vect = _mm_set1_epi16(max_base_x);
  const __m128i final_top_val = _mm_set1_epi16(top_row[max_base_x]);
  const __m128i base_step8_vect = _mm_set1_epi16(base_step8);
  int top_x = xstep;
  int y = 0;
  do {
    int top_base_x = top_x >> scale_bits;

    if (top_base_x >= max_base_x) {
      for (int i = y; i < height; ++i) {
        memset(dest, top_row[max_base_x], width);
        dest += stride;
      }
      return;
    }

    const int shift_val = (LeftShift(top_x, upsample_shift) & 0x3F) >> 1;
    const __m128i shift = _mm_set1_epi8(shift_val);
    const __m128i opposite_shift = _mm_sub_epi8(max_shift, shift);
    const __m128i shifts = _mm_unpacklo_epi8(opposite_shift, shift);
    __m128i top_index_vect = _mm_set1_epi16(top_base_x);
    top_index_vect = _mm_add_epi16(top_index_vect, offsets);

    int x = 0;
    for (; x < width - 8;
         x += 8, top_base_x += base_step8,
         top_index_vect = _mm_add_epi16(top_index_vect, base_step8_vect)) {
      const __m128i past_max = _mm_cmpgt_epi16(top_index_vect, max_base_x_vect);
      // Assuming a buffer zone of 8 bytes at the end of top_row, this prevents
      // reading out of bounds. If all indices are past max and we don't need to
      // use the loaded bytes at all, |top_base_x| becomes 0. |top_base_x| will
      // reset for the next |y|.
      top_base_x &= ~_mm_cvtsi128_si32(past_max);
      const __m128i top_vals = LoadUnaligned16(top_row + top_base_x);
      __m128i vals = _mm_shuffle_epi8(top_vals, sampler);
      vals = _mm_maddubs_epi16(vals, shifts);
      vals = RightShiftWithRounding_U16(vals, rounding_bits);
      vals = _mm_blendv_epi8(vals, final_top_val, past_max);
      StoreLo8(dest + x, _mm_packus_epi16(vals, vals));
    }
    const __m128i past_max = _mm_cmpgt_epi16(top_index_vect, max_base_x_vect);
    __m128i vals;
    if (upsampled) {
      vals = LoadUnaligned16(top_row + top_base_x);
    } else {
      const __m128i top_vals = LoadLo8(top_row + top_base_x);
      vals = _mm_shuffle_epi8(top_vals, sampler);
      vals = _mm_insert_epi8(vals, top_row[top_base_x + 8], 15);
    }
    vals = _mm_maddubs_epi16(vals, shifts);
    vals = RightShiftWithRounding_U16(vals, rounding_bits);
    vals = _mm_blendv_epi8(vals, final_top_val, past_max);
    StoreLo8(dest + x, _mm_packus_epi16(vals, vals));
    dest += stride;
    top_x += xstep;
  } while (++y < height);
}

void DirectionalIntraPredictorZone1_SSE4_1(void* const dest, ptrdiff_t stride,
                                           const void* const top_row,
                                           const int width, const int height,
                                           const int xstep,
                                           const bool upsampled_top) {
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  auto* dst = static_cast<uint8_t*>(dest);
  DirectionalZone1_SSE4_1(dst, stride, top_ptr, width, height, xstep,
                          upsampled_top);
}

template <bool upsampled>
inline void DirectionalZone3_4x4(uint8_t* dest, ptrdiff_t stride,
                                 const uint8_t* const left_column,
                                 const int base_left_y, const int ystep) {
  // For use in the non-upsampled case.
  const __m128i sampler = _mm_set_epi64x(0, 0x0403030202010100);
  const int upsample_shift = static_cast<int>(upsampled);
  const int scale_bits = 6 - upsample_shift;
  const __m128i max_shift = _mm_set1_epi8(32);
  // Downscaling for a weighted average whose weights sum to 32 (max_shift).
  const int rounding_bits = 5;

  __m128i result_block[4];
  for (int x = 0, left_y = base_left_y; x < 4; x++, left_y += ystep) {
    const int left_base_y = left_y >> scale_bits;
    const int shift_val = ((left_y << upsample_shift) & 0x3F) >> 1;
    const __m128i shift = _mm_set1_epi8(shift_val);
    const __m128i opposite_shift = _mm_sub_epi8(max_shift, shift);
    const __m128i shifts = _mm_unpacklo_epi8(opposite_shift, shift);
    __m128i vals;
    if (upsampled) {
      vals = LoadLo8(left_column + left_base_y);
    } else {
      const __m128i top_vals = LoadLo8(left_column + left_base_y);
      vals = _mm_shuffle_epi8(top_vals, sampler);
    }
    vals = _mm_maddubs_epi16(vals, shifts);
    vals = RightShiftWithRounding_U16(vals, rounding_bits);
    result_block[x] = _mm_packus_epi16(vals, vals);
  }
  const __m128i result = Transpose4x4_U8(result_block);
  // This is result_row0.
  Store4(dest, result);
  dest += stride;
  const int result_row1 = _mm_extract_epi32(result, 1);
  memcpy(dest, &result_row1, sizeof(result_row1));
  dest += stride;
  const int result_row2 = _mm_extract_epi32(result, 2);
  memcpy(dest, &result_row2, sizeof(result_row2));
  dest += stride;
  const int result_row3 = _mm_extract_epi32(result, 3);
  memcpy(dest, &result_row3, sizeof(result_row3));
}

template <bool upsampled, int height>
inline void DirectionalZone3_8xH(uint8_t* dest, ptrdiff_t stride,
                                 const uint8_t* const left_column,
                                 const int base_left_y, const int ystep) {
  // For use in the non-upsampled case.
  const __m128i sampler =
      _mm_set_epi64x(0x0807070606050504, 0x0403030202010100);
  const int upsample_shift = static_cast<int>(upsampled);
  const int scale_bits = 6 - upsample_shift;
  const __m128i max_shift = _mm_set1_epi8(32);
  // Downscaling for a weighted average whose weights sum to 32 (max_shift).
  const int rounding_bits = 5;

  __m128i result_block[8];
  for (int x = 0, left_y = base_left_y; x < 8; x++, left_y += ystep) {
    const int left_base_y = left_y >> scale_bits;
    const int shift_val = (LeftShift(left_y, upsample_shift) & 0x3F) >> 1;
    const __m128i shift = _mm_set1_epi8(shift_val);
    const __m128i opposite_shift = _mm_sub_epi8(max_shift, shift);
    const __m128i shifts = _mm_unpacklo_epi8(opposite_shift, shift);
    __m128i vals;
    if (upsampled) {
      vals = LoadUnaligned16(left_column + left_base_y);
    } else {
      const __m128i top_vals = LoadUnaligned16(left_column + left_base_y);
      vals = _mm_shuffle_epi8(top_vals, sampler);
    }
    vals = _mm_maddubs_epi16(vals, shifts);
    result_block[x] = RightShiftWithRounding_U16(vals, rounding_bits);
  }
  Transpose8x8_U16(result_block, result_block);
  for (int y = 0; y < height; ++y) {
    StoreLo8(dest, _mm_packus_epi16(result_block[y], result_block[y]));
    dest += stride;
  }
}

// 7.11.2.4 (9) angle > 180
void DirectionalIntraPredictorZone3_SSE4_1(void* dest, ptrdiff_t stride,
                                           const void* const left_column,
                                           const int width, const int height,
                                           const int ystep,
                                           const bool upsampled) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  auto* dst = static_cast<uint8_t*>(dest);
  const int upsample_shift = static_cast<int>(upsampled);
  if (width == 4 || height == 4) {
    const ptrdiff_t stride4 = stride << 2;
    if (upsampled) {
      int left_y = ystep;
      int x = 0;
      do {
        uint8_t* dst_x = dst + x;
        int y = 0;
        do {
          DirectionalZone3_4x4<true>(
              dst_x, stride, left_ptr + (y << upsample_shift), left_y, ystep);
          dst_x += stride4;
          y += 4;
        } while (y < height);
        left_y += ystep << 2;
        x += 4;
      } while (x < width);
    } else {
      int left_y = ystep;
      int x = 0;
      do {
        uint8_t* dst_x = dst + x;
        int y = 0;
        do {
          DirectionalZone3_4x4<false>(dst_x, stride, left_ptr + y, left_y,
                                      ystep);
          dst_x += stride4;
          y += 4;
        } while (y < height);
        left_y += ystep << 2;
        x += 4;
      } while (x < width);
    }
    return;
  }

  const ptrdiff_t stride8 = stride << 3;
  if (upsampled) {
    int left_y = ystep;
    int x = 0;
    do {
      uint8_t* dst_x = dst + x;
      int y = 0;
      do {
        DirectionalZone3_8xH<true, 8>(
            dst_x, stride, left_ptr + (y << upsample_shift), left_y, ystep);
        dst_x += stride8;
        y += 8;
      } while (y < height);
      left_y += ystep << 3;
      x += 8;
    } while (x < width);
  } else {
    int left_y = ystep;
    int x = 0;
    do {
      uint8_t* dst_x = dst + x;
      int y = 0;
      do {
        DirectionalZone3_8xH<false, 8>(
            dst_x, stride, left_ptr + (y << upsample_shift), left_y, ystep);
        dst_x += stride8;
        y += 8;
      } while (y < height);
      left_y += ystep << 3;
      x += 8;
    } while (x < width);
  }
}

//------------------------------------------------------------------------------
// Directional Zone 2 Functions
// 7.11.2.4 (8)

// DirectionalBlend* selectively overwrites the values written by
// DirectionalZone2FromLeftCol*. |zone_bounds| has one 16-bit index for each
// row.
template <int y_selector>
inline void DirectionalBlend4_SSE4_1(uint8_t* dest,
                                     const __m128i& dest_index_vect,
                                     const __m128i& vals,
                                     const __m128i& zone_bounds) {
  const __m128i max_dest_x_vect = _mm_shufflelo_epi16(zone_bounds, y_selector);
  const __m128i use_left = _mm_cmplt_epi16(dest_index_vect, max_dest_x_vect);
  const __m128i original_vals = _mm_cvtepu8_epi16(Load4(dest));
  const __m128i blended_vals = _mm_blendv_epi8(vals, original_vals, use_left);
  Store4(dest, _mm_packus_epi16(blended_vals, blended_vals));
}

inline void DirectionalBlend8_SSE4_1(uint8_t* dest,
                                     const __m128i& dest_index_vect,
                                     const __m128i& vals,
                                     const __m128i& zone_bounds,
                                     const __m128i& bounds_selector) {
  const __m128i max_dest_x_vect =
      _mm_shuffle_epi8(zone_bounds, bounds_selector);
  const __m128i use_left = _mm_cmplt_epi16(dest_index_vect, max_dest_x_vect);
  const __m128i original_vals = _mm_cvtepu8_epi16(LoadLo8(dest));
  const __m128i blended_vals = _mm_blendv_epi8(vals, original_vals, use_left);
  StoreLo8(dest, _mm_packus_epi16(blended_vals, blended_vals));
}

constexpr int kDirectionalWeightBits = 5;
// |source| is packed with 4 or 8 pairs of 8-bit values from left or top.
// |shifts| is named to match the specification, with 4 or 8 pairs of (32 -
// shift) and shift. Shift is guaranteed to be between 0 and 32.
inline __m128i DirectionalZone2FromSource_SSE4_1(const uint8_t* const source,
                                                 const __m128i& shifts,
                                                 const __m128i& sampler) {
  const __m128i src_vals = LoadUnaligned16(source);
  __m128i vals = _mm_shuffle_epi8(src_vals, sampler);
  vals = _mm_maddubs_epi16(vals, shifts);
  return RightShiftWithRounding_U16(vals, kDirectionalWeightBits);
}

// Because the source values "move backwards" as the row index increases, the
// indices derived from ystep are generally negative. This is accommodated by
// making sure the relative indices are within [-15, 0] when the function is
// called, and sliding them into the inclusive range [0, 15], relative to a
// lower base address.
constexpr int kPositiveIndexOffset = 15;

template <bool upsampled>
inline void DirectionalZone2FromLeftCol_4x4_SSE4_1(
    uint8_t* dst, ptrdiff_t stride, const uint8_t* const left_column_base,
    __m128i left_y) {
  const int upsample_shift = static_cast<int>(upsampled);
  const int scale_bits = 6 - upsample_shift;
  const __m128i max_shifts = _mm_set1_epi8(32);
  const __m128i shift_mask = _mm_set1_epi32(0x003F003F);
  const __m128i index_increment = _mm_cvtsi32_si128(0x01010101);
  const __m128i positive_offset = _mm_set1_epi8(kPositiveIndexOffset);
  // Left_column and sampler are both offset by 15 so the indices are always
  // positive.
  const uint8_t* left_column = left_column_base - kPositiveIndexOffset;
  for (int y = 0; y < 4; dst += stride, ++y) {
    __m128i offset_y = _mm_srai_epi16(left_y, scale_bits);
    offset_y = _mm_packs_epi16(offset_y, offset_y);

    const __m128i adjacent = _mm_add_epi8(offset_y, index_increment);
    __m128i sampler = _mm_unpacklo_epi8(offset_y, adjacent);
    // Slide valid |offset_y| indices from range [-15, 0] to [0, 15] so they
    // can work as shuffle indices. Some values may be out of bounds, but their
    // pred results will be masked over by top prediction.
    sampler = _mm_add_epi8(sampler, positive_offset);

    __m128i shifts = _mm_srli_epi16(
        _mm_and_si128(_mm_slli_epi16(left_y, upsample_shift), shift_mask), 1);
    shifts = _mm_packus_epi16(shifts, shifts);
    const __m128i opposite_shifts = _mm_sub_epi8(max_shifts, shifts);
    shifts = _mm_unpacklo_epi8(opposite_shifts, shifts);
    const __m128i vals = DirectionalZone2FromSource_SSE4_1(
        left_column + (y << upsample_shift), shifts, sampler);
    Store4(dst, _mm_packus_epi16(vals, vals));
  }
}

template <bool upsampled>
inline void DirectionalZone2FromLeftCol_8x8_SSE4_1(
    uint8_t* dst, ptrdiff_t stride, const uint8_t* const left_column,
    __m128i left_y) {
  const int upsample_shift = static_cast<int>(upsampled);
  const int scale_bits = 6 - upsample_shift;
  const __m128i max_shifts = _mm_set1_epi8(32);
  const __m128i shift_mask = _mm_set1_epi32(0x003F003F);
  const __m128i index_increment = _mm_set1_epi8(1);
  const __m128i denegation = _mm_set1_epi8(kPositiveIndexOffset);
  for (int y = 0; y < 8; dst += stride, ++y) {
    __m128i offset_y = _mm_srai_epi16(left_y, scale_bits);
    offset_y = _mm_packs_epi16(offset_y, offset_y);
    const __m128i adjacent = _mm_add_epi8(offset_y, index_increment);

    // Offset the relative index because ystep is negative in Zone 2 and shuffle
    // indices must be nonnegative.
    __m128i sampler = _mm_unpacklo_epi8(offset_y, adjacent);
    sampler = _mm_add_epi8(sampler, denegation);

    __m128i shifts = _mm_srli_epi16(
        _mm_and_si128(_mm_slli_epi16(left_y, upsample_shift), shift_mask), 1);
    shifts = _mm_packus_epi16(shifts, shifts);
    const __m128i opposite_shifts = _mm_sub_epi8(max_shifts, shifts);
    shifts = _mm_unpacklo_epi8(opposite_shifts, shifts);

    // The specification adds (y << 6) to left_y, which is subject to
    // upsampling, but this puts sampler indices out of the 0-15 range. It is
    // equivalent to offset the source address by (y << upsample_shift) instead.
    const __m128i vals = DirectionalZone2FromSource_SSE4_1(
        left_column - kPositiveIndexOffset + (y << upsample_shift), shifts,
        sampler);
    StoreLo8(dst, _mm_packus_epi16(vals, vals));
  }
}

// |zone_bounds| is an epi16 of the relative x index at which base >= -(1 <<
// upsampled_top), for each row. When there are 4 values, they can be duplicated
// with a non-register shuffle mask.
// |shifts| is one pair of weights that applies throughout a given row.
template <bool upsampled_top>
inline void DirectionalZone1Blend_4x4(
    uint8_t* dest, const uint8_t* const top_row, ptrdiff_t stride,
    __m128i sampler, const __m128i& zone_bounds, const __m128i& shifts,
    const __m128i& dest_index_x, int top_x, const int xstep) {
  const int upsample_shift = static_cast<int>(upsampled_top);
  const int scale_bits_x = 6 - upsample_shift;
  top_x -= xstep;

  int top_base_x = (top_x >> scale_bits_x);
  const __m128i vals0 = DirectionalZone2FromSource_SSE4_1(
      top_row + top_base_x, _mm_shufflelo_epi16(shifts, 0x00), sampler);
  DirectionalBlend4_SSE4_1<0x00>(dest, dest_index_x, vals0, zone_bounds);
  top_x -= xstep;
  dest += stride;

  top_base_x = (top_x >> scale_bits_x);
  const __m128i vals1 = DirectionalZone2FromSource_SSE4_1(
      top_row + top_base_x, _mm_shufflelo_epi16(shifts, 0x55), sampler);
  DirectionalBlend4_SSE4_1<0x55>(dest, dest_index_x, vals1, zone_bounds);
  top_x -= xstep;
  dest += stride;

  top_base_x = (top_x >> scale_bits_x);
  const __m128i vals2 = DirectionalZone2FromSource_SSE4_1(
      top_row + top_base_x, _mm_shufflelo_epi16(shifts, 0xAA), sampler);
  DirectionalBlend4_SSE4_1<0xAA>(dest, dest_index_x, vals2, zone_bounds);
  top_x -= xstep;
  dest += stride;

  top_base_x = (top_x >> scale_bits_x);
  const __m128i vals3 = DirectionalZone2FromSource_SSE4_1(
      top_row + top_base_x, _mm_shufflelo_epi16(shifts, 0xFF), sampler);
  DirectionalBlend4_SSE4_1<0xFF>(dest, dest_index_x, vals3, zone_bounds);
}

template <bool upsampled_top, int height>
inline void DirectionalZone1Blend_8xH(
    uint8_t* dest, const uint8_t* const top_row, ptrdiff_t stride,
    __m128i sampler, const __m128i& zone_bounds, const __m128i& shifts,
    const __m128i& dest_index_x, int top_x, const int xstep) {
  const int upsample_shift = static_cast<int>(upsampled_top);
  const int scale_bits_x = 6 - upsample_shift;

  __m128i y_selector = _mm_set1_epi32(0x01000100);
  const __m128i index_increment = _mm_set1_epi32(0x02020202);
  for (int y = 0; y < height; ++y,
           y_selector = _mm_add_epi8(y_selector, index_increment),
           dest += stride) {
    top_x -= xstep;
    const int top_base_x = top_x >> scale_bits_x;
    const __m128i vals = DirectionalZone2FromSource_SSE4_1(
        top_row + top_base_x, _mm_shuffle_epi8(shifts, y_selector), sampler);
    DirectionalBlend8_SSE4_1(dest, dest_index_x, vals, zone_bounds, y_selector);
  }
}

template <bool shuffle_left_column, bool upsampled_left, bool upsampled_top>
inline void DirectionalZone2_8xH(
    uint8_t* LIBGAV1_RESTRICT const dst, const ptrdiff_t stride,
    const uint8_t* LIBGAV1_RESTRICT const top_row,
    const uint8_t* LIBGAV1_RESTRICT const left_column, const int height,
    const int xstep, const int ystep, const int x, const int left_offset,
    const __m128i& xstep_for_shift, const __m128i& xstep_bounds_base,
    const __m128i& left_y) {
  const int upsample_left_shift = static_cast<int>(upsampled_left);
  const int upsample_top_shift = static_cast<int>(upsampled_top);

  // Loop incrementers for moving by block (8x8). This function handles blocks
  // with height 4 as well. They are calculated in one pass so these variables
  // do not get used.
  const ptrdiff_t stride8 = stride << 3;
  const int xstep8 = xstep << 3;
  const __m128i xstep8_vect = _mm_set1_epi16(xstep8);

  // Cover 8x4 case.
  const int min_height = (height == 4) ? 4 : 8;

  // The first stage, before the first y-loop, covers blocks that are only
  // computed from the top row. The second stage, comprising two y-loops, covers
  // blocks that have a mixture of values computed from top or left. The final
  // stage covers blocks that are only computed from the left.
  uint8_t* dst_x = dst + x;

  // Round down to the nearest multiple of 8 (or 4, if height is 4).
  const int max_top_only_y =
      std::min(((x + 1) << 6) / xstep, height) & ~(min_height - 1);
  DirectionalZone1_4xH(dst_x, stride, top_row + (x << upsample_top_shift),
                       max_top_only_y, -xstep, upsampled_top);
  DirectionalZone1_4xH(dst_x + 4, stride,
                       top_row + ((x + 4) << upsample_top_shift),
                       max_top_only_y, -xstep, upsampled_top);
  if (max_top_only_y == height) return;

  const __m128i max_shift = _mm_set1_epi8(32);
  const __m128i shift_mask = _mm_set1_epi32(0x003F003F);
  const __m128i dest_index_x =
      _mm_set_epi32(0x00070006, 0x00050004, 0x00030002, 0x00010000);
  const __m128i sampler_top =
      upsampled_top
          ? _mm_set_epi32(0x0F0E0D0C, 0x0B0A0908, 0x07060504, 0x03020100)
          : _mm_set_epi32(0x08070706, 0x06050504, 0x04030302, 0x02010100);
  int y = max_top_only_y;
  dst_x += stride * y;
  const int xstep_y = xstep * y;
  const __m128i xstep_y_vect = _mm_set1_epi16(xstep_y);
  // All rows from |min_left_only_y| down for this set of columns, only need
  // |left_column| to compute.
  const int min_left_only_y =
      Align(std::min(((x + 8) << 6) / xstep, height), 8);

  __m128i xstep_bounds = _mm_add_epi16(xstep_bounds_base, xstep_y_vect);
  __m128i xstep_for_shift_y = _mm_sub_epi16(xstep_for_shift, xstep_y_vect);
  int top_x = -xstep_y;

  const auto base_left_y = static_cast<int16_t>(_mm_extract_epi16(left_y, 0));
  for (; y < min_left_only_y;
       y += 8, dst_x += stride8,
       xstep_bounds = _mm_add_epi16(xstep_bounds, xstep8_vect),
       xstep_for_shift_y = _mm_sub_epi16(xstep_for_shift_y, xstep8_vect),
       top_x -= xstep8) {
    // Pick up from the last y-value, using the 10% slower but secure method for
    // left prediction.
    if (shuffle_left_column) {
      DirectionalZone2FromLeftCol_8x8_SSE4_1<upsampled_left>(
          dst_x, stride,
          left_column + ((left_offset + y) << upsample_left_shift), left_y);
    } else {
      DirectionalZone3_8xH<upsampled_left, 8>(
          dst_x, stride,
          left_column + ((left_offset + y) << upsample_left_shift), base_left_y,
          -ystep);
    }

    __m128i shifts = _mm_srli_epi16(
        _mm_and_si128(_mm_slli_epi16(xstep_for_shift_y, upsample_top_shift),
                      shift_mask),
        1);
    shifts = _mm_packus_epi16(shifts, shifts);
    __m128i opposite_shifts = _mm_sub_epi8(max_shift, shifts);
    shifts = _mm_unpacklo_epi8(opposite_shifts, shifts);
    __m128i xstep_bounds_off = _mm_srai_epi16(xstep_bounds, 6);
    DirectionalZone1Blend_8xH<upsampled_top, 8>(
        dst_x, top_row + (x << upsample_top_shift), stride, sampler_top,
        xstep_bounds_off, shifts, dest_index_x, top_x, xstep);
  }
  // Loop over y for left_only rows.
  for (; y < height; y += 8, dst_x += stride8) {
    DirectionalZone3_8xH<upsampled_left, 8>(
        dst_x, stride, left_column + ((left_offset + y) << upsample_left_shift),
        base_left_y, -ystep);
  }
}

// 7.11.2.4 (8) 90 < angle > 180
// The strategy for this function is to know how many blocks can be processed
// with just pixels from |top_ptr|, then handle mixed blocks, then handle only
// blocks that take from |left_ptr|. Additionally, a fast index-shuffle
// approach is used for pred values from |left_column| in sections that permit
// it.
template <bool upsampled_left, bool upsampled_top>
inline void DirectionalZone2_SSE4_1(void* dest, ptrdiff_t stride,
                                    const uint8_t* const top_row,
                                    const uint8_t* const left_column,
                                    const int width, const int height,
                                    const int xstep, const int ystep) {
  auto* dst = static_cast<uint8_t*>(dest);
  const int upsample_top_shift = static_cast<int>(upsampled_top);
  // All columns from |min_top_only_x| to the right will only need |top_row|
  // to compute. This assumes minimum |xstep| is 3.
  const int min_top_only_x = std::min((height * xstep) >> 6, width);

  // Accumulate xstep across 8 rows.
  const __m128i xstep_dup = _mm_set1_epi16(-xstep);
  const __m128i increments = _mm_set_epi16(8, 7, 6, 5, 4, 3, 2, 1);
  const __m128i xstep_for_shift = _mm_mullo_epi16(xstep_dup, increments);
  // Offsets the original zone bound value to simplify x < (y+1)*xstep/64 -1
  const __m128i scaled_one = _mm_set1_epi16(-64);
  __m128i xstep_bounds_base =
      (xstep == 64) ? _mm_sub_epi16(scaled_one, xstep_for_shift)
                    : _mm_sub_epi16(_mm_set1_epi16(-1), xstep_for_shift);

  const int left_base_increment = ystep >> 6;
  const int ystep_remainder = ystep & 0x3F;
  const int ystep8 = ystep << 3;
  const int left_base_increment8 = ystep8 >> 6;
  const int ystep_remainder8 = ystep8 & 0x3F;
  const __m128i increment_left8 = _mm_set1_epi16(-ystep_remainder8);

  // If the 64 scaling is regarded as a decimal point, the first value of the
  // left_y vector omits the portion which is covered under the left_column
  // offset. Following values need the full ystep as a relative offset.
  const __m128i ystep_init = _mm_set1_epi16(-ystep_remainder);
  const __m128i ystep_dup = _mm_set1_epi16(-ystep);
  const __m128i dest_index_x =
      _mm_set_epi32(0x00070006, 0x00050004, 0x00030002, 0x00010000);
  __m128i left_y = _mm_mullo_epi16(ystep_dup, dest_index_x);
  left_y = _mm_add_epi16(ystep_init, left_y);

  // Analysis finds that, for most angles (ystep < 132), all segments that use
  // both top_row and left_column can compute from left_column using byte
  // shuffles from a single vector. For steeper angles, the shuffle is also
  // fully reliable when x >= 32.
  const int shuffle_left_col_x = (ystep < 132) ? 0 : 32;
  const int min_shuffle_x = std::min(min_top_only_x, shuffle_left_col_x);
  const __m128i increment_top8 = _mm_set1_epi16(8 << 6);
  int x = 0;

  for (int left_offset = -left_base_increment; x < min_shuffle_x;
       x += 8,
           xstep_bounds_base = _mm_sub_epi16(xstep_bounds_base, increment_top8),
           // Watch left_y because it can still get big.
       left_y = _mm_add_epi16(left_y, increment_left8),
           left_offset -= left_base_increment8) {
    DirectionalZone2_8xH<false, upsampled_left, upsampled_top>(
        dst, stride, top_row, left_column, height, xstep, ystep, x, left_offset,
        xstep_for_shift, xstep_bounds_base, left_y);
  }
  for (int left_offset = -left_base_increment; x < min_top_only_x;
       x += 8,
           xstep_bounds_base = _mm_sub_epi16(xstep_bounds_base, increment_top8),
           // Watch left_y because it can still get big.
       left_y = _mm_add_epi16(left_y, increment_left8),
           left_offset -= left_base_increment8) {
    DirectionalZone2_8xH<true, upsampled_left, upsampled_top>(
        dst, stride, top_row, left_column, height, xstep, ystep, x, left_offset,
        xstep_for_shift, xstep_bounds_base, left_y);
  }
  for (; x < width; x += 4) {
    DirectionalZone1_4xH(dst + x, stride, top_row + (x << upsample_top_shift),
                         height, -xstep, upsampled_top);
  }
}

template <bool upsampled_left, bool upsampled_top>
inline void DirectionalZone2_4_SSE4_1(void* dest, ptrdiff_t stride,
                                      const uint8_t* const top_row,
                                      const uint8_t* const left_column,
                                      const int width, const int height,
                                      const int xstep, const int ystep) {
  auto* dst = static_cast<uint8_t*>(dest);
  const int upsample_left_shift = static_cast<int>(upsampled_left);
  const int upsample_top_shift = static_cast<int>(upsampled_top);
  const __m128i max_shift = _mm_set1_epi8(32);
  const ptrdiff_t stride4 = stride << 2;
  const __m128i dest_index_x = _mm_set_epi32(0, 0, 0x00030002, 0x00010000);
  const __m128i sampler_top =
      upsampled_top
          ? _mm_set_epi32(0x0F0E0D0C, 0x0B0A0908, 0x07060504, 0x03020100)
          : _mm_set_epi32(0x08070706, 0x06050504, 0x04030302, 0x02010100);
  // All columns from |min_top_only_x| to the right will only need |top_row| to
  // compute.
  assert(xstep >= 3);
  const int min_top_only_x = std::min((height * xstep) >> 6, width);

  const int xstep4 = xstep << 2;
  const __m128i xstep4_vect = _mm_set1_epi16(xstep4);
  const __m128i xstep_dup = _mm_set1_epi16(-xstep);
  const __m128i increments = _mm_set_epi32(0, 0, 0x00040003, 0x00020001);
  __m128i xstep_for_shift = _mm_mullo_epi16(xstep_dup, increments);
  const __m128i scaled_one = _mm_set1_epi16(-64);
  // Offsets the original zone bound value to simplify x < (y+1)*xstep/64 -1
  __m128i xstep_bounds_base =
      (xstep == 64) ? _mm_sub_epi16(scaled_one, xstep_for_shift)
                    : _mm_sub_epi16(_mm_set1_epi16(-1), xstep_for_shift);

  const int left_base_increment = ystep >> 6;
  const int ystep_remainder = ystep & 0x3F;
  const int ystep4 = ystep << 2;
  const int left_base_increment4 = ystep4 >> 6;
  // This is guaranteed to be less than 64, but accumulation may bring it past
  // 64 for higher x values.
  const int ystep_remainder4 = ystep4 & 0x3F;
  const __m128i increment_left4 = _mm_set1_epi16(-ystep_remainder4);
  const __m128i increment_top4 = _mm_set1_epi16(4 << 6);

  // If the 64 scaling is regarded as a decimal point, the first value of the
  // left_y vector omits the portion which will go into the left_column offset.
  // Following values need the full ystep as a relative offset.
  const __m128i ystep_init = _mm_set1_epi16(-ystep_remainder);
  const __m128i ystep_dup = _mm_set1_epi16(-ystep);
  __m128i left_y = _mm_mullo_epi16(ystep_dup, dest_index_x);
  left_y = _mm_add_epi16(ystep_init, left_y);
  const __m128i shift_mask = _mm_set1_epi32(0x003F003F);

  int x = 0;
  // Loop over x for columns with a mixture of sources.
  for (int left_offset = -left_base_increment; x < min_top_only_x; x += 4,
           xstep_bounds_base = _mm_sub_epi16(xstep_bounds_base, increment_top4),
           left_y = _mm_add_epi16(left_y, increment_left4),
           left_offset -= left_base_increment4) {
    uint8_t* dst_x = dst + x;

    // Round down to the nearest multiple of 4.
    const int max_top_only_y = std::min((x << 6) / xstep, height) & ~3;
    DirectionalZone1_4xH(dst_x, stride, top_row + (x << upsample_top_shift),
                         max_top_only_y, -xstep, upsampled_top);
    int y = max_top_only_y;
    dst_x += stride * y;
    const int xstep_y = xstep * y;
    const __m128i xstep_y_vect = _mm_set1_epi16(xstep_y);
    // All rows from |min_left_only_y| down for this set of columns, only need
    // |left_column| to compute. Rounded up to the nearest multiple of 4.
    const int min_left_only_y = std::min(((x + 4) << 6) / xstep, height);

    __m128i xstep_bounds = _mm_add_epi16(xstep_bounds_base, xstep_y_vect);
    __m128i xstep_for_shift_y = _mm_sub_epi16(xstep_for_shift, xstep_y_vect);
    int top_x = -xstep_y;

    // Loop over y for mixed rows.
    for (; y < min_left_only_y;
         y += 4, dst_x += stride4,
         xstep_bounds = _mm_add_epi16(xstep_bounds, xstep4_vect),
         xstep_for_shift_y = _mm_sub_epi16(xstep_for_shift_y, xstep4_vect),
         top_x -= xstep4) {
      DirectionalZone2FromLeftCol_4x4_SSE4_1<upsampled_left>(
          dst_x, stride,
          left_column + ((left_offset + y) * (1 << upsample_left_shift)),
          left_y);

      __m128i shifts = _mm_srli_epi16(
          _mm_and_si128(_mm_slli_epi16(xstep_for_shift_y, upsample_top_shift),
                        shift_mask),
          1);
      shifts = _mm_packus_epi16(shifts, shifts);
      const __m128i opposite_shifts = _mm_sub_epi8(max_shift, shifts);
      shifts = _mm_unpacklo_epi8(opposite_shifts, shifts);
      const __m128i xstep_bounds_off = _mm_srai_epi16(xstep_bounds, 6);
      DirectionalZone1Blend_4x4<upsampled_top>(
          dst_x, top_row + (x << upsample_top_shift), stride, sampler_top,
          xstep_bounds_off, shifts, dest_index_x, top_x, xstep);
    }
    // Loop over y for left-only rows, if any.
    for (; y < height; y += 4, dst_x += stride4) {
      DirectionalZone2FromLeftCol_4x4_SSE4_1<upsampled_left>(
          dst_x, stride,
          left_column + ((left_offset + y) << upsample_left_shift), left_y);
    }
  }
  // Loop over top-only columns, if any.
  for (; x < width; x += 4) {
    DirectionalZone1_4xH(dst + x, stride, top_row + (x << upsample_top_shift),
                         height, -xstep, upsampled_top);
  }
}

void DirectionalIntraPredictorZone2_SSE4_1(void* const dest, ptrdiff_t stride,
                                           const void* const top_row,
                                           const void* const left_column,
                                           const int width, const int height,
                                           const int xstep, const int ystep,
                                           const bool upsampled_top,
                                           const bool upsampled_left) {
  // Increasing the negative buffer for this function allows more rows to be
  // processed at a time without branching in an inner loop to check the base.
  uint8_t top_buffer[288];
  uint8_t left_buffer[288];
  memcpy(top_buffer + 128, static_cast<const uint8_t*>(top_row) - 16, 160);
  memcpy(left_buffer + 128, static_cast<const uint8_t*>(left_column) - 16, 160);
  const uint8_t* top_ptr = top_buffer + 144;
  const uint8_t* left_ptr = left_buffer + 144;
  if (width == 4 || height == 4) {
    if (upsampled_left) {
      if (upsampled_top) {
        DirectionalZone2_4_SSE4_1<true, true>(dest, stride, top_ptr, left_ptr,
                                              width, height, xstep, ystep);
      } else {
        DirectionalZone2_4_SSE4_1<true, false>(dest, stride, top_ptr, left_ptr,
                                               width, height, xstep, ystep);
      }
    } else {
      if (upsampled_top) {
        DirectionalZone2_4_SSE4_1<false, true>(dest, stride, top_ptr, left_ptr,
                                               width, height, xstep, ystep);
      } else {
        DirectionalZone2_4_SSE4_1<false, false>(dest, stride, top_ptr, left_ptr,
                                                width, height, xstep, ystep);
      }
    }
    return;
  }
  if (upsampled_left) {
    if (upsampled_top) {
      DirectionalZone2_SSE4_1<true, true>(dest, stride, top_ptr, left_ptr,
                                          width, height, xstep, ystep);
    } else {
      DirectionalZone2_SSE4_1<true, false>(dest, stride, top_ptr, left_ptr,
                                           width, height, xstep, ystep);
    }
  } else {
    if (upsampled_top) {
      DirectionalZone2_SSE4_1<false, true>(dest, stride, top_ptr, left_ptr,
                                           width, height, xstep, ystep);
    } else {
      DirectionalZone2_SSE4_1<false, false>(dest, stride, top_ptr, left_ptr,
                                            width, height, xstep, ystep);
    }
  }
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
  static_cast<void>(dsp);
#if DSP_ENABLED_8BPP_SSE4_1(DirectionalIntraPredictorZone1)
  dsp->directional_intra_predictor_zone1 =
      DirectionalIntraPredictorZone1_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(DirectionalIntraPredictorZone2)
  dsp->directional_intra_predictor_zone2 =
      DirectionalIntraPredictorZone2_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(DirectionalIntraPredictorZone3)
  dsp->directional_intra_predictor_zone3 =
      DirectionalIntraPredictorZone3_SSE4_1;
#endif
}

}  // namespace
}  // namespace low_bitdepth

//------------------------------------------------------------------------------
#if LIBGAV1_MAX_BITDEPTH >= 10
namespace high_bitdepth {
namespace {

//------------------------------------------------------------------------------
// 7.11.2.4. Directional intra prediction process

// Special case: An |xstep| of 64 corresponds to an angle delta of 45, meaning
// upsampling is ruled out. In addition, the bits masked by 0x3F for
// |shift_val| are 0 for all multiples of 64, so the formula
// val = top[top_base_x]*shift + top[top_base_x+1]*(32-shift), reduces to
// val = top[top_base_x+1] << 5, meaning only the second set of pixels is
// involved in the output. Hence |top| is offset by 1.
inline void DirectionalZone1_Step64(uint16_t* dst, ptrdiff_t stride,
                                    const uint16_t* const top, const int width,
                                    const int height) {
  ptrdiff_t offset = 1;
  if (height == 4) {
    memcpy(dst, top + offset, width * sizeof(dst[0]));
    dst += stride;
    memcpy(dst, top + offset + 1, width * sizeof(dst[0]));
    dst += stride;
    memcpy(dst, top + offset + 2, width * sizeof(dst[0]));
    dst += stride;
    memcpy(dst, top + offset + 3, width * sizeof(dst[0]));
    return;
  }
  int y = height;
  do {
    memcpy(dst, top + offset, width * sizeof(dst[0]));
    dst += stride;
    memcpy(dst, top + offset + 1, width * sizeof(dst[0]));
    dst += stride;
    memcpy(dst, top + offset + 2, width * sizeof(dst[0]));
    dst += stride;
    memcpy(dst, top + offset + 3, width * sizeof(dst[0]));
    dst += stride;
    memcpy(dst, top + offset + 4, width * sizeof(dst[0]));
    dst += stride;
    memcpy(dst, top + offset + 5, width * sizeof(dst[0]));
    dst += stride;
    memcpy(dst, top + offset + 6, width * sizeof(dst[0]));
    dst += stride;
    memcpy(dst, top + offset + 7, width * sizeof(dst[0]));
    dst += stride;

    offset += 8;
    y -= 8;
  } while (y != 0);
}

// Produce a weighted average whose weights sum to 32.
inline __m128i CombineTopVals4(const __m128i& top_vals, const __m128i& sampler,
                               const __m128i& shifts,
                               const __m128i& top_indices,
                               const __m128i& final_top_val,
                               const __m128i& border_index) {
  const __m128i sampled_values = _mm_shuffle_epi8(top_vals, sampler);
  __m128i prod = _mm_mullo_epi16(sampled_values, shifts);
  prod = _mm_hadd_epi16(prod, prod);
  const __m128i result = RightShiftWithRounding_U16(prod, 5 /*log2(32)*/);

  const __m128i past_max = _mm_cmpgt_epi16(top_indices, border_index);
  // Replace pixels from invalid range with top-right corner.
  return _mm_blendv_epi8(result, final_top_val, past_max);
}

// When width is 4, only one load operation is needed per iteration. We also
// avoid extra loop precomputations that cause too much overhead.
inline void DirectionalZone1_4xH(uint16_t* dst, ptrdiff_t stride,
                                 const uint16_t* const top, const int height,
                                 const int xstep, const bool upsampled,
                                 const __m128i& sampler) {
  const int upsample_shift = static_cast<int>(upsampled);
  const int index_scale_bits = 6 - upsample_shift;
  const int max_base_x = (height + 3 /* width - 1 */) << upsample_shift;
  const __m128i max_base_x_vect = _mm_set1_epi16(max_base_x);
  const __m128i final_top_val = _mm_set1_epi16(top[max_base_x]);

  // Each 16-bit value here corresponds to a position that may exceed
  // |max_base_x|. When added to the top_base_x, it is used to mask values
  // that pass the end of |top|. Starting from 1 to simulate "cmpge" because
  // only cmpgt is available.
  const __m128i offsets =
      _mm_set_epi32(0x00080007, 0x00060005, 0x00040003, 0x00020001);

  // All rows from |min_corner_only_y| down will simply use memcpy.
  // |max_base_x| is always greater than |height|, so clipping the denominator
  // to 1 is enough to make the logic work.
  const int xstep_units = std::max(xstep >> index_scale_bits, 1);
  const int min_corner_only_y = std::min(max_base_x / xstep_units, height);

  int y = 0;
  int top_x = xstep;
  const __m128i max_shift = _mm_set1_epi16(32);

  for (; y < min_corner_only_y; ++y, dst += stride, top_x += xstep) {
    const int top_base_x = top_x >> index_scale_bits;

    // Permit negative values of |top_x|.
    const int shift_val = (LeftShift(top_x, upsample_shift) & 0x3F) >> 1;
    const __m128i shift = _mm_set1_epi16(shift_val);
    const __m128i opposite_shift = _mm_sub_epi16(max_shift, shift);
    const __m128i shifts = _mm_unpacklo_epi16(opposite_shift, shift);
    __m128i top_index_vect = _mm_set1_epi16(top_base_x);
    top_index_vect = _mm_add_epi16(top_index_vect, offsets);

    // Load 8 values because we will select the sampled values based on
    // |upsampled|.
    const __m128i values = LoadUnaligned16(top + top_base_x);
    const __m128i pred =
        CombineTopVals4(values, sampler, shifts, top_index_vect, final_top_val,
                        max_base_x_vect);
    StoreLo8(dst, pred);
  }

  // Fill in corner-only rows.
  for (; y < height; ++y) {
    Memset(dst, top[max_base_x], /* width */ 4);
    dst += stride;
  }
}

// General purpose combine function.
// |check_border| means the final source value has to be duplicated into the
// result. This simplifies the loop structures that use precomputed boundaries
// to identify sections where it is safe to compute without checking for the
// right border.
template <bool check_border>
inline __m128i CombineTopVals(
    const __m128i& top_vals_0, const __m128i& top_vals_1,
    const __m128i& sampler, const __m128i& shifts,
    const __m128i& top_indices = _mm_setzero_si128(),
    const __m128i& final_top_val = _mm_setzero_si128(),
    const __m128i& border_index = _mm_setzero_si128()) {
  constexpr int scale_int_bits = 5;
  const __m128i sampled_values_0 = _mm_shuffle_epi8(top_vals_0, sampler);
  const __m128i sampled_values_1 = _mm_shuffle_epi8(top_vals_1, sampler);
  const __m128i prod_0 = _mm_mullo_epi16(sampled_values_0, shifts);
  const __m128i prod_1 = _mm_mullo_epi16(sampled_values_1, shifts);
  const __m128i combined = _mm_hadd_epi16(prod_0, prod_1);
  const __m128i result = RightShiftWithRounding_U16(combined, scale_int_bits);
  if (check_border) {
    const __m128i past_max = _mm_cmpgt_epi16(top_indices, border_index);
    // Replace pixels from invalid range with top-right corner.
    return _mm_blendv_epi8(result, final_top_val, past_max);
  }
  return result;
}

// 7.11.2.4 (7) angle < 90
inline void DirectionalZone1_Large(uint16_t* dest, ptrdiff_t stride,
                                   const uint16_t* const top_row,
                                   const int width, const int height,
                                   const int xstep, const bool upsampled,
                                   const __m128i& sampler) {
  const int upsample_shift = static_cast<int>(upsampled);
  const int index_scale_bits = 6 - upsample_shift;
  const int max_base_x = ((width + height) - 1) << upsample_shift;

  const __m128i max_shift = _mm_set1_epi16(32);
  const int base_step = 1 << upsample_shift;
  const int base_step8 = base_step << 3;

  // All rows from |min_corner_only_y| down will simply use memcpy.
  // |max_base_x| is always greater than |height|, so clipping to 1 is enough
  // to make the logic work.
  const int xstep_units = std::max(xstep >> index_scale_bits, 1);
  const int min_corner_only_y = std::min(max_base_x / xstep_units, height);

  // Rows up to this y-value can be computed without checking for bounds.
  const int max_no_corner_y = std::min(
      LeftShift((max_base_x - (base_step * width)), index_scale_bits) / xstep,
      height);
  // No need to check for exceeding |max_base_x| in the first loop.
  int y = 0;
  int top_x = xstep;
  for (; y < max_no_corner_y; ++y, dest += stride, top_x += xstep) {
    int top_base_x = top_x >> index_scale_bits;
    // Permit negative values of |top_x|.
    const int shift_val = (LeftShift(top_x, upsample_shift) & 0x3F) >> 1;
    const __m128i shift = _mm_set1_epi16(shift_val);
    const __m128i opposite_shift = _mm_sub_epi16(max_shift, shift);
    const __m128i shifts = _mm_unpacklo_epi16(opposite_shift, shift);
    int x = 0;
    do {
      const __m128i top_vals_0 = LoadUnaligned16(top_row + top_base_x);
      const __m128i top_vals_1 =
          LoadUnaligned16(top_row + top_base_x + (4 << upsample_shift));

      const __m128i pred =
          CombineTopVals<false>(top_vals_0, top_vals_1, sampler, shifts);

      StoreUnaligned16(dest + x, pred);
      top_base_x += base_step8;
      x += 8;
    } while (x < width);
  }

  // Each 16-bit value here corresponds to a position that may exceed
  // |max_base_x|. When added to |top_base_x|, it is used to mask values
  // that pass the end of the |top| buffer. Starting from 1 to simulate "cmpge"
  // which is not supported for packed integers.
  const __m128i offsets =
      _mm_set_epi32(0x00080007, 0x00060005, 0x00040003, 0x00020001);

  const __m128i max_base_x_vect = _mm_set1_epi16(max_base_x);
  const __m128i final_top_val = _mm_set1_epi16(top_row[max_base_x]);
  const __m128i base_step8_vect = _mm_set1_epi16(base_step8);
  for (; y < min_corner_only_y; ++y, dest += stride, top_x += xstep) {
    int top_base_x = top_x >> index_scale_bits;

    const int shift_val = (LeftShift(top_x, upsample_shift) & 0x3F) >> 1;
    const __m128i shift = _mm_set1_epi16(shift_val);
    const __m128i opposite_shift = _mm_sub_epi16(max_shift, shift);
    const __m128i shifts = _mm_unpacklo_epi16(opposite_shift, shift);
    __m128i top_index_vect = _mm_set1_epi16(top_base_x);
    top_index_vect = _mm_add_epi16(top_index_vect, offsets);

    int x = 0;
    const int min_corner_only_x =
        std::min(width, ((max_base_x - top_base_x) >> upsample_shift) + 7) & ~7;
    for (; x < min_corner_only_x;
         x += 8, top_base_x += base_step8,
         top_index_vect = _mm_add_epi16(top_index_vect, base_step8_vect)) {
      const __m128i top_vals_0 = LoadUnaligned16(top_row + top_base_x);
      const __m128i top_vals_1 =
          LoadUnaligned16(top_row + top_base_x + (4 << upsample_shift));
      const __m128i pred =
          CombineTopVals<true>(top_vals_0, top_vals_1, sampler, shifts,
                               top_index_vect, final_top_val, max_base_x_vect);
      StoreUnaligned16(dest + x, pred);
    }
    // Corner-only section of the row.
    Memset(dest + x, top_row[max_base_x], width - x);
  }
  // Fill in corner-only rows.
  for (; y < height; ++y) {
    Memset(dest, top_row[max_base_x], width);
    dest += stride;
  }
}

// 7.11.2.4 (7) angle < 90
inline void DirectionalIntraPredictorZone1_SSE4_1(
    void* dest_ptr, ptrdiff_t stride, const void* const top_ptr,
    const int width, const int height, const int xstep, const bool upsampled) {
  const auto* const top_row = static_cast<const uint16_t*>(top_ptr);
  auto* dest = static_cast<uint16_t*>(dest_ptr);
  stride /= sizeof(uint16_t);
  const int upsample_shift = static_cast<int>(upsampled);
  if (xstep == 64) {
    DirectionalZone1_Step64(dest, stride, top_row, width, height);
    return;
  }
  // Each base pixel paired with its following pixel, for hadd purposes.
  const __m128i adjacency_shuffler = _mm_set_epi16(
      0x0908, 0x0706, 0x0706, 0x0504, 0x0504, 0x0302, 0x0302, 0x0100);
  // This is equivalent to not shuffling at all.
  const __m128i identity_shuffler = _mm_set_epi16(
      0x0F0E, 0x0D0C, 0x0B0A, 0x0908, 0x0706, 0x0504, 0x0302, 0x0100);
  // This represents a trade-off between code size and speed. When upsampled
  // is true, no shuffle is necessary. But to avoid in-loop branching, we
  // would need 2 copies of the main function body.
  const __m128i sampler = upsampled ? identity_shuffler : adjacency_shuffler;
  if (width == 4) {
    DirectionalZone1_4xH(dest, stride, top_row, height, xstep, upsampled,
                         sampler);
    return;
  }
  if (width >= 32) {
    DirectionalZone1_Large(dest, stride, top_row, width, height, xstep,
                           upsampled, sampler);
    return;
  }
  const int index_scale_bits = 6 - upsample_shift;
  const int max_base_x = ((width + height) - 1) << upsample_shift;

  const __m128i max_shift = _mm_set1_epi16(32);
  const int base_step = 1 << upsample_shift;
  const int base_step8 = base_step << 3;

  // No need to check for exceeding |max_base_x| in the loops.
  if (((xstep * height) >> index_scale_bits) + base_step * width < max_base_x) {
    int top_x = xstep;
    int y = height;
    do {
      int top_base_x = top_x >> index_scale_bits;
      // Permit negative values of |top_x|.
      const int shift_val = (LeftShift(top_x, upsample_shift) & 0x3F) >> 1;
      const __m128i shift = _mm_set1_epi16(shift_val);
      const __m128i opposite_shift = _mm_sub_epi16(max_shift, shift);
      const __m128i shifts = _mm_unpacklo_epi16(opposite_shift, shift);
      int x = 0;
      do {
        const __m128i top_vals_0 = LoadUnaligned16(top_row + top_base_x);
        const __m128i top_vals_1 =
            LoadUnaligned16(top_row + top_base_x + (4 << upsample_shift));
        const __m128i pred =
            CombineTopVals<false>(top_vals_0, top_vals_1, sampler, shifts);
        StoreUnaligned16(dest + x, pred);
        top_base_x += base_step8;
        x += 8;
      } while (x < width);
      dest += stride;
      top_x += xstep;
    } while (--y != 0);
    return;
  }

  // General case. Blocks with width less than 32 do not benefit from x-wise
  // loop splitting, but do benefit from using memset on appropriate rows.

  // Each 16-bit value here corresponds to a position that may exceed
  // |max_base_x|. When added to the top_base_x, it is used to mask values
  // that pass the end of |top|. Starting from 1 to simulate "cmpge" which is
  // not supported for packed integers.
  const __m128i offsets =
      _mm_set_epi32(0x00080007, 0x00060005, 0x00040003, 0x00020001);

  const __m128i max_base_x_vect = _mm_set1_epi16(max_base_x);
  const __m128i final_top_val = _mm_set1_epi16(top_row[max_base_x]);
  const __m128i base_step8_vect = _mm_set1_epi16(base_step8);

  // All rows from |min_corner_only_y| down will simply use memcpy.
  // |max_base_x| is always greater than |height|, so clipping the denominator
  // to 1 is enough to make the logic work.
  const int xstep_units = std::max(xstep >> index_scale_bits, 1);
  const int min_corner_only_y = std::min(max_base_x / xstep_units, height);

  int top_x = xstep;
  int y = 0;
  for (; y < min_corner_only_y; ++y, dest += stride, top_x += xstep) {
    int top_base_x = top_x >> index_scale_bits;

    const int shift_val = (LeftShift(top_x, upsample_shift) & 0x3F) >> 1;
    const __m128i shift = _mm_set1_epi16(shift_val);
    const __m128i opposite_shift = _mm_sub_epi16(max_shift, shift);
    const __m128i shifts = _mm_unpacklo_epi16(opposite_shift, shift);
    __m128i top_index_vect = _mm_set1_epi16(top_base_x);
    top_index_vect = _mm_add_epi16(top_index_vect, offsets);

    for (int x = 0; x < width; x += 8, top_base_x += base_step8,
             top_index_vect = _mm_add_epi16(top_index_vect, base_step8_vect)) {
      const __m128i top_vals_0 = LoadUnaligned16(top_row + top_base_x);
      const __m128i top_vals_1 =
          LoadUnaligned16(top_row + top_base_x + (4 << upsample_shift));
      const __m128i pred =
          CombineTopVals<true>(top_vals_0, top_vals_1, sampler, shifts,
                               top_index_vect, final_top_val, max_base_x_vect);
      StoreUnaligned16(dest + x, pred);
    }
  }

  // Fill in corner-only rows.
  for (; y < height; ++y) {
    Memset(dest, top_row[max_base_x], width);
    dest += stride;
  }
}

void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(10);
  assert(dsp != nullptr);
  static_cast<void>(dsp);
#if DSP_ENABLED_10BPP_SSE4_1(DirectionalIntraPredictorZone1)
  dsp->directional_intra_predictor_zone1 =
      DirectionalIntraPredictorZone1_SSE4_1;
#endif
}

}  // namespace
}  // namespace high_bitdepth

#endif  // LIBGAV1_MAX_BITDEPTH >= 10

void IntraPredDirectionalInit_SSE4_1() {
  low_bitdepth::Init8bpp();
#if LIBGAV1_MAX_BITDEPTH >= 10
  high_bitdepth::Init10bpp();
#endif
}

}  // namespace dsp
}  // namespace libgav1

#else   // !LIBGAV1_TARGETING_SSE4_1
namespace libgav1 {
namespace dsp {

void IntraPredDirectionalInit_SSE4_1() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_TARGETING_SSE4_1
