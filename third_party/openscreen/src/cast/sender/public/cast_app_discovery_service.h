// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_SENDER_PUBLIC_CAST_APP_DISCOVERY_SERVICE_H_
#define CAST_SENDER_PUBLIC_CAST_APP_DISCOVERY_SERVICE_H_

#include <vector>

#include "cast/common/public/receiver_info.h"

namespace openscreen {
namespace cast {

class CastMediaSource;

// Interface for app discovery for Cast receivers.
class CastAppDiscoveryService {
 public:
  using AvailabilityCallback =
      std::function<void(const CastMediaSource& source,
                         const std::vector<ReceiverInfo>& receivers)>;

  class Subscription {
   public:
    Subscription(CastAppDiscoveryService* discovery_service, uint32_t id);
    Subscription(Subscription&&) noexcept;
    Subscription(Subscription&) = delete;
    Subscription& operator=(Subscription&&);
    Subscription& operator=(const Subscription&) = delete;
    ~Subscription();

    void Reset();

   private:
    friend class CastAppDiscoveryService;

    void Swap(Subscription& other);

    CastAppDiscoveryService* discovery_service_;
    uint32_t id_;
  };

  virtual ~CastAppDiscoveryService() = default;

  // Adds an availability query for |source|. Results will be continuously
  // returned via |callback| until the returned Subscription is destroyed by the
  // caller.  If there are cached results available, |callback| will be invoked
  // before this method returns.  |callback| may be invoked with an empty list
  // if all receivers respond to the respective queries with "unavailable" or
  // don't respond before a timeout.  |callback| may be invoked successively
  // with the same list.
  virtual Subscription StartObservingAvailability(
      const CastMediaSource& source,
      AvailabilityCallback callback) = 0;

  // Refreshes the state of app discovery in the service. It is suitable to call
  // this method when the user initiates a user gesture.
  virtual void Refresh() = 0;

 private:
  virtual void RemoveAvailabilityCallback(uint32_t id) = 0;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_SENDER_PUBLIC_CAST_APP_DISCOVERY_SERVICE_H_
