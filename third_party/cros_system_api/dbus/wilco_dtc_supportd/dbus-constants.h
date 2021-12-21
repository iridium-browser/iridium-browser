// Copyright 2018 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Constants for the D-Bus API exposed by the wilco_dtc_supportd daemon.
// Normally the consumer of this API is the browser.

#ifndef SYSTEM_API_DBUS_WILCO_DTC_SUPPORTD_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_WILCO_DTC_SUPPORTD_DBUS_CONSTANTS_H_

namespace diagnostics {

constexpr char kWilcoDtcSupportdServiceInterface[] =
    "org.chromium.WilcoDtcSupportdInterface";
constexpr char kWilcoDtcSupportdServicePath[] =
    "/org/chromium/WilcoDtcSupportd";
constexpr char kWilcoDtcSupportdServiceName[] = "org.chromium.WilcoDtcSupportd";

constexpr char kWilcoDtcSupportdBootstrapMojoConnectionMethod[] =
    "BootstrapMojoConnection";

// Token used for the Mojo connection pipe.
constexpr char kWilcoDtcSupportdMojoConnectionChannelToken[] =
    "wilco_dtc_supportd";

}  // namespace diagnostics

#endif  // SYSTEM_API_DBUS_WILCO_DTC_SUPPORTD_DBUS_CONSTANTS_H_
