// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/crypto/secure_hash.h"

#include <stddef.h>

#include <cstring>

#include "openssl/mem.h"
#include "util/crypto/openssl_util.h"
#include "util/osp_logging.h"

namespace openscreen {

SecureHash::SecureHash(const EVP_MD* type) {
  EVP_DigestInit(ctx_.get(), type);
}

SecureHash::SecureHash(const SecureHash& other) {
  *this = other;
}

SecureHash& SecureHash::operator=(const SecureHash& other) {
  EVP_MD_CTX_copy_ex(this->ctx_.get(), other.ctx_.get());
  return *this;
}

SecureHash::SecureHash(SecureHash&& other) noexcept = default;
SecureHash& SecureHash::operator=(SecureHash&& other) noexcept = default;

SecureHash::~SecureHash() = default;

void SecureHash::Update(const uint8_t* input, size_t len) {
  EVP_DigestUpdate(ctx_.get(), input, len);
}

void SecureHash::Finish(uint8_t* output) {
  EVP_DigestFinal(ctx_.get(), output, nullptr);
}

void SecureHash::Update(const std::string& input) {
  Update(reinterpret_cast<const uint8_t*>(input.data()), input.length());
}

void SecureHash::Finish(char* output) {
  Finish(reinterpret_cast<uint8_t*>(output));
}

size_t SecureHash::GetHashLength() const {
  return EVP_MD_CTX_size(ctx_.get());
}

}  // namespace openscreen
