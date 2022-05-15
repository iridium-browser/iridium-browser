// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/crypto/digest_sign.h"

namespace openscreen {

ErrorOr<std::string> SignData(const EVP_MD* digest,
                              EVP_PKEY* private_key,
                              absl::Span<const uint8_t> data) {
  bssl::ScopedEVP_MD_CTX ctx;
  if (!EVP_DigestSignInit(ctx.get(), nullptr, digest, nullptr, private_key)) {
    return Error::Code::kEVPInitializationError;
  }
  size_t signature_length = 0;
  if ((EVP_DigestSign(ctx.get(), nullptr, &signature_length, data.data(),
                      data.size()) != 1) ||
      signature_length == 0) {
    return Error::Code::kEVPInitializationError;
  }

  std::string signature(signature_length, 0);
  if (EVP_DigestSign(ctx.get(), reinterpret_cast<uint8_t*>(&signature[0]),
                     &signature_length, data.data(), data.size()) != 1) {
    return Error::Code::kCreateSignatureFailed;
  }
  return signature;
}

}  // namespace openscreen
