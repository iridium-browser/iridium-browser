// Copyright 2021 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_FUSEBOX_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_FUSEBOX_DBUS_CONSTANTS_H_

namespace fusebox {

// FuseBoxService interface/name/path (chrome)
const char kFuseBoxServiceInterface[] = "org.chromium.FuseBoxService";
const char kFuseBoxServiceName[] = "org.chromium.FuseBoxService";
const char kFuseBoxServicePath[] = "/org/chromium/FuseBoxService";

// FuseBoxService methods.
const char kFuseBoxOperationMethod[] = "FuseBoxOperation";
const char kOpenMethod[] = "Open";
const char kReadDirMethod[] = "ReadDir";
const char kReadMethod[] = "Read";
const char kStatMethod[] = "Stat";

// FuseBoxClient interface/name/path (chromeos)
const char kFuseBoxClientInterface[] = "org.chromium.FuseBoxClient";
const char kFuseBoxClientName[] = "org.chromium.FuseBoxClient";
const char kFuseBoxClientPath[] = "/org/chromium/FuseBoxClient";

// FuseBoxClient methods.
const char kAttachStorageMethod[] = "AttachStorage";
const char kDetachStorageMethod[] = "DetachStorage";
const char kReadDirResponseMethod[] = "ReadDirResponse";

// Deprecated: use the group above's variables instead. They have the same
// values, but the group above have consistent kFooBarMethod variable names.
// The "FuseBox" (note that it's "FuseBox" not "FuseBoxClient") parts of the
// variable names are unnecessary. We are already in "namespace fusebox".
//
// TODO(nigeltao): delete this group when nothing refers to them.
const char kFuseBoxAttachStorageMethod[] = "AttachStorage";
const char kFuseBoxDetachStorageMethod[] = "DetachStorage";
const char kFuseBoxReadDirResponseMethod[] = "ReadDirResponse";

}  // namespace fusebox

#endif  // SYSTEM_API_DBUS_FUSEBOX_DBUS_CONSTANTS_H_
