// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_SENDER_CAST_PLATFORM_CLIENT_H_
#define CAST_SENDER_CAST_PLATFORM_CLIENT_H_

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "absl/types/optional.h"
#include "cast/common/channel/cast_message_handler.h"
#include "cast/sender/channel/message_util.h"
#include "util/alarm.h"
#include "util/json/json_value.h"

namespace openscreen {
namespace cast {

struct ReceiverInfo;
class VirtualConnectionRouter;

// This class handles Cast messages that generally relate to the "platform", in
// other words not a specific app currently running (e.g. app availability,
// receiver status).  These messages follow a request/response format, so each
// request requires a corresponding response callback.  These requests will also
// timeout if there is no response after a certain amount of time (currently 5
// seconds).  The timeout callbacks will be called on the thread managed by
// |task_runner|.
class CastPlatformClient final : public CastMessageHandler {
 public:
  using AppAvailabilityCallback =
      std::function<void(const std::string& app_id, AppAvailabilityResult)>;

  CastPlatformClient(VirtualConnectionRouter* router,
                     ClockNowFunctionPtr clock,
                     TaskRunner* task_runner);
  ~CastPlatformClient() override;

  // Requests availability information for |app_id| from the receiver identified
  // by |receiver_id|.  |callback| will be called exactly once with a result.
  absl::optional<int> RequestAppAvailability(const std::string& receiver_id,
                                             const std::string& app_id,
                                             AppAvailabilityCallback callback);

  // Notifies this object about general receiver connectivity or property
  // changes.
  void AddOrUpdateReceiver(const ReceiverInfo& receiver, int socket_id);
  void RemoveReceiver(const ReceiverInfo& receiver);

  void CancelRequest(int request_id);

 private:
  struct AvailabilityRequest {
    int request_id;
    std::string app_id;
    std::unique_ptr<Alarm> timeout;
    AppAvailabilityCallback callback;
  };

  struct PendingRequests {
    std::vector<AvailabilityRequest> availability;
  };

  // CastMessageHandler overrides.
  void OnMessage(VirtualConnectionRouter* router,
                 CastSocket* socket,
                 ::cast::channel::CastMessage message) override;

  void HandleResponse(const std::string& receiver_id,
                      int request_id,
                      const Json::Value& message);

  void CancelAppAvailabilityRequest(int request_id);

  static int GetNextRequestId();

  static int next_request_id_;

  const std::string sender_id_;
  VirtualConnectionRouter* const virtual_conn_router_;
  std::map<std::string /* receiver_id */, int> socket_id_by_receiver_id_;
  std::map<std::string /* receiver_id */, PendingRequests>
      pending_requests_by_receiver_id_;

  const ClockNowFunctionPtr clock_;
  TaskRunner* const task_runner_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_SENDER_CAST_PLATFORM_CLIENT_H_
