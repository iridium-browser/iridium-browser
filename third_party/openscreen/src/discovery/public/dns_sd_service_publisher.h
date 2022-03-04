// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_PUBLIC_DNS_SD_SERVICE_PUBLISHER_H_
#define DISCOVERY_PUBLIC_DNS_SD_SERVICE_PUBLISHER_H_

#include <string>
#include <utility>

#include "discovery/dnssd/public/dns_sd_instance.h"
#include "discovery/dnssd/public/dns_sd_instance_endpoint.h"
#include "discovery/dnssd/public/dns_sd_publisher.h"
#include "discovery/dnssd/public/dns_sd_service.h"
#include "platform/base/error.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace discovery {

// This class represents a top-level discovery API which sits on top of DNS-SD.
// The main purpose of this class is to hide DNS-SD internals from embedders who
// do not care about the specific functionality and do not need to understand
// DNS-SD Internals.
// T is the service-specific type which stores information regarding a specific
// service instance.
// NOTE: This class is not thread-safe and calls will be made to DnsSdService in
// the same sequence and on the same threads from which these methods are
// called. This is to avoid forcing design decisions on embedders who write
// their own implementations of the DNS-SD layer.
template <typename T>
class DnsSdServicePublisher : public DnsSdPublisher::Client {
 public:
  // This function type is responsible for converting from a T type to a
  // DNS service instance (to publish to the network).
  using ServiceInstanceConverter = std::function<DnsSdInstance(const T&)>;

  DnsSdServicePublisher(DnsSdService* service,
                        std::string service_name,
                        ServiceInstanceConverter conversion)
      : conversion_(conversion),
        service_name_(std::move(service_name)),
        publisher_(service ? service->GetPublisher() : nullptr) {
    OSP_DCHECK(publisher_);
  }

  ~DnsSdServicePublisher() = default;

  Error Register(const T& service) {
    if (!service.IsValid()) {
      return Error::Code::kParameterInvalid;
    }

    DnsSdInstance instance = conversion_(service);
    return publisher_->Register(instance, this);
  }

  Error UpdateRegistration(const T& service) {
    if (!service.IsValid()) {
      return Error::Code::kParameterInvalid;
    }

    DnsSdInstance instance = conversion_(service);
    return publisher_->UpdateRegistration(instance);
  }

  ErrorOr<int> DeregisterAll() {
    return publisher_->DeregisterAll(service_name_);
  }

 protected:
  // DnsSdPublisher::Client overrides.
  //
  // Embedders who care about the instance id with which the service was
  // published may override this method.
  void OnEndpointClaimed(
      const DnsSdInstance& requested_instance,
      const DnsSdInstanceEndpoint& claimed_endpoint) override {
    OSP_DVLOG << "Instance ID '" << claimed_endpoint.instance_id()
              << "' claimed for requested ID '"
              << requested_instance.instance_id() << "'";
    OnInstanceClaimed(requested_instance.instance_id());
  }

  virtual void OnInstanceClaimed(const std::string& requested_instance_id) {}

 private:
  ServiceInstanceConverter conversion_;
  std::string service_name_;
  DnsSdPublisher* const publisher_;
};

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_PUBLIC_DNS_SD_SERVICE_PUBLISHER_H_
