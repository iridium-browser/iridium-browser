// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/test/fake_task_runner.h"

#include <utility>

#include "util/osp_logging.h"

namespace openscreen {

FakeTaskRunner::FakeTaskRunner(FakeClock* clock) : clock_(clock) {
  OSP_CHECK(clock_);
  clock_->SubscribeToTimeChanges(this);
}

FakeTaskRunner::~FakeTaskRunner() {
  clock_->UnsubscribeFromTimeChanges(this);
}

void FakeTaskRunner::RunTasksUntilIdle() {
  // If there is bad code that posts tasks indefinitely, this loop will never
  // break. However, that also means there is a code path spinning a CPU core at
  // 100% all the time. Rather than mitigate this problem scenario, purposely
  // let it manifest here in the hopes that unit testing will reveal it (e.g., a
  // unit test that never finishes running).
  for (;;) {
    const auto current_time = FakeClock::now();
    const auto end_of_range = delayed_tasks_.upper_bound(current_time);
    for (auto it = delayed_tasks_.begin(); it != end_of_range; ++it) {
      ready_to_run_tasks_.push_back(std::move(it->second));
    }
    delayed_tasks_.erase(delayed_tasks_.begin(), end_of_range);

    if (ready_to_run_tasks_.empty()) {
      break;
    }

    std::vector<Task> running_tasks;
    running_tasks.swap(ready_to_run_tasks_);
    for (Task& running_task : running_tasks) {
      // Move the task out of the vector and onto the stack so that it destroys
      // just after being run. This helps catch "dangling reference/pointer"
      // bugs.
      Task task = std::move(running_task);
      task();
    }
  }
}

void FakeTaskRunner::PostPackagedTask(Task task) {
  ready_to_run_tasks_.push_back(std::move(task));
}

void FakeTaskRunner::PostPackagedTaskWithDelay(Task task,
                                               Clock::duration delay) {
  delayed_tasks_.emplace(
      std::make_pair(FakeClock::now() + delay, std::move(task)));
}

bool FakeTaskRunner::IsRunningOnTaskRunner() {
  return true;
}

Clock::time_point FakeTaskRunner::GetResumeTime() const {
  if (!ready_to_run_tasks_.empty()) {
    return FakeClock::now();
  }
  if (!delayed_tasks_.empty()) {
    return delayed_tasks_.begin()->first;
  }
  return Clock::time_point::max();
}

}  // namespace openscreen
