// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_SIMPLE_FRACTION_H_
#define UTIL_SIMPLE_FRACTION_H_

#include <cmath>
#include <limits>
#include <string>

#include "absl/strings/string_view.h"
#include "platform/base/error.h"

namespace openscreen {

// SimpleFraction is used to represent simple (or "common") fractions, composed
// of a rational number written a/b where a and b are both integers.
// Some helpful notes on SimpleFraction assumptions/limitations:
// 1. SimpleFraction does not perform reductions. 2/4 != 1/2, and -1/-1 != 1/1.
// 2. denominator = 0 is considered undefined.
// 3. numerator = saturates range to int min or int max
// 4. A SimpleFraction is "positive" if and only if it is defined and at least
//    equal to zero. Since reductions are not performed, -1/-1 is negative.
class SimpleFraction {
 public:
  static ErrorOr<SimpleFraction> FromString(absl::string_view value);
  std::string ToString() const;

  constexpr SimpleFraction() = default;
  constexpr SimpleFraction(int numerator)  // NOLINT
      : numerator_(numerator) {}
  constexpr SimpleFraction(int numerator, int denominator)
      : numerator_(numerator), denominator_(denominator) {}

  constexpr SimpleFraction(const SimpleFraction&) = default;
  constexpr SimpleFraction(SimpleFraction&&) noexcept = default;
  constexpr SimpleFraction& operator=(const SimpleFraction&) = default;
  constexpr SimpleFraction& operator=(SimpleFraction&&) = default;
  ~SimpleFraction() = default;

  constexpr bool operator==(const SimpleFraction& other) const {
    return numerator_ == other.numerator_ && denominator_ == other.denominator_;
  }

  constexpr bool operator!=(const SimpleFraction& other) const {
    return !(*this == other);
  }

  constexpr bool is_defined() const { return denominator_ != 0; }

  constexpr bool is_positive() const {
    return (numerator_ >= 0) && (denominator_ > 0);
  }

  constexpr explicit operator double() const {
    if (denominator_ == 0) {
      return nan("");
    }
    return static_cast<double>(numerator_) / static_cast<double>(denominator_);
  }

  constexpr int numerator() const { return numerator_; }
  constexpr int denominator() const { return denominator_; }

 private:
  int numerator_ = 0;
  int denominator_ = 1;
};

}  // namespace openscreen

#endif  // UTIL_SIMPLE_FRACTION_H_
