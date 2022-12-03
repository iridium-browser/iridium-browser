// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/dnssd/impl/network_interface_config.h"

namespace openscreen {
namespace discovery {

NetworkInterfaceConfig::NetworkInterfaceConfig(
    NetworkInterfaceIndex network_interface,
    IPAddress address_v4,
    IPAddress address_v6)
    : network_interface_(network_interface),
      address_v4_(address_v4),
      address_v6_(address_v6) {}

NetworkInterfaceConfig::NetworkInterfaceConfig() = default;

NetworkInterfaceConfig::~NetworkInterfaceConfig() = default;

bool NetworkInterfaceConfig::HasAddressV4() const {
  return address_v4_ ? true : false;
}

bool NetworkInterfaceConfig::HasAddressV6() const {
  return address_v6_ ? true : false;
}

const IPAddress& NetworkInterfaceConfig::GetAddress() const {
  return HasAddressV4() ? address_v4_ : address_v6_;
}

}  // namespace discovery
}  // namespace openscreen
