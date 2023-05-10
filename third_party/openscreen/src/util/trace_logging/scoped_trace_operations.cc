// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/trace_logging/scoped_trace_operations.h"

#include "absl/types/optional.h"
#include "platform/api/trace_logging_platform.h"
#include "platform/base/trace_logging_activation.h"
#include "util/osp_logging.h"

#if defined(ENABLE_TRACE_LOGGING)

namespace openscreen {
namespace internal {

// static
bool ScopedTraceOperation::TraceAsyncEnd(const uint32_t line,
                                         const char* file,
                                         TraceId id,
                                         Error::Code e) {
  const CurrentTracingDestination destination;
  if (destination) {
    TraceEvent end_event;
    end_event.start_time = Clock::now();
    end_event.line_number = line;
    end_event.file_name = file;
    end_event.ids.current = id;
    end_event.result = e;
    destination->LogAsyncEnd(std::move(end_event));
    return true;
  }
  return false;
}

ScopedTraceOperation::ScopedTraceOperation(TraceId trace_id,
                                           TraceId parent_id,
                                           TraceId root_id) {
  if (traces_ == nullptr) {
    // Create the stack if it doesnt' exist.
    traces_ = new TraceStack();

    // Create a new root node. This will re-call this constructor and add the
    // root node to the stack before proceeding with the original node.
    root_node_ = new TraceIdSetter(TraceIdHierarchy::Empty());
    OSP_DCHECK(!traces_->empty());
  }

  // Setting trace id fields.
  root_id_ = root_id != kUnsetTraceId ? root_id : traces_->top()->root_id_;
  parent_id_ =
      parent_id != kUnsetTraceId ? parent_id : traces_->top()->trace_id_;
  trace_id_ =
      trace_id != kUnsetTraceId ? trace_id : trace_id_counter_.fetch_add(1);

  // Add this item to the stack.
  traces_->push(this);
  OSP_DCHECK(traces_->size() < 1024);
}

ScopedTraceOperation::~ScopedTraceOperation() {
  OSP_CHECK(traces_ != nullptr && !traces_->empty());
  OSP_CHECK_EQ(traces_->top(), this);
  traces_->pop();

  // If there's only one item left, it must be the root node. Deleting the root
  // node will re-call this destructor and delete the traces_ stack.
  if (traces_->size() == 1) {
    OSP_CHECK_EQ(traces_->top(), root_node_);
    delete root_node_;
    root_node_ = nullptr;
  } else if (traces_->empty()) {
    delete traces_;
    traces_ = nullptr;
  }
}

// static
thread_local ScopedTraceOperation::TraceStack* ScopedTraceOperation::traces_ =
    nullptr;

// static
thread_local ScopedTraceOperation* ScopedTraceOperation::root_node_ = nullptr;

// static
std::atomic<std::uint64_t> ScopedTraceOperation::trace_id_counter_{
    uint64_t{0x01} << (sizeof(TraceId) * 8 - 1)};

TraceLoggerBase::TraceLoggerBase(TraceCategory category,
                                 const char* name,
                                 const char* file,
                                 uint32_t line,
                                 std::vector<TraceEvent::Argument> arguments,
                                 TraceId current,
                                 TraceId parent,
                                 TraceId root)
    : ScopedTraceOperation(current, parent, root),
      event_(category, Clock::now(), name, file, line) {
  event_.arguments = std::move(arguments);
  event_.TruncateStrings();
}

TraceLoggerBase::TraceLoggerBase(TraceCategory category,
                                 const char* name,
                                 const char* file,
                                 uint32_t line,
                                 std::vector<TraceEvent::Argument> arguments,
                                 TraceIdHierarchy ids)
    : TraceLoggerBase(category,
                      name,
                      file,
                      line,
                      std::move(arguments),
                      ids.current,
                      ids.parent,
                      ids.root) {}

SynchronousTraceLogger::~SynchronousTraceLogger() {
  const CurrentTracingDestination destination;
  if (destination) {
    const auto end_time = Clock::now();
    event_.ids = to_hierarchy();
    destination->LogTrace(event_, end_time);
  }
}

AsynchronousTraceLogger::~AsynchronousTraceLogger() {
  const CurrentTracingDestination destination;
  if (destination) {
    event_.ids = to_hierarchy();
    destination->LogAsyncStart(event_);
  }
}

TraceIdSetter::~TraceIdSetter() = default;

}  // namespace internal
}  // namespace openscreen

#endif  // defined(ENABLE_TRACE_LOGGING)
