// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/api/trace_logging_platform.h"

#ifndef PLATFORM_TEST_TRACE_LOGGING_HELPERS_H_
#define PLATFORM_TEST_TRACE_LOGGING_HELPERS_H_

#include <chrono>

#include "gmock/gmock.h"
#include "platform/api/trace_event.h"
#include "platform/base/trace_logging_activation.h"
#include "util/chrono_helpers.h"
#include "util/osp_logging.h"

namespace openscreen {

enum TraceHierarchyParts { kRoot = 0x1, kParent = 0x2, kCurrent = 0x4 };

enum ArgumentId { kFirst, kSecond };

class MockLoggingPlatform : public TraceLoggingPlatform {
 public:
  MockLoggingPlatform() {
    StartTracing(this);

    ON_CALL(*this, IsTraceLoggingEnabled(::testing::_))
        .WillByDefault(::testing::Return(true));
  }

  ~MockLoggingPlatform() override { StopTracing(); }

  MOCK_METHOD1(IsTraceLoggingEnabled, bool(TraceCategory category));
  MOCK_METHOD2(LogTrace, void(TraceEvent event, Clock::time_point));
  MOCK_METHOD1(LogAsyncStart, void(TraceEvent event));
  MOCK_METHOD1(LogAsyncEnd, void(TraceEvent event));
};

// Methods to validate the results of platform-layer calls.
template <uint64_t milliseconds>
void ValidateTraceTimestampDiff(TraceEvent event, Clock::time_point end_time) {
  const auto elapsed = to_milliseconds(end_time - event.start_time);
  ASSERT_GE(static_cast<uint64_t>(elapsed.count()), milliseconds);
}

template <Error::Code result>
void ValidateTraceErrorCode(TraceEvent event, Clock::time_point end_time) {
  ASSERT_EQ(result, event.result);
}

template <TraceId Current,
          TraceId Parent,
          TraceId Root,
          TraceHierarchyParts parts>
void ValidateTraceIdHierarchyOnAsyncTrace(TraceEvent event) {
  if (parts & TraceHierarchyParts::kCurrent) {
    EXPECT_EQ(event.ids.current, Current);
  }
  if (parts & TraceHierarchyParts::kParent) {
    EXPECT_EQ(event.ids.parent, Parent);
  }
  if (parts & TraceHierarchyParts::kRoot) {
    EXPECT_EQ(event.ids.root, Root);
  }
}

template <TraceId Current,
          TraceId Parent,
          TraceId Root,
          TraceHierarchyParts parts>
void ValidateTraceIdHierarchyOnSyncTrace(TraceEvent event,
                                         Clock::time_point end_time) {
  ValidateTraceIdHierarchyOnAsyncTrace<Current, Parent, Root, parts>(event);
}

}  // namespace openscreen

#endif  // PLATFORM_TEST_TRACE_LOGGING_HELPERS_H_
