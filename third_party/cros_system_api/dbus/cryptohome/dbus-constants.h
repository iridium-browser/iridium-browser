// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_CRYPTOHOME_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_CRYPTOHOME_DBUS_CONSTANTS_H_

namespace user_data_auth {

// Interface exposed by the cryptohome daemon.
const char kUserDataAuthServiceName[] = "org.chromium.UserDataAuth";
const char kUserDataAuthServicePath[] = "/org/chromium/UserDataAuth";

const char kUserDataAuthInterface[] = "org.chromium.UserDataAuthInterface";
const char kArcQuotaInterface[] = "org.chromium.ArcQuota";
const char kCryptohomePkcs11Interface[] =
    "org.chromium.CryptohomePkcs11Interface";
const char kInstallAttributesInterface[] =
    "org.chromium.InstallAttributesInterface";
const char kCryptohomeMiscInterface[] = "org.chromium.CryptohomeMiscInterface";

// 5 minutes timeout for all cryptohome calls.
// This is a bit on the long side, but we want to be cautious.
constexpr int kUserDataAuthServiceTimeoutInMs = 5 * 60 * 1000;

// Methods of the |kUserDataAuthInterface| interface:
const char kIsMounted[] = "IsMounted";
const char kUnmount[] = "Unmount";
const char kMount[] = "Mount";
const char kRemove[] = "Remove";
const char kRename[] = "Rename";
const char kListKeys[] = "ListKeys";
const char kGetKeyData[] = "GetKeyData";
const char kCheckKey[] = "CheckKey";
const char kAddKey[] = "AddKey";
const char kRemoveKey[] = "RemoveKey";
const char kMassRemoveKeys[] = "MassRemoveKeys";
const char kMigrateKey[] = "MigrateKey";
const char kStartFingerprintAuthSession[] = "StartFingerprintAuthSession";
const char kEndFingerprintAuthSession[] = "EndFingerprintAuthSession";
const char kGetWebAuthnSecret[] = "GetWebAuthnSecret";
const char kStartMigrateToDircrypto[] = "StartMigrateToDircrypto";
const char kNeedsDircryptoMigration[] = "NeedsDircryptoMigration";
const char kGetSupportedKeyPolicies[] = "GetSupportedKeyPolicies";
const char kGetAccountDiskUsage[] = "GetAccountDiskUsage";
const char kStartAuthSession[] = "StartAuthSession";
const char kAddCredentials[] = "AddCredentials";
const char kAuthenticateAuthSession[] = "AuthenticateAuthSession";
const char kInvalidateAuthSession[] = "InvalidateAuthSession";

// Methods of the |kArcQuotaInterface| interface:
const char kGetArcDiskFeatures[] = "GetArcDiskFeatures";
const char kGetCurrentSpaceForArcUid[] = "GetCurrentSpaceForArcUid";
const char kGetCurrentSpaceForArcGid[] = "GetCurrentSpaceForArcGid";
const char kGetCurrentSpaceForArcProjectId[] = "GetCurrentSpaceForArcProjectId";
const char kSetProjectId[] = "SetProjectId";

// Methods of the |kCryptohomePkcs11Interface| interface:
const char kPkcs11IsTpmTokenReady[] = "Pkcs11IsTpmTokenReady";
const char kPkcs11GetTpmTokenInfo[] = "Pkcs11GetTpmTokenInfo";
const char kPkcs11Terminate[] = "Pkcs11Terminate";
const char kPkcs11RestoreTpmTokens[] = "Pkcs11RestoreTpmTokens";

// Methods of the |kInstallAttributesInterface| interface:
const char kInstallAttributesGet[] = "InstallAttributesGet";
const char kInstallAttributesSet[] = "InstallAttributesSet";
const char kInstallAttributesFinalize[] = "InstallAttributesFinalize";
const char kInstallAttributesGetStatus[] = "InstallAttributesGetStatus";
const char kGetFirmwareManagementParameters[] =
    "GetFirmwareManagementParameters";
const char kRemoveFirmwareManagementParameters[] =
    "RemoveFirmwareManagementParameters";
const char kSetFirmwareManagementParameters[] =
    "SetFirmwareManagementParameters";

// Methods of the |kCryptohomeMiscInterface| interface:
const char kGetSystemSalt[] = "GetSystemSalt";
const char kUpdateCurrentUserActivityTimestamp[] =
    "UpdateCurrentUserActivityTimestamp";
const char kGetSanitizedUsername[] = "GetSanitizedUsername";
const char kGetLoginStatus[] = "GetLoginStatus";
const char kGetStatusString[] = "GetStatusString";
const char kLockToSingleUserMountUntilReboot[] =
    "LockToSingleUserMountUntilReboot";
const char kGetRsuDeviceId[] = "GetRsuDeviceId";
const char kCheckHealth[] = "CheckHealth";

// Signals of the |kUserDataAuthInterface| interface:
const char kDircryptoMigrationProgress[] = "DircryptoMigrationProgress";
const char kLowDiskSpace[] = "LowDiskSpace";

}  // namespace user_data_auth

namespace cryptohome {

// Error code
enum MountError {
  MOUNT_ERROR_NONE = 0,
  MOUNT_ERROR_FATAL = 1,
  MOUNT_ERROR_KEY_FAILURE = 2,
  MOUNT_ERROR_INVALID_ARGS = 3,
  MOUNT_ERROR_MOUNT_POINT_BUSY = 4,
  MOUNT_ERROR_EPHEMERAL_MOUNT_BY_OWNER = 5,
  MOUNT_ERROR_CREATE_CRYPTOHOME_FAILED = 6,
  MOUNT_ERROR_REMOVE_INVALID_USER_FAILED = 7,
  MOUNT_ERROR_TPM_COMM_ERROR = 8,
  MOUNT_ERROR_UNPRIVILEGED_KEY = 9,
  MOUNT_ERROR_SETUP_PROCESS_KEYRING_FAILED = 10,
  MOUNT_ERROR_UNEXPECTED_MOUNT_TYPE = 11,
  MOUNT_ERROR_KEYRING_FAILED = 12,
  MOUNT_ERROR_DIR_CREATION_FAILED = 13,
  MOUNT_ERROR_SET_DIR_CRYPTO_KEY_FAILED = 14,
  MOUNT_ERROR_MOUNT_ECRYPTFS_FAILED = 15,
  MOUNT_ERROR_TPM_DEFEND_LOCK = 16,
  MOUNT_ERROR_SETUP_GROUP_ACCESS_FAILED = 17,
  MOUNT_ERROR_MOUNT_HOMES_AND_DAEMON_STORES_FAILED = 18,
  MOUNT_ERROR_TPM_UPDATE_REQUIRED = 19,
  // DANGER: returning this MOUNT_ERROR_VAULT_UNRECOVERABLE may cause vault
  // destruction. Only use it if the vault destruction is the
  // acceptable/expected behaviour upon returning error.
  MOUNT_ERROR_VAULT_UNRECOVERABLE = 20,
  MOUNT_ERROR_MOUNT_DMCRYPT_FAILED = 21,
  MOUNT_ERROR_USER_DOES_NOT_EXIST = 32,
  MOUNT_ERROR_TPM_NEEDS_REBOOT = 64,
  // Encrypted in old method, need migration before mounting.
  MOUNT_ERROR_OLD_ENCRYPTION = 128,
  // Previous migration attempt was aborted in the middle. Must resume it first.
  MOUNT_ERROR_PREVIOUS_MIGRATION_INCOMPLETE = 256,
  // The operation to remove a key failed.
  MOUNT_ERROR_REMOVE_FAILED = 512,
  MOUNT_ERROR_RECREATED = 1 << 31,
};
// Status code signaled from MigrateToDircrypto().
enum DircryptoMigrationStatus {
  // 0 means a successful completion.
  DIRCRYPTO_MIGRATION_SUCCESS = 0,
  // Negative values mean failing completion.
  // TODO(kinaba,dspaid): Add error codes as needed here.
  DIRCRYPTO_MIGRATION_FAILED = -1,
  // Positive values mean intermediate state report for the running migration.
  // TODO(kinaba,dspaid): Add state codes as needed.
  DIRCRYPTO_MIGRATION_INITIALIZING = 1,
  DIRCRYPTO_MIGRATION_IN_PROGRESS = 2,
};

// Type of paths that are allowed for SetProjectId().
enum SetProjectIdAllowedPathType {
  // /home/user/<obfuscated_username>/Downloads/
  PATH_DOWNLOADS = 0,
  // /home/root/<obfuscated_username>/android-data/
  PATH_ANDROID_DATA = 1,
};

// Interface for key delegate service to be used by the cryptohome daemon.

const char kCryptohomeKeyDelegateInterface[] =
    "org.chromium.CryptohomeKeyDelegateInterface";

// Methods of the |kCryptohomeKeyDelegateInterface| interface:
const char kCryptohomeKeyDelegateChallengeKey[] = "ChallengeKey";

}  // namespace cryptohome

#endif  // SYSTEM_API_DBUS_CRYPTOHOME_DBUS_CONSTANTS_H_
