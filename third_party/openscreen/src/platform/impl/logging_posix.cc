// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>
#include <sstream>

#include "platform/impl/logging.h"
#include "platform/impl/logging_test.h"
#include "util/trace_logging.h"

namespace openscreen {
namespace {

int g_log_fd = STDERR_FILENO;
LogLevel g_log_level = LogLevel::kWarning;
std::vector<std::string>* g_log_messages_for_test = nullptr;

std::ostream& operator<<(std::ostream& os, const LogLevel& level) {
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
  os << level_string;
  return os;
}

}  // namespace

void SetLogFifoOrDie(const char* filename) {
  if (g_log_fd != STDERR_FILENO) {
    close(g_log_fd);
    g_log_fd = STDERR_FILENO;
  }

  // Note: The use of OSP_CHECK/OSP_LOG_* here will log to stderr.
  struct stat st = {};
  int open_result = -1;
  if (stat(filename, &st) == -1 && errno == ENOENT) {
    if (mkfifo(filename, 0644) == 0) {
      open_result = open(filename, O_WRONLY);
      OSP_CHECK_NE(open_result, -1)
          << "open(" << filename << ") failed: " << strerror(errno);
    } else {
      OSP_LOG_FATAL << "mkfifo(" << filename << ") failed: " << strerror(errno);
    }
  } else if (S_ISFIFO(st.st_mode)) {
    open_result = open(filename, O_WRONLY);
    OSP_CHECK_NE(open_result, -1)
        << "open(" << filename << ") failed: " << strerror(errno);
  } else {
    OSP_LOG_FATAL << "not a FIFO special file: " << filename;
  }

  // Direct all logging to the opened FIFO file.
  g_log_fd = open_result;
}

void SetLogLevel(LogLevel level) {
  g_log_level = level;
}

LogLevel GetLogLevel() {
  return g_log_level;
}

bool IsLoggingOn(LogLevel level, const char* file) {
  // Possible future enhancement: Use glob patterns passed on the command-line
  // to use a different logging level for certain files, like in Chromium.
  return level >= g_log_level;
}

void LogWithLevel(LogLevel level,
                  const char* file,
                  int line,
                  std::stringstream message) {
  if (level < g_log_level)
    return;

  std::stringstream ss;
  ss << "[" << level << ":" << file << "(" << line << "):T" << std::hex
     << TRACE_CURRENT_ID << "] " << message.rdbuf() << '\n';
  const auto ss_str = ss.str();
  const auto bytes_written = write(g_log_fd, ss_str.c_str(), ss_str.size());
  OSP_DCHECK(bytes_written);
  if (g_log_messages_for_test) {
    g_log_messages_for_test->push_back(ss_str);
  }
}

void LogTraceMessage(const std::string& message) {
  const std::string to_write = message + '\n';
  const auto bytes_written = write(g_log_fd, to_write.c_str(), to_write.size());
  OSP_DCHECK(bytes_written);
}

[[noreturn]] void Break() {
// Generally this will just resolve to an abort anyways, but gives the
// compiler a chance to peform a more appropriate, target specific trap
// as appropriate.
#if defined(_DEBUG)
  __builtin_trap();
#else
  std::abort();
#endif
}

void SetLogBufferForTest(std::vector<std::string>* messages) {
  g_log_messages_for_test = messages;
}

}  // namespace openscreen
