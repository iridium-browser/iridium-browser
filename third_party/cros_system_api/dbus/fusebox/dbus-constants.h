// Copyright 2021 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_FUSEBOX_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_FUSEBOX_DBUS_CONSTANTS_H_

namespace fusebox {

// FuseBoxService interface/name/path (chrome)
const char kFuseBoxServiceInterface[] = "org.chromium.FuseBoxService";
const char kFuseBoxServiceName[] = "org.chromium.FuseBoxService";
const char kFuseBoxServicePath[] = "/org/chromium/FuseBoxService";

// FuseBoxService debugging methods.
const char kGetDebugInfoMethod[] = "GetDebugInfo";
const char kGetPrivateDebugInfoMethod[] = "GetPrivateDebugInfo";
const char kGetPublicDebugInfoMethod[] = "GetPublicDebugInfo";
const char kPingMethod[] = "Ping";

// FuseBoxService entry methods.
const char kStatMethod[] = "Stat";

// FuseBoxService directory entry methods.
const char kReadDirMethod[] = "ReadDir";
const char kReadDir2Method[] = "ReadDir2";
const char kMkDirMethod[] = "MkDir";
const char kRmDirMethod[] = "RmDir";

// FuseBoxService file entry methods.
const char kOpenMethod[] = "Open";
const char kOpen2Method[] = "Open2";
const char kOpenFDMethod[] = "OpenFD";
const char kReadMethod[] = "Read";
const char kWriteMethod[] = "Write";
const char kTruncateMethod[] = "Truncate";
const char kFlushMethod[] = "Flush";
const char kCloseMethod[] = "Close";
const char kClose2Method[] = "Close2";
const char kCloseFDMethod[] = "CloseFD";
const char kCreateMethod[] = "Create";
const char kUnlinkMethod[] = "Unlink";

// FuseBoxService other methods.
const char kListStoragesMethod[] = "ListStorages";

// FuseBoxService signals.
const char kStorageAttachedSignal[] = "StorageAttached";
const char kStorageDetachedSignal[] = "StorageDetached";

// FuseBoxReverseService interface/name/path (chromeos /usr/bin/fusebox daemon)
const char kFuseBoxReverseServiceInterface[] =
    "org.chromium.FuseBoxReverseService";
const char kFuseBoxReverseServiceName[] = "org.chromium.FuseBoxReverseService";
const char kFuseBoxReverseServicePath[] = "/org/chromium/FuseBoxReverseService";

// FuseBoxReverseService methods.
const char kAttachStorageMethod[] = "AttachStorage";
const char kDetachStorageMethod[] = "DetachStorage";
const char kReplyToReadDirMethod[] = "ReplyToReadDir";

// Monikers: shared names between Fusebox client and server. Monikers are names
// automatically attached to the client daemon FUSE root node. A moniker's name
// has a subdir and FileSystemURL. They are attached by AttachStorage(name).
//
// See chrome/browser/ash/file_manager/fusebox_moniker.h for more about Fusebox
// monikers. TODO(crbug.com/1327541): consider changing dummy:// => fusebox://,
// and removing the trailing slash on filename prefix.
const char kMonikerAttachStorageArg[] = "moniker dummy://moniker";
const char kMonikerFileSystemURL[] = "dummy://moniker";
const char kMonikerSubdir[] = "moniker";
const char kMonikerFilenamePrefixWithTrailingSlash[] =
    "/media/fuse/fusebox/moniker/";

}  // namespace fusebox

#endif  // SYSTEM_API_DBUS_FUSEBOX_DBUS_CONSTANTS_H_
