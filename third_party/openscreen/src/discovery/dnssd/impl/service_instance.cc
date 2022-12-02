// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/dnssd/impl/service_instance.h"

#include <utility>

#include "discovery/common/config.h"
#include "discovery/mdns/public/mdns_service.h"
#include "platform/api/task_runner.h"

namespace openscreen {
namespace discovery {

ServiceInstance::ServiceInstance(TaskRunner* task_runner,
                                 ReportingClient* reporting_client,
                                 const Config& config,
                                 const InterfaceInfo& network_info)
    : task_runner_(task_runner),
      mdns_service_(MdnsService::Create(task_runner,
                                        reporting_client,
                                        config,
                                        network_info)),
      network_config_(network_info.index,
                      network_info.GetIpAddressV4(),
                      network_info.GetIpAddressV6()) {
  if (config.enable_querying) {
    querier_ = std::make_unique<QuerierImpl>(
        mdns_service_.get(), task_runner_, reporting_client, &network_config_);
  }
  if (config.enable_publication) {
    publisher_ = std::make_unique<PublisherImpl>(
        mdns_service_.get(), reporting_client, task_runner_, &network_config_);
  }
}

ServiceInstance::~ServiceInstance() {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());
}

}  // namespace discovery
}  // namespace openscreen
