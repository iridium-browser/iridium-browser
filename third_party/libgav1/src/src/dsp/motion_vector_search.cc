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

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "src/dsp/dsp.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/types.h"

namespace libgav1 {
namespace dsp {
namespace {

// Silence unused function warnings when the C functions are not used.
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS || \
    !defined(LIBGAV1_Dsp8bpp_MotionVectorSearch)

void MvProjectionCompoundLowPrecision_C(
    const MotionVector* LIBGAV1_RESTRICT const temporal_mvs,
    const int8_t* LIBGAV1_RESTRICT const temporal_reference_offsets,
    const int reference_offsets[2], const int count,
    CompoundMotionVector* LIBGAV1_RESTRICT const candidate_mvs) {
  // To facilitate the compilers, make a local copy of |reference_offsets|.
  const int offsets[2] = {reference_offsets[0], reference_offsets[1]};
  int index = 0;
  do {
    candidate_mvs[index].mv64 = 0;
    for (int i = 0; i < 2; ++i) {
      // |offsets| non-zero check usually equals true and could be ignored.
      if (offsets[i] != 0) {
        GetMvProjection(
            temporal_mvs[index], offsets[i],
            kProjectionMvDivisionLookup[temporal_reference_offsets[index]],
            &candidate_mvs[index].mv[i]);
        for (auto& mv : candidate_mvs[index].mv[i].mv) {
          // The next line is equivalent to:
          // if ((mv & 1) != 0) mv += (mv > 0) ? -1 : 1;
          mv = (mv - (mv >> 15)) & ~1;
        }
      }
    }
  } while (++index < count);
}

void MvProjectionCompoundForceInteger_C(
    const MotionVector* LIBGAV1_RESTRICT const temporal_mvs,
    const int8_t* LIBGAV1_RESTRICT const temporal_reference_offsets,
    const int reference_offsets[2], const int count,
    CompoundMotionVector* LIBGAV1_RESTRICT const candidate_mvs) {
  // To facilitate the compilers, make a local copy of |reference_offsets|.
  const int offsets[2] = {reference_offsets[0], reference_offsets[1]};
  int index = 0;
  do {
    candidate_mvs[index].mv64 = 0;
    for (int i = 0; i < 2; ++i) {
      // |offsets| non-zero check usually equals true and could be ignored.
      if (offsets[i] != 0) {
        GetMvProjection(
            temporal_mvs[index], offsets[i],
            kProjectionMvDivisionLookup[temporal_reference_offsets[index]],
            &candidate_mvs[index].mv[i]);
        for (auto& mv : candidate_mvs[index].mv[i].mv) {
          // The next line is equivalent to:
          // const int value = (std::abs(static_cast<int>(mv)) + 3) & ~7;
          // const int sign = mv >> 15;
          // mv = ApplySign(value, sign);
          mv = (mv + 3 - (mv >> 15)) & ~7;
        }
      }
    }
  } while (++index < count);
}

void MvProjectionCompoundHighPrecision_C(
    const MotionVector* LIBGAV1_RESTRICT const temporal_mvs,
    const int8_t* LIBGAV1_RESTRICT const temporal_reference_offsets,
    const int reference_offsets[2], const int count,
    CompoundMotionVector* LIBGAV1_RESTRICT const candidate_mvs) {
  // To facilitate the compilers, make a local copy of |reference_offsets|.
  const int offsets[2] = {reference_offsets[0], reference_offsets[1]};
  int index = 0;
  do {
    candidate_mvs[index].mv64 = 0;
    for (int i = 0; i < 2; ++i) {
      // |offsets| non-zero check usually equals true and could be ignored.
      if (offsets[i] != 0) {
        GetMvProjection(
            temporal_mvs[index], offsets[i],
            kProjectionMvDivisionLookup[temporal_reference_offsets[index]],
            &candidate_mvs[index].mv[i]);
      }
    }
  } while (++index < count);
}

void MvProjectionSingleLowPrecision_C(
    const MotionVector* LIBGAV1_RESTRICT const temporal_mvs,
    const int8_t* LIBGAV1_RESTRICT const temporal_reference_offsets,
    const int reference_offset, const int count,
    MotionVector* LIBGAV1_RESTRICT const candidate_mvs) {
  int index = 0;
  do {
    GetMvProjection(
        temporal_mvs[index], reference_offset,
        kProjectionMvDivisionLookup[temporal_reference_offsets[index]],
        &candidate_mvs[index]);
    for (auto& mv : candidate_mvs[index].mv) {
      // The next line is equivalent to:
      // if ((mv & 1) != 0) mv += (mv > 0) ? -1 : 1;
      mv = (mv - (mv >> 15)) & ~1;
    }
  } while (++index < count);
}

void MvProjectionSingleForceInteger_C(
    const MotionVector* LIBGAV1_RESTRICT const temporal_mvs,
    const int8_t* LIBGAV1_RESTRICT const temporal_reference_offsets,
    const int reference_offset, const int count,
    MotionVector* LIBGAV1_RESTRICT const candidate_mvs) {
  int index = 0;
  do {
    GetMvProjection(
        temporal_mvs[index], reference_offset,
        kProjectionMvDivisionLookup[temporal_reference_offsets[index]],
        &candidate_mvs[index]);
    for (auto& mv : candidate_mvs[index].mv) {
      // The next line is equivalent to:
      // const int value = (std::abs(static_cast<int>(mv)) + 3) & ~7;
      // const int sign = mv >> 15;
      // mv = ApplySign(value, sign);
      mv = (mv + 3 - (mv >> 15)) & ~7;
    }
  } while (++index < count);
}

void MvProjectionSingleHighPrecision_C(
    const MotionVector* LIBGAV1_RESTRICT const temporal_mvs,
    const int8_t* LIBGAV1_RESTRICT const temporal_reference_offsets,
    const int reference_offset, const int count,
    MotionVector* LIBGAV1_RESTRICT const candidate_mvs) {
  int index = 0;
  do {
    GetMvProjection(
        temporal_mvs[index], reference_offset,
        kProjectionMvDivisionLookup[temporal_reference_offsets[index]],
        &candidate_mvs[index]);
  } while (++index < count);
}

#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS ||
        // !defined(LIBGAV1_Dsp8bpp_MotionVectorSearch)

}  // namespace

void MotionVectorSearchInit_C() {
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS || \
    !defined(LIBGAV1_Dsp8bpp_MotionVectorSearch)
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
  dsp->mv_projection_compound[0] = MvProjectionCompoundLowPrecision_C;
  dsp->mv_projection_compound[1] = MvProjectionCompoundForceInteger_C;
  dsp->mv_projection_compound[2] = MvProjectionCompoundHighPrecision_C;
  dsp->mv_projection_single[0] = MvProjectionSingleLowPrecision_C;
  dsp->mv_projection_single[1] = MvProjectionSingleForceInteger_C;
  dsp->mv_projection_single[2] = MvProjectionSingleHighPrecision_C;
#endif
}

}  // namespace dsp
}  // namespace libgav1
