// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/mdns/mdns_sender.h"

#include <memory>
#include <vector>

#include "discovery/mdns/mdns_records.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/test/fake_udp_socket.h"
#include "platform/test/mock_udp_socket.h"

namespace openscreen {
namespace discovery {

using testing::_;
using testing::Args;
using testing::Return;
using testing::StrictMock;
using testing::WithArgs;

namespace {

ACTION_P(VoidPointerMatchesBytes, expected_data) {
  const uint8_t* actual_data = static_cast<const uint8_t*>(arg0);
  for (size_t i = 0; i < expected_data.size(); ++i) {
    EXPECT_EQ(actual_data[i], expected_data[i]);
  }
}

}  // namespace

class MdnsSenderTest : public testing::Test {
 public:
  MdnsSenderTest()
      : a_question_(DomainName{"testing", "local"},
                    DnsType::kA,
                    DnsClass::kIN,
                    ResponseType::kMulticast),
        a_record_(DomainName{"testing", "local"},
                  DnsType::kA,
                  DnsClass::kIN,
                  RecordType::kShared,
                  std::chrono::seconds(120),
                  ARecordRdata(IPAddress{172, 0, 0, 1})),
        query_message_(1, MessageType::Query),
        response_message_(1, MessageType::Response),
        ipv4_multicast_endpoint_{
            .address = IPAddress(kDefaultMulticastGroupIPv4),
            .port = kDefaultMulticastPort},
        ipv6_multicast_endpoint_{
            .address = IPAddress(kDefaultMulticastGroupIPv6),
            .port = kDefaultMulticastPort} {
    query_message_.AddQuestion(a_question_);
    response_message_.AddAnswer(a_record_);
  }

 protected:
  // clang-format off
  const std::vector<uint8_t> kQueryBytes = {
      0x00, 0x01,  // ID = 1
      0x00, 0x00,  // FLAGS = None
      0x00, 0x01,  // Question count
      0x00, 0x00,  // Answer count
      0x00, 0x00,  // Authority count
      0x00, 0x00,  // Additional count
      // Question
      0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
      0x05, 'l', 'o', 'c', 'a', 'l',
      0x00,
      0x00, 0x01,  // TYPE = A (1)
      0x00, 0x01,  // CLASS = IN (1)
  };

  const std::vector<uint8_t> kResponseBytes = {
      0x00, 0x01,  // ID = 1
      0x84, 0x00,  // FLAGS = AA | RESPONSE
      0x00, 0x00,  // Question count
      0x00, 0x01,  // Answer count
      0x00, 0x00,  // Authority count
      0x00, 0x00,  // Additional count
      // Answer
      0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
      0x05, 'l', 'o', 'c', 'a', 'l',
      0x00,
      0x00, 0x01,              // TYPE = A (1)
      0x00, 0x01,              // CLASS = IN (1)
      0x00, 0x00, 0x00, 0x78,  // TTL = 120 seconds
      0x00, 0x04,              // RDLENGTH = 4 bytes
      0xac, 0x00, 0x00, 0x01,  // 172.0.0.1
  };
  // clang-format on

  MdnsQuestion a_question_;
  MdnsRecord a_record_;
  MdnsMessage query_message_;
  MdnsMessage response_message_;
  IPEndpoint ipv4_multicast_endpoint_;
  IPEndpoint ipv6_multicast_endpoint_;
};

TEST_F(MdnsSenderTest, SendMulticast) {
  StrictMock<MockUdpSocket> socket;
  EXPECT_CALL(socket, IsIPv4()).WillRepeatedly(Return(true));
  EXPECT_CALL(socket, IsIPv6()).WillRepeatedly(Return(true));
  MdnsSender sender(&socket);
  EXPECT_CALL(socket, SendMessage(_, kQueryBytes.size(), _))
      .WillOnce(WithArgs<0>(VoidPointerMatchesBytes(kQueryBytes)));
  EXPECT_EQ(sender.SendMulticast(query_message_), Error::Code::kNone);
}

TEST_F(MdnsSenderTest, SendUnicastIPv4) {
  IPEndpoint endpoint{.address = IPAddress{192, 168, 1, 1}, .port = 31337};

  StrictMock<MockUdpSocket> socket;
  MdnsSender sender(&socket);
  EXPECT_CALL(socket, SendMessage(_, kResponseBytes.size(), _))
      .WillOnce(WithArgs<0>(VoidPointerMatchesBytes(kResponseBytes)));
  EXPECT_EQ(sender.SendMessage(response_message_, endpoint),
            Error::Code::kNone);
}

TEST_F(MdnsSenderTest, SendUnicastIPv6) {
  constexpr uint16_t kIPv6AddressHextets[] = {
      0xfe80, 0x0000, 0x0000, 0x0000, 0x0202, 0xb3ff, 0xfe1e, 0x8329,
  };
  IPEndpoint endpoint{.address = IPAddress(kIPv6AddressHextets), .port = 31337};

  StrictMock<MockUdpSocket> socket;
  MdnsSender sender(&socket);
  EXPECT_CALL(socket, SendMessage(_, kResponseBytes.size(), _))
      .WillOnce(WithArgs<0>(VoidPointerMatchesBytes(kResponseBytes)));
  EXPECT_EQ(sender.SendMessage(response_message_, endpoint),
            Error::Code::kNone);
}

TEST_F(MdnsSenderTest, MessageTooBig) {
  MdnsMessage big_message_(1, MessageType::Query);
  for (size_t i = 0; i < 100; ++i) {
    big_message_.AddQuestion(a_question_);
    big_message_.AddAnswer(a_record_);
  }

  StrictMock<MockUdpSocket> socket;
  EXPECT_CALL(socket, IsIPv4()).WillRepeatedly(Return(true));
  EXPECT_CALL(socket, IsIPv6()).WillRepeatedly(Return(true));
  MdnsSender sender(&socket);
  EXPECT_EQ(sender.SendMulticast(big_message_),
            Error::Code::kInsufficientBuffer);
}

TEST_F(MdnsSenderTest, ReturnsErrorOnSocketFailure) {
  FakeUdpSocket::MockClient socket_client;
  FakeUdpSocket socket(nullptr, &socket_client);
  MdnsSender sender(&socket);
  Error error = Error(Error::Code::kConnectionFailed, "error message");
  socket.EnqueueSendResult(error);
  EXPECT_CALL(socket_client, OnSendError(_, error)).Times(1);
  EXPECT_EQ(sender.SendMulticast(query_message_), Error::Code::kNone);
  EXPECT_EQ(socket.send_queue_size(), size_t{0});
}

}  // namespace discovery
}  // namespace openscreen
