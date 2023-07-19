// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBIPP_IPP_LOG_H_
#define LIBIPP_IPP_LOG_H_

#include <string>

namespace ipp {

// Single entry in error logs used by Client and Server classes defined below.
struct Log {
  // Description of the error, it is always non-empty string.
  std::string message;
  // Position in the input buffer, set to 0 if unknown.
  std::size_t buf_offset = 0;
  // String with hex representation of part of the frame corresponding to
  // position given in buf_offset. Empty string if buf_offset == 0.
  std::string frame_context;
  // Attribute/Group names where the error occurred. Empty string if
  // unknown.
  std::string parser_context;
};

}  // namespace ipp

#endif  //  LIBIPP_IPP_LOG_H_
