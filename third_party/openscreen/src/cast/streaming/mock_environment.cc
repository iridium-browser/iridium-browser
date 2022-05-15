// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/mock_environment.h"

namespace openscreen {
namespace cast {

MockEnvironment::MockEnvironment(ClockNowFunctionPtr now_function,
                                 TaskRunner* task_runner) {
  task_runner_ = task_runner;
  now_function_ = now_function;
}

MockEnvironment::~MockEnvironment() = default;

}  // namespace cast
}  // namespace openscreen
