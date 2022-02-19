// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Based on
// https://cs.chromium.org/chromium/src/v8/src/base/platform/semaphore.h

#ifndef UTIL_SEMAPHORE_H_
#define UTIL_SEMAPHORE_H_

#include "util/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#elif defined(OS_MACOSX)
#include <dispatch/dispatch.h>
#elif defined(OS_ZOS)
#include "zos-semaphore.h"
#elif defined(OS_POSIX)
#include <semaphore.h>
#else
#error Port.
#endif

class Semaphore {
 public:
  explicit Semaphore(int count);
  ~Semaphore();

  // Increments the semaphore counter.
  void Signal();

  // Decrements the semaphore counter if it is positive, or blocks until it
  // becomes positive and then decrements the counter.
  void Wait();

#if defined(OS_MACOSX)
  using NativeHandle = dispatch_semaphore_t;
#elif defined(OS_POSIX)
  using NativeHandle = sem_t;
#elif defined(OS_WIN)
  using NativeHandle = HANDLE;
#endif

  NativeHandle& native_handle() { return native_handle_; }
  const NativeHandle& native_handle() const { return native_handle_; }

 private:
  NativeHandle native_handle_;

  Semaphore(const Semaphore&) = delete;
  Semaphore& operator=(const Semaphore&) = delete;
};

#endif  // UTIL_SEMAPHORE_H_
