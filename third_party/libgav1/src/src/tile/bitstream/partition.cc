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

#include <cassert>
#include <cstdint>

#include "src/symbol_decoder_context.h"
#include "src/tile.h"
#include "src/utils/block_parameters_holder.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/entropy_decoder.h"
#include "src/utils/types.h"

namespace libgav1 {
namespace {

uint16_t PartitionCdfGatherHorizontalAlike(const uint16_t* const partition_cdf,
                                           BlockSize block_size) {
  // The spec computes the cdf value using the following formula (not writing
  // partition_cdf[] and using short forms for partition names for clarity):
  //   cdf = None - H + V - S + S - HTS + HTS - HBS + HBS - VLS;
  //   if (block_size != 128x128) {
  //     cdf += VRS - H4;
  //   }
  // After canceling out the repeated terms with opposite signs, we have:
  //   cdf = None - H + V - VLS;
  //   if (block_size != 128x128) {
  //     cdf += VRS - H4;
  //   }
  uint16_t cdf = partition_cdf[kPartitionNone] -
                 partition_cdf[kPartitionHorizontal] +
                 partition_cdf[kPartitionVertical] -
                 partition_cdf[kPartitionVerticalWithLeftSplit];
  if (block_size != kBlock128x128) {
    cdf += partition_cdf[kPartitionVerticalWithRightSplit] -
           partition_cdf[kPartitionHorizontal4];
  }
  return cdf;
}

uint16_t PartitionCdfGatherVerticalAlike(const uint16_t* const partition_cdf,
                                         BlockSize block_size) {
  // The spec computes the cdf value using the following formula (not writing
  // partition_cdf[] and using short forms for partition names for clarity):
  //   cdf = H - V + V - S + HBS - VLS + VLS - VRS + S - HTS;
  //   if (block_size != 128x128) {
  //     cdf += H4 - V4;
  //   }
  // V4 is always zero. So, after canceling out the repeated terms with opposite
  // signs, we have:
  //   cdf = H + HBS - VRS - HTS;
  //   if (block_size != 128x128) {
  //     cdf += H4;
  //   }
  // VRS is zero for 128x128 blocks. So, further simplifying we have:
  //   cdf = H + HBS - HTS;
  //   if (block_size != 128x128) {
  //     cdf += H4 - VRS;
  //   }
  uint16_t cdf = partition_cdf[kPartitionHorizontal] +
                 partition_cdf[kPartitionHorizontalWithBottomSplit] -
                 partition_cdf[kPartitionHorizontalWithTopSplit];
  if (block_size != kBlock128x128) {
    cdf += partition_cdf[kPartitionHorizontal4] -
           partition_cdf[kPartitionVerticalWithRightSplit];
  }
  return cdf;
}

}  // namespace

uint16_t* Tile::GetPartitionCdf(int row4x4, int column4x4,
                                BlockSize block_size) {
  const int block_size_log2 = k4x4WidthLog2[block_size];
  int top = 0;
  if (IsTopInside(row4x4)) {
    top = static_cast<int>(
        k4x4WidthLog2[block_parameters_holder_.Find(row4x4 - 1, column4x4)
                          ->size] < block_size_log2);
  }
  int left = 0;
  if (IsLeftInside(column4x4)) {
    left = static_cast<int>(
        k4x4HeightLog2[block_parameters_holder_.Find(row4x4, column4x4 - 1)
                           ->size] < block_size_log2);
  }
  const int context = left * 2 + top;
  return symbol_decoder_context_.partition_cdf[block_size_log2 - 1][context];
}

bool Tile::ReadPartition(int row4x4, int column4x4, BlockSize block_size,
                         bool has_rows, bool has_columns,
                         Partition* const partition) {
  if (IsBlockSmallerThan8x8(block_size)) {
    *partition = kPartitionNone;
    return true;
  }
  if (!has_rows && !has_columns) {
    *partition = kPartitionSplit;
    return true;
  }
  uint16_t* const partition_cdf =
      GetPartitionCdf(row4x4, column4x4, block_size);
  if (partition_cdf == nullptr) {
    return false;
  }
  if (has_rows && has_columns) {
    const int bsize_log2 = k4x4WidthLog2[block_size];
    // The partition block size should be 8x8 or above.
    assert(bsize_log2 > 0);
    if (bsize_log2 == 1) {
      *partition = static_cast<Partition>(
          reader_.ReadSymbol<kPartitionSplit + 1>(partition_cdf));
    } else if (bsize_log2 == 5) {
      *partition = static_cast<Partition>(
          reader_.ReadSymbol<kPartitionVerticalWithRightSplit + 1>(
              partition_cdf));
    } else {
      *partition = static_cast<Partition>(
          reader_.ReadSymbol<kMaxPartitionTypes>(partition_cdf));
    }
  } else if (has_columns) {
    const uint16_t cdf =
        PartitionCdfGatherVerticalAlike(partition_cdf, block_size);
    *partition = reader_.ReadSymbolWithoutCdfUpdate(cdf) ? kPartitionSplit
                                                         : kPartitionHorizontal;
  } else {
    const uint16_t cdf =
        PartitionCdfGatherHorizontalAlike(partition_cdf, block_size);
    *partition = reader_.ReadSymbolWithoutCdfUpdate(cdf) ? kPartitionSplit
                                                         : kPartitionVertical;
  }
  return true;
}

}  // namespace libgav1
