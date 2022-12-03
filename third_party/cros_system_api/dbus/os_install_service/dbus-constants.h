// Copyright 2021 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_OS_INSTALL_SERVICE_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_OS_INSTALL_SERVICE_DBUS_CONSTANTS_H_

namespace os_install_service {

const char kOsInstallServiceInterface[] = "org.chromium.OsInstallService";
const char kOsInstallServiceServicePath[] = "/org/chromium/OsInstallService";
const char kOsInstallServiceServiceName[] = "org.chromium.OsInstallService";

const char kMethodStartOsInstall[] = "StartOsInstall";

const char kSignalOsInstallStatusChanged[] = "OsInstallStatusChanged";

const char kStatusInProgress[] = "InProgress";
const char kStatusFailed[] = "Failed";
const char kStatusSucceeded[] = "Succeeded";
const char kStatusNoDestinationDeviceFound[] = "NoDestinationDeviceFound";

}  // namespace os_install_service

#endif  // SYSTEM_API_DBUS_OS_INSTALL_SERVICE_DBUS_CONSTANTS_H_
