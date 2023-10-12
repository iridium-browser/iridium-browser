// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

#include "platform/api/task_runner.h"
#include "util/osp_logging.h"
#include "util/scoped_wake_lock.h"

namespace openscreen {

class ScopedWakeLockMac : public ScopedWakeLock {
 public:
  explicit ScopedWakeLockMac(TaskRunner& task_runner);
  ~ScopedWakeLockMac() override;

 private:
  TaskRunner& task_runner_;

  struct LockState {
    int reference_count = 0;
    IOPMAssertionID assertion_id = kIOPMNullAssertionID;
  };

  static void AcquireWakeLock();
  static void ReleaseWakeLock();

  static LockState lock_state_;
};

SerialDeletePtr<ScopedWakeLock> ScopedWakeLock::Create(
    TaskRunner& task_runner) {
  return SerialDeletePtr<ScopedWakeLock>(task_runner,
                                         new ScopedWakeLockMac(task_runner));
}

ScopedWakeLockMac::ScopedWakeLockMac(TaskRunner& task_runner)
    : ScopedWakeLock(), task_runner_(task_runner) {
  task_runner_.PostTask([] {
    if (lock_state_.reference_count++ == 0) {
      AcquireWakeLock();
    }
  });
}

ScopedWakeLockMac::~ScopedWakeLockMac() {
  task_runner_.PostTask([] {
    if (--lock_state_.reference_count == 0) {
      ReleaseWakeLock();
    }
  });
}

// static
void ScopedWakeLockMac::AcquireWakeLock() {
  // The new way of doing an IOPM assertion requires constructing a standard
  // Foundation dictionary and adding the expected properties.
  CFMutableDictionaryRef assertion_properties = CFDictionaryCreateMutable(
      0, 3, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  // This property means that we are requesting that the display not dim
  // or go to sleep.
  CFDictionarySetValue(assertion_properties, kIOPMAssertionTypeKey,
                       kIOPMAssertionTypeNoDisplaySleep);
  CFDictionarySetValue(assertion_properties, kIOPMAssertionNameKey,
                       CFSTR("Open Screen ScopedWakeLock"));

  const IOReturn result = IOPMAssertionCreateWithProperties(
      assertion_properties, &lock_state_.assertion_id);
  OSP_DCHECK_EQ(result, kIOReturnSuccess);
}

// static
void ScopedWakeLockMac::ReleaseWakeLock() {
  const IOReturn result = IOPMAssertionRelease(lock_state_.assertion_id);
  OSP_DCHECK_EQ(result, kIOReturnSuccess);
}

ScopedWakeLockMac::LockState ScopedWakeLockMac::lock_state_{};

}  // namespace openscreen
