// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_TEST_FAKE_CLOCK_H_
#define PLATFORM_TEST_FAKE_CLOCK_H_

#include <atomic>
#include <thread>
#include <vector>

#include "platform/api/time.h"

namespace openscreen {

class FakeTaskRunner;

class FakeClock {
 public:
  explicit FakeClock(Clock::time_point start_time);
  ~FakeClock();

  // Moves the clock forward, simulating execution by running tasks in all
  // FakeTaskRunners. The clock advances in one or more jumps, to ensure delayed
  // tasks see the correct "now time" when they are executed.
  void Advance(Clock::duration delta);

  static Clock::time_point now();

 protected:
  friend class FakeTaskRunner;

  void SubscribeToTimeChanges(FakeTaskRunner* task_runner);
  void UnsubscribeFromTimeChanges(FakeTaskRunner* task_runner);

 private:
  // Used for ensuring this FakeClock's lifecycle and mutations occur on the
  // same thread.
  const std::thread::id control_thread_id_;

  std::vector<FakeTaskRunner*> task_runners_;

  // This variable is atomic because some tests (e.g., TaskRunnerImplTest) will
  // call now() on a different thread.
  static std::atomic<Clock::time_point> now_;
};

}  // namespace openscreen

#endif  // PLATFORM_TEST_FAKE_CLOCK_H_
