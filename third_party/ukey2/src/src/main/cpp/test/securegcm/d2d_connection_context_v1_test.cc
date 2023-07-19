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

#include "securegcm/d2d_connection_context_v1.h"

#include "securegcm/d2d_crypto_ops.h"
#include "gtest/gtest.h"

namespace securegcm {

using securemessage::CryptoOps;

namespace {

// The encode and decode keys should be 32 bytes.
const char kEncodeKeyData[] = "initiator_encode_key_for_aes_256";
const char kDecodeKeyData[] = "initiator_decode_key_for_aes_256";

}  // namespace

// A friend to access the private variables of D2DConnectionContextV1.
class D2DConnectionContextV1Peer {
 public:
  explicit D2DConnectionContextV1Peer(const std::string& savedSessionInfo) {
    context_ = D2DConnectionContextV1::FromSavedSession(savedSessionInfo);
  }

  D2DConnectionContextV1* GetContext() { return context_.get(); }

  uint32_t GetEncodeSequenceNumber() {
    return context_->encode_sequence_number_;
  }

  uint32_t GetDecodeSequenceNumber() {
    return context_->decode_sequence_number_;
  }

 private:
  std::unique_ptr<D2DConnectionContextV1> context_;
};

TEST(D2DConnectionContextionV1Test, SaveSession) {
  CryptoOps::SecretKey encodeKey = CryptoOps::SecretKey(
      kEncodeKeyData, CryptoOps::KeyAlgorithm::AES_256_KEY);
  CryptoOps::SecretKey decodeKey = CryptoOps::SecretKey(
      kDecodeKeyData, CryptoOps::KeyAlgorithm::AES_256_KEY);

  D2DConnectionContextV1 initiator =
      D2DConnectionContextV1(encodeKey, decodeKey, 0, 1);
  D2DConnectionContextV1 responder =
      D2DConnectionContextV1(decodeKey, encodeKey, 1, 0);

  std::unique_ptr<std::string> initiatorSavedSessionState =
      initiator.SaveSession();
  std::unique_ptr<std::string> responderSavedSessionState =
      responder.SaveSession();

  D2DConnectionContextV1Peer restoredInitiator =
      D2DConnectionContextV1Peer(*initiatorSavedSessionState);
  D2DConnectionContextV1Peer restoredResponder =
      D2DConnectionContextV1Peer(*responderSavedSessionState);

  // Verify internal state matches initialization.
  EXPECT_EQ(0, restoredInitiator.GetEncodeSequenceNumber());
  EXPECT_EQ(1, restoredInitiator.GetDecodeSequenceNumber());
  EXPECT_EQ(1, restoredResponder.GetEncodeSequenceNumber());
  EXPECT_EQ(0, restoredResponder.GetDecodeSequenceNumber());

  EXPECT_EQ(*restoredInitiator.GetContext()->GetSessionUnique(),
            *restoredResponder.GetContext()->GetSessionUnique());

  const std::string message = "ping";

  // Ensure that they can still talk to one another.
  std::string encodedMessage =
      *restoredInitiator.GetContext()->EncodeMessageToPeer(message);
  std::string decodedMessage =
      *restoredResponder.GetContext()->DecodeMessageFromPeer(encodedMessage);

  EXPECT_EQ(message, decodedMessage);

  encodedMessage =
      *restoredResponder.GetContext()->EncodeMessageToPeer(message);
  decodedMessage =
      *restoredInitiator.GetContext()->DecodeMessageFromPeer(encodedMessage);

  EXPECT_EQ(message, decodedMessage);
}

TEST(D2DConnectionContextionV1Test, SaveSession_TooShort) {
  CryptoOps::SecretKey encodeKey = CryptoOps::SecretKey(
      kEncodeKeyData, CryptoOps::KeyAlgorithm::AES_256_KEY);
  CryptoOps::SecretKey decodeKey = CryptoOps::SecretKey(
      kDecodeKeyData, CryptoOps::KeyAlgorithm::AES_256_KEY);

  D2DConnectionContextV1 initiator =
      D2DConnectionContextV1(encodeKey, decodeKey, 0, 1);

  std::unique_ptr<std::string> initiatorSavedSessionState =
      initiator.SaveSession();

  // Try to rebuild the context with a shorter session state.
  std::string shortSessionState = initiatorSavedSessionState->substr(
      0, initiatorSavedSessionState->size() - 1);

  D2DConnectionContextV1Peer restoredInitiator =
      D2DConnectionContextV1Peer(shortSessionState);

  // nullptr is returned on error. It should not crash.
  EXPECT_EQ(restoredInitiator.GetContext(), nullptr);
}

}  // namespace securegcm
