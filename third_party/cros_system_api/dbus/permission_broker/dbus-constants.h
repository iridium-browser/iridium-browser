// Copyright 2015 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_PERMISSION_BROKER_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_PERMISSION_BROKER_DBUS_CONSTANTS_H_

namespace permission_broker {
const char kPermissionBrokerInterface[] = "org.chromium.PermissionBroker";
const char kPermissionBrokerServicePath[] = "/org/chromium/PermissionBroker";
const char kPermissionBrokerServiceName[] = "org.chromium.PermissionBroker";

// Methods
const char kCheckPathAccess[] = "CheckPathAccess";
const char kOpenPath[] = "OpenPath";
const char kOpenPathAndRegisterClient[] = "OpenPathAndRegisterClient";
const char kClaimDevicePath[] = "ClaimDevicePath";
const char kDetachInterface[] = "DetachInterface";
const char kReattachInterface[] = "ReattachInterface";
const char kRequestAdbPortForward[] = "RequestAdbPortForward";
const char kRequestLoopbackTcpPortLockdown[] = "RequestLoopbackTcpPortLockdown";
const char kRequestTcpPortAccess[] = "RequestTcpPortAccess";
const char kRequestTcpPortForward[] = "RequestTcpPortForward";
const char kRequestUdpPortAccess[] = "RequestUdpPortAccess";
const char kRequestUdpPortForward[] = "RequestUdpPortForward";
const char kReleaseAdbPortForward[] = "ReleaseAdbPortForward";
const char kReleaseLoopbackTcpPort[] = "ReleaseLoopbackTcpPort";
const char kReleaseTcpPort[] = "ReleaseTcpPort";
const char kReleaseTcpPortForward[] = "ReleaseTcpPortForward";
const char kReleaseUdpPort[] = "ReleaseUdpPort";
const char kReleaseUdpPortForward[] = "ReleaseUdpPortForward";
const char kPowerCycleUsbPorts[] = "PowerCycleUsbPorts";
}  // namespace permission_broker

#endif  // SYSTEM_API_DBUS_PERMISSION_BROKER_DBUS_CONSTANTS_H_
