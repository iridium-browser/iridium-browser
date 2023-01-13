// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_PUBLIC_SERVER_CONFIG_H_
#define OSP_PUBLIC_SERVER_CONFIG_H_

#include <cstdint>
#include <vector>

#include "platform/api/network_interface.h"
#include "platform/base/ip_address.h"

namespace openscreen {
namespace osp {

struct ServerConfig {
  ServerConfig();
  ~ServerConfig();

  // The indexes of network interfaces that should be used by the Open Screen
  // Library.  The indexes derive from the values of
  // openscreen::InterfaceInfo::index.
  std::vector<NetworkInterfaceIndex> interface_indexes;

  // The list of connection endpoints that are advertised for Open Screen
  // protocol connections.  These must be reachable via one interface in
  // |interface_indexes|.
  std::vector<IPEndpoint> connection_endpoints;
};

}  // namespace osp
}  // namespace openscreen

#endif  // OSP_PUBLIC_SERVER_CONFIG_H_
