// Copyright 2015 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_LORGNETTE_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_LORGNETTE_DBUS_CONSTANTS_H_

namespace lorgnette {
const char kManagerServiceName[] = "org.chromium.lorgnette";
const char kManagerServiceInterface[] = "org.chromium.lorgnette.Manager";
const char kManagerServicePath[] = "/org/chromium/lorgnette/Manager";
const char kManagerServiceError[] = "org.chromium.lorgnette.Error";

// Methods.
const char kListScannersMethod[] = "ListScanners";
const char kGetScannerCapabilitiesMethod[] = "GetScannerCapabilities";
const char kScanImageMethod[] = "ScanImage";
const char kStartScanMethod[] = "StartScan";
const char kCancelScanMethod[] = "CancelScan";
const char kGetNextImageMethod[] = "GetNextImage";

// Signals.
const char kScanStatusChangedSignal[] = "ScanStatusChanged";

// Parameters supplied to a "ScanImage" request.
const char kScanPropertyMode[] = "Mode";
const char kScanPropertyModeColor[] = "Color";
const char kScanPropertyModeGray[] = "Gray";
const char kScanPropertyModeLineart[] = "Lineart";
const char kScanPropertyResolution[] = "Resolution";

// The name used as a source name for the only DocumentSource in
// ScannerCapabilities' source field if the SANE backend does not export a
// source option.
const char kUnspecifiedDefaultSourceName[] = "DefaultSource";
}  // namespace lorgnette

#endif  // SYSTEM_API_DBUS_LORGNETTE_DBUS_CONSTANTS_H_
