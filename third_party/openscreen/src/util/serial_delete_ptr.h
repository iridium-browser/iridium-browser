// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_SERIAL_DELETE_PTR_H_
#define UTIL_SERIAL_DELETE_PTR_H_

#include <cassert>
#include <memory>
#include <utility>

#include "platform/api/task_runner.h"
#include "platform/api/task_runner_deleter.h"

namespace openscreen {

// A specialization of std::unique_ptr<T> that uses TaskRunnerDeleter<T> to
// delete the object asynchronously.
//
// TODO(issuetracker.google.com/288327294): Replace usages with
// std::unique_ptr<T> and TaskRunnerDeleter<T> and delete.
template <typename Type>
class SerialDeletePtr : public std::unique_ptr<Type, TaskRunnerDeleter> {
 public:
  SerialDeletePtr() noexcept
      : std::unique_ptr<Type, TaskRunnerDeleter>(nullptr, TaskRunnerDeleter()) {
  }

  explicit SerialDeletePtr(TaskRunner& task_runner) noexcept
      : std::unique_ptr<Type, TaskRunnerDeleter>(
            nullptr,
            TaskRunnerDeleter(task_runner)) {}

  SerialDeletePtr(TaskRunner& task_runner, Type* pointer) noexcept
      : std::unique_ptr<Type, TaskRunnerDeleter>(
            pointer,
            TaskRunnerDeleter(task_runner)) {}
};

template <typename Type, typename... Args>
SerialDeletePtr<Type> MakeSerialDelete(TaskRunner* task_runner,
                                       Args&&... args) {
  return SerialDeletePtr<Type>(*task_runner,
                               new Type(std::forward<Args>(args)...));
}

}  // namespace openscreen

#endif  // UTIL_SERIAL_DELETE_PTR_H_
