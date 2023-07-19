// Copyright 2013 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_CONSTANTS_CRYPTOHOME_H_
#define SYSTEM_API_CONSTANTS_CRYPTOHOME_H_

#include <stdint.h>

namespace cryptohome {

// Cleanup is trigerred if the amount of free disk space goes below this value.
const int64_t kMinFreeSpaceInBytes = 512 * 1LL << 20;
// Flag file in temporary storage. The presence of the file means the device
// is locked to be able to access only a single user data, until reboot.
constexpr char kLockedToSingleUserFile[] =
    "/run/cryptohome/locked_to_single_user";
// Path to the mount namespace to run user sessions in.
constexpr char kUserSessionMountNamespacePath[] = "/run/namespaces/mnt_chrome";

}  // namespace cryptohome

#endif  // SYSTEM_API_CONSTANTS_CRYPTOHOME_H_
