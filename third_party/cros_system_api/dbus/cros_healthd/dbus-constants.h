// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Constants for the D-Bus API exposed by the cros_healthd daemon. Normally the
// consumer of this API is the browser.

#ifndef SYSTEM_API_DBUS_CROS_HEALTHD_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_CROS_HEALTHD_DBUS_CONSTANTS_H_

namespace diagnostics {

constexpr char kCrosHealthdServiceInterface[] =
    "org.chromium.CrosHealthdInterface";
constexpr char kCrosHealthdServicePath[] = "/org/chromium/CrosHealthd";
constexpr char kCrosHealthdServiceName[] = "org.chromium.CrosHealthd";

constexpr char kCrosHealthdBootstrapMojoConnectionMethod[] =
    "BootstrapCrosHealthdMojoConnection";

// Token used for the Mojo connection pipe.
constexpr char kCrosHealthdMojoConnectionChannelToken[] = "cros_healthd";

}  // namespace diagnostics

#endif  // SYSTEM_API_DBUS_CROS_HEALTHD_DBUS_CONSTANTS_H_
