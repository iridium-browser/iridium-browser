// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/protocol/castv2/validation.h"

#include <numeric>
#include <string>

#include "absl/strings/string_view.h"
#include "cast/protocol/castv2/receiver_examples/get_app_availability_data.h"
#include "cast/protocol/castv2/receiver_examples/get_app_availability_response_data.h"
#include "cast/protocol/castv2/receiver_examples/launch_data.h"
#include "cast/protocol/castv2/receiver_examples/stop_data.h"
#include "cast/protocol/castv2/receiver_schema_data.h"
#include "cast/protocol/castv2/streaming_examples/answer_data.h"
#include "cast/protocol/castv2/streaming_examples/capabilities_response_data.h"
#include "cast/protocol/castv2/streaming_examples/get_capabilities_data.h"
#include "cast/protocol/castv2/streaming_examples/get_status_data.h"
#include "cast/protocol/castv2/streaming_examples/offer_data.h"
#include "cast/protocol/castv2/streaming_examples/rpc_data.h"
#include "cast/protocol/castv2/streaming_examples/status_response_data.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "json/value.h"
#include "platform/base/error.h"
#include "util/json/json_serialization.h"
#include "util/osp_logging.h"
#include "util/std_util.h"
#include "util/stringprintf.h"

namespace openscreen {
namespace cast {

namespace {

constexpr char kEmptyJson[] = "{}";

// Schema format string, that allows for specifying definitions,
// properties, and required fields.
constexpr char kSchemaFormat[] = R"({
  "$schema": "http://json-schema.org/draft-07/schema#",
  "$id": "https://something/app_schema_data.h",
  "definitions": {
    %s
  },
  "type": "object",
  "properties": {
    %s
  },
  "required": [%s]
})";

// Fields used for an appId containing schema
constexpr char kAppIdDefinition[] = R"("app_id": {
    "type": "string",
    "enum": ["0F5096E8", "85CDB22F"]
  })";
constexpr char kAppIdName[] = "\"appId\"";
constexpr char kAppIdProperty[] =
    R"(  "appId": {"$ref": "#/definitions/app_id"})";

// Teest documents containing an appId.
constexpr char kValidAppIdDocument[] = R"({ "appId": "0F5096E8" })";
constexpr char kInvalidAppIdDocument[] = R"({ "appId": "FooBar" })";

std::string BuildSchema(const char* definitions,
                        const char* properties,
                        const char* required) {
  return StringPrintf(kSchemaFormat, definitions, properties, required);
}

bool TestValidate(absl::string_view document, absl::string_view schema) {
  ErrorOr<Json::Value> document_root = json::Parse(document);
  EXPECT_TRUE(document_root.is_value());
  ErrorOr<Json::Value> schema_root = json::Parse(schema);
  EXPECT_TRUE(schema_root.is_value());

  std::vector<Error> errors =
      Validate(document_root.value(), schema_root.value());
  return errors.empty();
}

const std::string& GetEmptySchema() {
  static const std::string kEmptySchema = BuildSchema("", "", "");
  return kEmptySchema;
}

const std::string& GetAppSchema() {
  static const std::string kAppIdSchema =
      BuildSchema(kAppIdDefinition, kAppIdProperty, kAppIdName);
  return kAppIdSchema;
}

class StreamingValidationTest : public testing::TestWithParam<const char*> {};
class ReceiverValidationTest : public testing::TestWithParam<const char*> {};

}  // namespace

TEST(ValidationTest, EmptyPassesEmpty) {
  EXPECT_TRUE(TestValidate(kEmptyJson, kEmptyJson));
}

TEST(ValidationTest, EmptyPassesBasicSchema) {
  EXPECT_TRUE(TestValidate(kEmptyJson, GetEmptySchema()));
}

TEST(ValidationTest, EmptyFailsAppIdSchema) {
  EXPECT_FALSE(TestValidate(kEmptyJson, GetAppSchema()));
}

TEST(ValidationTest, InvalidAppIdFailsAppIdSchema) {
  EXPECT_FALSE(TestValidate(kInvalidAppIdDocument, GetAppSchema()));
}

TEST(ValidationTest, ValidAppIdPassesAppIdSchema) {
  EXPECT_TRUE(TestValidate(kValidAppIdDocument, GetAppSchema()));
}

TEST(ValidationTest, InvalidAppIdPassesEmptySchema) {
  EXPECT_TRUE(TestValidate(kInvalidAppIdDocument, GetEmptySchema()));
}

TEST(ValidationTest, ValidAppIdPassesEmptySchema) {
  EXPECT_TRUE(TestValidate(kValidAppIdDocument, GetEmptySchema()));
}

INSTANTIATE_TEST_SUITE_P(StreamingValidations,
                         StreamingValidationTest,
                         testing::Values(kAnswer,
                                         kCapabilitiesResponse,
                                         kGetCapabilities,
                                         kGetStatus,
                                         kOffer,
                                         kRpc,
                                         kStatusResponse));

TEST_P(StreamingValidationTest, ExampleStreamingMessages) {
  ErrorOr<Json::Value> message_root = json::Parse(GetParam());
  EXPECT_TRUE(message_root.is_value());
  EXPECT_TRUE(ValidateStreamingMessage(message_root.value()).empty());
}

void ExpectReceiverMessageValid(const char* message) {}

INSTANTIATE_TEST_SUITE_P(ReceiverValidations,
                         ReceiverValidationTest,
                         testing::Values(kGetAppAvailability,
                                         kGetAppAvailabilityResponse,
                                         kLaunch,
                                         kStop));

TEST_P(ReceiverValidationTest, ExampleReceiverMessages) {
  ErrorOr<Json::Value> message_root = json::Parse(GetParam());
  EXPECT_TRUE(message_root.is_value());
  EXPECT_TRUE(ValidateReceiverMessage(message_root.value()).empty());
}
}  // namespace cast
}  // namespace openscreen
