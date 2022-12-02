/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <immintrin.h>

#include "config/aom_dsp_rtcd.h"
#include "aom/aom_integer.h"
#include "aom_dsp/x86/bitdepth_conversion_avx2.h"
#include "aom_ports/mem.h"

static void hadamard_col8x2_avx2(__m256i *in, int iter) {
  __m256i a0 = in[0];
  __m256i a1 = in[1];
  __m256i a2 = in[2];
  __m256i a3 = in[3];
  __m256i a4 = in[4];
  __m256i a5 = in[5];
  __m256i a6 = in[6];
  __m256i a7 = in[7];

  __m256i b0 = _mm256_add_epi16(a0, a1);
  __m256i b1 = _mm256_sub_epi16(a0, a1);
  __m256i b2 = _mm256_add_epi16(a2, a3);
  __m256i b3 = _mm256_sub_epi16(a2, a3);
  __m256i b4 = _mm256_add_epi16(a4, a5);
  __m256i b5 = _mm256_sub_epi16(a4, a5);
  __m256i b6 = _mm256_add_epi16(a6, a7);
  __m256i b7 = _mm256_sub_epi16(a6, a7);

  a0 = _mm256_add_epi16(b0, b2);
  a1 = _mm256_add_epi16(b1, b3);
  a2 = _mm256_sub_epi16(b0, b2);
  a3 = _mm256_sub_epi16(b1, b3);
  a4 = _mm256_add_epi16(b4, b6);
  a5 = _mm256_add_epi16(b5, b7);
  a6 = _mm256_sub_epi16(b4, b6);
  a7 = _mm256_sub_epi16(b5, b7);

  if (iter == 0) {
    b0 = _mm256_add_epi16(a0, a4);
    b7 = _mm256_add_epi16(a1, a5);
    b3 = _mm256_add_epi16(a2, a6);
    b4 = _mm256_add_epi16(a3, a7);
    b2 = _mm256_sub_epi16(a0, a4);
    b6 = _mm256_sub_epi16(a1, a5);
    b1 = _mm256_sub_epi16(a2, a6);
    b5 = _mm256_sub_epi16(a3, a7);

    a0 = _mm256_unpacklo_epi16(b0, b1);
    a1 = _mm256_unpacklo_epi16(b2, b3);
    a2 = _mm256_unpackhi_epi16(b0, b1);
    a3 = _mm256_unpackhi_epi16(b2, b3);
    a4 = _mm256_unpacklo_epi16(b4, b5);
    a5 = _mm256_unpacklo_epi16(b6, b7);
    a6 = _mm256_unpackhi_epi16(b4, b5);
    a7 = _mm256_unpackhi_epi16(b6, b7);

    b0 = _mm256_unpacklo_epi32(a0, a1);
    b1 = _mm256_unpacklo_epi32(a4, a5);
    b2 = _mm256_unpackhi_epi32(a0, a1);
    b3 = _mm256_unpackhi_epi32(a4, a5);
    b4 = _mm256_unpacklo_epi32(a2, a3);
    b5 = _mm256_unpacklo_epi32(a6, a7);
    b6 = _mm256_unpackhi_epi32(a2, a3);
    b7 = _mm256_unpackhi_epi32(a6, a7);

    in[0] = _mm256_unpacklo_epi64(b0, b1);
    in[1] = _mm256_unpackhi_epi64(b0, b1);
    in[2] = _mm256_unpacklo_epi64(b2, b3);
    in[3] = _mm256_unpackhi_epi64(b2, b3);
    in[4] = _mm256_unpacklo_epi64(b4, b5);
    in[5] = _mm256_unpackhi_epi64(b4, b5);
    in[6] = _mm256_unpacklo_epi64(b6, b7);
    in[7] = _mm256_unpackhi_epi64(b6, b7);
  } else {
    in[0] = _mm256_add_epi16(a0, a4);
    in[7] = _mm256_add_epi16(a1, a5);
    in[3] = _mm256_add_epi16(a2, a6);
    in[4] = _mm256_add_epi16(a3, a7);
    in[2] = _mm256_sub_epi16(a0, a4);
    in[6] = _mm256_sub_epi16(a1, a5);
    in[1] = _mm256_sub_epi16(a2, a6);
    in[5] = _mm256_sub_epi16(a3, a7);
  }
}

void aom_hadamard_lp_8x8_dual_avx2(const int16_t *src_diff,
                                   ptrdiff_t src_stride, int16_t *coeff) {
  __m256i src[8];
  src[0] = _mm256_loadu_si256((const __m256i *)src_diff);
  src[1] = _mm256_loadu_si256((const __m256i *)(src_diff += src_stride));
  src[2] = _mm256_loadu_si256((const __m256i *)(src_diff += src_stride));
  src[3] = _mm256_loadu_si256((const __m256i *)(src_diff += src_stride));
  src[4] = _mm256_loadu_si256((const __m256i *)(src_diff += src_stride));
  src[5] = _mm256_loadu_si256((const __m256i *)(src_diff += src_stride));
  src[6] = _mm256_loadu_si256((const __m256i *)(src_diff += src_stride));
  src[7] = _mm256_loadu_si256((const __m256i *)(src_diff + src_stride));

  hadamard_col8x2_avx2(src, 0);
  hadamard_col8x2_avx2(src, 1);

  _mm256_storeu_si256((__m256i *)coeff,
                      _mm256_permute2x128_si256(src[0], src[1], 0x20));
  coeff += 16;
  _mm256_storeu_si256((__m256i *)coeff,
                      _mm256_permute2x128_si256(src[2], src[3], 0x20));
  coeff += 16;
  _mm256_storeu_si256((__m256i *)coeff,
                      _mm256_permute2x128_si256(src[4], src[5], 0x20));
  coeff += 16;
  _mm256_storeu_si256((__m256i *)coeff,
                      _mm256_permute2x128_si256(src[6], src[7], 0x20));
  coeff += 16;
  _mm256_storeu_si256((__m256i *)coeff,
                      _mm256_permute2x128_si256(src[0], src[1], 0x31));
  coeff += 16;
  _mm256_storeu_si256((__m256i *)coeff,
                      _mm256_permute2x128_si256(src[2], src[3], 0x31));
  coeff += 16;
  _mm256_storeu_si256((__m256i *)coeff,
                      _mm256_permute2x128_si256(src[4], src[5], 0x31));
  coeff += 16;
  _mm256_storeu_si256((__m256i *)coeff,
                      _mm256_permute2x128_si256(src[6], src[7], 0x31));
}

static INLINE void hadamard_16x16_avx2(const int16_t *src_diff,
                                       ptrdiff_t src_stride, tran_low_t *coeff,
                                       int is_final) {
  DECLARE_ALIGNED(32, int16_t, temp_coeff[16 * 16]);
  int16_t *t_coeff = temp_coeff;
  int16_t *coeff16 = (int16_t *)coeff;
  int idx;
  for (idx = 0; idx < 2; ++idx) {
    const int16_t *src_ptr = src_diff + idx * 8 * src_stride;
    aom_hadamard_lp_8x8_dual_avx2(src_ptr, src_stride,
                                  t_coeff + (idx * 64 * 2));
  }

  for (idx = 0; idx < 64; idx += 16) {
    const __m256i coeff0 = _mm256_loadu_si256((const __m256i *)t_coeff);
    const __m256i coeff1 = _mm256_loadu_si256((const __m256i *)(t_coeff + 64));
    const __m256i coeff2 = _mm256_loadu_si256((const __m256i *)(t_coeff + 128));
    const __m256i coeff3 = _mm256_loadu_si256((const __m256i *)(t_coeff + 192));

    __m256i b0 = _mm256_add_epi16(coeff0, coeff1);
    __m256i b1 = _mm256_sub_epi16(coeff0, coeff1);
    __m256i b2 = _mm256_add_epi16(coeff2, coeff3);
    __m256i b3 = _mm256_sub_epi16(coeff2, coeff3);

    b0 = _mm256_srai_epi16(b0, 1);
    b1 = _mm256_srai_epi16(b1, 1);
    b2 = _mm256_srai_epi16(b2, 1);
    b3 = _mm256_srai_epi16(b3, 1);
    if (is_final) {
      store_tran_low(_mm256_add_epi16(b0, b2), coeff);
      store_tran_low(_mm256_add_epi16(b1, b3), coeff + 64);
      store_tran_low(_mm256_sub_epi16(b0, b2), coeff + 128);
      store_tran_low(_mm256_sub_epi16(b1, b3), coeff + 192);
      coeff += 16;
    } else {
      _mm256_storeu_si256((__m256i *)coeff16, _mm256_add_epi16(b0, b2));
      _mm256_storeu_si256((__m256i *)(coeff16 + 64), _mm256_add_epi16(b1, b3));
      _mm256_storeu_si256((__m256i *)(coeff16 + 128), _mm256_sub_epi16(b0, b2));
      _mm256_storeu_si256((__m256i *)(coeff16 + 192), _mm256_sub_epi16(b1, b3));
      coeff16 += 16;
    }
    t_coeff += 16;
  }
}

void aom_hadamard_16x16_avx2(const int16_t *src_diff, ptrdiff_t src_stride,
                             tran_low_t *coeff) {
  hadamard_16x16_avx2(src_diff, src_stride, coeff, 1);
}

void aom_hadamard_lp_16x16_avx2(const int16_t *src_diff, ptrdiff_t src_stride,
                                int16_t *coeff) {
  int16_t *t_coeff = coeff;
  for (int idx = 0; idx < 2; ++idx) {
    const int16_t *src_ptr = src_diff + idx * 8 * src_stride;
    aom_hadamard_lp_8x8_dual_avx2(src_ptr, src_stride,
                                  t_coeff + (idx * 64 * 2));
  }

  for (int idx = 0; idx < 64; idx += 16) {
    const __m256i coeff0 = _mm256_loadu_si256((const __m256i *)t_coeff);
    const __m256i coeff1 = _mm256_loadu_si256((const __m256i *)(t_coeff + 64));
    const __m256i coeff2 = _mm256_loadu_si256((const __m256i *)(t_coeff + 128));
    const __m256i coeff3 = _mm256_loadu_si256((const __m256i *)(t_coeff + 192));

    __m256i b0 = _mm256_add_epi16(coeff0, coeff1);
    __m256i b1 = _mm256_sub_epi16(coeff0, coeff1);
    __m256i b2 = _mm256_add_epi16(coeff2, coeff3);
    __m256i b3 = _mm256_sub_epi16(coeff2, coeff3);

    b0 = _mm256_srai_epi16(b0, 1);
    b1 = _mm256_srai_epi16(b1, 1);
    b2 = _mm256_srai_epi16(b2, 1);
    b3 = _mm256_srai_epi16(b3, 1);
    _mm256_storeu_si256((__m256i *)coeff, _mm256_add_epi16(b0, b2));
    _mm256_storeu_si256((__m256i *)(coeff + 64), _mm256_add_epi16(b1, b3));
    _mm256_storeu_si256((__m256i *)(coeff + 128), _mm256_sub_epi16(b0, b2));
    _mm256_storeu_si256((__m256i *)(coeff + 192), _mm256_sub_epi16(b1, b3));
    coeff += 16;
    t_coeff += 16;
  }
}

void aom_hadamard_32x32_avx2(const int16_t *src_diff, ptrdiff_t src_stride,
                             tran_low_t *coeff) {
  // For high bitdepths, it is unnecessary to store_tran_low
  // (mult/unpack/store), then load_tran_low (load/pack) the same memory in the
  // next stage.  Output to an intermediate buffer first, then store_tran_low()
  // in the final stage.
  DECLARE_ALIGNED(32, int16_t, temp_coeff[32 * 32]);
  int16_t *t_coeff = temp_coeff;
  int idx;
  for (idx = 0; idx < 4; ++idx) {
    // src_diff: 9 bit, dynamic range [-255, 255]
    const int16_t *src_ptr =
        src_diff + (idx >> 1) * 16 * src_stride + (idx & 0x01) * 16;
    hadamard_16x16_avx2(src_ptr, src_stride,
                        (tran_low_t *)(t_coeff + idx * 256), 0);
  }

  for (idx = 0; idx < 256; idx += 16) {
    const __m256i coeff0 = _mm256_loadu_si256((const __m256i *)t_coeff);
    const __m256i coeff1 = _mm256_loadu_si256((const __m256i *)(t_coeff + 256));
    const __m256i coeff2 = _mm256_loadu_si256((const __m256i *)(t_coeff + 512));
    const __m256i coeff3 = _mm256_loadu_si256((const __m256i *)(t_coeff + 768));

    __m256i b0 = _mm256_add_epi16(coeff0, coeff1);
    __m256i b1 = _mm256_sub_epi16(coeff0, coeff1);
    __m256i b2 = _mm256_add_epi16(coeff2, coeff3);
    __m256i b3 = _mm256_sub_epi16(coeff2, coeff3);

    b0 = _mm256_srai_epi16(b0, 2);
    b1 = _mm256_srai_epi16(b1, 2);
    b2 = _mm256_srai_epi16(b2, 2);
    b3 = _mm256_srai_epi16(b3, 2);

    store_tran_low(_mm256_add_epi16(b0, b2), coeff);
    store_tran_low(_mm256_add_epi16(b1, b3), coeff + 256);
    store_tran_low(_mm256_sub_epi16(b0, b2), coeff + 512);
    store_tran_low(_mm256_sub_epi16(b1, b3), coeff + 768);

    coeff += 16;
    t_coeff += 16;
  }
}

#if CONFIG_AV1_HIGHBITDEPTH
static void highbd_hadamard_col8_avx2(__m256i *in, int iter) {
  __m256i a0 = in[0];
  __m256i a1 = in[1];
  __m256i a2 = in[2];
  __m256i a3 = in[3];
  __m256i a4 = in[4];
  __m256i a5 = in[5];
  __m256i a6 = in[6];
  __m256i a7 = in[7];

  __m256i b0 = _mm256_add_epi32(a0, a1);
  __m256i b1 = _mm256_sub_epi32(a0, a1);
  __m256i b2 = _mm256_add_epi32(a2, a3);
  __m256i b3 = _mm256_sub_epi32(a2, a3);
  __m256i b4 = _mm256_add_epi32(a4, a5);
  __m256i b5 = _mm256_sub_epi32(a4, a5);
  __m256i b6 = _mm256_add_epi32(a6, a7);
  __m256i b7 = _mm256_sub_epi32(a6, a7);

  a0 = _mm256_add_epi32(b0, b2);
  a1 = _mm256_add_epi32(b1, b3);
  a2 = _mm256_sub_epi32(b0, b2);
  a3 = _mm256_sub_epi32(b1, b3);
  a4 = _mm256_add_epi32(b4, b6);
  a5 = _mm256_add_epi32(b5, b7);
  a6 = _mm256_sub_epi32(b4, b6);
  a7 = _mm256_sub_epi32(b5, b7);

  if (iter == 0) {
    b0 = _mm256_add_epi32(a0, a4);
    b7 = _mm256_add_epi32(a1, a5);
    b3 = _mm256_add_epi32(a2, a6);
    b4 = _mm256_add_epi32(a3, a7);
    b2 = _mm256_sub_epi32(a0, a4);
    b6 = _mm256_sub_epi32(a1, a5);
    b1 = _mm256_sub_epi32(a2, a6);
    b5 = _mm256_sub_epi32(a3, a7);

    a0 = _mm256_unpacklo_epi32(b0, b1);
    a1 = _mm256_unpacklo_epi32(b2, b3);
    a2 = _mm256_unpackhi_epi32(b0, b1);
    a3 = _mm256_unpackhi_epi32(b2, b3);
    a4 = _mm256_unpacklo_epi32(b4, b5);
    a5 = _mm256_unpacklo_epi32(b6, b7);
    a6 = _mm256_unpackhi_epi32(b4, b5);
    a7 = _mm256_unpackhi_epi32(b6, b7);

    b0 = _mm256_unpacklo_epi64(a0, a1);
    b1 = _mm256_unpacklo_epi64(a4, a5);
    b2 = _mm256_unpackhi_epi64(a0, a1);
    b3 = _mm256_unpackhi_epi64(a4, a5);
    b4 = _mm256_unpacklo_epi64(a2, a3);
    b5 = _mm256_unpacklo_epi64(a6, a7);
    b6 = _mm256_unpackhi_epi64(a2, a3);
    b7 = _mm256_unpackhi_epi64(a6, a7);

    in[0] = _mm256_permute2x128_si256(b0, b1, 0x20);
    in[1] = _mm256_permute2x128_si256(b0, b1, 0x31);
    in[2] = _mm256_permute2x128_si256(b2, b3, 0x20);
    in[3] = _mm256_permute2x128_si256(b2, b3, 0x31);
    in[4] = _mm256_permute2x128_si256(b4, b5, 0x20);
    in[5] = _mm256_permute2x128_si256(b4, b5, 0x31);
    in[6] = _mm256_permute2x128_si256(b6, b7, 0x20);
    in[7] = _mm256_permute2x128_si256(b6, b7, 0x31);
  } else {
    in[0] = _mm256_add_epi32(a0, a4);
    in[7] = _mm256_add_epi32(a1, a5);
    in[3] = _mm256_add_epi32(a2, a6);
    in[4] = _mm256_add_epi32(a3, a7);
    in[2] = _mm256_sub_epi32(a0, a4);
    in[6] = _mm256_sub_epi32(a1, a5);
    in[1] = _mm256_sub_epi32(a2, a6);
    in[5] = _mm256_sub_epi32(a3, a7);
  }
}

void aom_highbd_hadamard_8x8_avx2(const int16_t *src_diff, ptrdiff_t src_stride,
                                  tran_low_t *coeff) {
  __m128i src16[8];
  __m256i src32[8];

  src16[0] = _mm_loadu_si128((const __m128i *)src_diff);
  src16[1] = _mm_loadu_si128((const __m128i *)(src_diff += src_stride));
  src16[2] = _mm_loadu_si128((const __m128i *)(src_diff += src_stride));
  src16[3] = _mm_loadu_si128((const __m128i *)(src_diff += src_stride));
  src16[4] = _mm_loadu_si128((const __m128i *)(src_diff += src_stride));
  src16[5] = _mm_loadu_si128((const __m128i *)(src_diff += src_stride));
  src16[6] = _mm_loadu_si128((const __m128i *)(src_diff += src_stride));
  src16[7] = _mm_loadu_si128((const __m128i *)(src_diff + src_stride));

  src32[0] = _mm256_cvtepi16_epi32(src16[0]);
  src32[1] = _mm256_cvtepi16_epi32(src16[1]);
  src32[2] = _mm256_cvtepi16_epi32(src16[2]);
  src32[3] = _mm256_cvtepi16_epi32(src16[3]);
  src32[4] = _mm256_cvtepi16_epi32(src16[4]);
  src32[5] = _mm256_cvtepi16_epi32(src16[5]);
  src32[6] = _mm256_cvtepi16_epi32(src16[6]);
  src32[7] = _mm256_cvtepi16_epi32(src16[7]);

  highbd_hadamard_col8_avx2(src32, 0);
  highbd_hadamard_col8_avx2(src32, 1);

  _mm256_storeu_si256((__m256i *)coeff, src32[0]);
  coeff += 8;
  _mm256_storeu_si256((__m256i *)coeff, src32[1]);
  coeff += 8;
  _mm256_storeu_si256((__m256i *)coeff, src32[2]);
  coeff += 8;
  _mm256_storeu_si256((__m256i *)coeff, src32[3]);
  coeff += 8;
  _mm256_storeu_si256((__m256i *)coeff, src32[4]);
  coeff += 8;
  _mm256_storeu_si256((__m256i *)coeff, src32[5]);
  coeff += 8;
  _mm256_storeu_si256((__m256i *)coeff, src32[6]);
  coeff += 8;
  _mm256_storeu_si256((__m256i *)coeff, src32[7]);
}

void aom_highbd_hadamard_16x16_avx2(const int16_t *src_diff,
                                    ptrdiff_t src_stride, tran_low_t *coeff) {
  int idx;
  tran_low_t *t_coeff = coeff;
  for (idx = 0; idx < 4; ++idx) {
    const int16_t *src_ptr =
        src_diff + (idx >> 1) * 8 * src_stride + (idx & 0x01) * 8;
    aom_highbd_hadamard_8x8_avx2(src_ptr, src_stride, t_coeff + idx * 64);
  }

  for (idx = 0; idx < 64; idx += 8) {
    __m256i coeff0 = _mm256_loadu_si256((const __m256i *)t_coeff);
    __m256i coeff1 = _mm256_loadu_si256((const __m256i *)(t_coeff + 64));
    __m256i coeff2 = _mm256_loadu_si256((const __m256i *)(t_coeff + 128));
    __m256i coeff3 = _mm256_loadu_si256((const __m256i *)(t_coeff + 192));

    __m256i b0 = _mm256_add_epi32(coeff0, coeff1);
    __m256i b1 = _mm256_sub_epi32(coeff0, coeff1);
    __m256i b2 = _mm256_add_epi32(coeff2, coeff3);
    __m256i b3 = _mm256_sub_epi32(coeff2, coeff3);

    b0 = _mm256_srai_epi32(b0, 1);
    b1 = _mm256_srai_epi32(b1, 1);
    b2 = _mm256_srai_epi32(b2, 1);
    b3 = _mm256_srai_epi32(b3, 1);

    coeff0 = _mm256_add_epi32(b0, b2);
    coeff1 = _mm256_add_epi32(b1, b3);
    coeff2 = _mm256_sub_epi32(b0, b2);
    coeff3 = _mm256_sub_epi32(b1, b3);

    _mm256_storeu_si256((__m256i *)coeff, coeff0);
    _mm256_storeu_si256((__m256i *)(coeff + 64), coeff1);
    _mm256_storeu_si256((__m256i *)(coeff + 128), coeff2);
    _mm256_storeu_si256((__m256i *)(coeff + 192), coeff3);

    coeff += 8;
    t_coeff += 8;
  }
}

void aom_highbd_hadamard_32x32_avx2(const int16_t *src_diff,
                                    ptrdiff_t src_stride, tran_low_t *coeff) {
  int idx;
  tran_low_t *t_coeff = coeff;
  for (idx = 0; idx < 4; ++idx) {
    const int16_t *src_ptr =
        src_diff + (idx >> 1) * 16 * src_stride + (idx & 0x01) * 16;
    aom_highbd_hadamard_16x16_avx2(src_ptr, src_stride, t_coeff + idx * 256);
  }

  for (idx = 0; idx < 256; idx += 8) {
    __m256i coeff0 = _mm256_loadu_si256((const __m256i *)t_coeff);
    __m256i coeff1 = _mm256_loadu_si256((const __m256i *)(t_coeff + 256));
    __m256i coeff2 = _mm256_loadu_si256((const __m256i *)(t_coeff + 512));
    __m256i coeff3 = _mm256_loadu_si256((const __m256i *)(t_coeff + 768));

    __m256i b0 = _mm256_add_epi32(coeff0, coeff1);
    __m256i b1 = _mm256_sub_epi32(coeff0, coeff1);
    __m256i b2 = _mm256_add_epi32(coeff2, coeff3);
    __m256i b3 = _mm256_sub_epi32(coeff2, coeff3);

    b0 = _mm256_srai_epi32(b0, 2);
    b1 = _mm256_srai_epi32(b1, 2);
    b2 = _mm256_srai_epi32(b2, 2);
    b3 = _mm256_srai_epi32(b3, 2);

    coeff0 = _mm256_add_epi32(b0, b2);
    coeff1 = _mm256_add_epi32(b1, b3);
    coeff2 = _mm256_sub_epi32(b0, b2);
    coeff3 = _mm256_sub_epi32(b1, b3);

    _mm256_storeu_si256((__m256i *)coeff, coeff0);
    _mm256_storeu_si256((__m256i *)(coeff + 256), coeff1);
    _mm256_storeu_si256((__m256i *)(coeff + 512), coeff2);
    _mm256_storeu_si256((__m256i *)(coeff + 768), coeff3);

    coeff += 8;
    t_coeff += 8;
  }
}
#endif  // CONFIG_AV1_HIGHBITDEPTH

int aom_satd_avx2(const tran_low_t *coeff, int length) {
  __m256i accum = _mm256_setzero_si256();
  int i;

  for (i = 0; i < length; i += 8, coeff += 8) {
    const __m256i src_line = _mm256_loadu_si256((const __m256i *)coeff);
    const __m256i abs = _mm256_abs_epi32(src_line);
    accum = _mm256_add_epi32(accum, abs);
  }

  {  // 32 bit horizontal add
    const __m256i a = _mm256_srli_si256(accum, 8);
    const __m256i b = _mm256_add_epi32(accum, a);
    const __m256i c = _mm256_srli_epi64(b, 32);
    const __m256i d = _mm256_add_epi32(b, c);
    const __m128i accum_128 = _mm_add_epi32(_mm256_castsi256_si128(d),
                                            _mm256_extractf128_si256(d, 1));
    return _mm_cvtsi128_si32(accum_128);
  }
}

int aom_satd_lp_avx2(const int16_t *coeff, int length) {
  const __m256i one = _mm256_set1_epi16(1);
  __m256i accum = _mm256_setzero_si256();

  for (int i = 0; i < length; i += 16) {
    const __m256i src_line = _mm256_loadu_si256((const __m256i *)coeff);
    const __m256i abs = _mm256_abs_epi16(src_line);
    const __m256i sum = _mm256_madd_epi16(abs, one);
    accum = _mm256_add_epi32(accum, sum);
    coeff += 16;
  }

  {  // 32 bit horizontal add
    const __m256i a = _mm256_srli_si256(accum, 8);
    const __m256i b = _mm256_add_epi32(accum, a);
    const __m256i c = _mm256_srli_epi64(b, 32);
    const __m256i d = _mm256_add_epi32(b, c);
    const __m128i accum_128 = _mm_add_epi32(_mm256_castsi256_si128(d),
                                            _mm256_extractf128_si256(d, 1));
    return _mm_cvtsi128_si32(accum_128);
  }
}

static INLINE __m256i calc_avg_8x8_dual_avx2(const uint8_t *s, int p) {
  const __m256i s0 =
      _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *)(s)));
  const __m256i s1 =
      _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *)(s + p)));
  const __m256i s2 =
      _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *)(s + 2 * p)));
  const __m256i s3 =
      _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *)(s + 3 * p)));
  const __m256i sum0 =
      _mm256_add_epi16(_mm256_add_epi16(s0, s1), _mm256_add_epi16(s2, s3));
  const __m256i s4 =
      _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *)(s + 4 * p)));
  const __m256i s5 =
      _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *)(s + 5 * p)));
  const __m256i s6 =
      _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *)(s + 6 * p)));
  const __m256i s7 =
      _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *)(s + 7 * p)));
  const __m256i sum1 =
      _mm256_add_epi16(_mm256_add_epi16(s4, s5), _mm256_add_epi16(s6, s7));

  // The result of two 8x8 sub-blocks in 16x16 block.
  return _mm256_add_epi16(sum0, sum1);
}

void aom_avg_8x8_quad_avx2(const uint8_t *s, int p, int x16_idx, int y16_idx,
                           int *avg) {
  // Process 1st and 2nd 8x8 sub-blocks in a 16x16 block.
  const uint8_t *s_tmp = s + y16_idx * p + x16_idx;
  __m256i result_0 = calc_avg_8x8_dual_avx2(s_tmp, p);

  // Process 3rd and 4th 8x8 sub-blocks in a 16x16 block.
  s_tmp = s + ((y16_idx + 8) * p) + x16_idx;
  __m256i result_1 = calc_avg_8x8_dual_avx2(s_tmp, p);

  const __m256i constant_32 = _mm256_set1_epi16(32);
  result_0 = _mm256_hadd_epi16(result_0, result_1);
  result_1 = _mm256_adds_epu16(result_0, _mm256_srli_si256(result_0, 4));
  result_0 = _mm256_adds_epu16(result_1, _mm256_srli_si256(result_1, 2));
  result_0 = _mm256_adds_epu16(result_0, constant_32);
  result_0 = _mm256_srli_epi16(result_0, 6);
  avg[0] = _mm_extract_epi16(_mm256_castsi256_si128(result_0), 0);
  avg[1] = _mm_extract_epi16(_mm256_extracti128_si256(result_0, 1), 0);
  avg[2] = _mm_extract_epi16(_mm256_castsi256_si128(result_0), 4);
  avg[3] = _mm_extract_epi16(_mm256_extracti128_si256(result_0, 1), 4);
}

void aom_int_pro_row_avx2(int16_t *hbuf, const uint8_t *ref,
                          const int ref_stride, const int width,
                          const int height, int norm_factor) {
  // SIMD implementation assumes width and height to be multiple of 16 and 2
  // respectively. For any odd width or height, SIMD support needs to be added.
  assert(width % 16 == 0 && height % 2 == 0);

  if (width % 32 == 0) {
    const __m256i zero = _mm256_setzero_si256();
    for (int wd = 0; wd < width; wd += 32) {
      const uint8_t *ref_tmp = ref + wd;
      int16_t *hbuf_tmp = hbuf + wd;
      __m256i s0 = zero;
      __m256i s1 = zero;
      int idx = 0;
      do {
        __m256i src_line = _mm256_loadu_si256((const __m256i *)ref_tmp);
        __m256i t0 = _mm256_unpacklo_epi8(src_line, zero);
        __m256i t1 = _mm256_unpackhi_epi8(src_line, zero);
        s0 = _mm256_adds_epu16(s0, t0);
        s1 = _mm256_adds_epu16(s1, t1);
        ref_tmp += ref_stride;

        src_line = _mm256_loadu_si256((const __m256i *)ref_tmp);
        t0 = _mm256_unpacklo_epi8(src_line, zero);
        t1 = _mm256_unpackhi_epi8(src_line, zero);
        s0 = _mm256_adds_epu16(s0, t0);
        s1 = _mm256_adds_epu16(s1, t1);
        ref_tmp += ref_stride;
        idx += 2;
      } while (idx < height);
      s0 = _mm256_srai_epi16(s0, norm_factor);
      s1 = _mm256_srai_epi16(s1, norm_factor);
      _mm_storeu_si128((__m128i *)(hbuf_tmp), _mm256_castsi256_si128(s0));
      _mm_storeu_si128((__m128i *)(hbuf_tmp + 8), _mm256_castsi256_si128(s1));
      _mm_storeu_si128((__m128i *)(hbuf_tmp + 16),
                       _mm256_extractf128_si256(s0, 1));
      _mm_storeu_si128((__m128i *)(hbuf_tmp + 24),
                       _mm256_extractf128_si256(s1, 1));
    }
  } else if (width % 16 == 0) {
    aom_int_pro_row_sse2(hbuf, ref, ref_stride, width, height, norm_factor);
  }
}

void aom_int_pro_col_avx2(int16_t *vbuf, const uint8_t *ref,
                          const int ref_stride, const int width,
                          const int height, int norm_factor) {
  // SIMD implementation assumes width to be multiple of 16. For any odd width,
  // SIMD support needs to be added.
  assert(width % 16 == 0);

  if (width == 128) {
    const __m256i zero = _mm256_setzero_si256();
    for (int ht = 0; ht < height; ++ht) {
      const __m256i src_line0 = _mm256_loadu_si256((const __m256i *)ref);
      const __m256i src_line1 = _mm256_loadu_si256((const __m256i *)(ref + 32));
      const __m256i src_line2 = _mm256_loadu_si256((const __m256i *)(ref + 64));
      const __m256i src_line3 = _mm256_loadu_si256((const __m256i *)(ref + 96));
      const __m256i s0 = _mm256_sad_epu8(src_line0, zero);
      const __m256i s1 = _mm256_sad_epu8(src_line1, zero);
      const __m256i s2 = _mm256_sad_epu8(src_line2, zero);
      const __m256i s3 = _mm256_sad_epu8(src_line3, zero);
      const __m256i result0_256bit = _mm256_adds_epu16(s0, s1);
      const __m256i result1_256bit = _mm256_adds_epu16(s2, s3);
      const __m256i result_256bit =
          _mm256_adds_epu16(result0_256bit, result1_256bit);

      const __m128i result =
          _mm_adds_epu16(_mm256_castsi256_si128(result_256bit),
                         _mm256_extractf128_si256(result_256bit, 1));
      __m128i result1 = _mm_adds_epu16(result, _mm_srli_si128(result, 8));
      vbuf[ht] = _mm_extract_epi16(result1, 0) >> norm_factor;
      ref += ref_stride;
    }
  } else if (width == 64) {
    const __m256i zero = _mm256_setzero_si256();
    for (int ht = 0; ht < height; ++ht) {
      const __m256i src_line0 = _mm256_loadu_si256((const __m256i *)ref);
      const __m256i src_line1 = _mm256_loadu_si256((const __m256i *)(ref + 32));
      const __m256i s1 = _mm256_sad_epu8(src_line0, zero);
      const __m256i s2 = _mm256_sad_epu8(src_line1, zero);
      const __m256i result_256bit = _mm256_adds_epu16(s1, s2);

      const __m128i result =
          _mm_adds_epu16(_mm256_castsi256_si128(result_256bit),
                         _mm256_extractf128_si256(result_256bit, 1));
      __m128i result1 = _mm_adds_epu16(result, _mm_srli_si128(result, 8));
      vbuf[ht] = _mm_extract_epi16(result1, 0) >> norm_factor;
      ref += ref_stride;
    }
  } else if (width == 32) {
    assert(height % 2 == 0);
    const __m256i zero = _mm256_setzero_si256();
    for (int ht = 0; ht < height; ht += 2) {
      const __m256i src_line0 = _mm256_loadu_si256((const __m256i *)ref);
      const __m256i src_line1 =
          _mm256_loadu_si256((const __m256i *)(ref + ref_stride));
      const __m256i s0 = _mm256_sad_epu8(src_line0, zero);
      const __m256i s1 = _mm256_sad_epu8(src_line1, zero);

      __m128i result0 = _mm_adds_epu16(_mm256_castsi256_si128(s0),
                                       _mm256_extractf128_si256(s0, 1));
      __m128i result1 = _mm_adds_epu16(_mm256_castsi256_si128(s1),
                                       _mm256_extractf128_si256(s1, 1));
      __m128i result2 = _mm_adds_epu16(result0, _mm_srli_si128(result0, 8));
      __m128i result3 = _mm_adds_epu16(result1, _mm_srli_si128(result1, 8));

      vbuf[ht] = _mm_extract_epi16(result2, 0) >> norm_factor;
      vbuf[ht + 1] = _mm_extract_epi16(result3, 0) >> norm_factor;
      ref += (2 * ref_stride);
    }
  } else if (width == 16) {
    aom_int_pro_col_sse2(vbuf, ref, ref_stride, width, height, norm_factor);
  }
}

static inline void calc_vector_mean_sse_64wd(const int16_t *ref,
                                             const int16_t *src, __m256i *mean,
                                             __m256i *sse) {
  const __m256i src_line0 = _mm256_loadu_si256((const __m256i *)src);
  const __m256i src_line1 = _mm256_loadu_si256((const __m256i *)(src + 16));
  const __m256i src_line2 = _mm256_loadu_si256((const __m256i *)(src + 32));
  const __m256i src_line3 = _mm256_loadu_si256((const __m256i *)(src + 48));
  const __m256i ref_line0 = _mm256_loadu_si256((const __m256i *)ref);
  const __m256i ref_line1 = _mm256_loadu_si256((const __m256i *)(ref + 16));
  const __m256i ref_line2 = _mm256_loadu_si256((const __m256i *)(ref + 32));
  const __m256i ref_line3 = _mm256_loadu_si256((const __m256i *)(ref + 48));

  const __m256i diff0 = _mm256_sub_epi16(ref_line0, src_line0);
  const __m256i diff1 = _mm256_sub_epi16(ref_line1, src_line1);
  const __m256i diff2 = _mm256_sub_epi16(ref_line2, src_line2);
  const __m256i diff3 = _mm256_sub_epi16(ref_line3, src_line3);
  const __m256i diff_sqr0 = _mm256_madd_epi16(diff0, diff0);
  const __m256i diff_sqr1 = _mm256_madd_epi16(diff1, diff1);
  const __m256i diff_sqr2 = _mm256_madd_epi16(diff2, diff2);
  const __m256i diff_sqr3 = _mm256_madd_epi16(diff3, diff3);

  *mean = _mm256_add_epi16(*mean, _mm256_add_epi16(diff0, diff1));
  *mean = _mm256_add_epi16(*mean, diff2);
  *mean = _mm256_add_epi16(*mean, diff3);
  *sse = _mm256_add_epi32(*sse, _mm256_add_epi32(diff_sqr0, diff_sqr1));
  *sse = _mm256_add_epi32(*sse, diff_sqr2);
  *sse = _mm256_add_epi32(*sse, diff_sqr3);
}

#define CALC_VAR_FROM_MEAN_SSE(mean, sse)                                    \
  {                                                                          \
    mean = _mm256_madd_epi16(mean, _mm256_set1_epi16(1));                    \
    mean = _mm256_hadd_epi32(mean, sse);                                     \
    mean = _mm256_add_epi32(mean, _mm256_bsrli_epi128(mean, 4));             \
    const __m128i result = _mm_add_epi32(_mm256_castsi256_si128(mean),       \
                                         _mm256_extractf128_si256(mean, 1)); \
    /*(mean * mean): dynamic range 31 bits.*/                                \
    const int mean_int = _mm_extract_epi32(result, 0);                       \
    const int sse_int = _mm_extract_epi32(result, 2);                        \
    const unsigned int mean_abs = abs(mean_int);                             \
    var = sse_int - ((mean_abs * mean_abs) >> (bwl + 2));                    \
  }

// ref: [0 - 510]
// src: [0 - 510]
// bwl: {2, 3, 4, 5}
int aom_vector_var_avx2(const int16_t *ref, const int16_t *src, int bwl) {
  const int width = 4 << bwl;
  assert(width % 16 == 0 && width <= 128);
  int var = 0;

  // Instead of having a loop over width 16, considered loop unrolling to avoid
  // some addition operations.
  if (width == 128) {
    __m256i mean = _mm256_setzero_si256();
    __m256i sse = _mm256_setzero_si256();

    calc_vector_mean_sse_64wd(src, ref, &mean, &sse);
    calc_vector_mean_sse_64wd(src + 64, ref + 64, &mean, &sse);
    CALC_VAR_FROM_MEAN_SSE(mean, sse)
  } else if (width == 64) {
    __m256i mean = _mm256_setzero_si256();
    __m256i sse = _mm256_setzero_si256();

    calc_vector_mean_sse_64wd(src, ref, &mean, &sse);
    CALC_VAR_FROM_MEAN_SSE(mean, sse)
  } else if (width == 32) {
    const __m256i src_line0 = _mm256_loadu_si256((const __m256i *)src);
    const __m256i ref_line0 = _mm256_loadu_si256((const __m256i *)ref);
    const __m256i src_line1 = _mm256_loadu_si256((const __m256i *)(src + 16));
    const __m256i ref_line1 = _mm256_loadu_si256((const __m256i *)(ref + 16));

    const __m256i diff0 = _mm256_sub_epi16(ref_line0, src_line0);
    const __m256i diff1 = _mm256_sub_epi16(ref_line1, src_line1);
    const __m256i diff_sqr0 = _mm256_madd_epi16(diff0, diff0);
    const __m256i diff_sqr1 = _mm256_madd_epi16(diff1, diff1);
    const __m256i sse = _mm256_add_epi32(diff_sqr0, diff_sqr1);
    __m256i mean = _mm256_add_epi16(diff0, diff1);

    CALC_VAR_FROM_MEAN_SSE(mean, sse)
  } else if (width == 16) {
    const __m256i src_line = _mm256_loadu_si256((const __m256i *)src);
    const __m256i ref_line = _mm256_loadu_si256((const __m256i *)ref);
    __m256i mean = _mm256_sub_epi16(ref_line, src_line);
    const __m256i sse = _mm256_madd_epi16(mean, mean);

    CALC_VAR_FROM_MEAN_SSE(mean, sse)
  }
  return var;
}
