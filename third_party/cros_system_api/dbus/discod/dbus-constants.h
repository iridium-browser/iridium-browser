// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_DISCOD_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_DISCOD_DBUS_CONSTANTS_H_

namespace discod {

const char kDiscodInterface[] = "org.chromium.Discod";
const char kDiscodServicePath[] = "/org/chromium/Discod";
const char kDiscodServiceName[] = "org.chromium.Discod";
const char kDiscodServiceError[] = "org.chromium.Discod.Error";

// Methods.
const char kEnableWriteBoost[] = "EnableWriteBoost";

}  // namespace discod

#endif  // SYSTEM_API_DBUS_DISCOD_DBUS_CONSTANTS_H_
