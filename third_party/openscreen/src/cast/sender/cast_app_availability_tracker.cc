// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/sender/cast_app_availability_tracker.h"

#include "util/osp_logging.h"

namespace openscreen {
namespace cast {

CastAppAvailabilityTracker::CastAppAvailabilityTracker() = default;
CastAppAvailabilityTracker::~CastAppAvailabilityTracker() = default;

std::vector<std::string> CastAppAvailabilityTracker::RegisterSource(
    const CastMediaSource& source) {
  if (registered_sources_.find(source.source_id()) !=
      registered_sources_.end()) {
    return {};
  }

  registered_sources_.emplace(source.source_id(), source);

  std::vector<std::string> new_app_ids;
  for (const std::string& app_id : source.app_ids()) {
    if (++registration_count_by_app_id_[app_id] == 1) {
      new_app_ids.push_back(app_id);
    }
  }
  return new_app_ids;
}

void CastAppAvailabilityTracker::UnregisterSource(
    const CastMediaSource& source) {
  UnregisterSource(source.source_id());
}

void CastAppAvailabilityTracker::UnregisterSource(
    const std::string& source_id) {
  auto it = registered_sources_.find(source_id);
  if (it == registered_sources_.end()) {
    return;
  }

  for (const std::string& app_id : it->second.app_ids()) {
    auto count_it = registration_count_by_app_id_.find(app_id);
    OSP_DCHECK(count_it != registration_count_by_app_id_.end());
    if (--(count_it->second) == 0) {
      registration_count_by_app_id_.erase(count_it);
    }
  }

  registered_sources_.erase(it);
}

std::vector<CastMediaSource> CastAppAvailabilityTracker::UpdateAppAvailability(
    const std::string& receiver_id,
    const std::string& app_id,
    AppAvailability availability) {
  auto& availabilities = app_availabilities_[receiver_id];
  auto it = availabilities.find(app_id);

  AppAvailabilityResult old_availability = it == availabilities.end()
                                               ? AppAvailabilityResult::kUnknown
                                               : it->second.availability;
  AppAvailabilityResult new_availability = availability.availability;

  // Updated if status changes from/to kAvailable.
  bool updated = (old_availability == AppAvailabilityResult::kAvailable ||
                  new_availability == AppAvailabilityResult::kAvailable) &&
                 old_availability != new_availability;
  availabilities[app_id] = availability;

  if (!updated) {
    return {};
  }

  std::vector<CastMediaSource> affected_sources;
  for (const auto& source : registered_sources_) {
    if (source.second.ContainsAppId(app_id)) {
      affected_sources.push_back(source.second);
    }
  }
  return affected_sources;
}

std::vector<CastMediaSource>
CastAppAvailabilityTracker::RemoveResultsForReceiver(
    const std::string& receiver_id) {
  auto affected_sources = GetSupportedSources(receiver_id);
  app_availabilities_.erase(receiver_id);
  return affected_sources;
}

std::vector<CastMediaSource> CastAppAvailabilityTracker::GetSupportedSources(
    const std::string& receiver_id) const {
  auto it = app_availabilities_.find(receiver_id);
  if (it == app_availabilities_.end()) {
    return std::vector<CastMediaSource>();
  }

  // Find all app IDs that are available on the receiver.
  std::vector<std::string> supported_app_ids;
  for (const auto& availability : it->second) {
    if (availability.second.availability == AppAvailabilityResult::kAvailable) {
      supported_app_ids.push_back(availability.first);
    }
  }

  // Find all registered sources whose query results contain the receiver ID.
  std::vector<CastMediaSource> sources;
  for (const auto& source : registered_sources_) {
    if (source.second.ContainsAnyAppIdFrom(supported_app_ids)) {
      sources.push_back(source.second);
    }
  }
  return sources;
}

CastAppAvailabilityTracker::AppAvailability
CastAppAvailabilityTracker::GetAvailability(const std::string& receiver_id,
                                            const std::string& app_id) const {
  auto availabilities_it = app_availabilities_.find(receiver_id);
  if (availabilities_it == app_availabilities_.end()) {
    return {AppAvailabilityResult::kUnknown, Clock::time_point{}};
  }

  const auto& availability_map = availabilities_it->second;
  auto availability_it = availability_map.find(app_id);
  if (availability_it == availability_map.end()) {
    return {AppAvailabilityResult::kUnknown, Clock::time_point{}};
  }

  return availability_it->second;
}

std::vector<std::string> CastAppAvailabilityTracker::GetRegisteredApps() const {
  std::vector<std::string> registered_apps;
  for (const auto& app_ids_and_count : registration_count_by_app_id_) {
    registered_apps.push_back(app_ids_and_count.first);
  }

  return registered_apps;
}

std::vector<std::string> CastAppAvailabilityTracker::GetAvailableReceivers(
    const CastMediaSource& source) const {
  std::vector<std::string> receiver_ids;
  // For each receiver, check if there is at least one available app in
  // |source|.
  for (const auto& availabilities : app_availabilities_) {
    for (const std::string& app_id : source.app_ids()) {
      const auto& availabilities_map = availabilities.second;
      auto availability_it = availabilities_map.find(app_id);
      if (availability_it != availabilities_map.end() &&
          availability_it->second.availability ==
              AppAvailabilityResult::kAvailable) {
        receiver_ids.push_back(availabilities.first);
        break;
      }
    }
  }
  return receiver_ids;
}

}  // namespace cast
}  // namespace openscreen
