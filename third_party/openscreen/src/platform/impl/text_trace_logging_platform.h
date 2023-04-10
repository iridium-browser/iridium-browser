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

  bool IsTraceLoggingEnabled(TraceCategory category) override;

  void LogTrace(TraceEvent event, Clock::time_point end_time) override;

  void LogAsyncStart(TraceEvent event) override;

  void LogAsyncEnd(TraceEvent event) override;
};

}  // namespace openscreen

#endif  // PLATFORM_IMPL_TEXT_TRACE_LOGGING_PLATFORM_H_
