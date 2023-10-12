// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_DNS_SD_PUBLISHER_CLIENT_H_
#define OSP_IMPL_DNS_SD_PUBLISHER_CLIENT_H_

#include <memory>

#include "discovery/dnssd/public/dns_sd_service.h"
#include "discovery/public/dns_sd_service_publisher.h"
#include "osp/impl/service_publisher_impl.h"
#include "platform/api/task_runner_deleter.h"

namespace openscreen {

class TaskRunner;

namespace osp {

class DnsSdPublisherClient final : public ServicePublisherImpl::Delegate {
 public:
  explicit DnsSdPublisherClient(TaskRunner& task_runner);
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

  void StartPublisherInternal(const ServicePublisher::Config& config);
  std::unique_ptr<discovery::DnsSdService, TaskRunnerDeleter>
  CreateDnsSdServiceInternal(const ServicePublisher::Config& config);

  TaskRunner& task_runner_;
  std::unique_ptr<discovery::DnsSdService, TaskRunnerDeleter> dns_sd_service_;

  using OspDnsSdPublisher =
      discovery::DnsSdServicePublisher<ServicePublisher::Config>;

  std::unique_ptr<OspDnsSdPublisher> dns_sd_publisher_;
};

}  // namespace osp
}  // namespace openscreen

#endif  // OSP_IMPL_DNS_SD_PUBLISHER_CLIENT_H_
