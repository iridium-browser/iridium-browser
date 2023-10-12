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

void WriteBigEndian(uint32_t value, uint32_t num_bytes, uint8_t* input) {
  for (int i = num_bytes - 1; i >= 0; --i) {
    input[i] = value & 0xff;
    value >>= 8;
  }
}

bool SetPrimaryItemIdToGainmapId(Data& input) {
  AvifInfoFeatures f;
  if (AvifInfoGetFeatures(input.data(), input.size(), &f) != kAvifInfoOk) {
    return false;
  }
  WriteBigEndian(f.gainmap_item_id, f.primary_item_id_bytes,
                 &input[f.primary_item_id_location]);
  return true;
}

void ExpectEqual(const AvifInfoFeatures& actual,
                 const AvifInfoFeatures& expected) {
  EXPECT_EQ(actual.width, expected.width);
  EXPECT_EQ(actual.height, expected.height);
  EXPECT_EQ(actual.bit_depth, expected.bit_depth);
  EXPECT_EQ(actual.num_channels, expected.num_channels);
  EXPECT_EQ(actual.has_gainmap, expected.has_gainmap);
  EXPECT_EQ(actual.gainmap_item_id, expected.gainmap_item_id);
  EXPECT_EQ(actual.primary_item_id_location, expected.primary_item_id_location);
  EXPECT_EQ(actual.primary_item_id_bytes, expected.primary_item_id_bytes);
}

//------------------------------------------------------------------------------
// Positive tests

TEST(AvifInfoGetTest, Ok) {
  const Data input = LoadFile("avifinfo_test_1x1.avif");
  ASSERT_FALSE(input.empty());

  ASSERT_EQ(AvifInfoIdentify(input.data(), input.size()), kAvifInfoOk);
  AvifInfoFeatures f;
  ASSERT_EQ(AvifInfoGetFeatures(input.data(), input.size(), &f), kAvifInfoOk);
  ExpectEqual(f, {.width = 1u,
                  .height = 1u,
                  .bit_depth = 8u,
                  .num_channels = 3u,
                  .has_gainmap = 0u,
                  .primary_item_id_location = 96u,
                  .primary_item_id_bytes = 2u});
}

TEST(AvifInfoGetTest, WithAlpha) {
  const Data input = LoadFile("avifinfo_test_2x2_alpha.avif");
  ASSERT_FALSE(input.empty());

  ASSERT_EQ(AvifInfoIdentify(input.data(), input.size()), kAvifInfoOk);
  AvifInfoFeatures f;
  ASSERT_EQ(AvifInfoGetFeatures(input.data(), input.size(), &f), kAvifInfoOk);
  ExpectEqual(f, {.width = 2u,
                  .height = 2u,
                  .bit_depth = 8u,
                  .num_channels = 4u,
                  .has_gainmap = 0u,
                  .primary_item_id_location = 96u,
                  .primary_item_id_bytes = 2u});
}

TEST(AvifInfoGetTest, WithGainmap) {
  const Data input = LoadFile("avifinfo_test_20x20_gainmap.avif");
  ASSERT_FALSE(input.empty());

  ASSERT_EQ(AvifInfoIdentify(input.data(), input.size()), kAvifInfoOk);
  AvifInfoFeatures f;
  ASSERT_EQ(AvifInfoGetFeatures(input.data(), input.size(), &f), kAvifInfoOk);
  ExpectEqual(f, {.width = 20u,
                  .height = 20u,
                  .bit_depth = 8u,
                  .num_channels = 3u,
                  .has_gainmap = 1u,
                  .gainmap_item_id = 2u,
                  .primary_item_id_location = 96u,
                  .primary_item_id_bytes = 2u});

  Data gainmap = input;
  ASSERT_TRUE(SetPrimaryItemIdToGainmapId(gainmap));
  ASSERT_EQ(AvifInfoIdentify(gainmap.data(), gainmap.size()), kAvifInfoOk);
  AvifInfoFeatures gainmap_f;
  ASSERT_EQ(AvifInfoGetFeatures(gainmap.data(), gainmap.size(), &gainmap_f),
            kAvifInfoOk);
  // TODO(maryla-uc): find a small test file with a gainmap that is smaller
  // than the main image.
  ExpectEqual(gainmap_f, {.width = 20u,
                          .height = 20u,
                          .bit_depth = 8u,
                          .num_channels = 1u,  // the gainmap is monochrome
                          .has_gainmap = 1u,
                          .gainmap_item_id = 2u,
                          .primary_item_id_location = 96u,
                          .primary_item_id_bytes = 2u});
}

TEST(AvifInfoGetTest, NoPixi10b) {
  // Same as above but "meta" box size is stored as 64 bits, "av1C" has
  // 'high_bitdepth' set to true, "pixi" was renamed to "pixy" and "mdat" size
  // is 0 (extends to the end of the file).
  const Data input =
      LoadFile("avifinfo_test_1x1_10b_nopixi_metasize64b_mdatsize0.avif");
  ASSERT_FALSE(input.empty());

  ASSERT_EQ(AvifInfoIdentify(input.data(), input.size()), kAvifInfoOk);
  AvifInfoFeatures f;
  ASSERT_EQ(AvifInfoGetFeatures(input.data(), input.size(), &f), kAvifInfoOk);
  ExpectEqual(f, {.width = 1u,
                  .height = 1u,
                  .bit_depth = 10u,
                  .num_channels = 3u,
                  .has_gainmap = 0u,
                  .primary_item_id_location = 104u,
                  .primary_item_id_bytes = 2u});
}

TEST(AvifInfoGetTest, EnoughBytes) {
  Data input = LoadFile("avifinfo_test_1x1.avif");
  ASSERT_FALSE(input.empty());
  // Truncate 'input' just after the required information (discard AV1 box).
  const uint8_t kMdatTag[] = {'m', 'd', 'a', 't'};
  input.resize(std::search(input.begin(), input.end(), kMdatTag, kMdatTag + 4) -
               input.begin());

  ASSERT_EQ(AvifInfoIdentify(input.data(), input.size()), kAvifInfoOk);
  AvifInfoFeatures f;
  ASSERT_EQ(AvifInfoGetFeatures(input.data(), input.size(), &f), kAvifInfoOk);
  ExpectEqual(f, {.width = 1u,
                  .height = 1u,
                  .bit_depth = 8u,
                  .num_channels = 3u,
                  .has_gainmap = 0u,
                  .primary_item_id_location = 96u,
                  .primary_item_id_bytes = 2u});
}

TEST(AvifInfoGetTest, Null) {
  const Data input = LoadFile("avifinfo_test_1x1.avif");
  ASSERT_FALSE(input.empty());

  ASSERT_EQ(AvifInfoGetFeatures(input.data(), input.size(), nullptr),
            kAvifInfoOk);
}

//------------------------------------------------------------------------------
// Negative tests

TEST(AvifInfoGetTest, Empty) {
  ASSERT_EQ(AvifInfoIdentify(nullptr, 0), kAvifInfoNotEnoughData);
  AvifInfoFeatures f;
  ASSERT_EQ(AvifInfoGetFeatures(nullptr, 0, &f), kAvifInfoNotEnoughData);
  ExpectEqual(f, {0});
}

TEST(AvifInfoGetTest, NotEnoughBytes) {
  Data input = LoadFile("avifinfo_test_1x1.avif");
  ASSERT_FALSE(input.empty());
  // Truncate 'input' before having all the required information.
  const uint8_t kIpmaTag[] = {'i', 'p', 'm', 'a'};
  input.resize(std::search(input.begin(), input.end(), kIpmaTag, kIpmaTag + 4) -
               input.begin());

  ASSERT_EQ(AvifInfoIdentify(input.data(), input.size()), kAvifInfoOk);
  AvifInfoFeatures f;
  ASSERT_EQ(AvifInfoGetFeatures(input.data(), input.size(), &f),
            kAvifInfoNotEnoughData);
}

TEST(AvifInfoGetTest, Broken) {
  Data input = LoadFile("avifinfo_test_1x1.avif");
  ASSERT_FALSE(input.empty());
  // Change "ispe" to "aspe".
  const uint8_t kIspeTag[] = {'i', 's', 'p', 'e'};
  std::search(input.begin(), input.end(), kIspeTag, kIspeTag + 4)[0] = 'a';

  ASSERT_EQ(AvifInfoIdentify(input.data(), input.size()), kAvifInfoOk);
  AvifInfoFeatures f;
  ASSERT_EQ(AvifInfoGetFeatures(input.data(), input.size(), &f),
            kAvifInfoInvalidFile);
  ExpectEqual(f, {0});
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

  ASSERT_EQ(AvifInfoIdentify(input.data(), input.size()), kAvifInfoOk);
  AvifInfoFeatures f;
  ASSERT_EQ(AvifInfoGetFeatures(input.data(), input.size(), &f),
            kAvifInfoTooComplex);
  ExpectEqual(f, {0});
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

  ASSERT_EQ(AvifInfoIdentify(input.data(), input.size()), kAvifInfoOk);
  AvifInfoFeatures f;
  ASSERT_EQ(AvifInfoGetFeatures(reinterpret_cast<uint8_t*>(input.data()),
                                input.size() * 4, &f),
            kAvifInfoTooComplex);
}

TEST(AvifInfoReadTest, Null) {
  ASSERT_EQ(AvifInfoIdentifyStream(/*stream=*/nullptr, /*read=*/nullptr,
                                   /*skip=*/nullptr),
            kAvifInfoNotEnoughData);
  AvifInfoFeatures f;
  ASSERT_EQ(AvifInfoGetFeaturesStream(/*stream=*/nullptr, /*read=*/nullptr,
                                      /*skip=*/nullptr, &f),
            kAvifInfoNotEnoughData);
  ExpectEqual(f, {0});
}

//------------------------------------------------------------------------------

}  // namespace
