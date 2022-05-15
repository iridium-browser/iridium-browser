// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_SMBFS_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_SMBFS_DBUS_CONSTANTS_H_

namespace smbfs {

// General
const char kSmbFsInterface[] = "org.chromium.SmbFs";
const char kSmbFsServicePath[] = "/org/chromium/SmbFs";
const char kSmbFsServiceName[] = "org.chromium.SmbFs";

// Methods
const char kOpenIpcChannelMethod[] = "OpenIpcChannel";

}  // namespace smbfs

#endif  // SYSTEM_API_DBUS_SMBFS_DBUS_CONSTANTS_H_
