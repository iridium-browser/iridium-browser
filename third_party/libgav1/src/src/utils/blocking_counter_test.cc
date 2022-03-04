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

#include "src/utils/blocking_counter.h"

#include <array>
#include <memory>

#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "gtest/gtest.h"
#include "src/utils/threadpool.h"

namespace libgav1 {
namespace {

constexpr int kNumWorkers = 10;
constexpr int kNumJobs = 20;

TEST(BlockingCounterTest, BasicFunctionality) {
  std::unique_ptr<ThreadPool> pool = ThreadPool::Create(kNumWorkers);
  BlockingCounter counter(kNumJobs);
  std::array<bool, kNumJobs> done = {};

  // Schedule the jobs.
  for (int i = 0; i < kNumJobs; ++i) {
    pool->Schedule([&counter, &done, i]() {
      absl::SleepFor(absl::Seconds(1));
      done[i] = true;
      counter.Decrement();
    });
  }

  // Wait for the jobs to complete. This should always return true.
  ASSERT_TRUE(counter.Wait());

  // Make sure the jobs were actually complete.
  for (const auto& job_done : done) {
    EXPECT_TRUE(job_done);
  }
}

TEST(BlockingCounterTest, IncrementBy) {
  std::unique_ptr<ThreadPool> pool = ThreadPool::Create(kNumWorkers);
  BlockingCounter counter(0);
  std::array<bool, kNumJobs> done = {};

  // Schedule the jobs.
  for (int i = 0; i < kNumJobs; ++i) {
    counter.IncrementBy(1);
    pool->Schedule([&counter, &done, i]() {
      absl::SleepFor(absl::Seconds(1));
      done[i] = true;
      counter.Decrement();
    });
  }

  // Wait for the jobs to complete. This should always return true.
  ASSERT_TRUE(counter.Wait());

  // Make sure the jobs were actually complete.
  for (const auto& job_done : done) {
    EXPECT_TRUE(job_done);
  }
}

TEST(BlockingCounterWithStatusTest, BasicFunctionality) {
  std::unique_ptr<ThreadPool> pool = ThreadPool::Create(kNumWorkers);
  BlockingCounterWithStatus counter(kNumJobs);
  std::array<bool, kNumJobs> done = {};

  // Schedule the jobs.
  for (int i = 0; i < kNumJobs; ++i) {
    pool->Schedule([&counter, &done, i]() {
      absl::SleepFor(absl::Seconds(1));
      done[i] = true;
      counter.Decrement(true);
    });
  }

  // Wait for the jobs to complete. This should return true since all the jobs
  // reported |job_succeeded| as true.
  ASSERT_TRUE(counter.Wait());

  // Make sure the jobs were actually complete.
  for (const auto& job_done : done) {
    EXPECT_TRUE(job_done);
  }
}

TEST(BlockingCounterWithStatusTest, BasicFunctionalityWithStatus) {
  std::unique_ptr<ThreadPool> pool = ThreadPool::Create(kNumWorkers);
  BlockingCounterWithStatus counter(kNumJobs);
  std::array<bool, kNumJobs> done = {};

  // Schedule the jobs.
  for (int i = 0; i < kNumJobs; ++i) {
    pool->Schedule([&counter, &done, i]() {
      absl::SleepFor(absl::Seconds(1));
      done[i] = true;
      counter.Decrement(i != 10);
    });
  }

  // Wait for the jobs to complete. This should return false since one of the
  // jobs reported |job_succeeded| as false.
  ASSERT_FALSE(counter.Wait());

  // Make sure the jobs were actually complete.
  for (const auto& job_done : done) {
    EXPECT_TRUE(job_done);
  }
}

}  // namespace
}  // namespace libgav1
