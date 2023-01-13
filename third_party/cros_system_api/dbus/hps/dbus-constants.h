// Copyright 2021 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_HPS_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_HPS_DBUS_CONSTANTS_H_

namespace hps {

// Use of this API is restricted by policy. Consult
// go/cros-pdd#bookmark=id.7emuxnhxv638 and Chrome OS Privacy before using.

constexpr char kHpsServiceInterface[] = "org.chromium.Hps";
constexpr char kHpsServicePath[] = "/org/chromium/Hps";
constexpr char kHpsServiceName[] = "org.chromium.Hps";

// Methods exported by hpsd.
constexpr char kEnableHpsSense[] = "EnableHpsSense";
constexpr char kDisableHpsSense[] = "DisableHpsSense";
constexpr char kGetResultHpsSense[] = "GetResultHpsSense";
constexpr char kEnableHpsNotify[] = "EnableHpsNotify";
constexpr char kDisableHpsNotify[] = "DisableHpsNotify";
constexpr char kGetResultHpsNotify[] = "GetResultHpsNotify";

// Signals emitted by hpsd.
constexpr char kHpsSenseChanged[] = "HpsSenseChanged";
constexpr char kHpsNotifyChanged[] = "HpsNotifyChanged";

}  // namespace hps

#endif  // SYSTEM_API_DBUS_HPS_DBUS_CONSTANTS_H_
