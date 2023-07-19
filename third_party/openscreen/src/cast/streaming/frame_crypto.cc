// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/frame_crypto.h"

#include <random>
#include <utility>

#include "openssl/crypto.h"
#include "openssl/err.h"
#include "openssl/rand.h"
#include "platform/base/span.h"
#include "util/big_endian.h"
#include "util/crypto/openssl_util.h"
#include "util/crypto/random_bytes.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {

EncryptedFrame::EncryptedFrame() {
  data = owned_data_;
}

EncryptedFrame::~EncryptedFrame() = default;

EncryptedFrame::EncryptedFrame(EncryptedFrame&& other) noexcept
    : EncodedFrame(static_cast<EncodedFrame&&>(other)),
      owned_data_(std::move(other.owned_data_)) {
  data = owned_data_;
  other.data = ByteView();
}

EncryptedFrame& EncryptedFrame::operator=(EncryptedFrame&& other) {
  this->EncodedFrame::operator=(static_cast<EncodedFrame&&>(other));
  owned_data_ = std::move(other.owned_data_);
  data = owned_data_;
  other.data = ByteView();
  return *this;
}

FrameCrypto::FrameCrypto(const std::array<uint8_t, 16>& aes_key,
                         const std::array<uint8_t, 16>& cast_iv_mask)
    : aes_key_{}, cast_iv_mask_(cast_iv_mask) {
  // Ensure that the library has been initialized. CRYPTO_library_init() may be
  // safely called multiple times during the life of a process.
  CRYPTO_library_init();

  // Initialize the 244-byte AES_KEY struct once, here at construction time. The
  // const_cast<> is reasonable as this is a one-time-ctor-initialized value
  // that will remain constant from here onward.
  const int return_code = AES_set_encrypt_key(
      aes_key.data(), aes_key.size() * 8, const_cast<AES_KEY*>(&aes_key_));
  if (return_code != 0) {
    ClearOpenSSLERRStack(CURRENT_LOCATION);
    OSP_LOG_FATAL << "Failure when setting encryption key; unsafe to continue.";
    OSP_NOTREACHED();
  }
}

FrameCrypto::~FrameCrypto() = default;

EncryptedFrame FrameCrypto::Encrypt(const EncodedFrame& encoded_frame) const {
  EncryptedFrame result;
  encoded_frame.CopyMetadataTo(&result);
  result.owned_data_.resize(encoded_frame.data.size());
  result.data = result.owned_data_;
  EncryptCommon(encoded_frame.frame_id, encoded_frame.data, result.owned_data_);
  return result;
}

void FrameCrypto::Decrypt(const EncryptedFrame& encrypted_frame,
                          ByteBuffer out) const {
  // AES-CTC is symmetric. Thus, decryption back to the plaintext is the same as
  // encrypting the ciphertext; and both are the same size.
  OSP_DCHECK_EQ(encrypted_frame.data.size(), out.size());
  EncryptCommon(encrypted_frame.frame_id, encrypted_frame.data, out);
}

void FrameCrypto::EncryptCommon(FrameId frame_id,
                                ByteView in,
                                ByteBuffer out) const {
  OSP_DCHECK(!frame_id.is_null());
  OSP_DCHECK_EQ(in.size(), out.size());

  // Compute the AES nonce for Cast Streaming payload encryption, which is based
  // on the |frame_id|.
  std::array<uint8_t, 16> aes_nonce{/* zero initialized */};
  static_assert(AES_BLOCK_SIZE == sizeof(aes_nonce),
                "AES_BLOCK_SIZE is not 16 bytes.");
  WriteBigEndian<uint32_t>(frame_id.lower_32_bits(), aes_nonce.data() + 8);
  for (size_t i = 0; i < aes_nonce.size(); ++i) {
    aes_nonce[i] ^= cast_iv_mask_[i];
  }

  std::array<uint8_t, 16> ecount_buf{/* zero initialized */};
  unsigned int block_offset = 0;
  AES_ctr128_encrypt(in.data(), out.data(), in.size(), &aes_key_,
                     aes_nonce.data(), ecount_buf.data(), &block_offset);
}

}  // namespace cast
}  // namespace openscreen
