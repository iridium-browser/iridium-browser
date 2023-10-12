// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_TEST_FAKE_TASK_RUNNER_H_
#define PLATFORM_TEST_FAKE_TASK_RUNNER_H_

#include <map>
#include <vector>

#include "platform/api/task_runner.h"
#include "platform/api/time.h"
#include "platform/test/fake_clock.h"

namespace openscreen {

// Usage:
//
//   #include ".../gtest.h"
//
//   class FooTest : public testing::Test {
//    public:
//     FakeClock* clock() { return &clock_; }
//     FakeTaskRunner* task_runner() { return &task_runner_; }
//
//    private:
//     FakeClock clock_{Clock::now()};
//     FakeTaskRunner task_runner_{&clock_};
//   };
//
//   TEST_F(FooTest, RunsTask) {
//     Foo foo(task_runner());
//     foo.DoSomethingToPostATask();
//     task_runner()->RunTasksUntilIdle();
//     // Alternatively: clock()->Advance(std::chrono::seconds(0));
//   }
//
//   TEST_F(FooTest, RunsDelayedTask) {
//     Foo foo(task_runner());
//     foo.DoSomethingInOneSecond();  // Schedules 1-second delayed task.
//     clock()->Advance(std::chrono::seconds(3));  // Delayed Task runs here!
//   }
class FakeTaskRunner : public TaskRunner {
 public:
  using Task = TaskRunner::Task;

  explicit FakeTaskRunner(FakeClock* clock);
  ~FakeTaskRunner() override;

  // Runs all ready-to-run tasks.
  void RunTasksUntilIdle();

  // TaskRunner implementation.
  void PostPackagedTask(Task task) override;
  void PostPackagedTaskWithDelay(Task task, Clock::duration delay) override;
  bool IsRunningOnTaskRunner() override;

  int ready_task_count() const { return ready_to_run_tasks_.size(); }
  int delayed_task_count() const { return delayed_tasks_.size(); }

  // Returns the time at which the next task is scheduled to run, or
  // Clock::time_point::max() if there is none scheduled.
  Clock::time_point GetResumeTime() const;

 private:
  FakeClock* const clock_;

  std::vector<Task> ready_to_run_tasks_;
  std::multimap<Clock::time_point, Task> delayed_tasks_;
};

}  // namespace openscreen

#endif  // PLATFORM_TEST_FAKE_TASK_RUNNER_H_
