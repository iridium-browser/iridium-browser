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

#include "src/reconstruction.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "absl/strings/match.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/dsp/inverse_transform.h"
#include "src/utils/array_2d.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/cpu.h"
#include "src/utils/memory.h"
#include "tests/block_utils.h"
#include "tests/utils.h"

namespace libgav1 {
namespace {

// Import the scan tables in the anonymous namespace.
#include "src/scan_tables.inc"

constexpr int kTestTransformSize = 4;
constexpr int8_t kTestBitdepth = 8;

using testing::ElementsAreArray;

// The 'int' parameter is unused but required to allow for instantiations of C,
// NEON, etc.
class ReconstructionTest : public testing::TestWithParam<int> {
 public:
  ReconstructionTest() = default;
  ReconstructionTest(const ReconstructionTest&) = delete;
  ReconstructionTest& operator=(const ReconstructionTest&) = delete;
  ~ReconstructionTest() override = default;

 protected:
  void SetUp() override {
    test_utils::ResetDspTable(kTestBitdepth);
    dsp::InverseTransformInit_C();
    dsp_ = dsp::GetDspTable(kTestBitdepth);
    ASSERT_NE(dsp_, nullptr);
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    if (test_info->value_param() != nullptr) {
      const char* const test_case = test_info->test_suite_name();
      if (absl::StartsWith(test_case, "C/")) {
      } else if (absl::StartsWith(test_case, "SSE41/")) {
        if ((GetCpuInfo() & kSSE4_1) != 0) {
          dsp::InverseTransformInit_SSE4_1();
        }
      } else if (absl::StartsWith(test_case, "NEON/")) {
        dsp::InverseTransformInit_NEON();
      } else {
        FAIL() << "Unrecognized architecture prefix in test case name: "
               << test_case;
      }
    }
    InitBuffers();
  }

  void InitBuffers(int width = kTestTransformSize,
                   int height = kTestTransformSize) {
    const int size = width * height;
    buffer_.clear();
    buffer_.resize(size);
    residual_buffer_.clear();
    residual_buffer_.resize(size);
    for (int i = 0; i < size; ++i) {
      buffer_[i] = residual_buffer_[i] = i % 256;
    }
    frame_buffer_.Reset(height, width, buffer_.data());
  }

  template <int bitdepth>
  void TestWht();

  std::vector<uint8_t> buffer_;
  std::vector<int16_t> residual_buffer_;
  // |frame_buffer_| is just a 2D array view into the |buffer_|.
  Array2DView<uint8_t> frame_buffer_;
  const dsp::Dsp* dsp_;
};

template <int bitdepth>
void ReconstructionTest::TestWht() {
  static_assert(bitdepth == kBitdepth8 || bitdepth == kBitdepth10, "");
  for (const auto transform :
       dsp_->inverse_transforms[dsp::kTransform1dWht][dsp::kTransform1dSize4]) {
    if (transform == nullptr) {
      GTEST_SKIP() << "No function available for dsp::kTransform1dWht";
    }
  }
  constexpr int max = 16 << bitdepth;
  constexpr int min = -max;
  static constexpr int16_t residual_inputs[][16]{
      {64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {69, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, max - 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, min - 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      // Note these are unrealistic inputs, but serve to test each position in
      // the array and match extremes in some commercial test vectors.
      {max, max, max, max, max, max, max, max, max, max, max, max, max, max,
       max, max},
      {min, min, min, min, min, min, min, min, min, min, min, min, min, min,
       min, min}};
  // Before the Reconstruct() call, the frame buffer is filled with all 127.
  // After the Reconstruct() call, the frame buffer is expected to have the
  // following values.
  static constexpr uint8_t frame_outputs[][16]{
      {131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 131,
       131, 131},
      {132, 131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 131,
       131, 131},
      {255, 255, 0, 0, 255, 255, 0, 0, 0, 0, 255, 255, 0, 0, 255, 255},
      {0, 0, 255, 255, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255, 0, 0},
      {255, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
       127, 127},
      {0, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
       127},
  };

  const TransformSize tx_size = kTransformSize4x4;
  const TransformType tx_type = kTransformTypeDctDct;
  const int tx_width = kTransformWidth[tx_size];
  const int tx_height = kTransformHeight[tx_size];
  const uint16_t* const scan = kScan[GetTransformClass(tx_type)][tx_size];

  InitBuffers(tx_width, tx_height);

  const int num_tests = sizeof(residual_inputs) / sizeof(residual_inputs[0]);
  for (int i = 0; i < num_tests; ++i) {
    int16_t eob;  // Also known as non_zero_coeff_count.
    for (eob = 15; eob >= 0; --eob) {
      if (residual_inputs[i][scan[eob]] != 0) break;
    }
    ++eob;
    memcpy(residual_buffer_.data(), residual_inputs[i],
           sizeof(residual_inputs[i]));
    memset(buffer_.data(), 127, sizeof(frame_outputs[i]));
    Reconstruct(*dsp_, tx_type, tx_size, /*lossless=*/true,
                residual_buffer_.data(), 0, 0, &frame_buffer_, eob);

    EXPECT_TRUE(test_utils::CompareBlocks(buffer_.data(), frame_outputs[i],
                                          tx_width, tx_height, tx_width,
                                          tx_width, false, true))
        << "Mismatch WHT test case " << i;
  }
}

TEST_P(ReconstructionTest, ReconstructionSimple) {
  for (const auto transform :
       dsp_->inverse_transforms[dsp::kTransform1dIdentity]
                               [dsp::kTransform1dSize4]) {
    if (transform == nullptr) GTEST_SKIP();
  }
  Reconstruct(*dsp_, kTransformTypeIdentityIdentity, kTransformSize4x4, false,
              residual_buffer_.data(), 0, 0, &frame_buffer_, 16);
  // clang-format off
  static constexpr uint8_t expected_output_buffer[] = {
      0, 1, 2, 3,
      5, 6, 7, 8,
      9, 10, 11, 12,
      14, 15, 16, 17
  };
  // clang-format on
  EXPECT_THAT(buffer_, ElementsAreArray(expected_output_buffer));
}

TEST_P(ReconstructionTest, ReconstructionFlipY) {
  for (const auto transform :
       dsp_->inverse_transforms[dsp::kTransform1dIdentity]
                               [dsp::kTransform1dSize4]) {
    if (transform == nullptr) GTEST_SKIP();
  }
  Reconstruct(*dsp_, kTransformTypeIdentityFlipadst, kTransformSize4x4, false,
              residual_buffer_.data(), 0, 0, &frame_buffer_, 16);
  // clang-format off
  static constexpr uint8_t expected_buffer[] = {
      0, 1, 2, 3,
      4, 5, 6, 7,
      7, 8, 9, 10,
      14, 15, 16, 17
  };
  // clang-format on
  EXPECT_THAT(buffer_, ElementsAreArray(expected_buffer));
}

TEST_P(ReconstructionTest, ReconstructionFlipX) {
  for (const auto transform :
       dsp_->inverse_transforms[dsp::kTransform1dIdentity]
                               [dsp::kTransform1dSize4]) {
    if (transform == nullptr) GTEST_SKIP();
  }
  Reconstruct(*dsp_, kTransformTypeFlipadstIdentity, kTransformSize4x4, false,
              residual_buffer_.data(), 0, 0, &frame_buffer_, 16);
  // clang-format off
  static constexpr uint8_t expected_buffer[] = {
      0, 1, 2, 3,
      4, 5, 6, 8,
      8, 10, 10, 13,
      12, 14, 14, 18
  };
  // clang-format on
  EXPECT_THAT(buffer_, ElementsAreArray(expected_buffer));
}

TEST_P(ReconstructionTest, ReconstructionFlipXAndFlipY) {
  for (const auto transform :
       dsp_->inverse_transforms[dsp::kTransform1dIdentity]
                               [dsp::kTransform1dSize4]) {
    if (transform == nullptr) GTEST_SKIP();
  }
  Reconstruct(*dsp_, kTransformTypeFlipadstFlipadst, kTransformSize4x4, false,
              residual_buffer_.data(), 0, 0, &frame_buffer_, 16);
  // clang-format off
  static constexpr uint8_t expected_buffer[] = {
      0, 1, 2, 3,
      4, 5, 6, 8,
      8, 8, 10, 9,
      12, 14, 14, 19
  };
  // clang-format on
  EXPECT_THAT(buffer_, ElementsAreArray(expected_buffer));
}

TEST_P(ReconstructionTest, ReconstructionNonZeroStart) {
  uint8_t buffer[64] = {};
  Array2DView<uint8_t> frame_buffer(8, 8, buffer);
  int k = 0;
  for (int i = 0; i < kTestTransformSize; ++i) {
    for (int j = 0; j < kTestTransformSize; ++j) {
      frame_buffer[i + 4][j + 4] = k++;
    }
  }
  for (const auto transform :
       dsp_->inverse_transforms[dsp::kTransform1dIdentity]
                               [dsp::kTransform1dSize4]) {
    if (transform == nullptr) GTEST_SKIP();
  }
  Reconstruct(*dsp_, kTransformTypeIdentityIdentity, kTransformSize4x4, false,
              residual_buffer_.data(), 4, 4, &frame_buffer, 64);
  // clang-format off
  static constexpr uint8_t expected_buffer[] = {
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 1, 2, 3,
      0, 0, 0, 0, 5, 6, 7, 8,
      0, 0, 0, 0, 9, 10, 11, 12,
      0, 0, 0, 0, 14, 15, 16, 17
  };
  // clang-format on
  EXPECT_THAT(buffer, ElementsAreArray(expected_buffer));
}

TEST_P(ReconstructionTest, Wht8bit) { TestWht<kBitdepth8>(); }

#if LIBGAV1_MAX_BITDEPTH >= 10
TEST_P(ReconstructionTest, Wht10bit) { TestWht<kBitdepth10>(); }
#endif

INSTANTIATE_TEST_SUITE_P(C, ReconstructionTest, testing::Values(0));

#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, ReconstructionTest, testing::Values(0));
#endif

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, ReconstructionTest, testing::Values(0));
#endif

}  // namespace
}  // namespace libgav1
