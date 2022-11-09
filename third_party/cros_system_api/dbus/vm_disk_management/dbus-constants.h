// Copyright 2021 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_VM_DISK_MANAGEMENT_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_VM_DISK_MANAGEMENT_DBUS_CONSTANTS_H_

namespace vm_tools {
namespace disk_management {

const char kVmDiskManagementServiceName[] =
    "org.chromium.VmDiskManagementService";
const char kVmDiskManagementServicePath[] =
    "/org/chromium/VmDiskManagementService";
const char kVmDiskManagementServiceInterface[] =
    "org.chromium.VmDiskManagementService";

const char kVmDiskManagementServiceGetDiskInfoMethod[] = "GetDiskInfo";
const char kVmDiskManagementServiceRequestSpaceMethod[] = "RequestSpace";
const char kVmDiskManagementServiceReleaseSpaceMethod[] = "ReleaseSpace";

}  // namespace disk_management
}  // namespace vm_tools

#endif  // SYSTEM_API_DBUS_VM_DISK_MANAGEMENT_DBUS_CONSTANTS_H_
