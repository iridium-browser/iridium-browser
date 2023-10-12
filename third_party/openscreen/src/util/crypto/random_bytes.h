// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_CRYPTO_RANDOM_BYTES_H_
#define UTIL_CRYPTO_RANDOM_BYTES_H_

#include <array>
#include <cstdint>

namespace openscreen {

std::array<uint8_t, 16> GenerateRandomBytes16();
void GenerateRandomBytes(uint8_t* out, int len);

}  // namespace openscreen

#endif  // UTIL_CRYPTO_RANDOM_BYTES_H_
