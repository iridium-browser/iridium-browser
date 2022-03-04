// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_BLUETOOTH_AVAILABLE_UTILITY_H_
#define CHROME_BROWSER_METRICS_BLUETOOTH_AVAILABLE_UTILITY_H_

namespace bluetooth_utility {

// Reports the bluetooth availability of the system's hardware.
// This currently only works on ChromeOS and Windows. For Bluetooth
// availability on OS X, see chrome/browser/mac/bluetooth_utility.h.
void ReportBluetoothAvailability();

}  // namespace bluetooth_utility

#endif  // CHROME_BROWSER_METRICS_BLUETOOTH_AVAILABLE_UTILITY_H_
