// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/crypto/secure_hash.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "openssl/evp.h"
#include "openssl/sha.h"

namespace openscreen {
TEST(SecureHashTest, TestUpdate) {
  // Example B.3 from FIPS 180-2: long message.
  std::string input3(500000, 'a');  // 'a' repeated half a million times
  const uint8_t kExpectedHashOfInput3[] = {
      0xcd, 0xc7, 0x6e, 0x5c, 0x99, 0x14, 0xfb, 0x92, 0x81, 0xa1, 0xc7,
      0xe2, 0x84, 0xd7, 0x3e, 0x67, 0xf1, 0x80, 0x9a, 0x48, 0xa4, 0x97,
      0x20, 0x0e, 0x04, 0x6d, 0x39, 0xcc, 0xc7, 0x11, 0x2c, 0xd0};

  SecureHash ctx(EVP_sha256());
  std::vector<uint8_t> output3(ctx.GetHashLength());
  ctx.Update(input3);
  ctx.Update(input3);
  ctx.Finish(output3.data());
  EXPECT_THAT(output3, testing::ElementsAreArray(kExpectedHashOfInput3));
}

TEST(SecureHashTest, TestCopyable) {
  std::string input1(10001, 'a');  // 'a' repeated 10001 times
  std::string input2(10001, 'd');  // 'd' repeated 10001 times

  const uint8_t kExpectedHashOfInput1[SHA256_DIGEST_LENGTH] = {
      0x0c, 0xab, 0x99, 0xa0, 0x58, 0x60, 0x0f, 0xfa, 0xad, 0x12, 0x92,
      0xd0, 0xc5, 0x3c, 0x05, 0x48, 0xeb, 0xaf, 0x88, 0xdd, 0x1d, 0x01,
      0x03, 0x03, 0x45, 0x70, 0x5f, 0x01, 0x8a, 0x81, 0x39, 0x09};
  const uint8_t kExpectedHashOfInput1And2[SHA256_DIGEST_LENGTH] = {
      0x4c, 0x8e, 0x26, 0x5a, 0xc3, 0x85, 0x1f, 0x1f, 0xa5, 0x04, 0x1c,
      0xc7, 0x88, 0x53, 0x1c, 0xc7, 0x80, 0x47, 0x15, 0xfb, 0x47, 0xff,
      0x72, 0xb1, 0x28, 0x37, 0xb0, 0x4d, 0x6e, 0x22, 0x2e, 0x4d};

  SecureHash ctx1(EVP_sha256());
  std::vector<uint8_t> output1(ctx1.GetHashLength());
  ctx1.Update(input1);

  SecureHash ctx2 = ctx1;
  std::vector<uint8_t> output2(ctx2.GetHashLength());

  SecureHash ctx3 = ctx1;
  std::vector<uint8_t> output3(ctx3.GetHashLength());

  // At this point, ctx1, ctx2, and ctx3 are all equivalent and represent the
  // state after hashing input1.

  // Updating ctx1 and ctx2 with input2 should produce equivalent results.
  ctx1.Update(input2);
  ctx1.Finish(output1.data());

  ctx2.Update(input2);
  ctx2.Finish(output2.data());

  EXPECT_THAT(output1, testing::ElementsAreArray(output2));
  EXPECT_THAT(output1, testing::ElementsAreArray(kExpectedHashOfInput1And2));

  // Finish() ctx3, which should produce the hash of input1.
  ctx3.Finish(output3.data());
  EXPECT_THAT(output3, testing::ElementsAreArray(kExpectedHashOfInput1));
}

TEST(SecureHashTest, TestLength) {
  SecureHash ctx(EVP_sha256());
  EXPECT_EQ(static_cast<size_t>(SHA256_DIGEST_LENGTH), ctx.GetHashLength());
}

TEST(SecureHashTest, Equality) {
  std::string input1(10001, 'a');  // 'a' repeated 10001 times
  std::string input2(10001, 'd');  // 'd' repeated 10001 times

  // Call Update() twice on input1 and input2.
  SecureHash ctx1(EVP_sha256());
  std::vector<uint8_t> output1(ctx1.GetHashLength());
  ctx1.Update(input1);
  ctx1.Update(input2);
  ctx1.Finish(output1.data());

  // Call Update() once one input1 + input2 (concatenation).
  SecureHash ctx2(EVP_sha256());
  std::vector<uint8_t> output2(ctx2.GetHashLength());
  std::string input3 = input1 + input2;
  ctx2.Update(input3);
  ctx2.Finish(output2.data());

  // The hash should be the same.
  EXPECT_THAT(output1, testing::ElementsAreArray(output2));
}
}  // namespace openscreen
