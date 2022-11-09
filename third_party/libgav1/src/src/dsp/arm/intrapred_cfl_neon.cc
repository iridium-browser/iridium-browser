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

#include "src/dsp/intrapred_cfl.h"
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

// Divide by the number of elements.
inline uint32_t Average(const uint32_t sum, const int width, const int height) {
  return RightShiftWithRounding(sum, FloorLog2(width) + FloorLog2(height));
}

// Subtract |val| from every element in |a|.
inline void BlockSubtract(const uint32_t val,
                          int16_t a[kCflLumaBufferStride][kCflLumaBufferStride],
                          const int width, const int height) {
  assert(val <= INT16_MAX);
  const int16x8_t val_v = vdupq_n_s16(static_cast<int16_t>(val));

  for (int y = 0; y < height; ++y) {
    if (width == 4) {
      const int16x4_t b = vld1_s16(a[y]);
      vst1_s16(a[y], vsub_s16(b, vget_low_s16(val_v)));
    } else if (width == 8) {
      const int16x8_t b = vld1q_s16(a[y]);
      vst1q_s16(a[y], vsubq_s16(b, val_v));
    } else if (width == 16) {
      const int16x8_t b = vld1q_s16(a[y]);
      const int16x8_t c = vld1q_s16(a[y] + 8);
      vst1q_s16(a[y], vsubq_s16(b, val_v));
      vst1q_s16(a[y] + 8, vsubq_s16(c, val_v));
    } else /* block_width == 32 */ {
      const int16x8_t b = vld1q_s16(a[y]);
      const int16x8_t c = vld1q_s16(a[y] + 8);
      const int16x8_t d = vld1q_s16(a[y] + 16);
      const int16x8_t e = vld1q_s16(a[y] + 24);
      vst1q_s16(a[y], vsubq_s16(b, val_v));
      vst1q_s16(a[y] + 8, vsubq_s16(c, val_v));
      vst1q_s16(a[y] + 16, vsubq_s16(d, val_v));
      vst1q_s16(a[y] + 24, vsubq_s16(e, val_v));
    }
  }
}

namespace low_bitdepth {
namespace {

template <int block_width, int block_height>
void CflSubsampler420_NEON(
    int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
    const int max_luma_width, const int max_luma_height,
    const void* LIBGAV1_RESTRICT const source, const ptrdiff_t stride) {
  const auto* src = static_cast<const uint8_t*>(source);
  uint32_t sum;
  if (block_width == 4) {
    assert(max_luma_width >= 8);
    uint32x2_t running_sum = vdup_n_u32(0);

    for (int y = 0; y < block_height; ++y) {
      const uint8x8_t row0 = vld1_u8(src);
      const uint8x8_t row1 = vld1_u8(src + stride);

      uint16x4_t sum_row = vpadal_u8(vpaddl_u8(row0), row1);
      sum_row = vshl_n_u16(sum_row, 1);
      running_sum = vpadal_u16(running_sum, sum_row);
      vst1_s16(luma[y], vreinterpret_s16_u16(sum_row));

      if (y << 1 < max_luma_height - 2) {
        // Once this threshold is reached the loop could be simplified.
        src += stride << 1;
      }
    }

    sum = SumVector(running_sum);
  } else if (block_width == 8) {
    const uint16x8_t x_index = {0, 2, 4, 6, 8, 10, 12, 14};
    const uint16x8_t x_max_index =
        vdupq_n_u16(max_luma_width == 8 ? max_luma_width - 2 : 16);
    const uint16x8_t x_mask = vcltq_u16(x_index, x_max_index);

    uint32x4_t running_sum = vdupq_n_u32(0);

    for (int y = 0; y < block_height; ++y) {
      const uint8x16_t row0 = vld1q_u8(src);
      const uint8x16_t row1 = vld1q_u8(src + stride);
      const uint16x8_t sum_row = vpadalq_u8(vpaddlq_u8(row0), row1);
      const uint16x8_t sum_row_shifted = vshlq_n_u16(sum_row, 1);

      // Dup the 2x2 sum at the max luma offset.
      const uint16x8_t max_luma_sum =
          vdupq_lane_u16(vget_low_u16(sum_row_shifted), 3);
      const uint16x8_t final_sum_row =
          vbslq_u16(x_mask, sum_row_shifted, max_luma_sum);
      vst1q_s16(luma[y], vreinterpretq_s16_u16(final_sum_row));

      running_sum = vpadalq_u16(running_sum, final_sum_row);

      if (y << 1 < max_luma_height - 2) {
        src += stride << 1;
      }
    }

    sum = SumVector(running_sum);
  } else /* block_width >= 16 */ {
    const uint16x8_t x_max_index = vdupq_n_u16(max_luma_width - 2);
    uint32x4_t running_sum = vdupq_n_u32(0);

    for (int y = 0; y < block_height; ++y) {
      // Calculate the 2x2 sum at the max_luma offset
      const uint8_t a00 = src[max_luma_width - 2];
      const uint8_t a01 = src[max_luma_width - 1];
      const uint8_t a10 = src[max_luma_width - 2 + stride];
      const uint8_t a11 = src[max_luma_width - 1 + stride];
      // Dup the 2x2 sum at the max luma offset.
      const uint16x8_t max_luma_sum =
          vdupq_n_u16(static_cast<uint16_t>((a00 + a01 + a10 + a11) << 1));
      uint16x8_t x_index = {0, 2, 4, 6, 8, 10, 12, 14};

      ptrdiff_t src_x_offset = 0;
      for (int x = 0; x < block_width; x += 8, src_x_offset += 16) {
        const uint16x8_t x_mask = vcltq_u16(x_index, x_max_index);
        const uint8x16_t row0 = vld1q_u8(src + src_x_offset);
        const uint8x16_t row1 = vld1q_u8(src + src_x_offset + stride);
        const uint16x8_t sum_row = vpadalq_u8(vpaddlq_u8(row0), row1);
        const uint16x8_t sum_row_shifted = vshlq_n_u16(sum_row, 1);
        const uint16x8_t final_sum_row =
            vbslq_u16(x_mask, sum_row_shifted, max_luma_sum);
        vst1q_s16(luma[y] + x, vreinterpretq_s16_u16(final_sum_row));

        running_sum = vpadalq_u16(running_sum, final_sum_row);
        x_index = vaddq_u16(x_index, vdupq_n_u16(16));
      }

      if (y << 1 < max_luma_height - 2) {
        src += stride << 1;
      }
    }
    sum = SumVector(running_sum);
  }

  const uint32_t average = Average(sum, block_width, block_height);
  BlockSubtract(average, luma, block_width, block_height);
}

template <int block_width, int block_height>
void CflSubsampler444_NEON(
    int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
    const int max_luma_width, const int max_luma_height,
    const void* LIBGAV1_RESTRICT const source, const ptrdiff_t stride) {
  const auto* src = static_cast<const uint8_t*>(source);
  uint32_t sum;
  if (block_width == 4) {
    assert(max_luma_width >= 4);
    assert(max_luma_height <= block_height);
    assert((max_luma_height % 2) == 0);
    uint32x4_t running_sum = vdupq_n_u32(0);
    uint8x8_t row = vdup_n_u8(0);

    uint16x8_t row_shifted;
    int y = 0;
    do {
      row = Load4<0>(src, row);
      row = Load4<1>(src + stride, row);
      if (y < (max_luma_height - 1)) {
        src += stride << 1;
      }

      row_shifted = vshll_n_u8(row, 3);
      running_sum = vpadalq_u16(running_sum, row_shifted);
      vst1_s16(luma[y], vreinterpret_s16_u16(vget_low_u16(row_shifted)));
      vst1_s16(luma[y + 1], vreinterpret_s16_u16(vget_high_u16(row_shifted)));
      y += 2;
    } while (y < max_luma_height);

    row_shifted =
        vcombine_u16(vget_high_u16(row_shifted), vget_high_u16(row_shifted));
    for (; y < block_height; y += 2) {
      running_sum = vpadalq_u16(running_sum, row_shifted);
      vst1_s16(luma[y], vreinterpret_s16_u16(vget_low_u16(row_shifted)));
      vst1_s16(luma[y + 1], vreinterpret_s16_u16(vget_high_u16(row_shifted)));
    }

    sum = SumVector(running_sum);
  } else if (block_width == 8) {
    const uint8x8_t x_index = {0, 1, 2, 3, 4, 5, 6, 7};
    const uint8x8_t x_max_index = vdup_n_u8(max_luma_width - 1);
    const uint8x8_t x_mask = vclt_u8(x_index, x_max_index);

    uint32x4_t running_sum = vdupq_n_u32(0);

    for (int y = 0; y < block_height; ++y) {
      const uint8x8_t x_max = vdup_n_u8(src[max_luma_width - 1]);
      const uint8x8_t row = vbsl_u8(x_mask, vld1_u8(src), x_max);

      const uint16x8_t row_shifted = vshll_n_u8(row, 3);
      running_sum = vpadalq_u16(running_sum, row_shifted);
      vst1q_s16(luma[y], vreinterpretq_s16_u16(row_shifted));

      if (y < max_luma_height - 1) {
        src += stride;
      }
    }

    sum = SumVector(running_sum);
  } else /* block_width >= 16 */ {
    const uint8x16_t x_max_index = vdupq_n_u8(max_luma_width - 1);
    uint32x4_t running_sum = vdupq_n_u32(0);

    for (int y = 0; y < block_height; ++y) {
      uint8x16_t x_index = {0, 1, 2,  3,  4,  5,  6,  7,
                            8, 9, 10, 11, 12, 13, 14, 15};
      const uint8x16_t x_max = vdupq_n_u8(src[max_luma_width - 1]);
      for (int x = 0; x < block_width; x += 16) {
        const uint8x16_t x_mask = vcltq_u8(x_index, x_max_index);
        const uint8x16_t row = vbslq_u8(x_mask, vld1q_u8(src + x), x_max);

        const uint16x8_t row_shifted_low = vshll_n_u8(vget_low_u8(row), 3);
        const uint16x8_t row_shifted_high = vshll_n_u8(vget_high_u8(row), 3);
        running_sum = vpadalq_u16(running_sum, row_shifted_low);
        running_sum = vpadalq_u16(running_sum, row_shifted_high);
        vst1q_s16(luma[y] + x, vreinterpretq_s16_u16(row_shifted_low));
        vst1q_s16(luma[y] + x + 8, vreinterpretq_s16_u16(row_shifted_high));

        x_index = vaddq_u8(x_index, vdupq_n_u8(16));
      }
      if (y < max_luma_height - 1) {
        src += stride;
      }
    }
    sum = SumVector(running_sum);
  }

  const uint32_t average = Average(sum, block_width, block_height);
  BlockSubtract(average, luma, block_width, block_height);
}

// Saturate |dc + ((alpha * luma) >> 6))| to uint8_t.
inline uint8x8_t Combine8(const int16x8_t luma, const int alpha,
                          const int16x8_t dc) {
  const int16x8_t la = vmulq_n_s16(luma, alpha);
  // Subtract the sign bit to round towards zero.
  const int16x8_t sub_sign = vsraq_n_s16(la, la, 15);
  // Shift and accumulate.
  const int16x8_t result = vrsraq_n_s16(dc, sub_sign, 6);
  return vqmovun_s16(result);
}

// The range of luma/alpha is not really important because it gets saturated to
// uint8_t. Saturated int16_t >> 6 outranges uint8_t.
template <int block_height>
inline void CflIntraPredictor4xN_NEON(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
    const int alpha) {
  auto* dst = static_cast<uint8_t*>(dest);
  const int16x8_t dc = vdupq_n_s16(dst[0]);
  for (int y = 0; y < block_height; y += 2) {
    const int16x4_t luma_row0 = vld1_s16(luma[y]);
    const int16x4_t luma_row1 = vld1_s16(luma[y + 1]);
    const uint8x8_t sum =
        Combine8(vcombine_s16(luma_row0, luma_row1), alpha, dc);
    StoreLo4(dst, sum);
    dst += stride;
    StoreHi4(dst, sum);
    dst += stride;
  }
}

template <int block_height>
inline void CflIntraPredictor8xN_NEON(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
    const int alpha) {
  auto* dst = static_cast<uint8_t*>(dest);
  const int16x8_t dc = vdupq_n_s16(dst[0]);
  for (int y = 0; y < block_height; ++y) {
    const int16x8_t luma_row = vld1q_s16(luma[y]);
    const uint8x8_t sum = Combine8(luma_row, alpha, dc);
    vst1_u8(dst, sum);
    dst += stride;
  }
}

template <int block_height>
inline void CflIntraPredictor16xN_NEON(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
    const int alpha) {
  auto* dst = static_cast<uint8_t*>(dest);
  const int16x8_t dc = vdupq_n_s16(dst[0]);
  for (int y = 0; y < block_height; ++y) {
    const int16x8_t luma_row_0 = vld1q_s16(luma[y]);
    const int16x8_t luma_row_1 = vld1q_s16(luma[y] + 8);
    const uint8x8_t sum_0 = Combine8(luma_row_0, alpha, dc);
    const uint8x8_t sum_1 = Combine8(luma_row_1, alpha, dc);
    vst1_u8(dst, sum_0);
    vst1_u8(dst + 8, sum_1);
    dst += stride;
  }
}

template <int block_height>
inline void CflIntraPredictor32xN_NEON(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
    const int alpha) {
  auto* dst = static_cast<uint8_t*>(dest);
  const int16x8_t dc = vdupq_n_s16(dst[0]);
  for (int y = 0; y < block_height; ++y) {
    const int16x8_t luma_row_0 = vld1q_s16(luma[y]);
    const int16x8_t luma_row_1 = vld1q_s16(luma[y] + 8);
    const int16x8_t luma_row_2 = vld1q_s16(luma[y] + 16);
    const int16x8_t luma_row_3 = vld1q_s16(luma[y] + 24);
    const uint8x8_t sum_0 = Combine8(luma_row_0, alpha, dc);
    const uint8x8_t sum_1 = Combine8(luma_row_1, alpha, dc);
    const uint8x8_t sum_2 = Combine8(luma_row_2, alpha, dc);
    const uint8x8_t sum_3 = Combine8(luma_row_3, alpha, dc);
    vst1_u8(dst, sum_0);
    vst1_u8(dst + 8, sum_1);
    vst1_u8(dst + 16, sum_2);
    vst1_u8(dst + 24, sum_3);
    dst += stride;
  }
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);

  dsp->cfl_subsamplers[kTransformSize4x4][kSubsamplingType420] =
      CflSubsampler420_NEON<4, 4>;
  dsp->cfl_subsamplers[kTransformSize4x8][kSubsamplingType420] =
      CflSubsampler420_NEON<4, 8>;
  dsp->cfl_subsamplers[kTransformSize4x16][kSubsamplingType420] =
      CflSubsampler420_NEON<4, 16>;

  dsp->cfl_subsamplers[kTransformSize8x4][kSubsamplingType420] =
      CflSubsampler420_NEON<8, 4>;
  dsp->cfl_subsamplers[kTransformSize8x8][kSubsamplingType420] =
      CflSubsampler420_NEON<8, 8>;
  dsp->cfl_subsamplers[kTransformSize8x16][kSubsamplingType420] =
      CflSubsampler420_NEON<8, 16>;
  dsp->cfl_subsamplers[kTransformSize8x32][kSubsamplingType420] =
      CflSubsampler420_NEON<8, 32>;

  dsp->cfl_subsamplers[kTransformSize16x4][kSubsamplingType420] =
      CflSubsampler420_NEON<16, 4>;
  dsp->cfl_subsamplers[kTransformSize16x8][kSubsamplingType420] =
      CflSubsampler420_NEON<16, 8>;
  dsp->cfl_subsamplers[kTransformSize16x16][kSubsamplingType420] =
      CflSubsampler420_NEON<16, 16>;
  dsp->cfl_subsamplers[kTransformSize16x32][kSubsamplingType420] =
      CflSubsampler420_NEON<16, 32>;

  dsp->cfl_subsamplers[kTransformSize32x8][kSubsamplingType420] =
      CflSubsampler420_NEON<32, 8>;
  dsp->cfl_subsamplers[kTransformSize32x16][kSubsamplingType420] =
      CflSubsampler420_NEON<32, 16>;
  dsp->cfl_subsamplers[kTransformSize32x32][kSubsamplingType420] =
      CflSubsampler420_NEON<32, 32>;

  dsp->cfl_subsamplers[kTransformSize4x4][kSubsamplingType444] =
      CflSubsampler444_NEON<4, 4>;
  dsp->cfl_subsamplers[kTransformSize4x8][kSubsamplingType444] =
      CflSubsampler444_NEON<4, 8>;
  dsp->cfl_subsamplers[kTransformSize4x16][kSubsamplingType444] =
      CflSubsampler444_NEON<4, 16>;

  dsp->cfl_subsamplers[kTransformSize8x4][kSubsamplingType444] =
      CflSubsampler444_NEON<8, 4>;
  dsp->cfl_subsamplers[kTransformSize8x8][kSubsamplingType444] =
      CflSubsampler444_NEON<8, 8>;
  dsp->cfl_subsamplers[kTransformSize8x16][kSubsamplingType444] =
      CflSubsampler444_NEON<8, 16>;
  dsp->cfl_subsamplers[kTransformSize8x32][kSubsamplingType444] =
      CflSubsampler444_NEON<8, 32>;

  dsp->cfl_subsamplers[kTransformSize16x4][kSubsamplingType444] =
      CflSubsampler444_NEON<16, 4>;
  dsp->cfl_subsamplers[kTransformSize16x8][kSubsamplingType444] =
      CflSubsampler444_NEON<16, 8>;
  dsp->cfl_subsamplers[kTransformSize16x16][kSubsamplingType444] =
      CflSubsampler444_NEON<16, 16>;
  dsp->cfl_subsamplers[kTransformSize16x32][kSubsamplingType444] =
      CflSubsampler444_NEON<16, 32>;

  dsp->cfl_subsamplers[kTransformSize32x8][kSubsamplingType444] =
      CflSubsampler444_NEON<32, 8>;
  dsp->cfl_subsamplers[kTransformSize32x16][kSubsamplingType444] =
      CflSubsampler444_NEON<32, 16>;
  dsp->cfl_subsamplers[kTransformSize32x32][kSubsamplingType444] =
      CflSubsampler444_NEON<32, 32>;

  dsp->cfl_intra_predictors[kTransformSize4x4] = CflIntraPredictor4xN_NEON<4>;
  dsp->cfl_intra_predictors[kTransformSize4x8] = CflIntraPredictor4xN_NEON<8>;
  dsp->cfl_intra_predictors[kTransformSize4x16] = CflIntraPredictor4xN_NEON<16>;

  dsp->cfl_intra_predictors[kTransformSize8x4] = CflIntraPredictor8xN_NEON<4>;
  dsp->cfl_intra_predictors[kTransformSize8x8] = CflIntraPredictor8xN_NEON<8>;
  dsp->cfl_intra_predictors[kTransformSize8x16] = CflIntraPredictor8xN_NEON<16>;
  dsp->cfl_intra_predictors[kTransformSize8x32] = CflIntraPredictor8xN_NEON<32>;

  dsp->cfl_intra_predictors[kTransformSize16x4] = CflIntraPredictor16xN_NEON<4>;
  dsp->cfl_intra_predictors[kTransformSize16x8] = CflIntraPredictor16xN_NEON<8>;
  dsp->cfl_intra_predictors[kTransformSize16x16] =
      CflIntraPredictor16xN_NEON<16>;
  dsp->cfl_intra_predictors[kTransformSize16x32] =
      CflIntraPredictor16xN_NEON<32>;

  dsp->cfl_intra_predictors[kTransformSize32x8] = CflIntraPredictor32xN_NEON<8>;
  dsp->cfl_intra_predictors[kTransformSize32x16] =
      CflIntraPredictor32xN_NEON<16>;
  dsp->cfl_intra_predictors[kTransformSize32x32] =
      CflIntraPredictor32xN_NEON<32>;
  // Max Cfl predictor size is 32x32.
}

}  // namespace
}  // namespace low_bitdepth

//------------------------------------------------------------------------------
#if LIBGAV1_MAX_BITDEPTH >= 10
namespace high_bitdepth {
namespace {

//------------------------------------------------------------------------------
// CflSubsampler
#ifndef __aarch64__
uint16x8_t vpaddq_u16(uint16x8_t a, uint16x8_t b) {
  return vcombine_u16(vpadd_u16(vget_low_u16(a), vget_high_u16(a)),
                      vpadd_u16(vget_low_u16(b), vget_high_u16(b)));
}
#endif

// This duplicates the last two 16-bit values in |row|.
inline uint16x8_t LastRowSamples(const uint16x8_t row) {
  const uint32x2_t a = vget_high_u32(vreinterpretq_u32_u16(row));
  const uint32x4_t b = vdupq_lane_u32(a, 1);
  return vreinterpretq_u16_u32(b);
}

// This duplicates the last unsigned 16-bit value in |row|.
inline uint16x8_t LastRowResult(const uint16x8_t row) {
  const uint16x4_t a = vget_high_u16(row);
  const uint16x8_t b = vdupq_lane_u16(a, 0x3);
  return b;
}

// This duplicates the last signed 16-bit value in |row|.
inline int16x8_t LastRowResult(const int16x8_t row) {
  const int16x4_t a = vget_high_s16(row);
  const int16x8_t b = vdupq_lane_s16(a, 0x3);
  return b;
}

// Takes in two sums of input row pairs, and completes the computation for two
// output rows.
inline uint16x8_t StoreLumaResults4_420(const uint16x8_t vertical_sum0,
                                        const uint16x8_t vertical_sum1,
                                        int16_t* luma_ptr) {
  const uint16x8_t result = vpaddq_u16(vertical_sum0, vertical_sum1);
  const uint16x8_t result_shifted = vshlq_n_u16(result, 1);
  vst1_s16(luma_ptr, vreinterpret_s16_u16(vget_low_u16(result_shifted)));
  vst1_s16(luma_ptr + kCflLumaBufferStride,
           vreinterpret_s16_u16(vget_high_u16(result_shifted)));
  return result_shifted;
}

// Takes two halves of a vertically added pair of rows and completes the
// computation for one output row.
inline uint16x8_t StoreLumaResults8_420(const uint16x8_t vertical_sum0,
                                        const uint16x8_t vertical_sum1,
                                        int16_t* luma_ptr) {
  const uint16x8_t result = vpaddq_u16(vertical_sum0, vertical_sum1);
  const uint16x8_t result_shifted = vshlq_n_u16(result, 1);
  vst1q_s16(luma_ptr, vreinterpretq_s16_u16(result_shifted));
  return result_shifted;
}

template <int block_height_log2, bool is_inside>
void CflSubsampler444_4xH_NEON(
    int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
    const int max_luma_height, const void* LIBGAV1_RESTRICT const source,
    ptrdiff_t stride) {
  static_assert(block_height_log2 <= 4, "");
  const int block_height = 1 << block_height_log2;
  const int visible_height = max_luma_height;
  const auto* src = static_cast<const uint16_t*>(source);
  const ptrdiff_t src_stride = stride / sizeof(src[0]);
  int16_t* luma_ptr = luma[0];
  uint16x4_t sum = vdup_n_u16(0);
  uint16x4_t samples[2];
  int y = visible_height;

  do {
    samples[0] = vld1_u16(src);
    samples[1] = vld1_u16(src + src_stride);
    src += src_stride << 1;
    sum = vadd_u16(sum, samples[0]);
    sum = vadd_u16(sum, samples[1]);
    y -= 2;
  } while (y != 0);

  if (!is_inside) {
    y = visible_height;
    samples[1] = vshl_n_u16(samples[1], 1);
    do {
      sum = vadd_u16(sum, samples[1]);
      y += 2;
    } while (y < block_height);
  }

  // Here the left shift by 3 (to increase precision) is nullified in right
  // shift ((log2 of width 4) + 1).
  const uint32_t average_sum =
      RightShiftWithRounding(SumVector(vpaddl_u16(sum)), block_height_log2 - 1);
  const int16x4_t averages = vdup_n_s16(static_cast<int16_t>(average_sum));

  const auto* ssrc = static_cast<const int16_t*>(source);
  int16x4_t ssample;
  luma_ptr = luma[0];
  y = visible_height;
  do {
    ssample = vld1_s16(ssrc);
    ssample = vshl_n_s16(ssample, 3);
    vst1_s16(luma_ptr, vsub_s16(ssample, averages));
    ssrc += src_stride;
    luma_ptr += kCflLumaBufferStride;
  } while (--y != 0);

  if (!is_inside) {
    y = visible_height;
    // Replicate last line
    do {
      vst1_s16(luma_ptr, vsub_s16(ssample, averages));
      luma_ptr += kCflLumaBufferStride;
    } while (++y < block_height);
  }
}

template <int block_height_log2>
void CflSubsampler444_4xH_NEON(
    int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
    const int max_luma_width, const int max_luma_height,
    const void* LIBGAV1_RESTRICT const source, ptrdiff_t stride) {
  static_cast<void>(max_luma_width);
  static_cast<void>(max_luma_height);
  static_assert(block_height_log2 <= 4, "");
  assert(max_luma_width >= 4);
  assert(max_luma_height >= 4);
  const int block_height = 1 << block_height_log2;

  if (block_height <= max_luma_height) {
    CflSubsampler444_4xH_NEON<block_height_log2, true>(luma, max_luma_height,
                                                       source, stride);
  } else {
    CflSubsampler444_4xH_NEON<block_height_log2, false>(luma, max_luma_height,
                                                        source, stride);
  }
}

template <int block_height_log2, bool is_inside>
void CflSubsampler444_8xH_NEON(
    int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
    const int max_luma_height, const void* LIBGAV1_RESTRICT const source,
    ptrdiff_t stride) {
  const int block_height = 1 << block_height_log2;
  const int visible_height = max_luma_height;
  const auto* src = static_cast<const uint16_t*>(source);
  const ptrdiff_t src_stride = stride / sizeof(src[0]);
  int16_t* luma_ptr = luma[0];
  uint32x4_t sum = vdupq_n_u32(0);
  uint16x8_t samples;
  int y = visible_height;

  do {
    samples = vld1q_u16(src);
    src += src_stride;
    sum = vpadalq_u16(sum, samples);
  } while (--y != 0);

  if (!is_inside) {
    y = visible_height;
    do {
      sum = vpadalq_u16(sum, samples);
    } while (++y < block_height);
  }

  // Here the left shift by 3 (to increase precision) is nullified in right
  // shift (log2 of width 8).
  const uint32_t average_sum =
      RightShiftWithRounding(SumVector(sum), block_height_log2);
  const int16x8_t averages = vdupq_n_s16(static_cast<int16_t>(average_sum));

  const auto* ssrc = static_cast<const int16_t*>(source);
  int16x8_t ssample;
  luma_ptr = luma[0];
  y = visible_height;
  do {
    ssample = vld1q_s16(ssrc);
    ssample = vshlq_n_s16(ssample, 3);
    vst1q_s16(luma_ptr, vsubq_s16(ssample, averages));
    ssrc += src_stride;
    luma_ptr += kCflLumaBufferStride;
  } while (--y != 0);

  if (!is_inside) {
    y = visible_height;
    // Replicate last line
    do {
      vst1q_s16(luma_ptr, vsubq_s16(ssample, averages));
      luma_ptr += kCflLumaBufferStride;
    } while (++y < block_height);
  }
}

template <int block_height_log2>
void CflSubsampler444_8xH_NEON(
    int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
    const int max_luma_width, const int max_luma_height,
    const void* LIBGAV1_RESTRICT const source, ptrdiff_t stride) {
  static_cast<void>(max_luma_width);
  static_cast<void>(max_luma_height);
  static_assert(block_height_log2 <= 5, "");
  assert(max_luma_width >= 4);
  assert(max_luma_height >= 4);
  const int block_height = 1 << block_height_log2;
  const int block_width = 8;

  const int horz_inside = block_width <= max_luma_width;
  const int vert_inside = block_height <= max_luma_height;
  if (horz_inside && vert_inside) {
    CflSubsampler444_8xH_NEON<block_height_log2, true>(luma, max_luma_height,
                                                       source, stride);
  } else {
    CflSubsampler444_8xH_NEON<block_height_log2, false>(luma, max_luma_height,
                                                        source, stride);
  }
}

template <int block_width_log2, int block_height_log2, bool is_inside>
void CflSubsampler444_WxH_NEON(
    int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
    const int max_luma_width, const int max_luma_height,
    const void* LIBGAV1_RESTRICT const source, ptrdiff_t stride) {
  const int block_height = 1 << block_height_log2;
  const int visible_height = max_luma_height;
  const int block_width = 1 << block_width_log2;
  const auto* src = static_cast<const uint16_t*>(source);
  const ptrdiff_t src_stride = stride / sizeof(src[0]);
  int16_t* luma_ptr = luma[0];
  uint32x4_t sum = vdupq_n_u32(0);
  uint16x8_t samples[4];
  int y = visible_height;

  do {
    samples[0] = vld1q_u16(src);
    samples[1] =
        (max_luma_width >= 16) ? vld1q_u16(src + 8) : LastRowResult(samples[0]);
    uint16x8_t inner_sum = vaddq_u16(samples[0], samples[1]);
    if (block_width == 32) {
      samples[2] = (max_luma_width >= 24) ? vld1q_u16(src + 16)
                                          : LastRowResult(samples[1]);
      samples[3] = (max_luma_width == 32) ? vld1q_u16(src + 24)
                                          : LastRowResult(samples[2]);
      inner_sum = vaddq_u16(samples[2], inner_sum);
      inner_sum = vaddq_u16(samples[3], inner_sum);
    }
    sum = vpadalq_u16(sum, inner_sum);
    src += src_stride;
  } while (--y != 0);

  if (!is_inside) {
    y = visible_height;
    uint16x8_t inner_sum = vaddq_u16(samples[0], samples[1]);
    if (block_width == 32) {
      inner_sum = vaddq_u16(samples[2], inner_sum);
      inner_sum = vaddq_u16(samples[3], inner_sum);
    }
    do {
      sum = vpadalq_u16(sum, inner_sum);
    } while (++y < block_height);
  }

  // Here the left shift by 3 (to increase precision) is subtracted in right
  // shift factor (block_width_log2 + block_height_log2 - 3).
  const uint32_t average_sum = RightShiftWithRounding(
      SumVector(sum), block_width_log2 + block_height_log2 - 3);
  const int16x8_t averages = vdupq_n_s16(static_cast<int16_t>(average_sum));

  const auto* ssrc = static_cast<const int16_t*>(source);
  int16x8_t ssamples_ext = vdupq_n_s16(0);
  int16x8_t ssamples[4];
  luma_ptr = luma[0];
  y = visible_height;
  do {
    int idx = 0;
    for (int x = 0; x < block_width; x += 8) {
      if (max_luma_width > x) {
        ssamples[idx] = vld1q_s16(&ssrc[x]);
        ssamples[idx] = vshlq_n_s16(ssamples[idx], 3);
        ssamples_ext = ssamples[idx];
      } else {
        ssamples[idx] = LastRowResult(ssamples_ext);
      }
      vst1q_s16(&luma_ptr[x], vsubq_s16(ssamples[idx++], averages));
    }
    ssrc += src_stride;
    luma_ptr += kCflLumaBufferStride;
  } while (--y != 0);

  if (!is_inside) {
    y = visible_height;
    // Replicate last line
    do {
      int idx = 0;
      for (int x = 0; x < block_width; x += 8) {
        vst1q_s16(&luma_ptr[x], vsubq_s16(ssamples[idx++], averages));
      }
      luma_ptr += kCflLumaBufferStride;
    } while (++y < block_height);
  }
}

template <int block_width_log2, int block_height_log2>
void CflSubsampler444_WxH_NEON(
    int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
    const int max_luma_width, const int max_luma_height,
    const void* LIBGAV1_RESTRICT const source, ptrdiff_t stride) {
  static_assert(block_width_log2 == 4 || block_width_log2 == 5,
                "This function will only work for block_width 16 and 32.");
  static_assert(block_height_log2 <= 5, "");
  assert(max_luma_width >= 4);
  assert(max_luma_height >= 4);

  const int block_height = 1 << block_height_log2;
  const int vert_inside = block_height <= max_luma_height;
  if (vert_inside) {
    CflSubsampler444_WxH_NEON<block_width_log2, block_height_log2, true>(
        luma, max_luma_width, max_luma_height, source, stride);
  } else {
    CflSubsampler444_WxH_NEON<block_width_log2, block_height_log2, false>(
        luma, max_luma_width, max_luma_height, source, stride);
  }
}

template <int block_height_log2>
void CflSubsampler420_4xH_NEON(
    int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
    const int /*max_luma_width*/, const int max_luma_height,
    const void* LIBGAV1_RESTRICT const source, ptrdiff_t stride) {
  const int block_height = 1 << block_height_log2;
  const auto* src = static_cast<const uint16_t*>(source);
  const ptrdiff_t src_stride = stride / sizeof(src[0]);
  int16_t* luma_ptr = luma[0];
  const int luma_height = std::min(block_height, max_luma_height >> 1);
  int y = luma_height;

  uint32x4_t final_sum = vdupq_n_u32(0);
  do {
    const uint16x8_t samples_row0 = vld1q_u16(src);
    src += src_stride;
    const uint16x8_t samples_row1 = vld1q_u16(src);
    src += src_stride;
    const uint16x8_t luma_sum01 = vaddq_u16(samples_row0, samples_row1);

    const uint16x8_t samples_row2 = vld1q_u16(src);
    src += src_stride;
    const uint16x8_t samples_row3 = vld1q_u16(src);
    src += src_stride;
    const uint16x8_t luma_sum23 = vaddq_u16(samples_row2, samples_row3);
    uint16x8_t sum = StoreLumaResults4_420(luma_sum01, luma_sum23, luma_ptr);
    luma_ptr += kCflLumaBufferStride << 1;

    const uint16x8_t samples_row4 = vld1q_u16(src);
    src += src_stride;
    const uint16x8_t samples_row5 = vld1q_u16(src);
    src += src_stride;
    const uint16x8_t luma_sum45 = vaddq_u16(samples_row4, samples_row5);

    const uint16x8_t samples_row6 = vld1q_u16(src);
    src += src_stride;
    const uint16x8_t samples_row7 = vld1q_u16(src);
    src += src_stride;
    const uint16x8_t luma_sum67 = vaddq_u16(samples_row6, samples_row7);
    sum =
        vaddq_u16(sum, StoreLumaResults4_420(luma_sum45, luma_sum67, luma_ptr));
    luma_ptr += kCflLumaBufferStride << 1;

    final_sum = vpadalq_u16(final_sum, sum);
    y -= 4;
  } while (y != 0);

  const uint16x4_t final_fill =
      vreinterpret_u16_s16(vld1_s16(luma_ptr - kCflLumaBufferStride));
  const uint32x4_t final_fill_to_sum = vmovl_u16(final_fill);
  for (y = luma_height; y < block_height; ++y) {
    vst1_s16(luma_ptr, vreinterpret_s16_u16(final_fill));
    luma_ptr += kCflLumaBufferStride;
    final_sum = vaddq_u32(final_sum, final_fill_to_sum);
  }
  const uint32_t average_sum = RightShiftWithRounding(
      SumVector(final_sum), block_height_log2 + 2 /*log2 of width 4*/);
  const int16x4_t averages = vdup_n_s16(static_cast<int16_t>(average_sum));
  luma_ptr = luma[0];
  y = block_height;
  do {
    const int16x4_t samples = vld1_s16(luma_ptr);
    vst1_s16(luma_ptr, vsub_s16(samples, averages));
    luma_ptr += kCflLumaBufferStride;
  } while (--y != 0);
}

template <int block_height_log2, int max_luma_width>
inline void CflSubsampler420Impl_8xH_NEON(
    int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
    const int max_luma_height, const void* LIBGAV1_RESTRICT const source,
    ptrdiff_t stride) {
  const int block_height = 1 << block_height_log2;
  const auto* src = static_cast<const uint16_t*>(source);
  const ptrdiff_t src_stride = stride / sizeof(src[0]);
  int16_t* luma_ptr = luma[0];
  const int luma_height = std::min(block_height, max_luma_height >> 1);
  int y = luma_height;

  uint32x4_t final_sum = vdupq_n_u32(0);
  do {
    const uint16x8_t samples_row00 = vld1q_u16(src);
    const uint16x8_t samples_row01 = (max_luma_width == 16)
                                         ? vld1q_u16(src + 8)
                                         : LastRowSamples(samples_row00);
    src += src_stride;
    const uint16x8_t samples_row10 = vld1q_u16(src);
    const uint16x8_t samples_row11 = (max_luma_width == 16)
                                         ? vld1q_u16(src + 8)
                                         : LastRowSamples(samples_row10);
    src += src_stride;
    const uint16x8_t luma_sum00 = vaddq_u16(samples_row00, samples_row10);
    const uint16x8_t luma_sum01 = vaddq_u16(samples_row01, samples_row11);
    uint16x8_t sum = StoreLumaResults8_420(luma_sum00, luma_sum01, luma_ptr);
    luma_ptr += kCflLumaBufferStride;

    const uint16x8_t samples_row20 = vld1q_u16(src);
    const uint16x8_t samples_row21 = (max_luma_width == 16)
                                         ? vld1q_u16(src + 8)
                                         : LastRowSamples(samples_row20);
    src += src_stride;
    const uint16x8_t samples_row30 = vld1q_u16(src);
    const uint16x8_t samples_row31 = (max_luma_width == 16)
                                         ? vld1q_u16(src + 8)
                                         : LastRowSamples(samples_row30);
    src += src_stride;
    const uint16x8_t luma_sum10 = vaddq_u16(samples_row20, samples_row30);
    const uint16x8_t luma_sum11 = vaddq_u16(samples_row21, samples_row31);
    sum =
        vaddq_u16(sum, StoreLumaResults8_420(luma_sum10, luma_sum11, luma_ptr));
    luma_ptr += kCflLumaBufferStride;

    const uint16x8_t samples_row40 = vld1q_u16(src);
    const uint16x8_t samples_row41 = (max_luma_width == 16)
                                         ? vld1q_u16(src + 8)
                                         : LastRowSamples(samples_row40);
    src += src_stride;
    const uint16x8_t samples_row50 = vld1q_u16(src);
    const uint16x8_t samples_row51 = (max_luma_width == 16)
                                         ? vld1q_u16(src + 8)
                                         : LastRowSamples(samples_row50);
    src += src_stride;
    const uint16x8_t luma_sum20 = vaddq_u16(samples_row40, samples_row50);
    const uint16x8_t luma_sum21 = vaddq_u16(samples_row41, samples_row51);
    sum =
        vaddq_u16(sum, StoreLumaResults8_420(luma_sum20, luma_sum21, luma_ptr));
    luma_ptr += kCflLumaBufferStride;

    const uint16x8_t samples_row60 = vld1q_u16(src);
    const uint16x8_t samples_row61 = (max_luma_width == 16)
                                         ? vld1q_u16(src + 8)
                                         : LastRowSamples(samples_row60);
    src += src_stride;
    const uint16x8_t samples_row70 = vld1q_u16(src);
    const uint16x8_t samples_row71 = (max_luma_width == 16)
                                         ? vld1q_u16(src + 8)
                                         : LastRowSamples(samples_row70);
    src += src_stride;
    const uint16x8_t luma_sum30 = vaddq_u16(samples_row60, samples_row70);
    const uint16x8_t luma_sum31 = vaddq_u16(samples_row61, samples_row71);
    sum =
        vaddq_u16(sum, StoreLumaResults8_420(luma_sum30, luma_sum31, luma_ptr));
    luma_ptr += kCflLumaBufferStride;

    final_sum = vpadalq_u16(final_sum, sum);
    y -= 4;
  } while (y != 0);

  // Duplicate the final row downward to the end after max_luma_height.
  const uint16x8_t final_fill =
      vreinterpretq_u16_s16(vld1q_s16(luma_ptr - kCflLumaBufferStride));
  const uint32x4_t final_fill_to_sum =
      vaddl_u16(vget_low_u16(final_fill), vget_high_u16(final_fill));

  for (y = luma_height; y < block_height; ++y) {
    vst1q_s16(luma_ptr, vreinterpretq_s16_u16(final_fill));
    luma_ptr += kCflLumaBufferStride;
    final_sum = vaddq_u32(final_sum, final_fill_to_sum);
  }

  const uint32_t average_sum = RightShiftWithRounding(
      SumVector(final_sum), block_height_log2 + 3 /*log2 of width 8*/);
  const int16x8_t averages = vdupq_n_s16(static_cast<int16_t>(average_sum));
  luma_ptr = luma[0];
  y = block_height;
  do {
    const int16x8_t samples = vld1q_s16(luma_ptr);
    vst1q_s16(luma_ptr, vsubq_s16(samples, averages));
    luma_ptr += kCflLumaBufferStride;
  } while (--y != 0);
}

template <int block_height_log2>
void CflSubsampler420_8xH_NEON(
    int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
    const int max_luma_width, const int max_luma_height,
    const void* LIBGAV1_RESTRICT const source, ptrdiff_t stride) {
  if (max_luma_width == 8) {
    CflSubsampler420Impl_8xH_NEON<block_height_log2, 8>(luma, max_luma_height,
                                                        source, stride);
  } else {
    CflSubsampler420Impl_8xH_NEON<block_height_log2, 16>(luma, max_luma_height,
                                                         source, stride);
  }
}

template <int block_width_log2, int block_height_log2, int max_luma_width>
inline void CflSubsampler420Impl_WxH_NEON(
    int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
    const int max_luma_height, const void* LIBGAV1_RESTRICT const source,
    ptrdiff_t stride) {
  const auto* src = static_cast<const uint16_t*>(source);
  const ptrdiff_t src_stride = stride / sizeof(src[0]);
  const int block_height = 1 << block_height_log2;
  const int luma_height = std::min(block_height, max_luma_height >> 1);
  int16_t* luma_ptr = luma[0];
  // Begin first y section, covering width up to 32.
  int y = luma_height;

  uint16x8_t final_fill0, final_fill1;
  uint32x4_t final_sum = vdupq_n_u32(0);
  do {
    const uint16_t* src_next = src + src_stride;
    const uint16x8_t samples_row00 = vld1q_u16(src);
    const uint16x8_t samples_row01 = (max_luma_width >= 16)
                                         ? vld1q_u16(src + 8)
                                         : LastRowSamples(samples_row00);
    const uint16x8_t samples_row02 = (max_luma_width >= 24)
                                         ? vld1q_u16(src + 16)
                                         : LastRowSamples(samples_row01);
    const uint16x8_t samples_row03 = (max_luma_width == 32)
                                         ? vld1q_u16(src + 24)
                                         : LastRowSamples(samples_row02);
    const uint16x8_t samples_row10 = vld1q_u16(src_next);
    const uint16x8_t samples_row11 = (max_luma_width >= 16)
                                         ? vld1q_u16(src_next + 8)
                                         : LastRowSamples(samples_row10);
    const uint16x8_t samples_row12 = (max_luma_width >= 24)
                                         ? vld1q_u16(src_next + 16)
                                         : LastRowSamples(samples_row11);
    const uint16x8_t samples_row13 = (max_luma_width == 32)
                                         ? vld1q_u16(src_next + 24)
                                         : LastRowSamples(samples_row12);
    const uint16x8_t luma_sum0 = vaddq_u16(samples_row00, samples_row10);
    const uint16x8_t luma_sum1 = vaddq_u16(samples_row01, samples_row11);
    const uint16x8_t luma_sum2 = vaddq_u16(samples_row02, samples_row12);
    const uint16x8_t luma_sum3 = vaddq_u16(samples_row03, samples_row13);
    final_fill0 = StoreLumaResults8_420(luma_sum0, luma_sum1, luma_ptr);
    final_fill1 = StoreLumaResults8_420(luma_sum2, luma_sum3, luma_ptr + 8);
    const uint16x8_t sum = vaddq_u16(final_fill0, final_fill1);

    final_sum = vpadalq_u16(final_sum, sum);

    // Because max_luma_width is at most 32, any values beyond x=16 will
    // necessarily be duplicated.
    if (block_width_log2 == 5) {
      const uint16x8_t wide_fill = LastRowResult(final_fill1);
      final_sum = vpadalq_u16(final_sum, vshlq_n_u16(wide_fill, 1));
    }
    src += src_stride << 1;
    luma_ptr += kCflLumaBufferStride;
  } while (--y != 0);

  // Begin second y section.
  y = luma_height;
  if (y < block_height) {
    uint32x4_t wide_fill;
    if (block_width_log2 == 5) {
      // There are 16 16-bit fill values per row, shifting by 2 accounts for
      // the widening to 32-bit.  (a << 2) = (a + a) << 1.
      wide_fill = vshll_n_u16(vget_low_u16(LastRowResult(final_fill1)), 2);
    }
    const uint16x8_t final_inner_sum = vaddq_u16(final_fill0, final_fill1);
    const uint32x4_t final_fill_to_sum = vaddl_u16(
        vget_low_u16(final_inner_sum), vget_high_u16(final_inner_sum));

    do {
      vst1q_s16(luma_ptr, vreinterpretq_s16_u16(final_fill0));
      vst1q_s16(luma_ptr + 8, vreinterpretq_s16_u16(final_fill1));
      if (block_width_log2 == 5) {
        final_sum = vaddq_u32(final_sum, wide_fill);
      }
      luma_ptr += kCflLumaBufferStride;
      final_sum = vaddq_u32(final_sum, final_fill_to_sum);
    } while (++y < block_height);
  }  // End second y section.

  const uint32_t average_sum = RightShiftWithRounding(
      SumVector(final_sum), block_width_log2 + block_height_log2);
  const int16x8_t averages = vdupq_n_s16(static_cast<int16_t>(average_sum));

  luma_ptr = luma[0];
  y = block_height;
  do {
    const int16x8_t samples0 = vld1q_s16(luma_ptr);
    vst1q_s16(luma_ptr, vsubq_s16(samples0, averages));
    const int16x8_t samples1 = vld1q_s16(luma_ptr + 8);
    const int16x8_t final_row_result = vsubq_s16(samples1, averages);
    vst1q_s16(luma_ptr + 8, final_row_result);

    if (block_width_log2 == 5) {
      const int16x8_t wide_fill = LastRowResult(final_row_result);
      vst1q_s16(luma_ptr + 16, wide_fill);
      vst1q_s16(luma_ptr + 24, wide_fill);
    }
    luma_ptr += kCflLumaBufferStride;
  } while (--y != 0);
}

//------------------------------------------------------------------------------
// Choose subsampler based on max_luma_width
template <int block_width_log2, int block_height_log2>
void CflSubsampler420_WxH_NEON(
    int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
    const int max_luma_width, const int max_luma_height,
    const void* LIBGAV1_RESTRICT const source, ptrdiff_t stride) {
  switch (max_luma_width) {
    case 8:
      CflSubsampler420Impl_WxH_NEON<block_width_log2, block_height_log2, 8>(
          luma, max_luma_height, source, stride);
      return;
    case 16:
      CflSubsampler420Impl_WxH_NEON<block_width_log2, block_height_log2, 16>(
          luma, max_luma_height, source, stride);
      return;
    case 24:
      CflSubsampler420Impl_WxH_NEON<block_width_log2, block_height_log2, 24>(
          luma, max_luma_height, source, stride);
      return;
    default:
      assert(max_luma_width == 32);
      CflSubsampler420Impl_WxH_NEON<block_width_log2, block_height_log2, 32>(
          luma, max_luma_height, source, stride);
      return;
  }
}

//------------------------------------------------------------------------------
// CflIntraPredictor

// |luma| can be within +/-(((1 << bitdepth) - 1) << 3), inclusive.
// |alpha| can be -16 to 16 (inclusive).
// Clip |dc + ((alpha * luma) >> 6))| to 0, (1 << bitdepth) - 1.
inline uint16x8_t Combine8(const int16x8_t luma, const int16x8_t alpha_abs,
                           const int16x8_t alpha_signed, const int16x8_t dc,
                           const uint16x8_t max_value) {
  const int16x8_t luma_abs = vabsq_s16(luma);
  const int16x8_t luma_alpha_sign =
      vshrq_n_s16(veorq_s16(luma, alpha_signed), 15);
  // (alpha * luma) >> 6
  const int16x8_t la_abs = vqrdmulhq_s16(luma_abs, alpha_abs);
  // Convert back to signed values.
  const int16x8_t la =
      vsubq_s16(veorq_s16(la_abs, luma_alpha_sign), luma_alpha_sign);
  const int16x8_t result = vaddq_s16(la, dc);
  const int16x8_t zero = vdupq_n_s16(0);
  // Clip.
  return vminq_u16(vreinterpretq_u16_s16(vmaxq_s16(result, zero)), max_value);
}

template <int block_height, int bitdepth = 10>
inline void CflIntraPredictor4xN_NEON(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
    const int alpha) {
  auto* dst = static_cast<uint16_t*>(dest);
  const ptrdiff_t dst_stride = stride >> 1;
  const uint16x8_t max_value = vdupq_n_u16((1 << bitdepth) - 1);
  const int16x8_t alpha_signed = vdupq_n_s16(alpha << 9);
  const int16x8_t alpha_abs = vabsq_s16(alpha_signed);
  const int16x8_t dc = vdupq_n_s16(dst[0]);
  for (int y = 0; y < block_height; y += 2) {
    const int16x4_t luma_row0 = vld1_s16(luma[y]);
    const int16x4_t luma_row1 = vld1_s16(luma[y + 1]);
    const int16x8_t combined_luma = vcombine_s16(luma_row0, luma_row1);
    const uint16x8_t sum =
        Combine8(combined_luma, alpha_abs, alpha_signed, dc, max_value);
    vst1_u16(dst, vget_low_u16(sum));
    dst += dst_stride;
    vst1_u16(dst, vget_high_u16(sum));
    dst += dst_stride;
  }
}

template <int block_height, int bitdepth = 10>
inline void CflIntraPredictor8xN_NEON(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
    const int alpha) {
  auto* dst = static_cast<uint16_t*>(dest);
  const ptrdiff_t dst_stride = stride >> 1;
  const uint16x8_t max_value = vdupq_n_u16((1 << bitdepth) - 1);
  const int16x8_t alpha_signed = vdupq_n_s16(alpha << 9);
  const int16x8_t alpha_abs = vabsq_s16(alpha_signed);
  const int16x8_t dc = vdupq_n_s16(dst[0]);
  for (int y = 0; y < block_height; ++y) {
    const int16x8_t luma_row = vld1q_s16(luma[y]);
    const uint16x8_t sum =
        Combine8(luma_row, alpha_abs, alpha_signed, dc, max_value);
    vst1q_u16(dst, sum);
    dst += dst_stride;
  }
}

template <int block_height, int bitdepth = 10>
inline void CflIntraPredictor16xN_NEON(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
    const int alpha) {
  auto* dst = static_cast<uint16_t*>(dest);
  const ptrdiff_t dst_stride = stride >> 1;
  const uint16x8_t max_value = vdupq_n_u16((1 << bitdepth) - 1);
  const int16x8_t alpha_signed = vdupq_n_s16(alpha << 9);
  const int16x8_t alpha_abs = vabsq_s16(alpha_signed);
  const int16x8_t dc = vdupq_n_s16(dst[0]);
  for (int y = 0; y < block_height; ++y) {
    const int16x8_t luma_row_0 = vld1q_s16(luma[y]);
    const int16x8_t luma_row_1 = vld1q_s16(luma[y] + 8);
    const uint16x8_t sum_0 =
        Combine8(luma_row_0, alpha_abs, alpha_signed, dc, max_value);
    const uint16x8_t sum_1 =
        Combine8(luma_row_1, alpha_abs, alpha_signed, dc, max_value);
    vst1q_u16(dst, sum_0);
    vst1q_u16(dst + 8, sum_1);
    dst += dst_stride;
  }
}

template <int block_height, int bitdepth = 10>
inline void CflIntraPredictor32xN_NEON(
    void* LIBGAV1_RESTRICT const dest, const ptrdiff_t stride,
    const int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride],
    const int alpha) {
  auto* dst = static_cast<uint16_t*>(dest);
  const ptrdiff_t dst_stride = stride >> 1;
  const uint16x8_t max_value = vdupq_n_u16((1 << bitdepth) - 1);
  const int16x8_t alpha_signed = vdupq_n_s16(alpha << 9);
  const int16x8_t alpha_abs = vabsq_s16(alpha_signed);
  const int16x8_t dc = vdupq_n_s16(dst[0]);
  for (int y = 0; y < block_height; ++y) {
    const int16x8_t luma_row_0 = vld1q_s16(luma[y]);
    const int16x8_t luma_row_1 = vld1q_s16(luma[y] + 8);
    const int16x8_t luma_row_2 = vld1q_s16(luma[y] + 16);
    const int16x8_t luma_row_3 = vld1q_s16(luma[y] + 24);
    const uint16x8_t sum_0 =
        Combine8(luma_row_0, alpha_abs, alpha_signed, dc, max_value);
    const uint16x8_t sum_1 =
        Combine8(luma_row_1, alpha_abs, alpha_signed, dc, max_value);
    const uint16x8_t sum_2 =
        Combine8(luma_row_2, alpha_abs, alpha_signed, dc, max_value);
    const uint16x8_t sum_3 =
        Combine8(luma_row_3, alpha_abs, alpha_signed, dc, max_value);
    vst1q_u16(dst, sum_0);
    vst1q_u16(dst + 8, sum_1);
    vst1q_u16(dst + 16, sum_2);
    vst1q_u16(dst + 24, sum_3);
    dst += dst_stride;
  }
}

void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth10);
  assert(dsp != nullptr);

  dsp->cfl_subsamplers[kTransformSize4x4][kSubsamplingType420] =
      CflSubsampler420_4xH_NEON<2>;
  dsp->cfl_subsamplers[kTransformSize4x8][kSubsamplingType420] =
      CflSubsampler420_4xH_NEON<3>;
  dsp->cfl_subsamplers[kTransformSize4x16][kSubsamplingType420] =
      CflSubsampler420_4xH_NEON<4>;

  dsp->cfl_subsamplers[kTransformSize8x4][kSubsamplingType420] =
      CflSubsampler420_8xH_NEON<2>;
  dsp->cfl_subsamplers[kTransformSize8x8][kSubsamplingType420] =
      CflSubsampler420_8xH_NEON<3>;
  dsp->cfl_subsamplers[kTransformSize8x16][kSubsamplingType420] =
      CflSubsampler420_8xH_NEON<4>;
  dsp->cfl_subsamplers[kTransformSize8x32][kSubsamplingType420] =
      CflSubsampler420_8xH_NEON<5>;

  dsp->cfl_subsamplers[kTransformSize16x4][kSubsamplingType420] =
      CflSubsampler420_WxH_NEON<4, 2>;
  dsp->cfl_subsamplers[kTransformSize16x8][kSubsamplingType420] =
      CflSubsampler420_WxH_NEON<4, 3>;
  dsp->cfl_subsamplers[kTransformSize16x16][kSubsamplingType420] =
      CflSubsampler420_WxH_NEON<4, 4>;
  dsp->cfl_subsamplers[kTransformSize16x32][kSubsamplingType420] =
      CflSubsampler420_WxH_NEON<4, 5>;

  dsp->cfl_subsamplers[kTransformSize32x8][kSubsamplingType420] =
      CflSubsampler420_WxH_NEON<5, 3>;
  dsp->cfl_subsamplers[kTransformSize32x16][kSubsamplingType420] =
      CflSubsampler420_WxH_NEON<5, 4>;
  dsp->cfl_subsamplers[kTransformSize32x32][kSubsamplingType420] =
      CflSubsampler420_WxH_NEON<5, 5>;

  dsp->cfl_subsamplers[kTransformSize4x4][kSubsamplingType444] =
      CflSubsampler444_4xH_NEON<2>;
  dsp->cfl_subsamplers[kTransformSize4x8][kSubsamplingType444] =
      CflSubsampler444_4xH_NEON<3>;
  dsp->cfl_subsamplers[kTransformSize4x16][kSubsamplingType444] =
      CflSubsampler444_4xH_NEON<4>;

  dsp->cfl_subsamplers[kTransformSize8x4][kSubsamplingType444] =
      CflSubsampler444_8xH_NEON<2>;
  dsp->cfl_subsamplers[kTransformSize8x8][kSubsamplingType444] =
      CflSubsampler444_8xH_NEON<3>;
  dsp->cfl_subsamplers[kTransformSize8x16][kSubsamplingType444] =
      CflSubsampler444_8xH_NEON<4>;
  dsp->cfl_subsamplers[kTransformSize8x32][kSubsamplingType444] =
      CflSubsampler444_8xH_NEON<5>;

  dsp->cfl_subsamplers[kTransformSize16x4][kSubsamplingType444] =
      CflSubsampler444_WxH_NEON<4, 2>;
  dsp->cfl_subsamplers[kTransformSize16x8][kSubsamplingType444] =
      CflSubsampler444_WxH_NEON<4, 3>;
  dsp->cfl_subsamplers[kTransformSize16x16][kSubsamplingType444] =
      CflSubsampler444_WxH_NEON<4, 4>;
  dsp->cfl_subsamplers[kTransformSize16x32][kSubsamplingType444] =
      CflSubsampler444_WxH_NEON<4, 5>;

  dsp->cfl_subsamplers[kTransformSize32x8][kSubsamplingType444] =
      CflSubsampler444_WxH_NEON<5, 3>;
  dsp->cfl_subsamplers[kTransformSize32x16][kSubsamplingType444] =
      CflSubsampler444_WxH_NEON<5, 4>;
  dsp->cfl_subsamplers[kTransformSize32x32][kSubsamplingType444] =
      CflSubsampler444_WxH_NEON<5, 5>;

  dsp->cfl_intra_predictors[kTransformSize4x4] = CflIntraPredictor4xN_NEON<4>;
  dsp->cfl_intra_predictors[kTransformSize4x8] = CflIntraPredictor4xN_NEON<8>;
  dsp->cfl_intra_predictors[kTransformSize4x16] = CflIntraPredictor4xN_NEON<16>;

  dsp->cfl_intra_predictors[kTransformSize8x4] = CflIntraPredictor8xN_NEON<4>;
  dsp->cfl_intra_predictors[kTransformSize8x8] = CflIntraPredictor8xN_NEON<8>;
  dsp->cfl_intra_predictors[kTransformSize8x16] = CflIntraPredictor8xN_NEON<16>;
  dsp->cfl_intra_predictors[kTransformSize8x32] = CflIntraPredictor8xN_NEON<32>;

  dsp->cfl_intra_predictors[kTransformSize16x4] = CflIntraPredictor16xN_NEON<4>;
  dsp->cfl_intra_predictors[kTransformSize16x8] = CflIntraPredictor16xN_NEON<8>;
  dsp->cfl_intra_predictors[kTransformSize16x16] =
      CflIntraPredictor16xN_NEON<16>;
  dsp->cfl_intra_predictors[kTransformSize16x32] =
      CflIntraPredictor16xN_NEON<32>;
  dsp->cfl_intra_predictors[kTransformSize32x8] = CflIntraPredictor32xN_NEON<8>;
  dsp->cfl_intra_predictors[kTransformSize32x16] =
      CflIntraPredictor32xN_NEON<16>;
  dsp->cfl_intra_predictors[kTransformSize32x32] =
      CflIntraPredictor32xN_NEON<32>;
  // Max Cfl predictor size is 32x32.
}

}  // namespace
}  // namespace high_bitdepth
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

void IntraPredCflInit_NEON() {
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

void IntraPredCflInit_NEON() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_ENABLE_NEON
