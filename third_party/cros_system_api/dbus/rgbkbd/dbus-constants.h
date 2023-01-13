// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_RGBKBD_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_RGBKBD_DBUS_CONSTANTS_H_

namespace rgbkbd {

const char kRgbkbdServicePath[] = "/org/chromium/Rgbkbd";
const char kRgbkbdServiceName[] = "org.chromium.Rgbkbd";

// Methods
const char kGetRgbKeyboardCapabilities[] = "GetRgbKeyboardCapabilities";
const char kSetCapsLockState[] = "SetCapsLockState";
const char kSetStaticBackgroundColor[] = "SetStaticBackgroundColor";
const char kSetZoneColor[] = "SetZoneColor";
const char kSetRainbowMode[] = "SetRainbowMode";
const char kSetTestingMode[] = "SetTestingMode";
const char kSetAnimationMode[] = "SetAnimationMode";

// Signals
const char kCapabilityUpdatedForTesting[] = "CapabilityUpdatedForTesting";

enum class RgbKeyboardCapabilities {
  kNone = 0,
  kIndividualKey = 1,
  kFourZoneFortyLed = 2,
  kFourZoneTwelveLed = 3,
  kFourZoneFourLed = 4,
  kMaxValue = kFourZoneFourLed,
};

enum class RgbAnimationMode {
  kBasicTestPattern = 0,
};

}  // namespace rgbkbd

#endif  // SYSTEM_API_DBUS_RGBKBD_DBUS_CONSTANTS_H_
