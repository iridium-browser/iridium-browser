// Copyright 2020 The libgav1 Authors
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

#include "src/dsp/super_res.h"
#include "src/utils/cpu.h"

#if LIBGAV1_TARGETING_SSE4_1

#include <smmintrin.h>

#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/dsp/x86/common_sse4.h"
#include "src/dsp/x86/transpose_sse4.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"

namespace libgav1 {
namespace dsp {
namespace low_bitdepth {
namespace {

// Upscale_Filter as defined in AV1 Section 7.16
// Negative to make them fit in 8-bit.
alignas(16) const int8_t
    kNegativeUpscaleFilter[kSuperResFilterShifts][kSuperResFilterTaps] = {
        {0, 0, 0, -128, 0, 0, 0, 0},       {0, 0, 1, -128, -2, 1, 0, 0},
        {0, -1, 3, -127, -4, 2, -1, 0},    {0, -1, 4, -127, -6, 3, -1, 0},
        {0, -2, 6, -126, -8, 3, -1, 0},    {0, -2, 7, -125, -11, 4, -1, 0},
        {1, -2, 8, -125, -13, 5, -2, 0},   {1, -3, 9, -124, -15, 6, -2, 0},
        {1, -3, 10, -123, -18, 6, -2, 1},  {1, -3, 11, -122, -20, 7, -3, 1},
        {1, -4, 12, -121, -22, 8, -3, 1},  {1, -4, 13, -120, -25, 9, -3, 1},
        {1, -4, 14, -118, -28, 9, -3, 1},  {1, -4, 15, -117, -30, 10, -4, 1},
        {1, -5, 16, -116, -32, 11, -4, 1}, {1, -5, 16, -114, -35, 12, -4, 1},
        {1, -5, 17, -112, -38, 12, -4, 1}, {1, -5, 18, -111, -40, 13, -5, 1},
        {1, -5, 18, -109, -43, 14, -5, 1}, {1, -6, 19, -107, -45, 14, -5, 1},
        {1, -6, 19, -105, -48, 15, -5, 1}, {1, -6, 19, -103, -51, 16, -5, 1},
        {1, -6, 20, -101, -53, 16, -6, 1}, {1, -6, 20, -99, -56, 17, -6, 1},
        {1, -6, 20, -97, -58, 17, -6, 1},  {1, -6, 20, -95, -61, 18, -6, 1},
        {2, -7, 20, -93, -64, 18, -6, 2},  {2, -7, 20, -91, -66, 19, -6, 1},
        {2, -7, 20, -88, -69, 19, -6, 1},  {2, -7, 20, -86, -71, 19, -6, 1},
        {2, -7, 20, -84, -74, 20, -7, 2},  {2, -7, 20, -81, -76, 20, -7, 1},
        {2, -7, 20, -79, -79, 20, -7, 2},  {1, -7, 20, -76, -81, 20, -7, 2},
        {2, -7, 20, -74, -84, 20, -7, 2},  {1, -6, 19, -71, -86, 20, -7, 2},
        {1, -6, 19, -69, -88, 20, -7, 2},  {1, -6, 19, -66, -91, 20, -7, 2},
        {2, -6, 18, -64, -93, 20, -7, 2},  {1, -6, 18, -61, -95, 20, -6, 1},
        {1, -6, 17, -58, -97, 20, -6, 1},  {1, -6, 17, -56, -99, 20, -6, 1},
        {1, -6, 16, -53, -101, 20, -6, 1}, {1, -5, 16, -51, -103, 19, -6, 1},
        {1, -5, 15, -48, -105, 19, -6, 1}, {1, -5, 14, -45, -107, 19, -6, 1},
        {1, -5, 14, -43, -109, 18, -5, 1}, {1, -5, 13, -40, -111, 18, -5, 1},
        {1, -4, 12, -38, -112, 17, -5, 1}, {1, -4, 12, -35, -114, 16, -5, 1},
        {1, -4, 11, -32, -116, 16, -5, 1}, {1, -4, 10, -30, -117, 15, -4, 1},
        {1, -3, 9, -28, -118, 14, -4, 1},  {1, -3, 9, -25, -120, 13, -4, 1},
        {1, -3, 8, -22, -121, 12, -4, 1},  {1, -3, 7, -20, -122, 11, -3, 1},
        {1, -2, 6, -18, -123, 10, -3, 1},  {0, -2, 6, -15, -124, 9, -3, 1},
        {0, -2, 5, -13, -125, 8, -2, 1},   {0, -1, 4, -11, -125, 7, -2, 0},
        {0, -1, 3, -8, -126, 6, -2, 0},    {0, -1, 3, -6, -127, 4, -1, 0},
        {0, -1, 2, -4, -127, 3, -1, 0},    {0, 0, 1, -2, -128, 1, 0, 0},
};

void SuperResCoefficients_SSE4_1(const int upscaled_width,
                                 const int initial_subpixel_x, const int step,
                                 void* const coefficients) {
  auto* dst = static_cast<uint8_t*>(coefficients);
  int subpixel_x = initial_subpixel_x;
  int x = RightShiftWithCeiling(upscaled_width, 4);
  do {
    for (int i = 0; i < 8; ++i, dst += 16) {
      int remainder = subpixel_x & kSuperResScaleMask;
      __m128i filter =
          LoadLo8(kNegativeUpscaleFilter[remainder >> kSuperResExtraBits]);
      subpixel_x += step;
      remainder = subpixel_x & kSuperResScaleMask;
      filter = LoadHi8(filter,
                       kNegativeUpscaleFilter[remainder >> kSuperResExtraBits]);
      subpixel_x += step;
      StoreAligned16(dst, filter);
    }
  } while (--x != 0);
}

void SuperRes_SSE4_1(const void* LIBGAV1_RESTRICT const coefficients,
                     void* LIBGAV1_RESTRICT const source,
                     const ptrdiff_t source_stride, const int height,
                     const int downscaled_width, const int upscaled_width,
                     const int initial_subpixel_x, const int step,
                     void* LIBGAV1_RESTRICT const dest,
                     const ptrdiff_t dest_stride) {
  auto* src = static_cast<uint8_t*>(source) - DivideBy2(kSuperResFilterTaps);
  auto* dst = static_cast<uint8_t*>(dest);
  int y = height;
  do {
    const auto* filter = static_cast<const uint8_t*>(coefficients);
    uint8_t* dst_ptr = dst;
    ExtendLine<uint8_t>(src + DivideBy2(kSuperResFilterTaps), downscaled_width,
                        kSuperResHorizontalBorder, kSuperResHorizontalBorder);
    int subpixel_x = initial_subpixel_x;
    // The below code calculates up to 15 extra upscaled pixels which will
    // over-read up to 15 downscaled pixels in the end of each row.
    // kSuperResHorizontalPadding protects this behavior from segmentation
    // faults and threading issues.
    int x = RightShiftWithCeiling(upscaled_width, 4);
    do {
      __m128i weighted_src[8];
      for (int i = 0; i < 8; ++i, filter += 16) {
        // TODO(b/178652672): Remove Msan loads when hadd bug is resolved.
        // It's fine to write uninitialized bytes outside the frame, but the
        // inside-frame pixels are incorrectly labeled uninitialized if
        // uninitialized values go through the hadd intrinsics.
        // |src| is offset 4 pixels to the left, and there are 4 extended border
        // pixels, so a difference of 0 from |downscaled_width| indicates 8 good
        // bytes. A difference of 1 indicates 7 good bytes.
        const int msan_bytes_lo =
            (subpixel_x >> kSuperResScaleBits) - downscaled_width;
        __m128i s =
            LoadLo8Msan(&src[subpixel_x >> kSuperResScaleBits], msan_bytes_lo);
        subpixel_x += step;
        const int msan_bytes_hi =
            (subpixel_x >> kSuperResScaleBits) - downscaled_width;
        s = LoadHi8Msan(s, &src[subpixel_x >> kSuperResScaleBits],
                        msan_bytes_hi);
        subpixel_x += step;
        const __m128i f = LoadAligned16(filter);
        weighted_src[i] = _mm_maddubs_epi16(s, f);
      }

      __m128i a[4];
      a[0] = _mm_hadd_epi16(weighted_src[0], weighted_src[1]);
      a[1] = _mm_hadd_epi16(weighted_src[2], weighted_src[3]);
      a[2] = _mm_hadd_epi16(weighted_src[4], weighted_src[5]);
      a[3] = _mm_hadd_epi16(weighted_src[6], weighted_src[7]);
      Transpose2x16_U16(a, a);
      a[0] = _mm_adds_epi16(a[0], a[1]);
      a[1] = _mm_adds_epi16(a[2], a[3]);
      const __m128i rounding = _mm_set1_epi16(1 << (kFilterBits - 1));
      a[0] = _mm_subs_epi16(rounding, a[0]);
      a[1] = _mm_subs_epi16(rounding, a[1]);
      a[0] = _mm_srai_epi16(a[0], kFilterBits);
      a[1] = _mm_srai_epi16(a[1], kFilterBits);
      StoreAligned16(dst_ptr, _mm_packus_epi16(a[0], a[1]));
      dst_ptr += 16;
    } while (--x != 0);
    src += source_stride;
    dst += dest_stride;
  } while (--y != 0);
}

void Init8bpp() {
  Dsp* dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
#if DSP_ENABLED_8BPP_SSE4_1(SuperResCoefficients)
  dsp->super_res_coefficients = SuperResCoefficients_SSE4_1;
#endif  // DSP_ENABLED_8BPP_SSE4_1(SuperResCoefficients)
#if DSP_ENABLED_8BPP_SSE4_1(SuperRes)
  dsp->super_res = SuperRes_SSE4_1;
#endif  // DSP_ENABLED_8BPP_SSE4_1(SuperRes)
}

}  // namespace
}  // namespace low_bitdepth

//------------------------------------------------------------------------------
#if LIBGAV1_MAX_BITDEPTH >= 10
namespace high_bitdepth {
namespace {

// Upscale_Filter as defined in AV1 Section 7.16
alignas(16) const int16_t
    kUpscaleFilter[kSuperResFilterShifts][kSuperResFilterTaps] = {
        {0, 0, 0, 128, 0, 0, 0, 0},        {0, 0, -1, 128, 2, -1, 0, 0},
        {0, 1, -3, 127, 4, -2, 1, 0},      {0, 1, -4, 127, 6, -3, 1, 0},
        {0, 2, -6, 126, 8, -3, 1, 0},      {0, 2, -7, 125, 11, -4, 1, 0},
        {-1, 2, -8, 125, 13, -5, 2, 0},    {-1, 3, -9, 124, 15, -6, 2, 0},
        {-1, 3, -10, 123, 18, -6, 2, -1},  {-1, 3, -11, 122, 20, -7, 3, -1},
        {-1, 4, -12, 121, 22, -8, 3, -1},  {-1, 4, -13, 120, 25, -9, 3, -1},
        {-1, 4, -14, 118, 28, -9, 3, -1},  {-1, 4, -15, 117, 30, -10, 4, -1},
        {-1, 5, -16, 116, 32, -11, 4, -1}, {-1, 5, -16, 114, 35, -12, 4, -1},
        {-1, 5, -17, 112, 38, -12, 4, -1}, {-1, 5, -18, 111, 40, -13, 5, -1},
        {-1, 5, -18, 109, 43, -14, 5, -1}, {-1, 6, -19, 107, 45, -14, 5, -1},
        {-1, 6, -19, 105, 48, -15, 5, -1}, {-1, 6, -19, 103, 51, -16, 5, -1},
        {-1, 6, -20, 101, 53, -16, 6, -1}, {-1, 6, -20, 99, 56, -17, 6, -1},
        {-1, 6, -20, 97, 58, -17, 6, -1},  {-1, 6, -20, 95, 61, -18, 6, -1},
        {-2, 7, -20, 93, 64, -18, 6, -2},  {-2, 7, -20, 91, 66, -19, 6, -1},
        {-2, 7, -20, 88, 69, -19, 6, -1},  {-2, 7, -20, 86, 71, -19, 6, -1},
        {-2, 7, -20, 84, 74, -20, 7, -2},  {-2, 7, -20, 81, 76, -20, 7, -1},
        {-2, 7, -20, 79, 79, -20, 7, -2},  {-1, 7, -20, 76, 81, -20, 7, -2},
        {-2, 7, -20, 74, 84, -20, 7, -2},  {-1, 6, -19, 71, 86, -20, 7, -2},
        {-1, 6, -19, 69, 88, -20, 7, -2},  {-1, 6, -19, 66, 91, -20, 7, -2},
        {-2, 6, -18, 64, 93, -20, 7, -2},  {-1, 6, -18, 61, 95, -20, 6, -1},
        {-1, 6, -17, 58, 97, -20, 6, -1},  {-1, 6, -17, 56, 99, -20, 6, -1},
        {-1, 6, -16, 53, 101, -20, 6, -1}, {-1, 5, -16, 51, 103, -19, 6, -1},
        {-1, 5, -15, 48, 105, -19, 6, -1}, {-1, 5, -14, 45, 107, -19, 6, -1},
        {-1, 5, -14, 43, 109, -18, 5, -1}, {-1, 5, -13, 40, 111, -18, 5, -1},
        {-1, 4, -12, 38, 112, -17, 5, -1}, {-1, 4, -12, 35, 114, -16, 5, -1},
        {-1, 4, -11, 32, 116, -16, 5, -1}, {-1, 4, -10, 30, 117, -15, 4, -1},
        {-1, 3, -9, 28, 118, -14, 4, -1},  {-1, 3, -9, 25, 120, -13, 4, -1},
        {-1, 3, -8, 22, 121, -12, 4, -1},  {-1, 3, -7, 20, 122, -11, 3, -1},
        {-1, 2, -6, 18, 123, -10, 3, -1},  {0, 2, -6, 15, 124, -9, 3, -1},
        {0, 2, -5, 13, 125, -8, 2, -1},    {0, 1, -4, 11, 125, -7, 2, 0},
        {0, 1, -3, 8, 126, -6, 2, 0},      {0, 1, -3, 6, 127, -4, 1, 0},
        {0, 1, -2, 4, 127, -3, 1, 0},      {0, 0, -1, 2, 128, -1, 0, 0},
};

void SuperResCoefficients_SSE4_1(const int upscaled_width,
                                 const int initial_subpixel_x, const int step,
                                 void* const coefficients) {
  auto* dst = static_cast<uint16_t*>(coefficients);
  int subpixel_x = initial_subpixel_x;
  int x = RightShiftWithCeiling(upscaled_width, 3);
  do {
    for (int i = 0; i < 8; ++i, dst += 8) {
      int remainder = subpixel_x & kSuperResScaleMask;
      __m128i filter =
          LoadAligned16(kUpscaleFilter[remainder >> kSuperResExtraBits]);
      subpixel_x += step;
      StoreAligned16(dst, filter);
    }
  } while (--x != 0);
}

template <int bitdepth>
void SuperRes_SSE4_1(const void* LIBGAV1_RESTRICT const coefficients,
                     void* LIBGAV1_RESTRICT const source,
                     const ptrdiff_t source_stride, const int height,
                     const int downscaled_width, const int upscaled_width,
                     const int initial_subpixel_x, const int step,
                     void* LIBGAV1_RESTRICT const dest,
                     const ptrdiff_t dest_stride) {
  auto* src = static_cast<uint16_t*>(source) - DivideBy2(kSuperResFilterTaps);
  auto* dst = static_cast<uint16_t*>(dest);
  int y = height;
  do {
    const auto* filter = static_cast<const uint16_t*>(coefficients);
    uint16_t* dst_ptr = dst;
    ExtendLine<uint16_t>(src + DivideBy2(kSuperResFilterTaps), downscaled_width,
                         kSuperResHorizontalBorder, kSuperResHorizontalPadding);
    int subpixel_x = initial_subpixel_x;
    // The below code calculates up to 7 extra upscaled
    // pixels which will over-read up to 7 downscaled pixels in the end of each
    // row. kSuperResHorizontalPadding accounts for this.
    int x = RightShiftWithCeiling(upscaled_width, 3);
    do {
      __m128i weighted_src[8];
      for (int i = 0; i < 8; ++i, filter += 8) {
        const __m128i s =
            LoadUnaligned16(&src[subpixel_x >> kSuperResScaleBits]);
        subpixel_x += step;
        const __m128i f = LoadAligned16(filter);
        weighted_src[i] = _mm_madd_epi16(s, f);
      }

      __m128i a[4];
      a[0] = _mm_hadd_epi32(weighted_src[0], weighted_src[1]);
      a[1] = _mm_hadd_epi32(weighted_src[2], weighted_src[3]);
      a[2] = _mm_hadd_epi32(weighted_src[4], weighted_src[5]);
      a[3] = _mm_hadd_epi32(weighted_src[6], weighted_src[7]);

      a[0] = _mm_hadd_epi32(a[0], a[1]);
      a[1] = _mm_hadd_epi32(a[2], a[3]);
      a[0] = RightShiftWithRounding_S32(a[0], kFilterBits);
      a[1] = RightShiftWithRounding_S32(a[1], kFilterBits);

      // Clip the values at (1 << bd) - 1
      const __m128i clipped_16 = _mm_min_epi16(
          _mm_packus_epi32(a[0], a[1]), _mm_set1_epi16((1 << bitdepth) - 1));
      StoreAligned16(dst_ptr, clipped_16);
      dst_ptr += 8;
    } while (--x != 0);
    src += source_stride;
    dst += dest_stride;
  } while (--y != 0);
}

void Init10bpp() {
  Dsp* dsp = dsp_internal::GetWritableDspTable(kBitdepth10);
  assert(dsp != nullptr);
  static_cast<void>(dsp);
#if DSP_ENABLED_10BPP_SSE4_1(SuperResCoefficients)
  dsp->super_res_coefficients = SuperResCoefficients_SSE4_1;
#else
  static_cast<void>(SuperResCoefficients_SSE4_1);
#endif
#if DSP_ENABLED_10BPP_SSE4_1(SuperRes)
  dsp->super_res = SuperRes_SSE4_1<10>;
#else
  static_cast<void>(SuperRes_SSE4_1);
#endif
}

}  // namespace
}  // namespace high_bitdepth
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

void SuperResInit_SSE4_1() {
  low_bitdepth::Init8bpp();
#if LIBGAV1_MAX_BITDEPTH >= 10
  high_bitdepth::Init10bpp();
#endif
}

}  // namespace dsp
}  // namespace libgav1

#else   // !LIBGAV1_TARGETING_SSE4_1

namespace libgav1 {
namespace dsp {

void SuperResInit_SSE4_1() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_TARGETING_SSE4_1
