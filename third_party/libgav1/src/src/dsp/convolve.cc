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

#include "src/dsp/convolve.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"

namespace libgav1 {
namespace dsp {
namespace {

constexpr int kHorizontalOffset = 3;
constexpr int kVerticalOffset = 3;

// Compound prediction output ranges from ConvolveTest.ShowRange.
// In some cases, the horizontal or vertical filter will be omitted. This table
// shows the general case, where the downscaled horizontal output is input to
// the vertical filter via the |intermediate_result| array. The final output is
// either Pixel or compound values, depending on the |is_compound| variable.
// Bitdepth:  8 Input range:            [       0,      255]
//   Horizontal upscaled range:         [   -7140,    23460]
//   Horizontal downscaled range:       [   -1785,     5865]
//   Vertical upscaled range:           [ -328440,   589560]
//   Pixel output range:                [       0,      255]
//   Compound output range:             [   -5132,     9212]
//
// Bitdepth: 10 Input range:            [       0,     1023]
//   Horizontal upscaled range:         [  -28644,    94116]
//   Horizontal downscaled range:       [   -7161,    23529]
//   Vertical upscaled range:           [-1317624,  2365176]
//   Pixel output range:                [       0,     1023]
//   Compound output range:             [    3988,    61532]
//
// Bitdepth: 12 Input range:            [       0,     4095]
//   Horizontal upscaled range:         [ -114660,   376740]
//   Horizontal downscaled range:       [   -7166,    23546]
//   Vertical upscaled range:           [-1318560,  2366880]
//   Pixel output range:                [       0,     4095]
//   Compound output range:             [    3974,    61559]

template <int bitdepth, typename Pixel>
void ConvolveScale2D_C(const void* LIBGAV1_RESTRICT const reference,
                       const ptrdiff_t reference_stride,
                       const int horizontal_filter_index,
                       const int vertical_filter_index, const int subpixel_x,
                       const int subpixel_y, const int step_x, const int step_y,
                       const int width, const int height,
                       void* LIBGAV1_RESTRICT prediction,
                       const ptrdiff_t pred_stride) {
  constexpr int kRoundBitsHorizontal = (bitdepth == 12)
                                           ? kInterRoundBitsHorizontal12bpp
                                           : kInterRoundBitsHorizontal;
  constexpr int kRoundBitsVertical =
      (bitdepth == 12) ? kInterRoundBitsVertical12bpp : kInterRoundBitsVertical;
  const int intermediate_height =
      (((height - 1) * step_y + (1 << kScaleSubPixelBits) - 1) >>
       kScaleSubPixelBits) +
      kSubPixelTaps;
  // The output of the horizontal filter, i.e. the intermediate_result, is
  // guaranteed to fit in int16_t.
  int16_t intermediate_result[kMaxSuperBlockSizeInPixels *
                              (2 * kMaxSuperBlockSizeInPixels + 8)];
  const int intermediate_stride = kMaxSuperBlockSizeInPixels;
  const int max_pixel_value = (1 << bitdepth) - 1;

  // Horizontal filter.
  // Filter types used for width <= 4 are different from those for width > 4.
  // When width > 4, the valid filter index range is always [0, 3].
  // When width <= 4, the valid filter index range is always [4, 5].
  // Similarly for height.
  int filter_index = GetFilterIndex(horizontal_filter_index, width);
  int16_t* intermediate = intermediate_result;
  const auto* src = static_cast<const Pixel*>(reference);
  const ptrdiff_t src_stride = reference_stride / sizeof(Pixel);
  auto* dest = static_cast<Pixel*>(prediction);
  const ptrdiff_t dest_stride = pred_stride / sizeof(Pixel);
  const int ref_x = subpixel_x >> kScaleSubPixelBits;
  // Note: assume the input src is already aligned to the correct start
  // position.
  int y = 0;
  do {
    int p = subpixel_x;
    int x = 0;
    do {
      int sum = 0;
      const Pixel* src_x = &src[(p >> kScaleSubPixelBits) - ref_x];
      const int filter_id = (p >> 6) & kSubPixelMask;
      for (int k = 0; k < kSubPixelTaps; ++k) {
        sum += kHalfSubPixelFilters[filter_index][filter_id][k] * src_x[k];
      }
      intermediate[x] = RightShiftWithRounding(sum, kRoundBitsHorizontal - 1);
      p += step_x;
    } while (++x < width);

    src += src_stride;
    intermediate += intermediate_stride;
  } while (++y < intermediate_height);

  // Vertical filter.
  filter_index = GetFilterIndex(vertical_filter_index, height);
  intermediate = intermediate_result;
  int p = subpixel_y & 1023;
  y = 0;
  do {
    const int filter_id = (p >> 6) & kSubPixelMask;
    int x = 0;
    do {
      int sum = 0;
      for (int k = 0; k < kSubPixelTaps; ++k) {
        sum +=
            kHalfSubPixelFilters[filter_index][filter_id][k] *
            intermediate[((p >> kScaleSubPixelBits) + k) * intermediate_stride +
                         x];
      }
      dest[x] = Clip3(RightShiftWithRounding(sum, kRoundBitsVertical - 1), 0,
                      max_pixel_value);
    } while (++x < width);

    dest += dest_stride;
    p += step_y;
  } while (++y < height);
}

template <int bitdepth, typename Pixel>
void ConvolveCompoundScale2D_C(
    const void* LIBGAV1_RESTRICT const reference,
    const ptrdiff_t reference_stride, const int horizontal_filter_index,
    const int vertical_filter_index, const int subpixel_x, const int subpixel_y,
    const int step_x, const int step_y, const int width, const int height,
    void* LIBGAV1_RESTRICT prediction, const ptrdiff_t pred_stride) {
  // All compound functions output to the predictor buffer with |pred_stride|
  // equal to |width|.
  assert(pred_stride == width);
  // Compound functions start at 4x4.
  assert(width >= 4 && height >= 4);
  constexpr int kRoundBitsHorizontal = (bitdepth == 12)
                                           ? kInterRoundBitsHorizontal12bpp
                                           : kInterRoundBitsHorizontal;
  constexpr int kRoundBitsVertical = kInterRoundBitsCompoundVertical;
  const int intermediate_height =
      (((height - 1) * step_y + (1 << kScaleSubPixelBits) - 1) >>
       kScaleSubPixelBits) +
      kSubPixelTaps;
  // The output of the horizontal filter, i.e. the intermediate_result, is
  // guaranteed to fit in int16_t.
  int16_t intermediate_result[kMaxSuperBlockSizeInPixels *
                              (2 * kMaxSuperBlockSizeInPixels + 8)];
  const int intermediate_stride = kMaxSuperBlockSizeInPixels;

  // Horizontal filter.
  // Filter types used for width <= 4 are different from those for width > 4.
  // When width > 4, the valid filter index range is always [0, 3].
  // When width <= 4, the valid filter index range is always [4, 5].
  // Similarly for height.
  int filter_index = GetFilterIndex(horizontal_filter_index, width);
  int16_t* intermediate = intermediate_result;
  const auto* src = static_cast<const Pixel*>(reference);
  const ptrdiff_t src_stride = reference_stride / sizeof(Pixel);
  auto* dest = static_cast<uint16_t*>(prediction);
  const int ref_x = subpixel_x >> kScaleSubPixelBits;
  // Note: assume the input src is already aligned to the correct start
  // position.
  int y = 0;
  do {
    int p = subpixel_x;
    int x = 0;
    do {
      int sum = 0;
      const Pixel* src_x = &src[(p >> kScaleSubPixelBits) - ref_x];
      const int filter_id = (p >> 6) & kSubPixelMask;
      for (int k = 0; k < kSubPixelTaps; ++k) {
        sum += kHalfSubPixelFilters[filter_index][filter_id][k] * src_x[k];
      }
      intermediate[x] = RightShiftWithRounding(sum, kRoundBitsHorizontal - 1);
      p += step_x;
    } while (++x < width);

    src += src_stride;
    intermediate += intermediate_stride;
  } while (++y < intermediate_height);

  // Vertical filter.
  filter_index = GetFilterIndex(vertical_filter_index, height);
  intermediate = intermediate_result;
  int p = subpixel_y & 1023;
  y = 0;
  do {
    const int filter_id = (p >> 6) & kSubPixelMask;
    int x = 0;
    do {
      int sum = 0;
      for (int k = 0; k < kSubPixelTaps; ++k) {
        sum +=
            kHalfSubPixelFilters[filter_index][filter_id][k] *
            intermediate[((p >> kScaleSubPixelBits) + k) * intermediate_stride +
                         x];
      }
      sum = RightShiftWithRounding(sum, kRoundBitsVertical - 1);
      sum += (bitdepth == 8) ? 0 : kCompoundOffset;
      dest[x] = sum;
    } while (++x < width);

    dest += pred_stride;
    p += step_y;
  } while (++y < height);
}

template <int bitdepth, typename Pixel>
void ConvolveCompound2D_C(const void* LIBGAV1_RESTRICT const reference,
                          const ptrdiff_t reference_stride,
                          const int horizontal_filter_index,
                          const int vertical_filter_index,
                          const int horizontal_filter_id,
                          const int vertical_filter_id, const int width,
                          const int height, void* LIBGAV1_RESTRICT prediction,
                          const ptrdiff_t pred_stride) {
  // All compound functions output to the predictor buffer with |pred_stride|
  // equal to |width|.
  assert(pred_stride == width);
  // Compound functions start at 4x4.
  assert(width >= 4 && height >= 4);
  constexpr int kRoundBitsHorizontal = (bitdepth == 12)
                                           ? kInterRoundBitsHorizontal12bpp
                                           : kInterRoundBitsHorizontal;
  constexpr int kRoundBitsVertical = kInterRoundBitsCompoundVertical;
  const int intermediate_height = height + kSubPixelTaps - 1;
  // The output of the horizontal filter, i.e. the intermediate_result, is
  // guaranteed to fit in int16_t.
  int16_t intermediate_result[kMaxSuperBlockSizeInPixels *
                              (kMaxSuperBlockSizeInPixels + kSubPixelTaps - 1)];
  const int intermediate_stride = kMaxSuperBlockSizeInPixels;

  // Horizontal filter.
  // Filter types used for width <= 4 are different from those for width > 4.
  // When width > 4, the valid filter index range is always [0, 3].
  // When width <= 4, the valid filter index range is always [4, 5].
  // Similarly for height.
  int filter_index = GetFilterIndex(horizontal_filter_index, width);
  int16_t* intermediate = intermediate_result;
  const ptrdiff_t src_stride = reference_stride / sizeof(Pixel);
  const auto* src = static_cast<const Pixel*>(reference) -
                    kVerticalOffset * src_stride - kHorizontalOffset;
  auto* dest = static_cast<uint16_t*>(prediction);

  // If |horizontal_filter_id| == 0 then ConvolveVertical() should be called.
  assert(horizontal_filter_id != 0);
  int y = 0;
  do {
    int x = 0;
    do {
      int sum = 0;
      for (int k = 0; k < kSubPixelTaps; ++k) {
        sum += kHalfSubPixelFilters[filter_index][horizontal_filter_id][k] *
               src[x + k];
      }
      intermediate[x] = RightShiftWithRounding(sum, kRoundBitsHorizontal - 1);
    } while (++x < width);

    src += src_stride;
    intermediate += intermediate_stride;
  } while (++y < intermediate_height);

  // Vertical filter.
  filter_index = GetFilterIndex(vertical_filter_index, height);
  intermediate = intermediate_result;
  // If |vertical_filter_id| == 0 then ConvolveHorizontal() should be called.
  assert(vertical_filter_id != 0);
  y = 0;
  do {
    int x = 0;
    do {
      int sum = 0;
      for (int k = 0; k < kSubPixelTaps; ++k) {
        sum += kHalfSubPixelFilters[filter_index][vertical_filter_id][k] *
               intermediate[k * intermediate_stride + x];
      }
      sum = RightShiftWithRounding(sum, kRoundBitsVertical - 1);
      sum += (bitdepth == 8) ? 0 : kCompoundOffset;
      dest[x] = sum;
    } while (++x < width);

    dest += pred_stride;
    intermediate += intermediate_stride;
  } while (++y < height);
}

// This function is a simplified version of ConvolveCompound2D_C.
// It is called when it is single prediction mode, where both horizontal and
// vertical filtering are required.
// The output is the single prediction of the block, clipped to valid pixel
// range.
template <int bitdepth, typename Pixel>
void Convolve2D_C(const void* LIBGAV1_RESTRICT const reference,
                  const ptrdiff_t reference_stride,
                  const int horizontal_filter_index,
                  const int vertical_filter_index,
                  const int horizontal_filter_id, const int vertical_filter_id,
                  const int width, const int height,
                  void* LIBGAV1_RESTRICT prediction,
                  const ptrdiff_t pred_stride) {
  constexpr int kRoundBitsHorizontal = (bitdepth == 12)
                                           ? kInterRoundBitsHorizontal12bpp
                                           : kInterRoundBitsHorizontal;
  constexpr int kRoundBitsVertical =
      (bitdepth == 12) ? kInterRoundBitsVertical12bpp : kInterRoundBitsVertical;
  const int intermediate_height = height + kSubPixelTaps - 1;
  // The output of the horizontal filter, i.e. the intermediate_result, is
  // guaranteed to fit in int16_t.
  int16_t intermediate_result[kMaxSuperBlockSizeInPixels *
                              (kMaxSuperBlockSizeInPixels + kSubPixelTaps - 1)];
  const int intermediate_stride = kMaxSuperBlockSizeInPixels;
  const int max_pixel_value = (1 << bitdepth) - 1;

  // Horizontal filter.
  // Filter types used for width <= 4 are different from those for width > 4.
  // When width > 4, the valid filter index range is always [0, 3].
  // When width <= 4, the valid filter index range is always [4, 5].
  // Similarly for height.
  int filter_index = GetFilterIndex(horizontal_filter_index, width);
  int16_t* intermediate = intermediate_result;
  const ptrdiff_t src_stride = reference_stride / sizeof(Pixel);
  const auto* src = static_cast<const Pixel*>(reference) -
                    kVerticalOffset * src_stride - kHorizontalOffset;
  auto* dest = static_cast<Pixel*>(prediction);
  const ptrdiff_t dest_stride = pred_stride / sizeof(Pixel);
  // If |horizontal_filter_id| == 0 then ConvolveVertical() should be called.
  assert(horizontal_filter_id != 0);
  int y = 0;
  do {
    int x = 0;
    do {
      int sum = 0;
      for (int k = 0; k < kSubPixelTaps; ++k) {
        sum += kHalfSubPixelFilters[filter_index][horizontal_filter_id][k] *
               src[x + k];
      }
      intermediate[x] = RightShiftWithRounding(sum, kRoundBitsHorizontal - 1);
    } while (++x < width);

    src += src_stride;
    intermediate += intermediate_stride;
  } while (++y < intermediate_height);

  // Vertical filter.
  filter_index = GetFilterIndex(vertical_filter_index, height);
  intermediate = intermediate_result;
  // If |vertical_filter_id| == 0 then ConvolveHorizontal() should be called.
  assert(vertical_filter_id != 0);
  y = 0;
  do {
    int x = 0;
    do {
      int sum = 0;
      for (int k = 0; k < kSubPixelTaps; ++k) {
        sum += kHalfSubPixelFilters[filter_index][vertical_filter_id][k] *
               intermediate[k * intermediate_stride + x];
      }
      dest[x] = Clip3(RightShiftWithRounding(sum, kRoundBitsVertical - 1), 0,
                      max_pixel_value);
    } while (++x < width);

    dest += dest_stride;
    intermediate += intermediate_stride;
  } while (++y < height);
}

// This function is a simplified version of Convolve2D_C.
// It is called when it is single prediction mode, where only horizontal
// filtering is required.
// The output is the single prediction of the block, clipped to valid pixel
// range.
template <int bitdepth, typename Pixel>
void ConvolveHorizontal_C(const void* LIBGAV1_RESTRICT const reference,
                          const ptrdiff_t reference_stride,
                          const int horizontal_filter_index,
                          const int /*vertical_filter_index*/,
                          const int horizontal_filter_id,
                          const int /*vertical_filter_id*/, const int width,
                          const int height, void* LIBGAV1_RESTRICT prediction,
                          const ptrdiff_t pred_stride) {
  constexpr int kRoundBitsHorizontal = (bitdepth == 12)
                                           ? kInterRoundBitsHorizontal12bpp
                                           : kInterRoundBitsHorizontal;
  const int filter_index = GetFilterIndex(horizontal_filter_index, width);
  const int bits = kFilterBits - kRoundBitsHorizontal;
  const auto* src = static_cast<const Pixel*>(reference) - kHorizontalOffset;
  const ptrdiff_t src_stride = reference_stride / sizeof(Pixel);
  auto* dest = static_cast<Pixel*>(prediction);
  const ptrdiff_t dest_stride = pred_stride / sizeof(Pixel);
  const int max_pixel_value = (1 << bitdepth) - 1;
  int y = 0;
  do {
    int x = 0;
    do {
      int sum = 0;
      for (int k = 0; k < kSubPixelTaps; ++k) {
        sum += kHalfSubPixelFilters[filter_index][horizontal_filter_id][k] *
               src[x + k];
      }
      sum = RightShiftWithRounding(sum, kRoundBitsHorizontal - 1);
      dest[x] = Clip3(RightShiftWithRounding(sum, bits), 0, max_pixel_value);
    } while (++x < width);

    src += src_stride;
    dest += dest_stride;
  } while (++y < height);
}

// This function is a simplified version of Convolve2D_C.
// It is called when it is single prediction mode, where only vertical
// filtering is required.
// The output is the single prediction of the block, clipped to valid pixel
// range.
template <int bitdepth, typename Pixel>
void ConvolveVertical_C(const void* LIBGAV1_RESTRICT const reference,
                        const ptrdiff_t reference_stride,
                        const int /*horizontal_filter_index*/,
                        const int vertical_filter_index,
                        const int /*horizontal_filter_id*/,
                        const int vertical_filter_id, const int width,
                        const int height, void* LIBGAV1_RESTRICT prediction,
                        const ptrdiff_t pred_stride) {
  const int filter_index = GetFilterIndex(vertical_filter_index, height);
  const ptrdiff_t src_stride = reference_stride / sizeof(Pixel);
  const auto* src =
      static_cast<const Pixel*>(reference) - kVerticalOffset * src_stride;
  auto* dest = static_cast<Pixel*>(prediction);
  const ptrdiff_t dest_stride = pred_stride / sizeof(Pixel);
  // Copy filters must call ConvolveCopy().
  assert(vertical_filter_id != 0);

  const int max_pixel_value = (1 << bitdepth) - 1;
  int y = 0;
  do {
    int x = 0;
    do {
      int sum = 0;
      for (int k = 0; k < kSubPixelTaps; ++k) {
        sum += kHalfSubPixelFilters[filter_index][vertical_filter_id][k] *
               src[k * src_stride + x];
      }
      dest[x] = Clip3(RightShiftWithRounding(sum, kFilterBits - 1), 0,
                      max_pixel_value);
    } while (++x < width);

    src += src_stride;
    dest += dest_stride;
  } while (++y < height);
}

template <int bitdepth, typename Pixel>
void ConvolveCopy_C(const void* LIBGAV1_RESTRICT const reference,
                    const ptrdiff_t reference_stride,
                    const int /*horizontal_filter_index*/,
                    const int /*vertical_filter_index*/,
                    const int /*horizontal_filter_id*/,
                    const int /*vertical_filter_id*/, const int width,
                    const int height, void* LIBGAV1_RESTRICT prediction,
                    const ptrdiff_t pred_stride) {
  const auto* src = static_cast<const uint8_t*>(reference);
  auto* dest = static_cast<uint8_t*>(prediction);
  int y = 0;
  do {
    memcpy(dest, src, width * sizeof(Pixel));
    src += reference_stride;
    dest += pred_stride;
  } while (++y < height);
}

template <int bitdepth, typename Pixel>
void ConvolveCompoundCopy_C(const void* LIBGAV1_RESTRICT const reference,
                            const ptrdiff_t reference_stride,
                            const int /*horizontal_filter_index*/,
                            const int /*vertical_filter_index*/,
                            const int /*horizontal_filter_id*/,
                            const int /*vertical_filter_id*/, const int width,
                            const int height, void* LIBGAV1_RESTRICT prediction,
                            const ptrdiff_t pred_stride) {
  // All compound functions output to the predictor buffer with |pred_stride|
  // equal to |width|.
  assert(pred_stride == width);
  // Compound functions start at 4x4.
  assert(width >= 4 && height >= 4);
  constexpr int kRoundBitsVertical =
      ((bitdepth == 12) ? kInterRoundBitsVertical12bpp
                        : kInterRoundBitsVertical) -
      kInterRoundBitsCompoundVertical;
  const auto* src = static_cast<const Pixel*>(reference);
  const ptrdiff_t src_stride = reference_stride / sizeof(Pixel);
  auto* dest = static_cast<uint16_t*>(prediction);
  int y = 0;
  do {
    int x = 0;
    do {
      int sum = (bitdepth == 8) ? 0 : ((1 << bitdepth) + (1 << (bitdepth - 1)));
      sum += src[x];
      dest[x] = sum << kRoundBitsVertical;
    } while (++x < width);
    src += src_stride;
    dest += pred_stride;
  } while (++y < height);
}

// This function is a simplified version of ConvolveCompound2D_C.
// It is called when it is compound prediction mode, where only horizontal
// filtering is required.
// The output is not clipped to valid pixel range. Its output will be
// blended with another predictor to generate the final prediction of the block.
template <int bitdepth, typename Pixel>
void ConvolveCompoundHorizontal_C(
    const void* LIBGAV1_RESTRICT const reference,
    const ptrdiff_t reference_stride, const int horizontal_filter_index,
    const int /*vertical_filter_index*/, const int horizontal_filter_id,
    const int /*vertical_filter_id*/, const int width, const int height,
    void* LIBGAV1_RESTRICT prediction, const ptrdiff_t pred_stride) {
  // All compound functions output to the predictor buffer with |pred_stride|
  // equal to |width|.
  assert(pred_stride == width);
  // Compound functions start at 4x4.
  assert(width >= 4 && height >= 4);
  constexpr int kRoundBitsHorizontal = (bitdepth == 12)
                                           ? kInterRoundBitsHorizontal12bpp
                                           : kInterRoundBitsHorizontal;
  const int filter_index = GetFilterIndex(horizontal_filter_index, width);
  const auto* src = static_cast<const Pixel*>(reference) - kHorizontalOffset;
  const ptrdiff_t src_stride = reference_stride / sizeof(Pixel);
  auto* dest = static_cast<uint16_t*>(prediction);
  // Copy filters must call ConvolveCopy().
  assert(horizontal_filter_id != 0);
  int y = 0;
  do {
    int x = 0;
    do {
      int sum = 0;
      for (int k = 0; k < kSubPixelTaps; ++k) {
        sum += kHalfSubPixelFilters[filter_index][horizontal_filter_id][k] *
               src[x + k];
      }
      sum = RightShiftWithRounding(sum, kRoundBitsHorizontal - 1);
      sum += (bitdepth == 8) ? 0 : kCompoundOffset;
      dest[x] = sum;
    } while (++x < width);

    src += src_stride;
    dest += pred_stride;
  } while (++y < height);
}

// This function is a simplified version of ConvolveCompound2D_C.
// It is called when it is compound prediction mode, where only vertical
// filtering is required.
// The output is not clipped to valid pixel range. Its output will be
// blended with another predictor to generate the final prediction of the block.
template <int bitdepth, typename Pixel>
void ConvolveCompoundVertical_C(
    const void* LIBGAV1_RESTRICT const reference,
    const ptrdiff_t reference_stride, const int /*horizontal_filter_index*/,
    const int vertical_filter_index, const int /*horizontal_filter_id*/,
    const int vertical_filter_id, const int width, const int height,
    void* LIBGAV1_RESTRICT prediction, const ptrdiff_t pred_stride) {
  // All compound functions output to the predictor buffer with |pred_stride|
  // equal to |width|.
  assert(pred_stride == width);
  // Compound functions start at 4x4.
  assert(width >= 4 && height >= 4);
  constexpr int kRoundBitsHorizontal = (bitdepth == 12)
                                           ? kInterRoundBitsHorizontal12bpp
                                           : kInterRoundBitsHorizontal;
  const int filter_index = GetFilterIndex(vertical_filter_index, height);
  const ptrdiff_t src_stride = reference_stride / sizeof(Pixel);
  const auto* src =
      static_cast<const Pixel*>(reference) - kVerticalOffset * src_stride;
  auto* dest = static_cast<uint16_t*>(prediction);
  // Copy filters must call ConvolveCopy().
  assert(vertical_filter_id != 0);
  int y = 0;
  do {
    int x = 0;
    do {
      int sum = 0;
      for (int k = 0; k < kSubPixelTaps; ++k) {
        sum += kHalfSubPixelFilters[filter_index][vertical_filter_id][k] *
               src[k * src_stride + x];
      }
      sum = RightShiftWithRounding(sum, kRoundBitsHorizontal - 1);
      sum += (bitdepth == 8) ? 0 : kCompoundOffset;
      dest[x] = sum;
    } while (++x < width);
    src += src_stride;
    dest += pred_stride;
  } while (++y < height);
}

// This function is used when intra block copy is present.
// It is called when it is single prediction mode for U/V plane, where the
// reference block is from current frame and both horizontal and vertical
// filtering are required.
// The output is the single prediction of the block, clipped to valid pixel
// range.
template <int bitdepth, typename Pixel>
void ConvolveIntraBlockCopy2D_C(
    const void* LIBGAV1_RESTRICT const reference,
    const ptrdiff_t reference_stride, const int /*horizontal_filter_index*/,
    const int /*vertical_filter_index*/, const int /*horizontal_filter_id*/,
    const int /*vertical_filter_id*/, const int width, const int height,
    void* LIBGAV1_RESTRICT prediction, const ptrdiff_t pred_stride) {
  assert(width >= 4 && width <= kMaxSuperBlockSizeInPixels);
  assert(height >= 4 && height <= kMaxSuperBlockSizeInPixels);
  const auto* src = static_cast<const Pixel*>(reference);
  const ptrdiff_t src_stride = reference_stride / sizeof(Pixel);
  auto* dest = static_cast<Pixel*>(prediction);
  const ptrdiff_t dest_stride = pred_stride / sizeof(Pixel);
  const int intermediate_height = height + 1;
  uint16_t intermediate_result[kMaxSuperBlockSizeInPixels *
                               (kMaxSuperBlockSizeInPixels + 1)];
  uint16_t* intermediate = intermediate_result;
  // Note: allow vertical access to height + 1. Because this function is only
  // for u/v plane of intra block copy, such access is guaranteed to be within
  // the prediction block.
  int y = 0;
  do {
    int x = 0;
    do {
      intermediate[x] = src[x] + src[x + 1];
    } while (++x < width);

    src += src_stride;
    intermediate += width;
  } while (++y < intermediate_height);

  intermediate = intermediate_result;
  y = 0;
  do {
    int x = 0;
    do {
      dest[x] =
          RightShiftWithRounding(intermediate[x] + intermediate[x + width], 2);
    } while (++x < width);

    intermediate += width;
    dest += dest_stride;
  } while (++y < height);
}

// This function is used when intra block copy is present.
// It is called when it is single prediction mode for U/V plane, where the
// reference block is from the current frame and only horizontal or vertical
// filtering is required.
// The output is the single prediction of the block, clipped to valid pixel
// range.
// The filtering of intra block copy is simply the average of current and
// the next pixel.
template <int bitdepth, typename Pixel, bool is_horizontal>
void ConvolveIntraBlockCopy1D_C(
    const void* LIBGAV1_RESTRICT const reference,
    const ptrdiff_t reference_stride, const int /*horizontal_filter_index*/,
    const int /*vertical_filter_index*/, const int /*horizontal_filter_id*/,
    const int /*vertical_filter_id*/, const int width, const int height,
    void* LIBGAV1_RESTRICT prediction, const ptrdiff_t pred_stride) {
  assert(width >= 4 && width <= kMaxSuperBlockSizeInPixels);
  assert(height >= 4 && height <= kMaxSuperBlockSizeInPixels);
  const auto* src = static_cast<const Pixel*>(reference);
  const ptrdiff_t src_stride = reference_stride / sizeof(Pixel);
  auto* dest = static_cast<Pixel*>(prediction);
  const ptrdiff_t dest_stride = pred_stride / sizeof(Pixel);
  const ptrdiff_t offset = is_horizontal ? 1 : src_stride;
  int y = 0;
  do {
    int x = 0;
    do {
      dest[x] = RightShiftWithRounding(src[x] + src[x + offset], 1);
    } while (++x < width);

    src += src_stride;
    dest += dest_stride;
  } while (++y < height);
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(8);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->convolve[0][0][0][0] = ConvolveCopy_C<8, uint8_t>;
  dsp->convolve[0][0][0][1] = ConvolveHorizontal_C<8, uint8_t>;
  dsp->convolve[0][0][1][0] = ConvolveVertical_C<8, uint8_t>;
  dsp->convolve[0][0][1][1] = Convolve2D_C<8, uint8_t>;

  dsp->convolve[0][1][0][0] = ConvolveCompoundCopy_C<8, uint8_t>;
  dsp->convolve[0][1][0][1] = ConvolveCompoundHorizontal_C<8, uint8_t>;
  dsp->convolve[0][1][1][0] = ConvolveCompoundVertical_C<8, uint8_t>;
  dsp->convolve[0][1][1][1] = ConvolveCompound2D_C<8, uint8_t>;

  dsp->convolve[1][0][0][0] = ConvolveCopy_C<8, uint8_t>;
  dsp->convolve[1][0][0][1] =
      ConvolveIntraBlockCopy1D_C<8, uint8_t, /*is_horizontal=*/true>;
  dsp->convolve[1][0][1][0] =
      ConvolveIntraBlockCopy1D_C<8, uint8_t, /*is_horizontal=*/false>;
  dsp->convolve[1][0][1][1] = ConvolveIntraBlockCopy2D_C<8, uint8_t>;

  dsp->convolve[1][1][0][0] = nullptr;
  dsp->convolve[1][1][0][1] = nullptr;
  dsp->convolve[1][1][1][0] = nullptr;
  dsp->convolve[1][1][1][1] = nullptr;

  dsp->convolve_scale[0] = ConvolveScale2D_C<8, uint8_t>;
  dsp->convolve_scale[1] = ConvolveCompoundScale2D_C<8, uint8_t>;
#else  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
#ifndef LIBGAV1_Dsp8bpp_ConvolveCopy
  dsp->convolve[0][0][0][0] = ConvolveCopy_C<8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_ConvolveHorizontal
  dsp->convolve[0][0][0][1] = ConvolveHorizontal_C<8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_ConvolveVertical
  dsp->convolve[0][0][1][0] = ConvolveVertical_C<8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_Convolve2D
  dsp->convolve[0][0][1][1] = Convolve2D_C<8, uint8_t>;
#endif

#ifndef LIBGAV1_Dsp8bpp_ConvolveCompoundCopy
  dsp->convolve[0][1][0][0] = ConvolveCompoundCopy_C<8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_ConvolveCompoundHorizontal
  dsp->convolve[0][1][0][1] = ConvolveCompoundHorizontal_C<8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_ConvolveCompoundVertical
  dsp->convolve[0][1][1][0] = ConvolveCompoundVertical_C<8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_ConvolveCompound2D
  dsp->convolve[0][1][1][1] = ConvolveCompound2D_C<8, uint8_t>;
#endif

#ifndef LIBGAV1_Dsp8bpp_ConvolveIntraBlockCopy
  dsp->convolve[1][0][0][0] = ConvolveCopy_C<8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_ConvolveIntraBlockCopyHorizontal
  dsp->convolve[1][0][0][1] =
      ConvolveIntraBlockCopy1D_C<8, uint8_t, /*is_horizontal=*/true>;
#endif
#ifndef LIBGAV1_Dsp8bpp_ConvolveIntraBlockCopyVertical
  dsp->convolve[1][0][1][0] =
      ConvolveIntraBlockCopy1D_C<8, uint8_t, /*is_horizontal=*/false>;
#endif
#ifndef LIBGAV1_Dsp8bpp_ConvolveIntraBlockCopy2D
  dsp->convolve[1][0][1][1] = ConvolveIntraBlockCopy2D_C<8, uint8_t>;
#endif

  dsp->convolve[1][1][0][0] = nullptr;
  dsp->convolve[1][1][0][1] = nullptr;
  dsp->convolve[1][1][1][0] = nullptr;
  dsp->convolve[1][1][1][1] = nullptr;

#ifndef LIBGAV1_Dsp8bpp_ConvolveScale2D
  dsp->convolve_scale[0] = ConvolveScale2D_C<8, uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_ConvolveCompoundScale2D
  dsp->convolve_scale[1] = ConvolveCompoundScale2D_C<8, uint8_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}

#if LIBGAV1_MAX_BITDEPTH >= 10
void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(10);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->convolve[0][0][0][0] = ConvolveCopy_C<10, uint16_t>;
  dsp->convolve[0][0][0][1] = ConvolveHorizontal_C<10, uint16_t>;
  dsp->convolve[0][0][1][0] = ConvolveVertical_C<10, uint16_t>;
  dsp->convolve[0][0][1][1] = Convolve2D_C<10, uint16_t>;

  dsp->convolve[0][1][0][0] = ConvolveCompoundCopy_C<10, uint16_t>;
  dsp->convolve[0][1][0][1] = ConvolveCompoundHorizontal_C<10, uint16_t>;
  dsp->convolve[0][1][1][0] = ConvolveCompoundVertical_C<10, uint16_t>;
  dsp->convolve[0][1][1][1] = ConvolveCompound2D_C<10, uint16_t>;

  dsp->convolve[1][0][0][0] = ConvolveCopy_C<10, uint16_t>;
  dsp->convolve[1][0][0][1] =
      ConvolveIntraBlockCopy1D_C<10, uint16_t, /*is_horizontal=*/true>;
  dsp->convolve[1][0][1][0] =
      ConvolveIntraBlockCopy1D_C<10, uint16_t, /*is_horizontal=*/false>;
  dsp->convolve[1][0][1][1] = ConvolveIntraBlockCopy2D_C<10, uint16_t>;

  dsp->convolve[1][1][0][0] = nullptr;
  dsp->convolve[1][1][0][1] = nullptr;
  dsp->convolve[1][1][1][0] = nullptr;
  dsp->convolve[1][1][1][1] = nullptr;

  dsp->convolve_scale[0] = ConvolveScale2D_C<10, uint16_t>;
  dsp->convolve_scale[1] = ConvolveCompoundScale2D_C<10, uint16_t>;
#else  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
#ifndef LIBGAV1_Dsp10bpp_ConvolveCopy
  dsp->convolve[0][0][0][0] = ConvolveCopy_C<10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_ConvolveHorizontal
  dsp->convolve[0][0][0][1] = ConvolveHorizontal_C<10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_ConvolveVertical
  dsp->convolve[0][0][1][0] = ConvolveVertical_C<10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_Convolve2D
  dsp->convolve[0][0][1][1] = Convolve2D_C<10, uint16_t>;
#endif

#ifndef LIBGAV1_Dsp10bpp_ConvolveCompoundCopy
  dsp->convolve[0][1][0][0] = ConvolveCompoundCopy_C<10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_ConvolveCompoundHorizontal
  dsp->convolve[0][1][0][1] = ConvolveCompoundHorizontal_C<10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_ConvolveCompoundVertical
  dsp->convolve[0][1][1][0] = ConvolveCompoundVertical_C<10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_ConvolveCompound2D
  dsp->convolve[0][1][1][1] = ConvolveCompound2D_C<10, uint16_t>;
#endif

#ifndef LIBGAV1_Dsp10bpp_ConvolveIntraBlockCopy
  dsp->convolve[1][0][0][0] = ConvolveCopy_C<10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_ConvolveIntraBlockHorizontal
  dsp->convolve[1][0][0][1] =
      ConvolveIntraBlockCopy1D_C<10, uint16_t, /*is_horizontal=*/true>;
#endif
#ifndef LIBGAV1_Dsp10bpp_ConvolveIntraBlockVertical
  dsp->convolve[1][0][1][0] =
      ConvolveIntraBlockCopy1D_C<10, uint16_t, /*is_horizontal=*/false>;
#endif
#ifndef LIBGAV1_Dsp10bpp_ConvolveIntraBlock2D
  dsp->convolve[1][0][1][1] = ConvolveIntraBlockCopy2D_C<10, uint16_t>;
#endif

  dsp->convolve[1][1][0][0] = nullptr;
  dsp->convolve[1][1][0][1] = nullptr;
  dsp->convolve[1][1][1][0] = nullptr;
  dsp->convolve[1][1][1][1] = nullptr;

#ifndef LIBGAV1_Dsp10bpp_ConvolveScale2D
  dsp->convolve_scale[0] = ConvolveScale2D_C<10, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_ConvolveCompoundScale2D
  dsp->convolve_scale[1] = ConvolveCompoundScale2D_C<10, uint16_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
void Init12bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(12);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->convolve[0][0][0][0] = ConvolveCopy_C<12, uint16_t>;
  dsp->convolve[0][0][0][1] = ConvolveHorizontal_C<12, uint16_t>;
  dsp->convolve[0][0][1][0] = ConvolveVertical_C<12, uint16_t>;
  dsp->convolve[0][0][1][1] = Convolve2D_C<12, uint16_t>;

  dsp->convolve[0][1][0][0] = ConvolveCompoundCopy_C<12, uint16_t>;
  dsp->convolve[0][1][0][1] = ConvolveCompoundHorizontal_C<12, uint16_t>;
  dsp->convolve[0][1][1][0] = ConvolveCompoundVertical_C<12, uint16_t>;
  dsp->convolve[0][1][1][1] = ConvolveCompound2D_C<12, uint16_t>;

  dsp->convolve[1][0][0][0] = ConvolveCopy_C<12, uint16_t>;
  dsp->convolve[1][0][0][1] =
      ConvolveIntraBlockCopy1D_C<12, uint16_t, /*is_horizontal=*/true>;
  dsp->convolve[1][0][1][0] =
      ConvolveIntraBlockCopy1D_C<12, uint16_t, /*is_horizontal=*/false>;
  dsp->convolve[1][0][1][1] = ConvolveIntraBlockCopy2D_C<12, uint16_t>;

  dsp->convolve[1][1][0][0] = nullptr;
  dsp->convolve[1][1][0][1] = nullptr;
  dsp->convolve[1][1][1][0] = nullptr;
  dsp->convolve[1][1][1][1] = nullptr;

  dsp->convolve_scale[0] = ConvolveScale2D_C<12, uint16_t>;
  dsp->convolve_scale[1] = ConvolveCompoundScale2D_C<12, uint16_t>;
#else  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
#ifndef LIBGAV1_Dsp12bpp_ConvolveCopy
  dsp->convolve[0][0][0][0] = ConvolveCopy_C<12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_ConvolveHorizontal
  dsp->convolve[0][0][0][1] = ConvolveHorizontal_C<12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_ConvolveVertical
  dsp->convolve[0][0][1][0] = ConvolveVertical_C<12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_Convolve2D
  dsp->convolve[0][0][1][1] = Convolve2D_C<12, uint16_t>;
#endif

#ifndef LIBGAV1_Dsp12bpp_ConvolveCompoundCopy
  dsp->convolve[0][1][0][0] = ConvolveCompoundCopy_C<12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_ConvolveCompoundHorizontal
  dsp->convolve[0][1][0][1] = ConvolveCompoundHorizontal_C<12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_ConvolveCompoundVertical
  dsp->convolve[0][1][1][0] = ConvolveCompoundVertical_C<12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_ConvolveCompound2D
  dsp->convolve[0][1][1][1] = ConvolveCompound2D_C<12, uint16_t>;
#endif

#ifndef LIBGAV1_Dsp12bpp_ConvolveIntraBlockCopy
  dsp->convolve[1][0][0][0] = ConvolveCopy_C<12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_ConvolveIntraBlockHorizontal
  dsp->convolve[1][0][0][1] =
      ConvolveIntraBlockCopy1D_C<12, uint16_t, /*is_horizontal=*/true>;
#endif
#ifndef LIBGAV1_Dsp12bpp_ConvolveIntraBlockVertical
  dsp->convolve[1][0][1][0] =
      ConvolveIntraBlockCopy1D_C<12, uint16_t, /*is_horizontal=*/false>;
#endif
#ifndef LIBGAV1_Dsp12bpp_ConvolveIntraBlock2D
  dsp->convolve[1][0][1][1] = ConvolveIntraBlockCopy2D_C<12, uint16_t>;
#endif

  dsp->convolve[1][1][0][0] = nullptr;
  dsp->convolve[1][1][0][1] = nullptr;
  dsp->convolve[1][1][1][0] = nullptr;
  dsp->convolve[1][1][1][1] = nullptr;

#ifndef LIBGAV1_Dsp12bpp_ConvolveScale2D
  dsp->convolve_scale[0] = ConvolveScale2D_C<12, uint16_t>;
#endif
#ifndef LIBGAV1_Dsp12bpp_ConvolveCompoundScale2D
  dsp->convolve_scale[1] = ConvolveCompoundScale2D_C<12, uint16_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace

void ConvolveInit_C() {
  Init8bpp();
#if LIBGAV1_MAX_BITDEPTH >= 10
  Init10bpp();
#endif
#if LIBGAV1_MAX_BITDEPTH == 12
  Init12bpp();
#endif
}

}  // namespace dsp
}  // namespace libgav1
