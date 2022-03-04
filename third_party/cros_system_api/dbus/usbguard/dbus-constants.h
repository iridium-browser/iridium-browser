// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_USBGUARD_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_USBGUARD_DBUS_CONSTANTS_H_

namespace usbguard {
constexpr char kUsbguardServiceName[] = "org.usbguard1";
constexpr char kUsbguardDevicesInterface[] = "org.usbguard.Devices1";
constexpr char kUsbguardDevicesInterfacePath[] = "/org/usbguard1/Devices";
constexpr char kDevicePolicyChangedSignalName[] = "DevicePolicyChanged";
}  // namespace usbguard

#endif  // SYSTEM_API_DBUS_USBGUARD_DBUS_CONSTANTS_H_
