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

#include "src/dsp/loop_filter.h"
#include "src/utils/cpu.h"

#if LIBGAV1_TARGETING_SSE4_1

#include <smmintrin.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/dsp/x86/common_sse4.h"

namespace libgav1 {
namespace dsp {
namespace {

inline __m128i FilterAdd2Sub2(const __m128i& total, const __m128i& a1,
                              const __m128i& a2, const __m128i& s1,
                              const __m128i& s2) {
  __m128i x = _mm_add_epi16(a1, total);
  x = _mm_add_epi16(_mm_sub_epi16(x, _mm_add_epi16(s1, s2)), a2);
  return x;
}

}  // namespace

namespace low_bitdepth {
namespace {

inline __m128i AbsDiff(const __m128i& a, const __m128i& b) {
  return _mm_or_si128(_mm_subs_epu8(a, b), _mm_subs_epu8(b, a));
}

inline __m128i CheckOuterThreshF4(const __m128i& q1q0, const __m128i& p1p0,
                                  const __m128i& outer_thresh) {
  const __m128i fe = _mm_set1_epi8(static_cast<int8_t>(0xfe));
  //  abs(p0 - q0) * 2 + abs(p1 - q1) / 2 <= outer_thresh;
  const __m128i abs_pmq = AbsDiff(p1p0, q1q0);
  const __m128i a = _mm_adds_epu8(abs_pmq, abs_pmq);
  const __m128i b = _mm_srli_epi16(_mm_and_si128(abs_pmq, fe), 1);
  const __m128i c = _mm_adds_epu8(a, _mm_srli_si128(b, 4));
  return _mm_subs_epu8(c, outer_thresh);
}

inline __m128i Hev(const __m128i& qp1, const __m128i& qp0,
                   const __m128i& hev_thresh) {
  const __m128i abs_qp1mqp0 = AbsDiff(qp1, qp0);
  const __m128i max_pq =
      _mm_max_epu8(abs_qp1mqp0, _mm_srli_si128(abs_qp1mqp0, 4));
  const __m128i hev_mask0 = _mm_cvtepu8_epi16(max_pq);
  const __m128i hev_mask1 = _mm_cmpgt_epi16(hev_mask0, hev_thresh);
  const __m128i hev_mask = _mm_packs_epi16(hev_mask1, hev_mask1);
  return hev_mask;
}

inline __m128i AddShift3(const __m128i& a, const __m128i& b) {
  const __m128i c = _mm_adds_epi8(a, b);
  const __m128i d = _mm_unpacklo_epi8(c, c);
  const __m128i e = _mm_srai_epi16(d, 11); /* >> 3 */
  return _mm_packs_epi16(e, e);
}

inline __m128i AddShift1(const __m128i& a, const __m128i& b) {
  const __m128i c = _mm_adds_epi8(a, b);
  const __m128i d = _mm_unpacklo_epi8(c, c);
  const __m128i e = _mm_srai_epi16(d, 9); /* >> 1 */
  return _mm_packs_epi16(e, e);
}

//------------------------------------------------------------------------------
// 4-tap filters

inline __m128i NeedsFilter4(const __m128i& q1q0, const __m128i& p1p0,
                            const __m128i& qp1, const __m128i& qp0,
                            const __m128i& outer_thresh,
                            const __m128i& inner_thresh) {
  const __m128i outer_mask = CheckOuterThreshF4(q1q0, p1p0, outer_thresh);
  const __m128i abs_qp1mqp0 = AbsDiff(qp1, qp0);
  const __m128i inner_mask = _mm_subs_epu8(
      _mm_max_epu8(abs_qp1mqp0, _mm_srli_si128(abs_qp1mqp0, 4)), inner_thresh);
  // ~mask
  const __m128i zero = _mm_setzero_si128();
  const __m128i a = _mm_or_si128(outer_mask, inner_mask);
  const __m128i b = _mm_cmpeq_epi8(a, zero);
  return b;
}

inline void Filter4(const __m128i& qp1, const __m128i& qp0, __m128i* oqp1,
                    __m128i* oqp0, const __m128i& mask, const __m128i& hev) {
  const __m128i t80 = _mm_set1_epi8(static_cast<int8_t>(0x80));
  const __m128i t1 = _mm_set1_epi8(0x1);
  const __m128i qp1qp0 = _mm_unpacklo_epi64(qp0, qp1);
  const __m128i qps1qps0 = _mm_xor_si128(qp1qp0, t80);
  const __m128i ps1qs0 = _mm_shuffle_epi32(qps1qps0, 0x09);
  const __m128i qs1ps0 = _mm_shuffle_epi32(qps1qps0, 0x0c);
  const __m128i _hev = _mm_unpacklo_epi32(hev, hev);
  const __m128i x = _mm_subs_epi8(ps1qs0, qs1ps0);
  __m128i a = _mm_and_si128(_mm_srli_si128(x, 4), _hev);

  a = _mm_adds_epi8(a, x);
  a = _mm_adds_epi8(a, x);
  a = _mm_adds_epi8(a, x);
  a = _mm_and_si128(a, mask);
  a = _mm_unpacklo_epi32(a, a);

  const __m128i t4t3 = _mm_set_epi32(0x0, 0x0, 0x04040404, 0x03030303);
  const __m128i a1a2 = AddShift3(a, t4t3);
  const __m128i a1a1 = _mm_shuffle_epi32(a1a2, 0x55);
  const __m128i a3a3 = _mm_andnot_si128(_hev, AddShift1(a1a1, t1));
  // -1 -1 -1 -1 1 1 1 1 -1 -1 -1 -1 1 1 1 1
  const __m128i adjust_sign_for_add =
      _mm_unpacklo_epi32(t1, _mm_cmpeq_epi8(t1, t1));

  const __m128i a3a3a1a2 = _mm_unpacklo_epi64(a1a2, a3a3);
  const __m128i ma3a3ma1a2 = _mm_sign_epi8(a3a3a1a2, adjust_sign_for_add);

  const __m128i b = _mm_adds_epi8(qps1qps0, ma3a3ma1a2);
  const __m128i c = _mm_xor_si128(b, t80);

  *oqp0 = c;
  *oqp1 = _mm_srli_si128(c, 8);
}

void Horizontal4(void* dest, ptrdiff_t stride, int outer_thresh,
                 int inner_thresh, int hev_thresh) {
  auto* const dst = static_cast<uint8_t*>(dest);
  const __m128i zero = _mm_setzero_si128();
  const __m128i v_outer_thresh =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(outer_thresh), zero);
  const __m128i v_inner_thresh =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(inner_thresh), zero);
  const __m128i v_hev_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(hev_thresh), 0);

  const __m128i p1 = Load4(dst - 2 * stride);
  const __m128i p0 = Load4(dst - 1 * stride);
  const __m128i q0 = Load4(dst + 0 * stride);
  const __m128i q1 = Load4(dst + 1 * stride);
  const __m128i qp1 = _mm_unpacklo_epi32(p1, q1);
  const __m128i qp0 = _mm_unpacklo_epi32(p0, q0);
  const __m128i q1q0 = _mm_unpacklo_epi32(q0, q1);
  const __m128i p1p0 = _mm_unpacklo_epi32(p0, p1);

  const __m128i v_hev_mask = Hev(qp1, qp0, v_hev_thresh);
  const __m128i v_needs_mask =
      NeedsFilter4(q1q0, p1p0, qp1, qp0, v_outer_thresh, v_inner_thresh);

  __m128i oqp1;
  __m128i oqp0;
  Filter4(qp1, qp0, &oqp1, &oqp0, v_needs_mask, v_hev_mask);

  Store4(dst - 2 * stride, oqp1);
  Store4(dst - 1 * stride, oqp0);
  Store4(dst + 0 * stride, _mm_srli_si128(oqp0, 4));
  Store4(dst + 1 * stride, _mm_srli_si128(oqp1, 4));
}

inline void Transpose4x4(const __m128i& x0, const __m128i& x1,
                         const __m128i& x2, const __m128i& x3, __m128i* d0,
                         __m128i* d1, __m128i* d2, __m128i* d3) {
  // input
  // x0   00 01 02 03 xx xx xx xx xx xx xx xx xx xx xx xx
  // x1   10 11 12 13 xx xx xx xx xx xx xx xx xx xx xx xx
  // x2   20 21 22 23 xx xx xx xx xx xx xx xx xx xx xx xx
  // x3   30 31 32 33 xx xx xx xx xx xx xx xx xx xx xx xx
  // output
  // d0   00 10 20 30 xx xx xx xx xx xx xx xx xx xx xx xx
  // d1   01 11 21 31 xx xx xx xx xx xx xx xx xx xx xx xx
  // d2   02 12 22 32 xx xx xx xx xx xx xx xx xx xx xx xx
  // d3   03 13 23 33 xx xx xx xx xx xx xx xx xx xx xx xx

  // 00 10 01 11 02 12 03 13 04 14 05 15 06 16 07 17
  const __m128i w0 = _mm_unpacklo_epi8(x0, x1);
  // 20 30 21 31 22 32 23 33 24 34 25 35 26 36 27 37
  const __m128i w1 = _mm_unpacklo_epi8(x2, x3);

  // 00 10 20 30 01 11 21 31 02 12 22 32 03 13 23 33
  *d0 = _mm_unpacklo_epi16(w0, w1);
  // 01 11 21 31 xx xx xx xx xx xx xx xx xx xx xx xx
  *d1 = _mm_srli_si128(*d0, 4);
  // 02 12 22 32 xx xx xx xx xx xx xx xx xx xx xx xx
  *d2 = _mm_srli_si128(*d0, 8);
  // 03 13 23 33 xx xx xx xx xx xx xx xx xx xx xx xx
  *d3 = _mm_srli_si128(*d0, 12);
}

void Vertical4(void* dest, ptrdiff_t stride, int outer_thresh, int inner_thresh,
               int hev_thresh) {
  auto* const dst = static_cast<uint8_t*>(dest);
  const __m128i zero = _mm_setzero_si128();
  const __m128i v_outer_thresh =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(outer_thresh), zero);
  const __m128i v_inner_thresh =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(inner_thresh), zero);
  const __m128i v_hev_thresh0 =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(hev_thresh), zero);
  const __m128i v_hev_thresh = _mm_unpacklo_epi8(v_hev_thresh0, zero);

  __m128i x0 = Load4(dst - 2 + 0 * stride);
  __m128i x1 = Load4(dst - 2 + 1 * stride);
  __m128i x2 = Load4(dst - 2 + 2 * stride);
  __m128i x3 = Load4(dst - 2 + 3 * stride);

  // 00 10 01 11 02 12 03 13 04 14 05 15 06 16 07 17
  const __m128i w0 = _mm_unpacklo_epi8(x0, x1);
  // 20 30 21 31 22 32 23 33 24 34 25 35 26 36 27 37
  const __m128i w1 = _mm_unpacklo_epi8(x2, x3);
  // 00 10 20 30 01 11 21 31 02 12 22 32 03 13 23 33
  const __m128i d0 = _mm_unpacklo_epi16(w0, w1);
  const __m128i qp1 = _mm_shuffle_epi32(d0, 0xc);
  const __m128i qp0 = _mm_srli_si128(d0, 4);
  const __m128i q1q0 = _mm_srli_si128(d0, 8);
  const __m128i p1p0 = _mm_shuffle_epi32(d0, 0x1);

  const __m128i v_hev_mask = Hev(qp1, qp0, v_hev_thresh);
  const __m128i v_needs_mask =
      NeedsFilter4(q1q0, p1p0, qp1, qp0, v_outer_thresh, v_inner_thresh);

  __m128i oqp1;
  __m128i oqp0;
  Filter4(qp1, qp0, &oqp1, &oqp0, v_needs_mask, v_hev_mask);

  const __m128i p1 = oqp1;
  const __m128i p0 = oqp0;
  const __m128i q0 = _mm_srli_si128(oqp0, 4);
  const __m128i q1 = _mm_srli_si128(oqp1, 4);

  Transpose4x4(p1, p0, q0, q1, &x0, &x1, &x2, &x3);

  Store4(dst - 2 + 0 * stride, x0);
  Store4(dst - 2 + 1 * stride, x1);
  Store4(dst - 2 + 2 * stride, x2);
  Store4(dst - 2 + 3 * stride, x3);
}

//------------------------------------------------------------------------------
// 5-tap (chroma) filters

inline __m128i NeedsFilter6(const __m128i& q1q0, const __m128i& p1p0,
                            const __m128i& qp2, const __m128i& qp1,
                            const __m128i& qp0, const __m128i& outer_thresh,
                            const __m128i& inner_thresh) {
  const __m128i outer_mask = CheckOuterThreshF4(q1q0, p1p0, outer_thresh);
  const __m128i abs_qp2mqp1 = AbsDiff(qp2, qp1);
  const __m128i abs_qp1mqp0 = AbsDiff(qp1, qp0);
  const __m128i max_pq = _mm_max_epu8(abs_qp2mqp1, abs_qp1mqp0);
  const __m128i inner_mask = _mm_subs_epu8(
      _mm_max_epu8(max_pq, _mm_srli_si128(max_pq, 4)), inner_thresh);
  // ~mask
  const __m128i zero = _mm_setzero_si128();
  const __m128i a = _mm_or_si128(outer_mask, inner_mask);
  const __m128i b = _mm_cmpeq_epi8(a, zero);
  return b;
}

inline __m128i IsFlat3(const __m128i& qp2, const __m128i& qp1,
                       const __m128i& qp0, const __m128i& flat_thresh) {
  const __m128i abs_pq2mpq0 = AbsDiff(qp2, qp0);
  const __m128i abs_qp1mqp0 = AbsDiff(qp1, qp0);
  const __m128i max_pq = _mm_max_epu8(abs_pq2mpq0, abs_qp1mqp0);
  const __m128i flat_mask = _mm_subs_epu8(
      _mm_max_epu8(max_pq, _mm_srli_si128(max_pq, 4)), flat_thresh);
  // ~mask
  const __m128i zero = _mm_setzero_si128();
  const __m128i a = _mm_cmpeq_epi8(flat_mask, zero);
  return a;
}

inline void Filter6(const __m128i& qp2, const __m128i& qp1, const __m128i& qp0,
                    __m128i* oqp1, __m128i* oqp0) {
  const __m128i four = _mm_set1_epi16(4);
  const __m128i qp2_lo = _mm_cvtepu8_epi16(qp2);
  const __m128i qp1_lo = _mm_cvtepu8_epi16(qp1);
  const __m128i qp0_lo = _mm_cvtepu8_epi16(qp0);
  const __m128i pq1_lo = _mm_shuffle_epi32(qp1_lo, 0x4e);
  const __m128i pq0_lo = _mm_shuffle_epi32(qp0_lo, 0x4e);

  __m128i f6_lo =
      _mm_add_epi16(_mm_add_epi16(qp2_lo, four), _mm_add_epi16(qp2_lo, qp2_lo));

  f6_lo = _mm_add_epi16(_mm_add_epi16(f6_lo, qp1_lo), qp1_lo);

  f6_lo = _mm_add_epi16(_mm_add_epi16(f6_lo, qp0_lo),
                        _mm_add_epi16(qp0_lo, pq0_lo));

  // p2 * 3 + p1 * 2 + p0 * 2 + q0
  // q2 * 3 + q1 * 2 + q0 * 2 + p0
  *oqp1 = _mm_srli_epi16(f6_lo, 3);
  *oqp1 = _mm_packus_epi16(*oqp1, *oqp1);

  // p2 + p1 * 2 + p0 * 2 + q0 * 2 + q1
  // q2 + q1 * 2 + q0 * 2 + p0 * 2 + p1
  f6_lo = FilterAdd2Sub2(f6_lo, pq0_lo, pq1_lo, qp2_lo, qp2_lo);
  *oqp0 = _mm_srli_epi16(f6_lo, 3);
  *oqp0 = _mm_packus_epi16(*oqp0, *oqp0);
}

void Horizontal6(void* dest, ptrdiff_t stride, int outer_thresh,
                 int inner_thresh, int hev_thresh) {
  auto* const dst = static_cast<uint8_t*>(dest);
  const __m128i zero = _mm_setzero_si128();
  const __m128i v_flat_thresh = _mm_set1_epi8(1);
  const __m128i v_outer_thresh =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(outer_thresh), zero);
  const __m128i v_inner_thresh =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(inner_thresh), zero);
  const __m128i v_hev_thresh0 =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(hev_thresh), zero);
  const __m128i v_hev_thresh = _mm_unpacklo_epi8(v_hev_thresh0, zero);

  const __m128i p2 = Load4(dst - 3 * stride);
  const __m128i p1 = Load4(dst - 2 * stride);
  const __m128i p0 = Load4(dst - 1 * stride);
  const __m128i q0 = Load4(dst + 0 * stride);
  const __m128i q1 = Load4(dst + 1 * stride);
  const __m128i q2 = Load4(dst + 2 * stride);
  const __m128i qp2 = _mm_unpacklo_epi32(p2, q2);
  const __m128i qp1 = _mm_unpacklo_epi32(p1, q1);
  const __m128i qp0 = _mm_unpacklo_epi32(p0, q0);
  const __m128i q1q0 = _mm_unpacklo_epi32(q0, q1);
  const __m128i p1p0 = _mm_unpacklo_epi32(p0, p1);

  const __m128i v_hev_mask = Hev(qp1, qp0, v_hev_thresh);
  const __m128i v_needs_mask =
      NeedsFilter6(q1q0, p1p0, qp2, qp1, qp0, v_outer_thresh, v_inner_thresh);
  __m128i oqp1;
  __m128i oqp0;

  Filter4(qp1, qp0, &oqp1, &oqp0, v_needs_mask, v_hev_mask);

  const __m128i v_isflat3_mask = IsFlat3(qp2, qp1, qp0, v_flat_thresh);
  const __m128i v_mask =
      _mm_shuffle_epi32(_mm_and_si128(v_needs_mask, v_isflat3_mask), 0);

  if (_mm_test_all_zeros(v_mask, v_mask) == 0) {
    __m128i oqp1_f6;
    __m128i oqp0_f6;

    Filter6(qp2, qp1, qp0, &oqp1_f6, &oqp0_f6);

    oqp1 = _mm_blendv_epi8(oqp1, oqp1_f6, v_mask);
    oqp0 = _mm_blendv_epi8(oqp0, oqp0_f6, v_mask);
  }

  Store4(dst - 2 * stride, oqp1);
  Store4(dst - 1 * stride, oqp0);
  Store4(dst + 0 * stride, _mm_srli_si128(oqp0, 4));
  Store4(dst + 1 * stride, _mm_srli_si128(oqp1, 4));
}

inline void Transpose8x4To4x8(const __m128i& x0, const __m128i& x1,
                              const __m128i& x2, const __m128i& x3, __m128i* d0,
                              __m128i* d1, __m128i* d2, __m128i* d3,
                              __m128i* d4, __m128i* d5, __m128i* d6,
                              __m128i* d7) {
  // input
  // x0   00 01 02 03 04 05 06 07 xx xx xx xx xx xx xx xx
  // x1   10 11 12 13 14 15 16 17 xx xx xx xx xx xx xx xx
  // x2   20 21 22 23 24 25 26 27 xx xx xx xx xx xx xx xx
  // x3   30 31 32 33 34 35 36 37 xx xx xx xx xx xx xx xx
  // output
  // 00 10 20 30 xx xx xx xx xx xx xx xx xx xx xx xx
  // 01 11 21 31 xx xx xx xx xx xx xx xx xx xx xx xx
  // 02 12 22 32 xx xx xx xx xx xx xx xx xx xx xx xx
  // 03 13 23 33 xx xx xx xx xx xx xx xx xx xx xx xx
  // 04 14 24 34 xx xx xx xx xx xx xx xx xx xx xx xx
  // 05 15 25 35 xx xx xx xx xx xx xx xx xx xx xx xx
  // 06 16 26 36 xx xx xx xx xx xx xx xx xx xx xx xx
  // 07 17 27 37 xx xx xx xx xx xx xx xx xx xx xx xx

  // 00 10 01 11 02 12 03 13 04 14 05 15 06 16 07 17
  const __m128i w0 = _mm_unpacklo_epi8(x0, x1);
  // 20 30 21 31 22 32 23 33 24 34 25 35 26 36 27 37
  const __m128i w1 = _mm_unpacklo_epi8(x2, x3);
  // 00 10 20 30 01 11 21 31 02 12 22 32 03 13 23 33
  const __m128i ww0 = _mm_unpacklo_epi16(w0, w1);
  // 04 14 24 34 05 15 25 35 06 16 26 36 07 17 27 37
  const __m128i ww1 = _mm_unpackhi_epi16(w0, w1);

  // 00 10 20 30 xx xx xx xx xx xx xx xx xx xx xx xx
  *d0 = ww0;
  // 01 11 21 31 xx xx xx xx xx xx xx xx xx xx xx xx
  *d1 = _mm_srli_si128(ww0, 4);
  // 02 12 22 32 xx xx xx xx xx xx xx xx xx xx xx xx
  *d2 = _mm_srli_si128(ww0, 8);
  // 03 13 23 33 xx xx xx xx xx xx xx xx xx xx xx xx
  *d3 = _mm_srli_si128(ww0, 12);
  // 04 14 24 34 xx xx xx xx xx xx xx xx xx xx xx xx
  *d4 = ww1;
  // 05 15 25 35 xx xx xx xx xx xx xx xx xx xx xx xx
  *d5 = _mm_srli_si128(ww1, 4);
  // 06 16 26 36 xx xx xx xx xx xx xx xx xx xx xx xx
  *d6 = _mm_srli_si128(ww1, 8);
  // 07 17 27 37 xx xx xx xx xx xx xx xx xx xx xx xx
  *d7 = _mm_srli_si128(ww1, 12);
}

void Vertical6(void* dest, ptrdiff_t stride, int outer_thresh, int inner_thresh,
               int hev_thresh) {
  auto* const dst = static_cast<uint8_t*>(dest);
  const __m128i zero = _mm_setzero_si128();
  const __m128i v_flat_thresh = _mm_set1_epi8(1);
  const __m128i v_outer_thresh =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(outer_thresh), zero);
  const __m128i v_inner_thresh =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(inner_thresh), zero);
  const __m128i v_hev_thresh0 =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(hev_thresh), zero);
  const __m128i v_hev_thresh = _mm_unpacklo_epi8(v_hev_thresh0, zero);

  __m128i x0 = LoadLo8(dst - 3 + 0 * stride);
  __m128i x1 = LoadLo8(dst - 3 + 1 * stride);
  __m128i x2 = LoadLo8(dst - 3 + 2 * stride);
  __m128i x3 = LoadLo8(dst - 3 + 3 * stride);

  __m128i p2, p1, p0, q0, q1, q2;
  __m128i z0, z1;  // not used

  Transpose8x4To4x8(x0, x1, x2, x3, &p2, &p1, &p0, &q0, &q1, &q2, &z0, &z1);

  const __m128i qp2 = _mm_unpacklo_epi32(p2, q2);
  const __m128i qp1 = _mm_unpacklo_epi32(p1, q1);
  const __m128i qp0 = _mm_unpacklo_epi32(p0, q0);
  const __m128i q1q0 = _mm_unpacklo_epi32(q0, q1);
  const __m128i p1p0 = _mm_unpacklo_epi32(p0, p1);

  const __m128i v_hev_mask = Hev(qp1, qp0, v_hev_thresh);
  const __m128i v_needs_mask =
      NeedsFilter6(q1q0, p1p0, qp2, qp1, qp0, v_outer_thresh, v_inner_thresh);
  __m128i oqp1;
  __m128i oqp0;

  Filter4(qp1, qp0, &oqp1, &oqp0, v_needs_mask, v_hev_mask);

  const __m128i v_isflat3_mask = IsFlat3(qp2, qp1, qp0, v_flat_thresh);
  const __m128i v_mask =
      _mm_shuffle_epi32(_mm_and_si128(v_needs_mask, v_isflat3_mask), 0);

  if (_mm_test_all_zeros(v_mask, v_mask) == 0) {
    __m128i oqp1_f6;
    __m128i oqp0_f6;

    Filter6(qp2, qp1, qp0, &oqp1_f6, &oqp0_f6);

    oqp1 = _mm_blendv_epi8(oqp1, oqp1_f6, v_mask);
    oqp0 = _mm_blendv_epi8(oqp0, oqp0_f6, v_mask);
  }

  p1 = oqp1;
  p0 = oqp0;
  q0 = _mm_srli_si128(oqp0, 4);
  q1 = _mm_srli_si128(oqp1, 4);

  Transpose4x4(p1, p0, q0, q1, &x0, &x1, &x2, &x3);

  Store4(dst - 2 + 0 * stride, x0);
  Store4(dst - 2 + 1 * stride, x1);
  Store4(dst - 2 + 2 * stride, x2);
  Store4(dst - 2 + 3 * stride, x3);
}

//------------------------------------------------------------------------------
// 7-tap filters

inline __m128i NeedsFilter8(const __m128i& q1q0, const __m128i& p1p0,
                            const __m128i& qp3, const __m128i& qp2,
                            const __m128i& qp1, const __m128i& qp0,
                            const __m128i& outer_thresh,
                            const __m128i& inner_thresh) {
  const __m128i outer_mask = CheckOuterThreshF4(q1q0, p1p0, outer_thresh);
  const __m128i abs_qp2mqp1 = AbsDiff(qp2, qp1);
  const __m128i abs_qp1mqp0 = AbsDiff(qp1, qp0);
  const __m128i max_pq_a = _mm_max_epu8(abs_qp2mqp1, abs_qp1mqp0);
  const __m128i abs_pq3mpq2 = AbsDiff(qp3, qp2);
  const __m128i max_pq = _mm_max_epu8(max_pq_a, abs_pq3mpq2);
  const __m128i inner_mask = _mm_subs_epu8(
      _mm_max_epu8(max_pq, _mm_srli_si128(max_pq, 4)), inner_thresh);
  // ~mask
  const __m128i zero = _mm_setzero_si128();
  const __m128i a = _mm_or_si128(outer_mask, inner_mask);
  const __m128i b = _mm_cmpeq_epi8(a, zero);
  return b;
}

inline __m128i IsFlat4(const __m128i& qp3, const __m128i& qp2,
                       const __m128i& qp1, const __m128i& qp0,
                       const __m128i& flat_thresh) {
  const __m128i abs_pq2mpq0 = AbsDiff(qp2, qp0);
  const __m128i abs_qp1mqp0 = AbsDiff(qp1, qp0);
  const __m128i max_pq_a = _mm_max_epu8(abs_pq2mpq0, abs_qp1mqp0);
  const __m128i abs_pq3mpq0 = AbsDiff(qp3, qp0);
  const __m128i max_pq = _mm_max_epu8(max_pq_a, abs_pq3mpq0);
  const __m128i flat_mask = _mm_subs_epu8(
      _mm_max_epu8(max_pq, _mm_srli_si128(max_pq, 4)), flat_thresh);
  // ~mask
  const __m128i zero = _mm_setzero_si128();
  const __m128i a = _mm_cmpeq_epi8(flat_mask, zero);
  return a;
}

inline void Filter8(const __m128i& qp3, const __m128i& qp2, const __m128i& qp1,
                    const __m128i& qp0, __m128i* oqp2, __m128i* oqp1,
                    __m128i* oqp0) {
  const __m128i four = _mm_set1_epi16(4);
  const __m128i qp3_lo = _mm_cvtepu8_epi16(qp3);
  const __m128i qp2_lo = _mm_cvtepu8_epi16(qp2);
  const __m128i qp1_lo = _mm_cvtepu8_epi16(qp1);
  const __m128i qp0_lo = _mm_cvtepu8_epi16(qp0);
  const __m128i pq2_lo = _mm_shuffle_epi32(qp2_lo, 0x4e);
  const __m128i pq1_lo = _mm_shuffle_epi32(qp1_lo, 0x4e);
  const __m128i pq0_lo = _mm_shuffle_epi32(qp0_lo, 0x4e);

  __m128i f8_lo =
      _mm_add_epi16(_mm_add_epi16(qp3_lo, four), _mm_add_epi16(qp3_lo, qp3_lo));

  f8_lo = _mm_add_epi16(_mm_add_epi16(f8_lo, qp2_lo), qp2_lo);

  f8_lo = _mm_add_epi16(_mm_add_epi16(f8_lo, qp1_lo),
                        _mm_add_epi16(qp0_lo, pq0_lo));

  // p3 + p3 + p3 + 2 * p2 + p1 + p0 + q0
  // q3 + q3 + q3 + 2 * q2 + q1 + q0 + p0
  *oqp2 = _mm_srli_epi16(f8_lo, 3);
  *oqp2 = _mm_packus_epi16(*oqp2, *oqp2);

  // p3 + p3 + p2 + 2 * p1 + p0 + q0 + q1
  // q3 + q3 + q2 + 2 * q1 + q0 + p0 + p1
  f8_lo = FilterAdd2Sub2(f8_lo, qp1_lo, pq1_lo, qp3_lo, qp2_lo);
  *oqp1 = _mm_srli_epi16(f8_lo, 3);
  *oqp1 = _mm_packus_epi16(*oqp1, *oqp1);

  // p3 + p2 + p1 + 2 * p0 + q0 + q1 + q2
  // q3 + q2 + q1 + 2 * q0 + p0 + p1 + p2
  f8_lo = FilterAdd2Sub2(f8_lo, qp0_lo, pq2_lo, qp3_lo, qp1_lo);
  *oqp0 = _mm_srli_epi16(f8_lo, 3);
  *oqp0 = _mm_packus_epi16(*oqp0, *oqp0);
}

void Horizontal8(void* dest, ptrdiff_t stride, int outer_thresh,
                 int inner_thresh, int hev_thresh) {
  auto* const dst = static_cast<uint8_t*>(dest);
  const __m128i zero = _mm_setzero_si128();
  const __m128i v_flat_thresh = _mm_set1_epi8(1);
  const __m128i v_outer_thresh =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(outer_thresh), zero);
  const __m128i v_inner_thresh =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(inner_thresh), zero);
  const __m128i v_hev_thresh0 =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(hev_thresh), zero);
  const __m128i v_hev_thresh = _mm_unpacklo_epi8(v_hev_thresh0, zero);

  const __m128i p3 = Load4(dst - 4 * stride);
  const __m128i p2 = Load4(dst - 3 * stride);
  const __m128i p1 = Load4(dst - 2 * stride);
  const __m128i p0 = Load4(dst - 1 * stride);
  const __m128i q0 = Load4(dst + 0 * stride);
  const __m128i q1 = Load4(dst + 1 * stride);
  const __m128i q2 = Load4(dst + 2 * stride);
  const __m128i q3 = Load4(dst + 3 * stride);

  const __m128i qp3 = _mm_unpacklo_epi32(p3, q3);
  const __m128i qp2 = _mm_unpacklo_epi32(p2, q2);
  const __m128i qp1 = _mm_unpacklo_epi32(p1, q1);
  const __m128i qp0 = _mm_unpacklo_epi32(p0, q0);
  const __m128i q1q0 = _mm_unpacklo_epi32(q0, q1);
  const __m128i p1p0 = _mm_unpacklo_epi32(p0, p1);

  const __m128i v_hev_mask = Hev(qp1, qp0, v_hev_thresh);
  const __m128i v_needs_mask = NeedsFilter8(q1q0, p1p0, qp3, qp2, qp1, qp0,
                                            v_outer_thresh, v_inner_thresh);
  __m128i oqp1;
  __m128i oqp0;

  Filter4(qp1, qp0, &oqp1, &oqp0, v_needs_mask, v_hev_mask);

  const __m128i v_isflat4_mask = IsFlat4(qp3, qp2, qp1, qp0, v_flat_thresh);
  const __m128i v_mask =
      _mm_shuffle_epi32(_mm_and_si128(v_needs_mask, v_isflat4_mask), 0);

  if (_mm_test_all_zeros(v_mask, v_mask) == 0) {
    __m128i oqp2_f8;
    __m128i oqp1_f8;
    __m128i oqp0_f8;

    Filter8(qp3, qp2, qp1, qp0, &oqp2_f8, &oqp1_f8, &oqp0_f8);

    oqp2_f8 = _mm_blendv_epi8(qp2, oqp2_f8, v_mask);
    oqp1 = _mm_blendv_epi8(oqp1, oqp1_f8, v_mask);
    oqp0 = _mm_blendv_epi8(oqp0, oqp0_f8, v_mask);
    Store4(dst - 3 * stride, oqp2_f8);
    Store4(dst + 2 * stride, _mm_srli_si128(oqp2_f8, 4));
  }

  Store4(dst - 2 * stride, oqp1);
  Store4(dst - 1 * stride, oqp0);
  Store4(dst + 0 * stride, _mm_srli_si128(oqp0, 4));
  Store4(dst + 1 * stride, _mm_srli_si128(oqp1, 4));
}

inline void Transpose8x8To8x4(const __m128i& x0, const __m128i& x1,
                              const __m128i& x2, const __m128i& x3,
                              const __m128i& x4, const __m128i& x5,
                              const __m128i& x6, const __m128i& x7, __m128i* d0,
                              __m128i* d1, __m128i* d2, __m128i* d3) {
  // input
  // x0 00 01 02 03 04 05 06 07
  // x1 10 11 12 13 14 15 16 17
  // x2 20 21 22 23 24 25 26 27
  // x3 30 31 32 33 34 35 36 37
  // x4 40 41 42 43 44 45 46 47
  // x5 50 51 52 53 54 55 56 57
  // x6 60 61 62 63 64 65 66 67
  // x7 70 71 72 73 74 75 76 77
  // output
  // d0 00 10 20 30 40 50 60 70 xx xx xx xx xx xx xx xx
  // d1 01 11 21 31 41 51 61 71 xx xx xx xx xx xx xx xx
  // d2 02 12 22 32 42 52 62 72 xx xx xx xx xx xx xx xx
  // d3 03 13 23 33 43 53 63 73 xx xx xx xx xx xx xx xx

  // 00 10 01 11 02 12 03 13 04 14 05 15 06 16 07 17
  const __m128i w0 = _mm_unpacklo_epi8(x0, x1);
  // 20 30 21 31 22 32 23 33 24 34 25 35 26 36 27 37
  const __m128i w1 = _mm_unpacklo_epi8(x2, x3);
  // 40 50 41 51 42 52 43 53 44 54 45 55 46 56 47 57
  const __m128i w2 = _mm_unpacklo_epi8(x4, x5);
  // 60 70 61 71 62 72 63 73 64 74 65 75 66 76 67 77
  const __m128i w3 = _mm_unpacklo_epi8(x6, x7);

  // 00 10 20 30 01 11 21 31 02 12 22 32 03 13 23 33
  const __m128i w4 = _mm_unpacklo_epi16(w0, w1);
  // 40 50 60 70 41 51 61 71 42 52 62 72 43 53 63 73
  const __m128i w5 = _mm_unpacklo_epi16(w2, w3);

  // 00 10 20 30 40 50 60 70 01 11 21 31 41 51 61 71
  *d0 = _mm_unpacklo_epi32(w4, w5);
  *d1 = _mm_srli_si128(*d0, 8);
  // 02 12 22 32 42 52 62 72 03 13 23 33 43 53 63 73
  *d2 = _mm_unpackhi_epi32(w4, w5);
  *d3 = _mm_srli_si128(*d2, 8);
}

void Vertical8(void* dest, ptrdiff_t stride, int outer_thresh, int inner_thresh,
               int hev_thresh) {
  auto* const dst = static_cast<uint8_t*>(dest);
  const __m128i zero = _mm_setzero_si128();
  const __m128i v_flat_thresh = _mm_set1_epi8(1);
  const __m128i v_outer_thresh =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(outer_thresh), zero);
  const __m128i v_inner_thresh =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(inner_thresh), zero);
  const __m128i v_hev_thresh0 =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(hev_thresh), zero);
  const __m128i v_hev_thresh = _mm_unpacklo_epi8(v_hev_thresh0, zero);

  __m128i x0 = LoadLo8(dst - 4 + 0 * stride);
  __m128i x1 = LoadLo8(dst - 4 + 1 * stride);
  __m128i x2 = LoadLo8(dst - 4 + 2 * stride);
  __m128i x3 = LoadLo8(dst - 4 + 3 * stride);

  __m128i p3, p2, p1, p0, q0, q1, q2, q3;
  Transpose8x4To4x8(x0, x1, x2, x3, &p3, &p2, &p1, &p0, &q0, &q1, &q2, &q3);

  const __m128i qp3 = _mm_unpacklo_epi32(p3, q3);
  const __m128i qp2 = _mm_unpacklo_epi32(p2, q2);
  const __m128i qp1 = _mm_unpacklo_epi32(p1, q1);
  const __m128i qp0 = _mm_unpacklo_epi32(p0, q0);
  const __m128i q1q0 = _mm_unpacklo_epi32(q0, q1);
  const __m128i p1p0 = _mm_unpacklo_epi32(p0, p1);

  const __m128i v_hev_mask = Hev(qp1, qp0, v_hev_thresh);
  const __m128i v_needs_mask = NeedsFilter8(q1q0, p1p0, qp3, qp2, qp1, qp0,
                                            v_outer_thresh, v_inner_thresh);
  __m128i oqp1;
  __m128i oqp0;

  Filter4(qp1, qp0, &oqp1, &oqp0, v_needs_mask, v_hev_mask);

  const __m128i v_isflat4_mask = IsFlat4(qp3, qp2, qp1, qp0, v_flat_thresh);
  const __m128i v_mask =
      _mm_shuffle_epi32(_mm_and_si128(v_needs_mask, v_isflat4_mask), 0);

  if (_mm_test_all_zeros(v_mask, v_mask) == 0) {
    __m128i oqp2_f8;
    __m128i oqp1_f8;
    __m128i oqp0_f8;

    Filter8(qp3, qp2, qp1, qp0, &oqp2_f8, &oqp1_f8, &oqp0_f8);

    oqp2_f8 = _mm_blendv_epi8(qp2, oqp2_f8, v_mask);
    oqp1 = _mm_blendv_epi8(oqp1, oqp1_f8, v_mask);
    oqp0 = _mm_blendv_epi8(oqp0, oqp0_f8, v_mask);

    p2 = oqp2_f8;
    q2 = _mm_srli_si128(oqp2_f8, 4);
  }

  p1 = oqp1;
  p0 = oqp0;
  q0 = _mm_srli_si128(oqp0, 4);
  q1 = _mm_srli_si128(oqp1, 4);

  Transpose8x8To8x4(p3, p2, p1, p0, q0, q1, q2, q3, &x0, &x1, &x2, &x3);

  StoreLo8(dst - 4 + 0 * stride, x0);
  StoreLo8(dst - 4 + 1 * stride, x1);
  StoreLo8(dst - 4 + 2 * stride, x2);
  StoreLo8(dst - 4 + 3 * stride, x3);
}

//------------------------------------------------------------------------------
// 13-tap filters

inline void Filter14(const __m128i& qp6, const __m128i& qp5, const __m128i& qp4,
                     const __m128i& qp3, const __m128i& qp2, const __m128i& qp1,
                     const __m128i& qp0, __m128i* oqp5, __m128i* oqp4,
                     __m128i* oqp3, __m128i* oqp2, __m128i* oqp1,
                     __m128i* oqp0) {
  const __m128i eight = _mm_set1_epi16(8);
  const __m128i qp6_lo = _mm_cvtepu8_epi16(qp6);
  const __m128i qp5_lo = _mm_cvtepu8_epi16(qp5);
  const __m128i qp4_lo = _mm_cvtepu8_epi16(qp4);
  const __m128i qp3_lo = _mm_cvtepu8_epi16(qp3);
  const __m128i qp2_lo = _mm_cvtepu8_epi16(qp2);
  const __m128i qp1_lo = _mm_cvtepu8_epi16(qp1);
  const __m128i qp0_lo = _mm_cvtepu8_epi16(qp0);
  const __m128i pq5_lo = _mm_shuffle_epi32(qp5_lo, 0x4e);
  const __m128i pq4_lo = _mm_shuffle_epi32(qp4_lo, 0x4e);
  const __m128i pq3_lo = _mm_shuffle_epi32(qp3_lo, 0x4e);
  const __m128i pq2_lo = _mm_shuffle_epi32(qp2_lo, 0x4e);
  const __m128i pq1_lo = _mm_shuffle_epi32(qp1_lo, 0x4e);
  const __m128i pq0_lo = _mm_shuffle_epi32(qp0_lo, 0x4e);

  __m128i f14_lo =
      _mm_add_epi16(eight, _mm_sub_epi16(_mm_slli_epi16(qp6_lo, 3), qp6_lo));

  f14_lo = _mm_add_epi16(_mm_add_epi16(f14_lo, qp5_lo),
                         _mm_add_epi16(qp5_lo, qp4_lo));

  f14_lo = _mm_add_epi16(_mm_add_epi16(f14_lo, qp4_lo),
                         _mm_add_epi16(qp3_lo, qp2_lo));

  f14_lo = _mm_add_epi16(_mm_add_epi16(f14_lo, qp1_lo),
                         _mm_add_epi16(qp0_lo, pq0_lo));

  // p6 * 7 + p5 * 2 + p4 * 2 + p3 + p2 + p1 + p0 + q0
  // q6 * 7 + q5 * 2 + q4 * 2 + q3 + q2 + q1 + q0 + p0
  *oqp5 = _mm_srli_epi16(f14_lo, 4);
  *oqp5 = _mm_packus_epi16(*oqp5, *oqp5);

  // p6 * 5 + p5 * 2 + p4 * 2 + p3 * 2 + p2 + p1 + p0 + q0 + q1
  // q6 * 5 + q5 * 2 + q4 * 2 + q3 * 2 + q2 + q1 + q0 + p0 + p1
  f14_lo = FilterAdd2Sub2(f14_lo, qp3_lo, pq1_lo, qp6_lo, qp6_lo);
  *oqp4 = _mm_srli_epi16(f14_lo, 4);
  *oqp4 = _mm_packus_epi16(*oqp4, *oqp4);

  // p6 * 4 + p5 + p4 * 2 + p3 * 2 + p2 * 2 + p1 + p0 + q0 + q1 + q2
  // q6 * 4 + q5 + q4 * 2 + q3 * 2 + q2 * 2 + q1 + q0 + p0 + p1 + p2
  f14_lo = FilterAdd2Sub2(f14_lo, qp2_lo, pq2_lo, qp6_lo, qp5_lo);
  *oqp3 = _mm_srli_epi16(f14_lo, 4);
  *oqp3 = _mm_packus_epi16(*oqp3, *oqp3);

  // p6 * 3 + p5 + p4 + p3 * 2 + p2 * 2 + p1 * 2 + p0 + q0 + q1 + q2 + q3
  // q6 * 3 + q5 + q4 + q3 * 2 + q2 * 2 + q1 * 2 + q0 + p0 + p1 + p2 + p3
  f14_lo = FilterAdd2Sub2(f14_lo, qp1_lo, pq3_lo, qp6_lo, qp4_lo);
  *oqp2 = _mm_srli_epi16(f14_lo, 4);
  *oqp2 = _mm_packus_epi16(*oqp2, *oqp2);

  // p6 * 2 + p5 + p4 + p3 + p2 * 2 + p1 * 2 + p0 * 2 + q0 + q1 + q2 + q3 + q4
  // q6 * 2 + q5 + q4 + q3 + q2 * 2 + q1 * 2 + q0 * 2 + p0 + p1 + p2 + p3 + p4
  f14_lo = FilterAdd2Sub2(f14_lo, qp0_lo, pq4_lo, qp6_lo, qp3_lo);
  *oqp1 = _mm_srli_epi16(f14_lo, 4);
  *oqp1 = _mm_packus_epi16(*oqp1, *oqp1);

  // p6 + p5 + p4 + p3 + p2 + p1 * 2 + p0 * 2 + q0 * 2 + q1 + q2 + q3 + q4 + q5
  // q6 + q5 + q4 + q3 + q2 + q1 * 2 + q0 * 2 + p0 * 2 + p1 + p2 + p3 + p4 + p5
  f14_lo = FilterAdd2Sub2(f14_lo, pq0_lo, pq5_lo, qp6_lo, qp2_lo);
  *oqp0 = _mm_srli_epi16(f14_lo, 4);
  *oqp0 = _mm_packus_epi16(*oqp0, *oqp0);
}

void Horizontal14(void* dest, ptrdiff_t stride, int outer_thresh,
                  int inner_thresh, int hev_thresh) {
  auto* const dst = static_cast<uint8_t*>(dest);
  const __m128i zero = _mm_setzero_si128();
  const __m128i v_flat_thresh = _mm_set1_epi8(1);
  const __m128i v_outer_thresh =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(outer_thresh), zero);
  const __m128i v_inner_thresh =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(inner_thresh), zero);
  const __m128i v_hev_thresh0 =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(hev_thresh), zero);
  const __m128i v_hev_thresh = _mm_unpacklo_epi8(v_hev_thresh0, zero);

  const __m128i p3 = Load4(dst - 4 * stride);
  const __m128i p2 = Load4(dst - 3 * stride);
  const __m128i p1 = Load4(dst - 2 * stride);
  const __m128i p0 = Load4(dst - 1 * stride);
  const __m128i q0 = Load4(dst + 0 * stride);
  const __m128i q1 = Load4(dst + 1 * stride);
  const __m128i q2 = Load4(dst + 2 * stride);
  const __m128i q3 = Load4(dst + 3 * stride);

  const __m128i qp3 = _mm_unpacklo_epi32(p3, q3);
  const __m128i qp2 = _mm_unpacklo_epi32(p2, q2);
  const __m128i qp1 = _mm_unpacklo_epi32(p1, q1);
  const __m128i qp0 = _mm_unpacklo_epi32(p0, q0);
  const __m128i q1q0 = _mm_unpacklo_epi32(q0, q1);
  const __m128i p1p0 = _mm_unpacklo_epi32(p0, p1);

  const __m128i v_hev_mask = Hev(qp1, qp0, v_hev_thresh);
  const __m128i v_needs_mask = NeedsFilter8(q1q0, p1p0, qp3, qp2, qp1, qp0,
                                            v_outer_thresh, v_inner_thresh);

  __m128i oqp1;
  __m128i oqp0;

  Filter4(qp1, qp0, &oqp1, &oqp0, v_needs_mask, v_hev_mask);

  const __m128i v_isflat4_mask = IsFlat4(qp3, qp2, qp1, qp0, v_flat_thresh);
  const __m128i v_mask =
      _mm_shuffle_epi32(_mm_and_si128(v_needs_mask, v_isflat4_mask), 0);

  if (_mm_test_all_zeros(v_mask, v_mask) == 0) {
    const __m128i p6 = Load4(dst - 7 * stride);
    const __m128i p5 = Load4(dst - 6 * stride);
    const __m128i p4 = Load4(dst - 5 * stride);
    const __m128i q4 = Load4(dst + 4 * stride);
    const __m128i q5 = Load4(dst + 5 * stride);
    const __m128i q6 = Load4(dst + 6 * stride);
    const __m128i qp6 = _mm_unpacklo_epi32(p6, q6);
    const __m128i qp5 = _mm_unpacklo_epi32(p5, q5);
    const __m128i qp4 = _mm_unpacklo_epi32(p4, q4);

    const __m128i v_isflatouter4_mask =
        IsFlat4(qp6, qp5, qp4, qp0, v_flat_thresh);
    const __m128i v_flat4_mask =
        _mm_shuffle_epi32(_mm_and_si128(v_mask, v_isflatouter4_mask), 0);

    __m128i oqp2_f8;
    __m128i oqp1_f8;
    __m128i oqp0_f8;

    Filter8(qp3, qp2, qp1, qp0, &oqp2_f8, &oqp1_f8, &oqp0_f8);

    oqp2_f8 = _mm_blendv_epi8(qp2, oqp2_f8, v_mask);
    oqp1 = _mm_blendv_epi8(oqp1, oqp1_f8, v_mask);
    oqp0 = _mm_blendv_epi8(oqp0, oqp0_f8, v_mask);

    if (_mm_test_all_zeros(v_flat4_mask, v_flat4_mask) == 0) {
      __m128i oqp5_f14;
      __m128i oqp4_f14;
      __m128i oqp3_f14;
      __m128i oqp2_f14;
      __m128i oqp1_f14;
      __m128i oqp0_f14;

      Filter14(qp6, qp5, qp4, qp3, qp2, qp1, qp0, &oqp5_f14, &oqp4_f14,
               &oqp3_f14, &oqp2_f14, &oqp1_f14, &oqp0_f14);

      oqp5_f14 = _mm_blendv_epi8(qp5, oqp5_f14, v_flat4_mask);
      oqp4_f14 = _mm_blendv_epi8(qp4, oqp4_f14, v_flat4_mask);
      oqp3_f14 = _mm_blendv_epi8(qp3, oqp3_f14, v_flat4_mask);
      oqp2_f8 = _mm_blendv_epi8(oqp2_f8, oqp2_f14, v_flat4_mask);
      oqp1 = _mm_blendv_epi8(oqp1, oqp1_f14, v_flat4_mask);
      oqp0 = _mm_blendv_epi8(oqp0, oqp0_f14, v_flat4_mask);

      Store4(dst - 6 * stride, oqp5_f14);
      Store4(dst - 5 * stride, oqp4_f14);
      Store4(dst - 4 * stride, oqp3_f14);
      Store4(dst + 3 * stride, _mm_srli_si128(oqp3_f14, 4));
      Store4(dst + 4 * stride, _mm_srli_si128(oqp4_f14, 4));
      Store4(dst + 5 * stride, _mm_srli_si128(oqp5_f14, 4));
    }

    Store4(dst - 3 * stride, oqp2_f8);
    Store4(dst + 2 * stride, _mm_srli_si128(oqp2_f8, 4));
  }

  Store4(dst - 2 * stride, oqp1);
  Store4(dst - 1 * stride, oqp0);
  Store4(dst + 0 * stride, _mm_srli_si128(oqp0, 4));
  Store4(dst + 1 * stride, _mm_srli_si128(oqp1, 4));
}

// Each of the 8x4 blocks of input data (p7-p0 and q0-q7) are transposed to 4x8,
// then unpacked to the correct qp register. (qp7 - qp0)
//
// p7 p6 p5 p4 p3 p2 p1 p0  q0 q1 q2 q3 q4 q5 q6 q7
//
// 00 01 02 03 04 05 06 07  08 09 0a 0b 0c 0d 0e 0f
// 10 11 12 13 14 15 16 17  18 19 1a 1b 1c 1d 1e 1f
// 20 21 22 23 24 25 26 27  28 29 2a 2b 2c 2d 2e 2f
// 30 31 32 33 34 35 36 37  38 39 3a 3b 3c 3d 3e 3f

inline void DualTranspose8x4To4x8(const __m128i& x0, const __m128i& x1,
                                  const __m128i& x2, const __m128i& x3,
                                  __m128i* q0p0, __m128i* q1p1, __m128i* q2p2,
                                  __m128i* q3p3, __m128i* q4p4, __m128i* q5p5,
                                  __m128i* q6p6, __m128i* q7p7) {
  // 00 10 01 11 02 12 03 13  04 14 05 15 06 16 07 17
  const __m128i w0 = _mm_unpacklo_epi8(x0, x1);
  // 20 30 21 31 22 32 23 33  24 34 25 35 26 36 27 37
  const __m128i w1 = _mm_unpacklo_epi8(x2, x3);
  // 08 18 09 19 0a 1a 0b 1b  0c 1c 0d 1d 0e 1e 0f 1f
  const __m128i w2 = _mm_unpackhi_epi8(x0, x1);
  // 28 38 29 39 2a 3a 2b 3b  2c 3c 2d 3d 2e 3e 2f 3f
  const __m128i w3 = _mm_unpackhi_epi8(x2, x3);
  // 00 10 20 30 01 11 21 31  02 12 22 32 03 13 23 33
  const __m128i ww0 = _mm_unpacklo_epi16(w0, w1);
  // 04 14 24 34 05 15 25 35  06 16 26 36 07 17 27 37
  const __m128i ww1 = _mm_unpackhi_epi16(w0, w1);
  // 08 18 28 38 09 19 29 39  0a 1a 2a 3a 0b 1b 2b 3b
  const __m128i ww2 = _mm_unpacklo_epi16(w2, w3);
  // 0c 1c 2c 3c 0d 1d 2d 3d  0e 1e 2e 3e 0f 1f 2f 3f
  const __m128i ww3 = _mm_unpackhi_epi16(w2, w3);
  // 00 10 20 30  0f 1f 2f 3f  xx xx xx xx xx xx xx xx
  *q7p7 = _mm_unpacklo_epi32(ww0, _mm_srli_si128(ww3, 12));
  // 01 11 21 31  0e 1e 2e 3e  xx xx xx xx xx xx xx xx
  *q6p6 = _mm_unpackhi_epi32(_mm_slli_si128(ww0, 4), ww3);
  // 02 12 22 32  0d 1d 2d 3d  xx xx xx xx xx xx xx xx
  *q5p5 = _mm_unpackhi_epi32(ww0, _mm_slli_si128(ww3, 4));
  // 03 13 23 33  0c 1c 2c 3c  xx xx xx xx xx xx xx xx
  *q4p4 = _mm_unpacklo_epi32(_mm_srli_si128(ww0, 12), ww3);
  // 04 14 24 34  0b 1b 2b 3b  xx xx xx xx xx xx xx xx
  *q3p3 = _mm_unpacklo_epi32(ww1, _mm_srli_si128(ww2, 12));
  // 05 15 25 35  0a 1a 2a 3a  xx xx xx xx xx xx xx xx
  *q2p2 = _mm_unpackhi_epi32(_mm_slli_si128(ww1, 4), ww2);
  // 06 16 26 36  09 19 29 39  xx xx xx xx xx xx xx xx
  *q1p1 = _mm_unpackhi_epi32(ww1, _mm_slli_si128(ww2, 4));
  // 07 17 27 37  08 18 28 38  xx xx xx xx xx xx xx xx
  *q0p0 = _mm_unpacklo_epi32(_mm_srli_si128(ww1, 12), ww2);
}

inline void DualTranspose4x8To8x4(const __m128i& qp7, const __m128i& qp6,
                                  const __m128i& qp5, const __m128i& qp4,
                                  const __m128i& qp3, const __m128i& qp2,
                                  const __m128i& qp1, const __m128i& qp0,
                                  __m128i* x0, __m128i* x1, __m128i* x2,
                                  __m128i* x3) {
  // qp7: 00 10 20 30  0f 1f 2f 3f  xx xx xx xx xx xx xx xx
  // qp6: 01 11 21 31  0e 1e 2e 3e  xx xx xx xx xx xx xx xx
  // qp5: 02 12 22 32  0d 1d 2d 3d  xx xx xx xx xx xx xx xx
  // qp4: 03 13 23 33  0c 1c 2c 3c  xx xx xx xx xx xx xx xx
  // qp3: 04 14 24 34  0b 1b 2b 3b  xx xx xx xx xx xx xx xx
  // qp2: 05 15 25 35  0a 1a 2a 3a  xx xx xx xx xx xx xx xx
  // qp1: 06 16 26 36  09 19 29 39  xx xx xx xx xx xx xx xx
  // qp0: 07 17 27 37  08 18 28 38  xx xx xx xx xx xx xx xx

  // 00 01 10 11 20 21 30 31  0f 0e 1f 1e 2f 2e 3f 3e
  const __m128i w0 = _mm_unpacklo_epi8(qp7, qp6);
  // 02 03 12 13 22 23 32 33  xx xx xx xx xx xx xx xx
  const __m128i w1 = _mm_unpacklo_epi8(qp5, qp4);
  // 04 05 14 15 24 25 34 35  xx xx xx xx xx xx xx xx
  const __m128i w2 = _mm_unpacklo_epi8(qp3, qp2);
  // 06 07 16 17 26 27 36 37  xx xx xx xx xx xx xx xx
  const __m128i w3 = _mm_unpacklo_epi8(qp1, qp0);
  // 00 01 02 03 10 11 12 13 20 21 22 23 30 31 32 33
  const __m128i w4 = _mm_unpacklo_epi16(w0, w1);
  // 04 05 06 07 14 15 16 17 24 25 26 27 34 35 36 37
  const __m128i w5 = _mm_unpacklo_epi16(w2, w3);
  // 00 01 02 03 04 05 06 07 10 11 12 13 14 15 16 17
  const __m128i d0 = _mm_unpacklo_epi32(w4, w5);
  // 20 21 22 23 24 25 26 27 30 31 32 33 34 35 36 37
  const __m128i d2 = _mm_unpackhi_epi32(w4, w5);
  // xx xx xx xx xx xx xx xx 08 09 18 19 28 29 38 39
  const __m128i w10 = _mm_unpacklo_epi8(qp0, qp1);
  // xx xx xx xx xx xx xx xx 0a 0b 1a 1b 2a 2b 3a 3b
  const __m128i w11 = _mm_unpacklo_epi8(qp2, qp3);
  // xx xx xx xx xx xx xx xx 0c 0d 1c 1d 2c 2d 3c 3d
  const __m128i w12 = _mm_unpacklo_epi8(qp4, qp5);
  // xx xx xx xx xx xx xx xx 0e 0f 1e 1f 2e 2f 3e 3f
  const __m128i w13 = _mm_unpacklo_epi8(qp6, qp7);
  // 08 09 0a 0b 18 19 1a 1b 28 29 2a 2b 38 39 3a 3b
  const __m128i w14 = _mm_unpackhi_epi16(w10, w11);
  // 0c 0d 0e 0f 1c 1d 1e 1f 2c 2d 2e 2f 3c 3d 3e 3f
  const __m128i w15 = _mm_unpackhi_epi16(w12, w13);
  // 08 09 0a 0b 0c 0d 0e 0f 18 19 1a 1b 1c 1d 1e 1f
  const __m128i d1 = _mm_unpacklo_epi32(w14, w15);
  // 28 29 2a 2b 2c 2d 2e 2f 38 39 3a 3b 3c 3d 3e 3f
  const __m128i d3 = _mm_unpackhi_epi32(w14, w15);

  // p7 p6 p5 p4 p3 p2 p1 p0  q0 q1 q2 q3 q4 q5 q6 q7
  //
  // 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f
  *x0 = _mm_unpacklo_epi64(d0, d1);
  // 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f
  *x1 = _mm_unpackhi_epi64(d0, d1);
  // 20 21 22 23 24 25 26 27 28 29 2a 2b 2c 2d 2e 2f
  *x2 = _mm_unpacklo_epi64(d2, d3);
  // 30 31 32 33 34 35 36 37 38 39 3a 3b 3c 3d 3e 3f
  *x3 = _mm_unpackhi_epi64(d2, d3);
}

void Vertical14(void* dest, ptrdiff_t stride, int outer_thresh,
                int inner_thresh, int hev_thresh) {
  auto* const dst = static_cast<uint8_t*>(dest);
  const __m128i zero = _mm_setzero_si128();
  const __m128i v_flat_thresh = _mm_set1_epi8(1);
  const __m128i v_outer_thresh =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(outer_thresh), zero);
  const __m128i v_inner_thresh =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(inner_thresh), zero);
  const __m128i v_hev_thresh0 =
      _mm_shuffle_epi8(_mm_cvtsi32_si128(hev_thresh), zero);
  const __m128i v_hev_thresh = _mm_unpacklo_epi8(v_hev_thresh0, zero);

  __m128i x0 = LoadUnaligned16(dst - 8 + 0 * stride);
  __m128i x1 = LoadUnaligned16(dst - 8 + 1 * stride);
  __m128i x2 = LoadUnaligned16(dst - 8 + 2 * stride);
  __m128i x3 = LoadUnaligned16(dst - 8 + 3 * stride);

  __m128i qp7, qp6, qp5, qp4, qp3, qp2, qp1, qp0;

  DualTranspose8x4To4x8(x0, x1, x2, x3, &qp0, &qp1, &qp2, &qp3, &qp4, &qp5,
                        &qp6, &qp7);

  const __m128i qp1qp0 = _mm_unpacklo_epi64(qp0, qp1);
  const __m128i q1q0 = _mm_shuffle_epi32(qp1qp0, 0x0d);
  const __m128i p1p0 = _mm_shuffle_epi32(qp1qp0, 0x08);

  const __m128i v_hev_mask = Hev(qp1, qp0, v_hev_thresh);
  const __m128i v_needs_mask = NeedsFilter8(q1q0, p1p0, qp3, qp2, qp1, qp0,
                                            v_outer_thresh, v_inner_thresh);

  __m128i oqp1;
  __m128i oqp0;

  Filter4(qp1, qp0, &oqp1, &oqp0, v_needs_mask, v_hev_mask);

  const __m128i v_isflat4_mask = IsFlat4(qp3, qp2, qp1, qp0, v_flat_thresh);
  const __m128i v_mask =
      _mm_shuffle_epi32(_mm_and_si128(v_needs_mask, v_isflat4_mask), 0);

  if (_mm_test_all_zeros(v_mask, v_mask) == 0) {
    const __m128i v_isflatouter4_mask =
        IsFlat4(qp6, qp5, qp4, qp0, v_flat_thresh);
    const __m128i v_flat4_mask =
        _mm_shuffle_epi32(_mm_and_si128(v_mask, v_isflatouter4_mask), 0);

    __m128i oqp2_f8;
    __m128i oqp1_f8;
    __m128i oqp0_f8;

    Filter8(qp3, qp2, qp1, qp0, &oqp2_f8, &oqp1_f8, &oqp0_f8);

    oqp2_f8 = _mm_blendv_epi8(qp2, oqp2_f8, v_mask);
    oqp1 = _mm_blendv_epi8(oqp1, oqp1_f8, v_mask);
    oqp0 = _mm_blendv_epi8(oqp0, oqp0_f8, v_mask);

    if (_mm_test_all_zeros(v_flat4_mask, v_flat4_mask) == 0) {
      __m128i oqp5_f14;
      __m128i oqp4_f14;
      __m128i oqp3_f14;
      __m128i oqp2_f14;
      __m128i oqp1_f14;
      __m128i oqp0_f14;

      Filter14(qp6, qp5, qp4, qp3, qp2, qp1, qp0, &oqp5_f14, &oqp4_f14,
               &oqp3_f14, &oqp2_f14, &oqp1_f14, &oqp0_f14);

      oqp5_f14 = _mm_blendv_epi8(qp5, oqp5_f14, v_flat4_mask);
      oqp4_f14 = _mm_blendv_epi8(qp4, oqp4_f14, v_flat4_mask);
      oqp3_f14 = _mm_blendv_epi8(qp3, oqp3_f14, v_flat4_mask);
      oqp2_f8 = _mm_blendv_epi8(oqp2_f8, oqp2_f14, v_flat4_mask);
      oqp1 = _mm_blendv_epi8(oqp1, oqp1_f14, v_flat4_mask);
      oqp0 = _mm_blendv_epi8(oqp0, oqp0_f14, v_flat4_mask);
      qp3 = oqp3_f14;
      qp4 = oqp4_f14;
      qp5 = oqp5_f14;
    }
    qp2 = oqp2_f8;
  }

  DualTranspose4x8To8x4(qp7, qp6, qp5, qp4, qp3, qp2, oqp1, oqp0, &x0, &x1, &x2,
                        &x3);

  StoreUnaligned16(dst - 8 + 0 * stride, x0);
  StoreUnaligned16(dst - 8 + 1 * stride, x1);
  StoreUnaligned16(dst - 8 + 2 * stride, x2);
  StoreUnaligned16(dst - 8 + 3 * stride, x3);
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
  static_cast<void>(dsp);
#if DSP_ENABLED_8BPP_SSE4_1(LoopFilterSize4_LoopFilterTypeHorizontal)
  dsp->loop_filters[kLoopFilterSize4][kLoopFilterTypeHorizontal] = Horizontal4;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(LoopFilterSize6_LoopFilterTypeHorizontal)
  dsp->loop_filters[kLoopFilterSize6][kLoopFilterTypeHorizontal] = Horizontal6;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(LoopFilterSize8_LoopFilterTypeHorizontal)
  dsp->loop_filters[kLoopFilterSize8][kLoopFilterTypeHorizontal] = Horizontal8;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(LoopFilterSize14_LoopFilterTypeHorizontal)
  dsp->loop_filters[kLoopFilterSize14][kLoopFilterTypeHorizontal] =
      Horizontal14;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(LoopFilterSize4_LoopFilterTypeVertical)
  dsp->loop_filters[kLoopFilterSize4][kLoopFilterTypeVertical] = Vertical4;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(LoopFilterSize6_LoopFilterTypeVertical)
  dsp->loop_filters[kLoopFilterSize6][kLoopFilterTypeVertical] = Vertical6;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(LoopFilterSize8_LoopFilterTypeVertical)
  dsp->loop_filters[kLoopFilterSize8][kLoopFilterTypeVertical] = Vertical8;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(LoopFilterSize14_LoopFilterTypeVertical)
  dsp->loop_filters[kLoopFilterSize14][kLoopFilterTypeVertical] = Vertical14;
#endif
}
}  // namespace
}  // namespace low_bitdepth

//------------------------------------------------------------------------------
namespace high_bitdepth {
namespace {

#if LIBGAV1_MAX_BITDEPTH >= 10

template <int bitdepth>
struct LoopFilterFuncs_SSE4_1 {
  LoopFilterFuncs_SSE4_1() = delete;

  static constexpr int kThreshShift = bitdepth - 8;

  static void Vertical4(void* dest, ptrdiff_t stride, int outer_thresh,
                        int inner_thresh, int hev_thresh);
  static void Horizontal4(void* dest, ptrdiff_t stride, int outer_thresh,
                          int inner_thresh, int hev_thresh);
  static void Vertical6(void* dest, ptrdiff_t stride, int outer_thresh,
                        int inner_thresh, int hev_thresh);
  static void Horizontal6(void* dest, ptrdiff_t stride, int outer_thresh,
                          int inner_thresh, int hev_thresh);
  static void Vertical8(void* dest, ptrdiff_t stride, int outer_thresh,
                        int inner_thresh, int hev_thresh);
  static void Horizontal8(void* dest, ptrdiff_t stride, int outer_thresh,
                          int inner_thresh, int hev_thresh);
  static void Vertical14(void* dest, ptrdiff_t stride, int outer_thresh,
                         int inner_thresh, int hev_thresh);
  static void Horizontal14(void* dest, ptrdiff_t stride, int outer_thresh,
                           int inner_thresh, int hev_thresh);
};

inline __m128i Clamp(const __m128i& min, const __m128i& max,
                     const __m128i& val) {
  const __m128i a = _mm_min_epi16(val, max);
  const __m128i b = _mm_max_epi16(a, min);
  return b;
}

inline __m128i AddShift3(const __m128i& a, const __m128i& b,
                         const __m128i& vmin, const __m128i& vmax) {
  const __m128i c = _mm_adds_epi16(a, b);
  const __m128i d = Clamp(vmin, vmax, c);
  const __m128i e = _mm_srai_epi16(d, 3); /* >> 3 */
  return e;
}

inline __m128i AddShift1(const __m128i& a, const __m128i& b) {
  const __m128i c = _mm_adds_epi16(a, b);
  const __m128i e = _mm_srai_epi16(c, 1); /* >> 1 */
  return e;
}

inline __m128i AbsDiff(const __m128i& a, const __m128i& b) {
  return _mm_or_si128(_mm_subs_epu16(a, b), _mm_subs_epu16(b, a));
}

inline __m128i Hev(const __m128i& qp1, const __m128i& qp0,
                   const __m128i& hev_thresh) {
  const __m128i abs_qp1mqp0 = AbsDiff(qp1, qp0);
  const __m128i max_pq =
      _mm_max_epu16(abs_qp1mqp0, _mm_srli_si128(abs_qp1mqp0, 8));
  const __m128i hev_mask = _mm_cmpgt_epi16(max_pq, hev_thresh);
  return hev_mask;
}

inline __m128i CheckOuterThreshF4(const __m128i& q1q0, const __m128i& p1p0,
                                  const __m128i& outer_thresh) {
  //  abs(p0 - q0) * 2 + abs(p1 - q1) / 2 <= outer_thresh;
  const __m128i abs_pmq = AbsDiff(p1p0, q1q0);
  const __m128i a = _mm_adds_epu16(abs_pmq, abs_pmq);
  const __m128i b = _mm_srli_epi16(abs_pmq, 1);
  const __m128i c = _mm_adds_epu16(a, _mm_srli_si128(b, 8));
  return _mm_subs_epu16(c, outer_thresh);
}

inline __m128i NeedsFilter4(const __m128i& q1q0, const __m128i& p1p0,
                            const __m128i& qp1, const __m128i& qp0,
                            const __m128i& outer_thresh,
                            const __m128i& inner_thresh) {
  const __m128i outer_mask = CheckOuterThreshF4(q1q0, p1p0, outer_thresh);
  const __m128i abs_qp1mqp0 = AbsDiff(qp1, qp0);
  const __m128i max_abs_qp1mqp =
      _mm_max_epu16(abs_qp1mqp0, _mm_srli_si128(abs_qp1mqp0, 8));
  const __m128i inner_mask = _mm_subs_epu16(max_abs_qp1mqp, inner_thresh);
  // ~mask
  const __m128i zero = _mm_setzero_si128();
  const __m128i a = _mm_or_si128(outer_mask, inner_mask);
  const __m128i b = _mm_cmpeq_epi16(a, zero);
  return b;
}

inline void Filter4(const __m128i& qp1, const __m128i& qp0, __m128i* oqp1,
                    __m128i* oqp0, const __m128i& mask, const __m128i& hev,
                    int bitdepth) {
  const __m128i t4 = _mm_set1_epi16(4);
  const __m128i t3 = _mm_set1_epi16(3);
  const __m128i t80 = _mm_set1_epi16(static_cast<int16_t>(1 << (bitdepth - 1)));
  const __m128i t1 = _mm_set1_epi16(0x1);
  const __m128i vmin = _mm_subs_epi16(_mm_setzero_si128(), t80);
  const __m128i vmax = _mm_subs_epi16(t80, t1);
  const __m128i ps1 = _mm_subs_epi16(qp1, t80);
  const __m128i ps0 = _mm_subs_epi16(qp0, t80);
  const __m128i qs0 = _mm_srli_si128(ps0, 8);
  const __m128i qs1 = _mm_srli_si128(ps1, 8);

  __m128i a = _mm_subs_epi16(ps1, qs1);
  a = _mm_and_si128(Clamp(vmin, vmax, a), hev);

  const __m128i x = _mm_subs_epi16(qs0, ps0);
  a = _mm_adds_epi16(a, x);
  a = _mm_adds_epi16(a, x);
  a = _mm_adds_epi16(a, x);
  a = _mm_and_si128(Clamp(vmin, vmax, a), mask);

  const __m128i a1 = AddShift3(a, t4, vmin, vmax);
  const __m128i a2 = AddShift3(a, t3, vmin, vmax);
  const __m128i a3 = _mm_andnot_si128(hev, AddShift1(a1, t1));

  const __m128i ops1 = _mm_adds_epi16(ps1, a3);
  const __m128i ops0 = _mm_adds_epi16(ps0, a2);
  const __m128i oqs0 = _mm_subs_epi16(qs0, a1);
  const __m128i oqs1 = _mm_subs_epi16(qs1, a3);

  __m128i oqps1 = _mm_unpacklo_epi64(ops1, oqs1);
  __m128i oqps0 = _mm_unpacklo_epi64(ops0, oqs0);

  oqps1 = Clamp(vmin, vmax, oqps1);
  oqps0 = Clamp(vmin, vmax, oqps0);

  *oqp1 = _mm_adds_epi16(oqps1, t80);
  *oqp0 = _mm_adds_epi16(oqps0, t80);
}

template <int bitdepth>
void LoopFilterFuncs_SSE4_1<bitdepth>::Horizontal4(void* dest,
                                                   ptrdiff_t stride8,
                                                   int outer_thresh,
                                                   int inner_thresh,
                                                   int hev_thresh) {
  auto* const dst = static_cast<uint16_t*>(dest);
  const ptrdiff_t stride = stride8 / 2;
  const __m128i v_outer_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(outer_thresh << kThreshShift), 0);
  const __m128i v_inner_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(inner_thresh << kThreshShift), 0);
  const __m128i v_hev_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(hev_thresh << kThreshShift), 0);
  const __m128i p1 = LoadLo8(dst - 2 * stride);
  const __m128i p0 = LoadLo8(dst - 1 * stride);
  const __m128i qp0 = LoadHi8(p0, dst + 0 * stride);
  const __m128i qp1 = LoadHi8(p1, dst + 1 * stride);
  const __m128i q1q0 = _mm_unpackhi_epi64(qp0, qp1);
  const __m128i p1p0 = _mm_unpacklo_epi64(qp0, qp1);
  const __m128i v_hev_mask = Hev(qp1, qp0, v_hev_thresh);
  const __m128i v_needs_mask =
      NeedsFilter4(q1q0, p1p0, qp1, qp0, v_outer_thresh, v_inner_thresh);

  __m128i oqp1;
  __m128i oqp0;
  Filter4(qp1, qp0, &oqp1, &oqp0, v_needs_mask, v_hev_mask, bitdepth);

  StoreLo8(dst - 2 * stride, oqp1);
  StoreLo8(dst - 1 * stride, oqp0);
  StoreHi8(dst + 0 * stride, oqp0);
  StoreHi8(dst + 1 * stride, oqp1);
}

template <int bitdepth>
void LoopFilterFuncs_SSE4_1<bitdepth>::Vertical4(void* dest, ptrdiff_t stride8,
                                                 int outer_thresh,
                                                 int inner_thresh,
                                                 int hev_thresh) {
  auto* const dst = static_cast<uint16_t*>(dest);
  const ptrdiff_t stride = stride8 / 2;
  const __m128i v_outer_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(outer_thresh << kThreshShift), 0);
  const __m128i v_inner_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(inner_thresh << kThreshShift), 0);
  const __m128i v_hev_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(hev_thresh << kThreshShift), 0);
  const __m128i x0 = LoadLo8(dst - 2 + 0 * stride);
  const __m128i x1 = LoadLo8(dst - 2 + 1 * stride);
  const __m128i x2 = LoadLo8(dst - 2 + 2 * stride);
  const __m128i x3 = LoadLo8(dst - 2 + 3 * stride);
  // 00 10 01 11 02 12 03 13
  const __m128i w0 = _mm_unpacklo_epi16(x0, x1);
  // 20 30 21 31 22 32 23 33
  const __m128i w1 = _mm_unpacklo_epi16(x2, x3);
  // 00 10 20 30 01 11 21 31   p0p1
  const __m128i a = _mm_unpacklo_epi32(w0, w1);
  const __m128i p1p0 = _mm_shuffle_epi32(a, 0x4e);
  // 02 12 22 32 03 13 23 33   q1q0
  const __m128i q1q0 = _mm_unpackhi_epi32(w0, w1);
  const __m128i qp1 = _mm_unpackhi_epi64(p1p0, q1q0);
  const __m128i qp0 = _mm_unpacklo_epi64(p1p0, q1q0);
  const __m128i v_hev_mask = Hev(qp1, qp0, v_hev_thresh);
  const __m128i v_needs_mask =
      NeedsFilter4(q1q0, p1p0, qp1, qp0, v_outer_thresh, v_inner_thresh);

  __m128i oqp1;
  __m128i oqp0;
  Filter4(qp1, qp0, &oqp1, &oqp0, v_needs_mask, v_hev_mask, bitdepth);

  // 00 10 01 11 02 12 03 13
  const __m128i w2 = _mm_unpacklo_epi16(oqp1, oqp0);
  // 20 30 21 31 22 32 23 33
  const __m128i w3 = _mm_unpackhi_epi16(oqp0, oqp1);
  // 00 10 20 30 01 11 21 31
  const __m128i op0p1 = _mm_unpacklo_epi32(w2, w3);
  // 02 12 22 32 03 13 23 33
  const __m128i oq1q0 = _mm_unpackhi_epi32(w2, w3);

  StoreLo8(dst - 2 + 0 * stride, op0p1);
  StoreHi8(dst - 2 + 1 * stride, op0p1);
  StoreLo8(dst - 2 + 2 * stride, oq1q0);
  StoreHi8(dst - 2 + 3 * stride, oq1q0);
}

//------------------------------------------------------------------------------
// 5-tap (chroma) filters

inline __m128i CheckOuterThreshF6(const __m128i& qp1, const __m128i& qp0,
                                  const __m128i& outer_thresh) {
  //  abs(p0 - q0) * 2 + abs(p1 - q1) / 2 <= outer_thresh;
  const __m128i q1q0 = _mm_unpackhi_epi64(qp0, qp1);
  const __m128i p1p0 = _mm_unpacklo_epi64(qp0, qp1);
  return CheckOuterThreshF4(q1q0, p1p0, outer_thresh);
}

inline __m128i NeedsFilter6(const __m128i& qp2, const __m128i& qp1,
                            const __m128i& qp0, const __m128i& outer_thresh,
                            const __m128i& inner_thresh) {
  const __m128i outer_mask = CheckOuterThreshF6(qp1, qp0, outer_thresh);
  const __m128i abs_qp2mqp1 = AbsDiff(qp2, qp1);
  const __m128i abs_qp1mqp0 = AbsDiff(qp1, qp0);
  const __m128i max_pq = _mm_max_epu16(abs_qp2mqp1, abs_qp1mqp0);
  const __m128i inner_mask = _mm_subs_epu16(
      _mm_max_epu16(max_pq, _mm_srli_si128(max_pq, 8)), inner_thresh);
  // ~mask
  const __m128i zero = _mm_setzero_si128();
  const __m128i a = _mm_or_si128(outer_mask, inner_mask);
  const __m128i b = _mm_cmpeq_epi16(a, zero);
  return b;
}

inline __m128i IsFlat3(const __m128i& qp2, const __m128i& qp1,
                       const __m128i& qp0, const __m128i& flat_thresh) {
  const __m128i abs_pq2mpq0 = AbsDiff(qp2, qp0);
  const __m128i abs_qp1mqp0 = AbsDiff(qp1, qp0);
  const __m128i max_pq = _mm_max_epu16(abs_pq2mpq0, abs_qp1mqp0);
  const __m128i flat_mask = _mm_subs_epu16(
      _mm_max_epu16(max_pq, _mm_srli_si128(max_pq, 8)), flat_thresh);
  // ~mask
  const __m128i zero = _mm_setzero_si128();
  const __m128i a = _mm_cmpeq_epi16(flat_mask, zero);
  return a;
}

inline void Filter6(const __m128i& qp2, const __m128i& qp1, const __m128i& qp0,
                    __m128i* oqp1, __m128i* oqp0) {
  const __m128i four = _mm_set1_epi16(4);
  const __m128i qp2_lo = qp2;
  const __m128i qp1_lo = qp1;
  const __m128i qp0_lo = qp0;
  const __m128i pq1_lo = _mm_shuffle_epi32(qp1_lo, 0x4e);
  const __m128i pq0_lo = _mm_shuffle_epi32(qp0_lo, 0x4e);

  __m128i f6_lo;
  f6_lo =
      _mm_add_epi16(_mm_add_epi16(qp2_lo, four), _mm_add_epi16(qp2_lo, qp2_lo));

  f6_lo = _mm_add_epi16(_mm_add_epi16(f6_lo, qp1_lo), qp1_lo);

  f6_lo = _mm_add_epi16(_mm_add_epi16(f6_lo, qp0_lo),
                        _mm_add_epi16(qp0_lo, pq0_lo));

  // p2 * 3 + p1 * 2 + p0 * 2 + q0
  // q2 * 3 + q1 * 2 + q0 * 2 + p0
  *oqp1 = _mm_srli_epi16(f6_lo, 3);

  // p2 + p1 * 2 + p0 * 2 + q0 * 2 + q1
  // q2 + q1 * 2 + q0 * 2 + p0 * 2 + p1
  f6_lo = FilterAdd2Sub2(f6_lo, pq0_lo, pq1_lo, qp2_lo, qp2_lo);
  *oqp0 = _mm_srli_epi16(f6_lo, 3);
}

template <int bitdepth>
void LoopFilterFuncs_SSE4_1<bitdepth>::Horizontal6(void* dest,
                                                   ptrdiff_t stride8,
                                                   int outer_thresh,
                                                   int inner_thresh,
                                                   int hev_thresh) {
  auto* const dst = static_cast<uint16_t*>(dest);
  const ptrdiff_t stride = stride8 / 2;
  const __m128i v_flat_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(1 << kThreshShift), 0);
  const __m128i v_outer_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(outer_thresh << kThreshShift), 0);
  const __m128i v_inner_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(inner_thresh << kThreshShift), 0);
  const __m128i v_hev_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(hev_thresh << kThreshShift), 0);

  const __m128i p2 = LoadLo8(dst - 3 * stride);
  const __m128i p1 = LoadLo8(dst - 2 * stride);
  const __m128i p0 = LoadLo8(dst - 1 * stride);
  const __m128i q0 = LoadLo8(dst + 0 * stride);
  const __m128i q1 = LoadLo8(dst + 1 * stride);
  const __m128i q2 = LoadLo8(dst + 2 * stride);

  const __m128i qp2 = _mm_unpacklo_epi64(p2, q2);
  const __m128i qp1 = _mm_unpacklo_epi64(p1, q1);
  const __m128i qp0 = _mm_unpacklo_epi64(p0, q0);

  const __m128i v_hev_mask = Hev(qp1, qp0, v_hev_thresh);
  const __m128i v_needs_mask =
      NeedsFilter6(qp2, qp1, qp0, v_outer_thresh, v_inner_thresh);
  __m128i oqp1;
  __m128i oqp0;

  Filter4(qp1, qp0, &oqp1, &oqp0, v_needs_mask, v_hev_mask, bitdepth);

  const __m128i v_isflat3_mask = IsFlat3(qp2, qp1, qp0, v_flat_thresh);
  const __m128i v_mask_lo = _mm_and_si128(v_needs_mask, v_isflat3_mask);
  const __m128i v_mask = _mm_unpacklo_epi64(v_mask_lo, v_mask_lo);

  if (_mm_test_all_zeros(v_mask, v_mask) == 0) {
    __m128i oqp1_f6;
    __m128i oqp0_f6;

    Filter6(qp2, qp1, qp0, &oqp1_f6, &oqp0_f6);

    oqp1 = _mm_blendv_epi8(oqp1, oqp1_f6, v_mask);
    oqp0 = _mm_blendv_epi8(oqp0, oqp0_f6, v_mask);
  }

  StoreLo8(dst - 2 * stride, oqp1);
  StoreLo8(dst - 1 * stride, oqp0);
  StoreHi8(dst + 0 * stride, oqp0);
  StoreHi8(dst + 1 * stride, oqp1);
}

inline void Transpose8x4To4x8(const __m128i& x0, const __m128i& x1,
                              const __m128i& x2, const __m128i& x3, __m128i* d0,
                              __m128i* d1, __m128i* d2, __m128i* d3,
                              __m128i* d4, __m128i* d5, __m128i* d6,
                              __m128i* d7) {
  // input
  // x0   00 01 02 03 04 05 06 07
  // x1   10 11 12 13 14 15 16 17
  // x2   20 21 22 23 24 25 26 27
  // x3   30 31 32 33 34 35 36 37
  // output
  // 00 10 20 30 xx xx xx xx
  // 01 11 21 31 xx xx xx xx
  // 02 12 22 32 xx xx xx xx
  // 03 13 23 33 xx xx xx xx
  // 04 14 24 34 xx xx xx xx
  // 05 15 25 35 xx xx xx xx
  // 06 16 26 36 xx xx xx xx
  // 07 17 27 37 xx xx xx xx

  // 00 10 01 11 02 12 03 13
  const __m128i w0 = _mm_unpacklo_epi16(x0, x1);
  // 20 30 21 31 22 32 23 33
  const __m128i w1 = _mm_unpacklo_epi16(x2, x3);
  // 04 14 05 15 06 16 07 17
  const __m128i w2 = _mm_unpackhi_epi16(x0, x1);
  // 24 34 25 35 26 36 27 37
  const __m128i w3 = _mm_unpackhi_epi16(x2, x3);

  // 00 10 20 30 01 11 21 31
  const __m128i ww0 = _mm_unpacklo_epi32(w0, w1);
  // 04 14 24 34 05 15 25 35
  const __m128i ww1 = _mm_unpacklo_epi32(w2, w3);
  // 02 12 22 32 03 13 23 33
  const __m128i ww2 = _mm_unpackhi_epi32(w0, w1);
  // 06 16 26 36 07 17 27 37
  const __m128i ww3 = _mm_unpackhi_epi32(w2, w3);

  // 00 10 20 30 xx xx xx xx
  *d0 = ww0;
  // 01 11 21 31 xx xx xx xx
  *d1 = _mm_srli_si128(ww0, 8);
  // 02 12 22 32 xx xx xx xx
  *d2 = ww2;
  // 03 13 23 33 xx xx xx xx
  *d3 = _mm_srli_si128(ww2, 8);
  // 04 14 24 34 xx xx xx xx
  *d4 = ww1;
  // 05 15 25 35 xx xx xx xx
  *d5 = _mm_srli_si128(ww1, 8);
  // 06 16 26 36 xx xx xx xx
  *d6 = ww3;
  // 07 17 27 37 xx xx xx xx
  *d7 = _mm_srli_si128(ww3, 8);
}

template <int bitdepth>
void LoopFilterFuncs_SSE4_1<bitdepth>::Vertical6(void* dest, ptrdiff_t stride8,
                                                 int outer_thresh,
                                                 int inner_thresh,
                                                 int hev_thresh) {
  auto* const dst = static_cast<uint16_t*>(dest);
  const ptrdiff_t stride = stride8 / 2;
  const __m128i v_flat_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(1 << kThreshShift), 0);
  const __m128i v_outer_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(outer_thresh << kThreshShift), 0);
  const __m128i v_inner_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(inner_thresh << kThreshShift), 0);
  const __m128i v_hev_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(hev_thresh << kThreshShift), 0);

  __m128i x0 = LoadUnaligned16(dst - 3 + 0 * stride);
  __m128i x1 = LoadUnaligned16(dst - 3 + 1 * stride);
  __m128i x2 = LoadUnaligned16(dst - 3 + 2 * stride);
  __m128i x3 = LoadUnaligned16(dst - 3 + 3 * stride);

  __m128i p2, p1, p0, q0, q1, q2;
  __m128i z0, z1;  // not used

  Transpose8x4To4x8(x0, x1, x2, x3, &p2, &p1, &p0, &q0, &q1, &q2, &z0, &z1);

  const __m128i qp2 = _mm_unpacklo_epi64(p2, q2);
  const __m128i qp1 = _mm_unpacklo_epi64(p1, q1);
  const __m128i qp0 = _mm_unpacklo_epi64(p0, q0);

  const __m128i v_hev_mask = Hev(qp1, qp0, v_hev_thresh);
  const __m128i v_needs_mask =
      NeedsFilter6(qp2, qp1, qp0, v_outer_thresh, v_inner_thresh);
  __m128i oqp1;
  __m128i oqp0;

  Filter4(qp1, qp0, &oqp1, &oqp0, v_needs_mask, v_hev_mask, bitdepth);

  const __m128i v_isflat3_mask = IsFlat3(qp2, qp1, qp0, v_flat_thresh);
  const __m128i v_mask_lo = _mm_and_si128(v_needs_mask, v_isflat3_mask);
  const __m128i v_mask = _mm_unpacklo_epi64(v_mask_lo, v_mask_lo);

  if (_mm_test_all_zeros(v_mask, v_mask) == 0) {
    __m128i oqp1_f6;
    __m128i oqp0_f6;

    Filter6(qp2, qp1, qp0, &oqp1_f6, &oqp0_f6);

    oqp1 = _mm_blendv_epi8(oqp1, oqp1_f6, v_mask);
    oqp0 = _mm_blendv_epi8(oqp0, oqp0_f6, v_mask);
  }

  // 00 10 01 11 02 12 03 13
  const __m128i w2 = _mm_unpacklo_epi16(oqp1, oqp0);
  // 20 30 21 31 22 32 23 33
  const __m128i w3 = _mm_unpackhi_epi16(oqp0, oqp1);
  // 00 10 20 30 01 11 21 31
  const __m128i op0p1 = _mm_unpacklo_epi32(w2, w3);
  // 02 12 22 32 03 13 23 33
  const __m128i oq1q0 = _mm_unpackhi_epi32(w2, w3);

  StoreLo8(dst - 2 + 0 * stride, op0p1);
  StoreHi8(dst - 2 + 1 * stride, op0p1);
  StoreLo8(dst - 2 + 2 * stride, oq1q0);
  StoreHi8(dst - 2 + 3 * stride, oq1q0);
}

//------------------------------------------------------------------------------
// 7-tap filters
inline __m128i NeedsFilter8(const __m128i& qp3, const __m128i& qp2,
                            const __m128i& qp1, const __m128i& qp0,
                            const __m128i& outer_thresh,
                            const __m128i& inner_thresh) {
  const __m128i outer_mask = CheckOuterThreshF6(qp1, qp0, outer_thresh);
  const __m128i abs_qp2mqp1 = AbsDiff(qp2, qp1);
  const __m128i abs_qp1mqp0 = AbsDiff(qp1, qp0);
  const __m128i max_pq_a = _mm_max_epu16(abs_qp2mqp1, abs_qp1mqp0);
  const __m128i abs_pq3mpq2 = AbsDiff(qp3, qp2);
  const __m128i max_pq = _mm_max_epu16(max_pq_a, abs_pq3mpq2);
  const __m128i inner_mask = _mm_subs_epu16(
      _mm_max_epu16(max_pq, _mm_srli_si128(max_pq, 8)), inner_thresh);
  // ~mask
  const __m128i zero = _mm_setzero_si128();
  const __m128i a = _mm_or_si128(outer_mask, inner_mask);
  const __m128i b = _mm_cmpeq_epi16(a, zero);
  return b;
}

inline __m128i IsFlat4(const __m128i& qp3, const __m128i& qp2,
                       const __m128i& qp1, const __m128i& qp0,
                       const __m128i& flat_thresh) {
  const __m128i abs_pq2mpq0 = AbsDiff(qp2, qp0);
  const __m128i abs_qp1mqp0 = AbsDiff(qp1, qp0);
  const __m128i max_pq_a = _mm_max_epu16(abs_pq2mpq0, abs_qp1mqp0);
  const __m128i abs_pq3mpq0 = AbsDiff(qp3, qp0);
  const __m128i max_pq = _mm_max_epu16(max_pq_a, abs_pq3mpq0);
  const __m128i flat_mask = _mm_subs_epu16(
      _mm_max_epu16(max_pq, _mm_srli_si128(max_pq, 8)), flat_thresh);
  // ~mask
  const __m128i zero = _mm_setzero_si128();
  const __m128i a = _mm_cmpeq_epi16(flat_mask, zero);
  return a;
}

inline void Filter8(const __m128i& qp3, const __m128i& qp2, const __m128i& qp1,
                    const __m128i& qp0, __m128i* oqp2, __m128i* oqp1,
                    __m128i* oqp0) {
  const __m128i four = _mm_set1_epi16(4);
  const __m128i qp3_lo = qp3;
  const __m128i qp2_lo = qp2;
  const __m128i qp1_lo = qp1;
  const __m128i qp0_lo = qp0;
  const __m128i pq2_lo = _mm_shuffle_epi32(qp2_lo, 0x4e);
  const __m128i pq1_lo = _mm_shuffle_epi32(qp1_lo, 0x4e);
  const __m128i pq0_lo = _mm_shuffle_epi32(qp0_lo, 0x4e);

  __m128i f8_lo =
      _mm_add_epi16(_mm_add_epi16(qp3_lo, four), _mm_add_epi16(qp3_lo, qp3_lo));

  f8_lo = _mm_add_epi16(_mm_add_epi16(f8_lo, qp2_lo), qp2_lo);

  f8_lo = _mm_add_epi16(_mm_add_epi16(f8_lo, qp1_lo),
                        _mm_add_epi16(qp0_lo, pq0_lo));

  // p3 + p3 + p3 + 2 * p2 + p1 + p0 + q0
  // q3 + q3 + q3 + 2 * q2 + q1 + q0 + p0
  *oqp2 = _mm_srli_epi16(f8_lo, 3);

  // p3 + p3 + p2 + 2 * p1 + p0 + q0 + q1
  // q3 + q3 + q2 + 2 * q1 + q0 + p0 + p1
  f8_lo = FilterAdd2Sub2(f8_lo, qp1_lo, pq1_lo, qp3_lo, qp2_lo);
  *oqp1 = _mm_srli_epi16(f8_lo, 3);

  // p3 + p2 + p1 + 2 * p0 + q0 + q1 + q2
  // q3 + q2 + q1 + 2 * q0 + p0 + p1 + p2
  f8_lo = FilterAdd2Sub2(f8_lo, qp0_lo, pq2_lo, qp3_lo, qp1_lo);
  *oqp0 = _mm_srli_epi16(f8_lo, 3);
}

template <int bitdepth>
void LoopFilterFuncs_SSE4_1<bitdepth>::Horizontal8(void* dest,
                                                   ptrdiff_t stride8,
                                                   int outer_thresh,
                                                   int inner_thresh,
                                                   int hev_thresh) {
  auto* const dst = static_cast<uint16_t*>(dest);
  const ptrdiff_t stride = stride8 / 2;
  const __m128i v_flat_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(1 << kThreshShift), 0);
  const __m128i v_outer_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(outer_thresh << kThreshShift), 0);
  const __m128i v_inner_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(inner_thresh << kThreshShift), 0);
  const __m128i v_hev_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(hev_thresh << kThreshShift), 0);

  const __m128i p3 = LoadLo8(dst - 4 * stride);
  const __m128i p2 = LoadLo8(dst - 3 * stride);
  const __m128i p1 = LoadLo8(dst - 2 * stride);
  const __m128i p0 = LoadLo8(dst - 1 * stride);
  const __m128i q0 = LoadLo8(dst + 0 * stride);
  const __m128i q1 = LoadLo8(dst + 1 * stride);
  const __m128i q2 = LoadLo8(dst + 2 * stride);
  const __m128i q3 = LoadLo8(dst + 3 * stride);
  const __m128i qp3 = _mm_unpacklo_epi64(p3, q3);
  const __m128i qp2 = _mm_unpacklo_epi64(p2, q2);
  const __m128i qp1 = _mm_unpacklo_epi64(p1, q1);
  const __m128i qp0 = _mm_unpacklo_epi64(p0, q0);

  const __m128i v_hev_mask = Hev(qp1, qp0, v_hev_thresh);
  const __m128i v_needs_mask =
      NeedsFilter8(qp3, qp2, qp1, qp0, v_outer_thresh, v_inner_thresh);
  __m128i oqp1;
  __m128i oqp0;

  Filter4(qp1, qp0, &oqp1, &oqp0, v_needs_mask, v_hev_mask, bitdepth);

  const __m128i v_isflat4_mask = IsFlat4(qp3, qp2, qp1, qp0, v_flat_thresh);
  const __m128i v_mask_lo = _mm_and_si128(v_needs_mask, v_isflat4_mask);
  const __m128i v_mask = _mm_unpacklo_epi64(v_mask_lo, v_mask_lo);

  if (_mm_test_all_zeros(v_mask, v_mask) == 0) {
    __m128i oqp2_f8;
    __m128i oqp1_f8;
    __m128i oqp0_f8;

    Filter8(qp3, qp2, qp1, qp0, &oqp2_f8, &oqp1_f8, &oqp0_f8);

    oqp2_f8 = _mm_blendv_epi8(qp2, oqp2_f8, v_mask);
    oqp1 = _mm_blendv_epi8(oqp1, oqp1_f8, v_mask);
    oqp0 = _mm_blendv_epi8(oqp0, oqp0_f8, v_mask);
    StoreLo8(dst - 3 * stride, oqp2_f8);
    StoreHi8(dst + 2 * stride, oqp2_f8);
  }

  StoreLo8(dst - 2 * stride, oqp1);
  StoreLo8(dst - 1 * stride, oqp0);
  StoreHi8(dst + 0 * stride, oqp0);
  StoreHi8(dst + 1 * stride, oqp1);
}

inline void TransposeLower4x8To8x4(const __m128i& x0, const __m128i& x1,
                                   const __m128i& x2, const __m128i& x3,
                                   const __m128i& x4, const __m128i& x5,
                                   const __m128i& x6, const __m128i& x7,
                                   __m128i* d0, __m128i* d1, __m128i* d2,
                                   __m128i* d3) {
  // input
  // x0 00 01 02 03 04 05 06 07
  // x1 10 11 12 13 14 15 16 17
  // x2 20 21 22 23 24 25 26 27
  // x3 30 31 32 33 34 35 36 37
  // x4 40 41 42 43 44 45 46 47
  // x5 50 51 52 53 54 55 56 57
  // x6 60 61 62 63 64 65 66 67
  // x7 70 71 72 73 74 75 76 77
  // output
  // d0 00 10 20 30 40 50 60 70
  // d1 01 11 21 31 41 51 61 71
  // d2 02 12 22 32 42 52 62 72
  // d3 03 13 23 33 43 53 63 73

  // 00 10 01 11 02 12 03 13
  const __m128i w0 = _mm_unpacklo_epi16(x0, x1);
  // 20 30 21 31 22 32 23 33
  const __m128i w1 = _mm_unpacklo_epi16(x2, x3);
  // 40 50 41 51 42 52 43 53
  const __m128i w2 = _mm_unpacklo_epi16(x4, x5);
  // 60 70 61 71 62 72 63 73
  const __m128i w3 = _mm_unpacklo_epi16(x6, x7);

  // 00 10 20 30 01 11 21 31
  const __m128i w4 = _mm_unpacklo_epi32(w0, w1);
  // 40 50 60 70 41 51 61 71
  const __m128i w5 = _mm_unpacklo_epi32(w2, w3);
  // 02 12 22 32 03 13 23 33
  const __m128i w6 = _mm_unpackhi_epi32(w0, w1);
  // 42 52 62 72 43 53 63 73
  const __m128i w7 = _mm_unpackhi_epi32(w2, w3);

  // 00 10 20 30 40 50 60 70
  *d0 = _mm_unpacklo_epi64(w4, w5);
  // 01 11 21 31 41 51 61 71
  *d1 = _mm_unpackhi_epi64(w4, w5);
  // 02 12 22 32 42 52 62 72
  *d2 = _mm_unpacklo_epi64(w6, w7);
  // 03 13 23 33 43 53 63 73
  *d3 = _mm_unpackhi_epi64(w6, w7);
}

template <int bitdepth>
void LoopFilterFuncs_SSE4_1<bitdepth>::Vertical8(void* dest, ptrdiff_t stride8,
                                                 int outer_thresh,
                                                 int inner_thresh,
                                                 int hev_thresh) {
  auto* const dst = static_cast<uint16_t*>(dest);
  const ptrdiff_t stride = stride8 / 2;
  const __m128i v_flat_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(1 << kThreshShift), 0);
  const __m128i v_outer_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(outer_thresh << kThreshShift), 0);
  const __m128i v_inner_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(inner_thresh << kThreshShift), 0);
  const __m128i v_hev_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(hev_thresh << kThreshShift), 0);

  __m128i x0 = LoadUnaligned16(dst - 4 + 0 * stride);
  __m128i x1 = LoadUnaligned16(dst - 4 + 1 * stride);
  __m128i x2 = LoadUnaligned16(dst - 4 + 2 * stride);
  __m128i x3 = LoadUnaligned16(dst - 4 + 3 * stride);

  __m128i p3, p2, p1, p0, q0, q1, q2, q3;
  Transpose8x4To4x8(x0, x1, x2, x3, &p3, &p2, &p1, &p0, &q0, &q1, &q2, &q3);

  const __m128i qp3 = _mm_unpacklo_epi64(p3, q3);
  const __m128i qp2 = _mm_unpacklo_epi64(p2, q2);
  const __m128i qp1 = _mm_unpacklo_epi64(p1, q1);
  const __m128i qp0 = _mm_unpacklo_epi64(p0, q0);

  const __m128i v_hev_mask = Hev(qp1, qp0, v_hev_thresh);
  const __m128i v_needs_mask =
      NeedsFilter8(qp3, qp2, qp1, qp0, v_outer_thresh, v_inner_thresh);
  __m128i oqp1;
  __m128i oqp0;

  Filter4(qp1, qp0, &oqp1, &oqp0, v_needs_mask, v_hev_mask, bitdepth);

  const __m128i v_isflat4_mask = IsFlat4(qp3, qp2, qp1, qp0, v_flat_thresh);
  const __m128i v_mask_lo = _mm_and_si128(v_needs_mask, v_isflat4_mask);
  const __m128i v_mask = _mm_unpacklo_epi64(v_mask_lo, v_mask_lo);

  if (_mm_test_all_zeros(v_mask, v_mask) == 0) {
    __m128i oqp2_f8;
    __m128i oqp1_f8;
    __m128i oqp0_f8;

    Filter8(qp3, qp2, qp1, qp0, &oqp2_f8, &oqp1_f8, &oqp0_f8);

    oqp2_f8 = _mm_blendv_epi8(qp2, oqp2_f8, v_mask);
    oqp1 = _mm_blendv_epi8(oqp1, oqp1_f8, v_mask);
    oqp0 = _mm_blendv_epi8(oqp0, oqp0_f8, v_mask);

    p2 = oqp2_f8;
    q2 = _mm_srli_si128(oqp2_f8, 8);
  }

  p1 = oqp1;
  p0 = oqp0;
  q0 = _mm_srli_si128(oqp0, 8);
  q1 = _mm_srli_si128(oqp1, 8);

  TransposeLower4x8To8x4(p3, p2, p1, p0, q0, q1, q2, q3, &x0, &x1, &x2, &x3);

  StoreUnaligned16(dst - 4 + 0 * stride, x0);
  StoreUnaligned16(dst - 4 + 1 * stride, x1);
  StoreUnaligned16(dst - 4 + 2 * stride, x2);
  StoreUnaligned16(dst - 4 + 3 * stride, x3);
}

//------------------------------------------------------------------------------
// 13-tap filters

inline void Filter14(const __m128i& qp6, const __m128i& qp5, const __m128i& qp4,
                     const __m128i& qp3, const __m128i& qp2, const __m128i& qp1,
                     const __m128i& qp0, __m128i* oqp5, __m128i* oqp4,
                     __m128i* oqp3, __m128i* oqp2, __m128i* oqp1,
                     __m128i* oqp0) {
  const __m128i eight = _mm_set1_epi16(8);
  const __m128i qp6_lo = qp6;
  const __m128i qp5_lo = qp5;
  const __m128i qp4_lo = qp4;
  const __m128i qp3_lo = qp3;
  const __m128i qp2_lo = qp2;
  const __m128i qp1_lo = qp1;
  const __m128i qp0_lo = qp0;
  const __m128i pq5_lo = _mm_shuffle_epi32(qp5_lo, 0x4e);
  const __m128i pq4_lo = _mm_shuffle_epi32(qp4_lo, 0x4e);
  const __m128i pq3_lo = _mm_shuffle_epi32(qp3_lo, 0x4e);
  const __m128i pq2_lo = _mm_shuffle_epi32(qp2_lo, 0x4e);
  const __m128i pq1_lo = _mm_shuffle_epi32(qp1_lo, 0x4e);
  const __m128i pq0_lo = _mm_shuffle_epi32(qp0_lo, 0x4e);

  __m128i f14_lo =
      _mm_add_epi16(eight, _mm_sub_epi16(_mm_slli_epi16(qp6_lo, 3), qp6_lo));

  f14_lo = _mm_add_epi16(_mm_add_epi16(f14_lo, qp5_lo),
                         _mm_add_epi16(qp5_lo, qp4_lo));

  f14_lo = _mm_add_epi16(_mm_add_epi16(f14_lo, qp4_lo),
                         _mm_add_epi16(qp3_lo, qp2_lo));

  f14_lo = _mm_add_epi16(_mm_add_epi16(f14_lo, qp1_lo),
                         _mm_add_epi16(qp0_lo, pq0_lo));

  // p6 * 7 + p5 * 2 + p4 * 2 + p3 + p2 + p1 + p0 + q0
  // q6 * 7 + q5 * 2 + q4 * 2 + q3 + q2 + q1 + q0 + p0
  *oqp5 = _mm_srli_epi16(f14_lo, 4);

  // p6 * 5 + p5 * 2 + p4 * 2 + p3 * 2 + p2 + p1 + p0 + q0 + q1
  // q6 * 5 + q5 * 2 + q4 * 2 + q3 * 2 + q2 + q1 + q0 + p0 + p1
  f14_lo = FilterAdd2Sub2(f14_lo, qp3_lo, pq1_lo, qp6_lo, qp6_lo);
  *oqp4 = _mm_srli_epi16(f14_lo, 4);

  // p6 * 4 + p5 + p4 * 2 + p3 * 2 + p2 * 2 + p1 + p0 + q0 + q1 + q2
  // q6 * 4 + q5 + q4 * 2 + q3 * 2 + q2 * 2 + q1 + q0 + p0 + p1 + p2
  f14_lo = FilterAdd2Sub2(f14_lo, qp2_lo, pq2_lo, qp6_lo, qp5_lo);
  *oqp3 = _mm_srli_epi16(f14_lo, 4);

  // p6 * 3 + p5 + p4 + p3 * 2 + p2 * 2 + p1 * 2 + p0 + q0 + q1 + q2 + q3
  // q6 * 3 + q5 + q4 + q3 * 2 + q2 * 2 + q1 * 2 + q0 + p0 + p1 + p2 + p3
  f14_lo = FilterAdd2Sub2(f14_lo, qp1_lo, pq3_lo, qp6_lo, qp4_lo);
  *oqp2 = _mm_srli_epi16(f14_lo, 4);

  // p6 * 2 + p5 + p4 + p3 + p2 * 2 + p1 * 2 + p0 * 2 + q0 + q1 + q2 + q3 + q4
  // q6 * 2 + q5 + q4 + q3 + q2 * 2 + q1 * 2 + q0 * 2 + p0 + p1 + p2 + p3 + p4
  f14_lo = FilterAdd2Sub2(f14_lo, qp0_lo, pq4_lo, qp6_lo, qp3_lo);
  *oqp1 = _mm_srli_epi16(f14_lo, 4);

  // p6 + p5 + p4 + p3 + p2 + p1 * 2 + p0 * 2 + q0 * 2 + q1 + q2 + q3 + q4 + q5
  // q6 + q5 + q4 + q3 + q2 + q1 * 2 + q0 * 2 + p0 * 2 + p1 + p2 + p3 + p4 + p5
  f14_lo = FilterAdd2Sub2(f14_lo, pq0_lo, pq5_lo, qp6_lo, qp2_lo);
  *oqp0 = _mm_srli_epi16(f14_lo, 4);
}

template <int bitdepth>
void LoopFilterFuncs_SSE4_1<bitdepth>::Horizontal14(void* dest,
                                                    ptrdiff_t stride8,
                                                    int outer_thresh,
                                                    int inner_thresh,
                                                    int hev_thresh) {
  auto* const dst = static_cast<uint16_t*>(dest);
  const ptrdiff_t stride = stride8 / 2;
  const __m128i v_flat_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(1 << kThreshShift), 0);
  const __m128i v_outer_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(outer_thresh << kThreshShift), 0);
  const __m128i v_inner_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(inner_thresh << kThreshShift), 0);
  const __m128i v_hev_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(hev_thresh << kThreshShift), 0);

  const __m128i p3 = LoadLo8(dst - 4 * stride);
  const __m128i p2 = LoadLo8(dst - 3 * stride);
  const __m128i p1 = LoadLo8(dst - 2 * stride);
  const __m128i p0 = LoadLo8(dst - 1 * stride);
  const __m128i q0 = LoadLo8(dst + 0 * stride);
  const __m128i q1 = LoadLo8(dst + 1 * stride);
  const __m128i q2 = LoadLo8(dst + 2 * stride);
  const __m128i q3 = LoadLo8(dst + 3 * stride);
  const __m128i qp3 = _mm_unpacklo_epi64(p3, q3);
  const __m128i qp2 = _mm_unpacklo_epi64(p2, q2);
  const __m128i qp1 = _mm_unpacklo_epi64(p1, q1);
  const __m128i qp0 = _mm_unpacklo_epi64(p0, q0);

  const __m128i v_hev_mask = Hev(qp1, qp0, v_hev_thresh);
  const __m128i v_needs_mask =
      NeedsFilter8(qp3, qp2, qp1, qp0, v_outer_thresh, v_inner_thresh);

  __m128i oqp1;
  __m128i oqp0;

  Filter4(qp1, qp0, &oqp1, &oqp0, v_needs_mask, v_hev_mask, bitdepth);

  const __m128i v_isflat4_mask = IsFlat4(qp3, qp2, qp1, qp0, v_flat_thresh);
  const __m128i v_mask_lo = _mm_and_si128(v_needs_mask, v_isflat4_mask);
  const __m128i v_mask = _mm_unpacklo_epi64(v_mask_lo, v_mask_lo);

  if (_mm_test_all_zeros(v_mask, v_mask) == 0) {
    const __m128i p6 = LoadLo8(dst - 7 * stride);
    const __m128i p5 = LoadLo8(dst - 6 * stride);
    const __m128i p4 = LoadLo8(dst - 5 * stride);
    const __m128i q4 = LoadLo8(dst + 4 * stride);
    const __m128i q5 = LoadLo8(dst + 5 * stride);
    const __m128i q6 = LoadLo8(dst + 6 * stride);
    const __m128i qp6 = _mm_unpacklo_epi64(p6, q6);
    const __m128i qp5 = _mm_unpacklo_epi64(p5, q5);
    const __m128i qp4 = _mm_unpacklo_epi64(p4, q4);

    const __m128i v_isflatouter4_mask =
        IsFlat4(qp6, qp5, qp4, qp0, v_flat_thresh);
    const __m128i v_flat4_mask_lo = _mm_and_si128(v_mask, v_isflatouter4_mask);
    const __m128i v_flat4_mask =
        _mm_unpacklo_epi64(v_flat4_mask_lo, v_flat4_mask_lo);

    __m128i oqp2_f8;
    __m128i oqp1_f8;
    __m128i oqp0_f8;

    Filter8(qp3, qp2, qp1, qp0, &oqp2_f8, &oqp1_f8, &oqp0_f8);

    oqp2_f8 = _mm_blendv_epi8(qp2, oqp2_f8, v_mask);
    oqp1 = _mm_blendv_epi8(oqp1, oqp1_f8, v_mask);
    oqp0 = _mm_blendv_epi8(oqp0, oqp0_f8, v_mask);

    if (_mm_test_all_zeros(v_flat4_mask, v_flat4_mask) == 0) {
      __m128i oqp5_f14;
      __m128i oqp4_f14;
      __m128i oqp3_f14;
      __m128i oqp2_f14;
      __m128i oqp1_f14;
      __m128i oqp0_f14;

      Filter14(qp6, qp5, qp4, qp3, qp2, qp1, qp0, &oqp5_f14, &oqp4_f14,
               &oqp3_f14, &oqp2_f14, &oqp1_f14, &oqp0_f14);

      oqp5_f14 = _mm_blendv_epi8(qp5, oqp5_f14, v_flat4_mask);
      oqp4_f14 = _mm_blendv_epi8(qp4, oqp4_f14, v_flat4_mask);
      oqp3_f14 = _mm_blendv_epi8(qp3, oqp3_f14, v_flat4_mask);
      oqp2_f8 = _mm_blendv_epi8(oqp2_f8, oqp2_f14, v_flat4_mask);
      oqp1 = _mm_blendv_epi8(oqp1, oqp1_f14, v_flat4_mask);
      oqp0 = _mm_blendv_epi8(oqp0, oqp0_f14, v_flat4_mask);

      StoreLo8(dst - 6 * stride, oqp5_f14);
      StoreLo8(dst - 5 * stride, oqp4_f14);
      StoreLo8(dst - 4 * stride, oqp3_f14);

      StoreHi8(dst + 3 * stride, oqp3_f14);
      StoreHi8(dst + 4 * stride, oqp4_f14);
      StoreHi8(dst + 5 * stride, oqp5_f14);
    }

    StoreLo8(dst - 3 * stride, oqp2_f8);
    StoreHi8(dst + 2 * stride, oqp2_f8);
  }

  StoreLo8(dst - 2 * stride, oqp1);
  StoreLo8(dst - 1 * stride, oqp0);
  StoreHi8(dst + 0 * stride, oqp0);
  StoreHi8(dst + 1 * stride, oqp1);
}

inline void TransposeUpper4x8To8x4(const __m128i& x0, const __m128i& x1,
                                   const __m128i& x2, const __m128i& x3,
                                   const __m128i& x4, const __m128i& x5,
                                   const __m128i& x6, const __m128i& x7,
                                   __m128i* d0, __m128i* d1, __m128i* d2,
                                   __m128i* d3) {
  // input
  // x0 00 01 02 03 xx xx xx xx
  // x1 10 11 12 13 xx xx xx xx
  // x2 20 21 22 23 xx xx xx xx
  // x3 30 31 32 33 xx xx xx xx
  // x4 40 41 42 43 xx xx xx xx
  // x5 50 51 52 53 xx xx xx xx
  // x6 60 61 62 63 xx xx xx xx
  // x7 70 71 72 73 xx xx xx xx
  // output
  // d0 00 10 20 30 40 50 60 70
  // d1 01 11 21 31 41 51 61 71
  // d2 02 12 22 32 42 52 62 72
  // d3 03 13 23 33 43 53 63 73

  // 00 10 01 11 02 12 03 13
  const __m128i w0 = _mm_unpackhi_epi16(x0, x1);
  // 20 30 21 31 22 32 23 33
  const __m128i w1 = _mm_unpackhi_epi16(x2, x3);
  // 40 50 41 51 42 52 43 53
  const __m128i w2 = _mm_unpackhi_epi16(x4, x5);
  // 60 70 61 71 62 72 63 73
  const __m128i w3 = _mm_unpackhi_epi16(x6, x7);

  // 00 10 20 30 01 11 21 31
  const __m128i w4 = _mm_unpacklo_epi32(w0, w1);
  // 40 50 60 70 41 51 61 71
  const __m128i w5 = _mm_unpacklo_epi32(w2, w3);
  // 02 12 22 32 03 13 23 33
  const __m128i w6 = _mm_unpackhi_epi32(w0, w1);
  // 42 52 62 72 43 53 63 73
  const __m128i w7 = _mm_unpackhi_epi32(w2, w3);

  // 00 10 20 30 40 50 60 70
  *d0 = _mm_unpacklo_epi64(w4, w5);
  // 01 11 21 31 41 51 61 71
  *d1 = _mm_unpackhi_epi64(w4, w5);
  // 02 12 22 32 42 52 62 72
  *d2 = _mm_unpacklo_epi64(w6, w7);
  // 03 13 23 33 43 53 63 73
  *d3 = _mm_unpackhi_epi64(w6, w7);
}

template <int bitdepth>
void LoopFilterFuncs_SSE4_1<bitdepth>::Vertical14(void* dest, ptrdiff_t stride8,
                                                  int outer_thresh,
                                                  int inner_thresh,
                                                  int hev_thresh) {
  auto* const dst = static_cast<uint16_t*>(dest);
  const ptrdiff_t stride = stride8 / 2;
  const __m128i v_flat_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(1 << kThreshShift), 0);
  const __m128i v_outer_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(outer_thresh << kThreshShift), 0);
  const __m128i v_inner_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(inner_thresh << kThreshShift), 0);
  const __m128i v_hev_thresh =
      _mm_shufflelo_epi16(_mm_cvtsi32_si128(hev_thresh << kThreshShift), 0);

  // p7 p6 p5 p4 p3 p2 p1 p0  q0 q1 q2 q3 q4 q5 q6 q7
  //
  // 00 01 02 03 04 05 06 07  08 09 0a 0b 0c 0d 0e 0f
  // 10 11 12 13 14 15 16 17  18 19 1a 1b 1c 1d 1e 1f
  // 20 21 22 23 24 25 26 27  28 29 2a 2b 2c 2d 2e 2f
  // 30 31 32 33 34 35 36 37  38 39 3a 3b 3c 3d 3e 3f

  __m128i x0 = LoadUnaligned16(dst - 8 + 0 * stride);
  __m128i x1 = LoadUnaligned16(dst - 8 + 1 * stride);
  __m128i x2 = LoadUnaligned16(dst - 8 + 2 * stride);
  __m128i x3 = LoadUnaligned16(dst - 8 + 3 * stride);

  __m128i p7, p6, p5, p4, p3, p2, p1, p0;
  __m128i q7, q6, q5, q4, q3, q2, q1, q0;

  Transpose8x4To4x8(x0, x1, x2, x3, &p7, &p6, &p5, &p4, &p3, &p2, &p1, &p0);

  x0 = LoadUnaligned16(dst - 8 + 8 + 0 * stride);
  x1 = LoadUnaligned16(dst - 8 + 8 + 1 * stride);
  x2 = LoadUnaligned16(dst - 8 + 8 + 2 * stride);
  x3 = LoadUnaligned16(dst - 8 + 8 + 3 * stride);

  Transpose8x4To4x8(x0, x1, x2, x3, &q0, &q1, &q2, &q3, &q4, &q5, &q6, &q7);

  __m128i qp7 = _mm_unpacklo_epi64(p7, q7);
  __m128i qp6 = _mm_unpacklo_epi64(p6, q6);
  __m128i qp5 = _mm_unpacklo_epi64(p5, q5);
  __m128i qp4 = _mm_unpacklo_epi64(p4, q4);
  __m128i qp3 = _mm_unpacklo_epi64(p3, q3);
  __m128i qp2 = _mm_unpacklo_epi64(p2, q2);
  __m128i qp1 = _mm_unpacklo_epi64(p1, q1);
  __m128i qp0 = _mm_unpacklo_epi64(p0, q0);

  const __m128i v_hev_mask = Hev(qp1, qp0, v_hev_thresh);
  const __m128i v_needs_mask =
      NeedsFilter8(qp3, qp2, qp1, qp0, v_outer_thresh, v_inner_thresh);

  __m128i oqp1;
  __m128i oqp0;

  Filter4(qp1, qp0, &oqp1, &oqp0, v_needs_mask, v_hev_mask, bitdepth);

  const __m128i v_isflat4_mask = IsFlat4(qp3, qp2, qp1, qp0, v_flat_thresh);
  const __m128i v_mask_lo = _mm_and_si128(v_needs_mask, v_isflat4_mask);
  const __m128i v_mask = _mm_unpacklo_epi64(v_mask_lo, v_mask_lo);

  if (_mm_test_all_zeros(v_mask, v_mask) == 0) {
    const __m128i v_isflatouter4_mask =
        IsFlat4(qp6, qp5, qp4, qp0, v_flat_thresh);
    const __m128i v_flat4_mask_lo = _mm_and_si128(v_mask, v_isflatouter4_mask);
    const __m128i v_flat4_mask =
        _mm_unpacklo_epi64(v_flat4_mask_lo, v_flat4_mask_lo);

    __m128i oqp2_f8;
    __m128i oqp1_f8;
    __m128i oqp0_f8;

    Filter8(qp3, qp2, qp1, qp0, &oqp2_f8, &oqp1_f8, &oqp0_f8);

    oqp2_f8 = _mm_blendv_epi8(qp2, oqp2_f8, v_mask);
    oqp1 = _mm_blendv_epi8(oqp1, oqp1_f8, v_mask);
    oqp0 = _mm_blendv_epi8(oqp0, oqp0_f8, v_mask);

    if (_mm_test_all_zeros(v_flat4_mask, v_flat4_mask) == 0) {
      __m128i oqp5_f14;
      __m128i oqp4_f14;
      __m128i oqp3_f14;
      __m128i oqp2_f14;
      __m128i oqp1_f14;
      __m128i oqp0_f14;

      Filter14(qp6, qp5, qp4, qp3, qp2, qp1, qp0, &oqp5_f14, &oqp4_f14,
               &oqp3_f14, &oqp2_f14, &oqp1_f14, &oqp0_f14);

      oqp5_f14 = _mm_blendv_epi8(qp5, oqp5_f14, v_flat4_mask);
      oqp4_f14 = _mm_blendv_epi8(qp4, oqp4_f14, v_flat4_mask);
      oqp3_f14 = _mm_blendv_epi8(qp3, oqp3_f14, v_flat4_mask);
      oqp2_f8 = _mm_blendv_epi8(oqp2_f8, oqp2_f14, v_flat4_mask);
      oqp1 = _mm_blendv_epi8(oqp1, oqp1_f14, v_flat4_mask);
      oqp0 = _mm_blendv_epi8(oqp0, oqp0_f14, v_flat4_mask);
      qp3 = oqp3_f14;
      qp4 = oqp4_f14;
      qp5 = oqp5_f14;
    }
    qp2 = oqp2_f8;
  }

  TransposeLower4x8To8x4(qp7, qp6, qp5, qp4, qp3, qp2, oqp1, oqp0, &x0, &x1,
                         &x2, &x3);

  StoreUnaligned16(dst - 8 + 0 * stride, x0);
  StoreUnaligned16(dst - 8 + 1 * stride, x1);
  StoreUnaligned16(dst - 8 + 2 * stride, x2);
  StoreUnaligned16(dst - 8 + 3 * stride, x3);

  TransposeUpper4x8To8x4(oqp0, oqp1, qp2, qp3, qp4, qp5, qp6, qp7, &x0, &x1,
                         &x2, &x3);

  StoreUnaligned16(dst - 8 + 8 + 0 * stride, x0);
  StoreUnaligned16(dst - 8 + 8 + 1 * stride, x1);
  StoreUnaligned16(dst - 8 + 8 + 2 * stride, x2);
  StoreUnaligned16(dst - 8 + 8 + 3 * stride, x3);
}

using Defs10bpp = LoopFilterFuncs_SSE4_1<kBitdepth10>;

void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth10);
  assert(dsp != nullptr);
  static_cast<void>(dsp);
#if DSP_ENABLED_10BPP_SSE4_1(LoopFilterSize4_LoopFilterTypeHorizontal)
  dsp->loop_filters[kLoopFilterSize4][kLoopFilterTypeHorizontal] =
      Defs10bpp::Horizontal4;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(LoopFilterSize6_LoopFilterTypeHorizontal)
  dsp->loop_filters[kLoopFilterSize6][kLoopFilterTypeHorizontal] =
      Defs10bpp::Horizontal6;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(LoopFilterSize8_LoopFilterTypeHorizontal)
  dsp->loop_filters[kLoopFilterSize8][kLoopFilterTypeHorizontal] =
      Defs10bpp::Horizontal8;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(LoopFilterSize14_LoopFilterTypeHorizontal)
  dsp->loop_filters[kLoopFilterSize14][kLoopFilterTypeHorizontal] =
      Defs10bpp::Horizontal14;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(LoopFilterSize4_LoopFilterTypeVertical)
  dsp->loop_filters[kLoopFilterSize4][kLoopFilterTypeVertical] =
      Defs10bpp::Vertical4;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(LoopFilterSize6_LoopFilterTypeVertical)
  dsp->loop_filters[kLoopFilterSize6][kLoopFilterTypeVertical] =
      Defs10bpp::Vertical6;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(LoopFilterSize8_LoopFilterTypeVertical)
  dsp->loop_filters[kLoopFilterSize8][kLoopFilterTypeVertical] =
      Defs10bpp::Vertical8;
#endif
#if DSP_ENABLED_10BPP_SSE4_1(LoopFilterSize14_LoopFilterTypeVertical)
  dsp->loop_filters[kLoopFilterSize14][kLoopFilterTypeVertical] =
      Defs10bpp::Vertical14;
#endif
}
#endif
}  // namespace
}  // namespace high_bitdepth

void LoopFilterInit_SSE4_1() {
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

void LoopFilterInit_SSE4_1() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_TARGETING_SSE4_1
