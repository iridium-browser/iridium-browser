// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/dns_sd_watcher_client.h"

#include <string>
#include <utility>

#include "discovery/common/config.h"
#include "discovery/dnssd/public/dns_sd_instance.h"
#include "discovery/dnssd/public/dns_sd_instance_endpoint.h"
#include "discovery/dnssd/public/dns_sd_txt_record.h"
#include "discovery/public/dns_sd_service_factory.h"
#include "platform/base/error.h"
#include "platform/base/interface_info.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace osp {

using State = ServiceListener::State;

namespace {

constexpr char kFriendlyNameTxtKey[] = "fn";

ErrorOr<ServiceInfo> DnsSdInstanceEndpointToServiceInfo(
    const discovery::DnsSdInstanceEndpoint& endpoint) {
  if (endpoint.service_id() != kOpenScreenServiceName) {
    return {Error::Code::kParameterInvalid, "Not a Open Screen receiver."};
  }
  if (endpoint.network_interface() == kInvalidNetworkInterfaceIndex) {
    return {Error::Code::kParameterInvalid, "Invalid network inferface index."};
  }
  std::string friendly_name =
      endpoint.txt().GetStringValue(kFriendlyNameTxtKey).value("");
  if (friendly_name.empty()) {
    return {Error::Code::kParameterInvalid,
            "Missing receiver friendly name in record."};
  }

  ServiceInfo service_info{endpoint.service_id(), friendly_name,
                           endpoint.network_interface()};
  for (const IPEndpoint& record : endpoint.endpoints()) {
    if (!service_info.v4_endpoint && record.address.IsV4()) {
      service_info.v4_endpoint = record;
    }
    if (!service_info.v6_endpoint && record.address.IsV6()) {
      service_info.v6_endpoint = record;
    }
  }
  if (!service_info.v4_endpoint && !service_info.v6_endpoint) {
    return {Error::Code::kParameterInvalid,
            "No IPv4 nor IPv6 address in record."};
  }

  return service_info;
}

}  // namespace

DnsSdWatcherClient::DnsSdWatcherClient(TaskRunner& task_runner)
    : task_runner_(task_runner) {}

DnsSdWatcherClient::~DnsSdWatcherClient() = default;

void DnsSdWatcherClient::StartListener(const ServiceListener::Config& config) {
  OSP_LOG_INFO << "StartListener with " << config.network_interfaces.size()
               << " interfaces";
  StartWatcherInternal(config);
  dns_sd_watcher_->StartDiscovery();
  SetState(State::kRunning);
}

void DnsSdWatcherClient::StartAndSuspendListener(
    const ServiceListener::Config& config) {
  StartWatcherInternal(config);
  SetState(State::kSuspended);
}

void DnsSdWatcherClient::StopListener() {
  dns_sd_watcher_.reset();
  SetState(State::kStopped);
}

void DnsSdWatcherClient::SuspendListener() {
  OSP_DCHECK(dns_sd_watcher_);
  dns_sd_watcher_->StopDiscovery();
  SetState(State::kSuspended);
}

void DnsSdWatcherClient::ResumeListener() {
  OSP_DCHECK(dns_sd_watcher_);
  dns_sd_watcher_->StartDiscovery();
  SetState(State::kRunning);
}

void DnsSdWatcherClient::SearchNow(State from) {
  OSP_DCHECK(dns_sd_watcher_);
  if (from == State::kSuspended) {
    dns_sd_watcher_->StartDiscovery();
  }

  dns_sd_watcher_->DiscoverNow();
  SetState(State::kSearching);
}

void DnsSdWatcherClient::StartWatcherInternal(
    const ServiceListener::Config& config) {
  OSP_DCHECK(!dns_sd_watcher_);
  if (!dns_sd_service_) {
    dns_sd_service_ = CreateDnsSdServiceInternal(config);
  }
  dns_sd_watcher_ = std::make_unique<OspDnsSdWatcher>(
      dns_sd_service_.get(), kOpenScreenServiceName,
      DnsSdInstanceEndpointToServiceInfo,
      [this](std::vector<OspDnsSdWatcher::ConstRefT> all) {
        OnDnsWatcherUpdated(std::move(all));
      });
}

SerialDeletePtr<discovery::DnsSdService>
DnsSdWatcherClient::CreateDnsSdServiceInternal(
    const ServiceListener::Config& config) {
  // NOTE: With the current API, the client cannot customize the behavior of
  // DNS-SD beyond the interface list.
  openscreen::discovery::Config dns_sd_config;
  dns_sd_config.enable_publication = false;
  dns_sd_config.network_info = config.network_interfaces;

  // NOTE:
  // It's desirable for the DNS-SD publisher and the DNS-SD listener for OSP to
  // share the underlying mDNS socket and state, to avoid the agent from
  // binding 2 sockets per network interface.
  //
  // This can be accomplished by having the agent use a shared instance of the
  // discovery::DnsSdService, e.g. through a ref-counting handle, so that the
  // OSP publisher and the OSP listener don't have to coordinate through an
  // additional object.
  return CreateDnsSdService(task_runner_, listener_, dns_sd_config);
}

void DnsSdWatcherClient::OnDnsWatcherUpdated(
    std::vector<OspDnsSdWatcher::ConstRefT> all) {
  std::vector<ServiceInfo> discovered_services;
  for (const ServiceInfo& service : all) {
    if (!service.v4_endpoint && !service.v6_endpoint) {
      continue;
    }
    discovered_services.push_back(service);
  }
  listener_->OnReceiverUpdated(discovered_services);
}

}  // namespace osp
}  // namespace openscreen
