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

#include <algorithm>
#include <cstdint>
#include <tuple>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"

namespace libgav1 {
namespace {

// Import all the constants in the anonymous namespace.
#include "src/scan_tables.inc"

class ScanOrderTest
    : public testing::TestWithParam<std::tuple<TransformClass, TransformSize>> {
 public:
  ScanOrderTest() = default;
  ScanOrderTest(const ScanOrderTest&) = delete;
  ScanOrderTest& operator=(const ScanOrderTest&) = delete;
  ~ScanOrderTest() override = default;

 protected:
  TransformClass tx_class_ = std::get<0>(GetParam());
  TransformSize tx_size_ = std::get<1>(GetParam());
};

TEST_P(ScanOrderTest, AllIndicesAreScannedExactlyOnce) {
  const int tx_width = kTransformWidth[tx_size_];
  const int tx_height = kTransformHeight[tx_size_];
  int num_indices;
  if (tx_class_ == kTransformClass2D || std::max(tx_width, tx_height) == 64) {
    const int clamped_tx_width = std::min(32, tx_width);
    const int clamped_tx_height = std::min(32, tx_height);
    num_indices = clamped_tx_width * clamped_tx_height;
  } else {
    num_indices =
        (std::max(tx_width, tx_height) > 16) ? 64 : tx_width * tx_height;
  }
  const uint16_t* const scan = kScan[tx_class_][tx_size_];
  ASSERT_NE(scan, nullptr);
  // Ensure that all the indices are scanned exactly once.
  std::vector<int> scanned;
  scanned.resize(num_indices);
  for (int i = 0; i < num_indices; ++i) {
    scanned[scan[i]]++;
  }
  EXPECT_THAT(scanned, testing::Each(1));
}

constexpr TransformClass kTestTransformClasses[] = {
    kTransformClass2D, kTransformClassVertical, kTransformClassHorizontal};

constexpr TransformSize kTestTransformSizes[] = {
    kTransformSize4x4,   kTransformSize4x8,   kTransformSize4x16,
    kTransformSize8x4,   kTransformSize8x8,   kTransformSize8x16,
    kTransformSize8x32,  kTransformSize16x4,  kTransformSize16x8,
    kTransformSize16x16, kTransformSize16x32, kTransformSize16x64,
    kTransformSize32x8,  kTransformSize32x16, kTransformSize32x32,
    kTransformSize32x64, kTransformSize64x16, kTransformSize64x32,
    kTransformSize64x64};

INSTANTIATE_TEST_SUITE_P(
    C, ScanOrderTest,
    testing::Combine(testing::ValuesIn(kTestTransformClasses),
                     testing::ValuesIn(kTestTransformSizes)));

}  // namespace
}  // namespace libgav1
