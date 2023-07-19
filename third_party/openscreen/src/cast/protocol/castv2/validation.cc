// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/protocol/castv2/validation.h"

#include <mutex>  // NOLINT
#include <string>

#include "absl/strings/str_join.h"
#include "cast/protocol/castv2/receiver_schema_data.h"
#include "cast/protocol/castv2/streaming_schema_data.h"
#include "third_party/valijson/src/include/valijson/adapters/jsoncpp_adapter.hpp"
#include "third_party/valijson/src/include/valijson/schema.hpp"
#include "third_party/valijson/src/include/valijson/schema_parser.hpp"
#include "third_party/valijson/src/include/valijson/utils/jsoncpp_utils.hpp"
#include "third_party/valijson/src/include/valijson/validator.hpp"
#include "util/json/json_serialization.h"
#include "util/osp_logging.h"
#include "util/std_util.h"
#include "util/stringprintf.h"

namespace openscreen {
namespace cast {

namespace {

std::vector<Error> MapErrors(const valijson::ValidationResults& results) {
  std::vector<Error> errors;
  errors.reserve(results.numErrors());
  for (const auto& result : results) {
    const std::string context = absl::StrJoin(result.context, ", ");
    errors.emplace_back(Error::Code::kJsonParseError,
                        StringPrintf("Node: %s, Message: %s", context.c_str(),
                                     result.description.c_str()));
  }
  return errors;
}

void LoadSchema(const char* schema_json, valijson::Schema* schema) {
  Json::Value root = json::Parse(schema_json).value();
  valijson::adapters::JsonCppAdapter adapter(root);
  valijson::SchemaParser parser;
  parser.populateSchema(adapter, *schema);
}

std::vector<Error> Validate(const Json::Value& document,
                            const valijson::Schema& schema) {
  valijson::Validator validator;
  valijson::adapters::JsonCppAdapter document_adapter(document);
  valijson::ValidationResults results;
  if (validator.validate(schema, document_adapter, &results)) {
    return {};
  }
  return MapErrors(results);
}

}  // anonymous namespace
std::vector<Error> Validate(const Json::Value& document,
                            const Json::Value& schema_root) {
  valijson::adapters::JsonCppAdapter adapter(schema_root);
  valijson::Schema schema;
  valijson::SchemaParser parser;
  parser.populateSchema(adapter, schema);

  return Validate(document, schema);
}

std::vector<Error> ValidateStreamingMessage(const Json::Value& message) {
  static valijson::Schema schema;
  static std::once_flag flag;
  std::call_once(flag, [] { LoadSchema(kStreamingSchema, &schema); });
  return Validate(message, schema);
}

std::vector<Error> ValidateReceiverMessage(const Json::Value& message) {
  static valijson::Schema schema;
  static std::once_flag flag;
  std::call_once(flag, [] { LoadSchema(kReceiverSchema, &schema); });
  return Validate(message, schema);
}

}  // namespace cast
}  // namespace openscreen
