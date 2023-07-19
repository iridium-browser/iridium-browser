// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/dnssd/impl/service_dispatcher.h"

#include <utility>

#include "discovery/common/config.h"
#include "discovery/dnssd/impl/service_instance.h"
#include "discovery/dnssd/public/dns_sd_instance.h"
#include "discovery/mdns/public/mdns_service.h"
#include "platform/api/serial_delete_ptr.h"
#include "platform/api/task_runner.h"
#include "util/trace_logging.h"

namespace openscreen {
namespace discovery {
namespace {

void ForAllQueriers(
    std::vector<std::unique_ptr<ServiceInstance>>* service_instances,
    std::function<void(DnsSdQuerier*)> action) {
  for (auto& service_instance : *service_instances) {
    auto* querier = service_instance->GetQuerier();
    OSP_CHECK(querier);

    action(querier);
  }
}

Error ForAllPublishers(
    std::vector<std::unique_ptr<ServiceInstance>>* service_instances,
    std::function<Error(DnsSdPublisher*)> action,
    const char* operation) {
  Error result = Error::None();
  for (auto& service_instance : *service_instances) {
    auto* publisher = service_instance->GetPublisher();
    OSP_CHECK(publisher);

    TRACE_SCOPED(TraceCategory::kDiscovery, operation);
    Error inner_result = action(publisher);
    TRACE_SET_RESULT(inner_result);
    if (!inner_result.ok()) {
      result = std::move(inner_result);
    }
  }
  return result;
}

}  // namespace

// static
SerialDeletePtr<DnsSdService> CreateDnsSdService(
    TaskRunner* task_runner,
    ReportingClient* reporting_client,
    const Config& config) {
  return SerialDeletePtr<DnsSdService>(
      task_runner,
      new ServiceDispatcher(task_runner, reporting_client, config));
}

ServiceDispatcher::ServiceDispatcher(TaskRunner* task_runner,
                                     ReportingClient* reporting_client,
                                     const Config& config)
    : task_runner_(task_runner),
      publisher_(config.enable_publication ? this : nullptr),
      querier_(config.enable_querying ? this : nullptr) {
  OSP_DCHECK_GT(config.network_info.size(), 0);
  OSP_DCHECK(task_runner);

  service_instances_.reserve(config.network_info.size());
  for (const auto& network_info : config.network_info) {
    service_instances_.push_back(std::make_unique<ServiceInstance>(
        task_runner_, reporting_client, config, network_info));
  }
}

ServiceDispatcher::~ServiceDispatcher() {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());
}

// DnsSdQuerier overrides.
void ServiceDispatcher::StartQuery(const std::string& service, Callback* cb) {
  TRACE_DEFAULT_SCOPED(TraceCategory::kDiscovery);
  auto start_query = [&service, cb](DnsSdQuerier* querier) {
    querier->StartQuery(service, cb);
  };
  ForAllQueriers(&service_instances_, std::move(start_query));
}

void ServiceDispatcher::StopQuery(const std::string& service, Callback* cb) {
  TRACE_DEFAULT_SCOPED(TraceCategory::kDiscovery);
  auto stop_query = [&service, cb](DnsSdQuerier* querier) {
    querier->StopQuery(service, cb);
  };
  ForAllQueriers(&service_instances_, std::move(stop_query));
}

void ServiceDispatcher::ReinitializeQueries(const std::string& service) {
  TRACE_DEFAULT_SCOPED(TraceCategory::kDiscovery);
  auto reinitialize_queries = [&service](DnsSdQuerier* querier) {
    querier->ReinitializeQueries(service);
  };
  ForAllQueriers(&service_instances_, std::move(reinitialize_queries));
}

// DnsSdPublisher overrides.
Error ServiceDispatcher::Register(const DnsSdInstance& instance,
                                  Client* client) {
  TRACE_DEFAULT_SCOPED(TraceCategory::kDiscovery);
  auto register_instance = [&instance, client](DnsSdPublisher* publisher) {
    return publisher->Register(instance, client);
  };
  return ForAllPublishers(&service_instances_, std::move(register_instance),
                          "DNS-SD.Register");
}

Error ServiceDispatcher::UpdateRegistration(const DnsSdInstance& instance) {
  TRACE_DEFAULT_SCOPED(TraceCategory::kDiscovery);
  auto update_registration = [&instance](DnsSdPublisher* publisher) {
    return publisher->UpdateRegistration(instance);
  };
  return ForAllPublishers(&service_instances_, std::move(update_registration),
                          "DNS-SD.UpdateRegistration");
}

ErrorOr<int> ServiceDispatcher::DeregisterAll(const std::string& service) {
  TRACE_DEFAULT_SCOPED(TraceCategory::kDiscovery);
  int total = 0;
  Error failure = Error::None();
  for (auto& service_instance : service_instances_) {
    auto* publisher = service_instance->GetPublisher();
    OSP_CHECK(publisher);

    TRACE_SCOPED(TraceCategory::kDiscovery, "DNS-SD.DeregisterAll");
    auto result = publisher->DeregisterAll(service);
    if (result.is_error()) {
      TRACE_SET_RESULT(result.error());
      failure = std::move(result.error());
    } else {
      total += result.value();
    }
  }

  if (!failure.ok()) {
    return failure;
  } else {
    return total;
  }
}

}  // namespace discovery
}  // namespace openscreen
