// Copyright 2018 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_VM_PLUGIN_DISPATCHER_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_VM_PLUGIN_DISPATCHER_DBUS_CONSTANTS_H_

namespace vm_tools {
namespace plugin_dispatcher {

const char kVmPluginDispatcherInterface[] = "org.chromium.VmPluginDispatcher";
const char kVmPluginDispatcherServicePath[] =
    "/org/chromium/VmPluginDispatcher";
const char kVmPluginDispatcherServiceName[] = "org.chromium.VmPluginDispatcher";

const char kShutdownDispatherMethod[] = "ShutdownDispatcher";
const char kSendProblemReportMethod[] = "SendProblemReport";

const char kRegisterVmMethod[] = "RegisterVm";
const char kUnregisterVmMethod[] = "UnregisterVm";

const char kListVmsMethod[] = "ListVms";

const char kStartVmMethod[] = "StartVm";
const char kStopVmMethod[] = "StopVm";
const char kSuspendVmMethod[] = "SuspendVm";
const char kResetVmMethod[] = "ResetVm";

const char kShowVmMethod[] = "ShowVm";

const char kVmStateChangedSignal[] = "VmStateChanged";
const char kVmToolsStateChangedSignal[] = "VmToolsStateChanged";

}  // namespace plugin_dispatcher
}  // namespace vm_tools

#endif  // SYSTEM_API_DBUS_VM_PLUGIN_DISPATCHER_DBUS_CONSTANTS_H_
