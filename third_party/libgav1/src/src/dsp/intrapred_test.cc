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

#include "src/dsp/intrapred.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <ostream>

#include "absl/strings/match.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "gtest/gtest.h"
#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/dsp/intrapred_smooth.h"
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

constexpr int kMaxBlockSize = 64;
constexpr int kTotalPixels = kMaxBlockSize * kMaxBlockSize;

template <int bitdepth, typename Pixel>
class IntraPredTestBase : public testing::TestWithParam<TransformSize>,
                          public test_utils::MaxAlignedAllocable {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  IntraPredTestBase() {
    switch (tx_size_) {
      case kNumTransformSizes:
        EXPECT_NE(tx_size_, kNumTransformSizes);
        break;
      default:
        block_width_ = kTransformWidth[tx_size_];
        block_height_ = kTransformHeight[tx_size_];
        break;
    }
  }

  IntraPredTestBase(const IntraPredTestBase&) = delete;
  IntraPredTestBase& operator=(const IntraPredTestBase&) = delete;
  ~IntraPredTestBase() override = default;

 protected:
  struct IntraPredMem {
    void Reset(libvpx_test::ACMRandom* rnd) {
      ASSERT_NE(rnd, nullptr);
      Pixel* const left = left_mem + 16;
      Pixel* const top = top_mem + 16;
      const int mask = (1 << bitdepth) - 1;
      for (auto& r : ref_src) r = rnd->Rand16() & mask;
      for (int i = 0; i < kMaxBlockSize; ++i) left[i] = rnd->Rand16() & mask;
      for (int i = -1; i < kMaxBlockSize; ++i) top[i] = rnd->Rand16() & mask;

      // Some directional predictors require top-right, bottom-left.
      for (int i = kMaxBlockSize; i < 2 * kMaxBlockSize; ++i) {
        left[i] = rnd->Rand16() & mask;
        top[i] = rnd->Rand16() & mask;
      }
      // TODO(jzern): reorder this and regenerate the digests after switching
      // random number generators.
      // Upsampling in the directional predictors extends left/top[-1] to [-2].
      left[-1] = rnd->Rand16() & mask;
      left[-2] = rnd->Rand16() & mask;
      top[-2] = rnd->Rand16() & mask;
      memset(left_mem, 0, sizeof(left_mem[0]) * 14);
      memset(top_mem, 0, sizeof(top_mem[0]) * 14);
      memset(top_mem + kMaxBlockSize * 2 + 16, 0,
             sizeof(top_mem[0]) * kTopMemPadding);
    }

    // Set ref_src, top-left, top and left to |pixel|.
    void Set(const Pixel pixel) {
      Pixel* const left = left_mem + 16;
      Pixel* const top = top_mem + 16;
      for (auto& r : ref_src) r = pixel;
      // Upsampling in the directional predictors extends left/top[-1] to [-2].
      for (int i = -2; i < 2 * kMaxBlockSize; ++i) {
        left[i] = top[i] = pixel;
      }
    }

    // DirectionalZone1_Large() overreads up to 7 pixels in |top_mem|.
    static constexpr int kTopMemPadding = 7;
    alignas(kMaxAlignment) Pixel dst[kTotalPixels];
    alignas(kMaxAlignment) Pixel ref_src[kTotalPixels];
    alignas(kMaxAlignment) Pixel left_mem[kMaxBlockSize * 2 + 16];
    alignas(
        kMaxAlignment) Pixel top_mem[kMaxBlockSize * 2 + 16 + kTopMemPadding];
  };

  void SetUp() override { test_utils::ResetDspTable(bitdepth); }

  const TransformSize tx_size_ = GetParam();
  int block_width_;
  int block_height_;
  IntraPredMem intra_pred_mem_;
};

//------------------------------------------------------------------------------
// IntraPredTest

template <int bitdepth, typename Pixel>
class IntraPredTest : public IntraPredTestBase<bitdepth, Pixel> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  IntraPredTest() = default;
  IntraPredTest(const IntraPredTest&) = delete;
  IntraPredTest& operator=(const IntraPredTest&) = delete;
  ~IntraPredTest() override = default;

 protected:
  using IntraPredTestBase<bitdepth, Pixel>::tx_size_;
  using IntraPredTestBase<bitdepth, Pixel>::block_width_;
  using IntraPredTestBase<bitdepth, Pixel>::block_height_;
  using IntraPredTestBase<bitdepth, Pixel>::intra_pred_mem_;

  void SetUp() override {
    IntraPredTestBase<bitdepth, Pixel>::SetUp();
    IntraPredInit_C();
    IntraPredSmoothInit_C();

    const Dsp* const dsp = GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);
    memcpy(base_intrapreds_, dsp->intra_predictors[tx_size_],
           sizeof(base_intrapreds_));

    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const char* const test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
      memset(base_intrapreds_, 0, sizeof(base_intrapreds_));
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      if ((GetCpuInfo() & kSSE4_1) != 0) {
        IntraPredInit_SSE4_1();
        IntraPredSmoothInit_SSE4_1();
      }
    } else if (absl::StartsWith(test_case, "NEON/")) {
      IntraPredInit_NEON();
      IntraPredSmoothInit_NEON();
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }

    memcpy(cur_intrapreds_, dsp->intra_predictors[tx_size_],
           sizeof(cur_intrapreds_));

    for (int i = 0; i < kNumIntraPredictors; ++i) {
      // skip functions that haven't been specialized for this particular
      // architecture.
      if (cur_intrapreds_[i] == base_intrapreds_[i]) {
        cur_intrapreds_[i] = nullptr;
      }
    }
  }

  // These tests modify intra_pred_mem_.
  void TestSpeed(const char* const digests[kNumIntraPredictors], int num_runs);
  void TestSaturatedValues();
  void TestRandomValues();

  IntraPredictorFunc base_intrapreds_[kNumIntraPredictors];
  IntraPredictorFunc cur_intrapreds_[kNumIntraPredictors];
};

template <int bitdepth, typename Pixel>
void IntraPredTest<bitdepth, Pixel>::TestSpeed(
    const char* const digests[kNumIntraPredictors], const int num_runs) {
  ASSERT_NE(digests, nullptr);
  const auto* const left =
      reinterpret_cast<const uint8_t*>(intra_pred_mem_.left_mem + 16);
  const auto* const top =
      reinterpret_cast<const uint8_t*>(intra_pred_mem_.top_mem + 16);

  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
  intra_pred_mem_.Reset(&rnd);

  for (int i = 0; i < kNumIntraPredictors; ++i) {
    if (cur_intrapreds_[i] == nullptr) continue;
    memcpy(intra_pred_mem_.dst, intra_pred_mem_.ref_src,
           sizeof(intra_pred_mem_.dst));
    const absl::Time start = absl::Now();
    for (int run = 0; run < num_runs; ++run) {
      const ptrdiff_t stride = kMaxBlockSize * sizeof(Pixel);
      cur_intrapreds_[i](intra_pred_mem_.dst, stride, top, left);
    }
    const absl::Duration elapsed_time = absl::Now() - start;
    test_utils::CheckMd5Digest(ToString(tx_size_),
                               ToString(static_cast<IntraPredictor>(i)),
                               digests[i], intra_pred_mem_.dst,
                               sizeof(intra_pred_mem_.dst), elapsed_time);
  }
}

template <int bitdepth, typename Pixel>
void IntraPredTest<bitdepth, Pixel>::TestSaturatedValues() {
  Pixel* const left = intra_pred_mem_.left_mem + 16;
  Pixel* const top = intra_pred_mem_.top_mem + 16;
  const auto kMaxPixel = static_cast<Pixel>((1 << bitdepth) - 1);
  intra_pred_mem_.Set(kMaxPixel);

  // skip DcFill
  for (int i = 1; i < kNumIntraPredictors; ++i) {
    if (cur_intrapreds_[i] == nullptr) continue;
    memcpy(intra_pred_mem_.dst, intra_pred_mem_.ref_src,
           sizeof(intra_pred_mem_.dst));
    const ptrdiff_t stride = kMaxBlockSize * sizeof(Pixel);
    cur_intrapreds_[i](intra_pred_mem_.dst, stride, top, left);
    if (!test_utils::CompareBlocks(intra_pred_mem_.dst, intra_pred_mem_.ref_src,
                                   block_width_, block_height_, kMaxBlockSize,
                                   kMaxBlockSize, true)) {
      ADD_FAILURE() << "Expected " << ToString(static_cast<IntraPredictor>(i))
                    << " to produce a block containing '"
                    << static_cast<int>(kMaxPixel) << "'";
    }
  }
}

template <int bitdepth, typename Pixel>
void IntraPredTest<bitdepth, Pixel>::TestRandomValues() {
  // Use an alternate seed to differentiate this test from TestSpeed().
  libvpx_test::ACMRandom rnd(test_utils::kAlternateDeterministicSeed);
  for (int i = 0; i < kNumIntraPredictors; ++i) {
    // Skip the 'C' test case as this is used as the reference.
    if (base_intrapreds_[i] == nullptr) continue;
    if (cur_intrapreds_[i] == nullptr) continue;
    // It may be worthwhile to temporarily increase this loop size when testing
    // changes that specifically affect this test.
    for (int n = 0; n < 10000; ++n) {
      intra_pred_mem_.Reset(&rnd);

      memcpy(intra_pred_mem_.dst, intra_pred_mem_.ref_src,
             sizeof(intra_pred_mem_.dst));
      const Pixel* const top = intra_pred_mem_.top_mem + 16;
      const Pixel* const left = intra_pred_mem_.left_mem + 16;
      const ptrdiff_t stride = kMaxBlockSize * sizeof(Pixel);
      base_intrapreds_[i](intra_pred_mem_.ref_src, stride, top, left);
      cur_intrapreds_[i](intra_pred_mem_.dst, stride, top, left);
      if (!test_utils::CompareBlocks(
              intra_pred_mem_.dst, intra_pred_mem_.ref_src, block_width_,
              block_height_, kMaxBlockSize, kMaxBlockSize, true)) {
        ADD_FAILURE() << "Result from optimized version of "
                      << ToString(static_cast<IntraPredictor>(i))
                      << " differs from reference in iteration #" << n;
        break;
      }
    }
  }
}

//------------------------------------------------------------------------------

using IntraPredTest8bpp = IntraPredTest<8, uint8_t>;

const char* const* GetIntraPredDigests8bpp(TransformSize tx_size) {
  static const char* const kDigests4x4[kNumIntraPredictors] = {
      "7b1c762e28747f885d2b7d83cb8aa75c", "73353f179207f1432d40a132809e3a50",
      "80c9237c838b0ec0674ccb070df633d5", "1cd79116b41fda884e7fa047f5eb14df",
      "33211425772ee539a59981a2e9dc10c1", "d6f5f65a267f0e9a2752e8151cc1dcd7",
      "7ff8c762cb766eb0665682152102ce4b", "2276b861ae4599de15938651961907ec",
      "766982bc69f4aaaa8e71014c2dc219bc", "e2c31b5fd2199c49e17c31610339ab3f",
  };
  static const char* const kDigests4x8[kNumIntraPredictors] = {
      "0a0d8641ecfa0e82f541acdc894d5574", "1a40371af6cff9c278c5b0def9e4b3e7",
      "3631a7a99569663b514f15b590523822", "646c7b592136285bd31501494e7393e7",
      "ecbe89cc64dc2688123d3cfe865b5237", "79048e70ecbb7d43a4703f62718588c0",
      "f3de11bf1198a00675d806d29c41d676", "32bb6cd018f6e871c342fcc21c7180cf",
      "6f076a1e5ab3d69cf08811d62293e4be", "2a84460a8b189b4589824cf6b3b39954",
  };
  static const char* const kDigests4x16[kNumIntraPredictors] = {
      "cb8240be98444ede5ae98ca94afc1557", "460acbcf825a1fa0d8f2aa6bf2d6a21c",
      "7896fdbbfe538dce1dc3a5b0873d74b0", "504aea29c6b27f21555d5516b8de2d8a",
      "c5738e7fa82b91ea0e39232120da56ea", "19abbd934c243a6d9df7585d81332dd5",
      "9e42b7b342e45c842dfa8aedaddbdfaa", "0e9eb07a89f8bf96bc219d5d1c3d9f6d",
      "659393c31633e0f498bae384c9df5c7b", "bee3a28312da99dd550ec309ae4fff25",
  };
  static const char* const kDigests8x4[kNumIntraPredictors] = {
      "5950744064518f77867c8e14ebd8b5d7", "46b6cbdc76efd03f4ac77870d54739f7",
      "efe21fd1b98cb1663950e0bf49483b3b", "3c647b64760b298092cbb8e2f5c06bfd",
      "c3595929687ffb04c59b128d56e2632f", "d89ad2ddf8a74a520fdd1d7019fd75b4",
      "53907cb70ad597ee5885f6c58201f98b", "09d2282a29008b7fb47eb60ed6653d06",
      "e341fc1c910d7cb2dac5dbc58b9c9af9", "a8fabd4c259b607a90a2e4d18cae49de",
  };
  static const char* const kDigests8x8[kNumIntraPredictors] = {
      "06fb7cb52719855a38b4883b4b241749", "2013aafd42a4303efb553e42264ab8b0",
      "2f070511d5680c12ca73a20e47fd6e23", "9923705af63e454392625794d5459fe0",
      "04007a0d39778621266e2208a22c4fac", "2d296c202d36b4a53f1eaddda274e4a1",
      "c87806c220d125c7563c2928e836fbbd", "339b49710a0099087e51ab5afc8d8713",
      "c90fbc020afd9327bf35dccae099bf77", "95b356a7c346334d29294a5e2d13cfd9",
  };
  static const char* const kDigests8x16[kNumIntraPredictors] = {
      "3c5a4574d96b5bb1013429636554e761", "8cf56b17c52d25eb785685f2ab48b194",
      "7911e2e02abfbe226f17529ac5db08fc", "064e509948982f66a14293f406d88d42",
      "5c443aa713891406d5be3af4b3cf67c6", "5d2cb98e532822ca701110cda9ada968",
      "3d58836e17918b8890012dd96b95bb9d", "20e8d61ddc451b9e553a294073349ffd",
      "a9aa6cf9d0dcf1977a1853ccc264e40b", "103859f85750153f47b81f68ab7881f2",
  };
  static const char* const kDigests8x32[kNumIntraPredictors] = {
      "b393a2db7a76acaccc39e04d9dc3e8ac", "bbda713ee075a7ef095f0f479b5a1f82",
      "f337dce3980f70730d6f6c2c756e3b62", "796189b05dc026e865c9e95491b255d1",
      "ea932c21e7189eeb215c1990491320ab", "a9fffdf9455eba5e3b01317cae140289",
      "9525dbfdbf5fba61ef9c7aa5fe887503", "8c6a7e3717ff8a459f415c79bb17341c",
      "3761071bfaa2363a315fe07223f95a2d", "0e5aeb9b3f485b90df750469f60c15aa",
  };
  static const char* const kDigests16x4[kNumIntraPredictors] = {
      "1c0a950b3ac500def73b165b6a38467c", "95e7f7300f19da280c6a506e40304462",
      "28a6af15e31f76d3ff189012475d78f5", "e330d67b859bceef62b96fc9e1f49a34",
      "36eca3b8083ce2fb5f7e6227dfc34e71", "08f567d2abaa8e83e4d9b33b3f709538",
      "dc2d0ba13aa9369446932f03b53dc77d", "9ab342944c4b1357aa79d39d7bebdd3a",
      "77ec278c5086c88b91d68eef561ed517", "60fbe11bfe216c182aaacdec326c4dae",
  };
  static const char* const kDigests16x8[kNumIntraPredictors] = {
      "053a2bc4b5b7287fee524af4e77f077a", "619b720b13f14f32391a99ea7ff550d5",
      "728d61c11b06baf7fe77881003a918b9", "889997b89a44c9976cb34f573e2b1eea",
      "b43bfc31d1c770bb9ca5ca158c9beec4", "9d3fe9f762e0c6e4f114042147c50c7f",
      "c74fdd7c9938603b01e7ecf9fdf08d61", "870c7336db1102f80f74526bd5a7cf4e",
      "3fd5354a6190903d6a0b661fe177daf6", "409ca6b0b2558aeadf5ef2b8a887e67a",
  };
  static const char* const kDigests16x16[kNumIntraPredictors] = {
      "1fa9e2086f6594bda60c30384fbf1635", "2098d2a030cd7c6be613edc74dc2faf8",
      "f3c72b0c8e73f1ddca04d14f52d194d8", "6b31f2ee24cf88d3844a2fc67e1f39f3",
      "d91a22a83575e9359c5e4871ab30ddca", "24c32a0d38b4413d2ef9bf1f842c8634",
      "6e9e47bf9da9b2b9ae293e0bbd8ff086", "968b82804b5200b074bcdba9718140d4",
      "4e6d7e612c5ae0bbdcc51a453cd1db3f", "ce763a41977647d072f33e277d69c7b9",
  };
  static const char* const kDigests16x32[kNumIntraPredictors] = {
      "01afd04432026ff56327d6226b720be2", "a6e7be906cc6f1e7a520151bfa7c303d",
      "bc05c46f18d0638f0228f1de64f07cd5", "204e613e429935f721a5b29cec7d44bb",
      "aa0a7c9a7482dfc06d9685072fc5bafd", "ffb60f090d83c624bb4f7dc3a630ac4f",
      "36bcb9ca9bb5eac520b050409de25da5", "34d9a5dd3363668391bc3bd05b468182",
      "1e149c28db8b234e43931c347a523794", "6e8aff02470f177c3ff4416db79fc508",
  };
  static const char* const kDigests16x64[kNumIntraPredictors] = {
      "727797ef15ccd8d325476fe8f12006a3", "f77c544ac8035e01920deae40cee7b07",
      "12b0c69595328c465e0b25e0c9e3e9fc", "3b2a053ee8b05a8ac35ad23b0422a151",
      "f3be77c0fe67eb5d9d515e92bec21eb7", "f1ece6409e01e9dd98b800d49628247d",
      "efd2ec9bfbbd4fd1f6604ea369df1894", "ec703de918422b9e03197ba0ed60a199",
      "739418efb89c07f700895deaa5d0b3e3", "9943ae1bbeeebfe1d3a92dc39e049d63",
  };
  static const char* const kDigests32x8[kNumIntraPredictors] = {
      "4da55401331ed98acec0c516d7307513", "0ae6f3974701a5e6c20baccd26b4ca52",
      "79b799f1eb77d5189535dc4e18873a0e", "90e943adf3de4f913864dce4e52b4894",
      "5e1b9cc800a89ef45f5bdcc9e99e4e96", "3103405df20d254cbf32ac30872ead4b",
      "648550e369b77687bff3c7d6f249b02f", "f9f73bcd8aadfc059fa260325df957a1",
      "204cef70d741c25d4fe2b1d10d2649a5", "04c05e18488496eba64100faa25e8baf",
  };
  static const char* const kDigests32x16[kNumIntraPredictors] = {
      "86ad1e1047abaf9959150222e8f19593", "1908cbe04eb4e5c9d35f1af7ffd7ee72",
      "6ad3bb37ebe8374b0a4c2d18fe3ebb6a", "08d3cfe7a1148bff55eb6166da3378c6",
      "656a722394764d17b6c42401b9e0ad3b", "4aa00c192102efeb325883737e562f0d",
      "9881a90ca88bca4297073e60b3bb771a", "8cd74aada398a3d770fc3ace38ecd311",
      "0a927e3f5ff8e8338984172cc0653b13", "d881d68b4eb3ee844e35e04ad6721f5f",
  };
  static const char* const kDigests32x32[kNumIntraPredictors] = {
      "1303ca680644e3d8c9ffd4185bb2835b", "2a4d9f5cc8da307d4cf7dc021df10ba9",
      "ced60d3f4e4b011a6a0314dd8a4b1fd8", "ced60d3f4e4b011a6a0314dd8a4b1fd8",
      "1464b01aa928e9bd82c66bad0f921693", "90deadfb13d7c3b855ba21b326c1e202",
      "af96a74f8033dff010e53a8521bc6f63", "9f1039f2ef082aaee69fcb7d749037c2",
      "3f82893e478e204f2d254b34222d14dc", "ddb2b95ffb65b84dd4ff1f7256223305",
  };
  static const char* const kDigests32x64[kNumIntraPredictors] = {
      "e1e8ed803236367821981500a3d9eebe", "0f46d124ba9f48cdd5d5290acf786d6d",
      "4e2a2cfd8f56f15939bdfc753145b303", "0ce332b343934b34cd4417725faa85cb",
      "1d2f8e48e3adb7c448be05d9f66f4954", "9fb2e176636a5689b26f73ca73fcc512",
      "e720ebccae7e25e36f23da53ae5b5d6a", "86fe4364734169aaa4520d799890d530",
      "b1870290764bb1b100d1974e2bd70f1d", "ce5b238e19d85ef69d85badfab4e63ae",
  };
  static const char* const kDigests64x16[kNumIntraPredictors] = {
      "de1b736e9d99129609d6ef3a491507a0", "516d8f6eb054d74d150e7b444185b6b9",
      "69e462c3338a9aaf993c3f7cfbc15649", "821b76b1494d4f84d20817840f719a1a",
      "fd9b4276e7affe1e0e4ce4f428058994", "cd82fd361a4767ac29a9f406b480b8f3",
      "2792c2f810157a4a6cb13c28529ff779", "1220442d90c4255ba0969d28b91e93a6",
      "c7253e10b45f7f67dfee3256c9b94825", "879792198071c7e0b50b9b5010d8c18f",
  };
  static const char* const kDigests64x32[kNumIntraPredictors] = {
      "e48e1ac15e97191a8fda08d62fff343e", "80c15b303235f9bc2259027bb92dfdc4",
      "538424b24bd0830f21788e7238ca762f", "a6c5aeb722615089efbca80b02951ceb",
      "12604b37875533665078405ef4582e35", "0048afa17bd3e1632d68b96048836530",
      "07a0cfcb56a5eed50c4bd6c26814336b", "529d8a070de5bc6531fa3ee8f450c233",
      "33c50a11c7d78f72434064f634305e95", "e0ef7f0559c1a50ec5a8c12011b962f7",
  };
  static const char* const kDigests64x64[kNumIntraPredictors] = {
      "a1650dbcd56e10288c3e269eca37967d", "be91585259bc37bf4dc1651936e90b3e",
      "afe020786b83b793c2bbd9468097ff6e", "6e1094fa7b50bc813aa2ba29f5df8755",
      "9e5c34f3797e0cdd3cd9d4c05b0d8950", "bc87be7ac899cc6a28f399d7516c49fe",
      "9811fd0d2dd515f06122f5d1bd18b784", "3c140e466f2c2c0d9cb7d2157ab8dc27",
      "9543de76c925a8f6adc884cc7f98dc91", "df1df0376cc944afe7e74e94f53e575a",
  };

  switch (tx_size) {
    case kTransformSize4x4:
      return kDigests4x4;
    case kTransformSize4x8:
      return kDigests4x8;
    case kTransformSize4x16:
      return kDigests4x16;
    case kTransformSize8x4:
      return kDigests8x4;
    case kTransformSize8x8:
      return kDigests8x8;
    case kTransformSize8x16:
      return kDigests8x16;
    case kTransformSize8x32:
      return kDigests8x32;
    case kTransformSize16x4:
      return kDigests16x4;
    case kTransformSize16x8:
      return kDigests16x8;
    case kTransformSize16x16:
      return kDigests16x16;
    case kTransformSize16x32:
      return kDigests16x32;
    case kTransformSize16x64:
      return kDigests16x64;
    case kTransformSize32x8:
      return kDigests32x8;
    case kTransformSize32x16:
      return kDigests32x16;
    case kTransformSize32x32:
      return kDigests32x32;
    case kTransformSize32x64:
      return kDigests32x64;
    case kTransformSize64x16:
      return kDigests64x16;
    case kTransformSize64x32:
      return kDigests64x32;
    case kTransformSize64x64:
      return kDigests64x64;
    default:
      ADD_FAILURE() << "Unknown transform size: " << tx_size;
      return nullptr;
  }
}

TEST_P(IntraPredTest8bpp, DISABLED_Speed) {
  const auto num_runs =
      static_cast<int>(2.0e9 / (block_width_ * block_height_));
  TestSpeed(GetIntraPredDigests8bpp(tx_size_), num_runs);
}

TEST_P(IntraPredTest8bpp, FixedInput) {
  TestSpeed(GetIntraPredDigests8bpp(tx_size_), 1);
}

TEST_P(IntraPredTest8bpp, Overflow) { TestSaturatedValues(); }
TEST_P(IntraPredTest8bpp, Random) { TestRandomValues(); }

//------------------------------------------------------------------------------

#if LIBGAV1_MAX_BITDEPTH >= 10
using IntraPredTest10bpp = IntraPredTest<10, uint16_t>;

const char* const* GetIntraPredDigests10bpp(TransformSize tx_size) {
  static const char* const kDigests4x4[kNumIntraPredictors] = {
      "432bf9e762416bec582cb3654cbc4545", "8b9707ff4d506e0cb326f2d9a8d78705",
      "a076275258cc5af87ed8b075136fb219", "f9587004012a8d2cecaa347331ccdf96",
      "1c4e6890c5e6eed495fe54a6b6df8d6f", "0ae15fae8969a3c972ee895f325955a3",
      "97db177738b831da8066df4f3fb7adbd", "4add5685b8a56991c9dce4ff7086ec25",
      "75c6a655256188e378e70658b8f1631f", "14a27db20f9d5594ef74a7ea10c3e5ef",
  };
  static const char* const kDigests4x8[kNumIntraPredictors] = {
      "9cbd7c18aca2737fa41db27150798819", "13d1e734692e27339c10b07da33c1113",
      "0617cf74e2dd5d34ea517af1767fa47e", "c6a7b01228ccdf74af8528ef8f5f55c6",
      "13b05d87b3d566b2f7a4b332cd8a762e", "b26ae0e8da1fe8989dfe2900fa2c3847",
      "c30f3acdd386bdac91028fe48b751810", "04d2baf5192c5af97ca18d3b9b0d5968",
      "a0ef82983822fc815bf1e8326cd41e33", "20bf218bae5f6b5c6d56b85f3f9bbadb",
  };
  static const char* const kDigests4x16[kNumIntraPredictors] = {
      "d9b47bdddaa5e22312ff9ece7a3cae08", "cb76c79971b502dd8999a7047b3e2f86",
      "3b09a3ff431d03b379acfdc444602540", "88608f6fcd687831e871053723cf76c3",
      "a7bd2a17de1cf19c9a4b2c550f277a5c", "29b389f564f266a67687b8d2bc750418",
      "4680847c30fe93c06f87e2ee1da544d6", "0e4eda11e1fe6ebe8526c2a2c5390bbb",
      "bf3e20197282885acabb158f3a77ba59", "fccea71d1a253316b905f4a073c84a36",
  };
  static const char* const kDigests8x4[kNumIntraPredictors] = {
      "05ba0ed96aac48cd94e7597f12184320", "d97d04e791904d3cedc34d5430a4d1d2",
      "49217081a169c2d30b0a43f816d0b58b", "09e2a6a6bfe35b83e9434ee9c8dcf417",
      "4b03c8822169ee4fa058513d65f0e32f", "cabdeebc923837ee3f2d3480354d6a81",
      "957eda610a23a011ed25976aee94eaf0", "4a197e3dfce1f0d3870138a9b66423aa",
      "18c0d0fbe0e96a0baf2f98fa1908cbb9", "21114e5737328cdbba9940e4f85a0855",
  };
  static const char* const kDigests8x8[kNumIntraPredictors] = {
      "430e99eecda7e6434e1973dbdcc2a29d", "88864d7402c09b57735db49c58707304",
      "8312f80b936380ceb51375e29a4fd75d", "472a7ed9c68bdbd9ecca197b7a8b3f01",
      "4f66ee4dc0cb752c3b65d576cd06bb5c", "36383d6f61799143470129e2d5241a6f",
      "c96279406c8d2d02771903e93a4e8d37", "4fb64f9700ed0bf08fbe7ab958535348",
      "c008c33453ac9cf8c42ae6ec88f9941c", "39c401a9938b23e318ae7819e458daf1",
  };
  static const char* const kDigests8x16[kNumIntraPredictors] = {
      "bda6b75fedfe0705f9732ff84c918672", "4ff130a47429e0762386557018ec10b2",
      "8156557bf938d8e3a266318e57048fc5", "bdfa8e01a825ec7ae2d80519e3c94eec",
      "108fc8e5608fe09f9cc30d7a52cbc0c1", "a2271660af5424b64c6399ca5509dee1",
      "b09af9729f39516b28ff62363f8c0cb2", "4fe67869dac99048dfcf4d4e621884ec",
      "311f498369a9c98f77a961bf91e73e65", "d66e78b9f41d5ee6a4b25e37ec9af324",
  };
  static const char* const kDigests8x32[kNumIntraPredictors] = {
      "26c45325f02521e7e5c66c0aa0819329", "79dfb68513d4ccd2530c485f0367858e",
      "8288e99b4d738b13956882c3ad3f03fe", "7c4993518b1620b8be8872581bb72239",
      "2b1c3126012d981f787ed0a2601ee377", "051ba9f0c4d4fecb1fcd81fdea94cae4",
      "320362239ad402087303a4df39512bb1", "210df35b2055c9c01b9e3e5ae24e524b",
      "f8536db74ce68c0081bbd8799dac25f9", "27f2fe316854282579906d071af6b705",
  };
  static const char* const kDigests16x4[kNumIntraPredictors] = {
      "decff67721ff7e9e65ec641e78f5ccf3", "99e3b2fbdabfa9b76b749cfb6530a9fd",
      "accdb3d25629916963a069f1e1c0e061", "ad42855e9146748b0e235b8428487b4b",
      "53025e465f267e7af2896ebd028447a0", "577d26fcd2d655cc77a1f1f875648699",
      "7a61a3619267221b448b20723840e9f0", "fb4ccc569bdae3614e87bc5be1e84284",
      "b866095d8a3e6910cc4f92f8d8d6075a", "6ba9013cba1624872bfbac111e8d344a",
  };
  static const char* const kDigests16x8[kNumIntraPredictors] = {
      "2832156bd076c75f8be5622f34cb3efe", "da70e516f5a8842dd4965b80cd8d2a76",
      "c3e137c6d79c57be2073d1eda22c8d1e", "8c5d28c7b3301b50326582dd7f89a175",
      "9d8558775155b201cd178ab61458b642", "ecbddb9c6808e0c609c8fe537b7f7408",
      "29a123c22cb4020170f9a80edf1208da", "653d0cd0688aa682334156f7b4599b34",
      "1bfa66ae92a22a0346511db1713fe7df", "1802ad1e657e7fc08fc063342f471ca1",
  };
  static const char* const kDigests16x16[kNumIntraPredictors] = {
      "2270c626de9d49769660ae9184a6428f", "9f069625cdcdd856e2e7ec19ff4fcd50",
      "34167b9c413362a377aa7b1faf92ae6d", "3cec2b23d179765daea8dfb87c9efdd5",
      "daa8f0863a5df2aef2b20999961cc8f8", "d9e4dd4bc63991e4f09cb97eb25f4db4",
      "4e1a182fc3fcf5b9f5a73898f81c2004", "c58e4275406c9fd1c2a74b40c27afff0",
      "b8092796fd4e4dd9d2b92afb770129ba", "75424d1f18ff00c4093743d033c6c9b6",
  };
  static const char* const kDigests16x32[kNumIntraPredictors] = {
      "5aa050947f3d488537f5a68c23bb135b", "9e66143a2c3863b6fe171275a192d378",
      "86b0c4777625e84d52913073d234f860", "9e2144fcf2107c76cec4241416bbecd5",
      "c72be592efc72c3c86f2359b6f622aba", "c4e0e735545f78f43e21e9c39eab7b8f",
      "52122e7c84a4bab67a8a359efb427023", "7b5fd8bb7e0744e81fd6fa4ed4c2e0fb",
      "a9950d110bffb0411a8fcd1262dceef0", "2a2dd496f01f5d87f257ed202a703cbe",
  };
  static const char* const kDigests16x64[kNumIntraPredictors] = {
      "eeb1b873e81ca428b11f162bd5b28843", "39ce7d22791f82562b0ca1e0afdf1604",
      "6bd6bdac8982a4b84613f9963d35d5e9", "a9ac2438e87522621c7e6fe6d02c01ab",
      "a8b9c471fe6c66ed0717e77fea77bba1", "e050b6aa38aee6e951d3be5a94a8abd0",
      "3c5ecc31aa45e8175d37e90af247bca6", "30c0f9e412ea726970f575f910edfb94",
      "f3d96395816ce58fb98480a5b4c32ab2", "9c14811957e013fb009dcd4a3716b338",
  };
  static const char* const kDigests32x8[kNumIntraPredictors] = {
      "d6560d7fc9ae9bd7c25e2983b4a825e3", "90a67154bbdc26cd06ab0fa25fff3c53",
      "c42d37c5a634e68fafc982626842db0b", "ecc8646d258cfa431facbc0dba168f80",
      "9f3c167b790b52242dc8686c68eac389", "62dc3bc34406636ccec0941579461f65",
      "5c0f0ebdb3c936d4decc40d5261aec7c", "dbfc0f056ca25e0331042da6d292e10a",
      "14fa525d74e6774781198418d505c595", "5f95e70db03da9ed70cd79e23f19199c",
  };
  static const char* const kDigests32x16[kNumIntraPredictors] = {
      "dfe3630aa9eeb1adcc8604269a309f26", "ba6180227d09f5a573f69dc6ee1faf80",
      "03edea9d71ca3d588e1a0a69aecdf555", "2c8805415f44b4fac6692090dc1b1ddd",
      "18efd17ed72a6e92ef8b0a692cf7a2e3", "63a6e0abfb839b43c68c23b2c43c8918",
      "be15479205bb60f5a17baaa81a6b47ad", "243d21e1d9f9dd2b981292ac7769315a",
      "21de1cb5269e0e1d08930c519e676bf7", "73065b3e27e9c4a3a6d043712d3d8b25",
  };
  static const char* const kDigests32x32[kNumIntraPredictors] = {
      "c3136bb829088e33401b1affef91f692", "68bbcf93d17366db38bbc7605e07e322",
      "2786be5fb7c25eeec4d2596c4154c3eb", "25ac7468e691753b8291be859aac7493",
      "a6805ce21bfd26760e749efc8f590fa3", "5a38fd324b466e8ac43f5e289d38107e",
      "dd0628fc5cc920b82aa941378fa907c8", "8debadbdb2dec3dc7eb43927e9d36998",
      "61e1bc223c9e04c64152cc4531b6c099", "900b00ac1f20c0a8d22f8b026c0ee1cc",
  };
  static const char* const kDigests32x64[kNumIntraPredictors] = {
      "5a591b2b83f0a6cce3c57ce164a5f983", "f42167ec516102b83b2c5176df57316b",
      "58f3772d3df511c8289b340beb178d96", "c24166e7dc252d34ac6f92712956d751",
      "7dca3acfe2ea09e6292a9ece2078b827", "5c029235fc0820804e40187d2b22a96e",
      "375572944368afbc04ca97dab7fb3328", "8867235908736fd99c4022e4ed604e6e",
      "63ec336034d62846b75558c49082870f", "46f35d85eb8499d61bfeac1c49e52531",
  };
  static const char* const kDigests64x16[kNumIntraPredictors] = {
      "67755882209304659a0e6bfc324e16b9", "cd89b272fecb5f23431b3f606f590722",
      "9bcff7d971a4af0a2d1cac6d66d83482", "d8d6bb55ebeec4f03926908d391e15ba",
      "0eb5b5ced3e7177a1dd6a1e72e7a7d21", "92b47fe431d9cf66f9e601854f0f3017",
      "7dc599557eddb2ea480f86fc89c76b30", "4f40175676c164320fe8005440ad9217",
      "b00eacb24081a041127f136e9e5983ec", "cb0ab76a5e90f2eb75c38b99b9833ff8",
  };
  static const char* const kDigests64x32[kNumIntraPredictors] = {
      "21d873011d1b4ef1daedd9aa8c6938ea", "4866da21db0261f738903d97081cb785",
      "a722112233a82595a8d001a4078b834d", "24c7a133c6fcb59129c3782ef908a6c1",
      "490e40505dd255d3a909d8a72c280cbc", "2afe719fb30bf2a664829bb74c8f9e2a",
      "623adad2ebb8f23e355cd77ace4616cd", "d6092541e9262ad009bef79a5d350a86",
      "ae86d8fba088683ced8abfd7e1ddf380", "32aa8aa21f2f24333d31f99e12b95c53",
  };
  static const char* const kDigests64x64[kNumIntraPredictors] = {
      "6d88aeb40dfe3ac43c68808ca3c00806", "6a75d88ac291d6a3aaf0eec0ddf2aa65",
      "30ef52d7dc451affdd587c209f5cb2dd", "e073f7969f392258eaa907cf0636452a",
      "de10f07016a2343bcd3a9deb29f4361e", "dc35ff273fea4355d2c8351c2ed14e6e",
      "01b9a545968ac75c3639ddabb837fa0b", "85c98ed9c0ea1523a15281bc9a909b8c",
      "4c255f7ef7fd46db83f323806d79dca4", "fe2fe6ffb19cb8330e2f2534271d6522",
  };

  switch (tx_size) {
    case kTransformSize4x4:
      return kDigests4x4;
    case kTransformSize4x8:
      return kDigests4x8;
    case kTransformSize4x16:
      return kDigests4x16;
    case kTransformSize8x4:
      return kDigests8x4;
    case kTransformSize8x8:
      return kDigests8x8;
    case kTransformSize8x16:
      return kDigests8x16;
    case kTransformSize8x32:
      return kDigests8x32;
    case kTransformSize16x4:
      return kDigests16x4;
    case kTransformSize16x8:
      return kDigests16x8;
    case kTransformSize16x16:
      return kDigests16x16;
    case kTransformSize16x32:
      return kDigests16x32;
    case kTransformSize16x64:
      return kDigests16x64;
    case kTransformSize32x8:
      return kDigests32x8;
    case kTransformSize32x16:
      return kDigests32x16;
    case kTransformSize32x32:
      return kDigests32x32;
    case kTransformSize32x64:
      return kDigests32x64;
    case kTransformSize64x16:
      return kDigests64x16;
    case kTransformSize64x32:
      return kDigests64x32;
    case kTransformSize64x64:
      return kDigests64x64;
    default:
      ADD_FAILURE() << "Unknown transform size: " << tx_size;
      return nullptr;
  }
}

TEST_P(IntraPredTest10bpp, DISABLED_Speed) {
  const auto num_runs =
      static_cast<int>(2.0e9 / (block_width_ * block_height_));
  TestSpeed(GetIntraPredDigests10bpp(tx_size_), num_runs);
}

TEST_P(IntraPredTest10bpp, FixedInput) {
  TestSpeed(GetIntraPredDigests10bpp(tx_size_), 1);
}

TEST_P(IntraPredTest10bpp, Overflow) { TestSaturatedValues(); }
TEST_P(IntraPredTest10bpp, Random) { TestRandomValues(); }
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using IntraPredTest12bpp = IntraPredTest<12, uint16_t>;

const char* const* GetIntraPredDigests12bpp(TransformSize tx_size) {
  static const char* const kDigests4x4[kNumIntraPredictors] = {
      "f7008e0f65bdeed97375ae5e98e3309b", "a34cc5d9d1ef875df4ee2ce010d0a80a",
      "74f615beeb217ad317ced813851be36a", "b3312e86313805b061c66a08e09de653",
      "2db47240c95530b39084bdacccf4bb8e", "76bb839cac394b5777c64b6d4b570a27",
      "a74ee60527be86059e822f7463f49ad5", "b157a40aaa14391c237471ba6d148a50",
      "d4f7bd2e97e2b23f7a6a059837a10b2a", "8a9bcb30e9aff59b6feef5d1bf546d28",
  };
  static const char* const kDigests4x8[kNumIntraPredictors] = {
      "4c2a59e1d4a58c129c709f05d1a83f4a", "5fbedd99a90a20727195dfbe8f9969ad",
      "d4645e21ccf5f6d3c4ca7a3d9b0156ba", "98aa17ea5423192c81a04afd2d2669ed",
      "67dad5b5eefdeb2af1e4d3875b282c6c", "881dcafd6323509fb80cd5bbdf2870c4",
      "03ece373dfd56bd2fd86ad00ad6f5000", "41b28f2578d2ed7f38e708d57b231948",
      "9f935505190f52ff4da9556e43f607be", "815700d2abb055bce6902d130e77416d",
  };
  static const char* const kDigests4x16[kNumIntraPredictors] = {
      "bfc47cd4eef143a6ebf517730756a718", "ef07a3af3e353f9dfaebc48c8ac92c82",
      "ceec5d9d24254efd3c6a00cbf11dd24d", "4e07f512a69cf95608c3c0c3013ed808",
      "cedb7c900bb6839026bf79d054edb4fc", "48d958a18a019809f12eb2ad2eb358bc",
      "8f296f4b9fb621a910368609cc2cccdf", "073a6f2ca8a23d6131ff97e2a3b736e1",
      "f4772cc60b68c4f958c08c0fd8eb8d48", "2f8946cf19abecf0fda3addbfb8f9dcf",
  };
  static const char* const kDigests8x4[kNumIntraPredictors] = {
      "4f245b07a91e6d604da9f22cf277d6f1", "a6dc25d1e24ba9e842c312f67eea211d",
      "0475204441f44ea95bfd69c6e04eaed8", "313bcf1e2fc762d31ff765d3c18a6f67",
      "7e9223ece684a1885c2108741052c6c8", "79f1e6f070d9b1d0f1de2ff77bccc0dc",
      "63adca1101ee4799b1cfa26d88aa0657", "e8b940a5e39ea5313930c903464de843",
      "42a8e470d3b000f4f57c44c632f0051b", "e8a57663f73da3d4320f8e82a3fecfc2",
  };
  static const char* const kDigests8x8[kNumIntraPredictors] = {
      "7fa3c8bdd9ce04dc4df27863499cf4d4", "83f1312edd9af928a1cef60613730bc3",
      "ceb35042adc6095a545b490f20e5d81b", "73aa503f329a055ff59a24093e682c41",
      "14a9a427525ec38d2eb13e698728e911", "9143ddf66234e74acc156565d684fcac",
      "05182bbe4fd90f3b496033ee5b7c54f9", "d9c6184c23af1f5a903a4a00539b883a",
      "c4c2d4000ca2defc7a8169215121d9fc", "0b938bc7782b32796bffece28d17bb69",
  };
  static const char* const kDigests8x16[kNumIntraPredictors] = {
      "50197f063138616c37ef09f8bf8a3016", "ef2008f6d9f2176feb17b7d4312022e2",
      "0d243ffbba0a2e65738d7ee768620c36", "51b52564a2733c2c56ba319db5d8e3b8",
      "0e2b41482ac1347c3bb6d0e394fe7bec", "edb43c19850452e6b20dfb2c001adb0b",
      "6cd29f537b5e4180f5aaefd9140b65ef", "6808f618bdff33e0f3d6db60ea487bc1",
      "0303c17746192b0c52b4d75ea97ca24d", "225d1debd7828fa01bc9a610a443cda9",
  };
  static const char* const kDigests8x32[kNumIntraPredictors] = {
      "dc047c402c6ac4014d621fbd41b460d5", "49eb33c3a112f059e02d6d4b99da8b41",
      "c906c9105a406ae6c63e69f57ed2fc7c", "2ead452591ddd2455660f96ce79314ab",
      "437a2a78562752ee8291227f88e0323a", "51834dbdcf1e89667ffbb931bec9006c",
      "959c1778e11a7c61a5a97176c79ecb6a", "2e51e44dd1953fc6fccc3b1c1ca602ed",
      "7f94114cddb0ba780cc0c8d00db3f8d2", "b5b3770e6061249a3206915a3f9464e7",
  };
  static const char* const kDigests16x4[kNumIntraPredictors] = {
      "9deb173fa962d9adde8a9ae256708c32", "264624b41e43cfe9378ee9b4fb5028a6",
      "404919a41bdc7f1a1f9d089223373bb8", "5294ed9fcc16eaf5f9a1f66a2a36ae7c",
      "a2ed1fa4262bca265dcc62eb1586f0ac", "58494af62f86464dbe471130b2bc4ab0",
      "fe1f25f7096fc3426cc7964326cc46ad", "cf7f6c8f7257436b9934cecf3b7523e1",
      "6325036f243abfcd7777754e6a7bdacc", "9dce11a98e18422b04dd9d7be7d420da",
  };
  static const char* const kDigests16x8[kNumIntraPredictors] = {
      "92d5b7d4033dcd8cb729bf8e166e339a", "6cbd9f198828fd3422c9bfaf8c2f1c1d",
      "2b204014b6dc477f67b36818bcdab1ca", "2ce0b9cf224d4654168c559d7c1424c2",
      "ec70341b9dd57b379f5283820c9461c7", "3fe1e2a20e44171c90ebca5a45b83460",
      "0305852b25351ff472a45f45ec1638fa", "565c78271fbe3b25b0eee542095be005",
      "8bc15e98659cef6236bcb072541bb2ca", "875c87bf4daba7cb436ea2fdb5a427dd",
  };
  static const char* const kDigests16x16[kNumIntraPredictors] = {
      "c9d12bce78d8846f081345906e1315f4", "0b57c8fde6dec15458b1c289245100cb",
      "1c11978c4e6bbc77767395c63d2f70a8", "e749f26b26b46d8cb7cb13c1c777db94",
      "40459af05e865e94ff7adcdec1685c15", "f3ae419e99a60dbde3afa24ba6588a36",
      "fe3912418bca24cee3132de2c193d1fc", "cdc8e3ce27a12f1cbfe01d1adf2eb6bd",
      "ce354b30ce15a6918172dea55a292b93", "e762d01726d641194982a5fb8c148eb7",
  };
  static const char* const kDigests16x32[kNumIntraPredictors] = {
      "ad8f118b07e053df3887215449633a07", "e8979aa743aef82937d93d87fc9fdb85",
      "a8afb62cbf602cfcd4b570832afe1d55", "404183cf003764a4f032f0f4810cd42c",
      "4afcf1bc5589a13b11679571aa953b86", "202df8f5a2d7eb3816de172608115f2b",
      "ce42bca92d6d7f9df85dbaac72e35064", "61c463c8070b78ca2bdf578044fec440",
      "3abf6e4d779208e15e3f9a0dfc0254f9", "13df5504084105af7c66a1b013fe44e1",
  };
  static const char* const kDigests16x64[kNumIntraPredictors] = {
      "3ac1f642019493dec1b737d7a3a1b4e5", "cbf69d5d157c9f3355a4757b1d6e3414",
      "96d00ddc7537bf7f196006591b733b4e", "8cba1b70a0bde29e8ef235cedc5faa7d",
      "35f9ee300d7fa3c97338e81a6f21dcd4", "aae335442e77c8ebc280f16ea50ba9c7",
      "a6140fdac2278644328be094d88731db", "2df93621b6ff100f7008432d509f4161",
      "c77bf5aee39e7ed4a3dd715f816f452a", "02109bd63557d90225c32a8f1338258e",
  };
  static const char* const kDigests32x8[kNumIntraPredictors] = {
      "155688dec409ff50f2333c14a6367247", "cf935e78abafa6ff7258c5af229f55b6",
      "b4bf83a28ba319c597151a041ff838c3", "fe97f3e6cd5fe6c5979670c11d940dda",
      "b898c9a989e1e72461a6f47e913d5383", "bb73baa6476ce90118e83e2fd08f2299",
      "c93be6d8ec318bd805899466821bb779", "ab366991ef842e9d417d52241f6966e6",
      "9e7e4c96a271e9e40771eac39c21f661", "9459f2e6d1291b8b8a2fe0635ce1a33d",
  };
  static const char* const kDigests32x16[kNumIntraPredictors] = {
      "48374c1241409e26d81e5106c73da420", "97c918bdba2ece52156dbc776b9b70d4",
      "a44ce9c03f6622a3e93bfe3b928eb6f1", "2384ad95e3e7302f20857121e187aa48",
      "47e72c6dc0087b6fd99e91cff854c269", "142dc3cbb05b82a496780f7fc3d66ccc",
      "4a39fb768efcd4f30d6eae816e6a68c4", "d0c31f9d52d984a0335557eafe2b47fa",
      "81b3af5c7893729b837e4d304917f7cd", "941cbcd411887dc7fa3a5c7395690d1a",
  };
  static const char* const kDigests32x32[kNumIntraPredictors] = {
      "00892ee43a1bbb11347c1f44fb94b1a2", "d66397ba868e62cec99daf5ea73bebd0",
      "65fe746e79ac1e779caae8abcc15eb6b", "8e308fe96b9845112d79c54f9d7981a0",
      "47bc8847a7c9aed3417cd5250ba57875", "1a4008b7f0f61a3c73a2ee1d1452b414",
      "24d25ef488bb457a5a4c4892e47a363d", "6d9d964f5317ab32a8edf57c23775238",
      "544fc36c1a35c588359ae492cb5bc143", "ac170d94dbd944e9723de9c18bace1a3",
  };
  static const char* const kDigests32x64[kNumIntraPredictors] = {
      "7d0bd7dea26226741dbca9a97f27fa74", "a8bdc852ef704dd4975c61893e8fbc3f",
      "f29d6d03c143ddf96fef04c19f2c8333", "ad9cfc395a5c5644a21d958c7274ac14",
      "45c27c5cca9a91b6ae8379feb0881c9f", "8a0b78df1e001b85c874d686eac4aa1b",
      "ce9fa75fac54a3f6c0cc3f2083b938f1", "c0dca10d88762c954af18dc9e3791a39",
      "61df229eddfccab913b8fda4bb02f9ac", "4f4df6bc8d50a5600b573f0e44d70e66",
  };
  static const char* const kDigests64x16[kNumIntraPredictors] = {
      "e99d072de858094c98b01bd4a6772634", "525da4b187acd81b1ff1116b60461141",
      "1348f249690d9eefe09d9ad7ead2c801", "a5e2f9fb685d5f4a048e9a96affd25a4",
      "873bfa9dc24693f19721f7c8d527f7d3", "0acfc6507bd3468e9679efc127d6e4b9",
      "57d03f8d079c7264854e22ac1157cfae", "6c2c4036f70c7d957a9399b5436c0774",
      "42b8e4a97b7f8416c72a5148c031c0b1", "a38a2c5f79993dfae8530e9e25800893",
  };
  static const char* const kDigests64x32[kNumIntraPredictors] = {
      "68bd283cfd1a125f6b2ee47cee874d36", "b4581311a0a73d95dfac7f8f44591032",
      "5ecc7fdc52d2f575ad4f2d0e9e6b1e11", "db9d82921fd88b24fdff6f849f2f9c87",
      "804179f05c032908a5e36077bb87c994", "fc5fd041a8ee779015394d0c066ee43c",
      "68f5579ccadfe9a1baafb158334a3db2", "fe237e45e215ab06d79046da9ad71e84",
      "9a8a938a6824551bf7d21b8fd1d70ea1", "eb7332f2017cd96882c76e7136aeaf53",
  };
  static const char* const kDigests64x64[kNumIntraPredictors] = {
      "d9a906c0e692b22e1b4414e71a704b7e", "12ac11889ae5f55b7781454efd706a6a",
      "3f1ef5f473a49eba743f17a3324adf9d", "a6baa0d4bfb2269a94c7a38f86a4bccf",
      "47d4cadd56f70c11ff8f3e5d8df81161", "de997744cf24c16c5ac2a36b02b351cc",
      "23781211ae178ddeb6c4bb97a6bd7d83", "a79d2e28340ca34b9e37daabbf030f63",
      "0372bd3ddfc258750a6ac106b70587f4", "228ef625d9460cbf6fa253a16a730976",
  };

  switch (tx_size) {
    case kTransformSize4x4:
      return kDigests4x4;
    case kTransformSize4x8:
      return kDigests4x8;
    case kTransformSize4x16:
      return kDigests4x16;
    case kTransformSize8x4:
      return kDigests8x4;
    case kTransformSize8x8:
      return kDigests8x8;
    case kTransformSize8x16:
      return kDigests8x16;
    case kTransformSize8x32:
      return kDigests8x32;
    case kTransformSize16x4:
      return kDigests16x4;
    case kTransformSize16x8:
      return kDigests16x8;
    case kTransformSize16x16:
      return kDigests16x16;
    case kTransformSize16x32:
      return kDigests16x32;
    case kTransformSize16x64:
      return kDigests16x64;
    case kTransformSize32x8:
      return kDigests32x8;
    case kTransformSize32x16:
      return kDigests32x16;
    case kTransformSize32x32:
      return kDigests32x32;
    case kTransformSize32x64:
      return kDigests32x64;
    case kTransformSize64x16:
      return kDigests64x16;
    case kTransformSize64x32:
      return kDigests64x32;
    case kTransformSize64x64:
      return kDigests64x64;
    default:
      ADD_FAILURE() << "Unknown transform size: " << tx_size;
      return nullptr;
  }
}

TEST_P(IntraPredTest12bpp, DISABLED_Speed) {
  const auto num_runs =
      static_cast<int>(2.0e9 / (block_width_ * block_height_));
  TestSpeed(GetIntraPredDigests12bpp(tx_size_), num_runs);
}

TEST_P(IntraPredTest12bpp, FixedInput) {
  TestSpeed(GetIntraPredDigests12bpp(tx_size_), 1);
}

TEST_P(IntraPredTest12bpp, Overflow) { TestSaturatedValues(); }
TEST_P(IntraPredTest12bpp, Random) { TestRandomValues(); }
#endif  // LIBGAV1_MAX_BITDEPTH == 12

constexpr TransformSize kTransformSizes[] = {
    kTransformSize4x4,   kTransformSize4x8,   kTransformSize4x16,
    kTransformSize8x4,   kTransformSize8x8,   kTransformSize8x16,
    kTransformSize8x32,  kTransformSize16x4,  kTransformSize16x8,
    kTransformSize16x16, kTransformSize16x32, kTransformSize16x64,
    kTransformSize32x8,  kTransformSize32x16, kTransformSize32x32,
    kTransformSize32x64, kTransformSize64x16, kTransformSize64x32,
    kTransformSize64x64};

INSTANTIATE_TEST_SUITE_P(C, IntraPredTest8bpp,
                         testing::ValuesIn(kTransformSizes));
#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, IntraPredTest8bpp,
                         testing::ValuesIn(kTransformSizes));
#endif  // LIBGAV1_ENABLE_SSE4_1
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, IntraPredTest8bpp,
                         testing::ValuesIn(kTransformSizes));
#endif  // LIBGAV1_ENABLE_NEON

#if LIBGAV1_MAX_BITDEPTH >= 10
INSTANTIATE_TEST_SUITE_P(C, IntraPredTest10bpp,
                         testing::ValuesIn(kTransformSizes));
#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, IntraPredTest10bpp,
                         testing::ValuesIn(kTransformSizes));
#endif  // LIBGAV1_ENABLE_SSE4_1
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, IntraPredTest10bpp,
                         testing::ValuesIn(kTransformSizes));
#endif  // LIBGAV1_ENABLE_NEON

#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
INSTANTIATE_TEST_SUITE_P(C, IntraPredTest12bpp,
                         testing::ValuesIn(kTransformSizes));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace
}  // namespace dsp

static std::ostream& operator<<(std::ostream& os, const TransformSize tx_size) {
  return os << ToString(tx_size);
}

}  // namespace libgav1
