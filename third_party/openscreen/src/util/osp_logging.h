// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_OSP_LOGGING_H_
#define UTIL_OSP_LOGGING_H_

#include <sstream>
#include <utility>

#include "platform/api/logging.h"

namespace openscreen {
namespace internal {

// The stream-based logging macros below are adapted from Chromium's
// base/logging.h.
class LogMessage {
 public:
  LogMessage(LogLevel level, const char* file, int line)
      : level_(level), file_(file), line_(line) {}

  ~LogMessage() {
    LogWithLevel(level_, file_, line_, std::move(stream_));
    if (level_ == LogLevel::kFatal) {
      Break();
    }
  }

  std::ostream& stream() { return stream_; }

 protected:
  const LogLevel level_;

  // The file here comes from the __FILE__ macro, which should persist while
  // we are doing the logging. Hence, keeping it unmanaged here and not
  // creating a copy should be safe.
  const char* const file_;
  const int line_;
  std::stringstream stream_;
};

// Used by the OSP_LAZY_STREAM macro to return void after evaluating an ostream
// chain expression.
class Voidify {
 public:
  void operator&(std::ostream&) {}
};

}  // namespace internal
}  // namespace openscreen

#define OSP_LAZY_STREAM(condition, stream) \
  !(condition) ? (void)0 : openscreen::internal::Voidify() & (stream)
#define OSP_LOG_IS_ON(level_enum) \
  openscreen::IsLoggingOn(openscreen::LogLevel::level_enum, __FILE__)
#define OSP_LOG_STREAM(level_enum)                                             \
  openscreen::internal::LogMessage(openscreen::LogLevel::level_enum, __FILE__, \
                                   __LINE__)                                   \
      .stream()

#define OSP_VLOG \
  OSP_LAZY_STREAM(OSP_LOG_IS_ON(kVerbose), OSP_LOG_STREAM(kVerbose))
#define OSP_LOG_INFO \
  OSP_LAZY_STREAM(OSP_LOG_IS_ON(kInfo), OSP_LOG_STREAM(kInfo))
#define OSP_LOG_WARN \
  OSP_LAZY_STREAM(OSP_LOG_IS_ON(kWarning), OSP_LOG_STREAM(kWarning))
#define OSP_LOG_ERROR \
  OSP_LAZY_STREAM(OSP_LOG_IS_ON(kError), OSP_LOG_STREAM(kError))
#define OSP_LOG_FATAL \
  OSP_LAZY_STREAM(OSP_LOG_IS_ON(kFatal), OSP_LOG_STREAM(kFatal))

#define OSP_VLOG_IF(condition) !(condition) ? (void)0 : OSP_VLOG
#define OSP_LOG_IF(level, condition) !(condition) ? (void)0 : OSP_LOG_##level

#define OSP_CHECK(condition) \
  OSP_LOG_IF(FATAL, !(condition)) << "OSP_CHECK(" << #condition << ") failed: "

#define OSP_CHECK_EQ(a, b) \
  OSP_CHECK((a) == (b)) << (a) << " vs. " << (b) << ": "
#define OSP_CHECK_NE(a, b) \
  OSP_CHECK((a) != (b)) << (a) << " vs. " << (b) << ": "
#define OSP_CHECK_LT(a, b) OSP_CHECK((a) < (b)) << (a) << " vs. " << (b) << ": "
#define OSP_CHECK_LE(a, b) \
  OSP_CHECK((a) <= (b)) << (a) << " vs. " << (b) << ": "
#define OSP_CHECK_GT(a, b) OSP_CHECK((a) > (b)) << (a) << " vs. " << (b) << ": "
#define OSP_CHECK_GE(a, b) \
  OSP_CHECK((a) >= (b)) << (a) << " vs. " << (b) << ": "

#if defined(_DEBUG) || defined(DCHECK_ALWAYS_ON)
#define OSP_DCHECK_IS_ON() 1
#define OSP_DCHECK(condition) OSP_CHECK(condition)
#define OSP_DCHECK_EQ(a, b) OSP_CHECK_EQ(a, b)
#define OSP_DCHECK_NE(a, b) OSP_CHECK_NE(a, b)
#define OSP_DCHECK_LT(a, b) OSP_CHECK_LT(a, b)
#define OSP_DCHECK_LE(a, b) OSP_CHECK_LE(a, b)
#define OSP_DCHECK_GT(a, b) OSP_CHECK_GT(a, b)
#define OSP_DCHECK_GE(a, b) OSP_CHECK_GE(a, b)
#else
#define OSP_DCHECK_IS_ON() 0
// When DCHECKs are off, nothing will be logged. Use that fact to make
// references to the |condition| expression (or |a| and |b|) so the compiler
// won't emit unused variable warnings/errors when DCHECKs are turned off.
#define OSP_EAT_STREAM OSP_LOG_IF(FATAL, false)
#define OSP_DCHECK(condition) OSP_EAT_STREAM << !(condition)
#define OSP_DCHECK_EQ(a, b) OSP_EAT_STREAM << !((a) == (b))
#define OSP_DCHECK_NE(a, b) OSP_EAT_STREAM << !((a) != (b))
#define OSP_DCHECK_LT(a, b) OSP_EAT_STREAM << !((a) < (b))
#define OSP_DCHECK_LE(a, b) OSP_EAT_STREAM << !((a) <= (b))
#define OSP_DCHECK_GT(a, b) OSP_EAT_STREAM << !((a) > (b))
#define OSP_DCHECK_GE(a, b) OSP_EAT_STREAM << !((a) >= (b))
#endif

#define OSP_DVLOG OSP_VLOG_IF(OSP_DCHECK_IS_ON())
#define OSP_DLOG_INFO OSP_LOG_IF(INFO, OSP_DCHECK_IS_ON())
#define OSP_DLOG_WARN OSP_LOG_IF(WARN, OSP_DCHECK_IS_ON())
#define OSP_DLOG_ERROR OSP_LOG_IF(ERROR, OSP_DCHECK_IS_ON())
#define OSP_DLOG_FATAL OSP_LOG_IF(FATAL, OSP_DCHECK_IS_ON())
#define OSP_DVLOG_IF(condition) OSP_VLOG_IF(OSP_DCHECK_IS_ON() && (condition))
#define OSP_DLOG_IF(level, condition) \
  OSP_LOG_IF(level, OSP_DCHECK_IS_ON() && (condition))

// Log when unimplemented code points are reached: If verbose logging is turned
// on, log always. Otherwise, just attempt to log once.
#define OSP_UNIMPLEMENTED()                                           \
  if (OSP_LOG_IS_ON(kVerbose)) {                                      \
    OSP_LOG_STREAM(kVerbose) << __func__ << ": UNIMPLEMENTED() hit."; \
  } else {                                                            \
    static bool needs_warning = true;                                 \
    if (needs_warning) {                                              \
      OSP_LOG_WARN << __func__ << ": UNIMPLEMENTED() hit.";           \
      needs_warning = false;                                          \
    }                                                                 \
  }

// Since Break() is annotated as noreturn, this will properly signal to the
// compiler that this code is truly not reached (and thus doesn't need a return
// statement for non-void returning functions/methods).
#define OSP_NOTREACHED()                                \
  {                                                     \
    OSP_LOG_FATAL << __func__ << ": NOTREACHED() hit."; \
    Break();                                            \
  }

#endif  // UTIL_OSP_LOGGING_H_
