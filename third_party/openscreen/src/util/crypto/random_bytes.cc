// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/crypto/random_bytes.h"

#include "openssl/rand.h"
#include "util/osp_logging.h"

namespace openscreen {

std::array<uint8_t, 16> GenerateRandomBytes16() {
  std::array<uint8_t, 16> result;
  GenerateRandomBytes(result.begin(), result.size());
  return result;
}

void GenerateRandomBytes(uint8_t* out, int len) {
  // Working cryptography is mandatory for our library to run.
  OSP_CHECK(RAND_bytes(out, len) == 1);
}

}  // namespace openscreen
