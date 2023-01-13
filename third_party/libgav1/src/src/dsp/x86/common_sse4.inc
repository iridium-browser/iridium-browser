/*
 * Copyright 2021 The libgav1 Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//------------------------------------------------------------------------------
// Load functions.

inline __m128i Load2(const void* src) {
  int16_t val;
  memcpy(&val, src, sizeof(val));
  return _mm_cvtsi32_si128(val);
}

inline __m128i Load2x2(const void* src1, const void* src2) {
  uint16_t val1;
  uint16_t val2;
  memcpy(&val1, src1, sizeof(val1));
  memcpy(&val2, src2, sizeof(val2));
  return _mm_cvtsi32_si128(val1 | (val2 << 16));
}

// Load 2 uint8_t values into |lane| * 2 and |lane| * 2 + 1.
template <int lane>
inline __m128i Load2(const void* const buf, __m128i val) {
  int16_t temp;
  memcpy(&temp, buf, 2);
  return _mm_insert_epi16(val, temp, lane);
}

inline __m128i Load4(const void* src) {
  // With new compilers such as clang 8.0.0 we can use the new _mm_loadu_si32
  // intrinsic. Both _mm_loadu_si32(src) and the code here are compiled into a
  // movss instruction.
  //
  // Until compiler support of _mm_loadu_si32 is widespread, use of
  // _mm_loadu_si32 is banned.
  int val;
  memcpy(&val, src, sizeof(val));
  return _mm_cvtsi32_si128(val);
}

inline __m128i Load4x2(const void* src1, const void* src2) {
  // With new compilers such as clang 8.0.0 we can use the new _mm_loadu_si32
  // intrinsic. Both _mm_loadu_si32(src) and the code here are compiled into a
  // movss instruction.
  //
  // Until compiler support of _mm_loadu_si32 is widespread, use of
  // _mm_loadu_si32 is banned.
  int val1, val2;
  memcpy(&val1, src1, sizeof(val1));
  memcpy(&val2, src2, sizeof(val2));
  return _mm_insert_epi32(_mm_cvtsi32_si128(val1), val2, 1);
}

inline __m128i LoadLo8(const void* a) {
  return _mm_loadl_epi64(static_cast<const __m128i*>(a));
}

inline __m128i LoadHi8(const __m128i v, const void* a) {
  const __m128 x =
      _mm_loadh_pi(_mm_castsi128_ps(v), static_cast<const __m64*>(a));
  return _mm_castps_si128(x);
}

inline __m128i LoadUnaligned16(const void* a) {
  return _mm_loadu_si128(static_cast<const __m128i*>(a));
}

inline __m128i LoadAligned16(const void* a) {
  assert((reinterpret_cast<uintptr_t>(a) & 0xf) == 0);
  return _mm_load_si128(static_cast<const __m128i*>(a));
}

//------------------------------------------------------------------------------
// Load functions to avoid MemorySanitizer's use-of-uninitialized-value warning.

inline __m128i MaskOverreads(const __m128i source,
                             const ptrdiff_t over_read_in_bytes) {
  __m128i dst = source;
#if LIBGAV1_MSAN
  if (over_read_in_bytes > 0) {
    __m128i mask = _mm_set1_epi8(-1);
    for (ptrdiff_t i = 0; i < over_read_in_bytes; ++i) {
      mask = _mm_srli_si128(mask, 1);
    }
    dst = _mm_and_si128(dst, mask);
  }
#else
  static_cast<void>(over_read_in_bytes);
#endif
  return dst;
}

inline __m128i LoadLo8Msan(const void* const source,
                           const ptrdiff_t over_read_in_bytes) {
  return MaskOverreads(LoadLo8(source), over_read_in_bytes + 8);
}

inline __m128i LoadHi8Msan(const __m128i v, const void* source,
                           const ptrdiff_t over_read_in_bytes) {
  return MaskOverreads(LoadHi8(v, source), over_read_in_bytes);
}

inline __m128i LoadAligned16Msan(const void* const source,
                                 const ptrdiff_t over_read_in_bytes) {
  return MaskOverreads(LoadAligned16(source), over_read_in_bytes);
}

inline __m128i LoadUnaligned16Msan(const void* const source,
                                   const ptrdiff_t over_read_in_bytes) {
  return MaskOverreads(LoadUnaligned16(source), over_read_in_bytes);
}

//------------------------------------------------------------------------------
// Store functions.

inline void Store2(void* dst, const __m128i x) {
  const int val = _mm_cvtsi128_si32(x);
  memcpy(dst, &val, 2);
}

inline void Store4(void* dst, const __m128i x) {
  const int val = _mm_cvtsi128_si32(x);
  memcpy(dst, &val, sizeof(val));
}

inline void StoreLo8(void* a, const __m128i v) {
  _mm_storel_epi64(static_cast<__m128i*>(a), v);
}

inline void StoreHi8(void* a, const __m128i v) {
  _mm_storeh_pi(static_cast<__m64*>(a), _mm_castsi128_ps(v));
}

inline void StoreAligned16(void* a, const __m128i v) {
  assert((reinterpret_cast<uintptr_t>(a) & 0xf) == 0);
  _mm_store_si128(static_cast<__m128i*>(a), v);
}

inline void StoreUnaligned16(void* a, const __m128i v) {
  _mm_storeu_si128(static_cast<__m128i*>(a), v);
}

//------------------------------------------------------------------------------
// Arithmetic utilities.

inline __m128i RightShiftWithRounding_U16(const __m128i v_val_d, int bits) {
  assert(bits <= 16);
  // Shift out all but the last bit.
  const __m128i v_tmp_d = _mm_srli_epi16(v_val_d, bits - 1);
  // Avg with zero will shift by 1 and round.
  return _mm_avg_epu16(v_tmp_d, _mm_setzero_si128());
}

inline __m128i RightShiftWithRounding_S16(const __m128i v_val_d, int bits) {
  assert(bits < 16);
  const __m128i v_bias_d =
      _mm_set1_epi16(static_cast<int16_t>((1 << bits) >> 1));
  const __m128i v_tmp_d = _mm_add_epi16(v_val_d, v_bias_d);
  return _mm_srai_epi16(v_tmp_d, bits);
}

inline __m128i RightShiftWithRounding_U32(const __m128i v_val_d, int bits) {
  const __m128i v_bias_d = _mm_set1_epi32((1 << bits) >> 1);
  const __m128i v_tmp_d = _mm_add_epi32(v_val_d, v_bias_d);
  return _mm_srli_epi32(v_tmp_d, bits);
}

inline __m128i RightShiftWithRounding_S32(const __m128i v_val_d, int bits) {
  const __m128i v_bias_d = _mm_set1_epi32((1 << bits) >> 1);
  const __m128i v_tmp_d = _mm_add_epi32(v_val_d, v_bias_d);
  return _mm_srai_epi32(v_tmp_d, bits);
}

// Use this when |bits| is not an immediate value.
inline __m128i VariableRightShiftWithRounding_S32(const __m128i v_val_d,
                                                  int bits) {
  const __m128i v_bias_d =
      _mm_set1_epi32(static_cast<int32_t>((1 << bits) >> 1));
  const __m128i v_tmp_d = _mm_add_epi32(v_val_d, v_bias_d);
  return _mm_sra_epi32(v_tmp_d, _mm_cvtsi32_si128(bits));
}

//------------------------------------------------------------------------------
// Masking utilities
inline __m128i MaskHighNBytes(int n) {
  static constexpr uint8_t kMask[32] = {
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  };

  return LoadUnaligned16(kMask + n);
}
