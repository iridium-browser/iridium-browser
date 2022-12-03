// Copyright 2019 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_CHUNNELD_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_CHUNNELD_DBUS_CONSTANTS_H_

namespace vm_tools {
namespace chunneld {

const char kChunneldInterface[] = "org.chromium.Chunneld";
const char kChunneldServicePath[] = "/org/chromium/Chunneld";
const char kChunneldServiceName[] = "org.chromium.Chunneld";

const char kUpdateListeningPortsMethod[] = "UpdateListeningPorts";

}  // namespace chunneld
}  // namespace vm_tools

#endif  // SYSTEM_API_DBUS_CHUNNELD_DBUS_CONSTANTS_H_
