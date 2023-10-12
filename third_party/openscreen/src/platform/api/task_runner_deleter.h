// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_API_TASK_RUNNER_DELETER_H_
#define PLATFORM_API_TASK_RUNNER_DELETER_H_

#include "platform/api/task_runner.h"

namespace openscreen {

// Helper that deletes an object on the provided TaskRunner.
//
// Usage with std::unique_ptr:
//
// std::unique_ptr<Foo, TaskRunnerDeleter> some_foo;
// ...
// some_foo = std::unique_ptr<Foo, TaskRunnerDeleter>(
//    new Foo(),
//    TaskRunnerDeleter(task_runner));
struct TaskRunnerDeleter {
  // Exists for backwards compatibility with SerialDeletePtr; can be removed
  // with SerialDeletePtr.
  TaskRunnerDeleter();
  explicit TaskRunnerDeleter(TaskRunner& task_runner);
  ~TaskRunnerDeleter();

  TaskRunnerDeleter(const TaskRunnerDeleter&);
  TaskRunnerDeleter& operator=(const TaskRunnerDeleter&);
  TaskRunnerDeleter(TaskRunnerDeleter&&) noexcept;
  TaskRunnerDeleter& operator=(TaskRunnerDeleter&&) noexcept;

  // For compatibility with std:: deleters.
  template <typename T>
  void operator()(const T* ptr) {
    if (task_runner_ && ptr)
      task_runner_->DeleteSoon(ptr);
  }

  TaskRunner* task_runner_{nullptr};
};

}  // namespace openscreen

#endif  // PLATFORM_API_TASK_RUNNER_DELETER_H_
