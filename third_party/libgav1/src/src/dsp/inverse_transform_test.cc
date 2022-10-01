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

#include "src/dsp/inverse_transform.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ostream>

#include "absl/strings/match.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "gtest/gtest.h"
#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/utils/array_2d.h"
#include "src/utils/bit_mask_set.h"
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

const char* const kTransform1dSizeNames[kNumTransform1dSizes] = {
    "kTransform1dSize4", "kTransform1dSize8", "kTransform1dSize16",
    "kTransform1dSize32", "kTransform1dSize64"};

constexpr Transform1dSize kRowTransform1dSizes[] = {
    kTransform1dSize4,  kTransform1dSize4,  kTransform1dSize4,
    kTransform1dSize8,  kTransform1dSize8,  kTransform1dSize8,
    kTransform1dSize8,  kTransform1dSize16, kTransform1dSize16,
    kTransform1dSize16, kTransform1dSize16, kTransform1dSize16,
    kTransform1dSize32, kTransform1dSize32, kTransform1dSize32,
    kTransform1dSize32, kTransform1dSize64, kTransform1dSize64,
    kTransform1dSize64};

constexpr Transform1dSize kColTransform1dSizes[] = {
    kTransform1dSize4,  kTransform1dSize8,  kTransform1dSize16,
    kTransform1dSize4,  kTransform1dSize8,  kTransform1dSize16,
    kTransform1dSize32, kTransform1dSize4,  kTransform1dSize8,
    kTransform1dSize16, kTransform1dSize32, kTransform1dSize64,
    kTransform1dSize8,  kTransform1dSize16, kTransform1dSize32,
    kTransform1dSize64, kTransform1dSize16, kTransform1dSize32,
    kTransform1dSize64};

template <int bitdepth, typename SrcPixel, typename DstPixel>
class InverseTransformTestBase : public testing::TestWithParam<TransformSize>,
                                 public test_utils::MaxAlignedAllocable {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  InverseTransformTestBase() {
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

  InverseTransformTestBase(const InverseTransformTestBase&) = delete;
  InverseTransformTestBase& operator=(const InverseTransformTestBase&) = delete;
  ~InverseTransformTestBase() override = default;

 protected:
  struct InverseTransformMem {
    void Reset(libvpx_test::ACMRandom* rnd, int width, int height) {
      ASSERT_NE(rnd, nullptr);
      // Limit the size of the residual values to bitdepth + sign in order
      // to prevent outranging in the transforms.
      const int num_bits = bitdepth + 1;
      const int sign_shift = (bitdepth == 8 ? 16 : 32) - num_bits;
      const int mask = (1 << num_bits) - 1;
      // Fill residual with random data.  For widths == 64, only fill the upper
      // left 32 x min(block_height_, 32).
      memset(ref_src, 0, sizeof(ref_src));
      SrcPixel* r = ref_src;
      const int stride = width;
      for (int y = 0; y < std::min(height, 32); ++y) {
        for (int x = 0; x < std::min(width, 32); ++x) {
          r[x] = rnd->Rand16() & mask;
          // The msb of num_bits is the sign bit, so force each 16 bit value to
          // the correct sign.
          r[x] = (r[x] << sign_shift) >> sign_shift;
        }
        r += stride;
      }

      // Set frame data to random values.
      for (int y = 0; y < kMaxBlockSize; ++y) {
        for (int x = 0; x < kMaxBlockSize; ++x) {
          const int mask = (1 << bitdepth) - 1;
          cur_frame[y * kMaxBlockSize + x] = base_frame[y * kMaxBlockSize + x] =
              rnd->Rand16() & mask;
        }
      }
    }

    // Set ref_src to |pixel|.
    void Set(const SrcPixel pixel) {
      for (auto& r : ref_src) r = pixel;
    }

    alignas(kMaxAlignment) DstPixel base_frame[kTotalPixels];
    alignas(kMaxAlignment) DstPixel cur_frame[kTotalPixels];

    alignas(kMaxAlignment) SrcPixel base_residual[kTotalPixels];
    alignas(kMaxAlignment) SrcPixel cur_residual[kTotalPixels];

    alignas(kMaxAlignment) SrcPixel ref_src[kTotalPixels];
  };

  void SetUp() override { test_utils::ResetDspTable(bitdepth); }

  const TransformSize tx_size_ = GetParam();
  int block_width_;
  int block_height_;
  InverseTransformMem inverse_transform_mem_;
};

//------------------------------------------------------------------------------
// InverseTransformTest

template <int bitdepth, typename Pixel, typename DstPixel>
class InverseTransformTest
    : public InverseTransformTestBase<bitdepth, Pixel, DstPixel> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  InverseTransformTest() = default;
  InverseTransformTest(const InverseTransformTest&) = delete;
  InverseTransformTest& operator=(const InverseTransformTest&) = delete;
  ~InverseTransformTest() override = default;

 protected:
  using InverseTransformTestBase<bitdepth, Pixel, DstPixel>::tx_size_;
  using InverseTransformTestBase<bitdepth, Pixel, DstPixel>::block_width_;
  using InverseTransformTestBase<bitdepth, Pixel, DstPixel>::block_height_;
  using InverseTransformTestBase<bitdepth, Pixel,
                                 DstPixel>::inverse_transform_mem_;

  void SetUp() override {
    InverseTransformTestBase<bitdepth, Pixel, DstPixel>::SetUp();
    InverseTransformInit_C();

    const Dsp* const dsp = GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);

    tx_size_1d_row_ = kRowTransform1dSizes[tx_size_];
    tx_size_1d_column_ = kColTransform1dSizes[tx_size_];

    memcpy(base_inverse_transforms_, dsp->inverse_transforms,
           sizeof(base_inverse_transforms_));

    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const char* const test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
      memset(base_inverse_transforms_, 0, sizeof(base_inverse_transforms_));
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      if ((GetCpuInfo() & kSSE4_1) != 0) {
        InverseTransformInit_SSE4_1();
      }
    } else if (absl::StartsWith(test_case, "NEON/")) {
      InverseTransformInit_NEON();
      InverseTransformInit10bpp_NEON();
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }

    memcpy(cur_inverse_transforms_, dsp->inverse_transforms,
           sizeof(cur_inverse_transforms_));

    for (int i = 0; i < kNumTransform1ds; ++i) {
      // skip functions that haven't been specialized for this particular
      // architecture.
      if (cur_inverse_transforms_[i][tx_size_1d_row_][kRow] ==
          base_inverse_transforms_[i][tx_size_1d_row_][kRow]) {
        cur_inverse_transforms_[i][tx_size_1d_row_][kRow] = nullptr;
      }
      if (cur_inverse_transforms_[i][tx_size_1d_column_][kColumn] ==
          base_inverse_transforms_[i][tx_size_1d_column_][kColumn]) {
        cur_inverse_transforms_[i][tx_size_1d_column_][kColumn] = nullptr;
      }
    }

    base_frame_buffer_.Reset(kMaxBlockSize, kMaxBlockSize,
                             inverse_transform_mem_.base_frame);

    cur_frame_buffer_.Reset(kMaxBlockSize, kMaxBlockSize,
                            inverse_transform_mem_.cur_frame);
  }

  // These tests modify inverse_transform_mem_.
  void TestRandomValues(int num_tests);
  void TestDcOnlyRandomValue(int num_tests);

  Array2DView<DstPixel> base_frame_buffer_;
  Array2DView<DstPixel> cur_frame_buffer_;

  Transform1dSize tx_size_1d_row_ = kTransform1dSize4;
  Transform1dSize tx_size_1d_column_ = kTransform1dSize4;

  InverseTransformAddFuncs base_inverse_transforms_;
  InverseTransformAddFuncs cur_inverse_transforms_;
};

constexpr TransformType kLibgav1TxType[kNumTransformTypes] = {
    kTransformTypeDctDct,           kTransformTypeAdstDct,
    kTransformTypeDctAdst,          kTransformTypeAdstAdst,
    kTransformTypeFlipadstDct,      kTransformTypeDctFlipadst,
    kTransformTypeFlipadstFlipadst, kTransformTypeAdstFlipadst,
    kTransformTypeFlipadstAdst,     kTransformTypeIdentityIdentity,
    kTransformTypeIdentityDct,      kTransformTypeDctIdentity,
    kTransformTypeIdentityAdst,     kTransformTypeAdstIdentity,
    kTransformTypeIdentityFlipadst, kTransformTypeFlipadstIdentity};

// Maps TransformType to dsp::Transform1d for the row transforms.
constexpr Transform1d kRowTransform[kNumTransformTypes] = {
    kTransform1dDct,      kTransform1dAdst,     kTransform1dDct,
    kTransform1dAdst,     kTransform1dAdst,     kTransform1dDct,
    kTransform1dAdst,     kTransform1dAdst,     kTransform1dAdst,
    kTransform1dIdentity, kTransform1dIdentity, kTransform1dDct,
    kTransform1dIdentity, kTransform1dAdst,     kTransform1dIdentity,
    kTransform1dAdst};

// Maps TransformType to dsp::Transform1d for the column transforms.
constexpr Transform1d kColumnTransform[kNumTransformTypes] = {
    kTransform1dDct,      kTransform1dDct,      kTransform1dAdst,
    kTransform1dAdst,     kTransform1dDct,      kTransform1dAdst,
    kTransform1dAdst,     kTransform1dAdst,     kTransform1dAdst,
    kTransform1dIdentity, kTransform1dDct,      kTransform1dIdentity,
    kTransform1dAdst,     kTransform1dIdentity, kTransform1dAdst,
    kTransform1dIdentity};

// Mask indicating whether the transform sets contain a particular transform
// type. If |tx_type| is present in |tx_set|, then the |tx_type|th LSB is set.
constexpr BitMaskSet kTransformTypeInSetMask[kNumTransformSets] = {
    BitMaskSet(0x1),    BitMaskSet(0xE0F), BitMaskSet(0x20F),
    BitMaskSet(0xFFFF), BitMaskSet(0xFFF), BitMaskSet(0x201)};

bool IsTxSizeTypeValid(TransformSize tx_size, TransformType tx_type) {
  const TransformSize tx_size_square_max = kTransformSizeSquareMax[tx_size];
  TransformSet tx_set;
  if (tx_size_square_max > kTransformSize32x32) {
    tx_set = kTransformSetDctOnly;
  } else if (tx_size_square_max == kTransformSize32x32) {
    tx_set = kTransformSetInter3;
  } else if (tx_size_square_max == kTransformSize16x16) {
    tx_set = kTransformSetInter2;
  } else {
    tx_set = kTransformSetInter1;
  }
  return kTransformTypeInSetMask[tx_set].Contains(tx_type);
}

template <int bitdepth, typename Pixel, typename DstPixel>
void InverseTransformTest<bitdepth, Pixel, DstPixel>::TestRandomValues(
    int num_tests) {
  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());

  for (int tx_type_idx = -1; tx_type_idx < kNumTransformTypes; ++tx_type_idx) {
    const TransformType tx_type = (tx_type_idx == -1)
                                      ? kTransformTypeDctDct
                                      : kLibgav1TxType[tx_type_idx];
    const Transform1d row_transform =
        (tx_type_idx == -1) ? kTransform1dWht : kRowTransform[tx_type];
    const Transform1d column_transform =
        (tx_type_idx == -1) ? kTransform1dWht : kColumnTransform[tx_type];

    // Skip the 'C' test case as this is used as the reference.
    if (base_inverse_transforms_[row_transform][tx_size_1d_row_][kRow] ==
            nullptr ||
        cur_inverse_transforms_[row_transform][tx_size_1d_row_][kRow] ==
            nullptr ||
        base_inverse_transforms_[column_transform][tx_size_1d_column_]
                                [kColumn] == nullptr ||
        cur_inverse_transforms_[column_transform][tx_size_1d_column_]
                               [kColumn] == nullptr) {
      continue;
    }

    // Only test valid tx_size for given tx_type.  See 5.11.40.
    if (!IsTxSizeTypeValid(tx_size_, tx_type)) continue;

    absl::Duration base_elapsed_time[2];
    absl::Duration cur_elapsed_time[2];

    for (int n = 0; n < num_tests; ++n) {
      const int tx_height = std::min(block_height_, 32);
      const int start_x = 0;
      const int start_y = 0;

      inverse_transform_mem_.Reset(&rnd, block_width_, block_height_);
      memcpy(inverse_transform_mem_.base_residual,
             inverse_transform_mem_.ref_src,
             sizeof(inverse_transform_mem_.ref_src));
      memcpy(inverse_transform_mem_.cur_residual,
             inverse_transform_mem_.ref_src,
             sizeof(inverse_transform_mem_.ref_src));

      const absl::Time base_row_start = absl::Now();
      base_inverse_transforms_[row_transform][tx_size_1d_row_][kRow](
          tx_type, tx_size_, tx_height, inverse_transform_mem_.base_residual,
          start_x, start_y, &base_frame_buffer_);
      base_elapsed_time[kRow] += absl::Now() - base_row_start;

      const absl::Time cur_row_start = absl::Now();
      cur_inverse_transforms_[row_transform][tx_size_1d_row_][kRow](
          tx_type, tx_size_, tx_height, inverse_transform_mem_.cur_residual,
          start_x, start_y, &cur_frame_buffer_);
      cur_elapsed_time[kRow] += absl::Now() - cur_row_start;

      const absl::Time base_column_start = absl::Now();
      base_inverse_transforms_[column_transform][tx_size_1d_column_][kColumn](
          tx_type, tx_size_, tx_height, inverse_transform_mem_.base_residual,
          start_x, start_y, &base_frame_buffer_);
      base_elapsed_time[kColumn] += absl::Now() - base_column_start;

      const absl::Time cur_column_start = absl::Now();
      cur_inverse_transforms_[column_transform][tx_size_1d_column_][kColumn](
          tx_type, tx_size_, tx_height, inverse_transform_mem_.cur_residual,
          start_x, start_y, &cur_frame_buffer_);
      cur_elapsed_time[kColumn] += absl::Now() - cur_column_start;

      if (!test_utils::CompareBlocks(inverse_transform_mem_.base_frame,
                                     inverse_transform_mem_.cur_frame,
                                     block_width_, block_height_, kMaxBlockSize,
                                     kMaxBlockSize, false)) {
        ADD_FAILURE() << "Result from optimized version of "
                      << ToString(
                             static_cast<Transform1dSize>(tx_size_1d_column_))
                      << " differs from reference in iteration #" << n
                      << " tx_type_idx:" << tx_type_idx;
        break;
      }
    }

    if (num_tests > 1) {
      const auto base_row_elapsed_time_us =
          static_cast<int>(absl::ToInt64Microseconds(base_elapsed_time[kRow]));
      const auto cur_row_elapsed_time_us =
          static_cast<int>(absl::ToInt64Microseconds(cur_elapsed_time[kRow]));
      printf("TxType %30s[%19s]:: base_row: %5d us  cur_row: %5d us  %2.2fx \n",
             (tx_type_idx == -1) ? ToString(row_transform) : ToString(tx_type),
             kTransform1dSizeNames[tx_size_1d_row_], base_row_elapsed_time_us,
             cur_row_elapsed_time_us,
             static_cast<float>(base_row_elapsed_time_us) /
                 static_cast<float>(cur_row_elapsed_time_us));
      const auto base_column_elapsed_time_us = static_cast<int>(
          absl::ToInt64Microseconds(base_elapsed_time[kColumn]));
      const auto cur_column_elapsed_time_us = static_cast<int>(
          absl::ToInt64Microseconds(cur_elapsed_time[kColumn]));
      printf(
          "TxType %30s[%19s]:: base_col: %5d us  cur_col: %5d us  %2.2fx \n",
          (tx_type_idx == -1) ? ToString(column_transform) : ToString(tx_type),
          kTransform1dSizeNames[tx_size_1d_column_],
          base_column_elapsed_time_us, cur_column_elapsed_time_us,
          static_cast<float>(base_column_elapsed_time_us) /
              static_cast<float>(cur_column_elapsed_time_us));
    }
  }
}

template <int bitdepth, typename Pixel, typename DstPixel>
void InverseTransformTest<bitdepth, Pixel, DstPixel>::TestDcOnlyRandomValue(
    int num_tests) {
  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());

  for (int tx_type_idx = 0; tx_type_idx < kNumTransformTypes; ++tx_type_idx) {
    const TransformType tx_type = kLibgav1TxType[tx_type_idx];
    const Transform1d row_transform = kRowTransform[tx_type];
    const Transform1d column_transform = kColumnTransform[tx_type];

    if (cur_inverse_transforms_[row_transform][tx_size_1d_row_][kRow] ==
            nullptr ||
        cur_inverse_transforms_[column_transform][tx_size_1d_column_]
                               [kColumn] == nullptr) {
      continue;
    }

    // Only test valid tx_size for given tx_type.  See 5.11.40.
    if (IsTxSizeTypeValid(tx_size_, tx_type) == 0) continue;

    absl::Duration base_elapsed_time[2];
    absl::Duration cur_elapsed_time[2];

    for (int n = 0; n < num_tests; ++n) {
      const int tx_height = std::min(block_height_, 32);
      const int start_x = 0;
      const int start_y = 0;

      // Using width == 1 and height == 1 will reset only the dc value.
      inverse_transform_mem_.Reset(&rnd, 1, 1);
      memcpy(inverse_transform_mem_.base_residual,
             inverse_transform_mem_.ref_src,
             sizeof(inverse_transform_mem_.ref_src));
      memcpy(inverse_transform_mem_.cur_residual,
             inverse_transform_mem_.ref_src,
             sizeof(inverse_transform_mem_.ref_src));

      // For this test, the "base" contains the output when the
      // tx_height is set to the max for the given block size.  The
      // "cur" contains the output when the passed in tx_height is 1.
      // Compare the outputs for match.
      const absl::Time base_row_start = absl::Now();
      cur_inverse_transforms_[row_transform][tx_size_1d_row_][kRow](
          tx_type, tx_size_, tx_height, inverse_transform_mem_.base_residual,
          start_x, start_y, &base_frame_buffer_);
      base_elapsed_time[kRow] += absl::Now() - base_row_start;

      const absl::Time cur_row_start = absl::Now();
      cur_inverse_transforms_[row_transform][tx_size_1d_row_][kRow](
          tx_type, tx_size_, /*adjusted_tx_height=*/1,
          inverse_transform_mem_.cur_residual, start_x, start_y,
          &cur_frame_buffer_);
      cur_elapsed_time[kRow] += absl::Now() - cur_row_start;

      const absl::Time base_column_start = absl::Now();
      cur_inverse_transforms_[column_transform][tx_size_1d_column_][kColumn](
          tx_type, tx_size_, tx_height, inverse_transform_mem_.base_residual,
          start_x, start_y, &base_frame_buffer_);
      base_elapsed_time[kColumn] += absl::Now() - base_column_start;

      const absl::Time cur_column_start = absl::Now();
      cur_inverse_transforms_[column_transform][tx_size_1d_column_][kColumn](
          tx_type, tx_size_, /*adjusted_tx_height=*/1,
          inverse_transform_mem_.cur_residual, start_x, start_y,
          &cur_frame_buffer_);
      cur_elapsed_time[kColumn] += absl::Now() - cur_column_start;

      if (!test_utils::CompareBlocks(inverse_transform_mem_.base_frame,
                                     inverse_transform_mem_.cur_frame,
                                     block_width_, block_height_, kMaxBlockSize,
                                     kMaxBlockSize, false)) {
        ADD_FAILURE() << "Result from dc only version of "
                      << ToString(
                             static_cast<Transform1dSize>(tx_size_1d_column_))
                      << " differs from reference in iteration #" << n
                      << "tx_type_idx:" << tx_type_idx;
        break;
      }
    }

    if (num_tests > 1) {
      const auto base_row_elapsed_time_us =
          static_cast<int>(absl::ToInt64Microseconds(base_elapsed_time[kRow]));
      const auto cur_row_elapsed_time_us =
          static_cast<int>(absl::ToInt64Microseconds(cur_elapsed_time[kRow]));
      printf("TxType %30s[%19s]:: base_row: %5d us  cur_row: %5d us  %2.2fx \n",
             ToString(tx_type), kTransform1dSizeNames[tx_size_1d_row_],
             base_row_elapsed_time_us, cur_row_elapsed_time_us,
             static_cast<float>(base_row_elapsed_time_us) /
                 static_cast<float>(cur_row_elapsed_time_us));
      const auto base_column_elapsed_time_us = static_cast<int>(
          absl::ToInt64Microseconds(base_elapsed_time[kColumn]));
      const auto cur_column_elapsed_time_us = static_cast<int>(
          absl::ToInt64Microseconds(cur_elapsed_time[kColumn]));
      printf("TxType %30s[%19s]:: base_col: %5d us  cur_col: %5d us  %2.2fx \n",
             ToString(tx_type), kTransform1dSizeNames[tx_size_1d_column_],
             base_column_elapsed_time_us, cur_column_elapsed_time_us,
             static_cast<float>(base_column_elapsed_time_us) /
                 static_cast<float>(cur_column_elapsed_time_us));
    }
  }
}

using InverseTransformTest8bpp = InverseTransformTest<8, int16_t, uint8_t>;

TEST_P(InverseTransformTest8bpp, Random) { TestRandomValues(1); }

TEST_P(InverseTransformTest8bpp, DISABLED_Speed) { TestRandomValues(10000); }

TEST_P(InverseTransformTest8bpp, DcRandom) { TestDcOnlyRandomValue(1); }

constexpr TransformSize kTransformSizesAll[] = {
    kTransformSize4x4,   kTransformSize4x8,   kTransformSize4x16,
    kTransformSize8x4,   kTransformSize8x8,   kTransformSize8x16,
    kTransformSize8x32,  kTransformSize16x4,  kTransformSize16x8,
    kTransformSize16x16, kTransformSize16x32, kTransformSize16x64,
    kTransformSize32x8,  kTransformSize32x16, kTransformSize32x32,
    kTransformSize32x64, kTransformSize64x16, kTransformSize64x32,
    kTransformSize64x64};

INSTANTIATE_TEST_SUITE_P(C, InverseTransformTest8bpp,
                         testing::ValuesIn(kTransformSizesAll));
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, InverseTransformTest8bpp,
                         testing::ValuesIn(kTransformSizesAll));
#endif
#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, InverseTransformTest8bpp,
                         testing::ValuesIn(kTransformSizesAll));
#endif

#if LIBGAV1_MAX_BITDEPTH >= 10
using InverseTransformTest10bpp = InverseTransformTest<10, int32_t, uint16_t>;

TEST_P(InverseTransformTest10bpp, Random) { TestRandomValues(1); }

TEST_P(InverseTransformTest10bpp, DISABLED_Speed) { TestRandomValues(10000); }

TEST_P(InverseTransformTest10bpp, DcRandom) { TestDcOnlyRandomValue(1); }

INSTANTIATE_TEST_SUITE_P(C, InverseTransformTest10bpp,
                         testing::ValuesIn(kTransformSizesAll));

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, InverseTransformTest10bpp,
                         testing::ValuesIn(kTransformSizesAll));
#endif
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using InverseTransformTest12bpp = InverseTransformTest<12, int32_t, uint16_t>;

TEST_P(InverseTransformTest12bpp, Random) { TestRandomValues(1); }

TEST_P(InverseTransformTest12bpp, DISABLED_Speed) { TestRandomValues(12000); }

TEST_P(InverseTransformTest12bpp, DcRandom) { TestDcOnlyRandomValue(1); }

INSTANTIATE_TEST_SUITE_P(C, InverseTransformTest12bpp,
                         testing::ValuesIn(kTransformSizesAll));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace
}  // namespace dsp

static std::ostream& operator<<(std::ostream& os, const TransformSize param) {
  return os << ToString(param);
}

}  // namespace libgav1
