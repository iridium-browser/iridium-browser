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

#include "src/dsp/motion_field_projection.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>

#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "gtest/gtest.h"
#include "src/dsp/dsp.h"
#include "src/utils/array_2d.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/cpu.h"
#include "src/utils/reference_info.h"
#include "src/utils/types.h"
#include "tests/third_party/libvpx/acm_random.h"
#include "tests/utils.h"

namespace libgav1 {
namespace dsp {
namespace {

constexpr int kMotionFieldWidth = 160;
constexpr int kMotionFieldHight = 120;

// The 'int' parameter is unused but required to allow for instantiations of C,
// NEON, etc.
class MotionFieldProjectionTest : public testing::TestWithParam<int> {
 public:
  MotionFieldProjectionTest() = default;
  MotionFieldProjectionTest(const MotionFieldProjectionTest&) = delete;
  MotionFieldProjectionTest& operator=(const MotionFieldProjectionTest&) =
      delete;
  ~MotionFieldProjectionTest() override = default;

  void SetUp() override {
    test_utils::ResetDspTable(8);
    MotionFieldProjectionInit_C();
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const char* const test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
    } else if (absl::StartsWith(test_case, "NEON/")) {
      MotionFieldProjectionInit_NEON();
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      if ((GetCpuInfo() & kSSE4_1) != 0) {
        MotionFieldProjectionInit_SSE4_1();
      }
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }
    const Dsp* const dsp = GetDspTable(8);
    ASSERT_NE(dsp, nullptr);
    target_motion_field_projection_kernel_func_ =
        dsp->motion_field_projection_kernel;
  }

  void SetInputData(int motion_field_width, libvpx_test::ACMRandom* rnd);
  void TestRandomValues(bool speed);

 private:
  MotionFieldProjectionKernelFunc target_motion_field_projection_kernel_func_;
  ReferenceInfo reference_info_;
  TemporalMotionField motion_field_;
};

void MotionFieldProjectionTest::SetInputData(
    const int motion_field_width, libvpx_test::ACMRandom* const rnd) {
  ASSERT_TRUE(reference_info_.Reset(kMotionFieldHight, motion_field_width));
  ASSERT_TRUE(motion_field_.mv.Reset(kMotionFieldHight, motion_field_width,
                                     /*zero_initialize=*/false));
  ASSERT_TRUE(motion_field_.reference_offset.Reset(kMotionFieldHight,
                                                   motion_field_width,
                                                   /*zero_initialize=*/false));
  constexpr int order_hint_bits = 6;
  unsigned int order_hint_shift_bits = Mod32(32 - order_hint_bits);
  const unsigned int current_frame_order_hint =
      rnd->Rand8() & ((1 << order_hint_bits) - 1);  // [0, 63]
  uint8_t reference_frame_order_hint = 0;
  reference_info_.relative_distance_to[0] = 0;
  reference_info_.skip_references[kReferenceFrameIntra] = true;
  reference_info_.projection_divisions[kReferenceFrameIntra] = 0;
  for (int i = kReferenceFrameLast; i < kNumReferenceFrameTypes; ++i) {
    reference_frame_order_hint =
        rnd->Rand8() & ((1 << order_hint_bits) - 1);  // [0, 63]
    const int relative_distance_to =
        GetRelativeDistance(current_frame_order_hint,
                            reference_frame_order_hint, order_hint_shift_bits);
    reference_info_.relative_distance_to[i] = relative_distance_to;
    reference_info_.skip_references[i] =
        relative_distance_to > kMaxFrameDistance || relative_distance_to <= 0;
    reference_info_.projection_divisions[i] =
        reference_info_.skip_references[i]
            ? 0
            : kProjectionMvDivisionLookup[relative_distance_to];
  }
  for (int y = 0; y < kMotionFieldHight; ++y) {
    for (int x = 0; x < motion_field_width; ++x) {
      reference_info_.motion_field_reference_frame[y][x] =
          static_cast<ReferenceFrameType>(rnd->Rand16() &
                                          kReferenceFrameAlternate);
      reference_info_.motion_field_mv[y][x].mv[0] = rnd->Rand16Signed() / 512;
      reference_info_.motion_field_mv[y][x].mv[1] = rnd->Rand16Signed() / 512;
    }
  }
  MotionVector invalid_mv;
  invalid_mv.mv[0] = kInvalidMvValue;
  invalid_mv.mv[1] = kInvalidMvValue;
  MotionVector* const motion_field_mv = &motion_field_.mv[0][0];
  int8_t* const motion_field_reference_offset =
      &motion_field_.reference_offset[0][0];
  std::fill(motion_field_mv, motion_field_mv + motion_field_.mv.size(),
            invalid_mv);
  std::fill(
      motion_field_reference_offset,
      motion_field_reference_offset + motion_field_.reference_offset.size(),
      -128);
}

void MotionFieldProjectionTest::TestRandomValues(bool speed) {
  static const char* const kDigestMv[8] = {
      "87c2a74538f5c015809492ac2e521075", "ba7b4a5d82c6083b13a5b02eb7655ab7",
      "8c37d96bf1744d5553860bf44a4f60a3", "720aa644f85e48995db9785e87cd02e3",
      "9289c0c66524bb77a605870d78285f35", "f0326509885c2b2c89feeac53698cd47",
      "6b9ad1d672dec825cb1803063d35badc", "dfe06c57cc9c70d27246df7fd0afa0b2"};
  static const char* const kDigestReferenceOffset[8] = {
      "d8d1384268d7cf5c4514b39c329f94fb", "7f30e79ceb064befbad64a20d206a540",
      "61e2eb5644edbd3a91b939403edc891e", "7a018f1bf88193e86934241af445dc36",
      "2d6166bf8bbe1db77baf687ecf71d028", "95fee61f0219e06076d6f0e1073b1a4e",
      "64d0a63751267bdc573cab761f1fe685", "906a99e0e791dbcb9183c9b68ecc4ea3"};
  const int num_tests = speed ? 2000 : 1;
  if (target_motion_field_projection_kernel_func_ == nullptr) return;
  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
  for (int width_idx = 0; width_idx < 8; ++width_idx) {
    const int motion_field_width = kMotionFieldWidth + width_idx;
    SetInputData(motion_field_width, &rnd);
    const int dst_sign = ((rnd.Rand16() & 1) != 0) ? 0 : -1;
    const int reference_to_current_with_sign =
        rnd.PseudoUniform(2 * kMaxFrameDistance + 1) - kMaxFrameDistance;
    assert(std::abs(reference_to_current_with_sign) <= kMaxFrameDistance);
    // Step of y8 and x8 is at least 16 except the last hop.
    for (int step = 16; step <= 80; step += 16) {
      const absl::Time start = absl::Now();
      for (int k = 0; k < num_tests; ++k) {
        for (int y8 = 0; y8 < kMotionFieldHight; y8 += step) {
          const int y8_end = std::min(y8 + step, kMotionFieldHight);
          for (int x8 = 0; x8 < motion_field_width; x8 += step) {
            const int x8_end = std::min(x8 + step, motion_field_width);
            target_motion_field_projection_kernel_func_(
                reference_info_, reference_to_current_with_sign, dst_sign, y8,
                y8_end, x8, x8_end, &motion_field_);
          }
        }
      }
      const absl::Duration elapsed_time = absl::Now() - start;
      test_utils::CheckMd5Digest(
          "MotionFieldProjectionKernel",
          absl::StrFormat("(mv) width %d  step %d", motion_field_width, step)
              .c_str(),
          kDigestMv[width_idx], motion_field_.mv[0],
          sizeof(motion_field_.mv[0][0]) * motion_field_.mv.size(),
          elapsed_time);
      test_utils::CheckMd5Digest(
          "MotionFieldProjectionKernel",
          absl::StrFormat("(ref offset) width %d  step %d", motion_field_width,
                          step)
              .c_str(),
          kDigestReferenceOffset[width_idx], motion_field_.reference_offset[0],
          sizeof(motion_field_.reference_offset[0][0]) *
              motion_field_.reference_offset.size(),
          elapsed_time);
    }
  }
}

TEST_P(MotionFieldProjectionTest, Correctness) { TestRandomValues(false); }

TEST_P(MotionFieldProjectionTest, DISABLED_Speed) { TestRandomValues(true); }

INSTANTIATE_TEST_SUITE_P(C, MotionFieldProjectionTest, testing::Values(0));

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, MotionFieldProjectionTest, testing::Values(0));
#endif

#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, MotionFieldProjectionTest, testing::Values(0));
#endif

}  // namespace
}  // namespace dsp
}  // namespace libgav1
