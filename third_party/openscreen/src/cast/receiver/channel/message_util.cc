// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/receiver/channel/message_util.h"

#include "util/json/json_serialization.h"
#include "util/json/json_value.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {

using ::cast::channel::CastMessage;

namespace {

ErrorOr<CastMessage> CreateAppAvailabilityResponse(
    int request_id,
    const std::string& sender_id,
    const std::string& app_id,
    AppAvailabilityResult availability_result) {
  CastMessage availability_response;
  Json::Value dict(Json::ValueType::objectValue);
  dict[kMessageKeyRequestId] = request_id;
  Json::Value availability(Json::ValueType::objectValue);
  availability[app_id.c_str()] =
      availability_result == AppAvailabilityResult::kAvailable
          ? kMessageValueAppAvailable
          : kMessageValueAppUnavailable;
  dict[kMessageKeyAvailability] = std::move(availability);
  ErrorOr<std::string> serialized = json::Stringify(dict);
  if (!serialized) {
    return Error::Code::kJsonWriteError;
  }

  availability_response.set_source_id(kPlatformReceiverId);
  availability_response.set_destination_id(sender_id);
  availability_response.set_namespace_(kReceiverNamespace);
  availability_response.set_protocol_version(
      ::cast::channel::CastMessage_ProtocolVersion_CASTV2_1_0);
  availability_response.set_payload_utf8(std::move(serialized.value()));
  availability_response.set_payload_type(
      ::cast::channel::CastMessage_PayloadType_STRING);
  return availability_response;
}

}  // namespace

ErrorOr<CastMessage> CreateAppAvailableResponse(int request_id,
                                                const std::string& sender_id,
                                                const std::string& app_id) {
  return CreateAppAvailabilityResponse(request_id, sender_id, app_id,
                                       AppAvailabilityResult::kAvailable);
}

ErrorOr<CastMessage> CreateAppUnavailableResponse(int request_id,
                                                  const std::string& sender_id,
                                                  const std::string& app_id) {
  return CreateAppAvailabilityResponse(request_id, sender_id, app_id,
                                       AppAvailabilityResult::kUnavailable);
}

}  // namespace cast
}  // namespace openscreen
