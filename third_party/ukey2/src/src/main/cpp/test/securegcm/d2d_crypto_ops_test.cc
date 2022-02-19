// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "securegcm/d2d_crypto_ops.h"

#include "gtest/gtest.h"
#include "securemessage/crypto_ops.h"
#include "securemessage/secure_message_builder.h"

namespace securegcm {

using securemessage::CryptoOps;
using securemessage::HeaderAndBody;
using securemessage::SecureMessage;

namespace {

const char kPayloadData[] = "Test payload";
const char kSecretKeyData[] = "secret key must be 32 bytes long";
const char kOtherSecretKeyData[] = "other secret key****************";
const char kInvalidSigncryptedMessage[] = "Not a protobuf";
const int kSupportedProtocolVersion = 1;
const int kUnsupportedProtocolVersion = 2;

}  // namespace

class D2DCryptoOpsTest : public testing::Test {
 public:
  D2DCryptoOpsTest()
      : payload_(DEVICE_TO_DEVICE_MESSAGE, kPayloadData),
        secret_key_(kSecretKeyData, CryptoOps::AES_256_KEY) {}

 protected:
  const D2DCryptoOps::Payload payload_;
  const CryptoOps::SecretKey secret_key_;
};

TEST_F(D2DCryptoOpsTest, Signcrypt_EmptyPayload) {
  // Signcrypting an empty payload should fail.
  D2DCryptoOps::Payload empty_payload(DEVICE_TO_DEVICE_MESSAGE, string());
  EXPECT_FALSE(D2DCryptoOps::SigncryptPayload(empty_payload, secret_key_));
}

TEST_F(D2DCryptoOpsTest, VerifyDecrypt_InvalidMessage) {
  // VerifyDecrypting an invalid payload should fail.
  EXPECT_FALSE(D2DCryptoOps::VerifyDecryptPayload(kInvalidSigncryptedMessage,
                                                  secret_key_));
}

TEST_F(D2DCryptoOpsTest, VerifyDecrypt_NoPublicMetadata) {
  std::unique_ptr<string> signcrypted_message =
      D2DCryptoOps::SigncryptPayload(payload_, secret_key_);

  // Clear metadata field in header.
  SecureMessage secure_message;
  ASSERT_TRUE(secure_message.ParseFromString(*signcrypted_message));
  HeaderAndBody header_and_body;
  ASSERT_TRUE(
      header_and_body.ParseFromString(secure_message.header_and_body()));
  header_and_body.mutable_header()->clear_public_metadata();
  secure_message.set_header_and_body(header_and_body.SerializeAsString());

  // Decrypting the message should now fail.
  EXPECT_FALSE(D2DCryptoOps::VerifyDecryptPayload(
      secure_message.SerializeAsString(), secret_key_));
}

TEST_F(D2DCryptoOpsTest, VerifyDecrypt_ProtocolVersionNotSupported) {
  std::unique_ptr<string> signcrypted_message =
      D2DCryptoOps::SigncryptPayload(payload_, secret_key_);

  // Change version in metadata field in header to an unsupported version.
  SecureMessage secure_message;
  ASSERT_TRUE(secure_message.ParseFromString(*signcrypted_message));
  HeaderAndBody header_and_body;
  ASSERT_TRUE(
      header_and_body.ParseFromString(secure_message.header_and_body()));
  GcmMetadata metadata;
  ASSERT_TRUE(
      metadata.ParseFromString(header_and_body.header().public_metadata()));
  EXPECT_EQ(kSupportedProtocolVersion, metadata.version());
  metadata.set_version(kUnsupportedProtocolVersion);
  header_and_body.mutable_header()->set_public_metadata(
      metadata.SerializeAsString());
  secure_message.set_header_and_body(header_and_body.SerializeAsString());

  // Decrypting the message should now fail.
  EXPECT_FALSE(D2DCryptoOps::VerifyDecryptPayload(
      secure_message.SerializeAsString(), secret_key_));
}

TEST_F(D2DCryptoOpsTest, SigncryptThenVerifyDecrypt_SuccessWithSameKey) {
  // Signcrypt the payload.
  std::unique_ptr<string> signcrypted_message =
      D2DCryptoOps::SigncryptPayload(payload_, secret_key_);
  ASSERT_TRUE(signcrypted_message);

  // Decrypt the signcrypted message.
  std::unique_ptr<D2DCryptoOps::Payload> decrypted_payload =
      D2DCryptoOps::VerifyDecryptPayload(*signcrypted_message, secret_key_);
  ASSERT_TRUE(decrypted_payload);

  // Check that decrypted payload is the same.
  EXPECT_EQ(payload_.type(), decrypted_payload->type());
  EXPECT_EQ(payload_.message(), decrypted_payload->message());
}

TEST_F(D2DCryptoOpsTest, SigncryptThenVerifyDecrypt_FailsWithDifferentKey) {
  CryptoOps::SecretKey other_secret_key(kOtherSecretKeyData,
                                        CryptoOps::AES_256_KEY);

  // Signcrypt the payload with first secret key.
  std::unique_ptr<string> signcrypted_message =
      D2DCryptoOps::SigncryptPayload(payload_, secret_key_);
  ASSERT_TRUE(signcrypted_message);

  // Decrypting the signcrypted message with the other secret key should fail.
  EXPECT_FALSE(D2DCryptoOps::VerifyDecryptPayload(*signcrypted_message,
                                                  other_secret_key));
}

TEST_F(D2DCryptoOpsTest, DeriveNewKeyForPurpose_ClientServer) {
  CryptoOps::SecretKey master_key(kSecretKeyData, CryptoOps::AES_256_KEY);

  std::unique_ptr<CryptoOps::SecretKey> derived_key1 =
      D2DCryptoOps::DeriveNewKeyForPurpose(master_key, "client");
  std::unique_ptr<CryptoOps::SecretKey> derived_key2 =
      D2DCryptoOps::DeriveNewKeyForPurpose(master_key, "server");

  ASSERT_TRUE(derived_key1);
  ASSERT_TRUE(derived_key2);
  EXPECT_EQ(CryptoOps::AES_256_KEY, derived_key1->algorithm());
  EXPECT_EQ(CryptoOps::AES_256_KEY, derived_key2->algorithm());
  EXPECT_NE(derived_key1->data().String(), derived_key2->data().String());
}

TEST_F(D2DCryptoOpsTest, DeriveNewKeyForPurpose_InvalidMasterKeySize) {
  CryptoOps::SecretKey master_key("Invalid Size", CryptoOps::AES_256_KEY);
  EXPECT_FALSE(D2DCryptoOps::DeriveNewKeyForPurpose(master_key, "purpose"));
}

TEST_F(D2DCryptoOpsTest, DeriveNewKeyForPurpose_PurposeEmpty) {
  CryptoOps::SecretKey master_key(kSecretKeyData, CryptoOps::AES_256_KEY);
  EXPECT_FALSE(D2DCryptoOps::DeriveNewKeyForPurpose(master_key, string()));
}

}  // namespace securegcm
