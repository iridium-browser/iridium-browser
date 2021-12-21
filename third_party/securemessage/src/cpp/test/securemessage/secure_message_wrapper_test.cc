/*
 * Copyright 2021 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "securemessage/secure_message_wrapper.h"

#include <gtest/gtest.h>

#include "proto/securemessage.pb.h"

static const ::securemessage::SigScheme kSignatureScheme =
    ::securemessage::SigScheme::ECDSA_P256_SHA256;
static const ::securemessage::EncScheme kEncryptionScheme =
    ::securemessage::EncScheme::AES_256_CBC;
static const char* kVerificationKeyId = "test verification key id";
static const char* kDecryptionKeyId = "test decryption key id";
static const char* kIV = "test iv";
static const char* kPublicMetaData = "test metadata";
static const char* kBody = "test body";
static const char* kHeader = "test header";
static const char* kAssociatedData = "test associated data";
static const char* kInvalidHeaderAndBody = "test invalid header and body";

using std::unique_ptr;

namespace securemessage {
namespace {

class SecureMessageWrapperTest : public testing::Test {
 protected:
  unique_ptr<Header> complete_header_;
  unique_ptr<Header> partial_header_;
  unique_ptr<HeaderAndBody> complete_header_and_body_;
  unique_ptr<HeaderAndBody> partial_header_and_body_;

  virtual void SetUp() {
    complete_header_.reset(new Header());
    complete_header_->set_signature_scheme(kSignatureScheme);
    complete_header_->set_encryption_scheme(kEncryptionScheme);
    complete_header_->set_verification_key_id(kVerificationKeyId);
    complete_header_->set_decryption_key_id(kDecryptionKeyId);
    complete_header_->set_iv(kIV);
    complete_header_->set_public_metadata(kPublicMetaData);
    complete_header_->set_associated_data_length(strlen(kAssociatedData));

    partial_header_.reset(new Header());
    partial_header_->set_verification_key_id(kVerificationKeyId);
    partial_header_->set_decryption_key_id(kDecryptionKeyId);
    partial_header_->set_iv(kIV);
    partial_header_->set_public_metadata(kPublicMetaData);
    partial_header_->set_associated_data_length(strlen(kAssociatedData));

    complete_header_and_body_.reset(new HeaderAndBody());
    complete_header_and_body_->set_body(kBody);
    complete_header_and_body_->set_allocated_header(
        new Header(*complete_header_));

    partial_header_and_body_.reset(new HeaderAndBody());
    partial_header_and_body_->set_body(kBody);
    partial_header_and_body_->set_allocated_header(
        new Header(*partial_header_));
  }

  virtual void TearDown() {
    complete_header_->Clear();
    partial_header_->Clear();
    complete_header_and_body_->Clear();
    partial_header_and_body_->Clear();
  }
};

TEST_F(SecureMessageWrapperTest, ParseHeaderIvComplete) {
  std::string header_and_body_bytes =
      complete_header_and_body_->SerializeAsString();
  EXPECT_EQ(*SecureMessageWrapper::ParseHeaderIv(header_and_body_bytes), kIV);
}

TEST_F(SecureMessageWrapperTest, ParseHeaderIvPartial) {
  std::string header_and_body_bytes =
      partial_header_and_body_->SerializePartialAsString();
  EXPECT_EQ(SecureMessageWrapper::ParseHeaderIv(header_and_body_bytes),
            nullptr);
}

TEST_F(SecureMessageWrapperTest, ParseHeaderIvInvalid) {
  EXPECT_EQ(SecureMessageWrapper::ParseHeaderIv(kInvalidHeaderAndBody),
            nullptr);
}

TEST_F(SecureMessageWrapperTest, ParseHeaderComplete) {
  std::string header_and_body_bytes =
      complete_header_and_body_->SerializeAsString();
  EXPECT_EQ(*SecureMessageWrapper::ParseHeader(header_and_body_bytes),
            complete_header_->SerializeAsString());
}

TEST_F(SecureMessageWrapperTest, ParseHeaderPartial) {
  std::string header_and_body_bytes =
      partial_header_and_body_->SerializePartialAsString();
  EXPECT_EQ(SecureMessageWrapper::ParseHeader(header_and_body_bytes), nullptr);
}

TEST_F(SecureMessageWrapperTest, ParseHeaderInvalid) {
  EXPECT_EQ(SecureMessageWrapper::ParseHeader(kInvalidHeaderAndBody), nullptr);
}

TEST_F(SecureMessageWrapperTest, ParseInternalHeaderComplete) {
  HeaderAndBodyInternal header_and_body_internal;
  header_and_body_internal.set_header(kHeader);
  header_and_body_internal.set_body(kBody);

  std::string header_and_body_internal_bytes =
      header_and_body_internal.SerializeAsString();
  EXPECT_EQ(*SecureMessageWrapper::ParseInternalHeader(
                header_and_body_internal_bytes),
            kHeader);
}

TEST_F(SecureMessageWrapperTest, ParseInternalHeaderPartial) {
  HeaderAndBodyInternal header_and_body_internal;
  header_and_body_internal.set_header(kHeader);

  std::string header_and_body_internal_bytes =
      header_and_body_internal.SerializePartialAsString();
  EXPECT_EQ(
      SecureMessageWrapper::ParseInternalHeader(header_and_body_internal_bytes),
      nullptr);
}

TEST_F(SecureMessageWrapperTest, ParseInternalHeaderInvalid) {
  EXPECT_EQ(SecureMessageWrapper::ParseInternalHeader(kInvalidHeaderAndBody),
            nullptr);
}

TEST_F(SecureMessageWrapperTest, ParseBodyComplete) {
  std::string header_and_body_bytes =
      complete_header_and_body_->SerializeAsString();
  EXPECT_EQ(*SecureMessageWrapper::ParseBody(header_and_body_bytes), kBody);
}

TEST_F(SecureMessageWrapperTest, ParseBodyPartial) {
  std::string header_and_body_bytes =
      partial_header_and_body_->SerializePartialAsString();
  EXPECT_EQ(SecureMessageWrapper::ParseBody(header_and_body_bytes), nullptr);
}

TEST_F(SecureMessageWrapperTest, ParseBodyInvalid) {
  EXPECT_EQ(SecureMessageWrapper::ParseBody(kInvalidHeaderAndBody), nullptr);
}

}  // namespace
}  // namespace securemessage
