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

#include "src/warp_prediction.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>

#include "src/tile.h"
#include "src/utils/block_parameters_holder.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/logging.h"

namespace libgav1 {
namespace {

constexpr int kWarpModelTranslationClamp = 1 << 23;
constexpr int kWarpModelAffineClamp = 1 << 13;
constexpr int kLargestMotionVectorDiff = 256;

constexpr uint16_t kDivisorLookup[257] = {
    16384, 16320, 16257, 16194, 16132, 16070, 16009, 15948, 15888, 15828, 15768,
    15709, 15650, 15592, 15534, 15477, 15420, 15364, 15308, 15252, 15197, 15142,
    15087, 15033, 14980, 14926, 14873, 14821, 14769, 14717, 14665, 14614, 14564,
    14513, 14463, 14413, 14364, 14315, 14266, 14218, 14170, 14122, 14075, 14028,
    13981, 13935, 13888, 13843, 13797, 13752, 13707, 13662, 13618, 13574, 13530,
    13487, 13443, 13400, 13358, 13315, 13273, 13231, 13190, 13148, 13107, 13066,
    13026, 12985, 12945, 12906, 12866, 12827, 12788, 12749, 12710, 12672, 12633,
    12596, 12558, 12520, 12483, 12446, 12409, 12373, 12336, 12300, 12264, 12228,
    12193, 12157, 12122, 12087, 12053, 12018, 11984, 11950, 11916, 11882, 11848,
    11815, 11782, 11749, 11716, 11683, 11651, 11619, 11586, 11555, 11523, 11491,
    11460, 11429, 11398, 11367, 11336, 11305, 11275, 11245, 11215, 11185, 11155,
    11125, 11096, 11067, 11038, 11009, 10980, 10951, 10923, 10894, 10866, 10838,
    10810, 10782, 10755, 10727, 10700, 10673, 10645, 10618, 10592, 10565, 10538,
    10512, 10486, 10460, 10434, 10408, 10382, 10356, 10331, 10305, 10280, 10255,
    10230, 10205, 10180, 10156, 10131, 10107, 10082, 10058, 10034, 10010, 9986,
    9963,  9939,  9916,  9892,  9869,  9846,  9823,  9800,  9777,  9754,  9732,
    9709,  9687,  9664,  9642,  9620,  9598,  9576,  9554,  9533,  9511,  9489,
    9468,  9447,  9425,  9404,  9383,  9362,  9341,  9321,  9300,  9279,  9259,
    9239,  9218,  9198,  9178,  9158,  9138,  9118,  9098,  9079,  9059,  9039,
    9020,  9001,  8981,  8962,  8943,  8924,  8905,  8886,  8867,  8849,  8830,
    8812,  8793,  8775,  8756,  8738,  8720,  8702,  8684,  8666,  8648,  8630,
    8613,  8595,  8577,  8560,  8542,  8525,  8508,  8490,  8473,  8456,  8439,
    8422,  8405,  8389,  8372,  8355,  8339,  8322,  8306,  8289,  8273,  8257,
    8240,  8224,  8208,  8192};

// Number of fractional bits of lookup in divisor lookup table.
constexpr int kDivisorLookupBits = 8;
// Number of fractional bits of entries in divisor lookup table.
constexpr int kDivisorLookupPrecisionBits = 14;

// 7.11.3.7.
template <typename T>
void GenerateApproximateDivisor(T value, int16_t* division_factor,
                                int16_t* division_shift) {
  const int n = FloorLog2(std::abs(value));
  const T e = std::abs(value) - (static_cast<T>(1) << n);
  const int entry = (n > kDivisorLookupBits)
                        ? RightShiftWithRounding(e, n - kDivisorLookupBits)
                        : static_cast<int>(e << (kDivisorLookupBits - n));
  *division_shift = n + kDivisorLookupPrecisionBits;
  *division_factor =
      (value < 0) ? -kDivisorLookup[entry] : kDivisorLookup[entry];
}

// 7.11.3.8.
int LeastSquareProduct(int a, int b) { return ((a * b) >> 2) + a + b; }

// 7.11.3.8.
int DiagonalClamp(int32_t value) {
  return Clip3(value,
               (1 << kWarpedModelPrecisionBits) - kWarpModelAffineClamp + 1,
               (1 << kWarpedModelPrecisionBits) + kWarpModelAffineClamp - 1);
}

// 7.11.3.8.
int NonDiagonalClamp(int32_t value) {
  return Clip3(value, -kWarpModelAffineClamp + 1, kWarpModelAffineClamp - 1);
}

int16_t GetShearParameter(int value) {
  return static_cast<int16_t>(
      LeftShift(RightShiftWithRoundingSigned(Clip3(value, INT16_MIN, INT16_MAX),
                                             kWarpParamRoundingBits),
                kWarpParamRoundingBits));
}

}  // namespace

bool SetupShear(GlobalMotion* const warp_params) {
  int16_t division_shift;
  int16_t division_factor;
  const auto* const params = warp_params->params;
  GenerateApproximateDivisor<int32_t>(params[2], &division_factor,
                                      &division_shift);
  const int alpha = params[2] - (1 << kWarpedModelPrecisionBits);
  const int beta = params[3];
  const int64_t v = LeftShift(params[4], kWarpedModelPrecisionBits);
  const int gamma =
      RightShiftWithRoundingSigned(v * division_factor, division_shift);
  const int64_t w = static_cast<int64_t>(params[3]) * params[4];
  const int delta =
      params[5] -
      RightShiftWithRoundingSigned(w * division_factor, division_shift) -
      (1 << kWarpedModelPrecisionBits);

  warp_params->alpha = GetShearParameter(alpha);
  warp_params->beta = GetShearParameter(beta);
  warp_params->gamma = GetShearParameter(gamma);
  warp_params->delta = GetShearParameter(delta);
  if ((4 * std::abs(warp_params->alpha) + 7 * std::abs(warp_params->beta) >=
       (1 << kWarpedModelPrecisionBits)) ||
      (4 * std::abs(warp_params->gamma) + 4 * std::abs(warp_params->delta) >=
       (1 << kWarpedModelPrecisionBits))) {
    return false;  // NOLINT (easier condition to understand).
  }

  return true;
}

bool WarpEstimation(const int num_samples, const int block_width4x4,
                    const int block_height4x4, const int row4x4,
                    const int column4x4, const MotionVector& mv,
                    const int candidates[kMaxLeastSquaresSamples][4],
                    GlobalMotion* const warp_params) {
  // |a| fits into int32_t. To avoid cast to int64_t in the following
  // computation, we declare |a| as int64_t.
  int64_t a[2][2] = {};
  int bx[2] = {};
  int by[2] = {};

  // Note: for simplicity, the spec always uses absolute coordinates
  // in the warp estimation process. subpixel_mid_x, subpixel_mid_y,
  // and candidates are relative to the top left of the frame.
  // In contrast, libaom uses a mixture of coordinate systems.
  // In av1/common/warped_motion.c:find_affine_int(). The coordinate is relative
  // to the top left of the block.
  // mid_y/mid_x: the row/column coordinate of the center of the block.
  const int mid_y = MultiplyBy4(row4x4) + MultiplyBy2(block_height4x4) - 1;
  const int mid_x = MultiplyBy4(column4x4) + MultiplyBy2(block_width4x4) - 1;
  const int subpixel_mid_y = MultiplyBy8(mid_y);
  const int subpixel_mid_x = MultiplyBy8(mid_x);
  const int reference_subpixel_mid_y = subpixel_mid_y + mv.mv[0];
  const int reference_subpixel_mid_x = subpixel_mid_x + mv.mv[1];

  for (int i = 0; i < num_samples; ++i) {
    // candidates[][0] and candidates[][1] are the row/column coordinates of the
    // sample point in this block, to the top left of the frame.
    // candidates[][2] and candidates[][3] are the row/column coordinates of the
    // sample point in this reference block, to the top left of the frame.
    // sy/sx: the row/column coordinates of the sample point, with center of
    // the block as origin.
    const int sy = candidates[i][0] - subpixel_mid_y;
    const int sx = candidates[i][1] - subpixel_mid_x;
    // dy/dx: the row/column coordinates of the sample point in the reference
    // block, with center of the reference block as origin.
    const int dy = candidates[i][2] - reference_subpixel_mid_y;
    const int dx = candidates[i][3] - reference_subpixel_mid_x;
    if (std::abs(sx - dx) < kLargestMotionVectorDiff &&
        std::abs(sy - dy) < kLargestMotionVectorDiff) {
      a[0][0] += LeastSquareProduct(sx, sx) + 8;
      a[0][1] += LeastSquareProduct(sx, sy) + 4;
      a[1][1] += LeastSquareProduct(sy, sy) + 8;
      bx[0] += LeastSquareProduct(sx, dx) + 8;
      bx[1] += LeastSquareProduct(sy, dx) + 4;
      by[0] += LeastSquareProduct(sx, dy) + 4;
      by[1] += LeastSquareProduct(sy, dy) + 8;
    }
  }

  // a[0][1] == a[1][0], because the matrix is symmetric. We don't have to
  // compute a[1][0].
  const int64_t determinant = a[0][0] * a[1][1] - a[0][1] * a[0][1];
  if (determinant == 0) return false;

  int16_t division_shift;
  int16_t division_factor;
  GenerateApproximateDivisor<int64_t>(determinant, &division_factor,
                                      &division_shift);

  division_shift -= kWarpedModelPrecisionBits;

  const int64_t params_2 = a[1][1] * bx[0] - a[0][1] * bx[1];
  const int64_t params_3 = -a[0][1] * bx[0] + a[0][0] * bx[1];
  const int64_t params_4 = a[1][1] * by[0] - a[0][1] * by[1];
  const int64_t params_5 = -a[0][1] * by[0] + a[0][0] * by[1];
  auto* const params = warp_params->params;

  if (division_shift <= 0) {
    division_factor <<= -division_shift;
    params[2] = static_cast<int32_t>(params_2) * division_factor;
    params[3] = static_cast<int32_t>(params_3) * division_factor;
    params[4] = static_cast<int32_t>(params_4) * division_factor;
    params[5] = static_cast<int32_t>(params_5) * division_factor;
  } else {
    params[2] = RightShiftWithRoundingSigned(params_2 * division_factor,
                                             division_shift);
    params[3] = RightShiftWithRoundingSigned(params_3 * division_factor,
                                             division_shift);
    params[4] = RightShiftWithRoundingSigned(params_4 * division_factor,
                                             division_shift);
    params[5] = RightShiftWithRoundingSigned(params_5 * division_factor,
                                             division_shift);
  }

  params[2] = DiagonalClamp(params[2]);
  params[3] = NonDiagonalClamp(params[3]);
  params[4] = NonDiagonalClamp(params[4]);
  params[5] = DiagonalClamp(params[5]);

  const int vx = mv.mv[1] * (1 << (kWarpedModelPrecisionBits - 3)) -
                 (mid_x * (params[2] - (1 << kWarpedModelPrecisionBits)) +
                  mid_y * params[3]);
  const int vy = mv.mv[0] * (1 << (kWarpedModelPrecisionBits - 3)) -
                 (mid_x * params[4] +
                  mid_y * (params[5] - (1 << kWarpedModelPrecisionBits)));
  params[0] =
      Clip3(vx, -kWarpModelTranslationClamp, kWarpModelTranslationClamp - 1);
  params[1] =
      Clip3(vy, -kWarpModelTranslationClamp, kWarpModelTranslationClamp - 1);
  return true;
}

}  // namespace libgav1
