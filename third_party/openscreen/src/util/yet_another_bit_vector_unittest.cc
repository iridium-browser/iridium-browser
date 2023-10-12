// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/yet_another_bit_vector.h"

#include <algorithm>

#include "absl/algorithm/container.h"
#include "gtest/gtest.h"
#include "platform/base/span.h"

namespace openscreen {
namespace {

constexpr uint8_t kBitPatterns[] = {0b00000000, 0b11111111, 0b01010101,
                                    0b10101010, 0b00100100, 0b01001001,
                                    0b10010010, 0b00110110};

// These are used for testing various vector sizes, begins/ends of ranges, etc.
// They will exercise both the "inlined storage" (size <= 64 case) and
// "heap-allocated storage" cases. These are all of the prime numbers less than
// 100, and also any non-negative multiples of 64 less than 192.
const int kTestSizes[] = {0,  1,  2,  3,  5,  7,  11, 13, 17,  19,
                          23, 29, 31, 37, 41, 43, 47, 53, 59,  61,
                          64, 67, 71, 73, 79, 83, 89, 97, 127, 128};

// Returns a subspan of |kTestSizes| that contains all values in the range
// [first,last].
Span<const int> GetTestSizesInRange(int first, int last) {
  const auto begin = absl::c_lower_bound(kTestSizes, first);
  const auto end = absl::c_upper_bound(kTestSizes, last);
  return Span<const int>(&*begin, std::distance(begin, end));
}

// Returns true if an infinitely-repeating |pattern| has a bit set at the given
// |position|.
constexpr bool IsSetInPattern(uint8_t pattern, int position) {
  constexpr int kRepeatPeriod = sizeof(pattern) * CHAR_BIT;
  return !!((pattern >> (position % kRepeatPeriod)) & 1);
}

// Fills an infinitely-repeating |pattern| in |v|, but only modifies the bits at
// and after the given |from| position.
void FillWithPattern(uint8_t pattern, int from, YetAnotherBitVector* v) {
  for (int i = from; i < v->size(); ++i) {
    if (IsSetInPattern(pattern, i)) {
      v->Set(i);
    } else {
      v->Clear(i);
    }
  }
}

// Tests that construction and resizes initialize the vector to the correct size
// and set or clear all of its bits, as requested.
TEST(YetAnotherBitVectorTest, ConstructsAndResizes) {
  YetAnotherBitVector v;
  ASSERT_EQ(v.size(), 0);

  for (int fill_set = 0; fill_set <= 1; ++fill_set) {
    for (int size : kTestSizes) {
      const bool all_bits_should_be_set = !!fill_set;
      v.Resize(size, all_bits_should_be_set ? YetAnotherBitVector::SET
                                            : YetAnotherBitVector::CLEARED);
      ASSERT_EQ(size, v.size());
      for (int i = 0; i < size; ++i) {
        ASSERT_EQ(all_bits_should_be_set, v.IsSet(i));
      }
    }
  }
}

// Tests that individual bits can be set and cleared for various vector sizes
// and bit patterns.
TEST(YetAnotherBitVectorTest, SetsAndClearsIndividualBits) {
  YetAnotherBitVector v;
  for (int fill_set = 0; fill_set <= 1; ++fill_set) {
    for (int size : kTestSizes) {
      v.Resize(size, fill_set ? YetAnotherBitVector::SET
                              : YetAnotherBitVector::CLEARED);

      for (uint8_t pattern : kBitPatterns) {
        FillWithPattern(pattern, 0, &v);
        for (int i = 0; i < size; ++i) {
          ASSERT_EQ(IsSetInPattern(pattern, i), v.IsSet(i));
        }
      }
    }
  }
}

// Tests that the vector shifts its bits right by various amounts, for various
// vector sizes and bit patterns.
TEST(YetAnotherBitVectorTest, ShiftsRight) {
  YetAnotherBitVector v;
  for (int size : kTestSizes) {
    v.Resize(size, YetAnotherBitVector::CLEARED);

    for (int steps_per_shift : GetTestSizesInRange(0, size)) {
      for (uint8_t pattern : kBitPatterns) {
        FillWithPattern(pattern, 0, &v);

        if (size == 0 || steps_per_shift == 0) {
          v.ShiftRight(0);
          for (int i = 0; i < size; ++i) {
            ASSERT_EQ(IsSetInPattern(pattern, i), v.IsSet(i));
          }
        } else {
          const int num_shifts = 2 * size / steps_per_shift;
          for (int iteration = 1; iteration <= num_shifts; ++iteration) {
            v.ShiftRight(steps_per_shift);
            const int total_shift_amount = iteration * steps_per_shift;
            for (int i = 0; i < size; ++i) {
              const int original_position = i + total_shift_amount;
              if (original_position >= size) {
                ASSERT_FALSE(v.IsSet(i));
              } else {
                ASSERT_EQ(IsSetInPattern(pattern, original_position),
                          v.IsSet(i));
              }
            }
          }
        }
      }
    }
  }
}

// Tests the FindFirstSet() operation, for various vector sizes and bit
// patterns.
TEST(YetAnotherBitVectorTest, FindsTheFirstBitSet) {
  YetAnotherBitVector v;

  // For various sizes of vector where no bits are set, the FFS operation should
  // always return size().
  for (int size : kTestSizes) {
    v.Resize(size, YetAnotherBitVector::CLEARED);
    ASSERT_EQ(size, v.FindFirstSet());
  }

  // For various sizes of vector where only one bit is set, the FFS operation
  // should always return the position of that bit.
  for (int size : kTestSizes) {
    v.Resize(size, YetAnotherBitVector::CLEARED);

    for (int position_plus_one : GetTestSizesInRange(1, size)) {
      const int position = position_plus_one - 1;
      v.Set(position);
      ASSERT_EQ(position, v.FindFirstSet());
      v.Clear(position);
    }
  }

  // For various sizes of vector where a pattern of bits are set, the FFS
  // operation should always return the first one set.
  for (int size : kTestSizes) {
    v.Resize(size, YetAnotherBitVector::CLEARED);

    for (int position_plus_one : GetTestSizesInRange(1, size)) {
      const int position = position_plus_one - 1;
      v.ClearAll();
      v.Set(position);
      for (uint8_t pattern : kBitPatterns) {
        FillWithPattern(pattern, position_plus_one, &v);
        ASSERT_EQ(position, v.FindFirstSet());
      }
      v.Clear(position);
    }
  }
}

// Tests the CountBitsSet() operation, for various vector sizes, bit patterns,
// and ranges of bits being counted.
TEST(YetAnotherBitVector, CountsTheNumberOfBitsSet) {
  YetAnotherBitVector v;

  // For various sizes of vector where no bits are set, the operation should
  // always return zero for any range.
  for (int size : kTestSizes) {
    v.Resize(size, YetAnotherBitVector::CLEARED);
    for (int begin : GetTestSizesInRange(0, size)) {
      for (int end : GetTestSizesInRange(begin, size)) {
        ASSERT_EQ(0, v.CountBitsSet(begin, end));
      }
    }
  }

  // For various sizes of vector where all bits are set, the operation should
  // always return the length of the range (or zero for invalid ranges).
  for (int size : kTestSizes) {
    v.Resize(size, YetAnotherBitVector::SET);
    for (int begin : GetTestSizesInRange(0, size)) {
      for (int end : GetTestSizesInRange(begin, size)) {
        ASSERT_EQ(end - begin, v.CountBitsSet(begin, end));
      }
    }
  }

  // Test various sizes of vector where various patterns of bits are set.
  for (int size : kTestSizes) {
    v.Resize(size, YetAnotherBitVector::CLEARED);
    for (uint8_t pattern : kBitPatterns) {
      FillWithPattern(pattern, 0, &v);

      for (int begin : GetTestSizesInRange(0, size)) {
        for (int end : GetTestSizesInRange(begin, size)) {
          // Note: The expected value being manually computed by examining each
          // bit individually by calling IsSet(). Thus, this value is only good
          // if IsSet() is working (which is tested by a different unit test).
          int expected_popcount = 0;
          for (int i = begin; i < end; ++i) {
            if (v.IsSet(i)) {
              ++expected_popcount;
            }
          }

          ASSERT_EQ(expected_popcount, v.CountBitsSet(begin, end));
        }
      }
    }
  }
}

}  // namespace
}  // namespace openscreen
