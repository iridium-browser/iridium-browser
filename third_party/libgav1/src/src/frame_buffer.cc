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

#include "src/gav1/frame_buffer.h"

#include <cstdint>

#include "src/frame_buffer_utils.h"
#include "src/utils/common.h"

extern "C" {

Libgav1StatusCode Libgav1ComputeFrameBufferInfo(
    int bitdepth, Libgav1ImageFormat image_format, int width, int height,
    int left_border, int right_border, int top_border, int bottom_border,
    int stride_alignment, Libgav1FrameBufferInfo* info) {
  switch (bitdepth) {
    case 8:
#if LIBGAV1_MAX_BITDEPTH >= 10
    case 10:
#endif
#if LIBGAV1_MAX_BITDEPTH == 12
    case 12:
#endif
      break;
    default:
      return kLibgav1StatusInvalidArgument;
  }
  switch (image_format) {
    case kLibgav1ImageFormatYuv420:
    case kLibgav1ImageFormatYuv422:
    case kLibgav1ImageFormatYuv444:
    case kLibgav1ImageFormatMonochrome400:
      break;
    default:
      return kLibgav1StatusInvalidArgument;
  }
  // All int arguments must be nonnegative. Borders must be a multiple of 2.
  // |stride_alignment| must be a power of 2.
  if ((width | height | left_border | right_border | top_border |
       bottom_border | stride_alignment) < 0 ||
      ((left_border | right_border | top_border | bottom_border) & 1) != 0 ||
      (stride_alignment & (stride_alignment - 1)) != 0 || info == nullptr) {
    return kLibgav1StatusInvalidArgument;
  }

  bool is_monochrome;
  int8_t subsampling_x;
  int8_t subsampling_y;
  libgav1::DecomposeImageFormat(image_format, &is_monochrome, &subsampling_x,
                                &subsampling_y);

  // Calculate y_stride (in bytes). It is padded to a multiple of
  // |stride_alignment| bytes.
  int y_stride = width + left_border + right_border;
#if LIBGAV1_MAX_BITDEPTH >= 10
  if (bitdepth > 8) y_stride *= sizeof(uint16_t);
#endif
  y_stride = libgav1::Align(y_stride, stride_alignment);
  // Size of the Y buffer in bytes.
  const uint64_t y_buffer_size =
      (height + top_border + bottom_border) * static_cast<uint64_t>(y_stride) +
      (stride_alignment - 1);

  const int uv_width =
      is_monochrome ? 0 : libgav1::SubsampledValue(width, subsampling_x);
  const int uv_height =
      is_monochrome ? 0 : libgav1::SubsampledValue(height, subsampling_y);
  const int uv_left_border = is_monochrome ? 0 : left_border >> subsampling_x;
  const int uv_right_border = is_monochrome ? 0 : right_border >> subsampling_x;
  const int uv_top_border = is_monochrome ? 0 : top_border >> subsampling_y;
  const int uv_bottom_border =
      is_monochrome ? 0 : bottom_border >> subsampling_y;

  // Calculate uv_stride (in bytes). It is padded to a multiple of
  // |stride_alignment| bytes.
  int uv_stride = uv_width + uv_left_border + uv_right_border;
#if LIBGAV1_MAX_BITDEPTH >= 10
  if (bitdepth > 8) uv_stride *= sizeof(uint16_t);
#endif
  uv_stride = libgav1::Align(uv_stride, stride_alignment);
  // Size of the U or V buffer in bytes.
  const uint64_t uv_buffer_size =
      is_monochrome ? 0
                    : (uv_height + uv_top_border + uv_bottom_border) *
                              static_cast<uint64_t>(uv_stride) +
                          (stride_alignment - 1);

  // Check if it is safe to cast y_buffer_size and uv_buffer_size to size_t.
  if (y_buffer_size > SIZE_MAX || uv_buffer_size > SIZE_MAX) {
    return kLibgav1StatusInvalidArgument;
  }

  int left_border_bytes = left_border;
  int uv_left_border_bytes = uv_left_border;
#if LIBGAV1_MAX_BITDEPTH >= 10
  if (bitdepth > 8) {
    left_border_bytes *= sizeof(uint16_t);
    uv_left_border_bytes *= sizeof(uint16_t);
  }
#endif

  info->y_stride = y_stride;
  info->uv_stride = uv_stride;
  info->y_buffer_size = static_cast<size_t>(y_buffer_size);
  info->uv_buffer_size = static_cast<size_t>(uv_buffer_size);
  info->y_plane_offset = top_border * y_stride + left_border_bytes;
  info->uv_plane_offset = uv_top_border * uv_stride + uv_left_border_bytes;
  info->stride_alignment = stride_alignment;
  return kLibgav1StatusOk;
}

Libgav1StatusCode Libgav1SetFrameBuffer(const Libgav1FrameBufferInfo* info,
                                        uint8_t* y_buffer, uint8_t* u_buffer,
                                        uint8_t* v_buffer,
                                        void* buffer_private_data,
                                        Libgav1FrameBuffer* frame_buffer) {
  if (info == nullptr ||
      (info->uv_buffer_size == 0 &&
       (u_buffer != nullptr || v_buffer != nullptr)) ||
      frame_buffer == nullptr) {
    return kLibgav1StatusInvalidArgument;
  }
  if (y_buffer == nullptr || (info->uv_buffer_size != 0 &&
                              (u_buffer == nullptr || v_buffer == nullptr))) {
    return kLibgav1StatusOutOfMemory;
  }
  frame_buffer->plane[0] = libgav1::AlignAddr(y_buffer + info->y_plane_offset,
                                              info->stride_alignment);
  frame_buffer->plane[1] = libgav1::AlignAddr(u_buffer + info->uv_plane_offset,
                                              info->stride_alignment);
  frame_buffer->plane[2] = libgav1::AlignAddr(v_buffer + info->uv_plane_offset,
                                              info->stride_alignment);
  frame_buffer->stride[0] = info->y_stride;
  frame_buffer->stride[1] = frame_buffer->stride[2] = info->uv_stride;
  frame_buffer->private_data = buffer_private_data;
  return kLibgav1StatusOk;
}

}  // extern "C"
