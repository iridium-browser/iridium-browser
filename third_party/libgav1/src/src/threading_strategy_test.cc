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

#include "src/threading_strategy.h"

#include <memory>
#include <utility>
#include <vector>

#include "absl/strings/str_cat.h"
#include "gtest/gtest.h"
#include "src/frame_scratch_buffer.h"
#include "src/obu_parser.h"
#include "src/utils/constants.h"
#include "src/utils/threadpool.h"
#include "src/utils/types.h"

namespace libgav1 {
namespace {

class ThreadingStrategyTest : public testing::Test {
 protected:
  ThreadingStrategy strategy_;
  ObuFrameHeader frame_header_ = {};
};

TEST_F(ThreadingStrategyTest, MaxThreadEnforced) {
  frame_header_.tile_info.tile_count = 32;
  ASSERT_TRUE(strategy_.Reset(frame_header_, 32));
  EXPECT_NE(strategy_.tile_thread_pool(), nullptr);
  for (int i = 0; i < 32; ++i) {
    EXPECT_EQ(strategy_.row_thread_pool(i), nullptr);
  }
  EXPECT_NE(strategy_.post_filter_thread_pool(), nullptr);
}

TEST_F(ThreadingStrategyTest, UseAllThreadsForTiles) {
  frame_header_.tile_info.tile_count = 8;
  ASSERT_TRUE(strategy_.Reset(frame_header_, 8));
  EXPECT_NE(strategy_.tile_thread_pool(), nullptr);
  for (int i = 0; i < 8; ++i) {
    EXPECT_EQ(strategy_.row_thread_pool(i), nullptr);
  }
  EXPECT_NE(strategy_.post_filter_thread_pool(), nullptr);
}

TEST_F(ThreadingStrategyTest, RowThreads) {
  frame_header_.tile_info.tile_count = 2;
  ASSERT_TRUE(strategy_.Reset(frame_header_, 8));
  EXPECT_NE(strategy_.tile_thread_pool(), nullptr);
  // Each tile should get 3 threads each.
  for (int i = 0; i < 2; ++i) {
    EXPECT_NE(strategy_.row_thread_pool(i), nullptr);
  }
  EXPECT_NE(strategy_.post_filter_thread_pool(), nullptr);
}

TEST_F(ThreadingStrategyTest, RowThreadsUnequal) {
  frame_header_.tile_info.tile_count = 2;

  ASSERT_TRUE(strategy_.Reset(frame_header_, 9));
  EXPECT_NE(strategy_.tile_thread_pool(), nullptr);
  EXPECT_NE(strategy_.row_thread_pool(0), nullptr);
  EXPECT_NE(strategy_.row_thread_pool(1), nullptr);
  EXPECT_NE(strategy_.post_filter_thread_pool(), nullptr);
}

// Test a random combination of tile_count and thread_count.
TEST_F(ThreadingStrategyTest, MultipleCalls) {
  frame_header_.tile_info.tile_count = 2;
  ASSERT_TRUE(strategy_.Reset(frame_header_, 8));
  EXPECT_NE(strategy_.tile_thread_pool(), nullptr);
  for (int i = 0; i < 2; ++i) {
    EXPECT_NE(strategy_.row_thread_pool(i), nullptr);
  }
  EXPECT_NE(strategy_.post_filter_thread_pool(), nullptr);

  frame_header_.tile_info.tile_count = 8;
  ASSERT_TRUE(strategy_.Reset(frame_header_, 8));
  EXPECT_NE(strategy_.tile_thread_pool(), nullptr);
  // Row threads must have been reset.
  for (int i = 0; i < 8; ++i) {
    EXPECT_EQ(strategy_.row_thread_pool(i), nullptr);
  }
  EXPECT_NE(strategy_.post_filter_thread_pool(), nullptr);

  frame_header_.tile_info.tile_count = 8;
  ASSERT_TRUE(strategy_.Reset(frame_header_, 16));
  EXPECT_NE(strategy_.tile_thread_pool(), nullptr);
  for (int i = 0; i < 8; ++i) {
    // See ThreadingStrategy::Reset().
#if defined(__ANDROID__)
    if (i >= 4) {
      EXPECT_EQ(strategy_.row_thread_pool(i), nullptr) << "i = " << i;
      continue;
    }
#endif
    EXPECT_NE(strategy_.row_thread_pool(i), nullptr) << "i = " << i;
  }
  EXPECT_NE(strategy_.post_filter_thread_pool(), nullptr);

  frame_header_.tile_info.tile_count = 4;
  ASSERT_TRUE(strategy_.Reset(frame_header_, 16));
  EXPECT_NE(strategy_.tile_thread_pool(), nullptr);
  for (int i = 0; i < 4; ++i) {
    EXPECT_NE(strategy_.row_thread_pool(i), nullptr);
  }
  // All the other row threads must be reset.
  for (int i = 4; i < 8; ++i) {
    EXPECT_EQ(strategy_.row_thread_pool(i), nullptr);
  }
  EXPECT_NE(strategy_.post_filter_thread_pool(), nullptr);

  frame_header_.tile_info.tile_count = 4;
  ASSERT_TRUE(strategy_.Reset(frame_header_, 6));
  EXPECT_NE(strategy_.tile_thread_pool(), nullptr);
  // First two tiles will get 1 thread each.
  for (int i = 0; i < 2; ++i) {
    // See ThreadingStrategy::Reset().
#if defined(__ANDROID__)
    if (i == 1) {
      EXPECT_EQ(strategy_.row_thread_pool(i), nullptr) << "i = " << i;
      continue;
    }
#endif
    EXPECT_NE(strategy_.row_thread_pool(i), nullptr) << "i = " << i;
  }
  // All the other row threads must be reset.
  for (int i = 2; i < 8; ++i) {
    EXPECT_EQ(strategy_.row_thread_pool(i), nullptr) << "i = " << i;
  }
  EXPECT_NE(strategy_.post_filter_thread_pool(), nullptr);

  ASSERT_TRUE(strategy_.Reset(frame_header_, 1));
  EXPECT_EQ(strategy_.tile_thread_pool(), nullptr);
  for (int i = 0; i < 8; ++i) {
    EXPECT_EQ(strategy_.row_thread_pool(i), nullptr);
  }
  EXPECT_EQ(strategy_.post_filter_thread_pool(), nullptr);
}

// Tests the following order of calls (with thread count fixed at 4):
//  * 1 Tile - 2 Tiles - 1 Tile.
TEST_F(ThreadingStrategyTest, MultipleCalls2) {
  frame_header_.tile_info.tile_count = 1;
  ASSERT_TRUE(strategy_.Reset(frame_header_, 4));
  // When there is only one tile, tile thread pool must be nullptr.
  EXPECT_EQ(strategy_.tile_thread_pool(), nullptr);
  EXPECT_NE(strategy_.row_thread_pool(0), nullptr);
  for (int i = 1; i < 8; ++i) {
    EXPECT_EQ(strategy_.row_thread_pool(i), nullptr);
  }
  EXPECT_NE(strategy_.post_filter_thread_pool(), nullptr);

  frame_header_.tile_info.tile_count = 2;
  ASSERT_TRUE(strategy_.Reset(frame_header_, 4));
  EXPECT_NE(strategy_.tile_thread_pool(), nullptr);
  for (int i = 0; i < 2; ++i) {
    // See ThreadingStrategy::Reset().
#if defined(__ANDROID__)
    if (i == 1) {
      EXPECT_EQ(strategy_.row_thread_pool(i), nullptr) << "i = " << i;
      continue;
    }
#endif
    EXPECT_NE(strategy_.row_thread_pool(i), nullptr);
  }
  for (int i = 2; i < 8; ++i) {
    EXPECT_EQ(strategy_.row_thread_pool(i), nullptr);
  }
  EXPECT_NE(strategy_.post_filter_thread_pool(), nullptr);

  frame_header_.tile_info.tile_count = 1;
  ASSERT_TRUE(strategy_.Reset(frame_header_, 4));
  EXPECT_EQ(strategy_.tile_thread_pool(), nullptr);
  EXPECT_NE(strategy_.row_thread_pool(0), nullptr);
  for (int i = 1; i < 8; ++i) {
    EXPECT_EQ(strategy_.row_thread_pool(i), nullptr);
  }
  EXPECT_NE(strategy_.post_filter_thread_pool(), nullptr);
}

void VerifyFrameParallel(int thread_count, int tile_count, int tile_columns,
                         int expected_frame_threads,
                         const std::vector<int>& expected_tile_threads) {
  ASSERT_EQ(expected_frame_threads, expected_tile_threads.size());
  ASSERT_GT(thread_count, 1);
  std::unique_ptr<ThreadPool> frame_thread_pool;
  FrameScratchBufferPool frame_scratch_buffer_pool;
  ASSERT_TRUE(InitializeThreadPoolsForFrameParallel(
      thread_count, tile_count, tile_columns, &frame_thread_pool,
      &frame_scratch_buffer_pool));
  if (expected_frame_threads == 0) {
    EXPECT_EQ(frame_thread_pool, nullptr);
    return;
  }
  EXPECT_NE(frame_thread_pool.get(), nullptr);
  EXPECT_EQ(frame_thread_pool->num_threads(), expected_frame_threads);
  std::vector<std::unique_ptr<FrameScratchBuffer>> frame_scratch_buffers;
  int actual_thread_count = frame_thread_pool->num_threads();
  for (int i = 0; i < expected_frame_threads; ++i) {
    SCOPED_TRACE(absl::StrCat("i: ", i));
    frame_scratch_buffers.push_back(frame_scratch_buffer_pool.Get());
    ThreadPool* const thread_pool =
        frame_scratch_buffers.back()->threading_strategy.thread_pool();
    if (expected_tile_threads[i] > 0) {
      EXPECT_NE(thread_pool, nullptr);
      EXPECT_EQ(thread_pool->num_threads(), expected_tile_threads[i]);
      actual_thread_count += thread_pool->num_threads();
    } else {
      EXPECT_EQ(thread_pool, nullptr);
    }
  }
  EXPECT_EQ(thread_count, actual_thread_count);
  for (auto& frame_scratch_buffer : frame_scratch_buffers) {
    frame_scratch_buffer_pool.Release(std::move(frame_scratch_buffer));
  }
}

TEST(FrameParallelStrategyTest, FrameParallel) {
  // This loop has thread_count <= 3 * tile count. So there should be no frame
  // threads irrespective of the number of tile columns.
  for (int thread_count = 2; thread_count <= 6; ++thread_count) {
    VerifyFrameParallel(thread_count, /*tile_count=*/2, /*tile_columns=*/1,
                        /*expected_frame_threads=*/0,
                        /*expected_tile_threads=*/{});
    VerifyFrameParallel(thread_count, /*tile_count=*/2, /*tile_columns=*/2,
                        /*expected_frame_threads=*/0,
                        /*expected_tile_threads=*/{});
  }

  // Equal number of tile threads for each frame thread.
  VerifyFrameParallel(
      /*thread_count=*/8, /*tile_count=*/1, /*tile_columns=*/1,
      /*expected_frame_threads=*/4, /*expected_tile_threads=*/{1, 1, 1, 1});
  VerifyFrameParallel(
      /*thread_count=*/12, /*tile_count=*/2, /*tile_columns=*/2,
      /*expected_frame_threads=*/4, /*expected_tile_threads=*/{2, 2, 2, 2});
  VerifyFrameParallel(
      /*thread_count=*/18, /*tile_count=*/2, /*tile_columns=*/2,
      /*expected_frame_threads=*/6,
      /*expected_tile_threads=*/{2, 2, 2, 2, 2, 2});
  VerifyFrameParallel(
      /*thread_count=*/16, /*tile_count=*/3, /*tile_columns=*/3,
      /*expected_frame_threads=*/4, /*expected_tile_threads=*/{3, 3, 3, 3});

  // Unequal number of tile threads for each frame thread.
  VerifyFrameParallel(
      /*thread_count=*/7, /*tile_count=*/1, /*tile_columns=*/1,
      /*expected_frame_threads=*/3, /*expected_tile_threads=*/{2, 1, 1});
  VerifyFrameParallel(
      /*thread_count=*/14, /*tile_count=*/2, /*tile_columns=*/2,
      /*expected_frame_threads=*/4, /*expected_tile_threads=*/{3, 3, 2, 2});
  VerifyFrameParallel(
      /*thread_count=*/20, /*tile_count=*/2, /*tile_columns=*/2,
      /*expected_frame_threads=*/6,
      /*expected_tile_threads=*/{3, 3, 2, 2, 2, 2});
  VerifyFrameParallel(
      /*thread_count=*/17, /*tile_count=*/3, /*tile_columns=*/3,
      /*expected_frame_threads=*/4, /*expected_tile_threads=*/{4, 3, 3, 3});
}

TEST(FrameParallelStrategyTest, ThreadCountDoesNotExceedkMaxThreads) {
  std::unique_ptr<ThreadPool> frame_thread_pool;
  FrameScratchBufferPool frame_scratch_buffer_pool;
  ASSERT_TRUE(InitializeThreadPoolsForFrameParallel(
      /*thread_count=*/kMaxThreads + 10, /*tile_count=*/2, /*tile_columns=*/2,
      &frame_thread_pool, &frame_scratch_buffer_pool));
  EXPECT_NE(frame_thread_pool.get(), nullptr);
  std::vector<std::unique_ptr<FrameScratchBuffer>> frame_scratch_buffers;
  int actual_thread_count = frame_thread_pool->num_threads();
  for (int i = 0; i < frame_thread_pool->num_threads(); ++i) {
    SCOPED_TRACE(absl::StrCat("i: ", i));
    frame_scratch_buffers.push_back(frame_scratch_buffer_pool.Get());
    ThreadPool* const thread_pool =
        frame_scratch_buffers.back()->threading_strategy.thread_pool();
    if (thread_pool != nullptr) {
      actual_thread_count += thread_pool->num_threads();
    }
  }
  // In this case, the exact number of frame threads and tile threads depend on
  // the value of kMaxThreads. So simply ensure that the total number of threads
  // does not exceed kMaxThreads.
  EXPECT_LE(actual_thread_count, kMaxThreads);
  for (auto& frame_scratch_buffer : frame_scratch_buffers) {
    frame_scratch_buffer_pool.Release(std::move(frame_scratch_buffer));
  }
}

}  // namespace
}  // namespace libgav1
