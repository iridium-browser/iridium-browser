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

#if LIBGAV1_ENABLE_NEON

#include <arm_neon.h>

#include "src/dsp/arm/common_neon.h"
#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/utils/common.h"
#include "src/utils/compiler_attributes.h"
#include "src/utils/constants.h"

namespace libgav1 {
namespace dsp {

namespace low_bitdepth {
namespace {

void SuperResCoefficients_NEON(const int upscaled_width,
                               const int initial_subpixel_x, const int step,
                               void* const coefficients) {
  auto* dst = static_cast<uint8_t*>(coefficients);
  int subpixel_x = initial_subpixel_x;
  int x = RightShiftWithCeiling(upscaled_width, 3);
  do {
    uint8x8_t filter[8];
    uint8x16_t d[kSuperResFilterTaps / 2];
    for (int i = 0; i < 8; ++i, subpixel_x += step) {
      filter[i] =
          vld1_u8(kUpscaleFilterUnsigned[(subpixel_x & kSuperResScaleMask) >>
                                         kSuperResExtraBits]);
    }
    Transpose8x8(filter, d);
    vst1q_u8(dst, d[0]);
    dst += 16;
    vst1q_u8(dst, d[1]);
    dst += 16;
    vst1q_u8(dst, d[2]);
    dst += 16;
    vst1q_u8(dst, d[3]);
    dst += 16;
  } while (--x != 0);
}

// Maximum sum of positive taps: 171 = 7 + 86 + 71 + 7
// Maximum sum: 255*171 == 0xAA55
// The sum is clipped to [0, 255], so adding all positive and then
// subtracting all negative with saturation is sufficient.
//           0 1 2 3 4 5 6 7
// tap sign: - + - + + - + -
inline uint8x8_t SuperRes(const uint8x8_t src[kSuperResFilterTaps],
                          const uint8_t** coefficients) {
  uint8x16_t f[kSuperResFilterTaps / 2];
  for (int i = 0; i < kSuperResFilterTaps / 2; ++i, *coefficients += 16) {
    f[i] = vld1q_u8(*coefficients);
  }
  uint16x8_t res = vmull_u8(src[1], vget_high_u8(f[0]));
  res = vmlal_u8(res, src[3], vget_high_u8(f[1]));
  res = vmlal_u8(res, src[4], vget_low_u8(f[2]));
  res = vmlal_u8(res, src[6], vget_low_u8(f[3]));
  uint16x8_t temp = vmull_u8(src[0], vget_low_u8(f[0]));
  temp = vmlal_u8(temp, src[2], vget_low_u8(f[1]));
  temp = vmlal_u8(temp, src[5], vget_high_u8(f[2]));
  temp = vmlal_u8(temp, src[7], vget_high_u8(f[3]));
  res = vqsubq_u16(res, temp);
  return vqrshrn_n_u16(res, kFilterBits);
}

void SuperRes_NEON(const void* LIBGAV1_RESTRICT const coefficients,
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
#if LIBGAV1_MSAN
    // Initialize the padding area to prevent msan warnings.
    const int super_res_right_border = kSuperResHorizontalPadding;
#else
    const int super_res_right_border = kSuperResHorizontalBorder;
#endif
    ExtendLine<uint8_t>(src + DivideBy2(kSuperResFilterTaps), downscaled_width,
                        kSuperResHorizontalBorder, super_res_right_border);
    int subpixel_x = initial_subpixel_x;
    uint8x8_t sr[8];
    uint8x16_t s[8];
    int x = RightShiftWithCeiling(upscaled_width, 4);
    // The below code calculates up to 15 extra upscaled
    // pixels which will over-read up to 15 downscaled pixels in the end of each
    // row. kSuperResHorizontalPadding accounts for this.
    do {
      for (int i = 0; i < 8; ++i, subpixel_x += step) {
        sr[i] = vld1_u8(&src[subpixel_x >> kSuperResScaleBits]);
      }
      for (int i = 0; i < 8; ++i, subpixel_x += step) {
        const uint8x8_t s_hi = vld1_u8(&src[subpixel_x >> kSuperResScaleBits]);
        s[i] = vcombine_u8(sr[i], s_hi);
      }
      Transpose8x16(s);
      // Do not use loop for the following 8 instructions, since the compiler
      // will generate redundant code.
      sr[0] = vget_low_u8(s[0]);
      sr[1] = vget_low_u8(s[1]);
      sr[2] = vget_low_u8(s[2]);
      sr[3] = vget_low_u8(s[3]);
      sr[4] = vget_low_u8(s[4]);
      sr[5] = vget_low_u8(s[5]);
      sr[6] = vget_low_u8(s[6]);
      sr[7] = vget_low_u8(s[7]);
      const uint8x8_t d0 = SuperRes(sr, &filter);
      // Do not use loop for the following 8 instructions, since the compiler
      // will generate redundant code.
      sr[0] = vget_high_u8(s[0]);
      sr[1] = vget_high_u8(s[1]);
      sr[2] = vget_high_u8(s[2]);
      sr[3] = vget_high_u8(s[3]);
      sr[4] = vget_high_u8(s[4]);
      sr[5] = vget_high_u8(s[5]);
      sr[6] = vget_high_u8(s[6]);
      sr[7] = vget_high_u8(s[7]);
      const uint8x8_t d1 = SuperRes(sr, &filter);
      vst1q_u8(dst_ptr, vcombine_u8(d0, d1));
      dst_ptr += 16;
    } while (--x != 0);
    src += source_stride;
    dst += dest_stride;
  } while (--y != 0);
}

void Init8bpp() {
  Dsp* dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  dsp->super_res_coefficients = SuperResCoefficients_NEON;
  dsp->super_res = SuperRes_NEON;
}

}  // namespace
}  // namespace low_bitdepth

//------------------------------------------------------------------------------
#if LIBGAV1_MAX_BITDEPTH >= 10
namespace high_bitdepth {
namespace {

void SuperResCoefficients_NEON(const int upscaled_width,
                               const int initial_subpixel_x, const int step,
                               void* const coefficients) {
  auto* dst = static_cast<uint16_t*>(coefficients);
  int subpixel_x = initial_subpixel_x;
  int x = RightShiftWithCeiling(upscaled_width, 3);
  do {
    uint16x8_t filter[8];
    for (int i = 0; i < 8; ++i, subpixel_x += step) {
      const uint8x8_t filter_8 =
          vld1_u8(kUpscaleFilterUnsigned[(subpixel_x & kSuperResScaleMask) >>
                                         kSuperResExtraBits]);
      // uint8_t -> uint16_t
      filter[i] = vmovl_u8(filter_8);
    }

    Transpose8x8(filter);

    vst1q_u16(dst, filter[0]);
    dst += 8;
    vst1q_u16(dst, filter[1]);
    dst += 8;
    vst1q_u16(dst, filter[2]);
    dst += 8;
    vst1q_u16(dst, filter[3]);
    dst += 8;
    vst1q_u16(dst, filter[4]);
    dst += 8;
    vst1q_u16(dst, filter[5]);
    dst += 8;
    vst1q_u16(dst, filter[6]);
    dst += 8;
    vst1q_u16(dst, filter[7]);
    dst += 8;
  } while (--x != 0);
}

// The sum is clipped to [0, ((1 << bitdepth) -1)]. Adding all positive and then
// subtracting all negative with saturation will clip to zero.
//           0 1 2 3 4 5 6 7
// tap sign: - + - + + - + -
inline uint16x8_t SuperRes(const uint16x8_t src[kSuperResFilterTaps],
                           const uint16_t** coefficients, int bitdepth) {
  uint16x8_t f[kSuperResFilterTaps];
  for (int i = 0; i < kSuperResFilterTaps; ++i, *coefficients += 8) {
    f[i] = vld1q_u16(*coefficients);
  }

  uint32x4_t res_lo = vmull_u16(vget_low_u16(src[1]), vget_low_u16(f[1]));
  res_lo = vmlal_u16(res_lo, vget_low_u16(src[3]), vget_low_u16(f[3]));
  res_lo = vmlal_u16(res_lo, vget_low_u16(src[4]), vget_low_u16(f[4]));
  res_lo = vmlal_u16(res_lo, vget_low_u16(src[6]), vget_low_u16(f[6]));

  uint32x4_t temp_lo = vmull_u16(vget_low_u16(src[0]), vget_low_u16(f[0]));
  temp_lo = vmlal_u16(temp_lo, vget_low_u16(src[2]), vget_low_u16(f[2]));
  temp_lo = vmlal_u16(temp_lo, vget_low_u16(src[5]), vget_low_u16(f[5]));
  temp_lo = vmlal_u16(temp_lo, vget_low_u16(src[7]), vget_low_u16(f[7]));

  res_lo = vqsubq_u32(res_lo, temp_lo);

  uint32x4_t res_hi = vmull_u16(vget_high_u16(src[1]), vget_high_u16(f[1]));
  res_hi = vmlal_u16(res_hi, vget_high_u16(src[3]), vget_high_u16(f[3]));
  res_hi = vmlal_u16(res_hi, vget_high_u16(src[4]), vget_high_u16(f[4]));
  res_hi = vmlal_u16(res_hi, vget_high_u16(src[6]), vget_high_u16(f[6]));

  uint32x4_t temp_hi = vmull_u16(vget_high_u16(src[0]), vget_high_u16(f[0]));
  temp_hi = vmlal_u16(temp_hi, vget_high_u16(src[2]), vget_high_u16(f[2]));
  temp_hi = vmlal_u16(temp_hi, vget_high_u16(src[5]), vget_high_u16(f[5]));
  temp_hi = vmlal_u16(temp_hi, vget_high_u16(src[7]), vget_high_u16(f[7]));

  res_hi = vqsubq_u32(res_hi, temp_hi);

  const uint16x8_t res = vcombine_u16(vqrshrn_n_u32(res_lo, kFilterBits),
                                      vqrshrn_n_u32(res_hi, kFilterBits));

  // Clip the result at (1 << bd) - 1.
  return vminq_u16(res, vdupq_n_u16((1 << bitdepth) - 1));
}

template <int bitdepth>
void SuperRes_NEON(const void* LIBGAV1_RESTRICT const coefficients,
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
#if LIBGAV1_MSAN
    // Initialize the padding area to prevent msan warnings.
    const int super_res_right_border = kSuperResHorizontalPadding;
#else
    const int super_res_right_border = kSuperResHorizontalBorder;
#endif
    ExtendLine<uint16_t>(src + DivideBy2(kSuperResFilterTaps), downscaled_width,
                         kSuperResHorizontalBorder, super_res_right_border);
    int subpixel_x = initial_subpixel_x;
    uint16x8_t sr[8];
    int x = RightShiftWithCeiling(upscaled_width, 3);
    // The below code calculates up to 7 extra upscaled
    // pixels which will over-read up to 7 downscaled pixels in the end of each
    // row. kSuperResHorizontalBorder accounts for this.
    do {
      for (int i = 0; i < 8; ++i, subpixel_x += step) {
        sr[i] = vld1q_u16(&src[subpixel_x >> kSuperResScaleBits]);
      }

      Transpose8x8(sr);

      const uint16x8_t d0 = SuperRes(sr, &filter, bitdepth);
      vst1q_u16(dst_ptr, d0);
      dst_ptr += 8;
    } while (--x != 0);
    src += source_stride;
    dst += dest_stride;
  } while (--y != 0);
}

void Init10bpp() {
  Dsp* dsp = dsp_internal::GetWritableDspTable(kBitdepth10);
  assert(dsp != nullptr);
  dsp->super_res_coefficients = SuperResCoefficients_NEON;
  dsp->super_res = SuperRes_NEON<10>;
}

}  // namespace
}  // namespace high_bitdepth
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

void SuperResInit_NEON() {
  low_bitdepth::Init8bpp();
#if LIBGAV1_MAX_BITDEPTH >= 10
  high_bitdepth::Init10bpp();
#endif
}
}  // namespace dsp
}  // namespace libgav1

#else   // !LIBGAV1_ENABLE_NEON

namespace libgav1 {
namespace dsp {

void SuperResInit_NEON() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_ENABLE_NEON
