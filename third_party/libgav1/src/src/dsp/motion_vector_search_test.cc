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

#include "src/dsp/motion_vector_search.h"

#include <cstdint>
#include <string>

#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "gtest/gtest.h"
#include "src/dsp/dsp.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/cpu.h"
#include "src/utils/memory.h"
#include "src/utils/types.h"
#include "tests/third_party/libvpx/acm_random.h"
#include "tests/utils.h"

namespace libgav1 {
namespace dsp {
namespace {

// The 'int' parameter is unused but required to allow for instantiations of C,
// NEON, etc.
class MotionVectorSearchTest : public testing::TestWithParam<int>,
                               public test_utils::MaxAlignedAllocable {
 public:
  MotionVectorSearchTest() = default;
  MotionVectorSearchTest(const MotionVectorSearchTest&) = delete;
  MotionVectorSearchTest& operator=(const MotionVectorSearchTest&) = delete;
  ~MotionVectorSearchTest() override = default;

  void SetUp() override {
    test_utils::ResetDspTable(8);
    MotionVectorSearchInit_C();
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const char* const test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
    } else if (absl::StartsWith(test_case, "NEON/")) {
      MotionVectorSearchInit_NEON();
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      if ((GetCpuInfo() & kSSE4_1) != 0) {
        MotionVectorSearchInit_SSE4_1();
      }
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }
    const Dsp* const dsp = GetDspTable(8);
    ASSERT_NE(dsp, nullptr);
    mv_projection_compound_[0] = dsp->mv_projection_compound[0];
    mv_projection_compound_[1] = dsp->mv_projection_compound[1];
    mv_projection_compound_[2] = dsp->mv_projection_compound[2];
    mv_projection_single_[0] = dsp->mv_projection_single[0];
    mv_projection_single_[1] = dsp->mv_projection_single[1];
    mv_projection_single_[2] = dsp->mv_projection_single[2];
  }

  void SetInputData(libvpx_test::ACMRandom* rnd);
  void TestRandomValues(bool speed);

 private:
  MvProjectionCompoundFunc mv_projection_compound_[3];
  MvProjectionSingleFunc mv_projection_single_[3];
  int reference_offsets_[2];
  alignas(kMaxAlignment)
      MotionVector temporal_mvs_[kMaxTemporalMvCandidatesWithPadding];
  int8_t temporal_reference_offsets_[kMaxTemporalMvCandidatesWithPadding];
  CompoundMotionVector compound_mv_org_[kMaxTemporalMvCandidates + 1]
                                       [kMaxTemporalMvCandidatesWithPadding];
  alignas(kMaxAlignment)
      CompoundMotionVector compound_mv_[kMaxTemporalMvCandidates + 1]
                                       [kMaxTemporalMvCandidatesWithPadding];
  MotionVector single_mv_org_[kMaxTemporalMvCandidates + 1]
                             [kMaxTemporalMvCandidatesWithPadding];
  alignas(kMaxAlignment)
      MotionVector single_mv_[kMaxTemporalMvCandidates + 1]
                             [kMaxTemporalMvCandidatesWithPadding];
};

void MotionVectorSearchTest::SetInputData(libvpx_test::ACMRandom* const rnd) {
  reference_offsets_[0] =
      Clip3(rnd->Rand16(), -kMaxFrameDistance, kMaxFrameDistance);
  reference_offsets_[1] =
      Clip3(rnd->Rand16(), -kMaxFrameDistance, kMaxFrameDistance);
  for (int i = 0; i < kMaxTemporalMvCandidatesWithPadding; ++i) {
    temporal_reference_offsets_[i] = rnd->RandRange(kMaxFrameDistance);
    for (auto& mv : temporal_mvs_[i].mv) {
      mv = rnd->Rand16Signed() / 8;
    }
  }
  for (int i = 0; i <= kMaxTemporalMvCandidates; ++i) {
    for (int j = 0; j < kMaxTemporalMvCandidatesWithPadding; ++j) {
      for (int k = 0; k < 2; ++k) {
        single_mv_[i][j].mv[k] = rnd->Rand16Signed();
        for (auto& mv : compound_mv_[i][j].mv[k].mv) {
          mv = rnd->Rand16Signed();
        }
      }
      compound_mv_org_[i][j] = compound_mv_[i][j];
      single_mv_org_[i][j] = single_mv_[i][j];
    }
  }
}

void MotionVectorSearchTest::TestRandomValues(bool speed) {
  static const char* const kDigestCompound[3] = {
      "74c055b06c3701b2e50f2c964a6130b9", "cab21dd54f0a1bf6e80b58cdcf1fe0a9",
      "e42de30cd84fa4e7b8581a330ed08a8b"};
  static const char* const kDigestSingle[3] = {
      "265ffbb59d0895183f8e2d90b6652c71", "5068d980c4ce42ed3f11963b8aece6cc",
      "7e699d58df3954a38ff11c8e34151e66"};
  const int num_tests = speed ? 1000000 : 1;
  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
  for (int function_index = 0; function_index < 3; ++function_index) {
    SetInputData(&rnd);
    if (mv_projection_compound_[function_index] == nullptr) continue;
    const absl::Time start = absl::Now();
    for (int count = 1; count <= kMaxTemporalMvCandidates; ++count) {
      const int total_count = count + (count & 1);
      for (int i = 0; i < num_tests; ++i) {
        mv_projection_compound_[function_index](
            temporal_mvs_, temporal_reference_offsets_, reference_offsets_,
            count, compound_mv_[count]);
      }
      // One more element could be calculated in SIMD implementations.
      // Restore the original values if any.
      for (int i = count; i < total_count; ++i) {
        compound_mv_[count][i] = compound_mv_org_[count][i];
      }
    }
    const absl::Duration elapsed_time = absl::Now() - start;
    test_utils::CheckMd5Digest(
        "MvProjectionCompound",
        absl::StrFormat("function_index %d", function_index).c_str(),
        kDigestCompound[function_index], compound_mv_, sizeof(compound_mv_),
        elapsed_time);
  }
  for (int function_index = 0; function_index < 3; ++function_index) {
    SetInputData(&rnd);
    if (mv_projection_single_[function_index] == nullptr) continue;
    const absl::Time start = absl::Now();
    for (int count = 1; count <= kMaxTemporalMvCandidates; ++count) {
      const int total_count = (count + 3) & ~3;
      for (int i = 0; i < num_tests; ++i) {
        mv_projection_single_[function_index](
            temporal_mvs_, temporal_reference_offsets_, reference_offsets_[0],
            count, single_mv_[count]);
      }
      // Up to three more elements could be calculated in SIMD implementations.
      // Restore the original values if any.
      for (int i = count; i < total_count; ++i) {
        single_mv_[count][i] = single_mv_org_[count][i];
      }
    }
    const absl::Duration elapsed_time = absl::Now() - start;
    test_utils::CheckMd5Digest(
        "MvProjectionSingle",
        absl::StrFormat("function_index %d", function_index).c_str(),
        kDigestSingle[function_index], single_mv_, sizeof(single_mv_),
        elapsed_time);
  }
}

TEST_P(MotionVectorSearchTest, Correctness) { TestRandomValues(false); }

TEST_P(MotionVectorSearchTest, DISABLED_Speed) { TestRandomValues(true); }

INSTANTIATE_TEST_SUITE_P(C, MotionVectorSearchTest, testing::Values(0));

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, MotionVectorSearchTest, testing::Values(0));
#endif

#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, MotionVectorSearchTest, testing::Values(0));
#endif

}  // namespace
}  // namespace dsp
}  // namespace libgav1
