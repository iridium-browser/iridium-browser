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

#include "src/symbol_decoder_context.h"

#include <cassert>
#include <cstring>
#include <type_traits>

namespace libgav1 {
namespace {

// Import all the constants in the anonymous namespace.
#include "src/symbol_decoder_context_cdfs.inc"

uint8_t GetQuantizerContext(int base_quantizer_index) {
  if (base_quantizer_index <= 20) return 0;
  if (base_quantizer_index <= 60) return 1;
  if (base_quantizer_index <= 120) return 2;
  return 3;
}

// Reset*Counters() are helper functions to reset the CDF arrays where the
// counters are not in the last element of the innermost dimension.

void ResetPartitionCounters(SymbolDecoderContext* const context) {
  int block_size_log2 = k4x4WidthLog2[kBlock8x8];
  for (auto& d1 : context->partition_cdf) {
    const int cdf_size =
        SymbolDecoderContext::PartitionCdfSize(block_size_log2++);
    for (auto& d2 : d1) {
      d2[cdf_size] = 0;
    }
  }
}

void ResetPaletteColorIndexCounters(SymbolDecoderContext* const context) {
  for (auto& d1 : context->palette_color_index_cdf) {
    int cdf_size = kMinPaletteSize;
    for (auto& d2 : d1) {
      for (auto& d3 : d2) {
        d3[cdf_size] = 0;
      }
      ++cdf_size;
    }
  }
}

void ResetTxTypeCounters(SymbolDecoderContext* const context) {
  int set_index = kTransformSetIntra1;
  for (auto& d1 : context->intra_tx_type_cdf) {
    const int cdf_size = kNumTransformTypesInSet[set_index++];
    for (auto& d2 : d1) {
      for (auto& d3 : d2) {
        d3[cdf_size] = 0;
      }
    }
  }
  for (auto& d1 : context->inter_tx_type_cdf) {
    const int cdf_size = kNumTransformTypesInSet[set_index++];
    for (auto& d2 : d1) {
      d2[cdf_size] = 0;
    }
  }
}

void ResetTxDepthCounters(SymbolDecoderContext* const context) {
  int delta = 1;
  for (auto& d1 : context->tx_depth_cdf) {
    const int cdf_size = kMaxTxDepthSymbolCount - delta;
    delta = 0;
    for (auto& d2 : d1) {
      d2[cdf_size] = 0;
    }
  }
}

void ResetUVModeCounters(SymbolDecoderContext* const context) {
  int cdf_size = kIntraPredictionModesUV - 1;
  for (auto& d1 : context->uv_mode_cdf) {
    for (auto& d2 : d1) {
      d2[cdf_size] = 0;
    }
    ++cdf_size;
  }
}

}  // namespace

#define CDF_COPY(source, destination)                       \
  static_assert(sizeof(source) == sizeof(destination), ""); \
  memcpy(destination, source, sizeof(source))

void SymbolDecoderContext::Initialize(int base_quantizer_index) {
  CDF_COPY(kDefaultPartitionCdf, partition_cdf);
  CDF_COPY(kDefaultSkipCdf, skip_cdf);
  CDF_COPY(kDefaultSkipModeCdf, skip_mode_cdf);
  CDF_COPY(kDefaultSegmentIdCdf, segment_id_cdf);
  CDF_COPY(kDefaultUsePredictedSegmentIdCdf, use_predicted_segment_id_cdf);
  CDF_COPY(kDefaultDeltaQCdf, delta_q_cdf);
  CDF_COPY(kDefaultDeltaQCdf, delta_lf_cdf);
  for (auto& delta_lf_multi_cdf_entry : delta_lf_multi_cdf) {
    CDF_COPY(kDefaultDeltaQCdf, delta_lf_multi_cdf_entry);
  }
  CDF_COPY(kDefaultIntraBlockCopyCdf, intra_block_copy_cdf);
  CDF_COPY(kDefaultIntraFrameYModeCdf, intra_frame_y_mode_cdf);
  CDF_COPY(kDefaultYModeCdf, y_mode_cdf);
  CDF_COPY(kDefaultAngleDeltaCdf, angle_delta_cdf);
  CDF_COPY(kDefaultUVModeCdf, uv_mode_cdf);
  CDF_COPY(kDefaultCflAlphaSignsCdf, cfl_alpha_signs_cdf);
  CDF_COPY(kDefaultCflAlphaCdf, cfl_alpha_cdf);
  CDF_COPY(kDefaultUseFilterIntraCdf, use_filter_intra_cdf);
  CDF_COPY(kDefaultFilterIntraModeCdf, filter_intra_mode_cdf);
  CDF_COPY(kDefaultTxDepthCdf, tx_depth_cdf);
  CDF_COPY(kDefaultTxSplitCdf, tx_split_cdf);
  CDF_COPY(kDefaultInterTxTypeCdf, inter_tx_type_cdf);
  CDF_COPY(kDefaultIntraTxTypeCdf, intra_tx_type_cdf);
  CDF_COPY(kDefaultRestorationTypeCdf, restoration_type_cdf);
  CDF_COPY(kDefaultUseWienerCdf, use_wiener_cdf);
  CDF_COPY(kDefaultUseSgrProjCdf, use_sgrproj_cdf);
  CDF_COPY(kDefaultHasPaletteYCdf, has_palette_y_cdf);
  CDF_COPY(kDefaultPaletteYSizeCdf, palette_y_size_cdf);
  CDF_COPY(kDefaultHasPaletteUVCdf, has_palette_uv_cdf);
  CDF_COPY(kDefaultPaletteUVSizeCdf, palette_uv_size_cdf);
  CDF_COPY(kDefaultPaletteColorIndexCdf, palette_color_index_cdf);
  CDF_COPY(kDefaultIsInterCdf, is_inter_cdf);
  CDF_COPY(kDefaultUseCompoundReferenceCdf, use_compound_reference_cdf);
  CDF_COPY(kDefaultCompoundReferenceTypeCdf, compound_reference_type_cdf);
  CDF_COPY(kDefaultCompoundReferenceCdf, compound_reference_cdf);
  CDF_COPY(kDefaultCompoundBackwardReferenceCdf,
           compound_backward_reference_cdf);
  CDF_COPY(kDefaultSingleReferenceCdf, single_reference_cdf);
  CDF_COPY(kDefaultCompoundPredictionModeCdf, compound_prediction_mode_cdf);
  CDF_COPY(kDefaultNewMvCdf, new_mv_cdf);
  CDF_COPY(kDefaultZeroMvCdf, zero_mv_cdf);
  CDF_COPY(kDefaultReferenceMvCdf, reference_mv_cdf);
  CDF_COPY(kDefaultRefMvIndexCdf, ref_mv_index_cdf);
  CDF_COPY(kDefaultIsInterIntraCdf, is_inter_intra_cdf);
  CDF_COPY(kDefaultInterIntraModeCdf, inter_intra_mode_cdf);
  CDF_COPY(kDefaultIsWedgeInterIntraCdf, is_wedge_inter_intra_cdf);
  CDF_COPY(kDefaultWedgeIndexCdf, wedge_index_cdf);
  CDF_COPY(kDefaultUseObmcCdf, use_obmc_cdf);
  CDF_COPY(kDefaultMotionModeCdf, motion_mode_cdf);
  CDF_COPY(kDefaultIsExplicitCompoundTypeCdf, is_explicit_compound_type_cdf);
  CDF_COPY(kDefaultIsCompoundTypeAverageCdf, is_compound_type_average_cdf);
  CDF_COPY(kDefaultCompoundTypeCdf, compound_type_cdf);
  CDF_COPY(kDefaultInterpolationFilterCdf, interpolation_filter_cdf);
  for (int i = 0; i < kMvContexts; ++i) {
    CDF_COPY(kDefaultMvJointCdf, mv_joint_cdf[i]);
    for (int j = 0; j < kNumMvComponents; ++j) {
      CDF_COPY(kDefaultMvSignCdf, mv_sign_cdf[i][j]);
      CDF_COPY(kDefaultMvClassCdf, mv_class_cdf[i][j]);
      CDF_COPY(kDefaultMvClass0BitCdf, mv_class0_bit_cdf[i][j]);
      CDF_COPY(kDefaultMvClass0FractionCdf, mv_class0_fraction_cdf[i][j]);
      CDF_COPY(kDefaultMvClass0HighPrecisionCdf,
               mv_class0_high_precision_cdf[i][j]);
      CDF_COPY(kDefaultMvBitCdf, mv_bit_cdf[i][j]);
      CDF_COPY(kDefaultMvFractionCdf, mv_fraction_cdf[i][j]);
      CDF_COPY(kDefaultMvHighPrecisionCdf, mv_high_precision_cdf[i][j]);
    }
  }
  const int quantizer_context = GetQuantizerContext(base_quantizer_index);
  CDF_COPY(kDefaultAllZeroCdf[quantizer_context], all_zero_cdf);
  CDF_COPY(kDefaultEobPt16Cdf[quantizer_context], eob_pt_16_cdf);
  CDF_COPY(kDefaultEobPt32Cdf[quantizer_context], eob_pt_32_cdf);
  CDF_COPY(kDefaultEobPt64Cdf[quantizer_context], eob_pt_64_cdf);
  CDF_COPY(kDefaultEobPt128Cdf[quantizer_context], eob_pt_128_cdf);
  CDF_COPY(kDefaultEobPt256Cdf[quantizer_context], eob_pt_256_cdf);
  CDF_COPY(kDefaultEobPt512Cdf[quantizer_context], eob_pt_512_cdf);
  CDF_COPY(kDefaultEobPt1024Cdf[quantizer_context], eob_pt_1024_cdf);
  CDF_COPY(kDefaultEobExtraCdf[quantizer_context], eob_extra_cdf);
  CDF_COPY(kDefaultCoeffBaseEobCdf[quantizer_context], coeff_base_eob_cdf);
  CDF_COPY(kDefaultCoeffBaseCdf[quantizer_context], coeff_base_cdf);
  CDF_COPY(kDefaultCoeffBaseRangeCdf[quantizer_context], coeff_base_range_cdf);
  CDF_COPY(kDefaultDcSignCdf[quantizer_context], dc_sign_cdf);
}

void SymbolDecoderContext::ResetIntraFrameYModeCdf() {
  CDF_COPY(kDefaultIntraFrameYModeCdf, intra_frame_y_mode_cdf);
}

#undef CDF_COPY

// These macros set the last element in the inner-most dimension of the array to
// zero.
#define RESET_COUNTER_1D(array)                              \
  do {                                                       \
    (array)[std::extent<decltype(array), 0>::value - 1] = 0; \
  } while (false)

#define RESET_COUNTER_2D(array)                           \
  do {                                                    \
    for (auto& d1 : (array)) {                            \
      d1[std::extent<decltype(array), 1>::value - 1] = 0; \
    }                                                     \
  } while (false)

#define RESET_COUNTER_3D(array)                             \
  do {                                                      \
    for (auto& d1 : (array)) {                              \
      for (auto& d2 : d1) {                                 \
        d2[std::extent<decltype(array), 2>::value - 1] = 0; \
      }                                                     \
    }                                                       \
  } while (false)

#define RESET_COUNTER_4D(array)                               \
  do {                                                        \
    for (auto& d1 : (array)) {                                \
      for (auto& d2 : d1) {                                   \
        for (auto& d3 : d2) {                                 \
          d3[std::extent<decltype(array), 3>::value - 1] = 0; \
        }                                                     \
      }                                                       \
    }                                                         \
  } while (false)

void SymbolDecoderContext::ResetCounters() {
  ResetPartitionCounters(this);
  RESET_COUNTER_2D(segment_id_cdf);
  RESET_COUNTER_2D(use_predicted_segment_id_cdf);
  RESET_COUNTER_2D(skip_cdf);
  RESET_COUNTER_2D(skip_mode_cdf);
  RESET_COUNTER_1D(delta_q_cdf);
  RESET_COUNTER_1D(delta_lf_cdf);
  RESET_COUNTER_2D(delta_lf_multi_cdf);
  RESET_COUNTER_1D(intra_block_copy_cdf);
  RESET_COUNTER_3D(intra_frame_y_mode_cdf);
  RESET_COUNTER_2D(y_mode_cdf);
  RESET_COUNTER_2D(angle_delta_cdf);
  ResetUVModeCounters(this);
  RESET_COUNTER_1D(cfl_alpha_signs_cdf);
  RESET_COUNTER_2D(cfl_alpha_cdf);
  RESET_COUNTER_2D(use_filter_intra_cdf);
  RESET_COUNTER_1D(filter_intra_mode_cdf);
  ResetTxDepthCounters(this);
  RESET_COUNTER_2D(tx_split_cdf);
  RESET_COUNTER_3D(all_zero_cdf);
  ResetTxTypeCounters(this);
  RESET_COUNTER_3D(eob_pt_16_cdf);
  RESET_COUNTER_3D(eob_pt_32_cdf);
  RESET_COUNTER_3D(eob_pt_64_cdf);
  RESET_COUNTER_3D(eob_pt_128_cdf);
  RESET_COUNTER_3D(eob_pt_256_cdf);
  RESET_COUNTER_2D(eob_pt_512_cdf);
  RESET_COUNTER_2D(eob_pt_1024_cdf);
  RESET_COUNTER_4D(eob_extra_cdf);
  RESET_COUNTER_4D(coeff_base_eob_cdf);
  RESET_COUNTER_4D(coeff_base_cdf);
  RESET_COUNTER_4D(coeff_base_range_cdf);
  RESET_COUNTER_3D(dc_sign_cdf);
  RESET_COUNTER_1D(restoration_type_cdf);
  RESET_COUNTER_1D(use_wiener_cdf);
  RESET_COUNTER_1D(use_sgrproj_cdf);
  RESET_COUNTER_3D(has_palette_y_cdf);
  RESET_COUNTER_2D(palette_y_size_cdf);
  RESET_COUNTER_2D(has_palette_uv_cdf);
  RESET_COUNTER_2D(palette_uv_size_cdf);
  ResetPaletteColorIndexCounters(this);
  RESET_COUNTER_2D(is_inter_cdf);
  RESET_COUNTER_2D(use_compound_reference_cdf);
  RESET_COUNTER_2D(compound_reference_type_cdf);
  RESET_COUNTER_4D(compound_reference_cdf);
  RESET_COUNTER_3D(compound_backward_reference_cdf);
  RESET_COUNTER_3D(single_reference_cdf);
  RESET_COUNTER_2D(compound_prediction_mode_cdf);
  RESET_COUNTER_2D(new_mv_cdf);
  RESET_COUNTER_2D(zero_mv_cdf);
  RESET_COUNTER_2D(reference_mv_cdf);
  RESET_COUNTER_2D(ref_mv_index_cdf);
  RESET_COUNTER_2D(is_inter_intra_cdf);
  RESET_COUNTER_2D(inter_intra_mode_cdf);
  RESET_COUNTER_2D(is_wedge_inter_intra_cdf);
  RESET_COUNTER_2D(wedge_index_cdf);
  RESET_COUNTER_2D(use_obmc_cdf);
  RESET_COUNTER_2D(motion_mode_cdf);
  RESET_COUNTER_2D(is_explicit_compound_type_cdf);
  RESET_COUNTER_2D(is_compound_type_average_cdf);
  RESET_COUNTER_2D(compound_type_cdf);
  RESET_COUNTER_2D(interpolation_filter_cdf);
  RESET_COUNTER_2D(mv_joint_cdf);
  RESET_COUNTER_3D(mv_sign_cdf);
  RESET_COUNTER_3D(mv_class_cdf);
  RESET_COUNTER_3D(mv_class0_bit_cdf);
  RESET_COUNTER_4D(mv_class0_fraction_cdf);
  RESET_COUNTER_3D(mv_class0_high_precision_cdf);
  RESET_COUNTER_4D(mv_bit_cdf);
  RESET_COUNTER_3D(mv_fraction_cdf);
  RESET_COUNTER_3D(mv_high_precision_cdf);
}

#undef RESET_COUNTER_1D
#undef RESET_COUNTER_2D
#undef RESET_COUNTER_3D
#undef RESET_COUNTER_4D

int SymbolDecoderContext::PartitionCdfSize(int block_size_log2) {
  assert(block_size_log2 > 0);
  assert(block_size_log2 < 6);

  switch (block_size_log2) {
    case 1:
      return kPartitionSplit + 1;
    case 5:
      return kPartitionVerticalWithRightSplit + 1;
    default:
      return kMaxPartitionTypes;
  }
}

}  // namespace libgav1
