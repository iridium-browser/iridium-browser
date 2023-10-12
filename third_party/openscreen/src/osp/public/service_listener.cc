// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/public/service_listener.h"

namespace openscreen {
namespace osp {

ServiceListener::Metrics::Metrics() = default;
ServiceListener::Metrics::~Metrics() = default;

ServiceListener::Config::Config() = default;
ServiceListener::Config::~Config() = default;

bool ServiceListener::Config::IsValid() const {
  return !network_interfaces.empty();
}

ServiceListener::ServiceListener() : state_(State::kStopped) {}
ServiceListener::~ServiceListener() = default;

void ServiceListener::SetConfig(const Config& config) {
  config_ = config;
}

}  // namespace osp
}  // namespace openscreen
