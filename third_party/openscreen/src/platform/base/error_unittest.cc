// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/base/error.h"

#include "gtest/gtest.h"

namespace {

class Dummy {
 public:
  Dummy() = default;
  explicit Dummy(const std::string& x) : message(x) {}
  ~Dummy() = default;

  std::string message;
};

}  // namespace

namespace openscreen {

TEST(ErrorTest, TestDefaultError) {
  const Error error;
  EXPECT_EQ(error, Error::None());
  EXPECT_EQ(error.message(), "");
}

TEST(ErrorTest, TestNonDefaultError) {
  const Error error(Error::Code::kCborParsing, "Parse error");
  EXPECT_EQ(error.code(), Error::Code::kCborParsing);
  EXPECT_EQ(error.message(), "Parse error");

  const Error error2(error);
  EXPECT_EQ(error2.code(), Error::Code::kCborParsing);
  EXPECT_EQ(error2.message(), "Parse error");

  const Error error3 = error2;
  EXPECT_EQ(error, error2);
  EXPECT_EQ(error, error3);
  EXPECT_EQ(error2, error3);

  const Error default_error;
  EXPECT_FALSE(error == default_error);
  EXPECT_FALSE(error2 == default_error);
  EXPECT_FALSE(error3 == default_error);

  const Error error4(std::move(error2));
  const Error error5 = std::move(error3);
  EXPECT_EQ(error, error4);
  EXPECT_EQ(error, error5);
}

TEST(ErrorOrTest, ErrorToString) {
  const Error error_none(Error::Code::kNone);
  const Error error_none_with_msg(Error::Code::kNone, "Nothing to see here");
  const Error error_transient(Error::Code::kAgain);
  const Error error_transient_with_msg(Error::Code::kAgain,
                                       "Try again later maybe?");
  const Error error(Error::Code::kAddressInUse);
  const Error error_with_msg(Error::Code::kAddressInUse,
                             "Somebody called dibs");

  EXPECT_EQ("Success = \"\"", error_none.ToString());
  EXPECT_EQ("Success = \"Nothing to see here\"",
            error_none_with_msg.ToString());
  EXPECT_EQ("Failure: Transient = \"\"", error_transient.ToString());
  EXPECT_EQ("Failure: Transient = \"Try again later maybe?\"",
            error_transient_with_msg.ToString());
  EXPECT_EQ("Failure: AddressInUse = \"\"", error.ToString());
  EXPECT_EQ("Failure: AddressInUse = \"Somebody called dibs\"",
            error_with_msg.ToString());
}

TEST(ErrorOrTest, ErrorCodeToString) {
  constexpr auto error_none = Error::Code::kNone;
  constexpr auto error_transient = Error::Code::kAgain;
  constexpr auto error = Error::Code::kAddressInUse;

  EXPECT_EQ("Success", ToString(error_none));
  EXPECT_EQ("Failure: Transient", ToString(error_transient));
  EXPECT_EQ("Failure: AddressInUse", ToString(error));
}

TEST(ErrorOrTest, ErrorOrWithError) {
  ErrorOr<Dummy> error_or1(Error(Error::Code::kCborParsing, "Parse Error"));
  const ErrorOr<Dummy> error_or2(Error::Code::kCborParsing);
  ErrorOr<Dummy> error_or3(Error::Code::kCborParsing, "Parse Error Again");

  EXPECT_FALSE(error_or1);
  EXPECT_FALSE(error_or1.is_value());
  EXPECT_TRUE(error_or1.is_error());
  EXPECT_EQ(error_or1.error().code(), Error::Code::kCborParsing);
  EXPECT_EQ(error_or1.error().message(), "Parse Error");

  EXPECT_FALSE(error_or2);
  EXPECT_FALSE(error_or2.is_value());
  EXPECT_TRUE(error_or2.is_error());
  EXPECT_EQ(error_or2.error().code(), Error::Code::kCborParsing);
  EXPECT_EQ(error_or2.error().message(), "");

  EXPECT_FALSE(error_or3);
  EXPECT_FALSE(error_or3.is_value());
  EXPECT_TRUE(error_or3.is_error());
  EXPECT_EQ(error_or3.error().code(), Error::Code::kCborParsing);
  EXPECT_EQ(error_or3.error().message(), "Parse Error Again");

  const ErrorOr<Dummy> error_or4(std::move(error_or1));
  const ErrorOr<Dummy> error_or5 = std::move(error_or3);

  EXPECT_FALSE(error_or4);
  EXPECT_FALSE(error_or4.is_value());
  EXPECT_TRUE(error_or4.is_error());
  EXPECT_EQ(error_or4.error().code(), Error::Code::kCborParsing);
  EXPECT_EQ(error_or4.error().message(), "Parse Error");

  EXPECT_FALSE(error_or5);
  EXPECT_FALSE(error_or5.is_value());
  EXPECT_TRUE(error_or5.is_error());
  EXPECT_EQ(error_or5.error().code(), Error::Code::kCborParsing);
  EXPECT_EQ(error_or5.error().message(), "Parse Error Again");
}

TEST(ErrorOrTest, ErrorOrWithPrimitiveValue) {
  const ErrorOr<int> error_or_integer(1337);
  EXPECT_TRUE(error_or_integer);
  EXPECT_TRUE(error_or_integer.is_value());
  EXPECT_FALSE(error_or_integer.is_error());
  EXPECT_EQ(error_or_integer.value(), 1337);
}

TEST(ErrorOrTest, ErrorOrWithValue) {
  ErrorOr<Dummy> error_or1(Dummy("Winterfell"));
  ErrorOr<Dummy> error_or2(Dummy("Riverrun"));

  EXPECT_TRUE(error_or1);
  EXPECT_TRUE(error_or1.is_value());
  EXPECT_FALSE(error_or1.is_error());
  EXPECT_EQ(error_or1.value().message, "Winterfell");

  EXPECT_TRUE(error_or2);
  EXPECT_TRUE(error_or2.is_value());
  EXPECT_FALSE(error_or2.is_error());
  EXPECT_EQ(error_or2.value().message, "Riverrun");

  const ErrorOr<Dummy> error_or3(std::move(error_or1));
  ErrorOr<Dummy> error_or4 = std::move(error_or2);

  EXPECT_TRUE(error_or3);
  EXPECT_TRUE(error_or3.is_value());
  EXPECT_FALSE(error_or3.is_error());
  EXPECT_EQ(error_or3.value().message, "Winterfell");

  EXPECT_TRUE(error_or4);
  EXPECT_TRUE(error_or4.is_value());
  EXPECT_FALSE(error_or4.is_error());
  EXPECT_EQ(error_or4.value().message, "Riverrun");

  Dummy value = std::move(error_or4.value());
  EXPECT_EQ(value.message, "Riverrun");
}

TEST(ErrorOrTest, ComparisonTests) {
  const ErrorOr<int> e1(7);
  const ErrorOr<int> e2(7);
  const ErrorOr<int> e3(2);
  const ErrorOr<int> e4(10);

  const ErrorOr<int> e5(Error::Code::kAgain);
  const ErrorOr<int> e6(Error::Code::kCborParsing);
  const ErrorOr<int> e7(Error::Code::kCborEncoding);
  const ErrorOr<int> e8(Error::Code::kCborEncoding);

  const ErrorOr<int> e9(Error::Code::kAgain, "foo");
  const ErrorOr<int> e10(Error::Code::kAgain, "bar");

  EXPECT_EQ(e1, e2);
  EXPECT_EQ(e7, e8);
  EXPECT_LE(e1, e2);
  EXPECT_GE(e7, e8);

  EXPECT_NE(e1, e3);
  EXPECT_NE(e1, e4);
  EXPECT_NE(e1, e5);
  EXPECT_NE(e1, e6);
  EXPECT_NE(e1, e7);
  EXPECT_NE(e5, e2);
  EXPECT_NE(e5, e3);
  EXPECT_NE(e5, e4);
  EXPECT_NE(e5, e6);
  EXPECT_NE(e5, e9);
  EXPECT_NE(e5, e10);
  EXPECT_NE(e9, e10);

  EXPECT_LT(e3, e1);
  EXPECT_GT(e4, e1);
  EXPECT_LT(e5, e6);
  EXPECT_LE(e5, e9);
  EXPECT_LE(e5, e10);
  EXPECT_LE(e9, e10);
  EXPECT_GT(e7, e6);

  EXPECT_GT(e1, e5);
  EXPECT_GT(e2, e5);
  EXPECT_GT(e3, e5);
  EXPECT_GT(e4, e5);
  EXPECT_GT(e1, e6);
  EXPECT_GT(e2, e6);
  EXPECT_GT(e3, e6);
  EXPECT_GT(e4, e6);
  EXPECT_GT(e1, e7);
  EXPECT_GT(e2, e7);
  EXPECT_GT(e3, e7);
  EXPECT_GT(e4, e7);
}

}  // namespace openscreen
