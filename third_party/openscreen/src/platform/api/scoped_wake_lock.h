// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_API_SCOPED_WAKE_LOCK_H_
#define PLATFORM_API_SCOPED_WAKE_LOCK_H_

#include <memory>

#include "platform/api/serial_delete_ptr.h"
#include "platform/api/task_runner.h"

namespace openscreen {

// Ensures that the device does not got to sleep. This is used, for example,
// while Open Screen is communicating with peers over the network for things
// like media streaming.
//
// The wake lock is RAII: It is automatically engaged when the ScopedWakeLock is
// created and released when the ScopedWakeLock is destroyed. Open Screen code
// may sometimes create multiple instances. In that case, the wake lock should
// be engaged upon creating the first instance, and then held until all
// instances have been destroyed.
class ScopedWakeLock {
 public:
  static SerialDeletePtr<ScopedWakeLock> Create(TaskRunner* task_runner);

  // Instances are not copied nor moved.
  ScopedWakeLock(const ScopedWakeLock&) = delete;
  ScopedWakeLock(ScopedWakeLock&&) = delete;
  ScopedWakeLock& operator=(const ScopedWakeLock&) = delete;
  ScopedWakeLock& operator=(ScopedWakeLock&&) = delete;

  ScopedWakeLock();
  virtual ~ScopedWakeLock();
};

}  // namespace openscreen

#endif  // PLATFORM_API_SCOPED_WAKE_LOCK_H_
