// Copyright 2021 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_RMAD_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_RMAD_DBUS_CONSTANTS_H_

namespace rmad {
const char kRmadInterfaceName[] = "org.chromium.Rmad";
const char kRmadServicePath[] = "/org/chromium/Rmad";
const char kRmadServiceName[] = "org.chromium.Rmad";

// Methods
const char kIsRmaRequiredMethod[] = "IsRmaRequired";
const char kGetCurrentStateMethod[] = "GetCurrentState";
const char kTransitionNextStateMethod[] = "TransitionNextState";
const char kTransitionPreviousStateMethod[] = "TransitionPreviousState";
const char kAbortRmaMethod[] = "AbortRma";
// Legacy method. Keep it to be compatible with Chrome.
// TODO(chenghan): Remove it in the future.
const char kGetLogPathMethod[] = "GetLogPath";
const char kGetLogMethod[] = "GetLog";

// Signals.
const char kErrorSignal[] = "Error";
const char kHardwareVerificationResultSignal[] = "HardwareVerificationResult";
const char kUpdateRoFirmwareStatusSignal[] = "UpdateRoFirmwareStatus";
const char kCalibrationOverallSignal[] = "CalibrationOverall";
const char kCalibrationProgressSignal[] = "CalibrationProgress";
const char kProvisioningProgressSignal[] = "ProvisioningProgress";
const char kFinalizeProgressSignal[] = "FinalizeProgress";
const char kHardwareWriteProtectionStateSignal[] =
    "HardwareWriteProtectionState";
const char kPowerCableStateSignal[] = "PowerCableState";

}  // namespace rmad

#endif  // SYSTEM_API_DBUS_RMAD_DBUS_CONSTANTS_H_
