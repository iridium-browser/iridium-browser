// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_TEXT_TRACE_LOGGING_PLATFORM_H_
#define PLATFORM_IMPL_TEXT_TRACE_LOGGING_PLATFORM_H_

#include "platform/api/trace_logging_platform.h"

namespace openscreen {

class TextTraceLoggingPlatform : public TraceLoggingPlatform {
 public:
  TextTraceLoggingPlatform();
  ~TextTraceLoggingPlatform() override;

  bool IsTraceLoggingEnabled(TraceCategory::Value category) override;

  void LogTrace(const char* name,
                const uint32_t line,
                const char* file,
                Clock::time_point start_time,
                Clock::time_point end_time,
                TraceIdHierarchy ids,
                Error::Code error) override;

  void LogAsyncStart(const char* name,
                     const uint32_t line,
                     const char* file,
                     Clock::time_point timestamp,
                     TraceIdHierarchy ids) override;

  void LogAsyncEnd(const uint32_t line,
                   const char* file,
                   Clock::time_point timestamp,
                   TraceId trace_id,
                   Error::Code error) override;
};

}  // namespace openscreen

#endif  // PLATFORM_IMPL_TEXT_TRACE_LOGGING_PLATFORM_H_
