// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/dnssd/impl/querier_impl.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "discovery/common/reporting_client.h"
#include "discovery/dnssd/impl/conversion_layer.h"
#include "discovery/dnssd/impl/network_interface_config.h"
#include "platform/api/task_runner.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace discovery {
namespace {

static constexpr char kLocalDomain[] = "local";

// Removes all error instances from the below records, and calls the log
// function on all errors present in |new_endpoints|. Input vectors are expected
// to be sorted in ascending order.
void ProcessErrors(std::vector<ErrorOr<DnsSdInstanceEndpoint>>* old_endpoints,
                   std::vector<ErrorOr<DnsSdInstanceEndpoint>>* new_endpoints,
                   std::function<void(Error)> log) {
  OSP_DCHECK(old_endpoints);
  OSP_DCHECK(new_endpoints);

  auto old_it = old_endpoints->begin();
  auto new_it = new_endpoints->begin();

  // Iterate across both vectors and log new errors in the process.
  // NOTE: In sorted order, all errors will appear before all non-errors.
  while (old_it != old_endpoints->end() && new_it != new_endpoints->end()) {
    ErrorOr<DnsSdInstanceEndpoint>& old_ep = *old_it;
    ErrorOr<DnsSdInstanceEndpoint>& new_ep = *new_it;

    if (new_ep.is_value()) {
      break;
    }

    // If they are equal, the element is in both |old_endpoints| and
    // |new_endpoints|, so skip it in both vectors.
    if (old_ep == new_ep) {
      old_it++;
      new_it++;
      continue;
    }

    // There's an error in |old_endpoints| not in |new_endpoints|, so skip it.
    if (old_ep < new_ep) {
      old_it++;
      continue;
    }

    // There's an error in |new_endpoints| not in |old_endpoints|, so it's a new
    // error from the applied changes. Log it.
    log(std::move(new_ep.error()));
    new_it++;
  }

  // Skip all remaining errors in the old vector.
  for (; old_it != old_endpoints->end() && old_it->is_error(); old_it++) {
  }

  // Log all errors remaining in the new vector.
  for (; new_it != new_endpoints->end() && new_it->is_error(); new_it++) {
    log(std::move(new_it->error()));
  }

  // Erase errors.
  old_endpoints->erase(old_endpoints->begin(), old_it);
  new_endpoints->erase(new_endpoints->begin(), new_it);
}

// Returns a vector containing the value of each ErrorOr<> instance provided.
// All ErrorOr<> values are expected to be non-errors.
std::vector<DnsSdInstanceEndpoint> GetValues(
    std::vector<ErrorOr<DnsSdInstanceEndpoint>> endpoints) {
  std::vector<DnsSdInstanceEndpoint> results;
  results.reserve(endpoints.size());
  for (ErrorOr<DnsSdInstanceEndpoint>& endpoint : endpoints) {
    results.push_back(std::move(endpoint.value()));
  }
  return results;
}

bool IsEqualOrUpdate(const absl::optional<DnsSdInstanceEndpoint>& first,
                     const absl::optional<DnsSdInstanceEndpoint>& second) {
  if (!first.has_value() || !second.has_value()) {
    return !first.has_value() && !second.has_value();
  }

  // In the remaining case, both |first| and |second| must be values.
  const DnsSdInstanceEndpoint& a = first.value();
  const DnsSdInstanceEndpoint& b = second.value();

  // All endpoints from this querier should have the same network interface
  // because the querier is only associated with a single network interface.
  OSP_DCHECK_EQ(a.network_interface(), b.network_interface());

  // Function returns true if first < second.
  return a.instance_id() == b.instance_id() &&
         a.service_id() == b.service_id() && a.domain_id() == b.domain_id();
}

bool IsNotEqualOrUpdate(const absl::optional<DnsSdInstanceEndpoint>& first,
                        const absl::optional<DnsSdInstanceEndpoint>& second) {
  return !IsEqualOrUpdate(first, second);
}

// Calculates the created, updated, and deleted elements using the provided
// sets, appending these values to the provided vectors. Each of the input
// vectors is expected to contain only elements such that
// |element|.is_error() == false. Additionally, input vectors are expected to
// be sorted in ascending order.
//
// NOTE: A lot of operations are used to do this, but each is only O(n) so the
// resulting algorithm is still fast.
void CalculateChangeSets(std::vector<DnsSdInstanceEndpoint> old_endpoints,
                         std::vector<DnsSdInstanceEndpoint> new_endpoints,
                         std::vector<DnsSdInstanceEndpoint>* created_out,
                         std::vector<DnsSdInstanceEndpoint>* updated_out,
                         std::vector<DnsSdInstanceEndpoint>* deleted_out) {
  OSP_DCHECK(created_out);
  OSP_DCHECK(updated_out);
  OSP_DCHECK(deleted_out);

  // Use set difference with default operators to find the elements present in
  // one list but not the others.
  //
  // NOTE: Because absl::optional<...> types are used here and below, calls to
  // the ctor and dtor for empty elements are no-ops.
  const int total_count = old_endpoints.size() + new_endpoints.size();

  // This is the set of elements that aren't in the old endpoints, meaning the
  // old endpoint either didn't exist or had different TXT / Address / etc..
  std::vector<absl::optional<DnsSdInstanceEndpoint>> created_or_updated(
      total_count);
  auto new_end = std::set_difference(new_endpoints.begin(), new_endpoints.end(),
                                     old_endpoints.begin(), old_endpoints.end(),
                                     created_or_updated.begin());
  created_or_updated.erase(new_end, created_or_updated.end());

  // This is the set of elements that are only in the old endpoints, similar to
  // the above.
  std::vector<absl::optional<DnsSdInstanceEndpoint>> deleted_or_updated(
      total_count);
  new_end = std::set_difference(old_endpoints.begin(), old_endpoints.end(),
                                new_endpoints.begin(), new_endpoints.end(),
                                deleted_or_updated.begin());
  deleted_or_updated.erase(new_end, deleted_or_updated.end());

  // Next, find the elements which were updated.
  const size_t max_count =
      std::max(created_or_updated.size(), deleted_or_updated.size());
  std::vector<absl::optional<DnsSdInstanceEndpoint>> updated(max_count);
  new_end = std::set_intersection(
      created_or_updated.begin(), created_or_updated.end(),
      deleted_or_updated.begin(), deleted_or_updated.end(), updated.begin(),
      IsNotEqualOrUpdate);
  updated.erase(new_end, updated.end());

  // Use the updated elements to find all created and deleted elements.
  std::vector<absl::optional<DnsSdInstanceEndpoint>> created(
      created_or_updated.size());
  new_end = std::set_difference(
      created_or_updated.begin(), created_or_updated.end(), updated.begin(),
      updated.end(), created.begin(), IsNotEqualOrUpdate);
  created.erase(new_end, created.end());

  std::vector<absl::optional<DnsSdInstanceEndpoint>> deleted(
      deleted_or_updated.size());
  new_end = std::set_difference(
      deleted_or_updated.begin(), deleted_or_updated.end(), updated.begin(),
      updated.end(), deleted.begin(), IsNotEqualOrUpdate);
  deleted.erase(new_end, deleted.end());

  // Return the calculated elements back to the caller in the output variables.
  created_out->reserve(created.size());
  for (absl::optional<DnsSdInstanceEndpoint>& endpoint : created) {
    OSP_DCHECK(endpoint.has_value());
    created_out->push_back(std::move(endpoint.value()));
  }

  updated_out->reserve(updated.size());
  for (absl::optional<DnsSdInstanceEndpoint>& endpoint : updated) {
    OSP_DCHECK(endpoint.has_value());
    updated_out->push_back(std::move(endpoint.value()));
  }

  deleted_out->reserve(deleted.size());
  for (absl::optional<DnsSdInstanceEndpoint>& endpoint : deleted) {
    OSP_DCHECK(endpoint.has_value());
    deleted_out->push_back(std::move(endpoint.value()));
  }
}

}  // namespace

QuerierImpl::QuerierImpl(MdnsService* mdns_querier,
                         TaskRunner* task_runner,
                         ReportingClient* reporting_client,
                         const NetworkInterfaceConfig* network_config)
    : mdns_querier_(mdns_querier),
      task_runner_(task_runner),
      reporting_client_(reporting_client) {
  OSP_DCHECK(mdns_querier_);
  OSP_DCHECK(task_runner_);

  OSP_DCHECK(network_config);
  graph_ = DnsDataGraph::Create(network_config->network_interface());
}

QuerierImpl::~QuerierImpl() = default;

void QuerierImpl::StartQuery(const std::string& service, Callback* callback) {
  OSP_DCHECK(callback);
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());

  OSP_DVLOG << "Starting DNS-SD query for service '" << service << "'";

  // Start tracking the new callback
  const ServiceKey key(service, kLocalDomain);
  auto it = callback_map_.emplace(key, std::vector<Callback*>{}).first;
  it->second.push_back(callback);

  const DomainName domain = key.GetName();

  // If the associated service isn't tracked yet, start tracking it and start
  // queries for the relevant PTR records.
  if (!graph_->IsTracked(domain)) {
    std::function<void(const DomainName&)> mdns_query(
        [this, &domain](const DomainName& changed_domain) {
          OSP_DVLOG << "Starting mDNS query for '" << domain << "'";
          mdns_querier_->StartQuery(changed_domain, DnsType::kANY,
                                    DnsClass::kANY, this);
        });
    graph_->StartTracking(domain, std::move(mdns_query));
    return;
  }

  // Else, it's already being tracked so fire creation callbacks for any already
  // found service instances.
  const std::vector<ErrorOr<DnsSdInstanceEndpoint>> endpoints =
      graph_->CreateEndpoints(DnsDataGraph::DomainGroup::kPtr, domain);
  for (const auto& endpoint : endpoints) {
    if (endpoint.is_value()) {
      callback->OnEndpointCreated(endpoint.value());
    }
  }
}

void QuerierImpl::StopQuery(const std::string& service, Callback* callback) {
  OSP_DCHECK(callback);
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());

  OSP_DVLOG << "Stopping DNS-SD query for service '" << service << "'";

  ServiceKey key(service, kLocalDomain);
  const auto callbacks_it = callback_map_.find(key);
  if (callbacks_it == callback_map_.end()) {
    return;
  }

  std::vector<Callback*>& callbacks = callbacks_it->second;
  const auto it = std::find(callbacks.begin(), callbacks.end(), callback);
  if (it == callbacks.end()) {
    return;
  }

  callbacks.erase(it);
  if (callbacks.empty()) {
    callback_map_.erase(callbacks_it);

    DomainName domain = key.GetName();

    std::function<void(const DomainName&)> stop_mdns_query(
        [this](const DomainName& changed_domain) {
          OSP_DVLOG << "Stopping mDNS query for '" << changed_domain << "'";
          mdns_querier_->StopQuery(changed_domain, DnsType::kANY,
                                   DnsClass::kANY, this);
        });
    graph_->StopTracking(domain, std::move(stop_mdns_query));
  }
}

bool QuerierImpl::IsQueryRunning(const std::string& service) const {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());
  const ServiceKey key(service, kLocalDomain);
  return graph_->IsTracked(key.GetName());
}

void QuerierImpl::ReinitializeQueries(const std::string& service) {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());

  OSP_DVLOG << "Re-initializing query for service '" << service << "'";

  const ServiceKey key(service, kLocalDomain);
  const DomainName domain = key.GetName();

  std::function<void(const DomainName&)> start_callback(
      [this](const DomainName& d) {
        mdns_querier_->StartQuery(d, DnsType::kANY, DnsClass::kANY, this);
      });
  std::function<void(const DomainName&)> stop_callback(
      [this](const DomainName& d) {
        mdns_querier_->StopQuery(d, DnsType::kANY, DnsClass::kANY, this);
      });
  graph_->StopTracking(domain, std::move(stop_callback));

  // Restart top-level queries.
  mdns_querier_->ReinitializeQueries(GetPtrQueryInfo(key).name);

  graph_->StartTracking(domain, std::move(start_callback));
}

std::vector<PendingQueryChange> QuerierImpl::OnRecordChanged(
    const MdnsRecord& record,
    RecordChangedEvent event) {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());

#ifdef _DEBUG
  OSP_DVLOG << "Record " << record << " has received change of type '" << event
            << "'";
#endif

  std::function<void(Error)> log = [this](Error error) mutable {
    reporting_client_->OnRecoverableError(
        Error(Error::Code::kProcessReceivedRecordFailure));
  };

  // Get the details to use for calling CreateEndpoints(). Special case PTR
  // records to optimize performance.
  const DomainName& create_endpoints_domain =
      record.dns_type() != DnsType::kPTR
          ? record.name()
          : absl::get<PtrRecordRdata>(record.rdata()).ptr_domain();
  const DnsDataGraph::DomainGroup create_endpoints_group =
      record.dns_type() != DnsType::kPTR
          ? DnsDataGraph::GetDomainGroup(record)
          : DnsDataGraph::DomainGroup::kSrvAndTxt;

  // Get the current set of DnsSdInstanceEndpoints prior to this change. Special
  // case PTR records to avoid iterating over unrelated child domains.
  std::vector<ErrorOr<DnsSdInstanceEndpoint>> old_endpoints_or_errors =
      graph_->CreateEndpoints(create_endpoints_group, create_endpoints_domain);

  // Apply the changes, creating a list of all pending changes that should be
  // applied afterwards.
  ErrorOr<std::vector<PendingQueryChange>> pending_changes_or_error =
      ApplyRecordChanges(record, event);
  if (pending_changes_or_error.is_error()) {
    OSP_DVLOG << "Failed to apply changes for " << record.dns_type()
              << " record change of type " << event << " with error "
              << pending_changes_or_error.error();
    log(std::move(pending_changes_or_error.error()));
    return {};
  }
  std::vector<PendingQueryChange>& pending_changes =
      pending_changes_or_error.value();

  // Get the new set of DnsSdInstanceEndpoints following this change.
  std::vector<ErrorOr<DnsSdInstanceEndpoint>> new_endpoints_or_errors =
      graph_->CreateEndpoints(create_endpoints_group, create_endpoints_domain);

  // Return early if the resulting sets are equal. This will frequently be the
  // case, especially when both sets are empty.
  std::sort(old_endpoints_or_errors.begin(), old_endpoints_or_errors.end());
  std::sort(new_endpoints_or_errors.begin(), new_endpoints_or_errors.end());
  if (old_endpoints_or_errors.size() == new_endpoints_or_errors.size() &&
      std::equal(old_endpoints_or_errors.begin(), old_endpoints_or_errors.end(),
                 new_endpoints_or_errors.begin())) {
    return pending_changes;
  }

  // Log all errors and erase them.
  ProcessErrors(&old_endpoints_or_errors, &new_endpoints_or_errors,
                std::move(log));
  const size_t old_endpoints_or_errors_count = old_endpoints_or_errors.size();
  const size_t new_endpoints_or_errors_count = new_endpoints_or_errors.size();
  std::vector<DnsSdInstanceEndpoint> old_endpoints =
      GetValues(std::move(old_endpoints_or_errors));
  std::vector<DnsSdInstanceEndpoint> new_endpoints =
      GetValues(std::move(new_endpoints_or_errors));
  OSP_DCHECK_EQ(old_endpoints.size(), old_endpoints_or_errors_count);
  OSP_DCHECK_EQ(new_endpoints.size(), new_endpoints_or_errors_count);

  // Calculate the changes and call callbacks.
  //
  // NOTE: As the input sets are expected to be small, the generated sets will
  // also be small.
  std::vector<DnsSdInstanceEndpoint> created;
  std::vector<DnsSdInstanceEndpoint> updated;
  std::vector<DnsSdInstanceEndpoint> deleted;
  CalculateChangeSets(std::move(old_endpoints), std::move(new_endpoints),
                      &created, &updated, &deleted);

  InvokeChangeCallbacks(std::move(created), std::move(updated),
                        std::move(deleted));
  return pending_changes;
}

void QuerierImpl::InvokeChangeCallbacks(
    std::vector<DnsSdInstanceEndpoint> created,
    std::vector<DnsSdInstanceEndpoint> updated,
    std::vector<DnsSdInstanceEndpoint> deleted) {
  // Find an endpoint and use it to create the key, or return if there is none.
  DnsSdInstanceEndpoint* some_endpoint;
  if (!created.empty()) {
    some_endpoint = &created.front();
  } else if (!updated.empty()) {
    some_endpoint = &updated.front();
  } else if (!deleted.empty()) {
    some_endpoint = &deleted.front();
  } else {
    return;
  }
  ServiceKey key(some_endpoint->service_id(), some_endpoint->domain_id());

  // Find all callbacks.
  auto it = callback_map_.find(key);
  if (it == callback_map_.end()) {
    return;
  }

  // Call relevant callbacks.
  std::vector<Callback*>& callbacks = it->second;
  for (Callback* callback : callbacks) {
    for (const DnsSdInstanceEndpoint& endpoint : created) {
      callback->OnEndpointCreated(endpoint);
    }
    for (const DnsSdInstanceEndpoint& endpoint : updated) {
      callback->OnEndpointUpdated(endpoint);
    }
    for (const DnsSdInstanceEndpoint& endpoint : deleted) {
      callback->OnEndpointDeleted(endpoint);
    }
  }
}

ErrorOr<std::vector<PendingQueryChange>> QuerierImpl::ApplyRecordChanges(
    const MdnsRecord& record,
    RecordChangedEvent event) {
  std::vector<PendingQueryChange> pending_changes;
  std::function<void(DomainName)> creation_callback(
      [this, &pending_changes](DomainName domain) mutable {
        pending_changes.push_back({std::move(domain), DnsType::kANY,
                                   DnsClass::kANY, this,
                                   PendingQueryChange::kStartQuery});
      });
  std::function<void(DomainName)> deletion_callback(
      [this, &pending_changes](DomainName domain) mutable {
        pending_changes.push_back({std::move(domain), DnsType::kANY,
                                   DnsClass::kANY, this,
                                   PendingQueryChange::kStopQuery});
      });
  Error result =
      graph_->ApplyDataRecordChange(record, event, std::move(creation_callback),
                                    std::move(deletion_callback));
  if (!result.ok()) {
    return result;
  }

  return pending_changes;
}

}  // namespace discovery
}  // namespace openscreen
