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
      "035267cb2ac5a0f8ff50c2d30ad52226",
      "3231f4972dd858b734e0cc48c4cd001e",
      "7e163b69721a13ec9f75b5cd74ffee3f",
      "" /*kBlock4x16*/,
      "b75e90abc224acca8754c82039b3ba93",
      "9f555f3a2c1a933a663d6103b8118dea",
      "8539e54f34cd6668ff6e6606210be201",
      "20f85c9db7c878c21fbf2052936f269e",
      "620ec166de57b0639260b2d72eebfc3e",
      "be666394b5a894d78f4097b6cca272fe",
      "57a96816e84cdb381f596c23827b5922",
      "f2e0d348f608f246b6d8d799b66c189e",
      "161ac051f38372d9339d36728b9926ba",
      "d5fad48aaf132a81cb62bba4f07bbebb",
      "e10be2dca2f7dae38dae75150fc1612d",
      "7f744481eb551bbc224b5236c82cbade",
      "0d99bbf31ecddc1c2d5063a68c0e9375",
      "5fb8ec5f582f0ebfe519ed55860f67c4",

      // mask_is_inverse = true.
      "a4250ca39daa700836138371d36d465f",
      "abe9a9a1c3a5accda9bfefd4d6e81ccb",
      "e95b08878d0bb5f2293c27c3a6fe0253",
      "" /*kBlock4x16*/,
      "e1c52be02ce9ab2800015bb08b866c31",
      "eea1dc73811f73866edfeb4555865f20",
      "3178e64085645bd819256a8ab43c7b0a",
      "ee83884e4d5cd2c9ac04879116bab681",
      "d107eff7d5ae9ba14d2c6b3b8d9fca49",
      "400aeea7d299626fc336c46b1ad7a9d8",
      "e9e26a400f67f3ad36350fe4171fc613",
      "4c31ad714f470f34127febaf1bac714b",
      "bbdcb1097c66d561dd4ea16b3fb73f97",
      "3a21dfbf53e4c964e303a75a3308ce15",
      "3416dab4512fd0dc61d788b433cd624e",
      "68ace8f01fdd74aec3fee528c8167738",
      "9fabe05a6523da81a45150e19f75acff",
      "7c0643e4d02421d06d7ca71822a94e1d",
  };
  return kDigest[id];
}

#if LIBGAV1_MAX_BITDEPTH >= 10
const char* GetDigest10bpp(int id) {
  static const char* const kDigest[] = {
      "1dc9bdd042e5228705b857b42798e364",
      "c054c8644bd482ce78a139d8e063e013",
      "bbe4ac48f013f34c84779da05b0bcbe0",
      "" /*kBlock4x16*/,
      "13d4759277637a607f25439182553708",
      "f089667610561a47d50f9f930ad7c454",
      "46715e6f7819f59725bdb083f4403255",
      "3774541c339ae3af920ef2b1d6abf6a1",
      "94913b01d226cb5eb273dfee84b51f65",
      "be0c0847629dfff8e0e991ed67697a7d",
      "716b5398b77d7459274d4ea9c91ebd8e",
      "f5c1b0b461df4182529949472242b421",
      "5e9576ea4cf107249ce4ae89a72b9c95",
      "da021bcdf7936f7bd9a2399c69e4d37c",
      "b3a310a39c1900e00f992839ff188656",
      "9f3a15351af5945615f296242ec56a38",
      "b6e0bd03c521c5f00e90530daa7d4432",
      "3270d7f621d488aec5b76bcf121debd0",

      // mask_is_inverse = true.
      "33df96dd246683133eefe4caea6e3f7d",
      "73e0ccc5d42806548a4b59f856256c1e",
      "3561a0358cf831aee9477d07feafae2d",
      "" /*kBlock4x16*/,
      "c5a2e633c0cd6925e68f21f47f0e2d84",
      "8755a2d3840dde5fd6a0cce6bd6642c5",
      "85ec538b72cecd6ea1fddab5ce3b4e64",
      "a53e0dec84c675c4c6b1f5792b0232ff",
      "86180da325f9727670a98cf2dbf7410e",
      "a5fdc95104948047e179b2bc3d47f51d",
      "9b95b3858187838e4669180e2ddb295e",
      "6e40ca55608f6bf2f8cd91c8dbf3ddbf",
      "d3a092672e921b588279d57e50b31888",
      "9883eb19b733ee9f1cb6a6b6a1a00bb5",
      "dd34764e068b228b7820321b06864e63",
      "6c743dc9c8c87c7044151d29993e5042",
      "44925dab01011a98b8ab1f0308fa852a",
      "6d984b2ccfa056278e2130771127a943",
  };
  return kDigest[id];
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

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
    func_(block_1_, block_2_, mask_, kMaxPredictionSize);
  }
  const absl::Duration elapsed_time = absl::Now() - start;
  if (use_fixed_values) {
    int fixed_value = (value_1 - value_2 == 0) ? 38 : 64;
    if (mask_is_inverse_) fixed_value = 64 - fixed_value;
    for (int y = 0; y < height_; ++y) {
      for (int x = 0; x < width_; ++x) {
        ASSERT_EQ(static_cast<int>(mask_[y * kMaxPredictionSize + x]),
                  fixed_value)
            << "x: " << x << " y: " << y;
      }
    }
  } else {
    const int id_offset = mask_is_inverse_ ? kMaxBlockSizes - 4 : 0;
    const int id = id_offset +
                   static_cast<int>(DimensionsToBlockSize(width_, height_)) - 4;
    if (bitdepth == 8) {
      test_utils::CheckMd5Digest(
          absl::StrFormat("BlockSize %dx%d", width_, height_).c_str(),
          "WeightMask", GetDigest8bpp(id), mask_, sizeof(mask_), elapsed_time);
#if LIBGAV1_MAX_BITDEPTH >= 10
    } else {
      test_utils::CheckMd5Digest(
          absl::StrFormat("BlockSize %dx%d", width_, height_).c_str(),
          "WeightMask", GetDigest10bpp(id), mask_, sizeof(mask_), elapsed_time);
#endif
    }
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

}  // namespace
}  // namespace dsp
}  // namespace libgav1
