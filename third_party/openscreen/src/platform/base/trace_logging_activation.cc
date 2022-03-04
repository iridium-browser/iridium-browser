// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/base/trace_logging_activation.h"

#include <atomic>
#include <cassert>
#include <thread>

namespace openscreen {

namespace {

// If tracing is active, this is a valid pointer to an object that implements
// the TraceLoggingPlatform interface. If tracing is not active, this is
// nullptr.
std::atomic<TraceLoggingPlatform*> g_current_destination{};

// The count of threads currently calling into the current TraceLoggingPlatform.
std::atomic<int> g_use_count{};

inline TraceLoggingPlatform* PinCurrentDestination() {
  // NOTE: It's important to increment the global use count *before* loading the
  // pointer, to ensure the referent is pinned-down (i.e., any thread executing
  // StopTracing() stays blocked) until CurrentTracingDestination's destructor
  // calls UnpinCurrentDestination().
  g_use_count.fetch_add(1);
  return g_current_destination.load(std::memory_order_relaxed);
}

inline void UnpinCurrentDestination() {
  g_use_count.fetch_sub(1);
}

}  // namespace

void StartTracing(TraceLoggingPlatform* destination) {
  assert(destination);
  auto* const old_destination = g_current_destination.exchange(destination);
  (void)old_destination;  // Prevent "unused variable" compiler warnings.
  assert(old_destination == nullptr || old_destination == destination);
}

void StopTracing() {
  auto* const old_destination = g_current_destination.exchange(nullptr);
  if (!old_destination) {
    return;  // Already stopped.
  }

  // Block the current thread until the global use count goes to zero. At that
  // point, there can no longer be any dangling references. Theoretically, this
  // loop may never terminate; but in practice, that should never happen. If it
  // did happen, that would mean one or more CPU cores are continuously spending
  // most of their time executing the TraceLoggingPlatform methods, yet those
  // methods are supposed to be super-cheap and take near-zero time to execute!
  int iters = 0;
  while (g_use_count.load(std::memory_order_relaxed) != 0) {
    assert(iters < 1024);
    std::this_thread::yield();
    ++iters;
  }
}

CurrentTracingDestination::CurrentTracingDestination()
    : destination_(PinCurrentDestination()) {}

CurrentTracingDestination::~CurrentTracingDestination() {
  UnpinCurrentDestination();
}

}  // namespace openscreen
