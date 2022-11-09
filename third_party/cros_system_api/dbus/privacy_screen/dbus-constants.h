// Copyright 2021 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_PRIVACY_SCREEN_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_PRIVACY_SCREEN_DBUS_CONSTANTS_H_

namespace privacy_screen {

// Privacy screen service in Chromium:
constexpr char kPrivacyScreenServiceName[] =
    "org.chromium.PrivacyScreenService";
constexpr char kPrivacyScreenServicePath[] =
    "/org/chromium/PrivacyScreenService";
constexpr char kPrivacyScreenServiceInterface[] =
    "org.chromium.PrivacyScreenService";

// Methods:
constexpr char kPrivacyScreenServiceGetPrivacyScreenSettingMethod[] =
    "GetPrivacyScreenSetting";

// Signals:
constexpr char kPrivacyScreenServicePrivacyScreenSettingChangedSignal[] =
    "PrivacyScreenSettingChanged";

}  // namespace privacy_screen

#endif  // SYSTEM_API_DBUS_PRIVACY_SCREEN_DBUS_CONSTANTS_H_
