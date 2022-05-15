// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/public/service_publisher.h"

namespace openscreen {
namespace osp {

ServicePublisher::Metrics::Metrics() = default;
ServicePublisher::Metrics::~Metrics() = default;

ServicePublisher::Config::Config() = default;
ServicePublisher::Config::~Config() = default;

bool ServicePublisher::Config::IsValid() const {
  return !friendly_name.empty() && !service_instance_name.empty() &&
         connection_server_port > 0 && !network_interfaces.empty();
}

ServicePublisher::~ServicePublisher() = default;

void ServicePublisher::SetConfig(const Config& config) {
  config_ = config;
}

ServicePublisher::ServicePublisher(Observer* observer)
    : state_(State::kStopped), observer_(observer) {}

}  // namespace osp
}  // namespace openscreen
