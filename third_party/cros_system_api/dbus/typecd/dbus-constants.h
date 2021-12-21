// Copyright 2021 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_TYPECD_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_TYPECD_DBUS_CONSTANTS_H_

namespace typecd {

constexpr char kTypecdServiceName[] = "org.chromium.typecd";
constexpr char kTypecdServiceInterface[] = "org.chromium.typecd";
constexpr char kTypecdServicePath[] = "/org/chromium/typecd";

// Signals.
constexpr char kTypecdDeviceConnected[] = "DeviceConnected";

enum class DeviceConnectedType {
  kThunderboltOnly = 0,
  // Device supports both Thunderbolt & DisplayPort alternate modes.
  kThunderboltDp = 1,
};

}  // namespace typecd

#endif  // SYSTEM_API_DBUS_TYPECD_DBUS_CONSTANTS_H_
