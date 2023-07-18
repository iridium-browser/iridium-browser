// Copyright 2023 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBIPP_BUILDER_H_
#define LIBIPP_BUILDER_H_

#include <cstdint>
#include <vector>

#include "frame.h"

namespace ipp {

// Return the size of the binary representation of the frame in bytes.
size_t LIBIPP_EXPORT CalculateLengthOfBinaryFrame(const Frame& frame);

// Save the binary representation of the frame to the given buffer. Use
// CalculateLengthOfBinaryFrame() function before calling this method to make
// sure that the given buffer is large enough. The method returns the number of
// bytes written to `buffer` or 0 when `buffer_length` is smaller than binary
// representation of the frame.
size_t LIBIPP_EXPORT BuildBinaryFrame(const Frame& frame,
                                      uint8_t* buffer,
                                      size_t buffer_length);

// Return the binary representation of the frame as a vector.
std::vector<uint8_t> LIBIPP_EXPORT BuildBinaryFrame(const Frame& frame);

}  // namespace ipp

#endif  //  LIBIPP_BUILDER_H_
