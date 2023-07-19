// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/test/byte_view_test_util.h"

#include <cstring>

#include "gtest/gtest.h"

namespace openscreen {
namespace {

bool ExpectSameLength(const ByteView& first, const ByteView& second) {
  EXPECT_FALSE(first.empty());
  EXPECT_FALSE(second.empty());
  EXPECT_EQ(first.size(), second.size())
      << "Expected sizes to be equal (" << first.size() << " vs "
      << second.size() << ")";
  return first.size() == second.size();
}

}  // namespace

void ExpectByteViewsHaveSameBytes(const ByteView& first,
                                  const ByteView& second) {
  if (!ExpectSameLength(first, second))
    return;
  EXPECT_EQ(std::memcmp(first.data(), second.data(), first.size()), 0)
      << "Expected bytes to be equal";
}

void ExpectByteViewsHaveDifferentBytes(const ByteView& first,
                                       const ByteView& second) {
  if (!ExpectSameLength(first, second))
    return;
  EXPECT_NE(std::memcmp(first.data(), second.data(), first.size()), 0)
      << "Expected bytes to be different";
}

ByteView ByteViewFromLiteral(const char* literal) {
  return ByteView{reinterpret_cast<const uint8_t* const>(literal),
                  sizeof(literal) - 1};
}

}  // namespace openscreen
