// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/api/time.h"

#include <chrono>
#include <thread>

#include "gtest/gtest.h"
#include "util/chrono_helpers.h"

namespace openscreen {
namespace {

// Tests that the clock always seems to tick forward. If this test is broken, or
// is flaky, the time source is probably not monotonic.
TEST(TimeTest, PlatformClockIsMonotonic) {
  constexpr auto kSleepPeriod = milliseconds(2);
  for (int i = 0; i < 50; ++i) {
    const auto start = Clock::now();
    std::this_thread::sleep_for(kSleepPeriod);
    const auto stop = Clock::now();
    EXPECT_GE(stop - start, kSleepPeriod / 2);
  }
}

// Tests that the clock ticks at least 10000 times per second, a requirement
// specified in the API header comments.
TEST(TimeTest, PlatformClockHasSufficientResolution) {
  constexpr std::chrono::duration<int, Clock::kRequiredResolution>
      kMaxAllowedDurationBetweenTicks(1);
  constexpr int kMaxRetries = 100;

  // Loop until a small-enough clock change is observed. The platform is given
  // multiple chances because unpredictable events, like thread context
  // switches, could suspend the current thread for a "long" time, masking what
  // the clock is actually capable of.
  Clock::duration delta = microseconds(0);
  for (int i = 0; i < kMaxRetries; ++i) {
    const auto start = Clock::now();
    // Loop until the clock changes.
    do {
      delta = Clock::now() - start;
      ASSERT_LE(microseconds(0), delta);
    } while (delta == microseconds(0));

    if (delta <= kMaxAllowedDurationBetweenTicks) {
      break;
    }
  }

  EXPECT_LE(delta, kMaxAllowedDurationBetweenTicks);
}

}  // namespace
}  // namespace openscreen
