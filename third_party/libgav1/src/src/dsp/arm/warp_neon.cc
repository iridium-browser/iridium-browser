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

#include "src/dsp/warp.h"
#include "src/utils/cpu.h"

#if LIBGAV1_ENABLE_NEON

#include <arm_neon.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <type_traits>

#include "src/dsp/arm/common_neon.h"
#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"

namespace libgav1 {
namespace dsp {
namespace {

// Number of extra bits of precision in warped filtering.
constexpr int kWarpedDiffPrecisionBits = 10;

}  // namespace

namespace low_bitdepth {
namespace {

constexpr int kFirstPassOffset = 1 << 14;
constexpr int kOffsetRemoval =
    (kFirstPassOffset >> kInterRoundBitsHorizontal) * 128;

// Applies the horizontal filter to one source row and stores the result in
// |intermediate_result_row|. |intermediate_result_row| is a row in the 15x8
// |intermediate_result| two-dimensional array.
//
// src_row_centered contains 16 "centered" samples of a source row. (We center
// the samples by subtracting 128 from the samples.)
void HorizontalFilter(const int sx4, const int16_t alpha,
                      const int8x16_t src_row_centered,
                      int16_t intermediate_result_row[8]) {
  int sx = sx4 - MultiplyBy4(alpha);
  int8x8_t filter[8];
  for (auto& f : filter) {
    const int offset = RightShiftWithRounding(sx, kWarpedDiffPrecisionBits) +
                       kWarpedPixelPrecisionShifts;
    f = vld1_s8(kWarpedFilters8[offset]);
    sx += alpha;
  }
  Transpose8x8(filter);
  // Add kFirstPassOffset to ensure |sum| stays within uint16_t.
  // Add 128 (offset) * 128 (filter sum) (also 1 << 14) to account for the
  // centering of the source samples. These combined are 1 << 15 or -32768.
  int16x8_t sum =
      vdupq_n_s16(static_cast<int16_t>(kFirstPassOffset + 128 * 128));
  // Unrolled k = 0..7 loop. We need to manually unroll the loop because the
  // third argument (an index value) to vextq_s8() must be a constant
  // (immediate). src_row_window is a sliding window of length 8 into
  // src_row_centered.
  // k = 0.
  int8x8_t src_row_window = vget_low_s8(src_row_centered);
  sum = vmlal_s8(sum, filter[0], src_row_window);
  // k = 1.
  src_row_window = vget_low_s8(vextq_s8(src_row_centered, src_row_centered, 1));
  sum = vmlal_s8(sum, filter[1], src_row_window);
  // k = 2.
  src_row_window = vget_low_s8(vextq_s8(src_row_centered, src_row_centered, 2));
  sum = vmlal_s8(sum, filter[2], src_row_window);
  // k = 3.
  src_row_window = vget_low_s8(vextq_s8(src_row_centered, src_row_centered, 3));
  sum = vmlal_s8(sum, filter[3], src_row_window);
  // k = 4.
  src_row_window = vget_low_s8(vextq_s8(src_row_centered, src_row_centered, 4));
  sum = vmlal_s8(sum, filter[4], src_row_window);
  // k = 5.
  src_row_window = vget_low_s8(vextq_s8(src_row_centered, src_row_centered, 5));
  sum = vmlal_s8(sum, filter[5], src_row_window);
  // k = 6.
  src_row_window = vget_low_s8(vextq_s8(src_row_centered, src_row_centered, 6));
  sum = vmlal_s8(sum, filter[6], src_row_window);
  // k = 7.
  src_row_window = vget_low_s8(vextq_s8(src_row_centered, src_row_centered, 7));
  sum = vmlal_s8(sum, filter[7], src_row_window);
  // End of unrolled k = 0..7 loop.
  // Due to the offset |sum| is guaranteed to be unsigned.
  uint16x8_t sum_unsigned = vreinterpretq_u16_s16(sum);
  sum_unsigned = vrshrq_n_u16(sum_unsigned, kInterRoundBitsHorizontal);
  // After the shift |sum_unsigned| will fit into int16_t.
  vst1q_s16(intermediate_result_row, vreinterpretq_s16_u16(sum_unsigned));
}

template <bool is_compound>
void Warp_NEON(const void* LIBGAV1_RESTRICT const source,
               const ptrdiff_t source_stride, const int source_width,
               const int source_height,
               const int* LIBGAV1_RESTRICT const warp_params,
               const int subsampling_x, const int subsampling_y,
               const int block_start_x, const int block_start_y,
               const int block_width, const int block_height,
               const int16_t alpha, const int16_t beta, const int16_t gamma,
               const int16_t delta, void* LIBGAV1_RESTRICT dest,
               const ptrdiff_t dest_stride) {
  constexpr int kRoundBitsVertical =
      is_compound ? kInterRoundBitsCompoundVertical : kInterRoundBitsVertical;
  union {
    // Intermediate_result is the output of the horizontal filtering and
    // rounding. The range is within 13 (= bitdepth + kFilterBits + 1 -
    // kInterRoundBitsHorizontal) bits (unsigned). We use the signed int16_t
    // type so that we can multiply it by kWarpedFilters (which has signed
    // values) using vmlal_s16().
    int16_t intermediate_result[15][8];  // 15 rows, 8 columns.
    // In the simple special cases where the samples in each row are all the
    // same, store one sample per row in a column vector.
    int16_t intermediate_result_column[15];
  };

  const auto* const src = static_cast<const uint8_t*>(source);
  using DestType =
      typename std::conditional<is_compound, int16_t, uint8_t>::type;
  auto* dst = static_cast<DestType*>(dest);

  assert(block_width >= 8);
  assert(block_height >= 8);

  // Warp process applies for each 8x8 block.
  int start_y = block_start_y;
  do {
    int start_x = block_start_x;
    do {
      const int src_x = (start_x + 4) << subsampling_x;
      const int src_y = (start_y + 4) << subsampling_y;
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
      if (filter_params.ix4 - 7 >= source_width - 1 ||
          filter_params.ix4 + 7 <= 0) {
        // Regions 1 and 2.
        // Points to the left or right border of the first row of |src|.
        const uint8_t* first_row_border =
            (filter_params.ix4 + 7 <= 0) ? src : src + source_width - 1;
        // In general, for y in [-7, 8), the row number iy4 + y is clipped:
        //   const int row = Clip3(iy4 + y, 0, source_height - 1);
        // In two special cases, iy4 + y is clipped to either 0 or
        // source_height - 1 for all y. In the rest of the cases, iy4 + y is
        // bounded and we can avoid clipping iy4 + y by relying on a reference
        // frame's boundary extension on the top and bottom.
        if (filter_params.iy4 - 7 >= source_height - 1 ||
            filter_params.iy4 + 7 <= 0) {
          // Region 1.
          // Every sample used to calculate the prediction block has the same
          // value. So the whole prediction block has the same value.
          const int row = (filter_params.iy4 + 7 <= 0) ? 0 : source_height - 1;
          const uint8_t row_border_pixel =
              first_row_border[row * source_stride];

          DestType* dst_row = dst + start_x - block_start_x;
          for (int y = 0; y < 8; ++y) {
            if (is_compound) {
              const int16x8_t sum =
                  vdupq_n_s16(row_border_pixel << (kInterRoundBitsVertical -
                                                   kRoundBitsVertical));
              vst1q_s16(reinterpret_cast<int16_t*>(dst_row), sum);
            } else {
              memset(dst_row, row_border_pixel, 8);
            }
            dst_row += dest_stride;
          }
          // End of region 1. Continue the |start_x| do-while loop.
          start_x += 8;
          continue;
        }

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
          const int row = filter_params.iy4 + y;
          int sum = first_row_border[row * source_stride];
          sum <<= (kFilterBits - kInterRoundBitsHorizontal);
          intermediate_result_column[y + 7] = sum;
        }
        // Vertical filter.
        DestType* dst_row = dst + start_x - block_start_x;
        int sy4 = (filter_params.y4 & ((1 << kWarpedModelPrecisionBits) - 1)) -
                  MultiplyBy4(delta);
        for (int y = 0; y < 8; ++y) {
          int sy = sy4 - MultiplyBy4(gamma);
#if defined(__aarch64__)
          const int16x8_t intermediate =
              vld1q_s16(&intermediate_result_column[y]);
          int16_t tmp[8];
          for (int x = 0; x < 8; ++x) {
            const int offset =
                RightShiftWithRounding(sy, kWarpedDiffPrecisionBits) +
                kWarpedPixelPrecisionShifts;
            const int16x8_t filter = vld1q_s16(kWarpedFilters[offset]);
            const int32x4_t product_low =
                vmull_s16(vget_low_s16(filter), vget_low_s16(intermediate));
            const int32x4_t product_high =
                vmull_s16(vget_high_s16(filter), vget_high_s16(intermediate));
            // vaddvq_s32 is only available on __aarch64__.
            const int32_t sum =
                vaddvq_s32(product_low) + vaddvq_s32(product_high);
            const int16_t sum_descale =
                RightShiftWithRounding(sum, kRoundBitsVertical);
            if (is_compound) {
              dst_row[x] = sum_descale;
            } else {
              tmp[x] = sum_descale;
            }
            sy += gamma;
          }
          if (!is_compound) {
            const int16x8_t sum = vld1q_s16(tmp);
            vst1_u8(reinterpret_cast<uint8_t*>(dst_row), vqmovun_s16(sum));
          }
#else   // !defined(__aarch64__)
          int16x8_t filter[8];
          for (int x = 0; x < 8; ++x) {
            const int offset =
                RightShiftWithRounding(sy, kWarpedDiffPrecisionBits) +
                kWarpedPixelPrecisionShifts;
            filter[x] = vld1q_s16(kWarpedFilters[offset]);
            sy += gamma;
          }
          Transpose8x8(filter);
          int32x4_t sum_low = vdupq_n_s32(0);
          int32x4_t sum_high = sum_low;
          for (int k = 0; k < 8; ++k) {
            const int16_t intermediate = intermediate_result_column[y + k];
            sum_low =
                vmlal_n_s16(sum_low, vget_low_s16(filter[k]), intermediate);
            sum_high =
                vmlal_n_s16(sum_high, vget_high_s16(filter[k]), intermediate);
          }
          const int16x8_t sum =
              vcombine_s16(vrshrn_n_s32(sum_low, kRoundBitsVertical),
                           vrshrn_n_s32(sum_high, kRoundBitsVertical));
          if (is_compound) {
            vst1q_s16(reinterpret_cast<int16_t*>(dst_row), sum);
          } else {
            vst1_u8(reinterpret_cast<uint8_t*>(dst_row), vqmovun_s16(sum));
          }
#endif  // defined(__aarch64__)
          dst_row += dest_stride;
          sy4 += delta;
        }
        // End of region 2. Continue the |start_x| do-while loop.
        start_x += 8;
        continue;
      }

      // Regions 3 and 4.
      // At this point, we know ix4 - 7 < source_width - 1 and ix4 + 7 > 0.

      // In general, for y in [-7, 8), the row number iy4 + y is clipped:
      //   const int row = Clip3(iy4 + y, 0, source_height - 1);
      // In two special cases, iy4 + y is clipped to either 0 or
      // source_height - 1 for all y. In the rest of the cases, iy4 + y is
      // bounded and we can avoid clipping iy4 + y by relying on a reference
      // frame's boundary extension on the top and bottom.
      if (filter_params.iy4 - 7 >= source_height - 1 ||
          filter_params.iy4 + 7 <= 0) {
        // Region 3.
        // Horizontal filter.
        const int row = (filter_params.iy4 + 7 <= 0) ? 0 : source_height - 1;
        const uint8_t* const src_row = src + row * source_stride;
        // Read 15 samples from &src_row[ix4 - 7]. The 16th sample is also
        // read but is ignored.
        //
        // NOTE: This may read up to 13 bytes before src_row[0] or up to 14
        // bytes after src_row[source_width - 1]. We assume the source frame
        // has left and right borders of at least 13 bytes that extend the
        // frame boundary pixels. We also assume there is at least one extra
        // padding byte after the right border of the last source row.
        const uint8x16_t src_row_v = vld1q_u8(&src_row[filter_params.ix4 - 7]);
        // Convert src_row_v to int8 (subtract 128).
        const int8x16_t src_row_centered =
            vreinterpretq_s8_u8(vsubq_u8(src_row_v, vdupq_n_u8(128)));
        int sx4 = (filter_params.x4 & ((1 << kWarpedModelPrecisionBits) - 1)) -
                  beta * 7;
        for (int y = -7; y < 8; ++y) {
          HorizontalFilter(sx4, alpha, src_row_centered,
                           intermediate_result[y + 7]);
          sx4 += beta;
        }
      } else {
        // Region 4.
        // Horizontal filter.
        int sx4 = (filter_params.x4 & ((1 << kWarpedModelPrecisionBits) - 1)) -
                  beta * 7;
        for (int y = -7; y < 8; ++y) {
          // We may over-read up to 13 pixels above the top source row, or up
          // to 13 pixels below the bottom source row. This is proved in
          // warp.cc.
          const int row = filter_params.iy4 + y;
          const uint8_t* const src_row = src + row * source_stride;
          // Read 15 samples from &src_row[ix4 - 7]. The 16th sample is also
          // read but is ignored.
          //
          // NOTE: This may read up to 13 bytes before src_row[0] or up to 14
          // bytes after src_row[source_width - 1]. We assume the source frame
          // has left and right borders of at least 13 bytes that extend the
          // frame boundary pixels. We also assume there is at least one extra
          // padding byte after the right border of the last source row.
          const uint8x16_t src_row_v =
              vld1q_u8(&src_row[filter_params.ix4 - 7]);
          // Convert src_row_v to int8 (subtract 128).
          const int8x16_t src_row_centered =
              vreinterpretq_s8_u8(vsubq_u8(src_row_v, vdupq_n_u8(128)));
          HorizontalFilter(sx4, alpha, src_row_centered,
                           intermediate_result[y + 7]);
          sx4 += beta;
        }
      }

      // Regions 3 and 4.
      // Vertical filter.
      DestType* dst_row = dst + start_x - block_start_x;
      int sy4 = (filter_params.y4 & ((1 << kWarpedModelPrecisionBits) - 1)) -
                MultiplyBy4(delta);
      for (int y = 0; y < 8; ++y) {
        int sy = sy4 - MultiplyBy4(gamma);
        int16x8_t filter[8];
        for (auto& f : filter) {
          const int offset =
              RightShiftWithRounding(sy, kWarpedDiffPrecisionBits) +
              kWarpedPixelPrecisionShifts;
          f = vld1q_s16(kWarpedFilters[offset]);
          sy += gamma;
        }
        Transpose8x8(filter);
        int32x4_t sum_low = vdupq_n_s32(-kOffsetRemoval);
        int32x4_t sum_high = sum_low;
        for (int k = 0; k < 8; ++k) {
          const int16x8_t intermediate = vld1q_s16(intermediate_result[y + k]);
          sum_low = vmlal_s16(sum_low, vget_low_s16(filter[k]),
                              vget_low_s16(intermediate));
          sum_high = vmlal_s16(sum_high, vget_high_s16(filter[k]),
                               vget_high_s16(intermediate));
        }
        const int16x8_t sum =
            vcombine_s16(vrshrn_n_s32(sum_low, kRoundBitsVertical),
                         vrshrn_n_s32(sum_high, kRoundBitsVertical));
        if (is_compound) {
          vst1q_s16(reinterpret_cast<int16_t*>(dst_row), sum);
        } else {
          vst1_u8(reinterpret_cast<uint8_t*>(dst_row), vqmovun_s16(sum));
        }
        dst_row += dest_stride;
        sy4 += delta;
      }
      start_x += 8;
    } while (start_x < block_start_x + block_width);
    dst += 8 * dest_stride;
    start_y += 8;
  } while (start_y < block_start_y + block_height);
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
  dsp->warp = Warp_NEON</*is_compound=*/false>;
  dsp->warp_compound = Warp_NEON</*is_compound=*/true>;
}

}  // namespace
}  // namespace low_bitdepth

//------------------------------------------------------------------------------
#if LIBGAV1_MAX_BITDEPTH >= 10
namespace high_bitdepth {
namespace {

LIBGAV1_ALWAYS_INLINE uint16x8x2_t LoadSrcRow(uint16_t const* ptr) {
  uint16x8x2_t x;
  // Clang/gcc uses ldp here.
  x.val[0] = vld1q_u16(ptr);
  x.val[1] = vld1q_u16(ptr + 8);
  return x;
}

LIBGAV1_ALWAYS_INLINE void HorizontalFilter(
    const int sx4, const int16_t alpha, const uint16x8x2_t src_row,
    int16_t intermediate_result_row[8]) {
  int sx = sx4 - MultiplyBy4(alpha);
  int8x8_t filter8[8];
  for (auto& f : filter8) {
    const int offset = RightShiftWithRounding(sx, kWarpedDiffPrecisionBits) +
                       kWarpedPixelPrecisionShifts;
    f = vld1_s8(kWarpedFilters8[offset]);
    sx += alpha;
  }

  Transpose8x8(filter8);

  int16x8_t filter[8];
  for (int i = 0; i < 8; ++i) {
    filter[i] = vmovl_s8(filter8[i]);
  }

  int32x4x2_t sum;
  int16x8_t src_row_window;
  // k = 0.
  src_row_window = vreinterpretq_s16_u16(src_row.val[0]);
  sum.val[0] = vmull_s16(vget_low_s16(filter[0]), vget_low_s16(src_row_window));
  sum.val[1] = VMullHighS16(filter[0], src_row_window);
  // k = 1.
  src_row_window =
      vreinterpretq_s16_u16(vextq_u16(src_row.val[0], src_row.val[1], 1));
  sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(filter[1]),
                         vget_low_s16(src_row_window));
  sum.val[1] = VMlalHighS16(sum.val[1], filter[1], src_row_window);
  // k = 2.
  src_row_window =
      vreinterpretq_s16_u16(vextq_u16(src_row.val[0], src_row.val[1], 2));
  sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(filter[2]),
                         vget_low_s16(src_row_window));
  sum.val[1] = VMlalHighS16(sum.val[1], filter[2], src_row_window);
  // k = 3.
  src_row_window =
      vreinterpretq_s16_u16(vextq_u16(src_row.val[0], src_row.val[1], 3));
  sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(filter[3]),
                         vget_low_s16(src_row_window));
  sum.val[1] = VMlalHighS16(sum.val[1], filter[3], src_row_window);
  // k = 4.
  src_row_window =
      vreinterpretq_s16_u16(vextq_u16(src_row.val[0], src_row.val[1], 4));
  sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(filter[4]),
                         vget_low_s16(src_row_window));
  sum.val[1] = VMlalHighS16(sum.val[1], filter[4], src_row_window);
  // k = 5.
  src_row_window =
      vreinterpretq_s16_u16(vextq_u16(src_row.val[0], src_row.val[1], 5));
  sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(filter[5]),
                         vget_low_s16(src_row_window));
  sum.val[1] = VMlalHighS16(sum.val[1], filter[5], src_row_window);
  // k = 6.
  src_row_window =
      vreinterpretq_s16_u16(vextq_u16(src_row.val[0], src_row.val[1], 6));
  sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(filter[6]),
                         vget_low_s16(src_row_window));
  sum.val[1] = VMlalHighS16(sum.val[1], filter[6], src_row_window);
  // k = 7.
  src_row_window =
      vreinterpretq_s16_u16(vextq_u16(src_row.val[0], src_row.val[1], 7));
  sum.val[0] = vmlal_s16(sum.val[0], vget_low_s16(filter[7]),
                         vget_low_s16(src_row_window));
  sum.val[1] = VMlalHighS16(sum.val[1], filter[7], src_row_window);
  // End of unrolled k = 0..7 loop.

  vst1_s16(intermediate_result_row,
           vrshrn_n_s32(sum.val[0], kInterRoundBitsHorizontal));
  vst1_s16(intermediate_result_row + 4,
           vrshrn_n_s32(sum.val[1], kInterRoundBitsHorizontal));
}

template <bool is_compound>
void Warp_NEON(const void* LIBGAV1_RESTRICT const source,
               const ptrdiff_t source_stride, const int source_width,
               const int source_height,
               const int* LIBGAV1_RESTRICT const warp_params,
               const int subsampling_x, const int subsampling_y,
               const int block_start_x, const int block_start_y,
               const int block_width, const int block_height,
               const int16_t alpha, const int16_t beta, const int16_t gamma,
               const int16_t delta, void* LIBGAV1_RESTRICT dest,
               const ptrdiff_t dest_stride) {
  constexpr int kRoundBitsVertical =
      is_compound ? kInterRoundBitsCompoundVertical : kInterRoundBitsVertical;
  union {
    // Intermediate_result is the output of the horizontal filtering and
    // rounding. The range is within 13 (= bitdepth + kFilterBits + 1 -
    // kInterRoundBitsHorizontal) bits (unsigned). We use the signed int16_t
    // type so that we can multiply it by kWarpedFilters (which has signed
    // values) using vmlal_s16().
    int16_t intermediate_result[15][8];  // 15 rows, 8 columns.
    // In the simple special cases where the samples in each row are all the
    // same, store one sample per row in a column vector.
    int16_t intermediate_result_column[15];
  };

  const auto* const src = static_cast<const uint16_t*>(source);
  const ptrdiff_t src_stride = source_stride >> 1;
  using DestType =
      typename std::conditional<is_compound, int16_t, uint16_t>::type;
  auto* dst = static_cast<DestType*>(dest);
  const ptrdiff_t dst_stride = is_compound ? dest_stride : dest_stride >> 1;
  assert(block_width >= 8);
  assert(block_height >= 8);

  // Warp process applies for each 8x8 block.
  int start_y = block_start_y;
  do {
    int start_x = block_start_x;
    do {
      const int src_x = (start_x + 4) << subsampling_x;
      const int src_y = (start_y + 4) << subsampling_y;
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
      if (filter_params.ix4 - 7 >= source_width - 1 ||
          filter_params.ix4 + 7 <= 0) {
        // Regions 1 and 2.
        // Points to the left or right border of the first row of |src|.
        const uint16_t* first_row_border =
            (filter_params.ix4 + 7 <= 0) ? src : src + source_width - 1;
        // In general, for y in [-7, 8), the row number iy4 + y is clipped:
        //   const int row = Clip3(iy4 + y, 0, source_height - 1);
        // In two special cases, iy4 + y is clipped to either 0 or
        // source_height - 1 for all y. In the rest of the cases, iy4 + y is
        // bounded and we can avoid clipping iy4 + y by relying on a reference
        // frame's boundary extension on the top and bottom.
        if (filter_params.iy4 - 7 >= source_height - 1 ||
            filter_params.iy4 + 7 <= 0) {
          // Region 1.
          // Every sample used to calculate the prediction block has the same
          // value. So the whole prediction block has the same value.
          const int row = (filter_params.iy4 + 7 <= 0) ? 0 : source_height - 1;
          const uint16_t row_border_pixel = first_row_border[row * src_stride];

          DestType* dst_row = dst + start_x - block_start_x;
          for (int y = 0; y < 8; ++y) {
            if (is_compound) {
              const int16x8_t sum =
                  vdupq_n_s16(row_border_pixel << (kInterRoundBitsVertical -
                                                   kRoundBitsVertical));
              vst1q_s16(reinterpret_cast<int16_t*>(dst_row),
                        vaddq_s16(sum, vdupq_n_s16(kCompoundOffset)));
            } else {
              vst1q_u16(reinterpret_cast<uint16_t*>(dst_row),
                        vdupq_n_u16(row_border_pixel));
            }
            dst_row += dst_stride;
          }
          // End of region 1. Continue the |start_x| do-while loop.
          start_x += 8;
          continue;
        }

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
          const int row = filter_params.iy4 + y;
          int sum = first_row_border[row * src_stride];
          sum <<= (kFilterBits - kInterRoundBitsHorizontal);
          intermediate_result_column[y + 7] = sum;
        }
        // Vertical filter.
        DestType* dst_row = dst + start_x - block_start_x;
        int sy4 = (filter_params.y4 & ((1 << kWarpedModelPrecisionBits) - 1)) -
                  MultiplyBy4(delta);
        for (int y = 0; y < 8; ++y) {
          int sy = sy4 - MultiplyBy4(gamma);
#if defined(__aarch64__)
          const int16x8_t intermediate =
              vld1q_s16(&intermediate_result_column[y]);
          int16_t tmp[8];
          for (int x = 0; x < 8; ++x) {
            const int offset =
                RightShiftWithRounding(sy, kWarpedDiffPrecisionBits) +
                kWarpedPixelPrecisionShifts;
            const int16x8_t filter = vld1q_s16(kWarpedFilters[offset]);
            const int32x4_t product_low =
                vmull_s16(vget_low_s16(filter), vget_low_s16(intermediate));
            const int32x4_t product_high =
                vmull_s16(vget_high_s16(filter), vget_high_s16(intermediate));
            // vaddvq_s32 is only available on __aarch64__.
            const int32_t sum =
                vaddvq_s32(product_low) + vaddvq_s32(product_high);
            const int16_t sum_descale =
                RightShiftWithRounding(sum, kRoundBitsVertical);
            if (is_compound) {
              dst_row[x] = sum_descale + kCompoundOffset;
            } else {
              tmp[x] = sum_descale;
            }
            sy += gamma;
          }
          if (!is_compound) {
            const uint16x8_t v_max_bitdepth =
                vdupq_n_u16((1 << kBitdepth10) - 1);
            const int16x8_t sum = vld1q_s16(tmp);
            const uint16x8_t d0 =
                vminq_u16(vreinterpretq_u16_s16(vmaxq_s16(sum, vdupq_n_s16(0))),
                          v_max_bitdepth);
            vst1q_u16(reinterpret_cast<uint16_t*>(dst_row), d0);
          }
#else   // !defined(__aarch64__)
          int16x8_t filter[8];
          for (int x = 0; x < 8; ++x) {
            const int offset =
                RightShiftWithRounding(sy, kWarpedDiffPrecisionBits) +
                kWarpedPixelPrecisionShifts;
            filter[x] = vld1q_s16(kWarpedFilters[offset]);
            sy += gamma;
          }
          Transpose8x8(filter);
          int32x4_t sum_low = vdupq_n_s32(0);
          int32x4_t sum_high = sum_low;
          for (int k = 0; k < 8; ++k) {
            const int16_t intermediate = intermediate_result_column[y + k];
            sum_low =
                vmlal_n_s16(sum_low, vget_low_s16(filter[k]), intermediate);
            sum_high =
                vmlal_n_s16(sum_high, vget_high_s16(filter[k]), intermediate);
          }
          if (is_compound) {
            const int16x8_t sum =
                vcombine_s16(vrshrn_n_s32(sum_low, kRoundBitsVertical),
                             vrshrn_n_s32(sum_high, kRoundBitsVertical));
            vst1q_s16(reinterpret_cast<int16_t*>(dst_row),
                      vaddq_s16(sum, vdupq_n_s16(kCompoundOffset)));
          } else {
            const uint16x4_t v_max_bitdepth =
                vdup_n_u16((1 << kBitdepth10) - 1);
            const uint16x4_t d0 = vmin_u16(
                vqrshrun_n_s32(sum_low, kRoundBitsVertical), v_max_bitdepth);
            const uint16x4_t d1 = vmin_u16(
                vqrshrun_n_s32(sum_high, kRoundBitsVertical), v_max_bitdepth);
            vst1_u16(reinterpret_cast<uint16_t*>(dst_row), d0);
            vst1_u16(reinterpret_cast<uint16_t*>(dst_row + 4), d1);
          }
#endif  // defined(__aarch64__)
          dst_row += dst_stride;
          sy4 += delta;
        }
        // End of region 2. Continue the |start_x| do-while loop.
        start_x += 8;
        continue;
      }

      // Regions 3 and 4.
      // At this point, we know ix4 - 7 < source_width - 1 and ix4 + 7 > 0.

      // In general, for y in [-7, 8), the row number iy4 + y is clipped:
      //   const int row = Clip3(iy4 + y, 0, source_height - 1);
      // In two special cases, iy4 + y is clipped to either 0 or
      // source_height - 1 for all y. In the rest of the cases, iy4 + y is
      // bounded and we can avoid clipping iy4 + y by relying on a reference
      // frame's boundary extension on the top and bottom.
      if (filter_params.iy4 - 7 >= source_height - 1 ||
          filter_params.iy4 + 7 <= 0) {
        // Region 3.
        // Horizontal filter.
        const int row = (filter_params.iy4 + 7 <= 0) ? 0 : source_height - 1;
        const uint16_t* const src_row = src + row * src_stride;
        // Read 15 samples from &src_row[ix4 - 7]. The 16th sample is also
        // read but is ignored.
        //
        // NOTE: This may read up to 13 pixels before src_row[0] or up to 14
        // pixels after src_row[source_width - 1]. We assume the source frame
        // has left and right borders of at least 13 pixels that extend the
        // frame boundary pixels. We also assume there is at least one extra
        // padding pixel after the right border of the last source row.
        const uint16x8x2_t src_row_v =
            LoadSrcRow(&src_row[filter_params.ix4 - 7]);
        int sx4 = (filter_params.x4 & ((1 << kWarpedModelPrecisionBits) - 1)) -
                  beta * 7;
        for (int y = -7; y < 8; ++y) {
          HorizontalFilter(sx4, alpha, src_row_v, intermediate_result[y + 7]);
          sx4 += beta;
        }
      } else {
        // Region 4.
        // Horizontal filter.
        int sx4 = (filter_params.x4 & ((1 << kWarpedModelPrecisionBits) - 1)) -
                  beta * 7;
        for (int y = -7; y < 8; ++y) {
          // We may over-read up to 13 pixels above the top source row, or up
          // to 13 pixels below the bottom source row. This is proved in
          // warp.cc.
          const int row = filter_params.iy4 + y;
          const uint16_t* const src_row = src + row * src_stride;
          // Read 15 samples from &src_row[ix4 - 7]. The 16th sample is also
          // read but is ignored.
          //
          // NOTE: This may read up to pixels bytes before src_row[0] or up to
          // 14 pixels after src_row[source_width - 1]. We assume the source
          // frame has left and right borders of at least 13 pixels that extend
          // the frame boundary pixels. We also assume there is at least one
          // extra padding pixel after the right border of the last source row.
          const uint16x8x2_t src_row_v =
              LoadSrcRow(&src_row[filter_params.ix4 - 7]);
          HorizontalFilter(sx4, alpha, src_row_v, intermediate_result[y + 7]);
          sx4 += beta;
        }
      }

      // Regions 3 and 4.
      // Vertical filter.
      DestType* dst_row = dst + start_x - block_start_x;
      int sy4 = (filter_params.y4 & ((1 << kWarpedModelPrecisionBits) - 1)) -
                MultiplyBy4(delta);
      for (int y = 0; y < 8; ++y) {
        int sy = sy4 - MultiplyBy4(gamma);
        int16x8_t filter[8];
        for (auto& f : filter) {
          const int offset =
              RightShiftWithRounding(sy, kWarpedDiffPrecisionBits) +
              kWarpedPixelPrecisionShifts;
          f = vld1q_s16(kWarpedFilters[offset]);
          sy += gamma;
        }
        Transpose8x8(filter);
        int32x4_t sum_low = vdupq_n_s32(0);
        int32x4_t sum_high = sum_low;
        for (int k = 0; k < 8; ++k) {
          const int16x8_t intermediate = vld1q_s16(intermediate_result[y + k]);
          sum_low = vmlal_s16(sum_low, vget_low_s16(filter[k]),
                              vget_low_s16(intermediate));
          sum_high = vmlal_s16(sum_high, vget_high_s16(filter[k]),
                               vget_high_s16(intermediate));
        }
        if (is_compound) {
          const int16x8_t sum =
              vcombine_s16(vrshrn_n_s32(sum_low, kRoundBitsVertical),
                           vrshrn_n_s32(sum_high, kRoundBitsVertical));
          vst1q_s16(reinterpret_cast<int16_t*>(dst_row),
                    vaddq_s16(sum, vdupq_n_s16(kCompoundOffset)));
        } else {
          const uint16x4_t v_max_bitdepth = vdup_n_u16((1 << kBitdepth10) - 1);
          const uint16x4_t d0 = vmin_u16(
              vqrshrun_n_s32(sum_low, kRoundBitsVertical), v_max_bitdepth);
          const uint16x4_t d1 = vmin_u16(
              vqrshrun_n_s32(sum_high, kRoundBitsVertical), v_max_bitdepth);
          vst1_u16(reinterpret_cast<uint16_t*>(dst_row), d0);
          vst1_u16(reinterpret_cast<uint16_t*>(dst_row + 4), d1);
        }
        dst_row += dst_stride;
        sy4 += delta;
      }
      start_x += 8;
    } while (start_x < block_start_x + block_width);
    dst += 8 * dst_stride;
    start_y += 8;
  } while (start_y < block_start_y + block_height);
}

void Init10bpp() {
  Dsp* dsp = dsp_internal::GetWritableDspTable(kBitdepth10);
  assert(dsp != nullptr);
  dsp->warp = Warp_NEON</*is_compound=*/false>;
  dsp->warp_compound = Warp_NEON</*is_compound=*/true>;
}

}  // namespace
}  // namespace high_bitdepth
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

void WarpInit_NEON() {
  low_bitdepth::Init8bpp();
#if LIBGAV1_MAX_BITDEPTH >= 10
  high_bitdepth::Init10bpp();
#endif
}

}  // namespace dsp
}  // namespace libgav1
#else   // !LIBGAV1_ENABLE_NEON
namespace libgav1 {
namespace dsp {

void WarpInit_NEON() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_ENABLE_NEON
