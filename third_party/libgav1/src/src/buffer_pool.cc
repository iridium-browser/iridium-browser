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

#include "src/buffer_pool.h"

#include <cassert>
#include <cstring>

#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/logging.h"

namespace libgav1 {

namespace {

// Copies the feature_enabled, feature_data, segment_id_pre_skip, and
// last_active_segment_id fields of Segmentation.
void CopySegmentationParameters(const Segmentation& from, Segmentation* to) {
  memcpy(to->feature_enabled, from.feature_enabled,
         sizeof(to->feature_enabled));
  memcpy(to->feature_data, from.feature_data, sizeof(to->feature_data));
  to->segment_id_pre_skip = from.segment_id_pre_skip;
  to->last_active_segment_id = from.last_active_segment_id;
}

}  // namespace

RefCountedBuffer::RefCountedBuffer() = default;

RefCountedBuffer::~RefCountedBuffer() = default;

bool RefCountedBuffer::Realloc(int bitdepth, bool is_monochrome, int width,
                               int height, int subsampling_x, int subsampling_y,
                               int left_border, int right_border,
                               int top_border, int bottom_border) {
  // The YuvBuffer::Realloc() could call the get frame buffer callback which
  // will need to be thread safe. So we ensure that we only call Realloc() once
  // at any given time.
  std::lock_guard<std::mutex> lock(pool_->mutex_);
  assert(!buffer_private_data_valid_);
  if (!yuv_buffer_.Realloc(
          bitdepth, is_monochrome, width, height, subsampling_x, subsampling_y,
          left_border, right_border, top_border, bottom_border,
          pool_->get_frame_buffer_, pool_->callback_private_data_,
          &buffer_private_data_)) {
    return false;
  }
  buffer_private_data_valid_ = true;
  return true;
}

bool RefCountedBuffer::SetFrameDimensions(const ObuFrameHeader& frame_header) {
  upscaled_width_ = frame_header.upscaled_width;
  frame_width_ = frame_header.width;
  frame_height_ = frame_header.height;
  render_width_ = frame_header.render_width;
  render_height_ = frame_header.render_height;
  rows4x4_ = frame_header.rows4x4;
  columns4x4_ = frame_header.columns4x4;
  if (frame_header.refresh_frame_flags != 0 &&
      !IsIntraFrame(frame_header.frame_type)) {
    const int rows4x4_half = DivideBy2(rows4x4_);
    const int columns4x4_half = DivideBy2(columns4x4_);
    if (!reference_info_.Reset(rows4x4_half, columns4x4_half)) {
      return false;
    }
  }
  return segmentation_map_.Allocate(rows4x4_, columns4x4_);
}

void RefCountedBuffer::SetGlobalMotions(
    const std::array<GlobalMotion, kNumReferenceFrameTypes>& global_motions) {
  for (int ref = kReferenceFrameLast; ref <= kReferenceFrameAlternate; ++ref) {
    static_assert(sizeof(global_motion_[ref].params) ==
                      sizeof(global_motions[ref].params),
                  "");
    memcpy(global_motion_[ref].params, global_motions[ref].params,
           sizeof(global_motion_[ref].params));
  }
}

void RefCountedBuffer::SetFrameContext(const SymbolDecoderContext& context) {
  frame_context_ = context;
  frame_context_.ResetIntraFrameYModeCdf();
  frame_context_.ResetCounters();
}

void RefCountedBuffer::GetSegmentationParameters(
    Segmentation* segmentation) const {
  CopySegmentationParameters(/*from=*/segmentation_, /*to=*/segmentation);
}

void RefCountedBuffer::SetSegmentationParameters(
    const Segmentation& segmentation) {
  CopySegmentationParameters(/*from=*/segmentation, /*to=*/&segmentation_);
}

void RefCountedBuffer::SetBufferPool(BufferPool* pool) { pool_ = pool; }

void RefCountedBuffer::ReturnToBufferPool(RefCountedBuffer* ptr) {
  ptr->pool_->ReturnUnusedBuffer(ptr);
}

BufferPool::BufferPool(
    FrameBufferSizeChangedCallback on_frame_buffer_size_changed,
    GetFrameBufferCallback get_frame_buffer,
    ReleaseFrameBufferCallback release_frame_buffer,
    void* callback_private_data) {
  if (get_frame_buffer != nullptr) {
    // on_frame_buffer_size_changed may be null.
    assert(release_frame_buffer != nullptr);
    on_frame_buffer_size_changed_ = on_frame_buffer_size_changed;
    get_frame_buffer_ = get_frame_buffer;
    release_frame_buffer_ = release_frame_buffer;
    callback_private_data_ = callback_private_data;
  } else {
    on_frame_buffer_size_changed_ = OnInternalFrameBufferSizeChanged;
    get_frame_buffer_ = GetInternalFrameBuffer;
    release_frame_buffer_ = ReleaseInternalFrameBuffer;
    callback_private_data_ = &internal_frame_buffers_;
  }
}

BufferPool::~BufferPool() {
  for (const auto* buffer : buffers_) {
    if (buffer->in_use_) {
      assert(false && "RefCountedBuffer still in use at destruction time.");
      LIBGAV1_DLOG(ERROR, "RefCountedBuffer still in use at destruction time.");
    }
    delete buffer;
  }
}

bool BufferPool::OnFrameBufferSizeChanged(int bitdepth,
                                          Libgav1ImageFormat image_format,
                                          int width, int height,
                                          int left_border, int right_border,
                                          int top_border, int bottom_border) {
  if (on_frame_buffer_size_changed_ == nullptr) return true;
  return on_frame_buffer_size_changed_(callback_private_data_, bitdepth,
                                       image_format, width, height, left_border,
                                       right_border, top_border, bottom_border,
                                       /*stride_alignment=*/16) == kStatusOk;
}

RefCountedBufferPtr BufferPool::GetFreeBuffer() {
  std::unique_lock<std::mutex> lock(mutex_);
  for (auto buffer : buffers_) {
    if (!buffer->in_use_) {
      buffer->in_use_ = true;
      buffer->progress_row_ = -1;
      buffer->frame_state_ = kFrameStateUnknown;
      buffer->hdr_cll_set_ = false;
      buffer->hdr_mdcv_set_ = false;
      buffer->itut_t35_set_ = false;
      lock.unlock();
      return RefCountedBufferPtr(buffer, RefCountedBuffer::ReturnToBufferPool);
    }
  }
  lock.unlock();
  auto* const buffer = new (std::nothrow) RefCountedBuffer();
  if (buffer == nullptr) {
    LIBGAV1_DLOG(ERROR, "Failed to allocate a new reference counted buffer.");
    return RefCountedBufferPtr();
  }
  buffer->SetBufferPool(this);
  buffer->in_use_ = true;
  buffer->progress_row_ = -1;
  buffer->frame_state_ = kFrameStateUnknown;
  lock.lock();
  const bool ok = buffers_.push_back(buffer);
  lock.unlock();
  if (!ok) {
    LIBGAV1_DLOG(
        ERROR,
        "Failed to push the new reference counted buffer into the vector.");
    delete buffer;
    return RefCountedBufferPtr();
  }
  return RefCountedBufferPtr(buffer, RefCountedBuffer::ReturnToBufferPool);
}

void BufferPool::Abort() {
  std::unique_lock<std::mutex> lock(mutex_);
  for (auto buffer : buffers_) {
    if (buffer->in_use_) {
      buffer->Abort();
    }
  }
}

void BufferPool::ReturnUnusedBuffer(RefCountedBuffer* buffer) {
  std::lock_guard<std::mutex> lock(mutex_);
  assert(buffer->in_use_);
  buffer->in_use_ = false;
  if (buffer->buffer_private_data_valid_) {
    release_frame_buffer_(callback_private_data_, buffer->buffer_private_data_);
    buffer->buffer_private_data_valid_ = false;
  }
}

}  // namespace libgav1
