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

#include "src/dsp/intrapred_directional.h"

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
constexpr int kNumDirectionalIntraPredictors = 3;

constexpr int kBaseAngles[] = {45, 67, 90, 113, 135, 157, 180, 203};

const char* const kDirectionalPredNames[kNumDirectionalIntraPredictors] = {
    "kDirectionalIntraPredictorZone1", "kDirectionalIntraPredictorZone2",
    "kDirectionalIntraPredictorZone3"};

int16_t GetDirectionalIntraPredictorDerivative(const int angle) {
  EXPECT_GE(angle, 3);
  EXPECT_LE(angle, 87);
  return kDirectionalIntraPredictorDerivative[DivideBy2(angle) - 1];
}

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
#if LIBGAV1_MSAN
      // Match the behavior of Tile::IntraPrediction to prevent warnings due to
      // assembly code (safely) overreading to fill a register.
      memset(left_mem, 0, sizeof(left_mem));
      memset(top_mem, 0, sizeof(top_mem));
#endif  // LIBGAV1_MSAN
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
#if LIBGAV1_MSAN
      // Match the behavior of Tile::IntraPrediction to prevent warnings due to
      // assembly code (safely) overreading to fill a register.
      memset(left_mem, 0, sizeof(left_mem));
      memset(top_mem, 0, sizeof(top_mem));
#endif  // LIBGAV1_MSAN
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
// DirectionalIntraPredTest

template <int bitdepth, typename Pixel>
class DirectionalIntraPredTest : public IntraPredTestBase<bitdepth, Pixel> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  DirectionalIntraPredTest() = default;
  DirectionalIntraPredTest(const DirectionalIntraPredTest&) = delete;
  DirectionalIntraPredTest& operator=(const DirectionalIntraPredTest&) = delete;
  ~DirectionalIntraPredTest() override = default;

 protected:
  using IntraPredTestBase<bitdepth, Pixel>::tx_size_;
  using IntraPredTestBase<bitdepth, Pixel>::block_width_;
  using IntraPredTestBase<bitdepth, Pixel>::block_height_;
  using IntraPredTestBase<bitdepth, Pixel>::intra_pred_mem_;

  enum Zone { kZone1, kZone2, kZone3, kNumZones };

  enum { kAngleDeltaStart = -9, kAngleDeltaStop = 9, kAngleDeltaStep = 3 };

  void SetUp() override {
    IntraPredTestBase<bitdepth, Pixel>::SetUp();
    IntraPredDirectionalInit_C();

    const Dsp* const dsp = GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);
    base_directional_intra_pred_zone1_ = dsp->directional_intra_predictor_zone1;
    base_directional_intra_pred_zone2_ = dsp->directional_intra_predictor_zone2;
    base_directional_intra_pred_zone3_ = dsp->directional_intra_predictor_zone3;

    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const char* const test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
      base_directional_intra_pred_zone1_ = nullptr;
      base_directional_intra_pred_zone2_ = nullptr;
      base_directional_intra_pred_zone3_ = nullptr;
    } else if (absl::StartsWith(test_case, "NEON/")) {
      IntraPredDirectionalInit_NEON();
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      if ((GetCpuInfo() & kSSE4_1) != 0) {
        IntraPredDirectionalInit_SSE4_1();
      }
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }

    cur_directional_intra_pred_zone1_ = dsp->directional_intra_predictor_zone1;
    cur_directional_intra_pred_zone2_ = dsp->directional_intra_predictor_zone2;
    cur_directional_intra_pred_zone3_ = dsp->directional_intra_predictor_zone3;

    // Skip functions that haven't been specialized for this particular
    // architecture.
    if (cur_directional_intra_pred_zone1_ ==
        base_directional_intra_pred_zone1_) {
      cur_directional_intra_pred_zone1_ = nullptr;
    }
    if (cur_directional_intra_pred_zone2_ ==
        base_directional_intra_pred_zone2_) {
      cur_directional_intra_pred_zone2_ = nullptr;
    }
    if (cur_directional_intra_pred_zone3_ ==
        base_directional_intra_pred_zone3_) {
      cur_directional_intra_pred_zone3_ = nullptr;
    }
  }

  bool IsEdgeUpsampled(int delta, const int filter_type) const {
    delta = std::abs(delta);
    if (delta == 0 || delta >= 40) return false;
    const int block_wh = block_width_ + block_height_;
    return (filter_type == 1) ? block_wh <= 8 : block_wh <= 16;
  }

  // Returns the minimum and maximum (exclusive) range of angles that the
  // predictor should be applied to.
  void GetZoneAngleRange(const Zone zone, int* const min_angle,
                         int* const max_angle) const {
    ASSERT_NE(min_angle, nullptr);
    ASSERT_NE(max_angle, nullptr);
    switch (zone) {
        // The overall minimum angle comes from mode D45_PRED, yielding:
        // min_angle = 45-(MAX_ANGLE_DELTA*ANGLE_STEP) = 36
        // The overall maximum angle comes from mode D203_PRED, yielding:
        // max_angle = 203+(MAX_ANGLE_DELTA*ANGLE_STEP) = 212
        // The angles 180 and 90 are not permitted because they correspond to
        // V_PRED and H_PRED, which are handled in distinct functions.
      case kZone1:
        *min_angle = 36;
        *max_angle = 87;
        break;
      case kZone2:
        *min_angle = 93;
        *max_angle = 177;
        break;
      case kZone3:
        *min_angle = 183;
        *max_angle = 212;
        break;
      case kNumZones:
        FAIL() << "Invalid zone value: " << zone;
        break;
    }
  }

  // These tests modify intra_pred_mem_.
  void TestSpeed(const char* const digests[kNumDirectionalIntraPredictors],
                 Zone zone, int num_runs);
  void TestSaturatedValues();
  void TestRandomValues();

  DirectionalIntraPredictorZone1Func base_directional_intra_pred_zone1_;
  DirectionalIntraPredictorZone2Func base_directional_intra_pred_zone2_;
  DirectionalIntraPredictorZone3Func base_directional_intra_pred_zone3_;
  DirectionalIntraPredictorZone1Func cur_directional_intra_pred_zone1_;
  DirectionalIntraPredictorZone2Func cur_directional_intra_pred_zone2_;
  DirectionalIntraPredictorZone3Func cur_directional_intra_pred_zone3_;
};

template <int bitdepth, typename Pixel>
void DirectionalIntraPredTest<bitdepth, Pixel>::TestSpeed(
    const char* const digests[kNumDirectionalIntraPredictors], const Zone zone,
    const int num_runs) {
  switch (zone) {
    case kZone1:
      if (cur_directional_intra_pred_zone1_ == nullptr) return;
      break;
    case kZone2:
      if (cur_directional_intra_pred_zone2_ == nullptr) return;
      break;
    case kZone3:
      if (cur_directional_intra_pred_zone3_ == nullptr) return;
      break;
    case kNumZones:
      FAIL() << "Invalid zone value: " << zone;
      break;
  }
  ASSERT_NE(digests, nullptr);
  const Pixel* const left = intra_pred_mem_.left_mem + 16;
  const Pixel* const top = intra_pred_mem_.top_mem + 16;

  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
  intra_pred_mem_.Reset(&rnd);

  // Allocate separate blocks for each angle + filter + upsampled combination.
  // Add a 1 pixel right border to test for overwrites.
  static constexpr int kMaxZoneAngles = 27;  // zone 2
  static constexpr int kMaxFilterTypes = 2;
  static constexpr int kBlockBorder = 1;
  static constexpr int kBorderSize =
      kBlockBorder * kMaxZoneAngles * kMaxFilterTypes;
  const int ref_stride =
      kMaxZoneAngles * kMaxFilterTypes * block_width_ + kBorderSize;
  const size_t ref_alloc_size = sizeof(Pixel) * ref_stride * block_height_;

  using AlignedPtr = std::unique_ptr<Pixel[], decltype(&AlignedFree)>;
  AlignedPtr ref_src(static_cast<Pixel*>(AlignedAlloc(16, ref_alloc_size)),
                     &AlignedFree);
  AlignedPtr dest(static_cast<Pixel*>(AlignedAlloc(16, ref_alloc_size)),
                  &AlignedFree);
  ASSERT_NE(ref_src, nullptr);
  ASSERT_NE(dest, nullptr);

  const int mask = (1 << bitdepth) - 1;
  for (size_t i = 0; i < ref_alloc_size / sizeof(ref_src[0]); ++i) {
    ref_src[i] = rnd.Rand16() & mask;
  }

  int min_angle = 0, max_angle = 0;
  ASSERT_NO_FATAL_FAILURE(GetZoneAngleRange(zone, &min_angle, &max_angle));

  absl::Duration elapsed_time;
  for (int run = 0; run < num_runs; ++run) {
    Pixel* dst = dest.get();
    memcpy(dst, ref_src.get(), ref_alloc_size);
    for (const auto& base_angle : kBaseAngles) {
      for (int filter_type = 0; filter_type <= 1; ++filter_type) {
        for (int angle_delta = kAngleDeltaStart; angle_delta <= kAngleDeltaStop;
             angle_delta += kAngleDeltaStep) {
          const int predictor_angle = base_angle + angle_delta;
          if (predictor_angle < min_angle || predictor_angle > max_angle) {
            continue;
          }

          ASSERT_GT(predictor_angle, 0) << "base_angle: " << base_angle
                                        << " angle_delta: " << angle_delta;
          const bool upsampled_left =
              IsEdgeUpsampled(predictor_angle - 180, filter_type);
          const bool upsampled_top =
              IsEdgeUpsampled(predictor_angle - 90, filter_type);
          const ptrdiff_t stride = ref_stride * sizeof(ref_src[0]);
          if (predictor_angle < 90) {
            ASSERT_EQ(zone, kZone1);
            const int xstep =
                GetDirectionalIntraPredictorDerivative(predictor_angle);
            const absl::Time start = absl::Now();
            cur_directional_intra_pred_zone1_(dst, stride, top, block_width_,
                                              block_height_, xstep,
                                              upsampled_top);
            elapsed_time += absl::Now() - start;
          } else if (predictor_angle < 180) {
            ASSERT_EQ(zone, kZone2);
            const int xstep =
                GetDirectionalIntraPredictorDerivative(180 - predictor_angle);
            const int ystep =
                GetDirectionalIntraPredictorDerivative(predictor_angle - 90);
            const absl::Time start = absl::Now();
            cur_directional_intra_pred_zone2_(
                dst, stride, top, left, block_width_, block_height_, xstep,
                ystep, upsampled_top, upsampled_left);
            elapsed_time += absl::Now() - start;
          } else {
            ASSERT_EQ(zone, kZone3);
            ASSERT_LT(predictor_angle, 270);
            const int ystep =
                GetDirectionalIntraPredictorDerivative(270 - predictor_angle);
            const absl::Time start = absl::Now();
            cur_directional_intra_pred_zone3_(dst, stride, left, block_width_,
                                              block_height_, ystep,
                                              upsampled_left);
            elapsed_time += absl::Now() - start;
          }
          dst += block_width_ + kBlockBorder;
        }
      }
    }
  }

  test_utils::CheckMd5Digest(ToString(tx_size_), kDirectionalPredNames[zone],
                             digests[zone], dest.get(), ref_alloc_size,
                             elapsed_time);
}

template <int bitdepth, typename Pixel>
void DirectionalIntraPredTest<bitdepth, Pixel>::TestSaturatedValues() {
  const Pixel* const left = intra_pred_mem_.left_mem + 16;
  const Pixel* const top = intra_pred_mem_.top_mem + 16;
  const auto kMaxPixel = static_cast<Pixel>((1 << bitdepth) - 1);
  intra_pred_mem_.Set(kMaxPixel);

  for (int i = kZone1; i < kNumZones; ++i) {
    switch (i) {
      case kZone1:
        if (cur_directional_intra_pred_zone1_ == nullptr) continue;
        break;
      case kZone2:
        if (cur_directional_intra_pred_zone2_ == nullptr) continue;
        break;
      case kZone3:
        if (cur_directional_intra_pred_zone3_ == nullptr) continue;
        break;
      case kNumZones:
        FAIL() << "Invalid zone value: " << i;
        break;
    }
    int min_angle = 0, max_angle = 0;
    ASSERT_NO_FATAL_FAILURE(
        GetZoneAngleRange(static_cast<Zone>(i), &min_angle, &max_angle));

    for (const auto& base_angle : kBaseAngles) {
      for (int filter_type = 0; filter_type <= 1; ++filter_type) {
        for (int angle_delta = kAngleDeltaStart; angle_delta <= kAngleDeltaStop;
             angle_delta += kAngleDeltaStep) {
          const int predictor_angle = base_angle + angle_delta;
          if (predictor_angle <= min_angle || predictor_angle >= max_angle) {
            continue;
          }
          ASSERT_GT(predictor_angle, 0) << "base_angle: " << base_angle
                                        << " angle_delta: " << angle_delta;

          memcpy(intra_pred_mem_.dst, intra_pred_mem_.ref_src,
                 sizeof(intra_pred_mem_.dst));

          const bool upsampled_left =
              IsEdgeUpsampled(predictor_angle - 180, filter_type);
          const bool upsampled_top =
              IsEdgeUpsampled(predictor_angle - 90, filter_type);
          const ptrdiff_t stride = kMaxBlockSize * sizeof(Pixel);
          if (predictor_angle < 90) {
            const int xstep =
                GetDirectionalIntraPredictorDerivative(predictor_angle);
            cur_directional_intra_pred_zone1_(intra_pred_mem_.dst, stride, top,
                                              block_width_, block_height_,
                                              xstep, upsampled_top);
          } else if (predictor_angle < 180) {
            const int xstep =
                GetDirectionalIntraPredictorDerivative(180 - predictor_angle);
            const int ystep =
                GetDirectionalIntraPredictorDerivative(predictor_angle - 90);
            cur_directional_intra_pred_zone2_(
                intra_pred_mem_.dst, stride, top, left, block_width_,
                block_height_, xstep, ystep, upsampled_top, upsampled_left);
          } else {
            ASSERT_LT(predictor_angle, 270);
            const int ystep =
                GetDirectionalIntraPredictorDerivative(270 - predictor_angle);
            cur_directional_intra_pred_zone3_(intra_pred_mem_.dst, stride, left,
                                              block_width_, block_height_,
                                              ystep, upsampled_left);
          }

          if (!test_utils::CompareBlocks(
                  intra_pred_mem_.dst, intra_pred_mem_.ref_src, block_width_,
                  block_height_, kMaxBlockSize, kMaxBlockSize, true)) {
            ADD_FAILURE() << "Expected " << kDirectionalPredNames[i]
                          << " (angle: " << predictor_angle
                          << " filter type: " << filter_type
                          << ") to produce a block containing '"
                          << static_cast<int>(kMaxPixel) << "'";
            return;
          }
        }
      }
    }
  }
}

template <int bitdepth, typename Pixel>
void DirectionalIntraPredTest<bitdepth, Pixel>::TestRandomValues() {
  const Pixel* const left = intra_pred_mem_.left_mem + 16;
  const Pixel* const top = intra_pred_mem_.top_mem + 16;
  // Use an alternate seed to differentiate this test from TestSpeed().
  libvpx_test::ACMRandom rnd(test_utils::kAlternateDeterministicSeed);

  for (int i = kZone1; i < kNumZones; ++i) {
    // Only run when there is a reference version (base) and a different
    // optimized version (cur).
    switch (i) {
      case kZone1:
        if (base_directional_intra_pred_zone1_ == nullptr ||
            cur_directional_intra_pred_zone1_ == nullptr) {
          continue;
        }
        break;
      case kZone2:
        if (base_directional_intra_pred_zone2_ == nullptr ||
            cur_directional_intra_pred_zone2_ == nullptr) {
          continue;
        }
        break;
      case kZone3:
        if (base_directional_intra_pred_zone3_ == nullptr ||
            cur_directional_intra_pred_zone3_ == nullptr) {
          continue;
        }
        break;
      case kNumZones:
        FAIL() << "Invalid zone value: " << i;
        break;
    }
    int min_angle = 0, max_angle = 0;
    ASSERT_NO_FATAL_FAILURE(
        GetZoneAngleRange(static_cast<Zone>(i), &min_angle, &max_angle));

    for (const auto& base_angle : kBaseAngles) {
      for (int n = 0; n < 1000; ++n) {
        for (int filter_type = 0; filter_type <= 1; ++filter_type) {
          for (int angle_delta = kAngleDeltaStart;
               angle_delta <= kAngleDeltaStop; angle_delta += kAngleDeltaStep) {
            const int predictor_angle = base_angle + angle_delta;
            if (predictor_angle <= min_angle || predictor_angle >= max_angle) {
              continue;
            }
            ASSERT_GT(predictor_angle, 0) << "base_angle: " << base_angle
                                          << " angle_delta: " << angle_delta;

            intra_pred_mem_.Reset(&rnd);
            memcpy(intra_pred_mem_.dst, intra_pred_mem_.ref_src,
                   sizeof(intra_pred_mem_.dst));

            const bool upsampled_left =
                IsEdgeUpsampled(predictor_angle - 180, filter_type);
            const bool upsampled_top =
                IsEdgeUpsampled(predictor_angle - 90, filter_type);
            const ptrdiff_t stride = kMaxBlockSize * sizeof(Pixel);
            if (predictor_angle < 90) {
              const int xstep =
                  GetDirectionalIntraPredictorDerivative(predictor_angle);
              base_directional_intra_pred_zone1_(
                  intra_pred_mem_.ref_src, stride, top, block_width_,
                  block_height_, xstep, upsampled_top);
              cur_directional_intra_pred_zone1_(
                  intra_pred_mem_.dst, stride, top, block_width_, block_height_,
                  xstep, upsampled_top);
            } else if (predictor_angle < 180) {
              const int xstep =
                  GetDirectionalIntraPredictorDerivative(180 - predictor_angle);
              const int ystep =
                  GetDirectionalIntraPredictorDerivative(predictor_angle - 90);
              base_directional_intra_pred_zone2_(
                  intra_pred_mem_.ref_src, stride, top, left, block_width_,
                  block_height_, xstep, ystep, upsampled_top, upsampled_left);
              cur_directional_intra_pred_zone2_(
                  intra_pred_mem_.dst, stride, top, left, block_width_,
                  block_height_, xstep, ystep, upsampled_top, upsampled_left);
            } else {
              ASSERT_LT(predictor_angle, 270);
              const int ystep =
                  GetDirectionalIntraPredictorDerivative(270 - predictor_angle);
              base_directional_intra_pred_zone3_(
                  intra_pred_mem_.ref_src, stride, left, block_width_,
                  block_height_, ystep, upsampled_left);
              cur_directional_intra_pred_zone3_(
                  intra_pred_mem_.dst, stride, left, block_width_,
                  block_height_, ystep, upsampled_left);
            }

            if (!test_utils::CompareBlocks(
                    intra_pred_mem_.dst, intra_pred_mem_.ref_src, block_width_,
                    block_height_, kMaxBlockSize, kMaxBlockSize, true)) {
              ADD_FAILURE() << "Result from optimized version of "
                            << kDirectionalPredNames[i]
                            << " differs from reference at angle "
                            << predictor_angle << " with filter type "
                            << filter_type << " in iteration #" << n;
              return;
            }
          }
        }
      }
    }
  }
}

using DirectionalIntraPredTest8bpp = DirectionalIntraPredTest<8, uint8_t>;

const char* const* GetDirectionalIntraPredDigests8bpp(TransformSize tx_size) {
  static const char* const kDigests4x4[kNumDirectionalIntraPredictors] = {
      "9cfc1da729ad08682e165826c29b280b",
      "bb73539c7afbda7bddd2184723b932d6",
      "9d2882800ffe948196e984a26a2da72c",
  };
  static const char* const kDigests4x8[kNumDirectionalIntraPredictors] = {
      "090efe6f83cc6fa301f65d3bbd5c38d2",
      "d0fba4cdfb90f8bd293a94cae9db1a15",
      "f7ad0eeab4389d0baa485d30fec87617",
  };
  static const char* const kDigests4x16[kNumDirectionalIntraPredictors] = {
      "1d32b33c75fe85248c48cdc8caa78d84",
      "7000e18159443d366129a6cc6ef8fcee",
      "06c02fac5f8575f687abb3f634eb0b4c",
  };
  static const char* const kDigests8x4[kNumDirectionalIntraPredictors] = {
      "1b591799685bc135982114b731293f78",
      "5cd9099acb9f7b2618dafa6712666580",
      "d023883efede88f99c19d006044d9fa1",
  };
  static const char* const kDigests8x8[kNumDirectionalIntraPredictors] = {
      "f1e46ecf62a2516852f30c5025adb7ea",
      "864442a209c16998065af28d8cdd839a",
      "411a6e554868982af577de69e53f12e8",
  };
  static const char* const kDigests8x16[kNumDirectionalIntraPredictors] = {
      "89278302be913a85cfb06feaea339459",
      "6c42f1a9493490cd4529fd40729cec3c",
      "2516b5e1c681e5dcb1acedd5f3d41106",
  };
  static const char* const kDigests8x32[kNumDirectionalIntraPredictors] = {
      "aea7078f3eeaa8afbfe6c959c9e676f1",
      "cad30babf12729dda5010362223ba65c",
      "ff384ebdc832007775af418a2aae1463",
  };
  static const char* const kDigests16x4[kNumDirectionalIntraPredictors] = {
      "964a821c313c831e12f4d32e616c0b55",
      "adf6dad3a84ab4d16c16eea218bec57a",
      "a54fa008d43895e523474686c48a81c2",
  };
  static const char* const kDigests16x8[kNumDirectionalIntraPredictors] = {
      "fe2851b4e4f9fcf924cf17d50415a4c0",
      "50a0e279c481437ff315d08eb904c733",
      "0682065c8fb6cbf9be4949316c87c9e5",
  };
  static const char* const kDigests16x16[kNumDirectionalIntraPredictors] = {
      "ef15503b1943642e7a0bace1616c0e11",
      "bf1a4d3f855f1072a902a88ec6ce0350",
      "7e87a03e29cd7fd843fd71b729a18f3f",
  };
  static const char* const kDigests16x32[kNumDirectionalIntraPredictors] = {
      "f7b636615d2e5bf289b5db452a6f188d",
      "e95858c532c10d00b0ce7a02a02121dd",
      "34a18ccf58ef490f32268e85ce8c7de4",
  };
  static const char* const kDigests16x64[kNumDirectionalIntraPredictors] = {
      "b250099986c2fab9670748598058846b",
      "f25d80af4da862a9b6b72979f1e17cb4",
      "5347dc7bc346733b4887f6c8ad5e0898",
  };
  static const char* const kDigests32x8[kNumDirectionalIntraPredictors] = {
      "72e4c9f8af043b1cb1263490351818ab",
      "1fc010d2df011b9e4e3d0957107c78df",
      "f4cbfa3ca941ef08b972a68d7e7bafc4",
  };
  static const char* const kDigests32x16[kNumDirectionalIntraPredictors] = {
      "37e5a1aaf7549d2bce08eece9d20f0f6",
      "6a2794025d0aca414ab17baa3cf8251a",
      "63dd37a6efdc91eeefef166c99ce2db1",
  };
  static const char* const kDigests32x32[kNumDirectionalIntraPredictors] = {
      "198aabc958992eb49cceab97d1acb43e",
      "aee88b6c8bacfcf38799fe338e6c66e7",
      "01e8f8f96696636f6d79d33951907a16",
  };
  static const char* const kDigests32x64[kNumDirectionalIntraPredictors] = {
      "0611390202c4f90f7add7aec763ded58",
      "960240c7ceda2ccfac7c90b71460578a",
      "7e7d97594aab8ad56e8c01c340335607",
  };
  static const char* const kDigests64x16[kNumDirectionalIntraPredictors] = {
      "7e1f567e7fc510757f2d89d638bc826f",
      "c929d687352ce40a58670be2ce3c8c90",
      "f6881e6a9ba3c3d3d730b425732656b1",
  };
  static const char* const kDigests64x32[kNumDirectionalIntraPredictors] = {
      "27b4c2a7081d4139f22003ba8b6dfdf2",
      "301e82740866b9274108a04c872fa848",
      "98d3aa4fef838f4abf00dac33806659f",
  };
  static const char* const kDigests64x64[kNumDirectionalIntraPredictors] = {
      "b31816db8fade3accfd975b21aa264c7",
      "2adce01a03b9452633d5830e1a9b4e23",
      "7b988fadba8b07c36e88d7be6b270494",
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
    case kTransformSize16x64:
      return kDigests16x64;
    case kTransformSize32x8:
      return kDigests32x8;
    case kTransformSize32x16:
      return kDigests32x16;
    case kTransformSize32x32:
      return kDigests32x32;
    case kTransformSize32x64:
      return kDigests32x64;
    case kTransformSize64x16:
      return kDigests64x16;
    case kTransformSize64x32:
      return kDigests64x32;
    case kTransformSize64x64:
      return kDigests64x64;
    default:
      ADD_FAILURE() << "Unknown transform size: " << tx_size;
      return nullptr;
  }
}

TEST_P(DirectionalIntraPredTest8bpp, DISABLED_Speed) {
#if LIBGAV1_ENABLE_NEON
  const auto num_runs = static_cast<int>(2e5 / (block_width_ * block_height_));
#else
  const int num_runs = static_cast<int>(4e7 / (block_width_ * block_height_));
#endif
  for (int i = kZone1; i < kNumZones; ++i) {
    TestSpeed(GetDirectionalIntraPredDigests8bpp(tx_size_),
              static_cast<Zone>(i), num_runs);
  }
}

TEST_P(DirectionalIntraPredTest8bpp, FixedInput) {
  for (int i = kZone1; i < kNumZones; ++i) {
    TestSpeed(GetDirectionalIntraPredDigests8bpp(tx_size_),
              static_cast<Zone>(i), 1);
  }
}

TEST_P(DirectionalIntraPredTest8bpp, Overflow) { TestSaturatedValues(); }
TEST_P(DirectionalIntraPredTest8bpp, Random) { TestRandomValues(); }

//------------------------------------------------------------------------------

#if LIBGAV1_MAX_BITDEPTH >= 10
using DirectionalIntraPredTest10bpp = DirectionalIntraPredTest<10, uint16_t>;

const char* const* GetDirectionalIntraPredDigests10bpp(TransformSize tx_size) {
  static const char* const kDigests4x4[kNumDirectionalIntraPredictors] = {
      "a683f4d7ccd978737615f61ecb4d638d",
      "90c94374eaf7e9501f197863937b8639",
      "0d3969cd081523ac6a906eecc7980c43",
  };
  static const char* const kDigests4x8[kNumDirectionalIntraPredictors] = {
      "c3ffa2979b325644e4a56c882fe27347",
      "1f61f5ee413a9a3b8d1d93869ec2aee0",
      "4795ea944779ec4a783408769394d874",
  };
  static const char* const kDigests4x16[kNumDirectionalIntraPredictors] = {
      "45c3282c9aa51024c1d64a40f230aa45",
      "5cd47dd69f8bd0b15365a0c5cfc0a49a",
      "06336c507b05f98c1d6a21abc43e6182",
  };
  static const char* const kDigests8x4[kNumDirectionalIntraPredictors] = {
      "7370476ff0abbdc5e92f811b8879c861",
      "a239a50adb28a4791b52a0dfff3bee06",
      "4779a17f958a9ca04e8ec08c5aba1d36",
  };
  static const char* const kDigests8x8[kNumDirectionalIntraPredictors] = {
      "305463f346c376594f82aad8304e0362",
      "0cd481e5bda286c87a645417569fd948",
      "48c7899dc9b7163b0b1f61b3a2b4b73e",
  };
  static const char* const kDigests8x16[kNumDirectionalIntraPredictors] = {
      "5c18fd5339be90628c82b1fb6af50d5e",
      "35eaa566ebd3bb7c903cfead5dc9ac78",
      "9fdb0e790e5965810d02c02713c84071",
  };
  static const char* const kDigests8x32[kNumDirectionalIntraPredictors] = {
      "2168d6cc858c704748b7b343ced2ac3a",
      "1d3ce273107447faafd2e55877e48ffb",
      "d344164049d1fe9b65a3ae8764bbbd37",
  };
  static const char* const kDigests16x4[kNumDirectionalIntraPredictors] = {
      "dcef2cf51abe3fe150f388a14c762d30",
      "6a810b289b1c14f8eab8ca1274e91ecd",
      "c94da7c11f3fb11963d85c8804fce2d9",
  };
  static const char* const kDigests16x8[kNumDirectionalIntraPredictors] = {
      "50a0d08b0d99b7a574bad2cfb36efc39",
      "2dcb55874db39da70c8ca1318559f9fe",
      "6390bcd30ff3bc389ecc0a0952bea531",
  };
  static const char* const kDigests16x16[kNumDirectionalIntraPredictors] = {
      "7146c83c2620935606d49f3cb5876f41",
      "2318ddf30c070a53c9b9cf199cd1b2c5",
      "e9042e2124925aa7c1b6110617cb10e8",
  };
  static const char* const kDigests16x32[kNumDirectionalIntraPredictors] = {
      "c970f401de7b7c5bb4e3ad447fcbef8f",
      "a18cc70730eecdaa31dbcf4306ff490f",
      "32c1528ad4a576a2210399d6b4ccd46e",
  };
  static const char* const kDigests16x64[kNumDirectionalIntraPredictors] = {
      "00b3f0007da2e5d01380594a3d7162d5",
      "1971af519e4a18967b7311f93efdd1b8",
      "e6139769ce5a9c4982cfab9363004516",
  };
  static const char* const kDigests32x8[kNumDirectionalIntraPredictors] = {
      "08107ad971179cc9f465ae5966bd4901",
      "b215212a3c0dfe9182c4f2e903d731f7",
      "791274416a0da87c674e1ae318b3ce09",
  };
  static const char* const kDigests32x16[kNumDirectionalIntraPredictors] = {
      "94ea6cccae35b5d08799aa003ac08ccf",
      "ae105e20e63fb55d4fd9d9e59dc62dde",
      "973d0b2358ea585e4f486e7e645c5310",
  };
  static const char* const kDigests32x32[kNumDirectionalIntraPredictors] = {
      "d14c695c4853ddf5e5d8256bc1d1ed60",
      "6bd0ebeb53adecc11442b1218b870cb7",
      "e03bc402a9999aba8272275dce93e89f",
  };
  static const char* const kDigests32x64[kNumDirectionalIntraPredictors] = {
      "b21a8a8723758392ee659eeeae518a1e",
      "e50285454896210ce44d6f04dfde05a7",
      "f0f8ea0c6c2acc8d7d390927c3a90370",
  };
  static const char* const kDigests64x16[kNumDirectionalIntraPredictors] = {
      "ce51db16fd4fa56e601631397b098c89",
      "aa87a8635e02c1e91d13158c61e443f6",
      "4c1ee3afd46ef34bd711a34d0bf86f13",
  };
  static const char* const kDigests64x32[kNumDirectionalIntraPredictors] = {
      "25aaf5971e24e543e3e69a47254af777",
      "eb6f444b3df127d69460778ab5bf8fc1",
      "2f846cc0d506f90c0a58438600819817",
  };
  static const char* const kDigests64x64[kNumDirectionalIntraPredictors] = {
      "b26ce5b5f4b5d4a438b52e5987877fb8",
      "35721a00a70938111939cf69988d928e",
      "0af7ec35939483fac82c246a13845806",
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
    case kTransformSize16x64:
      return kDigests16x64;
    case kTransformSize32x8:
      return kDigests32x8;
    case kTransformSize32x16:
      return kDigests32x16;
    case kTransformSize32x32:
      return kDigests32x32;
    case kTransformSize32x64:
      return kDigests32x64;
    case kTransformSize64x16:
      return kDigests64x16;
    case kTransformSize64x32:
      return kDigests64x32;
    case kTransformSize64x64:
      return kDigests64x64;
    default:
      ADD_FAILURE() << "Unknown transform size: " << tx_size;
      return nullptr;
  }
}

TEST_P(DirectionalIntraPredTest10bpp, DISABLED_Speed) {
#if LIBGAV1_ENABLE_NEON
  const int num_runs = static_cast<int>(2e5 / (block_width_ * block_height_));
#else
  const int num_runs = static_cast<int>(4e7 / (block_width_ * block_height_));
#endif
  for (int i = kZone1; i < kNumZones; ++i) {
    TestSpeed(GetDirectionalIntraPredDigests10bpp(tx_size_),
              static_cast<Zone>(i), num_runs);
  }
}

TEST_P(DirectionalIntraPredTest10bpp, FixedInput) {
  for (int i = kZone1; i < kNumZones; ++i) {
    TestSpeed(GetDirectionalIntraPredDigests10bpp(tx_size_),
              static_cast<Zone>(i), 1);
  }
}

TEST_P(DirectionalIntraPredTest10bpp, Overflow) { TestSaturatedValues(); }
TEST_P(DirectionalIntraPredTest10bpp, Random) { TestRandomValues(); }
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

//------------------------------------------------------------------------------

#if LIBGAV1_MAX_BITDEPTH == 12
using DirectionalIntraPredTest12bpp = DirectionalIntraPredTest<12, uint16_t>;

const char* const* GetDirectionalIntraPredDigests12bpp(TransformSize tx_size) {
  static const char* const kDigests4x4[kNumDirectionalIntraPredictors] = {
      "78f3297743f75e928e755b6ffa2d3050",
      "7315da39861c6e3ef2e47c913e3be349",
      "5609cb40b575f24d05880df202a60bd3",
  };
  static const char* const kDigests4x8[kNumDirectionalIntraPredictors] = {
      "efb2363d3c25427abe198806c8ba4d6b",
      "b5aaa41665a10e7e7944fb7fc90fd59a",
      "5a85610342339ca3109d775fa18dc25c",
  };
  static const char* const kDigests4x16[kNumDirectionalIntraPredictors] = {
      "9045679914980ea1f579d84509397b6e",
      "f9f50bdc9f81a93095fd9d6998174aa7",
      "46c1f82e85b8ba5b03bab41a2f561483",
  };
  static const char* const kDigests8x4[kNumDirectionalIntraPredictors] = {
      "a0ae0956b2b667c528b7803d733d49da",
      "5d9f60ef8904c4faedb6cfc19e54418a",
      "4ffdcbbbcb23bca8286f1c286b9cb3e8",
  };
  static const char* const kDigests8x8[kNumDirectionalIntraPredictors] = {
      "086116c6b116613b8b47a086726566ea",
      "141dca7fcae0e4d4b88887a618271ea1",
      "3575a34278aa0fb1eed934290982f4a7",
  };
  static const char* const kDigests8x16[kNumDirectionalIntraPredictors] = {
      "7922f40216c78a40abaf675667e79493",
      "55d20588240171df2e24d105ee1563ad",
      "674b4d8f4dbf514d22e21cc4baeda1d3",
  };
  static const char* const kDigests8x32[kNumDirectionalIntraPredictors] = {
      "32d4d7e256d3b304026ddb5430cf6a09",
      "72f4be2569f4e067c252d51ff4030de3",
      "6779a132e1bac0ac43c2373f56553ed8",
  };
  static const char* const kDigests16x4[kNumDirectionalIntraPredictors] = {
      "1be2e0efc1403f9e22cfb8aeb28763d9",
      "558c8a5418ac91d21a5839c454a9391f",
      "7693ebef9b86416ebd6e78e98fcafba7",
  };
  static const char* const kDigests16x8[kNumDirectionalIntraPredictors] = {
      "e6217ed1c673ae42e84f8757316b580d",
      "028aa582c11a9733f0cd693211a067c5",
      "082de9fc7c4bc80a8ec8522b5a5cb52c",
  };
  static const char* const kDigests16x16[kNumDirectionalIntraPredictors] = {
      "e3b293c09bdc9c5c543ad046a3f0d64f",
      "2de5803a6ed497c1039c8e6d675c1dd3",
      "05742f807560f5d5206e54b70097dc4a",
  };
  static const char* const kDigests16x32[kNumDirectionalIntraPredictors] = {
      "57f2ca4ba56be253eff7e6b73df5003d",
      "ef8bea00437e01fb798a22cda59f0191",
      "989ff38c96600c2f108d6e6fa381fd13",
  };
  static const char* const kDigests16x64[kNumDirectionalIntraPredictors] = {
      "f5540f4874c02aa2222a3ba75106f841",
      "17e5d20f798a96c39abc8a81e7aa7bc6",
      "0fe9ea14c9dcae466b4a36f1c7db6978",
  };
  static const char* const kDigests32x8[kNumDirectionalIntraPredictors] = {
      "aff9429951ab1885c0d9ed29aa1b6a9f",
      "4b686e2a879bf0b4aadd06b412e0eb48",
      "39325d71cddc272bfa1dd2dc80d09ffe",
  };
  static const char* const kDigests32x16[kNumDirectionalIntraPredictors] = {
      "b83dffdf8bad2b7c3808925b6138ca1e",
      "3656b58c7aaf2025979b4a3ed8a2841e",
      "cfcc0c6ae3fa5e7d45dec581479459f6",
  };
  static const char* const kDigests32x32[kNumDirectionalIntraPredictors] = {
      "3c91b3b9e2df73ffb718e0bf53c5a5c2",
      "0dbe27603e111158e70d99e181befb83",
      "edecbffb32ae1e49b66b6e55ad0af6c6",
  };
  static const char* const kDigests32x64[kNumDirectionalIntraPredictors] = {
      "a3290917f755c7ccdc7b77eb3c6c89a7",
      "42f89db41fbb366ddb78ef79a043f3e3",
      "7f7bcbe33aa003b166677c68d12490e9",
  };
  static const char* const kDigests64x16[kNumDirectionalIntraPredictors] = {
      "d4f4c6b70a82695f843e9227bd7d9cc8",
      "550a0bd87936801651d552e229b683e9",
      "a4c730ad71f566a930c5672e1b2f48f1",
  };
  static const char* const kDigests64x32[kNumDirectionalIntraPredictors] = {
      "2087c9264c4c5fea9a6fe20dcedbe2b9",
      "d4dd51d9578a3fc2eb75086fba867c22",
      "6121a67d63e40107e780d0938aeb3d21",
  };
  static const char* const kDigests64x64[kNumDirectionalIntraPredictors] = {
      "09c3818a07bc54467634c2bfce66f58f",
      "8da453b8d72d73d71ba15a14ddd59db4",
      "9bc939aa54445722469b120b8a505cb3",
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
    case kTransformSize16x64:
      return kDigests16x64;
    case kTransformSize32x8:
      return kDigests32x8;
    case kTransformSize32x16:
      return kDigests32x16;
    case kTransformSize32x32:
      return kDigests32x32;
    case kTransformSize32x64:
      return kDigests32x64;
    case kTransformSize64x16:
      return kDigests64x16;
    case kTransformSize64x32:
      return kDigests64x32;
    case kTransformSize64x64:
      return kDigests64x64;
    default:
      ADD_FAILURE() << "Unknown transform size: " << tx_size;
      return nullptr;
  }
}

TEST_P(DirectionalIntraPredTest12bpp, DISABLED_Speed) {
#if LIBGAV1_ENABLE_NEON
  const int num_runs = static_cast<int>(2e7 / (block_width_ * block_height_));
#else
  const int num_runs = static_cast<int>(4e7 / (block_width_ * block_height_));
#endif
  for (int i = kZone1; i < kNumZones; ++i) {
    TestSpeed(GetDirectionalIntraPredDigests12bpp(tx_size_),
              static_cast<Zone>(i), num_runs);
  }
}

TEST_P(DirectionalIntraPredTest12bpp, FixedInput) {
  for (int i = kZone1; i < kNumZones; ++i) {
    TestSpeed(GetDirectionalIntraPredDigests12bpp(tx_size_),
              static_cast<Zone>(i), 1);
  }
}

TEST_P(DirectionalIntraPredTest12bpp, Overflow) { TestSaturatedValues(); }
TEST_P(DirectionalIntraPredTest12bpp, Random) { TestRandomValues(); }
#endif  // LIBGAV1_MAX_BITDEPTH == 12

constexpr TransformSize kTransformSizes[] = {
    kTransformSize4x4,   kTransformSize4x8,   kTransformSize4x16,
    kTransformSize8x4,   kTransformSize8x8,   kTransformSize8x16,
    kTransformSize8x32,  kTransformSize16x4,  kTransformSize16x8,
    kTransformSize16x16, kTransformSize16x32, kTransformSize16x64,
    kTransformSize32x8,  kTransformSize32x16, kTransformSize32x32,
    kTransformSize32x64, kTransformSize64x16, kTransformSize64x32,
    kTransformSize64x64};

INSTANTIATE_TEST_SUITE_P(C, DirectionalIntraPredTest8bpp,
                         testing::ValuesIn(kTransformSizes));
#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, DirectionalIntraPredTest8bpp,
                         testing::ValuesIn(kTransformSizes));
#endif  // LIBGAV1_ENABLE_SSE4_1
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, DirectionalIntraPredTest8bpp,
                         testing::ValuesIn(kTransformSizes));
#endif  // LIBGAV1_ENABLE_NEON

#if LIBGAV1_MAX_BITDEPTH >= 10
INSTANTIATE_TEST_SUITE_P(C, DirectionalIntraPredTest10bpp,
                         testing::ValuesIn(kTransformSizes));
#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, DirectionalIntraPredTest10bpp,
                         testing::ValuesIn(kTransformSizes));
#endif  // LIBGAV1_ENABLE_SSE4_1
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, DirectionalIntraPredTest10bpp,
                         testing::ValuesIn(kTransformSizes));
#endif  // LIBGAV1_ENABLE_NEON
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
INSTANTIATE_TEST_SUITE_P(C, DirectionalIntraPredTest12bpp,
                         testing::ValuesIn(kTransformSizes));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace
}  // namespace dsp

static std::ostream& operator<<(std::ostream& os, const TransformSize tx_size) {
  return os << ToString(tx_size);
}

}  // namespace libgav1
