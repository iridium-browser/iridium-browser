// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_STRINGPRINTF_H_
#define UTIL_STRINGPRINTF_H_

#include <stdint.h>

#include <ostream>
#include <string>

namespace openscreen {

// Enable compile-time checking of the printf format argument, if available.
#if defined(__GNUC__) || defined(__clang__)
#define OSP_CHECK_PRINTF_ARGS(format_param, dots_param) \
  __attribute__((format(printf, format_param, dots_param)))
#else
#define OSP_CHECK_PRINTF_ARGS(format_param, dots_param)
#endif

// Returns a std::string containing the results of a std::sprintf() call. This
// is an efficient, zero-copy wrapper.
[[nodiscard]] std::string StringPrintf(const char* format, ...)
    OSP_CHECK_PRINTF_ARGS(1, 2);

template <typename It>
void PrettyPrintAsciiHex(std::ostream& os, It first, It last) {
  auto it = first;
  while (it != last) {
    uint8_t c = *it++;
    if (c >= ' ' && c <= '~') {
      os.put(c);
    } else {
      // Output a hex escape sequence for non-printable values.
      os.put('\\');
      os.put('x');
      char digit = (c >> 4) & 0xf;
      if (digit >= 0 && digit <= 9) {
        os.put(digit + '0');
      } else {
        os.put(digit - 10 + 'a');
      }
      digit = c & 0xf;
      if (digit >= 0 && digit <= 9) {
        os.put(digit + '0');
      } else {
        os.put(digit - 10 + 'a');
      }
    }
  }
}

// Returns a hex string representation of the given |bytes|.
std::string HexEncode(const uint8_t* bytes, std::size_t len);

}  // namespace openscreen

#endif  // UTIL_STRINGPRINTF_H_
