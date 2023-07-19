// Copyright (c) 2021, Alliance for Open Media. All rights reserved
//
// This source code is subject to the terms of the BSD 2 Clause License and
// the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
// was not distributed with this source code in the LICENSE file, you can
// obtain it at www.aomedia.org/license/software. If the Alliance for Open
// Media Patent License 1.0 was not distributed with this source code in the
// PATENTS file, you can obtain it at www.aomedia.org/license/patent.

#include "avifinfo.h"

#include <algorithm>
#include <fstream>
#include <vector>

#include "gtest/gtest.h"

namespace {

using Data = std::vector<uint8_t>;

Data LoadFile(const char file_name[]) {
  std::ifstream file(file_name, std::ios::binary | std::ios::ate);
  if (!file) return Data();
  const auto file_size = file.tellg();
  Data bytes(file_size * sizeof(char));
  file.seekg(0);  // Rewind.
  return file.read(reinterpret_cast<char*>(bytes.data()), file_size) ? bytes
                                                                     : Data();
}

bool AreEqual(const AvifInfoFeatures& a, const AvifInfoFeatures& b) {
  return a.width == b.width && a.height == b.height &&
         a.bit_depth == b.bit_depth && a.num_channels == b.num_channels;
}

//------------------------------------------------------------------------------
// Positive tests

TEST(AvifInfoGetTest, Ok) {
  const Data input = LoadFile("avifinfo_test_1x1.avif");
  ASSERT_FALSE(input.empty());

  EXPECT_EQ(AvifInfoIdentify(input.data(), input.size()), kAvifInfoOk);
  AvifInfoFeatures f;
  EXPECT_EQ(AvifInfoGetFeatures(input.data(), input.size(), &f), kAvifInfoOk);
  EXPECT_TRUE(AreEqual(f, {1u, 1u, 8u, 3u}));
}

TEST(AvifInfoGetTest, NoPixi10b) {
  // Same as above but "meta" box size is stored as 64 bits, "av1C" has
  // 'high_bitdepth' set to true, "pixi" was renamed to "pixy" and "mdat" size
  // is 0 (extends to the end of the file).
  const Data input =
      LoadFile("avifinfo_test_1x1_10b_nopixi_metasize64b_mdatsize0.avif");
  ASSERT_FALSE(input.empty());

  EXPECT_EQ(AvifInfoIdentify(input.data(), input.size()), kAvifInfoOk);
  AvifInfoFeatures f;
  EXPECT_EQ(AvifInfoGetFeatures(input.data(), input.size(), &f), kAvifInfoOk);
  EXPECT_TRUE(AreEqual(f, {1u, 1u, 10u, 3u}));
}

TEST(AvifInfoGetTest, EnoughBytes) {
  Data input = LoadFile("avifinfo_test_1x1.avif");
  ASSERT_FALSE(input.empty());
  // Truncate 'input' just after the required information (discard AV1 box).
  const uint8_t kMdatTag[] = {'m', 'd', 'a', 't'};
  input.resize(std::search(input.begin(), input.end(), kMdatTag, kMdatTag + 4) -
               input.begin());

  EXPECT_EQ(AvifInfoIdentify(input.data(), input.size()), kAvifInfoOk);
  AvifInfoFeatures f;
  EXPECT_EQ(AvifInfoGetFeatures(input.data(), input.size(), &f), kAvifInfoOk);
  EXPECT_TRUE(AreEqual(f, {1u, 1u, 8u, 3u}));
}

TEST(AvifInfoGetTest, Null) {
  const Data input = LoadFile("avifinfo_test_1x1.avif");
  ASSERT_FALSE(input.empty());

  EXPECT_EQ(AvifInfoGetFeatures(input.data(), input.size(), nullptr),
            kAvifInfoOk);
}

//------------------------------------------------------------------------------
// Negative tests

TEST(AvifInfoGetTest, Empty) {
  EXPECT_EQ(AvifInfoIdentify(nullptr, 0), kAvifInfoNotEnoughData);
  AvifInfoFeatures f;
  EXPECT_EQ(AvifInfoGetFeatures(nullptr, 0, &f), kAvifInfoNotEnoughData);
  EXPECT_TRUE(AreEqual(f, {0}));
}

TEST(AvifInfoGetTest, NotEnoughBytes) {
  Data input = LoadFile("avifinfo_test_1x1.avif");
  ASSERT_FALSE(input.empty());
  // Truncate 'input' before having all the required information.
  const uint8_t kIpmaTag[] = {'i', 'p', 'm', 'a'};
  input.resize(std::search(input.begin(), input.end(), kIpmaTag, kIpmaTag + 4) -
               input.begin());

  EXPECT_EQ(AvifInfoIdentify(input.data(), input.size()), kAvifInfoOk);
  AvifInfoFeatures f;
  EXPECT_EQ(AvifInfoGetFeatures(input.data(), input.size(), &f),
            kAvifInfoNotEnoughData);
}

TEST(AvifInfoGetTest, Broken) {
  Data input = LoadFile("avifinfo_test_1x1.avif");
  ASSERT_FALSE(input.empty());
  // Change "ispe" to "aspe".
  const uint8_t kIspeTag[] = {'i', 's', 'p', 'e'};
  std::search(input.begin(), input.end(), kIspeTag, kIspeTag + 4)[0] = 'a';

  EXPECT_EQ(AvifInfoIdentify(input.data(), input.size()), kAvifInfoOk);
  AvifInfoFeatures f;
  EXPECT_EQ(AvifInfoGetFeatures(input.data(), input.size(), &f),
            kAvifInfoInvalidFile);
  EXPECT_TRUE(AreEqual(f, {0}));
}

TEST(AvifInfoGetTest, MetaBoxIsTooBig) {
  Data input = LoadFile("avifinfo_test_1x1.avif");
  ASSERT_FALSE(input.empty());
  // Change "meta" box size to the maximum size 2^64-1.
  const uint8_t kMetaTag[] = {'m', 'e', 't', 'a'};
  auto meta_tag =
      std::search(input.begin(), input.end(), kMetaTag, kMetaTag + 4);
  meta_tag[-4] = meta_tag[-3] = meta_tag[-2] = 0;
  meta_tag[-1] = 1;  // 32-bit "1" then 4-char "meta" then 64-bit size.
  input.insert(meta_tag + 4, {255, 255, 255, 255, 255, 255, 255, 255});

  EXPECT_EQ(AvifInfoIdentify(input.data(), input.size()), kAvifInfoOk);
  AvifInfoFeatures f;
  EXPECT_EQ(AvifInfoGetFeatures(input.data(), input.size(), &f),
            kAvifInfoTooComplex);
  EXPECT_TRUE(AreEqual(f, {0}));
}

TEST(AvifInfoGetTest, TooManyBoxes) {
  // Create a valid-ish input with too many boxes to parse.
  Data input = {0,   0,   0,   16,  'f', 't', 'y', 'p',
                'a', 'v', 'i', 'f', 0,   0,   0,   0};
  const uint32_t kNumBoxes = 12345;
  input.reserve(input.size() + kNumBoxes * 8);
  for (uint32_t i = 0; i < kNumBoxes; ++i) {
    const uint8_t kBox[] = {0, 0, 0, 8, 'a', 'b', 'c', 'd'};
    input.insert(input.end(), kBox, kBox + kBox[3]);
  }

  EXPECT_EQ(AvifInfoIdentify(input.data(), input.size()), kAvifInfoOk);
  AvifInfoFeatures f;
  EXPECT_EQ(AvifInfoGetFeatures(reinterpret_cast<uint8_t*>(input.data()),
                                input.size() * 4, &f),
            kAvifInfoTooComplex);
}

TEST(AvifInfoReadTest, Null) {
  EXPECT_EQ(AvifInfoIdentifyStream(/*stream=*/nullptr, /*read=*/nullptr,
                                   /*skip=*/nullptr),
            kAvifInfoNotEnoughData);
  AvifInfoFeatures f;
  EXPECT_EQ(AvifInfoGetFeaturesStream(/*stream=*/nullptr, /*read=*/nullptr,
                                      /*skip=*/nullptr, &f),
            kAvifInfoNotEnoughData);
  EXPECT_TRUE(AreEqual(f, {0}));
}

//------------------------------------------------------------------------------

}  // namespace
