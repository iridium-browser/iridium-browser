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

#include "src/dsp/obmc.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ostream>
#include <string>

#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "gtest/gtest.h"
#include "src/dsp/dsp.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/cpu.h"
#include "tests/block_utils.h"
#include "tests/third_party/libvpx/acm_random.h"
#include "tests/utils.h"

namespace libgav1 {
namespace dsp {
namespace {

#include "src/dsp/obmc.inc"

constexpr int kMaxBlendingBlockSize = 64;
constexpr int kNumSpeedTests = 2e8;

const char* GetDigest8bpp(int id) {
  static const char* const kDigest[] = {
      "c8659acd1e8ecdab06be73f0954fa1ae", "e785f31f2723a193fefd534bd6f6c18f",
      "751fcd8a345fef1c38a25293c9b528c0", "69af412dfa5e96ad43b79c178cb1c58b",
      "2766a64622e183bb4614f2018f14fa85", "8d98589a5cef6e68ee8fadf19d420e3c",
      "19eccf31dd8cf1abcee9414128fe4141", "35019f98e30bcbc6ab624682a0628519",
      "199c551164e73c100045d7ab033ffdcc", "ad5a5eb2906265690c22741b0715f37b",
      "e2152dea159249149ff4151111b73ed6", "1edd570bec7e63780d83588f6aacda25",
      "b24ad192e151b1e0f74d1493004cb1b6", "6c1ce7ed3463cc60870e336f990d4f14",
      "2e6b7a06da21512dfdd9a517d2988655", "971ba1c41ab13bb341c04f936760f546",
      "55b803239d9f12888c666c5320450937", "3d0838963f8c95dafbfb8e5e25c865d2",
      "98a9be6245720d4e0da18115c1a1dbd7", "7e7afe3136ad681b5ea05664fe916548",
      "33971753243f09106173199b7bae1ef5", "65413f33c19a42c112d395121aa4b3b4",
  };
  assert(id >= 0);
  assert(id < sizeof(kDigest) / sizeof(kDigest[0]));
  return kDigest[id];
}

const char* GetDigestSpeed8bpp(int id) {
  static const char* const kDigest[] = {
      "5ea519b616cd2998fbb9b25b4c2660cb", "f23d18197a96de48901738d130a147d9",
      "07b4140c693947a63865f835089766c4", "62547d29bc4dfb2e201e9d907c09e345",
      "c3988da521be50aeb9944564001b282b", "d5a8ff9ca1bd49f4260bb497c489b06c",
      "b3e94f1e33c316759ebf47620327168c", "c5e64a34ca7e55f4daed19cbe4c27049",
      "3b234eb729e8e79db8692c4cbe1b6667", "f9f3060a44c3a575470f9700b3c3a75b",
      "e3a1960b0a7238db1184a3f9d8e9a4b2", "ba9938553703d520bc0ade427c397140",
      "31bf64a6ed1e8002d488c0b9dcffb80a", "9ab1f3ae2e7f70cd27452f30cecfd18e",
      "eaf25ac79ad70fc17ca96d8fcdf0f939", "9aaa88cb5e6b8757e37c3430bd664e70",
      "8293874b2794df8fd22f5a35c3de7bee", "e9d6ee9106227c2c67ea9e6a4652e4ad",
      "29f8a6fc2a650f3945a4ea6d3b975b6d", "8f300a257e913a42666b4921b2b0b5c5",
      "a526265c4b3c8593736a82ddc1fd1603", "76e248f6756ac96343204b0e48d72a9e",
  };
  assert(id >= 0);
  assert(id < sizeof(kDigest) / sizeof(kDigest[0]));
  return kDigest[id];
}

#if LIBGAV1_MAX_BITDEPTH >= 10
const char* GetDigest10bpp(int id) {
  static const char* const kDigest[] = {
      "6f922e4142b644ca3f1eb0f363a1c34e", "84e7c098a9335b36082fec0bc7203075",
      "40f00ea6884fea23a3b7fae59e3b02c3", "70cb92d08b4fdb6dd9c7d418cb1455d3",
      "ed550798b56e70439a93cb48c359e873", "55e0d927b984e78cd51a1961e58a431d",
      "482a6856b87265a82e4ea3fdadb2d95b", "0be46226ff87d74ff2ce68a83eaf9cca",
      "bb4461f0131a1693a0a76f21d92a480b", "ea24f78d74c7864fb247c9a98c9b97b6",
      "d2e70b81882aeb3d9fccef89e7552a9d", "f5d882ee6d9ae6f7dfa467ca99301424",
      "824ddb98eb4129b3d254c0bc7a64cd73", "5eaaafa8ef9b7ba5e2856a947e5b33df",
      "071de1494e0f1b2f99266b90bdc43ddd", "c33227a96dad506adc32dacfb371ab78",
      "e8a632f9fff240c439d4ae6e86795046", "26b90d74f18f9df4427b6180d48db1fc",
      "e4a01e492ddc0398b5c5b60c81468242", "f1b4f7ab5c8b949e51db104f2e33565a",
      "b1fb9ecc6a552e2b23ee92e2f3e4122a", "a683d20129a91bb20b904aa20c0499b1",
  };
  assert(id >= 0);
  assert(id < sizeof(kDigest) / sizeof(kDigest[0]));
  return kDigest[id];
}

const char* GetDigestSpeed10bpp(int id) {
  static const char* const kDigest[] = {
      "80557576299708005111029cef04da53", "24f84f07f53f61cd46bdcfe1e05ff9b5",
      "4dd6bc62145baa5357a4cbf6d7a6ef15", "0b7aa27cee43b8ae0c02d07887eaa225",
      "9e28cdae73ca97433499c31ca79e1d07", "1cacd6466a143f88e736fffaf21e2246",
      "9c7699626660d8965e06a54282a408f3", "eef893efef62b2eb4aaad06fc462819c",
      "4965d0a3ff750813df85c0082b21bd4b", "ec10fd79fbf552abc595def392e9a863",
      "a148bbafdc4466fbb700b31acccca8ac", "5da9d960988549f53b817003b93e4d01",
      "b4c4f88d1fb54869ce7ff452ca7786a6", "d607f785fce62bad85102054539e7089",
      "b441761ea2817e4618c594aaa11d670a", "1cc5e08e6d5f9315dbc0369b97af941d",
      "568cc1a3a67ba4e6e77f54602d0ed3e3", "522f14c068f788bc284a7d1e47d623ed",
      "b543855cbe384b88861c881853c28192", "5faaafc124e94eedc69dc0f5d33dacac",
      "13ca4d01bd20085459e6126555e1f7b5", "46d46fae3c8a7d9e4725154d8d2b76d8",
  };
  assert(id >= 0);
  assert(id < sizeof(kDigest) / sizeof(kDigest[0]));
  return kDigest[id];
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
const char* GetDigest12bpp(int id) {
  static const char* const kDigest[] = {
      "eb18c776d7b56280f01cca40b04a9c44", "058d4a6ed025eac5dcf7aec3203c0882",
      "8355884d7470e9c6af9309ab23bee859", "2ba330551ac58d1d034b947d7ab9b59f",
      "0d25cd773c81e4c57f82513e3b031f01", "b9075f7c3b9a240dbb015a24454eeb71",
      "563ed8683723d1e4f2746280bca3db0a", "d7125306bd8c952d0f85fe1515ca16a7",
      "5bf99c7e4a918c9b6a7e251484ea6527", "38ac9c685e8d2bd2771b6f2b38268301",
      "abc39dbde7470e08b15417ee97c704b2", "37e12753d23b7a8df92b1d32f3170d9f",
      "9a609776cfa31f64826225d0a6b7afdd", "ccdd89e70e94f751fd891b124c1c3210",
      "2bbf7b095e26ed4f27e7d05e20117084", "9a1b403c3a7c00da5686bcb87f1270e8",
      "701d651e391043ab8ebbd0023a430980", "0047f10bdd8321494e8e82597fe2f969",
      "f97e662d139b2811e3d3227de95135a2", "852933b90d4a70f9254157381ed641e0",
      "cfcda707ec8e4361ef741dc716888348", "95e34eab83b3159f61685db248c6a881",
  };
  assert(id >= 0);
  assert(id < sizeof(kDigest) / sizeof(kDigest[0]));
  return kDigest[id];
}

const char* GetDigestSpeed12bpp(int id) {
  static const char* const kDigest[] = {
      "6c0f37c41d72ce40d95545ac0f08d88a", "8a8efeb7d8b2f852d76d0176b6c6878f",
      "5757c88d1cdc0cd29c47c346474161f0", "fef8cf06d16ba7357bfc061e43080cd3",
      "6bd11582448532bce8b91cc8807ab6a0", "1e6dd42eada2d636e210f4e20a771102",
      "377a0472f45fcb42f1712243ea845530", "e3760f2b6e69c1b40e71ecde711d227c",
      "6721638d1a5dadb96ddd0ca067c737ca", "3d3a23210a8496a76991bcec5045808b",
      "2cbd26ecf7d4e927ab569083d3ddb4ca", "7d61af2d7841d1a39a2e930bac166804",
      "dd929506442fb1f2e67130fe8cdf487b", "c0e57f8d2546d5bcb646a24d09d83d7c",
      "2989c6487456c92eb003c8e17e904f45", "5cfb60a3be6ee5c41e0f655a3020f687",
      "28f37d47cb07aa382659ff556a55a4c6", "b6478ab317b11f592deb60d02ce62f2f",
      "bc78e7250c101f82e794d4fa0ee55025", "24304ed23d336a46f205206d3c5d48ef",
      "dc1e71d95d06c1086bb7f9e05e38bf39", "32606ef72985e7de608df2e8760784b7",
  };
  assert(id >= 0);
  assert(id < sizeof(kDigest) / sizeof(kDigest[0]));
  return kDigest[id];
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

struct ObmcTestParam {
  ObmcTestParam(int width, int height, ObmcDirection blending_direction)
      : width(width), height(height), blending_direction(blending_direction) {}
  int width;
  int height;
  ObmcDirection blending_direction;
};

std::ostream& operator<<(std::ostream& os, const ObmcTestParam& param) {
  return os << "BlockSize" << param.width << "x" << param.height
            << ", blending_direction: " << ToString(param.blending_direction);
}

template <int bitdepth, typename Pixel>
class ObmcBlendTest : public testing::TestWithParam<ObmcTestParam> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  ObmcBlendTest() = default;
  ~ObmcBlendTest() override = default;

  void SetUp() override {
    test_utils::ResetDspTable(bitdepth);
    ObmcInit_C();
    const dsp::Dsp* const dsp = dsp::GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const absl::string_view test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      if ((GetCpuInfo() & kSSE4_1) != 0) {
        ObmcInit_SSE4_1();
      }
    } else if (absl::StartsWith(test_case, "NEON/")) {
      ObmcInit_NEON();
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }
    func_ = dsp->obmc_blend[blending_direction_];
  }

 protected:
  int GetDigestId() const {
    // blending_direction_ == kObmcDirectionVertical:
    // (width, height):
    // (4, 2), id = 0. (4, 4), id = 1. (4, 8), id = 2. (8, 4), id = 3.
    // ...
    // blending_direction_ == kObmcDirectionHorizontal: id starts from 11.
    // Vertical skips (2, 4) while horizontal skips (4, 2) creating a gap after
    // (2, 4).
    const int id = (blending_direction_ == kObmcDirectionVertical) ? 0
                   : (width_ == 2)                                 ? 12
                                                                   : 11;
    if (width_ == height_) return id + 3 * (FloorLog2(width_) - 1) - 2;
    if (width_ < height_) return id + 3 * (FloorLog2(width_) - 1) - 1;
    return id + 3 * (FloorLog2(height_) - 1);
  }

  // Note |digest| is only used when |use_fixed_values| is false.
  void Test(const char* digest, bool use_fixed_values, int value);
  void TestSpeed(const char* digest, int num_runs);

 private:
  const int width_ = GetParam().width;
  const int height_ = GetParam().height;
  const ObmcDirection blending_direction_ = GetParam().blending_direction;
  Pixel source1_[kMaxBlendingBlockSize * kMaxBlendingBlockSize] = {};
  Pixel source2_[kMaxBlendingBlockSize * kMaxBlendingBlockSize] = {};
  dsp::ObmcBlendFunc func_;
};

template <int bitdepth, typename Pixel>
void ObmcBlendTest<bitdepth, Pixel>::Test(const char* const digest,
                                          const bool use_fixed_values,
                                          const int value) {
  if (func_ == nullptr) return;
  if (use_fixed_values) {
    std::fill(source1_,
              source1_ + kMaxBlendingBlockSize * kMaxBlendingBlockSize, value);
    std::fill(source2_,
              source2_ + kMaxBlendingBlockSize * kMaxBlendingBlockSize, value);
  } else {
    libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
    Pixel* src_1 = source1_;
    Pixel* src_2 = source2_;
    const int mask = (1 << bitdepth) - 1;
    for (int y = 0; y < height_; ++y) {
      for (int x = 0; x < width_; ++x) {
        src_1[x] = rnd.Rand16() & mask;
        src_2[x] = rnd.Rand16() & mask;
      }
      src_1 += kMaxBlendingBlockSize;
      src_2 += width_;
    }
  }
  const ptrdiff_t stride = kMaxBlendingBlockSize * sizeof(Pixel);
  func_(source1_, stride, width_, height_, source2_,
        width_ * sizeof(source2_[0]));
  if (use_fixed_values) {
    const bool success = test_utils::CompareBlocks(
        source1_, source2_, width_, height_, kMaxBlendingBlockSize,
        kMaxBlendingBlockSize, false);
    EXPECT_TRUE(success);
  } else {
    test_utils::CheckMd5Digest(
        ToString(blending_direction_),
        absl::StrFormat("%dx%d", width_, height_).c_str(), digest, source1_,
        sizeof(source1_), absl::Duration());
  }
}

template <int bitdepth, typename Pixel>
void ObmcBlendTest<bitdepth, Pixel>::TestSpeed(const char* const digest,
                                               const int num_runs) {
  if (func_ == nullptr) return;
  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
  Pixel* src_1 = source1_;
  Pixel* src_2 = source2_;
  const int mask = (1 << bitdepth) - 1;
  for (int y = 0; y < height_; ++y) {
    for (int x = 0; x < width_; ++x) {
      src_1[x] = rnd.Rand16() & mask;
      src_2[x] = rnd.Rand16() & mask;
    }
    src_1 += kMaxBlendingBlockSize;
    src_2 += width_;
  }
  const ptrdiff_t stride = kMaxBlendingBlockSize * sizeof(Pixel);
  uint8_t dest[sizeof(Pixel) * kMaxBlendingBlockSize * kMaxBlendingBlockSize];
  absl::Duration elapsed_time;
  for (int i = 0; i < num_runs; ++i) {
    memcpy(dest, source1_,
           sizeof(Pixel) * kMaxBlendingBlockSize * kMaxBlendingBlockSize);
    const absl::Time start = absl::Now();
    func_(dest, stride, width_, height_, source2_,
          width_ * sizeof(source2_[0]));
    elapsed_time += absl::Now() - start;
  }
  memcpy(source1_, dest,
         sizeof(Pixel) * kMaxBlendingBlockSize * kMaxBlendingBlockSize);
  test_utils::CheckMd5Digest(ToString(blending_direction_),
                             absl::StrFormat("%dx%d", width_, height_).c_str(),
                             digest, source1_, sizeof(source1_), elapsed_time);
}

const ObmcTestParam kObmcTestParam[] = {
    ObmcTestParam(4, 2, kObmcDirectionVertical),
    ObmcTestParam(4, 4, kObmcDirectionVertical),
    ObmcTestParam(4, 8, kObmcDirectionVertical),
    ObmcTestParam(8, 4, kObmcDirectionVertical),
    ObmcTestParam(8, 8, kObmcDirectionVertical),
    ObmcTestParam(8, 16, kObmcDirectionVertical),
    ObmcTestParam(16, 8, kObmcDirectionVertical),
    ObmcTestParam(16, 16, kObmcDirectionVertical),
    ObmcTestParam(16, 32, kObmcDirectionVertical),
    ObmcTestParam(32, 16, kObmcDirectionVertical),
    ObmcTestParam(32, 32, kObmcDirectionVertical),
    ObmcTestParam(2, 4, kObmcDirectionHorizontal),
    ObmcTestParam(4, 4, kObmcDirectionHorizontal),
    ObmcTestParam(4, 8, kObmcDirectionHorizontal),
    ObmcTestParam(8, 4, kObmcDirectionHorizontal),
    ObmcTestParam(8, 8, kObmcDirectionHorizontal),
    ObmcTestParam(8, 16, kObmcDirectionHorizontal),
    ObmcTestParam(16, 8, kObmcDirectionHorizontal),
    ObmcTestParam(16, 16, kObmcDirectionHorizontal),
    ObmcTestParam(16, 32, kObmcDirectionHorizontal),
    ObmcTestParam(32, 16, kObmcDirectionHorizontal),
    ObmcTestParam(32, 32, kObmcDirectionHorizontal),
};

using ObmcBlendTest8bpp = ObmcBlendTest<8, uint8_t>;

TEST_P(ObmcBlendTest8bpp, Blending) {
  Test(/*digest=*/nullptr, /*use_fixed_values=*/true, 0);
  Test(/*digest=*/nullptr, /*use_fixed_values=*/true, 1);
  Test(/*digest=*/nullptr, /*use_fixed_values=*/true, 128);
  Test(/*digest=*/nullptr, /*use_fixed_values=*/true, 255);
  Test(GetDigest8bpp(GetDigestId()), /*use_fixed_values=*/false, -1);
}

TEST_P(ObmcBlendTest8bpp, DISABLED_Speed) {
  TestSpeed(GetDigestSpeed8bpp(GetDigestId()),
            kNumSpeedTests / (GetParam().height * GetParam().width));
}

INSTANTIATE_TEST_SUITE_P(C, ObmcBlendTest8bpp,
                         testing::ValuesIn(kObmcTestParam));

#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, ObmcBlendTest8bpp,
                         testing::ValuesIn(kObmcTestParam));
#endif

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, ObmcBlendTest8bpp,
                         testing::ValuesIn(kObmcTestParam));
#endif

#if LIBGAV1_MAX_BITDEPTH >= 10
using ObmcBlendTest10bpp = ObmcBlendTest<10, uint16_t>;

TEST_P(ObmcBlendTest10bpp, Blending) {
  Test(/*digest=*/nullptr, /*use_fixed_values=*/true, 0);
  Test(/*digest=*/nullptr, /*use_fixed_values=*/true, 1);
  Test(/*digest=*/nullptr, /*use_fixed_values=*/true, 128);
  Test(/*digest=*/nullptr, /*use_fixed_values=*/true, (1 << 10) - 1);
  Test(GetDigest10bpp(GetDigestId()), /*use_fixed_values=*/false, -1);
}

TEST_P(ObmcBlendTest10bpp, DISABLED_Speed) {
  TestSpeed(GetDigestSpeed10bpp(GetDigestId()),
            kNumSpeedTests / (GetParam().height * GetParam().width));
}

INSTANTIATE_TEST_SUITE_P(C, ObmcBlendTest10bpp,
                         testing::ValuesIn(kObmcTestParam));
#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, ObmcBlendTest10bpp,
                         testing::ValuesIn(kObmcTestParam));
#endif
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, ObmcBlendTest10bpp,
                         testing::ValuesIn(kObmcTestParam));
#endif
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using ObmcBlendTest12bpp = ObmcBlendTest<12, uint16_t>;

TEST_P(ObmcBlendTest12bpp, Blending) {
  Test(/*digest=*/nullptr, /*use_fixed_values=*/true, 0);
  Test(/*digest=*/nullptr, /*use_fixed_values=*/true, 1);
  Test(/*digest=*/nullptr, /*use_fixed_values=*/true, 128);
  Test(/*digest=*/nullptr, /*use_fixed_values=*/true, (1 << 12) - 1);
  Test(GetDigest12bpp(GetDigestId()), /*use_fixed_values=*/false, -1);
}

TEST_P(ObmcBlendTest12bpp, DISABLED_Speed) {
  TestSpeed(GetDigestSpeed12bpp(GetDigestId()),
            kNumSpeedTests / (GetParam().height * GetParam().width));
}

INSTANTIATE_TEST_SUITE_P(C, ObmcBlendTest12bpp,
                         testing::ValuesIn(kObmcTestParam));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace
}  // namespace dsp
}  // namespace libgav1
