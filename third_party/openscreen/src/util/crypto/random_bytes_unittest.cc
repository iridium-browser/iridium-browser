// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/crypto/random_bytes.h"

#include <algorithm>
#include <utility>

#include "gtest/gtest.h"

namespace openscreen {
namespace {

struct NonZero {
  NonZero() = default;
  bool operator()(int n) const { return n > 0; }
};

}  // namespace

TEST(RandomBytesTest, CanGenerateRandomBytes) {
  std::array<uint8_t, 4> bytes;
  GenerateRandomBytes(bytes.begin(), bytes.size());

  NonZero pred;
  ASSERT_TRUE(std::any_of(bytes.begin(), bytes.end(), pred));
}

TEST(RandomBytesTest, CanGenerate16RandomBytes) {
  std::array<uint8_t, 16> bytes = GenerateRandomBytes16();

  NonZero pred;
  ASSERT_TRUE(std::any_of(bytes.begin(), bytes.end(), pred));
}

TEST(RandomBytesTest, KeysAreNotIdentical) {
  constexpr int kNumKeys = 100;
  constexpr int kKeyLength = 100;
  std::array<std::array<uint8_t, kKeyLength>, kNumKeys> keys;
  for (int i = 0; i < kNumKeys; ++i) {
    GenerateRandomBytes(keys[i].begin(), kKeyLength);
  }

  std::sort(std::begin(keys), std::end(keys));
  ASSERT_TRUE(std::adjacent_find(std::begin(keys), std::end(keys)) ==
              std::end(keys));
}

}  // namespace openscreen
