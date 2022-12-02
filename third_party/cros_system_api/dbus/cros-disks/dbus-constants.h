// Copyright 2015 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_CROS_DISKS_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_CROS_DISKS_DBUS_CONSTANTS_H_

namespace cros_disks {
const char kCrosDisksInterface[] = "org.chromium.CrosDisks";
const char kCrosDisksServicePath[] = "/org/chromium/CrosDisks";
const char kCrosDisksServiceName[] = "org.chromium.CrosDisks";
const char kCrosDisksServiceError[] = "org.chromium.CrosDisks.Error";

// Methods.
const char kEnumerateAutoMountableDevices[] = "EnumerateAutoMountableDevices";
const char kEnumerateDevices[] = "EnumerateDevices";
const char kEnumerateMountEntries[] = "EnumerateMountEntries";
const char kFormat[] = "Format";
const char kSinglePartitionFormat[] = "SinglePartitionFormat";
const char kGetDeviceProperties[] = "GetDeviceProperties";
const char kMount[] = "Mount";
const char kRename[] = "Rename";
const char kUnmount[] = "Unmount";

// Signals.
const char kDeviceAdded[] = "DeviceAdded";
const char kDeviceScanned[] = "DeviceScanned";
const char kDeviceRemoved[] = "DeviceRemoved";
const char kDiskAdded[] = "DiskAdded";
const char kDiskChanged[] = "DiskChanged";
const char kDiskRemoved[] = "DiskRemoved";
const char kFormatCompleted[] = "FormatCompleted";
const char kMountCompleted[] = "MountCompleted";
const char kMountProgress[] = "MountProgress";
const char kRenameCompleted[] = "RenameCompleted";

// Properties.
// TODO(benchan): Drop unnecessary 'Device' / 'Drive' prefix as they were
// carried through old code base.
const char kDeviceFile[] = "DeviceFile";
const char kDeviceIsDrive[] = "DeviceIsDrive";
const char kDeviceIsMediaAvailable[] = "DeviceIsMediaAvailable";
const char kDeviceIsMounted[] = "DeviceIsMounted";
const char kDeviceIsOnBootDevice[] = "DeviceIsOnBootDevice";
const char kDeviceIsOnRemovableDevice[] = "DeviceIsOnRemovableDevice";
const char kDeviceIsReadOnly[] = "DeviceIsReadOnly";
const char kDeviceIsVirtual[] = "DeviceIsVirtual";
const char kDeviceMediaType[] = "DeviceMediaType";
const char kDeviceMountPaths[] = "DeviceMountPaths";
const char kDevicePresentationHide[] = "DevicePresentationHide";
const char kDeviceSize[] = "DeviceSize";
const char kDriveModel[] = "DriveModel";
const char kIsAutoMountable[] = "IsAutoMountable";
const char kIdLabel[] = "IdLabel";
const char kIdUuid[] = "IdUuid";
const char kVendorId[] = "VendorId";
const char kVendorName[] = "VendorName";
const char kProductId[] = "ProductId";
const char kProductName[] = "ProductName";
const char kBusNumber[] = "BusNumber";
const char kDeviceNumber[] = "DeviceNumber";
const char kStorageDevicePath[] = "StorageDevicePath";
const char kFileSystemType[] = "FileSystemType";

// Format options.
const char kFormatLabelOption[] = "Label";

// Enum values.
// DeviceMediaType enum values are reported through UMA.
// All values but DEVICE_MEDIA_NUM_VALUES should not be changed or removed.
// Additional values can be added but DEVICE_MEDIA_NUM_VALUES should always
// be the last value in the enum.
enum DeviceMediaType {
  DEVICE_MEDIA_UNKNOWN = 0,
  DEVICE_MEDIA_USB = 1,
  DEVICE_MEDIA_SD = 2,
  DEVICE_MEDIA_OPTICAL_DISC = 3,
  DEVICE_MEDIA_MOBILE = 4,
  DEVICE_MEDIA_DVD = 5,
  DEVICE_MEDIA_NUM_VALUES,
};

enum FormatErrorType {
  FORMAT_ERROR_NONE = 0,
  FORMAT_ERROR_UNKNOWN = 1,
  FORMAT_ERROR_INTERNAL = 2,
  FORMAT_ERROR_INVALID_DEVICE_PATH = 3,
  FORMAT_ERROR_DEVICE_BEING_FORMATTED = 4,
  FORMAT_ERROR_UNSUPPORTED_FILESYSTEM = 5,
  FORMAT_ERROR_FORMAT_PROGRAM_NOT_FOUND = 6,
  FORMAT_ERROR_FORMAT_PROGRAM_FAILED = 7,
  FORMAT_ERROR_DEVICE_NOT_ALLOWED = 8,
  FORMAT_ERROR_INVALID_OPTIONS = 9,
  FORMAT_ERROR_LONG_NAME = 10,
  FORMAT_ERROR_INVALID_CHARACTER = 11,
};

// TODO(crbug.com/1368408): After both Chrome and cros-disks use these enum
// values, make these error values contiguous so that they can be directly
// reported via UMA. See
// https://source.chromium.org/chromium/chromium/src/+/main:chromeos/ash/components/dbus/cros_disks/cros_disks_client.h?q=MountError.
//
// The *_EX values are used during the transition.
enum MountErrorType {
  MOUNT_ERROR_NONE = 0,
  MOUNT_ERROR_UNKNOWN = 1,
  MOUNT_ERROR_INTERNAL = 2,
  MOUNT_ERROR_INVALID_ARGUMENT = 3,
  MOUNT_ERROR_INVALID_PATH = 4,
  MOUNT_ERROR_PATH_ALREADY_MOUNTED = 5,
  MOUNT_ERROR_PATH_NOT_MOUNTED = 6,
  MOUNT_ERROR_DIRECTORY_CREATION_FAILED = 7,
  MOUNT_ERROR_INVALID_MOUNT_OPTIONS = 8,
  MOUNT_ERROR_INVALID_UNMOUNT_OPTIONS = 9,
  MOUNT_ERROR_INSUFFICIENT_PERMISSIONS = 10,

  // The FUSE mounter cannot be found.
  MOUNT_ERROR_MOUNT_PROGRAM_NOT_FOUND = 11,

  // The FUSE mounter finished with an error.
  MOUNT_ERROR_MOUNT_PROGRAM_FAILED = 12,

  // Either the FUSE mounter needs a password, or the provided password is
  // incorrect.
  MOUNT_ERROR_NEED_PASSWORD = 13,
  MOUNT_ERROR_NEED_PASSWORD_EX = 117,

  // The FUSE mounter is currently launching, and it hasn't daemonized yet.
  MOUNT_ERROR_IN_PROGRESS = 14,
  MOUNT_ERROR_IN_PROGRESS_EX = 118,

  // The FUSE mounter was cancelled (killed) while it was launching.
  MOUNT_ERROR_CANCELLED = 15,
  MOUNT_ERROR_CANCELLED_EX = 119,

  // The device is busy.
  MOUNT_ERROR_BUSY = 20,

  MOUNT_ERROR_INVALID_DEVICE_PATH = 100,
  MOUNT_ERROR_UNKNOWN_FILESYSTEM = 101,
  MOUNT_ERROR_UNSUPPORTED_FILESYSTEM = 102,
  MOUNT_ERROR_INVALID_ARCHIVE = 201,
  MOUNT_ERROR_UNSUPPORTED_ARCHIVE = 202,
};

// MountSourceType enum values are solely used by Chrome/CrosDisks in
// the MountCompleted signal, and currently not reported through UMA.
enum MountSourceType {
  MOUNT_SOURCE_INVALID = 0,
  MOUNT_SOURCE_REMOVABLE_DEVICE = 1,
  MOUNT_SOURCE_ARCHIVE = 2,
  MOUNT_SOURCE_NETWORK_STORAGE = 3,
};

enum PartitionErrorType {
  PARTITION_ERROR_NONE = 0,
  PARTITION_ERROR_UNKNOWN = 1,
  PARTITION_ERROR_INTERNAL = 2,
  PARTITION_ERROR_INVALID_DEVICE_PATH = 3,
  PARTITION_ERROR_DEVICE_BEING_PARTITIONED = 4,
  PARTITION_ERROR_PROGRAM_NOT_FOUND = 5,
  PARTITION_ERROR_PROGRAM_FAILED = 6,
  PARTITION_ERROR_DEVICE_NOT_ALLOWED = 7,
};

enum RenameErrorType {
  RENAME_ERROR_NONE = 0,
  RENAME_ERROR_UNKNOWN = 1,
  RENAME_ERROR_INTERNAL = 2,
  RENAME_ERROR_INVALID_DEVICE_PATH = 3,
  RENAME_ERROR_DEVICE_BEING_RENAMED = 4,
  RENAME_ERROR_UNSUPPORTED_FILESYSTEM = 5,
  RENAME_ERROR_RENAME_PROGRAM_NOT_FOUND = 6,
  RENAME_ERROR_RENAME_PROGRAM_FAILED = 7,
  RENAME_ERROR_DEVICE_NOT_ALLOWED = 8,
  RENAME_ERROR_LONG_NAME = 9,
  RENAME_ERROR_INVALID_CHARACTER = 10,
};
}  // namespace cros_disks

#endif  // SYSTEM_API_DBUS_CROS_DISKS_DBUS_CONSTANTS_H_
