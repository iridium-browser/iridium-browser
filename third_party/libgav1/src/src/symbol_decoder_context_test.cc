// Copyright 2021 The libgav1 Authors
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

#include "src/symbol_decoder_context.h"

#include <cstdint>
#include <cstring>

#include "gtest/gtest.h"
#include "src/utils/constants.h"

namespace libgav1 {
namespace {

TEST(SymbolDecoderContextTest, ResetIntraFrameYModeCdf) {
  // Note these are zero-initialized separately to avoid differences in padding
  // values added to tables for alignment purposes when comparing the contexts
  // with memcmp().
  libgav1::SymbolDecoderContext gold_context = {};
  libgav1::SymbolDecoderContext context = {};
  gold_context.Initialize(0);
  context.Initialize(0);
  EXPECT_EQ(memcmp(&gold_context, &context, sizeof(gold_context)), 0);
  EXPECT_EQ(context.intra_frame_y_mode_cdf[0][0][0], 32768 - 15588);
  EXPECT_EQ(context.intra_frame_y_mode_cdf[0][0][1], 32768 - 17027);
  ++context.intra_frame_y_mode_cdf[0][0][0];
  --context.intra_frame_y_mode_cdf[0][0][1];
  EXPECT_NE(memcmp(&gold_context, &context, sizeof(gold_context)), 0);
  context.ResetIntraFrameYModeCdf();
  EXPECT_EQ(memcmp(&gold_context, &context, sizeof(gold_context)), 0);
}

void ResetAndVerifyCounters(libgav1::SymbolDecoderContext* const context) {
  libgav1::SymbolDecoderContext gold_context = {};
  gold_context.Initialize(0);
  EXPECT_NE(memcmp(&gold_context, context, sizeof(gold_context)), 0);
  context->ResetCounters();
  EXPECT_EQ(memcmp(&gold_context, context, sizeof(gold_context)), 0);
}

TEST(SymbolDecoderContextTest, ResetCounters1d) {
  libgav1::SymbolDecoderContext context = {};
  context.Initialize(0);
  int value = 0;
  context.delta_q_cdf[libgav1::kDeltaSymbolCount] = ++value;
  context.delta_lf_cdf[libgav1::kDeltaSymbolCount] = ++value;
  context.intra_block_copy_cdf[libgav1::kBooleanSymbolCount] = ++value;
  context.cfl_alpha_signs_cdf[libgav1::kCflAlphaSignsSymbolCount] = ++value;
  context.filter_intra_mode_cdf[libgav1::kNumFilterIntraPredictors] = ++value;
  context.restoration_type_cdf[libgav1::kRestorationTypeSymbolCount] = ++value;
  context.use_wiener_cdf[libgav1::kBooleanSymbolCount] = ++value;
  context.use_sgrproj_cdf[libgav1::kBooleanSymbolCount] = ++value;
  ResetAndVerifyCounters(&context);
}

void IncreasePartitionCounters(SymbolDecoderContext* symbol_context,
                               int value) {
  const int min_bsize_log2 = k4x4WidthLog2[kBlock8x8];
  const int max_bsize_log2 = k4x4WidthLog2[kBlock128x128];
  for (int block_size_log2 = min_bsize_log2; block_size_log2 <= max_bsize_log2;
       ++block_size_log2) {
    for (int context = 0; context < kPartitionContexts; ++context) {
      const int cdf_size =
          SymbolDecoderContext::PartitionCdfSize(block_size_log2);
      symbol_context->partition_cdf[block_size_log2 - min_bsize_log2][context]
                                   [cdf_size] += value;
    }
  }
}

void IncreasePaletteColorIndexCounters(SymbolDecoderContext* symbol_context,
                                       int value) {
  for (auto& palette_color_index_cdf_plane :
       symbol_context->palette_color_index_cdf) {
    for (int symbol_count = 0; symbol_count < kPaletteSizeSymbolCount;
         ++symbol_count) {
      const int cdf_size = symbol_count + kMinPaletteSize;
      for (int context = 0; context < kPaletteColorIndexContexts; ++context) {
        palette_color_index_cdf_plane[symbol_count][context][cdf_size] += value;
      }
    }
  }
}

void IncreaseTxTypeCounters(SymbolDecoderContext* context, int value) {
  for (int set_idx = kTransformSetIntra1; set_idx <= kTransformSetIntra2;
       ++set_idx) {
    auto tx_set = static_cast<TransformSet>(set_idx);
    for (int tx_size = 0; tx_size < kNumExtendedTransformSizes; ++tx_size) {
      for (int mode = 0; mode < kIntraPredictionModesY; ++mode) {
        context->intra_tx_type_cdf[SymbolDecoderContext::TxTypeIndex(
            tx_set)][tx_size][mode][kNumTransformTypesInSet[tx_set]] += value;
      }
    }
  }

  for (int set_idx = kTransformSetInter1; set_idx <= kTransformSetInter3;
       ++set_idx) {
    auto tx_set = static_cast<TransformSet>(set_idx);
    for (int tx_size = 0; tx_size < kNumExtendedTransformSizes; ++tx_size) {
      context->inter_tx_type_cdf[SymbolDecoderContext::TxTypeIndex(tx_set)]
                                [tx_size][kNumTransformTypesInSet[tx_set]] +=
          value;
    }
  }
}

void IncreaseTxDepthCounters(SymbolDecoderContext* symbol_context, int value) {
  for (int context = 0; context < kTxDepthContexts; ++context) {
    symbol_context->tx_depth_cdf[0][context][kMaxTxDepthSymbolCount - 1] +=
        value;
  }

  for (int plane_category = 1; plane_category < 4; ++plane_category) {
    for (int context = 0; context < kTxDepthContexts; ++context) {
      symbol_context
          ->tx_depth_cdf[plane_category][context][kMaxTxDepthSymbolCount] +=
          value;
    }
  }
}

void IncreaseUVModeCounters(SymbolDecoderContext* symbol_context, int value) {
  for (int cfl_allowed = 0; cfl_allowed < kBooleanSymbolCount; ++cfl_allowed) {
    for (int mode = 0; mode < kIntraPredictionModesY; ++mode) {
      symbol_context->uv_mode_cdf[cfl_allowed][mode][kIntraPredictionModesUV -
                                                     (1 - cfl_allowed)] +=
          value;
    }
  }
}

#define ASSIGN_COUNTER_2D(array, offset) \
  do {                                   \
    for (auto& d1 : context.array) {     \
      d1[libgav1::offset] = ++value;     \
    }                                    \
  } while (false)

TEST(SymbolDecoderContextTest, ResetCounters2d) {
  libgav1::SymbolDecoderContext context = {};
  context.Initialize(0);
  int value = 0;
  ASSIGN_COUNTER_2D(segment_id_cdf, kMaxSegments);
  ASSIGN_COUNTER_2D(use_predicted_segment_id_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_2D(skip_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_2D(skip_mode_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_2D(delta_lf_multi_cdf, kDeltaSymbolCount);
  ASSIGN_COUNTER_2D(y_mode_cdf, kIntraPredictionModesY);
  ASSIGN_COUNTER_2D(angle_delta_cdf, kAngleDeltaSymbolCount);
  ASSIGN_COUNTER_2D(cfl_alpha_cdf, kCflAlphaSymbolCount);
  ASSIGN_COUNTER_2D(use_filter_intra_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_2D(tx_split_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_2D(eob_pt_512_cdf, kEobPt512SymbolCount);
  ASSIGN_COUNTER_2D(eob_pt_1024_cdf, kEobPt1024SymbolCount);
  ASSIGN_COUNTER_2D(palette_y_size_cdf, kPaletteSizeSymbolCount);
  ASSIGN_COUNTER_2D(has_palette_uv_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_2D(palette_uv_size_cdf, kPaletteSizeSymbolCount);
  ASSIGN_COUNTER_2D(is_inter_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_2D(use_compound_reference_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_2D(compound_reference_type_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_2D(compound_prediction_mode_cdf,
                    kNumCompoundInterPredictionModes);
  ASSIGN_COUNTER_2D(new_mv_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_2D(zero_mv_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_2D(reference_mv_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_2D(ref_mv_index_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_2D(is_inter_intra_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_2D(inter_intra_mode_cdf, kNumInterIntraModes);
  ASSIGN_COUNTER_2D(is_wedge_inter_intra_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_2D(wedge_index_cdf, kWedgeIndexSymbolCount);
  ASSIGN_COUNTER_2D(use_obmc_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_2D(motion_mode_cdf, kNumMotionModes);
  ASSIGN_COUNTER_2D(is_explicit_compound_type_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_2D(is_compound_type_average_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_2D(compound_type_cdf, kNumExplicitCompoundPredictionTypes);
  ASSIGN_COUNTER_2D(interpolation_filter_cdf, kNumExplicitInterpolationFilters);
  ASSIGN_COUNTER_2D(mv_joint_cdf, kNumMvJointTypes);
  ResetAndVerifyCounters(&context);
}

#undef ASSIGN_COUNTER_2D

#define ASSIGN_COUNTER_3D(array, offset) \
  do {                                   \
    for (auto& d1 : context.array) {     \
      for (auto& d2 : d1) {              \
        d2[libgav1::offset] = ++value;   \
      }                                  \
    }                                    \
  } while (false)

TEST(SymbolDecoderContextTest, ResetCounters3d) {
  libgav1::SymbolDecoderContext context = {};
  context.Initialize(0);
  int value = 0;
  ASSIGN_COUNTER_3D(intra_frame_y_mode_cdf, kIntraPredictionModesY);
  ASSIGN_COUNTER_3D(all_zero_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_3D(eob_pt_16_cdf, kEobPt16SymbolCount);
  ASSIGN_COUNTER_3D(eob_pt_32_cdf, kEobPt32SymbolCount);
  ASSIGN_COUNTER_3D(eob_pt_64_cdf, kEobPt64SymbolCount);
  ASSIGN_COUNTER_3D(eob_pt_128_cdf, kEobPt128SymbolCount);
  ASSIGN_COUNTER_3D(eob_pt_256_cdf, kEobPt256SymbolCount);
  ASSIGN_COUNTER_3D(dc_sign_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_3D(has_palette_y_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_3D(compound_backward_reference_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_3D(single_reference_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_3D(mv_sign_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_3D(mv_class_cdf, kMvClassSymbolCount);
  ASSIGN_COUNTER_3D(mv_class0_bit_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_3D(mv_class0_high_precision_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_3D(mv_fraction_cdf, kMvFractionSymbolCount);
  ASSIGN_COUNTER_3D(mv_high_precision_cdf, kBooleanSymbolCount);
  IncreasePartitionCounters(&context, value);
  IncreaseTxTypeCounters(&context, value);
  IncreaseTxDepthCounters(&context, value);
  IncreaseUVModeCounters(&context, value);
  ResetAndVerifyCounters(&context);
}

#undef ASSIGN_COUNTER_3D

#define ASSIGN_COUNTER_4D(array, offset) \
  do {                                   \
    for (auto& d1 : context.array) {     \
      for (auto& d2 : d1) {              \
        for (auto& d3 : d2) {            \
          d3[libgav1::offset] = ++value; \
        }                                \
      }                                  \
    }                                    \
  } while (false)

TEST(SymbolDecoderContextTest, ResetCounters4d) {
  libgav1::SymbolDecoderContext context = {};
  context.Initialize(0);
  int value = 0;
  ASSIGN_COUNTER_4D(eob_extra_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_4D(coeff_base_eob_cdf, kCoeffBaseEobSymbolCount);
  ASSIGN_COUNTER_4D(coeff_base_cdf, kCoeffBaseSymbolCount);
  ASSIGN_COUNTER_4D(coeff_base_range_cdf, kCoeffBaseRangeSymbolCount);
  ASSIGN_COUNTER_4D(compound_reference_cdf, kBooleanSymbolCount);
  ASSIGN_COUNTER_4D(mv_class0_fraction_cdf, kMvFractionSymbolCount);
  ASSIGN_COUNTER_4D(mv_bit_cdf, kBooleanSymbolCount);
  IncreasePaletteColorIndexCounters(&context, value);
  IncreaseTxTypeCounters(&context, value);
  ResetAndVerifyCounters(&context);
}

#undef ASSIGN_COUNTER_4D

}  // namespace
}  // namespace libgav1
