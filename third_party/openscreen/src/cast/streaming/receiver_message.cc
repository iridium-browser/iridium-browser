// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/receiver_message.h"

#include <utility>

#include "absl/strings/ascii.h"
#include "absl/types/optional.h"
#include "cast/streaming/message_fields.h"
#include "json/reader.h"
#include "json/writer.h"
#include "platform/base/error.h"
#include "util/base64.h"
#include "util/enum_name_table.h"
#include "util/json/json_helpers.h"
#include "util/json/json_serialization.h"

namespace openscreen {
namespace cast {

namespace {

EnumNameTable<ReceiverMessage::Type, 5> kMessageTypeNames{
    {{kMessageTypeAnswer, ReceiverMessage::Type::kAnswer},
     {"CAPABILITIES_RESPONSE", ReceiverMessage::Type::kCapabilitiesResponse},
     {"RPC", ReceiverMessage::Type::kRpc}}};

EnumNameTable<MediaCapability, 10> kMediaCapabilityNames{
    {{"audio", MediaCapability::kAudio},
     {"aac", MediaCapability::kAac},
     {"opus", MediaCapability::kOpus},
     {"video", MediaCapability::kVideo},
     {"4k", MediaCapability::k4k},
     {"h264", MediaCapability::kH264},
     {"vp8", MediaCapability::kVp8},
     {"vp9", MediaCapability::kVp9},
     {"hevc", MediaCapability::kHevc},
     {"av1", MediaCapability::kAv1}}};

ReceiverMessage::Type GetMessageType(const Json::Value& root) {
  std::string type;
  if (!json::TryParseString(root[kMessageType], &type)) {
    return ReceiverMessage::Type::kUnknown;
  }

  absl::AsciiStrToUpper(&type);
  ErrorOr<ReceiverMessage::Type> parsed = GetEnum(kMessageTypeNames, type);

  return parsed.value(ReceiverMessage::Type::kUnknown);
}

bool TryParseCapability(const Json::Value& value, MediaCapability* out) {
  std::string c;
  if (!json::TryParseString(value, &c)) {
    return false;
  }

  const ErrorOr<MediaCapability> capability = GetEnum(kMediaCapabilityNames, c);
  if (capability.is_error()) {
    return false;
  }

  *out = capability.value();
  return true;
}

}  // namespace

// static
ErrorOr<ReceiverError> ReceiverError::Parse(const Json::Value& value) {
  if (!value) {
    return Error(Error::Code::kParameterInvalid,
                 "Empty JSON in receiver error parsing");
  }

  int code;
  std::string description;
  if (!json::TryParseInt(value[kErrorCode], &code) ||
      !json::TryParseString(value[kErrorDescription], &description)) {
    return Error::Code::kJsonParseError;
  }
  return ReceiverError{code, description};
}

Json::Value ReceiverError::ToJson() const {
  Json::Value root;
  root[kErrorCode] = code;
  root[kErrorDescription] = description;
  return root;
}

// static
ErrorOr<ReceiverCapability> ReceiverCapability::Parse(
    const Json::Value& value) {
  if (!value) {
    return Error(Error::Code::kParameterInvalid,
                 "Empty JSON in capabilities parsing");
  }

  int remoting_version;
  if (!json::TryParseInt(value["remoting"], &remoting_version)) {
    remoting_version = ReceiverCapability::kRemotingVersionUnknown;
  }

  std::vector<MediaCapability> capabilities;
  if (!json::TryParseArray<MediaCapability>(
          value["mediaCaps"], TryParseCapability, &capabilities)) {
    return Error(Error::Code::kJsonParseError,
                 "Failed to parse media capabilities");
  }

  return ReceiverCapability{remoting_version, std::move(capabilities)};
}

Json::Value ReceiverCapability::ToJson() const {
  Json::Value root;
  root["remoting"] = remoting_version;
  Json::Value capabilities(Json::ValueType::arrayValue);
  for (const auto& capability : media_capabilities) {
    capabilities.append(GetEnumName(kMediaCapabilityNames, capability).value());
  }
  root["mediaCaps"] = std::move(capabilities);
  return root;
}

// static
ErrorOr<ReceiverMessage> ReceiverMessage::Parse(const Json::Value& value) {
  ReceiverMessage message;
  if (!value) {
    return Error(Error::Code::kJsonParseError, "Invalid message body");
  }

  std::string result;
  if (!json::TryParseString(value[kResult], &result)) {
    result = kResultError;
  }

  message.type = GetMessageType(value);
  message.valid =
      (result == kResultOk || message.type == ReceiverMessage::Type::kRpc);
  if (!message.valid) {
    ErrorOr<ReceiverError> error =
        ReceiverError::Parse(value[kErrorMessageBody]);
    if (error.is_value()) {
      message.body = std::move(error.value());
    }
    return message;
  }

  switch (message.type) {
    case Type::kAnswer: {
      Answer answer;
      if (openscreen::cast::Answer::TryParse(value[kAnswerMessageBody],
                                             &answer)) {
        message.body = std::move(answer);
        message.valid = true;
      }
    } break;

    case Type::kCapabilitiesResponse: {
      ErrorOr<ReceiverCapability> capability =
          ReceiverCapability::Parse(value[kCapabilitiesMessageBody]);
      if (capability.is_value()) {
        message.body = std::move(capability.value());
        message.valid = true;
      }
    } break;

    case Type::kRpc: {
      std::string encoded_rpc;
      std::vector<uint8_t> rpc;
      if (json::TryParseString(value[kRpcMessageBody], &encoded_rpc) &&
          base64::Decode(encoded_rpc, &rpc)) {
        message.body = std::move(rpc);
        message.valid = true;
      }
    } break;

    default:
      break;
  }

  if (message.type != ReceiverMessage::Type::kRpc &&
      !json::TryParseInt(value[kSequenceNumber], &(message.sequence_number))) {
    message.sequence_number = -1;
    message.valid = false;
  }

  return message;
}

ErrorOr<Json::Value> ReceiverMessage::ToJson() const {
  OSP_CHECK(type != ReceiverMessage::Type::kUnknown)
      << "Trying to send an unknown message is a developer error";

  Json::Value root;
  root[kMessageType] = GetEnumName(kMessageTypeNames, type).value();
  if (sequence_number >= 0) {
    root[kSequenceNumber] = sequence_number;
  }

  switch (type) {
    case ReceiverMessage::Type::kAnswer:
      if (valid) {
        root[kResult] = kResultOk;
        root[kAnswerMessageBody] = absl::get<Answer>(body).ToJson();
      } else {
        root[kResult] = kResultError;
        root[kErrorMessageBody] = absl::get<ReceiverError>(body).ToJson();
      }
      break;

    case ReceiverMessage::Type::kCapabilitiesResponse:
      if (valid) {
        root[kResult] = kResultOk;
        root[kCapabilitiesMessageBody] =
            absl::get<ReceiverCapability>(body).ToJson();
      } else {
        root[kResult] = kResultError;
        root[kErrorMessageBody] = absl::get<ReceiverError>(body).ToJson();
      }
      break;

    // NOTE: RPC messages do NOT have a result field.
    case ReceiverMessage::Type::kRpc:
      root[kRpcMessageBody] =
          base64::Encode(absl::get<std::vector<uint8_t>>(body));
      break;

    default:
      OSP_NOTREACHED();
  }

  return root;
}

}  // namespace cast
}  // namespace openscreen
