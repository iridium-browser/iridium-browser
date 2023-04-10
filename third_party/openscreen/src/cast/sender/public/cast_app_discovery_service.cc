// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/sender/public/cast_app_discovery_service.h"

namespace openscreen {
namespace cast {

CastAppDiscoveryService::Subscription::Subscription(
    CastAppDiscoveryService* discovery_service,
    uint32_t id)
    : discovery_service_(discovery_service), id_(id) {}

CastAppDiscoveryService::Subscription::Subscription(
    Subscription&& other) noexcept
    : discovery_service_(other.discovery_service_), id_(other.id_) {
  other.discovery_service_ = nullptr;
}

CastAppDiscoveryService::Subscription&
CastAppDiscoveryService::Subscription::operator=(Subscription&& other) {
  id_ = other.id_;
  discovery_service_ = other.discovery_service_;
  other.discovery_service_ = nullptr;
  return *this;
}

CastAppDiscoveryService::Subscription::~Subscription() {
  Reset();
}

void CastAppDiscoveryService::Subscription::Reset() {
  if (discovery_service_) {
    discovery_service_->RemoveAvailabilityCallback(id_);
  }
  discovery_service_ = nullptr;
}

void CastAppDiscoveryService::Subscription::Swap(Subscription& other) {
  CastAppDiscoveryService* service = other.discovery_service_;
  other.discovery_service_ = discovery_service_;
  discovery_service_ = service;

  uint32_t id = other.id_;
  other.id_ = id_;
  id_ = id;
}

}  // namespace cast
}  // namespace openscreen
