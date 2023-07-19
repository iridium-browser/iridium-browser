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

#include "src/utils/segmentation.h"

#include <cstdint>

#include "gtest/gtest.h"
#include "src/utils/common.h"
#include "src/utils/types.h"

namespace libgav1 {
namespace {

int GetUnsignedBits(const unsigned int num_values) {
  return (num_values > 0) ? FloorLog2(num_values) + 1 : 0;
}

// Check that kSegmentationFeatureBits and kSegmentationFeatureMaxValues are
// consistent with each other.
TEST(SegmentationTest, FeatureBitsAndMaxValuesConsistency) {
  for (int feature = 0; feature < kSegmentFeatureMax; feature++) {
    EXPECT_EQ(kSegmentationFeatureBits[feature],
              GetUnsignedBits(kSegmentationFeatureMaxValues[feature]));
  }
}

}  // namespace
}  // namespace libgav1
