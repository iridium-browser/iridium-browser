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

#include "src/dsp/cdef.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"

namespace libgav1 {
namespace dsp {
namespace {

#include "src/dsp/cdef.inc"

// Silence unused function warnings when CdefDirection_C is obviated.
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS ||           \
    !defined(LIBGAV1_Dsp8bpp_CdefDirection) ||    \
    (LIBGAV1_MAX_BITDEPTH >= 10 &&                \
     !defined(LIBGAV1_Dsp10bpp_CdefDirection)) || \
    (LIBGAV1_MAX_BITDEPTH == 12 && !defined(LIBGAV1_Dsp12bpp_CdefDirection))
constexpr int16_t kDivisionTable[] = {840, 420, 280, 210, 168, 140, 120, 105};

int32_t Square(int32_t x) { return x * x; }

template <int bitdepth, typename Pixel>
void CdefDirection_C(const void* LIBGAV1_RESTRICT const source,
                     ptrdiff_t stride,
                     uint8_t* LIBGAV1_RESTRICT const direction,
                     int* LIBGAV1_RESTRICT const variance) {
  assert(direction != nullptr);
  assert(variance != nullptr);
  const auto* src = static_cast<const Pixel*>(source);
  stride /= sizeof(Pixel);
  int32_t cost[8] = {};
  // |partial| does not have to be int32_t for 8bpp. int16_t will suffice. We
  // use int32_t to keep it simple since |cost| will have to be int32_t.
  int32_t partial[8][15] = {};
  for (int i = 0; i < 8; ++i) {
    for (int j = 0; j < 8; ++j) {
      const int x = (src[j] >> (bitdepth - 8)) - 128;
      partial[0][i + j] += x;
      partial[1][i + j / 2] += x;
      partial[2][i] += x;
      partial[3][3 + i - j / 2] += x;
      partial[4][7 + i - j] += x;
      partial[5][3 - i / 2 + j] += x;
      partial[6][j] += x;
      partial[7][i / 2 + j] += x;
    }
    src += stride;
  }
  for (int i = 0; i < 8; ++i) {
    cost[2] += Square(partial[2][i]);
    cost[6] += Square(partial[6][i]);
  }
  cost[2] *= kDivisionTable[7];
  cost[6] *= kDivisionTable[7];
  for (int i = 0; i < 7; ++i) {
    cost[0] += (Square(partial[0][i]) + Square(partial[0][14 - i])) *
               kDivisionTable[i];
    cost[4] += (Square(partial[4][i]) + Square(partial[4][14 - i])) *
               kDivisionTable[i];
  }
  cost[0] += Square(partial[0][7]) * kDivisionTable[7];
  cost[4] += Square(partial[4][7]) * kDivisionTable[7];
  for (int i = 1; i < 8; i += 2) {
    for (int j = 0; j < 5; ++j) {
      cost[i] += Square(partial[i][3 + j]);
    }
    cost[i] *= kDivisionTable[7];
    for (int j = 0; j < 3; ++j) {
      cost[i] += (Square(partial[i][j]) + Square(partial[i][10 - j])) *
                 kDivisionTable[2 * j + 1];
    }
  }
  int32_t best_cost = 0;
  *direction = 0;
  for (int i = 0; i < 8; ++i) {
    if (cost[i] > best_cost) {
      best_cost = cost[i];
      *direction = i;
    }
  }
  *variance = (best_cost - cost[(*direction + 4) & 7]) >> 10;
}
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS ||
        // !defined(LIBGAV1_Dsp8bpp_CdefDirection) ||
        // (LIBGAV1_MAX_BITDEPTH >= 10 &&
        //  !defined(LIBGAV1_Dsp10bpp_CdefDirection))
        // (LIBGAV1_MAX_BITDEPTH == 12 &&
        //  !defined(LIBGAV1_Dsp12bpp_CdefDirection))

// Silence unused function warnings when CdefFilter_C is obviated.
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS ||                                       \
    !defined(LIBGAV1_Dsp8bpp_CdefFilters) ||                                  \
    (LIBGAV1_MAX_BITDEPTH >= 10 && !defined(LIBGAV1_Dsp10bpp_CdefFilters)) || \
    (LIBGAV1_MAX_BITDEPTH == 12 && !defined(LIBGAV1_Dsp12bpp_CdefFilters))

int Constrain(int diff, int threshold, int damping) {
  assert(threshold != 0);
  damping = std::max(0, damping - FloorLog2(threshold));
  const int sign = (diff < 0) ? -1 : 1;
  return sign *
         Clip3(threshold - (std::abs(diff) >> damping), 0, std::abs(diff));
}

// Filters the source block. It doesn't check whether the candidate pixel is
// inside the frame. However it requires the source input to be padded with a
// constant large value (kCdefLargeValue) if at the boundary.
template <int block_width, int bitdepth, typename Pixel,
          bool enable_primary = true, bool enable_secondary = true>
void CdefFilter_C(const uint16_t* LIBGAV1_RESTRICT src,
                  const ptrdiff_t src_stride, const int block_height,
                  const int primary_strength, const int secondary_strength,
                  const int damping, const int direction,
                  void* LIBGAV1_RESTRICT const dest,
                  const ptrdiff_t dest_stride) {
  static_assert(block_width == 4 || block_width == 8, "Invalid CDEF width.");
  static_assert(enable_primary || enable_secondary, "");
  assert(block_height == 4 || block_height == 8);
  assert(direction >= 0 && direction <= 7);
  constexpr int coeff_shift = bitdepth - 8;
  // Section 5.9.19. CDEF params syntax.
  assert(primary_strength >= 0 && primary_strength <= 15 << coeff_shift);
  assert(secondary_strength >= 0 && secondary_strength <= 4 << coeff_shift &&
         secondary_strength != 3 << coeff_shift);
  assert(primary_strength != 0 || secondary_strength != 0);
  // damping is decreased by 1 for chroma.
  assert((damping >= 3 && damping <= 6 + coeff_shift) ||
         (damping >= 2 && damping <= 5 + coeff_shift));
  // When only primary_strength or secondary_strength are non-zero the number
  // of pixels inspected (4 for primary_strength, 8 for secondary_strength) and
  // the taps used don't exceed the amount the sum is
  // descaled by (16) so we can skip tracking and clipping to the minimum and
  // maximum value observed.
  constexpr bool clipping_required = enable_primary && enable_secondary;
  static constexpr int kCdefSecondaryTaps[2] = {kCdefSecondaryTap0,
                                                kCdefSecondaryTap1};
  auto* dst = static_cast<Pixel*>(dest);
  const ptrdiff_t dst_stride = dest_stride / sizeof(Pixel);
  int y = block_height;
  do {
    int x = 0;
    do {
      int16_t sum = 0;
      const uint16_t pixel_value = src[x];
      uint16_t max_value = pixel_value;
      uint16_t min_value = pixel_value;
      for (int k = 0; k < 2; ++k) {
        static constexpr int signs[] = {-1, 1};
        for (const int& sign : signs) {
          if (enable_primary) {
            const int dy = sign * kCdefDirections[direction][k][0];
            const int dx = sign * kCdefDirections[direction][k][1];
            const uint16_t value = src[dy * src_stride + dx + x];
            // Note: the summation can ignore the condition check in SIMD
            // implementation, because Constrain() will return 0 when
            // value == kCdefLargeValue.
            if (value != kCdefLargeValue) {
              sum += Constrain(value - pixel_value, primary_strength, damping) *
                     kCdefPrimaryTaps[(primary_strength >> coeff_shift) & 1][k];
              if (clipping_required) {
                max_value = std::max(value, max_value);
                min_value = std::min(value, min_value);
              }
            }
          }

          if (enable_secondary) {
            static constexpr int offsets[] = {-2, 2};
            for (const int& offset : offsets) {
              const int dy = sign * kCdefDirections[direction + offset][k][0];
              const int dx = sign * kCdefDirections[direction + offset][k][1];
              const uint16_t value = src[dy * src_stride + dx + x];
              // Note: the summation can ignore the condition check in SIMD
              // implementation.
              if (value != kCdefLargeValue) {
                sum += Constrain(value - pixel_value, secondary_strength,
                                 damping) *
                       kCdefSecondaryTaps[k];
                if (clipping_required) {
                  max_value = std::max(value, max_value);
                  min_value = std::min(value, min_value);
                }
              }
            }
          }
        }
      }

      const int offset = (8 + sum - (sum < 0)) >> 4;
      if (clipping_required) {
        dst[x] = static_cast<Pixel>(
            Clip3(pixel_value + offset, min_value, max_value));
      } else {
        dst[x] = static_cast<Pixel>(pixel_value + offset);
      }
    } while (++x < block_width);

    src += src_stride;
    dst += dst_stride;
  } while (--y != 0);
}
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS ||
        // !defined(LIBGAV1_Dsp8bpp_CdefFilters) ||
        // (LIBGAV1_MAX_BITDEPTH >= 10 &&
        //  !defined(LIBGAV1_Dsp10bpp_CdefFilters))
        // (LIBGAV1_MAX_BITDEPTH == 12 &&
        //  !defined(LIBGAV1_Dsp12bpp_CdefFilters))

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(8);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->cdef_direction = CdefDirection_C<8, uint8_t>;
  dsp->cdef_filters[0][0] = CdefFilter_C<4, 8, uint8_t>;
  dsp->cdef_filters[0][1] = CdefFilter_C<4, 8, uint8_t, /*enable_primary=*/true,
                                         /*enable_secondary=*/false>;
  dsp->cdef_filters[0][2] =
      CdefFilter_C<4, 8, uint8_t, /*enable_primary=*/false>;
  dsp->cdef_filters[1][0] = CdefFilter_C<8, 8, uint8_t>;
  dsp->cdef_filters[1][1] = CdefFilter_C<8, 8, uint8_t, /*enable_primary=*/true,
                                         /*enable_secondary=*/false>;
  dsp->cdef_filters[1][2] =
      CdefFilter_C<8, 8, uint8_t, /*enable_primary=*/false>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp8bpp_CdefDirection
  dsp->cdef_direction = CdefDirection_C<8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_CdefFilters
  dsp->cdef_filters[0][0] = CdefFilter_C<4, 8, uint8_t>;
  dsp->cdef_filters[0][1] = CdefFilter_C<4, 8, uint8_t, /*enable_primary=*/true,
                                         /*enable_secondary=*/false>;
  dsp->cdef_filters[0][2] =
      CdefFilter_C<4, 8, uint8_t, /*enable_primary=*/false>;
  dsp->cdef_filters[1][0] = CdefFilter_C<8, 8, uint8_t>;
  dsp->cdef_filters[1][1] = CdefFilter_C<8, 8, uint8_t, /*enable_primary=*/true,
                                         /*enable_secondary=*/false>;
  dsp->cdef_filters[1][2] =
      CdefFilter_C<8, 8, uint8_t, /*enable_primary=*/false>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}

#if LIBGAV1_MAX_BITDEPTH >= 10
void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(10);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->cdef_direction = CdefDirection_C<10, uint16_t>;
  dsp->cdef_filters[0][0] = CdefFilter_C<4, 10, uint16_t>;
  dsp->cdef_filters[0][1] =
      CdefFilter_C<4, 10, uint16_t, /*enable_primary=*/true,
                   /*enable_secondary=*/false>;
  dsp->cdef_filters[0][2] =
      CdefFilter_C<4, 10, uint16_t, /*enable_primary=*/false>;
  dsp->cdef_filters[1][0] = CdefFilter_C<8, 10, uint16_t>;
  dsp->cdef_filters[1][1] =
      CdefFilter_C<8, 10, uint16_t, /*enable_primary=*/true,
                   /*enable_secondary=*/false>;
  dsp->cdef_filters[1][2] =
      CdefFilter_C<8, 10, uint16_t, /*enable_primary=*/false>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp10bpp_CdefDirection
  dsp->cdef_direction = CdefDirection_C<10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_CdefFilters
  dsp->cdef_filters[0][0] = CdefFilter_C<4, 10, uint16_t>;
  dsp->cdef_filters[0][1] =
      CdefFilter_C<4, 10, uint16_t, /*enable_primary=*/true,
                   /*enable_secondary=*/false>;
  dsp->cdef_filters[0][2] =
      CdefFilter_C<4, 10, uint16_t, /*enable_primary=*/false>;
  dsp->cdef_filters[1][0] = CdefFilter_C<8, 10, uint16_t>;
  dsp->cdef_filters[1][1] =
      CdefFilter_C<8, 10, uint16_t, /*enable_primary=*/true,
                   /*enable_secondary=*/false>;
  dsp->cdef_filters[1][2] =
      CdefFilter_C<8, 10, uint16_t, /*enable_primary=*/false>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
void Init12bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(12);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->cdef_direction = CdefDirection_C<12, uint16_t>;
  dsp->cdef_filters[0][0] = CdefFilter_C<4, 12, uint16_t>;
  dsp->cdef_filters[0][1] =
      CdefFilter_C<4, 12, uint16_t, /*enable_primary=*/true,
                   /*enable_secondary=*/false>;
  dsp->cdef_filters[0][2] =
      CdefFilter_C<4, 12, uint16_t, /*enable_primary=*/false>;
  dsp->cdef_filters[1][0] = CdefFilter_C<8, 12, uint16_t>;
  dsp->cdef_filters[1][1] =
      CdefFilter_C<8, 12, uint16_t, /*enable_primary=*/true,
                   /*enable_secondary=*/false>;
  dsp->cdef_filters[1][2] =
      CdefFilter_C<8, 12, uint16_t, /*enable_primary=*/false>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp12bpp_CdefDirection
  dsp->cdef_direction = CdefDirection_C<12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_CdefFilters
  dsp->cdef_filters[0][0] = CdefFilter_C<4, 12, uint16_t>;
  dsp->cdef_filters[0][1] =
      CdefFilter_C<4, 12, uint16_t, /*enable_primary=*/true,
                   /*enable_secondary=*/false>;
  dsp->cdef_filters[0][2] =
      CdefFilter_C<4, 12, uint16_t, /*enable_primary=*/false>;
  dsp->cdef_filters[1][0] = CdefFilter_C<8, 12, uint16_t>;
  dsp->cdef_filters[1][1] =
      CdefFilter_C<8, 12, uint16_t, /*enable_primary=*/true,
                   /*enable_secondary=*/false>;
  dsp->cdef_filters[1][2] =
      CdefFilter_C<8, 12, uint16_t, /*enable_primary=*/false>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace

void CdefInit_C() {
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
