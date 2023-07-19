// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Based on
// https://cs.chromium.org/chromium/src/v8/src/base/platform/semaphore.cc

#include "util/semaphore.h"

#include "base/logging.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

#if defined(OS_MACOSX)

Semaphore::Semaphore(int count) {
  native_handle_ = dispatch_semaphore_create(count);
  DCHECK(native_handle_);
}

Semaphore::~Semaphore() {
  dispatch_release(native_handle_);
}

void Semaphore::Signal() {
  dispatch_semaphore_signal(native_handle_);
}

void Semaphore::Wait() {
  dispatch_semaphore_wait(native_handle_, DISPATCH_TIME_FOREVER);
}

#elif defined(OS_POSIX)

Semaphore::Semaphore(int count) {
  DCHECK_GE(count, 0);
  int result = sem_init(&native_handle_, 0, count);
  DCHECK_EQ(0, result);
}

Semaphore::~Semaphore() {
  int result = sem_destroy(&native_handle_);
  DCHECK_EQ(0, result);
}

void Semaphore::Signal() {
  int result = sem_post(&native_handle_);
  // This check may fail with <libc-2.21, which we use on the try bots, if the
  // semaphore is destroyed while sem_post is still executed. A work around is
  // to extend the lifetime of the semaphore.
  CHECK_EQ(0, result);
}

void Semaphore::Wait() {
  while (true) {
    int result = sem_wait(&native_handle_);
    if (result == 0)
      return;  // Semaphore was signalled.
    // Signal caused spurious wakeup.
    DCHECK_EQ(-1, result);
    DCHECK_EQ(EINTR, errno);
  }
}

#elif defined(OS_WIN)

Semaphore::Semaphore(int count) {
  DCHECK_GE(count, 0);
  native_handle_ = ::CreateSemaphoreA(nullptr, count, 0x7FFFFFFF, nullptr);
  DCHECK(native_handle_);
}

Semaphore::~Semaphore() {
  BOOL result = CloseHandle(native_handle_);
  DCHECK(result);
}

void Semaphore::Signal() {
  LONG dummy;
  BOOL result = ReleaseSemaphore(native_handle_, 1, &dummy);
  DCHECK(result);
}

void Semaphore::Wait() {
  DWORD result = WaitForSingleObject(native_handle_, INFINITE);
  DCHECK(result == WAIT_OBJECT_0);
}

#endif
