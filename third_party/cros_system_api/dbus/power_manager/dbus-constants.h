// Copyright 2015 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_POWER_MANAGER_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_POWER_MANAGER_DBUS_CONSTANTS_H_

namespace power_manager {

// powerd
const char kPowerManagerInterface[] = "org.chromium.PowerManager";
const char kPowerManagerServicePath[] = "/org/chromium/PowerManager";
const char kPowerManagerServiceName[] = "org.chromium.PowerManager";

// Methods exposed by powerd.
const char kSetScreenBrightnessMethod[] = "SetScreenBrightness";
const char kDecreaseScreenBrightnessMethod[] = "DecreaseScreenBrightness";
const char kIncreaseScreenBrightnessMethod[] = "IncreaseScreenBrightness";
const char kGetScreenBrightnessPercentMethod[] = "GetScreenBrightnessPercent";
const char kGetKeyboardBrightnessPercentMethod[] =
    "GetKeyboardBrightnessPercent";
const char kSetKeyboardBrightnessMethod[] = "SetKeyboardBrightness";
const char kDecreaseKeyboardBrightnessMethod[] = "DecreaseKeyboardBrightness";
const char kIncreaseKeyboardBrightnessMethod[] = "IncreaseKeyboardBrightness";
const char kToggleKeyboardBacklightMethod[] = "ToggleKeyboardBacklight";
const char kRequestRestartMethod[] = "RequestRestart";
const char kRequestShutdownMethod[] = "RequestShutdown";
const char kRequestSuspendMethod[] = "RequestSuspend";
const char kGetPowerSupplyPropertiesMethod[] = "GetPowerSupplyProperties";
const char kGetBatteryStateMethod[] = "GetBatteryState";
const char kGetSwitchStatesMethod[] = "GetSwitchStates";
const char kHandleUserActivityMethod[] = "HandleUserActivity";
const char kHandleVideoActivityMethod[] = "HandleVideoActivity";
const char kHandleWakeNotificationMethod[] = "HandleWakeNotification";
const char kSetIsProjectingMethod[] = "SetIsProjecting";
const char kSetPolicyMethod[] = "SetPolicy";
const char kSetPowerSourceMethod[] = "SetPowerSource";
const char kSetBacklightsForcedOffMethod[] = "SetBacklightsForcedOff";
const char kGetBacklightsForcedOffMethod[] = "GetBacklightsForcedOff";
const char kRegisterSuspendDelayMethod[] = "RegisterSuspendDelay";
const char kUnregisterSuspendDelayMethod[] = "UnregisterSuspendDelay";
const char kHandleSuspendReadinessMethod[] = "HandleSuspendReadiness";
const char kRegisterDarkSuspendDelayMethod[] = "RegisterDarkSuspendDelay";
const char kUnregisterDarkSuspendDelayMethod[] = "UnregisterDarkSuspendDelay";
const char kHandleDarkSuspendReadinessMethod[] = "HandleDarkSuspendReadiness";
const char kHandlePowerButtonAcknowledgmentMethod[] =
    "HandlePowerButtonAcknowledgment";
const char kIgnoreNextPowerButtonPressMethod[] = "IgnoreNextPowerButtonPress";
const char kRecordDarkResumeWakeReasonMethod[] = "RecordDarkResumeWakeReason";
const char kGetInactivityDelaysMethod[] = "GetInactivityDelays";
const char kCreateArcTimersMethod[] = "CreateArcTimers";
const char kStartArcTimerMethod[] = "StartArcTimer";
const char kDeleteArcTimersMethod[] = "DeleteArcTimers";
const char kChangeWifiRegDomainMethod[] = "ChangeWifiRegDomain";
const char kGetTabletModeMethod[] = "GetTabletMode";
const char kChargeNowForAdaptiveChargingMethod[] =
    "ChargeNowForAdaptiveCharging";
const char kGetChargeHistoryMethod[] = "GetChargeHistory";
const char kRefreshAllPeripheralBatteryMethod[] = "RefreshAllPeripheralBattery";
const char kGetThermalStateMethod[] = "GetThermalState";
const char kSetExternalDisplayALSBrightnessMethod[] =
    "SetExternalDisplayALSBrightness";
const char kGetExternalDisplayALSBrightnessMethod[] =
    "GetExternalDisplayALSBrightness";
const char kGetBatterySaverModeState[] = "GetBatterySaverModeState";
const char kSetBatterySaverModeState[] = "SetBatterySaverModeState";

// Signals emitted by powerd.
const char kScreenBrightnessChangedSignal[] = "ScreenBrightnessChanged";
const char kKeyboardBrightnessChangedSignal[] = "KeyboardBrightnessChanged";
const char kPeripheralBatteryStatusSignal[] = "PeripheralBatteryStatus";
const char kPowerSupplyPollSignal[] = "PowerSupplyPoll";
const char kBatteryStatePollSignal[] = "BatteryStatePoll";
const char kSuspendImminentSignal[] = "SuspendImminent";
const char kDarkSuspendImminentSignal[] = "DarkSuspendImminent";
const char kSuspendDoneSignal[] = "SuspendDone";
const char kHibernateResumeReadySignal[] = "HibernateResumeReady";
const char kInputEventSignal[] = "InputEvent";
const char kIdleActionImminentSignal[] = "IdleActionImminent";
const char kIdleActionDeferredSignal[] = "IdleActionDeferred";
const char kScreenIdleStateChangedSignal[] = "ScreenIdleStateChanged";
const char kInactivityDelaysChangedSignal[] = "InactivityDelaysChanged";
const char kAmbientColorTemperatureChangedSignal[] =
    "AmbientColorTemperatureChanged";
const char kLidClosedSignal[] = "LidClosed";
const char kLidOpenedSignal[] = "LidOpened";
const char kThermalEventSignal[] = "ThermalEvent";
const char kBatterySaverModeStateChanged[] = "BatterySaverModeStateChanged";

// Values
const int kBrightnessTransitionGradual = 1;
const int kBrightnessTransitionInstant = 2;
enum UserActivityType {
  USER_ACTIVITY_OTHER = 0,
  USER_ACTIVITY_BRIGHTNESS_UP_KEY_PRESS = 1,
  USER_ACTIVITY_BRIGHTNESS_DOWN_KEY_PRESS = 2,
  USER_ACTIVITY_VOLUME_UP_KEY_PRESS = 3,
  USER_ACTIVITY_VOLUME_DOWN_KEY_PRESS = 4,
  USER_ACTIVITY_VOLUME_MUTE_KEY_PRESS = 5,
};
enum RequestRestartReason {
  // An explicit user request (e.g. clicking a button).
  REQUEST_RESTART_FOR_USER = 0,
  // A system update.
  REQUEST_RESTART_FOR_UPDATE = 1,
  // Some other reason.
  REQUEST_RESTART_OTHER = 2,
  // DeviceScheduledReboot policy.
  REQUEST_RESTART_SCHEDULED_REBOOT_POLICY = 3,
  // Remote action reboot from the admin console.
  REQUEST_RESTART_REMOTE_ACTION_REBOOT = 4,
  // chrome.runtime.restart API.
  REQUEST_RESTART_API = 5,
};
enum RequestShutdownReason {
  // An explicit user request (e.g. clicking a button).
  REQUEST_SHUTDOWN_FOR_USER = 0,
  // Some other reason.
  REQUEST_SHUTDOWN_OTHER = 1,
};
enum WifiRegDomainDbus {
  WIFI_REG_DOMAIN_FCC = 0,
  WIFI_REG_DOMAIN_EU = 1,
  WIFI_REG_DOMAIN_REST_OF_WORLD = 2,
  WIFI_REG_DOMAIN_NONE = 3,
};
enum RequestSuspendFlavor {
  REQUEST_SUSPEND_DEFAULT = 0,
  REQUEST_SUSPEND_TO_RAM = 1,
  REQUEST_SUSPEND_TO_DISK = 2,
  RESUME_FROM_DISK_PREPARE = 3,
  RESUME_FROM_DISK_ABORT = 4,
};

}  // namespace power_manager

#endif  // SYSTEM_API_DBUS_POWER_MANAGER_DBUS_CONSTANTS_H_
