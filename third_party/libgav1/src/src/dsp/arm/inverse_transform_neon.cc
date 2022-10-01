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

#if LIBGAV1_ENABLE_NEON

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
namespace low_bitdepth {
namespace {

// Include the constants and utility functions inside the anonymous namespace.
#include "src/dsp/inverse_transform.inc"

//------------------------------------------------------------------------------

// Note this is only used in the final stage of Dct32/64 and Adst16 as the in
// place version causes additional stack usage with clang.
LIBGAV1_ALWAYS_INLINE void Transpose8x8(const int16x8_t in[8],
                                        int16x8_t out[8]) {
  // Swap 16 bit elements. Goes from:
  // a0: 00 01 02 03 04 05 06 07
  // a1: 10 11 12 13 14 15 16 17
  // a2: 20 21 22 23 24 25 26 27
  // a3: 30 31 32 33 34 35 36 37
  // a4: 40 41 42 43 44 45 46 47
  // a5: 50 51 52 53 54 55 56 57
  // a6: 60 61 62 63 64 65 66 67
  // a7: 70 71 72 73 74 75 76 77
  // to:
  // b0.val[0]: 00 10 02 12 04 14 06 16
  // b0.val[1]: 01 11 03 13 05 15 07 17
  // b1.val[0]: 20 30 22 32 24 34 26 36
  // b1.val[1]: 21 31 23 33 25 35 27 37
  // b2.val[0]: 40 50 42 52 44 54 46 56
  // b2.val[1]: 41 51 43 53 45 55 47 57
  // b3.val[0]: 60 70 62 72 64 74 66 76
  // b3.val[1]: 61 71 63 73 65 75 67 77

  const int16x8x2_t b0 = vtrnq_s16(in[0], in[1]);
  const int16x8x2_t b1 = vtrnq_s16(in[2], in[3]);
  const int16x8x2_t b2 = vtrnq_s16(in[4], in[5]);
  const int16x8x2_t b3 = vtrnq_s16(in[6], in[7]);

  // Swap 32 bit elements resulting in:
  // c0.val[0]: 00 10 20 30 04 14 24 34
  // c0.val[1]: 02 12 22 32 06 16 26 36
  // c1.val[0]: 01 11 21 31 05 15 25 35
  // c1.val[1]: 03 13 23 33 07 17 27 37
  // c2.val[0]: 40 50 60 70 44 54 64 74
  // c2.val[1]: 42 52 62 72 46 56 66 76
  // c3.val[0]: 41 51 61 71 45 55 65 75
  // c3.val[1]: 43 53 63 73 47 57 67 77

  const int32x4x2_t c0 = vtrnq_s32(vreinterpretq_s32_s16(b0.val[0]),
                                   vreinterpretq_s32_s16(b1.val[0]));
  const int32x4x2_t c1 = vtrnq_s32(vreinterpretq_s32_s16(b0.val[1]),
                                   vreinterpretq_s32_s16(b1.val[1]));
  const int32x4x2_t c2 = vtrnq_s32(vreinterpretq_s32_s16(b2.val[0]),
                                   vreinterpretq_s32_s16(b3.val[0]));
  const int32x4x2_t c3 = vtrnq_s32(vreinterpretq_s32_s16(b2.val[1]),
                                   vreinterpretq_s32_s16(b3.val[1]));

  // Swap 64 bit elements resulting in:
  // d0.val[0]: 00 10 20 30 40 50 60 70
  // d0.val[1]: 04 14 24 34 44 54 64 74
  // d1.val[0]: 01 11 21 31 41 51 61 71
  // d1.val[1]: 05 15 25 35 45 55 65 75
  // d2.val[0]: 02 12 22 32 42 52 62 72
  // d2.val[1]: 06 16 26 36 46 56 66 76
  // d3.val[0]: 03 13 23 33 43 53 63 73
  // d3.val[1]: 07 17 27 37 47 57 67 77
  const int16x8x2_t d0 = VtrnqS64(c0.val[0], c2.val[0]);
  const int16x8x2_t d1 = VtrnqS64(c1.val[0], c3.val[0]);
  const int16x8x2_t d2 = VtrnqS64(c0.val[1], c2.val[1]);
  const int16x8x2_t d3 = VtrnqS64(c1.val[1], c3.val[1]);

  out[0] = d0.val[0];
  out[1] = d1.val[0];
  out[2] = d2.val[0];
  out[3] = d3.val[0];
  out[4] = d0.val[1];
  out[5] = d1.val[1];
  out[6] = d2.val[1];
  out[7] = d3.val[1];
}

LIBGAV1_ALWAYS_INLINE void Transpose4x8To8x4(const uint16x8_t in[8],
                                             uint16x8_t out[4]) {
  // Swap 16 bit elements. Goes from:
  // a0: 00 01 02 03
  // a1: 10 11 12 13
  // a2: 20 21 22 23
  // a3: 30 31 32 33
  // a4: 40 41 42 43
  // a5: 50 51 52 53
  // a6: 60 61 62 63
  // a7: 70 71 72 73
  // to:
  // b0.val[0]: 00 10 02 12
  // b0.val[1]: 01 11 03 13
  // b1.val[0]: 20 30 22 32
  // b1.val[1]: 21 31 23 33
  // b2.val[0]: 40 50 42 52
  // b2.val[1]: 41 51 43 53
  // b3.val[0]: 60 70 62 72
  // b3.val[1]: 61 71 63 73

  uint16x4x2_t b0 = vtrn_u16(vget_low_u16(in[0]), vget_low_u16(in[1]));
  uint16x4x2_t b1 = vtrn_u16(vget_low_u16(in[2]), vget_low_u16(in[3]));
  uint16x4x2_t b2 = vtrn_u16(vget_low_u16(in[4]), vget_low_u16(in[5]));
  uint16x4x2_t b3 = vtrn_u16(vget_low_u16(in[6]), vget_low_u16(in[7]));

  // Swap 32 bit elements resulting in:
  // c0.val[0]: 00 10 20 30
  // c0.val[1]: 02 12 22 32
  // c1.val[0]: 01 11 21 31
  // c1.val[1]: 03 13 23 33
  // c2.val[0]: 40 50 60 70
  // c2.val[1]: 42 52 62 72
  // c3.val[0]: 41 51 61 71
  // c3.val[1]: 43 53 63 73

  uint32x2x2_t c0 = vtrn_u32(vreinterpret_u32_u16(b0.val[0]),
                             vreinterpret_u32_u16(b1.val[0]));
  uint32x2x2_t c1 = vtrn_u32(vreinterpret_u32_u16(b0.val[1]),
                             vreinterpret_u32_u16(b1.val[1]));
  uint32x2x2_t c2 = vtrn_u32(vreinterpret_u32_u16(b2.val[0]),
                             vreinterpret_u32_u16(b3.val[0]));
  uint32x2x2_t c3 = vtrn_u32(vreinterpret_u32_u16(b2.val[1]),
                             vreinterpret_u32_u16(b3.val[1]));

  // Swap 64 bit elements resulting in:
  // o0: 00 10 20 30 40 50 60 70
  // o1: 01 11 21 31 41 51 61 71
  // o2: 02 12 22 32 42 52 62 72
  // o3: 03 13 23 33 43 53 63 73

  out[0] = vcombine_u16(vreinterpret_u16_u32(c0.val[0]),
                        vreinterpret_u16_u32(c2.val[0]));
  out[1] = vcombine_u16(vreinterpret_u16_u32(c1.val[0]),
                        vreinterpret_u16_u32(c3.val[0]));
  out[2] = vcombine_u16(vreinterpret_u16_u32(c0.val[1]),
                        vreinterpret_u16_u32(c2.val[1]));
  out[3] = vcombine_u16(vreinterpret_u16_u32(c1.val[1]),
                        vreinterpret_u16_u32(c3.val[1]));
}

LIBGAV1_ALWAYS_INLINE void Transpose4x8To8x4(const int16x8_t in[8],
                                             int16x8_t out[4]) {
  Transpose4x8To8x4(reinterpret_cast<const uint16x8_t*>(in),
                    reinterpret_cast<uint16x8_t*>(out));
}

LIBGAV1_ALWAYS_INLINE void Transpose8x4To4x8(const int16x8_t in[4],
                                             int16x8_t out[8]) {
  // Swap 16 bit elements. Goes from:
  // a0: 00 01 02 03 04 05 06 07
  // a1: 10 11 12 13 14 15 16 17
  // a2: 20 21 22 23 24 25 26 27
  // a3: 30 31 32 33 34 35 36 37
  // to:
  // b0.val[0]: 00 10 02 12 04 14 06 16
  // b0.val[1]: 01 11 03 13 05 15 07 17
  // b1.val[0]: 20 30 22 32 24 34 26 36
  // b1.val[1]: 21 31 23 33 25 35 27 37
  const int16x8x2_t b0 = vtrnq_s16(in[0], in[1]);
  const int16x8x2_t b1 = vtrnq_s16(in[2], in[3]);

  // Swap 32 bit elements resulting in:
  // c0.val[0]: 00 10 20 30 04 14 24 34
  // c0.val[1]: 02 12 22 32 06 16 26 36
  // c1.val[0]: 01 11 21 31 05 15 25 35
  // c1.val[1]: 03 13 23 33 07 17 27 37
  const int32x4x2_t c0 = vtrnq_s32(vreinterpretq_s32_s16(b0.val[0]),
                                   vreinterpretq_s32_s16(b1.val[0]));
  const int32x4x2_t c1 = vtrnq_s32(vreinterpretq_s32_s16(b0.val[1]),
                                   vreinterpretq_s32_s16(b1.val[1]));

  // The upper 8 bytes are don't cares.
  // out[0]: 00 10 20 30 04 14 24 34
  // out[1]: 01 11 21 31 05 15 25 35
  // out[2]: 02 12 22 32 06 16 26 36
  // out[3]: 03 13 23 33 07 17 27 37
  // out[4]: 04 14 24 34 04 14 24 34
  // out[5]: 05 15 25 35 05 15 25 35
  // out[6]: 06 16 26 36 06 16 26 36
  // out[7]: 07 17 27 37 07 17 27 37
  out[0] = vreinterpretq_s16_s32(c0.val[0]);
  out[1] = vreinterpretq_s16_s32(c1.val[0]);
  out[2] = vreinterpretq_s16_s32(c0.val[1]);
  out[3] = vreinterpretq_s16_s32(c1.val[1]);
  out[4] = vreinterpretq_s16_s32(
      vcombine_s32(vget_high_s32(c0.val[0]), vget_high_s32(c0.val[0])));
  out[5] = vreinterpretq_s16_s32(
      vcombine_s32(vget_high_s32(c1.val[0]), vget_high_s32(c1.val[0])));
  out[6] = vreinterpretq_s16_s32(
      vcombine_s32(vget_high_s32(c0.val[1]), vget_high_s32(c0.val[1])));
  out[7] = vreinterpretq_s16_s32(
      vcombine_s32(vget_high_s32(c1.val[1]), vget_high_s32(c1.val[1])));
}

//------------------------------------------------------------------------------
template <int store_width, int store_count>
LIBGAV1_ALWAYS_INLINE void StoreDst(int16_t* LIBGAV1_RESTRICT dst,
                                    int32_t stride, int32_t idx,
                                    const int16x8_t* const s) {
  assert(store_count % 4 == 0);
  assert(store_width == 8 || store_width == 16);
  // NOTE: It is expected that the compiler will unroll these loops.
  if (store_width == 16) {
    for (int i = 0; i < store_count; i += 4) {
      vst1q_s16(&dst[i * stride + idx], (s[i]));
      vst1q_s16(&dst[(i + 1) * stride + idx], (s[i + 1]));
      vst1q_s16(&dst[(i + 2) * stride + idx], (s[i + 2]));
      vst1q_s16(&dst[(i + 3) * stride + idx], (s[i + 3]));
    }
  } else {
    // store_width == 8
    for (int i = 0; i < store_count; i += 4) {
      vst1_s16(&dst[i * stride + idx], vget_low_s16(s[i]));
      vst1_s16(&dst[(i + 1) * stride + idx], vget_low_s16(s[i + 1]));
      vst1_s16(&dst[(i + 2) * stride + idx], vget_low_s16(s[i + 2]));
      vst1_s16(&dst[(i + 3) * stride + idx], vget_low_s16(s[i + 3]));
    }
  }
}

template <int load_width, int load_count>
LIBGAV1_ALWAYS_INLINE void LoadSrc(const int16_t* LIBGAV1_RESTRICT src,
                                   int32_t stride, int32_t idx, int16x8_t* x) {
  assert(load_count % 4 == 0);
  assert(load_width == 8 || load_width == 16);
  // NOTE: It is expected that the compiler will unroll these loops.
  if (load_width == 16) {
    for (int i = 0; i < load_count; i += 4) {
      x[i] = vld1q_s16(&src[i * stride + idx]);
      x[i + 1] = vld1q_s16(&src[(i + 1) * stride + idx]);
      x[i + 2] = vld1q_s16(&src[(i + 2) * stride + idx]);
      x[i + 3] = vld1q_s16(&src[(i + 3) * stride + idx]);
    }
  } else {
    // load_width == 8
    const int64x2_t zero = vdupq_n_s64(0);
    for (int i = 0; i < load_count; i += 4) {
      // The src buffer is aligned to 32 bytes.  Each load will always be 8
      // byte aligned.
      x[i] = vreinterpretq_s16_s64(vld1q_lane_s64(
          reinterpret_cast<const int64_t*>(&src[i * stride + idx]), zero, 0));
      x[i + 1] = vreinterpretq_s16_s64(vld1q_lane_s64(
          reinterpret_cast<const int64_t*>(&src[(i + 1) * stride + idx]), zero,
          0));
      x[i + 2] = vreinterpretq_s16_s64(vld1q_lane_s64(
          reinterpret_cast<const int64_t*>(&src[(i + 2) * stride + idx]), zero,
          0));
      x[i + 3] = vreinterpretq_s16_s64(vld1q_lane_s64(
          reinterpret_cast<const int64_t*>(&src[(i + 3) * stride + idx]), zero,
          0));
    }
  }
}

// Butterfly rotate 4 values.
LIBGAV1_ALWAYS_INLINE void ButterflyRotation_4(int16x8_t* a, int16x8_t* b,
                                               const int angle,
                                               const bool flip) {
  const int16_t cos128 = Cos128(angle);
  const int16_t sin128 = Sin128(angle);
  const int32x4_t acc_x = vmull_n_s16(vget_low_s16(*a), cos128);
  const int32x4_t acc_y = vmull_n_s16(vget_low_s16(*a), sin128);
  const int32x4_t x0 = vmlsl_n_s16(acc_x, vget_low_s16(*b), sin128);
  const int32x4_t y0 = vmlal_n_s16(acc_y, vget_low_s16(*b), cos128);
  const int16x4_t x1 = vqrshrn_n_s32(x0, 12);
  const int16x4_t y1 = vqrshrn_n_s32(y0, 12);
  const int16x8_t x = vcombine_s16(x1, x1);
  const int16x8_t y = vcombine_s16(y1, y1);
  if (flip) {
    *a = y;
    *b = x;
  } else {
    *a = x;
    *b = y;
  }
}

// Butterfly rotate 8 values.
LIBGAV1_ALWAYS_INLINE void ButterflyRotation_8(int16x8_t* a, int16x8_t* b,
                                               const int angle,
                                               const bool flip) {
  const int16_t cos128 = Cos128(angle);
  const int16_t sin128 = Sin128(angle);
  const int32x4_t acc_x = vmull_n_s16(vget_low_s16(*a), cos128);
  const int32x4_t acc_y = vmull_n_s16(vget_low_s16(*a), sin128);
  const int32x4_t x0 = vmlsl_n_s16(acc_x, vget_low_s16(*b), sin128);
  const int32x4_t y0 = vmlal_n_s16(acc_y, vget_low_s16(*b), cos128);
  const int16x4_t x1 = vqrshrn_n_s32(x0, 12);
  const int16x4_t y1 = vqrshrn_n_s32(y0, 12);

  const int32x4_t acc_x_hi = vmull_n_s16(vget_high_s16(*a), cos128);
  const int32x4_t acc_y_hi = vmull_n_s16(vget_high_s16(*a), sin128);
  const int32x4_t x0_hi = vmlsl_n_s16(acc_x_hi, vget_high_s16(*b), sin128);
  const int32x4_t y0_hi = vmlal_n_s16(acc_y_hi, vget_high_s16(*b), cos128);
  const int16x4_t x1_hi = vqrshrn_n_s32(x0_hi, 12);
  const int16x4_t y1_hi = vqrshrn_n_s32(y0_hi, 12);

  const int16x8_t x = vcombine_s16(x1, x1_hi);
  const int16x8_t y = vcombine_s16(y1, y1_hi);
  if (flip) {
    *a = y;
    *b = x;
  } else {
    *a = x;
    *b = y;
  }
}

LIBGAV1_ALWAYS_INLINE void ButterflyRotation_FirstIsZero(int16x8_t* a,
                                                         int16x8_t* b,
                                                         const int angle,
                                                         const bool flip) {
#if defined(__ARM_FEATURE_QRDMX) && defined(__aarch64__) && \
    defined(__clang__)  // ARM v8.1-A
  // Clang optimizes vqrdmulhq_n_s16 and vqsubq_s16 (in HadamardRotation) into
  // vqrdmlshq_s16 resulting in an "off by one" error. For now, do not use
  // vqrdmulhq_n_s16().
  const int16_t cos128 = Cos128(angle);
  const int16_t sin128 = Sin128(angle);
  const int32x4_t x0 = vmull_n_s16(vget_low_s16(*b), -sin128);
  const int32x4_t y0 = vmull_n_s16(vget_low_s16(*b), cos128);
  const int16x4_t x1 = vqrshrn_n_s32(x0, 12);
  const int16x4_t y1 = vqrshrn_n_s32(y0, 12);

  const int32x4_t x0_hi = vmull_n_s16(vget_high_s16(*b), -sin128);
  const int32x4_t y0_hi = vmull_n_s16(vget_high_s16(*b), cos128);
  const int16x4_t x1_hi = vqrshrn_n_s32(x0_hi, 12);
  const int16x4_t y1_hi = vqrshrn_n_s32(y0_hi, 12);

  const int16x8_t x = vcombine_s16(x1, x1_hi);
  const int16x8_t y = vcombine_s16(y1, y1_hi);
  if (flip) {
    *a = y;
    *b = x;
  } else {
    *a = x;
    *b = y;
  }
#else
  const int16_t cos128 = Cos128(angle);
  const int16_t sin128 = Sin128(angle);
  // For this function, the max value returned by Sin128() is 4091, which fits
  // inside 12 bits.  This leaves room for the sign bit and the 3 left shifted
  // bits.
  assert(sin128 <= 0xfff);
  const int16x8_t x = vqrdmulhq_n_s16(*b, -sin128 << 3);
  const int16x8_t y = vqrdmulhq_n_s16(*b, cos128 << 3);
  if (flip) {
    *a = y;
    *b = x;
  } else {
    *a = x;
    *b = y;
  }
#endif
}

LIBGAV1_ALWAYS_INLINE void ButterflyRotation_SecondIsZero(int16x8_t* a,
                                                          int16x8_t* b,
                                                          const int angle,
                                                          const bool flip) {
#if defined(__ARM_FEATURE_QRDMX) && defined(__aarch64__) && \
    defined(__clang__)  // ARM v8.1-A
  // Clang optimizes vqrdmulhq_n_s16 and vqsubq_s16 (in HadamardRotation) into
  // vqrdmlshq_s16 resulting in an "off by one" error. For now, do not use
  // vqrdmulhq_n_s16().
  const int16_t cos128 = Cos128(angle);
  const int16_t sin128 = Sin128(angle);
  const int32x4_t x0 = vmull_n_s16(vget_low_s16(*a), cos128);
  const int32x4_t y0 = vmull_n_s16(vget_low_s16(*a), sin128);
  const int16x4_t x1 = vqrshrn_n_s32(x0, 12);
  const int16x4_t y1 = vqrshrn_n_s32(y0, 12);

  const int32x4_t x0_hi = vmull_n_s16(vget_high_s16(*a), cos128);
  const int32x4_t y0_hi = vmull_n_s16(vget_high_s16(*a), sin128);
  const int16x4_t x1_hi = vqrshrn_n_s32(x0_hi, 12);
  const int16x4_t y1_hi = vqrshrn_n_s32(y0_hi, 12);

  const int16x8_t x = vcombine_s16(x1, x1_hi);
  const int16x8_t y = vcombine_s16(y1, y1_hi);
  if (flip) {
    *a = y;
    *b = x;
  } else {
    *a = x;
    *b = y;
  }
#else
  const int16_t cos128 = Cos128(angle);
  const int16_t sin128 = Sin128(angle);
  const int16x8_t x = vqrdmulhq_n_s16(*a, cos128 << 3);
  const int16x8_t y = vqrdmulhq_n_s16(*a, sin128 << 3);
  if (flip) {
    *a = y;
    *b = x;
  } else {
    *a = x;
    *b = y;
  }
#endif
}

LIBGAV1_ALWAYS_INLINE void HadamardRotation(int16x8_t* a, int16x8_t* b,
                                            bool flip) {
  int16x8_t x, y;
  if (flip) {
    y = vqaddq_s16(*b, *a);
    x = vqsubq_s16(*b, *a);
  } else {
    x = vqaddq_s16(*a, *b);
    y = vqsubq_s16(*a, *b);
  }
  *a = x;
  *b = y;
}

using ButterflyRotationFunc = void (*)(int16x8_t* a, int16x8_t* b, int angle,
                                       bool flip);

//------------------------------------------------------------------------------
// Discrete Cosine Transforms (DCT).

template <int width>
LIBGAV1_ALWAYS_INLINE bool DctDcOnly(void* dest, int adjusted_tx_height,
                                     bool should_round, int row_shift) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int16_t*>(dest);
  const int16x8_t v_src = vdupq_n_s16(dst[0]);
  const uint16x8_t v_mask = vdupq_n_u16(should_round ? 0xffff : 0);
  const int16x8_t v_src_round =
      vqrdmulhq_n_s16(v_src, kTransformRowMultiplier << 3);
  const int16x8_t s0 = vbslq_s16(v_mask, v_src_round, v_src);
  const int16_t cos128 = Cos128(32);
  const int16x8_t xy = vqrdmulhq_n_s16(s0, cos128 << 3);
  // vqrshlq_s16 will shift right if shift value is negative.
  const int16x8_t xy_shifted = vqrshlq_s16(xy, vdupq_n_s16(-row_shift));

  if (width == 4) {
    vst1_s16(dst, vget_low_s16(xy_shifted));
  } else {
    for (int i = 0; i < width; i += 8) {
      vst1q_s16(dst, xy_shifted);
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
    const int16x4_t v_src = vld1_s16(dst);
    const int16x4_t xy = vqrdmulh_n_s16(v_src, cos128 << 3);
    vst1_s16(dst, xy);
  } else {
    int i = 0;
    do {
      const int16x8_t v_src = vld1q_s16(&dst[i]);
      const int16x8_t xy = vqrdmulhq_n_s16(v_src, cos128 << 3);
      vst1q_s16(&dst[i], xy);
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
LIBGAV1_ALWAYS_INLINE void Dct4Stages(int16x8_t* s) {
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

template <ButterflyRotationFunc butterfly_rotation, bool stage_is_rectangular>
LIBGAV1_ALWAYS_INLINE void Dct4_NEON(void* dest, int32_t step, bool transpose) {
  auto* const dst = static_cast<int16_t*>(dest);
  int16x8_t s[4], x[4];

  if (stage_is_rectangular) {
    if (transpose) {
      assert(step == 4);
      int16x8x4_t y = vld4q_s16(dst);
      for (int i = 0; i < 4; ++i) x[i] = y.val[i];
    } else {
      LoadSrc<16, 4>(dst, step, 0, x);
    }
  } else {
    if (transpose) {
      assert(step == 4);
      int16x4x4_t y = vld4_s16(dst);
      for (int i = 0; i < 4; ++i) x[i] = vcombine_s16(y.val[i], y.val[i]);
    } else {
      LoadSrc<8, 4>(dst, step, 0, x);
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
      int16x8x4_t y;
      for (int i = 0; i < 4; ++i) y.val[i] = s[i];
      vst4q_s16(dst, y);
    } else {
      StoreDst<16, 4>(dst, step, 0, s);
    }
  } else {
    if (transpose) {
      int16x4x4_t y;
      for (int i = 0; i < 4; ++i) y.val[i] = vget_low_s16(s[i]);
      vst4_s16(dst, y);
    } else {
      StoreDst<8, 4>(dst, step, 0, s);
    }
  }
}

template <ButterflyRotationFunc butterfly_rotation,
          bool is_fast_butterfly = false>
LIBGAV1_ALWAYS_INLINE void Dct8Stages(int16x8_t* s) {
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
LIBGAV1_ALWAYS_INLINE void Dct8_NEON(void* dest, int32_t step, bool transpose) {
  auto* const dst = static_cast<int16_t*>(dest);
  int16x8_t s[8], x[8];

  if (stage_is_rectangular) {
    if (transpose) {
      int16x8_t input[4];
      LoadSrc<16, 4>(dst, step, 0, input);
      Transpose8x4To4x8(input, x);
    } else {
      LoadSrc<8, 8>(dst, step, 0, x);
    }
  } else if (transpose) {
    LoadSrc<16, 8>(dst, step, 0, x);
    dsp::Transpose8x8(x);
  } else {
    LoadSrc<16, 8>(dst, step, 0, x);
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
      int16x8_t output[4];
      Transpose4x8To8x4(s, output);
      StoreDst<16, 4>(dst, step, 0, output);
    } else {
      StoreDst<8, 8>(dst, step, 0, s);
    }
  } else if (transpose) {
    dsp::Transpose8x8(s);
    StoreDst<16, 8>(dst, step, 0, s);
  } else {
    StoreDst<16, 8>(dst, step, 0, s);
  }
}

template <ButterflyRotationFunc butterfly_rotation,
          bool is_fast_butterfly = false>
LIBGAV1_ALWAYS_INLINE void Dct16Stages(int16x8_t* s) {
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
LIBGAV1_ALWAYS_INLINE void Dct16_NEON(void* dest, int32_t step, bool is_row,
                                      int row_shift) {
  auto* const dst = static_cast<int16_t*>(dest);
  int16x8_t s[16], x[16];

  if (stage_is_rectangular) {
    if (is_row) {
      int16x8_t input[4];
      LoadSrc<16, 4>(dst, step, 0, input);
      Transpose8x4To4x8(input, x);
      LoadSrc<16, 4>(dst, step, 8, input);
      Transpose8x4To4x8(input, &x[8]);
    } else {
      LoadSrc<8, 16>(dst, step, 0, x);
    }
  } else if (is_row) {
    for (int idx = 0; idx < 16; idx += 8) {
      LoadSrc<16, 8>(dst, step, idx, &x[idx]);
      dsp::Transpose8x8(&x[idx]);
    }
  } else {
    LoadSrc<16, 16>(dst, step, 0, x);
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

  if (is_row) {
    const int16x8_t v_row_shift = vdupq_n_s16(-row_shift);
    for (auto& i : s) {
      i = vqrshlq_s16(i, v_row_shift);
    }
  }

  if (stage_is_rectangular) {
    if (is_row) {
      int16x8_t output[4];
      Transpose4x8To8x4(s, output);
      StoreDst<16, 4>(dst, step, 0, output);
      Transpose4x8To8x4(&s[8], output);
      StoreDst<16, 4>(dst, step, 8, output);
    } else {
      StoreDst<8, 16>(dst, step, 0, s);
    }
  } else if (is_row) {
    for (int idx = 0; idx < 16; idx += 8) {
      dsp::Transpose8x8(&s[idx]);
      StoreDst<16, 8>(dst, step, idx, &s[idx]);
    }
  } else {
    StoreDst<16, 16>(dst, step, 0, s);
  }
}

template <ButterflyRotationFunc butterfly_rotation,
          bool is_fast_butterfly = false>
LIBGAV1_ALWAYS_INLINE void Dct32Stages(int16x8_t* s) {
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
LIBGAV1_ALWAYS_INLINE void Dct32_NEON(void* dest, const int32_t step,
                                      const bool is_row, int row_shift) {
  auto* const dst = static_cast<int16_t*>(dest);
  int16x8_t s[32], x[32];

  if (is_row) {
    for (int idx = 0; idx < 32; idx += 8) {
      LoadSrc<16, 8>(dst, step, idx, &x[idx]);
      dsp::Transpose8x8(&x[idx]);
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

  if (is_row) {
    const int16x8_t v_row_shift = vdupq_n_s16(-row_shift);
    for (int idx = 0; idx < 32; idx += 8) {
      int16x8_t output[8];
      Transpose8x8(&s[idx], output);
      for (auto& o : output) {
        o = vqrshlq_s16(o, v_row_shift);
      }
      StoreDst<16, 8>(dst, step, idx, output);
    }
  } else {
    StoreDst<16, 32>(dst, step, 0, s);
  }
}

// Allow the compiler to call this function instead of force inlining. Tests
// show the performance is slightly faster.
void Dct64_NEON(void* dest, int32_t step, bool is_row, int row_shift) {
  auto* const dst = static_cast<int16_t*>(dest);
  int16x8_t s[64], x[32];

  if (is_row) {
    // The last 32 values of every row are always zero if the |tx_width| is
    // 64.
    for (int idx = 0; idx < 32; idx += 8) {
      LoadSrc<16, 8>(dst, step, idx, &x[idx]);
      dsp::Transpose8x8(&x[idx]);
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

  if (is_row) {
    const int16x8_t v_row_shift = vdupq_n_s16(-row_shift);
    for (int idx = 0; idx < 64; idx += 8) {
      int16x8_t output[8];
      Transpose8x8(&s[idx], output);
      for (auto& o : output) {
        o = vqrshlq_s16(o, v_row_shift);
      }
      StoreDst<16, 8>(dst, step, idx, output);
    }
  } else {
    StoreDst<16, 64>(dst, step, 0, s);
  }
}

//------------------------------------------------------------------------------
// Asymmetric Discrete Sine Transforms (ADST).

LIBGAV1_ALWAYS_INLINE void Adst4_NEON(void* dest, int32_t step,
                                      bool transpose) {
  auto* const dst = static_cast<int16_t*>(dest);
  int32x4_t s[7];
  int16x4_t x[4];

  if (transpose) {
    assert(step == 4);
    int16x4x4_t y = vld4_s16(dst);
    for (int i = 0; i < 4; ++i) x[i] = y.val[i];
  } else {
    x[0] = vld1_s16(dst);
    x[1] = vld1_s16(dst + 1 * step);
    x[2] = vld1_s16(dst + 2 * step);
    x[3] = vld1_s16(dst + 3 * step);
  }

  // stage 1.
  s[5] = vmull_n_s16(x[3], kAdst4Multiplier[1]);
  s[6] = vmull_n_s16(x[3], kAdst4Multiplier[3]);

  // stage 2.
  const int32x4_t a7 = vsubl_s16(x[0], x[2]);
  const int32x4_t b7 = vaddw_s16(a7, x[3]);

  // stage 3.
  s[0] = vmull_n_s16(x[0], kAdst4Multiplier[0]);
  s[1] = vmull_n_s16(x[0], kAdst4Multiplier[1]);
  // s[0] = s[0] + s[3]
  s[0] = vmlal_n_s16(s[0], x[2], kAdst4Multiplier[3]);
  // s[1] = s[1] - s[4]
  s[1] = vmlsl_n_s16(s[1], x[2], kAdst4Multiplier[0]);

  s[3] = vmull_n_s16(x[1], kAdst4Multiplier[2]);
  s[2] = vmulq_n_s32(b7, kAdst4Multiplier[2]);

  // stage 4.
  s[0] = vaddq_s32(s[0], s[5]);
  s[1] = vsubq_s32(s[1], s[6]);

  // stages 5 and 6.
  const int32x4_t x0 = vaddq_s32(s[0], s[3]);
  const int32x4_t x1 = vaddq_s32(s[1], s[3]);
  const int32x4_t x3_a = vaddq_s32(s[0], s[1]);
  const int32x4_t x3 = vsubq_s32(x3_a, s[3]);
  const int16x4_t dst_0 = vqrshrn_n_s32(x0, 12);
  const int16x4_t dst_1 = vqrshrn_n_s32(x1, 12);
  const int16x4_t dst_2 = vqrshrn_n_s32(s[2], 12);
  const int16x4_t dst_3 = vqrshrn_n_s32(x3, 12);

  x[0] = dst_0;
  x[1] = dst_1;
  x[2] = dst_2;
  x[3] = dst_3;

  if (transpose) {
    int16x4x4_t y;
    for (int i = 0; i < 4; ++i) y.val[i] = x[i];
    vst4_s16(dst, y);
  } else {
    vst1_s16(dst, x[0]);
    vst1_s16(dst + 1 * step, x[1]);
    vst1_s16(dst + 2 * step, x[2]);
    vst1_s16(dst + 3 * step, x[3]);
  }
}

alignas(8) constexpr int16_t kAdst4DcOnlyMultiplier[4] = {1321, 2482, 3344,
                                                          2482};

LIBGAV1_ALWAYS_INLINE bool Adst4DcOnly(void* dest, int adjusted_tx_height,
                                       bool should_round, int row_shift) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int16_t*>(dest);
  int32x4_t s[2];

  const int16x4_t v_src0 = vdup_n_s16(dst[0]);
  const uint16x4_t v_mask = vdup_n_u16(should_round ? 0xffff : 0);
  const int16x4_t v_src_round =
      vqrdmulh_n_s16(v_src0, kTransformRowMultiplier << 3);
  const int16x4_t v_src = vbsl_s16(v_mask, v_src_round, v_src0);
  const int16x4_t kAdst4DcOnlyMultipliers = vld1_s16(kAdst4DcOnlyMultiplier);
  s[1] = vdupq_n_s32(0);

  // s0*k0 s0*k1 s0*k2 s0*k1
  s[0] = vmull_s16(kAdst4DcOnlyMultipliers, v_src);
  // 0     0     0     s0*k0
  s[1] = vextq_s32(s[1], s[0], 1);

  const int32x4_t x3 = vaddq_s32(s[0], s[1]);
  const int16x4_t dst_0 = vqrshrn_n_s32(x3, 12);

  // vqrshlq_s16 will shift right if shift value is negative.
  vst1_s16(dst, vqrshl_s16(dst_0, vdup_n_s16(-row_shift)));

  return true;
}

LIBGAV1_ALWAYS_INLINE bool Adst4DcOnlyColumn(void* dest, int adjusted_tx_height,
                                             int width) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int16_t*>(dest);
  int32x4_t s[4];

  int i = 0;
  do {
    const int16x4_t v_src = vld1_s16(&dst[i]);

    s[0] = vmull_n_s16(v_src, kAdst4Multiplier[0]);
    s[1] = vmull_n_s16(v_src, kAdst4Multiplier[1]);
    s[2] = vmull_n_s16(v_src, kAdst4Multiplier[2]);

    const int32x4_t x0 = s[0];
    const int32x4_t x1 = s[1];
    const int32x4_t x2 = s[2];
    const int32x4_t x3 = vaddq_s32(s[0], s[1]);
    const int16x4_t dst_0 = vqrshrn_n_s32(x0, 12);
    const int16x4_t dst_1 = vqrshrn_n_s32(x1, 12);
    const int16x4_t dst_2 = vqrshrn_n_s32(x2, 12);
    const int16x4_t dst_3 = vqrshrn_n_s32(x3, 12);

    vst1_s16(&dst[i], dst_0);
    vst1_s16(&dst[i + width * 1], dst_1);
    vst1_s16(&dst[i + width * 2], dst_2);
    vst1_s16(&dst[i + width * 3], dst_3);

    i += 4;
  } while (i < width);

  return true;
}

template <ButterflyRotationFunc butterfly_rotation, bool stage_is_rectangular>
LIBGAV1_ALWAYS_INLINE void Adst8_NEON(void* dest, int32_t step,
                                      bool transpose) {
  auto* const dst = static_cast<int16_t*>(dest);
  int16x8_t s[8], x[8];

  if (stage_is_rectangular) {
    if (transpose) {
      int16x8_t input[4];
      LoadSrc<16, 4>(dst, step, 0, input);
      Transpose8x4To4x8(input, x);
    } else {
      LoadSrc<8, 8>(dst, step, 0, x);
    }
  } else {
    if (transpose) {
      LoadSrc<16, 8>(dst, step, 0, x);
      dsp::Transpose8x8(x);
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
  x[0] = s[0];
  x[1] = vqnegq_s16(s[4]);
  x[2] = s[6];
  x[3] = vqnegq_s16(s[2]);
  x[4] = s[3];
  x[5] = vqnegq_s16(s[7]);
  x[6] = s[5];
  x[7] = vqnegq_s16(s[1]);

  if (stage_is_rectangular) {
    if (transpose) {
      int16x8_t output[4];
      Transpose4x8To8x4(x, output);
      StoreDst<16, 4>(dst, step, 0, output);
    } else {
      StoreDst<8, 8>(dst, step, 0, x);
    }
  } else {
    if (transpose) {
      dsp::Transpose8x8(x);
      StoreDst<16, 8>(dst, step, 0, x);
    } else {
      StoreDst<16, 8>(dst, step, 0, x);
    }
  }
}

LIBGAV1_ALWAYS_INLINE bool Adst8DcOnly(void* dest, int adjusted_tx_height,
                                       bool should_round, int row_shift) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int16_t*>(dest);
  int16x8_t s[8];

  const int16x8_t v_src = vdupq_n_s16(dst[0]);
  const uint16x8_t v_mask = vdupq_n_u16(should_round ? 0xffff : 0);
  const int16x8_t v_src_round =
      vqrdmulhq_n_s16(v_src, kTransformRowMultiplier << 3);
  // stage 1.
  s[1] = vbslq_s16(v_mask, v_src_round, v_src);

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
  int16x8_t x[8];
  x[0] = s[0];
  x[1] = vqnegq_s16(s[4]);
  x[2] = s[6];
  x[3] = vqnegq_s16(s[2]);
  x[4] = s[3];
  x[5] = vqnegq_s16(s[7]);
  x[6] = s[5];
  x[7] = vqnegq_s16(s[1]);

  for (int i = 0; i < 8; ++i) {
    // vqrshlq_s16 will shift right if shift value is negative.
    x[i] = vqrshlq_s16(x[i], vdupq_n_s16(-row_shift));
    vst1q_lane_s16(&dst[i], x[i], 0);
  }

  return true;
}

LIBGAV1_ALWAYS_INLINE bool Adst8DcOnlyColumn(void* dest, int adjusted_tx_height,
                                             int width) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int16_t*>(dest);
  int16x8_t s[8];

  int i = 0;
  do {
    const int16x8_t v_src = vld1q_s16(dst);
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
    int16x8_t x[8];
    x[0] = s[0];
    x[1] = vqnegq_s16(s[4]);
    x[2] = s[6];
    x[3] = vqnegq_s16(s[2]);
    x[4] = s[3];
    x[5] = vqnegq_s16(s[7]);
    x[6] = s[5];
    x[7] = vqnegq_s16(s[1]);

    for (int j = 0; j < 8; ++j) {
      vst1_s16(&dst[j * width], vget_low_s16(x[j]));
    }
    i += 4;
    dst += 4;
  } while (i < width);

  return true;
}

template <ButterflyRotationFunc butterfly_rotation, bool stage_is_rectangular>
LIBGAV1_ALWAYS_INLINE void Adst16_NEON(void* dest, int32_t step, bool is_row,
                                       int row_shift) {
  auto* const dst = static_cast<int16_t*>(dest);
  int16x8_t s[16], x[16];

  if (stage_is_rectangular) {
    if (is_row) {
      int16x8_t input[4];
      LoadSrc<16, 4>(dst, step, 0, input);
      Transpose8x4To4x8(input, x);
      LoadSrc<16, 4>(dst, step, 8, input);
      Transpose8x4To4x8(input, &x[8]);
    } else {
      LoadSrc<8, 16>(dst, step, 0, x);
    }
  } else {
    if (is_row) {
      for (int idx = 0; idx < 16; idx += 8) {
        LoadSrc<16, 8>(dst, step, idx, &x[idx]);
        dsp::Transpose8x8(&x[idx]);
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
  x[0] = s[0];
  x[1] = vqnegq_s16(s[8]);
  x[2] = s[12];
  x[3] = vqnegq_s16(s[4]);
  x[4] = s[6];
  x[5] = vqnegq_s16(s[14]);
  x[6] = s[10];
  x[7] = vqnegq_s16(s[2]);
  x[8] = s[3];
  x[9] = vqnegq_s16(s[11]);
  x[10] = s[15];
  x[11] = vqnegq_s16(s[7]);
  x[12] = s[5];
  x[13] = vqnegq_s16(s[13]);
  x[14] = s[9];
  x[15] = vqnegq_s16(s[1]);

  if (stage_is_rectangular) {
    if (is_row) {
      const int16x8_t v_row_shift = vdupq_n_s16(-row_shift);
      int16x8_t output[4];
      Transpose4x8To8x4(x, output);
      for (auto& o : output) {
        o = vqrshlq_s16(o, v_row_shift);
      }
      StoreDst<16, 4>(dst, step, 0, output);
      Transpose4x8To8x4(&x[8], output);
      for (auto& o : output) {
        o = vqrshlq_s16(o, v_row_shift);
      }
      StoreDst<16, 4>(dst, step, 8, output);
    } else {
      StoreDst<8, 16>(dst, step, 0, x);
    }
  } else {
    if (is_row) {
      const int16x8_t v_row_shift = vdupq_n_s16(-row_shift);
      for (int idx = 0; idx < 16; idx += 8) {
        int16x8_t output[8];
        Transpose8x8(&x[idx], output);
        for (auto& o : output) {
          o = vqrshlq_s16(o, v_row_shift);
        }
        StoreDst<16, 8>(dst, step, idx, output);
      }
    } else {
      StoreDst<16, 16>(dst, step, 0, x);
    }
  }
}

LIBGAV1_ALWAYS_INLINE void Adst16DcOnlyInternal(int16x8_t* s, int16x8_t* x) {
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
  x[1] = vqnegq_s16(s[8]);
  x[2] = s[12];
  x[3] = vqnegq_s16(s[4]);
  x[4] = s[6];
  x[5] = vqnegq_s16(s[14]);
  x[6] = s[10];
  x[7] = vqnegq_s16(s[2]);
  x[8] = s[3];
  x[9] = vqnegq_s16(s[11]);
  x[10] = s[15];
  x[11] = vqnegq_s16(s[7]);
  x[12] = s[5];
  x[13] = vqnegq_s16(s[13]);
  x[14] = s[9];
  x[15] = vqnegq_s16(s[1]);
}

LIBGAV1_ALWAYS_INLINE bool Adst16DcOnly(void* dest, int adjusted_tx_height,
                                        bool should_round, int row_shift) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int16_t*>(dest);
  int16x8_t s[16];
  int16x8_t x[16];

  const int16x8_t v_src = vdupq_n_s16(dst[0]);
  const uint16x8_t v_mask = vdupq_n_u16(should_round ? 0xffff : 0);
  const int16x8_t v_src_round =
      vqrdmulhq_n_s16(v_src, kTransformRowMultiplier << 3);
  // stage 1.
  s[1] = vbslq_s16(v_mask, v_src_round, v_src);

  Adst16DcOnlyInternal(s, x);

  for (int i = 0; i < 16; ++i) {
    // vqrshlq_s16 will shift right if shift value is negative.
    x[i] = vqrshlq_s16(x[i], vdupq_n_s16(-row_shift));
    vst1q_lane_s16(&dst[i], x[i], 0);
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
    int16x8_t s[16];
    int16x8_t x[16];
    const int16x8_t v_src = vld1q_s16(dst);
    // stage 1.
    s[1] = v_src;

    Adst16DcOnlyInternal(s, x);

    for (int j = 0; j < 16; ++j) {
      vst1_s16(&dst[j * width], vget_low_s16(x[j]));
    }
    i += 4;
    dst += 4;
  } while (i < width);

  return true;
}

//------------------------------------------------------------------------------
// Identity Transforms.

template <bool is_row_shift>
LIBGAV1_ALWAYS_INLINE void Identity4_NEON(void* dest, int32_t step) {
  auto* const dst = static_cast<int16_t*>(dest);

  if (is_row_shift) {
    const int shift = 1;
    const int32x4_t v_dual_round = vdupq_n_s32((1 + (shift << 1)) << 11);
    const int16x4_t v_multiplier = vdup_n_s16(kIdentity4Multiplier);
    const int32x4_t v_shift = vdupq_n_s32(-(12 + shift));
    for (int i = 0; i < 4; i += 2) {
      const int16x8_t v_src = vld1q_s16(&dst[i * step]);
      const int32x4_t v_src_mult_lo =
          vmlal_s16(v_dual_round, vget_low_s16(v_src), v_multiplier);
      const int32x4_t v_src_mult_hi =
          vmlal_s16(v_dual_round, vget_high_s16(v_src), v_multiplier);
      const int32x4_t shift_lo = vqshlq_s32(v_src_mult_lo, v_shift);
      const int32x4_t shift_hi = vqshlq_s32(v_src_mult_hi, v_shift);
      vst1q_s16(&dst[i * step],
                vcombine_s16(vqmovn_s32(shift_lo), vqmovn_s32(shift_hi)));
    }
  } else {
    for (int i = 0; i < 4; i += 2) {
      const int16x8_t v_src = vld1q_s16(&dst[i * step]);
      const int16x8_t a =
          vqrdmulhq_n_s16(v_src, kIdentity4MultiplierFraction << 3);
      const int16x8_t b = vqaddq_s16(v_src, a);
      vst1q_s16(&dst[i * step], b);
    }
  }
}

LIBGAV1_ALWAYS_INLINE bool Identity4DcOnly(void* dest, int adjusted_tx_height,
                                           bool should_round, int tx_height) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int16_t*>(dest);
  const int16x4_t v_src0 = vdup_n_s16(dst[0]);
  const uint16x4_t v_mask = vdup_n_u16(should_round ? 0xffff : 0);
  const int16x4_t v_src_round =
      vqrdmulh_n_s16(v_src0, kTransformRowMultiplier << 3);
  const int16x4_t v_src = vbsl_s16(v_mask, v_src_round, v_src0);
  const int shift = tx_height < 16 ? 0 : 1;
  const int32x4_t v_dual_round = vdupq_n_s32((1 + (shift << 1)) << 11);
  const int16x4_t v_multiplier = vdup_n_s16(kIdentity4Multiplier);
  const int32x4_t v_shift = vdupq_n_s32(-(12 + shift));
  const int32x4_t v_src_mult_lo = vmlal_s16(v_dual_round, v_src, v_multiplier);
  const int32x4_t dst_0 = vqshlq_s32(v_src_mult_lo, v_shift);
  vst1_lane_s16(dst, vqmovn_s32(dst_0), 0);
  return true;
}

template <int identity_size>
LIBGAV1_ALWAYS_INLINE void IdentityColumnStoreToFrame(
    Array2DView<uint8_t> frame, const int start_x, const int start_y,
    const int tx_width, const int tx_height,
    const int16_t* LIBGAV1_RESTRICT source) {
  const int stride = frame.columns();
  uint8_t* LIBGAV1_RESTRICT dst = frame[start_y] + start_x;

  if (identity_size < 32) {
    if (tx_width == 4) {
      uint8x8_t frame_data = vdup_n_u8(0);
      int i = 0;
      do {
        const int16x4_t v_src = vld1_s16(&source[i * tx_width]);

        int16x4_t v_dst_i;
        if (identity_size == 4) {
          const int16x4_t v_src_fraction =
              vqrdmulh_n_s16(v_src, kIdentity4MultiplierFraction << 3);
          v_dst_i = vqadd_s16(v_src, v_src_fraction);
        } else if (identity_size == 8) {
          v_dst_i = vqadd_s16(v_src, v_src);
        } else {  // identity_size == 16
          const int16x4_t v_src_mult =
              vqrdmulh_n_s16(v_src, kIdentity4MultiplierFraction << 4);
          const int16x4_t v_srcx2 = vqadd_s16(v_src, v_src);
          v_dst_i = vqadd_s16(v_srcx2, v_src_mult);
        }

        frame_data = Load4<0>(dst, frame_data);
        const int16x4_t a = vrshr_n_s16(v_dst_i, 4);
        const uint16x8_t b =
            vaddw_u8(vreinterpretq_u16_s16(vcombine_s16(a, a)), frame_data);
        const uint8x8_t d = vqmovun_s16(vreinterpretq_s16_u16(b));
        StoreLo4(dst, d);
        dst += stride;
      } while (++i < tx_height);
    } else {
      int i = 0;
      do {
        const int row = i * tx_width;
        int j = 0;
        do {
          const int16x8_t v_src = vld1q_s16(&source[row + j]);

          int16x8_t v_dst_i;
          if (identity_size == 4) {
            const int16x8_t v_src_fraction =
                vqrdmulhq_n_s16(v_src, kIdentity4MultiplierFraction << 3);
            v_dst_i = vqaddq_s16(v_src, v_src_fraction);
          } else if (identity_size == 8) {
            v_dst_i = vqaddq_s16(v_src, v_src);
          } else {  // identity_size == 16
            const int16x8_t v_src_mult =
                vqrdmulhq_n_s16(v_src, kIdentity4MultiplierFraction << 4);
            const int16x8_t v_srcx2 = vqaddq_s16(v_src, v_src);
            v_dst_i = vqaddq_s16(v_src_mult, v_srcx2);
          }

          const uint8x8_t frame_data = vld1_u8(dst + j);
          const int16x8_t a = vrshrq_n_s16(v_dst_i, 4);
          const uint16x8_t b = vaddw_u8(vreinterpretq_u16_s16(a), frame_data);
          const uint8x8_t d = vqmovun_s16(vreinterpretq_s16_u16(b));
          vst1_u8(dst + j, d);
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
        const int16x8_t v_dst_i = vld1q_s16(&source[row + j]);
        const uint8x8_t frame_data = vld1_u8(dst + j);
        const int16x8_t a = vrshrq_n_s16(v_dst_i, 2);
        const uint16x8_t b = vaddw_u8(vreinterpretq_u16_s16(a), frame_data);
        const uint8x8_t d = vqmovun_s16(vreinterpretq_s16_u16(b));
        vst1_u8(dst + j, d);
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

  if (tx_width == 4) {
    uint8x8_t frame_data = vdup_n_u8(0);
    int i = 0;
    do {
      const int16x4_t v_src = vld1_s16(&source[i * tx_width]);
      const int16x4_t v_src_mult =
          vqrdmulh_n_s16(v_src, kIdentity4MultiplierFraction << 3);
      const int16x4_t v_dst_row = vqadd_s16(v_src, v_src_mult);
      const int16x4_t v_src_mult2 =
          vqrdmulh_n_s16(v_dst_row, kIdentity4MultiplierFraction << 3);
      const int16x4_t v_dst_col = vqadd_s16(v_dst_row, v_src_mult2);
      frame_data = Load4<0>(dst, frame_data);
      const int16x4_t a = vrshr_n_s16(v_dst_col, 4);
      const uint16x8_t b =
          vaddw_u8(vreinterpretq_u16_s16(vcombine_s16(a, a)), frame_data);
      const uint8x8_t d = vqmovun_s16(vreinterpretq_s16_u16(b));
      StoreLo4(dst, d);
      dst += stride;
    } while (++i < tx_height);
  } else {
    int i = 0;
    do {
      const int row = i * tx_width;
      int j = 0;
      do {
        const int16x8_t v_src = vld1q_s16(&source[row + j]);
        const int16x8_t v_src_round =
            vqrdmulhq_n_s16(v_src, kTransformRowMultiplier << 3);
        const int16x8_t v_dst_row = vqaddq_s16(v_src_round, v_src_round);
        const int16x8_t v_src_mult2 =
            vqrdmulhq_n_s16(v_dst_row, kIdentity4MultiplierFraction << 3);
        const int16x8_t v_dst_col = vqaddq_s16(v_dst_row, v_src_mult2);
        const uint8x8_t frame_data = vld1_u8(dst + j);
        const int16x8_t a = vrshrq_n_s16(v_dst_col, 4);
        const uint16x8_t b = vaddw_u8(vreinterpretq_u16_s16(a), frame_data);
        const uint8x8_t d = vqmovun_s16(vreinterpretq_s16_u16(b));
        vst1_u8(dst + j, d);
        j += 8;
      } while (j < tx_width);
      dst += stride;
    } while (++i < tx_height);
  }
}

LIBGAV1_ALWAYS_INLINE void Identity8Row32_NEON(void* dest, int32_t step) {
  auto* const dst = static_cast<int16_t*>(dest);

  // When combining the identity8 multiplier with the row shift, the
  // calculations for tx_height equal to 32 can be simplified from
  // ((A * 2) + 2) >> 2) to ((A + 1) >> 1).
  for (int i = 0; i < 4; ++i) {
    const int16x8_t v_src = vld1q_s16(&dst[i * step]);
    const int16x8_t a = vrshrq_n_s16(v_src, 1);
    vst1q_s16(&dst[i * step], a);
  }
}

LIBGAV1_ALWAYS_INLINE void Identity8Row4_NEON(void* dest, int32_t step) {
  auto* const dst = static_cast<int16_t*>(dest);

  for (int i = 0; i < 4; ++i) {
    const int16x8_t v_src = vld1q_s16(&dst[i * step]);
    // For bitdepth == 8, the identity row clamps to a signed 16bit value, so
    // saturating add here is ok.
    const int16x8_t v_srcx2 = vqaddq_s16(v_src, v_src);
    vst1q_s16(&dst[i * step], v_srcx2);
  }
}

LIBGAV1_ALWAYS_INLINE bool Identity8DcOnly(void* dest, int adjusted_tx_height,
                                           bool should_round, int row_shift) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int16_t*>(dest);
  const int16x4_t v_src0 = vdup_n_s16(dst[0]);
  const uint16x4_t v_mask = vdup_n_u16(should_round ? 0xffff : 0);
  const int16x4_t v_src_round =
      vqrdmulh_n_s16(v_src0, kTransformRowMultiplier << 3);
  const int16x4_t v_src = vbsl_s16(v_mask, v_src_round, v_src0);
  const int32x4_t v_srcx2 = vaddl_s16(v_src, v_src);
  const int32x4_t dst_0 = vqrshlq_s32(v_srcx2, vdupq_n_s32(-row_shift));
  vst1_lane_s16(dst, vqmovn_s32(dst_0), 0);
  return true;
}

LIBGAV1_ALWAYS_INLINE void Identity16Row_NEON(void* dest, int32_t step,
                                              int shift) {
  auto* const dst = static_cast<int16_t*>(dest);
  const int32x4_t v_dual_round = vdupq_n_s32((1 + (shift << 1)) << 11);
  const int32x4_t v_shift = vdupq_n_s32(-(12 + shift));

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 2; ++j) {
      const int16x8_t v_src = vld1q_s16(&dst[i * step + j * 8]);
      const int32x4_t v_src_mult_lo =
          vmlal_n_s16(v_dual_round, vget_low_s16(v_src), kIdentity16Multiplier);
      const int32x4_t v_src_mult_hi = vmlal_n_s16(
          v_dual_round, vget_high_s16(v_src), kIdentity16Multiplier);
      const int32x4_t shift_lo = vqshlq_s32(v_src_mult_lo, v_shift);
      const int32x4_t shift_hi = vqshlq_s32(v_src_mult_hi, v_shift);
      vst1q_s16(&dst[i * step + j * 8],
                vcombine_s16(vqmovn_s32(shift_lo), vqmovn_s32(shift_hi)));
    }
  }
}

LIBGAV1_ALWAYS_INLINE bool Identity16DcOnly(void* dest, int adjusted_tx_height,
                                            bool should_round, int shift) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int16_t*>(dest);
  const int16x4_t v_src0 = vdup_n_s16(dst[0]);
  const uint16x4_t v_mask = vdup_n_u16(should_round ? 0xffff : 0);
  const int16x4_t v_src_round =
      vqrdmulh_n_s16(v_src0, kTransformRowMultiplier << 3);
  const int16x4_t v_src = vbsl_s16(v_mask, v_src_round, v_src0);
  const int32x4_t v_dual_round = vdupq_n_s32((1 + (shift << 1)) << 11);
  const int16x4_t v_multiplier = vdup_n_s16(kIdentity16Multiplier);
  const int32x4_t v_shift = vdupq_n_s32(-(12 + shift));
  const int32x4_t v_src_mult_lo =
      vmlal_s16(v_dual_round, (v_src), v_multiplier);
  const int32x4_t dst_0 = vqshlq_s32(v_src_mult_lo, v_shift);
  vst1_lane_s16(dst, vqmovn_s32(dst_0), 0);
  return true;
}

LIBGAV1_ALWAYS_INLINE void Identity32Row16_NEON(void* dest,
                                                const int32_t step) {
  auto* const dst = static_cast<int16_t*>(dest);

  // When combining the identity32 multiplier with the row shift, the
  // calculation for tx_height equal to 16 can be simplified from
  // ((A * 4) + 1) >> 1) to (A * 2).
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 32; j += 8) {
      const int16x8_t v_src = vld1q_s16(&dst[i * step + j]);
      // For bitdepth == 8, the identity row clamps to a signed 16bit value, so
      // saturating add here is ok.
      const int16x8_t v_dst_i = vqaddq_s16(v_src, v_src);
      vst1q_s16(&dst[i * step + j], v_dst_i);
    }
  }
}

LIBGAV1_ALWAYS_INLINE bool Identity32DcOnly(void* dest,
                                            int adjusted_tx_height) {
  if (adjusted_tx_height > 1) return false;

  auto* dst = static_cast<int16_t*>(dest);
  const int16x4_t v_src0 = vdup_n_s16(dst[0]);
  const int16x4_t v_src = vqrdmulh_n_s16(v_src0, kTransformRowMultiplier << 3);
  // When combining the identity32 multiplier with the row shift, the
  // calculation for tx_height equal to 16 can be simplified from
  // ((A * 4) + 1) >> 1) to (A * 2).
  const int16x4_t v_dst_0 = vqadd_s16(v_src, v_src);
  vst1_lane_s16(dst, v_dst_0, 0);
  return true;
}

//------------------------------------------------------------------------------
// Walsh Hadamard Transform.

// Transposes a 4x4 matrix and then permutes the rows of the transposed matrix
// for the WHT. The input matrix is in two "wide" int16x8_t variables. The
// output matrix is in four int16x4_t variables.
//
// Input:
// in[0]: 00 01 02 03  10 11 12 13
// in[1]: 20 21 22 23  30 31 32 33
// Output:
// out[0]: 00 10 20 30
// out[1]: 03 13 23 33
// out[2]: 01 11 21 31
// out[3]: 02 12 22 32
LIBGAV1_ALWAYS_INLINE void TransposeAndPermute4x4WideInput(
    const int16x8_t in[2], int16x4_t out[4]) {
  // Swap 32 bit elements. Goes from:
  // in[0]: 00 01 02 03  10 11 12 13
  // in[1]: 20 21 22 23  30 31 32 33
  // to:
  // b0.val[0]: 00 01 20 21  10 11 30 31
  // b0.val[1]: 02 03 22 23  12 13 32 33

  const int32x4x2_t b0 =
      vtrnq_s32(vreinterpretq_s32_s16(in[0]), vreinterpretq_s32_s16(in[1]));

  // Swap 16 bit elements. Goes from:
  // vget_low_s32(b0.val[0]):  00 01 20 21
  // vget_high_s32(b0.val[0]): 10 11 30 31
  // vget_low_s32(b0.val[1]):  02 03 22 23
  // vget_high_s32(b0.val[1]): 12 13 32 33
  // to:
  // c0.val[0]: 00 10 20 30
  // c0.val[1]: 01 11 21 32
  // c1.val[0]: 02 12 22 32
  // c1.val[1]: 03 13 23 33

  const int16x4x2_t c0 =
      vtrn_s16(vreinterpret_s16_s32(vget_low_s32(b0.val[0])),
               vreinterpret_s16_s32(vget_high_s32(b0.val[0])));
  const int16x4x2_t c1 =
      vtrn_s16(vreinterpret_s16_s32(vget_low_s32(b0.val[1])),
               vreinterpret_s16_s32(vget_high_s32(b0.val[1])));

  out[0] = c0.val[0];
  out[1] = c1.val[1];
  out[2] = c0.val[1];
  out[3] = c1.val[0];
}

// Process 4 wht4 rows and columns.
LIBGAV1_ALWAYS_INLINE void Wht4_NEON(uint8_t* LIBGAV1_RESTRICT dst,
                                     const int dst_stride,
                                     const void* LIBGAV1_RESTRICT source,
                                     const int adjusted_tx_height) {
  const auto* const src = static_cast<const int16_t*>(source);
  int16x4_t s[4];

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
    s[0] = vdup_n_s16(h);
    s[0] = vset_lane_s16(f, s[0], 0);
    s[1] = vdup_n_s16(i);
    s[1] = vset_lane_s16(g, s[1], 0);
    s[2] = s[3] = s[1];
  } else {
    // Load the 4x4 source in transposed form.
    int16x4x4_t columns = vld4_s16(src);
    // Shift right and permute the columns for the WHT.
    s[0] = vshr_n_s16(columns.val[0], 2);
    s[2] = vshr_n_s16(columns.val[1], 2);
    s[3] = vshr_n_s16(columns.val[2], 2);
    s[1] = vshr_n_s16(columns.val[3], 2);

    // Row transforms.
    s[0] = vadd_s16(s[0], s[2]);
    s[3] = vsub_s16(s[3], s[1]);
    int16x4_t e = vhsub_s16(s[0], s[3]);  // e = (s[0] - s[3]) >> 1
    s[1] = vsub_s16(e, s[1]);
    s[2] = vsub_s16(e, s[2]);
    s[0] = vsub_s16(s[0], s[1]);
    s[3] = vadd_s16(s[3], s[2]);

    int16x8_t x[2];
    x[0] = vcombine_s16(s[0], s[1]);
    x[1] = vcombine_s16(s[2], s[3]);
    TransposeAndPermute4x4WideInput(x, s);

    // Column transforms.
    s[0] = vadd_s16(s[0], s[2]);
    s[3] = vsub_s16(s[3], s[1]);
    e = vhsub_s16(s[0], s[3]);  // e = (s[0] - s[3]) >> 1
    s[1] = vsub_s16(e, s[1]);
    s[2] = vsub_s16(e, s[2]);
    s[0] = vsub_s16(s[0], s[1]);
    s[3] = vadd_s16(s[3], s[2]);
  }

  // Store to frame.
  uint8x8_t frame_data = vdup_n_u8(0);
  for (int row = 0; row < 4; row += 2) {
    frame_data = Load4<0>(dst, frame_data);
    frame_data = Load4<1>(dst + dst_stride, frame_data);
    const int16x8_t residual = vcombine_s16(s[row], s[row + 1]);
    const uint16x8_t b = vaddw_u8(vreinterpretq_u16_s16(residual), frame_data);
    frame_data = vqmovun_s16(vreinterpretq_s16_u16(b));
    StoreLo4(dst, frame_data);
    dst += dst_stride;
    StoreHi4(dst, frame_data);
    dst += dst_stride;
  }
}

//------------------------------------------------------------------------------
// row/column transform loops

template <int tx_height>
LIBGAV1_ALWAYS_INLINE void FlipColumns(int16_t* source, int tx_width) {
  if (tx_width >= 16) {
    int i = 0;
    do {
      const int16x8_t a = vld1q_s16(&source[i]);
      const int16x8_t b = vld1q_s16(&source[i + 8]);
      const int16x8_t c = vrev64q_s16(a);
      const int16x8_t d = vrev64q_s16(b);
      vst1q_s16(&source[i], vcombine_s16(vget_high_s16(d), vget_low_s16(d)));
      vst1q_s16(&source[i + 8],
                vcombine_s16(vget_high_s16(c), vget_low_s16(c)));
      i += 16;
    } while (i < tx_width * tx_height);
  } else if (tx_width == 8) {
    for (int i = 0; i < 8 * tx_height; i += 8) {
      const int16x8_t a = vld1q_s16(&source[i]);
      const int16x8_t b = vrev64q_s16(a);
      vst1q_s16(&source[i], vcombine_s16(vget_high_s16(b), vget_low_s16(b)));
    }
  } else {
    // Process two rows per iteration.
    for (int i = 0; i < 4 * tx_height; i += 8) {
      const int16x8_t a = vld1q_s16(&source[i]);
      vst1q_s16(&source[i], vrev64q_s16(a));
    }
  }
}

template <int tx_width>
LIBGAV1_ALWAYS_INLINE void ApplyRounding(int16_t* source, int num_rows) {
  if (tx_width == 4) {
    // Process two rows per iteration.
    int i = 0;
    do {
      const int16x8_t a = vld1q_s16(&source[i]);
      const int16x8_t b = vqrdmulhq_n_s16(a, kTransformRowMultiplier << 3);
      vst1q_s16(&source[i], b);
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
        const int16x8_t a = vld1q_s16(&source[i * tx_width + j]);
        const int16x8_t b = vqrdmulhq_n_s16(a, kTransformRowMultiplier << 3);
        vst1q_s16(&source[i * tx_width + j], b);
        j += 8;
      } while (j < non_zero_width);
    } while (++i < num_rows);
  }
}

template <int tx_width>
LIBGAV1_ALWAYS_INLINE void RowShift(int16_t* source, int num_rows,
                                    int row_shift) {
  // vqrshlq_s16 will shift right if shift value is negative.
  row_shift = -row_shift;

  if (tx_width == 4) {
    // Process two rows per iteration.
    int i = 0;
    do {
      const int16x8_t residual = vld1q_s16(&source[i]);
      vst1q_s16(&source[i], vqrshlq_s16(residual, vdupq_n_s16(row_shift)));
      i += 8;
    } while (i < tx_width * num_rows);
  } else {
    int i = 0;
    do {
      for (int j = 0; j < tx_width; j += 8) {
        const int16x8_t residual = vld1q_s16(&source[i * tx_width + j]);
        const int16x8_t residual_shifted =
            vqrshlq_s16(residual, vdupq_n_s16(row_shift));
        vst1q_s16(&source[i * tx_width + j], residual_shifted);
      }
    } while (++i < num_rows);
  }
}

template <int tx_height, bool enable_flip_rows = false>
LIBGAV1_ALWAYS_INLINE void StoreToFrameWithRound(
    Array2DView<uint8_t> frame, const int start_x, const int start_y,
    const int tx_width, const int16_t* LIBGAV1_RESTRICT source,
    TransformType tx_type) {
  const bool flip_rows =
      enable_flip_rows ? kTransformFlipRowsMask.Contains(tx_type) : false;
  const int stride = frame.columns();
  uint8_t* LIBGAV1_RESTRICT dst = frame[start_y] + start_x;

  // Enable for 4x4, 4x8, 4x16
  if (tx_height < 32 && tx_width == 4) {
    uint8x8_t frame_data = vdup_n_u8(0);
    for (int i = 0; i < tx_height; ++i) {
      const int row = flip_rows ? (tx_height - i - 1) * 4 : i * 4;
      const int16x4_t residual = vld1_s16(&source[row]);
      frame_data = Load4<0>(dst, frame_data);
      const int16x4_t a = vrshr_n_s16(residual, 4);
      const uint16x8_t b =
          vaddw_u8(vreinterpretq_u16_s16(vcombine_s16(a, a)), frame_data);
      const uint8x8_t d = vqmovun_s16(vreinterpretq_s16_u16(b));
      StoreLo4(dst, d);
      dst += stride;
    }
    // Enable for 8x4, 8x8, 8x16, 8x32
  } else if (tx_height < 64 && tx_width == 8) {
    for (int i = 0; i < tx_height; ++i) {
      const int row = flip_rows ? (tx_height - i - 1) * 8 : i * 8;
      const int16x8_t residual = vld1q_s16(&source[row]);
      const uint8x8_t frame_data = vld1_u8(dst);
      const int16x8_t a = vrshrq_n_s16(residual, 4);
      const uint16x8_t b = vaddw_u8(vreinterpretq_u16_s16(a), frame_data);
      const uint8x8_t d = vqmovun_s16(vreinterpretq_s16_u16(b));
      vst1_u8(dst, d);
      dst += stride;
    }
    // Remaining widths >= 16.
  } else {
    for (int i = 0; i < tx_height; ++i) {
      const int y = start_y + i;
      const int row = flip_rows ? (tx_height - i - 1) * tx_width : i * tx_width;
      int j = 0;
      do {
        const int x = start_x + j;
        const int16x8_t residual = vld1q_s16(&source[row + j]);
        const int16x8_t residual_hi = vld1q_s16(&source[row + j + 8]);
        const uint8x16_t frame_data = vld1q_u8(frame[y] + x);
        const int16x8_t a = vrshrq_n_s16(residual, 4);
        const int16x8_t a_hi = vrshrq_n_s16(residual_hi, 4);
        const uint16x8_t b =
            vaddw_u8(vreinterpretq_u16_s16(a), vget_low_u8(frame_data));
        const uint16x8_t b_hi =
            vaddw_u8(vreinterpretq_u16_s16(a_hi), vget_high_u8(frame_data));
        vst1q_u8(frame[y] + x,
                 vcombine_u8(vqmovun_s16(vreinterpretq_s16_u16(b)),
                             vqmovun_s16(vreinterpretq_s16_u16(b_hi))));
        j += 16;
      } while (j < tx_width);
    }
  }
}

void Dct4TransformLoopRow_NEON(TransformType /*tx_type*/, TransformSize tx_size,
                               int adjusted_tx_height, void* src_buffer,
                               int /*start_x*/, int /*start_y*/,
                               void* /*dst_frame*/) {
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

  if (adjusted_tx_height == 4) {
    // Process 4 1d dct4 rows in parallel.
    Dct4_NEON<ButterflyRotation_4, false>(src, /*step=*/4, /*transpose=*/true);
  } else {
    // Process 8 1d dct4 rows in parallel per iteration.
    int i = adjusted_tx_height;
    auto* data = src;
    do {
      Dct4_NEON<ButterflyRotation_8, true>(data, /*step=*/4,
                                           /*transpose=*/true);
      data += 32;
      i -= 8;
    } while (i != 0);
  }
  if (tx_height == 16) {
    RowShift<4>(src, adjusted_tx_height, 1);
  }
}

void Dct4TransformLoopColumn_NEON(TransformType tx_type, TransformSize tx_size,
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
      Dct4_NEON<ButterflyRotation_4, false>(src, tx_width, /*transpose=*/false);
    } else {
      // Process 8 1d dct4 columns in parallel per iteration.
      int i = tx_width;
      auto* data = src;
      do {
        Dct4_NEON<ButterflyRotation_8, true>(data, tx_width,
                                             /*transpose=*/false);
        data += 8;
        i -= 8;
      } while (i != 0);
    }
  }

  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  StoreToFrameWithRound<4>(frame, start_x, start_y, tx_width, src, tx_type);
}

void Dct8TransformLoopRow_NEON(TransformType /*tx_type*/, TransformSize tx_size,
                               int adjusted_tx_height, void* src_buffer,
                               int /*start_x*/, int /*start_y*/,
                               void* /*dst_frame*/) {
  auto* src = static_cast<int16_t*>(src_buffer);
  const bool should_round = kShouldRound[tx_size];
  const uint8_t row_shift = kTransformRowShift[tx_size];

  if (DctDcOnly<8>(src, adjusted_tx_height, should_round, row_shift)) {
    return;
  }

  if (should_round) {
    ApplyRounding<8>(src, adjusted_tx_height);
  }

  if (adjusted_tx_height == 4) {
    // Process 4 1d dct8 rows in parallel.
    Dct8_NEON<ButterflyRotation_4, true>(src, /*step=*/8, /*transpose=*/true);
  } else {
    // Process 8 1d dct8 rows in parallel per iteration.
    assert(adjusted_tx_height % 8 == 0);
    int i = adjusted_tx_height;
    auto* data = src;
    do {
      Dct8_NEON<ButterflyRotation_8, false>(data, /*step=*/8,
                                            /*transpose=*/true);
      data += 64;
      i -= 8;
    } while (i != 0);
  }
  if (row_shift > 0) {
    RowShift<8>(src, adjusted_tx_height, row_shift);
  }
}

void Dct8TransformLoopColumn_NEON(TransformType tx_type, TransformSize tx_size,
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
      Dct8_NEON<ButterflyRotation_4, true>(src, 4, /*transpose=*/false);
    } else {
      // Process 8 1d dct8 columns in parallel per iteration.
      int i = tx_width;
      auto* data = src;
      do {
        Dct8_NEON<ButterflyRotation_8, false>(data, tx_width,
                                              /*transpose=*/false);
        data += 8;
        i -= 8;
      } while (i != 0);
    }
  }
  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  StoreToFrameWithRound<8>(frame, start_x, start_y, tx_width, src, tx_type);
}

void Dct16TransformLoopRow_NEON(TransformType /*tx_type*/,
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

  if (adjusted_tx_height == 4) {
    // Process 4 1d dct16 rows in parallel.
    Dct16_NEON<ButterflyRotation_4, true>(src, 16, /*is_row=*/true, row_shift);
  } else {
    assert(adjusted_tx_height % 8 == 0);
    int i = adjusted_tx_height;
    do {
      // Process 8 1d dct16 rows in parallel per iteration.
      Dct16_NEON<ButterflyRotation_8, false>(src, 16, /*is_row=*/true,
                                             row_shift);
      src += 128;
      i -= 8;
    } while (i != 0);
  }
}

void Dct16TransformLoopColumn_NEON(TransformType tx_type, TransformSize tx_size,
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
      Dct16_NEON<ButterflyRotation_4, true>(src, 4, /*is_row=*/false,
                                            /*row_shift=*/0);
    } else {
      int i = tx_width;
      auto* data = src;
      do {
        // Process 8 1d dct16 columns in parallel per iteration.
        Dct16_NEON<ButterflyRotation_8, false>(data, tx_width, /*is_row=*/false,
                                               /*row_shift=*/0);
        data += 8;
        i -= 8;
      } while (i != 0);
    }
  }
  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  StoreToFrameWithRound<16>(frame, start_x, start_y, tx_width, src, tx_type);
}

void Dct32TransformLoopRow_NEON(TransformType /*tx_type*/,
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
    Dct32_NEON(&src[i * 32], 32, /*is_row=*/true, row_shift);
    i += 8;
  } while (i < adjusted_tx_height);
}

void Dct32TransformLoopColumn_NEON(TransformType tx_type, TransformSize tx_size,
                                   int adjusted_tx_height,
                                   void* LIBGAV1_RESTRICT src_buffer,
                                   int start_x, int start_y,
                                   void* LIBGAV1_RESTRICT dst_frame) {
  auto* src = static_cast<int16_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (!DctDcOnlyColumn<32>(src, adjusted_tx_height, tx_width)) {
    // Process 8 1d dct32 columns in parallel per iteration.
    int i = tx_width;
    auto* data = src;
    do {
      Dct32_NEON(data, tx_width, /*is_row=*/false, /*row_shift=*/0);
      data += 8;
      i -= 8;
    } while (i != 0);
  }
  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  StoreToFrameWithRound<32>(frame, start_x, start_y, tx_width, src, tx_type);
}

void Dct64TransformLoopRow_NEON(TransformType /*tx_type*/,
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
    Dct64_NEON(&src[i * 64], 64, /*is_row=*/true, row_shift);
    i += 8;
  } while (i < adjusted_tx_height);
}

void Dct64TransformLoopColumn_NEON(TransformType tx_type, TransformSize tx_size,
                                   int adjusted_tx_height,
                                   void* LIBGAV1_RESTRICT src_buffer,
                                   int start_x, int start_y,
                                   void* LIBGAV1_RESTRICT dst_frame) {
  auto* src = static_cast<int16_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (!DctDcOnlyColumn<64>(src, adjusted_tx_height, tx_width)) {
    // Process 8 1d dct64 columns in parallel per iteration.
    int i = tx_width;
    auto* data = src;
    do {
      Dct64_NEON(data, tx_width, /*is_row=*/false, /*row_shift=*/0);
      data += 8;
      i -= 8;
    } while (i != 0);
  }
  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  StoreToFrameWithRound<64>(frame, start_x, start_y, tx_width, src, tx_type);
}

void Adst4TransformLoopRow_NEON(TransformType /*tx_type*/,
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
  int i = adjusted_tx_height;
  auto* data = src;
  do {
    Adst4_NEON(data, /*step=*/4, /*transpose=*/true);
    data += 16;
    i -= 4;
  } while (i != 0);

  if (tx_height == 16) {
    RowShift<4>(src, adjusted_tx_height, 1);
  }
}

void Adst4TransformLoopColumn_NEON(TransformType tx_type, TransformSize tx_size,
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
    int i = tx_width;
    auto* data = src;
    do {
      Adst4_NEON(data, tx_width, /*transpose=*/false);
      data += 4;
      i -= 4;
    } while (i != 0);
  }

  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  StoreToFrameWithRound<4, /*enable_flip_rows=*/true>(frame, start_x, start_y,
                                                      tx_width, src, tx_type);
}

void Adst8TransformLoopRow_NEON(TransformType /*tx_type*/,
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

  if (adjusted_tx_height == 4) {
    // Process 4 1d adst8 rows in parallel.
    Adst8_NEON<ButterflyRotation_4, true>(src, /*step=*/8, /*transpose=*/true);
  } else {
    // Process 8 1d adst8 rows in parallel per iteration.
    assert(adjusted_tx_height % 8 == 0);
    int i = adjusted_tx_height;
    auto* data = src;
    do {
      Adst8_NEON<ButterflyRotation_8, false>(data, /*step=*/8,
                                             /*transpose=*/true);
      data += 64;
      i -= 8;
    } while (i != 0);
  }
  if (row_shift > 0) {
    RowShift<8>(src, adjusted_tx_height, row_shift);
  }
}

void Adst8TransformLoopColumn_NEON(TransformType tx_type, TransformSize tx_size,
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
      Adst8_NEON<ButterflyRotation_4, true>(src, 4, /*transpose=*/false);
    } else {
      // Process 8 1d adst8 columns in parallel per iteration.
      int i = tx_width;
      auto* data = src;
      do {
        Adst8_NEON<ButterflyRotation_8, false>(data, tx_width,
                                               /*transpose=*/false);
        data += 8;
        i -= 8;
      } while (i != 0);
    }
  }
  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  StoreToFrameWithRound<8, /*enable_flip_rows=*/true>(frame, start_x, start_y,
                                                      tx_width, src, tx_type);
}

void Adst16TransformLoopRow_NEON(TransformType /*tx_type*/,
                                 TransformSize tx_size, int adjusted_tx_height,
                                 void* src_buffer, int /*start_x*/,
                                 int /*start_y*/, void* /*dst_frame*/) {
  auto* src = static_cast<int16_t*>(src_buffer);
  const bool should_round = kShouldRound[tx_size];
  const uint8_t row_shift = kTransformRowShift[tx_size];

  if (Adst16DcOnly(src, adjusted_tx_height, should_round, row_shift)) {
    return;
  }

  if (should_round) {
    ApplyRounding<16>(src, adjusted_tx_height);
  }

  if (adjusted_tx_height == 4) {
    // Process 4 1d adst16 rows in parallel.
    Adst16_NEON<ButterflyRotation_4, true>(src, 16, /*is_row=*/true, row_shift);
  } else {
    assert(adjusted_tx_height % 8 == 0);
    int i = adjusted_tx_height;
    do {
      // Process 8 1d adst16 rows in parallel per iteration.
      Adst16_NEON<ButterflyRotation_8, false>(src, 16, /*is_row=*/true,
                                              row_shift);
      src += 128;
      i -= 8;
    } while (i != 0);
  }
}

void Adst16TransformLoopColumn_NEON(TransformType tx_type,
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

  if (!Adst16DcOnlyColumn(src, adjusted_tx_height, tx_width)) {
    if (tx_width == 4) {
      // Process 4 1d adst16 columns in parallel.
      Adst16_NEON<ButterflyRotation_4, true>(src, 4, /*is_row=*/false,
                                             /*row_shift=*/0);
    } else {
      int i = tx_width;
      auto* data = src;
      do {
        // Process 8 1d adst16 columns in parallel per iteration.
        Adst16_NEON<ButterflyRotation_8, false>(
            data, tx_width, /*is_row=*/false, /*row_shift=*/0);
        data += 8;
        i -= 8;
      } while (i != 0);
    }
  }
  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
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
    int i = adjusted_tx_height;
    do {
      Identity4_NEON<false>(src, /*step=*/4);
      src += 16;
      i -= 4;
    } while (i != 0);
  } else {
    int i = adjusted_tx_height;
    do {
      Identity4_NEON<true>(src, /*step=*/4);
      src += 16;
      i -= 4;
    } while (i != 0);
  }
}

void Identity4TransformLoopColumn_NEON(TransformType tx_type,
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
  auto* src = static_cast<int16_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (kTransformFlipColumnsMask.Contains(tx_type)) {
    FlipColumns<8>(src, tx_width);
  }
  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  IdentityColumnStoreToFrame<8>(frame, start_x, start_y, tx_width,
                                adjusted_tx_height, src);
}

void Identity16TransformLoopRow_NEON(TransformType /*tx_type*/,
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
  int i = adjusted_tx_height;
  do {
    Identity16Row_NEON(src, /*step=*/16, kTransformRowShift[tx_size]);
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
  auto* src = static_cast<int16_t*>(src_buffer);
  const int tx_width = kTransformWidth[tx_size];

  if (kTransformFlipColumnsMask.Contains(tx_type)) {
    FlipColumns<16>(src, tx_width);
  }
  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
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

  // Process kTransformSize32x16.  The src is always rounded before the
  // identity transform and shifted by 1 afterwards.
  auto* src = static_cast<int16_t*>(src_buffer);
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
  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  auto* src = static_cast<int16_t*>(src_buffer);
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
  const auto* src = static_cast<int16_t*>(src_buffer);
  auto& frame = *static_cast<Array2DView<uint8_t>*>(dst_frame);
  uint8_t* dst = frame[start_y] + start_x;
  const int dst_stride = frame.columns();
  Wht4_NEON(dst, dst_stride, src, adjusted_tx_height);
}

//------------------------------------------------------------------------------

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
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
}  // namespace low_bitdepth

void InverseTransformInit_NEON() { low_bitdepth::Init8bpp(); }

}  // namespace dsp
}  // namespace libgav1
#else   // !LIBGAV1_ENABLE_NEON
namespace libgav1 {
namespace dsp {

void InverseTransformInit_NEON() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_ENABLE_NEON
