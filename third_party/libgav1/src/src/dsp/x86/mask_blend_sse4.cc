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

#include "src/dsp/mask_blend.h"
#include "src/utils/cpu.h"

#if LIBGAV1_TARGETING_SSE4_1

#include <smmintrin.h>

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/dsp/x86/common_sse4.h"
#include "src/utils/common.h"

namespace libgav1 {
namespace dsp {
namespace {

template <int subsampling_x, int subsampling_y>
inline __m128i GetMask8(const uint8_t* mask, const ptrdiff_t stride) {
  if (subsampling_x == 1 && subsampling_y == 1) {
    const __m128i one = _mm_set1_epi8(1);
    const __m128i mask_val_0 = LoadUnaligned16(mask);
    const __m128i mask_val_1 = LoadUnaligned16(mask + stride);
    const __m128i add_0 = _mm_adds_epu8(mask_val_0, mask_val_1);
    const __m128i mask_0 = _mm_maddubs_epi16(add_0, one);
    return RightShiftWithRounding_U16(mask_0, 2);
  }
  if (subsampling_x == 1) {
    const __m128i row_vals = LoadUnaligned16(mask);
    const __m128i mask_val_0 = _mm_cvtepu8_epi16(row_vals);
    const __m128i mask_val_1 = _mm_cvtepu8_epi16(_mm_srli_si128(row_vals, 8));
    __m128i subsampled_mask = _mm_hadd_epi16(mask_val_0, mask_val_1);
    return RightShiftWithRounding_U16(subsampled_mask, 1);
  }
  assert(subsampling_y == 0 && subsampling_x == 0);
  const __m128i mask_val = LoadLo8(mask);
  return _mm_cvtepu8_epi16(mask_val);
}

// Imitate behavior of ARM vtrn1q_u64.
inline __m128i Transpose1_U64(const __m128i a, const __m128i b) {
  return _mm_castps_si128(
      _mm_movelh_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b)));
}

// Imitate behavior of ARM vtrn2q_u64.
inline __m128i Transpose2_U64(const __m128i a, const __m128i b) {
  return _mm_castps_si128(
      _mm_movehl_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b)));
}

// Width can only be 4 when it is subsampled from a block of width 8, hence
// subsampling_x is always 1 when this function is called.
template <int subsampling_x, int subsampling_y>
inline __m128i GetMask4x2(const uint8_t* mask) {
  if (subsampling_x == 1 && subsampling_y == 1) {
    const __m128i mask_val_01 = LoadUnaligned16(mask);
    // Stride is fixed because this is the smallest block size.
    const __m128i mask_val_23 = LoadUnaligned16(mask + 16);
    // Transpose rows to add row 0 to row 1, and row 2 to row 3.
    const __m128i mask_val_02 = Transpose1_U64(mask_val_01, mask_val_23);
    const __m128i mask_val_13 = Transpose2_U64(mask_val_23, mask_val_01);
    const __m128i add_0 = _mm_adds_epu8(mask_val_02, mask_val_13);
    const __m128i one = _mm_set1_epi8(1);
    const __m128i mask_0 = _mm_maddubs_epi16(add_0, one);
    return RightShiftWithRounding_U16(mask_0, 2);
  }
  return GetMask8<subsampling_x, 0>(mask, 0);
}

template <int subsampling_x, int subsampling_y>
inline __m128i GetInterIntraMask4x2(const uint8_t* mask,
                                    ptrdiff_t mask_stride) {
  if (subsampling_x == 1) {
    return GetMask4x2<subsampling_x, subsampling_y>(mask);
  }
  // When using intra or difference weighted masks, the function doesn't use
  // subsampling, so |mask_stride| may be 4 or 8.
  assert(subsampling_y == 0 && subsampling_x == 0);
  const __m128i mask_val_0 = Load4(mask);
  const __m128i mask_val_1 = Load4(mask + mask_stride);
  return _mm_cvtepu8_epi16(
      _mm_or_si128(mask_val_0, _mm_slli_si128(mask_val_1, 4)));
}

}  // namespace

namespace low_bitdepth {
namespace {

// This function returns a 16-bit packed mask to fit in _mm_madd_epi16.
// 16-bit is also the lowest packing for hadd, but without subsampling there is
// an unfortunate conversion required.
template <int subsampling_x, int subsampling_y>
inline __m128i GetMask8(const uint8_t* LIBGAV1_RESTRICT mask,
                        ptrdiff_t stride) {
  if (subsampling_x == 1) {
    const __m128i row_vals = LoadUnaligned16(mask);

    const __m128i mask_val_0 = _mm_cvtepu8_epi16(row_vals);
    const __m128i mask_val_1 = _mm_cvtepu8_epi16(_mm_srli_si128(row_vals, 8));
    __m128i subsampled_mask = _mm_hadd_epi16(mask_val_0, mask_val_1);

    if (subsampling_y == 1) {
      const __m128i next_row_vals = LoadUnaligned16(mask + stride);
      const __m128i next_mask_val_0 = _mm_cvtepu8_epi16(next_row_vals);
      const __m128i next_mask_val_1 =
          _mm_cvtepu8_epi16(_mm_srli_si128(next_row_vals, 8));
      subsampled_mask = _mm_add_epi16(
          subsampled_mask, _mm_hadd_epi16(next_mask_val_0, next_mask_val_1));
    }
    return RightShiftWithRounding_U16(subsampled_mask, 1 + subsampling_y);
  }
  assert(subsampling_y == 0 && subsampling_x == 0);
  const __m128i mask_val = LoadLo8(mask);
  return _mm_cvtepu8_epi16(mask_val);
}

inline void WriteMaskBlendLine4x2(const int16_t* LIBGAV1_RESTRICT const pred_0,
                                  const int16_t* LIBGAV1_RESTRICT const pred_1,
                                  const __m128i pred_mask_0,
                                  const __m128i pred_mask_1,
                                  uint8_t* LIBGAV1_RESTRICT dst,
                                  const ptrdiff_t dst_stride) {
  const __m128i pred_val_0 = LoadAligned16(pred_0);
  const __m128i pred_val_1 = LoadAligned16(pred_1);
  const __m128i mask_lo = _mm_unpacklo_epi16(pred_mask_0, pred_mask_1);
  const __m128i mask_hi = _mm_unpackhi_epi16(pred_mask_0, pred_mask_1);
  const __m128i pred_lo = _mm_unpacklo_epi16(pred_val_0, pred_val_1);
  const __m128i pred_hi = _mm_unpackhi_epi16(pred_val_0, pred_val_1);

  // int res = (mask_value * prediction_0[x] +
  //      (64 - mask_value) * prediction_1[x]) >> 6;
  const __m128i compound_pred_lo = _mm_madd_epi16(pred_lo, mask_lo);
  const __m128i compound_pred_hi = _mm_madd_epi16(pred_hi, mask_hi);
  const __m128i compound_pred = _mm_packus_epi32(
      _mm_srli_epi32(compound_pred_lo, 6), _mm_srli_epi32(compound_pred_hi, 6));

  // dst[x] = static_cast<Pixel>(
  //     Clip3(RightShiftWithRounding(res, inter_post_round_bits), 0,
  //           (1 << kBitdepth8) - 1));
  const __m128i result = RightShiftWithRounding_S16(compound_pred, 4);
  const __m128i res = _mm_packus_epi16(result, result);
  Store4(dst, res);
  Store4(dst + dst_stride, _mm_srli_si128(res, 4));
}

template <int subsampling_x, int subsampling_y>
inline void MaskBlending4x4_SSE4_1(const int16_t* LIBGAV1_RESTRICT pred_0,
                                   const int16_t* LIBGAV1_RESTRICT pred_1,
                                   const uint8_t* LIBGAV1_RESTRICT mask,
                                   uint8_t* LIBGAV1_RESTRICT dst,
                                   const ptrdiff_t dst_stride) {
  constexpr ptrdiff_t mask_stride = 4 << subsampling_x;
  const __m128i mask_inverter = _mm_set1_epi16(64);
  __m128i pred_mask_0 = GetMask4x2<subsampling_x, subsampling_y>(mask);
  __m128i pred_mask_1 = _mm_sub_epi16(mask_inverter, pred_mask_0);
  WriteMaskBlendLine4x2(pred_0, pred_1, pred_mask_0, pred_mask_1, dst,
                        dst_stride);
  pred_0 += 4 << 1;
  pred_1 += 4 << 1;
  mask += mask_stride << (1 + subsampling_y);
  dst += dst_stride << 1;

  pred_mask_0 = GetMask4x2<subsampling_x, subsampling_y>(mask);
  pred_mask_1 = _mm_sub_epi16(mask_inverter, pred_mask_0);
  WriteMaskBlendLine4x2(pred_0, pred_1, pred_mask_0, pred_mask_1, dst,
                        dst_stride);
}

template <int subsampling_x, int subsampling_y>
inline void MaskBlending4xH_SSE4_1(
    const int16_t* LIBGAV1_RESTRICT pred_0,
    const int16_t* LIBGAV1_RESTRICT pred_1,
    const uint8_t* LIBGAV1_RESTRICT const mask_ptr, const int height,
    uint8_t* LIBGAV1_RESTRICT dst, const ptrdiff_t dst_stride) {
  assert(subsampling_x == 1);
  const uint8_t* mask = mask_ptr;
  constexpr ptrdiff_t mask_stride = 4 << subsampling_x;
  if (height == 4) {
    MaskBlending4x4_SSE4_1<subsampling_x, subsampling_y>(pred_0, pred_1, mask,
                                                         dst, dst_stride);
    return;
  }
  const __m128i mask_inverter = _mm_set1_epi16(64);
  int y = 0;
  do {
    __m128i pred_mask_0 = GetMask4x2<subsampling_x, subsampling_y>(mask);
    __m128i pred_mask_1 = _mm_sub_epi16(mask_inverter, pred_mask_0);

    WriteMaskBlendLine4x2(pred_0, pred_1, pred_mask_0, pred_mask_1, dst,
                          dst_stride);
    pred_0 += 4 << 1;
    pred_1 += 4 << 1;
    mask += mask_stride << (1 + subsampling_y);
    dst += dst_stride << 1;

    pred_mask_0 = GetMask4x2<subsampling_x, subsampling_y>(mask);
    pred_mask_1 = _mm_sub_epi16(mask_inverter, pred_mask_0);
    WriteMaskBlendLine4x2(pred_0, pred_1, pred_mask_0, pred_mask_1, dst,
                          dst_stride);
    pred_0 += 4 << 1;
    pred_1 += 4 << 1;
    mask += mask_stride << (1 + subsampling_y);
    dst += dst_stride << 1;

    pred_mask_0 = GetMask4x2<subsampling_x, subsampling_y>(mask);
    pred_mask_1 = _mm_sub_epi16(mask_inverter, pred_mask_0);
    WriteMaskBlendLine4x2(pred_0, pred_1, pred_mask_0, pred_mask_1, dst,
                          dst_stride);
    pred_0 += 4 << 1;
    pred_1 += 4 << 1;
    mask += mask_stride << (1 + subsampling_y);
    dst += dst_stride << 1;

    pred_mask_0 = GetMask4x2<subsampling_x, subsampling_y>(mask);
    pred_mask_1 = _mm_sub_epi16(mask_inverter, pred_mask_0);
    WriteMaskBlendLine4x2(pred_0, pred_1, pred_mask_0, pred_mask_1, dst,
                          dst_stride);
    pred_0 += 4 << 1;
    pred_1 += 4 << 1;
    mask += mask_stride << (1 + subsampling_y);
    dst += dst_stride << 1;
    y += 8;
  } while (y < height);
}

template <int subsampling_x, int subsampling_y>
inline void MaskBlend_SSE4_1(const void* LIBGAV1_RESTRICT prediction_0,
                             const void* LIBGAV1_RESTRICT prediction_1,
                             const ptrdiff_t /*prediction_stride_1*/,
                             const uint8_t* LIBGAV1_RESTRICT const mask_ptr,
                             const ptrdiff_t mask_stride, const int width,
                             const int height, void* LIBGAV1_RESTRICT dest,
                             const ptrdiff_t dst_stride) {
  auto* dst = static_cast<uint8_t*>(dest);
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  const ptrdiff_t pred_stride_0 = width;
  const ptrdiff_t pred_stride_1 = width;
  if (width == 4) {
    MaskBlending4xH_SSE4_1<subsampling_x, subsampling_y>(
        pred_0, pred_1, mask_ptr, height, dst, dst_stride);
    return;
  }
  const uint8_t* mask = mask_ptr;
  const __m128i mask_inverter = _mm_set1_epi16(64);
  int y = 0;
  do {
    int x = 0;
    do {
      const __m128i pred_mask_0 = GetMask8<subsampling_x, subsampling_y>(
          mask + (x << subsampling_x), mask_stride);
      // 64 - mask
      const __m128i pred_mask_1 = _mm_sub_epi16(mask_inverter, pred_mask_0);
      const __m128i mask_lo = _mm_unpacklo_epi16(pred_mask_0, pred_mask_1);
      const __m128i mask_hi = _mm_unpackhi_epi16(pred_mask_0, pred_mask_1);

      const __m128i pred_val_0 = LoadAligned16(pred_0 + x);
      const __m128i pred_val_1 = LoadAligned16(pred_1 + x);
      const __m128i pred_lo = _mm_unpacklo_epi16(pred_val_0, pred_val_1);
      const __m128i pred_hi = _mm_unpackhi_epi16(pred_val_0, pred_val_1);
      // int res = (mask_value * prediction_0[x] +
      //      (64 - mask_value) * prediction_1[x]) >> 6;
      const __m128i compound_pred_lo = _mm_madd_epi16(pred_lo, mask_lo);
      const __m128i compound_pred_hi = _mm_madd_epi16(pred_hi, mask_hi);

      const __m128i res = _mm_packus_epi32(_mm_srli_epi32(compound_pred_lo, 6),
                                           _mm_srli_epi32(compound_pred_hi, 6));
      // dst[x] = static_cast<Pixel>(
      //     Clip3(RightShiftWithRounding(res, inter_post_round_bits), 0,
      //           (1 << kBitdepth8) - 1));
      const __m128i result = RightShiftWithRounding_S16(res, 4);
      StoreLo8(dst + x, _mm_packus_epi16(result, result));

      x += 8;
    } while (x < width);
    dst += dst_stride;
    pred_0 += pred_stride_0;
    pred_1 += pred_stride_1;
    mask += mask_stride << subsampling_y;
  } while (++y < height);
}

inline void InterIntraWriteMaskBlendLine8bpp4x2(
    const uint8_t* LIBGAV1_RESTRICT const pred_0,
    uint8_t* LIBGAV1_RESTRICT const pred_1, const ptrdiff_t pred_stride_1,
    const __m128i pred_mask_0, const __m128i pred_mask_1) {
  const __m128i pred_mask = _mm_unpacklo_epi8(pred_mask_0, pred_mask_1);

  const __m128i pred_val_0 = LoadLo8(pred_0);
  __m128i pred_val_1 = Load4(pred_1);
  pred_val_1 = _mm_or_si128(_mm_slli_si128(Load4(pred_1 + pred_stride_1), 4),
                            pred_val_1);
  const __m128i pred = _mm_unpacklo_epi8(pred_val_0, pred_val_1);
  // int res = (mask_value * prediction_1[x] +
  //      (64 - mask_value) * prediction_0[x]) >> 6;
  const __m128i compound_pred = _mm_maddubs_epi16(pred, pred_mask);
  const __m128i result = RightShiftWithRounding_U16(compound_pred, 6);
  const __m128i res = _mm_packus_epi16(result, result);

  Store4(pred_1, res);
  Store4(pred_1 + pred_stride_1, _mm_srli_si128(res, 4));
}

template <int subsampling_x, int subsampling_y>
inline void InterIntraMaskBlending8bpp4x4_SSE4_1(
    const uint8_t* LIBGAV1_RESTRICT pred_0, uint8_t* LIBGAV1_RESTRICT pred_1,
    const ptrdiff_t pred_stride_1, const uint8_t* LIBGAV1_RESTRICT mask,
    const ptrdiff_t mask_stride) {
  const __m128i mask_inverter = _mm_set1_epi8(64);
  const __m128i pred_mask_u16_first =
      GetInterIntraMask4x2<subsampling_x, subsampling_y>(mask, mask_stride);
  mask += mask_stride << (1 + subsampling_y);
  const __m128i pred_mask_u16_second =
      GetInterIntraMask4x2<subsampling_x, subsampling_y>(mask, mask_stride);
  mask += mask_stride << (1 + subsampling_y);
  __m128i pred_mask_1 =
      _mm_packus_epi16(pred_mask_u16_first, pred_mask_u16_second);
  __m128i pred_mask_0 = _mm_sub_epi8(mask_inverter, pred_mask_1);
  InterIntraWriteMaskBlendLine8bpp4x2(pred_0, pred_1, pred_stride_1,
                                      pred_mask_0, pred_mask_1);
  pred_0 += 4 << 1;
  pred_1 += pred_stride_1 << 1;

  pred_mask_1 = _mm_srli_si128(pred_mask_1, 8);
  pred_mask_0 = _mm_sub_epi8(mask_inverter, pred_mask_1);
  InterIntraWriteMaskBlendLine8bpp4x2(pred_0, pred_1, pred_stride_1,
                                      pred_mask_0, pred_mask_1);
}

template <int subsampling_x, int subsampling_y>
inline void InterIntraMaskBlending8bpp4xH_SSE4_1(
    const uint8_t* LIBGAV1_RESTRICT pred_0, uint8_t* LIBGAV1_RESTRICT pred_1,
    const ptrdiff_t pred_stride_1,
    const uint8_t* LIBGAV1_RESTRICT const mask_ptr, const ptrdiff_t mask_stride,
    const int height) {
  const uint8_t* mask = mask_ptr;
  if (height == 4) {
    InterIntraMaskBlending8bpp4x4_SSE4_1<subsampling_x, subsampling_y>(
        pred_0, pred_1, pred_stride_1, mask, mask_stride);
    return;
  }
  int y = 0;
  do {
    InterIntraMaskBlending8bpp4x4_SSE4_1<subsampling_x, subsampling_y>(
        pred_0, pred_1, pred_stride_1, mask, mask_stride);
    pred_0 += 4 << 2;
    pred_1 += pred_stride_1 << 2;
    mask += mask_stride << (2 + subsampling_y);

    InterIntraMaskBlending8bpp4x4_SSE4_1<subsampling_x, subsampling_y>(
        pred_0, pred_1, pred_stride_1, mask, mask_stride);
    pred_0 += 4 << 2;
    pred_1 += pred_stride_1 << 2;
    mask += mask_stride << (2 + subsampling_y);
    y += 8;
  } while (y < height);
}

// This version returns 8-bit packed values to fit in _mm_maddubs_epi16 because,
// when is_inter_intra is true, the prediction values are brought to 8-bit
// packing as well.
template <int subsampling_x, int subsampling_y>
inline __m128i GetInterIntraMask8bpp8(const uint8_t* LIBGAV1_RESTRICT mask,
                                      ptrdiff_t stride) {
  if (subsampling_x == 1) {
    const __m128i ret = GetMask8<subsampling_x, subsampling_y>(mask, stride);
    return _mm_packus_epi16(ret, ret);
  }
  assert(subsampling_y == 0 && subsampling_x == 0);
  // Unfortunately there is no shift operation for 8-bit packing, or else we
  // could return everything with 8-bit packing.
  const __m128i mask_val = LoadLo8(mask);
  return mask_val;
}

template <int subsampling_x, int subsampling_y>
void InterIntraMaskBlend8bpp_SSE4_1(
    const uint8_t* LIBGAV1_RESTRICT prediction_0,
    uint8_t* LIBGAV1_RESTRICT prediction_1, const ptrdiff_t prediction_stride_1,
    const uint8_t* LIBGAV1_RESTRICT const mask_ptr, const ptrdiff_t mask_stride,
    const int width, const int height) {
  if (width == 4) {
    InterIntraMaskBlending8bpp4xH_SSE4_1<subsampling_x, subsampling_y>(
        prediction_0, prediction_1, prediction_stride_1, mask_ptr, mask_stride,
        height);
    return;
  }
  const uint8_t* mask = mask_ptr;
  const __m128i mask_inverter = _mm_set1_epi8(64);
  int y = 0;
  do {
    int x = 0;
    do {
      const __m128i pred_mask_1 =
          GetInterIntraMask8bpp8<subsampling_x, subsampling_y>(
              mask + (x << subsampling_x), mask_stride);
      // 64 - mask
      const __m128i pred_mask_0 = _mm_sub_epi8(mask_inverter, pred_mask_1);
      const __m128i pred_mask = _mm_unpacklo_epi8(pred_mask_0, pred_mask_1);

      const __m128i pred_val_0 = LoadLo8(prediction_0 + x);
      const __m128i pred_val_1 = LoadLo8(prediction_1 + x);
      const __m128i pred = _mm_unpacklo_epi8(pred_val_0, pred_val_1);
      // int res = (mask_value * prediction_1[x] +
      //      (64 - mask_value) * prediction_0[x]) >> 6;
      const __m128i compound_pred = _mm_maddubs_epi16(pred, pred_mask);
      const __m128i result = RightShiftWithRounding_U16(compound_pred, 6);
      const __m128i res = _mm_packus_epi16(result, result);

      StoreLo8(prediction_1 + x, res);

      x += 8;
    } while (x < width);
    prediction_0 += width;
    prediction_1 += prediction_stride_1;
    mask += mask_stride << subsampling_y;
  } while (++y < height);
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
#if DSP_ENABLED_8BPP_SSE4_1(MaskBlend444)
  dsp->mask_blend[0][0] = MaskBlend_SSE4_1<0, 0>;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(MaskBlend422)
  dsp->mask_blend[1][0] = MaskBlend_SSE4_1<1, 0>;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(MaskBlend420)
  dsp->mask_blend[2][0] = MaskBlend_SSE4_1<1, 1>;
#endif
  // The is_inter_intra index of mask_blend[][] is replaced by
  // inter_intra_mask_blend_8bpp[] in 8-bit.
#if DSP_ENABLED_8BPP_SSE4_1(InterIntraMaskBlend8bpp444)
  dsp->inter_intra_mask_blend_8bpp[0] = InterIntraMaskBlend8bpp_SSE4_1<0, 0>;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(InterIntraMaskBlend8bpp422)
  dsp->inter_intra_mask_blend_8bpp[1] = InterIntraMaskBlend8bpp_SSE4_1<1, 0>;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(InterIntraMaskBlend8bpp420)
  dsp->inter_intra_mask_blend_8bpp[2] = InterIntraMaskBlend8bpp_SSE4_1<1, 1>;
#endif
}

}  // namespace
}  // namespace low_bitdepth

#if LIBGAV1_MAX_BITDEPTH >= 10
namespace high_bitdepth {
namespace {

constexpr int kMax10bppSample = (1 << 10) - 1;
constexpr int kMaskInverse = 64;
constexpr int kRoundBitsMaskBlend = 4;

inline __m128i RightShiftWithRoundingConst_S32(const __m128i v_val_d, int bits,
                                               const __m128i shift) {
  const __m128i v_tmp_d = _mm_add_epi32(v_val_d, shift);
  return _mm_srai_epi32(v_tmp_d, bits);
}

template <int subsampling_x, int subsampling_y>
inline __m128i GetMask4x2(const uint8_t* mask) {
  if (subsampling_x == 1 && subsampling_y == 1) {
    const __m128i mask_row_01 = LoadUnaligned16(mask);
    const __m128i mask_row_23 = LoadUnaligned16(mask + 16);
    const __m128i mask_val_0 = _mm_cvtepu8_epi16(mask_row_01);
    const __m128i mask_val_1 =
        _mm_cvtepu8_epi16(_mm_srli_si128(mask_row_01, 8));
    const __m128i mask_val_2 = _mm_cvtepu8_epi16(mask_row_23);
    const __m128i mask_val_3 =
        _mm_cvtepu8_epi16(_mm_srli_si128(mask_row_23, 8));
    const __m128i subsampled_mask_02 = _mm_hadd_epi16(mask_val_0, mask_val_2);
    const __m128i subsampled_mask_13 = _mm_hadd_epi16(mask_val_1, mask_val_3);
    const __m128i subsampled_mask =
        _mm_add_epi16(subsampled_mask_02, subsampled_mask_13);
    return RightShiftWithRounding_U16(subsampled_mask, 2);
  }
  if (subsampling_x == 1) {
    const __m128i mask_row_01 = LoadUnaligned16(mask);
    const __m128i mask_val_0 = _mm_cvtepu8_epi16(mask_row_01);
    const __m128i mask_val_1 =
        _mm_cvtepu8_epi16(_mm_srli_si128(mask_row_01, 8));
    const __m128i subsampled_mask = _mm_hadd_epi16(mask_val_0, mask_val_1);
    return RightShiftWithRounding_U16(subsampled_mask, 1);
  }
  return _mm_cvtepu8_epi16(LoadLo8(mask));
}

inline void WriteMaskBlendLine10bpp4x2_SSE4_1(
    const uint16_t* LIBGAV1_RESTRICT pred_0,
    const uint16_t* LIBGAV1_RESTRICT pred_1, const ptrdiff_t pred_stride_1,
    const __m128i& pred_mask_0, const __m128i& pred_mask_1,
    const __m128i& offset, const __m128i& max, const __m128i& shift4,
    uint16_t* LIBGAV1_RESTRICT dst, const ptrdiff_t dst_stride) {
  const __m128i pred_val_0 = LoadUnaligned16(pred_0);
  const __m128i pred_val_1 = LoadHi8(LoadLo8(pred_1), pred_1 + pred_stride_1);

  // int res = (mask_value * pred_0[x] + (64 - mask_value) * pred_1[x]) >> 6;
  const __m128i compound_pred_lo_0 = _mm_mullo_epi16(pred_val_0, pred_mask_0);
  const __m128i compound_pred_hi_0 = _mm_mulhi_epu16(pred_val_0, pred_mask_0);
  const __m128i compound_pred_lo_1 = _mm_mullo_epi16(pred_val_1, pred_mask_1);
  const __m128i compound_pred_hi_1 = _mm_mulhi_epu16(pred_val_1, pred_mask_1);
  const __m128i pack0_lo =
      _mm_unpacklo_epi16(compound_pred_lo_0, compound_pred_hi_0);
  const __m128i pack0_hi =
      _mm_unpackhi_epi16(compound_pred_lo_0, compound_pred_hi_0);
  const __m128i pack1_lo =
      _mm_unpacklo_epi16(compound_pred_lo_1, compound_pred_hi_1);
  const __m128i pack1_hi =
      _mm_unpackhi_epi16(compound_pred_lo_1, compound_pred_hi_1);
  const __m128i compound_pred_lo = _mm_add_epi32(pack0_lo, pack1_lo);
  const __m128i compound_pred_hi = _mm_add_epi32(pack0_hi, pack1_hi);
  // res -= (bitdepth == 8) ? 0 : kCompoundOffset;
  const __m128i sub_0 =
      _mm_sub_epi32(_mm_srli_epi32(compound_pred_lo, 6), offset);
  const __m128i sub_1 =
      _mm_sub_epi32(_mm_srli_epi32(compound_pred_hi, 6), offset);

  // dst[x] = static_cast<Pixel>(
  //     Clip3(RightShiftWithRounding(res, inter_post_round_bits), 0,
  //           (1 << kBitdepth8) - 1));
  const __m128i shift_0 =
      RightShiftWithRoundingConst_S32(sub_0, kRoundBitsMaskBlend, shift4);
  const __m128i shift_1 =
      RightShiftWithRoundingConst_S32(sub_1, kRoundBitsMaskBlend, shift4);
  const __m128i result = _mm_min_epi16(_mm_packus_epi32(shift_0, shift_1), max);
  StoreLo8(dst, result);
  StoreHi8(dst + dst_stride, result);
}

template <int subsampling_x, int subsampling_y>
inline void MaskBlend10bpp4x4_SSE4_1(const uint16_t* LIBGAV1_RESTRICT pred_0,
                                     const uint16_t* LIBGAV1_RESTRICT pred_1,
                                     const ptrdiff_t pred_stride_1,
                                     const uint8_t* LIBGAV1_RESTRICT mask,
                                     const ptrdiff_t mask_stride,
                                     uint16_t* LIBGAV1_RESTRICT dst,
                                     const ptrdiff_t dst_stride) {
  const __m128i mask_inverter = _mm_set1_epi16(kMaskInverse);
  const __m128i shift4 = _mm_set1_epi32((1 << kRoundBitsMaskBlend) >> 1);
  const __m128i offset = _mm_set1_epi32(kCompoundOffset);
  const __m128i max = _mm_set1_epi16(kMax10bppSample);
  __m128i pred_mask_0 = GetMask4x2<subsampling_x, subsampling_y>(mask);
  __m128i pred_mask_1 = _mm_sub_epi16(mask_inverter, pred_mask_0);
  WriteMaskBlendLine10bpp4x2_SSE4_1(pred_0, pred_1, pred_stride_1, pred_mask_0,
                                    pred_mask_1, offset, max, shift4, dst,
                                    dst_stride);
  pred_0 += 4 << 1;
  pred_1 += pred_stride_1 << 1;
  mask += mask_stride << (1 + subsampling_y);
  dst += dst_stride << 1;

  pred_mask_0 = GetMask4x2<subsampling_x, subsampling_y>(mask);
  pred_mask_1 = _mm_sub_epi16(mask_inverter, pred_mask_0);
  WriteMaskBlendLine10bpp4x2_SSE4_1(pred_0, pred_1, pred_stride_1, pred_mask_0,
                                    pred_mask_1, offset, max, shift4, dst,
                                    dst_stride);
}

template <int subsampling_x, int subsampling_y>
inline void MaskBlend10bpp4xH_SSE4_1(
    const uint16_t* LIBGAV1_RESTRICT pred_0,
    const uint16_t* LIBGAV1_RESTRICT pred_1, const ptrdiff_t pred_stride_1,
    const uint8_t* LIBGAV1_RESTRICT const mask_ptr, const ptrdiff_t mask_stride,
    const int height, uint16_t* LIBGAV1_RESTRICT dst,
    const ptrdiff_t dst_stride) {
  const uint8_t* mask = mask_ptr;
  if (height == 4) {
    MaskBlend10bpp4x4_SSE4_1<subsampling_x, subsampling_y>(
        pred_0, pred_1, pred_stride_1, mask, mask_stride, dst, dst_stride);
    return;
  }
  const __m128i mask_inverter = _mm_set1_epi16(kMaskInverse);
  const uint8_t pred0_stride2 = 4 << 1;
  const ptrdiff_t pred1_stride2 = pred_stride_1 << 1;
  const ptrdiff_t mask_stride2 = mask_stride << (1 + subsampling_y);
  const ptrdiff_t dst_stride2 = dst_stride << 1;
  const __m128i offset = _mm_set1_epi32(kCompoundOffset);
  const __m128i max = _mm_set1_epi16(kMax10bppSample);
  const __m128i shift4 = _mm_set1_epi32((1 << kRoundBitsMaskBlend) >> 1);
  int y = height;
  do {
    __m128i pred_mask_0 = GetMask4x2<subsampling_x, subsampling_y>(mask);
    __m128i pred_mask_1 = _mm_sub_epi16(mask_inverter, pred_mask_0);

    WriteMaskBlendLine10bpp4x2_SSE4_1(pred_0, pred_1, pred_stride_1,
                                      pred_mask_0, pred_mask_1, offset, max,
                                      shift4, dst, dst_stride);
    pred_0 += pred0_stride2;
    pred_1 += pred1_stride2;
    mask += mask_stride2;
    dst += dst_stride2;

    pred_mask_0 = GetMask4x2<subsampling_x, subsampling_y>(mask);
    pred_mask_1 = _mm_sub_epi16(mask_inverter, pred_mask_0);
    WriteMaskBlendLine10bpp4x2_SSE4_1(pred_0, pred_1, pred_stride_1,
                                      pred_mask_0, pred_mask_1, offset, max,
                                      shift4, dst, dst_stride);
    pred_0 += pred0_stride2;
    pred_1 += pred1_stride2;
    mask += mask_stride2;
    dst += dst_stride2;

    pred_mask_0 = GetMask4x2<subsampling_x, subsampling_y>(mask);
    pred_mask_1 = _mm_sub_epi16(mask_inverter, pred_mask_0);
    WriteMaskBlendLine10bpp4x2_SSE4_1(pred_0, pred_1, pred_stride_1,
                                      pred_mask_0, pred_mask_1, offset, max,
                                      shift4, dst, dst_stride);
    pred_0 += pred0_stride2;
    pred_1 += pred1_stride2;
    mask += mask_stride2;
    dst += dst_stride2;

    pred_mask_0 = GetMask4x2<subsampling_x, subsampling_y>(mask);
    pred_mask_1 = _mm_sub_epi16(mask_inverter, pred_mask_0);
    WriteMaskBlendLine10bpp4x2_SSE4_1(pred_0, pred_1, pred_stride_1,
                                      pred_mask_0, pred_mask_1, offset, max,
                                      shift4, dst, dst_stride);
    pred_0 += pred0_stride2;
    pred_1 += pred1_stride2;
    mask += mask_stride2;
    dst += dst_stride2;
    y -= 8;
  } while (y != 0);
}

template <int subsampling_x, int subsampling_y>
inline void MaskBlend10bpp_SSE4_1(
    const void* LIBGAV1_RESTRICT prediction_0,
    const void* LIBGAV1_RESTRICT prediction_1,
    const ptrdiff_t prediction_stride_1,
    const uint8_t* LIBGAV1_RESTRICT const mask_ptr, const ptrdiff_t mask_stride,
    const int width, const int height, void* LIBGAV1_RESTRICT dest,
    const ptrdiff_t dest_stride) {
  auto* dst = static_cast<uint16_t*>(dest);
  const ptrdiff_t dst_stride = dest_stride / sizeof(dst[0]);
  const auto* pred_0 = static_cast<const uint16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const uint16_t*>(prediction_1);
  const ptrdiff_t pred_stride_0 = width;
  const ptrdiff_t pred_stride_1 = prediction_stride_1;
  if (width == 4) {
    MaskBlend10bpp4xH_SSE4_1<subsampling_x, subsampling_y>(
        pred_0, pred_1, pred_stride_1, mask_ptr, mask_stride, height, dst,
        dst_stride);
    return;
  }
  const uint8_t* mask = mask_ptr;
  const __m128i mask_inverter = _mm_set1_epi16(kMaskInverse);
  const ptrdiff_t mask_stride_ss = mask_stride << subsampling_y;
  const __m128i offset = _mm_set1_epi32(kCompoundOffset);
  const __m128i max = _mm_set1_epi16(kMax10bppSample);
  const __m128i shift4 = _mm_set1_epi32((1 << kRoundBitsMaskBlend) >> 1);
  int y = height;
  do {
    int x = 0;
    do {
      const __m128i pred_mask_0 = GetMask8<subsampling_x, subsampling_y>(
          mask + (x << subsampling_x), mask_stride);
      const __m128i pred_val_0 = LoadUnaligned16(pred_0 + x);
      const __m128i pred_val_1 = LoadUnaligned16(pred_1 + x);
      // 64 - mask
      const __m128i pred_mask_1 = _mm_sub_epi16(mask_inverter, pred_mask_0);

      const __m128i compound_pred_lo_0 =
          _mm_mullo_epi16(pred_val_0, pred_mask_0);
      const __m128i compound_pred_hi_0 =
          _mm_mulhi_epu16(pred_val_0, pred_mask_0);
      const __m128i compound_pred_lo_1 =
          _mm_mullo_epi16(pred_val_1, pred_mask_1);
      const __m128i compound_pred_hi_1 =
          _mm_mulhi_epu16(pred_val_1, pred_mask_1);
      const __m128i pack0_lo =
          _mm_unpacklo_epi16(compound_pred_lo_0, compound_pred_hi_0);
      const __m128i pack0_hi =
          _mm_unpackhi_epi16(compound_pred_lo_0, compound_pred_hi_0);
      const __m128i pack1_lo =
          _mm_unpacklo_epi16(compound_pred_lo_1, compound_pred_hi_1);
      const __m128i pack1_hi =
          _mm_unpackhi_epi16(compound_pred_lo_1, compound_pred_hi_1);
      const __m128i compound_pred_lo = _mm_add_epi32(pack0_lo, pack1_lo);
      const __m128i compound_pred_hi = _mm_add_epi32(pack0_hi, pack1_hi);

      const __m128i sub_0 =
          _mm_sub_epi32(_mm_srli_epi32(compound_pred_lo, 6), offset);
      const __m128i sub_1 =
          _mm_sub_epi32(_mm_srli_epi32(compound_pred_hi, 6), offset);
      const __m128i shift_0 =
          RightShiftWithRoundingConst_S32(sub_0, kRoundBitsMaskBlend, shift4);
      const __m128i shift_1 =
          RightShiftWithRoundingConst_S32(sub_1, kRoundBitsMaskBlend, shift4);
      const __m128i result =
          _mm_min_epi16(_mm_packus_epi32(shift_0, shift_1), max);
      StoreUnaligned16(dst + x, result);
      x += 8;
    } while (x < width);
    dst += dst_stride;
    pred_0 += pred_stride_0;
    pred_1 += pred_stride_1;
    mask += mask_stride_ss;
  } while (--y != 0);
}
inline void InterIntraWriteMaskBlendLine10bpp4x2_SSE4_1(
    const uint16_t* LIBGAV1_RESTRICT prediction_0,
    const uint16_t* LIBGAV1_RESTRICT prediction_1,
    const ptrdiff_t pred_stride_1, const __m128i& pred_mask_0,
    const __m128i& pred_mask_1, const __m128i& shift6,
    uint16_t* LIBGAV1_RESTRICT dst, const ptrdiff_t dst_stride) {
  const __m128i pred_val_0 = LoadUnaligned16(prediction_0);
  const __m128i pred_val_1 =
      LoadHi8(LoadLo8(prediction_1), prediction_1 + pred_stride_1);

  const __m128i mask_0 = _mm_unpacklo_epi16(pred_mask_1, pred_mask_0);
  const __m128i mask_1 = _mm_unpackhi_epi16(pred_mask_1, pred_mask_0);
  const __m128i pred_0 = _mm_unpacklo_epi16(pred_val_0, pred_val_1);
  const __m128i pred_1 = _mm_unpackhi_epi16(pred_val_0, pred_val_1);

  const __m128i compound_pred_0 = _mm_madd_epi16(pred_0, mask_0);
  const __m128i compound_pred_1 = _mm_madd_epi16(pred_1, mask_1);
  const __m128i shift_0 =
      RightShiftWithRoundingConst_S32(compound_pred_0, 6, shift6);
  const __m128i shift_1 =
      RightShiftWithRoundingConst_S32(compound_pred_1, 6, shift6);
  const __m128i res = _mm_packus_epi32(shift_0, shift_1);
  StoreLo8(dst, res);
  StoreHi8(dst + dst_stride, res);
}

template <int subsampling_x, int subsampling_y>
inline void InterIntraMaskBlend10bpp4x4_SSE4_1(
    const uint16_t* LIBGAV1_RESTRICT pred_0,
    const uint16_t* LIBGAV1_RESTRICT pred_1, const ptrdiff_t pred_stride_1,
    const uint8_t* LIBGAV1_RESTRICT mask, const ptrdiff_t mask_stride,
    uint16_t* LIBGAV1_RESTRICT dst, const ptrdiff_t dst_stride) {
  const __m128i mask_inverter = _mm_set1_epi16(kMaskInverse);
  const __m128i shift6 = _mm_set1_epi32((1 << 6) >> 1);
  __m128i pred_mask_0 =
      GetInterIntraMask4x2<subsampling_x, subsampling_y>(mask, mask_stride);
  __m128i pred_mask_1 = _mm_sub_epi16(mask_inverter, pred_mask_0);
  InterIntraWriteMaskBlendLine10bpp4x2_SSE4_1(pred_0, pred_1, pred_stride_1,
                                              pred_mask_0, pred_mask_1, shift6,
                                              dst, dst_stride);
  pred_0 += 4 << 1;
  pred_1 += pred_stride_1 << 1;
  mask += mask_stride << (1 + subsampling_y);
  dst += dst_stride << 1;

  pred_mask_0 =
      GetInterIntraMask4x2<subsampling_x, subsampling_y>(mask, mask_stride);
  pred_mask_1 = _mm_sub_epi16(mask_inverter, pred_mask_0);
  InterIntraWriteMaskBlendLine10bpp4x2_SSE4_1(pred_0, pred_1, pred_stride_1,
                                              pred_mask_0, pred_mask_1, shift6,
                                              dst, dst_stride);
}

template <int subsampling_x, int subsampling_y>
inline void InterIntraMaskBlend10bpp4xH_SSE4_1(
    const uint16_t* LIBGAV1_RESTRICT pred_0,
    const uint16_t* LIBGAV1_RESTRICT pred_1, const ptrdiff_t pred_stride_1,
    const uint8_t* LIBGAV1_RESTRICT const mask_ptr, const ptrdiff_t mask_stride,
    const int height, uint16_t* LIBGAV1_RESTRICT dst,
    const ptrdiff_t dst_stride) {
  const uint8_t* mask = mask_ptr;
  if (height == 4) {
    InterIntraMaskBlend10bpp4x4_SSE4_1<subsampling_x, subsampling_y>(
        pred_0, pred_1, pred_stride_1, mask, mask_stride, dst, dst_stride);
    return;
  }
  const __m128i mask_inverter = _mm_set1_epi16(kMaskInverse);
  const __m128i shift6 = _mm_set1_epi32((1 << 6) >> 1);
  const uint8_t pred0_stride2 = 4 << 1;
  const ptrdiff_t pred1_stride2 = pred_stride_1 << 1;
  const ptrdiff_t mask_stride2 = mask_stride << (1 + subsampling_y);
  const ptrdiff_t dst_stride2 = dst_stride << 1;
  int y = height;
  do {
    __m128i pred_mask_0 =
        GetInterIntraMask4x2<subsampling_x, subsampling_y>(mask, mask_stride);
    __m128i pred_mask_1 = _mm_sub_epi16(mask_inverter, pred_mask_0);
    InterIntraWriteMaskBlendLine10bpp4x2_SSE4_1(pred_0, pred_1, pred_stride_1,
                                                pred_mask_0, pred_mask_1,
                                                shift6, dst, dst_stride);
    pred_0 += pred0_stride2;
    pred_1 += pred1_stride2;
    mask += mask_stride2;
    dst += dst_stride2;

    pred_mask_0 =
        GetInterIntraMask4x2<subsampling_x, subsampling_y>(mask, mask_stride);
    pred_mask_1 = _mm_sub_epi16(mask_inverter, pred_mask_0);
    InterIntraWriteMaskBlendLine10bpp4x2_SSE4_1(pred_0, pred_1, pred_stride_1,
                                                pred_mask_0, pred_mask_1,
                                                shift6, dst, dst_stride);
    pred_0 += pred0_stride2;
    pred_1 += pred1_stride2;
    mask += mask_stride2;
    dst += dst_stride2;

    pred_mask_0 =
        GetInterIntraMask4x2<subsampling_x, subsampling_y>(mask, mask_stride);
    pred_mask_1 = _mm_sub_epi16(mask_inverter, pred_mask_0);
    InterIntraWriteMaskBlendLine10bpp4x2_SSE4_1(pred_0, pred_1, pred_stride_1,
                                                pred_mask_0, pred_mask_1,
                                                shift6, dst, dst_stride);
    pred_0 += pred0_stride2;
    pred_1 += pred1_stride2;
    mask += mask_stride2;
    dst += dst_stride2;

    pred_mask_0 =
        GetInterIntraMask4x2<subsampling_x, subsampling_y>(mask, mask_stride);
    pred_mask_1 = _mm_sub_epi16(mask_inverter, pred_mask_0);
    InterIntraWriteMaskBlendLine10bpp4x2_SSE4_1(pred_0, pred_1, pred_stride_1,
                                                pred_mask_0, pred_mask_1,
                                                shift6, dst, dst_stride);
    pred_0 += pred0_stride2;
    pred_1 += pred1_stride2;
    mask += mask_stride2;
    dst += dst_stride2;
    y -= 8;
  } while (y != 0);
}

template <int subsampling_x, int subsampling_y>
inline void InterIntraMaskBlend10bpp_SSE4_1(
    const void* LIBGAV1_RESTRICT prediction_0,
    const void* LIBGAV1_RESTRICT prediction_1,
    const ptrdiff_t prediction_stride_1,
    const uint8_t* LIBGAV1_RESTRICT const mask_ptr, const ptrdiff_t mask_stride,
    const int width, const int height, void* LIBGAV1_RESTRICT dest,
    const ptrdiff_t dest_stride) {
  auto* dst = static_cast<uint16_t*>(dest);
  const ptrdiff_t dst_stride = dest_stride / sizeof(dst[0]);
  const auto* pred_0 = static_cast<const uint16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const uint16_t*>(prediction_1);
  const ptrdiff_t pred_stride_0 = width;
  const ptrdiff_t pred_stride_1 = prediction_stride_1;
  if (width == 4) {
    InterIntraMaskBlend10bpp4xH_SSE4_1<subsampling_x, subsampling_y>(
        pred_0, pred_1, pred_stride_1, mask_ptr, mask_stride, height, dst,
        dst_stride);
    return;
  }
  const uint8_t* mask = mask_ptr;
  const __m128i mask_inverter = _mm_set1_epi16(kMaskInverse);
  const __m128i shift6 = _mm_set1_epi32((1 << 6) >> 1);
  const ptrdiff_t mask_stride_ss = mask_stride << subsampling_y;
  int y = height;
  do {
    int x = 0;
    do {
      const __m128i pred_mask_0 = GetMask8<subsampling_x, subsampling_y>(
          mask + (x << subsampling_x), mask_stride);
      const __m128i pred_val_0 = LoadUnaligned16(pred_0 + x);
      const __m128i pred_val_1 = LoadUnaligned16(pred_1 + x);
      // 64 - mask
      const __m128i pred_mask_1 = _mm_sub_epi16(mask_inverter, pred_mask_0);
      const __m128i mask_0 = _mm_unpacklo_epi16(pred_mask_1, pred_mask_0);
      const __m128i mask_1 = _mm_unpackhi_epi16(pred_mask_1, pred_mask_0);
      const __m128i pred_0 = _mm_unpacklo_epi16(pred_val_0, pred_val_1);
      const __m128i pred_1 = _mm_unpackhi_epi16(pred_val_0, pred_val_1);

      const __m128i compound_pred_0 = _mm_madd_epi16(pred_0, mask_0);
      const __m128i compound_pred_1 = _mm_madd_epi16(pred_1, mask_1);
      const __m128i shift_0 =
          RightShiftWithRoundingConst_S32(compound_pred_0, 6, shift6);
      const __m128i shift_1 =
          RightShiftWithRoundingConst_S32(compound_pred_1, 6, shift6);
      StoreUnaligned16(dst + x, _mm_packus_epi32(shift_0, shift_1));
      x += 8;
    } while (x < width);
    dst += dst_stride;
    pred_0 += pred_stride_0;
    pred_1 += pred_stride_1;
    mask += mask_stride_ss;
  } while (--y != 0);
}

void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth10);
  assert(dsp != nullptr);

#if DSP_ENABLED_10BPP_SSE4_1(MaskBlend444)
  dsp->mask_blend[0][0] = MaskBlend10bpp_SSE4_1<0, 0>;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(MaskBlend422)
  dsp->mask_blend[1][0] = MaskBlend10bpp_SSE4_1<1, 0>;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(MaskBlend420)
  dsp->mask_blend[2][0] = MaskBlend10bpp_SSE4_1<1, 1>;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(MaskBlendInterIntra444)
  dsp->mask_blend[0][1] = InterIntraMaskBlend10bpp_SSE4_1<0, 0>;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(MaskBlendInterIntra422)
  dsp->mask_blend[1][1] = InterIntraMaskBlend10bpp_SSE4_1<1, 0>;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(MaskBlendInterIntra420)
  dsp->mask_blend[2][1] = InterIntraMaskBlend10bpp_SSE4_1<1, 1>;
#endif
}

}  // namespace
}  // namespace high_bitdepth
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

void MaskBlendInit_SSE4_1() {
  low_bitdepth::Init8bpp();
#if LIBGAV1_MAX_BITDEPTH >= 10
  high_bitdepth::Init10bpp();
#endif  // LIBGAV1_MAX_BITDEPTH >= 10
}

}  // namespace dsp
}  // namespace libgav1

#else   // !LIBGAV1_TARGETING_SSE4_1

namespace libgav1 {
namespace dsp {

void MaskBlendInit_SSE4_1() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_TARGETING_SSE4_1
