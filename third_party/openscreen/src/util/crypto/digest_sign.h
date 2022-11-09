// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_CRYPTO_DIGEST_SIGN_H_
#define UTIL_CRYPTO_DIGEST_SIGN_H_

#include <openssl/evp.h>

#include <string>

#include "absl/types/span.h"
#include "platform/base/error.h"

namespace openscreen {

ErrorOr<std::string> SignData(const EVP_MD* digest,
                              EVP_PKEY* private_key,
                              absl::Span<const uint8_t> data);

}  // namespace openscreen

#endif  // UTIL_CRYPTO_DIGEST_SIGN_H_
