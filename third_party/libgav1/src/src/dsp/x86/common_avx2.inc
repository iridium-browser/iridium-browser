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
// Compatibility functions.

inline __m256i SetrM128i(const __m128i lo, const __m128i hi) {
  // For compatibility with older gcc toolchains (< 8) use
  // _mm256_inserti128_si256 over _mm256_setr_m128i. Newer gcc implementations
  // are implemented similarly to the following, clang uses a different method
  // but no differences in assembly have been observed.
  return _mm256_inserti128_si256(_mm256_castsi128_si256(lo), hi, 1);
}

//------------------------------------------------------------------------------
// Load functions.

inline __m256i LoadAligned32(const void* a) {
  assert((reinterpret_cast<uintptr_t>(a) & 0x1f) == 0);
  return _mm256_load_si256(static_cast<const __m256i*>(a));
}

inline void LoadAligned64(const void* a, __m256i dst[2]) {
  assert((reinterpret_cast<uintptr_t>(a) & 0x1f) == 0);
  dst[0] = _mm256_load_si256(static_cast<const __m256i*>(a) + 0);
  dst[1] = _mm256_load_si256(static_cast<const __m256i*>(a) + 1);
}

inline __m256i LoadUnaligned32(const void* a) {
  return _mm256_loadu_si256(static_cast<const __m256i*>(a));
}

//------------------------------------------------------------------------------
// Load functions to avoid MemorySanitizer's use-of-uninitialized-value warning.

inline __m256i MaskOverreads(const __m256i source,
                             const ptrdiff_t over_read_in_bytes) {
  __m256i dst = source;
#if LIBGAV1_MSAN
  if (over_read_in_bytes >= 32) return _mm256_setzero_si256();
  if (over_read_in_bytes > 0) {
    __m128i m = _mm_set1_epi8(-1);
    for (ptrdiff_t i = 0; i < over_read_in_bytes % 16; ++i) {
      m = _mm_srli_si128(m, 1);
    }
    const __m256i mask = (over_read_in_bytes < 16)
                             ? SetrM128i(_mm_set1_epi8(-1), m)
                             : SetrM128i(m, _mm_setzero_si128());
    dst = _mm256_and_si256(dst, mask);
  }
#else
  static_cast<void>(over_read_in_bytes);
#endif
  return dst;
}

inline __m256i LoadAligned32Msan(const void* const source,
                                 const ptrdiff_t over_read_in_bytes) {
  return MaskOverreads(LoadAligned32(source), over_read_in_bytes);
}

inline void LoadAligned64Msan(const void* const source,
                              const ptrdiff_t over_read_in_bytes,
                              __m256i dst[2]) {
  dst[0] = MaskOverreads(LoadAligned32(source), over_read_in_bytes);
  dst[1] = MaskOverreads(LoadAligned32(static_cast<const __m256i*>(source) + 1),
                         over_read_in_bytes);
}

inline __m256i LoadUnaligned32Msan(const void* const source,
                                   const ptrdiff_t over_read_in_bytes) {
  return MaskOverreads(LoadUnaligned32(source), over_read_in_bytes);
}

//------------------------------------------------------------------------------
// Store functions.

inline void StoreAligned32(void* a, const __m256i v) {
  assert((reinterpret_cast<uintptr_t>(a) & 0x1f) == 0);
  _mm256_store_si256(static_cast<__m256i*>(a), v);
}

inline void StoreAligned64(void* a, const __m256i v[2]) {
  assert((reinterpret_cast<uintptr_t>(a) & 0x1f) == 0);
  _mm256_store_si256(static_cast<__m256i*>(a) + 0, v[0]);
  _mm256_store_si256(static_cast<__m256i*>(a) + 1, v[1]);
}

inline void StoreUnaligned32(void* a, const __m256i v) {
  _mm256_storeu_si256(static_cast<__m256i*>(a), v);
}

//------------------------------------------------------------------------------
// Arithmetic utilities.

inline __m256i RightShiftWithRounding_S16(const __m256i v_val_d, int bits) {
  assert(bits <= 16);
  const __m256i v_bias_d =
      _mm256_set1_epi16(static_cast<int16_t>((1 << bits) >> 1));
  const __m256i v_tmp_d = _mm256_add_epi16(v_val_d, v_bias_d);
  return _mm256_srai_epi16(v_tmp_d, bits);
}

inline __m256i RightShiftWithRounding_S32(const __m256i v_val_d, int bits) {
  const __m256i v_bias_d = _mm256_set1_epi32((1 << bits) >> 1);
  const __m256i v_tmp_d = _mm256_add_epi32(v_val_d, v_bias_d);
  return _mm256_srai_epi32(v_tmp_d, bits);
}
