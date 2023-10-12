// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_CRYPTO_DIGEST_SIGN_H_
#define UTIL_CRYPTO_DIGEST_SIGN_H_

#include <openssl/evp.h>

#include <string>

#include "platform/base/error.h"
#include "platform/base/span.h"

namespace openscreen {

ErrorOr<std::string> SignData(const EVP_MD* digest,
                              EVP_PKEY* private_key,
                              ByteView data);

}  // namespace openscreen

#endif  // UTIL_CRYPTO_DIGEST_SIGN_H_
