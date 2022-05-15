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

#include "aom_dsp/aom_simd.h"
#define SIMD_FUNC(name) name##_neon
#include "av1/common/cdef_block_simd.h"

void cdef_copy_rect8_8bit_to_16bit_neon(uint16_t *dst, int dstride,
                                        const uint8_t *src, int sstride, int v,
                                        int h) {
  int j;
  for (int i = 0; i < v; i++) {
    for (j = 0; j < (h & ~0x7); j += 8) {
      v64 row = v64_load_unaligned(&src[i * sstride + j]);
      v128_store_unaligned(&dst[i * dstride + j], v128_unpack_u8_s16(row));
    }
    for (; j < h; j++) {
      dst[i * dstride + j] = src[i * sstride + j];
    }
  }
}
