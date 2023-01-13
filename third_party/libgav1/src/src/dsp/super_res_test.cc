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

#include "src/dsp/super_res.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
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

constexpr int kNumSpeedTests = 5e5;

const char* GetDigest8bpp(int id) {
  static const char* const kDigestSuperRes[] = {
      "52eb4eac1df0c51599d57696405b69d0", "ccb07cc8295fd1440ff2e3b9199ec4f9",
      "baef34cca795b95f3d1fd81d609da679", "03f1579c2773c8ba9c867316a22b94a3"};
  return kDigestSuperRes[id];
}

#if LIBGAV1_MAX_BITDEPTH >= 10
const char* GetDigest10bpp(int id) {
  static const char* const kDigestSuperRes[] = {
      "8fd78e05d944aeb11fac278b47ee60ba", "948eaecb70fa5614ce1c1c95e9942dc3",
      "126cd7727e787e0625ec3f5ce97f8fa0", "85c806c41d40b841764bcb54f6d3a712"};
  return kDigestSuperRes[id];
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
const char* GetDigest12bpp(int id) {
  static const char* const kDigestSuperRes[] = {
      "9a08983d82df4983700976f18919201b", "6e5edbafcb6c38db37258bf79c00ea32",
      "f5c57e6d3b518f9585f768ed19b91568", "b5de9b93c8a1a50580e7c7c9456fb615"};
  return kDigestSuperRes[id];
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

struct SuperResTestParam {
  SuperResTestParam(int downscaled_width, int upscaled_width)
      : downscaled_width(downscaled_width), upscaled_width(upscaled_width) {}
  int downscaled_width;
  int upscaled_width;
};

template <int bitdepth, typename Pixel, typename Coefficient>
class SuperResTest : public testing::TestWithParam<SuperResTestParam>,
                     public test_utils::MaxAlignedAllocable {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  SuperResTest() = default;
  void SetUp() override {
    test_utils::ResetDspTable(bitdepth);
    SuperResInit_C();
    const dsp::Dsp* const dsp = dsp::GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);

    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const std::vector<std::string> split_test_name =
        absl::StrSplit(test_info->name(), '/');
    ASSERT_TRUE(absl::SimpleAtoi(split_test_name[1], &test_id_));
    const absl::string_view test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
    } else if (absl::StartsWith(test_case, "NEON/")) {
      SuperResInit_NEON();
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      SuperResInit_SSE4_1();
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }
    super_res_coefficients_ = dsp->super_res_coefficients;
    func_ = dsp->super_res;
  }

  void TestComputeSuperRes(int fixed_value, int num_runs);

 private:
  static constexpr int kHeight = 127;
  // The maximum width that must be allocated.
  static constexpr int kUpscaledBufferWidth = 192;
  // Allow room for the filter taps.
  static constexpr int kStride =
      ((kUpscaledBufferWidth + 2 * kSuperResHorizontalBorder + 15) & ~15);
  const int kDownscaledWidth = GetParam().downscaled_width;
  const int kUpscaledWidth = GetParam().upscaled_width;
  int test_id_;
  SuperResCoefficientsFunc super_res_coefficients_;
  SuperResFunc func_;
  Pixel source_buffer_[kHeight][kStride];
  alignas(kMaxAlignment) Pixel dest_buffer_[kHeight][kStride];
  alignas(kMaxAlignment) Coefficient
      superres_coefficients_[kSuperResFilterTaps * kUpscaledBufferWidth];
};

template <int bitdepth, typename Pixel, typename Coefficient>
void SuperResTest<bitdepth, Pixel, Coefficient>::TestComputeSuperRes(
    int fixed_value, int num_runs) {
  if (func_ == nullptr) return;
  const int superres_width = kDownscaledWidth << kSuperResScaleBits;
  const int step = (superres_width + kUpscaledWidth / 2) / kUpscaledWidth;
  const int error = step * kUpscaledWidth - superres_width;
  const int initial_subpixel_x =
      ((-((kUpscaledWidth - kDownscaledWidth) << (kSuperResScaleBits - 1)) +
        DivideBy2(kUpscaledWidth)) /
           kUpscaledWidth +
       (1 << (kSuperResExtraBits - 1)) - error / 2) &
      kSuperResScaleMask;
  if (super_res_coefficients_ != nullptr) {
    super_res_coefficients_(kUpscaledWidth, initial_subpixel_x, step,
                            superres_coefficients_);
  }
  memset(dest_buffer_, 0, sizeof(dest_buffer_));
  if (fixed_value != 0) {
    SetBlock<Pixel>(kHeight, kStride, fixed_value, source_buffer_[0], kStride);
  } else {
    // Random values.
    libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
    const int bitdepth_mask = (1 << bitdepth) - 1;
    for (int y = 0; y < kHeight; ++y) {
      for (int x = 0; x < kStride; ++x) {
        source_buffer_[y][x] = rnd.Rand16() & bitdepth_mask;
      }
    }
  }
  // Offset starting point in the buffer to accommodate line extension.
  Pixel* src_ptr = source_buffer_[0] + kSuperResHorizontalBorder;

  const absl::Time start = absl::Now();
  for (int i = 0; i < num_runs; ++i) {
    func_(superres_coefficients_, src_ptr, kStride, kHeight, kDownscaledWidth,
          kUpscaledWidth, initial_subpixel_x, step, dest_buffer_, kStride);
  }
  const absl::Duration elapsed_time = absl::Now() - start;

  if (fixed_value != 0) {
    for (int y = 0; y < kHeight; ++y) {
      for (int x = 0; x < kUpscaledWidth; ++x) {
        EXPECT_TRUE(dest_buffer_[y][x] == fixed_value)
            << "At location [" << y << ", " << x
            << "]\nexpected: " << fixed_value
            << "\nactual: " << dest_buffer_[y][x];
      }
    }
  } else if (num_runs == 1) {
    // Random values.
    if ((kUpscaledWidth & 15) != 0) {
      // The SIMD functions overwrite up to 15 pixels in each row. Reset them.
      for (int y = 0; y < kHeight; ++y) {
        for (int x = kUpscaledWidth; x < Align(kUpscaledWidth, 16); ++x) {
          dest_buffer_[y][x] = 0;
        }
      }
    }
    const char* expected_digest = nullptr;
    switch (bitdepth) {
      case 8:
        expected_digest = GetDigest8bpp(test_id_);
        break;
#if LIBGAV1_MAX_BITDEPTH >= 10
      case 10:
        expected_digest = GetDigest10bpp(test_id_);
        break;
#endif
#if LIBGAV1_MAX_BITDEPTH == 12
      case 12:
        expected_digest = GetDigest12bpp(test_id_);
        break;
#endif
    }
    ASSERT_NE(expected_digest, nullptr);
    test_utils::CheckMd5Digest(
        "SuperRes",
        absl::StrFormat("width %d, step %d, start %d", kUpscaledWidth, step,
                        initial_subpixel_x)
            .c_str(),
        expected_digest, dest_buffer_, sizeof(dest_buffer_), elapsed_time);
  } else {
    // Speed test.
    printf("Mode SuperRes [width %d, step %d, start %d]: %d us\n",
           kUpscaledWidth, step, initial_subpixel_x,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  }
}

using SuperResTest8bpp = SuperResTest<8, uint8_t, int8_t>;

TEST_P(SuperResTest8bpp, FixedValues) {
  TestComputeSuperRes(100, 1);
  TestComputeSuperRes(255, 1);
  TestComputeSuperRes(1, 1);
}

TEST_P(SuperResTest8bpp, RandomValues) { TestComputeSuperRes(0, 1); }

TEST_P(SuperResTest8bpp, DISABLED_Speed) {
  TestComputeSuperRes(0, kNumSpeedTests);
}

const SuperResTestParam kSuperResTestParams[] = {
    SuperResTestParam(96, 192),
    SuperResTestParam(171, 192),
    SuperResTestParam(102, 128),
    SuperResTestParam(61, 121),
};

INSTANTIATE_TEST_SUITE_P(C, SuperResTest8bpp,
                         testing::ValuesIn(kSuperResTestParams));

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, SuperResTest8bpp,
                         testing::ValuesIn(kSuperResTestParams));
#endif

#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, SuperResTest8bpp,
                         testing::ValuesIn(kSuperResTestParams));
#endif

#if LIBGAV1_MAX_BITDEPTH >= 10
using SuperResTest10bpp = SuperResTest<10, uint16_t, int16_t>;

TEST_P(SuperResTest10bpp, FixedValues) {
  TestComputeSuperRes(100, 1);
  TestComputeSuperRes(511, 1);
  TestComputeSuperRes(1, 1);
}

TEST_P(SuperResTest10bpp, RandomValues) { TestComputeSuperRes(0, 1); }

TEST_P(SuperResTest10bpp, DISABLED_Speed) {
  TestComputeSuperRes(0, kNumSpeedTests);
}

INSTANTIATE_TEST_SUITE_P(C, SuperResTest10bpp,
                         testing::ValuesIn(kSuperResTestParams));

#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, SuperResTest10bpp,
                         testing::ValuesIn(kSuperResTestParams));
#endif

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, SuperResTest10bpp,
                         testing::ValuesIn(kSuperResTestParams));
#endif
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using SuperResTest12bpp = SuperResTest<12, uint16_t, int16_t>;

TEST_P(SuperResTest12bpp, FixedValues) {
  TestComputeSuperRes(100, 1);
  TestComputeSuperRes(2047, 1);
  TestComputeSuperRes(1, 1);
}

TEST_P(SuperResTest12bpp, RandomValues) { TestComputeSuperRes(0, 1); }

TEST_P(SuperResTest12bpp, DISABLED_Speed) {
  TestComputeSuperRes(0, kNumSpeedTests);
}

INSTANTIATE_TEST_SUITE_P(C, SuperResTest12bpp,
                         testing::ValuesIn(kSuperResTestParams));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace
}  // namespace dsp
}  // namespace libgav1
