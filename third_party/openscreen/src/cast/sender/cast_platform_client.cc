// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/sender/cast_platform_client.h"

#include <memory>
#include <random>
#include <utility>

#include "absl/strings/str_cat.h"
#include "cast/common/channel/virtual_connection_router.h"
#include "cast/common/public/cast_socket.h"
#include "cast/common/public/receiver_info.h"
#include "util/json/json_serialization.h"
#include "util/osp_logging.h"
#include "util/stringprintf.h"

namespace openscreen {
namespace cast {

static constexpr std::chrono::seconds kRequestTimeout = std::chrono::seconds(5);

CastPlatformClient::CastPlatformClient(VirtualConnectionRouter* router,
                                       ClockNowFunctionPtr clock,
                                       TaskRunner* task_runner)
    : sender_id_(MakeUniqueSessionId("sender")),
      virtual_conn_router_(router),
      clock_(clock),
      task_runner_(task_runner) {
  OSP_DCHECK(virtual_conn_router_);
  OSP_DCHECK(clock_);
  OSP_DCHECK(task_runner_);
  virtual_conn_router_->AddHandlerForLocalId(sender_id_, this);
}

CastPlatformClient::~CastPlatformClient() {
  virtual_conn_router_->RemoveConnectionsByLocalId(sender_id_);
  virtual_conn_router_->RemoveHandlerForLocalId(sender_id_);

  for (auto& pending_requests : pending_requests_by_receiver_id_) {
    for (auto& avail_request : pending_requests.second.availability) {
      avail_request.callback(avail_request.app_id,
                             AppAvailabilityResult::kUnknown);
    }
  }
}

absl::optional<int> CastPlatformClient::RequestAppAvailability(
    const std::string& receiver_id,
    const std::string& app_id,
    AppAvailabilityCallback callback) {
  auto entry = socket_id_by_receiver_id_.find(receiver_id);
  if (entry == socket_id_by_receiver_id_.end()) {
    callback(app_id, AppAvailabilityResult::kUnknown);
    return absl::nullopt;
  }
  int socket_id = entry->second;

  int request_id = GetNextRequestId();
  ErrorOr<::cast::channel::CastMessage> message =
      CreateAppAvailabilityRequest(sender_id_, request_id, app_id);
  OSP_DCHECK(message);

  PendingRequests& pending_requests =
      pending_requests_by_receiver_id_[receiver_id];
  auto timeout = std::make_unique<Alarm>(clock_, task_runner_);
  timeout->ScheduleFromNow(
      [this, request_id]() { CancelAppAvailabilityRequest(request_id); },
      kRequestTimeout);
  pending_requests.availability.push_back(AvailabilityRequest{
      request_id, app_id, std::move(timeout), std::move(callback)});

  VirtualConnection virtual_conn{sender_id_, kPlatformReceiverId, socket_id};
  if (!virtual_conn_router_->GetConnectionData(virtual_conn)) {
    virtual_conn_router_->AddConnection(virtual_conn,
                                        VirtualConnection::AssociatedData{});
  }

  virtual_conn_router_->Send(std::move(virtual_conn),
                             std::move(message.value()));

  return request_id;
}

void CastPlatformClient::AddOrUpdateReceiver(const ReceiverInfo& receiver,
                                             int socket_id) {
  socket_id_by_receiver_id_[receiver.unique_id] = socket_id;
}

void CastPlatformClient::RemoveReceiver(const ReceiverInfo& receiver) {
  auto pending_requests_it =
      pending_requests_by_receiver_id_.find(receiver.unique_id);
  if (pending_requests_it != pending_requests_by_receiver_id_.end()) {
    for (const AvailabilityRequest& availability :
         pending_requests_it->second.availability) {
      availability.callback(availability.app_id,
                            AppAvailabilityResult::kUnknown);
    }
    pending_requests_by_receiver_id_.erase(pending_requests_it);
  }
  socket_id_by_receiver_id_.erase(receiver.unique_id);
}

void CastPlatformClient::CancelRequest(int request_id) {
  for (auto entry = pending_requests_by_receiver_id_.begin();
       entry != pending_requests_by_receiver_id_.end(); ++entry) {
    auto& pending_requests = entry->second;
    auto it = std::find_if(pending_requests.availability.begin(),
                           pending_requests.availability.end(),
                           [request_id](const AvailabilityRequest& request) {
                             return request.request_id == request_id;
                           });
    if (it != pending_requests.availability.end()) {
      pending_requests.availability.erase(it);
      break;
    }
  }
}

void CastPlatformClient::OnMessage(VirtualConnectionRouter* router,
                                   CastSocket* socket,
                                   ::cast::channel::CastMessage message) {
  if (message.payload_type() !=
          ::cast::channel::CastMessage_PayloadType_STRING ||
      message.namespace_() != kReceiverNamespace ||
      message.source_id() != kPlatformReceiverId) {
    return;
  }
  ErrorOr<Json::Value> dict_or_error = json::Parse(message.payload_utf8());
  if (dict_or_error.is_error()) {
    return;
  }

  Json::Value& dict = dict_or_error.value();
  absl::optional<int> request_id =
      MaybeGetInt(dict, JSON_EXPAND_FIND_CONSTANT_ARGS(kMessageKeyRequestId));
  if (request_id) {
    auto socket_map_entry = std::find_if(
        socket_id_by_receiver_id_.begin(), socket_id_by_receiver_id_.end(),
        [socket_id =
             ToCastSocketId(socket)](const std::pair<std::string, int>& entry) {
          return entry.second == socket_id;
        });
    if (socket_map_entry != socket_id_by_receiver_id_.end()) {
      HandleResponse(socket_map_entry->first, request_id.value(), dict);
    }
  }
}

void CastPlatformClient::HandleResponse(const std::string& receiver_id,
                                        int request_id,
                                        const Json::Value& message) {
  auto entry = pending_requests_by_receiver_id_.find(receiver_id);
  if (entry == pending_requests_by_receiver_id_.end()) {
    return;
  }
  PendingRequests& pending_requests = entry->second;
  auto it = std::find_if(pending_requests.availability.begin(),
                         pending_requests.availability.end(),
                         [request_id](const AvailabilityRequest& request) {
                           return request.request_id == request_id;
                         });
  if (it != pending_requests.availability.end()) {
    // TODO(btolsch): Can all of this manual parsing/checking be cleaned up into
    // a single parsing API along with other message handling?
    const Json::Value* maybe_availability =
        message.find(JSON_EXPAND_FIND_CONSTANT_ARGS(kMessageKeyAvailability));
    if (maybe_availability && maybe_availability->isObject()) {
      absl::optional<absl::string_view> result =
          MaybeGetString(*maybe_availability, &it->app_id[0],
                         &it->app_id[0] + it->app_id.size());
      if (result) {
        AppAvailabilityResult availability_result =
            AppAvailabilityResult::kUnknown;
        if (result.value() == kMessageValueAppAvailable) {
          availability_result = AppAvailabilityResult::kAvailable;
        } else if (result.value() == kMessageValueAppUnavailable) {
          availability_result = AppAvailabilityResult::kUnavailable;
        } else {
          OSP_VLOG << "Invalid availability result: " << result.value();
        }
        it->callback(it->app_id, availability_result);
      }
    }
    pending_requests.availability.erase(it);
  }
}

void CastPlatformClient::CancelAppAvailabilityRequest(int request_id) {
  for (auto& entry : pending_requests_by_receiver_id_) {
    PendingRequests& pending_requests = entry.second;
    auto it = std::find_if(pending_requests.availability.begin(),
                           pending_requests.availability.end(),
                           [request_id](const AvailabilityRequest& request) {
                             return request.request_id == request_id;
                           });
    if (it != pending_requests.availability.end()) {
      it->callback(it->app_id, AppAvailabilityResult::kUnknown);
      pending_requests.availability.erase(it);
    }
  }
}

// static
int CastPlatformClient::GetNextRequestId() {
  return next_request_id_++;
}

// static
int CastPlatformClient::next_request_id_ = 0;

}  // namespace cast
}  // namespace openscreen
