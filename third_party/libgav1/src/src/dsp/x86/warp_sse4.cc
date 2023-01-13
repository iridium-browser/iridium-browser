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

#include "src/dsp/warp.h"
#include "src/utils/cpu.h"

#if LIBGAV1_TARGETING_SSE4_1

#include <smmintrin.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/dsp/x86/common_sse4.h"
#include "src/dsp/x86/transpose_sse4.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"

namespace libgav1 {
namespace dsp {
namespace low_bitdepth {
namespace {

// Number of extra bits of precision in warped filtering.
constexpr int kWarpedDiffPrecisionBits = 10;

// This assumes the two filters contain filter[x] and filter[x+2].
inline __m128i AccumulateFilter(const __m128i sum, const __m128i filter_0,
                                const __m128i filter_1,
                                const __m128i& src_window) {
  const __m128i filter_taps = _mm_unpacklo_epi8(filter_0, filter_1);
  const __m128i src =
      _mm_unpacklo_epi8(src_window, _mm_srli_si128(src_window, 2));
  return _mm_add_epi16(sum, _mm_maddubs_epi16(src, filter_taps));
}

constexpr int kFirstPassOffset = 1 << 14;
constexpr int kOffsetRemoval =
    (kFirstPassOffset >> kInterRoundBitsHorizontal) * 128;

// Applies the horizontal filter to one source row and stores the result in
// |intermediate_result_row|. |intermediate_result_row| is a row in the 15x8
// |intermediate_result| two-dimensional array.
inline void HorizontalFilter(const int sx4, const int16_t alpha,
                             const __m128i src_row,
                             int16_t intermediate_result_row[8]) {
  int sx = sx4 - MultiplyBy4(alpha);
  __m128i filter[8];
  for (__m128i& f : filter) {
    const int offset = RightShiftWithRounding(sx, kWarpedDiffPrecisionBits) +
                       kWarpedPixelPrecisionShifts;
    f = LoadLo8(kWarpedFilters8[offset]);
    sx += alpha;
  }
  Transpose8x8To4x16_U8(filter, filter);
  // |filter| now contains two filters per register.
  // Staggered combinations allow us to take advantage of _mm_maddubs_epi16
  // without overflowing the sign bit. The sign bit is hit only where two taps
  // paired in a single madd add up to more than 128. This is only possible with
  // two adjacent "inner" taps. Therefore, pairing odd with odd and even with
  // even guarantees safety. |sum| is given a negative offset to allow for large
  // intermediate values.
  // k = 0, 2.
  __m128i src_row_window = src_row;
  __m128i sum = _mm_set1_epi16(-kFirstPassOffset);
  sum = AccumulateFilter(sum, filter[0], filter[1], src_row_window);

  // k = 1, 3.
  src_row_window = _mm_srli_si128(src_row_window, 1);
  sum = AccumulateFilter(sum, _mm_srli_si128(filter[0], 8),
                         _mm_srli_si128(filter[1], 8), src_row_window);
  // k = 4, 6.
  src_row_window = _mm_srli_si128(src_row_window, 3);
  sum = AccumulateFilter(sum, filter[2], filter[3], src_row_window);

  // k = 5, 7.
  src_row_window = _mm_srli_si128(src_row_window, 1);
  sum = AccumulateFilter(sum, _mm_srli_si128(filter[2], 8),
                         _mm_srli_si128(filter[3], 8), src_row_window);

  sum = RightShiftWithRounding_S16(sum, kInterRoundBitsHorizontal);
  StoreUnaligned16(intermediate_result_row, sum);
}

template <bool is_compound>
inline void WriteVerticalFilter(const __m128i filter[8],
                                const int16_t intermediate_result[15][8], int y,
                                void* LIBGAV1_RESTRICT dst_row) {
  constexpr int kRoundBitsVertical =
      is_compound ? kInterRoundBitsCompoundVertical : kInterRoundBitsVertical;
  __m128i sum_low = _mm_set1_epi32(kOffsetRemoval);
  __m128i sum_high = sum_low;
  for (int k = 0; k < 8; k += 2) {
    const __m128i filters_low = _mm_unpacklo_epi16(filter[k], filter[k + 1]);
    const __m128i filters_high = _mm_unpackhi_epi16(filter[k], filter[k + 1]);
    const __m128i intermediate_0 = LoadUnaligned16(intermediate_result[y + k]);
    const __m128i intermediate_1 =
        LoadUnaligned16(intermediate_result[y + k + 1]);
    const __m128i intermediate_low =
        _mm_unpacklo_epi16(intermediate_0, intermediate_1);
    const __m128i intermediate_high =
        _mm_unpackhi_epi16(intermediate_0, intermediate_1);

    const __m128i product_low = _mm_madd_epi16(filters_low, intermediate_low);
    const __m128i product_high =
        _mm_madd_epi16(filters_high, intermediate_high);
    sum_low = _mm_add_epi32(sum_low, product_low);
    sum_high = _mm_add_epi32(sum_high, product_high);
  }
  sum_low = RightShiftWithRounding_S32(sum_low, kRoundBitsVertical);
  sum_high = RightShiftWithRounding_S32(sum_high, kRoundBitsVertical);
  if (is_compound) {
    const __m128i sum = _mm_packs_epi32(sum_low, sum_high);
    StoreUnaligned16(static_cast<int16_t*>(dst_row), sum);
  } else {
    const __m128i sum = _mm_packus_epi32(sum_low, sum_high);
    StoreLo8(static_cast<uint8_t*>(dst_row), _mm_packus_epi16(sum, sum));
  }
}

template <bool is_compound>
inline void WriteVerticalFilter(const __m128i filter[8],
                                const int16_t* LIBGAV1_RESTRICT
                                    intermediate_result_column,
                                void* LIBGAV1_RESTRICT dst_row) {
  constexpr int kRoundBitsVertical =
      is_compound ? kInterRoundBitsCompoundVertical : kInterRoundBitsVertical;
  __m128i sum_low = _mm_setzero_si128();
  __m128i sum_high = _mm_setzero_si128();
  for (int k = 0; k < 8; k += 2) {
    const __m128i filters_low = _mm_unpacklo_epi16(filter[k], filter[k + 1]);
    const __m128i filters_high = _mm_unpackhi_epi16(filter[k], filter[k + 1]);
    // Equivalent to unpacking two vectors made by duplicating int16_t values.
    const __m128i intermediate =
        _mm_set1_epi32((intermediate_result_column[k + 1] << 16) |
                       intermediate_result_column[k]);
    const __m128i product_low = _mm_madd_epi16(filters_low, intermediate);
    const __m128i product_high = _mm_madd_epi16(filters_high, intermediate);
    sum_low = _mm_add_epi32(sum_low, product_low);
    sum_high = _mm_add_epi32(sum_high, product_high);
  }
  sum_low = RightShiftWithRounding_S32(sum_low, kRoundBitsVertical);
  sum_high = RightShiftWithRounding_S32(sum_high, kRoundBitsVertical);
  if (is_compound) {
    const __m128i sum = _mm_packs_epi32(sum_low, sum_high);
    StoreUnaligned16(static_cast<int16_t*>(dst_row), sum);
  } else {
    const __m128i sum = _mm_packus_epi32(sum_low, sum_high);
    StoreLo8(static_cast<uint8_t*>(dst_row), _mm_packus_epi16(sum, sum));
  }
}

template <bool is_compound, typename DestType>
inline void VerticalFilter(const int16_t source[15][8], int64_t y4, int gamma,
                           int delta, DestType* LIBGAV1_RESTRICT dest_row,
                           ptrdiff_t dest_stride) {
  int sy4 = (y4 & ((1 << kWarpedModelPrecisionBits) - 1)) - MultiplyBy4(delta);
  for (int y = 0; y < 8; ++y) {
    int sy = sy4 - MultiplyBy4(gamma);
    __m128i filter[8];
    for (__m128i& f : filter) {
      const int offset = RightShiftWithRounding(sy, kWarpedDiffPrecisionBits) +
                         kWarpedPixelPrecisionShifts;
      f = LoadUnaligned16(kWarpedFilters[offset]);
      sy += gamma;
    }
    Transpose8x8_U16(filter, filter);
    WriteVerticalFilter<is_compound>(filter, source, y, dest_row);
    dest_row += dest_stride;
    sy4 += delta;
  }
}

template <bool is_compound, typename DestType>
inline void VerticalFilter(const int16_t* LIBGAV1_RESTRICT source_cols,
                           int64_t y4, int gamma, int delta,
                           DestType* LIBGAV1_RESTRICT dest_row,
                           ptrdiff_t dest_stride) {
  int sy4 = (y4 & ((1 << kWarpedModelPrecisionBits) - 1)) - MultiplyBy4(delta);
  for (int y = 0; y < 8; ++y) {
    int sy = sy4 - MultiplyBy4(gamma);
    __m128i filter[8];
    for (__m128i& f : filter) {
      const int offset = RightShiftWithRounding(sy, kWarpedDiffPrecisionBits) +
                         kWarpedPixelPrecisionShifts;
      f = LoadUnaligned16(kWarpedFilters[offset]);
      sy += gamma;
    }
    Transpose8x8_U16(filter, filter);
    WriteVerticalFilter<is_compound>(filter, &source_cols[y], dest_row);
    dest_row += dest_stride;
    sy4 += delta;
  }
}

template <bool is_compound, typename DestType>
inline void WarpRegion1(const uint8_t* LIBGAV1_RESTRICT src,
                        ptrdiff_t source_stride, int source_width,
                        int source_height, int ix4, int iy4,
                        DestType* LIBGAV1_RESTRICT dst_row,
                        ptrdiff_t dest_stride) {
  // Region 1
  // Points to the left or right border of the first row of |src|.
  const uint8_t* first_row_border =
      (ix4 + 7 <= 0) ? src : src + source_width - 1;
  // In general, for y in [-7, 8), the row number iy4 + y is clipped:
  //   const int row = Clip3(iy4 + y, 0, source_height - 1);
  // In two special cases, iy4 + y is clipped to either 0 or
  // source_height - 1 for all y. In the rest of the cases, iy4 + y is
  // bounded and we can avoid clipping iy4 + y by relying on a reference
  // frame's boundary extension on the top and bottom.
  // Region 1.
  // Every sample used to calculate the prediction block has the same
  // value. So the whole prediction block has the same value.
  const int row = (iy4 + 7 <= 0) ? 0 : source_height - 1;
  const uint8_t row_border_pixel = first_row_border[row * source_stride];

  if (is_compound) {
    const __m128i sum =
        _mm_set1_epi16(row_border_pixel << (kInterRoundBitsVertical -
                                            kInterRoundBitsCompoundVertical));
    StoreUnaligned16(dst_row, sum);
  } else {
    memset(dst_row, row_border_pixel, 8);
  }
  const DestType* const first_dst_row = dst_row;
  dst_row += dest_stride;
  for (int y = 1; y < 8; ++y) {
    memcpy(dst_row, first_dst_row, 8 * sizeof(*dst_row));
    dst_row += dest_stride;
  }
}

template <bool is_compound, typename DestType>
inline void WarpRegion2(const uint8_t* LIBGAV1_RESTRICT src,
                        ptrdiff_t source_stride, int source_width, int64_t y4,
                        int ix4, int iy4, int gamma, int delta,
                        int16_t intermediate_result_column[15],
                        DestType* LIBGAV1_RESTRICT dst_row,
                        ptrdiff_t dest_stride) {
  // Region 2.
  // Points to the left or right border of the first row of |src|.
  const uint8_t* first_row_border =
      (ix4 + 7 <= 0) ? src : src + source_width - 1;
  // In general, for y in [-7, 8), the row number iy4 + y is clipped:
  //   const int row = Clip3(iy4 + y, 0, source_height - 1);
  // In two special cases, iy4 + y is clipped to either 0 or
  // source_height - 1 for all y. In the rest of the cases, iy4 + y is
  // bounded and we can avoid clipping iy4 + y by relying on a reference
  // frame's boundary extension on the top and bottom.

  // Region 2.
  // Horizontal filter.
  // The input values in this region are generated by extending the border
  // which makes them identical in the horizontal direction. This
  // computation could be inlined in the vertical pass but most
  // implementations will need a transpose of some sort.
  // It is not necessary to use the offset values here because the
  // horizontal pass is a simple shift and the vertical pass will always
  // require using 32 bits.
  for (int y = -7; y < 8; ++y) {
    // We may over-read up to 13 pixels above the top source row, or up
    // to 13 pixels below the bottom source row. This is proved in
    // warp.cc.
    const int row = iy4 + y;
    int sum = first_row_border[row * source_stride];
    sum <<= (kFilterBits - kInterRoundBitsHorizontal);
    intermediate_result_column[y + 7] = sum;
  }
  // Region 2 vertical filter.
  VerticalFilter<is_compound, DestType>(intermediate_result_column, y4, gamma,
                                        delta, dst_row, dest_stride);
}

template <bool is_compound, typename DestType>
inline void WarpRegion3(const uint8_t* LIBGAV1_RESTRICT src,
                        ptrdiff_t source_stride, int source_height, int alpha,
                        int beta, int64_t x4, int ix4, int iy4,
                        int16_t intermediate_result[15][8]) {
  // Region 3
  // At this point, we know ix4 - 7 < source_width - 1 and ix4 + 7 > 0.

  // In general, for y in [-7, 8), the row number iy4 + y is clipped:
  //   const int row = Clip3(iy4 + y, 0, source_height - 1);
  // In two special cases, iy4 + y is clipped to either 0 or
  // source_height - 1 for all y. In the rest of the cases, iy4 + y is
  // bounded and we can avoid clipping iy4 + y by relying on a reference
  // frame's boundary extension on the top and bottom.
  // Horizontal filter.
  const int row = (iy4 + 7 <= 0) ? 0 : source_height - 1;
  const uint8_t* const src_row = src + row * source_stride;
  // Read 15 samples from &src_row[ix4 - 7]. The 16th sample is also
  // read but is ignored.
  //
  // NOTE: This may read up to 13 bytes before src_row[0] or up to 14
  // bytes after src_row[source_width - 1]. We assume the source frame
  // has left and right borders of at least 13 bytes that extend the
  // frame boundary pixels. We also assume there is at least one extra
  // padding byte after the right border of the last source row.
  const __m128i src_row_v = LoadUnaligned16(&src_row[ix4 - 7]);
  int sx4 = (x4 & ((1 << kWarpedModelPrecisionBits) - 1)) - beta * 7;
  for (int y = -7; y < 8; ++y) {
    HorizontalFilter(sx4, alpha, src_row_v, intermediate_result[y + 7]);
    sx4 += beta;
  }
}

template <bool is_compound, typename DestType>
inline void WarpRegion4(const uint8_t* LIBGAV1_RESTRICT src,
                        ptrdiff_t source_stride, int alpha, int beta,
                        int64_t x4, int ix4, int iy4,
                        int16_t intermediate_result[15][8]) {
  // Region 4.
  // At this point, we know ix4 - 7 < source_width - 1 and ix4 + 7 > 0.

  // In general, for y in [-7, 8), the row number iy4 + y is clipped:
  //   const int row = Clip3(iy4 + y, 0, source_height - 1);
  // In two special cases, iy4 + y is clipped to either 0 or
  // source_height - 1 for all y. In the rest of the cases, iy4 + y is
  // bounded and we can avoid clipping iy4 + y by relying on a reference
  // frame's boundary extension on the top and bottom.
  // Horizontal filter.
  int sx4 = (x4 & ((1 << kWarpedModelPrecisionBits) - 1)) - beta * 7;
  for (int y = -7; y < 8; ++y) {
    // We may over-read up to 13 pixels above the top source row, or up
    // to 13 pixels below the bottom source row. This is proved in
    // warp.cc.
    const int row = iy4 + y;
    const uint8_t* const src_row = src + row * source_stride;
    // Read 15 samples from &src_row[ix4 - 7]. The 16th sample is also
    // read but is ignored.
    //
    // NOTE: This may read up to 13 bytes before src_row[0] or up to 14
    // bytes after src_row[source_width - 1]. We assume the source frame
    // has left and right borders of at least 13 bytes that extend the
    // frame boundary pixels. We also assume there is at least one extra
    // padding byte after the right border of the last source row.
    const __m128i src_row_v = LoadUnaligned16(&src_row[ix4 - 7]);
    // Convert src_row_v to int8 (subtract 128).
    HorizontalFilter(sx4, alpha, src_row_v, intermediate_result[y + 7]);
    sx4 += beta;
  }
}

template <bool is_compound, typename DestType>
inline void HandleWarpBlock(const uint8_t* LIBGAV1_RESTRICT src,
                            ptrdiff_t source_stride, int source_width,
                            int source_height,
                            const int* LIBGAV1_RESTRICT warp_params,
                            int subsampling_x, int subsampling_y, int src_x,
                            int src_y, int16_t alpha, int16_t beta,
                            int16_t gamma, int16_t delta,
                            DestType* LIBGAV1_RESTRICT dst_row,
                            ptrdiff_t dest_stride) {
  union {
    // Intermediate_result is the output of the horizontal filtering and
    // rounding. The range is within 13 (= bitdepth + kFilterBits + 1 -
    // kInterRoundBitsHorizontal) bits (unsigned). We use the signed int16_t
    // type so that we can start with a negative offset and restore it on the
    // final filter sum.
    int16_t intermediate_result[15][8];  // 15 rows, 8 columns.
    // In the simple special cases where the samples in each row are all the
    // same, store one sample per row in a column vector.
    int16_t intermediate_result_column[15];
  };

  const WarpFilterParams filter_params = GetWarpFilterParams(
      src_x, src_y, subsampling_x, subsampling_y, warp_params);
  // A prediction block may fall outside the frame's boundaries. If a
  // prediction block is calculated using only samples outside the frame's
  // boundary, the filtering can be simplified. We can divide the plane
  // into several regions and handle them differently.
  //
  //                |           |
  //            1   |     3     |   1
  //                |           |
  //         -------+-----------+-------
  //                |***********|
  //            2   |*****4*****|   2
  //                |***********|
  //         -------+-----------+-------
  //                |           |
  //            1   |     3     |   1
  //                |           |
  //
  // At the center, region 4 represents the frame and is the general case.
  //
  // In regions 1 and 2, the prediction block is outside the frame's
  // boundary horizontally. Therefore the horizontal filtering can be
  // simplified. Furthermore, in the region 1 (at the four corners), the
  // prediction is outside the frame's boundary both horizontally and
  // vertically, so we get a constant prediction block.
  //
  // In region 3, the prediction block is outside the frame's boundary
  // vertically. Unfortunately because we apply the horizontal filters
  // first, by the time we apply the vertical filters, they no longer see
  // simple inputs. So the only simplification is that all the rows are
  // the same, but we still need to apply all the horizontal and vertical
  // filters.

  // Check for two simple special cases, where the horizontal filter can
  // be significantly simplified.
  //
  // In general, for each row, the horizontal filter is calculated as
  // follows:
  //   for (int x = -4; x < 4; ++x) {
  //     const int offset = ...;
  //     int sum = first_pass_offset;
  //     for (int k = 0; k < 8; ++k) {
  //       const int column = Clip3(ix4 + x + k - 3, 0, source_width - 1);
  //       sum += kWarpedFilters[offset][k] * src_row[column];
  //     }
  //     ...
  //   }
  // The column index before clipping, ix4 + x + k - 3, varies in the range
  // ix4 - 7 <= ix4 + x + k - 3 <= ix4 + 7. If ix4 - 7 >= source_width - 1
  // or ix4 + 7 <= 0, then all the column indexes are clipped to the same
  // border index (source_width - 1 or 0, respectively). Then for each x,
  // the inner for loop of the horizontal filter is reduced to multiplying
  // the border pixel by the sum of the filter coefficients.
  if (filter_params.ix4 - 7 >= source_width - 1 || filter_params.ix4 + 7 <= 0) {
    if ((filter_params.iy4 - 7 >= source_height - 1 ||
         filter_params.iy4 + 7 <= 0)) {
      // Outside the frame in both directions. One repeated value.
      WarpRegion1<is_compound, DestType>(
          src, source_stride, source_width, source_height, filter_params.ix4,
          filter_params.iy4, dst_row, dest_stride);
      return;
    }
    // Outside the frame horizontally. Rows repeated.
    WarpRegion2<is_compound, DestType>(
        src, source_stride, source_width, filter_params.y4, filter_params.ix4,
        filter_params.iy4, gamma, delta, intermediate_result_column, dst_row,
        dest_stride);
    return;
  }

  if ((filter_params.iy4 - 7 >= source_height - 1 ||
       filter_params.iy4 + 7 <= 0)) {
    // Outside the frame vertically.
    WarpRegion3<is_compound, DestType>(
        src, source_stride, source_height, alpha, beta, filter_params.x4,
        filter_params.ix4, filter_params.iy4, intermediate_result);
  } else {
    // Inside the frame.
    WarpRegion4<is_compound, DestType>(src, source_stride, alpha, beta,
                                       filter_params.x4, filter_params.ix4,
                                       filter_params.iy4, intermediate_result);
  }
  // Region 3 and 4 vertical filter.
  VerticalFilter<is_compound, DestType>(intermediate_result, filter_params.y4,
                                        gamma, delta, dst_row, dest_stride);
}

template <bool is_compound>
void Warp_SSE4_1(const void* LIBGAV1_RESTRICT source, ptrdiff_t source_stride,
                 int source_width, int source_height,
                 const int* LIBGAV1_RESTRICT warp_params, int subsampling_x,
                 int subsampling_y, int block_start_x, int block_start_y,
                 int block_width, int block_height, int16_t alpha, int16_t beta,
                 int16_t gamma, int16_t delta, void* LIBGAV1_RESTRICT dest,
                 ptrdiff_t dest_stride) {
  const auto* const src = static_cast<const uint8_t*>(source);
  using DestType =
      typename std::conditional<is_compound, int16_t, uint8_t>::type;
  auto* dst = static_cast<DestType*>(dest);

  // Warp process applies for each 8x8 block.
  assert(block_width >= 8);
  assert(block_height >= 8);
  const int block_end_x = block_start_x + block_width;
  const int block_end_y = block_start_y + block_height;

  const int start_x = block_start_x;
  const int start_y = block_start_y;
  int src_x = (start_x + 4) << subsampling_x;
  int src_y = (start_y + 4) << subsampling_y;
  const int end_x = (block_end_x + 4) << subsampling_x;
  const int end_y = (block_end_y + 4) << subsampling_y;
  do {
    DestType* dst_row = dst;
    src_x = (start_x + 4) << subsampling_x;
    do {
      HandleWarpBlock<is_compound, DestType>(
          src, source_stride, source_width, source_height, warp_params,
          subsampling_x, subsampling_y, src_x, src_y, alpha, beta, gamma, delta,
          dst_row, dest_stride);
      src_x += (8 << subsampling_x);
      dst_row += 8;
    } while (src_x < end_x);
    dst += 8 * dest_stride;
    src_y += (8 << subsampling_y);
  } while (src_y < end_y);
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
  dsp->warp = Warp_SSE4_1</*is_compound=*/false>;
  dsp->warp_compound = Warp_SSE4_1</*is_compound=*/true>;
}

}  // namespace
}  // namespace low_bitdepth

void WarpInit_SSE4_1() { low_bitdepth::Init8bpp(); }

}  // namespace dsp
}  // namespace libgav1
#else   // !LIBGAV1_TARGETING_SSE4_1

namespace libgav1 {
namespace dsp {

void WarpInit_SSE4_1() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_TARGETING_SSE4_1
