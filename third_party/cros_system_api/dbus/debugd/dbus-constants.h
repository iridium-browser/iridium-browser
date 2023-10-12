// Copyright 2015 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_DEBUGD_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_DEBUGD_DBUS_CONSTANTS_H_

namespace debugd {
const char kDebugdInterface[] = "org.chromium.debugd";
const char kDebugdServicePath[] = "/org/chromium/debugd";
const char kDebugdServiceName[] = "org.chromium.debugd";

// Methods.
const char kCupsAddManuallyConfiguredPrinter[] =
    "CupsAddManuallyConfiguredPrinter";
const char kCupsAddManuallyConfiguredPrinterV2[] =
    "CupsAddManuallyConfiguredPrinterV2";
const char kCupsAddAutoConfiguredPrinter[] = "CupsAddAutoConfiguredPrinter";
const char kCupsAddAutoConfiguredPrinterV2[] = "CupsAddAutoConfiguredPrinterV2";
const char kCupsRemovePrinter[] = "CupsRemovePrinter";
const char kCupsRetrievePpd[] = "CupsRetrievePpd";
const char kDumpDebugLogs[] = "DumpDebugLogs";
const char kGetInterfaces[] = "GetInterfaces";
const char kGetNetworkStatus[] = "GetNetworkStatus";
const char kGetPerfOutput[] = "GetPerfOutput";
const char kGetPerfOutputFd[] = "GetPerfOutputFd";
const char kGetPerfOutputV2[] = "GetPerfOutputV2";
const char kStopPerf[] = "StopPerf";
const char kGetIpAddrs[] = "GetIpAddrs";
const char kGetRoutes[] = "GetRoutes";
const char kSetDebugMode[] = "SetDebugMode";
const char kSystraceStart[] = "SystraceStart";
const char kSystraceStop[] = "SystraceStop";
const char kSystraceStatus[] = "SystraceStatus";
const char kGetLog[] = "GetLog";
const char kGetAllLogs[] = "GetAllLogs";
const char kGetFeedbackLogsV2[] = "GetFeedbackLogsV2";
const char kGetFeedbackLogsV3[] = "GetFeedbackLogsV3";
const char kKstaledSetRatio[] = "KstaledSetRatio";
const char kTestICMP[] = "TestICMP";
const char kTestICMPWithOptions[] = "TestICMPWithOptions";
const char kLogKernelTaskStates[] = "LogKernelTaskStates";
const char kUploadCrashes[] = "UploadCrashes";
const char kUploadSingleCrash[] = "UploadSingleCrash";
const char kRemoveRootfsVerification[] = "RemoveRootfsVerification";
const char kEnableChromeRemoteDebugging[] = "EnableChromeRemoteDebugging";
const char kEnableBootFromUsb[] = "EnableBootFromUsb";
const char kConfigureSshServer[] = "ConfigureSshServer";
const char kSetUserPassword[] = "SetUserPassword";
const char kEnableChromeDevFeatures[] = "EnableChromeDevFeatures";
const char kQueryDevFeatures[] = "QueryDevFeatures";
const char kSetOomScoreAdj[] = "SetOomScoreAdj";
const char kStartVmPluginDispatcher[] = "StartVmPluginDispatcher";
const char kStopVmPluginDispatcher[] = "StopVmPluginDispatcher";
const char kSetRlzPingSent[] = "SetRlzPingSent";
const char kSetU2fFlags[] = "SetU2fFlags";
const char kGetU2fFlags[] = "GetU2fFlags";
const char kSetSchedulerConfiguration[] = "SetSchedulerConfiguration";
const char kSetSchedulerConfigurationV2[] = "SetSchedulerConfigurationV2";
const char kSwapSetParameter[] = "SwapSetParameter";
const char kSwapZramEnableWriteback[] = "SwapZramEnableWriteback";
const char kSwapZramMarkIdle[] = "SwapZramMarkIdle";
const char kSwapZramSetWritebackLimit[] = "SwapZramSetWritebackLimit";
const char kSwapZramWriteback[] = "InitiateSwapZramWriteback";
const char kSwapSetSwappiness[] = "SwapSetSwappiness";
const char kBackupArcBugReport[] = "BackupArcBugReport";
const char kDeleteArcBugReportBackup[] = "DeleteArcBugReportBackup";
const char kKernelFeatureList[] = "KernelFeatureList";
const char kKernelFeatureEnable[] = "KernelFeatureEnable";
// PacketCaptureStart method isn't defined as it's not needed by any component.
const char kPacketCaptureStop[] = "PacketCaptureStop";
const char kDRMTraceAnnotateLog[] = "DRMTraceAnnotateLog";

// Signals.
const char kPacketCaptureStartSignal[] = "PacketCaptureStart";
const char kPacketCaptureStopSignal[] = "PacketCaptureStop";

// Properties.
const char kCrashSenderTestMode[] = "CrashSenderTestMode";

// ZramWritebackMode contains the allowed modes of operation
// for zram writeback.
enum ZramWritebackMode {
  WRITEBACK_IDLE = 0x001,
  WRITEBACK_HUGE = 0x002,
  WRITEBACK_HUGE_IDLE = 0x004,
};

// Values.
enum DevFeatureFlag {
  DEV_FEATURES_DISABLED = 1 << 0,
  DEV_FEATURE_ROOTFS_VERIFICATION_REMOVED = 1 << 1,
  DEV_FEATURE_BOOT_FROM_USB_ENABLED = 1 << 2,
  DEV_FEATURE_SSH_SERVER_CONFIGURED = 1 << 3,
  DEV_FEATURE_DEV_MODE_ROOT_PASSWORD_SET = 1 << 4,
  DEV_FEATURE_SYSTEM_ROOT_PASSWORD_SET = 1 << 5,
  DEV_FEATURE_CHROME_REMOTE_DEBUGGING_ENABLED = 1 << 6,
};

// CupsAdd* error codes
enum CupsResult {
  CUPS_SUCCESS = 0,
  CUPS_FATAL = 1,
  CUPS_INVALID_PPD = 2,
  CUPS_LPADMIN_FAILURE = 3,
  CUPS_AUTOCONF_FAILURE = 4,
  CUPS_BAD_URI = 5,
  CUPS_IO_ERROR = 6,
  CUPS_MEMORY_ALLOC_ERROR = 7,
  CUPS_PRINTER_UNREACHABLE = 8,
  CUPS_PRINTER_WRONG_RESPONSE = 9,
  CUPS_PRINTER_NOT_AUTOCONF = 10
};

// DRMTraceCategories flags. These values must align with those in
// org.chromium.debug.xml.
enum DRMTraceCategories {
  DRMTraceCategory_CORE = 0x001,
  DRMTraceCategory_DRIVER = 0x002,
  DRMTraceCategory_KMS = 0x004,
  DRMTraceCategory_PRIME = 0x008,
  DRMTraceCategory_ATOMIC = 0x010,
  DRMTraceCategory_VBL = 0x020,
  DRMTraceCategory_STATE = 0x040,
  DRMTraceCategory_LEASE = 0x080,
  DRMTraceCategory_DP = 0x100,
  DRMTraceCategory_DRMRES = 0x200,
};

// DRMTraceSizes enum. These values must align with those in
// org.chromium.debugd.xml
enum DRMTraceSizes {
  DRMTraceSize_DEFAULT = 0,
  DRMTraceSize_DEBUG = 1,
};

// DRMSnapshotType enum. These values must align with those in
// org.chromium.debugd.xml
enum DRMSnapshotType {
  DRMSnapshotType_TRACE = 0,
  DRMSnapshotType_MODETEST = 1,
};

// FeedbackLogType contains the enum representation of different log categories
// debugd/src/log_tool.h reads from.
enum FeedbackLogType {
  ARC_BUG_REPORT = 0,
  CONNECTIVITY_REPORT = 1,
  VERBOSE_COMMAND_LOGS = 2,
  COMMAND_LOGS = 3,
  FEEDBACK_LOGS = 4,
  BLUETOOTH_BQR = 5,
  LSB_RELEASE_INFO = 6,
  PERF_DATA = 7,
  OS_RELEASE_INFO = 8,
  VAR_LOG_FILES = 9,
  PMT_DATA = 10,
};

// PrintscanDebugCategories flags. These values must align with those in
// org.chromium.debug.xml.
enum PrintscanDebugCategories {
  PrintscanDebugCategory_PRINTING = 0x1,
  PrintscanDebugCategory_SCANNING = 0x2,
};

// Debug log keys which should be substituted in the system info dialog.
const char kIwlwifiDumpKey[] = "iwlwifi_dump";

namespace scheduler_configuration {

// Keys which should be given to SetSchedulerConfiguration.
constexpr char kConservativeScheduler[] = "conservative";
constexpr char kCoreIsolationScheduler[] = "core-scheduling";
constexpr char kPerformanceScheduler[] = "performance";

}  // namespace scheduler_configuration

namespace u2f_flags {
constexpr char kU2f[] = "u2f";
constexpr char kG2f[] = "g2f";
constexpr char kVerbose[] = "verbose";
constexpr char kUserKeys[] = "user_keys";
constexpr char kAllowlistData[] = "allowlist_data";
constexpr char kCorpProtocol[] = "corp_protocol";
}  // namespace u2f_flags

}  // namespace debugd

#endif  // SYSTEM_API_DBUS_DEBUGD_DBUS_CONSTANTS_H_
