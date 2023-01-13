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

#include "src/utils/entropy_decoder.h"

#include <cassert>
#include <cstring>

#include "src/utils/common.h"
#include "src/utils/compiler_attributes.h"
#include "src/utils/constants.h"
#include "src/utils/cpu.h"

#if defined(__ARM_NEON__) || defined(__aarch64__) || \
    (defined(_MSC_VER) && defined(_M_ARM))
#define LIBGAV1_ENTROPY_DECODER_ENABLE_NEON 1
#else
#define LIBGAV1_ENTROPY_DECODER_ENABLE_NEON 0
#endif

#if LIBGAV1_ENTROPY_DECODER_ENABLE_NEON
#include <arm_neon.h>
#endif

#if defined(__SSE2__) || defined(LIBGAV1_X86_MSVC)
#define LIBGAV1_ENTROPY_DECODER_ENABLE_SSE2 1
#else
#define LIBGAV1_ENTROPY_DECODER_ENABLE_SSE2 0
#endif

#if LIBGAV1_ENTROPY_DECODER_ENABLE_SSE2
#include <emmintrin.h>
#endif

namespace libgav1 {
namespace {

constexpr uint32_t kReadBitMask = ~255;
constexpr int kCdfPrecision = 6;
constexpr int kMinimumProbabilityPerSymbol = 4;

// This function computes the "cur" variable as specified inside the do-while
// loop in Section 8.2.6 of the spec. This function is monotonically
// decreasing as the values of index increases (note that the |cdf| array is
// sorted in decreasing order).
uint32_t ScaleCdf(uint32_t values_in_range_shifted, const uint16_t* const cdf,
                  int index, int symbol_count) {
  return ((values_in_range_shifted * (cdf[index] >> kCdfPrecision)) >> 1) +
         (kMinimumProbabilityPerSymbol * (symbol_count - index));
}

void UpdateCdf(uint16_t* LIBGAV1_RESTRICT const cdf, const int symbol_count,
               const int symbol) {
  const uint16_t count = cdf[symbol_count];
  // rate is computed in the spec as:
  //  3 + ( cdf[N] > 15 ) + ( cdf[N] > 31 ) + Min(FloorLog2(N), 2)
  // In this case cdf[N] is |count|.
  // Min(FloorLog2(N), 2) is 1 for symbol_count == {2, 3} and 2 for all
  // symbol_count > 3. So the equation becomes:
  //  4 + (count > 15) + (count > 31) + (symbol_count > 3).
  // Note that the largest value for count is 32 (it is not incremented beyond
  // 32). So using that information:
  //  count >> 4 is 0 for count from 0 to 15.
  //  count >> 4 is 1 for count from 16 to 31.
  //  count >> 4 is 2 for count == 31.
  // Now, the equation becomes:
  //  4 + (count >> 4) + (symbol_count > 3).
  // Since (count >> 4) can only be 0 or 1 or 2, the addition could be replaced
  // with bitwise or:
  //  (4 | (count >> 4)) + (symbol_count > 3).
  // but using addition will allow the compiler to eliminate an operation when
  // symbol_count is known and this function is inlined.
  const int rate = (count >> 4) + 4 + static_cast<int>(symbol_count > 3);
  // Hints for further optimizations:
  //
  // 1. clang can vectorize this for loop with width 4, even though the loop
  // contains an if-else statement. Therefore, it may be advantageous to use
  // "i < symbol_count" as the loop condition when symbol_count is 8, 12, or 16
  // (a multiple of 4 that's not too small).
  //
  // 2. The for loop can be rewritten in the following form, which would enable
  // clang to vectorize the loop with width 8:
  //
  //   const int rounding = (1 << rate) - 1;
  //   for (int i = 0; i < symbol_count - 1; ++i) {
  //     const uint16_t a = (i < symbol) ? kCdfMaxProbability : rounding;
  //     cdf[i] += static_cast<int16_t>(a - cdf[i]) >> rate;
  //   }
  //
  // The subtraction (a - cdf[i]) relies on the overflow semantics of unsigned
  // integer arithmetic. The result of the unsigned subtraction is cast to a
  // signed integer and right-shifted. This requires the right shift of a
  // signed integer be an arithmetic shift, which is true for clang, gcc, and
  // Visual C++.
  assert(symbol_count - 1 > 0);
  int i = 0;
  do {
    if (i < symbol) {
      cdf[i] += (kCdfMaxProbability - cdf[i]) >> rate;
    } else {
      cdf[i] -= cdf[i] >> rate;
    }
  } while (++i < symbol_count - 1);
  cdf[symbol_count] += static_cast<uint16_t>(count < 32);
}

// Define the UpdateCdfN functions. UpdateCdfN is a specialized implementation
// of UpdateCdf based on the fact that symbol_count == N. UpdateCdfN uses the
// SIMD instruction sets if available.

#if LIBGAV1_ENTROPY_DECODER_ENABLE_NEON

// The UpdateCdf() method contains the following for loop:
//
//   for (int i = 0; i < symbol_count - 1; ++i) {
//     if (i < symbol) {
//       cdf[i] += (kCdfMaxProbability - cdf[i]) >> rate;
//     } else {
//       cdf[i] -= cdf[i] >> rate;
//     }
//   }
//
// It can be rewritten in the following two forms, which are amenable to SIMD
// implementations:
//
//   const int rounding = (1 << rate) - 1;
//   for (int i = 0; i < symbol_count - 1; ++i) {
//     const uint16_t a = (i < symbol) ? kCdfMaxProbability : rounding;
//     cdf[i] += static_cast<int16_t>(a - cdf[i]) >> rate;
//   }
//
// or:
//
//   const int rounding = (1 << rate) - 1;
//   for (int i = 0; i < symbol_count - 1; ++i) {
//     const uint16_t a = (i < symbol) ? (kCdfMaxProbability - rounding) : 0;
//     cdf[i] -= static_cast<int16_t>(cdf[i] - a) >> rate;
//   }
//
// The following ARM NEON implementations use a modified version of the first
// form, using the comparison mask and unsigned rollover to avoid the need to
// calculate rounding.
//
// The cdf array has symbol_count + 1 elements. The first symbol_count elements
// are the CDF. The last element is a count that is initialized to 0 and may
// grow up to 32. The for loop in UpdateCdf updates the CDF in the array. Since
// cdf[symbol_count - 1] is always 0, the for loop does not update
// cdf[symbol_count - 1]. However, it would be correct to have the for loop
// update cdf[symbol_count - 1] anyway: since symbol_count - 1 >= symbol, the
// for loop would take the else branch when i is symbol_count - 1:
//      cdf[i] -= cdf[i] >> rate;
// Since cdf[symbol_count - 1] is 0, cdf[symbol_count - 1] would still be 0
// after the update. The ARM NEON implementations take advantage of this in the
// following two cases:
// 1. When symbol_count is 8 or 16, the vectorized code updates the first
//    symbol_count elements in the array.
// 2. When symbol_count is 7, the vectorized code updates all the 8 elements in
//    the cdf array. Since an invalid CDF value is written into cdf[7], the
//    count in cdf[7] needs to be fixed up after the vectorized code.

void UpdateCdf5(uint16_t* LIBGAV1_RESTRICT const cdf, const int symbol) {
  uint16x4_t cdf_vec = vld1_u16(cdf);
  const uint16_t count = cdf[5];
  const int rate = (count >> 4) + 5;
  const uint16x4_t cdf_max_probability = vdup_n_u16(kCdfMaxProbability);
  const uint16x4_t index = vcreate_u16(0x0003000200010000);
  const uint16x4_t symbol_vec = vdup_n_u16(symbol);
  const uint16x4_t mask = vcge_u16(index, symbol_vec);
  // i < symbol: 32768, i >= symbol: 65535.
  const uint16x4_t a = vorr_u16(mask, cdf_max_probability);
  // i < symbol: 32768 - cdf, i >= symbol: 65535 - cdf.
  const int16x4_t diff = vreinterpret_s16_u16(vsub_u16(a, cdf_vec));
  // i < symbol: cdf - 0, i >= symbol: cdf - 65535.
  const uint16x4_t cdf_offset = vsub_u16(cdf_vec, mask);
  const int16x4_t negative_rate = vdup_n_s16(-rate);
  // i < symbol: (32768 - cdf) >> rate, i >= symbol: (65535 (-1) - cdf) >> rate.
  const uint16x4_t delta = vreinterpret_u16_s16(vshl_s16(diff, negative_rate));
  // i < symbol: (cdf - 0) + ((32768 - cdf) >> rate).
  // i >= symbol: (cdf - 65535) + ((65535 - cdf) >> rate).
  cdf_vec = vadd_u16(cdf_offset, delta);
  vst1_u16(cdf, cdf_vec);
  cdf[5] = count + static_cast<uint16_t>(count < 32);
}

// This version works for |symbol_count| = 7, 8, or 9.
// See UpdateCdf5 for implementation details.
template <int symbol_count>
void UpdateCdf7To9(uint16_t* LIBGAV1_RESTRICT const cdf, const int symbol) {
  static_assert(symbol_count >= 7 && symbol_count <= 9, "");
  uint16x8_t cdf_vec = vld1q_u16(cdf);
  const uint16_t count = cdf[symbol_count];
  const int rate = (count >> 4) + 5;
  const uint16x8_t cdf_max_probability = vdupq_n_u16(kCdfMaxProbability);
  const uint16x8_t index = vcombine_u16(vcreate_u16(0x0003000200010000),
                                        vcreate_u16(0x0007000600050004));
  const uint16x8_t symbol_vec = vdupq_n_u16(symbol);
  const uint16x8_t mask = vcgeq_u16(index, symbol_vec);
  const uint16x8_t a = vorrq_u16(mask, cdf_max_probability);
  const int16x8_t diff = vreinterpretq_s16_u16(vsubq_u16(a, cdf_vec));
  const uint16x8_t cdf_offset = vsubq_u16(cdf_vec, mask);
  const int16x8_t negative_rate = vdupq_n_s16(-rate);
  const uint16x8_t delta =
      vreinterpretq_u16_s16(vshlq_s16(diff, negative_rate));
  cdf_vec = vaddq_u16(cdf_offset, delta);
  vst1q_u16(cdf, cdf_vec);
  cdf[symbol_count] = count + static_cast<uint16_t>(count < 32);
}

void UpdateCdf7(uint16_t* const cdf, const int symbol) {
  UpdateCdf7To9<7>(cdf, symbol);
}

void UpdateCdf8(uint16_t* const cdf, const int symbol) {
  UpdateCdf7To9<8>(cdf, symbol);
}

void UpdateCdf9(uint16_t* const cdf, const int symbol) {
  UpdateCdf7To9<9>(cdf, symbol);
}

// See UpdateCdf5 for implementation details.
void UpdateCdf11(uint16_t* LIBGAV1_RESTRICT const cdf, const int symbol) {
  uint16x8_t cdf_vec = vld1q_u16(cdf + 2);
  const uint16_t count = cdf[11];
  cdf[11] = count + static_cast<uint16_t>(count < 32);
  const int rate = (count >> 4) + 5;
  if (symbol > 1) {
    cdf[0] += (kCdfMaxProbability - cdf[0]) >> rate;
    cdf[1] += (kCdfMaxProbability - cdf[1]) >> rate;
    const uint16x8_t cdf_max_probability = vdupq_n_u16(kCdfMaxProbability);
    const uint16x8_t symbol_vec = vdupq_n_u16(symbol);
    const int16x8_t negative_rate = vdupq_n_s16(-rate);
    const uint16x8_t index = vcombine_u16(vcreate_u16(0x0005000400030002),
                                          vcreate_u16(0x0009000800070006));
    const uint16x8_t mask = vcgeq_u16(index, symbol_vec);
    const uint16x8_t a = vorrq_u16(mask, cdf_max_probability);
    const int16x8_t diff = vreinterpretq_s16_u16(vsubq_u16(a, cdf_vec));
    const uint16x8_t cdf_offset = vsubq_u16(cdf_vec, mask);
    const uint16x8_t delta =
        vreinterpretq_u16_s16(vshlq_s16(diff, negative_rate));
    cdf_vec = vaddq_u16(cdf_offset, delta);
    vst1q_u16(cdf + 2, cdf_vec);
  } else {
    if (symbol != 0) {
      cdf[0] += (kCdfMaxProbability - cdf[0]) >> rate;
      cdf[1] -= cdf[1] >> rate;
    } else {
      cdf[0] -= cdf[0] >> rate;
      cdf[1] -= cdf[1] >> rate;
    }
    const int16x8_t negative_rate = vdupq_n_s16(-rate);
    const uint16x8_t delta = vshlq_u16(cdf_vec, negative_rate);
    cdf_vec = vsubq_u16(cdf_vec, delta);
    vst1q_u16(cdf + 2, cdf_vec);
  }
}

// See UpdateCdf5 for implementation details.
void UpdateCdf13(uint16_t* LIBGAV1_RESTRICT const cdf, const int symbol) {
  uint16x8_t cdf_vec0 = vld1q_u16(cdf);
  uint16x8_t cdf_vec1 = vld1q_u16(cdf + 4);
  const uint16_t count = cdf[13];
  const int rate = (count >> 4) + 5;
  const uint16x8_t cdf_max_probability = vdupq_n_u16(kCdfMaxProbability);
  const uint16x8_t symbol_vec = vdupq_n_u16(symbol);
  const int16x8_t negative_rate = vdupq_n_s16(-rate);

  uint16x8_t index = vcombine_u16(vcreate_u16(0x0003000200010000),
                                  vcreate_u16(0x0007000600050004));
  uint16x8_t mask = vcgeq_u16(index, symbol_vec);
  uint16x8_t a = vorrq_u16(mask, cdf_max_probability);
  int16x8_t diff = vreinterpretq_s16_u16(vsubq_u16(a, cdf_vec0));
  uint16x8_t cdf_offset = vsubq_u16(cdf_vec0, mask);
  uint16x8_t delta = vreinterpretq_u16_s16(vshlq_s16(diff, negative_rate));
  cdf_vec0 = vaddq_u16(cdf_offset, delta);
  vst1q_u16(cdf, cdf_vec0);

  index = vcombine_u16(vcreate_u16(0x0007000600050004),
                       vcreate_u16(0x000b000a00090008));
  mask = vcgeq_u16(index, symbol_vec);
  a = vorrq_u16(mask, cdf_max_probability);
  diff = vreinterpretq_s16_u16(vsubq_u16(a, cdf_vec1));
  cdf_offset = vsubq_u16(cdf_vec1, mask);
  delta = vreinterpretq_u16_s16(vshlq_s16(diff, negative_rate));
  cdf_vec1 = vaddq_u16(cdf_offset, delta);
  vst1q_u16(cdf + 4, cdf_vec1);

  cdf[13] = count + static_cast<uint16_t>(count < 32);
}

// See UpdateCdf5 for implementation details.
void UpdateCdf16(uint16_t* LIBGAV1_RESTRICT const cdf, const int symbol) {
  uint16x8_t cdf_vec = vld1q_u16(cdf);
  const uint16_t count = cdf[16];
  const int rate = (count >> 4) + 5;
  const uint16x8_t cdf_max_probability = vdupq_n_u16(kCdfMaxProbability);
  const uint16x8_t symbol_vec = vdupq_n_u16(symbol);
  const int16x8_t negative_rate = vdupq_n_s16(-rate);

  uint16x8_t index = vcombine_u16(vcreate_u16(0x0003000200010000),
                                  vcreate_u16(0x0007000600050004));
  uint16x8_t mask = vcgeq_u16(index, symbol_vec);
  uint16x8_t a = vorrq_u16(mask, cdf_max_probability);
  int16x8_t diff = vreinterpretq_s16_u16(vsubq_u16(a, cdf_vec));
  uint16x8_t cdf_offset = vsubq_u16(cdf_vec, mask);
  uint16x8_t delta = vreinterpretq_u16_s16(vshlq_s16(diff, negative_rate));
  cdf_vec = vaddq_u16(cdf_offset, delta);
  vst1q_u16(cdf, cdf_vec);

  cdf_vec = vld1q_u16(cdf + 8);
  index = vcombine_u16(vcreate_u16(0x000b000a00090008),
                       vcreate_u16(0x000f000e000d000c));
  mask = vcgeq_u16(index, symbol_vec);
  a = vorrq_u16(mask, cdf_max_probability);
  diff = vreinterpretq_s16_u16(vsubq_u16(a, cdf_vec));
  cdf_offset = vsubq_u16(cdf_vec, mask);
  delta = vreinterpretq_u16_s16(vshlq_s16(diff, negative_rate));
  cdf_vec = vaddq_u16(cdf_offset, delta);
  vst1q_u16(cdf + 8, cdf_vec);

  cdf[16] = count + static_cast<uint16_t>(count < 32);
}

#else  // !LIBGAV1_ENTROPY_DECODER_ENABLE_NEON

#if LIBGAV1_ENTROPY_DECODER_ENABLE_SSE2

inline __m128i LoadLo8(const void* a) {
  return _mm_loadl_epi64(static_cast<const __m128i*>(a));
}

inline __m128i LoadUnaligned16(const void* a) {
  return _mm_loadu_si128(static_cast<const __m128i*>(a));
}

inline void StoreLo8(void* a, const __m128i v) {
  _mm_storel_epi64(static_cast<__m128i*>(a), v);
}

inline void StoreUnaligned16(void* a, const __m128i v) {
  _mm_storeu_si128(static_cast<__m128i*>(a), v);
}

void UpdateCdf5(uint16_t* LIBGAV1_RESTRICT const cdf, const int symbol) {
  __m128i cdf_vec = LoadLo8(cdf);
  const uint16_t count = cdf[5];
  const int rate = (count >> 4) + 5;
  const __m128i cdf_max_probability =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(kCdfMaxProbability), 0);
  const __m128i index = _mm_set_epi32(0x0, 0x0, 0x00040003, 0x00020001);
  const __m128i symbol_vec = _mm_shufflelo_epi16(_mm_cvtsi32_si128(symbol), 0);
  // i >= symbol.
  const __m128i mask = _mm_cmpgt_epi16(index, symbol_vec);
  // i < symbol: 32768, i >= symbol: 65535.
  const __m128i a = _mm_or_si128(mask, cdf_max_probability);
  // i < symbol: 32768 - cdf, i >= symbol: 65535 - cdf.
  const __m128i diff = _mm_sub_epi16(a, cdf_vec);
  // i < symbol: cdf - 0, i >= symbol: cdf - 65535.
  const __m128i cdf_offset = _mm_sub_epi16(cdf_vec, mask);
  // i < symbol: (32768 - cdf) >> rate, i >= symbol: (65535 (-1) - cdf) >> rate.
  const __m128i delta = _mm_sra_epi16(diff, _mm_cvtsi32_si128(rate));
  // i < symbol: (cdf - 0) + ((32768 - cdf) >> rate).
  // i >= symbol: (cdf - 65535) + ((65535 - cdf) >> rate).
  cdf_vec = _mm_add_epi16(cdf_offset, delta);
  StoreLo8(cdf, cdf_vec);
  cdf[5] = count + static_cast<uint16_t>(count < 32);
}

// This version works for |symbol_count| = 7, 8, or 9.
// See UpdateCdf5 for implementation details.
template <int symbol_count>
void UpdateCdf7To9(uint16_t* LIBGAV1_RESTRICT const cdf, const int symbol) {
  static_assert(symbol_count >= 7 && symbol_count <= 9, "");
  __m128i cdf_vec = LoadUnaligned16(cdf);
  const uint16_t count = cdf[symbol_count];
  const int rate = (count >> 4) + 5;
  const __m128i cdf_max_probability =
      _mm_set1_epi16(static_cast<int16_t>(kCdfMaxProbability));
  const __m128i index =
      _mm_set_epi32(0x00080007, 0x00060005, 0x00040003, 0x00020001);
  const __m128i symbol_vec = _mm_set1_epi16(static_cast<int16_t>(symbol));
  const __m128i mask = _mm_cmpgt_epi16(index, symbol_vec);
  const __m128i a = _mm_or_si128(mask, cdf_max_probability);
  const __m128i diff = _mm_sub_epi16(a, cdf_vec);
  const __m128i cdf_offset = _mm_sub_epi16(cdf_vec, mask);
  const __m128i delta = _mm_sra_epi16(diff, _mm_cvtsi32_si128(rate));
  cdf_vec = _mm_add_epi16(cdf_offset, delta);
  StoreUnaligned16(cdf, cdf_vec);
  cdf[symbol_count] = count + static_cast<uint16_t>(count < 32);
}

void UpdateCdf7(uint16_t* const cdf, const int symbol) {
  UpdateCdf7To9<7>(cdf, symbol);
}

void UpdateCdf8(uint16_t* const cdf, const int symbol) {
  UpdateCdf7To9<8>(cdf, symbol);
}

void UpdateCdf9(uint16_t* const cdf, const int symbol) {
  UpdateCdf7To9<9>(cdf, symbol);
}

// See UpdateCdf5 for implementation details.
void UpdateCdf11(uint16_t* LIBGAV1_RESTRICT const cdf, const int symbol) {
  __m128i cdf_vec = LoadUnaligned16(cdf + 2);
  const uint16_t count = cdf[11];
  cdf[11] = count + static_cast<uint16_t>(count < 32);
  const int rate = (count >> 4) + 5;
  if (symbol > 1) {
    cdf[0] += (kCdfMaxProbability - cdf[0]) >> rate;
    cdf[1] += (kCdfMaxProbability - cdf[1]) >> rate;
    const __m128i cdf_max_probability =
        _mm_set1_epi16(static_cast<int16_t>(kCdfMaxProbability));
    const __m128i index =
        _mm_set_epi32(0x000a0009, 0x00080007, 0x00060005, 0x00040003);
    const __m128i symbol_vec = _mm_set1_epi16(static_cast<int16_t>(symbol));
    const __m128i mask = _mm_cmpgt_epi16(index, symbol_vec);
    const __m128i a = _mm_or_si128(mask, cdf_max_probability);
    const __m128i diff = _mm_sub_epi16(a, cdf_vec);
    const __m128i cdf_offset = _mm_sub_epi16(cdf_vec, mask);
    const __m128i delta = _mm_sra_epi16(diff, _mm_cvtsi32_si128(rate));
    cdf_vec = _mm_add_epi16(cdf_offset, delta);
    StoreUnaligned16(cdf + 2, cdf_vec);
  } else {
    if (symbol != 0) {
      cdf[0] += (kCdfMaxProbability - cdf[0]) >> rate;
      cdf[1] -= cdf[1] >> rate;
    } else {
      cdf[0] -= cdf[0] >> rate;
      cdf[1] -= cdf[1] >> rate;
    }
    const __m128i delta = _mm_sra_epi16(cdf_vec, _mm_cvtsi32_si128(rate));
    cdf_vec = _mm_sub_epi16(cdf_vec, delta);
    StoreUnaligned16(cdf + 2, cdf_vec);
  }
}

// See UpdateCdf5 for implementation details.
void UpdateCdf13(uint16_t* LIBGAV1_RESTRICT const cdf, const int symbol) {
  __m128i cdf_vec0 = LoadLo8(cdf);
  __m128i cdf_vec1 = LoadUnaligned16(cdf + 4);
  const uint16_t count = cdf[13];
  const int rate = (count >> 4) + 5;
  const __m128i cdf_max_probability =
      _mm_set1_epi16(static_cast<int16_t>(kCdfMaxProbability));
  const __m128i symbol_vec = _mm_set1_epi16(static_cast<int16_t>(symbol));

  const __m128i index = _mm_set_epi32(0x0, 0x0, 0x00040003, 0x00020001);
  const __m128i mask = _mm_cmpgt_epi16(index, symbol_vec);
  const __m128i a = _mm_or_si128(mask, cdf_max_probability);
  const __m128i diff = _mm_sub_epi16(a, cdf_vec0);
  const __m128i cdf_offset = _mm_sub_epi16(cdf_vec0, mask);
  const __m128i delta = _mm_sra_epi16(diff, _mm_cvtsi32_si128(rate));
  cdf_vec0 = _mm_add_epi16(cdf_offset, delta);
  StoreLo8(cdf, cdf_vec0);

  const __m128i index1 =
      _mm_set_epi32(0x000c000b, 0x000a0009, 0x00080007, 0x00060005);
  const __m128i mask1 = _mm_cmpgt_epi16(index1, symbol_vec);
  const __m128i a1 = _mm_or_si128(mask1, cdf_max_probability);
  const __m128i diff1 = _mm_sub_epi16(a1, cdf_vec1);
  const __m128i cdf_offset1 = _mm_sub_epi16(cdf_vec1, mask1);
  const __m128i delta1 = _mm_sra_epi16(diff1, _mm_cvtsi32_si128(rate));
  cdf_vec1 = _mm_add_epi16(cdf_offset1, delta1);
  StoreUnaligned16(cdf + 4, cdf_vec1);

  cdf[13] = count + static_cast<uint16_t>(count < 32);
}

void UpdateCdf16(uint16_t* LIBGAV1_RESTRICT const cdf, const int symbol) {
  __m128i cdf_vec0 = LoadUnaligned16(cdf);
  const uint16_t count = cdf[16];
  const int rate = (count >> 4) + 5;
  const __m128i cdf_max_probability =
      _mm_set1_epi16(static_cast<int16_t>(kCdfMaxProbability));
  const __m128i symbol_vec = _mm_set1_epi16(static_cast<int16_t>(symbol));

  const __m128i index =
      _mm_set_epi32(0x00080007, 0x00060005, 0x00040003, 0x00020001);
  const __m128i mask = _mm_cmpgt_epi16(index, symbol_vec);
  const __m128i a = _mm_or_si128(mask, cdf_max_probability);
  const __m128i diff = _mm_sub_epi16(a, cdf_vec0);
  const __m128i cdf_offset = _mm_sub_epi16(cdf_vec0, mask);
  const __m128i delta = _mm_sra_epi16(diff, _mm_cvtsi32_si128(rate));
  cdf_vec0 = _mm_add_epi16(cdf_offset, delta);
  StoreUnaligned16(cdf, cdf_vec0);

  __m128i cdf_vec1 = LoadUnaligned16(cdf + 8);
  const __m128i index1 =
      _mm_set_epi32(0x0010000f, 0x000e000d, 0x000c000b, 0x000a0009);
  const __m128i mask1 = _mm_cmpgt_epi16(index1, symbol_vec);
  const __m128i a1 = _mm_or_si128(mask1, cdf_max_probability);
  const __m128i diff1 = _mm_sub_epi16(a1, cdf_vec1);
  const __m128i cdf_offset1 = _mm_sub_epi16(cdf_vec1, mask1);
  const __m128i delta1 = _mm_sra_epi16(diff1, _mm_cvtsi32_si128(rate));
  cdf_vec1 = _mm_add_epi16(cdf_offset1, delta1);
  StoreUnaligned16(cdf + 8, cdf_vec1);

  cdf[16] = count + static_cast<uint16_t>(count < 32);
}

#else  // !LIBGAV1_ENTROPY_DECODER_ENABLE_SSE2

void UpdateCdf5(uint16_t* const cdf, const int symbol) {
  UpdateCdf(cdf, 5, symbol);
}

void UpdateCdf7(uint16_t* const cdf, const int symbol) {
  UpdateCdf(cdf, 7, symbol);
}

void UpdateCdf8(uint16_t* const cdf, const int symbol) {
  UpdateCdf(cdf, 8, symbol);
}

void UpdateCdf9(uint16_t* const cdf, const int symbol) {
  UpdateCdf(cdf, 9, symbol);
}

void UpdateCdf11(uint16_t* const cdf, const int symbol) {
  UpdateCdf(cdf, 11, symbol);
}

void UpdateCdf13(uint16_t* const cdf, const int symbol) {
  UpdateCdf(cdf, 13, symbol);
}

void UpdateCdf16(uint16_t* const cdf, const int symbol) {
  UpdateCdf(cdf, 16, symbol);
}

#endif  // LIBGAV1_ENTROPY_DECODER_ENABLE_SSE2
#endif  // LIBGAV1_ENTROPY_DECODER_ENABLE_NEON

inline EntropyDecoder::WindowSize HostToBigEndian(
    const EntropyDecoder::WindowSize x) {
  static_assert(sizeof(x) == 4 || sizeof(x) == 8, "");
#if defined(__GNUC__)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  return (sizeof(x) == 8) ? __builtin_bswap64(x) : __builtin_bswap32(x);
#else
  return x;
#endif
#elif defined(_WIN32)
  // Note Windows targets are assumed to be little endian.
  return static_cast<EntropyDecoder::WindowSize>(
      (sizeof(x) == 8) ? _byteswap_uint64(static_cast<unsigned __int64>(x))
                       : _byteswap_ulong(static_cast<unsigned long>(x)));
#else
#error Unknown compiler!
#endif  // defined(__GNUC__)
}

}  // namespace

#if !LIBGAV1_CXX17
constexpr int EntropyDecoder::kWindowSize;  // static.
#endif

EntropyDecoder::EntropyDecoder(const uint8_t* data, size_t size,
                               bool allow_update_cdf)
    : data_(data),
      data_end_(data + size),
      data_memcpy_end_((size >= sizeof(WindowSize))
                           ? data + size - sizeof(WindowSize) + 1
                           : data),
      allow_update_cdf_(allow_update_cdf),
      values_in_range_(kCdfMaxProbability) {
  if (data_ < data_memcpy_end_) {
    // This is a simplified version of PopulateBits() which loads 8 extra bits
    // and skips the unnecessary shifts of value and window_diff_.
    WindowSize value;
    memcpy(&value, data_, sizeof(value));
    data_ += sizeof(value);
    window_diff_ = HostToBigEndian(value) ^ -1;
    // Note the initial value of bits_ is larger than kMaxCachedBits as it's
    // used to restore the most significant 0 bit that would be present after
    // PopulateBits() when we extract the first symbol value.
    // As shown in Section 8.2.2 Initialization process for symbol decoder,
    // which uses a fixed offset to read the symbol values, the most
    // significant bit is always 0:
    //   The variable numBits is set equal to Min( sz * 8, 15).
    //   The variable buf is read using the f(numBits) parsing process.
    //   The variable paddedBuf is set equal to ( buf << (15 - numBits) ).
    //   The variable SymbolValue is set to ((1 << 15) - 1) ^ paddedBuf.
    bits_ = kWindowSize - 15;
    return;
  }
  window_diff_ = 0;
  bits_ = -15;
  PopulateBits();
}

// This is similar to the ReadSymbol() implementation but it is optimized based
// on the following facts:
//   * The probability is fixed at half. So some multiplications can be replaced
//     with bit operations.
//   * Symbol count is fixed at 2.
int EntropyDecoder::ReadBit() {
  const uint32_t curr =
      ((values_in_range_ & kReadBitMask) >> 1) + kMinimumProbabilityPerSymbol;
  const auto symbol_value = static_cast<uint16_t>(window_diff_ >> bits_);
  int bit = 1;
  if (symbol_value >= curr) {
    values_in_range_ -= curr;
    window_diff_ -= static_cast<WindowSize>(curr) << bits_;
    bit = 0;
  } else {
    values_in_range_ = curr;
  }
  NormalizeRange();
  return bit;
}

int64_t EntropyDecoder::ReadLiteral(int num_bits) {
  assert(num_bits <= 32);
  assert(num_bits > 0);
  uint32_t literal = 0;
  int bit = num_bits - 1;
  do {
    // ARM can combine a shift operation with a constant number of bits with
    // some other operations, such as the OR operation.
    // Here is an ARM disassembly example:
    // orr w1, w0, w1, lsl #1
    // which left shifts register w1 by 1 bit and OR the shift result with
    // register w0.
    // The next 2 lines are equivalent to:
    // literal |= static_cast<uint32_t>(ReadBit()) << bit;
    literal <<= 1;
    literal |= static_cast<uint32_t>(ReadBit());
  } while (--bit >= 0);
  return literal;
}

int EntropyDecoder::ReadSymbol(uint16_t* LIBGAV1_RESTRICT const cdf,
                               int symbol_count) {
  const int symbol = ReadSymbolImpl(cdf, symbol_count);
  if (allow_update_cdf_) {
    UpdateCdf(cdf, symbol_count, symbol);
  }
  return symbol;
}

bool EntropyDecoder::ReadSymbol(uint16_t* LIBGAV1_RESTRICT cdf) {
  assert(cdf[1] == 0);
  const bool symbol = ReadSymbolImpl(cdf[0]) != 0;
  if (allow_update_cdf_) {
    const uint16_t count = cdf[2];
    // rate is computed in the spec as:
    //  3 + ( cdf[N] > 15 ) + ( cdf[N] > 31 ) + Min(FloorLog2(N), 2)
    // In this case N is 2 and cdf[N] is |count|. So the equation becomes:
    //  4 + (count > 15) + (count > 31)
    // Note that the largest value for count is 32 (it is not incremented beyond
    // 32). So using that information:
    //  count >> 4 is 0 for count from 0 to 15.
    //  count >> 4 is 1 for count from 16 to 31.
    //  count >> 4 is 2 for count == 32.
    // Now, the equation becomes:
    //  4 + (count >> 4).
    // Since (count >> 4) can only be 0 or 1 or 2, the addition can be replaced
    // with bitwise or. So the final equation is:
    //  4 | (count >> 4).
    const int rate = 4 | (count >> 4);
    if (symbol) {
      cdf[0] += (kCdfMaxProbability - cdf[0]) >> rate;
    } else {
      cdf[0] -= cdf[0] >> rate;
    }
    cdf[2] += static_cast<uint16_t>(count < 32);
  }
  return symbol;
}

bool EntropyDecoder::ReadSymbolWithoutCdfUpdate(uint16_t cdf) {
  return ReadSymbolImpl(cdf) != 0;
}

template <int symbol_count>
int EntropyDecoder::ReadSymbol(uint16_t* LIBGAV1_RESTRICT const cdf) {
  static_assert(symbol_count >= 3 && symbol_count <= 16, "");
  if (symbol_count == 3 || symbol_count == 4) {
    return ReadSymbol3Or4(cdf, symbol_count);
  }
  int symbol;
  if (symbol_count == 8) {
    symbol = ReadSymbolImpl8(cdf);
  } else if (symbol_count <= 13) {
    symbol = ReadSymbolImpl(cdf, symbol_count);
  } else {
    symbol = ReadSymbolImplBinarySearch(cdf, symbol_count);
  }
  if (allow_update_cdf_) {
    if (symbol_count == 5) {
      UpdateCdf5(cdf, symbol);
    } else if (symbol_count == 7) {
      UpdateCdf7(cdf, symbol);
    } else if (symbol_count == 8) {
      UpdateCdf8(cdf, symbol);
    } else if (symbol_count == 9) {
      UpdateCdf9(cdf, symbol);
    } else if (symbol_count == 11) {
      UpdateCdf11(cdf, symbol);
    } else if (symbol_count == 13) {
      UpdateCdf13(cdf, symbol);
    } else if (symbol_count == 16) {
      UpdateCdf16(cdf, symbol);
    } else {
      UpdateCdf(cdf, symbol_count, symbol);
    }
  }
  return symbol;
}

int EntropyDecoder::ReadSymbolImpl(const uint16_t* LIBGAV1_RESTRICT const cdf,
                                   int symbol_count) {
  assert(cdf[symbol_count - 1] == 0);
  --symbol_count;
  uint32_t curr = values_in_range_;
  int symbol = -1;
  uint32_t prev;
  const auto symbol_value = static_cast<uint16_t>(window_diff_ >> bits_);
  uint32_t delta = kMinimumProbabilityPerSymbol * symbol_count;
  // Search through the |cdf| array to determine where the scaled cdf value and
  // |symbol_value| cross over.
  do {
    prev = curr;
    curr = (((values_in_range_ >> 8) * (cdf[++symbol] >> kCdfPrecision)) >> 1) +
           delta;
    delta -= kMinimumProbabilityPerSymbol;
  } while (symbol_value < curr);
  values_in_range_ = prev - curr;
  window_diff_ -= static_cast<WindowSize>(curr) << bits_;
  NormalizeRange();
  return symbol;
}

int EntropyDecoder::ReadSymbolImplBinarySearch(
    const uint16_t* LIBGAV1_RESTRICT const cdf, int symbol_count) {
  assert(cdf[symbol_count - 1] == 0);
  assert(symbol_count > 1 && symbol_count <= 16);
  --symbol_count;
  const auto symbol_value = static_cast<uint16_t>(window_diff_ >> bits_);
  // Search through the |cdf| array to determine where the scaled cdf value and
  // |symbol_value| cross over. Since the CDFs are sorted, we can use binary
  // search to do this. Let |symbol| be the index of the first |cdf| array
  // entry whose scaled cdf value is less than or equal to |symbol_value|. The
  // binary search maintains the invariant:
  //   low <= symbol <= high + 1
  // and terminates when low == high + 1.
  int low = 0;
  int high = symbol_count - 1;
  // The binary search maintains the invariants that |prev| is the scaled cdf
  // value for low - 1 and |curr| is the scaled cdf value for high + 1. (By
  // convention, the scaled cdf value for -1 is values_in_range_.) When the
  // binary search terminates, |prev| is the scaled cdf value for symbol - 1
  // and |curr| is the scaled cdf value for |symbol|.
  uint32_t prev = values_in_range_;
  uint32_t curr = 0;
  const uint32_t values_in_range_shifted = values_in_range_ >> 8;
  do {
    const int mid = DivideBy2(low + high);
    const uint32_t scaled_cdf =
        ScaleCdf(values_in_range_shifted, cdf, mid, symbol_count);
    if (symbol_value < scaled_cdf) {
      low = mid + 1;
      prev = scaled_cdf;
    } else {
      high = mid - 1;
      curr = scaled_cdf;
    }
  } while (low <= high);
  assert(low == high + 1);
  // At this point, |low| is the symbol that has been decoded.
  values_in_range_ = prev - curr;
  window_diff_ -= static_cast<WindowSize>(curr) << bits_;
  NormalizeRange();
  return low;
}

int EntropyDecoder::ReadSymbolImpl(uint16_t cdf) {
  const auto symbol_value = static_cast<uint16_t>(window_diff_ >> bits_);
  const uint32_t curr =
      (((values_in_range_ >> 8) * (cdf >> kCdfPrecision)) >> 1) +
      kMinimumProbabilityPerSymbol;
  const int symbol = static_cast<int>(symbol_value < curr);
  if (symbol == 1) {
    values_in_range_ = curr;
  } else {
    values_in_range_ -= curr;
    window_diff_ -= static_cast<WindowSize>(curr) << bits_;
  }
  NormalizeRange();
  return symbol;
}

// Equivalent to ReadSymbol(cdf, [3,4]), with the ReadSymbolImpl and UpdateCdf
// calls inlined.
int EntropyDecoder::ReadSymbol3Or4(uint16_t* LIBGAV1_RESTRICT const cdf,
                                   const int symbol_count) {
  assert(cdf[symbol_count - 1] == 0);
  uint32_t curr = values_in_range_;
  uint32_t prev;
  const auto symbol_value = static_cast<uint16_t>(window_diff_ >> bits_);
  uint32_t delta = kMinimumProbabilityPerSymbol * (symbol_count - 1);
  const uint32_t values_in_range_shifted = values_in_range_ >> 8;

  // Search through the |cdf| array to determine where the scaled cdf value and
  // |symbol_value| cross over. If allow_update_cdf_ is true, update the |cdf|
  // array.
  //
  // The original code is:
  //
  //  int symbol = -1;
  //  do {
  //    prev = curr;
  //    curr =
  //        ((values_in_range_shifted * (cdf[++symbol] >> kCdfPrecision)) >> 1)
  //        + delta;
  //    delta -= kMinimumProbabilityPerSymbol;
  //  } while (symbol_value < curr);
  //  if (allow_update_cdf_) {
  //    UpdateCdf(cdf, [3,4], symbol);
  //  }
  //
  // The do-while loop is unrolled with three or four iterations, and the
  // UpdateCdf call is inlined and merged into the iterations.
  int symbol = 0;
  // Iteration 0.
  prev = curr;
  curr =
      ((values_in_range_shifted * (cdf[symbol] >> kCdfPrecision)) >> 1) + delta;
  if (symbol_value >= curr) {
    // symbol == 0.
    if (allow_update_cdf_) {
      // Inlined version of UpdateCdf(cdf, [3,4], /*symbol=*/0).
      const uint16_t count = cdf[symbol_count];
      cdf[symbol_count] += static_cast<uint16_t>(count < 32);
      const int rate = (count >> 4) + 4 + static_cast<int>(symbol_count == 4);
      if (symbol_count == 4) {
#if LIBGAV1_ENTROPY_DECODER_ENABLE_NEON
        // 1. On Motorola Moto G5 Plus (running 32-bit Android 8.1.0), the ARM
        // NEON code is slower. Consider using the C version if __arm__ is
        // defined.
        // 2. The ARM NEON code (compiled for arm64) is slightly slower on
        // Samsung Galaxy S8+ (SM-G955FD).
        uint16x4_t cdf_vec = vld1_u16(cdf);
        const int16x4_t negative_rate = vdup_n_s16(-rate);
        const uint16x4_t delta = vshl_u16(cdf_vec, negative_rate);
        cdf_vec = vsub_u16(cdf_vec, delta);
        vst1_u16(cdf, cdf_vec);
#elif LIBGAV1_ENTROPY_DECODER_ENABLE_SSE2
        __m128i cdf_vec = LoadLo8(cdf);
        const __m128i delta = _mm_sra_epi16(cdf_vec, _mm_cvtsi32_si128(rate));
        cdf_vec = _mm_sub_epi16(cdf_vec, delta);
        StoreLo8(cdf, cdf_vec);
#else  // !LIBGAV1_ENTROPY_DECODER_ENABLE_SSE2
        cdf[0] -= cdf[0] >> rate;
        cdf[1] -= cdf[1] >> rate;
        cdf[2] -= cdf[2] >> rate;
#endif
      } else {  // symbol_count == 3.
        cdf[0] -= cdf[0] >> rate;
        cdf[1] -= cdf[1] >> rate;
      }
    }
    goto found;
  }
  ++symbol;
  delta -= kMinimumProbabilityPerSymbol;
  // Iteration 1.
  prev = curr;
  curr =
      ((values_in_range_shifted * (cdf[symbol] >> kCdfPrecision)) >> 1) + delta;
  if (symbol_value >= curr) {
    // symbol == 1.
    if (allow_update_cdf_) {
      // Inlined version of UpdateCdf(cdf, [3,4], /*symbol=*/1).
      const uint16_t count = cdf[symbol_count];
      cdf[symbol_count] += static_cast<uint16_t>(count < 32);
      const int rate = (count >> 4) + 4 + static_cast<int>(symbol_count == 4);
      cdf[0] += (kCdfMaxProbability - cdf[0]) >> rate;
      cdf[1] -= cdf[1] >> rate;
      if (symbol_count == 4) cdf[2] -= cdf[2] >> rate;
    }
    goto found;
  }
  ++symbol;
  if (symbol_count == 4) {
    delta -= kMinimumProbabilityPerSymbol;
    // Iteration 2.
    prev = curr;
    curr = ((values_in_range_shifted * (cdf[symbol] >> kCdfPrecision)) >> 1) +
           delta;
    if (symbol_value >= curr) {
      // symbol == 2.
      if (allow_update_cdf_) {
        // Inlined version of UpdateCdf(cdf, 4, /*symbol=*/2).
        const uint16_t count = cdf[4];
        cdf[4] += static_cast<uint16_t>(count < 32);
        const int rate = (count >> 4) + 5;
        cdf[0] += (kCdfMaxProbability - cdf[0]) >> rate;
        cdf[1] += (kCdfMaxProbability - cdf[1]) >> rate;
        cdf[2] -= cdf[2] >> rate;
      }
      goto found;
    }
    ++symbol;
  }
  // |delta| is 0 for the last iteration.
  // Iteration 2 (symbol_count == 3) or 3 (symbol_count == 4).
  prev = curr;
  // Since cdf[symbol_count - 1] is 0 and |delta| is 0, |curr| is also 0.
  curr = 0;
  // symbol == [2,3].
  if (allow_update_cdf_) {
    // Inlined version of UpdateCdf(cdf, [3,4], /*symbol=*/[2,3]).
    const uint16_t count = cdf[symbol_count];
    cdf[symbol_count] += static_cast<uint16_t>(count < 32);
    const int rate = (4 | (count >> 4)) + static_cast<int>(symbol_count == 4);
    if (symbol_count == 4) {
#if LIBGAV1_ENTROPY_DECODER_ENABLE_NEON
      // On Motorola Moto G5 Plus (running 32-bit Android 8.1.0), the ARM NEON
      // code is a tiny bit slower. Consider using the C version if __arm__ is
      // defined.
      uint16x4_t cdf_vec = vld1_u16(cdf);
      const uint16x4_t cdf_max_probability = vdup_n_u16(kCdfMaxProbability);
      const int16x4_t diff =
          vreinterpret_s16_u16(vsub_u16(cdf_max_probability, cdf_vec));
      const int16x4_t negative_rate = vdup_n_s16(-rate);
      const uint16x4_t delta =
          vreinterpret_u16_s16(vshl_s16(diff, negative_rate));
      cdf_vec = vadd_u16(cdf_vec, delta);
      vst1_u16(cdf, cdf_vec);
      cdf[3] = 0;
#elif LIBGAV1_ENTROPY_DECODER_ENABLE_SSE2
      __m128i cdf_vec = LoadLo8(cdf);
      const __m128i cdf_max_probability =
          _mm_shufflelo_epi16(_mm_cvtsi32_si128(kCdfMaxProbability), 0);
      const __m128i diff = _mm_sub_epi16(cdf_max_probability, cdf_vec);
      const __m128i delta = _mm_sra_epi16(diff, _mm_cvtsi32_si128(rate));
      cdf_vec = _mm_add_epi16(cdf_vec, delta);
      StoreLo8(cdf, cdf_vec);
      cdf[3] = 0;
#else  // !LIBGAV1_ENTROPY_DECODER_ENABLE_SSE2
      cdf[0] += (kCdfMaxProbability - cdf[0]) >> rate;
      cdf[1] += (kCdfMaxProbability - cdf[1]) >> rate;
      cdf[2] += (kCdfMaxProbability - cdf[2]) >> rate;
#endif
    } else {  // symbol_count == 3.
      cdf[0] += (kCdfMaxProbability - cdf[0]) >> rate;
      cdf[1] += (kCdfMaxProbability - cdf[1]) >> rate;
    }
  }
found:
  // End of unrolled do-while loop.

  values_in_range_ = prev - curr;
  window_diff_ -= static_cast<WindowSize>(curr) << bits_;
  NormalizeRange();
  return symbol;
}

int EntropyDecoder::ReadSymbolImpl8(
    const uint16_t* LIBGAV1_RESTRICT const cdf) {
  assert(cdf[7] == 0);
  uint32_t curr = values_in_range_;
  uint32_t prev;
  const auto symbol_value = static_cast<uint16_t>(window_diff_ >> bits_);
  uint32_t delta = kMinimumProbabilityPerSymbol * 7;
  // Search through the |cdf| array to determine where the scaled cdf value and
  // |symbol_value| cross over.
  //
  // The original code is:
  //
  // int symbol = -1;
  // do {
  //   prev = curr;
  //   curr =
  //       (((values_in_range_ >> 8) * (cdf[++symbol] >> kCdfPrecision)) >> 1)
  //       + delta;
  //   delta -= kMinimumProbabilityPerSymbol;
  // } while (symbol_value < curr);
  //
  // The do-while loop is unrolled with eight iterations.
  int symbol = 0;

#define READ_SYMBOL_ITERATION                                                \
  prev = curr;                                                               \
  curr = (((values_in_range_ >> 8) * (cdf[symbol] >> kCdfPrecision)) >> 1) + \
         delta;                                                              \
  if (symbol_value >= curr) goto found;                                      \
  ++symbol;                                                                  \
  delta -= kMinimumProbabilityPerSymbol

  READ_SYMBOL_ITERATION;  // Iteration 0.
  READ_SYMBOL_ITERATION;  // Iteration 1.
  READ_SYMBOL_ITERATION;  // Iteration 2.
  READ_SYMBOL_ITERATION;  // Iteration 3.
  READ_SYMBOL_ITERATION;  // Iteration 4.
  READ_SYMBOL_ITERATION;  // Iteration 5.

  // The last two iterations can be simplified, so they don't use the
  // READ_SYMBOL_ITERATION macro.
#undef READ_SYMBOL_ITERATION

  // Iteration 6.
  prev = curr;
  curr =
      (((values_in_range_ >> 8) * (cdf[symbol] >> kCdfPrecision)) >> 1) + delta;
  if (symbol_value >= curr) goto found;  // symbol == 6.
  ++symbol;
  // |delta| is 0 for the last iteration.
  // Iteration 7.
  prev = curr;
  // Since cdf[7] is 0 and |delta| is 0, |curr| is also 0.
  curr = 0;
  // symbol == 7.
found:
  // End of unrolled do-while loop.

  values_in_range_ = prev - curr;
  window_diff_ -= static_cast<WindowSize>(curr) << bits_;
  NormalizeRange();
  return symbol;
}

void EntropyDecoder::PopulateBits() {
  constexpr int kMaxCachedBits = kWindowSize - 16;
#if defined(__aarch64__)
  // Fast path: read eight bytes and add the first six bytes to window_diff_.
  // This fast path makes the following assumptions.
  // 1. We assume that unaligned load of uint64_t is fast.
  // 2. When there are enough bytes in data_, the for loop below reads 6 or 7
  //    bytes depending on the value of bits_. This fast path always reads 6
  //    bytes, which results in more calls to PopulateBits(). We assume that
  //    making more calls to a faster PopulateBits() is overall a win.
  // NOTE: Although this fast path could also be used on x86_64, it hurts
  // performance (measured on Lenovo ThinkStation P920 running Linux). (The
  // reason is still unknown.) Therefore this fast path is only used on arm64.
  static_assert(kWindowSize == 64, "");
  if (data_ < data_memcpy_end_) {
    uint64_t value;
    // arm64 supports unaligned loads, so this memcpy call is compiled to a
    // single ldr instruction.
    memcpy(&value, data_, sizeof(value));
    data_ += kMaxCachedBits >> 3;
    value = HostToBigEndian(value) ^ -1;
    value >>= kWindowSize - kMaxCachedBits;
    window_diff_ = value | (window_diff_ << kMaxCachedBits);
    bits_ += kMaxCachedBits;
    return;
  }
#endif

  const uint8_t* data = data_;
  int bits = bits_;
  WindowSize window_diff = window_diff_;

  int count = kWindowSize - 9 - (bits + 15);
  // The fast path above, if compiled, would cause clang 8.0.7 to vectorize
  // this loop. Since -15 <= bits_ <= -1, this loop has at most 6 or 7
  // iterations when WindowSize is 64 bits. So it is not profitable to
  // vectorize this loop. Note that clang 8.0.7 does not vectorize this loop if
  // the fast path above is not compiled.

#ifdef __clang__
#pragma clang loop vectorize(disable) interleave(disable)
#endif
  for (; count >= 0 && data < data_end_; count -= 8) {
    const uint8_t value = *data++ ^ -1;
    window_diff = static_cast<WindowSize>(value) | (window_diff << 8);
    bits += 8;
  }
  assert(bits <= kMaxCachedBits);
  if (data == data_end_) {
    // Shift in some 1s. This is equivalent to providing fake 0 data bits.
    window_diff = ((window_diff + 1) << (kMaxCachedBits - bits)) - 1;
    bits = kMaxCachedBits;
  }

  data_ = data;
  bits_ = bits;
  window_diff_ = window_diff;
}

void EntropyDecoder::NormalizeRange() {
  const int bits_used = 15 ^ FloorLog2(values_in_range_);
  bits_ -= bits_used;
  values_in_range_ <<= bits_used;
  if (bits_ < 0) PopulateBits();
}

// Explicit instantiations.
template int EntropyDecoder::ReadSymbol<3>(uint16_t* cdf);
template int EntropyDecoder::ReadSymbol<4>(uint16_t* cdf);
template int EntropyDecoder::ReadSymbol<5>(uint16_t* cdf);
template int EntropyDecoder::ReadSymbol<6>(uint16_t* cdf);
template int EntropyDecoder::ReadSymbol<7>(uint16_t* cdf);
template int EntropyDecoder::ReadSymbol<8>(uint16_t* cdf);
template int EntropyDecoder::ReadSymbol<9>(uint16_t* cdf);
template int EntropyDecoder::ReadSymbol<10>(uint16_t* cdf);
template int EntropyDecoder::ReadSymbol<11>(uint16_t* cdf);
template int EntropyDecoder::ReadSymbol<12>(uint16_t* cdf);
template int EntropyDecoder::ReadSymbol<13>(uint16_t* cdf);
template int EntropyDecoder::ReadSymbol<14>(uint16_t* cdf);
template int EntropyDecoder::ReadSymbol<16>(uint16_t* cdf);

}  // namespace libgav1
