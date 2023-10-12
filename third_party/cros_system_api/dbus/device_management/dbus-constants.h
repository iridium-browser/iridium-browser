// Copyright 2023 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_DEVICE_MANAGEMENT_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_DEVICE_MANAGEMENT_DBUS_CONSTANTS_H_

namespace device_management {

inline constexpr char kDeviceManagementServiceName[] =
    "org.chromium.DeviceManagement";
inline constexpr char kDeviceManagementServicePath[] =
    "/org/chromium/DeviceManagement";

// Interface exposed by the device_management daemon
inline constexpr char kDeviceManagementInterface[] =
    "org.chromium.DeviceManagement";

// Methods of the |kDeviceManagementInterface| interface:
inline constexpr char kInstallAttributesGet[] = "InstallAttributesGet";
inline constexpr char kInstallAttributesSet[] = "InstallAttributesSet";
inline constexpr char kInstallAttributesFinalize[] =
    "InstallAttributesFinalize";
inline constexpr char kInstallAttributesGetStatus[] =
    "InstallAttributesGetStatus";
inline constexpr char kGetFirmwareManagementParameters[] =
    "GetFirmwareManagementParameters";
inline constexpr char kRemoveFirmwareManagementParameters[] =
    "RemoveFirmwareManagementParameters";
inline constexpr char kSetFirmwareManagementParameters[] =
    "SetFirmwareManagementParameters";

}  // namespace device_management

#endif  // SYSTEM_API_DBUS_DEVICE_MANAGEMENT_DBUS_CONSTANTS_H_
