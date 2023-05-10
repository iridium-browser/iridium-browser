// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_SATURATE_CAST_H_
#define UTIL_SATURATE_CAST_H_

#include <cmath>
#include <limits>
#include <type_traits>

namespace openscreen {

// Case 0: When To and From are the same type, saturate_cast<> is pass-through.
template <typename To, typename From>
constexpr std::enable_if_t<
    std::is_same<std::remove_cv<To>, std::remove_cv<From>>::value,
    To>
saturate_cast(From from) {
  return from;
}

// Because of the way C++ signed versus unsigned comparison works (i.e., the
// type promotion strategy employed), extra care must be taken to range-check
// the input value. For example, if the current architecture is 32-bits, then
// any int32_t compared with a uint32_t will NOT promote to a int64_t↔int64_t
// comparison. Instead, it will become a uint32_t↔uint32_t comparison (!),
// which will sometimes produce invalid results.

// Case 1: "From" and "To" are either both signed, or are both unsigned. In
// this case, the smaller of the two types will be promoted to match the
// larger's size, and a valid comparison will be made.
template <typename To, typename From>
constexpr std::enable_if_t<
    std::is_integral<From>::value && std::is_integral<To>::value &&
        (std::is_signed<From>::value == std::is_signed<To>::value),
    To>
saturate_cast(From from) {
  if (from <= std::numeric_limits<To>::min()) {
    return std::numeric_limits<To>::min();
  }
  if (from >= std::numeric_limits<To>::max()) {
    return std::numeric_limits<To>::max();
  }
  return static_cast<To>(from);
}

// Case 2: "From" is signed, but "To" is unsigned.
template <typename To, typename From>
constexpr std::enable_if_t<
    std::is_integral<From>::value && std::is_integral<To>::value &&
        std::is_signed<From>::value && !std::is_signed<To>::value,
    To>
saturate_cast(From from) {
  if (from <= From{0}) {
    return To{0};
  }
  if (static_cast<std::make_unsigned_t<From>>(from) >=
      std::numeric_limits<To>::max()) {
    return std::numeric_limits<To>::max();
  }
  return static_cast<To>(from);
}

// Case 3: "From" is unsigned, but "To" is signed.
template <typename To, typename From>
constexpr std::enable_if_t<
    std::is_integral<From>::value && std::is_integral<To>::value &&
        !std::is_signed<From>::value && std::is_signed<To>::value,
    To>
saturate_cast(From from) {
  if (from >= static_cast<typename std::make_unsigned_t<To>>(
                  std::numeric_limits<To>::max())) {
    return std::numeric_limits<To>::max();
  }
  return static_cast<To>(from);
}

// Case 4: "From" is a floating-point type, and "To" is an integer type (signed
// or unsigned). The result is truncated, per the usual C++ float-to-int
// conversion rules.
template <typename To, typename From>
constexpr std::enable_if_t<std::is_floating_point<From>::value &&
                               std::is_integral<To>::value,
                           To>
saturate_cast(From from) {
  // Note: It's invalid to compare the argument against
  // std::numeric_limits<To>::max() because the latter, an integer value, will
  // be type-promoted to the floating-point type. The problem is that the
  // conversion is imprecise, as "max int" might not be exactly representable as
  // a floating-point value (depending on the actual types of From and To).
  //
  // Thus, the strategy is to compare only floating-point values/constants to
  // determine whether the bounds of the range of integers has been exceeded.
  // Two assumptions here: 1) "To" is either unsigned, or is a 2's complement
  // signed integer type. 2) "From" is a floating-point type that can exactly
  // represent all powers of 2 within its value range.
  static_assert((~To(1) + To(1)) == To(-1), "assumed 2's complement integers");
  constexpr From kMaxIntPlusOne =
      From(To(1) << (std::numeric_limits<To>::digits - 1)) * From(2);
  constexpr From kMaxInt = kMaxIntPlusOne - 1;
  // Note: In some cases, the kMaxInt constant will equal kMaxIntPlusOne because
  // there isn't an exact floating-point representation for 2^N - 1. That said,
  // the following upper-bound comparison is still valid because all
  // floating-point values less than 2^N would also be less than 2^N - 1.
  if (from >= kMaxInt) {
    return std::numeric_limits<To>::max();
  }
  if (std::is_signed<To>::value) {
    constexpr From kMinInt = -kMaxIntPlusOne;
    if (from <= kMinInt) {
      return std::numeric_limits<To>::min();
    }
  } else /* if To is unsigned */ {
    if (from <= From(0)) {
      return To(0);
    }
  }
  return static_cast<To>(from);
}

// Like saturate_cast<>, but rounds to the nearest integer instead of
// truncating.
template <typename To, typename From>
constexpr std::enable_if_t<std::is_floating_point<From>::value &&
                               std::is_integral<To>::value,
                           To>
rounded_saturate_cast(From from) {
  const To saturated = saturate_cast<To>(from);
  if (saturated == std::numeric_limits<To>::min() ||
      saturated == std::numeric_limits<To>::max()) {
    return saturated;
  }

  static_assert(sizeof(To) <= sizeof(decltype(llround(from))),
                "No version of lround() for the required range of values.");
  if (sizeof(To) <= sizeof(decltype(lround(from)))) {
    return static_cast<To>(lround(from));
  }
  return static_cast<To>(llround(from));
}

}  // namespace openscreen

#endif  // UTIL_SATURATE_CAST_H_
