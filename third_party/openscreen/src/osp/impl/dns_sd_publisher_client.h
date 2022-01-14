// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_DNS_SD_PUBLISHER_CLIENT_H_
#define OSP_IMPL_DNS_SD_PUBLISHER_CLIENT_H_

#include <memory>

#include "discovery/common/reporting_client.h"
#include "discovery/dnssd/public/dns_sd_service.h"
#include "discovery/public/dns_sd_service_publisher.h"
#include "osp/impl/service_publisher_impl.h"
#include "platform/api/serial_delete_ptr.h"

namespace openscreen {

class TaskRunner;

namespace osp {

class DnsSdPublisherClient final : public ServicePublisherImpl::Delegate,
                                   openscreen::discovery::ReportingClient {
 public:
  DnsSdPublisherClient(ServicePublisher::Observer* observer,
                       openscreen::TaskRunner* task_runner);
  ~DnsSdPublisherClient() override;

  // ServicePublisherImpl::Delegate overrides.
  void StartPublisher(const ServicePublisher::Config& config) override;
  void StartAndSuspendPublisher(
      const ServicePublisher::Config& config) override;
  void StopPublisher() override;
  void SuspendPublisher() override;
  void ResumePublisher(const ServicePublisher::Config& config) override;

 private:
  DnsSdPublisherClient(const DnsSdPublisherClient&) = delete;
  DnsSdPublisherClient(DnsSdPublisherClient&&) noexcept = delete;

  // openscreen::discovery::ReportingClient overrides.
  void OnFatalError(Error) override;
  void OnRecoverableError(Error) override;

  void StartPublisherInternal(const ServicePublisher::Config& config);
  SerialDeletePtr<discovery::DnsSdService> CreateDnsSdServiceInternal(
      const ServicePublisher::Config& config);

  ServicePublisher::Observer* const observer_;
  TaskRunner* const task_runner_;
  SerialDeletePtr<discovery::DnsSdService> dns_sd_service_;

  using OspDnsSdPublisher =
      discovery::DnsSdServicePublisher<ServicePublisher::Config>;

  std::unique_ptr<OspDnsSdPublisher> dns_sd_publisher_;
};

}  // namespace osp
}  // namespace openscreen

#endif  // OSP_IMPL_DNS_SD_PUBLISHER_CLIENT_H_
