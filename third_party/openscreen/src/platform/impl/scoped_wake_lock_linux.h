// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_SCOPED_WAKE_LOCK_LINUX_H_
#define PLATFORM_IMPL_SCOPED_WAKE_LOCK_LINUX_H_

#include "platform/api/scoped_wake_lock.h"

namespace openscreen {

class ScopedWakeLockLinux : public ScopedWakeLock {
 public:
  ScopedWakeLockLinux();
  ~ScopedWakeLockLinux() override;

 private:
  // TODO(jophba): implement linux wake lock.
  static void AcquireWakeLock();
  static void ReleaseWakeLock();

  static int reference_count_;
};

}  // namespace openscreen

#endif  // PLATFORM_IMPL_SCOPED_WAKE_LOCK_LINUX_H_
