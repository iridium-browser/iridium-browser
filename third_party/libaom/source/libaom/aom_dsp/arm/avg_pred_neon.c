/*
 * Copyright (c) 2023, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <arm_neon.h>
#include <assert.h>

#include "config/aom_dsp_rtcd.h"
#include "aom_dsp/arm/mem_neon.h"

void aom_comp_avg_pred_neon(uint8_t *comp_pred, const uint8_t *pred, int width,
                            int height, const uint8_t *ref, int ref_stride) {
  if (width > 8) {
    do {
      const uint8_t *pred_ptr = pred;
      const uint8_t *ref_ptr = ref;
      uint8_t *comp_pred_ptr = comp_pred;
      int w = width;

      do {
        const uint8x16_t p = vld1q_u8(pred_ptr);
        const uint8x16_t r = vld1q_u8(ref_ptr);
        const uint8x16_t avg = vrhaddq_u8(p, r);

        vst1q_u8(comp_pred_ptr, avg);

        ref_ptr += 16;
        pred_ptr += 16;
        comp_pred_ptr += 16;
        w -= 16;
      } while (w != 0);

      ref += ref_stride;
      pred += width;
      comp_pred += width;
    } while (--height != 0);
  } else if (width == 8) {
    int h = height / 2;

    do {
      const uint8x16_t p = vld1q_u8(pred);
      const uint8x16_t r = load_u8_8x2(ref, ref_stride);
      const uint8x16_t avg = vrhaddq_u8(p, r);

      vst1q_u8(comp_pred, avg);

      ref += 2 * ref_stride;
      pred += 16;
      comp_pred += 16;
    } while (--h != 0);
  } else {
    int h = height / 4;
    assert(width == 4);

    do {
      const uint8x16_t p = vld1q_u8(pred);
      const uint8x16_t r = load_unaligned_u8q(ref, ref_stride);
      const uint8x16_t avg = vrhaddq_u8(p, r);

      vst1q_u8(comp_pred, avg);

      ref += 4 * ref_stride;
      pred += 16;
      comp_pred += 16;
    } while (--h != 0);
  }
}
