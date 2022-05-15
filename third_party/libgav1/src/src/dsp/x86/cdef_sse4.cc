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

#include "src/dsp/cdef.h"
#include "src/utils/cpu.h"

#if LIBGAV1_TARGETING_SSE4_1

#include <emmintrin.h>
#include <tmmintrin.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

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

#include "src/dsp/cdef.inc"

// Used when calculating odd |cost[x]| values.
// Holds elements 1 3 5 7 7 7 7 7
alignas(16) constexpr uint32_t kCdefDivisionTableOddPadded[] = {
    420, 210, 140, 105, 105, 105, 105, 105};

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
LIBGAV1_ALWAYS_INLINE void AddPartial_D0_D4(__m128i* v_src_16,
                                            __m128i* partial_lo,
                                            __m128i* partial_hi) {
  // 00 01 02 03 04 05 06 07
  *partial_lo = v_src_16[0];
  // 00 00 00 00 00 00 00 00
  *partial_hi = _mm_setzero_si128();

  // 00 10 11 12 13 14 15 16
  *partial_lo = _mm_add_epi16(*partial_lo, _mm_slli_si128(v_src_16[1], 2));
  // 17 00 00 00 00 00 00 00
  *partial_hi = _mm_add_epi16(*partial_hi, _mm_srli_si128(v_src_16[1], 14));

  // 00 00 20 21 22 23 24 25
  *partial_lo = _mm_add_epi16(*partial_lo, _mm_slli_si128(v_src_16[2], 4));
  // 26 27 00 00 00 00 00 00
  *partial_hi = _mm_add_epi16(*partial_hi, _mm_srli_si128(v_src_16[2], 12));

  // 00 00 00 30 31 32 33 34
  *partial_lo = _mm_add_epi16(*partial_lo, _mm_slli_si128(v_src_16[3], 6));
  // 35 36 37 00 00 00 00 00
  *partial_hi = _mm_add_epi16(*partial_hi, _mm_srli_si128(v_src_16[3], 10));

  // 00 00 00 00 40 41 42 43
  *partial_lo = _mm_add_epi16(*partial_lo, _mm_slli_si128(v_src_16[4], 8));
  // 44 45 46 47 00 00 00 00
  *partial_hi = _mm_add_epi16(*partial_hi, _mm_srli_si128(v_src_16[4], 8));

  // 00 00 00 00 00 50 51 52
  *partial_lo = _mm_add_epi16(*partial_lo, _mm_slli_si128(v_src_16[5], 10));
  // 53 54 55 56 57 00 00 00
  *partial_hi = _mm_add_epi16(*partial_hi, _mm_srli_si128(v_src_16[5], 6));

  // 00 00 00 00 00 00 60 61
  *partial_lo = _mm_add_epi16(*partial_lo, _mm_slli_si128(v_src_16[6], 12));
  // 62 63 64 65 66 67 00 00
  *partial_hi = _mm_add_epi16(*partial_hi, _mm_srli_si128(v_src_16[6], 4));

  // 00 00 00 00 00 00 00 70
  *partial_lo = _mm_add_epi16(*partial_lo, _mm_slli_si128(v_src_16[7], 14));
  // 71 72 73 74 75 76 77 00
  *partial_hi = _mm_add_epi16(*partial_hi, _mm_srli_si128(v_src_16[7], 2));
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
LIBGAV1_ALWAYS_INLINE void AddPartial_D1_D3(__m128i* v_src_16,
                                            __m128i* partial_lo,
                                            __m128i* partial_hi) {
  __m128i v_d1_temp[8];
  const __m128i v_zero = _mm_setzero_si128();

  for (int i = 0; i < 8; ++i) {
    v_d1_temp[i] = _mm_hadd_epi16(v_src_16[i], v_zero);
  }

  *partial_lo = *partial_hi = v_zero;
  // A0 A1 A2 A3 00 00 00 00
  *partial_lo = _mm_add_epi16(*partial_lo, v_d1_temp[0]);

  // 00 B0 B1 B2 B3 00 00 00
  *partial_lo = _mm_add_epi16(*partial_lo, _mm_slli_si128(v_d1_temp[1], 2));

  // 00 00 C0 C1 C2 C3 00 00
  *partial_lo = _mm_add_epi16(*partial_lo, _mm_slli_si128(v_d1_temp[2], 4));
  // 00 00 00 D0 D1 D2 D3 00
  *partial_lo = _mm_add_epi16(*partial_lo, _mm_slli_si128(v_d1_temp[3], 6));
  // 00 00 00 00 E0 E1 E2 E3
  *partial_lo = _mm_add_epi16(*partial_lo, _mm_slli_si128(v_d1_temp[4], 8));

  // 00 00 00 00 00 F0 F1 F2
  *partial_lo = _mm_add_epi16(*partial_lo, _mm_slli_si128(v_d1_temp[5], 10));
  // F3 00 00 00 00 00 00 00
  *partial_hi = _mm_add_epi16(*partial_hi, _mm_srli_si128(v_d1_temp[5], 6));

  // 00 00 00 00 00 00 G0 G1
  *partial_lo = _mm_add_epi16(*partial_lo, _mm_slli_si128(v_d1_temp[6], 12));
  // G2 G3 00 00 00 00 00 00
  *partial_hi = _mm_add_epi16(*partial_hi, _mm_srli_si128(v_d1_temp[6], 4));

  // 00 00 00 00 00 00 00 H0
  *partial_lo = _mm_add_epi16(*partial_lo, _mm_slli_si128(v_d1_temp[7], 14));
  // H1 H2 H3 00 00 00 00 00
  *partial_hi = _mm_add_epi16(*partial_hi, _mm_srli_si128(v_d1_temp[7], 2));
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
LIBGAV1_ALWAYS_INLINE void AddPartial_D5_D7(__m128i* v_src, __m128i* partial_lo,
                                            __m128i* partial_hi) {
  __m128i v_pair_add[4];
  // Add vertical source pairs.
  v_pair_add[0] = _mm_add_epi16(v_src[0], v_src[1]);
  v_pair_add[1] = _mm_add_epi16(v_src[2], v_src[3]);
  v_pair_add[2] = _mm_add_epi16(v_src[4], v_src[5]);
  v_pair_add[3] = _mm_add_epi16(v_src[6], v_src[7]);

  // 00 01 02 03 04 05 06 07
  // 10 11 12 13 14 15 16 17
  *partial_lo = v_pair_add[0];
  // 00 00 00 00 00 00 00 00
  // 00 00 00 00 00 00 00 00
  *partial_hi = _mm_setzero_si128();

  // 00 20 21 22 23 24 25 26
  // 00 30 31 32 33 34 35 36
  *partial_lo = _mm_add_epi16(*partial_lo, _mm_slli_si128(v_pair_add[1], 2));
  // 27 00 00 00 00 00 00 00
  // 37 00 00 00 00 00 00 00
  *partial_hi = _mm_add_epi16(*partial_hi, _mm_srli_si128(v_pair_add[1], 14));

  // 00 00 40 41 42 43 44 45
  // 00 00 50 51 52 53 54 55
  *partial_lo = _mm_add_epi16(*partial_lo, _mm_slli_si128(v_pair_add[2], 4));
  // 46 47 00 00 00 00 00 00
  // 56 57 00 00 00 00 00 00
  *partial_hi = _mm_add_epi16(*partial_hi, _mm_srli_si128(v_pair_add[2], 12));

  // 00 00 00 60 61 62 63 64
  // 00 00 00 70 71 72 73 74
  *partial_lo = _mm_add_epi16(*partial_lo, _mm_slli_si128(v_pair_add[3], 6));
  // 65 66 67 00 00 00 00 00
  // 75 76 77 00 00 00 00 00
  *partial_hi = _mm_add_epi16(*partial_hi, _mm_srli_si128(v_pair_add[3], 10));
}

LIBGAV1_ALWAYS_INLINE void AddPartial(const uint8_t* LIBGAV1_RESTRICT src,
                                      ptrdiff_t stride, __m128i* partial_lo,
                                      __m128i* partial_hi) {
  // 8x8 input
  // 00 01 02 03 04 05 06 07
  // 10 11 12 13 14 15 16 17
  // 20 21 22 23 24 25 26 27
  // 30 31 32 33 34 35 36 37
  // 40 41 42 43 44 45 46 47
  // 50 51 52 53 54 55 56 57
  // 60 61 62 63 64 65 66 67
  // 70 71 72 73 74 75 76 77
  __m128i v_src[8];
  for (auto& i : v_src) {
    i = LoadLo8(src);
    src += stride;
  }

  const __m128i v_zero = _mm_setzero_si128();
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
  const __m128i v_src_4_0 = _mm_unpacklo_epi64(v_src[0], v_src[4]);
  const __m128i v_src_5_1 = _mm_unpacklo_epi64(v_src[1], v_src[5]);
  const __m128i v_src_6_2 = _mm_unpacklo_epi64(v_src[2], v_src[6]);
  const __m128i v_src_7_3 = _mm_unpacklo_epi64(v_src[3], v_src[7]);
  const __m128i v_hsum_4_0 = _mm_sad_epu8(v_src_4_0, v_zero);
  const __m128i v_hsum_5_1 = _mm_sad_epu8(v_src_5_1, v_zero);
  const __m128i v_hsum_6_2 = _mm_sad_epu8(v_src_6_2, v_zero);
  const __m128i v_hsum_7_3 = _mm_sad_epu8(v_src_7_3, v_zero);
  const __m128i v_hsum_1_0 = _mm_unpacklo_epi16(v_hsum_4_0, v_hsum_5_1);
  const __m128i v_hsum_3_2 = _mm_unpacklo_epi16(v_hsum_6_2, v_hsum_7_3);
  const __m128i v_hsum_5_4 = _mm_unpackhi_epi16(v_hsum_4_0, v_hsum_5_1);
  const __m128i v_hsum_7_6 = _mm_unpackhi_epi16(v_hsum_6_2, v_hsum_7_3);
  partial_lo[2] =
      _mm_unpacklo_epi64(_mm_unpacklo_epi32(v_hsum_1_0, v_hsum_3_2),
                         _mm_unpacklo_epi32(v_hsum_5_4, v_hsum_7_6));

  __m128i v_src_16[8];
  for (int i = 0; i < 8; ++i) {
    v_src_16[i] = _mm_cvtepu8_epi16(v_src[i]);
  }

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
  partial_lo[6] = v_src_16[0];
  for (int i = 1; i < 8; ++i) {
    partial_lo[6] = _mm_add_epi16(partial_lo[6], v_src_16[i]);
  }

  // partial for direction 0
  AddPartial_D0_D4(v_src_16, &partial_lo[0], &partial_hi[0]);

  // partial for direction 1
  AddPartial_D1_D3(v_src_16, &partial_lo[1], &partial_hi[1]);

  // partial for direction 7
  AddPartial_D5_D7(v_src_16, &partial_lo[7], &partial_hi[7]);

  __m128i v_src_reverse[8];
  const __m128i reverser =
      _mm_set_epi32(0x01000302, 0x05040706, 0x09080b0a, 0x0d0c0f0e);
  for (int i = 0; i < 8; ++i) {
    v_src_reverse[i] = _mm_shuffle_epi8(v_src_16[i], reverser);
  }

  // partial for direction 4
  AddPartial_D0_D4(v_src_reverse, &partial_lo[4], &partial_hi[4]);

  // partial for direction 3
  AddPartial_D1_D3(v_src_reverse, &partial_lo[3], &partial_hi[3]);

  // partial for direction 5
  AddPartial_D5_D7(v_src_reverse, &partial_lo[5], &partial_hi[5]);
}

inline uint32_t SumVector_S32(__m128i a) {
  a = _mm_hadd_epi32(a, a);
  a = _mm_add_epi32(a, _mm_srli_si128(a, 4));
  return _mm_cvtsi128_si32(a);
}

// |cost[0]| and |cost[4]| square the input and sum with the corresponding
// element from the other end of the vector:
// |kCdefDivisionTable[]| element:
// cost[0] += (Square(partial[0][i]) + Square(partial[0][14 - i])) *
//             kCdefDivisionTable[i + 1];
// cost[0] += Square(partial[0][7]) * kCdefDivisionTable[8];
inline uint32_t Cost0Or4(const __m128i a, const __m128i b,
                         const __m128i division_table[2]) {
  // Reverse and clear upper 2 bytes.
  const __m128i reverser = _mm_set_epi32(static_cast<int>(0x80800100),
                                         0x03020504, 0x07060908, 0x0b0a0d0c);
  // 14 13 12 11 10 09 08 ZZ
  const __m128i b_reversed = _mm_shuffle_epi8(b, reverser);
  // 00 14 01 13 02 12 03 11
  const __m128i ab_lo = _mm_unpacklo_epi16(a, b_reversed);
  // 04 10 05 09 06 08 07 ZZ
  const __m128i ab_hi = _mm_unpackhi_epi16(a, b_reversed);

  // Square(partial[0][i]) + Square(partial[0][14 - i])
  const __m128i square_lo = _mm_madd_epi16(ab_lo, ab_lo);
  const __m128i square_hi = _mm_madd_epi16(ab_hi, ab_hi);

  const __m128i c = _mm_mullo_epi32(square_lo, division_table[0]);
  const __m128i d = _mm_mullo_epi32(square_hi, division_table[1]);
  return SumVector_S32(_mm_add_epi32(c, d));
}

inline uint32_t CostOdd(const __m128i a, const __m128i b,
                        const __m128i division_table[2]) {
  // Reverse and clear upper 10 bytes.
  const __m128i reverser =
      _mm_set_epi32(static_cast<int>(0x80808080), static_cast<int>(0x80808080),
                    static_cast<int>(0x80800100), 0x03020504);
  // 10 09 08 ZZ ZZ ZZ ZZ ZZ
  const __m128i b_reversed = _mm_shuffle_epi8(b, reverser);
  // 00 10 01 09 02 08 03 ZZ
  const __m128i ab_lo = _mm_unpacklo_epi16(a, b_reversed);
  // 04 ZZ 05 ZZ 06 ZZ 07 ZZ
  const __m128i ab_hi = _mm_unpackhi_epi16(a, b_reversed);

  // Square(partial[0][i]) + Square(partial[0][10 - i])
  const __m128i square_lo = _mm_madd_epi16(ab_lo, ab_lo);
  const __m128i square_hi = _mm_madd_epi16(ab_hi, ab_hi);

  const __m128i c = _mm_mullo_epi32(square_lo, division_table[0]);
  const __m128i d = _mm_mullo_epi32(square_hi, division_table[1]);
  return SumVector_S32(_mm_add_epi32(c, d));
}

// Sum of squared elements.
inline uint32_t SquareSum_S16(const __m128i a) {
  const __m128i square = _mm_madd_epi16(a, a);
  return SumVector_S32(square);
}

void CdefDirection_SSE4_1(const void* LIBGAV1_RESTRICT const source,
                          ptrdiff_t stride,
                          uint8_t* LIBGAV1_RESTRICT const direction,
                          int* LIBGAV1_RESTRICT const variance) {
  assert(direction != nullptr);
  assert(variance != nullptr);
  const auto* src = static_cast<const uint8_t*>(source);
  uint32_t cost[8];
  __m128i partial_lo[8], partial_hi[8];

  AddPartial(src, stride, partial_lo, partial_hi);

  cost[2] = kCdefDivisionTable[7] * SquareSum_S16(partial_lo[2]);
  cost[6] = kCdefDivisionTable[7] * SquareSum_S16(partial_lo[6]);

  const __m128i division_table[2] = {LoadUnaligned16(kCdefDivisionTable),
                                     LoadUnaligned16(kCdefDivisionTable + 4)};

  cost[0] = Cost0Or4(partial_lo[0], partial_hi[0], division_table);
  cost[4] = Cost0Or4(partial_lo[4], partial_hi[4], division_table);

  const __m128i division_table_odd[2] = {
      LoadAligned16(kCdefDivisionTableOddPadded),
      LoadAligned16(kCdefDivisionTableOddPadded + 4)};

  cost[1] = CostOdd(partial_lo[1], partial_hi[1], division_table_odd);
  cost[3] = CostOdd(partial_lo[3], partial_hi[3], division_table_odd);
  cost[5] = CostOdd(partial_lo[5], partial_hi[5], division_table_odd);
  cost[7] = CostOdd(partial_lo[7], partial_hi[7], division_table_odd);

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
inline void LoadDirection(const uint16_t* LIBGAV1_RESTRICT const src,
                          const ptrdiff_t stride, __m128i* output,
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
  output[0] = LoadUnaligned16(src - y_0 * stride - x_0);
  output[1] = LoadUnaligned16(src + y_0 * stride + x_0);
  output[2] = LoadUnaligned16(src - y_1 * stride - x_1);
  output[3] = LoadUnaligned16(src + y_1 * stride + x_1);
}

// Load 4 vectors based on the given |direction|. Use when |block_width| == 4 to
// do 2 rows at a time.
void LoadDirection4(const uint16_t* LIBGAV1_RESTRICT const src,
                    const ptrdiff_t stride, __m128i* output,
                    const int direction) {
  const int y_0 = kCdefDirections[direction][0][0];
  const int x_0 = kCdefDirections[direction][0][1];
  const int y_1 = kCdefDirections[direction][1][0];
  const int x_1 = kCdefDirections[direction][1][1];
  output[0] = LoadHi8(LoadLo8(src - y_0 * stride - x_0),
                      src - y_0 * stride + stride - x_0);
  output[1] = LoadHi8(LoadLo8(src + y_0 * stride + x_0),
                      src + y_0 * stride + stride + x_0);
  output[2] = LoadHi8(LoadLo8(src - y_1 * stride - x_1),
                      src - y_1 * stride + stride - x_1);
  output[3] = LoadHi8(LoadLo8(src + y_1 * stride + x_1),
                      src + y_1 * stride + stride + x_1);
}

inline __m128i Constrain(const __m128i& pixel, const __m128i& reference,
                         const __m128i& damping, const __m128i& threshold) {
  const __m128i diff = _mm_sub_epi16(pixel, reference);
  const __m128i abs_diff = _mm_abs_epi16(diff);
  // sign(diff) * Clip3(threshold - (std::abs(diff) >> damping),
  //                    0, std::abs(diff))
  const __m128i shifted_diff = _mm_srl_epi16(abs_diff, damping);
  // For bitdepth == 8, the threshold range is [0, 15] and the damping range is
  // [3, 6]. If pixel == kCdefLargeValue(0x4000), shifted_diff will always be
  // larger than threshold. Subtract using saturation will return 0 when pixel
  // == kCdefLargeValue.
  static_assert(kCdefLargeValue == 0x4000, "Invalid kCdefLargeValue");
  const __m128i thresh_minus_shifted_diff =
      _mm_subs_epu16(threshold, shifted_diff);
  const __m128i clamp_abs_diff =
      _mm_min_epi16(thresh_minus_shifted_diff, abs_diff);
  // Restore the sign.
  return _mm_sign_epi16(clamp_abs_diff, diff);
}

inline __m128i ApplyConstrainAndTap(const __m128i& pixel, const __m128i& val,
                                    const __m128i& tap, const __m128i& damping,
                                    const __m128i& threshold) {
  const __m128i constrained = Constrain(val, pixel, damping, threshold);
  return _mm_mullo_epi16(constrained, tap);
}

template <int width, bool enable_primary = true, bool enable_secondary = true>
void CdefFilter_SSE4_1(const uint16_t* LIBGAV1_RESTRICT src,
                       const ptrdiff_t src_stride, const int height,
                       const int primary_strength, const int secondary_strength,
                       const int damping, const int direction,
                       void* LIBGAV1_RESTRICT dest,
                       const ptrdiff_t dst_stride) {
  static_assert(width == 8 || width == 4, "Invalid CDEF width.");
  static_assert(enable_primary || enable_secondary, "");
  constexpr bool clipping_required = enable_primary && enable_secondary;
  auto* dst = static_cast<uint8_t*>(dest);
  __m128i primary_damping_shift, secondary_damping_shift;

  // FloorLog2() requires input to be > 0.
  // 8-bit damping range: Y: [3, 6], UV: [2, 5].
  if (enable_primary) {
    // primary_strength: [0, 15] -> FloorLog2: [0, 3] so a clamp is necessary
    // for UV filtering.
    primary_damping_shift =
        _mm_cvtsi32_si128(std::max(0, damping - FloorLog2(primary_strength)));
  }
  if (enable_secondary) {
    // secondary_strength: [0, 4] -> FloorLog2: [0, 2] so no clamp to 0 is
    // necessary.
    assert(damping - FloorLog2(secondary_strength) >= 0);
    secondary_damping_shift =
        _mm_cvtsi32_si128(damping - FloorLog2(secondary_strength));
  }

  const __m128i primary_tap_0 =
      _mm_set1_epi16(kCdefPrimaryTaps[primary_strength & 1][0]);
  const __m128i primary_tap_1 =
      _mm_set1_epi16(kCdefPrimaryTaps[primary_strength & 1][1]);
  const __m128i secondary_tap_0 = _mm_set1_epi16(kCdefSecondaryTap0);
  const __m128i secondary_tap_1 = _mm_set1_epi16(kCdefSecondaryTap1);
  const __m128i cdef_large_value_mask =
      _mm_set1_epi16(static_cast<int16_t>(~kCdefLargeValue));
  const __m128i primary_threshold = _mm_set1_epi16(primary_strength);
  const __m128i secondary_threshold = _mm_set1_epi16(secondary_strength);

  int y = height;
  do {
    __m128i pixel;
    if (width == 8) {
      pixel = LoadUnaligned16(src);
    } else {
      pixel = LoadHi8(LoadLo8(src), src + src_stride);
    }

    __m128i min = pixel;
    __m128i max = pixel;
    __m128i sum;

    if (enable_primary) {
      // Primary |direction|.
      __m128i primary_val[4];
      if (width == 8) {
        LoadDirection(src, src_stride, primary_val, direction);
      } else {
        LoadDirection4(src, src_stride, primary_val, direction);
      }

      if (clipping_required) {
        min = _mm_min_epu16(min, primary_val[0]);
        min = _mm_min_epu16(min, primary_val[1]);
        min = _mm_min_epu16(min, primary_val[2]);
        min = _mm_min_epu16(min, primary_val[3]);

        // The source is 16 bits, however, we only really care about the lower
        // 8 bits.  The upper 8 bits contain the "large" flag.  After the final
        // primary max has been calculated, zero out the upper 8 bits.  Use this
        // to find the "16 bit" max.
        const __m128i max_p01 = _mm_max_epu8(primary_val[0], primary_val[1]);
        const __m128i max_p23 = _mm_max_epu8(primary_val[2], primary_val[3]);
        const __m128i max_p = _mm_max_epu8(max_p01, max_p23);
        max = _mm_max_epu16(max, _mm_and_si128(max_p, cdef_large_value_mask));
      }

      sum = ApplyConstrainAndTap(pixel, primary_val[0], primary_tap_0,
                                 primary_damping_shift, primary_threshold);
      sum = _mm_add_epi16(
          sum, ApplyConstrainAndTap(pixel, primary_val[1], primary_tap_0,
                                    primary_damping_shift, primary_threshold));
      sum = _mm_add_epi16(
          sum, ApplyConstrainAndTap(pixel, primary_val[2], primary_tap_1,
                                    primary_damping_shift, primary_threshold));
      sum = _mm_add_epi16(
          sum, ApplyConstrainAndTap(pixel, primary_val[3], primary_tap_1,
                                    primary_damping_shift, primary_threshold));
    } else {
      sum = _mm_setzero_si128();
    }

    if (enable_secondary) {
      // Secondary |direction| values (+/- 2). Clamp |direction|.
      __m128i secondary_val[8];
      if (width == 8) {
        LoadDirection(src, src_stride, secondary_val, direction + 2);
        LoadDirection(src, src_stride, secondary_val + 4, direction - 2);
      } else {
        LoadDirection4(src, src_stride, secondary_val, direction + 2);
        LoadDirection4(src, src_stride, secondary_val + 4, direction - 2);
      }

      if (clipping_required) {
        min = _mm_min_epu16(min, secondary_val[0]);
        min = _mm_min_epu16(min, secondary_val[1]);
        min = _mm_min_epu16(min, secondary_val[2]);
        min = _mm_min_epu16(min, secondary_val[3]);
        min = _mm_min_epu16(min, secondary_val[4]);
        min = _mm_min_epu16(min, secondary_val[5]);
        min = _mm_min_epu16(min, secondary_val[6]);
        min = _mm_min_epu16(min, secondary_val[7]);

        const __m128i max_s01 =
            _mm_max_epu8(secondary_val[0], secondary_val[1]);
        const __m128i max_s23 =
            _mm_max_epu8(secondary_val[2], secondary_val[3]);
        const __m128i max_s45 =
            _mm_max_epu8(secondary_val[4], secondary_val[5]);
        const __m128i max_s67 =
            _mm_max_epu8(secondary_val[6], secondary_val[7]);
        const __m128i max_s = _mm_max_epu8(_mm_max_epu8(max_s01, max_s23),
                                           _mm_max_epu8(max_s45, max_s67));
        max = _mm_max_epu16(max, _mm_and_si128(max_s, cdef_large_value_mask));
      }

      sum = _mm_add_epi16(
          sum,
          ApplyConstrainAndTap(pixel, secondary_val[0], secondary_tap_0,
                               secondary_damping_shift, secondary_threshold));
      sum = _mm_add_epi16(
          sum,
          ApplyConstrainAndTap(pixel, secondary_val[1], secondary_tap_0,
                               secondary_damping_shift, secondary_threshold));
      sum = _mm_add_epi16(
          sum,
          ApplyConstrainAndTap(pixel, secondary_val[2], secondary_tap_1,
                               secondary_damping_shift, secondary_threshold));
      sum = _mm_add_epi16(
          sum,
          ApplyConstrainAndTap(pixel, secondary_val[3], secondary_tap_1,
                               secondary_damping_shift, secondary_threshold));
      sum = _mm_add_epi16(
          sum,
          ApplyConstrainAndTap(pixel, secondary_val[4], secondary_tap_0,
                               secondary_damping_shift, secondary_threshold));
      sum = _mm_add_epi16(
          sum,
          ApplyConstrainAndTap(pixel, secondary_val[5], secondary_tap_0,
                               secondary_damping_shift, secondary_threshold));
      sum = _mm_add_epi16(
          sum,
          ApplyConstrainAndTap(pixel, secondary_val[6], secondary_tap_1,
                               secondary_damping_shift, secondary_threshold));
      sum = _mm_add_epi16(
          sum,
          ApplyConstrainAndTap(pixel, secondary_val[7], secondary_tap_1,
                               secondary_damping_shift, secondary_threshold));
    }
    // Clip3(pixel + ((8 + sum - (sum < 0)) >> 4), min, max))
    const __m128i sum_lt_0 = _mm_srai_epi16(sum, 15);
    // 8 + sum
    sum = _mm_add_epi16(sum, _mm_set1_epi16(8));
    // (... - (sum < 0)) >> 4
    sum = _mm_add_epi16(sum, sum_lt_0);
    sum = _mm_srai_epi16(sum, 4);
    // pixel + ...
    sum = _mm_add_epi16(sum, pixel);
    if (clipping_required) {
      // Clip3
      sum = _mm_min_epi16(sum, max);
      sum = _mm_max_epi16(sum, min);
    }

    const __m128i result = _mm_packus_epi16(sum, sum);
    if (width == 8) {
      src += src_stride;
      StoreLo8(dst, result);
      dst += dst_stride;
      --y;
    } else {
      src += src_stride << 1;
      Store4(dst, result);
      dst += dst_stride;
      Store4(dst, _mm_srli_si128(result, 4));
      dst += dst_stride;
      y -= 2;
    }
  } while (y != 0);
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(8);
  assert(dsp != nullptr);
  dsp->cdef_direction = CdefDirection_SSE4_1;
  dsp->cdef_filters[0][0] = CdefFilter_SSE4_1<4>;
  dsp->cdef_filters[0][1] =
      CdefFilter_SSE4_1<4, /*enable_primary=*/true, /*enable_secondary=*/false>;
  dsp->cdef_filters[0][2] = CdefFilter_SSE4_1<4, /*enable_primary=*/false>;
  dsp->cdef_filters[1][0] = CdefFilter_SSE4_1<8>;
  dsp->cdef_filters[1][1] =
      CdefFilter_SSE4_1<8, /*enable_primary=*/true, /*enable_secondary=*/false>;
  dsp->cdef_filters[1][2] = CdefFilter_SSE4_1<8, /*enable_primary=*/false>;
}

}  // namespace
}  // namespace low_bitdepth

void CdefInit_SSE4_1() { low_bitdepth::Init8bpp(); }

}  // namespace dsp
}  // namespace libgav1
#else   // !LIBGAV1_TARGETING_SSE4_1
namespace libgav1 {
namespace dsp {

void CdefInit_SSE4_1() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_TARGETING_SSE4_1
