// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_BASE_TRACE_LOGGING_ACTIVATION_H_
#define PLATFORM_BASE_TRACE_LOGGING_ACTIVATION_H_

namespace openscreen {

class TraceLoggingPlatform;

// Start or Stop trace logging. It is illegal to call StartTracing() a second
// time without having called StopTracing() to stop the prior tracing session.
//
// Note that StopTracing() may block until all threads have returned from any
// in-progress calls into the TraceLoggingPlatform's methods.
void StartTracing(TraceLoggingPlatform* destination);
void StopTracing();

// An immutable, non-copyable and non-movable smart pointer that references the
// current trace logging destination. If tracing was active when this class was
// intantiated, the pointer is valid for the life of the instance, and can be
// used to directly invoke the methods of the TraceLoggingPlatform API. If
// tracing was not active when this class was intantiated, the pointer is null
// for the life of the instance and must not be dereferenced.
//
// An instance should be short-lived, as a platform's call to StopTracing() will
// be blocked until there are no instances remaining.
//
// NOTE: This is generally not used directly, but instead via the
// util/trace_logging macros.
class CurrentTracingDestination {
 public:
  CurrentTracingDestination();
  ~CurrentTracingDestination();

  explicit operator bool() const noexcept { return !!destination_; }
  TraceLoggingPlatform* operator->() const noexcept { return destination_; }

 private:
  CurrentTracingDestination(const CurrentTracingDestination&) = delete;
  CurrentTracingDestination(CurrentTracingDestination&&) = delete;
  CurrentTracingDestination& operator=(const CurrentTracingDestination&) =
      delete;
  CurrentTracingDestination& operator=(CurrentTracingDestination&&) = delete;

  // The destination at the time this class was constructed, and is valid for
  // the lifetime of this class. This is nullptr if tracing was inactive.
  TraceLoggingPlatform* const destination_;
};

}  // namespace openscreen

#endif  // PLATFORM_BASE_TRACE_LOGGING_ACTIVATION_H_
