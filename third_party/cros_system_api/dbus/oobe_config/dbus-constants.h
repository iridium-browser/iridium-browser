// Copyright 2018 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_OOBE_CONFIG_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_OOBE_CONFIG_DBUS_CONSTANTS_H_

namespace oobe_config {

// General
const char kOobeConfigRestoreInterface[] = "org.chromium.OobeConfigRestore";
const char kOobeConfigRestoreServicePath[] = "/org/chromium/OobeConfigRestore";
const char kOobeConfigRestoreServiceName[] = "org.chromium.OobeConfigRestore";

// Methods
const char kProcessAndGetOobeAutoConfigMethod[] = "ProcessAndGetOobeAutoConfig";

}  // namespace oobe_config

#endif  // SYSTEM_API_DBUS_OOBE_CONFIG_DBUS_CONSTANTS_H_
