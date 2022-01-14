// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_INTEGER_DIVISION_H_
#define UTIL_INTEGER_DIVISION_H_

#include <type_traits>

namespace openscreen {

// Returns CEIL(num ÷ denom). |denom| must not equal zero. This function is
// compatible with any integer-like type, including the integer-based
// std::chrono duration types.
//
// Optimization note: See DividePositivesRoundingUp().
template <typename Integer>
constexpr auto DivideRoundingUp(Integer num, Integer denom) {
  if (denom < Integer{0}) {
    num *= -1;
    denom *= -1;
  }
  if (num < Integer{0}) {
    return num / denom;
  }
  return (num + denom - Integer{1}) / denom;
}

// Same as DivideRoundingUp(), except is more-efficient for hot code paths that
// know |num| is always greater or equal to zero, and |denom| is always greater
// than zero.
template <typename Integer>
constexpr Integer DividePositivesRoundingUp(Integer num, Integer denom) {
  return DivideRoundingUp<typename std::make_unsigned<Integer>::type>(num,
                                                                      denom);
}

// Divides |num| by |denom|, and rounds to the nearest integer (exactly halfway
// between integers will round to the higher integer). This function is
// compatible with any integer-like type, including the integer-based
// std::chrono duration types.
//
// Optimization note: See DividePositivesRoundingNearest().
template <typename Integer>
constexpr auto DivideRoundingNearest(Integer num, Integer denom) {
  if (denom < Integer{0}) {
    num *= -1;
    denom *= -1;
  }
  if (num < Integer{0}) {
    return (num - ((denom - Integer{1}) / 2)) / denom;
  }
  return (num + (denom / 2)) / denom;
}

// Same as DivideRoundingNearest(), except is more-efficient for hot code paths
// that know |num| is always greater or equal to zero, and |denom| is always
// greater than zero.
template <typename Integer>
constexpr Integer DividePositivesRoundingNearest(Integer num, Integer denom) {
  return DivideRoundingNearest<typename std::make_unsigned<Integer>::type>(
      num, denom);
}

}  // namespace openscreen

#endif  // UTIL_INTEGER_DIVISION_H_
