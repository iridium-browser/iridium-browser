// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_AUDIO_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_AUDIO_DBUS_CONSTANTS_H_

namespace cras {
const char kCrasServicePath[] = "/org/chromium/cras";
const char kCrasServiceName[] = "org.chromium.cras";
const char kCrasControlInterface[] = "org.chromium.cras.Control";

// Methods.
const char kSetOutputVolume[] = "SetOutputVolume";
const char kSetOutputNodeVolume[] = "SetOutputNodeVolume";
const char kSwapLeftRight[] = "SwapLeftRight";
const char kSetDisplayRotation[] = "SetDisplayRotation";
const char kSetOutputMute[] = "SetOutputMute";
const char kSetOutputUserMute[] = "SetOutputUserMute";
const char kSetSuspendAudio[] = "SetSuspendAudio";
const char kSetInputGain[] = "SetInputGain";
const char kSetInputNodeGain[] = "SetInputNodeGain";
const char kSetInputMute[] = "SetInputMute";
const char kGetVolumeState[] = "GetVolumeState";
const char kGetDefaultOutputBufferSize[] = "GetDefaultOutputBufferSize";
const char kGetNodes[] = "GetNodes";
const char kSetActiveOutputNode[] = "SetActiveOutputNode";
const char kSetActiveInputNode[] = "SetActiveInputNode";
const char kSetHotwordModel[] = "SetHotwordModel";
const char kAddActiveOutputNode[] = "AddActiveOutputNode";
const char kAddActiveInputNode[] = "AddActiveInputNode";
const char kRemoveActiveOutputNode[] = "RemoveActiveOutputNode";
const char kRemoveActiveInputNode[] = "RemoveActiveInputNode";
const char kGetNumberOfActiveStreams[] = "GetNumberOfActiveStreams";
const char kGetNumberOfActiveInputStreams[] = "GetNumberOfActiveInputStreams";
const char kGetNumberOfActiveOutputStreams[] = "GetNumberOfActiveOutputStreams";
const char kGetNumberOfInputStreamsWithPermission[] =
    "GetNumberOfInputStreamsWithPermission";
const char kGetNumberOfNonChromeOutputStreams[] =
    "GetNumberOfNonChromeOutputStreams";
const char kIsAudioOutputActive[] = "IsAudioOutputActive";
const char kSetGlobalOutputChannelRemix[] = "SetGlobalOutputChannelRemix";
const char kGetSystemAecSupported[] = "GetSystemAecSupported";
const char kGetSystemAecGroupId[] = "GetSystemAecGroupId";
const char kGetSystemNsSupported[] = "GetSystemNsSupported";
const char kGetSystemAgcSupported[] = "GetSystemAgcSupported";
const char kSetPlayerPlaybackStatus[] = "SetPlayerPlaybackStatus";
const char kSetPlayerIdentity[] = "SetPlayerIdentity";
const char kSetPlayerPosition[] = "SetPlayerPosition";
const char kSetPlayerMetadata[] = "SetPlayerMetadata";
const char kSetNextHandsfreeProfile[] = "SetNextHandsfreeProfile";
const char kSetFixA2dpPacketSize[] = "SetFixA2dpPacketSize";
const char kResendBluetoothBattery[] = "ResendBluetoothBattery";
const char kGetDeprioritizeBtWbsMic[] = "GetDeprioritizeBtWbsMic";
const char kSetNoiseCancellationEnabled[] = "SetNoiseCancellationEnabled";
const char kIsNoiseCancellationSupported[] = "IsNoiseCancellationSupported";
const char kSetFlossEnabled[] = "SetFlossEnabled";
const char kSetSpeakOnMuteDetection[] = "SetSpeakOnMuteDetection";
const char kSpeakOnMuteDetectionEnabled[] = "SpeakOnMuteDetectionEnabled";

// Names of properties returned by GetNodes() and GetNodeInfos()
const char kIsInputProperty[] = "IsInput";
const char kIdProperty[] = "Id";
const char kTypeProperty[] = "Type";
const char kNameProperty[] = "Name";
const char kDeviceNameProperty[] = "DeviceName";
const char kActiveProperty[] = "Active";
const char kPluggedTimeProperty[] = "PluggedTime";
const char kStableDeviceIdProperty[] = "StableDeviceId";
const char kStableDeviceIdNewProperty[] = "StableDeviceIdNew";
const char kMaxSupportedChannelsProperty[] = "MaxSupportedChannels";
const char kAudioEffectProperty[] = "AudioEffect";
const char kNodeVolumeProperty[] = "NodeVolume";
const char kInputNodeGainProperty[] = "InputNodeGain";
const char kNumberOfVolumeStepsProperty[] = "NumberOfVolumeSteps";
// The following two properties are optional.
const char kNumberOfUnderrunsProperty[] = "NumberOfUnderruns";
const char kNumberOfSevereUnderrunsProperty[] = "NumberOfSevereUnderruns";
enum AudioEffectType {
  EFFECT_TYPE_NOISE_CANCELLATION = 1 << 0,
};
// Screen Rotation in clock-wise degrees.
// This enum corresponds to enum Rotation in chromium ui/display/display.h
enum class DisplayRotation {
  ROTATE_0 = 0,
  ROTATE_90,
  ROTATE_180,
  ROTATE_270,
  NUM_DISPLAY_ROTATION,
};
// Names of properties returned by
// * method - GetNumberOfInputStreamsWithPermission and
// * signal - NumberOfInputStreamsWithPermissionChanged.
const char kClientType[] = "ClientType";
const char kNumStreamsWithPermission[] = "NumStreamsWithPermission";

// Signals.
const char kOutputVolumeChanged[] = "OutputVolumeChanged";
const char kOutputMuteChanged[] = "OutputMuteChanged";
const char kOutputNodeVolumeChanged[] = "OutputNodeVolumeChanged";
const char kNodeLeftRightSwappedChanged[] = "NodeLeftRightSwappedChanged";
const char kInputGainChanged[] = "InputGainChanged";
const char kInputMuteChanged[] = "InputMuteChanged";
const char kInputNodeGainChanged[] = "InputNodeGainChanged";
const char kNodesChanged[] = "NodesChanged";
const char kActiveOutputNodeChanged[] = "ActiveOutputNodeChanged";
const char kActiveInputNodeChanged[] = "ActiveInputNodeChanged";
const char kNumberOfActiveStreamsChanged[] = "NumberOfActiveStreamsChanged";
const char kNumberOfInputStreamsWithPermissionChanged[] =
    "NumberOfInputStreamsWithPermissionChanged";
const char kNumberOfNonChromeOutputStreamsChanged[] =
    "NumberOfNonChromeOutputStreamsChanged";
const char kAudioOutputActiveStateChanged[] = "AudioOutputActiveStateChanged";
const char kHotwordTriggered[] = "HotwordTriggered";
const char kBluetoothBatteryChanged[] = "BluetoothBatteryChanged";
const char kSurveyTrigger[] = "SurveyTrigger";
const char kSpeakOnMuteDetected[] = "SpeakOnMuteDetected";
}  // namespace cras

#endif  // SYSTEM_API_DBUS_AUDIO_DBUS_CONSTANTS_H_
