// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_NETWORK_INTERFACE_H_
#define PLATFORM_IMPL_NETWORK_INTERFACE_H_

#include <vector>

#include "absl/types/optional.h"
#include "platform/base/interface_info.h"

namespace openscreen {

// The below functions are responsible for returning the network interfaces
// provided of the current machine. GetAllInterfaces() returns all interfaces,
// real or virtual. GetLoopbackInterfaceForTesting() returns one such interface
// which is associated with the machine's loopback interface, while
// GetNetworkInterfaces() returns all non-loopback interfaces.
std::vector<InterfaceInfo> GetAllInterfaces();
absl::optional<InterfaceInfo> GetLoopbackInterfaceForTesting();
std::vector<InterfaceInfo> GetNetworkInterfaces();

}  // namespace openscreen

#endif  // PLATFORM_IMPL_NETWORK_INTERFACE_H_
