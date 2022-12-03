// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/trace_logging.h"

#include <chrono>
#include <thread>

#include "absl/types/optional.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/test/trace_logging_helpers.h"

namespace openscreen {
namespace {

#if defined(ENABLE_TRACE_LOGGING)
constexpr TraceHierarchyParts kAllParts = static_cast<TraceHierarchyParts>(
    TraceHierarchyParts::kRoot | TraceHierarchyParts::kParent |
    TraceHierarchyParts::kCurrent);
constexpr TraceHierarchyParts kParentAndRoot = static_cast<TraceHierarchyParts>(
    TraceHierarchyParts::kRoot | TraceHierarchyParts::kParent);
constexpr TraceId kEmptyId = TraceId{0};
#endif

using ::testing::_;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Invoke;

// All the unit tests below should use TestTraceLoggingPlatform for the
// library's tracing output mock. This is a StrictMock to ensure that, when not
// compiling with ENABLE_TRACE_LOGGING, the mock receives no method calls.
using StrictMockLoggingPlatform = ::testing::StrictMock<MockLoggingPlatform>;

TEST(TraceLoggingTest, MacroCallScopedDoesNotSegFault) {
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::Value::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogTrace(_, _, _, _, _, _, _)).Times(1);
#endif
  { TRACE_SCOPED(TraceCategory::Value::kAny, "test"); }
}

TEST(TraceLoggingTest, MacroCallDefaultScopedDoesNotSegFault) {
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::Value::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogTrace(_, _, _, _, _, _, _)).Times(1);
#endif
  { TRACE_DEFAULT_SCOPED(TraceCategory::Value::kAny); }
}

TEST(TraceLoggingTest, MacroCallUnscopedDoesNotSegFault) {
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::Value::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogAsyncStart(_, _, _, _, _)).Times(1);
#endif
  { TRACE_ASYNC_START(TraceCategory::Value::kAny, "test"); }
}

TEST(TraceLoggingTest, MacroVariablesUniquelyNames) {
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::Value::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogTrace(_, _, _, _, _, _, _)).Times(3);
  EXPECT_CALL(platform, LogAsyncStart(_, _, _, _, _)).Times(2);
#endif

  {
    TRACE_SCOPED(TraceCategory::Value::kAny, "test1");
    TRACE_SCOPED(TraceCategory::Value::kAny, "test2");
    TRACE_ASYNC_START(TraceCategory::Value::kAny, "test3");
    TRACE_ASYNC_START(TraceCategory::Value::kAny, "test4");
    TRACE_DEFAULT_SCOPED(TraceCategory::Value::kAny);
  }
}

TEST(TraceLoggingTest, ExpectTimestampsReflectDelay) {
  constexpr uint32_t delay_in_ms = 50;
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::Value::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogTrace(_, _, _, _, _, _, _))
      .WillOnce(DoAll(Invoke(ValidateTraceTimestampDiff<delay_in_ms>),
                      Invoke(ValidateTraceErrorCode<Error::Code::kNone>)));
#endif

  {
    TRACE_SCOPED(TraceCategory::Value::kAny, "Name");
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_in_ms));
  }
}

TEST(TraceLoggingTest, ExpectErrorsPassedToResult) {
  constexpr Error::Code result_code = Error::Code::kParseError;
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::Value::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogTrace(_, _, _, _, _, _, _))
      .WillOnce(Invoke(ValidateTraceErrorCode<result_code>));
#endif

  {
    TRACE_SCOPED(TraceCategory::Value::kAny, "Name");
    TRACE_SET_RESULT(result_code);
  }
}

TEST(TraceLoggingTest, ExpectUnsetTraceIdNotSet) {
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::Value::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogTrace(_, _, _, _, _, _, _)).Times(1);
#endif

  TraceIdHierarchy h = {kUnsetTraceId, kUnsetTraceId, kUnsetTraceId};
  {
    TRACE_SCOPED(TraceCategory::Value::kAny, "Name", h);

    auto ids = TRACE_HIERARCHY;
    EXPECT_NE(ids.current, kUnsetTraceId);
    EXPECT_NE(ids.parent, kUnsetTraceId);
    EXPECT_NE(ids.root, kUnsetTraceId);
  }
}

TEST(TraceLoggingTest, ExpectCreationWithIdsToWork) {
  constexpr TraceId current = 0x32;
  constexpr TraceId parent = 0x47;
  constexpr TraceId root = 0x84;
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::Value::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogTrace(_, _, _, _, _, _, _))
      .WillOnce(
          DoAll(Invoke(ValidateTraceErrorCode<Error::Code::kNone>),
                Invoke(ValidateTraceIdHierarchyOnSyncTrace<current, parent,
                                                           root, kAllParts>)));
#endif

  {
    TraceIdHierarchy h = {current, parent, root};
    TRACE_SCOPED(TraceCategory::Value::kAny, "Name", h);

#if defined(ENABLE_TRACE_LOGGING)
    auto ids = TRACE_HIERARCHY;
    EXPECT_EQ(ids.current, current);
    EXPECT_EQ(ids.parent, parent);
    EXPECT_EQ(ids.root, root);

    EXPECT_EQ(TRACE_CURRENT_ID, current);
    EXPECT_EQ(TRACE_ROOT_ID, root);
#endif
  }
}

TEST(TraceLoggingTest, ExpectHirearchyToBeApplied) {
  constexpr TraceId current = 0x32;
  constexpr TraceId parent = 0x47;
  constexpr TraceId root = 0x84;
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::Value::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogTrace(_, _, _, _, _, _, _))
      .WillOnce(DoAll(
          Invoke(ValidateTraceErrorCode<Error::Code::kNone>),
          Invoke(ValidateTraceIdHierarchyOnSyncTrace<kEmptyId, current, root,
                                                     kParentAndRoot>)))
      .WillOnce(
          DoAll(Invoke(ValidateTraceErrorCode<Error::Code::kNone>),
                Invoke(ValidateTraceIdHierarchyOnSyncTrace<current, parent,
                                                           root, kAllParts>)));
#endif

  {
    TraceIdHierarchy h = {current, parent, root};
    TRACE_SCOPED(TraceCategory::Value::kAny, "Name", h);
#if defined(ENABLE_TRACE_LOGGING)
    auto ids = TRACE_HIERARCHY;
    EXPECT_EQ(ids.current, current);
    EXPECT_EQ(ids.parent, parent);
    EXPECT_EQ(ids.root, root);
#endif

    TRACE_SCOPED(TraceCategory::Value::kAny, "Name");
#if defined(ENABLE_TRACE_LOGGING)
    ids = TRACE_HIERARCHY;
    EXPECT_NE(ids.current, current);
    EXPECT_EQ(ids.parent, current);
    EXPECT_EQ(ids.root, root);
#endif
  }
}

TEST(TraceLoggingTest, ExpectHirearchyToEndAfterScopeWhenSetWithSetter) {
  constexpr TraceId current = 0x32;
  constexpr TraceId parent = 0x47;
  constexpr TraceId root = 0x84;
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::Value::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogTrace(_, _, _, _, _, _, _))
      .WillOnce(DoAll(
          Invoke(ValidateTraceErrorCode<Error::Code::kNone>),
          Invoke(ValidateTraceIdHierarchyOnSyncTrace<kEmptyId, current, root,
                                                     kParentAndRoot>)));
#endif

  {
    TraceIdHierarchy ids = {current, parent, root};
    TRACE_SET_HIERARCHY(ids);
    {
      TRACE_SCOPED(TraceCategory::Value::kAny, "Name");
      ids = TRACE_HIERARCHY;
#if defined(ENABLE_TRACE_LOGGING)
      EXPECT_NE(ids.current, current);
      EXPECT_EQ(ids.parent, current);
      EXPECT_EQ(ids.root, root);
#endif
    }

    ids = TRACE_HIERARCHY;
#if defined(ENABLE_TRACE_LOGGING)
    EXPECT_EQ(ids.current, current);
    EXPECT_EQ(ids.parent, parent);
    EXPECT_EQ(ids.root, root);
#endif
  }
}

TEST(TraceLoggingTest, ExpectHierarchyToEndAfterScope) {
  constexpr TraceId current = 0x32;
  constexpr TraceId parent = 0x47;
  constexpr TraceId root = 0x84;
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::Value::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogTrace(_, _, _, _, _, _, _))
      .WillOnce(DoAll(
          Invoke(ValidateTraceErrorCode<Error::Code::kNone>),
          Invoke(ValidateTraceIdHierarchyOnSyncTrace<kEmptyId, current, root,
                                                     kParentAndRoot>)))
      .WillOnce(
          DoAll(Invoke(ValidateTraceErrorCode<Error::Code::kNone>),
                Invoke(ValidateTraceIdHierarchyOnSyncTrace<current, parent,
                                                           root, kAllParts>)));
#endif

  {
    TraceIdHierarchy ids = {current, parent, root};
    TRACE_SCOPED(TraceCategory::Value::kAny, "Name", ids);
    {
      TRACE_SCOPED(TraceCategory::Value::kAny, "Name");
      ids = TRACE_HIERARCHY;
#if defined(ENABLE_TRACE_LOGGING)
      EXPECT_NE(ids.current, current);
      EXPECT_EQ(ids.parent, current);
      EXPECT_EQ(ids.root, root);
#endif
    }

    ids = TRACE_HIERARCHY;
#if defined(ENABLE_TRACE_LOGGING)
    EXPECT_EQ(ids.current, current);
    EXPECT_EQ(ids.parent, parent);
    EXPECT_EQ(ids.root, root);
#endif
  }
}

TEST(TraceLoggingTest, ExpectSetHierarchyToApply) {
  constexpr TraceId current = 0x32;
  constexpr TraceId parent = 0x47;
  constexpr TraceId root = 0x84;
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::Value::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogTrace(_, _, _, _, _, _, _))
      .WillOnce(DoAll(
          Invoke(ValidateTraceErrorCode<Error::Code::kNone>),
          Invoke(ValidateTraceIdHierarchyOnSyncTrace<kEmptyId, current, root,
                                                     kParentAndRoot>)));
#endif

  {
    TraceIdHierarchy ids = {current, parent, root};
    TRACE_SET_HIERARCHY(ids);
    ids = TRACE_HIERARCHY;
#if defined(ENABLE_TRACE_LOGGING)
    EXPECT_EQ(ids.current, current);
    EXPECT_EQ(ids.parent, parent);
    EXPECT_EQ(ids.root, root);
#endif

    TRACE_SCOPED(TraceCategory::Value::kAny, "Name");
    ids = TRACE_HIERARCHY;
#if defined(ENABLE_TRACE_LOGGING)
    EXPECT_NE(ids.current, current);
    EXPECT_EQ(ids.parent, current);
    EXPECT_EQ(ids.root, root);
#endif
  }
}

TEST(TraceLoggingTest, CheckTraceAsyncStartLogsCorrectly) {
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::Value::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogAsyncStart(_, _, _, _, _)).Times(1);
#endif

  { TRACE_ASYNC_START(TraceCategory::Value::kAny, "Name"); }
}

TEST(TraceLoggingTest, CheckTraceAsyncStartSetsHierarchy) {
  constexpr TraceId current = 32;
  constexpr TraceId parent = 47;
  constexpr TraceId root = 84;
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::Value::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogAsyncStart(_, _, _, _, _))
      .WillOnce(
          Invoke(ValidateTraceIdHierarchyOnAsyncTrace<kEmptyId, current, root,
                                                      kParentAndRoot>));
#endif

  {
    TraceIdHierarchy ids = {current, parent, root};
    TRACE_SET_HIERARCHY(ids);
    {
      TRACE_ASYNC_START(TraceCategory::Value::kAny, "Name");
      ids = TRACE_HIERARCHY;
#if defined(ENABLE_TRACE_LOGGING)
      EXPECT_NE(ids.current, current);
      EXPECT_EQ(ids.parent, current);
      EXPECT_EQ(ids.root, root);
#endif
    }

    ids = TRACE_HIERARCHY;
#if defined(ENABLE_TRACE_LOGGING)
    EXPECT_EQ(ids.current, current);
    EXPECT_EQ(ids.parent, parent);
    EXPECT_EQ(ids.root, root);
#endif
  }
}

TEST(TraceLoggingTest, CheckTraceAsyncEndLogsCorrectly) {
  constexpr TraceId id = 12345;
  constexpr Error::Code result = Error::Code::kAgain;
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::Value::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogAsyncEnd(_, _, _, id, result)).Times(1);
#endif

  TRACE_ASYNC_END(TraceCategory::Value::kAny, id, result);
}

}  // namespace
}  // namespace openscreen
