// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/test/fake_clock.h"

#include <algorithm>

#include "platform/base/trivial_clock_traits.h"
#include "platform/test/fake_task_runner.h"
#include "util/osp_logging.h"
#include "util/std_util.h"

namespace openscreen {
namespace {

constexpr Clock::time_point kInvalid = Clock::time_point::min();
}

using clock_operators::operator<<;

FakeClock::FakeClock(Clock::time_point start_time)
    : control_thread_id_(std::this_thread::get_id()) {
  OSP_CHECK_EQ(now_.load(std::memory_order_acquire), kInvalid)
      << "Only one FakeClock instance allowed!";
  now_.store(start_time, std::memory_order_release);
}

FakeClock::~FakeClock() {
  OSP_CHECK_EQ(std::this_thread::get_id(), control_thread_id_);
  OSP_CHECK(task_runners_.empty());
  // Set |now_| to kInvalid to flag that this FakeClock has been destroyed.
  now_.store(kInvalid, std::memory_order_release);
}

Clock::time_point FakeClock::now() {
  const Clock::time_point value = now_.load(std::memory_order_acquire);
  OSP_CHECK_NE(value, kInvalid) << "No FakeClock instance!";
  return value;
}

void FakeClock::Advance(Clock::duration delta) {
  OSP_CHECK_EQ(std::this_thread::get_id(), control_thread_id_);

  const Clock::time_point stop_time = now() + delta;

  for (;;) {
    // Run tasks at the current time, since this might cause additional delayed
    // tasks to be posted.
    for (FakeTaskRunner* task_runner : task_runners_) {
      task_runner->RunTasksUntilIdle();
    }

    // Find the next "step-to" time, and advance the clock to that point.
    Clock::time_point step_to = Clock::time_point::max();
    for (FakeTaskRunner* task_runner : task_runners_) {
      step_to = std::min(step_to, task_runner->GetResumeTime());
    }
    if (step_to > stop_time) {
      break;  // No tasks are scheduled for the remaining time range.
    }

    OSP_DCHECK_GT(step_to, now());
    now_.store(step_to, std::memory_order_release);
  }

  // Skip over any remaining "dead time."
  now_.store(stop_time, std::memory_order_release);
}

void FakeClock::SubscribeToTimeChanges(FakeTaskRunner* task_runner) {
  OSP_CHECK_EQ(std::this_thread::get_id(), control_thread_id_);
  OSP_CHECK(!Contains(task_runners_, task_runner));
  task_runners_.push_back(task_runner);
}

void FakeClock::UnsubscribeFromTimeChanges(FakeTaskRunner* task_runner) {
  OSP_CHECK_EQ(std::this_thread::get_id(), control_thread_id_);
  auto it = std::find(task_runners_.begin(), task_runners_.end(), task_runner);
  OSP_CHECK(it != task_runners_.end());
  task_runners_.erase(it);
}

// static
std::atomic<Clock::time_point> FakeClock::now_{kInvalid};

}  // namespace openscreen
