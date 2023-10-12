// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_STRINGPRINTF_H_
#define UTIL_STRINGPRINTF_H_

#include <stdint.h>

#include <ostream>
#include <string>

#include "platform/base/span.h"

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

// Returns a hex string representation of the given |bytes|.
std::string HexEncode(const uint8_t* bytes, size_t len);
std::string HexEncode(ByteView bytes);

}  // namespace openscreen

#endif  // UTIL_STRINGPRINTF_H_
