// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/task_runner.h"

#include <unistd.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/api/time.h"
#include "platform/test/fake_clock.h"
#include "util/chrono_helpers.h"
namespace openscreen {
namespace {

using ::testing::_;

const auto kTaskRunnerSleepTime = milliseconds(1);
constexpr Clock::duration kWaitTimeout = milliseconds(1000);

void WaitUntilCondition(std::function<bool()> predicate) {
  while (!predicate()) {
    std::this_thread::sleep_for(kTaskRunnerSleepTime);
  }
}

class FakeTaskWaiter final : public TaskRunnerImpl::TaskWaiter {
 public:
  explicit FakeTaskWaiter(ClockNowFunctionPtr now_function)
      : now_function_(now_function) {}
  ~FakeTaskWaiter() override = default;

  Error WaitForTaskToBePosted(Clock::duration timeout) override {
    Clock::time_point start = now_function_();
    waiting_.store(true);
    while (!has_event_.load() && (now_function_() - start) < timeout) {
      EXPECT_EQ(usleep(100 /* microseconds */), 0);
    }
    waiting_.store(false);
    has_event_.store(false);
    return Error::None();
  }

  void OnTaskPosted() override { has_event_.store(true); }

  void WakeUpAndStop() {
    OnTaskPosted();
    task_runner_->RequestStopSoon();
  }

  bool IsWaiting() const { return waiting_.load(); }

  void SetTaskRunner(TaskRunnerImpl* task_runner) {
    task_runner_ = task_runner;
  }

 private:
  const ClockNowFunctionPtr now_function_;
  TaskRunnerImpl* task_runner_;
  std::atomic<bool> has_event_{false};
  std::atomic<bool> waiting_{false};
};

class TaskRunnerWithWaiterFactory {
 public:
  static std::unique_ptr<TaskRunnerImpl> Create(
      ClockNowFunctionPtr now_function) {
    fake_waiter = std::make_unique<FakeTaskWaiter>(now_function);
    auto runner = std::make_unique<TaskRunnerImpl>(
        now_function, fake_waiter.get(), std::chrono::hours(1));
    fake_waiter->SetTaskRunner(runner.get());
    return runner;
  }

  static std::unique_ptr<FakeTaskWaiter> fake_waiter;
};

// static
std::unique_ptr<FakeTaskWaiter> TaskRunnerWithWaiterFactory::fake_waiter;

}  // anonymous namespace

TEST(TaskRunnerImplTest, TaskRunnerExecutesTaskAndStops) {
  FakeClock fake_clock{Clock::time_point(milliseconds(1337))};
  TaskRunnerImpl runner(&fake_clock.now);

  std::string ran_tasks = "";
  runner.PostTask([&ran_tasks] { ran_tasks += "1"; });
  runner.RequestStopSoon();

  runner.RunUntilStopped();
  EXPECT_EQ(ran_tasks, "1");
}

TEST(TaskRunnerImplTest, TaskRunnerRunsDelayedTasksInOrder) {
  FakeClock fake_clock{Clock::time_point(milliseconds(1337))};
  TaskRunnerImpl runner(&fake_clock.now);

  std::thread t([&runner] { runner.RunUntilStopped(); });

  std::string ran_tasks = "";

  const auto kDelayTime = milliseconds(5);
  const auto task_one = [&ran_tasks] { ran_tasks += "1"; };
  runner.PostTaskWithDelay(task_one, kDelayTime);

  const auto task_two = [&ran_tasks] { ran_tasks += "2"; };
  runner.PostTaskWithDelay(task_two, kDelayTime * 2);

  EXPECT_EQ(ran_tasks, "");
  fake_clock.Advance(kDelayTime);
  WaitUntilCondition([&ran_tasks] { return ran_tasks == "1"; });
  EXPECT_EQ(ran_tasks, "1");

  fake_clock.Advance(kDelayTime);
  WaitUntilCondition([&ran_tasks] { return ran_tasks == "12"; });
  EXPECT_EQ(ran_tasks, "12");

  runner.RequestStopSoon();
  t.join();
}

TEST(TaskRunnerImplTest, SingleThreadedTaskRunnerRunsSequentially) {
  FakeClock fake_clock{Clock::time_point(milliseconds(1337))};
  TaskRunnerImpl runner(&fake_clock.now);

  std::string ran_tasks;
  const auto task_one = [&ran_tasks] { ran_tasks += "1"; };
  const auto task_two = [&ran_tasks] { ran_tasks += "2"; };
  const auto task_three = [&ran_tasks] { ran_tasks += "3"; };
  const auto task_four = [&ran_tasks] { ran_tasks += "4"; };
  const auto task_five = [&ran_tasks] { ran_tasks += "5"; };

  runner.PostTask(task_one);
  runner.PostTask(task_two);
  runner.PostTask(task_three);
  runner.PostTask(task_four);
  runner.PostTask(task_five);
  runner.RequestStopSoon();
  EXPECT_EQ(ran_tasks, "");

  runner.RunUntilStopped();
  EXPECT_EQ(ran_tasks, "12345");
}

TEST(TaskRunnerImplTest, RunsAllImmediateTasksBeforeStopping) {
  FakeClock fake_clock{Clock::time_point(milliseconds(1337))};
  TaskRunnerImpl runner(&fake_clock.now);

  std::string result;
  runner.PostTask([&] {
    result += "Alice";

    // Post a task that runs just before the quit task.
    runner.PostTask([&] {
      result += " says goodbye";

      // These tasks will enter the queue after the quit task *and* after the
      // main loop breaks. They will be executed by the flushing phase.
      runner.PostTask([&] {
        result += " and is not";
        runner.PostTask([&] { result += " forgotten."; });
      });
    });

    // Post the quit task.
    runner.RequestStopSoon();
  });

  EXPECT_EQ(result, "");
  runner.RunUntilStopped();
  // All posted tasks will execute because RequestStopSoon() guarantees all
  // immediately-runnable tasks will run before exiting, even if new
  // immediately-runnable tasks are posted in the meantime.
  EXPECT_EQ(result, "Alice says goodbye and is not forgotten.");
}

TEST(TaskRunnerImplTest, TaskRunnerIsStableWithLotsOfTasks) {
  FakeClock fake_clock{Clock::time_point(milliseconds(1337))};
  TaskRunnerImpl runner(&fake_clock.now);

  const int kNumberOfTasks = 500;
  std::string expected_ran_tasks;
  expected_ran_tasks.append(kNumberOfTasks, '1');

  std::string ran_tasks;
  for (int i = 0; i < kNumberOfTasks; ++i) {
    const auto task = [&ran_tasks] { ran_tasks += "1"; };
    runner.PostTask(task);
  }

  runner.RequestStopSoon();
  runner.RunUntilStopped();
  EXPECT_EQ(ran_tasks, expected_ran_tasks);
}

TEST(TaskRunnerImplTest, TaskRunnerDelayedTasksDontBlockImmediateTasks) {
  TaskRunnerImpl runner(Clock::now);

  std::string ran_tasks;
  const auto task = [&ran_tasks] { ran_tasks += "1"; };
  const auto delayed_task = [&ran_tasks] { ran_tasks += "A"; };

  runner.PostTaskWithDelay(delayed_task, milliseconds(10000));
  runner.PostTask(task);

  runner.RequestStopSoon();
  runner.RunUntilStopped();
  // The immediate task should have run, even though the delayed task
  // was added first.

  EXPECT_EQ(ran_tasks, "1");
}

TEST(TaskRunnerImplTest, TaskRunnerUsesEventWaiter) {
  std::unique_ptr<TaskRunnerImpl> runner =
      TaskRunnerWithWaiterFactory::Create(Clock::now);

  std::atomic<int> x{0};
  std::thread t([&runner, &x] {
    runner.get()->RunUntilStopped();
    x = 1;
  });

  const Clock::time_point start1 = Clock::now();
  FakeTaskWaiter* fake_waiter = TaskRunnerWithWaiterFactory::fake_waiter.get();
  while ((Clock::now() - start1) < kWaitTimeout && !fake_waiter->IsWaiting()) {
    std::this_thread::sleep_for(kTaskRunnerSleepTime);
  }
  ASSERT_TRUE(fake_waiter->IsWaiting());

  fake_waiter->WakeUpAndStop();
  const Clock::time_point start2 = Clock::now();
  while ((Clock::now() - start2) < kWaitTimeout && x == 0) {
    std::this_thread::sleep_for(kTaskRunnerSleepTime);
  }
  ASSERT_EQ(x, 1);
  ASSERT_FALSE(fake_waiter->IsWaiting());
  t.join();
}

TEST(TaskRunnerImplTest, WakesEventWaiterOnPostTask) {
  std::unique_ptr<TaskRunnerImpl> runner =
      TaskRunnerWithWaiterFactory::Create(Clock::now);

  std::atomic<int> x{0};
  std::thread t([&runner] { runner.get()->RunUntilStopped(); });

  const Clock::time_point start1 = Clock::now();
  FakeTaskWaiter* fake_waiter = TaskRunnerWithWaiterFactory::fake_waiter.get();
  while ((Clock::now() - start1) < kWaitTimeout && !fake_waiter->IsWaiting()) {
    std::this_thread::sleep_for(kTaskRunnerSleepTime);
  }
  ASSERT_TRUE(fake_waiter->IsWaiting());

  runner->PostTask([&x]() { x = 1; });
  const Clock::time_point start2 = Clock::now();
  while ((Clock::now() - start2) < kWaitTimeout && x == 0) {
    std::this_thread::sleep_for(kTaskRunnerSleepTime);
  }
  ASSERT_EQ(x, 1);

  fake_waiter->WakeUpAndStop();
  t.join();
}

class RepeatedClass {
 public:
  MOCK_METHOD0(Repeat, absl::optional<Clock::duration>());

  absl::optional<Clock::duration> DoCall() {
    auto result = Repeat();
    execution_count++;
    return result;
  }

  std::atomic<int> execution_count{0};
};

}  // namespace openscreen
