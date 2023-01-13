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

#include "src/residual_buffer_pool.h"

#include <mutex>  // NOLINT (unapproved c++11 header)
#include <utility>

namespace libgav1 {
namespace {

// The maximum queue size is derived using the following formula:
//   ((sb_size * sb_size) / 16) + (2 * (((sb_size / x) * (sb_size / y)) / 16)).
// Where:
//   sb_size is the superblock size (64 or 128).
//   16 is 4*4 which is kMinTransformWidth * kMinTransformHeight.
//   x is subsampling_x + 1.
//   y is subsampling_y + 1.
// The first component is for the Y plane and the second component is for the U
// and V planes.
// For example, for 128x128 superblocks with 422 subsampling the size is:
//   ((128 * 128) / 16) + (2 * (((128 / 2) * (128 / 1)) / 16)) = 2048.
//
// First dimension: use_128x128_superblock.
// Second dimension: subsampling_x.
// Third dimension: subsampling_y.
constexpr int kMaxQueueSize[2][2][2] = {
    // 64x64 superblocks.
    {
        {768, 512},
        {512, 384},
    },
    // 128x128 superblocks.
    {
        {3072, 2048},
        {2048, 1536},
    },
};

}  // namespace

ResidualBufferStack::~ResidualBufferStack() {
  while (top_ != nullptr) {
    ResidualBuffer* top = top_;
    top_ = top_->next_;
    delete top;
  }
}

void ResidualBufferStack::Push(std::unique_ptr<ResidualBuffer> buffer) {
  buffer->next_ = top_;
  top_ = buffer.release();
  ++num_buffers_;
}

std::unique_ptr<ResidualBuffer> ResidualBufferStack::Pop() {
  std::unique_ptr<ResidualBuffer> top;
  if (top_ != nullptr) {
    top.reset(top_);
    top_ = top_->next_;
    top->next_ = nullptr;
    --num_buffers_;
  }
  return top;
}

void ResidualBufferStack::Swap(ResidualBufferStack* other) {
  std::swap(top_, other->top_);
  std::swap(num_buffers_, other->num_buffers_);
}

ResidualBufferPool::ResidualBufferPool(bool use_128x128_superblock,
                                       int subsampling_x, int subsampling_y,
                                       size_t residual_size)
    : buffer_size_(GetResidualBufferSize(
          use_128x128_superblock ? 128 : 64, use_128x128_superblock ? 128 : 64,
          subsampling_x, subsampling_y, residual_size)),
      queue_size_(kMaxQueueSize[static_cast<int>(use_128x128_superblock)]
                               [subsampling_x][subsampling_y]) {}

void ResidualBufferPool::Reset(bool use_128x128_superblock, int subsampling_x,
                               int subsampling_y, size_t residual_size) {
  const size_t buffer_size = GetResidualBufferSize(
      use_128x128_superblock ? 128 : 64, use_128x128_superblock ? 128 : 64,
      subsampling_x, subsampling_y, residual_size);
  const int queue_size = kMaxQueueSize[static_cast<int>(use_128x128_superblock)]
                                      [subsampling_x][subsampling_y];
  if (buffer_size == buffer_size_ && queue_size == queue_size_) {
    // The existing buffers (if any) are still valid, so don't do anything.
    return;
  }
  buffer_size_ = buffer_size;
  queue_size_ = queue_size;
  // The existing buffers (if any) are no longer valid since the buffer size or
  // the queue size has changed. Clear the stack.
  ResidualBufferStack buffers;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    // Move the buffers in the stack to the local variable |buffers| and clear
    // the stack.
    buffers.Swap(&buffers_);
    // Release mutex_ before freeing the buffers.
  }
  // As the local variable |buffers| goes out of scope, its destructor frees
  // the buffers that were in the stack.
}

std::unique_ptr<ResidualBuffer> ResidualBufferPool::Get() {
  std::unique_ptr<ResidualBuffer> buffer = nullptr;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    buffer = buffers_.Pop();
  }
  if (buffer == nullptr) {
    buffer = ResidualBuffer::Create(buffer_size_, queue_size_);
  }
  return buffer;
}

void ResidualBufferPool::Release(std::unique_ptr<ResidualBuffer> buffer) {
  buffer->transform_parameters()->Clear();
  buffer->partition_tree_order()->Clear();
  std::lock_guard<std::mutex> lock(mutex_);
  buffers_.Push(std::move(buffer));
}

size_t ResidualBufferPool::Size() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return buffers_.Size();
}

}  // namespace libgav1
