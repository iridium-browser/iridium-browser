// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/dnssd/impl/publisher_impl.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "absl/types/optional.h"
#include "discovery/common/reporting_client.h"
#include "discovery/dnssd/impl/conversion_layer.h"
#include "discovery/dnssd/impl/instance_key.h"
#include "discovery/dnssd/impl/network_interface_config.h"
#include "discovery/mdns/public/mdns_constants.h"
#include "platform/api/task_runner.h"
#include "platform/base/error.h"
#include "util/trace_logging.h"

namespace openscreen {
namespace discovery {
namespace {

DnsSdInstanceEndpoint CreateEndpoint(
    DnsSdInstance instance,
    InstanceKey key,
    const NetworkInterfaceConfig& network_config) {
  std::vector<IPEndpoint> endpoints;
  if (network_config.HasAddressV4()) {
    endpoints.push_back({network_config.address_v4(), instance.port()});
  }
  if (network_config.HasAddressV6()) {
    endpoints.push_back({network_config.address_v6(), instance.port()});
  }
  return DnsSdInstanceEndpoint(
      key.instance_id(), key.service_id(), key.domain_id(), instance.txt(),
      network_config.network_interface(), std::move(endpoints));
}

DnsSdInstanceEndpoint UpdateDomain(
    const DomainName& name,
    DnsSdInstance instance,
    const NetworkInterfaceConfig& network_config) {
  return CreateEndpoint(std::move(instance), InstanceKey(name), network_config);
}

DnsSdInstanceEndpoint CreateEndpoint(
    DnsSdInstance instance,
    const NetworkInterfaceConfig& network_config) {
  InstanceKey key(instance);
  return CreateEndpoint(std::move(instance), std::move(key), network_config);
}

template <typename T>
inline typename std::map<DnsSdInstance, T>::iterator FindKey(
    std::map<DnsSdInstance, T>* instances,
    const InstanceKey& key) {
  return std::find_if(instances->begin(), instances->end(),
                      [&key](const std::pair<DnsSdInstance, T>& pair) {
                        return key == InstanceKey(pair.first);
                      });
}

template <typename T>
int EraseInstancesWithServiceId(std::map<DnsSdInstance, T>* instances,
                                const std::string& service_id) {
  int removed_count = 0;
  for (auto it = instances->begin(); it != instances->end();) {
    if (it->first.service_id() == service_id) {
      removed_count++;
      it = instances->erase(it);
    } else {
      it++;
    }
  }

  return removed_count;
}

}  // namespace

PublisherImpl::PublisherImpl(MdnsService* publisher,
                             ReportingClient* reporting_client,
                             TaskRunner* task_runner,
                             const NetworkInterfaceConfig* network_config)
    : mdns_publisher_(publisher),
      reporting_client_(reporting_client),
      task_runner_(task_runner),
      network_config_(network_config) {
  OSP_DCHECK(mdns_publisher_);
  OSP_DCHECK(reporting_client_);
  OSP_DCHECK(task_runner_);
}

PublisherImpl::~PublisherImpl() = default;

Error PublisherImpl::Register(const DnsSdInstance& instance, Client* client) {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());
  OSP_DCHECK(client != nullptr);

  if (published_instances_.find(instance) != published_instances_.end()) {
    UpdateRegistration(instance);
  } else if (pending_instances_.find(instance) != pending_instances_.end()) {
    return Error::Code::kOperationInProgress;
  }

  InstanceKey key(instance);
  const IPAddress& address = network_config_->GetAddress();
  OSP_DCHECK(address);
  pending_instances_.emplace(CreateEndpoint(instance, *network_config_),
                             client);

  OSP_DVLOG << "Registering instance '" << instance.instance_id() << "'";

  return mdns_publisher_->StartProbe(this, GetDomainName(key), address);
}

Error PublisherImpl::UpdateRegistration(const DnsSdInstance& instance) {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());

  // Check if the instance is still pending publication.
  auto it = FindKey(&pending_instances_, InstanceKey(instance));

  OSP_DVLOG << "Updating instance '" << instance.instance_id() << "'";

  // If it is a pending instance, update it. Else, try to update a published
  // instance.
  if (it != pending_instances_.end()) {
    // The instance, service, and domain ids have not changed, so only the
    // remaining data needs to change. The ongoing probe does not need to be
    // modified.
    Client* const client = it->second;
    pending_instances_.erase(it);
    pending_instances_.emplace(CreateEndpoint(instance, *network_config_),
                               client);
    return Error::None();
  } else {
    return UpdatePublishedRegistration(instance);
  }
}

Error PublisherImpl::UpdatePublishedRegistration(
    const DnsSdInstance& instance) {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());

  auto published_instance_it =
      FindKey(&published_instances_, InstanceKey(instance));

  // Check preconditions called out in header. Specifically, the updated
  // instance must be making changes to an already published instance.
  if (published_instance_it == published_instances_.end()) {
    return Error::Code::kParameterInvalid;
  }

  const DnsSdInstanceEndpoint updated_endpoint =
      UpdateDomain(GetDomainName(InstanceKey(published_instance_it->second)),
                   instance, *network_config_);
  if (published_instance_it->second == updated_endpoint) {
    return Error::Code::kParameterInvalid;
  }

  // Get all instances which have changed. By design, there an only be one
  // instance of each DnsType, so use that here to simplify this step. First in
  // each pair is the old instances, second is the new instance.
  std::map<DnsType,
           std::pair<absl::optional<MdnsRecord>, absl::optional<MdnsRecord>>>
      changed_records;
  const std::vector<MdnsRecord> old_records =
      GetDnsRecords(published_instance_it->second);
  const std::vector<MdnsRecord> new_records = GetDnsRecords(updated_endpoint);

  // Populate the first part of each pair in |changed_instances|.
  for (size_t i = 0; i < old_records.size(); i++) {
    const auto key = old_records[i].dns_type();
    OSP_DCHECK(changed_records.find(key) == changed_records.end());
    auto value = std::make_pair(std::move(old_records[i]), absl::nullopt);
    changed_records.emplace(key, std::move(value));
  }

  // Populate the second part of each pair in |changed_records|.
  for (size_t i = 0; i < new_records.size(); i++) {
    const auto key = new_records[i].dns_type();
    auto find_it = changed_records.find(key);
    if (find_it == changed_records.end()) {
      std::pair<absl::optional<MdnsRecord>, absl::optional<MdnsRecord>> value(
          absl::nullopt, std::move(new_records[i]));
      changed_records.emplace(key, std::move(value));
    } else {
      find_it->second.second = std::move(new_records[i]);
    }
  }

  // Apply changes called out in |changed_records|.
  Error total_result = Error::None();
  for (const auto& pair : changed_records) {
    OSP_DCHECK(pair.second.first != absl::nullopt ||
               pair.second.second != absl::nullopt);
    if (pair.second.first == absl::nullopt) {
      TRACE_SCOPED(TraceCategory::kDiscovery, "mdns.RegisterRecord");
      auto error = mdns_publisher_->RegisterRecord(pair.second.second.value());
      TRACE_SET_RESULT(error);
      if (!error.ok()) {
        total_result = error;
      }
    } else if (pair.second.second == absl::nullopt) {
      TRACE_SCOPED(TraceCategory::kDiscovery, "mdns.UnregisterRecord");
      auto error = mdns_publisher_->UnregisterRecord(pair.second.first.value());
      TRACE_SET_RESULT(error);
      if (!error.ok()) {
        total_result = error;
      }
    } else if (pair.second.first.value() != pair.second.second.value()) {
      TRACE_SCOPED(TraceCategory::kDiscovery, "mdns.UpdateRegisteredRecord");
      auto error = mdns_publisher_->UpdateRegisteredRecord(
          pair.second.first.value(), pair.second.second.value());
      TRACE_SET_RESULT(error);
      if (!error.ok()) {
        total_result = error;
      }
    }
  }

  // Replace the old instances with the new ones.
  published_instances_.erase(published_instance_it);
  published_instances_.emplace(instance, std::move(updated_endpoint));

  return total_result;
}

ErrorOr<int> PublisherImpl::DeregisterAll(const std::string& service) {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());

  OSP_DVLOG << "Deregistering all instances";

  int removed_count = 0;
  Error error = Error::None();
  for (auto it = published_instances_.begin();
       it != published_instances_.end();) {
    if (it->second.service_id() == service) {
      for (const auto& mdns_record : GetDnsRecords(it->second)) {
        TRACE_SCOPED(TraceCategory::kDiscovery, "mdns.UnregisterRecord");
        auto publisher_error = mdns_publisher_->UnregisterRecord(mdns_record);
        TRACE_SET_RESULT(error);
        if (!publisher_error.ok()) {
          error = publisher_error;
        }
      }
      removed_count++;
      it = published_instances_.erase(it);
    } else {
      it++;
    }
  }

  removed_count += EraseInstancesWithServiceId(&pending_instances_, service);

  if (!error.ok()) {
    return error;
  } else {
    return removed_count;
  }
}

void PublisherImpl::OnDomainFound(const DomainName& requested_name,
                                  const DomainName& confirmed_name) {
  TRACE_DEFAULT_SCOPED(TraceCategory::kDiscovery);
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());

  OSP_DVLOG << "Domain successfully claimed: '" << confirmed_name
            << "' based on requested name: '" << requested_name << "'";

  auto it = FindKey(&pending_instances_, InstanceKey(requested_name));

  if (it == pending_instances_.end()) {
    // This will be hit if the instance was deregister'd before the probe phase
    // was completed.
    return;
  }

  DnsSdInstance requested_instance = std::move(it->first);
  DnsSdInstanceEndpoint endpoint =
      CreateEndpoint(requested_instance, *network_config_);
  Client* const client = it->second;
  pending_instances_.erase(it);

  InstanceKey requested_key(requested_instance);

  if (requested_name != confirmed_name) {
    OSP_DCHECK(HasValidDnsRecordAddress(confirmed_name));
    endpoint =
        UpdateDomain(confirmed_name, requested_instance, *network_config_);
  }

  for (const auto& mdns_record : GetDnsRecords(endpoint)) {
    TRACE_SCOPED(TraceCategory::kDiscovery, "mdns.RegisterRecord");
    Error result = mdns_publisher_->RegisterRecord(mdns_record);
    if (!result.ok()) {
      reporting_client_->OnRecoverableError(
          Error(Error::Code::kRecordPublicationError, result.ToString()));
    }
  }

  auto pair = published_instances_.emplace(std::move(requested_instance),
                                           std::move(endpoint));
  client->OnEndpointClaimed(pair.first->first, pair.first->second);
}

}  // namespace discovery
}  // namespace openscreen
