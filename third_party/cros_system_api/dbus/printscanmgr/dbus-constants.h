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

}  // namespace printscanmgr

#endif  // SYSTEM_API_DBUS_PRINTSCANMGR_DBUS_CONSTANTS_H_
