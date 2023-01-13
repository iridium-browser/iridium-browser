/*
 * Copyright (c) 2021, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <emmintrin.h>  // SSE2

#include "config/aom_dsp_rtcd.h"
#include "aom_dsp/x86/synonyms.h"

static int64_t k_means_horizontal_sum_sse2(__m128i a) {
  const __m128i sum1 = _mm_unpackhi_epi64(a, a);
  const __m128i sum2 = _mm_add_epi64(a, sum1);
  int64_t res;
  _mm_storel_epi64((__m128i *)&res, sum2);
  return res;
}

void av1_calc_indices_dim1_sse2(const int16_t *data, const int16_t *centroids,
                                uint8_t *indices, int64_t *total_dist, int n,
                                int k) {
  const __m128i v_zero = _mm_setzero_si128();
  __m128i dist[PALETTE_MAX_SIZE];
  __m128i sum = _mm_setzero_si128();

  for (int i = 0; i < n; i += 8) {
    __m128i in = _mm_loadu_si128((__m128i *)data);
    for (int j = 0; j < k; j++) {
      __m128i cent = _mm_set1_epi16(centroids[j]);
      __m128i d1 = _mm_sub_epi16(in, cent);
      __m128i d2 = _mm_sub_epi16(cent, in);
      dist[j] = _mm_max_epi16(d1, d2);
    }

    __m128i ind = _mm_setzero_si128();
    for (int j = 1; j < k; j++) {
      __m128i cmp = _mm_cmpgt_epi16(dist[0], dist[j]);
      dist[0] = _mm_min_epi16(dist[0], dist[j]);
      __m128i ind1 = _mm_set1_epi16(j);
      ind = _mm_or_si128(_mm_andnot_si128(cmp, ind), _mm_and_si128(cmp, ind1));
    }
    if (total_dist) {
      // Square, convert to 32 bit and add together.
      dist[0] = _mm_madd_epi16(dist[0], dist[0]);
      // Convert to 64 bit and add to sum.
      const __m128i dist1 = _mm_unpacklo_epi32(dist[0], v_zero);
      const __m128i dist2 = _mm_unpackhi_epi32(dist[0], v_zero);
      sum = _mm_add_epi64(sum, dist1);
      sum = _mm_add_epi64(sum, dist2);
    }
    __m128i p2 = _mm_packus_epi16(ind, v_zero);
    _mm_storel_epi64((__m128i *)indices, p2);
    indices += 8;
    data += 8;
  }
  if (total_dist) {
    *total_dist = k_means_horizontal_sum_sse2(sum);
  }
}

void av1_calc_indices_dim2_sse2(const int16_t *data, const int16_t *centroids,
                                uint8_t *indices, int64_t *total_dist, int n,
                                int k) {
  const __m128i v_zero = _mm_setzero_si128();
  int l = 1;
  __m128i dist[PALETTE_MAX_SIZE];
  __m128i ind[2];
  __m128i sum = _mm_setzero_si128();

  for (int i = 0; i < n; i += 4) {
    l = (l == 0) ? 1 : 0;
    __m128i ind1 = _mm_loadu_si128((__m128i *)data);
    for (int j = 0; j < k; j++) {
      const int16_t cx = centroids[2 * j], cy = centroids[2 * j + 1];
      const __m128i cent = _mm_set_epi16(cy, cx, cy, cx, cy, cx, cy, cx);
      const __m128i d1 = _mm_sub_epi16(ind1, cent);
      dist[j] = _mm_madd_epi16(d1, d1);
    }

    ind[l] = _mm_setzero_si128();
    for (int j = 1; j < k; j++) {
      __m128i cmp = _mm_cmpgt_epi32(dist[0], dist[j]);
      __m128i dist1 = _mm_andnot_si128(cmp, dist[0]);
      __m128i dist2 = _mm_and_si128(cmp, dist[j]);
      dist[0] = _mm_or_si128(dist1, dist2);
      ind1 = _mm_set1_epi32(j);
      ind[l] =
          _mm_or_si128(_mm_andnot_si128(cmp, ind[l]), _mm_and_si128(cmp, ind1));
    }
    ind[l] = _mm_packus_epi16(ind[l], v_zero);
    if (total_dist) {
      // Convert to 64 bit and add to sum.
      const __m128i dist1 = _mm_unpacklo_epi32(dist[0], v_zero);
      const __m128i dist2 = _mm_unpackhi_epi32(dist[0], v_zero);
      sum = _mm_add_epi64(sum, dist1);
      sum = _mm_add_epi64(sum, dist2);
    }
    if (l == 1) {
      __m128i p2 = _mm_packus_epi16(_mm_unpacklo_epi64(ind[0], ind[1]), v_zero);
      _mm_storel_epi64((__m128i *)indices, p2);
      indices += 8;
    }
    data += 8;
  }
  if (total_dist) {
    *total_dist = k_means_horizontal_sum_sse2(sum);
  }
}
