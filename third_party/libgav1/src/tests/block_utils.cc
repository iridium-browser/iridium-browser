// Copyright 2020 The libgav1 Authors
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

#include "tests/block_utils.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace libgav1 {
namespace test_utils {
namespace {

#define LIBGAV1_DEBUG_FORMAT_CODE "x"
template <typename Pixel>
void PrintBlockDiff(const Pixel* block1, const Pixel* block2, int width,
                    int height, int stride1, int stride2,
                    const bool print_padding) {
  const int print_width = print_padding ? std::min(stride1, stride2) : width;
  const int field_width = (sizeof(Pixel) == 1) ? 4 : 5;

  for (int y = 0; y < height; ++y) {
    printf("[%2d] ", y);
    for (int x = 0; x < print_width; ++x) {
      if (x >= width) {
        if (block1[x] == block2[x]) {
          printf("[%*" LIBGAV1_DEBUG_FORMAT_CODE "] ", field_width, block1[x]);
        } else {
          printf("[*%*" LIBGAV1_DEBUG_FORMAT_CODE "] ", field_width - 1,
                 block1[x]);
        }
      } else {
        if (block1[x] == block2[x]) {
          printf("%*" LIBGAV1_DEBUG_FORMAT_CODE " ", field_width, block1[x]);
        } else {
          printf("*%*" LIBGAV1_DEBUG_FORMAT_CODE " ", field_width - 1,
                 block1[x]);
        }
      }
    }
    printf("\n");
    block1 += stride1;
    block2 += stride2;
  }
}

}  // namespace

template <typename Pixel>
void PrintBlock(const Pixel* block, int width, int height, int stride,
                const bool print_padding /*= false*/) {
  const int print_width = print_padding ? stride : width;
  const int field_width = (sizeof(Pixel) == 1) ? 4 : 5;
  for (int y = 0; y < height; ++y) {
    printf("[%2d] ", y);
    for (int x = 0; x < print_width; ++x) {
      if (x >= width) {
        printf("[%*" LIBGAV1_DEBUG_FORMAT_CODE "] ", field_width, block[x]);
      } else {
        printf("%*" LIBGAV1_DEBUG_FORMAT_CODE " ", field_width, block[x]);
      }
    }
    printf("\n");
    block += stride;
  }
}
#undef LIBGAV1_DEBUG_FORMAT_CODE

template void PrintBlock(const uint8_t* block, int width, int height,
                         int stride, bool print_padding /*= false*/);
template void PrintBlock(const uint16_t* block, int width, int height,
                         int stride, bool print_padding /*= false*/);
template void PrintBlock(const int8_t* block, int width, int height, int stride,
                         bool print_padding /*= false*/);
template void PrintBlock(const int16_t* block, int width, int height,
                         int stride, bool print_padding /*= false*/);

template <typename Pixel>
bool CompareBlocks(const Pixel* block1, const Pixel* block2, int width,
                   int height, int stride1, int stride2,
                   const bool check_padding, const bool print_diff /*= true*/) {
  bool ok = true;
  const int check_width = check_padding ? std::min(stride1, stride2) : width;
  for (int y = 0; y < height; ++y) {
    const uint64_t row1 = static_cast<uint64_t>(y) * stride1;
    const uint64_t row2 = static_cast<uint64_t>(y) * stride2;
    ok = memcmp(block1 + row1, block2 + row2,
                sizeof(block1[0]) * check_width) == 0;
    if (!ok) break;
  }
  if (!ok && print_diff) {
    printf("block1 (width: %d height: %d stride: %d):\n", width, height,
           stride1);
    PrintBlockDiff(block1, block2, width, height, stride1, stride2,
                   check_padding);
    printf("\nblock2 (width: %d height: %d stride: %d):\n", width, height,
           stride2);
    PrintBlockDiff(block2, block1, width, height, stride2, stride1,
                   check_padding);
  }
  return ok;
}

template bool CompareBlocks(const uint8_t* block1, const uint8_t* block2,
                            int width, int height, int stride1, int stride2,
                            const bool check_padding,
                            const bool print_diff /*= true*/);
template bool CompareBlocks(const uint16_t* block1, const uint16_t* block2,
                            int width, int height, int stride1, int stride2,
                            const bool check_padding,
                            const bool print_diff /*= true*/);
template bool CompareBlocks(const int8_t* block1, const int8_t* block2,
                            int width, int height, int stride1, int stride2,
                            const bool check_padding,
                            const bool print_diff /*= true*/);
template bool CompareBlocks(const int16_t* block1, const int16_t* block2,
                            int width, int height, int stride1, int stride2,
                            const bool check_padding,
                            const bool print_diff /*= true*/);

}  // namespace test_utils
}  // namespace libgav1
