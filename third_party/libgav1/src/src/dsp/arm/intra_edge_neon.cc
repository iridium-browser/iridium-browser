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

#include "src/dsp/intra_edge.h"
#include "src/utils/cpu.h"

#if LIBGAV1_ENABLE_NEON

#include <arm_neon.h>

#include <algorithm>
#include <cassert>

#include "src/dsp/arm/common_neon.h"
#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/utils/common.h"

namespace libgav1 {
namespace dsp {
namespace {

// Simplified version of intra_edge.cc:kKernels[][]. Only |strength| 1 and 2 are
// required.
constexpr int kKernelsNEON[3][2] = {{4, 8}, {5, 6}};

}  // namespace

namespace low_bitdepth {
namespace {

void IntraEdgeFilter_NEON(void* buffer, const int size, const int strength) {
  assert(strength == 1 || strength == 2 || strength == 3);
  const int kernel_index = strength - 1;
  auto* const dst_buffer = static_cast<uint8_t*>(buffer);

  // The first element is not written out (but it is input) so the number of
  // elements written is |size| - 1.
  if (size == 1) return;

  const uint8x16_t v_index = vcombine_u8(vcreate_u8(0x0706050403020100),
                                         vcreate_u8(0x0f0e0d0c0b0a0908));
  // |strength| 1 and 2 use a 3 tap filter.
  if (strength < 3) {
    // The last value requires extending the buffer (duplicating
    // |dst_buffer[size - 1]). Calculate it here to avoid extra processing in
    // neon.
    const uint8_t last_val = RightShiftWithRounding(
        kKernelsNEON[kernel_index][0] * dst_buffer[size - 2] +
            kKernelsNEON[kernel_index][1] * dst_buffer[size - 1] +
            kKernelsNEON[kernel_index][0] * dst_buffer[size - 1],
        4);

    const uint8x8_t krn1 = vdup_n_u8(kKernelsNEON[kernel_index][1]);

    // The first value we need gets overwritten by the output from the
    // previous iteration.
    uint8x16_t src_0 = vld1q_u8(dst_buffer);
    int i = 1;

    // Process blocks until there are less than 16 values remaining.
    for (; i < size - 15; i += 16) {
      // Loading these at the end of the block with |src_0| will read past the
      // end of |top_row_data[160]|, the source of |buffer|.
      const uint8x16_t src_1 = vld1q_u8(dst_buffer + i);
      const uint8x16_t src_2 = vld1q_u8(dst_buffer + i + 1);
      uint16x8_t sum_lo = vaddl_u8(vget_low_u8(src_0), vget_low_u8(src_2));
      sum_lo = vmulq_n_u16(sum_lo, kKernelsNEON[kernel_index][0]);
      sum_lo = vmlal_u8(sum_lo, vget_low_u8(src_1), krn1);
      uint16x8_t sum_hi = vaddl_u8(vget_high_u8(src_0), vget_high_u8(src_2));
      sum_hi = vmulq_n_u16(sum_hi, kKernelsNEON[kernel_index][0]);
      sum_hi = vmlal_u8(sum_hi, vget_high_u8(src_1), krn1);

      const uint8x16_t result =
          vcombine_u8(vrshrn_n_u16(sum_lo, 4), vrshrn_n_u16(sum_hi, 4));

      // Load the next row before overwriting. This loads an extra 15 values
      // past |size| on the trailing iteration.
      src_0 = vld1q_u8(dst_buffer + i + 15);

      vst1q_u8(dst_buffer + i, result);
    }

    // The last output value |last_val| was already calculated so if
    // |remainder| == 1 then we don't have to do anything.
    const int remainder = (size - 1) & 0xf;
    if (remainder > 1) {
      const uint8x16_t src_1 = vld1q_u8(dst_buffer + i);
      const uint8x16_t src_2 = vld1q_u8(dst_buffer + i + 1);

      uint16x8_t sum_lo = vaddl_u8(vget_low_u8(src_0), vget_low_u8(src_2));
      sum_lo = vmulq_n_u16(sum_lo, kKernelsNEON[kernel_index][0]);
      sum_lo = vmlal_u8(sum_lo, vget_low_u8(src_1), krn1);
      uint16x8_t sum_hi = vaddl_u8(vget_high_u8(src_0), vget_high_u8(src_2));
      sum_hi = vmulq_n_u16(sum_hi, kKernelsNEON[kernel_index][0]);
      sum_hi = vmlal_u8(sum_hi, vget_high_u8(src_1), krn1);

      const uint8x16_t result =
          vcombine_u8(vrshrn_n_u16(sum_lo, 4), vrshrn_n_u16(sum_hi, 4));
      const uint8x16_t v_remainder = vdupq_n_u8(remainder);
      // Create over write mask.
      const uint8x16_t mask = vcleq_u8(v_remainder, v_index);
      const uint8x16_t dst_remainder = vbslq_u8(mask, src_1, result);
      vst1q_u8(dst_buffer + i, dst_remainder);
    }

    dst_buffer[size - 1] = last_val;
    return;
  }

  assert(strength == 3);
  // 5 tap filter. The first element requires duplicating |buffer[0]| and the
  // last two elements require duplicating |buffer[size - 1]|.
  uint8_t special_vals[3];
  special_vals[0] = RightShiftWithRounding(
      (dst_buffer[0] << 1) + (dst_buffer[0] << 2) + (dst_buffer[1] << 2) +
          (dst_buffer[2] << 2) + (dst_buffer[3] << 1),
      4);
  // Clamp index for very small |size| values.
  const int first_index_min = std::max(size - 4, 0);
  const int second_index_min = std::max(size - 3, 0);
  const int third_index_min = std::max(size - 2, 0);
  special_vals[1] = RightShiftWithRounding(
      (dst_buffer[first_index_min] << 1) + (dst_buffer[second_index_min] << 2) +
          (dst_buffer[third_index_min] << 2) + (dst_buffer[size - 1] << 2) +
          (dst_buffer[size - 1] << 1),
      4);
  special_vals[2] = RightShiftWithRounding(
      (dst_buffer[second_index_min] << 1) + (dst_buffer[third_index_min] << 2) +
          // x << 2 + x << 2 == x << 3
          (dst_buffer[size - 1] << 3) + (dst_buffer[size - 1] << 1),
      4);

  // The first two values we need get overwritten by the output from the
  // previous iteration.
  uint8x16_t src_0 = vld1q_u8(dst_buffer - 1);
  uint8x16_t src_1 = vld1q_u8(dst_buffer);
  int i = 1;

  for (; i < size - 15; i += 16) {
    // Loading these at the end of the block with |src_[01]| will read past
    // the end of |top_row_data[160]|, the source of |buffer|.
    const uint8x16_t src_2 = vld1q_u8(dst_buffer + i);
    const uint8x16_t src_3 = vld1q_u8(dst_buffer + i + 1);
    const uint8x16_t src_4 = vld1q_u8(dst_buffer + i + 2);

    uint16x8_t sum_lo =
        vshlq_n_u16(vaddl_u8(vget_low_u8(src_0), vget_low_u8(src_4)), 1);
    const uint16x8_t sum_123_lo = vaddw_u8(
        vaddl_u8(vget_low_u8(src_1), vget_low_u8(src_2)), vget_low_u8(src_3));
    sum_lo = vaddq_u16(sum_lo, vshlq_n_u16(sum_123_lo, 2));

    uint16x8_t sum_hi =
        vshlq_n_u16(vaddl_u8(vget_high_u8(src_0), vget_high_u8(src_4)), 1);
    const uint16x8_t sum_123_hi =
        vaddw_u8(vaddl_u8(vget_high_u8(src_1), vget_high_u8(src_2)),
                 vget_high_u8(src_3));
    sum_hi = vaddq_u16(sum_hi, vshlq_n_u16(sum_123_hi, 2));

    const uint8x16_t result =
        vcombine_u8(vrshrn_n_u16(sum_lo, 4), vrshrn_n_u16(sum_hi, 4));

    src_0 = vld1q_u8(dst_buffer + i + 14);
    src_1 = vld1q_u8(dst_buffer + i + 15);

    vst1q_u8(dst_buffer + i, result);
  }

  const int remainder = (size - 1) & 0xf;
  // Like the 3 tap but if there are two remaining values we have already
  // calculated them.
  if (remainder > 2) {
    const uint8x16_t src_2 = vld1q_u8(dst_buffer + i);
    const uint8x16_t src_3 = vld1q_u8(dst_buffer + i + 1);
    const uint8x16_t src_4 = vld1q_u8(dst_buffer + i + 2);

    uint16x8_t sum_lo =
        vshlq_n_u16(vaddl_u8(vget_low_u8(src_0), vget_low_u8(src_4)), 1);
    const uint16x8_t sum_123_lo = vaddw_u8(
        vaddl_u8(vget_low_u8(src_1), vget_low_u8(src_2)), vget_low_u8(src_3));
    sum_lo = vaddq_u16(sum_lo, vshlq_n_u16(sum_123_lo, 2));

    uint16x8_t sum_hi =
        vshlq_n_u16(vaddl_u8(vget_high_u8(src_0), vget_high_u8(src_4)), 1);
    const uint16x8_t sum_123_hi =
        vaddw_u8(vaddl_u8(vget_high_u8(src_1), vget_high_u8(src_2)),
                 vget_high_u8(src_3));
    sum_hi = vaddq_u16(sum_hi, vshlq_n_u16(sum_123_hi, 2));

    const uint8x16_t result =
        vcombine_u8(vrshrn_n_u16(sum_lo, 4), vrshrn_n_u16(sum_hi, 4));
    const uint8x16_t v_remainder = vdupq_n_u8(remainder);
    // Create over write mask.
    const uint8x16_t mask = vcleq_u8(v_remainder, v_index);
    const uint8x16_t dst_remainder = vbslq_u8(mask, src_2, result);
    vst1q_u8(dst_buffer + i, dst_remainder);
  }

  dst_buffer[1] = special_vals[0];
  // Avoid overwriting |dst_buffer[0]|.
  if (size > 2) dst_buffer[size - 2] = special_vals[1];
  dst_buffer[size - 1] = special_vals[2];
}

// (-|src0| + |src1| * 9 + |src2| * 9 - |src3|) >> 4
uint8x8_t Upsample(const uint8x8_t src0, const uint8x8_t src1,
                   const uint8x8_t src2, const uint8x8_t src3) {
  const uint16x8_t middle = vmulq_n_u16(vaddl_u8(src1, src2), 9);
  const uint16x8_t ends = vaddl_u8(src0, src3);
  const int16x8_t sum =
      vsubq_s16(vreinterpretq_s16_u16(middle), vreinterpretq_s16_u16(ends));
  return vqrshrun_n_s16(sum, 4);
}

void IntraEdgeUpsampler_NEON(void* buffer, const int size) {
  assert(size % 4 == 0 && size <= 16);
  auto* const pixel_buffer = static_cast<uint8_t*>(buffer);
  // This is OK because we don't read this value for |size| 4 or 8 but if we
  // write |pixel_buffer[size]| and then vld() it, that seems to introduce
  // some latency.
  pixel_buffer[-2] = pixel_buffer[-1];
  if (size == 4) {
    // This uses one load and two vtbl() which is better than 4x Load{Lo,Hi}4().
    const uint8x8_t src = vld1_u8(pixel_buffer - 1);
    // The outside values are negated so put those in the same vector.
    const uint8x8_t src03 = vtbl1_u8(src, vcreate_u8(0x0404030202010000));
    // Reverse |src1| and |src2| so we can use |src2| for the interleave at the
    // end.
    const uint8x8_t src21 = vtbl1_u8(src, vcreate_u8(0x0302010004030201));

    const uint16x8_t middle = vmull_u8(src21, vdup_n_u8(9));
    const int16x8_t half_sum = vsubq_s16(
        vreinterpretq_s16_u16(middle), vreinterpretq_s16_u16(vmovl_u8(src03)));
    const int16x4_t sum =
        vadd_s16(vget_low_s16(half_sum), vget_high_s16(half_sum));
    const uint8x8_t result = vqrshrun_n_s16(vcombine_s16(sum, sum), 4);

    vst1_u8(pixel_buffer - 1, InterleaveLow8(result, src21));
    return;
  }
  if (size == 8) {
    // Likewise, one load + multiple vtbls seems preferred to multiple loads.
    const uint8x16_t src = vld1q_u8(pixel_buffer - 1);
    const uint8x8_t src0 = VQTbl1U8(src, vcreate_u8(0x0605040302010000));
    const uint8x8_t src1 = vget_low_u8(src);
    const uint8x8_t src2 = VQTbl1U8(src, vcreate_u8(0x0807060504030201));
    const uint8x8_t src3 = VQTbl1U8(src, vcreate_u8(0x0808070605040302));

    const uint8x8x2_t output = {Upsample(src0, src1, src2, src3), src2};
    vst2_u8(pixel_buffer - 1, output);
    return;
  }
  assert(size == 12 || size == 16);
  // Extend the input borders to avoid branching later.
  pixel_buffer[size] = pixel_buffer[size - 1];
  const uint8x16_t src0 = vld1q_u8(pixel_buffer - 2);
  const uint8x16_t src1 = vld1q_u8(pixel_buffer - 1);
  const uint8x16_t src2 = vld1q_u8(pixel_buffer);
  const uint8x16_t src3 = vld1q_u8(pixel_buffer + 1);

  const uint8x8_t result_lo = Upsample(vget_low_u8(src0), vget_low_u8(src1),
                                       vget_low_u8(src2), vget_low_u8(src3));

  const uint8x8x2_t output_lo = {result_lo, vget_low_u8(src2)};
  vst2_u8(pixel_buffer - 1, output_lo);

  const uint8x8_t result_hi = Upsample(vget_high_u8(src0), vget_high_u8(src1),
                                       vget_high_u8(src2), vget_high_u8(src3));

  if (size == 12) {
    vst1_u8(pixel_buffer + 15, InterleaveLow8(result_hi, vget_high_u8(src2)));
  } else /* size == 16 */ {
    const uint8x8x2_t output_hi = {result_hi, vget_high_u8(src2)};
    vst2_u8(pixel_buffer + 15, output_hi);
  }
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
  dsp->intra_edge_filter = IntraEdgeFilter_NEON;
  dsp->intra_edge_upsampler = IntraEdgeUpsampler_NEON;
}

}  // namespace
}  // namespace low_bitdepth

//------------------------------------------------------------------------------
#if LIBGAV1_MAX_BITDEPTH >= 10
namespace high_bitdepth {
namespace {

const uint16_t kRemainderMask[8][8] = {
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {0xffff, 0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {0xffff, 0xffff, 0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000},
    {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x0000, 0x0000, 0x0000},
    {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x0000, 0x0000},
    {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x0000},
};

void IntraEdgeFilter_NEON(void* buffer, const int size, const int strength) {
  assert(strength == 1 || strength == 2 || strength == 3);
  const int kernel_index = strength - 1;
  auto* const dst_buffer = static_cast<uint16_t*>(buffer);

  // The first element is not written out (but it is input) so the number of
  // elements written is |size| - 1.
  if (size == 1) return;

  // |strength| 1 and 2 use a 3 tap filter.
  if (strength < 3) {
    // The last value requires extending the buffer (duplicating
    // |dst_buffer[size - 1]). Calculate it here to avoid extra processing in
    // neon.
    const uint16_t last_val = RightShiftWithRounding(
        kKernelsNEON[kernel_index][0] * dst_buffer[size - 2] +
            kKernelsNEON[kernel_index][1] * dst_buffer[size - 1] +
            kKernelsNEON[kernel_index][0] * dst_buffer[size - 1],
        4);

    const uint16_t krn0 = kKernelsNEON[kernel_index][0];
    const uint16_t krn1 = kKernelsNEON[kernel_index][1];

    // The first value we need gets overwritten by the output from the
    // previous iteration.
    uint16x8_t src_0 = vld1q_u16(dst_buffer);
    int i = 1;

    // Process blocks until there are less than 16 values remaining.
    for (; i < size - 7; i += 8) {
      // Loading these at the end of the block with |src_0| will read past the
      // end of |top_row_data[160]|, the source of |buffer|.
      const uint16x8_t src_1 = vld1q_u16(dst_buffer + i);
      const uint16x8_t src_2 = vld1q_u16(dst_buffer + i + 1);
      const uint16x8_t sum_02 = vmulq_n_u16(vaddq_u16(src_0, src_2), krn0);
      const uint16x8_t sum = vmlaq_n_u16(sum_02, src_1, krn1);
      const uint16x8_t result = vrshrq_n_u16(sum, 4);
      // Load the next row before overwriting. This loads an extra 7 values
      // past |size| on the trailing iteration.
      src_0 = vld1q_u16(dst_buffer + i + 7);
      vst1q_u16(dst_buffer + i, result);
    }

    // The last output value |last_val| was already calculated so if
    // |remainder| == 1 then we don't have to do anything.
    const int remainder = (size - 1) & 0x7;
    if (remainder > 1) {
      const uint16x8_t src_1 = vld1q_u16(dst_buffer + i);
      const uint16x8_t src_2 = vld1q_u16(dst_buffer + i + 1);
      const uint16x8_t sum_02 = vmulq_n_u16(vaddq_u16(src_0, src_2), krn0);
      const uint16x8_t sum = vmlaq_n_u16(sum_02, src_1, krn1);
      const uint16x8_t result = vrshrq_n_u16(sum, 4);
      const uint16x8_t mask = vld1q_u16(kRemainderMask[remainder]);
      const uint16x8_t dst_remainder = vbslq_u16(mask, result, src_1);
      vst1q_u16(dst_buffer + i, dst_remainder);
    }

    dst_buffer[size - 1] = last_val;
    return;
  }

  assert(strength == 3);
  // 5 tap filter. The first element requires duplicating |buffer[0]| and the
  // last two elements require duplicating |buffer[size - 1]|.
  uint16_t special_vals[3];
  special_vals[0] = RightShiftWithRounding(
      (dst_buffer[0] << 1) + (dst_buffer[0] << 2) + (dst_buffer[1] << 2) +
          (dst_buffer[2] << 2) + (dst_buffer[3] << 1),
      4);
  // Clamp index for very small |size| values.
  const int first_index_min = std::max(size - 4, 0);
  const int second_index_min = std::max(size - 3, 0);
  const int third_index_min = std::max(size - 2, 0);
  special_vals[1] = RightShiftWithRounding(
      (dst_buffer[first_index_min] << 1) + (dst_buffer[second_index_min] << 2) +
          (dst_buffer[third_index_min] << 2) + (dst_buffer[size - 1] << 2) +
          (dst_buffer[size - 1] << 1),
      4);
  special_vals[2] = RightShiftWithRounding(
      (dst_buffer[second_index_min] << 1) + (dst_buffer[third_index_min] << 2) +
          // x << 2 + x << 2 == x << 3
          (dst_buffer[size - 1] << 3) + (dst_buffer[size - 1] << 1),
      4);

  // The first two values we need get overwritten by the output from the
  // previous iteration.
  uint16x8_t src_0 = vld1q_u16(dst_buffer - 1);
  uint16x8_t src_1 = vld1q_u16(dst_buffer);
  int i = 1;

  for (; i < size - 7; i += 8) {
    // Loading these at the end of the block with |src_[01]| will read past
    // the end of |top_row_data[160]|, the source of |buffer|.
    const uint16x8_t src_2 = vld1q_u16(dst_buffer + i);
    const uint16x8_t src_3 = vld1q_u16(dst_buffer + i + 1);
    const uint16x8_t src_4 = vld1q_u16(dst_buffer + i + 2);
    const uint16x8_t sum_04 = vshlq_n_u16(vaddq_u16(src_0, src_4), 1);
    const uint16x8_t sum_123 = vaddq_u16(vaddq_u16(src_1, src_2), src_3);
    const uint16x8_t sum = vaddq_u16(sum_04, vshlq_n_u16(sum_123, 2));
    const uint16x8_t result = vrshrq_n_u16(sum, 4);

    // Load the next before overwriting.
    src_0 = vld1q_u16(dst_buffer + i + 6);
    src_1 = vld1q_u16(dst_buffer + i + 7);

    vst1q_u16(dst_buffer + i, result);
  }

  const int remainder = (size - 1) & 0x7;
  // Like the 3 tap but if there are two remaining values we have already
  // calculated them.
  if (remainder > 2) {
    const uint16x8_t src_2 = vld1q_u16(dst_buffer + i);
    const uint16x8_t src_3 = vld1q_u16(dst_buffer + i + 1);
    const uint16x8_t src_4 = vld1q_u16(dst_buffer + i + 2);
    const uint16x8_t sum_04 = vshlq_n_u16(vaddq_u16(src_0, src_4), 1);
    const uint16x8_t sum_123 = vaddq_u16(vaddq_u16(src_1, src_2), src_3);
    const uint16x8_t sum = vaddq_u16(sum_04, vshlq_n_u16(sum_123, 2));
    const uint16x8_t result = vrshrq_n_u16(sum, 4);
    const uint16x8_t mask = vld1q_u16(kRemainderMask[remainder]);
    const uint16x8_t dst_remainder = vbslq_u16(mask, result, src_2);
    vst1q_u16(dst_buffer + i, dst_remainder);
  }

  dst_buffer[1] = special_vals[0];
  // Avoid overwriting |dst_buffer[0]|.
  if (size > 2) dst_buffer[size - 2] = special_vals[1];
  dst_buffer[size - 1] = special_vals[2];
}

void IntraEdgeUpsampler_NEON(void* buffer, const int size) {
  assert(size % 4 == 0 && size <= 16);
  auto* const pixel_buffer = static_cast<uint16_t*>(buffer);

  // Extend first/last samples
  pixel_buffer[-2] = pixel_buffer[-1];
  pixel_buffer[size] = pixel_buffer[size - 1];

  const int16x8_t src_lo = vreinterpretq_s16_u16(vld1q_u16(pixel_buffer - 2));
  const int16x8_t src_hi =
      vreinterpretq_s16_u16(vld1q_u16(pixel_buffer - 2 + 8));
  const int16x8_t src9_hi = vaddq_s16(src_hi, vshlq_n_s16(src_hi, 3));
  const int16x8_t src9_lo = vaddq_s16(src_lo, vshlq_n_s16(src_lo, 3));

  int16x8_t sum_lo = vsubq_s16(vextq_s16(src9_lo, src9_hi, 1), src_lo);
  sum_lo = vaddq_s16(sum_lo, vextq_s16(src9_lo, src9_hi, 2));
  sum_lo = vsubq_s16(sum_lo, vextq_s16(src_lo, src_hi, 3));
  sum_lo = vrshrq_n_s16(sum_lo, 4);

  uint16x8x2_t result_lo;
  result_lo.val[0] =
      vminq_u16(vreinterpretq_u16_s16(vmaxq_s16(sum_lo, vdupq_n_s16(0))),
                vdupq_n_u16((1 << kBitdepth10) - 1));
  result_lo.val[1] = vreinterpretq_u16_s16(vextq_s16(src_lo, src_hi, 2));

  if (size > 8) {
    const int16x8_t src_hi_extra =
        vreinterpretq_s16_u16(vld1q_u16(pixel_buffer + 16 - 2));
    const int16x8_t src9_hi_extra =
        vaddq_s16(src_hi_extra, vshlq_n_s16(src_hi_extra, 3));

    int16x8_t sum_hi = vsubq_s16(vextq_s16(src9_hi, src9_hi_extra, 1), src_hi);
    sum_hi = vaddq_s16(sum_hi, vextq_s16(src9_hi, src9_hi_extra, 2));
    sum_hi = vsubq_s16(sum_hi, vextq_s16(src_hi, src_hi_extra, 3));
    sum_hi = vrshrq_n_s16(sum_hi, 4);

    uint16x8x2_t result_hi;
    result_hi.val[0] =
        vminq_u16(vreinterpretq_u16_s16(vmaxq_s16(sum_hi, vdupq_n_s16(0))),
                  vdupq_n_u16((1 << kBitdepth10) - 1));
    result_hi.val[1] =
        vreinterpretq_u16_s16(vextq_s16(src_hi, src_hi_extra, 2));
    vst2q_u16(pixel_buffer - 1, result_lo);
    vst2q_u16(pixel_buffer + 15, result_hi);
  } else {
    vst2q_u16(pixel_buffer - 1, result_lo);
  }
}

void Init10bpp() {
  Dsp* dsp = dsp_internal::GetWritableDspTable(kBitdepth10);
  assert(dsp != nullptr);
  dsp->intra_edge_filter = IntraEdgeFilter_NEON;
  dsp->intra_edge_upsampler = IntraEdgeUpsampler_NEON;
}

}  // namespace
}  // namespace high_bitdepth
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

void IntraEdgeInit_NEON() {
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

void IntraEdgeInit_NEON() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_ENABLE_NEON
