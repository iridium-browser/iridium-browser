// Copyright 2018 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_RUNTIME_PROBE_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_RUNTIME_PROBE_DBUS_CONSTANTS_H_

namespace runtime_probe {
const char kRuntimeProbeInterfaceName[] = "org.chromium.RuntimeProbe";
const char kRuntimeProbeServicePath[] = "/org/chromium/RuntimeProbe";
const char kRuntimeProbeServiceName[] = "org.chromium.RuntimeProbe";

// Methods
const char kProbeCategoriesMethod[] = "ProbeCategories";
const char kGetKnownComponentsMethod[] = "GetKnownComponents";

// Constants
const auto kTypeWireless = "wireless";

}  // namespace runtime_probe

#endif  // SYSTEM_API_DBUS_RUNTIME_PROBE_DBUS_CONSTANTS_H_
