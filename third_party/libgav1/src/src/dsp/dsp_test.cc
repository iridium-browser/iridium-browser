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

#include "src/dsp/dsp.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "absl/strings/str_cat.h"
#include "gtest/gtest.h"
#include "src/dsp/constants.h"
#include "src/utils/constants.h"
#include "src/utils/cpu.h"

#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
#include "tests/utils.h"
#endif

namespace libgav1 {
namespace dsp {
namespace {

// Maps 1D transform to the maximum valid size for the corresponding transform.
constexpr int kMaxTransform1dSize[kNumTransform1ds] = {
    kTransform1dSize64,  // Dct.
    kTransform1dSize16,  // Adst.
    kTransform1dSize32,  // Identity.
    kTransform1dSize4,   // Wht.
};

void CheckTables(bool c_only) {
#if LIBGAV1_MAX_BITDEPTH == 12
  static constexpr int kBitdepths[] = {kBitdepth8, kBitdepth10, kBitdepth12};
#elif LIBGAV1_MAX_BITDEPTH >= 10
  static constexpr int kBitdepths[] = {kBitdepth8, kBitdepth10};
#else
  static constexpr int kBitdepths[] = {kBitdepth8};
#endif

  for (const auto& bitdepth : kBitdepths) {
    const Dsp* const dsp = GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);
    SCOPED_TRACE(absl::StrCat("bitdepth: ", bitdepth));
    for (int i = 0; i < kNumTransformSizes; ++i) {
      for (int j = 0; j < kNumIntraPredictors; ++j) {
        EXPECT_NE(dsp->intra_predictors[i][j], nullptr)
            << "index [" << i << "][" << j << "]";
      }
    }
    EXPECT_NE(dsp->directional_intra_predictor_zone1, nullptr);
    EXPECT_NE(dsp->directional_intra_predictor_zone2, nullptr);
    EXPECT_NE(dsp->directional_intra_predictor_zone3, nullptr);
    EXPECT_NE(dsp->filter_intra_predictor, nullptr);
    for (int i = 0; i < kNumTransformSizes; ++i) {
      if (std::max(kTransformWidth[i], kTransformHeight[i]) == 64) {
        EXPECT_EQ(dsp->cfl_intra_predictors[i], nullptr)
            << "index [" << i << "]";
        for (int j = 0; j < kNumSubsamplingTypes; ++j) {
          EXPECT_EQ(dsp->cfl_subsamplers[i][j], nullptr)
              << "index [" << i << "][" << j << "]";
        }
      } else {
        EXPECT_NE(dsp->cfl_intra_predictors[i], nullptr)
            << "index [" << i << "]";
        for (int j = 0; j < kNumSubsamplingTypes; ++j) {
          EXPECT_NE(dsp->cfl_subsamplers[i][j], nullptr)
              << "index [" << i << "][" << j << "]";
        }
      }
    }
    EXPECT_NE(dsp->intra_edge_filter, nullptr);
    EXPECT_NE(dsp->intra_edge_upsampler, nullptr);
    for (int i = 0; i < kNumTransform1ds; ++i) {
      for (int j = 0; j < kNumTransform1dSizes; ++j) {
        for (int k = 0; k < 2; ++k) {
          if (j <= kMaxTransform1dSize[i]) {
            EXPECT_NE(dsp->inverse_transforms[i][j][k], nullptr)
                << "index [" << i << "][" << j << "][" << k << "]";
          } else {
            EXPECT_EQ(dsp->inverse_transforms[i][j][k], nullptr)
                << "index [" << i << "][" << j << "][" << k << "]";
          }
        }
      }
    }
    for (int i = 0; i < kNumLoopFilterSizes; ++i) {
      for (int j = 0; j < kNumLoopFilterTypes; ++j) {
        EXPECT_NE(dsp->loop_filters[i][j], nullptr)
            << "index [" << i << "][" << j << "]";
      }
    }
    for (int i = 0; i < 2; ++i) {
      EXPECT_NE(dsp->loop_restorations[i], nullptr) << "index [" << i << "]";
    }

    bool super_res_coefficients_is_nonnull = LIBGAV1_ENABLE_NEON;
#if LIBGAV1_ENABLE_SSE4_1
    const uint32_t cpu_features = GetCpuInfo();
    super_res_coefficients_is_nonnull = (cpu_features & kSSE4_1) != 0;
#endif
    if (c_only || bitdepth == kBitdepth12) {
      super_res_coefficients_is_nonnull = false;
    }
    if (super_res_coefficients_is_nonnull) {
      EXPECT_NE(dsp->super_res_coefficients, nullptr);
    } else {
      EXPECT_EQ(dsp->super_res_coefficients, nullptr);
    }

    EXPECT_NE(dsp->super_res, nullptr);
    EXPECT_NE(dsp->cdef_direction, nullptr);
    for (int i = 0; i < 2; ++i) {
      for (int j = 0; j < 3; ++j) {
        EXPECT_NE(dsp->cdef_filters[i][j], nullptr)
            << "index [" << i << "][" << j << "]";
      }
    }
    for (auto convolve_func : dsp->convolve_scale) {
      EXPECT_NE(convolve_func, nullptr);
    }
    for (int j = 0; j < 2; ++j) {
      for (int k = 0; k < 2; ++k) {
        for (int l = 0; l < 2; ++l) {
          for (int m = 0; m < 2; ++m) {
            if (j == 1 && k == 1) {
              EXPECT_EQ(dsp->convolve[j][k][l][m], nullptr);
            } else {
              EXPECT_NE(dsp->convolve[j][k][l][m], nullptr);
            }
          }
        }
      }
    }
    for (const auto& m : dsp->mask_blend) {
      for (int i = 0; i < 2; ++i) {
        if (i == 0 || bitdepth >= 10) {
          EXPECT_NE(m[i], nullptr);
        } else {
          EXPECT_EQ(m[i], nullptr);
        }
      }
    }
    for (const auto& m : dsp->inter_intra_mask_blend_8bpp) {
      if (bitdepth == 8) {
        EXPECT_NE(m, nullptr);
      } else {
        EXPECT_EQ(m, nullptr);
      }
    }
    for (int i = kBlock4x4; i < kMaxBlockSizes; ++i) {
      const int width_index = k4x4WidthLog2[i] - 1;
      const int height_index = k4x4HeightLog2[i] - 1;
      // Only block sizes >= 8x8 are handled with this function.
      if (width_index < 0 || height_index < 0) continue;

      for (size_t j = 0; j < 2; ++j) {
        EXPECT_NE(dsp->weight_mask[width_index][height_index][j], nullptr)
            << ToString(static_cast<BlockSize>(i)) << " index [" << width_index
            << "]"
            << "[" << height_index << "][" << j << "]";
      }
    }

    EXPECT_NE(dsp->average_blend, nullptr);
    EXPECT_NE(dsp->distance_weighted_blend, nullptr);
    for (int i = 0; i < kNumObmcDirections; ++i) {
      EXPECT_NE(dsp->obmc_blend[i], nullptr)
          << "index [" << ToString(static_cast<ObmcDirection>(i)) << "]";
    }
    EXPECT_NE(dsp->warp, nullptr);
    EXPECT_NE(dsp->warp_compound, nullptr);

    for (int i = 0; i < kNumAutoRegressionLags - 1; ++i) {
      EXPECT_NE(dsp->film_grain.luma_auto_regression[i], nullptr)
          << "index [" << i << "]";
    }
    for (int i = 0; i < 2; ++i) {
      for (int j = 0; j < kNumAutoRegressionLags; ++j) {
        if (i == 0 && j == 0) {
          EXPECT_EQ(dsp->film_grain.chroma_auto_regression[i][j], nullptr)
              << " index [" << i << "]"
              << "[" << j << "]";
        } else {
          EXPECT_NE(dsp->film_grain.chroma_auto_regression[i][j], nullptr)
              << " index [" << i << "]"
              << "[" << j << "]";
        }
      }
      EXPECT_NE(dsp->film_grain.construct_noise_stripes[i], nullptr)
          << "index [" << i << "]";
      EXPECT_NE(dsp->film_grain.blend_noise_chroma[i], nullptr)
          << "index [" << i << "]";
    }
    EXPECT_NE(dsp->film_grain.construct_noise_image_overlap, nullptr);
    EXPECT_NE(dsp->film_grain.initialize_scaling_lut, nullptr);
    EXPECT_NE(dsp->film_grain.blend_noise_luma, nullptr);

    if (bitdepth == 8) {
      EXPECT_NE(dsp->motion_field_projection_kernel, nullptr);
      EXPECT_NE(dsp->mv_projection_compound[0], nullptr);
      EXPECT_NE(dsp->mv_projection_compound[1], nullptr);
      EXPECT_NE(dsp->mv_projection_compound[2], nullptr);
      EXPECT_NE(dsp->mv_projection_single[0], nullptr);
      EXPECT_NE(dsp->mv_projection_single[1], nullptr);
      EXPECT_NE(dsp->mv_projection_single[2], nullptr);
    } else {
      EXPECT_EQ(dsp->motion_field_projection_kernel, nullptr);
      EXPECT_EQ(dsp->mv_projection_compound[0], nullptr);
      EXPECT_EQ(dsp->mv_projection_compound[1], nullptr);
      EXPECT_EQ(dsp->mv_projection_compound[2], nullptr);
      EXPECT_EQ(dsp->mv_projection_single[0], nullptr);
      EXPECT_EQ(dsp->mv_projection_single[1], nullptr);
      EXPECT_EQ(dsp->mv_projection_single[2], nullptr);
    }
  }
}

TEST(Dsp, TablesArePopulated) {
  DspInit();
  CheckTables(/*c_only=*/false);
}

#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
TEST(Dsp, TablesArePopulatedCOnly) {
  test_utils::ResetDspTable(kBitdepth8);
#if LIBGAV1_MAX_BITDEPTH >= 10
  test_utils::ResetDspTable(kBitdepth10);
#endif
#if LIBGAV1_MAX_BITDEPTH == 12
  test_utils::ResetDspTable(kBitdepth12);
#endif
  dsp_internal::DspInit_C();
  CheckTables(/*c_only=*/true);
}
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS

TEST(Dsp, GetDspTable) {
  EXPECT_EQ(GetDspTable(1), nullptr);
  EXPECT_NE(GetDspTable(kBitdepth8), nullptr);
  EXPECT_EQ(dsp_internal::GetWritableDspTable(1), nullptr);
  EXPECT_NE(dsp_internal::GetWritableDspTable(kBitdepth8), nullptr);
#if LIBGAV1_MAX_BITDEPTH >= 10
  EXPECT_NE(GetDspTable(kBitdepth10), nullptr);
  EXPECT_NE(dsp_internal::GetWritableDspTable(kBitdepth10), nullptr);
#else
  EXPECT_EQ(GetDspTable(kBitdepth10), nullptr);
  EXPECT_EQ(dsp_internal::GetWritableDspTable(kBitdepth10), nullptr);
#endif
#if LIBGAV1_MAX_BITDEPTH == 12
  EXPECT_NE(GetDspTable(kBitdepth12), nullptr);
  EXPECT_NE(dsp_internal::GetWritableDspTable(kBitdepth12), nullptr);
#else
  EXPECT_EQ(GetDspTable(kBitdepth12), nullptr);
  EXPECT_EQ(dsp_internal::GetWritableDspTable(kBitdepth12), nullptr);
#endif
}

}  // namespace
}  // namespace dsp
}  // namespace libgav1
