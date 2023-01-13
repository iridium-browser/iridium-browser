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

#include "src/utils/threadpool.h"

#include <cassert>
#include <cstdint>
#include <memory>

#include "absl/synchronization/mutex.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "gtest/gtest.h"
#include "src/utils/compiler_attributes.h"
#include "src/utils/executor.h"

namespace libgav1 {
namespace {

class SimpleGuardedInteger {
 public:
  explicit SimpleGuardedInteger(int initial_value) : value_(initial_value) {}
  SimpleGuardedInteger(const SimpleGuardedInteger&) = delete;
  SimpleGuardedInteger& operator=(const SimpleGuardedInteger&) = delete;

  void Decrement() {
    absl::MutexLock l(&mutex_);
    assert(value_ >= 1);
    --value_;
    changed_.SignalAll();
  }

  void Increment() {
    absl::MutexLock l(&mutex_);
    ++value_;
    changed_.SignalAll();
  }

  int Value() {
    absl::MutexLock l(&mutex_);
    return value_;
  }

  void WaitForZero() {
    absl::MutexLock l(&mutex_);
    while (value_ != 0) {
      changed_.Wait(&mutex_);
    }
  }

 private:
  absl::Mutex mutex_;
  absl::CondVar changed_;
  int value_ LIBGAV1_GUARDED_BY(mutex_);
};

// Loops for |milliseconds| of wall-clock time.
void LoopForMs(int64_t milliseconds) {
  const absl::Time deadline = absl::Now() + absl::Milliseconds(milliseconds);
  while (absl::Now() < deadline) {
  }
}

// A function that increments the given integer.
void IncrementIntegerJob(SimpleGuardedInteger* value) {
  LoopForMs(100);
  value->Increment();
}

TEST(ThreadPoolTest, ThreadedIntegerIncrement) {
  std::unique_ptr<ThreadPool> thread_pool = ThreadPool::Create(100);
  ASSERT_NE(thread_pool, nullptr);
  EXPECT_EQ(thread_pool->num_threads(), 100);
  SimpleGuardedInteger count(0);
  for (int i = 0; i < 1000; ++i) {
    thread_pool->Schedule([&count]() { IncrementIntegerJob(&count); });
  }
  thread_pool.reset(nullptr);
  EXPECT_EQ(count.Value(), 1000);
}

// Test a ThreadPool via the Executor interface.
TEST(ThreadPoolTest, ExecutorInterface) {
  std::unique_ptr<ThreadPool> thread_pool = ThreadPool::Create(100);
  ASSERT_NE(thread_pool, nullptr);
  std::unique_ptr<Executor> executor(thread_pool.release());
  SimpleGuardedInteger count(0);
  for (int i = 0; i < 1000; ++i) {
    executor->Schedule([&count]() { IncrementIntegerJob(&count); });
  }
  executor.reset(nullptr);
  EXPECT_EQ(count.Value(), 1000);
}

TEST(ThreadPoolTest, DestroyWithoutUse) {
  std::unique_ptr<ThreadPool> thread_pool = ThreadPool::Create(100);
  EXPECT_NE(thread_pool, nullptr);
  thread_pool.reset(nullptr);
}

// If num_threads is 0, ThreadPool::Create() should return a null pointer.
TEST(ThreadPoolTest, NumThreadsZero) {
  std::unique_ptr<ThreadPool> thread_pool = ThreadPool::Create(0);
  EXPECT_EQ(thread_pool, nullptr);
}

// If num_threads is 1, the closures are run in FIFO order.
TEST(ThreadPoolTest, OneThreadRunsClosuresFIFO) {
  int count = 0;  // Declare first so that it outlives the thread pool.
  std::unique_ptr<ThreadPool> pool = ThreadPool::Create(1);
  ASSERT_NE(pool, nullptr);
  EXPECT_EQ(pool->num_threads(), 1);
  for (int i = 0; i < 1000; ++i) {
    pool->Schedule([&count, i]() {
      EXPECT_EQ(count, i);
      count++;
    });
  }
}

}  // namespace
}  // namespace libgav1
