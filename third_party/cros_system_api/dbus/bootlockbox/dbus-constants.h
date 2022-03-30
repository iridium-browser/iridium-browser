// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_BOOTLOCKBOX_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_BOOTLOCKBOX_DBUS_CONSTANTS_H_

namespace cryptohome {

// Interface exposed by the boot lockbox daemon.

const char kBootLockboxInterface[] = "org.chromium.BootLockboxInterface";
const char kBootLockboxServicePath[] = "/org/chromium/BootLockbox";
const char kBootLockboxServiceName[] = "org.chromium.BootLockbox";

const char kBootLockboxStoreBootLockbox[] = "StoreBootLockbox";
const char kBootLockboxReadBootLockbox[] = "ReadBootLockbox";
const char kBootLockboxFinalizeBootLockbox[] = "FinalizeBootLockbox";

}  // namespace cryptohome

#endif  // SYSTEM_API_DBUS_BOOTLOCKBOX_DBUS_CONSTANTS_H_
