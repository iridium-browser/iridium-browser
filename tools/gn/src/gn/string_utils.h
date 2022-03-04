// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_STRING_UTILS_H_
#define TOOLS_GN_STRING_UTILS_H_

#include <string>
#include <string_view>
#include <vector>

class Err;
class Scope;
class Token;
class Value;

inline std::string operator+(const std::string& a, std::string_view b) {
  std::string ret;
  ret.reserve(a.size() + b.size());
  ret.assign(a);
  ret.append(b.data(), b.size());
  return ret;
}

inline std::string operator+(std::string_view a, const std::string& b) {
  std::string ret;
  ret.reserve(a.size() + b.size());
  ret.assign(a.data(), a.size());
  ret.append(b);
  return ret;
}

// Unescapes and expands variables in the given literal, writing the result
// to the given value. On error, sets |err| and returns false.
bool ExpandStringLiteral(Scope* scope,
                         const Token& literal,
                         Value* result,
                         Err* err);

// Returns the minimum number of inserts, deleted, and replacements of
// characters needed to transform s1 to s2, or max_edit_distance + 1 if
// transforming s1 into s2 isn't possible in at most max_edit_distance steps.
size_t EditDistance(std::string_view s1,
                    std::string_view s2,
                    size_t max_edit_distance);

// Given a string |text| and a vector of correctly-spelled strings |words|,
// returns the first string in |words| closest to |text|, or an empty
// std::string_view if none of the strings in |words| is close.
std::string_view SpellcheckString(std::string_view text,
                                  const std::vector<std::string_view>& words);

// Reads stdin until end-of-data and returns what it read.
std::string ReadStdin();

#endif  // TOOLS_GN_STRING_UTILS_H_
