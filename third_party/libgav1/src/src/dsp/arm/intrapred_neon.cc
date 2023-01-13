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

#include "src/dsp/intrapred.h"
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
namespace {

//------------------------------------------------------------------------------
// DcPredFuncs_NEON

using DcSumFunc = uint32x2_t (*)(const void* ref_0, const int ref_0_size_log2,
                                 const bool use_ref_1, const void* ref_1,
                                 const int ref_1_size_log2);
using DcStoreFunc = void (*)(void* dest, ptrdiff_t stride, const uint32x2_t dc);

// DC intra-predictors for square blocks.
template <int block_width_log2, int block_height_log2, DcSumFunc sumfn,
          DcStoreFunc storefn>
struct DcPredFuncs_NEON {
  DcPredFuncs_NEON() = delete;

  static void DcTop(void* dest, ptrdiff_t stride, const void* top_row,
                    const void* left_column);
  static void DcLeft(void* dest, ptrdiff_t stride, const void* top_row,
                     const void* left_column);
  static void Dc(void* dest, ptrdiff_t stride, const void* top_row,
                 const void* left_column);
};

template <int block_width_log2, int block_height_log2, DcSumFunc sumfn,
          DcStoreFunc storefn>
void DcPredFuncs_NEON<block_width_log2, block_height_log2, sumfn, storefn>::
    DcTop(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
          const void* LIBGAV1_RESTRICT const top_row,
          const void* /*left_column*/) {
  const uint32x2_t sum = sumfn(top_row, block_width_log2, false, nullptr, 0);
  const uint32x2_t dc = vrshr_n_u32(sum, block_width_log2);
  storefn(dest, stride, dc);
}

template <int block_width_log2, int block_height_log2, DcSumFunc sumfn,
          DcStoreFunc storefn>
void DcPredFuncs_NEON<block_width_log2, block_height_log2, sumfn, storefn>::
    DcLeft(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
           const void* /*top_row*/,
           const void* LIBGAV1_RESTRICT const left_column) {
  const uint32x2_t sum =
      sumfn(left_column, block_height_log2, false, nullptr, 0);
  const uint32x2_t dc = vrshr_n_u32(sum, block_height_log2);
  storefn(dest, stride, dc);
}

template <int block_width_log2, int block_height_log2, DcSumFunc sumfn,
          DcStoreFunc storefn>
void DcPredFuncs_NEON<block_width_log2, block_height_log2, sumfn, storefn>::Dc(
    void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
    const void* LIBGAV1_RESTRICT const top_row,
    const void* LIBGAV1_RESTRICT const left_column) {
  const uint32x2_t sum =
      sumfn(top_row, block_width_log2, true, left_column, block_height_log2);
  if (block_width_log2 == block_height_log2) {
    const uint32x2_t dc = vrshr_n_u32(sum, block_width_log2 + 1);
    storefn(dest, stride, dc);
  } else {
    // TODO(johannkoenig): Compare this to mul/shift in vectors.
    const int divisor = (1 << block_width_log2) + (1 << block_height_log2);
    uint32_t dc = vget_lane_u32(sum, 0);
    dc += divisor >> 1;
    dc /= divisor;
    storefn(dest, stride, vdup_n_u32(dc));
  }
}

// Sum all the elements in the vector into the low 32 bits.
inline uint32x2_t Sum(const uint16x4_t val) {
  const uint32x2_t sum = vpaddl_u16(val);
  return vpadd_u32(sum, sum);
}

// Sum all the elements in the vector into the low 32 bits.
inline uint32x2_t Sum(const uint16x8_t val) {
  const uint32x4_t sum_0 = vpaddlq_u16(val);
  const uint64x2_t sum_1 = vpaddlq_u32(sum_0);
  return vadd_u32(vget_low_u32(vreinterpretq_u32_u64(sum_1)),
                  vget_high_u32(vreinterpretq_u32_u64(sum_1)));
}

}  // namespace

//------------------------------------------------------------------------------
namespace low_bitdepth {
namespace {

// Add and expand the elements in the |val_[01]| to uint16_t but do not sum the
// entire vector.
inline uint16x8_t Add(const uint8x16_t val_0, const uint8x16_t val_1) {
  const uint16x8_t sum_0 = vpaddlq_u8(val_0);
  const uint16x8_t sum_1 = vpaddlq_u8(val_1);
  return vaddq_u16(sum_0, sum_1);
}

// Add and expand the elements in the |val_[0123]| to uint16_t but do not sum
// the entire vector.
inline uint16x8_t Add(const uint8x16_t val_0, const uint8x16_t val_1,
                      const uint8x16_t val_2, const uint8x16_t val_3) {
  const uint16x8_t sum_0 = Add(val_0, val_1);
  const uint16x8_t sum_1 = Add(val_2, val_3);
  return vaddq_u16(sum_0, sum_1);
}

// Load and combine 32 uint8_t values.
inline uint16x8_t LoadAndAdd32(const uint8_t* buf) {
  const uint8x16_t val_0 = vld1q_u8(buf);
  const uint8x16_t val_1 = vld1q_u8(buf + 16);
  return Add(val_0, val_1);
}

// Load and combine 64 uint8_t values.
inline uint16x8_t LoadAndAdd64(const uint8_t* buf) {
  const uint8x16_t val_0 = vld1q_u8(buf);
  const uint8x16_t val_1 = vld1q_u8(buf + 16);
  const uint8x16_t val_2 = vld1q_u8(buf + 32);
  const uint8x16_t val_3 = vld1q_u8(buf + 48);
  return Add(val_0, val_1, val_2, val_3);
}

// |ref_[01]| each point to 1 << |ref[01]_size_log2| packed uint8_t values.
// If |use_ref_1| is false then only sum |ref_0|.
// For |ref[01]_size_log2| == 4 this relies on |ref_[01]| being aligned to
// uint32_t.
inline uint32x2_t DcSum_NEON(const void* LIBGAV1_RESTRICT ref_0,
                             const int ref_0_size_log2, const bool use_ref_1,
                             const void* LIBGAV1_RESTRICT ref_1,
                             const int ref_1_size_log2) {
  const auto* const ref_0_u8 = static_cast<const uint8_t*>(ref_0);
  const auto* const ref_1_u8 = static_cast<const uint8_t*>(ref_1);
  if (ref_0_size_log2 == 2) {
    uint8x8_t val = Load4(ref_0_u8);
    if (use_ref_1) {
      switch (ref_1_size_log2) {
        case 2: {  // 4x4
          val = Load4<1>(ref_1_u8, val);
          return Sum(vpaddl_u8(val));
        }
        case 3: {  // 4x8
          const uint8x8_t val_1 = vld1_u8(ref_1_u8);
          const uint16x4_t sum_0 = vpaddl_u8(val);
          const uint16x4_t sum_1 = vpaddl_u8(val_1);
          return Sum(vadd_u16(sum_0, sum_1));
        }
        case 4: {  // 4x16
          const uint8x16_t val_1 = vld1q_u8(ref_1_u8);
          return Sum(vaddw_u8(vpaddlq_u8(val_1), val));
        }
      }
    }
    // 4x1
    const uint16x4_t sum = vpaddl_u8(val);
    return vpaddl_u16(sum);
  }
  if (ref_0_size_log2 == 3) {
    const uint8x8_t val_0 = vld1_u8(ref_0_u8);
    if (use_ref_1) {
      switch (ref_1_size_log2) {
        case 2: {  // 8x4
          const uint8x8_t val_1 = Load4(ref_1_u8);
          const uint16x4_t sum_0 = vpaddl_u8(val_0);
          const uint16x4_t sum_1 = vpaddl_u8(val_1);
          return Sum(vadd_u16(sum_0, sum_1));
        }
        case 3: {  // 8x8
          const uint8x8_t val_1 = vld1_u8(ref_1_u8);
          const uint16x4_t sum_0 = vpaddl_u8(val_0);
          const uint16x4_t sum_1 = vpaddl_u8(val_1);
          return Sum(vadd_u16(sum_0, sum_1));
        }
        case 4: {  // 8x16
          const uint8x16_t val_1 = vld1q_u8(ref_1_u8);
          return Sum(vaddw_u8(vpaddlq_u8(val_1), val_0));
        }
        case 5: {  // 8x32
          return Sum(vaddw_u8(LoadAndAdd32(ref_1_u8), val_0));
        }
      }
    }
    // 8x1
    return Sum(vpaddl_u8(val_0));
  }
  if (ref_0_size_log2 == 4) {
    const uint8x16_t val_0 = vld1q_u8(ref_0_u8);
    if (use_ref_1) {
      switch (ref_1_size_log2) {
        case 2: {  // 16x4
          const uint8x8_t val_1 = Load4(ref_1_u8);
          return Sum(vaddw_u8(vpaddlq_u8(val_0), val_1));
        }
        case 3: {  // 16x8
          const uint8x8_t val_1 = vld1_u8(ref_1_u8);
          return Sum(vaddw_u8(vpaddlq_u8(val_0), val_1));
        }
        case 4: {  // 16x16
          const uint8x16_t val_1 = vld1q_u8(ref_1_u8);
          return Sum(Add(val_0, val_1));
        }
        case 5: {  // 16x32
          const uint16x8_t sum_0 = vpaddlq_u8(val_0);
          const uint16x8_t sum_1 = LoadAndAdd32(ref_1_u8);
          return Sum(vaddq_u16(sum_0, sum_1));
        }
        case 6: {  // 16x64
          const uint16x8_t sum_0 = vpaddlq_u8(val_0);
          const uint16x8_t sum_1 = LoadAndAdd64(ref_1_u8);
          return Sum(vaddq_u16(sum_0, sum_1));
        }
      }
    }
    // 16x1
    return Sum(vpaddlq_u8(val_0));
  }
  if (ref_0_size_log2 == 5) {
    const uint16x8_t sum_0 = LoadAndAdd32(ref_0_u8);
    if (use_ref_1) {
      switch (ref_1_size_log2) {
        case 3: {  // 32x8
          const uint8x8_t val_1 = vld1_u8(ref_1_u8);
          return Sum(vaddw_u8(sum_0, val_1));
        }
        case 4: {  // 32x16
          const uint8x16_t val_1 = vld1q_u8(ref_1_u8);
          const uint16x8_t sum_1 = vpaddlq_u8(val_1);
          return Sum(vaddq_u16(sum_0, sum_1));
        }
        case 5: {  // 32x32
          const uint16x8_t sum_1 = LoadAndAdd32(ref_1_u8);
          return Sum(vaddq_u16(sum_0, sum_1));
        }
        case 6: {  // 32x64
          const uint16x8_t sum_1 = LoadAndAdd64(ref_1_u8);
          return Sum(vaddq_u16(sum_0, sum_1));
        }
      }
    }
    // 32x1
    return Sum(sum_0);
  }

  assert(ref_0_size_log2 == 6);
  const uint16x8_t sum_0 = LoadAndAdd64(ref_0_u8);
  if (use_ref_1) {
    switch (ref_1_size_log2) {
      case 4: {  // 64x16
        const uint8x16_t val_1 = vld1q_u8(ref_1_u8);
        const uint16x8_t sum_1 = vpaddlq_u8(val_1);
        return Sum(vaddq_u16(sum_0, sum_1));
      }
      case 5: {  // 64x32
        const uint16x8_t sum_1 = LoadAndAdd32(ref_1_u8);
        return Sum(vaddq_u16(sum_0, sum_1));
      }
      case 6: {  // 64x64
        const uint16x8_t sum_1 = LoadAndAdd64(ref_1_u8);
        return Sum(vaddq_u16(sum_0, sum_1));
      }
    }
  }
  // 64x1
  return Sum(sum_0);
}

template <int width, int height>
inline void DcStore_NEON(void* const dest, ptrdiff_t stride,
                         const uint32x2_t dc) {
  const uint8x16_t dc_dup = vdupq_lane_u8(vreinterpret_u8_u32(dc), 0);
  auto* dst = static_cast<uint8_t*>(dest);
  if (width == 4) {
    int i = height - 1;
    do {
      StoreLo4(dst, vget_low_u8(dc_dup));
      dst += stride;
    } while (--i != 0);
    StoreLo4(dst, vget_low_u8(dc_dup));
  } else if (width == 8) {
    int i = height - 1;
    do {
      vst1_u8(dst, vget_low_u8(dc_dup));
      dst += stride;
    } while (--i != 0);
    vst1_u8(dst, vget_low_u8(dc_dup));
  } else if (width == 16) {
    int i = height - 1;
    do {
      vst1q_u8(dst, dc_dup);
      dst += stride;
    } while (--i != 0);
    vst1q_u8(dst, dc_dup);
  } else if (width == 32) {
    int i = height - 1;
    do {
      vst1q_u8(dst, dc_dup);
      vst1q_u8(dst + 16, dc_dup);
      dst += stride;
    } while (--i != 0);
    vst1q_u8(dst, dc_dup);
    vst1q_u8(dst + 16, dc_dup);
  } else {
    assert(width == 64);
    int i = height - 1;
    do {
      vst1q_u8(dst, dc_dup);
      vst1q_u8(dst + 16, dc_dup);
      vst1q_u8(dst + 32, dc_dup);
      vst1q_u8(dst + 48, dc_dup);
      dst += stride;
    } while (--i != 0);
    vst1q_u8(dst, dc_dup);
    vst1q_u8(dst + 16, dc_dup);
    vst1q_u8(dst + 32, dc_dup);
    vst1q_u8(dst + 48, dc_dup);
  }
}

template <int width, int height>
inline void Paeth4Or8xN_NEON(void* LIBGAV1_RESTRICT const dest,
                             ptrdiff_t stride,
                             const void* LIBGAV1_RESTRICT const top_row,
                             const void* LIBGAV1_RESTRICT const left_column) {
  auto* dest_u8 = static_cast<uint8_t*>(dest);
  const auto* const top_row_u8 = static_cast<const uint8_t*>(top_row);
  const auto* const left_col_u8 = static_cast<const uint8_t*>(left_column);

  const uint8x8_t top_left = vdup_n_u8(top_row_u8[-1]);
  const uint16x8_t top_left_x2 = vdupq_n_u16(top_row_u8[-1] + top_row_u8[-1]);
  uint8x8_t top;
  if (width == 4) {
    top = Load4(top_row_u8);
  } else {  // width == 8
    top = vld1_u8(top_row_u8);
  }

  for (int y = 0; y < height; ++y) {
    const uint8x8_t left = vdup_n_u8(left_col_u8[y]);

    const uint8x8_t left_dist = vabd_u8(top, top_left);
    const uint8x8_t top_dist = vabd_u8(left, top_left);
    const uint16x8_t top_left_dist =
        vabdq_u16(vaddl_u8(top, left), top_left_x2);

    const uint8x8_t left_le_top = vcle_u8(left_dist, top_dist);
    const uint8x8_t left_le_top_left =
        vmovn_u16(vcleq_u16(vmovl_u8(left_dist), top_left_dist));
    const uint8x8_t top_le_top_left =
        vmovn_u16(vcleq_u16(vmovl_u8(top_dist), top_left_dist));

    // if (left_dist <= top_dist && left_dist <= top_left_dist)
    const uint8x8_t left_mask = vand_u8(left_le_top, left_le_top_left);
    //   dest[x] = left_column[y];
    // Fill all the unused spaces with 'top'. They will be overwritten when
    // the positions for top_left are known.
    uint8x8_t result = vbsl_u8(left_mask, left, top);
    // else if (top_dist <= top_left_dist)
    //   dest[x] = top_row[x];
    // Add these values to the mask. They were already set.
    const uint8x8_t left_or_top_mask = vorr_u8(left_mask, top_le_top_left);
    // else
    //   dest[x] = top_left;
    result = vbsl_u8(left_or_top_mask, result, top_left);

    if (width == 4) {
      StoreLo4(dest_u8, result);
    } else {  // width == 8
      vst1_u8(dest_u8, result);
    }
    dest_u8 += stride;
  }
}

// Calculate X distance <= TopLeft distance and pack the resulting mask into
// uint8x8_t.
inline uint8x16_t XLeTopLeft(const uint8x16_t x_dist,
                             const uint16x8_t top_left_dist_low,
                             const uint16x8_t top_left_dist_high) {
  const uint8x16_t top_left_dist = vcombine_u8(vqmovn_u16(top_left_dist_low),
                                               vqmovn_u16(top_left_dist_high));
  return vcleq_u8(x_dist, top_left_dist);
}

// Select the closest values and collect them.
inline uint8x16_t SelectPaeth(const uint8x16_t top, const uint8x16_t left,
                              const uint8x16_t top_left,
                              const uint8x16_t left_le_top,
                              const uint8x16_t left_le_top_left,
                              const uint8x16_t top_le_top_left) {
  // if (left_dist <= top_dist && left_dist <= top_left_dist)
  const uint8x16_t left_mask = vandq_u8(left_le_top, left_le_top_left);
  //   dest[x] = left_column[y];
  // Fill all the unused spaces with 'top'. They will be overwritten when
  // the positions for top_left are known.
  uint8x16_t result = vbslq_u8(left_mask, left, top);
  // else if (top_dist <= top_left_dist)
  //   dest[x] = top_row[x];
  // Add these values to the mask. They were already set.
  const uint8x16_t left_or_top_mask = vorrq_u8(left_mask, top_le_top_left);
  // else
  //   dest[x] = top_left;
  return vbslq_u8(left_or_top_mask, result, top_left);
}

// Generate numbered and high/low versions of top_left_dist.
#define TOP_LEFT_DIST(num)                                              \
  const uint16x8_t top_left_##num##_dist_low = vabdq_u16(               \
      vaddl_u8(vget_low_u8(top[num]), vget_low_u8(left)), top_left_x2); \
  const uint16x8_t top_left_##num##_dist_high = vabdq_u16(              \
      vaddl_u8(vget_high_u8(top[num]), vget_low_u8(left)), top_left_x2)

// Generate numbered versions of XLeTopLeft with x = left.
#define LEFT_LE_TOP_LEFT(num)                                  \
  const uint8x16_t left_le_top_left_##num =                    \
      XLeTopLeft(left_##num##_dist, top_left_##num##_dist_low, \
                 top_left_##num##_dist_high)

// Generate numbered versions of XLeTopLeft with x = top.
#define TOP_LE_TOP_LEFT(num)                           \
  const uint8x16_t top_le_top_left_##num = XLeTopLeft( \
      top_dist, top_left_##num##_dist_low, top_left_##num##_dist_high)

template <int width, int height>
inline void Paeth16PlusxN_NEON(void* LIBGAV1_RESTRICT const dest,
                               ptrdiff_t stride,
                               const void* LIBGAV1_RESTRICT const top_row,
                               const void* LIBGAV1_RESTRICT const left_column) {
  auto* dest_u8 = static_cast<uint8_t*>(dest);
  const auto* const top_row_u8 = static_cast<const uint8_t*>(top_row);
  const auto* const left_col_u8 = static_cast<const uint8_t*>(left_column);

  const uint8x16_t top_left = vdupq_n_u8(top_row_u8[-1]);
  const uint16x8_t top_left_x2 = vdupq_n_u16(top_row_u8[-1] + top_row_u8[-1]);
  uint8x16_t top[4];
  top[0] = vld1q_u8(top_row_u8);
  if (width > 16) {
    top[1] = vld1q_u8(top_row_u8 + 16);
    if (width == 64) {
      top[2] = vld1q_u8(top_row_u8 + 32);
      top[3] = vld1q_u8(top_row_u8 + 48);
    }
  }

  for (int y = 0; y < height; ++y) {
    const uint8x16_t left = vdupq_n_u8(left_col_u8[y]);

    const uint8x16_t top_dist = vabdq_u8(left, top_left);

    const uint8x16_t left_0_dist = vabdq_u8(top[0], top_left);
    TOP_LEFT_DIST(0);
    const uint8x16_t left_0_le_top = vcleq_u8(left_0_dist, top_dist);
    LEFT_LE_TOP_LEFT(0);
    TOP_LE_TOP_LEFT(0);

    const uint8x16_t result_0 =
        SelectPaeth(top[0], left, top_left, left_0_le_top, left_le_top_left_0,
                    top_le_top_left_0);
    vst1q_u8(dest_u8, result_0);

    if (width > 16) {
      const uint8x16_t left_1_dist = vabdq_u8(top[1], top_left);
      TOP_LEFT_DIST(1);
      const uint8x16_t left_1_le_top = vcleq_u8(left_1_dist, top_dist);
      LEFT_LE_TOP_LEFT(1);
      TOP_LE_TOP_LEFT(1);

      const uint8x16_t result_1 =
          SelectPaeth(top[1], left, top_left, left_1_le_top, left_le_top_left_1,
                      top_le_top_left_1);
      vst1q_u8(dest_u8 + 16, result_1);

      if (width == 64) {
        const uint8x16_t left_2_dist = vabdq_u8(top[2], top_left);
        TOP_LEFT_DIST(2);
        const uint8x16_t left_2_le_top = vcleq_u8(left_2_dist, top_dist);
        LEFT_LE_TOP_LEFT(2);
        TOP_LE_TOP_LEFT(2);

        const uint8x16_t result_2 =
            SelectPaeth(top[2], left, top_left, left_2_le_top,
                        left_le_top_left_2, top_le_top_left_2);
        vst1q_u8(dest_u8 + 32, result_2);

        const uint8x16_t left_3_dist = vabdq_u8(top[3], top_left);
        TOP_LEFT_DIST(3);
        const uint8x16_t left_3_le_top = vcleq_u8(left_3_dist, top_dist);
        LEFT_LE_TOP_LEFT(3);
        TOP_LE_TOP_LEFT(3);

        const uint8x16_t result_3 =
            SelectPaeth(top[3], left, top_left, left_3_le_top,
                        left_le_top_left_3, top_le_top_left_3);
        vst1q_u8(dest_u8 + 48, result_3);
      }
    }

    dest_u8 += stride;
  }
}

struct DcDefs {
  DcDefs() = delete;

  using _4x4 = DcPredFuncs_NEON<2, 2, DcSum_NEON, DcStore_NEON<4, 4>>;
  using _4x8 = DcPredFuncs_NEON<2, 3, DcSum_NEON, DcStore_NEON<4, 8>>;
  using _4x16 = DcPredFuncs_NEON<2, 4, DcSum_NEON, DcStore_NEON<4, 16>>;
  using _8x4 = DcPredFuncs_NEON<3, 2, DcSum_NEON, DcStore_NEON<8, 4>>;
  using _8x8 = DcPredFuncs_NEON<3, 3, DcSum_NEON, DcStore_NEON<8, 8>>;
  using _8x16 = DcPredFuncs_NEON<3, 4, DcSum_NEON, DcStore_NEON<8, 16>>;
  using _8x32 = DcPredFuncs_NEON<3, 5, DcSum_NEON, DcStore_NEON<8, 32>>;
  using _16x4 = DcPredFuncs_NEON<4, 2, DcSum_NEON, DcStore_NEON<16, 4>>;
  using _16x8 = DcPredFuncs_NEON<4, 3, DcSum_NEON, DcStore_NEON<16, 8>>;
  using _16x16 = DcPredFuncs_NEON<4, 4, DcSum_NEON, DcStore_NEON<16, 16>>;
  using _16x32 = DcPredFuncs_NEON<4, 5, DcSum_NEON, DcStore_NEON<16, 32>>;
  using _16x64 = DcPredFuncs_NEON<4, 6, DcSum_NEON, DcStore_NEON<16, 64>>;
  using _32x8 = DcPredFuncs_NEON<5, 3, DcSum_NEON, DcStore_NEON<32, 8>>;
  using _32x16 = DcPredFuncs_NEON<5, 4, DcSum_NEON, DcStore_NEON<32, 16>>;
  using _32x32 = DcPredFuncs_NEON<5, 5, DcSum_NEON, DcStore_NEON<32, 32>>;
  using _32x64 = DcPredFuncs_NEON<5, 6, DcSum_NEON, DcStore_NEON<32, 64>>;
  using _64x16 = DcPredFuncs_NEON<6, 4, DcSum_NEON, DcStore_NEON<64, 16>>;
  using _64x32 = DcPredFuncs_NEON<6, 5, DcSum_NEON, DcStore_NEON<64, 32>>;
  using _64x64 = DcPredFuncs_NEON<6, 6, DcSum_NEON, DcStore_NEON<64, 64>>;
};

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
  // 4x4
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDcTop] =
      DcDefs::_4x4::DcTop;
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDcLeft] =
      DcDefs::_4x4::DcLeft;
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDc] =
      DcDefs::_4x4::Dc;
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorPaeth] =
      Paeth4Or8xN_NEON<4, 4>;

  // 4x8
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorDcTop] =
      DcDefs::_4x8::DcTop;
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorDcLeft] =
      DcDefs::_4x8::DcLeft;
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorDc] =
      DcDefs::_4x8::Dc;
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorPaeth] =
      Paeth4Or8xN_NEON<4, 8>;

  // 4x16
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorDcTop] =
      DcDefs::_4x16::DcTop;
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorDcLeft] =
      DcDefs::_4x16::DcLeft;
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorDc] =
      DcDefs::_4x16::Dc;
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorPaeth] =
      Paeth4Or8xN_NEON<4, 16>;

  // 8x4
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorDcTop] =
      DcDefs::_8x4::DcTop;
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorDcLeft] =
      DcDefs::_8x4::DcLeft;
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorDc] =
      DcDefs::_8x4::Dc;
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorPaeth] =
      Paeth4Or8xN_NEON<8, 4>;

  // 8x8
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorDcTop] =
      DcDefs::_8x8::DcTop;
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorDcLeft] =
      DcDefs::_8x8::DcLeft;
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorDc] =
      DcDefs::_8x8::Dc;
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorPaeth] =
      Paeth4Or8xN_NEON<8, 8>;

  // 8x16
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorDcTop] =
      DcDefs::_8x16::DcTop;
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorDcLeft] =
      DcDefs::_8x16::DcLeft;
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorDc] =
      DcDefs::_8x16::Dc;
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorPaeth] =
      Paeth4Or8xN_NEON<8, 16>;

  // 8x32
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorDcTop] =
      DcDefs::_8x32::DcTop;
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorDcLeft] =
      DcDefs::_8x32::DcLeft;
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorDc] =
      DcDefs::_8x32::Dc;
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorPaeth] =
      Paeth4Or8xN_NEON<8, 32>;

  // 16x4
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorDcTop] =
      DcDefs::_16x4::DcTop;
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorDcLeft] =
      DcDefs::_16x4::DcLeft;
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorDc] =
      DcDefs::_16x4::Dc;
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorPaeth] =
      Paeth16PlusxN_NEON<16, 4>;

  // 16x8
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorDcTop] =
      DcDefs::_16x8::DcTop;
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorDcLeft] =
      DcDefs::_16x8::DcLeft;
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorDc] =
      DcDefs::_16x8::Dc;
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorPaeth] =
      Paeth16PlusxN_NEON<16, 8>;

  // 16x16
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorDcTop] =
      DcDefs::_16x16::DcTop;
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorDcLeft] =
      DcDefs::_16x16::DcLeft;
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorDc] =
      DcDefs::_16x16::Dc;
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorPaeth] =
      Paeth16PlusxN_NEON<16, 16>;

  // 16x32
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorDcTop] =
      DcDefs::_16x32::DcTop;
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorDcLeft] =
      DcDefs::_16x32::DcLeft;
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorDc] =
      DcDefs::_16x32::Dc;
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorPaeth] =
      Paeth16PlusxN_NEON<16, 32>;

  // 16x64
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorDcTop] =
      DcDefs::_16x64::DcTop;
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorDcLeft] =
      DcDefs::_16x64::DcLeft;
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorDc] =
      DcDefs::_16x64::Dc;
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorPaeth] =
      Paeth16PlusxN_NEON<16, 64>;

  // 32x8
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorDcTop] =
      DcDefs::_32x8::DcTop;
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorDcLeft] =
      DcDefs::_32x8::DcLeft;
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorDc] =
      DcDefs::_32x8::Dc;
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorPaeth] =
      Paeth16PlusxN_NEON<32, 8>;

  // 32x16
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorDcTop] =
      DcDefs::_32x16::DcTop;
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorDcLeft] =
      DcDefs::_32x16::DcLeft;
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorDc] =
      DcDefs::_32x16::Dc;
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorPaeth] =
      Paeth16PlusxN_NEON<32, 16>;

  // 32x32
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorDcTop] =
      DcDefs::_32x32::DcTop;
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorDcLeft] =
      DcDefs::_32x32::DcLeft;
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorDc] =
      DcDefs::_32x32::Dc;
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorPaeth] =
      Paeth16PlusxN_NEON<32, 32>;

  // 32x64
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorDcTop] =
      DcDefs::_32x64::DcTop;
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorDcLeft] =
      DcDefs::_32x64::DcLeft;
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorDc] =
      DcDefs::_32x64::Dc;
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorPaeth] =
      Paeth16PlusxN_NEON<32, 64>;

  // 64x16
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorDcTop] =
      DcDefs::_64x16::DcTop;
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorDcLeft] =
      DcDefs::_64x16::DcLeft;
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorDc] =
      DcDefs::_64x16::Dc;
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorPaeth] =
      Paeth16PlusxN_NEON<64, 16>;

  // 64x32
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorDcTop] =
      DcDefs::_64x32::DcTop;
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorDcLeft] =
      DcDefs::_64x32::DcLeft;
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorDc] =
      DcDefs::_64x32::Dc;
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorPaeth] =
      Paeth16PlusxN_NEON<64, 32>;

  // 64x64
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorDcTop] =
      DcDefs::_64x64::DcTop;
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorDcLeft] =
      DcDefs::_64x64::DcLeft;
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorDc] =
      DcDefs::_64x64::Dc;
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorPaeth] =
      Paeth16PlusxN_NEON<64, 64>;
}

}  // namespace
}  // namespace low_bitdepth

//------------------------------------------------------------------------------
#if LIBGAV1_MAX_BITDEPTH >= 10
namespace high_bitdepth {
namespace {

// Add the elements in the given vectors together but do not sum the entire
// vector.
inline uint16x8_t Add(const uint16x8_t val_0, const uint16x8_t val_1,
                      const uint16x8_t val_2, const uint16x8_t val_3) {
  const uint16x8_t sum_0 = vaddq_u16(val_0, val_1);
  const uint16x8_t sum_1 = vaddq_u16(val_2, val_3);
  return vaddq_u16(sum_0, sum_1);
}

// Load and combine 16 uint16_t values.
inline uint16x8_t LoadAndAdd16(const uint16_t* buf) {
  const uint16x8_t val_0 = vld1q_u16(buf);
  const uint16x8_t val_1 = vld1q_u16(buf + 8);
  return vaddq_u16(val_0, val_1);
}

// Load and combine 32 uint16_t values.
inline uint16x8_t LoadAndAdd32(const uint16_t* buf) {
  const uint16x8_t val_0 = vld1q_u16(buf);
  const uint16x8_t val_1 = vld1q_u16(buf + 8);
  const uint16x8_t val_2 = vld1q_u16(buf + 16);
  const uint16x8_t val_3 = vld1q_u16(buf + 24);
  return Add(val_0, val_1, val_2, val_3);
}

// Load and combine 64 uint16_t values.
inline uint16x8_t LoadAndAdd64(const uint16_t* buf) {
  const uint16x8_t val_0 = vld1q_u16(buf);
  const uint16x8_t val_1 = vld1q_u16(buf + 8);
  const uint16x8_t val_2 = vld1q_u16(buf + 16);
  const uint16x8_t val_3 = vld1q_u16(buf + 24);
  const uint16x8_t val_4 = vld1q_u16(buf + 32);
  const uint16x8_t val_5 = vld1q_u16(buf + 40);
  const uint16x8_t val_6 = vld1q_u16(buf + 48);
  const uint16x8_t val_7 = vld1q_u16(buf + 56);
  const uint16x8_t sum_0 = Add(val_0, val_1, val_2, val_3);
  const uint16x8_t sum_1 = Add(val_4, val_5, val_6, val_7);
  return vaddq_u16(sum_0, sum_1);
}

// |ref_[01]| each point to 1 << |ref[01]_size_log2| packed uint16_t values.
// If |use_ref_1| is false then only sum |ref_0|.
inline uint32x2_t DcSum_NEON(const void* LIBGAV1_RESTRICT ref_0,
                             const int ref_0_size_log2, const bool use_ref_1,
                             const void* LIBGAV1_RESTRICT ref_1,
                             const int ref_1_size_log2) {
  const auto* ref_0_u16 = static_cast<const uint16_t*>(ref_0);
  const auto* ref_1_u16 = static_cast<const uint16_t*>(ref_1);
  if (ref_0_size_log2 == 2) {
    const uint16x4_t val_0 = vld1_u16(ref_0_u16);
    if (use_ref_1) {
      switch (ref_1_size_log2) {
        case 2: {  // 4x4
          const uint16x4_t val_1 = vld1_u16(ref_1_u16);
          return Sum(vadd_u16(val_0, val_1));
        }
        case 3: {  // 4x8
          const uint16x8_t val_1 = vld1q_u16(ref_1_u16);
          const uint16x8_t sum_0 = vcombine_u16(vdup_n_u16(0), val_0);
          return Sum(vaddq_u16(sum_0, val_1));
        }
        case 4: {  // 4x16
          const uint16x8_t sum_0 = vcombine_u16(vdup_n_u16(0), val_0);
          const uint16x8_t sum_1 = LoadAndAdd16(ref_1_u16);
          return Sum(vaddq_u16(sum_0, sum_1));
        }
      }
    }
    // 4x1
    return Sum(val_0);
  }
  if (ref_0_size_log2 == 3) {
    const uint16x8_t val_0 = vld1q_u16(ref_0_u16);
    if (use_ref_1) {
      switch (ref_1_size_log2) {
        case 2: {  // 8x4
          const uint16x4_t val_1 = vld1_u16(ref_1_u16);
          const uint16x8_t sum_1 = vcombine_u16(vdup_n_u16(0), val_1);
          return Sum(vaddq_u16(val_0, sum_1));
        }
        case 3: {  // 8x8
          const uint16x8_t val_1 = vld1q_u16(ref_1_u16);
          return Sum(vaddq_u16(val_0, val_1));
        }
        case 4: {  // 8x16
          const uint16x8_t sum_1 = LoadAndAdd16(ref_1_u16);
          return Sum(vaddq_u16(val_0, sum_1));
        }
        case 5: {  // 8x32
          const uint16x8_t sum_1 = LoadAndAdd32(ref_1_u16);
          return Sum(vaddq_u16(val_0, sum_1));
        }
      }
    }
    // 8x1
    return Sum(val_0);
  }
  if (ref_0_size_log2 == 4) {
    const uint16x8_t sum_0 = LoadAndAdd16(ref_0_u16);
    if (use_ref_1) {
      switch (ref_1_size_log2) {
        case 2: {  // 16x4
          const uint16x4_t val_1 = vld1_u16(ref_1_u16);
          const uint16x8_t sum_1 = vcombine_u16(vdup_n_u16(0), val_1);
          return Sum(vaddq_u16(sum_0, sum_1));
        }
        case 3: {  // 16x8
          const uint16x8_t val_1 = vld1q_u16(ref_1_u16);
          return Sum(vaddq_u16(sum_0, val_1));
        }
        case 4: {  // 16x16
          const uint16x8_t sum_1 = LoadAndAdd16(ref_1_u16);
          return Sum(vaddq_u16(sum_0, sum_1));
        }
        case 5: {  // 16x32
          const uint16x8_t sum_1 = LoadAndAdd32(ref_1_u16);
          return Sum(vaddq_u16(sum_0, sum_1));
        }
        case 6: {  // 16x64
          const uint16x8_t sum_1 = LoadAndAdd64(ref_1_u16);
          return Sum(vaddq_u16(sum_0, sum_1));
        }
      }
    }
    // 16x1
    return Sum(sum_0);
  }
  if (ref_0_size_log2 == 5) {
    const uint16x8_t sum_0 = LoadAndAdd32(ref_0_u16);
    if (use_ref_1) {
      switch (ref_1_size_log2) {
        case 3: {  // 32x8
          const uint16x8_t val_1 = vld1q_u16(ref_1_u16);
          return Sum(vaddq_u16(sum_0, val_1));
        }
        case 4: {  // 32x16
          const uint16x8_t sum_1 = LoadAndAdd16(ref_1_u16);
          return Sum(vaddq_u16(sum_0, sum_1));
        }
        case 5: {  // 32x32
          const uint16x8_t sum_1 = LoadAndAdd32(ref_1_u16);
          return Sum(vaddq_u16(sum_0, sum_1));
        }
        case 6: {  // 32x64
          const uint16x8_t sum_1 = LoadAndAdd64(ref_1_u16);
          return Sum(vaddq_u16(sum_0, sum_1));
        }
      }
    }
    // 32x1
    return Sum(sum_0);
  }

  assert(ref_0_size_log2 == 6);
  const uint16x8_t sum_0 = LoadAndAdd64(ref_0_u16);
  if (use_ref_1) {
    switch (ref_1_size_log2) {
      case 4: {  // 64x16
        const uint16x8_t sum_1 = LoadAndAdd16(ref_1_u16);
        return Sum(vaddq_u16(sum_0, sum_1));
      }
      case 5: {  // 64x32
        const uint16x8_t sum_1 = LoadAndAdd32(ref_1_u16);
        return Sum(vaddq_u16(sum_0, sum_1));
      }
      case 6: {  // 64x64
        const uint16x8_t sum_1 = LoadAndAdd64(ref_1_u16);
        return Sum(vaddq_u16(sum_0, sum_1));
      }
    }
  }
  // 64x1
  return Sum(sum_0);
}

template <int width, int height>
inline void DcStore_NEON(void* const dest, ptrdiff_t stride,
                         const uint32x2_t dc) {
  auto* dest_u16 = static_cast<uint16_t*>(dest);
  ptrdiff_t stride_u16 = stride >> 1;
  const uint16x8_t dc_dup = vdupq_lane_u16(vreinterpret_u16_u32(dc), 0);
  if (width == 4) {
    int i = height - 1;
    do {
      vst1_u16(dest_u16, vget_low_u16(dc_dup));
      dest_u16 += stride_u16;
    } while (--i != 0);
    vst1_u16(dest_u16, vget_low_u16(dc_dup));
  } else if (width == 8) {
    int i = height - 1;
    do {
      vst1q_u16(dest_u16, dc_dup);
      dest_u16 += stride_u16;
    } while (--i != 0);
    vst1q_u16(dest_u16, dc_dup);
  } else if (width == 16) {
    int i = height - 1;
    do {
      vst1q_u16(dest_u16, dc_dup);
      vst1q_u16(dest_u16 + 8, dc_dup);
      dest_u16 += stride_u16;
    } while (--i != 0);
    vst1q_u16(dest_u16, dc_dup);
    vst1q_u16(dest_u16 + 8, dc_dup);
  } else if (width == 32) {
    int i = height - 1;
    do {
      vst1q_u16(dest_u16, dc_dup);
      vst1q_u16(dest_u16 + 8, dc_dup);
      vst1q_u16(dest_u16 + 16, dc_dup);
      vst1q_u16(dest_u16 + 24, dc_dup);
      dest_u16 += stride_u16;
    } while (--i != 0);
    vst1q_u16(dest_u16, dc_dup);
    vst1q_u16(dest_u16 + 8, dc_dup);
    vst1q_u16(dest_u16 + 16, dc_dup);
    vst1q_u16(dest_u16 + 24, dc_dup);
  } else {
    assert(width == 64);
    int i = height - 1;
    do {
      vst1q_u16(dest_u16, dc_dup);
      vst1q_u16(dest_u16 + 8, dc_dup);
      vst1q_u16(dest_u16 + 16, dc_dup);
      vst1q_u16(dest_u16 + 24, dc_dup);
      vst1q_u16(dest_u16 + 32, dc_dup);
      vst1q_u16(dest_u16 + 40, dc_dup);
      vst1q_u16(dest_u16 + 48, dc_dup);
      vst1q_u16(dest_u16 + 56, dc_dup);
      dest_u16 += stride_u16;
    } while (--i != 0);
    vst1q_u16(dest_u16, dc_dup);
    vst1q_u16(dest_u16 + 8, dc_dup);
    vst1q_u16(dest_u16 + 16, dc_dup);
    vst1q_u16(dest_u16 + 24, dc_dup);
    vst1q_u16(dest_u16 + 32, dc_dup);
    vst1q_u16(dest_u16 + 40, dc_dup);
    vst1q_u16(dest_u16 + 48, dc_dup);
    vst1q_u16(dest_u16 + 56, dc_dup);
  }
}

struct DcDefs {
  DcDefs() = delete;

  using _4x4 = DcPredFuncs_NEON<2, 2, DcSum_NEON, DcStore_NEON<4, 4>>;
  using _4x8 = DcPredFuncs_NEON<2, 3, DcSum_NEON, DcStore_NEON<4, 8>>;
  using _4x16 = DcPredFuncs_NEON<2, 4, DcSum_NEON, DcStore_NEON<4, 16>>;
  using _8x4 = DcPredFuncs_NEON<3, 2, DcSum_NEON, DcStore_NEON<8, 4>>;
  using _8x8 = DcPredFuncs_NEON<3, 3, DcSum_NEON, DcStore_NEON<8, 8>>;
  using _8x16 = DcPredFuncs_NEON<3, 4, DcSum_NEON, DcStore_NEON<8, 16>>;
  using _8x32 = DcPredFuncs_NEON<3, 5, DcSum_NEON, DcStore_NEON<8, 32>>;
  using _16x4 = DcPredFuncs_NEON<4, 2, DcSum_NEON, DcStore_NEON<16, 4>>;
  using _16x8 = DcPredFuncs_NEON<4, 3, DcSum_NEON, DcStore_NEON<16, 8>>;
  using _16x16 = DcPredFuncs_NEON<4, 4, DcSum_NEON, DcStore_NEON<16, 16>>;
  using _16x32 = DcPredFuncs_NEON<4, 5, DcSum_NEON, DcStore_NEON<16, 32>>;
  using _16x64 = DcPredFuncs_NEON<4, 6, DcSum_NEON, DcStore_NEON<16, 64>>;
  using _32x8 = DcPredFuncs_NEON<5, 3, DcSum_NEON, DcStore_NEON<32, 8>>;
  using _32x16 = DcPredFuncs_NEON<5, 4, DcSum_NEON, DcStore_NEON<32, 16>>;
  using _32x32 = DcPredFuncs_NEON<5, 5, DcSum_NEON, DcStore_NEON<32, 32>>;
  using _32x64 = DcPredFuncs_NEON<5, 6, DcSum_NEON, DcStore_NEON<32, 64>>;
  using _64x16 = DcPredFuncs_NEON<6, 4, DcSum_NEON, DcStore_NEON<64, 16>>;
  using _64x32 = DcPredFuncs_NEON<6, 5, DcSum_NEON, DcStore_NEON<64, 32>>;
  using _64x64 = DcPredFuncs_NEON<6, 6, DcSum_NEON, DcStore_NEON<64, 64>>;
};

// IntraPredFuncs_NEON::Horizontal -- duplicate left column across all rows

template <int block_height>
void Horizontal4xH_NEON(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                        const void* /*top_row*/,
                        const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left = static_cast<const uint16_t*>(left_column);
  auto* dst = static_cast<uint8_t*>(dest);
  int y = 0;
  do {
    auto* dst16 = reinterpret_cast<uint16_t*>(dst);
    const uint16x4_t row = vld1_dup_u16(left + y);
    vst1_u16(dst16, row);
    dst += stride;
  } while (++y < block_height);
}

template <int block_height>
void Horizontal8xH_NEON(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                        const void* /*top_row*/,
                        const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left = static_cast<const uint16_t*>(left_column);
  auto* dst = static_cast<uint8_t*>(dest);
  int y = 0;
  do {
    auto* dst16 = reinterpret_cast<uint16_t*>(dst);
    const uint16x8_t row = vld1q_dup_u16(left + y);
    vst1q_u16(dst16, row);
    dst += stride;
  } while (++y < block_height);
}

template <int block_height>
void Horizontal16xH_NEON(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                         const void* /*top_row*/,
                         const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left = static_cast<const uint16_t*>(left_column);
  auto* dst = static_cast<uint8_t*>(dest);
  int y = 0;
  do {
    const uint16x8_t row0 = vld1q_dup_u16(left + y);
    const uint16x8_t row1 = vld1q_dup_u16(left + y + 1);
    auto* dst16 = reinterpret_cast<uint16_t*>(dst);
    vst1q_u16(dst16, row0);
    vst1q_u16(dst16 + 8, row0);
    dst += stride;
    dst16 = reinterpret_cast<uint16_t*>(dst);
    vst1q_u16(dst16, row1);
    vst1q_u16(dst16 + 8, row1);
    dst += stride;
    y += 2;
  } while (y < block_height);
}

template <int block_height>
void Horizontal32xH_NEON(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                         const void* /*top_row*/,
                         const void* LIBGAV1_RESTRICT const left_column) {
  const auto* const left = static_cast<const uint16_t*>(left_column);
  auto* dst = static_cast<uint8_t*>(dest);
  int y = 0;
  do {
    const uint16x8_t row0 = vld1q_dup_u16(left + y);
    const uint16x8_t row1 = vld1q_dup_u16(left + y + 1);
    auto* dst16 = reinterpret_cast<uint16_t*>(dst);
    vst1q_u16(dst16, row0);
    vst1q_u16(dst16 + 8, row0);
    vst1q_u16(dst16 + 16, row0);
    vst1q_u16(dst16 + 24, row0);
    dst += stride;
    dst16 = reinterpret_cast<uint16_t*>(dst);
    vst1q_u16(dst16, row1);
    vst1q_u16(dst16 + 8, row1);
    vst1q_u16(dst16 + 16, row1);
    vst1q_u16(dst16 + 24, row1);
    dst += stride;
    y += 2;
  } while (y < block_height);
}

// IntraPredFuncs_NEON::Vertical -- copy top row to all rows

template <int block_height>
void Vertical4xH_NEON(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                      const void* LIBGAV1_RESTRICT const top_row,
                      const void* const /*left_column*/) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  auto* dst = static_cast<uint8_t*>(dest);
  const uint8x8_t row = vld1_u8(top);
  int y = block_height;
  do {
    vst1_u8(dst, row);
    dst += stride;
  } while (--y != 0);
}

template <int block_height>
void Vertical8xH_NEON(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                      const void* LIBGAV1_RESTRICT const top_row,
                      const void* const /*left_column*/) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  auto* dst = static_cast<uint8_t*>(dest);
  const uint8x16_t row = vld1q_u8(top);
  int y = block_height;
  do {
    vst1q_u8(dst, row);
    dst += stride;
  } while (--y != 0);
}

template <int block_height>
void Vertical16xH_NEON(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                       const void* LIBGAV1_RESTRICT const top_row,
                       const void* const /*left_column*/) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  auto* dst = static_cast<uint8_t*>(dest);
  const uint8x16_t row0 = vld1q_u8(top);
  const uint8x16_t row1 = vld1q_u8(top + 16);
  int y = block_height;
  do {
    vst1q_u8(dst, row0);
    vst1q_u8(dst + 16, row1);
    dst += stride;
    vst1q_u8(dst, row0);
    vst1q_u8(dst + 16, row1);
    dst += stride;
    y -= 2;
  } while (y != 0);
}

template <int block_height>
void Vertical32xH_NEON(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                       const void* LIBGAV1_RESTRICT const top_row,
                       const void* const /*left_column*/) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  auto* dst = static_cast<uint8_t*>(dest);
  const uint8x16_t row0 = vld1q_u8(top);
  const uint8x16_t row1 = vld1q_u8(top + 16);
  const uint8x16_t row2 = vld1q_u8(top + 32);
  const uint8x16_t row3 = vld1q_u8(top + 48);
  int y = block_height;
  do {
    vst1q_u8(dst, row0);
    vst1q_u8(dst + 16, row1);
    vst1q_u8(dst + 32, row2);
    vst1q_u8(dst + 48, row3);
    dst += stride;
    vst1q_u8(dst, row0);
    vst1q_u8(dst + 16, row1);
    vst1q_u8(dst + 32, row2);
    vst1q_u8(dst + 48, row3);
    dst += stride;
    y -= 2;
  } while (y != 0);
}

template <int block_height>
void Vertical64xH_NEON(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                       const void* LIBGAV1_RESTRICT const top_row,
                       const void* const /*left_column*/) {
  const auto* const top = static_cast<const uint8_t*>(top_row);
  auto* dst = static_cast<uint8_t*>(dest);
  const uint8x16_t row0 = vld1q_u8(top);
  const uint8x16_t row1 = vld1q_u8(top + 16);
  const uint8x16_t row2 = vld1q_u8(top + 32);
  const uint8x16_t row3 = vld1q_u8(top + 48);
  const uint8x16_t row4 = vld1q_u8(top + 64);
  const uint8x16_t row5 = vld1q_u8(top + 80);
  const uint8x16_t row6 = vld1q_u8(top + 96);
  const uint8x16_t row7 = vld1q_u8(top + 112);
  int y = block_height;
  do {
    vst1q_u8(dst, row0);
    vst1q_u8(dst + 16, row1);
    vst1q_u8(dst + 32, row2);
    vst1q_u8(dst + 48, row3);
    vst1q_u8(dst + 64, row4);
    vst1q_u8(dst + 80, row5);
    vst1q_u8(dst + 96, row6);
    vst1q_u8(dst + 112, row7);
    dst += stride;
    vst1q_u8(dst, row0);
    vst1q_u8(dst + 16, row1);
    vst1q_u8(dst + 32, row2);
    vst1q_u8(dst + 48, row3);
    vst1q_u8(dst + 64, row4);
    vst1q_u8(dst + 80, row5);
    vst1q_u8(dst + 96, row6);
    vst1q_u8(dst + 112, row7);
    dst += stride;
    y -= 2;
  } while (y != 0);
}

template <int height>
inline void Paeth4xH_NEON(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                          const void* LIBGAV1_RESTRICT const top_ptr,
                          const void* LIBGAV1_RESTRICT const left_ptr) {
  auto* dst = static_cast<uint8_t*>(dest);
  const auto* const top_row = static_cast<const uint16_t*>(top_ptr);
  const auto* const left_col = static_cast<const uint16_t*>(left_ptr);

  const uint16x4_t top_left = vdup_n_u16(top_row[-1]);
  const uint16x4_t top_left_x2 = vshl_n_u16(top_left, 1);
  const uint16x4_t top = vld1_u16(top_row);

  for (int y = 0; y < height; ++y) {
    auto* dst16 = reinterpret_cast<uint16_t*>(dst);
    const uint16x4_t left = vdup_n_u16(left_col[y]);

    const uint16x4_t left_dist = vabd_u16(top, top_left);
    const uint16x4_t top_dist = vabd_u16(left, top_left);
    const uint16x4_t top_left_dist = vabd_u16(vadd_u16(top, left), top_left_x2);

    const uint16x4_t left_le_top = vcle_u16(left_dist, top_dist);
    const uint16x4_t left_le_top_left = vcle_u16(left_dist, top_left_dist);
    const uint16x4_t top_le_top_left = vcle_u16(top_dist, top_left_dist);

    // if (left_dist <= top_dist && left_dist <= top_left_dist)
    const uint16x4_t left_mask = vand_u16(left_le_top, left_le_top_left);
    //   dest[x] = left_column[y];
    // Fill all the unused spaces with 'top'. They will be overwritten when
    // the positions for top_left are known.
    uint16x4_t result = vbsl_u16(left_mask, left, top);
    // else if (top_dist <= top_left_dist)
    //   dest[x] = top_row[x];
    // Add these values to the mask. They were already set.
    const uint16x4_t left_or_top_mask = vorr_u16(left_mask, top_le_top_left);
    // else
    //   dest[x] = top_left;
    result = vbsl_u16(left_or_top_mask, result, top_left);

    vst1_u16(dst16, result);
    dst += stride;
  }
}

template <int height>
inline void Paeth8xH_NEON(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                          const void* LIBGAV1_RESTRICT const top_ptr,
                          const void* LIBGAV1_RESTRICT const left_ptr) {
  auto* dst = static_cast<uint8_t*>(dest);
  const auto* const top_row = static_cast<const uint16_t*>(top_ptr);
  const auto* const left_col = static_cast<const uint16_t*>(left_ptr);

  const uint16x8_t top_left = vdupq_n_u16(top_row[-1]);
  const uint16x8_t top_left_x2 = vshlq_n_u16(top_left, 1);
  const uint16x8_t top = vld1q_u16(top_row);

  for (int y = 0; y < height; ++y) {
    auto* dst16 = reinterpret_cast<uint16_t*>(dst);
    const uint16x8_t left = vdupq_n_u16(left_col[y]);

    const uint16x8_t left_dist = vabdq_u16(top, top_left);
    const uint16x8_t top_dist = vabdq_u16(left, top_left);
    const uint16x8_t top_left_dist =
        vabdq_u16(vaddq_u16(top, left), top_left_x2);

    const uint16x8_t left_le_top = vcleq_u16(left_dist, top_dist);
    const uint16x8_t left_le_top_left = vcleq_u16(left_dist, top_left_dist);
    const uint16x8_t top_le_top_left = vcleq_u16(top_dist, top_left_dist);

    // if (left_dist <= top_dist && left_dist <= top_left_dist)
    const uint16x8_t left_mask = vandq_u16(left_le_top, left_le_top_left);
    //   dest[x] = left_column[y];
    // Fill all the unused spaces with 'top'. They will be overwritten when
    // the positions for top_left are known.
    uint16x8_t result = vbslq_u16(left_mask, left, top);
    // else if (top_dist <= top_left_dist)
    //   dest[x] = top_row[x];
    // Add these values to the mask. They were already set.
    const uint16x8_t left_or_top_mask = vorrq_u16(left_mask, top_le_top_left);
    // else
    //   dest[x] = top_left;
    result = vbslq_u16(left_or_top_mask, result, top_left);

    vst1q_u16(dst16, result);
    dst += stride;
  }
}

// For 16xH and above.
template <int width, int height>
inline void PaethWxH_NEON(void* LIBGAV1_RESTRICT const dest, ptrdiff_t stride,
                          const void* LIBGAV1_RESTRICT const top_ptr,
                          const void* LIBGAV1_RESTRICT const left_ptr) {
  auto* dst = static_cast<uint8_t*>(dest);
  const auto* const top_row = static_cast<const uint16_t*>(top_ptr);
  const auto* const left_col = static_cast<const uint16_t*>(left_ptr);

  const uint16x8_t top_left = vdupq_n_u16(top_row[-1]);
  const uint16x8_t top_left_x2 = vshlq_n_u16(top_left, 1);

  uint16x8_t top[width >> 3];
  for (int i = 0; i < width >> 3; ++i) {
    top[i] = vld1q_u16(top_row + (i << 3));
  }

  for (int y = 0; y < height; ++y) {
    auto* dst_x = reinterpret_cast<uint16_t*>(dst);
    const uint16x8_t left = vdupq_n_u16(left_col[y]);
    const uint16x8_t top_dist = vabdq_u16(left, top_left);

    for (int i = 0; i < (width >> 3); ++i) {
      const uint16x8_t left_dist = vabdq_u16(top[i], top_left);
      const uint16x8_t top_left_dist =
          vabdq_u16(vaddq_u16(top[i], left), top_left_x2);

      const uint16x8_t left_le_top = vcleq_u16(left_dist, top_dist);
      const uint16x8_t left_le_top_left = vcleq_u16(left_dist, top_left_dist);
      const uint16x8_t top_le_top_left = vcleq_u16(top_dist, top_left_dist);

      // if (left_dist <= top_dist && left_dist <= top_left_dist)
      const uint16x8_t left_mask = vandq_u16(left_le_top, left_le_top_left);
      //   dest[x] = left_column[y];
      // Fill all the unused spaces with 'top'. They will be overwritten when
      // the positions for top_left are known.
      uint16x8_t result = vbslq_u16(left_mask, left, top[i]);
      // else if (top_dist <= top_left_dist)
      //   dest[x] = top_row[x];
      // Add these values to the mask. They were already set.
      const uint16x8_t left_or_top_mask = vorrq_u16(left_mask, top_le_top_left);
      // else
      //   dest[x] = top_left;
      result = vbslq_u16(left_or_top_mask, result, top_left);

      vst1q_u16(dst_x, result);
      dst_x += 8;
    }
    dst += stride;
  }
}

void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth10);
  assert(dsp != nullptr);
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDcTop] =
      DcDefs::_4x4::DcTop;
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDcLeft] =
      DcDefs::_4x4::DcLeft;
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorDc] =
      DcDefs::_4x4::Dc;
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorVertical] =
      Vertical4xH_NEON<4>;
  dsp->intra_predictors[kTransformSize4x4][kIntraPredictorPaeth] =
      Paeth4xH_NEON<4>;

  // 4x8
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorDcTop] =
      DcDefs::_4x8::DcTop;
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorDcLeft] =
      DcDefs::_4x8::DcLeft;
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorDc] =
      DcDefs::_4x8::Dc;
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorHorizontal] =
      Horizontal4xH_NEON<8>;
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorVertical] =
      Vertical4xH_NEON<8>;
  dsp->intra_predictors[kTransformSize4x8][kIntraPredictorPaeth] =
      Paeth4xH_NEON<8>;

  // 4x16
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorDcTop] =
      DcDefs::_4x16::DcTop;
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorDcLeft] =
      DcDefs::_4x16::DcLeft;
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorDc] =
      DcDefs::_4x16::Dc;
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorHorizontal] =
      Horizontal4xH_NEON<16>;
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorVertical] =
      Vertical4xH_NEON<16>;
  dsp->intra_predictors[kTransformSize4x16][kIntraPredictorPaeth] =
      Paeth4xH_NEON<16>;

  // 8x4
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorDcTop] =
      DcDefs::_8x4::DcTop;
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorDcLeft] =
      DcDefs::_8x4::DcLeft;
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorDc] =
      DcDefs::_8x4::Dc;
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorVertical] =
      Vertical8xH_NEON<4>;
  dsp->intra_predictors[kTransformSize8x4][kIntraPredictorPaeth] =
      Paeth8xH_NEON<4>;

  // 8x8
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorDcTop] =
      DcDefs::_8x8::DcTop;
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorDcLeft] =
      DcDefs::_8x8::DcLeft;
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorDc] =
      DcDefs::_8x8::Dc;
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorHorizontal] =
      Horizontal8xH_NEON<8>;
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorVertical] =
      Vertical8xH_NEON<8>;
  dsp->intra_predictors[kTransformSize8x8][kIntraPredictorPaeth] =
      Paeth8xH_NEON<8>;

  // 8x16
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorDcTop] =
      DcDefs::_8x16::DcTop;
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorDcLeft] =
      DcDefs::_8x16::DcLeft;
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorDc] =
      DcDefs::_8x16::Dc;
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorVertical] =
      Vertical8xH_NEON<16>;
  dsp->intra_predictors[kTransformSize8x16][kIntraPredictorPaeth] =
      Paeth8xH_NEON<16>;

  // 8x32
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorDcTop] =
      DcDefs::_8x32::DcTop;
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorDcLeft] =
      DcDefs::_8x32::DcLeft;
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorDc] =
      DcDefs::_8x32::Dc;
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorHorizontal] =
      Horizontal8xH_NEON<32>;
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorVertical] =
      Vertical8xH_NEON<32>;
  dsp->intra_predictors[kTransformSize8x32][kIntraPredictorPaeth] =
      Paeth8xH_NEON<32>;

  // 16x4
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorDcTop] =
      DcDefs::_16x4::DcTop;
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorDcLeft] =
      DcDefs::_16x4::DcLeft;
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorDc] =
      DcDefs::_16x4::Dc;
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorVertical] =
      Vertical16xH_NEON<4>;
  dsp->intra_predictors[kTransformSize16x4][kIntraPredictorPaeth] =
      PaethWxH_NEON<16, 4>;

  // 16x8
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorDcTop] =
      DcDefs::_16x8::DcTop;
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorDcLeft] =
      DcDefs::_16x8::DcLeft;
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorDc] =
      DcDefs::_16x8::Dc;
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorHorizontal] =
      Horizontal16xH_NEON<8>;
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorVertical] =
      Vertical16xH_NEON<8>;
  dsp->intra_predictors[kTransformSize16x8][kIntraPredictorPaeth] =
      PaethWxH_NEON<16, 8>;

  // 16x16
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorDcTop] =
      DcDefs::_16x16::DcTop;
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorDcLeft] =
      DcDefs::_16x16::DcLeft;
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorDc] =
      DcDefs::_16x16::Dc;
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorVertical] =
      Vertical16xH_NEON<16>;
  dsp->intra_predictors[kTransformSize16x16][kIntraPredictorPaeth] =
      PaethWxH_NEON<16, 16>;

  // 16x32
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorDcTop] =
      DcDefs::_16x32::DcTop;
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorDcLeft] =
      DcDefs::_16x32::DcLeft;
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorDc] =
      DcDefs::_16x32::Dc;
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorVertical] =
      Vertical16xH_NEON<32>;
  dsp->intra_predictors[kTransformSize16x32][kIntraPredictorPaeth] =
      PaethWxH_NEON<16, 32>;

  // 16x64
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorDcTop] =
      DcDefs::_16x64::DcTop;
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorDcLeft] =
      DcDefs::_16x64::DcLeft;
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorDc] =
      DcDefs::_16x64::Dc;
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorVertical] =
      Vertical16xH_NEON<64>;
  dsp->intra_predictors[kTransformSize16x64][kIntraPredictorPaeth] =
      PaethWxH_NEON<16, 64>;

  // 32x8
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorDcTop] =
      DcDefs::_32x8::DcTop;
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorDcLeft] =
      DcDefs::_32x8::DcLeft;
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorDc] =
      DcDefs::_32x8::Dc;
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorVertical] =
      Vertical32xH_NEON<8>;
  dsp->intra_predictors[kTransformSize32x8][kIntraPredictorPaeth] =
      PaethWxH_NEON<32, 8>;

  // 32x16
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorDcTop] =
      DcDefs::_32x16::DcTop;
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorDcLeft] =
      DcDefs::_32x16::DcLeft;
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorDc] =
      DcDefs::_32x16::Dc;
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorVertical] =
      Vertical32xH_NEON<16>;
  dsp->intra_predictors[kTransformSize32x16][kIntraPredictorPaeth] =
      PaethWxH_NEON<32, 16>;

  // 32x32
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorDcTop] =
      DcDefs::_32x32::DcTop;
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorDcLeft] =
      DcDefs::_32x32::DcLeft;
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorDc] =
      DcDefs::_32x32::Dc;
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorVertical] =
      Vertical32xH_NEON<32>;
  dsp->intra_predictors[kTransformSize32x32][kIntraPredictorPaeth] =
      PaethWxH_NEON<32, 32>;

  // 32x64
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorDcTop] =
      DcDefs::_32x64::DcTop;
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorDcLeft] =
      DcDefs::_32x64::DcLeft;
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorDc] =
      DcDefs::_32x64::Dc;
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorHorizontal] =
      Horizontal32xH_NEON<64>;
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorVertical] =
      Vertical32xH_NEON<64>;
  dsp->intra_predictors[kTransformSize32x64][kIntraPredictorPaeth] =
      PaethWxH_NEON<32, 64>;

  // 64x16
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorDcTop] =
      DcDefs::_64x16::DcTop;
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorDcLeft] =
      DcDefs::_64x16::DcLeft;
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorDc] =
      DcDefs::_64x16::Dc;
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorVertical] =
      Vertical64xH_NEON<16>;
  dsp->intra_predictors[kTransformSize64x16][kIntraPredictorPaeth] =
      PaethWxH_NEON<64, 16>;

  // 64x32
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorDcTop] =
      DcDefs::_64x32::DcTop;
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorDcLeft] =
      DcDefs::_64x32::DcLeft;
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorDc] =
      DcDefs::_64x32::Dc;
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorVertical] =
      Vertical64xH_NEON<32>;
  dsp->intra_predictors[kTransformSize64x32][kIntraPredictorPaeth] =
      PaethWxH_NEON<64, 32>;

  // 64x64
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorDcTop] =
      DcDefs::_64x64::DcTop;
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorDcLeft] =
      DcDefs::_64x64::DcLeft;
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorDc] =
      DcDefs::_64x64::Dc;
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorVertical] =
      Vertical64xH_NEON<64>;
  dsp->intra_predictors[kTransformSize64x64][kIntraPredictorPaeth] =
      PaethWxH_NEON<64, 64>;
}

}  // namespace
}  // namespace high_bitdepth
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

void IntraPredInit_NEON() {
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

void IntraPredInit_NEON() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_ENABLE_NEON
