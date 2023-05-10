// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "errors.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace ipp {

namespace {

std::string UnsignedToString(size_t x) {
  std::string s;
  do {
    s.push_back('0' + (x % 10));
    x /= 10;
  } while (x > 0);
  std::reverse(s.begin(), s.end());
  return s;
}

// Converts the least significant 4 bits to hexadecimal digit (ASCII char).
char ToHexDigit(uint8_t v) {
  v &= 0x0f;
  if (v < 10)
    return ('0' + v);
  return ('a' + (v - 10));
}

// Converts byte to 2-digit hexadecimal representation.
std::string ToHexByte(uint8_t v) {
  std::string s(2, '0');
  s[0] = ToHexDigit(v >> 4);
  s[1] = ToHexDigit(v);
  return s;
}

std::string ToJsonString(const std::string& str, std::string special_chars) {
  std::string out;
  for (char c : str) {
    if (c < 0x20 || c > 0x7e || c == '\\' || c == '"' ||
        special_chars.find(c) != std::string::npos) {
      out.push_back('\\');
      switch (c) {
        case '\\':
        case '"':
          out.push_back(c);
          break;
        case '\n':
          out.push_back('n');
          break;
        case '\t':
          out.push_back('t');
          break;
        default:
          out += "u00";
          out += ToHexByte(c);
          break;
      }
    } else {
      out.push_back(c);
    }
  }
  return out;
}

}  // namespace

std::string AttrPath::AsString() const {
  std::string out = (group_ == kHeader) ? "header" : ToString(group_);
  for (const Segment& segment : path_) {
    out += "[";
    out += UnsignedToString(segment.collection_index);
    out += "]>";
    out += ToJsonString(segment.attribute_name, "[]>");
  }
  return out;
}

}  // namespace ipp
