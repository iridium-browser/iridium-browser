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

#include "src/utils/common.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "absl/base/macros.h"
#include "gtest/gtest.h"
#include "src/utils/constants.h"
#include "src/utils/memory.h"
#include "src/utils/types.h"

namespace libgav1 {
namespace {

int BitLength(int64_t n) {
  int count = 0;
  while (n != 0) {
    ++count;
    n >>= 1;
  }
  return count;
}

TEST(CommonUtilsTest, Align) {
  for (int i = 0; i <= 8; ++i) {
    const int alignment = 1 << i;
    SCOPED_TRACE("alignment: " + std::to_string(alignment));
    EXPECT_EQ(Align(0, alignment), 0);
    EXPECT_EQ(Align(1, alignment), alignment);
    EXPECT_EQ(Align(alignment + 1, alignment), 2 * alignment);
    if (i > 1) {
      EXPECT_EQ(Align(alignment - 1, alignment), alignment);
      EXPECT_EQ(Align(2 * alignment - 1, alignment), 2 * alignment);
    }
  }
}

TEST(CommonUtilsTest, AlignAddr) {
  auto buf = MakeAlignedUniquePtr<uint8_t>(/*alignment=*/1024, 512);
  ASSERT_NE(buf, nullptr);
  auto* const bufptr = buf.get();
  ASSERT_EQ(reinterpret_cast<uintptr_t>(bufptr) % 1024, 0);

  for (int i = 0; i <= 8; ++i) {
    const int alignment = 1 << i;
    ASSERT_LE(alignment, 1024);
    SCOPED_TRACE("alignment: " + std::to_string(alignment));
    EXPECT_EQ(AlignAddr(nullptr, alignment), nullptr);
    EXPECT_EQ(AlignAddr(bufptr, alignment), bufptr);
    EXPECT_EQ(AlignAddr(bufptr + 1, alignment), bufptr + alignment);
    EXPECT_EQ(AlignAddr(bufptr + alignment + 1, alignment),
              bufptr + 2 * alignment);
    if (i > 1) {
      EXPECT_EQ(AlignAddr(bufptr + alignment - 1, alignment),
                bufptr + alignment);
      EXPECT_EQ(AlignAddr(bufptr + 2 * alignment - 1, alignment),
                bufptr + 2 * alignment);
    }
  }
}

TEST(CommonUtilsTest, Clip3) {
  // Value <= lower boundary.
  EXPECT_EQ(Clip3(10, 20, 30), 20);
  EXPECT_EQ(Clip3(20, 20, 30), 20);
  // Value >= higher boundary.
  EXPECT_EQ(Clip3(40, 20, 30), 30);
  EXPECT_EQ(Clip3(30, 20, 30), 30);
  // Value within boundary.
  EXPECT_EQ(Clip3(25, 20, 30), 25);
  // Clipping based on bitdepth (clamp between 0 and 2^bitdepth - 1). Make sure
  // that the resulting values are always in the pixel range for the
  // corresponding bitdepth.
  static constexpr int bitdepths[] = {8, 10, 12};
  static constexpr int pixels[] = {100, 500, 5000, -100, -500, -5000};
  for (const auto& bitdepth : bitdepths) {
    for (const auto& pixel : pixels) {
      const int clipped_pixel = Clip3(pixel, 0, (1 << bitdepth) - 1);
      EXPECT_GE(clipped_pixel, 0)
          << "Clip3 mismatch for bitdepth: " << bitdepth << " pixel: " << pixel;
      EXPECT_LE(clipped_pixel, (1 << bitdepth) - 1)
          << "Clip3 mismatch for bitdepth: " << bitdepth << " pixel: " << pixel;
    }
  }
}

template <typename Pixel>
void TestExtendLine(int width, const int left, int right, Pixel left_value,
                    Pixel right_value) {
  constexpr int size = 1000;
  ASSERT_LE(width + left + right, size);
  Pixel line[size];
  Pixel* line_start = line + left;
  line_start[0] = left_value;
  line_start[width - 1] = right_value;
  ExtendLine<Pixel>(line_start, width, left, right);
  for (int x = 0; x < left; x++) {
    EXPECT_EQ(left_value, line[x]) << "Left side mismatch at x: " << x;
  }
  for (int x = 0; x < right; x++) {
    EXPECT_EQ(right_value, line[left + width + x])
        << "Right side mismatch at x: " << x;
  }
}

TEST(CommonUtilsTest, ExtendLine) {
  TestExtendLine<uint8_t>(300, 0, 0, 31, 13);
  TestExtendLine<uint8_t>(100, 10, 20, 31, 13);
  TestExtendLine<uint8_t>(257, 31, 77, 59, 255);
  TestExtendLine<uint16_t>(600, 0, 0, 1234, 4321);
  TestExtendLine<uint16_t>(200, 55, 88, 12345, 54321);
  TestExtendLine<uint16_t>(2, 99, 333, 257, 513);
}

template <typename T>
void TestMemSetBlock(int rows, int columns, ptrdiff_t stride, T value) {
  constexpr int size = 1000;
  T block[size];
  static_assert(sizeof(T) == 1, "");
  ASSERT_LE(rows * stride, size);
  ASSERT_LE(columns, stride);
  MemSetBlock<T>(rows, columns, value, block, stride);
  for (int y = 0; y < rows; y++) {
    for (int x = 0; x < columns; x++) {
      EXPECT_EQ(value, block[y * stride + x])
          << "Mismatch at y: " << y << " x: " << x;
    }
  }
}

TEST(CommonUtilsTest, MemSetBlock) {
  TestMemSetBlock<bool>(15, 28, 29, true);
  TestMemSetBlock<bool>(17, 1, 24, false);
  TestMemSetBlock<bool>(7, 2, 13, true);
  TestMemSetBlock<int8_t>(35, 17, 19, 123);
  TestMemSetBlock<uint8_t>(19, 16, 16, 234);
}

template <typename T>
void TestSetBlock(int rows, int columns, ptrdiff_t stride, T value) {
  constexpr int size = 1000;
  T block[size];
  ASSERT_LE(rows * stride, size);
  ASSERT_LE(columns, stride);
  SetBlock<T>(rows, columns, value, block, stride);
  for (int y = 0; y < rows; y++) {
    for (int x = 0; x < columns; x++) {
      EXPECT_EQ(value, block[y * stride + x])
          << "Mismatch at y: " << y << " x: " << x;
    }
  }
}

TEST(CommonUtilsTest, SetBlock) {
  // Test 1-byte block set.
  TestSetBlock<bool>(15, 28, 29, true);
  TestSetBlock<bool>(17, 1, 24, false);
  TestSetBlock<bool>(7, 2, 13, true);
  TestSetBlock<int8_t>(35, 17, 19, 123);
  TestSetBlock<uint8_t>(19, 16, 16, 234);
  // Test 2-byte block set.
  TestSetBlock<int16_t>(23, 27, 28, 1234);
  TestSetBlock<uint16_t>(13, 39, 44, 4321);
  // Test 4-byte block set.
  TestSetBlock<int>(14, 7, 7, 12345);
  TestSetBlock<int>(33, 4, 15, 54321);
  // Test pointer block set.
  int data;
  TestSetBlock<int*>(23, 8, 25, &data);
}

TEST(CommonUtilsTest, CountTrailingZeros) {
  EXPECT_EQ(CountTrailingZeros(0x1), 0);
  EXPECT_EQ(CountTrailingZeros(0x3), 0);
  EXPECT_EQ(CountTrailingZeros(0x7), 0);
  EXPECT_EQ(CountTrailingZeros(0xF), 0);
  EXPECT_EQ(CountTrailingZeros(0x2), 1);
  EXPECT_EQ(CountTrailingZeros(0x6), 1);
  EXPECT_EQ(CountTrailingZeros(0xE), 1);
  EXPECT_EQ(CountTrailingZeros(0x4), 2);
  EXPECT_EQ(CountTrailingZeros(0xC), 2);
  EXPECT_EQ(CountTrailingZeros(0x8), 3);
  EXPECT_EQ(CountTrailingZeros(0x10), 4);
  EXPECT_EQ(CountTrailingZeros(0x30), 4);
  EXPECT_EQ(CountTrailingZeros(0x70), 4);
  EXPECT_EQ(CountTrailingZeros(0xF0), 4);
  EXPECT_EQ(CountTrailingZeros(0x20), 5);
  EXPECT_EQ(CountTrailingZeros(0x60), 5);
  EXPECT_EQ(CountTrailingZeros(0xE0), 5);
  EXPECT_EQ(CountTrailingZeros(0x40), 6);
  EXPECT_EQ(CountTrailingZeros(0xC0), 6);
  EXPECT_EQ(CountTrailingZeros(0x80), 7);
  EXPECT_EQ(CountTrailingZeros(0x31), 0);
  EXPECT_EQ(CountTrailingZeros(0x32), 1);
  EXPECT_EQ(CountTrailingZeros(0x34), 2);
  EXPECT_EQ(CountTrailingZeros(0x38), 3);
  EXPECT_EQ(CountTrailingZeros(0x310), 4);
  EXPECT_EQ(CountTrailingZeros(0x320), 5);
  EXPECT_EQ(CountTrailingZeros(0x340), 6);
  EXPECT_EQ(CountTrailingZeros(0x380), 7);
}

TEST(CommonUtilsTest, FloorLog2) {
  // Powers of 2.
  EXPECT_EQ(FloorLog2(1), 0);
  EXPECT_EQ(FloorLog2(2), 1);
  EXPECT_EQ(FloorLog2(8), 3);
  EXPECT_EQ(FloorLog2(64), 6);
  // Powers of 2 +/- 1.
  EXPECT_EQ(FloorLog2(9), 3);
  EXPECT_EQ(FloorLog2(15), 3);
  EXPECT_EQ(FloorLog2(63), 5);
  // Large value, smaller than 32 bit.
  EXPECT_EQ(FloorLog2(0x7fffffff), 30);
  EXPECT_EQ(FloorLog2(0x80000000), 31);
  // Larger than 32 bit.
  EXPECT_EQ(FloorLog2(uint64_t{0x7fffffffffffffff}), 62);
  EXPECT_EQ(FloorLog2(uint64_t{0x8000000000000000}), 63);
  EXPECT_EQ(FloorLog2(uint64_t{0xffffffffffffffff}), 63);
}

TEST(CommonUtilsTest, CeilLog2) {
  // Even though log2(0) is -inf, here we explicitly define it to be 0.
  EXPECT_EQ(CeilLog2(0), 0);
  // Powers of 2.
  EXPECT_EQ(CeilLog2(1), 0);
  EXPECT_EQ(CeilLog2(2), 1);
  EXPECT_EQ(CeilLog2(8), 3);
  EXPECT_EQ(CeilLog2(64), 6);
  // Powers of 2 +/- 1.
  EXPECT_EQ(CeilLog2(9), 4);
  EXPECT_EQ(CeilLog2(15), 4);
  EXPECT_EQ(CeilLog2(63), 6);
  // Large value.
  EXPECT_EQ(CeilLog2(0x7fffffff), 31);
}

TEST(CommonUtilsTest, RightShiftWithCeiling) {
  // Shift 1 bit.
  EXPECT_EQ(RightShiftWithCeiling(1, 1), 1);
  EXPECT_EQ(RightShiftWithCeiling(2, 1), 1);
  EXPECT_EQ(RightShiftWithCeiling(3, 1), 2);
  EXPECT_EQ(RightShiftWithCeiling(4, 1), 2);
  EXPECT_EQ(RightShiftWithCeiling(5, 1), 3);
  // Shift 2 bits.
  EXPECT_EQ(RightShiftWithCeiling(1, 2), 1);
  EXPECT_EQ(RightShiftWithCeiling(2, 2), 1);
  EXPECT_EQ(RightShiftWithCeiling(3, 2), 1);
  EXPECT_EQ(RightShiftWithCeiling(4, 2), 1);
  EXPECT_EQ(RightShiftWithCeiling(5, 2), 2);
  // Shift 20 bits.
  EXPECT_EQ(RightShiftWithCeiling(1, 20), 1);
  EXPECT_EQ(RightShiftWithCeiling((1 << 20) - 1, 20), 1);
  EXPECT_EQ(RightShiftWithCeiling(1 << 20, 20), 1);
  EXPECT_EQ(RightShiftWithCeiling((1 << 20) + 1, 20), 2);
  EXPECT_EQ(RightShiftWithCeiling((1 << 21) - 1, 20), 2);
}

template <typename Input, typename Output>
void VerifyRightShiftWithRounding(const Input* const values,
                                  const int* const bits,
                                  const Output* const rounded_values,
                                  size_t count) {
  for (size_t i = 0; i < count; ++i) {
    const Output rounded_value = RightShiftWithRounding(values[i], bits[i]);
    EXPECT_EQ(rounded_value, rounded_values[i]) << "Mismatch at index " << i;
    // Rounding reduces the bit length by |bits[i]| - 1.
    EXPECT_LE(BitLength(rounded_value), BitLength(values[i]) - (bits[i] - 1))
        << "Mismatch at index " << i;
  }
}

TEST(CommonUtilTest, RightShiftWithRoundingInt32) {
  static constexpr int32_t values[] = {5, 203, 204, 255, 40000, 50000};
  static constexpr int bits[] = {0, 3, 3, 3, 12, 12};
  static constexpr int32_t rounded_values[] = {5, 25, 26, 32, 10, 12};
  static_assert(ABSL_ARRAYSIZE(values) == ABSL_ARRAYSIZE(bits), "");
  static_assert(ABSL_ARRAYSIZE(values) == ABSL_ARRAYSIZE(rounded_values), "");
  VerifyRightShiftWithRounding<int32_t, int32_t>(values, bits, rounded_values,
                                                 ABSL_ARRAYSIZE(values));
}

TEST(CommonUtilTest, RightShiftWithRoundingUint32) {
  static constexpr uint32_t values[] = {5,     203,   204,       255,
                                        40000, 50000, 0x7fffffff};
  static constexpr int bits[] = {0, 3, 3, 3, 12, 12, 20};
  static constexpr uint32_t rounded_values[] = {5, 25, 26, 32, 10, 12, 2048};
  static_assert(ABSL_ARRAYSIZE(values) == ABSL_ARRAYSIZE(bits), "");
  static_assert(ABSL_ARRAYSIZE(values) == ABSL_ARRAYSIZE(rounded_values), "");
  VerifyRightShiftWithRounding<uint32_t, uint32_t>(values, bits, rounded_values,
                                                   ABSL_ARRAYSIZE(values));
}

TEST(CommonUtilTest, RightShiftWithRoundingInt64) {
  static constexpr int64_t values[] = {5,     203,   204,        255,
                                       40000, 50000, 0x7fffffff, 0x8fffffff};
  static constexpr int bits[] = {0, 3, 3, 3, 12, 12, 20, 20};
  static constexpr int32_t rounded_values[] = {5,  25, 26,   32,
                                               10, 12, 2048, 2304};
  static_assert(ABSL_ARRAYSIZE(values) == ABSL_ARRAYSIZE(bits), "");
  static_assert(ABSL_ARRAYSIZE(values) == ABSL_ARRAYSIZE(rounded_values), "");
  VerifyRightShiftWithRounding<int64_t, int32_t>(values, bits, rounded_values,
                                                 ABSL_ARRAYSIZE(values));
}

template <typename Input>
void VerifyRightShiftWithRoundingSigned(const Input* const values,
                                        const int* const bits,
                                        const int32_t* const rounded_values,
                                        int count) {
  for (int i = 0; i < count; ++i) {
    int32_t rounded_value = RightShiftWithRoundingSigned(values[i], bits[i]);
    EXPECT_EQ(rounded_value, rounded_values[i]) << "Mismatch at index " << i;
    rounded_value = RightShiftWithRoundingSigned(-values[i], bits[i]);
    EXPECT_EQ(rounded_value, -rounded_values[i]) << "Mismatch at index " << i;
  }
}

TEST(CommonUtilTest, RightShiftWithRoundingSignedInt32) {
  static constexpr int32_t values[] = {203, 204, 255, 40000, 50000};
  static constexpr int bits[] = {3, 3, 3, 12, 12};
  static constexpr int32_t rounded_values[] = {25, 26, 32, 10, 12};
  static_assert(ABSL_ARRAYSIZE(values) == ABSL_ARRAYSIZE(bits), "");
  static_assert(ABSL_ARRAYSIZE(values) == ABSL_ARRAYSIZE(rounded_values), "");
  VerifyRightShiftWithRoundingSigned<int32_t>(values, bits, rounded_values,
                                              ABSL_ARRAYSIZE(values));
}

TEST(CommonUtilTest, RightShiftWithRoundingSignedInt64) {
  static constexpr int64_t values[] = {203,   204,        255,       40000,
                                       50000, 0x7fffffff, 0x8fffffff};
  static constexpr int bits[] = {3, 3, 3, 12, 12, 20, 20};
  static constexpr int32_t rounded_values[] = {25, 26, 32, 10, 12, 2048, 2304};
  static_assert(ABSL_ARRAYSIZE(values) == ABSL_ARRAYSIZE(bits), "");
  static_assert(ABSL_ARRAYSIZE(values) == ABSL_ARRAYSIZE(rounded_values), "");
  VerifyRightShiftWithRoundingSigned<int64_t>(values, bits, rounded_values,
                                              ABSL_ARRAYSIZE(values));
}

TEST(CommonUtilTest, GetResidualBufferSize) {
  // No subsampling.
  EXPECT_EQ(GetResidualBufferSize(64, 64, 0, 0, 2),
            /* 2*(64*64*3/1 + 32*4) = */ 24832);
  // Only X is subsampled.
  EXPECT_EQ(GetResidualBufferSize(64, 64, 1, 0, 2),
            /* 2*(64*64*2/1 + 32*4) = */ 16640);
  // Only Y is subsampled.
  EXPECT_EQ(GetResidualBufferSize(64, 64, 0, 1, 2),
            /* 2*(64*64*2/1 + 32*4) = */ 16640);
  // Both X and Y are subsampled.
  EXPECT_EQ(GetResidualBufferSize(64, 64, 1, 1, 2),
            /* 2*(64*64*3/2 + 32*4) = */ 12544);
}

//------------------------------------------------------------------------------
// Tests for bitstream util functions

TEST(BitstreamUtilTest, IsIntraFrame) {
  EXPECT_TRUE(IsIntraFrame(kFrameKey));
  EXPECT_TRUE(IsIntraFrame(kFrameIntraOnly));
  EXPECT_FALSE(IsIntraFrame(kFrameInter));
  EXPECT_FALSE(IsIntraFrame(kFrameSwitch));
}

TEST(BitstreamUtilTest, GetTransformClass) {
  static constexpr TransformClass expected_classes[kNumTransformTypes] = {
      kTransformClass2D,       kTransformClass2D,
      kTransformClass2D,       kTransformClass2D,
      kTransformClass2D,       kTransformClass2D,
      kTransformClass2D,       kTransformClass2D,
      kTransformClass2D,       kTransformClass2D,
      kTransformClassVertical, kTransformClassHorizontal,
      kTransformClassVertical, kTransformClassHorizontal,
      kTransformClassVertical, kTransformClassHorizontal,
  };
  for (int i = 0; i < kNumTransformTypes; ++i) {
    EXPECT_EQ(GetTransformClass(static_cast<TransformType>(i)),
              expected_classes[i])
        << "Mismatch at index " << i;
  }
}

TEST(BitstreamUtilTest, RowOrColumn4x4ToPixel) {
  EXPECT_EQ(RowOrColumn4x4ToPixel(10, kPlaneY, 0), 40);
  EXPECT_EQ(RowOrColumn4x4ToPixel(10, kPlaneY, 1),
            40);  // Subsampling should have no effect on Y plane.
  EXPECT_EQ(RowOrColumn4x4ToPixel(10, kPlaneU, 0), 40);
  EXPECT_EQ(RowOrColumn4x4ToPixel(10, kPlaneU, 1), 20);
  EXPECT_EQ(RowOrColumn4x4ToPixel(10, kPlaneV, 0), 40);
  EXPECT_EQ(RowOrColumn4x4ToPixel(10, kPlaneV, 1), 20);
}

TEST(BitstreamUtilTest, GetPlaneType) {
  EXPECT_EQ(GetPlaneType(kPlaneY), kPlaneTypeY);
  EXPECT_EQ(GetPlaneType(kPlaneU), kPlaneTypeUV);
  EXPECT_EQ(GetPlaneType(kPlaneV), kPlaneTypeUV);
}

TEST(BitstreamUtils, IsDirectionalMode) {
  static constexpr bool is_directional_modes[kNumPredictionModes] = {
      false, true,  true,  true,  true,  true,  true,  true,  true,
      false, false, false, false, false, false, false, false, false,
      false, false, false, false, false, false, false, false,
  };
  for (int i = 0; i < kNumPredictionModes; ++i) {
    EXPECT_EQ(IsDirectionalMode(static_cast<PredictionMode>(i)),
              is_directional_modes[i])
        << "Mismatch at index " << i;
  }
}

TEST(BitstreamUtils, GetRelativeDistance) {
  // Both order_hint_bits and order_hint_shift_bits are zero. (a and b must be
  // zero.)
  EXPECT_EQ(GetRelativeDistance(0, 0, 0), 0);
  EXPECT_EQ(GetRelativeDistance(10, 20, 27), -10);

  EXPECT_EQ(GetRelativeDistance(2, 1, 30), 1);
  EXPECT_EQ(GetRelativeDistance(2, 1, 29), 1);

  EXPECT_EQ(GetRelativeDistance(1, 2, 30), -1);
  EXPECT_EQ(GetRelativeDistance(1, 2, 29), -1);

  // With an order_hint_bits of 4 and an order_hint_shift_bits of 28, 16 is the
  // same as 0, 17 is the same as 1, etc. The most positive distance is 7, and
  // the most negative distance is -8.

  EXPECT_EQ(GetRelativeDistance(2, 6, 28), -4);
  EXPECT_EQ(GetRelativeDistance(6, 2, 28), 4);
  // 18 - 14 = 4.
  EXPECT_EQ(GetRelativeDistance(2, 14, 28), 4);
  // 14 - 18 = -4.
  EXPECT_EQ(GetRelativeDistance(14, 2, 28), -4);
  // If a and b are exactly 8 apart, GetRelativeDistance() cannot tell whether
  // a is before or after b. GetRelativeDistance(a, b) and
  // GetRelativeDistance(b, a) are both -8.
  // 1 - 9 = -8.
  EXPECT_EQ(GetRelativeDistance(1, 9, 28), -8);
  // 9 - 17 = -8.
  EXPECT_EQ(GetRelativeDistance(9, 1, 28), -8);

  // With an order_hint_bits of 5 and an order_hint_shift_bits of 27, 32 is the
  // same as 0, 33 is the same as 1, etc. The most positive distance is 15, and
  // the most negative distance is -16.

  // 31 - 32 = -1.
  EXPECT_EQ(GetRelativeDistance(31, 0, 27), -1);
  // 32 - 31 = 1.
  EXPECT_EQ(GetRelativeDistance(0, 31, 27), 1);
  // 30 - 33 = -3.
  EXPECT_EQ(GetRelativeDistance(30, 1, 27), -3);
  // 33 - 30 = 3.
  EXPECT_EQ(GetRelativeDistance(1, 30, 27), 3);
  // 25 - 36 = -11.
  EXPECT_EQ(GetRelativeDistance(25, 4, 27), -11);
  // 36 - 25 = 11.
  EXPECT_EQ(GetRelativeDistance(4, 25, 27), 11);
  // 15 - 0 = 15.
  EXPECT_EQ(GetRelativeDistance(15, 0, 27), 15);
  // If a and b are exactly 16 apart, GetRelativeDistance() cannot tell whether
  // a is before or after b. GetRelativeDistance(a, b) and
  // GetRelativeDistance(b, a) are both -16.
  // 16 - 32 = -16.
  EXPECT_EQ(GetRelativeDistance(16, 0, 27), -16);
  // 0 - 16 = -16.
  EXPECT_EQ(GetRelativeDistance(0, 16, 27), -16);
}

TEST(BitstreamUtils, ApplySign) {
  // ApplyPositive(0) = 0
  EXPECT_EQ(ApplySign(0, 0), 0);
  // ApplyNegative(0) = 0
  EXPECT_EQ(ApplySign(0, -1), 0);

  // ApplyPositive(1) = 1
  EXPECT_EQ(ApplySign(1, 0), 1);
  // ApplyNegative(1) = -1
  EXPECT_EQ(ApplySign(1, -1), -1);

  // ApplyPositive(-1) = -1
  EXPECT_EQ(ApplySign(-1, 0), -1);
  // ApplyNegative(-1) = 1
  EXPECT_EQ(ApplySign(-1, -1), 1);

  // ApplyPositive(1234) = 1234
  EXPECT_EQ(ApplySign(1234, 0), 1234);
  // ApplyNegative(1234) = -1234
  EXPECT_EQ(ApplySign(1234, -1), -1234);

  // ApplyPositive(-1234) = -1234
  EXPECT_EQ(ApplySign(-1234, 0), -1234);
  // ApplyNegative(-1234) = 1234
  EXPECT_EQ(ApplySign(-1234, -1), 1234);
}

// 7.9.3. (without the clamp for numerator and denominator).
int SpecGetMvProjectionKernel(int mv, int numerator, int denominator) {
  int value = mv * numerator * kProjectionMvDivisionLookup[denominator];
  if (value >= 0) {
    value += 1 << 13;
    value >>= 14;
  } else {
    value = -value;
    value += 1 << 13;
    value >>= 14;
    value = -value;
  }
  if (value < (-(1 << 14) + 1)) value = -(1 << 14) + 1;
  if (value > (1 << 14) - 1) value = (1 << 14) - 1;
  return value;
}

void SpecGetMvProjectionNoClamp(const MotionVector& mv, int numerator,
                                int denominator, MotionVector* projection_mv) {
  for (int i = 0; i < 2; ++i) {
    projection_mv->mv[i] =
        SpecGetMvProjectionKernel(mv.mv[i], numerator, denominator);
  }
}

TEST(BitstreamUtils, GetMvProjection) {
  const int16_t mvs[5][2] = {
      {0, 0}, {11, 73}, {-84, 272}, {733, -827}, {-472, -697}};
  for (auto& mv_value : mvs) {
    for (int numerator = -kMaxFrameDistance; numerator <= kMaxFrameDistance;
         ++numerator) {
      for (int denominator = 0; denominator <= kMaxFrameDistance;
           ++denominator) {
        MotionVector mv, projection_mv, spec_projection_mv;
        mv.mv[0] = mv_value[0];
        mv.mv[1] = mv_value[1];
        GetMvProjection(mv, numerator, kProjectionMvDivisionLookup[denominator],
                        &projection_mv);
        SpecGetMvProjectionNoClamp(mv, numerator, denominator,
                                   &spec_projection_mv);
        EXPECT_EQ(projection_mv.mv32, spec_projection_mv.mv32);
      }
    }
  }
}

// 7.9.4.
int SpecProject(int value, int delta, int dst_sign) {
  constexpr int kMiSizeLog2 = 2;
  const int sign = (dst_sign == 0) ? 1 : dst_sign;
  int offset;
  if (delta >= 0) {
    offset = delta >> (3 + 1 + kMiSizeLog2);
  } else {
    offset = -((-delta) >> (3 + 1 + kMiSizeLog2));
  }
  return value + sign * offset;
}

TEST(BitstreamUtils, Project) {
  for (int value = -10; value <= 10; ++value) {
    for (int delta = -256; delta <= 256; ++delta) {
      for (int dst_sign = -1; dst_sign <= 0; ++dst_sign) {
        EXPECT_EQ(Project(value, delta, dst_sign),
                  SpecProject(value, delta, dst_sign));
      }
    }
  }
}

TEST(BitstreamUtils, IsBlockSmallerThan8x8) {
  static constexpr bool is_block_smaller_than8x8[kMaxBlockSizes] = {
      true,  true,  false, true,  false, false, false, false,
      false, false, false, false, false, false, false, false,
      false, false, false, false, false, false,
  };
  for (int i = 0; i < kMaxBlockSizes; ++i) {
    EXPECT_EQ(IsBlockSmallerThan8x8(static_cast<BlockSize>(i)),
              is_block_smaller_than8x8[i])
        << "Mismatch at index " << i;
  }
}

TEST(BitstreamUtils, TransformSizeToSquareTransformIndex) {
  EXPECT_EQ(TransformSizeToSquareTransformIndex(kTransformSize4x4), 0);
  EXPECT_EQ(TransformSizeToSquareTransformIndex(kTransformSize8x8), 1);
  EXPECT_EQ(TransformSizeToSquareTransformIndex(kTransformSize16x16), 2);
  EXPECT_EQ(TransformSizeToSquareTransformIndex(kTransformSize32x32), 3);
  EXPECT_EQ(TransformSizeToSquareTransformIndex(kTransformSize64x64), 4);
}

}  // namespace
}  // namespace libgav1
