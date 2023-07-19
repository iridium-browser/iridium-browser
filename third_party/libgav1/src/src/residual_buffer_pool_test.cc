// Copyright 2021 The libgav1 Authors
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

#include <cstdint>
#include <memory>
#include <utility>

#include "gtest/gtest.h"
#include "src/utils/constants.h"
#include "src/utils/queue.h"
#include "src/utils/types.h"

namespace libgav1 {
namespace {

TEST(ResidualBufferTest, TestUsage) {
  ResidualBufferPool pool(true, 1, 1, sizeof(int16_t));
  EXPECT_EQ(pool.Size(), 0);
  // Get one buffer.
  std::unique_ptr<ResidualBuffer> buffer1 = pool.Get();
  uint8_t* const buffer1_ptr = buffer1->buffer();
  ASSERT_NE(buffer1_ptr, nullptr);
  // Get another buffer (while holding on to the first one).
  std::unique_ptr<ResidualBuffer> buffer2 = pool.Get();
  uint8_t* const buffer2_ptr = buffer2->buffer();
  ASSERT_NE(buffer2_ptr, nullptr);
  EXPECT_NE(buffer1_ptr, buffer2_ptr);
  // Return the second buffer.
  pool.Release(std::move(buffer2));
  EXPECT_EQ(pool.Size(), 1);
  // Get another buffer (this one should be the same as the buffer2).
  std::unique_ptr<ResidualBuffer> buffer3 = pool.Get();
  uint8_t* const buffer3_ptr = buffer3->buffer();
  ASSERT_NE(buffer3_ptr, nullptr);
  EXPECT_EQ(buffer3_ptr, buffer2_ptr);
  EXPECT_EQ(pool.Size(), 0);
  // Get another buffer (this one will be a new buffer).
  std::unique_ptr<ResidualBuffer> buffer4 = pool.Get();
  uint8_t* const buffer4_ptr = buffer4->buffer();
  ASSERT_NE(buffer4_ptr, nullptr);
  EXPECT_NE(buffer4_ptr, buffer1_ptr);
  EXPECT_NE(buffer4_ptr, buffer3_ptr);
  EXPECT_EQ(pool.Size(), 0);
  // Return all the buffers.
  pool.Release(std::move(buffer1));
  EXPECT_EQ(pool.Size(), 1);
  pool.Release(std::move(buffer3));
  EXPECT_EQ(pool.Size(), 2);
  pool.Release(std::move(buffer4));
  EXPECT_EQ(pool.Size(), 3);
  // Reset the buffer with same parameters.
  pool.Reset(true, 1, 1, sizeof(int16_t));
  EXPECT_EQ(pool.Size(), 3);
  // Reset the buffer size with different parameters.
  pool.Reset(true, 0, 1, sizeof(int32_t));
  // The existing buffers should now have been invalidated.
  EXPECT_EQ(pool.Size(), 0);
  // Get and return a buffer.
  std::unique_ptr<ResidualBuffer> buffer5 = pool.Get();
  uint8_t* const buffer5_ptr = buffer5->buffer();
  ASSERT_NE(buffer5_ptr, nullptr);
  pool.Release(std::move(buffer5));
  EXPECT_EQ(pool.Size(), 1);
  // Reset the buffer with different value for use128x128_superblock.
  pool.Reset(false, 0, 1, sizeof(int32_t));
  // The existing buffers should now have been invalidated.
  EXPECT_EQ(pool.Size(), 0);
}

TEST(ResidualBufferTest, TestQueue) {
  ResidualBufferPool pool(true, 1, 1, sizeof(int16_t));
  EXPECT_EQ(pool.Size(), 0);
  // Get one buffer.
  std::unique_ptr<ResidualBuffer> buffer1 = pool.Get();
  uint8_t* const buffer1_ptr = buffer1->buffer();
  ASSERT_NE(buffer1_ptr, nullptr);
  auto* queue1 = buffer1->transform_parameters();
  queue1->Push(TransformParameters(kTransformTypeAdstAdst, 10));
  EXPECT_EQ(queue1->Size(), 1);
  EXPECT_EQ(queue1->Front().type, kTransformTypeAdstAdst);
  EXPECT_EQ(queue1->Front().non_zero_coeff_count, 10);
  queue1->Push(TransformParameters(kTransformTypeDctDct, 20));
  EXPECT_EQ(queue1->Size(), 2);
  EXPECT_EQ(queue1->Front().type, kTransformTypeAdstAdst);
  EXPECT_EQ(queue1->Front().non_zero_coeff_count, 10);
  queue1->Pop();
  EXPECT_EQ(queue1->Size(), 1);
  EXPECT_EQ(queue1->Front().type, kTransformTypeDctDct);
  EXPECT_EQ(queue1->Front().non_zero_coeff_count, 20);
  // Return the buffer.
  pool.Release(std::move(buffer1));
  EXPECT_EQ(pool.Size(), 1);
  // Get another buffer (should be the same as buffer1).
  std::unique_ptr<ResidualBuffer> buffer2 = pool.Get();
  uint8_t* const buffer2_ptr = buffer2->buffer();
  ASSERT_NE(buffer2_ptr, nullptr);
  EXPECT_EQ(buffer1_ptr, buffer2_ptr);
  // Releasing the buffer should've cleared the queue.
  EXPECT_EQ(buffer2->transform_parameters()->Size(), 0);
}

TEST(ResidualBufferTest, TestStackPushPop) {
  ResidualBufferStack buffers;
  EXPECT_EQ(buffers.Size(), 0);
  EXPECT_EQ(buffers.Pop(), nullptr);

  std::unique_ptr<ResidualBuffer> buffer0 = ResidualBuffer::Create(128, 128);
  ResidualBuffer* const buffer0_ptr = buffer0.get();
  EXPECT_NE(buffer0_ptr, nullptr);
  std::unique_ptr<ResidualBuffer> buffer1 = ResidualBuffer::Create(128, 128);
  ResidualBuffer* const buffer1_ptr = buffer1.get();
  EXPECT_NE(buffer1_ptr, nullptr);
  std::unique_ptr<ResidualBuffer> buffer2 = ResidualBuffer::Create(128, 128);
  ResidualBuffer* const buffer2_ptr = buffer2.get();
  EXPECT_NE(buffer2_ptr, nullptr);

  // Push two buffers onto the stack.
  buffers.Push(std::move(buffer0));
  EXPECT_EQ(buffers.Size(), 1);
  buffers.Push(std::move(buffer1));
  EXPECT_EQ(buffers.Size(), 2);

  // Pop one buffer off the stack.
  std::unique_ptr<ResidualBuffer> top = buffers.Pop();
  EXPECT_EQ(buffers.Size(), 1);
  EXPECT_EQ(top.get(), buffer1_ptr);

  // Push one buffer onto the stack.
  buffers.Push(std::move(buffer2));
  EXPECT_EQ(buffers.Size(), 2);

  // Pop two buffers off the stack
  top = buffers.Pop();
  EXPECT_EQ(buffers.Size(), 1);
  EXPECT_EQ(top.get(), buffer2_ptr);
  top = buffers.Pop();
  EXPECT_EQ(buffers.Size(), 0);
  EXPECT_EQ(top.get(), buffer0_ptr);

  // Try to pop a buffer off an empty stack.
  top = buffers.Pop();
  EXPECT_EQ(buffers.Size(), 0);
  EXPECT_EQ(top, nullptr);
}

TEST(ResidualBufferTest, TestStackSwap) {
  ResidualBufferStack buffers;
  EXPECT_EQ(buffers.Size(), 0);
  EXPECT_EQ(buffers.Pop(), nullptr);

  std::unique_ptr<ResidualBuffer> buffer0 = ResidualBuffer::Create(128, 128);
  ResidualBuffer* const buffer0_ptr = buffer0.get();
  EXPECT_NE(buffer0_ptr, nullptr);
  std::unique_ptr<ResidualBuffer> buffer1 = ResidualBuffer::Create(128, 128);
  ResidualBuffer* const buffer1_ptr = buffer1.get();
  EXPECT_NE(buffer1_ptr, nullptr);
  std::unique_ptr<ResidualBuffer> buffer2 = ResidualBuffer::Create(128, 128);
  ResidualBuffer* const buffer2_ptr = buffer2.get();
  EXPECT_NE(buffer2_ptr, nullptr);

  // Push three buffers onto the stack.
  buffers.Push(std::move(buffer0));
  EXPECT_EQ(buffers.Size(), 1);
  buffers.Push(std::move(buffer1));
  EXPECT_EQ(buffers.Size(), 2);
  buffers.Push(std::move(buffer2));
  EXPECT_EQ(buffers.Size(), 3);

  // Swap the contents of the stacks.
  ResidualBufferStack swapped;
  swapped.Swap(&buffers);
  EXPECT_EQ(buffers.Size(), 0);
  EXPECT_EQ(swapped.Size(), 3);

  // Pop three buffers off the swapped stack.
  std::unique_ptr<ResidualBuffer> top = swapped.Pop();
  EXPECT_EQ(swapped.Size(), 2);
  EXPECT_EQ(top.get(), buffer2_ptr);
  top = swapped.Pop();
  EXPECT_EQ(swapped.Size(), 1);
  EXPECT_EQ(top.get(), buffer1_ptr);
  top = swapped.Pop();
  EXPECT_EQ(swapped.Size(), 0);
  EXPECT_EQ(top.get(), buffer0_ptr);
}

}  // namespace
}  // namespace libgav1
