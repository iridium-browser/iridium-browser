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

#if LIBGAV1_TARGETING_SSE4_1

#include <smmintrin.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/dsp/x86/common_sse4.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/types.h"

namespace libgav1 {
namespace dsp {
namespace {

inline __m128i LoadDivision(const __m128i division_table,
                            const __m128i reference_offset) {
  const __m128i kOne = _mm_set1_epi16(0x0100);
  const __m128i t = _mm_add_epi8(reference_offset, reference_offset);
  const __m128i tt = _mm_unpacklo_epi8(t, t);
  const __m128i idx = _mm_add_epi8(tt, kOne);
  return _mm_shuffle_epi8(division_table, idx);
}

inline __m128i MvProjection(const __m128i mv, const __m128i denominator,
                            const int numerator) {
  const __m128i m0 = _mm_madd_epi16(mv, denominator);
  const __m128i m = _mm_mullo_epi32(m0, _mm_set1_epi32(numerator));
  // Add the sign (0 or -1) to round towards zero.
  const __m128i sign = _mm_srai_epi32(m, 31);
  const __m128i add_sign = _mm_add_epi32(m, sign);
  const __m128i sum = _mm_add_epi32(add_sign, _mm_set1_epi32(1 << 13));
  return _mm_srai_epi32(sum, 14);
}

inline __m128i MvProjectionClip(const __m128i mv, const __m128i denominator,
                                const int numerator) {
  const __m128i mv0 = _mm_unpacklo_epi16(mv, _mm_setzero_si128());
  const __m128i mv1 = _mm_unpackhi_epi16(mv, _mm_setzero_si128());
  const __m128i denorm0 = _mm_unpacklo_epi16(denominator, _mm_setzero_si128());
  const __m128i denorm1 = _mm_unpackhi_epi16(denominator, _mm_setzero_si128());
  const __m128i s0 = MvProjection(mv0, denorm0, numerator);
  const __m128i s1 = MvProjection(mv1, denorm1, numerator);
  const __m128i projection = _mm_packs_epi32(s0, s1);
  const __m128i projection_mv_clamp = _mm_set1_epi16(kProjectionMvClamp);
  const __m128i projection_mv_clamp_negative =
      _mm_set1_epi16(-kProjectionMvClamp);
  const __m128i clamp = _mm_min_epi16(projection, projection_mv_clamp);
  return _mm_max_epi16(clamp, projection_mv_clamp_negative);
}

inline __m128i Project_SSE4_1(const __m128i delta, const __m128i dst_sign) {
  // Add 63 to negative delta so that it shifts towards zero.
  const __m128i delta_sign = _mm_srai_epi16(delta, 15);
  const __m128i delta_sign_63 = _mm_srli_epi16(delta_sign, 10);
  const __m128i delta_adjust = _mm_add_epi16(delta, delta_sign_63);
  const __m128i offset0 = _mm_srai_epi16(delta_adjust, 6);
  const __m128i offset1 = _mm_xor_si128(offset0, dst_sign);
  return _mm_sub_epi16(offset1, dst_sign);
}

inline void GetPosition(
    const __m128i division_table, const MotionVector* const mv,
    const int numerator, const int x8_start, const int x8_end, const int x8,
    const __m128i& r_offsets, const __m128i& source_reference_type8,
    const __m128i& skip_r, const __m128i& y8_floor8, const __m128i& y8_ceiling8,
    const __m128i& d_sign, const int delta, __m128i* const r,
    __m128i* const position_xy, int64_t* const skip_64, __m128i mvs[2]) {
  const auto* const mv_int = reinterpret_cast<const int32_t*>(mv + x8);
  *r = _mm_shuffle_epi8(r_offsets, source_reference_type8);
  const __m128i denorm = LoadDivision(division_table, source_reference_type8);
  __m128i projection_mv[2];
  mvs[0] = LoadUnaligned16(mv_int + 0);
  mvs[1] = LoadUnaligned16(mv_int + 4);
  // Deinterlace x and y components
  const __m128i kShuffle =
      _mm_setr_epi8(0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15);
  const __m128i mv0 = _mm_shuffle_epi8(mvs[0], kShuffle);
  const __m128i mv1 = _mm_shuffle_epi8(mvs[1], kShuffle);
  const __m128i mv_y = _mm_unpacklo_epi64(mv0, mv1);
  const __m128i mv_x = _mm_unpackhi_epi64(mv0, mv1);
  // numerator could be 0.
  projection_mv[0] = MvProjectionClip(mv_y, denorm, numerator);
  projection_mv[1] = MvProjectionClip(mv_x, denorm, numerator);
  // Do not update the motion vector if the block position is not valid or
  // if position_x8 is outside the current range of x8_start and x8_end.
  // Note that position_y8 will always be within the range of y8_start and
  // y8_end.
  // After subtracting the base, valid projections are within 8-bit.
  const __m128i position_y = Project_SSE4_1(projection_mv[0], d_sign);
  const __m128i position_x = Project_SSE4_1(projection_mv[1], d_sign);
  const __m128i positions = _mm_packs_epi16(position_x, position_y);
  const __m128i k01234567 =
      _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 0, 0, 0, 0, 0, 0, 0, 0);
  *position_xy = _mm_add_epi8(positions, k01234567);
  const int x8_floor = std::max(
      x8_start - x8, delta - kProjectionMvMaxHorizontalOffset);  // [-8, 8]
  const int x8_ceiling =
      std::min(x8_end - x8, delta + 8 + kProjectionMvMaxHorizontalOffset) -
      1;  // [-1, 15]
  const __m128i x8_floor8 = _mm_set1_epi8(x8_floor);
  const __m128i x8_ceiling8 = _mm_set1_epi8(x8_ceiling);
  const __m128i floor_xy = _mm_unpacklo_epi64(x8_floor8, y8_floor8);
  const __m128i ceiling_xy = _mm_unpacklo_epi64(x8_ceiling8, y8_ceiling8);
  const __m128i underflow = _mm_cmplt_epi8(*position_xy, floor_xy);
  const __m128i overflow = _mm_cmpgt_epi8(*position_xy, ceiling_xy);
  const __m128i out = _mm_or_si128(underflow, overflow);
  const __m128i skip_low = _mm_or_si128(skip_r, out);
  const __m128i skip = _mm_or_si128(skip_low, _mm_srli_si128(out, 8));
  StoreLo8(skip_64, skip);
}

template <int idx>
inline void Store(const __m128i position, const __m128i reference_offset,
                  const __m128i mv, int8_t* dst_reference_offset,
                  MotionVector* dst_mv) {
  const ptrdiff_t offset =
      static_cast<int16_t>(_mm_extract_epi16(position, idx));
  if ((idx & 3) == 0) {
    dst_mv[offset].mv32 = static_cast<uint32_t>(_mm_cvtsi128_si32(mv));
  } else {
    dst_mv[offset].mv32 = static_cast<uint32_t>(_mm_extract_epi32(mv, idx & 3));
  }
  dst_reference_offset[offset] = _mm_extract_epi8(reference_offset, idx);
}

template <int idx>
inline void CheckStore(const int8_t* skips, const __m128i position,
                       const __m128i reference_offset, const __m128i mv,
                       int8_t* dst_reference_offset, MotionVector* dst_mv) {
  if (skips[idx] == 0) {
    Store<idx>(position, reference_offset, mv, dst_reference_offset, dst_mv);
  }
}

// 7.9.2.
void MotionFieldProjectionKernel_SSE4_1(
    const ReferenceInfo& reference_info,
    const int reference_to_current_with_sign, const int dst_sign,
    const int y8_start, const int y8_end, const int x8_start, const int x8_end,
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
  const __m128i d_sign = _mm_set1_epi16(dst_sign);

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
  const __m128i skip_reference = LoadLo8(skip_references);
  const __m128i r_offsets = LoadLo8(reference_offsets);
  const __m128i division_table = LoadUnaligned16(projection_divisions);

  int y8 = y8_start;
  do {
    const int y8_floor = (y8 & ~7) - y8;                             // [-7, 0]
    const int y8_ceiling = std::min(y8_end - y8, y8_floor + 8) - 1;  // [0, 7]
    const __m128i y8_floor8 = _mm_set1_epi8(y8_floor);
    const __m128i y8_ceiling8 = _mm_set1_epi8(y8_ceiling);
    int x8;

    for (x8 = adjusted_x8_start; x8 < adjusted_x8_end8; x8 += 8) {
      const __m128i source_reference_type8 =
          LoadLo8(source_reference_types + x8);
      const __m128i skip_r =
          _mm_shuffle_epi8(skip_reference, source_reference_type8);
      int64_t early_skip;
      StoreLo8(&early_skip, skip_r);
      // Early termination #1 if all are skips. Chance is typically ~30-40%.
      if (early_skip == -1) continue;
      int64_t skip_64;
      __m128i r, position_xy, mvs[2];
      GetPosition(division_table, mv, reference_to_current_with_sign, x8_start,
                  x8_end, x8, r_offsets, source_reference_type8, skip_r,
                  y8_floor8, y8_ceiling8, d_sign, 0, &r, &position_xy, &skip_64,
                  mvs);
      // Early termination #2 if all are skips.
      // Chance is typically ~15-25% after Early termination #1.
      if (skip_64 == -1) continue;
      const __m128i p_y = _mm_cvtepi8_epi16(_mm_srli_si128(position_xy, 8));
      const __m128i p_x = _mm_cvtepi8_epi16(position_xy);
      const __m128i p_y_offset = _mm_mullo_epi16(p_y, _mm_set1_epi16(stride));
      const __m128i pos = _mm_add_epi16(p_y_offset, p_x);
      const __m128i position = _mm_add_epi16(pos, _mm_set1_epi16(x8));
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
        const __m128i source_reference_type8 =
            LoadLo8(source_reference_types + x8);
        const __m128i skip_r =
            _mm_shuffle_epi8(skip_reference, source_reference_type8);
        int64_t early_skip;
        StoreLo8(&early_skip, skip_r);
        // Early termination #1 if all are skips.
        if (early_skip != -1) {
          int64_t skip_64;
          __m128i r, position_xy, mvs[2];
          GetPosition(division_table, mv, reference_to_current_with_sign,
                      x8_start, x8_end, x8, r_offsets, source_reference_type8,
                      skip_r, y8_floor8, y8_ceiling8, d_sign, delta, &r,
                      &position_xy, &skip_64, mvs);
          // Early termination #2 if all are skips.
          if (skip_64 != -1) {
            const __m128i p_y =
                _mm_cvtepi8_epi16(_mm_srli_si128(position_xy, 8));
            const __m128i p_x = _mm_cvtepi8_epi16(position_xy);
            const __m128i p_y_offset =
                _mm_mullo_epi16(p_y, _mm_set1_epi16(stride));
            const __m128i pos = _mm_add_epi16(p_y_offset, p_x);
            const __m128i position = _mm_add_epi16(pos, _mm_set1_epi16(x8));
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
          if (position_y8 < y8_floor || position_y8 > y8_ceiling) continue;
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

void MotionFieldProjectionInit_SSE4_1() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
  dsp->motion_field_projection_kernel = MotionFieldProjectionKernel_SSE4_1;
}

}  // namespace dsp
}  // namespace libgav1

#else   // !LIBGAV1_TARGETING_SSE4_1
namespace libgav1 {
namespace dsp {

void MotionFieldProjectionInit_SSE4_1() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_TARGETING_SSE4_1
