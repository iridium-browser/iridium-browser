// Copyright 2023 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_PRINTSCANMGR_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_PRINTSCANMGR_DBUS_CONSTANTS_H_

namespace printscanmgr {
const char kPrintscanmgrInterface[] = "org.chromium.printscanmgr";
const char kPrintscanmgrServicePath[] = "/org/chromium/printscanmgr";
const char kPrintscanmgrServiceName[] = "org.chromium.printscanmgr";

// Methods.
const char kCupsAddManuallyConfiguredPrinter[] =
    "CupsAddManuallyConfiguredPrinter";
const char kCupsAddAutoConfiguredPrinter[] = "CupsAddAutoConfiguredPrinter";
const char kCupsRemovePrinter[] = "CupsRemovePrinter";
const char kCupsRetrievePpd[] = "CupsRetrievePpd";
const char kPrintscanDebugSetCategories[] = "PrintscanDebugSetCategories";

// CupsAdd* error codes.
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

// PrintscanDebugCategories flags. These values must align with those in
// org.chromium.printscanmgr.xml.
enum PrintscanDebugCategories {
  PrintscanDebugCategory_PRINTING = 0x1,
  PrintscanDebugCategory_SCANNING = 0x2,
};

}  // namespace printscanmgr

#endif  // SYSTEM_API_DBUS_PRINTSCANMGR_DBUS_CONSTANTS_H_
