// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <chrono>
#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/test/trace_logging_helpers.h"
#include "util/trace_logging.h"

#if defined(ENABLE_TRACE_LOGGING)

namespace openscreen {
namespace internal {
namespace {

MATCHER_P(HasSameNameAndLocation, expected, "") {
  return arg.name == expected.name && arg.file_name == expected.file_name &&
         arg.line_number == expected.line_number;
}

}  // namespace

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;

// These tests validate that parameters are passed correctly by using the Trace
// Internals.
constexpr auto category = TraceCategory::kMdns;
constexpr uint32_t line = 10;

TEST(TraceLoggingInternalTest, CreatingNoTraceObjectValid) {
  TraceInstanceHelper<SynchronousTraceLogger>::Empty();
}

TEST(TraceLoggingInternalTest, TestMacroStyleInitializationTrue) {
  constexpr uint32_t delay_in_ms = 50;
  MockLoggingPlatform platform;
  EXPECT_CALL(platform, LogTrace(_, _))
      .WillOnce(DoAll(Invoke(ValidateTraceTimestampDiff<delay_in_ms>),
                      Invoke(ValidateTraceErrorCode<Error::Code::kNone>)));

  {
    uint8_t temp[sizeof(SynchronousTraceLogger)];
    auto ptr = TraceInstanceHelper<SynchronousTraceLogger>::Create(
        temp, category, "Name", __FILE__, line);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_in_ms));
    auto ids = ScopedTraceOperation::hierarchy();
    EXPECT_NE(ids.current, kEmptyTraceId);
  }
  auto ids2 = ScopedTraceOperation::hierarchy();
  EXPECT_EQ(ids2.current, kEmptyTraceId);
  EXPECT_EQ(ids2.parent, kEmptyTraceId);
  EXPECT_EQ(ids2.root, kEmptyTraceId);
}

TEST(TraceLoggingInternalTest, TestMacroStyleInitializationFalse) {
  MockLoggingPlatform platform;
  EXPECT_CALL(platform, LogTrace(_, _)).Times(0);

  {
    auto ptr = TraceInstanceHelper<SynchronousTraceLogger>::Empty();
    auto ids = ScopedTraceOperation::hierarchy();
    EXPECT_EQ(ids.current, kEmptyTraceId);
    EXPECT_EQ(ids.parent, kEmptyTraceId);
    EXPECT_EQ(ids.root, kEmptyTraceId);
  }
  auto ids2 = ScopedTraceOperation::hierarchy();
  EXPECT_EQ(ids2.current, kEmptyTraceId);
  EXPECT_EQ(ids2.parent, kEmptyTraceId);
  EXPECT_EQ(ids2.root, kEmptyTraceId);
}

TEST(TraceLoggingInternalTest, ExpectParametersPassedToResult) {
  MockLoggingPlatform platform;

  TraceEvent expected;
  expected.name = "Name";
  expected.line_number = line;
  expected.file_name = __FILE__;

  EXPECT_CALL(platform, LogTrace(HasSameNameAndLocation(expected), _))
      .WillOnce(Invoke(ValidateTraceErrorCode<Error::Code::kNone>));

  { SynchronousTraceLogger{category, "Name", __FILE__, line}; }
}

TEST(TraceLoggingInternalTest, CheckTraceAsyncStartLogsCorrectly) {
  MockLoggingPlatform platform;

  TraceEvent expected;
  expected.name = "Name";
  expected.line_number = line;
  expected.file_name = __FILE__;

  EXPECT_CALL(platform, LogAsyncStart(HasSameNameAndLocation(expected)));

  { AsynchronousTraceLogger{category, "Name", __FILE__, line}; }
}

TEST(TraceLoggingInternalTest, ValidateGettersValidOnEmptyStack) {
  EXPECT_EQ(ScopedTraceOperation::current_id(), kEmptyTraceId);
  EXPECT_EQ(ScopedTraceOperation::root_id(), kEmptyTraceId);

  auto ids = ScopedTraceOperation::hierarchy();
  EXPECT_EQ(ids.current, kEmptyTraceId);
  EXPECT_EQ(ids.parent, kEmptyTraceId);
  EXPECT_EQ(ids.root, kEmptyTraceId);
}

TEST(TraceLoggingInternalTest, ValidateSetResultDoesNotSegfaultOnEmptyStack) {
  Error error = Error::Code::kNone;
  ScopedTraceOperation::set_result(error);

  ScopedTraceOperation::set_result(Error::Code::kNone);
}

}  // namespace internal
}  // namespace openscreen

#endif  // defined(ENABLE_TRACE_LOGGING)
