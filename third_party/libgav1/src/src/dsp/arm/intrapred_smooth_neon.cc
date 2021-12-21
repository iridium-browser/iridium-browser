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

#include "src/dsp/intrapred_smooth.h"
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
#include "src/utils/constants.h"

namespace libgav1 {
namespace dsp {

namespace low_bitdepth {
namespace {

// Note these constants are duplicated from intrapred.cc to allow the compiler
// to have visibility of the values. This helps reduce loads and in the
// creation of the inverse weights.
constexpr uint8_t kSmoothWeights[] = {
#include "src/dsp/smooth_weights.inc"
};

// TODO(b/150459137): Keeping the intermediate values in uint16_t would allow
// processing more values at once. At the high end, it could do 4x4 or 8x2 at a
// time.
inline uint16x4_t CalculatePred(const uint16x4_t weighted_top,
                                const uint16x4_t weighted_left,
                                const uint16x4_t weighted_bl,
                                const uint16x4_t weighted_tr) {
  const uint32x4_t pred_0 = vaddl_u16(weighted_top, weighted_left);
  const uint32x4_t pred_1 = vaddl_u16(weighted_bl, weighted_tr);
  const uint32x4_t pred_2 = vaddq_u32(pred_0, pred_1);
  return vrshrn_n_u32(pred_2, kSmoothWeightScale + 1);
}

template <int width, int height>
inline void Smooth4Or8xN_NEON(void* LIBGAV1_RESTRICT const dest,
                              ptrdiff_t stride,
                              const void* LIBGAV1_RESTRICT const top_row,
                              const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const auto* const left = static_cast<const uint8_t*>(left_column);
  const uint8_t top_right = top[width - 1];
  const uint8_t bottom_left = left[height - 1];
  const uint8_t* const weights_y = kSmoothWeights + height - 4;
  auto* dst = static_cast<uint8_t*>(dest);

  uint8x8_t top_v;
  if (width == 4) {
    top_v = Load4(top);
  } else {  // width == 8
    top_v = vld1_u8(top);
  }
  const uint8x8_t top_right_v = vdup_n_u8(top_right);
  const uint8x8_t bottom_left_v = vdup_n_u8(bottom_left);
  // Over-reads for 4xN but still within the array.
  const uint8x8_t weights_x_v = vld1_u8(kSmoothWeights + width - 4);
  // 256 - weights = vneg_s8(weights)
  const uint8x8_t scaled_weights_x =
      vreinterpret_u8_s8(vneg_s8(vreinterpret_s8_u8(weights_x_v)));

  for (int y = 0; y < height; ++y) {
    const uint8x8_t left_v = vdup_n_u8(left[y]);
    const uint8x8_t weights_y_v = vdup_n_u8(weights_y[y]);
    const uint8x8_t scaled_weights_y = vdup_n_u8(256 - weights_y[y]);
    const uint16x8_t weighted_bl = vmull_u8(scaled_weights_y, bottom_left_v);

    const uint16x8_t weighted_top = vmull_u8(weights_y_v, top_v);
    const uint16x8_t weighted_left = vmull_u8(weights_x_v, left_v);
    const uint16x8_t weighted_tr = vmull_u8(scaled_weights_x, top_right_v);
    const uint16x4_t dest_0 =
        CalculatePred(vget_low_u16(weighted_top), vget_low_u16(weighted_left),
                      vget_low_u16(weighted_tr), vget_low_u16(weighted_bl));

    if (width == 4) {
      StoreLo4(dst, vmovn_u16(vcombine_u16(dest_0, dest_0)));
    } else {  // width == 8
      const uint16x4_t dest_1 = CalculatePred(
          vget_high_u16(weighted_top), vget_high_u16(weighted_left),
          vget_high_u16(weighted_tr), vget_high_u16(weighted_bl));
      vst1_u8(dst, vmovn_u16(vcombine_u16(dest_0, dest_1)));
    }
    dst += stride;
  }
}

inline uint8x16_t CalculateWeightsAndPred(
    const uint8x16_t top, const uint8x8_t left, const uint8x8_t top_right,
    const uint8x8_t weights_y, const uint8x16_t weights_x,
    const uint8x16_t scaled_weights_x, const uint16x8_t weighted_bl) {
  const uint16x8_t weighted_top_low = vmull_u8(weights_y, vget_low_u8(top));
  const uint16x8_t weighted_left_low = vmull_u8(vget_low_u8(weights_x), left);
  const uint16x8_t weighted_tr_low =
      vmull_u8(vget_low_u8(scaled_weights_x), top_right);
  const uint16x4_t dest_0 = CalculatePred(
      vget_low_u16(weighted_top_low), vget_low_u16(weighted_left_low),
      vget_low_u16(weighted_tr_low), vget_low_u16(weighted_bl));
  const uint16x4_t dest_1 = CalculatePred(
      vget_high_u16(weighted_top_low), vget_high_u16(weighted_left_low),
      vget_high_u16(weighted_tr_low), vget_high_u16(weighted_bl));
  const uint8x8_t dest_0_u8 = vmovn_u16(vcombine_u16(dest_0, dest_1));

  const uint16x8_t weighted_top_high = vmull_u8(weights_y, vget_high_u8(top));
  const uint16x8_t weighted_left_high = vmull_u8(vget_high_u8(weights_x), left);
  const uint16x8_t weighted_tr_high =
      vmull_u8(vget_high_u8(scaled_weights_x), top_right);
  const uint16x4_t dest_2 = CalculatePred(
      vget_low_u16(weighted_top_high), vget_low_u16(weighted_left_high),
      vget_low_u16(weighted_tr_high), vget_low_u16(weighted_bl));
  const uint16x4_t dest_3 = CalculatePred(
      vget_high_u16(weighted_top_high), vget_high_u16(weighted_left_high),
      vget_high_u16(weighted_tr_high), vget_high_u16(weighted_bl));
  const uint8x8_t dest_1_u8 = vmovn_u16(vcombine_u16(dest_2, dest_3));

  return vcombine_u8(dest_0_u8, dest_1_u8);
}

template <int width, int height>
inline void Smooth16PlusxN_NEON(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const auto* const left = static_cast<const uint8_t*>(left_column);
  const uint8_t top_right = top[width - 1];
  const uint8_t bottom_left = left[height - 1];
  const uint8_t* const weights_y = kSmoothWeights + height - 4;
  auto* dst = static_cast<uint8_t*>(dest);

  uint8x16_t top_v[4];
  top_v[0] = vld1q_u8(top);
  if (width > 16) {
    top_v[1] = vld1q_u8(top + 16);
    if (width == 64) {
      top_v[2] = vld1q_u8(top + 32);
      top_v[3] = vld1q_u8(top + 48);
    }
  }

  const uint8x8_t top_right_v = vdup_n_u8(top_right);
  const uint8x8_t bottom_left_v = vdup_n_u8(bottom_left);

  // TODO(johannkoenig): Consider re-reading top_v and weights_x_v in the loop.
  // This currently has a performance slope similar to Paeth so it does not
  // appear to be register bound for arm64.
  uint8x16_t weights_x_v[4];
  weights_x_v[0] = vld1q_u8(kSmoothWeights + width - 4);
  if (width > 16) {
    weights_x_v[1] = vld1q_u8(kSmoothWeights + width + 16 - 4);
    if (width == 64) {
      weights_x_v[2] = vld1q_u8(kSmoothWeights + width + 32 - 4);
      weights_x_v[3] = vld1q_u8(kSmoothWeights + width + 48 - 4);
    }
  }

  uint8x16_t scaled_weights_x[4];
  scaled_weights_x[0] =
      vreinterpretq_u8_s8(vnegq_s8(vreinterpretq_s8_u8(weights_x_v[0])));
  if (width > 16) {
    scaled_weights_x[1] =
        vreinterpretq_u8_s8(vnegq_s8(vreinterpretq_s8_u8(weights_x_v[1])));
    if (width == 64) {
      scaled_weights_x[2] =
          vreinterpretq_u8_s8(vnegq_s8(vreinterpretq_s8_u8(weights_x_v[2])));
      scaled_weights_x[3] =
          vreinterpretq_u8_s8(vnegq_s8(vreinterpretq_s8_u8(weights_x_v[3])));
    }
  }

  for (int y = 0; y < height; ++y) {
    const uint8x8_t left_v = vdup_n_u8(left[y]);
    const uint8x8_t weights_y_v = vdup_n_u8(weights_y[y]);
    const uint8x8_t scaled_weights_y = vdup_n_u8(256 - weights_y[y]);
    const uint16x8_t weighted_bl = vmull_u8(scaled_weights_y, bottom_left_v);

    vst1q_u8(dst, CalculateWeightsAndPred(top_v[0], left_v, top_right_v,
                                          weights_y_v, weights_x_v[0],
                                          scaled_weights_x[0], weighted_bl));

    if (width > 16) {
      vst1q_u8(dst + 16, CalculateWeightsAndPred(
                             top_v[1], left_v, top_right_v, weights_y_v,
                             weights_x_v[1], scaled_weights_x[1], weighted_bl));
      if (width == 64) {
        vst1q_u8(dst + 32,
                 CalculateWeightsAndPred(top_v[2], left_v, top_right_v,
                                         weights_y_v, weights_x_v[2],
                                         scaled_weights_x[2], weighted_bl));
        vst1q_u8(dst + 48,
                 CalculateWeightsAndPred(top_v[3], left_v, top_right_v,
                                         weights_y_v, weights_x_v[3],
                                         scaled_weights_x[3], weighted_bl));
      }
    }

    dst += stride;
  }
}

template <int width, int height>
inline void SmoothVertical4Or8xN_NEON(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const auto* const left = static_cast<const uint8_t*>(left_column);
  const uint8_t bottom_left = left[height - 1];
  const uint8_t* const weights_y = kSmoothWeights + height - 4;
  auto* dst = static_cast<uint8_t*>(dest);

  uint8x8_t top_v;
  if (width == 4) {
    top_v = Load4(top);
  } else {  // width == 8
    top_v = vld1_u8(top);
  }

  const uint8x8_t bottom_left_v = vdup_n_u8(bottom_left);

  for (int y = 0; y < height; ++y) {
    const uint8x8_t weights_y_v = vdup_n_u8(weights_y[y]);
    const uint8x8_t scaled_weights_y = vdup_n_u8(256 - weights_y[y]);

    const uint16x8_t weighted_top = vmull_u8(weights_y_v, top_v);
    const uint16x8_t weighted_bl = vmull_u8(scaled_weights_y, bottom_left_v);
    const uint16x8_t pred = vaddq_u16(weighted_top, weighted_bl);
    const uint8x8_t pred_scaled = vrshrn_n_u16(pred, kSmoothWeightScale);

    if (width == 4) {
      StoreLo4(dst, pred_scaled);
    } else {  // width == 8
      vst1_u8(dst, pred_scaled);
    }
    dst += stride;
  }
}

inline uint8x16_t CalculateVerticalWeightsAndPred(
    const uint8x16_t top, const uint8x8_t weights_y,
    const uint16x8_t weighted_bl) {
  const uint16x8_t weighted_top_low = vmull_u8(weights_y, vget_low_u8(top));
  const uint16x8_t weighted_top_high = vmull_u8(weights_y, vget_high_u8(top));
  const uint16x8_t pred_low = vaddq_u16(weighted_top_low, weighted_bl);
  const uint16x8_t pred_high = vaddq_u16(weighted_top_high, weighted_bl);
  const uint8x8_t pred_scaled_low = vrshrn_n_u16(pred_low, kSmoothWeightScale);
  const uint8x8_t pred_scaled_high =
      vrshrn_n_u16(pred_high, kSmoothWeightScale);
  return vcombine_u8(pred_scaled_low, pred_scaled_high);
}

template <int width, int height>
inline void SmoothVertical16PlusxN_NEON(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const auto* const left = static_cast<const uint8_t*>(left_column);
  const uint8_t bottom_left = left[height - 1];
  const uint8_t* const weights_y = kSmoothWeights + height - 4;
  auto* dst = static_cast<uint8_t*>(dest);

  uint8x16_t top_v[4];
  top_v[0] = vld1q_u8(top);
  if (width > 16) {
    top_v[1] = vld1q_u8(top + 16);
    if (width == 64) {
      top_v[2] = vld1q_u8(top + 32);
      top_v[3] = vld1q_u8(top + 48);
    }
  }

  const uint8x8_t bottom_left_v = vdup_n_u8(bottom_left);

  for (int y = 0; y < height; ++y) {
    const uint8x8_t weights_y_v = vdup_n_u8(weights_y[y]);
    const uint8x8_t scaled_weights_y = vdup_n_u8(256 - weights_y[y]);
    const uint16x8_t weighted_bl = vmull_u8(scaled_weights_y, bottom_left_v);

    const uint8x16_t pred_0 =
        CalculateVerticalWeightsAndPred(top_v[0], weights_y_v, weighted_bl);
    vst1q_u8(dst, pred_0);

    if (width > 16) {
      const uint8x16_t pred_1 =
          CalculateVerticalWeightsAndPred(top_v[1], weights_y_v, weighted_bl);
      vst1q_u8(dst + 16, pred_1);

      if (width == 64) {
        const uint8x16_t pred_2 =
            CalculateVerticalWeightsAndPred(top_v[2], weights_y_v, weighted_bl);
        vst1q_u8(dst + 32, pred_2);

        const uint8x16_t pred_3 =
            CalculateVerticalWeightsAndPred(top_v[3], weights_y_v, weighted_bl);
        vst1q_u8(dst + 48, pred_3);
      }
    }

    dst += stride;
  }
}

template <int width, int height>
inline void SmoothHorizontal4Or8xN_NEON(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const auto* const left = static_cast<const uint8_t*>(left_column);
  const uint8_t top_right = top[width - 1];
  auto* dst = static_cast<uint8_t*>(dest);

  const uint8x8_t top_right_v = vdup_n_u8(top_right);
  // Over-reads for 4xN but still within the array.
  const uint8x8_t weights_x = vld1_u8(kSmoothWeights + width - 4);
  // 256 - weights = vneg_s8(weights)
  const uint8x8_t scaled_weights_x =
      vreinterpret_u8_s8(vneg_s8(vreinterpret_s8_u8(weights_x)));

  for (int y = 0; y < height; ++y) {
    const uint8x8_t left_v = vdup_n_u8(left[y]);

    const uint16x8_t weighted_left = vmull_u8(weights_x, left_v);
    const uint16x8_t weighted_tr = vmull_u8(scaled_weights_x, top_right_v);
    const uint16x8_t pred = vaddq_u16(weighted_left, weighted_tr);
    const uint8x8_t pred_scaled = vrshrn_n_u16(pred, kSmoothWeightScale);

    if (width == 4) {
      StoreLo4(dst, pred_scaled);
    } else {  // width == 8
      vst1_u8(dst, pred_scaled);
    }
    dst += stride;
  }
}

inline uint8x16_t CalculateHorizontalWeightsAndPred(
    const uint8x8_t left, const uint8x8_t top_right, const uint8x16_t weights_x,
    const uint8x16_t scaled_weights_x) {
  const uint16x8_t weighted_left_low = vmull_u8(vget_low_u8(weights_x), left);
  const uint16x8_t weighted_tr_low =
      vmull_u8(vget_low_u8(scaled_weights_x), top_right);
  const uint16x8_t pred_low = vaddq_u16(weighted_left_low, weighted_tr_low);
  const uint8x8_t pred_scaled_low = vrshrn_n_u16(pred_low, kSmoothWeightScale);

  const uint16x8_t weighted_left_high = vmull_u8(vget_high_u8(weights_x), left);
  const uint16x8_t weighted_tr_high =
      vmull_u8(vget_high_u8(scaled_weights_x), top_right);
  const uint16x8_t pred_high = vaddq_u16(weighted_left_high, weighted_tr_high);
  const uint8x8_t pred_scaled_high =
      vrshrn_n_u16(pred_high, kSmoothWeightScale);

  return vcombine_u8(pred_scaled_low, pred_scaled_high);
}

template <int width, int height>
inline void SmoothHorizontal16PlusxN_NEON(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  const auto* const left = static_cast<const uint8_t*>(left_column);
  const uint8_t top_right = top[width - 1];
  auto* dst = static_cast<uint8_t*>(dest);

  const uint8x8_t top_right_v = vdup_n_u8(top_right);

  uint8x16_t weights_x[4];
  weights_x[0] = vld1q_u8(kSmoothWeights + width - 4);
  if (width > 16) {
    weights_x[1] = vld1q_u8(kSmoothWeights + width + 16 - 4);
    if (width == 64) {
      weights_x[2] = vld1q_u8(kSmoothWeights + width + 32 - 4);
      weights_x[3] = vld1q_u8(kSmoothWeights + width + 48 - 4);
    }
  }

  uint8x16_t scaled_weights_x[4];
  scaled_weights_x[0] =
      vreinterpretq_u8_s8(vnegq_s8(vreinterpretq_s8_u8(weights_x[0])));
  if (width > 16) {
    scaled_weights_x[1] =
        vreinterpretq_u8_s8(vnegq_s8(vreinterpretq_s8_u8(weights_x[1])));
    if (width == 64) {
      scaled_weights_x[2] =
          vreinterpretq_u8_s8(vnegq_s8(vreinterpretq_s8_u8(weights_x[2])));
      scaled_weights_x[3] =
          vreinterpretq_u8_s8(vnegq_s8(vreinterpretq_s8_u8(weights_x[3])));
    }
  }

  for (int y = 0; y < height; ++y) {
    const uint8x8_t left_v = vdup_n_u8(left[y]);

    const uint8x16_t pred_0 = CalculateHorizontalWeightsAndPred(
        left_v, top_right_v, weights_x[0], scaled_weights_x[0]);
    vst1q_u8(dst, pred_0);

    if (width > 16) {
      const uint8x16_t pred_1 = CalculateHorizontalWeightsAndPred(
          left_v, top_right_v, weights_x[1], scaled_weights_x[1]);
      vst1q_u8(dst + 16, pred_1);

      if (width == 64) {
        const uint8x16_t pred_2 = CalculateHorizontalWeightsAndPred(
            left_v, top_right_v, weights_x[2], scaled_weights_x[2]);
        vst1q_u8(dst + 32, pred_2);

        const uint8x16_t pred_3 = CalculateHorizontalWeightsAndPred(
            left_v, top_right_v, weights_x[3], scaled_weights_x[3]);
        vst1q_u8(dst + 48, pred_3);
      }
    }
    dst += stride;
  }
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
  // 4x4
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorSmooth] =
      Smooth4Or8xN_NEON<4, 4>;
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorSmoothVertical] =
      SmoothVertical4Or8xN_NEON<4, 4>;
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal4Or8xN_NEON<4, 4>;

  // 4x8
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorSmooth] =
      Smooth4Or8xN_NEON<4, 8>;
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorSmoothVertical] =
      SmoothVertical4Or8xN_NEON<4, 8>;
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal4Or8xN_NEON<4, 8>;

  // 4x16
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorSmooth] =
      Smooth4Or8xN_NEON<4, 16>;
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorSmoothVertical] =
      SmoothVertical4Or8xN_NEON<4, 16>;
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal4Or8xN_NEON<4, 16>;

  // 8x4
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorSmooth] =
      Smooth4Or8xN_NEON<8, 4>;
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorSmoothVertical] =
      SmoothVertical4Or8xN_NEON<8, 4>;
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal4Or8xN_NEON<8, 4>;

  // 8x8
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorSmooth] =
      Smooth4Or8xN_NEON<8, 8>;
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorSmoothVertical] =
      SmoothVertical4Or8xN_NEON<8, 8>;
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal4Or8xN_NEON<8, 8>;

  // 8x16
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorSmooth] =
      Smooth4Or8xN_NEON<8, 16>;
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorSmoothVertical] =
      SmoothVertical4Or8xN_NEON<8, 16>;
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal4Or8xN_NEON<8, 16>;

  // 8x32
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorSmooth] =
      Smooth4Or8xN_NEON<8, 32>;
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorSmoothVertical] =
      SmoothVertical4Or8xN_NEON<8, 32>;
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal4Or8xN_NEON<8, 32>;

  // 16x4
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorSmooth] =
      Smooth16PlusxN_NEON<16, 4>;
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorSmoothVertical] =
      SmoothVertical16PlusxN_NEON<16, 4>;
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal16PlusxN_NEON<16, 4>;

  // 16x8
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorSmooth] =
      Smooth16PlusxN_NEON<16, 8>;
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorSmoothVertical] =
      SmoothVertical16PlusxN_NEON<16, 8>;
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal16PlusxN_NEON<16, 8>;

  // 16x16
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorSmooth] =
      Smooth16PlusxN_NEON<16, 16>;
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorSmoothVertical] =
      SmoothVertical16PlusxN_NEON<16, 16>;
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal16PlusxN_NEON<16, 16>;

  // 16x32
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorSmooth] =
      Smooth16PlusxN_NEON<16, 32>;
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorSmoothVertical] =
      SmoothVertical16PlusxN_NEON<16, 32>;
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal16PlusxN_NEON<16, 32>;

  // 16x64
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorSmooth] =
      Smooth16PlusxN_NEON<16, 64>;
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorSmoothVertical] =
      SmoothVertical16PlusxN_NEON<16, 64>;
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal16PlusxN_NEON<16, 64>;

  // 32x8
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorSmooth] =
      Smooth16PlusxN_NEON<32, 8>;
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorSmoothVertical] =
      SmoothVertical16PlusxN_NEON<32, 8>;
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal16PlusxN_NEON<32, 8>;

  // 32x16
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorSmooth] =
      Smooth16PlusxN_NEON<32, 16>;
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorSmoothVertical] =
      SmoothVertical16PlusxN_NEON<32, 16>;
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal16PlusxN_NEON<32, 16>;

  // 32x32
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorSmooth] =
      Smooth16PlusxN_NEON<32, 32>;
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorSmoothVertical] =
      SmoothVertical16PlusxN_NEON<32, 32>;
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal16PlusxN_NEON<32, 32>;

  // 32x64
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorSmooth] =
      Smooth16PlusxN_NEON<32, 64>;
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorSmoothVertical] =
      SmoothVertical16PlusxN_NEON<32, 64>;
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal16PlusxN_NEON<32, 64>;

  // 64x16
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorSmooth] =
      Smooth16PlusxN_NEON<64, 16>;
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorSmoothVertical] =
      SmoothVertical16PlusxN_NEON<64, 16>;
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal16PlusxN_NEON<64, 16>;

  // 64x32
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorSmooth] =
      Smooth16PlusxN_NEON<64, 32>;
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorSmoothVertical] =
      SmoothVertical16PlusxN_NEON<64, 32>;
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal16PlusxN_NEON<64, 32>;

  // 64x64
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorSmooth] =
      Smooth16PlusxN_NEON<64, 64>;
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorSmoothVertical] =
      SmoothVertical16PlusxN_NEON<64, 64>;
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal16PlusxN_NEON<64, 64>;
}

}  // namespace
}  // namespace low_bitdepth

#if LIBGAV1_MAX_BITDEPTH >= 10
namespace high_bitdepth {
namespace {

// Note these constants are duplicated from intrapred.cc to allow the compiler
// to have visibility of the values. This helps reduce loads and in the
// creation of the inverse weights.
constexpr uint16_t kSmoothWeights[] = {
#include "src/dsp/smooth_weights.inc"
};

template <int height>
inline void Smooth4xH_NEON(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                           const void* LIBGAV1_RESTRICT const top_row,
                           const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint16_t*>(top_row);
  const auto* const left = static_cast<const uint16_t*>(left_column);
  const uint16_t top_right = top[3];
  const uint16_t bottom_left = left[height - 1];
  const uint16_t* const weights_y = kSmoothWeights + height - 4;
  auto* dst = static_cast<uint8_t*>(dest);

  const uint16x4_t top_v = vld1_u16(top);
  const uint16x4_t bottom_left_v = vdup_n_u16(bottom_left);
  const uint16x4_t weights_x_v = vld1_u16(kSmoothWeights);
  const uint16x4_t scaled_weights_x = vsub_u16(vdup_n_u16(256), weights_x_v);

  // Weighted top right doesn't change with each row.
  const uint32x4_t weighted_tr = vmull_n_u16(scaled_weights_x, top_right);

  for (int y = 0; y < height; ++y) {
    // Each variable in the running summation is named for the last item to be
    // accumulated.
    const uint32x4_t weighted_top =
        vmlal_n_u16(weighted_tr, top_v, weights_y[y]);
    const uint32x4_t weighted_left =
        vmlal_n_u16(weighted_top, weights_x_v, left[y]);
    const uint32x4_t weighted_bl =
        vmlal_n_u16(weighted_left, bottom_left_v, 256 - weights_y[y]);

    const uint16x4_t pred = vrshrn_n_u32(weighted_bl, kSmoothWeightScale + 1);
    vst1_u16(reinterpret_cast<uint16_t*>(dst), pred);
    dst += stride;
  }
}

// Common code between 8xH and [16|32|64]xH.
inline void CalculatePred8(uint16_t* LIBGAV1_RESTRICT dst,
                           const uint32x4_t& weighted_corners_low,
                           const uint32x4_t& weighted_corners_high,
                           const uint16x4x2_t& top_vals,
                           const uint16x4x2_t& weights_x, const uint16_t left_y,
                           const uint16_t weight_y) {
  // Each variable in the running summation is named for the last item to be
  // accumulated.
  const uint32x4_t weighted_top_low =
      vmlal_n_u16(weighted_corners_low, top_vals.val[0], weight_y);
  const uint32x4_t weighted_edges_low =
      vmlal_n_u16(weighted_top_low, weights_x.val[0], left_y);

  const uint16x4_t pred_low =
      vrshrn_n_u32(weighted_edges_low, kSmoothWeightScale + 1);
  vst1_u16(dst, pred_low);

  const uint32x4_t weighted_top_high =
      vmlal_n_u16(weighted_corners_high, top_vals.val[1], weight_y);
  const uint32x4_t weighted_edges_high =
      vmlal_n_u16(weighted_top_high, weights_x.val[1], left_y);

  const uint16x4_t pred_high =
      vrshrn_n_u32(weighted_edges_high, kSmoothWeightScale + 1);
  vst1_u16(dst + 4, pred_high);
}

template <int height>
inline void Smooth8xH_NEON(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                           const void* LIBGAV1_RESTRICT const top_row,
                           const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint16_t*>(top_row);
  const auto* const left = static_cast<const uint16_t*>(left_column);
  const uint16_t top_right = top[7];
  const uint16_t bottom_left = left[height - 1];
  const uint16_t* const weights_y = kSmoothWeights + height - 4;

  auto* dst = static_cast<uint8_t*>(dest);

  const uint16x4x2_t top_vals = {vld1_u16(top), vld1_u16(top + 4)};
  const uint16x4_t bottom_left_v = vdup_n_u16(bottom_left);
  const uint16x4x2_t weights_x = {vld1_u16(kSmoothWeights + 4),
                                  vld1_u16(kSmoothWeights + 8)};
  // Weighted top right doesn't change with each row.
  const uint32x4_t weighted_tr_low =
      vmull_n_u16(vsub_u16(vdup_n_u16(256), weights_x.val[0]), top_right);
  const uint32x4_t weighted_tr_high =
      vmull_n_u16(vsub_u16(vdup_n_u16(256), weights_x.val[1]), top_right);

  for (int y = 0; y < height; ++y) {
    // |weighted_bl| is invariant across the row.
    const uint32x4_t weighted_bl =
        vmull_n_u16(bottom_left_v, 256 - weights_y[y]);
    const uint32x4_t weighted_corners_low =
        vaddq_u32(weighted_bl, weighted_tr_low);
    const uint32x4_t weighted_corners_high =
        vaddq_u32(weighted_bl, weighted_tr_high);
    CalculatePred8(reinterpret_cast<uint16_t*>(dst), weighted_corners_low,
                   weighted_corners_high, top_vals, weights_x, left[y],
                   weights_y[y]);
    dst += stride;
  }
}

// For width 16 and above.
template <int width, int height>
inline void SmoothWxH_NEON(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                           const void* LIBGAV1_RESTRICT const top_row,
                           const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint16_t*>(top_row);
  const auto* const left = static_cast<const uint16_t*>(left_column);
  const uint16_t top_right = top[width - 1];
  const uint16_t bottom_left = left[height - 1];
  const uint16_t* const weights_y = kSmoothWeights + height - 4;

  auto* dst = static_cast<uint8_t*>(dest);

  const uint16x4_t weight_scaling = vdup_n_u16(256);
  // Precompute weighted values that don't vary with |y|.
  uint32x4_t weighted_tr_low[width >> 3];
  uint32x4_t weighted_tr_high[width >> 3];
  for (int i = 0; i < width >> 3; ++i) {
    const int x = i << 3;
    const uint16x4_t weights_x_low = vld1_u16(kSmoothWeights + width - 4 + x);
    weighted_tr_low[i] =
        vmull_n_u16(vsub_u16(weight_scaling, weights_x_low), top_right);
    const uint16x4_t weights_x_high = vld1_u16(kSmoothWeights + width + x);
    weighted_tr_high[i] =
        vmull_n_u16(vsub_u16(weight_scaling, weights_x_high), top_right);
  }

  const uint16x4_t bottom_left_v = vdup_n_u16(bottom_left);
  for (int y = 0; y < height; ++y) {
    // |weighted_bl| is invariant across the row.
    const uint32x4_t weighted_bl =
        vmull_n_u16(bottom_left_v, 256 - weights_y[y]);
    auto* dst_x = reinterpret_cast<uint16_t*>(dst);
    for (int i = 0; i < width >> 3; ++i) {
      const int x = i << 3;
      const uint16x4x2_t top_vals = {vld1_u16(top + x), vld1_u16(top + x + 4)};
      const uint32x4_t weighted_corners_low =
          vaddq_u32(weighted_bl, weighted_tr_low[i]);
      const uint32x4_t weighted_corners_high =
          vaddq_u32(weighted_bl, weighted_tr_high[i]);
      // Accumulate weighted edge values and store.
      const uint16x4x2_t weights_x = {vld1_u16(kSmoothWeights + width - 4 + x),
                                      vld1_u16(kSmoothWeights + width + x)};
      CalculatePred8(dst_x, weighted_corners_low, weighted_corners_high,
                     top_vals, weights_x, left[y], weights_y[y]);
      dst_x += 8;
    }
    dst += stride;
  }
}

template <int height>
inline void SmoothVertical4xH_NEON(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint16_t*>(top_row);
  const auto* const left = static_cast<const uint16_t*>(left_column);
  const uint16_t bottom_left = left[height - 1];
  const uint16_t* const weights_y = kSmoothWeights + height - 4;

  auto* dst = static_cast<uint8_t*>(dest);

  const uint16x4_t top_v = vld1_u16(top);
  const uint16x4_t bottom_left_v = vdup_n_u16(bottom_left);

  for (int y = 0; y < height; ++y) {
    auto* dst16 = reinterpret_cast<uint16_t*>(dst);
    const uint32x4_t weighted_bl =
        vmull_n_u16(bottom_left_v, 256 - weights_y[y]);
    const uint32x4_t weighted_top =
        vmlal_n_u16(weighted_bl, top_v, weights_y[y]);
    vst1_u16(dst16, vrshrn_n_u32(weighted_top, kSmoothWeightScale));

    dst += stride;
  }
}

template <int height>
inline void SmoothVertical8xH_NEON(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint16_t*>(top_row);
  const auto* const left = static_cast<const uint16_t*>(left_column);
  const uint16_t bottom_left = left[height - 1];
  const uint16_t* const weights_y = kSmoothWeights + height - 4;

  auto* dst = static_cast<uint8_t*>(dest);

  const uint16x4_t top_low = vld1_u16(top);
  const uint16x4_t top_high = vld1_u16(top + 4);
  const uint16x4_t bottom_left_v = vdup_n_u16(bottom_left);

  for (int y = 0; y < height; ++y) {
    auto* dst16 = reinterpret_cast<uint16_t*>(dst);
    // |weighted_bl| is invariant across the row.
    const uint32x4_t weighted_bl =
        vmull_n_u16(bottom_left_v, 256 - weights_y[y]);

    const uint32x4_t weighted_top_low =
        vmlal_n_u16(weighted_bl, top_low, weights_y[y]);
    vst1_u16(dst16, vrshrn_n_u32(weighted_top_low, kSmoothWeightScale));

    const uint32x4_t weighted_top_high =
        vmlal_n_u16(weighted_bl, top_high, weights_y[y]);
    vst1_u16(dst16 + 4, vrshrn_n_u32(weighted_top_high, kSmoothWeightScale));
    dst += stride;
  }
}

// For width 16 and above.
template <int width, int height>
inline void SmoothVerticalWxH_NEON(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint16_t*>(top_row);
  const auto* const left = static_cast<const uint16_t*>(left_column);
  const uint16_t bottom_left = left[height - 1];
  const uint16_t* const weights_y = kSmoothWeights + height - 4;

  auto* dst = static_cast<uint8_t*>(dest);

  uint16x4x2_t top_vals[width >> 3];
  for (int i = 0; i < width >> 3; ++i) {
    const int x = i << 3;
    top_vals[i] = {vld1_u16(top + x), vld1_u16(top + x + 4)};
  }

  const uint16x4_t bottom_left_v = vdup_n_u16(bottom_left);
  for (int y = 0; y < height; ++y) {
    // |weighted_bl| is invariant across the row.
    const uint32x4_t weighted_bl =
        vmull_n_u16(bottom_left_v, 256 - weights_y[y]);

    auto* dst_x = reinterpret_cast<uint16_t*>(dst);
    for (int i = 0; i < width >> 3; ++i) {
      const uint32x4_t weighted_top_low =
          vmlal_n_u16(weighted_bl, top_vals[i].val[0], weights_y[y]);
      vst1_u16(dst_x, vrshrn_n_u32(weighted_top_low, kSmoothWeightScale));

      const uint32x4_t weighted_top_high =
          vmlal_n_u16(weighted_bl, top_vals[i].val[1], weights_y[y]);
      vst1_u16(dst_x + 4, vrshrn_n_u32(weighted_top_high, kSmoothWeightScale));
      dst_x += 8;
    }
    dst += stride;
  }
}

template <int height>
inline void SmoothHorizontal4xH_NEON(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint16_t*>(top_row);
  const auto* const left = static_cast<const uint16_t*>(left_column);
  const uint16_t top_right = top[3];

  auto* dst = static_cast<uint8_t*>(dest);

  const uint16x4_t weights_x = vld1_u16(kSmoothWeights);
  const uint16x4_t scaled_weights_x = vsub_u16(vdup_n_u16(256), weights_x);

  const uint32x4_t weighted_tr = vmull_n_u16(scaled_weights_x, top_right);
  for (int y = 0; y < height; ++y) {
    auto* dst16 = reinterpret_cast<uint16_t*>(dst);
    const uint32x4_t weighted_left =
        vmlal_n_u16(weighted_tr, weights_x, left[y]);
    vst1_u16(dst16, vrshrn_n_u32(weighted_left, kSmoothWeightScale));
    dst += stride;
  }
}

template <int height>
inline void SmoothHorizontal8xH_NEON(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint16_t*>(top_row);
  const auto* const left = static_cast<const uint16_t*>(left_column);
  const uint16_t top_right = top[7];

  auto* dst = static_cast<uint8_t*>(dest);

  const uint16x4x2_t weights_x = {vld1_u16(kSmoothWeights + 4),
                                  vld1_u16(kSmoothWeights + 8)};

  const uint32x4_t weighted_tr_low =
      vmull_n_u16(vsub_u16(vdup_n_u16(256), weights_x.val[0]), top_right);
  const uint32x4_t weighted_tr_high =
      vmull_n_u16(vsub_u16(vdup_n_u16(256), weights_x.val[1]), top_right);

  for (int y = 0; y < height; ++y) {
    auto* dst16 = reinterpret_cast<uint16_t*>(dst);
    const uint16_t left_y = left[y];
    const uint32x4_t weighted_left_low =
        vmlal_n_u16(weighted_tr_low, weights_x.val[0], left_y);
    vst1_u16(dst16, vrshrn_n_u32(weighted_left_low, kSmoothWeightScale));

    const uint32x4_t weighted_left_high =
        vmlal_n_u16(weighted_tr_high, weights_x.val[1], left_y);
    vst1_u16(dst16 + 4, vrshrn_n_u32(weighted_left_high, kSmoothWeightScale));
    dst += stride;
  }
}

// For width 16 and above.
template <int width, int height>
inline void SmoothHorizontalWxH_NEON(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const top = static_cast<const uint16_t*>(top_row);
  const auto* const left = static_cast<const uint16_t*>(left_column);
  const uint16_t top_right = top[width - 1];

  auto* dst = static_cast<uint8_t*>(dest);

  const uint16x4_t weight_scaling = vdup_n_u16(256);

  uint16x4_t weights_x_low[width >> 3];
  uint16x4_t weights_x_high[width >> 3];
  uint32x4_t weighted_tr_low[width >> 3];
  uint32x4_t weighted_tr_high[width >> 3];
  for (int i = 0; i < width >> 3; ++i) {
    const int x = i << 3;
    weights_x_low[i] = vld1_u16(kSmoothWeights + width - 4 + x);
    weighted_tr_low[i] =
        vmull_n_u16(vsub_u16(weight_scaling, weights_x_low[i]), top_right);
    weights_x_high[i] = vld1_u16(kSmoothWeights + width + x);
    weighted_tr_high[i] =
        vmull_n_u16(vsub_u16(weight_scaling, weights_x_high[i]), top_right);
  }

  for (int y = 0; y < height; ++y) {
    auto* dst_x = reinterpret_cast<uint16_t*>(dst);
    const uint16_t left_y = left[y];
    for (int i = 0; i < width >> 3; ++i) {
      const uint32x4_t weighted_left_low =
          vmlal_n_u16(weighted_tr_low[i], weights_x_low[i], left_y);
      vst1_u16(dst_x, vrshrn_n_u32(weighted_left_low, kSmoothWeightScale));

      const uint32x4_t weighted_left_high =
          vmlal_n_u16(weighted_tr_high[i], weights_x_high[i], left_y);
      vst1_u16(dst_x + 4, vrshrn_n_u32(weighted_left_high, kSmoothWeightScale));
      dst_x += 8;
    }
    dst += stride;
  }
}

void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth10);
  assert(dsp != nullptr);
  // 4x4
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorSmooth] =
      Smooth4xH_NEON<4>;
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorSmoothVertical] =
      SmoothVertical4xH_NEON<4>;
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal4xH_NEON<4>;

  // 4x8
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorSmooth] =
      Smooth4xH_NEON<8>;
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorSmoothVertical] =
      SmoothVertical4xH_NEON<8>;
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal4xH_NEON<8>;

  // 4x16
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorSmooth] =
      Smooth4xH_NEON<16>;
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorSmoothVertical] =
      SmoothVertical4xH_NEON<16>;
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal4xH_NEON<16>;

  // 8x4
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorSmooth] =
      Smooth8xH_NEON<4>;
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorSmoothVertical] =
      SmoothVertical8xH_NEON<4>;
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal8xH_NEON<4>;

  // 8x8
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorSmooth] =
      Smooth8xH_NEON<8>;
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorSmoothVertical] =
      SmoothVertical8xH_NEON<8>;
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal8xH_NEON<8>;

  // 8x16
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorSmooth] =
      Smooth8xH_NEON<16>;
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorSmoothVertical] =
      SmoothVertical8xH_NEON<16>;
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal8xH_NEON<16>;

  // 8x32
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorSmooth] =
      Smooth8xH_NEON<32>;
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorSmoothVertical] =
      SmoothVertical8xH_NEON<32>;
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontal8xH_NEON<32>;

  // 16x4
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorSmooth] =
      SmoothWxH_NEON<16, 4>;
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorSmoothVertical] =
      SmoothVerticalWxH_NEON<16, 4>;
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontalWxH_NEON<16, 4>;

  // 16x8
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorSmooth] =
      SmoothWxH_NEON<16, 8>;
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorSmoothVertical] =
      SmoothVerticalWxH_NEON<16, 8>;
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontalWxH_NEON<16, 8>;

  // 16x16
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorSmooth] =
      SmoothWxH_NEON<16, 16>;
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorSmoothVertical] =
      SmoothVerticalWxH_NEON<16, 16>;
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontalWxH_NEON<16, 16>;

  // 16x32
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorSmooth] =
      SmoothWxH_NEON<16, 32>;
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorSmoothVertical] =
      SmoothVerticalWxH_NEON<16, 32>;
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontalWxH_NEON<16, 32>;

  // 16x64
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorSmooth] =
      SmoothWxH_NEON<16, 64>;
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorSmoothVertical] =
      SmoothVerticalWxH_NEON<16, 64>;
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontalWxH_NEON<16, 64>;

  // 32x8
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorSmooth] =
      SmoothWxH_NEON<32, 8>;
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorSmoothVertical] =
      SmoothVerticalWxH_NEON<32, 8>;
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontalWxH_NEON<32, 8>;

  // 32x16
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorSmooth] =
      SmoothWxH_NEON<32, 16>;
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorSmoothVertical] =
      SmoothVerticalWxH_NEON<32, 16>;
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontalWxH_NEON<32, 16>;

  // 32x32
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorSmooth] =
      SmoothWxH_NEON<32, 32>;
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorSmoothVertical] =
      SmoothVerticalWxH_NEON<32, 32>;
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontalWxH_NEON<32, 32>;

  // 32x64
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorSmooth] =
      SmoothWxH_NEON<32, 64>;
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorSmoothVertical] =
      SmoothVerticalWxH_NEON<32, 64>;
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontalWxH_NEON<32, 64>;

  // 64x16
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorSmooth] =
      SmoothWxH_NEON<64, 16>;
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorSmoothVertical] =
      SmoothVerticalWxH_NEON<64, 16>;
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontalWxH_NEON<64, 16>;

  // 64x32
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorSmooth] =
      SmoothWxH_NEON<64, 32>;
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorSmoothVertical] =
      SmoothVerticalWxH_NEON<64, 32>;
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontalWxH_NEON<64, 32>;

  // 64x64
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorSmooth] =
      SmoothWxH_NEON<64, 64>;
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorSmoothVertical] =
      SmoothVerticalWxH_NEON<64, 64>;
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorSmoothHorizontal] =
      SmoothHorizontalWxH_NEON<64, 64>;
}
}  // namespace
}  // namespace high_bitdepth
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

void IntraPredSmoothInit_NEON() {
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

void IntraPredSmoothInit_NEON() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_ENABLE_NEON
