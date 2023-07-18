// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/base/span.h"

#include <stdint.h>

#include <array>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

namespace {

constexpr char kSampleBytes[] = "googleplex";
const uint8_t* kSampleData =
    reinterpret_cast<const uint8_t* const>(&kSampleBytes[0]);
constexpr size_t kSampleSize =
    sizeof(kSampleBytes) - 1;  // Ignore null terminator.

}  // namespace

namespace openscreen {

TEST(SpanTest, TestBasics) {
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

TEST(SpanTest, TestIterators) {
  ByteView googlePlex = ByteView(kSampleData, kSampleSize);
  size_t idx = 0;

  for (const uint8_t* it = googlePlex.begin(); it != googlePlex.end(); it++) {
    EXPECT_EQ(*it, kSampleBytes[idx]);
    idx++;
  }
}

TEST(SpanTest, TestRemove) {
  ByteView googlePlex = ByteView(kSampleData, kSampleSize);

  googlePlex.remove_prefix(2);
  EXPECT_EQ(googlePlex.size(), size_t{8});
  EXPECT_EQ(googlePlex[0], 'o');

  googlePlex.remove_suffix(2);
  EXPECT_EQ(googlePlex.size(), size_t{6});
  EXPECT_EQ(googlePlex[5], 'l');
}

TEST(SpanTest, ConstConversions) {
  std::array<uint8_t, kSampleSize> mutable_data;

  // Pointer-and-size construction
  Span<const uint8_t> const_span =
      Span<const uint8_t>(kSampleData, kSampleSize);
  Span<const uint8_t> const_span2 =
      Span<const uint8_t>(mutable_data.data(), mutable_data.size());

  Span<uint8_t> mutable_span =
      Span<uint8_t>(mutable_data.data(), mutable_data.size());

  // ILLEGAL!
  // Span<uint8_t> mutable_span =  Span<uint8_t>(kSampleData, kSampleSize);

  // Copy constructors
  Span<const uint8_t> const_span3(const_span);
  Span<const uint8_t> const_span4(mutable_span);
  Span<uint8_t> mutable_span2(mutable_span);
  Span<uint8_t> mutable_span3(mutable_span);

  // ILLEGAL!
  // Span<uint8_t> mutable_span3(const_span);

  // Move constructors
  Span<const uint8_t> const_span5(std::move(const_span3));
  Span<const uint8_t> const_span6(std::move(mutable_span2));
  Span<uint8_t> mutable_span4(std::move(mutable_span3));

  // ILLEGAL!
  // Span<uint8_t> mutable_span5(std::move(const_span4));

  // Copy assignment
  Span<const uint8_t> const_span7;
  const_span7 = const_span2;

  Span<const uint8_t> const_span8;
  const_span8 = mutable_span4;

  // ILLEGAL!
  // Span<uint8_t> mutable_span6;
  // mutable_span6 = const_span5;

  Span<uint8_t> mutable_span7;
  mutable_span7 = mutable_span4;

  // Move assignment
  Span<const uint8_t> const_span9;
  const_span9 = std::move(const_span5);

  Span<const uint8_t> const_span10;
  const_span10 = std::move(mutable_span4);

  // ILLEGAL!
  // Span<uint8_t> mutable_span8;
  // mutable_span8 = std::move(const_span5);

  Span<uint8_t> mutable_span9;
  mutable_span9 = std::move(mutable_span4);

  // Vector construction
  std::vector<uint8_t> mutable_vector({1, 2, 3, 4});
  Span<const uint8_t> const_span11(mutable_vector);
  Span<uint8_t> mutable_span10(mutable_vector);
}

}  // namespace openscreen
