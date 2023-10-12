// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/serial_delete_ptr.h"

#include "gtest/gtest.h"
#include "platform/test/fake_clock.h"
#include "platform/test/fake_task_runner.h"

namespace openscreen {

class SerialDeletePtrTest : public ::testing::Test {
 public:
  SerialDeletePtrTest() : clock_(Clock::now()), task_runner_(&clock_) {}

 protected:
  FakeClock clock_;
  FakeTaskRunner task_runner_;
};

TEST_F(SerialDeletePtrTest, Empty) {
  SerialDeletePtr<int> pointer(task_runner_);
}

TEST_F(SerialDeletePtrTest, Nullptr) {
  SerialDeletePtr<int> pointer(task_runner_, nullptr);
}

TEST_F(SerialDeletePtrTest, DefaultDeleter) {
  { SerialDeletePtr<int> pointer(task_runner_, new int); }
  task_runner_.RunTasksUntilIdle();
}

}  // namespace openscreen
