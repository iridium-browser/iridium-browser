// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/base64.h"

#include <string>
#include <vector>

#include "gtest/gtest.h"

namespace openscreen {
namespace base64 {

namespace {

constexpr char kText[] = "hello world";
constexpr char kBase64Text[] = "aGVsbG8gd29ybGQ=";

// More sophisticated comparisons here, such as EXPECT_STREQ, may
// cause memory failures on some platforms (e.g. ASAN) due to mismatched
// lengths.
void CheckEquals(const char* expected, const std::vector<uint8_t>& actual) {
  EXPECT_EQ(0, std::memcmp(actual.data(), expected, actual.size()));
}

void CheckEncodeDecode(const char* to_encode, const char* encode_expected) {
  std::string encoded = Encode(to_encode);
  EXPECT_EQ(encode_expected, encoded);

  std::vector<uint8_t> decoded;
  EXPECT_TRUE(Decode(encoded, &decoded));

  CheckEquals(to_encode, decoded);
}

}  // namespace

TEST(Base64Test, ZeroSize) {
  CheckEncodeDecode("", "");
}

TEST(Base64Test, Basic) {
  CheckEncodeDecode(kText, kBase64Text);
}

TEST(Base64Test, Binary) {
  const uint8_t kData[] = {0x00, 0x01, 0xFE, 0xFF};

  std::string binary_encoded = Encode(absl::MakeConstSpan(kData));

  // Check that encoding the same data through the StringPiece interface gives
  // the same results.
  std::string string_piece_encoded = Encode(
      absl::string_view(reinterpret_cast<const char*>(kData), sizeof(kData)));

  EXPECT_EQ(binary_encoded, string_piece_encoded);
}

TEST(Base64Test, InPlace) {
  std::string text(kText);

  text = Encode(text);
  EXPECT_EQ(kBase64Text, text);

  std::vector<uint8_t> out;
  EXPECT_TRUE(Decode(text, &out));
  CheckEquals(kText, out);
}

}  // namespace base64
}  // namespace openscreen
