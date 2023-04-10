// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/sender/channel/message_util.h"

#include "cast/sender/channel/cast_auth_util.h"
#include "util/json/json_serialization.h"

namespace openscreen {
namespace cast {

using ::cast::channel::AuthChallenge;
using ::cast::channel::CastMessage;
using ::cast::channel::DeviceAuthMessage;

CastMessage CreateAuthChallengeMessage(const AuthContext& auth_context) {
  CastMessage message;
  DeviceAuthMessage auth_message;

  AuthChallenge* challenge = auth_message.mutable_challenge();
  challenge->set_sender_nonce(auth_context.nonce());
  challenge->set_hash_algorithm(::cast::channel::SHA256);

  std::string auth_message_string;
  auth_message.SerializeToString(&auth_message_string);

  message.set_protocol_version(CastMessage::CASTV2_1_0);
  message.set_source_id(kPlatformSenderId);
  message.set_destination_id(kPlatformReceiverId);
  message.set_namespace_(kAuthNamespace);
  message.set_payload_type(::cast::channel::CastMessage_PayloadType_BINARY);
  message.set_payload_binary(auth_message_string);

  return message;
}

ErrorOr<CastMessage> CreateAppAvailabilityRequest(const std::string& sender_id,
                                                  int request_id,
                                                  const std::string& app_id) {
  Json::Value dict(Json::ValueType::objectValue);
  dict[kMessageKeyType] = Json::Value(
      CastMessageTypeToString(CastMessageType::kGetAppAvailability));
  Json::Value app_id_value(Json::ValueType::arrayValue);
  app_id_value.append(Json::Value(app_id));
  dict[kMessageKeyAppId] = std::move(app_id_value);
  dict[kMessageKeyRequestId] = Json::Value(request_id);

  CastMessage message;
  message.set_payload_type(::cast::channel::CastMessage_PayloadType_STRING);
  ErrorOr<std::string> serialized = json::Stringify(dict);
  if (serialized.is_error()) {
    return serialized.error();
  }
  message.set_payload_utf8(serialized.value());

  message.set_protocol_version(
      ::cast::channel::CastMessage_ProtocolVersion_CASTV2_1_0);
  message.set_source_id(sender_id);
  message.set_destination_id(kPlatformReceiverId);
  message.set_namespace_(kReceiverNamespace);

  return message;
}

}  // namespace cast
}  // namespace openscreen
