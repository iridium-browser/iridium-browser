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

#include "src/post_filter.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/utils/array_2d.h"
#include "src/utils/blocking_counter.h"
#include "src/utils/common.h"
#include "src/utils/compiler_attributes.h"
#include "src/utils/constants.h"
#include "src/utils/memory.h"
#include "src/utils/types.h"

namespace libgav1 {
namespace {

// Import all the constants in the anonymous namespace.
#include "src/post_filter/deblock_thresholds.inc"

// Row indices of loop restoration border. This is used to populate the
// |loop_restoration_border_| when either cdef is on or multithreading is
// enabled. The dimension is subsampling_y.
constexpr int kLoopRestorationBorderRows[2] = {54, 26};

}  // namespace

PostFilter::PostFilter(const ObuFrameHeader& frame_header,
                       const ObuSequenceHeader& sequence_header,
                       FrameScratchBuffer* const frame_scratch_buffer,
                       YuvBuffer* const frame_buffer, const dsp::Dsp* dsp,
                       int do_post_filter_mask)
    : frame_header_(frame_header),
      loop_restoration_(frame_header.loop_restoration),
      dsp_(*dsp),
      bitdepth_(sequence_header.color_config.bitdepth),
      subsampling_x_{0, sequence_header.color_config.subsampling_x,
                     sequence_header.color_config.subsampling_x},
      subsampling_y_{0, sequence_header.color_config.subsampling_y,
                     sequence_header.color_config.subsampling_y},
      planes_(sequence_header.color_config.is_monochrome ? kMaxPlanesMonochrome
                                                         : kMaxPlanes),
      pixel_size_log2_(static_cast<int>((bitdepth_ == 8) ? sizeof(uint8_t)
                                                         : sizeof(uint16_t)) -
                       1),
      inner_thresh_(kInnerThresh[frame_header.loop_filter.sharpness]),
      outer_thresh_(kOuterThresh[frame_header.loop_filter.sharpness]),
      needs_chroma_deblock_(frame_header.loop_filter.level[kPlaneU + 1] != 0 ||
                            frame_header.loop_filter.level[kPlaneV + 1] != 0),
      do_cdef_(DoCdef(frame_header, do_post_filter_mask)),
      do_deblock_(DoDeblock(frame_header, do_post_filter_mask)),
      do_restoration_(
          DoRestoration(loop_restoration_, do_post_filter_mask, planes_)),
      do_superres_(DoSuperRes(frame_header, do_post_filter_mask)),
      cdef_index_(frame_scratch_buffer->cdef_index),
      cdef_skip_(frame_scratch_buffer->cdef_skip),
      inter_transform_sizes_(frame_scratch_buffer->inter_transform_sizes),
      restoration_info_(&frame_scratch_buffer->loop_restoration_info),
      superres_coefficients_{
          frame_scratch_buffer->superres_coefficients[kPlaneTypeY].get(),
          frame_scratch_buffer
              ->superres_coefficients
                  [(sequence_header.color_config.is_monochrome ||
                    sequence_header.color_config.subsampling_x == 0)
                       ? kPlaneTypeY
                       : kPlaneTypeUV]
              .get()},
      superres_line_buffer_(frame_scratch_buffer->superres_line_buffer),
      block_parameters_(frame_scratch_buffer->block_parameters_holder),
      frame_buffer_(*frame_buffer),
      cdef_border_(frame_scratch_buffer->cdef_border),
      loop_restoration_border_(frame_scratch_buffer->loop_restoration_border),
      thread_pool_(
          frame_scratch_buffer->threading_strategy.post_filter_thread_pool()) {
  const int8_t zero_delta_lf[kFrameLfCount] = {};
  ComputeDeblockFilterLevels(zero_delta_lf, deblock_filter_levels_);
  if (DoSuperRes()) {
    int plane = kPlaneY;
    const int width = frame_header_.width;
    const int upscaled_width_fh = frame_header_.upscaled_width;
    do {
      const int downscaled_width =
          SubsampledValue(width, subsampling_x_[plane]);
      const int upscaled_width =
          SubsampledValue(upscaled_width_fh, subsampling_x_[plane]);
      const int superres_width = downscaled_width << kSuperResScaleBits;
      super_res_info_[plane].step =
          (superres_width + upscaled_width / 2) / upscaled_width;
      const int error =
          super_res_info_[plane].step * upscaled_width - superres_width;
      super_res_info_[plane].initial_subpixel_x =
          ((-((upscaled_width - downscaled_width) << (kSuperResScaleBits - 1)) +
            DivideBy2(upscaled_width)) /
               upscaled_width +
           (1 << (kSuperResExtraBits - 1)) - error / 2) &
          kSuperResScaleMask;
      super_res_info_[plane].upscaled_width = upscaled_width;
    } while (++plane < planes_);
    if (dsp->super_res_coefficients != nullptr) {
      int plane = kPlaneY;
      const int number_loops = (superres_coefficients_[kPlaneTypeY] ==
                                superres_coefficients_[kPlaneTypeUV])
                                   ? kMaxPlanesMonochrome
                                   : static_cast<int>(kNumPlaneTypes);
      do {
        dsp->super_res_coefficients(super_res_info_[plane].upscaled_width,
                                    super_res_info_[plane].initial_subpixel_x,
                                    super_res_info_[plane].step,
                                    superres_coefficients_[plane]);
      } while (++plane < number_loops);
    }
  }
  int plane = kPlaneY;
  do {
    loop_restoration_buffer_[plane] = frame_buffer_.data(plane);
    cdef_buffer_[plane] = frame_buffer_.data(plane);
    superres_buffer_[plane] = frame_buffer_.data(plane);
    source_buffer_[plane] = frame_buffer_.data(plane);
  } while (++plane < planes_);
  if (DoCdef() || DoRestoration() || DoSuperRes()) {
    plane = kPlaneY;
    const int pixel_size_log2 = pixel_size_log2_;
    do {
      int horizontal_shift = 0;
      int vertical_shift = 0;
      if (DoRestoration() &&
          loop_restoration_.type[plane] != kLoopRestorationTypeNone) {
        horizontal_shift += frame_buffer_.alignment();
        if (!DoCdef() && thread_pool_ == nullptr) {
          vertical_shift += kRestorationVerticalBorder;
        }
        superres_buffer_[plane] +=
            vertical_shift * frame_buffer_.stride(plane) +
            (horizontal_shift << pixel_size_log2);
      }
      if (DoSuperRes()) {
        vertical_shift += kSuperResVerticalBorder;
      }
      cdef_buffer_[plane] += vertical_shift * frame_buffer_.stride(plane) +
                             (horizontal_shift << pixel_size_log2);
      if (DoCdef() && thread_pool_ == nullptr) {
        horizontal_shift += frame_buffer_.alignment();
        vertical_shift += kCdefBorder;
      }
      assert(horizontal_shift <= frame_buffer_.right_border(plane));
      assert(vertical_shift <= frame_buffer_.bottom_border(plane));
      source_buffer_[plane] += vertical_shift * frame_buffer_.stride(plane) +
                               (horizontal_shift << pixel_size_log2);
    } while (++plane < planes_);
  }
}

// The following example illustrates how ExtendFrame() extends a frame.
// Suppose the frame width is 8 and height is 4, and left, right, top, and
// bottom are all equal to 3.
//
// Before:
//
//       ABCDEFGH
//       IJKLMNOP
//       QRSTUVWX
//       YZabcdef
//
// After:
//
//   AAA|ABCDEFGH|HHH  [3]
//   AAA|ABCDEFGH|HHH
//   AAA|ABCDEFGH|HHH
//   ---+--------+---
//   AAA|ABCDEFGH|HHH  [1]
//   III|IJKLMNOP|PPP
//   QQQ|QRSTUVWX|XXX
//   YYY|YZabcdef|fff
//   ---+--------+---
//   YYY|YZabcdef|fff  [2]
//   YYY|YZabcdef|fff
//   YYY|YZabcdef|fff
//
// ExtendFrame() first extends the rows to the left and to the right[1]. Then
// it copies the extended last row to the bottom borders[2]. Finally it copies
// the extended first row to the top borders[3].
// static
template <typename Pixel>
void PostFilter::ExtendFrame(Pixel* const frame_start, const int width,
                             const int height, const ptrdiff_t stride,
                             const int left, const int right, const int top,
                             const int bottom) {
  Pixel* src = frame_start;
  // Copy to left and right borders.
  int y = height;
  do {
    ExtendLine<Pixel>(src, width, left, right);
    src += stride;
  } while (--y != 0);
  // Copy to bottom borders. For performance we copy |stride| pixels
  // (including some padding pixels potentially) in each row, ending at the
  // bottom right border pixel. In the diagram the asterisks indicate padding
  // pixels.
  //
  // |<--- stride --->|
  // **YYY|YZabcdef|fff <-- Copy from the extended last row.
  // -----+--------+---
  // **YYY|YZabcdef|fff
  // **YYY|YZabcdef|fff
  // **YYY|YZabcdef|fff <-- bottom right border pixel
  assert(src == frame_start + height * stride);
  Pixel* dst = src - left;
  src = dst - stride;
  for (int y = 0; y < bottom; ++y) {
    memcpy(dst, src, sizeof(Pixel) * stride);
    dst += stride;
  }
  // Copy to top borders. For performance we copy |stride| pixels (including
  // some padding pixels potentially) in each row, starting from the top left
  // border pixel. In the diagram the asterisks indicate padding pixels.
  //
  // +-- top left border pixel
  // |
  // v
  // AAA|ABCDEFGH|HHH**
  // AAA|ABCDEFGH|HHH**
  // AAA|ABCDEFGH|HHH**
  // ---+--------+-----
  // AAA|ABCDEFGH|HHH** <-- Copy from the extended first row.
  // |<--- stride --->|
  src = frame_start - left;
  dst = frame_start - left - top * stride;
  for (int y = 0; y < top; ++y) {
    memcpy(dst, src, sizeof(Pixel) * stride);
    dst += stride;
  }
}

template void PostFilter::ExtendFrame<uint8_t>(uint8_t* const frame_start,
                                               const int width,
                                               const int height,
                                               const ptrdiff_t stride,
                                               const int left, const int right,
                                               const int top, const int bottom);

#if LIBGAV1_MAX_BITDEPTH >= 10
template void PostFilter::ExtendFrame<uint16_t>(
    uint16_t* const frame_start, const int width, const int height,
    const ptrdiff_t stride, const int left, const int right, const int top,
    const int bottom);
#endif

void PostFilter::ExtendFrameBoundary(uint8_t* const frame_start,
                                     const int width, const int height,
                                     const ptrdiff_t stride, const int left,
                                     const int right, const int top,
                                     const int bottom) const {
#if LIBGAV1_MAX_BITDEPTH >= 10
  if (bitdepth_ >= 10) {
    ExtendFrame<uint16_t>(reinterpret_cast<uint16_t*>(frame_start), width,
                          height, stride >> 1, left, right, top, bottom);
    return;
  }
#endif
  ExtendFrame<uint8_t>(frame_start, width, height, stride, left, right, top,
                       bottom);
}

void PostFilter::ExtendBordersForReferenceFrame() {
  if (frame_header_.refresh_frame_flags == 0) return;
  const int upscaled_width = frame_header_.upscaled_width;
  const int height = frame_header_.height;
  int plane = kPlaneY;
  do {
    const int plane_width =
        SubsampledValue(upscaled_width, subsampling_x_[plane]);
    const int plane_height = SubsampledValue(height, subsampling_y_[plane]);
    assert(frame_buffer_.left_border(plane) >= kMinLeftBorderPixels &&
           frame_buffer_.right_border(plane) >= kMinRightBorderPixels &&
           frame_buffer_.top_border(plane) >= kMinTopBorderPixels &&
           frame_buffer_.bottom_border(plane) >= kMinBottomBorderPixels);
    // plane subsampling_x_ left_border
    //   Y        N/A         64, 48
    //  U,V        0          64, 48
    //  U,V        1          32, 16
    assert(frame_buffer_.left_border(plane) >= 16);
    // The |left| argument to ExtendFrameBoundary() must be at least
    // kMinLeftBorderPixels (13) for warp.
    static_assert(16 >= kMinLeftBorderPixels, "");
    ExtendFrameBoundary(
        frame_buffer_.data(plane), plane_width, plane_height,
        frame_buffer_.stride(plane), frame_buffer_.left_border(plane),
        frame_buffer_.right_border(plane), frame_buffer_.top_border(plane),
        frame_buffer_.bottom_border(plane));
  } while (++plane < planes_);
}

void PostFilter::CopyDeblockedPixels(Plane plane, int row4x4) {
  const ptrdiff_t src_stride = frame_buffer_.stride(plane);
  const uint8_t* const src = GetSourceBuffer(plane, row4x4, 0);
  const int row_offset = DivideBy4(row4x4);
  const ptrdiff_t dst_stride = loop_restoration_border_.stride(plane);
  uint8_t* dst = loop_restoration_border_.data(plane) + row_offset * dst_stride;
  const int num_pixels = SubsampledValue(MultiplyBy4(frame_header_.columns4x4),
                                         subsampling_x_[plane]);
  const int row_width = num_pixels << pixel_size_log2_;
  int last_valid_row = -1;
  const int plane_height =
      SubsampledValue(frame_header_.height, subsampling_y_[plane]);
  int row = kLoopRestorationBorderRows[subsampling_y_[plane]];
  const int absolute_row = (MultiplyBy4(row4x4) >> subsampling_y_[plane]) + row;
  for (int i = 0; i < 4; ++i, ++row) {
    if (absolute_row + i >= plane_height) {
      if (last_valid_row == -1) break;
      // If we run out of rows, copy the last valid row (mimics the bottom
      // border extension).
      row = last_valid_row;
    }
    memcpy(dst, src + row * src_stride, row_width);
    last_valid_row = row;
    dst += dst_stride;
  }
}

void PostFilter::CopyBordersForOneSuperBlockRow(int row4x4, int sb4x4,
                                                bool for_loop_restoration) {
  // Number of rows to be subtracted from the start position described by
  // row4x4. We always lag by 8 rows (to account for in-loop post filters).
  const int row_offset = (row4x4 == 0) ? 0 : 8;
  // Number of rows to be subtracted from the height described by sb4x4.
  const int height_offset = (row4x4 == 0) ? 8 : 0;
  // If cdef is off and post filter multithreading is off, then loop restoration
  // needs 2 extra rows for the bottom border in each plane.
  const int extra_rows =
      (for_loop_restoration && thread_pool_ == nullptr && !DoCdef()) ? 2 : 0;
  const int upscaled_width = frame_header_.upscaled_width;
  const int height = frame_header_.height;
  int plane = kPlaneY;
  do {
    const int plane_width =
        SubsampledValue(upscaled_width, subsampling_x_[plane]);
    const int plane_height = SubsampledValue(height, subsampling_y_[plane]);
    const int row = (MultiplyBy4(row4x4) - row_offset) >> subsampling_y_[plane];
    assert(row >= 0);
    if (row >= plane_height) break;
    const int num_rows =
        std::min(SubsampledValue(MultiplyBy4(sb4x4) - height_offset,
                                 subsampling_y_[plane]) +
                     extra_rows,
                 plane_height - row);
    // We only need to track the progress of the Y plane since the progress of
    // the U and V planes will be inferred from the progress of the Y plane.
    if (!for_loop_restoration && plane == kPlaneY) {
      progress_row_ = row + num_rows;
    }
    const bool copy_bottom = row + num_rows == plane_height;
    const ptrdiff_t stride = frame_buffer_.stride(plane);
    uint8_t* const start = (for_loop_restoration ? superres_buffer_[plane]
                                                 : frame_buffer_.data(plane)) +
                           row * stride;
#if LIBGAV1_MSAN
    const int right_padding =
        (frame_buffer_.stride(plane) >> static_cast<int>(bitdepth_ > 8)) -
        ((frame_buffer_.left_border(plane) + frame_buffer_.width(plane) +
          frame_buffer_.right_border(plane)));
    const int padded_right_border_size =
        frame_buffer_.right_border(plane) + right_padding;
    // The optimized loop restoration code may read into the next row's left
    // border depending on the start of the last superblock and the size of the
    // right border. This is safe as the post filter is applied after
    // reconstruction is complete and the threaded implementations do not read
    // from the left border.
    const int left_border_overread =
        (for_loop_restoration && padded_right_border_size < 64)
            ? 63 - padded_right_border_size
            : 0;
    assert(!for_loop_restoration || left_border_overread == 0 ||
           (frame_buffer_.bottom_border(plane) > 0 &&
            left_border_overread <= frame_buffer_.left_border(plane)));
    const int left_border = (for_loop_restoration && left_border_overread == 0)
                                ? kRestorationHorizontalBorder
                                : frame_buffer_.left_border(plane);
    // The optimized loop restoration code will overread the visible frame
    // buffer into the right border. Extend the right boundary further to
    // prevent msan warnings.
    const int right_border = for_loop_restoration
                                 ? std::min(padded_right_border_size, 63)
                                 : frame_buffer_.right_border(plane);
#else
    const int left_border = for_loop_restoration
                                ? kRestorationHorizontalBorder
                                : frame_buffer_.left_border(plane);
    const int right_border = for_loop_restoration
                                 ? kRestorationHorizontalBorder
                                 : frame_buffer_.right_border(plane);
#endif
    const int top_border =
        (row == 0) ? (for_loop_restoration ? kRestorationVerticalBorder
                                           : frame_buffer_.top_border(plane))
                   : 0;
    const int bottom_border =
        copy_bottom
            ? (for_loop_restoration ? kRestorationVerticalBorder
                                    : frame_buffer_.bottom_border(plane))
            : 0;
    ExtendFrameBoundary(start, plane_width, num_rows, stride, left_border,
                        right_border, top_border, bottom_border);
  } while (++plane < planes_);
}

void PostFilter::SetupLoopRestorationBorder(const int row4x4) {
  assert(row4x4 >= 0);
  assert(!DoCdef());
  assert(DoRestoration());
  const int upscaled_width = frame_header_.upscaled_width;
  const int height = frame_header_.height;
  int plane = kPlaneY;
  do {
    if (loop_restoration_.type[plane] == kLoopRestorationTypeNone) {
      continue;
    }
    const int row_offset = DivideBy4(row4x4);
    const int num_pixels =
        SubsampledValue(upscaled_width, subsampling_x_[plane]);
    const int row_width = num_pixels << pixel_size_log2_;
    const int plane_height = SubsampledValue(height, subsampling_y_[plane]);
    const int row = kLoopRestorationBorderRows[subsampling_y_[plane]];
    const int absolute_row =
        (MultiplyBy4(row4x4) >> subsampling_y_[plane]) + row;
    const ptrdiff_t src_stride = frame_buffer_.stride(plane);
    const uint8_t* src =
        GetSuperResBuffer(static_cast<Plane>(plane), row4x4, 0) +
        row * src_stride;
    const ptrdiff_t dst_stride = loop_restoration_border_.stride(plane);
    uint8_t* dst =
        loop_restoration_border_.data(plane) + row_offset * dst_stride;
    for (int i = 0; i < 4; ++i) {
      memcpy(dst, src, row_width);
#if LIBGAV1_MAX_BITDEPTH >= 10
      if (bitdepth_ >= 10) {
        ExtendLine<uint16_t>(dst, num_pixels, kRestorationHorizontalBorder,
                             kRestorationHorizontalBorder);
      } else  // NOLINT.
#endif
        ExtendLine<uint8_t>(dst, num_pixels, kRestorationHorizontalBorder,
                            kRestorationHorizontalBorder);
      // If we run out of rows, copy the last valid row (mimics the bottom
      // border extension).
      if (absolute_row + i < plane_height - 1) src += src_stride;
      dst += dst_stride;
    }
  } while (++plane < planes_);
}

void PostFilter::SetupLoopRestorationBorder(int row4x4_start, int sb4x4) {
  assert(row4x4_start >= 0);
  assert(DoCdef());
  assert(DoRestoration());
  for (int sb_y = 0; sb_y < sb4x4; sb_y += 16) {
    const int row4x4 = row4x4_start + sb_y;
    const int row_offset_start = DivideBy4(row4x4);
    const std::array<uint8_t*, kMaxPlanes> dst = {
        loop_restoration_border_.data(kPlaneY) +
            row_offset_start * static_cast<ptrdiff_t>(
                                   loop_restoration_border_.stride(kPlaneY)),
        loop_restoration_border_.data(kPlaneU) +
            row_offset_start * static_cast<ptrdiff_t>(
                                   loop_restoration_border_.stride(kPlaneU)),
        loop_restoration_border_.data(kPlaneV) +
            row_offset_start * static_cast<ptrdiff_t>(
                                   loop_restoration_border_.stride(kPlaneV))};
    // If SuperRes is enabled, then we apply SuperRes for the rows to be copied
    // directly with |loop_restoration_border_| as the destination. Otherwise,
    // we simply copy the rows.
    if (DoSuperRes()) {
      std::array<uint8_t*, kMaxPlanes> src;
      std::array<int, kMaxPlanes> rows;
      const int height = frame_header_.height;
      int plane = kPlaneY;
      do {
        if (loop_restoration_.type[plane] == kLoopRestorationTypeNone) {
          rows[plane] = 0;
          continue;
        }
        const int plane_height = SubsampledValue(height, subsampling_y_[plane]);
        const int row = kLoopRestorationBorderRows[subsampling_y_[plane]];
        const int absolute_row =
            (MultiplyBy4(row4x4) >> subsampling_y_[plane]) + row;
        src[plane] = GetSourceBuffer(static_cast<Plane>(plane), row4x4, 0) +
                     row * static_cast<ptrdiff_t>(frame_buffer_.stride(plane));
        rows[plane] = Clip3(plane_height - absolute_row, 0, 4);
      } while (++plane < planes_);
      ApplySuperRes(src, rows, /*line_buffer_row=*/-1, dst,
                    /*dst_is_loop_restoration_border=*/true);
      // If we run out of rows, copy the last valid row (mimics the bottom
      // border extension).
      plane = kPlaneY;
      do {
        if (rows[plane] == 0 || rows[plane] >= 4) continue;
        const ptrdiff_t stride = loop_restoration_border_.stride(plane);
        uint8_t* dst_line = dst[plane] + rows[plane] * stride;
        const uint8_t* const src_line = dst_line - stride;
        const int upscaled_width = super_res_info_[plane].upscaled_width
                                   << pixel_size_log2_;
        for (int i = rows[plane]; i < 4; ++i) {
          memcpy(dst_line, src_line, upscaled_width);
          dst_line += stride;
        }
      } while (++plane < planes_);
    } else {
      int plane = kPlaneY;
      do {
        CopyDeblockedPixels(static_cast<Plane>(plane), row4x4);
      } while (++plane < planes_);
    }
    // Extend the left and right boundaries needed for loop restoration.
    const int upscaled_width = frame_header_.upscaled_width;
    int plane = kPlaneY;
    do {
      if (loop_restoration_.type[plane] == kLoopRestorationTypeNone) {
        continue;
      }
      uint8_t* dst_line = dst[plane];
      const int plane_width =
          SubsampledValue(upscaled_width, subsampling_x_[plane]);
      for (int i = 0; i < 4; ++i) {
#if LIBGAV1_MAX_BITDEPTH >= 10
        if (bitdepth_ >= 10) {
          ExtendLine<uint16_t>(dst_line, plane_width,
                               kRestorationHorizontalBorder,
                               kRestorationHorizontalBorder);
        } else  // NOLINT.
#endif
        {
          ExtendLine<uint8_t>(dst_line, plane_width,
                              kRestorationHorizontalBorder,
                              kRestorationHorizontalBorder);
        }
        dst_line += loop_restoration_border_.stride(plane);
      }
    } while (++plane < planes_);
  }
}

void PostFilter::RunJobs(WorkerFunction worker) {
  std::atomic<int> row4x4(0);
  const int num_workers = thread_pool_->num_threads();
  BlockingCounter pending_workers(num_workers);
  for (int i = 0; i < num_workers; ++i) {
    thread_pool_->Schedule([this, &row4x4, &pending_workers, worker]() {
      (this->*worker)(&row4x4);
      pending_workers.Decrement();
    });
  }
  // Run the jobs on the current thread.
  (this->*worker)(&row4x4);
  // Wait for the threadpool jobs to finish.
  pending_workers.Wait();
}

void PostFilter::ApplyFilteringThreaded() {
  if (DoDeblock()) {
    RunJobs(&PostFilter::DeblockFilterWorker<kLoopFilterTypeVertical>);
    RunJobs(&PostFilter::DeblockFilterWorker<kLoopFilterTypeHorizontal>);
  }
  if (DoCdef() && DoRestoration()) {
    for (int row4x4 = 0; row4x4 < frame_header_.rows4x4;
         row4x4 += kNum4x4InLoopFilterUnit) {
      SetupLoopRestorationBorder(row4x4, kNum4x4InLoopFilterUnit);
    }
  }
  if (DoCdef()) {
    for (int row4x4 = 0; row4x4 < frame_header_.rows4x4;
         row4x4 += kNum4x4InLoopFilterUnit) {
      SetupCdefBorder(row4x4);
    }
    RunJobs(&PostFilter::ApplyCdefWorker);
  }
  if (DoSuperRes()) ApplySuperResThreaded();
  if (DoRestoration()) {
    if (!DoCdef()) {
      int row4x4 = 0;
      do {
        SetupLoopRestorationBorder(row4x4);
        row4x4 += kNum4x4InLoopFilterUnit;
      } while (row4x4 < frame_header_.rows4x4);
    }
    RunJobs(&PostFilter::ApplyLoopRestorationWorker);
  }
  ExtendBordersForReferenceFrame();
}

int PostFilter::ApplyFilteringForOneSuperBlockRow(int row4x4, int sb4x4,
                                                  bool is_last_row,
                                                  bool do_deblock) {
  if (row4x4 < 0) return -1;
  if (DoDeblock() && do_deblock) {
    VerticalDeblockFilter(row4x4, row4x4 + sb4x4, 0, frame_header_.columns4x4);
    HorizontalDeblockFilter(row4x4, row4x4 + sb4x4, 0,
                            frame_header_.columns4x4);
  }
  if (DoRestoration() && DoCdef()) {
    SetupLoopRestorationBorder(row4x4, sb4x4);
  }
  if (DoCdef()) {
    ApplyCdefForOneSuperBlockRow(row4x4, sb4x4, is_last_row);
  }
  if (DoSuperRes()) {
    ApplySuperResForOneSuperBlockRow(row4x4, sb4x4, is_last_row);
  }
  if (DoRestoration()) {
    CopyBordersForOneSuperBlockRow(row4x4, sb4x4, true);
    ApplyLoopRestoration(row4x4, sb4x4);
    if (is_last_row) {
      // Loop restoration operates with a lag of 8 rows. So make sure to cover
      // all the rows of the last superblock row.
      CopyBordersForOneSuperBlockRow(row4x4 + sb4x4, 16, true);
      ApplyLoopRestoration(row4x4 + sb4x4, 16);
    }
  }
  if (frame_header_.refresh_frame_flags != 0 && DoBorderExtensionInLoop()) {
    CopyBordersForOneSuperBlockRow(row4x4, sb4x4, false);
    if (is_last_row) {
      CopyBordersForOneSuperBlockRow(row4x4 + sb4x4, 16, false);
    }
  }
  if (is_last_row && !DoBorderExtensionInLoop()) {
    ExtendBordersForReferenceFrame();
  }
  return is_last_row ? frame_header_.height : progress_row_;
}

}  // namespace libgav1
