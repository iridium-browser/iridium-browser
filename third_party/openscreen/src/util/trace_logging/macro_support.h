// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_TRACE_LOGGING_MACRO_SUPPORT_H_
#define UTIL_TRACE_LOGGING_MACRO_SUPPORT_H_

#ifndef INCLUDING_FROM_UTIL_TRACE_LOGGING_H_
#error "Do not include this header directly. Use util/trace_logging.h."
#endif

#ifndef ENABLE_TRACE_LOGGING
#error "BUG: This file should not have been reached."
#endif

#include "platform/api/trace_logging_platform.h"
#include "platform/base/trace_logging_activation.h"
#include "platform/base/trace_logging_types.h"
#include "util/trace_logging/scoped_trace_operations.h"

// Helper macros. These are used to simplify the macros below.
// NOTE: These cannot be #undef'd or they will stop working outside this file.
// NOTE: Two of these below macros are intentionally the same. This is to work
// around optimizations in the C++ Precompiler.
#define TRACE_INTERNAL_CONCAT(a, b) a##b
#define TRACE_INTERNAL_CONCAT_CONST(a, b) TRACE_INTERNAL_CONCAT(a, b)
#define TRACE_INTERNAL_UNIQUE_VAR_NAME(a) \
  TRACE_INTERNAL_CONCAT_CONST(a, __LINE__)

// Because we need to suppress unused variables, and this code is used
// repeatedly in below macros, define helper macros to do this on a per-compiler
// basis until we begin using C++ 17 which supports [[maybe_unused]] officially.
#if defined(__clang__)
#define TRACE_INTERNAL_IGNORE_UNUSED_VAR [[maybe_unused]]
#elif defined(__GNUC__)
#define TRACE_INTERNAL_IGNORE_UNUSED_VAR __attribute__((unused))
#else
#define TRACE_INTERNAL_IGNORE_UNUSED_VAR [[maybe_unused]]
#endif  // defined(__clang__)

namespace openscreen {
namespace internal {

inline bool IsTraceLoggingEnabled(TraceCategory category) {
  const CurrentTracingDestination destination;
  return destination && destination->IsTraceLoggingEnabled(category);
}

}  // namespace internal
}  // namespace openscreen

#define TRACE_IS_ENABLED(category) \
  openscreen::internal::IsTraceLoggingEnabled(category)

// Internal logging macros.
#define TRACE_SET_HIERARCHY_INTERNAL(line, ids)                            \
  alignas(32) uint8_t TRACE_INTERNAL_CONCAT_CONST(                         \
      tracing_storage, line)[sizeof(openscreen::internal::TraceIdSetter)]; \
  TRACE_INTERNAL_IGNORE_UNUSED_VAR                                         \
  const auto TRACE_INTERNAL_UNIQUE_VAR_NAME(trace_ref_) =                  \
      TRACE_IS_ENABLED(openscreen::TraceCategory::kAny)                    \
          ? openscreen::internal::TraceInstanceHelper<                     \
                openscreen::internal::TraceIdSetter>::                     \
                Create(TRACE_INTERNAL_CONCAT_CONST(tracing_storage, line), \
                       ids)                                                \
          : openscreen::internal::TraceInstanceHelper<                     \
                openscreen::internal::TraceIdSetter>::Empty()

#define TRACE_SCOPED_INTERNAL(line, category, name, ...)                   \
  alignas(32) uint8_t TRACE_INTERNAL_CONCAT_CONST(                         \
      tracing_storage,                                                     \
      line)[sizeof(openscreen::internal::SynchronousTraceLogger)];         \
  TRACE_INTERNAL_IGNORE_UNUSED_VAR                                         \
  const auto TRACE_INTERNAL_UNIQUE_VAR_NAME(trace_ref_) =                  \
      TRACE_IS_ENABLED(category)                                           \
          ? openscreen::internal::TraceInstanceHelper<                     \
                openscreen::internal::SynchronousTraceLogger>::            \
                Create(TRACE_INTERNAL_CONCAT_CONST(tracing_storage, line), \
                       category, name, __FILE__, __LINE__, ##__VA_ARGS__)  \
          : openscreen::internal::TraceInstanceHelper<                     \
                openscreen::internal::SynchronousTraceLogger>::Empty()

#define TRACE_ASYNC_START_INTERNAL(line, category, name, ...)             \
  alignas(32) uint8_t TRACE_INTERNAL_CONCAT_CONST(                        \
      temp_storage,                                                       \
      line)[sizeof(openscreen::internal::AsynchronousTraceLogger)];       \
  TRACE_INTERNAL_IGNORE_UNUSED_VAR                                        \
  const auto TRACE_INTERNAL_UNIQUE_VAR_NAME(trace_ref_) =                 \
      TRACE_IS_ENABLED(category)                                          \
          ? openscreen::internal::TraceInstanceHelper<                    \
                openscreen::internal::AsynchronousTraceLogger>::          \
                Create(TRACE_INTERNAL_CONCAT_CONST(temp_storage, line),   \
                       category, name, __FILE__, __LINE__, ##__VA_ARGS__) \
          : openscreen::internal::TraceInstanceHelper<                    \
                openscreen::internal::AsynchronousTraceLogger>::Empty()

#endif  // UTIL_TRACE_LOGGING_MACRO_SUPPORT_H_
