// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/standalone_sender/streaming_encoder_util.h"

#include <string.h>

#include <algorithm>

namespace openscreen {
namespace cast {
void CopyPlane(const uint8_t* src,
               int src_stride,
               int num_rows,
               uint8_t* dst,
               int dst_stride) {
  if (src_stride == dst_stride) {
    memcpy(dst, src, src_stride * num_rows);
    return;
  }
  const int bytes_per_row = std::min(src_stride, dst_stride);
  while (--num_rows >= 0) {
    memcpy(dst, src, bytes_per_row);
    dst += dst_stride;
    src += src_stride;
  }
}
}  // namespace cast
}  // namespace openscreen
