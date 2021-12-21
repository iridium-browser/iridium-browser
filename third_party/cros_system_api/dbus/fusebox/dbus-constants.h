// Copyright 2021 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_FUSEBOX_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_FUSEBOX_DBUS_CONSTANTS_H_

namespace fusebox {

// Interface FuseBoxService (chrome)
const char kFuseBoxServiceInterface[] = "org.chromium.FuseBoxService";
const char kFuseBoxServiceName[] = "org.chromium.FuseBoxService";
const char kFuseBoxServicePath[] = "/org/chromium/FuseBoxService";
const char kFuseBoxOperationMethod[] = "FuseBoxOperation";

// Interface FuseBoxClient (chromeos)
const char kFuseBoxClientInterface[] = "org.chromium.FuseBoxClient";
const char kFuseBoxClientName[] = "org.chromium.FuseBoxClient";
const char kFuseBoxClientPath[] = "/org/chromium/FuseBoxClient";

}  // namespace fusebox

#endif  // SYSTEM_API_DBUS_FUSEBOX_DBUS_CONSTANTS_H_
