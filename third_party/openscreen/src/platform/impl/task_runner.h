// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_TASK_RUNNER_H_
#define PLATFORM_IMPL_TASK_RUNNER_H_

#include <condition_variable>  // NOLINT
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "absl/base/thread_annotations.h"
#include "absl/types/optional.h"
#include "platform/api/task_runner.h"
#include "platform/api/time.h"
#include "platform/base/error.h"
#include "util/trace_logging.h"

namespace openscreen {

class TaskRunnerImpl final : public TaskRunner {
 public:
  using Task = TaskRunner::Task;

  class TaskWaiter {
   public:
    virtual ~TaskWaiter() = default;

    // These calls should be thread-safe.  The absolute minimum is that
    // OnTaskPosted must be safe to call from another thread while this is
    // inside WaitForTaskToBePosted.  NOTE: There may be spurious wakeups from
    // WaitForTaskToBePosted depending on whether the specific implementation
    // chooses to clear queued WakeUps before entering WaitForTaskToBePosted.

    // Blocks until some event occurs, which means new tasks may have been
    // posted.  Wait may only block up to |timeout| where 0 means don't block at
    // all (not block forever).
    virtual Error WaitForTaskToBePosted(Clock::duration timeout) = 0;

    // If a WaitForTaskToBePosted call is currently blocking, unblock it
    // immediately.
    virtual void OnTaskPosted() = 0;
  };

  explicit TaskRunnerImpl(
      ClockNowFunctionPtr now_function,
      TaskWaiter* event_waiter = nullptr,
      Clock::duration waiter_timeout = std::chrono::milliseconds(100));

  // TaskRunner overrides
  ~TaskRunnerImpl() final;
  void PostPackagedTask(Task task) final;
  void PostPackagedTaskWithDelay(Task task, Clock::duration delay) final;
  bool IsRunningOnTaskRunner() final;

  // Blocks the current thread, executing tasks from the queue with the desired
  // timing; and does not return until some time after RequestStopSoon() is
  // called.
  void RunUntilStopped();

  // Blocks the current thread, executing tasks from the queue with the desired
  // timing; and does not return until some time after the current process is
  // signaled with SIGINT or SIGTERM, or after RequestStopSoon() is called.
  void RunUntilSignaled();

  // Thread-safe method for requesting the TaskRunner to stop running after all
  // non-delayed tasks in the queue have run. This behavior allows final
  // clean-up tasks to be executed before the TaskRunner stops.
  //
  // If any non-delayed tasks post additional non-delayed tasks, those will be
  // run as well before returning.
  void RequestStopSoon();

 private:
#if defined(ENABLE_TRACE_LOGGING)
  // Wrapper around a Task used to store the TraceId Metadata along with the
  // task itself, and to set the current TraceIdHierarchy before executing the
  // task.
  class TaskWithMetadata {
   public:
    // NOTE: 'explicit' keyword omitted so that conversion construtor can be
    // used. This simplifies switching between 'Task' and 'TaskWithMetadata'
    // based on the compilation flag.
    TaskWithMetadata(Task task)  // NOLINT
        : task_(std::move(task)), trace_ids_(TRACE_HIERARCHY) {}

    void operator()() {
      TRACE_SET_HIERARCHY(trace_ids_);
      std::move(task_)();
    }

   private:
    Task task_;
    TraceIdHierarchy trace_ids_;
  };
#else   // !defined(ENABLE_TRACE_LOGGING)
  using TaskWithMetadata = Task;
#endif  // defined(ENABLE_TRACE_LOGGING)

  // Helper that runs all tasks in |running_tasks_| and then clears it.
  void RunRunnableTasks();

  // Look at all tasks in the delayed task queue, then schedule them if the
  // minimum delay time has elapsed.
  void ScheduleDelayedTasks();

  // Transfers all ready-to-run tasks from |tasks_| to |running_tasks_|. If
  // there are no ready-to-run tasks, and |is_running_| is true, this method
  // will block waiting for new tasks. Returns true if any tasks were
  // transferred.
  bool GrabMoreRunnableTasks();

  const ClockNowFunctionPtr now_function_;

  // Flag that indicates whether the task runner loop should continue. This is
  // only meant to be read/written on the thread executing RunUntilStopped().
  bool is_running_;

  // This mutex is used for |tasks_| and |delayed_tasks_|, and also for
  // notifying the run loop to wake up when it is waiting for a task to be added
  // to the queue in |run_loop_wakeup_|.
  std::mutex task_mutex_;
  std::vector<TaskWithMetadata> tasks_ ABSL_GUARDED_BY(task_mutex_);
  std::multimap<Clock::time_point, TaskWithMetadata> delayed_tasks_
      ABSL_GUARDED_BY(task_mutex_);

  // When |task_waiter_| is nullptr, |run_loop_wakeup_| is used for sleeping the
  // task runner.  Otherwise, |run_loop_wakeup_| isn't used and |task_waiter_|
  // is used instead (along with |waiter_timeout_|).
  std::condition_variable run_loop_wakeup_;
  TaskWaiter* const task_waiter_;
  Clock::duration waiter_timeout_;

  // To prevent excessive re-allocation of the underlying array of the |tasks_|
  // vector, use an A/B vector-swap mechanism. |running_tasks_| starts out
  // empty, and is swapped with |tasks_| when it is time to run the Tasks.
  std::vector<TaskWithMetadata> running_tasks_;

  std::thread::id task_runner_thread_id_;

  OSP_DISALLOW_COPY_AND_ASSIGN(TaskRunnerImpl);
};
}  // namespace openscreen

#endif  // PLATFORM_IMPL_TASK_RUNNER_H_
