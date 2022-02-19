// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/simple_fraction.h"

#include <cmath>
#include <limits>

#include "gtest/gtest.h"

namespace openscreen {

namespace {

constexpr int kMin = std::numeric_limits<int>::min();
constexpr int kMax = std::numeric_limits<int>::max();

void ExpectFromStringEquals(const char* s, const SimpleFraction& expected) {
  const ErrorOr<SimpleFraction> f = SimpleFraction::FromString(std::string(s));
  EXPECT_TRUE(f.is_value()) << "from string: '" << s << "'";
  EXPECT_EQ(expected, f.value());
}

void ExpectFromStringError(const char* s) {
  const auto f = SimpleFraction::FromString(std::string(s));
  EXPECT_TRUE(f.is_error()) << "from string: '" << s << "'";
}
}  // namespace

TEST(SimpleFractionTest, FromStringParsesCorrectFractions) {
  ExpectFromStringEquals("1/2", SimpleFraction{1, 2});
  ExpectFromStringEquals("99/3", SimpleFraction{99, 3});
  ExpectFromStringEquals("-1/2", SimpleFraction{-1, 2});
  ExpectFromStringEquals("-13/-37", SimpleFraction{-13, -37});
  ExpectFromStringEquals("1/0", SimpleFraction{1, 0});
  ExpectFromStringEquals("1", SimpleFraction{1, 1});
  ExpectFromStringEquals("0", SimpleFraction{0, 1});
  ExpectFromStringEquals("-20", SimpleFraction{-20, 1});
  ExpectFromStringEquals("100", SimpleFraction{100, 1});
}

TEST(SimpleFractionTest, FromStringErrorsOnInvalid) {
  ExpectFromStringError("");
  ExpectFromStringError("/");
  ExpectFromStringError("1/");
  ExpectFromStringError("/1");
  ExpectFromStringError("888/");
  ExpectFromStringError("1/2/3");
  ExpectFromStringError("not a fraction at all");
}

TEST(SimpleFractionTest, Equality) {
  EXPECT_EQ((SimpleFraction{1, 2}), (SimpleFraction{1, 2}));
  EXPECT_EQ((SimpleFraction{1, 0}), (SimpleFraction{1, 0}));
  EXPECT_NE((SimpleFraction{1, 2}), (SimpleFraction{1, 3}));

  // We currently don't do any reduction.
  EXPECT_NE((SimpleFraction{2, 4}), (SimpleFraction{1, 2}));
  EXPECT_NE((SimpleFraction{9, 10}), (SimpleFraction{-9, -10}));
}

TEST(SimpleFractionTest, Definition) {
  EXPECT_TRUE((SimpleFraction{kMin, 1}).is_defined());
  EXPECT_TRUE((SimpleFraction{kMax, 1}).is_defined());

  EXPECT_FALSE((SimpleFraction{kMin, 0}).is_defined());
  EXPECT_FALSE((SimpleFraction{kMax, 0}).is_defined());
  EXPECT_FALSE((SimpleFraction{0, 0}).is_defined());
  EXPECT_FALSE((SimpleFraction{-0, -0}).is_defined());
}

TEST(SimpleFractionTest, Positivity) {
  EXPECT_TRUE((SimpleFraction{1234, 20}).is_positive());
  EXPECT_TRUE((SimpleFraction{kMax - 1, 20}).is_positive());
  EXPECT_TRUE((SimpleFraction{0, kMax}).is_positive());
  EXPECT_TRUE((SimpleFraction{kMax, 1}).is_positive());

  // Since C++ doesn't have a truly negative zero, this is positive.
  EXPECT_TRUE((SimpleFraction{-0, 1}).is_positive());

  EXPECT_FALSE((SimpleFraction{0, kMin}).is_positive());
  EXPECT_FALSE((SimpleFraction{-0, -1}).is_positive());
  EXPECT_FALSE((SimpleFraction{kMin + 1, 20}).is_positive());
  EXPECT_FALSE((SimpleFraction{kMin, 1}).is_positive());
  EXPECT_FALSE((SimpleFraction{kMin, 0}).is_positive());
  EXPECT_FALSE((SimpleFraction{kMax, 0}).is_positive());
  EXPECT_FALSE((SimpleFraction{0, 0}).is_positive());
  EXPECT_FALSE((SimpleFraction{-0, -0}).is_positive());
}

TEST(SimpleFractionTest, CastToDouble) {
  EXPECT_DOUBLE_EQ(0.0, static_cast<double>(SimpleFraction{0, 1}));
  EXPECT_DOUBLE_EQ(1.0, static_cast<double>(SimpleFraction{1, 1}));
  EXPECT_TRUE(std::isnan(static_cast<double>(SimpleFraction{1, 0})));
  EXPECT_DOUBLE_EQ(1.0, static_cast<double>(SimpleFraction{kMax, kMax}));
  EXPECT_DOUBLE_EQ(1.0, static_cast<double>(SimpleFraction{kMin, kMin}));
}

}  // namespace openscreen
