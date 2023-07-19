// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_DNSSD_TESTING_FAKE_NETWORK_INTERFACE_CONFIG_H_
#define DISCOVERY_DNSSD_TESTING_FAKE_NETWORK_INTERFACE_CONFIG_H_

#include "discovery/dnssd/impl/network_interface_config.h"
#include "gmock/gmock.h"

namespace openscreen {
namespace discovery {

class FakeNetworkInterfaceConfig : public NetworkInterfaceConfig {
 public:
  FakeNetworkInterfaceConfig() = default;

  void set_network_interface(NetworkInterfaceIndex network_interface) {
    network_interface_ = network_interface;
  }

  void set_address_v4(IPAddress address) { address_v4_ = std::move(address); }

  void set_address_v6(IPAddress address) { address_v6_ = std::move(address); }
};

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_DNSSD_TESTING_FAKE_NETWORK_INTERFACE_CONFIG_H_
