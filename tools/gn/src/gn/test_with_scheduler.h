// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_TEST_WITH_SCHEDULER_H_
#define TOOLS_GN_TEST_WITH_SCHEDULER_H_

#include "gn/scheduler.h"
#include "util/msg_loop.h"
#include "util/test/test.h"

class TestWithScheduler : public testing::Test {
 protected:
  TestWithScheduler();
  ~TestWithScheduler() override;

  Scheduler& scheduler() { return scheduler_; }

 private:
  MsgLoop run_loop_;
  Scheduler scheduler_;

  TestWithScheduler(const TestWithScheduler&) = delete;
  TestWithScheduler& operator=(const TestWithScheduler&) = delete;
};

#endif  // TOOLS_GN_TEST_WITH_SCHEDULER_H_
