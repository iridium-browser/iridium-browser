// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_MOCK_ENVIRONMENT_H_
#define CAST_STREAMING_MOCK_ENVIRONMENT_H_

#include "cast/streaming/environment.h"
#include "gmock/gmock.h"

namespace openscreen {
namespace cast {

// An Environment that can intercept all packet sends, for unit testing.
class MockEnvironment : public Environment {
 public:
  MockEnvironment(ClockNowFunctionPtr now_function, TaskRunner* task_runner);
  ~MockEnvironment() override;

  // Used to return fake values, to simulate a bound socket for testing.
  MOCK_METHOD(IPEndpoint, GetBoundLocalEndpoint, (), (const, override));

  // Used for intercepting packet sends from the implementation under test.
  MOCK_METHOD(void, SendPacket, (absl::Span<const uint8_t> packet), (override));
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_MOCK_ENVIRONMENT_H_
