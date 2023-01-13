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
#include <cstdint>
#include <cstring>

#include "src/dsp/constants.h"
#include "src/obu_parser.h"
#include "src/symbol_decoder_context.h"
#include "src/tile.h"
#include "src/utils/array_2d.h"
#include "src/utils/block_parameters_holder.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/entropy_decoder.h"
#include "src/utils/segmentation.h"
#include "src/utils/stack.h"
#include "src/utils/types.h"

namespace libgav1 {
namespace {

constexpr uint8_t kMaxVariableTransformTreeDepth = 2;
// Max_Tx_Depth array from section 5.11.5 in the spec with the following
// modification: If the element is not zero, it is subtracted by one. That is
// the only way in which this array is being used.
constexpr int kTxDepthCdfIndex[kMaxBlockSizes] = {
    0, 0, 1, 0, 0, 1, 2, 1, 1, 1, 2, 3, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3};

constexpr TransformSize kMaxTransformSizeRectangle[kMaxBlockSizes] = {
    kTransformSize4x4,   kTransformSize4x8,   kTransformSize4x16,
    kTransformSize8x4,   kTransformSize8x8,   kTransformSize8x16,
    kTransformSize8x32,  kTransformSize16x4,  kTransformSize16x8,
    kTransformSize16x16, kTransformSize16x32, kTransformSize16x64,
    kTransformSize32x8,  kTransformSize32x16, kTransformSize32x32,
    kTransformSize32x64, kTransformSize64x16, kTransformSize64x32,
    kTransformSize64x64, kTransformSize64x64, kTransformSize64x64,
    kTransformSize64x64};

TransformSize GetSquareTransformSize(uint8_t pixels) {
  switch (pixels) {
    case 128:
    case 64:
      return kTransformSize64x64;
    case 32:
      return kTransformSize32x32;
    case 16:
      return kTransformSize16x16;
    case 8:
      return kTransformSize8x8;
    default:
      return kTransformSize4x4;
  }
}

}  // namespace

int Tile::GetTopTransformWidth(const Block& block, int row4x4, int column4x4,
                               bool ignore_skip) {
  if (row4x4 == block.row4x4) {
    if (!block.top_available[kPlaneY]) return 64;
    const BlockParameters& bp_top =
        *block_parameters_holder_.Find(row4x4 - 1, column4x4);
    if ((ignore_skip || bp_top.skip) && bp_top.is_inter) {
      return kBlockWidthPixels[bp_top.size];
    }
  }
  return kTransformWidth[inter_transform_sizes_[row4x4 - 1][column4x4]];
}

int Tile::GetLeftTransformHeight(const Block& block, int row4x4, int column4x4,
                                 bool ignore_skip) {
  if (column4x4 == block.column4x4) {
    if (!block.left_available[kPlaneY]) return 64;
    const BlockParameters& bp_left =
        *block_parameters_holder_.Find(row4x4, column4x4 - 1);
    if ((ignore_skip || bp_left.skip) && bp_left.is_inter) {
      return kBlockHeightPixels[bp_left.size];
    }
  }
  return kTransformHeight[inter_transform_sizes_[row4x4][column4x4 - 1]];
}

TransformSize Tile::ReadFixedTransformSize(const Block& block) {
  BlockParameters& bp = *block.bp;
  if (frame_header_.segmentation
          .lossless[bp.prediction_parameters->segment_id]) {
    return kTransformSize4x4;
  }
  const TransformSize max_rect_tx_size = kMaxTransformSizeRectangle[block.size];
  const bool allow_select = !bp.skip || !bp.is_inter;
  if (block.size == kBlock4x4 || !allow_select ||
      frame_header_.tx_mode != kTxModeSelect) {
    return max_rect_tx_size;
  }
  const int max_tx_width = kTransformWidth[max_rect_tx_size];
  const int max_tx_height = kTransformHeight[max_rect_tx_size];
  const int top_width =
      block.top_available[kPlaneY]
          ? GetTopTransformWidth(block, block.row4x4, block.column4x4, true)
          : 0;
  const int left_height =
      block.left_available[kPlaneY]
          ? GetLeftTransformHeight(block, block.row4x4, block.column4x4, true)
          : 0;
  const auto context = static_cast<int>(top_width >= max_tx_width) +
                       static_cast<int>(left_height >= max_tx_height);
  const int cdf_index = kTxDepthCdfIndex[block.size];
  uint16_t* const cdf =
      symbol_decoder_context_.tx_depth_cdf[cdf_index][context];
  const int tx_depth = (cdf_index == 0)
                           ? static_cast<int>(reader_.ReadSymbol(cdf))
                           : reader_.ReadSymbol<3>(cdf);
  assert(tx_depth < 3);
  TransformSize tx_size = max_rect_tx_size;
  if (tx_depth == 0) return tx_size;
  tx_size = kSplitTransformSize[tx_size];
  if (tx_depth == 1) return tx_size;
  return kSplitTransformSize[tx_size];
}

void Tile::ReadVariableTransformTree(const Block& block, int row4x4,
                                     int column4x4, TransformSize tx_size) {
  const uint8_t pixels = std::max(block.width, block.height);
  const TransformSize max_tx_size = GetSquareTransformSize(pixels);
  const int context_delta = (kNumSquareTransformSizes - 1 -
                             TransformSizeToSquareTransformIndex(max_tx_size)) *
                            6;

  // Branching factor is 4 and maximum depth is 2. So the maximum stack size
  // necessary is (4 - 1) + 4 = 7.
  Stack<TransformTreeNode, 7> stack;
  stack.Push(TransformTreeNode(column4x4, row4x4, tx_size, 0));

  do {
    TransformTreeNode node = stack.Pop();
    const int tx_width4x4 = kTransformWidth4x4[node.tx_size];
    const int tx_height4x4 = kTransformHeight4x4[node.tx_size];
    if (node.tx_size != kTransformSize4x4 &&
        node.depth != kMaxVariableTransformTreeDepth) {
      const auto top =
          static_cast<int>(GetTopTransformWidth(block, node.y, node.x, false) <
                           kTransformWidth[node.tx_size]);
      const auto left = static_cast<int>(
          GetLeftTransformHeight(block, node.y, node.x, false) <
          kTransformHeight[node.tx_size]);
      const int context =
          static_cast<int>(max_tx_size > kTransformSize8x8 &&
                           kTransformSizeSquareMax[node.tx_size] !=
                               max_tx_size) *
              3 +
          context_delta + top + left;
      // tx_split.
      if (reader_.ReadSymbol(symbol_decoder_context_.tx_split_cdf[context])) {
        const TransformSize sub_tx_size = kSplitTransformSize[node.tx_size];
        const int step_width4x4 = kTransformWidth4x4[sub_tx_size];
        const int step_height4x4 = kTransformHeight4x4[sub_tx_size];
        // The loops have to run in reverse order because we use a stack for
        // DFS.
        for (int i = tx_height4x4 - step_height4x4; i >= 0;
             i -= step_height4x4) {
          for (int j = tx_width4x4 - step_width4x4; j >= 0;
               j -= step_width4x4) {
            if (node.y + i >= frame_header_.rows4x4 ||
                node.x + j >= frame_header_.columns4x4) {
              continue;
            }
            stack.Push(TransformTreeNode(node.x + j, node.y + i, sub_tx_size,
                                         node.depth + 1));
          }
        }
        continue;
      }
    }
    // tx_split is false.
    for (int i = 0; i < tx_height4x4; ++i) {
      static_assert(sizeof(TransformSize) == 1, "");
      memset(&inter_transform_sizes_[node.y + i][node.x], node.tx_size,
             tx_width4x4);
    }
  } while (!stack.Empty());
}

void Tile::DecodeTransformSize(const Block& block) {
  BlockParameters& bp = *block.bp;
  if (frame_header_.tx_mode == kTxModeSelect && block.size > kBlock4x4 &&
      bp.is_inter && !bp.skip &&
      !frame_header_.segmentation
           .lossless[bp.prediction_parameters->segment_id]) {
    const TransformSize max_tx_size = kMaxTransformSizeRectangle[block.size];
    const int tx_width4x4 = kTransformWidth4x4[max_tx_size];
    const int tx_height4x4 = kTransformHeight4x4[max_tx_size];
    for (int row = block.row4x4; row < block.row4x4 + block.height4x4;
         row += tx_height4x4) {
      for (int column = block.column4x4;
           column < block.column4x4 + block.width4x4; column += tx_width4x4) {
        ReadVariableTransformTree(block, row, column, max_tx_size);
      }
    }
  } else {
    const TransformSize transform_size = ReadFixedTransformSize(block);
    for (int row = block.row4x4; row < block.row4x4 + block.height4x4; ++row) {
      static_assert(sizeof(TransformSize) == 1, "");
      memset(&inter_transform_sizes_[row][block.column4x4], transform_size,
             block.width4x4);
    }
  }
}

}  // namespace libgav1
