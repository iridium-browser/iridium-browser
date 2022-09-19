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

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <type_traits>

#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/memory.h"

namespace libgav1 {
namespace dsp {
namespace {

// Number of extra bits of precision in warped filtering.
constexpr int kWarpedDiffPrecisionBits = 10;

// Warp prediction output ranges from WarpTest.ShowRange.
// Bitdepth:  8 Input range:            [       0,      255]
//   8bpp intermediate offset: 16384.
//   intermediate range:                [    4399,    61009]
//   first pass output range:           [     550,     7626]
//   8bpp intermediate offset removal: 262144.
//   intermediate range:                [ -620566,  1072406]
//   second pass output range:          [       0,      255]
//   compound second pass output range: [   -4848,     8378]
//
// Bitdepth: 10 Input range:            [       0,     1023]
//   intermediate range:                [  -48081,   179025]
//   first pass output range:           [   -6010,    22378]
//   intermediate range:                [-2103516,  4198620]
//   second pass output range:          [       0,     1023]
//   compound second pass output range: [    8142,    57378]
//
// Bitdepth: 12 Input range:            [       0,     4095]
//   intermediate range:                [ -192465,   716625]
//   first pass output range:           [   -6015,    22395]
//   intermediate range:                [-2105190,  4201830]
//   second pass output range:          [       0,     4095]
//   compound second pass output range: [    8129,    57403]

template <bool is_compound, int bitdepth, typename Pixel>
void Warp_C(const void* LIBGAV1_RESTRICT const source, ptrdiff_t source_stride,
            const int source_width, const int source_height,
            const int* LIBGAV1_RESTRICT const warp_params,
            const int subsampling_x, const int subsampling_y,
            const int block_start_x, const int block_start_y,
            const int block_width, const int block_height, const int16_t alpha,
            const int16_t beta, const int16_t gamma, const int16_t delta,
            void* LIBGAV1_RESTRICT dest, ptrdiff_t dest_stride) {
  assert(block_width >= 8 && block_height >= 8);
  if (is_compound) {
    assert(dest_stride == block_width);
  }
  constexpr int kRoundBitsHorizontal = (bitdepth == 12)
                                           ? kInterRoundBitsHorizontal12bpp
                                           : kInterRoundBitsHorizontal;
  constexpr int kRoundBitsVertical =
      is_compound        ? kInterRoundBitsCompoundVertical
      : (bitdepth == 12) ? kInterRoundBitsVertical12bpp
                         : kInterRoundBitsVertical;

  // Only used for 8bpp. Allows for keeping the first pass intermediates within
  // uint16_t. With 10/12bpp the intermediate value will always require int32_t.
  constexpr int first_pass_offset = (bitdepth == 8) ? 1 << 14 : 0;
  constexpr int offset_removal =
      (first_pass_offset >> kRoundBitsHorizontal) * 128;

  constexpr int kMaxPixel = (1 << bitdepth) - 1;
  union {
    // |intermediate_result| is the output of the horizontal filtering and
    // rounding. The range is within int16_t.
    int16_t intermediate_result[15][8];  // 15 rows, 8 columns.
    // In the simple special cases where the samples in each row are all the
    // same, store one sample per row in a column vector.
    int16_t intermediate_result_column[15];
  };
  const auto* const src = static_cast<const Pixel*>(source);
  source_stride /= sizeof(Pixel);
  using DestType =
      typename std::conditional<is_compound, uint16_t, Pixel>::type;
  auto* dst = static_cast<DestType*>(dest);
  if (!is_compound) dest_stride /= sizeof(dst[0]);

  assert(block_width >= 8);
  assert(block_height >= 8);

  // Warp process applies for each 8x8 block (or smaller).
  for (int start_y = block_start_y; start_y < block_start_y + block_height;
       start_y += 8) {
    for (int start_x = block_start_x; start_x < block_start_x + block_width;
         start_x += 8) {
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
        const Pixel* first_row_border =
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
          const Pixel row_border_pixel = first_row_border[row * source_stride];
          DestType* dst_row = dst + start_x - block_start_x;
          if (is_compound) {
            int sum = row_border_pixel
                      << ((14 - kRoundBitsHorizontal) - kRoundBitsVertical);
            sum += (bitdepth == 8) ? 0 : kCompoundOffset;
            Memset(dst_row, sum, 8);
          } else {
            Memset(dst_row, row_border_pixel, 8);
          }
          const DestType* const first_dst_row = dst_row;
          dst_row += dest_stride;
          for (int y = 1; y < 8; ++y) {
            memcpy(dst_row, first_dst_row, 8 * sizeof(*dst_row));
            dst_row += dest_stride;
          }
          // End of region 1. Continue the |start_x| for loop.
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
          // to 13 pixels below the bottom source row. This is proved below.
          const int row = filter_params.iy4 + y;
          int sum = first_row_border[row * source_stride];
          sum <<= kFilterBits - kRoundBitsHorizontal;
          intermediate_result_column[y + 7] = sum;
        }
        // Vertical filter.
        DestType* dst_row = dst + start_x - block_start_x;
        int sy4 = (filter_params.y4 & ((1 << kWarpedModelPrecisionBits) - 1)) -
                  MultiplyBy4(delta);
        for (int y = 0; y < 8; ++y) {
          int sy = sy4 - MultiplyBy4(gamma);
          for (int x = 0; x < 8; ++x) {
            const int offset =
                RightShiftWithRounding(sy, kWarpedDiffPrecisionBits) +
                kWarpedPixelPrecisionShifts;
            assert(offset >= 0);
            assert(offset < 3 * kWarpedPixelPrecisionShifts + 1);
            int sum = 0;
            for (int k = 0; k < 8; ++k) {
              sum +=
                  kWarpedFilters[offset][k] * intermediate_result_column[y + k];
            }
            sum = RightShiftWithRounding(sum, kRoundBitsVertical);
            if (is_compound) {
              sum += (bitdepth == 8) ? 0 : kCompoundOffset;
              dst_row[x] = static_cast<DestType>(sum);
            } else {
              dst_row[x] = static_cast<DestType>(Clip3(sum, 0, kMaxPixel));
            }
            sy += gamma;
          }
          dst_row += dest_stride;
          sy4 += delta;
        }
        // End of region 2. Continue the |start_x| for loop.
        continue;
      }

      // Regions 3 and 4.
      // At this point, we know ix4 - 7 < source_width - 1 and ix4 + 7 > 0.
      // It follows that -6 <= ix4 <= source_width + 5. This inequality is
      // used below.

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
        const Pixel* const src_row = src + row * source_stride;
        int sx4 = (filter_params.x4 & ((1 << kWarpedModelPrecisionBits) - 1)) -
                  beta * 7;
        for (int y = -7; y < 8; ++y) {
          int sx = sx4 - MultiplyBy4(alpha);
          for (int x = -4; x < 4; ++x) {
            const int offset =
                RightShiftWithRounding(sx, kWarpedDiffPrecisionBits) +
                kWarpedPixelPrecisionShifts;
            // Since alpha and beta have been validated by SetupShear(), one
            // can prove that 0 <= offset <= 3 * 2^6.
            assert(offset >= 0);
            assert(offset < 3 * kWarpedPixelPrecisionShifts + 1);
            // For SIMD optimization:
            // |first_pass_offset| guarantees the sum fits in uint16_t for 8bpp.
            // For 10/12 bit, the range of sum requires 32 bits.
            int sum = first_pass_offset;
            for (int k = 0; k < 8; ++k) {
              // We assume the source frame has left and right borders of at
              // least 13 pixels that extend the frame boundary pixels.
              //
              // Since -4 <= x <= 3 and 0 <= k <= 7, using the inequality on
              // ix4 above, we have
              //   -13 <= ix4 + x + k - 3 <= source_width + 12,
              // or
              //   -13 <= column <= (source_width - 1) + 13.
              // Therefore we may over-read up to 13 pixels before the source
              // row, or up to 13 pixels after the source row.
              const int column = filter_params.ix4 + x + k - 3;
              sum += kWarpedFilters[offset][k] * src_row[column];
            }
            intermediate_result[y + 7][x + 4] =
                RightShiftWithRounding(sum, kRoundBitsHorizontal);
            sx += alpha;
          }
          sx4 += beta;
        }
      } else {
        // Region 4.
        // Horizontal filter.
        // At this point, we know iy4 - 7 < source_height - 1 and iy4 + 7 > 0.
        // It follows that -6 <= iy4 <= source_height + 5. This inequality is
        // used below.
        int sx4 = (filter_params.x4 & ((1 << kWarpedModelPrecisionBits) - 1)) -
                  beta * 7;
        for (int y = -7; y < 8; ++y) {
          // We assume the source frame has top and bottom borders of at least
          // 13 pixels that extend the frame boundary pixels.
          //
          // Since -7 <= y <= 7, using the inequality on iy4 above, we have
          //   -13 <= iy4 + y <= source_height + 12,
          // or
          //   -13 <= row <= (source_height - 1) + 13.
          // Therefore we may over-read up to 13 pixels above the top source
          // row, or up to 13 pixels below the bottom source row.
          const int row = filter_params.iy4 + y;
          const Pixel* const src_row = src + row * source_stride;
          int sx = sx4 - MultiplyBy4(alpha);
          for (int x = -4; x < 4; ++x) {
            const int offset =
                RightShiftWithRounding(sx, kWarpedDiffPrecisionBits) +
                kWarpedPixelPrecisionShifts;
            // Since alpha and beta have been validated by SetupShear(), one
            // can prove that 0 <= offset <= 3 * 2^6.
            assert(offset >= 0);
            assert(offset < 3 * kWarpedPixelPrecisionShifts + 1);
            // For SIMD optimization:
            // |first_pass_offset| guarantees the sum fits in uint16_t for 8bpp.
            // For 10/12 bit, the range of sum requires 32 bits.
            int sum = first_pass_offset;
            for (int k = 0; k < 8; ++k) {
              // We assume the source frame has left and right borders of at
              // least 13 pixels that extend the frame boundary pixels.
              //
              // Since -4 <= x <= 3 and 0 <= k <= 7, using the inequality on
              // ix4 above, we have
              //   -13 <= ix4 + x + k - 3 <= source_width + 12,
              // or
              //   -13 <= column <= (source_width - 1) + 13.
              // Therefore we may over-read up to 13 pixels before the source
              // row, or up to 13 pixels after the source row.
              const int column = filter_params.ix4 + x + k - 3;
              sum += kWarpedFilters[offset][k] * src_row[column];
            }
            intermediate_result[y + 7][x + 4] =
                RightShiftWithRounding(sum, kRoundBitsHorizontal) -
                offset_removal;
            sx += alpha;
          }
          sx4 += beta;
        }
      }

      // Regions 3 and 4.
      // Vertical filter.
      DestType* dst_row = dst + start_x - block_start_x;
      int sy4 = (filter_params.y4 & ((1 << kWarpedModelPrecisionBits) - 1)) -
                MultiplyBy4(delta);
      // The spec says we should use the following loop condition:
      //   y < std::min(4, block_start_y + block_height - start_y - 4);
      // We can prove that block_start_y + block_height - start_y >= 8, which
      // implies std::min(4, block_start_y + block_height - start_y - 4) = 4.
      // So the loop condition is simply y < 4.
      //
      //   Proof:
      //      start_y < block_start_y + block_height
      //   => block_start_y + block_height - start_y > 0
      //   => block_height - (start_y - block_start_y) > 0
      //
      //   Since block_height >= 8 and is a power of 2, it follows that
      //   block_height is a multiple of 8. start_y - block_start_y is also a
      //   multiple of 8. Therefore their difference is a multiple of 8. Since
      //   their difference is > 0, their difference must be >= 8.
      //
      // We then add an offset of 4 to y so that the loop starts with y = 0
      // and continues if y < 8.
      for (int y = 0; y < 8; ++y) {
        int sy = sy4 - MultiplyBy4(gamma);
        // The spec says we should use the following loop condition:
        //   x < std::min(4, block_start_x + block_width - start_x - 4);
        // Similar to the above, we can prove that the loop condition can be
        // simplified to x < 4.
        //
        // We then add an offset of 4 to x so that the loop starts with x = 0
        // and continues if x < 8.
        for (int x = 0; x < 8; ++x) {
          const int offset =
              RightShiftWithRounding(sy, kWarpedDiffPrecisionBits) +
              kWarpedPixelPrecisionShifts;
          // Since gamma and delta have been validated by SetupShear(), one can
          // prove that 0 <= offset <= 3 * 2^6.
          assert(offset >= 0);
          assert(offset < 3 * kWarpedPixelPrecisionShifts + 1);
          int sum = 0;
          for (int k = 0; k < 8; ++k) {
            sum += kWarpedFilters[offset][k] * intermediate_result[y + k][x];
          }
          sum -= offset_removal;
          sum = RightShiftWithRounding(sum, kRoundBitsVertical);
          if (is_compound) {
            sum += (bitdepth == 8) ? 0 : kCompoundOffset;
            dst_row[x] = static_cast<DestType>(sum);
          } else {
            dst_row[x] = static_cast<DestType>(Clip3(sum, 0, kMaxPixel));
          }
          sy += gamma;
        }
        dst_row += dest_stride;
        sy4 += delta;
      }
    }
    dst += 8 * dest_stride;
  }
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(8);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->warp = Warp_C</*is_compound=*/false, 8, uint8_t>;
  dsp->warp_compound = Warp_C</*is_compound=*/true, 8, uint8_t>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp8bpp_Warp
  dsp->warp = Warp_C</*is_compound=*/false, 8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_WarpCompound
  dsp->warp_compound = Warp_C</*is_compound=*/true, 8, uint8_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}

#if LIBGAV1_MAX_BITDEPTH >= 10
void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(10);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->warp = Warp_C</*is_compound=*/false, 10, uint16_t>;
  dsp->warp_compound = Warp_C</*is_compound=*/true, 10, uint16_t>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp10bpp_Warp
  dsp->warp = Warp_C</*is_compound=*/false, 10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_WarpCompound
  dsp->warp_compound = Warp_C</*is_compound=*/true, 10, uint16_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
void Init12bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(12);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->warp = Warp_C</*is_compound=*/false, 12, uint16_t>;
  dsp->warp_compound = Warp_C</*is_compound=*/true, 12, uint16_t>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp12bpp_Warp
  dsp->warp = Warp_C</*is_compound=*/false, 12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_WarpCompound
  dsp->warp_compound = Warp_C</*is_compound=*/true, 12, uint16_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace

void WarpInit_C() {
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
