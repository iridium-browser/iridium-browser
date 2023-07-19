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

#if LIBGAV1_TARGETING_SSE4_1

#include <smmintrin.h>

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

constexpr int kProjectionMvDivisionLookup_32bit[kMaxFrameDistance + 1] = {
    0,    16384, 8192, 5461, 4096, 3276, 2730, 2340, 2048, 1820, 1638,
    1489, 1365,  1260, 1170, 1092, 1024, 963,  910,  862,  819,  780,
    744,  712,   682,  655,  630,  606,  585,  564,  546,  528};

inline __m128i MvProjection(const __m128i mv, const __m128i denominator,
                            const __m128i numerator) {
  const __m128i m0 = _mm_madd_epi16(mv, denominator);
  const __m128i m = _mm_mullo_epi32(m0, numerator);
  // Add the sign (0 or -1) to round towards zero.
  const __m128i sign = _mm_srai_epi32(m, 31);
  const __m128i add_sign = _mm_add_epi32(m, sign);
  const __m128i sum = _mm_add_epi32(add_sign, _mm_set1_epi32(1 << 13));
  return _mm_srai_epi32(sum, 14);
}

inline __m128i MvProjectionClip(const __m128i mvs[2],
                                const __m128i denominators[2],
                                const __m128i numerator) {
  const __m128i s0 = MvProjection(mvs[0], denominators[0], numerator);
  const __m128i s1 = MvProjection(mvs[1], denominators[1], numerator);
  const __m128i mv = _mm_packs_epi32(s0, s1);
  const __m128i projection_mv_clamp = _mm_set1_epi16(kProjectionMvClamp);
  const __m128i projection_mv_clamp_negative =
      _mm_set1_epi16(-kProjectionMvClamp);
  const __m128i clamp = _mm_min_epi16(mv, projection_mv_clamp);
  return _mm_max_epi16(clamp, projection_mv_clamp_negative);
}

inline __m128i MvProjectionCompoundClip(
    const MotionVector* LIBGAV1_RESTRICT const temporal_mvs,
    const int8_t temporal_reference_offsets[2],
    const int reference_offsets[2]) {
  const auto* const tmvs = reinterpret_cast<const int32_t*>(temporal_mvs);
  const __m128i temporal_mv = LoadLo8(tmvs);
  const __m128i temporal_mv_0 = _mm_cvtepu16_epi32(temporal_mv);
  __m128i mvs[2], denominators[2];
  mvs[0] = _mm_unpacklo_epi64(temporal_mv_0, temporal_mv_0);
  mvs[1] = _mm_unpackhi_epi64(temporal_mv_0, temporal_mv_0);
  denominators[0] = _mm_set1_epi32(
      kProjectionMvDivisionLookup[temporal_reference_offsets[0]]);
  denominators[1] = _mm_set1_epi32(
      kProjectionMvDivisionLookup[temporal_reference_offsets[1]]);
  const __m128i offsets = LoadLo8(reference_offsets);
  const __m128i numerator = _mm_unpacklo_epi32(offsets, offsets);
  return MvProjectionClip(mvs, denominators, numerator);
}

inline __m128i MvProjectionSingleClip(
    const MotionVector* LIBGAV1_RESTRICT const temporal_mvs,
    const int8_t* LIBGAV1_RESTRICT const temporal_reference_offsets,
    const int reference_offset) {
  const auto* const tmvs = reinterpret_cast<const int16_t*>(temporal_mvs);
  const __m128i temporal_mv = LoadAligned16(tmvs);
  __m128i lookup = _mm_cvtsi32_si128(
      kProjectionMvDivisionLookup_32bit[temporal_reference_offsets[0]]);
  lookup = _mm_insert_epi32(
      lookup, kProjectionMvDivisionLookup_32bit[temporal_reference_offsets[1]],
      1);
  lookup = _mm_insert_epi32(
      lookup, kProjectionMvDivisionLookup_32bit[temporal_reference_offsets[2]],
      2);
  lookup = _mm_insert_epi32(
      lookup, kProjectionMvDivisionLookup_32bit[temporal_reference_offsets[3]],
      3);
  __m128i mvs[2], denominators[2];
  mvs[0] = _mm_unpacklo_epi16(temporal_mv, _mm_setzero_si128());
  mvs[1] = _mm_unpackhi_epi16(temporal_mv, _mm_setzero_si128());
  denominators[0] = _mm_unpacklo_epi32(lookup, lookup);
  denominators[1] = _mm_unpackhi_epi32(lookup, lookup);
  const __m128i numerator = _mm_set1_epi32(reference_offset);
  return MvProjectionClip(mvs, denominators, numerator);
}

inline void LowPrecision(const __m128i mv, void* const candidate_mvs) {
  const __m128i kRoundDownMask = _mm_set1_epi16(~1);
  const __m128i sign = _mm_srai_epi16(mv, 15);
  const __m128i sub_sign = _mm_sub_epi16(mv, sign);
  const __m128i d = _mm_and_si128(sub_sign, kRoundDownMask);
  StoreAligned16(candidate_mvs, d);
}

inline void ForceInteger(const __m128i mv, void* const candidate_mvs) {
  const __m128i kRoundDownMask = _mm_set1_epi16(~7);
  const __m128i sign = _mm_srai_epi16(mv, 15);
  const __m128i mv1 = _mm_add_epi16(mv, _mm_set1_epi16(3));
  const __m128i mv2 = _mm_sub_epi16(mv1, sign);
  const __m128i mv3 = _mm_and_si128(mv2, kRoundDownMask);
  StoreAligned16(candidate_mvs, mv3);
}

void MvProjectionCompoundLowPrecision_SSE4_1(
    const MotionVector* LIBGAV1_RESTRICT temporal_mvs,
    const int8_t* LIBGAV1_RESTRICT temporal_reference_offsets,
    const int reference_offsets[2], const int count,
    CompoundMotionVector* LIBGAV1_RESTRICT candidate_mvs) {
  // |reference_offsets| non-zero check usually equals true and is ignored.
  // To facilitate the compilers, make a local copy of |reference_offsets|.
  const int offsets[2] = {reference_offsets[0], reference_offsets[1]};
  // One more element could be calculated.
  int i = 0;
  do {
    const __m128i mv = MvProjectionCompoundClip(
        temporal_mvs + i, temporal_reference_offsets + i, offsets);
    LowPrecision(mv, candidate_mvs + i);
    i += 2;
  } while (i < count);
}

void MvProjectionCompoundForceInteger_SSE4_1(
    const MotionVector* LIBGAV1_RESTRICT temporal_mvs,
    const int8_t* LIBGAV1_RESTRICT temporal_reference_offsets,
    const int reference_offsets[2], const int count,
    CompoundMotionVector* LIBGAV1_RESTRICT candidate_mvs) {
  // |reference_offsets| non-zero check usually equals true and is ignored.
  // To facilitate the compilers, make a local copy of |reference_offsets|.
  const int offsets[2] = {reference_offsets[0], reference_offsets[1]};
  // One more element could be calculated.
  int i = 0;
  do {
    const __m128i mv = MvProjectionCompoundClip(
        temporal_mvs + i, temporal_reference_offsets + i, offsets);
    ForceInteger(mv, candidate_mvs + i);
    i += 2;
  } while (i < count);
}

void MvProjectionCompoundHighPrecision_SSE4_1(
    const MotionVector* LIBGAV1_RESTRICT temporal_mvs,
    const int8_t* LIBGAV1_RESTRICT temporal_reference_offsets,
    const int reference_offsets[2], const int count,
    CompoundMotionVector* LIBGAV1_RESTRICT candidate_mvs) {
  // |reference_offsets| non-zero check usually equals true and is ignored.
  // To facilitate the compilers, make a local copy of |reference_offsets|.
  const int offsets[2] = {reference_offsets[0], reference_offsets[1]};
  // One more element could be calculated.
  int i = 0;
  do {
    const __m128i mv = MvProjectionCompoundClip(
        temporal_mvs + i, temporal_reference_offsets + i, offsets);
    StoreAligned16(candidate_mvs + i, mv);
    i += 2;
  } while (i < count);
}

void MvProjectionSingleLowPrecision_SSE4_1(
    const MotionVector* LIBGAV1_RESTRICT temporal_mvs,
    const int8_t* LIBGAV1_RESTRICT temporal_reference_offsets,
    const int reference_offset, const int count,
    MotionVector* LIBGAV1_RESTRICT candidate_mvs) {
  // Up to three more elements could be calculated.
  int i = 0;
  do {
    const __m128i mv = MvProjectionSingleClip(
        temporal_mvs + i, temporal_reference_offsets + i, reference_offset);
    LowPrecision(mv, candidate_mvs + i);
    i += 4;
  } while (i < count);
}

void MvProjectionSingleForceInteger_SSE4_1(
    const MotionVector* LIBGAV1_RESTRICT temporal_mvs,
    const int8_t* LIBGAV1_RESTRICT temporal_reference_offsets,
    const int reference_offset, const int count,
    MotionVector* LIBGAV1_RESTRICT candidate_mvs) {
  // Up to three more elements could be calculated.
  int i = 0;
  do {
    const __m128i mv = MvProjectionSingleClip(
        temporal_mvs + i, temporal_reference_offsets + i, reference_offset);
    ForceInteger(mv, candidate_mvs + i);
    i += 4;
  } while (i < count);
}

void MvProjectionSingleHighPrecision_SSE4_1(
    const MotionVector* LIBGAV1_RESTRICT temporal_mvs,
    const int8_t* LIBGAV1_RESTRICT temporal_reference_offsets,
    const int reference_offset, const int count,
    MotionVector* LIBGAV1_RESTRICT candidate_mvs) {
  // Up to three more elements could be calculated.
  int i = 0;
  do {
    const __m128i mv = MvProjectionSingleClip(
        temporal_mvs + i, temporal_reference_offsets + i, reference_offset);
    StoreAligned16(candidate_mvs + i, mv);
    i += 4;
  } while (i < count);
}

}  // namespace

void MotionVectorSearchInit_SSE4_1() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
  dsp->mv_projection_compound[0] = MvProjectionCompoundLowPrecision_SSE4_1;
  dsp->mv_projection_compound[1] = MvProjectionCompoundForceInteger_SSE4_1;
  dsp->mv_projection_compound[2] = MvProjectionCompoundHighPrecision_SSE4_1;
  dsp->mv_projection_single[0] = MvProjectionSingleLowPrecision_SSE4_1;
  dsp->mv_projection_single[1] = MvProjectionSingleForceInteger_SSE4_1;
  dsp->mv_projection_single[2] = MvProjectionSingleHighPrecision_SSE4_1;
}

}  // namespace dsp
}  // namespace libgav1

#else   // !LIBGAV1_TARGETING_SSE4_1
namespace libgav1 {
namespace dsp {

void MotionVectorSearchInit_SSE4_1() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_TARGETING_SSE4_1
