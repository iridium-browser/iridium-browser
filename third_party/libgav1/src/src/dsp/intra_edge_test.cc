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

#include "src/dsp/intra_edge.h"

#include <cstdint>
#include <cstdio>
#include <ostream>

#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "gtest/gtest.h"
#include "src/dsp/dsp.h"
#include "src/utils/compiler_attributes.h"
#include "src/utils/constants.h"
#include "src/utils/cpu.h"
#include "tests/third_party/libvpx/acm_random.h"
#include "tests/utils.h"

namespace libgav1 {
namespace dsp {
namespace {

const char kIntraEdge[] = "IntraEdge";
const char kIntraEdgeFilterName[] = "Intra Edge Filter";
const char kIntraEdgeUpsamplerName[] = "Intra Edge Upsampler";

constexpr int kIntraEdgeBufferSize = 144;  // see Tile::IntraPrediction.
constexpr int kIntraEdgeFilterTestMaxSize = 129;
constexpr int kIntraEdgeFilterTestFixedInput[kIntraEdgeFilterTestMaxSize] = {
    159, 208, 54,  136, 205, 124, 125, 165, 164, 63,  171, 143, 210, 236, 253,
    233, 139, 113, 66,  211, 133, 61,  91,  123, 187, 76,  110, 172, 61,  103,
    239, 147, 247, 120, 18,  106, 180, 159, 208, 54,  136, 205, 124, 125, 165,
    164, 63,  171, 143, 210, 236, 253, 233, 139, 113, 66,  211, 133, 61,  91,
    123, 187, 76,  110, 172, 61,  103, 239, 147, 247, 120, 18,  106, 180, 159,
    208, 54,  136, 205, 124, 125, 165, 164, 63,  171, 143, 210, 236, 253, 233,
    139, 113, 66,  211, 133, 61,  91,  123, 187, 76,  110, 172, 61,  103, 239,
    147, 247, 120, 18,  106, 180, 159, 208, 54,  136, 205, 124, 125, 165, 164,
    63,  171, 143, 210, 236, 253, 233, 139, 113,
};
constexpr int kIntraEdgeUpsamplerTestFixedInput[] = {
    208, 54,  136, 205, 124, 125, 165, 164, 63,
    171, 143, 210, 236, 208, 54,  136, 205};

struct EdgeFilterParams {
  int size;
  int strength;
};

std::ostream& operator<<(std::ostream& os, const EdgeFilterParams& param) {
  return os << "size: " << param.size << ", strength: " << param.strength;
}

// Each size is paired with strength 1, 2, and 3.
// In general, the size is expressible as 2^n+1, but all sizes up to 129 are
// permissible.
constexpr EdgeFilterParams kIntraEdgeFilterParamList[] = {
    {1, 1},  {1, 2},  {1, 3},  {2, 1},   {2, 2},   {2, 3},  {5, 1},  {5, 2},
    {5, 3},  {9, 1},  {9, 2},  {9, 3},   {17, 1},  {17, 2}, {17, 3}, {33, 1},
    {33, 2}, {33, 3}, {50, 1}, {50, 2},  {50, 3},  {55, 1}, {55, 2}, {55, 3},
    {65, 1}, {65, 2}, {65, 3}, {129, 1}, {129, 2}, {129, 3}};

template <int bitdepth, typename Pixel>
class IntraEdgeFilterTest : public testing::TestWithParam<EdgeFilterParams> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  IntraEdgeFilterTest() = default;
  IntraEdgeFilterTest(const IntraEdgeFilterTest&) = delete;
  IntraEdgeFilterTest& operator=(const IntraEdgeFilterTest&) = delete;
  ~IntraEdgeFilterTest() override = default;

 protected:
  void SetUp() override {
    test_utils::ResetDspTable(bitdepth);
    IntraEdgeInit_C();

    const Dsp* const dsp = GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);
    base_intra_edge_filter_ = dsp->intra_edge_filter;

    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const absl::string_view test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
      base_intra_edge_filter_ = nullptr;
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      if ((GetCpuInfo() & kSSE4_1) != 0) {
        IntraEdgeInit_SSE4_1();
      }
    } else if (absl::StartsWith(test_case, "NEON/")) {
      IntraEdgeInit_NEON();
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }

#if LIBGAV1_MSAN
    // Match the behavior of Tile::IntraPrediction to prevent warnings due to
    // assembly code (safely) overreading to fill a register.
    memset(buffer_, 0, sizeof(buffer_));
#endif  // LIBGAV1_MSAN
    cur_intra_edge_filter_ = dsp->intra_edge_filter;
  }

  void TestFixedValues(const char* digest);
  void TestRandomValues(int num_runs);

  Pixel buffer_[kIntraEdgeBufferSize];
  Pixel base_buffer_[kIntraEdgeBufferSize];
  int strength_ = GetParam().strength;
  int size_ = GetParam().size;

  IntraEdgeFilterFunc base_intra_edge_filter_;
  IntraEdgeFilterFunc cur_intra_edge_filter_;
};

template <int bitdepth, typename Pixel>
void IntraEdgeFilterTest<bitdepth, Pixel>::TestFixedValues(
    const char* const digest) {
  if (cur_intra_edge_filter_ == nullptr) return;
  for (int i = 0; i < kIntraEdgeFilterTestMaxSize; ++i) {
    buffer_[i] = kIntraEdgeFilterTestFixedInput[i];
  }
  const absl::Time start = absl::Now();
  cur_intra_edge_filter_(buffer_, size_, strength_);
  const absl::Duration elapsed_time = absl::Now() - start;
  test_utils::CheckMd5Digest(kIntraEdge, kIntraEdgeFilterName, digest, buffer_,
                             kIntraEdgeFilterTestMaxSize * sizeof(buffer_[0]),
                             elapsed_time);
}

template <int bitdepth, typename Pixel>
void IntraEdgeFilterTest<bitdepth, Pixel>::TestRandomValues(int num_runs) {
  if (base_intra_edge_filter_ == nullptr) return;
  if (cur_intra_edge_filter_ == nullptr) return;
  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
  absl::Duration elapsed_time;
  absl::Duration base_elapsed_time;
  memset(base_buffer_, 0, sizeof(base_buffer_));
  memset(buffer_, 0, sizeof(buffer_));
  for (int num_tests = 0; num_tests < num_runs; ++num_tests) {
    for (int i = 0; i < size_; ++i) {
      const Pixel val = rnd(1 << bitdepth);
      buffer_[i] = val;
      base_buffer_[i] = val;
    }
    const absl::Time base_start = absl::Now();
    base_intra_edge_filter_(base_buffer_, size_, strength_);
    base_elapsed_time += absl::Now() - base_start;
    const absl::Time start = absl::Now();
    cur_intra_edge_filter_(buffer_, size_, strength_);
    elapsed_time += absl::Now() - start;
  }
  if (num_runs > 1) {
    printf("Mode %s[%31s] Size %3d Strength %d C: %5d us SIMD: %5d us %2.2fx\n",
           kIntraEdge, kIntraEdgeFilterName, size_, strength_,
           static_cast<int>(absl::ToInt64Microseconds(base_elapsed_time)),
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)),
           absl::ToDoubleMicroseconds(base_elapsed_time) /
               absl::ToDoubleMicroseconds(elapsed_time));
  } else {
    printf("Mode %s[%31s] Size %3d Strength %d\n", kIntraEdge,
           kIntraEdgeFilterName, size_, strength_);
  }
  for (int i = 0; i < kIntraEdgeFilterTestMaxSize; ++i) {
    EXPECT_EQ(buffer_[i], base_buffer_[i]) << "Mismatch in index: " << i;
  }
}

using IntraEdgeFilterTest8bpp = IntraEdgeFilterTest<8, uint8_t>;

const char* GetIntraEdgeFilterDigest8bpp(int strength, int size) {
  static const char* const kDigestsSize1[3] = {
      "f7f681cf7047602fafc7fb416ecf46e1", "f7f681cf7047602fafc7fb416ecf46e1",
      "f7f681cf7047602fafc7fb416ecf46e1"};
  static const char* const kDigestsSize2[3] = {
      "cb24cc54900fb75d767f3de797451e43", "380c80c89e1e8cda81ee0d3d4b29b8b7",
      "a7eb3dba95ff35c2df45a274afbc9772"};
  static const char* const kDigestsSize5[3] = {
      "23380cb37688d4c3a8f70a276be65eed", "ec1e23d5b996a527ed3d45c0d552bf22",
      "d313523d3b7646fdbb873c61ffe7a51a"};
  static const char* const kDigestsSize9[3] = {
      "e79597e9d62893754fc77d80ca86329a", "f7644e9748984914100e7031c6432272",
      "bdf4f16734c86338716fb436c196ecc6"};
  static const char* const kDigestsSize17[3] = {
      "13ad15c833e850348eecb9fea4f3cadb", "e5988a72391250c702a8192893df40dd",
      "8f68603598638fa33203fe1233d273b1"};
  static const char* const kDigestsSize33[3] = {
      "51156da8f4d527e0c011040769987dbd", "eff17eaf73a7bb7fd4c921510ade9f67",
      "aca87680e0649d0728091c92c6de8871"};
  static const char* const kDigestsSize50[3] = {
      "87c1d43751125f1ea4987517a90d378d", "942a9d056231683bdfc52346b6b032c2",
      "16a9148daf0e5f69808b9f0caa1ef110"};
  static const char* const kDigestsSize55[3] = {
      "833480d74957fb0356dec5b09412eefa", "a307ef31f10affc3b7fb262d05f1b80a",
      "0318b2fde088c472215fe155f3b48d36"};
  static const char* const kDigestsSize65[3] = {
      "5000dada34ed2e6692bb44a4398ddf53", "8da6c776d897064ecd4a1e84aae92dd3",
      "d7c71db339c28d33119974987b2f9d85"};
  static const char* const kDigestsSize129[3] = {
      "bf174d8b45b8131404fd4a4686f8c117", "e81518d6d85eed2f1b18c59424561d6b",
      "7306715602b0f5536771724a2f0a39bc"};

  switch (size) {
    case 1:
      return kDigestsSize1[strength - 1];
    case 2:
      return kDigestsSize2[strength - 1];
    case 5:
      return kDigestsSize5[strength - 1];
    case 9:
      return kDigestsSize9[strength - 1];
    case 17:
      return kDigestsSize17[strength - 1];
    case 33:
      return kDigestsSize33[strength - 1];
    case 50:
      return kDigestsSize50[strength - 1];
    case 55:
      return kDigestsSize55[strength - 1];
    case 65:
      return kDigestsSize65[strength - 1];
    case 129:
      return kDigestsSize129[strength - 1];
    default:
      ADD_FAILURE() << "Unknown edge size: " << size;
      return nullptr;
  }
}

TEST_P(IntraEdgeFilterTest8bpp, Correctness) {
  TestFixedValues(GetIntraEdgeFilterDigest8bpp(strength_, size_));
  TestRandomValues(1);
}

TEST_P(IntraEdgeFilterTest8bpp, DISABLED_Speed) { TestRandomValues(1e7); }

#if LIBGAV1_MAX_BITDEPTH >= 10
using IntraEdgeFilterTest10bpp = IntraEdgeFilterTest<10, uint16_t>;

const char* GetIntraEdgeFilterDigest10bpp(int strength, int size) {
  static const char* const kDigestsSize1[3] = {
      "2d2088560e3ccb5b809c97f5299bb1c0", "2d2088560e3ccb5b809c97f5299bb1c0",
      "2d2088560e3ccb5b809c97f5299bb1c0"};
  static const char* const kDigestsSize2[3] = {
      "db3e785852e98fba18a1fb531f68466c", "8caea330489bc6ed0f99fbf769f53181",
      "bcdd1b21f3baf5f6f29caea9ef93fb0c"};
  static const char* const kDigestsSize5[3] = {
      "326f4193a62f5a959b86d95f5204608e", "4673e453203f75eae97ef44f43f098f2",
      "48d516b06313683aca30e975ce6a3cad"};
  static const char* const kDigestsSize9[3] = {
      "79217575a32e36a51d9dd40621af9c2d", "ccec1c16bc09b28ad6513c5e4c48b6d2",
      "bb61aa9c5fa720c667a053769e7b7d08"};
  static const char* const kDigestsSize17[3] = {
      "46d90e99ba46e89326a5fa547bcd9361", "824aee8950aecb356d5f4a91dbc90a7d",
      "37d44d10a2545385af1da55f8c08564f"};
  static const char* const kDigestsSize33[3] = {
      "c95108e06eb2aef61ecb6839af306edd", "832c695460b4dd2b85c5f8726e4470d1",
      "994902f549eefd83fbcbf7ecb7dc5cca"};
  static const char* const kDigestsSize50[3] = {
      "48119ef1436c3a4fe69d275bbaafedf8", "72c221c91c3df0a324ccbc9acea35f89",
      "84e40aadcc416ef3f51cea3cc23b30c7"};
  static const char* const kDigestsSize55[3] = {
      "6b68e4e0b00c4eb38a6d0d83c0f34658", "43a919f928a80379df5c9e07c9d8000d",
      "7c320d55b11f93185b811bdaa379f2db"};
  static const char* const kDigestsSize65[3] = {
      "c28de89cf9f3bc5a904647ab2c64caf7", "7ce63b1b28dce0624fc7586e8fb3ab8f",
      "d06e6b88585f7f1a1f6af5bb59ee2180"};
  static const char* const kDigestsSize129[3] = {
      "79160902c5c85004382d5ffa549b43cc", "3b0df95c3ca7b0b559b79234cf434738",
      "500786d8561effec283d4f3d13886f8c"};

  switch (size) {
    case 1:
      return kDigestsSize1[strength - 1];
    case 2:
      return kDigestsSize2[strength - 1];
    case 5:
      return kDigestsSize5[strength - 1];
    case 9:
      return kDigestsSize9[strength - 1];
    case 17:
      return kDigestsSize17[strength - 1];
    case 33:
      return kDigestsSize33[strength - 1];
    case 50:
      return kDigestsSize50[strength - 1];
    case 55:
      return kDigestsSize55[strength - 1];
    case 65:
      return kDigestsSize65[strength - 1];
    case 129:
      return kDigestsSize129[strength - 1];
    default:
      ADD_FAILURE() << "Unknown edge size: " << size;
      return nullptr;
  }
}

TEST_P(IntraEdgeFilterTest10bpp, FixedInput) {
  TestFixedValues(GetIntraEdgeFilterDigest10bpp(strength_, size_));
  TestRandomValues(1);
}

TEST_P(IntraEdgeFilterTest10bpp, DISABLED_Speed) { TestRandomValues(1e7); }
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using IntraEdgeFilterTest12bpp = IntraEdgeFilterTest<12, uint16_t>;

const char* GetIntraEdgeFilterDigest12bpp(int strength, int size) {
  return GetIntraEdgeFilterDigest10bpp(strength, size);
}

TEST_P(IntraEdgeFilterTest12bpp, FixedInput) {
  TestFixedValues(GetIntraEdgeFilterDigest12bpp(strength_, size_));
  TestRandomValues(1);
}

TEST_P(IntraEdgeFilterTest12bpp, DISABLED_Speed) { TestRandomValues(1e7); }
#endif  // LIBGAV1_MAX_BITDEPTH == 12

template <int bitdepth, typename Pixel>
class IntraEdgeUpsamplerTest : public testing::TestWithParam<int> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  IntraEdgeUpsamplerTest() = default;
  IntraEdgeUpsamplerTest(const IntraEdgeUpsamplerTest&) = delete;
  IntraEdgeUpsamplerTest& operator=(const IntraEdgeUpsamplerTest&) = delete;
  ~IntraEdgeUpsamplerTest() override = default;

 protected:
  void SetUp() override {
    test_utils::ResetDspTable(bitdepth);
    IntraEdgeInit_C();

    const Dsp* const dsp = GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);
    base_intra_edge_upsampler_ = dsp->intra_edge_upsampler;
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const absl::string_view test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
      base_intra_edge_upsampler_ = nullptr;
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      if ((GetCpuInfo() & kSSE4_1) != 0) {
        IntraEdgeInit_SSE4_1();
      }
    } else if (absl::StartsWith(test_case, "NEON/")) {
      IntraEdgeInit_NEON();
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }
    cur_intra_edge_upsampler_ = dsp->intra_edge_upsampler;
#if LIBGAV1_MSAN
    // Match the behavior of Tile::IntraPrediction to prevent warnings due to
    // assembly code (safely) overreading to fill a register.
    memset(buffer_, 0, sizeof(buffer_));
#endif
  }

  void TestFixedValues(const char* digest);
  void TestRandomValues(int num_runs);

  Pixel buffer_[128];
  Pixel base_buffer_[128];
  int size_ = GetParam();

  IntraEdgeUpsamplerFunc base_intra_edge_upsampler_;
  IntraEdgeUpsamplerFunc cur_intra_edge_upsampler_;
};

template <int bitdepth, typename Pixel>
void IntraEdgeUpsamplerTest<bitdepth, Pixel>::TestFixedValues(
    const char* const digest) {
  if (cur_intra_edge_upsampler_ == nullptr) return;
  buffer_[0] = 0;
  for (int i = 0; i < size_ + 1; ++i) {
    buffer_[i + 1] = kIntraEdgeUpsamplerTestFixedInput[i];
  }
  const absl::Time start = absl::Now();
  cur_intra_edge_upsampler_(buffer_ + 2, size_);
  const absl::Duration elapsed_time = absl::Now() - start;
  test_utils::CheckMd5Digest(kIntraEdge, kIntraEdgeUpsamplerName, digest,
                             buffer_, (size_ * 2 + 1) * sizeof(buffer_[0]),
                             elapsed_time);
}

template <int bitdepth, typename Pixel>
void IntraEdgeUpsamplerTest<bitdepth, Pixel>::TestRandomValues(int num_runs) {
  if (base_intra_edge_upsampler_ == nullptr) return;
  if (cur_intra_edge_upsampler_ == nullptr) return;
  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
  absl::Duration base_elapsed_time;
  absl::Duration elapsed_time;
  for (int num_tests = 0; num_tests < num_runs; ++num_tests) {
    // Populate what will be buffer[-2..size] when passed to the upsample
    // function.
    buffer_[0] = 0;
    base_buffer_[0] = 0;
    for (int i = 1; i < size_ + 2; ++i) {
      const Pixel val = rnd(1 << bitdepth);
      buffer_[i] = val;
      base_buffer_[i] = val;
    }
    const absl::Time base_start = absl::Now();
    base_intra_edge_upsampler_(base_buffer_ + 2, size_);
    base_elapsed_time += absl::Now() - base_start;
    const absl::Time start = absl::Now();
    cur_intra_edge_upsampler_(buffer_ + 2, size_);
    elapsed_time += absl::Now() - start;
  }
  if (num_runs > 1) {
    printf("Mode %s[%31s] size %d C: %5d us SIMD: %5d us %2.2fx\n", kIntraEdge,
           kIntraEdgeUpsamplerName, size_,
           static_cast<int>(absl::ToInt64Microseconds(base_elapsed_time)),
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)),
           absl::ToDoubleMicroseconds(base_elapsed_time) /
               absl::ToDoubleMicroseconds(elapsed_time));
  } else {
    printf("Mode %s[%31s]: size %d \n", kIntraEdge, kIntraEdgeUpsamplerName,
           size_);
  }

  for (int i = 0; i < size_ * 2 + 1; ++i) {
    EXPECT_EQ(buffer_[i], base_buffer_[i]) << "Mismatch in index: " << i;
  }
}

using IntraEdgeUpsamplerTest8bpp = IntraEdgeUpsamplerTest<8, uint8_t>;

constexpr int kIntraEdgeUpsampleSizes[] = {4, 8, 12, 16};

const char* GetIntraEdgeUpsampleDigest8bpp(int size) {
  switch (size) {
    case 4:
      return "aa9002e03f8d15eb26bbee76f40bb923";
    case 8:
      return "cacfca86d65eff0d951eb21fc15f242a";
    case 12:
      return "0529e00a1fa80bc866fa7662ad2d7b9f";
    case 16:
      return "03e3b3e0ea438ea48ef05651c0a54986";
    default:
      ADD_FAILURE() << "Unknown upsample size: " << size;
      return "";
  }
}

TEST_P(IntraEdgeUpsamplerTest8bpp, Correctness) {
  TestFixedValues(GetIntraEdgeUpsampleDigest8bpp(size_));
  TestRandomValues(1);
}

TEST_P(IntraEdgeUpsamplerTest8bpp, DISABLED_Speed) { TestRandomValues(5e7); }

#if LIBGAV1_MAX_BITDEPTH >= 10
using IntraEdgeUpsamplerTest10bpp = IntraEdgeUpsamplerTest<10, uint16_t>;

const char* GetIntraEdgeUpsampleDigest10bpp(int size) {
  switch (size) {
    case 4:
      return "341c6bb705a02bba65b34f92d8ca83cf";
    case 8:
      return "fdbe4b3b341921dcb0edf00dfc4d7667";
    case 12:
      return "ad69a491287495ec9973d4006d5ac461";
    case 16:
      return "04acf32e517d80ce4c4958e711b9b890";
    default:
      ADD_FAILURE() << "Unknown upsample size: " << size;
      return "";
  }
}

TEST_P(IntraEdgeUpsamplerTest10bpp, FixedInput) {
  TestFixedValues(GetIntraEdgeUpsampleDigest10bpp(size_));
  TestRandomValues(1);
}

TEST_P(IntraEdgeUpsamplerTest10bpp, DISABLED_Speed) { TestRandomValues(5e7); }
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using IntraEdgeUpsamplerTest12bpp = IntraEdgeUpsamplerTest<12, uint16_t>;

const char* GetIntraEdgeUpsampleDigest12bpp(int size) {
  return GetIntraEdgeUpsampleDigest10bpp(size);
}

TEST_P(IntraEdgeUpsamplerTest12bpp, FixedInput) {
  TestFixedValues(GetIntraEdgeUpsampleDigest12bpp(size_));
  TestRandomValues(1);
}

TEST_P(IntraEdgeUpsamplerTest12bpp, DISABLED_Speed) { TestRandomValues(5e7); }
#endif  // LIBGAV1_MAX_BITDEPTH == 12

INSTANTIATE_TEST_SUITE_P(C, IntraEdgeFilterTest8bpp,
                         testing::ValuesIn(kIntraEdgeFilterParamList));
#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, IntraEdgeFilterTest8bpp,
                         testing::ValuesIn(kIntraEdgeFilterParamList));
#endif
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, IntraEdgeFilterTest8bpp,
                         testing::ValuesIn(kIntraEdgeFilterParamList));
#endif
INSTANTIATE_TEST_SUITE_P(C, IntraEdgeUpsamplerTest8bpp,
                         testing::ValuesIn(kIntraEdgeUpsampleSizes));
#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, IntraEdgeUpsamplerTest8bpp,
                         testing::ValuesIn(kIntraEdgeUpsampleSizes));
#endif
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, IntraEdgeUpsamplerTest8bpp,
                         testing::ValuesIn(kIntraEdgeUpsampleSizes));
#endif

#if LIBGAV1_MAX_BITDEPTH >= 10
INSTANTIATE_TEST_SUITE_P(C, IntraEdgeFilterTest10bpp,
                         testing::ValuesIn(kIntraEdgeFilterParamList));
INSTANTIATE_TEST_SUITE_P(C, IntraEdgeUpsamplerTest10bpp,
                         testing::ValuesIn(kIntraEdgeUpsampleSizes));

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, IntraEdgeFilterTest10bpp,
                         testing::ValuesIn(kIntraEdgeFilterParamList));
INSTANTIATE_TEST_SUITE_P(NEON, IntraEdgeUpsamplerTest10bpp,
                         testing::ValuesIn(kIntraEdgeUpsampleSizes));
#endif

#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
INSTANTIATE_TEST_SUITE_P(C, IntraEdgeFilterTest12bpp,
                         testing::ValuesIn(kIntraEdgeFilterParamList));
INSTANTIATE_TEST_SUITE_P(C, IntraEdgeUpsamplerTest12bpp,
                         testing::ValuesIn(kIntraEdgeUpsampleSizes));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace
}  // namespace dsp
}  // namespace libgav1
