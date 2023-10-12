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
#include "src/post_filter.h"
#include "src/utils/blocking_counter.h"

namespace libgav1 {

template <typename Pixel>
void PostFilter::ApplyLoopRestorationForOneRow(
    const Pixel* src_buffer, const ptrdiff_t stride, const Plane plane,
    const int plane_height, const int plane_width, const int unit_y,
    const int unit_row, const int current_process_unit_height,
    const int plane_unit_size, Pixel* dst_buffer) {
  const int num_horizontal_units =
      restoration_info_->num_horizontal_units(static_cast<Plane>(plane));
  const RestorationUnitInfo* const restoration_info =
      restoration_info_->loop_restoration_info(static_cast<Plane>(plane),
                                               unit_row * num_horizontal_units);
  const bool in_place = DoCdef() || thread_pool_ != nullptr;
  const Pixel* border = nullptr;
  ptrdiff_t border_stride = 0;
  src_buffer += unit_y * stride;
  if (in_place) {
    const int border_unit_y = std::max(
        RightShiftWithCeiling(unit_y, 4 - subsampling_y_[plane]) - 4, 0);
    border_stride = loop_restoration_border_.stride(plane) / sizeof(Pixel);
    border =
        reinterpret_cast<const Pixel*>(loop_restoration_border_.data(plane)) +
        border_unit_y * border_stride;
  }
  int unit_column = 0;
  int column = 0;
  do {
    const int current_process_unit_width =
        std::min(plane_unit_size, plane_width - column);
    const Pixel* src = src_buffer + column;
    unit_column = std::min(unit_column, num_horizontal_units - 1);
    if (restoration_info[unit_column].type == kLoopRestorationTypeNone) {
      Pixel* dst = dst_buffer + column;
      if (in_place) {
        int k = current_process_unit_height;
        do {
          memmove(dst, src, current_process_unit_width * sizeof(Pixel));
          src += stride;
          dst += stride;
        } while (--k != 0);
      } else {
        CopyPlane(src, stride, current_process_unit_width,
                  current_process_unit_height, dst, stride);
      }
    } else {
      const Pixel* top_border = src - kRestorationVerticalBorder * stride;
      ptrdiff_t top_border_stride = stride;
      const Pixel* bottom_border = src + current_process_unit_height * stride;
      ptrdiff_t bottom_border_stride = stride;
      const bool frame_bottom_border =
          (unit_y + current_process_unit_height >= plane_height);
      if (in_place && (unit_y != 0 || !frame_bottom_border)) {
        const Pixel* loop_restoration_border = border + column;
        if (unit_y != 0) {
          top_border = loop_restoration_border;
          top_border_stride = border_stride;
          loop_restoration_border += 4 * border_stride;
        }
        if (!frame_bottom_border) {
          bottom_border = loop_restoration_border +
                          kRestorationVerticalBorder * border_stride;
          bottom_border_stride = border_stride;
        }
      }
#if LIBGAV1_MSAN
      // The optimized loop filter may read past initialized values within the
      // buffer.
      RestorationBuffer restoration_buffer = {};
#else
      RestorationBuffer restoration_buffer;
#endif
      const LoopRestorationType type = restoration_info[unit_column].type;
      assert(type == kLoopRestorationTypeSgrProj ||
             type == kLoopRestorationTypeWiener);
      const dsp::LoopRestorationFunc restoration_func =
          dsp_.loop_restorations[type - 2];
      restoration_func(restoration_info[unit_column], src, stride, top_border,
                       top_border_stride, bottom_border, bottom_border_stride,
                       current_process_unit_width, current_process_unit_height,
                       &restoration_buffer, dst_buffer + column);
    }
    ++unit_column;
    column += plane_unit_size;
  } while (column < plane_width);
}

template <typename Pixel>
void PostFilter::ApplyLoopRestorationForOneSuperBlockRow(const int row4x4_start,
                                                         const int sb4x4) {
  assert(row4x4_start >= 0);
  assert(DoRestoration());
  int plane = kPlaneY;
  const int upscaled_width = frame_header_.upscaled_width;
  const int height = frame_header_.height;
  do {
    if (loop_restoration_.type[plane] == kLoopRestorationTypeNone) {
      continue;
    }
    const ptrdiff_t stride = frame_buffer_.stride(plane) / sizeof(Pixel);
    const int unit_height_offset =
        kRestorationUnitOffset >> subsampling_y_[plane];
    const int plane_height = SubsampledValue(height, subsampling_y_[plane]);
    const int plane_width =
        SubsampledValue(upscaled_width, subsampling_x_[plane]);
    const int plane_unit_size = 1 << loop_restoration_.unit_size_log2[plane];
    const int plane_process_unit_height =
        kRestorationUnitHeight >> subsampling_y_[plane];
    int y = (row4x4_start == 0)
                ? 0
                : (MultiplyBy4(row4x4_start) >> subsampling_y_[plane]) -
                      unit_height_offset;
    int expected_height = plane_process_unit_height -
                          ((row4x4_start == 0) ? unit_height_offset : 0);
    int current_process_unit_height;
    for (int sb_y = 0; sb_y < sb4x4;
         sb_y += 16, y += current_process_unit_height) {
      if (y >= plane_height) break;
      const int unit_row = std::min(
          (y + unit_height_offset) >> loop_restoration_.unit_size_log2[plane],
          restoration_info_->num_vertical_units(static_cast<Plane>(plane)) - 1);
      current_process_unit_height = std::min(expected_height, plane_height - y);
      expected_height = plane_process_unit_height;
      ApplyLoopRestorationForOneRow<Pixel>(
          reinterpret_cast<Pixel*>(superres_buffer_[plane]), stride,
          static_cast<Plane>(plane), plane_height, plane_width, y, unit_row,
          current_process_unit_height, plane_unit_size,
          reinterpret_cast<Pixel*>(loop_restoration_buffer_[plane]) +
              y * stride);
    }
  } while (++plane < planes_);
}

void PostFilter::ApplyLoopRestoration(const int row4x4_start, const int sb4x4) {
#if LIBGAV1_MAX_BITDEPTH >= 10
  if (bitdepth_ >= 10) {
    ApplyLoopRestorationForOneSuperBlockRow<uint16_t>(row4x4_start, sb4x4);
    return;
  }
#endif
  ApplyLoopRestorationForOneSuperBlockRow<uint8_t>(row4x4_start, sb4x4);
}

void PostFilter::ApplyLoopRestorationWorker(std::atomic<int>* row4x4_atomic) {
  int row4x4;
  // Loop Restoration operates with a lag of 8 rows (4 for chroma with
  // subsampling) and hence we need to make sure to cover the last 8 rows of the
  // last superblock row. So we run this loop for an extra iteration to
  // accomplish that.
  const int row4x4_end = frame_header_.rows4x4 + kNum4x4InLoopRestorationUnit;
  while ((row4x4 = row4x4_atomic->fetch_add(kNum4x4InLoopRestorationUnit,
                                            std::memory_order_relaxed)) <
         row4x4_end) {
    CopyBordersForOneSuperBlockRow(row4x4, kNum4x4InLoopRestorationUnit,
                                   /*for_loop_restoration=*/true);
#if LIBGAV1_MAX_BITDEPTH >= 10
    if (bitdepth_ >= 10) {
      ApplyLoopRestorationForOneSuperBlockRow<uint16_t>(
          row4x4, kNum4x4InLoopRestorationUnit);
      continue;
    }
#endif
    ApplyLoopRestorationForOneSuperBlockRow<uint8_t>(
        row4x4, kNum4x4InLoopRestorationUnit);
  }
}

}  // namespace libgav1
