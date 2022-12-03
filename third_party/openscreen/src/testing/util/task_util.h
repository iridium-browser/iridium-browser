// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_UTIL_TASK_UTIL_H_
#define TESTING_UTIL_TASK_UTIL_H_

#include <thread>

#include "gtest/gtest.h"
#include "platform/api/time.h"
#include "util/osp_logging.h"

namespace openscreen {

template <typename Cond>
void WaitForCondition(Cond condition,
                      Clock::duration delay = std::chrono::milliseconds(250),
                      int max_attempts = 8) {
  int attempts = 1;
  do {
    OSP_LOG_INFO << "--- Checking condition, attempt " << attempts << "/"
                 << max_attempts;
    if (condition()) {
      break;
    }
    std::this_thread::sleep_for(delay);
  } while (attempts++ < max_attempts);
  ASSERT_TRUE(condition());
}

}  // namespace openscreen

#endif  // TESTING_UTIL_TASK_UTIL_H_
