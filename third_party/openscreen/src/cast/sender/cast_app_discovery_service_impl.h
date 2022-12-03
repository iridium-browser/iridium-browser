// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_SENDER_CAST_APP_DISCOVERY_SERVICE_IMPL_H_
#define CAST_SENDER_CAST_APP_DISCOVERY_SERVICE_IMPL_H_

#include <map>
#include <string>
#include <vector>

#include "cast/common/public/receiver_info.h"
#include "cast/sender/cast_app_availability_tracker.h"
#include "cast/sender/cast_platform_client.h"
#include "cast/sender/public/cast_app_discovery_service.h"
#include "platform/api/time.h"
#include "util/weak_ptr.h"

namespace openscreen {
namespace cast {

// Keeps track of availability queries, receives receiver updates, and issues
// app availability requests based on these signals.
class CastAppDiscoveryServiceImpl : public CastAppDiscoveryService {
 public:
  // |platform_client| must outlive |this|.
  CastAppDiscoveryServiceImpl(CastPlatformClient* platform_client,
                              ClockNowFunctionPtr clock);
  ~CastAppDiscoveryServiceImpl() override;

  // CastAppDiscoveryService implementation.
  Subscription StartObservingAvailability(
      const CastMediaSource& source,
      AvailabilityCallback callback) override;

  // Reissues app availability requests for currently registered (receiver_id,
  // app_id) pairs whose status is kUnavailable or kUnknown.
  void Refresh() override;

  void AddOrUpdateReceiver(const ReceiverInfo& receiver);
  void RemoveReceiver(const ReceiverInfo& receiver);

 private:
  struct AvailabilityCallbackEntry {
    uint32_t id;
    AvailabilityCallback callback;
  };

  // Issues an app availability request for |app_id| to the receiver given by
  // |receiver_id|.
  void RequestAppAvailability(const std::string& receiver_id,
                              const std::string& app_id);

  // Updates the availability result for |receiver_id| and |app_id| with
  // |result|, and notifies callbacks with updated availability query results.
  void UpdateAppAvailability(const std::string& receiver_id,
                             const std::string& app_id,
                             AppAvailabilityResult result);

  // Updates the availability query results for |sources|.
  void UpdateAvailabilityQueries(const std::vector<CastMediaSource>& sources);

  std::vector<ReceiverInfo> GetReceiversByIds(
      const std::vector<std::string>& receiver_ids) const;

  // Returns true if an app availability request should be issued for
  // |receiver_id| and |app_id|. |now| is used for checking whether previously
  // cached results should be refreshed.
  bool ShouldRefreshAppAvailability(const std::string& receiver_id,
                                    const std::string& app_id,
                                    Clock::time_point now) const;

  void RemoveAvailabilityCallback(uint32_t id) override;

  std::map<std::string, ReceiverInfo> receivers_by_id_;

  // Registered availability queries and their associated callbacks keyed by
  // media source IDs.
  std::map<std::string, std::vector<AvailabilityCallbackEntry>> avail_queries_;

  // Callback ID tracking.
  uint32_t next_avail_query_id_ = 1U;

  CastPlatformClient* const platform_client_;

  CastAppAvailabilityTracker availability_tracker_;

  const ClockNowFunctionPtr clock_;

  WeakPtrFactory<CastAppDiscoveryServiceImpl> weak_factory_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_SENDER_CAST_APP_DISCOVERY_SERVICE_IMPL_H_
