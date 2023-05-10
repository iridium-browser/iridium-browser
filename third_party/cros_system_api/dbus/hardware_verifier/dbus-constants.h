// Copyright 2021 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_HARDWARE_VERIFIER_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_HARDWARE_VERIFIER_DBUS_CONSTANTS_H_

namespace hardware_verifier {
const char kHardwareVerifierInterfaceName[] = "org.chromium.HardwareVerifier";
const char kHardwareVerifierServicePath[] = "/org/chromium/HardwareVerifier";
const char kHardwareVerifierServiceName[] = "org.chromium.HardwareVerifier";

// Methods
const char kVerifyComponentsMethod[] = "VerifyComponents";

}  // namespace hardware_verifier

#endif  // SYSTEM_API_DBUS_HARDWARE_VERIFIER_DBUS_CONSTANTS_H_
