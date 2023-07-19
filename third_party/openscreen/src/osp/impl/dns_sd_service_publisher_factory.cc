// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <memory>

#include "discovery/dnssd/public/dns_sd_publisher.h"
#include "osp/impl/dns_sd_publisher_client.h"
#include "osp/impl/service_publisher_impl.h"
#include "osp/public/service_publisher.h"
#include "osp/public/service_publisher_factory.h"

namespace openscreen {

class TaskRunner;

namespace osp {

// static
std::unique_ptr<ServicePublisher> ServicePublisherFactory::Create(
    const ServicePublisher::Config& config,
    ServicePublisher::Observer* observer,
    TaskRunner* task_runner) {
  auto dns_sd_client =
      std::make_unique<DnsSdPublisherClient>(observer, task_runner);
  auto publisher_impl = std::make_unique<ServicePublisherImpl>(
      observer, std::move(dns_sd_client));
  publisher_impl->SetConfig(config);
  return publisher_impl;
}

}  // namespace osp
}  // namespace openscreen
