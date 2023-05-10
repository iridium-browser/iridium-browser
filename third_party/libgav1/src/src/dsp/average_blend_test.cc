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

#include "src/dsp/average_blend.h"

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
#include "src/dsp/distance_weighted_blend.h"
#include "src/dsp/dsp.h"
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

constexpr int kNumSpeedTests = 5e8;
constexpr char kAverageBlend[] = "AverageBlend";
// average_blend is applied to compound prediction values. This implies a range
// far exceeding that of pixel values.
// The ranges include kCompoundOffset in 10bpp and 12bpp.
// see: src/dsp/convolve.cc & src/dsp/warp.cc.
constexpr int kCompoundPredictionRange[3][2] = {
    // 8bpp
    {-5132, 9212},
    // 10bpp
    {3988, 61532},
    // 12bpp
    {3974, 61559},
};

template <int bitdepth, typename Pixel>
class AverageBlendTest : public testing::TestWithParam<BlockSize>,
                         public test_utils::MaxAlignedAllocable {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  AverageBlendTest() = default;
  ~AverageBlendTest() override = default;

  void SetUp() override {
    test_utils::ResetDspTable(bitdepth);
    AverageBlendInit_C();
    DistanceWeightedBlendInit_C();
    const dsp::Dsp* const dsp = dsp::GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);
    base_func_ = dsp->average_blend;
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const absl::string_view test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
      base_func_ = nullptr;
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      if ((GetCpuInfo() & kSSE4_1) != 0) {
        AverageBlendInit_SSE4_1();
      }
    } else if (absl::StartsWith(test_case, "NEON/")) {
      AverageBlendInit_NEON();
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }
    func_ = dsp->average_blend;
    dist_blend_func_ = dsp->distance_weighted_blend;
  }

 protected:
  void Test(const char* digest, int num_tests, bool debug);

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
  dsp::AverageBlendFunc base_func_;
  dsp::AverageBlendFunc func_;
  dsp::DistanceWeightedBlendFunc dist_blend_func_;
};

template <int bitdepth, typename Pixel>
void AverageBlendTest<bitdepth, Pixel>::Test(const char* digest, int num_tests,
                                             bool debug) {
  if (func_ == nullptr) return;
  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
  PredType* src_1 = source1_;
  PredType* src_2 = source2_;
  for (int y = 0; y < height_; ++y) {
    for (int x = 0; x < width_; ++x) {
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
    func_(source1_, source2_, width_, height_, dest_,
          sizeof(dest_[0]) * kDestStride);
    elapsed_time += absl::Now() - start;
  }
  if (debug) {
    if (base_func_ != nullptr) {
      base_func_(source1_, source2_, width_, height_, reference_,
                 sizeof(reference_[0]) * kDestStride);
    } else {
      // Use dist_blend_func_ as the base for C tests.
      const int8_t weight = 8;
      dist_blend_func_(source1_, source2_, weight, weight, width_, height_,
                       reference_, sizeof(reference_[0]) * kDestStride);
    }
    EXPECT_TRUE(test_utils::CompareBlocks(dest_, reference_, width_, height_,
                                          kDestStride, kDestStride, false));
  }

  test_utils::CheckMd5Digest(kAverageBlend, ToString(GetParam()), digest, dest_,
                             sizeof(dest_[0]) * kDestStride * height_,
                             elapsed_time);
}

const BlockSize kTestParam[] = {
    kBlock4x4,    kBlock4x8,     kBlock4x16,  kBlock8x4,   kBlock8x8,
    kBlock8x16,   kBlock8x32,    kBlock16x4,  kBlock16x8,  kBlock16x16,
    kBlock16x32,  kBlock16x64,   kBlock32x8,  kBlock32x16, kBlock32x32,
    kBlock32x64,  kBlock64x16,   kBlock64x32, kBlock64x64, kBlock64x128,
    kBlock128x64, kBlock128x128,
};

using AverageBlendTest8bpp = AverageBlendTest<8, uint8_t>;

const char* GetAverageBlendDigest8bpp(const BlockSize block_size) {
  static const char* const kDigests[kMaxBlockSizes] = {
      // 4xN
      "152bcc35946900b1ed16369b3e7a81b7",
      "c23e9b5698f7384eaae30a3908118b77",
      "f2da31d940f62490c368c03d32d3ede8",
      // 8xN
      "73c95485ef956e1d9ab914e88e6a202b",
      "d90d3abd368e58c513070a88b34649ba",
      "77f7d53d0edeffb3537afffd9ff33a4a",
      "460b9b1e6b83f65f013cfcaf67ec0122",
      // 16xN
      "96454a56de940174ff92e9bb686d6d38",
      "a50e268e93b48ae39cc2a47d377410e2",
      "65c8502ff6d78065d466f9911ed6bb3e",
      "bc2c873b9f5d74b396e1df705e87f699",
      "b4dae656484b2d255d1e09b7f34e12c1",
      // 32xN
      "7e1e5db92b22a96e5226a23de883d766",
      "ca40d46d89773e7f858b15fcecd43cc0",
      "bfdc894707323f4dc43d1326309f8368",
      "f4733417621719b7feba3166ec0da5b9",
      // 64xN
      "378fa0594d22f01c8e8931c2a908d7c4",
      "db38fe2e082bd4a09acb3bb1d52ee11e",
      "3ad44401cc731215c46c9b7d96f7e4ae",
      "6c43267be5ed03d204a05fe36090f870",
      // 128xN
      "c8cfe46ebf166c1cbf08e8804206aadb",
      "b0557b5156d2334c8ce4a7ee12f9d6b4",
  };
  assert(block_size < kMaxBlockSizes);
  return kDigests[block_size];
}

TEST_P(AverageBlendTest8bpp, Blending) {
  Test(GetAverageBlendDigest8bpp(GetParam()), 1, false);
}

TEST_P(AverageBlendTest8bpp, DISABLED_Speed) {
  Test(GetAverageBlendDigest8bpp(GetParam()),
       kNumSpeedTests /
           (kBlockHeightPixels[GetParam()] * kBlockWidthPixels[GetParam()]),
       false);
}

INSTANTIATE_TEST_SUITE_P(C, AverageBlendTest8bpp,
                         testing::ValuesIn(kTestParam));
#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, AverageBlendTest8bpp,
                         testing::ValuesIn(kTestParam));
#endif
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, AverageBlendTest8bpp,
                         testing::ValuesIn(kTestParam));
#endif

#if LIBGAV1_MAX_BITDEPTH >= 10
using AverageBlendTest10bpp = AverageBlendTest<10, uint16_t>;

const char* GetAverageBlendDigest10bpp(const BlockSize block_size) {
  static const char* const kDigests[kMaxBlockSizes] = {
      // 4xN
      "98c0671c092b4288adcaaa17362cc4a3",
      "7083f3def8bfb63ab3a985ef5616a923",
      "a7211ee2eaa6f88e08875b377d17b0f1",
      // 8xN
      "11f9ab881700f2ef0f82d8d4662868c6",
      "3bee144b9ea6f4288b860c24f88a22f3",
      "27113bd17bf95034f100e9046c7b59d2",
      "c42886a5e16e23a81e43833d34467558",
      // 16xN
      "b0ac2eb0a7a6596d6d1339074c7f8771",
      "24c9e079b9a8647a6ee03f5441f2cdd9",
      "dd05777751ccdb4356856c90e1176e53",
      "27b1d69d035b1525c013b7373cfe3875",
      "08c46403afe19e6b008ccc8f56633da9",
      // 32xN
      "36d434db11298aba76166df06e9b8125",
      "efd24dd7b555786bff1a482e51170ea3",
      "3b37ddac87de443cd18784f02c2d1dd5",
      "80d8070939a743a20689a65bf5dc0a68",
      // 64xN
      "88e747246237c6408d0bd4cc3ecc8396",
      "af1fe8c52487c9f2951c3ea516828abb",
      "ea6f18ff56b053748c18032b7e048e83",
      "af0cb87fe27d24c2e0afd2c90a8533a6",
      // 128xN
      "16a83b19911d6dc7278a694b8baa9901",
      "bd22e77ce6fa727267ff63eeb4dcb19c",
  };
  assert(block_size < kMaxBlockSizes);
  return kDigests[block_size];
}

TEST_P(AverageBlendTest10bpp, Blending) {
  Test(GetAverageBlendDigest10bpp(GetParam()), 1, false);
}

TEST_P(AverageBlendTest10bpp, DISABLED_Speed) {
  Test(GetAverageBlendDigest10bpp(GetParam()),
       kNumSpeedTests /
           (kBlockHeightPixels[GetParam()] * kBlockHeightPixels[GetParam()]) /
           2,
       false);
}

INSTANTIATE_TEST_SUITE_P(C, AverageBlendTest10bpp,
                         testing::ValuesIn(kTestParam));
#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, AverageBlendTest10bpp,
                         testing::ValuesIn(kTestParam));
#endif
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, AverageBlendTest10bpp,
                         testing::ValuesIn(kTestParam));
#endif
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using AverageBlendTest12bpp = AverageBlendTest<12, uint16_t>;

const char* GetAverageBlendDigest12bpp(const BlockSize block_size) {
  static const char* const kDigests[kMaxBlockSizes] = {
      // 4xN
      "8f5ad8fba61a0f1cb6b77f5460c241be",
      "3a9d017848fdb4162315c689b4449ac6",
      "bb97029fff021b168b98b209dcee5123",
      // 8xN
      "a7ff1b199965b8856499ae3f1b2c48eb",
      "05220c72835fc4662d261183df0a57cf",
      "97de8c325f1475c44e1afc44183e55ad",
      "60d820c46cad14d9d934da238bb79707",
      // 16xN
      "f3e4863121819bc28f7c1f453898650c",
      "5f5f68d21269d7df546c848921e8f2cd",
      "17efe0b0fce1f8d4c7bc6eacf769063e",
      "3da591e201f44511cdd6c465692ace1e",
      "5a0ca6c88664d2e918a032b5fcf66070",
      // 32xN
      "efe236bee8a9fef90b99d8012006f985",
      "d6ff3aacbbbadff6d0ccb0873fb9fa2a",
      "38801f7361052873423d57b574aabddc",
      "55c76772ecdc1721e92ca04d2fc7c089",
      // 64xN
      "4261ecdde34eedc4e5066a93e0f64881",
      "fe82e012efab872672193316d670fd82",
      "6c698bc2d4acf4444a64ac55ae9641de",
      "98626e25101cff69019d1b7e6e439404",
      // 128xN
      "fe0f3c89dd39786df1c952a2470d680d",
      "af7e166fc3d8c9ce85789acf3467ed9d",
  };
  assert(block_size < kMaxBlockSizes);
  return kDigests[block_size];
}

TEST_P(AverageBlendTest12bpp, Blending) {
  Test(GetAverageBlendDigest12bpp(GetParam()), 1, false);
}

TEST_P(AverageBlendTest12bpp, DISABLED_Speed) {
  Test(GetAverageBlendDigest12bpp(GetParam()),
       kNumSpeedTests /
           (kBlockHeightPixels[GetParam()] * kBlockHeightPixels[GetParam()]) /
           2,
       false);
}

INSTANTIATE_TEST_SUITE_P(C, AverageBlendTest12bpp,
                         testing::ValuesIn(kTestParam));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace
}  // namespace dsp

static std::ostream& operator<<(std::ostream& os, const BlockSize param) {
  return os << ToString(param);
}

}  // namespace libgav1
