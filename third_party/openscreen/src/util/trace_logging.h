// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_TRACE_LOGGING_H_
#define UTIL_TRACE_LOGGING_H_

#include <string>
#include <utility>
#include <vector>

#include "platform/base/trace_logging_types.h"

// All compile-time macros for tracing.
// NOTE: The ternary operator is used here to ensure that the TraceLogger object
// is only constructed if tracing is enabled, but at the same time is created in
// the caller's scope. The C++ standards guide guarantees that the constructor
// should only be called when IsTraceLoggingEnabled(...) evaluates to true.
// static_cast calls are used because if the type of the result of the ternary
// operator does not match the expected type, temporary storage is used for the
// created object, which results in an extra call to the constructor and
// destructor of the tracing objects.
//
// Further details about how these macros are used can be found in
// docs/trace_logging.md.

#if defined(ENABLE_TRACE_LOGGING)

#define INCLUDING_FROM_UTIL_TRACE_LOGGING_H_
#include "util/trace_logging/macro_support.h"
#undef INCLUDING_FROM_UTIL_TRACE_LOGGING_H_

#define TRACE_SET_RESULT(result)                                      \
  do {                                                                \
    if (TRACE_IS_ENABLED(openscreen::TraceCategory::kAny)) {          \
      openscreen::internal::ScopedTraceOperation::set_result(result); \
    }                                                                 \
  } while (false)
#define TRACE_SET_HIERARCHY(ids) TRACE_SET_HIERARCHY_INTERNAL(__LINE__, ids)
#define TRACE_HIERARCHY                                          \
  (TRACE_IS_ENABLED(openscreen::TraceCategory::kAny)             \
       ? openscreen::internal::ScopedTraceOperation::hierarchy() \
       : openscreen::TraceIdHierarchy::Empty())
#define TRACE_CURRENT_ID                                          \
  (TRACE_IS_ENABLED(openscreen::TraceCategory::kAny)              \
       ? openscreen::internal::ScopedTraceOperation::current_id() \
       : kEmptyTraceId)
#define TRACE_ROOT_ID                                          \
  (TRACE_IS_ENABLED(openscreen::TraceCategory::kAny)           \
       ? openscreen::internal::ScopedTraceOperation::root_id() \
       : kEmptyTraceId)

// NOTE: this method requires arguments to be passed as already serialized
// strings.
inline std::vector<openscreen::TraceEvent::Argument> ToArgumentArray(
    const char* argname = {},
    std::string argval = {},
    const char* argname_two = {},
    std::string argval_two = {}) {
  std::vector<openscreen::TraceEvent::Argument> out;
  if (argname) {
    out.emplace_back(argname, std::move(argval));
  }
  if (argname_two) {
    out.emplace_back(argname_two, std::move(argval_two));
  }
  return out;
}

// Synchronous Trace Macros.
//
// Scoped traces with no arguments.
#define TRACE_SCOPED(category, name, ...)                            \
  TRACE_SCOPED_INTERNAL(__LINE__, category, name, ToArgumentArray(), \
                        ##__VA_ARGS__)
#define TRACE_DEFAULT_SCOPED(category, ...) \
  TRACE_SCOPED(category, __PRETTY_FUNCTION__, ##__VA_ARGS__)

// Scoped traces with one argument.
#define TRACE_SCOPED1(category, name, argname, argval, ...) \
  TRACE_SCOPED_INTERNAL(__LINE__, category, name,           \
                        ToArgumentArray(argname, argval), ##__VA_ARGS__)
#define TRACE_DEFAULT_SCOPED1(category, argname, argval, ...) \
  TRACE_SCOPED1(category, __PRETTY_FUNCTION__, argname, argval, ##__VA_ARGS__)

// Scoped traces with two arguments.
#define TRACE_SCOPED2(category, name, argname, argval, argname_two, \
                      argval_two, ...)                              \
  TRACE_SCOPED_INTERNAL(                                            \
      __LINE__, category, name,                                     \
      ToArgumentArray(argname, argval, argname_two, argval_two),    \
      ##__VA_ARGS__)
#define TRACE_DEFAULT_SCOPED2(category, argname, argval, argname_two,        \
                              argval_two, ...)                               \
  TRACE_SCOPED2(category, __PRETTY_FUNCTION__, argname, argval, argname_two, \
                argval_two, ##__VA_ARGS__)

// Asynchronous Trace Macros.
#define TRACE_ASYNC_START(category, name, ...)                            \
  TRACE_ASYNC_START_INTERNAL(__LINE__, category, name, ToArgumentArray(), \
                             ##__VA_ARGS__)

#define TRACE_ASYNC_START1(category, name, argname, argval, ...) \
  TRACE_ASYNC_START_INTERNAL(__LINE__, category, name,           \
                             ToArgumentArray(argname, argval), ##__VA_ARGS__)

#define TRACE_ASYNC_START2(category, name, argname, argval, argname_two, \
                           argval_two, ...)                              \
  TRACE_ASYNC_START_INTERNAL(                                            \
      __LINE__, category, name,                                          \
      ToArgumentArray(argname, argval, argname_two, argval_two),         \
      ##__VA_ARGS__)

#define TRACE_ASYNC_END(category, id, result)                  \
  TRACE_IS_ENABLED(category)                                   \
  ? openscreen::internal::ScopedTraceOperation::TraceAsyncEnd( \
        __LINE__, __FILE__, id, result)                        \
  : false

#else  // ENABLE_TRACE_LOGGING not defined

namespace openscreen {
namespace internal {
// Consumes |args| (to avoid "warn unused variable" errors at compile time), and
// provides a "void" result type in the macros below.
template <typename... Args>
inline void DoNothingForTracing(Args... args) {}
}  // namespace internal
}  // namespace openscreen

#define TRACE_SET_RESULT(result) \
  openscreen::internal::DoNothingForTracing(result)
#define TRACE_SET_HIERARCHY(ids) openscreen::internal::DoNothingForTracing(ids)
#define TRACE_HIERARCHY openscreen::TraceIdHierarchy::Empty()
#define TRACE_CURRENT_ID openscreen::kEmptyTraceId
#define TRACE_ROOT_ID openscreen::kEmptyTraceId
#define TRACE_SCOPED(category, name, ...) \
  openscreen::internal::DoNothingForTracing(category, name, ##__VA_ARGS__)
#define TRACE_DEFAULT_SCOPED(category, ...) \
  TRACE_SCOPED(category, __PRETTY_FUNCTION__, ##__VA_ARGS__)

#define TRACE_SCOPED1(category, name, argname, argval, ...)                  \
  openscreen::internal::DoNothingForTracing(category, name, argname, argval, \
                                            ##__VA_ARGS__)
#define TRACE_DEFAULT_SCOPED1(category, argname, argval, ...) \
  TRACE_SCOPED1(category, __PRETTY_FUNCTION__, argname, argval, ##__VA_ARGS__)

#define TRACE_SCOPED2(category, name, argname, argval, argname_two, \
                      argval_two, ...)                              \
  openscreen::internal::DoNothingForTracing(                        \
      category, name, argname, argval, argname_two, argval_two, ##__VA_ARGS__)
#define TRACE_DEFAULT_SCOPED2(category, argname, argval, argname_two,        \
                              argval_two, ...)                               \
  TRACE_SCOPED2(category, __PRETTY_FUNCTION__, argname, argval, argname_two, \
                argval_two, ##__VA_ARGS__)

#define TRACE_ASYNC_START(category, name, ...) \
  openscreen::internal::DoNothingForTracing(category, name, ##__VA_ARGS__)
#define TRACE_ASYNC_END(category, id, result) \
  openscreen::internal::DoNothingForTracing(category, id, result)

#endif  // defined(ENABLE_TRACE_LOGGING)

#endif  // UTIL_TRACE_LOGGING_H_
