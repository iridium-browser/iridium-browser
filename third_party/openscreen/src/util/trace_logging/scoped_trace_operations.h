// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_TRACE_LOGGING_SCOPED_TRACE_OPERATIONS_H_
#define UTIL_TRACE_LOGGING_SCOPED_TRACE_OPERATIONS_H_

#include <atomic>
#include <cstring>
#include <memory>
#include <stack>
#include <utility>
#include <vector>

#include "platform/api/time.h"
#include "platform/api/trace_logging_platform.h"
#include "platform/base/error.h"
#include "platform/base/trace_logging_types.h"
#include "util/osp_logging.h"

#if defined(ENABLE_TRACE_LOGGING)

namespace openscreen {
namespace internal {

// A base class for all trace logging objects which will create new entries in
// the Trace Hierarchy.
// 1) The sharing of all static and thread_local variables across template
// specializations.
// 2) Including all children in the same traces vector.
class ScopedTraceOperation {
 public:
  // Define the destructor to remove this item from the stack when it's
  // destroyed.
  virtual ~ScopedTraceOperation();

  // Getters the current Trace Hierarchy. If the traces_ stack hasn't been
  // created yet, return as if the empty root node is there.
  static TraceId current_id() {
    return traces_ == nullptr ? kEmptyTraceId : traces_->top()->trace_id_;
  }

  static TraceId root_id() {
    return traces_ == nullptr ? kEmptyTraceId : traces_->top()->root_id_;
  }

  static TraceIdHierarchy hierarchy() {
    if (traces_ == nullptr) {
      return TraceIdHierarchy::Empty();
    }

    return traces_->top()->to_hierarchy();
  }

  // Static method to set the result of the most recent trace.
  static void set_result(const Error& error) { set_result(error.code()); }
  static void set_result(Error::Code error) {
    if (traces_ == nullptr) {
      return;
    }
    traces_->top()->SetTraceResult(error);
  }

  // Traces the end of an asynchronous call.
  // NOTE: This returns a bool rather than a void because it keeps the syntax of
  // the ternary operator in the macros simpler.
  static bool TraceAsyncEnd(const uint32_t line,
                            const char* file,
                            TraceId id,
                            Error::Code e);

 protected:
  // Sets the result of this trace log.
  // NOTE: this must be define in this class rather than TraceLogger so that it
  // can be called on traces.back() without a potentially unsafe cast or type
  // checking at runtime.
  virtual void SetTraceResult(Error::Code error) = 0;

  // Constructor to set all trace id information.
  ScopedTraceOperation(TraceId current_id = kUnsetTraceId,
                       TraceId parent_id = kUnsetTraceId,
                       TraceId root_id = kUnsetTraceId);

  // Current TraceId information.
  TraceId trace_id_;
  TraceId parent_id_;
  TraceId root_id_;

  TraceIdHierarchy to_hierarchy() { return {trace_id_, parent_id_, root_id_}; }

 private:
  // NOTE: A std::vector is used for backing the stack because it provides the
  // best perf. Further perf improvement could be achieved later by swapping
  // this out for a circular buffer once OSP supports that. Additional details
  // can be found here:
  // https://www.codeproject.com/Articles/1185449/Performance-of-a-Circular-Buffer-vs-Vector-Deque-a
  using TraceStack =
      std::stack<ScopedTraceOperation*, std::vector<ScopedTraceOperation*>>;

  // Counter to pick IDs when it is not provided.
  static std::atomic<std::uint64_t> trace_id_counter_;

  // The LIFO stack of TraceLoggers currently being watched by this
  // thread.
  static thread_local TraceStack* traces_;
  static thread_local ScopedTraceOperation* root_node_;

  OSP_DISALLOW_COPY_AND_ASSIGN(ScopedTraceOperation);
};

// The class which does actual trace logging.
class TraceLoggerBase : public ScopedTraceOperation {
 public:
  TraceLoggerBase(TraceCategory category,
                  const char* name,
                  const char* file,
                  uint32_t line,
                  std::vector<TraceEvent::Argument> arguments = {},
                  TraceId current = kUnsetTraceId,
                  TraceId parent = kUnsetTraceId,
                  TraceId root = kUnsetTraceId);

  TraceLoggerBase(TraceCategory category,
                  const char* name,
                  const char* file,
                  uint32_t line,
                  std::vector<TraceEvent::Argument> arguments,
                  TraceIdHierarchy ids);

 protected:
  // Set the result.
  void SetTraceResult(Error::Code error) override { event_.result = error; }

  TraceEvent event_;

 private:
  OSP_DISALLOW_COPY_AND_ASSIGN(TraceLoggerBase);
};

class SynchronousTraceLogger : public TraceLoggerBase {
 public:
  using TraceLoggerBase::TraceLoggerBase;

  ~SynchronousTraceLogger() override;

 private:
  OSP_DISALLOW_COPY_AND_ASSIGN(SynchronousTraceLogger);
};

class AsynchronousTraceLogger : public TraceLoggerBase {
 public:
  using TraceLoggerBase::TraceLoggerBase;

  ~AsynchronousTraceLogger() override;

 private:
  OSP_DISALLOW_COPY_AND_ASSIGN(AsynchronousTraceLogger);
};

// Inserts a fake element into the ScopedTraceOperation stack to set
// the current TraceId Hierarchy manually.
class TraceIdSetter final : public ScopedTraceOperation {
 public:
  explicit TraceIdSetter(TraceIdHierarchy ids)
      : ScopedTraceOperation(ids.current, ids.parent, ids.root) {}
  ~TraceIdSetter() final;

  // Creates a new TraceIdSetter to set the full TraceId Hierarchy to default
  // values and does not push it to the traces stack.
  static TraceIdSetter* CreateStackRootNode();

 private:
  // Implement abstract method for use in Macros.
  void SetTraceResult(Error::Code error) {}

  OSP_DISALLOW_COPY_AND_ASSIGN(TraceIdSetter);
};

// This helper object allows us to delete objects allocated on the stack in a
// unique_ptr.
template <class T>
class TraceInstanceHelper {
 private:
  class TraceOperationOnStackDeleter {
   public:
    void operator()(T* ptr) { ptr->~T(); }
  };

  using TraceInstanceWrapper = std::unique_ptr<T, TraceOperationOnStackDeleter>;

 public:
  template <typename... Args>
  static TraceInstanceWrapper Create(uint8_t storage[sizeof(T)], Args... args) {
    return TraceInstanceWrapper(new (storage) T(std::forward<Args&&>(args)...));
  }

  static TraceInstanceWrapper Empty() { return TraceInstanceWrapper(); }
};

}  // namespace internal
}  // namespace openscreen

#endif  // defined(ENABLE_TRACE_LOGGING)

#endif  // UTIL_TRACE_LOGGING_SCOPED_TRACE_OPERATIONS_H_
