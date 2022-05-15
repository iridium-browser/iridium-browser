// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/public/cast_socket.h"

#include "cast/common/channel/message_framer.h"
#include "cast/common/channel/proto/cast_channel.pb.h"
#include "cast/common/channel/testing/fake_cast_socket.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace openscreen {
namespace cast {

using ::cast::channel::CastMessage;

namespace {

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

class CastSocketTest : public ::testing::Test {
 public:
  void SetUp() override {
    message_.set_protocol_version(CastMessage::CASTV2_1_0);
    message_.set_source_id("source");
    message_.set_destination_id("destination");
    message_.set_namespace_("namespace");
    message_.set_payload_type(CastMessage::STRING);
    message_.set_payload_utf8("payload");
    ErrorOr<std::vector<uint8_t>> serialized_or_error =
        message_serialization::Serialize(message_);
    ASSERT_TRUE(serialized_or_error);
    frame_serial_ = std::move(serialized_or_error.value());
  }

 protected:
  MockTlsConnection& connection() { return *fake_socket_.connection; }
  MockCastSocketClient& mock_client() { return fake_socket_.mock_client; }
  CastSocket& socket() { return fake_socket_.socket; }

  FakeCastSocket fake_socket_;
  CastMessage message_;
  std::vector<uint8_t> frame_serial_;
};

}  // namespace

TEST_F(CastSocketTest, SendMessage) {
  EXPECT_CALL(connection(), Send(_, _))
      .WillOnce(Invoke([this](const void* data, size_t len) {
        EXPECT_EQ(
            frame_serial_,
            std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(data),
                                 reinterpret_cast<const uint8_t*>(data) + len));
        return true;
      }));
  ASSERT_TRUE(socket().Send(message_).ok());
}

TEST_F(CastSocketTest, SendMessageEventuallyBlocks) {
  EXPECT_CALL(connection(), Send(_, _))
      .Times(3)
      .WillRepeatedly(Invoke([this](const void* data, size_t len) {
        EXPECT_EQ(
            frame_serial_,
            std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(data),
                                 reinterpret_cast<const uint8_t*>(data) + len));
        return true;
      }))
      .RetiresOnSaturation();
  ASSERT_TRUE(socket().Send(message_).ok());
  ASSERT_TRUE(socket().Send(message_).ok());
  ASSERT_TRUE(socket().Send(message_).ok());

  EXPECT_CALL(connection(), Send(_, _)).WillOnce(Return(false));
  ASSERT_EQ(socket().Send(message_).code(), Error::Code::kAgain);
}

TEST_F(CastSocketTest, ReadCompleteMessage) {
  const uint8_t* data = frame_serial_.data();
  EXPECT_CALL(mock_client(), OnMessage(_, _))
      .WillOnce(Invoke([this](CastSocket* socket, CastMessage message) {
        EXPECT_EQ(message_.SerializeAsString(), message.SerializeAsString());
      }));
  connection().OnRead(std::vector<uint8_t>(data, data + frame_serial_.size()));
}

TEST_F(CastSocketTest, ReadChunkedMessage) {
  const uint8_t* data = frame_serial_.data();
  EXPECT_CALL(mock_client(), OnMessage(_, _)).Times(0);
  connection().OnRead(std::vector<uint8_t>(data, data + 10));

  EXPECT_CALL(mock_client(), OnMessage(_, _))
      .WillOnce(Invoke([this](CastSocket* socket, CastMessage message) {
        EXPECT_EQ(message_.SerializeAsString(), message.SerializeAsString());
      }));
  connection().OnRead(
      std::vector<uint8_t>(data + 10, data + frame_serial_.size()));

  std::vector<uint8_t> double_message;
  double_message.insert(double_message.end(), frame_serial_.begin(),
                        frame_serial_.end());
  double_message.insert(double_message.end(), frame_serial_.begin(),
                        frame_serial_.end());
  data = double_message.data();
  EXPECT_CALL(mock_client(), OnMessage(_, _))
      .WillOnce(Invoke([this](CastSocket* socket, CastMessage message) {
        EXPECT_EQ(message_.SerializeAsString(), message.SerializeAsString());
      }));
  connection().OnRead(
      std::vector<uint8_t>(data, data + frame_serial_.size() + 10));

  EXPECT_CALL(mock_client(), OnMessage(_, _))
      .WillOnce(Invoke([this](CastSocket* socket, CastMessage message) {
        EXPECT_EQ(message_.SerializeAsString(), message.SerializeAsString());
      }));
  connection().OnRead(std::vector<uint8_t>(data + frame_serial_.size() + 10,
                                           data + double_message.size()));
}

TEST_F(CastSocketTest, ReadMultipleMessagesPerBlock) {
  CastMessage message2;
  std::vector<uint8_t> frame_serial2;
  message2.set_protocol_version(CastMessage::CASTV2_1_0);
  message2.set_source_id("alt-source");
  message2.set_destination_id("alt-destination");
  message2.set_namespace_("alt-namespace");
  message2.set_payload_type(CastMessage::STRING);
  message2.set_payload_utf8("alternate payload");
  ErrorOr<std::vector<uint8_t>> serialized_or_error =
      message_serialization::Serialize(message2);
  ASSERT_TRUE(serialized_or_error);
  frame_serial2 = std::move(serialized_or_error.value());

  std::vector<uint8_t> send_data;
  send_data.reserve(frame_serial_.size() + frame_serial2.size());
  send_data.insert(send_data.end(), frame_serial_.begin(), frame_serial_.end());
  send_data.insert(send_data.end(), frame_serial2.begin(), frame_serial2.end());
  EXPECT_CALL(mock_client(), OnMessage(_, _))
      .WillOnce(Invoke([this](CastSocket* socket, CastMessage message) {
        EXPECT_EQ(message_.SerializeAsString(), message.SerializeAsString());
      }))
      .WillOnce([message2](CastSocket* socket, CastMessage message) {
        EXPECT_EQ(message2.SerializeAsString(), message.SerializeAsString());
      });
  connection().OnRead(std::move(send_data));
}

TEST_F(CastSocketTest, SanitizedAddress) {
  std::array<uint8_t, 2> result1 = socket().GetSanitizedIpAddress();
  EXPECT_EQ(result1[0], 1u);
  EXPECT_EQ(result1[1], 9u);

  FakeCastSocket v6_socket(IPEndpoint{{1, 2, 3, 4}, 1025},
                           IPEndpoint{{0x1819, 0x1a1b, 0x1c1d, 0x1e1f, 0x207b,
                                       0x7c7d, 0x7e7f, 0x8081},
                                      4321});
  std::array<uint8_t, 2> result2 = v6_socket.socket.GetSanitizedIpAddress();
  EXPECT_EQ(result2[0], 128);
  EXPECT_EQ(result2[1], 129);
}

}  // namespace cast
}  // namespace openscreen
