// Copyright 2021 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_SPACED_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_SPACED_DBUS_CONSTANTS_H_

namespace spaced {

const char kSpacedInterface[] = "org.chromium.Spaced";
const char kSpacedServicePath[] = "/org/chromium/Spaced";
const char kSpacedServiceName[] = "org.chromium.Spaced";

// Methods.
const char kGetFreeDiskSpaceMethod[] = "GetFreeDiskSpace";
const char kGetTotalDiskSpaceMethod[] = "GetTotalDiskSpace";
const char kGetRootDeviceSizeMethod[] = "GetRootDeviceSize";

}  // namespace spaced

#endif  // SYSTEM_API_DBUS_SPACED_DBUS_CONSTANTS_H_
