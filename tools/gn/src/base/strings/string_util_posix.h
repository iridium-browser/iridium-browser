// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_STRINGS_STRING_UTIL_POSIX_H_
#define BASE_STRINGS_STRING_UTIL_POSIX_H_

#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include "base/logging.h"

namespace base {

inline int vsnprintf(char* buffer,
                     size_t size,
                     const char* format,
                     va_list arguments) {
  return ::vsnprintf(buffer, size, format, arguments);
}

}  // namespace base

#endif  // BASE_STRINGS_STRING_UTIL_POSIX_H_
