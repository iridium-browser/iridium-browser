// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_CHRONO_HELPERS_H_
#define UTIL_CHRONO_HELPERS_H_

#include <chrono>

// This file is a collection of helpful utilities and using statement for
// working with std::chrono. In practice we previously defined these frequently,
// this header allows for a single set of convenience statements.
namespace openscreen {

using hours = std::chrono::hours;
using microseconds = std::chrono::microseconds;
using milliseconds = std::chrono::milliseconds;
using nanoseconds = std::chrono::nanoseconds;
using seconds = std::chrono::seconds;

// Casting statements. Note that duration_cast is not a type, it's a function,
// so its behavior is different than the using statements above.
template <typename D>
static constexpr hours to_hours(D d) {
  return std::chrono::duration_cast<hours>(d);
}

template <typename D>
static constexpr microseconds to_microseconds(D d) {
  return std::chrono::duration_cast<microseconds>(d);
}

template <typename D>
static constexpr milliseconds to_milliseconds(D d) {
  return std::chrono::duration_cast<milliseconds>(d);
}

template <typename D>
static constexpr nanoseconds to_nanoseconds(D d) {
  return std::chrono::duration_cast<nanoseconds>(d);
}

template <typename D>
static constexpr seconds to_seconds(D d) {
  return std::chrono::duration_cast<seconds>(d);
}

}  // namespace openscreen

#endif  // UTIL_CHRONO_HELPERS_H_
