// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/base/trivial_clock_traits.h"

namespace openscreen {

std::ostream& operator<<(std::ostream& os,
                         const TrivialClockTraits::duration& d) {
  constexpr char kUnits[] = " \u03BCs";  // Greek Mu + "s"
  return os << d.count() << kUnits;
}

std::ostream& operator<<(std::ostream& os,
                         const TrivialClockTraits::time_point& tp) {
  constexpr char kUnits[] = " \u03BCs-ticks";  // Greek Mu + "s-ticks"
  return os << tp.time_since_epoch().count() << kUnits;
}

std::ostream& operator<<(std::ostream& out, const std::chrono::hours& hrs) {
  return (out << hrs.count() << " hours");
}

std::ostream& operator<<(std::ostream& out, const std::chrono::minutes& mins) {
  return (out << mins.count() << " minutes");
}

std::ostream& operator<<(std::ostream& out, const std::chrono::seconds& secs) {
  return (out << secs.count() << " seconds");
}

std::ostream& operator<<(std::ostream& out,
                         const std::chrono::milliseconds& millis) {
  return (out << millis.count() << " ms");
}

}  // namespace openscreen
