// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/base/trivial_clock_traits.h"

namespace openscreen {
namespace {

constexpr char kMicrosecondsUnits[] = " µs";
constexpr char kMicrosecondsTicksUnits[] = " µs-ticks";

}  // namespace

std::string ToString(const TrivialClockTraits::duration& d) {
  return std::to_string(d.count()) + kMicrosecondsUnits;
}

std::string ToString(const TrivialClockTraits::time_point& tp) {
  return std::to_string(tp.time_since_epoch().count()) +
         kMicrosecondsTicksUnits;
}

namespace clock_operators {

std::ostream& operator<<(std::ostream& os,
                         const TrivialClockTraits::duration& d) {
  return os << d.count() << kMicrosecondsUnits;
}

std::ostream& operator<<(std::ostream& os,
                         const TrivialClockTraits::time_point& tp) {
  return os << tp.time_since_epoch().count() << kMicrosecondsTicksUnits;
}

std::ostream& operator<<(std::ostream& os, const std::chrono::hours& hrs) {
  return (os << hrs.count() << " hours");
}

std::ostream& operator<<(std::ostream& os, const std::chrono::minutes& mins) {
  return (os << mins.count() << " minutes");
}

std::ostream& operator<<(std::ostream& os, const std::chrono::seconds& secs) {
  return (os << secs.count() << " seconds");
}

std::ostream& operator<<(std::ostream& os,
                         const std::chrono::milliseconds& millis) {
  return (os << millis.count() << " ms");
}

}  // namespace clock_operators

}  // namespace openscreen
