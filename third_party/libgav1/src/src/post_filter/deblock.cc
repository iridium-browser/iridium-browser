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
#include <atomic>

#include "src/post_filter.h"

namespace libgav1 {
namespace {

constexpr uint8_t HevThresh(int level) { return DivideBy16(level); }

// GetLoopFilterSize* functions depend on this exact ordering of the
// LoopFilterSize enums.
static_assert(dsp::kLoopFilterSize4 == 0, "");
static_assert(dsp::kLoopFilterSize6 == 1, "");
static_assert(dsp::kLoopFilterSize8 == 2, "");
static_assert(dsp::kLoopFilterSize14 == 3, "");

dsp::LoopFilterSize GetLoopFilterSizeY(int filter_length) {
  // |filter_length| must be a power of 2.
  assert((filter_length & (filter_length - 1)) == 0);
  // This code is the branch free equivalent of:
  //   if (filter_length == 4) return kLoopFilterSize4;
  //   if (filter_length == 8) return kLoopFilterSize8;
  //   return kLoopFilterSize14;
  return static_cast<dsp::LoopFilterSize>(
      MultiplyBy2(static_cast<int>(filter_length > 4)) +
      static_cast<int>(filter_length > 8));
}

constexpr dsp::LoopFilterSize GetLoopFilterSizeUV(int filter_length) {
  // For U & V planes, size is kLoopFilterSize4 if |filter_length| is 4,
  // otherwise size is kLoopFilterSize6.
  return static_cast<dsp::LoopFilterSize>(filter_length != 4);
}

bool NonBlockBorderNeedsFilter(const BlockParameters& bp, int filter_id,
                               uint8_t* const level) {
  if (bp.deblock_filter_level[filter_id] == 0 || (bp.skip && bp.is_inter)) {
    return false;
  }
  *level = bp.deblock_filter_level[filter_id];
  return true;
}

// 7.14.5.
void ComputeDeblockFilterLevelsHelper(
    const ObuFrameHeader& frame_header, int segment_id, int level_index,
    const int8_t delta_lf[kFrameLfCount],
    uint8_t deblock_filter_levels[kNumReferenceFrameTypes][2]) {
  const int delta = delta_lf[frame_header.delta_lf.multi ? level_index : 0];
  uint8_t level = Clip3(frame_header.loop_filter.level[level_index] + delta, 0,
                        kMaxLoopFilterValue);
  const auto feature = static_cast<SegmentFeature>(
      kSegmentFeatureLoopFilterYVertical + level_index);
  level =
      Clip3(level + frame_header.segmentation.feature_data[segment_id][feature],
            0, kMaxLoopFilterValue);
  if (!frame_header.loop_filter.delta_enabled) {
    static_assert(sizeof(deblock_filter_levels[0][0]) == 1, "");
    memset(deblock_filter_levels, level, kNumReferenceFrameTypes * 2);
    return;
  }
  assert(frame_header.loop_filter.delta_enabled);
  const int shift = level >> 5;
  deblock_filter_levels[kReferenceFrameIntra][0] = Clip3(
      level +
          LeftShift(frame_header.loop_filter.ref_deltas[kReferenceFrameIntra],
                    shift),
      0, kMaxLoopFilterValue);
  // deblock_filter_levels[kReferenceFrameIntra][1] is never used. So it does
  // not have to be populated.
  for (int reference_frame = kReferenceFrameIntra + 1;
       reference_frame < kNumReferenceFrameTypes; ++reference_frame) {
    for (int mode_id = 0; mode_id < 2; ++mode_id) {
      deblock_filter_levels[reference_frame][mode_id] = Clip3(
          level +
              LeftShift(frame_header.loop_filter.ref_deltas[reference_frame] +
                            frame_header.loop_filter.mode_deltas[mode_id],
                        shift),
          0, kMaxLoopFilterValue);
    }
  }
}

}  // namespace

void PostFilter::ComputeDeblockFilterLevels(
    const int8_t delta_lf[kFrameLfCount],
    uint8_t deblock_filter_levels[kMaxSegments][kFrameLfCount]
                                 [kNumReferenceFrameTypes][2]) const {
  if (!DoDeblock()) return;
  const int num_segments =
      frame_header_.segmentation.enabled ? kMaxSegments : 1;
  for (int segment_id = 0; segment_id < num_segments; ++segment_id) {
    int level_index = 0;
    for (; level_index < 2; ++level_index) {
      ComputeDeblockFilterLevelsHelper(
          frame_header_, segment_id, level_index, delta_lf,
          deblock_filter_levels[segment_id][level_index]);
    }
    for (; level_index < kFrameLfCount; ++level_index) {
      if (frame_header_.loop_filter.level[level_index] != 0) {
        ComputeDeblockFilterLevelsHelper(
            frame_header_, segment_id, level_index, delta_lf,
            deblock_filter_levels[segment_id][level_index]);
      }
    }
  }
}

bool PostFilter::GetHorizontalDeblockFilterEdgeInfo(int row4x4, int column4x4,
                                                    uint8_t* level, int* step,
                                                    int* filter_length) const {
  *step = kTransformHeight[inter_transform_sizes_[row4x4][column4x4]];
  if (row4x4 == 0) return false;

  const BlockParameters* bp = block_parameters_.Find(row4x4, column4x4);
  const int row4x4_prev = row4x4 - 1;
  assert(row4x4_prev >= 0);
  const BlockParameters* bp_prev =
      block_parameters_.Find(row4x4_prev, column4x4);

  if (bp == bp_prev) {
    // Not a border.
    if (!NonBlockBorderNeedsFilter(*bp, 1, level)) return false;
  } else {
    const uint8_t level_this = bp->deblock_filter_level[1];
    *level = level_this;
    if (level_this == 0) {
      const uint8_t level_prev = bp_prev->deblock_filter_level[1];
      if (level_prev == 0) return false;
      *level = level_prev;
    }
  }
  const int step_prev =
      kTransformHeight[inter_transform_sizes_[row4x4_prev][column4x4]];
  *filter_length = std::min(*step, step_prev);
  return true;
}

void PostFilter::GetHorizontalDeblockFilterEdgeInfoUV(
    int row4x4, int column4x4, uint8_t* level_u, uint8_t* level_v, int* step,
    int* filter_length) const {
  const int subsampling_x = subsampling_x_[kPlaneU];
  const int subsampling_y = subsampling_y_[kPlaneU];
  row4x4 = GetDeblockPosition(row4x4, subsampling_y);
  column4x4 = GetDeblockPosition(column4x4, subsampling_x);
  const BlockParameters* bp = block_parameters_.Find(row4x4, column4x4);
  *level_u = 0;
  *level_v = 0;
  *step = kTransformHeight[bp->uv_transform_size];
  if (row4x4 == subsampling_y) {
    return;
  }

  bool need_filter_u = frame_header_.loop_filter.level[kPlaneU + 1] != 0;
  bool need_filter_v = frame_header_.loop_filter.level[kPlaneV + 1] != 0;
  assert(need_filter_u || need_filter_v);
  const int filter_id_u =
      kDeblockFilterLevelIndex[kPlaneU][kLoopFilterTypeHorizontal];
  const int filter_id_v =
      kDeblockFilterLevelIndex[kPlaneV][kLoopFilterTypeHorizontal];
  const int row4x4_prev = row4x4 - (1 << subsampling_y);
  assert(row4x4_prev >= 0);
  const BlockParameters* bp_prev =
      block_parameters_.Find(row4x4_prev, column4x4);

  if (bp == bp_prev) {
    // Not a border.
    const bool skip = bp->skip && bp->is_inter;
    need_filter_u =
        need_filter_u && bp->deblock_filter_level[filter_id_u] != 0 && !skip;
    need_filter_v =
        need_filter_v && bp->deblock_filter_level[filter_id_v] != 0 && !skip;
    if (!need_filter_u && !need_filter_v) return;
    if (need_filter_u) *level_u = bp->deblock_filter_level[filter_id_u];
    if (need_filter_v) *level_v = bp->deblock_filter_level[filter_id_v];
    *filter_length = *step;
    return;
  }

  // It is a border.
  if (need_filter_u) {
    const uint8_t level_u_this = bp->deblock_filter_level[filter_id_u];
    *level_u = level_u_this;
    if (level_u_this == 0) {
      *level_u = bp_prev->deblock_filter_level[filter_id_u];
    }
  }
  if (need_filter_v) {
    const uint8_t level_v_this = bp->deblock_filter_level[filter_id_v];
    *level_v = level_v_this;
    if (level_v_this == 0) {
      *level_v = bp_prev->deblock_filter_level[filter_id_v];
    }
  }
  const int step_prev = kTransformHeight[bp_prev->uv_transform_size];
  *filter_length = std::min(*step, step_prev);
}

bool PostFilter::GetVerticalDeblockFilterEdgeInfo(
    int row4x4, int column4x4, BlockParameters* const* bp_ptr, uint8_t* level,
    int* step, int* filter_length) const {
  const BlockParameters* bp = *bp_ptr;
  *step = kTransformWidth[inter_transform_sizes_[row4x4][column4x4]];
  if (column4x4 == 0) return false;

  const int filter_id = 0;
  const int column4x4_prev = column4x4 - 1;
  assert(column4x4_prev >= 0);
  const BlockParameters* bp_prev = *(bp_ptr - 1);
  if (bp == bp_prev) {
    // Not a border.
    if (!NonBlockBorderNeedsFilter(*bp, filter_id, level)) return false;
  } else {
    // It is a border.
    const uint8_t level_this = bp->deblock_filter_level[filter_id];
    *level = level_this;
    if (level_this == 0) {
      const uint8_t level_prev = bp_prev->deblock_filter_level[filter_id];
      if (level_prev == 0) return false;
      *level = level_prev;
    }
  }
  const int step_prev =
      kTransformWidth[inter_transform_sizes_[row4x4][column4x4_prev]];
  *filter_length = std::min(*step, step_prev);
  return true;
}

void PostFilter::GetVerticalDeblockFilterEdgeInfoUV(
    int column4x4, BlockParameters* const* bp_ptr, uint8_t* level_u,
    uint8_t* level_v, int* step, int* filter_length) const {
  const int subsampling_x = subsampling_x_[kPlaneU];
  column4x4 = GetDeblockPosition(column4x4, subsampling_x);
  const BlockParameters* bp = *bp_ptr;
  *level_u = 0;
  *level_v = 0;
  *step = kTransformWidth[bp->uv_transform_size];
  if (column4x4 == subsampling_x) {
    return;
  }

  bool need_filter_u = frame_header_.loop_filter.level[kPlaneU + 1] != 0;
  bool need_filter_v = frame_header_.loop_filter.level[kPlaneV + 1] != 0;
  assert(need_filter_u || need_filter_v);
  const int filter_id_u =
      kDeblockFilterLevelIndex[kPlaneU][kLoopFilterTypeVertical];
  const int filter_id_v =
      kDeblockFilterLevelIndex[kPlaneV][kLoopFilterTypeVertical];
  const BlockParameters* bp_prev = *(bp_ptr - (ptrdiff_t{1} << subsampling_x));

  if (bp == bp_prev) {
    // Not a border.
    const bool skip = bp->skip && bp->is_inter;
    need_filter_u =
        need_filter_u && bp->deblock_filter_level[filter_id_u] != 0 && !skip;
    need_filter_v =
        need_filter_v && bp->deblock_filter_level[filter_id_v] != 0 && !skip;
    if (!need_filter_u && !need_filter_v) return;
    if (need_filter_u) *level_u = bp->deblock_filter_level[filter_id_u];
    if (need_filter_v) *level_v = bp->deblock_filter_level[filter_id_v];
    *filter_length = *step;
    return;
  }

  // It is a border.
  if (need_filter_u) {
    const uint8_t level_u_this = bp->deblock_filter_level[filter_id_u];
    *level_u = level_u_this;
    if (level_u_this == 0) {
      *level_u = bp_prev->deblock_filter_level[filter_id_u];
    }
  }
  if (need_filter_v) {
    const uint8_t level_v_this = bp->deblock_filter_level[filter_id_v];
    *level_v = level_v_this;
    if (level_v_this == 0) {
      *level_v = bp_prev->deblock_filter_level[filter_id_v];
    }
  }
  const int step_prev = kTransformWidth[bp_prev->uv_transform_size];
  *filter_length = std::min(*step, step_prev);
}

void PostFilter::HorizontalDeblockFilter(int row4x4_start, int row4x4_end,
                                         int column4x4_start,
                                         int column4x4_end) {
  const int height4x4 = row4x4_end - row4x4_start;
  const int width4x4 = column4x4_end - column4x4_start;
  if (height4x4 <= 0 || width4x4 <= 0) return;

  const int column_step = 1;
  const int src_step = 4 << pixel_size_log2_;
  const ptrdiff_t src_stride = frame_buffer_.stride(kPlaneY);
  uint8_t* src = GetSourceBuffer(kPlaneY, row4x4_start, column4x4_start);
  int row_step;
  uint8_t level;
  int filter_length;

  const int width = frame_header_.width;
  const int height = frame_header_.height;
  for (int column4x4 = 0;
       column4x4 < width4x4 && MultiplyBy4(column4x4_start + column4x4) < width;
       column4x4 += column_step, src += src_step) {
    uint8_t* src_row = src;
    for (int row4x4 = 0;
         row4x4 < height4x4 && MultiplyBy4(row4x4_start + row4x4) < height;
         row4x4 += row_step) {
      const bool need_filter = GetHorizontalDeblockFilterEdgeInfo(
          row4x4_start + row4x4, column4x4_start + column4x4, &level, &row_step,
          &filter_length);
      if (need_filter) {
        assert(level > 0 && level <= kMaxLoopFilterValue);
        const dsp::LoopFilterSize size = GetLoopFilterSizeY(filter_length);
        dsp_.loop_filters[size][kLoopFilterTypeHorizontal](
            src_row, src_stride, outer_thresh_[level], inner_thresh_[level],
            HevThresh(level));
      }
      src_row += row_step * src_stride;
      row_step = DivideBy4(row_step);
    }
  }

  if (needs_chroma_deblock_) {
    const int8_t subsampling_x = subsampling_x_[kPlaneU];
    const int8_t subsampling_y = subsampling_y_[kPlaneU];
    const int column_step = 1 << subsampling_x;
    const ptrdiff_t src_stride_u = frame_buffer_.stride(kPlaneU);
    const ptrdiff_t src_stride_v = frame_buffer_.stride(kPlaneV);
    uint8_t* src_u = GetSourceBuffer(kPlaneU, row4x4_start, column4x4_start);
    uint8_t* src_v = GetSourceBuffer(kPlaneV, row4x4_start, column4x4_start);
    int row_step;
    uint8_t level_u;
    uint8_t level_v;
    int filter_length;

    for (int column4x4 = 0; column4x4 < width4x4 &&
                            MultiplyBy4(column4x4_start + column4x4) < width;
         column4x4 += column_step, src_u += src_step, src_v += src_step) {
      uint8_t* src_row_u = src_u;
      uint8_t* src_row_v = src_v;
      for (int row4x4 = 0;
           row4x4 < height4x4 && MultiplyBy4(row4x4_start + row4x4) < height;
           row4x4 += row_step) {
        GetHorizontalDeblockFilterEdgeInfoUV(
            row4x4_start + row4x4, column4x4_start + column4x4, &level_u,
            &level_v, &row_step, &filter_length);
        if (level_u != 0) {
          const dsp::LoopFilterSize size = GetLoopFilterSizeUV(filter_length);
          dsp_.loop_filters[size][kLoopFilterTypeHorizontal](
              src_row_u, src_stride_u, outer_thresh_[level_u],
              inner_thresh_[level_u], HevThresh(level_u));
        }
        if (level_v != 0) {
          const dsp::LoopFilterSize size = GetLoopFilterSizeUV(filter_length);
          dsp_.loop_filters[size][kLoopFilterTypeHorizontal](
              src_row_v, src_stride_v, outer_thresh_[level_v],
              inner_thresh_[level_v], HevThresh(level_v));
        }
        src_row_u += row_step * src_stride_u;
        src_row_v += row_step * src_stride_v;
        row_step = DivideBy4(row_step << subsampling_y);
      }
    }
  }
}

void PostFilter::VerticalDeblockFilter(int row4x4_start, int row4x4_end,
                                       int column4x4_start, int column4x4_end) {
  const int height4x4 = row4x4_end - row4x4_start;
  const int width4x4 = column4x4_end - column4x4_start;
  if (height4x4 <= 0 || width4x4 <= 0) return;

  const ptrdiff_t row_stride = MultiplyBy4(frame_buffer_.stride(kPlaneY));
  const ptrdiff_t src_stride = frame_buffer_.stride(kPlaneY);
  uint8_t* src = GetSourceBuffer(kPlaneY, row4x4_start, column4x4_start);
  int column_step;
  uint8_t level;
  int filter_length;

  BlockParameters* const* bp_row_base =
      block_parameters_.Address(row4x4_start, column4x4_start);
  const int bp_stride = block_parameters_.columns4x4();
  const int column_step_shift = pixel_size_log2_;
  const int width = frame_header_.width;
  const int height = frame_header_.height;
  for (int row4x4 = 0;
       row4x4 < height4x4 && MultiplyBy4(row4x4_start + row4x4) < height;
       ++row4x4, src += row_stride, bp_row_base += bp_stride) {
    uint8_t* src_row = src;
    BlockParameters* const* bp = bp_row_base;
    for (int column4x4 = 0; column4x4 < width4x4 &&
                            MultiplyBy4(column4x4_start + column4x4) < width;
         column4x4 += column_step, bp += column_step) {
      const bool need_filter = GetVerticalDeblockFilterEdgeInfo(
          row4x4_start + row4x4, column4x4_start + column4x4, bp, &level,
          &column_step, &filter_length);
      if (need_filter) {
        assert(level > 0 && level <= kMaxLoopFilterValue);
        const dsp::LoopFilterSize size = GetLoopFilterSizeY(filter_length);
        dsp_.loop_filters[size][kLoopFilterTypeVertical](
            src_row, src_stride, outer_thresh_[level], inner_thresh_[level],
            HevThresh(level));
      }
      src_row += column_step << column_step_shift;
      column_step = DivideBy4(column_step);
    }
  }

  if (needs_chroma_deblock_) {
    const int8_t subsampling_x = subsampling_x_[kPlaneU];
    const int8_t subsampling_y = subsampling_y_[kPlaneU];
    const int row_step = 1 << subsampling_y;
    uint8_t* src_u = GetSourceBuffer(kPlaneU, row4x4_start, column4x4_start);
    uint8_t* src_v = GetSourceBuffer(kPlaneV, row4x4_start, column4x4_start);
    const ptrdiff_t src_stride_u = frame_buffer_.stride(kPlaneU);
    const ptrdiff_t src_stride_v = frame_buffer_.stride(kPlaneV);
    const ptrdiff_t row_stride_u = MultiplyBy4(frame_buffer_.stride(kPlaneU));
    const ptrdiff_t row_stride_v = MultiplyBy4(frame_buffer_.stride(kPlaneV));
    const LoopFilterType type = kLoopFilterTypeVertical;
    int column_step;
    uint8_t level_u;
    uint8_t level_v;
    int filter_length;

    BlockParameters* const* bp_row_base = block_parameters_.Address(
        GetDeblockPosition(row4x4_start, subsampling_y),
        GetDeblockPosition(column4x4_start, subsampling_x));
    const int bp_stride = block_parameters_.columns4x4() << subsampling_y;
    for (int row4x4 = 0;
         row4x4 < height4x4 && MultiplyBy4(row4x4_start + row4x4) < height;
         row4x4 += row_step, src_u += row_stride_u, src_v += row_stride_v,
             bp_row_base += bp_stride) {
      uint8_t* src_row_u = src_u;
      uint8_t* src_row_v = src_v;
      BlockParameters* const* bp = bp_row_base;
      for (int column4x4 = 0; column4x4 < width4x4 &&
                              MultiplyBy4(column4x4_start + column4x4) < width;
           column4x4 += column_step, bp += column_step) {
        GetVerticalDeblockFilterEdgeInfoUV(column4x4_start + column4x4, bp,
                                           &level_u, &level_v, &column_step,
                                           &filter_length);
        if (level_u != 0) {
          const dsp::LoopFilterSize size = GetLoopFilterSizeUV(filter_length);
          dsp_.loop_filters[size][type](
              src_row_u, src_stride_u, outer_thresh_[level_u],
              inner_thresh_[level_u], HevThresh(level_u));
        }
        if (level_v != 0) {
          const dsp::LoopFilterSize size = GetLoopFilterSizeUV(filter_length);
          dsp_.loop_filters[size][type](
              src_row_v, src_stride_v, outer_thresh_[level_v],
              inner_thresh_[level_v], HevThresh(level_v));
        }
        src_row_u += column_step << column_step_shift;
        src_row_v += column_step << column_step_shift;
        column_step = DivideBy4(column_step << subsampling_x);
      }
    }
  }
}

template <LoopFilterType loop_filter_type>
void PostFilter::DeblockFilterWorker(std::atomic<int>* row4x4_atomic) {
  const int rows4x4 = frame_header_.rows4x4;
  const int columns4x4 = frame_header_.columns4x4;
  int row4x4;
  while ((row4x4 = row4x4_atomic->fetch_add(
              kNum4x4InLoopFilterUnit, std::memory_order_relaxed)) < rows4x4) {
    (this->*deblock_filter_func_[loop_filter_type])(
        row4x4, row4x4 + kNum4x4InLoopFilterUnit, 0, columns4x4);
  }
}

template void PostFilter::DeblockFilterWorker<kLoopFilterTypeVertical>(
    std::atomic<int>* row4x4_atomic);
template void PostFilter::DeblockFilterWorker<kLoopFilterTypeHorizontal>(
    std::atomic<int>* row4x4_atomic);

void PostFilter::ApplyDeblockFilter(LoopFilterType loop_filter_type,
                                    int row4x4_start, int column4x4_start,
                                    int column4x4_end, int sb4x4) {
  assert(row4x4_start >= 0);
  assert(DoDeblock());
  column4x4_end =
      std::min(Align(column4x4_end, static_cast<int>(kNum4x4InLoopFilterUnit)),
               frame_header_.columns4x4);
  if (column4x4_start >= column4x4_end) return;
  (this->*deblock_filter_func_[loop_filter_type])(
      row4x4_start, row4x4_start + sb4x4, column4x4_start, column4x4_end);
}

}  // namespace libgav1
