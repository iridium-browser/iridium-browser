// Copyright 2015 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_CROS_DISKS_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_CROS_DISKS_DBUS_CONSTANTS_H_

#include <ostream>

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

// Device media type.
// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
// See enum CrosDisksDeviceMediaType in tools/metrics/histograms/enums.xml.
enum class DeviceType {
  kUnknown = 0,
  kUSB = 1,          // USB stick.
  kSD = 2,           // SD card.
  kOpticalDisc = 3,  // Optical disc, excluding DVD.
  kMobile = 4,       // Storage on a mobile device (e.g. Android).
  kDVD = 5,          // DVD.

  kMaxValue = 5,
};

// Format error reported by cros-disks.
// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
// See enum CrosDisksClientFormatError in tools/metrics/histograms/enums.xml.
enum class FormatError {
  kSuccess = 0,
  kUnknownError = 1,
  kInternalError = 2,
  kInvalidDevicePath = 3,
  kDeviceBeingFormatted = 4,
  kUnsupportedFilesystem = 5,
  kFormatProgramNotFound = 6,
  kFormatProgramFailed = 7,
  kDeviceNotAllowed = 8,
  kInvalidOptions = 9,
  kLongName = 10,
  kInvalidCharacter = 11,

  kMaxValue = 11,
};

// Mount or unmount error code.
//
// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class MountError {
  // Success.
  kSuccess = 0,

  // Generic error code.
  kUnknownError = 1,

  // Internal error.
  kInternalError = 2,

  // Invalid argument.
  kInvalidArgument = 3,

  // Invalid path.
  kInvalidPath = 4,

  // Not used.
  kPathAlreadyMounted = 5,

  // Tried to unmount a path that is not currently mounted.
  kPathNotMounted = 6,

  // Cannot create directory.
  kDirectoryCreationFailed = 7,

  // Invalid mount options.
  kInvalidMountOptions = 8,

  // Not used.
  kInvalidUnmountOptions = 9,

  // Insufficient permissions.
  kInsufficientPermissions = 10,

  // The FUSE mounter cannot be found.
  kMountProgramNotFound = 11,

  // The FUSE mounter finished with an error.
  kMountProgramFailed = 12,

  // The provided path to mount is invalid.
  kInvalidDevicePath = 13,

  // Cannot determine file system of the device.
  kUnknownFilesystem = 14,

  // The file system of the device is recognized but not supported.
  kUnsupportedFilesystem = 15,

  // Not used.
  kInvalidArchive = 16,

  // Either the FUSE mounter needs a password, or the provided password is
  // incorrect.
  kNeedPassword = 17,

  // The FUSE mounter is currently launching, and it hasn't daemonized yet.
  kInProgress = 18,

  // The FUSE mounter was cancelled (killed) while it was launching.
  kCancelled = 19,

  // The device is busy.
  kBusy = 20,

  // Modify when adding enum values.
  kMaxValue = 20,
};

// MountSourceType enum values are solely used by Chrome/CrosDisks in
// the MountCompleted signal, and currently not reported through UMA.
enum MountSourceType {
  MOUNT_SOURCE_INVALID = 0,
  MOUNT_SOURCE_REMOVABLE_DEVICE = 1,
  MOUNT_SOURCE_ARCHIVE = 2,
  MOUNT_SOURCE_NETWORK_STORAGE = 3,
};

// Partition error reported by cros-disks.
enum class PartitionError {
  kSuccess = 0,
  kUnknownError = 1,
  kInternalError = 2,
  kInvalidDevicePath = 3,
  kDeviceBeingPartitioned = 4,
  kProgramNotFound = 5,
  kProgramFailed = 6,
  kDeviceNotAllowed = 7,
};

// Rename error reported by cros-disks.
enum class RenameError {
  kSuccess = 0,
  kUnknownError = 1,
  kInternalError = 2,
  kInvalidDevicePath = 3,
  kDeviceBeingRenamed = 4,
  kUnsupportedFilesystem = 5,
  kRenameProgramNotFound = 6,
  kRenameProgramFailed = 7,
  kDeviceNotAllowed = 8,
  kLongName = 9,
  kInvalidCharacter = 10,
};

// Output operators for logging and debugging.

template <typename C>
std::basic_ostream<C>& operator<<(std::basic_ostream<C>& out,
                                  const DeviceType type) {
  switch (type) {
#define PRINT(s)         \
  case DeviceType::k##s: \
    return out << #s;
    PRINT(Unknown)
    PRINT(USB)
    PRINT(SD)
    PRINT(OpticalDisc)
    PRINT(Mobile)
    PRINT(DVD)
#undef PRINT
  }

  return out << "DeviceType(" << std::underlying_type_t<DeviceType>(type)
             << ")";
}

template <typename C>
std::basic_ostream<C>& operator<<(std::basic_ostream<C>& out,
                                  const MountError error) {
  switch (error) {
#define PRINT(s)         \
  case MountError::k##s: \
    return out << #s;
    PRINT(Success)
    PRINT(UnknownError)
    PRINT(InternalError)
    PRINT(InvalidArgument)
    PRINT(InvalidPath)
    PRINT(PathAlreadyMounted)
    PRINT(PathNotMounted)
    PRINT(DirectoryCreationFailed)
    PRINT(InvalidMountOptions)
    PRINT(InvalidUnmountOptions)
    PRINT(InsufficientPermissions)
    PRINT(MountProgramNotFound)
    PRINT(MountProgramFailed)
    PRINT(InvalidDevicePath)
    PRINT(UnknownFilesystem)
    PRINT(UnsupportedFilesystem)
    PRINT(InvalidArchive)
    PRINT(NeedPassword)
    PRINT(InProgress)
    PRINT(Cancelled)
    PRINT(Busy)
#undef PRINT
  }

  return out << "MountError(" << std::underlying_type_t<MountError>(error)
             << ")";
}

template <typename C>
std::basic_ostream<C>& operator<<(std::basic_ostream<C>& out,
                                  const RenameError error) {
  switch (error) {
#define PRINT(s)          \
  case RenameError::k##s: \
    return out << #s;
    PRINT(Success)
    PRINT(UnknownError)
    PRINT(InternalError)
    PRINT(InvalidDevicePath)
    PRINT(DeviceBeingRenamed)
    PRINT(UnsupportedFilesystem)
    PRINT(RenameProgramNotFound)
    PRINT(RenameProgramFailed)
    PRINT(DeviceNotAllowed)
    PRINT(LongName)
    PRINT(InvalidCharacter)
#undef PRINT
  }

  return out << "RenameError(" << std::underlying_type_t<RenameError>(error)
             << ")";
}

template <typename C>
std::basic_ostream<C>& operator<<(std::basic_ostream<C>& out,
                                  const FormatError error) {
  switch (error) {
#define PRINT(s)          \
  case FormatError::k##s: \
    return out << #s;
    PRINT(Success)
    PRINT(UnknownError)
    PRINT(InternalError)
    PRINT(InvalidDevicePath)
    PRINT(DeviceBeingFormatted)
    PRINT(UnsupportedFilesystem)
    PRINT(FormatProgramNotFound)
    PRINT(FormatProgramFailed)
    PRINT(DeviceNotAllowed)
    PRINT(InvalidOptions)
    PRINT(LongName)
    PRINT(InvalidCharacter)
#undef PRINT
  }

  return out << "FormatError(" << std::underlying_type_t<FormatError>(error)
             << ")";
}

template <typename C>
std::basic_ostream<C>& operator<<(std::basic_ostream<C>& out,
                                  const PartitionError error) {
  switch (error) {
#define PRINT(s)             \
  case PartitionError::k##s: \
    return out << #s;
    PRINT(Success)
    PRINT(UnknownError)
    PRINT(InternalError)
    PRINT(InvalidDevicePath)
    PRINT(DeviceBeingPartitioned)
    PRINT(ProgramNotFound)
    PRINT(ProgramFailed)
    PRINT(DeviceNotAllowed)
#undef PRINT
  }

  return out << "PartitionError("
             << std::underlying_type_t<PartitionError>(error) << ")";
}

}  // namespace cros_disks

#endif  // SYSTEM_API_DBUS_CROS_DISKS_DBUS_CONSTANTS_H_
