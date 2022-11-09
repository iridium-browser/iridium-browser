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

#include "src/utils/bit_reader.h"

#include <cassert>
#include <cstdint>

#include "src/utils/common.h"

namespace libgav1 {
namespace {

bool Assign(int* const value, int assignment, bool return_value) {
  *value = assignment;
  return return_value;
}

// 5.9.29.
int InverseRecenter(int r, int v) {
  if (v > (r << 1)) {
    return v;
  }
  if ((v & 1) != 0) {
    return r - ((v + 1) >> 1);
  }
  return r + (v >> 1);
}

}  // namespace

bool BitReader::DecodeSignedSubexpWithReference(int low, int high,
                                                int reference, int control,
                                                int* const value) {
  if (!DecodeUnsignedSubexpWithReference(high - low, reference - low, control,
                                         value)) {
    return false;
  }
  *value += low;
  return true;
}

bool BitReader::DecodeUniform(int n, int* const value) {
  if (n <= 1) {
    return Assign(value, 0, true);
  }
  const int w = FloorLog2(n) + 1;
  const int m = (1 << w) - n;
  assert(w - 1 < 32);
  const int v = static_cast<int>(ReadLiteral(w - 1));
  if (v == -1) {
    return Assign(value, 0, false);
  }
  if (v < m) {
    return Assign(value, v, true);
  }
  const int extra_bit = ReadBit();
  if (extra_bit == -1) {
    return Assign(value, 0, false);
  }
  return Assign(value, (v << 1) - m + extra_bit, true);
}

bool BitReader::DecodeUnsignedSubexpWithReference(int mx, int reference,
                                                  int control,
                                                  int* const value) {
  int v;
  if (!DecodeSubexp(mx, control, &v)) return false;
  if ((reference << 1) <= mx) {
    *value = InverseRecenter(reference, v);
  } else {
    *value = mx - 1 - InverseRecenter(mx - 1 - reference, v);
  }
  return true;
}

bool BitReader::DecodeSubexp(int num_symbols, int control, int* const value) {
  int i = 0;
  int mk = 0;
  while (true) {
    const int b = (i != 0) ? control + i - 1 : control;
    if (b >= 32) {
      return Assign(value, 0, false);
    }
    const int a = 1 << b;
    if (num_symbols <= mk + 3 * a) {
      if (!DecodeUniform(num_symbols - mk, value)) return false;
      *value += mk;
      return true;
    }
    const int8_t subexp_more_bits = ReadBit();
    if (subexp_more_bits == -1) return false;
    if (subexp_more_bits != 0) {
      ++i;
      mk += a;
    } else {
      const int subexp_bits = static_cast<int>(ReadLiteral(b));
      if (subexp_bits == -1) {
        return Assign(value, 0, false);
      }
      return Assign(value, subexp_bits + mk, true);
    }
  }
}

}  // namespace libgav1
