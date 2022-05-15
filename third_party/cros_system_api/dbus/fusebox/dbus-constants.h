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
const char kCloseMethod[] = "Close";
const char kFlushMethod[] = "Flush";
const char kOpenMethod[] = "Open";
const char kReadDirMethod[] = "ReadDir";
const char kReadMethod[] = "Read";
const char kStatMethod[] = "Stat";
const char kWriteMethod[] = "Write";

// FuseBoxReverseService interface/name/path (chromeos /usr/bin/fusebox daemon)
const char kFuseBoxReverseServiceInterface[] =
    "org.chromium.FuseBoxReverseService";
const char kFuseBoxReverseServiceName[] = "org.chromium.FuseBoxReverseService";
const char kFuseBoxReverseServicePath[] = "/org/chromium/FuseBoxReverseService";

// FuseBoxReverseService methods.
const char kAttachStorageMethod[] = "AttachStorage";
const char kDetachStorageMethod[] = "DetachStorage";
const char kReplyToReadDirMethod[] = "ReplyToReadDir";

}  // namespace fusebox

#endif  // SYSTEM_API_DBUS_FUSEBOX_DBUS_CONSTANTS_H_
