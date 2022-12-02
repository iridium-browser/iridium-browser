// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/json/json_serialization.h"

#include <array>
#include <string>

#include "gtest/gtest.h"
#include "platform/base/error.h"

namespace openscreen {
namespace {
template <typename Value>
void AssertError(ErrorOr<Value> error_or, Error::Code code) {
  EXPECT_EQ(error_or.error().code(), code);
}
}  // namespace

TEST(JsonSerializationTest, MalformedDocumentReturnsParseError) {
  const std::array<std::string, 4> kMalformedDocuments{
      {"", "{", "{ foo: bar }", R"({"foo": "bar", "foo": baz})"}};

  for (auto& document : kMalformedDocuments) {
    AssertError(json::Parse(document), Error::Code::kJsonParseError);
  }
}

TEST(JsonSerializationTest, ValidEmptyDocumentParsedCorrectly) {
  const auto actual = json::Parse("{}");

  EXPECT_TRUE(actual.is_value());
  EXPECT_EQ(actual.value().getMemberNames().size(), 0u);
}

// Jsoncpp has its own suite of tests ensure that things are parsed correctly,
// so we only do some rudimentary checks here to make sure we didn't mangle
// the value.
TEST(JsonSerializationTest, ValidDocumentParsedCorrectly) {
  const auto actual = json::Parse(R"({"foo": "bar", "baz": 1337})");

  EXPECT_TRUE(actual.is_value());
  EXPECT_EQ(actual.value().getMemberNames().size(), 2u);
}

TEST(JsonSerializationTest, NullValueReturnsError) {
  const auto null_value = Json::Value();
  const auto actual = json::Stringify(null_value);

  EXPECT_TRUE(actual.is_error());
  EXPECT_EQ(actual.error().code(), Error::Code::kJsonWriteError);
}

TEST(JsonSerializationTest, ValidValueReturnsString) {
  const Json::Int64 value = 31337;
  const auto actual = json::Stringify(value);

  EXPECT_TRUE(actual.is_value());
  EXPECT_EQ(actual.value(), "31337");
}
}  // namespace openscreen
