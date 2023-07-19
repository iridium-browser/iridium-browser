// Copyright 2017 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_SWITCHES_MODEMFWD_SWITCHES_H_
#define SYSTEM_API_SWITCHES_MODEMFWD_SWITCHES_H_

// This file defines switches that are used by modemfwd.
// See its README.md file for more information about what each one does.

#include <string>
#include <vector>

namespace modemfwd {

const char kGetFirmwareInfo[] = "get_fw_info";
const char kShillFirmwareRevision[] = "shill_fw_revision";
const char kPrepareToFlash[] = "prepare_to_flash";
const char kFlashFirmware[] = "flash_fw";
const char kFlashModeCheck[] = "flash_mode_check";
const char kReboot[] = "reboot";
const char kClearAttachAPN[] = "clear_attach_apn";
const char kFwVersion[] = "fw_version";

// Keys used for the kFlashFirmware/kFwVersion/kGetFirmwareInfo switches
const char kFwMain[] = "main";
const char kFwCarrier[] = "carrier";
const char kFwOem[] = "oem";
const char kFwAp[] = "ap";
const char kFwDev[] = "dev";
const char kFwCarrierUuid[] = "carrier_uuid";

}  // namespace modemfwd

#endif  // SYSTEM_API_SWITCHES_MODEMFWD_SWITCHES_H_
