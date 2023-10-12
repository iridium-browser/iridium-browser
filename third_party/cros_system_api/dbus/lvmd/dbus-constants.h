// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_LVMD_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_LVMD_DBUS_CONSTANTS_H_

namespace lvmd {

constexpr char kLvmdInterface[] = "org.chromium.Lvmd";
constexpr char kLvmdServicePath[] = "/org/chromium/Lvmd";
constexpr char kLvmdServiceName[] = "org.chromium.Lvmd";

// TODO(b/236007986): Add methods to use in clients.

constexpr char kErrorInternal[] = "org.chromium.Lvmd.INTERNAL";

}  // namespace lvmd

#endif  // SYSTEM_API_DBUS_LVMD_DBUS_CONSTANTS_H_
