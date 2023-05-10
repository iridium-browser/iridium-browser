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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "src/dsp/dsp.h"
#include "src/utils/common.h"

namespace libgav1 {
namespace dsp {
namespace {

// 7.14.6.1.
template <int bitdepth, typename Pixel>
struct LoopFilterFuncs_C {
  LoopFilterFuncs_C() = delete;

  static constexpr int kMaxPixel = (1 << bitdepth) - 1;
  static constexpr int kMinSignedPixel = -(1 << (bitdepth - 1));
  static constexpr int kMaxSignedPixel = (1 << (bitdepth - 1)) - 1;
  static constexpr int kFlatThresh = 1 << (bitdepth - 8);

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

inline void AdjustThresholds(const int bitdepth, int* const outer_thresh,
                             int* const inner_thresh, int* const hev_thresh) {
  assert(*outer_thresh >= 7 && *outer_thresh <= 3 * kMaxLoopFilterValue + 4);
  assert(*inner_thresh >= 1 && *inner_thresh <= kMaxLoopFilterValue);
  assert(*hev_thresh >= 0 && *hev_thresh <= 3);
  *outer_thresh <<= bitdepth - 8;
  *inner_thresh <<= bitdepth - 8;
  *hev_thresh <<= bitdepth - 8;
}

//------------------------------------------------------------------------------
// 4-tap filters

// 7.14.6.2.
template <typename Pixel>
inline bool NeedsFilter4(const Pixel* p, ptrdiff_t step, int outer_thresh,
                         int inner_thresh) {
  const int p1 = p[-2 * step], p0 = p[-step];
  const int q0 = p[0], q1 = p[step];
  return std::abs(p1 - p0) <= inner_thresh &&
         std::abs(q1 - q0) <= inner_thresh &&
         std::abs(p0 - q0) * 2 + std::abs(p1 - q1) / 2 <= outer_thresh;
}

// 7.14.6.2.
template <typename Pixel>
inline bool Hev(const Pixel* p, ptrdiff_t step, int thresh) {
  const int p1 = p[-2 * step], p0 = p[-step], q0 = p[0], q1 = p[step];
  return (std::abs(p1 - p0) > thresh) || (std::abs(q1 - q0) > thresh);
}

// 7.14.6.3.
// 4 pixels in, 2 pixels out.
template <int bitdepth, typename Pixel>
inline void Filter2_C(Pixel* p, ptrdiff_t step) {
  const int p1 = p[-2 * step], p0 = p[-step], q0 = p[0], q1 = p[step];
  const int min_signed_val =
      LoopFilterFuncs_C<bitdepth, Pixel>::kMinSignedPixel;
  const int max_signed_val =
      LoopFilterFuncs_C<bitdepth, Pixel>::kMaxSignedPixel;
  // 8bpp: [-893,892], 10bpp: [-3581,3580], 12bpp [-14333,14332]
  const int a = 3 * (q0 - p0) + Clip3(p1 - q1, min_signed_val, max_signed_val);
  // 8bpp: [-16,15], 10bpp: [-64,63], 12bpp: [-256,255]
  const int a1 = Clip3(a + 4, min_signed_val, max_signed_val) >> 3;
  const int a2 = Clip3(a + 3, min_signed_val, max_signed_val) >> 3;
  const int max_unsigned_val = LoopFilterFuncs_C<bitdepth, Pixel>::kMaxPixel;
  p[-step] = Clip3(p0 + a2, 0, max_unsigned_val);
  p[0] = Clip3(q0 - a1, 0, max_unsigned_val);
}

// 7.14.6.3.
// 4 pixels in, 4 pixels out.
template <int bitdepth, typename Pixel>
inline void Filter4_C(Pixel* p, ptrdiff_t step) {
  const int p1 = p[-2 * step], p0 = p[-step], q0 = p[0], q1 = p[step];
  const int a = 3 * (q0 - p0);
  const int min_signed_val =
      LoopFilterFuncs_C<bitdepth, Pixel>::kMinSignedPixel;
  const int max_signed_val =
      LoopFilterFuncs_C<bitdepth, Pixel>::kMaxSignedPixel;
  const int a1 = Clip3(a + 4, min_signed_val, max_signed_val) >> 3;
  const int a2 = Clip3(a + 3, min_signed_val, max_signed_val) >> 3;
  const int a3 = (a1 + 1) >> 1;
  const int max_unsigned_val = LoopFilterFuncs_C<bitdepth, Pixel>::kMaxPixel;
  p[-2 * step] = Clip3(p1 + a3, 0, max_unsigned_val);
  p[-1 * step] = Clip3(p0 + a2, 0, max_unsigned_val);
  p[0 * step] = Clip3(q0 - a1, 0, max_unsigned_val);
  p[1 * step] = Clip3(q1 - a3, 0, max_unsigned_val);
}

template <int bitdepth, typename Pixel>
void LoopFilterFuncs_C<bitdepth, Pixel>::Vertical4(void* dest, ptrdiff_t stride,
                                                   int outer_thresh,
                                                   int inner_thresh,
                                                   int hev_thresh) {
  AdjustThresholds(bitdepth, &outer_thresh, &inner_thresh, &hev_thresh);
  auto* dst = static_cast<Pixel*>(dest);
  stride /= sizeof(Pixel);
  for (int i = 0; i < 4; ++i) {
    if (NeedsFilter4(dst, 1, outer_thresh, inner_thresh)) {
      if (Hev(dst, 1, hev_thresh)) {
        Filter2_C<bitdepth>(dst, 1);
      } else {
        Filter4_C<bitdepth>(dst, 1);
      }
    }
    dst += stride;
  }
}

template <int bitdepth, typename Pixel>
void LoopFilterFuncs_C<bitdepth, Pixel>::Horizontal4(void* dest,
                                                     ptrdiff_t stride,
                                                     int outer_thresh,
                                                     int inner_thresh,
                                                     int hev_thresh) {
  AdjustThresholds(bitdepth, &outer_thresh, &inner_thresh, &hev_thresh);
  auto* dst = static_cast<Pixel*>(dest);
  stride /= sizeof(Pixel);
  for (int i = 0; i < 4; ++i) {
    if (NeedsFilter4(dst, stride, outer_thresh, inner_thresh)) {
      if (Hev(dst, stride, hev_thresh)) {
        Filter2_C<bitdepth>(dst, stride);
      } else {
        Filter4_C<bitdepth>(dst, stride);
      }
    }
    ++dst;
  }
}

//------------------------------------------------------------------------------
// 5-tap (chroma) filters

// 7.14.6.2.
template <typename Pixel>
inline bool NeedsFilter6(const Pixel* p, ptrdiff_t step, int outer_thresh,
                         int inner_thresh) {
  const int p2 = p[-3 * step], p1 = p[-2 * step], p0 = p[-step];
  const int q0 = p[0], q1 = p[step], q2 = p[2 * step];
  return std::abs(p2 - p1) <= inner_thresh &&
         std::abs(p1 - p0) <= inner_thresh &&
         std::abs(q1 - q0) <= inner_thresh &&
         std::abs(q2 - q1) <= inner_thresh &&
         std::abs(p0 - q0) * 2 + std::abs(p1 - q1) / 2 <= outer_thresh;
}

// 7.14.6.2.
template <typename Pixel>
inline bool IsFlat3(const Pixel* p, ptrdiff_t step, int flat_thresh) {
  const int p2 = p[-3 * step], p1 = p[-2 * step], p0 = p[-step];
  const int q0 = p[0], q1 = p[step], q2 = p[2 * step];
  return std::abs(p1 - p0) <= flat_thresh && std::abs(q1 - q0) <= flat_thresh &&
         std::abs(p2 - p0) <= flat_thresh && std::abs(q2 - q0) <= flat_thresh;
}

template <typename Pixel>
inline Pixel ApplyFilter6(int filter_value) {
  return static_cast<Pixel>(RightShiftWithRounding(filter_value, 3));
}

// 7.14.6.4.
// 6 pixels in, 4 pixels out.
template <typename Pixel>
inline void Filter6_C(Pixel* p, ptrdiff_t step) {
  const int p2 = p[-3 * step], p1 = p[-2 * step], p0 = p[-step];
  const int q0 = p[0], q1 = p[step], q2 = p[2 * step];
  const int a1 = 2 * p1;
  const int a0 = 2 * p0;
  const int b0 = 2 * q0;
  const int b1 = 2 * q1;
  // The max is 8 * max_pixel + 4 for the rounder.
  // 8bpp: 2044 (11 bits), 10bpp: 8188 (13 bits), 12bpp: 32764 (15 bits)
  p[-2 * step] = ApplyFilter6<Pixel>(3 * p2 + a1 + a0 + q0);
  p[-1 * step] = ApplyFilter6<Pixel>(p2 + a1 + a0 + b0 + q1);
  p[0 * step] = ApplyFilter6<Pixel>(p1 + a0 + b0 + b1 + q2);
  p[1 * step] = ApplyFilter6<Pixel>(p0 + b0 + b1 + 3 * q2);
}

template <int bitdepth, typename Pixel>
void LoopFilterFuncs_C<bitdepth, Pixel>::Vertical6(void* dest, ptrdiff_t stride,
                                                   int outer_thresh,
                                                   int inner_thresh,
                                                   int hev_thresh) {
  const int flat_thresh = LoopFilterFuncs_C<bitdepth, Pixel>::kFlatThresh;
  AdjustThresholds(bitdepth, &outer_thresh, &inner_thresh, &hev_thresh);
  auto* dst = static_cast<Pixel*>(dest);
  stride /= sizeof(Pixel);
  for (int i = 0; i < 4; ++i) {
    if (NeedsFilter6(dst, 1, outer_thresh, inner_thresh)) {
      if (IsFlat3(dst, 1, flat_thresh)) {
        Filter6_C(dst, 1);
      } else if (Hev(dst, 1, hev_thresh)) {
        Filter2_C<bitdepth>(dst, 1);
      } else {
        Filter4_C<bitdepth>(dst, 1);
      }
    }
    dst += stride;
  }
}

template <int bitdepth, typename Pixel>
void LoopFilterFuncs_C<bitdepth, Pixel>::Horizontal6(void* dest,
                                                     ptrdiff_t stride,
                                                     int outer_thresh,
                                                     int inner_thresh,
                                                     int hev_thresh) {
  const int flat_thresh = LoopFilterFuncs_C<bitdepth, Pixel>::kFlatThresh;
  AdjustThresholds(bitdepth, &outer_thresh, &inner_thresh, &hev_thresh);
  auto* dst = static_cast<Pixel*>(dest);
  stride /= sizeof(Pixel);
  for (int i = 0; i < 4; ++i) {
    if (NeedsFilter6(dst, stride, outer_thresh, inner_thresh)) {
      if (IsFlat3(dst, stride, flat_thresh)) {
        Filter6_C(dst, stride);
      } else if (Hev(dst, stride, hev_thresh)) {
        Filter2_C<bitdepth>(dst, stride);
      } else {
        Filter4_C<bitdepth>(dst, stride);
      }
    }
    ++dst;
  }
}

//------------------------------------------------------------------------------
// 7-tap filters

// 7.14.6.2.
template <typename Pixel>
inline bool NeedsFilter8(const Pixel* p, ptrdiff_t step, int outer_thresh,
                         int inner_thresh) {
  const int p3 = p[-4 * step], p2 = p[-3 * step], p1 = p[-2 * step],
            p0 = p[-step];
  const int q0 = p[0], q1 = p[step], q2 = p[2 * step], q3 = p[3 * step];
  return std::abs(p3 - p2) <= inner_thresh &&
         std::abs(p2 - p1) <= inner_thresh &&
         std::abs(p1 - p0) <= inner_thresh &&
         std::abs(q1 - q0) <= inner_thresh &&
         std::abs(q2 - q1) <= inner_thresh &&
         std::abs(q3 - q2) <= inner_thresh &&
         std::abs(p0 - q0) * 2 + std::abs(p1 - q1) / 2 <= outer_thresh;
}

// 7.14.6.2.
template <typename Pixel>
inline bool IsFlat4(const Pixel* p, ptrdiff_t step, int flat_thresh) {
  const int p3 = p[-4 * step], p2 = p[-3 * step], p1 = p[-2 * step],
            p0 = p[-step];
  const int q0 = p[0], q1 = p[step], q2 = p[2 * step], q3 = p[3 * step];
  return std::abs(p1 - p0) <= flat_thresh && std::abs(q1 - q0) <= flat_thresh &&
         std::abs(p2 - p0) <= flat_thresh && std::abs(q2 - q0) <= flat_thresh &&
         std::abs(p3 - p0) <= flat_thresh && std::abs(q3 - q0) <= flat_thresh;
}

template <typename Pixel>
inline Pixel ApplyFilter8(int filter_value) {
  return static_cast<Pixel>(RightShiftWithRounding(filter_value, 3));
}

// 7.14.6.4.
// 8 pixels in, 6 pixels out.
template <typename Pixel>
inline void Filter8_C(Pixel* p, ptrdiff_t step) {
  const int p3 = p[-4 * step], p2 = p[-3 * step], p1 = p[-2 * step],
            p0 = p[-step];
  const int q0 = p[0], q1 = p[step], q2 = p[2 * step], q3 = p[3 * step];
  // The max is 8 * max_pixel + 4 for the rounder.
  // 8bpp: 2044 (11 bits), 10bpp: 8188 (13 bits), 12bpp: 32764 (15 bits)
  p[-3 * step] = ApplyFilter8<Pixel>(3 * p3 + 2 * p2 + p1 + p0 + q0);
  p[-2 * step] = ApplyFilter8<Pixel>(2 * p3 + p2 + 2 * p1 + p0 + q0 + q1);
  p[-1 * step] = ApplyFilter8<Pixel>(p3 + p2 + p1 + 2 * p0 + q0 + q1 + q2);
  p[0 * step] = ApplyFilter8<Pixel>(p2 + p1 + p0 + 2 * q0 + q1 + q2 + q3);
  p[1 * step] = ApplyFilter8<Pixel>(p1 + p0 + q0 + 2 * q1 + q2 + 2 * q3);
  p[2 * step] = ApplyFilter8<Pixel>(p0 + q0 + q1 + 2 * q2 + 3 * q3);
}

template <int bitdepth, typename Pixel>
void LoopFilterFuncs_C<bitdepth, Pixel>::Vertical8(void* dest, ptrdiff_t stride,
                                                   int outer_thresh,
                                                   int inner_thresh,
                                                   int hev_thresh) {
  const int flat_thresh = LoopFilterFuncs_C<bitdepth, Pixel>::kFlatThresh;
  AdjustThresholds(bitdepth, &outer_thresh, &inner_thresh, &hev_thresh);
  auto* dst = static_cast<Pixel*>(dest);
  stride /= sizeof(Pixel);
  for (int i = 0; i < 4; ++i) {
    if (NeedsFilter8(dst, 1, outer_thresh, inner_thresh)) {
      if (IsFlat4(dst, 1, flat_thresh)) {
        Filter8_C(dst, 1);
      } else if (Hev(dst, 1, hev_thresh)) {
        Filter2_C<bitdepth>(dst, 1);
      } else {
        Filter4_C<bitdepth>(dst, 1);
      }
    }
    dst += stride;
  }
}

template <int bitdepth, typename Pixel>
void LoopFilterFuncs_C<bitdepth, Pixel>::Horizontal8(void* dest,
                                                     ptrdiff_t stride,
                                                     int outer_thresh,
                                                     int inner_thresh,
                                                     int hev_thresh) {
  const int flat_thresh = LoopFilterFuncs_C<bitdepth, Pixel>::kFlatThresh;
  AdjustThresholds(bitdepth, &outer_thresh, &inner_thresh, &hev_thresh);
  auto* dst = static_cast<Pixel*>(dest);
  stride /= sizeof(Pixel);
  for (int i = 0; i < 4; ++i) {
    if (NeedsFilter8(dst, stride, outer_thresh, inner_thresh)) {
      if (IsFlat4(dst, stride, flat_thresh)) {
        Filter8_C(dst, stride);
      } else if (Hev(dst, stride, hev_thresh)) {
        Filter2_C<bitdepth>(dst, stride);
      } else {
        Filter4_C<bitdepth>(dst, stride);
      }
    }
    ++dst;
  }
}

//------------------------------------------------------------------------------
// 13-tap filters

// 7.14.6.2.
template <typename Pixel>
inline bool IsFlatOuter4(const Pixel* p, ptrdiff_t step, int flat_thresh) {
  const int p6 = p[-7 * step], p5 = p[-6 * step], p4 = p[-5 * step],
            p0 = p[-step];
  const int q0 = p[0], q4 = p[4 * step], q5 = p[5 * step], q6 = p[6 * step];
  return std::abs(p4 - p0) <= flat_thresh && std::abs(q4 - q0) <= flat_thresh &&
         std::abs(p5 - p0) <= flat_thresh && std::abs(q5 - q0) <= flat_thresh &&
         std::abs(p6 - p0) <= flat_thresh && std::abs(q6 - q0) <= flat_thresh;
}

template <typename Pixel>
inline Pixel ApplyFilter14(int filter_value) {
  return static_cast<Pixel>(RightShiftWithRounding(filter_value, 4));
}

// 7.14.6.4.
// 14 pixels in, 12 pixels out.
template <typename Pixel>
inline void Filter14_C(Pixel* p, ptrdiff_t step) {
  const int p6 = p[-7 * step], p5 = p[-6 * step], p4 = p[-5 * step],
            p3 = p[-4 * step], p2 = p[-3 * step], p1 = p[-2 * step],
            p0 = p[-step];
  const int q0 = p[0], q1 = p[step], q2 = p[2 * step], q3 = p[3 * step],
            q4 = p[4 * step], q5 = p[5 * step], q6 = p[6 * step];
  // The max is 16 * max_pixel + 8 for the rounder.
  // 8bpp: 4088 (12 bits), 10bpp: 16376 (14 bits), 12bpp: 65528 (16 bits)
  p[-6 * step] =
      ApplyFilter14<Pixel>(p6 * 7 + p5 * 2 + p4 * 2 + p3 + p2 + p1 + p0 + q0);
  p[-5 * step] = ApplyFilter14<Pixel>(p6 * 5 + p5 * 2 + p4 * 2 + p3 * 2 + p2 +
                                      p1 + p0 + q0 + q1);
  p[-4 * step] = ApplyFilter14<Pixel>(p6 * 4 + p5 + p4 * 2 + p3 * 2 + p2 * 2 +
                                      p1 + p0 + q0 + q1 + q2);
  p[-3 * step] = ApplyFilter14<Pixel>(p6 * 3 + p5 + p4 + p3 * 2 + p2 * 2 +
                                      p1 * 2 + p0 + q0 + q1 + q2 + q3);
  p[-2 * step] = ApplyFilter14<Pixel>(p6 * 2 + p5 + p4 + p3 + p2 * 2 + p1 * 2 +
                                      p0 * 2 + q0 + q1 + q2 + q3 + q4);
  p[-1 * step] = ApplyFilter14<Pixel>(p6 + p5 + p4 + p3 + p2 + p1 * 2 + p0 * 2 +
                                      q0 * 2 + q1 + q2 + q3 + q4 + q5);
  p[0 * step] = ApplyFilter14<Pixel>(p5 + p4 + p3 + p2 + p1 + p0 * 2 + q0 * 2 +
                                     q1 * 2 + q2 + q3 + q4 + q5 + q6);
  p[1 * step] = ApplyFilter14<Pixel>(p4 + p3 + p2 + p1 + p0 + q0 * 2 + q1 * 2 +
                                     q2 * 2 + q3 + q4 + q5 + q6 * 2);
  p[2 * step] = ApplyFilter14<Pixel>(p3 + p2 + p1 + p0 + q0 + q1 * 2 + q2 * 2 +
                                     q3 * 2 + q4 + q5 + q6 * 3);
  p[3 * step] = ApplyFilter14<Pixel>(p2 + p1 + p0 + q0 + q1 + q2 * 2 + q3 * 2 +
                                     q4 * 2 + q5 + q6 * 4);
  p[4 * step] = ApplyFilter14<Pixel>(p1 + p0 + q0 + q1 + q2 + q3 * 2 + q4 * 2 +
                                     q5 * 2 + q6 * 5);
  p[5 * step] =
      ApplyFilter14<Pixel>(p0 + q0 + q1 + q2 + q3 + q4 * 2 + q5 * 2 + q6 * 7);
}

template <int bitdepth, typename Pixel>
void LoopFilterFuncs_C<bitdepth, Pixel>::Vertical14(void* dest,
                                                    ptrdiff_t stride,
                                                    int outer_thresh,
                                                    int inner_thresh,
                                                    int hev_thresh) {
  const int flat_thresh = LoopFilterFuncs_C<bitdepth, Pixel>::kFlatThresh;
  AdjustThresholds(bitdepth, &outer_thresh, &inner_thresh, &hev_thresh);
  auto* dst = static_cast<Pixel*>(dest);
  stride /= sizeof(Pixel);
  for (int i = 0; i < 4; ++i) {
    if (NeedsFilter8(dst, 1, outer_thresh, inner_thresh)) {
      if (IsFlat4(dst, 1, flat_thresh)) {
        if (IsFlatOuter4(dst, 1, flat_thresh)) {
          Filter14_C(dst, 1);
        } else {
          Filter8_C(dst, 1);
        }
      } else if (Hev(dst, 1, hev_thresh)) {
        Filter2_C<bitdepth>(dst, 1);
      } else {
        Filter4_C<bitdepth>(dst, 1);
      }
    }
    dst += stride;
  }
}

template <int bitdepth, typename Pixel>
void LoopFilterFuncs_C<bitdepth, Pixel>::Horizontal14(void* dest,
                                                      ptrdiff_t stride,
                                                      int outer_thresh,
                                                      int inner_thresh,
                                                      int hev_thresh) {
  const int flat_thresh = LoopFilterFuncs_C<bitdepth, Pixel>::kFlatThresh;
  AdjustThresholds(bitdepth, &outer_thresh, &inner_thresh, &hev_thresh);
  auto* dst = static_cast<Pixel*>(dest);
  stride /= sizeof(Pixel);
  for (int i = 0; i < 4; ++i) {
    if (NeedsFilter8(dst, stride, outer_thresh, inner_thresh)) {
      if (IsFlat4(dst, stride, flat_thresh)) {
        if (IsFlatOuter4(dst, stride, flat_thresh)) {
          Filter14_C(dst, stride);
        } else {
          Filter8_C(dst, stride);
        }
      } else if (Hev(dst, stride, hev_thresh)) {
        Filter2_C<bitdepth>(dst, stride);
      } else {
        Filter4_C<bitdepth>(dst, stride);
      }
    }
    ++dst;
  }
}

using Defs8bpp = LoopFilterFuncs_C<8, uint8_t>;

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(8);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->loop_filters[kLoopFilterSize4][kLoopFilterTypeHorizontal] =
      Defs8bpp::Horizontal4;
  dsp->loop_filters[kLoopFilterSize4][kLoopFilterTypeVertical] =
      Defs8bpp::Vertical4;

  dsp->loop_filters[kLoopFilterSize6][kLoopFilterTypeHorizontal] =
      Defs8bpp::Horizontal6;
  dsp->loop_filters[kLoopFilterSize6][kLoopFilterTypeVertical] =
      Defs8bpp::Vertical6;

  dsp->loop_filters[kLoopFilterSize8][kLoopFilterTypeHorizontal] =
      Defs8bpp::Horizontal8;
  dsp->loop_filters[kLoopFilterSize8][kLoopFilterTypeVertical] =
      Defs8bpp::Vertical8;

  dsp->loop_filters[kLoopFilterSize14][kLoopFilterTypeHorizontal] =
      Defs8bpp::Horizontal14;
  dsp->loop_filters[kLoopFilterSize14][kLoopFilterTypeVertical] =
      Defs8bpp::Vertical14;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp8bpp_LoopFilterSize4_LoopFilterTypeHorizontal
  dsp->loop_filters[kLoopFilterSize4][kLoopFilterTypeHorizontal] =
      Defs8bpp::Horizontal4;
#endif
#ifndef LIBGAV1_Dsp8bpp_LoopFilterSize4_LoopFilterTypeVertical
  dsp->loop_filters[kLoopFilterSize4][kLoopFilterTypeVertical] =
      Defs8bpp::Vertical4;
#endif

#ifndef LIBGAV1_Dsp8bpp_LoopFilterSize6_LoopFilterTypeHorizontal
  dsp->loop_filters[kLoopFilterSize6][kLoopFilterTypeHorizontal] =
      Defs8bpp::Horizontal6;
#endif
#ifndef LIBGAV1_Dsp8bpp_LoopFilterSize6_LoopFilterTypeVertical
  dsp->loop_filters[kLoopFilterSize6][kLoopFilterTypeVertical] =
      Defs8bpp::Vertical6;
#endif

#ifndef LIBGAV1_Dsp8bpp_LoopFilterSize8_LoopFilterTypeHorizontal
  dsp->loop_filters[kLoopFilterSize8][kLoopFilterTypeHorizontal] =
      Defs8bpp::Horizontal8;
#endif
#ifndef LIBGAV1_Dsp8bpp_LoopFilterSize8_LoopFilterTypeVertical
  dsp->loop_filters[kLoopFilterSize8][kLoopFilterTypeVertical] =
      Defs8bpp::Vertical8;
#endif

#ifndef LIBGAV1_Dsp8bpp_LoopFilterSize14_LoopFilterTypeHorizontal
  dsp->loop_filters[kLoopFilterSize14][kLoopFilterTypeHorizontal] =
      Defs8bpp::Horizontal14;
#endif
#ifndef LIBGAV1_Dsp8bpp_LoopFilterSize14_LoopFilterTypeVertical
  dsp->loop_filters[kLoopFilterSize14][kLoopFilterTypeVertical] =
      Defs8bpp::Vertical14;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}

#if LIBGAV1_MAX_BITDEPTH >= 10
using Defs10bpp = LoopFilterFuncs_C<10, uint16_t>;

void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(10);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->loop_filters[kLoopFilterSize4][kLoopFilterTypeHorizontal] =
      Defs10bpp::Horizontal4;
  dsp->loop_filters[kLoopFilterSize4][kLoopFilterTypeVertical] =
      Defs10bpp::Vertical4;

  dsp->loop_filters[kLoopFilterSize6][kLoopFilterTypeHorizontal] =
      Defs10bpp::Horizontal6;
  dsp->loop_filters[kLoopFilterSize6][kLoopFilterTypeVertical] =
      Defs10bpp::Vertical6;

  dsp->loop_filters[kLoopFilterSize8][kLoopFilterTypeHorizontal] =
      Defs10bpp::Horizontal8;
  dsp->loop_filters[kLoopFilterSize8][kLoopFilterTypeVertical] =
      Defs10bpp::Vertical8;

  dsp->loop_filters[kLoopFilterSize14][kLoopFilterTypeHorizontal] =
      Defs10bpp::Horizontal14;
  dsp->loop_filters[kLoopFilterSize14][kLoopFilterTypeVertical] =
      Defs10bpp::Vertical14;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp10bpp_LoopFilterSize4_LoopFilterTypeHorizontal
  dsp->loop_filters[kLoopFilterSize4][kLoopFilterTypeHorizontal] =
      Defs10bpp::Horizontal4;
#endif
#ifndef LIBGAV1_Dsp10bpp_LoopFilterSize4_LoopFilterTypeVertical
  dsp->loop_filters[kLoopFilterSize4][kLoopFilterTypeVertical] =
      Defs10bpp::Vertical4;
#endif

#ifndef LIBGAV1_Dsp10bpp_LoopFilterSize6_LoopFilterTypeHorizontal
  dsp->loop_filters[kLoopFilterSize6][kLoopFilterTypeHorizontal] =
      Defs10bpp::Horizontal6;
#endif
#ifndef LIBGAV1_Dsp10bpp_LoopFilterSize6_LoopFilterTypeVertical
  dsp->loop_filters[kLoopFilterSize6][kLoopFilterTypeVertical] =
      Defs10bpp::Vertical6;
#endif

#ifndef LIBGAV1_Dsp10bpp_LoopFilterSize8_LoopFilterTypeHorizontal
  dsp->loop_filters[kLoopFilterSize8][kLoopFilterTypeHorizontal] =
      Defs10bpp::Horizontal8;
#endif
#ifndef LIBGAV1_Dsp10bpp_LoopFilterSize8_LoopFilterTypeVertical
  dsp->loop_filters[kLoopFilterSize8][kLoopFilterTypeVertical] =
      Defs10bpp::Vertical8;
#endif

#ifndef LIBGAV1_Dsp10bpp_LoopFilterSize14_LoopFilterTypeHorizontal
  dsp->loop_filters[kLoopFilterSize14][kLoopFilterTypeHorizontal] =
      Defs10bpp::Horizontal14;
#endif
#ifndef LIBGAV1_Dsp10bpp_LoopFilterSize14_LoopFilterTypeVertical
  dsp->loop_filters[kLoopFilterSize14][kLoopFilterTypeVertical] =
      Defs10bpp::Vertical14;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using Defs12bpp = LoopFilterFuncs_C<12, uint16_t>;

void Init12bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(12);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->loop_filters[kLoopFilterSize4][kLoopFilterTypeHorizontal] =
      Defs12bpp::Horizontal4;
  dsp->loop_filters[kLoopFilterSize4][kLoopFilterTypeVertical] =
      Defs12bpp::Vertical4;

  dsp->loop_filters[kLoopFilterSize6][kLoopFilterTypeHorizontal] =
      Defs12bpp::Horizontal6;
  dsp->loop_filters[kLoopFilterSize6][kLoopFilterTypeVertical] =
      Defs12bpp::Vertical6;

  dsp->loop_filters[kLoopFilterSize8][kLoopFilterTypeHorizontal] =
      Defs12bpp::Horizontal8;
  dsp->loop_filters[kLoopFilterSize8][kLoopFilterTypeVertical] =
      Defs12bpp::Vertical8;

  dsp->loop_filters[kLoopFilterSize14][kLoopFilterTypeHorizontal] =
      Defs12bpp::Horizontal14;
  dsp->loop_filters[kLoopFilterSize14][kLoopFilterTypeVertical] =
      Defs12bpp::Vertical14;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp12bpp_LoopFilterSize4_LoopFilterTypeHorizontal
  dsp->loop_filters[kLoopFilterSize4][kLoopFilterTypeHorizontal] =
      Defs12bpp::Horizontal4;
#endif
#ifndef LIBGAV1_Dsp12bpp_LoopFilterSize4_LoopFilterTypeVertical
  dsp->loop_filters[kLoopFilterSize4][kLoopFilterTypeVertical] =
      Defs12bpp::Vertical4;
#endif

#ifndef LIBGAV1_Dsp12bpp_LoopFilterSize6_LoopFilterTypeHorizontal
  dsp->loop_filters[kLoopFilterSize6][kLoopFilterTypeHorizontal] =
      Defs12bpp::Horizontal6;
#endif
#ifndef LIBGAV1_Dsp12bpp_LoopFilterSize6_LoopFilterTypeVertical
  dsp->loop_filters[kLoopFilterSize6][kLoopFilterTypeVertical] =
      Defs12bpp::Vertical6;
#endif

#ifndef LIBGAV1_Dsp12bpp_LoopFilterSize8_LoopFilterTypeHorizontal
  dsp->loop_filters[kLoopFilterSize8][kLoopFilterTypeHorizontal] =
      Defs12bpp::Horizontal8;
#endif
#ifndef LIBGAV1_Dsp12bpp_LoopFilterSize8_LoopFilterTypeVertical
  dsp->loop_filters[kLoopFilterSize8][kLoopFilterTypeVertical] =
      Defs12bpp::Vertical8;
#endif

#ifndef LIBGAV1_Dsp12bpp_LoopFilterSize14_LoopFilterTypeHorizontal
  dsp->loop_filters[kLoopFilterSize14][kLoopFilterTypeHorizontal] =
      Defs12bpp::Horizontal14;
#endif
#ifndef LIBGAV1_Dsp12bpp_LoopFilterSize14_LoopFilterTypeVertical
  dsp->loop_filters[kLoopFilterSize14][kLoopFilterTypeVertical] =
      Defs12bpp::Vertical14;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace

void LoopFilterInit_C() {
  Init8bpp();
#if LIBGAV1_MAX_BITDEPTH >= 10
  Init10bpp();
#endif
#if LIBGAV1_MAX_BITDEPTH == 12
  Init12bpp();
#endif
  // Local functions that may be unused depending on the optimizations
  // available.
  static_cast<void>(AdjustThresholds);
}

}  // namespace dsp
}  // namespace libgav1
