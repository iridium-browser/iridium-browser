// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/api/logging.h"

#include <string>
#include <vector>

#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "gtest/gtest.h"
#include "platform/impl/logging.h"
#include "platform/impl/logging_test.h"
#include "util/osp_logging.h"

namespace openscreen {

class LoggingTest : public ::testing::Test {
 public:
  LoggingTest() {}

  void SetUp() {
    previous_log_level = GetLogLevel();
    SetLogLevel(LogLevel::kInfo);
    SetLogBufferForTest(&log_messages);
    testing::FLAGS_gtest_death_test_style = "threadsafe";
  }

  void TearDown() {
    SetLogLevel(previous_log_level);
    SetLogBufferForTest(nullptr);
  }

 protected:
  void ExpectLog(LogLevel level, absl::string_view message) {
    const char* level_string = "";
    switch (level) {
      case LogLevel::kVerbose:
        level_string = "VERBOSE";
        break;
      case LogLevel::kInfo:
        level_string = "INFO";
        break;
      case LogLevel::kWarning:
        level_string = "WARNING";
        break;
      case LogLevel::kError:
        level_string = "ERROR";
        break;
      case LogLevel::kFatal:
        level_string = "FATAL";
        break;
    }
    expected_messages.push_back({level_string, std::string(message)});
  }

  void VerifyNoLogs() { EXPECT_TRUE(log_messages.empty()); }

  void VerifyLogs() {
    ASSERT_EQ(expected_messages.size(), log_messages.size());
    auto expected_it = expected_messages.begin();
    auto actual_it = log_messages.begin();
    // NOTE: This is somewhat brittle; it relies on details of how
    // logging_posix.cc formats log messages.
    while (expected_it != expected_messages.end()) {
      EXPECT_TRUE(
          absl::StartsWith(*actual_it, absl::StrCat("[", expected_it->level)));
      EXPECT_TRUE(absl::EndsWith(
          *actual_it, absl::StrCat("] ", expected_it->message, "\n")));
      actual_it++;
      expected_it++;
    }
    expected_messages.clear();
    log_messages.clear();
  }

 private:
  struct LogMessage {
    std::string level;
    std::string message;
  };
  std::vector<std::string> log_messages;
  std::vector<LogMessage> expected_messages;
  LogLevel previous_log_level = LogLevel::kWarning;
};

TEST_F(LoggingTest, UnconditionalLogging) {
  SetLogLevel(LogLevel::kVerbose);
  ExpectLog(LogLevel::kVerbose, "Verbose");
  ExpectLog(LogLevel::kInfo, "Info");
  ExpectLog(LogLevel::kWarning, "Warning");
  ExpectLog(LogLevel::kError, "Error");

  OSP_VLOG << "Verbose";
  OSP_LOG_INFO << "Info";
  OSP_LOG_WARN << "Warning";
  OSP_LOG_ERROR << "Error";

  VerifyLogs();

  SetLogLevel(LogLevel::kWarning);
  ExpectLog(LogLevel::kWarning, "Warning");
  ExpectLog(LogLevel::kError, "Error");

  OSP_VLOG << "Verbose";
  OSP_LOG_INFO << "Info";
  OSP_LOG_WARN << "Warning";
  OSP_LOG_ERROR << "Error";

  VerifyLogs();
}

TEST_F(LoggingTest, ConditionalLogging) {
  SetLogLevel(LogLevel::kVerbose);

  ExpectLog(LogLevel::kVerbose, "Verbose");
  ExpectLog(LogLevel::kInfo, "Info");
  ExpectLog(LogLevel::kWarning, "Warning");
  ExpectLog(LogLevel::kError, "Error");

  OSP_VLOG_IF(true) << "Verbose";
  OSP_LOG_IF(INFO, true) << "Info";
  OSP_LOG_IF(WARN, true) << "Warning";
  OSP_LOG_IF(ERROR, true) << "Error";
  VerifyLogs();

  OSP_VLOG_IF(false) << "Verbose";
  OSP_LOG_IF(INFO, false) << "Info";
  OSP_LOG_IF(WARN, false) << "Warning";
  OSP_LOG_IF(ERROR, false) << "Error";
  VerifyNoLogs();
}

TEST_F(LoggingTest, DebugUnconditionalLogging) {
  SetLogLevel(LogLevel::kVerbose);
  OSP_DVLOG << "Verbose";
  OSP_DLOG_INFO << "Info";
  OSP_DLOG_WARN << "Warning";
  OSP_DLOG_ERROR << "Error";

#if OSP_DCHECK_IS_ON()
  ExpectLog(LogLevel::kVerbose, "Verbose");
  ExpectLog(LogLevel::kInfo, "Info");
  ExpectLog(LogLevel::kWarning, "Warning");
  ExpectLog(LogLevel::kError, "Error");
  VerifyLogs();
#else
  VerifyNoLogs();
#endif  // OSP_DCHECK_IS_ON()

  SetLogLevel(LogLevel::kWarning);

  OSP_DVLOG << "Verbose";
  OSP_DLOG_INFO << "Info";
  OSP_DLOG_WARN << "Warning";
  OSP_DLOG_ERROR << "Error";

#if OSP_DCHECK_IS_ON()
  ExpectLog(LogLevel::kWarning, "Warning");
  ExpectLog(LogLevel::kError, "Error");
  VerifyLogs();
#else
  VerifyNoLogs();
#endif  // OSP_DCHECK_IS_ON()
}

TEST_F(LoggingTest, DebugConditionalLogging) {
  SetLogLevel(LogLevel::kVerbose);

  OSP_DVLOG_IF(true) << "Verbose";
  OSP_DLOG_IF(INFO, true) << "Info";
  OSP_DLOG_IF(WARN, true) << "Warning";
  OSP_DLOG_IF(ERROR, true) << "Error";

#if OSP_DCHECK_IS_ON()
  ExpectLog(LogLevel::kVerbose, "Verbose");
  ExpectLog(LogLevel::kInfo, "Info");
  ExpectLog(LogLevel::kWarning, "Warning");
  ExpectLog(LogLevel::kError, "Error");
  VerifyLogs();
#else
  VerifyNoLogs();
#endif  // OSP_DCHECK_IS_ON()

  OSP_DVLOG_IF(false) << "Verbose";
  OSP_DLOG_IF(INFO, false) << "Info";
  OSP_DLOG_IF(WARN, false) << "Warning";
  OSP_DLOG_IF(ERROR, false) << "Error";
  VerifyNoLogs();
}

TEST_F(LoggingTest, CheckAndLogFatal) {
  ASSERT_DEATH(OSP_CHECK(false), ".*OSP_CHECK\\(false\\) failed: ");

  ASSERT_DEATH(OSP_CHECK_EQ(1, 2),
               ".*OSP_CHECK\\(\\(1\\) == \\(2\\)\\) failed: 1 vs\\. 2: ");

  ASSERT_DEATH(OSP_CHECK_NE(1, 1),
               ".*OSP_CHECK\\(\\(1\\) != \\(1\\)\\) failed: 1 vs\\. 1: ");

  ASSERT_DEATH(OSP_CHECK_LT(2, 1),
               ".*OSP_CHECK\\(\\(2\\) < \\(1\\)\\) failed: 2 vs\\. 1: ");

  ASSERT_DEATH(OSP_CHECK_LE(2, 1),
               ".*OSP_CHECK\\(\\(2\\) <= \\(1\\)\\) failed: 2 vs\\. 1: ");

  ASSERT_DEATH(OSP_CHECK_GT(1, 2),
               ".*OSP_CHECK\\(\\(1\\) > \\(2\\)\\) failed: 1 vs\\. 2: ");

  ASSERT_DEATH(OSP_CHECK_GE(1, 2),
               ".*OSP_CHECK\\(\\(1\\) >= \\(2\\)\\) failed: 1 vs\\. 2: ");

  ASSERT_DEATH((OSP_LOG_FATAL << "Fatal"), ".*Fatal");

  VerifyLogs();
}

TEST_F(LoggingTest, DCheckAndDLogFatal) {
#if OSP_DCHECK_IS_ON()
  ASSERT_DEATH(OSP_DCHECK(false), ".*OSP_CHECK\\(false\\) failed: ");

  ASSERT_DEATH(OSP_DCHECK_EQ(1, 2),
               ".*OSP_CHECK\\(\\(1\\) == \\(2\\)\\) failed: 1 vs\\. 2: ");

  ASSERT_DEATH(OSP_DCHECK_NE(1, 1),
               ".*OSP_CHECK\\(\\(1\\) != \\(1\\)\\) failed: 1 vs\\. 1: ");

  ASSERT_DEATH(OSP_DCHECK_LT(2, 1),
               ".*OSP_CHECK\\(\\(2\\) < \\(1\\)\\) failed: 2 vs\\. 1: ");

  ASSERT_DEATH(OSP_DCHECK_LE(2, 1),
               ".*OSP_CHECK\\(\\(2\\) <= \\(1\\)\\) failed: 2 vs\\. 1: ");

  ASSERT_DEATH(OSP_DCHECK_GT(1, 2),
               ".*OSP_CHECK\\(\\(1\\) > \\(2\\)\\) failed: 1 vs\\. 2: ");

  ASSERT_DEATH(OSP_DCHECK_GE(1, 2),
               ".*OSP_CHECK\\(\\(1\\) >= \\(2\\)\\) failed: 1 vs\\. 2: ");

  ASSERT_DEATH((OSP_DLOG_FATAL << "Fatal"), ".*Fatal");

  VerifyLogs();
#else
  VerifyNoLogs();
#endif  // OSP_DCHECK_IS_ON()
}

TEST_F(LoggingTest, OspUnimplemented) {
  // Default is to log once per process if the level >= kWarning.
  SetLogLevel(LogLevel::kWarning);
  ExpectLog(LogLevel::kWarning, "TestBody: UNIMPLEMENTED() hit.");
  for (int i = 0; i < 2; i++) {
    OSP_UNIMPLEMENTED();
  }
  VerifyLogs();

  // Setting the level to kVerbose logs every time.
  SetLogLevel(LogLevel::kVerbose);
  std::string message("TestBody: UNIMPLEMENTED() hit.");
  ExpectLog(LogLevel::kVerbose, message);
  ExpectLog(LogLevel::kVerbose, message);
  for (int i = 0; i < 2; i++) {
    OSP_UNIMPLEMENTED();
  }
  VerifyLogs();
}

TEST_F(LoggingTest, OspNotReached) {
  ASSERT_DEATH(OSP_NOTREACHED(), ".*TestBody: NOTREACHED\\(\\) hit.");
}

}  // namespace openscreen
