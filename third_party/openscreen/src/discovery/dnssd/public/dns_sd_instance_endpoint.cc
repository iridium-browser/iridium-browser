// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/dnssd/public/dns_sd_instance_endpoint.h"

#include <algorithm>
#include <cctype>
#include <utility>
#include <vector>

#include "util/osp_logging.h"

namespace openscreen {
namespace discovery {

DnsSdInstanceEndpoint::DnsSdInstanceEndpoint(
    std::string instance_id,
    std::string service_id,
    std::string domain_id,
    DnsSdTxtRecord txt,
    NetworkInterfaceIndex network_interface,
    std::vector<IPEndpoint> endpoints)
    : DnsSdInstanceEndpoint(std::move(instance_id),
                            std::move(service_id),
                            std::move(domain_id),
                            std::move(txt),
                            network_interface,
                            std::move(endpoints),
                            std::vector<Subtype>{}) {}

DnsSdInstanceEndpoint::DnsSdInstanceEndpoint(
    std::string instance_id,
    std::string service_id,
    std::string domain_id,
    DnsSdTxtRecord txt,
    NetworkInterfaceIndex network_interface,
    std::vector<IPEndpoint> endpoints,
    std::vector<Subtype> subtypes)
    : DnsSdInstance(std::move(instance_id),
                    std::move(service_id),
                    std::move(domain_id),
                    std::move(txt),
                    endpoints.empty() ? 0 : endpoints[0].port,
                    std::move(subtypes)),
      endpoints_(std::move(endpoints)),
      network_interface_(network_interface) {
  InitializeEndpoints();
}

DnsSdInstanceEndpoint::DnsSdInstanceEndpoint(
    DnsSdInstance instance,
    NetworkInterfaceIndex network_interface,
    std::vector<IPEndpoint> endpoints)
    : DnsSdInstance(std::move(instance)),
      endpoints_(std::move(endpoints)),
      network_interface_(network_interface) {
  InitializeEndpoints();
}

DnsSdInstanceEndpoint::DnsSdInstanceEndpoint(
    const DnsSdInstanceEndpoint& other) = default;

DnsSdInstanceEndpoint::DnsSdInstanceEndpoint(DnsSdInstanceEndpoint&& other) =
    default;

DnsSdInstanceEndpoint::~DnsSdInstanceEndpoint() = default;

DnsSdInstanceEndpoint& DnsSdInstanceEndpoint::operator=(
    const DnsSdInstanceEndpoint& rhs) = default;

DnsSdInstanceEndpoint& DnsSdInstanceEndpoint::operator=(
    DnsSdInstanceEndpoint&& rhs) = default;

void DnsSdInstanceEndpoint::InitializeEndpoints() {
  OSP_CHECK(!endpoints_.empty());
  std::sort(endpoints_.begin(), endpoints_.end());
  for (const auto& endpoint : endpoints_) {
    OSP_DCHECK_EQ(endpoint.port, port());
    addresses_.push_back(endpoint.address);
  }
}

bool operator<(const DnsSdInstanceEndpoint& lhs,
               const DnsSdInstanceEndpoint& rhs) {
  if (lhs.network_interface_ != rhs.network_interface_) {
    return lhs.network_interface_ < rhs.network_interface_;
  }

  if (lhs.endpoints_.size() != rhs.endpoints_.size()) {
    return lhs.endpoints_.size() < rhs.endpoints_.size();
  }

  for (int i = 0; i < static_cast<int>(lhs.endpoints_.size()); i++) {
    if (lhs.endpoints_[i] != rhs.endpoints_[i]) {
      return lhs.endpoints_[i] < rhs.endpoints_[i];
    }
  }

  return static_cast<const DnsSdInstance&>(lhs) <
         static_cast<const DnsSdInstance&>(rhs);
}

}  // namespace discovery
}  // namespace openscreen
