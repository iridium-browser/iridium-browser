// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_CRYPTO_SECURE_HASH_H_
#define UTIL_CRYPTO_SECURE_HASH_H_

#include <stddef.h>

#include <memory>
#include <string>

#include "openssl/base.h"
#include "openssl/evp.h"
#include "platform/base/macros.h"

namespace openscreen {

// A wrapper to calculate secure hashes incrementally, allowing to
// be used when the full input is not known in advance. The end result will the
// same as if we have the full input in advance.
class SecureHash {
 public:
  explicit SecureHash(const EVP_MD* type);
  SecureHash(const SecureHash& other);
  SecureHash(SecureHash&& other) noexcept;
  SecureHash& operator=(const SecureHash& other);
  SecureHash& operator=(SecureHash&& other) noexcept;

  ~SecureHash();

  void Update(const uint8_t* input, size_t len);
  void Finish(uint8_t* output);

  // Handy versions that do the kludgy casting to unsigned in the background.
  void Update(const std::string& input);
  void Finish(char* output);

  size_t GetHashLength() const;

 private:
  bssl::UniquePtr<EVP_MD_CTX> ctx_ =
      bssl::UniquePtr<EVP_MD_CTX>(EVP_MD_CTX_new());
};

}  // namespace openscreen

#endif  // UTIL_CRYPTO_SECURE_HASH_H_
