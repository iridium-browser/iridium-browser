// Copyright 2021 The ChromiumOS Authors
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
constexpr char kTypecdCableWarning[] = "CableWarning";

// Methods.
constexpr char kTypecdSetPeripheralDataAccessMethod[] =
    "SetPeripheralDataAccess";

enum class DeviceConnectedType {
  kThunderboltOnly = 0,
  // Device supports both Thunderbolt & DisplayPort alternate modes.
  kThunderboltDp = 1,
};

enum class CableWarningType {
  // Reserved for generic cable warnings
  kOther = 0,
  // Partner supports DisplayAlt mode, but the cable does not.
  kInvalidDpCable = 1,
  // Partner supports USB4, but the cable restricts the connection to TBT.
  kInvalidUSB4ValidTBTCable = 2,
  // Partner supports USB4, but the cable prevents USB4 and TBT mode entry.
  kInvalidUSB4Cable = 3,
  // Partner supports TBT, but the cable prevents TBT mode entry.
  kInvalidTBTCable = 4,
  // USB speed is limited by the cable.
  kSpeedLimitingCable = 5,
};

}  // namespace typecd

#endif  // SYSTEM_API_DBUS_TYPECD_DBUS_CONSTANTS_H_
