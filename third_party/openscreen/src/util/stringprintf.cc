// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/stringprintf.h"

#include <cstdarg>
#include <cstdio>
#include <iomanip>
#include <sstream>

#include "util/osp_logging.h"

namespace openscreen {

std::string StringPrintf(const char* format, ...) {
  va_list vlist;
  va_start(vlist, format);
  const int length = std::vsnprintf(nullptr, 0, format, vlist);
  OSP_CHECK_GE(length, 0) << "Invalid format string: " << format;
  va_end(vlist);

  std::string result(length, '\0');
  // Note: There's no need to add one for the extra terminating NUL char since
  // the standard, since C++11, requires that "data() + size() points to [the
  // NUL terminator]". Thus, std::vsnprintf() will write the NUL to a valid
  // memory location.
  va_start(vlist, format);
  std::vsnprintf(&result[0], length + 1, format, vlist);
  va_end(vlist);

  return result;
}

std::string HexEncode(const uint8_t* bytes, std::size_t len) {
  std::ostringstream hex_dump;
  hex_dump << std::setfill('0') << std::hex;
  for (std::size_t i = 0; i < len; i++) {
    hex_dump << std::setw(2) << static_cast<int>(bytes[i]);
  }
  return hex_dump.str();
}

}  // namespace openscreen
