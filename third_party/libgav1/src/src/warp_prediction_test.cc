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

#include "src/warp_prediction.h"

#include <cstddef>
#include <cstdint>
#include <ostream>

#include "absl/base/macros.h"
#include "gtest/gtest.h"
#include "src/obu_parser.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/types.h"
#include "tests/third_party/libvpx/acm_random.h"

namespace libgav1 {
namespace {

constexpr int16_t kExpectedWarpParamsOutput[10][4] = {
    {0, 0, 0, 0},
    {2880, 2880, 2752, 2752},
    {-1408, -1408, -1472, -1472},
    {0, 0, 0, 0},
    {6784, 6784, 6144, 6144},  // Invalid.
    {-5312, -5312, -5824, -5824},
    {-3904, -3904, -4160, -4160},
    {2496, 2496, 2368, 2368},
    {1024, 1024, 1024, 1024},
    {-7808, -7808, -8832, -8832},  // Invalid.
};

constexpr bool kExpectedWarpValid[10] = {
    true, true, true, true, false, true, true, true, true, false,
};

int RandomWarpedParam(int seed_offset, int bits) {
  libvpx_test::ACMRandom rnd(seed_offset +
                             libvpx_test::ACMRandom::DeterministicSeed());
  // 1 in 8 chance of generating zero (arbitrary).
  const bool zero = (rnd.Rand16() & 7) == 0;
  if (zero) return 0;
  // Generate uniform values in the range [-(1 << bits), 1] U [1, 1 << bits].
  const int mask = (1 << bits) - 1;
  const int value = 1 + (rnd.RandRange(1U << 31) & mask);
  const bool sign = (rnd.Rand16() & 1) != 0;
  return sign ? value : -value;
}

void GenerateWarpedModel(GlobalMotion* warp_params, int seed) {
  do {
    warp_params->params[0] =
        RandomWarpedParam(seed, kWarpedModelPrecisionBits + 6);
    warp_params->params[1] =
        RandomWarpedParam(seed, kWarpedModelPrecisionBits + 6);
    warp_params->params[2] =
        RandomWarpedParam(seed, kWarpedModelPrecisionBits - 3) +
        (1 << kWarpedModelPrecisionBits);
    warp_params->params[3] =
        RandomWarpedParam(seed, kWarpedModelPrecisionBits - 3);
    warp_params->params[4] =
        RandomWarpedParam(seed, kWarpedModelPrecisionBits - 3);
    warp_params->params[5] =
        RandomWarpedParam(seed, kWarpedModelPrecisionBits - 3) +
        (1 << kWarpedModelPrecisionBits);
  } while (warp_params->params[2] == 0);
}

TEST(WarpPredictionTest, SetupShear) {
  for (size_t i = 0; i < ABSL_ARRAYSIZE(kExpectedWarpParamsOutput); ++i) {
    GlobalMotion warp_params;
    GenerateWarpedModel(&warp_params, static_cast<int>(i));
    const bool warp_valid = SetupShear(&warp_params);

    SCOPED_TRACE(testing::Message() << "Test failure at iteration: " << i);
    EXPECT_EQ(warp_valid, kExpectedWarpValid[i]);
    EXPECT_EQ(warp_params.alpha, kExpectedWarpParamsOutput[i][0]);
    EXPECT_EQ(warp_params.beta, kExpectedWarpParamsOutput[i][1]);
    EXPECT_EQ(warp_params.gamma, kExpectedWarpParamsOutput[i][2]);
    EXPECT_EQ(warp_params.delta, kExpectedWarpParamsOutput[i][3]);
  }

  // Test signed shift behavior in delta and gamma generation.
  GlobalMotion warp_params;
  warp_params.params[0] = 24748;
  warp_params.params[1] = -142530;
  warp_params.params[2] = 65516;
  warp_params.params[3] = -640;
  warp_params.params[4] = 256;
  warp_params.params[5] = 65310;
  EXPECT_TRUE(SetupShear(&warp_params));
  EXPECT_EQ(warp_params.alpha, 0);
  EXPECT_EQ(warp_params.beta, -640);
  EXPECT_EQ(warp_params.gamma, 256);
  EXPECT_EQ(warp_params.delta, -192);

  warp_params.params[0] = 24748;
  warp_params.params[1] = -142530;
  warp_params.params[2] = 61760;
  warp_params.params[3] = -640;
  warp_params.params[4] = -13312;
  warp_params.params[5] = 65310;
  EXPECT_TRUE(SetupShear(&warp_params));
  EXPECT_EQ(warp_params.alpha, -3776);
  EXPECT_EQ(warp_params.beta, -640);
  EXPECT_EQ(warp_params.gamma, -14144);
  EXPECT_EQ(warp_params.delta, -384);
}

struct WarpInputParam {
  WarpInputParam(int num_samples, int block_width4x4, int block_height4x4)
      : num_samples(num_samples),
        block_width4x4(block_width4x4),
        block_height4x4(block_height4x4) {}
  int num_samples;
  int block_width4x4;
  int block_height4x4;
};

std::ostream& operator<<(std::ostream& os, const WarpInputParam& param) {
  return os << "num_samples: " << param.num_samples
            << ", block_(width/height)4x4: " << param.block_width4x4 << "x"
            << param.block_height4x4;
}

const WarpInputParam warp_test_param[] = {
    // sample = 1.
    WarpInputParam(1, 1, 1),
    WarpInputParam(1, 1, 2),
    WarpInputParam(1, 2, 1),
    WarpInputParam(1, 2, 2),
    WarpInputParam(1, 2, 4),
    WarpInputParam(1, 4, 2),
    WarpInputParam(1, 4, 4),
    WarpInputParam(1, 4, 8),
    WarpInputParam(1, 8, 4),
    WarpInputParam(1, 8, 8),
    WarpInputParam(1, 8, 16),
    WarpInputParam(1, 16, 8),
    WarpInputParam(1, 16, 16),
    WarpInputParam(1, 16, 32),
    WarpInputParam(1, 32, 16),
    WarpInputParam(1, 32, 32),
    // sample = 8.
    WarpInputParam(8, 1, 1),
    WarpInputParam(8, 1, 2),
    WarpInputParam(8, 2, 1),
    WarpInputParam(8, 2, 2),
    WarpInputParam(8, 2, 4),
    WarpInputParam(8, 4, 2),
    WarpInputParam(8, 4, 4),
    WarpInputParam(8, 4, 8),
    WarpInputParam(8, 8, 4),
    WarpInputParam(8, 8, 8),
    WarpInputParam(8, 8, 16),
    WarpInputParam(8, 16, 8),
    WarpInputParam(8, 16, 16),
    WarpInputParam(8, 16, 32),
    WarpInputParam(8, 32, 16),
    WarpInputParam(8, 32, 32),
};

constexpr bool kExpectedWarpEstimationValid[2] = {false, true};

constexpr int kExpectedWarpEstimationOutput[16][6] = {
    {8388607, 8388607, 57345, -8191, -8191, 57345},
    {8388607, 8388607, 57345, -8191, -8191, 57345},
    {8388607, 8388607, 57345, -8191, -8191, 57345},
    {8388607, 8388607, 57345, -8191, -8191, 57345},
    {8388607, 8388607, 57345, -8191, -8191, 57345},
    {8388607, 8388607, 57345, -8191, -8191, 57345},
    {8388607, 8388607, 57345, -8191, -8191, 57345},
    {8388607, 8388607, 57345, -8191, -8191, 57345},
    {8388607, 8388607, 57345, -8191, -8191, 57345},
    {8388607, 8388607, 57345, -8191, -8191, 57345},
    {2146296, 1589240, 57345, 8191, -8191, 73727},
    {1753128, 1196072, 73727, -8191, 8191, 57345},
    {-8388608, -8388608, 73727, 8191, 8191, 73727},
    {-4435485, -8388608, 65260, 8191, 8191, 73727},
    {-8388608, -7552929, 73727, 8191, 8191, 68240},
    {-8388608, -8388608, 73727, 8191, 8191, 70800},
};

class WarpEstimationTest : public testing::TestWithParam<WarpInputParam> {
 public:
  WarpEstimationTest() = default;
  ~WarpEstimationTest() override = default;

 protected:
  WarpInputParam param_ = GetParam();
};

TEST_P(WarpEstimationTest, WarpEstimation) {
  // Set input params.
  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
  const int row4x4 = rnd.Rand8();
  const int column4x4 = rnd.Rand8();
  MotionVector mv;
  mv.mv[0] = rnd.Rand8();
  mv.mv[1] = rnd.Rand8();
  int candidates[kMaxLeastSquaresSamples][4];
  for (int i = 0; i < param_.num_samples; ++i) {
    // Make candidates relative to the top left of frame.
    candidates[i][0] = rnd.Rand8() + MultiplyBy32(row4x4);
    candidates[i][1] = rnd.Rand8() + MultiplyBy32(column4x4);
    candidates[i][2] = rnd.Rand8() + MultiplyBy32(row4x4);
    candidates[i][3] = rnd.Rand8() + MultiplyBy32(column4x4);
  }

  // Get output.
  GlobalMotion warp_params;
  const bool warp_success = WarpEstimation(
      param_.num_samples, param_.block_width4x4, param_.block_height4x4, row4x4,
      column4x4, mv, candidates, &warp_params);
  if (param_.num_samples == 1) {
    EXPECT_EQ(warp_success, kExpectedWarpEstimationValid[0]);
  } else {
    EXPECT_EQ(warp_success, kExpectedWarpEstimationValid[1]);
    int index = FloorLog2(param_.block_width4x4) * 3 - 1;
    if (param_.block_width4x4 == param_.block_height4x4) {
      index += 1;
    } else if (param_.block_width4x4 < param_.block_height4x4) {
      index += 2;
    }
    for (size_t i = 0; i < ABSL_ARRAYSIZE(warp_params.params); ++i) {
      EXPECT_EQ(warp_params.params[i], kExpectedWarpEstimationOutput[index][i]);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(WarpFuncTest, WarpEstimationTest,
                         testing::ValuesIn(warp_test_param));
}  // namespace
}  // namespace libgav1
