// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_API_TIME_H_
#define PLATFORM_API_TIME_H_

#include <chrono>

#include "platform/base/trivial_clock_traits.h"

namespace openscreen {

// The "reasonably high-resolution" source of monotonic time from the embedder,
// exhibiting the traits described in TrivialClockTraits. This class is not
// instantiated. It only contains a static now() function.
//
// For example, the default platform implementation bases this on
// std::chrono::steady_clock or std::chrono::high_resolution_clock, but an
// embedder may choose to use a different source of time (e.g., the embedder's
// time library, a simulated time source, or a mock).
class Clock : public TrivialClockTraits {
 public:
  // Returns the current time.
  static time_point now() noexcept;
};

// Returns the number of seconds since UNIX epoch (1 Jan 1970, midnight)
// according to the wall clock, which is subject to adjustments (e.g., via NTP).
// Note that this is NOT necessarily the same time source as Clock::now() above,
// and is NOT guaranteed to be monotonically non-decreasing; it is "calendar
// time."
std::chrono::seconds GetWallTimeSinceUnixEpoch() noexcept;

}  // namespace openscreen

#endif  // PLATFORM_API_TIME_H_
