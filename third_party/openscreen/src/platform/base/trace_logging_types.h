// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_BASE_TRACE_LOGGING_TYPES_H_
#define PLATFORM_BASE_TRACE_LOGGING_TYPES_H_

#include <stdint.h>

#include <limits>
#include <sstream>
#include <string>

namespace openscreen {

// Define TraceId type here since other TraceLogging files import it.
using TraceId = uint64_t;

// kEmptyTraceId is the Trace ID when tracing at a global level, not inside any
// tracing block - ie this will be the parent ID for a top level tracing block.
constexpr TraceId kEmptyTraceId = 0x0;

// kUnsetTraceId is the Trace ID passed in to the tracing library when no user-
// specified value is desired.
constexpr TraceId kUnsetTraceId = std::numeric_limits<TraceId>::max();

// A class to represent the current TraceId Hierarchy and for the user to
// pass around as needed.
struct TraceIdHierarchy {
  TraceId current;
  TraceId parent;
  TraceId root;

  static constexpr TraceIdHierarchy Empty() {
    return {kEmptyTraceId, kEmptyTraceId, kEmptyTraceId};
  }

  bool HasCurrent() const { return current != kUnsetTraceId; }
  bool HasParent() const { return parent != kUnsetTraceId; }
  bool HasRoot() const { return root != kUnsetTraceId; }

  std::string ToString() const;
};

std::ostream& operator<<(std::ostream& out, const TraceIdHierarchy& ids);

bool operator==(const TraceIdHierarchy& lhs, const TraceIdHierarchy& rhs);

bool operator!=(const TraceIdHierarchy& lhs, const TraceIdHierarchy& rhs);

// Supported trace category
enum class TraceCategory : int {
  kAny,
  kMdns,
  kQuic,
  kSsl,
  kPresentation,
  kStandaloneReceiver,
  kDiscovery,
  kStandaloneSender,
  kReceiver,
  kSender
};

const char* ToString(TraceCategory category);

}  // namespace openscreen

#endif  // PLATFORM_BASE_TRACE_LOGGING_TYPES_H_
