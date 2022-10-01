// Copyright 2021 The libgav1 Authors
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

#include "src/dsp/film_grain.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <tuple>
#include <type_traits>

#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "gtest/gtest.h"
#include "src/dsp/common.h"
#include "src/dsp/dsp.h"
#include "src/dsp/film_grain_common.h"
#include "src/film_grain.h"
#include "src/utils/array_2d.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/cpu.h"
#include "src/utils/memory.h"
#include "src/utils/threadpool.h"
#include "src/utils/types.h"
#include "tests/block_utils.h"
#include "tests/third_party/libvpx/acm_random.h"
#include "tests/utils.h"

namespace libgav1 {
namespace dsp {
namespace film_grain {
namespace {

constexpr int kNumSpeedTests = 50;
constexpr int kNumFilmGrainTestParams = 10;
constexpr size_t kLumaBlockSize = kLumaWidth * kLumaHeight;
constexpr size_t kChromaBlockSize = kMaxChromaWidth * kMaxChromaHeight;
// Dimensions for unit tests concerning applying grain to the whole frame.
constexpr size_t kNumTestStripes = 64;
constexpr int kNoiseStripeHeight = 34;
constexpr size_t kFrameWidth = 1921;
constexpr size_t kFrameHeight = (kNumTestStripes - 1) * 32 + 1;

/*
  The film grain parameters for 10 frames were generated with the following
  command line:
  aomenc --end-usage=q --cq-level=20 --cpu-used=8 -w 1920 -h 1080 \
    --denoise-noise-level=50 --ivf breaking_bad_21m23s_10frames.1920_1080.yuv \
    -o breaking_bad_21m23s_10frames.1920_1080.noise50.ivf
*/
constexpr FilmGrainParams kFilmGrainParams[10] = {
    {/*apply_grain=*/true,
     /*update_grain=*/true,
     /*chroma_scaling_from_luma=*/false,
     /*overlap_flag=*/true,
     /*clip_to_restricted_range=*/false,
     /*num_y_points=*/7,
     /*num_u_points=*/8,
     /*num_v_points=*/8,
     /*point_y_value=*/{0, 13, 27, 40, 54, 121, 255, 0, 0, 0, 0, 0, 0, 0},
     /*point_y_scaling=*/{71, 71, 91, 99, 98, 100, 100, 0, 0, 0, 0, 0, 0, 0},
     /*point_u_value=*/{0, 13, 27, 40, 54, 67, 94, 255, 0, 0},
     /*point_u_scaling=*/{37, 37, 43, 48, 48, 50, 51, 51, 0, 0},
     /*point_v_value=*/{0, 13, 27, 40, 54, 67, 107, 255, 0, 0},
     /*point_v_scaling=*/{48, 48, 43, 33, 32, 33, 34, 34, 0, 0},
     /*chroma_scaling=*/11,
     /*auto_regression_coeff_lag=*/3,
     /*auto_regression_coeff_y=*/{2,   -2,  -2,  10,  3, -2, 1,   -4,
                                  5,   -1,  -25, -13, 3, -1, 0,   7,
                                  -20, 103, 26,  -2,  1, 14, -49, 117},
     /*auto_regression_coeff_u=*/{-2,  1,  -3, 4,   -4, 0,  3,   5,  -5,
                                  -17, 17, 0,  -10, -5, -3, -30, 14, 70,
                                  29,  9,  -2, -10, 50, 71, -11},
     /*auto_regression_coeff_v=*/{3,   -2, -7, 6,   -7, -8, 3,   1,  -12,
                                  -15, 28, 5,  -11, -2, -7, -27, 32, 62,
                                  31,  18, -2, -6,  61, 43, 2},
     /*auto_regression_shift=*/8,
     /*grain_seed=*/7391,
     /*reference_index=*/0,
     /*grain_scale_shift=*/0,
     /*u_multiplier=*/0,
     /*u_luma_multiplier=*/64,
     /*u_offset=*/0,
     /*v_multiplier=*/0,
     /*v_luma_multiplier=*/64,
     /*v_offset=*/0},
    {/*apply_grain=*/true,
     /*update_grain=*/true,
     /*chroma_scaling_from_luma=*/false,
     /*overlap_flag=*/true,
     /*clip_to_restricted_range=*/false,
     /*num_y_points=*/8,
     /*num_u_points=*/7,
     /*num_v_points=*/8,
     /*point_y_value=*/{0, 13, 27, 40, 54, 94, 134, 255, 0, 0, 0, 0, 0, 0},
     /*point_y_scaling=*/{72, 72, 91, 99, 97, 100, 102, 102, 0, 0, 0, 0, 0, 0},
     /*point_u_value=*/{0, 13, 40, 54, 67, 134, 255, 0, 0, 0},
     /*point_u_scaling=*/{38, 38, 50, 49, 51, 53, 53, 0, 0, 0},
     /*point_v_value=*/{0, 13, 27, 40, 54, 67, 121, 255, 0, 0},
     /*point_v_scaling=*/{50, 50, 45, 34, 33, 35, 37, 37, 0, 0},
     /*chroma_scaling=*/11,
     /*auto_regression_coeff_lag=*/3,
     /*auto_regression_coeff_y=*/{2,   -2,  -2,  10,  3,  -1, 1,   -3,
                                  3,   1,   -27, -12, 2,  -1, 1,   7,
                                  -17, 100, 27,  0,   -1, 13, -50, 116},
     /*auto_regression_coeff_u=*/{-3,  1,  -2, 3,   -3, -1, 2,   5,  -3,
                                  -16, 16, -2, -10, -2, -1, -31, 14, 70,
                                  29,  9,  -1, -10, 47, 70, -11},
     /*auto_regression_coeff_v=*/{1,   0,  -5, 5,   -6, -6, 2,   1,  -10,
                                  -14, 26, 4,  -10, -3, -5, -26, 29, 63,
                                  31,  17, -1, -6,  55, 47, 2},
     /*auto_regression_shift=*/8,
     /*grain_seed=*/10772,
     /*reference_index=*/0,
     /*grain_scale_shift=*/0,
     /*u_multiplier=*/0,
     /*u_luma_multiplier=*/64,
     /*u_offset=*/0,
     /*v_multiplier=*/0,
     /*v_luma_multiplier=*/64,
     /*v_offset=*/0},
    {/*apply_grain=*/true,
     /*update_grain=*/true,
     /*chroma_scaling_from_luma=*/false,
     /*overlap_flag=*/true,
     /*clip_to_restricted_range=*/false,
     /*num_y_points=*/8,
     /*num_u_points=*/7,
     /*num_v_points=*/8,
     /*point_y_value=*/{0, 13, 27, 40, 54, 94, 134, 255, 0, 0, 0, 0, 0, 0},
     /*point_y_scaling=*/{71, 71, 91, 99, 98, 101, 103, 103, 0, 0, 0, 0, 0, 0},
     /*point_u_value=*/{0, 13, 40, 54, 81, 107, 255, 0, 0, 0},
     /*point_u_scaling=*/{37, 37, 49, 48, 51, 52, 52, 0, 0, 0},
     /*point_v_value=*/{0, 13, 27, 40, 54, 67, 121, 255, 0, 0},
     /*point_v_scaling=*/{49, 49, 44, 34, 32, 34, 36, 36, 0, 0},
     /*chroma_scaling=*/11,
     /*auto_regression_coeff_lag=*/3,
     /*auto_regression_coeff_y=*/{1,   -2,  -2,  10,  3, -1, 1,   -4,
                                  4,   1,   -26, -12, 2, -1, 1,   7,
                                  -18, 101, 26,  -1,  0, 13, -49, 116},
     /*auto_regression_coeff_u=*/{-3,  1,  -3, 4,   -3, -1, 2,   5,  -4,
                                  -16, 17, -2, -10, -3, -2, -31, 15, 70,
                                  28,  9,  -1, -10, 48, 70, -11},
     /*auto_regression_coeff_v=*/{1,   -1, -6, 5,   -6, -7, 2,   2,  -11,
                                  -14, 27, 5,  -11, -3, -6, -26, 30, 62,
                                  30,  18, -2, -6,  58, 45, 2},
     /*auto_regression_shift=*/8,
     /*grain_seed=*/14153,
     /*reference_index=*/0,
     /*grain_scale_shift=*/0,
     /*u_multiplier=*/0,
     /*u_luma_multiplier=*/64,
     /*u_offset=*/0,
     /*v_multiplier=*/0,
     /*v_luma_multiplier=*/64,
     /*v_offset=*/0},
    {/*apply_grain=*/true,
     /*update_grain=*/true,
     /*chroma_scaling_from_luma=*/false,
     /*overlap_flag=*/true,
     /*clip_to_restricted_range=*/false,
     /*num_y_points=*/7,
     /*num_u_points=*/5,
     /*num_v_points=*/7,
     /*point_y_value=*/{0, 13, 27, 40, 54, 121, 255, 0, 0, 0, 0, 0, 0, 0},
     /*point_y_scaling=*/{71, 71, 90, 99, 98, 100, 100, 0, 0, 0, 0, 0, 0, 0},
     /*point_u_value=*/{0, 13, 40, 107, 255, 0, 0, 0, 0, 0},
     /*point_u_scaling=*/{37, 37, 48, 51, 51, 0, 0, 0, 0, 0},
     /*point_v_value=*/{0, 13, 27, 40, 54, 94, 255, 0, 0, 0},
     /*point_v_scaling=*/{49, 49, 43, 33, 32, 34, 34, 0, 0, 0},
     /*chroma_scaling=*/11,
     /*auto_regression_coeff_lag=*/3,
     /*auto_regression_coeff_y=*/{2,   -2,  -2,  10,  3, -1, 1,   -4,
                                  6,   0,   -26, -13, 3, -1, 1,   6,
                                  -20, 103, 26,  -2,  1, 13, -48, 117},
     /*auto_regression_coeff_u=*/{-3,  1,  -2, 4,   -4, -1, 2,   5,  -5,
                                  -16, 18, -1, -10, -3, -2, -30, 16, 69,
                                  28,  9,  -2, -10, 50, 68, -11},
     /*auto_regression_coeff_v=*/{2,   -1, -6, 5,   -6, -7, 2,   2,  -11,
                                  -15, 29, 4,  -10, -3, -6, -26, 30, 62,
                                  31,  18, -3, -6,  59, 45, 3},
     /*auto_regression_shift=*/8,
     /*grain_seed=*/17534,
     /*reference_index=*/0,
     /*grain_scale_shift=*/0,
     /*u_multiplier=*/0,
     /*u_luma_multiplier=*/64,
     /*u_offset=*/0,
     /*v_multiplier=*/0,
     /*v_luma_multiplier=*/64,
     /*v_offset=*/0},
    {/*apply_grain=*/true,
     /*update_grain=*/true,
     /*chroma_scaling_from_luma=*/false,
     /*overlap_flag=*/true,
     /*clip_to_restricted_range=*/false,
     /*num_y_points=*/8,
     /*num_u_points=*/7,
     /*num_v_points=*/7,
     /*point_y_value=*/{0, 13, 27, 40, 54, 94, 134, 255, 0, 0, 0, 0, 0, 0},
     /*point_y_scaling=*/{71, 71, 91, 99, 98, 101, 103, 103, 0, 0, 0, 0, 0, 0},
     /*point_u_value=*/{0, 13, 40, 54, 81, 107, 255, 0, 0, 0},
     /*point_u_scaling=*/{37, 37, 49, 49, 52, 53, 53, 0, 0, 0},
     /*point_v_value=*/{0, 13, 27, 40, 54, 94, 255, 0, 0, 0},
     /*point_v_scaling=*/{50, 50, 44, 34, 33, 36, 37, 0, 0, 0},
     /*chroma_scaling=*/11,
     /*auto_regression_coeff_lag=*/3,
     /*auto_regression_coeff_y=*/{2,   -2,  -2,  10,  3, -1, 1,   -4,
                                  3,   1,   -26, -12, 2, -1, 1,   7,
                                  -17, 101, 26,  0,   0, 13, -50, 116},
     /*auto_regression_coeff_u=*/{-2,  1,  -2, 3,   -3, -1, 2,   5,  -4,
                                  -16, 16, -2, -10, -3, -1, -31, 14, 70,
                                  28,  9,  -1, -10, 48, 70, -11},
     /*auto_regression_coeff_v=*/{1,   0,  -5, 5,   -6, -6, 2,   2,  -10,
                                  -14, 26, 4,  -10, -3, -5, -26, 29, 63,
                                  30,  17, -1, -6,  56, 47, 3},
     /*auto_regression_shift=*/8,
     /*grain_seed=*/20915,
     /*reference_index=*/0,
     /*grain_scale_shift=*/0,
     /*u_multiplier=*/0,
     /*u_luma_multiplier=*/64,
     /*u_offset=*/0,
     /*v_multiplier=*/0,
     /*v_luma_multiplier=*/64,
     /*v_offset=*/0},
    {/*apply_grain=*/true,
     /*update_grain=*/true,
     /*chroma_scaling_from_luma=*/false,
     /*overlap_flag=*/true,
     /*clip_to_restricted_range=*/false,
     /*num_y_points=*/7,
     /*num_u_points=*/7,
     /*num_v_points=*/7,
     /*point_y_value=*/{0, 13, 27, 40, 54, 134, 255, 0, 0, 0, 0, 0, 0, 0},
     /*point_y_scaling=*/{72, 72, 91, 99, 97, 101, 101, 0, 0, 0, 0, 0, 0, 0},
     /*point_u_value=*/{0, 13, 40, 54, 67, 107, 255, 0, 0, 0},
     /*point_u_scaling=*/{38, 38, 51, 50, 52, 53, 54, 0, 0, 0},
     /*point_v_value=*/{0, 13, 27, 40, 54, 94, 255, 0, 0, 0},
     /*point_v_scaling=*/{51, 51, 45, 35, 33, 36, 36, 0, 0, 0},
     /*chroma_scaling=*/11,
     /*auto_regression_coeff_lag=*/3,
     /*auto_regression_coeff_y=*/{2,   -2,  -2,  9,   3,  -1, 1,   -3,
                                  2,   2,   -27, -12, 2,  0,  1,   7,
                                  -16, 100, 27,  0,   -1, 13, -51, 116},
     /*auto_regression_coeff_u=*/{-3,  1,  -2, 3,   -3, -1, 1,   4,  -2,
                                  -17, 14, -3, -10, -2, 0,  -31, 14, 71,
                                  29,  8,  -2, -10, 45, 71, -11},
     /*auto_regression_coeff_v=*/{0,   -1, -5, 4,   -6, -5, 2,   1,  -9,
                                  -14, 24, 3,  -10, -3, -4, -25, 29, 63,
                                  31,  16, -1, -7,  54, 48, 2},
     /*auto_regression_shift=*/8,
     /*grain_seed=*/24296,
     /*reference_index=*/0,
     /*grain_scale_shift=*/0,
     /*u_multiplier=*/0,
     /*u_luma_multiplier=*/64,
     /*u_offset=*/0,
     /*v_multiplier=*/0,
     /*v_luma_multiplier=*/64,
     /*v_offset=*/0},
    {/*apply_grain=*/true,
     /*update_grain=*/true,
     /*chroma_scaling_from_luma=*/false,
     /*overlap_flag=*/true,
     /*clip_to_restricted_range=*/false,
     /*num_y_points=*/7,
     /*num_u_points=*/7,
     /*num_v_points=*/8,
     /*point_y_value=*/{0, 13, 27, 40, 54, 134, 255, 0, 0, 0, 0, 0, 0, 0},
     /*point_y_scaling=*/{72, 72, 91, 99, 97, 101, 101, 0, 0, 0, 0, 0, 0, 0},
     /*point_u_value=*/{0, 13, 40, 54, 67, 134, 255, 0, 0, 0},
     /*point_u_scaling=*/{38, 38, 50, 50, 51, 53, 53, 0, 0, 0},
     /*point_v_value=*/{0, 13, 27, 40, 54, 67, 121, 255, 0, 0},
     /*point_v_scaling=*/{50, 50, 45, 34, 33, 35, 36, 36, 0, 0},
     /*chroma_scaling=*/11,
     /*auto_regression_coeff_lag=*/3,
     /*auto_regression_coeff_y=*/{2,   -2,  -2,  10,  3,  -1, 1,   -3,
                                  3,   2,   -27, -12, 2,  0,  1,   7,
                                  -17, 100, 27,  0,   -1, 13, -51, 116},
     /*auto_regression_coeff_u=*/{-3,  1,  -2, 3,   -3, -1, 1,   5,  -3,
                                  -16, 15, -2, -10, -2, -1, -31, 14, 70,
                                  29,  8,  -1, -10, 46, 71, -11},
     /*auto_regression_coeff_v=*/{1,   0,  -5, 5,   -6, -5, 2,   1,  -9,
                                  -14, 25, 4,  -10, -3, -5, -25, 29, 63,
                                  31,  17, -1, -7,  55, 47, 2},
     /*auto_regression_shift=*/8,
     /*grain_seed=*/27677,
     /*reference_index=*/0,
     /*grain_scale_shift=*/0,
     /*u_multiplier=*/0,
     /*u_luma_multiplier=*/64,
     /*u_offset=*/0,
     /*v_multiplier=*/0,
     /*v_luma_multiplier=*/64,
     /*v_offset=*/0},
    {/*apply_grain=*/true,
     /*update_grain=*/true,
     /*chroma_scaling_from_luma=*/false,
     /*overlap_flag=*/true,
     /*clip_to_restricted_range=*/false,
     /*num_y_points=*/7,
     /*num_u_points=*/7,
     /*num_v_points=*/8,
     /*point_y_value=*/{0, 13, 27, 40, 54, 121, 255, 0, 0, 0, 0, 0, 0, 0},
     /*point_y_scaling=*/{72, 72, 92, 99, 97, 101, 101, 0, 0, 0, 0, 0, 0, 0},
     /*point_u_value=*/{0, 13, 40, 54, 67, 174, 255, 0, 0, 0},
     /*point_u_scaling=*/{38, 38, 51, 50, 52, 54, 54, 0, 0, 0},
     /*point_v_value=*/{0, 13, 27, 40, 54, 67, 121, 255, 0, 0},
     /*point_v_scaling=*/{51, 51, 46, 35, 33, 35, 37, 37, 0, 0},
     /*chroma_scaling=*/11,
     /*auto_regression_coeff_lag=*/3,
     /*auto_regression_coeff_y=*/{1,   -1, -2,  9,   3,  -1, 1,   -3,
                                  2,   2,  -28, -12, 2,  0,  1,   8,
                                  -16, 99, 27,  0,   -1, 13, -51, 116},
     /*auto_regression_coeff_u=*/{-3,  1,  -2, 3,   -3, -1, 2,   4,  -2,
                                  -16, 14, -3, -10, -2, 0,  -31, 13, 71,
                                  29,  8,  -2, -11, 44, 72, -11},
     /*auto_regression_coeff_v=*/{0,   -1, -5, 4,   -6, -4, 2,   1,  -9,
                                  -13, 23, 3,  -10, -3, -4, -25, 28, 63,
                                  32,  16, -1, -7,  54, 49, 2},
     /*auto_regression_shift=*/8,
     /*grain_seed=*/31058,
     /*reference_index=*/0,
     /*grain_scale_shift=*/0,
     /*u_multiplier=*/0,
     /*u_luma_multiplier=*/64,
     /*u_offset=*/0,
     /*v_multiplier=*/0,
     /*v_luma_multiplier=*/64,
     /*v_offset=*/0},
    {/*apply_grain=*/true,
     /*update_grain=*/true,
     /*chroma_scaling_from_luma=*/false,
     /*overlap_flag=*/true,
     /*clip_to_restricted_range=*/false,
     /*num_y_points=*/7,
     /*num_u_points=*/7,
     /*num_v_points=*/9,
     /*point_y_value=*/{0, 13, 27, 40, 54, 121, 255, 0, 0, 0, 0, 0, 0, 0},
     /*point_y_scaling=*/{72, 72, 92, 99, 98, 100, 98, 0, 0, 0, 0, 0, 0, 0},
     /*point_u_value=*/{0, 13, 40, 54, 67, 228, 255, 0, 0, 0},
     /*point_u_scaling=*/{38, 38, 51, 51, 52, 54, 54, 0, 0, 0},
     /*point_v_value=*/{0, 13, 27, 40, 54, 67, 121, 201, 255, 0},
     /*point_v_scaling=*/{51, 51, 46, 35, 34, 35, 37, 37, 37, 0},
     /*chroma_scaling=*/11,
     /*auto_regression_coeff_lag=*/3,
     /*auto_regression_coeff_y=*/{1,   -1, -2,  9,   3,  -1, 1,   -3,
                                  2,   2,  -28, -12, 2,  0,  1,   8,
                                  -16, 99, 27,  0,   -1, 13, -52, 116},
     /*auto_regression_coeff_u=*/{-3,  1,  -2, 3,   -3, -1, 1,   4,  -2,
                                  -16, 13, -3, -10, -2, 0,  -31, 13, 71,
                                  29,  8,  -2, -11, 44, 72, -11},
     /*auto_regression_coeff_v=*/{0,   -1, -5, 4,   -6, -4, 2,   2,  -8,
                                  -13, 23, 3,  -10, -3, -4, -25, 28, 63,
                                  32,  16, -1, -7,  54, 49, 2},
     /*auto_regression_shift=*/8,
     /*grain_seed=*/34439,
     /*reference_index=*/0,
     /*grain_scale_shift=*/0,
     /*u_multiplier=*/0,
     /*u_luma_multiplier=*/64,
     /*u_offset=*/0,
     /*v_multiplier=*/0,
     /*v_luma_multiplier=*/64,
     /*v_offset=*/0},
    {/*apply_grain=*/true,
     /*update_grain=*/true,
     /*chroma_scaling_from_luma=*/false,
     /*overlap_flag=*/true,
     /*clip_to_restricted_range=*/false,
     /*num_y_points=*/7,
     /*num_u_points=*/7,
     /*num_v_points=*/9,
     /*point_y_value=*/{0, 13, 27, 40, 54, 121, 255, 0, 0, 0, 0, 0, 0, 0},
     /*point_y_scaling=*/{72, 72, 92, 99, 98, 99, 95, 0, 0, 0, 0, 0, 0, 0},
     /*point_u_value=*/{0, 13, 40, 54, 67, 228, 255, 0, 0, 0},
     /*point_u_scaling=*/{39, 39, 51, 51, 52, 54, 54, 0, 0, 0},
     /*point_v_value=*/{0, 13, 27, 40, 54, 67, 121, 201, 255, 0},
     /*point_v_scaling=*/{51, 51, 46, 35, 34, 35, 36, 35, 35, 0},
     /*chroma_scaling=*/11,
     /*auto_regression_coeff_lag=*/3,
     /*auto_regression_coeff_y=*/{1,   -1, -2,  9,   3,  -1, 1,   -3,
                                  2,   2,  -28, -11, 2,  0,  1,   8,
                                  -16, 99, 27,  0,   -1, 13, -52, 116},
     /*auto_regression_coeff_u=*/{-3,  1,  -2, 3,   -3, -1, 1,   4,  -2,
                                  -16, 13, -3, -10, -2, 0,  -30, 13, 71,
                                  29,  8,  -2, -10, 43, 72, -11},
     /*auto_regression_coeff_v=*/{0,   -1, -5, 3,   -6, -4, 2,   2,  -8,
                                  -13, 23, 3,  -10, -3, -4, -25, 28, 64,
                                  32,  16, -1, -7,  53, 49, 2},
     /*auto_regression_shift=*/8,
     /*grain_seed=*/37820,
     /*reference_index=*/0,
     /*grain_scale_shift=*/0,
     /*u_multiplier=*/0,
     /*u_luma_multiplier=*/64,
     /*u_offset=*/0,
     /*v_multiplier=*/0,
     /*v_luma_multiplier=*/64,
     /*v_offset=*/0}};

const char* GetTestDigestLuma(int bitdepth, int param_index) {
  static const char* const kTestDigestsLuma8bpp[10] = {
      "80da8e849110a10c0a73f9dec0d9a2fb", "54352f02aeda541e17a4c2d208897e2b",
      "2ad9021124c82aca3e7c9517d00d1236", "f6c5f64513925b09ceba31e92511f8a1",
      "46c6006578c68c3c8619f7a389c7de45", "fcddbd27545254dc50f1c333c8b7e313",
      "c6d4dc181bf7f2f93ae099b836685151", "2949ef836748271195914fef9acf4e46",
      "524e79bb87ed550e123d00a61df94381", "182222470d7b7a80017521d0261e4474",
  };
  static const char* const kTestDigestsLuma10bpp[10] = {
      "27a49a2131fb6d4dd4b8c34da1b7642e", "4ea9134f6831dd398545c85b2a68e31f",
      "4e12232a18a2b06e958d7ab6b953faad", "0ede12864ddaced2d8062ffa4225ce24",
      "5fee492c4a430b2417a64aa4920b69e9", "39af842a3f9370d796e8ef047c0c42a8",
      "0efbad5f9dc07391ad243232b8df1787", "2bd41882cd82960019aa2b87d5fb1fbc",
      "1c66629c0c4e7b6f9b0a7a6944fbad50", "2c633a50ead62f8e844a409545f46244",
  };
  static const char* const kTestDigestsLuma12bpp[10] = {
      "1dc9b38a93454a85eb924f25346ae369", "5f9d311ee5384a5a902f8e2d1297319e",
      "cf1a35878720564c7a741f91eef66565", "47a0608fe0f6f7ccae42a5ca05783cbf",
      "dbc28da0178e3c18a036c3f2203c300f", "04911d2074e3252119ee2d80426b8c01",
      "df19ab8103c40b726c842ccf7772208b", "39276967eb16710d98f82068c3eeba41",
      "b83100f18abb2062d9c9969f07182b86", "b39a69515491329698cf66f6d4fa371f",
  };

  switch (bitdepth) {
    case 8:
      return kTestDigestsLuma8bpp[param_index];
    case 10:
      return kTestDigestsLuma10bpp[param_index];
    case 12:
      return kTestDigestsLuma12bpp[param_index];
    default:
      assert(bitdepth == 8 || bitdepth == 10 || bitdepth == 12);
      return nullptr;
  }
}

const char* GetTestDigestChromaU(int bitdepth, int param_index) {
  static const char* const kTestDigestsChromaU8bpp[10] = {
      "e56b7bbe9f39bf987770b18aeca59514", "d0b3fd3cf2901dae31b73f20c510d83e",
      "800c01d58d9fb72136d21ec2bb07899a", "4cd0badba679e8edbcd60a931fce49a1",
      "cabec236cc17f91f3f08d8cde867aa72", "380a2205cf2d40c6a27152585f61a3b0",
      "3813526234dc7f90f80f6684772c729a", "97a43a73066d88f9cbd915d56fc9c196",
      "5b70b27a43dd63b03e23aecd3a935071", "d5cc98685582ffd47a41a97d2e377ac8",
  };
  static const char* const kTestDigestsChromaU10bpp[10] = {
      "9a6d0369ba86317598e65913276dae6d", "2512bdc4c88f21f8185b040b7752d1db",
      "1e86b779ce6555fcf5bd0ade2af67e73", "5ad463a354ffce522c52b616fb122024",
      "290d53c22c2143b0882acb887da3fdf1", "54622407d865371d7e70bbf29fdda626",
      "be306c6a94c55dbd9ef514f0ad4a0011", "904602329b0dec352b3b177b0a2554d2",
      "58afc9497d968c67fdf2c0cf23b33aa3", "74fee7be6f62724bf901fdd04a733b46",
  };
  static const char* const kTestDigestsChromaU12bpp[10] = {
      "846d608050fe7c19d6cabe2d53cb7821", "2caf4665a26aad50f68497e4b1326417",
      "ce40f0f8f8c207c7c985464c812fea33", "820de51d07a21da5c00833bab546f1fa",
      "5e7bedd8933cd274af03babb4dbb94dd", "d137cf584eabea86387460a6d3f62bfe",
      "f206e0c6ed35b3ab35c6ff37e151e963", "55d87981b7044df225b3b5935185449b",
      "6a655c8bf4df6af0e80ae6d004a73a25", "6234ae36076cc77161af6e6e3c04449a",
  };

  switch (bitdepth) {
    case 8:
      return kTestDigestsChromaU8bpp[param_index];
    case 10:
      return kTestDigestsChromaU10bpp[param_index];
    case 12:
      return kTestDigestsChromaU12bpp[param_index];
    default:
      assert(bitdepth == 8 || bitdepth == 10 || bitdepth == 12);
      return nullptr;
  }
}

const char* GetTestDigestChromaV(int bitdepth, int param_index) {
  static const char* const kTestDigestsChromaV8bpp[10] = {
      "7205ed6c07ed27b7b52d871e0559b8fa", "fad033b1482dba0ed2d450b461fa310e",
      "6bb39798ec6a0f7bda0b0fcb0a555734", "08c19856e10123ae520ccfc63e2fbe7b",
      "a7695a6b69fba740a50310dfa6cf1c00", "ac2eac2d13fc5b21c4f2995d5abe14b9",
      "be35cb30062db628a9e1304fca8b75dc", "f5bfc7a910c76bcd5b32c40772170879",
      "aca07b37d63f978d76df5cd75d0cea5e", "107c7c56d4ec21f346a1a02206301b0d",
  };
  static const char* const kTestDigestsChromaV10bpp[10] = {
      "910724a77710996c90e272f1c1e9ff8e", "d293f861580770a89f1e266931a012ad",
      "9e4f0c85fb533e51238586f9c3e68b6e", "a5ff4478d9eeb2168262c2e955e17a4f",
      "fba6b1e8f28e4e90c836d41f28a0c154", "50b9a93f9a1f3845e6903bff9270a3e6",
      "7b1624c3543badf5fadaee4d1e602e6b", "3be074e4ca0eec5770748b15661aaadd",
      "639197401032f272d6c30666a2d08f43", "28075dd34246bf9d5e6197b1944f646a",
  };
  static const char* const kTestDigestsChromaV12bpp[10] = {
      "4957ec919c20707d594fa5c2138c2550", "3f07c65bfb42c81768b1f5ad9611d1ce",
      "665d9547171c99faba95ac81a35c9a0c", "1b5d032e0cefdb4041ad51796de8a45e",
      "18fa974579a4f1ff8cd7df664fc339d5", "2ffaa4f143495ff73c06a580a97b6321",
      "4fd1f562bc47a68dbfaf7c566c7c4da6", "4d37c80c9caf110c1d3d20bd1a1875b3",
      "8ea29759640962613166dc5154837d14", "5ca4c10f42d0906c72ebee90fae6ce7d",
  };

  switch (bitdepth) {
    case 8:
      return kTestDigestsChromaV8bpp[param_index];
    case 10:
      return kTestDigestsChromaV10bpp[param_index];
    case 12:
      return kTestDigestsChromaV12bpp[param_index];
    default:
      assert(bitdepth == 8 || bitdepth == 10 || bitdepth == 12);
      return nullptr;
  }
}

const char* GetARTestDigestLuma(int bitdepth, int coeff_lag, int param_index) {
  static const char* const kTestDigestsLuma8bpp[3][kNumFilmGrainTestParams] = {
      {"a835127918f93478b45f1ba4d20d81bd", "a835127918f93478b45f1ba4d20d81bd",
       "e5db4da626e214bb17bcc7ecffa76303", "a835127918f93478b45f1ba4d20d81bd",
       "a835127918f93478b45f1ba4d20d81bd", "e5db4da626e214bb17bcc7ecffa76303",
       "a835127918f93478b45f1ba4d20d81bd", "1da62b7233de502123a18546b6c97da2",
       "1da62b7233de502123a18546b6c97da2", "1da62b7233de502123a18546b6c97da2"},
      {"11464b880de3ecd6e6189c5c4e7f9b28", "dfe411762e283b5f49bece02ec200951",
       "5c534d92afdf0a5b53dbe4fe7271929c", "2e1a68a18aca96c31320ba7ceab59be9",
       "584c0323e6b276cb9acb1a294d462d58", "9571eb8f1cbaa96ea3bf64a820a8d9f0",
       "305285ff0df87aba3c59e3fc0818697d", "0066d35c8818cf20230114dcd3765a4d",
       "0066d35c8818cf20230114dcd3765a4d", "16d61b046084ef2636eedc5a737cb6f6"},
      {"0c9e2cf1b6c3cad0f7668026e8ea0516", "7d094855292d0eded9e0d1b5bab1990b",
       "fbf28860a5f1285dcc6725a45256a86a", "dccb906904160ccabbd2c9a7797a4bf9",
       "46f645e17f08a3260b1ae70284e5c5b8", "124fdc90bed11a7320a0cbdee8b94400",
       "8d2978651dddeaef6282191fa146f0a0", "28b4d5aa33f05b3fb7f9323a11936bdc",
       "6a8ea684f6736a069e3612d1af6391a8", "2781ea40a63704dbfeb3a1ac5db6f2fc"},
  };

  static const char* const kTestDigestsLuma10bpp[3][kNumFilmGrainTestParams] = {
      {"5e6bc8444ece2d38420f51d82238d812", "5e6bc8444ece2d38420f51d82238d812",
       "2bfaec768794af33d60a9771f971f68d", "5e6bc8444ece2d38420f51d82238d812",
       "5e6bc8444ece2d38420f51d82238d812", "c880807a368c4e82c23bea6f035ad23f",
       "5e6bc8444ece2d38420f51d82238d812", "c576667da5286183ec3aab9a76f53a2e",
       "c576667da5286183ec3aab9a76f53a2e", "c576667da5286183ec3aab9a76f53a2e"},
      {"095c2dd4d4d52aff9696df9bfdb70062", "983d14afa497060792d472a449a380c7",
       "c5fdc0f7c594b2b36132cec6f45a79bd", "acff232ac5597c1712213150552281d1",
       "4dd7341923b1d260092853553b6b6246", "0ca8afd71a4f564ea1ce69c4af14e9ab",
       "9bc7565e5359d09194fcee28e4bf7b94", "6fea7805458b9d149f238a30e2dc3f13",
       "6fea7805458b9d149f238a30e2dc3f13", "681dff5fc7a7244ba4e4a582ca7ecb14"},
      {"cb99352c9c6300e7e825188bb4adaee0", "7e40674de0209bd72f8e9c6e39ee6f7c",
       "3e475572f6b4ecbb2730fd16751ad7ed", "e6e4c63abc9cb112d9d1f23886cd1415",
       "1a1c953b175c105c604902877e2bab18", "380a53072530223d4ee622e014ee4bdb",
       "6137394ea1172fb7ea0cbac237ff1703", "85ab0c813e46f97cb9f42542f44c01ad",
       "68c8ac462f0e28cb35402c538bee32f1", "0038502ffa4760c8feb6f9abd4de7250"},
  };

  static const char* const kTestDigestsLuma12bpp[3][kNumFilmGrainTestParams] = {
      {"d618bbb0e337969c91b1805f39561520", "d618bbb0e337969c91b1805f39561520",
       "678f6e911591daf9eca4e305dabdb2b3", "d618bbb0e337969c91b1805f39561520",
       "d618bbb0e337969c91b1805f39561520", "3b26f49612fd587c7360790d40adb5de",
       "d618bbb0e337969c91b1805f39561520", "33f77d3ff50cfc64c6bc9a896b567377",
       "33f77d3ff50cfc64c6bc9a896b567377", "33f77d3ff50cfc64c6bc9a896b567377"},
      {"362fd67050fb7abaf57c43a92d993423", "e014ae0eb9e697281015c38905cc46ef",
       "82b867e57151dc08afba31eccf5ccf69", "a94ba736cdce7bfa0b550285f59e47a9",
       "3f1b0b7dd3b10e322254d35e4e185b7c", "7929708e5f017d58c53513cb79b35fda",
       "6d26d31a091cbe642a7070933bd7de5a", "dc29ac40a994c0a760bfbad0bfc15b3a",
       "dc29ac40a994c0a760bfbad0bfc15b3a", "399b919db5190a5311ce8d166580827b"},
      {"6116d1f569f5b568eca4dc1fbf255086", "7e9cf31ea74e8ea99ffd12094ce6cd05",
       "bb982c4c39e82a333d744defd16f4388", "7c6e584b082dc6b97ed0d967def3993f",
       "fb234695353058f03c8e128f2f8de130", "9218c6ca67bf6a9237f98aa1ce7acdfd",
       "d1fb834bbb388ed066c5cbc1c79b5bdf", "d6f630daedc08216fcea12012e7408b5",
       "dd7fe49299e6f113a98debc7411c8db8", "8b89e45a5101a28c24209ae119eafeb8"},
  };

  switch (bitdepth) {
    case 8:
      return kTestDigestsLuma8bpp[coeff_lag - 1][param_index];
    case 10:
      return kTestDigestsLuma10bpp[coeff_lag - 1][param_index];
    case 12:
      return kTestDigestsLuma12bpp[coeff_lag - 1][param_index];
    default:
      assert(bitdepth == 8 || bitdepth == 10 || bitdepth == 12);
      return nullptr;
  }
}

const char* GetARTestDigestChromaU(int bitdepth, int coeff_lag,
                                   int subsampling_x, int subsampling_y) {
  static const char* const kTestDigestsChromaU8bpp[12] = {
      "11ced66de0eaf55c1ff9bad18d7b8ed7", "0c3b77345dd4ab0915ef53693ab93ce4",
      "b0645044ba080b3ceb8f299e269377d6", "50590ad5d895f0b4bc6694d878e9cd32",
      "85e1bf3741100135062f5b4abfe7639b", "76955b70dde61ca5c7d079c501b90906",
      "3f0995e1397fd9efd9fc46b67f7796b3", "0a0d6c3e4e1649eb101395bc97943a07",
      "1878855ed8db600ccae1d39abac52ec6", "13ab2b28320ed3ac2b820f08fdfd424d",
      "f3e95544a86ead5387e3dc4e043fd0f0", "ff8f5d2d97a6689e16a7e4f482f69f0b",
  };

  static const char* const kTestDigestsChromaU10bpp[12] = {
      "707f2aa5aa7e77bc6e83ab08287d748d", "0bcf40c7fead9ac3a5d71b4cc1e21549",
      "0c1df27053e5da7cf1276a122a8f4e8b", "782962f7425eb38923a4f87e7ab319d9",
      "b4a709ae5967afef55530b9ea8ef0062", "70a971a0b9bf06212d510b396f0f9095",
      "d033b89d6e31f8b13c83d94c840b7d54", "40bbe804bf3f90cee667d3b275e3c964",
      "90bb2b9d518b945adcfd1b1807f7d170", "4bc34aa157fe5ad4270c611afa75e878",
      "e2688d7286cd43fe0a3ea734d2ad0f77", "853193c4981bd882912171061327bdf2",
  };

  static const char* const kTestDigestsChromaU12bpp[12] = {
      "04c23b01d01c0e3f3247f3741581b383", "9f8ea1d66e44f6fe93d765ce56b2b0f3",
      "5dda44b128d6c244963f1e8e17cc1d22", "9dd0a79dd2f772310a95762d445bface",
      "0dbd40d930e4873d72ea72b9e3d62440", "d7d83c207c6b435a164206d5f457931f",
      "e8d04f6e63ed63838adff965275a1ff1", "fc09a903e941fcff8bad67a84f705775",
      "9cd706606a2aa40d0957547756f7abd9", "258b37e7b8f48db77dac7ea24073fe69",
      "80149b8bb05308da09c1383d8b79d3da", "e993f3bffae53204a1942feb1af42074",
  };

  assert(!(subsampling_x == 0 && subsampling_y == 1));
  const int base_index = 3 * coeff_lag + subsampling_x + subsampling_y;
  switch (bitdepth) {
    case 8:
      return kTestDigestsChromaU8bpp[base_index];
    case 10:
      return kTestDigestsChromaU10bpp[base_index];
    case 12:
      return kTestDigestsChromaU12bpp[base_index];
    default:
      assert(bitdepth == 8 || bitdepth == 10 || bitdepth == 12);
      return nullptr;
  }
}

const char* GetARTestDigestChromaV(int bitdepth, int coeff_lag,
                                   int subsampling_x, int subsampling_y) {
  static const char* const kTestDigestsChromaV8bpp[12] = {
      "5c2179f3d93be0a0da75d2bb90347c2f", "79b883847d7eaa7890e1d633b8e34353",
      "90ade818e55808e8cf58c11debb5ddd1", "1d0f2a14bc4df2b2a1abaf8137029f92",
      "ac753a57ade140dccb50c14f941ae1fc", "d24ab497558f6896f08dc17bcc3c50c1",
      "3d74436c63920022a95c85b234db4e33", "061c2d53ed84c830f454e395c362cb16",
      "05d24869d7fb952e332457a114c8b9b7", "fcee31b87a2ada8028c2a975e094856a",
      "c019e2c475737abcf9c2b2a52845c646", "9cd994baa7021f8bdf1d1c468c1c8e9c",
  };

  static const char* const kTestDigestsChromaV10bpp[12] = {
      "bc9e44454a05cac8571c15af5b720e79", "f0374436698d94e879c03331b1f30df4",
      "4580dd009abd6eeed59485057c55f63e", "7d1f7aecd45302bb461f4467f2770f72",
      "1f0d003fce6c5fedc147c6112813f43b", "4771a45c2c1a04c375400619d5536035",
      "df9cf619a78907c0f6e58bc13d7d5546", "dd3715ce65d905f30070a36977c818e0",
      "32de5800f76e34c128a1d89146b4010b", "db9d7c70c3f69feb68fae04398efc773",
      "d3d0912e3fdb956fef416a010bd7b4c2", "a2fca8abd9fd38d2eef3c4495d9eff78",
  };

  static const char* const kTestDigestsChromaV12bpp[12] = {
      "0d1890335f4464167de22353678ca9c6", "9e6830aba73139407196f1c811f910bc",
      "6018f2fb76bd648bef0262471cfeba5c", "78e1ae1b790d709cdb8997621cf0fde3",
      "5b44ae281d7f9db2f17aa3c24b4741dd", "f931d16991669cb16721de87da9b8067",
      "5580f2aed349d9cabdafb9fc25a57b1c", "86918cd78bf95e6d4405dd050f5890b8",
      "13c8b314eeebe35fa60b703d94e1b2c1", "13c6fb75cab3f42e0d4ca31e4d068b0e",
      "bb9ca0bd6f8cd67e44c8ac2803abf5a5", "0da4ea711ffe557bb66577392b6f148b",
  };

  assert(!(subsampling_x == 0 && subsampling_y == 1));
  const int base_index = 3 * coeff_lag + subsampling_x + subsampling_y;
  switch (bitdepth) {
    case 8:
      return kTestDigestsChromaV8bpp[base_index];
    case 10:
      return kTestDigestsChromaV10bpp[base_index];
    case 12:
      return kTestDigestsChromaV12bpp[base_index];
    default:
      assert(bitdepth == 8 || bitdepth == 10 || bitdepth == 12);
      return nullptr;
  }
}

const char* GetGrainGenerationTestDigestLuma(int bitdepth, int param_index) {
  static const char* const kTestDigestsLuma8bpp[kNumFilmGrainTestParams] = {
      "c48babd99e5cfcbaa13d8b6e0c12e644", "da4b971d2de19b709e2bc98d2e50caf3",
      "96c72faac19a79c138afeea8b8ae8c7a", "90a2b9c8304a44d14e83ca51bfd2fe8a",
      "72bd3aa85c17850acb430afb4183bf1a", "a0acf76349b9efbc9181fc31153d9ef6",
      "6da74dd631a4ec8b9372c0bbec22e246", "6e11fa230f0e5fbb13084255c22cabf9",
      "be1d257b762f9880d81680e9325932a2", "37e302075af8130b371de4430e8a22cf",
  };

  static const char* const kTestDigestsLuma10bpp[kNumFilmGrainTestParams] = {
      "0a40fd2f261095a6154584a531328142", "9d0c8173a94a0514c769e94b6f254030",
      "7894e959fdd5545895412e1512c9352d", "6802cad2748cf6db7f66f53807ee46ab",
      "ea24e962b98351c3d929a8ae41e320e2", "b333dc944274a3a094073889ca6e11d6",
      "7211d7ac0ff7d11b5ef1538c0d98f43d", "ef9f9cbc101a07da7bfa62637130e331",
      "85a122e32648fde84b883a1f98947c60", "dee656e3791138285bc5b71e3491a177",
  };

  static const char* const kTestDigestsLuma12bpp[kNumFilmGrainTestParams] = {
      "ae359794b5340d073d597117046886ac", "4d4ad3908b4fb0f248a0086537dd6b1e",
      "672a97e15180cbeeaf76d763992c9f23", "739124d10d16e00a158e833ea92107bc",
      "4c38c738ff7ffc50adaa4474584d3aae", "ca05ba7e51000a7d10e5cbb2101bbd86",
      "e207022b916bf03a76ac8742af29853d", "7454bf1859149237ff74f1161156c857",
      "10fc2a16e663bbc305255b0883cfcd45", "4228abff6899bb33839b579288ab29fe",
  };

  switch (bitdepth) {
    case 8:
      return kTestDigestsLuma8bpp[param_index];
    case 10:
      return kTestDigestsLuma10bpp[param_index];
    case 12:
      return kTestDigestsLuma12bpp[param_index];
    default:
      assert(bitdepth == 8 || bitdepth == 10 || bitdepth == 12);
      return nullptr;
  }
}

const char* GetConstructStripesTestDigest(int bitdepth, int overlap_flag,
                                          int subsampling_x,
                                          int subsampling_y) {
  static const char* const kTestDigests8bpp[6] = {
      "cd14aaa6fc1728290fa75772730a2155", "13ad4551feadccc3a3a9bd5e25878d2a",
      "ed6ad9532c96ef0d79ff3228c89a429f", "82f307a7f5fc3308c3ebe268b5169e70",
      "aed793d525b85349a8c2eb6d40e93969", "311c3deb727621a7d4f18e8defb65de7",
  };

  static const char* const kTestDigests10bpp[6] = {
      "4fe2fa1e428737de3595be3a097d0203", "80568c3c3b53bdbbd03b820179092dcd",
      "bc7b73099961a0739c36e027d6d09ea1", "e5331364e5146a6327fd94e1467f59a3",
      "125bf18b7787e8f0792ea12f9210de0d", "21cf98cbce17eca77dc150cc9be0e0a0",
  };

  static const char* const kTestDigests12bpp[6] = {
      "57f8e17078b6e8935252e918a2562636", "556a7b294a99bf1163b7166b4f68357e",
      "249bee5572cd7d1cc07182c97adc4ba7", "9bf43ae1998c2a5b2e5f4d8236b58747",
      "477c08fa26499936e5bb03bde097633e", "fe64b7166ff87ea0711ae4f519cadd59",
  };

  const int base_index = 3 * overlap_flag + subsampling_x + subsampling_y;
  switch (bitdepth) {
    case 8:
      return kTestDigests8bpp[base_index];
    case 10:
      return kTestDigests10bpp[base_index];
    case 12:
      return kTestDigests12bpp[base_index];
    default:
      assert(bitdepth == 8 || bitdepth == 10 || bitdepth == 12);
      return nullptr;
  }
}

const char* GetConstructImageTestDigest(int bitdepth, int overlap_flag,
                                        int subsampling_x, int subsampling_y) {
  static const char* const kTestDigests8bpp[6] = {
      "17030fc692e685557a3717f9334af7e8", "d16ea46147183cd7bc36bcfc2f936a5b",
      "68152958540dbec885f71e3bcd7aa088", "bb43b420f05a122eb4780aca06055ab1",
      "87567b04fbdf64f391258c0742de266b", "ce87d556048b3de32570faf6729f4010",
  };

  static const char* const kTestDigests10bpp[6] = {
      "5b31b29a5e22126a9bf8cd6a01645777", "2bb94a25164117f2ab18dae18e2c6577",
      "27e57a4ed6f0c9fe0a763a03f44805e8", "481642ab0b07437b76b169aa4eb82123",
      "656a9ef056b04565bec9ca7e0873c408", "a70fff81ab28d02d99dd4f142699ba39",
  };

  static const char* const kTestDigests12bpp[6] = {
      "146f7ceadaf77e7a3c41e191a58c1d3c", "de18526db39630936733e687cdca189e",
      "165c96ff63bf3136505ab1d239f7ceae", "a102636662547f84e5f6fb6c3e4ef959",
      "4cb073fcc783c158a95c0b1ce0d27e9f", "3a734c71d4325a7da53e2a6e00f81647",
  };

  const int base_index = 3 * overlap_flag + subsampling_x + subsampling_y;
  switch (bitdepth) {
    case 8:
      return kTestDigests8bpp[base_index];
    case 10:
      return kTestDigests10bpp[base_index];
    case 12:
      return kTestDigests12bpp[base_index];
    default:
      assert(bitdepth == 8 || bitdepth == 10 || bitdepth == 12);
      return nullptr;
  }
}

const char* GetScalingInitTestDigest(int param_index, int bitdepth) {
  static const char* const kTestDigests8bpp[kNumFilmGrainTestParams] = {
      "315202ca3bf9c46eac8605e89baffd2a", "640f6408702b07ab7e832e7326cce56f",
      "f75ee83e3912a3f25949e852d67326cf", "211223f5d6a4b42a8e3c662f921b71c0",
      "f75ee83e3912a3f25949e852d67326cf", "e7a1de8c5a2cac2145c586ecf1f9051c",
      "e7a1de8c5a2cac2145c586ecf1f9051c", "276fe5e3b30b2db2a9ff798eb6cb8e00",
      "ac67f1c3aff2f50ed4b1975bde67ffe3", "8db6145a60d506cc94f07cef8b27c681",
  };

  static const char* const kTestDigests10bpp[kNumFilmGrainTestParams] = {
      "c50be59c62b634ff45ddfbe5b978adfc", "7626286109a2a1eaf0a26f6b2bbab9aa",
      "f2302988140c47a0724fc55ff523b6ec", "5318e33d8a59a526347ffa6a72ba6ebd",
      "f2302988140c47a0724fc55ff523b6ec", "f435b5fe98e9d8b6c61fa6f457601c2c",
      "f435b5fe98e9d8b6c61fa6f457601c2c", "ff07a2944dbe094d01e199098764941c",
      "11b3e256c74cee2b5679f7457793869a", "89fab5c1db09e242d0494d1c696a774a",
  };

  static const char* const kTestDigests12bpp[kNumFilmGrainTestParams] = {
      "1554df49a863a851d146213e09d311a4", "84808c3ed3b5495a62c9d2dd9a08cb26",
      "bb31f083a3bd9ef26587478b8752f280", "34fdfe61d6871e4882e38062a0725c5c",
      "bb31f083a3bd9ef26587478b8752f280", "e7b8c3e4508ceabe89b78f10a9e160b8",
      "e7b8c3e4508ceabe89b78f10a9e160b8", "a0ccc9e3d0f0c9d1f08f1249264d92f5",
      "7992a96883c8a9a35d6ca8961bc4515b", "de906ce2c0fceed6f168215447b21b16",
  };

  switch (bitdepth) {
    case 8:
      return kTestDigests8bpp[param_index];
    case 10:
      return kTestDigests10bpp[param_index];
    case 12:
      return kTestDigests12bpp[param_index];
    default:
      assert(bitdepth == 8 || bitdepth == 10 || bitdepth == 12);
      return nullptr;
  }
}

const char* GetBlendLumaTestDigest(int bitdepth) {
  static const char* const kTestDigests[] = {
      "de35b16c702690b1d311cdd0973835d7",
      "60e9f24dcaaa0207a8db5ab5f3c66608",
      "8e7d44b620bb7768459074be6bfbca7b",
  };

  assert(bitdepth == 8 || bitdepth == 10 || bitdepth == 12);
  return kTestDigests[(bitdepth - 8) / 2];
}

const char* GetBlendChromaUTestDigest(int bitdepth,
                                      int chroma_scaling_from_luma,
                                      int subsampling_x, int subsampling_y) {
  static const char* const kTestDigests8bpp[6] = {
      "36ca194734d45e75079baba1f3ec9e9e", "182b388061f59fd3e24ef4581c536e67",
      "2e7843b4c624f03316c3cbe1cc835859", "39e6d9606915da6a41168fbb006b55e4",
      "3f44a4e252d4823544ac66a900dc7983", "1860f0831841f262d66b23f6a6b5833b",
  };

  static const char* const kTestDigests10bpp[6] = {
      "2054665564f55750c9588b505eb01ac0", "4d8b0e248f8a6bfc72516aa164e76b0b",
      "7e549800a4f9fff6833bb7738e272baf", "8de6f30dcda99a37b359fd815e62d2f7",
      "9b7958a2278a16bce2b7bc31fdd811f5", "c5c3c8cccf6a2b4e40b4a412a5bf4f08",
  };

  static const char* const kTestDigests12bpp[6] = {
      "8fad0cc641da35e0d2d8f178c7ce8394", "793eb9d2e6b4ea2e3bb08e7068236155",
      "9156bd85ab9493d8867a174f920bb1e6", "6834319b4c88e3e0c96b6f8d7efd08dd",
      "c40e492790d3803a734efbc6feca46e2", "d884c3b1e2c21d98844ca7639e0599a5",
  };

  const int base_index =
      3 * chroma_scaling_from_luma + subsampling_x + subsampling_y;
  switch (bitdepth) {
    case 8:
      return kTestDigests8bpp[base_index];
    case 10:
      return kTestDigests10bpp[base_index];
    case 12:
      return kTestDigests12bpp[base_index];
    default:
      assert(bitdepth == 8 || bitdepth == 10 || bitdepth == 12);
      return nullptr;
  }
}

const char* GetBlendChromaVTestDigest(int bitdepth,
                                      int chroma_scaling_from_luma,
                                      int subsampling_x, int subsampling_y) {
  static const char* const kTestDigests8bpp[6] = {
      "9a353e4f86d7ebaa980f7f6cfc0995ad", "17589b4039ed49ba16f32db9fae724b7",
      "76ae8bed48a173b548993b6e1824ff67", "c1458ac9bdfbf0b4d6a175343b17b27b",
      "fa76d1c8e48957537f26af6a5b54ec14", "313fe3c34568b7f9c5ecb09d419d4ba4",
  };

  static const char* const kTestDigests10bpp[6] = {
      "8ab5a8e03f07547260033d6a0b689e3c", "275ede58d311e2f5fd76f222f45a64fc",
      "ce13916e0f7b02087fd0356534d32770", "165bfc8cda0266936a67fa4ec9b215cb",
      "ed4382caa936acf1158ff8049d18ffac", "942bdd1344c9182dd7572099fb9372db",
  };

  static const char* const kTestDigests12bpp[6] = {
      "70704a1e171a3a70d40b7d0037a75fbc", "62549e2afbf36a1ed405a6574d39c542",
      "e93889927ab77c6e0767ff071d980c02", "a0c1f6ed78874137710fee7418d80959",
      "f6283e36a25cb867e30bdf0bfdb2124b", "741c2d48898835b9d9e3bd0b6ac6269a",
  };

  const int base_index =
      3 * chroma_scaling_from_luma + subsampling_x + subsampling_y;
  switch (bitdepth) {
    case 8:
      return kTestDigests8bpp[base_index];
    case 10:
      return kTestDigests10bpp[base_index];
    case 12:
      return kTestDigests12bpp[base_index];
    default:
      assert(bitdepth == 8 || bitdepth == 10 || bitdepth == 12);
      return nullptr;
  }
}

// GetFilmGrainRandomNumber() is only invoked with |bits| equal to 11 or 8. Test
// both values of |bits|.
TEST(FilmGrainTest, GetFilmGrainRandomNumber) {
  uint16_t seed = 51968;
  const struct {
    int rand;
    uint16_t seed;
  } kExpected11[5] = {
      {812, 25984}, {406, 12992}, {1227, 39264}, {1637, 52400}, {818, 26200},
  };
  for (int i = 0; i < 5; ++i) {
    int rand = GetFilmGrainRandomNumber(11, &seed);
    EXPECT_EQ(rand, kExpected11[i].rand) << "i = " << i;
    EXPECT_EQ(seed, kExpected11[i].seed) << "i = " << i;
  }
  const struct {
    int rand;
    uint16_t seed;
  } kExpected8[5] = {
      {179, 45868}, {89, 22934}, {44, 11467}, {150, 38501}, {75, 19250},
  };
  for (int i = 0; i < 5; ++i) {
    int rand = GetFilmGrainRandomNumber(8, &seed);
    EXPECT_EQ(rand, kExpected8[i].rand) << "i = " << i;
    EXPECT_EQ(seed, kExpected8[i].seed) << "i = " << i;
  }
}

// In FilmGrainParams, if num_u_points and num_v_points are both 0 and
// chroma_scaling_from_luma is false, GenerateChromaGrains() should set both
// the u_grain and v_grain arrays to all zeros.
TEST(FilmGrainTest, GenerateZeroChromaGrains) {
  FilmGrainParams film_grain_params = {};
  film_grain_params.apply_grain = true;
  film_grain_params.update_grain = true;
  film_grain_params.chroma_scaling = 8;
  film_grain_params.auto_regression_shift = 6;
  film_grain_params.grain_seed = 51968;

  int8_t u_grain[73 * 82];
  int8_t v_grain[73 * 82];
  const int chroma_width = 44;
  const int chroma_height = 38;

  // Initialize u_grain and v_grain with arbitrary nonzero values.
  memset(u_grain, 1, sizeof(u_grain));
  memset(v_grain, 2, sizeof(v_grain));
  for (int y = 0; y < chroma_height; ++y) {
    for (int x = 0; x < chroma_width; ++x) {
      EXPECT_NE(u_grain[y * chroma_width + x], 0);
      EXPECT_NE(v_grain[y * chroma_width + x], 0);
    }
  }

  FilmGrain<8>::GenerateChromaGrains(film_grain_params, chroma_width,
                                     chroma_height, u_grain, v_grain);

  for (int y = 0; y < chroma_height; ++y) {
    for (int x = 0; x < chroma_width; ++x) {
      EXPECT_EQ(u_grain[y * chroma_width + x], 0);
      EXPECT_EQ(v_grain[y * chroma_width + x], 0);
    }
  }
}

// First parameter is coefficient lag. Second parameter is the index into
// |kFilmGrainParams|.
template <int bitdepth>
class AutoRegressionTestLuma
    : public testing::TestWithParam<std::tuple<int, int>> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  using GrainType =
      typename std::conditional<bitdepth == 8, int8_t, int16_t>::type;

  AutoRegressionTestLuma() {
    FilmGrainInit_C();
    const dsp::Dsp* const dsp = dsp::GetDspTable(bitdepth);
    const int index = std::get<0>(GetParam()) - 1;
    base_luma_auto_regression_func_ =
        dsp->film_grain.luma_auto_regression[index];

    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const char* const test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
      base_luma_auto_regression_func_ = nullptr;
    } else if (absl::StartsWith(test_case, "NEON/")) {
#if LIBGAV1_ENABLE_NEON
      FilmGrainInit_NEON();
#endif
    }
    luma_auto_regression_func_ = dsp->film_grain.luma_auto_regression[index];
  }

 protected:
  // |compare| determines whether to compare the output blocks from the SIMD
  // implementation, if used, and the C implementation.
  // |saturate| determines whether to set the inputs to maximum values. This is
  // intended primarily as a way to simplify differences in output when
  // debugging.
  void TestAutoRegressiveFilterLuma(int coeff_lag, int param_index,
                                    int num_runs, bool saturate, bool compare);
  LumaAutoRegressionFunc luma_auto_regression_func_;
  LumaAutoRegressionFunc base_luma_auto_regression_func_;
  GrainType luma_block_buffer_[kLumaBlockSize];
  GrainType base_luma_block_buffer_[kLumaBlockSize];
};

// First parameter is coefficient lag. Second parameter is the index into
// |kFilmGrainParams|.
template <int bitdepth>
void AutoRegressionTestLuma<bitdepth>::TestAutoRegressiveFilterLuma(
    int coeff_lag, int param_index, int num_runs, bool saturate, bool compare) {
  if (luma_auto_regression_func_ == nullptr) return;
  // Compare is only needed for NEON tests to compare with C output.
  if (base_luma_auto_regression_func_ == nullptr && compare) return;
  FilmGrainParams params = kFilmGrainParams[param_index];
  params.auto_regression_coeff_lag = coeff_lag;
  const int grain_max = GetGrainMax<bitdepth>();
  for (int y = 0; y < kLumaHeight; ++y) {
    for (int x = 0; x < kLumaWidth; ++x) {
      if (saturate) {
        luma_block_buffer_[y * kLumaWidth + x] = grain_max;
      } else {
        luma_block_buffer_[y * kLumaWidth + x] =
            std::min(x - (kLumaWidth >> 1), y - (kLumaHeight >> 1)) *
            (1 << (bitdepth - 8));
      }
    }
  }

  if (saturate) {
    memset(params.auto_regression_coeff_y, 127,
           sizeof(params.auto_regression_coeff_y));
  }
  if (compare) {
    memcpy(base_luma_block_buffer_, luma_block_buffer_,
           sizeof(luma_block_buffer_));
  }

  const absl::Time start = absl::Now();
  for (int i = 0; i < num_runs; ++i) {
    luma_auto_regression_func_(params, luma_block_buffer_);
  }
  const absl::Duration elapsed_time = absl::Now() - start;
  if (num_runs > 1) {
    printf("AutoRegressionLuma lag=%d, param_index=%d: %d us\n", coeff_lag,
           param_index,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
    return;
  }
  if (compare) {
    base_luma_auto_regression_func_(params, base_luma_block_buffer_);
    EXPECT_TRUE(test_utils::CompareBlocks(
        luma_block_buffer_, base_luma_block_buffer_, kLumaWidth, kLumaHeight,
        kLumaWidth, kLumaWidth, false));
  } else {
    test_utils::CheckMd5Digest(
        "FilmGrain",
        absl::StrFormat("AutoRegressionLuma lag=%d, param_index=%d", coeff_lag,
                        param_index)
            .c_str(),
        GetARTestDigestLuma(bitdepth, coeff_lag, param_index),
        luma_block_buffer_, sizeof(luma_block_buffer_), elapsed_time);
  }
}

using AutoRegressionTestLuma8bpp = AutoRegressionTestLuma<8>;

TEST_P(AutoRegressionTestLuma8bpp, AutoRegressiveFilterLuma) {
  TestAutoRegressiveFilterLuma(std::get<0>(GetParam()), std::get<1>(GetParam()),
                               1, /*saturate=*/false,
                               /*compare=*/false);
}

TEST_P(AutoRegressionTestLuma8bpp, AutoRegressiveFilterLumaSaturated) {
  TestAutoRegressiveFilterLuma(std::get<0>(GetParam()), std::get<1>(GetParam()),
                               1, /*saturate=*/true,
                               /*compare=*/true);
}

TEST_P(AutoRegressionTestLuma8bpp, DISABLED_Speed) {
  TestAutoRegressiveFilterLuma(std::get<0>(GetParam()), std::get<1>(GetParam()),
                               1e5,
                               /*saturate=*/false, /*compare=*/false);
}

#if LIBGAV1_MAX_BITDEPTH >= 10
using AutoRegressionTestLuma10bpp = AutoRegressionTestLuma<10>;

TEST_P(AutoRegressionTestLuma10bpp, AutoRegressiveFilterLuma) {
  TestAutoRegressiveFilterLuma(std::get<0>(GetParam()), std::get<1>(GetParam()),
                               1, /*saturate=*/false,
                               /*compare=*/false);
}

TEST_P(AutoRegressionTestLuma10bpp, AutoRegressiveFilterLumaSaturated) {
  TestAutoRegressiveFilterLuma(std::get<0>(GetParam()), std::get<1>(GetParam()),
                               1, /*saturate=*/true,
                               /*compare=*/true);
}

TEST_P(AutoRegressionTestLuma10bpp, DISABLED_Speed) {
  TestAutoRegressiveFilterLuma(std::get<0>(GetParam()), std::get<1>(GetParam()),
                               1e5,
                               /*saturate=*/false, /*compare=*/false);
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using AutoRegressionTestLuma12bpp = AutoRegressionTestLuma<12>;

TEST_P(AutoRegressionTestLuma12bpp, AutoRegressiveFilterLuma) {
  TestAutoRegressiveFilterLuma(std::get<0>(GetParam()), std::get<1>(GetParam()),
                               1, /*saturate=*/false,
                               /*compare=*/false);
}

TEST_P(AutoRegressionTestLuma12bpp, AutoRegressiveFilterLumaSaturated) {
  TestAutoRegressiveFilterLuma(std::get<0>(GetParam()), std::get<1>(GetParam()),
                               1, /*saturate=*/true,
                               /*compare=*/true);
}

TEST_P(AutoRegressionTestLuma12bpp, DISABLED_Speed) {
  TestAutoRegressiveFilterLuma(std::get<0>(GetParam()), std::get<1>(GetParam()),
                               1e5,
                               /*saturate=*/false, /*compare=*/false);
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

INSTANTIATE_TEST_SUITE_P(
    C, AutoRegressionTestLuma8bpp,
    testing::Combine(testing::Range(1, 4) /* coeff_lag */,
                     testing::Range(0, 10) /* param_index */));
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(
    NEON, AutoRegressionTestLuma8bpp,
    testing::Combine(testing::Range(1, 4) /* coeff_lag */,
                     testing::Range(0, 10) /* param_index */));
#endif

#if LIBGAV1_MAX_BITDEPTH >= 10
INSTANTIATE_TEST_SUITE_P(
    C, AutoRegressionTestLuma10bpp,
    testing::Combine(testing::Range(1, 4) /* coeff_lag */,
                     testing::Range(0, 10) /* param_index */));
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(
    NEON, AutoRegressionTestLuma10bpp,
    testing::Combine(testing::Range(1, 4) /* coeff_lag */,
                     testing::Range(0, 10) /* param_index */));
#endif
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
INSTANTIATE_TEST_SUITE_P(
    C, AutoRegressionTestLuma12bpp,
    testing::Combine(testing::Range(1, 4) /* coeff_lag */,
                     testing::Range(0, 10) /* param_index */));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

struct AutoRegressionChromaTestParam {
  explicit AutoRegressionChromaTestParam(const std::tuple<int, int>& in)
      : coeff_lag(std::get<0>(in)) {
    switch (std::get<1>(in)) {
      case 0:
        subsampling_x = 0;
        subsampling_y = 0;
        break;
      case 1:
        subsampling_x = 1;
        subsampling_y = 0;
        break;
      default:
        assert(std::get<1>(in) == 2);
        subsampling_x = 1;
        subsampling_y = 1;
    }
  }
  const int coeff_lag;
  int subsampling_x;
  int subsampling_y;
};

template <int bitdepth>
class AutoRegressionTestChroma
    : public testing::TestWithParam<std::tuple<int, int>> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  using GrainType =
      typename std::conditional<bitdepth == 8, int8_t, int16_t>::type;

  AutoRegressionTestChroma() {
    AutoRegressionChromaTestParam test_param(GetParam());
    FilmGrainInit_C();
    const dsp::Dsp* const dsp = dsp::GetDspTable(bitdepth);
    // This test suite does not cover num_y_points == 0. This should be covered
    // in the test of the full synthesis process.
    base_chroma_auto_regression_func_ =
        dsp->film_grain.chroma_auto_regression[1][test_param.coeff_lag];

    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const char* const test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
      base_chroma_auto_regression_func_ = nullptr;
    } else if (absl::StartsWith(test_case, "NEON/")) {
#if LIBGAV1_ENABLE_NEON
      FilmGrainInit_NEON();
#endif
    }
    chroma_auto_regression_func_ =
        dsp->film_grain.chroma_auto_regression[1][test_param.coeff_lag];
  }

  ~AutoRegressionTestChroma() override = default;

 protected:
  // |compare| determines whether to compare the output blocks from the SIMD
  // implementation, if used, and the C implementation.
  // |saturate| determines whether to set the inputs to maximum values. This is
  // intended primarily as a way to simplify differences in output when
  // debugging.
  void TestAutoRegressiveFilterChroma(int coeff_lag, int subsampling_x,
                                      int subsampling_y, int num_runs,
                                      bool saturate, bool compare);
  ChromaAutoRegressionFunc chroma_auto_regression_func_;
  ChromaAutoRegressionFunc base_chroma_auto_regression_func_;
  GrainType luma_block_buffer_[kLumaBlockSize];
  GrainType u_block_buffer_[kChromaBlockSize];
  GrainType v_block_buffer_[kChromaBlockSize];
  GrainType base_u_block_buffer_[kChromaBlockSize];
  GrainType base_v_block_buffer_[kChromaBlockSize];
};

template <int bitdepth>
void AutoRegressionTestChroma<bitdepth>::TestAutoRegressiveFilterChroma(
    int coeff_lag, int subsampling_x, int subsampling_y, int num_runs,
    bool saturate, bool compare) {
  if (chroma_auto_regression_func_ == nullptr) return;
  // Compare is only needed for NEON tests to compare with C output.
  if (base_chroma_auto_regression_func_ == nullptr && compare) return;

  // This function relies on the first set of sampled params for basics. The
  // test param generators are used for coverage.
  FilmGrainParams params = kFilmGrainParams[0];
  params.auto_regression_coeff_lag = coeff_lag;
  const int grain_max = GetGrainMax<bitdepth>();
  const int grain_min = GetGrainMin<bitdepth>();
  const int chroma_width =
      (subsampling_x != 0) ? kMinChromaWidth : kMaxChromaWidth;
  const int chroma_height =
      (subsampling_y != 0) ? kMinChromaHeight : kMaxChromaHeight;
  if (saturate) {
    memset(params.auto_regression_coeff_u, 127,
           sizeof(params.auto_regression_coeff_u));
    memset(params.auto_regression_coeff_v, 127,
           sizeof(params.auto_regression_coeff_v));
    for (int y = 0; y < kLumaHeight; ++y) {
      for (int x = 0; x < kLumaWidth; ++x) {
        // This loop relies on the fact that kMaxChromaWidth == kLumaWidth.
        luma_block_buffer_[y * kLumaWidth + x] = grain_max;
        u_block_buffer_[y * kLumaWidth + x] = grain_max;
        v_block_buffer_[y * kLumaWidth + x] = grain_max;
      }
    }
  } else {
    libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
    // Allow any valid grain values.
    const int random_range = grain_max - grain_min + 1;
    for (int y = 0; y < kLumaHeight; ++y) {
      for (int x = 0; x < kLumaWidth; ++x) {
        // This loop relies on the fact that kMaxChromaWidth == kLumaWidth.
        const int random_y = rnd(random_range);
        luma_block_buffer_[y * kLumaWidth + x] = random_y + grain_min;
        const int random_u = rnd(random_range);
        u_block_buffer_[y * kLumaWidth + x] = random_u + grain_min;
        const int random_v = rnd(random_range);
        v_block_buffer_[y * kLumaWidth + x] = random_v + grain_min;
      }
    }
  }
  if (compare) {
    memcpy(base_u_block_buffer_, u_block_buffer_, sizeof(u_block_buffer_));
    memcpy(base_v_block_buffer_, v_block_buffer_, sizeof(v_block_buffer_));
  }

  const absl::Time start = absl::Now();
  for (int i = 0; i < num_runs; ++i) {
    chroma_auto_regression_func_(params, luma_block_buffer_, subsampling_x,
                                 subsampling_y, u_block_buffer_,
                                 v_block_buffer_);
  }
  const absl::Duration elapsed_time = absl::Now() - start;
  if (num_runs > 1) {
    printf("AutoRegressionChroma lag=%d, sub_x=%d, sub_y=%d: %d us\n",
           coeff_lag, subsampling_x, subsampling_y,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
    return;
  }
  if (compare) {
    base_chroma_auto_regression_func_(params, luma_block_buffer_, subsampling_x,
                                      subsampling_y, base_u_block_buffer_,
                                      base_v_block_buffer_);
    EXPECT_TRUE(test_utils::CompareBlocks(u_block_buffer_, base_u_block_buffer_,
                                          chroma_width, chroma_height,
                                          chroma_width, chroma_width, false));
    EXPECT_TRUE(test_utils::CompareBlocks(v_block_buffer_, base_v_block_buffer_,
                                          chroma_width, chroma_height,
                                          chroma_width, chroma_width, false));
  } else {
    test_utils::CheckMd5Digest(
        "FilmGrain",
        absl::StrFormat("AutoRegressionChromaU lag=%d, sub_x=%d, sub_y=%d",
                        coeff_lag, subsampling_x, subsampling_y)
            .c_str(),
        GetARTestDigestChromaU(bitdepth, coeff_lag, subsampling_x,
                               subsampling_y),
        u_block_buffer_, sizeof(u_block_buffer_), elapsed_time);
    test_utils::CheckMd5Digest(
        "FilmGrain",
        absl::StrFormat("AutoRegressionChromaV lag=%d, sub_x=%d, sub_y=%d",
                        coeff_lag, subsampling_x, subsampling_y)
            .c_str(),
        GetARTestDigestChromaV(bitdepth, coeff_lag, subsampling_x,
                               subsampling_y),
        v_block_buffer_, sizeof(v_block_buffer_), elapsed_time);
  }
}

using AutoRegressionTestChroma8bpp = AutoRegressionTestChroma<8>;

TEST_P(AutoRegressionTestChroma8bpp, AutoRegressiveFilterChroma) {
  AutoRegressionChromaTestParam test_param(GetParam());
  TestAutoRegressiveFilterChroma(test_param.coeff_lag, test_param.subsampling_x,
                                 test_param.subsampling_y, 1,
                                 /*saturate=*/false,
                                 /*compare=*/false);
}

TEST_P(AutoRegressionTestChroma8bpp, AutoRegressiveFilterChromaSaturated) {
  AutoRegressionChromaTestParam test_param(GetParam());
  TestAutoRegressiveFilterChroma(test_param.coeff_lag, test_param.subsampling_x,
                                 test_param.subsampling_y, 1, /*saturate=*/true,
                                 /*compare=*/true);
}

TEST_P(AutoRegressionTestChroma8bpp, DISABLED_Speed) {
  AutoRegressionChromaTestParam test_param(GetParam());
  TestAutoRegressiveFilterChroma(
      test_param.coeff_lag, test_param.subsampling_x, test_param.subsampling_y,
      // Subsampling cuts each dimension of the chroma blocks in half, so run
      // twice as many times to compensate.
      1e5 * (1 << (test_param.subsampling_y + test_param.subsampling_x)),
      /*saturate=*/false, /*compare=*/false);
}

#if LIBGAV1_MAX_BITDEPTH >= 10
using AutoRegressionTestChroma10bpp = AutoRegressionTestChroma<10>;

TEST_P(AutoRegressionTestChroma10bpp, AutoRegressiveFilterChroma) {
  AutoRegressionChromaTestParam test_param(GetParam());
  TestAutoRegressiveFilterChroma(test_param.coeff_lag, test_param.subsampling_x,
                                 test_param.subsampling_y, 1,
                                 /*saturate=*/false,
                                 /*compare=*/false);
}

TEST_P(AutoRegressionTestChroma10bpp, AutoRegressiveFilterChromaSaturated) {
  AutoRegressionChromaTestParam test_param(GetParam());
  TestAutoRegressiveFilterChroma(test_param.coeff_lag, test_param.subsampling_x,
                                 test_param.subsampling_y, 1, /*saturate=*/true,
                                 /*compare=*/true);
}

TEST_P(AutoRegressionTestChroma10bpp, DISABLED_Speed) {
  AutoRegressionChromaTestParam test_param(GetParam());
  TestAutoRegressiveFilterChroma(
      test_param.coeff_lag, test_param.subsampling_x, test_param.subsampling_y,
      // Subsampling cuts each dimension of the chroma blocks in half, so run
      // twice as many times to compensate.
      1e5 * (1 << (test_param.subsampling_y + test_param.subsampling_x)),
      /*saturate=*/false, /*compare=*/false);
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using AutoRegressionTestChroma12bpp = AutoRegressionTestChroma<12>;

TEST_P(AutoRegressionTestChroma12bpp, AutoRegressiveFilterChroma) {
  AutoRegressionChromaTestParam test_param(GetParam());
  TestAutoRegressiveFilterChroma(test_param.coeff_lag, test_param.subsampling_x,
                                 test_param.subsampling_y, 1,
                                 /*saturate=*/false,
                                 /*compare=*/false);
}

TEST_P(AutoRegressionTestChroma12bpp, AutoRegressiveFilterChromaSaturated) {
  AutoRegressionChromaTestParam test_param(GetParam());
  TestAutoRegressiveFilterChroma(test_param.coeff_lag, test_param.subsampling_x,
                                 test_param.subsampling_y, 1, /*saturate=*/true,
                                 /*compare=*/true);
}

TEST_P(AutoRegressionTestChroma12bpp, DISABLED_Speed) {
  AutoRegressionChromaTestParam test_param(GetParam());
  TestAutoRegressiveFilterChroma(
      test_param.coeff_lag, test_param.subsampling_x, test_param.subsampling_y,
      // Subsampling cuts each dimension of the chroma blocks in half, so run
      // twice as many times to compensate.
      1e5 * (1 << (test_param.subsampling_y + test_param.subsampling_x)),
      /*saturate=*/false, /*compare=*/false);
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

INSTANTIATE_TEST_SUITE_P(C, AutoRegressionTestChroma8bpp,
                         testing::Combine(testing::Range(0, 4) /* coeff_lag */,
                                          testing::Range(0,
                                                         3) /* subsampling */));

#if LIBGAV1_MAX_BITDEPTH >= 10
INSTANTIATE_TEST_SUITE_P(C, AutoRegressionTestChroma10bpp,
                         testing::Combine(testing::Range(0, 4) /* coeff_lag */,
                                          testing::Range(0,
                                                         3) /* subsampling */));
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
INSTANTIATE_TEST_SUITE_P(C, AutoRegressionTestChroma12bpp,
                         testing::Combine(testing::Range(0, 4) /* coeff_lag */,
                                          testing::Range(0,
                                                         3) /* subsampling */));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, AutoRegressionTestChroma8bpp,
                         testing::Combine(testing::Range(0, 4) /* coeff_lag */,
                                          testing::Range(0,
                                                         3) /* subsampling */));

#if LIBGAV1_MAX_BITDEPTH >= 10
INSTANTIATE_TEST_SUITE_P(NEON, AutoRegressionTestChroma10bpp,
                         testing::Combine(testing::Range(0, 4) /* coeff_lag */,
                                          testing::Range(0,
                                                         3) /* subsampling */));
#endif  // LIBGAV1_MAX_BITDEPTH >= 10
#endif  // LIBGAV1_ENABLE_NEON

template <int bitdepth>
class GrainGenerationTest : public testing::TestWithParam<int> {
 protected:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  using GrainType =
      typename std::conditional<bitdepth == 8, int8_t, int16_t>::type;

  void TestGenerateGrainLuma(int param_index, int num_runs);

  GrainType luma_block_buffer_[kLumaBlockSize];
};

template <int bitdepth>
void GrainGenerationTest<bitdepth>::TestGenerateGrainLuma(int param_index,
                                                          int num_runs) {
  FilmGrainParams params = kFilmGrainParams[param_index];

  const absl::Time start = absl::Now();
  for (int i = 0; i < num_runs; ++i) {
    FilmGrain<bitdepth>::GenerateLumaGrain(params, luma_block_buffer_);
  }
  const absl::Duration elapsed_time = absl::Now() - start;
  if (num_runs == 1) {
    test_utils::CheckMd5Digest(
        "FilmGrain",
        absl::StrFormat("GenerateGrainLuma param_index=%d", param_index)
            .c_str(),
        GetGrainGenerationTestDigestLuma(bitdepth, param_index),
        luma_block_buffer_, sizeof(luma_block_buffer_), elapsed_time);
  } else {
    printf("GenerateGrainLuma param_index=%d: %d us\n", param_index,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  }
}

using GrainGenerationTest8bpp = GrainGenerationTest<8>;

TEST_P(GrainGenerationTest8bpp, GenerateGrainLuma) {
  TestGenerateGrainLuma(GetParam(), 1);
}

TEST_P(GrainGenerationTest8bpp, DISABLED_LumaSpeed) {
  TestGenerateGrainLuma(GetParam(), 1e5);
}

#if LIBGAV1_MAX_BITDEPTH >= 10
using GrainGenerationTest10bpp = GrainGenerationTest<10>;

TEST_P(GrainGenerationTest10bpp, GenerateGrainLuma) {
  TestGenerateGrainLuma(GetParam(), 1);
}

TEST_P(GrainGenerationTest10bpp, DISABLED_LumaSpeed) {
  TestGenerateGrainLuma(GetParam(), 1e5);
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using GrainGenerationTest12bpp = GrainGenerationTest<12>;

TEST_P(GrainGenerationTest12bpp, GenerateGrainLuma) {
  TestGenerateGrainLuma(GetParam(), 1);
}

TEST_P(GrainGenerationTest12bpp, DISABLED_LumaSpeed) {
  TestGenerateGrainLuma(GetParam(), 1e5);
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

INSTANTIATE_TEST_SUITE_P(C, GrainGenerationTest8bpp,
                         testing::Range(0, 10) /* param_index */);

#if LIBGAV1_MAX_BITDEPTH >= 10
INSTANTIATE_TEST_SUITE_P(C, GrainGenerationTest10bpp,
                         testing::Range(0, 10) /* param_index */);
#endif  // LIBGAV1_MAX_BITDEPTH >= 10
#if LIBGAV1_MAX_BITDEPTH == 12
INSTANTIATE_TEST_SUITE_P(C, GrainGenerationTest12bpp,
                         testing::Range(0, 10) /* param_index */);
#endif  // LIBGAV1_MAX_BITDEPTH == 12

// This param type is used for both ConstructStripesTest and
// ConstructImageTest.
struct ConstructNoiseTestParam {
  explicit ConstructNoiseTestParam(const std::tuple<int, int>& in)
      : overlap_flag(std::get<0>(in)) {
    switch (std::get<1>(in)) {
      case 0:
        subsampling_x = 0;
        subsampling_y = 0;
        break;
      case 1:
        subsampling_x = 1;
        subsampling_y = 0;
        break;
      default:
        assert(std::get<1>(in) == 2);
        subsampling_x = 1;
        subsampling_y = 1;
    }
  }
  const int overlap_flag;
  int subsampling_x;
  int subsampling_y;
};

template <int bitdepth>
class ConstructStripesTest
    : public testing::TestWithParam<std::tuple<int, int>> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  using GrainType =
      typename std::conditional<bitdepth == 8, int8_t, int16_t>::type;

  ConstructStripesTest() {
    FilmGrainInit_C();
    const dsp::Dsp* const dsp = dsp::GetDspTable(bitdepth);
    base_construct_noise_stripes_func_ =
        dsp->film_grain.construct_noise_stripes[std::get<0>(GetParam())];

    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const char* const test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
      base_construct_noise_stripes_func_ = nullptr;
    } else if (absl::StartsWith(test_case, "NEON/")) {
#if LIBGAV1_ENABLE_NEON
      FilmGrainInit_NEON();
#endif
    }
    construct_noise_stripes_func_ =
        dsp->film_grain.construct_noise_stripes[std::get<0>(GetParam())];
  }

  ~ConstructStripesTest() override = default;

 protected:
  // |compare| determines whether to compare the output blocks from the SIMD
  // implementation, if used, and the C implementation.
  // |saturate| determines whether to set the inputs to maximum values. This is
  // intended primarily as a way to simplify differences in output when
  // debugging.
  void TestConstructNoiseStripes(int overlap_flag, int subsampling_x,
                                 int subsampling_y, int num_runs, bool saturate,
                                 bool compare);
  ConstructNoiseStripesFunc construct_noise_stripes_func_;
  ConstructNoiseStripesFunc base_construct_noise_stripes_func_;
  GrainType grain_buffer_[kLumaBlockSize];
  Array2DView<GrainType> noise_stripes_;
  // Owns the memory that noise_stripes_ points to.
  std::unique_ptr<GrainType[]> stripe_buffer_;
  Array2DView<GrainType> base_noise_stripes_;
  // Owns the memory that base_stripe_buffer_ points to.
  std::unique_ptr<GrainType[]> base_stripe_buffer_;
};

template <int bitdepth>
void ConstructStripesTest<bitdepth>::TestConstructNoiseStripes(
    int overlap_flag, int subsampling_x, int subsampling_y, int num_runs,
    bool saturate, bool compare) {
  if (construct_noise_stripes_func_ == nullptr) return;
  // Compare is only needed for NEON tests to compare with C output.
  if (base_construct_noise_stripes_func_ == nullptr && compare) return;

  const int stripe_width = ((kFrameWidth + subsampling_x) >> subsampling_x);
  const int stripe_height = kNoiseStripeHeight;
  const int stripe_size = stripe_height * stripe_width;
  const int stripe_buffer_size = stripe_size * kNumTestStripes;
  if (compare) {
    base_stripe_buffer_.reset(new (
        std::nothrow) GrainType[stripe_buffer_size + kNoiseStripePadding]());
    ASSERT_NE(base_stripe_buffer_, nullptr);
    base_noise_stripes_.Reset(kNumTestStripes, stripe_size,
                              base_stripe_buffer_.get());
  }
  stripe_buffer_.reset(
      new (std::nothrow) GrainType[stripe_buffer_size + kNoiseStripePadding]());
  ASSERT_NE(stripe_buffer_, nullptr);
  noise_stripes_.Reset(kNumTestStripes, stripe_size, stripe_buffer_.get());

  const int grain_max = GetGrainMax<bitdepth>();
  const int grain_min = GetGrainMin<bitdepth>();
  if (saturate) {
    for (int y = 0; y < kLumaHeight; ++y) {
      for (int x = 0; x < kLumaWidth; ++x) {
        grain_buffer_[y * kLumaWidth + x] = grain_max;
      }
    }
  } else {
    libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
    // Allow any valid grain values.
    const int random_range = grain_max - grain_min + 1;
    for (int y = 0; y < kLumaHeight; ++y) {
      for (int x = 0; x < kLumaWidth; ++x) {
        grain_buffer_[y * kLumaWidth + x] = grain_min + rnd(random_range);
      }
    }
  }

  const absl::Time start = absl::Now();
  for (int i = 0; i < num_runs; ++i) {
    construct_noise_stripes_func_(grain_buffer_, 68, kFrameWidth, kFrameHeight,
                                  subsampling_x, subsampling_y,
                                  &noise_stripes_);
  }
  const absl::Duration elapsed_time = absl::Now() - start;
  if (num_runs > 1) {
    printf(
        "ConstructNoiseStripes Speed Test for overlap=%d, sub_x=%d, "
        "sub_y=%d: %d us\n",
        overlap_flag, subsampling_x, subsampling_y,
        static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
    return;
  }
  if (compare) {
    base_construct_noise_stripes_func_(grain_buffer_, 68, kFrameWidth,
                                       kFrameHeight, subsampling_x,
                                       subsampling_y, &base_noise_stripes_);

    constexpr int kCompareWidth = 64;
    for (int stripe = 0; stripe < kNumTestStripes;) {
      EXPECT_TRUE(test_utils::CompareBlocks(
          noise_stripes_[stripe], base_noise_stripes_[stripe], kCompareWidth,
          stripe_height, stripe_width, stripe_width, /*check_padding=*/false,
          /*print_diff=*/false));
    }
  } else {
    test_utils::CheckMd5Digest(
        "FilmGrain",
        absl::StrFormat("ConstructNoiseStripes overlap=%d, sub_x=%d, sub_y=%d",
                        overlap_flag, subsampling_x, subsampling_y)
            .c_str(),
        GetConstructStripesTestDigest(bitdepth, overlap_flag, subsampling_x,
                                      subsampling_y),
        noise_stripes_[0], stripe_buffer_size, elapsed_time);
  }
}

using ConstructStripesTest8bpp = ConstructStripesTest<8>;

TEST_P(ConstructStripesTest8bpp, RandomValues) {
  ConstructNoiseTestParam test_params(GetParam());
  TestConstructNoiseStripes(test_params.overlap_flag, test_params.subsampling_x,
                            test_params.subsampling_y, /*num_runs=*/1,
                            /*saturate=*/false, /*compare=*/false);
}

TEST_P(ConstructStripesTest8bpp, SaturatedValues) {
  ConstructNoiseTestParam test_params(GetParam());
  TestConstructNoiseStripes(test_params.overlap_flag, test_params.subsampling_x,
                            test_params.subsampling_y, /*num_runs=*/1,
                            /*saturate=*/true, /*compare=*/true);
}
TEST_P(ConstructStripesTest8bpp, DISABLED_Speed) {
  ConstructNoiseTestParam test_params(GetParam());
  TestConstructNoiseStripes(test_params.overlap_flag, test_params.subsampling_x,
                            test_params.subsampling_y, /*num_runs=*/500,
                            /*saturate=*/false, /*compare=*/false);
}

#if LIBGAV1_MAX_BITDEPTH >= 10
using ConstructStripesTest10bpp = ConstructStripesTest<10>;

TEST_P(ConstructStripesTest10bpp, RandomValues) {
  ConstructNoiseTestParam test_params(GetParam());
  TestConstructNoiseStripes(test_params.overlap_flag, test_params.subsampling_x,
                            test_params.subsampling_y, /*num_runs=*/1,
                            /*saturate=*/false, /*compare=*/false);
}
TEST_P(ConstructStripesTest10bpp, SaturatedValues) {
  ConstructNoiseTestParam test_params(GetParam());
  TestConstructNoiseStripes(test_params.overlap_flag, test_params.subsampling_x,
                            test_params.subsampling_y, /*num_runs=*/1,
                            /*saturate=*/true, /*compare=*/true);
}

TEST_P(ConstructStripesTest10bpp, DISABLED_Speed) {
  ConstructNoiseTestParam test_params(GetParam());
  TestConstructNoiseStripes(test_params.overlap_flag, test_params.subsampling_x,
                            test_params.subsampling_y, /*num_runs=*/500,
                            /*saturate=*/false, /*compare=*/false);
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using ConstructStripesTest12bpp = ConstructStripesTest<12>;

TEST_P(ConstructStripesTest12bpp, RandomValues) {
  ConstructNoiseTestParam test_params(GetParam());
  TestConstructNoiseStripes(test_params.overlap_flag, test_params.subsampling_x,
                            test_params.subsampling_y, /*num_runs=*/1,
                            /*saturate=*/false, /*compare=*/false);
}
TEST_P(ConstructStripesTest12bpp, SaturatedValues) {
  ConstructNoiseTestParam test_params(GetParam());
  TestConstructNoiseStripes(test_params.overlap_flag, test_params.subsampling_x,
                            test_params.subsampling_y, /*num_runs=*/1,
                            /*saturate=*/true, /*compare=*/true);
}

TEST_P(ConstructStripesTest12bpp, DISABLED_Speed) {
  ConstructNoiseTestParam test_params(GetParam());
  TestConstructNoiseStripes(test_params.overlap_flag, test_params.subsampling_x,
                            test_params.subsampling_y, /*num_runs=*/500,
                            /*saturate=*/false, /*compare=*/false);
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

INSTANTIATE_TEST_SUITE_P(C, ConstructStripesTest8bpp,
                         testing::Combine(testing::Range(0, 2),
                                          testing::Range(0, 3)));

#if LIBGAV1_MAX_BITDEPTH >= 10
INSTANTIATE_TEST_SUITE_P(C, ConstructStripesTest10bpp,
                         testing::Combine(testing::Range(0, 2),
                                          testing::Range(0, 3)));
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
INSTANTIATE_TEST_SUITE_P(C, ConstructStripesTest12bpp,
                         testing::Combine(testing::Range(0, 2),
                                          testing::Range(0, 3)));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

template <int bitdepth>
class ConstructImageTest : public testing::TestWithParam<std::tuple<int, int>> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  using GrainType =
      typename std::conditional<bitdepth == 8, int8_t, int16_t>::type;

  ConstructImageTest() {
    FilmGrainInit_C();
    const dsp::Dsp* const dsp = dsp::GetDspTable(bitdepth);
    base_construct_noise_image_overlap_func_ =
        dsp->film_grain.construct_noise_image_overlap;

    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const char* const test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
      base_construct_noise_image_overlap_func_ = nullptr;
    } else if (absl::StartsWith(test_case, "NEON/")) {
#if LIBGAV1_ENABLE_NEON
      FilmGrainInit_NEON();
#endif
    }
    construct_noise_image_overlap_func_ =
        dsp->film_grain.construct_noise_image_overlap;
  }

  ~ConstructImageTest() override = default;

 protected:
  // |compare| determines whether to compare the output blocks from the SIMD
  // implementation, if used, and the C implementation.
  // |saturate| determines whether to set the inputs to maximum values. This is
  // intended primarily as a way to simplify differences in output when
  // debugging.
  void TestConstructNoiseImage(int overlap_flag, int subsampling_x,
                               int subsampling_y, int num_runs, bool saturate,
                               bool compare);
  ConstructNoiseImageOverlapFunc construct_noise_image_overlap_func_;
  ConstructNoiseImageOverlapFunc base_construct_noise_image_overlap_func_;
  Array2DView<GrainType> noise_stripes_;
  // Owns the memory that noise_stripes_ points to.
  std::unique_ptr<GrainType[]> stripe_buffer_;
  Array2D<GrainType> noise_image_;
  Array2D<GrainType> base_noise_image_;
};

template <int bitdepth>
void ConstructImageTest<bitdepth>::TestConstructNoiseImage(
    int overlap_flag, int subsampling_x, int subsampling_y, int num_runs,
    bool saturate, bool compare) {
  if (construct_noise_image_overlap_func_ == nullptr) return;
  // Compare is only needed for NEON tests to compare with C output.
  if (base_construct_noise_image_overlap_func_ == nullptr && compare) return;

  const int image_width = ((kFrameWidth + subsampling_x) >> subsampling_x);
  const int image_height = ((kFrameHeight + subsampling_y) >> subsampling_y);
  const int stripe_height =
      ((kNoiseStripeHeight + subsampling_y) >> subsampling_y);
  const int image_stride = image_width + kNoiseImagePadding;
  const int stripe_size = stripe_height * image_width;
  if (compare) {
    ASSERT_TRUE(base_noise_image_.Reset(image_height, image_stride,
                                        /*zero_initialize=*/false));
  }
  ASSERT_TRUE(noise_image_.Reset(image_height, image_stride,
                                 /*zero_initialize=*/false));
  // Stride between stripe rows is |image_width|. Padding is only at the
  // end of the final row of the final stripe to protect from overreads.
  stripe_buffer_.reset(
      new (std::nothrow)
          GrainType[kNumTestStripes * stripe_size + kNoiseStripePadding]);
  ASSERT_NE(stripe_buffer_, nullptr);
  noise_stripes_.Reset(kNumTestStripes, stripe_size, stripe_buffer_.get());

  const int grain_max = GetGrainMax<bitdepth>();
  const int grain_min = GetGrainMin<bitdepth>();
  if (saturate) {
    for (int i = 0; i < stripe_size; ++i) {
      noise_stripes_[0][i] = grain_max;
    }
    for (int stripe = 1; stripe < kNumTestStripes; ++stripe) {
      memcpy(noise_stripes_[stripe], noise_stripes_[0],
             stripe_size * sizeof(noise_stripes_[0][0]));
    }
  } else {
    libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
    // Allow any valid grain values.
    const int random_range = grain_max - grain_min + 1;
    for (int stripe = 0; stripe < kNumTestStripes; ++stripe) {
      // Assign all allocated memory for this stripe.
      for (int i = 0; i < stripe_height; ++i) {
        for (int x = 0; x < image_width; ++x) {
          noise_stripes_[stripe][i * image_width + x] =
              grain_min + rnd(random_range);
        }
      }
    }
  }

  const absl::Time start = absl::Now();
  for (int i = 0; i < num_runs; ++i) {
    FilmGrain<bitdepth>::ConstructNoiseImage(
        &noise_stripes_, kFrameWidth, kFrameHeight, subsampling_x,
        subsampling_y, overlap_flag << (1 - subsampling_y), &noise_image_);
    if (overlap_flag == 1) {
      construct_noise_image_overlap_func_(&noise_stripes_, kFrameWidth,
                                          kFrameHeight, subsampling_x,
                                          subsampling_y, &noise_image_);
    }
  }

  const absl::Duration elapsed_time = absl::Now() - start;
  if (num_runs > 1) {
    printf(
        "ConstructNoiseImage Speed Test for overlap=%d, sub_x=%d, "
        "sub_y=%d: %d us\n",
        overlap_flag, subsampling_x, subsampling_y,
        static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
    return;
  }
  if (compare) {
    FilmGrain<bitdepth>::ConstructNoiseImage(
        &noise_stripes_, kFrameWidth, kFrameHeight, subsampling_x,
        subsampling_y, overlap_flag << (1 - subsampling_y), &base_noise_image_);
    if (overlap_flag == 1) {
      base_construct_noise_image_overlap_func_(
          &noise_stripes_, kFrameWidth, kFrameHeight, subsampling_x,
          subsampling_y, &base_noise_image_);
    }
    constexpr int kCompareWidth = 72;
    constexpr int kCompareHeight = 72;
    EXPECT_TRUE(test_utils::CompareBlocks(
        noise_image_[0], base_noise_image_[0], kCompareWidth, kCompareHeight,
        image_stride, image_stride, /*check_padding=*/false,
        /*print_diff=*/false));
  } else {
    printf("BD%d \"%s\",\n", bitdepth,
           test_utils::GetMd5Sum(noise_image_[0], image_width, image_height,
                                 image_stride)
               .c_str());
    test_utils::CheckMd5Digest(
        "FilmGrain",
        absl::StrFormat("ConstructNoiseImage overlap=%d, sub_x=%d, sub_y=%d",
                        overlap_flag, subsampling_x, subsampling_y)
            .c_str(),
        GetConstructImageTestDigest(bitdepth, overlap_flag, subsampling_x,
                                    subsampling_y),
        noise_image_[0], image_width, image_height, image_stride, elapsed_time);
  }
}

using ConstructImageTest8bpp = ConstructImageTest<8>;

TEST_P(ConstructImageTest8bpp, RandomValues) {
  ConstructNoiseTestParam test_params(GetParam());
  TestConstructNoiseImage(test_params.overlap_flag, test_params.subsampling_x,
                          test_params.subsampling_y, /*num_runs=*/1,
                          /*saturate=*/false, /*compare=*/false);
}

TEST_P(ConstructImageTest8bpp, SaturatedValues) {
  ConstructNoiseTestParam test_params(GetParam());
  TestConstructNoiseImage(test_params.overlap_flag, test_params.subsampling_x,
                          test_params.subsampling_y, /*num_runs=*/1,
                          /*saturate=*/true, /*compare=*/true);
}

TEST_P(ConstructImageTest8bpp, DISABLED_Speed) {
  ConstructNoiseTestParam test_params(GetParam());
  TestConstructNoiseImage(test_params.overlap_flag, test_params.subsampling_x,
                          test_params.subsampling_y, /*num_runs=*/500,
                          /*saturate=*/false, /*compare=*/false);
}

#if LIBGAV1_MAX_BITDEPTH >= 10
using ConstructImageTest10bpp = ConstructImageTest<10>;

TEST_P(ConstructImageTest10bpp, RandomValues) {
  ConstructNoiseTestParam test_params(GetParam());
  TestConstructNoiseImage(test_params.overlap_flag, test_params.subsampling_x,
                          test_params.subsampling_y, /*num_runs=*/1,
                          /*saturate=*/false, /*compare=*/false);
}

TEST_P(ConstructImageTest10bpp, SaturatedValues) {
  ConstructNoiseTestParam test_params(GetParam());
  TestConstructNoiseImage(test_params.overlap_flag, test_params.subsampling_x,
                          test_params.subsampling_y, /*num_runs=*/1,
                          /*saturate=*/true, /*compare=*/true);
}

TEST_P(ConstructImageTest10bpp, DISABLED_Speed) {
  ConstructNoiseTestParam test_params(GetParam());
  TestConstructNoiseImage(test_params.overlap_flag, test_params.subsampling_x,
                          test_params.subsampling_y, /*num_runs=*/500,
                          /*saturate=*/false, /*compare=*/false);
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using ConstructImageTest12bpp = ConstructImageTest<12>;

TEST_P(ConstructImageTest12bpp, RandomValues) {
  ConstructNoiseTestParam test_params(GetParam());
  TestConstructNoiseImage(test_params.overlap_flag, test_params.subsampling_x,
                          test_params.subsampling_y, /*num_runs=*/1,
                          /*saturate=*/false, /*compare=*/false);
}

TEST_P(ConstructImageTest12bpp, SaturatedValues) {
  ConstructNoiseTestParam test_params(GetParam());
  TestConstructNoiseImage(test_params.overlap_flag, test_params.subsampling_x,
                          test_params.subsampling_y, /*num_runs=*/1,
                          /*saturate=*/true, /*compare=*/true);
}

TEST_P(ConstructImageTest12bpp, DISABLED_Speed) {
  ConstructNoiseTestParam test_params(GetParam());
  TestConstructNoiseImage(test_params.overlap_flag, test_params.subsampling_x,
                          test_params.subsampling_y, /*num_runs=*/500,
                          /*saturate=*/false, /*compare=*/false);
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

INSTANTIATE_TEST_SUITE_P(C, ConstructImageTest8bpp,
                         testing::Combine(testing::Range(0, 2),
                                          testing::Range(0, 3)));

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, ConstructImageTest8bpp,
                         testing::Combine(testing::Range(0, 2),
                                          testing::Range(0, 3)));
#endif  // LIBGAV1_ENABLE_NEON

#if LIBGAV1_MAX_BITDEPTH >= 10
INSTANTIATE_TEST_SUITE_P(C, ConstructImageTest10bpp,
                         testing::Combine(testing::Range(0, 2),
                                          testing::Range(0, 3)));
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
INSTANTIATE_TEST_SUITE_P(C, ConstructImageTest12bpp,
                         testing::Combine(testing::Range(0, 2),
                                          testing::Range(0, 3)));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

template <int bitdepth>
class ScalingLookupTableTest : public testing::TestWithParam<int> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  ScalingLookupTableTest() {
    test_utils::ResetDspTable(bitdepth);
    FilmGrainInit_C();
    const dsp::Dsp* const dsp = dsp::GetDspTable(bitdepth);

    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const char* const test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "NEON/")) {
#if LIBGAV1_ENABLE_NEON
      FilmGrainInit_NEON();
#endif
    }
    initialize_func_ = dsp->film_grain.initialize_scaling_lut;
  }
  ~ScalingLookupTableTest() override = default;

 protected:
  void TestSpeed(int num_runs);
  void ZeroPoints();

 private:
  static constexpr int kScalingLutBufferLength =
      (kScalingLookupTableSize + kScalingLookupTablePadding) << (bitdepth - 8);
  dsp::InitializeScalingLutFunc initialize_func_;
  int16_t scaling_lut_[kScalingLutBufferLength];
};

template <int bitdepth>
void ScalingLookupTableTest<bitdepth>::TestSpeed(int num_runs) {
  if (initialize_func_ == nullptr) return;
  const int param_index = GetParam();
  const FilmGrainParams& params = kFilmGrainParams[param_index];
  const absl::Time start = absl::Now();
  Memset(scaling_lut_, 0, kScalingLutBufferLength);
  for (int i = 0; i < num_runs; ++i) {
    initialize_func_(params.num_y_points, params.point_y_value,
                     params.point_y_scaling, scaling_lut_,
                     kScalingLutBufferLength);
  }
  const absl::Duration elapsed_time = absl::Now() - start;
  if (num_runs > 1) {
    printf("InitializeScalingLut: %d us\n",
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
    return;
  }
  test_utils::CheckMd5Digest(
      "FilmGrain",
      absl::StrFormat("InitializeScalingLut for param set: %d", param_index)
          .c_str(),
      GetScalingInitTestDigest(param_index, bitdepth), scaling_lut_,
      (sizeof(scaling_lut_[0]) * kScalingLookupTableSize) << (bitdepth - 8),
      elapsed_time);
}

template <int bitdepth>
void ScalingLookupTableTest<bitdepth>::ZeroPoints() {
  if (initialize_func_ == nullptr) return;
  const int param_index = GetParam();
  const FilmGrainParams& params = kFilmGrainParams[param_index];
  initialize_func_(0, params.point_y_value, params.point_y_scaling,
                   scaling_lut_, kScalingLookupTableSize);
  for (int i = 0; i < kScalingLookupTableSize; ++i) {
    ASSERT_EQ(scaling_lut_[i], 0);
  }
}

using ScalingLookupTableTest8bpp = ScalingLookupTableTest<8>;

TEST_P(ScalingLookupTableTest8bpp, ZeroPoints) { ZeroPoints(); }

TEST_P(ScalingLookupTableTest8bpp, Correctness) { TestSpeed(/*num_runs=*/1); }

TEST_P(ScalingLookupTableTest8bpp, DISABLED_Speed) {
  TestSpeed(/*num_runs=*/1e5);
}

#if LIBGAV1_MAX_BITDEPTH >= 10
using ScalingLookupTableTest10bpp = ScalingLookupTableTest<10>;

TEST_P(ScalingLookupTableTest10bpp, ZeroPoints) { ZeroPoints(); }

TEST_P(ScalingLookupTableTest10bpp, Correctness) { TestSpeed(/*num_runs=*/1); }

TEST_P(ScalingLookupTableTest10bpp, DISABLED_Speed) {
  TestSpeed(/*num_runs=*/1e5);
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using ScalingLookupTableTest12bpp = ScalingLookupTableTest<12>;

TEST_P(ScalingLookupTableTest12bpp, ZeroPoints) { ZeroPoints(); }

TEST_P(ScalingLookupTableTest12bpp, Correctness) { TestSpeed(/*num_runs=*/1); }

TEST_P(ScalingLookupTableTest12bpp, DISABLED_Speed) {
  TestSpeed(/*num_runs=*/1e5);
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

INSTANTIATE_TEST_SUITE_P(C, ScalingLookupTableTest8bpp,
                         testing::Range(0, kNumFilmGrainTestParams));

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, ScalingLookupTableTest8bpp,
                         testing::Range(0, kNumFilmGrainTestParams));
#endif

#if LIBGAV1_MAX_BITDEPTH >= 10
INSTANTIATE_TEST_SUITE_P(C, ScalingLookupTableTest10bpp,
                         testing::Range(0, kNumFilmGrainTestParams));

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, ScalingLookupTableTest10bpp,
                         testing::Range(0, kNumFilmGrainTestParams));
#endif
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
INSTANTIATE_TEST_SUITE_P(C, ScalingLookupTableTest12bpp,
                         testing::Range(0, kNumFilmGrainTestParams));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

struct BlendNoiseTestParam {
  explicit BlendNoiseTestParam(const std::tuple<int, int>& in)
      : chroma_scaling_from_luma(std::get<0>(in)) {
    switch (std::get<1>(in)) {
      case 0:
        subsampling_x = 0;
        subsampling_y = 0;
        break;
      case 1:
        subsampling_x = 1;
        subsampling_y = 0;
        break;
      default:
        assert(std::get<1>(in) == 2);
        subsampling_x = 1;
        subsampling_y = 1;
    }
  }
  const int chroma_scaling_from_luma;
  int subsampling_x;
  int subsampling_y;
};

template <int bitdepth, typename Pixel>
class BlendNoiseTest : public testing::TestWithParam<std::tuple<int, int>> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  using GrainType =
      typename std::conditional<bitdepth == 8, int8_t, int16_t>::type;

  BlendNoiseTest() {
    test_utils::ResetDspTable(bitdepth);
    FilmGrainInit_C();
    const dsp::Dsp* const dsp = dsp::GetDspTable(bitdepth);

    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const char* const test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "NEON/")) {
#if LIBGAV1_ENABLE_NEON
      FilmGrainInit_NEON();
#endif
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      FilmGrainInit_SSE4_1();
    }
    const BlendNoiseTestParam test_param(GetParam());
    chroma_scaling_from_luma_ = test_param.chroma_scaling_from_luma;
    blend_luma_func_ = dsp->film_grain.blend_noise_luma;
    blend_chroma_func_ =
        dsp->film_grain.blend_noise_chroma[chroma_scaling_from_luma_];
    subsampling_x_ = test_param.subsampling_x;
    subsampling_y_ = test_param.subsampling_y;

    uv_width_ = (width_ + subsampling_x_) >> subsampling_x_;
    uv_height_ = (height_ + subsampling_y_) >> subsampling_y_;
    uv_stride_ = uv_width_ * sizeof(Pixel);
    y_stride_ = width_ * sizeof(Pixel);
    const size_t buffer_size =
        sizeof(Pixel) * (width_ * height_ + 2 * uv_width_ * uv_height_ +
                         3 * kBorderPixelsFilmGrain);
    source_buffer_.reset(new (std::nothrow) uint8_t[buffer_size]);
    memset(source_buffer_.get(), 0, sizeof(source_buffer_[0]) * buffer_size);
    dest_buffer_.reset(new (std::nothrow) uint8_t[buffer_size]);
    memset(dest_buffer_.get(), 0, sizeof(dest_buffer_[0]) * buffer_size);
    source_plane_y_ = source_buffer_.get();
    source_plane_u_ =
        source_plane_y_ + y_stride_ * height_ + kBorderPixelsFilmGrain;
    source_plane_v_ =
        source_plane_u_ + uv_stride_ * uv_height_ + kBorderPixelsFilmGrain;
    dest_plane_y_ = dest_buffer_.get();
    dest_plane_u_ =
        dest_plane_y_ + y_stride_ * height_ + kBorderPixelsFilmGrain;
    dest_plane_v_ =
        dest_plane_u_ + uv_stride_ * uv_height_ + kBorderPixelsFilmGrain;
  }
  ~BlendNoiseTest() override = default;

 protected:
  void TestSpeed(int num_runs);

 private:
  static constexpr int kScalingLutBufferLength =
      (kScalingLookupTableSize + kScalingLookupTablePadding) << 2;

  void ConvertScalingLut10bpp(int16_t* scaling_lut_10bpp,
                              const int16_t* src_scaling_lut);
  dsp::BlendNoiseWithImageLumaFunc blend_luma_func_;
  dsp::BlendNoiseWithImageChromaFunc blend_chroma_func_;

  const int width_ = 1921;
  const int height_ = 1081;
  int chroma_scaling_from_luma_ = 0;
  int subsampling_x_ = 0;
  int subsampling_y_ = 0;
  int uv_width_ = 0;
  int uv_height_ = 0;
  int uv_stride_ = 0;
  int y_stride_ = 0;
  // This holds the data that |source_plane_y_|, |source_plane_u_|, and
  // |source_plane_v_| point to.
  std::unique_ptr<uint8_t[]> source_buffer_;
  // This holds the data that |dest_plane_y_|, |dest_plane_u_|, and
  // |dest_plane_v_| point to.
  std::unique_ptr<uint8_t[]> dest_buffer_;
  uint8_t* source_plane_y_ = nullptr;
  uint8_t* source_plane_u_ = nullptr;
  uint8_t* source_plane_v_ = nullptr;
  uint8_t* dest_plane_y_ = nullptr;
  uint8_t* dest_plane_u_ = nullptr;
  uint8_t* dest_plane_v_ = nullptr;
  Array2D<GrainType> noise_image_[kMaxPlanes];
  int16_t scaling_lut_10bpp_y_[kScalingLutBufferLength];
  int16_t scaling_lut_10bpp_u_[kScalingLutBufferLength];
  int16_t scaling_lut_10bpp_v_[kScalingLutBufferLength];
};

template <int bitdepth, typename Pixel>
void BlendNoiseTest<bitdepth, Pixel>::ConvertScalingLut10bpp(
    int16_t* scaling_lut_10bpp, const int16_t* src_scaling_lut) {
  for (int i = 0; i < kScalingLookupTableSize - 1; ++i) {
    const int x_base = i << 2;
    const int start = src_scaling_lut[i];
    const int end_index = std::min(i + 1, kScalingLookupTableSize - 1);
    const int end = src_scaling_lut[end_index];
    const int delta = end - start;
    scaling_lut_10bpp[x_base] = start;
    scaling_lut_10bpp[x_base + 1] = start + RightShiftWithRounding(delta, 2);
    scaling_lut_10bpp[x_base + 2] =
        start + RightShiftWithRounding(2 * delta, 2);
    scaling_lut_10bpp[x_base + 3] =
        start + RightShiftWithRounding(3 * delta, 2);
  }
}

template <int bitdepth, typename Pixel>
void BlendNoiseTest<bitdepth, Pixel>::TestSpeed(const int num_runs) {
  if (blend_chroma_func_ == nullptr || blend_luma_func_ == nullptr) return;
  ASSERT_TRUE(noise_image_[kPlaneY].Reset(height_,
                                          width_ + kBorderPixelsFilmGrain,
                                          /*zero_initialize=*/false));
  ASSERT_TRUE(noise_image_[kPlaneU].Reset(uv_height_,
                                          uv_width_ + kBorderPixelsFilmGrain,
                                          /*zero_initialize=*/false));
  ASSERT_TRUE(noise_image_[kPlaneV].Reset(uv_height_,
                                          uv_width_ + kBorderPixelsFilmGrain,
                                          /*zero_initialize=*/false));
  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
  // Allow any valid grain values.
  const int grain_max = GetGrainMax<bitdepth>();
  const int grain_min = GetGrainMin<bitdepth>();
  const int random_range = grain_max - grain_min + 1;
  auto* src_y = reinterpret_cast<Pixel*>(source_plane_y_);
  auto* src_u = reinterpret_cast<Pixel*>(source_plane_u_);
  auto* src_v = reinterpret_cast<Pixel*>(source_plane_v_);
  for (int y = 0; y < height_; ++y) {
    for (int x = 0; x < width_; ++x) {
      const int random_source_y = rnd(random_range);
      // Populating the luma source ensures the lookup table is tested. Chroma
      // planes are given identical values. Giving them different values would
      // artificially differentiate the outputs. It's important that the test
      // expect that different outputs are caused by the different scaling
      // lookup tables, rather than by different inputs.
      const int uv_y_pos = y >> subsampling_y_;
      const int uv_x_pos = x >> subsampling_x_;
      src_y[y * width_ + x] = random_source_y;
      src_u[uv_y_pos * uv_width_ + uv_x_pos] = random_source_y;
      src_v[uv_y_pos * uv_width_ + uv_x_pos] = random_source_y;
      const int random_y = rnd(random_range);
      noise_image_[kPlaneY][y][x] = random_y + grain_min;
      const int random_u = rnd(random_range);
      noise_image_[kPlaneU][uv_y_pos][uv_x_pos] = random_u + grain_min;
      const int random_v = rnd(random_range);
      noise_image_[kPlaneV][uv_y_pos][uv_x_pos] = random_v + grain_min;
    }
  }
  static constexpr int16_t kTestScalingLutY[kScalingLookupTableSize] = {
      72,  72,  72,  72,  72,  72,  72,  72,  72,  72,  72,  72,  72,  72,  73,
      75,  76,  77,  79,  80,  81,  83,  84,  86,  87,  88,  90,  91,  92,  92,
      93,  93,  94,  95,  95,  96,  97,  97,  98,  98,  99,  99,  99,  99,  98,
      98,  98,  98,  98,  98,  98,  97,  97,  97,  97,  97,  97,  97,  97,  97,
      97,  98,  98,  98,  98,  98,  98,  98,  98,  98,  98,  98,  98,  98,  98,
      99,  99,  99,  99,  99,  99,  99,  99,  99,  99,  99,  99,  99,  100, 100,
      100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
      101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101,
      101, 101, 101, 101, 101, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102,
  };
  static constexpr int16_t kTestScalingLutU[kScalingLookupTableSize] = {
      30,  42,  53,  65,  74,  74,  74,  74,  74,  74,  74,  74,  74,  74,  74,
      75,  76,  78,  79,  81,  82,  83,  85,  86,  88,  89,  91,  92,  93,  93,
      94,  94,  95,  95,  96,  96,  97,  97,  98,  98,  99,  99,  99,  99,  99,
      99,  99,  99,  98,  98,  98,  98,  98,  98,  98,  98,  98,  98,  98,  98,
      98,  98,  98,  98,  98,  98,  98,  98,  98,  98,  98,  98,  98,  98,  98,
      98,  98,  98,  98,  98,  98,  98,  98,  98,  98,  98,  98,  98,  99,  99,
      99,  99,  99,  99,  99,  99,  99,  99,  99,  99,  99,  99,  99,  99,  99,
      99,  99,  99,  99,  99,  99,  99,  99,  99,  99,  99,  99,  99,  99,  99,
      99,  99,  99,  99,  99,  99,  100, 100, 100, 100, 100, 100, 100, 100, 100,
      100, 100, 100, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120,
      110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110,
      98,  98,  98,  98,  98,  98,  98,  97,  97,  97,  97,  97,  97,  97,  97,
      97,  97,  97,  97,  97,  97,  97,  97,  97,  97,  97,  97,  97,  97,  97,
      97,  97,  97,  97,  97,  97,  97,  97,  97,  97,  96,  96,  96,  96,  96,
      96,  96,  96,  96,  96,  96,  96,  96,  96,  96,  96,  96,  96,  96,  96,
      96,  96,  96,  96,  96,  96,  96,  96,  96,  96,  96,  96,  96,  96,  95,
      95,  95,  95,  95,  95,  95,  95,  95,  95,  95,  95,  95,  95,  95,  95,
      95,  95,
  };
  static constexpr int16_t kTestScalingLutV[kScalingLookupTableSize] = {
      73,  73,  73,  73,  73,  73,  73,  73,  73,  73,  73,  73,  74,  74,  74,
      75,  75,  78,  79,  81,  82,  83,  85,  86,  88,  89,  91,  92,  93,  93,
      94,  94,  95,  95,  96,  96,  97,  97,  98,  98,  99,  99,  99,  99,  98,
      98,  98,  98,  98,  98,  98,  97,  97,  97,  97,  97,  97,  97,  97,  97,
      97,  97,  97,  98,  98,  98,  98,  98,  98,  98,  98,  98,  98,  98,  98,
      98,  98,  98,  98,  98,  99,  99,  99,  99,  99,  99,  99,  99,  99,  99,
      99,  99,  99,  99,  99,  99,  100, 100, 100, 100, 100, 100, 100, 100, 100,
      100, 100, 100, 100, 100, 100, 100, 100, 101, 101, 101, 101, 101, 101, 101,
      101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101,
      101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101,
      101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101,
      101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101,
      101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101,
      101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101,
      150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150,
      180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 180,
      200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200,
      255, 255,
  };

  if (bitdepth == 10) {
    for (int i = 0; i < kScalingLutBufferLength; ++i) {
      ConvertScalingLut10bpp(scaling_lut_10bpp_y_, kTestScalingLutY);
      ConvertScalingLut10bpp(scaling_lut_10bpp_u_, kTestScalingLutU);
      ConvertScalingLut10bpp(scaling_lut_10bpp_v_, kTestScalingLutV);
    }
  }
  const FilmGrainParams& params = kFilmGrainParams[0];
  const int min_value = 16 << (bitdepth - 8);
  const int max_value = 235 << (bitdepth - 8);
  const absl::Time start = absl::Now();
  for (int i = 0; i < num_runs; ++i) {
    if (chroma_scaling_from_luma_) {
      blend_chroma_func_(
          kPlaneU, params, noise_image_, min_value, max_value, width_, height_,
          /*start_height=*/0, subsampling_x_, subsampling_y_,
          (bitdepth == 10) ? scaling_lut_10bpp_y_ : kTestScalingLutY,
          source_plane_y_, y_stride_, source_plane_u_, uv_stride_,
          dest_plane_u_, uv_stride_);
      blend_chroma_func_(
          kPlaneV, params, noise_image_, min_value, max_value, width_, height_,
          /*start_height=*/0, subsampling_x_, subsampling_y_,
          (bitdepth == 10) ? scaling_lut_10bpp_y_ : kTestScalingLutY,
          source_plane_y_, y_stride_, source_plane_v_, uv_stride_,
          dest_plane_v_, uv_stride_);
    } else {
      blend_chroma_func_(
          kPlaneU, params, noise_image_, min_value, max_value, width_, height_,
          /*start_height=*/0, subsampling_x_, subsampling_y_,
          (bitdepth == 10) ? scaling_lut_10bpp_u_ : kTestScalingLutU,
          source_plane_y_, y_stride_, source_plane_u_, uv_stride_,
          dest_plane_u_, uv_stride_);
      blend_chroma_func_(
          kPlaneV, params, noise_image_, min_value, max_value, width_, height_,
          /*start_height=*/0, subsampling_x_, subsampling_y_,
          (bitdepth == 10) ? scaling_lut_10bpp_v_ : kTestScalingLutV,
          source_plane_y_, y_stride_, source_plane_v_, uv_stride_,
          dest_plane_v_, uv_stride_);
    }
    blend_luma_func_(noise_image_, min_value, max_value, params.chroma_scaling,
                     width_, height_, /*start_height=*/0,
                     (bitdepth == 10) ? scaling_lut_10bpp_y_ : kTestScalingLutY,
                     source_plane_y_, y_stride_, dest_plane_y_, y_stride_);
  }
  const absl::Duration elapsed_time = absl::Now() - start;
  const char* digest_luma = GetBlendLumaTestDigest(bitdepth);
  printf("YBD%d \"%s\",\n", bitdepth,
         test_utils::GetMd5Sum(dest_plane_y_, y_stride_ * height_).c_str());
  printf("UBD%d \"%s\",\n", bitdepth,
         test_utils::GetMd5Sum(dest_plane_u_, uv_stride_ * uv_height_).c_str());
  printf("VBD%d \"%s\",\n", bitdepth,
         test_utils::GetMd5Sum(dest_plane_v_, uv_stride_ * uv_height_).c_str());
  test_utils::CheckMd5Digest(
      "BlendNoiseWithImage",
      absl::StrFormat("Luma cfl=%d, sub_x=%d, sub_y=%d",
                      chroma_scaling_from_luma_, subsampling_x_, subsampling_y_)
          .c_str(),
      digest_luma, dest_plane_y_, y_stride_ * height_, elapsed_time);
  const char* digest_chroma_u = GetBlendChromaUTestDigest(
      bitdepth, chroma_scaling_from_luma_, subsampling_x_, subsampling_y_);
  test_utils::CheckMd5Digest(
      "BlendNoiseWithImage",
      absl::StrFormat("ChromaU cfl=%d, sub_x=%d, sub_y=%d",
                      chroma_scaling_from_luma_, subsampling_x_, subsampling_y_)
          .c_str(),
      digest_chroma_u, dest_plane_u_, uv_stride_ * uv_height_, elapsed_time);
  const char* digest_chroma_v = GetBlendChromaVTestDigest(
      bitdepth, chroma_scaling_from_luma_, subsampling_x_, subsampling_y_);
  test_utils::CheckMd5Digest(
      "BlendNoiseWithImage",
      absl::StrFormat("ChromaV cfl=%d, sub_x=%d, sub_y=%d",
                      chroma_scaling_from_luma_, subsampling_x_, subsampling_y_)
          .c_str(),
      digest_chroma_v, dest_plane_v_, uv_stride_ * uv_height_, elapsed_time);
}

using BlendNoiseTest8bpp = BlendNoiseTest<8, uint8_t>;

TEST_P(BlendNoiseTest8bpp, MatchesOriginalOutput) { TestSpeed(1); }

TEST_P(BlendNoiseTest8bpp, DISABLED_Speed) { TestSpeed(kNumSpeedTests); }

INSTANTIATE_TEST_SUITE_P(C, BlendNoiseTest8bpp,
                         testing::Combine(testing::Range(0, 2),
                                          testing::Range(0, 3)));
#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, BlendNoiseTest8bpp,
                         testing::Combine(testing::Range(0, 2),
                                          testing::Range(0, 3)));
#endif

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, BlendNoiseTest8bpp,
                         testing::Combine(testing::Range(0, 2),
                                          testing::Range(0, 3)));
#endif

#if LIBGAV1_MAX_BITDEPTH >= 10
using BlendNoiseTest10bpp = BlendNoiseTest<10, uint16_t>;

TEST_P(BlendNoiseTest10bpp, MatchesOriginalOutput) { TestSpeed(1); }

TEST_P(BlendNoiseTest10bpp, DISABLED_Speed) { TestSpeed(kNumSpeedTests); }

INSTANTIATE_TEST_SUITE_P(C, BlendNoiseTest10bpp,
                         testing::Combine(testing::Range(0, 2),
                                          testing::Range(0, 3)));
#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, BlendNoiseTest10bpp,
                         testing::Combine(testing::Range(0, 2),
                                          testing::Range(0, 3)));
#endif

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, BlendNoiseTest10bpp,
                         testing::Combine(testing::Range(0, 2),
                                          testing::Range(0, 3)));
#endif
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using BlendNoiseTest12bpp = BlendNoiseTest<12, uint16_t>;

TEST_P(BlendNoiseTest12bpp, MatchesOriginalOutput) { TestSpeed(1); }

TEST_P(BlendNoiseTest12bpp, DISABLED_Speed) { TestSpeed(kNumSpeedTests); }

INSTANTIATE_TEST_SUITE_P(C, BlendNoiseTest12bpp,
                         testing::Combine(testing::Range(0, 2),
                                          testing::Range(0, 3)));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

template <int bitdepth, typename Pixel>
class FilmGrainSpeedTest : public testing::TestWithParam<int> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  FilmGrainSpeedTest() {
    test_utils::ResetDspTable(bitdepth);
    FilmGrainInit_C();

    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const char* const test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "NEON/")) {
#if LIBGAV1_ENABLE_NEON
      FilmGrainInit_NEON();
#endif
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      FilmGrainInit_SSE4_1();
    }
    uv_width_ = (width_ + subsampling_x_) >> subsampling_x_;
    uv_height_ = (height_ + subsampling_y_) >> subsampling_y_;
    uv_stride_ = uv_width_ * sizeof(Pixel);
    y_stride_ = width_ * sizeof(Pixel);
    const size_t buffer_size =
        sizeof(Pixel) * (width_ * height_ + 2 * uv_width_ * uv_height_);
    source_buffer_.reset(new (std::nothrow) uint8_t[buffer_size]);
    memset(source_buffer_.get(), 0, sizeof(source_buffer_[0]) * buffer_size);
    dest_buffer_.reset(new (std::nothrow) uint8_t[buffer_size]);
    memset(dest_buffer_.get(), 0, sizeof(dest_buffer_[0]) * buffer_size);
    source_plane_y_ = source_buffer_.get();
    source_plane_u_ = source_plane_y_ + y_stride_ * height_;
    source_plane_v_ = source_plane_u_ + uv_stride_ * uv_height_;
    dest_plane_y_ = dest_buffer_.get();
    dest_plane_u_ = dest_plane_y_ + y_stride_ * height_;
    dest_plane_v_ = dest_plane_u_ + uv_stride_ * uv_height_;
    const int num_threads = GetParam();
    thread_pool_ = ThreadPool::Create(num_threads);
  }
  ~FilmGrainSpeedTest() override = default;

 protected:
  void TestSpeed(int num_runs);

 private:
  const int width_ = 1920;
  const int height_ = 1080;
  const int subsampling_x_ = 1;
  const int subsampling_y_ = 1;
  int uv_width_ = 0;
  int uv_height_ = 0;
  int uv_stride_ = 0;
  int y_stride_ = 0;
  std::unique_ptr<uint8_t[]> source_buffer_;
  std::unique_ptr<uint8_t[]> dest_buffer_;
  const uint8_t* source_plane_y_ = nullptr;
  const uint8_t* source_plane_u_ = nullptr;
  const uint8_t* source_plane_v_ = nullptr;
  uint8_t* dest_plane_y_ = nullptr;
  uint8_t* dest_plane_u_ = nullptr;
  uint8_t* dest_plane_v_ = nullptr;
  std::unique_ptr<ThreadPool> thread_pool_;
};

// Each run of the speed test adds film grain noise to 10 dummy frames. The
// film grain parameters for the 10 frames were generated with aomenc.
template <int bitdepth, typename Pixel>
void FilmGrainSpeedTest<bitdepth, Pixel>::TestSpeed(const int num_runs) {
  const dsp::Dsp* dsp = GetDspTable(bitdepth);
  if (dsp->film_grain.blend_noise_chroma[0] == nullptr ||
      dsp->film_grain.blend_noise_luma == nullptr) {
    return;
  }
  for (int k = 0; k < kNumFilmGrainTestParams; ++k) {
    const FilmGrainParams& params = kFilmGrainParams[k];
    const absl::Time start = absl::Now();
    for (int i = 0; i < num_runs; ++i) {
      FilmGrain<bitdepth> film_grain(params, /*is_monochrome=*/false,
                                     /*color_matrix_is_identity=*/false,
                                     subsampling_x_, subsampling_y_, width_,
                                     height_, thread_pool_.get());
      EXPECT_TRUE(film_grain.AddNoise(
          source_plane_y_, y_stride_, source_plane_u_, source_plane_v_,
          uv_stride_, dest_plane_y_, y_stride_, dest_plane_u_, dest_plane_v_,
          uv_stride_));
    }
    const absl::Duration elapsed_time = absl::Now() - start;
    const char* digest_luma = GetTestDigestLuma(bitdepth, k);
    test_utils::CheckMd5Digest(
        "FilmGrainSynthesisLuma",
        absl::StrFormat("kFilmGrainParams[%d]", k).c_str(), digest_luma,
        dest_plane_y_, y_stride_ * height_, elapsed_time);
    const char* digest_chroma_u = GetTestDigestChromaU(bitdepth, k);
    test_utils::CheckMd5Digest(
        "FilmGrainSynthesisChromaU",
        absl::StrFormat("kFilmGrainParams[%d]", k).c_str(), digest_chroma_u,
        dest_plane_u_, uv_stride_ * uv_height_, elapsed_time);
    const char* digest_chroma_v = GetTestDigestChromaV(bitdepth, k);
    test_utils::CheckMd5Digest(
        "FilmGrainSynthesisChromaV",
        absl::StrFormat("kFilmGrainParams[%d]", k).c_str(), digest_chroma_v,
        dest_plane_v_, uv_stride_ * uv_height_, elapsed_time);
  }
}

using FilmGrainSpeedTest8bpp = FilmGrainSpeedTest<8, uint8_t>;

TEST_P(FilmGrainSpeedTest8bpp, MatchesOriginalOutput) { TestSpeed(1); }

TEST_P(FilmGrainSpeedTest8bpp, DISABLED_Speed) { TestSpeed(kNumSpeedTests); }

INSTANTIATE_TEST_SUITE_P(C, FilmGrainSpeedTest8bpp, testing::Values(0, 3, 8));

#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, FilmGrainSpeedTest8bpp,
                         testing::Values(0, 3, 8));
#endif

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, FilmGrainSpeedTest8bpp,
                         testing::Values(0, 3, 8));
#endif

#if LIBGAV1_MAX_BITDEPTH >= 10
using FilmGrainSpeedTest10bpp = FilmGrainSpeedTest<10, uint16_t>;

TEST_P(FilmGrainSpeedTest10bpp, MatchesOriginalOutput) { TestSpeed(1); }

TEST_P(FilmGrainSpeedTest10bpp, DISABLED_Speed) { TestSpeed(kNumSpeedTests); }

INSTANTIATE_TEST_SUITE_P(C, FilmGrainSpeedTest10bpp, testing::Values(0, 3, 8));

#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, FilmGrainSpeedTest10bpp,
                         testing::Values(0, 3, 8));
#endif

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, FilmGrainSpeedTest10bpp,
                         testing::Values(0, 3, 8));
#endif

#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using FilmGrainSpeedTest12bpp = FilmGrainSpeedTest<12, uint16_t>;

TEST_P(FilmGrainSpeedTest12bpp, MatchesOriginalOutput) { TestSpeed(1); }

TEST_P(FilmGrainSpeedTest12bpp, DISABLED_Speed) { TestSpeed(kNumSpeedTests); }

INSTANTIATE_TEST_SUITE_P(C, FilmGrainSpeedTest12bpp, testing::Values(0, 3, 8));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace
}  // namespace film_grain
}  // namespace dsp
}  // namespace libgav1
