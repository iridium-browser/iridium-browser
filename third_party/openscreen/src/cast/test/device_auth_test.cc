// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include "cast/common/certificate/testing/test_helpers.h"
#include "cast/common/channel/proto/cast_channel.pb.h"
#include "cast/common/channel/testing/fake_cast_socket.h"
#include "cast/common/channel/testing/mock_socket_error_handler.h"
#include "cast/common/channel/virtual_connection_router.h"
#include "cast/common/public/cast_socket.h"
#include "cast/common/public/parsed_certificate.h"
#include "cast/common/public/trust_store.h"
#include "cast/receiver/channel/device_auth_namespace_handler.h"
#include "cast/receiver/channel/static_credentials.h"
#include "cast/receiver/channel/testing/device_auth_test_helpers.h"
#include "cast/sender/channel/cast_auth_util.h"
#include "cast/sender/channel/message_util.h"
#include "gtest/gtest.h"
#include "platform/test/paths.h"
#include "testing/util/read_file.h"

namespace openscreen {
namespace cast {
namespace {

using ::cast::channel::CastMessage;
using ::cast::channel::DeviceAuthMessage;

using ::testing::_;
using ::testing::Invoke;

const std::string& GetSpecificTestDataPath() {
  static std::string data_path = GetTestDataPath() + "cast/receiver/channel/";
  return data_path;
}

class DeviceAuthTest : public ::testing::Test {
 public:
  void SetUp() override {
    socket_ = fake_cast_socket_pair_.socket.get();
    router_.TakeSocket(&mock_error_handler_,
                       std::move(fake_cast_socket_pair_.socket));
    router_.AddHandlerForLocalId(kPlatformReceiverId, &auth_handler_);
  }

 protected:
  void RunAuthTest(std::string serialized_crl,
                   TrustStore* fake_crl_trust_store,
                   bool should_succeed = true,
                   bool record_this_test = false) {
    std::unique_ptr<ParsedCertificate> cert;
    std::unique_ptr<TrustStore> fake_trust_store;
    InitStaticCredentialsFromFiles(
        &creds_, &cert, &fake_trust_store, data_path_ + "device_key.pem",
        data_path_ + "device_chain.pem", data_path_ + "device_tls.pem");
    creds_.device_creds.serialized_crl = std::move(serialized_crl);

    // Send an auth challenge.  |auth_handler_| will automatically respond
    // via |router_| and we will catch the result in |challenge_reply|.
    AuthContext auth_context = AuthContext::Create();
    CastMessage auth_challenge = CreateAuthChallengeMessage(auth_context);
    if (record_this_test) {
      std::string output;
      DeviceAuthMessage auth_message;
      ASSERT_EQ(auth_challenge.payload_type(),
                ::cast::channel::CastMessage_PayloadType_BINARY);
      ASSERT_TRUE(
          auth_message.ParseFromString(auth_challenge.payload_binary()));
      ASSERT_TRUE(auth_message.has_challenge());
      ASSERT_FALSE(auth_message.has_response());
      ASSERT_FALSE(auth_message.has_error());
      ASSERT_TRUE(auth_challenge.SerializeToString(&output));

      const std::string pb_path = data_path_ + "auth_challenge.pb";
      FILE* fd = fopen(pb_path.c_str(), "wb");
      ASSERT_TRUE(fd);
      ASSERT_EQ(fwrite(output.data(), 1, output.size(), fd), output.size());
      fclose(fd);
    }
    CastMessage challenge_reply;
    EXPECT_CALL(fake_cast_socket_pair_.mock_peer_client, OnMessage(_, _))
        .WillOnce(
            Invoke([&challenge_reply](CastSocket* socket, CastMessage message) {
              challenge_reply = std::move(message);
            }));
    ASSERT_TRUE(
        fake_cast_socket_pair_.peer_socket->Send(std::move(auth_challenge))
            .ok());

    if (record_this_test) {
      std::string output;
      DeviceAuthMessage auth_message;
      ASSERT_EQ(challenge_reply.payload_type(),
                ::cast::channel::CastMessage_PayloadType_BINARY);
      ASSERT_TRUE(
          auth_message.ParseFromString(challenge_reply.payload_binary()));
      ASSERT_TRUE(auth_message.has_response());
      ASSERT_FALSE(auth_message.has_challenge());
      ASSERT_FALSE(auth_message.has_error());
      ASSERT_TRUE(auth_message.response().SerializeToString(&output));

      const std::string pb_path = data_path_ + "auth_response.pb";
      FILE* fd = fopen(pb_path.c_str(), "wb");
      ASSERT_TRUE(fd);
      ASSERT_EQ(fwrite(output.data(), 1, output.size(), fd), output.size());
      fclose(fd);
    }

    DateTime December2019 = {};
    December2019.year = 2019;
    December2019.month = 12;
    December2019.day = 17;
    const ErrorOr<CastDeviceCertPolicy> error_or_policy =
        AuthenticateChallengeReplyForTest(
            challenge_reply, *cert, auth_context,
            fake_crl_trust_store ? CRLPolicy::kCrlRequired
                                 : CRLPolicy::kCrlOptional,
            fake_trust_store.get(), fake_crl_trust_store, December2019);
    EXPECT_EQ(error_or_policy.is_value(), should_succeed);
  }

  const std::string& data_path_{GetSpecificTestDataPath()};
  FakeCastSocketPair fake_cast_socket_pair_;
  MockSocketErrorHandler mock_error_handler_;
  CastSocket* socket_;

  StaticCredentialsProvider creds_;
  VirtualConnectionRouter router_;
  DeviceAuthNamespaceHandler auth_handler_{&creds_};
};

TEST_F(DeviceAuthTest, MANUAL_SerializeTestData) {
  if (::testing::GTEST_FLAG(filter) ==
      "DeviceAuthTest.MANUAL_SerializeTestData") {
    RunAuthTest(std::string(), nullptr, true, true);
  }
}

TEST_F(DeviceAuthTest, AuthIntegration) {
  RunAuthTest(std::string(), nullptr);
}

TEST_F(DeviceAuthTest, GoodCrl) {
  auto fake_crl_trust_store =
      TrustStore::CreateInstanceFromPemFile(data_path_ + "crl_root.pem");
  RunAuthTest(ReadEntireFileToString(data_path_ + "good_crl.pb"),
              fake_crl_trust_store.get());
}

TEST_F(DeviceAuthTest, InvalidCrlTime) {
  auto fake_crl_trust_store =
      TrustStore::CreateInstanceFromPemFile(data_path_ + "crl_root.pem");
  RunAuthTest(ReadEntireFileToString(data_path_ + "invalid_time_crl.pb"),
              fake_crl_trust_store.get(), false);
}

TEST_F(DeviceAuthTest, IssuerRevoked) {
  auto fake_crl_trust_store =
      TrustStore::CreateInstanceFromPemFile(data_path_ + "crl_root.pem");
  RunAuthTest(ReadEntireFileToString(data_path_ + "issuer_revoked_crl.pb"),
              fake_crl_trust_store.get(), false);
}

TEST_F(DeviceAuthTest, DeviceRevoked) {
  auto fake_crl_trust_store =
      TrustStore::CreateInstanceFromPemFile(data_path_ + "crl_root.pem");
  RunAuthTest(ReadEntireFileToString(data_path_ + "device_revoked_crl.pb"),
              fake_crl_trust_store.get(), false);
}

TEST_F(DeviceAuthTest, IssuerSerialRevoked) {
  auto fake_crl_trust_store =
      TrustStore::CreateInstanceFromPemFile(data_path_ + "crl_root.pem");
  RunAuthTest(
      ReadEntireFileToString(data_path_ + "issuer_serial_revoked_crl.pb"),
      fake_crl_trust_store.get(), false);
}

TEST_F(DeviceAuthTest, DeviceSerialRevoked) {
  auto fake_crl_trust_store =
      TrustStore::CreateInstanceFromPemFile(data_path_ + "crl_root.pem");
  RunAuthTest(
      ReadEntireFileToString(data_path_ + "device_serial_revoked_crl.pb"),
      fake_crl_trust_store.get(), false);
}

TEST_F(DeviceAuthTest, BadCrlSignerCert) {
  auto fake_crl_trust_store =
      TrustStore::CreateInstanceFromPemFile(data_path_ + "crl_root.pem");
  RunAuthTest(ReadEntireFileToString(data_path_ + "bad_signer_cert_crl.pb"),
              fake_crl_trust_store.get(), false);
}

TEST_F(DeviceAuthTest, BadCrlSignature) {
  auto fake_crl_trust_store =
      TrustStore::CreateInstanceFromPemFile(data_path_ + "crl_root.pem");
  RunAuthTest(ReadEntireFileToString(data_path_ + "bad_signature_crl.pb"),
              fake_crl_trust_store.get(), false);
}

}  // namespace
}  // namespace cast
}  // namespace openscreen
