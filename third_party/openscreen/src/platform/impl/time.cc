// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/api/time.h"

#include <chrono>
#include <ctime>
#include <ratio>

#include "util/chrono_helpers.h"
#include "util/osp_logging.h"

using std::chrono::high_resolution_clock;
using std::chrono::steady_clock;
using std::chrono::system_clock;

namespace openscreen {

Clock::time_point Clock::now() noexcept {
  constexpr bool can_use_steady_clock =
      std::ratio_less_equal<steady_clock::period,
                            Clock::kRequiredResolution>::value;
  constexpr bool can_use_high_resolution_clock =
      std::ratio_less_equal<high_resolution_clock::period,
                            Clock::kRequiredResolution>::value &&
      high_resolution_clock::is_steady;
  static_assert(can_use_steady_clock || can_use_high_resolution_clock,
                "no suitable default clock on this platform");

  // Choose whether to use the steady_clock or the high_resolution_clock. The
  // general assumption here is that steady_clock will be the lesser expensive
  // to use. Only fall-back to high_resolution_clock if steady_clock does not
  // meet the resolution requirement.
  //
  // Note: Most of the expression below should be reduced at compile-time (by
  // any half-decent optimizing compiler), and so there won't be any branching
  // or significant math actually taking place here.
  if (can_use_steady_clock) {
    return Clock::time_point(
        Clock::to_duration(steady_clock::now().time_since_epoch()));
  }
  return Clock::time_point(
      Clock::to_duration(high_resolution_clock::now().time_since_epoch()));
}

std::chrono::seconds GetWallTimeSinceUnixEpoch() noexcept {
  // Note: Even though std::time_t is not necessarily "seconds since UNIX epoch"
  // before C++20, it is almost universally implemented that way on all
  // platforms. There is a unit test to confirm this behavior, so don't worry
  // about it here.
  const std::time_t since_epoch = system_clock::to_time_t(system_clock::now());

  // std::time_t is unspecified by the spec. If it's only a 32-bit integer, it's
  // possible that values will overflow in early 2038. Warn future developers a
  // year ahead of time.
  if (sizeof(std::time_t) <= 4) {
    constexpr std::time_t a_year_before_overflow =
        std::numeric_limits<std::time_t>::max() -
        to_seconds(365 * hours(24)).count();
    OSP_DCHECK_LE(since_epoch, a_year_before_overflow);
  }

  return std::chrono::seconds(since_epoch);
}

}  // namespace openscreen
