// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_API_SERIAL_DELETE_PTR_H_
#define PLATFORM_API_SERIAL_DELETE_PTR_H_

#include <cassert>
#include <memory>
#include <utility>

#include "platform/api/task_runner.h"

namespace openscreen {

// A functor that deletes an object via a task on a specific TaskRunner. This is
// used for ensuring an object is deleted after any tasks that reference it have
// completed.
template <typename Type, typename DeleterType>
class SerialDelete {
 public:
  SerialDelete() : deleter_() {}

  explicit SerialDelete(TaskRunner* task_runner)
      : task_runner_(task_runner), deleter_() {
    assert(task_runner);
  }

  template <typename DT>
  SerialDelete(TaskRunner* task_runner, DT&& deleter)
      : task_runner_(task_runner), deleter_(std::forward<DT>(deleter)) {
    assert(task_runner);
  }

  void operator()(Type* pointer) const {
    if (task_runner_) {
      // Deletion of the object depends on the task being run by the task
      // runner.
      task_runner_->PostTask(
          [pointer, deleter = std::move(deleter_)] { deleter(pointer); });
    }
  }

 private:
  TaskRunner* task_runner_;
  DeleterType deleter_;
};

// A wrapper around std::unique_ptr<> that uses SerialDelete<> to schedule the
// object's deletion.
template <typename Type, typename DeleterType = std::default_delete<Type>>
class SerialDeletePtr
    : public std::unique_ptr<Type, SerialDelete<Type, DeleterType>> {
 public:
  SerialDeletePtr() noexcept
      : std::unique_ptr<Type, SerialDelete<Type, DeleterType>>(
            nullptr,
            SerialDelete<Type, DeleterType>()) {}

  explicit SerialDeletePtr(TaskRunner* task_runner) noexcept
      : std::unique_ptr<Type, SerialDelete<Type, DeleterType>>(
            nullptr,
            SerialDelete<Type, DeleterType>(task_runner)) {
    assert(task_runner);
  }

  SerialDeletePtr(TaskRunner* task_runner, std::nullptr_t) noexcept
      : std::unique_ptr<Type, SerialDelete<Type, DeleterType>>(
            nullptr,
            SerialDelete<Type, DeleterType>(task_runner)) {
    assert(task_runner);
  }

  SerialDeletePtr(TaskRunner* task_runner, Type* pointer) noexcept
      : std::unique_ptr<Type, SerialDelete<Type, DeleterType>>(
            pointer,
            SerialDelete<Type, DeleterType>(task_runner)) {
    assert(task_runner);
  }

  SerialDeletePtr(
      TaskRunner* task_runner,
      Type* pointer,
      typename std::conditional<std::is_reference<DeleterType>::value,
                                DeleterType,
                                const DeleterType&>::type deleter) noexcept
      : std::unique_ptr<Type, SerialDelete<Type, DeleterType>>(
            pointer,
            SerialDelete<Type, DeleterType>(task_runner, deleter)) {
    assert(task_runner);
  }

  SerialDeletePtr(
      TaskRunner* task_runner,
      Type* pointer,
      typename std::remove_reference<DeleterType>::type&& deleter) noexcept
      : std::unique_ptr<Type, SerialDelete<Type, DeleterType>>(
            pointer,
            SerialDelete<Type, DeleterType>(task_runner, std::move(deleter))) {
    assert(task_runner);
  }
};

template <typename Type, typename... Args>
SerialDeletePtr<Type> MakeSerialDelete(TaskRunner* task_runner,
                                       Args&&... args) {
  return SerialDeletePtr<Type>(task_runner,
                               new Type(std::forward<Args>(args)...));
}

}  // namespace openscreen

#endif  // PLATFORM_API_SERIAL_DELETE_PTR_H_
