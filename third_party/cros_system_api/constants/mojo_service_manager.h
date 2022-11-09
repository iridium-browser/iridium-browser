// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_CONSTANTS_MOJO_SERVICE_MANAGER_H_
#define SYSTEM_API_CONSTANTS_MOJO_SERVICE_MANAGER_H_

#include <stdint.h>

namespace chromeos::mojo_service_manager {

// The socket path to connect to the service manager.
constexpr char kSocketPath[] = "/run/mojo/service_manager.sock";
// The  mojo pipe name for bootstrap the mojo.
constexpr int kMojoInvitationPipeName = 0;

}  // namespace chromeos::mojo_service_manager

#endif  // SYSTEM_API_CONSTANTS_MOJO_SERVICE_MANAGER_H_
