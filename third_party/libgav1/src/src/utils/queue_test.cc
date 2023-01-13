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

#include "src/utils/queue.h"

#include <utility>
#include <vector>

#include "gtest/gtest.h"

namespace libgav1 {
namespace {

struct TestClass {
  TestClass() = default;
  explicit TestClass(int i) : i(i) {}
  int i;
  // The vector exists simply so that the class is not trivially copyable.
  std::vector<int> dummy;
};

TEST(QueueTest, Basic) {
  Queue<TestClass> queue;
  ASSERT_TRUE(queue.Init(8));
  EXPECT_TRUE(queue.Empty());

  for (int i = 0; i < 8; ++i) {
    EXPECT_FALSE(queue.Full());
    TestClass test(i);
    queue.Push(std::move(test));
    EXPECT_EQ(queue.Back().i, i);
    EXPECT_FALSE(queue.Empty());
  }
  EXPECT_TRUE(queue.Full());

  for (int i = 0; i < 8; ++i) {
    EXPECT_FALSE(queue.Empty());
    EXPECT_EQ(queue.Front().i, i);
    queue.Pop();
    EXPECT_FALSE(queue.Full());
  }
  EXPECT_TRUE(queue.Empty());

  for (int i = 0; i < 8; ++i) {
    EXPECT_FALSE(queue.Full());
    TestClass test(i);
    queue.Push(std::move(test));
    EXPECT_EQ(queue.Back().i, i);
    EXPECT_FALSE(queue.Empty());
  }
  EXPECT_TRUE(queue.Full());
  queue.Clear();
  EXPECT_TRUE(queue.Empty());
  EXPECT_FALSE(queue.Full());
}

TEST(QueueTest, WrapAround) {
  Queue<TestClass> queue;
  ASSERT_TRUE(queue.Init(8));
  EXPECT_TRUE(queue.Empty());

  for (int i = 0; i < 100; ++i) {
    EXPECT_FALSE(queue.Full());
    TestClass test(i);
    queue.Push(std::move(test));
    EXPECT_EQ(queue.Back().i, i);
    EXPECT_FALSE(queue.Empty());
    EXPECT_EQ(queue.Front().i, i);
    queue.Pop();
    EXPECT_TRUE(queue.Empty());
  }
}

}  // namespace
}  // namespace libgav1
