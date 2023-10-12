// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/text_trace_logging_platform.h"

#include <limits>
#include <sstream>

#include "platform/impl/logging.h"
#include "util/chrono_helpers.h"
#include "util/osp_logging.h"

namespace openscreen {

using clock_operators::operator<<;

bool TextTraceLoggingPlatform::IsTraceLoggingEnabled(TraceCategory category) {
  return true;
}

TextTraceLoggingPlatform::TextTraceLoggingPlatform() {
  StartTracing(this);
}

TextTraceLoggingPlatform::~TextTraceLoggingPlatform() {
  StopTracing();
}

void TextTraceLoggingPlatform::LogTrace(TraceEvent event,
                                        Clock::time_point end_time) {
  const auto total_runtime = (end_time - event.start_time);
  std::stringstream ss;
  ss << "[TRACE"
     << " (" << std::dec << total_runtime << ")] " << event.ToString();
  LogTraceMessage(ss.str());
}

void TextTraceLoggingPlatform::LogAsyncStart(TraceEvent event) {
  std::stringstream ss;
  ss << "[ASYNC TRACE START]  " << event.ToString();
  LogTraceMessage(ss.str());
}

void TextTraceLoggingPlatform::LogAsyncEnd(TraceEvent event) {
  std::stringstream ss;
  ss << "[ASYNC TRACE END] " << event.ToString();
  LogTraceMessage(ss.str());
}

}  // namespace openscreen
