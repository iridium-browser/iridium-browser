// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_STRINGS_STRINGPRINTF_H_
#define BASE_STRINGS_STRINGPRINTF_H_

#include <stdarg.h>  // va_list

#include <string>

#include "base/compiler_specific.h"
#include "util/build_config.h"

namespace base {

// Return a C++ string given printf-like input.
[[nodiscard]] std::string StringPrintf(
    _Printf_format_string_ const char* format,
    ...) PRINTF_FORMAT(1, 2);

// Return a C++ string given vprintf-like input.
[[nodiscard]] std::string StringPrintV(const char* format, va_list ap)
    PRINTF_FORMAT(1, 0);

// Store result into a supplied string and return it.
const std::string& SStringPrintf(std::string* dst,
                                 _Printf_format_string_ const char* format,
                                 ...) PRINTF_FORMAT(2, 3);

// Append result to a supplied string.
void StringAppendF(std::string* dst,
                   _Printf_format_string_ const char* format,
                   ...) PRINTF_FORMAT(2, 3);

// Lower-level routine that takes a va_list and appends to a specified
// string.  All other routines are just convenience wrappers around it.
void StringAppendV(std::string* dst, const char* format, va_list ap)
    PRINTF_FORMAT(2, 0);

}  // namespace base

#endif  // BASE_STRINGS_STRINGPRINTF_H_
