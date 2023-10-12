// Copyright 2023 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_VM_WL_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_VM_WL_DBUS_CONSTANTS_H_

namespace vm_tools::wl {

// The VM WL service is used to managed wayland servers/connections between VM
// infra and chrome/compositor.

const char kVmWlServiceName[] = "org.chromium.VmWlService";
const char kVmWlServicePath[] = "/org/chromium/VmWlService";
const char kVmWlServiceInterface[] = "org.chromium.VmWlService";

// Given some metadata about the intended vm and an FD created with the socket()
// call, turn the FD into a wayland server designed to serve that VM.
const char kVmWlServiveListenOnSocketMethod[] = "ListenOnSocket";

// Clean-up helper for the ListenOnSocket method.
const char kVmWlServiceCloseSocketMethod[] = "CloseSocket";

}  // namespace vm_tools::wl

#endif  // SYSTEM_API_DBUS_VM_WL_DBUS_CONSTANTS_H_
