// Copyright 2021 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_VM_LAUNCH_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_VM_LAUNCH_DBUS_CONSTANTS_H_

namespace vm_tools {
namespace launch {

// The VM Launching service provides helper methods for launching VMs.
//
// For the most part, VMs are launched via Chrome's UI, but in cases where users
// prefer a command-line experience, this API is used to inform chrome about the
// launch process, and request various setup work.

const char kVmLaunchServiceName[] = "org.chromium.VmLaunchService";
const char kVmLaunchServicePath[] = "/org/chromium/VmLaunchService";
const char kVmLaunchServiceInterface[] = "org.chromium.VmLaunchService";

// As part of go/secure-exo-ids, each VM will use its own wayland server. When
// the launch request originates from Chrome, this is handled already, but when
// it originates from another system (vmc or concierge) we need to ask chrome
// for the server.
const char kVmLaunchServiceStartWaylandServerMethod[] = "StartWaylandServer";

// Clean-up helper for the above method.
const char kVmLaunchServiceStopWaylandServerMethod[] = "StopWaylandServer";

// Provide a token that will be used to allow/disallow certain VMs from running.
//
// TODO(b/218403711): Remove this method.
const char kVmLaunchServiceProvideVmTokenMethod[] = "ProvideVmToken";

// Ask chrome to launch a VM product (crostini, borealis, etc). This is
// preferable to using `vmc start` directly as the required arguments for that
// method changes depending on configuration, and Chrome is the source-of-truth
// as to what the correct arguments should be.
const char kVmLaunchServiceEnsureVmLaunchedMethod[] = "EnsureVmLaunched";

}  // namespace launch
}  // namespace vm_tools

#endif  // SYSTEM_API_DBUS_VM_LAUNCH_DBUS_CONSTANTS_H_
