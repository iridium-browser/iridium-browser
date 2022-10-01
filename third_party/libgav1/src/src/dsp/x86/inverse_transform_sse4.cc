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

#include "src/dsp/inverse_transform.h"
#include "src/utils/cpu.h"

#if LIBGAV1_TARGETING_SSE4_1

#include <smmintrin.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>

#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/dsp/x86/common_sse4.h"
#include "src/dsp/x86/transpose_sse4.h"
#include "src/utils/array_2d.h"
#include "src/utils/common.h"
#include "src/utils/compiler_attributes.h"

namespace libgav1 {
namespace dsp {
namespace low_bitdepth {
namespace {

// Include the constants and utility functions inside the anonymous namespace.
#include "src/dsp/inverse_transform.inc"

template <int store_width, int store_count>
LIBGAV1_ALWAYS_INLINE void StoreDst(int16_t* LIBGAV1_RESTRICT dst,
                                    int32_t stride, int32_t idx,
                                    const __m128i* s) {
  // NOTE: It is expected that the compiler will unroll these loops.
  if (store_width == 16) {
    for (int i = 0; i < store_count; i += 4) {
      StoreUnaligned16(&dst[i * stride + idx], s[i]);
      StoreUnaligned16(&dst[(i + 1) * stride + idx], s[i + 1]);
      StoreUnaligned16(&dst[(i + 2) * stride + idx], s[i + 2]);
      StoreUnaligned16(&dst[(i + 3) * stride + idx], s[i + 3]);
    }
  }
  if (store_width == 8) {
    for (int i = 0; i < store_count; i += 4) {
      StoreLo8(&dst[i * stride + idx], s[i]);
      StoreLo8(&dst[(i + 1) * stride + idx], s[i + 1]);
      StoreLo8(&dst[(i + 2) * stride + idx], s[i + 2]);
      StoreLo8(&dst[(i + 3) * stride + idx], s[i + 3]);
    }
  }
}

template <int load_width, int load_count>
LIBGAV1_ALWAYS_INLINE void LoadSrc(const int16_t* LIBGAV1_RESTRICT src,
                                   int32_t stride, int32_t idx, __m128i* x) {
  // NOTE: It is expected that the compiler will unroll these loops.
  if (load_width == 16) {
    for (int i = 0; i < load_count; i += 4) {
      x[i] = LoadUnaligned16(&src[i * stride + idx]);
      x[i + 1] = LoadUnaligned16(&src[(i + 1) * stride + idx]);
      x[i + 2] = LoadUnaligned16(&src[(i + 2) * stride + idx]);
      x[i + 3] = LoadUnaligned16(&src[(i + 3) * stride + idx]);
    }
  }
  if (load_width == 8) {
    for (int i = 0; i < load_count; i += 4) {
      x[i] = LoadLo8(&src[i * stride + idx]);
      x[i + 1] = LoadLo8(&src[(i + 1) * stride + idx]);
      x[i + 2] = LoadLo8(&src[(i + 2) * stride + idx]);
      x[i + 3] = LoadLo8(&src[(i + 3) * stride + idx]);
    }
  }
}

// Butterfly rotate 4 values.
LIBGAV1_ALWAYS_INLINE void ButterflyRotation_4(__m128i* a, __m128i* b,
                                               const int angle,
                                               const bool flip) {
  const int16_t cos128 = Cos128(angle);
  const int16_t sin128 = Sin128(angle);
  const __m128i psin_pcos = _mm_set1_epi32(
      static_cast<uint16_t>(cos128) | (static_cast<uint32_t>(sin128) << 16));
  const __m128i ba = _mm_unpacklo_epi16(*a, *b);
  const __m128i ab = _mm_unpacklo_epi16(*b, *a);
  const __m128i sign = _mm_set1_epi32(static_cast<int>(0x80000001));
  // -sin cos, -sin cos, -sin cos, -sin cos
  const __m128i msin_pcos = _mm_sign_epi16(psin_pcos, sign);
  const __m128i x0 = _mm_madd_epi16(ba, msin_pcos);
  const __m128i y0 = _mm_madd_epi16(ab, psin_pcos);
  const __m128i x1 = RightShiftWithRounding_S32(x0, 12);
  const __m128i y1 = RightShiftWithRounding_S32(y0, 12);
  const __m128i x = _mm_packs_epi32(x1, x1);
  const __m128i y = _mm_packs_epi32(y1, y1);
  if (flip) {
    *a = y;
    *b = x;
  } else {
    *a = x;
    *b = y;
  }
}

// Butterfly rotate 8 values.
LIBGAV1_ALWAYS_INLINE void ButterflyRotation_8(__m128i* a, __m128i* b,
                                               const int angle,
                                               const bool flip) {
  const int16_t cos128 = Cos128(angle);
  const int16_t sin128 = Sin128(angle);
  const __m128i psin_pcos = _mm_set1_epi32(
      static_cast<uint16_t>(cos128) | (static_cast<uint32_t>(sin128) << 16));
  const __m128i sign = _mm_set1_epi32(static_cast<int>(0x80000001));
  // -sin cos, -sin cos, -sin cos, -sin cos
  const __m128i msin_pcos = _mm_sign_epi16(psin_pcos, sign);
  const __m128i ba = _mm_unpacklo_epi16(*a, *b);
  const __m128i ab = _mm_unpacklo_epi16(*b, *a);
  const __m128i ba_hi = _mm_unpackhi_epi16(*a, *b);
  const __m128i ab_hi = _mm_unpackhi_epi16(*b, *a);
  const __m128i x0 = _mm_madd_epi16(ba, msin_pcos);
  const __m128i y0 = _mm_madd_epi16(ab, psin_pcos);
  const __m128i x0_hi = _mm_madd_epi16(ba_hi, msin_pcos);
  const __m128i y0_hi = _mm_madd_epi16(ab_hi, psin_pcos);
  const __m128i x1 = RightShiftWithRounding_S32(x0, 12);
  const __m128i y1 = RightShiftWithRounding_S32(y0, 12);
  const __m128i x1_hi = RightShiftWithRounding_S32(x0_hi, 12);
  const __m128i y1_hi = RightShiftWithRounding_S32(y0_hi, 12);
  const __m128i x = _mm_packs_epi32(x1, x1_hi);
  const __m128i y = _mm_packs_epi32(y1, y1_hi);
  if (flip) {
    *a = y;
    *b = x;
  } else {
    *a = x;
    *b = y;
  }
}

LIBGAV1_ALWAYS_INLINE void ButterflyRotation_FirstIsZero(__m128i* a, __m128i* b,
                                                         const int angle,
                                                         const bool flip) {
  const int16_t cos128 = Cos128(angle);
  const int16_t sin128 = Sin128(angle);
  const __m128i pcos = _mm_set1_epi16(cos128 << 3);
  const __m128i psin = _mm_set1_epi16(-(sin128 << 3));
  const __m128i x = _mm_mulhrs_epi16(*b, psin);
  const __m128i y = _mm_mulhrs_epi16(*b, pcos);
  if (flip) {
    *a = y;
    *b = x;
  } else {
    *a = x;
    *b = y;
  }
}

LIBGAV1_ALWAYS_INLINE void ButterflyRotation_SecondIsZero(__m128i* a,
                                                          __m128i* b,
                                                          const int angle,
                                                          const bool flip) {
  const int16_t cos128 = Cos128(angle);
  const int16_t sin128 = Sin128(angle);
  const __m128i pcos = _mm_set1_epi16(cos128 << 3);
  const __m128i psin = _mm_set1_epi16(sin128 << 3);
  const __m128i x = _mm_mulhrs_epi16(*a, pcos);
  const __m128i y = _mm_mulhrs_epi16(*a, psin);
  if (flip) {
    *a = y;
    *b = x;
  } else {
    *a = x;
    *b = y;
  }
}

LIBGAV1_ALWAYS_INLINE void HadamardRotation(__m128i* a, __m128i* b, bool flip) {
  __m128i x, y;
  if (flip) {
    y = _mm_adds_epi16(*b, *a);
    x = _mm_subs_epi16(*b, *a);
  } else {
    x = _mm_adds_epi16(*a, *b);
    y = _mm_subs_epi16(*a, *b);
  }
  *a = x;
  *b = y;
}

using ButterflyRotationFunc = void (*)(__m128i* a, __m128i* b, int angle,
                                       bool flip);

LIBGAV1_ALWAYS_INLINE __m128i ShiftResidual(const __m128i residual,
                                            const __m128i v_row_shift_add,
                                            const __m128i v_row_shift) {
  const __m128i k7ffd = _mm_set1_epi16(0x7ffd);
  // The max row_shift is 2, so int16_t values greater than 0x7ffd may
  // overflow.  Generate a mask for this case.
  const __m128i mask = _mm_cmpgt_epi16(residual, k7ffd);
  const __m128i x = _mm_add_epi16(residual, v_row_shift_add);
  // Assume int16_t values.
  const __m128i a = _mm_sra_epi16(x, v_row_shift);
  // Assume uint16_t values.
  const __m128i b = _mm_srl_epi16(x, v_row_shift);
  // Select the correct shifted value.
  return _mm_blendv_epi8(a, b, mask);
}

//------------------------------------------------------------------------------
// Discrete Cosine Transforms (DCT).

template <int width>
LIBGAV1_ALWAYS_INLINE bool DctDcOnly(void* dest, int adjusted_tx_height,
                                     bool should_round, int row_shift) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int16_t*>(dest);
  const __m128i v_src_lo = _mm_shufflelo_epi16(_mm_cvtsi32_si128(dst[0]), 0);
  const __m128i v_src =
      (width == 4) ? v_src_lo : _mm_shuffle_epi32(v_src_lo, 0);
  const __m128i v_mask =
      _mm_set1_epi16(should_round ? static_cast<int16_t>(0xffff) : 0);
  const __m128i v_kTransformRowMultiplier =
      _mm_set1_epi16(kTransformRowMultiplier << 3);
  const __m128i v_src_round =
      _mm_mulhrs_epi16(v_src, v_kTransformRowMultiplier);
  const __m128i s0 = _mm_blendv_epi8(v_src, v_src_round, v_mask);
  const int16_t cos128 = Cos128(32);
  const __m128i xy = _mm_mulhrs_epi16(s0, _mm_set1_epi16(cos128 << 3));

  // Expand to 32 bits to prevent int16_t overflows during the shift add.
  const __m128i v_row_shift_add = _mm_set1_epi32(row_shift);
  const __m128i v_row_shift = _mm_cvtepu32_epi64(v_row_shift_add);
  const __m128i a = _mm_cvtepi16_epi32(xy);
  const __m128i a1 = _mm_cvtepi16_epi32(_mm_srli_si128(xy, 8));
  const __m128i b = _mm_add_epi32(a, v_row_shift_add);
  const __m128i b1 = _mm_add_epi32(a1, v_row_shift_add);
  const __m128i c = _mm_sra_epi32(b, v_row_shift);
  const __m128i c1 = _mm_sra_epi32(b1, v_row_shift);
  const __m128i xy_shifted = _mm_packs_epi32(c, c1);

  if (width == 4) {
    StoreLo8(dst, xy_shifted);
  } else {
    for (int i = 0; i < width; i += 8) {
      StoreUnaligned16(dst, xy_shifted);
      dst += 8;
    }
  }
  return true;
}

template <int height>
LIBGAV1_ALWAYS_INLINE bool DctDcOnlyColumn(void* dest, int adjusted_tx_height,
                                           int width) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int16_t*>(dest);
  const int16_t cos128 = Cos128(32);

  // Calculate dc values for first row.
  if (width == 4) {
    const __m128i v_src = LoadLo8(dst);
    const __m128i xy = _mm_mulhrs_epi16(v_src, _mm_set1_epi16(cos128 << 3));
    StoreLo8(dst, xy);
  } else {
    int i = 0;
    do {
      const __m128i v_src = LoadUnaligned16(&dst[i]);
      const __m128i xy = _mm_mulhrs_epi16(v_src, _mm_set1_epi16(cos128 << 3));
      StoreUnaligned16(&dst[i], xy);
      i += 8;
    } while (i < width);
  }

  // Copy first row to the rest of the block.
  for (int y = 1; y < height; ++y) {
    memcpy(&dst[y * width], dst, width * sizeof(dst[0]));
  }
  return true;
}

template <ButterflyRotationFunc butterfly_rotation,
          bool is_fast_butterfly = false>
LIBGAV1_ALWAYS_INLINE void Dct4Stages(__m128i* s) {
  // stage 12.
  if (is_fast_butterfly) {
    ButterflyRotation_SecondIsZero(&s[0], &s[1], 32, true);
    ButterflyRotation_SecondIsZero(&s[2], &s[3], 48, false);
  } else {
    butterfly_rotation(&s[0], &s[1], 32, true);
    butterfly_rotation(&s[2], &s[3], 48, false);
  }

  // stage 17.
  HadamardRotation(&s[0], &s[3], false);
  HadamardRotation(&s[1], &s[2], false);
}

// Process 4 dct4 rows or columns, depending on the transpose flag.
template <ButterflyRotationFunc butterfly_rotation, bool stage_is_rectangular>
LIBGAV1_ALWAYS_INLINE void Dct4_SSE4_1(void* dest, int32_t step,
                                       bool transpose) {
  auto* const dst = static_cast<int16_t*>(dest);
  __m128i s[4], x[4];

  if (stage_is_rectangular) {
    if (transpose) {
      __m128i input[8];
      LoadSrc<8, 8>(dst, step, 0, input);
      Transpose4x8To8x4_U16(input, x);
    } else {
      LoadSrc<16, 4>(dst, step, 0, x);
    }
  } else {
    LoadSrc<8, 4>(dst, step, 0, x);
    if (transpose) {
      Transpose4x4_U16(x, x);
    }
  }
  // stage 1.
  // kBitReverseLookup 0, 2, 1, 3
  s[0] = x[0];
  s[1] = x[2];
  s[2] = x[1];
  s[3] = x[3];

  Dct4Stages<butterfly_rotation>(s);

  if (stage_is_rectangular) {
    if (transpose) {
      __m128i output[8];
      Transpose8x4To4x8_U16(s, output);
      StoreDst<8, 8>(dst, step, 0, output);
    } else {
      StoreDst<16, 4>(dst, step, 0, s);
    }
  } else {
    if (transpose) {
      Transpose4x4_U16(s, s);
    }
    StoreDst<8, 4>(dst, step, 0, s);
  }
}

template <ButterflyRotationFunc butterfly_rotation,
          bool is_fast_butterfly = false>
LIBGAV1_ALWAYS_INLINE void Dct8Stages(__m128i* s) {
  // stage 8.
  if (is_fast_butterfly) {
    ButterflyRotation_SecondIsZero(&s[4], &s[7], 56, false);
    ButterflyRotation_FirstIsZero(&s[5], &s[6], 24, false);
  } else {
    butterfly_rotation(&s[4], &s[7], 56, false);
    butterfly_rotation(&s[5], &s[6], 24, false);
  }

  // stage 13.
  HadamardRotation(&s[4], &s[5], false);
  HadamardRotation(&s[6], &s[7], true);

  // stage 18.
  butterfly_rotation(&s[6], &s[5], 32, true);

  // stage 22.
  HadamardRotation(&s[0], &s[7], false);
  HadamardRotation(&s[1], &s[6], false);
  HadamardRotation(&s[2], &s[5], false);
  HadamardRotation(&s[3], &s[4], false);
}

// Process dct8 rows or columns, depending on the transpose flag.
template <ButterflyRotationFunc butterfly_rotation, bool stage_is_rectangular>
LIBGAV1_ALWAYS_INLINE void Dct8_SSE4_1(void* dest, int32_t step,
                                       bool transpose) {
  auto* const dst = static_cast<int16_t*>(dest);
  __m128i s[8], x[8];

  if (stage_is_rectangular) {
    if (transpose) {
      __m128i input[4];
      LoadSrc<16, 4>(dst, step, 0, input);
      Transpose8x4To4x8_U16(input, x);
    } else {
      LoadSrc<8, 8>(dst, step, 0, x);
    }
  } else {
    if (transpose) {
      __m128i input[8];
      LoadSrc<16, 8>(dst, step, 0, input);
      Transpose8x8_U16(input, x);
    } else {
      LoadSrc<16, 8>(dst, step, 0, x);
    }
  }

  // stage 1.
  // kBitReverseLookup 0, 4, 2, 6, 1, 5, 3, 7,
  s[0] = x[0];
  s[1] = x[4];
  s[2] = x[2];
  s[3] = x[6];
  s[4] = x[1];
  s[5] = x[5];
  s[6] = x[3];
  s[7] = x[7];

  Dct4Stages<butterfly_rotation>(s);
  Dct8Stages<butterfly_rotation>(s);

  if (stage_is_rectangular) {
    if (transpose) {
      __m128i output[4];
      Transpose4x8To8x4_U16(s, output);
      StoreDst<16, 4>(dst, step, 0, output);
    } else {
      StoreDst<8, 8>(dst, step, 0, s);
    }
  } else {
    if (transpose) {
      __m128i output[8];
      Transpose8x8_U16(s, output);
      StoreDst<16, 8>(dst, step, 0, output);
    } else {
      StoreDst<16, 8>(dst, step, 0, s);
    }
  }
}

template <ButterflyRotationFunc butterfly_rotation,
          bool is_fast_butterfly = false>
LIBGAV1_ALWAYS_INLINE void Dct16Stages(__m128i* s) {
  // stage 5.
  if (is_fast_butterfly) {
    ButterflyRotation_SecondIsZero(&s[8], &s[15], 60, false);
    ButterflyRotation_FirstIsZero(&s[9], &s[14], 28, false);
    ButterflyRotation_SecondIsZero(&s[10], &s[13], 44, false);
    ButterflyRotation_FirstIsZero(&s[11], &s[12], 12, false);
  } else {
    butterfly_rotation(&s[8], &s[15], 60, false);
    butterfly_rotation(&s[9], &s[14], 28, false);
    butterfly_rotation(&s[10], &s[13], 44, false);
    butterfly_rotation(&s[11], &s[12], 12, false);
  }

  // stage 9.
  HadamardRotation(&s[8], &s[9], false);
  HadamardRotation(&s[10], &s[11], true);
  HadamardRotation(&s[12], &s[13], false);
  HadamardRotation(&s[14], &s[15], true);

  // stage 14.
  butterfly_rotation(&s[14], &s[9], 48, true);
  butterfly_rotation(&s[13], &s[10], 112, true);

  // stage 19.
  HadamardRotation(&s[8], &s[11], false);
  HadamardRotation(&s[9], &s[10], false);
  HadamardRotation(&s[12], &s[15], true);
  HadamardRotation(&s[13], &s[14], true);

  // stage 23.
  butterfly_rotation(&s[13], &s[10], 32, true);
  butterfly_rotation(&s[12], &s[11], 32, true);

  // stage 26.
  HadamardRotation(&s[0], &s[15], false);
  HadamardRotation(&s[1], &s[14], false);
  HadamardRotation(&s[2], &s[13], false);
  HadamardRotation(&s[3], &s[12], false);
  HadamardRotation(&s[4], &s[11], false);
  HadamardRotation(&s[5], &s[10], false);
  HadamardRotation(&s[6], &s[9], false);
  HadamardRotation(&s[7], &s[8], false);
}

// Process dct16 rows or columns, depending on the transpose flag.
template <ButterflyRotationFunc butterfly_rotation, bool stage_is_rectangular>
LIBGAV1_ALWAYS_INLINE void Dct16_SSE4_1(void* dest, int32_t step,
                                        bool transpose) {
  auto* const dst = static_cast<int16_t*>(dest);
  __m128i s[16], x[16];

  if (stage_is_rectangular) {
    if (transpose) {
      __m128i input[4];
      LoadSrc<16, 4>(dst, step, 0, input);
      Transpose8x4To4x8_U16(input, x);
      LoadSrc<16, 4>(dst, step, 8, input);
      Transpose8x4To4x8_U16(input, &x[8]);
    } else {
      LoadSrc<8, 16>(dst, step, 0, x);
    }
  } else {
    if (transpose) {
      for (int idx = 0; idx < 16; idx += 8) {
        __m128i input[8];
        LoadSrc<16, 8>(dst, step, idx, input);
        Transpose8x8_U16(input, &x[idx]);
      }
    } else {
      LoadSrc<16, 16>(dst, step, 0, x);
    }
  }

  // stage 1
  // kBitReverseLookup 0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15,
  s[0] = x[0];
  s[1] = x[8];
  s[2] = x[4];
  s[3] = x[12];
  s[4] = x[2];
  s[5] = x[10];
  s[6] = x[6];
  s[7] = x[14];
  s[8] = x[1];
  s[9] = x[9];
  s[10] = x[5];
  s[11] = x[13];
  s[12] = x[3];
  s[13] = x[11];
  s[14] = x[7];
  s[15] = x[15];

  Dct4Stages<butterfly_rotation>(s);
  Dct8Stages<butterfly_rotation>(s);
  Dct16Stages<butterfly_rotation>(s);

  if (stage_is_rectangular) {
    if (transpose) {
      __m128i output[4];
      Transpose4x8To8x4_U16(s, output);
      StoreDst<16, 4>(dst, step, 0, output);
      Transpose4x8To8x4_U16(&s[8], output);
      StoreDst<16, 4>(dst, step, 8, output);
    } else {
      StoreDst<8, 16>(dst, step, 0, s);
    }
  } else {
    if (transpose) {
      for (int idx = 0; idx < 16; idx += 8) {
        __m128i output[8];
        Transpose8x8_U16(&s[idx], output);
        StoreDst<16, 8>(dst, step, idx, output);
      }
    } else {
      StoreDst<16, 16>(dst, step, 0, s);
    }
  }
}

template <ButterflyRotationFunc butterfly_rotation,
          bool is_fast_butterfly = false>
LIBGAV1_ALWAYS_INLINE void Dct32Stages(__m128i* s) {
  // stage 3
  if (is_fast_butterfly) {
    ButterflyRotation_SecondIsZero(&s[16], &s[31], 62, false);
    ButterflyRotation_FirstIsZero(&s[17], &s[30], 30, false);
    ButterflyRotation_SecondIsZero(&s[18], &s[29], 46, false);
    ButterflyRotation_FirstIsZero(&s[19], &s[28], 14, false);
    ButterflyRotation_SecondIsZero(&s[20], &s[27], 54, false);
    ButterflyRotation_FirstIsZero(&s[21], &s[26], 22, false);
    ButterflyRotation_SecondIsZero(&s[22], &s[25], 38, false);
    ButterflyRotation_FirstIsZero(&s[23], &s[24], 6, false);
  } else {
    butterfly_rotation(&s[16], &s[31], 62, false);
    butterfly_rotation(&s[17], &s[30], 30, false);
    butterfly_rotation(&s[18], &s[29], 46, false);
    butterfly_rotation(&s[19], &s[28], 14, false);
    butterfly_rotation(&s[20], &s[27], 54, false);
    butterfly_rotation(&s[21], &s[26], 22, false);
    butterfly_rotation(&s[22], &s[25], 38, false);
    butterfly_rotation(&s[23], &s[24], 6, false);
  }
  // stage 6.
  HadamardRotation(&s[16], &s[17], false);
  HadamardRotation(&s[18], &s[19], true);
  HadamardRotation(&s[20], &s[21], false);
  HadamardRotation(&s[22], &s[23], true);
  HadamardRotation(&s[24], &s[25], false);
  HadamardRotation(&s[26], &s[27], true);
  HadamardRotation(&s[28], &s[29], false);
  HadamardRotation(&s[30], &s[31], true);

  // stage 10.
  butterfly_rotation(&s[30], &s[17], 24 + 32, true);
  butterfly_rotation(&s[29], &s[18], 24 + 64 + 32, true);
  butterfly_rotation(&s[26], &s[21], 24, true);
  butterfly_rotation(&s[25], &s[22], 24 + 64, true);

  // stage 15.
  HadamardRotation(&s[16], &s[19], false);
  HadamardRotation(&s[17], &s[18], false);
  HadamardRotation(&s[20], &s[23], true);
  HadamardRotation(&s[21], &s[22], true);
  HadamardRotation(&s[24], &s[27], false);
  HadamardRotation(&s[25], &s[26], false);
  HadamardRotation(&s[28], &s[31], true);
  HadamardRotation(&s[29], &s[30], true);

  // stage 20.
  butterfly_rotation(&s[29], &s[18], 48, true);
  butterfly_rotation(&s[28], &s[19], 48, true);
  butterfly_rotation(&s[27], &s[20], 48 + 64, true);
  butterfly_rotation(&s[26], &s[21], 48 + 64, true);

  // stage 24.
  HadamardRotation(&s[16], &s[23], false);
  HadamardRotation(&s[17], &s[22], false);
  HadamardRotation(&s[18], &s[21], false);
  HadamardRotation(&s[19], &s[20], false);
  HadamardRotation(&s[24], &s[31], true);
  HadamardRotation(&s[25], &s[30], true);
  HadamardRotation(&s[26], &s[29], true);
  HadamardRotation(&s[27], &s[28], true);

  // stage 27.
  butterfly_rotation(&s[27], &s[20], 32, true);
  butterfly_rotation(&s[26], &s[21], 32, true);
  butterfly_rotation(&s[25], &s[22], 32, true);
  butterfly_rotation(&s[24], &s[23], 32, true);

  // stage 29.
  HadamardRotation(&s[0], &s[31], false);
  HadamardRotation(&s[1], &s[30], false);
  HadamardRotation(&s[2], &s[29], false);
  HadamardRotation(&s[3], &s[28], false);
  HadamardRotation(&s[4], &s[27], false);
  HadamardRotation(&s[5], &s[26], false);
  HadamardRotation(&s[6], &s[25], false);
  HadamardRotation(&s[7], &s[24], false);
  HadamardRotation(&s[8], &s[23], false);
  HadamardRotation(&s[9], &s[22], false);
  HadamardRotation(&s[10], &s[21], false);
  HadamardRotation(&s[11], &s[20], false);
  HadamardRotation(&s[12], &s[19], false);
  HadamardRotation(&s[13], &s[18], false);
  HadamardRotation(&s[14], &s[17], false);
  HadamardRotation(&s[15], &s[16], false);
}

// Process dct32 rows or columns, depending on the transpose flag.
LIBGAV1_ALWAYS_INLINE void Dct32_SSE4_1(void* dest, const int32_t step,
                                        const bool transpose) {
  auto* const dst = static_cast<int16_t*>(dest);
  __m128i s[32], x[32];

  if (transpose) {
    for (int idx = 0; idx < 32; idx += 8) {
      __m128i input[8];
      LoadSrc<16, 8>(dst, step, idx, input);
      Transpose8x8_U16(input, &x[idx]);
    }
  } else {
    LoadSrc<16, 32>(dst, step, 0, x);
  }

  // stage 1
  // kBitReverseLookup
  // 0, 16, 8, 24, 4, 20, 12, 28, 2, 18, 10, 26, 6, 22, 14, 30,
  s[0] = x[0];
  s[1] = x[16];
  s[2] = x[8];
  s[3] = x[24];
  s[4] = x[4];
  s[5] = x[20];
  s[6] = x[12];
  s[7] = x[28];
  s[8] = x[2];
  s[9] = x[18];
  s[10] = x[10];
  s[11] = x[26];
  s[12] = x[6];
  s[13] = x[22];
  s[14] = x[14];
  s[15] = x[30];

  // 1, 17, 9, 25, 5, 21, 13, 29, 3, 19, 11, 27, 7, 23, 15, 31,
  s[16] = x[1];
  s[17] = x[17];
  s[18] = x[9];
  s[19] = x[25];
  s[20] = x[5];
  s[21] = x[21];
  s[22] = x[13];
  s[23] = x[29];
  s[24] = x[3];
  s[25] = x[19];
  s[26] = x[11];
  s[27] = x[27];
  s[28] = x[7];
  s[29] = x[23];
  s[30] = x[15];
  s[31] = x[31];

  Dct4Stages<ButterflyRotation_8>(s);
  Dct8Stages<ButterflyRotation_8>(s);
  Dct16Stages<ButterflyRotation_8>(s);
  Dct32Stages<ButterflyRotation_8>(s);

  if (transpose) {
    for (int idx = 0; idx < 32; idx += 8) {
      __m128i output[8];
      Transpose8x8_U16(&s[idx], output);
      StoreDst<16, 8>(dst, step, idx, output);
    }
  } else {
    StoreDst<16, 32>(dst, step, 0, s);
  }
}

// Allow the compiler to call this function instead of force inlining. Tests
// show the performance is slightly faster.
void Dct64_SSE4_1(void* dest, int32_t step, bool transpose) {
  auto* const dst = static_cast<int16_t*>(dest);
  __m128i s[64], x[32];

  if (transpose) {
    // The last 32 values of every row are always zero if the |tx_width| is
    // 64.
    for (int idx = 0; idx < 32; idx += 8) {
      __m128i input[8];
      LoadSrc<16, 8>(dst, step, idx, input);
      Transpose8x8_U16(input, &x[idx]);
    }
  } else {
    // The last 32 values of every column are always zero if the |tx_height| is
    // 64.
    LoadSrc<16, 32>(dst, step, 0, x);
  }

  // stage 1
  // kBitReverseLookup
  // 0, 32, 16, 48, 8, 40, 24, 56, 4, 36, 20, 52, 12, 44, 28, 60,
  s[0] = x[0];
  s[2] = x[16];
  s[4] = x[8];
  s[6] = x[24];
  s[8] = x[4];
  s[10] = x[20];
  s[12] = x[12];
  s[14] = x[28];

  // 2, 34, 18, 50, 10, 42, 26, 58, 6, 38, 22, 54, 14, 46, 30, 62,
  s[16] = x[2];
  s[18] = x[18];
  s[20] = x[10];
  s[22] = x[26];
  s[24] = x[6];
  s[26] = x[22];
  s[28] = x[14];
  s[30] = x[30];

  // 1, 33, 17, 49, 9, 41, 25, 57, 5, 37, 21, 53, 13, 45, 29, 61,
  s[32] = x[1];
  s[34] = x[17];
  s[36] = x[9];
  s[38] = x[25];
  s[40] = x[5];
  s[42] = x[21];
  s[44] = x[13];
  s[46] = x[29];

  // 3, 35, 19, 51, 11, 43, 27, 59, 7, 39, 23, 55, 15, 47, 31, 63
  s[48] = x[3];
  s[50] = x[19];
  s[52] = x[11];
  s[54] = x[27];
  s[56] = x[7];
  s[58] = x[23];
  s[60] = x[15];
  s[62] = x[31];

  Dct4Stages<ButterflyRotation_8, /*is_fast_butterfly=*/true>(s);
  Dct8Stages<ButterflyRotation_8, /*is_fast_butterfly=*/true>(s);
  Dct16Stages<ButterflyRotation_8, /*is_fast_butterfly=*/true>(s);
  Dct32Stages<ButterflyRotation_8, /*is_fast_butterfly=*/true>(s);

  //-- start dct 64 stages
  // stage 2.
  ButterflyRotation_SecondIsZero(&s[32], &s[63], 63 - 0, false);
  ButterflyRotation_FirstIsZero(&s[33], &s[62], 63 - 32, false);
  ButterflyRotation_SecondIsZero(&s[34], &s[61], 63 - 16, false);
  ButterflyRotation_FirstIsZero(&s[35], &s[60], 63 - 48, false);
  ButterflyRotation_SecondIsZero(&s[36], &s[59], 63 - 8, false);
  ButterflyRotation_FirstIsZero(&s[37], &s[58], 63 - 40, false);
  ButterflyRotation_SecondIsZero(&s[38], &s[57], 63 - 24, false);
  ButterflyRotation_FirstIsZero(&s[39], &s[56], 63 - 56, false);
  ButterflyRotation_SecondIsZero(&s[40], &s[55], 63 - 4, false);
  ButterflyRotation_FirstIsZero(&s[41], &s[54], 63 - 36, false);
  ButterflyRotation_SecondIsZero(&s[42], &s[53], 63 - 20, false);
  ButterflyRotation_FirstIsZero(&s[43], &s[52], 63 - 52, false);
  ButterflyRotation_SecondIsZero(&s[44], &s[51], 63 - 12, false);
  ButterflyRotation_FirstIsZero(&s[45], &s[50], 63 - 44, false);
  ButterflyRotation_SecondIsZero(&s[46], &s[49], 63 - 28, false);
  ButterflyRotation_FirstIsZero(&s[47], &s[48], 63 - 60, false);

  // stage 4.
  HadamardRotation(&s[32], &s[33], false);
  HadamardRotation(&s[34], &s[35], true);
  HadamardRotation(&s[36], &s[37], false);
  HadamardRotation(&s[38], &s[39], true);
  HadamardRotation(&s[40], &s[41], false);
  HadamardRotation(&s[42], &s[43], true);
  HadamardRotation(&s[44], &s[45], false);
  HadamardRotation(&s[46], &s[47], true);
  HadamardRotation(&s[48], &s[49], false);
  HadamardRotation(&s[50], &s[51], true);
  HadamardRotation(&s[52], &s[53], false);
  HadamardRotation(&s[54], &s[55], true);
  HadamardRotation(&s[56], &s[57], false);
  HadamardRotation(&s[58], &s[59], true);
  HadamardRotation(&s[60], &s[61], false);
  HadamardRotation(&s[62], &s[63], true);

  // stage 7.
  ButterflyRotation_8(&s[62], &s[33], 60 - 0, true);
  ButterflyRotation_8(&s[61], &s[34], 60 - 0 + 64, true);
  ButterflyRotation_8(&s[58], &s[37], 60 - 32, true);
  ButterflyRotation_8(&s[57], &s[38], 60 - 32 + 64, true);
  ButterflyRotation_8(&s[54], &s[41], 60 - 16, true);
  ButterflyRotation_8(&s[53], &s[42], 60 - 16 + 64, true);
  ButterflyRotation_8(&s[50], &s[45], 60 - 48, true);
  ButterflyRotation_8(&s[49], &s[46], 60 - 48 + 64, true);

  // stage 11.
  HadamardRotation(&s[32], &s[35], false);
  HadamardRotation(&s[33], &s[34], false);
  HadamardRotation(&s[36], &s[39], true);
  HadamardRotation(&s[37], &s[38], true);
  HadamardRotation(&s[40], &s[43], false);
  HadamardRotation(&s[41], &s[42], false);
  HadamardRotation(&s[44], &s[47], true);
  HadamardRotation(&s[45], &s[46], true);
  HadamardRotation(&s[48], &s[51], false);
  HadamardRotation(&s[49], &s[50], false);
  HadamardRotation(&s[52], &s[55], true);
  HadamardRotation(&s[53], &s[54], true);
  HadamardRotation(&s[56], &s[59], false);
  HadamardRotation(&s[57], &s[58], false);
  HadamardRotation(&s[60], &s[63], true);
  HadamardRotation(&s[61], &s[62], true);

  // stage 16.
  ButterflyRotation_8(&s[61], &s[34], 56, true);
  ButterflyRotation_8(&s[60], &s[35], 56, true);
  ButterflyRotation_8(&s[59], &s[36], 56 + 64, true);
  ButterflyRotation_8(&s[58], &s[37], 56 + 64, true);
  ButterflyRotation_8(&s[53], &s[42], 56 - 32, true);
  ButterflyRotation_8(&s[52], &s[43], 56 - 32, true);
  ButterflyRotation_8(&s[51], &s[44], 56 - 32 + 64, true);
  ButterflyRotation_8(&s[50], &s[45], 56 - 32 + 64, true);

  // stage 21.
  HadamardRotation(&s[32], &s[39], false);
  HadamardRotation(&s[33], &s[38], false);
  HadamardRotation(&s[34], &s[37], false);
  HadamardRotation(&s[35], &s[36], false);
  HadamardRotation(&s[40], &s[47], true);
  HadamardRotation(&s[41], &s[46], true);
  HadamardRotation(&s[42], &s[45], true);
  HadamardRotation(&s[43], &s[44], true);
  HadamardRotation(&s[48], &s[55], false);
  HadamardRotation(&s[49], &s[54], false);
  HadamardRotation(&s[50], &s[53], false);
  HadamardRotation(&s[51], &s[52], false);
  HadamardRotation(&s[56], &s[63], true);
  HadamardRotation(&s[57], &s[62], true);
  HadamardRotation(&s[58], &s[61], true);
  HadamardRotation(&s[59], &s[60], true);

  // stage 25.
  ButterflyRotation_8(&s[59], &s[36], 48, true);
  ButterflyRotation_8(&s[58], &s[37], 48, true);
  ButterflyRotation_8(&s[57], &s[38], 48, true);
  ButterflyRotation_8(&s[56], &s[39], 48, true);
  ButterflyRotation_8(&s[55], &s[40], 112, true);
  ButterflyRotation_8(&s[54], &s[41], 112, true);
  ButterflyRotation_8(&s[53], &s[42], 112, true);
  ButterflyRotation_8(&s[52], &s[43], 112, true);

  // stage 28.
  HadamardRotation(&s[32], &s[47], false);
  HadamardRotation(&s[33], &s[46], false);
  HadamardRotation(&s[34], &s[45], false);
  HadamardRotation(&s[35], &s[44], false);
  HadamardRotation(&s[36], &s[43], false);
  HadamardRotation(&s[37], &s[42], false);
  HadamardRotation(&s[38], &s[41], false);
  HadamardRotation(&s[39], &s[40], false);
  HadamardRotation(&s[48], &s[63], true);
  HadamardRotation(&s[49], &s[62], true);
  HadamardRotation(&s[50], &s[61], true);
  HadamardRotation(&s[51], &s[60], true);
  HadamardRotation(&s[52], &s[59], true);
  HadamardRotation(&s[53], &s[58], true);
  HadamardRotation(&s[54], &s[57], true);
  HadamardRotation(&s[55], &s[56], true);

  // stage 30.
  ButterflyRotation_8(&s[55], &s[40], 32, true);
  ButterflyRotation_8(&s[54], &s[41], 32, true);
  ButterflyRotation_8(&s[53], &s[42], 32, true);
  ButterflyRotation_8(&s[52], &s[43], 32, true);
  ButterflyRotation_8(&s[51], &s[44], 32, true);
  ButterflyRotation_8(&s[50], &s[45], 32, true);
  ButterflyRotation_8(&s[49], &s[46], 32, true);
  ButterflyRotation_8(&s[48], &s[47], 32, true);

  // stage 31.
  for (int i = 0; i < 32; i += 4) {
    HadamardRotation(&s[i], &s[63 - i], false);
    HadamardRotation(&s[i + 1], &s[63 - i - 1], false);
    HadamardRotation(&s[i + 2], &s[63 - i - 2], false);
    HadamardRotation(&s[i + 3], &s[63 - i - 3], false);
  }
  //-- end dct 64 stages

  if (transpose) {
    for (int idx = 0; idx < 64; idx += 8) {
      __m128i output[8];
      Transpose8x8_U16(&s[idx], output);
      StoreDst<16, 8>(dst, step, idx, output);
    }
  } else {
    StoreDst<16, 64>(dst, step, 0, s);
  }
}

//------------------------------------------------------------------------------
// Asymmetric Discrete Sine Transforms (ADST).

template <bool stage_is_rectangular>
LIBGAV1_ALWAYS_INLINE void Adst4_SSE4_1(void* dest, int32_t step,
                                        bool transpose) {
  auto* const dst = static_cast<int16_t*>(dest);
  __m128i s[8], x[4];

  if (stage_is_rectangular) {
    if (transpose) {
      __m128i input[8];
      LoadSrc<8, 8>(dst, step, 0, input);
      Transpose4x8To8x4_U16(input, x);
    } else {
      LoadSrc<16, 4>(dst, step, 0, x);
    }
  } else {
    LoadSrc<8, 4>(dst, step, 0, x);
    if (transpose) {
      Transpose4x4_U16(x, x);
    }
  }

  const __m128i kAdst4Multiplier_1 = _mm_set1_epi16(kAdst4Multiplier[1]);
  const __m128i kAdst4Multiplier_2 = _mm_set1_epi16(kAdst4Multiplier[2]);
  const __m128i kAdst4Multiplier_3 = _mm_set1_epi16(kAdst4Multiplier[3]);
  const __m128i kAdst4Multiplier_m0_1 =
      _mm_set1_epi32(static_cast<uint16_t>(kAdst4Multiplier[1]) |
                     (static_cast<uint32_t>(-kAdst4Multiplier[0]) << 16));
  const __m128i kAdst4Multiplier_3_0 =
      _mm_set1_epi32(static_cast<uint16_t>(kAdst4Multiplier[0]) |
                     (static_cast<uint32_t>(kAdst4Multiplier[3]) << 16));

  // stage 1.
  const __m128i x3_x0 = _mm_unpacklo_epi16(x[0], x[3]);
  const __m128i x2_x0 = _mm_unpacklo_epi16(x[0], x[2]);
  const __m128i zero_x1 = _mm_cvtepu16_epi32(x[1]);
  const __m128i zero_x2 = _mm_cvtepu16_epi32(x[2]);
  const __m128i zero_x3 = _mm_cvtepu16_epi32(x[3]);

  s[5] = _mm_madd_epi16(zero_x3, kAdst4Multiplier_1);
  s[6] = _mm_madd_epi16(zero_x3, kAdst4Multiplier_3);

  // stage 2.
  // ((src[0] - src[2]) + src[3]) * kAdst4Multiplier[2]
  const __m128i k2_x3_x0 = _mm_madd_epi16(x3_x0, kAdst4Multiplier_2);
  const __m128i k2_zero_x2 = _mm_madd_epi16(zero_x2, kAdst4Multiplier_2);
  const __m128i b7 = _mm_sub_epi32(k2_x3_x0, k2_zero_x2);

  // stage 3.
  s[0] = _mm_madd_epi16(x2_x0, kAdst4Multiplier_3_0);
  s[1] = _mm_madd_epi16(x2_x0, kAdst4Multiplier_m0_1);
  s[2] = b7;
  s[3] = _mm_madd_epi16(zero_x1, kAdst4Multiplier_2);

  // stage 4.
  s[0] = _mm_add_epi32(s[0], s[5]);
  s[1] = _mm_sub_epi32(s[1], s[6]);

  // stages 5 and 6.
  x[0] = _mm_add_epi32(s[0], s[3]);
  x[1] = _mm_add_epi32(s[1], s[3]);
  x[2] = _mm_add_epi32(s[0], s[1]);
  x[3] = _mm_sub_epi32(x[2], s[3]);

  x[0] = RightShiftWithRounding_S32(x[0], 12);
  x[1] = RightShiftWithRounding_S32(x[1], 12);
  x[2] = RightShiftWithRounding_S32(s[2], 12);
  x[3] = RightShiftWithRounding_S32(x[3], 12);

  x[0] = _mm_packs_epi32(x[0], x[1]);
  x[2] = _mm_packs_epi32(x[2], x[3]);
  x[1] = _mm_srli_si128(x[0], 8);
  x[3] = _mm_srli_si128(x[2], 8);

  if (stage_is_rectangular) {
    if (transpose) {
      __m128i output[8];
      Transpose8x4To4x8_U16(x, output);
      StoreDst<8, 8>(dst, step, 0, output);
    } else {
      StoreDst<16, 4>(dst, step, 0, x);
    }
  } else {
    if (transpose) {
      Transpose4x4_U16(x, x);
    }
    StoreDst<8, 4>(dst, step, 0, x);
  }
}

constexpr int16_t kAdst4DcOnlyMultiplier[8] = {1321, 0, 2482, 0,
                                               3344, 0, 2482, 1321};

LIBGAV1_ALWAYS_INLINE bool Adst4DcOnly(void* dest, int adjusted_tx_height,
                                       bool should_round, int row_shift) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int16_t*>(dest);
  const __m128i v_src =
      _mm_shuffle_epi32(_mm_shufflelo_epi16(_mm_cvtsi32_si128(dst[0]), 0), 0);
  const __m128i v_mask =
      _mm_set1_epi16(should_round ? static_cast<int16_t>(0xffff) : 0);
  const __m128i v_kTransformRowMultiplier =
      _mm_set1_epi16(kTransformRowMultiplier << 3);
  const __m128i v_src_round =
      _mm_mulhrs_epi16(v_src, v_kTransformRowMultiplier);
  const __m128i s0 = _mm_blendv_epi8(v_src, v_src_round, v_mask);
  const __m128i v_kAdst4DcOnlyMultipliers =
      LoadUnaligned16(kAdst4DcOnlyMultiplier);
  // s0*k0 s0*k1 s0*k2 s0*k1
  // +
  // s0*0  s0*0  s0*0  s0*k0
  const __m128i x3 = _mm_madd_epi16(s0, v_kAdst4DcOnlyMultipliers);
  const __m128i dst_0 = RightShiftWithRounding_S32(x3, 12);
  const __m128i v_row_shift_add = _mm_set1_epi32(row_shift);
  const __m128i v_row_shift = _mm_cvtepu32_epi64(v_row_shift_add);
  const __m128i a = _mm_add_epi32(dst_0, v_row_shift_add);
  const __m128i b = _mm_sra_epi32(a, v_row_shift);
  const __m128i c = _mm_packs_epi32(b, b);
  StoreLo8(dst, c);

  return true;
}

LIBGAV1_ALWAYS_INLINE bool Adst4DcOnlyColumn(void* dest, int adjusted_tx_height,
                                             int width) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int16_t*>(dest);
  int i = 0;
  do {
    const __m128i v_src = _mm_cvtepi16_epi32(LoadLo8(&dst[i]));
    const __m128i kAdst4Multiplier_0 = _mm_set1_epi32(kAdst4Multiplier[0]);
    const __m128i kAdst4Multiplier_1 = _mm_set1_epi32(kAdst4Multiplier[1]);
    const __m128i kAdst4Multiplier_2 = _mm_set1_epi32(kAdst4Multiplier[2]);
    const __m128i s0 = _mm_mullo_epi32(kAdst4Multiplier_0, v_src);
    const __m128i s1 = _mm_mullo_epi32(kAdst4Multiplier_1, v_src);
    const __m128i s2 = _mm_mullo_epi32(kAdst4Multiplier_2, v_src);
    const __m128i x0 = s0;
    const __m128i x1 = s1;
    const __m128i x2 = s2;
    const __m128i x3 = _mm_add_epi32(s0, s1);
    const __m128i dst_0 = RightShiftWithRounding_S32(x0, 12);
    const __m128i dst_1 = RightShiftWithRounding_S32(x1, 12);
    const __m128i dst_2 = RightShiftWithRounding_S32(x2, 12);
    const __m128i dst_3 = RightShiftWithRounding_S32(x3, 12);
    const __m128i dst_0_1 = _mm_packs_epi32(dst_0, dst_1);
    const __m128i dst_2_3 = _mm_packs_epi32(dst_2, dst_3);
    StoreLo8(&dst[i], dst_0_1);
    StoreHi8(&dst[i + width * 1], dst_0_1);
    StoreLo8(&dst[i + width * 2], dst_2_3);
    StoreHi8(&dst[i + width * 3], dst_2_3);
    i += 4;
  } while (i < width);

  return true;
}

template <ButterflyRotationFunc butterfly_rotation, bool stage_is_rectangular>
LIBGAV1_ALWAYS_INLINE void Adst8_SSE4_1(void* dest, int32_t step,
                                        bool transpose) {
  auto* const dst = static_cast<int16_t*>(dest);
  __m128i s[8], x[8];

  if (stage_is_rectangular) {
    if (transpose) {
      __m128i input[4];
      LoadSrc<16, 4>(dst, step, 0, input);
      Transpose8x4To4x8_U16(input, x);
    } else {
      LoadSrc<8, 8>(dst, step, 0, x);
    }
  } else {
    if (transpose) {
      __m128i input[8];
      LoadSrc<16, 8>(dst, step, 0, input);
      Transpose8x8_U16(input, x);
    } else {
      LoadSrc<16, 8>(dst, step, 0, x);
    }
  }

  // stage 1.
  s[0] = x[7];
  s[1] = x[0];
  s[2] = x[5];
  s[3] = x[2];
  s[4] = x[3];
  s[5] = x[4];
  s[6] = x[1];
  s[7] = x[6];

  // stage 2.
  butterfly_rotation(&s[0], &s[1], 60 - 0, true);
  butterfly_rotation(&s[2], &s[3], 60 - 16, true);
  butterfly_rotation(&s[4], &s[5], 60 - 32, true);
  butterfly_rotation(&s[6], &s[7], 60 - 48, true);

  // stage 3.
  HadamardRotation(&s[0], &s[4], false);
  HadamardRotation(&s[1], &s[5], false);
  HadamardRotation(&s[2], &s[6], false);
  HadamardRotation(&s[3], &s[7], false);

  // stage 4.
  butterfly_rotation(&s[4], &s[5], 48 - 0, true);
  butterfly_rotation(&s[7], &s[6], 48 - 32, true);

  // stage 5.
  HadamardRotation(&s[0], &s[2], false);
  HadamardRotation(&s[4], &s[6], false);
  HadamardRotation(&s[1], &s[3], false);
  HadamardRotation(&s[5], &s[7], false);

  // stage 6.
  butterfly_rotation(&s[2], &s[3], 32, true);
  butterfly_rotation(&s[6], &s[7], 32, true);

  // stage 7.
  const __m128i v_zero = _mm_setzero_si128();
  x[0] = s[0];
  x[1] = _mm_subs_epi16(v_zero, s[4]);
  x[2] = s[6];
  x[3] = _mm_subs_epi16(v_zero, s[2]);
  x[4] = s[3];
  x[5] = _mm_subs_epi16(v_zero, s[7]);
  x[6] = s[5];
  x[7] = _mm_subs_epi16(v_zero, s[1]);

  if (stage_is_rectangular) {
    if (transpose) {
      __m128i output[4];
      Transpose4x8To8x4_U16(x, output);
      StoreDst<16, 4>(dst, step, 0, output);
    } else {
      StoreDst<8, 8>(dst, step, 0, x);
    }
  } else {
    if (transpose) {
      __m128i output[8];
      Transpose8x8_U16(x, output);
      StoreDst<16, 8>(dst, step, 0, output);
    } else {
      StoreDst<16, 8>(dst, step, 0, x);
    }
  }
}

LIBGAV1_ALWAYS_INLINE bool Adst8DcOnly(void* dest, int adjusted_tx_height,
                                       bool should_round, int row_shift) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int16_t*>(dest);
  __m128i s[8];

  const __m128i v_src = _mm_shufflelo_epi16(_mm_cvtsi32_si128(dst[0]), 0);
  const __m128i v_mask =
      _mm_set1_epi16(should_round ? static_cast<int16_t>(0xffff) : 0);
  const __m128i v_kTransformRowMultiplier =
      _mm_set1_epi16(kTransformRowMultiplier << 3);
  const __m128i v_src_round =
      _mm_mulhrs_epi16(v_src, v_kTransformRowMultiplier);
  // stage 1.
  s[1] = _mm_blendv_epi8(v_src, v_src_round, v_mask);

  // stage 2.
  ButterflyRotation_FirstIsZero(&s[0], &s[1], 60, true);

  // stage 3.
  s[4] = s[0];
  s[5] = s[1];

  // stage 4.
  ButterflyRotation_4(&s[4], &s[5], 48, true);

  // stage 5.
  s[2] = s[0];
  s[3] = s[1];
  s[6] = s[4];
  s[7] = s[5];

  // stage 6.
  ButterflyRotation_4(&s[2], &s[3], 32, true);
  ButterflyRotation_4(&s[6], &s[7], 32, true);

  // stage 7.
  __m128i x[8];
  const __m128i v_zero = _mm_setzero_si128();
  x[0] = s[0];
  x[1] = _mm_subs_epi16(v_zero, s[4]);
  x[2] = s[6];
  x[3] = _mm_subs_epi16(v_zero, s[2]);
  x[4] = s[3];
  x[5] = _mm_subs_epi16(v_zero, s[7]);
  x[6] = s[5];
  x[7] = _mm_subs_epi16(v_zero, s[1]);

  const __m128i x1_x0 = _mm_unpacklo_epi16(x[0], x[1]);
  const __m128i x3_x2 = _mm_unpacklo_epi16(x[2], x[3]);
  const __m128i x5_x4 = _mm_unpacklo_epi16(x[4], x[5]);
  const __m128i x7_x6 = _mm_unpacklo_epi16(x[6], x[7]);
  const __m128i x3_x0 = _mm_unpacklo_epi32(x1_x0, x3_x2);
  const __m128i x7_x4 = _mm_unpacklo_epi32(x5_x4, x7_x6);

  const __m128i v_row_shift_add = _mm_set1_epi32(row_shift);
  const __m128i v_row_shift = _mm_cvtepu32_epi64(v_row_shift_add);
  const __m128i a = _mm_add_epi32(_mm_cvtepi16_epi32(x3_x0), v_row_shift_add);
  const __m128i a1 = _mm_add_epi32(_mm_cvtepi16_epi32(x7_x4), v_row_shift_add);
  const __m128i b = _mm_sra_epi32(a, v_row_shift);
  const __m128i b1 = _mm_sra_epi32(a1, v_row_shift);
  StoreUnaligned16(dst, _mm_packs_epi32(b, b1));

  return true;
}

LIBGAV1_ALWAYS_INLINE bool Adst8DcOnlyColumn(void* dest, int adjusted_tx_height,
                                             int width) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int16_t*>(dest);
  __m128i s[8];

  int i = 0;
  do {
    const __m128i v_src = LoadLo8(dst);
    // stage 1.
    s[1] = v_src;

    // stage 2.
    ButterflyRotation_FirstIsZero(&s[0], &s[1], 60, true);

    // stage 3.
    s[4] = s[0];
    s[5] = s[1];

    // stage 4.
    ButterflyRotation_4(&s[4], &s[5], 48, true);

    // stage 5.
    s[2] = s[0];
    s[3] = s[1];
    s[6] = s[4];
    s[7] = s[5];

    // stage 6.
    ButterflyRotation_4(&s[2], &s[3], 32, true);
    ButterflyRotation_4(&s[6], &s[7], 32, true);

    // stage 7.
    __m128i x[8];
    const __m128i v_zero = _mm_setzero_si128();
    x[0] = s[0];
    x[1] = _mm_subs_epi16(v_zero, s[4]);
    x[2] = s[6];
    x[3] = _mm_subs_epi16(v_zero, s[2]);
    x[4] = s[3];
    x[5] = _mm_subs_epi16(v_zero, s[7]);
    x[6] = s[5];
    x[7] = _mm_subs_epi16(v_zero, s[1]);

    for (int j = 0; j < 8; ++j) {
      StoreLo8(&dst[j * width], x[j]);
    }
    i += 4;
    dst += 4;
  } while (i < width);

  return true;
}

template <ButterflyRotationFunc butterfly_rotation, bool stage_is_rectangular>
LIBGAV1_ALWAYS_INLINE void Adst16_SSE4_1(void* dest, int32_t step,
                                         bool transpose) {
  auto* const dst = static_cast<int16_t*>(dest);
  __m128i s[16], x[16];

  if (stage_is_rectangular) {
    if (transpose) {
      __m128i input[4];
      LoadSrc<16, 4>(dst, step, 0, input);
      Transpose8x4To4x8_U16(input, x);
      LoadSrc<16, 4>(dst, step, 8, input);
      Transpose8x4To4x8_U16(input, &x[8]);
    } else {
      LoadSrc<8, 16>(dst, step, 0, x);
    }
  } else {
    if (transpose) {
      for (int idx = 0; idx < 16; idx += 8) {
        __m128i input[8];
        LoadSrc<16, 8>(dst, step, idx, input);
        Transpose8x8_U16(input, &x[idx]);
      }
    } else {
      LoadSrc<16, 16>(dst, step, 0, x);
    }
  }

  // stage 1.
  s[0] = x[15];
  s[1] = x[0];
  s[2] = x[13];
  s[3] = x[2];
  s[4] = x[11];
  s[5] = x[4];
  s[6] = x[9];
  s[7] = x[6];
  s[8] = x[7];
  s[9] = x[8];
  s[10] = x[5];
  s[11] = x[10];
  s[12] = x[3];
  s[13] = x[12];
  s[14] = x[1];
  s[15] = x[14];

  // stage 2.
  butterfly_rotation(&s[0], &s[1], 62 - 0, true);
  butterfly_rotation(&s[2], &s[3], 62 - 8, true);
  butterfly_rotation(&s[4], &s[5], 62 - 16, true);
  butterfly_rotation(&s[6], &s[7], 62 - 24, true);
  butterfly_rotation(&s[8], &s[9], 62 - 32, true);
  butterfly_rotation(&s[10], &s[11], 62 - 40, true);
  butterfly_rotation(&s[12], &s[13], 62 - 48, true);
  butterfly_rotation(&s[14], &s[15], 62 - 56, true);

  // stage 3.
  HadamardRotation(&s[0], &s[8], false);
  HadamardRotation(&s[1], &s[9], false);
  HadamardRotation(&s[2], &s[10], false);
  HadamardRotation(&s[3], &s[11], false);
  HadamardRotation(&s[4], &s[12], false);
  HadamardRotation(&s[5], &s[13], false);
  HadamardRotation(&s[6], &s[14], false);
  HadamardRotation(&s[7], &s[15], false);

  // stage 4.
  butterfly_rotation(&s[8], &s[9], 56 - 0, true);
  butterfly_rotation(&s[13], &s[12], 8 + 0, true);
  butterfly_rotation(&s[10], &s[11], 56 - 32, true);
  butterfly_rotation(&s[15], &s[14], 8 + 32, true);

  // stage 5.
  HadamardRotation(&s[0], &s[4], false);
  HadamardRotation(&s[8], &s[12], false);
  HadamardRotation(&s[1], &s[5], false);
  HadamardRotation(&s[9], &s[13], false);
  HadamardRotation(&s[2], &s[6], false);
  HadamardRotation(&s[10], &s[14], false);
  HadamardRotation(&s[3], &s[7], false);
  HadamardRotation(&s[11], &s[15], false);

  // stage 6.
  butterfly_rotation(&s[4], &s[5], 48 - 0, true);
  butterfly_rotation(&s[12], &s[13], 48 - 0, true);
  butterfly_rotation(&s[7], &s[6], 48 - 32, true);
  butterfly_rotation(&s[15], &s[14], 48 - 32, true);

  // stage 7.
  HadamardRotation(&s[0], &s[2], false);
  HadamardRotation(&s[4], &s[6], false);
  HadamardRotation(&s[8], &s[10], false);
  HadamardRotation(&s[12], &s[14], false);
  HadamardRotation(&s[1], &s[3], false);
  HadamardRotation(&s[5], &s[7], false);
  HadamardRotation(&s[9], &s[11], false);
  HadamardRotation(&s[13], &s[15], false);

  // stage 8.
  butterfly_rotation(&s[2], &s[3], 32, true);
  butterfly_rotation(&s[6], &s[7], 32, true);
  butterfly_rotation(&s[10], &s[11], 32, true);
  butterfly_rotation(&s[14], &s[15], 32, true);

  // stage 9.
  const __m128i v_zero = _mm_setzero_si128();
  x[0] = s[0];
  x[1] = _mm_subs_epi16(v_zero, s[8]);
  x[2] = s[12];
  x[3] = _mm_subs_epi16(v_zero, s[4]);
  x[4] = s[6];
  x[5] = _mm_subs_epi16(v_zero, s[14]);
  x[6] = s[10];
  x[7] = _mm_subs_epi16(v_zero, s[2]);
  x[8] = s[3];
  x[9] = _mm_subs_epi16(v_zero, s[11]);
  x[10] = s[15];
  x[11] = _mm_subs_epi16(v_zero, s[7]);
  x[12] = s[5];
  x[13] = _mm_subs_epi16(v_zero, s[13]);
  x[14] = s[9];
  x[15] = _mm_subs_epi16(v_zero, s[1]);

  if (stage_is_rectangular) {
    if (transpose) {
      __m128i output[4];
      Transpose4x8To8x4_U16(x, output);
      StoreDst<16, 4>(dst, step, 0, output);
      Transpose4x8To8x4_U16(&x[8], output);
      StoreDst<16, 4>(dst, step, 8, output);
    } else {
      StoreDst<8, 16>(dst, step, 0, x);
    }
  } else {
    if (transpose) {
      for (int idx = 0; idx < 16; idx += 8) {
        __m128i output[8];
        Transpose8x8_U16(&x[idx], output);
        StoreDst<16, 8>(dst, step, idx, output);
      }
    } else {
      StoreDst<16, 16>(dst, step, 0, x);
    }
  }
}

LIBGAV1_ALWAYS_INLINE void Adst16DcOnlyInternal(__m128i* s, __m128i* x) {
  // stage 2.
  ButterflyRotation_FirstIsZero(&s[0], &s[1], 62, true);

  // stage 3.
  s[8] = s[0];
  s[9] = s[1];

  // stage 4.
  ButterflyRotation_4(&s[8], &s[9], 56, true);

  // stage 5.
  s[4] = s[0];
  s[12] = s[8];
  s[5] = s[1];
  s[13] = s[9];

  // stage 6.
  ButterflyRotation_4(&s[4], &s[5], 48, true);
  ButterflyRotation_4(&s[12], &s[13], 48, true);

  // stage 7.
  s[2] = s[0];
  s[6] = s[4];
  s[10] = s[8];
  s[14] = s[12];
  s[3] = s[1];
  s[7] = s[5];
  s[11] = s[9];
  s[15] = s[13];

  // stage 8.
  ButterflyRotation_4(&s[2], &s[3], 32, true);
  ButterflyRotation_4(&s[6], &s[7], 32, true);
  ButterflyRotation_4(&s[10], &s[11], 32, true);
  ButterflyRotation_4(&s[14], &s[15], 32, true);

  // stage 9.
  const __m128i v_zero = _mm_setzero_si128();
  x[0] = s[0];
  x[1] = _mm_subs_epi16(v_zero, s[8]);
  x[2] = s[12];
  x[3] = _mm_subs_epi16(v_zero, s[4]);
  x[4] = s[6];
  x[5] = _mm_subs_epi16(v_zero, s[14]);
  x[6] = s[10];
  x[7] = _mm_subs_epi16(v_zero, s[2]);
  x[8] = s[3];
  x[9] = _mm_subs_epi16(v_zero, s[11]);
  x[10] = s[15];
  x[11] = _mm_subs_epi16(v_zero, s[7]);
  x[12] = s[5];
  x[13] = _mm_subs_epi16(v_zero, s[13]);
  x[14] = s[9];
  x[15] = _mm_subs_epi16(v_zero, s[1]);
}

LIBGAV1_ALWAYS_INLINE bool Adst16DcOnly(void* dest, int adjusted_tx_height,
                                        bool should_round, int row_shift) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int16_t*>(dest);
  __m128i s[16];
  __m128i x[16];

  const __m128i v_src = _mm_shufflelo_epi16(_mm_cvtsi32_si128(dst[0]), 0);
  const __m128i v_mask =
      _mm_set1_epi16(should_round ? static_cast<int16_t>(0xffff) : 0);
  const __m128i v_kTransformRowMultiplier =
      _mm_set1_epi16(kTransformRowMultiplier << 3);
  const __m128i v_src_round =
      _mm_mulhrs_epi16(v_src, v_kTransformRowMultiplier);
  // stage 1.
  s[1] = _mm_blendv_epi8(v_src, v_src_round, v_mask);

  Adst16DcOnlyInternal(s, x);

  for (int i = 0; i < 2; ++i) {
    const __m128i x1_x0 = _mm_unpacklo_epi16(x[0 + i * 8], x[1 + i * 8]);
    const __m128i x3_x2 = _mm_unpacklo_epi16(x[2 + i * 8], x[3 + i * 8]);
    const __m128i x5_x4 = _mm_unpacklo_epi16(x[4 + i * 8], x[5 + i * 8]);
    const __m128i x7_x6 = _mm_unpacklo_epi16(x[6 + i * 8], x[7 + i * 8]);
    const __m128i x3_x0 = _mm_unpacklo_epi32(x1_x0, x3_x2);
    const __m128i x7_x4 = _mm_unpacklo_epi32(x5_x4, x7_x6);

    const __m128i v_row_shift_add = _mm_set1_epi32(row_shift);
    const __m128i v_row_shift = _mm_cvtepu32_epi64(v_row_shift_add);
    const __m128i a = _mm_add_epi32(_mm_cvtepi16_epi32(x3_x0), v_row_shift_add);
    const __m128i a1 =
        _mm_add_epi32(_mm_cvtepi16_epi32(x7_x4), v_row_shift_add);
    const __m128i b = _mm_sra_epi32(a, v_row_shift);
    const __m128i b1 = _mm_sra_epi32(a1, v_row_shift);
    StoreUnaligned16(&dst[i * 8], _mm_packs_epi32(b, b1));
  }
  return true;
}

LIBGAV1_ALWAYS_INLINE bool Adst16DcOnlyColumn(void* dest,
                                              int adjusted_tx_height,
                                              int width) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int16_t*>(dest);
  int i = 0;
  do {
    __m128i s[16];
    __m128i x[16];
    const __m128i v_src = LoadUnaligned16(dst);
    // stage 1.
    s[1] = v_src;

    Adst16DcOnlyInternal(s, x);

    for (int j = 0; j < 16; ++j) {
      StoreLo8(&dst[j * width], x[j]);
    }
    i += 4;
    dst += 4;
  } while (i < width);

  return true;
}

//------------------------------------------------------------------------------
// Identity Transforms.

template <bool is_row_shift>
LIBGAV1_ALWAYS_INLINE void Identity4_SSE4_1(void* dest, int32_t step) {
  auto* const dst = static_cast<int16_t*>(dest);

  if (is_row_shift) {
    const int shift = 1;
    const __m128i v_dual_round = _mm_set1_epi16((1 + (shift << 1)) << 11);
    const __m128i v_multiplier_one =
        _mm_set1_epi32((kIdentity4Multiplier << 16) | 0x0001);
    for (int i = 0; i < 4; i += 2) {
      const __m128i v_src = LoadUnaligned16(&dst[i * step]);
      const __m128i v_src_round = _mm_unpacklo_epi16(v_dual_round, v_src);
      const __m128i v_src_round_hi = _mm_unpackhi_epi16(v_dual_round, v_src);
      const __m128i a = _mm_madd_epi16(v_src_round, v_multiplier_one);
      const __m128i a_hi = _mm_madd_epi16(v_src_round_hi, v_multiplier_one);
      const __m128i b = _mm_srai_epi32(a, 12 + shift);
      const __m128i b_hi = _mm_srai_epi32(a_hi, 12 + shift);
      StoreUnaligned16(&dst[i * step], _mm_packs_epi32(b, b_hi));
    }
  } else {
    const __m128i v_multiplier =
        _mm_set1_epi16(kIdentity4MultiplierFraction << 3);
    for (int i = 0; i < 4; i += 2) {
      const __m128i v_src = LoadUnaligned16(&dst[i * step]);
      const __m128i a = _mm_mulhrs_epi16(v_src, v_multiplier);
      const __m128i b = _mm_adds_epi16(a, v_src);
      StoreUnaligned16(&dst[i * step], b);
    }
  }
}

LIBGAV1_ALWAYS_INLINE bool Identity4DcOnly(void* dest, int adjusted_tx_height,
                                           bool should_round, int tx_height) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int16_t*>(dest);
  const __m128i v_src0 = _mm_cvtsi32_si128(dst[0]);
  const __m128i v_mask =
      _mm_set1_epi16(should_round ? static_cast<int16_t>(0xffff) : 0);
  const __m128i v_kTransformRowMultiplier =
      _mm_set1_epi16(kTransformRowMultiplier << 3);
  const __m128i v_src_round =
      _mm_mulhrs_epi16(v_src0, v_kTransformRowMultiplier);
  const __m128i v_src = _mm_blendv_epi8(v_src0, v_src_round, v_mask);

  const int shift = (tx_height < 16) ? 0 : 1;
  const __m128i v_dual_round = _mm_set1_epi16((1 + (shift << 1)) << 11);
  const __m128i v_multiplier_one =
      _mm_set1_epi32((kIdentity4Multiplier << 16) | 0x0001);
  const __m128i v_src_round_lo = _mm_unpacklo_epi16(v_dual_round, v_src);
  const __m128i a = _mm_madd_epi16(v_src_round_lo, v_multiplier_one);
  const __m128i b = _mm_srai_epi32(a, 12 + shift);
  dst[0] = _mm_extract_epi16(_mm_packs_epi32(b, b), 0);
  return true;
}

LIBGAV1_ALWAYS_INLINE void Identity4ColumnStoreToFrame(
    Array2DView<uint8_t> frame, const int start_x, const int start_y,
    const int tx_width, const int tx_height,
    const int16_t* LIBGAV1_RESTRICT source) {
  const int stride = frame.columns();
  uint8_t* LIBGAV1_RESTRICT dst = frame[start_y] + start_x;

  const __m128i v_multiplier_fraction =
      _mm_set1_epi16(static_cast<int16_t>(kIdentity4MultiplierFraction << 3));
  const __m128i v_eight = _mm_set1_epi16(8);

  if (tx_width == 4) {
    int i = 0;
    do {
      const __m128i v_src = LoadLo8(&source[i * tx_width]);
      const __m128i v_src_mult = _mm_mulhrs_epi16(v_src, v_multiplier_fraction);
      const __m128i frame_data = Load4(dst);
      const __m128i v_dst_i = _mm_adds_epi16(v_src_mult, v_src);
      const __m128i a = _mm_adds_epi16(v_dst_i, v_eight);
      const __m128i b = _mm_srai_epi16(a, 4);
      const __m128i c = _mm_cvtepu8_epi16(frame_data);
      const __m128i d = _mm_adds_epi16(c, b);
      Store4(dst, _mm_packus_epi16(d, d));
      dst += stride;
    } while (++i < tx_height);
  } else {
    int i = 0;
    do {
      const int row = i * tx_width;
      int j = 0;
      do {
        const __m128i v_src = LoadUnaligned16(&source[row + j]);
        const __m128i v_src_mult =
            _mm_mulhrs_epi16(v_src, v_multiplier_fraction);
        const __m128i frame_data = LoadLo8(dst + j);
        const __m128i v_dst_i = _mm_adds_epi16(v_src_mult, v_src);
        const __m128i a = _mm_adds_epi16(v_dst_i, v_eight);
        const __m128i b = _mm_srai_epi16(a, 4);
        const __m128i c = _mm_cvtepu8_epi16(frame_data);
        const __m128i d = _mm_adds_epi16(c, b);
        StoreLo8(dst + j, _mm_packus_epi16(d, d));
        j += 8;
      } while (j < tx_width);
      dst += stride;
    } while (++i < tx_height);
  }
}

LIBGAV1_ALWAYS_INLINE void Identity4RowColumnStoreToFrame(
    Array2DView<uint8_t> frame, const int start_x, const int start_y,
    const int tx_width, const int tx_height,
    const int16_t* LIBGAV1_RESTRICT source) {
  const int stride = frame.columns();
  uint8_t* LIBGAV1_RESTRICT dst = frame[start_y] + start_x;

  const __m128i v_multiplier_fraction =
      _mm_set1_epi16(static_cast<int16_t>(kIdentity4MultiplierFraction << 3));
  const __m128i v_eight = _mm_set1_epi16(8);
  const __m128i v_kTransformRowMultiplier =
      _mm_set1_epi16(kTransformRowMultiplier << 3);

  if (tx_width == 4) {
    int i = 0;
    do {
      const __m128i v_src = LoadLo8(&source[i * tx_width]);
      const __m128i v_src_mult = _mm_mulhrs_epi16(v_src, v_multiplier_fraction);
      const __m128i frame_data = Load4(dst);
      const __m128i v_dst_row = _mm_adds_epi16(v_src_mult, v_src);
      const __m128i v_src_mult2 =
          _mm_mulhrs_epi16(v_dst_row, v_multiplier_fraction);
      const __m128i frame_data16 = _mm_cvtepu8_epi16(frame_data);
      const __m128i v_dst_col = _mm_adds_epi16(v_src_mult2, v_dst_row);
      const __m128i a = _mm_adds_epi16(v_dst_col, v_eight);
      const __m128i b = _mm_srai_epi16(a, 4);
      const __m128i c = _mm_adds_epi16(frame_data16, b);
      Store4(dst, _mm_packus_epi16(c, c));
      dst += stride;
    } while (++i < tx_height);
  } else {
    int i = 0;
    do {
      const int row = i * tx_width;
      int j = 0;
      do {
        const __m128i v_src = LoadUnaligned16(&source[row + j]);
        const __m128i v_src_round =
            _mm_mulhrs_epi16(v_src, v_kTransformRowMultiplier);
        const __m128i v_dst_row = _mm_adds_epi16(v_src_round, v_src_round);
        const __m128i v_src_mult2 =
            _mm_mulhrs_epi16(v_dst_row, v_multiplier_fraction);
        const __m128i frame_data = LoadLo8(dst + j);
        const __m128i frame_data16 = _mm_cvtepu8_epi16(frame_data);
        const __m128i v_dst_col = _mm_adds_epi16(v_src_mult2, v_dst_row);
        const __m128i a = _mm_adds_epi16(v_dst_col, v_eight);
        const __m128i b = _mm_srai_epi16(a, 4);
        const __m128i c = _mm_adds_epi16(frame_data16, b);
        StoreLo8(dst + j, _mm_packus_epi16(c, c));
        j += 8;
      } while (j < tx_width);
      dst += stride;
    } while (++i < tx_height);
  }
}

LIBGAV1_ALWAYS_INLINE void Identity8Row32_SSE4_1(void* dest, int32_t step) {
  auto* const dst = static_cast<int16_t*>(dest);

  // When combining the identity8 multiplier with the row shift, the
  // calculations for tx_height equal to 32 can be simplified from
  // ((A * 2) + 2) >> 2) to ((A + 1) >> 1).
  const __m128i v_row_multiplier = _mm_set1_epi16(1 << 14);
  for (int h = 0; h < 4; ++h) {
    const __m128i v_src = LoadUnaligned16(&dst[h * step]);
    const __m128i v_src_mult = _mm_mulhrs_epi16(v_src, v_row_multiplier);
    StoreUnaligned16(&dst[h * step], v_src_mult);
  }
}

LIBGAV1_ALWAYS_INLINE void Identity8Row4_SSE4_1(void* dest, int32_t step) {
  auto* const dst = static_cast<int16_t*>(dest);

  for (int h = 0; h < 4; ++h) {
    const __m128i v_src = LoadUnaligned16(&dst[h * step]);
    // For bitdepth == 8, the identity row clamps to a signed 16bit value, so
    // saturating add here is ok.
    const __m128i a = _mm_adds_epi16(v_src, v_src);
    StoreUnaligned16(&dst[h * step], a);
  }
}

LIBGAV1_ALWAYS_INLINE bool Identity8DcOnly(void* dest, int adjusted_tx_height,
                                           bool should_round, int row_shift) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int16_t*>(dest);
  const __m128i v_src0 = _mm_cvtsi32_si128(dst[0]);
  const __m128i v_mask =
      _mm_set1_epi16(should_round ? static_cast<int16_t>(0xffff) : 0);
  const __m128i v_kTransformRowMultiplier =
      _mm_set1_epi16(kTransformRowMultiplier << 3);
  const __m128i v_src_round =
      _mm_mulhrs_epi16(v_src0, v_kTransformRowMultiplier);
  const __m128i v_src =
      _mm_cvtepi16_epi32(_mm_blendv_epi8(v_src0, v_src_round, v_mask));
  const __m128i v_srcx2 = _mm_add_epi32(v_src, v_src);
  const __m128i v_row_shift_add = _mm_set1_epi32(row_shift);
  const __m128i v_row_shift = _mm_cvtepu32_epi64(v_row_shift_add);
  const __m128i a = _mm_add_epi32(v_srcx2, v_row_shift_add);
  const __m128i b = _mm_sra_epi32(a, v_row_shift);
  dst[0] = _mm_extract_epi16(_mm_packs_epi32(b, b), 0);
  return true;
}

LIBGAV1_ALWAYS_INLINE void Identity8ColumnStoreToFrame_SSE4_1(
    Array2DView<uint8_t> frame, const int start_x, const int start_y,
    const int tx_width, const int tx_height,
    const int16_t* LIBGAV1_RESTRICT source) {
  const int stride = frame.columns();
  uint8_t* LIBGAV1_RESTRICT dst = frame[start_y] + start_x;
  const __m128i v_eight = _mm_set1_epi16(8);
  if (tx_width == 4) {
    int i = 0;
    do {
      const int row = i * tx_width;
      const __m128i v_src = LoadLo8(&source[row]);
      const __m128i v_dst_i = _mm_adds_epi16(v_src, v_src);
      const __m128i frame_data = Load4(dst);
      const __m128i a = _mm_adds_epi16(v_dst_i, v_eight);
      const __m128i b = _mm_srai_epi16(a, 4);
      const __m128i c = _mm_cvtepu8_epi16(frame_data);
      const __m128i d = _mm_adds_epi16(c, b);
      Store4(dst, _mm_packus_epi16(d, d));
      dst += stride;
    } while (++i < tx_height);
  } else {
    int i = 0;
    do {
      const int row = i * tx_width;
      int j = 0;
      do {
        const __m128i v_src = LoadUnaligned16(&source[row + j]);
        const __m128i v_dst_i = _mm_adds_epi16(v_src, v_src);
        const __m128i frame_data = LoadLo8(dst + j);
        const __m128i a = _mm_adds_epi16(v_dst_i, v_eight);
        const __m128i b = _mm_srai_epi16(a, 4);
        const __m128i c = _mm_cvtepu8_epi16(frame_data);
        const __m128i d = _mm_adds_epi16(c, b);
        StoreLo8(dst + j, _mm_packus_epi16(d, d));
        j += 8;
      } while (j < tx_width);
      dst += stride;
    } while (++i < tx_height);
  }
}

LIBGAV1_ALWAYS_INLINE void Identity16Row_SSE4_1(void* dest, int32_t step,
                                                int shift) {
  auto* const dst = static_cast<int16_t*>(dest);

  const __m128i v_dual_round = _mm_set1_epi16((1 + (shift << 1)) << 11);
  const __m128i v_multiplier_one =
      _mm_set1_epi32((kIdentity16Multiplier << 16) | 0x0001);
  const __m128i v_shift = _mm_set_epi64x(0, 12 + shift);

  for (int h = 0; h < 4; ++h) {
    const __m128i v_src = LoadUnaligned16(&dst[h * step]);
    const __m128i v_src2 = LoadUnaligned16(&dst[h * step + 8]);
    const __m128i v_src_round0 = _mm_unpacklo_epi16(v_dual_round, v_src);
    const __m128i v_src_round1 = _mm_unpackhi_epi16(v_dual_round, v_src);
    const __m128i v_src2_round0 = _mm_unpacklo_epi16(v_dual_round, v_src2);
    const __m128i v_src2_round1 = _mm_unpackhi_epi16(v_dual_round, v_src2);
    const __m128i madd0 = _mm_madd_epi16(v_src_round0, v_multiplier_one);
    const __m128i madd1 = _mm_madd_epi16(v_src_round1, v_multiplier_one);
    const __m128i madd20 = _mm_madd_epi16(v_src2_round0, v_multiplier_one);
    const __m128i madd21 = _mm_madd_epi16(v_src2_round1, v_multiplier_one);
    const __m128i shift0 = _mm_sra_epi32(madd0, v_shift);
    const __m128i shift1 = _mm_sra_epi32(madd1, v_shift);
    const __m128i shift20 = _mm_sra_epi32(madd20, v_shift);
    const __m128i shift21 = _mm_sra_epi32(madd21, v_shift);
    StoreUnaligned16(&dst[h * step], _mm_packs_epi32(shift0, shift1));
    StoreUnaligned16(&dst[h * step + 8], _mm_packs_epi32(shift20, shift21));
  }
}

LIBGAV1_ALWAYS_INLINE bool Identity16DcOnly(void* dest, int adjusted_tx_height,
                                            bool should_round, int shift) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int16_t*>(dest);
  const __m128i v_src0 = _mm_cvtsi32_si128(dst[0]);
  const __m128i v_mask =
      _mm_set1_epi16(should_round ? static_cast<int16_t>(0xffff) : 0);
  const __m128i v_kTransformRowMultiplier =
      _mm_set1_epi16(kTransformRowMultiplier << 3);
  const __m128i v_src_round0 =
      _mm_mulhrs_epi16(v_src0, v_kTransformRowMultiplier);
  const __m128i v_src = _mm_blendv_epi8(v_src0, v_src_round0, v_mask);
  const __m128i v_dual_round = _mm_set1_epi16((1 + (shift << 1)) << 11);
  const __m128i v_multiplier_one =
      _mm_set1_epi32((kIdentity16Multiplier << 16) | 0x0001);
  const __m128i v_shift = _mm_set_epi64x(0, 12 + shift);
  const __m128i v_src_round = _mm_unpacklo_epi16(v_dual_round, v_src);
  const __m128i a = _mm_madd_epi16(v_src_round, v_multiplier_one);
  const __m128i b = _mm_sra_epi32(a, v_shift);
  dst[0] = _mm_extract_epi16(_mm_packs_epi32(b, b), 0);
  return true;
}

LIBGAV1_ALWAYS_INLINE void Identity16ColumnStoreToFrame_SSE4_1(
    Array2DView<uint8_t> frame, const int start_x, const int start_y,
    const int tx_width, const int tx_height,
    const int16_t* LIBGAV1_RESTRICT source) {
  const int stride = frame.columns();
  uint8_t* LIBGAV1_RESTRICT dst = frame[start_y] + start_x;
  const __m128i v_eight = _mm_set1_epi16(8);
  const __m128i v_multiplier =
      _mm_set1_epi16(static_cast<int16_t>(kIdentity4MultiplierFraction << 4));

  if (tx_width == 4) {
    int i = 0;
    do {
      const __m128i v_src = LoadLo8(&source[i * tx_width]);
      const __m128i v_src_mult = _mm_mulhrs_epi16(v_src, v_multiplier);
      const __m128i frame_data = Load4(dst);
      const __m128i v_srcx2 = _mm_adds_epi16(v_src, v_src);
      const __m128i v_dst_i = _mm_adds_epi16(v_src_mult, v_srcx2);
      const __m128i a = _mm_adds_epi16(v_dst_i, v_eight);
      const __m128i b = _mm_srai_epi16(a, 4);
      const __m128i c = _mm_cvtepu8_epi16(frame_data);
      const __m128i d = _mm_adds_epi16(c, b);
      Store4(dst, _mm_packus_epi16(d, d));
      dst += stride;
    } while (++i < tx_height);
  } else {
    int i = 0;
    do {
      const int row = i * tx_width;
      int j = 0;
      do {
        const __m128i v_src = LoadUnaligned16(&source[row + j]);
        const __m128i v_src_mult = _mm_mulhrs_epi16(v_src, v_multiplier);
        const __m128i frame_data = LoadLo8(dst + j);
        const __m128i v_srcx2 = _mm_adds_epi16(v_src, v_src);
        const __m128i v_dst_i = _mm_adds_epi16(v_src_mult, v_srcx2);
        const __m128i a = _mm_adds_epi16(v_dst_i, v_eight);
        const __m128i b = _mm_srai_epi16(a, 4);
        const __m128i c = _mm_cvtepu8_epi16(frame_data);
        const __m128i d = _mm_adds_epi16(c, b);
        StoreLo8(dst + j, _mm_packus_epi16(d, d));
        j += 8;
      } while (j < tx_width);
      dst += stride;
    } while (++i < tx_height);
  }
}

LIBGAV1_ALWAYS_INLINE void Identity32Row16_SSE4_1(void* dest,
                                                  const int32_t step) {
  auto* const dst = static_cast<int16_t*>(dest);

  // When combining the identity32 multiplier with the row shift, the
  // calculation for tx_height equal to 16 can be simplified from
  // ((A * 4) + 1) >> 1) to (A * 2).
  for (int h = 0; h < 4; ++h) {
    for (int i = 0; i < 32; i += 8) {
      const __m128i v_src = LoadUnaligned16(&dst[h * step + i]);
      // For bitdepth == 8, the identity row clamps to a signed 16bit value, so
      // saturating add here is ok.
      const __m128i v_dst_i = _mm_adds_epi16(v_src, v_src);
      StoreUnaligned16(&dst[h * step + i], v_dst_i);
    }
  }
}

LIBGAV1_ALWAYS_INLINE bool Identity32DcOnly(void* dest,
                                            int adjusted_tx_height) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int16_t*>(dest);
  const __m128i v_src0 = _mm_cvtsi32_si128(dst[0]);
  const __m128i v_kTransformRowMultiplier =
      _mm_set1_epi16(kTransformRowMultiplier << 3);
  const __m128i v_src = _mm_mulhrs_epi16(v_src0, v_kTransformRowMultiplier);

  // When combining the identity32 multiplier with the row shift, the
  // calculation for tx_height equal to 16 can be simplified from
  // ((A * 4) + 1) >> 1) to (A * 2).
  const __m128i v_dst_0 = _mm_adds_epi16(v_src, v_src);
  dst[0] = _mm_extract_epi16(v_dst_0, 0);
  return true;
}

LIBGAV1_ALWAYS_INLINE void Identity32ColumnStoreToFrame(
    Array2DView<uint8_t> frame, const int start_x, const int start_y,
    const int tx_width, const int tx_height,
    const int16_t* LIBGAV1_RESTRICT source) {
  const int stride = frame.columns();
  uint8_t* LIBGAV1_RESTRICT dst = frame[start_y] + start_x;
  const __m128i v_two = _mm_set1_epi16(2);

  int i = 0;
  do {
    const int row = i * tx_width;
    int j = 0;
    do {
      const __m128i v_dst_i = LoadUnaligned16(&source[row + j]);
      const __m128i frame_data = LoadLo8(dst + j);
      const __m128i a = _mm_adds_epi16(v_dst_i, v_two);
      const __m128i b = _mm_srai_epi16(a, 2);
      const __m128i c = _mm_cvtepu8_epi16(frame_data);
      const __m128i d = _mm_adds_epi16(c, b);
      StoreLo8(dst + j, _mm_packus_epi16(d, d));
      j += 8;
    } while (j < tx_width);
    dst += stride;
  } while (++i < tx_height);
}

//------------------------------------------------------------------------------
// Walsh Hadamard Transform.

// Process 4 wht4 rows and columns.
LIBGAV1_ALWAYS_INLINE void Wht4_SSE4_1(Array2DView<uint8_t> frame,
                                       const int start_x, const int start_y,
                                       const void* LIBGAV1_RESTRICT source,
                                       const int adjusted_tx_height) {
  const auto* const src = static_cast<const int16_t*>(source);
  __m128i s[4], x[4];

  if (adjusted_tx_height == 1) {
    // Special case: only src[0] is nonzero.
    //   src[0]  0   0   0
    //       0   0   0   0
    //       0   0   0   0
    //       0   0   0   0
    //
    // After the row and column transforms are applied, we have:
    //       f   h   h   h
    //       g   i   i   i
    //       g   i   i   i
    //       g   i   i   i
    // where f, g, h, i are computed as follows.
    int16_t f = (src[0] >> 2) - (src[0] >> 3);
    const int16_t g = f >> 1;
    f = f - (f >> 1);
    const int16_t h = (src[0] >> 3) - (src[0] >> 4);
    const int16_t i = (src[0] >> 4);
    s[0] = _mm_set1_epi16(h);
    s[0] = _mm_insert_epi16(s[0], f, 0);
    s[1] = _mm_set1_epi16(i);
    s[1] = _mm_insert_epi16(s[1], g, 0);
    s[2] = s[3] = s[1];
  } else {
    x[0] = LoadLo8(&src[0 * 4]);
    x[2] = LoadLo8(&src[1 * 4]);
    x[3] = LoadLo8(&src[2 * 4]);
    x[1] = LoadLo8(&src[3 * 4]);

    // Row transforms.
    Transpose4x4_U16(x, x);
    s[0] = _mm_srai_epi16(x[0], 2);
    s[2] = _mm_srai_epi16(x[1], 2);
    s[3] = _mm_srai_epi16(x[2], 2);
    s[1] = _mm_srai_epi16(x[3], 2);
    s[0] = _mm_add_epi16(s[0], s[2]);
    s[3] = _mm_sub_epi16(s[3], s[1]);
    __m128i e = _mm_sub_epi16(s[0], s[3]);
    e = _mm_srai_epi16(e, 1);
    s[1] = _mm_sub_epi16(e, s[1]);
    s[2] = _mm_sub_epi16(e, s[2]);
    s[0] = _mm_sub_epi16(s[0], s[1]);
    s[3] = _mm_add_epi16(s[3], s[2]);
    Transpose4x4_U16(s, s);

    // Column transforms.
    s[0] = _mm_add_epi16(s[0], s[2]);
    s[3] = _mm_sub_epi16(s[3], s[1]);
    e = _mm_sub_epi16(s[0], s[3]);
    e = _mm_srai_epi16(e, 1);
    s[1] = _mm_sub_epi16(e, s[1]);
    s[2] = _mm_sub_epi16(e, s[2]);
    s[0] = _mm_sub_epi16(s[0], s[1]);
    s[3] = _mm_add_epi16(s[3], s[2]);
  }

  // Store to frame.
  const int stride = frame.columns();
  uint8_t* LIBGAV1_RESTRICT dst = frame[start_y] + start_x;
  for (int row = 0; row < 4; ++row) {
    const __m128i frame_data = Load4(dst);
    const __m128i a = _mm_cvtepu8_epi16(frame_data);
    const __m128i b = _mm_add_epi16(a, s[row]);
    Store4(dst, _mm_packus_epi16(b, b));
    dst += stride;
  }
}

//------------------------------------------------------------------------------
// row/column transform loops

template <bool enable_flip_rows = false>
LIBGAV1_ALWAYS_INLINE void StoreToFrameWithRound(
    Array2DView<uint8_t> frame, const int start_x, const int start_y,
    const int tx_width, const int tx_height,
    const int16_t* LIBGAV1_RESTRICT source, TransformType tx_type) {
  const bool flip_rows =
      enable_flip_rows ? kTransformFlipRowsMask.Contains(tx_type) : false;
  const __m128i v_eight = _mm_set1_epi16(8);
  const int stride = frame.columns();
  uint8_t* LIBGAV1_RESTRICT dst = frame[start_y] + start_x;
  if (tx_width == 4) {
    for (int i = 0; i < tx_height; ++i) {
      const int row = flip_rows ? (tx_height - i - 1) * 4 : i * 4;
      const __m128i residual = LoadLo8(&source[row]);
      const __m128i frame_data = Load4(dst);
      // Saturate to prevent overflowing int16_t
      const __m128i a = _mm_adds_epi16(residual, v_eight);
      const __m128i b = _mm_srai_epi16(a, 4);
      const __m128i c = _mm_cvtepu8_epi16(frame_data);
      const __m128i d = _mm_adds_epi16(c, b);
      Store4(dst, _mm_packus_epi16(d, d));
      dst += stride;
    }
  } else if (tx_width == 8) {
    for (int i = 0; i < tx_height; ++i) {
      const int row = flip_rows ? (tx_height - i - 1) * 8 : i * 8;
      const __m128i residual = LoadUnaligned16(&source[row]);
      const __m128i frame_data = LoadLo8(dst);
      // Saturate to prevent overflowing int16_t
      const __m128i b = _mm_adds_epi16(residual, v_eight);
      const __m128i c = _mm_srai_epi16(b, 4);
      const __m128i d = _mm_cvtepu8_epi16(frame_data);
      const __m128i e = _mm_adds_epi16(d, c);
      StoreLo8(dst, _mm_packus_epi16(e, e));
      dst += stride;
    }
  } else {
    for (int i = 0; i < tx_height; ++i) {
      const int y = start_y + i;
      const int row = flip_rows ? (tx_height - i - 1) * tx_width : i * tx_width;
      int j = 0;
      do {
        const int x = start_x + j;
        const __m128i residual = LoadUnaligned16(&source[row + j]);
        const __m128i residual_hi = LoadUnaligned16(&source[row + j + 8]);
        const __m128i frame_data = LoadUnaligned16(frame[y] + x);
        const __m128i b = _mm_adds_epi16(residual, v_eight);
        const __m128i b_hi = _mm_adds_epi16(residual_hi, v_eight);
        const __m128i c = _mm_srai_epi16(b, 4);
        const __m128i c_hi = _mm_srai_epi16(b_hi, 4);
        const __m128i d = _mm_cvtepu8_epi16(frame_data);
        const __m128i d_hi = _mm_cvtepu8_epi16(_mm_srli_si128(frame_data, 8));
        const __m128i e = _mm_adds_epi16(d, c);
        const __m128i e_hi = _mm_adds_epi16(d_hi, c_hi);
        StoreUnaligned16(frame[y] + x, _mm_packus_epi16(e, e_hi));
        j += 16;
      } while (j < tx_width);
    }
  }
}

template <int tx_height>
LIBGAV1_ALWAYS_INLINE void FlipColumns(int16_t* source, int tx_width) {
  const __m128i word_reverse_8 =
      _mm_set_epi32(0x01000302, 0x05040706, 0x09080b0a, 0x0d0c0f0e);
  if (tx_width >= 16) {
    int i = 0;
    do {
      // read 16 shorts
      const __m128i v3210 = LoadUnaligned16(&source[i]);
      const __m128i v7654 = LoadUnaligned16(&source[i + 8]);
      const __m128i v0123 = _mm_shuffle_epi8(v3210, word_reverse_8);
      const __m128i v4567 = _mm_shuffle_epi8(v7654, word_reverse_8);
      StoreUnaligned16(&source[i], v4567);
      StoreUnaligned16(&source[i + 8], v0123);
      i += 16;
    } while (i < tx_width * tx_height);
  } else if (tx_width == 8) {
    for (int i = 0; i < 8 * tx_height; i += 8) {
      const __m128i a = LoadUnaligned16(&source[i]);
      const __m128i b = _mm_shuffle_epi8(a, word_reverse_8);
      StoreUnaligned16(&source[i], b);
    }
  } else {
    const __m128i dual_word_reverse_4 =
        _mm_set_epi32(0x09080b0a, 0x0d0c0f0e, 0x01000302, 0x05040706);
    // Process two rows per iteration.
    for (int i = 0; i < 4 * tx_height; i += 8) {
      const __m128i a = LoadUnaligned16(&source[i]);
      const __m128i b = _mm_shuffle_epi8(a, dual_word_reverse_4);
      StoreUnaligned16(&source[i], b);
    }
  }
}

template <int tx_width>
LIBGAV1_ALWAYS_INLINE void ApplyRounding(int16_t* source, int num_rows) {
  const __m128i v_kTransformRowMultiplier =
      _mm_set1_epi16(kTransformRowMultiplier << 3);
  if (tx_width == 4) {
    // Process two rows per iteration.
    int i = 0;
    do {
      const __m128i a = LoadUnaligned16(&source[i]);
      const __m128i b = _mm_mulhrs_epi16(a, v_kTransformRowMultiplier);
      StoreUnaligned16(&source[i], b);
      i += 8;
    } while (i < tx_width * num_rows);
  } else {
    int i = 0;
    do {
      // The last 32 values of every row are always zero if the |tx_width| is
      // 64.
      const int non_zero_width = (tx_width < 64) ? tx_width : 32;
      int j = 0;
      do {
        const __m128i a = LoadUnaligned16(&source[i * tx_width + j]);
        const __m128i b = _mm_mulhrs_epi16(a, v_kTransformRowMultiplier);
        StoreUnaligned16(&source[i * tx_width + j], b);
        j += 8;
      } while (j < non_zero_width);
    } while (++i < num_rows);
  }
}

template <int tx_width>
LIBGAV1_ALWAYS_INLINE void RowShift(int16_t* source, int num_rows,
                                    int row_shift) {
  const __m128i v_row_shift_add = _mm_set1_epi16(row_shift);
  const __m128i v_row_shift = _mm_cvtepu16_epi64(v_row_shift_add);
  if (tx_width == 4) {
    // Process two rows per iteration.
    int i = 0;
    do {
      const __m128i residual = LoadUnaligned16(&source[i]);
      const __m128i shifted_residual =
          ShiftResidual(residual, v_row_shift_add, v_row_shift);
      StoreUnaligned16(&source[i], shifted_residual);
      i += 8;
    } while (i < tx_width * num_rows);
  } else {
    int i = 0;
    do {
      for (int j = 0; j < tx_width; j += 8) {
        const __m128i residual = LoadUnaligned16(&source[i * tx_width + j]);
        const __m128i shifted_residual =
            ShiftResidual(residual, v_row_shift_add, v_row_shift);
        StoreUnaligned16(&source[i * tx_width + j], shifted_residual);
      }
    } while (++i < num_rows);
  }
}

void Dct4TransformLoopRow_SSE4_1(TransformType /*tx_type*/,
                                 TransformSize tx_size, int adjusted_tx_height,
                                 void* src_buffer, int /*start_x*/,
                                 int /*start_y*/, void* /*dst_frame*/) {
  auto* src = static_cast<int16_t*>(src_buffer);
  const int tx_height = kTransformHeight[tx_size];
  const bool should_round = (tx_height == 8);
  const int row_shift = static_cast<int>(tx_height == 16);

  if (DctDcOnly<4>(src, adjusted_tx_height, should_round, row_shift)) {
    return;
  }

  if (should_round) {
    ApplyRounding<4>(src, adjusted_tx_height);
  }

  if (adjusted_tx_height <= 4) {
    // Process 4 1d dct4 rows in parallel.
    Dct4_SSE4_1<ButterflyRotation_4, false>(src, /*step=*/4,
                                            /*transpose=*/true);
  } else {
    // Process 8 1d dct4 rows in parallel per iteration.
    int i = 0;
    do {
      Dct4_SSE4_1<ButterflyRotation_8, true>(&src[i * 4], /*step=*/4,
                                             /*transpose=*/true);
      i += 8;
    } while (i < adjusted_tx_height);
  }
  if (tx_height == 16) {
    RowShift<4>(src, adjusted_tx_height, 1);
  }
}

void Dct4TransformLoopColumn_SSE4_1(TransformType tx_type,
                                    TransformSize tx_size,
                                    int adjusted_tx_height,
                                    void* LIBGAV1_RESTRICT src_buffer,
                                    int start_x, int start_y,
                                    void* LIBGAV1_RESTRICT dst_frame) {
  auto* src = static_cast<int16_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (kTransformFlipColumnsMask.Contains(tx_type)) {
    FlipColumns<4>(src, tx_width);
  }

  if (!DctDcOnlyColumn<4>(src, adjusted_tx_height, tx_width)) {
    if (tx_width == 4) {
      // Process 4 1d dct4 columns in parallel.
      Dct4_SSE4_1<ButterflyRotation_4, false>(src, tx_width,
                                              /*transpose=*/false);
    } else {
      // Process 8 1d dct4 columns in parallel per iteration.
      int i = 0;
      do {
        Dct4_SSE4_1<ButterflyRotation_8, true>(&src[i], tx_width,
                                               /*transpose=*/false);
        i += 8;
      } while (i < tx_width);
    }
  }
  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  StoreToFrameWithRound(frame, start_x, start_y, tx_width, 4, src, tx_type);
}

void Dct8TransformLoopRow_SSE4_1(TransformType /*tx_type*/,
                                 TransformSize tx_size, int adjusted_tx_height,
                                 void* src_buffer, int /*start_x*/,
                                 int /*start_y*/, void* /*dst_frame*/) {
  auto* src = static_cast<int16_t*>(src_buffer);
  const bool should_round = kShouldRound[tx_size];
  const uint8_t row_shift = kTransformRowShift[tx_size];

  if (DctDcOnly<8>(src, adjusted_tx_height, should_round, row_shift)) {
    return;
  }

  if (should_round) {
    ApplyRounding<8>(src, adjusted_tx_height);
  }

  if (adjusted_tx_height <= 4) {
    // Process 4 1d dct8 rows in parallel.
    Dct8_SSE4_1<ButterflyRotation_4, true>(src, /*step=*/8, /*transpose=*/true);
  } else {
    // Process 8 1d dct8 rows in parallel per iteration.
    int i = 0;
    do {
      Dct8_SSE4_1<ButterflyRotation_8, false>(&src[i * 8], /*step=*/8,
                                              /*transpose=*/true);
      i += 8;
    } while (i < adjusted_tx_height);
  }
  if (row_shift > 0) {
    RowShift<8>(src, adjusted_tx_height, row_shift);
  }
}

void Dct8TransformLoopColumn_SSE4_1(TransformType tx_type,
                                    TransformSize tx_size,
                                    int adjusted_tx_height,
                                    void* LIBGAV1_RESTRICT src_buffer,
                                    int start_x, int start_y,
                                    void* LIBGAV1_RESTRICT dst_frame) {
  auto* src = static_cast<int16_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (kTransformFlipColumnsMask.Contains(tx_type)) {
    FlipColumns<8>(src, tx_width);
  }

  if (!DctDcOnlyColumn<8>(src, adjusted_tx_height, tx_width)) {
    if (tx_width == 4) {
      // Process 4 1d dct8 columns in parallel.
      Dct8_SSE4_1<ButterflyRotation_4, true>(src, 4, /*transpose=*/false);
    } else {
      // Process 8 1d dct8 columns in parallel per iteration.
      int i = 0;
      do {
        Dct8_SSE4_1<ButterflyRotation_8, false>(&src[i], tx_width,
                                                /*transpose=*/false);
        i += 8;
      } while (i < tx_width);
    }
  }
  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  StoreToFrameWithRound(frame, start_x, start_y, tx_width, 8, src, tx_type);
}

void Dct16TransformLoopRow_SSE4_1(TransformType /*tx_type*/,
                                  TransformSize tx_size, int adjusted_tx_height,
                                  void* src_buffer, int /*start_x*/,
                                  int /*start_y*/, void* /*dst_frame*/) {
  auto* src = static_cast<int16_t*>(src_buffer);
  const bool should_round = kShouldRound[tx_size];
  const uint8_t row_shift = kTransformRowShift[tx_size];

  if (DctDcOnly<16>(src, adjusted_tx_height, should_round, row_shift)) {
    return;
  }

  if (should_round) {
    ApplyRounding<16>(src, adjusted_tx_height);
  }

  if (adjusted_tx_height <= 4) {
    // Process 4 1d dct16 rows in parallel.
    Dct16_SSE4_1<ButterflyRotation_4, true>(src, 16, /*transpose=*/true);
  } else {
    int i = 0;
    do {
      // Process 8 1d dct16 rows in parallel per iteration.
      Dct16_SSE4_1<ButterflyRotation_8, false>(&src[i * 16], 16,
                                               /*transpose=*/true);
      i += 8;
    } while (i < adjusted_tx_height);
  }
  // row_shift is always non zero here.
  RowShift<16>(src, adjusted_tx_height, row_shift);
}

void Dct16TransformLoopColumn_SSE4_1(TransformType tx_type,
                                     TransformSize tx_size,
                                     int adjusted_tx_height,
                                     void* LIBGAV1_RESTRICT src_buffer,
                                     int start_x, int start_y,
                                     void* LIBGAV1_RESTRICT dst_frame) {
  auto* src = static_cast<int16_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (kTransformFlipColumnsMask.Contains(tx_type)) {
    FlipColumns<16>(src, tx_width);
  }

  if (!DctDcOnlyColumn<16>(src, adjusted_tx_height, tx_width)) {
    if (tx_width == 4) {
      // Process 4 1d dct16 columns in parallel.
      Dct16_SSE4_1<ButterflyRotation_4, true>(src, 4, /*transpose=*/false);
    } else {
      int i = 0;
      do {
        // Process 8 1d dct16 columns in parallel per iteration.
        Dct16_SSE4_1<ButterflyRotation_8, false>(&src[i], tx_width,
                                                 /*transpose=*/false);
        i += 8;
      } while (i < tx_width);
    }
  }
  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  StoreToFrameWithRound(frame, start_x, start_y, tx_width, 16, src, tx_type);
}

void Dct32TransformLoopRow_SSE4_1(TransformType /*tx_type*/,
                                  TransformSize tx_size, int adjusted_tx_height,
                                  void* src_buffer, int /*start_x*/,
                                  int /*start_y*/, void* /*dst_frame*/) {
  auto* src = static_cast<int16_t*>(src_buffer);
  const bool should_round = kShouldRound[tx_size];
  const uint8_t row_shift = kTransformRowShift[tx_size];

  if (DctDcOnly<32>(src, adjusted_tx_height, should_round, row_shift)) {
    return;
  }

  if (should_round) {
    ApplyRounding<32>(src, adjusted_tx_height);
  }
  // Process 8 1d dct32 rows in parallel per iteration.
  int i = 0;
  do {
    Dct32_SSE4_1(&src[i * 32], 32, /*transpose=*/true);
    i += 8;
  } while (i < adjusted_tx_height);
  // row_shift is always non zero here.
  RowShift<32>(src, adjusted_tx_height, row_shift);
}

void Dct32TransformLoopColumn_SSE4_1(TransformType tx_type,
                                     TransformSize tx_size,
                                     int adjusted_tx_height,
                                     void* LIBGAV1_RESTRICT src_buffer,
                                     int start_x, int start_y,
                                     void* LIBGAV1_RESTRICT dst_frame) {
  auto* src = static_cast<int16_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (!DctDcOnlyColumn<32>(src, adjusted_tx_height, tx_width)) {
    // Process 8 1d dct32 columns in parallel per iteration.
    int i = 0;
    do {
      Dct32_SSE4_1(&src[i], tx_width, /*transpose=*/false);
      i += 8;
    } while (i < tx_width);
  }
  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  StoreToFrameWithRound(frame, start_x, start_y, tx_width, 32, src, tx_type);
}

void Dct64TransformLoopRow_SSE4_1(TransformType /*tx_type*/,
                                  TransformSize tx_size, int adjusted_tx_height,
                                  void* src_buffer, int /*start_x*/,
                                  int /*start_y*/, void* /*dst_frame*/) {
  auto* src = static_cast<int16_t*>(src_buffer);
  const bool should_round = kShouldRound[tx_size];
  const uint8_t row_shift = kTransformRowShift[tx_size];

  if (DctDcOnly<64>(src, adjusted_tx_height, should_round, row_shift)) {
    return;
  }

  if (should_round) {
    ApplyRounding<64>(src, adjusted_tx_height);
  }
  // Process 8 1d dct64 rows in parallel per iteration.
  int i = 0;
  do {
    Dct64_SSE4_1(&src[i * 64], 64, /*transpose=*/true);
    i += 8;
  } while (i < adjusted_tx_height);
  // row_shift is always non zero here.
  RowShift<64>(src, adjusted_tx_height, row_shift);
}

void Dct64TransformLoopColumn_SSE4_1(TransformType tx_type,
                                     TransformSize tx_size,
                                     int adjusted_tx_height,
                                     void* LIBGAV1_RESTRICT src_buffer,
                                     int start_x, int start_y,
                                     void* LIBGAV1_RESTRICT dst_frame) {
  auto* src = static_cast<int16_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (!DctDcOnlyColumn<64>(src, adjusted_tx_height, tx_width)) {
    // Process 8 1d dct64 columns in parallel per iteration.
    int i = 0;
    do {
      Dct64_SSE4_1(&src[i], tx_width, /*transpose=*/false);
      i += 8;
    } while (i < tx_width);
  }
  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  StoreToFrameWithRound(frame, start_x, start_y, tx_width, 64, src, tx_type);
}

void Adst4TransformLoopRow_SSE4_1(TransformType /*tx_type*/,
                                  TransformSize tx_size, int adjusted_tx_height,
                                  void* src_buffer, int /*start_x*/,
                                  int /*start_y*/, void* /*dst_frame*/) {
  auto* src = static_cast<int16_t*>(src_buffer);
  const int tx_height = kTransformHeight[tx_size];
  const int row_shift = static_cast<int>(tx_height == 16);
  const bool should_round = (tx_height == 8);

  if (Adst4DcOnly(src, adjusted_tx_height, should_round, row_shift)) {
    return;
  }

  if (should_round) {
    ApplyRounding<4>(src, adjusted_tx_height);
  }

  // Process 4 1d adst4 rows in parallel per iteration.
  int i = 0;
  do {
    Adst4_SSE4_1<false>(&src[i * 4], /*step=*/4, /*transpose=*/true);
    i += 4;
  } while (i < adjusted_tx_height);

  if (row_shift != 0) {
    RowShift<4>(src, adjusted_tx_height, 1);
  }
}

void Adst4TransformLoopColumn_SSE4_1(TransformType tx_type,
                                     TransformSize tx_size,
                                     int adjusted_tx_height,
                                     void* LIBGAV1_RESTRICT src_buffer,
                                     int start_x, int start_y,
                                     void* LIBGAV1_RESTRICT dst_frame) {
  auto* src = static_cast<int16_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (kTransformFlipColumnsMask.Contains(tx_type)) {
    FlipColumns<4>(src, tx_width);
  }

  if (!Adst4DcOnlyColumn(src, adjusted_tx_height, tx_width)) {
    // Process 4 1d adst4 columns in parallel per iteration.
    int i = 0;
    do {
      Adst4_SSE4_1<false>(&src[i], tx_width, /*transpose=*/false);
      i += 4;
    } while (i < tx_width);
  }
  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  StoreToFrameWithRound</*enable_flip_rows=*/true>(frame, start_x, start_y,
                                                   tx_width, 4, src, tx_type);
}

void Adst8TransformLoopRow_SSE4_1(TransformType /*tx_type*/,
                                  TransformSize tx_size, int adjusted_tx_height,
                                  void* src_buffer, int /*start_x*/,
                                  int /*start_y*/, void* /*dst_frame*/) {
  auto* src = static_cast<int16_t*>(src_buffer);
  const bool should_round = kShouldRound[tx_size];
  const uint8_t row_shift = kTransformRowShift[tx_size];

  if (Adst8DcOnly(src, adjusted_tx_height, should_round, row_shift)) {
    return;
  }

  if (should_round) {
    ApplyRounding<8>(src, adjusted_tx_height);
  }

  if (adjusted_tx_height <= 4) {
    // Process 4 1d adst8 rows in parallel.
    Adst8_SSE4_1<ButterflyRotation_4, true>(src, /*step=*/8,
                                            /*transpose=*/true);
  } else {
    // Process 8 1d adst8 rows in parallel per iteration.
    int i = 0;
    do {
      Adst8_SSE4_1<ButterflyRotation_8, false>(&src[i * 8], /*step=*/8,
                                               /*transpose=*/true);
      i += 8;
    } while (i < adjusted_tx_height);
  }
  if (row_shift > 0) {
    RowShift<8>(src, adjusted_tx_height, row_shift);
  }
}

void Adst8TransformLoopColumn_SSE4_1(TransformType tx_type,
                                     TransformSize tx_size,
                                     int adjusted_tx_height,
                                     void* LIBGAV1_RESTRICT src_buffer,
                                     int start_x, int start_y,
                                     void* LIBGAV1_RESTRICT dst_frame) {
  auto* src = static_cast<int16_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (kTransformFlipColumnsMask.Contains(tx_type)) {
    FlipColumns<8>(src, tx_width);
  }

  if (!Adst8DcOnlyColumn(src, adjusted_tx_height, tx_width)) {
    if (tx_width == 4) {
      // Process 4 1d adst8 columns in parallel.
      Adst8_SSE4_1<ButterflyRotation_4, true>(src, 4, /*transpose=*/false);
    } else {
      // Process 8 1d adst8 columns in parallel per iteration.
      int i = 0;
      do {
        Adst8_SSE4_1<ButterflyRotation_8, false>(&src[i], tx_width,
                                                 /*transpose=*/false);
        i += 8;
      } while (i < tx_width);
    }
  }
  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  StoreToFrameWithRound</*enable_flip_rows=*/true>(frame, start_x, start_y,
                                                   tx_width, 8, src, tx_type);
}

void Adst16TransformLoopRow_SSE4_1(TransformType /*tx_type*/,
                                   TransformSize tx_size,
                                   int adjusted_tx_height, void* src_buffer,
                                   int /*start_x*/, int /*start_y*/,
                                   void* /*dst_frame*/) {
  auto* src = static_cast<int16_t*>(src_buffer);
  const bool should_round = kShouldRound[tx_size];
  const uint8_t row_shift = kTransformRowShift[tx_size];

  if (Adst16DcOnly(src, adjusted_tx_height, should_round, row_shift)) {
    return;
  }

  if (should_round) {
    ApplyRounding<16>(src, adjusted_tx_height);
  }

  if (adjusted_tx_height <= 4) {
    // Process 4 1d adst16 rows in parallel.
    Adst16_SSE4_1<ButterflyRotation_4, true>(src, 16, /*transpose=*/true);
  } else {
    int i = 0;
    do {
      // Process 8 1d adst16 rows in parallel per iteration.
      Adst16_SSE4_1<ButterflyRotation_8, false>(&src[i * 16], 16,
                                                /*transpose=*/true);
      i += 8;
    } while (i < adjusted_tx_height);
  }
  // row_shift is always non zero here.
  RowShift<16>(src, adjusted_tx_height, row_shift);
}

void Adst16TransformLoopColumn_SSE4_1(TransformType tx_type,
                                      TransformSize tx_size,
                                      int adjusted_tx_height,
                                      void* LIBGAV1_RESTRICT src_buffer,
                                      int start_x, int start_y,
                                      void* LIBGAV1_RESTRICT dst_frame) {
  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  auto* src = static_cast<int16_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (kTransformFlipColumnsMask.Contains(tx_type)) {
    FlipColumns<16>(src, tx_width);
  }

  if (!Adst16DcOnlyColumn(src, adjusted_tx_height, tx_width)) {
    if (tx_width == 4) {
      // Process 4 1d adst16 columns in parallel.
      Adst16_SSE4_1<ButterflyRotation_4, true>(src, 4, /*transpose=*/false);
    } else {
      int i = 0;
      do {
        // Process 8 1d adst16 columns in parallel per iteration.
        Adst16_SSE4_1<ButterflyRotation_8, false>(&src[i], tx_width,
                                                  /*transpose=*/false);
        i += 8;
      } while (i < tx_width);
    }
  }
  StoreToFrameWithRound</*enable_flip_rows=*/true>(frame, start_x, start_y,
                                                   tx_width, 16, src, tx_type);
}

void Identity4TransformLoopRow_SSE4_1(TransformType tx_type,
                                      TransformSize tx_size,
                                      int adjusted_tx_height, void* src_buffer,
                                      int /*start_x*/, int /*start_y*/,
                                      void* /*dst_frame*/) {
  // Special case: Process row calculations during column transform call.
  // Improves performance.
  if (tx_type == kTransformTypeIdentityIdentity &&
      tx_size == kTransformSize4x4) {
    return;
  }

  auto* src = static_cast<int16_t*>(src_buffer);
  const int tx_height = kTransformHeight[tx_size];
  const bool should_round = (tx_height == 8);
  if (Identity4DcOnly(src, adjusted_tx_height, should_round, tx_height)) {
    return;
  }

  if (should_round) {
    ApplyRounding<4>(src, adjusted_tx_height);
  }
  if (tx_height < 16) {
    int i = 0;
    do {
      Identity4_SSE4_1<false>(&src[i * 4], /*step=*/4);
      i += 4;
    } while (i < adjusted_tx_height);
  } else {
    int i = 0;
    do {
      Identity4_SSE4_1<true>(&src[i * 4], /*step=*/4);
      i += 4;
    } while (i < adjusted_tx_height);
  }
}

void Identity4TransformLoopColumn_SSE4_1(TransformType tx_type,
                                         TransformSize tx_size,
                                         int adjusted_tx_height,
                                         void* LIBGAV1_RESTRICT src_buffer,
                                         int start_x, int start_y,
                                         void* LIBGAV1_RESTRICT dst_frame) {
  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  auto* src = static_cast<int16_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  // Special case: Process row calculations during column transform call.
  if (tx_type == kTransformTypeIdentityIdentity &&
      (tx_size == kTransformSize4x4 || tx_size == kTransformSize8x4)) {
    Identity4RowColumnStoreToFrame(frame, start_x, start_y, tx_width,
                                   adjusted_tx_height, src);
    return;
  }

  if (kTransformFlipColumnsMask.Contains(tx_type)) {
    FlipColumns<4>(src, tx_width);
  }

  Identity4ColumnStoreToFrame(frame, start_x, start_y, tx_width,
                              adjusted_tx_height, src);
}

void Identity8TransformLoopRow_SSE4_1(TransformType tx_type,
                                      TransformSize tx_size,
                                      int adjusted_tx_height, void* src_buffer,
                                      int /*start_x*/, int /*start_y*/,
                                      void* /*dst_frame*/) {
  // Special case: Process row calculations during column transform call.
  // Improves performance.
  if (tx_type == kTransformTypeIdentityIdentity &&
      tx_size == kTransformSize8x4) {
    return;
  }

  auto* src = static_cast<int16_t*>(src_buffer);
  const int tx_height = kTransformHeight[tx_size];
  const bool should_round = kShouldRound[tx_size];
  const uint8_t row_shift = kTransformRowShift[tx_size];
  if (Identity8DcOnly(src, adjusted_tx_height, should_round, row_shift)) {
    return;
  }

  if (should_round) {
    ApplyRounding<8>(src, adjusted_tx_height);
  }

  // When combining the identity8 multiplier with the row shift, the
  // calculations for tx_height == 8 and tx_height == 16 can be simplified
  // from ((A * 2) + 1) >> 1) to A.
  if ((tx_height & 0x18) != 0) {
    return;
  }
  if (tx_height == 32) {
    int i = 0;
    do {
      Identity8Row32_SSE4_1(&src[i * 8], /*step=*/8);
      i += 4;
    } while (i < adjusted_tx_height);
    return;
  }

  assert(tx_size == kTransformSize8x4);
  int i = 0;
  do {
    Identity8Row4_SSE4_1(&src[i * 8], /*step=*/8);
    i += 4;
  } while (i < adjusted_tx_height);
}

void Identity8TransformLoopColumn_SSE4_1(TransformType tx_type,
                                         TransformSize tx_size,
                                         int adjusted_tx_height,
                                         void* LIBGAV1_RESTRICT src_buffer,
                                         int start_x, int start_y,
                                         void* LIBGAV1_RESTRICT dst_frame) {
  auto* src = static_cast<int16_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (kTransformFlipColumnsMask.Contains(tx_type)) {
    FlipColumns<8>(src, tx_width);
  }

  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  Identity8ColumnStoreToFrame_SSE4_1(frame, start_x, start_y, tx_width,
                                     adjusted_tx_height, src);
}

void Identity16TransformLoopRow_SSE4_1(TransformType /*tx_type*/,
                                       TransformSize tx_size,
                                       int adjusted_tx_height, void* src_buffer,
                                       int /*start_x*/, int /*start_y*/,
                                       void* /*dst_frame*/) {
  auto* src = static_cast<int16_t*>(src_buffer);
  const bool should_round = kShouldRound[tx_size];
  const uint8_t row_shift = kTransformRowShift[tx_size];
  if (Identity16DcOnly(src, adjusted_tx_height, should_round, row_shift)) {
    return;
  }

  if (should_round) {
    ApplyRounding<16>(src, adjusted_tx_height);
  }
  int i = 0;
  do {
    Identity16Row_SSE4_1(&src[i * 16], /*step=*/16,
                         kTransformRowShift[tx_size]);
    i += 4;
  } while (i < adjusted_tx_height);
}

void Identity16TransformLoopColumn_SSE4_1(TransformType tx_type,
                                          TransformSize tx_size,
                                          int adjusted_tx_height,
                                          void* LIBGAV1_RESTRICT src_buffer,
                                          int start_x, int start_y,
                                          void* LIBGAV1_RESTRICT dst_frame) {
  auto* src = static_cast<int16_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (kTransformFlipColumnsMask.Contains(tx_type)) {
    FlipColumns<16>(src, tx_width);
  }
  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  Identity16ColumnStoreToFrame_SSE4_1(frame, start_x, start_y, tx_width,
                                      adjusted_tx_height, src);
}

void Identity32TransformLoopRow_SSE4_1(TransformType /*tx_type*/,
                                       TransformSize tx_size,
                                       int adjusted_tx_height, void* src_buffer,
                                       int /*start_x*/, int /*start_y*/,
                                       void* /*dst_frame*/) {
  const int tx_height = kTransformHeight[tx_size];
  // When combining the identity32 multiplier with the row shift, the
  // calculations for tx_height == 8 and tx_height == 32 can be simplified
  // from ((A * 4) + 2) >> 2) to A.
  if ((tx_height & 0x28) != 0) {
    return;
  }

  // Process kTransformSize32x16. The src is always rounded before the
  // identity transform and shifted by 1 afterwards.
  auto* src = static_cast<int16_t*>(src_buffer);
  if (Identity32DcOnly(src, adjusted_tx_height)) {
    return;
  }

  assert(tx_size == kTransformSize32x16);
  ApplyRounding<32>(src, adjusted_tx_height);
  int i = 0;
  do {
    Identity32Row16_SSE4_1(&src[i * 32], /*step=*/32);
    i += 4;
  } while (i < adjusted_tx_height);
}

void Identity32TransformLoopColumn_SSE4_1(TransformType /*tx_type*/,
                                          TransformSize tx_size,
                                          int adjusted_tx_height,
                                          void* LIBGAV1_RESTRICT src_buffer,
                                          int start_x, int start_y,
                                          void* LIBGAV1_RESTRICT dst_frame) {
  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  auto* src = static_cast<int16_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  Identity32ColumnStoreToFrame(frame, start_x, start_y, tx_width,
                               adjusted_tx_height, src);
}

void Wht4TransformLoopRow_SSE4_1(TransformType tx_type, TransformSize tx_size,
                                 int /*adjusted_tx_height*/,
                                 void* /*src_buffer*/, int /*start_x*/,
                                 int /*start_y*/, void* /*dst_frame*/) {
  assert(tx_type == kTransformTypeDctDct);
  assert(tx_size == kTransformSize4x4);
  static_cast<void>(tx_type);
  static_cast<void>(tx_size);
  // Do both row and column transforms in the column-transform pass.
}

void Wht4TransformLoopColumn_SSE4_1(TransformType tx_type,
                                    TransformSize tx_size,
                                    int adjusted_tx_height,
                                    void* LIBGAV1_RESTRICT src_buffer,
                                    int start_x, int start_y,
                                    void* LIBGAV1_RESTRICT dst_frame) {
  assert(tx_type == kTransformTypeDctDct);
  assert(tx_size == kTransformSize4x4);
  static_cast<void>(tx_type);
  static_cast<void>(tx_size);

  // Do both row and column transforms in the column-transform pass.
  // Process 4 1d wht4 rows and columns in parallel.
  const auto* src = static_cast<int16_t*>(src_buffer);
  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  Wht4_SSE4_1(frame, start_x, start_y, src, adjusted_tx_height);
}

//------------------------------------------------------------------------------

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);

  // Maximum transform size for Dct is 64.
#if DSP_ENABLED_8BPP_SSE4_1(Transform1dSize4_Transform1dDct)
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize4][kRow] =
      Dct4TransformLoopRow_SSE4_1;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize4][kColumn] =
      Dct4TransformLoopColumn_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(Transform1dSize8_Transform1dDct)
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize8][kRow] =
      Dct8TransformLoopRow_SSE4_1;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize8][kColumn] =
      Dct8TransformLoopColumn_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(Transform1dSize16_Transform1dDct)
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize16][kRow] =
      Dct16TransformLoopRow_SSE4_1;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize16][kColumn] =
      Dct16TransformLoopColumn_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(Transform1dSize32_Transform1dDct)
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize32][kRow] =
      Dct32TransformLoopRow_SSE4_1;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize32][kColumn] =
      Dct32TransformLoopColumn_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(Transform1dSize64_Transform1dDct)
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize64][kRow] =
      Dct64TransformLoopRow_SSE4_1;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize64][kColumn] =
      Dct64TransformLoopColumn_SSE4_1;
#endif

  // Maximum transform size for Adst is 16.
#if DSP_ENABLED_8BPP_SSE4_1(Transform1dSize4_Transform1dAdst)
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize4][kRow] =
      Adst4TransformLoopRow_SSE4_1;
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize4][kColumn] =
      Adst4TransformLoopColumn_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(Transform1dSize8_Transform1dAdst)
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize8][kRow] =
      Adst8TransformLoopRow_SSE4_1;
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize8][kColumn] =
      Adst8TransformLoopColumn_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(Transform1dSize16_Transform1dAdst)
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize16][kRow] =
      Adst16TransformLoopRow_SSE4_1;
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize16][kColumn] =
      Adst16TransformLoopColumn_SSE4_1;
#endif

  // Maximum transform size for Identity transform is 32.
#if DSP_ENABLED_8BPP_SSE4_1(Transform1dSize4_Transform1dIdentity)
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize4][kRow] =
      Identity4TransformLoopRow_SSE4_1;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize4][kColumn] =
      Identity4TransformLoopColumn_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(Transform1dSize8_Transform1dIdentity)
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize8][kRow] =
      Identity8TransformLoopRow_SSE4_1;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize8][kColumn] =
      Identity8TransformLoopColumn_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(Transform1dSize16_Transform1dIdentity)
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize16][kRow] =
      Identity16TransformLoopRow_SSE4_1;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize16][kColumn] =
      Identity16TransformLoopColumn_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(Transform1dSize32_Transform1dIdentity)
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize32][kRow] =
      Identity32TransformLoopRow_SSE4_1;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize32][kColumn] =
      Identity32TransformLoopColumn_SSE4_1;
#endif

  // Maximum transform size for Wht is 4.
#if DSP_ENABLED_8BPP_SSE4_1(Transform1dSize4_Transform1dWht)
  dsp->inverse_transforms[kTransform1dWht][kTransform1dSize4][kRow] =
      Wht4TransformLoopRow_SSE4_1;
  dsp->inverse_transforms[kTransform1dWht][kTransform1dSize4][kColumn] =
      Wht4TransformLoopColumn_SSE4_1;
#endif
}

}  // namespace
}  // namespace low_bitdepth

void InverseTransformInit_SSE4_1() { low_bitdepth::Init8bpp(); }

}  // namespace dsp
}  // namespace libgav1
#else   // !LIBGAV1_TARGETING_SSE4_1
namespace libgav1 {
namespace dsp {

void InverseTransformInit_SSE4_1() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_TARGETING_SSE4_1
