// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_NETWORK_INTERFACE_H_
#define PLATFORM_IMPL_NETWORK_INTERFACE_H_

#include <vector>

#include "absl/types/optional.h"
#include "platform/base/interface_info.h"

namespace openscreen {

// Implements the platform API.
std::vector<InterfaceInfo> GetNetworkInterfaces();

// Returns the system's loopback interface.  Used for unit tests.
absl::optional<InterfaceInfo> GetLoopbackInterfaceForTesting();

}  // namespace openscreen

#endif  // PLATFORM_IMPL_NETWORK_INTERFACE_H_
