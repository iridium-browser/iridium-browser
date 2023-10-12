// Copyright 2019 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_PATCHPANEL_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_PATCHPANEL_DBUS_CONSTANTS_H_

namespace patchpanel {

const char kPatchPanelInterface[] = "org.chromium.PatchPanel";
const char kPatchPanelServicePath[] = "/org/chromium/PatchPanel";
const char kPatchPanelServiceName[] = "org.chromium.PatchPanel";

// Exported methods.
const char kArcShutdownMethod[] = "ArcShutdown";
const char kArcStartupMethod[] = "ArcStartup";
const char kArcVmShutdownMethod[] = "ArcVmShutdown";
const char kArcVmStartupMethod[] = "ArcVmStartup";
const char kConnectNamespaceMethod[] = "ConnectNamespace";
const char kCreateLocalOnlyNetworkMethod[] = "CreateLocalOnlyNetwork";
const char kCreateTetheredNetworkMethod[] = "CreateTetheredNetwork";
const char kGetDevicesMethod[] = "GetDevices";
const char kGetDownstreamNetworkInfoMethod[] = "GetDownstreamNetworkInfo";
const char kGetTrafficCountersMethod[] = "GetTrafficCounters";
const char kModifyPortRuleMethod[] = "ModifyPortRule";
const char kParallelsVmShutdownMethod[] = "ParallelsVmShutdown";
const char kParallelsVmStartupMethod[] = "ParallelsVmStartup";
const char kNotifyAndroidInteractiveStateMethod[] =
    "NotifyAndroidInteractiveState";
const char kNotifyAndroidWifiMulticastLockChangeMethod[] =
    "NotifyAndroidWifiMulticastLockChange";
const char kSetDnsRedirectionRuleMethod[] = "SetDnsRedirectionRule";
const char kSetVpnIntentMethod[] = "SetVpnIntent";
const char kSetVpnLockdown[] = "SetVpnLockdown";
const char kTerminaVmShutdownMethod[] = "TerminaVmShutdown";
const char kTerminaVmStartupMethod[] = "TerminaVmStartup";

// Signals.
const char kNetworkDeviceChangedSignal[] = "NetworkDeviceChanged";
const char kNetworkConfigurationChangedSignal[] = "NetworkConfigurationChanged";
const char kNeighborReachabilityEventSignal[] = "NeighborReachabilityEvent";

}  // namespace patchpanel

#endif  // SYSTEM_API_DBUS_PATCHPANEL_DBUS_CONSTANTS_H_
