// Copyright 2020 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Constants for the D-Bus API exposed by the snapshot_manager daemon.
// Normally the consumer of this API is the browser.

#ifndef SYSTEM_API_DBUS_ARC_DATA_SNAPSHOTD_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_ARC_DATA_SNAPSHOTD_DBUS_CONSTANTS_H_

namespace arc {
namespace data_snapshotd {

constexpr char kArcDataSnapshotdServiceInterface[] =
    "org.chromium.ArcDataSnapshotd";
constexpr char kArcDataSnapshotdServicePath[] =
    "/org/chromium/ArcDataSnapshotd";
constexpr char kArcDataSnapshotdServiceName[] = "org.chromium.ArcDataSnapshotd";

// Methods:
constexpr char kGenerateKeyPairMethod[] = "GenerateKeyPair";
constexpr char kClearSnapshotMethod[] = "ClearSnapshot";
constexpr char kTakeSnapshotMethod[] = "TakeSnapshot";
constexpr char kLoadSnapshotMethod[] = "LoadSnapshot";
constexpr char kUpdateMethod[] = "Update";

// Signals:
constexpr char kUiCancelled[] = "UiCancelled";

}  // namespace data_snapshotd
}  // namespace arc

#endif  // SYSTEM_API_DBUS_ARC_DATA_SNAPSHOTD_DBUS_CONSTANTS_H_
