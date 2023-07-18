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

#include "src/dsp/distance_weighted_blend.h"
#include "src/utils/cpu.h"

#if LIBGAV1_ENABLE_NEON

#include <arm_neon.h>

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "src/dsp/arm/common_neon.h"
#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/utils/common.h"

namespace libgav1 {
namespace dsp {

constexpr int kInterPostRoundBit = 4;

namespace low_bitdepth {
namespace {

inline uint8x8_t ComputeWeightedAverage8(const int16x8_t pred0,
                                         const int16x8_t pred1,
                                         const int16x8_t weight) {
  // Given: p0,p1 in range [-5132,9212] and w0 = 16 - w1, w1 = 16 - w0
  // Output: (p0 * w0 + p1 * w1 + 128(=rounding bit)) >>
  //    8(=kInterPostRoundBit + 4)
  // The formula is manipulated to avoid lengthening to 32 bits.
  // p0 * w0 + p1 * w1 = p0 * w0 + (16 - w0) * p1
  // = (p0 - p1) * w0 + 16 * p1
  // Maximum value of p0 - p1 is 9212 + 5132 = 0x3808.
  const int16x8_t diff = vsubq_s16(pred0, pred1);
  // (((p0 - p1) * (w0 << 11) << 1) >> 16) + ((16 * p1) >> 4)
  const int16x8_t weighted_diff = vqdmulhq_s16(diff, weight);
  // ((p0 - p1) * w0 >> 4) + p1
  const int16x8_t upscaled_average = vaddq_s16(weighted_diff, pred1);
  // (((p0 - p1) * w0 >> 4) + p1 + (128 >> 4)) >> 4
  return vqrshrun_n_s16(upscaled_average, kInterPostRoundBit);
}

template <int width>
inline void DistanceWeightedBlendSmall_NEON(
    const int16_t* LIBGAV1_RESTRICT prediction_0,
    const int16_t* LIBGAV1_RESTRICT prediction_1, const int height,
    const int16x8_t weight, void* LIBGAV1_RESTRICT const dest,
    const ptrdiff_t dest_stride) {
  auto* dst = static_cast<uint8_t*>(dest);
  constexpr int step = 16 / width;

  int y = height;
  do {
    const int16x8_t src_00 = vld1q_s16(prediction_0);
    const int16x8_t src_10 = vld1q_s16(prediction_1);
    prediction_0 += 8;
    prediction_1 += 8;
    const uint8x8_t result0 = ComputeWeightedAverage8(src_00, src_10, weight);

    const int16x8_t src_01 = vld1q_s16(prediction_0);
    const int16x8_t src_11 = vld1q_s16(prediction_1);
    prediction_0 += 8;
    prediction_1 += 8;
    const uint8x8_t result1 = ComputeWeightedAverage8(src_01, src_11, weight);

    if (width == 4) {
      StoreLo4(dst, result0);
      dst += dest_stride;
      StoreHi4(dst, result0);
      dst += dest_stride;
      StoreLo4(dst, result1);
      dst += dest_stride;
      StoreHi4(dst, result1);
      dst += dest_stride;
    } else {
      assert(width == 8);
      vst1_u8(dst, result0);
      dst += dest_stride;
      vst1_u8(dst, result1);
      dst += dest_stride;
    }
    y -= step;
  } while (y != 0);
}

inline void DistanceWeightedBlendLarge_NEON(
    const int16_t* LIBGAV1_RESTRICT prediction_0,
    const int16_t* LIBGAV1_RESTRICT prediction_1, const int16x8_t weight,
    const int width, const int height, void* LIBGAV1_RESTRICT const dest,
    const ptrdiff_t dest_stride) {
  auto* dst = static_cast<uint8_t*>(dest);

  int y = height;
  do {
    int x = 0;
    do {
      const int16x8_t src0_lo = vld1q_s16(prediction_0 + x);
      const int16x8_t src1_lo = vld1q_s16(prediction_1 + x);
      const uint8x8_t res_lo =
          ComputeWeightedAverage8(src0_lo, src1_lo, weight);

      const int16x8_t src0_hi = vld1q_s16(prediction_0 + x + 8);
      const int16x8_t src1_hi = vld1q_s16(prediction_1 + x + 8);
      const uint8x8_t res_hi =
          ComputeWeightedAverage8(src0_hi, src1_hi, weight);

      const uint8x16_t result = vcombine_u8(res_lo, res_hi);
      vst1q_u8(dst + x, result);
      x += 16;
    } while (x < width);
    dst += dest_stride;
    prediction_0 += width;
    prediction_1 += width;
  } while (--y != 0);
}

inline void DistanceWeightedBlend_NEON(
    const void* LIBGAV1_RESTRICT prediction_0,
    const void* LIBGAV1_RESTRICT prediction_1, const uint8_t weight_0,
    const uint8_t /*weight_1*/, const int width, const int height,
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t dest_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  // Upscale the weight for vqdmulh.
  const int16x8_t weight = vdupq_n_s16(weight_0 << 11);
  if (width == 4) {
    DistanceWeightedBlendSmall_NEON<4>(pred_0, pred_1, height, weight, dest,
                                       dest_stride);
    return;
  }

  if (width == 8) {
    DistanceWeightedBlendSmall_NEON<8>(pred_0, pred_1, height, weight, dest,
                                       dest_stride);
    return;
  }

  DistanceWeightedBlendLarge_NEON(pred_0, pred_1, weight, width, height, dest,
                                  dest_stride);
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
  dsp->distance_weighted_blend = DistanceWeightedBlend_NEON;
}

}  // namespace
}  // namespace low_bitdepth

//------------------------------------------------------------------------------
#if LIBGAV1_MAX_BITDEPTH >= 10
namespace high_bitdepth {
namespace {

inline uint16x4x2_t ComputeWeightedAverage8(const uint16x4x2_t pred0,
                                            const uint16x4x2_t pred1,
                                            const uint16x4_t weights[2]) {
  const uint32x4_t wpred0_lo = vmull_u16(weights[0], pred0.val[0]);
  const uint32x4_t wpred0_hi = vmull_u16(weights[0], pred0.val[1]);
  const uint32x4_t blended_lo = vmlal_u16(wpred0_lo, weights[1], pred1.val[0]);
  const uint32x4_t blended_hi = vmlal_u16(wpred0_hi, weights[1], pred1.val[1]);
  const int32x4_t offset = vdupq_n_s32(kCompoundOffset * 16);
  const int32x4_t res_lo = vsubq_s32(vreinterpretq_s32_u32(blended_lo), offset);
  const int32x4_t res_hi = vsubq_s32(vreinterpretq_s32_u32(blended_hi), offset);
  const uint16x4_t bd_max = vdup_n_u16((1 << kBitdepth10) - 1);
  // Clip the result at (1 << bd) - 1.
  uint16x4x2_t result;
  result.val[0] =
      vmin_u16(vqrshrun_n_s32(res_lo, kInterPostRoundBit + 4), bd_max);
  result.val[1] =
      vmin_u16(vqrshrun_n_s32(res_hi, kInterPostRoundBit + 4), bd_max);
  return result;
}

inline uint16x4x4_t ComputeWeightedAverage8(const uint16x4x4_t pred0,
                                            const uint16x4x4_t pred1,
                                            const uint16x4_t weights[2]) {
  const int32x4_t offset = vdupq_n_s32(kCompoundOffset * 16);
  const uint32x4_t wpred0 = vmull_u16(weights[0], pred0.val[0]);
  const uint32x4_t wpred1 = vmull_u16(weights[0], pred0.val[1]);
  const uint32x4_t blended0 = vmlal_u16(wpred0, weights[1], pred1.val[0]);
  const uint32x4_t blended1 = vmlal_u16(wpred1, weights[1], pred1.val[1]);
  const int32x4_t res0 = vsubq_s32(vreinterpretq_s32_u32(blended0), offset);
  const int32x4_t res1 = vsubq_s32(vreinterpretq_s32_u32(blended1), offset);
  const uint32x4_t wpred2 = vmull_u16(weights[0], pred0.val[2]);
  const uint32x4_t wpred3 = vmull_u16(weights[0], pred0.val[3]);
  const uint32x4_t blended2 = vmlal_u16(wpred2, weights[1], pred1.val[2]);
  const uint32x4_t blended3 = vmlal_u16(wpred3, weights[1], pred1.val[3]);
  const int32x4_t res2 = vsubq_s32(vreinterpretq_s32_u32(blended2), offset);
  const int32x4_t res3 = vsubq_s32(vreinterpretq_s32_u32(blended3), offset);
  const uint16x4_t bd_max = vdup_n_u16((1 << kBitdepth10) - 1);
  // Clip the result at (1 << bd) - 1.
  uint16x4x4_t result;
  result.val[0] =
      vmin_u16(vqrshrun_n_s32(res0, kInterPostRoundBit + 4), bd_max);
  result.val[1] =
      vmin_u16(vqrshrun_n_s32(res1, kInterPostRoundBit + 4), bd_max);
  result.val[2] =
      vmin_u16(vqrshrun_n_s32(res2, kInterPostRoundBit + 4), bd_max);
  result.val[3] =
      vmin_u16(vqrshrun_n_s32(res3, kInterPostRoundBit + 4), bd_max);

  return result;
}

// We could use vld1_u16_x2, but for compatibility reasons, use this function
// instead. The compiler optimizes to the correct instruction.
inline uint16x4x2_t LoadU16x4_x2(uint16_t const* ptr) {
  uint16x4x2_t x;
  // gcc/clang (64 bit) optimizes the following to ldp.
  x.val[0] = vld1_u16(ptr);
  x.val[1] = vld1_u16(ptr + 4);
  return x;
}

// We could use vld1_u16_x4, but for compatibility reasons, use this function
// instead. The compiler optimizes to a pair of vld1_u16_x2, which showed better
// performance in the speed tests.
inline uint16x4x4_t LoadU16x4_x4(uint16_t const* ptr) {
  uint16x4x4_t x;
  x.val[0] = vld1_u16(ptr);
  x.val[1] = vld1_u16(ptr + 4);
  x.val[2] = vld1_u16(ptr + 8);
  x.val[3] = vld1_u16(ptr + 12);
  return x;
}

void DistanceWeightedBlend_NEON(const void* LIBGAV1_RESTRICT prediction_0,
                                const void* LIBGAV1_RESTRICT prediction_1,
                                const uint8_t weight_0, const uint8_t weight_1,
                                const int width, const int height,
                                void* LIBGAV1_RESTRICT const dest,
                                const ptrdiff_t dest_stride) {
  const auto* pred_0 = static_cast<const uint16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const uint16_t*>(prediction_1);
  auto* dst = static_cast<uint16_t*>(dest);
  const ptrdiff_t dst_stride = dest_stride / sizeof(dst[0]);
  const uint16x4_t weights[2] = {vdup_n_u16(weight_0), vdup_n_u16(weight_1)};

  if (width == 4) {
    int y = height;
    do {
      const uint16x4x2_t src0 = LoadU16x4_x2(pred_0);
      const uint16x4x2_t src1 = LoadU16x4_x2(pred_1);
      const uint16x4x2_t res = ComputeWeightedAverage8(src0, src1, weights);
      vst1_u16(dst, res.val[0]);
      vst1_u16(dst + dst_stride, res.val[1]);
      dst += dst_stride << 1;
      pred_0 += 8;
      pred_1 += 8;
      y -= 2;
    } while (y != 0);
  } else if (width == 8) {
    int y = height;
    do {
      const uint16x4x4_t src0 = LoadU16x4_x4(pred_0);
      const uint16x4x4_t src1 = LoadU16x4_x4(pred_1);
      const uint16x4x4_t res = ComputeWeightedAverage8(src0, src1, weights);
      vst1_u16(dst, res.val[0]);
      vst1_u16(dst + 4, res.val[1]);
      vst1_u16(dst + dst_stride, res.val[2]);
      vst1_u16(dst + dst_stride + 4, res.val[3]);
      dst += dst_stride << 1;
      pred_0 += 16;
      pred_1 += 16;
      y -= 2;
    } while (y != 0);
  } else {
    int y = height;
    do {
      int x = 0;
      do {
        const uint16x4x4_t src0 = LoadU16x4_x4(pred_0 + x);
        const uint16x4x4_t src1 = LoadU16x4_x4(pred_1 + x);
        const uint16x4x4_t res = ComputeWeightedAverage8(src0, src1, weights);
        vst1_u16(dst + x, res.val[0]);
        vst1_u16(dst + x + 4, res.val[1]);
        vst1_u16(dst + x + 8, res.val[2]);
        vst1_u16(dst + x + 12, res.val[3]);
        x += 16;
      } while (x < width);
      dst += dst_stride;
      pred_0 += width;
      pred_1 += width;
    } while (--y != 0);
  }
}

void Init10bpp() {
  Dsp* dsp = dsp_internal::GetWritableDspTable(kBitdepth10);
  assert(dsp != nullptr);
  dsp->distance_weighted_blend = DistanceWeightedBlend_NEON;
}

}  // namespace
}  // namespace high_bitdepth
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

void DistanceWeightedBlendInit_NEON() {
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

void DistanceWeightedBlendInit_NEON() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_ENABLE_NEON
