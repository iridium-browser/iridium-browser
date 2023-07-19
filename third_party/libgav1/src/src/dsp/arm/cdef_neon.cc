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
#include "src/utils/cpu.h"

#if LIBGAV1_ENABLE_NEON

#include <arm_neon.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "src/dsp/arm/common_neon.h"
#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"

namespace libgav1 {
namespace dsp {
namespace {

#include "src/dsp/cdef.inc"

// ----------------------------------------------------------------------------
// Refer to CdefDirection_C().
//
// int32_t partial[8][15] = {};
// for (int i = 0; i < 8; ++i) {
//   for (int j = 0; j < 8; ++j) {
//     const int x = 1;
//     partial[0][i + j] += x;
//     partial[1][i + j / 2] += x;
//     partial[2][i] += x;
//     partial[3][3 + i - j / 2] += x;
//     partial[4][7 + i - j] += x;
//     partial[5][3 - i / 2 + j] += x;
//     partial[6][j] += x;
//     partial[7][i / 2 + j] += x;
//   }
// }
//
// Using the code above, generate the position count for partial[8][15].
//
// partial[0]: 1 2 3 4 5 6 7 8 7 6 5 4 3 2 1
// partial[1]: 2 4 6 8 8 8 8 8 6 4 2 0 0 0 0
// partial[2]: 8 8 8 8 8 8 8 8 0 0 0 0 0 0 0
// partial[3]: 2 4 6 8 8 8 8 8 6 4 2 0 0 0 0
// partial[4]: 1 2 3 4 5 6 7 8 7 6 5 4 3 2 1
// partial[5]: 2 4 6 8 8 8 8 8 6 4 2 0 0 0 0
// partial[6]: 8 8 8 8 8 8 8 8 0 0 0 0 0 0 0
// partial[7]: 2 4 6 8 8 8 8 8 6 4 2 0 0 0 0
//
// The SIMD code shifts the input horizontally, then adds vertically to get the
// correct partial value for the given position.
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// partial[0][i + j] += x;
//
// 00 01 02 03 04 05 06 07  00 00 00 00 00 00 00
// 00 10 11 12 13 14 15 16  17 00 00 00 00 00 00
// 00 00 20 21 22 23 24 25  26 27 00 00 00 00 00
// 00 00 00 30 31 32 33 34  35 36 37 00 00 00 00
// 00 00 00 00 40 41 42 43  44 45 46 47 00 00 00
// 00 00 00 00 00 50 51 52  53 54 55 56 57 00 00
// 00 00 00 00 00 00 60 61  62 63 64 65 66 67 00
// 00 00 00 00 00 00 00 70  71 72 73 74 75 76 77
//
// partial[4] is the same except the source is reversed.
LIBGAV1_ALWAYS_INLINE void AddPartial_D0_D4(uint8x8_t* v_src,
                                            uint16x8_t* partial_lo,
                                            uint16x8_t* partial_hi) {
  const uint8x8_t v_zero = vdup_n_u8(0);
  // 00 01 02 03 04 05 06 07
  // 00 10 11 12 13 14 15 16
  *partial_lo = vaddl_u8(v_src[0], vext_u8(v_zero, v_src[1], 7));

  // 00 00 20 21 22 23 24 25
  *partial_lo = vaddw_u8(*partial_lo, vext_u8(v_zero, v_src[2], 6));
  // 17 00 00 00 00 00 00 00
  // 26 27 00 00 00 00 00 00
  *partial_hi =
      vaddl_u8(vext_u8(v_src[1], v_zero, 7), vext_u8(v_src[2], v_zero, 6));

  // 00 00 00 30 31 32 33 34
  *partial_lo = vaddw_u8(*partial_lo, vext_u8(v_zero, v_src[3], 5));
  // 35 36 37 00 00 00 00 00
  *partial_hi = vaddw_u8(*partial_hi, vext_u8(v_src[3], v_zero, 5));

  // 00 00 00 00 40 41 42 43
  *partial_lo = vaddw_u8(*partial_lo, vext_u8(v_zero, v_src[4], 4));
  // 44 45 46 47 00 00 00 00
  *partial_hi = vaddw_u8(*partial_hi, vext_u8(v_src[4], v_zero, 4));

  // 00 00 00 00 00 50 51 52
  *partial_lo = vaddw_u8(*partial_lo, vext_u8(v_zero, v_src[5], 3));
  // 53 54 55 56 57 00 00 00
  *partial_hi = vaddw_u8(*partial_hi, vext_u8(v_src[5], v_zero, 3));

  // 00 00 00 00 00 00 60 61
  *partial_lo = vaddw_u8(*partial_lo, vext_u8(v_zero, v_src[6], 2));
  // 62 63 64 65 66 67 00 00
  *partial_hi = vaddw_u8(*partial_hi, vext_u8(v_src[6], v_zero, 2));

  // 00 00 00 00 00 00 00 70
  *partial_lo = vaddw_u8(*partial_lo, vext_u8(v_zero, v_src[7], 1));
  // 71 72 73 74 75 76 77 00
  *partial_hi = vaddw_u8(*partial_hi, vext_u8(v_src[7], v_zero, 1));
}

// ----------------------------------------------------------------------------
// partial[1][i + j / 2] += x;
//
// A0 = src[0] + src[1], A1 = src[2] + src[3], ...
//
// A0 A1 A2 A3 00 00 00 00  00 00 00 00 00 00 00
// 00 B0 B1 B2 B3 00 00 00  00 00 00 00 00 00 00
// 00 00 C0 C1 C2 C3 00 00  00 00 00 00 00 00 00
// 00 00 00 D0 D1 D2 D3 00  00 00 00 00 00 00 00
// 00 00 00 00 E0 E1 E2 E3  00 00 00 00 00 00 00
// 00 00 00 00 00 F0 F1 F2  F3 00 00 00 00 00 00
// 00 00 00 00 00 00 G0 G1  G2 G3 00 00 00 00 00
// 00 00 00 00 00 00 00 H0  H1 H2 H3 00 00 00 00
//
// partial[3] is the same except the source is reversed.
LIBGAV1_ALWAYS_INLINE void AddPartial_D1_D3(uint8x8_t* v_src,
                                            uint16x8_t* partial_lo,
                                            uint16x8_t* partial_hi) {
  uint8x16_t v_d1_temp[8];
  const uint8x8_t v_zero = vdup_n_u8(0);
  const uint8x16_t v_zero_16 = vdupq_n_u8(0);

  for (int i = 0; i < 8; ++i) {
    v_d1_temp[i] = vcombine_u8(v_src[i], v_zero);
  }

  *partial_lo = *partial_hi = vdupq_n_u16(0);
  // A0 A1 A2 A3 00 00 00 00
  *partial_lo = vpadalq_u8(*partial_lo, v_d1_temp[0]);

  // 00 B0 B1 B2 B3 00 00 00
  *partial_lo = vpadalq_u8(*partial_lo, vextq_u8(v_zero_16, v_d1_temp[1], 14));

  // 00 00 C0 C1 C2 C3 00 00
  *partial_lo = vpadalq_u8(*partial_lo, vextq_u8(v_zero_16, v_d1_temp[2], 12));
  // 00 00 00 D0 D1 D2 D3 00
  *partial_lo = vpadalq_u8(*partial_lo, vextq_u8(v_zero_16, v_d1_temp[3], 10));
  // 00 00 00 00 E0 E1 E2 E3
  *partial_lo = vpadalq_u8(*partial_lo, vextq_u8(v_zero_16, v_d1_temp[4], 8));

  // 00 00 00 00 00 F0 F1 F2
  *partial_lo = vpadalq_u8(*partial_lo, vextq_u8(v_zero_16, v_d1_temp[5], 6));
  // F3 00 00 00 00 00 00 00
  *partial_hi = vpadalq_u8(*partial_hi, vextq_u8(v_d1_temp[5], v_zero_16, 6));

  // 00 00 00 00 00 00 G0 G1
  *partial_lo = vpadalq_u8(*partial_lo, vextq_u8(v_zero_16, v_d1_temp[6], 4));
  // G2 G3 00 00 00 00 00 00
  *partial_hi = vpadalq_u8(*partial_hi, vextq_u8(v_d1_temp[6], v_zero_16, 4));

  // 00 00 00 00 00 00 00 H0
  *partial_lo = vpadalq_u8(*partial_lo, vextq_u8(v_zero_16, v_d1_temp[7], 2));
  // H1 H2 H3 00 00 00 00 00
  *partial_hi = vpadalq_u8(*partial_hi, vextq_u8(v_d1_temp[7], v_zero_16, 2));
}

// ----------------------------------------------------------------------------
// partial[7][i / 2 + j] += x;
//
// 00 01 02 03 04 05 06 07  00 00 00 00 00 00 00
// 10 11 12 13 14 15 16 17  00 00 00 00 00 00 00
// 00 20 21 22 23 24 25 26  27 00 00 00 00 00 00
// 00 30 31 32 33 34 35 36  37 00 00 00 00 00 00
// 00 00 40 41 42 43 44 45  46 47 00 00 00 00 00
// 00 00 50 51 52 53 54 55  56 57 00 00 00 00 00
// 00 00 00 60 61 62 63 64  65 66 67 00 00 00 00
// 00 00 00 70 71 72 73 74  75 76 77 00 00 00 00
//
// partial[5] is the same except the source is reversed.
LIBGAV1_ALWAYS_INLINE void AddPartial_D5_D7(uint8x8_t* v_src,
                                            uint16x8_t* partial_lo,
                                            uint16x8_t* partial_hi) {
  const uint16x8_t v_zero = vdupq_n_u16(0);
  uint16x8_t v_pair_add[4];
  // Add vertical source pairs.
  v_pair_add[0] = vaddl_u8(v_src[0], v_src[1]);
  v_pair_add[1] = vaddl_u8(v_src[2], v_src[3]);
  v_pair_add[2] = vaddl_u8(v_src[4], v_src[5]);
  v_pair_add[3] = vaddl_u8(v_src[6], v_src[7]);

  // 00 01 02 03 04 05 06 07
  // 10 11 12 13 14 15 16 17
  *partial_lo = v_pair_add[0];
  // 00 00 00 00 00 00 00 00
  // 00 00 00 00 00 00 00 00
  *partial_hi = vdupq_n_u16(0);

  // 00 20 21 22 23 24 25 26
  // 00 30 31 32 33 34 35 36
  *partial_lo = vaddq_u16(*partial_lo, vextq_u16(v_zero, v_pair_add[1], 7));
  // 27 00 00 00 00 00 00 00
  // 37 00 00 00 00 00 00 00
  *partial_hi = vaddq_u16(*partial_hi, vextq_u16(v_pair_add[1], v_zero, 7));

  // 00 00 40 41 42 43 44 45
  // 00 00 50 51 52 53 54 55
  *partial_lo = vaddq_u16(*partial_lo, vextq_u16(v_zero, v_pair_add[2], 6));
  // 46 47 00 00 00 00 00 00
  // 56 57 00 00 00 00 00 00
  *partial_hi = vaddq_u16(*partial_hi, vextq_u16(v_pair_add[2], v_zero, 6));

  // 00 00 00 60 61 62 63 64
  // 00 00 00 70 71 72 73 74
  *partial_lo = vaddq_u16(*partial_lo, vextq_u16(v_zero, v_pair_add[3], 5));
  // 65 66 67 00 00 00 00 00
  // 75 76 77 00 00 00 00 00
  *partial_hi = vaddq_u16(*partial_hi, vextq_u16(v_pair_add[3], v_zero, 5));
}

template <int bitdepth>
LIBGAV1_ALWAYS_INLINE void AddPartial(const void* LIBGAV1_RESTRICT const source,
                                      ptrdiff_t stride, uint16x8_t* partial_lo,
                                      uint16x8_t* partial_hi) {
  const auto* src = static_cast<const uint8_t*>(source);

  // 8x8 input
  // 00 01 02 03 04 05 06 07
  // 10 11 12 13 14 15 16 17
  // 20 21 22 23 24 25 26 27
  // 30 31 32 33 34 35 36 37
  // 40 41 42 43 44 45 46 47
  // 50 51 52 53 54 55 56 57
  // 60 61 62 63 64 65 66 67
  // 70 71 72 73 74 75 76 77
  uint8x8_t v_src[8];
  if (bitdepth == kBitdepth8) {
    for (auto& v : v_src) {
      v = vld1_u8(src);
      src += stride;
    }
  } else {
    // bitdepth - 8
    constexpr int src_shift = (bitdepth == kBitdepth10) ? 2 : 4;
    for (auto& v : v_src) {
      v = vshrn_n_u16(vld1q_u16(reinterpret_cast<const uint16_t*>(src)),
                      src_shift);
      src += stride;
    }
  }
  // partial for direction 2
  // --------------------------------------------------------------------------
  // partial[2][i] += x;
  // 00 10 20 30 40 50 60 70  00 00 00 00 00 00 00 00
  // 01 11 21 33 41 51 61 71  00 00 00 00 00 00 00 00
  // 02 12 22 33 42 52 62 72  00 00 00 00 00 00 00 00
  // 03 13 23 33 43 53 63 73  00 00 00 00 00 00 00 00
  // 04 14 24 34 44 54 64 74  00 00 00 00 00 00 00 00
  // 05 15 25 35 45 55 65 75  00 00 00 00 00 00 00 00
  // 06 16 26 36 46 56 66 76  00 00 00 00 00 00 00 00
  // 07 17 27 37 47 57 67 77  00 00 00 00 00 00 00 00
  partial_lo[2] = vsetq_lane_u16(SumVector(v_src[0]), vdupq_n_u16(0), 0);
  partial_lo[2] = vsetq_lane_u16(SumVector(v_src[1]), partial_lo[2], 1);
  partial_lo[2] = vsetq_lane_u16(SumVector(v_src[2]), partial_lo[2], 2);
  partial_lo[2] = vsetq_lane_u16(SumVector(v_src[3]), partial_lo[2], 3);
  partial_lo[2] = vsetq_lane_u16(SumVector(v_src[4]), partial_lo[2], 4);
  partial_lo[2] = vsetq_lane_u16(SumVector(v_src[5]), partial_lo[2], 5);
  partial_lo[2] = vsetq_lane_u16(SumVector(v_src[6]), partial_lo[2], 6);
  partial_lo[2] = vsetq_lane_u16(SumVector(v_src[7]), partial_lo[2], 7);

  // partial for direction 6
  // --------------------------------------------------------------------------
  // partial[6][j] += x;
  // 00 01 02 03 04 05 06 07  00 00 00 00 00 00 00 00
  // 10 11 12 13 14 15 16 17  00 00 00 00 00 00 00 00
  // 20 21 22 23 24 25 26 27  00 00 00 00 00 00 00 00
  // 30 31 32 33 34 35 36 37  00 00 00 00 00 00 00 00
  // 40 41 42 43 44 45 46 47  00 00 00 00 00 00 00 00
  // 50 51 52 53 54 55 56 57  00 00 00 00 00 00 00 00
  // 60 61 62 63 64 65 66 67  00 00 00 00 00 00 00 00
  // 70 71 72 73 74 75 76 77  00 00 00 00 00 00 00 00
  partial_lo[6] = vaddl_u8(v_src[0], v_src[1]);
  for (int i = 2; i < 8; ++i) {
    partial_lo[6] = vaddw_u8(partial_lo[6], v_src[i]);
  }

  // partial for direction 0
  AddPartial_D0_D4(v_src, &partial_lo[0], &partial_hi[0]);

  // partial for direction 1
  AddPartial_D1_D3(v_src, &partial_lo[1], &partial_hi[1]);

  // partial for direction 7
  AddPartial_D5_D7(v_src, &partial_lo[7], &partial_hi[7]);

  uint8x8_t v_src_reverse[8];
  for (int i = 0; i < 8; ++i) {
    v_src_reverse[i] = vrev64_u8(v_src[i]);
  }

  // partial for direction 4
  AddPartial_D0_D4(v_src_reverse, &partial_lo[4], &partial_hi[4]);

  // partial for direction 3
  AddPartial_D1_D3(v_src_reverse, &partial_lo[3], &partial_hi[3]);

  // partial for direction 5
  AddPartial_D5_D7(v_src_reverse, &partial_lo[5], &partial_hi[5]);
}

uint32x4_t Square(uint16x4_t a) { return vmull_u16(a, a); }

uint32x4_t SquareAccumulate(uint32x4_t a, uint16x4_t b) {
  return vmlal_u16(a, b, b);
}

// |cost[0]| and |cost[4]| square the input and sum with the corresponding
// element from the other end of the vector:
// |kCdefDivisionTable[]| element:
// cost[0] += (Square(partial[0][i]) + Square(partial[0][14 - i])) *
//             kCdefDivisionTable[i + 1];
// cost[0] += Square(partial[0][7]) * kCdefDivisionTable[8];
// Because everything is being summed into a single value the distributive
// property allows us to mirror the division table and accumulate once.
uint32_t Cost0Or4(const uint16x8_t a, const uint16x8_t b,
                  const uint32x4_t division_table[4]) {
  uint32x4_t c = vmulq_u32(Square(vget_low_u16(a)), division_table[0]);
  c = vmlaq_u32(c, Square(vget_high_u16(a)), division_table[1]);
  c = vmlaq_u32(c, Square(vget_low_u16(b)), division_table[2]);
  c = vmlaq_u32(c, Square(vget_high_u16(b)), division_table[3]);
  return SumVector(c);
}

// |cost[2]| and |cost[6]| square the input and accumulate:
// cost[2] += Square(partial[2][i])
uint32_t SquareAccumulate(const uint16x8_t a) {
  uint32x4_t c = Square(vget_low_u16(a));
  c = SquareAccumulate(c, vget_high_u16(a));
  c = vmulq_n_u32(c, kCdefDivisionTable[7]);
  return SumVector(c);
}

uint32_t CostOdd(const uint16x8_t a, const uint16x8_t b, const uint32x4_t mask,
                 const uint32x4_t division_table[2]) {
  // Remove elements 0-2.
  uint32x4_t c = vandq_u32(mask, Square(vget_low_u16(a)));
  c = vaddq_u32(c, Square(vget_high_u16(a)));
  c = vmulq_n_u32(c, kCdefDivisionTable[7]);

  c = vmlaq_u32(c, Square(vget_low_u16(a)), division_table[0]);
  c = vmlaq_u32(c, Square(vget_low_u16(b)), division_table[1]);
  return SumVector(c);
}

template <int bitdepth>
void CdefDirection_NEON(const void* LIBGAV1_RESTRICT const source,
                        ptrdiff_t stride,
                        uint8_t* LIBGAV1_RESTRICT const direction,
                        int* LIBGAV1_RESTRICT const variance) {
  assert(direction != nullptr);
  assert(variance != nullptr);
  const auto* src = static_cast<const uint8_t*>(source);

  uint32_t cost[8];
  uint16x8_t partial_lo[8], partial_hi[8];

  AddPartial<bitdepth>(src, stride, partial_lo, partial_hi);

  cost[2] = SquareAccumulate(partial_lo[2]);
  cost[6] = SquareAccumulate(partial_lo[6]);

  const uint32x4_t division_table[4] = {
      vld1q_u32(kCdefDivisionTable), vld1q_u32(kCdefDivisionTable + 4),
      vld1q_u32(kCdefDivisionTable + 8), vld1q_u32(kCdefDivisionTable + 12)};

  cost[0] = Cost0Or4(partial_lo[0], partial_hi[0], division_table);
  cost[4] = Cost0Or4(partial_lo[4], partial_hi[4], division_table);

  const uint32x4_t division_table_odd[2] = {
      vld1q_u32(kCdefDivisionTableOdd), vld1q_u32(kCdefDivisionTableOdd + 4)};

  const uint32x4_t element_3_mask = {0, 0, 0, static_cast<uint32_t>(-1)};

  cost[1] =
      CostOdd(partial_lo[1], partial_hi[1], element_3_mask, division_table_odd);
  cost[3] =
      CostOdd(partial_lo[3], partial_hi[3], element_3_mask, division_table_odd);
  cost[5] =
      CostOdd(partial_lo[5], partial_hi[5], element_3_mask, division_table_odd);
  cost[7] =
      CostOdd(partial_lo[7], partial_hi[7], element_3_mask, division_table_odd);

  uint32_t best_cost = 0;
  *direction = 0;
  for (int i = 0; i < 8; ++i) {
    if (cost[i] > best_cost) {
      best_cost = cost[i];
      *direction = i;
    }
  }
  *variance = (best_cost - cost[(*direction + 4) & 7]) >> 10;
}

// -------------------------------------------------------------------------
// CdefFilter

// Load 4 vectors based on the given |direction|.
void LoadDirection(const uint16_t* LIBGAV1_RESTRICT const src,
                   const ptrdiff_t stride, uint16x8_t* output,
                   const int direction) {
  // Each |direction| describes a different set of source values. Expand this
  // set by negating each set. For |direction| == 0 this gives a diagonal line
  // from top right to bottom left. The first value is y, the second x. Negative
  // y values move up.
  //    a       b         c       d
  // {-1, 1}, {1, -1}, {-2, 2}, {2, -2}
  //         c
  //       a
  //     0
  //   b
  // d
  const int y_0 = kCdefDirections[direction][0][0];
  const int x_0 = kCdefDirections[direction][0][1];
  const int y_1 = kCdefDirections[direction][1][0];
  const int x_1 = kCdefDirections[direction][1][1];
  output[0] = vld1q_u16(src + y_0 * stride + x_0);
  output[1] = vld1q_u16(src - y_0 * stride - x_0);
  output[2] = vld1q_u16(src + y_1 * stride + x_1);
  output[3] = vld1q_u16(src - y_1 * stride - x_1);
}

// Load 4 vectors based on the given |direction|. Use when |block_width| == 4 to
// do 2 rows at a time.
void LoadDirection4(const uint16_t* LIBGAV1_RESTRICT const src,
                    const ptrdiff_t stride, uint16x8_t* output,
                    const int direction) {
  const int y_0 = kCdefDirections[direction][0][0];
  const int x_0 = kCdefDirections[direction][0][1];
  const int y_1 = kCdefDirections[direction][1][0];
  const int x_1 = kCdefDirections[direction][1][1];
  output[0] = vcombine_u16(vld1_u16(src + y_0 * stride + x_0),
                           vld1_u16(src + y_0 * stride + stride + x_0));
  output[1] = vcombine_u16(vld1_u16(src - y_0 * stride - x_0),
                           vld1_u16(src - y_0 * stride + stride - x_0));
  output[2] = vcombine_u16(vld1_u16(src + y_1 * stride + x_1),
                           vld1_u16(src + y_1 * stride + stride + x_1));
  output[3] = vcombine_u16(vld1_u16(src - y_1 * stride - x_1),
                           vld1_u16(src - y_1 * stride + stride - x_1));
}

int16x8_t Constrain(const uint16x8_t pixel, const uint16x8_t reference,
                    const uint16x8_t threshold, const int16x8_t damping) {
  // If reference > pixel, the difference will be negative, so convert to 0 or
  // -1.
  const uint16x8_t sign = vcgtq_u16(reference, pixel);
  const uint16x8_t abs_diff = vabdq_u16(pixel, reference);
  const uint16x8_t shifted_diff = vshlq_u16(abs_diff, damping);
  // For bitdepth == 8, the threshold range is [0, 15] and the damping range is
  // [3, 6]. If pixel == kCdefLargeValue(0x4000), shifted_diff will always be
  // larger than threshold. Subtract using saturation will return 0 when pixel
  // == kCdefLargeValue.
  static_assert(kCdefLargeValue == 0x4000, "Invalid kCdefLargeValue");
  const uint16x8_t thresh_minus_shifted_diff =
      vqsubq_u16(threshold, shifted_diff);
  const uint16x8_t clamp_abs_diff =
      vminq_u16(thresh_minus_shifted_diff, abs_diff);
  // Restore the sign.
  return vreinterpretq_s16_u16(
      vsubq_u16(veorq_u16(clamp_abs_diff, sign), sign));
}

template <typename Pixel>
uint16x8_t GetMaxPrimary(uint16x8_t* primary_val, uint16x8_t max,
                         uint16x8_t cdef_large_value_mask) {
  if (sizeof(Pixel) == 1) {
    // The source is 16 bits, however, we only really care about the lower
    // 8 bits.  The upper 8 bits contain the "large" flag.  After the final
    // primary max has been calculated, zero out the upper 8 bits.  Use this
    // to find the "16 bit" max.
    const uint8x16_t max_p01 = vmaxq_u8(vreinterpretq_u8_u16(primary_val[0]),
                                        vreinterpretq_u8_u16(primary_val[1]));
    const uint8x16_t max_p23 = vmaxq_u8(vreinterpretq_u8_u16(primary_val[2]),
                                        vreinterpretq_u8_u16(primary_val[3]));
    const uint16x8_t max_p = vreinterpretq_u16_u8(vmaxq_u8(max_p01, max_p23));
    max = vmaxq_u16(max, vandq_u16(max_p, cdef_large_value_mask));
  } else {
    // Convert kCdefLargeValue to 0 before calculating max.
    max = vmaxq_u16(max, vandq_u16(primary_val[0], cdef_large_value_mask));
    max = vmaxq_u16(max, vandq_u16(primary_val[1], cdef_large_value_mask));
    max = vmaxq_u16(max, vandq_u16(primary_val[2], cdef_large_value_mask));
    max = vmaxq_u16(max, vandq_u16(primary_val[3], cdef_large_value_mask));
  }
  return max;
}

template <typename Pixel>
uint16x8_t GetMaxSecondary(uint16x8_t* secondary_val, uint16x8_t max,
                           uint16x8_t cdef_large_value_mask) {
  if (sizeof(Pixel) == 1) {
    const uint8x16_t max_s01 = vmaxq_u8(vreinterpretq_u8_u16(secondary_val[0]),
                                        vreinterpretq_u8_u16(secondary_val[1]));
    const uint8x16_t max_s23 = vmaxq_u8(vreinterpretq_u8_u16(secondary_val[2]),
                                        vreinterpretq_u8_u16(secondary_val[3]));
    const uint8x16_t max_s45 = vmaxq_u8(vreinterpretq_u8_u16(secondary_val[4]),
                                        vreinterpretq_u8_u16(secondary_val[5]));
    const uint8x16_t max_s67 = vmaxq_u8(vreinterpretq_u8_u16(secondary_val[6]),
                                        vreinterpretq_u8_u16(secondary_val[7]));
    const uint16x8_t max_s = vreinterpretq_u16_u8(
        vmaxq_u8(vmaxq_u8(max_s01, max_s23), vmaxq_u8(max_s45, max_s67)));
    max = vmaxq_u16(max, vandq_u16(max_s, cdef_large_value_mask));
  } else {
    max = vmaxq_u16(max, vandq_u16(secondary_val[0], cdef_large_value_mask));
    max = vmaxq_u16(max, vandq_u16(secondary_val[1], cdef_large_value_mask));
    max = vmaxq_u16(max, vandq_u16(secondary_val[2], cdef_large_value_mask));
    max = vmaxq_u16(max, vandq_u16(secondary_val[3], cdef_large_value_mask));
    max = vmaxq_u16(max, vandq_u16(secondary_val[4], cdef_large_value_mask));
    max = vmaxq_u16(max, vandq_u16(secondary_val[5], cdef_large_value_mask));
    max = vmaxq_u16(max, vandq_u16(secondary_val[6], cdef_large_value_mask));
    max = vmaxq_u16(max, vandq_u16(secondary_val[7], cdef_large_value_mask));
  }
  return max;
}

template <typename Pixel, int width>
void StorePixels(void* dest, ptrdiff_t dst_stride, int16x8_t result) {
  auto* const dst8 = static_cast<uint8_t*>(dest);
  if (sizeof(Pixel) == 1) {
    const uint8x8_t dst_pixel = vqmovun_s16(result);
    if (width == 8) {
      vst1_u8(dst8, dst_pixel);
    } else {
      StoreLo4(dst8, dst_pixel);
      StoreHi4(dst8 + dst_stride, dst_pixel);
    }
  } else {
    const uint16x8_t dst_pixel = vreinterpretq_u16_s16(result);
    auto* const dst16 = reinterpret_cast<uint16_t*>(dst8);
    if (width == 8) {
      vst1q_u16(dst16, dst_pixel);
    } else {
      auto* const dst16_next_row =
          reinterpret_cast<uint16_t*>(dst8 + dst_stride);
      vst1_u16(dst16, vget_low_u16(dst_pixel));
      vst1_u16(dst16_next_row, vget_high_u16(dst_pixel));
    }
  }
}

template <int width, typename Pixel, bool enable_primary = true,
          bool enable_secondary = true>
void CdefFilter_NEON(const uint16_t* LIBGAV1_RESTRICT src,
                     const ptrdiff_t src_stride, const int height,
                     const int primary_strength, const int secondary_strength,
                     const int damping, const int direction,
                     void* LIBGAV1_RESTRICT dest, const ptrdiff_t dst_stride) {
  static_assert(width == 8 || width == 4, "");
  static_assert(enable_primary || enable_secondary, "");
  constexpr bool clipping_required = enable_primary && enable_secondary;
  auto* dst = static_cast<uint8_t*>(dest);
  const uint16x8_t cdef_large_value_mask =
      vdupq_n_u16(static_cast<uint16_t>(~kCdefLargeValue));
  const uint16x8_t primary_threshold = vdupq_n_u16(primary_strength);
  const uint16x8_t secondary_threshold = vdupq_n_u16(secondary_strength);

  int16x8_t primary_damping_shift, secondary_damping_shift;

  // FloorLog2() requires input to be > 0.
  // 8-bit damping range: Y: [3, 6], UV: [2, 5].
  // 10-bit damping range: Y: [3, 6 + 2], UV: [2, 5 + 2].
  if (enable_primary) {
    // 8-bit primary_strength: [0, 15] -> FloorLog2: [0, 3] so a clamp is
    // necessary for UV filtering.
    // 10-bit primary_strength: [0, 15 << 2].
    primary_damping_shift =
        vdupq_n_s16(-std::max(0, damping - FloorLog2(primary_strength)));
  }

  if (enable_secondary) {
    if (sizeof(Pixel) == 1) {
      // secondary_strength: [0, 4] -> FloorLog2: [0, 2] so no clamp to 0 is
      // necessary.
      assert(damping - FloorLog2(secondary_strength) >= 0);
      secondary_damping_shift =
          vdupq_n_s16(-(damping - FloorLog2(secondary_strength)));
    } else {
      // secondary_strength: [0, 4 << 2]
      secondary_damping_shift =
          vdupq_n_s16(-std::max(0, damping - FloorLog2(secondary_strength)));
    }
  }

  constexpr int coeff_shift = (sizeof(Pixel) == 1) ? 0 : kBitdepth10 - 8;
  const int primary_tap_0 =
      kCdefPrimaryTaps[(primary_strength >> coeff_shift) & 1][0];
  const int primary_tap_1 =
      kCdefPrimaryTaps[(primary_strength >> coeff_shift) & 1][1];

  int y = height;
  do {
    uint16x8_t pixel;
    if (width == 8) {
      pixel = vld1q_u16(src);
    } else {
      pixel = vcombine_u16(vld1_u16(src), vld1_u16(src + src_stride));
    }

    uint16x8_t min = pixel;
    uint16x8_t max = pixel;
    int16x8_t sum;

    if (enable_primary) {
      // Primary |direction|.
      uint16x8_t primary_val[4];
      if (width == 8) {
        LoadDirection(src, src_stride, primary_val, direction);
      } else {
        LoadDirection4(src, src_stride, primary_val, direction);
      }

      if (clipping_required) {
        min = vminq_u16(min, primary_val[0]);
        min = vminq_u16(min, primary_val[1]);
        min = vminq_u16(min, primary_val[2]);
        min = vminq_u16(min, primary_val[3]);

        max = GetMaxPrimary<Pixel>(primary_val, max, cdef_large_value_mask);
      }

      sum = Constrain(primary_val[0], pixel, primary_threshold,
                      primary_damping_shift);
      sum = vmulq_n_s16(sum, primary_tap_0);
      sum = vmlaq_n_s16(sum,
                        Constrain(primary_val[1], pixel, primary_threshold,
                                  primary_damping_shift),
                        primary_tap_0);
      sum = vmlaq_n_s16(sum,
                        Constrain(primary_val[2], pixel, primary_threshold,
                                  primary_damping_shift),
                        primary_tap_1);
      sum = vmlaq_n_s16(sum,
                        Constrain(primary_val[3], pixel, primary_threshold,
                                  primary_damping_shift),
                        primary_tap_1);
    } else {
      sum = vdupq_n_s16(0);
    }

    if (enable_secondary) {
      // Secondary |direction| values (+/- 2). Clamp |direction|.
      uint16x8_t secondary_val[8];
      if (width == 8) {
        LoadDirection(src, src_stride, secondary_val, direction + 2);
        LoadDirection(src, src_stride, secondary_val + 4, direction - 2);
      } else {
        LoadDirection4(src, src_stride, secondary_val, direction + 2);
        LoadDirection4(src, src_stride, secondary_val + 4, direction - 2);
      }

      if (clipping_required) {
        min = vminq_u16(min, secondary_val[0]);
        min = vminq_u16(min, secondary_val[1]);
        min = vminq_u16(min, secondary_val[2]);
        min = vminq_u16(min, secondary_val[3]);
        min = vminq_u16(min, secondary_val[4]);
        min = vminq_u16(min, secondary_val[5]);
        min = vminq_u16(min, secondary_val[6]);
        min = vminq_u16(min, secondary_val[7]);

        max = GetMaxSecondary<Pixel>(secondary_val, max, cdef_large_value_mask);
      }

      sum = vmlaq_n_s16(sum,
                        Constrain(secondary_val[0], pixel, secondary_threshold,
                                  secondary_damping_shift),
                        kCdefSecondaryTap0);
      sum = vmlaq_n_s16(sum,
                        Constrain(secondary_val[1], pixel, secondary_threshold,
                                  secondary_damping_shift),
                        kCdefSecondaryTap0);
      sum = vmlaq_n_s16(sum,
                        Constrain(secondary_val[2], pixel, secondary_threshold,
                                  secondary_damping_shift),
                        kCdefSecondaryTap1);
      sum = vmlaq_n_s16(sum,
                        Constrain(secondary_val[3], pixel, secondary_threshold,
                                  secondary_damping_shift),
                        kCdefSecondaryTap1);
      sum = vmlaq_n_s16(sum,
                        Constrain(secondary_val[4], pixel, secondary_threshold,
                                  secondary_damping_shift),
                        kCdefSecondaryTap0);
      sum = vmlaq_n_s16(sum,
                        Constrain(secondary_val[5], pixel, secondary_threshold,
                                  secondary_damping_shift),
                        kCdefSecondaryTap0);
      sum = vmlaq_n_s16(sum,
                        Constrain(secondary_val[6], pixel, secondary_threshold,
                                  secondary_damping_shift),
                        kCdefSecondaryTap1);
      sum = vmlaq_n_s16(sum,
                        Constrain(secondary_val[7], pixel, secondary_threshold,
                                  secondary_damping_shift),
                        kCdefSecondaryTap1);
    }
    // Clip3(pixel + ((8 + sum - (sum < 0)) >> 4), min, max))
    const int16x8_t sum_lt_0 = vshrq_n_s16(sum, 15);
    sum = vaddq_s16(sum, sum_lt_0);
    int16x8_t result = vrsraq_n_s16(vreinterpretq_s16_u16(pixel), sum, 4);
    if (clipping_required) {
      result = vminq_s16(result, vreinterpretq_s16_u16(max));
      result = vmaxq_s16(result, vreinterpretq_s16_u16(min));
    }

    StorePixels<Pixel, width>(dst, dst_stride, result);

    src += (width == 8) ? src_stride : src_stride << 1;
    dst += (width == 8) ? dst_stride : dst_stride << 1;
    y -= (width == 8) ? 1 : 2;
  } while (y != 0);
}

}  // namespace

namespace low_bitdepth {
namespace {

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
  dsp->cdef_direction = CdefDirection_NEON<kBitdepth8>;
  dsp->cdef_filters[0][0] = CdefFilter_NEON<4, uint8_t>;
  dsp->cdef_filters[0][1] = CdefFilter_NEON<4, uint8_t, /*enable_primary=*/true,
                                            /*enable_secondary=*/false>;
  dsp->cdef_filters[0][2] =
      CdefFilter_NEON<4, uint8_t, /*enable_primary=*/false>;
  dsp->cdef_filters[1][0] = CdefFilter_NEON<8, uint8_t>;
  dsp->cdef_filters[1][1] = CdefFilter_NEON<8, uint8_t, /*enable_primary=*/true,
                                            /*enable_secondary=*/false>;
  dsp->cdef_filters[1][2] =
      CdefFilter_NEON<8, uint8_t, /*enable_primary=*/false>;
}

}  // namespace
}  // namespace low_bitdepth

#if LIBGAV1_MAX_BITDEPTH >= 10
namespace high_bitdepth {
namespace {

void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth10);
  assert(dsp != nullptr);
  dsp->cdef_direction = CdefDirection_NEON<kBitdepth10>;
  dsp->cdef_filters[0][0] = CdefFilter_NEON<4, uint16_t>;
  dsp->cdef_filters[0][1] =
      CdefFilter_NEON<4, uint16_t, /*enable_primary=*/true,
                      /*enable_secondary=*/false>;
  dsp->cdef_filters[0][2] =
      CdefFilter_NEON<4, uint16_t, /*enable_primary=*/false>;
  dsp->cdef_filters[1][0] = CdefFilter_NEON<8, uint16_t>;
  dsp->cdef_filters[1][1] =
      CdefFilter_NEON<8, uint16_t, /*enable_primary=*/true,
                      /*enable_secondary=*/false>;
  dsp->cdef_filters[1][2] =
      CdefFilter_NEON<8, uint16_t, /*enable_primary=*/false>;
}

}  // namespace
}  // namespace high_bitdepth
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

void CdefInit_NEON() {
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

void CdefInit_NEON() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_ENABLE_NEON
