// Copyright 2020 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_IP_PERIPHERAL_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_IP_PERIPHERAL_DBUS_CONSTANTS_H_

namespace ip_peripheral {

const char kIpPeripheralServiceInterface[] =
    "org.chromium.IpPeripheralService.ChromeInterface";
const char kIpPeripheralServicePath[] = "/org/chromium/IpPeripheralService";
const char kIpPeripheralServiceName[] = "org.chromium.IpPeripheralService";

// Methods.
const char kGetPanMethod[] = "GetPan";
const char kGetTiltMethod[] = "GetTilt";
const char kGetZoomMethod[] = "GetZoom";
const char kSetPanMethod[] = "SetPan";
const char kSetTiltMethod[] = "SetTilt";
const char kSetZoomMethod[] = "SetZoom";

}  // namespace ip_peripheral
#endif  // SYSTEM_API_DBUS_IP_PERIPHERAL_DBUS_CONSTANTS_H_
