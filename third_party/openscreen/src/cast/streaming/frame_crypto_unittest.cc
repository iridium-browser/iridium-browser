// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/frame_crypto.h"

#include <array>
#include <cstring>
#include <vector>

#include "gtest/gtest.h"
#include "platform/base/span.h"
#include "platform/test/byte_view_test_util.h"
#include "util/crypto/random_bytes.h"

namespace openscreen {
namespace cast {
namespace {

TEST(FrameCryptoTest, EncryptsAndDecryptsFrames) {
  // Prepare two frames with different FrameIds, but having the same payload
  // bytes.
  EncodedFrame frame0;
  frame0.frame_id = FrameId::first();
  const char kPayload[] = "The quick brown fox jumps over the lazy dog.";
  std::vector<uint8_t> buffer(
      reinterpret_cast<const uint8_t*>(kPayload),
      reinterpret_cast<const uint8_t*>(kPayload) + sizeof(kPayload));
  frame0.data = buffer;
  EncodedFrame frame1;
  frame1.frame_id = frame0.frame_id + 1;
  frame1.data = frame0.data;

  const std::array<uint8_t, 16> key = GenerateRandomBytes16();
  const std::array<uint8_t, 16> iv = GenerateRandomBytes16();
  EXPECT_NE(0, memcmp(key.data(), iv.data(), sizeof(key)));
  const FrameCrypto crypto(key, iv);

  // Encrypt both frames, and confirm the encrypted data is something other than
  // the plaintext, and that both frames have different encrypted data.
  const EncryptedFrame encrypted_frame0 = crypto.Encrypt(frame0);
  EXPECT_EQ(frame0.frame_id, encrypted_frame0.frame_id);
  ASSERT_EQ(static_cast<int>(frame0.data.size()),
            FrameCrypto::GetPlaintextSize(encrypted_frame0));
  ExpectByteViewsHaveDifferentBytes(frame0.data, encrypted_frame0.data);
  const EncryptedFrame encrypted_frame1 = crypto.Encrypt(frame1);
  EXPECT_EQ(frame1.frame_id, encrypted_frame1.frame_id);
  ASSERT_EQ(static_cast<int>(frame1.data.size()),
            FrameCrypto::GetPlaintextSize(encrypted_frame1));
  ExpectByteViewsHaveDifferentBytes(frame1.data, encrypted_frame1.data);
  ExpectByteViewsHaveDifferentBytes(encrypted_frame0.data,
                                    encrypted_frame1.data);

  // Now, decrypt the encrypted frames, and confirm the original payload
  // plaintext is retrieved.
  std::vector<uint8_t> decrypted_frame0_buffer(
      FrameCrypto::GetPlaintextSize(encrypted_frame0));
  crypto.Decrypt(encrypted_frame0, decrypted_frame0_buffer);
  EncodedFrame decrypted_frame0;
  encrypted_frame0.CopyMetadataTo(&decrypted_frame0);
  decrypted_frame0.data = decrypted_frame0_buffer;
  EXPECT_EQ(frame0.frame_id, decrypted_frame0.frame_id);
  ExpectByteViewsHaveSameBytes(frame0.data, decrypted_frame0.data);

  std::vector<uint8_t> decrypted_frame1_buffer(
      FrameCrypto::GetPlaintextSize(encrypted_frame1));
  crypto.Decrypt(encrypted_frame1, decrypted_frame1_buffer);
  EncodedFrame decrypted_frame1;
  encrypted_frame1.CopyMetadataTo(&decrypted_frame1);
  decrypted_frame1.data = decrypted_frame1_buffer;
  EXPECT_EQ(frame1.frame_id, decrypted_frame1.frame_id);
  ExpectByteViewsHaveSameBytes(frame1.data, decrypted_frame1.data);
}

}  // namespace
}  // namespace cast
}  // namespace openscreen
