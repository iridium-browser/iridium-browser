// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/receiver_message.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "util/json/json_serialization.h"

namespace openscreen {
namespace cast {

TEST(ReceiverMessageTest, ReceiverErrorConstructors) {
  const ReceiverError kOpenscreenError(Error::Code::kNoStreamSelected,
                                       "description one");
  const int kOpenscreenCode = static_cast<int>(Error::Code::kNoStreamSelected);

  // All Open Screen codes are offset for ReceiverError transmission by a fixed
  // value.
  const int kOffsetOpenscreenCode = kOpenscreenCode + 10000;
  const ReceiverError kIntegerError(kOffsetOpenscreenCode, "description two");
  const ReceiverError kRandomError(1234, "description three");

  ASSERT_TRUE(kOpenscreenError.openscreen_code);
  EXPECT_EQ(Error::Code::kNoStreamSelected,
            kOpenscreenError.openscreen_code.value());
  EXPECT_EQ(kOffsetOpenscreenCode, kOpenscreenError.code);
  EXPECT_EQ("description one", kOpenscreenError.description);

  ASSERT_TRUE(kIntegerError.openscreen_code);
  EXPECT_EQ(Error::Code::kNoStreamSelected,
            kIntegerError.openscreen_code.value());
  EXPECT_EQ(kOffsetOpenscreenCode, kIntegerError.code);
  EXPECT_EQ("description two", kIntegerError.description);

  EXPECT_FALSE(kRandomError.openscreen_code);
  EXPECT_EQ(1234, kRandomError.code);
  EXPECT_EQ("description three", kRandomError.description);
}

TEST(ReceiverMessageTest, ReceiverErrorToError) {
  EXPECT_EQ(Error(Error::Code::kNotImplemented, "message"),
            ReceiverError(Error::Code::kNotImplemented, "message").ToError());
  EXPECT_EQ(Error(Error::Code::kUnknownError,
                  "Error code: 1234, description: message two"),
            ReceiverError(1234, "message two").ToError());
}

}  // namespace cast
}  // namespace openscreen
