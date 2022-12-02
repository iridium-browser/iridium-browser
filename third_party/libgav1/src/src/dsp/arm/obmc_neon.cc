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

#include "src/dsp/obmc.h"
#include "src/utils/cpu.h"

#if LIBGAV1_ENABLE_NEON

#include <arm_neon.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "src/dsp/arm/common_neon.h"
#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/utils/common.h"

namespace libgav1 {
namespace dsp {
namespace {
#include "src/dsp/obmc.inc"

}  // namespace

namespace low_bitdepth {
namespace {

inline void WriteObmcLine4(uint8_t* LIBGAV1_RESTRICT const pred,
                           const uint8_t* LIBGAV1_RESTRICT const obmc_pred,
                           const uint8x8_t pred_mask,
                           const uint8x8_t obmc_pred_mask) {
  const uint8x8_t pred_val = Load4(pred);
  const uint8x8_t obmc_pred_val = Load4(obmc_pred);
  const uint16x8_t weighted_pred = vmull_u8(pred_mask, pred_val);
  const uint8x8_t result =
      vrshrn_n_u16(vmlal_u8(weighted_pred, obmc_pred_mask, obmc_pred_val), 6);
  StoreLo4(pred, result);
}

inline void WriteObmcLine8(uint8_t* LIBGAV1_RESTRICT const pred,
                           const uint8x8_t obmc_pred_val,
                           const uint8x8_t pred_mask,
                           const uint8x8_t obmc_pred_mask) {
  const uint8x8_t pred_val = vld1_u8(pred);
  const uint16x8_t weighted_pred = vmull_u8(pred_mask, pred_val);
  const uint8x8_t result =
      vrshrn_n_u16(vmlal_u8(weighted_pred, obmc_pred_mask, obmc_pred_val), 6);
  vst1_u8(pred, result);
}

inline void OverlapBlendFromLeft2xH_NEON(
    uint8_t* LIBGAV1_RESTRICT pred, const ptrdiff_t prediction_stride,
    const int height, const uint8_t* LIBGAV1_RESTRICT obmc_pred,
    const ptrdiff_t obmc_prediction_stride) {
  const uint8x8_t mask_inverter = vdup_n_u8(64);
  const uint8x8_t pred_mask = Load2(kObmcMask);
  const uint8x8_t obmc_pred_mask = vsub_u8(mask_inverter, pred_mask);
  uint8x8_t pred_val = vdup_n_u8(0);
  uint8x8_t obmc_pred_val = vdup_n_u8(0);
  int y = 0;
  do {
    pred_val = Load2<0>(pred, pred_val);
    const uint16x8_t weighted_pred = vmull_u8(pred_mask, pred_val);
    obmc_pred_val = Load2<0>(obmc_pred, obmc_pred_val);
    const uint8x8_t result =
        vrshrn_n_u16(vmlal_u8(weighted_pred, obmc_pred_mask, obmc_pred_val), 6);
    Store2<0>(pred, result);

    pred += prediction_stride;
    obmc_pred += obmc_prediction_stride;
  } while (++y != height);
}

inline void OverlapBlendFromLeft4xH_NEON(
    uint8_t* LIBGAV1_RESTRICT pred, const ptrdiff_t prediction_stride,
    const int height, const uint8_t* LIBGAV1_RESTRICT obmc_pred,
    const ptrdiff_t obmc_prediction_stride) {
  const uint8x8_t mask_inverter = vdup_n_u8(64);
  const uint8x8_t pred_mask = Load4(kObmcMask + 2);
  // 64 - mask
  const uint8x8_t obmc_pred_mask = vsub_u8(mask_inverter, pred_mask);
  int y = 0;
  do {
    WriteObmcLine4(pred, obmc_pred, pred_mask, obmc_pred_mask);
    pred += prediction_stride;
    obmc_pred += obmc_prediction_stride;

    WriteObmcLine4(pred, obmc_pred, pred_mask, obmc_pred_mask);
    pred += prediction_stride;
    obmc_pred += obmc_prediction_stride;

    y += 2;
  } while (y != height);
}

inline void OverlapBlendFromLeft8xH_NEON(
    uint8_t* LIBGAV1_RESTRICT pred, const ptrdiff_t prediction_stride,
    const int height, const uint8_t* LIBGAV1_RESTRICT obmc_pred) {
  const uint8x8_t mask_inverter = vdup_n_u8(64);
  const uint8x8_t pred_mask = vld1_u8(kObmcMask + 6);
  constexpr int obmc_prediction_stride = 8;
  // 64 - mask
  const uint8x8_t obmc_pred_mask = vsub_u8(mask_inverter, pred_mask);
  int y = 0;
  do {
    const uint8x16_t obmc_pred_val = vld1q_u8(obmc_pred);
    WriteObmcLine8(pred, vget_low_u8(obmc_pred_val), pred_mask, obmc_pred_mask);
    pred += prediction_stride;

    WriteObmcLine8(pred, vget_high_u8(obmc_pred_val), pred_mask,
                   obmc_pred_mask);
    pred += prediction_stride;

    obmc_pred += obmc_prediction_stride << 1;
    y += 2;
  } while (y != height);
}

void OverlapBlendFromLeft_NEON(
    void* LIBGAV1_RESTRICT const prediction, const ptrdiff_t prediction_stride,
    const int width, const int height,
    const void* LIBGAV1_RESTRICT const obmc_prediction,
    const ptrdiff_t obmc_prediction_stride) {
  auto* pred = static_cast<uint8_t*>(prediction);
  const auto* obmc_pred = static_cast<const uint8_t*>(obmc_prediction);
  assert(width >= 2);
  assert(height >= 4);

  if (width == 2) {
    OverlapBlendFromLeft2xH_NEON(pred, prediction_stride, height, obmc_pred,
                                 obmc_prediction_stride);
    return;
  }
  if (width == 4) {
    OverlapBlendFromLeft4xH_NEON(pred, prediction_stride, height, obmc_pred,
                                 obmc_prediction_stride);
    return;
  }
  if (width == 8) {
    OverlapBlendFromLeft8xH_NEON(pred, prediction_stride, height, obmc_pred);
    return;
  }
  const uint8x16_t mask_inverter = vdupq_n_u8(64);
  const uint8_t* mask = kObmcMask + width - 2;
  int x = 0;
  do {
    pred = static_cast<uint8_t*>(prediction) + x;
    obmc_pred = static_cast<const uint8_t*>(obmc_prediction) + x;
    const uint8x16_t pred_mask = vld1q_u8(mask + x);
    // 64 - mask
    const uint8x16_t obmc_pred_mask = vsubq_u8(mask_inverter, pred_mask);
    int y = 0;
    do {
      const uint8x16_t pred_val = vld1q_u8(pred);
      const uint8x16_t obmc_pred_val = vld1q_u8(obmc_pred);
      const uint16x8_t weighted_pred_lo =
          vmull_u8(vget_low_u8(pred_mask), vget_low_u8(pred_val));
      const uint8x8_t result_lo =
          vrshrn_n_u16(vmlal_u8(weighted_pred_lo, vget_low_u8(obmc_pred_mask),
                                vget_low_u8(obmc_pred_val)),
                       6);
      const uint16x8_t weighted_pred_hi =
          vmull_u8(vget_high_u8(pred_mask), vget_high_u8(pred_val));
      const uint8x8_t result_hi =
          vrshrn_n_u16(vmlal_u8(weighted_pred_hi, vget_high_u8(obmc_pred_mask),
                                vget_high_u8(obmc_pred_val)),
                       6);
      vst1q_u8(pred, vcombine_u8(result_lo, result_hi));

      pred += prediction_stride;
      obmc_pred += obmc_prediction_stride;
    } while (++y < height);
    x += 16;
  } while (x < width);
}

inline void OverlapBlendFromTop4x4_NEON(
    uint8_t* LIBGAV1_RESTRICT pred, const ptrdiff_t prediction_stride,
    const uint8_t* LIBGAV1_RESTRICT obmc_pred,
    const ptrdiff_t obmc_prediction_stride, const int height) {
  uint8x8_t pred_mask = vdup_n_u8(kObmcMask[height - 2]);
  const uint8x8_t mask_inverter = vdup_n_u8(64);
  uint8x8_t obmc_pred_mask = vsub_u8(mask_inverter, pred_mask);
  WriteObmcLine4(pred, obmc_pred, pred_mask, obmc_pred_mask);
  pred += prediction_stride;
  obmc_pred += obmc_prediction_stride;

  if (height == 2) {
    return;
  }

  pred_mask = vdup_n_u8(kObmcMask[3]);
  obmc_pred_mask = vsub_u8(mask_inverter, pred_mask);
  WriteObmcLine4(pred, obmc_pred, pred_mask, obmc_pred_mask);
  pred += prediction_stride;
  obmc_pred += obmc_prediction_stride;

  pred_mask = vdup_n_u8(kObmcMask[4]);
  obmc_pred_mask = vsub_u8(mask_inverter, pred_mask);
  WriteObmcLine4(pred, obmc_pred, pred_mask, obmc_pred_mask);
}

inline void OverlapBlendFromTop4xH_NEON(
    uint8_t* LIBGAV1_RESTRICT pred, const ptrdiff_t prediction_stride,
    const int height, const uint8_t* LIBGAV1_RESTRICT obmc_pred,
    const ptrdiff_t obmc_prediction_stride) {
  if (height < 8) {
    OverlapBlendFromTop4x4_NEON(pred, prediction_stride, obmc_pred,
                                obmc_prediction_stride, height);
    return;
  }
  const uint8_t* mask = kObmcMask + height - 2;
  const uint8x8_t mask_inverter = vdup_n_u8(64);
  int y = 0;
  // Compute 6 lines for height 8, or 12 lines for height 16. The remaining
  // lines are unchanged as the corresponding mask value is 64.
  do {
    uint8x8_t pred_mask = vdup_n_u8(mask[y]);
    uint8x8_t obmc_pred_mask = vsub_u8(mask_inverter, pred_mask);
    WriteObmcLine4(pred, obmc_pred, pred_mask, obmc_pred_mask);
    pred += prediction_stride;
    obmc_pred += obmc_prediction_stride;

    pred_mask = vdup_n_u8(mask[y + 1]);
    obmc_pred_mask = vsub_u8(mask_inverter, pred_mask);
    WriteObmcLine4(pred, obmc_pred, pred_mask, obmc_pred_mask);
    pred += prediction_stride;
    obmc_pred += obmc_prediction_stride;

    pred_mask = vdup_n_u8(mask[y + 2]);
    obmc_pred_mask = vsub_u8(mask_inverter, pred_mask);
    WriteObmcLine4(pred, obmc_pred, pred_mask, obmc_pred_mask);
    pred += prediction_stride;
    obmc_pred += obmc_prediction_stride;

    pred_mask = vdup_n_u8(mask[y + 3]);
    obmc_pred_mask = vsub_u8(mask_inverter, pred_mask);
    WriteObmcLine4(pred, obmc_pred, pred_mask, obmc_pred_mask);
    pred += prediction_stride;
    obmc_pred += obmc_prediction_stride;

    pred_mask = vdup_n_u8(mask[y + 4]);
    obmc_pred_mask = vsub_u8(mask_inverter, pred_mask);
    WriteObmcLine4(pred, obmc_pred, pred_mask, obmc_pred_mask);
    pred += prediction_stride;
    obmc_pred += obmc_prediction_stride;

    pred_mask = vdup_n_u8(mask[y + 5]);
    obmc_pred_mask = vsub_u8(mask_inverter, pred_mask);
    WriteObmcLine4(pred, obmc_pred, pred_mask, obmc_pred_mask);
    pred += prediction_stride;
    obmc_pred += obmc_prediction_stride;

    // Increment for the right mask index.
    y += 6;
  } while (y < height - 4);
}

inline void OverlapBlendFromTop8xH_NEON(
    uint8_t* LIBGAV1_RESTRICT pred, const ptrdiff_t prediction_stride,
    const int height, const uint8_t* LIBGAV1_RESTRICT obmc_pred) {
  constexpr int obmc_prediction_stride = 8;
  const uint8x8_t mask_inverter = vdup_n_u8(64);
  const uint8_t* mask = kObmcMask + height - 2;
  const int compute_height = height - (height >> 2);
  int y = 0;
  do {
    const uint8x8_t pred_mask0 = vdup_n_u8(mask[y]);
    // 64 - mask
    const uint8x8_t obmc_pred_mask0 = vsub_u8(mask_inverter, pred_mask0);
    const uint8x16_t obmc_pred_val = vld1q_u8(obmc_pred);

    WriteObmcLine8(pred, vget_low_u8(obmc_pred_val), pred_mask0,
                   obmc_pred_mask0);
    pred += prediction_stride;
    ++y;

    const uint8x8_t pred_mask1 = vdup_n_u8(mask[y]);
    // 64 - mask
    const uint8x8_t obmc_pred_mask1 = vsub_u8(mask_inverter, pred_mask1);
    WriteObmcLine8(pred, vget_high_u8(obmc_pred_val), pred_mask1,
                   obmc_pred_mask1);
    pred += prediction_stride;
    obmc_pred += obmc_prediction_stride << 1;
  } while (++y < compute_height);
}

void OverlapBlendFromTop_NEON(
    void* LIBGAV1_RESTRICT const prediction, const ptrdiff_t prediction_stride,
    const int width, const int height,
    const void* LIBGAV1_RESTRICT const obmc_prediction,
    const ptrdiff_t obmc_prediction_stride) {
  auto* pred = static_cast<uint8_t*>(prediction);
  const auto* obmc_pred = static_cast<const uint8_t*>(obmc_prediction);
  assert(width >= 4);
  assert(height >= 2);

  if (width == 4) {
    OverlapBlendFromTop4xH_NEON(pred, prediction_stride, height, obmc_pred,
                                obmc_prediction_stride);
    return;
  }

  if (width == 8) {
    OverlapBlendFromTop8xH_NEON(pred, prediction_stride, height, obmc_pred);
    return;
  }

  const uint8_t* mask = kObmcMask + height - 2;
  const uint8x8_t mask_inverter = vdup_n_u8(64);
  // Stop when mask value becomes 64. This is inferred for 4xH.
  const int compute_height = height - (height >> 2);
  int y = 0;
  do {
    const uint8x8_t pred_mask = vdup_n_u8(mask[y]);
    // 64 - mask
    const uint8x8_t obmc_pred_mask = vsub_u8(mask_inverter, pred_mask);
    int x = 0;
    do {
      const uint8x16_t pred_val = vld1q_u8(pred + x);
      const uint8x16_t obmc_pred_val = vld1q_u8(obmc_pred + x);
      const uint16x8_t weighted_pred_lo =
          vmull_u8(pred_mask, vget_low_u8(pred_val));
      const uint8x8_t result_lo =
          vrshrn_n_u16(vmlal_u8(weighted_pred_lo, obmc_pred_mask,
                                vget_low_u8(obmc_pred_val)),
                       6);
      const uint16x8_t weighted_pred_hi =
          vmull_u8(pred_mask, vget_high_u8(pred_val));
      const uint8x8_t result_hi =
          vrshrn_n_u16(vmlal_u8(weighted_pred_hi, obmc_pred_mask,
                                vget_high_u8(obmc_pred_val)),
                       6);
      vst1q_u8(pred + x, vcombine_u8(result_lo, result_hi));

      x += 16;
    } while (x < width);
    pred += prediction_stride;
    obmc_pred += obmc_prediction_stride;
  } while (++y < compute_height);
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
  dsp->obmc_blend[kObmcDirectionVertical] = OverlapBlendFromTop_NEON;
  dsp->obmc_blend[kObmcDirectionHorizontal] = OverlapBlendFromLeft_NEON;
}

}  // namespace
}  // namespace low_bitdepth

#if LIBGAV1_MAX_BITDEPTH >= 10
namespace high_bitdepth {
namespace {

// This is a flat array of masks for each block dimension from 2 to 32. The
// starting index for each length is length-2. The value 64 leaves the result
// equal to |pred| and may be ignored if convenient. Vector loads may overrread
// values meant for larger sizes, but these values will be unused.
constexpr uint16_t kObmcMask[62] = {
    // Obmc Mask 2
    45, 64,
    // Obmc Mask 4
    39, 50, 59, 64,
    // Obmc Mask 8
    36, 42, 48, 53, 57, 61, 64, 64,
    // Obmc Mask 16
    34, 37, 40, 43, 46, 49, 52, 54, 56, 58, 60, 61, 64, 64, 64, 64,
    // Obmc Mask 32
    33, 35, 36, 38, 40, 41, 43, 44, 45, 47, 48, 50, 51, 52, 53, 55, 56, 57, 58,
    59, 60, 60, 61, 62, 64, 64, 64, 64, 64, 64, 64, 64};

inline uint16x4_t BlendObmc2Or4(uint16_t* const pred,
                                const uint16x4_t obmc_pred_val,
                                const uint16x4_t pred_mask,
                                const uint16x4_t obmc_pred_mask) {
  const uint16x4_t pred_val = vld1_u16(pred);
  const uint16x4_t weighted_pred = vmul_u16(pred_mask, pred_val);
  const uint16x4_t result =
      vrshr_n_u16(vmla_u16(weighted_pred, obmc_pred_mask, obmc_pred_val), 6);
  return result;
}

inline uint16x8_t BlendObmc8(uint16_t* LIBGAV1_RESTRICT const pred,
                             const uint16_t* LIBGAV1_RESTRICT const obmc_pred,
                             const uint16x8_t pred_mask,
                             const uint16x8_t obmc_pred_mask) {
  const uint16x8_t pred_val = vld1q_u16(pred);
  const uint16x8_t obmc_pred_val = vld1q_u16(obmc_pred);
  const uint16x8_t weighted_pred = vmulq_u16(pred_mask, pred_val);
  const uint16x8_t result =
      vrshrq_n_u16(vmlaq_u16(weighted_pred, obmc_pred_mask, obmc_pred_val), 6);
  return result;
}

inline void OverlapBlendFromLeft2xH_NEON(
    uint16_t* LIBGAV1_RESTRICT pred, const ptrdiff_t prediction_stride,
    const int height, const uint16_t* LIBGAV1_RESTRICT obmc_pred) {
  constexpr int obmc_prediction_stride = 2;
  const uint16x4_t mask_inverter = vdup_n_u16(64);
  // Second two lanes unused.
  const uint16x4_t pred_mask = vld1_u16(kObmcMask);
  const uint16x4_t obmc_pred_mask = vsub_u16(mask_inverter, pred_mask);
  int y = 0;
  do {
    const uint16x4_t obmc_pred_0 = vld1_u16(obmc_pred);
    const uint16x4_t result_0 =
        BlendObmc2Or4(pred, obmc_pred_0, pred_mask, obmc_pred_mask);
    Store2<0>(pred, result_0);

    pred = AddByteStride(pred, prediction_stride);
    obmc_pred += obmc_prediction_stride;

    const uint16x4_t obmc_pred_1 = vld1_u16(obmc_pred);
    const uint16x4_t result_1 =
        BlendObmc2Or4(pred, obmc_pred_1, pred_mask, obmc_pred_mask);
    Store2<0>(pred, result_1);

    pred = AddByteStride(pred, prediction_stride);
    obmc_pred += obmc_prediction_stride;

    y += 2;
  } while (y != height);
}

inline void OverlapBlendFromLeft4xH_NEON(
    uint16_t* LIBGAV1_RESTRICT pred, const ptrdiff_t prediction_stride,
    const int height, const uint16_t* LIBGAV1_RESTRICT obmc_pred) {
  constexpr int obmc_prediction_stride = 4;
  const uint16x4_t mask_inverter = vdup_n_u16(64);
  const uint16x4_t pred_mask = vld1_u16(kObmcMask + 2);
  // 64 - mask
  const uint16x4_t obmc_pred_mask = vsub_u16(mask_inverter, pred_mask);
  int y = 0;
  do {
    const uint16x8_t obmc_pred_val = vld1q_u16(obmc_pred);
    const uint16x4_t result_0 = BlendObmc2Or4(pred, vget_low_u16(obmc_pred_val),
                                              pred_mask, obmc_pred_mask);
    vst1_u16(pred, result_0);
    pred = AddByteStride(pred, prediction_stride);

    const uint16x4_t result_1 = BlendObmc2Or4(
        pred, vget_high_u16(obmc_pred_val), pred_mask, obmc_pred_mask);
    vst1_u16(pred, result_1);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred += obmc_prediction_stride << 1;

    y += 2;
  } while (y != height);
}

void OverlapBlendFromLeft_NEON(
    void* LIBGAV1_RESTRICT const prediction, const ptrdiff_t prediction_stride,
    const int width, const int height,
    const void* LIBGAV1_RESTRICT const obmc_prediction,
    const ptrdiff_t obmc_prediction_stride) {
  auto* pred = static_cast<uint16_t*>(prediction);
  const auto* obmc_pred = static_cast<const uint16_t*>(obmc_prediction);
  assert(width >= 2);
  assert(height >= 4);

  if (width == 2) {
    OverlapBlendFromLeft2xH_NEON(pred, prediction_stride, height, obmc_pred);
    return;
  }
  if (width == 4) {
    OverlapBlendFromLeft4xH_NEON(pred, prediction_stride, height, obmc_pred);
    return;
  }
  const uint16x8_t mask_inverter = vdupq_n_u16(64);
  const uint16_t* mask = kObmcMask + width - 2;
  int x = 0;
  do {
    uint16_t* pred_x = pred + x;
    const uint16_t* obmc_pred_x = obmc_pred + x;
    const uint16x8_t pred_mask = vld1q_u16(mask + x);
    // 64 - mask
    const uint16x8_t obmc_pred_mask = vsubq_u16(mask_inverter, pred_mask);
    int y = 0;
    do {
      const uint16x8_t result =
          BlendObmc8(pred_x, obmc_pred_x, pred_mask, obmc_pred_mask);
      vst1q_u16(pred_x, result);

      pred_x = AddByteStride(pred_x, prediction_stride);
      obmc_pred_x = AddByteStride(obmc_pred_x, obmc_prediction_stride);
    } while (++y < height);
    x += 8;
  } while (x < width);
}

template <int lane>
inline uint16x4_t BlendObmcFromTop4(uint16_t* const pred,
                                    const uint16x4_t obmc_pred_val,
                                    const uint16x8_t pred_mask,
                                    const uint16x8_t obmc_pred_mask) {
  const uint16x4_t pred_val = vld1_u16(pred);
  const uint16x4_t weighted_pred = VMulLaneQU16<lane>(pred_val, pred_mask);
  const uint16x4_t result = vrshr_n_u16(
      VMlaLaneQU16<lane>(weighted_pred, obmc_pred_val, obmc_pred_mask), 6);
  return result;
}

template <int lane>
inline uint16x8_t BlendObmcFromTop8(
    uint16_t* LIBGAV1_RESTRICT const pred,
    const uint16_t* LIBGAV1_RESTRICT const obmc_pred,
    const uint16x8_t pred_mask, const uint16x8_t obmc_pred_mask) {
  const uint16x8_t pred_val = vld1q_u16(pred);
  const uint16x8_t obmc_pred_val = vld1q_u16(obmc_pred);
  const uint16x8_t weighted_pred = VMulQLaneQU16<lane>(pred_val, pred_mask);
  const uint16x8_t result = vrshrq_n_u16(
      VMlaQLaneQU16<lane>(weighted_pred, obmc_pred_val, obmc_pred_mask), 6);
  return result;
}

inline void OverlapBlendFromTop4x2Or4_NEON(
    uint16_t* LIBGAV1_RESTRICT pred, const ptrdiff_t prediction_stride,
    const uint16_t* LIBGAV1_RESTRICT obmc_pred, const int height) {
  constexpr int obmc_prediction_stride = 4;
  const uint16x8_t pred_mask = vld1q_u16(&kObmcMask[height - 2]);
  const uint16x8_t mask_inverter = vdupq_n_u16(64);
  const uint16x8_t obmc_pred_mask = vsubq_u16(mask_inverter, pred_mask);
  const uint16x8_t obmc_pred_val_0 = vld1q_u16(obmc_pred);
  uint16x4_t result = BlendObmcFromTop4<0>(pred, vget_low_u16(obmc_pred_val_0),
                                           pred_mask, obmc_pred_mask);
  vst1_u16(pred, result);
  pred = AddByteStride(pred, prediction_stride);

  if (height == 2) {
    // Mask value is 64, meaning |pred| is unchanged.
    return;
  }

  result = BlendObmcFromTop4<1>(pred, vget_high_u16(obmc_pred_val_0), pred_mask,
                                obmc_pred_mask);
  vst1_u16(pred, result);
  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride << 1;

  const uint16x4_t obmc_pred_val_2 = vld1_u16(obmc_pred);
  result =
      BlendObmcFromTop4<2>(pred, obmc_pred_val_2, pred_mask, obmc_pred_mask);
  vst1_u16(pred, result);
}

inline void OverlapBlendFromTop4xH_NEON(
    uint16_t* LIBGAV1_RESTRICT pred, const ptrdiff_t prediction_stride,
    const int height, const uint16_t* LIBGAV1_RESTRICT obmc_pred) {
  if (height < 8) {
    OverlapBlendFromTop4x2Or4_NEON(pred, prediction_stride, obmc_pred, height);
    return;
  }
  constexpr int obmc_prediction_stride = 4;
  const uint16_t* mask = kObmcMask + height - 2;
  const uint16x8_t mask_inverter = vdupq_n_u16(64);
  int y = 0;
  // Compute 6 lines for height 8, or 12 lines for height 16. The remaining
  // lines are unchanged as the corresponding mask value is 64.
  do {
    const uint16x8_t pred_mask = vld1q_u16(&mask[y]);
    const uint16x8_t obmc_pred_mask = vsubq_u16(mask_inverter, pred_mask);
    // Load obmc row 0, 1.
    uint16x8_t obmc_pred_val = vld1q_u16(obmc_pred);
    uint16x4_t result = BlendObmcFromTop4<0>(pred, vget_low_u16(obmc_pred_val),
                                             pred_mask, obmc_pred_mask);
    vst1_u16(pred, result);
    pred = AddByteStride(pred, prediction_stride);

    result = BlendObmcFromTop4<1>(pred, vget_high_u16(obmc_pred_val), pred_mask,
                                  obmc_pred_mask);
    vst1_u16(pred, result);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred += obmc_prediction_stride << 1;

    // Load obmc row 2, 3.
    obmc_pred_val = vld1q_u16(obmc_pred);
    result = BlendObmcFromTop4<2>(pred, vget_low_u16(obmc_pred_val), pred_mask,
                                  obmc_pred_mask);
    vst1_u16(pred, result);
    pred = AddByteStride(pred, prediction_stride);

    result = BlendObmcFromTop4<3>(pred, vget_high_u16(obmc_pred_val), pred_mask,
                                  obmc_pred_mask);
    vst1_u16(pred, result);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred += obmc_prediction_stride << 1;

    // Load obmc row 4, 5.
    obmc_pred_val = vld1q_u16(obmc_pred);
    result = BlendObmcFromTop4<4>(pred, vget_low_u16(obmc_pred_val), pred_mask,
                                  obmc_pred_mask);
    vst1_u16(pred, result);
    pred = AddByteStride(pred, prediction_stride);

    result = BlendObmcFromTop4<5>(pred, vget_high_u16(obmc_pred_val), pred_mask,
                                  obmc_pred_mask);
    vst1_u16(pred, result);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred += obmc_prediction_stride << 1;

    // Increment for the right mask index.
    y += 6;
  } while (y < height - 4);
}

inline void OverlapBlendFromTop8xH_NEON(
    uint16_t* LIBGAV1_RESTRICT pred, const ptrdiff_t prediction_stride,
    const uint16_t* LIBGAV1_RESTRICT obmc_pred, const int height) {
  const uint16_t* mask = kObmcMask + height - 2;
  const uint16x8_t mask_inverter = vdupq_n_u16(64);
  uint16x8_t pred_mask = vld1q_u16(mask);
  uint16x8_t obmc_pred_mask = vsubq_u16(mask_inverter, pred_mask);
  uint16x8_t result =
      BlendObmcFromTop8<0>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);
  if (height == 2) return;

  constexpr int obmc_prediction_stride = 8;
  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  result = BlendObmcFromTop8<1>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);
  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  result = BlendObmcFromTop8<2>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);
  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  result = BlendObmcFromTop8<3>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);
  if (height == 4) return;

  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  result = BlendObmcFromTop8<4>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);
  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  result = BlendObmcFromTop8<5>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);

  if (height == 8) return;

  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  result = BlendObmcFromTop8<6>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);
  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  result = BlendObmcFromTop8<7>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);
  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  pred_mask = vld1q_u16(&mask[8]);
  obmc_pred_mask = vsubq_u16(mask_inverter, pred_mask);

  result = BlendObmcFromTop8<0>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);
  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  result = BlendObmcFromTop8<1>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);
  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  result = BlendObmcFromTop8<2>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);
  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  result = BlendObmcFromTop8<3>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);

  if (height == 16) return;

  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  result = BlendObmcFromTop8<4>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);
  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  result = BlendObmcFromTop8<5>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);
  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  result = BlendObmcFromTop8<6>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);
  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  result = BlendObmcFromTop8<7>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);
  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  pred_mask = vld1q_u16(&mask[16]);
  obmc_pred_mask = vsubq_u16(mask_inverter, pred_mask);

  result = BlendObmcFromTop8<0>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);
  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  result = BlendObmcFromTop8<1>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);
  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  result = BlendObmcFromTop8<2>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);
  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  result = BlendObmcFromTop8<3>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);
  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  result = BlendObmcFromTop8<4>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);
  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  result = BlendObmcFromTop8<5>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);
  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  result = BlendObmcFromTop8<6>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);
  pred = AddByteStride(pred, prediction_stride);
  obmc_pred += obmc_prediction_stride;

  result = BlendObmcFromTop8<7>(pred, obmc_pred, pred_mask, obmc_pred_mask);
  vst1q_u16(pred, result);
}

void OverlapBlendFromTop_NEON(
    void* LIBGAV1_RESTRICT const prediction, const ptrdiff_t prediction_stride,
    const int width, const int height,
    const void* LIBGAV1_RESTRICT const obmc_prediction,
    const ptrdiff_t obmc_prediction_stride) {
  auto* pred = static_cast<uint16_t*>(prediction);
  const auto* obmc_pred = static_cast<const uint16_t*>(obmc_prediction);
  assert(width >= 4);
  assert(height >= 2);

  if (width == 4) {
    OverlapBlendFromTop4xH_NEON(pred, prediction_stride, height, obmc_pred);
    return;
  }

  if (width == 8) {
    OverlapBlendFromTop8xH_NEON(pred, prediction_stride, obmc_pred, height);
    return;
  }

  const uint16_t* mask = kObmcMask + height - 2;
  const uint16x8_t mask_inverter = vdupq_n_u16(64);
  const uint16x8_t pred_mask = vld1q_u16(mask);
  // 64 - mask
  const uint16x8_t obmc_pred_mask = vsubq_u16(mask_inverter, pred_mask);
#define OBMC_ROW_FROM_TOP(n)                                   \
  do {                                                         \
    int x = 0;                                                 \
    do {                                                       \
      const uint16x8_t result = BlendObmcFromTop8<n>(          \
          pred + x, obmc_pred + x, pred_mask, obmc_pred_mask); \
      vst1q_u16(pred + x, result);                             \
                                                               \
      x += 8;                                                  \
    } while (x < width);                                       \
  } while (false)

  // Compute 1 row.
  if (height == 2) {
    OBMC_ROW_FROM_TOP(0);
    return;
  }

  // Compute 3 rows.
  if (height == 4) {
    OBMC_ROW_FROM_TOP(0);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(1);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(2);
    return;
  }

  // Compute 6 rows.
  if (height == 8) {
    OBMC_ROW_FROM_TOP(0);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(1);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(2);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(3);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(4);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(5);
    return;
  }

  // Compute 12 rows.
  if (height == 16) {
    OBMC_ROW_FROM_TOP(0);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(1);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(2);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(3);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(4);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(5);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(6);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(7);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);

    const uint16x8_t pred_mask = vld1q_u16(&mask[8]);
    // 64 - mask
    const uint16x8_t obmc_pred_mask = vsubq_u16(mask_inverter, pred_mask);
    OBMC_ROW_FROM_TOP(0);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(1);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(2);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(3);
    return;
  }

  // Stop when mask value becomes 64. This is a multiple of 8 for height 32
  // and 64.
  const int compute_height = height - (height >> 2);
  int y = 0;
  do {
    const uint16x8_t pred_mask = vld1q_u16(&mask[y]);
    // 64 - mask
    const uint16x8_t obmc_pred_mask = vsubq_u16(mask_inverter, pred_mask);
    OBMC_ROW_FROM_TOP(0);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(1);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(2);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(3);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(4);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(5);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(6);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);
    OBMC_ROW_FROM_TOP(7);
    pred = AddByteStride(pred, prediction_stride);
    obmc_pred = AddByteStride(obmc_pred, obmc_prediction_stride);

    y += 8;
  } while (y < compute_height);
}

void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth10);
  assert(dsp != nullptr);
  dsp->obmc_blend[kObmcDirectionVertical] = OverlapBlendFromTop_NEON;
  dsp->obmc_blend[kObmcDirectionHorizontal] = OverlapBlendFromLeft_NEON;
}

}  // namespace
}  // namespace high_bitdepth
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

void ObmcInit_NEON() {
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

void ObmcInit_NEON() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_ENABLE_NEON
