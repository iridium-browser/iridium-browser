// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/channel/message_framer.h"

#include <stddef.h>

#include <algorithm>
#include <string>

#include "cast/common/channel/proto/cast_channel.pb.h"
#include "gtest/gtest.h"
#include "util/big_endian.h"
#include "util/std_util.h"

namespace openscreen {
namespace cast {
namespace message_serialization {

using ::cast::channel::CastMessage;

namespace {

static constexpr size_t kHeaderSize = sizeof(uint32_t);

// Cast specifies a max message body size of 64 KiB.
static constexpr size_t kMaxBodySize = 65536;

}  // namespace

class CastFramerTest : public testing::Test {
 public:
  CastFramerTest() : buffer_(kHeaderSize + kMaxBodySize) {}

  void SetUp() override {
    cast_message_.set_protocol_version(CastMessage::CASTV2_1_0);
    cast_message_.set_source_id("source");
    cast_message_.set_destination_id("destination");
    cast_message_.set_namespace_("namespace");
    cast_message_.set_payload_type(CastMessage::STRING);
    cast_message_.set_payload_utf8("payload");
    ErrorOr<std::vector<uint8_t>> result = Serialize(cast_message_);
    ASSERT_TRUE(result.is_value());
    cast_message_serial_ = std::move(result.value());
  }

  void WriteToBuffer(const std::vector<uint8_t>& data) {
    memcpy(&buffer_[0], data.data(), data.size());
  }

  absl::Span<uint8_t> GetSpan(size_t size) {
    return absl::Span<uint8_t>(&buffer_[0], size);
  }
  absl::Span<uint8_t> GetSpan() { return GetSpan(cast_message_serial_.size()); }

 protected:
  CastMessage cast_message_;
  std::vector<uint8_t> cast_message_serial_;
  std::vector<uint8_t> buffer_;
};

TEST_F(CastFramerTest, TestMessageFramerCompleteMessage) {
  WriteToBuffer(cast_message_serial_);

  // Receive 1 byte of the header, framer demands 3 more bytes.
  ErrorOr<DeserializeResult> result = TryDeserialize(GetSpan(1));
  EXPECT_FALSE(result);
  EXPECT_EQ(Error::Code::kInsufficientBuffer, result.error().code());

  // TryDeserialize remaining 3, expect that the framer has moved on to
  // requesting the body contents.
  result = TryDeserialize(GetSpan(3));
  EXPECT_FALSE(result);
  EXPECT_EQ(Error::Code::kInsufficientBuffer, result.error().code());

  // Remainder of packet sent over the wire.
  result = TryDeserialize(GetSpan());
  ASSERT_TRUE(result);
  EXPECT_EQ(result.value().length, cast_message_serial_.size());
  const CastMessage& message = result.value().message;
  EXPECT_EQ(message.SerializeAsString(), cast_message_.SerializeAsString());
}

TEST_F(CastFramerTest, TestSerializeErrorMessageTooLarge) {
  CastMessage big_message;
  big_message.CopyFrom(cast_message_);
  std::string payload;
  payload.append(kMaxBodySize + 1, 'x');
  big_message.set_payload_utf8(payload);
  EXPECT_FALSE(Serialize(big_message));
}

TEST_F(CastFramerTest, TestCompleteMessageAtOnce) {
  WriteToBuffer(cast_message_serial_);

  ErrorOr<DeserializeResult> result = TryDeserialize(GetSpan());
  ASSERT_TRUE(result);
  EXPECT_EQ(result.value().length, cast_message_serial_.size());
  const CastMessage& message = result.value().message;
  EXPECT_EQ(message.SerializeAsString(), cast_message_.SerializeAsString());
}

TEST_F(CastFramerTest, TestTryDeserializeIllegalLargeMessage) {
  std::vector<uint8_t> mangled_cast_message = cast_message_serial_;
  mangled_cast_message[0] = 88;
  mangled_cast_message[1] = 88;
  mangled_cast_message[2] = 88;
  mangled_cast_message[3] = 88;
  WriteToBuffer(mangled_cast_message);

  ErrorOr<DeserializeResult> result = TryDeserialize(GetSpan(4));
  ASSERT_FALSE(result);
  EXPECT_EQ(Error::Code::kCastV2InvalidMessage, result.error().code());
}

TEST_F(CastFramerTest, TestTryDeserializeIllegalLargeMessage2) {
  std::vector<uint8_t> mangled_cast_message = cast_message_serial_;
  // Header indicates body size is 0x00010001 = 65537
  mangled_cast_message[0] = 0;
  mangled_cast_message[1] = 0x1;
  mangled_cast_message[2] = 0;
  mangled_cast_message[3] = 0x1;
  WriteToBuffer(mangled_cast_message);

  ErrorOr<DeserializeResult> result = TryDeserialize(GetSpan(4));
  ASSERT_FALSE(result);
  EXPECT_EQ(Error::Code::kCastV2InvalidMessage, result.error().code());
}

TEST_F(CastFramerTest, TestUnparsableBodyProto) {
  // Message header is OK, but the body is replaced with "x"es.
  std::vector<uint8_t> mangled_cast_message = cast_message_serial_;
  for (size_t i = kHeaderSize; i < mangled_cast_message.size(); ++i) {
    std::fill(mangled_cast_message.begin() + kHeaderSize,
              mangled_cast_message.end(), 'x');
  }
  WriteToBuffer(mangled_cast_message);

  // Send header.
  ErrorOr<DeserializeResult> result = TryDeserialize(GetSpan(4));
  EXPECT_FALSE(result);
  EXPECT_EQ(Error::Code::kInsufficientBuffer, result.error().code());

  // Send body, expect an error.
  result = TryDeserialize(GetSpan());
  ASSERT_FALSE(result);
  EXPECT_EQ(Error::Code::kCastV2InvalidMessage, result.error().code());
}

}  // namespace message_serialization
}  // namespace cast
}  // namespace openscreen
