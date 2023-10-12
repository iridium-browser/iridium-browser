// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_API_TRACE_EVENT_H_
#define PLATFORM_API_TRACE_EVENT_H_

#include <string>
#include <utility>
#include <vector>

#include "platform/api/time.h"
#include "platform/base/error.h"
#include "platform/base/trace_logging_activation.h"
#include "platform/base/trace_logging_types.h"

namespace openscreen {

// A collection of common properties of trace events.
struct TraceEvent {
  // Constructor with only the required fields.
  TraceEvent(TraceCategory category,
             Clock::time_point start_time,
             const char* name,
             const char* file_name,
             uint32_t line_number);

  TraceEvent();
  TraceEvent(TraceEvent&&) noexcept;
  TraceEvent(const TraceEvent&);
  TraceEvent& operator=(TraceEvent&&);
  TraceEvent& operator=(const TraceEvent&);
  ~TraceEvent();

  std::string ToString() const;

  // May be called to truncate all std::strings on this object.
  static const size_t kMaxStringLength{1024};
  void TruncateStrings();

  // The category of this event.
  TraceCategory category;

  // Timestamp for when the event was created.
  Clock::time_point start_time;

  // Name of this operation.
  const char* name{nullptr};

  // Name of the file the log was generated in.
  const char* file_name{nullptr};

  // Line number the log was generated on.
  uint32_t line_number{0};

  // The trace ids of this event and its ancestors.
  TraceIdHierarchy ids;

  // Optional result of the trace event.
  Error::Code result{Error::Code::kNone};

  // Optional list of arguments. May contain 0, 1, or 2 arguments.
  // Excess arguments will remain unused.
  using Argument = std::pair<const char*, std::string>;
  std::vector<Argument> arguments;
};

}  // namespace openscreen

#endif  // PLATFORM_API_TRACE_EVENT_H_
