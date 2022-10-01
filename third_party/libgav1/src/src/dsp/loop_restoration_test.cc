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

#include "src/dsp/loop_restoration.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>

#include "absl/strings/match.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "gtest/gtest.h"
#include "src/dsp/common.h"
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

// in unit of Pixel.
constexpr int kBorder = 16;
constexpr int kWidth = 256;
constexpr int kHeight = 255;
constexpr int kStride = kWidth + 2 * kBorder;
constexpr int kOffset = kBorder * kStride + kBorder;
constexpr int kMaxBlockSize = 288 * kStride;
constexpr int kUnitWidths[] = {32, 64, 128, 256};

constexpr int kNumRadiusTypes = 3;
constexpr int kNumWienerOrders = 4;
constexpr int kWienerOrders[] = {7, 5, 3, 1};
constexpr int kWienerOrderIdLookup[] = {0, 3, 0, 2, 0, 1, 0, 0};

template <int bitdepth, typename Pixel>
class SelfGuidedFilterTest : public testing::TestWithParam<int>,
                             public test_utils::MaxAlignedAllocable {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  SelfGuidedFilterTest() = default;
  SelfGuidedFilterTest(const SelfGuidedFilterTest&) = delete;
  SelfGuidedFilterTest& operator=(const SelfGuidedFilterTest&) = delete;
  ~SelfGuidedFilterTest() override = default;

  void SetUp() override {
    test_utils::ResetDspTable(bitdepth);
    LoopRestorationInit_C();
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const char* const test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
    } else if (absl::StartsWith(test_case, "AVX2/")) {
      if ((GetCpuInfo() & kAVX2) != 0) {
        LoopRestorationInit_AVX2();
#if LIBGAV1_MAX_BITDEPTH >= 10
        LoopRestorationInit10bpp_AVX2();
#endif
      }
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      if ((GetCpuInfo() & kSSE4_1) != 0) {
        LoopRestorationInit_SSE4_1();
#if LIBGAV1_MAX_BITDEPTH >= 10
        LoopRestorationInit10bpp_SSE4_1();
#endif
      }
    } else if (absl::StartsWith(test_case, "NEON/")) {
      LoopRestorationInit_NEON();
#if LIBGAV1_MAX_BITDEPTH >= 10
      LoopRestorationInit10bpp_NEON();
#endif
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }
    const Dsp* const dsp = GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);
    target_self_guided_filter_func_ = dsp->loop_restorations[1];
    restoration_info_.type = kLoopRestorationTypeSgrProj;
    memset(dst_, 0, sizeof(dst_));
  }

  void SetInputData(int type, Pixel value, int radius_index,
                    libvpx_test::ACMRandom* rnd);
  void TestFixedValues(int test_index, Pixel value);
  void TestRandomValues(bool speed);

 protected:
  const int unit_width_ = GetParam();
  const int unit_height_ = kRestorationUnitHeight;

 private:
  alignas(kMaxAlignment) Pixel src_[kMaxBlockSize];
  alignas(kMaxAlignment) Pixel dst_[kMaxBlockSize];
  RestorationUnitInfo restoration_info_;
  RestorationBuffer restoration_buffer_;
  LoopRestorationFunc target_self_guided_filter_func_;
};

template <int bitdepth, typename Pixel>
void SelfGuidedFilterTest<bitdepth, Pixel>::SetInputData(
    int type, Pixel value, int radius_index,
    libvpx_test::ACMRandom* const rnd) {
  const int mask = (1 << bitdepth) - 1;
  if (type == 0) {  // Set fixed values
    for (auto& s : src_) s = value;
  } else {  // Set random values
    for (auto& s : src_) s = rnd->Rand16() & mask;
  }
  for (auto& d : dst_) d = rnd->Rand16() & mask;
  restoration_info_.sgr_proj_info.multiplier[0] =
      kSgrProjMultiplierMin[0] +
      rnd->PseudoUniform(kSgrProjMultiplierMax[0] - kSgrProjMultiplierMin[0] +
                         1);
  restoration_info_.sgr_proj_info.multiplier[1] =
      kSgrProjMultiplierMin[1] +
      rnd->PseudoUniform(kSgrProjMultiplierMax[1] - kSgrProjMultiplierMin[1] +
                         1);
  // regulate multiplier so that it matches libaom.
  // Valid self-guided filter doesn't allow r0 and r1 to be 0 at the same time.
  // When r0 or r1 is zero, its corresponding multiplier is set to zero in
  // libaom.
  int index;
  if (radius_index == 0) {
    index = 0;  // r0 = 2, r1 = 1
  } else if (radius_index == 1) {
    index = 10;  // r0 = 0, r1 = 1
  } else /* if (radius_index == 2) */ {
    index = 14;  // r0 = 2, r1 = 0
  }
  const uint8_t r0 = kSgrProjParams[index][0];
  const uint8_t r1 = kSgrProjParams[index][2];
  static constexpr int kMultiplier[2] = {0, 95};
  restoration_info_.sgr_proj_info.index = index;
  if (r0 == 0) {
    restoration_info_.sgr_proj_info.multiplier[0] = kMultiplier[0];
  } else if (r1 == 0) {
    restoration_info_.sgr_proj_info.multiplier[1] = kMultiplier[1];
  }
}

template <int bitdepth, typename Pixel>
void SelfGuidedFilterTest<bitdepth, Pixel>::TestFixedValues(int test_index,
                                                            Pixel value) {
  static const char* const kDigest[][3][kNumRadiusTypes] = {
      {{"7b78783ff4f03625a50c2ebfd574adca", "4faa0810639016f11a9f761ce28c38b0",
        "a03314fc210bee68c7adbb44d2bbdac7"},
       {"fce031d1339cfef5016e76a643538a71", "d439e1060de3f07b5b29c9b0b7c08e54",
        "a6583fe9359877f4a259c81d900fc4fb"},
       {"8f9b6944c8965f34d444a667da3b0ebe", "84fa62c491c67c3a435fd5140e7a4f82",
        "d04b62d97228789e5c6928d40d5d900e"}},
      {{"948ea16a90c4cefef87ce5b0ee105fc6", "76740629877b721432b84dbbdb4e352a",
        "27100f37b3e42a5f2a051e1566edb6f8"},
       {"dd320de3bc82f4ba69738b2190ea9f85", "bf82f271e30a1aca91e53b086e133fb3",
        "69c274ac59c99999e1bfbf2fc4586ebd"},
       {"86ff2318bf8a584b8d5edd710681d621", "f6e1c104a764d6766cc278d5b216855a",
        "6d928703526ab114efba865ff5b11886"}},
      {{"9fbf1b246011250f38532a543cc6dd74", "d5c1e0142390ebb51b075c49f8ee9ff4",
        "92f31086ba2f9e1508983b22d93a4e5c"},
       {"2198321e6b95e7199738e60f5ddc6966", "34f74626027ffca010c824ddf0942b13",
        "43dd7df2c2a601262c68cd8af1c61b82"},
       {"1ab6138c3a82ac8ccd840f0553fdfb58", "be3bf92633f7165d3ad9c327d2dd53fe",
        "41115efff3adeb541e04db23faa22f23"}},
      {{"42364ff8dbdbd6706fa3b8855a4258be", "a7843fdfd4d3c0d80ba812b353b4d6b4",
        "f8a6a025827f29f857bed3e28ba3ea33"},
       {"b83c1f8d7712e37f9b21b033822e37ed", "589daf2e3e6f8715873920515cfc1b42",
        "20dcbe8e317a4373bebf11d56adc5f02"},
       {"7971a60337fcdb662c92db051bd0bb41", "75f89f346c2a37bf0c6695c0482531e6",
        "1595eeacd62cdce4d2fb094534c22c1e"}}};
  if (target_self_guided_filter_func_ == nullptr) return;
  ASSERT_LT(value, 1 << bitdepth);
  constexpr int bd_index = (bitdepth - 8) / 2;
  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
  const Pixel* const src = src_ + kOffset;
  Pixel* const dst = dst_ + kOffset;
  for (int radius_index = 0; radius_index < kNumRadiusTypes; ++radius_index) {
    SetInputData(0, value, radius_index, &rnd);
    const absl::Time start = absl::Now();
    for (int y = 0; y < kHeight; y += unit_height_) {
      const int height = std::min(unit_height_, kHeight - y);
      for (int x = 0; x < kWidth; x += unit_width_) {
        const int width = std::min(unit_width_, kWidth - x);
        const Pixel* const source = src + y * kStride + x;
        target_self_guided_filter_func_(
            restoration_info_, source, kStride,
            source - kRestorationVerticalBorder * kStride, kStride,
            source + height * kStride, kStride, width, height,
            &restoration_buffer_, dst + y * kStride + x);
      }
    }
    const absl::Duration elapsed_time = absl::Now() - start;
    test_utils::CheckMd5Digest(
        "kLoopRestorationTypeSgrProj", std::to_string(GetParam()).c_str(),
        kDigest[test_index][bd_index][radius_index], dst_ + kBorder * kStride,
        kHeight * kStride * sizeof(*dst_), elapsed_time);
  }
}

template <int bitdepth, typename Pixel>
void SelfGuidedFilterTest<bitdepth, Pixel>::TestRandomValues(bool speed) {
  static const char* const kDigest[][3][kNumRadiusTypes] = {
      {{"9f8358ed820943fa0abe3a8ebb5887db", "fb5d48870165522341843bcbfa8674fb",
        "ca67159cd29475ac5d52ca4a0df3ea10"},
       {"a78641886ea0cf8757057d1d91e01434", "1b95172a5f2f9c514c78afa4cf8e5678",
        "a8ba988283d9e1ad1f0dcdbf6bbdaade"},
       {"d95e98d031f9ba290e5183777d1e4905", "f806853cfadb50e6dbd4898412b92934",
        "741fbfdb79cda695afedda3d51dbb27f"}},
      {{"f219b445e5c80ffb5dd0359cc2cb4dd4", "699b2c9ddca1cbb0d4fc24cbcbe951e9",
        "a4005899fa8d3c3c4669910f93ff1290"},
       {"10a75cab3c78b891c8c6d92d55f685d1", "d46f158f57c628136f6f298ee8ca6e0e",
        "07203ad761775d5d317f2b7884afd9fe"},
       {"76b9ef906090fa81af64cce3bba0a54a", "8eecc59acdef8953aa9a96648c0ccd2c",
        "6e45a0ef60e0475f470dc93552047f07"}},
      {{"000d4e382be4003b514c9135893d0a37", "8fb082dca975be363bfc9c2d317ae084",
        "475bcb6a58f87da7723f6227bc2aca0e"},
       {"4d589683f69ccc5b416149dcc5c835d5", "986b6832df1f6020d50be61ae121e42f",
        "7cb5c5dbdb3d1c54cfa00def450842dc"},
       {"0e3dc23150d18c9d366d15e174727311", "8495122917770d822f1842ceff987b03",
        "4aeb9db902072cefd6af0aff8aaabd24"}},
      {{"fd43bfe34d63614554dd29fb24b12173", "5c1ba74ba3062c769d5c3c86a85ac9b9",
        "f1eda6d15b37172199d9949c2315832f"},
       {"a11be3117fb77e8fe113581b06f98bd1", "df94d12b774ad5cf744c871e707c36c8",
        "b23dc0b54c3500248d53377030428a61"},
       {"9c331f2b9410354685fe904f6c022dfa", "b540b0045b7723fbe962fd675db4b077",
        "3cecd1158126c9c9cc2873ecc8c1a135"}},
      {{"f3079b3b21d8dc6fce7bb1fd104be359", "c6fcbc686cfb97ab3a64f445d73aad36",
        "23966cba3e0e7803eeb951905861e0dd"},
       {"7210391a6fe26e5ca5ea205bc38aa035", "4c3e6eccad3ea152d320ecd1077169de",
        "dcee48f94126a2132963e86e93dd4903"},
       {"beb3dd8a2dbc5f83ef171b0ffcead3ab", "c373bd9c46bdb89a3d1e41759c315025",
        "cd407b212ab46fd4a451d5dc93a0ce4a"}}};
  if (target_self_guided_filter_func_ == nullptr) return;
  constexpr int bd_index = (bitdepth - 8) / 2;
  const int num_inputs = speed ? 1 : 5;
#if LIBGAV1_ENABLE_NEON
  const int num_tests = speed ? 4000 : 1;
#else
  const int num_tests = speed ? 10000 : 1;
#endif
  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
  const Pixel* const src = src_ + kOffset;
  Pixel* const dst = dst_ + kOffset;
  for (int i = 0; i < num_inputs; ++i) {
    for (int radius_index = 0; radius_index < kNumRadiusTypes; ++radius_index) {
      SetInputData(1, 0, radius_index, &rnd);
      const absl::Time start = absl::Now();
      for (int k = 0; k < num_tests; ++k) {
        for (int y = 0; y < kHeight; y += unit_height_) {
          const int height = std::min(unit_height_, kHeight - y);
          for (int x = 0; x < kWidth; x += unit_width_) {
            const int width = std::min(unit_width_, kWidth - x);
            const Pixel* const source = src + y * kStride + x;
            target_self_guided_filter_func_(
                restoration_info_, source, kStride,
                source - kRestorationVerticalBorder * kStride, kStride,
                source + height * kStride, kStride, width, height,
                &restoration_buffer_, dst + y * kStride + x);
          }
        }
      }
      const absl::Duration elapsed_time = absl::Now() - start;
      test_utils::CheckMd5Digest(
          "kLoopRestorationTypeSgrProj", std::to_string(GetParam()).c_str(),
          kDigest[i][bd_index][radius_index], dst_ + kBorder * kStride,
          kHeight * kStride * sizeof(*dst_), elapsed_time);
    }
  }
}

using SelfGuidedFilterTest8bpp = SelfGuidedFilterTest<8, uint8_t>;

TEST_P(SelfGuidedFilterTest8bpp, Correctness) {
  TestFixedValues(0, 0);
  TestFixedValues(1, 1);
  TestFixedValues(2, 128);
  TestFixedValues(3, 255);
  TestRandomValues(false);
}

TEST_P(SelfGuidedFilterTest8bpp, DISABLED_Speed) { TestRandomValues(true); }

INSTANTIATE_TEST_SUITE_P(C, SelfGuidedFilterTest8bpp,
                         testing::ValuesIn(kUnitWidths));
#if LIBGAV1_ENABLE_AVX2
INSTANTIATE_TEST_SUITE_P(AVX2, SelfGuidedFilterTest8bpp,
                         testing::ValuesIn(kUnitWidths));
#endif
#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, SelfGuidedFilterTest8bpp,
                         testing::ValuesIn(kUnitWidths));
#endif
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, SelfGuidedFilterTest8bpp,
                         testing::ValuesIn(kUnitWidths));
#endif

#if LIBGAV1_MAX_BITDEPTH >= 10
using SelfGuidedFilterTest10bpp = SelfGuidedFilterTest<10, uint16_t>;

TEST_P(SelfGuidedFilterTest10bpp, Correctness) {
  TestFixedValues(0, 0);
  TestFixedValues(1, 1);
  TestFixedValues(2, 512);
  TestFixedValues(3, 1023);
  TestRandomValues(false);
}

TEST_P(SelfGuidedFilterTest10bpp, DISABLED_Speed) { TestRandomValues(true); }

INSTANTIATE_TEST_SUITE_P(C, SelfGuidedFilterTest10bpp,
                         testing::ValuesIn(kUnitWidths));

#if LIBGAV1_ENABLE_AVX2
INSTANTIATE_TEST_SUITE_P(AVX2, SelfGuidedFilterTest10bpp,
                         testing::ValuesIn(kUnitWidths));
#endif
#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, SelfGuidedFilterTest10bpp,
                         testing::ValuesIn(kUnitWidths));
#endif
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, SelfGuidedFilterTest10bpp,
                         testing::ValuesIn(kUnitWidths));
#endif

#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using SelfGuidedFilterTest12bpp = SelfGuidedFilterTest<12, uint16_t>;

TEST_P(SelfGuidedFilterTest12bpp, Correctness) {
  TestFixedValues(0, 0);
  TestFixedValues(1, 1);
  TestFixedValues(2, 2048);
  TestFixedValues(3, 4095);
  TestRandomValues(false);
}

TEST_P(SelfGuidedFilterTest12bpp, DISABLED_Speed) { TestRandomValues(true); }

INSTANTIATE_TEST_SUITE_P(C, SelfGuidedFilterTest12bpp,
                         testing::ValuesIn(kUnitWidths));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

template <int bitdepth, typename Pixel>
class WienerFilterTest : public testing::TestWithParam<int>,
                         public test_utils::MaxAlignedAllocable {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  WienerFilterTest() = default;
  WienerFilterTest(const WienerFilterTest&) = delete;
  WienerFilterTest& operator=(const WienerFilterTest&) = delete;
  ~WienerFilterTest() override = default;

  void SetUp() override {
    test_utils::ResetDspTable(bitdepth);
    LoopRestorationInit_C();
    const Dsp* const dsp = GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);
    base_wiener_filter_func_ = dsp->loop_restorations[0];
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const char* const test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
    } else if (absl::StartsWith(test_case, "AVX2/")) {
      if ((GetCpuInfo() & kAVX2) != 0) {
        LoopRestorationInit_AVX2();
#if LIBGAV1_MAX_BITDEPTH >= 10
        LoopRestorationInit10bpp_AVX2();
#endif
      }
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      if ((GetCpuInfo() & kSSE4_1) != 0) {
        LoopRestorationInit_SSE4_1();
#if LIBGAV1_MAX_BITDEPTH >= 10
        LoopRestorationInit10bpp_SSE4_1();
#endif
      }
    } else if (absl::StartsWith(test_case, "NEON/")) {
      LoopRestorationInit_NEON();
#if LIBGAV1_MAX_BITDEPTH >= 10
      LoopRestorationInit10bpp_NEON();
#endif
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }
    target_wiener_filter_func_ = dsp->loop_restorations[0];
    restoration_info_.type = kLoopRestorationTypeWiener;
    memset(dst_, 0, sizeof(dst_));
    memset(tmp_, 0, sizeof(tmp_));
    memset(buffer_, 0, sizeof(buffer_));
  }

  static void CleanFilterByOrder(const int order,
                                 int16_t filter[kWienerFilterTaps]) {
    if (order <= 5) filter[0] = 0;
    if (order <= 3) filter[1] = 0;
    if (order <= 1) filter[2] = 0;
  }

  void SetInputData(int type, Pixel value, int vertical_order,
                    int horizontal_order);
  void TestFixedValues(int digest_id, Pixel value);
  void TestRandomValues(bool speed);
  void TestCompare2C();

 protected:
  const int unit_width_ = GetParam();
  const int unit_height_ = kRestorationUnitHeight;

 private:
  alignas(kMaxAlignment)
      uint16_t buffer_[(kRestorationUnitWidth + kWienerFilterTaps - 1) *
                       kRestorationUnitHeight];
  alignas(kMaxAlignment) Pixel src_[kMaxBlockSize];
  alignas(kMaxAlignment) Pixel dst_[kMaxBlockSize];
  alignas(kMaxAlignment) Pixel tmp_[kMaxBlockSize];
  RestorationUnitInfo restoration_info_;
  RestorationBuffer restoration_buffer_;
  LoopRestorationFunc base_wiener_filter_func_;
  LoopRestorationFunc target_wiener_filter_func_;
};

template <int bitdepth, typename Pixel>
void WienerFilterTest<bitdepth, Pixel>::SetInputData(
    int type, Pixel value, const int vertical_order,
    const int horizontal_order) {
  const int mask = (1 << bitdepth) - 1;
  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
  if (type == 0) {
    for (auto& s : src_) s = value;
  } else {
    for (auto& s : src_) s = rnd.Rand16() & mask;
  }
  int order = vertical_order;
  for (int i = WienerInfo::kVertical; i <= WienerInfo::kHorizontal; ++i) {
    auto& filter = restoration_info_.wiener_info.filter[i];
    filter[3] = 128;
    for (int j = 0; j < 3; ++j) {
      filter[j] = kWienerTapsMin[j] +
                  rnd.PseudoUniform(kWienerTapsMax[j] - kWienerTapsMin[j] + 1);
    }
    CleanFilterByOrder(order, filter);
    filter[3] -= 2 * (filter[0] + filter[1] + filter[2]);
    restoration_info_.wiener_info.number_leading_zero_coefficients[i] =
        (kWienerFilterTaps - order) / 2;
    order = horizontal_order;
  }
}

template <int bitdepth, typename Pixel>
void WienerFilterTest<bitdepth, Pixel>::TestFixedValues(int digest_id,
                                                        Pixel value) {
  static const char* const kDigest[3][4] = {
      {"74fc90760a14b13340cb718f200ba350", "5bacaca0128cd36f4805330b3787771d",
       "1109e17545cc4fbd5810b8b77e19fc36", "e7f914ec9d065aba92338016e17a526c"},
      {"c8cc38790ceb0bea1eb989686755e1e5", "70f573b7e8875262c638a68d2f317916",
       "193b19065899c835cb513149eb36d135", "f1dff65e3e53558b303ef0a2e3f3ba98"},
      {"c8cc38790ceb0bea1eb989686755e1e5", "70f573b7e8875262c638a68d2f317916",
       "961eeb92bd9d85eb47e3961ee93d279a", "039a279232bc90eebc0ec2fe3e18a7e1"},
  };
  if (target_wiener_filter_func_ == nullptr) return;
  ASSERT_LT(value, 1 << bitdepth);
  constexpr int bd_index = (bitdepth - 8) / 2;
  const Pixel* const src = src_ + kOffset;
  Pixel* const dst = dst_ + kOffset;
  for (const auto vertical_order : kWienerOrders) {
    for (const auto horizontal_order : kWienerOrders) {
      SetInputData(0, value, vertical_order, horizontal_order);
      memset(dst_, 0, sizeof(dst_));
      const absl::Time start = absl::Now();
      for (int y = 0; y < kHeight; y += unit_height_) {
        const int height = std::min(unit_height_, kHeight - y);
        for (int x = 0; x < kWidth; x += unit_width_) {
          const int width = std::min(unit_width_, kWidth - x);
          const Pixel* const source = src + y * kStride + x;
          target_wiener_filter_func_(
              restoration_info_, source, kStride,
              source - kRestorationVerticalBorder * kStride, kStride,
              source + height * kStride, kStride, width, height,
              &restoration_buffer_, dst + y * kStride + x);
        }
      }
      const absl::Duration elapsed_time = absl::Now() - start;
      test_utils::CheckMd5Digest(
          "kLoopRestorationTypeWiener", std::to_string(GetParam()).c_str(),
          kDigest[bd_index][digest_id], dst_, sizeof(dst_), elapsed_time);
    }
  }
}

template <int bitdepth, typename Pixel>
void WienerFilterTest<bitdepth, Pixel>::TestRandomValues(bool speed) {
  static const char* const kDigest[3][kNumWienerOrders][kNumWienerOrders] = {
      {{"40d0cf56d2ffb4f581e68b0fc97f547f", "5c04745209b684ba98004ebb0f64e70b",
        "545ed7d3f7e7ca3b86b4ada31f7aaee7", "0d6b2967f1bd1d99b720e563fe0cf03f"},
       {"44b37076f0cf27f6eb506aca50c1d3e4", "e927d64dc9249e05a65e10ee75baa7d9",
        "6136ecb4e29b17c9566504148943fd47", "c5ee2da81d44dc8cb2ac8021f724eb7a"},
       {"125cbb227313ec91a2683f26e6f049d1", "77671b6529c806d23b749f304b548f59",
        "28d53a1b486881895b8f73fa64486df1", "f5e32165bafe575d7ee7a6fbae75f36d"},
       {"e832c41f2566ab542b32abba9d4f27bd", "ab1336ee6b85cba651f35ee5d3b3cc5c",
        "52a673b6d14fbdca5ebdb1a34ee3326f",
        "ebb42c7c9111f2e39f21e2158e801d9e"}},
      {{"8cd9c6bd9983bd49564a58ed4af9098a", "f71f333c9d71237ed4e46f0ef2283196",
        "375b43abc1d6682d62f91c1841b8b0fc", "71e2444822ae9c697ddfc96e07c6e8a1"},
       {"d9ed3a66ceef405c08c87f6e91b71059", "c171fcff5fb7bb919f13ead7a4917a4c",
        "8fbd1edb82fcd78d4d286886f65a700a", "fe14a143e6b261c5bb07b179d40be5a2"},
       {"1c995f4e7f117857de73211b81093bd0", "5ab1ee3bb14adcd66d66802d58bee068",
        "d77430783e173ebd1b30e5d9336c8b69", "e159a3620747458dff7ed3d20da1a4b7"},
       {"5346fa07d195c257548a332753b057a3", "c77674bc0a638abc4d38d58e494fc7cf",
        "7cbc1562a9dd08e1973b3b9ac1afc765",
        "3c91bf1a34672cd40bf261c5820d3ec3"}},
      {{"501b57370c781372b514accd03d161af", "a4569b5eff7f7e8b696934d192619be5",
        "24eb2aa43118a8822f7a6a7384ab9ea7", "edd7ac227733b5a4496bfdbdf4eb34d7"},
       {"77624cf73299a1bd928eae3eb8945dbe", "b3f311cacbf45fa892761462d31b2598",
        "977c063d93a4b95cb365363763faa4da", "02313c9d360a1e0180ed05d3e4444c3d"},
       {"f499655ecdcbe0ac48553f1eee758589", "a009c83c03e47cbd05c1243e28579bd9",
        "d5f0b4fd761ff51efce949e6c5ec4833", "e3a9a57aacd2e6cfe0f792a885b3e0e3"},
       {"b4cf906e9bb02ffca15c1e9575962ca2", "d0ca9f933978c0c31175ba1b28a44ae8",
        "81ac1475530ffbd1c8d3ce7da87ffe6b",
        "b96412949c2e31b29388222ac8914fa2"}},
  };
  if (target_wiener_filter_func_ == nullptr) return;
  constexpr int bd_index = (bitdepth - 8) / 2;
#if LIBGAV1_ENABLE_NEON
  const int num_tests = speed ? 5000 : 1;
#else
  const int num_tests = speed ? 10000 : 1;
#endif
  const Pixel* const src = src_ + kOffset;
  Pixel* const dst = dst_ + kOffset;
  for (const auto vertical_order : kWienerOrders) {
    for (const auto horizontal_order : kWienerOrders) {
      SetInputData(1, (1 << bitdepth) - 1, vertical_order, horizontal_order);
      memset(dst_, 0, sizeof(dst_));
      const absl::Time start = absl::Now();
      for (int i = 0; i < num_tests; ++i) {
        for (int y = 0; y < kHeight; y += unit_height_) {
          const int height = std::min(unit_height_, kHeight - y);
          for (int x = 0; x < kWidth; x += unit_width_) {
            const int width = std::min(unit_width_, kWidth - x);
            const Pixel* const source = src + y * kStride + x;
            target_wiener_filter_func_(
                restoration_info_, source, kStride,
                source - kRestorationVerticalBorder * kStride, kStride,
                source + height * kStride, kStride, width, height,
                &restoration_buffer_, dst + y * kStride + x);
          }
        }
      }
      const absl::Duration elapsed_time = absl::Now() - start;
      test_utils::CheckMd5Digest(
          "kLoopRestorationTypeWiener", std::to_string(GetParam()).c_str(),
          kDigest[bd_index][kWienerOrderIdLookup[vertical_order]]
                 [kWienerOrderIdLookup[horizontal_order]],
          dst_, sizeof(dst_), elapsed_time);
    }
  }
}

template <int bitdepth, typename Pixel>
void WienerFilterTest<bitdepth, Pixel>::TestCompare2C() {
  if (base_wiener_filter_func_ == nullptr) return;
  if (target_wiener_filter_func_ == nullptr) return;
  if (base_wiener_filter_func_ == target_wiener_filter_func_) return;
  const Pixel* const src = src_ + kOffset;
  Pixel* const dst = dst_ + kOffset;
  Pixel* const tmp = tmp_ + kOffset;
  for (const auto vertical_order : kWienerOrders) {
    for (const auto horizontal_order : kWienerOrders) {
      SetInputData(1, (1 << bitdepth) - 1, vertical_order, horizontal_order);
      for (int x = 0; x < 2; ++x) {
        // Prepare min/max filter coefficients.
        int order = vertical_order;
        for (int i = WienerInfo::kVertical; i <= WienerInfo::kHorizontal; ++i) {
          auto& filter = restoration_info_.wiener_info.filter[i];
          for (int j = 0; j < 3; ++j) {
            filter[j] = (x == 0) ? kWienerTapsMin[j] : kWienerTapsMax[j];
          }
          CleanFilterByOrder(order, filter);
          filter[3] = 128 - 2 * (filter[0] + filter[1] + filter[2]);
          restoration_info_.wiener_info.number_leading_zero_coefficients[i] =
              (kWienerFilterTaps - order) / 2;
          order = horizontal_order;
        }
        base_wiener_filter_func_(restoration_info_, src, kStride,
                                 src - kRestorationVerticalBorder * kStride,
                                 kStride, src + unit_height_ * kStride, kStride,
                                 unit_width_, unit_height_,
                                 &restoration_buffer_, dst);
        target_wiener_filter_func_(restoration_info_, src, kStride,
                                   src - kRestorationVerticalBorder * kStride,
                                   kStride, src + unit_height_ * kStride,
                                   kStride, unit_width_, unit_height_,
                                   &restoration_buffer_, tmp);
        if (!test_utils::CompareBlocks(dst, tmp, unit_width_, unit_height_,
                                       kStride, kStride, false, true)) {
          ADD_FAILURE() << "Mismatch -- wiener taps min/max";
        }
      }
    }
  }
}

using WienerFilterTest8bpp = WienerFilterTest<8, uint8_t>;

TEST_P(WienerFilterTest8bpp, Correctness) {
  TestFixedValues(0, 0);
  TestFixedValues(1, 1);
  TestFixedValues(2, 128);
  TestFixedValues(3, 255);
  TestRandomValues(false);
}

TEST_P(WienerFilterTest8bpp, DISABLED_Speed) { TestRandomValues(true); }

TEST_P(WienerFilterTest8bpp, TestCompare2C) { TestCompare2C(); }

INSTANTIATE_TEST_SUITE_P(C, WienerFilterTest8bpp,
                         testing::ValuesIn(kUnitWidths));
#if LIBGAV1_ENABLE_AVX2
INSTANTIATE_TEST_SUITE_P(AVX2, WienerFilterTest8bpp,
                         testing::ValuesIn(kUnitWidths));
#endif
#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, WienerFilterTest8bpp,
                         testing::ValuesIn(kUnitWidths));
#endif
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, WienerFilterTest8bpp,
                         testing::ValuesIn(kUnitWidths));
#endif

#if LIBGAV1_MAX_BITDEPTH >= 10
using WienerFilterTest10bpp = WienerFilterTest<10, uint16_t>;

TEST_P(WienerFilterTest10bpp, Correctness) {
  TestFixedValues(0, 0);
  TestFixedValues(1, 1);
  TestFixedValues(2, 512);
  TestFixedValues(3, 1023);
  TestRandomValues(false);
}

TEST_P(WienerFilterTest10bpp, DISABLED_Speed) { TestRandomValues(true); }

TEST_P(WienerFilterTest10bpp, TestCompare2C) { TestCompare2C(); }

INSTANTIATE_TEST_SUITE_P(C, WienerFilterTest10bpp,
                         testing::ValuesIn(kUnitWidths));

#if LIBGAV1_ENABLE_AVX2
INSTANTIATE_TEST_SUITE_P(AVX2, WienerFilterTest10bpp,
                         testing::ValuesIn(kUnitWidths));
#endif
#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, WienerFilterTest10bpp,
                         testing::ValuesIn(kUnitWidths));
#endif
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, WienerFilterTest10bpp,
                         testing::ValuesIn(kUnitWidths));
#endif
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using WienerFilterTest12bpp = WienerFilterTest<12, uint16_t>;

TEST_P(WienerFilterTest12bpp, Correctness) {
  TestFixedValues(0, 0);
  TestFixedValues(1, 1);
  TestFixedValues(2, 2048);
  TestFixedValues(3, 4095);
  TestRandomValues(false);
}

TEST_P(WienerFilterTest12bpp, DISABLED_Speed) { TestRandomValues(true); }

TEST_P(WienerFilterTest12bpp, TestCompare2C) { TestCompare2C(); }

INSTANTIATE_TEST_SUITE_P(C, WienerFilterTest12bpp,
                         testing::ValuesIn(kUnitWidths));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace
}  // namespace dsp
}  // namespace libgav1
