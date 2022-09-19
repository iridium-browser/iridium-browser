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

#include "src/dsp/intrapred.h"
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
// Utility Functions

// This is a fast way to divide by a number of the form 2^n + 2^k, n > k.
// Divide by 2^k by right shifting by k, leaving the denominator 2^m + 1. In the
// block size cases, n - k is 1 or 2 (block is proportional to 1x2 or 1x4), so
// we use a multiplier that reflects division by 2+1=3 or 4+1=5 in the high
// bits.
constexpr int kThreeInverse = 0x5556;
constexpr int kFiveInverse = 0x3334;
template <int shiftk, int multiplier>
inline __m128i DivideByMultiplyShift_U32(const __m128i dividend) {
  const __m128i interm = _mm_srli_epi32(dividend, shiftk);
  return _mm_mulhi_epi16(interm, _mm_cvtsi32_si128(multiplier));
}

//------------------------------------------------------------------------------
// DcPredFuncs_SSE4_1

using DcSumFunc = __m128i (*)(const void* ref);
using DcStoreFunc = void (*)(void* dest, ptrdiff_t stride, const __m128i dc);
using WriteDuplicateFunc = void (*)(void* dest, ptrdiff_t stride,
                                    const __m128i column);
// For copying an entire column across a block.
using ColumnStoreFunc = void (*)(void* dest, ptrdiff_t stride,
                                 const void* column);

// DC intra-predictors for non-square blocks.
template <int width_log2, int height_log2, DcSumFunc top_sumfn,
          DcSumFunc left_sumfn, DcStoreFunc storefn, int shiftk, int dc_mult>
struct DcPredFuncs_SSE4_1 {
  DcPredFuncs_SSE4_1() = delete;

  static void DcTop(void* dest, ptrdiff_t stride, const void* top_row,
                    const void* left_column);
  static void DcLeft(void* dest, ptrdiff_t stride, const void* top_row,
                     const void* left_column);
  static void Dc(void* dest, ptrdiff_t stride, const void* top_row,
                 const void* left_column);
};

// Directional intra-predictors for square blocks.
template <ColumnStoreFunc col_storefn>
struct DirectionalPredFuncs_SSE4_1 {
  DirectionalPredFuncs_SSE4_1() = delete;

  static void Vertical(void* dest, ptrdiff_t stride, const void* top_row,
                       const void* left_column);
  static void Horizontal(void* dest, ptrdiff_t stride, const void* top_row,
                         const void* left_column);
};

template <int width_log2, int height_log2, DcSumFunc top_sumfn,
          DcSumFunc left_sumfn, DcStoreFunc storefn, int shiftk, int dc_mult>
void DcPredFuncs_SSE4_1<
    width_log2, height_log2, top_sumfn, left_sumfn, storefn, shiftk,
    dc_mult>::DcTop(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                    const void* LIBGAV1_RESTRICT const top_row,
                    const void* /*left_column*/) {
  const __m128i rounder = _mm_set1_epi32(1 << (width_log2 - 1));
  const __m128i sum = top_sumfn(top_row);
  const __m128i dc = _mm_srli_epi32(_mm_add_epi32(sum, rounder), width_log2);
  storefn(dest, stride, dc);
}

template <int width_log2, int height_log2, DcSumFunc top_sumfn,
          DcSumFunc left_sumfn, DcStoreFunc storefn, int shiftk, int dc_mult>
void DcPredFuncs_SSE4_1<
    width_log2, height_log2, top_sumfn, left_sumfn, storefn, shiftk,
    dc_mult>::DcLeft(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                     const void* /*top_row*/,
                     const void* LIBGAV1_RESTRICT const left_column) {
  const __m128i rounder = _mm_set1_epi32(1 << (height_log2 - 1));
  const __m128i sum = left_sumfn(left_column);
  const __m128i dc = _mm_srli_epi32(_mm_add_epi32(sum, rounder), height_log2);
  storefn(dest, stride, dc);
}

template <int width_log2, int height_log2, DcSumFunc top_sumfn,
          DcSumFunc left_sumfn, DcStoreFunc storefn, int shiftk, int dc_mult>
void DcPredFuncs_SSE4_1<
    width_log2, height_log2, top_sumfn, left_sumfn, storefn, shiftk,
    dc_mult>::Dc(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                 const void* LIBGAV1_RESTRICT const top_row,
                 const void* LIBGAV1_RESTRICT const left_column) {
  const __m128i rounder =
      _mm_set1_epi32((1 << (width_log2 - 1)) + (1 << (height_log2 - 1)));
  const __m128i sum_top = top_sumfn(top_row);
  const __m128i sum_left = left_sumfn(left_column);
  const __m128i sum = _mm_add_epi32(sum_top, sum_left);
  if (width_log2 == height_log2) {
    const __m128i dc =
        _mm_srli_epi32(_mm_add_epi32(sum, rounder), width_log2 + 1);
    storefn(dest, stride, dc);
  } else {
    const __m128i dc =
        DivideByMultiplyShift_U32<shiftk, dc_mult>(_mm_add_epi32(sum, rounder));
    storefn(dest, stride, dc);
  }
}

//------------------------------------------------------------------------------
// DcPredFuncs_SSE4_1 directional predictors

template <ColumnStoreFunc col_storefn>
void DirectionalPredFuncs_SSE4_1<col_storefn>::Horizontal(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* /*top_row*/, const void* LIBGAV1_RESTRICT const left_column) {
  col_storefn(dest, stride, left_column);
}

}  // namespace

//------------------------------------------------------------------------------
namespace low_bitdepth {
namespace {

// |ref| points to 4 bytes containing 4 packed ints.
inline __m128i DcSum4_SSE4_1(const void* const ref) {
  const __m128i vals = Load4(ref);
  const __m128i zero = _mm_setzero_si128();
  return _mm_sad_epu8(vals, zero);
}

inline __m128i DcSum8_SSE4_1(const void* const ref) {
  const __m128i vals = LoadLo8(ref);
  const __m128i zero = _mm_setzero_si128();
  return _mm_sad_epu8(vals, zero);
}

inline __m128i DcSum16_SSE4_1(const void* const ref) {
  const __m128i zero = _mm_setzero_si128();
  const __m128i vals = LoadUnaligned16(ref);
  const __m128i partial_sum = _mm_sad_epu8(vals, zero);
  return _mm_add_epi16(partial_sum, _mm_srli_si128(partial_sum, 8));
}

inline __m128i DcSum32_SSE4_1(const void* const ref) {
  const __m128i zero = _mm_setzero_si128();
  const __m128i vals1 = LoadUnaligned16(ref);
  const __m128i vals2 = LoadUnaligned16(static_cast<const uint8_t*>(ref) + 16);
  const __m128i partial_sum1 = _mm_sad_epu8(vals1, zero);
  const __m128i partial_sum2 = _mm_sad_epu8(vals2, zero);
  const __m128i partial_sum = _mm_add_epi16(partial_sum1, partial_sum2);
  return _mm_add_epi16(partial_sum, _mm_srli_si128(partial_sum, 8));
}

inline __m128i DcSum64_SSE4_1(const void* const ref) {
  const auto* const ref_ptr = static_cast<const uint8_t*>(ref);
  const __m128i zero = _mm_setzero_si128();
  const __m128i vals1 = LoadUnaligned16(ref_ptr);
  const __m128i vals2 = LoadUnaligned16(ref_ptr + 16);
  const __m128i vals3 = LoadUnaligned16(ref_ptr + 32);
  const __m128i vals4 = LoadUnaligned16(ref_ptr + 48);
  const __m128i partial_sum1 = _mm_sad_epu8(vals1, zero);
  const __m128i partial_sum2 = _mm_sad_epu8(vals2, zero);
  __m128i partial_sum = _mm_add_epi16(partial_sum1, partial_sum2);
  const __m128i partial_sum3 = _mm_sad_epu8(vals3, zero);
  partial_sum = _mm_add_epi16(partial_sum, partial_sum3);
  const __m128i partial_sum4 = _mm_sad_epu8(vals4, zero);
  partial_sum = _mm_add_epi16(partial_sum, partial_sum4);
  return _mm_add_epi16(partial_sum, _mm_srli_si128(partial_sum, 8));
}

template <int height>
inline void DcStore4xH_SSE4_1(void* const dest, ptrdiff_t stride,
                              const __m128i dc) {
  const __m128i zero = _mm_setzero_si128();
  const __m128i dc_dup = _mm_shuffle_epi8(dc, zero);
  int y = height - 1;
  auto* dst = static_cast<uint8_t*>(dest);
  do {
    Store4(dst, dc_dup);
    dst += stride;
  } while (--y != 0);
  Store4(dst, dc_dup);
}

template <int height>
inline void DcStore8xH_SSE4_1(void* const dest, ptrdiff_t stride,
                              const __m128i dc) {
  const __m128i zero = _mm_setzero_si128();
  const __m128i dc_dup = _mm_shuffle_epi8(dc, zero);
  int y = height - 1;
  auto* dst = static_cast<uint8_t*>(dest);
  do {
    StoreLo8(dst, dc_dup);
    dst += stride;
  } while (--y != 0);
  StoreLo8(dst, dc_dup);
}

template <int height>
inline void DcStore16xH_SSE4_1(void* const dest, ptrdiff_t stride,
                               const __m128i dc) {
  const __m128i zero = _mm_setzero_si128();
  const __m128i dc_dup = _mm_shuffle_epi8(dc, zero);
  int y = height - 1;
  auto* dst = static_cast<uint8_t*>(dest);
  do {
    StoreUnaligned16(dst, dc_dup);
    dst += stride;
  } while (--y != 0);
  StoreUnaligned16(dst, dc_dup);
}

template <int height>
inline void DcStore32xH_SSE4_1(void* const dest, ptrdiff_t stride,
                               const __m128i dc) {
  const __m128i zero = _mm_setzero_si128();
  const __m128i dc_dup = _mm_shuffle_epi8(dc, zero);
  int y = height - 1;
  auto* dst = static_cast<uint8_t*>(dest);
  do {
    StoreUnaligned16(dst, dc_dup);
    StoreUnaligned16(dst + 16, dc_dup);
    dst += stride;
  } while (--y != 0);
  StoreUnaligned16(dst, dc_dup);
  StoreUnaligned16(dst + 16, dc_dup);
}

template <int height>
inline void DcStore64xH_SSE4_1(void* const dest, ptrdiff_t stride,
                               const __m128i dc) {
  const __m128i zero = _mm_setzero_si128();
  const __m128i dc_dup = _mm_shuffle_epi8(dc, zero);
  int y = height - 1;
  auto* dst = static_cast<uint8_t*>(dest);
  do {
    StoreUnaligned16(dst, dc_dup);
    StoreUnaligned16(dst + 16, dc_dup);
    StoreUnaligned16(dst + 32, dc_dup);
    StoreUnaligned16(dst + 48, dc_dup);
    dst += stride;
  } while (--y != 0);
  StoreUnaligned16(dst, dc_dup);
  StoreUnaligned16(dst + 16, dc_dup);
  StoreUnaligned16(dst + 32, dc_dup);
  StoreUnaligned16(dst + 48, dc_dup);
}

// WriteDuplicateN assumes dup has 4 sets of 4 identical bytes that are meant to
// be copied for width N into dest.
inline void WriteDuplicate4x4(void* const dest, ptrdiff_t stride,
                              const __m128i dup32) {
  auto* dst = static_cast<uint8_t*>(dest);
  Store4(dst, dup32);
  dst += stride;
  const int row1 = _mm_extract_epi32(dup32, 1);
  memcpy(dst, &row1, 4);
  dst += stride;
  const int row2 = _mm_extract_epi32(dup32, 2);
  memcpy(dst, &row2, 4);
  dst += stride;
  const int row3 = _mm_extract_epi32(dup32, 3);
  memcpy(dst, &row3, 4);
}

inline void WriteDuplicate8x4(void* const dest, ptrdiff_t stride,
                              const __m128i dup32) {
  const __m128i dup64_lo = _mm_unpacklo_epi32(dup32, dup32);
  const __m128i dup64_hi = _mm_unpackhi_epi32(dup32, dup32);
  auto* dst = static_cast<uint8_t*>(dest);
  _mm_storel_epi64(reinterpret_cast<__m128i*>(dst), dup64_lo);
  dst += stride;
  _mm_storeh_pi(reinterpret_cast<__m64*>(dst), _mm_castsi128_ps(dup64_lo));
  dst += stride;
  _mm_storel_epi64(reinterpret_cast<__m128i*>(dst), dup64_hi);
  dst += stride;
  _mm_storeh_pi(reinterpret_cast<__m64*>(dst), _mm_castsi128_ps(dup64_hi));
}

inline void WriteDuplicate16x4(void* const dest, ptrdiff_t stride,
                               const __m128i dup32) {
  const __m128i dup64_lo = _mm_unpacklo_epi32(dup32, dup32);
  const __m128i dup64_hi = _mm_unpackhi_epi32(dup32, dup32);

  auto* dst = static_cast<uint8_t*>(dest);
  const __m128i dup128_0 = _mm_unpacklo_epi64(dup64_lo, dup64_lo);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_0);
  dst += stride;
  const __m128i dup128_1 = _mm_unpackhi_epi64(dup64_lo, dup64_lo);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_1);
  dst += stride;
  const __m128i dup128_2 = _mm_unpacklo_epi64(dup64_hi, dup64_hi);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_2);
  dst += stride;
  const __m128i dup128_3 = _mm_unpackhi_epi64(dup64_hi, dup64_hi);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_3);
}

inline void WriteDuplicate32x4(void* const dest, ptrdiff_t stride,
                               const __m128i dup32) {
  const __m128i dup64_lo = _mm_unpacklo_epi32(dup32, dup32);
  const __m128i dup64_hi = _mm_unpackhi_epi32(dup32, dup32);

  auto* dst = static_cast<uint8_t*>(dest);
  const __m128i dup128_0 = _mm_unpacklo_epi64(dup64_lo, dup64_lo);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_0);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 16), dup128_0);
  dst += stride;
  const __m128i dup128_1 = _mm_unpackhi_epi64(dup64_lo, dup64_lo);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_1);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 16), dup128_1);
  dst += stride;
  const __m128i dup128_2 = _mm_unpacklo_epi64(dup64_hi, dup64_hi);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_2);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 16), dup128_2);
  dst += stride;
  const __m128i dup128_3 = _mm_unpackhi_epi64(dup64_hi, dup64_hi);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_3);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 16), dup128_3);
}

inline void WriteDuplicate64x4(void* const dest, ptrdiff_t stride,
                               const __m128i dup32) {
  const __m128i dup64_lo = _mm_unpacklo_epi32(dup32, dup32);
  const __m128i dup64_hi = _mm_unpackhi_epi32(dup32, dup32);

  auto* dst = static_cast<uint8_t*>(dest);
  const __m128i dup128_0 = _mm_unpacklo_epi64(dup64_lo, dup64_lo);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_0);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 16), dup128_0);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 32), dup128_0);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 48), dup128_0);
  dst += stride;
  const __m128i dup128_1 = _mm_unpackhi_epi64(dup64_lo, dup64_lo);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_1);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 16), dup128_1);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 32), dup128_1);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 48), dup128_1);
  dst += stride;
  const __m128i dup128_2 = _mm_unpacklo_epi64(dup64_hi, dup64_hi);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_2);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 16), dup128_2);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 32), dup128_2);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 48), dup128_2);
  dst += stride;
  const __m128i dup128_3 = _mm_unpackhi_epi64(dup64_hi, dup64_hi);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_3);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 16), dup128_3);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 32), dup128_3);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 48), dup128_3);
}

// ColStoreN<height> copies each of the |height| values in |column| across its
// corresponding in dest.
template <WriteDuplicateFunc writefn>
inline void ColStore4_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                             ptrdiff_t stride,
                             const void* LIBGAV1_RESTRICT const column) {
  const __m128i col_data = Load4(column);
  const __m128i col_dup16 = _mm_unpacklo_epi8(col_data, col_data);
  const __m128i col_dup32 = _mm_unpacklo_epi16(col_dup16, col_dup16);
  writefn(dest, stride, col_dup32);
}

template <WriteDuplicateFunc writefn>
inline void ColStore8_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                             ptrdiff_t stride,
                             const void* LIBGAV1_RESTRICT const column) {
  const ptrdiff_t stride4 = stride << 2;
  const __m128i col_data = LoadLo8(column);
  const __m128i col_dup16 = _mm_unpacklo_epi8(col_data, col_data);
  const __m128i col_dup32_lo = _mm_unpacklo_epi16(col_dup16, col_dup16);
  auto* dst = static_cast<uint8_t*>(dest);
  writefn(dst, stride, col_dup32_lo);
  dst += stride4;
  const __m128i col_dup32_hi = _mm_unpackhi_epi16(col_dup16, col_dup16);
  writefn(dst, stride, col_dup32_hi);
}

template <WriteDuplicateFunc writefn>
inline void ColStore16_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                              ptrdiff_t stride,
                              const void* LIBGAV1_RESTRICT const column) {
  const ptrdiff_t stride4 = stride << 2;
  const __m128i col_data = _mm_loadu_si128(static_cast<const __m128i*>(column));
  const __m128i col_dup16_lo = _mm_unpacklo_epi8(col_data, col_data);
  const __m128i col_dup16_hi = _mm_unpackhi_epi8(col_data, col_data);
  const __m128i col_dup32_lolo = _mm_unpacklo_epi16(col_dup16_lo, col_dup16_lo);
  auto* dst = static_cast<uint8_t*>(dest);
  writefn(dst, stride, col_dup32_lolo);
  dst += stride4;
  const __m128i col_dup32_lohi = _mm_unpackhi_epi16(col_dup16_lo, col_dup16_lo);
  writefn(dst, stride, col_dup32_lohi);
  dst += stride4;
  const __m128i col_dup32_hilo = _mm_unpacklo_epi16(col_dup16_hi, col_dup16_hi);
  writefn(dst, stride, col_dup32_hilo);
  dst += stride4;
  const __m128i col_dup32_hihi = _mm_unpackhi_epi16(col_dup16_hi, col_dup16_hi);
  writefn(dst, stride, col_dup32_hihi);
}

template <WriteDuplicateFunc writefn>
inline void ColStore32_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                              ptrdiff_t stride,
                              const void* LIBGAV1_RESTRICT const column) {
  const ptrdiff_t stride4 = stride << 2;
  auto* dst = static_cast<uint8_t*>(dest);
  for (int y = 0; y < 32; y += 16) {
    const __m128i col_data =
        LoadUnaligned16(static_cast<const uint8_t*>(column) + y);
    const __m128i col_dup16_lo = _mm_unpacklo_epi8(col_data, col_data);
    const __m128i col_dup16_hi = _mm_unpackhi_epi8(col_data, col_data);
    const __m128i col_dup32_lolo =
        _mm_unpacklo_epi16(col_dup16_lo, col_dup16_lo);
    writefn(dst, stride, col_dup32_lolo);
    dst += stride4;
    const __m128i col_dup32_lohi =
        _mm_unpackhi_epi16(col_dup16_lo, col_dup16_lo);
    writefn(dst, stride, col_dup32_lohi);
    dst += stride4;
    const __m128i col_dup32_hilo =
        _mm_unpacklo_epi16(col_dup16_hi, col_dup16_hi);
    writefn(dst, stride, col_dup32_hilo);
    dst += stride4;
    const __m128i col_dup32_hihi =
        _mm_unpackhi_epi16(col_dup16_hi, col_dup16_hi);
    writefn(dst, stride, col_dup32_hihi);
    dst += stride4;
  }
}

template <WriteDuplicateFunc writefn>
inline void ColStore64_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                              ptrdiff_t stride,
                              const void* LIBGAV1_RESTRICT const column) {
  const ptrdiff_t stride4 = stride << 2;
  auto* dst = static_cast<uint8_t*>(dest);
  for (int y = 0; y < 64; y += 16) {
    const __m128i col_data =
        LoadUnaligned16(static_cast<const uint8_t*>(column) + y);
    const __m128i col_dup16_lo = _mm_unpacklo_epi8(col_data, col_data);
    const __m128i col_dup16_hi = _mm_unpackhi_epi8(col_data, col_data);
    const __m128i col_dup32_lolo =
        _mm_unpacklo_epi16(col_dup16_lo, col_dup16_lo);
    writefn(dst, stride, col_dup32_lolo);
    dst += stride4;
    const __m128i col_dup32_lohi =
        _mm_unpackhi_epi16(col_dup16_lo, col_dup16_lo);
    writefn(dst, stride, col_dup32_lohi);
    dst += stride4;
    const __m128i col_dup32_hilo =
        _mm_unpacklo_epi16(col_dup16_hi, col_dup16_hi);
    writefn(dst, stride, col_dup32_hilo);
    dst += stride4;
    const __m128i col_dup32_hihi =
        _mm_unpackhi_epi16(col_dup16_hi, col_dup16_hi);
    writefn(dst, stride, col_dup32_hihi);
    dst += stride4;
  }
}

struct DcDefs {
  DcDefs() = delete;

  using _4x4 = DcPredFuncs_SSE4_1<2, 2, DcSum4_SSE4_1, DcSum4_SSE4_1,
                                  DcStore4xH_SSE4_1<4>, 0, 0>;
  // shiftk is the smaller of width_log2 and height_log2.
  // dc_mult corresponds to the ratio of the smaller block size to the larger.
  using _4x8 = DcPredFuncs_SSE4_1<2, 3, DcSum4_SSE4_1, DcSum8_SSE4_1,
                                  DcStore4xH_SSE4_1<8>, 2, kThreeInverse>;
  using _4x16 = DcPredFuncs_SSE4_1<2, 4, DcSum4_SSE4_1, DcSum16_SSE4_1,
                                   DcStore4xH_SSE4_1<16>, 2, kFiveInverse>;

  using _8x4 = DcPredFuncs_SSE4_1<3, 2, DcSum8_SSE4_1, DcSum4_SSE4_1,
                                  DcStore8xH_SSE4_1<4>, 2, kThreeInverse>;
  using _8x8 = DcPredFuncs_SSE4_1<3, 3, DcSum8_SSE4_1, DcSum8_SSE4_1,
                                  DcStore8xH_SSE4_1<8>, 0, 0>;
  using _8x16 = DcPredFuncs_SSE4_1<3, 4, DcSum8_SSE4_1, DcSum16_SSE4_1,
                                   DcStore8xH_SSE4_1<16>, 3, kThreeInverse>;
  using _8x32 = DcPredFuncs_SSE4_1<3, 5, DcSum8_SSE4_1, DcSum32_SSE4_1,
                                   DcStore8xH_SSE4_1<32>, 3, kFiveInverse>;

  using _16x4 = DcPredFuncs_SSE4_1<4, 2, DcSum16_SSE4_1, DcSum4_SSE4_1,
                                   DcStore16xH_SSE4_1<4>, 2, kFiveInverse>;
  using _16x8 = DcPredFuncs_SSE4_1<4, 3, DcSum16_SSE4_1, DcSum8_SSE4_1,
                                   DcStore16xH_SSE4_1<8>, 3, kThreeInverse>;
  using _16x16 = DcPredFuncs_SSE4_1<4, 4, DcSum16_SSE4_1, DcSum16_SSE4_1,
                                    DcStore16xH_SSE4_1<16>, 0, 0>;
  using _16x32 = DcPredFuncs_SSE4_1<4, 5, DcSum16_SSE4_1, DcSum32_SSE4_1,
                                    DcStore16xH_SSE4_1<32>, 4, kThreeInverse>;
  using _16x64 = DcPredFuncs_SSE4_1<4, 6, DcSum16_SSE4_1, DcSum64_SSE4_1,
                                    DcStore16xH_SSE4_1<64>, 4, kFiveInverse>;

  using _32x8 = DcPredFuncs_SSE4_1<5, 3, DcSum32_SSE4_1, DcSum8_SSE4_1,
                                   DcStore32xH_SSE4_1<8>, 3, kFiveInverse>;
  using _32x16 = DcPredFuncs_SSE4_1<5, 4, DcSum32_SSE4_1, DcSum16_SSE4_1,
                                    DcStore32xH_SSE4_1<16>, 4, kThreeInverse>;
  using _32x32 = DcPredFuncs_SSE4_1<5, 5, DcSum32_SSE4_1, DcSum32_SSE4_1,
                                    DcStore32xH_SSE4_1<32>, 0, 0>;
  using _32x64 = DcPredFuncs_SSE4_1<5, 6, DcSum32_SSE4_1, DcSum64_SSE4_1,
                                    DcStore32xH_SSE4_1<64>, 5, kThreeInverse>;

  using _64x16 = DcPredFuncs_SSE4_1<6, 4, DcSum64_SSE4_1, DcSum16_SSE4_1,
                                    DcStore64xH_SSE4_1<16>, 4, kFiveInverse>;
  using _64x32 = DcPredFuncs_SSE4_1<6, 5, DcSum64_SSE4_1, DcSum32_SSE4_1,
                                    DcStore64xH_SSE4_1<32>, 5, kThreeInverse>;
  using _64x64 = DcPredFuncs_SSE4_1<6, 6, DcSum64_SSE4_1, DcSum64_SSE4_1,
                                    DcStore64xH_SSE4_1<64>, 0, 0>;
};

struct DirDefs {
  DirDefs() = delete;

  using _4x4 = DirectionalPredFuncs_SSE4_1<ColStore4_SSE4_1<WriteDuplicate4x4>>;
  using _4x8 = DirectionalPredFuncs_SSE4_1<ColStore8_SSE4_1<WriteDuplicate4x4>>;
  using _4x16 =
      DirectionalPredFuncs_SSE4_1<ColStore16_SSE4_1<WriteDuplicate4x4>>;
  using _8x4 = DirectionalPredFuncs_SSE4_1<ColStore4_SSE4_1<WriteDuplicate8x4>>;
  using _8x8 = DirectionalPredFuncs_SSE4_1<ColStore8_SSE4_1<WriteDuplicate8x4>>;
  using _8x16 =
      DirectionalPredFuncs_SSE4_1<ColStore16_SSE4_1<WriteDuplicate8x4>>;
  using _8x32 =
      DirectionalPredFuncs_SSE4_1<ColStore32_SSE4_1<WriteDuplicate8x4>>;
  using _16x4 =
      DirectionalPredFuncs_SSE4_1<ColStore4_SSE4_1<WriteDuplicate16x4>>;
  using _16x8 =
      DirectionalPredFuncs_SSE4_1<ColStore8_SSE4_1<WriteDuplicate16x4>>;
  using _16x16 =
      DirectionalPredFuncs_SSE4_1<ColStore16_SSE4_1<WriteDuplicate16x4>>;
  using _16x32 =
      DirectionalPredFuncs_SSE4_1<ColStore32_SSE4_1<WriteDuplicate16x4>>;
  using _16x64 =
      DirectionalPredFuncs_SSE4_1<ColStore64_SSE4_1<WriteDuplicate16x4>>;
  using _32x8 =
      DirectionalPredFuncs_SSE4_1<ColStore8_SSE4_1<WriteDuplicate32x4>>;
  using _32x16 =
      DirectionalPredFuncs_SSE4_1<ColStore16_SSE4_1<WriteDuplicate32x4>>;
  using _32x32 =
      DirectionalPredFuncs_SSE4_1<ColStore32_SSE4_1<WriteDuplicate32x4>>;
  using _32x64 =
      DirectionalPredFuncs_SSE4_1<ColStore64_SSE4_1<WriteDuplicate32x4>>;
  using _64x16 =
      DirectionalPredFuncs_SSE4_1<ColStore16_SSE4_1<WriteDuplicate64x4>>;
  using _64x32 =
      DirectionalPredFuncs_SSE4_1<ColStore32_SSE4_1<WriteDuplicate64x4>>;
  using _64x64 =
      DirectionalPredFuncs_SSE4_1<ColStore64_SSE4_1<WriteDuplicate64x4>>;
};

template <int y_mask>
inline void WritePaethLine4(uint8_t* LIBGAV1_RESTRICT dst, const __m128i& top,
                            const __m128i& left, const __m128i& top_lefts,
                            const __m128i& top_dists, const __m128i& left_dists,
                            const __m128i& top_left_diffs) {
  const __m128i top_dists_y = _mm_shuffle_epi32(top_dists, y_mask);

  const __m128i lefts_y = _mm_shuffle_epi32(left, y_mask);
  const __m128i top_left_dists =
      _mm_abs_epi32(_mm_add_epi32(lefts_y, top_left_diffs));

  // Section 7.11.2.2 specifies the logic and terms here. The less-or-equal
  // operation is unavailable, so the logic for selecting top, left, or
  // top_left is inverted.
  __m128i not_select_left = _mm_cmpgt_epi32(left_dists, top_left_dists);
  not_select_left =
      _mm_or_si128(not_select_left, _mm_cmpgt_epi32(left_dists, top_dists_y));
  const __m128i not_select_top = _mm_cmpgt_epi32(top_dists_y, top_left_dists);

  const __m128i left_out = _mm_andnot_si128(not_select_left, lefts_y);

  const __m128i top_left_out = _mm_and_si128(not_select_top, top_lefts);
  __m128i top_or_top_left_out = _mm_andnot_si128(not_select_top, top);
  top_or_top_left_out = _mm_or_si128(top_or_top_left_out, top_left_out);
  top_or_top_left_out = _mm_and_si128(not_select_left, top_or_top_left_out);

  // The sequence of 32-bit packed operations was found (see CL via blame) to
  // outperform 16-bit operations, despite the availability of the packus
  // function, when tested on a Xeon E7 v3.
  const __m128i cvtepi32_epi8 = _mm_set1_epi32(0x0C080400);
  const __m128i pred = _mm_shuffle_epi8(
      _mm_or_si128(left_out, top_or_top_left_out), cvtepi32_epi8);
  Store4(dst, pred);
}

// top_left_diffs is the only variable whose ints may exceed 8 bits. Otherwise
// we would be able to do all of these operations as epi8 for a 16-pixel version
// of this function. Still, since lefts_y is just a vector of duplicates, it
// could pay off to accommodate top_left_dists for cmpgt, and repack into epi8
// for the blends.
template <int y_mask>
inline void WritePaethLine8(uint8_t* LIBGAV1_RESTRICT dst, const __m128i& top,
                            const __m128i& left, const __m128i& top_lefts,
                            const __m128i& top_dists, const __m128i& left_dists,
                            const __m128i& top_left_diffs) {
  const __m128i select_y = _mm_set1_epi32(y_mask);
  const __m128i top_dists_y = _mm_shuffle_epi8(top_dists, select_y);

  const __m128i lefts_y = _mm_shuffle_epi8(left, select_y);
  const __m128i top_left_dists =
      _mm_abs_epi16(_mm_add_epi16(lefts_y, top_left_diffs));

  // Section 7.11.2.2 specifies the logic and terms here. The less-or-equal
  // operation is unavailable, so the logic for selecting top, left, or
  // top_left is inverted.
  __m128i not_select_left = _mm_cmpgt_epi16(left_dists, top_left_dists);
  not_select_left =
      _mm_or_si128(not_select_left, _mm_cmpgt_epi16(left_dists, top_dists_y));
  const __m128i not_select_top = _mm_cmpgt_epi16(top_dists_y, top_left_dists);

  const __m128i left_out = _mm_andnot_si128(not_select_left, lefts_y);

  const __m128i top_left_out = _mm_and_si128(not_select_top, top_lefts);
  __m128i top_or_top_left_out = _mm_andnot_si128(not_select_top, top);
  top_or_top_left_out = _mm_or_si128(top_or_top_left_out, top_left_out);
  top_or_top_left_out = _mm_and_si128(not_select_left, top_or_top_left_out);

  const __m128i pred = _mm_packus_epi16(
      _mm_or_si128(left_out, top_or_top_left_out), /* unused */ left_out);
  _mm_storel_epi64(reinterpret_cast<__m128i*>(dst), pred);
}

// |top| is an epi8 of length 16
// |left| is epi8 of unknown length, as y_mask specifies access
// |top_lefts| is an epi8 of 16 duplicates
// |top_dists| is an epi8 of unknown length, as y_mask specifies access
// |left_dists| is an epi8 of length 16
// |left_dists_lo| is an epi16 of length 8
// |left_dists_hi| is an epi16 of length 8
// |top_left_diffs_lo| is an epi16 of length 8
// |top_left_diffs_hi| is an epi16 of length 8
// The latter two vectors are epi16 because their values may reach -510.
// |left_dists| is provided alongside its spread out version because it doesn't
// change between calls and interacts with both kinds of packing.
template <int y_mask>
inline void WritePaethLine16(uint8_t* LIBGAV1_RESTRICT dst, const __m128i& top,
                             const __m128i& left, const __m128i& top_lefts,
                             const __m128i& top_dists,
                             const __m128i& left_dists,
                             const __m128i& left_dists_lo,
                             const __m128i& left_dists_hi,
                             const __m128i& top_left_diffs_lo,
                             const __m128i& top_left_diffs_hi) {
  const __m128i select_y = _mm_set1_epi32(y_mask);
  const __m128i top_dists_y8 = _mm_shuffle_epi8(top_dists, select_y);
  const __m128i top_dists_y16 = _mm_cvtepu8_epi16(top_dists_y8);
  const __m128i lefts_y8 = _mm_shuffle_epi8(left, select_y);
  const __m128i lefts_y16 = _mm_cvtepu8_epi16(lefts_y8);

  const __m128i top_left_dists_lo =
      _mm_abs_epi16(_mm_add_epi16(lefts_y16, top_left_diffs_lo));
  const __m128i top_left_dists_hi =
      _mm_abs_epi16(_mm_add_epi16(lefts_y16, top_left_diffs_hi));

  const __m128i left_gt_top_left_lo = _mm_packs_epi16(
      _mm_cmpgt_epi16(left_dists_lo, top_left_dists_lo), left_dists_lo);
  const __m128i left_gt_top_left_hi =
      _mm_packs_epi16(_mm_cmpgt_epi16(left_dists_hi, top_left_dists_hi),
                      /* unused second arg for pack */ left_dists_hi);
  const __m128i left_gt_top_left = _mm_alignr_epi8(
      left_gt_top_left_hi, _mm_slli_si128(left_gt_top_left_lo, 8), 8);

  const __m128i not_select_top_lo =
      _mm_packs_epi16(_mm_cmpgt_epi16(top_dists_y16, top_left_dists_lo),
                      /* unused second arg for pack */ top_dists_y16);
  const __m128i not_select_top_hi =
      _mm_packs_epi16(_mm_cmpgt_epi16(top_dists_y16, top_left_dists_hi),
                      /* unused second arg for pack */ top_dists_y16);
  const __m128i not_select_top = _mm_alignr_epi8(
      not_select_top_hi, _mm_slli_si128(not_select_top_lo, 8), 8);

  const __m128i left_leq_top =
      _mm_cmpeq_epi8(left_dists, _mm_min_epu8(top_dists_y8, left_dists));
  const __m128i select_left = _mm_andnot_si128(left_gt_top_left, left_leq_top);

  // Section 7.11.2.2 specifies the logic and terms here. The less-or-equal
  // operation is unavailable, so the logic for selecting top, left, or
  // top_left is inverted.
  const __m128i left_out = _mm_and_si128(select_left, lefts_y8);

  const __m128i top_left_out = _mm_and_si128(not_select_top, top_lefts);
  __m128i top_or_top_left_out = _mm_andnot_si128(not_select_top, top);
  top_or_top_left_out = _mm_or_si128(top_or_top_left_out, top_left_out);
  top_or_top_left_out = _mm_andnot_si128(select_left, top_or_top_left_out);
  const __m128i pred = _mm_or_si128(left_out, top_or_top_left_out);

  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), pred);
}

void Paeth4x4_SSE4_1(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                     const void* LIBGAV1_RESTRICT const top_row,
                     const void* LIBGAV1_RESTRICT const left_column) {
  const __m128i left = _mm_cvtepu8_epi32(Load4(left_column));
  const __m128i top = _mm_cvtepu8_epi32(Load4(top_row));

  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const __m128i top_lefts = _mm_set1_epi32(top_ptr[-1]);

  // Given that the spec defines "base" as top[x] + left[y] - top[-1],
  // pLeft = abs(base - left[y]) = abs(top[x] - top[-1])
  // pTop = abs(base - top[x]) = abs(left[y] - top[-1])
  const __m128i left_dists = _mm_abs_epi32(_mm_sub_epi32(top, top_lefts));
  const __m128i top_dists = _mm_abs_epi32(_mm_sub_epi32(left, top_lefts));

  const __m128i top_left_x2 = _mm_add_epi32(top_lefts, top_lefts);
  const __m128i top_left_diff = _mm_sub_epi32(top, top_left_x2);
  auto* dst = static_cast<uint8_t*>(dest);
  WritePaethLine4<0>(dst, top, left, top_lefts, top_dists, left_dists,
                     top_left_diff);
  dst += stride;
  WritePaethLine4<0x55>(dst, top, left, top_lefts, top_dists, left_dists,
                        top_left_diff);
  dst += stride;
  WritePaethLine4<0xAA>(dst, top, left, top_lefts, top_dists, left_dists,
                        top_left_diff);
  dst += stride;
  WritePaethLine4<0xFF>(dst, top, left, top_lefts, top_dists, left_dists,
                        top_left_diff);
}

void Paeth4x8_SSE4_1(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                     const void* LIBGAV1_RESTRICT const top_row,
                     const void* LIBGAV1_RESTRICT const left_column) {
  const __m128i left = LoadLo8(left_column);
  const __m128i left_lo = _mm_cvtepu8_epi32(left);
  const __m128i left_hi = _mm_cvtepu8_epi32(_mm_srli_si128(left, 4));

  const __m128i top = _mm_cvtepu8_epi32(Load4(top_row));
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const __m128i top_lefts = _mm_set1_epi32(top_ptr[-1]);

  // Given that the spec defines "base" as top[x] + left[y] - top[-1],
  // pLeft = abs(base - left[y]) = abs(top[x] - top[-1])
  // pTop = abs(base - top[x]) = abs(left[y] - top[-1])
  const __m128i left_dists = _mm_abs_epi32(_mm_sub_epi32(top, top_lefts));
  const __m128i top_dists_lo = _mm_abs_epi32(_mm_sub_epi32(left_lo, top_lefts));
  const __m128i top_dists_hi = _mm_abs_epi32(_mm_sub_epi32(left_hi, top_lefts));

  const __m128i top_left_x2 = _mm_add_epi32(top_lefts, top_lefts);
  const __m128i top_left_diff = _mm_sub_epi32(top, top_left_x2);
  auto* dst = static_cast<uint8_t*>(dest);
  WritePaethLine4<0>(dst, top, left_lo, top_lefts, top_dists_lo, left_dists,
                     top_left_diff);
  dst += stride;
  WritePaethLine4<0x55>(dst, top, left_lo, top_lefts, top_dists_lo, left_dists,
                        top_left_diff);
  dst += stride;
  WritePaethLine4<0xAA>(dst, top, left_lo, top_lefts, top_dists_lo, left_dists,
                        top_left_diff);
  dst += stride;
  WritePaethLine4<0xFF>(dst, top, left_lo, top_lefts, top_dists_lo, left_dists,
                        top_left_diff);
  dst += stride;
  WritePaethLine4<0>(dst, top, left_hi, top_lefts, top_dists_hi, left_dists,
                     top_left_diff);
  dst += stride;
  WritePaethLine4<0x55>(dst, top, left_hi, top_lefts, top_dists_hi, left_dists,
                        top_left_diff);
  dst += stride;
  WritePaethLine4<0xAA>(dst, top, left_hi, top_lefts, top_dists_hi, left_dists,
                        top_left_diff);
  dst += stride;
  WritePaethLine4<0xFF>(dst, top, left_hi, top_lefts, top_dists_hi, left_dists,
                        top_left_diff);
}

void Paeth4x16_SSE4_1(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                      const void* LIBGAV1_RESTRICT const top_row,
                      const void* LIBGAV1_RESTRICT const left_column) {
  const __m128i left = LoadUnaligned16(left_column);
  const __m128i left_0 = _mm_cvtepu8_epi32(left);
  const __m128i left_1 = _mm_cvtepu8_epi32(_mm_srli_si128(left, 4));
  const __m128i left_2 = _mm_cvtepu8_epi32(_mm_srli_si128(left, 8));
  const __m128i left_3 = _mm_cvtepu8_epi32(_mm_srli_si128(left, 12));

  const __m128i top = _mm_cvtepu8_epi32(Load4(top_row));
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const __m128i top_lefts = _mm_set1_epi32(top_ptr[-1]);

  // Given that the spec defines "base" as top[x] + left[y] - top[-1],
  // pLeft = abs(base - left[y]) = abs(top[x] - top[-1])
  // pTop = abs(base - top[x]) = abs(left[y] - top[-1])
  const __m128i left_dists = _mm_abs_epi32(_mm_sub_epi32(top, top_lefts));
  const __m128i top_dists_0 = _mm_abs_epi32(_mm_sub_epi32(left_0, top_lefts));
  const __m128i top_dists_1 = _mm_abs_epi32(_mm_sub_epi32(left_1, top_lefts));
  const __m128i top_dists_2 = _mm_abs_epi32(_mm_sub_epi32(left_2, top_lefts));
  const __m128i top_dists_3 = _mm_abs_epi32(_mm_sub_epi32(left_3, top_lefts));

  const __m128i top_left_x2 = _mm_add_epi32(top_lefts, top_lefts);
  const __m128i top_left_diff = _mm_sub_epi32(top, top_left_x2);

  auto* dst = static_cast<uint8_t*>(dest);
  WritePaethLine4<0>(dst, top, left_0, top_lefts, top_dists_0, left_dists,
                     top_left_diff);
  dst += stride;
  WritePaethLine4<0x55>(dst, top, left_0, top_lefts, top_dists_0, left_dists,
                        top_left_diff);
  dst += stride;
  WritePaethLine4<0xAA>(dst, top, left_0, top_lefts, top_dists_0, left_dists,
                        top_left_diff);
  dst += stride;
  WritePaethLine4<0xFF>(dst, top, left_0, top_lefts, top_dists_0, left_dists,
                        top_left_diff);
  dst += stride;
  WritePaethLine4<0>(dst, top, left_1, top_lefts, top_dists_1, left_dists,
                     top_left_diff);
  dst += stride;
  WritePaethLine4<0x55>(dst, top, left_1, top_lefts, top_dists_1, left_dists,
                        top_left_diff);
  dst += stride;
  WritePaethLine4<0xAA>(dst, top, left_1, top_lefts, top_dists_1, left_dists,
                        top_left_diff);
  dst += stride;
  WritePaethLine4<0xFF>(dst, top, left_1, top_lefts, top_dists_1, left_dists,
                        top_left_diff);
  dst += stride;
  WritePaethLine4<0>(dst, top, left_2, top_lefts, top_dists_2, left_dists,
                     top_left_diff);
  dst += stride;
  WritePaethLine4<0x55>(dst, top, left_2, top_lefts, top_dists_2, left_dists,
                        top_left_diff);
  dst += stride;
  WritePaethLine4<0xAA>(dst, top, left_2, top_lefts, top_dists_2, left_dists,
                        top_left_diff);
  dst += stride;
  WritePaethLine4<0xFF>(dst, top, left_2, top_lefts, top_dists_2, left_dists,
                        top_left_diff);
  dst += stride;
  WritePaethLine4<0>(dst, top, left_3, top_lefts, top_dists_3, left_dists,
                     top_left_diff);
  dst += stride;
  WritePaethLine4<0x55>(dst, top, left_3, top_lefts, top_dists_3, left_dists,
                        top_left_diff);
  dst += stride;
  WritePaethLine4<0xAA>(dst, top, left_3, top_lefts, top_dists_3, left_dists,
                        top_left_diff);
  dst += stride;
  WritePaethLine4<0xFF>(dst, top, left_3, top_lefts, top_dists_3, left_dists,
                        top_left_diff);
}

void Paeth8x4_SSE4_1(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                     const void* LIBGAV1_RESTRICT const top_row,
                     const void* LIBGAV1_RESTRICT const left_column) {
  const __m128i left = _mm_cvtepu8_epi16(Load4(left_column));
  const __m128i top = _mm_cvtepu8_epi16(LoadLo8(top_row));
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const __m128i top_lefts = _mm_set1_epi16(top_ptr[-1]);

  // Given that the spec defines "base" as top[x] + left[y] - top[-1],
  // pLeft = abs(base - left[y]) = abs(top[x] - top[-1])
  // pTop = abs(base - top[x]) = abs(left[y] - top[-1])
  const __m128i left_dists = _mm_abs_epi16(_mm_sub_epi16(top, top_lefts));
  const __m128i top_dists = _mm_abs_epi16(_mm_sub_epi16(left, top_lefts));

  const __m128i top_left_x2 = _mm_add_epi16(top_lefts, top_lefts);
  const __m128i top_left_diff = _mm_sub_epi16(top, top_left_x2);
  auto* dst = static_cast<uint8_t*>(dest);
  WritePaethLine8<0x01000100>(dst, top, left, top_lefts, top_dists, left_dists,
                              top_left_diff);
  dst += stride;
  WritePaethLine8<0x03020302>(dst, top, left, top_lefts, top_dists, left_dists,
                              top_left_diff);
  dst += stride;
  WritePaethLine8<0x05040504>(dst, top, left, top_lefts, top_dists, left_dists,
                              top_left_diff);
  dst += stride;
  WritePaethLine8<0x07060706>(dst, top, left, top_lefts, top_dists, left_dists,
                              top_left_diff);
}

void Paeth8x8_SSE4_1(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                     const void* LIBGAV1_RESTRICT const top_row,
                     const void* LIBGAV1_RESTRICT const left_column) {
  const __m128i left = _mm_cvtepu8_epi16(LoadLo8(left_column));
  const __m128i top = _mm_cvtepu8_epi16(LoadLo8(top_row));
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const __m128i top_lefts = _mm_set1_epi16(top_ptr[-1]);

  // Given that the spec defines "base" as top[x] + left[y] - top[-1],
  // pLeft = abs(base - left[y]) = abs(top[x] - top[-1])
  // pTop = abs(base - top[x]) = abs(left[y] - top[-1])
  const __m128i left_dists = _mm_abs_epi16(_mm_sub_epi16(top, top_lefts));
  const __m128i top_dists = _mm_abs_epi16(_mm_sub_epi16(left, top_lefts));

  const __m128i top_left_x2 = _mm_add_epi16(top_lefts, top_lefts);
  const __m128i top_left_diff = _mm_sub_epi16(top, top_left_x2);
  auto* dst = static_cast<uint8_t*>(dest);
  WritePaethLine8<0x01000100>(dst, top, left, top_lefts, top_dists, left_dists,
                              top_left_diff);
  dst += stride;
  WritePaethLine8<0x03020302>(dst, top, left, top_lefts, top_dists, left_dists,
                              top_left_diff);
  dst += stride;
  WritePaethLine8<0x05040504>(dst, top, left, top_lefts, top_dists, left_dists,
                              top_left_diff);
  dst += stride;
  WritePaethLine8<0x07060706>(dst, top, left, top_lefts, top_dists, left_dists,
                              top_left_diff);
  dst += stride;
  WritePaethLine8<0x09080908>(dst, top, left, top_lefts, top_dists, left_dists,
                              top_left_diff);
  dst += stride;
  WritePaethLine8<0x0B0A0B0A>(dst, top, left, top_lefts, top_dists, left_dists,
                              top_left_diff);
  dst += stride;
  WritePaethLine8<0x0D0C0D0C>(dst, top, left, top_lefts, top_dists, left_dists,
                              top_left_diff);
  dst += stride;
  WritePaethLine8<0x0F0E0F0E>(dst, top, left, top_lefts, top_dists, left_dists,
                              top_left_diff);
}

void Paeth8x16_SSE4_1(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                      const void* LIBGAV1_RESTRICT const top_row,
                      const void* LIBGAV1_RESTRICT const left_column) {
  const __m128i left = LoadUnaligned16(left_column);
  const __m128i left_lo = _mm_cvtepu8_epi16(left);
  const __m128i left_hi = _mm_cvtepu8_epi16(_mm_srli_si128(left, 8));
  const __m128i top = _mm_cvtepu8_epi16(LoadLo8(top_row));
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const __m128i top_lefts = _mm_set1_epi16(top_ptr[-1]);

  // Given that the spec defines "base" as top[x] + left[y] - top[-1],
  // pLeft = abs(base - left[y]) = abs(top[x] - top[-1])
  // pTop = abs(base - top[x]) = abs(left[y] - top[-1])
  const __m128i left_dists = _mm_abs_epi16(_mm_sub_epi16(top, top_lefts));
  const __m128i top_dists_lo = _mm_abs_epi16(_mm_sub_epi16(left_lo, top_lefts));
  const __m128i top_dists_hi = _mm_abs_epi16(_mm_sub_epi16(left_hi, top_lefts));

  const __m128i top_left_x2 = _mm_add_epi16(top_lefts, top_lefts);
  const __m128i top_left_diff = _mm_sub_epi16(top, top_left_x2);
  auto* dst = static_cast<uint8_t*>(dest);
  WritePaethLine8<0x01000100>(dst, top, left_lo, top_lefts, top_dists_lo,
                              left_dists, top_left_diff);
  dst += stride;
  WritePaethLine8<0x03020302>(dst, top, left_lo, top_lefts, top_dists_lo,
                              left_dists, top_left_diff);
  dst += stride;
  WritePaethLine8<0x05040504>(dst, top, left_lo, top_lefts, top_dists_lo,
                              left_dists, top_left_diff);
  dst += stride;
  WritePaethLine8<0x07060706>(dst, top, left_lo, top_lefts, top_dists_lo,
                              left_dists, top_left_diff);
  dst += stride;
  WritePaethLine8<0x09080908>(dst, top, left_lo, top_lefts, top_dists_lo,
                              left_dists, top_left_diff);
  dst += stride;
  WritePaethLine8<0x0B0A0B0A>(dst, top, left_lo, top_lefts, top_dists_lo,
                              left_dists, top_left_diff);
  dst += stride;
  WritePaethLine8<0x0D0C0D0C>(dst, top, left_lo, top_lefts, top_dists_lo,
                              left_dists, top_left_diff);
  dst += stride;
  WritePaethLine8<0x0F0E0F0E>(dst, top, left_lo, top_lefts, top_dists_lo,
                              left_dists, top_left_diff);
  dst += stride;
  WritePaethLine8<0x01000100>(dst, top, left_hi, top_lefts, top_dists_hi,
                              left_dists, top_left_diff);
  dst += stride;
  WritePaethLine8<0x03020302>(dst, top, left_hi, top_lefts, top_dists_hi,
                              left_dists, top_left_diff);
  dst += stride;
  WritePaethLine8<0x05040504>(dst, top, left_hi, top_lefts, top_dists_hi,
                              left_dists, top_left_diff);
  dst += stride;
  WritePaethLine8<0x07060706>(dst, top, left_hi, top_lefts, top_dists_hi,
                              left_dists, top_left_diff);
  dst += stride;
  WritePaethLine8<0x09080908>(dst, top, left_hi, top_lefts, top_dists_hi,
                              left_dists, top_left_diff);
  dst += stride;
  WritePaethLine8<0x0B0A0B0A>(dst, top, left_hi, top_lefts, top_dists_hi,
                              left_dists, top_left_diff);
  dst += stride;
  WritePaethLine8<0x0D0C0D0C>(dst, top, left_hi, top_lefts, top_dists_hi,
                              left_dists, top_left_diff);
  dst += stride;
  WritePaethLine8<0x0F0E0F0E>(dst, top, left_hi, top_lefts, top_dists_hi,
                              left_dists, top_left_diff);
}

void Paeth8x32_SSE4_1(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                      const void* LIBGAV1_RESTRICT const top_row,
                      const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  auto* const dst = static_cast<uint8_t*>(dest);
  Paeth8x16_SSE4_1(dst, stride, top_row, left_column);
  Paeth8x16_SSE4_1(dst + (stride << 4), stride, top_row, left_ptr + 16);
}

void Paeth16x4_SSE4_1(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                      const void* LIBGAV1_RESTRICT const top_row,
                      const void* LIBGAV1_RESTRICT const left_column) {
  const __m128i left = Load4(left_column);
  const __m128i top = LoadUnaligned16(top_row);
  const __m128i top_lo = _mm_cvtepu8_epi16(top);
  const __m128i top_hi = _mm_cvtepu8_epi16(_mm_srli_si128(top, 8));

  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const __m128i top_lefts16 = _mm_set1_epi16(top_ptr[-1]);
  const __m128i top_lefts8 = _mm_set1_epi8(static_cast<int8_t>(top_ptr[-1]));

  // Given that the spec defines "base" as top[x] + left[y] - top[-1],
  // pLeft = abs(base - left[y]) = abs(top[x] - top[-1])
  // pTop = abs(base - top[x]) = abs(left[y] - top[-1])

  const __m128i left_dists = _mm_or_si128(_mm_subs_epu8(top, top_lefts8),
                                          _mm_subs_epu8(top_lefts8, top));
  const __m128i left_dists_lo = _mm_cvtepu8_epi16(left_dists);
  const __m128i left_dists_hi =
      _mm_cvtepu8_epi16(_mm_srli_si128(left_dists, 8));
  const __m128i top_dists = _mm_or_si128(_mm_subs_epu8(left, top_lefts8),
                                         _mm_subs_epu8(top_lefts8, left));

  const __m128i top_left_x2 = _mm_add_epi16(top_lefts16, top_lefts16);
  const __m128i top_left_diff_lo = _mm_sub_epi16(top_lo, top_left_x2);
  const __m128i top_left_diff_hi = _mm_sub_epi16(top_hi, top_left_x2);
  auto* dst = static_cast<uint8_t*>(dest);
  WritePaethLine16<0>(dst, top, left, top_lefts8, top_dists, left_dists,
                      left_dists_lo, left_dists_hi, top_left_diff_lo,
                      top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x01010101>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x02020202>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x03030303>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
}

// Inlined for calling with offsets in larger transform sizes, mainly to
// preserve top_left.
inline void WritePaeth16x8(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                           const uint8_t top_left, const __m128i top,
                           const __m128i left) {
  const __m128i top_lo = _mm_cvtepu8_epi16(top);
  const __m128i top_hi = _mm_cvtepu8_epi16(_mm_srli_si128(top, 8));

  const __m128i top_lefts16 = _mm_set1_epi16(top_left);
  const __m128i top_lefts8 = _mm_set1_epi8(static_cast<int8_t>(top_left));

  // Given that the spec defines "base" as top[x] + left[y] - top_left,
  // pLeft = abs(base - left[y]) = abs(top[x] - top[-1])
  // pTop = abs(base - top[x]) = abs(left[y] - top[-1])

  const __m128i left_dists = _mm_or_si128(_mm_subs_epu8(top, top_lefts8),
                                          _mm_subs_epu8(top_lefts8, top));
  const __m128i left_dists_lo = _mm_cvtepu8_epi16(left_dists);
  const __m128i left_dists_hi =
      _mm_cvtepu8_epi16(_mm_srli_si128(left_dists, 8));
  const __m128i top_dists = _mm_or_si128(_mm_subs_epu8(left, top_lefts8),
                                         _mm_subs_epu8(top_lefts8, left));

  const __m128i top_left_x2 = _mm_add_epi16(top_lefts16, top_lefts16);
  const __m128i top_left_diff_lo = _mm_sub_epi16(top_lo, top_left_x2);
  const __m128i top_left_diff_hi = _mm_sub_epi16(top_hi, top_left_x2);
  auto* dst = static_cast<uint8_t*>(dest);
  WritePaethLine16<0>(dst, top, left, top_lefts8, top_dists, left_dists,
                      left_dists_lo, left_dists_hi, top_left_diff_lo,
                      top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x01010101>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x02020202>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x03030303>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x04040404>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x05050505>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x06060606>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x07070707>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
}

void Paeth16x8_SSE4_1(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                      const void* LIBGAV1_RESTRICT const top_row,
                      const void* LIBGAV1_RESTRICT const left_column) {
  const __m128i top = LoadUnaligned16(top_row);
  const __m128i left = LoadLo8(left_column);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  WritePaeth16x8(static_cast<uint8_t*>(dest), stride, top_ptr[-1], top, left);
}

void WritePaeth16x16(void* const dest, ptrdiff_t stride, const uint8_t top_left,
                     const __m128i top, const __m128i left) {
  const __m128i top_lo = _mm_cvtepu8_epi16(top);
  const __m128i top_hi = _mm_cvtepu8_epi16(_mm_srli_si128(top, 8));

  const __m128i top_lefts16 = _mm_set1_epi16(top_left);
  const __m128i top_lefts8 = _mm_set1_epi8(static_cast<int8_t>(top_left));

  // Given that the spec defines "base" as top[x] + left[y] - top[-1],
  // pLeft = abs(base - left[y]) = abs(top[x] - top[-1])
  // pTop = abs(base - top[x]) = abs(left[y] - top[-1])

  const __m128i left_dists = _mm_or_si128(_mm_subs_epu8(top, top_lefts8),
                                          _mm_subs_epu8(top_lefts8, top));
  const __m128i left_dists_lo = _mm_cvtepu8_epi16(left_dists);
  const __m128i left_dists_hi =
      _mm_cvtepu8_epi16(_mm_srli_si128(left_dists, 8));
  const __m128i top_dists = _mm_or_si128(_mm_subs_epu8(left, top_lefts8),
                                         _mm_subs_epu8(top_lefts8, left));

  const __m128i top_left_x2 = _mm_add_epi16(top_lefts16, top_lefts16);
  const __m128i top_left_diff_lo = _mm_sub_epi16(top_lo, top_left_x2);
  const __m128i top_left_diff_hi = _mm_sub_epi16(top_hi, top_left_x2);
  auto* dst = static_cast<uint8_t*>(dest);
  WritePaethLine16<0>(dst, top, left, top_lefts8, top_dists, left_dists,
                      left_dists_lo, left_dists_hi, top_left_diff_lo,
                      top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x01010101>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x02020202>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x03030303>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x04040404>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x05050505>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x06060606>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x07070707>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x08080808>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x09090909>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x0A0A0A0A>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x0B0B0B0B>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x0C0C0C0C>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x0D0D0D0D>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x0E0E0E0E>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
  dst += stride;
  WritePaethLine16<0x0F0F0F0F>(dst, top, left, top_lefts8, top_dists,
                               left_dists, left_dists_lo, left_dists_hi,
                               top_left_diff_lo, top_left_diff_hi);
}

void Paeth16x16_SSE4_1(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                       const void* LIBGAV1_RESTRICT const top_row,
                       const void* LIBGAV1_RESTRICT const left_column) {
  const __m128i left = LoadUnaligned16(left_column);
  const __m128i top = LoadUnaligned16(top_row);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  WritePaeth16x16(static_cast<uint8_t*>(dest), stride, top_ptr[-1], top, left);
}

void Paeth16x32_SSE4_1(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                       const void* LIBGAV1_RESTRICT const top_row,
                       const void* LIBGAV1_RESTRICT const left_column) {
  const __m128i left_0 = LoadUnaligned16(left_column);
  const __m128i top = LoadUnaligned16(top_row);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const uint8_t top_left = top_ptr[-1];
  auto* const dst = static_cast<uint8_t*>(dest);
  WritePaeth16x16(dst, stride, top_left, top, left_0);
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  const __m128i left_1 = LoadUnaligned16(left_ptr + 16);
  WritePaeth16x16(dst + (stride << 4), stride, top_left, top, left_1);
}

void Paeth16x64_SSE4_1(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                       const void* LIBGAV1_RESTRICT const top_row,
                       const void* LIBGAV1_RESTRICT const left_column) {
  const ptrdiff_t stride16 = stride << 4;
  const __m128i left_0 = LoadUnaligned16(left_column);
  const __m128i top = LoadUnaligned16(top_row);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const uint8_t top_left = top_ptr[-1];
  auto* dst = static_cast<uint8_t*>(dest);
  WritePaeth16x16(dst, stride, top_left, top, left_0);
  dst += stride16;
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  const __m128i left_1 = LoadUnaligned16(left_ptr + 16);
  WritePaeth16x16(dst, stride, top_left, top, left_1);
  dst += stride16;
  const __m128i left_2 = LoadUnaligned16(left_ptr + 32);
  WritePaeth16x16(dst, stride, top_left, top, left_2);
  dst += stride16;
  const __m128i left_3 = LoadUnaligned16(left_ptr + 48);
  WritePaeth16x16(dst, stride, top_left, top, left_3);
}

void Paeth32x8_SSE4_1(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                      const void* LIBGAV1_RESTRICT const top_row,
                      const void* LIBGAV1_RESTRICT const left_column) {
  const __m128i left = LoadLo8(left_column);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const __m128i top_0 = LoadUnaligned16(top_row);
  const uint8_t top_left = top_ptr[-1];
  auto* const dst = static_cast<uint8_t*>(dest);
  WritePaeth16x8(dst, stride, top_left, top_0, left);
  const __m128i top_1 = LoadUnaligned16(top_ptr + 16);
  WritePaeth16x8(dst + 16, stride, top_left, top_1, left);
}

void Paeth32x16_SSE4_1(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                       const void* LIBGAV1_RESTRICT const top_row,
                       const void* LIBGAV1_RESTRICT const left_column) {
  const __m128i left = LoadUnaligned16(left_column);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const __m128i top_0 = LoadUnaligned16(top_row);
  const uint8_t top_left = top_ptr[-1];
  auto* const dst = static_cast<uint8_t*>(dest);
  WritePaeth16x16(dst, stride, top_left, top_0, left);
  const __m128i top_1 = LoadUnaligned16(top_ptr + 16);
  WritePaeth16x16(dst + 16, stride, top_left, top_1, left);
}

void Paeth32x32_SSE4_1(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                       const void* LIBGAV1_RESTRICT const top_row,
                       const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  const __m128i left_0 = LoadUnaligned16(left_ptr);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const __m128i top_0 = LoadUnaligned16(top_ptr);
  const __m128i left_1 = LoadUnaligned16(left_ptr + 16);
  const __m128i top_1 = LoadUnaligned16(top_ptr + 16);
  const uint8_t top_left = top_ptr[-1];
  auto* dst = static_cast<uint8_t*>(dest);
  WritePaeth16x16(dst, stride, top_left, top_0, left_0);
  WritePaeth16x16(dst + 16, stride, top_left, top_1, left_0);
  dst += (stride << 4);
  WritePaeth16x16(dst, stride, top_left, top_0, left_1);
  WritePaeth16x16(dst + 16, stride, top_left, top_1, left_1);
}

void Paeth32x64_SSE4_1(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                       const void* LIBGAV1_RESTRICT const top_row,
                       const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  const __m128i left_0 = LoadUnaligned16(left_ptr);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const __m128i top_0 = LoadUnaligned16(top_ptr);
  const __m128i left_1 = LoadUnaligned16(left_ptr + 16);
  const __m128i top_1 = LoadUnaligned16(top_ptr + 16);
  const __m128i left_2 = LoadUnaligned16(left_ptr + 32);
  const __m128i left_3 = LoadUnaligned16(left_ptr + 48);
  const uint8_t top_left = top_ptr[-1];
  auto* dst = static_cast<uint8_t*>(dest);
  WritePaeth16x16(dst, stride, top_left, top_0, left_0);
  WritePaeth16x16(dst + 16, stride, top_left, top_1, left_0);
  dst += (stride << 4);
  WritePaeth16x16(dst, stride, top_left, top_0, left_1);
  WritePaeth16x16(dst + 16, stride, top_left, top_1, left_1);
  dst += (stride << 4);
  WritePaeth16x16(dst, stride, top_left, top_0, left_2);
  WritePaeth16x16(dst + 16, stride, top_left, top_1, left_2);
  dst += (stride << 4);
  WritePaeth16x16(dst, stride, top_left, top_0, left_3);
  WritePaeth16x16(dst + 16, stride, top_left, top_1, left_3);
}

void Paeth64x16_SSE4_1(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                       const void* LIBGAV1_RESTRICT const top_row,
                       const void* LIBGAV1_RESTRICT const left_column) {
  const __m128i left = LoadUnaligned16(left_column);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const __m128i top_0 = LoadUnaligned16(top_ptr);
  const __m128i top_1 = LoadUnaligned16(top_ptr + 16);
  const __m128i top_2 = LoadUnaligned16(top_ptr + 32);
  const __m128i top_3 = LoadUnaligned16(top_ptr + 48);
  const uint8_t top_left = top_ptr[-1];
  auto* dst = static_cast<uint8_t*>(dest);
  WritePaeth16x16(dst, stride, top_left, top_0, left);
  WritePaeth16x16(dst + 16, stride, top_left, top_1, left);
  WritePaeth16x16(dst + 32, stride, top_left, top_2, left);
  WritePaeth16x16(dst + 48, stride, top_left, top_3, left);
}

void Paeth64x32_SSE4_1(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                       const void* LIBGAV1_RESTRICT const top_row,
                       const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  const __m128i left_0 = LoadUnaligned16(left_ptr);
  const __m128i left_1 = LoadUnaligned16(left_ptr + 16);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const __m128i top_0 = LoadUnaligned16(top_ptr);
  const __m128i top_1 = LoadUnaligned16(top_ptr + 16);
  const __m128i top_2 = LoadUnaligned16(top_ptr + 32);
  const __m128i top_3 = LoadUnaligned16(top_ptr + 48);
  const uint8_t top_left = top_ptr[-1];
  auto* dst = static_cast<uint8_t*>(dest);
  WritePaeth16x16(dst, stride, top_left, top_0, left_0);
  WritePaeth16x16(dst + 16, stride, top_left, top_1, left_0);
  WritePaeth16x16(dst + 32, stride, top_left, top_2, left_0);
  WritePaeth16x16(dst + 48, stride, top_left, top_3, left_0);
  dst += (stride << 4);
  WritePaeth16x16(dst, stride, top_left, top_0, left_1);
  WritePaeth16x16(dst + 16, stride, top_left, top_1, left_1);
  WritePaeth16x16(dst + 32, stride, top_left, top_2, left_1);
  WritePaeth16x16(dst + 48, stride, top_left, top_3, left_1);
}

void Paeth64x64_SSE4_1(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                       const void* LIBGAV1_RESTRICT const top_row,
                       const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left_ptr = static_cast<const uint8_t*>(left_column);
  const __m128i left_0 = LoadUnaligned16(left_ptr);
  const __m128i left_1 = LoadUnaligned16(left_ptr + 16);
  const __m128i left_2 = LoadUnaligned16(left_ptr + 32);
  const __m128i left_3 = LoadUnaligned16(left_ptr + 48);
  const auto* const top_ptr = static_cast<const uint8_t*>(top_row);
  const __m128i top_0 = LoadUnaligned16(top_ptr);
  const __m128i top_1 = LoadUnaligned16(top_ptr + 16);
  const __m128i top_2 = LoadUnaligned16(top_ptr + 32);
  const __m128i top_3 = LoadUnaligned16(top_ptr + 48);
  const uint8_t top_left = top_ptr[-1];
  auto* dst = static_cast<uint8_t*>(dest);
  WritePaeth16x16(dst, stride, top_left, top_0, left_0);
  WritePaeth16x16(dst + 16, stride, top_left, top_1, left_0);
  WritePaeth16x16(dst + 32, stride, top_left, top_2, left_0);
  WritePaeth16x16(dst + 48, stride, top_left, top_3, left_0);
  dst += (stride << 4);
  WritePaeth16x16(dst, stride, top_left, top_0, left_1);
  WritePaeth16x16(dst + 16, stride, top_left, top_1, left_1);
  WritePaeth16x16(dst + 32, stride, top_left, top_2, left_1);
  WritePaeth16x16(dst + 48, stride, top_left, top_3, left_1);
  dst += (stride << 4);
  WritePaeth16x16(dst, stride, top_left, top_0, left_2);
  WritePaeth16x16(dst + 16, stride, top_left, top_1, left_2);
  WritePaeth16x16(dst + 32, stride, top_left, top_2, left_2);
  WritePaeth16x16(dst + 48, stride, top_left, top_3, left_2);
  dst += (stride << 4);
  WritePaeth16x16(dst, stride, top_left, top_0, left_3);
  WritePaeth16x16(dst + 16, stride, top_left, top_1, left_3);
  WritePaeth16x16(dst + 32, stride, top_left, top_2, left_3);
  WritePaeth16x16(dst + 48, stride, top_left, top_3, left_3);
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
  static_cast<void>(dsp);
// These guards check if this version of the function was not superseded by
// a higher optimization level, such as AVX. The corresponding #define also
// prevents the C version from being added to the table.
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x4_IntraPredictorDcTop)
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDcTop] =
      DcDefs::_4x4::DcTop;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x8_IntraPredictorDcTop)
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorDcTop] =
      DcDefs::_4x8::DcTop;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x16_IntraPredictorDcTop)
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorDcTop] =
      DcDefs::_4x16::DcTop;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x4_IntraPredictorDcTop)
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorDcTop] =
      DcDefs::_8x4::DcTop;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x8_IntraPredictorDcTop)
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorDcTop] =
      DcDefs::_8x8::DcTop;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x16_IntraPredictorDcTop)
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorDcTop] =
      DcDefs::_8x16::DcTop;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x32_IntraPredictorDcTop)
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorDcTop] =
      DcDefs::_8x32::DcTop;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x4_IntraPredictorDcTop)
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorDcTop] =
      DcDefs::_16x4::DcTop;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x8_IntraPredictorDcTop)
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorDcTop] =
      DcDefs::_16x8::DcTop;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x16_IntraPredictorDcTop)
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorDcTop] =
      DcDefs::_16x16::DcTop;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x32_IntraPredictorDcTop)
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorDcTop] =
      DcDefs::_16x32::DcTop;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x64_IntraPredictorDcTop)
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorDcTop] =
      DcDefs::_16x64::DcTop;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x8_IntraPredictorDcTop)
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorDcTop] =
      DcDefs::_32x8::DcTop;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x16_IntraPredictorDcTop)
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorDcTop] =
      DcDefs::_32x16::DcTop;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x32_IntraPredictorDcTop)
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorDcTop] =
      DcDefs::_32x32::DcTop;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x64_IntraPredictorDcTop)
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorDcTop] =
      DcDefs::_32x64::DcTop;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x16_IntraPredictorDcTop)
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorDcTop] =
      DcDefs::_64x16::DcTop;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x32_IntraPredictorDcTop)
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorDcTop] =
      DcDefs::_64x32::DcTop;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x64_IntraPredictorDcTop)
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorDcTop] =
      DcDefs::_64x64::DcTop;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x4_IntraPredictorDcLeft)
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDcLeft] =
      DcDefs::_4x4::DcLeft;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x8_IntraPredictorDcLeft)
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorDcLeft] =
      DcDefs::_4x8::DcLeft;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x16_IntraPredictorDcLeft)
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorDcLeft] =
      DcDefs::_4x16::DcLeft;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x4_IntraPredictorDcLeft)
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorDcLeft] =
      DcDefs::_8x4::DcLeft;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x8_IntraPredictorDcLeft)
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorDcLeft] =
      DcDefs::_8x8::DcLeft;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x16_IntraPredictorDcLeft)
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorDcLeft] =
      DcDefs::_8x16::DcLeft;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x32_IntraPredictorDcLeft)
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorDcLeft] =
      DcDefs::_8x32::DcLeft;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x4_IntraPredictorDcLeft)
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorDcLeft] =
      DcDefs::_16x4::DcLeft;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x8_IntraPredictorDcLeft)
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorDcLeft] =
      DcDefs::_16x8::DcLeft;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x16_IntraPredictorDcLeft)
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorDcLeft] =
      DcDefs::_16x16::DcLeft;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x32_IntraPredictorDcLeft)
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorDcLeft] =
      DcDefs::_16x32::DcLeft;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x64_IntraPredictorDcLeft)
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorDcLeft] =
      DcDefs::_16x64::DcLeft;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x8_IntraPredictorDcLeft)
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorDcLeft] =
      DcDefs::_32x8::DcLeft;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x16_IntraPredictorDcLeft)
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorDcLeft] =
      DcDefs::_32x16::DcLeft;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x32_IntraPredictorDcLeft)
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorDcLeft] =
      DcDefs::_32x32::DcLeft;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x64_IntraPredictorDcLeft)
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorDcLeft] =
      DcDefs::_32x64::DcLeft;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x16_IntraPredictorDcLeft)
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorDcLeft] =
      DcDefs::_64x16::DcLeft;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x32_IntraPredictorDcLeft)
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorDcLeft] =
      DcDefs::_64x32::DcLeft;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x64_IntraPredictorDcLeft)
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorDcLeft] =
      DcDefs::_64x64::DcLeft;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x4_IntraPredictorDc)
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDc] =
      DcDefs::_4x4::Dc;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x8_IntraPredictorDc)
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorDc] =
      DcDefs::_4x8::Dc;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x16_IntraPredictorDc)
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorDc] =
      DcDefs::_4x16::Dc;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x4_IntraPredictorDc)
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorDc] =
      DcDefs::_8x4::Dc;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x8_IntraPredictorDc)
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorDc] =
      DcDefs::_8x8::Dc;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x16_IntraPredictorDc)
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorDc] =
      DcDefs::_8x16::Dc;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x32_IntraPredictorDc)
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorDc] =
      DcDefs::_8x32::Dc;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x4_IntraPredictorDc)
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorDc] =
      DcDefs::_16x4::Dc;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x8_IntraPredictorDc)
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorDc] =
      DcDefs::_16x8::Dc;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x16_IntraPredictorDc)
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorDc] =
      DcDefs::_16x16::Dc;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x32_IntraPredictorDc)
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorDc] =
      DcDefs::_16x32::Dc;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x64_IntraPredictorDc)
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorDc] =
      DcDefs::_16x64::Dc;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x8_IntraPredictorDc)
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorDc] =
      DcDefs::_32x8::Dc;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x16_IntraPredictorDc)
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorDc] =
      DcDefs::_32x16::Dc;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x32_IntraPredictorDc)
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorDc] =
      DcDefs::_32x32::Dc;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x64_IntraPredictorDc)
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorDc] =
      DcDefs::_32x64::Dc;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x16_IntraPredictorDc)
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorDc] =
      DcDefs::_64x16::Dc;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x32_IntraPredictorDc)
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorDc] =
      DcDefs::_64x32::Dc;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x64_IntraPredictorDc)
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorDc] =
      DcDefs::_64x64::Dc;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x4_IntraPredictorPaeth)
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorPaeth] =
      Paeth4x4_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x8_IntraPredictorPaeth)
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorPaeth] =
      Paeth4x8_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x16_IntraPredictorPaeth)
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorPaeth] =
      Paeth4x16_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x4_IntraPredictorPaeth)
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorPaeth] =
      Paeth8x4_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x8_IntraPredictorPaeth)
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorPaeth] =
      Paeth8x8_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x16_IntraPredictorPaeth)
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorPaeth] =
      Paeth8x16_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x32_IntraPredictorPaeth)
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorPaeth] =
      Paeth8x32_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x4_IntraPredictorPaeth)
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorPaeth] =
      Paeth16x4_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x8_IntraPredictorPaeth)
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorPaeth] =
      Paeth16x8_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x16_IntraPredictorPaeth)
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorPaeth] =
      Paeth16x16_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x32_IntraPredictorPaeth)
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorPaeth] =
      Paeth16x32_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x64_IntraPredictorPaeth)
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorPaeth] =
      Paeth16x64_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x8_IntraPredictorPaeth)
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorPaeth] =
      Paeth32x8_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x16_IntraPredictorPaeth)
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorPaeth] =
      Paeth32x16_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x32_IntraPredictorPaeth)
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorPaeth] =
      Paeth32x32_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x64_IntraPredictorPaeth)
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorPaeth] =
      Paeth32x64_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x16_IntraPredictorPaeth)
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorPaeth] =
      Paeth64x16_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x32_IntraPredictorPaeth)
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorPaeth] =
      Paeth64x32_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x64_IntraPredictorPaeth)
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorPaeth] =
      Paeth64x64_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x4_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorHorizontal] =
      DirDefs::_4x4::Horizontal;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x8_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorHorizontal] =
      DirDefs::_4x8::Horizontal;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x16_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorHorizontal] =
      DirDefs::_4x16::Horizontal;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x4_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorHorizontal] =
      DirDefs::_8x4::Horizontal;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x8_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorHorizontal] =
      DirDefs::_8x8::Horizontal;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x16_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorHorizontal] =
      DirDefs::_8x16::Horizontal;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize8x32_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorHorizontal] =
      DirDefs::_8x32::Horizontal;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x4_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorHorizontal] =
      DirDefs::_16x4::Horizontal;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x8_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorHorizontal] =
      DirDefs::_16x8::Horizontal;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x16_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorHorizontal] =
      DirDefs::_16x16::Horizontal;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x32_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorHorizontal] =
      DirDefs::_16x32::Horizontal;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize16x64_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorHorizontal] =
      DirDefs::_16x64::Horizontal;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x8_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorHorizontal] =
      DirDefs::_32x8::Horizontal;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x16_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorHorizontal] =
      DirDefs::_32x16::Horizontal;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x32_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorHorizontal] =
      DirDefs::_32x32::Horizontal;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize32x64_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorHorizontal] =
      DirDefs::_32x64::Horizontal;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x16_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorHorizontal] =
      DirDefs::_64x16::Horizontal;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x32_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorHorizontal] =
      DirDefs::_64x32::Horizontal;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(TransformSize64x64_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorHorizontal] =
      DirDefs::_64x64::Horizontal;
#endif
}  // NOLINT(readability/fn_size)

}  // namespace
}  // namespace low_bitdepth

//------------------------------------------------------------------------------
#if LIBGAV1_MAX_BITDEPTH >= 10
namespace high_bitdepth {
namespace {

template <int height>
inline void DcStore4xH_SSE4_1(void* const dest, ptrdiff_t stride,
                              const __m128i dc) {
  const __m128i dc_dup = _mm_shufflelo_epi16(dc, 0);
  int y = height - 1;
  auto* dst = static_cast<uint8_t*>(dest);
  do {
    StoreLo8(dst, dc_dup);
    dst += stride;
  } while (--y != 0);
  StoreLo8(dst, dc_dup);
}

// WriteDuplicateN assumes dup has 4 32-bit "units," each of which comprises 2
// identical shorts that need N total copies written into dest. The unpacking
// works the same as in the 8bpp case, except that each 32-bit unit needs twice
// as many copies.
inline void WriteDuplicate4x4(void* const dest, ptrdiff_t stride,
                              const __m128i dup32) {
  const __m128i dup64_lo = _mm_unpacklo_epi32(dup32, dup32);
  auto* dst = static_cast<uint8_t*>(dest);
  _mm_storel_epi64(reinterpret_cast<__m128i*>(dst), dup64_lo);
  dst += stride;
  _mm_storeh_pi(reinterpret_cast<__m64*>(dst), _mm_castsi128_ps(dup64_lo));
  dst += stride;
  const __m128i dup64_hi = _mm_unpackhi_epi32(dup32, dup32);
  _mm_storel_epi64(reinterpret_cast<__m128i*>(dst), dup64_hi);
  dst += stride;
  _mm_storeh_pi(reinterpret_cast<__m64*>(dst), _mm_castsi128_ps(dup64_hi));
}

inline void WriteDuplicate8x4(void* const dest, ptrdiff_t stride,
                              const __m128i dup32) {
  const __m128i dup64_lo = _mm_unpacklo_epi32(dup32, dup32);
  const __m128i dup64_hi = _mm_unpackhi_epi32(dup32, dup32);

  auto* dst = static_cast<uint8_t*>(dest);
  const __m128i dup128_0 = _mm_unpacklo_epi64(dup64_lo, dup64_lo);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_0);
  dst += stride;
  const __m128i dup128_1 = _mm_unpackhi_epi64(dup64_lo, dup64_lo);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_1);
  dst += stride;
  const __m128i dup128_2 = _mm_unpacklo_epi64(dup64_hi, dup64_hi);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_2);
  dst += stride;
  const __m128i dup128_3 = _mm_unpackhi_epi64(dup64_hi, dup64_hi);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_3);
}

inline void WriteDuplicate16x4(void* const dest, ptrdiff_t stride,
                               const __m128i dup32) {
  const __m128i dup64_lo = _mm_unpacklo_epi32(dup32, dup32);
  const __m128i dup64_hi = _mm_unpackhi_epi32(dup32, dup32);

  auto* dst = static_cast<uint8_t*>(dest);
  const __m128i dup128_0 = _mm_unpacklo_epi64(dup64_lo, dup64_lo);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_0);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 16), dup128_0);
  dst += stride;
  const __m128i dup128_1 = _mm_unpackhi_epi64(dup64_lo, dup64_lo);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_1);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 16), dup128_1);
  dst += stride;
  const __m128i dup128_2 = _mm_unpacklo_epi64(dup64_hi, dup64_hi);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_2);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 16), dup128_2);
  dst += stride;
  const __m128i dup128_3 = _mm_unpackhi_epi64(dup64_hi, dup64_hi);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_3);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 16), dup128_3);
}

inline void WriteDuplicate32x4(void* const dest, ptrdiff_t stride,
                               const __m128i dup32) {
  const __m128i dup64_lo = _mm_unpacklo_epi32(dup32, dup32);
  const __m128i dup64_hi = _mm_unpackhi_epi32(dup32, dup32);

  auto* dst = static_cast<uint8_t*>(dest);
  const __m128i dup128_0 = _mm_unpacklo_epi64(dup64_lo, dup64_lo);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_0);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 16), dup128_0);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 32), dup128_0);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 48), dup128_0);
  dst += stride;
  const __m128i dup128_1 = _mm_unpackhi_epi64(dup64_lo, dup64_lo);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_1);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 16), dup128_1);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 32), dup128_1);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 48), dup128_1);
  dst += stride;
  const __m128i dup128_2 = _mm_unpacklo_epi64(dup64_hi, dup64_hi);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_2);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 16), dup128_2);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 32), dup128_2);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 48), dup128_2);
  dst += stride;
  const __m128i dup128_3 = _mm_unpackhi_epi64(dup64_hi, dup64_hi);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), dup128_3);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 16), dup128_3);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 32), dup128_3);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 48), dup128_3);
}

inline void WriteDuplicate64x4(void* const dest, ptrdiff_t stride,
                               const __m128i dup32) {
  const __m128i dup64_lo = _mm_unpacklo_epi32(dup32, dup32);
  const __m128i dup64_hi = _mm_unpackhi_epi32(dup32, dup32);

  auto* dst = static_cast<uint8_t*>(dest);
  const __m128i dup128_0 = _mm_unpacklo_epi64(dup64_lo, dup64_lo);
  for (int x = 0; x < 128; x += 16) {
    _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + x), dup128_0);
  }
  dst += stride;
  const __m128i dup128_1 = _mm_unpackhi_epi64(dup64_lo, dup64_lo);
  for (int x = 0; x < 128; x += 16) {
    _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + x), dup128_1);
  }
  dst += stride;
  const __m128i dup128_2 = _mm_unpacklo_epi64(dup64_hi, dup64_hi);
  for (int x = 0; x < 128; x += 16) {
    _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + x), dup128_2);
  }
  dst += stride;
  const __m128i dup128_3 = _mm_unpackhi_epi64(dup64_hi, dup64_hi);
  for (int x = 0; x < 128; x += 16) {
    _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + x), dup128_3);
  }
}

// ColStoreN<height> copies each of the |height| values in |column| across its
// corresponding row in dest.
template <WriteDuplicateFunc writefn>
inline void ColStore4_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                             ptrdiff_t stride,
                             const void* LIBGAV1_RESTRICT const column) {
  const __m128i col_data = LoadLo8(column);
  const __m128i col_dup32 = _mm_unpacklo_epi16(col_data, col_data);
  writefn(dest, stride, col_dup32);
}

template <WriteDuplicateFunc writefn>
inline void ColStore8_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                             ptrdiff_t stride,
                             const void* LIBGAV1_RESTRICT const column) {
  const __m128i col_data = LoadUnaligned16(column);
  const __m128i col_dup32_lo = _mm_unpacklo_epi16(col_data, col_data);
  const __m128i col_dup32_hi = _mm_unpackhi_epi16(col_data, col_data);
  auto* dst = static_cast<uint8_t*>(dest);
  writefn(dst, stride, col_dup32_lo);
  const ptrdiff_t stride4 = stride << 2;
  dst += stride4;
  writefn(dst, stride, col_dup32_hi);
}

template <WriteDuplicateFunc writefn>
inline void ColStore16_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                              ptrdiff_t stride,
                              const void* LIBGAV1_RESTRICT const column) {
  const ptrdiff_t stride4 = stride << 2;
  auto* dst = static_cast<uint8_t*>(dest);
  for (int y = 0; y < 32; y += 16) {
    const __m128i col_data =
        LoadUnaligned16(static_cast<const uint8_t*>(column) + y);
    const __m128i col_dup32_lo = _mm_unpacklo_epi16(col_data, col_data);
    const __m128i col_dup32_hi = _mm_unpackhi_epi16(col_data, col_data);
    writefn(dst, stride, col_dup32_lo);
    dst += stride4;
    writefn(dst, stride, col_dup32_hi);
    dst += stride4;
  }
}

template <WriteDuplicateFunc writefn>
inline void ColStore32_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                              ptrdiff_t stride,
                              const void* LIBGAV1_RESTRICT const column) {
  const ptrdiff_t stride4 = stride << 2;
  auto* dst = static_cast<uint8_t*>(dest);
  for (int y = 0; y < 64; y += 16) {
    const __m128i col_data =
        LoadUnaligned16(static_cast<const uint8_t*>(column) + y);
    const __m128i col_dup32_lo = _mm_unpacklo_epi16(col_data, col_data);
    const __m128i col_dup32_hi = _mm_unpackhi_epi16(col_data, col_data);
    writefn(dst, stride, col_dup32_lo);
    dst += stride4;
    writefn(dst, stride, col_dup32_hi);
    dst += stride4;
  }
}

template <WriteDuplicateFunc writefn>
inline void ColStore64_SSE4_1(void* LIBGAV1_RESTRICT const dest,
                              ptrdiff_t stride,
                              const void* LIBGAV1_RESTRICT const column) {
  const ptrdiff_t stride4 = stride << 2;
  auto* dst = static_cast<uint8_t*>(dest);
  for (int y = 0; y < 128; y += 16) {
    const __m128i col_data =
        LoadUnaligned16(static_cast<const uint8_t*>(column) + y);
    const __m128i col_dup32_lo = _mm_unpacklo_epi16(col_data, col_data);
    const __m128i col_dup32_hi = _mm_unpackhi_epi16(col_data, col_data);
    writefn(dst, stride, col_dup32_lo);
    dst += stride4;
    writefn(dst, stride, col_dup32_hi);
    dst += stride4;
  }
}

// |ref| points to 8 bytes containing 4 packed int16 values.
inline __m128i DcSum4_SSE4_1(const void* ref) {
  const __m128i vals = _mm_loadl_epi64(static_cast<const __m128i*>(ref));
  const __m128i ones = _mm_set1_epi16(1);

  // half_sum[31:0]  = a1+a2
  // half_sum[63:32] = a3+a4
  const __m128i half_sum = _mm_madd_epi16(vals, ones);
  // Place half_sum[63:32] in shift_sum[31:0].
  const __m128i shift_sum = _mm_srli_si128(half_sum, 4);
  return _mm_add_epi32(half_sum, shift_sum);
}

struct DcDefs {
  DcDefs() = delete;

  using _4x4 = DcPredFuncs_SSE4_1<2, 2, DcSum4_SSE4_1, DcSum4_SSE4_1,
                                  DcStore4xH_SSE4_1<4>, 0, 0>;
};

struct DirDefs {
  DirDefs() = delete;

  using _4x4 = DirectionalPredFuncs_SSE4_1<ColStore4_SSE4_1<WriteDuplicate4x4>>;
  using _4x8 = DirectionalPredFuncs_SSE4_1<ColStore8_SSE4_1<WriteDuplicate4x4>>;
  using _4x16 =
      DirectionalPredFuncs_SSE4_1<ColStore16_SSE4_1<WriteDuplicate4x4>>;
  using _8x4 = DirectionalPredFuncs_SSE4_1<ColStore4_SSE4_1<WriteDuplicate8x4>>;
  using _8x8 = DirectionalPredFuncs_SSE4_1<ColStore8_SSE4_1<WriteDuplicate8x4>>;
  using _8x16 =
      DirectionalPredFuncs_SSE4_1<ColStore16_SSE4_1<WriteDuplicate8x4>>;
  using _8x32 =
      DirectionalPredFuncs_SSE4_1<ColStore32_SSE4_1<WriteDuplicate8x4>>;
  using _16x4 =
      DirectionalPredFuncs_SSE4_1<ColStore4_SSE4_1<WriteDuplicate16x4>>;
  using _16x8 =
      DirectionalPredFuncs_SSE4_1<ColStore8_SSE4_1<WriteDuplicate16x4>>;
  using _16x16 =
      DirectionalPredFuncs_SSE4_1<ColStore16_SSE4_1<WriteDuplicate16x4>>;
  using _16x32 =
      DirectionalPredFuncs_SSE4_1<ColStore32_SSE4_1<WriteDuplicate16x4>>;
  using _16x64 =
      DirectionalPredFuncs_SSE4_1<ColStore64_SSE4_1<WriteDuplicate16x4>>;
  using _32x8 =
      DirectionalPredFuncs_SSE4_1<ColStore8_SSE4_1<WriteDuplicate32x4>>;
  using _32x16 =
      DirectionalPredFuncs_SSE4_1<ColStore16_SSE4_1<WriteDuplicate32x4>>;
  using _32x32 =
      DirectionalPredFuncs_SSE4_1<ColStore32_SSE4_1<WriteDuplicate32x4>>;
  using _32x64 =
      DirectionalPredFuncs_SSE4_1<ColStore64_SSE4_1<WriteDuplicate32x4>>;
  using _64x16 =
      DirectionalPredFuncs_SSE4_1<ColStore16_SSE4_1<WriteDuplicate64x4>>;
  using _64x32 =
      DirectionalPredFuncs_SSE4_1<ColStore32_SSE4_1<WriteDuplicate64x4>>;
  using _64x64 =
      DirectionalPredFuncs_SSE4_1<ColStore64_SSE4_1<WriteDuplicate64x4>>;
};

void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(10);
  assert(dsp != nullptr);
  static_cast<void>(dsp);
#if DSP_ENABLED_10BPP_SSE4_1(TransformSize4x4_IntraPredictorDcTop)
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDcTop] =
      DcDefs::_4x4::DcTop;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(TransformSize4x4_IntraPredictorDcLeft)
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDcLeft] =
      DcDefs::_4x4::DcLeft;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(TransformSize4x4_IntraPredictorDc)
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDc] =
      DcDefs::_4x4::Dc;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(TransformSize4x4_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorHorizontal] =
      DirDefs::_4x4::Horizontal;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(TransformSize4x8_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorHorizontal] =
      DirDefs::_4x8::Horizontal;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(TransformSize4x16_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorHorizontal] =
      DirDefs::_4x16::Horizontal;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(TransformSize8x4_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorHorizontal] =
      DirDefs::_8x4::Horizontal;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(TransformSize8x8_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorHorizontal] =
      DirDefs::_8x8::Horizontal;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(TransformSize8x16_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorHorizontal] =
      DirDefs::_8x16::Horizontal;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(TransformSize8x32_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorHorizontal] =
      DirDefs::_8x32::Horizontal;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(TransformSize16x4_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorHorizontal] =
      DirDefs::_16x4::Horizontal;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(TransformSize16x8_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorHorizontal] =
      DirDefs::_16x8::Horizontal;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(TransformSize16x16_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorHorizontal] =
      DirDefs::_16x16::Horizontal;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(TransformSize16x32_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorHorizontal] =
      DirDefs::_16x32::Horizontal;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(TransformSize16x64_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorHorizontal] =
      DirDefs::_16x64::Horizontal;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(TransformSize32x8_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorHorizontal] =
      DirDefs::_32x8::Horizontal;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(TransformSize32x16_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorHorizontal] =
      DirDefs::_32x16::Horizontal;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(TransformSize32x32_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorHorizontal] =
      DirDefs::_32x32::Horizontal;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(TransformSize32x64_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorHorizontal] =
      DirDefs::_32x64::Horizontal;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(TransformSize64x16_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorHorizontal] =
      DirDefs::_64x16::Horizontal;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(TransformSize64x32_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorHorizontal] =
      DirDefs::_64x32::Horizontal;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(TransformSize64x64_IntraPredictorHorizontal)
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorHorizontal] =
      DirDefs::_64x64::Horizontal;
#endif
}

}  // namespace
}  // namespace high_bitdepth
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

void IntraPredInit_SSE4_1() {
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

void IntraPredInit_SSE4_1() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_TARGETING_SSE4_1
