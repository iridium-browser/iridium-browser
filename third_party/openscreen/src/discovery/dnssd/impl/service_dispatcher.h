// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_DNSSD_IMPL_SERVICE_DISPATCHER_H_
#define DISCOVERY_DNSSD_IMPL_SERVICE_DISPATCHER_H_

#include <memory>
#include <string>
#include <vector>

#include "discovery/dnssd/impl/querier_impl.h"
#include "discovery/dnssd/impl/service_instance.h"
#include "discovery/dnssd/public/dns_sd_querier.h"
#include "discovery/dnssd/public/dns_sd_service.h"

namespace openscreen {

class TaskRunner;

namespace discovery {

struct Config;
class ReportingClient;

class ServiceDispatcher final : public DnsSdPublisher,
                                public DnsSdQuerier,
                                public DnsSdService {
 public:
  ServiceDispatcher(TaskRunner& task_runner,
                    ReportingClient* reporting_client,
                    const Config& config);
  ~ServiceDispatcher() override;

  // DnsSdService overrides.
  DnsSdQuerier* GetQuerier() override { return querier_; }
  DnsSdPublisher* GetPublisher() override { return publisher_; }

 private:
  // DnsSdQuerier overrides.
  void StartQuery(const std::string& service, Callback* cb) override;
  void StopQuery(const std::string& service, Callback* cb) override;
  void ReinitializeQueries(const std::string& service) override;

  // DnsSdPublisher overrides.
  Error Register(const DnsSdInstance& instance, Client* client) override;
  Error UpdateRegistration(const DnsSdInstance& instance) override;
  ErrorOr<int> DeregisterAll(const std::string& service) override;

  std::vector<std::unique_ptr<ServiceInstance>> service_instances_;

  TaskRunner& task_runner_;

  // Pointers either to this instance or to nullptr depending whether the below
  // types are supported.
  DnsSdPublisher* const publisher_;
  DnsSdQuerier* const querier_;
};

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_DNSSD_IMPL_SERVICE_DISPATCHER_H_
