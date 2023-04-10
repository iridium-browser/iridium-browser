// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/api/trace_event.h"

#include <sstream>

namespace openscreen {

TraceEvent::TraceEvent(TraceCategory category,
                       Clock::time_point start_time,
                       const char* name,
                       const char* file_name,
                       uint32_t line_number)
    : category(category),
      start_time(start_time),
      name(name),
      file_name(file_name),
      line_number(line_number) {}

TraceEvent::TraceEvent() = default;
TraceEvent::TraceEvent(TraceEvent&&) noexcept = default;
TraceEvent::TraceEvent(const TraceEvent&) = default;
TraceEvent& TraceEvent::operator=(TraceEvent&&) = default;
TraceEvent& TraceEvent::operator=(const TraceEvent&) = default;
TraceEvent::~TraceEvent() = default;

std::string TraceEvent::ToString() const {
  std::ostringstream oss;

  oss << ids << " " << openscreen::ToString(category) << "::" << name << " <"
      << file_name << ":" << line_number << ">";

  // We only support two arguments in total.
  if (!arguments.empty()) {
    oss << " { " << arguments[0].first << ": " << arguments[0].second;
    if (arguments.size() > 1) {
      oss << ", " << arguments[1].first << ": " << arguments[1].second;
    }
    oss << " }";
  }
  return oss.str();
}

void TraceEvent::TruncateStrings() {
  for (auto& argument : arguments) {
    if (argument.second.size() > kMaxStringLength) {
      argument.second.resize(kMaxStringLength);
      // Populate last three digits with ellipses to indicate that
      // we truncated this string.
      argument.second.replace(kMaxStringLength - 3, 3, "...");
    }
  }
}

}  // namespace openscreen
