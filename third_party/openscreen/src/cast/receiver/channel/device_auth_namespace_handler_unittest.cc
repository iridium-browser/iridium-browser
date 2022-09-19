// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/receiver/channel/device_auth_namespace_handler.h"

#include <utility>

#include "cast/common/certificate/testing/test_helpers.h"
#include "cast/common/channel/message_util.h"
#include "cast/common/channel/proto/cast_channel.pb.h"
#include "cast/common/channel/testing/fake_cast_socket.h"
#include "cast/common/channel/testing/mock_socket_error_handler.h"
#include "cast/common/channel/virtual_connection_router.h"
#include "cast/common/public/cast_socket.h"
#include "cast/receiver/channel/static_credentials.h"
#include "cast/receiver/channel/testing/device_auth_test_helpers.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/test/paths.h"
#include "testing/util/read_file.h"

namespace openscreen {
namespace cast {
namespace {

using ::cast::channel::AuthResponse;
using ::cast::channel::CastMessage;
using ::cast::channel::DeviceAuthMessage;
using ::cast::channel::SignatureAlgorithm;

using ::testing::_;
using ::testing::ElementsAreArray;
using ::testing::Invoke;

const std::string& GetSpecificTestDataPath() {
  static std::string data_path = GetTestDataPath() + "cast/receiver/channel/";
  return data_path;
}

class DeviceAuthNamespaceHandlerTest : public ::testing::Test {
 public:
  void SetUp() override {
    socket_ = fake_cast_socket_pair_.socket.get();
    router_.TakeSocket(&mock_error_handler_,
                       std::move(fake_cast_socket_pair_.socket));
    router_.AddHandlerForLocalId(kPlatformReceiverId, &auth_handler_);
  }

 protected:
  const std::string& data_path_{GetSpecificTestDataPath()};
  FakeCastSocketPair fake_cast_socket_pair_;
  MockSocketErrorHandler mock_error_handler_;
  CastSocket* socket_;

  StaticCredentialsProvider creds_;
  VirtualConnectionRouter router_;
  DeviceAuthNamespaceHandler auth_handler_{&creds_};
};

// The tests in this file use a pre-recorded AuthChallenge as input and a
// matching pre-recorded AuthResponse for verification.  This is to make it
// easier to keep sender and receiver code separate, because the code that would
// really generate an AuthChallenge and verify an AuthResponse is under
// //cast/sender.  The pre-recorded messages come from an integration test which
// _does_ properly call both sender and receiver sides, but can optionally
// record the messages for use in these unit tests.  That test is currently
// under //cast/test.  See //cast/test/README.md for more information.
//
// The tests generally follow this procedure:
//  1. Read a fake device certificate chain + TLS certificate from disk.
//  2. Read a pre-recorded CastMessage proto containing an AuthChallenge.
//  3. Send this CastMessage over a CastSocket to a DeviceAuthNamespaceHandler.
//  4. Catch the CastMessage response and check that it has an AuthResponse.
//  5. Check the AuthResponse against another pre-recorded protobuf.

TEST_F(DeviceAuthNamespaceHandlerTest, AuthResponse) {
  InitStaticCredentialsFromFiles(
      &creds_, nullptr, nullptr, data_path_ + "device_key.pem",
      data_path_ + "device_chain.pem", data_path_ + "device_tls.pem");

  // Send an auth challenge.  |auth_handler_| will automatically respond via
  // |router_| and we will catch the result in |challenge_reply|.
  CastMessage auth_challenge;
  const std::string auth_challenge_string =
      ReadEntireFileToString(data_path_ + "auth_challenge.pb");
  ASSERT_TRUE(auth_challenge.ParseFromString(auth_challenge_string));

  CastMessage challenge_reply;
  EXPECT_CALL(fake_cast_socket_pair_.mock_peer_client, OnMessage(_, _))
      .WillOnce(
          Invoke([&challenge_reply](CastSocket* socket, CastMessage message) {
            challenge_reply = std::move(message);
          }));
  ASSERT_TRUE(
      fake_cast_socket_pair_.peer_socket->Send(std::move(auth_challenge)).ok());

  const std::string auth_response_string =
      ReadEntireFileToString(data_path_ + "auth_response.pb");
  AuthResponse expected_auth_response;
  ASSERT_TRUE(expected_auth_response.ParseFromString(auth_response_string));

  DeviceAuthMessage auth_message;
  ASSERT_EQ(challenge_reply.payload_type(),
            ::cast::channel::CastMessage_PayloadType_BINARY);
  ASSERT_TRUE(auth_message.ParseFromString(challenge_reply.payload_binary()));
  ASSERT_TRUE(auth_message.has_response());
  ASSERT_FALSE(auth_message.has_challenge());
  ASSERT_FALSE(auth_message.has_error());
  const AuthResponse& auth_response = auth_message.response();

  EXPECT_EQ(expected_auth_response.signature(), auth_response.signature());
  EXPECT_EQ(expected_auth_response.client_auth_certificate(),
            auth_response.client_auth_certificate());
  EXPECT_EQ(expected_auth_response.signature_algorithm(),
            auth_response.signature_algorithm());
  EXPECT_EQ(expected_auth_response.sender_nonce(),
            auth_response.sender_nonce());
  EXPECT_EQ(expected_auth_response.hash_algorithm(),
            auth_response.hash_algorithm());
  EXPECT_EQ(expected_auth_response.crl(), auth_response.crl());
  EXPECT_THAT(
      auth_response.intermediate_certificate(),
      ElementsAreArray(expected_auth_response.intermediate_certificate()));
}

TEST_F(DeviceAuthNamespaceHandlerTest, BadNonce) {
  InitStaticCredentialsFromFiles(
      &creds_, nullptr, nullptr, data_path_ + "device_key.pem",
      data_path_ + "device_chain.pem", data_path_ + "device_tls.pem");

  // Send an auth challenge.  |auth_handler_| will automatically respond via
  // |router_| and we will catch the result in |challenge_reply|.
  CastMessage auth_challenge;
  const std::string auth_challenge_string =
      ReadEntireFileToString(data_path_ + "auth_challenge.pb");
  ASSERT_TRUE(auth_challenge.ParseFromString(auth_challenge_string));

  // Change the nonce to be different from what was used to record the correct
  // response originally.
  DeviceAuthMessage msg;
  ASSERT_EQ(auth_challenge.payload_type(),
            ::cast::channel::CastMessage_PayloadType_BINARY);
  ASSERT_TRUE(msg.ParseFromString(auth_challenge.payload_binary()));
  ASSERT_TRUE(msg.has_challenge());
  std::string* nonce = msg.mutable_challenge()->mutable_sender_nonce();
  (*nonce)[0] = ~(*nonce)[0];
  std::string new_payload;
  ASSERT_TRUE(msg.SerializeToString(&new_payload));
  auth_challenge.set_payload_binary(new_payload);

  CastMessage challenge_reply;
  EXPECT_CALL(fake_cast_socket_pair_.mock_peer_client, OnMessage(_, _))
      .WillOnce(
          Invoke([&challenge_reply](CastSocket* socket, CastMessage message) {
            challenge_reply = std::move(message);
          }));
  ASSERT_TRUE(
      fake_cast_socket_pair_.peer_socket->Send(std::move(auth_challenge)).ok());

  const std::string auth_response_string =
      ReadEntireFileToString(data_path_ + "auth_response.pb");
  AuthResponse expected_auth_response;
  ASSERT_TRUE(expected_auth_response.ParseFromString(auth_response_string));

  DeviceAuthMessage auth_message;
  ASSERT_EQ(challenge_reply.payload_type(),
            ::cast::channel::CastMessage_PayloadType_BINARY);
  ASSERT_TRUE(auth_message.ParseFromString(challenge_reply.payload_binary()));
  ASSERT_TRUE(auth_message.has_response());
  ASSERT_FALSE(auth_message.has_challenge());
  ASSERT_FALSE(auth_message.has_error());
  const AuthResponse& auth_response = auth_message.response();

  // NOTE: This is the ultimate result of the nonce-mismatch.
  EXPECT_NE(expected_auth_response.signature(), auth_response.signature());
}

TEST_F(DeviceAuthNamespaceHandlerTest, UnsupportedSignatureAlgorithm) {
  InitStaticCredentialsFromFiles(
      &creds_, nullptr, nullptr, data_path_ + "device_key.pem",
      data_path_ + "device_chain.pem", data_path_ + "device_tls.pem");

  // Send an auth challenge.  |auth_handler_| will automatically respond via
  // |router_| and we will catch the result in |challenge_reply|.
  CastMessage auth_challenge;
  const std::string auth_challenge_string =
      ReadEntireFileToString(data_path_ + "auth_challenge.pb");
  ASSERT_TRUE(auth_challenge.ParseFromString(auth_challenge_string));

  // Change the signature algorithm an unsupported value.
  DeviceAuthMessage msg;
  ASSERT_EQ(auth_challenge.payload_type(),
            ::cast::channel::CastMessage_PayloadType_BINARY);
  ASSERT_TRUE(msg.ParseFromString(auth_challenge.payload_binary()));
  ASSERT_TRUE(msg.has_challenge());
  msg.mutable_challenge()->set_signature_algorithm(
      SignatureAlgorithm::RSASSA_PSS);
  std::string new_payload;
  ASSERT_TRUE(msg.SerializeToString(&new_payload));
  auth_challenge.set_payload_binary(new_payload);

  CastMessage challenge_reply;
  EXPECT_CALL(fake_cast_socket_pair_.mock_peer_client, OnMessage(_, _))
      .WillOnce(
          Invoke([&challenge_reply](CastSocket* socket, CastMessage message) {
            challenge_reply = std::move(message);
          }));
  ASSERT_TRUE(
      fake_cast_socket_pair_.peer_socket->Send(std::move(auth_challenge)).ok());

  DeviceAuthMessage auth_message;
  ASSERT_EQ(challenge_reply.payload_type(),
            ::cast::channel::CastMessage_PayloadType_BINARY);
  ASSERT_TRUE(auth_message.ParseFromString(challenge_reply.payload_binary()));
  ASSERT_FALSE(auth_message.has_response());
  ASSERT_FALSE(auth_message.has_challenge());
  ASSERT_TRUE(auth_message.has_error());
}

}  // namespace
}  // namespace cast
}  // namespace openscreen
