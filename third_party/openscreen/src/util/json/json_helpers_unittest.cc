// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/json/json_helpers.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "util/chrono_helpers.h"

namespace openscreen {
namespace json {
namespace {

using ::testing::ElementsAre;

const Json::Value kNone;
const Json::Value kEmptyString = "";
const Json::Value kEmptyArray(Json::arrayValue);

struct Dummy {
  int value;

  constexpr bool operator==(const Dummy& other) const {
    return other.value == value;
  }
};

bool TryParseDummy(const Json::Value& value, Dummy* out) {
  int value_out;
  if (!TryParseInt(value, &value_out)) {
    return false;
  }
  *out = Dummy{value_out};
  return true;
}

}  // namespace

TEST(ParsingHelpersTest, TryParseDouble) {
  const Json::Value kValid = 13.37;
  const Json::Value kNotDouble = "coffee beans";
  const Json::Value kNegativeDouble = -4.2;
  const Json::Value kZeroDouble = 0.0;
  const Json::Value kNanDouble = std::nan("");

  double out;
  EXPECT_TRUE(TryParseDouble(kValid, &out));
  EXPECT_DOUBLE_EQ(13.37, out);
  EXPECT_TRUE(TryParseDouble(kZeroDouble, &out));
  EXPECT_DOUBLE_EQ(0.0, out);
  EXPECT_FALSE(TryParseDouble(kNotDouble, &out));
  EXPECT_FALSE(TryParseDouble(kNegativeDouble, &out));
  EXPECT_FALSE(TryParseDouble(kNone, &out));
  EXPECT_FALSE(TryParseDouble(kNanDouble, &out));
}

TEST(ParsingHelpersTest, TryParseInt) {
  const Json::Value kValid = 1337;
  const Json::Value kNotInt = "cold brew";
  const Json::Value kNegativeInt = -42;
  const Json::Value kZeroInt = 0;

  int out;
  EXPECT_TRUE(TryParseInt(kValid, &out));
  EXPECT_EQ(1337, out);
  EXPECT_TRUE(TryParseInt(kZeroInt, &out));
  EXPECT_EQ(0, out);
  EXPECT_FALSE(TryParseInt(kNone, &out));
  EXPECT_FALSE(TryParseInt(kNotInt, &out));
  EXPECT_FALSE(TryParseInt(kNegativeInt, &out));
}

TEST(ParsingHelpersTest, TryParseUint) {
  const Json::Value kValid = 1337u;
  const Json::Value kNotUint = "espresso";
  const Json::Value kZeroUint = 0u;

  uint32_t out;
  EXPECT_TRUE(TryParseUint(kValid, &out));
  EXPECT_EQ(1337u, out);
  EXPECT_TRUE(TryParseUint(kZeroUint, &out));
  EXPECT_EQ(0u, out);
  EXPECT_FALSE(TryParseUint(kNone, &out));
  EXPECT_FALSE(TryParseUint(kNotUint, &out));
}

TEST(ParsingHelpersTest, TryParseString) {
  const Json::Value kValid = "macchiato";
  const Json::Value kNotString = 42;

  std::string out;
  EXPECT_TRUE(TryParseString(kValid, &out));
  EXPECT_EQ("macchiato", out);
  EXPECT_TRUE(TryParseString(kEmptyString, &out));
  EXPECT_EQ("", out);
  EXPECT_FALSE(TryParseString(kNone, &out));
  EXPECT_FALSE(TryParseString(kNotString, &out));
}

// Simple fraction validity is tested extensively in its unit tests, so we
// just check the major cases here.
TEST(ParsingHelpersTest, TryParseSimpleFraction) {
  const Json::Value kValid = "42/30";
  const Json::Value kValidNumber = "42";
  const Json::Value kUndefined = "5/0";
  const Json::Value kNegative = "10/-2";
  const Json::Value kInvalidNumber = "-1";
  const Json::Value kNotSimpleFraction = "latte";
  const Json::Value kInteger = 123;
  const Json::Value kNegativeInteger = -5000;

  SimpleFraction out;
  EXPECT_TRUE(TryParseSimpleFraction(kValid, &out));
  EXPECT_EQ((SimpleFraction{42, 30}), out);
  EXPECT_TRUE(TryParseSimpleFraction(kValidNumber, &out));
  EXPECT_EQ((SimpleFraction{42, 1}), out);
  EXPECT_TRUE(TryParseSimpleFraction(kInteger, &out));
  EXPECT_EQ((SimpleFraction{123, 1}), out);
  EXPECT_FALSE(TryParseSimpleFraction(kUndefined, &out));
  EXPECT_FALSE(TryParseSimpleFraction(kNegative, &out));
  EXPECT_FALSE(TryParseSimpleFraction(kInvalidNumber, &out));
  EXPECT_FALSE(TryParseSimpleFraction(kNotSimpleFraction, &out));
  EXPECT_FALSE(TryParseSimpleFraction(kNone, &out));
  EXPECT_FALSE(TryParseSimpleFraction(kEmptyString, &out));
  EXPECT_FALSE(TryParseSimpleFraction(kNegativeInteger, &out));
}

TEST(ParsingHelpersTest, TryParseMilliseconds) {
  const Json::Value kValid = 1000;
  const Json::Value kValidFloat = 500.0;
  const Json::Value kNegativeNumber = -120;
  const Json::Value kZeroNumber = 0;
  const Json::Value kNotNumber = "affogato";

  milliseconds out;
  EXPECT_TRUE(TryParseMilliseconds(kValid, &out));
  EXPECT_EQ(milliseconds(1000), out);
  EXPECT_TRUE(TryParseMilliseconds(kValidFloat, &out));
  EXPECT_EQ(milliseconds(500), out);
  EXPECT_TRUE(TryParseMilliseconds(kZeroNumber, &out));
  EXPECT_EQ(milliseconds(0), out);
  EXPECT_FALSE(TryParseMilliseconds(kNone, &out));
  EXPECT_FALSE(TryParseMilliseconds(kNegativeNumber, &out));
  EXPECT_FALSE(TryParseMilliseconds(kNotNumber, &out));
}

TEST(ParsingHelpersTest, TryParseArray) {
  Json::Value valid_dummy_array;
  valid_dummy_array[0] = 123;
  valid_dummy_array[1] = 456;

  Json::Value invalid_dummy_array;
  invalid_dummy_array[0] = "iced coffee";
  invalid_dummy_array[1] = 456;

  std::vector<Dummy> out;
  EXPECT_TRUE(TryParseArray<Dummy>(valid_dummy_array, TryParseDummy, &out));
  EXPECT_THAT(out, ElementsAre(Dummy{123}, Dummy{456}));
  EXPECT_FALSE(TryParseArray<Dummy>(invalid_dummy_array, TryParseDummy, &out));
  EXPECT_FALSE(TryParseArray<Dummy>(kEmptyArray, TryParseDummy, &out));
}

TEST(ParsingHelpersTest, TryParseIntArray) {
  Json::Value valid_int_array;
  valid_int_array[0] = 123;
  valid_int_array[1] = 456;

  Json::Value invalid_int_array;
  invalid_int_array[0] = "iced coffee";
  invalid_int_array[1] = 456;

  std::vector<int> out;
  EXPECT_TRUE(TryParseIntArray(valid_int_array, &out));
  EXPECT_THAT(out, ElementsAre(123, 456));
  EXPECT_FALSE(TryParseIntArray(invalid_int_array, &out));
  EXPECT_FALSE(TryParseIntArray(kEmptyArray, &out));
}

TEST(ParsingHelpersTest, TryParseUintArray) {
  Json::Value valid_uint_array;
  valid_uint_array[0] = 123u;
  valid_uint_array[1] = 456u;

  Json::Value invalid_uint_array;
  invalid_uint_array[0] = "breve";
  invalid_uint_array[1] = 456u;

  std::vector<uint32_t> out;
  EXPECT_TRUE(TryParseUintArray(valid_uint_array, &out));
  EXPECT_THAT(out, ElementsAre(123u, 456u));
  EXPECT_FALSE(TryParseUintArray(invalid_uint_array, &out));
  EXPECT_FALSE(TryParseUintArray(kEmptyArray, &out));
}

TEST(ParsingHelpersTest, TryParseStringArray) {
  Json::Value valid_string_array;
  valid_string_array[0] = "nitro cold brew";
  valid_string_array[1] = "doppio espresso";

  Json::Value invalid_string_array;
  invalid_string_array[0] = "mocha latte";
  invalid_string_array[1] = 456;

  std::vector<std::string> out;
  EXPECT_TRUE(TryParseStringArray(valid_string_array, &out));
  EXPECT_THAT(out, ElementsAre("nitro cold brew", "doppio espresso"));
  EXPECT_FALSE(TryParseStringArray(invalid_string_array, &out));
  EXPECT_FALSE(TryParseStringArray(kEmptyArray, &out));
}

}  // namespace json
}  // namespace openscreen
