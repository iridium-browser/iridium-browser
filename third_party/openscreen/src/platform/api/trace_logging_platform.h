// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_API_TRACE_LOGGING_PLATFORM_H_
#define PLATFORM_API_TRACE_LOGGING_PLATFORM_H_

#include "platform/api/time.h"
#include "platform/base/error.h"
#include "platform/base/trace_logging_activation.h"
#include "platform/base/trace_logging_types.h"

namespace openscreen {

// Optional platform API to support logging trace events from Open Screen. To
// use this, implement the TraceLoggingPlatform interface and call
// StartTracing() and StopTracing() to turn tracing on/off (see
// platform/base/trace_logging_activation.h).
//
// All methods must be thread-safe and re-entrant.
class TraceLoggingPlatform {
 public:
  virtual ~TraceLoggingPlatform();

  // Determines whether trace logging is enabled for the given category.
  virtual bool IsTraceLoggingEnabled(TraceCategory::Value category) = 0;

  // Log a synchronous trace.
  virtual void LogTrace(const char* name,
                        const uint32_t line,
                        const char* file,
                        Clock::time_point start_time,
                        Clock::time_point end_time,
                        TraceIdHierarchy ids,
                        Error::Code error) = 0;

  // Log an asynchronous trace start.
  virtual void LogAsyncStart(const char* name,
                             const uint32_t line,
                             const char* file,
                             Clock::time_point timestamp,
                             TraceIdHierarchy ids) = 0;

  // Log an asynchronous trace end.
  virtual void LogAsyncEnd(const uint32_t line,
                           const char* file,
                           Clock::time_point timestamp,
                           TraceId trace_id,
                           Error::Code error) = 0;
};

}  // namespace openscreen

#endif  // PLATFORM_API_TRACE_LOGGING_PLATFORM_H_
