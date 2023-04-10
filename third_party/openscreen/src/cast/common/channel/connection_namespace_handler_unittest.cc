// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/channel/connection_namespace_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "cast/common/channel/message_util.h"
#include "cast/common/channel/testing/fake_cast_socket.h"
#include "cast/common/channel/testing/mock_socket_error_handler.h"
#include "cast/common/channel/virtual_connection.h"
#include "cast/common/channel/virtual_connection_router.h"
#include "cast/common/public/cast_socket.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "util/json/json_serialization.h"
#include "util/json/json_value.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {
namespace {

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;

using ::cast::channel::CastMessage;
using ::cast::channel::CastMessage_ProtocolVersion;

class MockVirtualConnectionPolicy
    : public ConnectionNamespaceHandler::VirtualConnectionPolicy {
 public:
  ~MockVirtualConnectionPolicy() override = default;

  MOCK_METHOD(bool,
              IsConnectionAllowed,
              (const VirtualConnection& virtual_conn),
              (const, override));
};

CastMessage MakeVersionedConnectMessage(
    const std::string& source_id,
    const std::string& destination_id,
    absl::optional<CastMessage_ProtocolVersion> version,
    std::vector<CastMessage_ProtocolVersion> version_list) {
  CastMessage connect_message = MakeConnectMessage(source_id, destination_id);
  Json::Value message(Json::ValueType::objectValue);
  message[kMessageKeyType] = kMessageTypeConnect;
  if (version) {
    message[kMessageKeyProtocolVersion] = version.value();
  }
  if (!version_list.empty()) {
    Json::Value list(Json::ValueType::arrayValue);
    for (CastMessage_ProtocolVersion v : version_list) {
      list.append(v);
    }
    message[kMessageKeyProtocolVersionList] = std::move(list);
  }
  ErrorOr<std::string> result = json::Stringify(message);
  OSP_DCHECK(result);
  connect_message.set_payload_utf8(std::move(result.value()));
  return connect_message;
}

void VerifyConnectionMessage(const CastMessage& message,
                             const std::string& source_id,
                             const std::string& destination_id) {
  EXPECT_EQ(message.source_id(), source_id);
  EXPECT_EQ(message.destination_id(), destination_id);
  EXPECT_EQ(message.namespace_(), kConnectionNamespace);
  ASSERT_EQ(message.payload_type(),
            ::cast::channel::CastMessage_PayloadType_STRING);
}

Json::Value ParseConnectionMessage(const CastMessage& message) {
  ErrorOr<Json::Value> result = json::Parse(message.payload_utf8());
  OSP_CHECK(result) << message.payload_utf8();
  return result.value();
}

}  // namespace

class ConnectionNamespaceHandlerTest : public ::testing::Test {
 public:
  void SetUp() override {
    socket_ = fake_cast_socket_pair_.socket.get();
    router_.TakeSocket(&mock_error_handler_,
                       std::move(fake_cast_socket_pair_.socket));

    ON_CALL(vc_policy_, IsConnectionAllowed(_))
        .WillByDefault(
            Invoke([](const VirtualConnection& virtual_conn) { return true; }));
  }

 protected:
  void ExpectCloseMessage(MockCastSocketClient* mock_client,
                          const std::string& source_id,
                          const std::string& destination_id) {
    EXPECT_CALL(*mock_client, OnMessage(_, _))
        .WillOnce(Invoke([&source_id, &destination_id](CastSocket* socket,
                                                       CastMessage message) {
          VerifyConnectionMessage(message, source_id, destination_id);
          Json::Value value = ParseConnectionMessage(message);
          absl::optional<absl::string_view> type = MaybeGetString(
              value, JSON_EXPAND_FIND_CONSTANT_ARGS(kMessageKeyType));
          ASSERT_TRUE(type) << message.payload_utf8();
          EXPECT_EQ(type.value(), kMessageTypeClose) << message.payload_utf8();
        }));
  }

  void ExpectConnectedMessage(
      MockCastSocketClient* mock_client,
      const std::string& source_id,
      const std::string& destination_id,
      absl::optional<CastMessage_ProtocolVersion> version = absl::nullopt) {
    EXPECT_CALL(*mock_client, OnMessage(_, _))
        .WillOnce(Invoke([&source_id, &destination_id, version](
                             CastSocket* socket, CastMessage message) {
          VerifyConnectionMessage(message, source_id, destination_id);
          Json::Value value = ParseConnectionMessage(message);
          absl::optional<absl::string_view> type = MaybeGetString(
              value, JSON_EXPAND_FIND_CONSTANT_ARGS(kMessageKeyType));
          ASSERT_TRUE(type) << message.payload_utf8();
          EXPECT_EQ(type.value(), kMessageTypeConnected)
              << message.payload_utf8();
          if (version) {
            absl::optional<int> message_version = MaybeGetInt(
                value,
                JSON_EXPAND_FIND_CONSTANT_ARGS(kMessageKeyProtocolVersion));
            ASSERT_TRUE(message_version) << message.payload_utf8();
            EXPECT_EQ(message_version.value(), version.value());
          }
        }));
  }

  FakeCastSocketPair fake_cast_socket_pair_;
  MockSocketErrorHandler mock_error_handler_;
  CastSocket* socket_;

  NiceMock<MockVirtualConnectionPolicy> vc_policy_;
  VirtualConnectionRouter router_;
  ConnectionNamespaceHandler connection_namespace_handler_{&router_,
                                                           &vc_policy_};

  const std::string sender_id_{"sender-5678"};
  const std::string receiver_id_{"receiver-3245"};
};

TEST_F(ConnectionNamespaceHandlerTest, Connect) {
  connection_namespace_handler_.OnMessage(
      &router_, socket_, MakeConnectMessage(sender_id_, receiver_id_));
  EXPECT_TRUE(router_.GetConnectionData(
      VirtualConnection{receiver_id_, sender_id_, socket_->socket_id()}));

  EXPECT_CALL(fake_cast_socket_pair_.mock_peer_client, OnMessage(_, _))
      .Times(0);
}

TEST_F(ConnectionNamespaceHandlerTest, PolicyDeniesConnection) {
  EXPECT_CALL(vc_policy_, IsConnectionAllowed(_))
      .WillOnce(
          Invoke([](const VirtualConnection& virtual_conn) { return false; }));
  ExpectCloseMessage(&fake_cast_socket_pair_.mock_peer_client, receiver_id_,
                     sender_id_);
  connection_namespace_handler_.OnMessage(
      &router_, socket_, MakeConnectMessage(sender_id_, receiver_id_));
  EXPECT_FALSE(router_.GetConnectionData(
      VirtualConnection{receiver_id_, sender_id_, socket_->socket_id()}));
}

TEST_F(ConnectionNamespaceHandlerTest, ConnectWithVersion) {
  ExpectConnectedMessage(
      &fake_cast_socket_pair_.mock_peer_client, receiver_id_, sender_id_,
      ::cast::channel::CastMessage_ProtocolVersion_CASTV2_1_2);
  connection_namespace_handler_.OnMessage(
      &router_, socket_,
      MakeVersionedConnectMessage(
          sender_id_, receiver_id_,
          ::cast::channel::CastMessage_ProtocolVersion_CASTV2_1_2, {}));
  EXPECT_TRUE(router_.GetConnectionData(
      VirtualConnection{receiver_id_, sender_id_, socket_->socket_id()}));
}

TEST_F(ConnectionNamespaceHandlerTest, ConnectWithVersionList) {
  ExpectConnectedMessage(
      &fake_cast_socket_pair_.mock_peer_client, receiver_id_, sender_id_,
      ::cast::channel::CastMessage_ProtocolVersion_CASTV2_1_3);
  connection_namespace_handler_.OnMessage(
      &router_, socket_,
      MakeVersionedConnectMessage(
          sender_id_, receiver_id_,
          ::cast::channel::CastMessage_ProtocolVersion_CASTV2_1_2,
          {::cast::channel::CastMessage_ProtocolVersion_CASTV2_1_3,
           ::cast::channel::CastMessage_ProtocolVersion_CASTV2_1_0}));
  EXPECT_TRUE(router_.GetConnectionData(
      VirtualConnection{receiver_id_, sender_id_, socket_->socket_id()}));
}

TEST_F(ConnectionNamespaceHandlerTest, Close) {
  connection_namespace_handler_.OnMessage(
      &router_, socket_, MakeConnectMessage(sender_id_, receiver_id_));
  EXPECT_TRUE(router_.GetConnectionData(
      VirtualConnection{receiver_id_, sender_id_, socket_->socket_id()}));

  connection_namespace_handler_.OnMessage(
      &router_, socket_, MakeCloseMessage(sender_id_, receiver_id_));
  EXPECT_FALSE(router_.GetConnectionData(
      VirtualConnection{receiver_id_, sender_id_, socket_->socket_id()}));
}

TEST_F(ConnectionNamespaceHandlerTest, CloseUnknown) {
  connection_namespace_handler_.OnMessage(
      &router_, socket_, MakeConnectMessage(sender_id_, receiver_id_));
  EXPECT_TRUE(router_.GetConnectionData(
      VirtualConnection{receiver_id_, sender_id_, socket_->socket_id()}));

  connection_namespace_handler_.OnMessage(
      &router_, socket_, MakeCloseMessage(sender_id_ + "098", receiver_id_));
  EXPECT_TRUE(router_.GetConnectionData(
      VirtualConnection{receiver_id_, sender_id_, socket_->socket_id()}));
}

}  // namespace cast
}  // namespace openscreen
