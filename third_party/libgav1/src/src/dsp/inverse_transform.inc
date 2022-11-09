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

// Constants and utility functions used for inverse transform implementations.
// This will be included inside an anonymous namespace on files where these are
// necessary.

// The value at index i is derived as: round(cos(pi * i / 128) * (1 << 12)).
constexpr int16_t kCos128[65] = {
    4096, 4095, 4091, 4085, 4076, 4065, 4052, 4036, 4017, 3996, 3973,
    3948, 3920, 3889, 3857, 3822, 3784, 3745, 3703, 3659, 3612, 3564,
    3513, 3461, 3406, 3349, 3290, 3229, 3166, 3102, 3035, 2967, 2896,
    2824, 2751, 2675, 2598, 2520, 2440, 2359, 2276, 2191, 2106, 2019,
    1931, 1842, 1751, 1660, 1567, 1474, 1380, 1285, 1189, 1092, 995,
    897,  799,  700,  601,  501,  401,  301,  201,  101,  0};

inline int16_t Cos128(int angle) {
  angle &= 0xff;

  // If |angle| is 128, this function returns -4096 (= -2^12), which will
  // cause the 32-bit multiplications in ButterflyRotation() to overflow if
  // dst[a] or dst[b] is -2^19 (a possible corner case when |range| is 20):
  //
  //   (-2^12) * (-2^19) = 2^31, which cannot be represented as an int32_t.
  //
  // Note: |range| is 20 when bitdepth is 12 and a row transform is performed.
  //
  // Assert that this angle is never used by DCT or ADST.
  assert(angle != 128);
  if (angle <= 64) return kCos128[angle];
  if (angle <= 128) return -kCos128[128 - angle];
  if (angle <= 192) return -kCos128[angle - 128];
  return kCos128[256 - angle];
}

inline int16_t Sin128(int angle) { return Cos128(angle - 64); }

// The value for index i is derived as:
// round(sqrt(2) * sin(i * pi / 9) * 2 / 3 * (1 << 12)).
constexpr int16_t kAdst4Multiplier[4] = {1321, 2482, 3344, 3803};

constexpr uint8_t kTransformRowShift[kNumTransformSizes] = {
    0, 0, 1, 0, 1, 1, 2, 1, 1, 2, 1, 2, 2, 1, 2, 1, 2, 1, 2};

constexpr bool kShouldRound[kNumTransformSizes] = {
    false, true,  false, true, false, true, false, false, true, false,
    true,  false, false, true, false, true, false, true,  false};

constexpr int16_t kIdentity4Multiplier /* round(2^12 * sqrt(2)) */ = 0x16A1;
constexpr int16_t kIdentity4MultiplierFraction /* round(2^12 * (sqrt(2) - 1))*/
    = 0x6A1;
constexpr int16_t kIdentity16Multiplier /* 2 * round(2^12 * sqrt(2)) */ = 11586;
constexpr int16_t kTransformRowMultiplier /* round(2^12 / sqrt(2)) */ = 2896;
