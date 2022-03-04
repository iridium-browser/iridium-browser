// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_DNSSD_IMPL_NETWORK_INTERFACE_CONFIG_H_
#define DISCOVERY_DNSSD_IMPL_NETWORK_INTERFACE_CONFIG_H_

#include "platform/base/interface_info.h"
#include "platform/base/ip_address.h"

namespace openscreen {
namespace discovery {

class NetworkInterfaceConfig {
 public:
  NetworkInterfaceConfig(NetworkInterfaceIndex network_interface,
                         IPAddress address_v4,
                         IPAddress address_v6);
  ~NetworkInterfaceConfig();

  NetworkInterfaceIndex network_interface() const { return network_interface_; }
  const IPAddress& address_v4() const { return address_v4_; }
  const IPAddress& address_v6() const { return address_v6_; }

  bool HasAddressV4() const;
  bool HasAddressV6() const;

  // Returns either the |address_v4_| or |address_v6_| depending which is
  // present, or the empty IPAddress if neither is present.
  const IPAddress& GetAddress() const;

 private:
  friend class FakeNetworkInterfaceConfig;

  NetworkInterfaceConfig();

  NetworkInterfaceIndex network_interface_;
  IPAddress address_v4_;
  IPAddress address_v6_;
};

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_DNSSD_IMPL_NETWORK_INTERFACE_CONFIG_H_
