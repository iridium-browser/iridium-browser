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

#include "src/internal_frame_buffer_list.h"

#include <cassert>
#include <cstdint>
#include <memory>
#include <new>
#include <utility>

#include "src/utils/common.h"

namespace libgav1 {
extern "C" {

Libgav1StatusCode OnInternalFrameBufferSizeChanged(
    void* callback_private_data, int bitdepth, Libgav1ImageFormat image_format,
    int width, int height, int left_border, int right_border, int top_border,
    int bottom_border, int stride_alignment) {
  auto* buffer_list =
      static_cast<InternalFrameBufferList*>(callback_private_data);
  return buffer_list->OnFrameBufferSizeChanged(
      bitdepth, image_format, width, height, left_border, right_border,
      top_border, bottom_border, stride_alignment);
}

Libgav1StatusCode GetInternalFrameBuffer(
    void* callback_private_data, int bitdepth, Libgav1ImageFormat image_format,
    int width, int height, int left_border, int right_border, int top_border,
    int bottom_border, int stride_alignment, Libgav1FrameBuffer* frame_buffer) {
  auto* buffer_list =
      static_cast<InternalFrameBufferList*>(callback_private_data);
  return buffer_list->GetFrameBuffer(
      bitdepth, image_format, width, height, left_border, right_border,
      top_border, bottom_border, stride_alignment, frame_buffer);
}

void ReleaseInternalFrameBuffer(void* callback_private_data,
                                void* buffer_private_data) {
  auto* buffer_list =
      static_cast<InternalFrameBufferList*>(callback_private_data);
  buffer_list->ReleaseFrameBuffer(buffer_private_data);
}

}  // extern "C"

StatusCode InternalFrameBufferList::OnFrameBufferSizeChanged(
    int /*bitdepth*/, Libgav1ImageFormat /*image_format*/, int /*width*/,
    int /*height*/, int /*left_border*/, int /*right_border*/,
    int /*top_border*/, int /*bottom_border*/, int /*stride_alignment*/) {
  return kStatusOk;
}

StatusCode InternalFrameBufferList::GetFrameBuffer(
    int bitdepth, Libgav1ImageFormat image_format, int width, int height,
    int left_border, int right_border, int top_border, int bottom_border,
    int stride_alignment, Libgav1FrameBuffer* frame_buffer) {
  FrameBufferInfo info;
  StatusCode status = ComputeFrameBufferInfo(
      bitdepth, image_format, width, height, left_border, right_border,
      top_border, bottom_border, stride_alignment, &info);
  if (status != kStatusOk) return status;

  if (info.uv_buffer_size > SIZE_MAX / 2 ||
      info.y_buffer_size > SIZE_MAX - 2 * info.uv_buffer_size) {
    return kStatusInvalidArgument;
  }
  const size_t min_size = info.y_buffer_size + 2 * info.uv_buffer_size;

  Buffer* buffer = nullptr;
  for (auto& buffer_ptr : buffers_) {
    if (!buffer_ptr->in_use) {
      buffer = buffer_ptr.get();
      break;
    }
  }
  if (buffer == nullptr) {
    std::unique_ptr<Buffer> new_buffer(new (std::nothrow) Buffer);
    if (new_buffer == nullptr || !buffers_.push_back(std::move(new_buffer))) {
      return kStatusOutOfMemory;
    }
    buffer = buffers_.back().get();
  }

  if (buffer->size < min_size) {
    std::unique_ptr<uint8_t[], MallocDeleter> new_data(
        static_cast<uint8_t*>(malloc(min_size)));
    if (new_data == nullptr) return kStatusOutOfMemory;
    buffer->data = std::move(new_data);
    buffer->size = min_size;
  }

  uint8_t* const y_buffer = buffer->data.get();
  uint8_t* const u_buffer =
      (info.uv_buffer_size == 0) ? nullptr : y_buffer + info.y_buffer_size;
  uint8_t* const v_buffer =
      (info.uv_buffer_size == 0) ? nullptr : u_buffer + info.uv_buffer_size;
  status = Libgav1SetFrameBuffer(&info, y_buffer, u_buffer, v_buffer, buffer,
                                 frame_buffer);
  if (status != kStatusOk) return status;
  buffer->in_use = true;
  return kStatusOk;
}

void InternalFrameBufferList::ReleaseFrameBuffer(void* buffer_private_data) {
  auto* const buffer = static_cast<Buffer*>(buffer_private_data);
  buffer->in_use = false;
}

}  // namespace libgav1
