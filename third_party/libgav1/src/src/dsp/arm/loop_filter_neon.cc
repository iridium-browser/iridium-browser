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

#include "src/dsp/loop_filter.h"
#include "src/utils/cpu.h"

#if LIBGAV1_ENABLE_NEON

#include <arm_neon.h>

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "src/dsp/arm/common_neon.h"
#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"

namespace libgav1 {
namespace dsp {
namespace {

// (abs(p1 - p0) > thresh) || (abs(q1 - q0) > thresh)
inline uint8x8_t Hev(const uint8x8_t abd_p0p1_q0q1, const uint8_t thresh) {
  const uint8x8_t a = vcgt_u8(abd_p0p1_q0q1, vdup_n_u8(thresh));
  return vorr_u8(a, RightShiftVector<32>(a));
}

// abs(p0 - q0) * 2 + abs(p1 - q1) / 2 <= outer_thresh
inline uint8x8_t OuterThreshold(const uint8x8_t p0q0, const uint8x8_t p1q1,
                                const uint8_t outer_thresh) {
  const uint8x8x2_t a = Interleave32(p0q0, p1q1);
  const uint8x8_t b = vabd_u8(a.val[0], a.val[1]);
  const uint8x8_t p0q0_double = vqadd_u8(b, b);
  const uint8x8_t p1q1_half = RightShiftVector<32>(vshr_n_u8(b, 1));
  const uint8x8_t c = vqadd_u8(p0q0_double, p1q1_half);
  return vcle_u8(c, vdup_n_u8(outer_thresh));
}

// abs(p1 - p0) <= inner_thresh && abs(q1 - q0) <= inner_thresh &&
//   OuterThreshold()
inline uint8x8_t NeedsFilter4(const uint8x8_t abd_p0p1_q0q1,
                              const uint8x8_t p0q0, const uint8x8_t p1q1,
                              const uint8_t inner_thresh,
                              const uint8_t outer_thresh) {
  const uint8x8_t a = vcle_u8(abd_p0p1_q0q1, vdup_n_u8(inner_thresh));
  const uint8x8_t inner_mask = vand_u8(a, RightShiftVector<32>(a));
  const uint8x8_t outer_mask = OuterThreshold(p0q0, p1q1, outer_thresh);
  return vand_u8(inner_mask, outer_mask);
}

inline void Filter4Masks(const uint8x8_t p0q0, const uint8x8_t p1q1,
                         const uint8_t hev_thresh, const uint8_t outer_thresh,
                         const uint8_t inner_thresh, uint8x8_t* const hev_mask,
                         uint8x8_t* const needs_filter4_mask) {
  // First half is |p0 - p1|, second half is |q0 - q1|.
  const uint8x8_t p0p1_q0q1 = vabd_u8(p0q0, p1q1);
  // This includes cases where NeedsFilter4() is not true and so Filter2() will
  // not be applied.
  const uint8x8_t hev_tmp_mask = Hev(p0p1_q0q1, hev_thresh);

  *needs_filter4_mask =
      NeedsFilter4(p0p1_q0q1, p0q0, p1q1, inner_thresh, outer_thresh);

  // Filter2() will only be applied if both NeedsFilter4() and Hev() are true.
  *hev_mask = vand_u8(hev_tmp_mask, *needs_filter4_mask);
}

// Calculate Filter4() or Filter2() based on |hev_mask|.
inline void Filter4(const uint8x8_t q0p1, const uint8x8_t p0q1,
                    const uint8x8_t hev_mask, uint8x8_t* const p1q1_result,
                    uint8x8_t* const p0q0_result) {
  const int16x4_t zero = vdup_n_s16(0);

  // a = 3 * (q0 - p0) + Clip3(p1 - q1, min_signed_val, max_signed_val);
  const int16x8_t q0mp0_p1mq1 = vreinterpretq_s16_u16(vsubl_u8(q0p1, p0q1));
  const int16x4_t q0mp0_3 = vmul_n_s16(vget_low_s16(q0mp0_p1mq1), 3);

  // If this is for Filter2() then include |p1mq1|. Otherwise zero it.
  const int16x4_t p1mq1 = vget_high_s16(q0mp0_p1mq1);
  const int8x8_t p1mq1_saturated = vqmovn_s16(vcombine_s16(p1mq1, zero));
  const int8x8_t hev_option =
      vand_s8(vreinterpret_s8_u8(hev_mask), p1mq1_saturated);

  const int16x4_t a =
      vget_low_s16(vaddw_s8(vcombine_s16(q0mp0_3, zero), hev_option));

  // We can not shift with rounding because the clamp comes *before* the
  // shifting. a1 = Clip3(a + 4, min_signed_val, max_signed_val) >> 3; a2 =
  // Clip3(a + 3, min_signed_val, max_signed_val) >> 3;
  const int16x4_t plus_four = vadd_s16(a, vdup_n_s16(4));
  const int16x4_t plus_three = vadd_s16(a, vdup_n_s16(3));
  const int8x8_t a2_a1 =
      vshr_n_s8(vqmovn_s16(vcombine_s16(plus_three, plus_four)), 3);

  // a3 is in the high 4 values.
  // a3 = (a1 + 1) >> 1;
  const int8x8_t a3 = vrshr_n_s8(a2_a1, 1);

  const int16x8_t p0q1_l = vreinterpretq_s16_u16(vmovl_u8(p0q1));
  const int16x8_t q0p1_l = vreinterpretq_s16_u16(vmovl_u8(q0p1));

  const int16x8_t p1q1_l =
      vcombine_s16(vget_high_s16(q0p1_l), vget_high_s16(p0q1_l));

  const int8x8_t a3_ma3 = InterleaveHigh32(a3, vneg_s8(a3));
  const int16x8_t p1q1_a3 = vaddw_s8(p1q1_l, a3_ma3);

  const int16x8_t p0q0_l =
      vcombine_s16(vget_low_s16(p0q1_l), vget_low_s16(q0p1_l));
  // Need to shift the second term or we end up with a2_ma2.
  const int8x8_t a2_ma1 =
      InterleaveLow32(a2_a1, RightShiftVector<32>(vneg_s8(a2_a1)));
  const int16x8_t p0q0_a = vaddw_s8(p0q0_l, a2_ma1);

  *p1q1_result = vqmovun_s16(p1q1_a3);
  *p0q0_result = vqmovun_s16(p0q0_a);
}

void Horizontal4_NEON(void* const dest, const ptrdiff_t stride,
                      const int outer_thresh, const int inner_thresh,
                      const int hev_thresh) {
  auto* dst = static_cast<uint8_t*>(dest);

  const uint8x8_t p1_v = Load4(dst - 2 * stride);
  const uint8x8_t p0_v = Load4(dst - stride);
  const uint8x8_t p0q0 = Load4<1>(dst, p0_v);
  const uint8x8_t p1q1 = Load4<1>(dst + stride, p1_v);

  uint8x8_t hev_mask;
  uint8x8_t needs_filter4_mask;
  Filter4Masks(p0q0, p1q1, hev_thresh, outer_thresh, inner_thresh, &hev_mask,
               &needs_filter4_mask);

  // Copy the masks to the high bits for packed comparisons later.
  hev_mask = InterleaveLow32(hev_mask, hev_mask);
  needs_filter4_mask = InterleaveLow32(needs_filter4_mask, needs_filter4_mask);

#if defined(__aarch64__)
  if (vaddv_u8(needs_filter4_mask) == 0) {
    // None of the values will be filtered.
    return;
  }
#endif  // defined(__aarch64__)

  uint8x8_t f_p1q1;
  uint8x8_t f_p0q0;
  const uint8x8x2_t q0p1xp0q1 = Interleave32(Transpose32(p0q0), p1q1);
  Filter4(q0p1xp0q1.val[0], q0p1xp0q1.val[1], hev_mask, &f_p1q1, &f_p0q0);

  // Already integrated the Hev mask when calculating the filtered values.
  const uint8x8_t p0q0_output = vbsl_u8(needs_filter4_mask, f_p0q0, p0q0);

  // p1/q1 are unmodified if only Hev() is true. This works because it was and'd
  // with |needs_filter4_mask| previously.
  const uint8x8_t p1q1_mask = veor_u8(hev_mask, needs_filter4_mask);
  const uint8x8_t p1q1_output = vbsl_u8(p1q1_mask, f_p1q1, p1q1);

  StoreLo4(dst - 2 * stride, p1q1_output);
  StoreLo4(dst - stride, p0q0_output);
  StoreHi4(dst, p0q0_output);
  StoreHi4(dst + stride, p1q1_output);
}

void Vertical4_NEON(void* const dest, const ptrdiff_t stride,
                    const int outer_thresh, const int inner_thresh,
                    const int hev_thresh) {
  auto* dst = static_cast<uint8_t*>(dest);

  // Move |dst| to the left side of the filter window.
  dst -= 2;

  // |p1q0| and |p0q1| are named for the values they will contain after the
  // transpose.
  const uint8x8_t row0 = Load4(dst);
  uint8x8_t p1q0 = Load4<1>(dst + stride, row0);
  const uint8x8_t row2 = Load4(dst + 2 * stride);
  uint8x8_t p0q1 = Load4<1>(dst + 3 * stride, row2);

  Transpose4x4(&p1q0, &p0q1);
  // Rearrange.
  const uint8x8x2_t p1q1xq0p0 = Interleave32(p1q0, Transpose32(p0q1));
  const uint8x8x2_t p1q1xp0q0 = {p1q1xq0p0.val[0],
                                 Transpose32(p1q1xq0p0.val[1])};

  uint8x8_t hev_mask;
  uint8x8_t needs_filter4_mask;
  Filter4Masks(p1q1xp0q0.val[1], p1q1xp0q0.val[0], hev_thresh, outer_thresh,
               inner_thresh, &hev_mask, &needs_filter4_mask);

  // Copy the masks to the high bits for packed comparisons later.
  hev_mask = InterleaveLow32(hev_mask, hev_mask);
  needs_filter4_mask = InterleaveLow32(needs_filter4_mask, needs_filter4_mask);

#if defined(__aarch64__)
  if (vaddv_u8(needs_filter4_mask) == 0) {
    // None of the values will be filtered.
    return;
  }
#endif  // defined(__aarch64__)

  uint8x8_t f_p1q1;
  uint8x8_t f_p0q0;
  Filter4(Transpose32(p1q0), p0q1, hev_mask, &f_p1q1, &f_p0q0);

  // Already integrated the Hev mask when calculating the filtered values.
  const uint8x8_t p0q0_output =
      vbsl_u8(needs_filter4_mask, f_p0q0, p1q1xp0q0.val[1]);

  // p1/q1 are unmodified if only Hev() is true. This works because it was and'd
  // with |needs_filter4_mask| previously.
  const uint8x8_t p1q1_mask = veor_u8(hev_mask, needs_filter4_mask);
  const uint8x8_t p1q1_output = vbsl_u8(p1q1_mask, f_p1q1, p1q1xp0q0.val[0]);

  // Put things back in order to reverse the transpose.
  const uint8x8x2_t p1p0xq1q0 = Interleave32(p1q1_output, p0q0_output);
  uint8x8_t output_0 = p1p0xq1q0.val[0],
            output_1 = Transpose32(p1p0xq1q0.val[1]);

  Transpose4x4(&output_0, &output_1);

  StoreLo4(dst, output_0);
  StoreLo4(dst + stride, output_1);
  StoreHi4(dst + 2 * stride, output_0);
  StoreHi4(dst + 3 * stride, output_1);
}

// abs(p1 - p0) <= flat_thresh && abs(q1 - q0) <= flat_thresh &&
//   abs(p2 - p0) <= flat_thresh && abs(q2 - q0) <= flat_thresh
// |flat_thresh| == 1 for 8 bit decode.
inline uint8x8_t IsFlat3(const uint8x8_t abd_p0p1_q0q1,
                         const uint8x8_t abd_p0p2_q0q2) {
  const uint8x8_t a = vmax_u8(abd_p0p1_q0q1, abd_p0p2_q0q2);
  const uint8x8_t b = vcle_u8(a, vdup_n_u8(1));
  return vand_u8(b, RightShiftVector<32>(b));
}

// abs(p2 - p1) <= inner_thresh && abs(p1 - p0) <= inner_thresh &&
//   abs(q1 - q0) <= inner_thresh && abs(q2 - q1) <= inner_thresh &&
//   OuterThreshold()
inline uint8x8_t NeedsFilter6(const uint8x8_t abd_p0p1_q0q1,
                              const uint8x8_t abd_p1p2_q1q2,
                              const uint8x8_t p0q0, const uint8x8_t p1q1,
                              const uint8_t inner_thresh,
                              const uint8_t outer_thresh) {
  const uint8x8_t a = vmax_u8(abd_p0p1_q0q1, abd_p1p2_q1q2);
  const uint8x8_t b = vcle_u8(a, vdup_n_u8(inner_thresh));
  const uint8x8_t inner_mask = vand_u8(b, RightShiftVector<32>(b));
  const uint8x8_t outer_mask = OuterThreshold(p0q0, p1q1, outer_thresh);
  return vand_u8(inner_mask, outer_mask);
}

inline void Filter6Masks(const uint8x8_t p2q2, const uint8x8_t p1q1,
                         const uint8x8_t p0q0, const uint8_t hev_thresh,
                         const uint8_t outer_thresh, const uint8_t inner_thresh,
                         uint8x8_t* const needs_filter6_mask,
                         uint8x8_t* const is_flat3_mask,
                         uint8x8_t* const hev_mask) {
  const uint8x8_t p0p1_q0q1 = vabd_u8(p0q0, p1q1);
  *hev_mask = Hev(p0p1_q0q1, hev_thresh);
  *is_flat3_mask = IsFlat3(p0p1_q0q1, vabd_u8(p0q0, p2q2));
  *needs_filter6_mask = NeedsFilter6(p0p1_q0q1, vabd_u8(p1q1, p2q2), p0q0, p1q1,
                                     inner_thresh, outer_thresh);
}

inline void Filter6(const uint8x8_t p2q2, const uint8x8_t p1q1,
                    const uint8x8_t p0q0, uint8x8_t* const p1q1_output,
                    uint8x8_t* const p0q0_output) {
  // Sum p1 and q1 output from opposite directions
  // p1 = (3 * p2) + (2 * p1) + (2 * p0) + q0
  //      ^^^^^^^^
  // q1 = p0 + (2 * q0) + (2 * q1) + (3 * q2)
  //                                 ^^^^^^^^
  const uint16x8_t p2q2_double = vaddl_u8(p2q2, p2q2);
  uint16x8_t sum = vaddw_u8(p2q2_double, p2q2);

  // p1 = (3 * p2) + (2 * p1) + (2 * p0) + q0
  //                 ^^^^^^^^
  // q1 = p0 + (2 * q0) + (2 * q1) + (3 * q2)
  //                      ^^^^^^^^
  sum = vaddq_u16(vaddl_u8(p1q1, p1q1), sum);

  // p1 = (3 * p2) + (2 * p1) + (2 * p0) + q0
  //                            ^^^^^^^^
  // q1 = p0 + (2 * q0) + (2 * q1) + (3 * q2)
  //           ^^^^^^^^
  sum = vaddq_u16(vaddl_u8(p0q0, p0q0), sum);

  // p1 = (3 * p2) + (2 * p1) + (2 * p0) + q0
  //                                       ^^
  // q1 = p0 + (2 * q0) + (2 * q1) + (3 * q2)
  //      ^^
  const uint8x8_t q0p0 = Transpose32(p0q0);
  sum = vaddw_u8(sum, q0p0);

  *p1q1_output = vrshrn_n_u16(sum, 3);

  // Convert to p0 and q0 output:
  // p0 = p1 - (2 * p2) + q0 + q1
  // q0 = q1 - (2 * q2) + p0 + p1
  sum = vsubq_u16(sum, p2q2_double);
  const uint8x8_t q1p1 = Transpose32(p1q1);
  sum = vaddq_u16(vaddl_u8(q0p0, q1p1), sum);

  *p0q0_output = vrshrn_n_u16(sum, 3);
}

void Horizontal6_NEON(void* const dest, const ptrdiff_t stride,
                      const int outer_thresh, const int inner_thresh,
                      const int hev_thresh) {
  auto* dst = static_cast<uint8_t*>(dest);

  const uint8x8_t p2_v = Load4(dst - 3 * stride);
  const uint8x8_t p1_v = Load4(dst - 2 * stride);
  const uint8x8_t p0_v = Load4(dst - stride);
  const uint8x8_t p0q0 = Load4<1>(dst, p0_v);
  const uint8x8_t p1q1 = Load4<1>(dst + stride, p1_v);
  const uint8x8_t p2q2 = Load4<1>(dst + 2 * stride, p2_v);

  uint8x8_t needs_filter6_mask, is_flat3_mask, hev_mask;
  Filter6Masks(p2q2, p1q1, p0q0, hev_thresh, outer_thresh, inner_thresh,
               &needs_filter6_mask, &is_flat3_mask, &hev_mask);

  needs_filter6_mask = InterleaveLow32(needs_filter6_mask, needs_filter6_mask);
  is_flat3_mask = InterleaveLow32(is_flat3_mask, is_flat3_mask);
  hev_mask = InterleaveLow32(hev_mask, hev_mask);

#if defined(__aarch64__)
  if (vaddv_u8(needs_filter6_mask) == 0) {
    // None of the values will be filtered.
    return;
  }
#endif  // defined(__aarch64__)

  uint8x8_t f_p1q1;
  uint8x8_t f_p0q0;
  const uint8x8x2_t q0p1xp0q1 = Interleave32(Transpose32(p0q0), p1q1);
  Filter4(q0p1xp0q1.val[0], q0p1xp0q1.val[1], hev_mask, &f_p1q1, &f_p0q0);
  // Reset the outer values if only a Hev() mask was required.
  f_p1q1 = vbsl_u8(hev_mask, p1q1, f_p1q1);

  uint8x8_t f6_p1q1, f6_p0q0;
#if defined(__aarch64__)
  if (vaddv_u8(vand_u8(is_flat3_mask, needs_filter6_mask)) == 0) {
    // Filter6() does not apply.
    const uint8x8_t zero = vdup_n_u8(0);
    f6_p1q1 = zero;
    f6_p0q0 = zero;
  } else {
#endif  // defined(__aarch64__)
    Filter6(p2q2, p1q1, p0q0, &f6_p1q1, &f6_p0q0);
#if defined(__aarch64__)
  }
#endif  // defined(__aarch64__)

  uint8x8_t p1q1_output = vbsl_u8(is_flat3_mask, f6_p1q1, f_p1q1);
  p1q1_output = vbsl_u8(needs_filter6_mask, p1q1_output, p1q1);
  StoreLo4(dst - 2 * stride, p1q1_output);
  StoreHi4(dst + stride, p1q1_output);

  uint8x8_t p0q0_output = vbsl_u8(is_flat3_mask, f6_p0q0, f_p0q0);
  p0q0_output = vbsl_u8(needs_filter6_mask, p0q0_output, p0q0);
  StoreLo4(dst - stride, p0q0_output);
  StoreHi4(dst, p0q0_output);
}

void Vertical6_NEON(void* const dest, const ptrdiff_t stride,
                    const int outer_thresh, const int inner_thresh,
                    const int hev_thresh) {
  auto* dst = static_cast<uint8_t*>(dest);

  // Move |dst| to the left side of the filter window.
  dst -= 3;

  // |p2q1|, |p1q2|, |p0xx| and |q0xx| are named for the values they will
  // contain after the transpose.
  // These over-read by 2 bytes. We only need 6.
  uint8x8_t p2q1 = vld1_u8(dst);
  uint8x8_t p1q2 = vld1_u8(dst + stride);
  uint8x8_t p0xx = vld1_u8(dst + 2 * stride);
  uint8x8_t q0xx = vld1_u8(dst + 3 * stride);

  Transpose8x4(&p2q1, &p1q2, &p0xx, &q0xx);

  const uint8x8x2_t p2q2xq1p1 = Interleave32(p2q1, Transpose32(p1q2));
  const uint8x8_t p2q2 = p2q2xq1p1.val[0];
  const uint8x8_t p1q1 = Transpose32(p2q2xq1p1.val[1]);
  const uint8x8_t p0q0 = InterleaveLow32(p0xx, q0xx);

  uint8x8_t needs_filter6_mask, is_flat3_mask, hev_mask;
  Filter6Masks(p2q2, p1q1, p0q0, hev_thresh, outer_thresh, inner_thresh,
               &needs_filter6_mask, &is_flat3_mask, &hev_mask);

  needs_filter6_mask = InterleaveLow32(needs_filter6_mask, needs_filter6_mask);
  is_flat3_mask = InterleaveLow32(is_flat3_mask, is_flat3_mask);
  hev_mask = InterleaveLow32(hev_mask, hev_mask);

#if defined(__aarch64__)
  if (vaddv_u8(needs_filter6_mask) == 0) {
    // None of the values will be filtered.
    return;
  }
#endif  // defined(__aarch64__)

  uint8x8_t f_p1q1;
  uint8x8_t f_p0q0;
  const uint8x8x2_t q0p1xp0q1 = Interleave32(Transpose32(p0q0), p1q1);
  Filter4(q0p1xp0q1.val[0], q0p1xp0q1.val[1], hev_mask, &f_p1q1, &f_p0q0);
  // Reset the outer values if only a Hev() mask was required.
  f_p1q1 = vbsl_u8(hev_mask, p1q1, f_p1q1);

  uint8x8_t f6_p1q1, f6_p0q0;
#if defined(__aarch64__)
  if (vaddv_u8(vand_u8(is_flat3_mask, needs_filter6_mask)) == 0) {
    // Filter6() does not apply.
    const uint8x8_t zero = vdup_n_u8(0);
    f6_p1q1 = zero;
    f6_p0q0 = zero;
  } else {
#endif  // defined(__aarch64__)
    Filter6(p2q2, p1q1, p0q0, &f6_p1q1, &f6_p0q0);
#if defined(__aarch64__)
  }
#endif  // defined(__aarch64__)

  uint8x8_t p1q1_output = vbsl_u8(is_flat3_mask, f6_p1q1, f_p1q1);
  p1q1_output = vbsl_u8(needs_filter6_mask, p1q1_output, p1q1);

  uint8x8_t p0q0_output = vbsl_u8(is_flat3_mask, f6_p0q0, f_p0q0);
  p0q0_output = vbsl_u8(needs_filter6_mask, p0q0_output, p0q0);

  // The six tap filter is only six taps on input. Output is limited to p1-q1.
  dst += 1;
  // Put things back in order to reverse the transpose.
  const uint8x8x2_t p1p0xq1q0 = Interleave32(p1q1_output, p0q0_output);
  uint8x8_t output_0 = p1p0xq1q0.val[0];
  uint8x8_t output_1 = Transpose32(p1p0xq1q0.val[1]);

  Transpose4x4(&output_0, &output_1);

  StoreLo4(dst, output_0);
  StoreLo4(dst + stride, output_1);
  StoreHi4(dst + 2 * stride, output_0);
  StoreHi4(dst + 3 * stride, output_1);
}

// IsFlat4 uses N=1, IsFlatOuter4 uses N=4.
// abs(p[N] - p0) <= flat_thresh && abs(q[N] - q0) <= flat_thresh &&
//   abs(p[N+1] - p0) <= flat_thresh && abs(q[N+1] - q0) <= flat_thresh &&
//   abs(p[N+2] - p0) <= flat_thresh && abs(q[N+1] - q0) <= flat_thresh
// |flat_thresh| == 1 for 8 bit decode.
inline uint8x8_t IsFlat4(const uint8x8_t abd_p0n0_q0n0,
                         const uint8x8_t abd_p0n1_q0n1,
                         const uint8x8_t abd_p0n2_q0n2) {
  const uint8x8_t a = vmax_u8(abd_p0n0_q0n0, abd_p0n1_q0n1);
  const uint8x8_t b = vmax_u8(a, abd_p0n2_q0n2);
  const uint8x8_t c = vcle_u8(b, vdup_n_u8(1));
  return vand_u8(c, RightShiftVector<32>(c));
}

// abs(p3 - p2) <= inner_thresh && abs(p2 - p1) <= inner_thresh &&
//   abs(p1 - p0) <= inner_thresh && abs(q1 - q0) <= inner_thresh &&
//   abs(q2 - q1) <= inner_thresh && abs(q3 - q2) <= inner_thresh
//   OuterThreshold()
inline uint8x8_t NeedsFilter8(const uint8x8_t abd_p0p1_q0q1,
                              const uint8x8_t abd_p1p2_q1q2,
                              const uint8x8_t abd_p2p3_q2q3,
                              const uint8x8_t p0q0, const uint8x8_t p1q1,
                              const uint8_t inner_thresh,
                              const uint8_t outer_thresh) {
  const uint8x8_t a = vmax_u8(abd_p0p1_q0q1, abd_p1p2_q1q2);
  const uint8x8_t b = vmax_u8(a, abd_p2p3_q2q3);
  const uint8x8_t c = vcle_u8(b, vdup_n_u8(inner_thresh));
  const uint8x8_t inner_mask = vand_u8(c, RightShiftVector<32>(c));
  const uint8x8_t outer_mask = OuterThreshold(p0q0, p1q1, outer_thresh);
  return vand_u8(inner_mask, outer_mask);
}

inline void Filter8Masks(const uint8x8_t p3q3, const uint8x8_t p2q2,
                         const uint8x8_t p1q1, const uint8x8_t p0q0,
                         const uint8_t hev_thresh, const uint8_t outer_thresh,
                         const uint8_t inner_thresh,
                         uint8x8_t* const needs_filter8_mask,
                         uint8x8_t* const is_flat4_mask,
                         uint8x8_t* const hev_mask) {
  const uint8x8_t p0p1_q0q1 = vabd_u8(p0q0, p1q1);
  *hev_mask = Hev(p0p1_q0q1, hev_thresh);
  *is_flat4_mask = IsFlat4(p0p1_q0q1, vabd_u8(p0q0, p2q2), vabd_u8(p0q0, p3q3));
  *needs_filter8_mask =
      NeedsFilter8(p0p1_q0q1, vabd_u8(p1q1, p2q2), vabd_u8(p2q2, p3q3), p0q0,
                   p1q1, inner_thresh, outer_thresh);
}

inline void Filter8(const uint8x8_t p3q3, const uint8x8_t p2q2,
                    const uint8x8_t p1q1, const uint8x8_t p0q0,
                    uint8x8_t* const p2q2_output, uint8x8_t* const p1q1_output,
                    uint8x8_t* const p0q0_output) {
  // Sum p2 and q2 output from opposite directions.
  // The formula is regrouped to allow 2 doubling operations to be combined.
  // p2 = (3 * p3) + (2 * p2) + p1 + p0 + q0
  //      ^^^^^^^^
  // q2 = p0 + q0 + q1 + (2 * q2) + (3 * q3)
  //                                ^^^^^^^^
  // p2q2 = p3q3 + 2 * (p3q3 + p2q2) + p1q1 + p0q0 + q0p0
  //                    ^^^^^^^^^^^
  const uint16x8_t p23q23 = vaddl_u8(p3q3, p2q2);

  // p2q2 = p3q3 + 2 * (p3q3 + p2q2) + p1q1 + p0q0 + q0p0
  //               ^^^^^
  uint16x8_t sum = vshlq_n_u16(p23q23, 1);

  // Add two other terms to make dual issue with shift more likely.
  // p2q2 = p3q3 + 2 * (p3q3 + p2q2) + p1q1 + p0q0 + q0p0
  //                                   ^^^^^^^^^^^
  const uint16x8_t p01q01 = vaddl_u8(p0q0, p1q1);

  // p2q2 = p3q3 + 2 * (p3q3 + p2q2) + p1q1 + p0q0 + q0p0
  //                                 ^^^^^^^^^^^^^
  sum = vaddq_u16(sum, p01q01);

  // p2q2 = p3q3 + 2 * (p3q3 + p2q2) + p1q1 + p0q0 + q0p0
  //        ^^^^^^
  sum = vaddw_u8(sum, p3q3);

  // p2q2 = p3q3 + 2 * (p3q3 + p2q2) + p1q1 + p0q0 + q0p0
  //                                               ^^^^^^
  const uint8x8_t q0p0 = Transpose32(p0q0);
  sum = vaddw_u8(sum, q0p0);

  *p2q2_output = vrshrn_n_u16(sum, 3);

  // Convert to p1 and q1 output:
  // p1 = p2 - p3 - p2 + p1 + q1
  // q1 = q2 - q3 - q2 + q0 + p1
  sum = vsubq_u16(sum, p23q23);
  const uint8x8_t q1p1 = Transpose32(p1q1);
  sum = vaddq_u16(sum, vaddl_u8(p1q1, q1p1));

  *p1q1_output = vrshrn_n_u16(sum, 3);

  // Convert to p0 and q0 output:
  // p0 = p1 - p3 - p1 + p0 + q2
  // q0 = q1 - q3 - q1 + q0 + p2
  sum = vsubq_u16(sum, vaddl_u8(p3q3, p1q1));
  const uint8x8_t q2p2 = Transpose32(p2q2);
  sum = vaddq_u16(sum, vaddl_u8(p0q0, q2p2));

  *p0q0_output = vrshrn_n_u16(sum, 3);
}

void Horizontal8_NEON(void* const dest, const ptrdiff_t stride,
                      const int outer_thresh, const int inner_thresh,
                      const int hev_thresh) {
  auto* dst = static_cast<uint8_t*>(dest);

  const uint8x8_t p3_v = Load4(dst - 4 * stride);
  const uint8x8_t p2_v = Load4(dst - 3 * stride);
  const uint8x8_t p1_v = Load4(dst - 2 * stride);
  const uint8x8_t p0_v = Load4(dst - stride);
  const uint8x8_t p0q0 = Load4<1>(dst, p0_v);
  const uint8x8_t p1q1 = Load4<1>(dst + stride, p1_v);
  const uint8x8_t p2q2 = Load4<1>(dst + 2 * stride, p2_v);
  const uint8x8_t p3q3 = Load4<1>(dst + 3 * stride, p3_v);

  uint8x8_t needs_filter8_mask, is_flat4_mask, hev_mask;
  Filter8Masks(p3q3, p2q2, p1q1, p0q0, hev_thresh, outer_thresh, inner_thresh,
               &needs_filter8_mask, &is_flat4_mask, &hev_mask);

  needs_filter8_mask = InterleaveLow32(needs_filter8_mask, needs_filter8_mask);
  is_flat4_mask = vand_u8(is_flat4_mask, needs_filter8_mask);
  is_flat4_mask = InterleaveLow32(is_flat4_mask, is_flat4_mask);
  hev_mask = InterleaveLow32(hev_mask, hev_mask);

#if defined(__aarch64__)
  if (vaddv_u8(needs_filter8_mask) == 0) {
    // None of the values will be filtered.
    return;
  }
#endif  // defined(__aarch64__)

  uint8x8_t f_p1q1;
  uint8x8_t f_p0q0;
  const uint8x8x2_t q0p1xp0q1 = Interleave32(Transpose32(p0q0), p1q1);
  Filter4(q0p1xp0q1.val[0], q0p1xp0q1.val[1], hev_mask, &f_p1q1, &f_p0q0);
  // Reset the outer values if only a Hev() mask was required.
  f_p1q1 = vbsl_u8(hev_mask, p1q1, f_p1q1);

  uint8x8_t f8_p2q2, f8_p1q1, f8_p0q0;
#if defined(__aarch64__)
  if (vaddv_u8(is_flat4_mask) == 0) {
    // Filter8() does not apply.
    const uint8x8_t zero = vdup_n_u8(0);
    f8_p2q2 = zero;
    f8_p1q1 = zero;
    f8_p0q0 = zero;
  } else {
#endif  // defined(__aarch64__)
    Filter8(p3q3, p2q2, p1q1, p0q0, &f8_p2q2, &f8_p1q1, &f8_p0q0);

    const uint8x8_t p2p2_output = vbsl_u8(is_flat4_mask, f8_p2q2, p2q2);
    StoreLo4(dst - 3 * stride, p2p2_output);
    StoreHi4(dst + 2 * stride, p2p2_output);
#if defined(__aarch64__)
  }
#endif  // defined(__aarch64__)

  uint8x8_t p1q1_output = vbsl_u8(is_flat4_mask, f8_p1q1, f_p1q1);
  p1q1_output = vbsl_u8(needs_filter8_mask, p1q1_output, p1q1);
  StoreLo4(dst - 2 * stride, p1q1_output);
  StoreHi4(dst + stride, p1q1_output);

  uint8x8_t p0q0_output = vbsl_u8(is_flat4_mask, f8_p0q0, f_p0q0);
  p0q0_output = vbsl_u8(needs_filter8_mask, p0q0_output, p0q0);
  StoreLo4(dst - stride, p0q0_output);
  StoreHi4(dst, p0q0_output);
}

void Vertical8_NEON(void* const dest, const ptrdiff_t stride,
                    const int outer_thresh, const int inner_thresh,
                    const int hev_thresh) {
  auto* dst = static_cast<uint8_t*>(dest);
  // Move |dst| to the left side of the filter window.
  dst -= 4;

  // |p3q0|, |p2q1|, |p1q2| and |p0q3| are named for the values they will
  // contain after the transpose.
  uint8x8_t p3q0 = vld1_u8(dst);
  uint8x8_t p2q1 = vld1_u8(dst + stride);
  uint8x8_t p1q2 = vld1_u8(dst + 2 * stride);
  uint8x8_t p0q3 = vld1_u8(dst + 3 * stride);

  Transpose8x4(&p3q0, &p2q1, &p1q2, &p0q3);
  const uint8x8x2_t p3q3xq0p0 = Interleave32(p3q0, Transpose32(p0q3));
  const uint8x8_t p3q3 = p3q3xq0p0.val[0];
  const uint8x8_t p0q0 = Transpose32(p3q3xq0p0.val[1]);
  const uint8x8x2_t p2q2xq1p1 = Interleave32(p2q1, Transpose32(p1q2));
  const uint8x8_t p2q2 = p2q2xq1p1.val[0];
  const uint8x8_t p1q1 = Transpose32(p2q2xq1p1.val[1]);

  uint8x8_t needs_filter8_mask, is_flat4_mask, hev_mask;
  Filter8Masks(p3q3, p2q2, p1q1, p0q0, hev_thresh, outer_thresh, inner_thresh,
               &needs_filter8_mask, &is_flat4_mask, &hev_mask);

  needs_filter8_mask = InterleaveLow32(needs_filter8_mask, needs_filter8_mask);
  is_flat4_mask = vand_u8(is_flat4_mask, needs_filter8_mask);
  is_flat4_mask = InterleaveLow32(is_flat4_mask, is_flat4_mask);
  hev_mask = InterleaveLow32(hev_mask, hev_mask);

#if defined(__aarch64__)
  if (vaddv_u8(needs_filter8_mask) == 0) {
    // None of the values will be filtered.
    return;
  }
#endif  // defined(__aarch64__)

  uint8x8_t f_p1q1;
  uint8x8_t f_p0q0;
  const uint8x8x2_t q0p1xp0q1 = Interleave32(Transpose32(p0q0), p1q1);
  Filter4(q0p1xp0q1.val[0], q0p1xp0q1.val[1], hev_mask, &f_p1q1, &f_p0q0);
  // Reset the outer values if only a Hev() mask was required.
  f_p1q1 = vbsl_u8(hev_mask, p1q1, f_p1q1);

  uint8x8_t f8_p2q2, f8_p1q1, f8_p0q0;
#if defined(__aarch64__)
  if (vaddv_u8(is_flat4_mask) == 0) {
    // Filter8() does not apply.
    const uint8x8_t zero = vdup_n_u8(0);
    f8_p2q2 = zero;
    f8_p1q1 = zero;
    f8_p0q0 = zero;
  } else {
#endif  // defined(__aarch64__)
    Filter8(p3q3, p2q2, p1q1, p0q0, &f8_p2q2, &f8_p1q1, &f8_p0q0);

#if defined(__aarch64__)
  }
#endif  // defined(__aarch64__)

  // Always prepare and store p2/q2 because we need to transpose it anyway.
  const uint8x8_t p2q2_output = vbsl_u8(is_flat4_mask, f8_p2q2, p2q2);

  uint8x8_t p1q1_output = vbsl_u8(is_flat4_mask, f8_p1q1, f_p1q1);
  p1q1_output = vbsl_u8(needs_filter8_mask, p1q1_output, p1q1);

  uint8x8_t p0q0_output = vbsl_u8(is_flat4_mask, f8_p0q0, f_p0q0);
  p0q0_output = vbsl_u8(needs_filter8_mask, p0q0_output, p0q0);

  // Write out p3/q3 as well. There isn't a good way to write out 6 bytes.
  // Variable names reflect the values before transposition.
  const uint8x8x2_t p3q0xq3p0_output =
      Interleave32(p3q3, Transpose32(p0q0_output));
  uint8x8_t p3q0_output = p3q0xq3p0_output.val[0];
  uint8x8_t p0q3_output = Transpose32(p3q0xq3p0_output.val[1]);
  const uint8x8x2_t p2q1xq2p1_output =
      Interleave32(p2q2_output, Transpose32(p1q1_output));
  uint8x8_t p2q1_output = p2q1xq2p1_output.val[0];
  uint8x8_t p1q2_output = Transpose32(p2q1xq2p1_output.val[1]);

  Transpose8x4(&p3q0_output, &p2q1_output, &p1q2_output, &p0q3_output);

  vst1_u8(dst, p3q0_output);
  vst1_u8(dst + stride, p2q1_output);
  vst1_u8(dst + 2 * stride, p1q2_output);
  vst1_u8(dst + 3 * stride, p0q3_output);
}

inline void Filter14(const uint8x8_t p6q6, const uint8x8_t p5q5,
                     const uint8x8_t p4q4, const uint8x8_t p3q3,
                     const uint8x8_t p2q2, const uint8x8_t p1q1,
                     const uint8x8_t p0q0, uint8x8_t* const p5q5_output,
                     uint8x8_t* const p4q4_output, uint8x8_t* const p3q3_output,
                     uint8x8_t* const p2q2_output, uint8x8_t* const p1q1_output,
                     uint8x8_t* const p0q0_output) {
  // Sum p5 and q5 output from opposite directions
  // p5 = (7 * p6) + (2 * p5) + (2 * p4) + p3 + p2 + p1 + p0 + q0
  //      ^^^^^^^^
  // q5 = p0 + q0 + q1 + q2 + q3 + (2 * q4) + (2 * q5) + (7 * q6)
  //                                                     ^^^^^^^^
  uint16x8_t sum = vsubw_u8(vshll_n_u8(p6q6, 3), p6q6);

  // p5 = (7 * p6) + (2 * p5) + (2 * p4) + p3 + p2 + p1 + p0 + q0
  //                 ^^^^^^^^
  // q5 = p0 + q0 + q1 + q2 + q3 + (2 * q4) + (2 * q5) + (7 * q6)
  //                                          ^^^^^^^^
  sum = vaddq_u16(vaddl_u8(p5q5, p5q5), sum);

  // p5 = (7 * p6) + (2 * p5) + (2 * p4) + p3 + p2 + p1 + p0 + q0
  //                            ^^^^^^^^
  // q5 = p0 + q0 + q1 + q2 + q3 + (2 * q4) + (2 * q5) + (7 * q6)
  //                               ^^^^^^^^
  sum = vaddq_u16(vaddl_u8(p4q4, p4q4), sum);

  // p5 = (7 * p6) + (2 * p5) + (2 * p4) + p3 + p2 + p1 + p0 + q0
  //                                       ^^^^^^^
  // q5 = p0 + q0 + q1 + q2 + q3 + (2 * q4) + (2 * q5) + (7 * q6)
  //                     ^^^^^^^
  sum = vaddq_u16(vaddl_u8(p3q3, p2q2), sum);

  // p5 = (7 * p6) + (2 * p5) + (2 * p4) + p3 + p2 + p1 + p0 + q0
  //                                                 ^^^^^^^
  // q5 = p0 + q0 + q1 + q2 + q3 + (2 * q4) + (2 * q5) + (7 * q6)
  //           ^^^^^^^
  sum = vaddq_u16(vaddl_u8(p1q1, p0q0), sum);

  // p5 = (7 * p6) + (2 * p5) + (2 * p4) + p3 + p2 + p1 + p0 + q0
  //                                                           ^^
  // q5 = p0 + q0 + q1 + q2 + q3 + (2 * q4) + (2 * q5) + (7 * q6)
  //      ^^
  const uint8x8_t q0p0 = Transpose32(p0q0);
  sum = vaddw_u8(sum, q0p0);

  *p5q5_output = vrshrn_n_u16(sum, 4);

  // Convert to p4 and q4 output:
  // p4 = p5 - (2 * p6) + p3 + q1
  // q4 = q5 - (2 * q6) + q3 + p1
  sum = vsubq_u16(sum, vaddl_u8(p6q6, p6q6));
  const uint8x8_t q1p1 = Transpose32(p1q1);
  sum = vaddq_u16(vaddl_u8(p3q3, q1p1), sum);

  *p4q4_output = vrshrn_n_u16(sum, 4);

  // Convert to p3 and q3 output:
  // p3 = p4 - p6 - p5 + p2 + q2
  // q3 = q4 - q6 - q5 + q2 + p2
  sum = vsubq_u16(sum, vaddl_u8(p6q6, p5q5));
  const uint8x8_t q2p2 = Transpose32(p2q2);
  sum = vaddq_u16(vaddl_u8(p2q2, q2p2), sum);

  *p3q3_output = vrshrn_n_u16(sum, 4);

  // Convert to p2 and q2 output:
  // p2 = p3 - p6 - p4 + p1 + q3
  // q2 = q3 - q6 - q4 + q1 + p3
  sum = vsubq_u16(sum, vaddl_u8(p6q6, p4q4));
  const uint8x8_t q3p3 = Transpose32(p3q3);
  sum = vaddq_u16(vaddl_u8(p1q1, q3p3), sum);

  *p2q2_output = vrshrn_n_u16(sum, 4);

  // Convert to p1 and q1 output:
  // p1 = p2 - p6 - p3 + p0 + q4
  // q1 = q2 - q6 - q3 + q0 + p4
  sum = vsubq_u16(sum, vaddl_u8(p6q6, p3q3));
  const uint8x8_t q4p4 = Transpose32(p4q4);
  sum = vaddq_u16(vaddl_u8(p0q0, q4p4), sum);

  *p1q1_output = vrshrn_n_u16(sum, 4);

  // Convert to p0 and q0 output:
  // p0 = p1 - p6 - p2 + q0 + q5
  // q0 = q1 - q6 - q2 + p0 + p5
  sum = vsubq_u16(sum, vaddl_u8(p6q6, p2q2));
  const uint8x8_t q5p5 = Transpose32(p5q5);
  sum = vaddq_u16(vaddl_u8(q0p0, q5p5), sum);

  *p0q0_output = vrshrn_n_u16(sum, 4);
}

void Horizontal14_NEON(void* const dest, const ptrdiff_t stride,
                       const int outer_thresh, const int inner_thresh,
                       const int hev_thresh) {
  auto* dst = static_cast<uint8_t*>(dest);

  const uint8x8_t p6_v = Load4(dst - 7 * stride);
  const uint8x8_t p5_v = Load4(dst - 6 * stride);
  const uint8x8_t p4_v = Load4(dst - 5 * stride);
  const uint8x8_t p3_v = Load4(dst - 4 * stride);
  const uint8x8_t p2_v = Load4(dst - 3 * stride);
  const uint8x8_t p1_v = Load4(dst - 2 * stride);
  const uint8x8_t p0_v = Load4(dst - stride);
  const uint8x8_t p0q0 = Load4<1>(dst, p0_v);
  const uint8x8_t p1q1 = Load4<1>(dst + stride, p1_v);
  const uint8x8_t p2q2 = Load4<1>(dst + 2 * stride, p2_v);
  const uint8x8_t p3q3 = Load4<1>(dst + 3 * stride, p3_v);
  const uint8x8_t p4q4 = Load4<1>(dst + 4 * stride, p4_v);
  const uint8x8_t p5q5 = Load4<1>(dst + 5 * stride, p5_v);
  const uint8x8_t p6q6 = Load4<1>(dst + 6 * stride, p6_v);

  uint8x8_t needs_filter8_mask, is_flat4_mask, hev_mask;
  Filter8Masks(p3q3, p2q2, p1q1, p0q0, hev_thresh, outer_thresh, inner_thresh,
               &needs_filter8_mask, &is_flat4_mask, &hev_mask);

  needs_filter8_mask = InterleaveLow32(needs_filter8_mask, needs_filter8_mask);
  is_flat4_mask = vand_u8(is_flat4_mask, needs_filter8_mask);
  is_flat4_mask = InterleaveLow32(is_flat4_mask, is_flat4_mask);
  hev_mask = InterleaveLow32(hev_mask, hev_mask);

#if defined(__aarch64__)
  if (vaddv_u8(needs_filter8_mask) == 0) {
    // None of the values will be filtered.
    return;
  }
#endif  // defined(__aarch64__)

  // Decide between Filter8() and Filter14().
  uint8x8_t is_flat_outer4_mask =
      IsFlat4(vabd_u8(p0q0, p4q4), vabd_u8(p0q0, p5q5), vabd_u8(p0q0, p6q6));
  is_flat_outer4_mask = vand_u8(is_flat4_mask, is_flat_outer4_mask);
  is_flat_outer4_mask =
      InterleaveLow32(is_flat_outer4_mask, is_flat_outer4_mask);

  uint8x8_t f_p1q1;
  uint8x8_t f_p0q0;
  const uint8x8x2_t q0p1xp0q1 = Interleave32(Transpose32(p0q0), p1q1);
  Filter4(q0p1xp0q1.val[0], q0p1xp0q1.val[1], hev_mask, &f_p1q1, &f_p0q0);
  // Reset the outer values if only a Hev() mask was required.
  f_p1q1 = vbsl_u8(hev_mask, p1q1, f_p1q1);

  uint8x8_t f8_p1q1, f8_p0q0;
  uint8x8_t f14_p2q2, f14_p1q1, f14_p0q0;
#if defined(__aarch64__)
  if (vaddv_u8(is_flat4_mask) == 0) {
    // Filter8() and Filter14() do not apply.
    const uint8x8_t zero = vdup_n_u8(0);
    f8_p1q1 = zero;
    f8_p0q0 = zero;
    f14_p1q1 = zero;
    f14_p0q0 = zero;
  } else {
#endif  // defined(__aarch64__)
    uint8x8_t f8_p2q2;
    Filter8(p3q3, p2q2, p1q1, p0q0, &f8_p2q2, &f8_p1q1, &f8_p0q0);

#if defined(__aarch64__)
    if (vaddv_u8(is_flat_outer4_mask) == 0) {
      // Filter14() does not apply.
      const uint8x8_t zero = vdup_n_u8(0);
      f14_p2q2 = zero;
      f14_p1q1 = zero;
      f14_p0q0 = zero;
    } else {
#endif  // defined(__aarch64__)
      uint8x8_t f14_p5q5, f14_p4q4, f14_p3q3;
      Filter14(p6q6, p5q5, p4q4, p3q3, p2q2, p1q1, p0q0, &f14_p5q5, &f14_p4q4,
               &f14_p3q3, &f14_p2q2, &f14_p1q1, &f14_p0q0);

      const uint8x8_t p5q5_output =
          vbsl_u8(is_flat_outer4_mask, f14_p5q5, p5q5);
      StoreLo4(dst - 6 * stride, p5q5_output);
      StoreHi4(dst + 5 * stride, p5q5_output);

      const uint8x8_t p4q4_output =
          vbsl_u8(is_flat_outer4_mask, f14_p4q4, p4q4);
      StoreLo4(dst - 5 * stride, p4q4_output);
      StoreHi4(dst + 4 * stride, p4q4_output);

      const uint8x8_t p3q3_output =
          vbsl_u8(is_flat_outer4_mask, f14_p3q3, p3q3);
      StoreLo4(dst - 4 * stride, p3q3_output);
      StoreHi4(dst + 3 * stride, p3q3_output);
#if defined(__aarch64__)
    }
#endif  // defined(__aarch64__)

    uint8x8_t p2q2_output = vbsl_u8(is_flat_outer4_mask, f14_p2q2, f8_p2q2);
    p2q2_output = vbsl_u8(is_flat4_mask, p2q2_output, p2q2);
    StoreLo4(dst - 3 * stride, p2q2_output);
    StoreHi4(dst + 2 * stride, p2q2_output);
#if defined(__aarch64__)
  }
#endif  // defined(__aarch64__)

  uint8x8_t p1q1_output = vbsl_u8(is_flat_outer4_mask, f14_p1q1, f8_p1q1);
  p1q1_output = vbsl_u8(is_flat4_mask, p1q1_output, f_p1q1);
  p1q1_output = vbsl_u8(needs_filter8_mask, p1q1_output, p1q1);
  StoreLo4(dst - 2 * stride, p1q1_output);
  StoreHi4(dst + stride, p1q1_output);

  uint8x8_t p0q0_output = vbsl_u8(is_flat_outer4_mask, f14_p0q0, f8_p0q0);
  p0q0_output = vbsl_u8(is_flat4_mask, p0q0_output, f_p0q0);
  p0q0_output = vbsl_u8(needs_filter8_mask, p0q0_output, p0q0);
  StoreLo4(dst - stride, p0q0_output);
  StoreHi4(dst, p0q0_output);
}

void Vertical14_NEON(void* const dest, const ptrdiff_t stride,
                     const int outer_thresh, const int inner_thresh,
                     const int hev_thresh) {
  auto* dst = static_cast<uint8_t*>(dest);
  dst -= 8;
  // input
  // p7 p6 p5 p4 p3 p2 p1 p0  q0 q1 q2 q3 q4 q5 q6 q7
  const uint8x16_t x0 = vld1q_u8(dst);
  dst += stride;
  const uint8x16_t x1 = vld1q_u8(dst);
  dst += stride;
  const uint8x16_t x2 = vld1q_u8(dst);
  dst += stride;
  const uint8x16_t x3 = vld1q_u8(dst);
  dst -= (stride * 3);

  // re-order input
#if defined(__aarch64__)
  const uint8x8_t index_qp3toqp0 = vcreate_u8(0x0b0a090804050607);
  const uint8x8_t index_qp7toqp4 = vcreate_u8(0x0f0e0d0c00010203);
  const uint8x16_t index_qp7toqp0 = vcombine_u8(index_qp3toqp0, index_qp7toqp4);

  uint8x16_t input_0 = vqtbl1q_u8(x0, index_qp7toqp0);
  uint8x16_t input_1 = vqtbl1q_u8(x1, index_qp7toqp0);
  uint8x16_t input_2 = vqtbl1q_u8(x2, index_qp7toqp0);
  uint8x16_t input_3 = vqtbl1q_u8(x3, index_qp7toqp0);
#else
  const uint8x8_t index_qp3toqp0 = vcreate_u8(0x0b0a090804050607);
  const uint8x8_t index_qp7toqp4 = vcreate_u8(0x0f0e0d0c00010203);

  const uint8x8_t x0_qp3qp0 = VQTbl1U8(x0, index_qp3toqp0);
  const uint8x8_t x1_qp3qp0 = VQTbl1U8(x1, index_qp3toqp0);
  const uint8x8_t x2_qp3qp0 = VQTbl1U8(x2, index_qp3toqp0);
  const uint8x8_t x3_qp3qp0 = VQTbl1U8(x3, index_qp3toqp0);

  const uint8x8_t x0_qp7qp4 = VQTbl1U8(x0, index_qp7toqp4);
  const uint8x8_t x1_qp7qp4 = VQTbl1U8(x1, index_qp7toqp4);
  const uint8x8_t x2_qp7qp4 = VQTbl1U8(x2, index_qp7toqp4);
  const uint8x8_t x3_qp7qp4 = VQTbl1U8(x3, index_qp7toqp4);

  const uint8x16_t input_0 = vcombine_u8(x0_qp3qp0, x0_qp7qp4);
  const uint8x16_t input_1 = vcombine_u8(x1_qp3qp0, x1_qp7qp4);
  const uint8x16_t input_2 = vcombine_u8(x2_qp3qp0, x2_qp7qp4);
  const uint8x16_t input_3 = vcombine_u8(x3_qp3qp0, x3_qp7qp4);
#endif
  // input after re-order
  // p0 p1 p2 p3 q0 q1 q2 q3  p4 p5 p6 p7 q4 q5 q6 q7

  const uint8x16x2_t in01 = vtrnq_u8(input_0, input_1);
  const uint8x16x2_t in23 = vtrnq_u8(input_2, input_3);
  const uint16x8x2_t in02 = vtrnq_u16(vreinterpretq_u16_u8(in01.val[0]),
                                      vreinterpretq_u16_u8(in23.val[0]));
  const uint16x8x2_t in13 = vtrnq_u16(vreinterpretq_u16_u8(in01.val[1]),
                                      vreinterpretq_u16_u8(in23.val[1]));

  const uint8x8_t p0q0 = vget_low_u8(vreinterpretq_u8_u16(in02.val[0]));
  const uint8x8_t p1q1 = vget_low_u8(vreinterpretq_u8_u16(in13.val[0]));

  const uint8x8_t p2q2 = vget_low_u8(vreinterpretq_u8_u16(in02.val[1]));
  const uint8x8_t p3q3 = vget_low_u8(vreinterpretq_u8_u16(in13.val[1]));

  const uint8x8_t p4q4 = vget_high_u8(vreinterpretq_u8_u16(in02.val[0]));
  const uint8x8_t p5q5 = vget_high_u8(vreinterpretq_u8_u16(in13.val[0]));

  const uint8x8_t p6q6 = vget_high_u8(vreinterpretq_u8_u16(in02.val[1]));
  const uint8x8_t p7q7 = vget_high_u8(vreinterpretq_u8_u16(in13.val[1]));

  uint8x8_t needs_filter8_mask, is_flat4_mask, hev_mask;
  Filter8Masks(p3q3, p2q2, p1q1, p0q0, hev_thresh, outer_thresh, inner_thresh,
               &needs_filter8_mask, &is_flat4_mask, &hev_mask);

  needs_filter8_mask = InterleaveLow32(needs_filter8_mask, needs_filter8_mask);
  is_flat4_mask = vand_u8(is_flat4_mask, needs_filter8_mask);
  is_flat4_mask = InterleaveLow32(is_flat4_mask, is_flat4_mask);
  hev_mask = InterleaveLow32(hev_mask, hev_mask);

#if defined(__aarch64__)
  if (vaddv_u8(needs_filter8_mask) == 0) {
    // None of the values will be filtered.
    return;
  }
#endif  // defined(__aarch64__)

  // Decide between Filter8() and Filter14().
  uint8x8_t is_flat_outer4_mask =
      IsFlat4(vabd_u8(p0q0, p4q4), vabd_u8(p0q0, p5q5), vabd_u8(p0q0, p6q6));
  is_flat_outer4_mask = vand_u8(is_flat4_mask, is_flat_outer4_mask);
  is_flat_outer4_mask =
      InterleaveLow32(is_flat_outer4_mask, is_flat_outer4_mask);

  uint8x8_t f_p0q0, f_p1q1;
  const uint8x8x2_t q0p1xp0q1 = Interleave32(Transpose32(p0q0), p1q1);
  Filter4(q0p1xp0q1.val[0], q0p1xp0q1.val[1], hev_mask, &f_p1q1, &f_p0q0);
  // Reset the outer values if only a Hev() mask was required.
  f_p1q1 = vbsl_u8(hev_mask, p1q1, f_p1q1);

  uint8x8_t p1q1_output, p0q0_output;
  uint8x8_t p5q5_output, p4q4_output, p3q3_output, p2q2_output;

#if defined(__aarch64__)
  if (vaddv_u8(is_flat4_mask) == 0) {
    // Filter8() and Filter14() do not apply.
    p1q1_output = p1q1;
    p0q0_output = p0q0;

    p5q5_output = p5q5;
    p4q4_output = p4q4;
    p3q3_output = p3q3;
    p2q2_output = p2q2;
  } else {
#endif  // defined(__aarch64__)
    uint8x8_t f8_p2q2, f8_p1q1, f8_p0q0;
    Filter8(p3q3, p2q2, p1q1, p0q0, &f8_p2q2, &f8_p1q1, &f8_p0q0);

#if defined(__aarch64__)
    if (vaddv_u8(is_flat_outer4_mask) == 0) {
      // Filter14() does not apply.
      p5q5_output = p5q5;
      p4q4_output = p4q4;
      p3q3_output = p3q3;
      p2q2_output = f8_p2q2;
      p1q1_output = f8_p1q1;
      p0q0_output = f8_p0q0;
    } else {
#endif  // defined(__aarch64__)
      uint8x8_t f14_p5q5, f14_p4q4, f14_p3q3, f14_p2q2, f14_p1q1, f14_p0q0;
      Filter14(p6q6, p5q5, p4q4, p3q3, p2q2, p1q1, p0q0, &f14_p5q5, &f14_p4q4,
               &f14_p3q3, &f14_p2q2, &f14_p1q1, &f14_p0q0);

      p5q5_output = vbsl_u8(is_flat_outer4_mask, f14_p5q5, p5q5);
      p4q4_output = vbsl_u8(is_flat_outer4_mask, f14_p4q4, p4q4);
      p3q3_output = vbsl_u8(is_flat_outer4_mask, f14_p3q3, p3q3);
      p2q2_output = vbsl_u8(is_flat_outer4_mask, f14_p2q2, f8_p2q2);
      p1q1_output = vbsl_u8(is_flat_outer4_mask, f14_p1q1, f8_p1q1);
      p0q0_output = vbsl_u8(is_flat_outer4_mask, f14_p0q0, f8_p0q0);
#if defined(__aarch64__)
    }
#endif  // defined(__aarch64__)
    p2q2_output = vbsl_u8(is_flat4_mask, p2q2_output, p2q2);
#if defined(__aarch64__)
  }
#endif  // defined(__aarch64__)

  p1q1_output = vbsl_u8(is_flat4_mask, p1q1_output, f_p1q1);
  p1q1_output = vbsl_u8(needs_filter8_mask, p1q1_output, p1q1);
  p0q0_output = vbsl_u8(is_flat4_mask, p0q0_output, f_p0q0);
  p0q0_output = vbsl_u8(needs_filter8_mask, p0q0_output, p0q0);

  const uint8x16_t p0q0_p4q4 = vcombine_u8(p0q0_output, p4q4_output);
  const uint8x16_t p2q2_p6q6 = vcombine_u8(p2q2_output, p6q6);
  const uint8x16_t p1q1_p5q5 = vcombine_u8(p1q1_output, p5q5_output);
  const uint8x16_t p3q3_p7q7 = vcombine_u8(p3q3_output, p7q7);

  const uint16x8x2_t out02 = vtrnq_u16(vreinterpretq_u16_u8(p0q0_p4q4),
                                       vreinterpretq_u16_u8(p2q2_p6q6));
  const uint16x8x2_t out13 = vtrnq_u16(vreinterpretq_u16_u8(p1q1_p5q5),
                                       vreinterpretq_u16_u8(p3q3_p7q7));
  const uint8x16x2_t out01 = vtrnq_u8(vreinterpretq_u8_u16(out02.val[0]),
                                      vreinterpretq_u8_u16(out13.val[0]));
  const uint8x16x2_t out23 = vtrnq_u8(vreinterpretq_u8_u16(out02.val[1]),
                                      vreinterpretq_u8_u16(out13.val[1]));

#if defined(__aarch64__)
  const uint8x8_t index_p7top0 = vcreate_u8(0x0001020308090a0b);
  const uint8x8_t index_q7toq0 = vcreate_u8(0x0f0e0d0c07060504);
  const uint8x16_t index_p7toq7 = vcombine_u8(index_p7top0, index_q7toq0);

  const uint8x16_t output_0 = vqtbl1q_u8(out01.val[0], index_p7toq7);
  const uint8x16_t output_1 = vqtbl1q_u8(out01.val[1], index_p7toq7);
  const uint8x16_t output_2 = vqtbl1q_u8(out23.val[0], index_p7toq7);
  const uint8x16_t output_3 = vqtbl1q_u8(out23.val[1], index_p7toq7);
#else
  const uint8x8_t index_p7top0 = vcreate_u8(0x0001020308090a0b);
  const uint8x8_t index_q7toq0 = vcreate_u8(0x0f0e0d0c07060504);

  const uint8x8_t x0_p7p0 = VQTbl1U8(out01.val[0], index_p7top0);
  const uint8x8_t x1_p7p0 = VQTbl1U8(out01.val[1], index_p7top0);
  const uint8x8_t x2_p7p0 = VQTbl1U8(out23.val[0], index_p7top0);
  const uint8x8_t x3_p7p0 = VQTbl1U8(out23.val[1], index_p7top0);

  const uint8x8_t x0_q7q0 = VQTbl1U8(out01.val[0], index_q7toq0);
  const uint8x8_t x1_q7q0 = VQTbl1U8(out01.val[1], index_q7toq0);
  const uint8x8_t x2_q7q0 = VQTbl1U8(out23.val[0], index_q7toq0);
  const uint8x8_t x3_q7q0 = VQTbl1U8(out23.val[1], index_q7toq0);

  const uint8x16_t output_0 = vcombine_u8(x0_p7p0, x0_q7q0);
  const uint8x16_t output_1 = vcombine_u8(x1_p7p0, x1_q7q0);
  const uint8x16_t output_2 = vcombine_u8(x2_p7p0, x2_q7q0);
  const uint8x16_t output_3 = vcombine_u8(x3_p7p0, x3_q7q0);
#endif

  vst1q_u8(dst, output_0);
  dst += stride;
  vst1q_u8(dst, output_1);
  dst += stride;
  vst1q_u8(dst, output_2);
  dst += stride;
  vst1q_u8(dst, output_3);
}

}  // namespace

void LoopFilterInit_NEON() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
  dsp->loop_filters[kLoopFilterSize4][kLoopFilterTypeHorizontal] =
      Horizontal4_NEON;
  dsp->loop_filters[kLoopFilterSize4][kLoopFilterTypeVertical] = Vertical4_NEON;

  dsp->loop_filters[kLoopFilterSize6][kLoopFilterTypeHorizontal] =
      Horizontal6_NEON;
  dsp->loop_filters[kLoopFilterSize6][kLoopFilterTypeVertical] = Vertical6_NEON;

  dsp->loop_filters[kLoopFilterSize8][kLoopFilterTypeHorizontal] =
      Horizontal8_NEON;
  dsp->loop_filters[kLoopFilterSize8][kLoopFilterTypeVertical] = Vertical8_NEON;

  dsp->loop_filters[kLoopFilterSize14][kLoopFilterTypeHorizontal] =
      Horizontal14_NEON;
  dsp->loop_filters[kLoopFilterSize14][kLoopFilterTypeVertical] =
      Vertical14_NEON;
}

}  // namespace dsp
}  // namespace libgav1

#else   // !LIBGAV1_ENABLE_NEON
namespace libgav1 {
namespace dsp {

void LoopFilterInit_NEON() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_ENABLE_NEON
