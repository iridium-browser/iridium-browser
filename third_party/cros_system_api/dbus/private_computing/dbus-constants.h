// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_PRIVATE_COMPUTING_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_PRIVATE_COMPUTING_DBUS_CONSTANTS_H_

namespace private_computing {

// Private Computing Device Active Daemon:
constexpr char kPrivateComputingInterface[] = "org.chromium.PrivateComputing";
constexpr char kPrivateComputingServicePath[] =
    "/org/chromium/PrivateComputing";
constexpr char kPrivateComputingServiceName[] = "org.chromium.PrivateComputing";

// Private Computing Device Active methods:
constexpr char kSaveLastPingDatesStatus[] = "SaveLastPingDatesStatus";
constexpr char kGetLastPingDatesStatus[] = "GetLastPingDatesStatus";

}  // namespace private_computing

#endif  // SYSTEM_API_DBUS_PRIVATE_COMPUTING_DBUS_CONSTANTS_H_
