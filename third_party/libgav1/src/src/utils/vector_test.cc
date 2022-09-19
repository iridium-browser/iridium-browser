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

#include "src/utils/vector.h"

#include <memory>
#include <new>
#include <utility>

#include "gtest/gtest.h"
#include "src/utils/compiler_attributes.h"

#if LIBGAV1_MSAN
#include <sanitizer/msan_interface.h>
#endif

namespace libgav1 {
namespace {

class Foo {
 public:
  Foo() = default;

  int x() const { return x_; }

 private:
  int x_ = 38;
};

class Point {
 public:
  Point(int x, int y) : x_(x), y_(y) {}

  int x() const { return x_; }
  int y() const { return y_; }

 private:
  int x_;
  int y_;
};

TEST(VectorTest, NoCtor) {
  VectorNoCtor<int> v;
  EXPECT_TRUE(v.resize(100));
  Vector<int> w;
  EXPECT_TRUE(w.resize(100));

#if LIBGAV1_MSAN
  // Use MemorySanitizer to check VectorNoCtor::resize() does not initialize
  // the memory while Vector::resize() does.
  //
  // __msan_test_shadow(const void *x, uptr size) returns the offset of the
  // first (at least partially) poisoned byte in the range, or -1 if the whole
  // range is good.
  for (size_t i = 0; i < 100; ++i) {
    EXPECT_EQ(__msan_test_shadow(&v[i], sizeof(int)), 0);
    EXPECT_EQ(__msan_test_shadow(&w[i], sizeof(int)), -1);
  }
#endif
}

TEST(VectorTest, Constructor) {
  Vector<Foo> v;
  EXPECT_TRUE(v.resize(100));
  for (const Foo& foo : v) {
    EXPECT_EQ(foo.x(), 38);
  }
}

TEST(VectorTest, PushBack) {
  // Create a vector containing integers
  Vector<int> v;
  EXPECT_TRUE(v.reserve(8));
  EXPECT_EQ(v.size(), 0);

  EXPECT_TRUE(v.push_back(25));
  EXPECT_EQ(v.size(), 1);
  EXPECT_EQ(v[0], 25);

  EXPECT_TRUE(v.push_back(13));
  EXPECT_EQ(v.size(), 2);
  EXPECT_EQ(v[0], 25);
  EXPECT_EQ(v[1], 13);
}

TEST(VectorTest, PushBackUnchecked) {
  Vector<std::unique_ptr<Point>> v;
  EXPECT_TRUE(v.reserve(2));
  EXPECT_EQ(v.size(), 0);

  std::unique_ptr<Point> point(new (std::nothrow) Point(1, 2));
  EXPECT_NE(point, nullptr);
  v.push_back_unchecked(std::move(point));
  EXPECT_EQ(v.size(), 1);
  EXPECT_EQ(v[0]->x(), 1);
  EXPECT_EQ(v[0]->y(), 2);

  point.reset(new (std::nothrow) Point(3, 4));
  EXPECT_NE(point, nullptr);
  v.push_back_unchecked(std::move(point));
  EXPECT_EQ(v.size(), 2);
  EXPECT_EQ(v[0]->x(), 1);
  EXPECT_EQ(v[0]->y(), 2);
  EXPECT_EQ(v[1]->x(), 3);
  EXPECT_EQ(v[1]->y(), 4);
}

TEST(VectorTest, EmplaceBack) {
  Vector<Point> v;
  EXPECT_EQ(v.size(), 0);

  EXPECT_TRUE(v.emplace_back(1, 2));
  EXPECT_EQ(v.size(), 1);
  EXPECT_EQ(v[0].x(), 1);
  EXPECT_EQ(v[0].y(), 2);

  EXPECT_TRUE(v.emplace_back(3, 4));
  EXPECT_EQ(v.size(), 2);
  EXPECT_EQ(v[0].x(), 1);
  EXPECT_EQ(v[0].y(), 2);
  EXPECT_EQ(v[1].x(), 3);
  EXPECT_EQ(v[1].y(), 4);
}

// Copy constructor and assignment are deleted, but move constructor and
// assignment are OK.
TEST(VectorTest, Move) {
  Vector<int> ints1;
  EXPECT_TRUE(ints1.reserve(4));
  EXPECT_TRUE(ints1.push_back(2));
  EXPECT_TRUE(ints1.push_back(3));
  EXPECT_TRUE(ints1.push_back(5));
  EXPECT_TRUE(ints1.push_back(7));

  // Move constructor.
  Vector<int> ints2(std::move(ints1));
  EXPECT_EQ(ints2.size(), 4);
  EXPECT_EQ(ints2[0], 2);
  EXPECT_EQ(ints2[1], 3);
  EXPECT_EQ(ints2[2], 5);
  EXPECT_EQ(ints2[3], 7);

  // Move assignment.
  Vector<int> ints3;
  EXPECT_TRUE(ints3.reserve(1));
  EXPECT_TRUE(ints3.push_back(11));
  ints3 = std::move(ints2);
  EXPECT_EQ(ints3.size(), 4);
  EXPECT_EQ(ints3[0], 2);
  EXPECT_EQ(ints3[1], 3);
  EXPECT_EQ(ints3[2], 5);
  EXPECT_EQ(ints3[3], 7);
}

TEST(VectorTest, Erase) {
  Vector<int> ints;
  EXPECT_TRUE(ints.reserve(4));
  EXPECT_TRUE(ints.push_back(2));
  EXPECT_TRUE(ints.push_back(3));
  EXPECT_TRUE(ints.push_back(5));
  EXPECT_TRUE(ints.push_back(7));

  EXPECT_EQ(ints.size(), 4);
  EXPECT_EQ(ints[0], 2);
  EXPECT_EQ(ints[1], 3);
  EXPECT_EQ(ints[2], 5);
  EXPECT_EQ(ints[3], 7);

  ints.erase(ints.begin());
  EXPECT_EQ(ints.size(), 3);
  EXPECT_EQ(ints[0], 3);
  EXPECT_EQ(ints[1], 5);
  EXPECT_EQ(ints[2], 7);
}

TEST(VectorTest, EraseNonTrivial) {
  // A simple class that sets an int value to 0 in the destructor.
  class Cleaner {
   public:
    explicit Cleaner(int* value) : value_(value) {}
    ~Cleaner() { *value_ = 0; }

    int value() const { return *value_; }

   private:
    int* value_;
  };
  int value1 = 100;
  int value2 = 200;
  Vector<std::unique_ptr<Cleaner>> v;
  EXPECT_TRUE(v.reserve(2));
  EXPECT_EQ(v.capacity(), 2);

  std::unique_ptr<Cleaner> c(new (std::nothrow) Cleaner(&value1));
  EXPECT_NE(c, nullptr);
  EXPECT_TRUE(v.push_back(std::move(c)));
  c.reset(new (std::nothrow) Cleaner(&value2));
  EXPECT_NE(c, nullptr);
  EXPECT_TRUE(v.push_back(std::move(c)));
  EXPECT_EQ(v.size(), 2);
  EXPECT_EQ(value1, 100);
  EXPECT_EQ(value2, 200);

  v.erase(v.begin());
  EXPECT_EQ(v.size(), 1);
  EXPECT_EQ(v.capacity(), 2);
  EXPECT_EQ(value1, 0);
  EXPECT_EQ(value2, 200);
  EXPECT_EQ(v[0].get()->value(), value2);

  EXPECT_TRUE(v.shrink_to_fit());
  EXPECT_EQ(v.size(), 1);
  EXPECT_EQ(v.capacity(), 1);
  EXPECT_EQ(value2, 200);
  EXPECT_EQ(v[0].get()->value(), value2);

  v.clear();
  EXPECT_TRUE(v.empty());
  EXPECT_EQ(value2, 0);
}

}  // namespace
}  // namespace libgav1
