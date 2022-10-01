// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/sender/cast_app_discovery_service_impl.h"

#include <algorithm>
#include <chrono>
#include <utility>

#include "cast/sender/public/cast_media_source.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {
namespace {

// The minimum time that must elapse before an app availability result can be
// force refreshed.
static constexpr std::chrono::minutes kRefreshThreshold =
    std::chrono::minutes(1);

}  // namespace

CastAppDiscoveryServiceImpl::CastAppDiscoveryServiceImpl(
    CastPlatformClient* platform_client,
    ClockNowFunctionPtr clock)
    : platform_client_(platform_client), clock_(clock), weak_factory_(this) {
  OSP_DCHECK(platform_client_);
  OSP_DCHECK(clock_);
}

CastAppDiscoveryServiceImpl::~CastAppDiscoveryServiceImpl() {
  OSP_CHECK_EQ(avail_queries_.size(), 0u);
}

CastAppDiscoveryService::Subscription
CastAppDiscoveryServiceImpl::StartObservingAvailability(
    const CastMediaSource& source,
    AvailabilityCallback callback) {
  const std::string& source_id = source.source_id();

  // Return cached results immediately, if available.
  std::vector<std::string> cached_receiver_ids =
      availability_tracker_.GetAvailableReceivers(source);
  if (!cached_receiver_ids.empty()) {
    callback(source, GetReceiversByIds(cached_receiver_ids));
  }

  auto& callbacks = avail_queries_[source_id];
  uint32_t query_id = next_avail_query_id_++;
  callbacks.push_back({query_id, std::move(callback)});
  if (callbacks.size() == 1) {
    // NOTE: Even though we retain availability results for an app unregistered
    // from the tracker, we will refresh the results when the app is
    // re-registered.
    std::vector<std::string> new_app_ids =
        availability_tracker_.RegisterSource(source);
    for (const auto& app_id : new_app_ids) {
      for (const auto& entry : receivers_by_id_) {
        RequestAppAvailability(entry.first, app_id);
      }
    }
  }

  return Subscription(this, query_id);
}

void CastAppDiscoveryServiceImpl::Refresh() {
  const auto app_ids = availability_tracker_.GetRegisteredApps();
  for (const auto& entry : receivers_by_id_) {
    for (const auto& app_id : app_ids) {
      RequestAppAvailability(entry.first, app_id);
    }
  }
}

void CastAppDiscoveryServiceImpl::AddOrUpdateReceiver(
    const ReceiverInfo& receiver) {
  const std::string& receiver_id = receiver.unique_id;
  receivers_by_id_[receiver_id] = receiver;

  // Any queries that currently contain this receiver should be updated.
  UpdateAvailabilityQueries(
      availability_tracker_.GetSupportedSources(receiver_id));

  for (const std::string& app_id : availability_tracker_.GetRegisteredApps()) {
    RequestAppAvailability(receiver_id, app_id);
  }
}

void CastAppDiscoveryServiceImpl::RemoveReceiver(const ReceiverInfo& receiver) {
  const std::string& receiver_id = receiver.unique_id;
  receivers_by_id_.erase(receiver_id);
  UpdateAvailabilityQueries(
      availability_tracker_.RemoveResultsForReceiver(receiver_id));
}

void CastAppDiscoveryServiceImpl::RequestAppAvailability(
    const std::string& receiver_id,
    const std::string& app_id) {
  if (ShouldRefreshAppAvailability(receiver_id, app_id, clock_())) {
    platform_client_->RequestAppAvailability(
        receiver_id, app_id,
        [self = weak_factory_.GetWeakPtr(), receiver_id](
            const std::string& id, AppAvailabilityResult availability) {
          if (self) {
            self->UpdateAppAvailability(receiver_id, id, availability);
          }
        });
  }
}

void CastAppDiscoveryServiceImpl::UpdateAppAvailability(
    const std::string& receiver_id,
    const std::string& app_id,
    AppAvailabilityResult availability) {
  if (receivers_by_id_.find(receiver_id) == receivers_by_id_.end()) {
    return;
  }

  OSP_DVLOG << "App " << app_id << " on receiver " << receiver_id << " is "
            << ToString(availability);

  UpdateAvailabilityQueries(availability_tracker_.UpdateAppAvailability(
      receiver_id, app_id, {availability, clock_()}));
}

void CastAppDiscoveryServiceImpl::UpdateAvailabilityQueries(
    const std::vector<CastMediaSource>& sources) {
  for (const auto& source : sources) {
    const std::string& source_id = source.source_id();
    auto it = avail_queries_.find(source_id);
    if (it == avail_queries_.end())
      continue;
    std::vector<std::string> receiver_ids =
        availability_tracker_.GetAvailableReceivers(source);
    std::vector<ReceiverInfo> receivers = GetReceiversByIds(receiver_ids);
    for (const auto& callback : it->second) {
      callback.callback(source, receivers);
    }
  }
}

std::vector<ReceiverInfo> CastAppDiscoveryServiceImpl::GetReceiversByIds(
    const std::vector<std::string>& receiver_ids) const {
  std::vector<ReceiverInfo> receivers;
  for (const std::string& receiver_id : receiver_ids) {
    auto entry = receivers_by_id_.find(receiver_id);
    if (entry != receivers_by_id_.end()) {
      receivers.push_back(entry->second);
    }
  }
  return receivers;
}

bool CastAppDiscoveryServiceImpl::ShouldRefreshAppAvailability(
    const std::string& receiver_id,
    const std::string& app_id,
    Clock::time_point now) const {
  // TODO(btolsch): Consider an exponential backoff mechanism instead.
  // Receivers will typically respond with "unavailable" immediately after boot
  // and then become available 10-30 seconds later.
  auto availability =
      availability_tracker_.GetAvailability(receiver_id, app_id);
  switch (availability.availability) {
    case AppAvailabilityResult::kAvailable:
      return false;
    case AppAvailabilityResult::kUnavailable:
      return (now - availability.time) > kRefreshThreshold;
    // TODO(btolsch): Should there be a background task for periodically
    // refreshing kUnknown (or even kUnavailable) results?
    case AppAvailabilityResult::kUnknown:
      return true;
  }

  OSP_NOTREACHED();
}

void CastAppDiscoveryServiceImpl::RemoveAvailabilityCallback(uint32_t id) {
  for (auto entry = avail_queries_.begin(); entry != avail_queries_.end();
       ++entry) {
    const std::string& source_id = entry->first;
    auto& callbacks = entry->second;
    auto it =
        std::find_if(callbacks.begin(), callbacks.end(),
                     [id](const AvailabilityCallbackEntry& callback_entry) {
                       return callback_entry.id == id;
                     });
    if (it != callbacks.end()) {
      callbacks.erase(it);
      if (callbacks.empty()) {
        availability_tracker_.UnregisterSource(source_id);
        avail_queries_.erase(entry);
      }
      return;
    }
  }
}

}  // namespace cast
}  // namespace openscreen
