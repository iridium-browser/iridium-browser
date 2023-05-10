// Copyright 2019 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_HERMES_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_HERMES_DBUS_CONSTANTS_H_

namespace hermes {

// Hermes D-Bus service identifiers.
const char kHermesServiceName[] = "org.chromium.Hermes";
const char kHermesManagerInterface[] = "org.chromium.Hermes.Manager";
const char kHermesManagerPath[] = "/org/chromium/Hermes/Manager";

const char kHermesEuiccInterface[] = "org.chromium.Hermes.Euicc";
const char kHermesProfileInterface[] = "org.chromium.Hermes.Profile";

namespace manager {

// Manager methods.
const char kSetTestMode[] = "SetTestMode";

// Manager properties.
const char kAvailableEuiccsProperty[] = "AvailableEuiccs";

}  // namespace manager

namespace euicc {

// Euicc methods.
const char kInstallProfileFromActivationCode[] =
    "InstallProfileFromActivationCode";
const char kInstallPendingProfile[] = "InstallPendingProfile";
const char kRefreshInstalledProfiles[] = "RefreshInstalledProfiles";
const char kRequestPendingProfiles[] = "RequestPendingProfiles";
const char kRefreshSmdxProfiles[] = "RefreshSmdxProfiles";
const char kRequestInstalledProfiles[] = "RequestInstalledProfiles";
const char kUninstallProfile[] = "UninstallProfile";
const char kResetMemory[] = "ResetMemory";

// Argument when a ResetMemory call is made
enum ResetOptions {
  kDeleteOperationalProfiles = 1,
  kDeleteFieldLoadedTestProfiles = 2,
};

// Euicc properties.
const char kEidProperty[] = "Eid";
const char kInstalledProfilesProperty[] = "InstalledProfiles";
const char kIsActiveProperty[] = "IsActive";
const char kPendingProfilesProperty[] = "PendingProfiles";
const char kProfilesProperty[] = "Profiles";
const char kPhysicalSlotProperty[] = "PhysicalSlot";

}  // namespace euicc

namespace profile {

// Profile methods.
const char kEnable[] = "Enable";
const char kDisable[] = "Disable";
const char kRename[] = "Rename";

// Profile properties.
const char kActivationCodeProperty[] = "ActivationCode";
const char kIccidProperty[] = "Iccid";
const char kMccMncProperty[] = "MccMnc";
const char kNameProperty[] = "Name";
const char kNicknameProperty[] = "Nickname";
const char kProfileClassProperty[] = "ProfileClass";
const char kServiceProviderProperty[] = "ServiceProvider";
const char kStateProperty[] = "State";

// Values for kProfileClassProperty.
enum ProfileClass {
  kTesting = 0,
  // Profile for provisioning a non-kProvisioning Profile. Should NOT be shown
  // to users normally. From the spec:
  //
  // Provisioning Profiles and their associated Profile Metadata SHALL not be
  // visible to the End User in the LUI. As a result, Provisioning Profiles
  // SHALL not be selectable by the End User nor deletable through any End User
  // action, including eUICC Memory Reset.
  kProvisioning = 1,
  // Profile available for normal servicing of user connectivity needs.
  kOperational = 2,
};

// Values for kStateProperty.
enum State {
  // Notified about from SM-DS but not installed.
  kPending = 0,
  // Installed on eUICC but not active.
  kInactive = 1,
  // Installed and active. Only one Profile may be active on a single eUICC.
  kActive = 2,
};

}  // namespace profile

// Error codes.
const char kErrorAlreadyDisabled[] =
    "org.chromium.Hermes.Error.AlreadyDisabled";
const char kErrorAlreadyEnabled[] = "org.chromium.Hermes.Error.AlreadyEnabled";
const char kErrorBadNotification[] =
    "org.chromium.Hermes.Error.BadNotification";
const char kErrorBadRequest[] = "org.chromium.Hermes.Error.BadRequest";
const char kErrorInternalLpaFailure[] =
    "org.chromium.Hermes.Error.InternalLpaFailure";
const char kErrorInvalidActivationCode[] =
    "org.chromium.Hermes.Error.InvalidActivationCode";
const char kErrorInvalidIccid[] = "org.chromium.Hermes.Error.InvalidIccid";
const char kErrorInvalidParameter[] =
    "org.chromium.Hermes.Error.InvalidParameter";
const char kErrorMalformedResponse[] =
    "org.chromium.Hermes.Error.MalformedResponse";
const char kErrorNeedConfirmationCode[] =
    "org.chromium.Hermes.Error.NeedConfirmationCode";
const char kErrorNoResponse[] = "org.chromium.Hermes.Error.NoResponse";
const char kErrorPendingProfile[] = "org.chromium.Hermes.Error.PendingProfile";
const char kErrorSendApduFailure[] =
    "org.chromium.Hermes.Error.SendApduFailure";
const char kErrorSendHttpsFailure[] =
    "org.chromium.Hermes.Error.SendHttpsFailure";
const char kErrorSendNotificationFailure[] =
    "org.chromium.Hermes.Error.SendNotificationFailure";
const char kErrorTestProfileInProd[] =
    "org.chromium.Hermes.Error.TestProfileInProd";
const char kErrorUnknown[] = "org.chromium.Hermes.Error.Unknown";
const char kErrorUnsupported[] = "org.chromium.Hermes.Error.Unsupported";
const char kErrorWrongState[] = "org.chromium.Hermes.Error.WrongState";

}  // namespace hermes

#endif  // SYSTEM_API_DBUS_HERMES_DBUS_CONSTANTS_H_
