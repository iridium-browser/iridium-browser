// Copyright 2020 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Constants for the D-Bus API exposed by the cfm mojo broker.
// The browser is 'normally' the consumer of this API.

#ifndef SYSTEM_API_DBUS_CFM_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_CFM_DBUS_CONSTANTS_H_

namespace cfm {
namespace broker {
constexpr char kServiceInterfaceName[] = "org.chromium.CfmHotlined";
constexpr char kServicePath[] = "/org/chromium/CfmHotlined";
constexpr char kServiceName[] = "org.chromium.CfmHotlined";

// Method names.
constexpr char kBootstrapMojoConnectionMethod[] = "BootstrapMojoConnection";

// Signal names.
constexpr char kMojoServiceRequestedSignal[] = "MojoServiceRequested";

}  // namespace broker
}  // namespace cfm

#endif  // SYSTEM_API_DBUS_CFM_DBUS_CONSTANTS_H_
