// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/sender/testing/test_helpers.h"

#include "cast/common/channel/message_util.h"
#include "cast/receiver/channel/message_util.h"
#include "cast/sender/channel/message_util.h"
#include "gtest/gtest.h"
#include "util/json/json_serialization.h"
#include "util/json/json_value.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {

using ::cast::channel::CastMessage;

void VerifyAppAvailabilityRequest(const CastMessage& message,
                                  const std::string& expected_app_id,
                                  int* request_id_out,
                                  std::string* sender_id_out) {
  std::string app_id_out;
  VerifyAppAvailabilityRequest(message, &app_id_out, request_id_out,
                               sender_id_out);
  EXPECT_EQ(app_id_out, expected_app_id);
}

void VerifyAppAvailabilityRequest(const CastMessage& message,
                                  std::string* app_id_out,
                                  int* request_id_out,
                                  std::string* sender_id_out) {
  EXPECT_EQ(message.namespace_(), kReceiverNamespace);
  EXPECT_EQ(message.destination_id(), kPlatformReceiverId);
  EXPECT_EQ(message.payload_type(),
            ::cast::channel::CastMessage_PayloadType_STRING);
  EXPECT_NE(message.source_id(), kPlatformSenderId);
  *sender_id_out = message.source_id();

  ErrorOr<Json::Value> maybe_value = json::Parse(message.payload_utf8());
  ASSERT_TRUE(maybe_value);
  Json::Value& value = maybe_value.value();

  absl::optional<absl::string_view> maybe_type =
      MaybeGetString(value, JSON_EXPAND_FIND_CONSTANT_ARGS(kMessageKeyType));
  ASSERT_TRUE(maybe_type);
  EXPECT_EQ(maybe_type.value(),
            CastMessageTypeToString(CastMessageType::kGetAppAvailability));

  absl::optional<int> maybe_id =
      MaybeGetInt(value, JSON_EXPAND_FIND_CONSTANT_ARGS(kMessageKeyRequestId));
  ASSERT_TRUE(maybe_id);
  *request_id_out = maybe_id.value();

  const Json::Value* maybe_app_ids =
      value.find(JSON_EXPAND_FIND_CONSTANT_ARGS(kMessageKeyAppId));
  ASSERT_TRUE(maybe_app_ids);
  ASSERT_TRUE(maybe_app_ids->isArray());
  ASSERT_EQ(maybe_app_ids->size(), 1u);
  Json::Value app_id_value = maybe_app_ids->get(0u, Json::Value(""));
  absl::optional<absl::string_view> maybe_app_id = MaybeGetString(app_id_value);
  ASSERT_TRUE(maybe_app_id);
  *app_id_out =
      std::string(maybe_app_id.value().begin(), maybe_app_id.value().end());
}

CastMessage CreateAppAvailableResponseChecked(int request_id,
                                              const std::string& sender_id,
                                              const std::string& app_id) {
  ErrorOr<CastMessage> message =
      CreateAppAvailableResponse(request_id, sender_id, app_id);
  OSP_CHECK(message);
  return std::move(message.value());
}

CastMessage CreateAppUnavailableResponseChecked(int request_id,
                                                const std::string& sender_id,
                                                const std::string& app_id) {
  ErrorOr<CastMessage> message =
      CreateAppUnavailableResponse(request_id, sender_id, app_id);
  OSP_CHECK(message);
  return std::move(message.value());
}

}  // namespace cast
}  // namespace openscreen
