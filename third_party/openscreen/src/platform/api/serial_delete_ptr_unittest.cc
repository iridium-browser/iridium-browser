// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/api/serial_delete_ptr.h"

#include "gtest/gtest.h"
#include "platform/test/fake_clock.h"
#include "platform/test/fake_task_runner.h"

namespace openscreen {

class SerialDeletePtrTest : public ::testing::Test {
 public:
  SerialDeletePtrTest() : clock_(Clock::now()), task_runner_(&clock_) {}

 protected:
  static void FunctionPointerDeleter(int* pointer) {
    deleter_called_ = true;
    delete pointer;
  }

  FakeClock clock_;
  FakeTaskRunner task_runner_;

  static bool deleter_called_;
};

// static
bool SerialDeletePtrTest::deleter_called_ = false;

TEST_F(SerialDeletePtrTest, Empty) {
  SerialDeletePtr<int> pointer(&task_runner_);
}

TEST_F(SerialDeletePtrTest, Nullptr) {
  SerialDeletePtr<int> pointer(&task_runner_, nullptr);
}

TEST_F(SerialDeletePtrTest, DefaultDeleter) {
  { SerialDeletePtr<int> pointer(&task_runner_, new int); }
  task_runner_.RunTasksUntilIdle();
}

TEST_F(SerialDeletePtrTest, FuncitonPointerDeleter) {
  deleter_called_ = false;
  {
    SerialDeletePtr<int, decltype(&FunctionPointerDeleter)> pointer(
        &task_runner_, new int, FunctionPointerDeleter);
  }
  task_runner_.RunTasksUntilIdle();
  EXPECT_TRUE(deleter_called_);
}

TEST_F(SerialDeletePtrTest, LambdaDeleter) {
  deleter_called_ = false;
  auto deleter = [](int* pointer) {
    deleter_called_ = true;
    delete pointer;
  };

  {
    SerialDeletePtr<int, decltype(deleter)> pointer(&task_runner_, new int,
                                                    deleter);
  }

  task_runner_.RunTasksUntilIdle();
  EXPECT_TRUE(deleter_called_);
}

TEST_F(SerialDeletePtrTest, BindDeleter) {
  deleter_called_ = false;
  auto deleter = std::bind(FunctionPointerDeleter, std::placeholders::_1);

  {
    SerialDeletePtr<int, decltype(deleter)> pointer(&task_runner_, new int,
                                                    deleter);
  }

  task_runner_.RunTasksUntilIdle();
  EXPECT_TRUE(deleter_called_);
}

TEST_F(SerialDeletePtrTest, FunctionDeleter) {
  deleter_called_ = false;
  std::function<void(int*)> deleter = FunctionPointerDeleter;

  {
    SerialDeletePtr<int, std::function<void(int*)>> pointer(&task_runner_,
                                                            new int, deleter);
  }

  task_runner_.RunTasksUntilIdle();
  EXPECT_TRUE(deleter_called_);
}

TEST_F(SerialDeletePtrTest, FunctionDeleterWithFunctionPointer) {
  deleter_called_ = false;

  {
    SerialDeletePtr<int, std::function<void(int*)>> pointer(
        &task_runner_, new int, FunctionPointerDeleter);
  }

  task_runner_.RunTasksUntilIdle();
  EXPECT_TRUE(deleter_called_);
}

TEST_F(SerialDeletePtrTest, FunctionDeleterWithLambda) {
  deleter_called_ = false;
  auto deleter = [](int* pointer) {
    deleter_called_ = true;
    delete pointer;
  };

  {
    SerialDeletePtr<int, std::function<void(int*)>> pointer(&task_runner_,
                                                            new int, deleter);
  }

  task_runner_.RunTasksUntilIdle();
  EXPECT_TRUE(deleter_called_);
}

TEST_F(SerialDeletePtrTest, FunctionDeleterWithBind) {
  deleter_called_ = false;
  auto deleter = std::bind(FunctionPointerDeleter, std::placeholders::_1);

  {
    SerialDeletePtr<int, std::function<void(int*)>> pointer(&task_runner_,
                                                            new int, deleter);
  }

  task_runner_.RunTasksUntilIdle();
  EXPECT_TRUE(deleter_called_);
}

TEST_F(SerialDeletePtrTest, StructDeleter) {
  struct TestDeleter {
    void operator()(int* pointer) const { delete pointer; }
  };

  TestDeleter deleter;
  {
    SerialDeletePtr<int, TestDeleter> pointer(&task_runner_, new int, deleter);
  }

  task_runner_.RunTasksUntilIdle();
}

TEST_F(SerialDeletePtrTest, Move) {
  deleter_called_ = false;

  {
    SerialDeletePtr<int, decltype(&FunctionPointerDeleter)> pointer(
        &task_runner_);
    {
      SerialDeletePtr<int, decltype(&FunctionPointerDeleter)> temp_pointer(
          &task_runner_, new int, FunctionPointerDeleter);
      pointer = std::move(temp_pointer);

      // No deletion should happen on move
      task_runner_.RunTasksUntilIdle();
      EXPECT_FALSE(deleter_called_);
    }
  }

  task_runner_.RunTasksUntilIdle();
  EXPECT_TRUE(deleter_called_);
}

}  // namespace openscreen
