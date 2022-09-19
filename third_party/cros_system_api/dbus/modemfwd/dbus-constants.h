// Copyright 2022 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_MODEMFWD_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_MODEMFWD_DBUS_CONSTANTS_H_

namespace modemfwd {

const char kModemfwdInterface[] = "org.chromium.Modemfwd";
const char kModemfwdServicePath[] = "/org/chromium/Modemfwd";
const char kModemfwdServiceName[] = "org.chromium.Modemfwd";

// Methods.
const char kSetDebugMode[] = "SetDebugMode";

// error result codes.
const char kErrorResultFailure[] = "org.chromium.Modemfwd.Error.Failure";
const char kErrorResultInitFailure[] =
    "org.chromium.Modemfwd.Error.InitFailure";
const char kErrorResultInitManifestFailure[] =
    "org.chromium.Modemfwd.Error.InitManifestFailure";
const char kErrorResultInitJournalFailure[] =
    "org.chromium.Modemfwd.Error.InitJournalFailure";
const char kErrorResultFailedToPrepareFirmwareFile[] =
    "org.chromium.Modemfwd.Error.FailedToPrepareFirmwareFile";
const char kErrorResultFailureReturnedByHelper[] =
    "org.chromium.Modemfwd.Error.FailureReturnedByHelper";
const char kErrorResultFlashFailure[] =
    "org.chromium.Modemfwd.Error.FlashFailure";

}  // namespace modemfwd

#endif  // SYSTEM_API_DBUS_MODEMFWD_DBUS_CONSTANTS_H_
