// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <memory>

#include "osp/impl/dns_sd_watcher_client.h"
#include "osp/impl/service_listener_impl.h"
#include "osp/public/service_listener.h"
#include "osp/public/service_listener_factory.h"

namespace openscreen {

class TaskRunner;

namespace osp {

// static
std::unique_ptr<ServiceListener> ServiceListenerFactory::Create(
    const ServiceListener::Config& config,
    TaskRunner& task_runner) {
  auto dns_sd_client = std::make_unique<DnsSdWatcherClient>(task_runner);
  auto listener_impl =
      std::make_unique<ServiceListenerImpl>(std::move(dns_sd_client));
  listener_impl->SetConfig(config);
  return listener_impl;
}

}  // namespace osp
}  // namespace openscreen
