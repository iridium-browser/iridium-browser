// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/base/byte_view.h"

#include "gtest/gtest.h"

namespace {

constexpr char kSampleBytes[] = "googleplex";
const uint8_t* kSampleData =
    reinterpret_cast<const uint8_t* const>(&kSampleBytes[0]);
constexpr size_t kSampleSize =
    sizeof(kSampleBytes) - 1;  // Ignore null terminator.

}  // namespace

namespace openscreen {

TEST(ByteViewTest, TestBasics) {
  ByteView nullView;
  EXPECT_EQ(nullView.data(), nullptr);
  EXPECT_EQ(nullView.size(), size_t{0});
  EXPECT_TRUE(nullView.empty());

  ByteView googlePlex = ByteView(kSampleData, kSampleSize);
  EXPECT_EQ(googlePlex.data(), kSampleData);
  EXPECT_EQ(googlePlex.size(), kSampleSize);
  EXPECT_FALSE(googlePlex.empty());

  EXPECT_EQ(googlePlex[0], 'g');
  EXPECT_EQ(googlePlex[9], 'x');

  ByteView copyBytes = googlePlex;
  EXPECT_EQ(copyBytes.data(), googlePlex.data());
  EXPECT_EQ(copyBytes.size(), googlePlex.size());

  ByteView firstBytes(googlePlex.first(4));
  EXPECT_EQ(firstBytes.data(), googlePlex.data());
  EXPECT_EQ(firstBytes.size(), size_t{4});
  EXPECT_EQ(firstBytes[0], 'g');
  EXPECT_EQ(firstBytes[3], 'g');

  ByteView lastBytes(googlePlex.last(4));
  EXPECT_EQ(lastBytes.data(), googlePlex.data() + 6);
  EXPECT_EQ(lastBytes.size(), size_t{4});
  EXPECT_EQ(lastBytes[0], 'p');
  EXPECT_EQ(lastBytes[3], 'x');

  ByteView middleBytes(googlePlex.subspan(2, 4));
  EXPECT_EQ(middleBytes.data(), googlePlex.data() + 2);
  EXPECT_EQ(middleBytes.size(), size_t{4});
  EXPECT_EQ(middleBytes[0], 'o');
  EXPECT_EQ(middleBytes[3], 'e');
}

TEST(ByteViewTest, TestIterators) {
  ByteView googlePlex = ByteView(kSampleData, kSampleSize);
  size_t idx = 0;

  for (const uint8_t* it = googlePlex.begin(); it != googlePlex.end(); it++) {
    EXPECT_EQ(*it, kSampleBytes[idx]);
    idx++;
  }
}

TEST(ByteViewTest, TestRemove) {
  ByteView googlePlex = ByteView(kSampleData, kSampleSize);

  googlePlex.remove_prefix(2);
  EXPECT_EQ(googlePlex.size(), size_t{8});
  EXPECT_EQ(googlePlex[0], 'o');

  googlePlex.remove_suffix(2);
  EXPECT_EQ(googlePlex.size(), size_t{6});
  EXPECT_EQ(googlePlex[5], 'l');
}

}  // namespace openscreen
