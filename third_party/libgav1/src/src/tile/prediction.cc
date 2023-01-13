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
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>

#include "src/buffer_pool.h"
#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/motion_vector.h"
#include "src/obu_parser.h"
#include "src/prediction_mask.h"
#include "src/tile.h"
#include "src/utils/array_2d.h"
#include "src/utils/bit_mask_set.h"
#include "src/utils/block_parameters_holder.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/logging.h"
#include "src/utils/memory.h"
#include "src/utils/types.h"
#include "src/warp_prediction.h"
#include "src/yuv_buffer.h"

namespace libgav1 {
namespace {

// Import all the constants in the anonymous namespace.
#include "src/inter_intra_masks.inc"

// Precision bits when scaling reference frames.
constexpr int kReferenceScaleShift = 14;
constexpr int kAngleStep = 3;
constexpr int kPredictionModeToAngle[kIntraPredictionModesUV] = {
    0, 90, 180, 45, 135, 113, 157, 203, 67, 0, 0, 0, 0};

// The following modes need both the left_column and top_row for intra
// prediction. For directional modes left/top requirement is inferred based on
// the prediction angle. For Dc modes, left/top requirement is inferred based on
// whether or not left/top is available.
constexpr BitMaskSet kNeedsLeftAndTop(kPredictionModeSmooth,
                                      kPredictionModeSmoothHorizontal,
                                      kPredictionModeSmoothVertical,
                                      kPredictionModePaeth);

int16_t GetDirectionalIntraPredictorDerivative(const int angle) {
  assert(angle >= 3);
  assert(angle <= 87);
  return kDirectionalIntraPredictorDerivative[DivideBy2(angle) - 1];
}

// Maps the block_size to an index as follows:
//  kBlock8x8 => 0.
//  kBlock8x16 => 1.
//  kBlock8x32 => 2.
//  kBlock16x8 => 3.
//  kBlock16x16 => 4.
//  kBlock16x32 => 5.
//  kBlock32x8 => 6.
//  kBlock32x16 => 7.
//  kBlock32x32 => 8.
int GetWedgeBlockSizeIndex(BlockSize block_size) {
  assert(block_size >= kBlock8x8);
  return block_size - kBlock8x8 - static_cast<int>(block_size >= kBlock16x8) -
         static_cast<int>(block_size >= kBlock32x8);
}

// Maps a dimension of 4, 8, 16 and 32 to indices 0, 1, 2 and 3 respectively.
int GetInterIntraMaskLookupIndex(int dimension) {
  assert(dimension == 4 || dimension == 8 || dimension == 16 ||
         dimension == 32);
  return FloorLog2(dimension) - 2;
}

// 7.11.2.9.
int GetIntraEdgeFilterStrength(int width, int height, int filter_type,
                               int delta) {
  const int sum = width + height;
  delta = std::abs(delta);
  if (filter_type == 0) {
    if (sum <= 8) {
      if (delta >= 56) return 1;
    } else if (sum <= 16) {
      if (delta >= 40) return 1;
    } else if (sum <= 24) {
      if (delta >= 32) return 3;
      if (delta >= 16) return 2;
      if (delta >= 8) return 1;
    } else if (sum <= 32) {
      if (delta >= 32) return 3;
      if (delta >= 4) return 2;
      return 1;
    } else {
      return 3;
    }
  } else {
    if (sum <= 8) {
      if (delta >= 64) return 2;
      if (delta >= 40) return 1;
    } else if (sum <= 16) {
      if (delta >= 48) return 2;
      if (delta >= 20) return 1;
    } else if (sum <= 24) {
      if (delta >= 4) return 3;
    } else {
      return 3;
    }
  }
  return 0;
}

// 7.11.2.10.
bool DoIntraEdgeUpsampling(int width, int height, int filter_type, int delta) {
  const int sum = width + height;
  delta = std::abs(delta);
  // This function should not be called when the prediction angle is 90 or 180.
  assert(delta != 0);
  if (delta >= 40) return false;
  return (filter_type == 1) ? sum <= 8 : sum <= 16;
}

constexpr uint8_t kQuantizedDistanceWeight[4][2] = {
    {2, 3}, {2, 5}, {2, 7}, {1, kMaxFrameDistance}};

constexpr uint8_t kQuantizedDistanceLookup[4][2] = {
    {9, 7}, {11, 5}, {12, 4}, {13, 3}};

void GetDistanceWeights(const int distance[2], int weight[2]) {
  // Note: distance[0] and distance[1] correspond to relative distance
  // between current frame and reference frame [1] and [0], respectively.
  const int order = static_cast<int>(distance[0] <= distance[1]);
  if (distance[0] == 0 || distance[1] == 0) {
    weight[0] = kQuantizedDistanceLookup[3][order];
    weight[1] = kQuantizedDistanceLookup[3][1 - order];
  } else {
    int i;
    for (i = 0; i < 3; ++i) {
      const int weight_0 = kQuantizedDistanceWeight[i][order];
      const int weight_1 = kQuantizedDistanceWeight[i][1 - order];
      if (order == 0) {
        if (distance[0] * weight_0 < distance[1] * weight_1) break;
      } else {
        if (distance[0] * weight_0 > distance[1] * weight_1) break;
      }
    }
    weight[0] = kQuantizedDistanceLookup[i][order];
    weight[1] = kQuantizedDistanceLookup[i][1 - order];
  }
}

dsp::IntraPredictor GetIntraPredictor(PredictionMode mode, bool has_left,
                                      bool has_top) {
  if (mode == kPredictionModeDc) {
    if (has_left && has_top) {
      return dsp::kIntraPredictorDc;
    }
    if (has_left) {
      return dsp::kIntraPredictorDcLeft;
    }
    if (has_top) {
      return dsp::kIntraPredictorDcTop;
    }
    return dsp::kIntraPredictorDcFill;
  }
  switch (mode) {
    case kPredictionModePaeth:
      return dsp::kIntraPredictorPaeth;
    case kPredictionModeSmooth:
      return dsp::kIntraPredictorSmooth;
    case kPredictionModeSmoothVertical:
      return dsp::kIntraPredictorSmoothVertical;
    case kPredictionModeSmoothHorizontal:
      return dsp::kIntraPredictorSmoothHorizontal;
    default:
      return dsp::kNumIntraPredictors;
  }
}

uint8_t* GetStartPoint(Array2DView<uint8_t>* const buffer, const int plane,
                       const int x, const int y, const int bitdepth) {
#if LIBGAV1_MAX_BITDEPTH >= 10
  if (bitdepth > 8) {
    Array2DView<uint16_t> buffer16(
        buffer[plane].rows(), buffer[plane].columns() / sizeof(uint16_t),
        reinterpret_cast<uint16_t*>(&buffer[plane][0][0]));
    return reinterpret_cast<uint8_t*>(&buffer16[y][x]);
  }
#endif  // LIBGAV1_MAX_BITDEPTH >= 10
  static_cast<void>(bitdepth);
  return &buffer[plane][y][x];
}

int GetPixelPositionFromHighScale(int start, int step, int offset) {
  return (start + step * offset) >> kScaleSubPixelBits;
}

dsp::MaskBlendFunc GetMaskBlendFunc(const dsp::Dsp& dsp, bool is_inter_intra,
                                    bool is_wedge_inter_intra,
                                    int subsampling_x, int subsampling_y) {
  return (is_inter_intra && !is_wedge_inter_intra)
             ? dsp.mask_blend[0][/*is_inter_intra=*/true]
             : dsp.mask_blend[subsampling_x + subsampling_y][is_inter_intra];
}

}  // namespace

template <typename Pixel>
void Tile::IntraPrediction(const Block& block, Plane plane, int x, int y,
                           bool has_left, bool has_top, bool has_top_right,
                           bool has_bottom_left, PredictionMode mode,
                           TransformSize tx_size) {
  const int width = kTransformWidth[tx_size];
  const int height = kTransformHeight[tx_size];
  const int x_shift = subsampling_x_[plane];
  const int y_shift = subsampling_y_[plane];
  const int max_x = (MultiplyBy4(frame_header_.columns4x4) >> x_shift) - 1;
  const int max_y = (MultiplyBy4(frame_header_.rows4x4) >> y_shift) - 1;
  // For performance reasons, do not initialize the following two buffers.
  alignas(kMaxAlignment) Pixel top_row_data[160];
  alignas(kMaxAlignment) Pixel left_column_data[160];
#if LIBGAV1_MSAN
  if (IsDirectionalMode(mode)) {
    memset(top_row_data, 0, sizeof(top_row_data));
    memset(left_column_data, 0, sizeof(left_column_data));
  }
#endif
  // Some predictors use |top_row_data| and |left_column_data| with a negative
  // offset to access pixels to the top-left of the current block. So have some
  // space before the arrays to allow populating those without having to move
  // the rest of the array.
  Pixel* const top_row = top_row_data + 16;
  Pixel* const left_column = left_column_data + 16;
  const int bitdepth = sequence_header_.color_config.bitdepth;
  const int top_and_left_size = width + height;
  const bool is_directional_mode = IsDirectionalMode(mode);
  const PredictionParameters& prediction_parameters =
      *block.bp->prediction_parameters;
  const bool use_filter_intra =
      (plane == kPlaneY && prediction_parameters.use_filter_intra);
  const int prediction_angle =
      is_directional_mode
          ? kPredictionModeToAngle[mode] +
                prediction_parameters.angle_delta[GetPlaneType(plane)] *
                    kAngleStep
          : 0;
  // Directional prediction requires buffers larger than the width or height.
  const int top_size = is_directional_mode ? top_and_left_size : width;
  const int left_size = is_directional_mode ? top_and_left_size : height;
  const int top_right_size =
      is_directional_mode ? (has_top_right ? 2 : 1) * width : width;
  const int bottom_left_size =
      is_directional_mode ? (has_bottom_left ? 2 : 1) * height : height;

  Array2DView<Pixel> buffer(buffer_[plane].rows(),
                            buffer_[plane].columns() / sizeof(Pixel),
                            reinterpret_cast<Pixel*>(&buffer_[plane][0][0]));
  const bool needs_top = use_filter_intra || kNeedsLeftAndTop.Contains(mode) ||
                         (is_directional_mode && prediction_angle < 180) ||
                         (mode == kPredictionModeDc && has_top);
  const bool needs_left = use_filter_intra || kNeedsLeftAndTop.Contains(mode) ||
                          (is_directional_mode && prediction_angle > 90) ||
                          (mode == kPredictionModeDc && has_left);

  const Pixel* top_row_src = buffer[y - 1];

  // Determine if we need to retrieve the top row from
  // |intra_prediction_buffer_|.
  if ((needs_top || needs_left) && use_intra_prediction_buffer_) {
    // Superblock index of block.row4x4. block.row4x4 is always in luma
    // dimension (no subsampling).
    const int current_superblock_index =
        block.row4x4 >> (sequence_header_.use_128x128_superblock ? 5 : 4);
    // Superblock index of y - 1. y is in the plane dimension (chroma planes
    // could be subsampled).
    const int plane_shift = (sequence_header_.use_128x128_superblock ? 7 : 6) -
                            subsampling_y_[plane];
    const int top_row_superblock_index = (y - 1) >> plane_shift;
    // If the superblock index of y - 1 is not that of the current superblock,
    // then we will have to retrieve the top row from the
    // |intra_prediction_buffer_|.
    if (current_superblock_index != top_row_superblock_index) {
      top_row_src = reinterpret_cast<const Pixel*>(
          (*intra_prediction_buffer_)[plane].get());
    }
  }

  if (needs_top) {
    // Compute top_row.
    if (has_top || has_left) {
      const int left_index = has_left ? x - 1 : x;
      top_row[-1] = has_top ? top_row_src[left_index] : buffer[y][left_index];
    } else {
      top_row[-1] = 1 << (bitdepth - 1);
    }
    if (!has_top && has_left) {
      Memset(top_row, buffer[y][x - 1], top_size);
    } else if (!has_top && !has_left) {
      Memset(top_row, (1 << (bitdepth - 1)) - 1, top_size);
    } else {
      const int top_limit = std::min(max_x - x + 1, top_right_size);
      memcpy(top_row, &top_row_src[x], top_limit * sizeof(Pixel));
      // Even though it is safe to call Memset with a size of 0, accessing
      // top_row_src[top_limit - x + 1] is not allowed when this condition is
      // false.
      if (top_size - top_limit > 0) {
        Memset(top_row + top_limit, top_row_src[top_limit + x - 1],
               top_size - top_limit);
      }
    }
  }
  if (needs_left) {
    // Compute left_column.
    if (has_top || has_left) {
      const int left_index = has_left ? x - 1 : x;
      left_column[-1] =
          has_top ? top_row_src[left_index] : buffer[y][left_index];
    } else {
      left_column[-1] = 1 << (bitdepth - 1);
    }
    if (!has_left && has_top) {
      Memset(left_column, top_row_src[x], left_size);
    } else if (!has_left && !has_top) {
      Memset(left_column, (1 << (bitdepth - 1)) + 1, left_size);
    } else {
      const int left_limit = std::min(max_y - y + 1, bottom_left_size);
      for (int i = 0; i < left_limit; ++i) {
        left_column[i] = buffer[y + i][x - 1];
      }
      // Even though it is safe to call Memset with a size of 0, accessing
      // buffer[left_limit - y + 1][x - 1] is not allowed when this condition is
      // false.
      if (left_size - left_limit > 0) {
        Memset(left_column + left_limit, buffer[left_limit + y - 1][x - 1],
               left_size - left_limit);
      }
    }
  }
  Pixel* const dest = &buffer[y][x];
  const ptrdiff_t dest_stride = buffer_[plane].columns();
  if (use_filter_intra) {
    dsp_.filter_intra_predictor(dest, dest_stride, top_row, left_column,
                                prediction_parameters.filter_intra_mode, width,
                                height);
  } else if (is_directional_mode) {
    DirectionalPrediction(block, plane, x, y, has_left, has_top, needs_left,
                          needs_top, prediction_angle, width, height, max_x,
                          max_y, tx_size, top_row, left_column);
  } else {
    const dsp::IntraPredictor predictor =
        GetIntraPredictor(mode, has_left, has_top);
    assert(predictor != dsp::kNumIntraPredictors);
    dsp_.intra_predictors[tx_size][predictor](dest, dest_stride, top_row,
                                              left_column);
  }
}

template void Tile::IntraPrediction<uint8_t>(const Block& block, Plane plane,
                                             int x, int y, bool has_left,
                                             bool has_top, bool has_top_right,
                                             bool has_bottom_left,
                                             PredictionMode mode,
                                             TransformSize tx_size);
#if LIBGAV1_MAX_BITDEPTH >= 10
template void Tile::IntraPrediction<uint16_t>(const Block& block, Plane plane,
                                              int x, int y, bool has_left,
                                              bool has_top, bool has_top_right,
                                              bool has_bottom_left,
                                              PredictionMode mode,
                                              TransformSize tx_size);
#endif

int Tile::GetIntraEdgeFilterType(const Block& block, Plane plane) const {
  bool top;
  bool left;
  if (plane == kPlaneY) {
    top = block.top_available[kPlaneY] &&
          kPredictionModeSmoothMask.Contains(block.bp_top->y_mode);
    left = block.left_available[kPlaneY] &&
           kPredictionModeSmoothMask.Contains(block.bp_left->y_mode);
  } else {
    top = block.top_available[plane] &&
          block.bp->prediction_parameters->chroma_top_uses_smooth_prediction;
    left = block.left_available[plane] &&
           block.bp->prediction_parameters->chroma_left_uses_smooth_prediction;
  }
  return static_cast<int>(top || left);
}

template <typename Pixel>
void Tile::DirectionalPrediction(const Block& block, Plane plane, int x, int y,
                                 bool has_left, bool has_top, bool needs_left,
                                 bool needs_top, int prediction_angle,
                                 int width, int height, int max_x, int max_y,
                                 TransformSize tx_size, Pixel* const top_row,
                                 Pixel* const left_column) {
  Array2DView<Pixel> buffer(buffer_[plane].rows(),
                            buffer_[plane].columns() / sizeof(Pixel),
                            reinterpret_cast<Pixel*>(&buffer_[plane][0][0]));
  Pixel* const dest = &buffer[y][x];
  const ptrdiff_t stride = buffer_[plane].columns();
  if (prediction_angle == 90) {
    dsp_.intra_predictors[tx_size][dsp::kIntraPredictorVertical](
        dest, stride, top_row, left_column);
    return;
  }
  if (prediction_angle == 180) {
    dsp_.intra_predictors[tx_size][dsp::kIntraPredictorHorizontal](
        dest, stride, top_row, left_column);
    return;
  }

  bool upsampled_top = false;
  bool upsampled_left = false;
  if (sequence_header_.enable_intra_edge_filter) {
    const int filter_type = GetIntraEdgeFilterType(block, plane);
    if (prediction_angle > 90 && prediction_angle < 180 &&
        (width + height) >= 24) {
      // 7.11.2.7.
      left_column[-1] = top_row[-1] = RightShiftWithRounding(
          left_column[0] * 5 + top_row[-1] * 6 + top_row[0] * 5, 4);
    }
    if (has_top && needs_top) {
      const int strength = GetIntraEdgeFilterStrength(
          width, height, filter_type, prediction_angle - 90);
      if (strength > 0) {
        const int num_pixels = std::min(width, max_x - x + 1) +
                               ((prediction_angle < 90) ? height : 0) + 1;
        dsp_.intra_edge_filter(top_row - 1, num_pixels, strength);
      }
    }
    if (has_left && needs_left) {
      const int strength = GetIntraEdgeFilterStrength(
          width, height, filter_type, prediction_angle - 180);
      if (strength > 0) {
        const int num_pixels = std::min(height, max_y - y + 1) +
                               ((prediction_angle > 180) ? width : 0) + 1;
        dsp_.intra_edge_filter(left_column - 1, num_pixels, strength);
      }
    }
    upsampled_top = DoIntraEdgeUpsampling(width, height, filter_type,
                                          prediction_angle - 90);
    if (upsampled_top && needs_top) {
      const int num_pixels = width + ((prediction_angle < 90) ? height : 0);
      dsp_.intra_edge_upsampler(top_row, num_pixels);
    }
    upsampled_left = DoIntraEdgeUpsampling(width, height, filter_type,
                                           prediction_angle - 180);
    if (upsampled_left && needs_left) {
      const int num_pixels = height + ((prediction_angle > 180) ? width : 0);
      dsp_.intra_edge_upsampler(left_column, num_pixels);
    }
  }

  if (prediction_angle < 90) {
    const int dx = GetDirectionalIntraPredictorDerivative(prediction_angle);
    dsp_.directional_intra_predictor_zone1(dest, stride, top_row, width, height,
                                           dx, upsampled_top);
  } else if (prediction_angle < 180) {
    const int dx =
        GetDirectionalIntraPredictorDerivative(180 - prediction_angle);
    const int dy =
        GetDirectionalIntraPredictorDerivative(prediction_angle - 90);
    dsp_.directional_intra_predictor_zone2(dest, stride, top_row, left_column,
                                           width, height, dx, dy, upsampled_top,
                                           upsampled_left);
  } else {
    assert(prediction_angle < 270);
    const int dy =
        GetDirectionalIntraPredictorDerivative(270 - prediction_angle);
    dsp_.directional_intra_predictor_zone3(dest, stride, left_column, width,
                                           height, dy, upsampled_left);
  }
}

template <typename Pixel>
void Tile::PalettePrediction(const Block& block, const Plane plane,
                             const int start_x, const int start_y, const int x,
                             const int y, const TransformSize tx_size) {
  const int tx_width = kTransformWidth[tx_size];
  const int tx_height = kTransformHeight[tx_size];
  const uint16_t* const palette =
      block.bp->prediction_parameters->palette_mode_info.color[plane];
  const PlaneType plane_type = GetPlaneType(plane);
  const int x4 = MultiplyBy4(x);
  const int y4 = MultiplyBy4(y);
  Array2DView<Pixel> buffer(buffer_[plane].rows(),
                            buffer_[plane].columns() / sizeof(Pixel),
                            reinterpret_cast<Pixel*>(&buffer_[plane][0][0]));
  for (int row = 0; row < tx_height; ++row) {
    assert(block.bp->prediction_parameters
               ->color_index_map[plane_type][y4 + row] != nullptr);
    for (int column = 0; column < tx_width; ++column) {
      buffer[start_y + row][start_x + column] =
          palette[block.bp->prediction_parameters
                      ->color_index_map[plane_type][y4 + row][x4 + column]];
    }
  }
}

template void Tile::PalettePrediction<uint8_t>(
    const Block& block, const Plane plane, const int start_x, const int start_y,
    const int x, const int y, const TransformSize tx_size);
#if LIBGAV1_MAX_BITDEPTH >= 10
template void Tile::PalettePrediction<uint16_t>(
    const Block& block, const Plane plane, const int start_x, const int start_y,
    const int x, const int y, const TransformSize tx_size);
#endif

template <typename Pixel>
void Tile::ChromaFromLumaPrediction(const Block& block, const Plane plane,
                                    const int start_x, const int start_y,
                                    const TransformSize tx_size) {
  const int subsampling_x = subsampling_x_[plane];
  const int subsampling_y = subsampling_y_[plane];
  const PredictionParameters& prediction_parameters =
      *block.bp->prediction_parameters;
  Array2DView<Pixel> y_buffer(
      buffer_[kPlaneY].rows(), buffer_[kPlaneY].columns() / sizeof(Pixel),
      reinterpret_cast<Pixel*>(&buffer_[kPlaneY][0][0]));
  if (!block.scratch_buffer->cfl_luma_buffer_valid) {
    const int luma_x = start_x << subsampling_x;
    const int luma_y = start_y << subsampling_y;
    dsp_.cfl_subsamplers[tx_size][subsampling_x + subsampling_y](
        block.scratch_buffer->cfl_luma_buffer,
        prediction_parameters.max_luma_width - luma_x,
        prediction_parameters.max_luma_height - luma_y,
        reinterpret_cast<uint8_t*>(&y_buffer[luma_y][luma_x]),
        buffer_[kPlaneY].columns());
    block.scratch_buffer->cfl_luma_buffer_valid = true;
  }
  Array2DView<Pixel> buffer(buffer_[plane].rows(),
                            buffer_[plane].columns() / sizeof(Pixel),
                            reinterpret_cast<Pixel*>(&buffer_[plane][0][0]));
  dsp_.cfl_intra_predictors[tx_size](
      reinterpret_cast<uint8_t*>(&buffer[start_y][start_x]),
      buffer_[plane].columns(), block.scratch_buffer->cfl_luma_buffer,
      (plane == kPlaneU) ? prediction_parameters.cfl_alpha_u
                         : prediction_parameters.cfl_alpha_v);
}

template void Tile::ChromaFromLumaPrediction<uint8_t>(
    const Block& block, const Plane plane, const int start_x, const int start_y,
    const TransformSize tx_size);
#if LIBGAV1_MAX_BITDEPTH >= 10
template void Tile::ChromaFromLumaPrediction<uint16_t>(
    const Block& block, const Plane plane, const int start_x, const int start_y,
    const TransformSize tx_size);
#endif

void Tile::InterIntraPrediction(
    uint16_t* const prediction_0, const uint8_t* const prediction_mask,
    const ptrdiff_t prediction_mask_stride,
    const PredictionParameters& prediction_parameters,
    const int prediction_width, const int prediction_height,
    const int subsampling_x, const int subsampling_y, uint8_t* const dest,
    const ptrdiff_t dest_stride) {
  assert(prediction_mask != nullptr);
  assert(prediction_parameters.compound_prediction_type ==
             kCompoundPredictionTypeIntra ||
         prediction_parameters.compound_prediction_type ==
             kCompoundPredictionTypeWedge);
  // The first buffer of InterIntra is from inter prediction.
  // The second buffer is from intra prediction.
#if LIBGAV1_MAX_BITDEPTH >= 10
  if (sequence_header_.color_config.bitdepth > 8) {
    GetMaskBlendFunc(dsp_, /*is_inter_intra=*/true,
                     prediction_parameters.is_wedge_inter_intra, subsampling_x,
                     subsampling_y)(
        prediction_0, reinterpret_cast<uint16_t*>(dest),
        dest_stride / sizeof(uint16_t), prediction_mask, prediction_mask_stride,
        prediction_width, prediction_height, dest, dest_stride);
    return;
  }
#endif
  const int function_index = prediction_parameters.is_wedge_inter_intra
                                 ? subsampling_x + subsampling_y
                                 : 0;
  // |is_inter_intra| prediction values are stored in a Pixel buffer but it is
  // currently declared as a uint16_t buffer.
  // TODO(johannkoenig): convert the prediction buffer to a uint8_t buffer and
  // remove the reinterpret_cast.
  dsp_.inter_intra_mask_blend_8bpp[function_index](
      reinterpret_cast<uint8_t*>(prediction_0), dest, dest_stride,
      prediction_mask, prediction_mask_stride, prediction_width,
      prediction_height);
}

void Tile::CompoundInterPrediction(
    const Block& block, const uint8_t* const prediction_mask,
    const ptrdiff_t prediction_mask_stride, const int prediction_width,
    const int prediction_height, const int subsampling_x,
    const int subsampling_y, const int candidate_row,
    const int candidate_column, uint8_t* dest, const ptrdiff_t dest_stride) {
  const PredictionParameters& prediction_parameters =
      *block.bp->prediction_parameters;

  void* prediction[2];
#if LIBGAV1_MAX_BITDEPTH >= 10
  const int bitdepth = sequence_header_.color_config.bitdepth;
  if (bitdepth > 8) {
    prediction[0] = block.scratch_buffer->prediction_buffer[0];
    prediction[1] = block.scratch_buffer->prediction_buffer[1];
  } else {
#endif
    prediction[0] = block.scratch_buffer->compound_prediction_buffer_8bpp[0];
    prediction[1] = block.scratch_buffer->compound_prediction_buffer_8bpp[1];
#if LIBGAV1_MAX_BITDEPTH >= 10
  }
#endif

  switch (prediction_parameters.compound_prediction_type) {
    case kCompoundPredictionTypeWedge:
    case kCompoundPredictionTypeDiffWeighted:
      GetMaskBlendFunc(dsp_, /*is_inter_intra=*/false,
                       prediction_parameters.is_wedge_inter_intra,
                       subsampling_x, subsampling_y)(
          prediction[0], prediction[1],
          /*prediction_stride=*/prediction_width, prediction_mask,
          prediction_mask_stride, prediction_width, prediction_height, dest,
          dest_stride);
      break;
    case kCompoundPredictionTypeDistance:
      DistanceWeightedPrediction(prediction[0], prediction[1], prediction_width,
                                 prediction_height, candidate_row,
                                 candidate_column, dest, dest_stride);
      break;
    default:
      assert(prediction_parameters.compound_prediction_type ==
             kCompoundPredictionTypeAverage);
      dsp_.average_blend(prediction[0], prediction[1], prediction_width,
                         prediction_height, dest, dest_stride);
      break;
  }
}

GlobalMotion* Tile::GetWarpParams(
    const Block& block, const Plane plane, const int prediction_width,
    const int prediction_height,
    const PredictionParameters& prediction_parameters,
    const ReferenceFrameType reference_type, bool* const is_local_valid,
    GlobalMotion* const global_motion_params,
    GlobalMotion* const local_warp_params) const {
  if (prediction_width < 8 || prediction_height < 8 ||
      frame_header_.force_integer_mv == 1) {
    return nullptr;
  }
  if (plane == kPlaneY) {
    *is_local_valid =
        prediction_parameters.motion_mode == kMotionModeLocalWarp &&
        WarpEstimation(
            prediction_parameters.num_warp_samples, DivideBy4(prediction_width),
            DivideBy4(prediction_height), block.row4x4, block.column4x4,
            block.bp->mv.mv[0], prediction_parameters.warp_estimate_candidates,
            local_warp_params) &&
        SetupShear(local_warp_params);
  }
  if (prediction_parameters.motion_mode == kMotionModeLocalWarp &&
      *is_local_valid) {
    return local_warp_params;
  }
  if (!IsScaled(reference_type)) {
    GlobalMotionTransformationType global_motion_type =
        (reference_type != kReferenceFrameIntra)
            ? global_motion_params->type
            : kNumGlobalMotionTransformationTypes;
    const bool is_global_valid =
        IsGlobalMvBlock(*block.bp, global_motion_type) &&
        SetupShear(global_motion_params);
    // Valid global motion type implies reference type can't be intra.
    assert(!is_global_valid || reference_type != kReferenceFrameIntra);
    if (is_global_valid) return global_motion_params;
  }
  return nullptr;
}

bool Tile::InterPrediction(const Block& block, const Plane plane, const int x,
                           const int y, const int prediction_width,
                           const int prediction_height, int candidate_row,
                           int candidate_column, bool* const is_local_valid,
                           GlobalMotion* const local_warp_params) {
  const int bitdepth = sequence_header_.color_config.bitdepth;
  const BlockParameters& bp = *block.bp;
  const BlockParameters& bp_reference =
      *block_parameters_holder_.Find(candidate_row, candidate_column);
  const bool is_compound =
      bp_reference.reference_frame[1] > kReferenceFrameIntra;
  assert(bp.is_inter);
  const bool is_inter_intra = bp.reference_frame[1] == kReferenceFrameIntra;

  const PredictionParameters& prediction_parameters =
      *block.bp->prediction_parameters;
  uint8_t* const dest = GetStartPoint(buffer_, plane, x, y, bitdepth);
  const ptrdiff_t dest_stride = buffer_[plane].columns();  // In bytes.
  for (int index = 0; index < 1 + static_cast<int>(is_compound); ++index) {
    const ReferenceFrameType reference_type =
        bp_reference.reference_frame[index];
    GlobalMotion global_motion_params =
        frame_header_.global_motion[reference_type];
    GlobalMotion* warp_params =
        GetWarpParams(block, plane, prediction_width, prediction_height,
                      prediction_parameters, reference_type, is_local_valid,
                      &global_motion_params, local_warp_params);
    if (warp_params != nullptr) {
      if (!BlockWarpProcess(block, plane, index, x, y, prediction_width,
                            prediction_height, warp_params, is_compound,
                            is_inter_intra, dest, dest_stride)) {
        return false;
      }
    } else {
      const int reference_index =
          prediction_parameters.use_intra_block_copy
              ? -1
              : frame_header_.reference_frame_index[reference_type -
                                                    kReferenceFrameLast];
      if (!BlockInterPrediction(
              block, plane, reference_index, bp_reference.mv.mv[index], x, y,
              prediction_width, prediction_height, candidate_row,
              candidate_column, block.scratch_buffer->prediction_buffer[index],
              is_compound, is_inter_intra, dest, dest_stride)) {
        return false;
      }
    }
  }

  const int subsampling_x = subsampling_x_[plane];
  const int subsampling_y = subsampling_y_[plane];
  ptrdiff_t prediction_mask_stride = 0;
  const uint8_t* prediction_mask = nullptr;
  if (prediction_parameters.compound_prediction_type ==
      kCompoundPredictionTypeWedge) {
    const Array2D<uint8_t>& wedge_mask =
        wedge_masks_[GetWedgeBlockSizeIndex(block.size)]
                    [prediction_parameters.wedge_sign]
                    [prediction_parameters.wedge_index];
    prediction_mask = wedge_mask[0];
    prediction_mask_stride = wedge_mask.columns();
  } else if (prediction_parameters.compound_prediction_type ==
             kCompoundPredictionTypeIntra) {
    // 7.11.3.13. The inter intra masks are precomputed and stored as a set of
    // look up tables.
    assert(prediction_parameters.inter_intra_mode < kNumInterIntraModes);
    prediction_mask =
        kInterIntraMasks[prediction_parameters.inter_intra_mode]
                        [GetInterIntraMaskLookupIndex(prediction_width)]
                        [GetInterIntraMaskLookupIndex(prediction_height)];
    prediction_mask_stride = prediction_width;
  } else if (prediction_parameters.compound_prediction_type ==
             kCompoundPredictionTypeDiffWeighted) {
    if (plane == kPlaneY) {
      assert(prediction_width >= 8);
      assert(prediction_height >= 8);
      dsp_.weight_mask[FloorLog2(prediction_width) - 3]
                      [FloorLog2(prediction_height) - 3]
                      [static_cast<int>(prediction_parameters.mask_is_inverse)](
                          block.scratch_buffer->prediction_buffer[0],
                          block.scratch_buffer->prediction_buffer[1],
                          block.scratch_buffer->weight_mask, block.width);
    }
    prediction_mask = block.scratch_buffer->weight_mask;
    prediction_mask_stride = block.width;
  }

  if (is_compound) {
    CompoundInterPrediction(block, prediction_mask, prediction_mask_stride,
                            prediction_width, prediction_height, subsampling_x,
                            subsampling_y, candidate_row, candidate_column,
                            dest, dest_stride);
  } else if (prediction_parameters.motion_mode == kMotionModeObmc) {
    // Obmc mode is allowed only for single reference (!is_compound).
    return ObmcPrediction(block, plane, prediction_width, prediction_height);
  } else if (is_inter_intra) {
    // InterIntra and obmc must be mutually exclusive.
    InterIntraPrediction(
        block.scratch_buffer->prediction_buffer[0], prediction_mask,
        prediction_mask_stride, prediction_parameters, prediction_width,
        prediction_height, subsampling_x, subsampling_y, dest, dest_stride);
  }
  return true;
}

bool Tile::ObmcBlockPrediction(const Block& block, const MotionVector& mv,
                               const Plane plane,
                               const int reference_frame_index, const int width,
                               const int height, const int x, const int y,
                               const int candidate_row,
                               const int candidate_column,
                               const ObmcDirection blending_direction) {
  const int bitdepth = sequence_header_.color_config.bitdepth;
  // Obmc's prediction needs to be clipped before blending with above/left
  // prediction blocks.
  // Obmc prediction is used only when is_compound is false. So it is safe to
  // use prediction_buffer[1] as a temporary buffer for the Obmc prediction.
  static_assert(sizeof(block.scratch_buffer->prediction_buffer[1]) >=
                    64 * 64 * sizeof(uint16_t),
                "");
  auto* const obmc_buffer =
      reinterpret_cast<uint8_t*>(block.scratch_buffer->prediction_buffer[1]);
  const ptrdiff_t obmc_buffer_stride =
      (bitdepth == 8) ? width : width * sizeof(uint16_t);
  if (!BlockInterPrediction(block, plane, reference_frame_index, mv, x, y,
                            width, height, candidate_row, candidate_column,
                            nullptr, false, false, obmc_buffer,
                            obmc_buffer_stride)) {
    return false;
  }

  uint8_t* const prediction = GetStartPoint(buffer_, plane, x, y, bitdepth);
  const ptrdiff_t prediction_stride = buffer_[plane].columns();
  dsp_.obmc_blend[blending_direction](prediction, prediction_stride, width,
                                      height, obmc_buffer, obmc_buffer_stride);
  return true;
}

bool Tile::ObmcPrediction(const Block& block, const Plane plane,
                          const int width, const int height) {
  const int subsampling_x = subsampling_x_[plane];
  const int subsampling_y = subsampling_y_[plane];
  if (block.top_available[kPlaneY] &&
      !IsBlockSmallerThan8x8(block.residual_size[plane])) {
    const int num_limit = std::min(uint8_t{4}, k4x4WidthLog2[block.size]);
    const int column4x4_max =
        std::min(block.column4x4 + block.width4x4, frame_header_.columns4x4);
    const int candidate_row = block.row4x4 - 1;
    const int block_start_y = MultiplyBy4(block.row4x4) >> subsampling_y;
    int column4x4 = block.column4x4;
    const int prediction_height = std::min(height >> 1, 32 >> subsampling_y);
    for (int i = 0, step; i < num_limit && column4x4 < column4x4_max;
         column4x4 += step) {
      const int candidate_column = column4x4 | 1;
      const BlockParameters& bp_top =
          *block_parameters_holder_.Find(candidate_row, candidate_column);
      const int candidate_block_size = bp_top.size;
      step = Clip3(kNum4x4BlocksWide[candidate_block_size], 2, 16);
      if (bp_top.reference_frame[0] > kReferenceFrameIntra) {
        i++;
        const int candidate_reference_frame_index =
            frame_header_.reference_frame_index[bp_top.reference_frame[0] -
                                                kReferenceFrameLast];
        const int prediction_width =
            std::min(width, MultiplyBy4(step) >> subsampling_x);
        if (!ObmcBlockPrediction(
                block, bp_top.mv.mv[0], plane, candidate_reference_frame_index,
                prediction_width, prediction_height,
                MultiplyBy4(column4x4) >> subsampling_x, block_start_y,
                candidate_row, candidate_column, kObmcDirectionVertical)) {
          return false;
        }
      }
    }
  }

  if (block.left_available[kPlaneY]) {
    const int num_limit = std::min(uint8_t{4}, k4x4HeightLog2[block.size]);
    const int row4x4_max =
        std::min(block.row4x4 + block.height4x4, frame_header_.rows4x4);
    const int candidate_column = block.column4x4 - 1;
    int row4x4 = block.row4x4;
    const int block_start_x = MultiplyBy4(block.column4x4) >> subsampling_x;
    const int prediction_width = std::min(width >> 1, 32 >> subsampling_x);
    for (int i = 0, step; i < num_limit && row4x4 < row4x4_max;
         row4x4 += step) {
      const int candidate_row = row4x4 | 1;
      const BlockParameters& bp_left =
          *block_parameters_holder_.Find(candidate_row, candidate_column);
      const int candidate_block_size = bp_left.size;
      step = Clip3(kNum4x4BlocksHigh[candidate_block_size], 2, 16);
      if (bp_left.reference_frame[0] > kReferenceFrameIntra) {
        i++;
        const int candidate_reference_frame_index =
            frame_header_.reference_frame_index[bp_left.reference_frame[0] -
                                                kReferenceFrameLast];
        const int prediction_height =
            std::min(height, MultiplyBy4(step) >> subsampling_y);
        if (!ObmcBlockPrediction(
                block, bp_left.mv.mv[0], plane, candidate_reference_frame_index,
                prediction_width, prediction_height, block_start_x,
                MultiplyBy4(row4x4) >> subsampling_y, candidate_row,
                candidate_column, kObmcDirectionHorizontal)) {
          return false;
        }
      }
    }
  }
  return true;
}

void Tile::DistanceWeightedPrediction(void* prediction_0, void* prediction_1,
                                      const int width, const int height,
                                      const int candidate_row,
                                      const int candidate_column, uint8_t* dest,
                                      ptrdiff_t dest_stride) {
  int distance[2];
  int weight[2];
  for (int reference = 0; reference < 2; ++reference) {
    const BlockParameters& bp =
        *block_parameters_holder_.Find(candidate_row, candidate_column);
    // Note: distance[0] and distance[1] correspond to relative distance
    // between current frame and reference frame [1] and [0], respectively.
    distance[1 - reference] = std::min(
        std::abs(static_cast<int>(
            current_frame_.reference_info()
                ->relative_distance_from[bp.reference_frame[reference]])),
        static_cast<int>(kMaxFrameDistance));
  }
  GetDistanceWeights(distance, weight);

  dsp_.distance_weighted_blend(prediction_0, prediction_1, weight[0], weight[1],
                               width, height, dest, dest_stride);
}

void Tile::ScaleMotionVector(const MotionVector& mv, const Plane plane,
                             const int reference_frame_index, const int x,
                             const int y, int* const start_x,
                             int* const start_y, int* const step_x,
                             int* const step_y) {
  const int reference_upscaled_width =
      (reference_frame_index == -1)
          ? frame_header_.upscaled_width
          : reference_frames_[reference_frame_index]->upscaled_width();
  const int reference_height =
      (reference_frame_index == -1)
          ? frame_header_.height
          : reference_frames_[reference_frame_index]->frame_height();
  assert(2 * frame_header_.width >= reference_upscaled_width &&
         2 * frame_header_.height >= reference_height &&
         frame_header_.width <= 16 * reference_upscaled_width &&
         frame_header_.height <= 16 * reference_height);
  const bool is_scaled_x = reference_upscaled_width != frame_header_.width;
  const bool is_scaled_y = reference_height != frame_header_.height;
  const int half_sample = 1 << (kSubPixelBits - 1);
  int orig_x = (x << kSubPixelBits) + ((2 * mv.mv[1]) >> subsampling_x_[plane]);
  int orig_y = (y << kSubPixelBits) + ((2 * mv.mv[0]) >> subsampling_y_[plane]);
  const int rounding_offset =
      DivideBy2(1 << (kScaleSubPixelBits - kSubPixelBits));
  if (is_scaled_x) {
    const int scale_x = ((reference_upscaled_width << kReferenceScaleShift) +
                         DivideBy2(frame_header_.width)) /
                        frame_header_.width;
    *step_x = RightShiftWithRoundingSigned(
        scale_x, kReferenceScaleShift - kScaleSubPixelBits);
    orig_x += half_sample;
    // When frame size is 4k and above, orig_x can be above 16 bits, scale_x can
    // be up to 15 bits. So we use int64_t to hold base_x.
    const int64_t base_x = static_cast<int64_t>(orig_x) * scale_x -
                           (half_sample << kReferenceScaleShift);
    *start_x =
        RightShiftWithRoundingSigned(
            base_x, kReferenceScaleShift + kSubPixelBits - kScaleSubPixelBits) +
        rounding_offset;
  } else {
    *step_x = 1 << kScaleSubPixelBits;
    *start_x = LeftShift(orig_x, 6) + rounding_offset;
  }
  if (is_scaled_y) {
    const int scale_y = ((reference_height << kReferenceScaleShift) +
                         DivideBy2(frame_header_.height)) /
                        frame_header_.height;
    *step_y = RightShiftWithRoundingSigned(
        scale_y, kReferenceScaleShift - kScaleSubPixelBits);
    orig_y += half_sample;
    const int64_t base_y = static_cast<int64_t>(orig_y) * scale_y -
                           (half_sample << kReferenceScaleShift);
    *start_y =
        RightShiftWithRoundingSigned(
            base_y, kReferenceScaleShift + kSubPixelBits - kScaleSubPixelBits) +
        rounding_offset;
  } else {
    *step_y = 1 << kScaleSubPixelBits;
    *start_y = LeftShift(orig_y, 6) + rounding_offset;
  }
}

// static.
bool Tile::GetReferenceBlockPosition(
    const int reference_frame_index, const bool is_scaled, const int width,
    const int height, const int ref_start_x, const int ref_last_x,
    const int ref_start_y, const int ref_last_y, const int start_x,
    const int start_y, const int step_x, const int step_y,
    const int left_border, const int right_border, const int top_border,
    const int bottom_border, int* ref_block_start_x, int* ref_block_start_y,
    int* ref_block_end_x, int* ref_block_end_y) {
  *ref_block_start_x = GetPixelPositionFromHighScale(start_x, 0, 0);
  *ref_block_start_y = GetPixelPositionFromHighScale(start_y, 0, 0);
  if (reference_frame_index == -1) {
    return false;
  }
  *ref_block_start_x -= kConvolveBorderLeftTop;
  *ref_block_start_y -= kConvolveBorderLeftTop;
  *ref_block_end_x = GetPixelPositionFromHighScale(start_x, step_x, width - 1) +
                     kConvolveBorderRight;
  *ref_block_end_y =
      GetPixelPositionFromHighScale(start_y, step_y, height - 1) +
      kConvolveBorderBottom;
  if (is_scaled) {
    const int block_height =
        (((height - 1) * step_y + (1 << kScaleSubPixelBits) - 1) >>
         kScaleSubPixelBits) +
        kSubPixelTaps;
    *ref_block_end_x += kConvolveScaleBorderRight - kConvolveBorderRight;
    *ref_block_end_y = *ref_block_start_y + block_height - 1;
  }
  // Determines if we need to extend beyond the left/right/top/bottom border.
  return *ref_block_start_x < (ref_start_x - left_border) ||
         *ref_block_end_x > (ref_last_x + right_border) ||
         *ref_block_start_y < (ref_start_y - top_border) ||
         *ref_block_end_y > (ref_last_y + bottom_border);
}

// Builds a block as the input for convolve, by copying the content of
// reference frame (either a decoded reference frame, or current frame).
// |block_extended_width| is the combined width of the block and its borders.
template <typename Pixel>
void Tile::BuildConvolveBlock(
    const Plane plane, const int reference_frame_index, const bool is_scaled,
    const int height, const int ref_start_x, const int ref_last_x,
    const int ref_start_y, const int ref_last_y, const int step_y,
    const int ref_block_start_x, const int ref_block_end_x,
    const int ref_block_start_y, uint8_t* block_buffer,
    ptrdiff_t convolve_buffer_stride, ptrdiff_t block_extended_width) {
  const YuvBuffer* const reference_buffer =
      (reference_frame_index == -1)
          ? current_frame_.buffer()
          : reference_frames_[reference_frame_index]->buffer();
  Array2DView<const Pixel> reference_block(
      reference_buffer->height(plane),
      reference_buffer->stride(plane) / sizeof(Pixel),
      reinterpret_cast<const Pixel*>(reference_buffer->data(plane)));
  auto* const block_head = reinterpret_cast<Pixel*>(block_buffer);
  convolve_buffer_stride /= sizeof(Pixel);
  int block_height = height + kConvolveBorderLeftTop + kConvolveBorderBottom;
  if (is_scaled) {
    block_height = (((height - 1) * step_y + (1 << kScaleSubPixelBits) - 1) >>
                    kScaleSubPixelBits) +
                   kSubPixelTaps;
  }
  const int copy_start_x = Clip3(ref_block_start_x, ref_start_x, ref_last_x);
  const int copy_start_y = Clip3(ref_block_start_y, ref_start_y, ref_last_y);
  const int copy_end_x = Clip3(ref_block_end_x, copy_start_x, ref_last_x);
  const int block_width = copy_end_x - copy_start_x + 1;
  const bool extend_left = ref_block_start_x < ref_start_x;
  const bool extend_right = ref_block_end_x > ref_last_x;
  const bool out_of_left = copy_start_x > ref_block_end_x;
  const bool out_of_right = copy_end_x < ref_block_start_x;
  if (out_of_left || out_of_right) {
    const int ref_x = out_of_left ? copy_start_x : copy_end_x;
    Pixel* buf_ptr = block_head;
    for (int y = 0, ref_y = copy_start_y; y < block_height; ++y) {
      Memset(buf_ptr, reference_block[ref_y][ref_x], block_extended_width);
      if (ref_block_start_y + y >= ref_start_y &&
          ref_block_start_y + y < ref_last_y) {
        ++ref_y;
      }
      buf_ptr += convolve_buffer_stride;
    }
  } else {
    Pixel* buf_ptr = block_head;
    const int left_width = copy_start_x - ref_block_start_x;
    for (int y = 0, ref_y = copy_start_y; y < block_height; ++y) {
      if (extend_left) {
        Memset(buf_ptr, reference_block[ref_y][copy_start_x], left_width);
      }
      memcpy(buf_ptr + left_width, &reference_block[ref_y][copy_start_x],
             block_width * sizeof(Pixel));
      if (extend_right) {
        Memset(buf_ptr + left_width + block_width,
               reference_block[ref_y][copy_end_x],
               block_extended_width - left_width - block_width);
      }
      if (ref_block_start_y + y >= ref_start_y &&
          ref_block_start_y + y < ref_last_y) {
        ++ref_y;
      }
      buf_ptr += convolve_buffer_stride;
    }
  }
}

bool Tile::BlockInterPrediction(
    const Block& block, const Plane plane, const int reference_frame_index,
    const MotionVector& mv, const int x, const int y, const int width,
    const int height, const int candidate_row, const int candidate_column,
    uint16_t* const prediction, const bool is_compound,
    const bool is_inter_intra, uint8_t* const dest,
    const ptrdiff_t dest_stride) {
  const BlockParameters& bp =
      *block_parameters_holder_.Find(candidate_row, candidate_column);
  int start_x;
  int start_y;
  int step_x;
  int step_y;
  ScaleMotionVector(mv, plane, reference_frame_index, x, y, &start_x, &start_y,
                    &step_x, &step_y);
  const int horizontal_filter_index = bp.interpolation_filter[1];
  const int vertical_filter_index = bp.interpolation_filter[0];
  const int subsampling_x = subsampling_x_[plane];
  const int subsampling_y = subsampling_y_[plane];
  // reference_frame_index equal to -1 indicates using current frame as
  // reference.
  const YuvBuffer* const reference_buffer =
      (reference_frame_index == -1)
          ? current_frame_.buffer()
          : reference_frames_[reference_frame_index]->buffer();
  const int reference_upscaled_width =
      (reference_frame_index == -1)
          ? MultiplyBy4(frame_header_.columns4x4)
          : reference_frames_[reference_frame_index]->upscaled_width();
  const int reference_height =
      (reference_frame_index == -1)
          ? MultiplyBy4(frame_header_.rows4x4)
          : reference_frames_[reference_frame_index]->frame_height();
  const int ref_start_x = 0;
  const int ref_last_x =
      SubsampledValue(reference_upscaled_width, subsampling_x) - 1;
  const int ref_start_y = 0;
  const int ref_last_y = SubsampledValue(reference_height, subsampling_y) - 1;

  const bool is_scaled = (reference_frame_index != -1) &&
                         (frame_header_.width != reference_upscaled_width ||
                          frame_header_.height != reference_height);
  const int bitdepth = sequence_header_.color_config.bitdepth;
  const int pixel_size = (bitdepth == 8) ? sizeof(uint8_t) : sizeof(uint16_t);
  int ref_block_start_x;
  int ref_block_start_y;
  int ref_block_end_x;
  int ref_block_end_y;
  const bool extend_block = GetReferenceBlockPosition(
      reference_frame_index, is_scaled, width, height, ref_start_x, ref_last_x,
      ref_start_y, ref_last_y, start_x, start_y, step_x, step_y,
      reference_buffer->left_border(plane),
      reference_buffer->right_border(plane),
      reference_buffer->top_border(plane),
      reference_buffer->bottom_border(plane), &ref_block_start_x,
      &ref_block_start_y, &ref_block_end_x, &ref_block_end_y);

  // In frame parallel mode, ensure that the reference block has been decoded
  // and available for referencing.
  if (reference_frame_index != -1 && frame_parallel_) {
    // For U and V planes with subsampling, we need to multiply the value of
    // ref_block_end_y by 2 since we only track the progress of the Y planes.
    const int reference_y_max = LeftShift(
        std::min(ref_block_end_y + kSubPixelTaps, ref_last_y), subsampling_y);
    if (reference_frame_progress_cache_[reference_frame_index] <
            reference_y_max &&
        !reference_frames_[reference_frame_index]->WaitUntil(
            reference_y_max,
            &reference_frame_progress_cache_[reference_frame_index])) {
      return false;
    }
  }

  const uint8_t* block_start = nullptr;
  ptrdiff_t convolve_buffer_stride;
  if (!extend_block) {
    const YuvBuffer* const reference_buffer =
        (reference_frame_index == -1)
            ? current_frame_.buffer()
            : reference_frames_[reference_frame_index]->buffer();
    convolve_buffer_stride = reference_buffer->stride(plane);
    if (reference_frame_index == -1 || is_scaled) {
      block_start = reference_buffer->data(plane) +
                    ref_block_start_y * reference_buffer->stride(plane) +
                    ref_block_start_x * pixel_size;
    } else {
      block_start = reference_buffer->data(plane) +
                    (ref_block_start_y + kConvolveBorderLeftTop) *
                        reference_buffer->stride(plane) +
                    (ref_block_start_x + kConvolveBorderLeftTop) * pixel_size;
    }
  } else {
    const int border_right =
        is_scaled ? kConvolveScaleBorderRight : kConvolveBorderRight;
    // The block width can be at most 2 times as much as current
    // block's width because of scaling.
    auto block_extended_width = Align<ptrdiff_t>(
        (2 * width + kConvolveBorderLeftTop + border_right) * pixel_size,
        kMaxAlignment);
    convolve_buffer_stride = block.scratch_buffer->convolve_block_buffer_stride;
#if LIBGAV1_MAX_BITDEPTH >= 10
    if (bitdepth > 8) {
      BuildConvolveBlock<uint16_t>(
          plane, reference_frame_index, is_scaled, height, ref_start_x,
          ref_last_x, ref_start_y, ref_last_y, step_y, ref_block_start_x,
          ref_block_end_x, ref_block_start_y,
          block.scratch_buffer->convolve_block_buffer.get(),
          convolve_buffer_stride, block_extended_width);
    } else {
#endif
      BuildConvolveBlock<uint8_t>(
          plane, reference_frame_index, is_scaled, height, ref_start_x,
          ref_last_x, ref_start_y, ref_last_y, step_y, ref_block_start_x,
          ref_block_end_x, ref_block_start_y,
          block.scratch_buffer->convolve_block_buffer.get(),
          convolve_buffer_stride, block_extended_width);
#if LIBGAV1_MAX_BITDEPTH >= 10
    }
#endif
    block_start = block.scratch_buffer->convolve_block_buffer.get() +
                  (is_scaled ? 0
                             : kConvolveBorderLeftTop * convolve_buffer_stride +
                                   kConvolveBorderLeftTop * pixel_size);
  }

  void* const output =
      (is_compound || is_inter_intra) ? prediction : static_cast<void*>(dest);
  ptrdiff_t output_stride = (is_compound || is_inter_intra)
                                ? /*prediction_stride=*/width
                                : dest_stride;
#if LIBGAV1_MAX_BITDEPTH >= 10
  // |is_inter_intra| calculations are written to the |prediction| buffer.
  // Unlike the |is_compound| calculations the output is Pixel and not uint16_t.
  // convolve_func() expects |output_stride| to be in bytes and not Pixels.
  // |prediction_stride| is in units of uint16_t. Adjust |output_stride| to
  // account for this.
  if (is_inter_intra && sequence_header_.color_config.bitdepth > 8) {
    output_stride *= 2;
  }
#endif
  assert(output != nullptr);
  if (is_scaled) {
    dsp::ConvolveScaleFunc convolve_func = dsp_.convolve_scale[is_compound];
    assert(convolve_func != nullptr);

    convolve_func(block_start, convolve_buffer_stride, horizontal_filter_index,
                  vertical_filter_index, start_x, start_y, step_x, step_y,
                  width, height, output, output_stride);
  } else {
    const int horizontal_filter_id = (start_x >> 6) & kSubPixelMask;
    const int vertical_filter_id = (start_y >> 6) & kSubPixelMask;

    dsp::ConvolveFunc convolve_func =
        dsp_.convolve[reference_frame_index == -1][is_compound]
                     [vertical_filter_id != 0][horizontal_filter_id != 0];
    assert(convolve_func != nullptr);

    convolve_func(block_start, convolve_buffer_stride, horizontal_filter_index,
                  vertical_filter_index, horizontal_filter_id,
                  vertical_filter_id, width, height, output, output_stride);
  }
  return true;
}

bool Tile::BlockWarpProcess(const Block& block, const Plane plane,
                            const int index, const int block_start_x,
                            const int block_start_y, const int width,
                            const int height, GlobalMotion* const warp_params,
                            const bool is_compound, const bool is_inter_intra,
                            uint8_t* const dest, const ptrdiff_t dest_stride) {
  assert(width >= 8 && height >= 8);
  const BlockParameters& bp = *block.bp;
  const int reference_frame_index =
      frame_header_.reference_frame_index[bp.reference_frame[index] -
                                          kReferenceFrameLast];
  const uint8_t* const source =
      reference_frames_[reference_frame_index]->buffer()->data(plane);
  ptrdiff_t source_stride =
      reference_frames_[reference_frame_index]->buffer()->stride(plane);
  const int source_width =
      reference_frames_[reference_frame_index]->buffer()->width(plane);
  const int source_height =
      reference_frames_[reference_frame_index]->buffer()->height(plane);
  uint16_t* const prediction = block.scratch_buffer->prediction_buffer[index];

  // In frame parallel mode, ensure that the reference block has been decoded
  // and available for referencing.
  if (frame_parallel_) {
    int reference_y_max = -1;
    // Find out the maximum y-coordinate for warping.
    for (int start_y = block_start_y; start_y < block_start_y + height;
         start_y += 8) {
      for (int start_x = block_start_x; start_x < block_start_x + width;
           start_x += 8) {
        const int src_x = (start_x + 4) << subsampling_x_[plane];
        const int src_y = (start_y + 4) << subsampling_y_[plane];
        const int64_t dst_y =
            src_x * warp_params->params[4] +
            static_cast<int64_t>(src_y) * warp_params->params[5] +
            warp_params->params[1];
        const int64_t y4 = dst_y >> subsampling_y_[plane];
        const int iy4 = static_cast<int>(y4 >> kWarpedModelPrecisionBits);
        reference_y_max = std::max(iy4 + 8, reference_y_max);
      }
    }
    // For U and V planes with subsampling, we need to multiply reference_y_max
    // by 2 since we only track the progress of Y planes.
    reference_y_max = LeftShift(reference_y_max, subsampling_y_[plane]);
    if (reference_frame_progress_cache_[reference_frame_index] <
            reference_y_max &&
        !reference_frames_[reference_frame_index]->WaitUntil(
            reference_y_max,
            &reference_frame_progress_cache_[reference_frame_index])) {
      return false;
    }
  }
  if (is_compound) {
    dsp_.warp_compound(source, source_stride, source_width, source_height,
                       warp_params->params, subsampling_x_[plane],
                       subsampling_y_[plane], block_start_x, block_start_y,
                       width, height, warp_params->alpha, warp_params->beta,
                       warp_params->gamma, warp_params->delta, prediction,
                       /*prediction_stride=*/width);
  } else {
    void* const output = is_inter_intra ? static_cast<void*>(prediction) : dest;
    ptrdiff_t output_stride =
        is_inter_intra ? /*prediction_stride=*/width : dest_stride;
#if LIBGAV1_MAX_BITDEPTH >= 10
    // |is_inter_intra| calculations are written to the |prediction| buffer.
    // Unlike the |is_compound| calculations the output is Pixel and not
    // uint16_t. warp_clip() expects |output_stride| to be in bytes and not
    // Pixels. |prediction_stride| is in units of uint16_t. Adjust
    // |output_stride| to account for this.
    if (is_inter_intra && sequence_header_.color_config.bitdepth > 8) {
      output_stride *= 2;
    }
#endif
    dsp_.warp(source, source_stride, source_width, source_height,
              warp_params->params, subsampling_x_[plane], subsampling_y_[plane],
              block_start_x, block_start_y, width, height, warp_params->alpha,
              warp_params->beta, warp_params->gamma, warp_params->delta, output,
              output_stride);
  }
  return true;
}

}  // namespace libgav1
