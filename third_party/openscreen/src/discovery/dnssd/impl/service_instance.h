// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_DNSSD_IMPL_SERVICE_INSTANCE_H_
#define DISCOVERY_DNSSD_IMPL_SERVICE_INSTANCE_H_

#include <memory>

#include "discovery/dnssd/impl/network_interface_config.h"
#include "discovery/dnssd/impl/publisher_impl.h"
#include "discovery/dnssd/impl/querier_impl.h"
#include "discovery/dnssd/public/dns_sd_service.h"
#include "platform/base/interface_info.h"

namespace openscreen {

class TaskRunner;

namespace discovery {

class MdnsService;

class ServiceInstance final : public DnsSdService {
 public:
  ServiceInstance(TaskRunner* task_runner,
                  ReportingClient* reporting_client,
                  const Config& config,
                  const InterfaceInfo& network_info);
  ServiceInstance(const ServiceInstance& other) = delete;
  ServiceInstance(ServiceInstance&& other) noexcept = delete;
  ~ServiceInstance() override;

  ServiceInstance& operator=(const ServiceInstance& other) = delete;
  ServiceInstance& operator=(ServiceInstance&& other) = delete;

  const NetworkInterfaceConfig& network_config() const {
    return network_config_;
  }

  // DnsSdService overrides.
  DnsSdQuerier* GetQuerier() override { return querier_.get(); }
  DnsSdPublisher* GetPublisher() override { return publisher_.get(); }

 private:
  TaskRunner* const task_runner_;

  std::unique_ptr<MdnsService> mdns_service_;

  std::unique_ptr<QuerierImpl> querier_;
  std::unique_ptr<PublisherImpl> publisher_;

  const NetworkInterfaceConfig network_config_;
};

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_DNSSD_IMPL_SERVICE_INSTANCE_H_
