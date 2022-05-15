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

#include "src/dsp/arm/common_neon.h"

#include "gtest/gtest.h"
#include "src/utils/cpu.h"

#if LIBGAV1_ENABLE_NEON
#include <cstdint>

#include "tests/block_utils.h"

namespace libgav1 {
namespace dsp {
namespace {

constexpr int kMaxBlockWidth = 16;
constexpr int kMaxBlockHeight = 16;

template <typename Pixel>
class TransposeTest : public testing::Test {
 public:
  TransposeTest() {
    for (int y = 0; y < kMaxBlockHeight; ++y) {
      for (int x = 0; x < kMaxBlockWidth; ++x) {
        src_block_[y][x] = y * 16 + x;
        expected_transpose_[y][x] = x * 16 + y;
      }
    }
  }

  TransposeTest(const TransposeTest&) = delete;
  TransposeTest& operator=(const TransposeTest&) = delete;
  ~TransposeTest() override = default;

 protected:
  Pixel src_block_[kMaxBlockHeight][kMaxBlockWidth];
  Pixel expected_transpose_[kMaxBlockHeight][kMaxBlockWidth];
};

using TransposeTestLowBitdepth = TransposeTest<uint8_t>;

TEST_F(TransposeTestLowBitdepth, Transpose4x4Test) {
  uint8x8_t a = Load4<1>(src_block_[1], Load4(src_block_[0]));
  uint8x8_t b = Load4<1>(src_block_[3], Load4(src_block_[2]));
  Transpose4x4(&a, &b);
  uint8_t output_4x4[4][4];
  StoreLo4(output_4x4[0], a);
  StoreLo4(output_4x4[1], b);
  StoreHi4(output_4x4[2], a);
  StoreHi4(output_4x4[3], b);
  EXPECT_TRUE(test_utils::CompareBlocks(expected_transpose_[0], output_4x4[0],
                                        4, 4, kMaxBlockWidth, 4, false));
}

TEST_F(TransposeTestLowBitdepth, Transpose8x4Test) {
  uint8x8_t a0 = Load4<1>(src_block_[4], Load4(src_block_[0]));
  uint8x8_t a1 = Load4<1>(src_block_[5], Load4(src_block_[1]));
  uint8x8_t a2 = Load4<1>(src_block_[6], Load4(src_block_[2]));
  uint8x8_t a3 = Load4<1>(src_block_[7], Load4(src_block_[3]));
  Transpose8x4(&a0, &a1, &a2, &a3);
  uint8_t output_8x4[4][8];
  vst1_u8(output_8x4[0], a0);
  vst1_u8(output_8x4[1], a1);
  vst1_u8(output_8x4[2], a2);
  vst1_u8(output_8x4[3], a3);
  EXPECT_TRUE(test_utils::CompareBlocks(expected_transpose_[0], output_8x4[0],
                                        8, 4, kMaxBlockWidth, 8, false));
}

TEST_F(TransposeTestLowBitdepth, Transpose8x8Test) {
  uint8x8_t input_8x8[8];
  for (int i = 0; i < 8; ++i) {
    input_8x8[i] = vld1_u8(src_block_[i]);
  }
  Transpose8x8(input_8x8);
  uint8_t output_8x8[8][8];
  for (int i = 0; i < 8; ++i) {
    vst1_u8(output_8x8[i], input_8x8[i]);
  }
  EXPECT_TRUE(test_utils::CompareBlocks(expected_transpose_[0], output_8x8[0],
                                        8, 8, kMaxBlockWidth, 8, false));
}

TEST_F(TransposeTestLowBitdepth, Transpose8x16Test) {
  uint8x16_t input_8x16[8];
  for (int i = 0; i < 8; ++i) {
    input_8x16[i] =
        vcombine_u8(vld1_u8(src_block_[i]), vld1_u8(src_block_[i + 8]));
  }
  Transpose8x16(input_8x16);
  uint8_t output_16x8[8][16];
  for (int i = 0; i < 8; ++i) {
    vst1q_u8(output_16x8[i], input_8x16[i]);
  }
  EXPECT_TRUE(test_utils::CompareBlocks(expected_transpose_[0], output_16x8[0],
                                        16, 8, kMaxBlockWidth, 16, false));
}

using TransposeTestHighBitdepth = TransposeTest<uint16_t>;

TEST_F(TransposeTestHighBitdepth, Transpose4x4Test) {
  uint16x4_t input_4x4[4];
  input_4x4[0] = vld1_u16(src_block_[0]);
  input_4x4[1] = vld1_u16(src_block_[1]);
  input_4x4[2] = vld1_u16(src_block_[2]);
  input_4x4[3] = vld1_u16(src_block_[3]);
  Transpose4x4(input_4x4);
  uint16_t output_4x4[4][4];
  for (int i = 0; i < 4; ++i) {
    vst1_u16(output_4x4[i], input_4x4[i]);
  }
  EXPECT_TRUE(test_utils::CompareBlocks(expected_transpose_[0], output_4x4[0],
                                        4, 4, kMaxBlockWidth, 4, false));
}

TEST_F(TransposeTestHighBitdepth, Transpose4x8Test) {
  uint16x8_t input_4x8[4];
  for (int i = 0; i < 4; ++i) {
    input_4x8[i] = vld1q_u16(src_block_[i]);
  }
  Transpose4x8(input_4x8);
  uint16_t output_4x8[4][8];
  for (int i = 0; i < 4; ++i) {
    vst1q_u16(output_4x8[i], input_4x8[i]);
    memcpy(&expected_transpose_[i][4], &expected_transpose_[i + 4][0],
           4 * sizeof(expected_transpose_[0][0]));
  }
  EXPECT_TRUE(test_utils::CompareBlocks(expected_transpose_[0], output_4x8[0],
                                        8, 4, kMaxBlockWidth, 8, false));
}

TEST_F(TransposeTestHighBitdepth, LoopFilterTranspose4x8Test) {
  uint16x8_t input_4x8[4];
  for (int i = 0; i < 4; ++i) {
    input_4x8[i] = vld1q_u16(src_block_[i]);
  }
  LoopFilterTranspose4x8(input_4x8);
  uint16_t output_4x8[4][8];
  for (int i = 0; i < 4; ++i) {
    vst1q_u16(output_4x8[i], input_4x8[i]);
  }
  // a[0]: 03 13 23 33 04 14 24 34  p0q0
  // a[1]: 02 12 22 32 05 15 25 35  p1q1
  // a[2]: 01 11 21 31 06 16 26 36  p2q2
  // a[3]: 00 10 20 30 07 17 27 37  p3q3
  static constexpr uint16_t expected_output[4][8] = {
      {0x03, 0x13, 0x23, 0x33, 0x04, 0x14, 0x24, 0x34},
      {0x02, 0x12, 0x22, 0x32, 0x05, 0x15, 0x25, 0x35},
      {0x01, 0x11, 0x21, 0x31, 0x06, 0x16, 0x26, 0x36},
      {0x00, 0x10, 0x20, 0x30, 0x07, 0x17, 0x27, 0x37},
  };
  EXPECT_TRUE(test_utils::CompareBlocks(expected_output[0], output_4x8[0], 8, 4,
                                        8, 8, false));
}

TEST_F(TransposeTestHighBitdepth, Transpose8x8Test) {
  uint16x8_t input_8x8[8];
  for (int i = 0; i < 8; ++i) {
    input_8x8[i] = vld1q_u16(src_block_[i]);
  }
  Transpose8x8(input_8x8);
  uint16_t output_8x8[8][8];
  for (int i = 0; i < 8; ++i) {
    vst1q_u16(output_8x8[i], input_8x8[i]);
  }
  EXPECT_TRUE(test_utils::CompareBlocks(expected_transpose_[0], output_8x8[0],
                                        8, 8, kMaxBlockWidth, 8, false));
}

TEST_F(TransposeTestHighBitdepth, Transpose8x8SignedTest) {
  int16x8_t input_8x8[8];
  for (int i = 0; i < 8; ++i) {
    input_8x8[i] = vreinterpretq_s16_u16(vld1q_u16(src_block_[i]));
  }
  Transpose8x8(input_8x8);
  uint16_t output_8x8[8][8];
  for (int i = 0; i < 8; ++i) {
    vst1q_u16(output_8x8[i], vreinterpretq_u16_s16(input_8x8[i]));
  }
  EXPECT_TRUE(test_utils::CompareBlocks(expected_transpose_[0], output_8x8[0],
                                        8, 8, kMaxBlockWidth, 8, false));
}

}  // namespace
}  // namespace dsp
}  // namespace libgav1

#else  // !LIBGAV1_ENABLE_NEON

TEST(CommonDspTest, NEON) {
  GTEST_SKIP()
      << "Build this module for Arm with NEON enabled to enable the tests.";
}

#endif  // LIBGAV1_ENABLE_NEON
