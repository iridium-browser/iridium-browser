// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_VM_CONCIERGE_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_VM_CONCIERGE_DBUS_CONSTANTS_H_

namespace vm_tools {
namespace concierge {

const char kVmConciergeInterface[] = "org.chromium.VmConcierge";
const char kVmConciergeServicePath[] = "/org/chromium/VmConcierge";
const char kVmConciergeServiceName[] = "org.chromium.VmConcierge";

const char kStartVmMethod[] = "StartVm";
const char kStopVmMethod[] = "StopVm";
const char kStopAllVmsMethod[] = "StopAllVms";
const char kSuspendVmMethod[] = "SuspendVm";
const char kResumeVmMethod[] = "ResumeVm";
const char kGetVmInfoMethod[] = "GetVmInfo";
const char kGetVmEnterpriseReportingInfoMethod[] =
    "GetVmEnterpriseReportingInfo";
const char kCreateDiskImageMethod[] = "CreateDiskImage";
const char kDestroyDiskImageMethod[] = "DestroyDiskImage";
const char kResizeDiskImageMethod[] = "ResizeDiskImage";
const char kExportDiskImageMethod[] = "ExportDiskImage";
const char kImportDiskImageMethod[] = "ImportDiskImage";
const char kCancelDiskImageMethod[] = "CancelDiskImageOperation";
const char kDiskImageStatusMethod[] = "DiskImageStatus";
const char kListVmDisksMethod[] = "ListVmDisks";
const char kStartContainerMethod[] = "StartContainer";
const char kGetContainerSshKeysMethod[] = "GetContainerSshKeys";
const char kSyncVmTimesMethod[] = "SyncVmTimes";
const char kAttachUsbDeviceMethod[] = "AttachUsbDevice";
const char kDetachUsbDeviceMethod[] = "DetachUsbDevice";
const char kListUsbDeviceMethod[] = "ListUsbDevices";
const char kStartPluginVmMethod[] = "StartPluginVm";
const char kGetDnsSettingsMethod[] = "GetDnsSettings";
const char kStartArcVmMethod[] = "StartArcVm";
const char kSetVmCpuRestrictionMethod[] = "SetVmCpuRestriction";
const char kAdjustVmMethod[] = "AdjustVm";
const char kSetVmIdMethod[] = "SetVmId";
const char kReclaimVmMemoryMethod[] = "ReclaimVmMemory";
const char kMakeRtVcpuMethod[] = "MakeRtVcpu";

const char kContainerStartupFailedSignal[] = "ContainerStartupFailed";
const char kDiskImageProgressSignal[] = "DiskImageProgress";
const char kDnsSettingsChangedSignal[] = "DnsSettingsChanged";
const char kVmStartedSignal[] = "VmStartedSignal";
const char kVmStartingUpSignal[] = "VmStartingUpSignal";
const char kVmStoppedSignal[] = "VmStoppedSignal";
const char kVmIdChangedSignal[] = "VmIdChangedSignal";

}  // namespace concierge
}  // namespace vm_tools

#endif  // SYSTEM_API_DBUS_VM_CONCIERGE_DBUS_CONSTANTS_H_
