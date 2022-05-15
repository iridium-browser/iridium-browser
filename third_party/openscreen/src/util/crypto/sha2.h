// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_CRYPTO_SHA2_H_
#define UTIL_CRYPTO_SHA2_H_

#include <openssl/sha.h>
#include <stddef.h>

#include <string>

#include "absl/strings/string_view.h"
#include "platform/base/error.h"

namespace openscreen {

// These functions perform SHA-256 operations.
//
// Functions for SHA-384 and SHA-512 can be added when the need arises.

// Computes the SHA-256 hash of the input string 'str' and stores the first
// 'len' bytes of the hash in the output buffer 'output'.  If 'len' > 32,
// only 32 bytes (the full hash) are stored in the 'output' buffer.
Error SHA256HashString(absl::string_view str,
                       uint8_t output[SHA256_DIGEST_LENGTH]);

// Convenience version of the above that returns the result in a 32-byte
// string.
ErrorOr<std::string> SHA256HashString(absl::string_view str);

}  // namespace openscreen

#endif  // UTIL_CRYPTO_SHA2_H_
