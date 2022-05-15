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

#include "src/utils/segmentation_map.h"

#include <cassert>
#include <cstring>
#include <new>

namespace libgav1 {

bool SegmentationMap::Allocate(int32_t rows4x4, int32_t columns4x4) {
  rows4x4_ = rows4x4;
  columns4x4_ = columns4x4;
  segment_id_buffer_.reset(new (std::nothrow) int8_t[rows4x4_ * columns4x4_]);
  if (segment_id_buffer_ == nullptr) return false;
  segment_id_.Reset(rows4x4_, columns4x4_, segment_id_buffer_.get());
  return true;
}

void SegmentationMap::Clear() {
  memset(segment_id_buffer_.get(), 0, rows4x4_ * columns4x4_);
}

void SegmentationMap::CopyFrom(const SegmentationMap& from) {
  assert(rows4x4_ == from.rows4x4_ && columns4x4_ == from.columns4x4_);
  memcpy(segment_id_buffer_.get(), from.segment_id_buffer_.get(),
         rows4x4_ * columns4x4_);
}

void SegmentationMap::FillBlock(int row4x4, int column4x4, int block_width4x4,
                                int block_height4x4, int8_t segment_id) {
  for (int y = 0; y < block_height4x4; ++y) {
    memset(&segment_id_[row4x4 + y][column4x4], segment_id, block_width4x4);
  }
}

}  // namespace libgav1
