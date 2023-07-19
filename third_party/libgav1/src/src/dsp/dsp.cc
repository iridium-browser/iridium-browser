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

#include "src/dsp/dsp.h"

#include <mutex>  // NOLINT (unapproved c++11 header)

#include "src/dsp/average_blend.h"
#include "src/dsp/cdef.h"
#include "src/dsp/convolve.h"
#include "src/dsp/distance_weighted_blend.h"
#include "src/dsp/film_grain.h"
#include "src/dsp/intra_edge.h"
#include "src/dsp/intrapred.h"
#include "src/dsp/intrapred_cfl.h"
#include "src/dsp/intrapred_directional.h"
#include "src/dsp/intrapred_filter.h"
#include "src/dsp/intrapred_smooth.h"
#include "src/dsp/inverse_transform.h"
#include "src/dsp/loop_filter.h"
#include "src/dsp/loop_restoration.h"
#include "src/dsp/mask_blend.h"
#include "src/dsp/motion_field_projection.h"
#include "src/dsp/motion_vector_search.h"
#include "src/dsp/obmc.h"
#include "src/dsp/super_res.h"
#include "src/dsp/warp.h"
#include "src/dsp/weight_mask.h"
#include "src/utils/cpu.h"

namespace libgav1 {
namespace dsp_internal {

void DspInit_C() {
  dsp::AverageBlendInit_C();
  dsp::CdefInit_C();
  dsp::ConvolveInit_C();
  dsp::DistanceWeightedBlendInit_C();
  dsp::FilmGrainInit_C();
  dsp::IntraEdgeInit_C();
  dsp::IntraPredCflInit_C();
  dsp::IntraPredDirectionalInit_C();
  dsp::IntraPredFilterInit_C();
  dsp::IntraPredInit_C();
  dsp::IntraPredSmoothInit_C();
  dsp::InverseTransformInit_C();
  dsp::LoopFilterInit_C();
  dsp::LoopRestorationInit_C();
  dsp::MaskBlendInit_C();
  dsp::MotionFieldProjectionInit_C();
  dsp::MotionVectorSearchInit_C();
  dsp::ObmcInit_C();
  dsp::SuperResInit_C();
  dsp::WarpInit_C();
  dsp::WeightMaskInit_C();
}

dsp::Dsp* GetWritableDspTable(int bitdepth) {
  switch (bitdepth) {
    case 8: {
      static dsp::Dsp dsp_8bpp;
      return &dsp_8bpp;
    }
#if LIBGAV1_MAX_BITDEPTH >= 10
    case 10: {
      static dsp::Dsp dsp_10bpp;
      return &dsp_10bpp;
    }
#endif
#if LIBGAV1_MAX_BITDEPTH == 12
    case 12: {
      static dsp::Dsp dsp_12bpp;
      return &dsp_12bpp;
    }
#endif
  }
  return nullptr;
}

}  // namespace dsp_internal

namespace dsp {

void DspInit() {
  static std::once_flag once;
  std::call_once(once, []() {
    dsp_internal::DspInit_C();
#if LIBGAV1_ENABLE_SSE4_1 || LIBGAV1_ENABLE_AVX2
    const uint32_t cpu_features = GetCpuInfo();
#if LIBGAV1_ENABLE_SSE4_1
    if ((cpu_features & kSSE4_1) != 0) {
      AverageBlendInit_SSE4_1();
      CdefInit_SSE4_1();
      ConvolveInit_SSE4_1();
      DistanceWeightedBlendInit_SSE4_1();
      FilmGrainInit_SSE4_1();
      IntraEdgeInit_SSE4_1();
      IntraPredCflInit_SSE4_1();
      IntraPredDirectionalInit_SSE4_1();
      IntraPredFilterInit_SSE4_1();
      IntraPredInit_SSE4_1();
      IntraPredCflInit_SSE4_1();
      IntraPredSmoothInit_SSE4_1();
      InverseTransformInit_SSE4_1();
      LoopFilterInit_SSE4_1();
      LoopRestorationInit_SSE4_1();
      MaskBlendInit_SSE4_1();
      MotionFieldProjectionInit_SSE4_1();
      MotionVectorSearchInit_SSE4_1();
      ObmcInit_SSE4_1();
      SuperResInit_SSE4_1();
      WarpInit_SSE4_1();
      WeightMaskInit_SSE4_1();
#if LIBGAV1_MAX_BITDEPTH >= 10
      LoopRestorationInit10bpp_SSE4_1();
#endif  // LIBGAV1_MAX_BITDEPTH >= 10
    }
#endif  // LIBGAV1_ENABLE_SSE4_1
#if LIBGAV1_ENABLE_AVX2
    if ((cpu_features & kAVX2) != 0) {
      CdefInit_AVX2();
      ConvolveInit_AVX2();
      LoopRestorationInit_AVX2();
#if LIBGAV1_MAX_BITDEPTH >= 10
      LoopRestorationInit10bpp_AVX2();
#endif  // LIBGAV1_MAX_BITDEPTH >= 10
    }
#endif  // LIBGAV1_ENABLE_AVX2
#endif  // LIBGAV1_ENABLE_SSE4_1 || LIBGAV1_ENABLE_AVX2
#if LIBGAV1_ENABLE_NEON
    AverageBlendInit_NEON();
    CdefInit_NEON();
    ConvolveInit_NEON();
    DistanceWeightedBlendInit_NEON();
    FilmGrainInit_NEON();
    IntraEdgeInit_NEON();
    IntraPredCflInit_NEON();
    IntraPredDirectionalInit_NEON();
    IntraPredFilterInit_NEON();
    IntraPredInit_NEON();
    IntraPredSmoothInit_NEON();
    InverseTransformInit_NEON();
    LoopFilterInit_NEON();
    LoopRestorationInit_NEON();
    MaskBlendInit_NEON();
    MotionFieldProjectionInit_NEON();
    MotionVectorSearchInit_NEON();
    ObmcInit_NEON();
    SuperResInit_NEON();
    WarpInit_NEON();
    WeightMaskInit_NEON();
#if LIBGAV1_MAX_BITDEPTH >= 10
    ConvolveInit10bpp_NEON();
    InverseTransformInit10bpp_NEON();
    LoopFilterInit10bpp_NEON();
    LoopRestorationInit10bpp_NEON();
#endif  // LIBGAV1_MAX_BITDEPTH >= 10
#endif  // LIBGAV1_ENABLE_NEON
  });
}

const Dsp* GetDspTable(int bitdepth) {
  return dsp_internal::GetWritableDspTable(bitdepth);
}

}  // namespace dsp
}  // namespace libgav1
