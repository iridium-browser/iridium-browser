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
#include "src/utils/cpu.h"

#if LIBGAV1_TARGETING_SSE4_1

#include <xmmintrin.h>

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
#include "src/utils/constants.h"

namespace libgav1 {
namespace dsp {
namespace {

//------------------------------------------------------------------------------
// FilterIntraPredictor_SSE4_1
// Section 7.11.2.3. Recursive intra prediction process
// This filter applies recursively to 4x2 sub-blocks within the transform block,
// meaning that the predicted pixels in each sub-block are used as inputs to
// sub-blocks below and to the right, if present.
//
// Each output value in the sub-block is predicted by a different filter applied
// to the same array of top-left, top, and left values. If fn refers to the
// output of the nth filter, given this block:
// TL T0 T1 T2 T3
// L0 f0 f1 f2 f3
// L1 f4 f5 f6 f7
// The filter input order is p0, p1, p2, p3, p4, p5, p6:
// p0 p1 p2 p3 p4
// p5 f0 f1 f2 f3
// p6 f4 f5 f6 f7
// Filters usually apply to 8 values for convenience, so in this case we fix
// the 8th filter tap to 0 and disregard the value of the 8th input.

// This shuffle mask selects 32-bit blocks in the order 0, 1, 0, 1, which
// duplicates the first 8 bytes of a 128-bit vector into the second 8 bytes.
constexpr int kDuplicateFirstHalf = 0x44;

// Apply all filter taps to the given 7 packed 16-bit values, keeping the 8th
// at zero to preserve the sum.
// |pixels| contains p0-p7 in order as shown above.
// |taps_0_1| contains the filter kernels used to predict f0 and f1, and so on.
inline void Filter4x2_SSE4_1(uint8_t* LIBGAV1_RESTRICT dst,
                             const ptrdiff_t stride, const __m128i& pixels,
                             const __m128i& taps_0_1, const __m128i& taps_2_3,
                             const __m128i& taps_4_5, const __m128i& taps_6_7) {
  const __m128i mul_0_01 = _mm_maddubs_epi16(pixels, taps_0_1);
  const __m128i mul_0_23 = _mm_maddubs_epi16(pixels, taps_2_3);
  // |output_half| contains 8 partial sums for f0-f7.
  __m128i output_half = _mm_hadd_epi16(mul_0_01, mul_0_23);
  __m128i output = _mm_hadd_epi16(output_half, output_half);
  const __m128i output_row0 =
      _mm_packus_epi16(RightShiftWithRounding_S16(output, 4),
                       /* unused half */ output);
  Store4(dst, output_row0);
  const __m128i mul_1_01 = _mm_maddubs_epi16(pixels, taps_4_5);
  const __m128i mul_1_23 = _mm_maddubs_epi16(pixels, taps_6_7);
  output_half = _mm_hadd_epi16(mul_1_01, mul_1_23);
  output = _mm_hadd_epi16(output_half, output_half);
  const __m128i output_row1 =
      _mm_packus_epi16(RightShiftWithRounding_S16(output, 4),
                       /* arbitrary pack arg */ output);
  Store4(dst + stride, output_row1);
}

// 4xH transform sizes are given special treatment because LoadLo8 goes out
// of bounds and every block involves the left column. The top-left pixel, p0,
// is stored in the top buffer for the first 4x2, but comes from the left buffer
// for successive blocks. This implementation takes advantage of the fact
// that the p5 and p6 for each sub-block come solely from the |left_ptr| buffer,
// using shifts to arrange things to fit reusable shuffle vectors.
inline void Filter4xH(uint8_t* LIBGAV1_RESTRICT dest, ptrdiff_t stride,
                      const uint8_t* LIBGAV1_RESTRICT const top_ptr,
                      const uint8_t* LIBGAV1_RESTRICT const left_ptr,
                      FilterIntraPredictor pred, const int height) {
  // Two filter kernels per vector.
  const __m128i taps_0_1 = LoadAligned16(kFilterIntraTaps[pred][0]);
  const __m128i taps_2_3 = LoadAligned16(kFilterIntraTaps[pred][2]);
  const __m128i taps_4_5 = LoadAligned16(kFilterIntraTaps[pred][4]);
  const __m128i taps_6_7 = LoadAligned16(kFilterIntraTaps[pred][6]);
  __m128i top = Load4(top_ptr - 1);
  __m128i pixels = _mm_insert_epi8(top, top_ptr[3], 4);
  __m128i left = (height == 4 ? Load4(left_ptr) : LoadLo8(left_ptr));
  left = _mm_slli_si128(left, 5);

  // Relative pixels: top[-1], top[0], top[1], top[2], top[3], left[0], left[1],
  // left[2], left[3], left[4], left[5], left[6], left[7]
  // Let rn represent a pixel usable as pn for the 4x2 after this one. We get:
  //  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
  // p0 p1 p2 p3 p4 p5 p6 r5 r6 ...
  //                   r0
  pixels = _mm_or_si128(left, pixels);

  // Two sets of the same input pixels to apply two filters at once.
  pixels = _mm_shuffle_epi32(pixels, kDuplicateFirstHalf);
  Filter4x2_SSE4_1(dest, stride, pixels, taps_0_1, taps_2_3, taps_4_5,
                   taps_6_7);
  dest += stride;  // Move to y = 1.
  pixels = Load4(dest);

  // Relative pixels: top[0], top[1], top[2], top[3], empty, left[-2], left[-1],
  // left[0], left[1], ...
  //  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
  // p1 p2 p3 p4 xx xx p0 p5 p6 r5 r6 ...
  //                         r0
  pixels = _mm_or_si128(left, pixels);

  // This mask rearranges bytes in the order: 6, 0, 1, 2, 3, 7, 8, 15. The last
  // byte is an unused value, which shall be multiplied by 0 when we apply the
  // filter.
  constexpr int64_t kInsertTopLeftFirstMask = 0x0F08070302010006;

  // Insert left[-1] in front as TL and put left[0] and left[1] at the end.
  const __m128i pixel_order1 = _mm_set1_epi64x(kInsertTopLeftFirstMask);
  pixels = _mm_shuffle_epi8(pixels, pixel_order1);
  dest += stride;  // Move to y = 2.
  Filter4x2_SSE4_1(dest, stride, pixels, taps_0_1, taps_2_3, taps_4_5,
                   taps_6_7);
  dest += stride;  // Move to y = 3.

  // Compute the middle 8 rows before using common code for the final 4 rows, in
  // order to fit the assumption that |left| has the next TL at position 8.
  if (height == 16) {
    // This shift allows us to use pixel_order2 twice after shifting by 2 later.
    left = _mm_slli_si128(left, 1);
    pixels = Load4(dest);

    // Relative pixels: top[0], top[1], top[2], top[3], empty, empty, left[-4],
    // left[-3], left[-2], left[-1], left[0], left[1], left[2], left[3]
    //  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
    // p1 p2 p3 p4 xx xx xx xx xx p0 p5 p6 r5 r6 ...
    //                                  r0
    pixels = _mm_or_si128(left, pixels);

    // This mask rearranges bytes in the order: 9, 0, 1, 2, 3, 7, 8, 15. The
    // last byte is an unused value, as above. The top-left was shifted to
    // position nine to keep two empty spaces after the top pixels.
    constexpr int64_t kInsertTopLeftSecondMask = 0x0F0B0A0302010009;

    // Insert (relative) left[-1] in front as TL and put left[0] and left[1] at
    // the end.
    const __m128i pixel_order2 = _mm_set1_epi64x(kInsertTopLeftSecondMask);
    pixels = _mm_shuffle_epi8(pixels, pixel_order2);
    dest += stride;  // Move to y = 4.

    // First 4x2 in the if body.
    Filter4x2_SSE4_1(dest, stride, pixels, taps_0_1, taps_2_3, taps_4_5,
                     taps_6_7);

    // Clear all but final pixel in the first 8 of left column.
    __m128i keep_top_left = _mm_srli_si128(left, 13);
    dest += stride;  // Move to y = 5.
    pixels = Load4(dest);
    left = _mm_srli_si128(left, 2);

    // Relative pixels: top[0], top[1], top[2], top[3], left[-6],
    // left[-5], left[-4], left[-3], left[-2], left[-1], left[0], left[1]
    //  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
    // p1 p2 p3 p4 xx xx xx xx xx p0 p5 p6 r5 r6 ...
    //                                  r0
    pixels = _mm_or_si128(left, pixels);
    left = LoadLo8(left_ptr + 8);

    pixels = _mm_shuffle_epi8(pixels, pixel_order2);
    dest += stride;  // Move to y = 6.

    // Second 4x2 in the if body.
    Filter4x2_SSE4_1(dest, stride, pixels, taps_0_1, taps_2_3, taps_4_5,
                     taps_6_7);

    // Position TL value so we can use pixel_order1.
    keep_top_left = _mm_slli_si128(keep_top_left, 6);
    dest += stride;  // Move to y = 7.
    pixels = Load4(dest);
    left = _mm_slli_si128(left, 7);
    left = _mm_or_si128(left, keep_top_left);

    // Relative pixels: top[0], top[1], top[2], top[3], empty, empty,
    // left[-1], left[0], left[1], left[2], left[3], ...
    //  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
    // p1 p2 p3 p4 xx xx p0 p5 p6 r5 r6 ...
    //                         r0
    pixels = _mm_or_si128(left, pixels);
    pixels = _mm_shuffle_epi8(pixels, pixel_order1);
    dest += stride;  // Move to y = 8.

    // Third 4x2 in the if body.
    Filter4x2_SSE4_1(dest, stride, pixels, taps_0_1, taps_2_3, taps_4_5,
                     taps_6_7);
    dest += stride;  // Move to y = 9.

    // Prepare final inputs.
    pixels = Load4(dest);
    left = _mm_srli_si128(left, 2);

    // Relative pixels: top[0], top[1], top[2], top[3], left[-3], left[-2]
    // left[-1], left[0], left[1], left[2], left[3], ...
    //  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
    // p1 p2 p3 p4 xx xx p0 p5 p6 r5 r6 ...
    //                         r0
    pixels = _mm_or_si128(left, pixels);
    pixels = _mm_shuffle_epi8(pixels, pixel_order1);
    dest += stride;  // Move to y = 10.

    // Fourth 4x2 in the if body.
    Filter4x2_SSE4_1(dest, stride, pixels, taps_0_1, taps_2_3, taps_4_5,
                     taps_6_7);
    dest += stride;  // Move to y = 11.
  }

  // In both the 8 and 16 case at this point, we can assume that |left| has the
  // next TL at position 8.
  if (height > 4) {
    // Erase prior left pixels by shifting TL to position 0.
    left = _mm_srli_si128(left, 8);
    left = _mm_slli_si128(left, 6);
    pixels = Load4(dest);

    // Relative pixels: top[0], top[1], top[2], top[3], empty, empty,
    // left[-1], left[0], left[1], left[2], left[3], ...
    //  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
    // p1 p2 p3 p4 xx xx p0 p5 p6 r5 r6 ...
    //                         r0
    pixels = _mm_or_si128(left, pixels);
    pixels = _mm_shuffle_epi8(pixels, pixel_order1);
    dest += stride;  // Move to y = 12 or 4.

    // First of final two 4x2 blocks.
    Filter4x2_SSE4_1(dest, stride, pixels, taps_0_1, taps_2_3, taps_4_5,
                     taps_6_7);
    dest += stride;  // Move to y = 13 or 5.
    pixels = Load4(dest);
    left = _mm_srli_si128(left, 2);

    // Relative pixels: top[0], top[1], top[2], top[3], left[-3], left[-2]
    // left[-1], left[0], left[1], left[2], left[3], ...
    //  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
    // p1 p2 p3 p4 xx xx p0 p5 p6 r5 r6 ...
    //                         r0
    pixels = _mm_or_si128(left, pixels);
    pixels = _mm_shuffle_epi8(pixels, pixel_order1);
    dest += stride;  // Move to y = 14 or 6.

    // Last of final two 4x2 blocks.
    Filter4x2_SSE4_1(dest, stride, pixels, taps_0_1, taps_2_3, taps_4_5,
                     taps_6_7);
  }
}

void FilterIntraPredictor_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                                 ptrdiff_t stride,
                                 const void* LIBGAV1_RESTRICT const top_row,
                                 const void* LIBGAV1_RESTRICT const left_column,
                                 FilterIntraPredictor pred, const int width,
                                 const int height) {
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  auto* dst = static_cast<uint8_t*>(dest);
  if (width == 4) {
    Filter4xH(dst, stride, top_ptr, left_ptr, pred, height);
    return;
  }

  // There is one set of 7 taps for each of the 4x2 output pixels.
  const __m128i taps_0_1 = LoadAligned16(kFilterIntraTaps[pred][0]);
  const __m128i taps_2_3 = LoadAligned16(kFilterIntraTaps[pred][2]);
  const __m128i taps_4_5 = LoadAligned16(kFilterIntraTaps[pred][4]);
  const __m128i taps_6_7 = LoadAligned16(kFilterIntraTaps[pred][6]);

  // This mask rearranges bytes in the order: 0, 1, 2, 3, 4, 8, 9, 15. The 15 at
  // the end is an unused value, which shall be multiplied by 0 when we apply
  // the filter.
  constexpr int64_t kCondenseLeftMask = 0x0F09080403020100;

  // Takes the "left section" and puts it right after p0-p4.
  const __m128i pixel_order1 = _mm_set1_epi64x(kCondenseLeftMask);

  // This mask rearranges bytes in the order: 8, 0, 1, 2, 3, 9, 10, 15. The last
  // byte is unused as above.
  constexpr int64_t kInsertTopLeftMask = 0x0F0A090302010008;

  // Shuffles the "top left" from the left section, to the front. Used when
  // grabbing data from left_column and not top_row.
  const __m128i pixel_order2 = _mm_set1_epi64x(kInsertTopLeftMask);

  // This first pass takes care of the cases where the top left pixel comes from
  // top_row.
  __m128i pixels = LoadLo8(top_ptr - 1);
  __m128i left = _mm_slli_si128(Load4(left_column), 8);
  pixels = _mm_or_si128(pixels, left);

  // Two sets of the same pixels to multiply with two sets of taps.
  pixels = _mm_shuffle_epi8(pixels, pixel_order1);
  Filter4x2_SSE4_1(dst, stride, pixels, taps_0_1, taps_2_3, taps_4_5, taps_6_7);
  left = _mm_srli_si128(left, 1);

  // Load
  pixels = Load4(dst + stride);

  // Because of the above shift, this OR 'invades' the final of the first 8
  // bytes of |pixels|. This is acceptable because the 8th filter tap is always
  // a padded 0.
  pixels = _mm_or_si128(pixels, left);
  pixels = _mm_shuffle_epi8(pixels, pixel_order2);
  const ptrdiff_t stride2 = stride << 1;
  const ptrdiff_t stride4 = stride << 2;
  Filter4x2_SSE4_1(dst + stride2, stride, pixels, taps_0_1, taps_2_3, taps_4_5,
                   taps_6_7);
  dst += 4;
  for (int x = 3; x < width - 4; x += 4) {
    pixels = Load4(top_ptr + x);
    pixels = _mm_insert_epi8(pixels, top_ptr[x + 4], 4);
    pixels = _mm_insert_epi8(pixels, dst[-1], 5);
    pixels = _mm_insert_epi8(pixels, dst[stride - 1], 6);

    // Duplicate bottom half into upper half.
    pixels = _mm_shuffle_epi32(pixels, kDuplicateFirstHalf);
    Filter4x2_SSE4_1(dst, stride, pixels, taps_0_1, taps_2_3, taps_4_5,
                     taps_6_7);
    pixels = Load4(dst + stride - 1);
    pixels = _mm_insert_epi8(pixels, dst[stride + 3], 4);
    pixels = _mm_insert_epi8(pixels, dst[stride2 - 1], 5);
    pixels = _mm_insert_epi8(pixels, dst[stride + stride2 - 1], 6);

    // Duplicate bottom half into upper half.
    pixels = _mm_shuffle_epi32(pixels, kDuplicateFirstHalf);
    Filter4x2_SSE4_1(dst + stride2, stride, pixels, taps_0_1, taps_2_3,
                     taps_4_5, taps_6_7);
    dst += 4;
  }

  // Now we handle heights that reference previous blocks rather than top_row.
  for (int y = 4; y < height; y += 4) {
    // Leftmost 4x4 block for this height.
    dst -= width;
    dst += stride4;

    // Top Left is not available by offset in these leftmost blocks.
    pixels = Load4(dst - stride);
    left = _mm_slli_si128(Load4(left_ptr + y - 1), 8);
    left = _mm_insert_epi8(left, left_ptr[y + 3], 12);
    pixels = _mm_or_si128(pixels, left);
    pixels = _mm_shuffle_epi8(pixels, pixel_order2);
    Filter4x2_SSE4_1(dst, stride, pixels, taps_0_1, taps_2_3, taps_4_5,
                     taps_6_7);

    // The bytes shifted into positions 6 and 7 will be ignored by the shuffle.
    left = _mm_srli_si128(left, 2);
    pixels = Load4(dst + stride);
    pixels = _mm_or_si128(pixels, left);
    pixels = _mm_shuffle_epi8(pixels, pixel_order2);
    Filter4x2_SSE4_1(dst + stride2, stride, pixels, taps_0_1, taps_2_3,
                     taps_4_5, taps_6_7);

    dst += 4;

    // Remaining 4x4 blocks for this height.
    for (int x = 4; x < width; x += 4) {
      pixels = Load4(dst - stride - 1);
      pixels = _mm_insert_epi8(pixels, dst[-stride + 3], 4);
      pixels = _mm_insert_epi8(pixels, dst[-1], 5);
      pixels = _mm_insert_epi8(pixels, dst[stride - 1], 6);

      // Duplicate bottom half into upper half.
      pixels = _mm_shuffle_epi32(pixels, kDuplicateFirstHalf);
      Filter4x2_SSE4_1(dst, stride, pixels, taps_0_1, taps_2_3, taps_4_5,
                       taps_6_7);
      pixels = Load4(dst + stride - 1);
      pixels = _mm_insert_epi8(pixels, dst[stride + 3], 4);
      pixels = _mm_insert_epi8(pixels, dst[stride2 - 1], 5);
      pixels = _mm_insert_epi8(pixels, dst[stride2 + stride - 1], 6);

      // Duplicate bottom half into upper half.
      pixels = _mm_shuffle_epi32(pixels, kDuplicateFirstHalf);
      Filter4x2_SSE4_1(dst + stride2, stride, pixels, taps_0_1, taps_2_3,
                       taps_4_5, taps_6_7);
      dst += 4;
    }
  }
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
  static_cast<void>(dsp);
// These guards check if this version of the function was not superseded by
// a higher optimization level, such as AVX. The corresponding #define also
// prevents the C version from being added to the table.
#if DSP_ENABLED_8BPP_SSE4_1(FilterIntraPredictor)
  dsp->filter_intra_predictor = FilterIntraPredictor_SSE4_1;
#endif
}

}  // namespace

void IntraPredFilterInit_SSE4_1() { Init8bpp(); }

}  // namespace dsp
}  // namespace libgav1

#else   // !LIBGAV1_TARGETING_SSE4_1
namespace libgav1 {
namespace dsp {

void IntraPredFilterInit_SSE4_1() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_TARGETING_SSE4_1
