// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/api/task_runner_deleter.h"

namespace openscreen {

TaskRunnerDeleter::TaskRunnerDeleter() = default;

TaskRunnerDeleter::TaskRunnerDeleter(TaskRunner& task_runner)
    : task_runner_(&task_runner) {}

TaskRunnerDeleter::~TaskRunnerDeleter() = default;

TaskRunnerDeleter::TaskRunnerDeleter(const TaskRunnerDeleter&) = default;
TaskRunnerDeleter& TaskRunnerDeleter::operator=(const TaskRunnerDeleter&) =
    default;
TaskRunnerDeleter::TaskRunnerDeleter(TaskRunnerDeleter&&) noexcept = default;
TaskRunnerDeleter& TaskRunnerDeleter::operator=(TaskRunnerDeleter&&) noexcept =
    default;

}  // namespace openscreen
