// Copyright 2020 The libgav1 Authors
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

#include "src/dsp/motion_field_projection.h"
#include "src/utils/cpu.h"

#if LIBGAV1_ENABLE_NEON

#include <arm_neon.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/types.h"

namespace libgav1 {
namespace dsp {
namespace {

inline int16x8_t LoadDivision(const int8x8x2_t division_table,
                              const int8x8_t reference_offset) {
  const int8x8_t kOne = vcreate_s8(0x0100010001000100);
  const int8x16_t kOneQ = vcombine_s8(kOne, kOne);
  const int8x8_t t = vadd_s8(reference_offset, reference_offset);
  const int8x8x2_t tt = vzip_s8(t, t);
  const int8x16_t t1 = vcombine_s8(tt.val[0], tt.val[1]);
  const int8x16_t idx = vaddq_s8(t1, kOneQ);
  const int8x8_t idx_low = vget_low_s8(idx);
  const int8x8_t idx_high = vget_high_s8(idx);
  const int16x4_t d0 = vreinterpret_s16_s8(vtbl2_s8(division_table, idx_low));
  const int16x4_t d1 = vreinterpret_s16_s8(vtbl2_s8(division_table, idx_high));
  return vcombine_s16(d0, d1);
}

inline int16x4_t MvProjection(const int16x4_t mv, const int16x4_t denominator,
                              const int numerator) {
  const int32x4_t m0 = vmull_s16(mv, denominator);
  const int32x4_t m = vmulq_n_s32(m0, numerator);
  // Add the sign (0 or -1) to round towards zero.
  const int32x4_t add_sign = vsraq_n_s32(m, m, 31);
  return vqrshrn_n_s32(add_sign, 14);
}

inline int16x8_t MvProjectionClip(const int16x8_t mv,
                                  const int16x8_t denominator,
                                  const int numerator) {
  const int16x4_t mv0 = vget_low_s16(mv);
  const int16x4_t mv1 = vget_high_s16(mv);
  const int16x4_t s0 = MvProjection(mv0, vget_low_s16(denominator), numerator);
  const int16x4_t s1 = MvProjection(mv1, vget_high_s16(denominator), numerator);
  const int16x8_t projection = vcombine_s16(s0, s1);
  const int16x8_t projection_mv_clamp = vdupq_n_s16(kProjectionMvClamp);
  const int16x8_t clamp = vminq_s16(projection, projection_mv_clamp);
  return vmaxq_s16(clamp, vnegq_s16(projection_mv_clamp));
}

inline int8x8_t Project_NEON(const int16x8_t delta, const int16x8_t dst_sign) {
  // Add 63 to negative delta so that it shifts towards zero.
  const int16x8_t delta_sign = vshrq_n_s16(delta, 15);
  const uint16x8_t delta_u = vreinterpretq_u16_s16(delta);
  const uint16x8_t delta_sign_u = vreinterpretq_u16_s16(delta_sign);
  const uint16x8_t delta_adjust_u = vsraq_n_u16(delta_u, delta_sign_u, 10);
  const int16x8_t delta_adjust = vreinterpretq_s16_u16(delta_adjust_u);
  const int16x8_t offset0 = vshrq_n_s16(delta_adjust, 6);
  const int16x8_t offset1 = veorq_s16(offset0, dst_sign);
  const int16x8_t offset2 = vsubq_s16(offset1, dst_sign);
  return vqmovn_s16(offset2);
}

inline void GetPosition(
    const int8x8x2_t division_table, const MotionVector* const mv,
    const int numerator, const int x8_start, const int x8_end, const int x8,
    const int8x8_t r_offsets, const int8x8_t source_reference_type8,
    const int8x8_t skip_r, const int8x8_t y8_floor8, const int8x8_t y8_ceiling8,
    const int16x8_t d_sign, const int delta, int8x8_t* const r,
    int8x8_t* const position_y8, int8x8_t* const position_x8,
    int64_t* const skip_64, int32x4_t mvs[2]) {
  const auto* const mv_int = reinterpret_cast<const int32_t*>(mv + x8);
  *r = vtbl1_s8(r_offsets, source_reference_type8);
  const int16x8_t denorm = LoadDivision(division_table, source_reference_type8);
  int16x8_t projection_mv[2];
  mvs[0] = vld1q_s32(mv_int + 0);
  mvs[1] = vld1q_s32(mv_int + 4);
  // Deinterlace x and y components
  const int16x8_t mv0 = vreinterpretq_s16_s32(mvs[0]);
  const int16x8_t mv1 = vreinterpretq_s16_s32(mvs[1]);
  const int16x8x2_t mv_yx = vuzpq_s16(mv0, mv1);
  // numerator could be 0.
  projection_mv[0] = MvProjectionClip(mv_yx.val[0], denorm, numerator);
  projection_mv[1] = MvProjectionClip(mv_yx.val[1], denorm, numerator);
  // Do not update the motion vector if the block position is not valid or
  // if position_x8 is outside the current range of x8_start and x8_end.
  // Note that position_y8 will always be within the range of y8_start and
  // y8_end.
  // After subtracting the base, valid projections are within 8-bit.
  *position_y8 = Project_NEON(projection_mv[0], d_sign);
  const int8x8_t position_x = Project_NEON(projection_mv[1], d_sign);
  const int8x8_t k01234567 = vcreate_s8(uint64_t{0x0706050403020100});
  *position_x8 = vqadd_s8(position_x, k01234567);
  const int8x16_t position_xy = vcombine_s8(*position_x8, *position_y8);
  const int x8_floor = std::max(
      x8_start - x8, delta - kProjectionMvMaxHorizontalOffset);  // [-8, 8]
  const int x8_ceiling = std::min(
      x8_end - x8, delta + 8 + kProjectionMvMaxHorizontalOffset);  // [0, 16]
  const int8x8_t x8_floor8 = vdup_n_s8(x8_floor);
  const int8x8_t x8_ceiling8 = vdup_n_s8(x8_ceiling);
  const int8x16_t floor_xy = vcombine_s8(x8_floor8, y8_floor8);
  const int8x16_t ceiling_xy = vcombine_s8(x8_ceiling8, y8_ceiling8);
  const uint8x16_t underflow = vcltq_s8(position_xy, floor_xy);
  const uint8x16_t overflow = vcgeq_s8(position_xy, ceiling_xy);
  const int8x16_t out = vreinterpretq_s8_u8(vorrq_u8(underflow, overflow));
  const int8x8_t skip_low = vorr_s8(skip_r, vget_low_s8(out));
  const int8x8_t skip = vorr_s8(skip_low, vget_high_s8(out));
  *skip_64 = vget_lane_s64(vreinterpret_s64_s8(skip), 0);
}

template <int idx>
inline void Store(const int16x8_t position, const int8x8_t reference_offset,
                  const int32x4_t mv, int8_t* dst_reference_offset,
                  MotionVector* dst_mv) {
  const ptrdiff_t offset = vgetq_lane_s16(position, idx);
  auto* const d_mv = reinterpret_cast<int32_t*>(&dst_mv[offset]);
  vst1q_lane_s32(d_mv, mv, idx & 3);
  vst1_lane_s8(&dst_reference_offset[offset], reference_offset, idx);
}

template <int idx>
inline void CheckStore(const int8_t* skips, const int16x8_t position,
                       const int8x8_t reference_offset, const int32x4_t mv,
                       int8_t* dst_reference_offset, MotionVector* dst_mv) {
  if (skips[idx] == 0) {
    Store<idx>(position, reference_offset, mv, dst_reference_offset, dst_mv);
  }
}

// 7.9.2.
void MotionFieldProjectionKernel_NEON(const ReferenceInfo& reference_info,
                                      const int reference_to_current_with_sign,
                                      const int dst_sign, const int y8_start,
                                      const int y8_end, const int x8_start,
                                      const int x8_end,
                                      TemporalMotionField* const motion_field) {
  const ptrdiff_t stride = motion_field->mv.columns();
  // The column range has to be offset by kProjectionMvMaxHorizontalOffset since
  // coordinates in that range could end up being position_x8 because of
  // projection.
  const int adjusted_x8_start =
      std::max(x8_start - kProjectionMvMaxHorizontalOffset, 0);
  const int adjusted_x8_end = std::min(
      x8_end + kProjectionMvMaxHorizontalOffset, static_cast<int>(stride));
  const int adjusted_x8_end8 = adjusted_x8_end & ~7;
  const int leftover = adjusted_x8_end - adjusted_x8_end8;
  const int8_t* const reference_offsets =
      reference_info.relative_distance_to.data();
  const bool* const skip_references = reference_info.skip_references.data();
  const int16_t* const projection_divisions =
      reference_info.projection_divisions.data();
  const ReferenceFrameType* source_reference_types =
      &reference_info.motion_field_reference_frame[y8_start][0];
  const MotionVector* mv = &reference_info.motion_field_mv[y8_start][0];
  int8_t* dst_reference_offset = motion_field->reference_offset[y8_start];
  MotionVector* dst_mv = motion_field->mv[y8_start];
  const int16x8_t d_sign = vdupq_n_s16(dst_sign);

  static_assert(sizeof(int8_t) == sizeof(bool), "");
  static_assert(sizeof(int8_t) == sizeof(ReferenceFrameType), "");
  static_assert(sizeof(int32_t) == sizeof(MotionVector), "");
  assert(dst_sign == 0 || dst_sign == -1);
  assert(stride == motion_field->reference_offset.columns());
  assert((y8_start & 7) == 0);
  assert((adjusted_x8_start & 7) == 0);
  // The final position calculation is represented with int16_t. Valid
  // position_y8 from its base is at most 7. After considering the horizontal
  // offset which is at most |stride - 1|, we have the following assertion,
  // which means this optimization works for frame width up to 32K (each
  // position is a 8x8 block).
  assert(8 * stride <= 32768);
  const int8x8_t skip_reference =
      vld1_s8(reinterpret_cast<const int8_t*>(skip_references));
  const int8x8_t r_offsets = vld1_s8(reference_offsets);
  const int8x16_t table = vreinterpretq_s8_s16(vld1q_s16(projection_divisions));
  int8x8x2_t division_table;
  division_table.val[0] = vget_low_s8(table);
  division_table.val[1] = vget_high_s8(table);

  int y8 = y8_start;
  do {
    const int y8_floor = (y8 & ~7) - y8;                         // [-7, 0]
    const int y8_ceiling = std::min(y8_end - y8, y8_floor + 8);  // [1, 8]
    const int8x8_t y8_floor8 = vdup_n_s8(y8_floor);
    const int8x8_t y8_ceiling8 = vdup_n_s8(y8_ceiling);
    int x8;

    for (x8 = adjusted_x8_start; x8 < adjusted_x8_end8; x8 += 8) {
      const int8x8_t source_reference_type8 =
          vld1_s8(reinterpret_cast<const int8_t*>(source_reference_types + x8));
      const int8x8_t skip_r = vtbl1_s8(skip_reference, source_reference_type8);
      const int64_t early_skip = vget_lane_s64(vreinterpret_s64_s8(skip_r), 0);
      // Early termination #1 if all are skips. Chance is typically ~30-40%.
      if (early_skip == -1) continue;
      int64_t skip_64;
      int8x8_t r, position_x8, position_y8;
      int32x4_t mvs[2];
      GetPosition(division_table, mv, reference_to_current_with_sign, x8_start,
                  x8_end, x8, r_offsets, source_reference_type8, skip_r,
                  y8_floor8, y8_ceiling8, d_sign, 0, &r, &position_y8,
                  &position_x8, &skip_64, mvs);
      // Early termination #2 if all are skips.
      // Chance is typically ~15-25% after Early termination #1.
      if (skip_64 == -1) continue;
      const int16x8_t p_y = vmovl_s8(position_y8);
      const int16x8_t p_x = vmovl_s8(position_x8);
      const int16x8_t pos = vmlaq_n_s16(p_x, p_y, stride);
      const int16x8_t position = vaddq_s16(pos, vdupq_n_s16(x8));
      if (skip_64 == 0) {
        // Store all. Chance is typically ~70-85% after Early termination #2.
        Store<0>(position, r, mvs[0], dst_reference_offset, dst_mv);
        Store<1>(position, r, mvs[0], dst_reference_offset, dst_mv);
        Store<2>(position, r, mvs[0], dst_reference_offset, dst_mv);
        Store<3>(position, r, mvs[0], dst_reference_offset, dst_mv);
        Store<4>(position, r, mvs[1], dst_reference_offset, dst_mv);
        Store<5>(position, r, mvs[1], dst_reference_offset, dst_mv);
        Store<6>(position, r, mvs[1], dst_reference_offset, dst_mv);
        Store<7>(position, r, mvs[1], dst_reference_offset, dst_mv);
      } else {
        // Check and store each.
        // Chance is typically ~15-30% after Early termination #2.
        // The compiler is smart enough to not create the local buffer skips[].
        int8_t skips[8];
        memcpy(skips, &skip_64, sizeof(skips));
        CheckStore<0>(skips, position, r, mvs[0], dst_reference_offset, dst_mv);
        CheckStore<1>(skips, position, r, mvs[0], dst_reference_offset, dst_mv);
        CheckStore<2>(skips, position, r, mvs[0], dst_reference_offset, dst_mv);
        CheckStore<3>(skips, position, r, mvs[0], dst_reference_offset, dst_mv);
        CheckStore<4>(skips, position, r, mvs[1], dst_reference_offset, dst_mv);
        CheckStore<5>(skips, position, r, mvs[1], dst_reference_offset, dst_mv);
        CheckStore<6>(skips, position, r, mvs[1], dst_reference_offset, dst_mv);
        CheckStore<7>(skips, position, r, mvs[1], dst_reference_offset, dst_mv);
      }
    }

    // The following leftover processing cannot be moved out of the do...while
    // loop. Doing so may change the result storing orders of the same position.
    if (leftover > 0) {
      // Use SIMD only when leftover is at least 4, and there are at least 8
      // elements in a row.
      if (leftover >= 4 && adjusted_x8_start < adjusted_x8_end8) {
        // Process the last 8 elements to avoid loading invalid memory. Some
        // elements may have been processed in the above loop, which is OK.
        const int delta = 8 - leftover;
        x8 = adjusted_x8_end - 8;
        const int8x8_t source_reference_type8 = vld1_s8(
            reinterpret_cast<const int8_t*>(source_reference_types + x8));
        const int8x8_t skip_r =
            vtbl1_s8(skip_reference, source_reference_type8);
        const int64_t early_skip =
            vget_lane_s64(vreinterpret_s64_s8(skip_r), 0);
        // Early termination #1 if all are skips.
        if (early_skip != -1) {
          int64_t skip_64;
          int8x8_t r, position_x8, position_y8;
          int32x4_t mvs[2];
          GetPosition(division_table, mv, reference_to_current_with_sign,
                      x8_start, x8_end, x8, r_offsets, source_reference_type8,
                      skip_r, y8_floor8, y8_ceiling8, d_sign, delta, &r,
                      &position_y8, &position_x8, &skip_64, mvs);
          // Early termination #2 if all are skips.
          if (skip_64 != -1) {
            const int16x8_t p_y = vmovl_s8(position_y8);
            const int16x8_t p_x = vmovl_s8(position_x8);
            const int16x8_t pos = vmlaq_n_s16(p_x, p_y, stride);
            const int16x8_t position = vaddq_s16(pos, vdupq_n_s16(x8));
            // Store up to 7 elements since leftover is at most 7.
            if (skip_64 == 0) {
              // Store all.
              Store<1>(position, r, mvs[0], dst_reference_offset, dst_mv);
              Store<2>(position, r, mvs[0], dst_reference_offset, dst_mv);
              Store<3>(position, r, mvs[0], dst_reference_offset, dst_mv);
              Store<4>(position, r, mvs[1], dst_reference_offset, dst_mv);
              Store<5>(position, r, mvs[1], dst_reference_offset, dst_mv);
              Store<6>(position, r, mvs[1], dst_reference_offset, dst_mv);
              Store<7>(position, r, mvs[1], dst_reference_offset, dst_mv);
            } else {
              // Check and store each.
              // The compiler is smart enough to not create the local buffer
              // skips[].
              int8_t skips[8];
              memcpy(skips, &skip_64, sizeof(skips));
              CheckStore<1>(skips, position, r, mvs[0], dst_reference_offset,
                            dst_mv);
              CheckStore<2>(skips, position, r, mvs[0], dst_reference_offset,
                            dst_mv);
              CheckStore<3>(skips, position, r, mvs[0], dst_reference_offset,
                            dst_mv);
              CheckStore<4>(skips, position, r, mvs[1], dst_reference_offset,
                            dst_mv);
              CheckStore<5>(skips, position, r, mvs[1], dst_reference_offset,
                            dst_mv);
              CheckStore<6>(skips, position, r, mvs[1], dst_reference_offset,
                            dst_mv);
              CheckStore<7>(skips, position, r, mvs[1], dst_reference_offset,
                            dst_mv);
            }
          }
        }
      } else {
        for (; x8 < adjusted_x8_end; ++x8) {
          const int source_reference_type = source_reference_types[x8];
          if (skip_references[source_reference_type]) continue;
          MotionVector projection_mv;
          // reference_to_current_with_sign could be 0.
          GetMvProjection(mv[x8], reference_to_current_with_sign,
                          projection_divisions[source_reference_type],
                          &projection_mv);
          // Do not update the motion vector if the block position is not valid
          // or if position_x8 is outside the current range of x8_start and
          // x8_end. Note that position_y8 will always be within the range of
          // y8_start and y8_end.
          const int position_y8 = Project(0, projection_mv.mv[0], dst_sign);
          if (position_y8 < y8_floor || position_y8 >= y8_ceiling) continue;
          const int x8_base = x8 & ~7;
          const int x8_floor =
              std::max(x8_start, x8_base - kProjectionMvMaxHorizontalOffset);
          const int x8_ceiling =
              std::min(x8_end, x8_base + 8 + kProjectionMvMaxHorizontalOffset);
          const int position_x8 = Project(x8, projection_mv.mv[1], dst_sign);
          if (position_x8 < x8_floor || position_x8 >= x8_ceiling) continue;
          dst_mv[position_y8 * stride + position_x8] = mv[x8];
          dst_reference_offset[position_y8 * stride + position_x8] =
              reference_offsets[source_reference_type];
        }
      }
    }

    source_reference_types += stride;
    mv += stride;
    dst_reference_offset += stride;
    dst_mv += stride;
  } while (++y8 < y8_end);
}

}  // namespace

void MotionFieldProjectionInit_NEON() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
  dsp->motion_field_projection_kernel = MotionFieldProjectionKernel_NEON;
}

}  // namespace dsp
}  // namespace libgav1

#else   // !LIBGAV1_ENABLE_NEON
namespace libgav1 {
namespace dsp {

void MotionFieldProjectionInit_NEON() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_ENABLE_NEON
