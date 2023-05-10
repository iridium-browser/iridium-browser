// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_STRINGS_UTF_STRING_CONVERSIONS_H_
#define BASE_STRINGS_UTF_STRING_CONVERSIONS_H_

#include <stddef.h>

#include <string>
#include <string_view>

namespace base {

bool UTF8ToUTF16(const char* src, size_t src_len, std::u16string* output);
std::u16string UTF8ToUTF16(std::string_view utf8);
bool UTF16ToUTF8(const char16_t* src, size_t src_len, std::string* output);
std::string UTF16ToUTF8(std::u16string_view utf16);

// This converts an ASCII string, typically a hardcoded constant, to a UTF16
// string.
std::u16string ASCIIToUTF16(std::string_view ascii);

// Converts to 7-bit ASCII by truncating. The result must be known to be ASCII
// beforehand.
std::string UTF16ToASCII(std::u16string_view utf16);

}  // namespace base

#endif  // BASE_STRINGS_UTF_STRING_CONVERSIONS_H_
