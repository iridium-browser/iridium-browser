// Copyright 2021 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_VM_SK_FORWARDING_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_VM_SK_FORWARDING_DBUS_CONSTANTS_H_

namespace vm_tools {
namespace sk_forwarding {

const char kVmSKForwardingServiceName[] = "org.chromium.VmSKForwardingService";
const char kVmSKForwardingServicePath[] = "/org/chromium/VmSKForwardingService";
const char kVmSKForwardingServiceInterface[] =
    "org.chromium.VmSKForwardingService";

const char kVmSKForwardingServiceForwardSecurityKeyMessageMethod[] =
    "ForwardSecurityKeyMessage";

}  // namespace sk_forwarding
}  // namespace vm_tools

#endif  // SYSTEM_API_DBUS_VM_SK_FORWARDING_DBUS_CONSTANTS_H_
