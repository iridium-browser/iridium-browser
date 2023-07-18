// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/dns_sd_publisher_client.h"

#include <utility>

#include "discovery/common/config.h"
#include "discovery/dnssd/public/dns_sd_instance.h"
#include "discovery/dnssd/public/dns_sd_txt_record.h"
#include "discovery/public/dns_sd_service_factory.h"
#include "osp/public/service_info.h"
#include "platform/base/macros.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace osp {

using State = ServicePublisher::State;

namespace {

constexpr char kFriendlyNameTxtKey[] = "fn";
constexpr char kDnsSdDomainId[] = "local";

discovery::DnsSdInstance ServiceConfigToDnsSdInstance(
    const ServicePublisher::Config& config) {
  discovery::DnsSdTxtRecord txt;
  const bool did_set_everything =
      txt.SetValue(kFriendlyNameTxtKey, config.friendly_name).ok();
  OSP_DCHECK(did_set_everything);

  // NOTE: Not totally clear how we should be using config.hostname, which in
  // principle is already part of config.service_instance_name.
  return discovery::DnsSdInstance(
      config.service_instance_name, kOpenScreenServiceName, kDnsSdDomainId,
      std::move(txt), config.connection_server_port);
}

}  // namespace

DnsSdPublisherClient::DnsSdPublisherClient(ServicePublisher::Observer* observer,
                                           openscreen::TaskRunner* task_runner)
    : observer_(observer), task_runner_(task_runner) {
  OSP_DCHECK(observer_);
  OSP_DCHECK(task_runner_);
}

DnsSdPublisherClient::~DnsSdPublisherClient() = default;

void DnsSdPublisherClient::StartPublisher(
    const ServicePublisher::Config& config) {
  OSP_LOG_INFO << "StartPublisher with " << config.network_interfaces.size()
               << " interfaces";
  StartPublisherInternal(config);
  Error result = dns_sd_publisher_->Register(config);
  if (result.ok()) {
    SetState(State::kRunning);
  } else {
    OnFatalError(result);
    SetState(State::kStopped);
  }
}

void DnsSdPublisherClient::StartAndSuspendPublisher(
    const ServicePublisher::Config& config) {
  StartPublisherInternal(config);
  SetState(State::kSuspended);
}

void DnsSdPublisherClient::StopPublisher() {
  dns_sd_publisher_.reset();
  SetState(State::kStopped);
}

void DnsSdPublisherClient::SuspendPublisher() {
  OSP_DCHECK(dns_sd_publisher_);
  dns_sd_publisher_->DeregisterAll();
  SetState(State::kSuspended);
}

void DnsSdPublisherClient::ResumePublisher(
    const ServicePublisher::Config& config) {
  OSP_DCHECK(dns_sd_publisher_);
  dns_sd_publisher_->Register(config);
  SetState(State::kRunning);
}

void DnsSdPublisherClient::OnFatalError(Error error) {
  observer_->OnError(error);
}

void DnsSdPublisherClient::OnRecoverableError(Error error) {
  observer_->OnError(error);
}

void DnsSdPublisherClient::StartPublisherInternal(
    const ServicePublisher::Config& config) {
  OSP_DCHECK(!dns_sd_publisher_);
  if (!dns_sd_service_) {
    dns_sd_service_ = CreateDnsSdServiceInternal(config);
  }
  dns_sd_publisher_ = std::make_unique<OspDnsSdPublisher>(
      dns_sd_service_.get(), kOpenScreenServiceName,
      ServiceConfigToDnsSdInstance);
}

SerialDeletePtr<discovery::DnsSdService>
DnsSdPublisherClient::CreateDnsSdServiceInternal(
    const ServicePublisher::Config& config) {
  // NOTE: With the current API, the client cannot customize the behavior of
  // DNS-SD beyond the interface list.
  openscreen::discovery::Config dns_sd_config;
  dns_sd_config.enable_querying = false;
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
  return CreateDnsSdService(task_runner_, this, dns_sd_config);
}

}  // namespace osp
}  // namespace openscreen
