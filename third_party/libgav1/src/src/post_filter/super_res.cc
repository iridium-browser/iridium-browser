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

void PostFilter::ApplySuperRes(const std::array<uint8_t*, kMaxPlanes>& src,
                               const std::array<int, kMaxPlanes>& rows,
                               const int line_buffer_row,
                               const std::array<uint8_t*, kMaxPlanes>& dst,
                               bool dst_is_loop_restoration_border /*=false*/) {
  int plane = kPlaneY;
  do {
    const int plane_width =
        MultiplyBy4(frame_header_.columns4x4) >> subsampling_x_[plane];
#if LIBGAV1_MAX_BITDEPTH >= 10
    if (bitdepth_ >= 10) {
      auto* input = reinterpret_cast<uint16_t*>(src[plane]);
      auto* output = reinterpret_cast<uint16_t*>(dst[plane]);
      const ptrdiff_t input_stride =
          frame_buffer_.stride(plane) / sizeof(uint16_t);
      const ptrdiff_t output_stride =
          (dst_is_loop_restoration_border
               ? loop_restoration_border_.stride(plane)
               : frame_buffer_.stride(plane)) /
          sizeof(uint16_t);
      if (rows[plane] > 0) {
        dsp_.super_res(superres_coefficients_[static_cast<int>(plane != 0)],
                       input, input_stride, rows[plane], plane_width,
                       super_res_info_[plane].upscaled_width,
                       super_res_info_[plane].initial_subpixel_x,
                       super_res_info_[plane].step, output, output_stride);
      }
      // In the multi-threaded case, the |superres_line_buffer_| holds the last
      // input row. Apply SuperRes for that row.
      if (line_buffer_row >= 0) {
        auto* const line_buffer_start =
            reinterpret_cast<uint16_t*>(superres_line_buffer_.data(plane)) +
            line_buffer_row * superres_line_buffer_.stride(plane) /
                sizeof(uint16_t) +
            kSuperResHorizontalBorder;
        dsp_.super_res(superres_coefficients_[static_cast<int>(plane != 0)],
                       line_buffer_start, /*source_stride=*/0,
                       /*height=*/1, plane_width,
                       super_res_info_[plane].upscaled_width,
                       super_res_info_[plane].initial_subpixel_x,
                       super_res_info_[plane].step,
                       output + rows[plane] * output_stride, /*dest_stride=*/0);
      }
      continue;
    }
#endif  // LIBGAV1_MAX_BITDEPTH >= 10
    uint8_t* input = src[plane];
    uint8_t* output = dst[plane];
    const ptrdiff_t input_stride = frame_buffer_.stride(plane);
    const ptrdiff_t output_stride = dst_is_loop_restoration_border
                                        ? loop_restoration_border_.stride(plane)
                                        : frame_buffer_.stride(plane);
    if (rows[plane] > 0) {
      dsp_.super_res(superres_coefficients_[static_cast<int>(plane != 0)],
                     input, input_stride, rows[plane], plane_width,
                     super_res_info_[plane].upscaled_width,
                     super_res_info_[plane].initial_subpixel_x,
                     super_res_info_[plane].step, output, output_stride);
    }
    // In the multi-threaded case, the |superres_line_buffer_| holds the last
    // input row. Apply SuperRes for that row.
    if (line_buffer_row >= 0) {
      uint8_t* const line_buffer_start =
          superres_line_buffer_.data(plane) +
          line_buffer_row * superres_line_buffer_.stride(plane) +
          kSuperResHorizontalBorder;
      dsp_.super_res(
          superres_coefficients_[static_cast<int>(plane != 0)],
          line_buffer_start, /*source_stride=*/0,
          /*height=*/1, plane_width, super_res_info_[plane].upscaled_width,
          super_res_info_[plane].initial_subpixel_x,
          super_res_info_[plane].step, output + rows[plane] * output_stride,
          /*dest_stride=*/0);
    }
  } while (++plane < planes_);
}

void PostFilter::ApplySuperResForOneSuperBlockRow(int row4x4_start, int sb4x4,
                                                  bool is_last_row) {
  assert(row4x4_start >= 0);
  assert(DoSuperRes());
  // If not doing cdef, then LR needs two rows of border with superres applied.
  const int num_rows_extra = (DoCdef() || !DoRestoration()) ? 0 : 2;
  std::array<uint8_t*, kMaxPlanes> src;
  std::array<uint8_t*, kMaxPlanes> dst;
  std::array<int, kMaxPlanes> rows;
  const int num_rows4x4 =
      std::min(sb4x4, frame_header_.rows4x4 - row4x4_start) -
      (is_last_row ? 0 : 2);
  if (row4x4_start > 0) {
    const int row4x4 = row4x4_start - 2;
    int plane = kPlaneY;
    do {
      const int row =
          (MultiplyBy4(row4x4) >> subsampling_y_[plane]) + num_rows_extra;
      const ptrdiff_t row_offset = row * frame_buffer_.stride(plane);
      src[plane] = cdef_buffer_[plane] + row_offset;
      dst[plane] = superres_buffer_[plane] + row_offset;
      // Note that the |num_rows_extra| subtraction is done after the value is
      // subsampled since we always need to work on |num_rows_extra| extra rows
      // irrespective of the plane subsampling.
      // Apply superres for the last 8-|num_rows_extra| rows of the previous
      // superblock.
      rows[plane] = (8 >> subsampling_y_[plane]) - num_rows_extra;
      // Apply superres for the current superblock row (except for the last
      // 8-|num_rows_extra| rows).
      rows[plane] += (MultiplyBy4(num_rows4x4) >> subsampling_y_[plane]) +
                     (is_last_row ? 0 : num_rows_extra);
    } while (++plane < planes_);
  } else {
    // Apply superres for the current superblock row (except for the last
    // 8-|num_rows_extra| rows).
    int plane = kPlaneY;
    do {
      const ptrdiff_t row_offset =
          (MultiplyBy4(row4x4_start) >> subsampling_y_[plane]) *
          frame_buffer_.stride(plane);
      src[plane] = cdef_buffer_[plane] + row_offset;
      dst[plane] = superres_buffer_[plane] + row_offset;
      // Note that the |num_rows_extra| addition is done after the value is
      // subsampled since we always need to work on |num_rows_extra| extra rows
      // irrespective of the plane subsampling.
      rows[plane] = (MultiplyBy4(num_rows4x4) >> subsampling_y_[plane]) +
                    (is_last_row ? 0 : num_rows_extra);
    } while (++plane < planes_);
  }
  ApplySuperRes(src, rows, /*line_buffer_row=*/-1, dst);
}

void PostFilter::ApplySuperResThreaded() {
  int num_threads = thread_pool_->num_threads() + 1;
  // The number of rows that will be processed by each thread in the thread pool
  // (other than the current thread).
  int thread_pool_rows = frame_header_.height / num_threads;
  thread_pool_rows = std::max(thread_pool_rows, 1);
  // Make rows of Y plane even when there is subsampling for the other planes.
  if ((thread_pool_rows & 1) != 0 && subsampling_y_[kPlaneU] != 0) {
    ++thread_pool_rows;
  }
  // Adjust the number of threads to what we really need.
  num_threads = Clip3(frame_header_.height / thread_pool_rows, 1, num_threads);
  // For the current thread, we round up to process all the remaining rows.
  int current_thread_rows =
      frame_header_.height - thread_pool_rows * (num_threads - 1);
  // Make rows of Y plane even when there is subsampling for the other planes.
  if ((current_thread_rows & 1) != 0 && subsampling_y_[kPlaneU] != 0) {
    ++current_thread_rows;
  }
  assert(current_thread_rows > 0);
  BlockingCounter pending_workers(num_threads - 1);
  for (int line_buffer_row = 0, row_start = 0; line_buffer_row < num_threads;
       ++line_buffer_row, row_start += thread_pool_rows) {
    std::array<uint8_t*, kMaxPlanes> src;
    std::array<uint8_t*, kMaxPlanes> dst;
    std::array<int, kMaxPlanes> rows;
    int plane = kPlaneY;
    const int pixel_size_log2 = pixel_size_log2_;
    do {
      src[plane] =
          GetBufferOffset(cdef_buffer_[plane], frame_buffer_.stride(plane),
                          static_cast<Plane>(plane), row_start, 0);
      dst[plane] =
          GetBufferOffset(superres_buffer_[plane], frame_buffer_.stride(plane),
                          static_cast<Plane>(plane), row_start, 0);
      rows[plane] =
          (((line_buffer_row < num_threads - 1) ? thread_pool_rows
                                                : current_thread_rows) >>
           subsampling_y_[plane]) -
          1;
      const int plane_width =
          MultiplyBy4(frame_header_.columns4x4) >> subsampling_x_[plane];
      uint8_t* const input =
          src[plane] + rows[plane] * frame_buffer_.stride(plane);
      uint8_t* const line_buffer_start =
          superres_line_buffer_.data(plane) +
          line_buffer_row * superres_line_buffer_.stride(plane) +
          (kSuperResHorizontalBorder << pixel_size_log2);
      memcpy(line_buffer_start, input, plane_width << pixel_size_log2);
    } while (++plane < planes_);
    if (line_buffer_row < num_threads - 1) {
      thread_pool_->Schedule(
          [this, src, rows, line_buffer_row, dst, &pending_workers]() {
            ApplySuperRes(src, rows, line_buffer_row, dst);
            pending_workers.Decrement();
          });
    } else {
      ApplySuperRes(src, rows, line_buffer_row, dst);
    }
  }
  // Wait for the threadpool jobs to finish.
  pending_workers.Wait();
}

}  // namespace libgav1
