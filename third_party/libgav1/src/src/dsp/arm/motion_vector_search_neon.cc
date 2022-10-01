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

#include "src/dsp/motion_vector_search.h"
#include "src/utils/cpu.h"

#if LIBGAV1_ENABLE_NEON

#include <arm_neon.h>

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

inline int16x4_t MvProjection(const int16x4_t mv, const int16x4_t denominator,
                              const int32x4_t numerator) {
  const int32x4_t m0 = vmull_s16(mv, denominator);
  const int32x4_t m = vmulq_s32(m0, numerator);
  // Add the sign (0 or -1) to round towards zero.
  const int32x4_t add_sign = vsraq_n_s32(m, m, 31);
  return vqrshrn_n_s32(add_sign, 14);
}

inline int16x4_t MvProjectionCompound(const int16x4_t mv,
                                      const int temporal_reference_offsets,
                                      const int reference_offsets[2]) {
  const int16x4_t denominator =
      vdup_n_s16(kProjectionMvDivisionLookup[temporal_reference_offsets]);
  const int32x2_t offset = vld1_s32(reference_offsets);
  const int32x2x2_t offsets = vzip_s32(offset, offset);
  const int32x4_t numerator = vcombine_s32(offsets.val[0], offsets.val[1]);
  return MvProjection(mv, denominator, numerator);
}

inline int16x8_t ProjectionClip(const int16x4_t mv0, const int16x4_t mv1) {
  const int16x8_t projection_mv_clamp = vdupq_n_s16(kProjectionMvClamp);
  const int16x8_t mv = vcombine_s16(mv0, mv1);
  const int16x8_t clamp = vminq_s16(mv, projection_mv_clamp);
  return vmaxq_s16(clamp, vnegq_s16(projection_mv_clamp));
}

inline int16x8_t MvProjectionCompoundClip(
    const MotionVector* LIBGAV1_RESTRICT const temporal_mvs,
    const int8_t* LIBGAV1_RESTRICT const temporal_reference_offsets,
    const int reference_offsets[2]) {
  const auto* const tmvs = reinterpret_cast<const int32_t*>(temporal_mvs);
  const int32x2_t temporal_mv = vld1_s32(tmvs);
  const int16x4_t tmv0 = vreinterpret_s16_s32(vdup_lane_s32(temporal_mv, 0));
  const int16x4_t tmv1 = vreinterpret_s16_s32(vdup_lane_s32(temporal_mv, 1));
  const int16x4_t mv0 = MvProjectionCompound(
      tmv0, temporal_reference_offsets[0], reference_offsets);
  const int16x4_t mv1 = MvProjectionCompound(
      tmv1, temporal_reference_offsets[1], reference_offsets);
  return ProjectionClip(mv0, mv1);
}

inline int16x8_t MvProjectionSingleClip(
    const MotionVector* LIBGAV1_RESTRICT const temporal_mvs,
    const int8_t* LIBGAV1_RESTRICT const temporal_reference_offsets,
    const int reference_offset, int16x4_t* const lookup) {
  const auto* const tmvs = reinterpret_cast<const int16_t*>(temporal_mvs);
  const int16x8_t temporal_mv = vld1q_s16(tmvs);
  *lookup = vld1_lane_s16(
      &kProjectionMvDivisionLookup[temporal_reference_offsets[0]], *lookup, 0);
  *lookup = vld1_lane_s16(
      &kProjectionMvDivisionLookup[temporal_reference_offsets[1]], *lookup, 1);
  *lookup = vld1_lane_s16(
      &kProjectionMvDivisionLookup[temporal_reference_offsets[2]], *lookup, 2);
  *lookup = vld1_lane_s16(
      &kProjectionMvDivisionLookup[temporal_reference_offsets[3]], *lookup, 3);
  const int16x4x2_t denominator = vzip_s16(*lookup, *lookup);
  const int16x4_t tmv0 = vget_low_s16(temporal_mv);
  const int16x4_t tmv1 = vget_high_s16(temporal_mv);
  const int32x4_t numerator = vdupq_n_s32(reference_offset);
  const int16x4_t mv0 = MvProjection(tmv0, denominator.val[0], numerator);
  const int16x4_t mv1 = MvProjection(tmv1, denominator.val[1], numerator);
  return ProjectionClip(mv0, mv1);
}

inline void LowPrecision(const int16x8_t mv, void* const candidate_mvs) {
  const int16x8_t kRoundDownMask = vdupq_n_s16(1);
  const uint16x8_t mvu = vreinterpretq_u16_s16(mv);
  const int16x8_t mv0 = vreinterpretq_s16_u16(vsraq_n_u16(mvu, mvu, 15));
  const int16x8_t mv1 = vbicq_s16(mv0, kRoundDownMask);
  vst1q_s16(static_cast<int16_t*>(candidate_mvs), mv1);
}

inline void ForceInteger(const int16x8_t mv, void* const candidate_mvs) {
  const int16x8_t kRoundDownMask = vdupq_n_s16(7);
  const uint16x8_t mvu = vreinterpretq_u16_s16(mv);
  const int16x8_t mv0 = vreinterpretq_s16_u16(vsraq_n_u16(mvu, mvu, 15));
  const int16x8_t mv1 = vaddq_s16(mv0, vdupq_n_s16(3));
  const int16x8_t mv2 = vbicq_s16(mv1, kRoundDownMask);
  vst1q_s16(static_cast<int16_t*>(candidate_mvs), mv2);
}

void MvProjectionCompoundLowPrecision_NEON(
    const MotionVector* LIBGAV1_RESTRICT temporal_mvs,
    const int8_t* LIBGAV1_RESTRICT temporal_reference_offsets,
    const int reference_offsets[2], const int count,
    CompoundMotionVector* LIBGAV1_RESTRICT candidate_mvs) {
  // |reference_offsets| non-zero check usually equals true and is ignored.
  // To facilitate the compilers, make a local copy of |reference_offsets|.
  const int offsets[2] = {reference_offsets[0], reference_offsets[1]};
  // One more element could be calculated.
  int loop_count = (count + 1) >> 1;
  do {
    const int16x8_t mv = MvProjectionCompoundClip(
        temporal_mvs, temporal_reference_offsets, offsets);
    LowPrecision(mv, candidate_mvs);
    temporal_mvs += 2;
    temporal_reference_offsets += 2;
    candidate_mvs += 2;
  } while (--loop_count != 0);
}

void MvProjectionCompoundForceInteger_NEON(
    const MotionVector* LIBGAV1_RESTRICT temporal_mvs,
    const int8_t* LIBGAV1_RESTRICT temporal_reference_offsets,
    const int reference_offsets[2], const int count,
    CompoundMotionVector* LIBGAV1_RESTRICT candidate_mvs) {
  // |reference_offsets| non-zero check usually equals true and is ignored.
  // To facilitate the compilers, make a local copy of |reference_offsets|.
  const int offsets[2] = {reference_offsets[0], reference_offsets[1]};
  // One more element could be calculated.
  int loop_count = (count + 1) >> 1;
  do {
    const int16x8_t mv = MvProjectionCompoundClip(
        temporal_mvs, temporal_reference_offsets, offsets);
    ForceInteger(mv, candidate_mvs);
    temporal_mvs += 2;
    temporal_reference_offsets += 2;
    candidate_mvs += 2;
  } while (--loop_count != 0);
}

void MvProjectionCompoundHighPrecision_NEON(
    const MotionVector* LIBGAV1_RESTRICT temporal_mvs,
    const int8_t* LIBGAV1_RESTRICT temporal_reference_offsets,
    const int reference_offsets[2], const int count,
    CompoundMotionVector* LIBGAV1_RESTRICT candidate_mvs) {
  // |reference_offsets| non-zero check usually equals true and is ignored.
  // To facilitate the compilers, make a local copy of |reference_offsets|.
  const int offsets[2] = {reference_offsets[0], reference_offsets[1]};
  // One more element could be calculated.
  int loop_count = (count + 1) >> 1;
  do {
    const int16x8_t mv = MvProjectionCompoundClip(
        temporal_mvs, temporal_reference_offsets, offsets);
    vst1q_s16(reinterpret_cast<int16_t*>(candidate_mvs), mv);
    temporal_mvs += 2;
    temporal_reference_offsets += 2;
    candidate_mvs += 2;
  } while (--loop_count != 0);
}

void MvProjectionSingleLowPrecision_NEON(
    const MotionVector* LIBGAV1_RESTRICT temporal_mvs,
    const int8_t* LIBGAV1_RESTRICT temporal_reference_offsets,
    const int reference_offset, const int count,
    MotionVector* LIBGAV1_RESTRICT candidate_mvs) {
  // Up to three more elements could be calculated.
  int loop_count = (count + 3) >> 2;
  int16x4_t lookup = vdup_n_s16(0);
  do {
    const int16x8_t mv = MvProjectionSingleClip(
        temporal_mvs, temporal_reference_offsets, reference_offset, &lookup);
    LowPrecision(mv, candidate_mvs);
    temporal_mvs += 4;
    temporal_reference_offsets += 4;
    candidate_mvs += 4;
  } while (--loop_count != 0);
}

void MvProjectionSingleForceInteger_NEON(
    const MotionVector* LIBGAV1_RESTRICT temporal_mvs,
    const int8_t* LIBGAV1_RESTRICT temporal_reference_offsets,
    const int reference_offset, const int count,
    MotionVector* LIBGAV1_RESTRICT candidate_mvs) {
  // Up to three more elements could be calculated.
  int loop_count = (count + 3) >> 2;
  int16x4_t lookup = vdup_n_s16(0);
  do {
    const int16x8_t mv = MvProjectionSingleClip(
        temporal_mvs, temporal_reference_offsets, reference_offset, &lookup);
    ForceInteger(mv, candidate_mvs);
    temporal_mvs += 4;
    temporal_reference_offsets += 4;
    candidate_mvs += 4;
  } while (--loop_count != 0);
}

void MvProjectionSingleHighPrecision_NEON(
    const MotionVector* LIBGAV1_RESTRICT temporal_mvs,
    const int8_t* LIBGAV1_RESTRICT temporal_reference_offsets,
    const int reference_offset, const int count,
    MotionVector* LIBGAV1_RESTRICT candidate_mvs) {
  // Up to three more elements could be calculated.
  int loop_count = (count + 3) >> 2;
  int16x4_t lookup = vdup_n_s16(0);
  do {
    const int16x8_t mv = MvProjectionSingleClip(
        temporal_mvs, temporal_reference_offsets, reference_offset, &lookup);
    vst1q_s16(reinterpret_cast<int16_t*>(candidate_mvs), mv);
    temporal_mvs += 4;
    temporal_reference_offsets += 4;
    candidate_mvs += 4;
  } while (--loop_count != 0);
}

}  // namespace

void MotionVectorSearchInit_NEON() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
  dsp->mv_projection_compound[0] = MvProjectionCompoundLowPrecision_NEON;
  dsp->mv_projection_compound[1] = MvProjectionCompoundForceInteger_NEON;
  dsp->mv_projection_compound[2] = MvProjectionCompoundHighPrecision_NEON;
  dsp->mv_projection_single[0] = MvProjectionSingleLowPrecision_NEON;
  dsp->mv_projection_single[1] = MvProjectionSingleForceInteger_NEON;
  dsp->mv_projection_single[2] = MvProjectionSingleHighPrecision_NEON;
}

}  // namespace dsp
}  // namespace libgav1

#else   // !LIBGAV1_ENABLE_NEON
namespace libgav1 {
namespace dsp {

void MotionVectorSearchInit_NEON() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_ENABLE_NEON
