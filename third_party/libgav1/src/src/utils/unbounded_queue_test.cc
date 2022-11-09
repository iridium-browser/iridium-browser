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

#include "src/utils/unbounded_queue.h"

#include <new>
#include <utility>

#include "gtest/gtest.h"

namespace libgav1 {
namespace {

class Integer {
 public:
  explicit Integer(int value) : value_(new (std::nothrow) int{value}) {}

  // Move only.
  Integer(Integer&& other) : value_(other.value_) { other.value_ = nullptr; }
  Integer& operator=(Integer&& other) {
    if (this != &other) {
      delete value_;
      value_ = other.value_;
      other.value_ = nullptr;
    }
    return *this;
  }

  ~Integer() { delete value_; }

  int value() const { return *value_; }

 private:
  int* value_;
};

TEST(UnboundedQueueTest, Basic) {
  UnboundedQueue<int> queue;
  ASSERT_TRUE(queue.Init());
  EXPECT_TRUE(queue.Empty());

  for (int i = 0; i < 8; ++i) {
    EXPECT_TRUE(queue.GrowIfNeeded());
    queue.Push(i);
    EXPECT_FALSE(queue.Empty());
  }

  for (int i = 0; i < 8; ++i) {
    EXPECT_FALSE(queue.Empty());
    EXPECT_EQ(queue.Front(), i);
    queue.Pop();
  }
  EXPECT_TRUE(queue.Empty());
}

TEST(UnboundedQueueTest, WrapAround) {
  UnboundedQueue<int> queue;
  ASSERT_TRUE(queue.Init());
  EXPECT_TRUE(queue.Empty());

  for (int i = 0; i < 1000; ++i) {
    EXPECT_TRUE(queue.GrowIfNeeded());
    queue.Push(i);
    EXPECT_FALSE(queue.Empty());
    EXPECT_EQ(queue.Front(), i);
    queue.Pop();
    EXPECT_TRUE(queue.Empty());
  }
}

TEST(UnboundedQueueTest, EmptyBeforeInit) {
  UnboundedQueue<int> queue;
  EXPECT_TRUE(queue.Empty());
}

TEST(UnboundedQueueTest, LotsOfElements) {
  UnboundedQueue<Integer> queue;
  ASSERT_TRUE(queue.Init());
  EXPECT_TRUE(queue.Empty());

  for (int i = 0; i < 10000; ++i) {
    Integer integer(i);
    EXPECT_EQ(integer.value(), i);
    EXPECT_TRUE(queue.GrowIfNeeded());
    queue.Push(std::move(integer));
    EXPECT_FALSE(queue.Empty());
  }

  for (int i = 0; i < 5000; ++i) {
    EXPECT_FALSE(queue.Empty());
    const Integer& integer = queue.Front();
    EXPECT_EQ(integer.value(), i);
    queue.Pop();
  }
  // Leave some elements in the queue to test destroying a nonempty queue.
  EXPECT_FALSE(queue.Empty());
}

// Copy constructor and assignment are deleted, but move constructor and
// assignment are OK.
TEST(UnboundedQueueTest, Move) {
  UnboundedQueue<int> ints1;
  ASSERT_TRUE(ints1.Init());
  EXPECT_TRUE(ints1.GrowIfNeeded());
  ints1.Push(2);
  EXPECT_TRUE(ints1.GrowIfNeeded());
  ints1.Push(3);
  EXPECT_TRUE(ints1.GrowIfNeeded());
  ints1.Push(5);
  EXPECT_TRUE(ints1.GrowIfNeeded());
  ints1.Push(7);

  // Move constructor.
  UnboundedQueue<int> ints2(std::move(ints1));
  EXPECT_EQ(ints2.Front(), 2);
  ints2.Pop();
  EXPECT_EQ(ints2.Front(), 3);
  ints2.Pop();
  EXPECT_EQ(ints2.Front(), 5);
  ints2.Pop();
  EXPECT_EQ(ints2.Front(), 7);
  ints2.Pop();
  EXPECT_TRUE(ints2.Empty());

  EXPECT_TRUE(ints2.GrowIfNeeded());
  ints2.Push(11);
  EXPECT_TRUE(ints2.GrowIfNeeded());
  ints2.Push(13);
  EXPECT_TRUE(ints2.GrowIfNeeded());
  ints2.Push(17);
  EXPECT_TRUE(ints2.GrowIfNeeded());
  ints2.Push(19);

  // Move assignment.
  UnboundedQueue<int> ints3;
  ASSERT_TRUE(ints3.Init());
  EXPECT_TRUE(ints3.GrowIfNeeded());
  ints3.Push(23);
  ints3 = std::move(ints2);
  EXPECT_EQ(ints3.Front(), 11);
  ints3.Pop();
  EXPECT_EQ(ints3.Front(), 13);
  ints3.Pop();
  EXPECT_EQ(ints3.Front(), 17);
  ints3.Pop();
  EXPECT_EQ(ints3.Front(), 19);
  ints3.Pop();
  EXPECT_TRUE(ints3.Empty());
}

}  // namespace
}  // namespace libgav1
