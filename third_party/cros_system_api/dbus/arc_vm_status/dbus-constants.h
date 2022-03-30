// Copyright 2022 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_ARC_VM_STATUS_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_ARC_VM_STATUS_DBUS_CONSTANTS_H_

namespace arc_vm_status {

constexpr char kArcVmStatusServicePath[] = "/org/chromium/ArcVmStatusService";
constexpr char kArcVmStatusServiceInterface[] =
    "org.chromium.ArcVmStatusService";
constexpr char kArcVmStatusBootCompleteSignal[] = "ArcVmBootCompleteSignal";

}  // namespace arc_vm_status

#endif  // SYSTEM_API_DBUS_ARC_VM_STATUS_DBUS_CONSTANTS_H_
