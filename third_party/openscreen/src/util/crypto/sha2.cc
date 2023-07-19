// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/crypto/sha2.h"

#include <stddef.h>

#include <memory>

#include "util/crypto/secure_hash.h"
#include "util/std_util.h"

namespace openscreen {

Error SHA256HashString(absl::string_view str,
                       uint8_t output[SHA256_DIGEST_LENGTH]) {
  bssl::UniquePtr<EVP_MD_CTX> context(EVP_MD_CTX_new());
  if (!EVP_Digest(str.data(), str.size(), output, nullptr, EVP_sha256(),
                  nullptr)) {
    return Error::Code::kSha256HashFailure;
  }

  return Error::None();
}

ErrorOr<std::string> SHA256HashString(absl::string_view str) {
  std::string output(SHA256_DIGEST_LENGTH, 0);
  const Error error =
      SHA256HashString(str, reinterpret_cast<uint8_t*>(data(output)));
  if (error != Error::None()) {
    return error;
  }

  return output;
}

}  // namespace openscreen
