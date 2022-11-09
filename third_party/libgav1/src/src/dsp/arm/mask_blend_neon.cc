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

#include "src/dsp/mask_blend.h"
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
namespace low_bitdepth {
namespace {

template <int subsampling_y>
inline uint8x8_t GetMask4x2(const uint8_t* mask) {
  if (subsampling_y == 1) {
    const uint8x16x2_t mask_val = vld2q_u8(mask);
    const uint8x16_t combined_horz = vaddq_u8(mask_val.val[0], mask_val.val[1]);
    const uint32x2_t row_01 = vreinterpret_u32_u8(vget_low_u8(combined_horz));
    const uint32x2_t row_23 = vreinterpret_u32_u8(vget_high_u8(combined_horz));

    const uint32x2x2_t row_02_13 = vtrn_u32(row_01, row_23);
    // Use a halving add to work around the case where all |mask| values are 64.
    return vrshr_n_u8(vhadd_u8(vreinterpret_u8_u32(row_02_13.val[0]),
                               vreinterpret_u8_u32(row_02_13.val[1])),
                      1);
  }
  // subsampling_x == 1
  const uint8x8x2_t mask_val = vld2_u8(mask);
  return vrhadd_u8(mask_val.val[0], mask_val.val[1]);
}

template <int subsampling_x, int subsampling_y>
inline uint8x8_t GetMask8(const uint8_t* mask) {
  if (subsampling_x == 1 && subsampling_y == 1) {
    const uint8x16x2_t mask_val = vld2q_u8(mask);
    const uint8x16_t combined_horz = vaddq_u8(mask_val.val[0], mask_val.val[1]);
    // Use a halving add to work around the case where all |mask| values are 64.
    return vrshr_n_u8(
        vhadd_u8(vget_low_u8(combined_horz), vget_high_u8(combined_horz)), 1);
  }
  if (subsampling_x == 1) {
    const uint8x8x2_t mask_val = vld2_u8(mask);
    return vrhadd_u8(mask_val.val[0], mask_val.val[1]);
  }
  assert(subsampling_y == 0 && subsampling_x == 0);
  return vld1_u8(mask);
}

inline void WriteMaskBlendLine4x2(const int16_t* LIBGAV1_RESTRICT const pred_0,
                                  const int16_t* LIBGAV1_RESTRICT const pred_1,
                                  const int16x8_t pred_mask_0,
                                  const int16x8_t pred_mask_1,
                                  uint8_t* LIBGAV1_RESTRICT dst,
                                  const ptrdiff_t dst_stride) {
  const int16x8_t pred_val_0 = vld1q_s16(pred_0);
  const int16x8_t pred_val_1 = vld1q_s16(pred_1);
  // int res = (mask_value * prediction_0[x] +
  //      (64 - mask_value) * prediction_1[x]) >> 6;
  const int32x4_t weighted_pred_0_lo =
      vmull_s16(vget_low_s16(pred_mask_0), vget_low_s16(pred_val_0));
  const int32x4_t weighted_pred_0_hi =
      vmull_s16(vget_high_s16(pred_mask_0), vget_high_s16(pred_val_0));
  const int32x4_t weighted_combo_lo = vmlal_s16(
      weighted_pred_0_lo, vget_low_s16(pred_mask_1), vget_low_s16(pred_val_1));
  const int32x4_t weighted_combo_hi =
      vmlal_s16(weighted_pred_0_hi, vget_high_s16(pred_mask_1),
                vget_high_s16(pred_val_1));
  // dst[x] = static_cast<Pixel>(
  //     Clip3(RightShiftWithRounding(res, inter_post_round_bits), 0,
  //         (1 << kBitdepth8) - 1));
  const uint8x8_t result =
      vqrshrun_n_s16(vcombine_s16(vshrn_n_s32(weighted_combo_lo, 6),
                                  vshrn_n_s32(weighted_combo_hi, 6)),
                     4);
  StoreLo4(dst, result);
  StoreHi4(dst + dst_stride, result);
}

template <int subsampling_y>
inline void MaskBlending4x4_NEON(const int16_t* LIBGAV1_RESTRICT pred_0,
                                 const int16_t* LIBGAV1_RESTRICT pred_1,
                                 const uint8_t* LIBGAV1_RESTRICT mask,
                                 uint8_t* LIBGAV1_RESTRICT dst,
                                 const ptrdiff_t dst_stride) {
  constexpr int subsampling_x = 1;
  constexpr ptrdiff_t mask_stride = 4 << subsampling_x;
  const int16x8_t mask_inverter = vdupq_n_s16(64);
  // Compound predictors use int16_t values and need to multiply long because
  // the Convolve range * 64 is 20 bits. Unfortunately there is no multiply
  // int16_t by int8_t and accumulate into int32_t instruction.
  int16x8_t pred_mask_0 = ZeroExtend(GetMask4x2<subsampling_y>(mask));
  int16x8_t pred_mask_1 = vsubq_s16(mask_inverter, pred_mask_0);
  WriteMaskBlendLine4x2(pred_0, pred_1, pred_mask_0, pred_mask_1, dst,
                        dst_stride);
  pred_0 += 4 << subsampling_x;
  pred_1 += 4 << subsampling_x;
  mask += mask_stride << (subsampling_x + subsampling_y);
  dst += dst_stride << subsampling_x;

  pred_mask_0 = ZeroExtend(GetMask4x2<subsampling_y>(mask));
  pred_mask_1 = vsubq_s16(mask_inverter, pred_mask_0);
  WriteMaskBlendLine4x2(pred_0, pred_1, pred_mask_0, pred_mask_1, dst,
                        dst_stride);
}

template <int subsampling_y>
inline void MaskBlending4xH_NEON(const int16_t* LIBGAV1_RESTRICT pred_0,
                                 const int16_t* LIBGAV1_RESTRICT pred_1,
                                 const uint8_t* LIBGAV1_RESTRICT const mask_ptr,
                                 const int height,
                                 uint8_t* LIBGAV1_RESTRICT dst,
                                 const ptrdiff_t dst_stride) {
  const uint8_t* mask = mask_ptr;
  if (height == 4) {
    MaskBlending4x4_NEON<subsampling_y>(pred_0, pred_1, mask, dst, dst_stride);
    return;
  }
  constexpr int subsampling_x = 1;
  constexpr ptrdiff_t mask_stride = 4 << subsampling_x;
  const int16x8_t mask_inverter = vdupq_n_s16(64);
  int y = 0;
  do {
    int16x8_t pred_mask_0 =
        vreinterpretq_s16_u16(vmovl_u8(GetMask4x2<subsampling_y>(mask)));
    int16x8_t pred_mask_1 = vsubq_s16(mask_inverter, pred_mask_0);

    WriteMaskBlendLine4x2(pred_0, pred_1, pred_mask_0, pred_mask_1, dst,
                          dst_stride);
    pred_0 += 4 << subsampling_x;
    pred_1 += 4 << subsampling_x;
    mask += mask_stride << (subsampling_x + subsampling_y);
    dst += dst_stride << subsampling_x;

    pred_mask_0 = ZeroExtend(GetMask4x2<subsampling_y>(mask));
    pred_mask_1 = vsubq_s16(mask_inverter, pred_mask_0);
    WriteMaskBlendLine4x2(pred_0, pred_1, pred_mask_0, pred_mask_1, dst,
                          dst_stride);
    pred_0 += 4 << subsampling_x;
    pred_1 += 4 << subsampling_x;
    mask += mask_stride << (subsampling_x + subsampling_y);
    dst += dst_stride << subsampling_x;

    pred_mask_0 = ZeroExtend(GetMask4x2<subsampling_y>(mask));
    pred_mask_1 = vsubq_s16(mask_inverter, pred_mask_0);
    WriteMaskBlendLine4x2(pred_0, pred_1, pred_mask_0, pred_mask_1, dst,
                          dst_stride);
    pred_0 += 4 << subsampling_x;
    pred_1 += 4 << subsampling_x;
    mask += mask_stride << (subsampling_x + subsampling_y);
    dst += dst_stride << subsampling_x;

    pred_mask_0 = ZeroExtend(GetMask4x2<subsampling_y>(mask));
    pred_mask_1 = vsubq_s16(mask_inverter, pred_mask_0);
    WriteMaskBlendLine4x2(pred_0, pred_1, pred_mask_0, pred_mask_1, dst,
                          dst_stride);
    pred_0 += 4 << subsampling_x;
    pred_1 += 4 << subsampling_x;
    mask += mask_stride << (subsampling_x + subsampling_y);
    dst += dst_stride << subsampling_x;
    y += 8;
  } while (y < height);
}

inline uint8x8_t CombinePred8(const int16_t* LIBGAV1_RESTRICT pred_0,
                              const int16_t* LIBGAV1_RESTRICT pred_1,
                              const int16x8_t pred_mask_0,
                              const int16x8_t pred_mask_1) {
  // First 8 values.
  const int16x8_t pred_val_0 = vld1q_s16(pred_0);
  const int16x8_t pred_val_1 = vld1q_s16(pred_1);
  // int res = (mask_value * prediction_0[x] +
  //      (64 - mask_value) * prediction_1[x]) >> 6;
  const int32x4_t weighted_pred_lo =
      vmull_s16(vget_low_s16(pred_mask_0), vget_low_s16(pred_val_0));
  const int32x4_t weighted_pred_hi =
      vmull_s16(vget_high_s16(pred_mask_0), vget_high_s16(pred_val_0));
  const int32x4_t weighted_combo_lo = vmlal_s16(
      weighted_pred_lo, vget_low_s16(pred_mask_1), vget_low_s16(pred_val_1));
  const int32x4_t weighted_combo_hi = vmlal_s16(
      weighted_pred_hi, vget_high_s16(pred_mask_1), vget_high_s16(pred_val_1));

  // dst[x] = static_cast<Pixel>(
  //     Clip3(RightShiftWithRounding(res, inter_post_round_bits), 0,
  //           (1 << kBitdepth8) - 1));
  return vqrshrun_n_s16(vcombine_s16(vshrn_n_s32(weighted_combo_lo, 6),
                                     vshrn_n_s32(weighted_combo_hi, 6)),
                        4);
}

template <int subsampling_x, int subsampling_y>
inline void MaskBlending8xH_NEON(const int16_t* LIBGAV1_RESTRICT pred_0,
                                 const int16_t* LIBGAV1_RESTRICT pred_1,
                                 const uint8_t* LIBGAV1_RESTRICT const mask_ptr,
                                 const int height,
                                 uint8_t* LIBGAV1_RESTRICT dst,
                                 const ptrdiff_t dst_stride) {
  const uint8_t* mask = mask_ptr;
  const int16x8_t mask_inverter = vdupq_n_s16(64);
  int y = height;
  do {
    const int16x8_t pred_mask_0 =
        ZeroExtend(GetMask8<subsampling_x, subsampling_y>(mask));
    // 64 - mask
    const int16x8_t pred_mask_1 = vsubq_s16(mask_inverter, pred_mask_0);
    const uint8x8_t result =
        CombinePred8(pred_0, pred_1, pred_mask_0, pred_mask_1);
    vst1_u8(dst, result);
    dst += dst_stride;
    mask += 8 << (subsampling_x + subsampling_y);
    pred_0 += 8;
    pred_1 += 8;
  } while (--y != 0);
}

template <int subsampling_x, int subsampling_y>
inline uint8x16_t GetMask16(const uint8_t* mask, const ptrdiff_t mask_stride) {
  if (subsampling_x == 1 && subsampling_y == 1) {
    const uint8x16x2_t mask_val0 = vld2q_u8(mask);
    const uint8x16x2_t mask_val1 = vld2q_u8(mask + mask_stride);
    const uint8x16_t combined_horz0 =
        vaddq_u8(mask_val0.val[0], mask_val0.val[1]);
    const uint8x16_t combined_horz1 =
        vaddq_u8(mask_val1.val[0], mask_val1.val[1]);
    // Use a halving add to work around the case where all |mask| values are 64.
    return vrshrq_n_u8(vhaddq_u8(combined_horz0, combined_horz1), 1);
  }
  if (subsampling_x == 1) {
    const uint8x16x2_t mask_val = vld2q_u8(mask);
    return vrhaddq_u8(mask_val.val[0], mask_val.val[1]);
  }
  assert(subsampling_y == 0 && subsampling_x == 0);
  return vld1q_u8(mask);
}

template <int subsampling_x, int subsampling_y>
inline void MaskBlend_NEON(const void* LIBGAV1_RESTRICT prediction_0,
                           const void* LIBGAV1_RESTRICT prediction_1,
                           const ptrdiff_t /*prediction_stride_1*/,
                           const uint8_t* LIBGAV1_RESTRICT const mask_ptr,
                           const ptrdiff_t mask_stride, const int width,
                           const int height, void* LIBGAV1_RESTRICT dest,
                           const ptrdiff_t dst_stride) {
  auto* dst = static_cast<uint8_t*>(dest);
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  if (width == 4) {
    MaskBlending4xH_NEON<subsampling_y>(pred_0, pred_1, mask_ptr, height, dst,
                                        dst_stride);
    return;
  }
  if (width == 8) {
    MaskBlending8xH_NEON<subsampling_x, subsampling_y>(pred_0, pred_1, mask_ptr,
                                                       height, dst, dst_stride);
    return;
  }
  const uint8_t* mask = mask_ptr;
  const int16x8_t mask_inverter = vdupq_n_s16(64);
  int y = 0;
  do {
    int x = 0;
    do {
      const uint8x16_t pred_mask_0 = GetMask16<subsampling_x, subsampling_y>(
          mask + (x << subsampling_x), mask_stride);
      const int16x8_t pred_mask_0_lo = ZeroExtend(vget_low_u8(pred_mask_0));
      const int16x8_t pred_mask_0_hi = ZeroExtend(vget_high_u8(pred_mask_0));
      // 64 - mask
      const int16x8_t pred_mask_1_lo = vsubq_s16(mask_inverter, pred_mask_0_lo);
      const int16x8_t pred_mask_1_hi = vsubq_s16(mask_inverter, pred_mask_0_hi);

      uint8x8_t result;
      result =
          CombinePred8(pred_0 + x, pred_1 + x, pred_mask_0_lo, pred_mask_1_lo);
      vst1_u8(dst + x, result);

      result = CombinePred8(pred_0 + x + 8, pred_1 + x + 8, pred_mask_0_hi,
                            pred_mask_1_hi);
      vst1_u8(dst + x + 8, result);

      x += 16;
    } while (x < width);
    dst += dst_stride;
    pred_0 += width;
    pred_1 += width;
    mask += mask_stride << subsampling_y;
  } while (++y < height);
}

template <int subsampling_x, int subsampling_y>
inline uint8x8_t GetInterIntraMask4x2(const uint8_t* mask,
                                      ptrdiff_t mask_stride) {
  if (subsampling_x == 1) {
    return GetMask4x2<subsampling_y>(mask);
  }
  // When using intra or difference weighted masks, the function doesn't use
  // subsampling, so |mask_stride| may be 4 or 8.
  assert(subsampling_y == 0 && subsampling_x == 0);
  const uint8x8_t mask_val0 = Load4(mask);
  return Load4<1>(mask + mask_stride, mask_val0);
}

inline void InterIntraWriteMaskBlendLine8bpp4x2(
    const uint8_t* LIBGAV1_RESTRICT const pred_0,
    uint8_t* LIBGAV1_RESTRICT const pred_1, const ptrdiff_t pred_stride_1,
    const uint8x8_t pred_mask_0, const uint8x8_t pred_mask_1) {
  const uint8x8_t pred_val_0 = vld1_u8(pred_0);
  uint8x8_t pred_val_1 = Load4(pred_1);
  pred_val_1 = Load4<1>(pred_1 + pred_stride_1, pred_val_1);

  const uint16x8_t weighted_pred_0 = vmull_u8(pred_mask_0, pred_val_0);
  const uint16x8_t weighted_combo =
      vmlal_u8(weighted_pred_0, pred_mask_1, pred_val_1);
  const uint8x8_t result = vrshrn_n_u16(weighted_combo, 6);
  StoreLo4(pred_1, result);
  StoreHi4(pred_1 + pred_stride_1, result);
}

template <int subsampling_x, int subsampling_y>
inline void InterIntraMaskBlending8bpp4x4_NEON(
    const uint8_t* LIBGAV1_RESTRICT pred_0, uint8_t* LIBGAV1_RESTRICT pred_1,
    const ptrdiff_t pred_stride_1, const uint8_t* LIBGAV1_RESTRICT mask,
    const ptrdiff_t mask_stride) {
  const uint8x8_t mask_inverter = vdup_n_u8(64);
  uint8x8_t pred_mask_1 =
      GetInterIntraMask4x2<subsampling_x, subsampling_y>(mask, mask_stride);
  uint8x8_t pred_mask_0 = vsub_u8(mask_inverter, pred_mask_1);
  InterIntraWriteMaskBlendLine8bpp4x2(pred_0, pred_1, pred_stride_1,
                                      pred_mask_0, pred_mask_1);
  pred_0 += 4 << 1;
  pred_1 += pred_stride_1 << 1;
  mask += mask_stride << (1 + subsampling_y);

  pred_mask_1 =
      GetInterIntraMask4x2<subsampling_x, subsampling_y>(mask, mask_stride);
  pred_mask_0 = vsub_u8(mask_inverter, pred_mask_1);
  InterIntraWriteMaskBlendLine8bpp4x2(pred_0, pred_1, pred_stride_1,
                                      pred_mask_0, pred_mask_1);
}

template <int subsampling_x, int subsampling_y>
inline void InterIntraMaskBlending8bpp4xH_NEON(
    const uint8_t* LIBGAV1_RESTRICT pred_0, uint8_t* LIBGAV1_RESTRICT pred_1,
    const ptrdiff_t pred_stride_1, const uint8_t* LIBGAV1_RESTRICT mask,
    const ptrdiff_t mask_stride, const int height) {
  if (height == 4) {
    InterIntraMaskBlending8bpp4x4_NEON<subsampling_x, subsampling_y>(
        pred_0, pred_1, pred_stride_1, mask, mask_stride);
    return;
  }
  int y = 0;
  do {
    InterIntraMaskBlending8bpp4x4_NEON<subsampling_x, subsampling_y>(
        pred_0, pred_1, pred_stride_1, mask, mask_stride);
    pred_0 += 4 << 2;
    pred_1 += pred_stride_1 << 2;
    mask += mask_stride << (2 + subsampling_y);

    InterIntraMaskBlending8bpp4x4_NEON<subsampling_x, subsampling_y>(
        pred_0, pred_1, pred_stride_1, mask, mask_stride);
    pred_0 += 4 << 2;
    pred_1 += pred_stride_1 << 2;
    mask += mask_stride << (2 + subsampling_y);
    y += 8;
  } while (y < height);
}

template <int subsampling_x, int subsampling_y>
inline void InterIntraMaskBlending8bpp8xH_NEON(
    const uint8_t* LIBGAV1_RESTRICT pred_0, uint8_t* LIBGAV1_RESTRICT pred_1,
    const ptrdiff_t pred_stride_1, const uint8_t* LIBGAV1_RESTRICT mask,
    const ptrdiff_t mask_stride, const int height) {
  const uint8x8_t mask_inverter = vdup_n_u8(64);
  int y = height;
  do {
    const uint8x8_t pred_mask_1 = GetMask8<subsampling_x, subsampling_y>(mask);
    // 64 - mask
    const uint8x8_t pred_mask_0 = vsub_u8(mask_inverter, pred_mask_1);
    const uint8x8_t pred_val_0 = vld1_u8(pred_0);
    const uint8x8_t pred_val_1 = vld1_u8(pred_1);
    const uint16x8_t weighted_pred_0 = vmull_u8(pred_mask_0, pred_val_0);
    // weighted_pred0 + weighted_pred1
    const uint16x8_t weighted_combo =
        vmlal_u8(weighted_pred_0, pred_mask_1, pred_val_1);
    const uint8x8_t result = vrshrn_n_u16(weighted_combo, 6);
    vst1_u8(pred_1, result);

    pred_0 += 8;
    pred_1 += pred_stride_1;
    mask += mask_stride << subsampling_y;
  } while (--y != 0);
}

template <int subsampling_x, int subsampling_y>
inline void InterIntraMaskBlend8bpp_NEON(
    const uint8_t* LIBGAV1_RESTRICT prediction_0,
    uint8_t* LIBGAV1_RESTRICT prediction_1, const ptrdiff_t prediction_stride_1,
    const uint8_t* LIBGAV1_RESTRICT const mask_ptr, const ptrdiff_t mask_stride,
    const int width, const int height) {
  if (width == 4) {
    InterIntraMaskBlending8bpp4xH_NEON<subsampling_x, subsampling_y>(
        prediction_0, prediction_1, prediction_stride_1, mask_ptr, mask_stride,
        height);
    return;
  }
  if (width == 8) {
    InterIntraMaskBlending8bpp8xH_NEON<subsampling_x, subsampling_y>(
        prediction_0, prediction_1, prediction_stride_1, mask_ptr, mask_stride,
        height);
    return;
  }
  const uint8_t* mask = mask_ptr;
  const uint8x16_t mask_inverter = vdupq_n_u8(64);
  int y = 0;
  do {
    int x = 0;
    do {
      const uint8x16_t pred_mask_1 = GetMask16<subsampling_x, subsampling_y>(
          mask + (x << subsampling_x), mask_stride);
      // 64 - mask
      const uint8x16_t pred_mask_0 = vsubq_u8(mask_inverter, pred_mask_1);
      const uint8x8_t pred_val_0_lo = vld1_u8(prediction_0);
      prediction_0 += 8;
      const uint8x8_t pred_val_0_hi = vld1_u8(prediction_0);
      prediction_0 += 8;
      // Ensure armv7 build combines the load.
      const uint8x16_t pred_val_1 = vld1q_u8(prediction_1 + x);
      const uint8x8_t pred_val_1_lo = vget_low_u8(pred_val_1);
      const uint8x8_t pred_val_1_hi = vget_high_u8(pred_val_1);
      const uint16x8_t weighted_pred_0_lo =
          vmull_u8(vget_low_u8(pred_mask_0), pred_val_0_lo);
      // weighted_pred0 + weighted_pred1
      const uint16x8_t weighted_combo_lo =
          vmlal_u8(weighted_pred_0_lo, vget_low_u8(pred_mask_1), pred_val_1_lo);
      const uint8x8_t result_lo = vrshrn_n_u16(weighted_combo_lo, 6);
      vst1_u8(prediction_1 + x, result_lo);
      const uint16x8_t weighted_pred_0_hi =
          vmull_u8(vget_high_u8(pred_mask_0), pred_val_0_hi);
      // weighted_pred0 + weighted_pred1
      const uint16x8_t weighted_combo_hi = vmlal_u8(
          weighted_pred_0_hi, vget_high_u8(pred_mask_1), pred_val_1_hi);
      const uint8x8_t result_hi = vrshrn_n_u16(weighted_combo_hi, 6);
      vst1_u8(prediction_1 + x + 8, result_hi);

      x += 16;
    } while (x < width);
    prediction_1 += prediction_stride_1;
    mask += mask_stride << subsampling_y;
  } while (++y < height);
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
  dsp->mask_blend[0][0] = MaskBlend_NEON<0, 0>;
  dsp->mask_blend[1][0] = MaskBlend_NEON<1, 0>;
  dsp->mask_blend[2][0] = MaskBlend_NEON<1, 1>;
  // The is_inter_intra index of mask_blend[][] is replaced by
  // inter_intra_mask_blend_8bpp[] in 8-bit.
  dsp->inter_intra_mask_blend_8bpp[0] = InterIntraMaskBlend8bpp_NEON<0, 0>;
  dsp->inter_intra_mask_blend_8bpp[1] = InterIntraMaskBlend8bpp_NEON<1, 0>;
  dsp->inter_intra_mask_blend_8bpp[2] = InterIntraMaskBlend8bpp_NEON<1, 1>;
}

}  // namespace
}  // namespace low_bitdepth

#if LIBGAV1_MAX_BITDEPTH >= 10
namespace high_bitdepth {
namespace {

template <int subsampling_x, int subsampling_y>
inline uint16x8_t GetMask4x2(const uint8_t* mask, ptrdiff_t mask_stride) {
  if (subsampling_x == 1) {
    const uint8x8_t mask_val0 = vld1_u8(mask);
    const uint8x8_t mask_val1 = vld1_u8(mask + (mask_stride << subsampling_y));
    uint16x8_t final_val = vpaddlq_u8(vcombine_u8(mask_val0, mask_val1));
    if (subsampling_y == 1) {
      const uint8x8_t next_mask_val0 = vld1_u8(mask + mask_stride);
      const uint8x8_t next_mask_val1 = vld1_u8(mask + mask_stride * 3);
      final_val = vaddq_u16(
          final_val, vpaddlq_u8(vcombine_u8(next_mask_val0, next_mask_val1)));
    }
    return vrshrq_n_u16(final_val, subsampling_y + 1);
  }
  assert(subsampling_y == 0 && subsampling_x == 0);
  const uint8x8_t mask_val0 = Load4(mask);
  const uint8x8_t mask_val = Load4<1>(mask + mask_stride, mask_val0);
  return vmovl_u8(mask_val);
}

template <int subsampling_x, int subsampling_y>
inline uint16x8_t GetMask8(const uint8_t* mask, ptrdiff_t mask_stride) {
  if (subsampling_x == 1) {
    uint16x8_t mask_val = vpaddlq_u8(vld1q_u8(mask));
    if (subsampling_y == 1) {
      const uint16x8_t next_mask_val = vpaddlq_u8(vld1q_u8(mask + mask_stride));
      mask_val = vaddq_u16(mask_val, next_mask_val);
    }
    return vrshrq_n_u16(mask_val, 1 + subsampling_y);
  }
  assert(subsampling_y == 0 && subsampling_x == 0);
  const uint8x8_t mask_val = vld1_u8(mask);
  return vmovl_u8(mask_val);
}

template <bool is_inter_intra>
uint16x8_t SumWeightedPred(const uint16x8_t pred_mask_0,
                           const uint16x8_t pred_mask_1,
                           const uint16x8_t pred_val_0,
                           const uint16x8_t pred_val_1) {
  if (is_inter_intra) {
    // dst[x] = static_cast<Pixel>(RightShiftWithRounding(
    //     mask_value * pred_1[x] + (64 - mask_value) * pred_0[x], 6));
    uint16x8_t sum = vmulq_u16(pred_mask_1, pred_val_0);
    sum = vmlaq_u16(sum, pred_mask_0, pred_val_1);
    return vrshrq_n_u16(sum, 6);
  } else {
    // int res = (mask_value * prediction_0[x] +
    //      (64 - mask_value) * prediction_1[x]) >> 6;
    const uint32x4_t weighted_pred_0_lo =
        vmull_u16(vget_low_u16(pred_mask_0), vget_low_u16(pred_val_0));
    const uint32x4_t weighted_pred_0_hi = VMullHighU16(pred_mask_0, pred_val_0);
    uint32x4x2_t sum;
    sum.val[0] = vmlal_u16(weighted_pred_0_lo, vget_low_u16(pred_mask_1),
                           vget_low_u16(pred_val_1));
    sum.val[1] = VMlalHighU16(weighted_pred_0_hi, pred_mask_1, pred_val_1);
    return vcombine_u16(vshrn_n_u32(sum.val[0], 6), vshrn_n_u32(sum.val[1], 6));
  }
}

template <bool is_inter_intra, int width, int bitdepth = 10>
inline void StoreShiftedResult(uint8_t* dst, const uint16x8_t result,
                               const ptrdiff_t dst_stride = 0) {
  if (is_inter_intra) {
    if (width == 4) {
      // Store 2 lines of width 4.
      assert(dst_stride != 0);
      vst1_u16(reinterpret_cast<uint16_t*>(dst), vget_low_u16(result));
      vst1_u16(reinterpret_cast<uint16_t*>(dst + dst_stride),
               vget_high_u16(result));
    } else {
      // Store 1 line of width 8.
      vst1q_u16(reinterpret_cast<uint16_t*>(dst), result);
    }
  } else {
    // res -= (bitdepth == 8) ? 0 : kCompoundOffset;
    // dst[x] = static_cast<Pixel>(
    //     Clip3(RightShiftWithRounding(res, inter_post_round_bits), 0,
    //           (1 << kBitdepth8) - 1));
    constexpr int inter_post_round_bits = (bitdepth == 12) ? 2 : 4;
    const uint16x8_t compound_result =
        vminq_u16(vrshrq_n_u16(vqsubq_u16(result, vdupq_n_u16(kCompoundOffset)),
                               inter_post_round_bits),
                  vdupq_n_u16((1 << bitdepth) - 1));
    if (width == 4) {
      // Store 2 lines of width 4.
      assert(dst_stride != 0);
      vst1_u16(reinterpret_cast<uint16_t*>(dst), vget_low_u16(compound_result));
      vst1_u16(reinterpret_cast<uint16_t*>(dst + dst_stride),
               vget_high_u16(compound_result));
    } else {
      // Store 1 line of width 8.
      vst1q_u16(reinterpret_cast<uint16_t*>(dst), compound_result);
    }
  }
}

template <int subsampling_x, int subsampling_y, bool is_inter_intra>
inline void MaskBlend4x2_NEON(const uint16_t* LIBGAV1_RESTRICT pred_0,
                              const uint16_t* LIBGAV1_RESTRICT pred_1,
                              const ptrdiff_t pred_stride_1,
                              const uint8_t* LIBGAV1_RESTRICT mask,
                              const uint16x8_t mask_inverter,
                              const ptrdiff_t mask_stride,
                              uint8_t* LIBGAV1_RESTRICT dst,
                              const ptrdiff_t dst_stride) {
  // This works because stride == width == 4.
  const uint16x8_t pred_val_0 = vld1q_u16(pred_0);
  const uint16x8_t pred_val_1 =
      is_inter_intra
          ? vcombine_u16(vld1_u16(pred_1), vld1_u16(pred_1 + pred_stride_1))
          : vld1q_u16(pred_1);
  const uint16x8_t pred_mask_0 =
      GetMask4x2<subsampling_x, subsampling_y>(mask, mask_stride);
  const uint16x8_t pred_mask_1 = vsubq_u16(mask_inverter, pred_mask_0);
  const uint16x8_t weighted_pred_sum = SumWeightedPred<is_inter_intra>(
      pred_mask_0, pred_mask_1, pred_val_0, pred_val_1);

  StoreShiftedResult<is_inter_intra, 4>(dst, weighted_pred_sum, dst_stride);
}

template <int subsampling_x, int subsampling_y, bool is_inter_intra>
inline void MaskBlending4x4_NEON(const uint16_t* LIBGAV1_RESTRICT pred_0,
                                 const uint16_t* LIBGAV1_RESTRICT pred_1,
                                 const ptrdiff_t pred_stride_1,
                                 const uint8_t* LIBGAV1_RESTRICT mask,
                                 const ptrdiff_t mask_stride,
                                 uint8_t* LIBGAV1_RESTRICT dst,
                                 const ptrdiff_t dst_stride) {
  // Double stride because the function works on 2 lines at a time.
  const ptrdiff_t mask_stride_y = mask_stride << (subsampling_y + 1);
  const ptrdiff_t dst_stride_y = dst_stride << 1;
  const uint16x8_t mask_inverter = vdupq_n_u16(64);

  MaskBlend4x2_NEON<subsampling_x, subsampling_y, is_inter_intra>(
      pred_0, pred_1, pred_stride_1, mask, mask_inverter, mask_stride, dst,
      dst_stride);

  pred_0 += 4 << 1;
  pred_1 += pred_stride_1 << 1;
  mask += mask_stride_y;
  dst += dst_stride_y;

  MaskBlend4x2_NEON<subsampling_x, subsampling_y, is_inter_intra>(
      pred_0, pred_1, pred_stride_1, mask, mask_inverter, mask_stride, dst,
      dst_stride);
}

template <int subsampling_x, int subsampling_y, bool is_inter_intra>
inline void MaskBlending4xH_NEON(const uint16_t* LIBGAV1_RESTRICT pred_0,
                                 const uint16_t* LIBGAV1_RESTRICT pred_1,
                                 const ptrdiff_t pred_stride_1,
                                 const uint8_t* LIBGAV1_RESTRICT const mask_ptr,
                                 const ptrdiff_t mask_stride, const int height,
                                 uint8_t* LIBGAV1_RESTRICT dst,
                                 const ptrdiff_t dst_stride) {
  const uint8_t* mask = mask_ptr;
  if (height == 4) {
    MaskBlending4x4_NEON<subsampling_x, subsampling_y, is_inter_intra>(
        pred_0, pred_1, pred_stride_1, mask, mask_stride, dst, dst_stride);
    return;
  }
  // Double stride because the function works on 2 lines at a time.
  const ptrdiff_t mask_stride_y = mask_stride << (subsampling_y + 1);
  const ptrdiff_t dst_stride_y = dst_stride << 1;
  const uint16x8_t mask_inverter = vdupq_n_u16(64);
  int y = 0;
  do {
    MaskBlend4x2_NEON<subsampling_x, subsampling_y, is_inter_intra>(
        pred_0, pred_1, pred_stride_1, mask, mask_inverter, mask_stride, dst,
        dst_stride);
    pred_0 += 4 << 1;
    pred_1 += pred_stride_1 << 1;
    mask += mask_stride_y;
    dst += dst_stride_y;

    MaskBlend4x2_NEON<subsampling_x, subsampling_y, is_inter_intra>(
        pred_0, pred_1, pred_stride_1, mask, mask_inverter, mask_stride, dst,
        dst_stride);
    pred_0 += 4 << 1;
    pred_1 += pred_stride_1 << 1;
    mask += mask_stride_y;
    dst += dst_stride_y;

    MaskBlend4x2_NEON<subsampling_x, subsampling_y, is_inter_intra>(
        pred_0, pred_1, pred_stride_1, mask, mask_inverter, mask_stride, dst,
        dst_stride);
    pred_0 += 4 << 1;
    pred_1 += pred_stride_1 << 1;
    mask += mask_stride_y;
    dst += dst_stride_y;

    MaskBlend4x2_NEON<subsampling_x, subsampling_y, is_inter_intra>(
        pred_0, pred_1, pred_stride_1, mask, mask_inverter, mask_stride, dst,
        dst_stride);
    pred_0 += 4 << 1;
    pred_1 += pred_stride_1 << 1;
    mask += mask_stride_y;
    dst += dst_stride_y;
    y += 8;
  } while (y < height);
}

template <int subsampling_x, int subsampling_y, bool is_inter_intra>
void MaskBlend8_NEON(const uint16_t* LIBGAV1_RESTRICT pred_0,
                     const uint16_t* LIBGAV1_RESTRICT pred_1,
                     const uint8_t* LIBGAV1_RESTRICT mask,
                     const uint16x8_t mask_inverter,
                     const ptrdiff_t mask_stride,
                     uint8_t* LIBGAV1_RESTRICT dst) {
  const uint16x8_t pred_val_0 = vld1q_u16(pred_0);
  const uint16x8_t pred_val_1 = vld1q_u16(pred_1);
  const uint16x8_t pred_mask_0 =
      GetMask8<subsampling_x, subsampling_y>(mask, mask_stride);
  const uint16x8_t pred_mask_1 = vsubq_u16(mask_inverter, pred_mask_0);
  const uint16x8_t weighted_pred_sum = SumWeightedPred<is_inter_intra>(
      pred_mask_0, pred_mask_1, pred_val_0, pred_val_1);

  StoreShiftedResult<is_inter_intra, 8>(dst, weighted_pred_sum);
}

template <int subsampling_x, int subsampling_y, bool is_inter_intra>
inline void MaskBlend_NEON(const void* LIBGAV1_RESTRICT prediction_0,
                           const void* LIBGAV1_RESTRICT prediction_1,
                           const ptrdiff_t prediction_stride_1,
                           const uint8_t* LIBGAV1_RESTRICT const mask_ptr,
                           const ptrdiff_t mask_stride, const int width,
                           const int height, void* LIBGAV1_RESTRICT dest,
                           const ptrdiff_t dst_stride) {
  if (!is_inter_intra) {
    assert(prediction_stride_1 == width);
  }
  auto* dst = static_cast<uint8_t*>(dest);
  const auto* pred_0 = static_cast<const uint16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const uint16_t*>(prediction_1);
  if (width == 4) {
    MaskBlending4xH_NEON<subsampling_x, subsampling_y, is_inter_intra>(
        pred_0, pred_1, prediction_stride_1, mask_ptr, mask_stride, height, dst,
        dst_stride);
    return;
  }
  const ptrdiff_t mask_stride_y = mask_stride << subsampling_y;
  const uint8_t* mask = mask_ptr;
  const uint16x8_t mask_inverter = vdupq_n_u16(64);
  int y = 0;
  do {
    int x = 0;
    do {
      MaskBlend8_NEON<subsampling_x, subsampling_y, is_inter_intra>(
          pred_0 + x, pred_1 + x, mask + (x << subsampling_x), mask_inverter,
          mask_stride,
          reinterpret_cast<uint8_t*>(reinterpret_cast<uint16_t*>(dst) + x));
      x += 8;
    } while (x < width);
    dst += dst_stride;
    pred_0 += width;
    pred_1 += prediction_stride_1;
    mask += mask_stride_y;
  } while (++y < height);
}

void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth10);
  assert(dsp != nullptr);
  dsp->mask_blend[0][0] = MaskBlend_NEON<0, 0, false>;
  dsp->mask_blend[1][0] = MaskBlend_NEON<1, 0, false>;
  dsp->mask_blend[2][0] = MaskBlend_NEON<1, 1, false>;

  dsp->mask_blend[0][1] = MaskBlend_NEON<0, 0, true>;
  dsp->mask_blend[1][1] = MaskBlend_NEON<1, 0, true>;
  dsp->mask_blend[2][1] = MaskBlend_NEON<1, 1, true>;
}

}  // namespace
}  // namespace high_bitdepth
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

void MaskBlendInit_NEON() {
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

void MaskBlendInit_NEON() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_ENABLE_NEON
