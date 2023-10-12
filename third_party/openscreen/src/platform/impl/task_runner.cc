// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/task_runner.h"

#include <csignal>
#include <thread>

#include "util/osp_logging.h"

namespace openscreen {

namespace {

// This is mutated by the signal handler installed by RunUntilSignaled(), and is
// checked by RunUntilStopped().
//
// Per the C++14 spec, passing visible changes to memory between a signal
// handler and a program thread must be done through a volatile variable.
volatile enum {
  kNotRunning,
  kNotSignaled,
  kSignaled
} g_signal_state = kNotRunning;

void OnReceivedSignal(int signal) {
  g_signal_state = kSignaled;
}

}  // namespace

TaskRunnerImpl::TaskRunnerImpl(ClockNowFunctionPtr now_function,
                               TaskWaiter* event_waiter,
                               Clock::duration waiter_timeout)
    : now_function_(now_function),
      is_running_(false),
      task_waiter_(event_waiter),
      waiter_timeout_(waiter_timeout) {}

TaskRunnerImpl::~TaskRunnerImpl() {
  // Ensure no thread is currently executing inside RunUntilStopped().
  OSP_DCHECK_EQ(task_runner_thread_id_, std::thread::id());
}

void TaskRunnerImpl::PostPackagedTask(Task task) {
  std::lock_guard<std::mutex> lock(task_mutex_);
  tasks_.emplace_back(std::move(task));
  if (task_waiter_) {
    task_waiter_->OnTaskPosted();
  } else {
    run_loop_wakeup_.notify_one();
  }
}

void TaskRunnerImpl::PostPackagedTaskWithDelay(Task task,
                                               Clock::duration delay) {
  std::lock_guard<std::mutex> lock(task_mutex_);
  if (delay <= Clock::duration::zero()) {
    tasks_.emplace_back(std::move(task));
  } else {
    delayed_tasks_.emplace(
        std::make_pair(now_function_() + delay, std::move(task)));
  }
  if (task_waiter_) {
    task_waiter_->OnTaskPosted();
  } else {
    run_loop_wakeup_.notify_one();
  }
}

bool TaskRunnerImpl::IsRunningOnTaskRunner() {
  return task_runner_thread_id_ == std::this_thread::get_id();
}

void TaskRunnerImpl::RunUntilStopped() {
  OSP_DCHECK(!is_running_);
  task_runner_thread_id_ = std::this_thread::get_id();
  is_running_ = true;

  OSP_DVLOG << "Running tasks until stopped...";
  // Main loop: Run until the |is_running_| flag is set back to false by the
  // "quit task" posted by RequestStopSoon(), or the process received a
  // termination signal.
  while (is_running_) {
    ScheduleDelayedTasks();
    if (GrabMoreRunnableTasks()) {
      RunRunnableTasks();
    }
    if (g_signal_state == kSignaled) {
      is_running_ = false;
    }
  }

  OSP_DVLOG << "Finished running, entering flushing phase...";
  // Flushing phase: Ensure all immediately-runnable tasks are run before
  // returning. Since running some tasks might cause more immediately-runnable
  // tasks to be posted, loop until there is no more work.
  //
  // If there is bad code that posts tasks indefinitely, this loop will never
  // break. However, that also means there is a code path spinning a CPU core at
  // 100% all the time. Rather than mitigate this problem scenario, purposely
  // let it manifest here in the hopes that unit testing will reveal it (e.g., a
  // unit test that never finishes running).
  while (GrabMoreRunnableTasks()) {
    RunRunnableTasks();
  }
  OSP_DVLOG << "Finished flushing...";
  task_runner_thread_id_ = std::thread::id();
}

void TaskRunnerImpl::RunUntilSignaled() {
  OSP_CHECK_EQ(g_signal_state, kNotRunning)
      << __func__ << " may not be invoked concurrently.";
  g_signal_state = kNotSignaled;
  const auto old_sigint_handler = std::signal(SIGINT, &OnReceivedSignal);
  const auto old_sigterm_handler = std::signal(SIGTERM, &OnReceivedSignal);

  RunUntilStopped();

  std::signal(SIGINT, old_sigint_handler);
  std::signal(SIGTERM, old_sigterm_handler);
  OSP_DVLOG << "Received SIGNIT or SIGTERM, setting state to not running...";
  g_signal_state = kNotRunning;
}

void TaskRunnerImpl::RequestStopSoon() {
  PostTask([this]() { is_running_ = false; });
}

void TaskRunnerImpl::RunRunnableTasks() {
  for (TaskWithMetadata& running_task : running_tasks_) {
    // Move the task to the stack so that its bound state is freed immediately
    // after being run.
    TaskWithMetadata task = std::move(running_task);
    task();
  }
  running_tasks_.clear();
}

void TaskRunnerImpl::ScheduleDelayedTasks() {
  std::lock_guard<std::mutex> lock(task_mutex_);

  // Getting the time can be expensive on some platforms, so only get it once.
  const auto current_time = now_function_();
  const auto end_of_range = delayed_tasks_.upper_bound(current_time);
  for (auto it = delayed_tasks_.begin(); it != end_of_range; ++it) {
    tasks_.push_back(std::move(it->second));
  }
  delayed_tasks_.erase(delayed_tasks_.begin(), end_of_range);
}

bool TaskRunnerImpl::GrabMoreRunnableTasks() {
  OSP_DCHECK(running_tasks_.empty());

  std::unique_lock<std::mutex> lock(task_mutex_);
  if (!tasks_.empty()) {
    running_tasks_.swap(tasks_);
    return true;
  }

  if (!is_running_) {
    return false;  // Stop was requested. Don't wait for more tasks.
  }

  if (task_waiter_) {
    Clock::duration timeout = waiter_timeout_;
    if (!delayed_tasks_.empty()) {
      Clock::duration next_task_delta =
          delayed_tasks_.begin()->first - now_function_();
      if (next_task_delta < timeout) {
        timeout = next_task_delta;
      }
    }
    lock.unlock();
    task_waiter_->WaitForTaskToBePosted(timeout);
    return false;
  }

  if (delayed_tasks_.empty()) {
    run_loop_wakeup_.wait(lock);
  } else {
    run_loop_wakeup_.wait_for(lock,
                              delayed_tasks_.begin()->first - now_function_());
  }
  return false;
}

}  // namespace openscreen
