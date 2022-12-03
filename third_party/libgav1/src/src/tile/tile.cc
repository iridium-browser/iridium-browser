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

#include "src/tile.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include <numeric>
#include <type_traits>
#include <utility>

#include "src/frame_scratch_buffer.h"
#include "src/motion_vector.h"
#include "src/reconstruction.h"
#include "src/utils/bit_mask_set.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/logging.h"
#include "src/utils/segmentation.h"
#include "src/utils/stack.h"

namespace libgav1 {
namespace {

// Import all the constants in the anonymous namespace.
#include "src/scan_tables.inc"

// Range above kNumQuantizerBaseLevels which the exponential golomb coding
// process is activated.
constexpr int kQuantizerCoefficientBaseRange = 12;
constexpr int kNumQuantizerBaseLevels = 2;
constexpr int kCoeffBaseRangeMaxIterations =
    kQuantizerCoefficientBaseRange / (kCoeffBaseRangeSymbolCount - 1);
constexpr int kEntropyContextLeft = 0;
constexpr int kEntropyContextTop = 1;

constexpr uint8_t kAllZeroContextsByTopLeft[5][5] = {{1, 2, 2, 2, 3},
                                                     {2, 4, 4, 4, 5},
                                                     {2, 4, 4, 4, 5},
                                                     {2, 4, 4, 4, 5},
                                                     {3, 5, 5, 5, 6}};

// The space complexity of DFS is O(branching_factor * max_depth). For the
// parameter tree, branching_factor = 4 (there could be up to 4 children for
// every node) and max_depth (excluding the root) = 5 (to go from a 128x128
// block all the way to a 4x4 block). The worse-case stack size is 16, by
// counting the number of 'o' nodes in the diagram:
//
//   |                    128x128  The highest level (corresponding to the
//   |                             root of the tree) has no node in the stack.
//   |-----------------+
//   |     |     |     |
//   |     o     o     o  64x64
//   |
//   |-----------------+
//   |     |     |     |
//   |     o     o     o  32x32    Higher levels have three nodes in the stack,
//   |                             because we pop one node off the stack before
//   |-----------------+           pushing its four children onto the stack.
//   |     |     |     |
//   |     o     o     o  16x16
//   |
//   |-----------------+
//   |     |     |     |
//   |     o     o     o  8x8
//   |
//   |-----------------+
//   |     |     |     |
//   o     o     o     o  4x4      Only the lowest level has four nodes in the
//                                 stack.
constexpr int kDfsStackSize = 16;

// Mask indicating whether the transform sets contain a particular transform
// type. If |tx_type| is present in |tx_set|, then the |tx_type|th LSB is set.
constexpr BitMaskSet kTransformTypeInSetMask[kNumTransformSets] = {
    BitMaskSet(0x1),    BitMaskSet(0xE0F), BitMaskSet(0x20F),
    BitMaskSet(0xFFFF), BitMaskSet(0xFFF), BitMaskSet(0x201)};

constexpr PredictionMode
    kFilterIntraModeToIntraPredictor[kNumFilterIntraPredictors] = {
        kPredictionModeDc, kPredictionModeVertical, kPredictionModeHorizontal,
        kPredictionModeD157, kPredictionModeDc};

// Mask used to determine the index for mode_deltas lookup.
constexpr BitMaskSet kPredictionModeDeltasMask(
    kPredictionModeNearestMv, kPredictionModeNearMv, kPredictionModeNewMv,
    kPredictionModeNearestNearestMv, kPredictionModeNearNearMv,
    kPredictionModeNearestNewMv, kPredictionModeNewNearestMv,
    kPredictionModeNearNewMv, kPredictionModeNewNearMv,
    kPredictionModeNewNewMv);

// This is computed as:
// min(transform_width_log2, 5) + min(transform_height_log2, 5) - 4.
constexpr uint8_t kEobMultiSizeLookup[kNumTransformSizes] = {
    0, 1, 2, 1, 2, 3, 4, 2, 3, 4, 5, 5, 4, 5, 6, 6, 5, 6, 6};

/* clang-format off */
constexpr uint8_t kCoeffBaseContextOffset[kNumTransformSizes][5][5] = {
    {{0, 1, 6, 6, 0}, {1, 6, 6, 21, 0}, {6, 6, 21, 21, 0}, {6, 21, 21, 21, 0},
     {0, 0, 0, 0, 0}},
    {{0, 11, 11, 11, 0}, {11, 11, 11, 11, 0}, {6, 6, 21, 21, 0},
     {6, 21, 21, 21, 0}, {21, 21, 21, 21, 0}},
    {{0, 11, 11, 11, 0}, {11, 11, 11, 11, 0}, {6, 6, 21, 21, 0},
     {6, 21, 21, 21, 0}, {21, 21, 21, 21, 0}},
    {{0, 16, 6, 6, 21}, {16, 16, 6, 21, 21}, {16, 16, 21, 21, 21},
     {16, 16, 21, 21, 21}, {0, 0, 0, 0, 0}},
    {{0, 1, 6, 6, 21}, {1, 6, 6, 21, 21}, {6, 6, 21, 21, 21},
     {6, 21, 21, 21, 21}, {21, 21, 21, 21, 21}},
    {{0, 11, 11, 11, 11}, {11, 11, 11, 11, 11}, {6, 6, 21, 21, 21},
     {6, 21, 21, 21, 21}, {21, 21, 21, 21, 21}},
    {{0, 11, 11, 11, 11}, {11, 11, 11, 11, 11}, {6, 6, 21, 21, 21},
     {6, 21, 21, 21, 21}, {21, 21, 21, 21, 21}},
    {{0, 16, 6, 6, 21}, {16, 16, 6, 21, 21}, {16, 16, 21, 21, 21},
     {16, 16, 21, 21, 21}, {0, 0, 0, 0, 0}},
    {{0, 16, 6, 6, 21}, {16, 16, 6, 21, 21}, {16, 16, 21, 21, 21},
     {16, 16, 21, 21, 21}, {16, 16, 21, 21, 21}},
    {{0, 1, 6, 6, 21}, {1, 6, 6, 21, 21}, {6, 6, 21, 21, 21},
     {6, 21, 21, 21, 21}, {21, 21, 21, 21, 21}},
    {{0, 11, 11, 11, 11}, {11, 11, 11, 11, 11}, {6, 6, 21, 21, 21},
     {6, 21, 21, 21, 21}, {21, 21, 21, 21, 21}},
    {{0, 11, 11, 11, 11}, {11, 11, 11, 11, 11}, {6, 6, 21, 21, 21},
     {6, 21, 21, 21, 21}, {21, 21, 21, 21, 21}},
    {{0, 16, 6, 6, 21}, {16, 16, 6, 21, 21}, {16, 16, 21, 21, 21},
     {16, 16, 21, 21, 21}, {16, 16, 21, 21, 21}},
    {{0, 16, 6, 6, 21}, {16, 16, 6, 21, 21}, {16, 16, 21, 21, 21},
     {16, 16, 21, 21, 21}, {16, 16, 21, 21, 21}},
    {{0, 1, 6, 6, 21}, {1, 6, 6, 21, 21}, {6, 6, 21, 21, 21},
     {6, 21, 21, 21, 21}, {21, 21, 21, 21, 21}},
    {{0, 11, 11, 11, 11}, {11, 11, 11, 11, 11}, {6, 6, 21, 21, 21},
     {6, 21, 21, 21, 21}, {21, 21, 21, 21, 21}},
    {{0, 16, 6, 6, 21}, {16, 16, 6, 21, 21}, {16, 16, 21, 21, 21},
     {16, 16, 21, 21, 21}, {16, 16, 21, 21, 21}},
    {{0, 16, 6, 6, 21}, {16, 16, 6, 21, 21}, {16, 16, 21, 21, 21},
     {16, 16, 21, 21, 21}, {16, 16, 21, 21, 21}},
    {{0, 1, 6, 6, 21}, {1, 6, 6, 21, 21}, {6, 6, 21, 21, 21},
     {6, 21, 21, 21, 21}, {21, 21, 21, 21, 21}}};
/* clang-format on */

// Extended the table size from 3 to 16 by repeating the last element to avoid
// the clips to row or column indices.
constexpr uint8_t kCoeffBasePositionContextOffset[16] = {
    26, 31, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36};

constexpr PredictionMode kInterIntraToIntraMode[kNumInterIntraModes] = {
    kPredictionModeDc, kPredictionModeVertical, kPredictionModeHorizontal,
    kPredictionModeSmooth};

// Number of horizontal luma samples before intra block copy can be used.
constexpr int kIntraBlockCopyDelayPixels = 256;
// Number of 64 by 64 blocks before intra block copy can be used.
constexpr int kIntraBlockCopyDelay64x64Blocks = kIntraBlockCopyDelayPixels / 64;

// Index [i][j] corresponds to the transform size of width 1 << (i + 2) and
// height 1 << (j + 2).
constexpr TransformSize k4x4SizeToTransformSize[5][5] = {
    {kTransformSize4x4, kTransformSize4x8, kTransformSize4x16,
     kNumTransformSizes, kNumTransformSizes},
    {kTransformSize8x4, kTransformSize8x8, kTransformSize8x16,
     kTransformSize8x32, kNumTransformSizes},
    {kTransformSize16x4, kTransformSize16x8, kTransformSize16x16,
     kTransformSize16x32, kTransformSize16x64},
    {kNumTransformSizes, kTransformSize32x8, kTransformSize32x16,
     kTransformSize32x32, kTransformSize32x64},
    {kNumTransformSizes, kNumTransformSizes, kTransformSize64x16,
     kTransformSize64x32, kTransformSize64x64}};

// Defined in section 9.3 of the spec.
constexpr TransformType kModeToTransformType[kIntraPredictionModesUV] = {
    kTransformTypeDctDct,   kTransformTypeDctAdst,  kTransformTypeAdstDct,
    kTransformTypeDctDct,   kTransformTypeAdstAdst, kTransformTypeDctAdst,
    kTransformTypeAdstDct,  kTransformTypeAdstDct,  kTransformTypeDctAdst,
    kTransformTypeAdstAdst, kTransformTypeDctAdst,  kTransformTypeAdstDct,
    kTransformTypeAdstAdst, kTransformTypeDctDct};

// Defined in section 5.11.47 of the spec. This array does not contain an entry
// for kTransformSetDctOnly, so the first dimension needs to be
// |kNumTransformSets| - 1.
constexpr TransformType kInverseTransformTypeBySet[kNumTransformSets - 1][16] =
    {{kTransformTypeIdentityIdentity, kTransformTypeDctDct,
      kTransformTypeIdentityDct, kTransformTypeDctIdentity,
      kTransformTypeAdstAdst, kTransformTypeDctAdst, kTransformTypeAdstDct},
     {kTransformTypeIdentityIdentity, kTransformTypeDctDct,
      kTransformTypeAdstAdst, kTransformTypeDctAdst, kTransformTypeAdstDct},
     {kTransformTypeIdentityIdentity, kTransformTypeIdentityDct,
      kTransformTypeDctIdentity, kTransformTypeIdentityAdst,
      kTransformTypeAdstIdentity, kTransformTypeIdentityFlipadst,
      kTransformTypeFlipadstIdentity, kTransformTypeDctDct,
      kTransformTypeDctAdst, kTransformTypeAdstDct, kTransformTypeDctFlipadst,
      kTransformTypeFlipadstDct, kTransformTypeAdstAdst,
      kTransformTypeFlipadstFlipadst, kTransformTypeFlipadstAdst,
      kTransformTypeAdstFlipadst},
     {kTransformTypeIdentityIdentity, kTransformTypeIdentityDct,
      kTransformTypeDctIdentity, kTransformTypeDctDct, kTransformTypeDctAdst,
      kTransformTypeAdstDct, kTransformTypeDctFlipadst,
      kTransformTypeFlipadstDct, kTransformTypeAdstAdst,
      kTransformTypeFlipadstFlipadst, kTransformTypeFlipadstAdst,
      kTransformTypeAdstFlipadst},
     {kTransformTypeIdentityIdentity, kTransformTypeDctDct}};

// Replaces all occurrences of 64x* and *x64 with 32x* and *x32 respectively.
constexpr TransformSize kAdjustedTransformSize[kNumTransformSizes] = {
    kTransformSize4x4,   kTransformSize4x8,   kTransformSize4x16,
    kTransformSize8x4,   kTransformSize8x8,   kTransformSize8x16,
    kTransformSize8x32,  kTransformSize16x4,  kTransformSize16x8,
    kTransformSize16x16, kTransformSize16x32, kTransformSize16x32,
    kTransformSize32x8,  kTransformSize32x16, kTransformSize32x32,
    kTransformSize32x32, kTransformSize32x16, kTransformSize32x32,
    kTransformSize32x32};

// This is the same as Max_Tx_Size_Rect array in the spec but with *x64 and 64*x
// transforms replaced with *x32 and 32x* respectively.
constexpr TransformSize kUVTransformSize[kMaxBlockSizes] = {
    kTransformSize4x4,   kTransformSize4x8,   kTransformSize4x16,
    kTransformSize8x4,   kTransformSize8x8,   kTransformSize8x16,
    kTransformSize8x32,  kTransformSize16x4,  kTransformSize16x8,
    kTransformSize16x16, kTransformSize16x32, kTransformSize16x32,
    kTransformSize32x8,  kTransformSize32x16, kTransformSize32x32,
    kTransformSize32x32, kTransformSize32x16, kTransformSize32x32,
    kTransformSize32x32, kTransformSize32x32, kTransformSize32x32,
    kTransformSize32x32};

// ith entry of this array is computed as:
// DivideBy2(TransformSizeToSquareTransformIndex(kTransformSizeSquareMin[i]) +
//           TransformSizeToSquareTransformIndex(kTransformSizeSquareMax[i]) +
//           1)
constexpr uint8_t kTransformSizeContext[kNumTransformSizes] = {
    0, 1, 1, 1, 1, 2, 2, 1, 2, 2, 3, 3, 2, 3, 3, 4, 3, 4, 4};

constexpr int8_t kSgrProjDefaultMultiplier[2] = {-32, 31};

constexpr int8_t kWienerDefaultFilter[kNumWienerCoefficients] = {3, -7, 15};

// Maps compound prediction modes into single modes. For e.g.
// kPredictionModeNearestNewMv will map to kPredictionModeNearestMv for index 0
// and kPredictionModeNewMv for index 1. It is used to simplify the logic in
// AssignMv (and avoid duplicate code). This is section 5.11.30. in the spec.
constexpr PredictionMode
    kCompoundToSinglePredictionMode[kNumCompoundInterPredictionModes][2] = {
        {kPredictionModeNearestMv, kPredictionModeNearestMv},
        {kPredictionModeNearMv, kPredictionModeNearMv},
        {kPredictionModeNearestMv, kPredictionModeNewMv},
        {kPredictionModeNewMv, kPredictionModeNearestMv},
        {kPredictionModeNearMv, kPredictionModeNewMv},
        {kPredictionModeNewMv, kPredictionModeNearMv},
        {kPredictionModeGlobalMv, kPredictionModeGlobalMv},
        {kPredictionModeNewMv, kPredictionModeNewMv},
};
PredictionMode GetSinglePredictionMode(int index, PredictionMode y_mode) {
  if (y_mode < kPredictionModeNearestNearestMv) {
    return y_mode;
  }
  const int lookup_index = y_mode - kPredictionModeNearestNearestMv;
  assert(lookup_index >= 0);
  return kCompoundToSinglePredictionMode[lookup_index][index];
}

// log2(dqDenom) in section 7.12.3 of the spec. We use the log2 value because
// dqDenom is always a power of two and hence right shift can be used instead of
// division.
constexpr uint8_t kQuantizationShift[kNumTransformSizes] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 2, 1, 2, 2};

// Returns the minimum of |length| or |max|-|start|. This is used to clamp array
// indices when accessing arrays whose bound is equal to |max|.
int GetNumElements(int length, int start, int max) {
  return std::min(length, max - start);
}

template <typename T>
void SetBlockValues(int rows, int columns, T value, T* dst, ptrdiff_t stride) {
  // Specialize all columns cases (values in kTransformWidth4x4[]) for better
  // performance.
  switch (columns) {
    case 1:
      MemSetBlock<T>(rows, 1, value, dst, stride);
      break;
    case 2:
      MemSetBlock<T>(rows, 2, value, dst, stride);
      break;
    case 4:
      MemSetBlock<T>(rows, 4, value, dst, stride);
      break;
    case 8:
      MemSetBlock<T>(rows, 8, value, dst, stride);
      break;
    default:
      assert(columns == 16);
      MemSetBlock<T>(rows, 16, value, dst, stride);
      break;
  }
}

void SetTransformType(const Tile::Block& block, int x4, int y4, int w4, int h4,
                      TransformType tx_type,
                      TransformType transform_types[32][32]) {
  const int y_offset = y4 - block.row4x4;
  const int x_offset = x4 - block.column4x4;
  TransformType* const dst = &transform_types[y_offset][x_offset];
  SetBlockValues<TransformType>(h4, w4, tx_type, dst, 32);
}

void StoreMotionFieldMvs(ReferenceFrameType reference_frame_to_store,
                         const MotionVector& mv_to_store, ptrdiff_t stride,
                         int rows, int columns,
                         ReferenceFrameType* reference_frame_row_start,
                         MotionVector* mv) {
  static_assert(sizeof(*reference_frame_row_start) == sizeof(int8_t), "");
  do {
    // Don't switch the following two memory setting functions.
    // Some ARM CPUs are quite sensitive to the order.
    memset(reference_frame_row_start, reference_frame_to_store, columns);
    std::fill(mv, mv + columns, mv_to_store);
    reference_frame_row_start += stride;
    mv += stride;
  } while (--rows != 0);
}

// Inverse transform process assumes that the quantized coefficients are stored
// as a virtual 2d array of size |tx_width| x tx_height. If transform width is
// 64, then this assumption is broken because the scan order used for populating
// the coefficients for such transforms is the same as the one used for
// corresponding transform with width 32 (e.g. the scan order used for 64x16 is
// the same as the one used for 32x16). So we must restore the coefficients to
// their correct positions and clean the positions they occupied.
template <typename ResidualType>
void MoveCoefficientsForTxWidth64(int clamped_tx_height, int tx_width,
                                  ResidualType* residual) {
  if (tx_width != 64) return;
  const int rows = clamped_tx_height - 2;
  auto* src = residual + 32 * rows;
  residual += 64 * rows;
  // Process 2 rows in each loop in reverse order to avoid overwrite.
  int x = rows >> 1;
  do {
    // The 2 rows can be processed in order.
    memcpy(residual, src, 32 * sizeof(src[0]));
    memcpy(residual + 64, src + 32, 32 * sizeof(src[0]));
    memset(src + 32, 0, 32 * sizeof(src[0]));
    src -= 64;
    residual -= 128;
  } while (--x);
  // Process the second row. The first row is already correct.
  memcpy(residual + 64, src + 32, 32 * sizeof(src[0]));
  memset(src + 32, 0, 32 * sizeof(src[0]));
}

void GetClampParameters(const Tile::Block& block, int min[2], int max[2]) {
  // 7.10.2.14 (part 1). (also contains implementations of 5.11.53
  // and 5.11.54).
  constexpr int kMvBorder4x4 = 4;
  const int row_border = kMvBorder4x4 + block.height4x4;
  const int column_border = kMvBorder4x4 + block.width4x4;
  const int macroblocks_to_top_edge = -block.row4x4;
  const int macroblocks_to_bottom_edge =
      block.tile.frame_header().rows4x4 - block.height4x4 - block.row4x4;
  const int macroblocks_to_left_edge = -block.column4x4;
  const int macroblocks_to_right_edge =
      block.tile.frame_header().columns4x4 - block.width4x4 - block.column4x4;
  min[0] = MultiplyBy32(macroblocks_to_top_edge - row_border);
  min[1] = MultiplyBy32(macroblocks_to_left_edge - column_border);
  max[0] = MultiplyBy32(macroblocks_to_bottom_edge + row_border);
  max[1] = MultiplyBy32(macroblocks_to_right_edge + column_border);
}

// Section 8.3.2 in the spec, under coeff_base_eob.
int GetCoeffBaseContextEob(TransformSize tx_size, int index) {
  if (index == 0) return 0;
  const TransformSize adjusted_tx_size = kAdjustedTransformSize[tx_size];
  const int tx_width_log2 = kTransformWidthLog2[adjusted_tx_size];
  const int tx_height = kTransformHeight[adjusted_tx_size];
  if (index <= DivideBy8(tx_height << tx_width_log2)) return 1;
  if (index <= DivideBy4(tx_height << tx_width_log2)) return 2;
  return 3;
}

// Section 8.3.2 in the spec, under coeff_br. Optimized for end of block based
// on the fact that {0, 1}, {1, 0}, {1, 1}, {0, 2} and {2, 0} will all be 0 in
// the end of block case.
int GetCoeffBaseRangeContextEob(int adjusted_tx_width_log2, int pos,
                                TransformClass tx_class) {
  if (pos == 0) return 0;
  const int tx_width = 1 << adjusted_tx_width_log2;
  const int row = pos >> adjusted_tx_width_log2;
  const int column = pos & (tx_width - 1);
  // This return statement is equivalent to:
  // return ((tx_class == kTransformClass2D && (row | column) < 2) ||
  //         (tx_class == kTransformClassHorizontal && column == 0) ||
  //         (tx_class == kTransformClassVertical && row == 0))
  //            ? 7
  //            : 14;
  return 14 >> ((static_cast<int>(tx_class == kTransformClass2D) &
                 static_cast<int>((row | column) < 2)) |
                (tx_class & static_cast<int>(column == 0)) |
                ((tx_class >> 1) & static_cast<int>(row == 0)));
}

}  // namespace

Tile::Tile(int tile_number, const uint8_t* const data, size_t size,
           const ObuSequenceHeader& sequence_header,
           const ObuFrameHeader& frame_header,
           RefCountedBuffer* const current_frame, const DecoderState& state,
           FrameScratchBuffer* const frame_scratch_buffer,
           const WedgeMaskArray& wedge_masks,
           const QuantizerMatrix& quantizer_matrix,
           SymbolDecoderContext* const saved_symbol_decoder_context,
           const SegmentationMap* prev_segment_ids,
           PostFilter* const post_filter, const dsp::Dsp* const dsp,
           ThreadPool* const thread_pool,
           BlockingCounterWithStatus* const pending_tiles, bool frame_parallel,
           bool use_intra_prediction_buffer)
    : number_(tile_number),
      row_(number_ / frame_header.tile_info.tile_columns),
      column_(number_ % frame_header.tile_info.tile_columns),
      data_(data),
      size_(size),
      read_deltas_(false),
      subsampling_x_{0, sequence_header.color_config.subsampling_x,
                     sequence_header.color_config.subsampling_x},
      subsampling_y_{0, sequence_header.color_config.subsampling_y,
                     sequence_header.color_config.subsampling_y},
      current_quantizer_index_(frame_header.quantizer.base_index),
      sequence_header_(sequence_header),
      frame_header_(frame_header),
      reference_frame_sign_bias_(state.reference_frame_sign_bias),
      reference_frames_(state.reference_frame),
      motion_field_(frame_scratch_buffer->motion_field),
      reference_order_hint_(state.reference_order_hint),
      wedge_masks_(wedge_masks),
      quantizer_matrix_(quantizer_matrix),
      reader_(data_, size_, frame_header_.enable_cdf_update),
      symbol_decoder_context_(frame_scratch_buffer->symbol_decoder_context),
      saved_symbol_decoder_context_(saved_symbol_decoder_context),
      prev_segment_ids_(prev_segment_ids),
      dsp_(*dsp),
      post_filter_(*post_filter),
      block_parameters_holder_(frame_scratch_buffer->block_parameters_holder),
      quantizer_(sequence_header_.color_config.bitdepth,
                 &frame_header_.quantizer),
      residual_size_((sequence_header_.color_config.bitdepth == 8)
                         ? sizeof(int16_t)
                         : sizeof(int32_t)),
      intra_block_copy_lag_(
          frame_header_.allow_intrabc
              ? (sequence_header_.use_128x128_superblock ? 3 : 5)
              : 1),
      current_frame_(*current_frame),
      cdef_index_(frame_scratch_buffer->cdef_index),
      cdef_skip_(frame_scratch_buffer->cdef_skip),
      inter_transform_sizes_(frame_scratch_buffer->inter_transform_sizes),
      thread_pool_(thread_pool),
      residual_buffer_pool_(frame_scratch_buffer->residual_buffer_pool.get()),
      tile_scratch_buffer_pool_(
          &frame_scratch_buffer->tile_scratch_buffer_pool),
      pending_tiles_(pending_tiles),
      frame_parallel_(frame_parallel),
      use_intra_prediction_buffer_(use_intra_prediction_buffer),
      intra_prediction_buffer_(
          use_intra_prediction_buffer_
              ? &frame_scratch_buffer->intra_prediction_buffers.get()[row_]
              : nullptr) {
  row4x4_start_ = frame_header.tile_info.tile_row_start[row_];
  row4x4_end_ = frame_header.tile_info.tile_row_start[row_ + 1];
  column4x4_start_ = frame_header.tile_info.tile_column_start[column_];
  column4x4_end_ = frame_header.tile_info.tile_column_start[column_ + 1];
  const int block_width4x4 = kNum4x4BlocksWide[SuperBlockSize()];
  const int block_width4x4_log2 = k4x4HeightLog2[SuperBlockSize()];
  superblock_rows_ =
      (row4x4_end_ - row4x4_start_ + block_width4x4 - 1) >> block_width4x4_log2;
  superblock_columns_ =
      (column4x4_end_ - column4x4_start_ + block_width4x4 - 1) >>
      block_width4x4_log2;
  // If |split_parse_and_decode_| is true, we do the necessary setup for
  // splitting the parsing and the decoding steps. This is done in the following
  // two cases:
  //  1) If there is multi-threading within a tile (this is done if
  //     |thread_pool_| is not nullptr and if there are at least as many
  //     superblock columns as |intra_block_copy_lag_|).
  //  2) If |frame_parallel| is true.
  split_parse_and_decode_ = (thread_pool_ != nullptr &&
                             superblock_columns_ > intra_block_copy_lag_) ||
                            frame_parallel;
  if (frame_parallel_) {
    reference_frame_progress_cache_.fill(INT_MIN);
  }
  memset(delta_lf_, 0, sizeof(delta_lf_));
  delta_lf_all_zero_ = true;
  const YuvBuffer& buffer = post_filter_.frame_buffer();
  for (int plane = kPlaneY; plane < PlaneCount(); ++plane) {
    // Verify that the borders are big enough for Reconstruct(). max_tx_length
    // is the maximum value of tx_width and tx_height for the plane.
    const int max_tx_length = (plane == kPlaneY) ? 64 : 32;
    // Reconstruct() may overwrite on the right. Since the right border of a
    // row is followed in memory by the left border of the next row, the
    // number of extra pixels to the right of a row is at least the sum of the
    // left and right borders.
    //
    // Note: This assertion actually checks the sum of the left and right
    // borders of post_filter_.GetUnfilteredBuffer(), which is a horizontally
    // and vertically shifted version of |buffer|. Since the sum of the left and
    // right borders is not changed by the shift, we can just check the sum of
    // the left and right borders of |buffer|.
    assert(buffer.left_border(plane) + buffer.right_border(plane) >=
           max_tx_length - 1);
    // Reconstruct() may overwrite on the bottom. We need an extra border row
    // on the bottom because we need the left border of that row.
    //
    // Note: This assertion checks the bottom border of
    // post_filter_.GetUnfilteredBuffer(). So we need to calculate the vertical
    // shift that the PostFilter constructor applied to |buffer| and reduce the
    // bottom border by that amount.
#ifndef NDEBUG
    const int vertical_shift = static_cast<int>(
        (post_filter_.GetUnfilteredBuffer(plane) - buffer.data(plane)) /
        buffer.stride(plane));
    const int bottom_border = buffer.bottom_border(plane) - vertical_shift;
    assert(bottom_border >= max_tx_length);
#endif
    // In AV1, a transform block of height H starts at a y coordinate that is
    // a multiple of H. If a transform block at the bottom of the frame has
    // height H, then Reconstruct() will write up to the row with index
    // Align(buffer.height(plane), H) - 1. Therefore the maximum number of
    // rows Reconstruct() may write to is
    // Align(buffer.height(plane), max_tx_length).
    buffer_[plane].Reset(Align(buffer.height(plane), max_tx_length),
                         buffer.stride(plane),
                         post_filter_.GetUnfilteredBuffer(plane));
  }
}

bool Tile::Init() {
  assert(coefficient_levels_.size() == dc_categories_.size());
  for (size_t i = 0; i < coefficient_levels_.size(); ++i) {
    const int contexts_per_plane = (i == kEntropyContextLeft)
                                       ? frame_header_.rows4x4
                                       : frame_header_.columns4x4;
    if (!coefficient_levels_[i].Reset(PlaneCount(), contexts_per_plane)) {
      LIBGAV1_DLOG(ERROR, "coefficient_levels_[%zu].Reset() failed.", i);
      return false;
    }
    if (!dc_categories_[i].Reset(PlaneCount(), contexts_per_plane)) {
      LIBGAV1_DLOG(ERROR, "dc_categories_[%zu].Reset() failed.", i);
      return false;
    }
  }
  if (split_parse_and_decode_) {
    assert(residual_buffer_pool_ != nullptr);
    if (!residual_buffer_threaded_.Reset(superblock_rows_, superblock_columns_,
                                         /*zero_initialize=*/false)) {
      LIBGAV1_DLOG(ERROR, "residual_buffer_threaded_.Reset() failed.");
      return false;
    }
  } else {
    // Add 32 * |kResidualPaddingVertical| padding to avoid bottom boundary
    // checks when parsing quantized coefficients.
    residual_buffer_ = MakeAlignedUniquePtr<uint8_t>(
        32, (4096 + 32 * kResidualPaddingVertical) * residual_size_);
    if (residual_buffer_ == nullptr) {
      LIBGAV1_DLOG(ERROR, "Allocation of residual_buffer_ failed.");
      return false;
    }
    prediction_parameters_.reset(new (std::nothrow) PredictionParameters());
    if (prediction_parameters_ == nullptr) {
      LIBGAV1_DLOG(ERROR, "Allocation of prediction_parameters_ failed.");
      return false;
    }
  }
  if (frame_header_.use_ref_frame_mvs) {
    assert(sequence_header_.enable_order_hint);
    SetupMotionField(frame_header_, current_frame_, reference_frames_,
                     row4x4_start_, row4x4_end_, column4x4_start_,
                     column4x4_end_, &motion_field_);
  }
  ResetLoopRestorationParams();
  if (!top_context_.Resize(superblock_columns_)) {
    LIBGAV1_DLOG(ERROR, "Allocation of top_context_ failed.");
    return false;
  }
  return true;
}

template <ProcessingMode processing_mode, bool save_symbol_decoder_context>
bool Tile::ProcessSuperBlockRow(int row4x4,
                                TileScratchBuffer* const scratch_buffer) {
  if (row4x4 < row4x4_start_ || row4x4 >= row4x4_end_) return true;
  assert(scratch_buffer != nullptr);
  const int block_width4x4 = kNum4x4BlocksWide[SuperBlockSize()];
  for (int column4x4 = column4x4_start_; column4x4 < column4x4_end_;
       column4x4 += block_width4x4) {
    if (!ProcessSuperBlock(row4x4, column4x4, scratch_buffer,
                           processing_mode)) {
      LIBGAV1_DLOG(ERROR, "Error decoding super block row: %d column: %d",
                   row4x4, column4x4);
      return false;
    }
  }
  if (save_symbol_decoder_context && row4x4 + block_width4x4 >= row4x4_end_) {
    SaveSymbolDecoderContext();
  }
  if (processing_mode == kProcessingModeDecodeOnly ||
      processing_mode == kProcessingModeParseAndDecode) {
    PopulateIntraPredictionBuffer(row4x4);
  }
  return true;
}

// Used in frame parallel mode. The symbol decoder context need not be saved in
// this case since it was done when parsing was complete.
template bool Tile::ProcessSuperBlockRow<kProcessingModeDecodeOnly, false>(
    int row4x4, TileScratchBuffer* scratch_buffer);
// Used in non frame parallel mode.
template bool Tile::ProcessSuperBlockRow<kProcessingModeParseAndDecode, true>(
    int row4x4, TileScratchBuffer* scratch_buffer);

void Tile::SaveSymbolDecoderContext() {
  if (frame_header_.enable_frame_end_update_cdf &&
      number_ == frame_header_.tile_info.context_update_id) {
    *saved_symbol_decoder_context_ = symbol_decoder_context_;
  }
}

bool Tile::ParseAndDecode() {
  if (split_parse_and_decode_) {
    if (!ThreadedParseAndDecode()) return false;
    SaveSymbolDecoderContext();
    return true;
  }
  std::unique_ptr<TileScratchBuffer> scratch_buffer =
      tile_scratch_buffer_pool_->Get();
  if (scratch_buffer == nullptr) {
    pending_tiles_->Decrement(false);
    LIBGAV1_DLOG(ERROR, "Failed to get scratch buffer.");
    return false;
  }
  const int block_width4x4 = kNum4x4BlocksWide[SuperBlockSize()];
  for (int row4x4 = row4x4_start_; row4x4 < row4x4_end_;
       row4x4 += block_width4x4) {
    if (!ProcessSuperBlockRow<kProcessingModeParseAndDecode, true>(
            row4x4, scratch_buffer.get())) {
      pending_tiles_->Decrement(false);
      return false;
    }
  }
  tile_scratch_buffer_pool_->Release(std::move(scratch_buffer));
  pending_tiles_->Decrement(true);
  return true;
}

bool Tile::Parse() {
  const int block_width4x4 = kNum4x4BlocksWide[SuperBlockSize()];
  std::unique_ptr<TileScratchBuffer> scratch_buffer =
      tile_scratch_buffer_pool_->Get();
  if (scratch_buffer == nullptr) {
    LIBGAV1_DLOG(ERROR, "Failed to get scratch buffer.");
    return false;
  }
  for (int row4x4 = row4x4_start_; row4x4 < row4x4_end_;
       row4x4 += block_width4x4) {
    if (!ProcessSuperBlockRow<kProcessingModeParseOnly, false>(
            row4x4, scratch_buffer.get())) {
      return false;
    }
  }
  tile_scratch_buffer_pool_->Release(std::move(scratch_buffer));
  SaveSymbolDecoderContext();
  return true;
}

bool Tile::Decode(
    std::mutex* const mutex, int* const superblock_row_progress,
    std::condition_variable* const superblock_row_progress_condvar) {
  const int block_width4x4 = sequence_header_.use_128x128_superblock ? 32 : 16;
  const int block_width4x4_log2 =
      sequence_header_.use_128x128_superblock ? 5 : 4;
  std::unique_ptr<TileScratchBuffer> scratch_buffer =
      tile_scratch_buffer_pool_->Get();
  if (scratch_buffer == nullptr) {
    LIBGAV1_DLOG(ERROR, "Failed to get scratch buffer.");
    return false;
  }
  for (int row4x4 = row4x4_start_, index = row4x4_start_ >> block_width4x4_log2;
       row4x4 < row4x4_end_; row4x4 += block_width4x4, ++index) {
    if (!ProcessSuperBlockRow<kProcessingModeDecodeOnly, false>(
            row4x4, scratch_buffer.get())) {
      return false;
    }
    if (post_filter_.DoDeblock()) {
      // Apply vertical deblock filtering for all the columns in this tile
      // except for the first 64 columns.
      post_filter_.ApplyDeblockFilter(
          kLoopFilterTypeVertical, row4x4,
          column4x4_start_ + kNum4x4InLoopFilterUnit, column4x4_end_,
          block_width4x4);
      // If this is the first superblock row of the tile, then we cannot apply
      // horizontal deblocking here since we don't know if the top row is
      // available. So it will be done by the calling thread in that case.
      if (row4x4 != row4x4_start_) {
        // Apply horizontal deblock filtering for all the columns in this tile
        // except for the first and the last 64 columns.
        // Note about the last tile of each row: For the last tile,
        // column4x4_end may not be a multiple of 16. In that case it is still
        // okay to simply subtract 16 since ApplyDeblockFilter() will only do
        // the filters in increments of 64 columns (or 32 columns for chroma
        // with subsampling).
        post_filter_.ApplyDeblockFilter(
            kLoopFilterTypeHorizontal, row4x4,
            column4x4_start_ + kNum4x4InLoopFilterUnit,
            column4x4_end_ - kNum4x4InLoopFilterUnit, block_width4x4);
      }
    }
    bool notify;
    {
      std::unique_lock<std::mutex> lock(*mutex);
      notify = ++superblock_row_progress[index] ==
               frame_header_.tile_info.tile_columns;
    }
    if (notify) {
      // We are done decoding this superblock row. Notify the post filtering
      // thread.
      superblock_row_progress_condvar[index].notify_one();
    }
  }
  tile_scratch_buffer_pool_->Release(std::move(scratch_buffer));
  return true;
}

bool Tile::ThreadedParseAndDecode() {
  {
    std::lock_guard<std::mutex> lock(threading_.mutex);
    if (!threading_.sb_state.Reset(superblock_rows_, superblock_columns_)) {
      pending_tiles_->Decrement(false);
      LIBGAV1_DLOG(ERROR, "threading.sb_state.Reset() failed.");
      return false;
    }
    // Account for the parsing job.
    ++threading_.pending_jobs;
  }

  const int block_width4x4 = kNum4x4BlocksWide[SuperBlockSize()];

  // Begin parsing.
  std::unique_ptr<TileScratchBuffer> scratch_buffer =
      tile_scratch_buffer_pool_->Get();
  if (scratch_buffer == nullptr) {
    pending_tiles_->Decrement(false);
    LIBGAV1_DLOG(ERROR, "Failed to get scratch buffer.");
    return false;
  }
  for (int row4x4 = row4x4_start_, row_index = 0; row4x4 < row4x4_end_;
       row4x4 += block_width4x4, ++row_index) {
    for (int column4x4 = column4x4_start_, column_index = 0;
         column4x4 < column4x4_end_;
         column4x4 += block_width4x4, ++column_index) {
      if (!ProcessSuperBlock(row4x4, column4x4, scratch_buffer.get(),
                             kProcessingModeParseOnly)) {
        std::lock_guard<std::mutex> lock(threading_.mutex);
        threading_.abort = true;
        break;
      }
      std::unique_lock<std::mutex> lock(threading_.mutex);
      if (threading_.abort) break;
      threading_.sb_state[row_index][column_index] = kSuperBlockStateParsed;
      // Schedule the decoding of this superblock if it is allowed.
      if (CanDecode(row_index, column_index)) {
        ++threading_.pending_jobs;
        threading_.sb_state[row_index][column_index] =
            kSuperBlockStateScheduled;
        lock.unlock();
        thread_pool_->Schedule(
            [this, row_index, column_index, block_width4x4]() {
              DecodeSuperBlock(row_index, column_index, block_width4x4);
            });
      }
    }
    std::lock_guard<std::mutex> lock(threading_.mutex);
    if (threading_.abort) break;
  }
  tile_scratch_buffer_pool_->Release(std::move(scratch_buffer));

  // We are done parsing. We can return here since the calling thread will make
  // sure that it waits for all the superblocks to be decoded.
  //
  // Finish using |threading_| before |pending_tiles_->Decrement()| because the
  // Tile object could go out of scope as soon as |pending_tiles_->Decrement()|
  // is called.
  threading_.mutex.lock();
  const bool no_pending_jobs = (--threading_.pending_jobs == 0);
  const bool job_succeeded = !threading_.abort;
  threading_.mutex.unlock();
  if (no_pending_jobs) {
    // We are done parsing and decoding this tile.
    pending_tiles_->Decrement(job_succeeded);
  }
  return job_succeeded;
}

bool Tile::CanDecode(int row_index, int column_index) const {
  assert(row_index >= 0);
  assert(column_index >= 0);
  // If |threading_.sb_state[row_index][column_index]| is not equal to
  // kSuperBlockStateParsed, then return false. This is ok because if
  // |threading_.sb_state[row_index][column_index]| is equal to:
  //   kSuperBlockStateNone - then the superblock is not yet parsed.
  //   kSuperBlockStateScheduled - then the superblock is already scheduled for
  //                               decode.
  //   kSuperBlockStateDecoded - then the superblock has already been decoded.
  if (row_index >= superblock_rows_ || column_index >= superblock_columns_ ||
      threading_.sb_state[row_index][column_index] != kSuperBlockStateParsed) {
    return false;
  }
  // First superblock has no dependencies.
  if (row_index == 0 && column_index == 0) {
    return true;
  }
  // Superblocks in the first row only depend on the superblock to the left of
  // it.
  if (row_index == 0) {
    return threading_.sb_state[0][column_index - 1] == kSuperBlockStateDecoded;
  }
  // All other superblocks depend on superblock to the left of it (if one
  // exists) and superblock to the top right with a lag of
  // |intra_block_copy_lag_| (if one exists).
  const int top_right_column_index =
      std::min(column_index + intra_block_copy_lag_, superblock_columns_ - 1);
  return threading_.sb_state[row_index - 1][top_right_column_index] ==
             kSuperBlockStateDecoded &&
         (column_index == 0 ||
          threading_.sb_state[row_index][column_index - 1] ==
              kSuperBlockStateDecoded);
}

void Tile::DecodeSuperBlock(int row_index, int column_index,
                            int block_width4x4) {
  const int row4x4 = row4x4_start_ + (row_index * block_width4x4);
  const int column4x4 = column4x4_start_ + (column_index * block_width4x4);
  std::unique_ptr<TileScratchBuffer> scratch_buffer =
      tile_scratch_buffer_pool_->Get();
  bool ok = scratch_buffer != nullptr;
  if (ok) {
    ok = ProcessSuperBlock(row4x4, column4x4, scratch_buffer.get(),
                           kProcessingModeDecodeOnly);
    tile_scratch_buffer_pool_->Release(std::move(scratch_buffer));
  }
  std::unique_lock<std::mutex> lock(threading_.mutex);
  if (ok) {
    threading_.sb_state[row_index][column_index] = kSuperBlockStateDecoded;
    // Candidate rows and columns that we could potentially begin the decoding
    // (if it is allowed to do so). The candidates are:
    //   1) The superblock to the bottom-left of the current superblock with a
    //   lag of |intra_block_copy_lag_| (or the beginning of the next superblock
    //   row in case there are less than |intra_block_copy_lag_| superblock
    //   columns in the Tile).
    //   2) The superblock to the right of the current superblock.
    const int candidate_row_indices[] = {row_index + 1, row_index};
    const int candidate_column_indices[] = {
        std::max(0, column_index - intra_block_copy_lag_), column_index + 1};
    for (size_t i = 0; i < std::extent<decltype(candidate_row_indices)>::value;
         ++i) {
      const int candidate_row_index = candidate_row_indices[i];
      const int candidate_column_index = candidate_column_indices[i];
      if (!CanDecode(candidate_row_index, candidate_column_index)) {
        continue;
      }
      ++threading_.pending_jobs;
      threading_.sb_state[candidate_row_index][candidate_column_index] =
          kSuperBlockStateScheduled;
      lock.unlock();
      thread_pool_->Schedule([this, candidate_row_index, candidate_column_index,
                              block_width4x4]() {
        DecodeSuperBlock(candidate_row_index, candidate_column_index,
                         block_width4x4);
      });
      lock.lock();
    }
  } else {
    threading_.abort = true;
  }
  // Finish using |threading_| before |pending_tiles_->Decrement()| because the
  // Tile object could go out of scope as soon as |pending_tiles_->Decrement()|
  // is called.
  const bool no_pending_jobs = (--threading_.pending_jobs == 0);
  const bool job_succeeded = !threading_.abort;
  lock.unlock();
  if (no_pending_jobs) {
    // We are done parsing and decoding this tile.
    pending_tiles_->Decrement(job_succeeded);
  }
}

void Tile::PopulateIntraPredictionBuffer(int row4x4) {
  const int block_width4x4 = kNum4x4BlocksWide[SuperBlockSize()];
  if (!use_intra_prediction_buffer_ || row4x4 + block_width4x4 >= row4x4_end_) {
    return;
  }
  const size_t pixel_size =
      (sequence_header_.color_config.bitdepth == 8 ? sizeof(uint8_t)
                                                   : sizeof(uint16_t));
  for (int plane = kPlaneY; plane < PlaneCount(); ++plane) {
    const int row_to_copy =
        (MultiplyBy4(row4x4 + block_width4x4) >> subsampling_y_[plane]) - 1;
    const size_t pixels_to_copy =
        (MultiplyBy4(column4x4_end_ - column4x4_start_) >>
         subsampling_x_[plane]) *
        pixel_size;
    const size_t column_start =
        MultiplyBy4(column4x4_start_) >> subsampling_x_[plane];
    void* start;
#if LIBGAV1_MAX_BITDEPTH >= 10
    if (sequence_header_.color_config.bitdepth > 8) {
      Array2DView<uint16_t> buffer(
          buffer_[plane].rows(), buffer_[plane].columns() / sizeof(uint16_t),
          reinterpret_cast<uint16_t*>(&buffer_[plane][0][0]));
      start = &buffer[row_to_copy][column_start];
    } else  // NOLINT
#endif
    {
      start = &buffer_[plane][row_to_copy][column_start];
    }
    memcpy((*intra_prediction_buffer_)[plane].get() + column_start * pixel_size,
           start, pixels_to_copy);
  }
}

int Tile::GetTransformAllZeroContext(const Block& block, Plane plane,
                                     TransformSize tx_size, int x4, int y4,
                                     int w4, int h4) {
  const int max_x4x4 = frame_header_.columns4x4 >> subsampling_x_[plane];
  const int max_y4x4 = frame_header_.rows4x4 >> subsampling_y_[plane];

  const int tx_width = kTransformWidth[tx_size];
  const int tx_height = kTransformHeight[tx_size];
  const BlockSize plane_size = block.residual_size[plane];
  const int block_width = kBlockWidthPixels[plane_size];
  const int block_height = kBlockHeightPixels[plane_size];

  int top = 0;
  int left = 0;
  const int num_top_elements = GetNumElements(w4, x4, max_x4x4);
  const int num_left_elements = GetNumElements(h4, y4, max_y4x4);
  if (plane == kPlaneY) {
    if (block_width == tx_width && block_height == tx_height) return 0;
    const uint8_t* coefficient_levels =
        &coefficient_levels_[kEntropyContextTop][plane][x4];
    for (int i = 0; i < num_top_elements; ++i) {
      top = std::max(top, static_cast<int>(coefficient_levels[i]));
    }
    coefficient_levels = &coefficient_levels_[kEntropyContextLeft][plane][y4];
    for (int i = 0; i < num_left_elements; ++i) {
      left = std::max(left, static_cast<int>(coefficient_levels[i]));
    }
    assert(top <= 4);
    assert(left <= 4);
    // kAllZeroContextsByTopLeft is pre-computed based on the logic in the spec
    // for top and left.
    return kAllZeroContextsByTopLeft[top][left];
  }
  const uint8_t* coefficient_levels =
      &coefficient_levels_[kEntropyContextTop][plane][x4];
  const int8_t* dc_categories = &dc_categories_[kEntropyContextTop][plane][x4];
  for (int i = 0; i < num_top_elements; ++i) {
    top |= coefficient_levels[i];
    top |= dc_categories[i];
  }
  coefficient_levels = &coefficient_levels_[kEntropyContextLeft][plane][y4];
  dc_categories = &dc_categories_[kEntropyContextLeft][plane][y4];
  for (int i = 0; i < num_left_elements; ++i) {
    left |= coefficient_levels[i];
    left |= dc_categories[i];
  }
  return static_cast<int>(top != 0) + static_cast<int>(left != 0) + 7 +
         3 * static_cast<int>(block_width * block_height >
                              tx_width * tx_height);
}

TransformSet Tile::GetTransformSet(TransformSize tx_size, bool is_inter) const {
  const TransformSize tx_size_square_min = kTransformSizeSquareMin[tx_size];
  const TransformSize tx_size_square_max = kTransformSizeSquareMax[tx_size];
  if (tx_size_square_max == kTransformSize64x64) return kTransformSetDctOnly;
  if (is_inter) {
    if (frame_header_.reduced_tx_set ||
        tx_size_square_max == kTransformSize32x32) {
      return kTransformSetInter3;
    }
    if (tx_size_square_min == kTransformSize16x16) return kTransformSetInter2;
    return kTransformSetInter1;
  }
  if (tx_size_square_max == kTransformSize32x32) return kTransformSetDctOnly;
  if (frame_header_.reduced_tx_set ||
      tx_size_square_min == kTransformSize16x16) {
    return kTransformSetIntra2;
  }
  return kTransformSetIntra1;
}

TransformType Tile::ComputeTransformType(const Block& block, Plane plane,
                                         TransformSize tx_size, int block_x,
                                         int block_y) {
  const BlockParameters& bp = *block.bp;
  const TransformSize tx_size_square_max = kTransformSizeSquareMax[tx_size];
  if (frame_header_.segmentation
          .lossless[bp.prediction_parameters->segment_id] ||
      tx_size_square_max == kTransformSize64x64) {
    return kTransformTypeDctDct;
  }
  if (plane == kPlaneY) {
    return transform_types_[block_y - block.row4x4][block_x - block.column4x4];
  }
  const TransformSet tx_set = GetTransformSet(tx_size, bp.is_inter);
  TransformType tx_type;
  if (bp.is_inter) {
    const int x4 =
        std::max(block.column4x4, block_x << subsampling_x_[kPlaneU]);
    const int y4 = std::max(block.row4x4, block_y << subsampling_y_[kPlaneU]);
    tx_type = transform_types_[y4 - block.row4x4][x4 - block.column4x4];
  } else {
    tx_type = kModeToTransformType[bp.prediction_parameters->uv_mode];
  }
  return kTransformTypeInSetMask[tx_set].Contains(tx_type)
             ? tx_type
             : kTransformTypeDctDct;
}

void Tile::ReadTransformType(const Block& block, int x4, int y4,
                             TransformSize tx_size) {
  BlockParameters& bp = *block.bp;
  const TransformSet tx_set = GetTransformSet(tx_size, bp.is_inter);

  TransformType tx_type = kTransformTypeDctDct;
  if (tx_set != kTransformSetDctOnly &&
      frame_header_.segmentation.qindex[bp.prediction_parameters->segment_id] >
          0) {
    const int cdf_index = SymbolDecoderContext::TxTypeIndex(tx_set);
    const int cdf_tx_size_index =
        TransformSizeToSquareTransformIndex(kTransformSizeSquareMin[tx_size]);
    uint16_t* cdf;
    if (bp.is_inter) {
      cdf = symbol_decoder_context_
                .inter_tx_type_cdf[cdf_index][cdf_tx_size_index];
      switch (tx_set) {
        case kTransformSetInter1:
          tx_type = static_cast<TransformType>(reader_.ReadSymbol<16>(cdf));
          break;
        case kTransformSetInter2:
          tx_type = static_cast<TransformType>(reader_.ReadSymbol<12>(cdf));
          break;
        default:
          assert(tx_set == kTransformSetInter3);
          tx_type = static_cast<TransformType>(reader_.ReadSymbol(cdf));
          break;
      }
    } else {
      const PredictionMode intra_direction =
          block.bp->prediction_parameters->use_filter_intra
              ? kFilterIntraModeToIntraPredictor[block.bp->prediction_parameters
                                                     ->filter_intra_mode]
              : bp.y_mode;
      cdf =
          symbol_decoder_context_
              .intra_tx_type_cdf[cdf_index][cdf_tx_size_index][intra_direction];
      assert(tx_set == kTransformSetIntra1 || tx_set == kTransformSetIntra2);
      tx_type = static_cast<TransformType>((tx_set == kTransformSetIntra1)
                                               ? reader_.ReadSymbol<7>(cdf)
                                               : reader_.ReadSymbol<5>(cdf));
    }

    // This array does not contain an entry for kTransformSetDctOnly, so the
    // first dimension needs to be offset by 1.
    tx_type = kInverseTransformTypeBySet[tx_set - 1][tx_type];
  }
  SetTransformType(block, x4, y4, kTransformWidth4x4[tx_size],
                   kTransformHeight4x4[tx_size], tx_type, transform_types_);
}

// Section 8.3.2 in the spec, under coeff_base and coeff_br.
// Bottom boundary checks are avoided by the padded rows.
// For a coefficient near the right boundary, the two right neighbors and the
// one bottom-right neighbor may be out of boundary. We don't check the right
// boundary for them, because the out of boundary neighbors project to positions
// above the diagonal line which goes through the current coefficient and these
// positions are still all 0s according to the diagonal scan order.
template <typename ResidualType>
void Tile::ReadCoeffBase2D(
    const uint16_t* scan, TransformSize tx_size, int adjusted_tx_width_log2,
    int eob,
    uint16_t coeff_base_cdf[kCoeffBaseContexts][kCoeffBaseSymbolCount + 1],
    uint16_t coeff_base_range_cdf[kCoeffBaseRangeContexts]
                                 [kCoeffBaseRangeSymbolCount + 1],
    ResidualType* const quantized_buffer, uint8_t* const level_buffer) {
  const int tx_width = 1 << adjusted_tx_width_log2;
  for (int i = eob - 2; i >= 1; --i) {
    const uint16_t pos = scan[i];
    const int row = pos >> adjusted_tx_width_log2;
    const int column = pos & (tx_width - 1);
    auto* const quantized = &quantized_buffer[pos];
    auto* const levels = &level_buffer[pos];
    const int neighbor_sum = 1 + levels[1] + levels[tx_width] +
                             levels[tx_width + 1] + levels[2] +
                             levels[MultiplyBy2(tx_width)];
    const int context =
        ((neighbor_sum > 7) ? 4 : DivideBy2(neighbor_sum)) +
        kCoeffBaseContextOffset[tx_size][std::min(row, 4)][std::min(column, 4)];
    int level =
        reader_.ReadSymbol<kCoeffBaseSymbolCount>(coeff_base_cdf[context]);
    levels[0] = level;
    if (level > kNumQuantizerBaseLevels) {
      // No need to clip quantized values to COEFF_BASE_RANGE + NUM_BASE_LEVELS
      // + 1, because we clip the overall output to 6 and the unclipped
      // quantized values will always result in an output of greater than 6.
      int context = std::min(6, DivideBy2(1 + quantized[1] +          // {0, 1}
                                          quantized[tx_width] +       // {1, 0}
                                          quantized[tx_width + 1]));  // {1, 1}
      context += 14 >> static_cast<int>((row | column) < 2);
      level += ReadCoeffBaseRange(coeff_base_range_cdf[context]);
    }
    quantized[0] = level;
  }
  // Read position 0.
  {
    auto* const quantized = &quantized_buffer[0];
    int level = reader_.ReadSymbol<kCoeffBaseSymbolCount>(coeff_base_cdf[0]);
    level_buffer[0] = level;
    if (level > kNumQuantizerBaseLevels) {
      // No need to clip quantized values to COEFF_BASE_RANGE + NUM_BASE_LEVELS
      // + 1, because we clip the overall output to 6 and the unclipped
      // quantized values will always result in an output of greater than 6.
      const int context =
          std::min(6, DivideBy2(1 + quantized[1] +          // {0, 1}
                                quantized[tx_width] +       // {1, 0}
                                quantized[tx_width + 1]));  // {1, 1}
      level += ReadCoeffBaseRange(coeff_base_range_cdf[context]);
    }
    quantized[0] = level;
  }
}

// Section 8.3.2 in the spec, under coeff_base and coeff_br.
// Bottom boundary checks are avoided by the padded rows.
// For a coefficient near the right boundary, the four right neighbors may be
// out of boundary. We don't do the boundary check for the first three right
// neighbors, because even for the transform blocks with smallest width 4, the
// first three out of boundary neighbors project to positions left of the
// current coefficient and these positions are still all 0s according to the
// column scan order. However, when transform block width is 4 and the current
// coefficient is on the right boundary, its fourth right neighbor projects to
// the under position on the same column, which could be nonzero. Therefore, we
// must skip the fourth right neighbor. To make it simple, for any coefficient,
// we always do the boundary check for its fourth right neighbor.
template <typename ResidualType>
void Tile::ReadCoeffBaseHorizontal(
    const uint16_t* scan, TransformSize /*tx_size*/, int adjusted_tx_width_log2,
    int eob,
    uint16_t coeff_base_cdf[kCoeffBaseContexts][kCoeffBaseSymbolCount + 1],
    uint16_t coeff_base_range_cdf[kCoeffBaseRangeContexts]
                                 [kCoeffBaseRangeSymbolCount + 1],
    ResidualType* const quantized_buffer, uint8_t* const level_buffer) {
  const int tx_width = 1 << adjusted_tx_width_log2;
  int i = eob - 2;
  do {
    const uint16_t pos = scan[i];
    const int column = pos & (tx_width - 1);
    auto* const quantized = &quantized_buffer[pos];
    auto* const levels = &level_buffer[pos];
    const int neighbor_sum =
        1 + (levels[1] +                                  // {0, 1}
             levels[tx_width] +                           // {1, 0}
             levels[2] +                                  // {0, 2}
             levels[3] +                                  // {0, 3}
             ((column + 4 < tx_width) ? levels[4] : 0));  // {0, 4}
    const int context = ((neighbor_sum > 7) ? 4 : DivideBy2(neighbor_sum)) +
                        kCoeffBasePositionContextOffset[column];
    int level =
        reader_.ReadSymbol<kCoeffBaseSymbolCount>(coeff_base_cdf[context]);
    levels[0] = level;
    if (level > kNumQuantizerBaseLevels) {
      // No need to clip quantized values to COEFF_BASE_RANGE + NUM_BASE_LEVELS
      // + 1, because we clip the overall output to 6 and the unclipped
      // quantized values will always result in an output of greater than 6.
      int context = std::min(6, DivideBy2(1 + quantized[1] +     // {0, 1}
                                          quantized[tx_width] +  // {1, 0}
                                          quantized[2]));        // {0, 2}
      if (pos != 0) {
        context += 14 >> static_cast<int>(column == 0);
      }
      level += ReadCoeffBaseRange(coeff_base_range_cdf[context]);
    }
    quantized[0] = level;
  } while (--i >= 0);
}

// Section 8.3.2 in the spec, under coeff_base and coeff_br.
// Bottom boundary checks are avoided by the padded rows.
// Right boundary check is performed explicitly.
template <typename ResidualType>
void Tile::ReadCoeffBaseVertical(
    const uint16_t* scan, TransformSize /*tx_size*/, int adjusted_tx_width_log2,
    int eob,
    uint16_t coeff_base_cdf[kCoeffBaseContexts][kCoeffBaseSymbolCount + 1],
    uint16_t coeff_base_range_cdf[kCoeffBaseRangeContexts]
                                 [kCoeffBaseRangeSymbolCount + 1],
    ResidualType* const quantized_buffer, uint8_t* const level_buffer) {
  const int tx_width = 1 << adjusted_tx_width_log2;
  int i = eob - 2;
  do {
    const uint16_t pos = scan[i];
    const int row = pos >> adjusted_tx_width_log2;
    const int column = pos & (tx_width - 1);
    auto* const quantized = &quantized_buffer[pos];
    auto* const levels = &level_buffer[pos];
    const int neighbor_sum =
        1 + (((column + 1 < tx_width) ? levels[1] : 0) +  // {0, 1}
             levels[tx_width] +                           // {1, 0}
             levels[MultiplyBy2(tx_width)] +              // {2, 0}
             levels[tx_width * 3] +                       // {3, 0}
             levels[MultiplyBy4(tx_width)]);              // {4, 0}
    const int context = ((neighbor_sum > 7) ? 4 : DivideBy2(neighbor_sum)) +
                        kCoeffBasePositionContextOffset[row];
    int level =
        reader_.ReadSymbol<kCoeffBaseSymbolCount>(coeff_base_cdf[context]);
    levels[0] = level;
    if (level > kNumQuantizerBaseLevels) {
      // No need to clip quantized values to COEFF_BASE_RANGE + NUM_BASE_LEVELS
      // + 1, because we clip the overall output to 6 and the unclipped
      // quantized values will always result in an output of greater than 6.
      const int quantized_column1 = (column + 1 < tx_width) ? quantized[1] : 0;
      int context =
          std::min(6, DivideBy2(1 + quantized_column1 +              // {0, 1}
                                quantized[tx_width] +                // {1, 0}
                                quantized[MultiplyBy2(tx_width)]));  // {2, 0}
      if (pos != 0) {
        context += 14 >> static_cast<int>(row == 0);
      }
      level += ReadCoeffBaseRange(coeff_base_range_cdf[context]);
    }
    quantized[0] = level;
  } while (--i >= 0);
}

int Tile::GetDcSignContext(int x4, int y4, int w4, int h4, Plane plane) {
  const int max_x4x4 = frame_header_.columns4x4 >> subsampling_x_[plane];
  const int8_t* dc_categories = &dc_categories_[kEntropyContextTop][plane][x4];
  // Set dc_sign to 8-bit long so that std::accumulate() saves sign extension.
  int8_t dc_sign = std::accumulate(
      dc_categories, dc_categories + GetNumElements(w4, x4, max_x4x4), 0);
  const int max_y4x4 = frame_header_.rows4x4 >> subsampling_y_[plane];
  dc_categories = &dc_categories_[kEntropyContextLeft][plane][y4];
  dc_sign = std::accumulate(
      dc_categories, dc_categories + GetNumElements(h4, y4, max_y4x4), dc_sign);
  // This return statement is equivalent to:
  //   if (dc_sign < 0) return 1;
  //   if (dc_sign > 0) return 2;
  //   return 0;
  // And it is better than:
  //   return static_cast<int>(dc_sign != 0) + static_cast<int>(dc_sign > 0);
  return static_cast<int>(dc_sign < 0) +
         MultiplyBy2(static_cast<int>(dc_sign > 0));
}

void Tile::SetEntropyContexts(int x4, int y4, int w4, int h4, Plane plane,
                              uint8_t coefficient_level, int8_t dc_category) {
  const int max_x4x4 = frame_header_.columns4x4 >> subsampling_x_[plane];
  const int num_top_elements = GetNumElements(w4, x4, max_x4x4);
  memset(&coefficient_levels_[kEntropyContextTop][plane][x4], coefficient_level,
         num_top_elements);
  memset(&dc_categories_[kEntropyContextTop][plane][x4], dc_category,
         num_top_elements);
  const int max_y4x4 = frame_header_.rows4x4 >> subsampling_y_[plane];
  const int num_left_elements = GetNumElements(h4, y4, max_y4x4);
  memset(&coefficient_levels_[kEntropyContextLeft][plane][y4],
         coefficient_level, num_left_elements);
  memset(&dc_categories_[kEntropyContextLeft][plane][y4], dc_category,
         num_left_elements);
}

template <typename ResidualType, bool is_dc_coefficient>
bool Tile::ReadSignAndApplyDequantization(
    const uint16_t* const scan, int i, int q_value,
    const uint8_t* const quantizer_matrix, int shift, int max_value,
    uint16_t* const dc_sign_cdf, int8_t* const dc_category,
    int* const coefficient_level, ResidualType* residual_buffer) {
  const int pos = is_dc_coefficient ? 0 : scan[i];
  // If residual_buffer[pos] is zero, then the rest of the function has no
  // effect.
  int level = residual_buffer[pos];
  if (level == 0) return true;
  const int sign = is_dc_coefficient
                       ? static_cast<int>(reader_.ReadSymbol(dc_sign_cdf))
                       : reader_.ReadBit();
  if (level > kNumQuantizerBaseLevels + kQuantizerCoefficientBaseRange) {
    int length = 0;
    bool golomb_length_bit = false;
    do {
      golomb_length_bit = reader_.ReadBit() != 0;
      ++length;
      if (length > 20) {
        LIBGAV1_DLOG(ERROR, "Invalid golomb_length %d", length);
        return false;
      }
    } while (!golomb_length_bit);
    int x = 1;
    for (int i = length - 2; i >= 0; --i) {
      x = (x << 1) | reader_.ReadBit();
    }
    level += x - 1;
  }
  if (is_dc_coefficient) {
    *dc_category = (sign != 0) ? -1 : 1;
  }
  level &= 0xfffff;
  *coefficient_level += level;
  // Apply dequantization. Step 1 of section 7.12.3 in the spec.
  int q = q_value;
  if (quantizer_matrix != nullptr) {
    q = RightShiftWithRounding(q * quantizer_matrix[pos], 5);
  }
  // The intermediate multiplication can exceed 32 bits, so it has to be
  // performed by promoting one of the values to int64_t.
  int32_t dequantized_value = (static_cast<int64_t>(q) * level) & 0xffffff;
  dequantized_value >>= shift;
  // At this point:
  //   * |dequantized_value| is always non-negative.
  //   * |sign| can be either 0 or 1.
  //   * min_value = -(max_value + 1).
  // We need to apply the following:
  // dequantized_value = sign ? -dequantized_value : dequantized_value;
  // dequantized_value = Clip3(dequantized_value, min_value, max_value);
  //
  // Note that -x == ~(x - 1).
  //
  // Now, The above two lines can be done with a std::min and xor as follows:
  dequantized_value = std::min(dequantized_value - sign, max_value) ^ -sign;
  residual_buffer[pos] = dequantized_value;
  return true;
}

int Tile::ReadCoeffBaseRange(uint16_t* cdf) {
  int level = 0;
  for (int j = 0; j < kCoeffBaseRangeMaxIterations; ++j) {
    const int coeff_base_range =
        reader_.ReadSymbol<kCoeffBaseRangeSymbolCount>(cdf);
    level += coeff_base_range;
    if (coeff_base_range < (kCoeffBaseRangeSymbolCount - 1)) break;
  }
  return level;
}

template <typename ResidualType>
int Tile::ReadTransformCoefficients(const Block& block, Plane plane,
                                    int start_x, int start_y,
                                    TransformSize tx_size,
                                    TransformType* const tx_type) {
  const int x4 = DivideBy4(start_x);
  const int y4 = DivideBy4(start_y);
  const int w4 = kTransformWidth4x4[tx_size];
  const int h4 = kTransformHeight4x4[tx_size];
  const int tx_size_context = kTransformSizeContext[tx_size];
  int context =
      GetTransformAllZeroContext(block, plane, tx_size, x4, y4, w4, h4);
  const bool all_zero = reader_.ReadSymbol(
      symbol_decoder_context_.all_zero_cdf[tx_size_context][context]);
  if (all_zero) {
    if (plane == kPlaneY) {
      SetTransformType(block, x4, y4, w4, h4, kTransformTypeDctDct,
                       transform_types_);
    }
    SetEntropyContexts(x4, y4, w4, h4, plane, 0, 0);
    // This is not used in this case, so it can be set to any value.
    *tx_type = kNumTransformTypes;
    return 0;
  }
  const int tx_width = kTransformWidth[tx_size];
  const int tx_height = kTransformHeight[tx_size];
  const TransformSize adjusted_tx_size = kAdjustedTransformSize[tx_size];
  const int adjusted_tx_width_log2 = kTransformWidthLog2[adjusted_tx_size];
  const int tx_padding =
      (1 << adjusted_tx_width_log2) * kResidualPaddingVertical;
  auto* residual = reinterpret_cast<ResidualType*>(*block.residual);
  // Clear padding to avoid bottom boundary checks when parsing quantized
  // coefficients.
  memset(residual, 0, (tx_width * tx_height + tx_padding) * residual_size_);
  uint8_t level_buffer[(32 + kResidualPaddingVertical) * 32];
  memset(
      level_buffer, 0,
      kTransformWidth[adjusted_tx_size] * kTransformHeight[adjusted_tx_size] +
          tx_padding);
  const int clamped_tx_height = std::min(tx_height, 32);
  if (plane == kPlaneY) {
    ReadTransformType(block, x4, y4, tx_size);
  }
  BlockParameters& bp = *block.bp;
  *tx_type = ComputeTransformType(block, plane, tx_size, x4, y4);
  const int eob_multi_size = kEobMultiSizeLookup[tx_size];
  const PlaneType plane_type = GetPlaneType(plane);
  const TransformClass tx_class = GetTransformClass(*tx_type);
  context = static_cast<int>(tx_class != kTransformClass2D);
  int eob_pt = 1;
  switch (eob_multi_size) {
    case 0:
      eob_pt += reader_.ReadSymbol<kEobPt16SymbolCount>(
          symbol_decoder_context_.eob_pt_16_cdf[plane_type][context]);
      break;
    case 1:
      eob_pt += reader_.ReadSymbol<kEobPt32SymbolCount>(
          symbol_decoder_context_.eob_pt_32_cdf[plane_type][context]);
      break;
    case 2:
      eob_pt += reader_.ReadSymbol<kEobPt64SymbolCount>(
          symbol_decoder_context_.eob_pt_64_cdf[plane_type][context]);
      break;
    case 3:
      eob_pt += reader_.ReadSymbol<kEobPt128SymbolCount>(
          symbol_decoder_context_.eob_pt_128_cdf[plane_type][context]);
      break;
    case 4:
      eob_pt += reader_.ReadSymbol<kEobPt256SymbolCount>(
          symbol_decoder_context_.eob_pt_256_cdf[plane_type][context]);
      break;
    case 5:
      eob_pt += reader_.ReadSymbol<kEobPt512SymbolCount>(
          symbol_decoder_context_.eob_pt_512_cdf[plane_type]);
      break;
    case 6:
    default:
      eob_pt += reader_.ReadSymbol<kEobPt1024SymbolCount>(
          symbol_decoder_context_.eob_pt_1024_cdf[plane_type]);
      break;
  }
  int eob = (eob_pt < 2) ? eob_pt : ((1 << (eob_pt - 2)) + 1);
  if (eob_pt >= 3) {
    context = eob_pt - 3;
    const bool eob_extra = reader_.ReadSymbol(
        symbol_decoder_context_
            .eob_extra_cdf[tx_size_context][plane_type][context]);
    if (eob_extra) eob += 1 << (eob_pt - 3);
    for (int i = 1; i < eob_pt - 2; ++i) {
      assert(eob_pt - i >= 3);
      assert(eob_pt <= kEobPt1024SymbolCount);
      if (reader_.ReadBit() != 0) {
        eob += 1 << (eob_pt - i - 3);
      }
    }
  }
  const uint16_t* scan = kScan[tx_class][tx_size];
  const int clamped_tx_size_context = std::min(tx_size_context, 3);
  auto coeff_base_range_cdf =
      symbol_decoder_context_
          .coeff_base_range_cdf[clamped_tx_size_context][plane_type];
  // Read the last coefficient.
  {
    context = GetCoeffBaseContextEob(tx_size, eob - 1);
    const uint16_t pos = scan[eob - 1];
    int level =
        1 + reader_.ReadSymbol<kCoeffBaseEobSymbolCount>(
                symbol_decoder_context_
                    .coeff_base_eob_cdf[tx_size_context][plane_type][context]);
    level_buffer[pos] = level;
    if (level > kNumQuantizerBaseLevels) {
      level +=
          ReadCoeffBaseRange(coeff_base_range_cdf[GetCoeffBaseRangeContextEob(
              adjusted_tx_width_log2, pos, tx_class)]);
    }
    residual[pos] = level;
  }
  if (eob > 1) {
    // Read all the other coefficients.
    // Lookup used to call the right variant of ReadCoeffBase*() based on the
    // transform class.
    static constexpr void (Tile::*kGetCoeffBaseFunc[])(
        const uint16_t* scan, TransformSize tx_size, int adjusted_tx_width_log2,
        int eob,
        uint16_t coeff_base_cdf[kCoeffBaseContexts][kCoeffBaseSymbolCount + 1],
        uint16_t coeff_base_range_cdf[kCoeffBaseRangeContexts]
                                     [kCoeffBaseRangeSymbolCount + 1],
        ResidualType* quantized_buffer,
        uint8_t* level_buffer) = {&Tile::ReadCoeffBase2D<ResidualType>,
                                  &Tile::ReadCoeffBaseHorizontal<ResidualType>,
                                  &Tile::ReadCoeffBaseVertical<ResidualType>};
    (this->*kGetCoeffBaseFunc[tx_class])(
        scan, tx_size, adjusted_tx_width_log2, eob,
        symbol_decoder_context_.coeff_base_cdf[tx_size_context][plane_type],
        coeff_base_range_cdf, residual, level_buffer);
  }
  const int max_value = (1 << (7 + sequence_header_.color_config.bitdepth)) - 1;
  const int current_quantizer_index =
      GetQIndex(frame_header_.segmentation,
                bp.prediction_parameters->segment_id, current_quantizer_index_);
  const int dc_q_value = quantizer_.GetDcValue(plane, current_quantizer_index);
  const int ac_q_value = quantizer_.GetAcValue(plane, current_quantizer_index);
  const int shift = kQuantizationShift[tx_size];
  const uint8_t* const quantizer_matrix =
      (frame_header_.quantizer.use_matrix &&
       *tx_type < kTransformTypeIdentityIdentity &&
       !frame_header_.segmentation
            .lossless[bp.prediction_parameters->segment_id] &&
       frame_header_.quantizer.matrix_level[plane] < 15)
          ? quantizer_matrix_[frame_header_.quantizer.matrix_level[plane]]
                             [plane_type][adjusted_tx_size]
                                 .get()
          : nullptr;
  int coefficient_level = 0;
  int8_t dc_category = 0;
  uint16_t* const dc_sign_cdf =
      (residual[0] != 0)
          ? symbol_decoder_context_.dc_sign_cdf[plane_type][GetDcSignContext(
                x4, y4, w4, h4, plane)]
          : nullptr;
  assert(scan[0] == 0);
  if (!ReadSignAndApplyDequantization<ResidualType, /*is_dc_coefficient=*/true>(
          scan, 0, dc_q_value, quantizer_matrix, shift, max_value, dc_sign_cdf,
          &dc_category, &coefficient_level, residual)) {
    return -1;
  }
  if (eob > 1) {
    int i = 1;
    do {
      if (!ReadSignAndApplyDequantization<ResidualType,
                                          /*is_dc_coefficient=*/false>(
              scan, i, ac_q_value, quantizer_matrix, shift, max_value, nullptr,
              nullptr, &coefficient_level, residual)) {
        return -1;
      }
    } while (++i < eob);
    MoveCoefficientsForTxWidth64(clamped_tx_height, tx_width, residual);
  }
  SetEntropyContexts(x4, y4, w4, h4, plane, std::min(4, coefficient_level),
                     dc_category);
  if (split_parse_and_decode_) {
    *block.residual += tx_width * tx_height * residual_size_;
  }
  return eob;
}

// CALL_BITDEPTH_FUNCTION is a macro that calls the appropriate template
// |function| depending on the value of |sequence_header_.color_config.bitdepth|
// with the variadic arguments.
#if LIBGAV1_MAX_BITDEPTH >= 10
#define CALL_BITDEPTH_FUNCTION(function, ...)         \
  do {                                                \
    if (sequence_header_.color_config.bitdepth > 8) { \
      function<uint16_t>(__VA_ARGS__);                \
    } else {                                          \
      function<uint8_t>(__VA_ARGS__);                 \
    }                                                 \
  } while (false)
#else
#define CALL_BITDEPTH_FUNCTION(function, ...) \
  do {                                        \
    function<uint8_t>(__VA_ARGS__);           \
  } while (false)
#endif

bool Tile::TransformBlock(const Block& block, Plane plane, int base_x,
                          int base_y, TransformSize tx_size, int x, int y,
                          ProcessingMode mode) {
  BlockParameters& bp = *block.bp;
  const int subsampling_x = subsampling_x_[plane];
  const int subsampling_y = subsampling_y_[plane];
  const int start_x = base_x + MultiplyBy4(x);
  const int start_y = base_y + MultiplyBy4(y);
  const int max_x = MultiplyBy4(frame_header_.columns4x4) >> subsampling_x;
  const int max_y = MultiplyBy4(frame_header_.rows4x4) >> subsampling_y;
  if (start_x >= max_x || start_y >= max_y) return true;
  const int row = DivideBy4(start_y << subsampling_y);
  const int column = DivideBy4(start_x << subsampling_x);
  const int mask = sequence_header_.use_128x128_superblock ? 31 : 15;
  const int sub_block_row4x4 = row & mask;
  const int sub_block_column4x4 = column & mask;
  const int step_x = kTransformWidth4x4[tx_size];
  const int step_y = kTransformHeight4x4[tx_size];
  const bool do_decode = mode == kProcessingModeDecodeOnly ||
                         mode == kProcessingModeParseAndDecode;
  if (do_decode && !bp.is_inter) {
    if (bp.prediction_parameters->palette_mode_info.size[GetPlaneType(plane)] >
        0) {
      CALL_BITDEPTH_FUNCTION(PalettePrediction, block, plane, start_x, start_y,
                             x, y, tx_size);
    } else {
      const PredictionMode mode =
          (plane == kPlaneY) ? bp.y_mode
                             : (bp.prediction_parameters->uv_mode ==
                                        kPredictionModeChromaFromLuma
                                    ? kPredictionModeDc
                                    : bp.prediction_parameters->uv_mode);
      const int tr_row4x4 = (sub_block_row4x4 >> subsampling_y);
      const int tr_column4x4 =
          (sub_block_column4x4 >> subsampling_x) + step_x + 1;
      const int bl_row4x4 = (sub_block_row4x4 >> subsampling_y) + step_y + 1;
      const int bl_column4x4 = (sub_block_column4x4 >> subsampling_x);
      const bool has_left = x > 0 || block.left_available[plane];
      const bool has_top = y > 0 || block.top_available[plane];

      CALL_BITDEPTH_FUNCTION(
          IntraPrediction, block, plane, start_x, start_y, has_left, has_top,
          block.scratch_buffer->block_decoded[plane][tr_row4x4][tr_column4x4],
          block.scratch_buffer->block_decoded[plane][bl_row4x4][bl_column4x4],
          mode, tx_size);
      if (plane != kPlaneY &&
          bp.prediction_parameters->uv_mode == kPredictionModeChromaFromLuma) {
        CALL_BITDEPTH_FUNCTION(ChromaFromLumaPrediction, block, plane, start_x,
                               start_y, tx_size);
      }
    }
    if (plane == kPlaneY) {
      block.bp->prediction_parameters->max_luma_width =
          start_x + MultiplyBy4(step_x);
      block.bp->prediction_parameters->max_luma_height =
          start_y + MultiplyBy4(step_y);
      block.scratch_buffer->cfl_luma_buffer_valid = false;
    }
  }
  if (!bp.skip) {
    const int sb_row_index = SuperBlockRowIndex(block.row4x4);
    const int sb_column_index = SuperBlockColumnIndex(block.column4x4);
    if (mode == kProcessingModeDecodeOnly) {
      Queue<TransformParameters>& tx_params =
          *residual_buffer_threaded_[sb_row_index][sb_column_index]
               ->transform_parameters();
      ReconstructBlock(block, plane, start_x, start_y, tx_size,
                       tx_params.Front().type,
                       tx_params.Front().non_zero_coeff_count);
      tx_params.Pop();
    } else {
      TransformType tx_type;
      int non_zero_coeff_count;
#if LIBGAV1_MAX_BITDEPTH >= 10
      if (sequence_header_.color_config.bitdepth > 8) {
        non_zero_coeff_count = ReadTransformCoefficients<int32_t>(
            block, plane, start_x, start_y, tx_size, &tx_type);
      } else  // NOLINT
#endif
      {
        non_zero_coeff_count = ReadTransformCoefficients<int16_t>(
            block, plane, start_x, start_y, tx_size, &tx_type);
      }
      if (non_zero_coeff_count < 0) return false;
      if (mode == kProcessingModeParseAndDecode) {
        ReconstructBlock(block, plane, start_x, start_y, tx_size, tx_type,
                         non_zero_coeff_count);
      } else {
        assert(mode == kProcessingModeParseOnly);
        residual_buffer_threaded_[sb_row_index][sb_column_index]
            ->transform_parameters()
            ->Push(TransformParameters(tx_type, non_zero_coeff_count));
      }
    }
  }
  if (do_decode) {
    bool* block_decoded =
        &block.scratch_buffer
             ->block_decoded[plane][(sub_block_row4x4 >> subsampling_y) + 1]
                            [(sub_block_column4x4 >> subsampling_x) + 1];
    SetBlockValues<bool>(step_y, step_x, true, block_decoded,
                         TileScratchBuffer::kBlockDecodedStride);
  }
  return true;
}

bool Tile::TransformTree(const Block& block, int start_x, int start_y,
                         BlockSize plane_size, ProcessingMode mode) {
  assert(plane_size <= kBlock64x64);
  // Branching factor is 4; Maximum Depth is 4; So the maximum stack size
  // required is (4 - 1) * 4 + 1 = 13.
  Stack<TransformTreeNode, 13> stack;
  // It is okay to cast BlockSize to TransformSize here since the enum are
  // equivalent for all BlockSize values <= kBlock64x64.
  stack.Push(TransformTreeNode(start_x, start_y,
                               static_cast<TransformSize>(plane_size)));

  do {
    TransformTreeNode node = stack.Pop();
    const int row = DivideBy4(node.y);
    const int column = DivideBy4(node.x);
    if (row >= frame_header_.rows4x4 || column >= frame_header_.columns4x4) {
      continue;
    }
    const TransformSize inter_tx_size = inter_transform_sizes_[row][column];
    const int width = kTransformWidth[node.tx_size];
    const int height = kTransformHeight[node.tx_size];
    if (width <= kTransformWidth[inter_tx_size] &&
        height <= kTransformHeight[inter_tx_size]) {
      if (!TransformBlock(block, kPlaneY, node.x, node.y, node.tx_size, 0, 0,
                          mode)) {
        return false;
      }
      continue;
    }
    // The split transform size look up gives the right transform size that we
    // should push in the stack.
    //   if (width > height) => transform size whose width is half.
    //   if (width < height) => transform size whose height is half.
    //   if (width == height) => transform size whose width and height are half.
    const TransformSize split_tx_size = kSplitTransformSize[node.tx_size];
    const int half_width = DivideBy2(width);
    if (width > height) {
      stack.Push(TransformTreeNode(node.x + half_width, node.y, split_tx_size));
      stack.Push(TransformTreeNode(node.x, node.y, split_tx_size));
      continue;
    }
    const int half_height = DivideBy2(height);
    if (width < height) {
      stack.Push(
          TransformTreeNode(node.x, node.y + half_height, split_tx_size));
      stack.Push(TransformTreeNode(node.x, node.y, split_tx_size));
      continue;
    }
    stack.Push(TransformTreeNode(node.x + half_width, node.y + half_height,
                                 split_tx_size));
    stack.Push(TransformTreeNode(node.x, node.y + half_height, split_tx_size));
    stack.Push(TransformTreeNode(node.x + half_width, node.y, split_tx_size));
    stack.Push(TransformTreeNode(node.x, node.y, split_tx_size));
  } while (!stack.Empty());
  return true;
}

void Tile::ReconstructBlock(const Block& block, Plane plane, int start_x,
                            int start_y, TransformSize tx_size,
                            TransformType tx_type, int non_zero_coeff_count) {
  // Reconstruction process. Steps 2 and 3 of Section 7.12.3 in the spec.
  assert(non_zero_coeff_count >= 0);
  if (non_zero_coeff_count == 0) return;
#if LIBGAV1_MAX_BITDEPTH >= 10
  if (sequence_header_.color_config.bitdepth > 8) {
    Array2DView<uint16_t> buffer(
        buffer_[plane].rows(), buffer_[plane].columns() / sizeof(uint16_t),
        reinterpret_cast<uint16_t*>(&buffer_[plane][0][0]));
    Reconstruct(dsp_, tx_type, tx_size,
                frame_header_.segmentation
                    .lossless[block.bp->prediction_parameters->segment_id],
                reinterpret_cast<int32_t*>(*block.residual), start_x, start_y,
                &buffer, non_zero_coeff_count);
  } else  // NOLINT
#endif
  {
    Reconstruct(dsp_, tx_type, tx_size,
                frame_header_.segmentation
                    .lossless[block.bp->prediction_parameters->segment_id],
                reinterpret_cast<int16_t*>(*block.residual), start_x, start_y,
                &buffer_[plane], non_zero_coeff_count);
  }
  if (split_parse_and_decode_) {
    *block.residual +=
        kTransformWidth[tx_size] * kTransformHeight[tx_size] * residual_size_;
  }
}

bool Tile::Residual(const Block& block, ProcessingMode mode) {
  const int width_chunks = std::max(1, block.width >> 6);
  const int height_chunks = std::max(1, block.height >> 6);
  const BlockSize size_chunk4x4 =
      (width_chunks > 1 || height_chunks > 1) ? kBlock64x64 : block.size;
  const BlockParameters& bp = *block.bp;
  for (int chunk_y = 0; chunk_y < height_chunks; ++chunk_y) {
    for (int chunk_x = 0; chunk_x < width_chunks; ++chunk_x) {
      const int num_planes = block.HasChroma() ? PlaneCount() : 1;
      int plane = kPlaneY;
      do {
        const int subsampling_x = subsampling_x_[plane];
        const int subsampling_y = subsampling_y_[plane];
        // For Y Plane, when lossless is true |bp.transform_size| is always
        // kTransformSize4x4. So we can simply use |bp.transform_size| here as
        // the Y plane's transform size (part of Section 5.11.37 in the spec).
        const TransformSize tx_size =
            (plane == kPlaneY)
                ? inter_transform_sizes_[block.row4x4][block.column4x4]
                : bp.uv_transform_size;
        const BlockSize plane_size =
            kPlaneResidualSize[size_chunk4x4][subsampling_x][subsampling_y];
        assert(plane_size != kBlockInvalid);
        if (bp.is_inter &&
            !frame_header_.segmentation
                 .lossless[bp.prediction_parameters->segment_id] &&
            plane == kPlaneY) {
          const int row_chunk4x4 = block.row4x4 + MultiplyBy16(chunk_y);
          const int column_chunk4x4 = block.column4x4 + MultiplyBy16(chunk_x);
          const int base_x = MultiplyBy4(column_chunk4x4 >> subsampling_x);
          const int base_y = MultiplyBy4(row_chunk4x4 >> subsampling_y);
          if (!TransformTree(block, base_x, base_y, plane_size, mode)) {
            return false;
          }
        } else {
          const int base_x = MultiplyBy4(block.column4x4 >> subsampling_x);
          const int base_y = MultiplyBy4(block.row4x4 >> subsampling_y);
          const int step_x = kTransformWidth4x4[tx_size];
          const int step_y = kTransformHeight4x4[tx_size];
          const int num4x4_wide = kNum4x4BlocksWide[plane_size];
          const int num4x4_high = kNum4x4BlocksHigh[plane_size];
          for (int y = 0; y < num4x4_high; y += step_y) {
            for (int x = 0; x < num4x4_wide; x += step_x) {
              if (!TransformBlock(
                      block, static_cast<Plane>(plane), base_x, base_y, tx_size,
                      x + (MultiplyBy16(chunk_x) >> subsampling_x),
                      y + (MultiplyBy16(chunk_y) >> subsampling_y), mode)) {
                return false;
              }
            }
          }
        }
      } while (++plane < num_planes);
    }
  }
  return true;
}

// The purpose of this function is to limit the maximum size of motion vectors
// and also, if use_intra_block_copy is true, to additionally constrain the
// motion vector so that the data is fetched from parts of the tile that have
// already been decoded and are not too close to the current block (in order to
// make a pipelined decoder implementation feasible).
bool Tile::IsMvValid(const Block& block, bool is_compound) const {
  const BlockParameters& bp = *block.bp;
  for (int i = 0; i < 1 + static_cast<int>(is_compound); ++i) {
    for (int mv_component : bp.mv.mv[i].mv) {
      if (std::abs(mv_component) >= (1 << 14)) {
        return false;
      }
    }
  }
  if (!block.bp->prediction_parameters->use_intra_block_copy) {
    return true;
  }
  if ((bp.mv.mv[0].mv32 & 0x00070007) != 0) {
    return false;
  }
  const int delta_row = bp.mv.mv[0].mv[0] >> 3;
  const int delta_column = bp.mv.mv[0].mv[1] >> 3;
  int src_top_edge = MultiplyBy4(block.row4x4) + delta_row;
  int src_left_edge = MultiplyBy4(block.column4x4) + delta_column;
  const int src_bottom_edge = src_top_edge + block.height;
  const int src_right_edge = src_left_edge + block.width;
  if (block.HasChroma()) {
    if (block.width < 8 && subsampling_x_[kPlaneU] != 0) {
      src_left_edge -= 4;
    }
    if (block.height < 8 && subsampling_y_[kPlaneU] != 0) {
      src_top_edge -= 4;
    }
  }
  if (src_top_edge < MultiplyBy4(row4x4_start_) ||
      src_left_edge < MultiplyBy4(column4x4_start_) ||
      src_bottom_edge > MultiplyBy4(row4x4_end_) ||
      src_right_edge > MultiplyBy4(column4x4_end_)) {
    return false;
  }
  // sb_height_log2 = use_128x128_superblock ? log2(128) : log2(64)
  const int sb_height_log2 =
      6 + static_cast<int>(sequence_header_.use_128x128_superblock);
  const int active_sb_row = MultiplyBy4(block.row4x4) >> sb_height_log2;
  const int active_64x64_block_column = MultiplyBy4(block.column4x4) >> 6;
  const int src_sb_row = (src_bottom_edge - 1) >> sb_height_log2;
  const int src_64x64_block_column = (src_right_edge - 1) >> 6;
  const int total_64x64_blocks_per_row =
      ((column4x4_end_ - column4x4_start_ - 1) >> 4) + 1;
  const int active_64x64_block =
      active_sb_row * total_64x64_blocks_per_row + active_64x64_block_column;
  const int src_64x64_block =
      src_sb_row * total_64x64_blocks_per_row + src_64x64_block_column;
  if (src_64x64_block >= active_64x64_block - kIntraBlockCopyDelay64x64Blocks) {
    return false;
  }

  // Wavefront constraint: use only top left area of frame for reference.
  if (src_sb_row > active_sb_row) return false;
  const int gradient =
      1 + kIntraBlockCopyDelay64x64Blocks +
      static_cast<int>(sequence_header_.use_128x128_superblock);
  const int wavefront_offset = gradient * (active_sb_row - src_sb_row);
  return src_64x64_block_column < active_64x64_block_column -
                                      kIntraBlockCopyDelay64x64Blocks +
                                      wavefront_offset;
}

bool Tile::AssignInterMv(const Block& block, bool is_compound) {
  int min[2];
  int max[2];
  GetClampParameters(block, min, max);
  BlockParameters& bp = *block.bp;
  const PredictionParameters& prediction_parameters = *bp.prediction_parameters;
  bp.mv.mv64 = 0;
  if (is_compound) {
    for (int i = 0; i < 2; ++i) {
      const PredictionMode mode = GetSinglePredictionMode(i, bp.y_mode);
      MotionVector predicted_mv;
      if (mode == kPredictionModeGlobalMv) {
        predicted_mv = prediction_parameters.global_mv[i];
      } else {
        const int ref_mv_index = (mode == kPredictionModeNearestMv ||
                                  (mode == kPredictionModeNewMv &&
                                   prediction_parameters.ref_mv_count <= 1))
                                     ? 0
                                     : prediction_parameters.ref_mv_index;
        predicted_mv = prediction_parameters.reference_mv(ref_mv_index, i);
        if (ref_mv_index < prediction_parameters.ref_mv_count) {
          predicted_mv.mv[0] = Clip3(predicted_mv.mv[0], min[0], max[0]);
          predicted_mv.mv[1] = Clip3(predicted_mv.mv[1], min[1], max[1]);
        }
      }
      if (mode == kPredictionModeNewMv) {
        ReadMotionVector(block, i);
        bp.mv.mv[i].mv[0] += predicted_mv.mv[0];
        bp.mv.mv[i].mv[1] += predicted_mv.mv[1];
      } else {
        bp.mv.mv[i] = predicted_mv;
      }
    }
  } else {
    const PredictionMode mode = GetSinglePredictionMode(0, bp.y_mode);
    MotionVector predicted_mv;
    if (mode == kPredictionModeGlobalMv) {
      predicted_mv = prediction_parameters.global_mv[0];
    } else {
      const int ref_mv_index = (mode == kPredictionModeNearestMv ||
                                (mode == kPredictionModeNewMv &&
                                 prediction_parameters.ref_mv_count <= 1))
                                   ? 0
                                   : prediction_parameters.ref_mv_index;
      predicted_mv = prediction_parameters.reference_mv(ref_mv_index);
      if (ref_mv_index < prediction_parameters.ref_mv_count) {
        predicted_mv.mv[0] = Clip3(predicted_mv.mv[0], min[0], max[0]);
        predicted_mv.mv[1] = Clip3(predicted_mv.mv[1], min[1], max[1]);
      }
    }
    if (mode == kPredictionModeNewMv) {
      ReadMotionVector(block, 0);
      bp.mv.mv[0].mv[0] += predicted_mv.mv[0];
      bp.mv.mv[0].mv[1] += predicted_mv.mv[1];
    } else {
      bp.mv.mv[0] = predicted_mv;
    }
  }
  return IsMvValid(block, is_compound);
}

bool Tile::AssignIntraMv(const Block& block) {
  // TODO(linfengz): Check if the clamping process is necessary.
  int min[2];
  int max[2];
  GetClampParameters(block, min, max);
  BlockParameters& bp = *block.bp;
  const PredictionParameters& prediction_parameters = *bp.prediction_parameters;
  const MotionVector& ref_mv_0 = prediction_parameters.reference_mv(0);
  bp.mv.mv64 = 0;
  ReadMotionVector(block, 0);
  if (ref_mv_0.mv32 == 0) {
    const MotionVector& ref_mv_1 = prediction_parameters.reference_mv(1);
    if (ref_mv_1.mv32 == 0) {
      const int super_block_size4x4 = kNum4x4BlocksHigh[SuperBlockSize()];
      if (block.row4x4 - super_block_size4x4 < row4x4_start_) {
        bp.mv.mv[0].mv[1] -= MultiplyBy32(super_block_size4x4);
        bp.mv.mv[0].mv[1] -= MultiplyBy8(kIntraBlockCopyDelayPixels);
      } else {
        bp.mv.mv[0].mv[0] -= MultiplyBy32(super_block_size4x4);
      }
    } else {
      bp.mv.mv[0].mv[0] += Clip3(ref_mv_1.mv[0], min[0], max[0]);
      bp.mv.mv[0].mv[1] += Clip3(ref_mv_1.mv[1], min[0], max[0]);
    }
  } else {
    bp.mv.mv[0].mv[0] += Clip3(ref_mv_0.mv[0], min[0], max[0]);
    bp.mv.mv[0].mv[1] += Clip3(ref_mv_0.mv[1], min[1], max[1]);
  }
  return IsMvValid(block, /*is_compound=*/false);
}

void Tile::ResetEntropyContext(const Block& block) {
  const int num_planes = block.HasChroma() ? PlaneCount() : 1;
  int plane = kPlaneY;
  do {
    const int subsampling_x = subsampling_x_[plane];
    const int start_x = block.column4x4 >> subsampling_x;
    const int end_x =
        std::min((block.column4x4 + block.width4x4) >> subsampling_x,
                 frame_header_.columns4x4);
    memset(&coefficient_levels_[kEntropyContextTop][plane][start_x], 0,
           end_x - start_x);
    memset(&dc_categories_[kEntropyContextTop][plane][start_x], 0,
           end_x - start_x);
    const int subsampling_y = subsampling_y_[plane];
    const int start_y = block.row4x4 >> subsampling_y;
    const int end_y =
        std::min((block.row4x4 + block.height4x4) >> subsampling_y,
                 frame_header_.rows4x4);
    memset(&coefficient_levels_[kEntropyContextLeft][plane][start_y], 0,
           end_y - start_y);
    memset(&dc_categories_[kEntropyContextLeft][plane][start_y], 0,
           end_y - start_y);
  } while (++plane < num_planes);
}

bool Tile::ComputePrediction(const Block& block) {
  const BlockParameters& bp = *block.bp;
  if (!bp.is_inter) return true;
  const int mask =
      (1 << (4 + static_cast<int>(sequence_header_.use_128x128_superblock))) -
      1;
  const int sub_block_row4x4 = block.row4x4 & mask;
  const int sub_block_column4x4 = block.column4x4 & mask;
  const int plane_count = block.HasChroma() ? PlaneCount() : 1;
  // Returns true if this block applies local warping. The state is determined
  // in the Y plane and carried for use in the U/V planes.
  // But the U/V planes will not apply warping when the block size is smaller
  // than 8x8, even if this variable is true.
  bool is_local_valid = false;
  // Local warping parameters, similar usage as is_local_valid.
  GlobalMotion local_warp_params;
  int plane = kPlaneY;
  do {
    const int8_t subsampling_x = subsampling_x_[plane];
    const int8_t subsampling_y = subsampling_y_[plane];
    const BlockSize plane_size = block.residual_size[plane];
    const int block_width4x4 = kNum4x4BlocksWide[plane_size];
    const int block_height4x4 = kNum4x4BlocksHigh[plane_size];
    const int block_width = MultiplyBy4(block_width4x4);
    const int block_height = MultiplyBy4(block_height4x4);
    const int base_x = MultiplyBy4(block.column4x4 >> subsampling_x);
    const int base_y = MultiplyBy4(block.row4x4 >> subsampling_y);
    if (bp.reference_frame[1] == kReferenceFrameIntra) {
      const int tr_row4x4 = sub_block_row4x4 >> subsampling_y;
      const int tr_column4x4 =
          (sub_block_column4x4 >> subsampling_x) + block_width4x4 + 1;
      const int bl_row4x4 =
          (sub_block_row4x4 >> subsampling_y) + block_height4x4;
      const int bl_column4x4 = (sub_block_column4x4 >> subsampling_x) + 1;
      const TransformSize tx_size =
          k4x4SizeToTransformSize[k4x4WidthLog2[plane_size]]
                                 [k4x4HeightLog2[plane_size]];
      const bool has_left = block.left_available[plane];
      const bool has_top = block.top_available[plane];
      CALL_BITDEPTH_FUNCTION(
          IntraPrediction, block, static_cast<Plane>(plane), base_x, base_y,
          has_left, has_top,
          block.scratch_buffer->block_decoded[plane][tr_row4x4][tr_column4x4],
          block.scratch_buffer->block_decoded[plane][bl_row4x4][bl_column4x4],
          kInterIntraToIntraMode[block.bp->prediction_parameters
                                     ->inter_intra_mode],
          tx_size);
    }
    int candidate_row = block.row4x4;
    int candidate_column = block.column4x4;
    bool some_use_intra = bp.reference_frame[0] == kReferenceFrameIntra;
    if (!some_use_intra && plane != 0) {
      candidate_row = (candidate_row >> subsampling_y) << subsampling_y;
      candidate_column = (candidate_column >> subsampling_x) << subsampling_x;
      if (candidate_row != block.row4x4) {
        // Top block.
        const BlockParameters& bp_top =
            *block_parameters_holder_.Find(candidate_row, block.column4x4);
        some_use_intra = bp_top.reference_frame[0] == kReferenceFrameIntra;
        if (!some_use_intra && candidate_column != block.column4x4) {
          // Top-left block.
          const BlockParameters& bp_top_left =
              *block_parameters_holder_.Find(candidate_row, candidate_column);
          some_use_intra =
              bp_top_left.reference_frame[0] == kReferenceFrameIntra;
        }
      }
      if (!some_use_intra && candidate_column != block.column4x4) {
        // Left block.
        const BlockParameters& bp_left =
            *block_parameters_holder_.Find(block.row4x4, candidate_column);
        some_use_intra = bp_left.reference_frame[0] == kReferenceFrameIntra;
      }
    }
    int prediction_width;
    int prediction_height;
    if (some_use_intra) {
      candidate_row = block.row4x4;
      candidate_column = block.column4x4;
      prediction_width = block_width;
      prediction_height = block_height;
    } else {
      prediction_width = block.width >> subsampling_x;
      prediction_height = block.height >> subsampling_y;
    }
    int r = 0;
    int y = 0;
    do {
      int c = 0;
      int x = 0;
      do {
        if (!InterPrediction(block, static_cast<Plane>(plane), base_x + x,
                             base_y + y, prediction_width, prediction_height,
                             candidate_row + r, candidate_column + c,
                             &is_local_valid, &local_warp_params)) {
          return false;
        }
        ++c;
        x += prediction_width;
      } while (x < block_width);
      ++r;
      y += prediction_height;
    } while (y < block_height);
  } while (++plane < plane_count);
  return true;
}

#undef CALL_BITDEPTH_FUNCTION

void Tile::PopulateDeblockFilterLevel(const Block& block) {
  if (!post_filter_.DoDeblock()) return;
  BlockParameters& bp = *block.bp;
  const int mode_id =
      static_cast<int>(kPredictionModeDeltasMask.Contains(bp.y_mode));
  for (int i = 0; i < kFrameLfCount; ++i) {
    if (delta_lf_all_zero_) {
      bp.deblock_filter_level[i] = post_filter_.GetZeroDeltaDeblockFilterLevel(
          bp.prediction_parameters->segment_id, i, bp.reference_frame[0],
          mode_id);
    } else {
      bp.deblock_filter_level[i] =
          deblock_filter_levels_[bp.prediction_parameters->segment_id][i]
                                [bp.reference_frame[0]][mode_id];
    }
  }
}

void Tile::PopulateCdefSkip(const Block& block) {
  if (!post_filter_.DoCdef() || block.bp->skip ||
      (frame_header_.cdef.bits > 0 &&
       cdef_index_[DivideBy16(block.row4x4)][DivideBy16(block.column4x4)] ==
           -1)) {
    return;
  }
  // The rest of this function is an efficient version of the following code:
  // for (int y = block.row4x4; y < block.row4x4 + block.height4x4; y++) {
  //   for (int x = block.column4x4; y < block.column4x4 + block.width4x4;
  //        x++) {
  //     const uint8_t mask = uint8_t{1} << ((x >> 1) & 0x7);
  //     cdef_skip_[y >> 1][x >> 4] |= mask;
  //   }
  // }

  // For all block widths other than 32, the mask will fit in uint8_t. For
  // block width == 32, the mask is always 0xFFFF.
  const int bw4 =
      std::max(DivideBy2(block.width4x4) + (block.column4x4 & 1), 1);
  const uint8_t mask = (block.width4x4 == 32)
                           ? 0xFF
                           : (uint8_t{0xFF} >> (8 - bw4))
                                 << (DivideBy2(block.column4x4) & 0x7);
  uint8_t* cdef_skip = &cdef_skip_[block.row4x4 >> 1][block.column4x4 >> 4];
  const int stride = cdef_skip_.columns();
  int row = 0;
  do {
    *cdef_skip |= mask;
    if (block.width4x4 == 32) {
      *(cdef_skip + 1) = 0xFF;
    }
    cdef_skip += stride;
    row += 2;
  } while (row < block.height4x4);
}

bool Tile::ProcessBlock(int row4x4, int column4x4, BlockSize block_size,
                        TileScratchBuffer* const scratch_buffer,
                        ResidualPtr* residual) {
  // Do not process the block if the starting point is beyond the visible frame.
  // This is equivalent to the has_row/has_column check in the
  // decode_partition() section of the spec when partition equals
  // kPartitionHorizontal or kPartitionVertical.
  if (row4x4 >= frame_header_.rows4x4 ||
      column4x4 >= frame_header_.columns4x4) {
    return true;
  }

  if (split_parse_and_decode_) {
    // Push block ordering info to the queue. DecodeBlock() will use this queue
    // to decode the blocks in the correct order.
    const int sb_row_index = SuperBlockRowIndex(row4x4);
    const int sb_column_index = SuperBlockColumnIndex(column4x4);
    residual_buffer_threaded_[sb_row_index][sb_column_index]
        ->partition_tree_order()
        ->Push(PartitionTreeNode(row4x4, column4x4, block_size));
  }

  BlockParameters* bp_ptr =
      block_parameters_holder_.Get(row4x4, column4x4, block_size);
  if (bp_ptr == nullptr) {
    LIBGAV1_DLOG(ERROR, "Failed to get BlockParameters.");
    return false;
  }
  BlockParameters& bp = *bp_ptr;
  Block block(this, block_size, row4x4, column4x4, scratch_buffer, residual);
  bp.size = block_size;
  bp.prediction_parameters =
      split_parse_and_decode_ ? std::unique_ptr<PredictionParameters>(
                                    new (std::nothrow) PredictionParameters())
                              : std::move(prediction_parameters_);
  if (bp.prediction_parameters == nullptr) return false;
  if (!DecodeModeInfo(block)) return false;
  PopulateDeblockFilterLevel(block);
  if (!ReadPaletteTokens(block)) return false;
  DecodeTransformSize(block);
  // Part of Section 5.11.37 in the spec (implemented as a simple lookup).
  bp.uv_transform_size =
      frame_header_.segmentation.lossless[bp.prediction_parameters->segment_id]
          ? kTransformSize4x4
          : kUVTransformSize[block.residual_size[kPlaneU]];
  if (bp.skip) ResetEntropyContext(block);
  PopulateCdefSkip(block);
  if (split_parse_and_decode_) {
    if (!Residual(block, kProcessingModeParseOnly)) return false;
  } else {
    if (!ComputePrediction(block) ||
        !Residual(block, kProcessingModeParseAndDecode)) {
      return false;
    }
  }
  // If frame_header_.segmentation.enabled is false,
  // bp.prediction_parameters->segment_id is 0 for all blocks. We don't need to
  // call save bp.prediction_parameters->segment_id in the current frame because
  // the current frame's segmentation map will be cleared to all 0s.
  //
  // If frame_header_.segmentation.enabled is true and
  // frame_header_.segmentation.update_map is false, we will copy the previous
  // frame's segmentation map to the current frame. So we don't need to call
  // save bp.prediction_parameters->segment_id in the current frame.
  if (frame_header_.segmentation.enabled &&
      frame_header_.segmentation.update_map) {
    const int x_limit = std::min(frame_header_.columns4x4 - column4x4,
                                 static_cast<int>(block.width4x4));
    const int y_limit = std::min(frame_header_.rows4x4 - row4x4,
                                 static_cast<int>(block.height4x4));
    current_frame_.segmentation_map()->FillBlock(
        row4x4, column4x4, x_limit, y_limit,
        bp.prediction_parameters->segment_id);
  }
  StoreMotionFieldMvsIntoCurrentFrame(block);
  if (!split_parse_and_decode_) {
    prediction_parameters_ = std::move(bp.prediction_parameters);
  }
  return true;
}

bool Tile::DecodeBlock(int row4x4, int column4x4, BlockSize block_size,
                       TileScratchBuffer* const scratch_buffer,
                       ResidualPtr* residual) {
  if (row4x4 >= frame_header_.rows4x4 ||
      column4x4 >= frame_header_.columns4x4) {
    return true;
  }
  Block block(this, block_size, row4x4, column4x4, scratch_buffer, residual);
  if (!ComputePrediction(block) ||
      !Residual(block, kProcessingModeDecodeOnly)) {
    return false;
  }
  block.bp->prediction_parameters.reset(nullptr);
  return true;
}

bool Tile::ProcessPartition(int row4x4_start, int column4x4_start,
                            TileScratchBuffer* const scratch_buffer,
                            ResidualPtr* residual) {
  Stack<PartitionTreeNode, kDfsStackSize> stack;

  // Set up the first iteration.
  stack.Push(
      PartitionTreeNode(row4x4_start, column4x4_start, SuperBlockSize()));

  // DFS loop. If it sees a terminal node (leaf node), ProcessBlock is invoked.
  // Otherwise, the children are pushed into the stack for future processing.
  do {
    PartitionTreeNode node = stack.Pop();
    int row4x4 = node.row4x4;
    int column4x4 = node.column4x4;
    BlockSize block_size = node.block_size;

    if (row4x4 >= frame_header_.rows4x4 ||
        column4x4 >= frame_header_.columns4x4) {
      continue;
    }
    const int block_width4x4 = kNum4x4BlocksWide[block_size];
    assert(block_width4x4 == kNum4x4BlocksHigh[block_size]);
    const int half_block4x4 = block_width4x4 >> 1;
    const bool has_rows = (row4x4 + half_block4x4) < frame_header_.rows4x4;
    const bool has_columns =
        (column4x4 + half_block4x4) < frame_header_.columns4x4;
    Partition partition;
    if (!ReadPartition(row4x4, column4x4, block_size, has_rows, has_columns,
                       &partition)) {
      LIBGAV1_DLOG(ERROR, "Failed to read partition for row: %d column: %d",
                   row4x4, column4x4);
      return false;
    }
    const BlockSize sub_size = kSubSize[partition][block_size];
    // Section 6.10.4: It is a requirement of bitstream conformance that
    // get_plane_residual_size( subSize, 1 ) is not equal to BLOCK_INVALID
    // every time subSize is computed.
    if (sub_size == kBlockInvalid ||
        kPlaneResidualSize[sub_size]
                          [sequence_header_.color_config.subsampling_x]
                          [sequence_header_.color_config.subsampling_y] ==
            kBlockInvalid) {
      LIBGAV1_DLOG(
          ERROR,
          "Invalid sub-block/plane size for row: %d column: %d partition: "
          "%d block_size: %d sub_size: %d subsampling_x/y: %d, %d",
          row4x4, column4x4, partition, block_size, sub_size,
          sequence_header_.color_config.subsampling_x,
          sequence_header_.color_config.subsampling_y);
      return false;
    }

    const int quarter_block4x4 = half_block4x4 >> 1;
    const BlockSize split_size = kSubSize[kPartitionSplit][block_size];
    assert(partition == kPartitionNone || sub_size != kBlockInvalid);
    switch (partition) {
      case kPartitionNone:
        if (!ProcessBlock(row4x4, column4x4, sub_size, scratch_buffer,
                          residual)) {
          return false;
        }
        break;
      case kPartitionSplit:
        // The children must be added in reverse order since a stack is being
        // used.
        stack.Push(PartitionTreeNode(row4x4 + half_block4x4,
                                     column4x4 + half_block4x4, sub_size));
        stack.Push(
            PartitionTreeNode(row4x4 + half_block4x4, column4x4, sub_size));
        stack.Push(
            PartitionTreeNode(row4x4, column4x4 + half_block4x4, sub_size));
        stack.Push(PartitionTreeNode(row4x4, column4x4, sub_size));
        break;
      case kPartitionHorizontal:
        if (!ProcessBlock(row4x4, column4x4, sub_size, scratch_buffer,
                          residual) ||
            !ProcessBlock(row4x4 + half_block4x4, column4x4, sub_size,
                          scratch_buffer, residual)) {
          return false;
        }
        break;
      case kPartitionVertical:
        if (!ProcessBlock(row4x4, column4x4, sub_size, scratch_buffer,
                          residual) ||
            !ProcessBlock(row4x4, column4x4 + half_block4x4, sub_size,
                          scratch_buffer, residual)) {
          return false;
        }
        break;
      case kPartitionHorizontalWithTopSplit:
        if (!ProcessBlock(row4x4, column4x4, split_size, scratch_buffer,
                          residual) ||
            !ProcessBlock(row4x4, column4x4 + half_block4x4, split_size,
                          scratch_buffer, residual) ||
            !ProcessBlock(row4x4 + half_block4x4, column4x4, sub_size,
                          scratch_buffer, residual)) {
          return false;
        }
        break;
      case kPartitionHorizontalWithBottomSplit:
        if (!ProcessBlock(row4x4, column4x4, sub_size, scratch_buffer,
                          residual) ||
            !ProcessBlock(row4x4 + half_block4x4, column4x4, split_size,
                          scratch_buffer, residual) ||
            !ProcessBlock(row4x4 + half_block4x4, column4x4 + half_block4x4,
                          split_size, scratch_buffer, residual)) {
          return false;
        }
        break;
      case kPartitionVerticalWithLeftSplit:
        if (!ProcessBlock(row4x4, column4x4, split_size, scratch_buffer,
                          residual) ||
            !ProcessBlock(row4x4 + half_block4x4, column4x4, split_size,
                          scratch_buffer, residual) ||
            !ProcessBlock(row4x4, column4x4 + half_block4x4, sub_size,
                          scratch_buffer, residual)) {
          return false;
        }
        break;
      case kPartitionVerticalWithRightSplit:
        if (!ProcessBlock(row4x4, column4x4, sub_size, scratch_buffer,
                          residual) ||
            !ProcessBlock(row4x4, column4x4 + half_block4x4, split_size,
                          scratch_buffer, residual) ||
            !ProcessBlock(row4x4 + half_block4x4, column4x4 + half_block4x4,
                          split_size, scratch_buffer, residual)) {
          return false;
        }
        break;
      case kPartitionHorizontal4:
        for (int i = 0; i < 4; ++i) {
          if (!ProcessBlock(row4x4 + i * quarter_block4x4, column4x4, sub_size,
                            scratch_buffer, residual)) {
            return false;
          }
        }
        break;
      case kPartitionVertical4:
        for (int i = 0; i < 4; ++i) {
          if (!ProcessBlock(row4x4, column4x4 + i * quarter_block4x4, sub_size,
                            scratch_buffer, residual)) {
            return false;
          }
        }
        break;
    }
  } while (!stack.Empty());
  return true;
}

void Tile::ResetLoopRestorationParams() {
  for (int plane = kPlaneY; plane < kMaxPlanes; ++plane) {
    for (int i = WienerInfo::kVertical; i <= WienerInfo::kHorizontal; ++i) {
      reference_unit_info_[plane].sgr_proj_info.multiplier[i] =
          kSgrProjDefaultMultiplier[i];
      for (int j = 0; j < kNumWienerCoefficients; ++j) {
        reference_unit_info_[plane].wiener_info.filter[i][j] =
            kWienerDefaultFilter[j];
      }
    }
  }
}

void Tile::ResetCdef(const int row4x4, const int column4x4) {
  if (frame_header_.cdef.bits == 0) return;
  const int row = DivideBy16(row4x4);
  const int column = DivideBy16(column4x4);
  cdef_index_[row][column] = -1;
  if (sequence_header_.use_128x128_superblock) {
    const int cdef_size4x4 = kNum4x4BlocksWide[kBlock64x64];
    const int border_row = DivideBy16(row4x4 + cdef_size4x4);
    const int border_column = DivideBy16(column4x4 + cdef_size4x4);
    cdef_index_[row][border_column] = -1;
    cdef_index_[border_row][column] = -1;
    cdef_index_[border_row][border_column] = -1;
  }
}

void Tile::ClearBlockDecoded(TileScratchBuffer* const scratch_buffer,
                             int row4x4, int column4x4) {
  // Set everything to false.
  memset(scratch_buffer->block_decoded, 0,
         sizeof(scratch_buffer->block_decoded));
  // Set specific edge cases to true.
  const int sb_size4 = sequence_header_.use_128x128_superblock ? 32 : 16;
  for (int plane = kPlaneY; plane < PlaneCount(); ++plane) {
    const int subsampling_x = subsampling_x_[plane];
    const int subsampling_y = subsampling_y_[plane];
    const int sb_width4 = (column4x4_end_ - column4x4) >> subsampling_x;
    const int sb_height4 = (row4x4_end_ - row4x4) >> subsampling_y;
    // The memset is equivalent to the following lines in the spec:
    // for ( x = -1; x <= ( sbSize4 >> subX ); x++ ) {
    //   if ( y < 0 && x < sbWidth4 ) {
    //     BlockDecoded[plane][y][x] = 1
    //   }
    // }
    const int num_elements =
        std::min((sb_size4 >> subsampling_x_[plane]) + 1, sb_width4) + 1;
    memset(&scratch_buffer->block_decoded[plane][0][0], 1, num_elements);
    // The for loop is equivalent to the following lines in the spec:
    // for ( y = -1; y <= ( sbSize4 >> subY ); y++ )
    //   if ( x < 0 && y < sbHeight4 )
    //     BlockDecoded[plane][y][x] = 1
    //   }
    // }
    // BlockDecoded[plane][sbSize4 >> subY][-1] = 0
    for (int y = -1; y < std::min((sb_size4 >> subsampling_y), sb_height4);
         ++y) {
      scratch_buffer->block_decoded[plane][y + 1][0] = true;
    }
  }
}

bool Tile::ProcessSuperBlock(int row4x4, int column4x4,
                             TileScratchBuffer* const scratch_buffer,
                             ProcessingMode mode) {
  const bool parsing =
      mode == kProcessingModeParseOnly || mode == kProcessingModeParseAndDecode;
  const bool decoding = mode == kProcessingModeDecodeOnly ||
                        mode == kProcessingModeParseAndDecode;
  if (parsing) {
    read_deltas_ = frame_header_.delta_q.present;
    ResetCdef(row4x4, column4x4);
  }
  if (decoding) {
    ClearBlockDecoded(scratch_buffer, row4x4, column4x4);
  }
  const BlockSize block_size = SuperBlockSize();
  if (parsing) {
    ReadLoopRestorationCoefficients(row4x4, column4x4, block_size);
  }
  if (parsing && decoding) {
    uint8_t* residual_buffer = residual_buffer_.get();
    if (!ProcessPartition(row4x4, column4x4, scratch_buffer,
                          &residual_buffer)) {
      LIBGAV1_DLOG(ERROR, "Error decoding partition row: %d column: %d", row4x4,
                   column4x4);
      return false;
    }
    return true;
  }
  const int sb_row_index = SuperBlockRowIndex(row4x4);
  const int sb_column_index = SuperBlockColumnIndex(column4x4);
  if (parsing) {
    residual_buffer_threaded_[sb_row_index][sb_column_index] =
        residual_buffer_pool_->Get();
    if (residual_buffer_threaded_[sb_row_index][sb_column_index] == nullptr) {
      LIBGAV1_DLOG(ERROR, "Failed to get residual buffer.");
      return false;
    }
    uint8_t* residual_buffer =
        residual_buffer_threaded_[sb_row_index][sb_column_index]->buffer();
    if (!ProcessPartition(row4x4, column4x4, scratch_buffer,
                          &residual_buffer)) {
      LIBGAV1_DLOG(ERROR, "Error parsing partition row: %d column: %d", row4x4,
                   column4x4);
      return false;
    }
  } else {
    if (!DecodeSuperBlock(sb_row_index, sb_column_index, scratch_buffer)) {
      LIBGAV1_DLOG(ERROR, "Error decoding superblock row: %d column: %d",
                   row4x4, column4x4);
      return false;
    }
    residual_buffer_pool_->Release(
        std::move(residual_buffer_threaded_[sb_row_index][sb_column_index]));
  }
  return true;
}

bool Tile::DecodeSuperBlock(int sb_row_index, int sb_column_index,
                            TileScratchBuffer* const scratch_buffer) {
  uint8_t* residual_buffer =
      residual_buffer_threaded_[sb_row_index][sb_column_index]->buffer();
  Queue<PartitionTreeNode>& partition_tree_order =
      *residual_buffer_threaded_[sb_row_index][sb_column_index]
           ->partition_tree_order();
  while (!partition_tree_order.Empty()) {
    PartitionTreeNode block = partition_tree_order.Front();
    if (!DecodeBlock(block.row4x4, block.column4x4, block.block_size,
                     scratch_buffer, &residual_buffer)) {
      LIBGAV1_DLOG(ERROR, "Error decoding block row: %d column: %d",
                   block.row4x4, block.column4x4);
      return false;
    }
    partition_tree_order.Pop();
  }
  return true;
}

void Tile::ReadLoopRestorationCoefficients(int row4x4, int column4x4,
                                           BlockSize block_size) {
  if (frame_header_.allow_intrabc) return;
  LoopRestorationInfo* const restoration_info = post_filter_.restoration_info();
  const bool is_superres_scaled =
      frame_header_.width != frame_header_.upscaled_width;
  for (int plane = kPlaneY; plane < PlaneCount(); ++plane) {
    LoopRestorationUnitInfo unit_info;
    if (restoration_info->PopulateUnitInfoForSuperBlock(
            static_cast<Plane>(plane), block_size, is_superres_scaled,
            frame_header_.superres_scale_denominator, row4x4, column4x4,
            &unit_info)) {
      for (int unit_row = unit_info.row_start; unit_row < unit_info.row_end;
           ++unit_row) {
        for (int unit_column = unit_info.column_start;
             unit_column < unit_info.column_end; ++unit_column) {
          const int unit_id = unit_row * restoration_info->num_horizontal_units(
                                             static_cast<Plane>(plane)) +
                              unit_column;
          restoration_info->ReadUnitCoefficients(
              &reader_, &symbol_decoder_context_, static_cast<Plane>(plane),
              unit_id, &reference_unit_info_);
        }
      }
    }
  }
}

void Tile::StoreMotionFieldMvsIntoCurrentFrame(const Block& block) {
  if (frame_header_.refresh_frame_flags == 0 ||
      IsIntraFrame(frame_header_.frame_type)) {
    return;
  }
  // Iterate over odd rows/columns beginning at the first odd row/column for the
  // block. It is done this way because motion field mvs are only needed at a
  // 8x8 granularity.
  const int row_start4x4 = block.row4x4 | 1;
  const int row_limit4x4 =
      std::min(block.row4x4 + block.height4x4, frame_header_.rows4x4);
  if (row_start4x4 >= row_limit4x4) return;
  const int column_start4x4 = block.column4x4 | 1;
  const int column_limit4x4 =
      std::min(block.column4x4 + block.width4x4, frame_header_.columns4x4);
  if (column_start4x4 >= column_limit4x4) return;

  // The largest reference MV component that can be saved.
  constexpr int kRefMvsLimit = (1 << 12) - 1;
  const BlockParameters& bp = *block.bp;
  ReferenceInfo* reference_info = current_frame_.reference_info();
  for (int i = 1; i >= 0; --i) {
    const ReferenceFrameType reference_frame_to_store = bp.reference_frame[i];
    // Must make a local copy so that StoreMotionFieldMvs() knows there is no
    // overlap between load and store.
    const MotionVector mv_to_store = bp.mv.mv[i];
    const int mv_row = std::abs(mv_to_store.mv[0]);
    const int mv_column = std::abs(mv_to_store.mv[1]);
    if (reference_frame_to_store > kReferenceFrameIntra &&
        // kRefMvsLimit equals 0x07FF, so we can first bitwise OR the two
        // absolute values and then compare with kRefMvsLimit to save a branch.
        // The next line is equivalent to:
        // mv_row <= kRefMvsLimit && mv_column <= kRefMvsLimit
        (mv_row | mv_column) <= kRefMvsLimit &&
        reference_info->relative_distance_from[reference_frame_to_store] < 0) {
      const int row_start8x8 = DivideBy2(row_start4x4);
      const int row_limit8x8 = DivideBy2(row_limit4x4);
      const int column_start8x8 = DivideBy2(column_start4x4);
      const int column_limit8x8 = DivideBy2(column_limit4x4);
      const int rows = row_limit8x8 - row_start8x8;
      const int columns = column_limit8x8 - column_start8x8;
      const ptrdiff_t stride = DivideBy2(current_frame_.columns4x4());
      ReferenceFrameType* const reference_frame_row_start =
          &reference_info
               ->motion_field_reference_frame[row_start8x8][column_start8x8];
      MotionVector* const mv =
          &reference_info->motion_field_mv[row_start8x8][column_start8x8];

      // Specialize columns cases 1, 2, 4, 8 and 16. This makes memset() inlined
      // and simplifies std::fill() for these cases.
      if (columns <= 1) {
        // Don't change the above condition to (columns == 1).
        // Condition (columns <= 1) may help the compiler simplify the inlining
        // of the general case of StoreMotionFieldMvs() by eliminating the
        // (columns == 0) case.
        assert(columns == 1);
        StoreMotionFieldMvs(reference_frame_to_store, mv_to_store, stride, rows,
                            1, reference_frame_row_start, mv);
      } else if (columns == 2) {
        StoreMotionFieldMvs(reference_frame_to_store, mv_to_store, stride, rows,
                            2, reference_frame_row_start, mv);
      } else if (columns == 4) {
        StoreMotionFieldMvs(reference_frame_to_store, mv_to_store, stride, rows,
                            4, reference_frame_row_start, mv);
      } else if (columns == 8) {
        StoreMotionFieldMvs(reference_frame_to_store, mv_to_store, stride, rows,
                            8, reference_frame_row_start, mv);
      } else if (columns == 16) {
        StoreMotionFieldMvs(reference_frame_to_store, mv_to_store, stride, rows,
                            16, reference_frame_row_start, mv);
      } else if (columns < 16) {
        // This always true condition (columns < 16) may help the compiler
        // simplify the inlining of the following function.
        // This general case is rare and usually only happens to the blocks
        // which contain the right boundary of the frame.
        StoreMotionFieldMvs(reference_frame_to_store, mv_to_store, stride, rows,
                            columns, reference_frame_row_start, mv);
      } else {
        assert(false);
      }
      return;
    }
  }
}

}  // namespace libgav1
