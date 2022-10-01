// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_KERBEROS_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_KERBEROS_DBUS_CONSTANTS_H_

namespace kerberos {

// General
const char kKerberosInterface[] = "org.chromium.Kerberos";
const char kKerberosServicePath[] = "/org/chromium/Kerberos";
const char kKerberosServiceName[] = "org.chromium.Kerberos";

// Methods
const char kAddAccountMethod[] = "AddAccount";
const char kRemoveAccountMethod[] = "RemoveAccount";
const char kClearAccountsMethod[] = "ClearAccounts";
const char kListAccountsMethod[] = "ListAccounts";
const char kSetConfigMethod[] = "SetConfig";
const char kValidateConfigMethod[] = "ValidateConfig";
const char kAcquireKerberosTgtMethod[] = "AcquireKerberosTgt";
const char kGetKerberosFilesMethod[] = "GetKerberosFiles";

// Signals
const char kKerberosFilesChangedSignal[] = "KerberosFilesChanged";
const char kKerberosTicketExpiringSignal[] = "KerberosTicketExpiring";

}  // namespace kerberos

#endif  // SYSTEM_API_DBUS_KERBEROS_DBUS_CONSTANTS_H_
