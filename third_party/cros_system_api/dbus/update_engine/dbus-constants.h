// Copyright 2015 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_UPDATE_ENGINE_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_UPDATE_ENGINE_DBUS_CONSTANTS_H_

namespace update_engine {
const char kUpdateEngineInterface[] = "org.chromium.UpdateEngineInterface";
const char kUpdateEngineServicePath[] = "/org/chromium/UpdateEngine";
const char kUpdateEngineServiceName[] = "org.chromium.UpdateEngine";

// Generic UpdateEngine D-Bus error.
const char kUpdateEngineServiceErrorFailed[] =
    "org.chromium.UpdateEngine.Error.Failed";

// Methods.
const char kUpdate[] = "Update";
const char kGetLastAttemptError[] = "GetLastAttemptError";
const char kGetStatusAdvanced[] = "GetStatusAdvanced";
const char kRebootIfNeeded[] = "RebootIfNeeded";
const char kSetChannel[] = "SetChannel";
const char kGetChannel[] = "GetChannel";
const char kSetCohortHint[] = "SetCohortHint";
const char kGetCohortHint[] = "GetCohortHint";
const char kAttemptRollback[] = "AttemptRollback";
const char kCanRollback[] = "CanRollback";
const char kSetUpdateOverCellularPermission[] =
    "SetUpdateOverCellularPermission";
const char kSetUpdateOverCellularTarget[] = "SetUpdateOverCellularTarget";
const char kToggleFeature[] = "ToggleFeature";
const char kIsFeatureEnabled[] = "IsFeatureEnabled";
const char kApplyDeferredUpdate[] = "ApplyDeferredUpdate";
const char kApplyDeferredUpdateAdvanced[] = "ApplyDeferredUpdateAdvanced";

// Signals.
const char kStatusUpdateAdvanced[] = "StatusUpdateAdvanced";

// Operations contained in |StatusUpdate| signals.
const char kUpdateStatusIdle[] = "UPDATE_STATUS_IDLE";
const char kUpdateStatusCheckingForUpdate[] =
    "UPDATE_STATUS_CHECKING_FOR_UPDATE";
const char kUpdateStatusUpdateAvailable[] = "UPDATE_STATUS_UPDATE_AVAILABLE";
const char kUpdateStatusDownloading[] = "UPDATE_STATUS_DOWNLOADING";
const char kUpdateStatusVerifying[] = "UPDATE_STATUS_VERIFYING";
const char kUpdateStatusFinalizing[] = "UPDATE_STATUS_FINALIZING";
const char kUpdateStatusUpdatedNeedReboot[] =
    "UPDATE_STATUS_UPDATED_NEED_REBOOT";
const char kUpdateStatusReportingErrorEvent[] =
    "UPDATE_STATUS_REPORTING_ERROR_EVENT";
const char kUpdateStatusAttemptingRollback[] =
    "UPDATE_STATUS_ATTEMPTING_ROLLBACK";
const char kUpdateStatusDisabled[] = "UPDATE_STATUS_DISABLED";
const char kUpdateStatusNeedPermissionToUpdate[] =
    "UPDATE_STATUS_NEED_PERMISSION_TO_UPDATE";
const char kUpdateStatusCleanupPreviousUpdate[] =
    "UPDATE_STATUS_CLEANUP_PREVIOUS_UPDATE";
const char kUpdateStatusUpdatedButDeferred[] =
    "UPDATE_STATUS_UPDATED_BUT_DEFERRED";

// Feature names.
const char kFeatureRepeatedUpdates[] = "feature-repeated-updates";
const char kFeatureConsumerAutoUpdate[] = "feature-consumer-auto-update";

// Action exit codes.
// Reference common/error_code.h in update_engine repo for direct mappings and
// future updates to this enum class. If new errors need to be added here, it
// must be kept in sync with common/error_code.h in update_engine.
enum class ErrorCode : int {
  kSuccess = 0,
  kError = 1,
  kDownloadTransferError = 9,
  kOmahaUpdateIgnoredPerPolicy = 35,
  kOmahaErrorInHTTPResponse = 37,
  kNoUpdate = 53,
};

}  // namespace update_engine

#endif  // SYSTEM_API_DBUS_UPDATE_ENGINE_DBUS_CONSTANTS_H_
