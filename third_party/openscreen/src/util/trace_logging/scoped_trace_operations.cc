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
  auto end_time = Clock::now();
  const CurrentTracingDestination destination;
  if (destination) {
    destination->LogAsyncEnd(line, file, end_time, id, e);
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

TraceLoggerBase::TraceLoggerBase(TraceCategory::Value category,
                                 const char* name,
                                 const char* file,
                                 uint32_t line,
                                 TraceId current,
                                 TraceId parent,
                                 TraceId root)
    : ScopedTraceOperation(current, parent, root),
      start_time_(Clock::now()),
      result_(Error::Code::kNone),
      name_(name),
      file_name_(file),
      line_number_(line),
      category_(category) {}

TraceLoggerBase::TraceLoggerBase(TraceCategory::Value category,
                                 const char* name,
                                 const char* file,
                                 uint32_t line,
                                 TraceIdHierarchy ids)
    : TraceLoggerBase(category,
                      name,
                      file,
                      line,
                      ids.current,
                      ids.parent,
                      ids.root) {}

SynchronousTraceLogger::~SynchronousTraceLogger() {
  const CurrentTracingDestination destination;
  if (destination) {
    auto end_time = Clock::now();
    destination->LogTrace(this->name_, this->line_number_, this->file_name_,
                          this->start_time_, end_time, this->to_hierarchy(),
                          this->result_);
  }
}

AsynchronousTraceLogger::~AsynchronousTraceLogger() {
  const CurrentTracingDestination destination;
  if (destination) {
    destination->LogAsyncStart(this->name_, this->line_number_,
                               this->file_name_, this->start_time_,
                               this->to_hierarchy());
  }
}

TraceIdSetter::~TraceIdSetter() = default;

}  // namespace internal
}  // namespace openscreen

#endif  // defined(ENABLE_TRACE_LOGGING)
