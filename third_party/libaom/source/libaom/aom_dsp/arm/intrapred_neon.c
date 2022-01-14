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

#include <arm_neon.h>

#include "config/aom_config.h"
#include "config/aom_dsp_rtcd.h"

#include "aom/aom_integer.h"

//------------------------------------------------------------------------------
// DC 4x4

// 'do_above' and 'do_left' facilitate branch removal when inlined.
static INLINE void dc_4x4(uint8_t *dst, ptrdiff_t stride, const uint8_t *above,
                          const uint8_t *left, int do_above, int do_left) {
  uint16x8_t sum_top;
  uint16x8_t sum_left;
  uint8x8_t dc0;

  if (do_above) {
    const uint8x8_t A = vld1_u8(above);  // top row
    const uint16x4_t p0 = vpaddl_u8(A);  // cascading summation of the top
    const uint16x4_t p1 = vpadd_u16(p0, p0);
    sum_top = vcombine_u16(p1, p1);
  }

  if (do_left) {
    const uint8x8_t L = vld1_u8(left);   // left border
    const uint16x4_t p0 = vpaddl_u8(L);  // cascading summation of the left
    const uint16x4_t p1 = vpadd_u16(p0, p0);
    sum_left = vcombine_u16(p1, p1);
  }

  if (do_above && do_left) {
    const uint16x8_t sum = vaddq_u16(sum_left, sum_top);
    dc0 = vrshrn_n_u16(sum, 3);
  } else if (do_above) {
    dc0 = vrshrn_n_u16(sum_top, 2);
  } else if (do_left) {
    dc0 = vrshrn_n_u16(sum_left, 2);
  } else {
    dc0 = vdup_n_u8(0x80);
  }

  {
    const uint8x8_t dc = vdup_lane_u8(dc0, 0);
    int i;
    for (i = 0; i < 4; ++i) {
      vst1_lane_u32((uint32_t *)(dst + i * stride), vreinterpret_u32_u8(dc), 0);
    }
  }
}

void aom_dc_predictor_4x4_neon(uint8_t *dst, ptrdiff_t stride,
                               const uint8_t *above, const uint8_t *left) {
  dc_4x4(dst, stride, above, left, 1, 1);
}

void aom_dc_left_predictor_4x4_neon(uint8_t *dst, ptrdiff_t stride,
                                    const uint8_t *above, const uint8_t *left) {
  (void)above;
  dc_4x4(dst, stride, NULL, left, 0, 1);
}

void aom_dc_top_predictor_4x4_neon(uint8_t *dst, ptrdiff_t stride,
                                   const uint8_t *above, const uint8_t *left) {
  (void)left;
  dc_4x4(dst, stride, above, NULL, 1, 0);
}

void aom_dc_128_predictor_4x4_neon(uint8_t *dst, ptrdiff_t stride,
                                   const uint8_t *above, const uint8_t *left) {
  (void)above;
  (void)left;
  dc_4x4(dst, stride, NULL, NULL, 0, 0);
}

//------------------------------------------------------------------------------
// DC 8x8

// 'do_above' and 'do_left' facilitate branch removal when inlined.
static INLINE void dc_8x8(uint8_t *dst, ptrdiff_t stride, const uint8_t *above,
                          const uint8_t *left, int do_above, int do_left) {
  uint16x8_t sum_top;
  uint16x8_t sum_left;
  uint8x8_t dc0;

  if (do_above) {
    const uint8x8_t A = vld1_u8(above);  // top row
    const uint16x4_t p0 = vpaddl_u8(A);  // cascading summation of the top
    const uint16x4_t p1 = vpadd_u16(p0, p0);
    const uint16x4_t p2 = vpadd_u16(p1, p1);
    sum_top = vcombine_u16(p2, p2);
  }

  if (do_left) {
    const uint8x8_t L = vld1_u8(left);   // left border
    const uint16x4_t p0 = vpaddl_u8(L);  // cascading summation of the left
    const uint16x4_t p1 = vpadd_u16(p0, p0);
    const uint16x4_t p2 = vpadd_u16(p1, p1);
    sum_left = vcombine_u16(p2, p2);
  }

  if (do_above && do_left) {
    const uint16x8_t sum = vaddq_u16(sum_left, sum_top);
    dc0 = vrshrn_n_u16(sum, 4);
  } else if (do_above) {
    dc0 = vrshrn_n_u16(sum_top, 3);
  } else if (do_left) {
    dc0 = vrshrn_n_u16(sum_left, 3);
  } else {
    dc0 = vdup_n_u8(0x80);
  }

  {
    const uint8x8_t dc = vdup_lane_u8(dc0, 0);
    int i;
    for (i = 0; i < 8; ++i) {
      vst1_u32((uint32_t *)(dst + i * stride), vreinterpret_u32_u8(dc));
    }
  }
}

void aom_dc_predictor_8x8_neon(uint8_t *dst, ptrdiff_t stride,
                               const uint8_t *above, const uint8_t *left) {
  dc_8x8(dst, stride, above, left, 1, 1);
}

void aom_dc_left_predictor_8x8_neon(uint8_t *dst, ptrdiff_t stride,
                                    const uint8_t *above, const uint8_t *left) {
  (void)above;
  dc_8x8(dst, stride, NULL, left, 0, 1);
}

void aom_dc_top_predictor_8x8_neon(uint8_t *dst, ptrdiff_t stride,
                                   const uint8_t *above, const uint8_t *left) {
  (void)left;
  dc_8x8(dst, stride, above, NULL, 1, 0);
}

void aom_dc_128_predictor_8x8_neon(uint8_t *dst, ptrdiff_t stride,
                                   const uint8_t *above, const uint8_t *left) {
  (void)above;
  (void)left;
  dc_8x8(dst, stride, NULL, NULL, 0, 0);
}

//------------------------------------------------------------------------------
// DC 16x16

// 'do_above' and 'do_left' facilitate branch removal when inlined.
static INLINE void dc_16x16(uint8_t *dst, ptrdiff_t stride,
                            const uint8_t *above, const uint8_t *left,
                            int do_above, int do_left) {
  uint16x8_t sum_top;
  uint16x8_t sum_left;
  uint8x8_t dc0;

  if (do_above) {
    const uint8x16_t A = vld1q_u8(above);  // top row
    const uint16x8_t p0 = vpaddlq_u8(A);   // cascading summation of the top
    const uint16x4_t p1 = vadd_u16(vget_low_u16(p0), vget_high_u16(p0));
    const uint16x4_t p2 = vpadd_u16(p1, p1);
    const uint16x4_t p3 = vpadd_u16(p2, p2);
    sum_top = vcombine_u16(p3, p3);
  }

  if (do_left) {
    const uint8x16_t L = vld1q_u8(left);  // left row
    const uint16x8_t p0 = vpaddlq_u8(L);  // cascading summation of the left
    const uint16x4_t p1 = vadd_u16(vget_low_u16(p0), vget_high_u16(p0));
    const uint16x4_t p2 = vpadd_u16(p1, p1);
    const uint16x4_t p3 = vpadd_u16(p2, p2);
    sum_left = vcombine_u16(p3, p3);
  }

  if (do_above && do_left) {
    const uint16x8_t sum = vaddq_u16(sum_left, sum_top);
    dc0 = vrshrn_n_u16(sum, 5);
  } else if (do_above) {
    dc0 = vrshrn_n_u16(sum_top, 4);
  } else if (do_left) {
    dc0 = vrshrn_n_u16(sum_left, 4);
  } else {
    dc0 = vdup_n_u8(0x80);
  }

  {
    const uint8x16_t dc = vdupq_lane_u8(dc0, 0);
    int i;
    for (i = 0; i < 16; ++i) {
      vst1q_u8(dst + i * stride, dc);
    }
  }
}

void aom_dc_predictor_16x16_neon(uint8_t *dst, ptrdiff_t stride,
                                 const uint8_t *above, const uint8_t *left) {
  dc_16x16(dst, stride, above, left, 1, 1);
}

void aom_dc_left_predictor_16x16_neon(uint8_t *dst, ptrdiff_t stride,
                                      const uint8_t *above,
                                      const uint8_t *left) {
  (void)above;
  dc_16x16(dst, stride, NULL, left, 0, 1);
}

void aom_dc_top_predictor_16x16_neon(uint8_t *dst, ptrdiff_t stride,
                                     const uint8_t *above,
                                     const uint8_t *left) {
  (void)left;
  dc_16x16(dst, stride, above, NULL, 1, 0);
}

void aom_dc_128_predictor_16x16_neon(uint8_t *dst, ptrdiff_t stride,
                                     const uint8_t *above,
                                     const uint8_t *left) {
  (void)above;
  (void)left;
  dc_16x16(dst, stride, NULL, NULL, 0, 0);
}

//------------------------------------------------------------------------------
// DC 32x32

// 'do_above' and 'do_left' facilitate branch removal when inlined.
static INLINE void dc_32x32(uint8_t *dst, ptrdiff_t stride,
                            const uint8_t *above, const uint8_t *left,
                            int do_above, int do_left) {
  uint16x8_t sum_top;
  uint16x8_t sum_left;
  uint8x8_t dc0;

  if (do_above) {
    const uint8x16_t A0 = vld1q_u8(above);  // top row
    const uint8x16_t A1 = vld1q_u8(above + 16);
    const uint16x8_t p0 = vpaddlq_u8(A0);  // cascading summation of the top
    const uint16x8_t p1 = vpaddlq_u8(A1);
    const uint16x8_t p2 = vaddq_u16(p0, p1);
    const uint16x4_t p3 = vadd_u16(vget_low_u16(p2), vget_high_u16(p2));
    const uint16x4_t p4 = vpadd_u16(p3, p3);
    const uint16x4_t p5 = vpadd_u16(p4, p4);
    sum_top = vcombine_u16(p5, p5);
  }

  if (do_left) {
    const uint8x16_t L0 = vld1q_u8(left);  // left row
    const uint8x16_t L1 = vld1q_u8(left + 16);
    const uint16x8_t p0 = vpaddlq_u8(L0);  // cascading summation of the left
    const uint16x8_t p1 = vpaddlq_u8(L1);
    const uint16x8_t p2 = vaddq_u16(p0, p1);
    const uint16x4_t p3 = vadd_u16(vget_low_u16(p2), vget_high_u16(p2));
    const uint16x4_t p4 = vpadd_u16(p3, p3);
    const uint16x4_t p5 = vpadd_u16(p4, p4);
    sum_left = vcombine_u16(p5, p5);
  }

  if (do_above && do_left) {
    const uint16x8_t sum = vaddq_u16(sum_left, sum_top);
    dc0 = vrshrn_n_u16(sum, 6);
  } else if (do_above) {
    dc0 = vrshrn_n_u16(sum_top, 5);
  } else if (do_left) {
    dc0 = vrshrn_n_u16(sum_left, 5);
  } else {
    dc0 = vdup_n_u8(0x80);
  }

  {
    const uint8x16_t dc = vdupq_lane_u8(dc0, 0);
    int i;
    for (i = 0; i < 32; ++i) {
      vst1q_u8(dst + i * stride, dc);
      vst1q_u8(dst + i * stride + 16, dc);
    }
  }
}

void aom_dc_predictor_32x32_neon(uint8_t *dst, ptrdiff_t stride,
                                 const uint8_t *above, const uint8_t *left) {
  dc_32x32(dst, stride, above, left, 1, 1);
}

void aom_dc_left_predictor_32x32_neon(uint8_t *dst, ptrdiff_t stride,
                                      const uint8_t *above,
                                      const uint8_t *left) {
  (void)above;
  dc_32x32(dst, stride, NULL, left, 0, 1);
}

void aom_dc_top_predictor_32x32_neon(uint8_t *dst, ptrdiff_t stride,
                                     const uint8_t *above,
                                     const uint8_t *left) {
  (void)left;
  dc_32x32(dst, stride, above, NULL, 1, 0);
}

void aom_dc_128_predictor_32x32_neon(uint8_t *dst, ptrdiff_t stride,
                                     const uint8_t *above,
                                     const uint8_t *left) {
  (void)above;
  (void)left;
  dc_32x32(dst, stride, NULL, NULL, 0, 0);
}

// -----------------------------------------------------------------------------

void aom_d135_predictor_4x4_neon(uint8_t *dst, ptrdiff_t stride,
                                 const uint8_t *above, const uint8_t *left) {
  const uint8x8_t XABCD_u8 = vld1_u8(above - 1);
  const uint64x1_t XABCD = vreinterpret_u64_u8(XABCD_u8);
  const uint64x1_t ____XABC = vshl_n_u64(XABCD, 32);
  const uint32x2_t zero = vdup_n_u32(0);
  const uint32x2_t IJKL = vld1_lane_u32((const uint32_t *)left, zero, 0);
  const uint8x8_t IJKL_u8 = vreinterpret_u8_u32(IJKL);
  const uint64x1_t LKJI____ = vreinterpret_u64_u8(vrev32_u8(IJKL_u8));
  const uint64x1_t LKJIXABC = vorr_u64(LKJI____, ____XABC);
  const uint8x8_t KJIXABC_ = vreinterpret_u8_u64(vshr_n_u64(LKJIXABC, 8));
  const uint8x8_t JIXABC__ = vreinterpret_u8_u64(vshr_n_u64(LKJIXABC, 16));
  const uint8_t D = vget_lane_u8(XABCD_u8, 4);
  const uint8x8_t JIXABCD_ = vset_lane_u8(D, JIXABC__, 6);
  const uint8x8_t LKJIXABC_u8 = vreinterpret_u8_u64(LKJIXABC);
  const uint8x8_t avg1 = vhadd_u8(JIXABCD_, LKJIXABC_u8);
  const uint8x8_t avg2 = vrhadd_u8(avg1, KJIXABC_);
  const uint64x1_t avg2_u64 = vreinterpret_u64_u8(avg2);
  const uint32x2_t r3 = vreinterpret_u32_u8(avg2);
  const uint32x2_t r2 = vreinterpret_u32_u64(vshr_n_u64(avg2_u64, 8));
  const uint32x2_t r1 = vreinterpret_u32_u64(vshr_n_u64(avg2_u64, 16));
  const uint32x2_t r0 = vreinterpret_u32_u64(vshr_n_u64(avg2_u64, 24));
  vst1_lane_u32((uint32_t *)(dst + 0 * stride), r0, 0);
  vst1_lane_u32((uint32_t *)(dst + 1 * stride), r1, 0);
  vst1_lane_u32((uint32_t *)(dst + 2 * stride), r2, 0);
  vst1_lane_u32((uint32_t *)(dst + 3 * stride), r3, 0);
}

void aom_v_predictor_4x4_neon(uint8_t *dst, ptrdiff_t stride,
                              const uint8_t *above, const uint8_t *left) {
  int i;
  uint32x2_t d0u32 = vdup_n_u32(0);
  (void)left;

  d0u32 = vld1_lane_u32((const uint32_t *)above, d0u32, 0);
  for (i = 0; i < 4; i++, dst += stride)
    vst1_lane_u32((uint32_t *)dst, d0u32, 0);
}

void aom_v_predictor_8x8_neon(uint8_t *dst, ptrdiff_t stride,
                              const uint8_t *above, const uint8_t *left) {
  int i;
  uint8x8_t d0u8 = vdup_n_u8(0);
  (void)left;

  d0u8 = vld1_u8(above);
  for (i = 0; i < 8; i++, dst += stride) vst1_u8(dst, d0u8);
}

void aom_v_predictor_16x16_neon(uint8_t *dst, ptrdiff_t stride,
                                const uint8_t *above, const uint8_t *left) {
  int i;
  uint8x16_t q0u8 = vdupq_n_u8(0);
  (void)left;

  q0u8 = vld1q_u8(above);
  for (i = 0; i < 16; i++, dst += stride) vst1q_u8(dst, q0u8);
}

void aom_v_predictor_32x32_neon(uint8_t *dst, ptrdiff_t stride,
                                const uint8_t *above, const uint8_t *left) {
  int i;
  uint8x16_t q0u8 = vdupq_n_u8(0);
  uint8x16_t q1u8 = vdupq_n_u8(0);
  (void)left;

  q0u8 = vld1q_u8(above);
  q1u8 = vld1q_u8(above + 16);
  for (i = 0; i < 32; i++, dst += stride) {
    vst1q_u8(dst, q0u8);
    vst1q_u8(dst + 16, q1u8);
  }
}

void aom_h_predictor_4x4_neon(uint8_t *dst, ptrdiff_t stride,
                              const uint8_t *above, const uint8_t *left) {
  uint8x8_t d0u8 = vdup_n_u8(0);
  uint32x2_t d1u32 = vdup_n_u32(0);
  (void)above;

  d1u32 = vld1_lane_u32((const uint32_t *)left, d1u32, 0);

  d0u8 = vdup_lane_u8(vreinterpret_u8_u32(d1u32), 0);
  vst1_lane_u32((uint32_t *)dst, vreinterpret_u32_u8(d0u8), 0);
  dst += stride;
  d0u8 = vdup_lane_u8(vreinterpret_u8_u32(d1u32), 1);
  vst1_lane_u32((uint32_t *)dst, vreinterpret_u32_u8(d0u8), 0);
  dst += stride;
  d0u8 = vdup_lane_u8(vreinterpret_u8_u32(d1u32), 2);
  vst1_lane_u32((uint32_t *)dst, vreinterpret_u32_u8(d0u8), 0);
  dst += stride;
  d0u8 = vdup_lane_u8(vreinterpret_u8_u32(d1u32), 3);
  vst1_lane_u32((uint32_t *)dst, vreinterpret_u32_u8(d0u8), 0);
}

void aom_h_predictor_8x8_neon(uint8_t *dst, ptrdiff_t stride,
                              const uint8_t *above, const uint8_t *left) {
  uint8x8_t d0u8 = vdup_n_u8(0);
  uint64x1_t d1u64 = vdup_n_u64(0);
  (void)above;

  d1u64 = vld1_u64((const uint64_t *)left);

  d0u8 = vdup_lane_u8(vreinterpret_u8_u64(d1u64), 0);
  vst1_u8(dst, d0u8);
  dst += stride;
  d0u8 = vdup_lane_u8(vreinterpret_u8_u64(d1u64), 1);
  vst1_u8(dst, d0u8);
  dst += stride;
  d0u8 = vdup_lane_u8(vreinterpret_u8_u64(d1u64), 2);
  vst1_u8(dst, d0u8);
  dst += stride;
  d0u8 = vdup_lane_u8(vreinterpret_u8_u64(d1u64), 3);
  vst1_u8(dst, d0u8);
  dst += stride;
  d0u8 = vdup_lane_u8(vreinterpret_u8_u64(d1u64), 4);
  vst1_u8(dst, d0u8);
  dst += stride;
  d0u8 = vdup_lane_u8(vreinterpret_u8_u64(d1u64), 5);
  vst1_u8(dst, d0u8);
  dst += stride;
  d0u8 = vdup_lane_u8(vreinterpret_u8_u64(d1u64), 6);
  vst1_u8(dst, d0u8);
  dst += stride;
  d0u8 = vdup_lane_u8(vreinterpret_u8_u64(d1u64), 7);
  vst1_u8(dst, d0u8);
}

void aom_h_predictor_16x16_neon(uint8_t *dst, ptrdiff_t stride,
                                const uint8_t *above, const uint8_t *left) {
  int j;
  uint8x8_t d2u8 = vdup_n_u8(0);
  uint8x16_t q0u8 = vdupq_n_u8(0);
  uint8x16_t q1u8 = vdupq_n_u8(0);
  (void)above;

  q1u8 = vld1q_u8(left);
  d2u8 = vget_low_u8(q1u8);
  for (j = 0; j < 2; j++, d2u8 = vget_high_u8(q1u8)) {
    q0u8 = vdupq_lane_u8(d2u8, 0);
    vst1q_u8(dst, q0u8);
    dst += stride;
    q0u8 = vdupq_lane_u8(d2u8, 1);
    vst1q_u8(dst, q0u8);
    dst += stride;
    q0u8 = vdupq_lane_u8(d2u8, 2);
    vst1q_u8(dst, q0u8);
    dst += stride;
    q0u8 = vdupq_lane_u8(d2u8, 3);
    vst1q_u8(dst, q0u8);
    dst += stride;
    q0u8 = vdupq_lane_u8(d2u8, 4);
    vst1q_u8(dst, q0u8);
    dst += stride;
    q0u8 = vdupq_lane_u8(d2u8, 5);
    vst1q_u8(dst, q0u8);
    dst += stride;
    q0u8 = vdupq_lane_u8(d2u8, 6);
    vst1q_u8(dst, q0u8);
    dst += stride;
    q0u8 = vdupq_lane_u8(d2u8, 7);
    vst1q_u8(dst, q0u8);
    dst += stride;
  }
}

void aom_h_predictor_32x32_neon(uint8_t *dst, ptrdiff_t stride,
                                const uint8_t *above, const uint8_t *left) {
  int j, k;
  uint8x8_t d2u8 = vdup_n_u8(0);
  uint8x16_t q0u8 = vdupq_n_u8(0);
  uint8x16_t q1u8 = vdupq_n_u8(0);
  (void)above;

  for (k = 0; k < 2; k++, left += 16) {
    q1u8 = vld1q_u8(left);
    d2u8 = vget_low_u8(q1u8);
    for (j = 0; j < 2; j++, d2u8 = vget_high_u8(q1u8)) {
      q0u8 = vdupq_lane_u8(d2u8, 0);
      vst1q_u8(dst, q0u8);
      vst1q_u8(dst + 16, q0u8);
      dst += stride;
      q0u8 = vdupq_lane_u8(d2u8, 1);
      vst1q_u8(dst, q0u8);
      vst1q_u8(dst + 16, q0u8);
      dst += stride;
      q0u8 = vdupq_lane_u8(d2u8, 2);
      vst1q_u8(dst, q0u8);
      vst1q_u8(dst + 16, q0u8);
      dst += stride;
      q0u8 = vdupq_lane_u8(d2u8, 3);
      vst1q_u8(dst, q0u8);
      vst1q_u8(dst + 16, q0u8);
      dst += stride;
      q0u8 = vdupq_lane_u8(d2u8, 4);
      vst1q_u8(dst, q0u8);
      vst1q_u8(dst + 16, q0u8);
      dst += stride;
      q0u8 = vdupq_lane_u8(d2u8, 5);
      vst1q_u8(dst, q0u8);
      vst1q_u8(dst + 16, q0u8);
      dst += stride;
      q0u8 = vdupq_lane_u8(d2u8, 6);
      vst1q_u8(dst, q0u8);
      vst1q_u8(dst + 16, q0u8);
      dst += stride;
      q0u8 = vdupq_lane_u8(d2u8, 7);
      vst1q_u8(dst, q0u8);
      vst1q_u8(dst + 16, q0u8);
      dst += stride;
    }
  }
}

static INLINE void highbd_dc_predictor(uint16_t *dst, ptrdiff_t stride, int bw,
                                       const uint16_t *above,
                                       const uint16_t *left) {
  assert(bw >= 4);
  assert(IS_POWER_OF_TWO(bw));
  int expected_dc, sum = 0;
  const int count = bw * 2;
  uint32x4_t sum_q = vdupq_n_u32(0);
  uint32x2_t sum_d;
  uint16_t *dst_1;
  if (bw >= 8) {
    for (int i = 0; i < bw; i += 8) {
      sum_q = vpadalq_u16(sum_q, vld1q_u16(above));
      sum_q = vpadalq_u16(sum_q, vld1q_u16(left));
      above += 8;
      left += 8;
    }
    sum_d = vadd_u32(vget_low_u32(sum_q), vget_high_u32(sum_q));
    sum = vget_lane_s32(vreinterpret_s32_u64(vpaddl_u32(sum_d)), 0);
    expected_dc = (sum + (count >> 1)) / count;
    const uint16x8_t dc = vdupq_n_u16((uint16_t)expected_dc);
    for (int r = 0; r < bw; r++) {
      dst_1 = dst;
      for (int i = 0; i < bw; i += 8) {
        vst1q_u16(dst_1, dc);
        dst_1 += 8;
      }
      dst += stride;
    }
  } else {  // 4x4
    sum_q = vaddl_u16(vld1_u16(above), vld1_u16(left));
    sum_d = vadd_u32(vget_low_u32(sum_q), vget_high_u32(sum_q));
    sum = vget_lane_s32(vreinterpret_s32_u64(vpaddl_u32(sum_d)), 0);
    expected_dc = (sum + (count >> 1)) / count;
    const uint16x4_t dc = vdup_n_u16((uint16_t)expected_dc);
    for (int r = 0; r < bw; r++) {
      vst1_u16(dst, dc);
      dst += stride;
    }
  }
}

#define intra_pred_highbd_sized_neon(type, width)               \
  void aom_highbd_##type##_predictor_##width##x##width##_neon(  \
      uint16_t *dst, ptrdiff_t stride, const uint16_t *above,   \
      const uint16_t *left, int bd) {                           \
    (void)bd;                                                   \
    highbd_##type##_predictor(dst, stride, width, above, left); \
  }

#define intra_pred_square(type)           \
  intra_pred_highbd_sized_neon(type, 4);  \
  intra_pred_highbd_sized_neon(type, 8);  \
  intra_pred_highbd_sized_neon(type, 16); \
  intra_pred_highbd_sized_neon(type, 32); \
  intra_pred_highbd_sized_neon(type, 64);

intra_pred_square(dc);
#undef intra_pred_square

/* ---------------------P R E D I C T I O N   Z 1--------------------------- */

static DECLARE_ALIGNED(16, uint8_t, EvenOddMaskx[8][16]) = {
  { 0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15 },
  { 0, 1, 3, 5, 7, 9, 11, 13, 0, 2, 4, 6, 8, 10, 12, 14 },
  { 0, 0, 2, 4, 6, 8, 10, 12, 0, 0, 3, 5, 7, 9, 11, 13 },
  { 0, 0, 0, 3, 5, 7, 9, 11, 0, 0, 0, 4, 6, 8, 10, 12 },
  { 0, 0, 0, 0, 4, 6, 8, 10, 0, 0, 0, 0, 5, 7, 9, 11 },
  { 0, 0, 0, 0, 0, 5, 7, 9, 0, 0, 0, 0, 0, 6, 8, 10 },
  { 0, 0, 0, 0, 0, 0, 6, 8, 0, 0, 0, 0, 0, 0, 7, 9 },
  { 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 8 }
};

// Low bit depth functions
static DECLARE_ALIGNED(32, uint8_t, BaseMask[33][32]) = {
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,    0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0xff, 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,    0,    0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,    0,    0,    0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,    0,    0,    0,    0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,    0,    0,    0,    0,    0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,    0,    0,    0,    0,    0,    0,    0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0,
    0,    0,    0,    0,    0,    0,    0,    0,    0, 0, 0, 0, 0, 0, 0, 0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0, 0, 0, 0, 0, 0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0,    0,    0,    0,    0,    0,    0,    0,    0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0,    0,    0,    0,    0,    0,    0,    0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0,    0,    0,    0,    0,    0,    0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0,    0,    0,    0,    0,    0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0,    0,    0,    0,    0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0,    0,    0,    0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0,    0,    0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0,    0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
};

/* clang-format on */
static AOM_FORCE_INLINE void dr_prediction_z1_HxW_internal_neon_64(
    int H, int W, uint8x8_t *dst, const uint8_t *above, int upsample_above,
    int dx) {
  const int frac_bits = 6 - upsample_above;
  const int max_base_x = ((W + H) - 1) << upsample_above;

  assert(dx > 0);
  // pre-filter above pixels
  // store in temp buffers:
  //   above[x] * 32 + 16
  //   above[x+1] - above[x]
  // final pixels will be calculated as:
  //   (above[x] * 32 + 16 + (above[x+1] - above[x]) * shift) >> 5

  uint16x8_t a0, a1;
  uint16x8_t diff, a32;
  uint16x8_t a16;
  uint8x8_t a_mbase_x;

  a16 = vdupq_n_u16(16);
  a_mbase_x = vdup_n_u8(above[max_base_x]);
  uint16x8_t v_32 = vdupq_n_u16(32);
  int16x8_t v_upsample_above = vdupq_n_s16(upsample_above);
  uint16x8_t c3f = vdupq_n_u16(0x3f);

  int x = dx;
  for (int r = 0; r < W; r++) {
    uint16x8_t res;
    uint16x8_t shift;
    uint8x8x2_t v_tmp_a0_128;

    int base = x >> frac_bits;
    int base_max_diff = (max_base_x - base) >> upsample_above;
    if (base_max_diff <= 0) {
      for (int i = r; i < W; ++i) {
        dst[i] = a_mbase_x;  // save 4 values
      }
      return;
    }

    if (base_max_diff > H) base_max_diff = H;

    if (upsample_above) {
      v_tmp_a0_128 = vld2_u8(above + base);
      shift = vshrq_n_u16(
          vandq_u16(vshlq_u16(vdupq_n_u16(x), v_upsample_above), c3f), 1);
    } else {
      v_tmp_a0_128.val[0] = vld1_u8(above + base);
      v_tmp_a0_128.val[1] = vld1_u8(above + base + 1);
      shift = vshrq_n_u16(vandq_u16(vdupq_n_u16(x), c3f), 1);
    }
    a0 = vmovl_u8(v_tmp_a0_128.val[0]);
    a1 = vmovl_u8(v_tmp_a0_128.val[1]);
    diff = vsubq_u16(a1, a0);        // a[x+1] - a[x]
    a32 = vmlaq_u16(a16, a0, v_32);  // a[x] * 32 + 16
    res = vmlaq_u16(a32, diff, shift);

    uint8x8_t mask = vld1_u8(BaseMask[base_max_diff]);
    dst[r] =
        vorr_u8(vand_u8(mask, vshrn_n_u16(res, 5)), vbic_u8(a_mbase_x, mask));

    x += dx;
  }
}

static void dr_prediction_z1_4xN_neon(int N, uint8_t *dst, ptrdiff_t stride,
                                      const uint8_t *above, int upsample_above,
                                      int dx) {
  uint8x8_t dstvec[16];

  dr_prediction_z1_HxW_internal_neon_64(4, N, dstvec, above, upsample_above,
                                        dx);
  for (int i = 0; i < N; i++) {
    vst1_lane_u32((uint32_t *)(dst + stride * i),
                  vreinterpret_u32_u8(dstvec[i]), 0);
  }
}

static void dr_prediction_z1_8xN_neon(int N, uint8_t *dst, ptrdiff_t stride,
                                      const uint8_t *above, int upsample_above,
                                      int dx) {
  uint8x8_t dstvec[32];

  dr_prediction_z1_HxW_internal_neon_64(8, N, dstvec, above, upsample_above,
                                        dx);
  for (int i = 0; i < N; i++) {
    vst1_u8(dst + stride * i, dstvec[i]);
  }
}

static AOM_FORCE_INLINE void dr_prediction_z1_HxW_internal_neon(
    int H, int W, uint8x16_t *dst, const uint8_t *above, int upsample_above,
    int dx) {
  const int frac_bits = 6 - upsample_above;
  const int max_base_x = ((W + H) - 1) << upsample_above;

  assert(dx > 0);
  // pre-filter above pixels
  // store in temp buffers:
  //   above[x] * 32 + 16
  //   above[x+1] - above[x]
  // final pixels will be calculated as:
  //   (above[x] * 32 + 16 + (above[x+1] - above[x]) * shift) >> 5

  uint8x16x2_t a0, a1;
  uint16x8x2_t diff, a32;
  uint16x8_t a16, c3f;
  uint8x16_t a_mbase_x;

  a16 = vdupq_n_u16(16);
  a_mbase_x = vdupq_n_u8(above[max_base_x]);
  c3f = vdupq_n_u16(0x3f);
  uint16x8_t v_32 = vdupq_n_u16(32);
  uint8x16_t v_zero = vdupq_n_u8(0);
  int16x8_t v_upsample_above = vdupq_n_s16(upsample_above);

  int x = dx;
  for (int r = 0; r < W; r++) {
    uint16x8x2_t res;
    uint16x8_t shift;
    uint8x16_t a0_128, a1_128;

    int base = x >> frac_bits;
    int base_max_diff = (max_base_x - base) >> upsample_above;
    if (base_max_diff <= 0) {
      for (int i = r; i < W; ++i) {
        dst[i] = a_mbase_x;  // save 4 values
      }
      return;
    }

    if (base_max_diff > H) base_max_diff = H;

    if (upsample_above) {
      uint8x8x2_t v_tmp_a0_128 = vld2_u8(above + base);
      a0_128 = vcombine_u8(v_tmp_a0_128.val[0], v_tmp_a0_128.val[1]);
      a1_128 = vextq_u8(a0_128, v_zero, 8);
      shift = vshrq_n_u16(
          vandq_u16(vshlq_u16(vdupq_n_u16(x), v_upsample_above), c3f), 1);
    } else {
      a0_128 = vld1q_u8(above + base);
      a1_128 = vld1q_u8(above + base + 1);
      shift = vshrq_n_u16(vandq_u16(vdupq_n_u16(x), c3f), 1);
    }
    a0 = vzipq_u8(a0_128, v_zero);
    a1 = vzipq_u8(a1_128, v_zero);
    diff.val[0] = vsubq_u16(vreinterpretq_u16_u8(a1.val[0]),
                            vreinterpretq_u16_u8(a0.val[0]));  // a[x+1] - a[x]
    diff.val[1] = vsubq_u16(vreinterpretq_u16_u8(a1.val[1]),
                            vreinterpretq_u16_u8(a0.val[1]));  // a[x+1] - a[x]
    a32.val[0] = vmlaq_u16(a16, vreinterpretq_u16_u8(a0.val[0]),
                           v_32);  // a[x] * 32 + 16
    a32.val[1] = vmlaq_u16(a16, vreinterpretq_u16_u8(a0.val[1]),
                           v_32);  // a[x] * 32 + 16
    res.val[0] = vmlaq_u16(a32.val[0], diff.val[0], shift);
    res.val[1] = vmlaq_u16(a32.val[1], diff.val[1], shift);
    uint8x16_t v_temp =
        vcombine_u8(vshrn_n_u16(res.val[0], 5), vshrn_n_u16(res.val[1], 5));

    uint8x16_t mask = vld1q_u8(BaseMask[base_max_diff]);
    dst[r] = vorrq_u8(vandq_u8(mask, v_temp), vbicq_u8(a_mbase_x, mask));

    x += dx;
  }
}

static void dr_prediction_z1_16xN_neon(int N, uint8_t *dst, ptrdiff_t stride,
                                       const uint8_t *above, int upsample_above,
                                       int dx) {
  uint8x16_t dstvec[64];

  dr_prediction_z1_HxW_internal_neon(16, N, dstvec, above, upsample_above, dx);
  for (int i = 0; i < N; i++) {
    vst1q_u8(dst + stride * i, dstvec[i]);
  }
}

static AOM_FORCE_INLINE void dr_prediction_z1_32xN_internal_neon(
    int N, uint8x16x2_t *dstvec, const uint8_t *above, int upsample_above,
    int dx) {
  // here upsample_above is 0 by design of av1_use_intra_edge_upsample
  (void)upsample_above;
  const int frac_bits = 6;
  const int max_base_x = ((32 + N) - 1);

  // pre-filter above pixels
  // store in temp buffers:
  //   above[x] * 32 + 16
  //   above[x+1] - above[x]
  // final pixels will be calculated as:
  //   (above[x] * 32 + 16 + (above[x+1] - above[x]) * shift) >> 5

  uint8x16_t a_mbase_x;
  uint8x16x2_t a0, a1;
  uint16x8x2_t diff, a32;
  uint16x8_t a16, c3f;

  a_mbase_x = vdupq_n_u8(above[max_base_x]);
  a16 = vdupq_n_u16(16);
  c3f = vdupq_n_u16(0x3f);
  uint16x8_t v_32 = vdupq_n_u16(32);
  uint8x16_t v_zero = vdupq_n_u8(0);

  int x = dx;
  for (int r = 0; r < N; r++) {
    uint16x8x2_t res;
    uint8x16_t res16[2];
    uint8x16_t a0_128, a1_128;

    int base = x >> frac_bits;
    int base_max_diff = (max_base_x - base);
    if (base_max_diff <= 0) {
      for (int i = r; i < N; ++i) {
        dstvec[i].val[0] = a_mbase_x;  // save 32 values
        dstvec[i].val[1] = a_mbase_x;
      }
      return;
    }
    if (base_max_diff > 32) base_max_diff = 32;

    uint16x8_t shift = vshrq_n_u16(vandq_u16(vdupq_n_u16(x), c3f), 1);

    for (int j = 0, jj = 0; j < 32; j += 16, jj++) {
      int mdiff = base_max_diff - j;
      if (mdiff <= 0) {
        res16[jj] = a_mbase_x;
      } else {
        a0_128 = vld1q_u8(above + base + j);
        a1_128 = vld1q_u8(above + base + j + 1);
        a0 = vzipq_u8(a0_128, v_zero);
        a1 = vzipq_u8(a1_128, v_zero);
        diff.val[0] =
            vsubq_u16(vreinterpretq_u16_u8(a1.val[0]),
                      vreinterpretq_u16_u8(a0.val[0]));  // a[x+1] - a[x]
        diff.val[1] =
            vsubq_u16(vreinterpretq_u16_u8(a1.val[1]),
                      vreinterpretq_u16_u8(a0.val[1]));  // a[x+1] - a[x]
        a32.val[0] = vmlaq_u16(a16, vreinterpretq_u16_u8(a0.val[0]),
                               v_32);  // a[x] * 32 + 16
        a32.val[1] = vmlaq_u16(a16, vreinterpretq_u16_u8(a0.val[1]),
                               v_32);  // a[x] * 32 + 16
        res.val[0] = vmlaq_u16(a32.val[0], diff.val[0], shift);
        res.val[1] = vmlaq_u16(a32.val[1], diff.val[1], shift);

        res16[jj] =
            vcombine_u8(vshrn_n_u16(res.val[0], 5), vshrn_n_u16(res.val[1], 5));
      }
    }

    uint8x16x2_t mask;

    mask.val[0] = vld1q_u8(BaseMask[base_max_diff]);
    mask.val[1] = vld1q_u8(BaseMask[base_max_diff] + 16);
    dstvec[r].val[0] = vorrq_u8(vandq_u8(mask.val[0], res16[0]),
                                vbicq_u8(a_mbase_x, mask.val[0]));
    dstvec[r].val[1] = vorrq_u8(vandq_u8(mask.val[1], res16[1]),
                                vbicq_u8(a_mbase_x, mask.val[1]));
    x += dx;
  }
}

static void dr_prediction_z1_32xN_neon(int N, uint8_t *dst, ptrdiff_t stride,
                                       const uint8_t *above, int upsample_above,
                                       int dx) {
  uint8x16x2_t dstvec[64];

  dr_prediction_z1_32xN_internal_neon(N, dstvec, above, upsample_above, dx);
  for (int i = 0; i < N; i++) {
    vst1q_u8(dst + stride * i, dstvec[i].val[0]);
    vst1q_u8(dst + stride * i + 16, dstvec[i].val[1]);
  }
}

static void dr_prediction_z1_64xN_neon(int N, uint8_t *dst, ptrdiff_t stride,
                                       const uint8_t *above, int upsample_above,
                                       int dx) {
  // here upsample_above is 0 by design of av1_use_intra_edge_upsample
  (void)upsample_above;
  const int frac_bits = 6;
  const int max_base_x = ((64 + N) - 1);

  // pre-filter above pixels
  // store in temp buffers:
  //   above[x] * 32 + 16
  //   above[x+1] - above[x]
  // final pixels will be calculated as:
  //   (above[x] * 32 + 16 + (above[x+1] - above[x]) * shift) >> 5

  uint8x16x2_t a0, a1;
  uint16x8x2_t a32, diff;
  uint16x8_t a16, c3f;
  uint8x16_t a_mbase_x, max_base_x128, mask128;

  a16 = vdupq_n_u16(16);
  a_mbase_x = vdupq_n_u8(above[max_base_x]);
  max_base_x128 = vdupq_n_u8(max_base_x);
  c3f = vdupq_n_u16(0x3f);
  uint16x8_t v_32 = vdupq_n_u16(32);
  uint8x16_t v_zero = vdupq_n_u8(0);
  uint8x16_t step = vdupq_n_u8(16);

  int x = dx;
  for (int r = 0; r < N; r++, dst += stride) {
    uint16x8x2_t res;

    int base = x >> frac_bits;
    if (base >= max_base_x) {
      for (int i = r; i < N; ++i) {
        vst1q_u8(dst, a_mbase_x);
        vst1q_u8(dst + 16, a_mbase_x);
        vst1q_u8(dst + 32, a_mbase_x);
        vst1q_u8(dst + 48, a_mbase_x);
        dst += stride;
      }
      return;
    }

    uint16x8_t shift = vshrq_n_u16(vandq_u16(vdupq_n_u16(x), c3f), 1);
    uint8x16_t a0_128, a1_128, res128;
    uint8x16_t base_inc128 =
        vaddq_u8(vdupq_n_u8(base), vcombine_u8(vcreate_u8(0x0706050403020100),
                                               vcreate_u8(0x0F0E0D0C0B0A0908)));

    for (int j = 0; j < 64; j += 16) {
      int mdif = max_base_x - (base + j);
      if (mdif <= 0) {
        vst1q_u8(dst + j, a_mbase_x);
      } else {
        a0_128 = vld1q_u8(above + base + j);
        a1_128 = vld1q_u8(above + base + 1 + j);
        a0 = vzipq_u8(a0_128, v_zero);
        a1 = vzipq_u8(a1_128, v_zero);
        diff.val[0] =
            vsubq_u16(vreinterpretq_u16_u8(a1.val[0]),
                      vreinterpretq_u16_u8(a0.val[0]));  // a[x+1] - a[x]
        diff.val[1] =
            vsubq_u16(vreinterpretq_u16_u8(a1.val[1]),
                      vreinterpretq_u16_u8(a0.val[1]));  // a[x+1] - a[x]
        a32.val[0] = vmlaq_u16(a16, vreinterpretq_u16_u8(a0.val[0]),
                               v_32);  // a[x] * 32 + 16
        a32.val[1] = vmlaq_u16(a16, vreinterpretq_u16_u8(a0.val[1]),
                               v_32);  // a[x] * 32 + 16
        res.val[0] = vmlaq_u16(a32.val[0], diff.val[0], shift);
        res.val[1] = vmlaq_u16(a32.val[1], diff.val[1], shift);
        uint8x16_t v_temp =
            vcombine_u8(vshrn_n_u16(res.val[0], 5), vshrn_n_u16(res.val[1], 5));

        mask128 = vcgtq_u8(vqsubq_u8(max_base_x128, base_inc128), v_zero);
        res128 =
            vorrq_u8(vandq_u8(mask128, v_temp), vbicq_u8(a_mbase_x, mask128));
        vst1q_u8(dst + j, res128);

        base_inc128 = vaddq_u8(base_inc128, step);
      }
    }
    x += dx;
  }
}

// Directional prediction, zone 1: 0 < angle < 90
void av1_dr_prediction_z1_neon(uint8_t *dst, ptrdiff_t stride, int bw, int bh,
                               const uint8_t *above, const uint8_t *left,
                               int upsample_above, int dx, int dy) {
  (void)left;
  (void)dy;

  switch (bw) {
    case 4:
      dr_prediction_z1_4xN_neon(bh, dst, stride, above, upsample_above, dx);
      break;
    case 8:
      dr_prediction_z1_8xN_neon(bh, dst, stride, above, upsample_above, dx);
      break;
    case 16:
      dr_prediction_z1_16xN_neon(bh, dst, stride, above, upsample_above, dx);
      break;
    case 32:
      dr_prediction_z1_32xN_neon(bh, dst, stride, above, upsample_above, dx);
      break;
    case 64:
      dr_prediction_z1_64xN_neon(bh, dst, stride, above, upsample_above, dx);
      break;
    default: break;
  }
  return;
}

/* ---------------------P R E D I C T I O N   Z 2--------------------------- */

static DECLARE_ALIGNED(16, uint8_t, LoadMaskz2[4][16]) = {
  { 0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0,
    0, 0, 0 },
  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff }
};

static AOM_FORCE_INLINE void vector_shift_x4(uint8x8_t *vec, uint8x8_t *v_zero,
                                             int shift_value) {
  switch (shift_value) {
    case 1: *vec = vext_u8(*v_zero, *vec, 7); break;
    case 2: *vec = vext_u8(*v_zero, *vec, 6); break;
    case 3: *vec = vext_u8(*v_zero, *vec, 5); break;
    default: break;
  }
}

static void dr_prediction_z2_Nx4_neon(int N, uint8_t *dst, ptrdiff_t stride,
                                      const uint8_t *above, const uint8_t *left,
                                      int upsample_above, int upsample_left,
                                      int dx, int dy) {
  const int min_base_x = -(1 << upsample_above);
  const int min_base_y = -(1 << upsample_left);
  const int frac_bits_x = 6 - upsample_above;
  const int frac_bits_y = 6 - upsample_left;

  assert(dx > 0);
  // pre-filter above pixels
  // store in temp buffers:
  //   above[x] * 32 + 16
  //   above[x+1] - above[x]
  // final pixels will be calculated as:
  //   (above[x] * 32 + 16 + (above[x+1] - above[x]) * shift) >> 5
  uint16x8_t a0_x, a1_x, a32, diff;
  uint16x8_t v_32 = vdupq_n_u16(32);
  uint16x8_t v_zero = vdupq_n_u16(0);
  uint16x8_t a16 = vdupq_n_u16(16);

  uint8x8_t v_zero_u8 = vdup_n_u8(0);
  uint16x4_t v_c3f = vdup_n_u16(0x3f);
  uint16x4_t r6 = vcreate_u16(0x00C0008000400000);
  int16x4_t v_upsample_left = vdup_n_s16(upsample_left);
  int16x4_t v_upsample_above = vdup_n_s16(upsample_above);
  int16x4_t v_1234 = vcreate_s16(0x0004000300020001);
  int16x4_t dy64 = vdup_n_s16(dy);
  int16x4_t v_frac_bits_y = vdup_n_s16(-frac_bits_y);
  int16x4_t min_base_y64 = vdup_n_s16(min_base_y);
  int16x4_t v_one = vdup_lane_s16(v_1234, 0);

  for (int r = 0; r < N; r++) {
    uint16x8_t res, shift;
    uint16x4_t ydx;
    uint8x8_t resx, resy;
    uint16x4x2_t v_shift;

    int y = r + 1;
    int base_x = (-y * dx) >> frac_bits_x;
    int base_shift = 0;
    if (base_x < (min_base_x - 1)) {
      base_shift = (min_base_x - base_x - 1) >> upsample_above;
    }
    int base_min_diff =
        (min_base_x - base_x + upsample_above) >> upsample_above;
    if (base_min_diff > 4) {
      base_min_diff = 4;
    } else {
      if (base_min_diff < 0) base_min_diff = 0;
    }

    if (base_shift > 3) {
      a0_x = v_zero;
      a1_x = v_zero;
      v_shift.val[0] = vreinterpret_u16_u8(v_zero_u8);
      v_shift.val[1] = vreinterpret_u16_u8(v_zero_u8);
    } else {
      ydx = vdup_n_u16(y * dx);

      if (upsample_above) {
        uint8x8x2_t v_tmp;
        v_tmp.val[0] = vld1_u8(above + base_x + base_shift);
        v_tmp.val[1] = vld1_u8(above + base_x + base_shift + 8);
        uint8x8_t v_index_low = vld1_u8(EvenOddMaskx[base_shift]);
        uint8x8_t v_index_high = vld1_u8(EvenOddMaskx[base_shift] + 8);
        a0_x = vmovl_u8(vtbl2_u8(v_tmp, v_index_low));
        a1_x = vmovl_u8(vtbl2_u8(v_tmp, v_index_high));
        v_shift.val[0] = vshr_n_u16(
            vand_u16(vshl_u16(vsub_u16(r6, ydx), v_upsample_above), v_c3f), 1);
      } else {
        uint8x8_t v_a0_x64 = vld1_u8(above + base_x + base_shift);
        vector_shift_x4(&v_a0_x64, &v_zero_u8, base_shift);
        uint8x8_t v_a1_x64 = vext_u8(v_a0_x64, v_zero_u8, 1);
        v_shift.val[0] = vshr_n_u16(vand_u16(vsub_u16(r6, ydx), v_c3f), 1);
        a0_x = vmovl_u8(v_a0_x64);
        a1_x = vmovl_u8(v_a1_x64);
      }
    }

    // y calc
    uint8x8_t a0_y, a1_y;
    if (base_x < min_base_x) {
      DECLARE_ALIGNED(32, int16_t, base_y_c[4]);
      int16x4_t v_r6 = vdup_n_s16(r << 6);
      int16x4_t y_c64 = vmls_s16(v_r6, v_1234, dy64);
      int16x4_t base_y_c64 = vshl_s16(y_c64, v_frac_bits_y);
      uint16x4_t mask64 = vcgt_s16(min_base_y64, base_y_c64);

      base_y_c64 = vbic_s16(base_y_c64, vreinterpret_s16_u16(mask64));
      vst1_s16(base_y_c, base_y_c64);
      a0_y = v_zero_u8;
      a0_y = vld1_lane_u8(left + base_y_c[0], a0_y, 0);
      a0_y = vld1_lane_u8(left + base_y_c[1], a0_y, 2);
      a0_y = vld1_lane_u8(left + base_y_c[2], a0_y, 4);
      a0_y = vld1_lane_u8(left + base_y_c[3], a0_y, 6);

      base_y_c64 = vadd_s16(base_y_c64, v_one);
      vst1_s16(base_y_c, base_y_c64);
      a1_y = v_zero_u8;
      a1_y = vld1_lane_u8(left + base_y_c[0], a1_y, 0);
      a1_y = vld1_lane_u8(left + base_y_c[1], a1_y, 2);
      a1_y = vld1_lane_u8(left + base_y_c[2], a1_y, 4);
      a1_y = vld1_lane_u8(left + base_y_c[3], a1_y, 6);

      if (upsample_left) {
        v_shift.val[1] = vshr_n_u16(
            vand_u16(vshl_u16(vreinterpret_u16_s16(y_c64), v_upsample_left),
                     v_c3f),
            1);
      } else {
        v_shift.val[1] =
            vshr_n_u16(vand_u16(vreinterpret_u16_s16(y_c64), v_c3f), 1);
      }

      a0_x = vcombine_u16(vget_low_u16(a0_x), vreinterpret_u16_u8(a0_y));
      a1_x = vcombine_u16(vget_low_u16(a1_x), vreinterpret_u16_u8(a1_y));
    }
    shift = vcombine_u16(v_shift.val[0], v_shift.val[1]);
    diff = vsubq_u16(a1_x, a0_x);      // a[x+1] - a[x]
    a32 = vmlaq_u16(a16, a0_x, v_32);  // a[x] * 32 + 16
    res = vmlaq_u16(a32, diff, shift);
    resx = vshrn_n_u16(res, 5);
    resy = vext_u8(resx, v_zero_u8, 4);

    uint8x8_t mask = vld1_u8(BaseMask[base_min_diff]);
    uint8x8_t v_resxy = vorr_u8(vand_u8(mask, resy), vbic_u8(resx, mask));
    vst1_lane_u32((uint32_t *)dst, vreinterpret_u32_u8(v_resxy), 0);

    dst += stride;
  }
}

static AOM_FORCE_INLINE void vector_shuffle(uint8x16_t *vec, uint8x16_t *vzero,
                                            int shift_value) {
  switch (shift_value) {
    case 1: *vec = vextq_u8(*vzero, *vec, 15); break;
    case 2: *vec = vextq_u8(*vzero, *vec, 14); break;
    case 3: *vec = vextq_u8(*vzero, *vec, 13); break;
    case 4: *vec = vextq_u8(*vzero, *vec, 12); break;
    case 5: *vec = vextq_u8(*vzero, *vec, 11); break;
    case 6: *vec = vextq_u8(*vzero, *vec, 10); break;
    case 7: *vec = vextq_u8(*vzero, *vec, 9); break;
    case 8: *vec = vextq_u8(*vzero, *vec, 8); break;
    case 9: *vec = vextq_u8(*vzero, *vec, 7); break;
    case 10: *vec = vextq_u8(*vzero, *vec, 6); break;
    case 11: *vec = vextq_u8(*vzero, *vec, 5); break;
    case 12: *vec = vextq_u8(*vzero, *vec, 4); break;
    case 13: *vec = vextq_u8(*vzero, *vec, 3); break;
    case 14: *vec = vextq_u8(*vzero, *vec, 2); break;
    case 15: *vec = vextq_u8(*vzero, *vec, 1); break;
    default: break;
  }
}

static void dr_prediction_z2_Nx8_neon(int N, uint8_t *dst, ptrdiff_t stride,
                                      const uint8_t *above, const uint8_t *left,
                                      int upsample_above, int upsample_left,
                                      int dx, int dy) {
  const int min_base_x = -(1 << upsample_above);
  const int min_base_y = -(1 << upsample_left);
  const int frac_bits_x = 6 - upsample_above;
  const int frac_bits_y = 6 - upsample_left;

  // pre-filter above pixels
  // store in temp buffers:
  //   above[x] * 32 + 16
  //   above[x+1] - above[x]
  // final pixels will be calculated as:
  //   (above[x] * 32 + 16 + (above[x+1] - above[x]) * shift) >> 5
  uint8x16x2_t a0_x, a1_x;
  uint16x8x2_t diff, a32;
  uint16x8_t c1234, a16, c3f;
  uint8x16_t a0_x128, a1_x128;
  int16x8_t min_base_y128, dy128;
  uint16x8_t v_32 = vdupq_n_u16(32);
  uint8x16_t v_zero = vdupq_n_u8(0);
  int16x8_t v_upsample_left = vdupq_n_s16(upsample_left);
  int16x8_t v_upsample_above = vdupq_n_s16(upsample_above);
  int16x8_t v_frac_bits_y = vdupq_n_s16(-frac_bits_y);

  a16 = vdupq_n_u16(16);
  c3f = vdupq_n_u16(0x3f);
  min_base_y128 = vdupq_n_s16(min_base_y);
  dy128 = vdupq_n_s16(dy);
  c1234 = vcombine_u16(vcreate_u16(0x0004000300020001),
                       vcreate_u16(0x0008000700060005));

  for (int r = 0; r < N; r++) {
    uint8x8_t resx, resy, resxy;
    uint16x8_t r6, ydx;
    uint16x8x2_t res, shift;

    int y = r + 1;
    int base_x = (-y * dx) >> frac_bits_x;
    int base_shift = 0;
    if (base_x < (min_base_x - 1)) {
      base_shift = (min_base_x - base_x - 1) >> upsample_above;
    }
    int base_min_diff =
        (min_base_x - base_x + upsample_above) >> upsample_above;
    if (base_min_diff > 8) {
      base_min_diff = 8;
    } else {
      if (base_min_diff < 0) base_min_diff = 0;
    }

    if (base_shift > 7) {
      a0_x.val[0] = v_zero;
      a0_x.val[1] = v_zero;
      a1_x.val[0] = v_zero;
      a1_x.val[1] = v_zero;
      shift.val[0] = vreinterpretq_u16_u8(v_zero);
      shift.val[1] = vreinterpretq_u16_u8(v_zero);
    } else {
      ydx = vdupq_n_u16(y * dx);
      r6 = vshlq_n_u16(vextq_u16(c1234, vreinterpretq_u16_u8(v_zero), 2), 6);

      if (upsample_above) {
        uint8x8x2_t v_tmp;
        v_tmp.val[0] = vld1_u8(above + base_x + base_shift);
        v_tmp.val[1] = vld1_u8(above + base_x + base_shift + 8);
        uint8x8_t v_index_low = vld1_u8(EvenOddMaskx[base_shift]);
        uint8x8_t v_index_high = vld1_u8(EvenOddMaskx[base_shift] + 8);
        shift.val[0] = vshrq_n_u16(
            vandq_u16(vshlq_u16(vsubq_u16(r6, ydx), v_upsample_above), c3f), 1);
        a0_x.val[0] =
            vreinterpretq_u8_u16(vmovl_u8(vtbl2_u8(v_tmp, v_index_low)));
        a1_x.val[0] =
            vreinterpretq_u8_u16(vmovl_u8(vtbl2_u8(v_tmp, v_index_high)));
      } else {
        a0_x128 = vld1q_u8(above + base_x + base_shift);
        a1_x128 = vextq_u8(a0_x128, v_zero, 1);
        vector_shuffle(&a0_x128, &v_zero, base_shift);
        vector_shuffle(&a1_x128, &v_zero, base_shift);
        shift.val[0] = vshrq_n_u16(vandq_u16(vsubq_u16(r6, ydx), c3f), 1);
        a0_x.val[0] = vreinterpretq_u8_u16(vmovl_u8(vget_low_u8(a0_x128)));
        a1_x.val[0] = vreinterpretq_u8_u16(vmovl_u8(vget_low_u8(a1_x128)));
      }
    }

    // y calc
    if (base_x < min_base_x) {
      DECLARE_ALIGNED(32, int16_t, base_y_c[16]);
      int16x8_t y_c128, base_y_c128;
      uint16x8_t mask128;
      int16x8_t v_r6 = vdupq_n_s16(r << 6);

      y_c128 = vmlsq_s16(v_r6, vreinterpretq_s16_u16(c1234), dy128);
      base_y_c128 = vshlq_s16(y_c128, v_frac_bits_y);
      mask128 = vcgtq_s16(min_base_y128, base_y_c128);

      base_y_c128 = vbicq_s16(base_y_c128, vreinterpretq_s16_u16(mask128));
      vst1q_s16(base_y_c, base_y_c128);
      a0_x.val[1] = v_zero;
      a0_x.val[1] = vld1q_lane_u8(left + base_y_c[0], a0_x.val[1], 0);
      a0_x.val[1] = vld1q_lane_u8(left + base_y_c[1], a0_x.val[1], 2);
      a0_x.val[1] = vld1q_lane_u8(left + base_y_c[2], a0_x.val[1], 4);
      a0_x.val[1] = vld1q_lane_u8(left + base_y_c[3], a0_x.val[1], 6);
      a0_x.val[1] = vld1q_lane_u8(left + base_y_c[4], a0_x.val[1], 8);
      a0_x.val[1] = vld1q_lane_u8(left + base_y_c[5], a0_x.val[1], 10);
      a0_x.val[1] = vld1q_lane_u8(left + base_y_c[6], a0_x.val[1], 12);
      a0_x.val[1] = vld1q_lane_u8(left + base_y_c[7], a0_x.val[1], 14);

      base_y_c128 =
          vaddq_s16(base_y_c128, vreinterpretq_s16_u16(vshrq_n_u16(a16, 4)));
      vst1q_s16(base_y_c, base_y_c128);
      a1_x.val[1] = v_zero;
      a1_x.val[1] = vld1q_lane_u8(left + base_y_c[0], a1_x.val[1], 0);
      a1_x.val[1] = vld1q_lane_u8(left + base_y_c[1], a1_x.val[1], 2);
      a1_x.val[1] = vld1q_lane_u8(left + base_y_c[2], a1_x.val[1], 4);
      a1_x.val[1] = vld1q_lane_u8(left + base_y_c[3], a1_x.val[1], 6);
      a1_x.val[1] = vld1q_lane_u8(left + base_y_c[4], a1_x.val[1], 8);
      a1_x.val[1] = vld1q_lane_u8(left + base_y_c[5], a1_x.val[1], 10);
      a1_x.val[1] = vld1q_lane_u8(left + base_y_c[6], a1_x.val[1], 12);
      a1_x.val[1] = vld1q_lane_u8(left + base_y_c[7], a1_x.val[1], 14);

      if (upsample_left) {
        shift.val[1] = vshrq_n_u16(
            vandq_u16(vshlq_u16(vreinterpretq_u16_s16(y_c128), v_upsample_left),
                      c3f),
            1);
      } else {
        shift.val[1] =
            vshrq_n_u16(vandq_u16(vreinterpretq_u16_s16(y_c128), c3f), 1);
      }
    }
    diff.val[0] =
        vsubq_u16(vreinterpretq_u16_u8(a1_x.val[0]),
                  vreinterpretq_u16_u8(a0_x.val[0]));  // a[x+1] - a[x]
    diff.val[1] =
        vsubq_u16(vreinterpretq_u16_u8(a1_x.val[1]),
                  vreinterpretq_u16_u8(a0_x.val[1]));  // a[x+1] - a[x]
    a32.val[0] = vmlaq_u16(a16, vreinterpretq_u16_u8(a0_x.val[0]),
                           v_32);  // a[x] * 32 + 16
    a32.val[1] = vmlaq_u16(a16, vreinterpretq_u16_u8(a0_x.val[1]),
                           v_32);  // a[x] * 32 + 16
    res.val[0] = vmlaq_u16(a32.val[0], diff.val[0], shift.val[0]);
    res.val[1] = vmlaq_u16(a32.val[1], diff.val[1], shift.val[1]);
    resx = vshrn_n_u16(res.val[0], 5);
    resy = vshrn_n_u16(res.val[1], 5);

    uint8x8_t mask = vld1_u8(BaseMask[base_min_diff]);

    resxy = vorr_u8(vand_u8(mask, resy), vbic_u8(resx, mask));
    vst1_u8(dst, resxy);
    dst += stride;
  }
}

static void dr_prediction_z2_HxW_neon(int H, int W, uint8_t *dst,
                                      ptrdiff_t stride, const uint8_t *above,
                                      const uint8_t *left, int upsample_above,
                                      int upsample_left, int dx, int dy) {
  // here upsample_above and upsample_left are 0 by design of
  // av1_use_intra_edge_upsample
  const int min_base_x = -1;
  const int min_base_y = -1;
  (void)upsample_above;
  (void)upsample_left;
  const int frac_bits_x = 6;
  const int frac_bits_y = 6;

  uint16x8_t a16, c1, c3f;
  int16x8_t min_base_y256, dy256;
  uint16x8x2_t a32, c0123, c1234, diff, shifty;
  uint8x16x2_t a0_x, a1_x, a0_y, a1_y;
  uint8x16_t a0_x128, a1_x128;
  uint16x8_t v_32 = vdupq_n_u16(32);
  uint8x16_t v_zero = vdupq_n_u8(0);
  int16x8_t v_frac_bits_y = vdupq_n_s16(-frac_bits_y);

  DECLARE_ALIGNED(32, int16_t, base_y_c[16]);

  a16 = vdupq_n_u16(16);
  c1 = vshrq_n_u16(a16, 4);
  min_base_y256 = vdupq_n_s16(min_base_y);
  c3f = vdupq_n_u16(0x3f);
  dy256 = vdupq_n_s16(dy);
  c0123.val[0] = vcombine_u16(vcreate_u16(0x0003000200010000),
                              vcreate_u16(0x0007000600050004));
  c0123.val[1] = vcombine_u16(vcreate_u16(0x000B000A00090008),
                              vcreate_u16(0x000F000E000D000C));
  c1234.val[0] = vaddq_u16(c0123.val[0], c1);
  c1234.val[1] = vaddq_u16(c0123.val[1], c1);

  for (int r = 0; r < H; r++) {
    uint16x8x2_t res, r6, shift;
    uint16x8_t ydx, j256;
    uint8x16_t resx, resy, resxy;
    int y = r + 1;
    ydx = vdupq_n_u16((uint16_t)(y * dx));

    int base_x = (-y * dx) >> frac_bits_x;
    for (int j = 0; j < W; j += 16) {
      j256 = vdupq_n_u16(j);

      int base_shift = 0;
      if ((base_x + j) < (min_base_x - 1)) {
        base_shift = (min_base_x - (base_x + j) - 1);
      }
      int base_min_diff = (min_base_x - base_x - j);
      if (base_min_diff > 16) {
        base_min_diff = 16;
      } else {
        if (base_min_diff < 0) base_min_diff = 0;
      }

      if (base_shift < 16) {
        a0_x128 = vld1q_u8(above + base_x + base_shift + j);
        a1_x128 = vld1q_u8(above + base_x + base_shift + 1 + j);
        vector_shuffle(&a0_x128, &v_zero, base_shift);
        vector_shuffle(&a1_x128, &v_zero, base_shift);
        a0_x = vzipq_u8(a0_x128, v_zero);
        a1_x = vzipq_u8(a1_x128, v_zero);
        r6.val[0] = vshlq_n_u16(vaddq_u16(c0123.val[0], j256), 6);
        r6.val[1] = vshlq_n_u16(vaddq_u16(c0123.val[1], j256), 6);
        shift.val[0] =
            vshrq_n_u16(vandq_u16(vsubq_u16(r6.val[0], ydx), c3f), 1);
        shift.val[1] =
            vshrq_n_u16(vandq_u16(vsubq_u16(r6.val[1], ydx), c3f), 1);
        diff.val[0] =
            vsubq_u16(vreinterpretq_u16_u8(a1_x.val[0]),
                      vreinterpretq_u16_u8(a0_x.val[0]));  // a[x+1] - a[x]
        diff.val[1] =
            vsubq_u16(vreinterpretq_u16_u8(a1_x.val[1]),
                      vreinterpretq_u16_u8(a0_x.val[1]));  // a[x+1] - a[x]
        a32.val[0] = vmlaq_u16(a16, vreinterpretq_u16_u8(a0_x.val[0]),
                               v_32);  // a[x] * 32 + 16
        a32.val[1] = vmlaq_u16(a16, vreinterpretq_u16_u8(a0_x.val[1]),
                               v_32);  // a[x] * 32 + 16
        res.val[0] = vmlaq_u16(a32.val[0], diff.val[0], shift.val[0]);
        res.val[1] = vmlaq_u16(a32.val[1], diff.val[1], shift.val[1]);
        resx =
            vcombine_u8(vshrn_n_u16(res.val[0], 5), vshrn_n_u16(res.val[1], 5));
      } else {
        resx = v_zero;
      }

      // y calc
      if (base_x < min_base_x) {
        uint16x8x2_t mask256;
        int16x8x2_t c256, y_c256, base_y_c256, mul16;
        int16x8_t v_r6 = vdupq_n_s16(r << 6);

        c256.val[0] = vaddq_s16(vreinterpretq_s16_u16(j256),
                                vreinterpretq_s16_u16(c1234.val[0]));
        c256.val[1] = vaddq_s16(vreinterpretq_s16_u16(j256),
                                vreinterpretq_s16_u16(c1234.val[1]));
        mul16.val[0] = vminq_s16(vmulq_s16(c256.val[0], dy256),
                                 vreinterpretq_s16_u16(vshrq_n_u16(
                                     vreinterpretq_u16_s16(min_base_y256), 1)));
        mul16.val[1] = vminq_s16(vmulq_s16(c256.val[1], dy256),
                                 vreinterpretq_s16_u16(vshrq_n_u16(
                                     vreinterpretq_u16_s16(min_base_y256), 1)));
        y_c256.val[0] = vsubq_s16(v_r6, mul16.val[0]);
        y_c256.val[1] = vsubq_s16(v_r6, mul16.val[1]);

        base_y_c256.val[0] = vshlq_s16(y_c256.val[0], v_frac_bits_y);
        base_y_c256.val[1] = vshlq_s16(y_c256.val[1], v_frac_bits_y);
        mask256.val[0] = vcgtq_s16(min_base_y256, base_y_c256.val[0]);
        mask256.val[1] = vcgtq_s16(min_base_y256, base_y_c256.val[1]);

        base_y_c256.val[0] = vorrq_s16(
            vandq_s16(vreinterpretq_s16_u16(mask256.val[0]), min_base_y256),
            vbicq_s16(base_y_c256.val[0],
                      vreinterpretq_s16_u16(mask256.val[0])));
        base_y_c256.val[1] = vorrq_s16(
            vandq_s16(vreinterpretq_s16_u16(mask256.val[1]), min_base_y256),
            vbicq_s16(base_y_c256.val[1],
                      vreinterpretq_s16_u16(mask256.val[1])));

        int16_t min_y = vgetq_lane_s16(base_y_c256.val[1], 7);
        int16_t max_y = vgetq_lane_s16(base_y_c256.val[0], 0);
        int16_t offset_diff = max_y - min_y;

        if (offset_diff < 16) {
          int16x8_t min_y256 =
              vdupq_lane_s16(vget_high_s16(base_y_c256.val[1]), 3);

          int16x8x2_t base_y_offset;
          base_y_offset.val[0] = vsubq_s16(base_y_c256.val[0], min_y256);
          base_y_offset.val[1] = vsubq_s16(base_y_c256.val[1], min_y256);

          int8x16_t base_y_offset128 =
              vcombine_s8(vqmovn_s16(base_y_offset.val[0]),
                          vqmovn_s16(base_y_offset.val[1]));

          uint8x16_t a0_y128, a1_y128;
          uint8x16_t v_loadmaskz2 = vld1q_u8(LoadMaskz2[offset_diff / 4]);
          a0_y128 = vld1q_u8(left + min_y);
          a0_y128 = vandq_u8(a0_y128, v_loadmaskz2);
          a1_y128 = vld1q_u8(left + min_y + 1);
          a1_y128 = vandq_u8(a1_y128, v_loadmaskz2);
#if defined(__aarch64__)
          a0_y128 = vqtbl1q_u8(a0_y128, vreinterpretq_u8_s8(base_y_offset128));
          a1_y128 = vqtbl1q_u8(a1_y128, vreinterpretq_u8_s8(base_y_offset128));
#else
          uint8x8x2_t v_tmp;
          uint8x8x2_t v_res;
          uint8x8_t v_index_low =
              vget_low_u8(vreinterpretq_u8_s8(base_y_offset128));
          uint8x8_t v_index_high =
              vget_high_u8(vreinterpretq_u8_s8(base_y_offset128));
          v_tmp.val[0] = vget_low_u8(a0_y128);
          v_tmp.val[1] = vget_high_u8(a0_y128);
          v_res.val[0] = vtbl2_u8(v_tmp, v_index_low);
          v_res.val[1] = vtbl2_u8(v_tmp, v_index_high);
          a0_y128 = vcombine_u8(v_res.val[0], v_res.val[1]);
          v_tmp.val[0] = vget_low_u8(a1_y128);
          v_tmp.val[1] = vget_high_u8(a1_y128);
          v_res.val[0] = vtbl2_u8(v_tmp, v_index_low);
          v_res.val[1] = vtbl2_u8(v_tmp, v_index_high);
          a1_y128 = vcombine_u8(v_res.val[0], v_res.val[1]);
#endif
          a0_y = vzipq_u8(a0_y128, v_zero);
          a1_y = vzipq_u8(a1_y128, v_zero);
        } else {
          base_y_c256.val[0] = vbicq_s16(base_y_c256.val[0],
                                         vreinterpretq_s16_u16(mask256.val[0]));
          base_y_c256.val[1] = vbicq_s16(base_y_c256.val[1],
                                         vreinterpretq_s16_u16(mask256.val[1]));
          vst1q_s16(base_y_c, base_y_c256.val[0]);
          vst1q_s16(base_y_c + 8, base_y_c256.val[1]);
          a0_y.val[0] = v_zero;
          a0_y.val[1] = v_zero;
          a0_y.val[0] = vld1q_lane_u8(left + base_y_c[0], a0_y.val[0], 0);
          a0_y.val[0] = vld1q_lane_u8(left + base_y_c[1], a0_y.val[0], 2);
          a0_y.val[0] = vld1q_lane_u8(left + base_y_c[2], a0_y.val[0], 4);
          a0_y.val[0] = vld1q_lane_u8(left + base_y_c[3], a0_y.val[0], 6);
          a0_y.val[0] = vld1q_lane_u8(left + base_y_c[4], a0_y.val[0], 8);
          a0_y.val[0] = vld1q_lane_u8(left + base_y_c[5], a0_y.val[0], 10);
          a0_y.val[0] = vld1q_lane_u8(left + base_y_c[6], a0_y.val[0], 12);
          a0_y.val[0] = vld1q_lane_u8(left + base_y_c[7], a0_y.val[0], 14);
          a0_y.val[1] = vld1q_lane_u8(left + base_y_c[8], a0_y.val[1], 0);
          a0_y.val[1] = vld1q_lane_u8(left + base_y_c[9], a0_y.val[1], 2);
          a0_y.val[1] = vld1q_lane_u8(left + base_y_c[10], a0_y.val[1], 4);
          a0_y.val[1] = vld1q_lane_u8(left + base_y_c[11], a0_y.val[1], 6);
          a0_y.val[1] = vld1q_lane_u8(left + base_y_c[12], a0_y.val[1], 8);
          a0_y.val[1] = vld1q_lane_u8(left + base_y_c[13], a0_y.val[1], 10);
          a0_y.val[1] = vld1q_lane_u8(left + base_y_c[14], a0_y.val[1], 12);
          a0_y.val[1] = vld1q_lane_u8(left + base_y_c[15], a0_y.val[1], 14);

          base_y_c256.val[0] =
              vaddq_s16(base_y_c256.val[0], vreinterpretq_s16_u16(c1));
          base_y_c256.val[1] =
              vaddq_s16(base_y_c256.val[1], vreinterpretq_s16_u16(c1));
          vst1q_s16(base_y_c, base_y_c256.val[0]);
          vst1q_s16(base_y_c + 8, base_y_c256.val[1]);
          a1_y.val[0] = v_zero;
          a1_y.val[1] = v_zero;
          a1_y.val[0] = vld1q_lane_u8(left + base_y_c[0], a1_y.val[0], 0);
          a1_y.val[0] = vld1q_lane_u8(left + base_y_c[1], a1_y.val[0], 2);
          a1_y.val[0] = vld1q_lane_u8(left + base_y_c[2], a1_y.val[0], 4);
          a1_y.val[0] = vld1q_lane_u8(left + base_y_c[3], a1_y.val[0], 6);
          a1_y.val[0] = vld1q_lane_u8(left + base_y_c[4], a1_y.val[0], 8);
          a1_y.val[0] = vld1q_lane_u8(left + base_y_c[5], a1_y.val[0], 10);
          a1_y.val[0] = vld1q_lane_u8(left + base_y_c[6], a1_y.val[0], 12);
          a1_y.val[0] = vld1q_lane_u8(left + base_y_c[7], a1_y.val[0], 14);
          a1_y.val[1] = vld1q_lane_u8(left + base_y_c[8], a1_y.val[1], 0);
          a1_y.val[1] = vld1q_lane_u8(left + base_y_c[9], a1_y.val[1], 2);
          a1_y.val[1] = vld1q_lane_u8(left + base_y_c[10], a1_y.val[1], 4);
          a1_y.val[1] = vld1q_lane_u8(left + base_y_c[11], a1_y.val[1], 6);
          a1_y.val[1] = vld1q_lane_u8(left + base_y_c[12], a1_y.val[1], 8);
          a1_y.val[1] = vld1q_lane_u8(left + base_y_c[13], a1_y.val[1], 10);
          a1_y.val[1] = vld1q_lane_u8(left + base_y_c[14], a1_y.val[1], 12);
          a1_y.val[1] = vld1q_lane_u8(left + base_y_c[15], a1_y.val[1], 14);
        }
        shifty.val[0] = vshrq_n_u16(
            vandq_u16(vreinterpretq_u16_s16(y_c256.val[0]), c3f), 1);
        shifty.val[1] = vshrq_n_u16(
            vandq_u16(vreinterpretq_u16_s16(y_c256.val[1]), c3f), 1);
        diff.val[0] =
            vsubq_u16(vreinterpretq_u16_u8(a1_y.val[0]),
                      vreinterpretq_u16_u8(a0_y.val[0]));  // a[x+1] - a[x]
        diff.val[1] =
            vsubq_u16(vreinterpretq_u16_u8(a1_y.val[1]),
                      vreinterpretq_u16_u8(a0_y.val[1]));  // a[x+1] - a[x]
        a32.val[0] = vmlaq_u16(a16, vreinterpretq_u16_u8(a0_y.val[0]),
                               v_32);  // a[x] * 32 + 16
        a32.val[1] = vmlaq_u16(a16, vreinterpretq_u16_u8(a0_y.val[1]),
                               v_32);  // a[x] * 32 + 16
        res.val[0] = vmlaq_u16(a32.val[0], diff.val[0], shifty.val[0]);
        res.val[1] = vmlaq_u16(a32.val[1], diff.val[1], shifty.val[1]);

        resy =
            vcombine_u8(vshrn_n_u16(res.val[0], 5), vshrn_n_u16(res.val[1], 5));
      } else {
        resy = v_zero;
      }
      uint8x16_t mask = vld1q_u8(BaseMask[base_min_diff]);
      resxy = vorrq_u8(vandq_u8(mask, resy), vbicq_u8(resx, mask));
      vst1q_u8(dst + j, resxy);
    }  // for j
    dst += stride;
  }
}

// Directional prediction, zone 2: 90 < angle < 180
void av1_dr_prediction_z2_neon(uint8_t *dst, ptrdiff_t stride, int bw, int bh,
                               const uint8_t *above, const uint8_t *left,
                               int upsample_above, int upsample_left, int dx,
                               int dy) {
  assert(dx > 0);
  assert(dy > 0);

  switch (bw) {
    case 4:
      dr_prediction_z2_Nx4_neon(bh, dst, stride, above, left, upsample_above,
                                upsample_left, dx, dy);
      break;
    case 8:
      dr_prediction_z2_Nx8_neon(bh, dst, stride, above, left, upsample_above,
                                upsample_left, dx, dy);
      break;
    default:
      dr_prediction_z2_HxW_neon(bh, bw, dst, stride, above, left,
                                upsample_above, upsample_left, dx, dy);
      break;
  }
  return;
}

/* ---------------------P R E D I C T I O N   Z 3--------------------------- */

static AOM_FORCE_INLINE void transpose4x16_neon(uint8x16_t *x,
                                                uint16x8x2_t *d) {
  uint8x16x2_t w0, w1;

  w0 = vzipq_u8(x[0], x[1]);
  w1 = vzipq_u8(x[2], x[3]);

  d[0] = vzipq_u16(vreinterpretq_u16_u8(w0.val[0]),
                   vreinterpretq_u16_u8(w1.val[0]));
  d[1] = vzipq_u16(vreinterpretq_u16_u8(w0.val[1]),
                   vreinterpretq_u16_u8(w1.val[1]));
}

static AOM_FORCE_INLINE void transpose4x8_8x4_low_neon(uint8x8_t *x,
                                                       uint16x4x2_t *d) {
  uint8x8x2_t w0, w1;

  w0 = vzip_u8(x[0], x[1]);
  w1 = vzip_u8(x[2], x[3]);

  *d = vzip_u16(vreinterpret_u16_u8(w0.val[0]), vreinterpret_u16_u8(w1.val[0]));
}

static AOM_FORCE_INLINE void transpose4x8_8x4_neon(uint8x8_t *x,
                                                   uint16x4x2_t *d) {
  uint8x8x2_t w0, w1;

  w0 = vzip_u8(x[0], x[1]);
  w1 = vzip_u8(x[2], x[3]);

  d[0] =
      vzip_u16(vreinterpret_u16_u8(w0.val[0]), vreinterpret_u16_u8(w1.val[0]));
  d[1] =
      vzip_u16(vreinterpret_u16_u8(w0.val[1]), vreinterpret_u16_u8(w1.val[1]));
}

static AOM_FORCE_INLINE void transpose8x8_low_neon(uint8x8_t *x,
                                                   uint32x2x2_t *d) {
  uint8x8x2_t w0, w1, w2, w3;
  uint16x4x2_t w4, w5;

  w0 = vzip_u8(x[0], x[1]);
  w1 = vzip_u8(x[2], x[3]);
  w2 = vzip_u8(x[4], x[5]);
  w3 = vzip_u8(x[6], x[7]);

  w4 = vzip_u16(vreinterpret_u16_u8(w0.val[0]), vreinterpret_u16_u8(w1.val[0]));
  w5 = vzip_u16(vreinterpret_u16_u8(w2.val[0]), vreinterpret_u16_u8(w3.val[0]));

  d[0] = vzip_u32(vreinterpret_u32_u16(w4.val[0]),
                  vreinterpret_u32_u16(w5.val[0]));
  d[1] = vzip_u32(vreinterpret_u32_u16(w4.val[1]),
                  vreinterpret_u32_u16(w5.val[1]));
}

static AOM_FORCE_INLINE void transpose8x8_neon(uint8x8_t *x, uint32x2x2_t *d) {
  uint8x8x2_t w0, w1, w2, w3;
  uint16x4x2_t w4, w5, w6, w7;

  w0 = vzip_u8(x[0], x[1]);
  w1 = vzip_u8(x[2], x[3]);
  w2 = vzip_u8(x[4], x[5]);
  w3 = vzip_u8(x[6], x[7]);

  w4 = vzip_u16(vreinterpret_u16_u8(w0.val[0]), vreinterpret_u16_u8(w1.val[0]));
  w5 = vzip_u16(vreinterpret_u16_u8(w2.val[0]), vreinterpret_u16_u8(w3.val[0]));

  d[0] = vzip_u32(vreinterpret_u32_u16(w4.val[0]),
                  vreinterpret_u32_u16(w5.val[0]));
  d[1] = vzip_u32(vreinterpret_u32_u16(w4.val[1]),
                  vreinterpret_u32_u16(w5.val[1]));

  w6 = vzip_u16(vreinterpret_u16_u8(w0.val[1]), vreinterpret_u16_u8(w1.val[1]));
  w7 = vzip_u16(vreinterpret_u16_u8(w2.val[1]), vreinterpret_u16_u8(w3.val[1]));

  d[2] = vzip_u32(vreinterpret_u32_u16(w6.val[0]),
                  vreinterpret_u32_u16(w7.val[0]));
  d[3] = vzip_u32(vreinterpret_u32_u16(w6.val[1]),
                  vreinterpret_u32_u16(w7.val[1]));
}

static AOM_FORCE_INLINE void transpose16x8_8x16_neon(uint8x8_t *x,
                                                     uint64x2_t *d) {
  uint8x8x2_t w0, w1, w2, w3, w8, w9, w10, w11;
  uint16x4x2_t w4, w5, w12, w13;
  uint32x2x2_t w6, w7, w14, w15;

  w0 = vzip_u8(x[0], x[1]);
  w1 = vzip_u8(x[2], x[3]);
  w2 = vzip_u8(x[4], x[5]);
  w3 = vzip_u8(x[6], x[7]);

  w8 = vzip_u8(x[8], x[9]);
  w9 = vzip_u8(x[10], x[11]);
  w10 = vzip_u8(x[12], x[13]);
  w11 = vzip_u8(x[14], x[15]);

  w4 = vzip_u16(vreinterpret_u16_u8(w0.val[0]), vreinterpret_u16_u8(w1.val[0]));
  w5 = vzip_u16(vreinterpret_u16_u8(w2.val[0]), vreinterpret_u16_u8(w3.val[0]));
  w12 =
      vzip_u16(vreinterpret_u16_u8(w8.val[0]), vreinterpret_u16_u8(w9.val[0]));
  w13 = vzip_u16(vreinterpret_u16_u8(w10.val[0]),
                 vreinterpret_u16_u8(w11.val[0]));

  w6 = vzip_u32(vreinterpret_u32_u16(w4.val[0]),
                vreinterpret_u32_u16(w5.val[0]));
  w7 = vzip_u32(vreinterpret_u32_u16(w4.val[1]),
                vreinterpret_u32_u16(w5.val[1]));
  w14 = vzip_u32(vreinterpret_u32_u16(w12.val[0]),
                 vreinterpret_u32_u16(w13.val[0]));
  w15 = vzip_u32(vreinterpret_u32_u16(w12.val[1]),
                 vreinterpret_u32_u16(w13.val[1]));

  // Store first 4-line result
  d[0] = vcombine_u64(vreinterpret_u64_u32(w6.val[0]),
                      vreinterpret_u64_u32(w14.val[0]));
  d[1] = vcombine_u64(vreinterpret_u64_u32(w6.val[1]),
                      vreinterpret_u64_u32(w14.val[1]));
  d[2] = vcombine_u64(vreinterpret_u64_u32(w7.val[0]),
                      vreinterpret_u64_u32(w15.val[0]));
  d[3] = vcombine_u64(vreinterpret_u64_u32(w7.val[1]),
                      vreinterpret_u64_u32(w15.val[1]));

  w4 = vzip_u16(vreinterpret_u16_u8(w0.val[1]), vreinterpret_u16_u8(w1.val[1]));
  w5 = vzip_u16(vreinterpret_u16_u8(w2.val[1]), vreinterpret_u16_u8(w3.val[1]));
  w12 =
      vzip_u16(vreinterpret_u16_u8(w8.val[1]), vreinterpret_u16_u8(w9.val[1]));
  w13 = vzip_u16(vreinterpret_u16_u8(w10.val[1]),
                 vreinterpret_u16_u8(w11.val[1]));

  w6 = vzip_u32(vreinterpret_u32_u16(w4.val[0]),
                vreinterpret_u32_u16(w5.val[0]));
  w7 = vzip_u32(vreinterpret_u32_u16(w4.val[1]),
                vreinterpret_u32_u16(w5.val[1]));
  w14 = vzip_u32(vreinterpret_u32_u16(w12.val[0]),
                 vreinterpret_u32_u16(w13.val[0]));
  w15 = vzip_u32(vreinterpret_u32_u16(w12.val[1]),
                 vreinterpret_u32_u16(w13.val[1]));

  // Store second 4-line result
  d[4] = vcombine_u64(vreinterpret_u64_u32(w6.val[0]),
                      vreinterpret_u64_u32(w14.val[0]));
  d[5] = vcombine_u64(vreinterpret_u64_u32(w6.val[1]),
                      vreinterpret_u64_u32(w14.val[1]));
  d[6] = vcombine_u64(vreinterpret_u64_u32(w7.val[0]),
                      vreinterpret_u64_u32(w15.val[0]));
  d[7] = vcombine_u64(vreinterpret_u64_u32(w7.val[1]),
                      vreinterpret_u64_u32(w15.val[1]));
}

static AOM_FORCE_INLINE void transpose8x16_16x8_neon(uint8x16_t *x,
                                                     uint64x2_t *d) {
  uint8x16x2_t w0, w1, w2, w3;
  uint16x8x2_t w4, w5, w6, w7;
  uint32x4x2_t w8, w9, w10, w11;

  w0 = vzipq_u8(x[0], x[1]);
  w1 = vzipq_u8(x[2], x[3]);
  w2 = vzipq_u8(x[4], x[5]);
  w3 = vzipq_u8(x[6], x[7]);

  w4 = vzipq_u16(vreinterpretq_u16_u8(w0.val[0]),
                 vreinterpretq_u16_u8(w1.val[0]));
  w5 = vzipq_u16(vreinterpretq_u16_u8(w2.val[0]),
                 vreinterpretq_u16_u8(w3.val[0]));
  w6 = vzipq_u16(vreinterpretq_u16_u8(w0.val[1]),
                 vreinterpretq_u16_u8(w1.val[1]));
  w7 = vzipq_u16(vreinterpretq_u16_u8(w2.val[1]),
                 vreinterpretq_u16_u8(w3.val[1]));

  w8 = vzipq_u32(vreinterpretq_u32_u16(w4.val[0]),
                 vreinterpretq_u32_u16(w5.val[0]));
  w9 = vzipq_u32(vreinterpretq_u32_u16(w6.val[0]),
                 vreinterpretq_u32_u16(w7.val[0]));
  w10 = vzipq_u32(vreinterpretq_u32_u16(w4.val[1]),
                  vreinterpretq_u32_u16(w5.val[1]));
  w11 = vzipq_u32(vreinterpretq_u32_u16(w6.val[1]),
                  vreinterpretq_u32_u16(w7.val[1]));

#if defined(__aarch64__)
  d[0] = vzip1q_u64(vreinterpretq_u64_u32(w8.val[0]),
                    vreinterpretq_u64_u32(w9.val[0]));
  d[1] = vzip2q_u64(vreinterpretq_u64_u32(w8.val[0]),
                    vreinterpretq_u64_u32(w9.val[0]));
  d[2] = vzip1q_u64(vreinterpretq_u64_u32(w8.val[1]),
                    vreinterpretq_u64_u32(w9.val[1]));
  d[3] = vzip2q_u64(vreinterpretq_u64_u32(w8.val[1]),
                    vreinterpretq_u64_u32(w9.val[1]));
  d[4] = vzip1q_u64(vreinterpretq_u64_u32(w10.val[0]),
                    vreinterpretq_u64_u32(w11.val[0]));
  d[5] = vzip2q_u64(vreinterpretq_u64_u32(w10.val[0]),
                    vreinterpretq_u64_u32(w11.val[0]));
  d[6] = vzip1q_u64(vreinterpretq_u64_u32(w10.val[1]),
                    vreinterpretq_u64_u32(w11.val[1]));
  d[7] = vzip2q_u64(vreinterpretq_u64_u32(w10.val[1]),
                    vreinterpretq_u64_u32(w11.val[1]));
#else
  d[0] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w8.val[0]), vget_low_u32(w9.val[0])));
  d[1] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w8.val[0]), vget_high_u32(w9.val[0])));
  d[2] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w8.val[1]), vget_low_u32(w9.val[1])));
  d[3] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w8.val[1]), vget_high_u32(w9.val[1])));
  d[4] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w10.val[0]), vget_low_u32(w11.val[0])));
  d[5] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w10.val[0]), vget_high_u32(w11.val[0])));
  d[6] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w10.val[1]), vget_low_u32(w11.val[1])));
  d[7] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w10.val[1]), vget_high_u32(w11.val[1])));
#endif
}

static AOM_FORCE_INLINE void transpose16x16_neon(uint8x16_t *x, uint64x2_t *d) {
  uint8x16x2_t w0, w1, w2, w3, w4, w5, w6, w7;
  uint16x8x2_t w8, w9, w10, w11;
  uint32x4x2_t w12, w13, w14, w15;

  w0 = vzipq_u8(x[0], x[1]);
  w1 = vzipq_u8(x[2], x[3]);
  w2 = vzipq_u8(x[4], x[5]);
  w3 = vzipq_u8(x[6], x[7]);

  w4 = vzipq_u8(x[8], x[9]);
  w5 = vzipq_u8(x[10], x[11]);
  w6 = vzipq_u8(x[12], x[13]);
  w7 = vzipq_u8(x[14], x[15]);

  w8 = vzipq_u16(vreinterpretq_u16_u8(w0.val[0]),
                 vreinterpretq_u16_u8(w1.val[0]));
  w9 = vzipq_u16(vreinterpretq_u16_u8(w2.val[0]),
                 vreinterpretq_u16_u8(w3.val[0]));
  w10 = vzipq_u16(vreinterpretq_u16_u8(w4.val[0]),
                  vreinterpretq_u16_u8(w5.val[0]));
  w11 = vzipq_u16(vreinterpretq_u16_u8(w6.val[0]),
                  vreinterpretq_u16_u8(w7.val[0]));

  w12 = vzipq_u32(vreinterpretq_u32_u16(w8.val[0]),
                  vreinterpretq_u32_u16(w9.val[0]));
  w13 = vzipq_u32(vreinterpretq_u32_u16(w10.val[0]),
                  vreinterpretq_u32_u16(w11.val[0]));
  w14 = vzipq_u32(vreinterpretq_u32_u16(w8.val[1]),
                  vreinterpretq_u32_u16(w9.val[1]));
  w15 = vzipq_u32(vreinterpretq_u32_u16(w10.val[1]),
                  vreinterpretq_u32_u16(w11.val[1]));

#if defined(__aarch64__)
  d[0] = vzip1q_u64(vreinterpretq_u64_u32(w12.val[0]),
                    vreinterpretq_u64_u32(w13.val[0]));
  d[1] = vzip2q_u64(vreinterpretq_u64_u32(w12.val[0]),
                    vreinterpretq_u64_u32(w13.val[0]));
  d[2] = vzip1q_u64(vreinterpretq_u64_u32(w12.val[1]),
                    vreinterpretq_u64_u32(w13.val[1]));
  d[3] = vzip2q_u64(vreinterpretq_u64_u32(w12.val[1]),
                    vreinterpretq_u64_u32(w13.val[1]));
  d[4] = vzip1q_u64(vreinterpretq_u64_u32(w14.val[0]),
                    vreinterpretq_u64_u32(w15.val[0]));
  d[5] = vzip2q_u64(vreinterpretq_u64_u32(w14.val[0]),
                    vreinterpretq_u64_u32(w15.val[0]));
  d[6] = vzip1q_u64(vreinterpretq_u64_u32(w14.val[1]),
                    vreinterpretq_u64_u32(w15.val[1]));
  d[7] = vzip2q_u64(vreinterpretq_u64_u32(w14.val[1]),
                    vreinterpretq_u64_u32(w15.val[1]));
#else
  d[0] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w12.val[0]), vget_low_u32(w13.val[0])));
  d[1] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w12.val[0]), vget_high_u32(w13.val[0])));
  d[2] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w12.val[1]), vget_low_u32(w13.val[1])));
  d[3] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w12.val[1]), vget_high_u32(w13.val[1])));
  d[4] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w14.val[0]), vget_low_u32(w15.val[0])));
  d[5] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w14.val[0]), vget_high_u32(w15.val[0])));
  d[6] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w14.val[1]), vget_low_u32(w15.val[1])));
  d[7] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w14.val[1]), vget_high_u32(w15.val[1])));
#endif

  // upper half
  w8 = vzipq_u16(vreinterpretq_u16_u8(w0.val[1]),
                 vreinterpretq_u16_u8(w1.val[1]));
  w9 = vzipq_u16(vreinterpretq_u16_u8(w2.val[1]),
                 vreinterpretq_u16_u8(w3.val[1]));
  w10 = vzipq_u16(vreinterpretq_u16_u8(w4.val[1]),
                  vreinterpretq_u16_u8(w5.val[1]));
  w11 = vzipq_u16(vreinterpretq_u16_u8(w6.val[1]),
                  vreinterpretq_u16_u8(w7.val[1]));

  w12 = vzipq_u32(vreinterpretq_u32_u16(w8.val[0]),
                  vreinterpretq_u32_u16(w9.val[0]));
  w13 = vzipq_u32(vreinterpretq_u32_u16(w10.val[0]),
                  vreinterpretq_u32_u16(w11.val[0]));
  w14 = vzipq_u32(vreinterpretq_u32_u16(w8.val[1]),
                  vreinterpretq_u32_u16(w9.val[1]));
  w15 = vzipq_u32(vreinterpretq_u32_u16(w10.val[1]),
                  vreinterpretq_u32_u16(w11.val[1]));

#if defined(__aarch64__)
  d[8] = vzip1q_u64(vreinterpretq_u64_u32(w12.val[0]),
                    vreinterpretq_u64_u32(w13.val[0]));
  d[9] = vzip2q_u64(vreinterpretq_u64_u32(w12.val[0]),
                    vreinterpretq_u64_u32(w13.val[0]));
  d[10] = vzip1q_u64(vreinterpretq_u64_u32(w12.val[1]),
                     vreinterpretq_u64_u32(w13.val[1]));
  d[11] = vzip2q_u64(vreinterpretq_u64_u32(w12.val[1]),
                     vreinterpretq_u64_u32(w13.val[1]));
  d[12] = vzip1q_u64(vreinterpretq_u64_u32(w14.val[0]),
                     vreinterpretq_u64_u32(w15.val[0]));
  d[13] = vzip2q_u64(vreinterpretq_u64_u32(w14.val[0]),
                     vreinterpretq_u64_u32(w15.val[0]));
  d[14] = vzip1q_u64(vreinterpretq_u64_u32(w14.val[1]),
                     vreinterpretq_u64_u32(w15.val[1]));
  d[15] = vzip2q_u64(vreinterpretq_u64_u32(w14.val[1]),
                     vreinterpretq_u64_u32(w15.val[1]));
#else
  d[8] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w12.val[0]), vget_low_u32(w13.val[0])));
  d[9] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w12.val[0]), vget_high_u32(w13.val[0])));
  d[10] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w12.val[1]), vget_low_u32(w13.val[1])));
  d[11] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w12.val[1]), vget_high_u32(w13.val[1])));
  d[12] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w14.val[0]), vget_low_u32(w15.val[0])));
  d[13] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w14.val[0]), vget_high_u32(w15.val[0])));
  d[14] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w14.val[1]), vget_low_u32(w15.val[1])));
  d[15] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w14.val[1]), vget_high_u32(w15.val[1])));
#endif
}

static AOM_FORCE_INLINE void transpose16x32_neon(uint8x16x2_t *x,
                                                 uint64x2x2_t *d) {
  uint8x16x2_t w0, w1, w2, w3, w8, w9, w10, w11;
  uint16x8x2_t w4, w5, w12, w13;
  uint32x4x2_t w6, w7, w14, w15;

  w0 = vzipq_u8(x[0].val[0], x[1].val[0]);
  w1 = vzipq_u8(x[2].val[0], x[3].val[0]);
  w2 = vzipq_u8(x[4].val[0], x[5].val[0]);
  w3 = vzipq_u8(x[6].val[0], x[7].val[0]);

  w8 = vzipq_u8(x[8].val[0], x[9].val[0]);
  w9 = vzipq_u8(x[10].val[0], x[11].val[0]);
  w10 = vzipq_u8(x[12].val[0], x[13].val[0]);
  w11 = vzipq_u8(x[14].val[0], x[15].val[0]);

  w4 = vzipq_u16(vreinterpretq_u16_u8(w0.val[0]),
                 vreinterpretq_u16_u8(w1.val[0]));
  w5 = vzipq_u16(vreinterpretq_u16_u8(w2.val[0]),
                 vreinterpretq_u16_u8(w3.val[0]));
  w12 = vzipq_u16(vreinterpretq_u16_u8(w8.val[0]),
                  vreinterpretq_u16_u8(w9.val[0]));
  w13 = vzipq_u16(vreinterpretq_u16_u8(w10.val[0]),
                  vreinterpretq_u16_u8(w11.val[0]));

  w6 = vzipq_u32(vreinterpretq_u32_u16(w4.val[0]),
                 vreinterpretq_u32_u16(w5.val[0]));
  w7 = vzipq_u32(vreinterpretq_u32_u16(w4.val[1]),
                 vreinterpretq_u32_u16(w5.val[1]));
  w14 = vzipq_u32(vreinterpretq_u32_u16(w12.val[0]),
                  vreinterpretq_u32_u16(w13.val[0]));
  w15 = vzipq_u32(vreinterpretq_u32_u16(w12.val[1]),
                  vreinterpretq_u32_u16(w13.val[1]));

  // Store first 4-line result

#if defined(__aarch64__)
  d[0].val[0] = vzip1q_u64(vreinterpretq_u64_u32(w6.val[0]),
                           vreinterpretq_u64_u32(w14.val[0]));
  d[0].val[1] = vzip2q_u64(vreinterpretq_u64_u32(w6.val[0]),
                           vreinterpretq_u64_u32(w14.val[0]));
  d[1].val[0] = vzip1q_u64(vreinterpretq_u64_u32(w6.val[1]),
                           vreinterpretq_u64_u32(w14.val[1]));
  d[1].val[1] = vzip2q_u64(vreinterpretq_u64_u32(w6.val[1]),
                           vreinterpretq_u64_u32(w14.val[1]));
  d[2].val[0] = vzip1q_u64(vreinterpretq_u64_u32(w7.val[0]),
                           vreinterpretq_u64_u32(w15.val[0]));
  d[2].val[1] = vzip2q_u64(vreinterpretq_u64_u32(w7.val[0]),
                           vreinterpretq_u64_u32(w15.val[0]));
  d[3].val[0] = vzip1q_u64(vreinterpretq_u64_u32(w7.val[1]),
                           vreinterpretq_u64_u32(w15.val[1]));
  d[3].val[1] = vzip2q_u64(vreinterpretq_u64_u32(w7.val[1]),
                           vreinterpretq_u64_u32(w15.val[1]));
#else
  d[0].val[0] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w6.val[0]), vget_low_u32(w14.val[0])));
  d[0].val[1] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w6.val[0]), vget_high_u32(w14.val[0])));
  d[1].val[0] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w6.val[1]), vget_low_u32(w14.val[1])));
  d[1].val[1] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w6.val[1]), vget_high_u32(w14.val[1])));
  d[2].val[0] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w7.val[0]), vget_low_u32(w15.val[0])));
  d[2].val[1] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w7.val[0]), vget_high_u32(w15.val[0])));
  d[3].val[0] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w7.val[1]), vget_low_u32(w15.val[1])));
  d[3].val[1] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w7.val[1]), vget_high_u32(w15.val[1])));
#endif

  w4 = vzipq_u16(vreinterpretq_u16_u8(w0.val[1]),
                 vreinterpretq_u16_u8(w1.val[1]));
  w5 = vzipq_u16(vreinterpretq_u16_u8(w2.val[1]),
                 vreinterpretq_u16_u8(w3.val[1]));
  w12 = vzipq_u16(vreinterpretq_u16_u8(w8.val[1]),
                  vreinterpretq_u16_u8(w9.val[1]));
  w13 = vzipq_u16(vreinterpretq_u16_u8(w10.val[1]),
                  vreinterpretq_u16_u8(w11.val[1]));

  w6 = vzipq_u32(vreinterpretq_u32_u16(w4.val[0]),
                 vreinterpretq_u32_u16(w5.val[0]));
  w7 = vzipq_u32(vreinterpretq_u32_u16(w4.val[1]),
                 vreinterpretq_u32_u16(w5.val[1]));
  w14 = vzipq_u32(vreinterpretq_u32_u16(w12.val[0]),
                  vreinterpretq_u32_u16(w13.val[0]));
  w15 = vzipq_u32(vreinterpretq_u32_u16(w12.val[1]),
                  vreinterpretq_u32_u16(w13.val[1]));

  // Store second 4-line result

#if defined(__aarch64__)
  d[4].val[0] = vzip1q_u64(vreinterpretq_u64_u32(w6.val[0]),
                           vreinterpretq_u64_u32(w14.val[0]));
  d[4].val[1] = vzip2q_u64(vreinterpretq_u64_u32(w6.val[0]),
                           vreinterpretq_u64_u32(w14.val[0]));
  d[5].val[0] = vzip1q_u64(vreinterpretq_u64_u32(w6.val[1]),
                           vreinterpretq_u64_u32(w14.val[1]));
  d[5].val[1] = vzip2q_u64(vreinterpretq_u64_u32(w6.val[1]),
                           vreinterpretq_u64_u32(w14.val[1]));
  d[6].val[0] = vzip1q_u64(vreinterpretq_u64_u32(w7.val[0]),
                           vreinterpretq_u64_u32(w15.val[0]));
  d[6].val[1] = vzip2q_u64(vreinterpretq_u64_u32(w7.val[0]),
                           vreinterpretq_u64_u32(w15.val[0]));
  d[7].val[0] = vzip1q_u64(vreinterpretq_u64_u32(w7.val[1]),
                           vreinterpretq_u64_u32(w15.val[1]));
  d[7].val[1] = vzip2q_u64(vreinterpretq_u64_u32(w7.val[1]),
                           vreinterpretq_u64_u32(w15.val[1]));
#else
  d[4].val[0] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w6.val[0]), vget_low_u32(w14.val[0])));
  d[4].val[1] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w6.val[0]), vget_high_u32(w14.val[0])));
  d[5].val[0] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w6.val[1]), vget_low_u32(w14.val[1])));
  d[5].val[1] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w6.val[1]), vget_high_u32(w14.val[1])));
  d[6].val[0] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w7.val[0]), vget_low_u32(w15.val[0])));
  d[6].val[1] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w7.val[0]), vget_high_u32(w15.val[0])));
  d[7].val[0] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w7.val[1]), vget_low_u32(w15.val[1])));
  d[7].val[1] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w7.val[1]), vget_high_u32(w15.val[1])));
#endif

  // upper half
  w0 = vzipq_u8(x[0].val[1], x[1].val[1]);
  w1 = vzipq_u8(x[2].val[1], x[3].val[1]);
  w2 = vzipq_u8(x[4].val[1], x[5].val[1]);
  w3 = vzipq_u8(x[6].val[1], x[7].val[1]);

  w8 = vzipq_u8(x[8].val[1], x[9].val[1]);
  w9 = vzipq_u8(x[10].val[1], x[11].val[1]);
  w10 = vzipq_u8(x[12].val[1], x[13].val[1]);
  w11 = vzipq_u8(x[14].val[1], x[15].val[1]);

  w4 = vzipq_u16(vreinterpretq_u16_u8(w0.val[0]),
                 vreinterpretq_u16_u8(w1.val[0]));
  w5 = vzipq_u16(vreinterpretq_u16_u8(w2.val[0]),
                 vreinterpretq_u16_u8(w3.val[0]));
  w12 = vzipq_u16(vreinterpretq_u16_u8(w8.val[0]),
                  vreinterpretq_u16_u8(w9.val[0]));
  w13 = vzipq_u16(vreinterpretq_u16_u8(w10.val[0]),
                  vreinterpretq_u16_u8(w11.val[0]));

  w6 = vzipq_u32(vreinterpretq_u32_u16(w4.val[0]),
                 vreinterpretq_u32_u16(w5.val[0]));
  w7 = vzipq_u32(vreinterpretq_u32_u16(w4.val[1]),
                 vreinterpretq_u32_u16(w5.val[1]));
  w14 = vzipq_u32(vreinterpretq_u32_u16(w12.val[0]),
                  vreinterpretq_u32_u16(w13.val[0]));
  w15 = vzipq_u32(vreinterpretq_u32_u16(w12.val[1]),
                  vreinterpretq_u32_u16(w13.val[1]));

  // Store first 4-line result

#if defined(__aarch64__)
  d[8].val[0] = vzip1q_u64(vreinterpretq_u64_u32(w6.val[0]),
                           vreinterpretq_u64_u32(w14.val[0]));
  d[8].val[1] = vzip2q_u64(vreinterpretq_u64_u32(w6.val[0]),
                           vreinterpretq_u64_u32(w14.val[0]));
  d[9].val[0] = vzip1q_u64(vreinterpretq_u64_u32(w6.val[1]),
                           vreinterpretq_u64_u32(w14.val[1]));
  d[9].val[1] = vzip2q_u64(vreinterpretq_u64_u32(w6.val[1]),
                           vreinterpretq_u64_u32(w14.val[1]));
  d[10].val[0] = vzip1q_u64(vreinterpretq_u64_u32(w7.val[0]),
                            vreinterpretq_u64_u32(w15.val[0]));
  d[10].val[1] = vzip2q_u64(vreinterpretq_u64_u32(w7.val[0]),
                            vreinterpretq_u64_u32(w15.val[0]));
  d[11].val[0] = vzip1q_u64(vreinterpretq_u64_u32(w7.val[1]),
                            vreinterpretq_u64_u32(w15.val[1]));
  d[11].val[1] = vzip2q_u64(vreinterpretq_u64_u32(w7.val[1]),
                            vreinterpretq_u64_u32(w15.val[1]));
#else
  d[8].val[0] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w6.val[0]), vget_low_u32(w14.val[0])));
  d[8].val[1] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w6.val[0]), vget_high_u32(w14.val[0])));
  d[9].val[0] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w6.val[1]), vget_low_u32(w14.val[1])));
  d[9].val[1] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w6.val[1]), vget_high_u32(w14.val[1])));
  d[10].val[0] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w7.val[0]), vget_low_u32(w15.val[0])));
  d[10].val[1] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w7.val[0]), vget_high_u32(w15.val[0])));
  d[11].val[0] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w7.val[1]), vget_low_u32(w15.val[1])));
  d[11].val[1] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w7.val[1]), vget_high_u32(w15.val[1])));
#endif

  w4 = vzipq_u16(vreinterpretq_u16_u8(w0.val[1]),
                 vreinterpretq_u16_u8(w1.val[1]));
  w5 = vzipq_u16(vreinterpretq_u16_u8(w2.val[1]),
                 vreinterpretq_u16_u8(w3.val[1]));
  w12 = vzipq_u16(vreinterpretq_u16_u8(w8.val[1]),
                  vreinterpretq_u16_u8(w9.val[1]));
  w13 = vzipq_u16(vreinterpretq_u16_u8(w10.val[1]),
                  vreinterpretq_u16_u8(w11.val[1]));

  w6 = vzipq_u32(vreinterpretq_u32_u16(w4.val[0]),
                 vreinterpretq_u32_u16(w5.val[0]));
  w7 = vzipq_u32(vreinterpretq_u32_u16(w4.val[1]),
                 vreinterpretq_u32_u16(w5.val[1]));
  w14 = vzipq_u32(vreinterpretq_u32_u16(w12.val[0]),
                  vreinterpretq_u32_u16(w13.val[0]));
  w15 = vzipq_u32(vreinterpretq_u32_u16(w12.val[1]),
                  vreinterpretq_u32_u16(w13.val[1]));

  // Store second 4-line result

#if defined(__aarch64__)
  d[12].val[0] = vzip1q_u64(vreinterpretq_u64_u32(w6.val[0]),
                            vreinterpretq_u64_u32(w14.val[0]));
  d[12].val[1] = vzip2q_u64(vreinterpretq_u64_u32(w6.val[0]),
                            vreinterpretq_u64_u32(w14.val[0]));
  d[13].val[0] = vzip1q_u64(vreinterpretq_u64_u32(w6.val[1]),
                            vreinterpretq_u64_u32(w14.val[1]));
  d[13].val[1] = vzip2q_u64(vreinterpretq_u64_u32(w6.val[1]),
                            vreinterpretq_u64_u32(w14.val[1]));
  d[14].val[0] = vzip1q_u64(vreinterpretq_u64_u32(w7.val[0]),
                            vreinterpretq_u64_u32(w15.val[0]));
  d[14].val[1] = vzip2q_u64(vreinterpretq_u64_u32(w7.val[0]),
                            vreinterpretq_u64_u32(w15.val[0]));
  d[15].val[0] = vzip1q_u64(vreinterpretq_u64_u32(w7.val[1]),
                            vreinterpretq_u64_u32(w15.val[1]));
  d[15].val[1] = vzip2q_u64(vreinterpretq_u64_u32(w7.val[1]),
                            vreinterpretq_u64_u32(w15.val[1]));
#else
  d[12].val[0] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w6.val[0]), vget_low_u32(w14.val[0])));
  d[12].val[1] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w6.val[0]), vget_high_u32(w14.val[0])));
  d[13].val[0] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w6.val[1]), vget_low_u32(w14.val[1])));
  d[13].val[1] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w6.val[1]), vget_high_u32(w14.val[1])));
  d[14].val[0] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w7.val[0]), vget_low_u32(w15.val[0])));
  d[14].val[1] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w7.val[0]), vget_high_u32(w15.val[0])));
  d[15].val[0] = vreinterpretq_u64_u32(
      vcombine_u32(vget_low_u32(w7.val[1]), vget_low_u32(w15.val[1])));
  d[15].val[1] = vreinterpretq_u64_u32(
      vcombine_u32(vget_high_u32(w7.val[1]), vget_high_u32(w15.val[1])));
#endif
}

static void transpose_TX_16X16(const uint8_t *src, ptrdiff_t pitchSrc,
                               uint8_t *dst, ptrdiff_t pitchDst) {
  uint8x16_t r[16];
  uint64x2_t d[16];
  for (int i = 0; i < 16; i++) {
    r[i] = vld1q_u8(src + i * pitchSrc);
  }
  transpose16x16_neon(r, d);
  for (int i = 0; i < 16; i++) {
    vst1q_u8(dst + i * pitchDst, vreinterpretq_u8_u64(d[i]));
  }
}

static void transpose(const uint8_t *src, ptrdiff_t pitchSrc, uint8_t *dst,
                      ptrdiff_t pitchDst, int width, int height) {
  for (int j = 0; j < height; j += 16) {
    for (int i = 0; i < width; i += 16) {
      transpose_TX_16X16(src + i * pitchSrc + j, pitchSrc,
                         dst + j * pitchDst + i, pitchDst);
    }
  }
}

static void dr_prediction_z3_4x4_neon(uint8_t *dst, ptrdiff_t stride,
                                      const uint8_t *left, int upsample_left,
                                      int dy) {
  uint8x8_t dstvec[4];
  uint16x4x2_t dest;

  dr_prediction_z1_HxW_internal_neon_64(4, 4, dstvec, left, upsample_left, dy);
  transpose4x8_8x4_low_neon(dstvec, &dest);
  vst1_lane_u32((uint32_t *)(dst + stride * 0),
                vreinterpret_u32_u16(dest.val[0]), 0);
  vst1_lane_u32((uint32_t *)(dst + stride * 1),
                vreinterpret_u32_u16(dest.val[0]), 1);
  vst1_lane_u32((uint32_t *)(dst + stride * 2),
                vreinterpret_u32_u16(dest.val[1]), 0);
  vst1_lane_u32((uint32_t *)(dst + stride * 3),
                vreinterpret_u32_u16(dest.val[1]), 1);
}

static void dr_prediction_z3_8x8_neon(uint8_t *dst, ptrdiff_t stride,
                                      const uint8_t *left, int upsample_left,
                                      int dy) {
  uint8x8_t dstvec[8];
  uint32x2x2_t d[4];

  dr_prediction_z1_HxW_internal_neon_64(8, 8, dstvec, left, upsample_left, dy);
  transpose8x8_neon(dstvec, d);
  vst1_u32((uint32_t *)(dst + 0 * stride), d[0].val[0]);
  vst1_u32((uint32_t *)(dst + 1 * stride), d[0].val[1]);
  vst1_u32((uint32_t *)(dst + 2 * stride), d[1].val[0]);
  vst1_u32((uint32_t *)(dst + 3 * stride), d[1].val[1]);
  vst1_u32((uint32_t *)(dst + 4 * stride), d[2].val[0]);
  vst1_u32((uint32_t *)(dst + 5 * stride), d[2].val[1]);
  vst1_u32((uint32_t *)(dst + 6 * stride), d[3].val[0]);
  vst1_u32((uint32_t *)(dst + 7 * stride), d[3].val[1]);
}

static void dr_prediction_z3_4x8_neon(uint8_t *dst, ptrdiff_t stride,
                                      const uint8_t *left, int upsample_left,
                                      int dy) {
  uint8x8_t dstvec[4];
  uint16x4x2_t d[2];

  dr_prediction_z1_HxW_internal_neon_64(8, 4, dstvec, left, upsample_left, dy);
  transpose4x8_8x4_neon(dstvec, d);
  vst1_lane_u32((uint32_t *)(dst + stride * 0),
                vreinterpret_u32_u16(d[0].val[0]), 0);
  vst1_lane_u32((uint32_t *)(dst + stride * 1),
                vreinterpret_u32_u16(d[0].val[0]), 1);
  vst1_lane_u32((uint32_t *)(dst + stride * 2),
                vreinterpret_u32_u16(d[0].val[1]), 0);
  vst1_lane_u32((uint32_t *)(dst + stride * 3),
                vreinterpret_u32_u16(d[0].val[1]), 1);
  vst1_lane_u32((uint32_t *)(dst + stride * 4),
                vreinterpret_u32_u16(d[1].val[0]), 0);
  vst1_lane_u32((uint32_t *)(dst + stride * 5),
                vreinterpret_u32_u16(d[1].val[0]), 1);
  vst1_lane_u32((uint32_t *)(dst + stride * 6),
                vreinterpret_u32_u16(d[1].val[1]), 0);
  vst1_lane_u32((uint32_t *)(dst + stride * 7),
                vreinterpret_u32_u16(d[1].val[1]), 1);
}

static void dr_prediction_z3_8x4_neon(uint8_t *dst, ptrdiff_t stride,
                                      const uint8_t *left, int upsample_left,
                                      int dy) {
  uint8x8_t dstvec[8];
  uint32x2x2_t d[2];

  dr_prediction_z1_HxW_internal_neon_64(4, 8, dstvec, left, upsample_left, dy);
  transpose8x8_low_neon(dstvec, d);
  vst1_u32((uint32_t *)(dst + 0 * stride), d[0].val[0]);
  vst1_u32((uint32_t *)(dst + 1 * stride), d[0].val[1]);
  vst1_u32((uint32_t *)(dst + 2 * stride), d[1].val[0]);
  vst1_u32((uint32_t *)(dst + 3 * stride), d[1].val[1]);
}

static void dr_prediction_z3_8x16_neon(uint8_t *dst, ptrdiff_t stride,
                                       const uint8_t *left, int upsample_left,
                                       int dy) {
  uint8x16_t dstvec[8];
  uint64x2_t d[8];

  dr_prediction_z1_HxW_internal_neon(16, 8, dstvec, left, upsample_left, dy);
  transpose8x16_16x8_neon(dstvec, d);
  for (int i = 0; i < 8; i++) {
    vst1_u8(dst + i * stride, vreinterpret_u8_u64(vget_low_u64(d[i])));
    vst1_u8(dst + (i + 8) * stride, vreinterpret_u8_u64(vget_high_u64(d[i])));
  }
}

static void dr_prediction_z3_16x8_neon(uint8_t *dst, ptrdiff_t stride,
                                       const uint8_t *left, int upsample_left,
                                       int dy) {
  uint8x8_t dstvec[16];
  uint64x2_t d[8];

  dr_prediction_z1_HxW_internal_neon_64(8, 16, dstvec, left, upsample_left, dy);
  transpose16x8_8x16_neon(dstvec, d);
  for (int i = 0; i < 8; i++) {
    vst1q_u8(dst + i * stride, vreinterpretq_u8_u64(d[i]));
  }
}

static void dr_prediction_z3_4x16_neon(uint8_t *dst, ptrdiff_t stride,
                                       const uint8_t *left, int upsample_left,
                                       int dy) {
  uint8x16_t dstvec[4];
  uint16x8x2_t d[2];

  dr_prediction_z1_HxW_internal_neon(16, 4, dstvec, left, upsample_left, dy);
  transpose4x16_neon(dstvec, d);
  vst1q_lane_u32((uint32_t *)(dst + stride * 0),
                 vreinterpretq_u32_u16(d[0].val[0]), 0);
  vst1q_lane_u32((uint32_t *)(dst + stride * 1),
                 vreinterpretq_u32_u16(d[0].val[0]), 1);
  vst1q_lane_u32((uint32_t *)(dst + stride * 2),
                 vreinterpretq_u32_u16(d[0].val[0]), 2);
  vst1q_lane_u32((uint32_t *)(dst + stride * 3),
                 vreinterpretq_u32_u16(d[0].val[0]), 3);

  vst1q_lane_u32((uint32_t *)(dst + stride * 4),
                 vreinterpretq_u32_u16(d[0].val[1]), 0);
  vst1q_lane_u32((uint32_t *)(dst + stride * 5),
                 vreinterpretq_u32_u16(d[0].val[1]), 1);
  vst1q_lane_u32((uint32_t *)(dst + stride * 6),
                 vreinterpretq_u32_u16(d[0].val[1]), 2);
  vst1q_lane_u32((uint32_t *)(dst + stride * 7),
                 vreinterpretq_u32_u16(d[0].val[1]), 3);

  vst1q_lane_u32((uint32_t *)(dst + stride * 8),
                 vreinterpretq_u32_u16(d[1].val[0]), 0);
  vst1q_lane_u32((uint32_t *)(dst + stride * 9),
                 vreinterpretq_u32_u16(d[1].val[0]), 1);
  vst1q_lane_u32((uint32_t *)(dst + stride * 10),
                 vreinterpretq_u32_u16(d[1].val[0]), 2);
  vst1q_lane_u32((uint32_t *)(dst + stride * 11),
                 vreinterpretq_u32_u16(d[1].val[0]), 3);

  vst1q_lane_u32((uint32_t *)(dst + stride * 12),
                 vreinterpretq_u32_u16(d[1].val[1]), 0);
  vst1q_lane_u32((uint32_t *)(dst + stride * 13),
                 vreinterpretq_u32_u16(d[1].val[1]), 1);
  vst1q_lane_u32((uint32_t *)(dst + stride * 14),
                 vreinterpretq_u32_u16(d[1].val[1]), 2);
  vst1q_lane_u32((uint32_t *)(dst + stride * 15),
                 vreinterpretq_u32_u16(d[1].val[1]), 3);
}

static void dr_prediction_z3_16x4_neon(uint8_t *dst, ptrdiff_t stride,
                                       const uint8_t *left, int upsample_left,
                                       int dy) {
  uint8x8_t dstvec[16];
  uint64x2_t d[8];

  dr_prediction_z1_HxW_internal_neon_64(4, 16, dstvec, left, upsample_left, dy);
  transpose16x8_8x16_neon(dstvec, d);
  for (int i = 0; i < 4; i++) {
    vst1q_u8(dst + i * stride, vreinterpretq_u8_u64(d[i]));
  }
}

static void dr_prediction_z3_8x32_neon(uint8_t *dst, ptrdiff_t stride,
                                       const uint8_t *left, int upsample_left,
                                       int dy) {
  uint8x16x2_t dstvec[16];
  uint64x2x2_t d[16];
  uint8x16_t v_zero = vdupq_n_u8(0);

  dr_prediction_z1_32xN_internal_neon(8, dstvec, left, upsample_left, dy);
  for (int i = 8; i < 16; i++) {
    dstvec[i].val[0] = v_zero;
    dstvec[i].val[1] = v_zero;
  }
  transpose16x32_neon(dstvec, d);
  for (int i = 0; i < 16; i++) {
    vst1_u8(dst + 2 * i * stride,
            vreinterpret_u8_u64(vget_low_u64(d[i].val[0])));
    vst1_u8(dst + (2 * i + 1) * stride,
            vreinterpret_u8_u64(vget_low_u64(d[i].val[1])));
  }
}

static void dr_prediction_z3_32x8_neon(uint8_t *dst, ptrdiff_t stride,
                                       const uint8_t *left, int upsample_left,
                                       int dy) {
  uint8x8_t dstvec[32];
  uint64x2_t d[16];

  dr_prediction_z1_HxW_internal_neon_64(8, 32, dstvec, left, upsample_left, dy);
  transpose16x8_8x16_neon(dstvec, d);
  transpose16x8_8x16_neon(dstvec + 16, d + 8);
  for (int i = 0; i < 8; i++) {
    vst1q_u8(dst + i * stride, vreinterpretq_u8_u64(d[i]));
    vst1q_u8(dst + i * stride + 16, vreinterpretq_u8_u64(d[i + 8]));
  }
}

static void dr_prediction_z3_16x16_neon(uint8_t *dst, ptrdiff_t stride,
                                        const uint8_t *left, int upsample_left,
                                        int dy) {
  uint8x16_t dstvec[16];
  uint64x2_t d[16];

  dr_prediction_z1_HxW_internal_neon(16, 16, dstvec, left, upsample_left, dy);
  transpose16x16_neon(dstvec, d);
  for (int i = 0; i < 16; i++) {
    vst1q_u8(dst + i * stride, vreinterpretq_u8_u64(d[i]));
  }
}

static void dr_prediction_z3_32x32_neon(uint8_t *dst, ptrdiff_t stride,
                                        const uint8_t *left, int upsample_left,
                                        int dy) {
  uint8x16x2_t dstvec[32];
  uint64x2x2_t d[32];

  dr_prediction_z1_32xN_internal_neon(32, dstvec, left, upsample_left, dy);
  transpose16x32_neon(dstvec, d);
  transpose16x32_neon(dstvec + 16, d + 16);
  for (int i = 0; i < 16; i++) {
    vst1q_u8(dst + 2 * i * stride, vreinterpretq_u8_u64(d[i].val[0]));
    vst1q_u8(dst + 2 * i * stride + 16, vreinterpretq_u8_u64(d[i + 16].val[0]));
    vst1q_u8(dst + (2 * i + 1) * stride, vreinterpretq_u8_u64(d[i].val[1]));
    vst1q_u8(dst + (2 * i + 1) * stride + 16,
             vreinterpretq_u8_u64(d[i + 16].val[1]));
  }
}

static void dr_prediction_z3_64x64_neon(uint8_t *dst, ptrdiff_t stride,
                                        const uint8_t *left, int upsample_left,
                                        int dy) {
  DECLARE_ALIGNED(16, uint8_t, dstT[64 * 64]);

  dr_prediction_z1_64xN_neon(64, dstT, 64, left, upsample_left, dy);
  transpose(dstT, 64, dst, stride, 64, 64);
}

static void dr_prediction_z3_16x32_neon(uint8_t *dst, ptrdiff_t stride,
                                        const uint8_t *left, int upsample_left,
                                        int dy) {
  uint8x16x2_t dstvec[16];
  uint64x2x2_t d[16];

  dr_prediction_z1_32xN_internal_neon(16, dstvec, left, upsample_left, dy);
  transpose16x32_neon(dstvec, d);
  for (int i = 0; i < 16; i++) {
    vst1q_u8(dst + 2 * i * stride, vreinterpretq_u8_u64(d[i].val[0]));
    vst1q_u8(dst + (2 * i + 1) * stride, vreinterpretq_u8_u64(d[i].val[1]));
  }
}

static void dr_prediction_z3_32x16_neon(uint8_t *dst, ptrdiff_t stride,
                                        const uint8_t *left, int upsample_left,
                                        int dy) {
  uint8x16_t dstvec[32];
  uint64x2_t d[16];

  dr_prediction_z1_HxW_internal_neon(16, 32, dstvec, left, upsample_left, dy);
  for (int i = 0; i < 32; i += 16) {
    transpose16x16_neon((dstvec + i), d);
    for (int j = 0; j < 16; j++) {
      vst1q_u8(dst + j * stride + i, vreinterpretq_u8_u64(d[j]));
    }
  }
}

static void dr_prediction_z3_32x64_neon(uint8_t *dst, ptrdiff_t stride,
                                        const uint8_t *left, int upsample_left,
                                        int dy) {
  uint8_t dstT[64 * 32];

  dr_prediction_z1_64xN_neon(32, dstT, 64, left, upsample_left, dy);
  transpose(dstT, 64, dst, stride, 32, 64);
}

static void dr_prediction_z3_64x32_neon(uint8_t *dst, ptrdiff_t stride,
                                        const uint8_t *left, int upsample_left,
                                        int dy) {
  uint8_t dstT[32 * 64];

  dr_prediction_z1_32xN_neon(64, dstT, 32, left, upsample_left, dy);
  transpose(dstT, 32, dst, stride, 64, 32);
}

static void dr_prediction_z3_16x64_neon(uint8_t *dst, ptrdiff_t stride,
                                        const uint8_t *left, int upsample_left,
                                        int dy) {
  uint8_t dstT[64 * 16];

  dr_prediction_z1_64xN_neon(16, dstT, 64, left, upsample_left, dy);
  transpose(dstT, 64, dst, stride, 16, 64);
}

static void dr_prediction_z3_64x16_neon(uint8_t *dst, ptrdiff_t stride,
                                        const uint8_t *left, int upsample_left,
                                        int dy) {
  uint8x16_t dstvec[64];
  uint64x2_t d[16];

  dr_prediction_z1_HxW_internal_neon(16, 64, dstvec, left, upsample_left, dy);
  for (int i = 0; i < 64; i += 16) {
    transpose16x16_neon((dstvec + i), d);
    for (int j = 0; j < 16; j++) {
      vst1q_u8(dst + j * stride + i, vreinterpretq_u8_u64(d[j]));
    }
  }
}

void av1_dr_prediction_z3_neon(uint8_t *dst, ptrdiff_t stride, int bw, int bh,
                               const uint8_t *above, const uint8_t *left,
                               int upsample_left, int dx, int dy) {
  (void)above;
  (void)dx;
  assert(dx == 1);
  assert(dy > 0);

  if (bw == bh) {
    switch (bw) {
      case 4:
        dr_prediction_z3_4x4_neon(dst, stride, left, upsample_left, dy);
        break;
      case 8:
        dr_prediction_z3_8x8_neon(dst, stride, left, upsample_left, dy);
        break;
      case 16:
        dr_prediction_z3_16x16_neon(dst, stride, left, upsample_left, dy);
        break;
      case 32:
        dr_prediction_z3_32x32_neon(dst, stride, left, upsample_left, dy);
        break;
      case 64:
        dr_prediction_z3_64x64_neon(dst, stride, left, upsample_left, dy);
        break;
    }
  } else {
    if (bw < bh) {
      if (bw + bw == bh) {
        switch (bw) {
          case 4:
            dr_prediction_z3_4x8_neon(dst, stride, left, upsample_left, dy);
            break;
          case 8:
            dr_prediction_z3_8x16_neon(dst, stride, left, upsample_left, dy);
            break;
          case 16:
            dr_prediction_z3_16x32_neon(dst, stride, left, upsample_left, dy);
            break;
          case 32:
            dr_prediction_z3_32x64_neon(dst, stride, left, upsample_left, dy);
            break;
        }
      } else {
        switch (bw) {
          case 4:
            dr_prediction_z3_4x16_neon(dst, stride, left, upsample_left, dy);
            break;
          case 8:
            dr_prediction_z3_8x32_neon(dst, stride, left, upsample_left, dy);
            break;
          case 16:
            dr_prediction_z3_16x64_neon(dst, stride, left, upsample_left, dy);
            break;
        }
      }
    } else {
      if (bh + bh == bw) {
        switch (bh) {
          case 4:
            dr_prediction_z3_8x4_neon(dst, stride, left, upsample_left, dy);
            break;
          case 8:
            dr_prediction_z3_16x8_neon(dst, stride, left, upsample_left, dy);
            break;
          case 16:
            dr_prediction_z3_32x16_neon(dst, stride, left, upsample_left, dy);
            break;
          case 32:
            dr_prediction_z3_64x32_neon(dst, stride, left, upsample_left, dy);
            break;
        }
      } else {
        switch (bh) {
          case 4:
            dr_prediction_z3_16x4_neon(dst, stride, left, upsample_left, dy);
            break;
          case 8:
            dr_prediction_z3_32x8_neon(dst, stride, left, upsample_left, dy);
            break;
          case 16:
            dr_prediction_z3_64x16_neon(dst, stride, left, upsample_left, dy);
            break;
        }
      }
    }
  }
}
static const int sm_weight_log2_scale = 8;

// max(block_size_wide[BLOCK_LARGEST], block_size_high[BLOCK_LARGEST])
#define MAX_BLOCK_DIM 64

/* clang-format off */
static const uint8_t sm_weight_arrays[2 * MAX_BLOCK_DIM] = {
    // Unused, because we always offset by bs, which is at least 2.
    0, 0,
    // bs = 2
    255, 128,
    // bs = 4
    255, 149, 85, 64,
    // bs = 8
    255, 197, 146, 105, 73, 50, 37, 32,
    // bs = 16
    255, 225, 196, 170, 145, 123, 102, 84, 68, 54, 43, 33, 26, 20, 17, 16,
    // bs = 32
    255, 240, 225, 210, 196, 182, 169, 157, 145, 133, 122, 111, 101, 92, 83, 74,
    66, 59, 52, 45, 39, 34, 29, 25, 21, 17, 14, 12, 10, 9, 8, 8,
    // bs = 64
    255, 248, 240, 233, 225, 218, 210, 203, 196, 189, 182, 176, 169, 163, 156,
    150, 144, 138, 133, 127, 121, 116, 111, 106, 101, 96, 91, 86, 82, 77, 73,
    69, 65, 61, 57, 54, 50, 47, 44, 41, 38, 35, 32, 29, 27, 25, 22, 20, 18, 16,
    15, 13, 12, 10, 9, 8, 7, 6, 6, 5, 5, 4, 4, 4,
};
/* clang-format on */

// -----------------------------------------------------------------------------
// SMOOTH_PRED

// pixels[0]: above and below_pred interleave vector
// pixels[1]: left vector
// pixels[2]: right_pred vector
static INLINE void load_pixel_w4(const uint8_t *above, const uint8_t *left,
                                 int height, uint8x16_t *pixels) {
  uint32x4_t zero = vdupq_n_u32(0);
  const uint8x8_t d = vcreate_u8(((const uint32_t *)above)[0]);
  if (height == 4)
    pixels[1] =
        vreinterpretq_u8_u32(vld1q_lane_u32((const uint32_t *)left, zero, 0));
  else if (height == 8) {
    pixels[1] = vreinterpretq_u8_u64(vsetq_lane_u64(
        ((const uint64_t *)left)[0], vreinterpretq_u64_u32(zero), 0));
  } else {
    pixels[1] = vld1q_u8(left);
  }

  pixels[2] = vreinterpretq_u8_u16(vdupq_n_u16(above[3]));

  const uint16x8_t bp = vdupq_n_u16(left[height - 1]);
#if defined(__aarch64__)
  pixels[0] = vreinterpretq_u8_u16(vzip1q_u16(vmovl_u8(d), bp));
#else
  pixels[0] = vreinterpretq_u8_u16(vzipq_u16(vmovl_u8(d), bp).val[0]);
#endif  // (__aarch64__)
}

// weight_h[0]: weight_h vector
// weight_h[1]: scale - weight_h vector
// weight_h[2]: same as [0], second half for height = 16 only
// weight_h[3]: same as [1], second half for height = 16 only
// weight_w[0]: weights_w and scale - weights_w interleave vector
static INLINE void load_weight_w4(const uint8_t *weight_array, int height,
                                  uint16x8_t *weight_h, uint16x8_t *weight_w) {
  const uint16x8_t d = vdupq_n_u16((uint16_t)(1 << sm_weight_log2_scale));
  const uint8x8_t t = vcreate_u8(((const uint32_t *)(weight_array))[1]);
  weight_h[0] = vmovl_u8(t);
  weight_h[1] = vsubw_u8(d, t);
#if defined(__aarch64__)
  weight_w[0] = vzip1q_u16(weight_h[0], weight_h[1]);
#else
  weight_w[0] = vzipq_u16(weight_h[0], weight_h[1]).val[0];
#endif  // (__aarch64__)

  if (height == 8) {
    const uint8x8_t weight = vld1_u8(&weight_array[8]);
    weight_h[0] = vmovl_u8(weight);
    weight_h[1] = vsubw_u8(d, weight);
  } else if (height == 16) {
    const uint8x16_t zero = vdupq_n_u8(0);
    const uint8x16_t weight = vld1q_u8(&weight_array[16]);
    const uint8x16x2_t weight_h_02 = vzipq_u8(weight, zero);
    weight_h[0] = vreinterpretq_u16_u8(weight_h_02.val[0]);
    weight_h[1] = vsubq_u16(d, vreinterpretq_u16_u8(weight_h_02.val[0]));
    weight_h[2] = vreinterpretq_u16_u8(weight_h_02.val[1]);
    weight_h[3] = vsubq_u16(d, vreinterpretq_u16_u8(weight_h_02.val[1]));
  }
}

static INLINE void smooth_pred_4xh(const uint8x16_t *pixel,
                                   const uint16x8_t *wh, const uint16x8_t *ww,
                                   int h, uint8_t *dst, ptrdiff_t stride,
                                   int second_half) {
  const uint16x4_t one = vdup_n_u16(1);
  const uint16x4_t inc = vdup_n_u16(0x202);
  uint16x4_t rep =
      second_half ? vdup_n_u16((uint16_t)0x8008) : vdup_n_u16((uint16_t)0x8000);
  uint16x4_t d = vdup_n_u16(0x100);
  const uint16x4_t v_pixel_0_lo = vmovn_u32(vreinterpretq_u32_u8(pixel[0]));
  const uint16x4_t v_pixel_0_hi =
      vmovn_u32(vreinterpretq_u32_u8(vextq_u8(pixel[0], pixel[0], 2)));
  const uint16x4_t v_pixel_2 = vget_low_u16(vreinterpretq_u16_u8(pixel[2]));
  const uint16x4_t ww_0_lo = vmovn_u32(vreinterpretq_u32_u16(ww[0]));
  const uint16x4_t ww_0_hi =
      vmovn_u32(vreinterpretq_u32_u16(vextq_u16(ww[0], ww[0], 1)));
  const uint8x8_t save_mask = vcreate_u8(0 + (2 << 8) + (4 << 16) + (6 << 24));

#if !defined(__aarch64__)
  const uint8x8x2_t v_split1 = { { vget_low_u8(vreinterpretq_u8_u16(wh[0])),
                                   vget_high_u8(
                                       vreinterpretq_u8_u16(wh[0])) } };
  const uint8x8x2_t v_split2 = { { vget_low_u8(vreinterpretq_u8_u16(wh[1])),
                                   vget_high_u8(
                                       vreinterpretq_u8_u16(wh[1])) } };
  const uint8x8x2_t v_split3 = { { vget_low_u8(pixel[1]),
                                   vget_high_u8(pixel[1]) } };
#endif  // (__aarch64__)

  for (int i = 0; i < h; ++i) {
#if defined(__aarch64__)
    const uint8x8_t wg =
        vqtbl1_u8(vreinterpretq_u8_u16(wh[0]), vreinterpret_u8_u16(d));
    const uint8x8_t sc =
        vqtbl1_u8(vreinterpretq_u8_u16(wh[1]), vreinterpret_u8_u16(d));
#else
    const uint8x8_t wg = vtbl2_u8(v_split1, vreinterpret_u8_u16(d));
    const uint8x8_t sc = vtbl2_u8(v_split2, vreinterpret_u8_u16(d));
#endif  // (__aarch64__)

    uint32x4_t sum = vmull_u16(v_pixel_0_lo, vreinterpret_u16_u8(wg));
    sum = vmlal_u16(sum, v_pixel_0_hi, vreinterpret_u16_u8(sc));

#if defined(__aarch64__)
    uint8x8_t b = vqtbl1_u8(pixel[1], vreinterpret_u8_u16(rep));
#else
    uint8x8_t b = vtbl2_u8(v_split3, vreinterpret_u8_u16(rep));
#endif  // (__aarch64__)

    sum = vmlal_u16(sum, vreinterpret_u16_u8(b), ww_0_lo);
    sum = vmlal_u16(sum, v_pixel_2, ww_0_hi);
    uint8x8_t sum_l = vreinterpret_u8_u16(vqrshrn_n_u32(sum, 9));
    uint32x2_t predsh = vreinterpret_u32_u8(vtbl1_u8(sum_l, save_mask));
    vst1_lane_u32((uint32_t *)dst, predsh, 0);

    dst += stride;

    rep = vadd_u16(rep, one);
    d = vadd_u16(d, inc);
  }
}

void aom_smooth_predictor_4x4_neon(uint8_t *dst, ptrdiff_t stride,
                                   const uint8_t *above, const uint8_t *left) {
  uint8x16_t pixels[3];
  load_pixel_w4(above, left, 4, pixels);

  uint16x8_t wh[4], ww[2];
  load_weight_w4(sm_weight_arrays, 4, wh, ww);

  smooth_pred_4xh(pixels, wh, ww, 4, dst, stride, 0);
}

void aom_smooth_predictor_4x8_neon(uint8_t *dst, ptrdiff_t stride,
                                   const uint8_t *above, const uint8_t *left) {
  uint8x16_t pixels[3];
  load_pixel_w4(above, left, 8, pixels);

  uint16x8_t wh[4], ww[2];
  load_weight_w4(sm_weight_arrays, 8, wh, ww);

  smooth_pred_4xh(pixels, wh, ww, 8, dst, stride, 0);
}

void aom_smooth_predictor_4x16_neon(uint8_t *dst, ptrdiff_t stride,
                                    const uint8_t *above, const uint8_t *left) {
  uint8x16_t pixels[3];
  load_pixel_w4(above, left, 16, pixels);

  uint16x8_t wh[4], ww[2];
  load_weight_w4(sm_weight_arrays, 16, wh, ww);

  smooth_pred_4xh(pixels, wh, ww, 8, dst, stride, 0);
  dst += stride << 3;
  smooth_pred_4xh(pixels, &wh[2], ww, 8, dst, stride, 1);
}

// pixels[0]: above and below_pred interleave vector, first half
// pixels[1]: above and below_pred interleave vector, second half
// pixels[2]: left vector
// pixels[3]: right_pred vector
// pixels[4]: above and below_pred interleave vector, first half
// pixels[5]: above and below_pred interleave vector, second half
// pixels[6]: left vector + 16
// pixels[7]: right_pred vector
static INLINE void load_pixel_w8(const uint8_t *above, const uint8_t *left,
                                 int height, uint8x16_t *pixels) {
  pixels[0] = vreinterpretq_u8_u16(vmovl_u8(vld1_u8(above)));
  pixels[1] = vreinterpretq_u8_u16(vdupq_n_u16((uint16_t)left[height - 1]));
  pixels[3] = vreinterpretq_u8_u16(vdupq_n_u16((uint16_t)above[7]));

  if (height == 4) {
    const uint32x4_t zero32 = vdupq_n_u32(0);
    pixels[2] =
        vreinterpretq_u8_u32(vld1q_lane_u32((const uint32_t *)left, zero32, 0));
  } else if (height == 8) {
    const uint64x2_t zero64 = vdupq_n_u64(0);
    pixels[2] = vreinterpretq_u8_u64(
        vsetq_lane_u64(((const uint64_t *)left)[0], zero64, 0));
  } else if (height == 16) {
    pixels[2] = vld1q_u8(left);
  } else {
    pixels[2] = vld1q_u8(left);
    pixels[4] = pixels[0];
    pixels[5] = pixels[1];
    pixels[6] = vld1q_u8(left + 16);
    pixels[7] = pixels[3];
  }
}

// weight_h[0]: weight_h vector
// weight_h[1]: scale - weight_h vector
// weight_h[2]: same as [0], offset 8
// weight_h[3]: same as [1], offset 8
// weight_h[4]: same as [0], offset 16
// weight_h[5]: same as [1], offset 16
// weight_h[6]: same as [0], offset 24
// weight_h[7]: same as [1], offset 24
// weight_w[0]: weights_w and scale - weights_w interleave vector, first half
// weight_w[1]: weights_w and scale - weights_w interleave vector, second half
static INLINE void load_weight_w8(const uint8_t *weight_array, int height,
                                  uint16x8_t *weight_h, uint16x8_t *weight_w) {
  const uint8x16_t zero = vdupq_n_u8(0);
  const int we_offset = height < 8 ? 4 : 8;
  uint8x16_t we = vld1q_u8(&weight_array[we_offset]);
#if defined(__aarch64__)
  weight_h[0] = vreinterpretq_u16_u8(vzip1q_u8(we, zero));
#else
  weight_h[0] = vreinterpretq_u16_u8(vzipq_u8(we, zero).val[0]);
#endif  // (__aarch64__)
  const uint16x8_t d = vdupq_n_u16(256);
  weight_h[1] = vsubq_u16(d, weight_h[0]);

  if (height == 4) {
    we = vextq_u8(we, zero, 4);
#if defined(__aarch64__)
    weight_w[0] = vreinterpretq_u16_u8(vzip1q_u8(we, zero));
#else
    weight_w[0] = vmovl_u8(vget_low_u8(we));
#endif  // (__aarch64__)
    weight_w[1] = vsubq_u16(d, weight_w[0]);
  } else {
    weight_w[0] = weight_h[0];
    weight_w[1] = weight_h[1];
  }

  if (height == 16) {
    we = vld1q_u8(&weight_array[16]);
    const uint8x16x2_t weight_h_02 = vzipq_u8(we, zero);
    weight_h[0] = vreinterpretq_u16_u8(weight_h_02.val[0]);
    weight_h[1] = vsubq_u16(d, weight_h[0]);
    weight_h[2] = vreinterpretq_u16_u8(weight_h_02.val[1]);
    weight_h[3] = vsubq_u16(d, weight_h[2]);
  } else if (height == 32) {
    const uint8x16_t weight_lo = vld1q_u8(&weight_array[32]);
    const uint8x16x2_t weight_h_02 = vzipq_u8(weight_lo, zero);
    weight_h[0] = vreinterpretq_u16_u8(weight_h_02.val[0]);
    weight_h[1] = vsubq_u16(d, weight_h[0]);
    weight_h[2] = vreinterpretq_u16_u8(weight_h_02.val[1]);
    weight_h[3] = vsubq_u16(d, weight_h[2]);
    const uint8x16_t weight_hi = vld1q_u8(&weight_array[32 + 16]);
    const uint8x16x2_t weight_h_46 = vzipq_u8(weight_hi, zero);
    weight_h[4] = vreinterpretq_u16_u8(weight_h_46.val[0]);
    weight_h[5] = vsubq_u16(d, weight_h[4]);
    weight_h[6] = vreinterpretq_u16_u8(weight_h_46.val[1]);
    weight_h[7] = vsubq_u16(d, weight_h[6]);
  }
}

static INLINE void smooth_pred_8xh(const uint8x16_t *pixels,
                                   const uint16x8_t *wh, const uint16x8_t *ww,
                                   int h, uint8_t *dst, ptrdiff_t stride,
                                   int second_half) {
  const uint16x8_t one = vdupq_n_u16(1);
  const uint16x8_t inc = vdupq_n_u16(0x202);
  uint16x8_t rep = second_half ? vdupq_n_u16((uint16_t)0x8008)
                               : vdupq_n_u16((uint16_t)0x8000);
  uint16x8_t d = vdupq_n_u16(0x100);

#if !defined(__aarch64__)
  const uint8x8x2_t v_split1 = { { vget_low_u8(vreinterpretq_u8_u16(wh[0])),
                                   vget_high_u8(
                                       vreinterpretq_u8_u16(wh[0])) } };
  const uint8x8x2_t v_split2 = { { vget_low_u8(vreinterpretq_u8_u16(wh[1])),
                                   vget_high_u8(
                                       vreinterpretq_u8_u16(wh[1])) } };
  const uint8x8x2_t v_split3 = { { vget_low_u8(pixels[2]),
                                   vget_high_u8(pixels[2]) } };
#endif

  for (int i = 0; i < h; ++i) {
#if defined(__aarch64__)
    const uint8x16_t wg_wg =
        vqtbl1q_u8(vreinterpretq_u8_u16(wh[0]), vreinterpretq_u8_u16(d));
    const uint8x16_t sc_sc =
        vqtbl1q_u8(vreinterpretq_u8_u16(wh[1]), vreinterpretq_u8_u16(d));
#else
    const uint8x8_t v_d_lo = vreinterpret_u8_u16(vget_low_u16(d));
    const uint8x8_t v_d_hi = vreinterpret_u8_u16(vget_high_u16(d));
    const uint8x16_t wg_wg =
        vcombine_u8(vtbl2_u8(v_split1, v_d_lo), vtbl2_u8(v_split1, v_d_hi));
    const uint8x16_t sc_sc =
        vcombine_u8(vtbl2_u8(v_split2, v_d_lo), vtbl2_u8(v_split2, v_d_hi));
#endif  // (__aarch64__)
    uint16x8_t s01 =
        vmulq_u16(vreinterpretq_u16_u8(pixels[0]), vreinterpretq_u16_u8(wg_wg));
    s01 = vmlaq_u16(s01, vreinterpretq_u16_u8(pixels[1]),
                    vreinterpretq_u16_u8(sc_sc));
#if defined(__aarch64__)
    const uint8x16_t b = vqtbl1q_u8(pixels[2], vreinterpretq_u8_u16(rep));
#else
    const uint8x16_t b = vcombine_u8(
        vtbl2_u8(v_split3, vget_low_u8(vreinterpretq_u8_u16(rep))),
        vtbl2_u8(v_split3, vget_high_u8(vreinterpretq_u8_u16(rep))));
#endif  // (__aarch64__)
    uint16x8_t sum0 = vmulq_u16(vreinterpretq_u16_u8(b), ww[0]);
    sum0 = vmlaq_u16(sum0, vreinterpretq_u16_u8(pixels[3]), ww[1]);

    uint32x4_t s0 = vaddl_u16(vget_low_u16(s01), vget_low_u16(sum0));
#if defined(__aarch64__)
    uint32x4_t s1 = vaddl_high_u16(s01, sum0);
#else
    uint32x4_t s1 = vaddl_u16(vget_high_u16(s01), vget_high_u16(sum0));
#endif  // (__aarch64__)

    sum0 = vcombine_u16(vqrshrn_n_u32(s0, 9), vqrshrn_n_u32(s1, 9));
    uint8x8_t predsh = vqmovn_u16(sum0);
    vst1_u8(dst, predsh);

    dst += stride;
    rep = vaddq_u16(rep, one);
    d = vaddq_u16(d, inc);
  }
}

void aom_smooth_predictor_8x4_neon(uint8_t *dst, ptrdiff_t stride,
                                   const uint8_t *above, const uint8_t *left) {
  uint8x16_t pixels[4];
  load_pixel_w8(above, left, 4, pixels);

  uint16x8_t wh[4], ww[2];
  load_weight_w8(sm_weight_arrays, 4, wh, ww);

  smooth_pred_8xh(pixels, wh, ww, 4, dst, stride, 0);
}

void aom_smooth_predictor_8x8_neon(uint8_t *dst, ptrdiff_t stride,
                                   const uint8_t *above, const uint8_t *left) {
  uint8x16_t pixels[4];
  load_pixel_w8(above, left, 8, pixels);

  uint16x8_t wh[4], ww[2];
  load_weight_w8(sm_weight_arrays, 8, wh, ww);

  smooth_pred_8xh(pixels, wh, ww, 8, dst, stride, 0);
}

void aom_smooth_predictor_8x16_neon(uint8_t *dst, ptrdiff_t stride,
                                    const uint8_t *above, const uint8_t *left) {
  uint8x16_t pixels[4];
  load_pixel_w8(above, left, 16, pixels);

  uint16x8_t wh[4], ww[2];
  load_weight_w8(sm_weight_arrays, 16, wh, ww);

  smooth_pred_8xh(pixels, wh, ww, 8, dst, stride, 0);
  dst += stride << 3;
  smooth_pred_8xh(pixels, &wh[2], ww, 8, dst, stride, 1);
}

void aom_smooth_predictor_8x32_neon(uint8_t *dst, ptrdiff_t stride,
                                    const uint8_t *above, const uint8_t *left) {
  uint8x16_t pixels[8];
  load_pixel_w8(above, left, 32, pixels);

  uint16x8_t wh[8], ww[2];
  load_weight_w8(sm_weight_arrays, 32, wh, ww);

  smooth_pred_8xh(&pixels[0], wh, ww, 8, dst, stride, 0);
  dst += stride << 3;
  smooth_pred_8xh(&pixels[0], &wh[2], ww, 8, dst, stride, 1);
  dst += stride << 3;
  smooth_pred_8xh(&pixels[4], &wh[4], ww, 8, dst, stride, 0);
  dst += stride << 3;
  smooth_pred_8xh(&pixels[4], &wh[6], ww, 8, dst, stride, 1);
}

static INLINE void smooth_predictor_wxh(uint8_t *dst, ptrdiff_t stride,
                                        const uint8_t *above,
                                        const uint8_t *left, uint32_t bw,
                                        uint32_t bh) {
  const uint8_t *const sm_weights_w = sm_weight_arrays + bw;
  const uint8_t *const sm_weights_h = sm_weight_arrays + bh;
  const uint16x8_t scale_value = vdupq_n_u16(256);

  for (uint32_t y = 0; y < bh; ++y) {
    const uint8x8_t left_y = vdup_n_u8(left[y]);
    const uint8x8_t weights_y_dup = vdup_n_u8(sm_weights_h[y]);
    const uint32x4_t pred_scaled_bl =
        vdupq_n_u32(256 + (256 - sm_weights_h[y]) * left[bh - 1]);

    for (uint32_t x = 0; x < bw; x += 8) {
      const uint8x8_t weights_x = vld1_u8(sm_weights_w + x);
      const uint8x8_t top_x = vld1_u8(above + x);

      uint16x8_t pred_m1, pred_m2;
      uint32x4_t pred_lo, pred_hi;
      pred_m1 = vmull_u8(top_x, weights_y_dup);
      pred_m2 = vmull_u8(weights_x, left_y);

      pred_lo = vaddl_u16(vget_low_u16(pred_m1), vget_low_u16(pred_m2));
#if defined(__aarch64__)
      pred_hi = vaddl_high_u16(pred_m1, pred_m2);
#else
      pred_hi = vaddl_u16(vget_high_u16(pred_m1), vget_high_u16(pred_m2));
#endif  // (__aarch64__)

      const uint16x8_t scale_m_weights_x = vsubw_u8(scale_value, weights_x);

      const uint16x8_t swxtr = vmulq_n_u16(scale_m_weights_x, above[bw - 1]);

      pred_lo = vaddq_u32(pred_lo, pred_scaled_bl);
      pred_hi = vaddq_u32(pred_hi, pred_scaled_bl);

      pred_lo = vaddw_u16(pred_lo, vget_low_u16(swxtr));
#if defined(__aarch64__)
      pred_hi = vaddw_high_u16(pred_hi, swxtr);
#else
      pred_hi = vaddw_u16(pred_hi, vget_high_u16(swxtr));
#endif  // (__aarch64__)

      uint16x8_t pred =
          vcombine_u16(vshrn_n_u32(pred_lo, 9), vshrn_n_u32(pred_hi, 9));

      uint8x8_t predsh = vqmovn_u16(pred);

      vst1_u8(dst + x, predsh);
    }

    dst += stride;
  }
}

void aom_smooth_predictor_16x4_neon(uint8_t *dst, ptrdiff_t stride,
                                    const uint8_t *above, const uint8_t *left) {
  smooth_predictor_wxh(dst, stride, above, left, 16, 4);
}

void aom_smooth_predictor_16x8_neon(uint8_t *dst, ptrdiff_t stride,
                                    const uint8_t *above, const uint8_t *left) {
  smooth_predictor_wxh(dst, stride, above, left, 16, 8);
}

void aom_smooth_predictor_16x16_neon(uint8_t *dst, ptrdiff_t stride,
                                     const uint8_t *above,
                                     const uint8_t *left) {
  smooth_predictor_wxh(dst, stride, above, left, 16, 16);
}

void aom_smooth_predictor_16x32_neon(uint8_t *dst, ptrdiff_t stride,
                                     const uint8_t *above,
                                     const uint8_t *left) {
  smooth_predictor_wxh(dst, stride, above, left, 16, 32);
}

void aom_smooth_predictor_32x8_neon(uint8_t *dst, ptrdiff_t stride,
                                    const uint8_t *above, const uint8_t *left) {
  smooth_predictor_wxh(dst, stride, above, left, 32, 8);
}

void aom_smooth_predictor_32x16_neon(uint8_t *dst, ptrdiff_t stride,
                                     const uint8_t *above,
                                     const uint8_t *left) {
  smooth_predictor_wxh(dst, stride, above, left, 32, 16);
}

void aom_smooth_predictor_32x32_neon(uint8_t *dst, ptrdiff_t stride,
                                     const uint8_t *above,
                                     const uint8_t *left) {
  smooth_predictor_wxh(dst, stride, above, left, 32, 32);
}

void aom_smooth_predictor_32x64_neon(uint8_t *dst, ptrdiff_t stride,
                                     const uint8_t *above,
                                     const uint8_t *left) {
  smooth_predictor_wxh(dst, stride, above, left, 32, 64);
}

void aom_smooth_predictor_64x64_neon(uint8_t *dst, ptrdiff_t stride,
                                     const uint8_t *above,
                                     const uint8_t *left) {
  smooth_predictor_wxh(dst, stride, above, left, 64, 64);
}

void aom_smooth_predictor_64x32_neon(uint8_t *dst, ptrdiff_t stride,
                                     const uint8_t *above,
                                     const uint8_t *left) {
  smooth_predictor_wxh(dst, stride, above, left, 64, 32);
}

void aom_smooth_predictor_64x16_neon(uint8_t *dst, ptrdiff_t stride,
                                     const uint8_t *above,
                                     const uint8_t *left) {
  smooth_predictor_wxh(dst, stride, above, left, 64, 16);
}

void aom_smooth_predictor_16x64_neon(uint8_t *dst, ptrdiff_t stride,
                                     const uint8_t *above,
                                     const uint8_t *left) {
  smooth_predictor_wxh(dst, stride, above, left, 16, 64);
}
