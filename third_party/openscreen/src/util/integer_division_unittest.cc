// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/integer_division.h"

#include <chrono>

#include "gtest/gtest.h"

namespace openscreen {
namespace {

constexpr int kDenominators[2] = {3, 4};

// Common test routine that tests one of the integer division functions using a
// fixed denominator and stepping, one-by-one, over a range of numerators around
// zero.
template <typename Input, typename Output>
void TestRangeAboutZero(int denom,
                        int range_of_numerators,
                        int first_expected_result,
                        Output (*function_to_test)(Input, Input)) {
  int expected_result = first_expected_result;
  int count_until_next_change = denom;
  for (int num = -range_of_numerators; num <= range_of_numerators; ++num) {
    EXPECT_EQ(expected_result, function_to_test(Input(num), Input(denom)))
        << "num=" << num << ", denom=" << denom;
    EXPECT_EQ(expected_result, function_to_test(Input(-num), Input(-denom)))
        << "num=" << (-num) << ", denom=" << (-denom);

    --count_until_next_change;
    if (count_until_next_change == 0) {  // Next result will be one higher.
      ++expected_result;
      count_until_next_change = denom;
    }
  }
}

TEST(IntegerDivision, DividesAndRoundsUpInts) {
  auto* const function_to_test = &DivideRoundingUp<int>;
  for (int denom : kDenominators) {
    TestRangeAboutZero(denom, denom == 3 ? 11 : 15, -3, function_to_test);
  }
}

TEST(IntegerDivision, DividesAndRoundsUpChronoDurations) {
  auto* const function_to_test = &DivideRoundingUp<std::chrono::milliseconds>;
  for (int denom : kDenominators) {
    TestRangeAboutZero(denom, denom == 3 ? 11 : 15, -3, function_to_test);
  }
}

// Assumption: DivideRoundingUp() is working (tested by the above two tests).
TEST(IntegerDivision, DividesPositivesAndRoundsUp) {
  for (int num = 0; num <= 6; ++num) {
    for (int denom = 1; denom <= 6; ++denom) {
      EXPECT_EQ(DivideRoundingUp(num, denom),
                DividePositivesRoundingUp(num, denom));
    }
  }
}

TEST(IntegerDivision, DividesAndRoundsNearestInts) {
  auto* const function_to_test = &DivideRoundingNearest<int>;
  for (int denom : kDenominators) {
    TestRangeAboutZero(denom, denom == 3 ? 10 : 14, -3, function_to_test);
  }
}

TEST(IntegerDivision, DividesAndRoundsNearestChronoDurations) {
  auto* const function_to_test =
      &DivideRoundingNearest<std::chrono::milliseconds>;
  for (int denom : kDenominators) {
    TestRangeAboutZero(denom, denom == 3 ? 10 : 14, -3, function_to_test);
  }
}

// Assumption: DivideRoundingNearest() is working (tested by the above two
// tests).
TEST(IntegerDivision, DividesPositivesAndRoundsNearest) {
  for (int num = 0; num <= 6; ++num) {
    for (int denom = 1; denom <= 6; ++denom) {
      EXPECT_EQ(DivideRoundingNearest(num, denom),
                DividePositivesRoundingNearest(num, denom));
    }
  }
}

}  // namespace
}  // namespace openscreen
