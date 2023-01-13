// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_SWAP_MANAGEMENT_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_SWAP_MANAGEMENT_DBUS_CONSTANTS_H_

namespace swap_management {

constexpr char kSwapManagementInterface[] = "org.chromium.SwapManagement";
constexpr char kSwapManagementServicePath[] = "/org/chromium/SwapManagement";
constexpr char kSwapManagementServiceName[] = "org.chromium.SwapManagement";

// Methods.
constexpr char kSwapEnable[] = "SwapEnable";
constexpr char kSwapDisable[] = "SwapDisable";
constexpr char kSwapStartStop[] = "SwapStartStop";
constexpr char kSwapStatus[] = "SwapStatus";
constexpr char kSwapSetParameter[] = "SwapSetParameter";
constexpr char kSwapZramEnableWriteback[] = "SwapZramEnableWriteback";
constexpr char kSwapZramSetWritebackLimit[] = "SwapZramSetWritebackLimit";
constexpr char kSwapZramMarkIdle[] = "SwapZramMarkIdle";
constexpr char kSwapZramWriteback[] = "InitiateSwapZramWriteback";
constexpr char kMGLRUSetEnable[] = "MGLRUSetEnable";

// ZramWritebackMode contains the allowed modes of operation
// for zram writeback. The definition is in:
// src/platform2/swap_management/dbus_binding/org.chromium.SwapManagement.xml
enum ZramWritebackMode {
  WRITEBACK_IDLE = 0x001,
  WRITEBACK_HUGE = 0x002,
  WRITEBACK_HUGE_IDLE = 0x004,
};

}  // namespace swap_management

#endif  // SYSTEM_API_DBUS_SWAP_MANAGEMENT_DBUS_CONSTANTS_H_
