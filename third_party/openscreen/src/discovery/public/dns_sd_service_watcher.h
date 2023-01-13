// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_PUBLIC_DNS_SD_SERVICE_WATCHER_H_
#define DISCOVERY_PUBLIC_DNS_SD_SERVICE_WATCHER_H_

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "discovery/dnssd/public/dns_sd_instance.h"
#include "discovery/dnssd/public/dns_sd_querier.h"
#include "discovery/dnssd/public/dns_sd_service.h"
#include "platform/base/error.h"
#include "util/hashing.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace discovery {

// This class represents a top-level discovery API which sits on top of DNS-SD.
// T is the service-specific type which stores information regarding a specific
// service instance.
// TODO(rwkeane): Include reporting client as ctor parameter once parallel CLs
// are in.
// NOTE: This class is not thread-safe and calls will be made to DnsSdService in
// the same sequence and on the same threads from which these methods are
// called. This is to avoid forcing design decisions on embedders who write
// their own implementations of the DNS-SD layer.
template <typename T>
class DnsSdServiceWatcher : public DnsSdQuerier::Callback {
 public:
  using ConstRefT = std::reference_wrapper<const T>;

  // The method which will be called when any new service instance is
  // discovered, a service instance changes its data (such as TXT or A data), or
  // a previously discovered service instance ceases to be available. The vector
  // is the set of all currently active service instances which have been
  // discovered so far.
  // NOTE: This callback may not modify the DnsSdServiceWatcher instance from
  // which it is called.
  using ServicesUpdatedCallback =
      std::function<void(std::vector<ConstRefT> services)>;

  // This function type is responsible for converting from a DNS service
  // instance (received from another mDNS endpoint) to a T type to be returned
  // to the caller.
  using ServiceConverter =
      std::function<ErrorOr<T>(const DnsSdInstanceEndpoint&)>;

  DnsSdServiceWatcher(DnsSdService* service,
                      std::string service_name,
                      ServiceConverter conversion,
                      ServicesUpdatedCallback callback)
      : conversion_(conversion),
        service_name_(std::move(service_name)),
        callback_(std::move(callback)),
        querier_(service ? service->GetQuerier() : nullptr) {
    OSP_DCHECK(querier_);
  }

  ~DnsSdServiceWatcher() = default;

  // Starts service discovery.
  void StartDiscovery() {
    OSP_DCHECK(!is_running_);
    is_running_ = true;

    querier_->StartQuery(service_name_, this);
  }

  // Stops service discovery.
  void StopDiscovery() {
    OSP_DCHECK(is_running_);
    is_running_ = false;

    querier_->StopQuery(service_name_, this);
  }

  // Returns whether or not discovery is currently ongoing.
  bool is_running() const { return is_running_; }

  // Re-initializes the process of service discovery, even if the underlying
  // implementation would not normally do so at this time. All previously
  // received service data is discarded.
  // NOTE: This call will return an error if StartDiscovery has not yet been
  // called.
  Error ForceRefresh() {
    if (!is_running_) {
      return Error::Code::kOperationInvalid;
    }

    querier_->ReinitializeQueries(service_name_);
    records_.clear();
    return Error::None();
  }

  // Re-initializes the process of service discovery, even if the underlying
  // implementation would not normally do so at this time. All previously
  // received service data is persisted.
  // NOTE: This call will return an error if StartDiscovery has not yet been
  // called.
  Error DiscoverNow() {
    if (!is_running_) {
      return Error::Code::kOperationInvalid;
    }

    querier_->ReinitializeQueries(service_name_);
    return Error::None();
  }

  // Returns the set of services which have been received so far.
  std::vector<ConstRefT> GetServices() const {
    std::vector<ConstRefT> refs;
    for (const auto& pair : records_) {
      refs.push_back(*pair.second.get());
    }

    OSP_DVLOG << "Currently " << records_.size()
              << " known service instances: [" << GetInstanceNames() << "]";

    return refs;
  }

 private:
  friend class TestServiceWatcher;

  using EndpointKey = std::pair<std::string, NetworkInterfaceIndex>;

  // DnsSdQuerier::Callback overrides.
  void OnEndpointCreated(const DnsSdInstanceEndpoint& new_endpoint) override {
    // NOTE: existence is not checked because records may be overwritten after
    // querier_->ReinitializeQueries() is called.
    ErrorOr<T> record = conversion_(new_endpoint);
    if (record.is_error()) {
      OSP_LOG_INFO << "Conversion of received record failed with error: "
                   << record.error();
      return;
    }
    records_[GetKey(new_endpoint)] =
        std::make_unique<T>(std::move(record.value()));
    callback_(GetServices());
  }

  void OnEndpointUpdated(
      const DnsSdInstanceEndpoint& modified_endpoint) override {
    auto it = records_.find(GetKey(modified_endpoint));
    if (it != records_.end()) {
      ErrorOr<T> record = conversion_(modified_endpoint);
      if (record.is_error()) {
        OSP_LOG_INFO << "Conversion of received record failed with error: "
                     << record.error();
        return;
      }
      auto ptr = std::make_unique<T>(std::move(record.value()));
      it->second.swap(ptr);

      callback_(GetServices());
    } else {
      OSP_LOG_INFO
          << "Received modified record for non-existent DNS-SD Instance "
          << modified_endpoint.instance_id();
    }
  }

  void OnEndpointDeleted(const DnsSdInstanceEndpoint& old_endpoint) override {
    if (records_.erase(GetKey(old_endpoint))) {
      callback_(GetServices());
    } else {
      OSP_LOG_INFO
          << "Received deletion of record for non-existent DNS-SD Instance "
          << old_endpoint.instance_id();
    }
  }

  EndpointKey GetKey(const DnsSdInstanceEndpoint& endpoint) const {
    return std::make_pair(endpoint.instance_id(), endpoint.network_interface());
  }

  std::string GetInstanceNames() const {
    auto it = records_.begin();
    if (it == records_.end()) {
      return "";
    }

    std::stringstream ss;
    ss << it->first.first << "/" << it->first.second;
    while (++it != records_.end()) {
      ss << ", " << it->first.first << "/" << it->first.second;
    }
    return ss.str();
  }

  // Set of all instance ids found so far, mapped to the T type that it
  // represents. unique_ptr<T> entities are used so that the const refs returned
  // from GetServices() and the ServicesUpdatedCallback can persist even once
  // this map is resized.
  // NOTE: Unordered map is used because this set is in  many cases expected to
  // be large.
  std::unordered_map<EndpointKey, std::unique_ptr<T>, PairHash> records_;

  // Represents whether discovery is currently running or not.
  bool is_running_ = false;

  // Converts from the DNS-SD representation of a service to the outside
  // representation.
  ServiceConverter conversion_;

  std::string service_name_;
  ServicesUpdatedCallback callback_;
  DnsSdQuerier* const querier_;
};

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_PUBLIC_DNS_SD_SERVICE_WATCHER_H_
