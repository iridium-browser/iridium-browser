// Copyright 2019 The libgav1 Authors
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

#include "src/dsp/film_grain.h"
#include "src/utils/cpu.h"

#if LIBGAV1_ENABLE_NEON
#include <arm_neon.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "src/dsp/arm/common_neon.h"
#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/dsp/film_grain_common.h"
#include "src/utils/array_2d.h"
#include "src/utils/common.h"
#include "src/utils/compiler_attributes.h"
#include "src/utils/constants.h"
#include "src/utils/memory.h"
#include "src/utils/types.h"

namespace libgav1 {
namespace dsp {
namespace film_grain {
namespace {

// These functions are overloaded for both possible sizes in order to simplify
// loading and storing to and from intermediate value types from within a
// template function.
inline int16x8_t GetSignedSource8(const int8_t* src) {
  return vmovl_s8(vld1_s8(src));
}

inline int16x8_t GetSignedSource8(const uint8_t* src) {
  return ZeroExtend(vld1_u8(src));
}

inline int16x8_t GetSignedSource8Msan(const uint8_t* src, int valid_range) {
  return ZeroExtend(Load1MsanU8(src, 8 - valid_range));
}

inline void StoreUnsigned8(uint8_t* dest, const uint16x8_t data) {
  vst1_u8(dest, vmovn_u16(data));
}

#if LIBGAV1_MAX_BITDEPTH >= 10
inline int16x8_t GetSignedSource8(const int16_t* src) { return vld1q_s16(src); }

inline int16x8_t GetSignedSource8(const uint16_t* src) {
  return vreinterpretq_s16_u16(vld1q_u16(src));
}

inline int16x8_t GetSignedSource8Msan(const uint16_t* src, int valid_range) {
  return vreinterpretq_s16_u16(Load1QMsanU16(src, 16 - valid_range));
}

inline void StoreUnsigned8(uint16_t* dest, const uint16x8_t data) {
  vst1q_u16(dest, data);
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

// Each element in |sum| represents one destination value's running
// autoregression formula. The fixed source values in |grain_lo| and |grain_hi|
// allow for a sliding window in successive calls to this function.
template <int position_offset>
inline int32x4x2_t AccumulateWeightedGrain(const int16x8_t grain_lo,
                                           const int16x8_t grain_hi,
                                           int16_t coeff, int32x4x2_t sum) {
  const int16x8_t grain = vextq_s16(grain_lo, grain_hi, position_offset);
  sum.val[0] = vmlal_n_s16(sum.val[0], vget_low_s16(grain), coeff);
  sum.val[1] = vmlal_n_s16(sum.val[1], vget_high_s16(grain), coeff);
  return sum;
}

// Because the autoregressive filter requires the output of each pixel to
// compute pixels that come after in the row, we have to finish the calculations
// one at a time.
template <int bitdepth, int auto_regression_coeff_lag, int lane>
inline void WriteFinalAutoRegression(int8_t* LIBGAV1_RESTRICT grain_cursor,
                                     int32x4x2_t sum,
                                     const int8_t* LIBGAV1_RESTRICT coeffs,
                                     int pos, int shift) {
  int32_t result = vgetq_lane_s32(sum.val[lane >> 2], lane & 3);

  for (int delta_col = -auto_regression_coeff_lag; delta_col < 0; ++delta_col) {
    result += grain_cursor[lane + delta_col] * coeffs[pos];
    ++pos;
  }
  grain_cursor[lane] =
      Clip3(grain_cursor[lane] + RightShiftWithRounding(result, shift),
            GetGrainMin<bitdepth>(), GetGrainMax<bitdepth>());
}

#if LIBGAV1_MAX_BITDEPTH >= 10
template <int bitdepth, int auto_regression_coeff_lag, int lane>
inline void WriteFinalAutoRegression(int16_t* LIBGAV1_RESTRICT grain_cursor,
                                     int32x4x2_t sum,
                                     const int8_t* LIBGAV1_RESTRICT coeffs,
                                     int pos, int shift) {
  int32_t result = vgetq_lane_s32(sum.val[lane >> 2], lane & 3);

  for (int delta_col = -auto_regression_coeff_lag; delta_col < 0; ++delta_col) {
    result += grain_cursor[lane + delta_col] * coeffs[pos];
    ++pos;
  }
  grain_cursor[lane] =
      Clip3(grain_cursor[lane] + RightShiftWithRounding(result, shift),
            GetGrainMin<bitdepth>(), GetGrainMax<bitdepth>());
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

// Because the autoregressive filter requires the output of each pixel to
// compute pixels that come after in the row, we have to finish the calculations
// one at a time.
template <int bitdepth, int auto_regression_coeff_lag, int lane>
inline void WriteFinalAutoRegressionChroma(
    int8_t* LIBGAV1_RESTRICT u_grain_cursor,
    int8_t* LIBGAV1_RESTRICT v_grain_cursor, int32x4x2_t sum_u,
    int32x4x2_t sum_v, const int8_t* LIBGAV1_RESTRICT coeffs_u,
    const int8_t* LIBGAV1_RESTRICT coeffs_v, int pos, int shift) {
  WriteFinalAutoRegression<bitdepth, auto_regression_coeff_lag, lane>(
      u_grain_cursor, sum_u, coeffs_u, pos, shift);
  WriteFinalAutoRegression<bitdepth, auto_regression_coeff_lag, lane>(
      v_grain_cursor, sum_v, coeffs_v, pos, shift);
}

#if LIBGAV1_MAX_BITDEPTH >= 10
template <int bitdepth, int auto_regression_coeff_lag, int lane>
inline void WriteFinalAutoRegressionChroma(
    int16_t* LIBGAV1_RESTRICT u_grain_cursor,
    int16_t* LIBGAV1_RESTRICT v_grain_cursor, int32x4x2_t sum_u,
    int32x4x2_t sum_v, const int8_t* LIBGAV1_RESTRICT coeffs_u,
    const int8_t* LIBGAV1_RESTRICT coeffs_v, int pos, int shift) {
  WriteFinalAutoRegression<bitdepth, auto_regression_coeff_lag, lane>(
      u_grain_cursor, sum_u, coeffs_u, pos, shift);
  WriteFinalAutoRegression<bitdepth, auto_regression_coeff_lag, lane>(
      v_grain_cursor, sum_v, coeffs_v, pos, shift);
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

inline void SetZero(int32x4x2_t* v) {
  v->val[0] = vdupq_n_s32(0);
  v->val[1] = vdupq_n_s32(0);
}

// Computes subsampled luma for use with chroma, by averaging in the x direction
// or y direction when applicable.
int16x8_t GetSubsampledLuma(const int8_t* const luma, int subsampling_x,
                            int subsampling_y, ptrdiff_t stride) {
  if (subsampling_y != 0) {
    assert(subsampling_x != 0);
    const int8x16_t src0 = vld1q_s8(luma);
    const int8x16_t src1 = vld1q_s8(luma + stride);
    const int16x8_t ret0 = vcombine_s16(vpaddl_s8(vget_low_s8(src0)),
                                        vpaddl_s8(vget_high_s8(src0)));
    const int16x8_t ret1 = vcombine_s16(vpaddl_s8(vget_low_s8(src1)),
                                        vpaddl_s8(vget_high_s8(src1)));
    return vrshrq_n_s16(vaddq_s16(ret0, ret1), 2);
  }
  if (subsampling_x != 0) {
    const int8x16_t src = vld1q_s8(luma);
    return vrshrq_n_s16(
        vcombine_s16(vpaddl_s8(vget_low_s8(src)), vpaddl_s8(vget_high_s8(src))),
        1);
  }
  return vmovl_s8(vld1_s8(luma));
}

// For BlendNoiseWithImageChromaWithCfl, only |subsampling_x| is needed.
inline uint16x8_t GetAverageLuma(const uint8_t* const luma, int subsampling_x) {
  if (subsampling_x != 0) {
    const uint8x16_t src = vld1q_u8(luma);
    return vrshrq_n_u16(vpaddlq_u8(src), 1);
  }
  return vmovl_u8(vld1_u8(luma));
}

inline uint16x8_t GetAverageLumaMsan(const uint8_t* const luma,
                                     int subsampling_x, int valid_range) {
  if (subsampling_x != 0) {
    const uint8x16_t src = MaskOverreadsQ(vld1q_u8(luma), 16 - valid_range);
    // MemorySanitizer registers vpaddlq_u8 as a use of the memory.
    return vrshrq_n_u16(vpaddlq_u8(src), 1);
  }
  return MaskOverreadsQ(vmovl_u8(vld1_u8(luma)), 16 - valid_range);
}

#if LIBGAV1_MAX_BITDEPTH >= 10
// Computes subsampled luma for use with chroma, by averaging in the x direction
// or y direction when applicable.
int16x8_t GetSubsampledLuma(const int16_t* const luma, int subsampling_x,
                            int subsampling_y, ptrdiff_t stride) {
  if (subsampling_y != 0) {
    assert(subsampling_x != 0);
    int16x8_t src0_lo = vld1q_s16(luma);
    int16x8_t src0_hi = vld1q_s16(luma + 8);
    const int16x8_t src1_lo = vld1q_s16(luma + stride);
    const int16x8_t src1_hi = vld1q_s16(luma + stride + 8);
    const int16x8_t src0 =
        vcombine_s16(vpadd_s16(vget_low_s16(src0_lo), vget_high_s16(src0_lo)),
                     vpadd_s16(vget_low_s16(src0_hi), vget_high_s16(src0_hi)));
    const int16x8_t src1 =
        vcombine_s16(vpadd_s16(vget_low_s16(src1_lo), vget_high_s16(src1_lo)),
                     vpadd_s16(vget_low_s16(src1_hi), vget_high_s16(src1_hi)));
    return vrshrq_n_s16(vaddq_s16(src0, src1), 2);
  }
  if (subsampling_x != 0) {
    const int16x8_t src_lo = vld1q_s16(luma);
    const int16x8_t src_hi = vld1q_s16(luma + 8);
    const int16x8_t ret =
        vcombine_s16(vpadd_s16(vget_low_s16(src_lo), vget_high_s16(src_lo)),
                     vpadd_s16(vget_low_s16(src_hi), vget_high_s16(src_hi)));
    return vrshrq_n_s16(ret, 1);
  }
  return vld1q_s16(luma);
}

// For BlendNoiseWithImageChromaWithCfl, only |subsampling_x| is needed.
inline uint16x8_t GetAverageLuma(const uint16_t* const luma,
                                 int subsampling_x) {
  if (subsampling_x != 0) {
    const uint16x8x2_t src = vld2q_u16(luma);
    return vrhaddq_u16(src.val[0], src.val[1]);
  }
  return vld1q_u16(luma);
}

inline uint16x8_t GetAverageLumaMsan(const uint16_t* const luma,
                                     int subsampling_x, int valid_range) {
  if (subsampling_x != 0) {
    const uint16x8x2_t src = vld2q_u16(luma);
    const uint16x8_t result = vrhaddq_u16(src.val[0], src.val[1]);
    return MaskOverreadsQ(result, 16 - valid_range);
  }
  return Load1QMsanU16(luma, 16 - valid_range);
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

template <int bitdepth, typename GrainType, int auto_regression_coeff_lag,
          bool use_luma>
void ApplyAutoRegressiveFilterToChromaGrains_NEON(
    const FilmGrainParams& params,
    const void* LIBGAV1_RESTRICT luma_grain_buffer, int subsampling_x,
    int subsampling_y, void* LIBGAV1_RESTRICT u_grain_buffer,
    void* LIBGAV1_RESTRICT v_grain_buffer) {
  static_assert(auto_regression_coeff_lag <= 3, "Invalid autoregression lag.");
  const auto* luma_grain = static_cast<const GrainType*>(luma_grain_buffer);
  auto* u_grain = static_cast<GrainType*>(u_grain_buffer);
  auto* v_grain = static_cast<GrainType*>(v_grain_buffer);
  const int auto_regression_shift = params.auto_regression_shift;
  const int chroma_width =
      (subsampling_x == 0) ? kMaxChromaWidth : kMinChromaWidth;
  const int chroma_height =
      (subsampling_y == 0) ? kMaxChromaHeight : kMinChromaHeight;
  // When |chroma_width| == 44, we write 8 at a time from x in [3, 34],
  // leaving [35, 40] to write at the end.
  const int chroma_width_remainder =
      (chroma_width - 2 * kAutoRegressionBorder) & 7;

  int y = kAutoRegressionBorder;
  luma_grain += kLumaWidth * y;
  u_grain += chroma_width * y;
  v_grain += chroma_width * y;
  do {
    // Each row is computed 8 values at a time in the following loop. At the
    // end of the loop, 4 values remain to write. They are given a special
    // reduced iteration at the end.
    int x = kAutoRegressionBorder;
    int luma_x = kAutoRegressionBorder;
    do {
      int pos = 0;
      int32x4x2_t sum_u;
      int32x4x2_t sum_v;
      SetZero(&sum_u);
      SetZero(&sum_v);

      if (auto_regression_coeff_lag > 0) {
        for (int delta_row = -auto_regression_coeff_lag; delta_row < 0;
             ++delta_row) {
          // These loads may overflow to the next row, but they are never called
          // on the final row of a grain block. Therefore, they will never
          // exceed the block boundaries.
          // Note: this could be slightly optimized to a single load in 8bpp,
          // but requires making a special first iteration and accumulate
          // function that takes an int8x16_t.
          const int16x8_t u_grain_lo =
              GetSignedSource8(u_grain + x + delta_row * chroma_width -
                               auto_regression_coeff_lag);
          const int16x8_t u_grain_hi =
              GetSignedSource8(u_grain + x + delta_row * chroma_width -
                               auto_regression_coeff_lag + 8);
          const int16x8_t v_grain_lo =
              GetSignedSource8(v_grain + x + delta_row * chroma_width -
                               auto_regression_coeff_lag);
          const int16x8_t v_grain_hi =
              GetSignedSource8(v_grain + x + delta_row * chroma_width -
                               auto_regression_coeff_lag + 8);
#define ACCUMULATE_WEIGHTED_GRAIN(offset)                                  \
  sum_u = AccumulateWeightedGrain<offset>(                                 \
      u_grain_lo, u_grain_hi, params.auto_regression_coeff_u[pos], sum_u); \
  sum_v = AccumulateWeightedGrain<offset>(                                 \
      v_grain_lo, v_grain_hi, params.auto_regression_coeff_v[pos++], sum_v)

          ACCUMULATE_WEIGHTED_GRAIN(0);
          ACCUMULATE_WEIGHTED_GRAIN(1);
          ACCUMULATE_WEIGHTED_GRAIN(2);
          // The horizontal |auto_regression_coeff_lag| loop is replaced with
          // if-statements to give vextq_s16 an immediate param.
          if (auto_regression_coeff_lag > 1) {
            ACCUMULATE_WEIGHTED_GRAIN(3);
            ACCUMULATE_WEIGHTED_GRAIN(4);
          }
          if (auto_regression_coeff_lag > 2) {
            assert(auto_regression_coeff_lag == 3);
            ACCUMULATE_WEIGHTED_GRAIN(5);
            ACCUMULATE_WEIGHTED_GRAIN(6);
          }
        }
      }

      if (use_luma) {
        const int16x8_t luma = GetSubsampledLuma(
            luma_grain + luma_x, subsampling_x, subsampling_y, kLumaWidth);

        // Luma samples get the final coefficient in the formula, but are best
        // computed all at once before the final row.
        const int coeff_u =
            params.auto_regression_coeff_u[pos + auto_regression_coeff_lag];
        const int coeff_v =
            params.auto_regression_coeff_v[pos + auto_regression_coeff_lag];

        sum_u.val[0] = vmlal_n_s16(sum_u.val[0], vget_low_s16(luma), coeff_u);
        sum_u.val[1] = vmlal_n_s16(sum_u.val[1], vget_high_s16(luma), coeff_u);
        sum_v.val[0] = vmlal_n_s16(sum_v.val[0], vget_low_s16(luma), coeff_v);
        sum_v.val[1] = vmlal_n_s16(sum_v.val[1], vget_high_s16(luma), coeff_v);
      }
      // At this point in the filter, the source addresses and destination
      // addresses overlap. Because this is an auto-regressive filter, the
      // higher lanes cannot be computed without the results of the lower lanes.
      // Each call to WriteFinalAutoRegression incorporates preceding values
      // on the final row, and writes a single sample. This allows the next
      // pixel's value to be computed in the next call.
#define WRITE_AUTO_REGRESSION_RESULT(lane)                                    \
  WriteFinalAutoRegressionChroma<bitdepth, auto_regression_coeff_lag, lane>(  \
      u_grain + x, v_grain + x, sum_u, sum_v, params.auto_regression_coeff_u, \
      params.auto_regression_coeff_v, pos, auto_regression_shift)

      WRITE_AUTO_REGRESSION_RESULT(0);
      WRITE_AUTO_REGRESSION_RESULT(1);
      WRITE_AUTO_REGRESSION_RESULT(2);
      WRITE_AUTO_REGRESSION_RESULT(3);
      WRITE_AUTO_REGRESSION_RESULT(4);
      WRITE_AUTO_REGRESSION_RESULT(5);
      WRITE_AUTO_REGRESSION_RESULT(6);
      WRITE_AUTO_REGRESSION_RESULT(7);

      x += 8;
      luma_x += 8 << subsampling_x;
    } while (x < chroma_width - kAutoRegressionBorder - chroma_width_remainder);

    // This is the "final iteration" of the above loop over width. We fill in
    // the remainder of the width, which is less than 8.
    int pos = 0;
    int32x4x2_t sum_u;
    int32x4x2_t sum_v;
    SetZero(&sum_u);
    SetZero(&sum_v);

    for (int delta_row = -auto_regression_coeff_lag; delta_row < 0;
         ++delta_row) {
      // These loads may overflow to the next row, but they are never called on
      // the final row of a grain block. Therefore, they will never exceed the
      // block boundaries.
      const int16x8_t u_grain_lo = GetSignedSource8(
          u_grain + x + delta_row * chroma_width - auto_regression_coeff_lag);
      const int16x8_t u_grain_hi =
          GetSignedSource8(u_grain + x + delta_row * chroma_width -
                           auto_regression_coeff_lag + 8);
      const int16x8_t v_grain_lo = GetSignedSource8(
          v_grain + x + delta_row * chroma_width - auto_regression_coeff_lag);
      const int16x8_t v_grain_hi =
          GetSignedSource8(v_grain + x + delta_row * chroma_width -
                           auto_regression_coeff_lag + 8);

      ACCUMULATE_WEIGHTED_GRAIN(0);
      ACCUMULATE_WEIGHTED_GRAIN(1);
      ACCUMULATE_WEIGHTED_GRAIN(2);
      // The horizontal |auto_regression_coeff_lag| loop is replaced with
      // if-statements to give vextq_s16 an immediate param.
      if (auto_regression_coeff_lag > 1) {
        ACCUMULATE_WEIGHTED_GRAIN(3);
        ACCUMULATE_WEIGHTED_GRAIN(4);
      }
      if (auto_regression_coeff_lag > 2) {
        assert(auto_regression_coeff_lag == 3);
        ACCUMULATE_WEIGHTED_GRAIN(5);
        ACCUMULATE_WEIGHTED_GRAIN(6);
      }
    }

    if (use_luma) {
      const int16x8_t luma = GetSubsampledLuma(
          luma_grain + luma_x, subsampling_x, subsampling_y, kLumaWidth);

      // Luma samples get the final coefficient in the formula, but are best
      // computed all at once before the final row.
      const int coeff_u =
          params.auto_regression_coeff_u[pos + auto_regression_coeff_lag];
      const int coeff_v =
          params.auto_regression_coeff_v[pos + auto_regression_coeff_lag];

      sum_u.val[0] = vmlal_n_s16(sum_u.val[0], vget_low_s16(luma), coeff_u);
      sum_u.val[1] = vmlal_n_s16(sum_u.val[1], vget_high_s16(luma), coeff_u);
      sum_v.val[0] = vmlal_n_s16(sum_v.val[0], vget_low_s16(luma), coeff_v);
      sum_v.val[1] = vmlal_n_s16(sum_v.val[1], vget_high_s16(luma), coeff_v);
    }

    WRITE_AUTO_REGRESSION_RESULT(0);
    WRITE_AUTO_REGRESSION_RESULT(1);
    WRITE_AUTO_REGRESSION_RESULT(2);
    WRITE_AUTO_REGRESSION_RESULT(3);
    if (chroma_width_remainder == 6) {
      WRITE_AUTO_REGRESSION_RESULT(4);
      WRITE_AUTO_REGRESSION_RESULT(5);
    }

    luma_grain += kLumaWidth << subsampling_y;
    u_grain += chroma_width;
    v_grain += chroma_width;
  } while (++y < chroma_height);
#undef ACCUMULATE_WEIGHTED_GRAIN
#undef WRITE_AUTO_REGRESSION_RESULT
}

// Applies an auto-regressive filter to the white noise in luma_grain.
template <int bitdepth, typename GrainType, int auto_regression_coeff_lag>
void ApplyAutoRegressiveFilterToLumaGrain_NEON(const FilmGrainParams& params,
                                               void* luma_grain_buffer) {
  static_assert(auto_regression_coeff_lag > 0, "");
  const int8_t* const auto_regression_coeff_y = params.auto_regression_coeff_y;
  const uint8_t auto_regression_shift = params.auto_regression_shift;

  int y = kAutoRegressionBorder;
  auto* luma_grain =
      static_cast<GrainType*>(luma_grain_buffer) + kLumaWidth * y;
  do {
    // Each row is computed 8 values at a time in the following loop. At the
    // end of the loop, 4 values remain to write. They are given a special
    // reduced iteration at the end.
    int x = kAutoRegressionBorder;
    do {
      int pos = 0;
      int32x4x2_t sum;
      SetZero(&sum);
      for (int delta_row = -auto_regression_coeff_lag; delta_row < 0;
           ++delta_row) {
        // These loads may overflow to the next row, but they are never called
        // on the final row of a grain block. Therefore, they will never exceed
        // the block boundaries.
        const int16x8_t src_grain_lo =
            GetSignedSource8(luma_grain + x + delta_row * kLumaWidth -
                             auto_regression_coeff_lag);
        const int16x8_t src_grain_hi =
            GetSignedSource8(luma_grain + x + delta_row * kLumaWidth -
                             auto_regression_coeff_lag + 8);

        // A pictorial representation of the auto-regressive filter for
        // various values of params.auto_regression_coeff_lag. The letter 'O'
        // represents the current sample. (The filter always operates on the
        // current sample with filter coefficient 1.) The letters 'X'
        // represent the neighboring samples that the filter operates on, below
        // their corresponding "offset" number.
        //
        // params.auto_regression_coeff_lag == 3:
        //   0 1 2 3 4 5 6
        //   X X X X X X X
        //   X X X X X X X
        //   X X X X X X X
        //   X X X O
        // params.auto_regression_coeff_lag == 2:
        //     0 1 2 3 4
        //     X X X X X
        //     X X X X X
        //     X X O
        // params.auto_regression_coeff_lag == 1:
        //       0 1 2
        //       X X X
        //       X O
        // params.auto_regression_coeff_lag == 0:
        //         O
        // The function relies on the caller to skip the call in the 0 lag
        // case.

#define ACCUMULATE_WEIGHTED_GRAIN(offset)                           \
  sum = AccumulateWeightedGrain<offset>(src_grain_lo, src_grain_hi, \
                                        auto_regression_coeff_y[pos++], sum)
        ACCUMULATE_WEIGHTED_GRAIN(0);
        ACCUMULATE_WEIGHTED_GRAIN(1);
        ACCUMULATE_WEIGHTED_GRAIN(2);
        // The horizontal |auto_regression_coeff_lag| loop is replaced with
        // if-statements to give vextq_s16 an immediate param.
        if (auto_regression_coeff_lag > 1) {
          ACCUMULATE_WEIGHTED_GRAIN(3);
          ACCUMULATE_WEIGHTED_GRAIN(4);
        }
        if (auto_regression_coeff_lag > 2) {
          assert(auto_regression_coeff_lag == 3);
          ACCUMULATE_WEIGHTED_GRAIN(5);
          ACCUMULATE_WEIGHTED_GRAIN(6);
        }
      }
      // At this point in the filter, the source addresses and destination
      // addresses overlap. Because this is an auto-regressive filter, the
      // higher lanes cannot be computed without the results of the lower lanes.
      // Each call to WriteFinalAutoRegression incorporates preceding values
      // on the final row, and writes a single sample. This allows the next
      // pixel's value to be computed in the next call.
#define WRITE_AUTO_REGRESSION_RESULT(lane)                             \
  WriteFinalAutoRegression<bitdepth, auto_regression_coeff_lag, lane>( \
      luma_grain + x, sum, auto_regression_coeff_y, pos,               \
      auto_regression_shift)

      WRITE_AUTO_REGRESSION_RESULT(0);
      WRITE_AUTO_REGRESSION_RESULT(1);
      WRITE_AUTO_REGRESSION_RESULT(2);
      WRITE_AUTO_REGRESSION_RESULT(3);
      WRITE_AUTO_REGRESSION_RESULT(4);
      WRITE_AUTO_REGRESSION_RESULT(5);
      WRITE_AUTO_REGRESSION_RESULT(6);
      WRITE_AUTO_REGRESSION_RESULT(7);
      x += 8;
      // Leave the final four pixels for the special iteration below.
    } while (x < kLumaWidth - kAutoRegressionBorder - 4);

    // Final 4 pixels in the row.
    int pos = 0;
    int32x4x2_t sum;
    SetZero(&sum);
    for (int delta_row = -auto_regression_coeff_lag; delta_row < 0;
         ++delta_row) {
      const int16x8_t src_grain_lo = GetSignedSource8(
          luma_grain + x + delta_row * kLumaWidth - auto_regression_coeff_lag);
      const int16x8_t src_grain_hi =
          GetSignedSource8(luma_grain + x + delta_row * kLumaWidth -
                           auto_regression_coeff_lag + 8);

      ACCUMULATE_WEIGHTED_GRAIN(0);
      ACCUMULATE_WEIGHTED_GRAIN(1);
      ACCUMULATE_WEIGHTED_GRAIN(2);
      // The horizontal |auto_regression_coeff_lag| loop is replaced with
      // if-statements to give vextq_s16 an immediate param.
      if (auto_regression_coeff_lag > 1) {
        ACCUMULATE_WEIGHTED_GRAIN(3);
        ACCUMULATE_WEIGHTED_GRAIN(4);
      }
      if (auto_regression_coeff_lag > 2) {
        assert(auto_regression_coeff_lag == 3);
        ACCUMULATE_WEIGHTED_GRAIN(5);
        ACCUMULATE_WEIGHTED_GRAIN(6);
      }
    }
    // delta_row == 0
    WRITE_AUTO_REGRESSION_RESULT(0);
    WRITE_AUTO_REGRESSION_RESULT(1);
    WRITE_AUTO_REGRESSION_RESULT(2);
    WRITE_AUTO_REGRESSION_RESULT(3);
    luma_grain += kLumaWidth;
  } while (++y < kLumaHeight);

#undef WRITE_AUTO_REGRESSION_RESULT
#undef ACCUMULATE_WEIGHTED_GRAIN
}

template <int bitdepth>
void InitializeScalingLookupTable_NEON(int num_points,
                                       const uint8_t point_value[],
                                       const uint8_t point_scaling[],
                                       int16_t* scaling_lut,
                                       const int scaling_lut_length) {
  static_assert(bitdepth < kBitdepth12,
                "NEON Scaling lookup table only supports 8bpp and 10bpp.");
  if (num_points == 0) {
    memset(scaling_lut, 0, sizeof(scaling_lut[0]) * scaling_lut_length);
    return;
  }
  static_assert(sizeof(scaling_lut[0]) == 2, "");
  Memset(scaling_lut, point_scaling[0],
         (static_cast<int>(point_value[0]) + 1) << (bitdepth - kBitdepth8));
  const int32x4_t steps = vmovl_s16(vcreate_s16(0x0003000200010000));
  const int32x4_t rounding = vdupq_n_s32(32768);
  for (int i = 0; i < num_points - 1; ++i) {
    const int delta_y = point_scaling[i + 1] - point_scaling[i];
    const int delta_x = point_value[i + 1] - point_value[i];
    // |delta| corresponds to b, for the function y = a + b*x.
    const int delta = delta_y * ((65536 + (delta_x >> 1)) / delta_x);
    const int delta4 = delta << 2;
    // vmull_n_u16 will not work here because |delta| typically exceeds the
    // range of uint16_t.
    int32x4_t upscaled_points0 = vmlaq_n_s32(rounding, steps, delta);
    const int32x4_t line_increment4 = vdupq_n_s32(delta4);
    // Get the second set of 4 points by adding 4 steps to the first set.
    int32x4_t upscaled_points1 = vaddq_s32(upscaled_points0, line_increment4);
    // We obtain the next set of 8 points by adding 8 steps to each of the
    // current 8 points.
    const int32x4_t line_increment8 = vshlq_n_s32(line_increment4, 1);
    const int16x8_t base_point = vdupq_n_s16(point_scaling[i]);
    int x = 0;
    // Derive and write 8 values (or 32 values, for 10bpp).
    do {
      const int16x4_t interp_points0 = vshrn_n_s32(upscaled_points0, 16);
      const int16x4_t interp_points1 = vshrn_n_s32(upscaled_points1, 16);
      const int16x8_t interp_points =
          vcombine_s16(interp_points0, interp_points1);
      // The spec guarantees that the max value of |point_value[i]| + x is 255.
      // Writing 8 values starting at the final table byte, leaves 7 values of
      // required padding.
      const int16x8_t full_interp = vaddq_s16(interp_points, base_point);
      const int x_base = (point_value[i] + x) << (bitdepth - kBitdepth8);
      if (bitdepth == kBitdepth10) {
        const int16x8_t next_val = vaddq_s16(
            base_point,
            vdupq_n_s16((vgetq_lane_s32(upscaled_points1, 3) + delta) >> 16));
        const int16x8_t start = full_interp;
        const int16x8_t end = vextq_s16(full_interp, next_val, 1);
        // lut[i << 2] = start;
        // lut[(i << 2) + 1] = start + RightShiftWithRounding(start - end, 2)
        // lut[(i << 2) + 2] = start +
        //                      RightShiftWithRounding(2 * (start - end), 2)
        // lut[(i << 2) + 3] = start +
        //                      RightShiftWithRounding(3 * (start - end), 2)
        const int16x8_t delta = vsubq_s16(end, start);
        const int16x8_t double_delta = vshlq_n_s16(delta, 1);
        const int16x8_t delta2 = vrshrq_n_s16(double_delta, 2);
        const int16x8_t delta3 =
            vrshrq_n_s16(vaddq_s16(delta, double_delta), 2);
        const int16x8x4_t result = {
            start, vaddq_s16(start, vrshrq_n_s16(delta, 2)),
            vaddq_s16(start, delta2), vaddq_s16(start, delta3)};
        Store4QMsanS16(&scaling_lut[x_base], result);
      } else {
        vst1q_s16(&scaling_lut[x_base], full_interp);
      }
      upscaled_points0 = vaddq_s32(upscaled_points0, line_increment8);
      upscaled_points1 = vaddq_s32(upscaled_points1, line_increment8);
      x += 8;
    } while (x < delta_x);
  }
  const int16_t last_point_value = point_value[num_points - 1];
  const int x_base = last_point_value << (bitdepth - kBitdepth8);
  Memset(&scaling_lut[x_base], point_scaling[num_points - 1],
         scaling_lut_length - x_base);
  if (bitdepth == kBitdepth10 && x_base > 0) {
    const int start = scaling_lut[x_base - 4];
    const int end = point_scaling[num_points - 1];
    const int delta = end - start;
    scaling_lut[x_base - 3] = start + RightShiftWithRounding(delta, 2);
    scaling_lut[x_base - 2] = start + RightShiftWithRounding(2 * delta, 2);
    scaling_lut[x_base - 1] = start + RightShiftWithRounding(3 * delta, 2);
  }
}

inline int16x8_t Clip3(const int16x8_t value, const int16x8_t low,
                       const int16x8_t high) {
  const int16x8_t clipped_to_ceiling = vminq_s16(high, value);
  return vmaxq_s16(low, clipped_to_ceiling);
}

template <int bitdepth, typename Pixel>
inline int16x8_t GetScalingFactors(const int16_t scaling_lut[],
                                   const Pixel* source,
                                   const int valid_range = 8) {
  int16_t start_vals[8];
  static_assert(bitdepth <= kBitdepth10,
                "NEON Film Grain is not yet implemented for 12bpp.");
#if LIBGAV1_MSAN
  if (valid_range < 8) memset(start_vals, 0, sizeof(start_vals));
#endif
  for (int i = 0; i < valid_range; ++i) {
    assert(source[i] < (kScalingLookupTableSize << (bitdepth - kBitdepth8)));
    start_vals[i] = scaling_lut[source[i]];
  }
  return vld1q_s16(start_vals);
}

template <int bitdepth>
inline int16x8_t ScaleNoise(const int16x8_t noise, const int16x8_t scaling,
                            const int16x8_t scaling_shift_vect) {
  if (bitdepth == kBitdepth8) {
    const int16x8_t upscaled_noise = vmulq_s16(noise, scaling);
    return vrshlq_s16(upscaled_noise, scaling_shift_vect);
  }
  // Scaling shift is in the range [8, 11]. The doubling multiply returning high
  // half is equivalent to a right shift by 15, so |scaling_shift_vect| should
  // provide a left shift equal to 15 - s, where s is the original shift
  // parameter.
  const int16x8_t scaling_up = vshlq_s16(scaling, scaling_shift_vect);
  return vqrdmulhq_s16(noise, scaling_up);
}

template <int bitdepth, typename GrainType, typename Pixel>
void BlendNoiseWithImageLuma_NEON(
    const void* LIBGAV1_RESTRICT noise_image_ptr, int min_value, int max_luma,
    int scaling_shift, int width, int height, int start_height,
    const int16_t* scaling_lut_y, const void* source_plane_y,
    ptrdiff_t source_stride_y, void* dest_plane_y, ptrdiff_t dest_stride_y) {
  const auto* noise_image =
      static_cast<const Array2D<GrainType>*>(noise_image_ptr);
  const auto* in_y_row = static_cast<const Pixel*>(source_plane_y);
  source_stride_y /= sizeof(Pixel);
  auto* out_y_row = static_cast<Pixel*>(dest_plane_y);
  dest_stride_y /= sizeof(Pixel);
  const int16x8_t floor = vdupq_n_s16(min_value);
  const int16x8_t ceiling = vdupq_n_s16(max_luma);
  // In 8bpp, the maximum upscaled noise is 127*255 = 0x7E81, which is safe
  // for 16 bit signed integers. In higher bitdepths, however, we have to
  // expand to 32 to protect the sign bit.
  const int16x8_t scaling_shift_vect = vdupq_n_s16(
      (bitdepth == kBitdepth10) ? 15 - scaling_shift : -scaling_shift);

  const int safe_width = width & ~15;
  int y = 0;
  do {
    int x = 0;
    for (; x + 8 <= safe_width; x += 8) {
      // This operation on the unsigned input is safe in 8bpp because the vector
      // is widened before it is reinterpreted.
      const int16x8_t orig0 = GetSignedSource8(&in_y_row[x]);
      const int16x8_t scaling0 =
          GetScalingFactors<bitdepth, Pixel>(scaling_lut_y, &in_y_row[x]);
      int16x8_t noise =
          GetSignedSource8(&(noise_image[kPlaneY][y + start_height][x]));

      noise = ScaleNoise<bitdepth>(noise, scaling0, scaling_shift_vect);
      const int16x8_t combined0 = vaddq_s16(orig0, noise);
      // In 8bpp, when params_.clip_to_restricted_range == false, we can replace
      // clipping with vqmovun_s16, but it's not likely to be worth copying the
      // function for just that case, though the gain would be very small.
      StoreUnsigned8(&out_y_row[x],
                     vreinterpretq_u16_s16(Clip3(combined0, floor, ceiling)));
      x += 8;

      // This operation on the unsigned input is safe in 8bpp because the vector
      // is widened before it is reinterpreted.
      const int16x8_t orig1 = GetSignedSource8(&in_y_row[x]);
      const int16x8_t scaling1 =
          GetScalingFactors<bitdepth, Pixel>(scaling_lut_y, &in_y_row[x]);
      noise = GetSignedSource8(&(noise_image[kPlaneY][y + start_height][x]));

      noise = ScaleNoise<bitdepth>(noise, scaling1, scaling_shift_vect);
      const int16x8_t combined1 = vaddq_s16(orig1, noise);
      // In 8bpp, when params_.clip_to_restricted_range == false, we can replace
      // clipping with vqmovun_s16, but it's not likely to be worth copying the
      // function for just that case, though the gain would be very small.
      StoreUnsigned8(&out_y_row[x],
                     vreinterpretq_u16_s16(Clip3(combined1, floor, ceiling)));
    }

    if (x < width) {
      assert(width - x < 16);
      if (x < width - 8) {
        const int16x8_t orig = GetSignedSource8(&in_y_row[x]);
        const int16x8_t scaling =
            GetScalingFactors<bitdepth, Pixel>(scaling_lut_y, &in_y_row[x]);
        int16x8_t noise =
            GetSignedSource8(&(noise_image[kPlaneY][y + start_height][x]));

        noise = ScaleNoise<bitdepth>(noise, scaling, scaling_shift_vect);
        const int16x8_t combined = vaddq_s16(orig, noise);
        // In 8bpp, when params_.clip_to_restricted_range == false, we can
        // replace clipping with vqmovun_s16, but it's not likely to be worth
        // copying the function for just that case, though the gain would be
        // very small.
        StoreUnsigned8(&out_y_row[x],
                       vreinterpretq_u16_s16(Clip3(combined, floor, ceiling)));
        x += 8;
      }
      const int valid_range_pixels = width - x;
      const int valid_range_bytes = (width - x) * sizeof(in_y_row[0]);
      const int16x8_t orig =
          GetSignedSource8Msan(&in_y_row[x], valid_range_bytes);
      const int16x8_t scaling = GetScalingFactors<bitdepth, Pixel>(
          scaling_lut_y, &in_y_row[x], valid_range_pixels);
      int16x8_t noise =
          GetSignedSource8(&(noise_image[kPlaneY][y + start_height][x]));
      noise = ScaleNoise<bitdepth>(noise, scaling, scaling_shift_vect);

      const int16x8_t combined = vaddq_s16(orig, noise);
      StoreUnsigned8(&out_y_row[x],
                     vreinterpretq_u16_s16(Clip3(combined, floor, ceiling)));
    }
    in_y_row += source_stride_y;
    out_y_row += dest_stride_y;
  } while (++y < height);
}

template <int bitdepth, typename GrainType, typename Pixel>
inline int16x8_t BlendChromaValsWithCfl(
    const Pixel* LIBGAV1_RESTRICT chroma_cursor,
    const GrainType* LIBGAV1_RESTRICT noise_image_cursor,
    const int16x8_t scaling, const int16x8_t scaling_shift_vect) {
  const int16x8_t orig = GetSignedSource8(chroma_cursor);
  int16x8_t noise = GetSignedSource8(noise_image_cursor);
  noise = ScaleNoise<bitdepth>(noise, scaling, scaling_shift_vect);
  return vaddq_s16(orig, noise);
}

template <int bitdepth, typename GrainType, typename Pixel>
LIBGAV1_ALWAYS_INLINE void BlendChromaPlaneWithCfl_NEON(
    const Array2D<GrainType>& noise_image, int min_value, int max_chroma,
    int width, int height, int start_height, int subsampling_x,
    int subsampling_y, int scaling_shift,
    const int16_t* LIBGAV1_RESTRICT scaling_lut,
    const Pixel* LIBGAV1_RESTRICT in_y_row, ptrdiff_t source_stride_y,
    const Pixel* in_chroma_row, ptrdiff_t source_stride_chroma,
    Pixel* out_chroma_row, ptrdiff_t dest_stride) {
  const int16x8_t floor = vdupq_n_s16(min_value);
  const int16x8_t ceiling = vdupq_n_s16(max_chroma);
  Pixel luma_buffer[16];
  // In 8bpp, the maximum upscaled noise is 127*255 = 0x7E81, which is safe
  // for 16 bit signed integers. In higher bitdepths, however, we have to
  // expand to 32 to protect the sign bit.
  const int16x8_t scaling_shift_vect = vdupq_n_s16(
      (bitdepth == kBitdepth10) ? 15 - scaling_shift : -scaling_shift);

  const int chroma_height = (height + subsampling_y) >> subsampling_y;
  const int chroma_width = (width + subsampling_x) >> subsampling_x;
  const int safe_chroma_width = chroma_width & ~7;

  // Writing to this buffer avoids the cost of doing 8 lane lookups in a row
  // in GetScalingFactors.
  Pixel average_luma_buffer[8];
  assert(start_height % 2 == 0);
  start_height >>= subsampling_y;
  int y = 0;
  do {
    int x = 0;
    for (; x + 8 <= safe_chroma_width; x += 8) {
      const int luma_x = x << subsampling_x;
      const uint16x8_t average_luma =
          GetAverageLuma(&in_y_row[luma_x], subsampling_x);
      StoreUnsigned8(average_luma_buffer, average_luma);

      const int16x8_t scaling =
          GetScalingFactors<bitdepth, Pixel>(scaling_lut, average_luma_buffer);
      const int16x8_t blended =
          BlendChromaValsWithCfl<bitdepth, GrainType, Pixel>(
              &in_chroma_row[x], &(noise_image[y + start_height][x]), scaling,
              scaling_shift_vect);

      // In 8bpp, when params_.clip_to_restricted_range == false, we can replace
      // clipping with vqmovun_s16, but it's not likely to be worth copying the
      // function for just that case.
      StoreUnsigned8(&out_chroma_row[x],
                     vreinterpretq_u16_s16(Clip3(blended, floor, ceiling)));
    }

    if (x < chroma_width) {
      const int luma_x = x << subsampling_x;
      const int valid_range_pixels = width - luma_x;
      const int valid_range_chroma_pixels = chroma_width - x;
      const int valid_range_bytes = valid_range_pixels * sizeof(in_y_row[0]);
      assert(valid_range_pixels < 16);
      memcpy(luma_buffer, &in_y_row[luma_x], valid_range_bytes);
      luma_buffer[valid_range_pixels] = in_y_row[width - 1];
      const uint16x8_t average_luma = GetAverageLumaMsan(
          luma_buffer, subsampling_x, valid_range_chroma_pixels << 1);

      StoreUnsigned8(average_luma_buffer, average_luma);

      const int16x8_t scaling = GetScalingFactors<bitdepth, Pixel>(
          scaling_lut, average_luma_buffer, valid_range_chroma_pixels);
      const int16x8_t blended =
          BlendChromaValsWithCfl<bitdepth, GrainType, Pixel>(
              &in_chroma_row[x], &(noise_image[y + start_height][x]), scaling,
              scaling_shift_vect);
      // In 8bpp, when params_.clip_to_restricted_range == false, we can replace
      // clipping with vqmovun_s16, but it's not likely to be worth copying the
      // function for just that case.
      StoreUnsigned8(&out_chroma_row[x],
                     vreinterpretq_u16_s16(Clip3(blended, floor, ceiling)));
    }

    in_y_row += source_stride_y << subsampling_y;
    in_chroma_row += source_stride_chroma;
    out_chroma_row += dest_stride;
  } while (++y < chroma_height);
}

// This function is for the case params_.chroma_scaling_from_luma == true.
// This further implies that scaling_lut_u == scaling_lut_v == scaling_lut_y.
template <int bitdepth, typename GrainType, typename Pixel>
void BlendNoiseWithImageChromaWithCfl_NEON(
    Plane plane, const FilmGrainParams& params,
    const void* LIBGAV1_RESTRICT noise_image_ptr, int min_value, int max_chroma,
    int width, int height, int start_height, int subsampling_x,
    int subsampling_y, const int16_t* LIBGAV1_RESTRICT scaling_lut,
    const void* LIBGAV1_RESTRICT source_plane_y, ptrdiff_t source_stride_y,
    const void* source_plane_uv, ptrdiff_t source_stride_uv,
    void* dest_plane_uv, ptrdiff_t dest_stride_uv) {
  const auto* noise_image =
      static_cast<const Array2D<GrainType>*>(noise_image_ptr);
  const auto* in_y = static_cast<const Pixel*>(source_plane_y);
  source_stride_y /= sizeof(Pixel);

  const auto* in_uv = static_cast<const Pixel*>(source_plane_uv);
  source_stride_uv /= sizeof(Pixel);
  auto* out_uv = static_cast<Pixel*>(dest_plane_uv);
  dest_stride_uv /= sizeof(Pixel);
  // Looping over one plane at a time is faster in higher resolutions, despite
  // re-computing luma.
  BlendChromaPlaneWithCfl_NEON<bitdepth, GrainType, Pixel>(
      noise_image[plane], min_value, max_chroma, width, height, start_height,
      subsampling_x, subsampling_y, params.chroma_scaling, scaling_lut, in_y,
      source_stride_y, in_uv, source_stride_uv, out_uv, dest_stride_uv);
}

}  // namespace

namespace low_bitdepth {
namespace {

inline int16x8_t BlendChromaValsNoCfl(
    const int16_t* LIBGAV1_RESTRICT scaling_lut, const int16x8_t orig,
    const int8_t* LIBGAV1_RESTRICT noise_image_cursor,
    const int16x8_t& average_luma, const int16x8_t& scaling_shift_vect,
    const int16x8_t& offset, int luma_multiplier, int chroma_multiplier,
    bool restrict_scaling_lookup, int valid_range_pixels = 0) {
  uint8_t merged_buffer[8];
  const int16x8_t weighted_luma = vmulq_n_s16(average_luma, luma_multiplier);
  const int16x8_t weighted_chroma = vmulq_n_s16(orig, chroma_multiplier);
  // Maximum value of |combined_u| is 127*255 = 0x7E81.
  const int16x8_t combined = vhaddq_s16(weighted_luma, weighted_chroma);
  // Maximum value of u_offset is (255 << 5) = 0x1FE0.
  // 0x7E81 + 0x1FE0 = 0x9E61, therefore another halving add is required.
  const uint8x8_t merged = vqshrun_n_s16(vhaddq_s16(offset, combined), 4);
  vst1_u8(merged_buffer, merged);

  const int16x8_t scaling =
      restrict_scaling_lookup
          ? GetScalingFactors<kBitdepth8, uint8_t>(scaling_lut, merged_buffer,
                                                   valid_range_pixels)
          : GetScalingFactors<kBitdepth8, uint8_t>(scaling_lut, merged_buffer);
  int16x8_t noise = GetSignedSource8(noise_image_cursor);
  noise = ScaleNoise<kBitdepth8>(noise, scaling, scaling_shift_vect);
  return vaddq_s16(orig, noise);
}

LIBGAV1_ALWAYS_INLINE void BlendChromaPlane8bpp_NEON(
    const Array2D<int8_t>& noise_image, int min_value, int max_chroma,
    int width, int height, int start_height, int subsampling_x,
    int subsampling_y, int scaling_shift, int chroma_offset,
    int chroma_multiplier, int luma_multiplier,
    const int16_t* LIBGAV1_RESTRICT scaling_lut,
    const uint8_t* LIBGAV1_RESTRICT in_y_row, ptrdiff_t source_stride_y,
    const uint8_t* in_chroma_row, ptrdiff_t source_stride_chroma,
    uint8_t* out_chroma_row, ptrdiff_t dest_stride) {
  const int16x8_t floor = vdupq_n_s16(min_value);
  const int16x8_t ceiling = vdupq_n_s16(max_chroma);
  // In 8bpp, the maximum upscaled noise is 127*255 = 0x7E81, which is safe
  // for 16 bit signed integers. In higher bitdepths, however, we have to
  // expand to 32 to protect the sign bit.
  const int16x8_t scaling_shift_vect = vdupq_n_s16(-scaling_shift);

  const int chroma_height = (height + subsampling_y) >> subsampling_y;
  const int chroma_width = (width + subsampling_x) >> subsampling_x;
  const int safe_chroma_width = chroma_width & ~7;
  uint8_t luma_buffer[16];
  const int16x8_t offset = vdupq_n_s16(chroma_offset << 5);

  start_height >>= subsampling_y;
  int y = 0;
  do {
    int x = 0;
    for (; x + 8 <= safe_chroma_width; x += 8) {
      const int luma_x = x << subsampling_x;
      const int valid_range_chroma_pixels = chroma_width - x;

      const int16x8_t orig_chroma = GetSignedSource8(&in_chroma_row[x]);
      const int16x8_t average_luma = vreinterpretq_s16_u16(GetAverageLumaMsan(
          &in_y_row[luma_x], subsampling_x, valid_range_chroma_pixels << 1));
      const int16x8_t blended = BlendChromaValsNoCfl(
          scaling_lut, orig_chroma, &(noise_image[y + start_height][x]),
          average_luma, scaling_shift_vect, offset, luma_multiplier,
          chroma_multiplier, /*restrict_scaling_lookup=*/false);
      // In 8bpp, when params_.clip_to_restricted_range == false, we can
      // replace clipping with vqmovun_s16, but the gain would be small.
      StoreUnsigned8(&out_chroma_row[x],
                     vreinterpretq_u16_s16(Clip3(blended, floor, ceiling)));
    }

    if (x < chroma_width) {
      // Begin right edge iteration. Same as the normal iterations, but the
      // |average_luma| computation requires a duplicated luma value at the
      // end.
      const int luma_x = x << subsampling_x;
      const int valid_range_pixels = width - luma_x;
      const int valid_range_bytes = valid_range_pixels * sizeof(in_y_row[0]);
      assert(valid_range_pixels < 16);
      memcpy(luma_buffer, &in_y_row[luma_x], valid_range_bytes);
      luma_buffer[valid_range_pixels] = in_y_row[width - 1];
      const int valid_range_chroma_pixels = chroma_width - x;

      const int16x8_t orig_chroma =
          GetSignedSource8Msan(&in_chroma_row[x], valid_range_chroma_pixels);
      const int16x8_t average_luma = vreinterpretq_s16_u16(GetAverageLumaMsan(
          luma_buffer, subsampling_x, valid_range_chroma_pixels << 1));
      const int16x8_t blended = BlendChromaValsNoCfl(
          scaling_lut, orig_chroma, &(noise_image[y + start_height][x]),
          average_luma, scaling_shift_vect, offset, luma_multiplier,
          chroma_multiplier, /*restrict_scaling_lookup=*/true,
          valid_range_chroma_pixels);
      StoreUnsigned8(&out_chroma_row[x],
                     vreinterpretq_u16_s16(Clip3(blended, floor, ceiling)));
      // End of right edge iteration.
    }

    in_y_row += source_stride_y << subsampling_y;
    in_chroma_row += source_stride_chroma;
    out_chroma_row += dest_stride;
  } while (++y < chroma_height);
}

// This function is for the case params_.chroma_scaling_from_luma == false.
void BlendNoiseWithImageChroma8bpp_NEON(
    Plane plane, const FilmGrainParams& params,
    const void* LIBGAV1_RESTRICT noise_image_ptr, int min_value, int max_chroma,
    int width, int height, int start_height, int subsampling_x,
    int subsampling_y, const int16_t* LIBGAV1_RESTRICT scaling_lut,
    const void* LIBGAV1_RESTRICT source_plane_y, ptrdiff_t source_stride_y,
    const void* source_plane_uv, ptrdiff_t source_stride_uv,
    void* dest_plane_uv, ptrdiff_t dest_stride_uv) {
  assert(plane == kPlaneU || plane == kPlaneV);
  const auto* noise_image =
      static_cast<const Array2D<int8_t>*>(noise_image_ptr);
  const auto* in_y = static_cast<const uint8_t*>(source_plane_y);
  const auto* in_uv = static_cast<const uint8_t*>(source_plane_uv);
  auto* out_uv = static_cast<uint8_t*>(dest_plane_uv);

  const int offset = (plane == kPlaneU) ? params.u_offset : params.v_offset;
  const int luma_multiplier =
      (plane == kPlaneU) ? params.u_luma_multiplier : params.v_luma_multiplier;
  const int multiplier =
      (plane == kPlaneU) ? params.u_multiplier : params.v_multiplier;
  BlendChromaPlane8bpp_NEON(noise_image[plane], min_value, max_chroma, width,
                            height, start_height, subsampling_x, subsampling_y,
                            params.chroma_scaling, offset, multiplier,
                            luma_multiplier, scaling_lut, in_y, source_stride_y,
                            in_uv, source_stride_uv, out_uv, dest_stride_uv);
}

inline void WriteOverlapLine8bpp_NEON(
    const int8_t* LIBGAV1_RESTRICT noise_stripe_row,
    const int8_t* LIBGAV1_RESTRICT noise_stripe_row_prev, int plane_width,
    const int8x8_t grain_coeff, const int8x8_t old_coeff,
    int8_t* LIBGAV1_RESTRICT noise_image_row) {
  int x = 0;
  do {
    // Note that these reads may exceed noise_stripe_row's width by up to 7
    // bytes.
    const int8x8_t source_grain = vld1_s8(noise_stripe_row + x);
    const int8x8_t source_old = vld1_s8(noise_stripe_row_prev + x);
    const int16x8_t weighted_grain = vmull_s8(grain_coeff, source_grain);
    const int16x8_t grain = vmlal_s8(weighted_grain, old_coeff, source_old);
    // Note that this write may exceed noise_image_row's width by up to 7 bytes.
    vst1_s8(noise_image_row + x, vqrshrn_n_s16(grain, 5));
    x += 8;
  } while (x < plane_width);
}

void ConstructNoiseImageOverlap8bpp_NEON(
    const void* LIBGAV1_RESTRICT noise_stripes_buffer, int width, int height,
    int subsampling_x, int subsampling_y,
    void* LIBGAV1_RESTRICT noise_image_buffer) {
  const auto* noise_stripes =
      static_cast<const Array2DView<int8_t>*>(noise_stripes_buffer);
  auto* noise_image = static_cast<Array2D<int8_t>*>(noise_image_buffer);
  const int plane_width = (width + subsampling_x) >> subsampling_x;
  const int plane_height = (height + subsampling_y) >> subsampling_y;
  const int stripe_height = 32 >> subsampling_y;
  const int stripe_mask = stripe_height - 1;
  int y = stripe_height;
  int luma_num = 1;
  if (subsampling_y == 0) {
    const int8x8_t first_row_grain_coeff = vdup_n_s8(17);
    const int8x8_t first_row_old_coeff = vdup_n_s8(27);
    const int8x8_t second_row_grain_coeff = first_row_old_coeff;
    const int8x8_t second_row_old_coeff = first_row_grain_coeff;
    for (; y < (plane_height & ~stripe_mask); ++luma_num, y += stripe_height) {
      const int8_t* noise_stripe = (*noise_stripes)[luma_num];
      const int8_t* noise_stripe_prev = (*noise_stripes)[luma_num - 1];
      WriteOverlapLine8bpp_NEON(
          noise_stripe, &noise_stripe_prev[32 * plane_width], plane_width,
          first_row_grain_coeff, first_row_old_coeff, (*noise_image)[y]);

      WriteOverlapLine8bpp_NEON(&noise_stripe[plane_width],
                                &noise_stripe_prev[(32 + 1) * plane_width],
                                plane_width, second_row_grain_coeff,
                                second_row_old_coeff, (*noise_image)[y + 1]);
    }
    // Either one partial stripe remains (remaining_height  > 0),
    // OR image is less than one stripe high (remaining_height < 0),
    // OR all stripes are completed (remaining_height == 0).
    const int remaining_height = plane_height - y;
    if (remaining_height <= 0) {
      return;
    }
    const int8_t* noise_stripe = (*noise_stripes)[luma_num];
    const int8_t* noise_stripe_prev = (*noise_stripes)[luma_num - 1];
    WriteOverlapLine8bpp_NEON(
        noise_stripe, &noise_stripe_prev[32 * plane_width], plane_width,
        first_row_grain_coeff, first_row_old_coeff, (*noise_image)[y]);

    if (remaining_height > 1) {
      WriteOverlapLine8bpp_NEON(&noise_stripe[plane_width],
                                &noise_stripe_prev[(32 + 1) * plane_width],
                                plane_width, second_row_grain_coeff,
                                second_row_old_coeff, (*noise_image)[y + 1]);
    }
  } else {  // subsampling_y == 1
    const int8x8_t first_row_grain_coeff = vdup_n_s8(22);
    const int8x8_t first_row_old_coeff = vdup_n_s8(23);
    for (; y < plane_height; ++luma_num, y += stripe_height) {
      const int8_t* noise_stripe = (*noise_stripes)[luma_num];
      const int8_t* noise_stripe_prev = (*noise_stripes)[luma_num - 1];
      WriteOverlapLine8bpp_NEON(
          noise_stripe, &noise_stripe_prev[16 * plane_width], plane_width,
          first_row_grain_coeff, first_row_old_coeff, (*noise_image)[y]);
    }
  }
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);

  // LumaAutoRegressionFunc
  dsp->film_grain.luma_auto_regression[0] =
      ApplyAutoRegressiveFilterToLumaGrain_NEON<kBitdepth8, int8_t, 1>;
  dsp->film_grain.luma_auto_regression[1] =
      ApplyAutoRegressiveFilterToLumaGrain_NEON<kBitdepth8, int8_t, 2>;
  dsp->film_grain.luma_auto_regression[2] =
      ApplyAutoRegressiveFilterToLumaGrain_NEON<kBitdepth8, int8_t, 3>;

  // ChromaAutoRegressionFunc[use_luma][auto_regression_coeff_lag]
  // Chroma autoregression should never be called when lag is 0 and use_luma
  // is false.
  dsp->film_grain.chroma_auto_regression[0][0] = nullptr;
  dsp->film_grain.chroma_auto_regression[0][1] =
      ApplyAutoRegressiveFilterToChromaGrains_NEON<kBitdepth8, int8_t, 1,
                                                   false>;
  dsp->film_grain.chroma_auto_regression[0][2] =
      ApplyAutoRegressiveFilterToChromaGrains_NEON<kBitdepth8, int8_t, 2,
                                                   false>;
  dsp->film_grain.chroma_auto_regression[0][3] =
      ApplyAutoRegressiveFilterToChromaGrains_NEON<kBitdepth8, int8_t, 3,
                                                   false>;
  dsp->film_grain.chroma_auto_regression[1][0] =
      ApplyAutoRegressiveFilterToChromaGrains_NEON<kBitdepth8, int8_t, 0, true>;
  dsp->film_grain.chroma_auto_regression[1][1] =
      ApplyAutoRegressiveFilterToChromaGrains_NEON<kBitdepth8, int8_t, 1, true>;
  dsp->film_grain.chroma_auto_regression[1][2] =
      ApplyAutoRegressiveFilterToChromaGrains_NEON<kBitdepth8, int8_t, 2, true>;
  dsp->film_grain.chroma_auto_regression[1][3] =
      ApplyAutoRegressiveFilterToChromaGrains_NEON<kBitdepth8, int8_t, 3, true>;

  dsp->film_grain.construct_noise_image_overlap =
      ConstructNoiseImageOverlap8bpp_NEON;

  dsp->film_grain.initialize_scaling_lut =
      InitializeScalingLookupTable_NEON<kBitdepth8>;

  dsp->film_grain.blend_noise_luma =
      BlendNoiseWithImageLuma_NEON<kBitdepth8, int8_t, uint8_t>;
  dsp->film_grain.blend_noise_chroma[0] = BlendNoiseWithImageChroma8bpp_NEON;
  dsp->film_grain.blend_noise_chroma[1] =
      BlendNoiseWithImageChromaWithCfl_NEON<kBitdepth8, int8_t, uint8_t>;
}

}  // namespace
}  // namespace low_bitdepth

#if LIBGAV1_MAX_BITDEPTH >= 10
namespace high_bitdepth {
namespace {

inline void WriteOverlapLine10bpp_NEON(
    const int16_t* LIBGAV1_RESTRICT noise_stripe_row,
    const int16_t* LIBGAV1_RESTRICT noise_stripe_row_prev, int plane_width,
    const int16x8_t grain_coeff, const int16x8_t old_coeff,
    int16_t* LIBGAV1_RESTRICT noise_image_row) {
  int x = 0;
  do {
    // Note that these reads may exceed noise_stripe_row's width by up to 7
    // values.
    const int16x8_t source_grain = vld1q_s16(noise_stripe_row + x);
    const int16x8_t source_old = vld1q_s16(noise_stripe_row_prev + x);
    // Maximum product is 511 * 27 = 0x35E5.
    const int16x8_t weighted_grain = vmulq_s16(grain_coeff, source_grain);
    // Maximum sum is 511 * (22 + 23) = 0x59D3.
    const int16x8_t grain_sum =
        vmlaq_s16(weighted_grain, old_coeff, source_old);
    // Note that this write may exceed noise_image_row's width by up to 7
    // values.
    const int16x8_t grain = Clip3S16(vrshrq_n_s16(grain_sum, 5),
                                     vdupq_n_s16(GetGrainMin<kBitdepth10>()),
                                     vdupq_n_s16(GetGrainMax<kBitdepth10>()));
    vst1q_s16(noise_image_row + x, grain);
    x += 8;
  } while (x < plane_width);
}

void ConstructNoiseImageOverlap10bpp_NEON(
    const void* LIBGAV1_RESTRICT noise_stripes_buffer, int width, int height,
    int subsampling_x, int subsampling_y,
    void* LIBGAV1_RESTRICT noise_image_buffer) {
  const auto* noise_stripes =
      static_cast<const Array2DView<int16_t>*>(noise_stripes_buffer);
  auto* noise_image = static_cast<Array2D<int16_t>*>(noise_image_buffer);
  const int plane_width = (width + subsampling_x) >> subsampling_x;
  const int plane_height = (height + subsampling_y) >> subsampling_y;
  const int stripe_height = 32 >> subsampling_y;
  const int stripe_mask = stripe_height - 1;
  int y = stripe_height;
  int luma_num = 1;
  if (subsampling_y == 0) {
    const int16x8_t first_row_grain_coeff = vdupq_n_s16(17);
    const int16x8_t first_row_old_coeff = vdupq_n_s16(27);
    const int16x8_t second_row_grain_coeff = first_row_old_coeff;
    const int16x8_t second_row_old_coeff = first_row_grain_coeff;
    for (; y < (plane_height & ~stripe_mask); ++luma_num, y += stripe_height) {
      const int16_t* noise_stripe = (*noise_stripes)[luma_num];
      const int16_t* noise_stripe_prev = (*noise_stripes)[luma_num - 1];
      WriteOverlapLine10bpp_NEON(
          noise_stripe, &noise_stripe_prev[32 * plane_width], plane_width,
          first_row_grain_coeff, first_row_old_coeff, (*noise_image)[y]);

      WriteOverlapLine10bpp_NEON(&noise_stripe[plane_width],
                                 &noise_stripe_prev[(32 + 1) * plane_width],
                                 plane_width, second_row_grain_coeff,
                                 second_row_old_coeff, (*noise_image)[y + 1]);
    }
    // Either one partial stripe remains (remaining_height > 0),
    // OR image is less than one stripe high (remaining_height < 0),
    // OR all stripes are completed (remaining_height == 0).
    const int remaining_height = plane_height - y;
    if (remaining_height <= 0) {
      return;
    }
    const int16_t* noise_stripe = (*noise_stripes)[luma_num];
    const int16_t* noise_stripe_prev = (*noise_stripes)[luma_num - 1];
    WriteOverlapLine10bpp_NEON(
        noise_stripe, &noise_stripe_prev[32 * plane_width], plane_width,
        first_row_grain_coeff, first_row_old_coeff, (*noise_image)[y]);

    if (remaining_height > 1) {
      WriteOverlapLine10bpp_NEON(&noise_stripe[plane_width],
                                 &noise_stripe_prev[(32 + 1) * plane_width],
                                 plane_width, second_row_grain_coeff,
                                 second_row_old_coeff, (*noise_image)[y + 1]);
    }
  } else {  // subsampling_y == 1
    const int16x8_t first_row_grain_coeff = vdupq_n_s16(22);
    const int16x8_t first_row_old_coeff = vdupq_n_s16(23);
    for (; y < plane_height; ++luma_num, y += stripe_height) {
      const int16_t* noise_stripe = (*noise_stripes)[luma_num];
      const int16_t* noise_stripe_prev = (*noise_stripes)[luma_num - 1];
      WriteOverlapLine10bpp_NEON(
          noise_stripe, &noise_stripe_prev[16 * plane_width], plane_width,
          first_row_grain_coeff, first_row_old_coeff, (*noise_image)[y]);
    }
  }
}

inline int16x8_t BlendChromaValsNoCfl(
    const int16_t* LIBGAV1_RESTRICT scaling_lut, const int16x8_t orig,
    const int16_t* LIBGAV1_RESTRICT noise_image_cursor,
    const int16x8_t& average_luma, const int16x8_t& scaling_shift_vect,
    const int32x4_t& offset, int luma_multiplier, int chroma_multiplier,
    bool restrict_scaling_lookup, int valid_range_pixels = 0) {
  uint16_t merged_buffer[8];
  const int32x4_t weighted_luma_low =
      vmull_n_s16(vget_low_s16(average_luma), luma_multiplier);
  const int32x4_t weighted_luma_high =
      vmull_n_s16(vget_high_s16(average_luma), luma_multiplier);
  // Maximum value of combined is 127 * 1023 = 0x1FB81.
  const int32x4_t combined_low =
      vmlal_n_s16(weighted_luma_low, vget_low_s16(orig), chroma_multiplier);
  const int32x4_t combined_high =
      vmlal_n_s16(weighted_luma_high, vget_high_s16(orig), chroma_multiplier);
  // Maximum value of offset is (255 << 8) = 0xFF00. Offset may be negative.
  const uint16x4_t merged_low =
      vqshrun_n_s32(vaddq_s32(offset, combined_low), 6);
  const uint16x4_t merged_high =
      vqshrun_n_s32(vaddq_s32(offset, combined_high), 6);
  const uint16x8_t max_pixel = vdupq_n_u16((1 << kBitdepth10) - 1);
  vst1q_u16(merged_buffer,
            vminq_u16(vcombine_u16(merged_low, merged_high), max_pixel));
  const int16x8_t scaling =
      restrict_scaling_lookup
          ? GetScalingFactors<kBitdepth10, uint16_t>(scaling_lut, merged_buffer,
                                                     valid_range_pixels)
          : GetScalingFactors<kBitdepth10, uint16_t>(scaling_lut,
                                                     merged_buffer);
  const int16x8_t noise = GetSignedSource8(noise_image_cursor);
  const int16x8_t scaled_noise =
      ScaleNoise<kBitdepth10>(noise, scaling, scaling_shift_vect);
  return vaddq_s16(orig, scaled_noise);
}

LIBGAV1_ALWAYS_INLINE void BlendChromaPlane10bpp_NEON(
    const Array2D<int16_t>& noise_image, int min_value, int max_chroma,
    int width, int height, int start_height, int subsampling_x,
    int subsampling_y, int scaling_shift, int chroma_offset,
    int chroma_multiplier, int luma_multiplier,
    const int16_t* LIBGAV1_RESTRICT scaling_lut,
    const uint16_t* LIBGAV1_RESTRICT in_y_row, ptrdiff_t source_stride_y,
    const uint16_t* in_chroma_row, ptrdiff_t source_stride_chroma,
    uint16_t* out_chroma_row, ptrdiff_t dest_stride) {
  const int16x8_t floor = vdupq_n_s16(min_value);
  const int16x8_t ceiling = vdupq_n_s16(max_chroma);
  const int16x8_t scaling_shift_vect = vdupq_n_s16(15 - scaling_shift);

  const int chroma_height = (height + subsampling_y) >> subsampling_y;
  const int chroma_width = (width + subsampling_x) >> subsampling_x;
  const int safe_chroma_width = chroma_width & ~7;
  uint16_t luma_buffer[16];
  // Offset is added before downshifting in order to take advantage of
  // saturation, so it has to be upscaled by 6 bits, plus 2 bits for 10bpp.
  const int32x4_t offset = vdupq_n_s32(chroma_offset << (6 + 2));

  start_height >>= subsampling_y;
  int y = 0;
  do {
    int x = 0;
    for (; x + 8 <= safe_chroma_width; x += 8) {
      const int luma_x = x << subsampling_x;
      const int16x8_t average_luma = vreinterpretq_s16_u16(
          GetAverageLuma(&in_y_row[luma_x], subsampling_x));
      const int16x8_t orig_chroma = GetSignedSource8(&in_chroma_row[x]);
      const int16x8_t blended = BlendChromaValsNoCfl(
          scaling_lut, orig_chroma, &(noise_image[y + start_height][x]),
          average_luma, scaling_shift_vect, offset, luma_multiplier,
          chroma_multiplier, /*restrict_scaling_lookup=*/false);
      StoreUnsigned8(&out_chroma_row[x],
                     vreinterpretq_u16_s16(Clip3(blended, floor, ceiling)));
    }

    if (x < chroma_width) {
      // Begin right edge iteration. Same as the normal iterations, but the
      // |average_luma| computation requires a duplicated luma value at the
      // end.
      const int luma_x = x << subsampling_x;
      const int valid_range_pixels = width - luma_x;
      const int valid_range_bytes = valid_range_pixels * sizeof(in_y_row[0]);
      assert(valid_range_pixels < 16);
      memcpy(luma_buffer, &in_y_row[luma_x], valid_range_bytes);
      luma_buffer[valid_range_pixels] = in_y_row[width - 1];
      const int valid_range_chroma_pixels = chroma_width - x;
      const int valid_range_chroma_bytes =
          (chroma_width - x) * sizeof(in_chroma_row[0]);
      const int16x8_t orig_chroma =
          GetSignedSource8Msan(&in_chroma_row[x], valid_range_chroma_bytes);

      const int16x8_t average_luma = vreinterpretq_s16_u16(GetAverageLumaMsan(
          luma_buffer, subsampling_x, valid_range_chroma_pixels << 1));
      const int16x8_t blended = BlendChromaValsNoCfl(
          scaling_lut, orig_chroma, &(noise_image[y + start_height][x]),
          average_luma, scaling_shift_vect, offset, luma_multiplier,
          chroma_multiplier, /*restrict_scaling_lookup=*/true,
          valid_range_chroma_pixels);
      StoreUnsigned8(&out_chroma_row[x],
                     vreinterpretq_u16_s16(Clip3(blended, floor, ceiling)));
      // End of right edge iteration.
    }

    in_y_row = AddByteStride(in_y_row, source_stride_y << subsampling_y);
    in_chroma_row = AddByteStride(in_chroma_row, source_stride_chroma);
    out_chroma_row = AddByteStride(out_chroma_row, dest_stride);
  } while (++y < chroma_height);
}

// This function is for the case params_.chroma_scaling_from_luma == false.
void BlendNoiseWithImageChroma10bpp_NEON(
    Plane plane, const FilmGrainParams& params,
    const void* LIBGAV1_RESTRICT noise_image_ptr, int min_value, int max_chroma,
    int width, int height, int start_height, int subsampling_x,
    int subsampling_y, const int16_t* LIBGAV1_RESTRICT scaling_lut,
    const void* LIBGAV1_RESTRICT source_plane_y, ptrdiff_t source_stride_y,
    const void* source_plane_uv, ptrdiff_t source_stride_uv,
    void* dest_plane_uv, ptrdiff_t dest_stride_uv) {
  assert(plane == kPlaneU || plane == kPlaneV);
  const auto* noise_image =
      static_cast<const Array2D<int16_t>*>(noise_image_ptr);
  const auto* in_y = static_cast<const uint16_t*>(source_plane_y);
  const auto* in_uv = static_cast<const uint16_t*>(source_plane_uv);
  auto* out_uv = static_cast<uint16_t*>(dest_plane_uv);

  const int offset = (plane == kPlaneU) ? params.u_offset : params.v_offset;
  const int luma_multiplier =
      (plane == kPlaneU) ? params.u_luma_multiplier : params.v_luma_multiplier;
  const int multiplier =
      (plane == kPlaneU) ? params.u_multiplier : params.v_multiplier;
  BlendChromaPlane10bpp_NEON(
      noise_image[plane], min_value, max_chroma, width, height, start_height,
      subsampling_x, subsampling_y, params.chroma_scaling, offset, multiplier,
      luma_multiplier, scaling_lut, in_y, source_stride_y, in_uv,
      source_stride_uv, out_uv, dest_stride_uv);
}

void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth10);
  assert(dsp != nullptr);

  // LumaAutoRegressionFunc
  dsp->film_grain.luma_auto_regression[0] =
      ApplyAutoRegressiveFilterToLumaGrain_NEON<kBitdepth10, int16_t, 1>;
  dsp->film_grain.luma_auto_regression[1] =
      ApplyAutoRegressiveFilterToLumaGrain_NEON<kBitdepth10, int16_t, 2>;
  dsp->film_grain.luma_auto_regression[2] =
      ApplyAutoRegressiveFilterToLumaGrain_NEON<kBitdepth10, int16_t, 3>;

  // ChromaAutoRegressionFunc[use_luma][auto_regression_coeff_lag][subsampling]
  // Chroma autoregression should never be called when lag is 0 and use_luma
  // is false.
  dsp->film_grain.chroma_auto_regression[0][0] = nullptr;
  dsp->film_grain.chroma_auto_regression[0][1] =
      ApplyAutoRegressiveFilterToChromaGrains_NEON<kBitdepth10, int16_t, 1,
                                                   false>;
  dsp->film_grain.chroma_auto_regression[0][2] =
      ApplyAutoRegressiveFilterToChromaGrains_NEON<kBitdepth10, int16_t, 2,
                                                   false>;
  dsp->film_grain.chroma_auto_regression[0][3] =
      ApplyAutoRegressiveFilterToChromaGrains_NEON<kBitdepth10, int16_t, 3,
                                                   false>;
  dsp->film_grain.chroma_auto_regression[1][0] =
      ApplyAutoRegressiveFilterToChromaGrains_NEON<kBitdepth10, int16_t, 0,
                                                   true>;
  dsp->film_grain.chroma_auto_regression[1][1] =
      ApplyAutoRegressiveFilterToChromaGrains_NEON<kBitdepth10, int16_t, 1,
                                                   true>;
  dsp->film_grain.chroma_auto_regression[1][2] =
      ApplyAutoRegressiveFilterToChromaGrains_NEON<kBitdepth10, int16_t, 2,
                                                   true>;
  dsp->film_grain.chroma_auto_regression[1][3] =
      ApplyAutoRegressiveFilterToChromaGrains_NEON<kBitdepth10, int16_t, 3,
                                                   true>;

  dsp->film_grain.construct_noise_image_overlap =
      ConstructNoiseImageOverlap10bpp_NEON;

  dsp->film_grain.initialize_scaling_lut =
      InitializeScalingLookupTable_NEON<kBitdepth10>;

  dsp->film_grain.blend_noise_luma =
      BlendNoiseWithImageLuma_NEON<kBitdepth10, int16_t, uint16_t>;
  dsp->film_grain.blend_noise_chroma[0] = BlendNoiseWithImageChroma10bpp_NEON;
  dsp->film_grain.blend_noise_chroma[1] =
      BlendNoiseWithImageChromaWithCfl_NEON<kBitdepth10, int16_t, uint16_t>;
}

}  // namespace
}  // namespace high_bitdepth
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

}  // namespace film_grain

void FilmGrainInit_NEON() {
  film_grain::low_bitdepth::Init8bpp();
#if LIBGAV1_MAX_BITDEPTH >= 10
  film_grain::high_bitdepth::Init10bpp();
#endif  // LIBGAV1_MAX_BITDEPTH >= 10
}

}  // namespace dsp
}  // namespace libgav1

#else   // !LIBGAV1_ENABLE_NEON

namespace libgav1 {
namespace dsp {

void FilmGrainInit_NEON() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_ENABLE_NEON
