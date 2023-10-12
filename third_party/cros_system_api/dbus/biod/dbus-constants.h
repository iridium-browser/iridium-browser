// Copyright 2017 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_BIOD_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_BIOD_DBUS_CONSTANTS_H_

namespace biod {
const char kBiodServicePath[] = "/org/chromium/BiometricsDaemon";
const char kBiodServiceName[] = "org.chromium.BiometricsDaemon";

// Interfaces for objects exported by biod
const char kBiometricsManagerInterface[] =
    "org.chromium.BiometricsDaemon.BiometricsManager";
const char kAuthSessionInterface[] =
    "org.chromium.BiometricsDaemon.AuthSession";
const char kEnrollSessionInterface[] =
    "org.chromium.BiometricsDaemon.EnrollSession";
const char kRecordInterface[] = "org.chromium.BiometricsDaemon.Record";

const char kAuthStackManagerInterface[] =
    "org.chromium.BiometricsDaemon.AuthStackManager";

// List of all BiometricsManagers
const char kCrosFpBiometricsManagerName[] = "CrosFpBiometricsManager";

// List of all AuthStackManagers
const char kCrosFpAuthStackManagerName[] = "CrosFpAuthStackManager";

// Methods
const char kBiometricsManagerStartEnrollSessionMethod[] = "StartEnrollSession";
const char kBiometricsManagerGetRecordsForUserMethod[] = "GetRecordsForUser";
const char kBiometricsManagerDestroyAllRecordsMethod[] = "DestroyAllRecords";
const char kBiometricsManagerStartAuthSessionMethod[] = "StartAuthSession";
const char kAuthSessionEndMethod[] = "End";
const char kEnrollSessionCancelMethod[] = "Cancel";
const char kRecordRemoveMethod[] = "Remove";
const char kRecordSetLabelMethod[] = "SetLabel";

const char kAuthStackManagerStartEnrollSessionMethod[] = "StartEnrollSession";
const char kAuthStackManagerStartAuthSessionMethod[] = "StartAuthSession";
const char kAuthStackManagerCreateCredentialMethod[] = "CreateCredential";
const char kAuthStackManagerAuthenticateCredentialMethod[] =
    "AuthenticateCredential";
const char kAuthStackManagerDeleteCredentialMethod[] = "DeleteCredential";

// Signals
const char kBiometricsManagerEnrollScanDoneSignal[] = "EnrollScanDone";
const char kBiometricsManagerAuthScanDoneSignal[] = "AuthScanDone";
const char kBiometricsManagerSessionFailedSignal[] = "SessionFailed";
const char kBiometricsManagerStatusChangedSignal[] = "StatusChanged";

// Properties
const char kBiometricsManagerBiometricTypeProperty[] = "Type";
const char kRecordLabelProperty[] = "Label";

// Values
enum BiometricType {
  BIOMETRIC_TYPE_UNKNOWN = 0,
  BIOMETRIC_TYPE_FINGERPRINT = 1,
  BIOMETRIC_TYPE_MAX,
};

// Enroll and Auth Session Errors
const char kDomain[] = "biod";
const char kInternalError[] = "Internal error";
const char kInvalidArguments[] = "Invalid Arguments";
const char kEnrollSessionExists[] = "Another EnrollSession already exists";
const char kAuthSessionExists[] = "Another AuthSession already exists";
const char kTemplatesFull[] = "No space for an additional template";
const char kEnrollImageNotRequested[] = "Enroll image was not requested";
const char kFpHwUnavailable[] = "Fingerprint hardware is unavailable";
const char kMatchNotRequested[] = "Match was not requested";
}  // namespace biod

#endif  // SYSTEM_API_DBUS_BIOD_DBUS_CONSTANTS_H_
