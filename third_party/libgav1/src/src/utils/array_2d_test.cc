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

#include "src/utils/array_2d.h"

#include <cstdint>
#include <memory>
#include <new>
#include <type_traits>

#include "gtest/gtest.h"
#include "src/utils/compiler_attributes.h"

#if LIBGAV1_MSAN
#include <sanitizer/msan_interface.h>
#endif

namespace libgav1 {
namespace {

constexpr int kRows = 50;
constexpr int kColumns = 200;

TEST(Array2dViewTest, TestUint8) {
  uint8_t data[kRows * kColumns] = {};
  Array2DView<uint8_t> data2d(kRows, kColumns, data);

  // Verify data.
  data[kColumns] = 100;
  data[kColumns + 1] = 101;
  data[kColumns * 2 + 10] = 210;
  data[kColumns * 2 + 40] = 240;
  EXPECT_EQ(data2d[1][0], 100);
  EXPECT_EQ(data2d[1][1], 101);
  EXPECT_EQ(data2d[2][10], 210);
  EXPECT_EQ(data2d[2][40], 240);

  // Verify pointers.
  EXPECT_EQ(data2d[10], data + 10 * kColumns);
}

TEST(Array2dViewTest, TestUint16) {
  uint16_t data[kRows * kColumns] = {};
  Array2DView<uint16_t> data2d(kRows, kColumns, data);

  // Verify data.
  data[kColumns] = 100;
  data[kColumns + 1] = 101;
  data[kColumns * 2 + 10] = 210;
  data[kColumns * 2 + 40] = 240;
  EXPECT_EQ(data2d[1][0], 100);
  EXPECT_EQ(data2d[1][1], 101);
  EXPECT_EQ(data2d[2][10], 210);
  EXPECT_EQ(data2d[2][40], 240);

  // Verify pointers.
  EXPECT_EQ(data2d[10], data + 10 * kColumns);
}

TEST(Array2dViewTest, TestUint8Const) {
  uint8_t data[kRows * kColumns] = {};
  // Declared as const to provide a read-only view of |data|.
  const Array2DView<uint8_t> data2d(kRows, kColumns, data);

  // Verify data.
  data[kColumns] = 100;
  data[kColumns + 1] = 101;
  data[kColumns * 2 + 10] = 210;
  data[kColumns * 2 + 40] = 240;
  EXPECT_EQ(data2d[1][0], 100);
  EXPECT_EQ(data2d[1][1], 101);
  EXPECT_EQ(data2d[2][10], 210);
  EXPECT_EQ(data2d[2][40], 240);

  // Verify pointers.
  EXPECT_EQ(data2d[10], data + 10 * kColumns);
}

TEST(Array2dTest, TestUint8) {
  Array2D<uint8_t> data2d;
  ASSERT_TRUE(data2d.Reset(kRows, kColumns, true));

  EXPECT_EQ(data2d.rows(), kRows);
  EXPECT_EQ(data2d.columns(), kColumns);

  // Verify pointers.
  for (int i = 0; i < kRows; ++i) {
    EXPECT_NE(data2d[i], nullptr);
  }

  // Verify data (must be zero initialized).
  for (int i = 0; i < kRows; ++i) {
    for (int j = 0; j < kColumns; ++j) {
      EXPECT_EQ(data2d[i][j], 0) << "Mismatch in [" << i << "][" << j << "]";
    }
  }

  // Reset to a 2d array of smaller size with zero_initialize == false.
  data2d[0][0] = 10;
  ASSERT_TRUE(data2d.Reset(kRows - 1, kColumns - 1, false));

  EXPECT_EQ(data2d.rows(), kRows - 1);
  EXPECT_EQ(data2d.columns(), kColumns - 1);

  // Verify pointers.
  for (int i = 0; i < kRows - 1; ++i) {
    EXPECT_NE(data2d[i], nullptr);
  }

  // Verify data (must be zero except for 0,0 because it was zero initialized in
  // the previous call to Reset).
  for (int i = 0; i < kRows - 1; ++i) {
    for (int j = 0; j < kColumns - 1; ++j) {
      if (i == 0 && j == 0) {
        EXPECT_EQ(data2d[i][j], 10) << "Mismatch in [" << i << "][" << j << "]";
      } else {
        EXPECT_EQ(data2d[i][j], 0) << "Mismatch in [" << i << "][" << j << "]";
      }
    }
  }

  // Reset to a 2d array of smaller size with zero_initialize == true.
  ASSERT_TRUE(data2d.Reset(kRows - 2, kColumns - 2, true));

  EXPECT_EQ(data2d.rows(), kRows - 2);
  EXPECT_EQ(data2d.columns(), kColumns - 2);

  // Verify pointers.
  for (int i = 0; i < kRows - 2; ++i) {
    EXPECT_NE(data2d[i], nullptr);
  }

  // Verify data (must be zero initialized).
  for (int i = 0; i < kRows - 2; ++i) {
    for (int j = 0; j < kColumns - 2; ++j) {
      EXPECT_EQ(data2d[i][j], 0) << "Mismatch in [" << i << "][" << j << "]";
    }
  }
}

TEST(Array2dTest, TestUniquePtr1) {
  // A simple class that sets an int value to 0 in the destructor.
  class Cleaner {
   public:
    explicit Cleaner(int* value) : value_(value) {}
    ~Cleaner() { *value_ = 0; }

   private:
    int* value_;
  };
  int value = 100;
  Array2D<std::unique_ptr<Cleaner>> data2d;
  ASSERT_TRUE(data2d.Reset(4, 4, true));
  data2d[0][0].reset(new (std::nothrow) Cleaner(&value));
  EXPECT_EQ(value, 100);
  // Reset to a smaller size. Depending on the implementation, the data_ buffer
  // may or may not be reused.
  ASSERT_TRUE(data2d.Reset(2, 2, true));
  // Reset to a much larger size. The data_ buffer will be reallocated.
  ASSERT_TRUE(data2d.Reset(32, 32, true));
  // The destructors of all elements in the former data_ buffer should have
  // been invoked.
  EXPECT_EQ(value, 0);
}

TEST(Array2dTest, TestUniquePtr2) {
  // A simple class that sets an int value to 0 in the destructor.
  class Cleaner {
   public:
    explicit Cleaner(int* value) : value_(value) {}
    ~Cleaner() { *value_ = 0; }

   private:
    int* value_;
  };
  int value1 = 100;
  int value2 = 200;
  Array2D<std::unique_ptr<Cleaner>> data2d;
  ASSERT_TRUE(data2d.Reset(4, 4, false));
  data2d[0][0].reset(new (std::nothrow) Cleaner(&value1));
  data2d[3][3].reset(new (std::nothrow) Cleaner(&value2));
  EXPECT_EQ(value1, 100);
  EXPECT_EQ(value2, 200);
  // Reset to a smaller size. Whether or not the data_ buffer is reused, the
  // destructors of all existing elements should be invoked.
  ASSERT_TRUE(data2d.Reset(2, 2, false));
  EXPECT_EQ(value1, 0);
  EXPECT_EQ(value2, 0);
}

// Shows that std::is_standard_layout is not relevant to the default
// initialization vs. value initialization issue, but std::is_trivial is.
TEST(Array2dTest, TestStructInit) {
  // Make one data member private so that this struct does not have a standard
  // layout. This also makes the struct not a POD type.
  struct Point {
    int x;
    int Y() const { return y; }

   private:
    int y;
  };

  EXPECT_TRUE(std::is_trivial<Point>::value);
  EXPECT_FALSE(std::is_standard_layout<Point>::value);

  // The Point structs in this array are default initialized.
  Array2D<Point> data2d_default_init;
  ASSERT_TRUE(data2d_default_init.Reset(kRows, kColumns, false));
  // The Point structs in this array are value initialized (i.e., zero
  // initialized).
  Array2D<Point> data2d;
  ASSERT_TRUE(data2d.Reset(kRows, kColumns, true));

#if LIBGAV1_MSAN
  // Use MemorySanitizer to check Reset(rows, columns, false) does not
  // initialize the memory while Reset(rows, columns, true) does.
  //
  // __msan_test_shadow(const void *x, uptr size) returns the offset of the
  // first (at least partially) poisoned byte in the range, or -1 if the whole
  // range is good.
  for (int i = 0; i < kRows; ++i) {
    EXPECT_EQ(__msan_test_shadow(data2d_default_init[i],
                                 sizeof(data2d_default_init[0][0]) * kColumns),
              0);
    EXPECT_EQ(__msan_test_shadow(data2d[i], sizeof(data2d[0][0]) * kColumns),
              -1);
    for (int j = 0; j < kColumns; ++j) {
      EXPECT_EQ(data2d[i][j].x, 0);
      EXPECT_EQ(data2d[i][j].Y(), 0);
    }
  }
#endif
}

}  // namespace
}  // namespace libgav1
