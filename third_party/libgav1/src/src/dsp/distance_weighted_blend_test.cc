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

#include "src/dsp/distance_weighted_blend.h"

#include <cassert>
#include <cstdint>
#include <ostream>
#include <string>
#include <type_traits>

#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "gtest/gtest.h"
#include "src/dsp/constants.h"
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

constexpr int kNumSpeedTests = 500000;

constexpr int kQuantizedDistanceLookup[4][2] = {
    {9, 7}, {11, 5}, {12, 4}, {13, 3}};

template <int bitdepth, typename Pixel>
class DistanceWeightedBlendTest : public testing::TestWithParam<BlockSize>,
                                  public test_utils::MaxAlignedAllocable {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  DistanceWeightedBlendTest() = default;
  ~DistanceWeightedBlendTest() override = default;

  void SetUp() override {
    test_utils::ResetDspTable(bitdepth);
    DistanceWeightedBlendInit_C();
    const dsp::Dsp* const dsp = dsp::GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);
    base_func_ = dsp->distance_weighted_blend;
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const absl::string_view test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
      base_func_ = nullptr;
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      if ((GetCpuInfo() & kSSE4_1) != 0) {
        DistanceWeightedBlendInit_SSE4_1();
      }
    } else if (absl::StartsWith(test_case, "NEON/")) {
      DistanceWeightedBlendInit_NEON();
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }
    func_ = dsp->distance_weighted_blend;
  }

 protected:
  void Test(const char* digest, int num_tests);

 private:
  using PredType =
      typename std::conditional<bitdepth == 8, int16_t, uint16_t>::type;
  static constexpr int kDestStride = kMaxSuperBlockSizeInPixels;
  const int width_ = kBlockWidthPixels[GetParam()];
  const int height_ = kBlockHeightPixels[GetParam()];
  alignas(kMaxAlignment) PredType
      source1_[kMaxSuperBlockSizeInPixels * kMaxSuperBlockSizeInPixels];
  alignas(kMaxAlignment) PredType
      source2_[kMaxSuperBlockSizeInPixels * kMaxSuperBlockSizeInPixels];
  Pixel dest_[kMaxSuperBlockSizeInPixels * kMaxSuperBlockSizeInPixels] = {};
  Pixel reference_[kMaxSuperBlockSizeInPixels * kMaxSuperBlockSizeInPixels] =
      {};
  dsp::DistanceWeightedBlendFunc base_func_;
  dsp::DistanceWeightedBlendFunc func_;
};

template <int bitdepth, typename Pixel>
void DistanceWeightedBlendTest<bitdepth, Pixel>::Test(const char* digest,
                                                      int num_tests) {
  if (func_ == nullptr) return;
  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
  PredType* src_1 = source1_;
  PredType* src_2 = source2_;

  const int index = rnd.Rand8() & 3;
  const uint8_t weight_0 = kQuantizedDistanceLookup[index][0];
  const uint8_t weight_1 = kQuantizedDistanceLookup[index][1];
  // In libgav1, predictors have an offset which are later subtracted and
  // clipped in distance weighted blending. Therefore we add the offset
  // here to match libaom's implementation.
  for (int y = 0; y < height_; ++y) {
    for (int x = 0; x < width_; ++x) {
      // distance_weighted_blend is applied to compound prediction values. This
      // implies a range far exceeding that of pixel values.
      // The ranges include kCompoundOffset in 10bpp and 12bpp.
      // see: src/dsp/convolve.cc & src/dsp/warp.cc.
      static constexpr int kCompoundPredictionRange[3][2] = {
          // 8bpp
          {-5132, 9212},
          // 10bpp
          {3988, 61532},
          // 12bpp
          {3974, 61559},
      };
      constexpr int bitdepth_index = (bitdepth - 8) >> 1;
      const int min_val = kCompoundPredictionRange[bitdepth_index][0];
      const int max_val = kCompoundPredictionRange[bitdepth_index][1];
      src_1[x] = static_cast<PredType>(rnd(max_val - min_val) + min_val);
      src_2[x] = static_cast<PredType>(rnd(max_val - min_val) + min_val);
    }
    src_1 += width_;
    src_2 += width_;
  }
  absl::Duration elapsed_time;
  for (int i = 0; i < num_tests; ++i) {
    const absl::Time start = absl::Now();
    func_(source1_, source2_, weight_0, weight_1, width_, height_, dest_,
          sizeof(Pixel) * kDestStride);
    elapsed_time += absl::Now() - start;
  }

  test_utils::CheckMd5Digest("DistanceWeightedBlend", ToString(GetParam()),
                             digest, dest_, sizeof(dest_), elapsed_time);
}

const BlockSize kTestParam[] = {
    kBlock4x4,    kBlock4x8,     kBlock4x16,  kBlock8x4,   kBlock8x8,
    kBlock8x16,   kBlock8x32,    kBlock16x4,  kBlock16x8,  kBlock16x16,
    kBlock16x32,  kBlock16x64,   kBlock32x8,  kBlock32x16, kBlock32x32,
    kBlock32x64,  kBlock64x16,   kBlock64x32, kBlock64x64, kBlock64x128,
    kBlock128x64, kBlock128x128,
};

const char* GetDistanceWeightedBlendDigest8bpp(const BlockSize block_size) {
  static const char* const kDigests[kMaxBlockSizes] = {
      // 4xN
      "ebf389f724f8ab46a2cac895e4e073ca",
      "09acd567b6b12c8cf8eb51d8b86eb4bf",
      "57bb4d65695d8ec6752f2bd8686b64fd",
      // 8xN
      "270905ac76f9a2cba8a552eb0bf7c8c1",
      "f0801c8574d2c271ef2bbea77a1d7352",
      "e761b580e3312be33a227492a233ce72",
      "ff214dab1a7e98e2285961d6421720c6",
      // 16xN
      "4f712609a36e817f9752326d58562ff8",
      "14243f5c5f7c7104160c1f2cef0a0fbc",
      "3ac3f3161b7c8dd8436b02abfdde104a",
      "81a00b704e0e41a5dbe6436ac70c098d",
      "af8fd02017c7acdff788be742d700baa",
      // 32xN
      "ee34332c66a6d6ed8ce64031aafe776c",
      "b5e3d22bd2dbdb624c8b86a1afb5ce6d",
      "607ffc22098d81b7e37a7bf62f4af5d3",
      "3823dbf043b4682f56d5ca698e755ea5",
      // 64xN
      "4acf556b921956c2bc24659cd5128401",
      "a298c544c9c3b27924b4c23cc687ea5a",
      "539e2df267782ce61c70103b23b7d922",
      "3b0cb2a0b5d384efee4d81401025bec1",
      // 128xN
      "8b56b636dd712c2f8d138badb7219991",
      "8cfc8836908902b8f915639b7bff45b3",
  };
  assert(block_size < kMaxBlockSizes);
  return kDigests[block_size];
}

using DistanceWeightedBlendTest8bpp = DistanceWeightedBlendTest<8, uint8_t>;

TEST_P(DistanceWeightedBlendTest8bpp, Blending) {
  Test(GetDistanceWeightedBlendDigest8bpp(GetParam()), 1);
}

TEST_P(DistanceWeightedBlendTest8bpp, DISABLED_Speed) {
  Test(GetDistanceWeightedBlendDigest8bpp(GetParam()), kNumSpeedTests);
}

INSTANTIATE_TEST_SUITE_P(C, DistanceWeightedBlendTest8bpp,
                         testing::ValuesIn(kTestParam));

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, DistanceWeightedBlendTest8bpp,
                         testing::ValuesIn(kTestParam));
#endif

#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, DistanceWeightedBlendTest8bpp,
                         testing::ValuesIn(kTestParam));
#endif

#if LIBGAV1_MAX_BITDEPTH >= 10
const char* GetDistanceWeightedBlendDigest10bpp(const BlockSize block_size) {
  static const char* const kDigests[] = {
      // 4xN
      "55f594b56e16d5c401274affebbcc3d3",
      "69df14da4bb33a8f7d7087921008e919",
      "1b61f33604c54015794198a13bfebf46",
      // 8xN
      "825a938185b152f7cf09bf1c0723ce2b",
      "85ea315c51d979bc9b45834d6b40ec6f",
      "92ebde208e8c39f7ec6de2de82182dbb",
      "520f84716db5b43684dbb703806383fe",
      // 16xN
      "12ca23e3e2930005a0511646e8c83da4",
      "6208694a6744f4a3906f58c1add670e3",
      "a33d63889df989a3bbf84ff236614267",
      "34830846ecb0572a98bbd192fed02b16",
      "34bb2f79c0bd7f9a80691b8af597f2a8",
      // 32xN
      "fa97f2d0e3143f1f44d3ac018b0d696d",
      "3df4a22456c9ab6ed346ab1b9750ae7d",
      "6276a058b35c6131bc0c94a4b4a37ebc",
      "9ca42da5d2d5eb339df03ae2c7a26914",
      // 64xN
      "800e692c520f99223bc24c1ac95a0166",
      "818b6d20426585ef7fe844015a03aaf5",
      "fb48691ccfff083e01d74826e88e613f",
      "0bd350bc5bc604a224d77a5f5a422698",
      // 128xN
      "a130840813cd6bd69d09bcf5f8d0180f",
      "6ece1846bea55e8f8f2ed7fbf73718de",
  };
  assert(block_size < kMaxBlockSizes);
  return kDigests[block_size];
}

using DistanceWeightedBlendTest10bpp = DistanceWeightedBlendTest<10, uint16_t>;

TEST_P(DistanceWeightedBlendTest10bpp, Blending) {
  Test(GetDistanceWeightedBlendDigest10bpp(GetParam()), 1);
}

TEST_P(DistanceWeightedBlendTest10bpp, DISABLED_Speed) {
  Test(GetDistanceWeightedBlendDigest10bpp(GetParam()), kNumSpeedTests);
}

INSTANTIATE_TEST_SUITE_P(C, DistanceWeightedBlendTest10bpp,
                         testing::ValuesIn(kTestParam));

#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, DistanceWeightedBlendTest10bpp,
                         testing::ValuesIn(kTestParam));
#endif
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, DistanceWeightedBlendTest10bpp,
                         testing::ValuesIn(kTestParam));
#endif
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
const char* GetDistanceWeightedBlendDigest12bpp(const BlockSize block_size) {
  static const char* const kDigests[] = {
      // 4xN
      "e30bf8f5f294206ad1dd79bd10a20827",
      "f0cfb60134562d9c5f2ec6ad106e01ef",
      "ad0876244e1b769203266a9c75b74afc",
      // 8xN
      "5265b954479c15a80f427561c5f36ff4",
      "7f157457d1671e4ecce7a0884e9e3f76",
      "d2cef5cf217f2d1f787c8951b7fe7cb2",
      "6d23059008adbbb84ac941c8b4968f5b",
      // 16xN
      "ae521a5656ed3440d1fa950c20d90a79",
      "935bec0e12b5dd3e0c34b3de8ba51476",
      "0334bafcdcd7ddddb673ded492bca25a",
      "c5360f08d0be77c79dc19fb55a0c5fe0",
      "c2d1e7a4244a8aaaac041aed0cefc148",
      // 32xN
      "ce7f3cf78ae4f836cf69763137f7f6a6",
      "800e52ebb14d5831c047d391cd760f95",
      "74aa2b412b42165f1967daf3042b4f17",
      "140d4cc600944b629b1991e88a9fe97c",
      // 64xN
      "3d206f93229ee2cea5c5da4e0ac6445a",
      "3d13028f8fffe79fd35752c0177291ca",
      "e7a7669acb5979dc7b15a19eed09cd4c",
      "599368f4971c203fc5fa32989fe8cb44",
      // 128xN
      "54b46af2e2c8d2081e26fa0315b4ffd7",
      "602e769bb2104e78223e68e50e7e86a0",
  };
  assert(block_size < kMaxBlockSizes);
  return kDigests[block_size];
}

using DistanceWeightedBlendTest12bpp = DistanceWeightedBlendTest<12, uint16_t>;

TEST_P(DistanceWeightedBlendTest12bpp, Blending) {
  Test(GetDistanceWeightedBlendDigest12bpp(GetParam()), 1);
}

TEST_P(DistanceWeightedBlendTest12bpp, DISABLED_Speed) {
  Test(GetDistanceWeightedBlendDigest12bpp(GetParam()), kNumSpeedTests);
}

INSTANTIATE_TEST_SUITE_P(C, DistanceWeightedBlendTest12bpp,
                         testing::ValuesIn(kTestParam));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace
}  // namespace dsp

static std::ostream& operator<<(std::ostream& os, const BlockSize param) {
  return os << ToString(param);
}

}  // namespace libgav1
