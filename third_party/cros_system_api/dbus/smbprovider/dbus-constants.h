// Copyright 2017 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_SMBPROVIDER_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_SMBPROVIDER_DBUS_CONSTANTS_H_

namespace smbprovider {

// General
const char kSmbProviderInterface[] = "org.chromium.SmbProvider";
const char kSmbProviderServicePath[] = "/org/chromium/SmbProvider";
const char kSmbProviderServiceName[] = "org.chromium.SmbProvider";

// Methods
const char kGetSharesMethod[] = "GetShares";
const char kSetupKerberosMethod[] = "SetupKerberos";
const char kParseNetBiosPacketMethod[] = "ParseNetBiosPacket";

}  // namespace smbprovider

#endif  // SYSTEM_API_DBUS_SMBPROVIDER_DBUS_CONSTANTS_H_
