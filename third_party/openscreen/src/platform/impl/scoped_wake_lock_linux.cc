// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/scoped_wake_lock_linux.h"

#include "platform/api/task_runner.h"
#include "platform/impl/platform_client_posix.h"
#include "util/osp_logging.h"

namespace openscreen {

int ScopedWakeLockLinux::reference_count_ = 0;

SerialDeletePtr<ScopedWakeLock> ScopedWakeLock::Create(
    TaskRunner* task_runner) {
  return SerialDeletePtr<ScopedWakeLock>(task_runner,
                                         new ScopedWakeLockLinux());
}

namespace {

TaskRunner* GetTaskRunner() {
  auto* const platform_client = PlatformClientPosix::GetInstance();
  OSP_DCHECK(platform_client);
  auto* const task_runner = platform_client->GetTaskRunner();
  OSP_DCHECK(task_runner);
  return task_runner;
}

}  // namespace

ScopedWakeLockLinux::ScopedWakeLockLinux() : ScopedWakeLock() {
  OSP_DCHECK(GetTaskRunner()->IsRunningOnTaskRunner());
  if (reference_count_++ == 0) {
    AcquireWakeLock();
  }
}

ScopedWakeLockLinux::~ScopedWakeLockLinux() {
  GetTaskRunner()->PostTask([] {
    if (--reference_count_ == 0) {
      ReleaseWakeLock();
    }
  });
}

// static
void ScopedWakeLockLinux::AcquireWakeLock() {
  OSP_VLOG << "Acquired wake lock: currently a noop";
}

// static
void ScopedWakeLockLinux::ReleaseWakeLock() {
  OSP_VLOG << "Released wake lock: currently a noop";
}

}  // namespace openscreen
