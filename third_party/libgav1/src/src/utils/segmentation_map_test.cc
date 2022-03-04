// Copyright 2021 The libgav1 Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/utils/segmentation_map.h"

#include <cstdint>

#include "gtest/gtest.h"

namespace libgav1 {
namespace {

TEST(SegmentationMapTest, Clear) {
  constexpr int32_t kRows4x4 = 60;
  constexpr int32_t kColumns4x4 = 80;
  SegmentationMap segmentation_map;
  ASSERT_TRUE(segmentation_map.Allocate(kRows4x4, kColumns4x4));

  segmentation_map.Clear();
  for (int row4x4 = 0; row4x4 < kRows4x4; ++row4x4) {
    for (int column4x4 = 0; column4x4 < kColumns4x4; ++column4x4) {
      EXPECT_EQ(segmentation_map.segment_id(row4x4, column4x4), 0);
    }
  }
}

TEST(SegmentationMapTest, FillBlock) {
  constexpr int32_t kRows4x4 = 60;
  constexpr int32_t kColumns4x4 = 80;
  SegmentationMap segmentation_map;
  ASSERT_TRUE(segmentation_map.Allocate(kRows4x4, kColumns4x4));

  // Fill the whole image with 2.
  segmentation_map.FillBlock(0, 0, kColumns4x4, kRows4x4, 2);
  // Fill a block with 1.
  constexpr int kBlockWidth4x4 = 10;
  constexpr int kBlockHeight4x4 = 20;
  segmentation_map.FillBlock(4, 6, kBlockWidth4x4, kBlockHeight4x4, 1);
  for (int row4x4 = 0; row4x4 < kRows4x4; ++row4x4) {
    for (int column4x4 = 0; column4x4 < kColumns4x4; ++column4x4) {
      if (4 <= row4x4 && row4x4 < 4 + kBlockHeight4x4 && 6 <= column4x4 &&
          column4x4 < 6 + kBlockWidth4x4) {
        // Inside the block.
        EXPECT_EQ(segmentation_map.segment_id(row4x4, column4x4), 1);
      } else {
        // Outside the block.
        EXPECT_EQ(segmentation_map.segment_id(row4x4, column4x4), 2);
      }
    }
  }
}

TEST(SegmentationMapTest, CopyFrom) {
  constexpr int32_t kRows4x4 = 60;
  constexpr int32_t kColumns4x4 = 80;
  SegmentationMap segmentation_map;
  ASSERT_TRUE(segmentation_map.Allocate(kRows4x4, kColumns4x4));

  // Split the segmentation map into four blocks of equal size.
  constexpr int kBlockWidth4x4 = 40;
  constexpr int kBlockHeight4x4 = 30;
  segmentation_map.FillBlock(0, 0, kBlockWidth4x4, kBlockHeight4x4, 1);
  segmentation_map.FillBlock(0, kBlockWidth4x4, kBlockWidth4x4, kBlockHeight4x4,
                             2);
  segmentation_map.FillBlock(kBlockHeight4x4, 0, kBlockWidth4x4,
                             kBlockHeight4x4, 3);
  segmentation_map.FillBlock(kBlockHeight4x4, kBlockWidth4x4, kBlockWidth4x4,
                             kBlockHeight4x4, 4);

  SegmentationMap segmentation_map2;
  ASSERT_TRUE(segmentation_map2.Allocate(kRows4x4, kColumns4x4));
  segmentation_map2.CopyFrom(segmentation_map);

  for (int row4x4 = 0; row4x4 < kBlockHeight4x4; ++row4x4) {
    for (int column4x4 = 0; column4x4 < kBlockWidth4x4; ++column4x4) {
      EXPECT_EQ(segmentation_map.segment_id(row4x4, column4x4), 1);
      EXPECT_EQ(segmentation_map2.segment_id(row4x4, column4x4), 1);
    }
  }
  for (int row4x4 = 0; row4x4 < kBlockHeight4x4; ++row4x4) {
    for (int column4x4 = 0; column4x4 < kBlockWidth4x4; ++column4x4) {
      EXPECT_EQ(segmentation_map.segment_id(row4x4, kBlockWidth4x4 + column4x4),
                2);
      EXPECT_EQ(
          segmentation_map2.segment_id(row4x4, kBlockWidth4x4 + column4x4), 2);
    }
  }
  for (int row4x4 = 0; row4x4 < kBlockHeight4x4; ++row4x4) {
    for (int column4x4 = 0; column4x4 < kBlockWidth4x4; ++column4x4) {
      EXPECT_EQ(
          segmentation_map.segment_id(kBlockHeight4x4 + row4x4, column4x4), 3);
      EXPECT_EQ(
          segmentation_map2.segment_id(kBlockHeight4x4 + row4x4, column4x4), 3);
    }
  }
  for (int row4x4 = 0; row4x4 < kBlockHeight4x4; ++row4x4) {
    for (int column4x4 = 0; column4x4 < kBlockWidth4x4; ++column4x4) {
      EXPECT_EQ(segmentation_map.segment_id(kBlockHeight4x4 + row4x4,
                                            kBlockWidth4x4 + column4x4),
                4);
      EXPECT_EQ(segmentation_map2.segment_id(kBlockHeight4x4 + row4x4,
                                             kBlockWidth4x4 + column4x4),
                4);
    }
  }
}

}  // namespace
}  // namespace libgav1
