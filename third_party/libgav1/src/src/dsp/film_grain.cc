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

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

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

template <int bitdepth>
void InitializeScalingLookupTable_C(int num_points, const uint8_t point_value[],
                                    const uint8_t point_scaling[],
                                    int16_t* scaling_lut,
                                    const int scaling_lut_length) {
  if (num_points == 0) {
    memset(scaling_lut, 0, sizeof(scaling_lut[0]) * scaling_lut_length);
    return;
  }
  constexpr int index_shift = (bitdepth == kBitdepth10) ? 2 : 0;
  static_assert(sizeof(scaling_lut[0]) == 2, "");
  Memset(scaling_lut, point_scaling[0],
         std::max(static_cast<int>(point_value[0]), 1) << index_shift);
  for (int i = 0; i < num_points - 1; ++i) {
    const int delta_y = point_scaling[i + 1] - point_scaling[i];
    const int delta_x = point_value[i + 1] - point_value[i];
    const int delta = delta_y * ((65536 + (delta_x >> 1)) / delta_x);
    for (int x = 0; x < delta_x; ++x) {
      const int v = point_scaling[i] + ((x * delta + 32768) >> 16);
      assert(v >= 0 && v <= UINT8_MAX);
      const int lut_index = (point_value[i] + x) << index_shift;
      scaling_lut[lut_index] = v;
    }
  }
  const int16_t last_point_value = point_value[num_points - 1];
  const int x_base = last_point_value << index_shift;
  Memset(&scaling_lut[x_base], point_scaling[num_points - 1],
         scaling_lut_length - x_base);
  // Fill in the gaps.
  if (bitdepth == kBitdepth10) {
    for (int x = 4; x < x_base + 4; x += 4) {
      const int start = scaling_lut[x - 4];
      const int end = scaling_lut[x];
      const int delta = end - start;
      scaling_lut[x - 3] = start + RightShiftWithRounding(delta, 2);
      scaling_lut[x - 2] = start + RightShiftWithRounding(2 * delta, 2);
      scaling_lut[x - 1] = start + RightShiftWithRounding(3 * delta, 2);
    }
  }
}

// Section 7.18.3.5.
template <int bitdepth>
int ScaleLut(const int16_t* scaling_lut, int index) {
  if (bitdepth <= kBitdepth10) {
    assert(index < kScalingLookupTableSize << (bitdepth - 2));
    return scaling_lut[index];
  }
  // Performs a piecewise linear interpolation into the scaling table.
  const int shift = bitdepth - kBitdepth8;
  const int quotient = index >> shift;
  const int remainder = index - (quotient << shift);
  assert(quotient + 1 < kScalingLookupTableSize);
  const int start = scaling_lut[quotient];
  const int end = scaling_lut[quotient + 1];
  return start + RightShiftWithRounding((end - start) * remainder, shift);
}

// Applies an auto-regressive filter to the white noise in luma_grain.
template <int bitdepth, typename GrainType>
void ApplyAutoRegressiveFilterToLumaGrain_C(const FilmGrainParams& params,
                                            void* luma_grain_buffer) {
  auto* luma_grain = static_cast<GrainType*>(luma_grain_buffer);
  const int grain_min = GetGrainMin<bitdepth>();
  const int grain_max = GetGrainMax<bitdepth>();
  const int auto_regression_coeff_lag = params.auto_regression_coeff_lag;
  assert(auto_regression_coeff_lag > 0 && auto_regression_coeff_lag <= 3);
  // A pictorial representation of the auto-regressive filter for various values
  // of auto_regression_coeff_lag. The letter 'O' represents the current sample.
  // (The filter always operates on the current sample with filter
  // coefficient 1.) The letters 'X' represent the neighboring samples that the
  // filter operates on.
  //
  // auto_regression_coeff_lag == 3:
  //   X X X X X X X
  //   X X X X X X X
  //   X X X X X X X
  //   X X X O
  // auto_regression_coeff_lag == 2:
  //     X X X X X
  //     X X X X X
  //     X X O
  // auto_regression_coeff_lag == 1:
  //       X X X
  //       X O
  // auto_regression_coeff_lag == 0:
  //         O
  //
  // Note that if auto_regression_coeff_lag is 0, the filter is the identity
  // filter and therefore can be skipped. This implementation assumes it is not
  // called in that case.
  const int shift = params.auto_regression_shift;
  for (int y = kAutoRegressionBorder; y < kLumaHeight; ++y) {
    for (int x = kAutoRegressionBorder; x < kLumaWidth - kAutoRegressionBorder;
         ++x) {
      int sum = 0;
      int pos = 0;
      int delta_row = -auto_regression_coeff_lag;
      // The last iteration (delta_row == 0) is shorter and is handled
      // separately.
      do {
        int delta_column = -auto_regression_coeff_lag;
        do {
          const int coeff = params.auto_regression_coeff_y[pos];
          sum += luma_grain[(y + delta_row) * kLumaWidth + (x + delta_column)] *
                 coeff;
          ++pos;
        } while (++delta_column <= auto_regression_coeff_lag);
      } while (++delta_row < 0);
      // Last iteration: delta_row == 0.
      {
        int delta_column = -auto_regression_coeff_lag;
        do {
          const int coeff = params.auto_regression_coeff_y[pos];
          sum += luma_grain[y * kLumaWidth + (x + delta_column)] * coeff;
          ++pos;
        } while (++delta_column < 0);
      }
      luma_grain[y * kLumaWidth + x] = Clip3(
          luma_grain[y * kLumaWidth + x] + RightShiftWithRounding(sum, shift),
          grain_min, grain_max);
    }
  }
}

template <int bitdepth, typename GrainType, int auto_regression_coeff_lag,
          bool use_luma>
void ApplyAutoRegressiveFilterToChromaGrains_C(
    const FilmGrainParams& params,
    const void* LIBGAV1_RESTRICT luma_grain_buffer, int subsampling_x,
    int subsampling_y, void* LIBGAV1_RESTRICT u_grain_buffer,
    void* LIBGAV1_RESTRICT v_grain_buffer) {
  static_assert(
      auto_regression_coeff_lag >= 0 && auto_regression_coeff_lag <= 3,
      "Unsupported autoregression lag for chroma.");
  const auto* luma_grain = static_cast<const GrainType*>(luma_grain_buffer);
  const int grain_min = GetGrainMin<bitdepth>();
  const int grain_max = GetGrainMax<bitdepth>();
  auto* u_grain = static_cast<GrainType*>(u_grain_buffer);
  auto* v_grain = static_cast<GrainType*>(v_grain_buffer);
  const int shift = params.auto_regression_shift;
  const int chroma_height =
      (subsampling_y == 0) ? kMaxChromaHeight : kMinChromaHeight;
  const int chroma_width =
      (subsampling_x == 0) ? kMaxChromaWidth : kMinChromaWidth;
  for (int y = kAutoRegressionBorder; y < chroma_height; ++y) {
    const int luma_y =
        ((y - kAutoRegressionBorder) << subsampling_y) + kAutoRegressionBorder;
    for (int x = kAutoRegressionBorder;
         x < chroma_width - kAutoRegressionBorder; ++x) {
      int sum_u = 0;
      int sum_v = 0;
      int pos = 0;
      int delta_row = -auto_regression_coeff_lag;
      do {
        int delta_column = -auto_regression_coeff_lag;
        do {
          if (delta_row == 0 && delta_column == 0) {
            break;
          }
          const int coeff_u = params.auto_regression_coeff_u[pos];
          const int coeff_v = params.auto_regression_coeff_v[pos];
          sum_u +=
              u_grain[(y + delta_row) * chroma_width + (x + delta_column)] *
              coeff_u;
          sum_v +=
              v_grain[(y + delta_row) * chroma_width + (x + delta_column)] *
              coeff_v;
          ++pos;
        } while (++delta_column <= auto_regression_coeff_lag);
      } while (++delta_row <= 0);
      if (use_luma) {
        int luma = 0;
        const int luma_x = ((x - kAutoRegressionBorder) << subsampling_x) +
                           kAutoRegressionBorder;
        int i = 0;
        do {
          int j = 0;
          do {
            luma += luma_grain[(luma_y + i) * kLumaWidth + (luma_x + j)];
          } while (++j <= subsampling_x);
        } while (++i <= subsampling_y);
        luma = SubsampledValue(luma, subsampling_x + subsampling_y);
        const int coeff_u = params.auto_regression_coeff_u[pos];
        const int coeff_v = params.auto_regression_coeff_v[pos];
        sum_u += luma * coeff_u;
        sum_v += luma * coeff_v;
      }
      u_grain[y * chroma_width + x] = Clip3(
          u_grain[y * chroma_width + x] + RightShiftWithRounding(sum_u, shift),
          grain_min, grain_max);
      v_grain[y * chroma_width + x] = Clip3(
          v_grain[y * chroma_width + x] + RightShiftWithRounding(sum_v, shift),
          grain_min, grain_max);
    }
  }
}

// This implementation is for the condition overlap_flag == false.
template <int bitdepth, typename GrainType>
void ConstructNoiseStripes_C(const void* LIBGAV1_RESTRICT grain_buffer,
                             int grain_seed, int width, int height,
                             int subsampling_x, int subsampling_y,
                             void* LIBGAV1_RESTRICT noise_stripes_buffer) {
  auto* noise_stripes =
      static_cast<Array2DView<GrainType>*>(noise_stripes_buffer);
  const auto* grain = static_cast<const GrainType*>(grain_buffer);
  const int half_width = DivideBy2(width + 1);
  const int half_height = DivideBy2(height + 1);
  assert(half_width > 0);
  assert(half_height > 0);
  static_assert(kLumaWidth == kMaxChromaWidth,
                "kLumaWidth width should be equal to kMaxChromaWidth");
  const int grain_width =
      (subsampling_x == 0) ? kMaxChromaWidth : kMinChromaWidth;
  const int plane_width = (width + subsampling_x) >> subsampling_x;
  constexpr int kNoiseStripeHeight = 34;
  int luma_num = 0;
  int y = 0;
  do {
    GrainType* const noise_stripe = (*noise_stripes)[luma_num];
    uint16_t seed = grain_seed;
    seed ^= ((luma_num * 37 + 178) & 255) << 8;
    seed ^= ((luma_num * 173 + 105) & 255);
    int x = 0;
    do {
      const int rand = GetFilmGrainRandomNumber(8, &seed);
      const int offset_x = rand >> 4;
      const int offset_y = rand & 15;
      const int plane_offset_x =
          (subsampling_x != 0) ? 6 + offset_x : 9 + offset_x * 2;
      const int plane_offset_y =
          (subsampling_y != 0) ? 6 + offset_y : 9 + offset_y * 2;
      int i = 0;
      do {
        // Section 7.18.3.5 says:
        //   noiseStripe[ lumaNum ][ 0 ] is 34 samples high and w samples
        //   wide (a few additional samples across are actually written to
        //   the array, but these are never read) ...
        //
        // Note: The warning in the parentheses also applies to
        // noiseStripe[ lumaNum ][ 1 ] and noiseStripe[ lumaNum ][ 2 ].
        //
        // Writes beyond the width of each row could happen below. To
        // prevent those writes, we clip the number of pixels to copy against
        // the remaining width.
        const int copy_size =
            std::min(kNoiseStripeHeight >> subsampling_x,
                     plane_width - (x << (1 - subsampling_x)));
        memcpy(&noise_stripe[i * plane_width + (x << (1 - subsampling_x))],
               &grain[(plane_offset_y + i) * grain_width + plane_offset_x],
               copy_size * sizeof(noise_stripe[0]));
      } while (++i < (kNoiseStripeHeight >> subsampling_y));
      x += 16;
    } while (x < half_width);

    ++luma_num;
    y += 16;
  } while (y < half_height);
}

// This implementation is for the condition overlap_flag == true.
template <int bitdepth, typename GrainType>
void ConstructNoiseStripesWithOverlap_C(
    const void* LIBGAV1_RESTRICT grain_buffer, int grain_seed, int width,
    int height, int subsampling_x, int subsampling_y,
    void* LIBGAV1_RESTRICT noise_stripes_buffer) {
  auto* noise_stripes =
      static_cast<Array2DView<GrainType>*>(noise_stripes_buffer);
  const auto* grain = static_cast<const GrainType*>(grain_buffer);
  const int half_width = DivideBy2(width + 1);
  const int half_height = DivideBy2(height + 1);
  assert(half_width > 0);
  assert(half_height > 0);
  static_assert(kLumaWidth == kMaxChromaWidth,
                "kLumaWidth width should be equal to kMaxChromaWidth");
  const int grain_width =
      (subsampling_x == 0) ? kMaxChromaWidth : kMinChromaWidth;
  const int plane_width = (width + subsampling_x) >> subsampling_x;
  constexpr int kNoiseStripeHeight = 34;
  int luma_num = 0;
  int y = 0;
  do {
    GrainType* const noise_stripe = (*noise_stripes)[luma_num];
    uint16_t seed = grain_seed;
    seed ^= ((luma_num * 37 + 178) & 255) << 8;
    seed ^= ((luma_num * 173 + 105) & 255);
    // Begin special iteration for x == 0.
    const int rand = GetFilmGrainRandomNumber(8, &seed);
    const int offset_x = rand >> 4;
    const int offset_y = rand & 15;
    const int plane_offset_x =
        (subsampling_x != 0) ? 6 + offset_x : 9 + offset_x * 2;
    const int plane_offset_y =
        (subsampling_y != 0) ? 6 + offset_y : 9 + offset_y * 2;
    // The overlap computation only occurs when x > 0, so it is omitted here.
    int i = 0;
    do {
      const int copy_size =
          std::min(kNoiseStripeHeight >> subsampling_x, plane_width);
      memcpy(&noise_stripe[i * plane_width],
             &grain[(plane_offset_y + i) * grain_width + plane_offset_x],
             copy_size * sizeof(noise_stripe[0]));
    } while (++i < (kNoiseStripeHeight >> subsampling_y));
    // End special iteration for x == 0.
    for (int x = 16; x < half_width; x += 16) {
      const int rand = GetFilmGrainRandomNumber(8, &seed);
      const int offset_x = rand >> 4;
      const int offset_y = rand & 15;
      const int plane_offset_x =
          (subsampling_x != 0) ? 6 + offset_x : 9 + offset_x * 2;
      const int plane_offset_y =
          (subsampling_y != 0) ? 6 + offset_y : 9 + offset_y * 2;
      int i = 0;
      do {
        int j = 0;
        int grain_sample =
            grain[(plane_offset_y + i) * grain_width + plane_offset_x];
        // The first pixel(s) of each segment of the noise_stripe are subject to
        // the "overlap" computation.
        if (subsampling_x == 0) {
          // Corresponds to the line in the spec:
          // if (j < 2 && x > 0)
          // j = 0
          int old = noise_stripe[i * plane_width + x * 2];
          grain_sample = old * 27 + grain_sample * 17;
          grain_sample =
              Clip3(RightShiftWithRounding(grain_sample, 5),
                    GetGrainMin<bitdepth>(), GetGrainMax<bitdepth>());
          noise_stripe[i * plane_width + x * 2] = grain_sample;

          // This check prevents overwriting for the iteration j = 1. The
          // continue applies to the i-loop.
          if (x * 2 + 1 >= plane_width) continue;
          // j = 1
          grain_sample =
              grain[(plane_offset_y + i) * grain_width + plane_offset_x + 1];
          old = noise_stripe[i * plane_width + x * 2 + 1];
          grain_sample = old * 17 + grain_sample * 27;
          grain_sample =
              Clip3(RightShiftWithRounding(grain_sample, 5),
                    GetGrainMin<bitdepth>(), GetGrainMax<bitdepth>());
          noise_stripe[i * plane_width + x * 2 + 1] = grain_sample;
          j = 2;
        } else {
          // Corresponds to the line in the spec:
          // if (j == 0 && x > 0)
          const int old = noise_stripe[i * plane_width + x];
          grain_sample = old * 23 + grain_sample * 22;
          grain_sample =
              Clip3(RightShiftWithRounding(grain_sample, 5),
                    GetGrainMin<bitdepth>(), GetGrainMax<bitdepth>());
          noise_stripe[i * plane_width + x] = grain_sample;
          j = 1;
        }
        // The following covers the rest of the loop over j as described in the
        // spec.
        //
        // Section 7.18.3.5 says:
        //   noiseStripe[ lumaNum ][ 0 ] is 34 samples high and w samples
        //   wide (a few additional samples across are actually written to
        //   the array, but these are never read) ...
        //
        // Note: The warning in the parentheses also applies to
        // noiseStripe[ lumaNum ][ 1 ] and noiseStripe[ lumaNum ][ 2 ].
        //
        // Writes beyond the width of each row could happen below. To
        // prevent those writes, we clip the number of pixels to copy against
        // the remaining width.
        const int copy_size =
            std::min(kNoiseStripeHeight >> subsampling_x,
                     plane_width - (x << (1 - subsampling_x))) -
            j;
        memcpy(&noise_stripe[i * plane_width + (x << (1 - subsampling_x)) + j],
               &grain[(plane_offset_y + i) * grain_width + plane_offset_x + j],
               copy_size * sizeof(noise_stripe[0]));
      } while (++i < (kNoiseStripeHeight >> subsampling_y));
    }

    ++luma_num;
    y += 16;
  } while (y < half_height);
}

template <int bitdepth, typename GrainType>
inline void WriteOverlapLine_C(
    const GrainType* LIBGAV1_RESTRICT noise_stripe_row,
    const GrainType* LIBGAV1_RESTRICT noise_stripe_row_prev, int plane_width,
    int grain_coeff, int old_coeff,
    GrainType* LIBGAV1_RESTRICT noise_image_row) {
  int x = 0;
  do {
    int grain = noise_stripe_row[x];
    const int old = noise_stripe_row_prev[x];
    grain = old * old_coeff + grain * grain_coeff;
    grain = Clip3(RightShiftWithRounding(grain, 5), GetGrainMin<bitdepth>(),
                  GetGrainMax<bitdepth>());
    noise_image_row[x] = grain;
  } while (++x < plane_width);
}

template <int bitdepth, typename GrainType>
void ConstructNoiseImageOverlap_C(
    const void* LIBGAV1_RESTRICT noise_stripes_buffer, int width, int height,
    int subsampling_x, int subsampling_y,
    void* LIBGAV1_RESTRICT noise_image_buffer) {
  const auto* noise_stripes =
      static_cast<const Array2DView<GrainType>*>(noise_stripes_buffer);
  auto* noise_image = static_cast<Array2D<GrainType>*>(noise_image_buffer);
  const int plane_width = (width + subsampling_x) >> subsampling_x;
  const int plane_height = (height + subsampling_y) >> subsampling_y;
  const int stripe_height = 32 >> subsampling_y;
  const int stripe_mask = stripe_height - 1;
  int y = stripe_height;
  int luma_num = 1;
  if (subsampling_y == 0) {
    // Begin complete stripes section. This is when we are guaranteed to have
    // two overlap rows in each stripe.
    for (; y < (plane_height & ~stripe_mask); ++luma_num, y += stripe_height) {
      const GrainType* noise_stripe = (*noise_stripes)[luma_num];
      const GrainType* noise_stripe_prev = (*noise_stripes)[luma_num - 1];
      // First overlap row.
      WriteOverlapLine_C<bitdepth>(noise_stripe,
                                   &noise_stripe_prev[32 * plane_width],
                                   plane_width, 17, 27, (*noise_image)[y]);
      // Second overlap row.
      WriteOverlapLine_C<bitdepth>(&noise_stripe[plane_width],
                                   &noise_stripe_prev[(32 + 1) * plane_width],
                                   plane_width, 27, 17, (*noise_image)[y + 1]);
    }
    // End complete stripes section.

    const int remaining_height = plane_height - y;
    // Either one partial stripe remains (remaining_height  > 0),
    // OR image is less than one stripe high (remaining_height < 0),
    // OR all stripes are completed (remaining_height == 0).
    if (remaining_height <= 0) {
      return;
    }
    const GrainType* noise_stripe = (*noise_stripes)[luma_num];
    const GrainType* noise_stripe_prev = (*noise_stripes)[luma_num - 1];
    WriteOverlapLine_C<bitdepth>(noise_stripe,
                                 &noise_stripe_prev[32 * plane_width],
                                 plane_width, 17, 27, (*noise_image)[y]);

    // Check if second overlap row is in the image.
    if (remaining_height > 1) {
      WriteOverlapLine_C<bitdepth>(&noise_stripe[plane_width],
                                   &noise_stripe_prev[(32 + 1) * plane_width],
                                   plane_width, 27, 17, (*noise_image)[y + 1]);
    }
  } else {  // |subsampling_y| == 1
    // No special checks needed for partial stripes, because if one exists, the
    // first and only overlap row is guaranteed to exist.
    for (; y < plane_height; ++luma_num, y += stripe_height) {
      const GrainType* noise_stripe = (*noise_stripes)[luma_num];
      const GrainType* noise_stripe_prev = (*noise_stripes)[luma_num - 1];
      WriteOverlapLine_C<bitdepth>(noise_stripe,
                                   &noise_stripe_prev[16 * plane_width],
                                   plane_width, 22, 23, (*noise_image)[y]);
    }
  }
}

template <int bitdepth, typename GrainType, typename Pixel>
void BlendNoiseWithImageLuma_C(const void* LIBGAV1_RESTRICT noise_image_ptr,
                               int min_value, int max_luma, int scaling_shift,
                               int width, int height, int start_height,
                               const int16_t* scaling_lut_y,
                               const void* source_plane_y,
                               ptrdiff_t source_stride_y, void* dest_plane_y,
                               ptrdiff_t dest_stride_y) {
  const auto* noise_image =
      static_cast<const Array2D<GrainType>*>(noise_image_ptr);
  const auto* in_y = static_cast<const Pixel*>(source_plane_y);
  source_stride_y /= sizeof(Pixel);
  auto* out_y = static_cast<Pixel*>(dest_plane_y);
  dest_stride_y /= sizeof(Pixel);

  int y = 0;
  do {
    int x = 0;
    do {
      const int orig = in_y[y * source_stride_y + x];
      int noise = noise_image[kPlaneY][y + start_height][x];
      noise = RightShiftWithRounding(
          ScaleLut<bitdepth>(scaling_lut_y, orig) * noise, scaling_shift);
      out_y[y * dest_stride_y + x] = Clip3(orig + noise, min_value, max_luma);
    } while (++x < width);
  } while (++y < height);
}

// This function is for the case params_.chroma_scaling_from_luma == false.
template <int bitdepth, typename GrainType, typename Pixel>
void BlendNoiseWithImageChroma_C(
    Plane plane, const FilmGrainParams& params,
    const void* LIBGAV1_RESTRICT noise_image_ptr, int min_value, int max_chroma,
    int width, int height, int start_height, int subsampling_x,
    int subsampling_y, const int16_t* scaling_lut_uv,
    const void* source_plane_y, ptrdiff_t source_stride_y,
    const void* source_plane_uv, ptrdiff_t source_stride_uv,
    void* dest_plane_uv, ptrdiff_t dest_stride_uv) {
  const auto* noise_image =
      static_cast<const Array2D<GrainType>*>(noise_image_ptr);

  const int chroma_width = (width + subsampling_x) >> subsampling_x;
  const int chroma_height = (height + subsampling_y) >> subsampling_y;

  const auto* in_y = static_cast<const Pixel*>(source_plane_y);
  source_stride_y /= sizeof(Pixel);
  const auto* in_uv = static_cast<const Pixel*>(source_plane_uv);
  source_stride_uv /= sizeof(Pixel);
  auto* out_uv = static_cast<Pixel*>(dest_plane_uv);
  dest_stride_uv /= sizeof(Pixel);

  const int offset = (plane == kPlaneU) ? params.u_offset : params.v_offset;
  const int luma_multiplier =
      (plane == kPlaneU) ? params.u_luma_multiplier : params.v_luma_multiplier;
  const int multiplier =
      (plane == kPlaneU) ? params.u_multiplier : params.v_multiplier;

  const int scaling_shift = params.chroma_scaling;
  start_height >>= subsampling_y;
  int y = 0;
  do {
    int x = 0;
    do {
      const int luma_x = x << subsampling_x;
      const int luma_y = y << subsampling_y;
      const int luma_next_x = std::min(luma_x + 1, width - 1);
      int average_luma;
      if (subsampling_x != 0) {
        average_luma = RightShiftWithRounding(
            in_y[luma_y * source_stride_y + luma_x] +
                in_y[luma_y * source_stride_y + luma_next_x],
            1);
      } else {
        average_luma = in_y[luma_y * source_stride_y + luma_x];
      }
      const int orig = in_uv[y * source_stride_uv + x];
      const int combined = average_luma * luma_multiplier + orig * multiplier;
      const int merged =
          Clip3((combined >> 6) + LeftShift(offset, bitdepth - kBitdepth8), 0,
                (1 << bitdepth) - 1);
      int noise = noise_image[plane][y + start_height][x];
      noise = RightShiftWithRounding(
          ScaleLut<bitdepth>(scaling_lut_uv, merged) * noise, scaling_shift);
      out_uv[y * dest_stride_uv + x] =
          Clip3(orig + noise, min_value, max_chroma);
    } while (++x < chroma_width);
  } while (++y < chroma_height);
}

// This function is for the case params_.chroma_scaling_from_luma == true.
// This further implies that scaling_lut_u == scaling_lut_v == scaling_lut_y.
template <int bitdepth, typename GrainType, typename Pixel>
void BlendNoiseWithImageChromaWithCfl_C(
    Plane plane, const FilmGrainParams& params,
    const void* LIBGAV1_RESTRICT noise_image_ptr, int min_value, int max_chroma,
    int width, int height, int start_height, int subsampling_x,
    int subsampling_y, const int16_t* scaling_lut, const void* source_plane_y,
    ptrdiff_t source_stride_y, const void* source_plane_uv,
    ptrdiff_t source_stride_uv, void* dest_plane_uv, ptrdiff_t dest_stride_uv) {
  const auto* noise_image =
      static_cast<const Array2D<GrainType>*>(noise_image_ptr);
  const auto* in_y = static_cast<const Pixel*>(source_plane_y);
  source_stride_y /= sizeof(Pixel);
  const auto* in_uv = static_cast<const Pixel*>(source_plane_uv);
  source_stride_uv /= sizeof(Pixel);
  auto* out_uv = static_cast<Pixel*>(dest_plane_uv);
  dest_stride_uv /= sizeof(Pixel);

  const int chroma_width = (width + subsampling_x) >> subsampling_x;
  const int chroma_height = (height + subsampling_y) >> subsampling_y;
  const int scaling_shift = params.chroma_scaling;
  start_height >>= subsampling_y;
  int y = 0;
  do {
    int x = 0;
    do {
      const int luma_x = x << subsampling_x;
      const int luma_y = y << subsampling_y;
      const int luma_next_x = std::min(luma_x + 1, width - 1);
      int average_luma;
      if (subsampling_x != 0) {
        average_luma = RightShiftWithRounding(
            in_y[luma_y * source_stride_y + luma_x] +
                in_y[luma_y * source_stride_y + luma_next_x],
            1);
      } else {
        average_luma = in_y[luma_y * source_stride_y + luma_x];
      }
      const int orig_uv = in_uv[y * source_stride_uv + x];
      int noise_uv = noise_image[plane][y + start_height][x];
      noise_uv = RightShiftWithRounding(
          ScaleLut<bitdepth>(scaling_lut, average_luma) * noise_uv,
          scaling_shift);
      out_uv[y * dest_stride_uv + x] =
          Clip3(orig_uv + noise_uv, min_value, max_chroma);
    } while (++x < chroma_width);
  } while (++y < chroma_height);
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(8);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  // LumaAutoRegressionFunc
  dsp->film_grain.luma_auto_regression[0] =
      ApplyAutoRegressiveFilterToLumaGrain_C<kBitdepth8, int8_t>;
  dsp->film_grain.luma_auto_regression[1] =
      ApplyAutoRegressiveFilterToLumaGrain_C<kBitdepth8, int8_t>;
  dsp->film_grain.luma_auto_regression[2] =
      ApplyAutoRegressiveFilterToLumaGrain_C<kBitdepth8, int8_t>;

  // ChromaAutoRegressionFunc
  // Chroma autoregression should never be called when lag is 0 and use_luma is
  // false.
  dsp->film_grain.chroma_auto_regression[0][0] = nullptr;
  dsp->film_grain.chroma_auto_regression[0][1] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth8, int8_t, 1, false>;
  dsp->film_grain.chroma_auto_regression[0][2] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth8, int8_t, 2, false>;
  dsp->film_grain.chroma_auto_regression[0][3] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth8, int8_t, 3, false>;
  dsp->film_grain.chroma_auto_regression[1][0] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth8, int8_t, 0, true>;
  dsp->film_grain.chroma_auto_regression[1][1] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth8, int8_t, 1, true>;
  dsp->film_grain.chroma_auto_regression[1][2] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth8, int8_t, 2, true>;
  dsp->film_grain.chroma_auto_regression[1][3] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth8, int8_t, 3, true>;

  // ConstructNoiseStripesFunc
  dsp->film_grain.construct_noise_stripes[0] =
      ConstructNoiseStripes_C<kBitdepth8, int8_t>;
  dsp->film_grain.construct_noise_stripes[1] =
      ConstructNoiseStripesWithOverlap_C<kBitdepth8, int8_t>;

  // ConstructNoiseImageOverlapFunc
  dsp->film_grain.construct_noise_image_overlap =
      ConstructNoiseImageOverlap_C<kBitdepth8, int8_t>;

  // InitializeScalingLutFunc
  dsp->film_grain.initialize_scaling_lut =
      InitializeScalingLookupTable_C<kBitdepth8>;

  // BlendNoiseWithImageLumaFunc
  dsp->film_grain.blend_noise_luma =
      BlendNoiseWithImageLuma_C<kBitdepth8, int8_t, uint8_t>;

  // BlendNoiseWithImageChromaFunc
  dsp->film_grain.blend_noise_chroma[0] =
      BlendNoiseWithImageChroma_C<kBitdepth8, int8_t, uint8_t>;
  dsp->film_grain.blend_noise_chroma[1] =
      BlendNoiseWithImageChromaWithCfl_C<kBitdepth8, int8_t, uint8_t>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp8bpp_FilmGrainAutoregressionLuma
  dsp->film_grain.luma_auto_regression[0] =
      ApplyAutoRegressiveFilterToLumaGrain_C<kBitdepth8, int8_t>;
  dsp->film_grain.luma_auto_regression[1] =
      ApplyAutoRegressiveFilterToLumaGrain_C<kBitdepth8, int8_t>;
  dsp->film_grain.luma_auto_regression[2] =
      ApplyAutoRegressiveFilterToLumaGrain_C<kBitdepth8, int8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_FilmGrainAutoregressionChroma
  // Chroma autoregression should never be called when lag is 0 and use_luma is
  // false.
  dsp->film_grain.chroma_auto_regression[0][0] = nullptr;
  dsp->film_grain.chroma_auto_regression[0][1] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth8, int8_t, 1, false>;
  dsp->film_grain.chroma_auto_regression[0][2] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth8, int8_t, 2, false>;
  dsp->film_grain.chroma_auto_regression[0][3] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth8, int8_t, 3, false>;
  dsp->film_grain.chroma_auto_regression[1][0] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth8, int8_t, 0, true>;
  dsp->film_grain.chroma_auto_regression[1][1] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth8, int8_t, 1, true>;
  dsp->film_grain.chroma_auto_regression[1][2] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth8, int8_t, 2, true>;
  dsp->film_grain.chroma_auto_regression[1][3] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth8, int8_t, 3, true>;
#endif
#ifndef LIBGAV1_Dsp8bpp_FilmGrainConstructNoiseStripes
  dsp->film_grain.construct_noise_stripes[0] =
      ConstructNoiseStripes_C<kBitdepth8, int8_t>;
  dsp->film_grain.construct_noise_stripes[1] =
      ConstructNoiseStripesWithOverlap_C<kBitdepth8, int8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_FilmGrainConstructNoiseImageOverlap
  dsp->film_grain.construct_noise_image_overlap =
      ConstructNoiseImageOverlap_C<kBitdepth8, int8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_FilmGrainInitializeScalingLutFunc
  dsp->film_grain.initialize_scaling_lut =
      InitializeScalingLookupTable_C<kBitdepth8>;
#endif
#ifndef LIBGAV1_Dsp8bpp_FilmGrainBlendNoiseLuma
  dsp->film_grain.blend_noise_luma =
      BlendNoiseWithImageLuma_C<kBitdepth8, int8_t, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_FilmGrainBlendNoiseChroma
  dsp->film_grain.blend_noise_chroma[0] =
      BlendNoiseWithImageChroma_C<kBitdepth8, int8_t, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_FilmGrainBlendNoiseChromaWithCfl
  dsp->film_grain.blend_noise_chroma[1] =
      BlendNoiseWithImageChromaWithCfl_C<kBitdepth8, int8_t, uint8_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}

#if LIBGAV1_MAX_BITDEPTH >= 10
void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(10);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS

  // LumaAutoRegressionFunc
  dsp->film_grain.luma_auto_regression[0] =
      ApplyAutoRegressiveFilterToLumaGrain_C<kBitdepth10, int16_t>;
  dsp->film_grain.luma_auto_regression[1] =
      ApplyAutoRegressiveFilterToLumaGrain_C<kBitdepth10, int16_t>;
  dsp->film_grain.luma_auto_regression[2] =
      ApplyAutoRegressiveFilterToLumaGrain_C<kBitdepth10, int16_t>;

  // ChromaAutoRegressionFunc
  // Chroma autoregression should never be called when lag is 0 and use_luma is
  // false.
  dsp->film_grain.chroma_auto_regression[0][0] = nullptr;
  dsp->film_grain.chroma_auto_regression[0][1] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth10, int16_t, 1, false>;
  dsp->film_grain.chroma_auto_regression[0][2] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth10, int16_t, 2, false>;
  dsp->film_grain.chroma_auto_regression[0][3] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth10, int16_t, 3, false>;
  dsp->film_grain.chroma_auto_regression[1][0] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth10, int16_t, 0, true>;
  dsp->film_grain.chroma_auto_regression[1][1] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth10, int16_t, 1, true>;
  dsp->film_grain.chroma_auto_regression[1][2] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth10, int16_t, 2, true>;
  dsp->film_grain.chroma_auto_regression[1][3] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth10, int16_t, 3, true>;

  // ConstructNoiseStripesFunc
  dsp->film_grain.construct_noise_stripes[0] =
      ConstructNoiseStripes_C<kBitdepth10, int16_t>;
  dsp->film_grain.construct_noise_stripes[1] =
      ConstructNoiseStripesWithOverlap_C<kBitdepth10, int16_t>;

  // ConstructNoiseImageOverlapFunc
  dsp->film_grain.construct_noise_image_overlap =
      ConstructNoiseImageOverlap_C<kBitdepth10, int16_t>;

  // InitializeScalingLutFunc
  dsp->film_grain.initialize_scaling_lut =
      InitializeScalingLookupTable_C<kBitdepth10>;

  // BlendNoiseWithImageLumaFunc
  dsp->film_grain.blend_noise_luma =
      BlendNoiseWithImageLuma_C<kBitdepth10, int16_t, uint16_t>;

  // BlendNoiseWithImageChromaFunc
  dsp->film_grain.blend_noise_chroma[0] =
      BlendNoiseWithImageChroma_C<kBitdepth10, int16_t, uint16_t>;
  dsp->film_grain.blend_noise_chroma[1] =
      BlendNoiseWithImageChromaWithCfl_C<kBitdepth10, int16_t, uint16_t>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp10bpp_FilmGrainAutoregressionLuma
  dsp->film_grain.luma_auto_regression[0] =
      ApplyAutoRegressiveFilterToLumaGrain_C<kBitdepth10, int16_t>;
  dsp->film_grain.luma_auto_regression[1] =
      ApplyAutoRegressiveFilterToLumaGrain_C<kBitdepth10, int16_t>;
  dsp->film_grain.luma_auto_regression[2] =
      ApplyAutoRegressiveFilterToLumaGrain_C<kBitdepth10, int16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_FilmGrainAutoregressionChroma
  // Chroma autoregression should never be called when lag is 0 and use_luma is
  // false.
  dsp->film_grain.chroma_auto_regression[0][0] = nullptr;
  dsp->film_grain.chroma_auto_regression[0][1] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth10, int16_t, 1, false>;
  dsp->film_grain.chroma_auto_regression[0][2] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth10, int16_t, 2, false>;
  dsp->film_grain.chroma_auto_regression[0][3] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth10, int16_t, 3, false>;
  dsp->film_grain.chroma_auto_regression[1][0] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth10, int16_t, 0, true>;
  dsp->film_grain.chroma_auto_regression[1][1] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth10, int16_t, 1, true>;
  dsp->film_grain.chroma_auto_regression[1][2] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth10, int16_t, 2, true>;
  dsp->film_grain.chroma_auto_regression[1][3] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth10, int16_t, 3, true>;
#endif
#ifndef LIBGAV1_Dsp10bpp_FilmGrainConstructNoiseStripes
  dsp->film_grain.construct_noise_stripes[0] =
      ConstructNoiseStripes_C<kBitdepth10, int16_t>;
  dsp->film_grain.construct_noise_stripes[1] =
      ConstructNoiseStripesWithOverlap_C<kBitdepth10, int16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_FilmGrainConstructNoiseImageOverlap
  dsp->film_grain.construct_noise_image_overlap =
      ConstructNoiseImageOverlap_C<kBitdepth10, int16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_FilmGrainInitializeScalingLutFunc
  dsp->film_grain.initialize_scaling_lut =
      InitializeScalingLookupTable_C<kBitdepth10>;
#endif
#ifndef LIBGAV1_Dsp10bpp_FilmGrainBlendNoiseLuma
  dsp->film_grain.blend_noise_luma =
      BlendNoiseWithImageLuma_C<kBitdepth10, int16_t, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_FilmGrainBlendNoiseChroma
  dsp->film_grain.blend_noise_chroma[0] =
      BlendNoiseWithImageChroma_C<kBitdepth10, int16_t, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_FilmGrainBlendNoiseChromaWithCfl
  dsp->film_grain.blend_noise_chroma[1] =
      BlendNoiseWithImageChromaWithCfl_C<kBitdepth10, int16_t, uint16_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
void Init12bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(12);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS

  // LumaAutoRegressionFunc
  dsp->film_grain.luma_auto_regression[0] =
      ApplyAutoRegressiveFilterToLumaGrain_C<kBitdepth12, int16_t>;
  dsp->film_grain.luma_auto_regression[1] =
      ApplyAutoRegressiveFilterToLumaGrain_C<kBitdepth12, int16_t>;
  dsp->film_grain.luma_auto_regression[2] =
      ApplyAutoRegressiveFilterToLumaGrain_C<kBitdepth12, int16_t>;

  // ChromaAutoRegressionFunc
  // Chroma autoregression should never be called when lag is 0 and use_luma is
  // false.
  dsp->film_grain.chroma_auto_regression[0][0] = nullptr;
  dsp->film_grain.chroma_auto_regression[0][1] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth12, int16_t, 1, false>;
  dsp->film_grain.chroma_auto_regression[0][2] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth12, int16_t, 2, false>;
  dsp->film_grain.chroma_auto_regression[0][3] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth12, int16_t, 3, false>;
  dsp->film_grain.chroma_auto_regression[1][0] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth12, int16_t, 0, true>;
  dsp->film_grain.chroma_auto_regression[1][1] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth12, int16_t, 1, true>;
  dsp->film_grain.chroma_auto_regression[1][2] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth12, int16_t, 2, true>;
  dsp->film_grain.chroma_auto_regression[1][3] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth12, int16_t, 3, true>;

  // ConstructNoiseStripesFunc
  dsp->film_grain.construct_noise_stripes[0] =
      ConstructNoiseStripes_C<kBitdepth12, int16_t>;
  dsp->film_grain.construct_noise_stripes[1] =
      ConstructNoiseStripesWithOverlap_C<kBitdepth12, int16_t>;

  // ConstructNoiseImageOverlapFunc
  dsp->film_grain.construct_noise_image_overlap =
      ConstructNoiseImageOverlap_C<kBitdepth12, int16_t>;

  // InitializeScalingLutFunc
  dsp->film_grain.initialize_scaling_lut =
      InitializeScalingLookupTable_C<kBitdepth12>;

  // BlendNoiseWithImageLumaFunc
  dsp->film_grain.blend_noise_luma =
      BlendNoiseWithImageLuma_C<kBitdepth12, int16_t, uint16_t>;

  // BlendNoiseWithImageChromaFunc
  dsp->film_grain.blend_noise_chroma[0] =
      BlendNoiseWithImageChroma_C<kBitdepth12, int16_t, uint16_t>;
  dsp->film_grain.blend_noise_chroma[1] =
      BlendNoiseWithImageChromaWithCfl_C<kBitdepth12, int16_t, uint16_t>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp12bpp_FilmGrainAutoregressionLuma
  dsp->film_grain.luma_auto_regression[0] =
      ApplyAutoRegressiveFilterToLumaGrain_C<kBitdepth12, int16_t>;
  dsp->film_grain.luma_auto_regression[1] =
      ApplyAutoRegressiveFilterToLumaGrain_C<kBitdepth12, int16_t>;
  dsp->film_grain.luma_auto_regression[2] =
      ApplyAutoRegressiveFilterToLumaGrain_C<kBitdepth12, int16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_FilmGrainAutoregressionChroma
  // Chroma autoregression should never be called when lag is 0 and use_luma is
  // false.
  dsp->film_grain.chroma_auto_regression[0][0] = nullptr;
  dsp->film_grain.chroma_auto_regression[0][1] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth12, int16_t, 1, false>;
  dsp->film_grain.chroma_auto_regression[0][2] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth12, int16_t, 2, false>;
  dsp->film_grain.chroma_auto_regression[0][3] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth12, int16_t, 3, false>;
  dsp->film_grain.chroma_auto_regression[1][0] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth12, int16_t, 0, true>;
  dsp->film_grain.chroma_auto_regression[1][1] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth12, int16_t, 1, true>;
  dsp->film_grain.chroma_auto_regression[1][2] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth12, int16_t, 2, true>;
  dsp->film_grain.chroma_auto_regression[1][3] =
      ApplyAutoRegressiveFilterToChromaGrains_C<kBitdepth12, int16_t, 3, true>;
#endif
#ifndef LIBGAV1_Dsp12bpp_FilmGrainConstructNoiseStripes
  dsp->film_grain.construct_noise_stripes[0] =
      ConstructNoiseStripes_C<kBitdepth12, int16_t>;
  dsp->film_grain.construct_noise_stripes[1] =
      ConstructNoiseStripesWithOverlap_C<kBitdepth12, int16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_FilmGrainConstructNoiseImageOverlap
  dsp->film_grain.construct_noise_image_overlap =
      ConstructNoiseImageOverlap_C<kBitdepth12, int16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_FilmGrainInitializeScalingLutFunc
  dsp->film_grain.initialize_scaling_lut =
      InitializeScalingLookupTable_C<kBitdepth12>;
#endif
#ifndef LIBGAV1_Dsp12bpp_FilmGrainBlendNoiseLuma
  dsp->film_grain.blend_noise_luma =
      BlendNoiseWithImageLuma_C<kBitdepth12, int16_t, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_FilmGrainBlendNoiseChroma
  dsp->film_grain.blend_noise_chroma[0] =
      BlendNoiseWithImageChroma_C<kBitdepth12, int16_t, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_FilmGrainBlendNoiseChromaWithCfl
  dsp->film_grain.blend_noise_chroma[1] =
      BlendNoiseWithImageChromaWithCfl_C<kBitdepth12, int16_t, uint16_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace
}  // namespace film_grain

void FilmGrainInit_C() {
  film_grain::Init8bpp();
#if LIBGAV1_MAX_BITDEPTH >= 10
  film_grain::Init10bpp();
#endif
#if LIBGAV1_MAX_BITDEPTH == 12
  film_grain::Init12bpp();
#endif
}

}  // namespace dsp
}  // namespace libgav1
