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

#include "src/dsp/warp.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ostream>
#include <string>
#include <type_traits>

#include "absl/base/macros.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "gtest/gtest.h"
#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/post_filter.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/cpu.h"
#include "src/utils/memory.h"
#include "tests/block_utils.h"
#include "tests/third_party/libvpx/acm_random.h"
#include "tests/utils.h"

namespace libgav1 {
namespace dsp {
namespace {

constexpr int kSourceBorderHorizontal = 16;
constexpr int kSourceBorderVertical = 13;

constexpr int kMaxSourceBlockWidth =
    kMaxSuperBlockSizeInPixels + kSourceBorderHorizontal * 2;
constexpr int kMaxSourceBlockHeight =
    kMaxSuperBlockSizeInPixels + kSourceBorderVertical * 2;
constexpr int kMaxDestBlockWidth =
    kMaxSuperBlockSizeInPixels + kConvolveBorderLeftTop * 2;
constexpr int kMaxDestBlockHeight =
    kMaxSuperBlockSizeInPixels + kConvolveBorderLeftTop * 2;

constexpr uint16_t kDivisorLookup[257] = {
    16384, 16320, 16257, 16194, 16132, 16070, 16009, 15948, 15888, 15828, 15768,
    15709, 15650, 15592, 15534, 15477, 15420, 15364, 15308, 15252, 15197, 15142,
    15087, 15033, 14980, 14926, 14873, 14821, 14769, 14717, 14665, 14614, 14564,
    14513, 14463, 14413, 14364, 14315, 14266, 14218, 14170, 14122, 14075, 14028,
    13981, 13935, 13888, 13843, 13797, 13752, 13707, 13662, 13618, 13574, 13530,
    13487, 13443, 13400, 13358, 13315, 13273, 13231, 13190, 13148, 13107, 13066,
    13026, 12985, 12945, 12906, 12866, 12827, 12788, 12749, 12710, 12672, 12633,
    12596, 12558, 12520, 12483, 12446, 12409, 12373, 12336, 12300, 12264, 12228,
    12193, 12157, 12122, 12087, 12053, 12018, 11984, 11950, 11916, 11882, 11848,
    11815, 11782, 11749, 11716, 11683, 11651, 11619, 11586, 11555, 11523, 11491,
    11460, 11429, 11398, 11367, 11336, 11305, 11275, 11245, 11215, 11185, 11155,
    11125, 11096, 11067, 11038, 11009, 10980, 10951, 10923, 10894, 10866, 10838,
    10810, 10782, 10755, 10727, 10700, 10673, 10645, 10618, 10592, 10565, 10538,
    10512, 10486, 10460, 10434, 10408, 10382, 10356, 10331, 10305, 10280, 10255,
    10230, 10205, 10180, 10156, 10131, 10107, 10082, 10058, 10034, 10010, 9986,
    9963,  9939,  9916,  9892,  9869,  9846,  9823,  9800,  9777,  9754,  9732,
    9709,  9687,  9664,  9642,  9620,  9598,  9576,  9554,  9533,  9511,  9489,
    9468,  9447,  9425,  9404,  9383,  9362,  9341,  9321,  9300,  9279,  9259,
    9239,  9218,  9198,  9178,  9158,  9138,  9118,  9098,  9079,  9059,  9039,
    9020,  9001,  8981,  8962,  8943,  8924,  8905,  8886,  8867,  8849,  8830,
    8812,  8793,  8775,  8756,  8738,  8720,  8702,  8684,  8666,  8648,  8630,
    8613,  8595,  8577,  8560,  8542,  8525,  8508,  8490,  8473,  8456,  8439,
    8422,  8405,  8389,  8372,  8355,  8339,  8322,  8306,  8289,  8273,  8257,
    8240,  8224,  8208,  8192};

template <bool is_compound>
const char* GetDigest8bpp(int id) {
  static const char* const kDigest[] = {
      "77ba358a0f5e19a8e69fa0a95712578e", "141b23d13a04e0b84d26d514de76d6b0",
      "b0265858454b979852ffadae323f0fb7", "9cf38e3579265b656f1f2100ba15b0e9",
      "ab51d05cc255ef8e37921182df1d89b1", "e3e96f90a4b07ca733e40f057dc01c41",
      "4eee8c1a52a62a266db9b1c9338e124c", "901a87d8f88f6324dbc0960a6de861ac",
      "da9cb6faf6adaeeae12b6784f39186c5", "14450ab05536cdb0d2f499716ccb559d",
      "566b396cbf008bbb869b364fdc81860d", "681a872baf2de4e58d73ea9ab8643a72",
      "7f17d290d513a7416761b3a01f10fd2f",
  };
  static const char* const kCompoundDigest[] = {
      "7e9339d265b7beac7bbe32fe7bb0fccb", "f747d663b427bb38a3ff36b0815a394c",
      "858cf54d2253281a919fbdb48fe91c53", "4721dd97a212c6068bd488f400259afc",
      "36878c7906492bc740112abdea77616f", "89deb68aa35764bbf3024b501a6bed50",
      "8ac5b08f9b2afd38143c357646af0f82", "bf6e2a64835ea0c9d7467394253d0eb2",
      "7b0a539acd2a27eff398dd084abad933", "61c8d81b397c1cf727ff8a9fabab90af",
      "4d412349a25a832c1fb3fb29e3f0e2b3", "2c6dd2a9a4ede9fa00adb567ba646f30",
      "b2a0ce68db3cadd207299f73112bed74",
  };
  assert(id >= 0);
  assert(id < sizeof(kDigest) / sizeof(kDigest[0]));
  return is_compound ? kCompoundDigest[id] : kDigest[id];
}

#if LIBGAV1_MAX_BITDEPTH >= 10
template <bool is_compound>
const char* GetDigest10bpp(int id) {
  static const char* const kDigest[] = {
      "1fef54f56a0bafccf7f8da1ac3b18b76", "8a65c72f171feafa2f393d31d6b7fe1b",
      "808019346f2f1f45f8cf2e9fc9a49320", "c28e2f2c6c830a29bcc2452166cba521",
      "f040674d6f54e8910d655f0d11fd8cdd", "473af9bb1c6023965c2284b716feef97",
      "e4f6d7babd0813d5afb0f575ebfa8166", "58f96ef8a880963a213624bb0d06d47c",
      "1ec0995fa4490628b679d03683233388", "9526fb102fde7dc1a7e160e65af6da33",
      "f0457427d0c0e31d82ea4f612f7f86f1", "ddc82ae298cccebad493ba9de0f69fbd",
      "5ed615091e2f62df26de7e91a985cb81",
  };
  static const char* const kCompoundDigest[] = {
      "8e6986ae143260e0b8b4887f15a141a1", "0a7f0db8316b8c3569f08834dd0c6f50",
      "90705b2e7dbe083e8a1f70f29d6f257e", "e428a75bea77d769d21f3f7a1d2b0b38",
      "a570b13d790c085c4ab50d71dd085d56", "e5d043c6cd6ff6dbab6e38a8877e93bd",
      "12ea96991e46e3e9aa78ab812ffa0525", "84293a94a53f1cf814fa25e793c3fe27",
      "b98a7502c84ac8437266f702dcc0a92e", "d8db5d52e9b0a5be0ad2d517d5bd16e9",
      "f3be504bbb609ce4cc71c5539252638a", "fcde83b54e14e9de23460644f244b047",
      "42eb66e752e9ef289b47053b5c73fdd6",
  };
  assert(id >= 0);
  assert(id < sizeof(kDigest) / sizeof(kDigest[0]));
  return is_compound ? kCompoundDigest[id] : kDigest[id];
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
template <bool is_compound>
const char* GetDigest12bpp(int id) {
  static const char* const kDigest[] = {
      "cd5d5e2102b8917ad70778f523d24bdf", "374a5f1b53a3fdf2eefa741eb71e6889",
      "311636841770ec2427084891df96bee5", "c40c537917b1f0d1d84c99dfcecd8219",
      "a1d9bb920e6c3d20c0cf84adc18e1f15", "13b5659acdb39b717526cb358c6f4026",
      "f81ea4f6fd1f4ebed1262e3fae37b5bb", "c1452fefcd9b9562fe3a0b7f9302809c",
      "8fed8a3159dc7b6b59a39ab2be6bee13", "b46458bc0e5cf1cee92aac4f0f608749",
      "2e6a1039ab111add89f5b44b13565f40", "9c666691860bdc89b03f601b40126196",
      "418a47157d992b94c302ca2e2f6ee07e",
  };
  static const char* const kCompoundDigest[] = {
      "8e6986ae143260e0b8b4887f15a141a1", "0a7f0db8316b8c3569f08834dd0c6f50",
      "90705b2e7dbe083e8a1f70f29d6f257e", "e428a75bea77d769d21f3f7a1d2b0b38",
      "a570b13d790c085c4ab50d71dd085d56", "e5d043c6cd6ff6dbab6e38a8877e93bd",
      "12ea96991e46e3e9aa78ab812ffa0525", "84293a94a53f1cf814fa25e793c3fe27",
      "b98a7502c84ac8437266f702dcc0a92e", "d8db5d52e9b0a5be0ad2d517d5bd16e9",
      "f3be504bbb609ce4cc71c5539252638a", "fcde83b54e14e9de23460644f244b047",
      "42eb66e752e9ef289b47053b5c73fdd6",
  };
  assert(id >= 0);
  assert(id < sizeof(kDigest) / sizeof(kDigest[0]));
  return is_compound ? kCompoundDigest[id] : kDigest[id];
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

int RandomWarpedParam(int seed_offset, int bits) {
  libvpx_test::ACMRandom rnd(seed_offset +
                             libvpx_test::ACMRandom::DeterministicSeed());
  // 1 in 8 chance of generating zero (arbitrary).
  const bool zero = (rnd.Rand16() & 7) == 0;
  if (zero) return 0;
  // Generate uniform values in the range [-(1 << bits), 1] U [1, 1 <<
  // bits].
  const int mask = (1 << bits) - 1;
  const int value = 1 + (rnd.RandRange(1u << 31) & mask);
  const bool sign = (rnd.Rand16() & 1) != 0;
  return sign ? value : -value;
}

// This function is a copy from warp_prediction.cc.
template <typename T>
void GenerateApproximateDivisor(T value, int16_t* division_factor,
                                int16_t* division_shift) {
  const int n = FloorLog2(std::abs(value));
  const T e = std::abs(value) - (static_cast<T>(1) << n);
  const int entry = (n > kDivisorLookupBits)
                        ? RightShiftWithRounding(e, n - kDivisorLookupBits)
                        : static_cast<int>(e << (kDivisorLookupBits - n));
  *division_shift = n + kDivisorLookupPrecisionBits;
  *division_factor =
      (value < 0) ? -kDivisorLookup[entry] : kDivisorLookup[entry];
}

// This function is a copy from warp_prediction.cc.
int16_t GetShearParameter(int value) {
  return static_cast<int16_t>(
      LeftShift(RightShiftWithRoundingSigned(value, kWarpParamRoundingBits),
                kWarpParamRoundingBits));
}

// This function is a copy from warp_prediction.cc.
// This function is used here to help generate valid warp parameters.
bool SetupShear(const int* params, int16_t* alpha, int16_t* beta,
                int16_t* gamma, int16_t* delta) {
  int16_t division_shift;
  int16_t division_factor;
  GenerateApproximateDivisor<int32_t>(params[2], &division_factor,
                                      &division_shift);
  const int alpha0 =
      Clip3(params[2] - (1 << kWarpedModelPrecisionBits), INT16_MIN, INT16_MAX);
  const int beta0 = Clip3(params[3], INT16_MIN, INT16_MAX);
  const int64_t v = LeftShift(params[4], kWarpedModelPrecisionBits);
  const int gamma0 =
      Clip3(RightShiftWithRoundingSigned(v * division_factor, division_shift),
            INT16_MIN, INT16_MAX);
  const int64_t w = static_cast<int64_t>(params[3]) * params[4];
  const int delta0 = Clip3(
      params[5] -
          RightShiftWithRoundingSigned(w * division_factor, division_shift) -
          (1 << kWarpedModelPrecisionBits),
      INT16_MIN, INT16_MAX);

  *alpha = GetShearParameter(alpha0);
  *beta = GetShearParameter(beta0);
  *gamma = GetShearParameter(gamma0);
  *delta = GetShearParameter(delta0);
  if ((4 * std::abs(*alpha) + 7 * std::abs(*beta) >=
       (1 << kWarpedModelPrecisionBits)) ||
      (4 * std::abs(*gamma) + 4 * std::abs(*delta) >=
       (1 << kWarpedModelPrecisionBits))) {
    return false;  // NOLINT (easier condition to understand).
  }

  return true;
}

void GenerateWarpedModel(int* params, int16_t* alpha, int16_t* beta,
                         int16_t* gamma, int16_t* delta, int seed) {
  do {
    params[0] = RandomWarpedParam(seed, kWarpedModelPrecisionBits + 6);
    params[1] = RandomWarpedParam(seed, kWarpedModelPrecisionBits + 6);
    params[2] = RandomWarpedParam(seed, kWarpedModelPrecisionBits - 3) +
                (1 << kWarpedModelPrecisionBits);
    params[3] = RandomWarpedParam(seed, kWarpedModelPrecisionBits - 3);
    params[4] = RandomWarpedParam(seed, kWarpedModelPrecisionBits - 3);
    params[5] = RandomWarpedParam(seed, kWarpedModelPrecisionBits - 3) +
                (1 << kWarpedModelPrecisionBits);
    ++seed;
  } while (params[2] == 0 || !SetupShear(params, alpha, beta, gamma, delta));
}

struct WarpTestParam {
  WarpTestParam(int width, int height) : width(width), height(height) {}
  int width;
  int height;
};

template <bool is_compound, int bitdepth, typename Pixel>
class WarpTest : public testing::TestWithParam<WarpTestParam> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  WarpTest() = default;
  ~WarpTest() override = default;

  void SetUp() override {
    test_utils::ResetDspTable(bitdepth);
    WarpInit_C();
    const dsp::Dsp* const dsp = dsp::GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const absl::string_view test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
    } else if (absl::StartsWith(test_case, "NEON/")) {
      WarpInit_NEON();
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      WarpInit_SSE4_1();
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }
    func_ = is_compound ? dsp->warp_compound : dsp->warp;
  }

 protected:
  using DestType =
      typename std::conditional<is_compound, uint16_t, Pixel>::type;

  void SetInputData(bool use_fixed_values, int value);
  void Test(bool use_fixed_values, int value, int num_runs = 1);
  void TestFixedValues();
  void TestRandomValues();
  void TestSpeed();

  const WarpTestParam param_ = GetParam();

 private:
  int warp_params_[8];
  dsp::WarpFunc func_;
  // Warp filters are 7-tap, which needs 3 pixels (kConvolveBorderLeftTop)
  // padding. Destination buffer indices are based on subsampling values (x+y):
  // 0: (4:4:4), 1:(4:2:2), 2: (4:2:0).
  Pixel source_[kMaxSourceBlockHeight * kMaxSourceBlockWidth] = {};
  DestType dest_[3][kMaxDestBlockHeight * kMaxDestBlockWidth] = {};
};

template <bool is_compound, int bitdepth, typename Pixel>
void WarpTest<is_compound, bitdepth, Pixel>::SetInputData(bool use_fixed_values,
                                                          int value) {
  if (use_fixed_values) {
    for (int y = 0; y < param_.height; ++y) {
      const int row = kSourceBorderVertical + y;
      Memset(source_ + row * kMaxSourceBlockWidth + kSourceBorderHorizontal,
             value, param_.width);
    }
  } else {
    const int mask = (1 << bitdepth) - 1;
    libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
    for (int y = 0; y < param_.height; ++y) {
      const int row = kSourceBorderVertical + y;
      for (int x = 0; x < param_.width; ++x) {
        const int column = kSourceBorderHorizontal + x;
        source_[row * kMaxSourceBlockWidth + column] = rnd.Rand16() & mask;
      }
    }
  }
  PostFilter::ExtendFrame<Pixel>(
      &source_[kSourceBorderVertical * kMaxSourceBlockWidth +
               kSourceBorderHorizontal],
      param_.width, param_.height, kMaxSourceBlockWidth,
      kSourceBorderHorizontal, kSourceBorderHorizontal, kSourceBorderVertical,
      kSourceBorderVertical);
}

template <bool is_compound, int bitdepth, typename Pixel>
void WarpTest<is_compound, bitdepth, Pixel>::Test(bool use_fixed_values,
                                                  int value,
                                                  int num_runs /*= 1*/) {
  if (func_ == nullptr) return;
  SetInputData(use_fixed_values, value);
  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
  const int source_offset =
      kSourceBorderVertical * kMaxSourceBlockWidth + kSourceBorderHorizontal;
  const int dest_offset =
      kConvolveBorderLeftTop * kMaxDestBlockWidth + kConvolveBorderLeftTop;
  const Pixel* const src = source_ + source_offset;
  const ptrdiff_t src_stride = kMaxSourceBlockWidth * sizeof(Pixel);
  const ptrdiff_t dst_stride =
      is_compound ? kMaxDestBlockWidth : kMaxDestBlockWidth * sizeof(Pixel);

  absl::Duration elapsed_time;
  for (int subsampling_x = 0; subsampling_x <= 1; ++subsampling_x) {
    for (int subsampling_y = 0; subsampling_y <= 1; ++subsampling_y) {
      if (subsampling_x == 0 && subsampling_y == 1) {
        // When both are 0: 4:4:4
        // When both are 1: 4:2:0
        // When only |subsampling_x| is 1: 4:2:2
        // Having only |subsampling_y| == 1 is unsupported.
        continue;
      }
      int params[8];
      int16_t alpha;
      int16_t beta;
      int16_t gamma;
      int16_t delta;
      GenerateWarpedModel(params, &alpha, &beta, &gamma, &delta, rnd.Rand8());

      const int dest_id = subsampling_x + subsampling_y;
      DestType* const dst = dest_[dest_id] + dest_offset;
      const absl::Time start = absl::Now();
      for (int n = 0; n < num_runs; ++n) {
        func_(src, src_stride, param_.width, param_.height, params,
              subsampling_x, subsampling_y, 0, 0, param_.width, param_.height,
              alpha, beta, gamma, delta, dst, dst_stride);
      }
      elapsed_time += absl::Now() - start;
    }
  }

  if (use_fixed_values) {
    // For fixed values, input and output are identical.
    for (size_t i = 0; i < ABSL_ARRAYSIZE(dest_); ++i) {
      // |is_compound| holds a few more bits of precision and an offset value.
      Pixel compensated_dest[kMaxDestBlockWidth * kMaxDestBlockHeight];
      const int compound_offset = (bitdepth == 8) ? 0 : kCompoundOffset;
      if (is_compound) {
        for (int y = 0; y < param_.height; ++y) {
          for (int x = 0; x < param_.width; ++x) {
            const int compound_value =
                dest_[i][dest_offset + y * kMaxDestBlockWidth + x];
            const int remove_offset = compound_value - compound_offset;
            const int full_shift =
                remove_offset >>
                (kInterRoundBitsVertical - kInterRoundBitsCompoundVertical);
            compensated_dest[y * kMaxDestBlockWidth + x] =
                Clip3(full_shift, 0, (1 << bitdepth) - 1);
          }
        }
      }
      Pixel* pixel_dest =
          is_compound ? compensated_dest
                      : reinterpret_cast<Pixel*>(dest_[i] + dest_offset);
      const bool success = test_utils::CompareBlocks(
          src, pixel_dest, param_.width, param_.height, kMaxSourceBlockWidth,
          kMaxDestBlockWidth, false);
      EXPECT_TRUE(success) << "subsampling_x + subsampling_y: " << i;
    }
  } else {
    // (width, height):
    // (8, 8), id = 0. (8, 16), id = 1. (16, 8), id = 2.
    // (16, 16), id = 3. (16, 32), id = 4. (32, 16), id = 5.
    // ...
    // (128, 128), id = 12.
    int id;
    if (param_.width == param_.height) {
      id = 3 * static_cast<int>(FloorLog2(param_.width) - 3);
    } else if (param_.width < param_.height) {
      id = 1 + 3 * static_cast<int>(FloorLog2(param_.width) - 3);
    } else {
      id = 2 + 3 * static_cast<int>(FloorLog2(param_.height) - 3);
    }

    const char* expected_digest = nullptr;
    switch (bitdepth) {
      case 8:
        expected_digest = GetDigest8bpp<is_compound>(id);
        break;
#if LIBGAV1_MAX_BITDEPTH >= 10
      case 10:
        expected_digest = GetDigest10bpp<is_compound>(id);
        break;
#endif
#if LIBGAV1_MAX_BITDEPTH == 12
      case 12:
        expected_digest = GetDigest12bpp<is_compound>(id);
        break;
#endif
    }
    ASSERT_NE(expected_digest, nullptr);
    test_utils::CheckMd5Digest(
        "Warp", absl::StrFormat("%dx%d", param_.width, param_.height).c_str(),
        expected_digest, dest_, sizeof(dest_), elapsed_time);
  }
}

template <bool is_compound, int bitdepth, typename Pixel>
void WarpTest<is_compound, bitdepth, Pixel>::TestFixedValues() {
  Test(true, 0);
  Test(true, 1);
  Test(true, 128);
  Test(true, (1 << bitdepth) - 1);
}

template <bool is_compound, int bitdepth, typename Pixel>
void WarpTest<is_compound, bitdepth, Pixel>::TestRandomValues() {
  Test(false, 0);
}

template <bool is_compound, int bitdepth, typename Pixel>
void WarpTest<is_compound, bitdepth, Pixel>::TestSpeed() {
  const int num_runs = static_cast<int>(1.0e7 / (param_.width * param_.height));
  Test(false, 0, num_runs);
}

void ApplyFilterToSignedInput(const int min_input, const int max_input,
                              const int8_t filter[kSubPixelTaps],
                              int* min_output, int* max_output) {
  int min = 0, max = 0;
  for (int i = 0; i < kSubPixelTaps; ++i) {
    const int tap = filter[i];
    if (tap > 0) {
      max += max_input * tap;
      min += min_input * tap;
    } else {
      min += max_input * tap;
      max += min_input * tap;
    }
  }
  *min_output = min;
  *max_output = max;
}

void ApplyFilterToUnsignedInput(const int max_input,
                                const int8_t filter[kSubPixelTaps],
                                int* min_output, int* max_output) {
  ApplyFilterToSignedInput(0, max_input, filter, min_output, max_output);
}

// Validate the maximum ranges for different parts of the Warp process.
template <int bitdepth>
void ShowRange() {
  constexpr int horizontal_bits = (bitdepth == kBitdepth12)
                                      ? kInterRoundBitsHorizontal12bpp
                                      : kInterRoundBitsHorizontal;
  constexpr int vertical_bits = (bitdepth == kBitdepth12)
                                    ? kInterRoundBitsVertical12bpp
                                    : kInterRoundBitsVertical;
  constexpr int compound_vertical_bits = kInterRoundBitsCompoundVertical;

  constexpr int compound_offset = (bitdepth == 8) ? 0 : kCompoundOffset;

  constexpr int max_input = (1 << bitdepth) - 1;

  const int8_t* worst_warp_filter = kWarpedFilters8[93];

  // First pass.
  printf("Bitdepth: %2d Input range:            [%8d, %8d]\n", bitdepth, 0,
         max_input);

  int min = 0, max = 0;
  ApplyFilterToUnsignedInput(max_input, worst_warp_filter, &min, &max);

  int first_pass_offset;
  if (bitdepth == 8) {
    // Derive an offset for 8 bit.
    for (first_pass_offset = 1; - first_pass_offset > min;
         first_pass_offset <<= 1) {
    }
    printf("  8bpp intermediate offset: %d.\n", first_pass_offset);
    min += first_pass_offset;
    max += first_pass_offset;
    assert(min > 0);
    assert(max < UINT16_MAX);
  } else {
    // 10bpp and 12bpp require int32_t for the intermediate values. Adding an
    // offset is not required.
    assert(min > INT32_MIN);
    assert(max > INT16_MAX && max < INT32_MAX);
  }

  printf("  intermediate range:                [%8d, %8d]\n", min, max);

  const int first_pass_min = RightShiftWithRounding(min, horizontal_bits);
  const int first_pass_max = RightShiftWithRounding(max, horizontal_bits);

  printf("  first pass output range:           [%8d, %8d]\n", first_pass_min,
         first_pass_max);

  // Second pass.
  if (bitdepth == 8) {
    ApplyFilterToUnsignedInput(first_pass_max, worst_warp_filter, &min, &max);
  } else {
    ApplyFilterToSignedInput(first_pass_min, first_pass_max, worst_warp_filter,
                             &min, &max);
  }

  if (bitdepth == 8) {
    // Remove the offset that was applied in the first pass since we must use
    // int32_t for this phase anyway. 128 is the sum of the filter taps.
    const int offset_removal = (first_pass_offset >> horizontal_bits) * 128;
    printf("  8bpp intermediate offset removal: %d.\n", offset_removal);
    max -= offset_removal;
    min -= offset_removal;
    assert(min < INT16_MIN && min > INT32_MIN);
    assert(max > INT16_MAX && max < INT32_MAX);
  } else {
    // 10bpp and 12bpp require int32_t for the intermediate values. Adding an
    // offset is not required.
    assert(min > INT32_MIN);
    assert(max > INT16_MAX && max < INT32_MAX);
  }

  printf("  intermediate range:                [%8d, %8d]\n", min, max);

  // Second pass non-compound output is clipped to Pixel values.
  const int second_pass_min =
      Clip3(RightShiftWithRounding(min, vertical_bits), 0, max_input);
  const int second_pass_max =
      Clip3(RightShiftWithRounding(max, vertical_bits), 0, max_input);
  printf("  second pass output range:          [%8d, %8d]\n", second_pass_min,
         second_pass_max);

  // Output is Pixel so matches Pixel values.
  assert(second_pass_min == 0);
  assert(second_pass_max == max_input);

  const int compound_second_pass_min =
      RightShiftWithRounding(min, compound_vertical_bits) + compound_offset;
  const int compound_second_pass_max =
      RightShiftWithRounding(max, compound_vertical_bits) + compound_offset;

  printf("  compound second pass output range: [%8d, %8d]\n",
         compound_second_pass_min, compound_second_pass_max);

  if (bitdepth == 8) {
    // 8bpp output is int16_t without an offset.
    assert(compound_second_pass_min > INT16_MIN);
    assert(compound_second_pass_max < INT16_MAX);
  } else {
    // 10bpp and 12bpp use the offset to fit inside uint16_t.
    assert(compound_second_pass_min > 0);
    assert(compound_second_pass_max < UINT16_MAX);
  }

  printf("\n");
}

TEST(WarpTest, ShowRange) {
  ShowRange<kBitdepth8>();
  ShowRange<kBitdepth10>();
  ShowRange<kBitdepth12>();
}

using WarpTest8bpp = WarpTest</*is_compound=*/false, 8, uint8_t>;
// TODO(jzern): Coverage could be added for kInterRoundBitsCompoundVertical via
// WarpCompoundTest.
// using WarpCompoundTest8bpp = WarpTest</*is_compound=*/true, 8, uint8_t>;

// Verifies the sum of the warped filter coefficients is 128 for every filter.
//
// Verifies the properties used in the calculation of ranges of variables in
// the block warp process:
// * The maximum sum of the positive warped filter coefficients is 175.
// * The minimum (i.e., most negative) sum of the negative warped filter
//   coefficients is -47.
//
// NOTE: This test is independent of the bitdepth and the implementation of the
// block warp function, so it just needs to be a test in the WarpTest8bpp class
// and does not need to be defined with TEST_P.
TEST(WarpTest8bpp, WarpedFilterCoefficientSums) {
  int max_positive_sum = 0;
  int min_negative_sum = 0;
  for (const auto& filter : kWarpedFilters) {
    int sum = 0;
    int positive_sum = 0;
    int negative_sum = 0;
    for (const auto coefficient : filter) {
      sum += coefficient;
      if (coefficient > 0) {
        positive_sum += coefficient;
      } else {
        negative_sum += coefficient;
      }
    }
    EXPECT_EQ(sum, 128);
    max_positive_sum = std::max(positive_sum, max_positive_sum);
    min_negative_sum = std::min(negative_sum, min_negative_sum);
  }
  EXPECT_EQ(max_positive_sum, 175);
  EXPECT_EQ(min_negative_sum, -47);
}

TEST_P(WarpTest8bpp, FixedValues) { TestFixedValues(); }

TEST_P(WarpTest8bpp, RandomValues) { TestRandomValues(); }

TEST_P(WarpTest8bpp, DISABLED_Speed) { TestSpeed(); }
const WarpTestParam warp_test_param[] = {
    WarpTestParam(8, 8),     WarpTestParam(8, 16),   WarpTestParam(16, 8),
    WarpTestParam(16, 16),   WarpTestParam(16, 32),  WarpTestParam(32, 16),
    WarpTestParam(32, 32),   WarpTestParam(32, 64),  WarpTestParam(64, 32),
    WarpTestParam(64, 64),   WarpTestParam(64, 128), WarpTestParam(128, 64),
    WarpTestParam(128, 128),
};

INSTANTIATE_TEST_SUITE_P(C, WarpTest8bpp, testing::ValuesIn(warp_test_param));

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, WarpTest8bpp,
                         testing::ValuesIn(warp_test_param));
#endif

#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, WarpTest8bpp,
                         testing::ValuesIn(warp_test_param));
#endif

#if LIBGAV1_MAX_BITDEPTH >= 10
using WarpTest10bpp = WarpTest</*is_compound=*/false, 10, uint16_t>;
// TODO(jzern): Coverage could be added for kInterRoundBitsCompoundVertical via
// WarpCompoundTest.
// using WarpCompoundTest10bpp = WarpTest</*is_compound=*/true, 10, uint16_t>;

TEST_P(WarpTest10bpp, FixedValues) { TestFixedValues(); }

TEST_P(WarpTest10bpp, RandomValues) { TestRandomValues(); }

TEST_P(WarpTest10bpp, DISABLED_Speed) { TestSpeed(); }

INSTANTIATE_TEST_SUITE_P(C, WarpTest10bpp, testing::ValuesIn(warp_test_param));

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, WarpTest10bpp,
                         testing::ValuesIn(warp_test_param));
#endif
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using WarpTest12bpp = WarpTest</*is_compound=*/false, 12, uint16_t>;
// TODO(jzern): Coverage could be added for kInterRoundBitsCompoundVertical via
// WarpCompoundTest.
// using WarpCompoundTest12bpp = WarpTest</*is_compound=*/true, 12, uint16_t>;

TEST_P(WarpTest12bpp, FixedValues) { TestFixedValues(); }

TEST_P(WarpTest12bpp, RandomValues) { TestRandomValues(); }

TEST_P(WarpTest12bpp, DISABLED_Speed) { TestSpeed(); }

INSTANTIATE_TEST_SUITE_P(C, WarpTest12bpp, testing::ValuesIn(warp_test_param));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

std::ostream& operator<<(std::ostream& os, const WarpTestParam& warp_param) {
  return os << "BlockSize" << warp_param.width << "x" << warp_param.height;
}

}  // namespace
}  // namespace dsp
}  // namespace libgav1
