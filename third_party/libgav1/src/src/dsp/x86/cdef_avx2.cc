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

#include "src/dsp/cdef.h"
#include "src/utils/cpu.h"

#if LIBGAV1_TARGETING_AVX2
#include <immintrin.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/dsp/x86/common_avx2.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"

namespace libgav1 {
namespace dsp {
namespace low_bitdepth {
namespace {

#include "src/dsp/cdef.inc"

// Used when calculating odd |cost[x]| values.
// Holds elements 1 3 5 7 7 7 7 7
alignas(32) constexpr uint32_t kCdefDivisionTableOddPairsPadded[] = {
    420, 210, 140, 105, 420, 210, 140, 105,
    105, 105, 105, 105, 105, 105, 105, 105};

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
LIBGAV1_ALWAYS_INLINE void AddPartial_D0_D4(__m256i* v_src_16,
                                            __m256i* partial_lo,
                                            __m256i* partial_hi) {
  // 00 01 02 03 04 05 06 07
  *partial_lo = v_src_16[0];
  // 00 00 00 00 00 00 00 00
  *partial_hi = _mm256_setzero_si256();

  // 00 10 11 12 13 14 15 16
  *partial_lo =
      _mm256_add_epi16(*partial_lo, _mm256_slli_si256(v_src_16[1], 2));
  // 17 00 00 00 00 00 00 00
  *partial_hi =
      _mm256_add_epi16(*partial_hi, _mm256_srli_si256(v_src_16[1], 14));

  // 00 00 20 21 22 23 24 25
  *partial_lo =
      _mm256_add_epi16(*partial_lo, _mm256_slli_si256(v_src_16[2], 4));
  // 26 27 00 00 00 00 00 00
  *partial_hi =
      _mm256_add_epi16(*partial_hi, _mm256_srli_si256(v_src_16[2], 12));

  // 00 00 00 30 31 32 33 34
  *partial_lo =
      _mm256_add_epi16(*partial_lo, _mm256_slli_si256(v_src_16[3], 6));
  // 35 36 37 00 00 00 00 00
  *partial_hi =
      _mm256_add_epi16(*partial_hi, _mm256_srli_si256(v_src_16[3], 10));

  // 00 00 00 00 40 41 42 43
  *partial_lo =
      _mm256_add_epi16(*partial_lo, _mm256_slli_si256(v_src_16[4], 8));
  // 44 45 46 47 00 00 00 00
  *partial_hi =
      _mm256_add_epi16(*partial_hi, _mm256_srli_si256(v_src_16[4], 8));

  // 00 00 00 00 00 50 51 52
  *partial_lo =
      _mm256_add_epi16(*partial_lo, _mm256_slli_si256(v_src_16[5], 10));
  // 53 54 55 56 57 00 00 00
  *partial_hi =
      _mm256_add_epi16(*partial_hi, _mm256_srli_si256(v_src_16[5], 6));

  // 00 00 00 00 00 00 60 61
  *partial_lo =
      _mm256_add_epi16(*partial_lo, _mm256_slli_si256(v_src_16[6], 12));
  // 62 63 64 65 66 67 00 00
  *partial_hi =
      _mm256_add_epi16(*partial_hi, _mm256_srli_si256(v_src_16[6], 4));

  // 00 00 00 00 00 00 00 70
  *partial_lo =
      _mm256_add_epi16(*partial_lo, _mm256_slli_si256(v_src_16[7], 14));
  // 71 72 73 74 75 76 77 00
  *partial_hi =
      _mm256_add_epi16(*partial_hi, _mm256_srli_si256(v_src_16[7], 2));
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
LIBGAV1_ALWAYS_INLINE void AddPartial_D1_D3(__m256i* v_src_16,
                                            __m256i* partial_lo,
                                            __m256i* partial_hi) {
  __m256i v_d1_temp[8];
  const __m256i v_zero = _mm256_setzero_si256();

  for (int i = 0; i < 8; ++i) {
    v_d1_temp[i] = _mm256_hadd_epi16(v_src_16[i], v_zero);
  }

  *partial_lo = *partial_hi = v_zero;
  // A0 A1 A2 A3 00 00 00 00
  *partial_lo = _mm256_add_epi16(*partial_lo, v_d1_temp[0]);

  // 00 B0 B1 B2 B3 00 00 00
  *partial_lo =
      _mm256_add_epi16(*partial_lo, _mm256_slli_si256(v_d1_temp[1], 2));

  // 00 00 C0 C1 C2 C3 00 00
  *partial_lo =
      _mm256_add_epi16(*partial_lo, _mm256_slli_si256(v_d1_temp[2], 4));
  // 00 00 00 D0 D1 D2 D3 00
  *partial_lo =
      _mm256_add_epi16(*partial_lo, _mm256_slli_si256(v_d1_temp[3], 6));
  // 00 00 00 00 E0 E1 E2 E3
  *partial_lo =
      _mm256_add_epi16(*partial_lo, _mm256_slli_si256(v_d1_temp[4], 8));

  // 00 00 00 00 00 F0 F1 F2
  *partial_lo =
      _mm256_add_epi16(*partial_lo, _mm256_slli_si256(v_d1_temp[5], 10));
  // F3 00 00 00 00 00 00 00
  *partial_hi =
      _mm256_add_epi16(*partial_hi, _mm256_srli_si256(v_d1_temp[5], 6));

  // 00 00 00 00 00 00 G0 G1
  *partial_lo =
      _mm256_add_epi16(*partial_lo, _mm256_slli_si256(v_d1_temp[6], 12));
  // G2 G3 00 00 00 00 00 00
  *partial_hi =
      _mm256_add_epi16(*partial_hi, _mm256_srli_si256(v_d1_temp[6], 4));

  // 00 00 00 00 00 00 00 H0
  *partial_lo =
      _mm256_add_epi16(*partial_lo, _mm256_slli_si256(v_d1_temp[7], 14));
  // H1 H2 H3 00 00 00 00 00
  *partial_hi =
      _mm256_add_epi16(*partial_hi, _mm256_srli_si256(v_d1_temp[7], 2));
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
LIBGAV1_ALWAYS_INLINE void AddPartial_D7_D5(__m256i* v_src, __m256i* partial_lo,
                                            __m256i* partial_hi) {
  __m256i v_pair_add[4];
  // Add vertical source pairs.
  v_pair_add[0] = _mm256_add_epi16(v_src[0], v_src[1]);
  v_pair_add[1] = _mm256_add_epi16(v_src[2], v_src[3]);
  v_pair_add[2] = _mm256_add_epi16(v_src[4], v_src[5]);
  v_pair_add[3] = _mm256_add_epi16(v_src[6], v_src[7]);

  // 00 01 02 03 04 05 06 07
  // 10 11 12 13 14 15 16 17
  *partial_lo = v_pair_add[0];
  // 00 00 00 00 00 00 00 00
  // 00 00 00 00 00 00 00 00
  *partial_hi = _mm256_setzero_si256();

  // 00 20 21 22 23 24 25 26
  // 00 30 31 32 33 34 35 36
  *partial_lo =
      _mm256_add_epi16(*partial_lo, _mm256_slli_si256(v_pair_add[1], 2));
  // 27 00 00 00 00 00 00 00
  // 37 00 00 00 00 00 00 00
  *partial_hi =
      _mm256_add_epi16(*partial_hi, _mm256_srli_si256(v_pair_add[1], 14));

  // 00 00 40 41 42 43 44 45
  // 00 00 50 51 52 53 54 55
  *partial_lo =
      _mm256_add_epi16(*partial_lo, _mm256_slli_si256(v_pair_add[2], 4));
  // 46 47 00 00 00 00 00 00
  // 56 57 00 00 00 00 00 00
  *partial_hi =
      _mm256_add_epi16(*partial_hi, _mm256_srli_si256(v_pair_add[2], 12));

  // 00 00 00 60 61 62 63 64
  // 00 00 00 70 71 72 73 74
  *partial_lo =
      _mm256_add_epi16(*partial_lo, _mm256_slli_si256(v_pair_add[3], 6));
  // 65 66 67 00 00 00 00 00
  // 75 76 77 00 00 00 00 00
  *partial_hi =
      _mm256_add_epi16(*partial_hi, _mm256_srli_si256(v_pair_add[3], 10));
}

LIBGAV1_ALWAYS_INLINE void AddPartial(const uint8_t* LIBGAV1_RESTRICT src,
                                      ptrdiff_t stride, __m256i* partial) {
  // 8x8 input
  // 00 01 02 03 04 05 06 07
  // 10 11 12 13 14 15 16 17
  // 20 21 22 23 24 25 26 27
  // 30 31 32 33 34 35 36 37
  // 40 41 42 43 44 45 46 47
  // 50 51 52 53 54 55 56 57
  // 60 61 62 63 64 65 66 67
  // 70 71 72 73 74 75 76 77
  __m256i v_src[8];
  for (auto& i : v_src) {
    i = _mm256_castsi128_si256(LoadLo8(src));
    // Dup lower lane.
    i = _mm256_permute2x128_si256(i, i, 0x0);
    src += stride;
  }

  const __m256i v_zero = _mm256_setzero_si256();
  // partial for direction 2
  // --------------------------------------------------------------------------
  // partial[2][i] += x;
  // 00 10 20 30 40 50 60 70  xx xx xx xx xx xx xx xx
  // 01 11 21 33 41 51 61 71  xx xx xx xx xx xx xx xx
  // 02 12 22 33 42 52 62 72  xx xx xx xx xx xx xx xx
  // 03 13 23 33 43 53 63 73  xx xx xx xx xx xx xx xx
  // 04 14 24 34 44 54 64 74  xx xx xx xx xx xx xx xx
  // 05 15 25 35 45 55 65 75  xx xx xx xx xx xx xx xx
  // 06 16 26 36 46 56 66 76  xx xx xx xx xx xx xx xx
  // 07 17 27 37 47 57 67 77  xx xx xx xx xx xx xx xx
  const __m256i v_src_4_0 = _mm256_unpacklo_epi64(v_src[0], v_src[4]);
  const __m256i v_src_5_1 = _mm256_unpacklo_epi64(v_src[1], v_src[5]);
  const __m256i v_src_6_2 = _mm256_unpacklo_epi64(v_src[2], v_src[6]);
  const __m256i v_src_7_3 = _mm256_unpacklo_epi64(v_src[3], v_src[7]);
  const __m256i v_hsum_4_0 = _mm256_sad_epu8(v_src_4_0, v_zero);
  const __m256i v_hsum_5_1 = _mm256_sad_epu8(v_src_5_1, v_zero);
  const __m256i v_hsum_6_2 = _mm256_sad_epu8(v_src_6_2, v_zero);
  const __m256i v_hsum_7_3 = _mm256_sad_epu8(v_src_7_3, v_zero);
  const __m256i v_hsum_1_0 = _mm256_unpacklo_epi16(v_hsum_4_0, v_hsum_5_1);
  const __m256i v_hsum_3_2 = _mm256_unpacklo_epi16(v_hsum_6_2, v_hsum_7_3);
  const __m256i v_hsum_5_4 = _mm256_unpackhi_epi16(v_hsum_4_0, v_hsum_5_1);
  const __m256i v_hsum_7_6 = _mm256_unpackhi_epi16(v_hsum_6_2, v_hsum_7_3);
  partial[2] =
      _mm256_unpacklo_epi64(_mm256_unpacklo_epi32(v_hsum_1_0, v_hsum_3_2),
                            _mm256_unpacklo_epi32(v_hsum_5_4, v_hsum_7_6));

  const __m256i extend_reverse = SetrM128i(
      _mm_set_epi32(static_cast<int>(0x80078006), static_cast<int>(0x80058004),
                    static_cast<int>(0x80038002), static_cast<int>(0x80018000)),
      _mm_set_epi32(static_cast<int>(0x80008001), static_cast<int>(0x80028003),
                    static_cast<int>(0x80048005),
                    static_cast<int>(0x80068007)));

  for (auto& i : v_src) {
    // Zero extend unsigned 8 to 16. The upper lane is reversed.
    i = _mm256_shuffle_epi8(i, extend_reverse);
  }

  // partial for direction 6
  // --------------------------------------------------------------------------
  // partial[6][j] += x;
  // 00 01 02 03 04 05 06 07  xx xx xx xx xx xx xx xx
  // 10 11 12 13 14 15 16 17  xx xx xx xx xx xx xx xx
  // 20 21 22 23 24 25 26 27  xx xx xx xx xx xx xx xx
  // 30 31 32 33 34 35 36 37  xx xx xx xx xx xx xx xx
  // 40 41 42 43 44 45 46 47  xx xx xx xx xx xx xx xx
  // 50 51 52 53 54 55 56 57  xx xx xx xx xx xx xx xx
  // 60 61 62 63 64 65 66 67  xx xx xx xx xx xx xx xx
  // 70 71 72 73 74 75 76 77  xx xx xx xx xx xx xx xx
  partial[6] = v_src[0];
  for (int i = 1; i < 8; ++i) {
    partial[6] = _mm256_add_epi16(partial[6], v_src[i]);
  }

  AddPartial_D0_D4(v_src, &partial[0], &partial[4]);
  AddPartial_D1_D3(v_src, &partial[1], &partial[3]);
  AddPartial_D7_D5(v_src, &partial[7], &partial[5]);
}

inline __m256i SumVectorPair_S32(__m256i a) {
  a = _mm256_hadd_epi32(a, a);
  a = _mm256_add_epi32(a, _mm256_srli_si256(a, 4));
  return a;
}

// |cost[0]| and |cost[4]| square the input and sum with the corresponding
// element from the other end of the vector:
// |kCdefDivisionTable[]| element:
// cost[0] += (Square(partial[0][i]) + Square(partial[0][14 - i])) *
//             kCdefDivisionTable[i + 1];
// cost[0] += Square(partial[0][7]) * kCdefDivisionTable[8];
inline void Cost0Or4_Pair(uint32_t* cost, const __m256i partial_0,
                          const __m256i partial_4,
                          const __m256i division_table) {
  const __m256i division_table_0 =
      _mm256_permute2x128_si256(division_table, division_table, 0x0);
  const __m256i division_table_1 =
      _mm256_permute2x128_si256(division_table, division_table, 0x11);

  // partial_lo
  const __m256i a = partial_0;
  // partial_hi
  const __m256i b = partial_4;

  // Reverse and clear upper 2 bytes.
  const __m256i reverser = _mm256_broadcastsi128_si256(_mm_set_epi32(
      static_cast<int>(0x80800100), 0x03020504, 0x07060908, 0x0b0a0d0c));

  // 14 13 12 11 10 09 08 ZZ
  const __m256i b_reversed = _mm256_shuffle_epi8(b, reverser);
  // 00 14 01 13 02 12 03 11
  const __m256i ab_lo = _mm256_unpacklo_epi16(a, b_reversed);
  // 04 10 05 09 06 08 07 ZZ
  const __m256i ab_hi = _mm256_unpackhi_epi16(a, b_reversed);

  // Square(partial[0][i]) + Square(partial[0][14 - i])
  const __m256i square_lo = _mm256_madd_epi16(ab_lo, ab_lo);
  const __m256i square_hi = _mm256_madd_epi16(ab_hi, ab_hi);

  const __m256i c = _mm256_mullo_epi32(square_lo, division_table_0);
  const __m256i d = _mm256_mullo_epi32(square_hi, division_table_1);
  const __m256i e = SumVectorPair_S32(_mm256_add_epi32(c, d));
  // Copy upper 32bit sum to lower lane.
  const __m128i sums =
      _mm256_castsi256_si128(_mm256_permute4x64_epi64(e, 0x08));
  cost[0] = _mm_cvtsi128_si32(sums);
  cost[4] = _mm_cvtsi128_si32(_mm_srli_si128(sums, 8));
}

template <int index_a, int index_b>
inline void CostOdd_Pair(uint32_t* cost, const __m256i partial_a,
                         const __m256i partial_b,
                         const __m256i division_table[2]) {
  // partial_lo
  const __m256i a = partial_a;
  // partial_hi
  const __m256i b = partial_b;

  // Reverse and clear upper 10 bytes.
  const __m256i reverser = _mm256_broadcastsi128_si256(
      _mm_set_epi32(static_cast<int>(0x80808080), static_cast<int>(0x80808080),
                    static_cast<int>(0x80800100), 0x03020504));

  // 10 09 08 ZZ ZZ ZZ ZZ ZZ
  const __m256i b_reversed = _mm256_shuffle_epi8(b, reverser);
  // 00 10 01 09 02 08 03 ZZ
  const __m256i ab_lo = _mm256_unpacklo_epi16(a, b_reversed);
  // 04 ZZ 05 ZZ 06 ZZ 07 ZZ
  const __m256i ab_hi = _mm256_unpackhi_epi16(a, b_reversed);

  // Square(partial[0][i]) + Square(partial[0][14 - i])
  const __m256i square_lo = _mm256_madd_epi16(ab_lo, ab_lo);
  const __m256i square_hi = _mm256_madd_epi16(ab_hi, ab_hi);

  const __m256i c = _mm256_mullo_epi32(square_lo, division_table[0]);
  const __m256i d = _mm256_mullo_epi32(square_hi, division_table[1]);
  const __m256i e = SumVectorPair_S32(_mm256_add_epi32(c, d));
  // Copy upper 32bit sum to lower lane.
  const __m128i sums =
      _mm256_castsi256_si128(_mm256_permute4x64_epi64(e, 0x08));
  cost[index_a] = _mm_cvtsi128_si32(sums);
  cost[index_b] = _mm_cvtsi128_si32(_mm_srli_si128(sums, 8));
}

inline void Cost2And6_Pair(uint32_t* cost, const __m256i partial_a,
                           const __m256i partial_b,
                           const __m256i division_table) {
  // The upper lane is a "don't care", so only use the lower lane for
  // calculating cost.
  const __m256i a = _mm256_permute2x128_si256(partial_a, partial_b, 0x20);

  const __m256i square_a = _mm256_madd_epi16(a, a);
  const __m256i b = _mm256_mullo_epi32(square_a, division_table);
  const __m256i c = SumVectorPair_S32(b);
  // Copy upper 32bit sum to lower lane.
  const __m128i sums =
      _mm256_castsi256_si128(_mm256_permute4x64_epi64(c, 0x08));
  cost[2] = _mm_cvtsi128_si32(sums);
  cost[6] = _mm_cvtsi128_si32(_mm_srli_si128(sums, 8));
}

void CdefDirection_AVX2(const void* LIBGAV1_RESTRICT const source,
                        ptrdiff_t stride,
                        uint8_t* LIBGAV1_RESTRICT const direction,
                        int* LIBGAV1_RESTRICT const variance) {
  assert(direction != nullptr);
  assert(variance != nullptr);
  const auto* src = static_cast<const uint8_t*>(source);
  uint32_t cost[8];

  // partial[0] = add partial 0,4 low
  // partial[1] = add partial 1,3 low
  // partial[2] = add partial 2 low
  // partial[3] = add partial 1,3 high
  // partial[4] = add partial 0,4 high
  // partial[5] = add partial 7,5 high
  // partial[6] = add partial 6 low
  // partial[7] = add partial 7,5 low
  __m256i partial[8];

  AddPartial(src, stride, partial);

  const __m256i division_table = LoadUnaligned32(kCdefDivisionTable);
  const __m256i division_table_7 =
      _mm256_broadcastd_epi32(_mm_cvtsi32_si128(kCdefDivisionTable[7]));

  Cost2And6_Pair(cost, partial[2], partial[6], division_table_7);

  Cost0Or4_Pair(cost, partial[0], partial[4], division_table);

  const __m256i division_table_odd[2] = {
      LoadUnaligned32(kCdefDivisionTableOddPairsPadded),
      LoadUnaligned32(kCdefDivisionTableOddPairsPadded + 8)};

  CostOdd_Pair<1, 3>(cost, partial[1], partial[3], division_table_odd);
  CostOdd_Pair<7, 5>(cost, partial[7], partial[5], division_table_odd);

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

inline __m256i Constrain(const __m256i& pixel, const __m256i& reference,
                         const __m128i& damping, const __m256i& threshold) {
  const __m256i diff = _mm256_sub_epi16(pixel, reference);
  const __m256i abs_diff = _mm256_abs_epi16(diff);
  // sign(diff) * Clip3(threshold - (std::abs(diff) >> damping),
  //                    0, std::abs(diff))
  const __m256i shifted_diff = _mm256_srl_epi16(abs_diff, damping);
  // For bitdepth == 8, the threshold range is [0, 15] and the damping range is
  // [3, 6]. If pixel == kCdefLargeValue(0x4000), shifted_diff will always be
  // larger than threshold. Subtract using saturation will return 0 when pixel
  // == kCdefLargeValue.
  static_assert(kCdefLargeValue == 0x4000, "Invalid kCdefLargeValue");
  const __m256i thresh_minus_shifted_diff =
      _mm256_subs_epu16(threshold, shifted_diff);
  const __m256i clamp_abs_diff =
      _mm256_min_epi16(thresh_minus_shifted_diff, abs_diff);
  // Restore the sign.
  return _mm256_sign_epi16(clamp_abs_diff, diff);
}

inline __m256i ApplyConstrainAndTap(const __m256i& pixel, const __m256i& val,
                                    const __m256i& tap, const __m128i& damping,
                                    const __m256i& threshold) {
  const __m256i constrained = Constrain(val, pixel, damping, threshold);
  return _mm256_mullo_epi16(constrained, tap);
}

template <int width, bool enable_primary = true, bool enable_secondary = true>
void CdefFilter_AVX2(const uint16_t* LIBGAV1_RESTRICT src,
                     const ptrdiff_t src_stride, const int height,
                     const int primary_strength, const int secondary_strength,
                     const int damping, const int direction,
                     void* LIBGAV1_RESTRICT dest, const ptrdiff_t dst_stride) {
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
  const __m256i primary_tap_0 = _mm256_broadcastw_epi16(
      _mm_cvtsi32_si128(kCdefPrimaryTaps[primary_strength & 1][0]));
  const __m256i primary_tap_1 = _mm256_broadcastw_epi16(
      _mm_cvtsi32_si128(kCdefPrimaryTaps[primary_strength & 1][1]));
  const __m256i secondary_tap_0 =
      _mm256_broadcastw_epi16(_mm_cvtsi32_si128(kCdefSecondaryTap0));
  const __m256i secondary_tap_1 =
      _mm256_broadcastw_epi16(_mm_cvtsi32_si128(kCdefSecondaryTap1));
  const __m256i cdef_large_value_mask = _mm256_broadcastw_epi16(
      _mm_cvtsi32_si128(static_cast<int16_t>(~kCdefLargeValue)));
  const __m256i primary_threshold =
      _mm256_broadcastw_epi16(_mm_cvtsi32_si128(primary_strength));
  const __m256i secondary_threshold =
      _mm256_broadcastw_epi16(_mm_cvtsi32_si128(secondary_strength));

  int y = height;
  do {
    __m128i pixel_128;
    if (width == 8) {
      pixel_128 = LoadUnaligned16(src);
    } else {
      pixel_128 = LoadHi8(LoadLo8(src), src + src_stride);
    }

    __m256i pixel = SetrM128i(pixel_128, pixel_128);

    __m256i min = pixel;
    __m256i max = pixel;
    __m256i sum_pair;

    if (enable_primary) {
      // Primary |direction|.
      __m128i primary_val_128[4];
      if (width == 8) {
        LoadDirection(src, src_stride, primary_val_128, direction);
      } else {
        LoadDirection4(src, src_stride, primary_val_128, direction);
      }

      __m256i primary_val[2];
      primary_val[0] = SetrM128i(primary_val_128[0], primary_val_128[1]);
      primary_val[1] = SetrM128i(primary_val_128[2], primary_val_128[3]);

      if (clipping_required) {
        min = _mm256_min_epu16(min, primary_val[0]);
        min = _mm256_min_epu16(min, primary_val[1]);

        // The source is 16 bits, however, we only really care about the lower
        // 8 bits.  The upper 8 bits contain the "large" flag.  After the final
        // primary max has been calculated, zero out the upper 8 bits.  Use this
        // to find the "16 bit" max.
        const __m256i max_p01 = _mm256_max_epu8(primary_val[0], primary_val[1]);
        max = _mm256_max_epu16(
            max, _mm256_and_si256(max_p01, cdef_large_value_mask));
      }

      sum_pair = ApplyConstrainAndTap(pixel, primary_val[0], primary_tap_0,
                                      primary_damping_shift, primary_threshold);
      sum_pair = _mm256_add_epi16(
          sum_pair,
          ApplyConstrainAndTap(pixel, primary_val[1], primary_tap_1,
                               primary_damping_shift, primary_threshold));
    } else {
      sum_pair = _mm256_setzero_si256();
    }

    if (enable_secondary) {
      // Secondary |direction| values (+/- 2). Clamp |direction|.
      __m128i secondary_val_128[8];
      if (width == 8) {
        LoadDirection(src, src_stride, secondary_val_128, direction + 2);
        LoadDirection(src, src_stride, secondary_val_128 + 4, direction - 2);
      } else {
        LoadDirection4(src, src_stride, secondary_val_128, direction + 2);
        LoadDirection4(src, src_stride, secondary_val_128 + 4, direction - 2);
      }

      __m256i secondary_val[4];
      secondary_val[0] = SetrM128i(secondary_val_128[0], secondary_val_128[1]);
      secondary_val[1] = SetrM128i(secondary_val_128[2], secondary_val_128[3]);
      secondary_val[2] = SetrM128i(secondary_val_128[4], secondary_val_128[5]);
      secondary_val[3] = SetrM128i(secondary_val_128[6], secondary_val_128[7]);

      if (clipping_required) {
        min = _mm256_min_epu16(min, secondary_val[0]);
        min = _mm256_min_epu16(min, secondary_val[1]);
        min = _mm256_min_epu16(min, secondary_val[2]);
        min = _mm256_min_epu16(min, secondary_val[3]);

        const __m256i max_s01 =
            _mm256_max_epu8(secondary_val[0], secondary_val[1]);
        const __m256i max_s23 =
            _mm256_max_epu8(secondary_val[2], secondary_val[3]);
        const __m256i max_s = _mm256_max_epu8(max_s01, max_s23);
        max = _mm256_max_epu8(max,
                              _mm256_and_si256(max_s, cdef_large_value_mask));
      }

      sum_pair = _mm256_add_epi16(
          sum_pair,
          ApplyConstrainAndTap(pixel, secondary_val[0], secondary_tap_0,
                               secondary_damping_shift, secondary_threshold));
      sum_pair = _mm256_add_epi16(
          sum_pair,
          ApplyConstrainAndTap(pixel, secondary_val[1], secondary_tap_1,
                               secondary_damping_shift, secondary_threshold));
      sum_pair = _mm256_add_epi16(
          sum_pair,
          ApplyConstrainAndTap(pixel, secondary_val[2], secondary_tap_0,
                               secondary_damping_shift, secondary_threshold));
      sum_pair = _mm256_add_epi16(
          sum_pair,
          ApplyConstrainAndTap(pixel, secondary_val[3], secondary_tap_1,
                               secondary_damping_shift, secondary_threshold));
    }

    __m128i sum = _mm_add_epi16(_mm256_castsi256_si128(sum_pair),
                                _mm256_extracti128_si256(sum_pair, 1));

    // Clip3(pixel + ((8 + sum - (sum < 0)) >> 4), min, max))
    const __m128i sum_lt_0 = _mm_srai_epi16(sum, 15);
    // 8 + sum
    sum = _mm_add_epi16(sum, _mm_set1_epi16(8));
    // (... - (sum < 0)) >> 4
    sum = _mm_add_epi16(sum, sum_lt_0);
    sum = _mm_srai_epi16(sum, 4);
    // pixel + ...
    sum = _mm_add_epi16(sum, _mm256_castsi256_si128(pixel));
    if (clipping_required) {
      const __m128i min_128 = _mm_min_epu16(_mm256_castsi256_si128(min),
                                            _mm256_extracti128_si256(min, 1));

      const __m128i max_128 = _mm_max_epu16(_mm256_castsi256_si128(max),
                                            _mm256_extracti128_si256(max, 1));
      // Clip3
      sum = _mm_min_epi16(sum, max_128);
      sum = _mm_max_epi16(sum, min_128);
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
  dsp->cdef_direction = CdefDirection_AVX2;

  dsp->cdef_filters[0][0] = CdefFilter_AVX2<4>;
  dsp->cdef_filters[0][1] =
      CdefFilter_AVX2<4, /*enable_primary=*/true, /*enable_secondary=*/false>;
  dsp->cdef_filters[0][2] = CdefFilter_AVX2<4, /*enable_primary=*/false>;
  dsp->cdef_filters[1][0] = CdefFilter_AVX2<8>;
  dsp->cdef_filters[1][1] =
      CdefFilter_AVX2<8, /*enable_primary=*/true, /*enable_secondary=*/false>;
  dsp->cdef_filters[1][2] = CdefFilter_AVX2<8, /*enable_primary=*/false>;
}

}  // namespace
}  // namespace low_bitdepth

void CdefInit_AVX2() { low_bitdepth::Init8bpp(); }

}  // namespace dsp
}  // namespace libgav1
#else   // !LIBGAV1_TARGETING_AVX2
namespace libgav1 {
namespace dsp {

void CdefInit_AVX2() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_TARGETING_AVX2
