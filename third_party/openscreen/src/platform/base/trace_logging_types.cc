// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/base/trace_logging_types.h"

#include <cstdlib>
#include <limits>

namespace openscreen {

std::string TraceIdHierarchy::ToString() const {
  std::stringstream ss;
  ss << "[" << std::hex << (HasRoot() ? root : 0) << ":"
     << (HasParent() ? parent : 0) << ":" << (HasCurrent() ? current : 0)
     << std::dec << "]";

  return ss.str();
}

std::ostream& operator<<(std::ostream& out, const TraceIdHierarchy& ids) {
  return out << ids.ToString();
}

bool operator==(const TraceIdHierarchy& lhs, const TraceIdHierarchy& rhs) {
  return lhs.current == rhs.current && lhs.parent == rhs.parent &&
         lhs.root == rhs.root;
}

bool operator!=(const TraceIdHierarchy& lhs, const TraceIdHierarchy& rhs) {
  return !(lhs == rhs);
}

const char* ToString(TraceCategory category) {
  switch (category) {
    case TraceCategory::kAny:
      return "Any";
    case TraceCategory::kMdns:
      return "Mdns";
    case TraceCategory::kQuic:
      return "Quic";
    case TraceCategory::kSsl:
      return "SSL";
    case TraceCategory::kPresentation:
      return "Presentation";
    case TraceCategory::kStandaloneReceiver:
      return "StandaloneReceiver";
    case TraceCategory::kDiscovery:
      return "Discovery";
    case TraceCategory::kStandaloneSender:
      return "StandaloneSender";
    case TraceCategory::kReceiver:
      return "Receiver";
    case TraceCategory::kSender:
      return "Sender";
  }

  // OSP_NOTREACHED is not available in platform/base.
  std::abort();
}

}  // namespace openscreen
