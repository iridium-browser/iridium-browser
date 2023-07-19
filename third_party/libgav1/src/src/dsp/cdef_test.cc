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

#include "src/dsp/cdef.h"

#include <cstdint>
#include <cstring>
#include <ostream>

#include "absl/strings/match.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "gtest/gtest.h"
#include "src/dsp/dsp.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/cpu.h"
#include "src/utils/memory.h"
#include "tests/third_party/libvpx/acm_random.h"
#include "tests/third_party/libvpx/md5_helper.h"
#include "tests/utils.h"

namespace libgav1 {
namespace dsp {
namespace {

constexpr char kCdef[] = "Cdef";
constexpr char kCdefDirectionName[] = "Cdef Direction";
constexpr char kCdefFilterName[] = "Cdef Filtering";
constexpr int kTestBufferStride = 8;
constexpr int kTestBufferSize = 64;
constexpr int kSourceStride = kMaxSuperBlockSizeInPixels + 2 * 8;
constexpr int kSourceBufferSize =
    (kMaxSuperBlockSizeInPixels + 2 * 3) * kSourceStride;
constexpr int kNumSpeedTests = 5000;

const char* GetDirectionDigest(const int bitdepth, const int num_runs) {
  static const char* const kDigest[3][2] = {
      {"de78c820a1fec7e81385aa0a615dbf8c", "7bfc543244f932a542691480dc4541b2"},
      {"b54236de5d25e16c0f8678d9784cb85e", "559144cf183f3c69cb0e5d98cbf532ff"},
      {"5532919a157c4f937da9e822bdb105f7", "dd9dfca6dfca83777d942e693c17627a"}};
  const int bitdepth_index = (bitdepth - 8) / 2;
  const int run_index = (num_runs == 1) ? 0 : 1;
  return kDigest[bitdepth_index][run_index];
}

// The 'int' parameter is unused but required to allow for instantiations of C,
// NEON, etc.
template <int bitdepth, typename Pixel>
class CdefDirectionTest : public testing::TestWithParam<int> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  CdefDirectionTest() = default;
  CdefDirectionTest(const CdefDirectionTest&) = delete;
  CdefDirectionTest& operator=(const CdefDirectionTest&) = delete;
  ~CdefDirectionTest() override = default;

 protected:
  void SetUp() override {
    test_utils::ResetDspTable(bitdepth);
    CdefInit_C();

    const Dsp* const dsp = GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);
    base_cdef_direction_ = nullptr;
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const char* const test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      CdefInit_SSE4_1();
    } else if (absl::StartsWith(test_case, "AVX2/")) {
      if ((GetCpuInfo() & kAVX2) != 0) {
        CdefInit_AVX2();
      }
    } else if (absl::StartsWith(test_case, "NEON/")) {
      CdefInit_NEON();
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }
    cur_cdef_direction_ = dsp->cdef_direction;
  }

  void TestRandomValues(int num_runs);

  Pixel buffer_[kTestBufferSize];
  int strength_;
  int size_;

  CdefDirectionFunc base_cdef_direction_;
  CdefDirectionFunc cur_cdef_direction_;
};

template <int bitdepth, typename Pixel>
void CdefDirectionTest<bitdepth, Pixel>::TestRandomValues(int num_runs) {
  if (cur_cdef_direction_ == nullptr) return;
  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
  absl::Duration elapsed_time;
  libvpx_test::MD5 actual_digest;
  for (int num_tests = 0; num_tests < num_runs; ++num_tests) {
    for (int level = 0; level < (1 << bitdepth); level += 1 + (bitdepth - 8)) {
      for (int bits = 0; bits <= bitdepth; ++bits) {
        for (auto& pixel : buffer_) {
          pixel = Clip3((rnd.Rand16() & ((1 << bits) - 1)) + level, 0,
                        (1 << bitdepth) - 1);
        }
        int output[2] = {};
        const absl::Time start = absl::Now();
        cur_cdef_direction_(buffer_, kTestBufferStride * sizeof(Pixel),
                            reinterpret_cast<uint8_t*>(&output[0]), &output[1]);
        elapsed_time += absl::Now() - start;
        actual_digest.Add(reinterpret_cast<const uint8_t*>(output),
                          sizeof(output));
      }
    }
  }
  test_utils::CheckMd5Digest(kCdef, kCdefDirectionName,
                             GetDirectionDigest(bitdepth, num_runs),
                             actual_digest.Get(), elapsed_time);
}

using CdefDirectionTest8bpp = CdefDirectionTest<8, uint8_t>;

TEST_P(CdefDirectionTest8bpp, Correctness) { TestRandomValues(1); }

TEST_P(CdefDirectionTest8bpp, DISABLED_Speed) {
  TestRandomValues(kNumSpeedTests / 100);
}

INSTANTIATE_TEST_SUITE_P(C, CdefDirectionTest8bpp, testing::Values(0));

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, CdefDirectionTest8bpp, testing::Values(0));
#endif

#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, CdefDirectionTest8bpp, testing::Values(0));
#endif

#if LIBGAV1_ENABLE_AVX2
INSTANTIATE_TEST_SUITE_P(AVX2, CdefDirectionTest8bpp, testing::Values(0));
#endif  // LIBGAV1_ENABLE_AVX2

#if LIBGAV1_MAX_BITDEPTH >= 10
using CdefDirectionTest10bpp = CdefDirectionTest<10, uint16_t>;

TEST_P(CdefDirectionTest10bpp, Correctness) { TestRandomValues(1); }

TEST_P(CdefDirectionTest10bpp, DISABLED_Speed) {
  TestRandomValues(kNumSpeedTests / 100);
}

INSTANTIATE_TEST_SUITE_P(C, CdefDirectionTest10bpp, testing::Values(0));

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, CdefDirectionTest10bpp, testing::Values(0));
#endif
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using CdefDirectionTest12bpp = CdefDirectionTest<12, uint16_t>;

TEST_P(CdefDirectionTest12bpp, Correctness) { TestRandomValues(1); }

TEST_P(CdefDirectionTest12bpp, DISABLED_Speed) {
  TestRandomValues(kNumSpeedTests / 100);
}

INSTANTIATE_TEST_SUITE_P(C, CdefDirectionTest12bpp, testing::Values(0));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

const char* GetDigest8bpp(int id) {
  static const char* const kDigest[] = {
      "b6fe1a1f5bbb23e35197160ce57d90bd", "8aed39871b19184f1d381b145779bc33",
      "82653dd66072e8ebd967083a0413ab03", "421c048396bc66ffaa6aafa016c7bc54",
      "1f70ba51091e8c6034c3f0974af241c3", "8f700997452a24091136ca58890a5be4",
      "9e3dea21ee4246172121f0420eccd899", "0848bdeffa74145758ef47992e1035c4",
      "0bb55818de986e9d988b0c1cc6883887", "9b558a7eefc934f90cd09ca26b998bfd",
      "3a38670f8c5f0c61cc47c9c79da728d2", "ed18fe91180e78008ccb98e9019bed69",
      "2aa4bbcb6fb088ad42bde76be014dff0", "88f746f0d6c079ab8e9ecc7ff67524c7",
      "7cffa948f5ddbccc7c6b07d15ca9eb69", "5e22c1c89735965dda935d1249129548",
      "e765133d133b94e1578c8c5616248a96", "da95d47cad74eb4a075893ca98e658ab",
  };
  return kDigest[id];
}

#if LIBGAV1_MAX_BITDEPTH >= 10
const char* GetDigest10bpp(int id) {
  static const char* const kDigest[] = {
      "0a9630b39974850998db653b07e09ab4", "97a924661d931b23ee57893da617ae70",
      "0d79516b9a491ce5112eb00bbae5eb80", "d5801fd96029a7509cf66dde61e8e2d8",
      "5bf5c0ea5a85e9b6c1e6991619c34ebc", "e2f1c08a8b3cd93b3a85511493a0ee31",
      "45c047d2be5e2dcf6094937780a3f88a", "346caf437c1ad85862de72a622e29845",
      "0e9cb69d24d9badbe956da779d912b05", "81803dcb00971237b3fe6372564a842f",
      "17681ad2ed4a2456d70760852af6c6fd", "5312f8049a08a5f9b1708fda936f7a55",
      "3f0f522f3a33e4ff2a97bdc1e614c5c4", "3818a50be7fe16aa0c636a7392d1eceb",
      "c6849b8cd77a076dc7e3c26e8cd55b9e", "223c0dd685bbc74aec1d088356708433",
      "90992957cb8103222aa2fb43c6cd2fc4", "a4ba6edcefe4130851c4c2607b147f95",
  };
  return kDigest[id];
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
const char* GetDigest12bpp(int id) {
  static const char* const kDigest[] = {
      "a32569989c42fd4254979f70c1c65f5a", "dc389048217633e2dd64126376be7d25",
      "3b0e8dae294895330f349863b1773c39", "9741fe8d27d109cb99b7a9cdc030f52a",
      "ab70f3729b52287c6432ba7624280a68", "c1e5cf39cbc8030b82e09633c6c67d42",
      "d5120a196164ff5a0ad7aa8c02e9b064", "1133759f3aee3a362a0ab668f6faf843",
      "feb0ab7f515665f79fce213e8cd2fb10", "e86ea55c2d6d5cc69716535bd455c99f",
      "e463da1b9d089b6ee82c041794257fd7", "27800e4af0cceeaf0a95c96275a7befe",
      "f42e426481db00582b327eb2971bca96", "6127ff289833dde0270000d8240f36b7",
      "cc5dbaf70e2fef7729a8e2ea9937fbcf", "51850b4e3e2a3919e110376fcb6318d3",
      "d5ac7ac25eb1b5aee293b2a2ec9de775", "64ecc00b2e24a2f07df833fb50ce09c3",
  };
  return kDigest[id];
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

struct CdefTestParam {
  CdefTestParam(int subsampling_x, int subsampling_y, int rows4x4,
                int columns4x4)
      : subsampling_x(subsampling_x),
        subsampling_y(subsampling_y),
        rows4x4(rows4x4),
        columns4x4(columns4x4) {}
  int subsampling_x;
  int subsampling_y;
  int rows4x4;
  int columns4x4;
};

std::ostream& operator<<(std::ostream& os, const CdefTestParam& param) {
  return os << "subsampling(x/y): " << param.subsampling_x << "/"
            << param.subsampling_y << ", (rows,columns)4x4: " << param.rows4x4
            << ", " << param.columns4x4;
}

// TODO(b/154245961): rework the parameters for this test to match
// CdefFilteringFuncs. It should cover 4x4, 8x4, 8x8 blocks and
// primary/secondary strength combinations for both Y and UV.
template <int bitdepth, typename Pixel>
class CdefFilteringTest : public testing::TestWithParam<CdefTestParam> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  CdefFilteringTest() = default;
  CdefFilteringTest(const CdefFilteringTest&) = delete;
  CdefFilteringTest& operator=(const CdefFilteringTest&) = delete;
  ~CdefFilteringTest() override = default;

 protected:
  void SetUp() override {
    test_utils::ResetDspTable(bitdepth);
    CdefInit_C();

    const Dsp* const dsp = GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const char* const test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
    } else if (absl::StartsWith(test_case, "NEON/")) {
      CdefInit_NEON();
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      CdefInit_SSE4_1();
    } else if (absl::StartsWith(test_case, "AVX2/")) {
      if ((GetCpuInfo() & kAVX2) != 0) {
        CdefInit_AVX2();
      }
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }
    memcpy(cur_cdef_filter_, dsp->cdef_filters, sizeof(cur_cdef_filter_));
  }

  void TestRandomValues(int num_runs);

  uint16_t source_[kSourceBufferSize];
  Pixel dest_[kMaxPlanes][kTestBufferSize];
  int primary_strength_;
  int secondary_strength_;
  int damping_;
  int direction_;
  CdefTestParam param_ = GetParam();

  CdefFilteringFuncs cur_cdef_filter_;
};

template <int bitdepth, typename Pixel>
void CdefFilteringTest<bitdepth, Pixel>::TestRandomValues(int num_runs) {
  const int id = static_cast<int>(param_.rows4x4 < 4) * 3 +
                 (param_.subsampling_x + param_.subsampling_y) * 6;
  absl::Duration elapsed_time;
  for (int num_tests = 0; num_tests < num_runs; ++num_tests) {
    for (int plane = kPlaneY; plane < kMaxPlanes; ++plane) {
      const int subsampling_x = (plane == kPlaneY) ? 0 : param_.subsampling_x;
      const int subsampling_y = (plane == kPlaneY) ? 0 : param_.subsampling_y;
      const int block_width = 8 >> subsampling_x;
      const int block_height = 8 >> subsampling_y;
      libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed() +
                                 id + plane);
      const int offset = 2 * kSourceStride + 2;
      // Fill boundaries with a large value such that cdef does not take them
      // into calculation.
      const int plane_width = MultiplyBy4(param_.columns4x4) >> subsampling_x;
      const int plane_height = MultiplyBy4(param_.rows4x4) >> subsampling_y;
      for (int y = 0; y < plane_height; ++y) {
        for (int x = 0; x < plane_width; ++x) {
          source_[y * kSourceStride + x + offset] =
              rnd.Rand16() & ((1 << bitdepth) - 1);
        }
      }
      for (int y = 0; y < 2; ++y) {
        Memset(&source_[y * kSourceStride], kCdefLargeValue, kSourceStride);
        Memset(&source_[(y + plane_height + 2) * kSourceStride],
               kCdefLargeValue, kSourceStride);
      }
      for (int y = 0; y < plane_height; ++y) {
        Memset(&source_[y * kSourceStride + offset - 2], kCdefLargeValue, 2);
        Memset(&source_[y * kSourceStride + offset + plane_width],
               kCdefLargeValue, 2);
      }
      do {
        int strength = rnd.Rand16() & 15;
        if (strength == 3) ++strength;
        primary_strength_ = strength << (bitdepth - 8);
      } while (primary_strength_ == 0);
      do {
        int strength = rnd.Rand16() & 3;
        if (strength == 3) ++strength;
        secondary_strength_ = strength << (bitdepth - 8);
      } while (secondary_strength_ == 0);
      damping_ = (rnd.Rand16() & 3) + 3;
      direction_ = (rnd.Rand16() & 7);

      memset(dest_[plane], 0, sizeof(dest_[plane]));
      const absl::Time start = absl::Now();
      const int width_index = block_width >> 3;
      if (cur_cdef_filter_[width_index][0] == nullptr) return;
      cur_cdef_filter_[width_index][0](
          source_ + offset, kSourceStride, block_height, primary_strength_,
          secondary_strength_, damping_, direction_, dest_[plane],
          kTestBufferStride * sizeof(dest_[0][0]));
      elapsed_time += absl::Now() - start;
    }
  }

  for (int plane = kPlaneY; plane < kMaxPlanes; ++plane) {
    const char* expected_digest = nullptr;
    switch (bitdepth) {
      case 8:
        expected_digest = GetDigest8bpp(id + plane);
        break;
#if LIBGAV1_MAX_BITDEPTH >= 10
      case 10:
        expected_digest = GetDigest10bpp(id + plane);
        break;
#endif
#if LIBGAV1_MAX_BITDEPTH == 12
      case 12:
        expected_digest = GetDigest12bpp(id + plane);
        break;
#endif
    }
    ASSERT_NE(expected_digest, nullptr);
    test_utils::CheckMd5Digest(kCdef, kCdefFilterName, expected_digest,
                               reinterpret_cast<uint8_t*>(dest_[plane]),
                               sizeof(dest_[plane]), elapsed_time);
  }
}

// Do not test single blocks with any subsampling. 2xH and Wx2 blocks are not
// supported.
const CdefTestParam cdef_test_param[] = {
    CdefTestParam(0, 0, 4, 4), CdefTestParam(0, 0, 2, 2),
    CdefTestParam(1, 0, 4, 4), CdefTestParam(1, 0, 2, 2),
    CdefTestParam(1, 1, 4, 4), CdefTestParam(1, 1, 2, 2),
};

using CdefFilteringTest8bpp = CdefFilteringTest<8, uint8_t>;

TEST_P(CdefFilteringTest8bpp, Correctness) { TestRandomValues(1); }

TEST_P(CdefFilteringTest8bpp, DISABLED_Speed) {
  TestRandomValues(kNumSpeedTests);
}

INSTANTIATE_TEST_SUITE_P(C, CdefFilteringTest8bpp,
                         testing::ValuesIn(cdef_test_param));

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, CdefFilteringTest8bpp,
                         testing::ValuesIn(cdef_test_param));
#endif

#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, CdefFilteringTest8bpp,
                         testing::ValuesIn(cdef_test_param));
#endif

#if LIBGAV1_ENABLE_AVX2
INSTANTIATE_TEST_SUITE_P(AVX2, CdefFilteringTest8bpp,
                         testing::ValuesIn(cdef_test_param));
#endif  // LIBGAV1_ENABLE_AVX2

#if LIBGAV1_MAX_BITDEPTH >= 10
using CdefFilteringTest10bpp = CdefFilteringTest<10, uint16_t>;

TEST_P(CdefFilteringTest10bpp, Correctness) { TestRandomValues(1); }

TEST_P(CdefFilteringTest10bpp, DISABLED_Speed) {
  TestRandomValues(kNumSpeedTests);
}

INSTANTIATE_TEST_SUITE_P(C, CdefFilteringTest10bpp,
                         testing::ValuesIn(cdef_test_param));

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, CdefFilteringTest10bpp,
                         testing::ValuesIn(cdef_test_param));
#endif
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using CdefFilteringTest12bpp = CdefFilteringTest<12, uint16_t>;

TEST_P(CdefFilteringTest12bpp, Correctness) { TestRandomValues(1); }

TEST_P(CdefFilteringTest12bpp, DISABLED_Speed) {
  TestRandomValues(kNumSpeedTests);
}

INSTANTIATE_TEST_SUITE_P(C, CdefFilteringTest12bpp,
                         testing::ValuesIn(cdef_test_param));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace
}  // namespace dsp
}  // namespace libgav1
