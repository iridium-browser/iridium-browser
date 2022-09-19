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

#include "src/dsp/loop_filter.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ostream>
#include <string>

#include "absl/strings/match.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "gtest/gtest.h"
#include "src/dsp/dsp.h"
#include "src/utils/constants.h"
#include "src/utils/cpu.h"
#include "tests/block_utils.h"
#include "tests/third_party/libvpx/acm_random.h"
#include "tests/third_party/libvpx/md5_helper.h"
#include "tests/utils.h"

namespace libgav1 {
namespace dsp {
namespace {

// Horizontal and Vertical need 32x32: 8  pixels preceding filtered section
//                                     16 pixels within filtered section
//                                     8  pixels following filtered section
constexpr int kNumPixels = 1024;
constexpr int kBlockStride = 32;

constexpr int kNumTests = 50000;
constexpr int kNumSpeedTests = 500000;

template <typename Pixel>
void InitInput(Pixel* dst, const int stride, const int bitdepth,
               libvpx_test::ACMRandom& rnd, const uint8_t inner_thresh,
               const bool transpose) {
  const int max_pixel = (1 << bitdepth) - 1;
  const int pixel_range = max_pixel + 1;
  Pixel tmp[kNumPixels];
  auto clip_pixel = [max_pixel](int val) {
    return static_cast<Pixel>(std::max(std::min(val, max_pixel), 0));
  };

  for (int i = 0; i < kNumPixels;) {
    const uint8_t val = rnd.Rand8();
    if (val & 0x80) {  // 50% chance to choose a new value.
      tmp[i++] = rnd(pixel_range);
    } else {  // 50% chance to repeat previous value in row X times.
      int j = 0;
      while (j++ < ((val & 0x1f) + 1) && i < kNumPixels) {
        if (i < 1) {
          tmp[i] = rnd(pixel_range);
        } else if (val & 0x20) {  // Increment by a value within the limit.
          tmp[i] = clip_pixel(tmp[i - 1] + (inner_thresh - 1));
        } else {  // Decrement by a value within the limit.
          tmp[i] = clip_pixel(tmp[i - 1] - (inner_thresh - 1));
        }
        ++i;
      }
    }
  }

  for (int i = 0; i < kNumPixels;) {
    const uint8_t val = rnd.Rand8();
    if (val & 0x80) {
      ++i;
    } else {  // 50% chance to repeat previous value in column X times.
      int j = 0;
      while (j++ < ((val & 0x1f) + 1) && i < kNumPixels) {
        if (i < 1) {
          tmp[i] = rnd(pixel_range);
        } else if (val & 0x20) {  // Increment by a value within the limit.
          tmp[(i % 32) * 32 + i / 32] = clip_pixel(
              tmp[((i - 1) % 32) * 32 + (i - 1) / 32] + (inner_thresh - 1));
        } else {  // Decrement by a value within the inner_thresh.
          tmp[(i % 32) * 32 + i / 32] = clip_pixel(
              tmp[((i - 1) % 32) * 32 + (i - 1) / 32] - (inner_thresh - 1));
        }
        ++i;
      }
    }
  }

  for (int i = 0; i < kNumPixels; ++i) {
    const int offset = transpose ? stride * (i % stride) + i / stride : i;
    dst[i] = tmp[offset];
  }
}

template <int bitdepth, typename Pixel>
class LoopFilterTest : public testing::TestWithParam<LoopFilterSize> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  LoopFilterTest() = default;
  LoopFilterTest(const LoopFilterTest&) = delete;
  LoopFilterTest& operator=(const LoopFilterTest&) = delete;
  ~LoopFilterTest() override = default;

 protected:
  void SetUp() override {
    test_utils::ResetDspTable(bitdepth);
    LoopFilterInit_C();

    const Dsp* const dsp = GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);
    memcpy(base_loop_filters_, dsp->loop_filters[size_],
           sizeof(base_loop_filters_));

    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const char* const test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
      memset(base_loop_filters_, 0, sizeof(base_loop_filters_));
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      if ((GetCpuInfo() & kSSE4_1) != 0) {
        LoopFilterInit_SSE4_1();
      }
    } else if (absl::StartsWith(test_case, "NEON/")) {
      LoopFilterInit_NEON();
#if LIBGAV1_MAX_BITDEPTH >= 10
      LoopFilterInit10bpp_NEON();
#endif
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }

    memcpy(cur_loop_filters_, dsp->loop_filters[size_],
           sizeof(cur_loop_filters_));

    for (int i = 0; i < kNumLoopFilterTypes; ++i) {
      // skip functions that haven't been specialized for this particular
      // architecture.
      if (cur_loop_filters_[i] == base_loop_filters_[i]) {
        cur_loop_filters_[i] = nullptr;
      }
    }
  }

  // Check |digests| if non-NULL otherwise print the filter timing.
  void TestRandomValues(const char* const digests[kNumLoopFilterTypes],
                        int num_runs) const;
  void TestSaturatedValues() const;

  const LoopFilterSize size_ = GetParam();
  LoopFilterFunc base_loop_filters_[kNumLoopFilterTypes];
  LoopFilterFunc cur_loop_filters_[kNumLoopFilterTypes];
};

template <int bitdepth, typename Pixel>
void LoopFilterTest<bitdepth, Pixel>::TestRandomValues(
    const char* const digests[kNumLoopFilterTypes], const int num_runs) const {
  for (int i = 0; i < kNumLoopFilterTypes; ++i) {
    libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
    if (cur_loop_filters_[i] == nullptr) continue;

    libvpx_test::MD5 md5_digest;
    absl::Duration elapsed_time;
    for (int n = 0; n < num_runs; ++n) {
      Pixel dst[kNumPixels];
      const auto outer_thresh = static_cast<uint8_t>(
          rnd(3 * kMaxLoopFilterValue - 2) + 7);  // [7, 193].
      const auto inner_thresh =
          static_cast<uint8_t>(rnd(kMaxLoopFilterValue) + 1);  // [1, 63].
      const auto hev_thresh =
          static_cast<uint8_t>(rnd(kMaxLoopFilterValue + 1) >> 4);  // [0, 3].
      InitInput(dst, kBlockStride, bitdepth, rnd, inner_thresh, (n & 1) == 0);

      const absl::Time start = absl::Now();
      cur_loop_filters_[i](dst + 8 + kBlockStride * 8, kBlockStride,
                           outer_thresh, inner_thresh, hev_thresh);
      elapsed_time += absl::Now() - start;

      md5_digest.Add(reinterpret_cast<const uint8_t*>(dst), sizeof(dst));
    }
    if (digests == nullptr) {
      const auto elapsed_time_us =
          static_cast<int>(absl::ToInt64Microseconds(elapsed_time));
      printf("Mode %s[%25s]: %5d us\n",
             ToString(static_cast<LoopFilterSize>(size_)),
             ToString(static_cast<LoopFilterType>(i)), elapsed_time_us);
    } else {
      const std::string digest = md5_digest.Get();
      printf("Mode %s[%25s]: MD5: %s\n",
             ToString(static_cast<LoopFilterSize>(size_)),
             ToString(static_cast<LoopFilterType>(i)), digest.c_str());
      EXPECT_STREQ(digests[i], digest.c_str());
    }
  }
}

template <int bitdepth, typename Pixel>
void LoopFilterTest<bitdepth, Pixel>::TestSaturatedValues() const {
  Pixel dst[kNumPixels], ref[kNumPixels];
  const auto value = static_cast<Pixel>((1 << bitdepth) - 1);
  for (auto& r : dst) r = value;
  memcpy(ref, dst, sizeof(dst));

  for (int i = 0; i < kNumLoopFilterTypes; ++i) {
    if (cur_loop_filters_[i] == nullptr) return;
    const int outer_thresh = 24;
    const int inner_thresh = 8;
    const int hev_thresh = 0;
    cur_loop_filters_[i](dst + 8 + kBlockStride * 8, kBlockStride, outer_thresh,
                         inner_thresh, hev_thresh);
    ASSERT_TRUE(test_utils::CompareBlocks(ref, dst, kBlockStride, kBlockStride,
                                          kBlockStride, kBlockStride, true))
        << ToString(static_cast<LoopFilterType>(i))
        << " output doesn't match reference";
  }
}

//------------------------------------------------------------------------------

using LoopFilterTest8bpp = LoopFilterTest<8, uint8_t>;

const char* const* GetDigests8bpp(LoopFilterSize size) {
  static const char* const kDigestsSize4[kNumLoopFilterTypes] = {
      "6ba725d697d6209cb36dd199b8ffb47a",
      "7dbb20e456ed0501fb4e7954f49f5e18",
  };
  static const char* const kDigestsSize6[kNumLoopFilterTypes] = {
      "89bb757faa44298b7f6e9c1a67f455a5",
      "be75d5a2fcd83709ff0845f7d83f7006",
  };
  static const char* const kDigestsSize8[kNumLoopFilterTypes] = {
      "b09137d68c7b4f8a8a15e33b4b69828f",
      "ef8a7f1aa073805516d3518a82a5cfa4",
  };
  static const char* const kDigestsSize14[kNumLoopFilterTypes] = {
      "6a7bc061ace0888275af88093f82ca08",
      "a957ddae005839aa41ba7691788b01e4",
  };

  switch (size) {
    case kLoopFilterSize4:
      return kDigestsSize4;
    case kLoopFilterSize6:
      return kDigestsSize6;
    case kLoopFilterSize8:
      return kDigestsSize8;
    case kLoopFilterSize14:
      return kDigestsSize14;
    default:
      ADD_FAILURE() << "Unknown loop filter size" << size;
      return nullptr;
  }
}

TEST_P(LoopFilterTest8bpp, DISABLED_Speed) {
  TestRandomValues(nullptr, kNumSpeedTests);
}

TEST_P(LoopFilterTest8bpp, FixedInput) {
  TestRandomValues(GetDigests8bpp(size_), kNumTests);
}

TEST_P(LoopFilterTest8bpp, SaturatedValues) { TestSaturatedValues(); }

constexpr LoopFilterSize kLoopFilterSizes[] = {
    kLoopFilterSize4, kLoopFilterSize6, kLoopFilterSize8, kLoopFilterSize14};

INSTANTIATE_TEST_SUITE_P(C, LoopFilterTest8bpp,
                         testing::ValuesIn(kLoopFilterSizes));

#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, LoopFilterTest8bpp,
                         testing::ValuesIn(kLoopFilterSizes));
#endif
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, LoopFilterTest8bpp,
                         testing::ValuesIn(kLoopFilterSizes));
#endif
//------------------------------------------------------------------------------

#if LIBGAV1_MAX_BITDEPTH >= 10
using LoopFilterTest10bpp = LoopFilterTest<10, uint16_t>;

const char* const* GetDigests10bpp(LoopFilterSize size) {
  static const char* const kDigestsSize4[kNumLoopFilterTypes] = {
      "72e75c478bb130ff1ebfa75f3a70b1a2",
      "f32d67b611080e0bf1a9d162ff47c133",
  };
  static const char* const kDigestsSize6[kNumLoopFilterTypes] = {
      "8aec73c60c87ac7cc6bc9cc5157a2795",
      "0e4385d3a0cbb2b1551e05ad2b0f07fb",
  };
  static const char* const kDigestsSize8[kNumLoopFilterTypes] = {
      "85cb2928fae43e1a27b2fe1b78ba7534",
      "d044fad9d7c64b93ecb60c88ac48e55f",
  };
  static const char* const kDigestsSize14[kNumLoopFilterTypes] = {
      "ebca95ec0db6efbac7ff7cbeabc0e6d0",
      "754ffaf0ac26a5953a029653bb5dd275",
  };

  switch (size) {
    case kLoopFilterSize4:
      return kDigestsSize4;
    case kLoopFilterSize6:
      return kDigestsSize6;
    case kLoopFilterSize8:
      return kDigestsSize8;
    case kLoopFilterSize14:
      return kDigestsSize14;
    default:
      ADD_FAILURE() << "Unknown loop filter size" << size;
      return nullptr;
  }
}

TEST_P(LoopFilterTest10bpp, DISABLED_Speed) {
  TestRandomValues(nullptr, kNumSpeedTests);
}

TEST_P(LoopFilterTest10bpp, FixedInput) {
  TestRandomValues(GetDigests10bpp(size_), kNumTests);
}

TEST_P(LoopFilterTest10bpp, SaturatedValues) { TestSaturatedValues(); }

INSTANTIATE_TEST_SUITE_P(C, LoopFilterTest10bpp,
                         testing::ValuesIn(kLoopFilterSizes));

#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, LoopFilterTest10bpp,
                         testing::ValuesIn(kLoopFilterSizes));
#endif
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, LoopFilterTest10bpp,
                         testing::ValuesIn(kLoopFilterSizes));
#endif
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

//------------------------------------------------------------------------------

#if LIBGAV1_MAX_BITDEPTH == 12
using LoopFilterTest12bpp = LoopFilterTest<12, uint16_t>;

const char* const* GetDigests12bpp(LoopFilterSize size) {
  static const char* const kDigestsSize4[kNumLoopFilterTypes] = {
      "a14599cbfe2daee633d556a15c47b1f6",
      "1f0a0794832de1012e2fed6b1cb02e69",
  };
  static const char* const kDigestsSize6[kNumLoopFilterTypes] = {
      "c76b24a73139239db10f16f36e01a625",
      "3f75d904e9dcb1886e84a0f03f60f31e",
  };
  static const char* const kDigestsSize8[kNumLoopFilterTypes] = {
      "57c6f0efe2ab3957f5500ca2a9670f37",
      "caa1f90c2eb2b65b280d678f8fcf6be8",
  };
  static const char* const kDigestsSize14[kNumLoopFilterTypes] = {
      "0c58f7466c36c3f4a2c1b4aa1b80f0b3",
      "63077978326e6dddb5b2c3bfe6d684f5",
  };

  switch (size) {
    case kLoopFilterSize4:
      return kDigestsSize4;
    case kLoopFilterSize6:
      return kDigestsSize6;
    case kLoopFilterSize8:
      return kDigestsSize8;
    case kLoopFilterSize14:
      return kDigestsSize14;
    default:
      ADD_FAILURE() << "Unknown loop filter size" << size;
      return nullptr;
  }
}

TEST_P(LoopFilterTest12bpp, DISABLED_Speed) {
  TestRandomValues(nullptr, kNumSpeedTests);
}

TEST_P(LoopFilterTest12bpp, FixedInput) {
  TestRandomValues(GetDigests12bpp(size_), kNumTests);
}

TEST_P(LoopFilterTest12bpp, SaturatedValues) { TestSaturatedValues(); }

INSTANTIATE_TEST_SUITE_P(C, LoopFilterTest12bpp,
                         testing::ValuesIn(kLoopFilterSizes));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace

static std::ostream& operator<<(std::ostream& os, const LoopFilterSize size) {
  return os << ToString(size);
}

}  // namespace dsp
}  // namespace libgav1
