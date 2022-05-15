// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/api/trace_logging_platform.h"

#ifndef PLATFORM_TEST_TRACE_LOGGING_HELPERS_H_
#define PLATFORM_TEST_TRACE_LOGGING_HELPERS_H_

#include <chrono>

#include "gmock/gmock.h"
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

  MOCK_METHOD1(IsTraceLoggingEnabled, bool(TraceCategory::Value category));
  MOCK_METHOD7(LogTrace,
               void(const char*,
                    const uint32_t,
                    const char* file,
                    Clock::time_point,
                    Clock::time_point,
                    TraceIdHierarchy ids,
                    Error::Code));
  MOCK_METHOD5(LogAsyncStart,
               void(const char*,
                    const uint32_t,
                    const char* file,
                    Clock::time_point,
                    TraceIdHierarchy));
  MOCK_METHOD5(LogAsyncEnd,
               void(const uint32_t,
                    const char* file,
                    Clock::time_point,
                    TraceId,
                    Error::Code));
};

// Methods to validate the results of platform-layer calls.
template <uint64_t milliseconds>
void ValidateTraceTimestampDiff(const char* name,
                                const uint32_t line,
                                const char* file,
                                Clock::time_point start_time,
                                Clock::time_point end_time,
                                TraceIdHierarchy ids,
                                Error error) {
  const auto elapsed = to_milliseconds(end_time - start_time);
  ASSERT_GE(static_cast<uint64_t>(elapsed.count()), milliseconds);
}

template <Error::Code result>
void ValidateTraceErrorCode(const char* name,
                            const uint32_t line,
                            const char* file,
                            Clock::time_point start_time,
                            Clock::time_point end_time,
                            TraceIdHierarchy ids,
                            Error error) {
  ASSERT_EQ(error.code(), result);
}

template <TraceId Current,
          TraceId Parent,
          TraceId Root,
          TraceHierarchyParts parts>
void ValidateTraceIdHierarchyOnSyncTrace(const char* name,
                                         const uint32_t line,
                                         const char* file,
                                         Clock::time_point start_time,
                                         Clock::time_point end_time,
                                         TraceIdHierarchy ids,
                                         Error error) {
  if (parts & TraceHierarchyParts::kCurrent) {
    ASSERT_EQ(ids.current, Current);
  }
  if (parts & TraceHierarchyParts::kParent) {
    ASSERT_EQ(ids.parent, Parent);
  }
  if (parts & TraceHierarchyParts::kRoot) {
    ASSERT_EQ(ids.root, Root);
  }
}

template <TraceId Current,
          TraceId Parent,
          TraceId Root,
          TraceHierarchyParts parts>
void ValidateTraceIdHierarchyOnAsyncTrace(const char* name,
                                          const uint32_t line,
                                          const char* file,
                                          Clock::time_point timestamp,
                                          TraceIdHierarchy ids) {
  if (parts & TraceHierarchyParts::kCurrent) {
    EXPECT_EQ(ids.current, Current);
  }
  if (parts & TraceHierarchyParts::kParent) {
    EXPECT_EQ(ids.parent, Parent);
  }
  if (parts & TraceHierarchyParts::kRoot) {
    EXPECT_EQ(ids.root, Root);
  }
}

}  // namespace openscreen

#endif  // PLATFORM_TEST_TRACE_LOGGING_HELPERS_H_
