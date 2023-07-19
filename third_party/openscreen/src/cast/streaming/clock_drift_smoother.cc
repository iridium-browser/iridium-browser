// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/clock_drift_smoother.h"

#include <cmath>

#include "util/chrono_helpers.h"
#include "util/osp_logging.h"
#include "util/saturate_cast.h"

namespace openscreen {
namespace cast {
namespace {

constexpr Clock::time_point kNullTime = Clock::time_point::min();
}

using clock_operators::operator<<;

ClockDriftSmoother::ClockDriftSmoother(Clock::duration time_constant)
    : time_constant_(time_constant),
      last_update_time_(kNullTime),
      estimated_tick_offset_(0.0) {
  OSP_DCHECK(time_constant_ > decltype(time_constant_)::zero());
}

ClockDriftSmoother::~ClockDriftSmoother() = default;

Clock::duration ClockDriftSmoother::Current() const {
  OSP_DCHECK(last_update_time_ != kNullTime);
  return Clock::duration(
      rounded_saturate_cast<Clock::duration::rep>(estimated_tick_offset_));
}

void ClockDriftSmoother::Reset(Clock::time_point now,
                               Clock::duration measured_offset) {
  OSP_DCHECK(now != kNullTime);
  last_update_time_ = now;
  estimated_tick_offset_ = static_cast<double>(measured_offset.count());
}

void ClockDriftSmoother::Update(Clock::time_point now,
                                Clock::duration measured_offset) {
  OSP_DCHECK(now != kNullTime);
  if (last_update_time_ == kNullTime) {
    Reset(now, measured_offset);
  } else if (now < last_update_time_) {
    // |now| is not monotonically non-decreasing.
    OSP_NOTREACHED();
  } else {
    const double elapsed_ticks =
        static_cast<double>((now - last_update_time_).count());
    last_update_time_ = now;
    // Compute a weighted-average between the last estimate and
    // |measured_offset|. The more time that has elasped since the last call to
    // Update(), the more-heavily |measured_offset| will be weighed.
    const double weight =
        elapsed_ticks / (elapsed_ticks + time_constant_.count());
    estimated_tick_offset_ =
        weight * static_cast<double>(measured_offset.count()) +
        (1.0 - weight) * estimated_tick_offset_;

    // If after calculation the current offset is lower than the weighted
    // average, we can simply use it and eliminate some of the error due to
    // transmission time.
    if (measured_offset < Current()) {
      Reset(now, measured_offset);
    }

    OSP_VLOG << "Local clock is ahead of the remote clock by: measured = "
             << measured_offset << ", "
             << "filtered = " << Current() << ".";
  }
}

// static
constexpr std::chrono::seconds ClockDriftSmoother::kDefaultTimeConstant;

}  // namespace cast
}  // namespace openscreen
