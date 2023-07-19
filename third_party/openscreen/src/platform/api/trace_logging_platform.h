// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_API_TRACE_LOGGING_PLATFORM_H_
#define PLATFORM_API_TRACE_LOGGING_PLATFORM_H_

#include <string>
#include <utility>
#include <vector>

#include "platform/api/time.h"
#include "platform/api/trace_event.h"
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

  // Determines whether trace logging is enabled for the given category. Note
  // that if any categories are supported, this function should return "true"
  // when called with TraceCategory::kAny.
  virtual bool IsTraceLoggingEnabled(TraceCategory category) = 0;

  // Log a synchronous trace.
  virtual void LogTrace(TraceEvent event, Clock::time_point end_time) = 0;

  // Log an asynchronous trace start.
  virtual void LogAsyncStart(TraceEvent event) = 0;

  // Log an asynchronous trace end.
  virtual void LogAsyncEnd(TraceEvent event) = 0;
};

}  // namespace openscreen

#endif  // PLATFORM_API_TRACE_LOGGING_PLATFORM_H_
