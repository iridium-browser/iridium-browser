// Copyright 2019 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_ANOMALY_DETECTOR_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_ANOMALY_DETECTOR_DBUS_CONSTANTS_H_

namespace anomaly_detector {

const char kAnomalyEventServiceName[] = "org.chromium.AnomalyEventService";
const char kAnomalyEventServicePath[] = "/org/chromium/AnomalyEventService";
const char kAnomalyEventServiceInterface[] =
    "org.chromium.AnomalyEventServiceInterface";

const char kAnomalyEventSignalName[] = "AnomalyEvent";
const char kAnomalyGuestFileCorruptionSignalName[] = "GuestFileCorruption";
const char kAnomalyGuestOomEventSignalName[] = "GuestOomEvent";
const char kAnomalyVmKernelLogMethod[] = "VmKernelLog";

}  // namespace anomaly_detector

#endif  // SYSTEM_API_DBUS_ANOMALY_DETECTOR_DBUS_CONSTANTS_H_
