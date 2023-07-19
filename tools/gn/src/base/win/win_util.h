// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_WIN_WIN_UTIL_H_
#define BASE_WIN_WIN_UTIL_H_

#include <string>
#include <string_view>

namespace base {

// Windows API calls take wchar_t but on that platform wchar_t should be the
// same as a char16_t.
inline const wchar_t* ToWCharT(const std::u16string* s) {
  static_assert(sizeof(std::u16string::value_type) == sizeof(wchar_t));
  return reinterpret_cast<const wchar_t*>(s->c_str());
}

inline const wchar_t* ToWCharT(const char16_t* s) {
  return reinterpret_cast<const wchar_t*>(s);
}

inline wchar_t* ToWCharT(char16_t* s) {
  return reinterpret_cast<wchar_t*>(s);
}

}  // namespace base

#endif  // BASE_WIN_WIN_UTIL_H_
