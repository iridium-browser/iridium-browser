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

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

#include "src/buffer_pool.h"
#include "src/dsp/constants.h"
#include "src/motion_vector.h"
#include "src/obu_parser.h"
#include "src/prediction_mask.h"
#include "src/symbol_decoder_context.h"
#include "src/tile.h"
#include "src/utils/array_2d.h"
#include "src/utils/bit_mask_set.h"
#include "src/utils/block_parameters_holder.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/entropy_decoder.h"
#include "src/utils/logging.h"
#include "src/utils/segmentation.h"
#include "src/utils/segmentation_map.h"
#include "src/utils/types.h"

namespace libgav1 {
namespace {

constexpr int kDeltaQSmall = 3;
constexpr int kDeltaLfSmall = 3;

constexpr uint8_t kIntraYModeContext[kIntraPredictionModesY] = {
    0, 1, 2, 3, 4, 4, 4, 4, 3, 0, 1, 2, 0};

constexpr uint8_t kSizeGroup[kMaxBlockSizes] = {
    0, 0, 0, 0, 1, 1, 1, 0, 1, 2, 2, 2, 1, 2, 3, 3, 2, 3, 3, 3, 3, 3};

constexpr int kCompoundModeNewMvContexts = 5;
constexpr uint8_t kCompoundModeContextMap[3][kCompoundModeNewMvContexts] = {
    {0, 1, 1, 1, 1}, {1, 2, 3, 4, 4}, {4, 4, 5, 6, 7}};

enum CflSign : uint8_t {
  kCflSignZero = 0,
  kCflSignNegative = 1,
  kCflSignPositive = 2
};

// For each possible value of the combined signs (which is read from the
// bitstream), this array stores the following: sign_u, sign_v, alpha_u_context,
// alpha_v_context. Only positive entries are used. Entry at index i is computed
// as follows:
// sign_u = i / 3
// sign_v = i % 3
// alpha_u_context = i - 2
// alpha_v_context = (sign_v - 1) * 3 + sign_u
constexpr int8_t kCflAlphaLookup[kCflAlphaSignsSymbolCount][4] = {
    {0, 1, -2, 0}, {0, 2, -1, 3}, {1, 0, 0, -2}, {1, 1, 1, 1},
    {1, 2, 2, 4},  {2, 0, 3, -1}, {2, 1, 4, 2},  {2, 2, 5, 5},
};

constexpr BitMaskSet kPredictionModeHasNearMvMask(kPredictionModeNearMv,
                                                  kPredictionModeNearNearMv,
                                                  kPredictionModeNearNewMv,
                                                  kPredictionModeNewNearMv);

constexpr BitMaskSet kIsInterIntraModeAllowedMask(kBlock8x8, kBlock8x16,
                                                  kBlock16x8, kBlock16x16,
                                                  kBlock16x32, kBlock32x16,
                                                  kBlock32x32);

bool IsBackwardReference(ReferenceFrameType type) {
  return type >= kReferenceFrameBackward && type <= kReferenceFrameAlternate;
}

bool IsSameDirectionReferencePair(ReferenceFrameType type1,
                                  ReferenceFrameType type2) {
  return (type1 >= kReferenceFrameBackward) ==
         (type2 >= kReferenceFrameBackward);
}

// This is called neg_deinterleave() in the spec.
int DecodeSegmentId(int diff, int reference, int max) {
  if (reference == 0) return diff;
  if (reference >= max - 1) return max - diff - 1;
  const int value = ((diff & 1) != 0) ? reference + ((diff + 1) >> 1)
                                      : reference - (diff >> 1);
  const int reference2 = (reference << 1);
  if (reference2 < max) {
    return (diff <= reference2) ? value : diff;
  }
  return (diff <= ((max - reference - 1) << 1)) ? value : max - (diff + 1);
}

// This is called DrlCtxStack in section 7.10.2.14 of the spec.
// In the spec, the weights of all the nearest mvs are incremented by a bonus
// weight which is larger than any natural weight, and the weights of the mvs
// are compared with this bonus weight to determine their contexts. We replace
// this procedure by introducing |nearest_mv_count| in PredictionParameters,
// which records the count of the nearest mvs. Since all the nearest mvs are in
// the beginning of the mv stack, the |index| of a mv in the mv stack can be
// compared with |nearest_mv_count| to get that mv's context.
int GetRefMvIndexContext(int nearest_mv_count, int index) {
  if (index + 1 < nearest_mv_count) {
    return 0;
  }
  if (index + 1 == nearest_mv_count) {
    return 1;
  }
  return 2;
}

// Returns true if both the width and height of the block is less than 64.
bool IsBlockDimensionLessThan64(BlockSize size) {
  return size <= kBlock32x32 && size != kBlock16x64;
}

int GetUseCompoundReferenceContext(const Tile::Block& block) {
  if (block.top_available[kPlaneY] && block.left_available[kPlaneY]) {
    if (block.IsTopSingle() && block.IsLeftSingle()) {
      return static_cast<int>(IsBackwardReference(block.TopReference(0))) ^
             static_cast<int>(IsBackwardReference(block.LeftReference(0)));
    }
    if (block.IsTopSingle()) {
      return 2 + static_cast<int>(IsBackwardReference(block.TopReference(0)) ||
                                  block.IsTopIntra());
    }
    if (block.IsLeftSingle()) {
      return 2 + static_cast<int>(IsBackwardReference(block.LeftReference(0)) ||
                                  block.IsLeftIntra());
    }
    return 4;
  }
  if (block.top_available[kPlaneY]) {
    return block.IsTopSingle()
               ? static_cast<int>(IsBackwardReference(block.TopReference(0)))
               : 3;
  }
  if (block.left_available[kPlaneY]) {
    return block.IsLeftSingle()
               ? static_cast<int>(IsBackwardReference(block.LeftReference(0)))
               : 3;
  }
  return 1;
}

// Calculates count0 by calling block.CountReferences() on the frame types from
// type0_start to type0_end, inclusive, and summing the results.
// Calculates count1 by calling block.CountReferences() on the frame types from
// type1_start to type1_end, inclusive, and summing the results.
// Compares count0 with count1 and returns 0, 1 or 2.
//
// See count_refs and ref_count_ctx in 8.3.2.
int GetReferenceContext(const Tile::Block& block,
                        ReferenceFrameType type0_start,
                        ReferenceFrameType type0_end,
                        ReferenceFrameType type1_start,
                        ReferenceFrameType type1_end) {
  int count0 = 0;
  int count1 = 0;
  for (int type = type0_start; type <= type0_end; ++type) {
    count0 += block.CountReferences(static_cast<ReferenceFrameType>(type));
  }
  for (int type = type1_start; type <= type1_end; ++type) {
    count1 += block.CountReferences(static_cast<ReferenceFrameType>(type));
  }
  return (count0 < count1) ? 0 : (count0 == count1 ? 1 : 2);
}

}  // namespace

bool Tile::ReadSegmentId(const Block& block) {
  // These two asserts ensure that current_frame_.segmentation_map() is not
  // nullptr.
  assert(frame_header_.segmentation.enabled);
  assert(frame_header_.segmentation.update_map);
  const SegmentationMap& map = *current_frame_.segmentation_map();
  int top_left = -1;
  if (block.top_available[kPlaneY] && block.left_available[kPlaneY]) {
    top_left = map.segment_id(block.row4x4 - 1, block.column4x4 - 1);
  }
  int top = -1;
  if (block.top_available[kPlaneY]) {
    top = map.segment_id(block.row4x4 - 1, block.column4x4);
  }
  int left = -1;
  if (block.left_available[kPlaneY]) {
    left = map.segment_id(block.row4x4, block.column4x4 - 1);
  }
  int pred;
  if (top == -1) {
    pred = (left == -1) ? 0 : left;
  } else if (left == -1) {
    pred = top;
  } else {
    pred = (top_left == top) ? top : left;
  }
  BlockParameters& bp = *block.bp;
  if (bp.skip) {
    bp.prediction_parameters->segment_id = pred;
    return true;
  }
  int context = 0;
  if (top_left < 0) {
    context = 0;
  } else if (top_left == top && top_left == left) {
    context = 2;
  } else if (top_left == top || top_left == left || top == left) {
    context = 1;
  }
  uint16_t* const segment_id_cdf =
      symbol_decoder_context_.segment_id_cdf[context];
  const int encoded_segment_id =
      reader_.ReadSymbol<kMaxSegments>(segment_id_cdf);
  bp.prediction_parameters->segment_id =
      DecodeSegmentId(encoded_segment_id, pred,
                      frame_header_.segmentation.last_active_segment_id + 1);
  // Check the bitstream conformance requirement in Section 6.10.8 of the spec.
  if (bp.prediction_parameters->segment_id < 0 ||
      bp.prediction_parameters->segment_id >
          frame_header_.segmentation.last_active_segment_id) {
    LIBGAV1_DLOG(
        ERROR,
        "Corrupted segment_ids: encoded %d, last active %d, postprocessed %d",
        encoded_segment_id, frame_header_.segmentation.last_active_segment_id,
        bp.prediction_parameters->segment_id);
    return false;
  }
  return true;
}

bool Tile::ReadIntraSegmentId(const Block& block) {
  BlockParameters& bp = *block.bp;
  if (!frame_header_.segmentation.enabled) {
    bp.prediction_parameters->segment_id = 0;
    return true;
  }
  return ReadSegmentId(block);
}

void Tile::ReadSkip(const Block& block) {
  BlockParameters& bp = *block.bp;
  if (frame_header_.segmentation.segment_id_pre_skip &&
      frame_header_.segmentation.FeatureActive(
          bp.prediction_parameters->segment_id, kSegmentFeatureSkip)) {
    bp.skip = true;
    return;
  }
  int context = 0;
  if (block.top_available[kPlaneY] && block.bp_top->skip) {
    ++context;
  }
  if (block.left_available[kPlaneY] && block.bp_left->skip) {
    ++context;
  }
  uint16_t* const skip_cdf = symbol_decoder_context_.skip_cdf[context];
  bp.skip = reader_.ReadSymbol(skip_cdf);
}

bool Tile::ReadSkipMode(const Block& block) {
  BlockParameters& bp = *block.bp;
  if (!frame_header_.skip_mode_present ||
      frame_header_.segmentation.FeatureActive(
          bp.prediction_parameters->segment_id, kSegmentFeatureSkip) ||
      frame_header_.segmentation.FeatureActive(
          bp.prediction_parameters->segment_id,
          kSegmentFeatureReferenceFrame) ||
      frame_header_.segmentation.FeatureActive(
          bp.prediction_parameters->segment_id, kSegmentFeatureGlobalMv) ||
      IsBlockDimension4(block.size)) {
    return false;
  }
  const int context =
      (block.left_available[kPlaneY]
           ? static_cast<int>(left_context_.skip_mode[block.left_context_index])
           : 0) +
      (block.top_available[kPlaneY]
           ? static_cast<int>(
                 block.top_context->skip_mode[block.top_context_index])
           : 0);
  return reader_.ReadSymbol(symbol_decoder_context_.skip_mode_cdf[context]);
}

void Tile::ReadCdef(const Block& block) {
  BlockParameters& bp = *block.bp;
  if (bp.skip || frame_header_.coded_lossless ||
      !sequence_header_.enable_cdef || frame_header_.allow_intrabc ||
      frame_header_.cdef.bits == 0) {
    return;
  }
  int8_t* const cdef_index =
      &cdef_index_[DivideBy16(block.row4x4)][DivideBy16(block.column4x4)];
  int stride = cdef_index_.columns();
  if (cdef_index[0] == -1) {
    cdef_index[0] =
        static_cast<int8_t>(reader_.ReadLiteral(frame_header_.cdef.bits));
    if (block.size == kBlock128x128) {
      // This condition is shorthand for block.width4x4 > 16 && block.height4x4
      // > 16.
      cdef_index[1] = cdef_index[0];
      cdef_index[stride] = cdef_index[0];
      cdef_index[stride + 1] = cdef_index[0];
    } else if (block.width4x4 > 16) {
      cdef_index[1] = cdef_index[0];
    } else if (block.height4x4 > 16) {
      cdef_index[stride] = cdef_index[0];
    }
  }
}

int Tile::ReadAndClipDelta(uint16_t* const cdf, int delta_small, int scale,
                           int min_value, int max_value, int value) {
  int abs = reader_.ReadSymbol<kDeltaSymbolCount>(cdf);
  if (abs == delta_small) {
    const int remaining_bit_count =
        static_cast<int>(reader_.ReadLiteral(3)) + 1;
    const int abs_remaining_bits =
        static_cast<int>(reader_.ReadLiteral(remaining_bit_count));
    abs = abs_remaining_bits + (1 << remaining_bit_count) + 1;
  }
  if (abs != 0) {
    const bool sign = reader_.ReadBit() != 0;
    const int scaled_abs = abs << scale;
    const int reduced_delta = sign ? -scaled_abs : scaled_abs;
    value += reduced_delta;
    value = Clip3(value, min_value, max_value);
  }
  return value;
}

void Tile::ReadQuantizerIndexDelta(const Block& block) {
  assert(read_deltas_);
  BlockParameters& bp = *block.bp;
  if ((block.size == SuperBlockSize() && bp.skip)) {
    return;
  }
  current_quantizer_index_ =
      ReadAndClipDelta(symbol_decoder_context_.delta_q_cdf, kDeltaQSmall,
                       frame_header_.delta_q.scale, kMinLossyQuantizer,
                       kMaxQuantizer, current_quantizer_index_);
}

void Tile::ReadLoopFilterDelta(const Block& block) {
  assert(read_deltas_);
  BlockParameters& bp = *block.bp;
  if (!frame_header_.delta_lf.present ||
      (block.size == SuperBlockSize() && bp.skip)) {
    return;
  }
  int frame_lf_count = 1;
  if (frame_header_.delta_lf.multi) {
    frame_lf_count = kFrameLfCount - (PlaneCount() > 1 ? 0 : 2);
  }
  bool recompute_deblock_filter_levels = false;
  for (int i = 0; i < frame_lf_count; ++i) {
    uint16_t* const delta_lf_abs_cdf =
        frame_header_.delta_lf.multi
            ? symbol_decoder_context_.delta_lf_multi_cdf[i]
            : symbol_decoder_context_.delta_lf_cdf;
    const int8_t old_delta_lf = delta_lf_[i];
    delta_lf_[i] = ReadAndClipDelta(
        delta_lf_abs_cdf, kDeltaLfSmall, frame_header_.delta_lf.scale,
        -kMaxLoopFilterValue, kMaxLoopFilterValue, delta_lf_[i]);
    recompute_deblock_filter_levels =
        recompute_deblock_filter_levels || (old_delta_lf != delta_lf_[i]);
  }
  delta_lf_all_zero_ =
      (delta_lf_[0] | delta_lf_[1] | delta_lf_[2] | delta_lf_[3]) == 0;
  if (!delta_lf_all_zero_ && recompute_deblock_filter_levels) {
    post_filter_.ComputeDeblockFilterLevels(delta_lf_, deblock_filter_levels_);
  }
}

void Tile::ReadPredictionModeY(const Block& block, bool intra_y_mode) {
  uint16_t* cdf;
  if (intra_y_mode) {
    const PredictionMode top_mode =
        block.top_available[kPlaneY] ? block.bp_top->y_mode : kPredictionModeDc;
    const PredictionMode left_mode = block.left_available[kPlaneY]
                                         ? block.bp_left->y_mode
                                         : kPredictionModeDc;
    const int top_context = kIntraYModeContext[top_mode];
    const int left_context = kIntraYModeContext[left_mode];
    cdf = symbol_decoder_context_
              .intra_frame_y_mode_cdf[top_context][left_context];
  } else {
    cdf = symbol_decoder_context_.y_mode_cdf[kSizeGroup[block.size]];
  }
  block.bp->y_mode = static_cast<PredictionMode>(
      reader_.ReadSymbol<kIntraPredictionModesY>(cdf));
}

void Tile::ReadIntraAngleInfo(const Block& block, PlaneType plane_type) {
  BlockParameters& bp = *block.bp;
  PredictionParameters& prediction_parameters =
      *block.bp->prediction_parameters;
  prediction_parameters.angle_delta[plane_type] = 0;
  const PredictionMode mode = (plane_type == kPlaneTypeY)
                                  ? bp.y_mode
                                  : bp.prediction_parameters->uv_mode;
  if (IsBlockSmallerThan8x8(block.size) || !IsDirectionalMode(mode)) return;
  uint16_t* const cdf =
      symbol_decoder_context_.angle_delta_cdf[mode - kPredictionModeVertical];
  prediction_parameters.angle_delta[plane_type] =
      reader_.ReadSymbol<kAngleDeltaSymbolCount>(cdf);
  prediction_parameters.angle_delta[plane_type] -= kMaxAngleDelta;
}

void Tile::ReadCflAlpha(const Block& block) {
  const int signs = reader_.ReadSymbol<kCflAlphaSignsSymbolCount>(
      symbol_decoder_context_.cfl_alpha_signs_cdf);
  const int8_t* const cfl_lookup = kCflAlphaLookup[signs];
  const auto sign_u = static_cast<CflSign>(cfl_lookup[0]);
  const auto sign_v = static_cast<CflSign>(cfl_lookup[1]);
  PredictionParameters& prediction_parameters =
      *block.bp->prediction_parameters;
  prediction_parameters.cfl_alpha_u = 0;
  if (sign_u != kCflSignZero) {
    assert(cfl_lookup[2] >= 0);
    prediction_parameters.cfl_alpha_u =
        reader_.ReadSymbol<kCflAlphaSymbolCount>(
            symbol_decoder_context_.cfl_alpha_cdf[cfl_lookup[2]]) +
        1;
    if (sign_u == kCflSignNegative) prediction_parameters.cfl_alpha_u *= -1;
  }
  prediction_parameters.cfl_alpha_v = 0;
  if (sign_v != kCflSignZero) {
    assert(cfl_lookup[3] >= 0);
    prediction_parameters.cfl_alpha_v =
        reader_.ReadSymbol<kCflAlphaSymbolCount>(
            symbol_decoder_context_.cfl_alpha_cdf[cfl_lookup[3]]) +
        1;
    if (sign_v == kCflSignNegative) prediction_parameters.cfl_alpha_v *= -1;
  }
}

void Tile::ReadPredictionModeUV(const Block& block) {
  BlockParameters& bp = *block.bp;
  bool chroma_from_luma_allowed;
  if (frame_header_.segmentation
          .lossless[bp.prediction_parameters->segment_id]) {
    chroma_from_luma_allowed = block.residual_size[kPlaneU] == kBlock4x4;
  } else {
    chroma_from_luma_allowed = IsBlockDimensionLessThan64(block.size);
  }
  uint16_t* const cdf =
      symbol_decoder_context_
          .uv_mode_cdf[static_cast<int>(chroma_from_luma_allowed)][bp.y_mode];
  if (chroma_from_luma_allowed) {
    bp.prediction_parameters->uv_mode = static_cast<PredictionMode>(
        reader_.ReadSymbol<kIntraPredictionModesUV>(cdf));
  } else {
    bp.prediction_parameters->uv_mode = static_cast<PredictionMode>(
        reader_.ReadSymbol<kIntraPredictionModesUV - 1>(cdf));
  }
}

int Tile::ReadMotionVectorComponent(const Block& block, const int component) {
  const int context =
      static_cast<int>(block.bp->prediction_parameters->use_intra_block_copy);
  const bool sign = reader_.ReadSymbol(
      symbol_decoder_context_.mv_sign_cdf[component][context]);
  const int mv_class = reader_.ReadSymbol<kMvClassSymbolCount>(
      symbol_decoder_context_.mv_class_cdf[component][context]);
  int magnitude = 1;
  int value;
  uint16_t* fraction_cdf;
  uint16_t* precision_cdf;
  if (mv_class == 0) {
    value = static_cast<int>(reader_.ReadSymbol(
        symbol_decoder_context_.mv_class0_bit_cdf[component][context]));
    fraction_cdf = symbol_decoder_context_
                       .mv_class0_fraction_cdf[component][context][value];
    precision_cdf = symbol_decoder_context_
                        .mv_class0_high_precision_cdf[component][context];
  } else {
    assert(mv_class <= kMvBitSymbolCount);
    value = 0;
    for (int i = 0; i < mv_class; ++i) {
      const int bit = static_cast<int>(reader_.ReadSymbol(
          symbol_decoder_context_.mv_bit_cdf[component][context][i]));
      value |= bit << i;
    }
    magnitude += 2 << (mv_class + 2);
    fraction_cdf = symbol_decoder_context_.mv_fraction_cdf[component][context];
    precision_cdf =
        symbol_decoder_context_.mv_high_precision_cdf[component][context];
  }
  const int fraction =
      (frame_header_.force_integer_mv == 0)
          ? reader_.ReadSymbol<kMvFractionSymbolCount>(fraction_cdf)
          : 3;
  const int precision =
      frame_header_.allow_high_precision_mv
          ? static_cast<int>(reader_.ReadSymbol(precision_cdf))
          : 1;
  magnitude += (value << 3) | (fraction << 1) | precision;
  return sign ? -magnitude : magnitude;
}

void Tile::ReadMotionVector(const Block& block, int index) {
  BlockParameters& bp = *block.bp;
  const int context =
      static_cast<int>(block.bp->prediction_parameters->use_intra_block_copy);
  const auto mv_joint =
      static_cast<MvJointType>(reader_.ReadSymbol<kNumMvJointTypes>(
          symbol_decoder_context_.mv_joint_cdf[context]));
  if (mv_joint == kMvJointTypeHorizontalZeroVerticalNonZero ||
      mv_joint == kMvJointTypeNonZero) {
    bp.mv.mv[index].mv[0] = ReadMotionVectorComponent(block, 0);
  }
  if (mv_joint == kMvJointTypeHorizontalNonZeroVerticalZero ||
      mv_joint == kMvJointTypeNonZero) {
    bp.mv.mv[index].mv[1] = ReadMotionVectorComponent(block, 1);
  }
}

void Tile::ReadFilterIntraModeInfo(const Block& block) {
  BlockParameters& bp = *block.bp;
  PredictionParameters& prediction_parameters =
      *block.bp->prediction_parameters;
  prediction_parameters.use_filter_intra = false;
  if (!sequence_header_.enable_filter_intra || bp.y_mode != kPredictionModeDc ||
      bp.prediction_parameters->palette_mode_info.size[kPlaneTypeY] != 0 ||
      !IsBlockDimensionLessThan64(block.size)) {
    return;
  }
  prediction_parameters.use_filter_intra = reader_.ReadSymbol(
      symbol_decoder_context_.use_filter_intra_cdf[block.size]);
  if (prediction_parameters.use_filter_intra) {
    prediction_parameters.filter_intra_mode = static_cast<FilterIntraPredictor>(
        reader_.ReadSymbol<kNumFilterIntraPredictors>(
            symbol_decoder_context_.filter_intra_mode_cdf));
  }
}

bool Tile::DecodeIntraModeInfo(const Block& block) {
  BlockParameters& bp = *block.bp;
  bp.skip = false;
  if (frame_header_.segmentation.segment_id_pre_skip &&
      !ReadIntraSegmentId(block)) {
    return false;
  }
  SetCdfContextSkipMode(block, false);
  ReadSkip(block);
  if (!frame_header_.segmentation.segment_id_pre_skip &&
      !ReadIntraSegmentId(block)) {
    return false;
  }
  ReadCdef(block);
  if (read_deltas_) {
    ReadQuantizerIndexDelta(block);
    ReadLoopFilterDelta(block);
    read_deltas_ = false;
  }
  PredictionParameters& prediction_parameters =
      *block.bp->prediction_parameters;
  prediction_parameters.use_intra_block_copy = false;
  if (frame_header_.allow_intrabc) {
    prediction_parameters.use_intra_block_copy =
        reader_.ReadSymbol(symbol_decoder_context_.intra_block_copy_cdf);
  }
  if (prediction_parameters.use_intra_block_copy) {
    bp.is_inter = true;
    bp.reference_frame[0] = kReferenceFrameIntra;
    bp.reference_frame[1] = kReferenceFrameNone;
    bp.y_mode = kPredictionModeDc;
    bp.prediction_parameters->uv_mode = kPredictionModeDc;
    SetCdfContextUVMode(block);
    prediction_parameters.motion_mode = kMotionModeSimple;
    prediction_parameters.compound_prediction_type =
        kCompoundPredictionTypeAverage;
    bp.prediction_parameters->palette_mode_info.size[kPlaneTypeY] = 0;
    bp.prediction_parameters->palette_mode_info.size[kPlaneTypeUV] = 0;
    SetCdfContextPaletteSize(block);
    bp.interpolation_filter[0] = kInterpolationFilterBilinear;
    bp.interpolation_filter[1] = kInterpolationFilterBilinear;
    MvContexts dummy_mode_contexts;
    FindMvStack(block, /*is_compound=*/false, &dummy_mode_contexts);
    return AssignIntraMv(block);
  }
  bp.is_inter = false;
  return ReadIntraBlockModeInfo(block, /*intra_y_mode=*/true);
}

int8_t Tile::ComputePredictedSegmentId(const Block& block) const {
  // If prev_segment_ids_ is null, treat it as if it pointed to a segmentation
  // map containing all 0s.
  if (prev_segment_ids_ == nullptr) return 0;

  const int x_limit = std::min(frame_header_.columns4x4 - block.column4x4,
                               static_cast<int>(block.width4x4));
  const int y_limit = std::min(frame_header_.rows4x4 - block.row4x4,
                               static_cast<int>(block.height4x4));
  int8_t id = 7;
  for (int y = 0; y < y_limit; ++y) {
    for (int x = 0; x < x_limit; ++x) {
      const int8_t prev_segment_id =
          prev_segment_ids_->segment_id(block.row4x4 + y, block.column4x4 + x);
      id = std::min(id, prev_segment_id);
    }
  }
  return id;
}

void Tile::SetCdfContextUsePredictedSegmentId(const Block& block,
                                              bool use_predicted_segment_id) {
  memset(left_context_.use_predicted_segment_id + block.left_context_index,
         static_cast<int>(use_predicted_segment_id), block.height4x4);
  memset(block.top_context->use_predicted_segment_id + block.top_context_index,
         static_cast<int>(use_predicted_segment_id), block.width4x4);
}

bool Tile::ReadInterSegmentId(const Block& block, bool pre_skip) {
  BlockParameters& bp = *block.bp;
  if (!frame_header_.segmentation.enabled) {
    bp.prediction_parameters->segment_id = 0;
    return true;
  }
  if (!frame_header_.segmentation.update_map) {
    bp.prediction_parameters->segment_id = ComputePredictedSegmentId(block);
    return true;
  }
  if (pre_skip) {
    if (!frame_header_.segmentation.segment_id_pre_skip) {
      bp.prediction_parameters->segment_id = 0;
      return true;
    }
  } else if (bp.skip) {
    SetCdfContextUsePredictedSegmentId(block, false);
    return ReadSegmentId(block);
  }
  if (frame_header_.segmentation.temporal_update) {
    const int context =
        (block.left_available[kPlaneY]
             ? static_cast<int>(
                   left_context_
                       .use_predicted_segment_id[block.left_context_index])
             : 0) +
        (block.top_available[kPlaneY]
             ? static_cast<int>(
                   block.top_context
                       ->use_predicted_segment_id[block.top_context_index])
             : 0);
    const bool use_predicted_segment_id = reader_.ReadSymbol(
        symbol_decoder_context_.use_predicted_segment_id_cdf[context]);
    SetCdfContextUsePredictedSegmentId(block, use_predicted_segment_id);
    if (use_predicted_segment_id) {
      bp.prediction_parameters->segment_id = ComputePredictedSegmentId(block);
      return true;
    }
  }
  return ReadSegmentId(block);
}

void Tile::ReadIsInter(const Block& block, bool skip_mode) {
  BlockParameters& bp = *block.bp;
  if (skip_mode) {
    bp.is_inter = true;
    return;
  }
  if (frame_header_.segmentation.FeatureActive(
          bp.prediction_parameters->segment_id,
          kSegmentFeatureReferenceFrame)) {
    bp.is_inter = frame_header_.segmentation
                      .feature_data[bp.prediction_parameters->segment_id]
                                   [kSegmentFeatureReferenceFrame] !=
                  kReferenceFrameIntra;
    return;
  }
  if (frame_header_.segmentation.FeatureActive(
          bp.prediction_parameters->segment_id, kSegmentFeatureGlobalMv)) {
    bp.is_inter = true;
    return;
  }
  int context = 0;
  if (block.top_available[kPlaneY] && block.left_available[kPlaneY]) {
    context = (block.IsTopIntra() && block.IsLeftIntra())
                  ? 3
                  : static_cast<int>(block.IsTopIntra() || block.IsLeftIntra());
  } else if (block.top_available[kPlaneY] || block.left_available[kPlaneY]) {
    context = 2 * static_cast<int>(block.top_available[kPlaneY]
                                       ? block.IsTopIntra()
                                       : block.IsLeftIntra());
  }
  bp.is_inter =
      reader_.ReadSymbol(symbol_decoder_context_.is_inter_cdf[context]);
}

void Tile::SetCdfContextPaletteSize(const Block& block) {
  const PaletteModeInfo& palette_mode_info =
      block.bp->prediction_parameters->palette_mode_info;
  for (int plane_type = kPlaneTypeY; plane_type <= kPlaneTypeUV; ++plane_type) {
    memset(left_context_.palette_size[plane_type] + block.left_context_index,
           palette_mode_info.size[plane_type], block.height4x4);
    memset(
        block.top_context->palette_size[plane_type] + block.top_context_index,
        palette_mode_info.size[plane_type], block.width4x4);
    if (palette_mode_info.size[plane_type] == 0) continue;
    for (int i = block.left_context_index;
         i < block.left_context_index + block.height4x4; ++i) {
      memcpy(left_context_.palette_color[i][plane_type],
             palette_mode_info.color[plane_type],
             kMaxPaletteSize * sizeof(palette_mode_info.color[0][0]));
    }
    for (int i = block.top_context_index;
         i < block.top_context_index + block.width4x4; ++i) {
      memcpy(block.top_context->palette_color[i][plane_type],
             palette_mode_info.color[plane_type],
             kMaxPaletteSize * sizeof(palette_mode_info.color[0][0]));
    }
  }
}

void Tile::SetCdfContextUVMode(const Block& block) {
  // BlockCdfContext.uv_mode is only used to compute is_smooth_prediction for
  // the intra edge upsamplers in the subsequent blocks. They have some special
  // rules for subsampled UV planes. For subsampled UV planes, update left
  // context only if current block contains the last odd column and update top
  // context only if current block contains the last odd row.
  if (subsampling_x_[kPlaneU] == 0 || (block.column4x4 & 1) == 1 ||
      block.width4x4 > 1) {
    memset(left_context_.uv_mode + block.left_context_index,
           block.bp->prediction_parameters->uv_mode, block.height4x4);
  }
  if (subsampling_y_[kPlaneU] == 0 || (block.row4x4 & 1) == 1 ||
      block.height4x4 > 1) {
    memset(block.top_context->uv_mode + block.top_context_index,
           block.bp->prediction_parameters->uv_mode, block.width4x4);
  }
}

bool Tile::ReadIntraBlockModeInfo(const Block& block, bool intra_y_mode) {
  BlockParameters& bp = *block.bp;
  bp.reference_frame[0] = kReferenceFrameIntra;
  bp.reference_frame[1] = kReferenceFrameNone;
  ReadPredictionModeY(block, intra_y_mode);
  ReadIntraAngleInfo(block, kPlaneTypeY);
  if (block.HasChroma()) {
    ReadPredictionModeUV(block);
    if (bp.prediction_parameters->uv_mode == kPredictionModeChromaFromLuma) {
      ReadCflAlpha(block);
    }
    if (block.left_available[kPlaneU]) {
      const int smooth_row =
          block.row4x4 + (~block.row4x4 & subsampling_y_[kPlaneU]);
      const int smooth_column =
          block.column4x4 - 1 - (block.column4x4 & subsampling_x_[kPlaneU]);
      const BlockParameters& bp_left =
          *block_parameters_holder_.Find(smooth_row, smooth_column);
      bp.prediction_parameters->chroma_left_uses_smooth_prediction =
          (bp_left.reference_frame[0] <= kReferenceFrameIntra) &&
          kPredictionModeSmoothMask.Contains(
              left_context_.uv_mode[CdfContextIndex(smooth_row)]);
    }
    if (block.top_available[kPlaneU]) {
      const int smooth_row =
          block.row4x4 - 1 - (block.row4x4 & subsampling_y_[kPlaneU]);
      const int smooth_column =
          block.column4x4 + (~block.column4x4 & subsampling_x_[kPlaneU]);
      const BlockParameters& bp_top =
          *block_parameters_holder_.Find(smooth_row, smooth_column);
      bp.prediction_parameters->chroma_top_uses_smooth_prediction =
          (bp_top.reference_frame[0] <= kReferenceFrameIntra) &&
          kPredictionModeSmoothMask.Contains(
              top_context_.get()[SuperBlockColumnIndex(smooth_column)]
                  .uv_mode[CdfContextIndex(smooth_column)]);
    }
    SetCdfContextUVMode(block);
    ReadIntraAngleInfo(block, kPlaneTypeUV);
  }
  ReadPaletteModeInfo(block);
  SetCdfContextPaletteSize(block);
  ReadFilterIntraModeInfo(block);
  return true;
}

CompoundReferenceType Tile::ReadCompoundReferenceType(const Block& block) {
  // compound and inter.
  const bool top_comp_inter = block.top_available[kPlaneY] &&
                              !block.IsTopIntra() && !block.IsTopSingle();
  const bool left_comp_inter = block.left_available[kPlaneY] &&
                               !block.IsLeftIntra() && !block.IsLeftSingle();
  // unidirectional compound.
  const bool top_uni_comp =
      top_comp_inter && IsSameDirectionReferencePair(block.TopReference(0),
                                                     block.TopReference(1));
  const bool left_uni_comp =
      left_comp_inter && IsSameDirectionReferencePair(block.LeftReference(0),
                                                      block.LeftReference(1));
  int context;
  if (block.top_available[kPlaneY] && !block.IsTopIntra() &&
      block.left_available[kPlaneY] && !block.IsLeftIntra()) {
    const int same_direction = static_cast<int>(IsSameDirectionReferencePair(
        block.TopReference(0), block.LeftReference(0)));
    if (!top_comp_inter && !left_comp_inter) {
      context = 1 + MultiplyBy2(same_direction);
    } else if (!top_comp_inter) {
      context = left_uni_comp ? 3 + same_direction : 1;
    } else if (!left_comp_inter) {
      context = top_uni_comp ? 3 + same_direction : 1;
    } else {
      if (!top_uni_comp && !left_uni_comp) {
        context = 0;
      } else if (!top_uni_comp || !left_uni_comp) {
        context = 2;
      } else {
        context = 3 + static_cast<int>(
                          (block.TopReference(0) == kReferenceFrameBackward) ==
                          (block.LeftReference(0) == kReferenceFrameBackward));
      }
    }
  } else if (block.top_available[kPlaneY] && block.left_available[kPlaneY]) {
    if (top_comp_inter) {
      context = 1 + MultiplyBy2(static_cast<int>(top_uni_comp));
    } else if (left_comp_inter) {
      context = 1 + MultiplyBy2(static_cast<int>(left_uni_comp));
    } else {
      context = 2;
    }
  } else if (top_comp_inter) {
    context = MultiplyBy4(static_cast<int>(top_uni_comp));
  } else if (left_comp_inter) {
    context = MultiplyBy4(static_cast<int>(left_uni_comp));
  } else {
    context = 2;
  }
  return static_cast<CompoundReferenceType>(reader_.ReadSymbol(
      symbol_decoder_context_.compound_reference_type_cdf[context]));
}

template <bool is_single, bool is_backward, int index>
uint16_t* Tile::GetReferenceCdf(
    const Block& block,
    CompoundReferenceType type /*= kNumCompoundReferenceTypes*/) {
  int context = 0;
  if ((type == kCompoundReferenceUnidirectional && index == 0) ||
      (is_single && index == 1)) {
    // uni_comp_ref and single_ref_p1.
    context =
        GetReferenceContext(block, kReferenceFrameLast, kReferenceFrameGolden,
                            kReferenceFrameBackward, kReferenceFrameAlternate);
  } else if (type == kCompoundReferenceUnidirectional && index == 1) {
    // uni_comp_ref_p1.
    context =
        GetReferenceContext(block, kReferenceFrameLast2, kReferenceFrameLast2,
                            kReferenceFrameLast3, kReferenceFrameGolden);
  } else if ((type == kCompoundReferenceUnidirectional && index == 2) ||
             (type == kCompoundReferenceBidirectional && index == 2) ||
             (is_single && index == 5)) {
    // uni_comp_ref_p2, comp_ref_p2 and single_ref_p5.
    context =
        GetReferenceContext(block, kReferenceFrameLast3, kReferenceFrameLast3,
                            kReferenceFrameGolden, kReferenceFrameGolden);
  } else if ((type == kCompoundReferenceBidirectional && index == 0) ||
             (is_single && index == 3)) {
    // comp_ref and single_ref_p3.
    context =
        GetReferenceContext(block, kReferenceFrameLast, kReferenceFrameLast2,
                            kReferenceFrameLast3, kReferenceFrameGolden);
  } else if ((type == kCompoundReferenceBidirectional && index == 1) ||
             (is_single && index == 4)) {
    // comp_ref_p1 and single_ref_p4.
    context =
        GetReferenceContext(block, kReferenceFrameLast, kReferenceFrameLast,
                            kReferenceFrameLast2, kReferenceFrameLast2);
  } else if ((is_single && index == 2) || (is_backward && index == 0)) {
    // single_ref_p2 and comp_bwdref.
    context = GetReferenceContext(
        block, kReferenceFrameBackward, kReferenceFrameAlternate2,
        kReferenceFrameAlternate, kReferenceFrameAlternate);
  } else if ((is_single && index == 6) || (is_backward && index == 1)) {
    // single_ref_p6 and comp_bwdref_p1.
    context = GetReferenceContext(
        block, kReferenceFrameBackward, kReferenceFrameBackward,
        kReferenceFrameAlternate2, kReferenceFrameAlternate2);
  }
  // When using GCC 12.x for some targets the compiler reports a false positive
  // with the context subscript when is_single=false, is_backward=false and
  // index=0. GetReferenceContext() can only return values between 0 and 2.
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
  assert(context >= 0 && context <= 2);
  if (is_single) {
    // The index parameter for single references is offset by one since the spec
    // uses 1-based index for these elements.
    return symbol_decoder_context_.single_reference_cdf[context][index - 1];
  }
  if (is_backward) {
    return symbol_decoder_context_
        .compound_backward_reference_cdf[context][index];
  }
  return symbol_decoder_context_.compound_reference_cdf[type][context][index];
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
}

void Tile::ReadReferenceFrames(const Block& block, bool skip_mode) {
  BlockParameters& bp = *block.bp;
  if (skip_mode) {
    bp.reference_frame[0] = frame_header_.skip_mode_frame[0];
    bp.reference_frame[1] = frame_header_.skip_mode_frame[1];
    return;
  }
  if (frame_header_.segmentation.FeatureActive(
          bp.prediction_parameters->segment_id,
          kSegmentFeatureReferenceFrame)) {
    bp.reference_frame[0] = static_cast<ReferenceFrameType>(
        frame_header_.segmentation
            .feature_data[bp.prediction_parameters->segment_id]
                         [kSegmentFeatureReferenceFrame]);
    bp.reference_frame[1] = kReferenceFrameNone;
    return;
  }
  if (frame_header_.segmentation.FeatureActive(
          bp.prediction_parameters->segment_id, kSegmentFeatureSkip) ||
      frame_header_.segmentation.FeatureActive(
          bp.prediction_parameters->segment_id, kSegmentFeatureGlobalMv)) {
    bp.reference_frame[0] = kReferenceFrameLast;
    bp.reference_frame[1] = kReferenceFrameNone;
    return;
  }
  const bool use_compound_reference =
      frame_header_.reference_mode_select &&
      std::min(block.width4x4, block.height4x4) >= 2 &&
      reader_.ReadSymbol(symbol_decoder_context_.use_compound_reference_cdf
                             [GetUseCompoundReferenceContext(block)]);
  if (use_compound_reference) {
    CompoundReferenceType reference_type = ReadCompoundReferenceType(block);
    if (reference_type == kCompoundReferenceUnidirectional) {
      // uni_comp_ref.
      if (reader_.ReadSymbol(
              GetReferenceCdf<false, false, 0>(block, reference_type))) {
        bp.reference_frame[0] = kReferenceFrameBackward;
        bp.reference_frame[1] = kReferenceFrameAlternate;
        return;
      }
      // uni_comp_ref_p1.
      if (!reader_.ReadSymbol(
              GetReferenceCdf<false, false, 1>(block, reference_type))) {
        bp.reference_frame[0] = kReferenceFrameLast;
        bp.reference_frame[1] = kReferenceFrameLast2;
        return;
      }
      // uni_comp_ref_p2.
      if (reader_.ReadSymbol(
              GetReferenceCdf<false, false, 2>(block, reference_type))) {
        bp.reference_frame[0] = kReferenceFrameLast;
        bp.reference_frame[1] = kReferenceFrameGolden;
        return;
      }
      bp.reference_frame[0] = kReferenceFrameLast;
      bp.reference_frame[1] = kReferenceFrameLast3;
      return;
    }
    assert(reference_type == kCompoundReferenceBidirectional);
    // comp_ref.
    if (reader_.ReadSymbol(
            GetReferenceCdf<false, false, 0>(block, reference_type))) {
      // comp_ref_p2.
      bp.reference_frame[0] =
          reader_.ReadSymbol(
              GetReferenceCdf<false, false, 2>(block, reference_type))
              ? kReferenceFrameGolden
              : kReferenceFrameLast3;
    } else {
      // comp_ref_p1.
      bp.reference_frame[0] =
          reader_.ReadSymbol(
              GetReferenceCdf<false, false, 1>(block, reference_type))
              ? kReferenceFrameLast2
              : kReferenceFrameLast;
    }
    // comp_bwdref.
    if (reader_.ReadSymbol(GetReferenceCdf<false, true, 0>(block))) {
      bp.reference_frame[1] = kReferenceFrameAlternate;
    } else {
      // comp_bwdref_p1.
      bp.reference_frame[1] =
          reader_.ReadSymbol(GetReferenceCdf<false, true, 1>(block))
              ? kReferenceFrameAlternate2
              : kReferenceFrameBackward;
    }
    return;
  }
  assert(!use_compound_reference);
  bp.reference_frame[1] = kReferenceFrameNone;
  // single_ref_p1.
  if (reader_.ReadSymbol(GetReferenceCdf<true, false, 1>(block))) {
    // single_ref_p2.
    if (reader_.ReadSymbol(GetReferenceCdf<true, false, 2>(block))) {
      bp.reference_frame[0] = kReferenceFrameAlternate;
      return;
    }
    // single_ref_p6.
    bp.reference_frame[0] =
        reader_.ReadSymbol(GetReferenceCdf<true, false, 6>(block))
            ? kReferenceFrameAlternate2
            : kReferenceFrameBackward;
    return;
  }
  // single_ref_p3.
  if (reader_.ReadSymbol(GetReferenceCdf<true, false, 3>(block))) {
    // single_ref_p5.
    bp.reference_frame[0] =
        reader_.ReadSymbol(GetReferenceCdf<true, false, 5>(block))
            ? kReferenceFrameGolden
            : kReferenceFrameLast3;
    return;
  }
  // single_ref_p4.
  bp.reference_frame[0] =
      reader_.ReadSymbol(GetReferenceCdf<true, false, 4>(block))
          ? kReferenceFrameLast2
          : kReferenceFrameLast;
}

void Tile::ReadInterPredictionModeY(const Block& block,
                                    const MvContexts& mode_contexts,
                                    bool skip_mode) {
  BlockParameters& bp = *block.bp;
  if (skip_mode) {
    bp.y_mode = kPredictionModeNearestNearestMv;
    return;
  }
  if (frame_header_.segmentation.FeatureActive(
          bp.prediction_parameters->segment_id, kSegmentFeatureSkip) ||
      frame_header_.segmentation.FeatureActive(
          bp.prediction_parameters->segment_id, kSegmentFeatureGlobalMv)) {
    bp.y_mode = kPredictionModeGlobalMv;
    return;
  }
  if (bp.reference_frame[1] > kReferenceFrameIntra) {
    const int idx0 = mode_contexts.reference_mv >> 1;
    const int idx1 =
        std::min(mode_contexts.new_mv, kCompoundModeNewMvContexts - 1);
    const int context = kCompoundModeContextMap[idx0][idx1];
    const int offset = reader_.ReadSymbol<kNumCompoundInterPredictionModes>(
        symbol_decoder_context_.compound_prediction_mode_cdf[context]);
    bp.y_mode =
        static_cast<PredictionMode>(kPredictionModeNearestNearestMv + offset);
    return;
  }
  // new_mv.
  if (!reader_.ReadSymbol(
          symbol_decoder_context_.new_mv_cdf[mode_contexts.new_mv])) {
    bp.y_mode = kPredictionModeNewMv;
    return;
  }
  // zero_mv.
  if (!reader_.ReadSymbol(
          symbol_decoder_context_.zero_mv_cdf[mode_contexts.zero_mv])) {
    bp.y_mode = kPredictionModeGlobalMv;
    return;
  }
  // ref_mv.
  bp.y_mode =
      reader_.ReadSymbol(
          symbol_decoder_context_.reference_mv_cdf[mode_contexts.reference_mv])
          ? kPredictionModeNearMv
          : kPredictionModeNearestMv;
}

void Tile::ReadRefMvIndex(const Block& block) {
  BlockParameters& bp = *block.bp;
  PredictionParameters& prediction_parameters =
      *block.bp->prediction_parameters;
  prediction_parameters.ref_mv_index = 0;
  if (bp.y_mode != kPredictionModeNewMv &&
      bp.y_mode != kPredictionModeNewNewMv &&
      !kPredictionModeHasNearMvMask.Contains(bp.y_mode)) {
    return;
  }
  const int start =
      static_cast<int>(kPredictionModeHasNearMvMask.Contains(bp.y_mode));
  prediction_parameters.ref_mv_index = start;
  for (int i = start; i < start + 2; ++i) {
    if (prediction_parameters.ref_mv_count <= i + 1) break;
    // drl_mode in the spec.
    const bool ref_mv_index_bit = reader_.ReadSymbol(
        symbol_decoder_context_.ref_mv_index_cdf[GetRefMvIndexContext(
            prediction_parameters.nearest_mv_count, i)]);
    prediction_parameters.ref_mv_index = i + static_cast<int>(ref_mv_index_bit);
    if (!ref_mv_index_bit) return;
  }
}

void Tile::ReadInterIntraMode(const Block& block, bool is_compound,
                              bool skip_mode) {
  BlockParameters& bp = *block.bp;
  PredictionParameters& prediction_parameters =
      *block.bp->prediction_parameters;
  prediction_parameters.inter_intra_mode = kNumInterIntraModes;
  prediction_parameters.is_wedge_inter_intra = false;
  if (skip_mode || !sequence_header_.enable_interintra_compound ||
      is_compound || !kIsInterIntraModeAllowedMask.Contains(block.size)) {
    return;
  }
  // kSizeGroup[block.size] is guaranteed to be non-zero because of the block
  // size constraint enforced in the above condition.
  assert(kSizeGroup[block.size] - 1 >= 0);
  if (!reader_.ReadSymbol(
          symbol_decoder_context_
              .is_inter_intra_cdf[kSizeGroup[block.size] - 1])) {
    prediction_parameters.inter_intra_mode = kNumInterIntraModes;
    return;
  }
  prediction_parameters.inter_intra_mode =
      static_cast<InterIntraMode>(reader_.ReadSymbol<kNumInterIntraModes>(
          symbol_decoder_context_
              .inter_intra_mode_cdf[kSizeGroup[block.size] - 1]));
  bp.reference_frame[1] = kReferenceFrameIntra;
  prediction_parameters.angle_delta[kPlaneTypeY] = 0;
  prediction_parameters.angle_delta[kPlaneTypeUV] = 0;
  prediction_parameters.use_filter_intra = false;
  prediction_parameters.is_wedge_inter_intra = reader_.ReadSymbol(
      symbol_decoder_context_.is_wedge_inter_intra_cdf[block.size]);
  if (!prediction_parameters.is_wedge_inter_intra) return;
  prediction_parameters.wedge_index =
      reader_.ReadSymbol<kWedgeIndexSymbolCount>(
          symbol_decoder_context_.wedge_index_cdf[block.size]);
  prediction_parameters.wedge_sign = 0;
}

void Tile::ReadMotionMode(const Block& block, bool is_compound,
                          bool skip_mode) {
  BlockParameters& bp = *block.bp;
  PredictionParameters& prediction_parameters =
      *block.bp->prediction_parameters;
  const auto global_motion_type =
      frame_header_.global_motion[bp.reference_frame[0]].type;
  if (skip_mode || !frame_header_.is_motion_mode_switchable ||
      IsBlockDimension4(block.size) ||
      (frame_header_.force_integer_mv == 0 &&
       (bp.y_mode == kPredictionModeGlobalMv ||
        bp.y_mode == kPredictionModeGlobalGlobalMv) &&
       global_motion_type > kGlobalMotionTransformationTypeTranslation) ||
      is_compound || bp.reference_frame[1] == kReferenceFrameIntra ||
      !block.HasOverlappableCandidates()) {
    prediction_parameters.motion_mode = kMotionModeSimple;
    return;
  }
  prediction_parameters.num_warp_samples = 0;
  int num_samples_scanned = 0;
  memset(prediction_parameters.warp_estimate_candidates, 0,
         sizeof(prediction_parameters.warp_estimate_candidates));
  FindWarpSamples(block, &prediction_parameters.num_warp_samples,
                  &num_samples_scanned,
                  prediction_parameters.warp_estimate_candidates);
  if (frame_header_.force_integer_mv != 0 ||
      prediction_parameters.num_warp_samples == 0 ||
      !frame_header_.allow_warped_motion || IsScaled(bp.reference_frame[0])) {
    prediction_parameters.motion_mode =
        reader_.ReadSymbol(symbol_decoder_context_.use_obmc_cdf[block.size])
            ? kMotionModeObmc
            : kMotionModeSimple;
    return;
  }
  prediction_parameters.motion_mode =
      static_cast<MotionMode>(reader_.ReadSymbol<kNumMotionModes>(
          symbol_decoder_context_.motion_mode_cdf[block.size]));
}

uint16_t* Tile::GetIsExplicitCompoundTypeCdf(const Block& block) {
  int context = 0;
  if (block.top_available[kPlaneY]) {
    if (!block.IsTopSingle()) {
      context += static_cast<int>(
          block.top_context
              ->is_explicit_compound_type[block.top_context_index]);
    } else if (block.TopReference(0) == kReferenceFrameAlternate) {
      context += 3;
    }
  }
  if (block.left_available[kPlaneY]) {
    if (!block.IsLeftSingle()) {
      context += static_cast<int>(
          left_context_.is_explicit_compound_type[block.left_context_index]);
    } else if (block.LeftReference(0) == kReferenceFrameAlternate) {
      context += 3;
    }
  }
  return symbol_decoder_context_.is_explicit_compound_type_cdf[std::min(
      context, kIsExplicitCompoundTypeContexts - 1)];
}

uint16_t* Tile::GetIsCompoundTypeAverageCdf(const Block& block) {
  const BlockParameters& bp = *block.bp;
  const ReferenceInfo& reference_info = *current_frame_.reference_info();
  const int forward =
      std::abs(reference_info.relative_distance_from[bp.reference_frame[0]]);
  const int backward =
      std::abs(reference_info.relative_distance_from[bp.reference_frame[1]]);
  int context = (forward == backward) ? 3 : 0;
  if (block.top_available[kPlaneY]) {
    if (!block.IsTopSingle()) {
      context += static_cast<int>(
          block.top_context->is_compound_type_average[block.top_context_index]);
    } else if (block.TopReference(0) == kReferenceFrameAlternate) {
      ++context;
    }
  }
  if (block.left_available[kPlaneY]) {
    if (!block.IsLeftSingle()) {
      context += static_cast<int>(
          left_context_.is_compound_type_average[block.left_context_index]);
    } else if (block.LeftReference(0) == kReferenceFrameAlternate) {
      ++context;
    }
  }
  return symbol_decoder_context_.is_compound_type_average_cdf[context];
}

void Tile::ReadCompoundType(const Block& block, bool is_compound,
                            bool skip_mode,
                            bool* const is_explicit_compound_type,
                            bool* const is_compound_type_average) {
  *is_explicit_compound_type = false;
  *is_compound_type_average = true;
  PredictionParameters& prediction_parameters =
      *block.bp->prediction_parameters;
  if (skip_mode) {
    prediction_parameters.compound_prediction_type =
        kCompoundPredictionTypeAverage;
    return;
  }
  if (is_compound) {
    if (sequence_header_.enable_masked_compound) {
      *is_explicit_compound_type =
          reader_.ReadSymbol(GetIsExplicitCompoundTypeCdf(block));
    }
    if (*is_explicit_compound_type) {
      if (kIsWedgeCompoundModeAllowed.Contains(block.size)) {
        // Only kCompoundPredictionTypeWedge and
        // kCompoundPredictionTypeDiffWeighted are signaled explicitly.
        prediction_parameters.compound_prediction_type =
            static_cast<CompoundPredictionType>(reader_.ReadSymbol(
                symbol_decoder_context_.compound_type_cdf[block.size]));
      } else {
        prediction_parameters.compound_prediction_type =
            kCompoundPredictionTypeDiffWeighted;
      }
    } else {
      if (sequence_header_.enable_jnt_comp) {
        *is_compound_type_average =
            reader_.ReadSymbol(GetIsCompoundTypeAverageCdf(block));
        prediction_parameters.compound_prediction_type =
            *is_compound_type_average ? kCompoundPredictionTypeAverage
                                      : kCompoundPredictionTypeDistance;
      } else {
        prediction_parameters.compound_prediction_type =
            kCompoundPredictionTypeAverage;
        return;
      }
    }
    if (prediction_parameters.compound_prediction_type ==
        kCompoundPredictionTypeWedge) {
      prediction_parameters.wedge_index =
          reader_.ReadSymbol<kWedgeIndexSymbolCount>(
              symbol_decoder_context_.wedge_index_cdf[block.size]);
      prediction_parameters.wedge_sign = static_cast<int>(reader_.ReadBit());
    } else if (prediction_parameters.compound_prediction_type ==
               kCompoundPredictionTypeDiffWeighted) {
      prediction_parameters.mask_is_inverse = reader_.ReadBit() != 0;
    }
    return;
  }
  if (prediction_parameters.inter_intra_mode != kNumInterIntraModes) {
    prediction_parameters.compound_prediction_type =
        prediction_parameters.is_wedge_inter_intra
            ? kCompoundPredictionTypeWedge
            : kCompoundPredictionTypeIntra;
    return;
  }
  prediction_parameters.compound_prediction_type =
      kCompoundPredictionTypeAverage;
}

uint16_t* Tile::GetInterpolationFilterCdf(const Block& block, int direction) {
  const BlockParameters& bp = *block.bp;
  int context = MultiplyBy8(direction) +
                MultiplyBy4(static_cast<int>(bp.reference_frame[1] >
                                             kReferenceFrameIntra));
  int top_type = kNumExplicitInterpolationFilters;
  if (block.top_available[kPlaneY]) {
    if (block.bp_top->reference_frame[0] == bp.reference_frame[0] ||
        block.bp_top->reference_frame[1] == bp.reference_frame[0]) {
      top_type = block.bp_top->interpolation_filter[direction];
    }
  }
  int left_type = kNumExplicitInterpolationFilters;
  if (block.left_available[kPlaneY]) {
    if (block.bp_left->reference_frame[0] == bp.reference_frame[0] ||
        block.bp_left->reference_frame[1] == bp.reference_frame[0]) {
      left_type = block.bp_left->interpolation_filter[direction];
    }
  }
  if (left_type == top_type) {
    context += left_type;
  } else if (left_type == kNumExplicitInterpolationFilters) {
    context += top_type;
  } else if (top_type == kNumExplicitInterpolationFilters) {
    context += left_type;
  } else {
    context += kNumExplicitInterpolationFilters;
  }
  return symbol_decoder_context_.interpolation_filter_cdf[context];
}

void Tile::ReadInterpolationFilter(const Block& block, bool skip_mode) {
  BlockParameters& bp = *block.bp;
  if (frame_header_.interpolation_filter != kInterpolationFilterSwitchable) {
    static_assert(
        sizeof(bp.interpolation_filter) / sizeof(bp.interpolation_filter[0]) ==
            2,
        "Interpolation filter array size is not 2");
    for (auto& interpolation_filter : bp.interpolation_filter) {
      interpolation_filter = frame_header_.interpolation_filter;
    }
    return;
  }
  bool interpolation_filter_present = true;
  if (skip_mode ||
      block.bp->prediction_parameters->motion_mode == kMotionModeLocalWarp) {
    interpolation_filter_present = false;
  } else if (!IsBlockDimension4(block.size) &&
             bp.y_mode == kPredictionModeGlobalMv) {
    interpolation_filter_present =
        frame_header_.global_motion[bp.reference_frame[0]].type ==
        kGlobalMotionTransformationTypeTranslation;
  } else if (!IsBlockDimension4(block.size) &&
             bp.y_mode == kPredictionModeGlobalGlobalMv) {
    interpolation_filter_present =
        frame_header_.global_motion[bp.reference_frame[0]].type ==
            kGlobalMotionTransformationTypeTranslation ||
        frame_header_.global_motion[bp.reference_frame[1]].type ==
            kGlobalMotionTransformationTypeTranslation;
  }
  for (int i = 0; i < (sequence_header_.enable_dual_filter ? 2 : 1); ++i) {
    bp.interpolation_filter[i] =
        interpolation_filter_present
            ? static_cast<InterpolationFilter>(
                  reader_.ReadSymbol<kNumExplicitInterpolationFilters>(
                      GetInterpolationFilterCdf(block, i)))
            : kInterpolationFilterEightTap;
  }
  if (!sequence_header_.enable_dual_filter) {
    bp.interpolation_filter[1] = bp.interpolation_filter[0];
  }
}

void Tile::SetCdfContextCompoundType(const Block& block,
                                     bool is_explicit_compound_type,
                                     bool is_compound_type_average) {
  memset(left_context_.is_explicit_compound_type + block.left_context_index,
         static_cast<int>(is_explicit_compound_type), block.height4x4);
  memset(left_context_.is_compound_type_average + block.left_context_index,
         static_cast<int>(is_compound_type_average), block.height4x4);
  memset(block.top_context->is_explicit_compound_type + block.top_context_index,
         static_cast<int>(is_explicit_compound_type), block.width4x4);
  memset(block.top_context->is_compound_type_average + block.top_context_index,
         static_cast<int>(is_compound_type_average), block.width4x4);
}

bool Tile::ReadInterBlockModeInfo(const Block& block, bool skip_mode) {
  BlockParameters& bp = *block.bp;
  bp.prediction_parameters->palette_mode_info.size[kPlaneTypeY] = 0;
  bp.prediction_parameters->palette_mode_info.size[kPlaneTypeUV] = 0;
  SetCdfContextPaletteSize(block);
  ReadReferenceFrames(block, skip_mode);
  const bool is_compound = bp.reference_frame[1] > kReferenceFrameIntra;
  MvContexts mode_contexts;
  FindMvStack(block, is_compound, &mode_contexts);
  ReadInterPredictionModeY(block, mode_contexts, skip_mode);
  ReadRefMvIndex(block);
  if (!AssignInterMv(block, is_compound)) return false;
  ReadInterIntraMode(block, is_compound, skip_mode);
  ReadMotionMode(block, is_compound, skip_mode);
  bool is_explicit_compound_type;
  bool is_compound_type_average;
  ReadCompoundType(block, is_compound, skip_mode, &is_explicit_compound_type,
                   &is_compound_type_average);
  SetCdfContextCompoundType(block, is_explicit_compound_type,
                            is_compound_type_average);
  ReadInterpolationFilter(block, skip_mode);
  return true;
}

void Tile::SetCdfContextSkipMode(const Block& block, bool skip_mode) {
  memset(left_context_.skip_mode + block.left_context_index,
         static_cast<int>(skip_mode), block.height4x4);
  memset(block.top_context->skip_mode + block.top_context_index,
         static_cast<int>(skip_mode), block.width4x4);
}

bool Tile::DecodeInterModeInfo(const Block& block) {
  BlockParameters& bp = *block.bp;
  block.bp->prediction_parameters->use_intra_block_copy = false;
  bp.skip = false;
  if (!ReadInterSegmentId(block, /*pre_skip=*/true)) return false;
  bool skip_mode = ReadSkipMode(block);
  SetCdfContextSkipMode(block, skip_mode);
  if (skip_mode) {
    bp.skip = true;
  } else {
    ReadSkip(block);
  }
  if (!frame_header_.segmentation.segment_id_pre_skip &&
      !ReadInterSegmentId(block, /*pre_skip=*/false)) {
    return false;
  }
  ReadCdef(block);
  if (read_deltas_) {
    ReadQuantizerIndexDelta(block);
    ReadLoopFilterDelta(block);
    read_deltas_ = false;
  }
  ReadIsInter(block, skip_mode);
  return bp.is_inter ? ReadInterBlockModeInfo(block, skip_mode)
                     : ReadIntraBlockModeInfo(block, /*intra_y_mode=*/false);
}

bool Tile::DecodeModeInfo(const Block& block) {
  return IsIntraFrame(frame_header_.frame_type) ? DecodeIntraModeInfo(block)
                                                : DecodeInterModeInfo(block);
}

}  // namespace libgav1
