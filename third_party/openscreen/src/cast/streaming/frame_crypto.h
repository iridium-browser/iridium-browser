// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_FRAME_CRYPTO_H_
#define CAST_STREAMING_FRAME_CRYPTO_H_

#include <stddef.h>
#include <stdint.h>

#include <array>
#include <vector>

#include "cast/streaming/encoded_frame.h"
#include "openssl/aes.h"
#include "platform/base/macros.h"
#include "platform/base/span.h"

namespace openscreen {
namespace cast {

class FrameCollector;
class FrameCrypto;

// A subclass of EncodedFrame that represents an EncodedFrame with encrypted
// payload data, and owns the buffer storing the encrypted payload data. Use
// FrameCrypto (below) to explicitly convert between EncryptedFrames and
// EncodedFrames.
struct EncryptedFrame : public EncodedFrame {
  EncryptedFrame();
  ~EncryptedFrame();
  EncryptedFrame(EncryptedFrame&&) noexcept;
  EncryptedFrame& operator=(EncryptedFrame&&);

 protected:
  // Since only FrameCrypto and FrameCollector are trusted to generate the
  // payload data, only they are allowed direct access to the storage.
  friend class FrameCollector;
  friend class FrameCrypto;

  // Note: EncodedFrame::data must be updated whenever any mutations are
  // performed on this member!
  std::vector<uint8_t> owned_data_;
};

// Encrypts EncodedFrames before sending, or decrypts EncryptedFrames that have
// been received.
class FrameCrypto {
 public:
  // Construct with the given 16-bytes AES key and IV mask. Both arguments
  // should be randomly-generated for each new streaming session.
  // GenerateRandomBytes() can be used to create them.
  FrameCrypto(const std::array<uint8_t, 16>& aes_key,
              const std::array<uint8_t, 16>& cast_iv_mask);

  ~FrameCrypto();

  EncryptedFrame Encrypt(const EncodedFrame& encoded_frame) const;

  // Decrypts `encrypted_frame` into `out`. `out` must have a sufficiently-sized
  // data buffer (see GetPlaintextSize()).
  void Decrypt(const EncryptedFrame& encrypted_frame, ByteBuffer out) const;

  // AES crypto inputs and outputs (for either encrypting or decrypting) are
  // always the same size in bytes. The following are just "documentative code."
  static int GetEncryptedSize(const EncodedFrame& encoded_frame) {
    return encoded_frame.data.size();
  }
  static int GetPlaintextSize(const EncryptedFrame& encrypted_frame) {
    return encrypted_frame.data.size();
  }

 private:
  // The 244-byte AES_KEY struct, derived from the |aes_key| passed to the ctor,
  // and initialized by boringssl's AES_set_encrypt_key() function.
  const AES_KEY aes_key_;

  // Random bytes used in the custom heuristic to generate a different
  // initialization vector for each frame.
  const std::array<uint8_t, 16> cast_iv_mask_;

  // AES-CTR is symmetric. Thus, the "meat" of both Encrypt() and Decrypt() is
  // the same.
  void EncryptCommon(FrameId frame_id, ByteView in, ByteBuffer out) const;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_FRAME_CRYPTO_H_
