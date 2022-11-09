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

#include "src/dsp/intrapred_filter.h"

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

const char* const kFilterIntraPredNames[kNumFilterIntraPredictors] = {
    "kFilterIntraPredictorDc",         "kFilterIntraPredictorVertical",
    "kFilterIntraPredictorHorizontal", "kFilterIntraPredictorD157",
    "kFilterIntraPredictorPaeth",
};

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
// FilterIntraPredTest

template <int bitdepth, typename Pixel>
class FilterIntraPredTest : public IntraPredTestBase<bitdepth, Pixel> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  FilterIntraPredTest() = default;
  FilterIntraPredTest(const FilterIntraPredTest&) = delete;
  FilterIntraPredTest& operator=(const FilterIntraPredTest&) = delete;
  ~FilterIntraPredTest() override = default;

 protected:
  using IntraPredTestBase<bitdepth, Pixel>::tx_size_;
  using IntraPredTestBase<bitdepth, Pixel>::block_width_;
  using IntraPredTestBase<bitdepth, Pixel>::block_height_;
  using IntraPredTestBase<bitdepth, Pixel>::intra_pred_mem_;

  void SetUp() override {
    IntraPredTestBase<bitdepth, Pixel>::SetUp();
    IntraPredFilterInit_C();

    const Dsp* const dsp = GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);
    base_filter_intra_pred_ = dsp->filter_intra_predictor;

    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const char* const test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
      // No need to compare C with itself.
      base_filter_intra_pred_ = nullptr;
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      if ((GetCpuInfo() & kSSE4_1) != 0) {
        IntraPredFilterInit_SSE4_1();
      }
    } else if (absl::StartsWith(test_case, "NEON/")) {
      IntraPredFilterInit_NEON();
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }

    // Put the current architecture-specific implementation up for testing and
    // comparison against C version.
    cur_filter_intra_pred_ = dsp->filter_intra_predictor;
  }

  // These tests modify intra_pred_mem_.
  void TestSpeed(const char* const digests[kNumFilterIntraPredictors],
                 int num_runs);
  void TestSaturatedValues();
  void TestRandomValues();

  FilterIntraPredictorFunc base_filter_intra_pred_;
  FilterIntraPredictorFunc cur_filter_intra_pred_;
};

template <int bitdepth, typename Pixel>
void FilterIntraPredTest<bitdepth, Pixel>::TestSpeed(
    const char* const digests[kNumFilterIntraPredictors], const int num_runs) {
  ASSERT_NE(digests, nullptr);
  const Pixel* const left = intra_pred_mem_.left_mem + 16;
  const Pixel* const top = intra_pred_mem_.top_mem + 16;

  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
  intra_pred_mem_.Reset(&rnd);

  // IntraPredInit_C() leaves the filter function empty.
  if (cur_filter_intra_pred_ == nullptr) return;
  for (int i = 0; i < kNumFilterIntraPredictors; ++i) {
    memcpy(intra_pred_mem_.dst, intra_pred_mem_.ref_src,
           sizeof(intra_pred_mem_.dst));
    const absl::Time start = absl::Now();
    for (int run = 0; run < num_runs; ++run) {
      const ptrdiff_t stride = kMaxBlockSize * sizeof(Pixel);
      cur_filter_intra_pred_(intra_pred_mem_.dst, stride, top, left,
                             static_cast<FilterIntraPredictor>(i), block_width_,
                             block_height_);
    }
    const absl::Duration elapsed_time = absl::Now() - start;
    test_utils::CheckMd5Digest(ToString(tx_size_), kFilterIntraPredNames[i],
                               digests[i], intra_pred_mem_.dst,
                               sizeof(intra_pred_mem_.dst), elapsed_time);
  }
}

template <int bitdepth, typename Pixel>
void FilterIntraPredTest<bitdepth, Pixel>::TestSaturatedValues() {
  Pixel* const left = intra_pred_mem_.left_mem + 16;
  Pixel* const top = intra_pred_mem_.top_mem + 16;
  const auto kMaxPixel = static_cast<Pixel>((1 << bitdepth) - 1);
  intra_pred_mem_.Set(kMaxPixel);

  // IntraPredInit_C() leaves the filter function empty.
  if (cur_filter_intra_pred_ == nullptr) return;
  for (int i = 0; i < kNumFilterIntraPredictors; ++i) {
    memcpy(intra_pred_mem_.dst, intra_pred_mem_.ref_src,
           sizeof(intra_pred_mem_.dst));
    const ptrdiff_t stride = kMaxBlockSize * sizeof(Pixel);
    cur_filter_intra_pred_(intra_pred_mem_.dst, stride, top, left,
                           static_cast<FilterIntraPredictor>(i), block_width_,
                           block_height_);
    if (!test_utils::CompareBlocks(intra_pred_mem_.dst, intra_pred_mem_.ref_src,
                                   block_width_, block_height_, kMaxBlockSize,
                                   kMaxBlockSize, true)) {
      ADD_FAILURE() << "Expected " << kFilterIntraPredNames[i]
                    << " to produce a block containing '"
                    << static_cast<int>(kMaxPixel) << "'";
    }
  }
}

template <int bitdepth, typename Pixel>
void FilterIntraPredTest<bitdepth, Pixel>::TestRandomValues() {
  // Skip the 'C' test case as this is used as the reference.
  if (base_filter_intra_pred_ == nullptr) return;

  // Use an alternate seed to differentiate this test from TestSpeed().
  libvpx_test::ACMRandom rnd(test_utils::kAlternateDeterministicSeed);
  for (int i = 0; i < kNumFilterIntraPredictors; ++i) {
    // It may be worthwhile to temporarily increase this loop size when testing
    // changes that specifically affect this test.
    for (int n = 0; n < 10000; ++n) {
      intra_pred_mem_.Reset(&rnd);

      memcpy(intra_pred_mem_.dst, intra_pred_mem_.ref_src,
             sizeof(intra_pred_mem_.dst));
      const Pixel* const top = intra_pred_mem_.top_mem + 16;
      const Pixel* const left = intra_pred_mem_.left_mem + 16;
      const ptrdiff_t stride = kMaxBlockSize * sizeof(Pixel);
      base_filter_intra_pred_(intra_pred_mem_.ref_src, stride, top, left,
                              static_cast<FilterIntraPredictor>(i),
                              block_width_, block_height_);
      cur_filter_intra_pred_(intra_pred_mem_.dst, stride, top, left,
                             static_cast<FilterIntraPredictor>(i), block_width_,
                             block_height_);
      if (!test_utils::CompareBlocks(
              intra_pred_mem_.dst, intra_pred_mem_.ref_src, block_width_,
              block_height_, kMaxBlockSize, kMaxBlockSize, true)) {
        ADD_FAILURE() << "Result from optimized version of "
                      << kFilterIntraPredNames[i]
                      << " differs from reference in iteration #" << n;
        break;
      }
    }
  }
}

//------------------------------------------------------------------------------
using FilterIntraPredTest8bpp = FilterIntraPredTest<8, uint8_t>;

const char* const* GetFilterIntraPredDigests8bpp(TransformSize tx_size) {
  static const char* const kDigests4x4[kNumFilterIntraPredictors] = {
      "a2486efcfb351d60a8941203073e89c6", "240716ae5ecaedc19edae1bdef49e05d",
      "dacf4af66a966aca7c75abe24cd9ba99", "311888773676f3c2ae3334c4e0f141e5",
      "2d3711616c8d8798f608e313cb07a72a",
  };
  static const char* const kDigests4x8[kNumFilterIntraPredictors] = {
      "1cb74ba1abc68d936e87c13511ed5fbf", "d64c2c08586a762dbdfa8e1150bede06",
      "73e9d1a9b6fa3e96fbd65c7dce507529", "e3ae17d9338e5aa3420d31d0e2d7ee87",
      "750dbfe3bc5508b7031957a1d315b8bc",
  };
  static const char* const kDigests4x16[kNumFilterIntraPredictors] = {
      "48a1060701bf68ec6342d6e24c10ef17", "0c91ff7988814d192ed95e840a87b4bf",
      "efe586b891c8828c4116c9fbf50850cc", "a3bfa10be2b155826f107e9256ac3ba1",
      "976273745b94a561fd52f5aa96fb280f",
  };
  static const char* const kDigests8x4[kNumFilterIntraPredictors] = {
      "73f82633aeb28db1d254d077edefd8a9", "8eee505cdb5828e33b67ff5572445dac",
      "9b0f101c28c66a916079fe5ed33b4021", "47fd44a7e5a5b55f067908192698e25c",
      "eab59a3710d9bdeca8fa03a15d3f95d6",
  };
  static const char* const kDigests8x8[kNumFilterIntraPredictors] = {
      "aa07b7a007c4c1d494ddb44a23c27bcd", "d27eee43f15dfcfe4c46cd46b681983b",
      "1015d26022cf57acfdb11fd3f6b9ccb0", "4f0e00ef556fbcac2fb31e3b18869070",
      "918c2553635763a0756b20154096bca6",
  };
  static const char* const kDigests8x16[kNumFilterIntraPredictors] = {
      "a8ac58b2efb02092035cca206dbf5fbe", "0b22b000b7f124b32545bc86dd9f0142",
      "cd6a08e023cad301c084b6ec2999da63", "c017f5f4fa5c05e7638ae4db98512b13",
      "893e6995522e23ed3d613ef3797ca580",
  };
  static const char* const kDigests8x32[kNumFilterIntraPredictors] = {
      "b3d5d4f09b778ae2b8cc0e9014c22320", "e473874a1e65228707489be9ca6477aa",
      "91bda5a2d32780af345bb3d49324732f", "20f2ff26f004f02e8e2be49e6cadc32f",
      "00c909b749e36142b133a7357271e83e",
  };
  static const char* const kDigests16x4[kNumFilterIntraPredictors] = {
      "ef252f074fc3f5367748436e676e78ca", "cd436d8803ea40db3a849e7c869855c7",
      "9cd8601b5d66e61fd002f8b11bfa58d9", "b982f17ee36ef0d1c2cfea20197d5666",
      "9e350d1cd65d520194281633f566810d",
  };
  static const char* const kDigests16x8[kNumFilterIntraPredictors] = {
      "9a7e0cf9b023a89ee619ee672ba2a219", "c20186bc642912ecd4d48bc4924a79b1",
      "77de044f4c7f717f947a36fc0aa17946", "3f2fc68f11e6ee0220adb8d1ee085c8e",
      "2f37e586769dfb88d9d4116b9c28c5ab",
  };
  static const char* const kDigests16x16[kNumFilterIntraPredictors] = {
      "36c5b85b9a6b1d2e8f44f09c81adfe9c", "78494ce3a6a78aa2879ad2e24d43a005",
      "aa30cd29a74407dbec80161745161eb2", "ae2a0975ef166e05e5e8c3701bd19e93",
      "6322fba6f3bcb1f6c8e78160d200809c",
  };
  static const char* const kDigests16x32[kNumFilterIntraPredictors] = {
      "82d54732c37424946bc73f5a78f64641", "071773c82869bb103c31e05f14ed3c2f",
      "3a0094c150bd6e21ce1f17243b21e76b", "998ffef26fc65333ae407bbe9d41a252",
      "6491add6b665aafc364c8c104a6a233d",
  };
  static const char* const kDigests32x8[kNumFilterIntraPredictors] = {
      "c60062105dd727e94f744c35f0d2156e", "36a9e4d543701c4c546016e35e9c4337",
      "05a8d07fe271023e63febfb44814d114", "0a28606925519d1ed067d64761619dc8",
      "bb8c34b143910ba49b01d13e94d936ac",
  };
  static const char* const kDigests32x16[kNumFilterIntraPredictors] = {
      "60e6caeec9194fcb409469e6e1393128", "5d764ead046443eb14f76822a569b056",
      "b1bf22fcc282614354166fa1eb6e5f8b", "4b188e729fe49ae24100b3ddd8f17313",
      "75f430fdea0b7b5b66866fd68a795a6a",
  };
  static const char* const kDigests32x32[kNumFilterIntraPredictors] = {
      "5bb91a37b1979866eb23b59dd352229d", "589aa983109500749609d7be1cb79711",
      "5e8fb1927cdbe21143494b56b5d400f6", "9e28f741d19c64b2a0577d83546d32d9",
      "73c73237a5d891096066b186abf96854",
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
    case kTransformSize32x8:
      return kDigests32x8;
    case kTransformSize32x16:
      return kDigests32x16;
    case kTransformSize32x32:
      return kDigests32x32;
    default:
      ADD_FAILURE() << "Unknown transform size: " << tx_size;
      return nullptr;
  }
}

TEST_P(FilterIntraPredTest8bpp, DISABLED_Speed) {
  const auto num_runs =
      static_cast<int>(2.5e8 / (block_width_ * block_height_));
  TestSpeed(GetFilterIntraPredDigests8bpp(tx_size_), num_runs);
}

TEST_P(FilterIntraPredTest8bpp, FixedInput) {
  TestSpeed(GetFilterIntraPredDigests8bpp(tx_size_), 1);
}

TEST_P(FilterIntraPredTest8bpp, Overflow) { TestSaturatedValues(); }
TEST_P(FilterIntraPredTest8bpp, Random) { TestRandomValues(); }

//------------------------------------------------------------------------------

#if LIBGAV1_MAX_BITDEPTH >= 10
using FilterIntraPredTest10bpp = FilterIntraPredTest<10, uint16_t>;

const char* const* GetFilterIntraPredDigests10bpp(TransformSize tx_size) {
  static const char* const kDigests4x4[kNumFilterIntraPredictors] = {
      "13a9014d9e255cde8e3e85abf6ef5151", "aee33aa3f3baec87a8c019743fff40f1",
      "fdd8ca2be424501f51fcdb603c2e757c", "aed00c082d1980d4bab45e9318b939f0",
      "1b363db246aa5400f49479b7d5d41799",
  };
  static const char* const kDigests4x8[kNumFilterIntraPredictors] = {
      "e718b9e31ba3da0392fd4b6cfba5d882", "31ba22989cdc3bb80749685f42c6c697",
      "6bc5b3a55b94018117569cfdced17bf9", "ec29979fb4936116493dfa1cfc93901c",
      "c6bcf564e63c42148d9917f089566432",
  };
  static const char* const kDigests4x16[kNumFilterIntraPredictors] = {
      "404bddd88dff2c0414b5398287e54f18", "ff4fb3039cec6c9ffed6d259cbbfd854",
      "7d6fa3ed9e728ff056a73c40bb6edeb6", "82845d942ad8048578e0037336905146",
      "f3c07ea65db08c639136a5a9270f95ff",
  };
  static const char* const kDigests8x4[kNumFilterIntraPredictors] = {
      "2008981638f27ba9123973a733e46c3d", "47efecf1f7628cbd8c22e168fcceb5ce",
      "04c857ffbd1edd6e2788b17410a4a39c", "deb0236c4277b4d7b174fba407e1c9d7",
      "5b58567f94ae9fa930f700c68c17399d",
  };
  static const char* const kDigests8x8[kNumFilterIntraPredictors] = {
      "d9bab44a6d1373e758bfa0ee88239093", "29b10ddb32d9de2ff0cad6126f010ff6",
      "1a03f9a18bdbab0811138cd969bf1f93", "e3273c24e77095ffa033a073f5bbcf7b",
      "5187bb3df943d154cb01fb2f244ff86f",
  };
  static const char* const kDigests8x16[kNumFilterIntraPredictors] = {
      "a2199f792634a56f1c4e88510e408773", "8fd8a98969d19832975ee7131cca9dbb",
      "d897380941f75b04b1327e63f136d7d6", "d36f52a157027d53b15b7c02a7983436",
      "0a8c23047b0364f5687b62b01f043359",
  };
  static const char* const kDigests8x32[kNumFilterIntraPredictors] = {
      "5b74ea8e4f60151cf2db9b23d803a2e2", "e0d6bb5fa7d181589c31fcf2755d7c0b",
      "42e590ffc88b8940b7aade22e13bbb6a", "e47c39ec1761aa7b5a9b1368ede7cfdc",
      "6e963a89beac6f3a362c269d1017f9a8",
  };
  static const char* const kDigests16x4[kNumFilterIntraPredictors] = {
      "9eaa079622b5dd95ad3a8feb68fa9bbb", "17e3aa6a0034e9eedcfc65b8ce6e7205",
      "eac5a5337dbaf9bcbc3d320745c8e190", "c6ba9a7e518be04f725bc1dbd399c204",
      "19020b82ce8bb49a511820c7e1d58e99",
  };
  static const char* const kDigests16x8[kNumFilterIntraPredictors] = {
      "2d2c3255d5dfc1479a5d82a7d5a0d42e", "0fbb4ee851b4ee58c6d30dd820d19e38",
      "fa77a1b056e8dc8efb702c7832531b32", "186269ca219dc663ad9b4a53e011a54b",
      "c12180a6dcde0c3579befbb5304ff70b",
  };
  static const char* const kDigests16x16[kNumFilterIntraPredictors] = {
      "dbb81d7ee7d3c83c271400d0160b2e83", "4da656a3ef238d90bb8339471a6fdb7e",
      "d95006bf299b84a1b04e38d5fa8fb4f7", "742a03331f0fbd66c57df0ae31104aca",
      "4d20aa440e38b6b7ac83c8c54d313169",
  };
  static const char* const kDigests16x32[kNumFilterIntraPredictors] = {
      "6247730c93789cc25bcb837781dfa05b", "9a93e14b06dd145e35ab21a0353bdebe",
      "6c5866353e30296a67d9bd7a65d6998d", "389d7f038d7997871745bb1305156ff9",
      "e7640d81f891e1d06e7da75c6ae74d93",
  };
  static const char* const kDigests32x8[kNumFilterIntraPredictors] = {
      "68f3a603b7c25dd78deffe91aef22834", "48c735e4aa951d6333d99e571bfeadc8",
      "35239df0993a429fc599a3037c731e4b", "ba7dd72e04af1a1fc1b30784c11df783",
      "78e9017f7434665d32ec59795aed0012",
  };
  static const char* const kDigests32x16[kNumFilterIntraPredictors] = {
      "8cf2f11f7f77901cb0c522ad191eb998", "204c76d68c5117b89b5c3a05d5548883",
      "f3751e41e7a595f43d8aaf9a40644e05", "81ea1a7d608d7b91dd3ede0f87e750ee",
      "b5951334dfbe6229d828e03cd2d98538",
  };
  static const char* const kDigests32x32[kNumFilterIntraPredictors] = {
      "9d8630188c3d1a4f28a6106e343c9380", "c6c92e059faa17163522409b7bf93230",
      "62e4c959cb06ec661d98769981fbd555", "01e61673f11011571246668e36cc61c5",
      "4530222ea1de546e202630fcf43f4526",
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
    case kTransformSize32x8:
      return kDigests32x8;
    case kTransformSize32x16:
      return kDigests32x16;
    case kTransformSize32x32:
      return kDigests32x32;
    default:
      ADD_FAILURE() << "Unknown transform size: " << tx_size;
      return nullptr;
  }
}

TEST_P(FilterIntraPredTest10bpp, DISABLED_Speed) {
  const auto num_runs =
      static_cast<int>(2.5e8 / (block_width_ * block_height_));
  TestSpeed(GetFilterIntraPredDigests10bpp(tx_size_), num_runs);
}

TEST_P(FilterIntraPredTest10bpp, FixedInput) {
  TestSpeed(GetFilterIntraPredDigests10bpp(tx_size_), 1);
}

TEST_P(FilterIntraPredTest10bpp, Overflow) { TestSaturatedValues(); }
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

//------------------------------------------------------------------------------

#if LIBGAV1_MAX_BITDEPTH == 12
using FilterIntraPredTest12bpp = FilterIntraPredTest<12, uint16_t>;

const char* const* GetFilterIntraPredDigests12bpp(TransformSize tx_size) {
  static const char* const kDigests4x4[kNumFilterIntraPredictors] = {
      "27682e2763f742e0c7156a263af54fe1", "f6fe9b73d8a2024b3125d25a42028be3",
      "8a232b8caa41f8c4f0b547f0aa072fd7", "411b24dc872e91de3a607f18b51c4e34",
      "9a106b70ca2df5317afc90aba0316a98",
  };
  static const char* const kDigests4x8[kNumFilterIntraPredictors] = {
      "a0d3f3a8f498727af0844a6df90da971", "bb02998e3d5d7b4643db616a5ce75c51",
      "eaa39425427c155dea1836c37fc14f7e", "747cc4fa0c9e3418f4a15ded9f846599",
      "c1a2aeaa01dd3edac4c26f74e01d8d57",
  };
  static const char* const kDigests4x16[kNumFilterIntraPredictors] = {
      "80c01fdef14e3db28987e323801c998e", "de5a2f59384a096324eebe843d4b8ba5",
      "f85e18efc9297793392607cdd84d8bc4", "d84bf2d9d4996c2f7fd82b6bbd52577b",
      "9d73771de09c17bd494f1f5f75ab1111",
  };
  static const char* const kDigests8x4[kNumFilterIntraPredictors] = {
      "7df2b038c4d816eb4949de6b933f0632", "0f1c45dd6e8d5534de0c9a279087ea8b",
      "1b79f3b10facd9ffc404cbafdd73aa43", "e19adec4f14d72c5157f9faf7fc9b23e",
      "a30ed988ea6ed797d4bf0945ffe7e330",
  };
  static const char* const kDigests8x8[kNumFilterIntraPredictors] = {
      "097a0c14d89ece69e779fa755a2b75c0", "ebadfc559b20246dcd8d74413ff4d088",
      "097c91bedc1e703b3eb54361d94df59a", "765bbad37b91e644292beac5f06811be",
      "f3c809461fa3325f0d33087ca79c47d0",
  };
  static const char* const kDigests8x16[kNumFilterIntraPredictors] = {
      "36464af48b38005b61f7f528a0b0c8ba", "47fa0868224c71d28d3cdcf247282c13",
      "ca34bb57a37ee3e5428814ec63f52117", "420bdca6b643f4421d465345cc264167",
      "339c124c07a611a65952dc9996ba6e12",
  };
  static const char* const kDigests8x32[kNumFilterIntraPredictors] = {
      "99ca0d3b3fbdd4661a2c07bdb2752a70", "6fedae1dbfe721210b65e08dc77847dd",
      "956810089f81dc9334103111afec2fbb", "ede4f0bee06def6d8a2037939415d845",
      "ca146dfe0edbdac3066a0ca387fb6277",
  };
  static const char* const kDigests16x4[kNumFilterIntraPredictors] = {
      "b0f7d5dbf7f9aa3f0ab13273de80dc9d", "a3537f2b60426e9f83aeef973161fcfd",
      "d4f868f793ab232bee17b49afcfc28a0", "fc43429761d10723b5f377eb6513e59a",
      "f59aabb06574ce24e1d1113753edb098",
  };
  static const char* const kDigests16x8[kNumFilterIntraPredictors] = {
      "0b539f1e2ecf0300bf3838ab1d80952c", "44f01a4324cda8d27ea44a8bd3620526",
      "a57819a22b422e7da9d85f09504a2c57", "dbff6a417a8f3606575acb3c98efe091",
      "534e8e8cd4b73cb4f6ec22f903727efa",
  };
  static const char* const kDigests16x16[kNumFilterIntraPredictors] = {
      "247192bd6a5c2821b8694e4669361103", "1935044a6220ac6315a58b402465b6da",
      "bdce29a3e988b804d429da1446a34c2a", "4697132c20395fabac2662cb8b1ce35a",
      "3d07a7beaff6925175fcd9a8e69542e6",
  };
  static const char* const kDigests16x32[kNumFilterIntraPredictors] = {
      "3429b83b7ba723bdd2e3e368979b51b0", "cd099d0eb7f4a20547f91d9402e3394a",
      "a6a7cc4e0f8ed34424264107b3657fb8", "0125ace62bec7c7ff7240bf5b6f689c5",
      "a0722dba921b078a6d569ecb81777bf8",
  };
  static const char* const kDigests32x8[kNumFilterIntraPredictors] = {
      "44b1b086ee37a93406e5db95dca825d7", "fdeed5c4644dc288f6dcc148e8d2867a",
      "b241d112f6fa7a24c44706fb76e49132", "a782dcf01a16231276dbd20121bad640",
      "4da9c0efd0bcb31f911af52779317fb9",
  };
  static const char* const kDigests32x16[kNumFilterIntraPredictors] = {
      "bf9704995a0a868c45280cac3415c0a7", "373626072ade7c8d709ab732149fd3ae",
      "9e4a2062aa86ac8dc5164002c953c7ca", "62eede30996d0e55afcf513fe9ad3c58",
      "a5f3bb32688d5189341304d12e4e6449",
  };
  static const char* const kDigests32x32[kNumFilterIntraPredictors] = {
      "bd93c4ddbe0f06e3f12be25ce490f68c", "bfe772b203b83c982f35a8ed0682cd16",
      "d357ae05ce215f4c5af650ae82909081", "bd640d3c511edaac1753b64c81afb75d",
      "4d05d67e02a7c4af7ae981b0eb8a4d7b",
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
    case kTransformSize32x8:
      return kDigests32x8;
    case kTransformSize32x16:
      return kDigests32x16;
    case kTransformSize32x32:
      return kDigests32x32;
    default:
      ADD_FAILURE() << "Unknown transform size: " << tx_size;
      return nullptr;
  }
}

TEST_P(FilterIntraPredTest12bpp, DISABLED_Speed) {
  const auto num_runs =
      static_cast<int>(2.5e8 / (block_width_ * block_height_));
  TestSpeed(GetFilterIntraPredDigests12bpp(tx_size_), num_runs);
}

TEST_P(FilterIntraPredTest12bpp, FixedInput) {
  TestSpeed(GetFilterIntraPredDigests12bpp(tx_size_), 1);
}

TEST_P(FilterIntraPredTest12bpp, Overflow) { TestSaturatedValues(); }
#endif  // LIBGAV1_MAX_BITDEPTH == 12

// Filter-intra and Cfl predictors are available only for transform sizes
// with max(width, height) <= 32.
constexpr TransformSize kTransformSizesSmallerThan32x32[] = {
    kTransformSize4x4,   kTransformSize4x8,   kTransformSize4x16,
    kTransformSize8x4,   kTransformSize8x8,   kTransformSize8x16,
    kTransformSize8x32,  kTransformSize16x4,  kTransformSize16x8,
    kTransformSize16x16, kTransformSize16x32, kTransformSize32x8,
    kTransformSize32x16, kTransformSize32x32};

INSTANTIATE_TEST_SUITE_P(C, FilterIntraPredTest8bpp,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, FilterIntraPredTest8bpp,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
#endif  // LIBGAV1_ENABLE_SSE4_1
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, FilterIntraPredTest8bpp,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
#endif  // LIBGAV1_ENABLE_NEON

#if LIBGAV1_MAX_BITDEPTH >= 10
INSTANTIATE_TEST_SUITE_P(C, FilterIntraPredTest10bpp,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, FilterIntraPredTest10bpp,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
#endif  // LIBGAV1_ENABLE_NEON
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
INSTANTIATE_TEST_SUITE_P(C, FilterIntraPredTest12bpp,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace
}  // namespace dsp

static std::ostream& operator<<(std::ostream& os, const TransformSize tx_size) {
  return os << ToString(tx_size);
}

}  // namespace libgav1
