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

#include "src/dsp/inverse_transform.h"
#include "src/utils/cpu.h"

#if LIBGAV1_ENABLE_NEON && LIBGAV1_MAX_BITDEPTH >= 10

#include <arm_neon.h>

#include <algorithm>
#include <cassert>
#include <cstdint>

#include "src/dsp/arm/common_neon.h"
#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/utils/array_2d.h"
#include "src/utils/common.h"
#include "src/utils/compiler_attributes.h"
#include "src/utils/constants.h"

namespace libgav1 {
namespace dsp {
namespace {

// Include the constants and utility functions inside the anonymous namespace.
#include "src/dsp/inverse_transform.inc"

//------------------------------------------------------------------------------

LIBGAV1_ALWAYS_INLINE void Transpose4x4(const int32x4_t in[4],
                                        int32x4_t out[4]) {
  // in:
  // 00 01 02 03
  // 10 11 12 13
  // 20 21 22 23
  // 30 31 32 33

  // 00 10 02 12   a.val[0]
  // 01 11 03 13   a.val[1]
  // 20 30 22 32   b.val[0]
  // 21 31 23 33   b.val[1]
  const int32x4x2_t a = vtrnq_s32(in[0], in[1]);
  const int32x4x2_t b = vtrnq_s32(in[2], in[3]);
  out[0] = vextq_s32(vextq_s32(a.val[0], a.val[0], 2), b.val[0], 2);
  out[1] = vextq_s32(vextq_s32(a.val[1], a.val[1], 2), b.val[1], 2);
  out[2] = vextq_s32(a.val[0], vextq_s32(b.val[0], b.val[0], 2), 2);
  out[3] = vextq_s32(a.val[1], vextq_s32(b.val[1], b.val[1], 2), 2);
  // out:
  // 00 10 20 30
  // 01 11 21 31
  // 02 12 22 32
  // 03 13 23 33
}

//------------------------------------------------------------------------------
template <int store_count>
LIBGAV1_ALWAYS_INLINE void StoreDst(int32_t* LIBGAV1_RESTRICT dst,
                                    int32_t stride, int32_t idx,
                                    const int32x4_t* const s) {
  assert(store_count % 4 == 0);
  for (int i = 0; i < store_count; i += 4) {
    vst1q_s32(&dst[i * stride + idx], s[i]);
    vst1q_s32(&dst[(i + 1) * stride + idx], s[i + 1]);
    vst1q_s32(&dst[(i + 2) * stride + idx], s[i + 2]);
    vst1q_s32(&dst[(i + 3) * stride + idx], s[i + 3]);
  }
}

template <int load_count>
LIBGAV1_ALWAYS_INLINE void LoadSrc(const int32_t* LIBGAV1_RESTRICT src,
                                   int32_t stride, int32_t idx, int32x4_t* x) {
  assert(load_count % 4 == 0);
  for (int i = 0; i < load_count; i += 4) {
    x[i] = vld1q_s32(&src[i * stride + idx]);
    x[i + 1] = vld1q_s32(&src[(i + 1) * stride + idx]);
    x[i + 2] = vld1q_s32(&src[(i + 2) * stride + idx]);
    x[i + 3] = vld1q_s32(&src[(i + 3) * stride + idx]);
  }
}

// Butterfly rotate 4 values.
LIBGAV1_ALWAYS_INLINE void ButterflyRotation_4(int32x4_t* a, int32x4_t* b,
                                               const int angle,
                                               const bool flip) {
  const int32_t cos128 = Cos128(angle);
  const int32_t sin128 = Sin128(angle);
  const int32x4_t acc_x = vmulq_n_s32(*a, cos128);
  const int32x4_t acc_y = vmulq_n_s32(*a, sin128);
  // The max range for the input is 18 bits. The cos128/sin128 is 13 bits,
  // which leaves 1 bit for the add/subtract. For 10bpp, x/y will fit in a 32
  // bit lane.
  const int32x4_t x0 = vmlsq_n_s32(acc_x, *b, sin128);
  const int32x4_t y0 = vmlaq_n_s32(acc_y, *b, cos128);
  const int32x4_t x = vrshrq_n_s32(x0, 12);
  const int32x4_t y = vrshrq_n_s32(y0, 12);
  if (flip) {
    *a = y;
    *b = x;
  } else {
    *a = x;
    *b = y;
  }
}

LIBGAV1_ALWAYS_INLINE void ButterflyRotation_FirstIsZero(int32x4_t* a,
                                                         int32x4_t* b,
                                                         const int angle,
                                                         const bool flip) {
  const int32_t cos128 = Cos128(angle);
  const int32_t sin128 = Sin128(angle);
  assert(sin128 <= 0xfff);
  const int32x4_t x0 = vmulq_n_s32(*b, -sin128);
  const int32x4_t y0 = vmulq_n_s32(*b, cos128);
  const int32x4_t x = vrshrq_n_s32(x0, 12);
  const int32x4_t y = vrshrq_n_s32(y0, 12);
  if (flip) {
    *a = y;
    *b = x;
  } else {
    *a = x;
    *b = y;
  }
}

LIBGAV1_ALWAYS_INLINE void ButterflyRotation_SecondIsZero(int32x4_t* a,
                                                          int32x4_t* b,
                                                          const int angle,
                                                          const bool flip) {
  const int32_t cos128 = Cos128(angle);
  const int32_t sin128 = Sin128(angle);
  const int32x4_t x0 = vmulq_n_s32(*a, cos128);
  const int32x4_t y0 = vmulq_n_s32(*a, sin128);
  const int32x4_t x = vrshrq_n_s32(x0, 12);
  const int32x4_t y = vrshrq_n_s32(y0, 12);
  if (flip) {
    *a = y;
    *b = x;
  } else {
    *a = x;
    *b = y;
  }
}

LIBGAV1_ALWAYS_INLINE void HadamardRotation(int32x4_t* a, int32x4_t* b,
                                            bool flip) {
  int32x4_t x, y;
  if (flip) {
    y = vqaddq_s32(*b, *a);
    x = vqsubq_s32(*b, *a);
  } else {
    x = vqaddq_s32(*a, *b);
    y = vqsubq_s32(*a, *b);
  }
  *a = x;
  *b = y;
}

LIBGAV1_ALWAYS_INLINE void HadamardRotation(int32x4_t* a, int32x4_t* b,
                                            bool flip, const int32x4_t min,
                                            const int32x4_t max) {
  int32x4_t x, y;
  if (flip) {
    y = vqaddq_s32(*b, *a);
    x = vqsubq_s32(*b, *a);
  } else {
    x = vqaddq_s32(*a, *b);
    y = vqsubq_s32(*a, *b);
  }
  *a = vmaxq_s32(vminq_s32(x, max), min);
  *b = vmaxq_s32(vminq_s32(y, max), min);
}

using ButterflyRotationFunc = void (*)(int32x4_t* a, int32x4_t* b, int angle,
                                       bool flip);

//------------------------------------------------------------------------------
// Discrete Cosine Transforms (DCT).

template <int width>
LIBGAV1_ALWAYS_INLINE bool DctDcOnly(void* dest, int adjusted_tx_height,
                                     bool should_round, int row_shift) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int32_t*>(dest);
  const int32x4_t v_src = vdupq_n_s32(dst[0]);
  const uint32x4_t v_mask = vdupq_n_u32(should_round ? 0xffffffff : 0);
  const int32x4_t v_src_round =
      vqrdmulhq_n_s32(v_src, kTransformRowMultiplier << (31 - 12));
  const int32x4_t s0 = vbslq_s32(v_mask, v_src_round, v_src);
  const int32_t cos128 = Cos128(32);
  const int32x4_t xy = vqrdmulhq_n_s32(s0, cos128 << (31 - 12));
  // vqrshlq_s32 will shift right if shift value is negative.
  const int32x4_t xy_shifted = vqrshlq_s32(xy, vdupq_n_s32(-row_shift));
  // Clamp result to signed 16 bits.
  const int32x4_t result = vmovl_s16(vqmovn_s32(xy_shifted));
  if (width == 4) {
    vst1q_s32(dst, result);
  } else {
    for (int i = 0; i < width; i += 4) {
      vst1q_s32(dst, result);
      dst += 4;
    }
  }
  return true;
}

template <int height>
LIBGAV1_ALWAYS_INLINE bool DctDcOnlyColumn(void* dest, int adjusted_tx_height,
                                           int width) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int32_t*>(dest);
  const int32_t cos128 = Cos128(32);

  // Calculate dc values for first row.
  if (width == 4) {
    const int32x4_t v_src = vld1q_s32(dst);
    const int32x4_t xy = vqrdmulhq_n_s32(v_src, cos128 << (31 - 12));
    vst1q_s32(dst, xy);
  } else {
    int i = 0;
    do {
      const int32x4_t v_src = vld1q_s32(&dst[i]);
      const int32x4_t xy = vqrdmulhq_n_s32(v_src, cos128 << (31 - 12));
      vst1q_s32(&dst[i], xy);
      i += 4;
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
LIBGAV1_ALWAYS_INLINE void Dct4Stages(int32x4_t* s, const int32x4_t min,
                                      const int32x4_t max,
                                      const bool is_last_stage) {
  // stage 12.
  if (is_fast_butterfly) {
    ButterflyRotation_SecondIsZero(&s[0], &s[1], 32, true);
    ButterflyRotation_SecondIsZero(&s[2], &s[3], 48, false);
  } else {
    butterfly_rotation(&s[0], &s[1], 32, true);
    butterfly_rotation(&s[2], &s[3], 48, false);
  }

  // stage 17.
  if (is_last_stage) {
    HadamardRotation(&s[0], &s[3], false);
    HadamardRotation(&s[1], &s[2], false);
  } else {
    HadamardRotation(&s[0], &s[3], false, min, max);
    HadamardRotation(&s[1], &s[2], false, min, max);
  }
}

template <ButterflyRotationFunc butterfly_rotation>
LIBGAV1_ALWAYS_INLINE void Dct4_NEON(void* dest, int32_t step, bool is_row,
                                     int row_shift) {
  auto* const dst = static_cast<int32_t*>(dest);
  // When |is_row| is true, set range to the row range, otherwise, set to the
  // column range.
  const int32_t range = is_row ? kBitdepth10 + 7 : 15;
  const int32x4_t min = vdupq_n_s32(-(1 << range));
  const int32x4_t max = vdupq_n_s32((1 << range) - 1);
  int32x4_t s[4], x[4];

  if (is_row) {
    assert(step == 4);
    int32x4x4_t y = vld4q_s32(dst);
    for (int i = 0; i < 4; ++i) x[i] = y.val[i];
  } else {
    LoadSrc<4>(dst, step, 0, x);
  }

  // stage 1.
  // kBitReverseLookup 0, 2, 1, 3
  s[0] = x[0];
  s[1] = x[2];
  s[2] = x[1];
  s[3] = x[3];

  Dct4Stages<butterfly_rotation>(s, min, max, /*is_last_stage=*/true);

  if (is_row) {
    const int32x4_t v_row_shift = vdupq_n_s32(-row_shift);
    for (auto& i : s) {
      i = vmovl_s16(vqmovn_s32(vqrshlq_s32(i, v_row_shift)));
    }
    int32x4x4_t y;
    for (int i = 0; i < 4; ++i) y.val[i] = s[i];
    vst4q_s32(dst, y);
  } else {
    StoreDst<4>(dst, step, 0, s);
  }
}

template <ButterflyRotationFunc butterfly_rotation,
          bool is_fast_butterfly = false>
LIBGAV1_ALWAYS_INLINE void Dct8Stages(int32x4_t* s, const int32x4_t min,
                                      const int32x4_t max,
                                      const bool is_last_stage) {
  // stage 8.
  if (is_fast_butterfly) {
    ButterflyRotation_SecondIsZero(&s[4], &s[7], 56, false);
    ButterflyRotation_FirstIsZero(&s[5], &s[6], 24, false);
  } else {
    butterfly_rotation(&s[4], &s[7], 56, false);
    butterfly_rotation(&s[5], &s[6], 24, false);
  }

  // stage 13.
  HadamardRotation(&s[4], &s[5], false, min, max);
  HadamardRotation(&s[6], &s[7], true, min, max);

  // stage 18.
  butterfly_rotation(&s[6], &s[5], 32, true);

  // stage 22.
  if (is_last_stage) {
    HadamardRotation(&s[0], &s[7], false);
    HadamardRotation(&s[1], &s[6], false);
    HadamardRotation(&s[2], &s[5], false);
    HadamardRotation(&s[3], &s[4], false);
  } else {
    HadamardRotation(&s[0], &s[7], false, min, max);
    HadamardRotation(&s[1], &s[6], false, min, max);
    HadamardRotation(&s[2], &s[5], false, min, max);
    HadamardRotation(&s[3], &s[4], false, min, max);
  }
}

// Process dct8 rows or columns, depending on the |is_row| flag.
template <ButterflyRotationFunc butterfly_rotation>
LIBGAV1_ALWAYS_INLINE void Dct8_NEON(void* dest, int32_t step, bool is_row,
                                     int row_shift) {
  auto* const dst = static_cast<int32_t*>(dest);
  const int32_t range = is_row ? kBitdepth10 + 7 : 15;
  const int32x4_t min = vdupq_n_s32(-(1 << range));
  const int32x4_t max = vdupq_n_s32((1 << range) - 1);
  int32x4_t s[8], x[8];

  if (is_row) {
    LoadSrc<4>(dst, step, 0, &x[0]);
    LoadSrc<4>(dst, step, 4, &x[4]);
    Transpose4x4(&x[0], &x[0]);
    Transpose4x4(&x[4], &x[4]);
  } else {
    LoadSrc<8>(dst, step, 0, &x[0]);
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

  Dct4Stages<butterfly_rotation>(s, min, max, /*is_last_stage=*/false);
  Dct8Stages<butterfly_rotation>(s, min, max, /*is_last_stage=*/true);

  if (is_row) {
    const int32x4_t v_row_shift = vdupq_n_s32(-row_shift);
    for (auto& i : s) {
      i = vmovl_s16(vqmovn_s32(vqrshlq_s32(i, v_row_shift)));
    }
    Transpose4x4(&s[0], &s[0]);
    Transpose4x4(&s[4], &s[4]);
    StoreDst<4>(dst, step, 0, &s[0]);
    StoreDst<4>(dst, step, 4, &s[4]);
  } else {
    StoreDst<8>(dst, step, 0, &s[0]);
  }
}

template <ButterflyRotationFunc butterfly_rotation,
          bool is_fast_butterfly = false>
LIBGAV1_ALWAYS_INLINE void Dct16Stages(int32x4_t* s, const int32x4_t min,
                                       const int32x4_t max,
                                       const bool is_last_stage) {
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
  HadamardRotation(&s[8], &s[9], false, min, max);
  HadamardRotation(&s[10], &s[11], true, min, max);
  HadamardRotation(&s[12], &s[13], false, min, max);
  HadamardRotation(&s[14], &s[15], true, min, max);

  // stage 14.
  butterfly_rotation(&s[14], &s[9], 48, true);
  butterfly_rotation(&s[13], &s[10], 112, true);

  // stage 19.
  HadamardRotation(&s[8], &s[11], false, min, max);
  HadamardRotation(&s[9], &s[10], false, min, max);
  HadamardRotation(&s[12], &s[15], true, min, max);
  HadamardRotation(&s[13], &s[14], true, min, max);

  // stage 23.
  butterfly_rotation(&s[13], &s[10], 32, true);
  butterfly_rotation(&s[12], &s[11], 32, true);

  // stage 26.
  if (is_last_stage) {
    HadamardRotation(&s[0], &s[15], false);
    HadamardRotation(&s[1], &s[14], false);
    HadamardRotation(&s[2], &s[13], false);
    HadamardRotation(&s[3], &s[12], false);
    HadamardRotation(&s[4], &s[11], false);
    HadamardRotation(&s[5], &s[10], false);
    HadamardRotation(&s[6], &s[9], false);
    HadamardRotation(&s[7], &s[8], false);
  } else {
    HadamardRotation(&s[0], &s[15], false, min, max);
    HadamardRotation(&s[1], &s[14], false, min, max);
    HadamardRotation(&s[2], &s[13], false, min, max);
    HadamardRotation(&s[3], &s[12], false, min, max);
    HadamardRotation(&s[4], &s[11], false, min, max);
    HadamardRotation(&s[5], &s[10], false, min, max);
    HadamardRotation(&s[6], &s[9], false, min, max);
    HadamardRotation(&s[7], &s[8], false, min, max);
  }
}

// Process dct16 rows or columns, depending on the |is_row| flag.
template <ButterflyRotationFunc butterfly_rotation>
LIBGAV1_ALWAYS_INLINE void Dct16_NEON(void* dest, int32_t step, bool is_row,
                                      int row_shift) {
  auto* const dst = static_cast<int32_t*>(dest);
  const int32_t range = is_row ? kBitdepth10 + 7 : 15;
  const int32x4_t min = vdupq_n_s32(-(1 << range));
  const int32x4_t max = vdupq_n_s32((1 << range) - 1);
  int32x4_t s[16], x[16];

  if (is_row) {
    for (int idx = 0; idx < 16; idx += 8) {
      LoadSrc<4>(dst, step, idx, &x[idx]);
      LoadSrc<4>(dst, step, idx + 4, &x[idx + 4]);
      Transpose4x4(&x[idx], &x[idx]);
      Transpose4x4(&x[idx + 4], &x[idx + 4]);
    }
  } else {
    LoadSrc<16>(dst, step, 0, &x[0]);
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

  Dct4Stages<butterfly_rotation>(s, min, max, /*is_last_stage=*/false);
  Dct8Stages<butterfly_rotation>(s, min, max, /*is_last_stage=*/false);
  Dct16Stages<butterfly_rotation>(s, min, max, /*is_last_stage=*/true);

  if (is_row) {
    const int32x4_t v_row_shift = vdupq_n_s32(-row_shift);
    for (auto& i : s) {
      i = vmovl_s16(vqmovn_s32(vqrshlq_s32(i, v_row_shift)));
    }
    for (int idx = 0; idx < 16; idx += 8) {
      Transpose4x4(&s[idx], &s[idx]);
      Transpose4x4(&s[idx + 4], &s[idx + 4]);
      StoreDst<4>(dst, step, idx, &s[idx]);
      StoreDst<4>(dst, step, idx + 4, &s[idx + 4]);
    }
  } else {
    StoreDst<16>(dst, step, 0, &s[0]);
  }
}

template <ButterflyRotationFunc butterfly_rotation,
          bool is_fast_butterfly = false>
LIBGAV1_ALWAYS_INLINE void Dct32Stages(int32x4_t* s, const int32x4_t min,
                                       const int32x4_t max,
                                       const bool is_last_stage) {
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
  HadamardRotation(&s[16], &s[17], false, min, max);
  HadamardRotation(&s[18], &s[19], true, min, max);
  HadamardRotation(&s[20], &s[21], false, min, max);
  HadamardRotation(&s[22], &s[23], true, min, max);
  HadamardRotation(&s[24], &s[25], false, min, max);
  HadamardRotation(&s[26], &s[27], true, min, max);
  HadamardRotation(&s[28], &s[29], false, min, max);
  HadamardRotation(&s[30], &s[31], true, min, max);

  // stage 10.
  butterfly_rotation(&s[30], &s[17], 24 + 32, true);
  butterfly_rotation(&s[29], &s[18], 24 + 64 + 32, true);
  butterfly_rotation(&s[26], &s[21], 24, true);
  butterfly_rotation(&s[25], &s[22], 24 + 64, true);

  // stage 15.
  HadamardRotation(&s[16], &s[19], false, min, max);
  HadamardRotation(&s[17], &s[18], false, min, max);
  HadamardRotation(&s[20], &s[23], true, min, max);
  HadamardRotation(&s[21], &s[22], true, min, max);
  HadamardRotation(&s[24], &s[27], false, min, max);
  HadamardRotation(&s[25], &s[26], false, min, max);
  HadamardRotation(&s[28], &s[31], true, min, max);
  HadamardRotation(&s[29], &s[30], true, min, max);

  // stage 20.
  butterfly_rotation(&s[29], &s[18], 48, true);
  butterfly_rotation(&s[28], &s[19], 48, true);
  butterfly_rotation(&s[27], &s[20], 48 + 64, true);
  butterfly_rotation(&s[26], &s[21], 48 + 64, true);

  // stage 24.
  HadamardRotation(&s[16], &s[23], false, min, max);
  HadamardRotation(&s[17], &s[22], false, min, max);
  HadamardRotation(&s[18], &s[21], false, min, max);
  HadamardRotation(&s[19], &s[20], false, min, max);
  HadamardRotation(&s[24], &s[31], true, min, max);
  HadamardRotation(&s[25], &s[30], true, min, max);
  HadamardRotation(&s[26], &s[29], true, min, max);
  HadamardRotation(&s[27], &s[28], true, min, max);

  // stage 27.
  butterfly_rotation(&s[27], &s[20], 32, true);
  butterfly_rotation(&s[26], &s[21], 32, true);
  butterfly_rotation(&s[25], &s[22], 32, true);
  butterfly_rotation(&s[24], &s[23], 32, true);

  // stage 29.
  if (is_last_stage) {
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
  } else {
    HadamardRotation(&s[0], &s[31], false, min, max);
    HadamardRotation(&s[1], &s[30], false, min, max);
    HadamardRotation(&s[2], &s[29], false, min, max);
    HadamardRotation(&s[3], &s[28], false, min, max);
    HadamardRotation(&s[4], &s[27], false, min, max);
    HadamardRotation(&s[5], &s[26], false, min, max);
    HadamardRotation(&s[6], &s[25], false, min, max);
    HadamardRotation(&s[7], &s[24], false, min, max);
    HadamardRotation(&s[8], &s[23], false, min, max);
    HadamardRotation(&s[9], &s[22], false, min, max);
    HadamardRotation(&s[10], &s[21], false, min, max);
    HadamardRotation(&s[11], &s[20], false, min, max);
    HadamardRotation(&s[12], &s[19], false, min, max);
    HadamardRotation(&s[13], &s[18], false, min, max);
    HadamardRotation(&s[14], &s[17], false, min, max);
    HadamardRotation(&s[15], &s[16], false, min, max);
  }
}

// Process dct32 rows or columns, depending on the |is_row| flag.
LIBGAV1_ALWAYS_INLINE void Dct32_NEON(void* dest, const int32_t step,
                                      const bool is_row, int row_shift) {
  auto* const dst = static_cast<int32_t*>(dest);
  const int32_t range = is_row ? kBitdepth10 + 7 : 15;
  const int32x4_t min = vdupq_n_s32(-(1 << range));
  const int32x4_t max = vdupq_n_s32((1 << range) - 1);
  int32x4_t s[32], x[32];

  if (is_row) {
    for (int idx = 0; idx < 32; idx += 8) {
      LoadSrc<4>(dst, step, idx, &x[idx]);
      LoadSrc<4>(dst, step, idx + 4, &x[idx + 4]);
      Transpose4x4(&x[idx], &x[idx]);
      Transpose4x4(&x[idx + 4], &x[idx + 4]);
    }
  } else {
    LoadSrc<32>(dst, step, 0, &x[0]);
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

  Dct4Stages<ButterflyRotation_4>(s, min, max, /*is_last_stage=*/false);
  Dct8Stages<ButterflyRotation_4>(s, min, max, /*is_last_stage=*/false);
  Dct16Stages<ButterflyRotation_4>(s, min, max, /*is_last_stage=*/false);
  Dct32Stages<ButterflyRotation_4>(s, min, max, /*is_last_stage=*/true);

  if (is_row) {
    const int32x4_t v_row_shift = vdupq_n_s32(-row_shift);
    for (int idx = 0; idx < 32; idx += 8) {
      int32x4_t output[8];
      Transpose4x4(&s[idx], &output[0]);
      Transpose4x4(&s[idx + 4], &output[4]);
      for (auto& o : output) {
        o = vmovl_s16(vqmovn_s32(vqrshlq_s32(o, v_row_shift)));
      }
      StoreDst<4>(dst, step, idx, &output[0]);
      StoreDst<4>(dst, step, idx + 4, &output[4]);
    }
  } else {
    StoreDst<32>(dst, step, 0, &s[0]);
  }
}

void Dct64_NEON(void* dest, int32_t step, bool is_row, int row_shift) {
  auto* const dst = static_cast<int32_t*>(dest);
  const int32_t range = is_row ? kBitdepth10 + 7 : 15;
  const int32x4_t min = vdupq_n_s32(-(1 << range));
  const int32x4_t max = vdupq_n_s32((1 << range) - 1);
  int32x4_t s[64], x[32];

  if (is_row) {
    // The last 32 values of every row are always zero if the |tx_width| is
    // 64.
    for (int idx = 0; idx < 32; idx += 8) {
      LoadSrc<4>(dst, step, idx, &x[idx]);
      LoadSrc<4>(dst, step, idx + 4, &x[idx + 4]);
      Transpose4x4(&x[idx], &x[idx]);
      Transpose4x4(&x[idx + 4], &x[idx + 4]);
    }
  } else {
    // The last 32 values of every column are always zero if the |tx_height| is
    // 64.
    LoadSrc<32>(dst, step, 0, &x[0]);
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

  Dct4Stages<ButterflyRotation_4, /*is_fast_butterfly=*/true>(
      s, min, max, /*is_last_stage=*/false);
  Dct8Stages<ButterflyRotation_4, /*is_fast_butterfly=*/true>(
      s, min, max, /*is_last_stage=*/false);
  Dct16Stages<ButterflyRotation_4, /*is_fast_butterfly=*/true>(
      s, min, max, /*is_last_stage=*/false);
  Dct32Stages<ButterflyRotation_4, /*is_fast_butterfly=*/true>(
      s, min, max, /*is_last_stage=*/false);

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
  HadamardRotation(&s[32], &s[33], false, min, max);
  HadamardRotation(&s[34], &s[35], true, min, max);
  HadamardRotation(&s[36], &s[37], false, min, max);
  HadamardRotation(&s[38], &s[39], true, min, max);
  HadamardRotation(&s[40], &s[41], false, min, max);
  HadamardRotation(&s[42], &s[43], true, min, max);
  HadamardRotation(&s[44], &s[45], false, min, max);
  HadamardRotation(&s[46], &s[47], true, min, max);
  HadamardRotation(&s[48], &s[49], false, min, max);
  HadamardRotation(&s[50], &s[51], true, min, max);
  HadamardRotation(&s[52], &s[53], false, min, max);
  HadamardRotation(&s[54], &s[55], true, min, max);
  HadamardRotation(&s[56], &s[57], false, min, max);
  HadamardRotation(&s[58], &s[59], true, min, max);
  HadamardRotation(&s[60], &s[61], false, min, max);
  HadamardRotation(&s[62], &s[63], true, min, max);

  // stage 7.
  ButterflyRotation_4(&s[62], &s[33], 60 - 0, true);
  ButterflyRotation_4(&s[61], &s[34], 60 - 0 + 64, true);
  ButterflyRotation_4(&s[58], &s[37], 60 - 32, true);
  ButterflyRotation_4(&s[57], &s[38], 60 - 32 + 64, true);
  ButterflyRotation_4(&s[54], &s[41], 60 - 16, true);
  ButterflyRotation_4(&s[53], &s[42], 60 - 16 + 64, true);
  ButterflyRotation_4(&s[50], &s[45], 60 - 48, true);
  ButterflyRotation_4(&s[49], &s[46], 60 - 48 + 64, true);

  // stage 11.
  HadamardRotation(&s[32], &s[35], false, min, max);
  HadamardRotation(&s[33], &s[34], false, min, max);
  HadamardRotation(&s[36], &s[39], true, min, max);
  HadamardRotation(&s[37], &s[38], true, min, max);
  HadamardRotation(&s[40], &s[43], false, min, max);
  HadamardRotation(&s[41], &s[42], false, min, max);
  HadamardRotation(&s[44], &s[47], true, min, max);
  HadamardRotation(&s[45], &s[46], true, min, max);
  HadamardRotation(&s[48], &s[51], false, min, max);
  HadamardRotation(&s[49], &s[50], false, min, max);
  HadamardRotation(&s[52], &s[55], true, min, max);
  HadamardRotation(&s[53], &s[54], true, min, max);
  HadamardRotation(&s[56], &s[59], false, min, max);
  HadamardRotation(&s[57], &s[58], false, min, max);
  HadamardRotation(&s[60], &s[63], true, min, max);
  HadamardRotation(&s[61], &s[62], true, min, max);

  // stage 16.
  ButterflyRotation_4(&s[61], &s[34], 56, true);
  ButterflyRotation_4(&s[60], &s[35], 56, true);
  ButterflyRotation_4(&s[59], &s[36], 56 + 64, true);
  ButterflyRotation_4(&s[58], &s[37], 56 + 64, true);
  ButterflyRotation_4(&s[53], &s[42], 56 - 32, true);
  ButterflyRotation_4(&s[52], &s[43], 56 - 32, true);
  ButterflyRotation_4(&s[51], &s[44], 56 - 32 + 64, true);
  ButterflyRotation_4(&s[50], &s[45], 56 - 32 + 64, true);

  // stage 21.
  HadamardRotation(&s[32], &s[39], false, min, max);
  HadamardRotation(&s[33], &s[38], false, min, max);
  HadamardRotation(&s[34], &s[37], false, min, max);
  HadamardRotation(&s[35], &s[36], false, min, max);
  HadamardRotation(&s[40], &s[47], true, min, max);
  HadamardRotation(&s[41], &s[46], true, min, max);
  HadamardRotation(&s[42], &s[45], true, min, max);
  HadamardRotation(&s[43], &s[44], true, min, max);
  HadamardRotation(&s[48], &s[55], false, min, max);
  HadamardRotation(&s[49], &s[54], false, min, max);
  HadamardRotation(&s[50], &s[53], false, min, max);
  HadamardRotation(&s[51], &s[52], false, min, max);
  HadamardRotation(&s[56], &s[63], true, min, max);
  HadamardRotation(&s[57], &s[62], true, min, max);
  HadamardRotation(&s[58], &s[61], true, min, max);
  HadamardRotation(&s[59], &s[60], true, min, max);

  // stage 25.
  ButterflyRotation_4(&s[59], &s[36], 48, true);
  ButterflyRotation_4(&s[58], &s[37], 48, true);
  ButterflyRotation_4(&s[57], &s[38], 48, true);
  ButterflyRotation_4(&s[56], &s[39], 48, true);
  ButterflyRotation_4(&s[55], &s[40], 112, true);
  ButterflyRotation_4(&s[54], &s[41], 112, true);
  ButterflyRotation_4(&s[53], &s[42], 112, true);
  ButterflyRotation_4(&s[52], &s[43], 112, true);

  // stage 28.
  HadamardRotation(&s[32], &s[47], false, min, max);
  HadamardRotation(&s[33], &s[46], false, min, max);
  HadamardRotation(&s[34], &s[45], false, min, max);
  HadamardRotation(&s[35], &s[44], false, min, max);
  HadamardRotation(&s[36], &s[43], false, min, max);
  HadamardRotation(&s[37], &s[42], false, min, max);
  HadamardRotation(&s[38], &s[41], false, min, max);
  HadamardRotation(&s[39], &s[40], false, min, max);
  HadamardRotation(&s[48], &s[63], true, min, max);
  HadamardRotation(&s[49], &s[62], true, min, max);
  HadamardRotation(&s[50], &s[61], true, min, max);
  HadamardRotation(&s[51], &s[60], true, min, max);
  HadamardRotation(&s[52], &s[59], true, min, max);
  HadamardRotation(&s[53], &s[58], true, min, max);
  HadamardRotation(&s[54], &s[57], true, min, max);
  HadamardRotation(&s[55], &s[56], true, min, max);

  // stage 30.
  ButterflyRotation_4(&s[55], &s[40], 32, true);
  ButterflyRotation_4(&s[54], &s[41], 32, true);
  ButterflyRotation_4(&s[53], &s[42], 32, true);
  ButterflyRotation_4(&s[52], &s[43], 32, true);
  ButterflyRotation_4(&s[51], &s[44], 32, true);
  ButterflyRotation_4(&s[50], &s[45], 32, true);
  ButterflyRotation_4(&s[49], &s[46], 32, true);
  ButterflyRotation_4(&s[48], &s[47], 32, true);

  // stage 31.
  for (int i = 0; i < 32; i += 4) {
    HadamardRotation(&s[i], &s[63 - i], false, min, max);
    HadamardRotation(&s[i + 1], &s[63 - i - 1], false, min, max);
    HadamardRotation(&s[i + 2], &s[63 - i - 2], false, min, max);
    HadamardRotation(&s[i + 3], &s[63 - i - 3], false, min, max);
  }
  //-- end dct 64 stages
  if (is_row) {
    const int32x4_t v_row_shift = vdupq_n_s32(-row_shift);
    for (int idx = 0; idx < 64; idx += 8) {
      int32x4_t output[8];
      Transpose4x4(&s[idx], &output[0]);
      Transpose4x4(&s[idx + 4], &output[4]);
      for (auto& o : output) {
        o = vmovl_s16(vqmovn_s32(vqrshlq_s32(o, v_row_shift)));
      }
      StoreDst<4>(dst, step, idx, &output[0]);
      StoreDst<4>(dst, step, idx + 4, &output[4]);
    }
  } else {
    StoreDst<64>(dst, step, 0, &s[0]);
  }
}

//------------------------------------------------------------------------------
// Asymmetric Discrete Sine Transforms (ADST).
LIBGAV1_ALWAYS_INLINE void Adst4_NEON(void* dest, int32_t step, bool is_row,
                                      int row_shift) {
  auto* const dst = static_cast<int32_t*>(dest);
  int32x4_t s[8];
  int32x4_t x[4];

  if (is_row) {
    assert(step == 4);
    int32x4x4_t y = vld4q_s32(dst);
    for (int i = 0; i < 4; ++i) x[i] = y.val[i];
  } else {
    LoadSrc<4>(dst, step, 0, x);
  }

  // stage 1.
  s[5] = vmulq_n_s32(x[3], kAdst4Multiplier[1]);
  s[6] = vmulq_n_s32(x[3], kAdst4Multiplier[3]);

  // stage 2.
  const int32x4_t a7 = vsubq_s32(x[0], x[2]);
  const int32x4_t b7 = vaddq_s32(a7, x[3]);

  // stage 3.
  s[0] = vmulq_n_s32(x[0], kAdst4Multiplier[0]);
  s[1] = vmulq_n_s32(x[0], kAdst4Multiplier[1]);
  // s[0] = s[0] + s[3]
  s[0] = vmlaq_n_s32(s[0], x[2], kAdst4Multiplier[3]);
  // s[1] = s[1] - s[4]
  s[1] = vmlsq_n_s32(s[1], x[2], kAdst4Multiplier[0]);

  s[3] = vmulq_n_s32(x[1], kAdst4Multiplier[2]);
  s[2] = vmulq_n_s32(b7, kAdst4Multiplier[2]);

  // stage 4.
  s[0] = vaddq_s32(s[0], s[5]);
  s[1] = vsubq_s32(s[1], s[6]);

  // stages 5 and 6.
  const int32x4_t x0 = vaddq_s32(s[0], s[3]);
  const int32x4_t x1 = vaddq_s32(s[1], s[3]);
  const int32x4_t x3_a = vaddq_s32(s[0], s[1]);
  const int32x4_t x3 = vsubq_s32(x3_a, s[3]);
  x[0] = vrshrq_n_s32(x0, 12);
  x[1] = vrshrq_n_s32(x1, 12);
  x[2] = vrshrq_n_s32(s[2], 12);
  x[3] = vrshrq_n_s32(x3, 12);

  if (is_row) {
    const int32x4_t v_row_shift = vdupq_n_s32(-row_shift);
    x[0] = vmovl_s16(vqmovn_s32(vqrshlq_s32(x[0], v_row_shift)));
    x[1] = vmovl_s16(vqmovn_s32(vqrshlq_s32(x[1], v_row_shift)));
    x[2] = vmovl_s16(vqmovn_s32(vqrshlq_s32(x[2], v_row_shift)));
    x[3] = vmovl_s16(vqmovn_s32(vqrshlq_s32(x[3], v_row_shift)));
    int32x4x4_t y;
    for (int i = 0; i < 4; ++i) y.val[i] = x[i];
    vst4q_s32(dst, y);
  } else {
    StoreDst<4>(dst, step, 0, x);
  }
}

alignas(16) constexpr int32_t kAdst4DcOnlyMultiplier[4] = {1321, 2482, 3344,
                                                           2482};

LIBGAV1_ALWAYS_INLINE bool Adst4DcOnly(void* dest, int adjusted_tx_height,
                                       bool should_round, int row_shift) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int32_t*>(dest);
  int32x4_t s[2];

  const int32x4_t v_src0 = vdupq_n_s32(dst[0]);
  const uint32x4_t v_mask = vdupq_n_u32(should_round ? 0xffffffff : 0);
  const int32x4_t v_src0_round =
      vqrdmulhq_n_s32(v_src0, kTransformRowMultiplier << (31 - 12));

  const int32x4_t v_src = vbslq_s32(v_mask, v_src0_round, v_src0);
  const int32x4_t kAdst4DcOnlyMultipliers = vld1q_s32(kAdst4DcOnlyMultiplier);
  s[1] = vdupq_n_s32(0);

  // s0*k0 s0*k1 s0*k2 s0*k1
  s[0] = vmulq_s32(kAdst4DcOnlyMultipliers, v_src);
  // 0     0     0     s0*k0
  s[1] = vextq_s32(s[1], s[0], 1);

  const int32x4_t x3 = vaddq_s32(s[0], s[1]);
  const int32x4_t dst_0 = vrshrq_n_s32(x3, 12);

  // vqrshlq_s32 will shift right if shift value is negative.
  vst1q_s32(dst,
            vmovl_s16(vqmovn_s32(vqrshlq_s32(dst_0, vdupq_n_s32(-row_shift)))));

  return true;
}

LIBGAV1_ALWAYS_INLINE bool Adst4DcOnlyColumn(void* dest, int adjusted_tx_height,
                                             int width) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int32_t*>(dest);
  int32x4_t s[4];

  int i = 0;
  do {
    const int32x4_t v_src = vld1q_s32(&dst[i]);

    s[0] = vmulq_n_s32(v_src, kAdst4Multiplier[0]);
    s[1] = vmulq_n_s32(v_src, kAdst4Multiplier[1]);
    s[2] = vmulq_n_s32(v_src, kAdst4Multiplier[2]);

    const int32x4_t x0 = s[0];
    const int32x4_t x1 = s[1];
    const int32x4_t x2 = s[2];
    const int32x4_t x3 = vaddq_s32(s[0], s[1]);
    const int32x4_t dst_0 = vrshrq_n_s32(x0, 12);
    const int32x4_t dst_1 = vrshrq_n_s32(x1, 12);
    const int32x4_t dst_2 = vrshrq_n_s32(x2, 12);
    const int32x4_t dst_3 = vrshrq_n_s32(x3, 12);

    vst1q_s32(&dst[i], dst_0);
    vst1q_s32(&dst[i + width * 1], dst_1);
    vst1q_s32(&dst[i + width * 2], dst_2);
    vst1q_s32(&dst[i + width * 3], dst_3);

    i += 4;
  } while (i < width);

  return true;
}

template <ButterflyRotationFunc butterfly_rotation>
LIBGAV1_ALWAYS_INLINE void Adst8_NEON(void* dest, int32_t step, bool is_row,
                                      int row_shift) {
  auto* const dst = static_cast<int32_t*>(dest);
  const int32_t range = is_row ? kBitdepth10 + 7 : 15;
  const int32x4_t min = vdupq_n_s32(-(1 << range));
  const int32x4_t max = vdupq_n_s32((1 << range) - 1);
  int32x4_t s[8], x[8];

  if (is_row) {
    LoadSrc<4>(dst, step, 0, &x[0]);
    LoadSrc<4>(dst, step, 4, &x[4]);
    Transpose4x4(&x[0], &x[0]);
    Transpose4x4(&x[4], &x[4]);
  } else {
    LoadSrc<8>(dst, step, 0, &x[0]);
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
  HadamardRotation(&s[0], &s[4], false, min, max);
  HadamardRotation(&s[1], &s[5], false, min, max);
  HadamardRotation(&s[2], &s[6], false, min, max);
  HadamardRotation(&s[3], &s[7], false, min, max);

  // stage 4.
  butterfly_rotation(&s[4], &s[5], 48 - 0, true);
  butterfly_rotation(&s[7], &s[6], 48 - 32, true);

  // stage 5.
  HadamardRotation(&s[0], &s[2], false, min, max);
  HadamardRotation(&s[4], &s[6], false, min, max);
  HadamardRotation(&s[1], &s[3], false, min, max);
  HadamardRotation(&s[5], &s[7], false, min, max);

  // stage 6.
  butterfly_rotation(&s[2], &s[3], 32, true);
  butterfly_rotation(&s[6], &s[7], 32, true);

  // stage 7.
  x[0] = s[0];
  x[1] = vqnegq_s32(s[4]);
  x[2] = s[6];
  x[3] = vqnegq_s32(s[2]);
  x[4] = s[3];
  x[5] = vqnegq_s32(s[7]);
  x[6] = s[5];
  x[7] = vqnegq_s32(s[1]);

  if (is_row) {
    const int32x4_t v_row_shift = vdupq_n_s32(-row_shift);
    for (auto& i : x) {
      i = vmovl_s16(vqmovn_s32(vqrshlq_s32(i, v_row_shift)));
    }
    Transpose4x4(&x[0], &x[0]);
    Transpose4x4(&x[4], &x[4]);
    StoreDst<4>(dst, step, 0, &x[0]);
    StoreDst<4>(dst, step, 4, &x[4]);
  } else {
    StoreDst<8>(dst, step, 0, &x[0]);
  }
}

LIBGAV1_ALWAYS_INLINE bool Adst8DcOnly(void* dest, int adjusted_tx_height,
                                       bool should_round, int row_shift) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int32_t*>(dest);
  int32x4_t s[8];

  const int32x4_t v_src = vdupq_n_s32(dst[0]);
  const uint32x4_t v_mask = vdupq_n_u32(should_round ? 0xffffffff : 0);
  const int32x4_t v_src_round =
      vqrdmulhq_n_s32(v_src, kTransformRowMultiplier << (31 - 12));
  // stage 1.
  s[1] = vbslq_s32(v_mask, v_src_round, v_src);

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
  int32x4_t x[8];
  x[0] = s[0];
  x[1] = vqnegq_s32(s[4]);
  x[2] = s[6];
  x[3] = vqnegq_s32(s[2]);
  x[4] = s[3];
  x[5] = vqnegq_s32(s[7]);
  x[6] = s[5];
  x[7] = vqnegq_s32(s[1]);

  for (int i = 0; i < 8; ++i) {
    // vqrshlq_s32 will shift right if shift value is negative.
    x[i] = vmovl_s16(vqmovn_s32(vqrshlq_s32(x[i], vdupq_n_s32(-row_shift))));
    vst1q_lane_s32(&dst[i], x[i], 0);
  }

  return true;
}

LIBGAV1_ALWAYS_INLINE bool Adst8DcOnlyColumn(void* dest, int adjusted_tx_height,
                                             int width) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int32_t*>(dest);
  int32x4_t s[8];

  int i = 0;
  do {
    const int32x4_t v_src = vld1q_s32(dst);
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
    int32x4_t x[8];
    x[0] = s[0];
    x[1] = vqnegq_s32(s[4]);
    x[2] = s[6];
    x[3] = vqnegq_s32(s[2]);
    x[4] = s[3];
    x[5] = vqnegq_s32(s[7]);
    x[6] = s[5];
    x[7] = vqnegq_s32(s[1]);

    for (int j = 0; j < 8; ++j) {
      vst1q_s32(&dst[j * width], x[j]);
    }
    i += 4;
    dst += 4;
  } while (i < width);

  return true;
}

template <ButterflyRotationFunc butterfly_rotation>
LIBGAV1_ALWAYS_INLINE void Adst16_NEON(void* dest, int32_t step, bool is_row,
                                       int row_shift) {
  auto* const dst = static_cast<int32_t*>(dest);
  const int32_t range = is_row ? kBitdepth10 + 7 : 15;
  const int32x4_t min = vdupq_n_s32(-(1 << range));
  const int32x4_t max = vdupq_n_s32((1 << range) - 1);
  int32x4_t s[16], x[16];

  if (is_row) {
    for (int idx = 0; idx < 16; idx += 8) {
      LoadSrc<4>(dst, step, idx, &x[idx]);
      LoadSrc<4>(dst, step, idx + 4, &x[idx + 4]);
      Transpose4x4(&x[idx], &x[idx]);
      Transpose4x4(&x[idx + 4], &x[idx + 4]);
    }
  } else {
    LoadSrc<16>(dst, step, 0, &x[0]);
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
  HadamardRotation(&s[0], &s[8], false, min, max);
  HadamardRotation(&s[1], &s[9], false, min, max);
  HadamardRotation(&s[2], &s[10], false, min, max);
  HadamardRotation(&s[3], &s[11], false, min, max);
  HadamardRotation(&s[4], &s[12], false, min, max);
  HadamardRotation(&s[5], &s[13], false, min, max);
  HadamardRotation(&s[6], &s[14], false, min, max);
  HadamardRotation(&s[7], &s[15], false, min, max);

  // stage 4.
  butterfly_rotation(&s[8], &s[9], 56 - 0, true);
  butterfly_rotation(&s[13], &s[12], 8 + 0, true);
  butterfly_rotation(&s[10], &s[11], 56 - 32, true);
  butterfly_rotation(&s[15], &s[14], 8 + 32, true);

  // stage 5.
  HadamardRotation(&s[0], &s[4], false, min, max);
  HadamardRotation(&s[8], &s[12], false, min, max);
  HadamardRotation(&s[1], &s[5], false, min, max);
  HadamardRotation(&s[9], &s[13], false, min, max);
  HadamardRotation(&s[2], &s[6], false, min, max);
  HadamardRotation(&s[10], &s[14], false, min, max);
  HadamardRotation(&s[3], &s[7], false, min, max);
  HadamardRotation(&s[11], &s[15], false, min, max);

  // stage 6.
  butterfly_rotation(&s[4], &s[5], 48 - 0, true);
  butterfly_rotation(&s[12], &s[13], 48 - 0, true);
  butterfly_rotation(&s[7], &s[6], 48 - 32, true);
  butterfly_rotation(&s[15], &s[14], 48 - 32, true);

  // stage 7.
  HadamardRotation(&s[0], &s[2], false, min, max);
  HadamardRotation(&s[4], &s[6], false, min, max);
  HadamardRotation(&s[8], &s[10], false, min, max);
  HadamardRotation(&s[12], &s[14], false, min, max);
  HadamardRotation(&s[1], &s[3], false, min, max);
  HadamardRotation(&s[5], &s[7], false, min, max);
  HadamardRotation(&s[9], &s[11], false, min, max);
  HadamardRotation(&s[13], &s[15], false, min, max);

  // stage 8.
  butterfly_rotation(&s[2], &s[3], 32, true);
  butterfly_rotation(&s[6], &s[7], 32, true);
  butterfly_rotation(&s[10], &s[11], 32, true);
  butterfly_rotation(&s[14], &s[15], 32, true);

  // stage 9.
  x[0] = s[0];
  x[1] = vqnegq_s32(s[8]);
  x[2] = s[12];
  x[3] = vqnegq_s32(s[4]);
  x[4] = s[6];
  x[5] = vqnegq_s32(s[14]);
  x[6] = s[10];
  x[7] = vqnegq_s32(s[2]);
  x[8] = s[3];
  x[9] = vqnegq_s32(s[11]);
  x[10] = s[15];
  x[11] = vqnegq_s32(s[7]);
  x[12] = s[5];
  x[13] = vqnegq_s32(s[13]);
  x[14] = s[9];
  x[15] = vqnegq_s32(s[1]);

  if (is_row) {
    const int32x4_t v_row_shift = vdupq_n_s32(-row_shift);
    for (auto& i : x) {
      i = vmovl_s16(vqmovn_s32(vqrshlq_s32(i, v_row_shift)));
    }
    for (int idx = 0; idx < 16; idx += 8) {
      Transpose4x4(&x[idx], &x[idx]);
      Transpose4x4(&x[idx + 4], &x[idx + 4]);
      StoreDst<4>(dst, step, idx, &x[idx]);
      StoreDst<4>(dst, step, idx + 4, &x[idx + 4]);
    }
  } else {
    StoreDst<16>(dst, step, 0, &x[0]);
  }
}

LIBGAV1_ALWAYS_INLINE void Adst16DcOnlyInternal(int32x4_t* s, int32x4_t* x) {
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
  x[0] = s[0];
  x[1] = vqnegq_s32(s[8]);
  x[2] = s[12];
  x[3] = vqnegq_s32(s[4]);
  x[4] = s[6];
  x[5] = vqnegq_s32(s[14]);
  x[6] = s[10];
  x[7] = vqnegq_s32(s[2]);
  x[8] = s[3];
  x[9] = vqnegq_s32(s[11]);
  x[10] = s[15];
  x[11] = vqnegq_s32(s[7]);
  x[12] = s[5];
  x[13] = vqnegq_s32(s[13]);
  x[14] = s[9];
  x[15] = vqnegq_s32(s[1]);
}

LIBGAV1_ALWAYS_INLINE bool Adst16DcOnly(void* dest, int adjusted_tx_height,
                                        bool should_round, int row_shift) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int32_t*>(dest);
  int32x4_t s[16];
  int32x4_t x[16];
  const int32x4_t v_src = vdupq_n_s32(dst[0]);
  const uint32x4_t v_mask = vdupq_n_u32(should_round ? 0xffffffff : 0);
  const int32x4_t v_src_round =
      vqrdmulhq_n_s32(v_src, kTransformRowMultiplier << (31 - 12));
  // stage 1.
  s[1] = vbslq_s32(v_mask, v_src_round, v_src);

  Adst16DcOnlyInternal(s, x);

  for (int i = 0; i < 16; ++i) {
    // vqrshlq_s32 will shift right if shift value is negative.
    x[i] = vmovl_s16(vqmovn_s32(vqrshlq_s32(x[i], vdupq_n_s32(-row_shift))));
    vst1q_lane_s32(&dst[i], x[i], 0);
  }

  return true;
}

LIBGAV1_ALWAYS_INLINE bool Adst16DcOnlyColumn(void* dest,
                                              int adjusted_tx_height,
                                              int width) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int32_t*>(dest);
  int i = 0;
  do {
    int32x4_t s[16];
    int32x4_t x[16];
    const int32x4_t v_src = vld1q_s32(dst);
    // stage 1.
    s[1] = v_src;

    Adst16DcOnlyInternal(s, x);

    for (int j = 0; j < 16; ++j) {
      vst1q_s32(&dst[j * width], x[j]);
    }
    i += 4;
    dst += 4;
  } while (i < width);

  return true;
}

//------------------------------------------------------------------------------
// Identity Transforms.

LIBGAV1_ALWAYS_INLINE void Identity4_NEON(void* dest, int32_t step, int shift) {
  auto* const dst = static_cast<int32_t*>(dest);
  const int32x4_t v_dual_round = vdupq_n_s32((1 + (shift << 1)) << 11);
  const int32x4_t v_multiplier = vdupq_n_s32(kIdentity4Multiplier);
  const int32x4_t v_shift = vdupq_n_s32(-(12 + shift));
  for (int i = 0; i < 4; ++i) {
    const int32x4_t v_src = vld1q_s32(&dst[i * step]);
    const int32x4_t v_src_mult_lo =
        vmlaq_s32(v_dual_round, v_src, v_multiplier);
    const int32x4_t shift_lo = vqshlq_s32(v_src_mult_lo, v_shift);
    vst1q_s32(&dst[i * step], vmovl_s16(vqmovn_s32(shift_lo)));
  }
}

LIBGAV1_ALWAYS_INLINE bool Identity4DcOnly(void* dest, int adjusted_tx_height,
                                           bool should_round, int tx_height) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int32_t*>(dest);
  const int32x4_t v_src0 = vdupq_n_s32(dst[0]);
  const uint32x4_t v_mask = vdupq_n_u32(should_round ? 0xffffffff : 0);
  const int32x4_t v_src_round =
      vqrdmulhq_n_s32(v_src0, kTransformRowMultiplier << (31 - 12));
  const int32x4_t v_src = vbslq_s32(v_mask, v_src_round, v_src0);
  const int shift = tx_height < 16 ? 0 : 1;
  const int32x4_t v_dual_round = vdupq_n_s32((1 + (shift << 1)) << 11);
  const int32x4_t v_multiplier = vdupq_n_s32(kIdentity4Multiplier);
  const int32x4_t v_shift = vdupq_n_s32(-(12 + shift));
  const int32x4_t v_src_mult_lo = vmlaq_s32(v_dual_round, v_src, v_multiplier);
  const int32x4_t dst_0 = vqshlq_s32(v_src_mult_lo, v_shift);
  vst1q_lane_s32(dst, vmovl_s16(vqmovn_s32(dst_0)), 0);
  return true;
}

template <int identity_size>
LIBGAV1_ALWAYS_INLINE void IdentityColumnStoreToFrame(
    Array2DView<uint16_t> frame, const int start_x, const int start_y,
    const int tx_width, const int tx_height,
    const int32_t* LIBGAV1_RESTRICT source) {
  static_assert(identity_size == 4 || identity_size == 8 ||
                    identity_size == 16 || identity_size == 32,
                "Invalid identity_size.");
  const int stride = frame.columns();
  uint16_t* LIBGAV1_RESTRICT dst = frame[start_y] + start_x;
  const int32x4_t v_dual_round = vdupq_n_s32((1 + (1 << 4)) << 11);
  const uint16x4_t v_max_bitdepth = vdup_n_u16((1 << kBitdepth10) - 1);

  if (identity_size < 32) {
    if (tx_width == 4) {
      int i = 0;
      do {
        int32x4x2_t v_src, v_dst_i, a, b;
        v_src.val[0] = vld1q_s32(&source[i * 4]);
        v_src.val[1] = vld1q_s32(&source[(i * 4) + 4]);
        if (identity_size == 4) {
          v_dst_i.val[0] =
              vmlaq_n_s32(v_dual_round, v_src.val[0], kIdentity4Multiplier);
          v_dst_i.val[1] =
              vmlaq_n_s32(v_dual_round, v_src.val[1], kIdentity4Multiplier);
          a.val[0] = vshrq_n_s32(v_dst_i.val[0], 4 + 12);
          a.val[1] = vshrq_n_s32(v_dst_i.val[1], 4 + 12);
        } else if (identity_size == 8) {
          v_dst_i.val[0] = vaddq_s32(v_src.val[0], v_src.val[0]);
          v_dst_i.val[1] = vaddq_s32(v_src.val[1], v_src.val[1]);
          a.val[0] = vrshrq_n_s32(v_dst_i.val[0], 4);
          a.val[1] = vrshrq_n_s32(v_dst_i.val[1], 4);
        } else {  // identity_size == 16
          v_dst_i.val[0] =
              vmlaq_n_s32(v_dual_round, v_src.val[0], kIdentity16Multiplier);
          v_dst_i.val[1] =
              vmlaq_n_s32(v_dual_round, v_src.val[1], kIdentity16Multiplier);
          a.val[0] = vshrq_n_s32(v_dst_i.val[0], 4 + 12);
          a.val[1] = vshrq_n_s32(v_dst_i.val[1], 4 + 12);
        }
        uint16x4x2_t frame_data;
        frame_data.val[0] = vld1_u16(dst);
        frame_data.val[1] = vld1_u16(dst + stride);
        b.val[0] = vaddw_s16(a.val[0], vreinterpret_s16_u16(frame_data.val[0]));
        b.val[1] = vaddw_s16(a.val[1], vreinterpret_s16_u16(frame_data.val[1]));
        vst1_u16(dst, vmin_u16(vqmovun_s32(b.val[0]), v_max_bitdepth));
        vst1_u16(dst + stride, vmin_u16(vqmovun_s32(b.val[1]), v_max_bitdepth));
        dst += stride << 1;
        i += 2;
      } while (i < tx_height);
    } else {
      int i = 0;
      do {
        const int row = i * tx_width;
        int j = 0;
        do {
          int32x4x2_t v_src, v_dst_i, a, b;
          v_src.val[0] = vld1q_s32(&source[row + j]);
          v_src.val[1] = vld1q_s32(&source[row + j + 4]);
          if (identity_size == 4) {
            v_dst_i.val[0] =
                vmlaq_n_s32(v_dual_round, v_src.val[0], kIdentity4Multiplier);
            v_dst_i.val[1] =
                vmlaq_n_s32(v_dual_round, v_src.val[1], kIdentity4Multiplier);
            a.val[0] = vshrq_n_s32(v_dst_i.val[0], 4 + 12);
            a.val[1] = vshrq_n_s32(v_dst_i.val[1], 4 + 12);
          } else if (identity_size == 8) {
            v_dst_i.val[0] = vaddq_s32(v_src.val[0], v_src.val[0]);
            v_dst_i.val[1] = vaddq_s32(v_src.val[1], v_src.val[1]);
            a.val[0] = vrshrq_n_s32(v_dst_i.val[0], 4);
            a.val[1] = vrshrq_n_s32(v_dst_i.val[1], 4);
          } else {  // identity_size == 16
            v_dst_i.val[0] =
                vmlaq_n_s32(v_dual_round, v_src.val[0], kIdentity16Multiplier);
            v_dst_i.val[1] =
                vmlaq_n_s32(v_dual_round, v_src.val[1], kIdentity16Multiplier);
            a.val[0] = vshrq_n_s32(v_dst_i.val[0], 4 + 12);
            a.val[1] = vshrq_n_s32(v_dst_i.val[1], 4 + 12);
          }
          uint16x4x2_t frame_data;
          frame_data.val[0] = vld1_u16(dst + j);
          frame_data.val[1] = vld1_u16(dst + j + 4);
          b.val[0] =
              vaddw_s16(a.val[0], vreinterpret_s16_u16(frame_data.val[0]));
          b.val[1] =
              vaddw_s16(a.val[1], vreinterpret_s16_u16(frame_data.val[1]));
          vst1_u16(dst + j, vmin_u16(vqmovun_s32(b.val[0]), v_max_bitdepth));
          vst1_u16(dst + j + 4,
                   vmin_u16(vqmovun_s32(b.val[1]), v_max_bitdepth));
          j += 8;
        } while (j < tx_width);
        dst += stride;
      } while (++i < tx_height);
    }
  } else {
    int i = 0;
    do {
      const int row = i * tx_width;
      int j = 0;
      do {
        const int32x4_t v_dst_i = vld1q_s32(&source[row + j]);
        const uint16x4_t frame_data = vld1_u16(dst + j);
        const int32x4_t a = vrshrq_n_s32(v_dst_i, 2);
        const int32x4_t b = vaddw_s16(a, vreinterpret_s16_u16(frame_data));
        const uint16x4_t d = vmin_u16(vqmovun_s32(b), v_max_bitdepth);
        vst1_u16(dst + j, d);
        j += 4;
      } while (j < tx_width);
      dst += stride;
    } while (++i < tx_height);
  }
}

LIBGAV1_ALWAYS_INLINE void Identity4RowColumnStoreToFrame(
    Array2DView<uint16_t> frame, const int start_x, const int start_y,
    const int tx_width, const int tx_height,
    const int32_t* LIBGAV1_RESTRICT source) {
  const int stride = frame.columns();
  uint16_t* LIBGAV1_RESTRICT dst = frame[start_y] + start_x;
  const int32x4_t v_round = vdupq_n_s32((1 + (0)) << 11);
  const uint16x4_t v_max_bitdepth = vdup_n_u16((1 << kBitdepth10) - 1);

  if (tx_width == 4) {
    int i = 0;
    do {
      const int32x4_t v_src = vld1q_s32(&source[i * 4]);
      const int32x4_t v_dst_row =
          vshrq_n_s32(vmlaq_n_s32(v_round, v_src, kIdentity4Multiplier), 12);
      const int32x4_t v_dst_col =
          vmlaq_n_s32(v_round, v_dst_row, kIdentity4Multiplier);
      const uint16x4_t frame_data = vld1_u16(dst);
      const int32x4_t a = vrshrq_n_s32(v_dst_col, 4 + 12);
      const int32x4_t b = vaddw_s16(a, vreinterpret_s16_u16(frame_data));
      vst1_u16(dst, vmin_u16(vqmovun_s32(b), v_max_bitdepth));
      dst += stride;
    } while (++i < tx_height);
  } else {
    int i = 0;
    do {
      const int row = i * tx_width;
      int j = 0;
      do {
        int32x4x2_t v_src, v_src_round, v_dst_row, v_dst_col, a, b;
        v_src.val[0] = vld1q_s32(&source[row + j]);
        v_src.val[1] = vld1q_s32(&source[row + j + 4]);
        v_src_round.val[0] = vshrq_n_s32(
            vmlaq_n_s32(v_round, v_src.val[0], kTransformRowMultiplier), 12);
        v_src_round.val[1] = vshrq_n_s32(
            vmlaq_n_s32(v_round, v_src.val[1], kTransformRowMultiplier), 12);
        v_dst_row.val[0] = vqaddq_s32(v_src_round.val[0], v_src_round.val[0]);
        v_dst_row.val[1] = vqaddq_s32(v_src_round.val[1], v_src_round.val[1]);
        v_dst_col.val[0] =
            vmlaq_n_s32(v_round, v_dst_row.val[0], kIdentity4Multiplier);
        v_dst_col.val[1] =
            vmlaq_n_s32(v_round, v_dst_row.val[1], kIdentity4Multiplier);
        uint16x4x2_t frame_data;
        frame_data.val[0] = vld1_u16(dst + j);
        frame_data.val[1] = vld1_u16(dst + j + 4);
        a.val[0] = vrshrq_n_s32(v_dst_col.val[0], 4 + 12);
        a.val[1] = vrshrq_n_s32(v_dst_col.val[1], 4 + 12);
        b.val[0] = vaddw_s16(a.val[0], vreinterpret_s16_u16(frame_data.val[0]));
        b.val[1] = vaddw_s16(a.val[1], vreinterpret_s16_u16(frame_data.val[1]));
        vst1_u16(dst + j, vmin_u16(vqmovun_s32(b.val[0]), v_max_bitdepth));
        vst1_u16(dst + j + 4, vmin_u16(vqmovun_s32(b.val[1]), v_max_bitdepth));
        j += 8;
      } while (j < tx_width);
      dst += stride;
    } while (++i < tx_height);
  }
}

LIBGAV1_ALWAYS_INLINE void Identity8Row32_NEON(void* dest, int32_t step) {
  auto* const dst = static_cast<int32_t*>(dest);

  // When combining the identity8 multiplier with the row shift, the
  // calculations for tx_height equal to 32 can be simplified from
  // ((A * 2) + 2) >> 2) to ((A + 1) >> 1).
  for (int i = 0; i < 4; ++i) {
    const int32x4_t v_src_lo = vld1q_s32(&dst[i * step]);
    const int32x4_t v_src_hi = vld1q_s32(&dst[(i * step) + 4]);
    const int32x4_t a_lo = vrshrq_n_s32(v_src_lo, 1);
    const int32x4_t a_hi = vrshrq_n_s32(v_src_hi, 1);
    vst1q_s32(&dst[i * step], vmovl_s16(vqmovn_s32(a_lo)));
    vst1q_s32(&dst[(i * step) + 4], vmovl_s16(vqmovn_s32(a_hi)));
  }
}

LIBGAV1_ALWAYS_INLINE void Identity8Row4_NEON(void* dest, int32_t step) {
  auto* const dst = static_cast<int32_t*>(dest);

  for (int i = 0; i < 4; ++i) {
    const int32x4_t v_src_lo = vld1q_s32(&dst[i * step]);
    const int32x4_t v_src_hi = vld1q_s32(&dst[(i * step) + 4]);
    const int32x4_t v_srcx2_lo = vqaddq_s32(v_src_lo, v_src_lo);
    const int32x4_t v_srcx2_hi = vqaddq_s32(v_src_hi, v_src_hi);
    vst1q_s32(&dst[i * step], vmovl_s16(vqmovn_s32(v_srcx2_lo)));
    vst1q_s32(&dst[(i * step) + 4], vmovl_s16(vqmovn_s32(v_srcx2_hi)));
  }
}

LIBGAV1_ALWAYS_INLINE bool Identity8DcOnly(void* dest, int adjusted_tx_height,
                                           bool should_round, int row_shift) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int32_t*>(dest);
  const int32x4_t v_src0 = vdupq_n_s32(dst[0]);
  const uint32x4_t v_mask = vdupq_n_u32(should_round ? 0xffffffff : 0);
  const int32x4_t v_src_round =
      vqrdmulhq_n_s32(v_src0, kTransformRowMultiplier << (31 - 12));
  const int32x4_t v_src = vbslq_s32(v_mask, v_src_round, v_src0);
  const int32x4_t v_srcx2 = vaddq_s32(v_src, v_src);
  const int32x4_t dst_0 = vqrshlq_s32(v_srcx2, vdupq_n_s32(-row_shift));
  vst1q_lane_s32(dst, vmovl_s16(vqmovn_s32(dst_0)), 0);
  return true;
}

LIBGAV1_ALWAYS_INLINE void Identity16Row_NEON(void* dest, int32_t step,
                                              int shift) {
  auto* const dst = static_cast<int32_t*>(dest);
  const int32x4_t v_dual_round = vdupq_n_s32((1 + (shift << 1)) << 11);
  const int32x4_t v_shift = vdupq_n_s32(-(12 + shift));

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 2; ++j) {
      int32x4x2_t v_src;
      v_src.val[0] = vld1q_s32(&dst[i * step + j * 8]);
      v_src.val[1] = vld1q_s32(&dst[i * step + j * 8 + 4]);
      const int32x4_t v_src_mult_lo =
          vmlaq_n_s32(v_dual_round, v_src.val[0], kIdentity16Multiplier);
      const int32x4_t v_src_mult_hi =
          vmlaq_n_s32(v_dual_round, v_src.val[1], kIdentity16Multiplier);
      const int32x4_t shift_lo = vqshlq_s32(v_src_mult_lo, v_shift);
      const int32x4_t shift_hi = vqshlq_s32(v_src_mult_hi, v_shift);
      vst1q_s32(&dst[i * step + j * 8], vmovl_s16(vqmovn_s32(shift_lo)));
      vst1q_s32(&dst[i * step + j * 8 + 4], vmovl_s16(vqmovn_s32(shift_hi)));
    }
  }
}

LIBGAV1_ALWAYS_INLINE bool Identity16DcOnly(void* dest, int adjusted_tx_height,
                                            bool should_round, int shift) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int32_t*>(dest);
  const int32x4_t v_src0 = vdupq_n_s32(dst[0]);
  const uint32x4_t v_mask = vdupq_n_u32(should_round ? 0xffffffff : 0);
  const int32x4_t v_src_round =
      vqrdmulhq_n_s32(v_src0, kTransformRowMultiplier << (31 - 12));
  const int32x4_t v_src = vbslq_s32(v_mask, v_src_round, v_src0);
  const int32x4_t v_dual_round = vdupq_n_s32((1 + (shift << 1)) << 11);
  const int32x4_t v_src_mult_lo =
      vmlaq_n_s32(v_dual_round, v_src, kIdentity16Multiplier);
  const int32x4_t dst_0 = vqshlq_s32(v_src_mult_lo, vdupq_n_s32(-(12 + shift)));
  vst1q_lane_s32(dst, vmovl_s16(vqmovn_s32(dst_0)), 0);
  return true;
}

LIBGAV1_ALWAYS_INLINE void Identity32Row16_NEON(void* dest,
                                                const int32_t step) {
  auto* const dst = static_cast<int32_t*>(dest);

  // When combining the identity32 multiplier with the row shift, the
  // calculation for tx_height equal to 16 can be simplified from
  // ((A * 4) + 1) >> 1) to (A * 2).
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 32; j += 4) {
      const int32x4_t v_src = vld1q_s32(&dst[i * step + j]);
      const int32x4_t v_dst_i = vqaddq_s32(v_src, v_src);
      vst1q_s32(&dst[i * step + j], v_dst_i);
    }
  }
}

LIBGAV1_ALWAYS_INLINE bool Identity32DcOnly(void* dest,
                                            int adjusted_tx_height) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int32_t*>(dest);
  const int32x2_t v_src0 = vdup_n_s32(dst[0]);
  const int32x2_t v_src =
      vqrdmulh_n_s32(v_src0, kTransformRowMultiplier << (31 - 12));
  // When combining the identity32 multiplier with the row shift, the
  // calculation for tx_height equal to 16 can be simplified from
  // ((A * 4) + 1) >> 1) to (A * 2).
  const int32x2_t v_dst_0 = vqadd_s32(v_src, v_src);
  vst1_lane_s32(dst, v_dst_0, 0);
  return true;
}

//------------------------------------------------------------------------------
// Walsh Hadamard Transform.

// Process 4 wht4 rows and columns.
LIBGAV1_ALWAYS_INLINE void Wht4_NEON(uint16_t* LIBGAV1_RESTRICT dst,
                                     const int dst_stride,
                                     const void* LIBGAV1_RESTRICT source,
                                     const int adjusted_tx_height) {
  const auto* const src = static_cast<const int32_t*>(source);
  int32x4_t s[4];

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
    int32_t f = (src[0] >> 2) - (src[0] >> 3);
    const int32_t g = f >> 1;
    f = f - (f >> 1);
    const int32_t h = (src[0] >> 3) - (src[0] >> 4);
    const int32_t i = (src[0] >> 4);
    s[0] = vdupq_n_s32(h);
    s[0] = vsetq_lane_s32(f, s[0], 0);
    s[1] = vdupq_n_s32(i);
    s[1] = vsetq_lane_s32(g, s[1], 0);
    s[2] = s[3] = s[1];
  } else {
    // Load the 4x4 source in transposed form.
    int32x4x4_t columns = vld4q_s32(src);

    // Shift right and permute the columns for the WHT.
    s[0] = vshrq_n_s32(columns.val[0], 2);
    s[2] = vshrq_n_s32(columns.val[1], 2);
    s[3] = vshrq_n_s32(columns.val[2], 2);
    s[1] = vshrq_n_s32(columns.val[3], 2);

    // Row transforms.
    s[0] = vaddq_s32(s[0], s[2]);
    s[3] = vsubq_s32(s[3], s[1]);
    int32x4_t e = vhsubq_s32(s[0], s[3]);  // e = (s[0] - s[3]) >> 1
    s[1] = vsubq_s32(e, s[1]);
    s[2] = vsubq_s32(e, s[2]);
    s[0] = vsubq_s32(s[0], s[1]);
    s[3] = vaddq_s32(s[3], s[2]);

    int32x4_t x[4];
    Transpose4x4(s, x);

    s[0] = x[0];
    s[2] = x[1];
    s[3] = x[2];
    s[1] = x[3];

    // Column transforms.
    s[0] = vaddq_s32(s[0], s[2]);
    s[3] = vsubq_s32(s[3], s[1]);
    e = vhsubq_s32(s[0], s[3]);  // e = (s[0] - s[3]) >> 1
    s[1] = vsubq_s32(e, s[1]);
    s[2] = vsubq_s32(e, s[2]);
    s[0] = vsubq_s32(s[0], s[1]);
    s[3] = vaddq_s32(s[3], s[2]);
  }

  // Store to frame.
  const uint16x4_t v_max_bitdepth = vdup_n_u16((1 << kBitdepth10) - 1);
  for (int row = 0; row < 4; row += 1) {
    const uint16x4_t frame_data = vld1_u16(dst);
    const int32x4_t b = vaddw_s16(s[row], vreinterpret_s16_u16(frame_data));
    vst1_u16(dst, vmin_u16(vqmovun_s32(b), v_max_bitdepth));
    dst += dst_stride;
  }
}

//------------------------------------------------------------------------------
// row/column transform loops

template <int tx_height>
LIBGAV1_ALWAYS_INLINE void FlipColumns(int32_t* source, int tx_width) {
  if (tx_width >= 16) {
    int i = 0;
    do {
      // 00 01 02 03
      const int32x4_t a = vld1q_s32(&source[i]);
      const int32x4_t b = vld1q_s32(&source[i + 4]);
      const int32x4_t c = vld1q_s32(&source[i + 8]);
      const int32x4_t d = vld1q_s32(&source[i + 12]);
      // 01 00 03 02
      const int32x4_t a_rev = vrev64q_s32(a);
      const int32x4_t b_rev = vrev64q_s32(b);
      const int32x4_t c_rev = vrev64q_s32(c);
      const int32x4_t d_rev = vrev64q_s32(d);
      // 03 02 01 00
      vst1q_s32(&source[i], vextq_s32(d_rev, d_rev, 2));
      vst1q_s32(&source[i + 4], vextq_s32(c_rev, c_rev, 2));
      vst1q_s32(&source[i + 8], vextq_s32(b_rev, b_rev, 2));
      vst1q_s32(&source[i + 12], vextq_s32(a_rev, a_rev, 2));
      i += 16;
    } while (i < tx_width * tx_height);
  } else if (tx_width == 8) {
    for (int i = 0; i < 8 * tx_height; i += 8) {
      // 00 01 02 03
      const int32x4_t a = vld1q_s32(&source[i]);
      const int32x4_t b = vld1q_s32(&source[i + 4]);
      // 01 00 03 02
      const int32x4_t a_rev = vrev64q_s32(a);
      const int32x4_t b_rev = vrev64q_s32(b);
      // 03 02 01 00
      vst1q_s32(&source[i], vextq_s32(b_rev, b_rev, 2));
      vst1q_s32(&source[i + 4], vextq_s32(a_rev, a_rev, 2));
    }
  } else {
    // Process two rows per iteration.
    for (int i = 0; i < 4 * tx_height; i += 8) {
      // 00 01 02 03
      const int32x4_t a = vld1q_s32(&source[i]);
      const int32x4_t b = vld1q_s32(&source[i + 4]);
      // 01 00 03 02
      const int32x4_t a_rev = vrev64q_s32(a);
      const int32x4_t b_rev = vrev64q_s32(b);
      // 03 02 01 00
      vst1q_s32(&source[i], vextq_s32(a_rev, a_rev, 2));
      vst1q_s32(&source[i + 4], vextq_s32(b_rev, b_rev, 2));
    }
  }
}

template <int tx_width>
LIBGAV1_ALWAYS_INLINE void ApplyRounding(int32_t* source, int num_rows) {
  // Process two rows per iteration.
  int i = 0;
  do {
    const int32x4_t a_lo = vld1q_s32(&source[i]);
    const int32x4_t a_hi = vld1q_s32(&source[i + 4]);
    const int32x4_t b_lo =
        vqrdmulhq_n_s32(a_lo, kTransformRowMultiplier << (31 - 12));
    const int32x4_t b_hi =
        vqrdmulhq_n_s32(a_hi, kTransformRowMultiplier << (31 - 12));
    vst1q_s32(&source[i], b_lo);
    vst1q_s32(&source[i + 4], b_hi);
    i += 8;
  } while (i < tx_width * num_rows);
}

template <int tx_width>
LIBGAV1_ALWAYS_INLINE void RowShift(int32_t* source, int num_rows,
                                    int row_shift) {
  // vqrshlq_s32 will shift right if shift value is negative.
  row_shift = -row_shift;

  // Process two rows per iteration.
  int i = 0;
  do {
    const int32x4_t residual0 = vld1q_s32(&source[i]);
    const int32x4_t residual1 = vld1q_s32(&source[i + 4]);
    vst1q_s32(&source[i], vqrshlq_s32(residual0, vdupq_n_s32(row_shift)));
    vst1q_s32(&source[i + 4], vqrshlq_s32(residual1, vdupq_n_s32(row_shift)));
    i += 8;
  } while (i < tx_width * num_rows);
}

template <int tx_height, bool enable_flip_rows = false>
LIBGAV1_ALWAYS_INLINE void StoreToFrameWithRound(
    Array2DView<uint16_t> frame, const int start_x, const int start_y,
    const int tx_width, const int32_t* LIBGAV1_RESTRICT source,
    TransformType tx_type) {
  const bool flip_rows =
      enable_flip_rows ? kTransformFlipRowsMask.Contains(tx_type) : false;
  const int stride = frame.columns();
  uint16_t* LIBGAV1_RESTRICT dst = frame[start_y] + start_x;

  if (tx_width == 4) {
    for (int i = 0; i < tx_height; ++i) {
      const int row = flip_rows ? (tx_height - i - 1) * 4 : i * 4;
      const int32x4_t residual = vld1q_s32(&source[row]);
      const uint16x4_t frame_data = vld1_u16(dst);
      const int32x4_t a = vrshrq_n_s32(residual, 4);
      const uint32x4_t b = vaddw_u16(vreinterpretq_u32_s32(a), frame_data);
      const uint16x4_t d = vqmovun_s32(vreinterpretq_s32_u32(b));
      vst1_u16(dst, vmin_u16(d, vdup_n_u16((1 << kBitdepth10) - 1)));
      dst += stride;
    }
  } else {
    for (int i = 0; i < tx_height; ++i) {
      const int y = start_y + i;
      const int row = flip_rows ? (tx_height - i - 1) * tx_width : i * tx_width;
      int j = 0;
      do {
        const int x = start_x + j;
        const int32x4_t residual = vld1q_s32(&source[row + j]);
        const int32x4_t residual_hi = vld1q_s32(&source[row + j + 4]);
        const uint16x8_t frame_data = vld1q_u16(frame[y] + x);
        const int32x4_t a = vrshrq_n_s32(residual, 4);
        const int32x4_t a_hi = vrshrq_n_s32(residual_hi, 4);
        const uint32x4_t b =
            vaddw_u16(vreinterpretq_u32_s32(a), vget_low_u16(frame_data));
        const uint32x4_t b_hi =
            vaddw_u16(vreinterpretq_u32_s32(a_hi), vget_high_u16(frame_data));
        const uint16x4_t d = vqmovun_s32(vreinterpretq_s32_u32(b));
        const uint16x4_t d_hi = vqmovun_s32(vreinterpretq_s32_u32(b_hi));
        vst1q_u16(frame[y] + x, vminq_u16(vcombine_u16(d, d_hi),
                                          vdupq_n_u16((1 << kBitdepth10) - 1)));
        j += 8;
      } while (j < tx_width);
    }
  }
}

void Dct4TransformLoopRow_NEON(TransformType /*tx_type*/, TransformSize tx_size,
                               int adjusted_tx_height, void* src_buffer,
                               int /*start_x*/, int /*start_y*/,
                               void* /*dst_frame*/) {
  auto* src = static_cast<int32_t*>(src_buffer);
  const int tx_height = kTransformHeight[tx_size];
  const bool should_round = (tx_height == 8);
  const int row_shift = static_cast<int>(tx_height == 16);

  if (DctDcOnly<4>(src, adjusted_tx_height, should_round, row_shift)) {
    return;
  }

  if (should_round) {
    ApplyRounding<4>(src, adjusted_tx_height);
  }

  // Process 4 1d dct4 rows in parallel per iteration.
  int i = adjusted_tx_height;
  auto* data = src;
  do {
    Dct4_NEON<ButterflyRotation_4>(data, /*step=*/4, /*is_row=*/true,
                                   row_shift);
    data += 16;
    i -= 4;
  } while (i != 0);
}

void Dct4TransformLoopColumn_NEON(TransformType tx_type, TransformSize tx_size,
                                  int adjusted_tx_height,
                                  void* LIBGAV1_RESTRICT src_buffer,
                                  int start_x, int start_y,
                                  void* LIBGAV1_RESTRICT dst_frame) {
  auto* src = static_cast<int32_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (kTransformFlipColumnsMask.Contains(tx_type)) {
    FlipColumns<4>(src, tx_width);
  }

  if (!DctDcOnlyColumn<4>(src, adjusted_tx_height, tx_width)) {
    // Process 4 1d dct4 columns in parallel per iteration.
    int i = tx_width;
    auto* data = src;
    do {
      Dct4_NEON<ButterflyRotation_4>(data, tx_width, /*transpose=*/false,
                                     /*row_shift=*/0);
      data += 4;
      i -= 4;
    } while (i != 0);
  }

  auto& frame = *static_cast<Array2DView<uint16_t>*>(dst_frame);
  StoreToFrameWithRound<4>(frame, start_x, start_y, tx_width, src, tx_type);
}

void Dct8TransformLoopRow_NEON(TransformType /*tx_type*/, TransformSize tx_size,
                               int adjusted_tx_height, void* src_buffer,
                               int /*start_x*/, int /*start_y*/,
                               void* /*dst_frame*/) {
  auto* src = static_cast<int32_t*>(src_buffer);
  const bool should_round = kShouldRound[tx_size];
  const uint8_t row_shift = kTransformRowShift[tx_size];

  if (DctDcOnly<8>(src, adjusted_tx_height, should_round, row_shift)) {
    return;
  }

  if (should_round) {
    ApplyRounding<8>(src, adjusted_tx_height);
  }

  // Process 4 1d dct8 rows in parallel per iteration.
  int i = adjusted_tx_height;
  auto* data = src;
  do {
    Dct8_NEON<ButterflyRotation_4>(data, /*step=*/8, /*is_row=*/true,
                                   row_shift);
    data += 32;
    i -= 4;
  } while (i != 0);
}

void Dct8TransformLoopColumn_NEON(TransformType tx_type, TransformSize tx_size,
                                  int adjusted_tx_height,
                                  void* LIBGAV1_RESTRICT src_buffer,
                                  int start_x, int start_y,
                                  void* LIBGAV1_RESTRICT dst_frame) {
  auto* src = static_cast<int32_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (kTransformFlipColumnsMask.Contains(tx_type)) {
    FlipColumns<8>(src, tx_width);
  }

  if (!DctDcOnlyColumn<8>(src, adjusted_tx_height, tx_width)) {
    // Process 4 1d dct8 columns in parallel per iteration.
    int i = tx_width;
    auto* data = src;
    do {
      Dct8_NEON<ButterflyRotation_4>(data, tx_width, /*is_row=*/false,
                                     /*row_shift=*/0);
      data += 4;
      i -= 4;
    } while (i != 0);
  }
  auto& frame = *static_cast<Array2DView<uint16_t>*>(dst_frame);
  StoreToFrameWithRound<8>(frame, start_x, start_y, tx_width, src, tx_type);
}

void Dct16TransformLoopRow_NEON(TransformType /*tx_type*/,
                                TransformSize tx_size, int adjusted_tx_height,
                                void* src_buffer, int /*start_x*/,
                                int /*start_y*/, void* /*dst_frame*/) {
  auto* src = static_cast<int32_t*>(src_buffer);
  const bool should_round = kShouldRound[tx_size];
  const uint8_t row_shift = kTransformRowShift[tx_size];

  if (DctDcOnly<16>(src, adjusted_tx_height, should_round, row_shift)) {
    return;
  }

  if (should_round) {
    ApplyRounding<16>(src, adjusted_tx_height);
  }

  assert(adjusted_tx_height % 4 == 0);
  int i = adjusted_tx_height;
  auto* data = src;
  do {
    // Process 4 1d dct16 rows in parallel per iteration.
    Dct16_NEON<ButterflyRotation_4>(data, 16, /*is_row=*/true, row_shift);
    data += 64;
    i -= 4;
  } while (i != 0);
}

void Dct16TransformLoopColumn_NEON(TransformType tx_type, TransformSize tx_size,
                                   int adjusted_tx_height,
                                   void* LIBGAV1_RESTRICT src_buffer,
                                   int start_x, int start_y,
                                   void* LIBGAV1_RESTRICT dst_frame) {
  auto* src = static_cast<int32_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (kTransformFlipColumnsMask.Contains(tx_type)) {
    FlipColumns<16>(src, tx_width);
  }

  if (!DctDcOnlyColumn<16>(src, adjusted_tx_height, tx_width)) {
    // Process 4 1d dct16 columns in parallel per iteration.
    int i = tx_width;
    auto* data = src;
    do {
      Dct16_NEON<ButterflyRotation_4>(data, tx_width, /*is_row=*/false,
                                      /*row_shift=*/0);
      data += 4;
      i -= 4;
    } while (i != 0);
  }
  auto& frame = *static_cast<Array2DView<uint16_t>*>(dst_frame);
  StoreToFrameWithRound<16>(frame, start_x, start_y, tx_width, src, tx_type);
}

void Dct32TransformLoopRow_NEON(TransformType /*tx_type*/,
                                TransformSize tx_size, int adjusted_tx_height,
                                void* src_buffer, int /*start_x*/,
                                int /*start_y*/, void* /*dst_frame*/) {
  auto* src = static_cast<int32_t*>(src_buffer);
  const bool should_round = kShouldRound[tx_size];
  const uint8_t row_shift = kTransformRowShift[tx_size];

  if (DctDcOnly<32>(src, adjusted_tx_height, should_round, row_shift)) {
    return;
  }

  if (should_round) {
    ApplyRounding<32>(src, adjusted_tx_height);
  }

  assert(adjusted_tx_height % 4 == 0);
  int i = adjusted_tx_height;
  auto* data = src;
  do {
    // Process 4 1d dct32 rows in parallel per iteration.
    Dct32_NEON(data, 32, /*is_row=*/true, row_shift);
    data += 128;
    i -= 4;
  } while (i != 0);
}

void Dct32TransformLoopColumn_NEON(TransformType tx_type, TransformSize tx_size,
                                   int adjusted_tx_height,
                                   void* LIBGAV1_RESTRICT src_buffer,
                                   int start_x, int start_y,
                                   void* LIBGAV1_RESTRICT dst_frame) {
  auto* src = static_cast<int32_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (kTransformFlipColumnsMask.Contains(tx_type)) {
    FlipColumns<32>(src, tx_width);
  }

  if (!DctDcOnlyColumn<32>(src, adjusted_tx_height, tx_width)) {
    // Process 4 1d dct32 columns in parallel per iteration.
    int i = tx_width;
    auto* data = src;
    do {
      Dct32_NEON(data, tx_width, /*is_row=*/false, /*row_shift=*/0);
      data += 4;
      i -= 4;
    } while (i != 0);
  }
  auto& frame = *static_cast<Array2DView<uint16_t>*>(dst_frame);
  StoreToFrameWithRound<32>(frame, start_x, start_y, tx_width, src, tx_type);
}

void Dct64TransformLoopRow_NEON(TransformType /*tx_type*/,
                                TransformSize tx_size, int adjusted_tx_height,
                                void* src_buffer, int /*start_x*/,
                                int /*start_y*/, void* /*dst_frame*/) {
  auto* src = static_cast<int32_t*>(src_buffer);
  const bool should_round = kShouldRound[tx_size];
  const uint8_t row_shift = kTransformRowShift[tx_size];

  if (DctDcOnly<64>(src, adjusted_tx_height, should_round, row_shift)) {
    return;
  }

  if (should_round) {
    ApplyRounding<64>(src, adjusted_tx_height);
  }

  assert(adjusted_tx_height % 4 == 0);
  int i = adjusted_tx_height;
  auto* data = src;
  do {
    // Process 4 1d dct64 rows in parallel per iteration.
    Dct64_NEON(data, 64, /*is_row=*/true, row_shift);
    data += 128 * 2;
    i -= 4;
  } while (i != 0);
}

void Dct64TransformLoopColumn_NEON(TransformType tx_type, TransformSize tx_size,
                                   int adjusted_tx_height,
                                   void* LIBGAV1_RESTRICT src_buffer,
                                   int start_x, int start_y,
                                   void* LIBGAV1_RESTRICT dst_frame) {
  auto* src = static_cast<int32_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (kTransformFlipColumnsMask.Contains(tx_type)) {
    FlipColumns<64>(src, tx_width);
  }

  if (!DctDcOnlyColumn<64>(src, adjusted_tx_height, tx_width)) {
    // Process 4 1d dct64 columns in parallel per iteration.
    int i = tx_width;
    auto* data = src;
    do {
      Dct64_NEON(data, tx_width, /*is_row=*/false, /*row_shift=*/0);
      data += 4;
      i -= 4;
    } while (i != 0);
  }
  auto& frame = *static_cast<Array2DView<uint16_t>*>(dst_frame);
  StoreToFrameWithRound<64>(frame, start_x, start_y, tx_width, src, tx_type);
}

void Adst4TransformLoopRow_NEON(TransformType /*tx_type*/,
                                TransformSize tx_size, int adjusted_tx_height,
                                void* src_buffer, int /*start_x*/,
                                int /*start_y*/, void* /*dst_frame*/) {
  auto* src = static_cast<int32_t*>(src_buffer);
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
  int i = adjusted_tx_height;
  auto* data = src;
  do {
    Adst4_NEON(data, /*step=*/4, /*is_row=*/true, row_shift);
    data += 16;
    i -= 4;
  } while (i != 0);
}

void Adst4TransformLoopColumn_NEON(TransformType tx_type, TransformSize tx_size,
                                   int adjusted_tx_height,
                                   void* LIBGAV1_RESTRICT src_buffer,
                                   int start_x, int start_y,
                                   void* LIBGAV1_RESTRICT dst_frame) {
  auto* src = static_cast<int32_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (kTransformFlipColumnsMask.Contains(tx_type)) {
    FlipColumns<4>(src, tx_width);
  }

  if (!Adst4DcOnlyColumn(src, adjusted_tx_height, tx_width)) {
    // Process 4 1d adst4 columns in parallel per iteration.
    int i = tx_width;
    auto* data = src;
    do {
      Adst4_NEON(data, tx_width, /*is_row=*/false, /*row_shift=*/0);
      data += 4;
      i -= 4;
    } while (i != 0);
  }

  auto& frame = *static_cast<Array2DView<uint16_t>*>(dst_frame);
  StoreToFrameWithRound<4, /*enable_flip_rows=*/true>(frame, start_x, start_y,
                                                      tx_width, src, tx_type);
}

void Adst8TransformLoopRow_NEON(TransformType /*tx_type*/,
                                TransformSize tx_size, int adjusted_tx_height,
                                void* src_buffer, int /*start_x*/,
                                int /*start_y*/, void* /*dst_frame*/) {
  auto* src = static_cast<int32_t*>(src_buffer);
  const bool should_round = kShouldRound[tx_size];
  const uint8_t row_shift = kTransformRowShift[tx_size];

  if (Adst8DcOnly(src, adjusted_tx_height, should_round, row_shift)) {
    return;
  }

  if (should_round) {
    ApplyRounding<8>(src, adjusted_tx_height);
  }

  // Process 4 1d adst8 rows in parallel per iteration.
  assert(adjusted_tx_height % 4 == 0);
  int i = adjusted_tx_height;
  auto* data = src;
  do {
    Adst8_NEON<ButterflyRotation_4>(data, /*step=*/8,
                                    /*transpose=*/true, row_shift);
    data += 32;
    i -= 4;
  } while (i != 0);
}

void Adst8TransformLoopColumn_NEON(TransformType tx_type, TransformSize tx_size,
                                   int adjusted_tx_height,
                                   void* LIBGAV1_RESTRICT src_buffer,
                                   int start_x, int start_y,
                                   void* LIBGAV1_RESTRICT dst_frame) {
  auto* src = static_cast<int32_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (kTransformFlipColumnsMask.Contains(tx_type)) {
    FlipColumns<8>(src, tx_width);
  }

  if (!Adst8DcOnlyColumn(src, adjusted_tx_height, tx_width)) {
    // Process 4 1d adst8 columns in parallel per iteration.
    int i = tx_width;
    auto* data = src;
    do {
      Adst8_NEON<ButterflyRotation_4>(data, tx_width, /*transpose=*/false,
                                      /*row_shift=*/0);
      data += 4;
      i -= 4;
    } while (i != 0);
  }
  auto& frame = *static_cast<Array2DView<uint16_t>*>(dst_frame);
  StoreToFrameWithRound<8, /*enable_flip_rows=*/true>(frame, start_x, start_y,
                                                      tx_width, src, tx_type);
}

void Adst16TransformLoopRow_NEON(TransformType /*tx_type*/,
                                 TransformSize tx_size, int adjusted_tx_height,
                                 void* src_buffer, int /*start_x*/,
                                 int /*start_y*/, void* /*dst_frame*/) {
  auto* src = static_cast<int32_t*>(src_buffer);
  const bool should_round = kShouldRound[tx_size];
  const uint8_t row_shift = kTransformRowShift[tx_size];

  if (Adst16DcOnly(src, adjusted_tx_height, should_round, row_shift)) {
    return;
  }

  if (should_round) {
    ApplyRounding<16>(src, adjusted_tx_height);
  }

  assert(adjusted_tx_height % 4 == 0);
  int i = adjusted_tx_height;
  do {
    // Process 4 1d adst16 rows in parallel per iteration.
    Adst16_NEON<ButterflyRotation_4>(src, 16, /*is_row=*/true, row_shift);
    src += 64;
    i -= 4;
  } while (i != 0);
}

void Adst16TransformLoopColumn_NEON(TransformType tx_type,
                                    TransformSize tx_size,
                                    int adjusted_tx_height,
                                    void* LIBGAV1_RESTRICT src_buffer,
                                    int start_x, int start_y,
                                    void* LIBGAV1_RESTRICT dst_frame) {
  auto* src = static_cast<int32_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (kTransformFlipColumnsMask.Contains(tx_type)) {
    FlipColumns<16>(src, tx_width);
  }

  if (!Adst16DcOnlyColumn(src, adjusted_tx_height, tx_width)) {
    int i = tx_width;
    auto* data = src;
    do {
      // Process 4 1d adst16 columns in parallel per iteration.
      Adst16_NEON<ButterflyRotation_4>(data, tx_width, /*is_row=*/false,
                                       /*row_shift=*/0);
      data += 4;
      i -= 4;
    } while (i != 0);
  }
  auto& frame = *static_cast<Array2DView<uint16_t>*>(dst_frame);
  StoreToFrameWithRound<16, /*enable_flip_rows=*/true>(frame, start_x, start_y,
                                                       tx_width, src, tx_type);
}

void Identity4TransformLoopRow_NEON(TransformType tx_type,
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

  auto* src = static_cast<int32_t*>(src_buffer);
  const int tx_height = kTransformHeight[tx_size];
  const bool should_round = (tx_height == 8);

  if (Identity4DcOnly(src, adjusted_tx_height, should_round, tx_height)) {
    return;
  }

  if (should_round) {
    ApplyRounding<4>(src, adjusted_tx_height);
  }

  const int shift = tx_height > 8 ? 1 : 0;
  int i = adjusted_tx_height;
  do {
    Identity4_NEON(src, /*step=*/4, shift);
    src += 16;
    i -= 4;
  } while (i != 0);
}

void Identity4TransformLoopColumn_NEON(TransformType tx_type,
                                       TransformSize tx_size,
                                       int adjusted_tx_height,
                                       void* LIBGAV1_RESTRICT src_buffer,
                                       int start_x, int start_y,
                                       void* LIBGAV1_RESTRICT dst_frame) {
  auto& frame = *static_cast<Array2DView<uint16_t>*>(dst_frame);
  auto* src = static_cast<int32_t*>(src_buffer);
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

  IdentityColumnStoreToFrame<4>(frame, start_x, start_y, tx_width,
                                adjusted_tx_height, src);
}

void Identity8TransformLoopRow_NEON(TransformType tx_type,
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

  auto* src = static_cast<int32_t*>(src_buffer);
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
  // from ((A * 2) + 1) >> 1) to A. For 10bpp, A must be clamped to a signed 16
  // bit value.
  if ((tx_height & 0x18) != 0) {
    for (int i = 0; i < tx_height; ++i) {
      const int32x4_t v_src_lo = vld1q_s32(&src[i * 8]);
      const int32x4_t v_src_hi = vld1q_s32(&src[(i * 8) + 4]);
      vst1q_s32(&src[i * 8], vmovl_s16(vqmovn_s32(v_src_lo)));
      vst1q_s32(&src[(i * 8) + 4], vmovl_s16(vqmovn_s32(v_src_hi)));
    }
    return;
  }
  if (tx_height == 32) {
    int i = adjusted_tx_height;
    do {
      Identity8Row32_NEON(src, /*step=*/8);
      src += 32;
      i -= 4;
    } while (i != 0);
    return;
  }

  assert(tx_size == kTransformSize8x4);
  int i = adjusted_tx_height;
  do {
    Identity8Row4_NEON(src, /*step=*/8);
    src += 32;
    i -= 4;
  } while (i != 0);
}

void Identity8TransformLoopColumn_NEON(TransformType tx_type,
                                       TransformSize tx_size,
                                       int adjusted_tx_height,
                                       void* LIBGAV1_RESTRICT src_buffer,
                                       int start_x, int start_y,
                                       void* LIBGAV1_RESTRICT dst_frame) {
  auto* src = static_cast<int32_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (kTransformFlipColumnsMask.Contains(tx_type)) {
    FlipColumns<8>(src, tx_width);
  }
  auto& frame = *static_cast<Array2DView<uint16_t>*>(dst_frame);
  IdentityColumnStoreToFrame<8>(frame, start_x, start_y, tx_width,
                                adjusted_tx_height, src);
}

void Identity16TransformLoopRow_NEON(TransformType /*tx_type*/,
                                     TransformSize tx_size,
                                     int adjusted_tx_height, void* src_buffer,
                                     int /*start_x*/, int /*start_y*/,
                                     void* /*dst_frame*/) {
  auto* src = static_cast<int32_t*>(src_buffer);
  const bool should_round = kShouldRound[tx_size];
  const uint8_t row_shift = kTransformRowShift[tx_size];

  if (Identity16DcOnly(src, adjusted_tx_height, should_round, row_shift)) {
    return;
  }

  if (should_round) {
    ApplyRounding<16>(src, adjusted_tx_height);
  }
  int i = adjusted_tx_height;
  do {
    Identity16Row_NEON(src, /*step=*/16, row_shift);
    src += 64;
    i -= 4;
  } while (i != 0);
}

void Identity16TransformLoopColumn_NEON(TransformType tx_type,
                                        TransformSize tx_size,
                                        int adjusted_tx_height,
                                        void* LIBGAV1_RESTRICT src_buffer,
                                        int start_x, int start_y,
                                        void* LIBGAV1_RESTRICT dst_frame) {
  auto* src = static_cast<int32_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (kTransformFlipColumnsMask.Contains(tx_type)) {
    FlipColumns<16>(src, tx_width);
  }
  auto& frame = *static_cast<Array2DView<uint16_t>*>(dst_frame);
  IdentityColumnStoreToFrame<16>(frame, start_x, start_y, tx_width,
                                 adjusted_tx_height, src);
}

void Identity32TransformLoopRow_NEON(TransformType /*tx_type*/,
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

  // Process kTransformSize32x16. The src is always rounded before the identity
  // transform and shifted by 1 afterwards.
  auto* src = static_cast<int32_t*>(src_buffer);
  if (Identity32DcOnly(src, adjusted_tx_height)) {
    return;
  }

  assert(tx_size == kTransformSize32x16);
  ApplyRounding<32>(src, adjusted_tx_height);
  int i = adjusted_tx_height;
  do {
    Identity32Row16_NEON(src, /*step=*/32);
    src += 128;
    i -= 4;
  } while (i != 0);
}

void Identity32TransformLoopColumn_NEON(TransformType /*tx_type*/,
                                        TransformSize tx_size,
                                        int adjusted_tx_height,
                                        void* LIBGAV1_RESTRICT src_buffer,
                                        int start_x, int start_y,
                                        void* LIBGAV1_RESTRICT dst_frame) {
  auto& frame = *static_cast<Array2DView<uint16_t>*>(dst_frame);
  auto* src = static_cast<int32_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  IdentityColumnStoreToFrame<32>(frame, start_x, start_y, tx_width,
                                 adjusted_tx_height, src);
}

void Wht4TransformLoopRow_NEON(TransformType tx_type, TransformSize tx_size,
                               int /*adjusted_tx_height*/, void* /*src_buffer*/,
                               int /*start_x*/, int /*start_y*/,
                               void* /*dst_frame*/) {
  assert(tx_type == kTransformTypeDctDct);
  assert(tx_size == kTransformSize4x4);
  static_cast<void>(tx_type);
  static_cast<void>(tx_size);
  // Do both row and column transforms in the column-transform pass.
}

void Wht4TransformLoopColumn_NEON(TransformType tx_type, TransformSize tx_size,
                                  int adjusted_tx_height,
                                  void* LIBGAV1_RESTRICT src_buffer,
                                  int start_x, int start_y,
                                  void* LIBGAV1_RESTRICT dst_frame) {
  assert(tx_type == kTransformTypeDctDct);
  assert(tx_size == kTransformSize4x4);
  static_cast<void>(tx_type);
  static_cast<void>(tx_size);

  // Process 4 1d wht4 rows and columns in parallel.
  const auto* src = static_cast<int32_t*>(src_buffer);
  auto& frame = *static_cast<Array2DView<uint16_t>*>(dst_frame);
  uint16_t* dst = frame[start_y] + start_x;
  const int dst_stride = frame.columns();
  Wht4_NEON(dst, dst_stride, src, adjusted_tx_height);
}

//------------------------------------------------------------------------------

void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth10);
  assert(dsp != nullptr);
  // Maximum transform size for Dct is 64.
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize4][kRow] =
      Dct4TransformLoopRow_NEON;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize4][kColumn] =
      Dct4TransformLoopColumn_NEON;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize8][kRow] =
      Dct8TransformLoopRow_NEON;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize8][kColumn] =
      Dct8TransformLoopColumn_NEON;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize16][kRow] =
      Dct16TransformLoopRow_NEON;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize16][kColumn] =
      Dct16TransformLoopColumn_NEON;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize32][kRow] =
      Dct32TransformLoopRow_NEON;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize32][kColumn] =
      Dct32TransformLoopColumn_NEON;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize64][kRow] =
      Dct64TransformLoopRow_NEON;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize64][kColumn] =
      Dct64TransformLoopColumn_NEON;

  // Maximum transform size for Adst is 16.
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize4][kRow] =
      Adst4TransformLoopRow_NEON;
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize4][kColumn] =
      Adst4TransformLoopColumn_NEON;
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize8][kRow] =
      Adst8TransformLoopRow_NEON;
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize8][kColumn] =
      Adst8TransformLoopColumn_NEON;
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize16][kRow] =
      Adst16TransformLoopRow_NEON;
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize16][kColumn] =
      Adst16TransformLoopColumn_NEON;

  // Maximum transform size for Identity transform is 32.
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize4][kRow] =
      Identity4TransformLoopRow_NEON;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize4][kColumn] =
      Identity4TransformLoopColumn_NEON;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize8][kRow] =
      Identity8TransformLoopRow_NEON;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize8][kColumn] =
      Identity8TransformLoopColumn_NEON;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize16][kRow] =
      Identity16TransformLoopRow_NEON;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize16][kColumn] =
      Identity16TransformLoopColumn_NEON;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize32][kRow] =
      Identity32TransformLoopRow_NEON;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize32][kColumn] =
      Identity32TransformLoopColumn_NEON;

  // Maximum transform size for Wht is 4.
  dsp->inverse_transforms[kTransform1dWht][kTransform1dSize4][kRow] =
      Wht4TransformLoopRow_NEON;
  dsp->inverse_transforms[kTransform1dWht][kTransform1dSize4][kColumn] =
      Wht4TransformLoopColumn_NEON;
}

}  // namespace

void InverseTransformInit10bpp_NEON() { Init10bpp(); }

}  // namespace dsp
}  // namespace libgav1
#else   // !LIBGAV1_ENABLE_NEON || LIBGAV1_MAX_BITDEPTH < 10
namespace libgav1 {
namespace dsp {

void InverseTransformInit10bpp_NEON() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_ENABLE_NEON && LIBGAV1_MAX_BITDEPTH >= 10
