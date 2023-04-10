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

#include "src/dsp/intrapred_cfl.h"

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
#include "src/utils/compiler_attributes.h"
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

const char* const kCflIntraPredName = "kCflIntraPredictor";

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
// CflIntraPredTest

template <int bitdepth, typename Pixel>
class CflIntraPredTest : public IntraPredTestBase<bitdepth, Pixel> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  CflIntraPredTest() = default;
  CflIntraPredTest(const CflIntraPredTest&) = delete;
  CflIntraPredTest& operator=(const CflIntraPredTest&) = delete;
  ~CflIntraPredTest() override = default;

 protected:
  using IntraPredTestBase<bitdepth, Pixel>::tx_size_;
  using IntraPredTestBase<bitdepth, Pixel>::block_width_;
  using IntraPredTestBase<bitdepth, Pixel>::block_height_;
  using IntraPredTestBase<bitdepth, Pixel>::intra_pred_mem_;

  void SetUp() override {
    IntraPredTestBase<bitdepth, Pixel>::SetUp();
    IntraPredCflInit_C();

    const Dsp* const dsp = GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);
    base_cfl_intra_pred_ = dsp->cfl_intra_predictors[tx_size_];

    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const char* const test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
      base_cfl_intra_pred_ = nullptr;
    } else if (absl::StartsWith(test_case, "NEON/")) {
      IntraPredCflInit_NEON();
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      if ((GetCpuInfo() & kSSE4_1) != 0) {
        IntraPredCflInit_SSE4_1();
      }
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }

    cur_cfl_intra_pred_ = dsp->cfl_intra_predictors[tx_size_];

    if (cur_cfl_intra_pred_ == base_cfl_intra_pred_) {
      cur_cfl_intra_pred_ = nullptr;
    }
  }

  // This test modifies intra_pred_mem_.
  void TestSpeed(const char* digest, int num_runs);
  void TestSaturatedValues();
  void TestRandomValues();

  CflIntraPredictorFunc base_cfl_intra_pred_;
  CflIntraPredictorFunc cur_cfl_intra_pred_;
};

template <int bitdepth, typename Pixel>
void CflIntraPredTest<bitdepth, Pixel>::TestSpeed(const char* const digest,
                                                  const int num_runs) {
  if (cur_cfl_intra_pred_ == nullptr) return;
  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
  int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride] = {};
  const int alpha = rnd(33) - 16;
  const int dc = rnd(1 << bitdepth);
  const int max_luma = ((1 << bitdepth) - 1) << 3;
  for (int i = 0; i < block_height_; ++i) {
    for (int j = 0; j < block_width_; ++j) {
      if (i < kCflLumaBufferStride && j < kCflLumaBufferStride) {
        luma[i][j] = max_luma - rnd(max_luma << 1);
      }
    }
  }
  for (auto& r : intra_pred_mem_.ref_src) r = dc;

  absl::Duration elapsed_time;
  for (int run = 0; run < num_runs; ++run) {
    const ptrdiff_t stride = kMaxBlockSize * sizeof(Pixel);
    memcpy(intra_pred_mem_.dst, intra_pred_mem_.ref_src,
           sizeof(intra_pred_mem_.dst));
    const absl::Time start = absl::Now();
    cur_cfl_intra_pred_(intra_pred_mem_.dst, stride, luma, alpha);
    elapsed_time += absl::Now() - start;
  }
  test_utils::CheckMd5Digest(ToString(tx_size_), kCflIntraPredName, digest,
                             intra_pred_mem_.dst, sizeof(intra_pred_mem_.dst),
                             elapsed_time);
}

template <int bitdepth, typename Pixel>
void CflIntraPredTest<bitdepth, Pixel>::TestSaturatedValues() {
  // Skip the 'C' test case as this is used as the reference.
  if (base_cfl_intra_pred_ == nullptr) return;

  int16_t luma_buffer[kCflLumaBufferStride][kCflLumaBufferStride];
  for (auto& line : luma_buffer) {
    for (auto& luma : line) luma = ((1 << bitdepth) - 1) << 3;
  }

  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
  static constexpr int kSaturatedAlpha[] = {-16, 16};
  for (const int alpha : kSaturatedAlpha) {
    for (auto& r : intra_pred_mem_.ref_src) r = (1 << bitdepth) - 1;
    memcpy(intra_pred_mem_.dst, intra_pred_mem_.ref_src,
           sizeof(intra_pred_mem_.dst));
    const ptrdiff_t stride = kMaxBlockSize * sizeof(Pixel);
    base_cfl_intra_pred_(intra_pred_mem_.ref_src, stride, luma_buffer, alpha);
    cur_cfl_intra_pred_(intra_pred_mem_.dst, stride, luma_buffer, alpha);
    if (!test_utils::CompareBlocks(intra_pred_mem_.dst, intra_pred_mem_.ref_src,
                                   block_width_, block_height_, kMaxBlockSize,
                                   kMaxBlockSize, true)) {
      ADD_FAILURE() << "Result from optimized version of CFL with alpha "
                    << alpha << " differs from reference.";
      break;
    }
  }
}

template <int bitdepth, typename Pixel>
void CflIntraPredTest<bitdepth, Pixel>::TestRandomValues() {
  // Skip the 'C' test case as this is used as the reference.
  if (base_cfl_intra_pred_ == nullptr) return;
  int16_t luma_buffer[kCflLumaBufferStride][kCflLumaBufferStride];

  const int max_luma = ((1 << bitdepth) - 1) << 3;
  // Use an alternate seed to differentiate this test from TestSpeed().
  libvpx_test::ACMRandom rnd(test_utils::kAlternateDeterministicSeed);
  for (auto& line : luma_buffer) {
    for (auto& luma : line) luma = max_luma - rnd(max_luma << 1);
  }
  const int dc = rnd(1 << bitdepth);
  for (auto& r : intra_pred_mem_.ref_src) r = dc;
  static constexpr int kSaturatedAlpha[] = {-16, 16};
  for (const int alpha : kSaturatedAlpha) {
    intra_pred_mem_.Reset(&rnd);
    memcpy(intra_pred_mem_.dst, intra_pred_mem_.ref_src,
           sizeof(intra_pred_mem_.dst));
    const ptrdiff_t stride = kMaxBlockSize * sizeof(Pixel);
    base_cfl_intra_pred_(intra_pred_mem_.ref_src, stride, luma_buffer, alpha);
    cur_cfl_intra_pred_(intra_pred_mem_.dst, stride, luma_buffer, alpha);
    if (!test_utils::CompareBlocks(intra_pred_mem_.dst, intra_pred_mem_.ref_src,
                                   block_width_, block_height_, kMaxBlockSize,
                                   kMaxBlockSize, true)) {
      ADD_FAILURE() << "Result from optimized version of CFL with alpha "
                    << alpha << " differs from reference.";
      break;
    }
  }
}

template <int bitdepth, typename Pixel, SubsamplingType subsampling_type>
class CflSubsamplerTest : public IntraPredTestBase<bitdepth, Pixel> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  CflSubsamplerTest() = default;
  CflSubsamplerTest(const CflSubsamplerTest&) = delete;
  CflSubsamplerTest& operator=(const CflSubsamplerTest&) = delete;
  ~CflSubsamplerTest() override = default;

 protected:
  using IntraPredTestBase<bitdepth, Pixel>::tx_size_;
  using IntraPredTestBase<bitdepth, Pixel>::block_width_;
  using IntraPredTestBase<bitdepth, Pixel>::block_height_;
  using IntraPredTestBase<bitdepth, Pixel>::intra_pred_mem_;

  void SetUp() override {
    IntraPredTestBase<bitdepth, Pixel>::SetUp();
    IntraPredCflInit_C();

    const Dsp* const dsp = GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);
    base_cfl_subsampler_ = dsp->cfl_subsamplers[tx_size_][subsampling_type];

    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const char* const test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
      base_cfl_subsampler_ = nullptr;
    } else if (absl::StartsWith(test_case, "NEON/")) {
      IntraPredCflInit_NEON();
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      if ((GetCpuInfo() & kSSE4_1) != 0) {
        IntraPredCflInit_SSE4_1();
      }
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }
    cur_cfl_subsampler_ = dsp->cfl_subsamplers[tx_size_][subsampling_type];
  }

  // This test modifies intra_pred_mem_.
  void TestSpeed(const char* digest, int num_runs);
  void TestSaturatedValues();
  void TestRandomValues();

  enum SubsamplingType SubsamplingType() const { return subsampling_type; }

  CflSubsamplerFunc base_cfl_subsampler_;
  CflSubsamplerFunc cur_cfl_subsampler_;
};

// There is no case where both source and output have lowest height or width
// when that dimension is subsampled.
int GetLumaWidth(int block_width, SubsamplingType subsampling_type) {
  if (block_width == 4) {
    const int width_shift =
        static_cast<int>(subsampling_type != kSubsamplingType444);
    return block_width << width_shift;
  }
  return block_width;
}

int GetLumaHeight(int block_height, SubsamplingType subsampling_type) {
  if (block_height == 4) {
    const int height_shift =
        static_cast<int>(subsampling_type == kSubsamplingType420);
    return block_height << height_shift;
  }
  return block_height;
}

template <int bitdepth, typename Pixel, SubsamplingType subsampling_type>
void CflSubsamplerTest<bitdepth, Pixel, subsampling_type>::TestSpeed(
    const char* const digest, const int num_runs) {
  // C declines initializing the table in normal circumstances because there are
  // assembly implementations.
  if (cur_cfl_subsampler_ == nullptr) return;
  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());

  const int width = GetLumaWidth(block_width_, subsampling_type);
  const int height = GetLumaHeight(block_height_, subsampling_type);
  Pixel* src = intra_pred_mem_.ref_src;
#if LIBGAV1_MSAN
  // Quiet 10bpp CflSubsampler420_NEON() msan warning.
  memset(src, 0, sizeof(intra_pred_mem_.ref_src));
#endif
  for (int i = 0; i < height; ++i) {
    for (int j = 0; j < width; ++j) {
      src[j] = rnd.RandRange(1 << bitdepth);
    }
    src += kMaxBlockSize;
  }
  const absl::Time start = absl::Now();
  int16_t luma[kCflLumaBufferStride][kCflLumaBufferStride] = {};
  const ptrdiff_t stride = kMaxBlockSize * sizeof(Pixel);
  for (int run = 0; run < num_runs; ++run) {
    cur_cfl_subsampler_(luma, width, height, intra_pred_mem_.ref_src, stride);
  }
  const absl::Duration elapsed_time = absl::Now() - start;
  test_utils::CheckMd5Digest(ToString(tx_size_), kCflIntraPredName, digest,
                             luma, sizeof(luma), elapsed_time);
}

template <int bitdepth, typename Pixel, SubsamplingType subsampling_type>
void CflSubsamplerTest<bitdepth, Pixel,
                       subsampling_type>::TestSaturatedValues() {
  if (base_cfl_subsampler_ == nullptr) return;
  const ptrdiff_t stride = kMaxBlockSize * sizeof(Pixel);
  for (int width = GetLumaWidth(block_width_, subsampling_type); width > 0;
       width -= 8) {
    for (int height = GetLumaHeight(block_height_, subsampling_type);
         height > 0; height -= 8) {
      Pixel* src = intra_pred_mem_.ref_src;
      for (int y = 0; y < height; ++y) {
        Memset(src, (1 << bitdepth) - 1, width);
        Memset(src + width, 0, kMaxBlockSize - width);
        src += kMaxBlockSize;
      }
      Memset(intra_pred_mem_.ref_src + kMaxBlockSize * height, 0,
             kMaxBlockSize * (kMaxBlockSize - height));

      int16_t luma_base[kCflLumaBufferStride][kCflLumaBufferStride] = {};
      int16_t luma_cur[kCflLumaBufferStride][kCflLumaBufferStride] = {};
      base_cfl_subsampler_(luma_base, width, height, intra_pred_mem_.ref_src,
                           stride);
      cur_cfl_subsampler_(luma_cur, width, height, intra_pred_mem_.ref_src,
                          stride);
      if (!test_utils::CompareBlocks(reinterpret_cast<uint16_t*>(luma_cur[0]),
                                     reinterpret_cast<uint16_t*>(luma_base[0]),
                                     block_width_, block_height_,
                                     kCflLumaBufferStride, kCflLumaBufferStride,
                                     true)) {
        FAIL() << "Result from optimized version of CFL subsampler"
               << " differs from reference. max_luma_width: " << width
               << " max_luma_height: " << height;
      }
    }
  }
}

template <int bitdepth, typename Pixel, SubsamplingType subsampling_type>
void CflSubsamplerTest<bitdepth, Pixel, subsampling_type>::TestRandomValues() {
  if (base_cfl_subsampler_ == nullptr) return;
  const ptrdiff_t stride = kMaxBlockSize * sizeof(Pixel);
  // Use an alternate seed to differentiate this test from TestSpeed().
  libvpx_test::ACMRandom rnd(test_utils::kAlternateDeterministicSeed);
  for (int width = GetLumaWidth(block_width_, subsampling_type); width > 0;
       width -= 8) {
    for (int height = GetLumaHeight(block_height_, subsampling_type);
         height > 0; height -= 8) {
      Pixel* src = intra_pred_mem_.ref_src;
      for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
          src[j] = rnd.RandRange(1 << bitdepth);
        }
        Memset(src + width, 0, kMaxBlockSize - width);
        src += kMaxBlockSize;
      }
      Memset(intra_pred_mem_.ref_src + kMaxBlockSize * height, 0,
             kMaxBlockSize * (kMaxBlockSize - height));

      int16_t luma_base[kCflLumaBufferStride][kCflLumaBufferStride] = {};
      int16_t luma_cur[kCflLumaBufferStride][kCflLumaBufferStride] = {};
      base_cfl_subsampler_(luma_base, width, height, intra_pred_mem_.ref_src,
                           stride);
      cur_cfl_subsampler_(luma_cur, width, height, intra_pred_mem_.ref_src,
                          stride);
      if (!test_utils::CompareBlocks(reinterpret_cast<uint16_t*>(luma_cur[0]),
                                     reinterpret_cast<uint16_t*>(luma_base[0]),
                                     block_width_, block_height_,
                                     kCflLumaBufferStride, kCflLumaBufferStride,
                                     true)) {
        FAIL() << "Result from optimized version of CFL subsampler"
               << " differs from reference. max_luma_width: " << width
               << " max_luma_height: " << height;
      }
    }
  }
}

//------------------------------------------------------------------------------

using CflIntraPredTest8bpp = CflIntraPredTest<8, uint8_t>;

const char* GetCflIntraPredDigest8bpp(TransformSize tx_size) {
  static const char* const kDigest4x4 = "9ea7088e082867fd5ae394ca549fe1ed";
  static const char* const kDigest4x8 = "323b0b4784b6658da781398e61f2da3d";
  static const char* const kDigest4x16 = "99eb9c65f227ca7f71dcac24645a4fec";
  static const char* const kDigest8x4 = "e8e782e31c94f3974b87b93d455262d8";
  static const char* const kDigest8x8 = "23ab9fb65e7bbbdb985709e115115eb5";
  static const char* const kDigest8x16 = "52f5add2fc4bbb2ff893148645e95b9c";
  static const char* const kDigest8x32 = "283fdee9af8afdb76f72dd7339c92c3c";
  static const char* const kDigest16x4 = "eead35f515b1aa8b5175b283192b86e6";
  static const char* const kDigest16x8 = "5778e934254eaab04230bc370f64f778";
  static const char* const kDigest16x16 = "4e8ed38ccba0d62f1213171da2212ed3";
  static const char* const kDigest16x32 = "61a29bd7699e18ca6ea5641d1d023bfd";
  static const char* const kDigest32x8 = "7f31607bd4f9ec879aa47f4daf9c7bb0";
  static const char* const kDigest32x16 = "eb84dfab900fa6a90e132b186b4c6c36";
  static const char* const kDigest32x32 = "e0ff35d407cb214578d61ef419c94237";

  switch (tx_size) {
    case kTransformSize4x4:
      return kDigest4x4;
    case kTransformSize4x8:
      return kDigest4x8;
    case kTransformSize4x16:
      return kDigest4x16;
    case kTransformSize8x4:
      return kDigest8x4;
    case kTransformSize8x8:
      return kDigest8x8;
    case kTransformSize8x16:
      return kDigest8x16;
    case kTransformSize8x32:
      return kDigest8x32;
    case kTransformSize16x4:
      return kDigest16x4;
    case kTransformSize16x8:
      return kDigest16x8;
    case kTransformSize16x16:
      return kDigest16x16;
    case kTransformSize16x32:
      return kDigest16x32;
    case kTransformSize32x8:
      return kDigest32x8;
    case kTransformSize32x16:
      return kDigest32x16;
    case kTransformSize32x32:
      return kDigest32x32;
    default:
      ADD_FAILURE() << "Unknown transform size: " << tx_size;
      return nullptr;
  }
}

TEST_P(CflIntraPredTest8bpp, DISABLED_Speed) {
  const auto num_runs =
      static_cast<int>(2.0e9 / (block_width_ * block_height_));
  TestSpeed(GetCflIntraPredDigest8bpp(tx_size_), num_runs);
}

TEST_P(CflIntraPredTest8bpp, FixedInput) {
  TestSpeed(GetCflIntraPredDigest8bpp(tx_size_), 1);
}

TEST_P(CflIntraPredTest8bpp, Overflow) { TestSaturatedValues(); }

TEST_P(CflIntraPredTest8bpp, Random) { TestRandomValues(); }

//------------------------------------------------------------------------------

using CflSubsamplerTest8bpp444 =
    CflSubsamplerTest<8, uint8_t, kSubsamplingType444>;
using CflSubsamplerTest8bpp422 =
    CflSubsamplerTest<8, uint8_t, kSubsamplingType422>;
using CflSubsamplerTest8bpp420 =
    CflSubsamplerTest<8, uint8_t, kSubsamplingType420>;

const char* GetCflSubsamplerDigest8bpp(TransformSize tx_size,
                                       SubsamplingType subsampling_type) {
  static const char* const kDigests4x4[3] = {
      "a8fa98d76cc3ccffcffc0d02dfae052c", "929cf2c23d926b500616797f8b1baf5b",
      "1d03f091956838e7f2b113aabd8b9da9"};
  static const char* const kDigests4x8[3] = {
      "717b84f867f413c87c90a7c5d0125c8c", "6ccd9f48842b1a802e128b46b8f4885d",
      "68a334f5d2abecbc78562b3280b5fb0c"};
  static const char* const kDigests4x16[3] = {
      "ecd1340b7e065dd8807fd9861abb7d99", "042c3fee17df7ef8fb8cef616f212a91",
      "b0600f0bc3fbfc374bb3628360dcae5c"};
  static const char* const kDigests8x4[3] = {
      "4ea5617f4ed8e9edc2fff88d0ab8e53f", "b02288905f218c9f54ce4a472ec7b22e",
      "3522d3a4dd3839d1a86fb39b31a86d52"};
  static const char* const kDigests8x8[3] = {
      "a0488493e6bcdb868713a95f9b4a0091", "ff6c1ac1d94fce63c282ba49186529bf",
      "082e34ba04d04d7cd6fe408823987602"};
  static const char* const kDigests8x16[3] = {
      "e01dd4bb21daaa6e991cd5b1e6f30300", "2a1b13f932e39cc5f561afea9956f47a",
      "d8d266282cb7123f780bd7266e8f5913"};
  static const char* const kDigests8x32[3] = {
      "0fc95e4ab798b95ccd2966ff75028b03", "6bc6e45ef2f664134449342fe76006ff",
      "d294fb6399edaa267aa167407c0ebccb"};
  static const char* const kDigests16x4[3] = {
      "4798c2cf649b786bd153ad88353d52aa", "43a4bfa3b8caf4b72f58c6a1d1054f64",
      "a928ebbec2db1508c8831a440d82eb98"};
  static const char* const kDigests16x8[3] = {
      "736b7f5b603cb34abcbe1b7e69b6ce93", "90422000ab20ecb519e4d277a9b3ea2b",
      "c8e71c2fddbb850c5a50592ee5975368"};
  static const char* const kDigests16x16[3] = {
      "4f15a694966ee50a9e987e9a0aa2423b", "9e31e2f5a7ce7bef738b135755e25dcd",
      "2ffeed4d592a0455f6d888913969827f"};
  static const char* const kDigests16x32[3] = {
      "3a10438bfe17ea39efad20608a0520eb", "79e8e8732a6ffc29dfbb0b3fc29c2883",
      "185ca976ccbef7fb5f3f8c6aa22d5a79"};
  static const char* const kDigests32x8[3] = {
      "683704f08839a15e42603e4977a3e815", "13d311635372aee8998fca1758e75e20",
      "9847d88eaaa57c086a2e6aed583048d3"};
  static const char* const kDigests32x16[3] = {
      "14b6761bf9f1156cf2496f532512aa99", "ee57bb7f0aa2302d29cdc1bfce72d5fc",
      "a4189655fe714b82eb88cb5092c0ad76"};
  static const char* const kDigests32x32[3] = {
      "dcfbe71b70a37418ccb90dbf27f04226", "c578556a584019c1bdc2d0c3b9fd0c88",
      "db200bc8ccbeacd6a42d6b8e5ad1d931"};

  switch (tx_size) {
    case kTransformSize4x4:
      return kDigests4x4[subsampling_type];
    case kTransformSize4x8:
      return kDigests4x8[subsampling_type];
    case kTransformSize4x16:
      return kDigests4x16[subsampling_type];
    case kTransformSize8x4:
      return kDigests8x4[subsampling_type];
    case kTransformSize8x8:
      return kDigests8x8[subsampling_type];
    case kTransformSize8x16:
      return kDigests8x16[subsampling_type];
    case kTransformSize8x32:
      return kDigests8x32[subsampling_type];
    case kTransformSize16x4:
      return kDigests16x4[subsampling_type];
    case kTransformSize16x8:
      return kDigests16x8[subsampling_type];
    case kTransformSize16x16:
      return kDigests16x16[subsampling_type];
    case kTransformSize16x32:
      return kDigests16x32[subsampling_type];
    case kTransformSize32x8:
      return kDigests32x8[subsampling_type];
    case kTransformSize32x16:
      return kDigests32x16[subsampling_type];
    case kTransformSize32x32:
      return kDigests32x32[subsampling_type];
    default:
      ADD_FAILURE() << "Unknown transform size: " << tx_size;
      return nullptr;
  }
}

TEST_P(CflSubsamplerTest8bpp444, DISABLED_Speed) {
  const auto num_runs =
      static_cast<int>(2.0e9 / (block_width_ * block_height_));
  TestSpeed(GetCflSubsamplerDigest8bpp(tx_size_, SubsamplingType()), num_runs);
}

TEST_P(CflSubsamplerTest8bpp444, FixedInput) {
  TestSpeed(GetCflSubsamplerDigest8bpp(tx_size_, SubsamplingType()), 1);
}

TEST_P(CflSubsamplerTest8bpp444, Overflow) { TestSaturatedValues(); }

TEST_P(CflSubsamplerTest8bpp444, Random) { TestRandomValues(); }

TEST_P(CflSubsamplerTest8bpp422, DISABLED_Speed) {
  const auto num_runs =
      static_cast<int>(2.0e9 / (block_width_ * block_height_));
  TestSpeed(GetCflSubsamplerDigest8bpp(tx_size_, SubsamplingType()), num_runs);
}

TEST_P(CflSubsamplerTest8bpp422, FixedInput) {
  TestSpeed(GetCflSubsamplerDigest8bpp(tx_size_, SubsamplingType()), 1);
}

TEST_P(CflSubsamplerTest8bpp422, Overflow) { TestSaturatedValues(); }

TEST_P(CflSubsamplerTest8bpp422, Random) { TestRandomValues(); }

TEST_P(CflSubsamplerTest8bpp420, DISABLED_Speed) {
  const auto num_runs =
      static_cast<int>(2.0e9 / (block_width_ * block_height_));
  TestSpeed(GetCflSubsamplerDigest8bpp(tx_size_, SubsamplingType()), num_runs);
}

TEST_P(CflSubsamplerTest8bpp420, FixedInput) {
  TestSpeed(GetCflSubsamplerDigest8bpp(tx_size_, SubsamplingType()), 1);
}

TEST_P(CflSubsamplerTest8bpp420, Overflow) { TestSaturatedValues(); }

TEST_P(CflSubsamplerTest8bpp420, Random) { TestRandomValues(); }

//------------------------------------------------------------------------------

#if LIBGAV1_MAX_BITDEPTH >= 10
using CflIntraPredTest10bpp = CflIntraPredTest<10, uint16_t>;

const char* GetCflIntraPredDigest10bpp(TransformSize tx_size) {
  static const char* const kDigest4x4 = "b4ca5f6fbb643a94eb05d59976d44c5d";
  static const char* const kDigest4x8 = "040139b76ee22af05c56baf887d3d43b";
  static const char* const kDigest4x16 = "4a1d59ace84ff07e68a0d30e9b1cebdd";
  static const char* const kDigest8x4 = "c2c149cea5fdcd18bfe5c19ec2a8aa90";
  static const char* const kDigest8x8 = "68ad90bd6f409548fa5551496b7cb0d0";
  static const char* const kDigest8x16 = "bdc54eff4de8c5d597b03afaa705d3fe";
  static const char* const kDigest8x32 = "362aebc6d68ff0d312d55dcd6a8a927d";
  static const char* const kDigest16x4 = "349e813aedd211581c5e64ba1938eaa7";
  static const char* const kDigest16x8 = "35c64f6da17f836618b5804185cf3eef";
  static const char* const kDigest16x16 = "95be0c78dbd8dda793c62c6635b4bfb7";
  static const char* const kDigest16x32 = "4752b9eda069854d3f5c56d3f2057e79";
  static const char* const kDigest32x8 = "dafc5e973e4b6a55861f4586a11b7dd1";
  static const char* const kDigest32x16 = "1e177ed3914a165183916aca1d01bb74";
  static const char* const kDigest32x32 = "4c9ab3cf9baa27bb34e29729dabc1ea6";

  switch (tx_size) {
    case kTransformSize4x4:
      return kDigest4x4;
    case kTransformSize4x8:
      return kDigest4x8;
    case kTransformSize4x16:
      return kDigest4x16;
    case kTransformSize8x4:
      return kDigest8x4;
    case kTransformSize8x8:
      return kDigest8x8;
    case kTransformSize8x16:
      return kDigest8x16;
    case kTransformSize8x32:
      return kDigest8x32;
    case kTransformSize16x4:
      return kDigest16x4;
    case kTransformSize16x8:
      return kDigest16x8;
    case kTransformSize16x16:
      return kDigest16x16;
    case kTransformSize16x32:
      return kDigest16x32;
    case kTransformSize32x8:
      return kDigest32x8;
    case kTransformSize32x16:
      return kDigest32x16;
    case kTransformSize32x32:
      return kDigest32x32;
    default:
      ADD_FAILURE() << "Unknown transform size: " << tx_size;
      return nullptr;
  }
}

TEST_P(CflIntraPredTest10bpp, DISABLED_Speed) {
  const auto num_runs =
      static_cast<int>(2.0e9 / (block_width_ * block_height_));
  TestSpeed(GetCflIntraPredDigest10bpp(tx_size_), num_runs);
}

TEST_P(CflIntraPredTest10bpp, FixedInput) {
  TestSpeed(GetCflIntraPredDigest10bpp(tx_size_), 1);
}

TEST_P(CflIntraPredTest10bpp, Overflow) { TestSaturatedValues(); }

TEST_P(CflIntraPredTest10bpp, Random) { TestRandomValues(); }

//------------------------------------------------------------------------------

using CflSubsamplerTest10bpp444 =
    CflSubsamplerTest<10, uint16_t, kSubsamplingType444>;
using CflSubsamplerTest10bpp422 =
    CflSubsamplerTest<10, uint16_t, kSubsamplingType422>;
using CflSubsamplerTest10bpp420 =
    CflSubsamplerTest<10, uint16_t, kSubsamplingType420>;

const char* GetCflSubsamplerDigest10bpp(TransformSize tx_size,
                                        SubsamplingType subsampling_type) {
  static const char* const kDigests4x4[3] = {
      "a8abcad9a6c9b046a100689135a108cb", "01081c2a0d0c15dabdbc725be5660451",
      "93d1d9df2861240d88f5618e42178654"};
  static const char* const kDigests4x8[3] = {
      "d1fd8cd0709ca6634ad85f3e331672e1", "0d603fcc910aca3db41fc7f64e826c27",
      "cf88b6d1b7b025cfa0082361775aeb75"};
  static const char* const kDigests4x16[3] = {
      "ce2e036a950388a564d8637b1416a6c6", "6c36c46cd72057a6b36bc12188b6d22c",
      "0884a0e53384cd5173035ad8966d8f2f"};
  static const char* const kDigests8x4[3] = {
      "174e961983ed71fb105ed71aa3f9daf5", "330946cc369a534618a1014b4e3f6f18",
      "8070668aa389c1d09f8aaf43c1223e8c"};
  static const char* const kDigests8x8[3] = {
      "86884feb35217010f73ccdbadecb635e", "b8cbc646e1bf1352e5b4b599eaef1193",
      "4a1110382e56b42d3b7a4132bccc01ee"};
  static const char* const kDigests8x16[3] = {
      "a694c4e1f89648ffb49efd6a1d35b300", "864b9da67d23a2f8284b28b2a1e5aa30",
      "bd012ca1cea256dd02c231339a4cf200"};
  static const char* const kDigests8x32[3] = {
      "60c42201bc24e518c1a3b3b6306d8125", "4d530e47c2b7555d5f311ee910d61842",
      "71888b17b832ef55c0cd9449c0e6b077"};
  static const char* const kDigests16x4[3] = {
      "6b6d5ae4cc294c070ce65ab31c5a7d4f", "0fbecee20d294939e7a0183c2b4a0b96",
      "917cd884923139d5c05a11000722e3b6"};
  static const char* const kDigests16x8[3] = {
      "688c41726d9ac35fb5b18c57bca76b9c", "d439a2e0a60d672b644cd1189e2858b9",
      "edded6d166a77a6c3ff46fddc13f372f"};
  static const char* const kDigests16x16[3] = {
      "feb2bad9f6bb3f60eaeaf6c1bfd89ca5", "d65cabce5fcd9a29d1dfc530e4764f3a",
      "2f1a91898812d2c9320c7506b3a72eb4"};
  static const char* const kDigests16x32[3] = {
      "6f23b1851444d29633e62ce77bf09559", "4a449fd078bd0c9657cdc24b709c0796",
      "e44e18cb8bda2d34b52c96d5b6b510be"};
  static const char* const kDigests32x8[3] = {
      "77bf9ba56f7e1d2f04068a8a00b139da", "a85a1dea82963dedab9a2f7ad4169b5f",
      "d12746071bee96ddc075c6368bc9fbaf"};
  static const char* const kDigests32x16[3] = {
      "cce3422f7f8cf57145f979359ac92f98", "1c18738d40bfa91296e5fdb7230bf9a7",
      "02513142d109aee10f081cacfb33d1c5"};
  static const char* const kDigests32x32[3] = {
      "789008e49d0276de186af968196dd4a7", "b8848b00968a7ba4787765b7214da05f",
      "12d13828db57605b00ce99469489651d"};

  switch (tx_size) {
    case kTransformSize4x4:
      return kDigests4x4[subsampling_type];
    case kTransformSize4x8:
      return kDigests4x8[subsampling_type];
    case kTransformSize4x16:
      return kDigests4x16[subsampling_type];
    case kTransformSize8x4:
      return kDigests8x4[subsampling_type];
    case kTransformSize8x8:
      return kDigests8x8[subsampling_type];
    case kTransformSize8x16:
      return kDigests8x16[subsampling_type];
    case kTransformSize8x32:
      return kDigests8x32[subsampling_type];
    case kTransformSize16x4:
      return kDigests16x4[subsampling_type];
    case kTransformSize16x8:
      return kDigests16x8[subsampling_type];
    case kTransformSize16x16:
      return kDigests16x16[subsampling_type];
    case kTransformSize16x32:
      return kDigests16x32[subsampling_type];
    case kTransformSize32x8:
      return kDigests32x8[subsampling_type];
    case kTransformSize32x16:
      return kDigests32x16[subsampling_type];
    case kTransformSize32x32:
      return kDigests32x32[subsampling_type];
    default:
      ADD_FAILURE() << "Unknown transform size: " << tx_size;
      return nullptr;
  }
}

TEST_P(CflSubsamplerTest10bpp444, DISABLED_Speed) {
  const auto num_runs =
      static_cast<int>(2.0e9 / (block_width_ * block_height_));
  TestSpeed(GetCflSubsamplerDigest10bpp(tx_size_, SubsamplingType()), num_runs);
}

TEST_P(CflSubsamplerTest10bpp444, FixedInput) {
  TestSpeed(GetCflSubsamplerDigest10bpp(tx_size_, SubsamplingType()), 1);
}

TEST_P(CflSubsamplerTest10bpp444, Overflow) { TestSaturatedValues(); }

TEST_P(CflSubsamplerTest10bpp444, Random) { TestRandomValues(); }

TEST_P(CflSubsamplerTest10bpp422, DISABLED_Speed) {
  const auto num_runs =
      static_cast<int>(2.0e9 / (block_width_ * block_height_));
  TestSpeed(GetCflSubsamplerDigest10bpp(tx_size_, SubsamplingType()), num_runs);
}

TEST_P(CflSubsamplerTest10bpp422, FixedInput) {
  TestSpeed(GetCflSubsamplerDigest10bpp(tx_size_, SubsamplingType()), 1);
}

TEST_P(CflSubsamplerTest10bpp422, Overflow) { TestSaturatedValues(); }

TEST_P(CflSubsamplerTest10bpp422, Random) { TestRandomValues(); }

TEST_P(CflSubsamplerTest10bpp420, DISABLED_Speed) {
  const auto num_runs =
      static_cast<int>(2.0e9 / (block_width_ * block_height_));
  TestSpeed(GetCflSubsamplerDigest10bpp(tx_size_, SubsamplingType()), num_runs);
}

TEST_P(CflSubsamplerTest10bpp420, FixedInput) {
  TestSpeed(GetCflSubsamplerDigest10bpp(tx_size_, SubsamplingType()), 1);
}

TEST_P(CflSubsamplerTest10bpp420, Overflow) { TestSaturatedValues(); }

TEST_P(CflSubsamplerTest10bpp420, Random) { TestRandomValues(); }
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

//------------------------------------------------------------------------------

#if LIBGAV1_MAX_BITDEPTH == 12
using CflIntraPredTest12bpp = CflIntraPredTest<12, uint16_t>;

const char* GetCflIntraPredDigest12bpp(TransformSize tx_size) {
  static const char* const kDigest4x4 = "1d92a681a58f99396f22acd8b3154e2b";
  static const char* const kDigest4x8 = "cf6833ebc64c9ae45f192ee384ef4aa3";
  static const char* const kDigest4x16 = "06a4fbb8590aca98a045c902ed15c777";
  static const char* const kDigest8x4 = "ad5944c7455f731ae8dd28b2b25a1b9f";
  static const char* const kDigest8x8 = "c19621e42ca2bc184d5065131d27be2c";
  static const char* const kDigest8x16 = "8faa7c95e8c3c18621168ed6759c1ac1";
  static const char* const kDigest8x32 = "502699ef7a8c7aebc8c3bc653e733703";
  static const char* const kDigest16x4 = "7f30bb038217967336fb8548a6f7df45";
  static const char* const kDigest16x8 = "b70943098d0fb256c2943e2ebdbe6d34";
  static const char* const kDigest16x16 = "4c34f5669880ab78d648b16b68ea0c24";
  static const char* const kDigest16x32 = "5d85daf690020ed235617870a1a179b1";
  static const char* const kDigest32x8 = "f8eec12e58c469ffb698fc60b13b927c";
  static const char* const kDigest32x16 = "f272bb7e5d2df333aa63d806c95e6748";
  static const char* const kDigest32x32 = "c737987c0a5414b03e6014f145dd999c";

  switch (tx_size) {
    case kTransformSize4x4:
      return kDigest4x4;
    case kTransformSize4x8:
      return kDigest4x8;
    case kTransformSize4x16:
      return kDigest4x16;
    case kTransformSize8x4:
      return kDigest8x4;
    case kTransformSize8x8:
      return kDigest8x8;
    case kTransformSize8x16:
      return kDigest8x16;
    case kTransformSize8x32:
      return kDigest8x32;
    case kTransformSize16x4:
      return kDigest16x4;
    case kTransformSize16x8:
      return kDigest16x8;
    case kTransformSize16x16:
      return kDigest16x16;
    case kTransformSize16x32:
      return kDigest16x32;
    case kTransformSize32x8:
      return kDigest32x8;
    case kTransformSize32x16:
      return kDigest32x16;
    case kTransformSize32x32:
      return kDigest32x32;
    default:
      ADD_FAILURE() << "Unknown transform size: " << tx_size;
      return nullptr;
  }
}

TEST_P(CflIntraPredTest12bpp, DISABLED_Speed) {
  const auto num_runs =
      static_cast<int>(2.0e9 / (block_width_ * block_height_));
  TestSpeed(GetCflIntraPredDigest12bpp(tx_size_), num_runs);
}

TEST_P(CflIntraPredTest12bpp, FixedInput) {
  TestSpeed(GetCflIntraPredDigest12bpp(tx_size_), 1);
}

TEST_P(CflIntraPredTest12bpp, Overflow) { TestSaturatedValues(); }

TEST_P(CflIntraPredTest12bpp, Random) { TestRandomValues(); }

//------------------------------------------------------------------------------

using CflSubsamplerTest12bpp444 =
    CflSubsamplerTest<12, uint16_t, kSubsamplingType444>;
using CflSubsamplerTest12bpp422 =
    CflSubsamplerTest<12, uint16_t, kSubsamplingType422>;
using CflSubsamplerTest12bpp420 =
    CflSubsamplerTest<12, uint16_t, kSubsamplingType420>;

const char* GetCflSubsamplerDigest12bpp(TransformSize tx_size,
                                        SubsamplingType subsampling_type) {
  static const char* const kDigests4x4[3] = {
      "44af37c60e9ccaacea004b57d5dea4cf",
      "e29dd1d93f23b23778ed8cd85910d987",
      "81e5dac2fd4c90f872ab814ed0f76ae5",
  };
  static const char* const kDigests4x8[3] = {
      "bfc04aed9fe41ec07b0462a219652d16",
      "693dd064636a0aa3be7aa098e867c512",
      "0636c25d88aacd85d63e56011e7c5d15",
  };
  static const char* const kDigests4x16[3] = {
      "6479ab30377288e75a78068d47c7e194",
      "7d6f9b8b3eb85e73626118fc9210e622",
      "1f3d474cd7c86899da90e515b8b7a906",
  };
  static const char* const kDigests8x4[3] = {
      "7da5a2029bcdab159225c475fdff02da",
      "096bfef24caa0670d2cd7b0bb63a7ba6",
      "f749310dfc8a6129ed438dbc845470c0",
  };
  static const char* const kDigests8x8[3] = {
      "08494051a7ff50718313a79ec7c51f92",
      "637efad0630e253f7cce11af1a0af456",
      "b220faf7dfedef860d59079dcf201757",
  };
  static const char* const kDigests8x16[3] = {
      "19f027af516e88d3b9e613e578deb126",
      "4f3bb155d70f9ea76d05b2f41b297a0c",
      "b7504347eeda1e59ba8e36385c219e40",
  };
  static const char* const kDigests8x32[3] = {
      "b8f1ef01c5672c87ee1004bb3cd7b8bc",
      "b3e3318b050eb1c165d1e320ef622fa7",
      "67754f7c5ae84dc23bb76ffaa2fa848e",
  };
  static const char* const kDigests16x4[3] = {
      "f687fb4e22d8a1446eeb4915036874f4",
      "7b5ef3d393a98dfe0ba49a0db2083465",
      "840bbb6edaa50e9f7d391033a3dda2d9",
  };
  static const char* const kDigests16x8[3] = {
      "dd9aed11d115a028035f0cee5b90d433",
      "340d5d0784356ea199d3d751f4d6ed5e",
      "e55f6fb5f34d829727e9dc2068098933",
  };
  static const char* const kDigests16x16[3] = {
      "1df36a20d76a405c6273b88b38693cf9",
      "2a7590d01df60b4bc6f10bfdb07b7a65",
      "510ee31a5bd609e8f4542bb817539668",
  };
  static const char* const kDigests16x32[3] = {
      "bdbc13b9fb7c3c50d25fda57f86f5ad9",
      "7c138c568794b3d0c8aabff2edc07efd",
      "581bef267c2a66e4c2fb079968440dbe",
  };
  static const char* const kDigests32x8[3] = {
      "26f62743793811475e2afe1414c5fee1",
      "6e6bf1678a04f2f727f0679564fb3630",
      "a4c15562c26dbcfa43fe03a2b6e728b5",
  };
  static const char* const kDigests32x16[3] = {
      "791f0713bbf032081da8ec08e58b9cd3",
      "5dc7a673e92767186ae86996f4a30691",
      "651f09d1244c817d92d1baa094c86f56",
  };
  static const char* const kDigests32x32[3] = {
      "543a9d76e7238d88ba86218ec47c1f49",
      "b0f2b29aae4858c1f09c27fc4344fd15",
      "1d45083875fed14c4e5f149384a3cd2d",
  };

  switch (tx_size) {
    case kTransformSize4x4:
      return kDigests4x4[subsampling_type];
    case kTransformSize4x8:
      return kDigests4x8[subsampling_type];
    case kTransformSize4x16:
      return kDigests4x16[subsampling_type];
    case kTransformSize8x4:
      return kDigests8x4[subsampling_type];
    case kTransformSize8x8:
      return kDigests8x8[subsampling_type];
    case kTransformSize8x16:
      return kDigests8x16[subsampling_type];
    case kTransformSize8x32:
      return kDigests8x32[subsampling_type];
    case kTransformSize16x4:
      return kDigests16x4[subsampling_type];
    case kTransformSize16x8:
      return kDigests16x8[subsampling_type];
    case kTransformSize16x16:
      return kDigests16x16[subsampling_type];
    case kTransformSize16x32:
      return kDigests16x32[subsampling_type];
    case kTransformSize32x8:
      return kDigests32x8[subsampling_type];
    case kTransformSize32x16:
      return kDigests32x16[subsampling_type];
    case kTransformSize32x32:
      return kDigests32x32[subsampling_type];
    default:
      ADD_FAILURE() << "Unknown transform size: " << tx_size;
      return nullptr;
  }
}

TEST_P(CflSubsamplerTest12bpp444, DISABLED_Speed) {
  const auto num_runs =
      static_cast<int>(2.0e9 / (block_width_ * block_height_));
  TestSpeed(GetCflSubsamplerDigest12bpp(tx_size_, SubsamplingType()), num_runs);
}

TEST_P(CflSubsamplerTest12bpp444, FixedInput) {
  TestSpeed(GetCflSubsamplerDigest12bpp(tx_size_, SubsamplingType()), 1);
}

TEST_P(CflSubsamplerTest12bpp444, Overflow) { TestSaturatedValues(); }

TEST_P(CflSubsamplerTest12bpp444, Random) { TestRandomValues(); }

TEST_P(CflSubsamplerTest12bpp422, DISABLED_Speed) {
  const auto num_runs =
      static_cast<int>(2.0e9 / (block_width_ * block_height_));
  TestSpeed(GetCflSubsamplerDigest12bpp(tx_size_, SubsamplingType()), num_runs);
}

TEST_P(CflSubsamplerTest12bpp422, FixedInput) {
  TestSpeed(GetCflSubsamplerDigest12bpp(tx_size_, SubsamplingType()), 1);
}

TEST_P(CflSubsamplerTest12bpp422, Overflow) { TestSaturatedValues(); }

TEST_P(CflSubsamplerTest12bpp422, Random) { TestRandomValues(); }

TEST_P(CflSubsamplerTest12bpp420, DISABLED_Speed) {
  const auto num_runs =
      static_cast<int>(2.0e9 / (block_width_ * block_height_));
  TestSpeed(GetCflSubsamplerDigest12bpp(tx_size_, SubsamplingType()), num_runs);
}

TEST_P(CflSubsamplerTest12bpp420, FixedInput) {
  TestSpeed(GetCflSubsamplerDigest12bpp(tx_size_, SubsamplingType()), 1);
}

TEST_P(CflSubsamplerTest12bpp420, Overflow) { TestSaturatedValues(); }

TEST_P(CflSubsamplerTest12bpp420, Random) { TestRandomValues(); }
#endif  // LIBGAV1_MAX_BITDEPTH == 12

// Cfl predictors are available only for transform sizes with
// max(width, height) <= 32.
constexpr TransformSize kTransformSizesSmallerThan32x32[] = {
    kTransformSize4x4,   kTransformSize4x8,   kTransformSize4x16,
    kTransformSize8x4,   kTransformSize8x8,   kTransformSize8x16,
    kTransformSize8x32,  kTransformSize16x4,  kTransformSize16x8,
    kTransformSize16x16, kTransformSize16x32, kTransformSize32x8,
    kTransformSize32x16, kTransformSize32x32};

INSTANTIATE_TEST_SUITE_P(C, CflIntraPredTest8bpp,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
INSTANTIATE_TEST_SUITE_P(C, CflSubsamplerTest8bpp444,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
INSTANTIATE_TEST_SUITE_P(C, CflSubsamplerTest8bpp422,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
INSTANTIATE_TEST_SUITE_P(C, CflSubsamplerTest8bpp420,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, CflIntraPredTest8bpp,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
INSTANTIATE_TEST_SUITE_P(SSE41, CflSubsamplerTest8bpp444,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
INSTANTIATE_TEST_SUITE_P(SSE41, CflSubsamplerTest8bpp420,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
#endif  // LIBGAV1_ENABLE_SSE4_1
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, CflIntraPredTest8bpp,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
INSTANTIATE_TEST_SUITE_P(NEON, CflSubsamplerTest8bpp444,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
INSTANTIATE_TEST_SUITE_P(NEON, CflSubsamplerTest8bpp420,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
#endif  // LIBGAV1_ENABLE_NEON

#if LIBGAV1_MAX_BITDEPTH >= 10
INSTANTIATE_TEST_SUITE_P(C, CflIntraPredTest10bpp,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
INSTANTIATE_TEST_SUITE_P(C, CflSubsamplerTest10bpp444,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
INSTANTIATE_TEST_SUITE_P(C, CflSubsamplerTest10bpp422,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
INSTANTIATE_TEST_SUITE_P(C, CflSubsamplerTest10bpp420,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, CflIntraPredTest10bpp,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
INSTANTIATE_TEST_SUITE_P(SSE41, CflSubsamplerTest10bpp444,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
INSTANTIATE_TEST_SUITE_P(SSE41, CflSubsamplerTest10bpp420,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
#endif  // LIBGAV1_ENABLE_SSE4_1
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, CflIntraPredTest10bpp,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
INSTANTIATE_TEST_SUITE_P(NEON, CflSubsamplerTest10bpp444,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
INSTANTIATE_TEST_SUITE_P(NEON, CflSubsamplerTest10bpp420,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
#endif  // LIBGAV1_ENABLE_NEON

#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
INSTANTIATE_TEST_SUITE_P(C, CflIntraPredTest12bpp,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
INSTANTIATE_TEST_SUITE_P(C, CflSubsamplerTest12bpp444,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
INSTANTIATE_TEST_SUITE_P(C, CflSubsamplerTest12bpp422,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
INSTANTIATE_TEST_SUITE_P(C, CflSubsamplerTest12bpp420,
                         testing::ValuesIn(kTransformSizesSmallerThan32x32));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace
}  // namespace dsp

static std::ostream& operator<<(std::ostream& os, const TransformSize tx_size) {
  return os << ToString(tx_size);
}

}  // namespace libgav1
