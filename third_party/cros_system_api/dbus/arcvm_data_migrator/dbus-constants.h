// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_ARCVM_DATA_MIGRATOR_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_ARCVM_DATA_MIGRATOR_DBUS_CONSTANTS_H_

namespace arc::data_migrator {

constexpr char kArcVmDataMigratorInterface[] = "org.chromium.ArcVmDataMigrator";
constexpr char kArcVmDataMigratorServicePath[] =
    "/org/chromium/ArcVmDataMigrator";
constexpr char kArcVmDataMigratorServiceName[] =
    "org.chromium.ArcVmDataMigrator";

// Method names.
constexpr char kGetAndroidDataInfoMethod[] = "GetAndroidDataInfo";
constexpr char kHasDataToMigrateMethod[] = "HasDataToMigrate";
constexpr char kStartMigrationMethod[] = "StartMigration";

// Signal names.
constexpr char kMigrationProgressSignal[] = "DataMigrationProgress";

}  // namespace arc::data_migrator

#endif  // SYSTEM_API_DBUS_ARCVM_DATA_MIGRATOR_DBUS_CONSTANTS_H_
