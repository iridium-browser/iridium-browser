// Copyright 2019 The libgav1 Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/utils/logging.h"

#include <cstdarg>
#include <cstdio>
#include <sstream>
#include <thread>  // NOLINT (unapproved c++11 header)

#if !defined(LIBGAV1_LOG_LEVEL)
#define LIBGAV1_LOG_LEVEL (1 << 30)
#endif

namespace libgav1 {
namespace internal {
#if LIBGAV1_ENABLE_LOGGING
namespace {

const char* LogSeverityName(LogSeverity severity) {
  switch (severity) {
    case LogSeverity::kInfo:
      return "INFO";
    case LogSeverity::kError:
      return "ERROR";
    case LogSeverity::kWarning:
      return "WARNING";
  }
  return "UNKNOWN";
}

}  // namespace

void Log(LogSeverity severity, const char* file, int line, const char* format,
         ...) {
  if (LIBGAV1_LOG_LEVEL < static_cast<int>(severity)) return;
  std::ostringstream ss;
  ss << std::hex << std::this_thread::get_id();
  fprintf(stderr, "%s %s %s:%d] ", LogSeverityName(severity), ss.str().c_str(),
          file, line);

  va_list ap;
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  fprintf(stderr, "\n");
}
#else   // !LIBGAV1_ENABLE_LOGGING
void Log(LogSeverity /*severity*/, const char* /*file*/, int /*line*/,
         const char* /*format*/, ...) {}
#endif  // LIBGAV1_ENABLE_LOGGING

}  // namespace internal
}  // namespace libgav1
