// Copyright 2020 The libgav1 Authors
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

#include "src/dsp/weight_mask.h"

#include <algorithm>
#include <cstdint>
#include <ostream>
#include <string>
#include <type_traits>

#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "gtest/gtest.h"
#include "src/dsp/dsp.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/cpu.h"
#include "src/utils/memory.h"
#include "tests/third_party/libvpx/acm_random.h"
#include "tests/utils.h"

namespace libgav1 {
namespace dsp {
namespace {

constexpr int kNumSpeedTests = 50000;
constexpr int kMaxPredictionSize = 128;
// weight_mask is only used with kCompoundPredictionTypeDiffWeighted with
// convolve producing the most extreme ranges.
// This includes kCompoundOffset in 10bpp and 12bpp.
// see: src/dsp/convolve.cc & src/dsp/warp.cc.
constexpr int kCompoundPredictionRange[3][2] = {
    // 8bpp
    {-5132, 9212},
    // 10bpp
    {3988, 61532},
    // 12bpp
    {3974, 61559},
};

const char* GetDigest8bpp(int id) {
  static const char* const kDigest[] = {
      "eaca5b6a96dcfe5e44f3926a071b48b3",
      "1d82c75cfdf8e57925eb1d5301647538",
      "25bd455d74fb891b97b133c528f8db60",
      "" /*kBlock4x16*/,
      "1d82c75cfdf8e57925eb1d5301647538",
      "25bd455d74fb891b97b133c528f8db60",
      "62a08776db35a186406a11ab92dee71c",
      "95131d1dc0e05fcf4bd234d5ce9eea11",
      "25bd455d74fb891b97b133c528f8db60",
      "62a08776db35a186406a11ab92dee71c",
      "95131d1dc0e05fcf4bd234d5ce9eea11",
      "0b3c75272e0fb0747b9850145d340c4c",
      "95131d1dc0e05fcf4bd234d5ce9eea11",
      "0b3c75272e0fb0747b9850145d340c4c",
      "f26c43d4bc823a89c1ed47ab8708bc06",
      "0d99bbf31ecddc1c2d5063a68c0e9375",
      "0d99bbf31ecddc1c2d5063a68c0e9375",
      "5fb8ec5f582f0ebfe519ed55860f67c4",

      // mask_is_inverse = true.
      "96811f3b192828ff679e4c9ad8069d7d",
      "a04dc180c028d55af70240163445523a",
      "8513e3988233d0a7de316a0179bb6139",
      "" /*kBlock4x16*/,
      "a04dc180c028d55af70240163445523a",
      "8513e3988233d0a7de316a0179bb6139",
      "f7356d42fb44a6ccb41253ba35b8b3c7",
      "3d2d61ffc203ee64fe91c9d16168a19d",
      "8513e3988233d0a7de316a0179bb6139",
      "f7356d42fb44a6ccb41253ba35b8b3c7",
      "3d2d61ffc203ee64fe91c9d16168a19d",
      "87a2011ac69fb597ca4f71bb3c35ebb0",
      "3d2d61ffc203ee64fe91c9d16168a19d",
      "87a2011ac69fb597ca4f71bb3c35ebb0",
      "97100a3639d567046dc8a99fcb84cb2e",
      "9fabe05a6523da81a45150e19f75acff",
      "9fabe05a6523da81a45150e19f75acff",
      "7c0643e4d02421d06d7ca71822a94e1d",
  };
  return kDigest[id];
}

#if LIBGAV1_MAX_BITDEPTH >= 10
const char* GetDigest10bpp(int id) {
  static const char* const kDigest[] = {
      "5ae8d64b65a671301a457b8a73368ab5",
      "61535217f179054d4b76a8d9352a223d",
      "1aa6614773570e7b021cd509849c4180",
      "" /*kBlock4x16*/,
      "61535217f179054d4b76a8d9352a223d",
      "1aa6614773570e7b021cd509849c4180",
      "f04c2825cfb6408c7778658f71fa176e",
      "e1694ea1f026dac7fe7e86a84482cf86",
      "1aa6614773570e7b021cd509849c4180",
      "f04c2825cfb6408c7778658f71fa176e",
      "e1694ea1f026dac7fe7e86a84482cf86",
      "9c4855d44c013fbddb373b2e9e311080",
      "e1694ea1f026dac7fe7e86a84482cf86",
      "9c4855d44c013fbddb373b2e9e311080",
      "f510e743c3efe3b83374a98ef8a30838",
      "b6e0bd03c521c5f00e90530daa7d4432",
      "b6e0bd03c521c5f00e90530daa7d4432",
      "3270d7f621d488aec5b76bcf121debd0",

      // mask_is_inverse = true.
      "9aa00fcfe21b71e30c5393699122a020",
      "4d8ce33262cf6b5375f363530815189a",
      "428625c51ac1bd4585988f7b36dff1db",
      "" /*kBlock4x16*/,
      "4d8ce33262cf6b5375f363530815189a",
      "428625c51ac1bd4585988f7b36dff1db",
      "1ef63c06a2d9c42da293fdf924032981",
      "5dd3f201d755d1c22c126a633bfbb3c0",
      "428625c51ac1bd4585988f7b36dff1db",
      "1ef63c06a2d9c42da293fdf924032981",
      "5dd3f201d755d1c22c126a633bfbb3c0",
      "fe1e6843e6f214939da516dcbea04a79",
      "5dd3f201d755d1c22c126a633bfbb3c0",
      "fe1e6843e6f214939da516dcbea04a79",
      "240187f27389b5e89f9ec6bdbd7d20a7",
      "44925dab01011a98b8ab1f0308fa852a",
      "44925dab01011a98b8ab1f0308fa852a",
      "6d984b2ccfa056278e2130771127a943",
  };
  return kDigest[id];
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
const char* GetDigest12bpp(int id) {
  static const char* const kDigest[] = {
      "57629d3872fd52ff4bbec439c5517ec5",
      "dba421ceeb534756c77167e00ae91a2c",
      "72e8ac1d450ef0c6c6b03e93856d5cc2",
      "" /*kBlock4x16*/,
      "dba421ceeb534756c77167e00ae91a2c",
      "72e8ac1d450ef0c6c6b03e93856d5cc2",
      "ae573eb368df04e6a0133b4e15471728",
      "ceede597b2729357b15e0d08bb9bb760",
      "72e8ac1d450ef0c6c6b03e93856d5cc2",
      "ae573eb368df04e6a0133b4e15471728",
      "ceede597b2729357b15e0d08bb9bb760",
      "c4976af803d7ad3f92ef26f25b9f3754",
      "ceede597b2729357b15e0d08bb9bb760",
      "c4976af803d7ad3f92ef26f25b9f3754",
      "1d957d49f71bb7f304705a11a597f0cb",
      "9522d5713fb951b79f42d78fbff914cf",
      "9522d5713fb951b79f42d78fbff914cf",
      "422c046013f79a9f46e2c855967570ba",

      // mask_is_inverse = true.
      "a585cca9bc459d10e081bc0eb847b6e3",
      "2fa4ec5f74fad2831d216c51c2cdad5a",
      "d6c9ac69a9eb3059f5bb6e42b486ebcd",
      "" /*kBlock4x16*/,
      "2fa4ec5f74fad2831d216c51c2cdad5a",
      "d6c9ac69a9eb3059f5bb6e42b486ebcd",
      "2ddd8c8a1841501964011030e2557e20",
      "97ef2575023dda008711015cf08d7590",
      "d6c9ac69a9eb3059f5bb6e42b486ebcd",
      "2ddd8c8a1841501964011030e2557e20",
      "97ef2575023dda008711015cf08d7590",
      "d69aff1e0d43395ce305c9be0dfb4c89",
      "97ef2575023dda008711015cf08d7590",
      "d69aff1e0d43395ce305c9be0dfb4c89",
      "48786f640191dcbee5b3321672778519",
      "6ad4718230353440b01f2bb78348157e",
      "6ad4718230353440b01f2bb78348157e",
      "ad49bd7af0ea17c84f434c7dfd0a911d",
  };
  return kDigest[id];
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

struct WeightMaskTestParam {
  WeightMaskTestParam(int width, int height, bool mask_is_inverse)
      : width(width), height(height), mask_is_inverse(mask_is_inverse) {}
  int width;
  int height;
  bool mask_is_inverse;
};

std::ostream& operator<<(std::ostream& os, const WeightMaskTestParam& param) {
  return os << param.width << "x" << param.height
            << ", mask_is_inverse: " << param.mask_is_inverse;
}

template <int bitdepth>
class WeightMaskTest : public testing::TestWithParam<WeightMaskTestParam>,
                       public test_utils::MaxAlignedAllocable {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  WeightMaskTest() = default;
  ~WeightMaskTest() override = default;

  void SetUp() override {
    test_utils::ResetDspTable(bitdepth);
    WeightMaskInit_C();
    const dsp::Dsp* const dsp = dsp::GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);
    const int width_index = FloorLog2(width_) - 3;
    const int height_index = FloorLog2(height_) - 3;
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const char* const test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
    } else if (absl::StartsWith(test_case, "NEON/")) {
      WeightMaskInit_NEON();
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      WeightMaskInit_SSE4_1();
    }
    func_ = dsp->weight_mask[width_index][height_index][mask_is_inverse_];
  }

 protected:
  void SetInputData(bool use_fixed_values, int value_1, int value_2);
  void Test(int num_runs, bool use_fixed_values, int value_1, int value_2);

 private:
  const int width_ = GetParam().width;
  const int height_ = GetParam().height;
  const bool mask_is_inverse_ = GetParam().mask_is_inverse;
  using PredType =
      typename std::conditional<bitdepth == 8, int16_t, uint16_t>::type;
  alignas(
      kMaxAlignment) PredType block_1_[kMaxPredictionSize * kMaxPredictionSize];
  alignas(
      kMaxAlignment) PredType block_2_[kMaxPredictionSize * kMaxPredictionSize];
  uint8_t mask_[kMaxPredictionSize * kMaxPredictionSize] = {};
  dsp::WeightMaskFunc func_;
};

template <int bitdepth>
void WeightMaskTest<bitdepth>::SetInputData(const bool use_fixed_values,
                                            const int value_1,
                                            const int value_2) {
  if (use_fixed_values) {
    std::fill(block_1_, block_1_ + kMaxPredictionSize * kMaxPredictionSize,
              value_1);
    std::fill(block_2_, block_2_ + kMaxPredictionSize * kMaxPredictionSize,
              value_2);
  } else {
    constexpr int bitdepth_index = (bitdepth - 8) >> 1;
    libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
    for (int y = 0; y < height_; ++y) {
      for (int x = 0; x < width_; ++x) {
        const int min_val = kCompoundPredictionRange[bitdepth_index][0];
        const int max_val = kCompoundPredictionRange[bitdepth_index][1];
        block_1_[y * width_ + x] =
            static_cast<PredType>(rnd(max_val - min_val) + min_val);
        block_2_[y * width_ + x] =
            static_cast<PredType>(rnd(max_val - min_val) + min_val);
      }
    }
  }
}

BlockSize DimensionsToBlockSize(int width, int height) {
  if (width == 4) {
    if (height == 4) return kBlock4x4;
    if (height == 8) return kBlock4x8;
    if (height == 16) return kBlock4x16;
    return kBlockInvalid;
  }
  if (width == 8) {
    if (height == 4) return kBlock8x4;
    if (height == 8) return kBlock8x8;
    if (height == 16) return kBlock8x16;
    if (height == 32) return kBlock8x32;
    return kBlockInvalid;
  }
  if (width == 16) {
    if (height == 4) return kBlock16x4;
    if (height == 8) return kBlock16x8;
    if (height == 16) return kBlock16x16;
    if (height == 32) return kBlock16x32;
    if (height == 64) return kBlock16x64;
    return kBlockInvalid;
  }
  if (width == 32) {
    if (height == 8) return kBlock32x8;
    if (height == 16) return kBlock32x16;
    if (height == 32) return kBlock32x32;
    if (height == 64) return kBlock32x64;
    return kBlockInvalid;
  }
  if (width == 64) {
    if (height == 16) return kBlock64x16;
    if (height == 32) return kBlock64x32;
    if (height == 64) return kBlock64x64;
    if (height == 128) return kBlock64x128;
    return kBlockInvalid;
  }
  if (width == 128) {
    if (height == 64) return kBlock128x64;
    if (height == 128) return kBlock128x128;
    return kBlockInvalid;
  }
  return kBlockInvalid;
}

template <int bitdepth>
void WeightMaskTest<bitdepth>::Test(const int num_runs,
                                    const bool use_fixed_values,
                                    const int value_1, const int value_2) {
  if (func_ == nullptr) return;
  SetInputData(use_fixed_values, value_1, value_2);
  const absl::Time start = absl::Now();
  for (int i = 0; i < num_runs; ++i) {
    func_(block_1_, block_2_, mask_, width_);
  }
  const absl::Duration elapsed_time = absl::Now() - start;
  if (use_fixed_values) {
    int fixed_value = (value_1 - value_2 == 0) ? 38 : 64;
    if (mask_is_inverse_) fixed_value = 64 - fixed_value;
    for (int y = 0; y < height_; ++y) {
      for (int x = 0; x < width_; ++x) {
        ASSERT_EQ(static_cast<int>(mask_[y * width_ + x]), fixed_value)
            << "x: " << x << " y: " << y;
      }
    }
  } else {
    const int id_offset = mask_is_inverse_ ? kMaxBlockSizes - 4 : 0;
    const int id = id_offset +
                   static_cast<int>(DimensionsToBlockSize(width_, height_)) - 4;
    const char* expected_digest = nullptr;
    switch (bitdepth) {
      case 8:
        expected_digest = GetDigest8bpp(id);
        break;
#if LIBGAV1_MAX_BITDEPTH >= 10
      case 10:
        expected_digest = GetDigest10bpp(id);
        break;
#endif
#if LIBGAV1_MAX_BITDEPTH == 12
      case 12:
        expected_digest = GetDigest12bpp(id);
        break;
#endif
    }
    ASSERT_NE(expected_digest, nullptr);
    test_utils::CheckMd5Digest(
        absl::StrFormat("BlockSize %dx%d", width_, height_).c_str(),
        "WeightMask", expected_digest, mask_, sizeof(mask_), elapsed_time);
  }
}

const WeightMaskTestParam weight_mask_test_param[] = {
    WeightMaskTestParam(8, 8, false),     WeightMaskTestParam(8, 16, false),
    WeightMaskTestParam(8, 32, false),    WeightMaskTestParam(16, 8, false),
    WeightMaskTestParam(16, 16, false),   WeightMaskTestParam(16, 32, false),
    WeightMaskTestParam(16, 64, false),   WeightMaskTestParam(32, 8, false),
    WeightMaskTestParam(32, 16, false),   WeightMaskTestParam(32, 32, false),
    WeightMaskTestParam(32, 64, false),   WeightMaskTestParam(64, 16, false),
    WeightMaskTestParam(64, 32, false),   WeightMaskTestParam(64, 64, false),
    WeightMaskTestParam(64, 128, false),  WeightMaskTestParam(128, 64, false),
    WeightMaskTestParam(128, 128, false), WeightMaskTestParam(8, 8, true),
    WeightMaskTestParam(8, 16, true),     WeightMaskTestParam(8, 32, true),
    WeightMaskTestParam(16, 8, true),     WeightMaskTestParam(16, 16, true),
    WeightMaskTestParam(16, 32, true),    WeightMaskTestParam(16, 64, true),
    WeightMaskTestParam(32, 8, true),     WeightMaskTestParam(32, 16, true),
    WeightMaskTestParam(32, 32, true),    WeightMaskTestParam(32, 64, true),
    WeightMaskTestParam(64, 16, true),    WeightMaskTestParam(64, 32, true),
    WeightMaskTestParam(64, 64, true),    WeightMaskTestParam(64, 128, true),
    WeightMaskTestParam(128, 64, true),   WeightMaskTestParam(128, 128, true),
};

using WeightMaskTest8bpp = WeightMaskTest<8>;

TEST_P(WeightMaskTest8bpp, FixedValues) {
  const int min = kCompoundPredictionRange[0][0];
  const int max = kCompoundPredictionRange[0][1];
  Test(1, true, min, min);
  Test(1, true, min, max);
  Test(1, true, max, min);
  Test(1, true, max, max);
}

TEST_P(WeightMaskTest8bpp, RandomValues) { Test(1, false, -1, -1); }

TEST_P(WeightMaskTest8bpp, DISABLED_Speed) {
  Test(kNumSpeedTests, false, -1, -1);
}

INSTANTIATE_TEST_SUITE_P(C, WeightMaskTest8bpp,
                         testing::ValuesIn(weight_mask_test_param));
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, WeightMaskTest8bpp,
                         testing::ValuesIn(weight_mask_test_param));
#endif
#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, WeightMaskTest8bpp,
                         testing::ValuesIn(weight_mask_test_param));
#endif

#if LIBGAV1_MAX_BITDEPTH >= 10
using WeightMaskTest10bpp = WeightMaskTest<10>;

TEST_P(WeightMaskTest10bpp, FixedValues) {
  const int min = kCompoundPredictionRange[1][0];
  const int max = kCompoundPredictionRange[1][1];
  Test(1, true, min, min);
  Test(1, true, min, max);
  Test(1, true, max, min);
  Test(1, true, max, max);
}

TEST_P(WeightMaskTest10bpp, RandomValues) { Test(1, false, -1, -1); }

TEST_P(WeightMaskTest10bpp, DISABLED_Speed) {
  Test(kNumSpeedTests, false, -1, -1);
}

INSTANTIATE_TEST_SUITE_P(C, WeightMaskTest10bpp,
                         testing::ValuesIn(weight_mask_test_param));
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, WeightMaskTest10bpp,
                         testing::ValuesIn(weight_mask_test_param));
#endif
#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, WeightMaskTest10bpp,
                         testing::ValuesIn(weight_mask_test_param));
#endif
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using WeightMaskTest12bpp = WeightMaskTest<12>;

TEST_P(WeightMaskTest12bpp, FixedValues) {
  const int min = kCompoundPredictionRange[2][0];
  const int max = kCompoundPredictionRange[2][1];
  Test(1, true, min, min);
  Test(1, true, min, max);
  Test(1, true, max, min);
  Test(1, true, max, max);
}

TEST_P(WeightMaskTest12bpp, RandomValues) { Test(1, false, -1, -1); }

TEST_P(WeightMaskTest12bpp, DISABLED_Speed) {
  Test(kNumSpeedTests, false, -1, -1);
}

INSTANTIATE_TEST_SUITE_P(C, WeightMaskTest12bpp,
                         testing::ValuesIn(weight_mask_test_param));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace
}  // namespace dsp
}  // namespace libgav1
