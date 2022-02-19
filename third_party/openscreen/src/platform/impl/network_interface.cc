// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/network_interface.h"

#include "util/std_util.h"

namespace openscreen {

std::vector<InterfaceInfo> GetNetworkInterfaces() {
  std::vector<InterfaceInfo> interfaces = GetAllInterfaces();

  const auto new_end = std::remove_if(
      interfaces.begin(), interfaces.end(), [](const InterfaceInfo& info) {
        return info.type != InterfaceInfo::Type::kEthernet &&
               info.type != InterfaceInfo::Type::kWifi &&
               info.type != InterfaceInfo::Type::kOther;
      });
  interfaces.erase(new_end, interfaces.end());

  return interfaces;
}

// Returns an InterfaceInfo associated with the system's loopback interface.
absl::optional<InterfaceInfo> GetLoopbackInterfaceForTesting() {
  const std::vector<InterfaceInfo> interfaces = GetAllInterfaces();
  auto it = std::find_if(
      interfaces.begin(), interfaces.end(), [](const InterfaceInfo& info) {
        return info.type == InterfaceInfo::Type::kLoopback &&
               ContainsIf(info.addresses, [](const IPSubnet& subnet) {
                 return subnet.address == IPAddress::kV4LoopbackAddress() ||
                        subnet.address == IPAddress::kV6LoopbackAddress();
               });
      });

  if (it == interfaces.end()) {
    return absl::nullopt;
  } else {
    return *it;
  }
}

}  // namespace openscreen
