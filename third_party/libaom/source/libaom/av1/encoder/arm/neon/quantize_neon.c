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

#include <math.h>

#include "aom_dsp/arm/mem_neon.h"
#include "aom_mem/aom_mem.h"

#include "av1/common/quant_common.h"
#include "av1/common/seg_common.h"

#include "av1/encoder/av1_quantize.h"
#include "av1/encoder/encoder.h"
#include "av1/encoder/rd.h"

void av1_quantize_fp_neon(const tran_low_t *coeff_ptr, intptr_t count,
                          const int16_t *zbin_ptr, const int16_t *round_ptr,
                          const int16_t *quant_ptr,
                          const int16_t *quant_shift_ptr,
                          tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr,
                          const int16_t *dequant_ptr, uint16_t *eob_ptr,
                          const int16_t *scan, const int16_t *iscan) {
  // TODO(jingning) Decide the need of these arguments after the
  // quantization process is completed.
  (void)zbin_ptr;
  (void)quant_shift_ptr;
  (void)scan;

  // Quantization pass: All coefficients with index >= zero_flag are
  // skippable. Note: zero_flag can be zero.
  int i;
  const int16x8_t v_zero = vdupq_n_s16(0);
  const int16x8_t v_one = vdupq_n_s16(1);
  int16x8_t v_eobmax_76543210 = vdupq_n_s16(-1);
  int16x8_t v_round = vmovq_n_s16(round_ptr[1]);
  int16x8_t v_quant = vmovq_n_s16(quant_ptr[1]);
  int16x8_t v_dequant = vmovq_n_s16(dequant_ptr[1]);
  // adjust for dc
  v_round = vsetq_lane_s16(round_ptr[0], v_round, 0);
  v_quant = vsetq_lane_s16(quant_ptr[0], v_quant, 0);
  v_dequant = vsetq_lane_s16(dequant_ptr[0], v_dequant, 0);
  // process dc and the first seven ac coeffs
  {
    const int16x8_t v_iscan = vld1q_s16(&iscan[0]);
    const int16x8_t v_coeff = load_tran_low_to_s16q(&coeff_ptr[0]);
    const int16x8_t v_coeff_sign = vshrq_n_s16(v_coeff, 15);
    const int16x8_t v_abs = vabsq_s16(v_coeff);
    const int16x8_t v_tmp = vqaddq_s16(v_abs, v_round);
    const int32x4_t v_tmp_lo =
        vmull_s16(vget_low_s16(v_tmp), vget_low_s16(v_quant));
    const int32x4_t v_tmp_hi =
        vmull_s16(vget_high_s16(v_tmp), vget_high_s16(v_quant));
    const int16x8_t v_tmp2 =
        vcombine_s16(vshrn_n_s32(v_tmp_lo, 16), vshrn_n_s32(v_tmp_hi, 16));
    const uint16x8_t v_nz_mask = vceqq_s16(v_tmp2, v_zero);
    const int16x8_t v_iscan_plus1 = vaddq_s16(v_iscan, v_one);
    const int16x8_t v_nz_iscan = vbslq_s16(v_nz_mask, v_zero, v_iscan_plus1);
    const int16x8_t v_qcoeff_a = veorq_s16(v_tmp2, v_coeff_sign);
    const int16x8_t v_qcoeff = vsubq_s16(v_qcoeff_a, v_coeff_sign);
    const int16x8_t v_dqcoeff = vmulq_s16(v_qcoeff, v_dequant);
    v_eobmax_76543210 = vmaxq_s16(v_eobmax_76543210, v_nz_iscan);
    store_s16q_to_tran_low(&qcoeff_ptr[0], v_qcoeff);
    store_s16q_to_tran_low(&dqcoeff_ptr[0], v_dqcoeff);
    v_round = vmovq_n_s16(round_ptr[1]);
    v_quant = vmovq_n_s16(quant_ptr[1]);
    v_dequant = vmovq_n_s16(dequant_ptr[1]);
  }
  // now process the rest of the ac coeffs
  for (i = 8; i < count; i += 8) {
    const int16x8_t v_iscan = vld1q_s16(&iscan[i]);
    const int16x8_t v_coeff = load_tran_low_to_s16q(&coeff_ptr[i]);
    const int16x8_t v_coeff_sign = vshrq_n_s16(v_coeff, 15);
    const int16x8_t v_abs = vabsq_s16(v_coeff);
    const int16x8_t v_tmp = vqaddq_s16(v_abs, v_round);
    const int32x4_t v_tmp_lo =
        vmull_s16(vget_low_s16(v_tmp), vget_low_s16(v_quant));
    const int32x4_t v_tmp_hi =
        vmull_s16(vget_high_s16(v_tmp), vget_high_s16(v_quant));
    const int16x8_t v_tmp2 =
        vcombine_s16(vshrn_n_s32(v_tmp_lo, 16), vshrn_n_s32(v_tmp_hi, 16));
    const uint16x8_t v_nz_mask = vceqq_s16(v_tmp2, v_zero);
    const int16x8_t v_iscan_plus1 = vaddq_s16(v_iscan, v_one);
    const int16x8_t v_nz_iscan = vbslq_s16(v_nz_mask, v_zero, v_iscan_plus1);
    const int16x8_t v_qcoeff_a = veorq_s16(v_tmp2, v_coeff_sign);
    const int16x8_t v_qcoeff = vsubq_s16(v_qcoeff_a, v_coeff_sign);
    const int16x8_t v_dqcoeff = vmulq_s16(v_qcoeff, v_dequant);
    v_eobmax_76543210 = vmaxq_s16(v_eobmax_76543210, v_nz_iscan);
    store_s16q_to_tran_low(&qcoeff_ptr[i], v_qcoeff);
    store_s16q_to_tran_low(&dqcoeff_ptr[i], v_dqcoeff);
  }
#ifdef __aarch64__
  *eob_ptr = vmaxvq_s16(v_eobmax_76543210);
#else
  {
    const int16x4_t v_eobmax_3210 = vmax_s16(vget_low_s16(v_eobmax_76543210),
                                             vget_high_s16(v_eobmax_76543210));
    const int64x1_t v_eobmax_xx32 =
        vshr_n_s64(vreinterpret_s64_s16(v_eobmax_3210), 32);
    const int16x4_t v_eobmax_tmp =
        vmax_s16(v_eobmax_3210, vreinterpret_s16_s64(v_eobmax_xx32));
    const int64x1_t v_eobmax_xxx3 =
        vshr_n_s64(vreinterpret_s64_s16(v_eobmax_tmp), 16);
    const int16x4_t v_eobmax_final =
        vmax_s16(v_eobmax_tmp, vreinterpret_s16_s64(v_eobmax_xxx3));

    *eob_ptr = (uint16_t)vget_lane_s16(v_eobmax_final, 0);
  }
#endif  // __aarch64__
}

static INLINE void calculate_dqcoeff_lp_and_store(const int16x8_t qcoeff,
                                                  const int16x8_t dequant,
                                                  int16_t *dqcoeff) {
  const int32x4_t dqcoeff_0 =
      vmull_s16(vget_low_s16(qcoeff), vget_low_s16(dequant));
  const int32x4_t dqcoeff_1 =
      vmull_s16(vget_high_s16(qcoeff), vget_high_s16(dequant));

  vst1q_s16(dqcoeff, vcombine_s16(vmovn_s32(dqcoeff_0), vmovn_s32(dqcoeff_1)));
}

void av1_quantize_lp_neon(const int16_t *coeff_ptr, intptr_t n_coeffs,
                          const int16_t *round_ptr, const int16_t *quant_ptr,
                          int16_t *qcoeff_ptr, int16_t *dqcoeff_ptr,
                          const int16_t *dequant_ptr, uint16_t *eob_ptr,
                          const int16_t *scan, const int16_t *iscan) {
  (void)scan;
  // Quantization pass: All coefficients with index >= zero_flag are
  // skippable. Note: zero_flag can be zero.
  const int16x8_t v_zero = vdupq_n_s16(0);
  const int16x8_t v_one = vdupq_n_s16(1);
  int16x8_t v_eobmax_76543210 = vdupq_n_s16(-1);
  int16x8_t v_round = vmovq_n_s16(round_ptr[1]);
  int16x8_t v_quant = vmovq_n_s16(quant_ptr[1]);
  int16x8_t v_dequant = vmovq_n_s16(dequant_ptr[1]);

  // adjust for dc
  v_round = vsetq_lane_s16(round_ptr[0], v_round, 0);
  v_quant = vsetq_lane_s16(quant_ptr[0], v_quant, 0);
  v_dequant = vsetq_lane_s16(dequant_ptr[0], v_dequant, 0);
  // process dc and the first seven ac coeffs
  {
    const int16x8_t v_iscan = vld1q_s16(&iscan[0]);
    const int16x8_t v_coeff = vld1q_s16(coeff_ptr);
    const int16x8_t v_coeff_sign = vshrq_n_s16(v_coeff, 15);
    const int16x8_t v_abs = vabsq_s16(v_coeff);
    const int16x8_t v_tmp = vqaddq_s16(v_abs, v_round);
    const int32x4_t v_tmp_lo =
        vmull_s16(vget_low_s16(v_tmp), vget_low_s16(v_quant));
    const int32x4_t v_tmp_hi =
        vmull_s16(vget_high_s16(v_tmp), vget_high_s16(v_quant));
    const int16x8_t v_tmp2 =
        vcombine_s16(vshrn_n_s32(v_tmp_lo, 16), vshrn_n_s32(v_tmp_hi, 16));
    const uint16x8_t v_nz_mask = vceqq_s16(v_tmp2, v_zero);
    const int16x8_t v_iscan_plus1 = vaddq_s16(v_iscan, v_one);
    const int16x8_t v_nz_iscan = vbslq_s16(v_nz_mask, v_zero, v_iscan_plus1);
    const int16x8_t v_qcoeff_a = veorq_s16(v_tmp2, v_coeff_sign);
    const int16x8_t v_qcoeff = vsubq_s16(v_qcoeff_a, v_coeff_sign);
    calculate_dqcoeff_lp_and_store(v_qcoeff, v_dequant, dqcoeff_ptr);
    v_eobmax_76543210 = vmaxq_s16(v_eobmax_76543210, v_nz_iscan);
    vst1q_s16(qcoeff_ptr, v_qcoeff);
    v_round = vmovq_n_s16(round_ptr[1]);
    v_quant = vmovq_n_s16(quant_ptr[1]);
    v_dequant = vmovq_n_s16(dequant_ptr[1]);
  }
  // now process the rest of the ac coeffs
  for (int i = 8; i < n_coeffs; i += 8) {
    const int16x8_t v_iscan = vld1q_s16(&iscan[i]);
    const int16x8_t v_coeff = vld1q_s16(coeff_ptr + i);
    const int16x8_t v_coeff_sign = vshrq_n_s16(v_coeff, 15);
    const int16x8_t v_abs = vabsq_s16(v_coeff);
    const int16x8_t v_tmp = vqaddq_s16(v_abs, v_round);
    const int32x4_t v_tmp_lo =
        vmull_s16(vget_low_s16(v_tmp), vget_low_s16(v_quant));
    const int32x4_t v_tmp_hi =
        vmull_s16(vget_high_s16(v_tmp), vget_high_s16(v_quant));
    const int16x8_t v_tmp2 =
        vcombine_s16(vshrn_n_s32(v_tmp_lo, 16), vshrn_n_s32(v_tmp_hi, 16));
    const uint16x8_t v_nz_mask = vceqq_s16(v_tmp2, v_zero);
    const int16x8_t v_iscan_plus1 = vaddq_s16(v_iscan, v_one);
    const int16x8_t v_nz_iscan = vbslq_s16(v_nz_mask, v_zero, v_iscan_plus1);
    const int16x8_t v_qcoeff_a = veorq_s16(v_tmp2, v_coeff_sign);
    const int16x8_t v_qcoeff = vsubq_s16(v_qcoeff_a, v_coeff_sign);
    calculate_dqcoeff_lp_and_store(v_qcoeff, v_dequant, dqcoeff_ptr + i);
    v_eobmax_76543210 = vmaxq_s16(v_eobmax_76543210, v_nz_iscan);
    vst1q_s16(qcoeff_ptr + i, v_qcoeff);
  }
#ifdef __aarch64__
  *eob_ptr = vmaxvq_s16(v_eobmax_76543210);
#else
  {
    const int16x4_t v_eobmax_3210 = vmax_s16(vget_low_s16(v_eobmax_76543210),
                                             vget_high_s16(v_eobmax_76543210));
    const int64x1_t v_eobmax_xx32 =
        vshr_n_s64(vreinterpret_s64_s16(v_eobmax_3210), 32);
    const int16x4_t v_eobmax_tmp =
        vmax_s16(v_eobmax_3210, vreinterpret_s16_s64(v_eobmax_xx32));
    const int64x1_t v_eobmax_xxx3 =
        vshr_n_s64(vreinterpret_s64_s16(v_eobmax_tmp), 16);
    const int16x4_t v_eobmax_final =
        vmax_s16(v_eobmax_tmp, vreinterpret_s16_s64(v_eobmax_xxx3));

    *eob_ptr = (uint16_t)vget_lane_s16(v_eobmax_final, 0);
  }
#endif  // __aarch64__
}

void av1_quantize_fp_32x32_neon(const tran_low_t *coeff_ptr, intptr_t n_coeffs,
                                const int16_t *zbin_ptr,
                                const int16_t *round_ptr,
                                const int16_t *quant_ptr,
                                const int16_t *quant_shift_ptr,
                                tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr,
                                const int16_t *dequant_ptr, uint16_t *eob_ptr,
                                const int16_t *scan, const int16_t *iscan) {
  const int log_scale = 1;
  const int rounding[2] = { ROUND_POWER_OF_TWO(round_ptr[0], log_scale),
                            ROUND_POWER_OF_TWO(round_ptr[1], log_scale) };

  (void)zbin_ptr;
  (void)quant_shift_ptr;
  (void)scan;

  memset(qcoeff_ptr, 0, n_coeffs * sizeof(*qcoeff_ptr));
  memset(dqcoeff_ptr, 0, n_coeffs * sizeof(*dqcoeff_ptr));

  const int16x8_t zero = vdupq_n_s16(0);
  int16x8_t v_eobmax_76543210 = vreinterpretq_s16_u16(vceqq_s16(zero, zero));
  int16x8_t round = vdupq_n_s16(rounding[1]);
  int16x8_t quant = vdupq_n_s16(quant_ptr[1]);
  int16x8_t dequant = vdupq_n_s16(dequant_ptr[1]);
  dequant = vsetq_lane_s16(dequant_ptr[0], dequant, 0);

  int16x8_t coeff = load_tran_low_to_s16q(&coeff_ptr[0]);

  int16x8_t abs = vabsq_s16(coeff);
  uint16x8_t check = vcgeq_s16(abs, vshrq_n_s16(dequant, 2));
  uint64_t nz_check = vget_lane_u64(vreinterpret_u64_u8(vmovn_u16(check)), 0);
  if (nz_check) {
    const int16x8_t coeff_sign = vshrq_n_s16(coeff, 15);
    const int16x8_t v_iscan = vld1q_s16(&iscan[0]);
    round = vsetq_lane_s16(rounding[0], round, 0);
    quant = vsetq_lane_s16(quant_ptr[0], quant, 0);

    abs = vqaddq_s16(abs, round);
    int16x8_t temp = vqdmulhq_s16(abs, quant);
    int16x8_t qcoeff_temp = vsubq_s16(veorq_s16(temp, coeff_sign), coeff_sign);
    abs = vreinterpretq_s16_u16(
        vshrq_n_u16(vreinterpretq_u16_s16(vmulq_s16(temp, dequant)), 1));
    int16x8_t dqcoeff_temp = vsubq_s16(veorq_s16(abs, coeff_sign), coeff_sign);

    int16x8_t coeff_nz_mask =
        vbslq_s16(check, qcoeff_temp, load_tran_low_to_s16q(&qcoeff_ptr[0]));
    store_s16q_to_tran_low(&qcoeff_ptr[0], coeff_nz_mask);
    coeff_nz_mask =
        vbslq_s16(check, dqcoeff_temp, load_tran_low_to_s16q(&dqcoeff_ptr[0]));
    store_s16q_to_tran_low(&dqcoeff_ptr[0], coeff_nz_mask);

    round = vsetq_lane_s16(rounding[1], round, 0);
    quant = vsetq_lane_s16(quant_ptr[1], quant, 0);

    uint16x8_t vtmp_mask = vcgtq_s16(abs, zero);
    const uint16x8_t v_nz_mask = vandq_u16(vtmp_mask, check);
    check = vandq_u16(v_nz_mask, vcgtq_s16(v_iscan, v_eobmax_76543210));
    v_eobmax_76543210 = vbslq_s16(check, v_iscan, v_eobmax_76543210);
  }

  dequant = vsetq_lane_s16(dequant_ptr[1], dequant, 0);

  for (int i = 8; i < n_coeffs; i += 8) {
    coeff = load_tran_low_to_s16q(&coeff_ptr[i]);
    abs = vabsq_s16(coeff);
    check = vcgeq_s16(abs, vshrq_n_s16(dequant, 2));

    nz_check = vget_lane_u64(vreinterpret_u64_u8(vmovn_u16(check)), 0);
    if (nz_check) {
      const int16x8_t coeff_sign = vshrq_n_s16(coeff, 15);
      const int16x8_t v_iscan = vld1q_s16(&iscan[i]);

      abs = vqaddq_s16(abs, round);
      int16x8_t temp = vqdmulhq_s16(abs, quant);
      int16x8_t qcoeff_temp =
          vsubq_s16(veorq_s16(temp, coeff_sign), coeff_sign);
      abs = vreinterpretq_s16_u16(
          vshrq_n_u16(vreinterpretq_u16_s16(vmulq_s16(temp, dequant)), 1));
      int16x8_t dqcoeff_temp =
          vsubq_s16(veorq_s16(abs, coeff_sign), coeff_sign);

      int16x8_t coeff_nz_mask =
          vbslq_s16(check, qcoeff_temp, load_tran_low_to_s16q(&qcoeff_ptr[i]));
      store_s16q_to_tran_low(&qcoeff_ptr[i], coeff_nz_mask);
      coeff_nz_mask = vbslq_s16(check, dqcoeff_temp,
                                load_tran_low_to_s16q(&dqcoeff_ptr[i]));
      store_s16q_to_tran_low(&dqcoeff_ptr[i], coeff_nz_mask);

      uint16x8_t vtmp_mask = vcgtq_s16(abs, zero);
      const uint16x8_t v_nz_mask = vandq_u16(vtmp_mask, check);
      check = vandq_u16(v_nz_mask, vcgtq_s16(v_iscan, v_eobmax_76543210));
      v_eobmax_76543210 = vbslq_s16(check, v_iscan, v_eobmax_76543210);
    }
  }
#ifdef __aarch64__
  *eob_ptr = vmaxvq_s16(v_eobmax_76543210) + 1;
#else
  {
    const int16x4_t v_eobmax_3210 = vmax_s16(vget_low_s16(v_eobmax_76543210),
                                             vget_high_s16(v_eobmax_76543210));
    const int64x1_t v_eobmax_xx32 =
        vshr_n_s64(vreinterpret_s64_s16(v_eobmax_3210), 32);
    const int16x4_t v_eobmax_tmp =
        vmax_s16(v_eobmax_3210, vreinterpret_s16_s64(v_eobmax_xx32));
    const int64x1_t v_eobmax_xxx3 =
        vshr_n_s64(vreinterpret_s64_s16(v_eobmax_tmp), 16);
    const int16x4_t v_eobmax_final =
        vmax_s16(v_eobmax_tmp, vreinterpret_s16_s64(v_eobmax_xxx3));

    *eob_ptr = (uint16_t)vget_lane_s16(v_eobmax_final, 0) + 1;
  }
#endif  // __aarch64__
}

void av1_quantize_fp_64x64_neon(const tran_low_t *coeff_ptr, intptr_t n_coeffs,
                                const int16_t *zbin_ptr,
                                const int16_t *round_ptr,
                                const int16_t *quant_ptr,
                                const int16_t *quant_shift_ptr,
                                tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr,
                                const int16_t *dequant_ptr, uint16_t *eob_ptr,
                                const int16_t *scan, const int16_t *iscan) {
  const int log_scale = 2;
  const int16x8_t v_log_scale =
      vreinterpretq_s16_s64(vdupq_n_s64(0xFFFEFFFEFFFEFFFE));

  const int rounding[2] = { ROUND_POWER_OF_TWO(round_ptr[0], log_scale),
                            ROUND_POWER_OF_TWO(round_ptr[1], log_scale) };

  (void)zbin_ptr;
  (void)quant_shift_ptr;
  (void)scan;

  memset(qcoeff_ptr, 0, n_coeffs * sizeof(*qcoeff_ptr));
  memset(dqcoeff_ptr, 0, n_coeffs * sizeof(*dqcoeff_ptr));

  const int16x8_t zero = vdupq_n_s16(0);
  int16x8_t v_eobmax_76543210 = vreinterpretq_s16_u16(vceqq_s16(zero, zero));

  int16x8_t round = vdupq_n_s16(rounding[1]);
  int16x8_t quant = vdupq_n_s16(quant_ptr[1]);
  int16x8_t dequant = vdupq_n_s16(dequant_ptr[1]);
  dequant = vsetq_lane_s16(dequant_ptr[0], dequant, 0);

  int16x8_t coeff = load_tran_low_to_s16q(&coeff_ptr[0]);
  int16x8_t abs = vabsq_s16(coeff);
  uint16x8_t check = vcgeq_u16(vshlq_n_u16(vreinterpretq_u16_s16(abs), 1),
                               vshrq_n_u16(vreinterpretq_u16_s16(dequant), 2));
  uint64_t nz_check = vget_lane_u64(vreinterpret_u64_u8(vmovn_u16(check)), 0);
  if (nz_check) {
    const int16x8_t coeff_sign = vshrq_n_s16(coeff, 15);
    const int16x8_t v_iscan = vld1q_s16(&iscan[0]);
    round = vsetq_lane_s16(rounding[0], round, 0);
    quant = vsetq_lane_s16(quant_ptr[0], quant, 0);
    abs = vqaddq_s16(abs, round);
    int16x8_t temp =
        vorrq_s16(vshlq_n_s16(vqdmulhq_s16(abs, quant), 1),
                  vreinterpretq_s16_u16(vshrq_n_u16(
                      vreinterpretq_u16_s16(vmulq_s16(abs, quant)), 14)));
    int16x8_t qcoeff_temp = vsubq_s16(veorq_s16(temp, coeff_sign), coeff_sign);

    abs = vreinterpretq_s16_u16(vshlq_u16(
        vreinterpretq_u16_s16(vmulq_s16(temp, dequant)), v_log_scale));
    abs = vorrq_s16(vshlq_n_s16(vqdmulhq_s16(temp, dequant), 13), abs);
    int16x8_t dqcoeff_temp = vsubq_s16(veorq_s16(abs, coeff_sign), coeff_sign);
    int16x8_t coeff_nz_mask =
        vbslq_s16(check, qcoeff_temp, load_tran_low_to_s16q(&qcoeff_ptr[0]));
    store_s16q_to_tran_low(&qcoeff_ptr[0], coeff_nz_mask);
    coeff_nz_mask =
        vbslq_s16(check, dqcoeff_temp, load_tran_low_to_s16q(&dqcoeff_ptr[0]));
    store_s16q_to_tran_low(&dqcoeff_ptr[0], coeff_nz_mask);

    round = vsetq_lane_s16(rounding[1], round, 0);
    quant = vsetq_lane_s16(quant_ptr[1], quant, 0);

    uint16x8_t vtmp_mask = vcgtq_s16(abs, zero);
    const uint16x8_t v_nz_mask = vandq_u16(vtmp_mask, check);
    check = vandq_u16(v_nz_mask, vcgtq_s16(v_iscan, v_eobmax_76543210));
    v_eobmax_76543210 = vbslq_s16(check, v_iscan, v_eobmax_76543210);
  }

  dequant = vsetq_lane_s16(dequant_ptr[1], dequant, 0);

  for (int i = 8; i < n_coeffs; i += 8) {
    coeff = load_tran_low_to_s16q(&coeff_ptr[i]);
    abs = vabsq_s16(coeff);
    check = vcgeq_u16(vshlq_n_u16(vreinterpretq_u16_s16(abs), 1),
                      vshrq_n_u16(vreinterpretq_u16_s16(dequant), 2));
    nz_check = vget_lane_u64(vreinterpret_u64_u8(vmovn_u16(check)), 0);
    if (nz_check) {
      const int16x8_t coeff_sign = vshrq_n_s16(coeff, 15);
      const int16x8_t v_iscan = vld1q_s16(&iscan[i]);
      abs = vqaddq_s16(abs, round);
      int16x8_t temp =
          vorrq_s16(vshlq_n_s16(vqdmulhq_s16(abs, quant), 1),
                    vreinterpretq_s16_u16(vshrq_n_u16(
                        vreinterpretq_u16_s16(vmulq_s16(abs, quant)), 14)));

      int16x8_t qcoeff_temp =
          vsubq_s16(veorq_s16(temp, coeff_sign), coeff_sign);

      abs = vreinterpretq_s16_u16(vshlq_u16(
          vreinterpretq_u16_s16(vmulq_s16(temp, dequant)), v_log_scale));
      abs = vorrq_s16(vshlq_n_s16(vqdmulhq_s16(temp, dequant), 13), abs);

      int16x8_t dqcoeff_temp =
          vsubq_s16(veorq_s16(abs, coeff_sign), coeff_sign);
      int16x8_t coeff_nz_mask =
          vbslq_s16(check, qcoeff_temp, load_tran_low_to_s16q(&qcoeff_ptr[i]));
      store_s16q_to_tran_low(&qcoeff_ptr[i], coeff_nz_mask);
      coeff_nz_mask = vbslq_s16(check, dqcoeff_temp,
                                load_tran_low_to_s16q(&dqcoeff_ptr[i]));
      store_s16q_to_tran_low(&dqcoeff_ptr[i], coeff_nz_mask);

      uint16x8_t vtmp_mask = vcgtq_s16(abs, zero);
      const uint16x8_t v_nz_mask = vandq_u16(vtmp_mask, check);

      check = vandq_u16(v_nz_mask, vcgtq_s16(v_iscan, v_eobmax_76543210));
      v_eobmax_76543210 = vbslq_s16(check, v_iscan, v_eobmax_76543210);
    }
  }
#ifdef __aarch64__
  *eob_ptr = vmaxvq_s16(v_eobmax_76543210) + 1;
#else
  {
    const int16x4_t v_eobmax_3210 = vmax_s16(vget_low_s16(v_eobmax_76543210),
                                             vget_high_s16(v_eobmax_76543210));
    const int64x1_t v_eobmax_xx32 =
        vshr_n_s64(vreinterpret_s64_s16(v_eobmax_3210), 32);
    const int16x4_t v_eobmax_tmp =
        vmax_s16(v_eobmax_3210, vreinterpret_s16_s64(v_eobmax_xx32));
    const int64x1_t v_eobmax_xxx3 =
        vshr_n_s64(vreinterpret_s64_s16(v_eobmax_tmp), 16);
    const int16x4_t v_eobmax_final =
        vmax_s16(v_eobmax_tmp, vreinterpret_s16_s64(v_eobmax_xxx3));

    *eob_ptr = (uint16_t)vget_lane_s16(v_eobmax_final, 0) + 1;
  }
#endif  // __aarch64__
}

void aom_quantize_b_neon(const tran_low_t *coeff_ptr, intptr_t n_coeffs,
                         const int16_t *zbin_ptr, const int16_t *round_ptr,
                         const int16_t *quant_ptr,
                         const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr,
                         tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr,
                         uint16_t *eob_ptr, const int16_t *scan,
                         const int16_t *iscan) {
  (void)quant_shift_ptr;
  (void)scan;

  const int zbins[2] = { zbin_ptr[0], zbin_ptr[1] };

  memset(qcoeff_ptr, 0, n_coeffs * sizeof(*qcoeff_ptr));
  memset(dqcoeff_ptr, 0, n_coeffs * sizeof(*dqcoeff_ptr));

  const int16x8_t zero = vdupq_n_s16(0);
  int16x8_t v_eobmax_76543210 = vreinterpretq_s16_u16(vceqq_s16(zero, zero));

  int16x8_t vzbins = vdupq_n_s16(zbins[1]), vround = vdupq_n_s16(round_ptr[1]);
  int16x8_t vdequant = vdupq_n_s16(dequant_ptr[1]);
  int16x8_t vquant = vdupq_n_s16(quant_ptr[1]);
  int16x8_t vquant_shift = vdupq_n_s16(quant_shift_ptr[1]);

  int16x8_t v_coeff = load_tran_low_to_s16q(&coeff_ptr[0]);
  int16x8_t v_coeff_sign = vshrq_n_s16(v_coeff, 15);
  int16x8_t v_abs = vabsq_s16(v_coeff);

  vzbins = vsetq_lane_s16(zbins[0], vzbins, 0);

  uint16x8_t vcond = vcgeq_s16(v_abs, vzbins);
  uint64_t nz_check = vget_lane_u64(vreinterpret_u64_u8(vmovn_u16(vcond)), 0);
  if (nz_check) {
    vround = vsetq_lane_s16(round_ptr[0], vround, 0);
    vquant = vsetq_lane_s16(quant_ptr[0], vquant, 0);
    vdequant = vsetq_lane_s16(dequant_ptr[0], vdequant, 0);
    vquant_shift = vsetq_lane_s16(quant_shift_ptr[0], vquant_shift, 0);

    int16x8_t vtmp = vqaddq_s16(v_abs, vround);
    int16x8_t vtmp2 = vsraq_n_s16(vtmp, vqdmulhq_s16(vtmp, vquant), 1);
    vtmp2 = vshrq_n_s16(vqdmulhq_s16(vtmp2, vquant_shift), 1);

    int16x8_t vdest = vsubq_s16(veorq_s16(vtmp2, v_coeff_sign), v_coeff_sign);
    int16x8_t coeff_nz_mask =
        vbslq_s16(vcond, vdest, load_tran_low_to_s16q(&qcoeff_ptr[0]));
    store_s16q_to_tran_low(&qcoeff_ptr[0], coeff_nz_mask);
    int16x8_t v_deq_abs = vmulq_s16(vtmp2, vdequant);

    vdest = vsubq_s16(veorq_s16(v_deq_abs, v_coeff_sign), v_coeff_sign);
    coeff_nz_mask =
        vbslq_s16(vcond, vdest, load_tran_low_to_s16q(&dqcoeff_ptr[0]));
    store_s16q_to_tran_low(&dqcoeff_ptr[0], coeff_nz_mask);

    vround = vsetq_lane_s16(round_ptr[1], vround, 0);
    vquant = vsetq_lane_s16(quant_ptr[1], vquant, 0);
    vdequant = vsetq_lane_s16(dequant_ptr[1], vdequant, 0);
    vquant_shift = vsetq_lane_s16(quant_shift_ptr[1], vquant_shift, 0);

    uint16x8_t vtmp_mask = vcgtq_s16(vtmp2, zero);
    const uint16x8_t v_nz_mask = vandq_u16(vtmp_mask, vcond);
    int16x8_t v_iscan = vld1q_s16(&iscan[0]);
    vcond = vandq_u16(v_nz_mask, vcgtq_s16(v_iscan, v_eobmax_76543210));
    v_eobmax_76543210 = vbslq_s16(vcond, v_iscan, v_eobmax_76543210);
  }
  vzbins = vsetq_lane_s16(zbins[1], vzbins, 0);

  for (int i = 8; i < n_coeffs; i += 8) {
    v_coeff = load_tran_low_to_s16q(&coeff_ptr[i]);
    v_coeff_sign = vshrq_n_s16(v_coeff, 15);
    v_abs = vabsq_s16(v_coeff);
    vcond = vcgeq_s16(v_abs, vzbins);

    nz_check = vget_lane_u64(vreinterpret_u64_u8(vmovn_u16(vcond)), 0);
    if (nz_check) {
      int16x8_t vtmp = vqaddq_s16(v_abs, vround);
      int16x8_t vtmp2 = vsraq_n_s16(vtmp, vqdmulhq_s16(vtmp, vquant), 1);

      vtmp2 = vshrq_n_s16(vqdmulhq_s16(vtmp2, vquant_shift), 1);
      int16x8_t vdest = vsubq_s16(veorq_s16(vtmp2, v_coeff_sign), v_coeff_sign);
      int16x8_t coeff_nz_mask =
          vbslq_s16(vcond, vdest, load_tran_low_to_s16q(&qcoeff_ptr[i]));
      store_s16q_to_tran_low(&qcoeff_ptr[i], coeff_nz_mask);
      int16x8_t v_deq_abs = vmulq_s16(vtmp2, vdequant);
      vdest = vsubq_s16(veorq_s16(v_deq_abs, v_coeff_sign), v_coeff_sign);
      coeff_nz_mask =
          vbslq_s16(vcond, vdest, load_tran_low_to_s16q(&dqcoeff_ptr[i]));
      store_s16q_to_tran_low(&dqcoeff_ptr[i], coeff_nz_mask);

      uint16x8_t vtmp_mask = vcgtq_s16(vtmp2, zero);
      const uint16x8_t v_nz_mask = vandq_u16(vtmp_mask, vcond);
      int16x8_t v_iscan = vld1q_s16(&iscan[i]);
      vcond = vandq_u16(v_nz_mask, vcgtq_s16(v_iscan, v_eobmax_76543210));
      v_eobmax_76543210 = vbslq_s16(vcond, v_iscan, v_eobmax_76543210);
    }
  }

#ifdef __aarch64__
  *eob_ptr = vmaxvq_s16(v_eobmax_76543210) + 1;
#else
  {
    const int16x4_t v_eobmax_3210 = vmax_s16(vget_low_s16(v_eobmax_76543210),
                                             vget_high_s16(v_eobmax_76543210));
    const int64x1_t v_eobmax_xx32 =
        vshr_n_s64(vreinterpret_s64_s16(v_eobmax_3210), 32);
    const int16x4_t v_eobmax_tmp =
        vmax_s16(v_eobmax_3210, vreinterpret_s16_s64(v_eobmax_xx32));
    const int64x1_t v_eobmax_xxx3 =
        vshr_n_s64(vreinterpret_s64_s16(v_eobmax_tmp), 16);
    const int16x4_t v_eobmax_final =
        vmax_s16(v_eobmax_tmp, vreinterpret_s16_s64(v_eobmax_xxx3));

    *eob_ptr = (uint16_t)vget_lane_s16(v_eobmax_final, 0) + 1;
  }
#endif  // __aarch64__
}

#define QM_MULL_SHIFT(x0, x1)                                              \
  vreinterpretq_s16_u16(vorrq_u16(                                         \
      vreinterpretq_u16_s16(vshlq_n_s16(                                   \
          vqdmulhq_s16(x0, vreinterpretq_s16_u16(x1)), 15 - AOM_QM_BITS)), \
      vshrq_n_u16(vmulq_u16(vreinterpretq_u16_s16(x0), x1), AOM_QM_BITS)))

static void aom_quantize_b_helper_16x16_neon(
    const tran_low_t *coeff_ptr, intptr_t n_coeffs, const int16_t *zbin_ptr,
    const int16_t *round_ptr, const int16_t *quant_ptr,
    const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr,
    tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr,
    const int16_t *scan, const int16_t *iscan, const qm_val_t *qm_ptr,
    const qm_val_t *iqm_ptr) {
  (void)scan;

  uint16x8_t vwt, viwt;
  const int zbins[2] = { zbin_ptr[0], zbin_ptr[1] };

  memset(qcoeff_ptr, 0, n_coeffs * sizeof(*qcoeff_ptr));
  memset(dqcoeff_ptr, 0, n_coeffs * sizeof(*dqcoeff_ptr));

  const int16x8_t zero = vdupq_n_s16(0);
  int16x8_t v_eobmax_76543210 = vreinterpretq_s16_u16(vceqq_s16(zero, zero));

  int16x8_t vzbins = vdupq_n_s16(zbins[1]), vround = vdupq_n_s16(round_ptr[1]);
  int16x8_t vdequant = vdupq_n_s16(dequant_ptr[1]);
  int16x8_t vquant = vdupq_n_s16(quant_ptr[1]);
  int16x8_t vquant_shift = vdupq_n_s16(quant_shift_ptr[1]);

  int16x8_t v_coeff = load_tran_low_to_s16q(&coeff_ptr[0]);
  int16x8_t v_coeff_sign = vshrq_n_s16(v_coeff, 15);
  int16x8_t v_abs = vabsq_s16(v_coeff);
  vzbins = vsetq_lane_s16(zbins[0], vzbins, 0);
  uint16x8_t vcond;
  if (qm_ptr == NULL) {
    vcond = vcgeq_s16(v_abs, vzbins);
  } else {
    vwt = vmovl_u8(vld1_u8(&qm_ptr[0]));
    vcond = vcgeq_s16(QM_MULL_SHIFT(v_abs, vwt), vzbins);
  }
  uint64_t nz_check = vget_lane_u64(vreinterpret_u64_u8(vmovn_u16(vcond)), 0);
  if (nz_check) {
    vround = vsetq_lane_s16(round_ptr[0], vround, 0);
    vquant = vsetq_lane_s16(quant_ptr[0], vquant, 0);
    vdequant = vsetq_lane_s16(dequant_ptr[0], vdequant, 0);
    vquant_shift = vsetq_lane_s16(quant_shift_ptr[0], vquant_shift, 0);

    int16x8_t vtmp = vqaddq_s16(v_abs, vround);

    int16x8_t vtmp2;
    if (qm_ptr == NULL) {
      vtmp2 = vsraq_n_s16(vtmp, vqdmulhq_s16(vtmp, vquant), 1);
    } else {
      vtmp2 = QM_MULL_SHIFT(vtmp, vwt);
      vtmp2 = vaddq_s16(vtmp2, vtmp);
    }

    vtmp2 = vshrq_n_s16(vqdmulhq_s16(vtmp2, vquant_shift), 1);
    int16x8_t vdest = vsubq_s16(veorq_s16(vtmp2, v_coeff_sign), v_coeff_sign);
    int16x8_t coeff_nz_mask =
        vbslq_s16(vcond, vdest, load_tran_low_to_s16q(&qcoeff_ptr[0]));
    store_s16q_to_tran_low(&qcoeff_ptr[0], coeff_nz_mask);

    if (iqm_ptr != NULL) {
      viwt = vmovl_u8(vld1_u8(&iqm_ptr[0]));
      vdequant = QM_MULL_SHIFT(vdequant, viwt);
    }
    int16x8_t v_deq_abs = vmulq_s16(vtmp2, vdequant);
    vdest = vsubq_s16(veorq_s16(v_deq_abs, v_coeff_sign), v_coeff_sign);
    coeff_nz_mask =
        vbslq_s16(vcond, vdest, load_tran_low_to_s16q(&dqcoeff_ptr[0]));
    store_s16q_to_tran_low(&dqcoeff_ptr[0], coeff_nz_mask);

    vround = vsetq_lane_s16(round_ptr[1], vround, 0);
    vquant = vsetq_lane_s16(quant_ptr[1], vquant, 0);
    vdequant = vsetq_lane_s16(dequant_ptr[1], vdequant, 0);
    vquant_shift = vsetq_lane_s16(quant_shift_ptr[1], vquant_shift, 0);

    uint16x8_t vtmp_mask = vcgtq_s16(vtmp2, zero);
    const uint16x8_t v_nz_mask = vandq_u16(vtmp_mask, vcond);
    int16x8_t v_iscan = vld1q_s16(&iscan[0]);
    vcond = vandq_u16(v_nz_mask, vcgtq_s16(v_iscan, v_eobmax_76543210));
    v_eobmax_76543210 = vbslq_s16(vcond, v_iscan, v_eobmax_76543210);
  }
  vzbins = vsetq_lane_s16(zbins[1], vzbins, 0);

  for (int i = 8; i < n_coeffs; i += 8) {
    v_coeff = load_tran_low_to_s16q(&coeff_ptr[i]);
    v_coeff_sign = vshrq_n_s16(v_coeff, 15);
    v_abs = vabsq_s16(v_coeff);

    if (qm_ptr == NULL) {
      vcond = vcgeq_s16(v_abs, vzbins);
    } else {
      vwt = vmovl_u8(vld1_u8(&qm_ptr[i]));
      vcond = vcgeq_s16(QM_MULL_SHIFT(v_abs, vwt), vzbins);
    }
    nz_check = vget_lane_u64(vreinterpret_u64_u8(vmovn_u16(vcond)), 0);
    if (nz_check) {
      int16x8_t vtmp = vqaddq_s16(v_abs, vround);

      int16x8_t vtmp2;
      if (qm_ptr == NULL) {
        vtmp2 = vsraq_n_s16(vtmp, vqdmulhq_s16(vtmp, vquant), 1);
      } else {
        vtmp2 = QM_MULL_SHIFT(vtmp, vwt);
        vtmp2 = vaddq_s16(vtmp2, vtmp);
      }

      vtmp2 = vshrq_n_s16(vqdmulhq_s16(vtmp2, vquant_shift), 1);
      int16x8_t vdest = vsubq_s16(veorq_s16(vtmp2, v_coeff_sign), v_coeff_sign);
      int16x8_t coeff_nz_mask =
          vbslq_s16(vcond, vdest, load_tran_low_to_s16q(&qcoeff_ptr[i]));
      store_s16q_to_tran_low(&qcoeff_ptr[i], coeff_nz_mask);

      if (iqm_ptr != NULL) {
        viwt = vmovl_u8(vld1_u8(&iqm_ptr[i]));
        vdequant = QM_MULL_SHIFT(vdequant, viwt);
      }
      int16x8_t v_deq_abs = vmulq_s16(vtmp2, vdequant);
      vdest = vsubq_s16(veorq_s16(v_deq_abs, v_coeff_sign), v_coeff_sign);
      coeff_nz_mask =
          vbslq_s16(vcond, vdest, load_tran_low_to_s16q(&dqcoeff_ptr[i]));
      store_s16q_to_tran_low(&dqcoeff_ptr[i], coeff_nz_mask);

      uint16x8_t vtmp_mask = vcgtq_s16(vtmp2, zero);
      const uint16x8_t v_nz_mask = vandq_u16(vtmp_mask, vcond);
      int16x8_t v_iscan = vld1q_s16(&iscan[i]);
      vcond = vandq_u16(v_nz_mask, vcgtq_s16(v_iscan, v_eobmax_76543210));
      v_eobmax_76543210 = vbslq_s16(vcond, v_iscan, v_eobmax_76543210);
    }
  }

#ifdef __aarch64__
  *eob_ptr = vmaxvq_s16(v_eobmax_76543210) + 1;
#else
  {
    const int16x4_t v_eobmax_3210 = vmax_s16(vget_low_s16(v_eobmax_76543210),
                                             vget_high_s16(v_eobmax_76543210));
    const int64x1_t v_eobmax_xx32 =
        vshr_n_s64(vreinterpret_s64_s16(v_eobmax_3210), 32);
    const int16x4_t v_eobmax_tmp =
        vmax_s16(v_eobmax_3210, vreinterpret_s16_s64(v_eobmax_xx32));
    const int64x1_t v_eobmax_xxx3 =
        vshr_n_s64(vreinterpret_s64_s16(v_eobmax_tmp), 16);
    const int16x4_t v_eobmax_final =
        vmax_s16(v_eobmax_tmp, vreinterpret_s16_s64(v_eobmax_xxx3));

    *eob_ptr = (uint16_t)vget_lane_s16(v_eobmax_final, 0) + 1;
  }
#endif  // __aarch64__
}

static void aom_quantize_b_helper_32x32_neon(
    const tran_low_t *coeff_ptr, intptr_t n_coeffs, const int16_t *zbin_ptr,
    const int16_t *round_ptr, const int16_t *quant_ptr,
    const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr,
    tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr,
    const int16_t *scan, const int16_t *iscan, const qm_val_t *qm_ptr,
    const qm_val_t *iqm_ptr) {
  (void)scan;

  uint16x8_t vwt, viwt;
  const int log_scale = 1;
  const int zbins[2] = { ROUND_POWER_OF_TWO(zbin_ptr[0], log_scale),
                         ROUND_POWER_OF_TWO(zbin_ptr[1], log_scale) };

  memset(qcoeff_ptr, 0, n_coeffs * sizeof(*qcoeff_ptr));
  memset(dqcoeff_ptr, 0, n_coeffs * sizeof(*dqcoeff_ptr));

  const int16x8_t zero = vdupq_n_s16(0);
  int16x8_t v_eobmax_76543210 = vreinterpretq_s16_u16(vceqq_s16(zero, zero));
  const int16x8_t v_log_scale = v_eobmax_76543210;

  int16x8_t vzbins = vdupq_n_s16(zbins[1]),
            vround = vdupq_n_s16(ROUND_POWER_OF_TWO(round_ptr[1], log_scale));
  int16x8_t vdequant = vdupq_n_s16(dequant_ptr[1]);
  int16x8_t vquant = vdupq_n_s16(quant_ptr[1]);
  int16x8_t vquant_shift = vdupq_n_s16(quant_shift_ptr[1]);

  int16x8_t v_coeff = load_tran_low_to_s16q(&coeff_ptr[0]);
  int16x8_t v_coeff_sign = vshrq_n_s16(v_coeff, 15);
  int16x8_t v_abs = vabsq_s16(v_coeff);
  vzbins = vsetq_lane_s16(zbins[0], vzbins, 0);
  uint16x8_t vcond;
  if (qm_ptr == NULL) {
    vcond = vcgeq_s16(v_abs, vzbins);
  } else {
    vwt = vmovl_u8(vld1_u8(&qm_ptr[0]));
    vcond = vcgeq_s16(QM_MULL_SHIFT(v_abs, vwt), vzbins);
  }
  uint64_t nz_check = vget_lane_u64(vreinterpret_u64_u8(vmovn_u16(vcond)), 0);
  if (nz_check) {
    vround =
        vsetq_lane_s16(ROUND_POWER_OF_TWO(round_ptr[0], log_scale), vround, 0);
    vquant = vsetq_lane_s16(quant_ptr[0], vquant, 0);
    vdequant = vsetq_lane_s16(dequant_ptr[0], vdequant, 0);
    vquant_shift = vsetq_lane_s16(quant_shift_ptr[0], vquant_shift, 0);

    int16x8_t vtmp = vqaddq_s16(v_abs, vround);

    int16x8_t vtmp2;
    if (qm_ptr == NULL) {
      vtmp2 = vsraq_n_s16(vtmp, vqdmulhq_s16(vtmp, vquant), 1);
    } else {
      vtmp2 = QM_MULL_SHIFT(vtmp, vwt);
      vtmp2 = vaddq_s16(vtmp2, vtmp);
    }

    vtmp2 = vqdmulhq_s16(vtmp2, vquant_shift);
    int16x8_t vdest = vsubq_s16(veorq_s16(vtmp2, v_coeff_sign), v_coeff_sign);
    int16x8_t coeff_nz_mask =
        vbslq_s16(vcond, vdest, load_tran_low_to_s16q(&qcoeff_ptr[0]));
    store_s16q_to_tran_low(&qcoeff_ptr[0], coeff_nz_mask);

    if (iqm_ptr != NULL) {
      viwt = vmovl_u8(vld1_u8(&iqm_ptr[0]));
      vdequant = QM_MULL_SHIFT(vdequant, viwt);
    }
    int16x8_t v_deq_abs = vreinterpretq_s16_u16(vshlq_u16(
        vreinterpretq_u16_s16(vmulq_s16(vtmp2, vdequant)), v_log_scale));
    vdest = vsubq_s16(veorq_s16(v_deq_abs, v_coeff_sign), v_coeff_sign);
    coeff_nz_mask =
        vbslq_s16(vcond, vdest, load_tran_low_to_s16q(&dqcoeff_ptr[0]));
    store_s16q_to_tran_low(&dqcoeff_ptr[0], coeff_nz_mask);

    vzbins = vsetq_lane_s16(zbins[1], vzbins, 0);
    vround =
        vsetq_lane_s16(ROUND_POWER_OF_TWO(round_ptr[1], log_scale), vround, 0);
    vquant = vsetq_lane_s16(quant_ptr[1], vquant, 0);
    vdequant = vsetq_lane_s16(dequant_ptr[1], vdequant, 0);
    vquant_shift = vsetq_lane_s16(quant_shift_ptr[1], vquant_shift, 0);

    uint16x8_t vtmp_mask = vcgtq_s16(vtmp2, zero);
    const uint16x8_t v_nz_mask = vandq_u16(vtmp_mask, vcond);
    int16x8_t v_iscan = vld1q_s16(&iscan[0]);
    vcond = vandq_u16(v_nz_mask, vcgtq_s16(v_iscan, v_eobmax_76543210));
    v_eobmax_76543210 = vbslq_s16(vcond, v_iscan, v_eobmax_76543210);
  }
  vzbins = vsetq_lane_s16(zbins[1], vzbins, 0);

  for (int i = 8; i < n_coeffs; i += 8) {
    v_coeff = load_tran_low_to_s16q(&coeff_ptr[i]);
    v_coeff_sign = vshrq_n_s16(v_coeff, 15);
    v_abs = vabsq_s16(v_coeff);

    if (qm_ptr == NULL) {
      vcond = vcgeq_s16(v_abs, vzbins);
    } else {
      vwt = vmovl_u8(vld1_u8(&qm_ptr[i]));
      vcond = vcgeq_s16(QM_MULL_SHIFT(v_abs, vwt), vzbins);
    }
    nz_check = vget_lane_u64(vreinterpret_u64_u8(vmovn_u16(vcond)), 0);
    if (nz_check) {
      int16x8_t vtmp = vqaddq_s16(v_abs, vround);

      int16x8_t vtmp2;
      if (qm_ptr == NULL) {
        vtmp2 = vsraq_n_s16(vtmp, vqdmulhq_s16(vtmp, vquant), 1);
      } else {
        vtmp2 = QM_MULL_SHIFT(vtmp, vwt);
        vtmp2 = vaddq_s16(vtmp2, vtmp);
      }
      vtmp2 = vqdmulhq_s16(vtmp2, vquant_shift);

      int16x8_t vdest = vsubq_s16(veorq_s16(vtmp2, v_coeff_sign), v_coeff_sign);
      int16x8_t coeff_nz_mask =
          vbslq_s16(vcond, vdest, load_tran_low_to_s16q(&qcoeff_ptr[i]));
      store_s16q_to_tran_low(&qcoeff_ptr[i], coeff_nz_mask);

      if (iqm_ptr != NULL) {
        viwt = vmovl_u8(vld1_u8(&iqm_ptr[i]));
        vdequant = QM_MULL_SHIFT(vdequant, viwt);
      }
      int16x8_t v_deq_abs = vreinterpretq_s16_u16(vshlq_u16(
          vreinterpretq_u16_s16(vmulq_s16(vtmp2, vdequant)), v_log_scale));
      vdest = vsubq_s16(veorq_s16(v_deq_abs, v_coeff_sign), v_coeff_sign);
      coeff_nz_mask =
          vbslq_s16(vcond, vdest, load_tran_low_to_s16q(&dqcoeff_ptr[i]));
      store_s16q_to_tran_low(&dqcoeff_ptr[i], coeff_nz_mask);

      uint16x8_t vtmp_mask = vcgtq_s16(vtmp2, zero);
      const uint16x8_t v_nz_mask = vandq_u16(vtmp_mask, vcond);
      int16x8_t v_iscan = vld1q_s16(&iscan[i]);
      vcond = vandq_u16(v_nz_mask, vcgtq_s16(v_iscan, v_eobmax_76543210));
      v_eobmax_76543210 = vbslq_s16(vcond, v_iscan, v_eobmax_76543210);
    }
  }

#ifdef __aarch64__
  *eob_ptr = vmaxvq_s16(v_eobmax_76543210) + 1;
#else
  {
    const int16x4_t v_eobmax_3210 = vmax_s16(vget_low_s16(v_eobmax_76543210),
                                             vget_high_s16(v_eobmax_76543210));
    const int64x1_t v_eobmax_xx32 =
        vshr_n_s64(vreinterpret_s64_s16(v_eobmax_3210), 32);
    const int16x4_t v_eobmax_tmp =
        vmax_s16(v_eobmax_3210, vreinterpret_s16_s64(v_eobmax_xx32));
    const int64x1_t v_eobmax_xxx3 =
        vshr_n_s64(vreinterpret_s64_s16(v_eobmax_tmp), 16);
    const int16x4_t v_eobmax_final =
        vmax_s16(v_eobmax_tmp, vreinterpret_s16_s64(v_eobmax_xxx3));

    *eob_ptr = (uint16_t)vget_lane_s16(v_eobmax_final, 0) + 1;
  }
#endif  // __aarch64__
}

static void aom_quantize_b_helper_64x64_neon(
    const tran_low_t *coeff_ptr, intptr_t n_coeffs, const int16_t *zbin_ptr,
    const int16_t *round_ptr, const int16_t *quant_ptr,
    const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr,
    tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr,
    const int16_t *scan, const int16_t *iscan, const qm_val_t *qm_ptr,
    const qm_val_t *iqm_ptr) {
  (void)scan;

  uint16x8_t vwt, viwt;
  const int log_scale = 2;
  const int16x8_t v_log_scale =
      vreinterpretq_s16_s64(vdupq_n_s64(0xFFFEFFFEFFFEFFFE));

  const int zbins[2] = { ROUND_POWER_OF_TWO(zbin_ptr[0], log_scale),
                         ROUND_POWER_OF_TWO(zbin_ptr[1], log_scale) };

  memset(qcoeff_ptr, 0, n_coeffs * sizeof(*qcoeff_ptr));
  memset(dqcoeff_ptr, 0, n_coeffs * sizeof(*dqcoeff_ptr));

  const int16x8_t zero = vdupq_n_s16(0);
  int16x8_t v_eobmax_76543210 = vreinterpretq_s16_u16(vceqq_s16(zero, zero));
  int16x8_t v_ones = vnegq_s16(v_eobmax_76543210);

  int16x8_t vzbins = vdupq_n_s16(zbins[1]),
            vround = vdupq_n_s16(ROUND_POWER_OF_TWO(round_ptr[1], log_scale));
  int16x8_t vdequant = vdupq_n_s16(dequant_ptr[1]);
  int16x8_t vquant = vdupq_n_s16(quant_ptr[1]);
  int16x8_t vquant_shift = vdupq_n_s16(quant_shift_ptr[1]);

  int16x8_t v_coeff = load_tran_low_to_s16q(&coeff_ptr[0]);
  int16x8_t v_coeff_sign = vshrq_n_s16(v_coeff, 15);
  int16x8_t v_abs = vabsq_s16(v_coeff);
  vzbins = vsetq_lane_s16(zbins[0], vzbins, 0);
  uint16x8_t vcond;
  if (qm_ptr == NULL) {
    vcond = vcgeq_s16(v_abs, vzbins);
  } else {
    vwt = vmovl_u8(vld1_u8(&qm_ptr[0]));
    vcond = vcgeq_s16(QM_MULL_SHIFT(v_abs, vwt), vzbins);
  }
  uint64_t nz_check = vget_lane_u64(vreinterpret_u64_u8(vmovn_u16(vcond)), 0);
  if (nz_check) {
    vround =
        vsetq_lane_s16(ROUND_POWER_OF_TWO(round_ptr[0], log_scale), vround, 0);
    vquant = vsetq_lane_s16(quant_ptr[0], vquant, 0);
    vdequant = vsetq_lane_s16(dequant_ptr[0], vdequant, 0);
    vquant_shift = vsetq_lane_s16(quant_shift_ptr[0], vquant_shift, 0);
    int16x8_t vtmp = vqaddq_s16(v_abs, vround);

    int16x8_t vtmp2;
    if (qm_ptr == NULL) {
      vtmp2 = vsraq_n_s16(vtmp, vqdmulhq_s16(vtmp, vquant), 1);
    } else {
      vtmp2 = QM_MULL_SHIFT(vtmp, vwt);
      vtmp2 = vaddq_s16(vtmp2, vtmp);
    }

    int16x8_t ones =
        vandq_s16(vshrq_n_s16(vmulq_s16(vtmp2, vquant_shift), 14), v_ones);
    vtmp2 =
        vaddq_s16(vshlq_s16(vqdmulhq_s16(vtmp2, vquant_shift), v_ones), ones);
    int16x8_t vdest = vsubq_s16(veorq_s16(vtmp2, v_coeff_sign), v_coeff_sign);
    int16x8_t coeff_nz_mask =
        vbslq_s16(vcond, vdest, load_tran_low_to_s16q(&qcoeff_ptr[0]));
    store_s16q_to_tran_low(&qcoeff_ptr[0], coeff_nz_mask);

    if (iqm_ptr != NULL) {
      viwt = vmovl_u8(vld1_u8(&iqm_ptr[0]));
      vdequant = QM_MULL_SHIFT(vdequant, viwt);
    }
    int16x8_t v_deq_abs = vreinterpretq_s16_u16(vshlq_u16(
        vreinterpretq_u16_s16(vmulq_s16(vtmp2, vdequant)), v_log_scale));
    v_deq_abs =
        vorrq_s16(vshlq_n_s16(vqdmulhq_s16(vtmp2, vdequant), 13), v_deq_abs);
    vdest = vsubq_s16(veorq_s16(v_deq_abs, v_coeff_sign), v_coeff_sign);
    coeff_nz_mask =
        vbslq_s16(vcond, vdest, load_tran_low_to_s16q(&dqcoeff_ptr[0]));
    store_s16q_to_tran_low(&dqcoeff_ptr[0], coeff_nz_mask);

    vround =
        vsetq_lane_s16(ROUND_POWER_OF_TWO(round_ptr[1], log_scale), vround, 0);
    vquant = vsetq_lane_s16(quant_ptr[1], vquant, 0);
    vdequant = vsetq_lane_s16(dequant_ptr[1], vdequant, 0);
    vquant_shift = vsetq_lane_s16(quant_shift_ptr[1], vquant_shift, 0);

    uint16x8_t vtmp_mask = vcgtq_s16(vtmp2, zero);
    const uint16x8_t v_nz_mask = vandq_u16(vtmp_mask, vcond);
    int16x8_t v_iscan = vld1q_s16(&iscan[0]);
    vcond = vandq_u16(v_nz_mask, vcgtq_s16(v_iscan, v_eobmax_76543210));
    v_eobmax_76543210 = vbslq_s16(vcond, v_iscan, v_eobmax_76543210);
  }
  vzbins = vsetq_lane_s16(zbins[1], vzbins, 0);

  for (int i = 8; i < n_coeffs; i += 8) {
    v_coeff = load_tran_low_to_s16q(&coeff_ptr[i]);
    v_coeff_sign = vshrq_n_s16(v_coeff, 15);
    v_abs = vabsq_s16(v_coeff);

    if (qm_ptr == NULL) {
      vcond = vcgeq_s16(v_abs, vzbins);
    } else {
      vwt = vmovl_u8(vld1_u8(&qm_ptr[i]));
      vcond = vcgeq_s16(QM_MULL_SHIFT(v_abs, vwt), vzbins);
    }
    nz_check = vget_lane_u64(vreinterpret_u64_u8(vmovn_u16(vcond)), 0);
    if (nz_check) {
      int16x8_t vtmp = vqaddq_s16(v_abs, vround);

      int16x8_t vtmp2;
      if (qm_ptr == NULL) {
        vtmp2 = vsraq_n_s16(vtmp, vqdmulhq_s16(vtmp, vquant), 1);
      } else {
        vtmp2 = QM_MULL_SHIFT(vtmp, vwt);
        vtmp2 = vaddq_s16(vtmp2, vtmp);
      }

      int16x8_t ones =
          vandq_s16(vshrq_n_s16(vmulq_s16(vtmp2, vquant_shift), 14), v_ones);
      vtmp2 =
          vaddq_s16(vshlq_s16(vqdmulhq_s16(vtmp2, vquant_shift), v_ones), ones);
      int16x8_t vdest = vsubq_s16(veorq_s16(vtmp2, v_coeff_sign), v_coeff_sign);
      int16x8_t coeff_nz_mask =
          vbslq_s16(vcond, vdest, load_tran_low_to_s16q(&qcoeff_ptr[i]));
      store_s16q_to_tran_low(&qcoeff_ptr[i], coeff_nz_mask);

      if (iqm_ptr != NULL) {
        viwt = vmovl_u8(vld1_u8(&iqm_ptr[i]));
        vdequant = QM_MULL_SHIFT(vdequant, viwt);
      }
      int16x8_t v_deq_abs = vreinterpretq_s16_u16(vshlq_u16(
          vreinterpretq_u16_s16(vmulq_s16(vtmp2, vdequant)), v_log_scale));
      v_deq_abs =
          vorrq_s16(vshlq_n_s16(vqdmulhq_s16(vtmp2, vdequant), 13), v_deq_abs);
      vdest = vsubq_s16(veorq_s16(v_deq_abs, v_coeff_sign), v_coeff_sign);
      coeff_nz_mask =
          vbslq_s16(vcond, vdest, load_tran_low_to_s16q(&dqcoeff_ptr[i]));
      store_s16q_to_tran_low(&dqcoeff_ptr[i], coeff_nz_mask);

      uint16x8_t vtmp_mask = vcgtq_s16(vtmp2, zero);
      const uint16x8_t v_nz_mask = vandq_u16(vtmp_mask, vcond);
      int16x8_t v_iscan = vld1q_s16(&iscan[i]);
      vcond = vandq_u16(v_nz_mask, vcgtq_s16(v_iscan, v_eobmax_76543210));
      v_eobmax_76543210 = vbslq_s16(vcond, v_iscan, v_eobmax_76543210);
    }
  }

#ifdef __aarch64__
  *eob_ptr = vmaxvq_s16(v_eobmax_76543210) + 1;
#else
  {
    const int16x4_t v_eobmax_3210 = vmax_s16(vget_low_s16(v_eobmax_76543210),
                                             vget_high_s16(v_eobmax_76543210));
    const int64x1_t v_eobmax_xx32 =
        vshr_n_s64(vreinterpret_s64_s16(v_eobmax_3210), 32);
    const int16x4_t v_eobmax_tmp =
        vmax_s16(v_eobmax_3210, vreinterpret_s16_s64(v_eobmax_xx32));
    const int64x1_t v_eobmax_xxx3 =
        vshr_n_s64(vreinterpret_s64_s16(v_eobmax_tmp), 16);
    const int16x4_t v_eobmax_final =
        vmax_s16(v_eobmax_tmp, vreinterpret_s16_s64(v_eobmax_xxx3));

    *eob_ptr = (uint16_t)vget_lane_s16(v_eobmax_final, 0) + 1;
  }
#endif  // __aarch64__
}

void aom_quantize_b_helper_neon(
    const tran_low_t *coeff_ptr, intptr_t n_coeffs, const int16_t *zbin_ptr,
    const int16_t *round_ptr, const int16_t *quant_ptr,
    const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr,
    tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr,
    const int16_t *scan, const int16_t *iscan, const qm_val_t *qm_ptr,
    const qm_val_t *iqm_ptr, const int log_scale) {
  switch (log_scale) {  // log_scale for AV1 encoder can be only 0, 1, 2
    case 0:
      aom_quantize_b_helper_16x16_neon(coeff_ptr, n_coeffs, zbin_ptr, round_ptr,
                                       quant_ptr, quant_shift_ptr, qcoeff_ptr,
                                       dqcoeff_ptr, dequant_ptr, eob_ptr, scan,
                                       iscan, qm_ptr, iqm_ptr);
      break;
    case 1:
      aom_quantize_b_helper_32x32_neon(coeff_ptr, n_coeffs, zbin_ptr, round_ptr,
                                       quant_ptr, quant_shift_ptr, qcoeff_ptr,
                                       dqcoeff_ptr, dequant_ptr, eob_ptr, scan,
                                       iscan, qm_ptr, iqm_ptr);
      break;
    case 2:
      aom_quantize_b_helper_64x64_neon(coeff_ptr, n_coeffs, zbin_ptr, round_ptr,
                                       quant_ptr, quant_shift_ptr, qcoeff_ptr,
                                       dqcoeff_ptr, dequant_ptr, eob_ptr, scan,
                                       iscan, qm_ptr, iqm_ptr);
      break;
  }
}

void aom_quantize_b_32x32_neon(const tran_low_t *coeff_ptr, intptr_t n_coeffs,
                               const int16_t *zbin_ptr,
                               const int16_t *round_ptr,
                               const int16_t *quant_ptr,
                               const int16_t *quant_shift_ptr,
                               tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr,
                               const int16_t *dequant_ptr, uint16_t *eob_ptr,
                               const int16_t *scan, const int16_t *iscan) {
  aom_quantize_b_helper_neon(coeff_ptr, n_coeffs, zbin_ptr, round_ptr,
                             quant_ptr, quant_shift_ptr, qcoeff_ptr,
                             dqcoeff_ptr, dequant_ptr, eob_ptr, scan, iscan,
                             NULL, NULL, 1);
}

void aom_quantize_b_64x64_neon(const tran_low_t *coeff_ptr, intptr_t n_coeffs,
                               const int16_t *zbin_ptr,
                               const int16_t *round_ptr,
                               const int16_t *quant_ptr,
                               const int16_t *quant_shift_ptr,
                               tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr,
                               const int16_t *dequant_ptr, uint16_t *eob_ptr,
                               const int16_t *scan, const int16_t *iscan) {
  aom_quantize_b_helper_neon(coeff_ptr, n_coeffs, zbin_ptr, round_ptr,
                             quant_ptr, quant_shift_ptr, qcoeff_ptr,
                             dqcoeff_ptr, dequant_ptr, eob_ptr, scan, iscan,
                             NULL, NULL, 2);
}
