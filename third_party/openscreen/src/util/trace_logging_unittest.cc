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

MATCHER_P(EqAsyncEndEvent, expected, "") {
  return arg.ids.current == expected.ids.current &&
         arg.result == expected.result;
}

MATCHER_P(HasSameArguments, expected, "") {
  return arg.arguments == expected.arguments;
}

// All the unit tests below should use TestTraceLoggingPlatform for the
// library's tracing output mock. This is a StrictMock to ensure that, when not
// compiling with ENABLE_TRACE_LOGGING, the mock receives no method calls.
using StrictMockLoggingPlatform = ::testing::StrictMock<MockLoggingPlatform>;

TEST(TraceLoggingTest, MacroCallScopedDoesNotSegFault) {
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogTrace(_, _));
#endif
  { TRACE_SCOPED(TraceCategory::kAny, "test"); }
}

TEST(TraceLoggingTest, MacroCallDefaultScopedDoesNotSegFault) {
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogTrace(_, _));
#endif
  { TRACE_DEFAULT_SCOPED(TraceCategory::kAny); }
}

TEST(TraceLoggingTest, MacroCallUnscopedDoesNotSegFault) {
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogAsyncStart(_));
#endif
  { TRACE_ASYNC_START(TraceCategory::kAny, "test"); }
}

TEST(TraceLoggingTest, MacroVariablesUniquelyNames) {
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogTrace(_, _)).Times(3);
  EXPECT_CALL(platform, LogAsyncStart(_)).Times(2);
#endif

  {
    TRACE_SCOPED(TraceCategory::kAny, "test1");
    TRACE_SCOPED(TraceCategory::kAny, "test2");
    TRACE_ASYNC_START(TraceCategory::kAny, "test3");
    TRACE_ASYNC_START(TraceCategory::kAny, "test4");
    TRACE_DEFAULT_SCOPED(TraceCategory::kAny);
  }
}

TEST(TraceLoggingTest, ExpectTimestampsReflectDelay) {
  constexpr uint32_t delay_in_ms = 50;
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogTrace(_, _))
      .WillOnce(DoAll(Invoke(ValidateTraceTimestampDiff<delay_in_ms>),
                      Invoke(ValidateTraceErrorCode<Error::Code::kNone>)));
#endif

  {
    TRACE_SCOPED(TraceCategory::kAny, "Name");
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_in_ms));
  }
}

TEST(TraceLoggingTest, ExpectErrorsPassedToResult) {
  constexpr Error::Code result_code = Error::Code::kParseError;
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogTrace(_, _))
      .WillOnce(Invoke(ValidateTraceErrorCode<result_code>));
#endif

  {
    TRACE_SCOPED(TraceCategory::kAny, "Name");
    TRACE_SET_RESULT(result_code);
  }
}

TEST(TraceLoggingTest, ExpectUnsetTraceIdNotSet) {
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogTrace(_, _));
#endif

  TraceIdHierarchy h = {kUnsetTraceId, kUnsetTraceId, kUnsetTraceId};
  {
    TRACE_SCOPED(TraceCategory::kAny, "Name", h);

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
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogTrace(_, _))
      .WillOnce(
          DoAll(Invoke(ValidateTraceErrorCode<Error::Code::kNone>),
                Invoke(ValidateTraceIdHierarchyOnSyncTrace<current, parent,
                                                           root, kAllParts>)));
#endif

  {
    TraceIdHierarchy h = {current, parent, root};
    TRACE_SCOPED(TraceCategory::kAny, "Name", h);

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

TEST(TraceLoggingTest, ExpectHierarchyToBeApplied) {
  constexpr TraceId current = 0x32;
  constexpr TraceId parent = 0x47;
  constexpr TraceId root = 0x84;
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogTrace(_, _))
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
    TRACE_SCOPED(TraceCategory::kAny, "Name", h);
#if defined(ENABLE_TRACE_LOGGING)
    auto ids = TRACE_HIERARCHY;
    EXPECT_EQ(ids.current, current);
    EXPECT_EQ(ids.parent, parent);
    EXPECT_EQ(ids.root, root);
#endif

    TRACE_SCOPED(TraceCategory::kAny, "Name");
#if defined(ENABLE_TRACE_LOGGING)
    ids = TRACE_HIERARCHY;
    EXPECT_NE(ids.current, current);
    EXPECT_EQ(ids.parent, current);
    EXPECT_EQ(ids.root, root);
#endif
  }
}

TEST(TraceLoggingTest, ExpectHierarchyToEndAfterScopeWhenSetWithSetter) {
  constexpr TraceId current = 0x32;
  constexpr TraceId parent = 0x47;
  constexpr TraceId root = 0x84;
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogTrace(_, _))
      .WillOnce(DoAll(
          Invoke(ValidateTraceErrorCode<Error::Code::kNone>),
          Invoke(ValidateTraceIdHierarchyOnSyncTrace<kEmptyId, current, root,
                                                     kParentAndRoot>)));
#endif

  {
    TraceIdHierarchy ids = {current, parent, root};
    TRACE_SET_HIERARCHY(ids);
    {
      TRACE_SCOPED(TraceCategory::kAny, "Name");
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
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogTrace(_, _))
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
    TRACE_SCOPED(TraceCategory::kAny, "Name", ids);
    {
      TRACE_SCOPED(TraceCategory::kAny, "Name");
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
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogTrace(_, _))
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

    TRACE_SCOPED(TraceCategory::kAny, "Name");
    ids = TRACE_HIERARCHY;
#if defined(ENABLE_TRACE_LOGGING)
    EXPECT_NE(ids.current, current);
    EXPECT_EQ(ids.parent, current);
    EXPECT_EQ(ids.root, root);
#endif
  }
}

TEST(TraceLoggingTest, CheckTraceSupportsArguments) {
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::kAny))
      .Times(AtLeast(1));

  TraceEvent expected;
  expected.arguments.emplace_back("foo", "300");
  EXPECT_CALL(platform, LogTrace(HasSameArguments(expected), _)).Times(2);
  expected.arguments.emplace_back("bar", "baz");
  EXPECT_CALL(platform, LogTrace(HasSameArguments(expected), _)).Times(2);
#endif

  {
    TRACE_SCOPED1(TraceCategory::kAny, "Name", "foo", std::to_string(300));
    TRACE_DEFAULT_SCOPED1(TraceCategory::kAny, "foo", std::to_string(300));
  }
  {
    TRACE_SCOPED2(TraceCategory::kAny, "Name", "foo", std::to_string(300),
                  "bar", "baz");
    TRACE_DEFAULT_SCOPED2(TraceCategory::kAny, "foo", std::to_string(300),
                          "bar", "baz");
  }
}

TEST(TraceLoggingTest, CheckTraceCropsArgumentLengths) {
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::kAny))
      .Times(AtLeast(1));

  TraceEvent expected;
  std::string really_long_argument(1337, 'F');
  expected.arguments.emplace_back("foo", really_long_argument);

  // First, TraceEvent allows us to crop the argument length directly.
  expected.TruncateStrings();
  EXPECT_EQ(expected.arguments[0].second.size(), 1024u);
  EXPECT_TRUE(std::all_of(expected.arguments[0].second.begin(),
                          expected.arguments[0].second.begin() + 1020,
                          [](const char c) { return c == 'F'; }))
      << expected.arguments[0].second;
  EXPECT_TRUE(std::all_of(expected.arguments[0].second.begin() + 1021,
                          expected.arguments[0].second.end(),
                          [](const char c) { return c == '.'; }))
      << expected.arguments[0].second;

  EXPECT_CALL(platform, LogTrace(HasSameArguments(expected), _)).Times(2);
  EXPECT_CALL(platform, LogAsyncStart(HasSameArguments(expected)));

  expected.arguments.emplace_back("bar", really_long_argument);
  expected.TruncateStrings();
  EXPECT_CALL(platform, LogTrace(HasSameArguments(expected), _)).Times(2);
  EXPECT_CALL(platform, LogAsyncStart(HasSameArguments(expected)));

#endif

  {
    TRACE_SCOPED1(TraceCategory::kAny, "Name", "foo", really_long_argument);
    TRACE_DEFAULT_SCOPED1(TraceCategory::kAny, "foo", really_long_argument);
    TRACE_ASYNC_START1(TraceCategory::kAny, "Name", "foo",
                       really_long_argument);

    TRACE_SCOPED2(TraceCategory::kAny, "Name", "foo", really_long_argument,
                  "bar", really_long_argument);
    TRACE_DEFAULT_SCOPED2(TraceCategory::kAny, "foo", really_long_argument,
                          "bar", really_long_argument);
    TRACE_ASYNC_START2(TraceCategory::kAny, "Name", "foo", really_long_argument,
                       "bar", really_long_argument);
  }
}

TEST(TraceLoggingTest, CheckTraceAsyncStartLogsCorrectly) {
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogAsyncStart(_));
#endif

  { TRACE_ASYNC_START(TraceCategory::kAny, "Name"); }
}

TEST(TraceLoggingTest, CheckTraceAsyncSupportsArguments) {
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::kAny))
      .Times(AtLeast(1));

  TraceEvent expected;
  expected.arguments.emplace_back("foo", "300");
  EXPECT_CALL(platform, LogAsyncStart(HasSameArguments(expected)));
  expected.arguments.emplace_back("bar", "baz");
  EXPECT_CALL(platform, LogAsyncStart(HasSameArguments(expected)));
#endif

  {
    TRACE_ASYNC_START1(TraceCategory::kAny, "Name", "foo", std::to_string(300));
  }
  {
    TRACE_ASYNC_START2(TraceCategory::kAny, "Name", "foo", std::to_string(300),
                       "bar", "baz");
  }
}

TEST(TraceLoggingTest, CheckTraceAsyncStartSetsHierarchy) {
  constexpr TraceId current = 32;
  constexpr TraceId parent = 47;
  constexpr TraceId root = 84;
  StrictMockLoggingPlatform platform;
#if defined(ENABLE_TRACE_LOGGING)
  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogAsyncStart(_))
      .WillOnce(
          Invoke(ValidateTraceIdHierarchyOnAsyncTrace<kEmptyId, current, root,
                                                      kParentAndRoot>));
#endif

  {
    TraceIdHierarchy ids = {current, parent, root};
    TRACE_SET_HIERARCHY(ids);
    {
      TRACE_ASYNC_START(TraceCategory::kAny, "Name");
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
  TraceEvent expected;
  expected.ids.current = id;
  expected.result = result;

  EXPECT_CALL(platform, IsTraceLoggingEnabled(TraceCategory::kAny))
      .Times(AtLeast(1));
  EXPECT_CALL(platform, LogAsyncEnd(EqAsyncEndEvent(expected)));
#endif

  TRACE_ASYNC_END(TraceCategory::kAny, id, result);
}

}  // namespace
}  // namespace openscreen
