// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_ALARM_H_
#define UTIL_ALARM_H_

#include <utility>

#include "platform/api/task_runner.h"
#include "platform/api/time.h"

namespace openscreen {

// A simple mechanism for running one Task in the future, but also allow for
// canceling the Task before it runs and/or re-scheduling a replacement Task to
// run at a different time. This mechanism is also scoped to its lifetime: if an
// Alarm is destroyed while it is scheduled, the Task is automatically canceled.
// It is safe for the client's Task to make re-entrant calls into all Alarm
// methods.
//
// Example use case: When using a TaskRunner, an object can safely schedule a
// callback into one of its instance methods (without the possibility of the
// Task executing after the object is destroyed).
//
// Design: In order to support efficient, arbitrary canceling and re-scheduling
// by the client, the Alarm posts a cancelable functor to the TaskRunner which,
// when invoked, then checks to see whether the Alarm instance still exists and,
// if so, calls its TryInvoke() method. The TryInvoke() method then determines:
// a) whether the invocation time of the client's Task has changed; and b)
// whether the Alarm was canceled in the meantime. From this, it either: a) does
// nothing; b) re-posts a new cancelable functor to the TaskRunner, to try
// running the client's Task later; or c) runs the client's Task.
class Alarm {
 public:
  Alarm(ClockNowFunctionPtr now_function, TaskRunner& task_runner);
  ~Alarm();

  // The design requires that Alarm instances not be copied or moved.
  Alarm(const Alarm&) = delete;
  Alarm& operator=(const Alarm&) = delete;
  Alarm(Alarm&&) noexcept = delete;
  Alarm& operator=(Alarm&&) noexcept = delete;

  // Schedule the |functor| to be invoked at |alarm_time|. If this Alarm was
  // already scheduled, the prior scheduling is canceled. The Functor can be any
  // callable target (e.g., function, lambda-expression, std::bind result,
  // etc.). If |alarm_time| is on or before "now," such as kImmediately, it is
  // scheduled to run as soon as possible.
  template <typename Functor>
  inline void Schedule(Functor functor, Clock::time_point alarm_time) {
    ScheduleWithTask(TaskRunner::Task(std::move(functor)), alarm_time);
  }

  // Same as Schedule(), but invoke the functor at the given |delay| after right
  // now.
  template <typename Functor>
  inline void ScheduleFromNow(Functor functor, Clock::duration delay) {
    ScheduleWithTask(TaskRunner::Task(std::move(functor)),
                     now_function_() + delay);
  }

  // Cancels an already-scheduled task from running, or no-op.
  void Cancel();

  // See comments for Schedule(). Generally, callers will want to call
  // Schedule() instead of this, for more-convenient caller-side syntax, unless
  // they already have a Task to pass-in.
  void ScheduleWithTask(TaskRunner::Task task, Clock::time_point alarm_time);

  // A special time_point value representing "as soon as possible."
  static constexpr Clock::time_point kImmediately = Clock::time_point::min();

 private:
  // A move-only functor that holds a raw pointer back to |this| and can be
  // canceled before its call operator is invoked. When canceled, its call
  // operator becomes a no-op.
  class CancelableFunctor;

  // Posts a delayed call to TryInvoke() to the TaskRunner.
  void InvokeLater(Clock::time_point now, Clock::time_point fire_time);

  // Examines whether to invoke the client's Task now; or try again later; or
  // just do nothing. See class-level design comments.
  void TryInvoke();

  const ClockNowFunctionPtr now_function_;
  TaskRunner& task_runner_;

  // This is the task the client wants to have run at a specific point-in-time.
  // This is NOT the task that Alarm provides to the TaskRunner.
  TaskRunner::Task scheduled_task_;
  Clock::time_point alarm_time_{};

  // When non-null, there is a task in the TaskRunner's queue that will call
  // TryInvoke() some time in the future. This member is exclusively maintained
  // by the CancelableFunctor class methods.
  CancelableFunctor* queued_fire_ = nullptr;

  // When the CancelableFunctor is scheduled to run. It may possibly execute
  // later than this, if the TaskRunner is falling behind.
  Clock::time_point next_fire_time_{};
};

}  // namespace openscreen

#endif  // UTIL_ALARM_H_
