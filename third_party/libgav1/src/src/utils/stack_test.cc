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

#include "src/utils/stack.h"

#include <cstdint>
#include <utility>

#include "gtest/gtest.h"

namespace libgav1 {
namespace {

constexpr int kStackSize = 8;

TEST(StackTest, SimpleType) {
  Stack<int, kStackSize> stack;
  EXPECT_TRUE(stack.Empty());

  for (int i = 0; i < kStackSize; ++i) {
    stack.Push(i);
    EXPECT_FALSE(stack.Empty());
  }

  for (int i = kStackSize - 1; i >= 0; --i) {
    EXPECT_EQ(stack.Pop(), i);
  }
  EXPECT_TRUE(stack.Empty());
}

TEST(StackTest, LargeStruct) {
  struct LargeMoveOnlyStruct {
    LargeMoveOnlyStruct() = default;
    // Move only.
    LargeMoveOnlyStruct(LargeMoveOnlyStruct&& other) = default;
    LargeMoveOnlyStruct& operator=(LargeMoveOnlyStruct&& other) = default;

    int32_t array1[1000];
    uint64_t array2[2000];
  };

  Stack<LargeMoveOnlyStruct, kStackSize> stack;
  EXPECT_TRUE(stack.Empty());

  LargeMoveOnlyStruct large_move_only_struct[kStackSize];
  for (int i = 0; i < kStackSize; ++i) {
    LargeMoveOnlyStruct& l = large_move_only_struct[i];
    l.array1[0] = i;
    l.array2[0] = i;
    stack.Push(std::move(l));
    EXPECT_FALSE(stack.Empty());
  }

  for (int i = kStackSize - 1; i >= 0; --i) {
    LargeMoveOnlyStruct l = stack.Pop();
    EXPECT_EQ(l.array1[0], i);
    EXPECT_EQ(l.array2[0], i);
  }
  EXPECT_TRUE(stack.Empty());
}

}  // namespace
}  // namespace libgav1
