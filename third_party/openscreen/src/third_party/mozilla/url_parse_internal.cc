// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/mozilla//url_parse_internal.h"

#include <ctype.h>

#include "third_party/mozilla/url_parse.h"

namespace openscreen {

namespace {

static const char* g_standard_schemes[] = {
    kHttpsScheme, kHttpScheme, kFileScheme, kFtpScheme, kWssScheme, kWsScheme,
};

}  // namespace

bool IsURLSlash(char ch) {
  return ch == '/' || ch == '\\';
}

bool ShouldTrimFromURL(char ch) {
  return ch <= ' ';
}

void TrimURL(const char* spec, int* begin, int* len, bool trim_path_end) {
  // Strip leading whitespace and control characters.
  while (*begin < *len && ShouldTrimFromURL(spec[*begin])) {
    (*begin)++;
  }

  if (trim_path_end) {
    // Strip trailing whitespace and control characters. We need the >i test
    // for when the input string is all blanks; we don't want to back past the
    // input.
    while (*len > *begin && ShouldTrimFromURL(spec[*len - 1])) {
      (*len)--;
    }
  }
}

int CountConsecutiveSlashes(const char* str, int begin_offset, int str_len) {
  int count = 0;
  while ((begin_offset + count) < str_len &&
         IsURLSlash(str[begin_offset + count])) {
    ++count;
  }
  return count;
}

bool CompareSchemeComponent(const char* spec,
                            const Component& component,
                            const char* compare_to) {
  if (!component.is_nonempty()) {
    return compare_to[0] == 0;  // When component is empty, match empty scheme.
  }
  for (int i = 0; i < component.len; ++i) {
    if (tolower(spec[i]) != compare_to[i]) {
      return false;
    }
  }
  return true;
}

bool IsStandard(const char* spec, const Component& component) {
  if (!component.is_nonempty()) {
    return false;
  }

  constexpr int scheme_count =
      sizeof(g_standard_schemes) / sizeof(g_standard_schemes[0]);
  for (int i = 0; i < scheme_count; ++i) {
    if (CompareSchemeComponent(spec, component, g_standard_schemes[i])) {
      return true;
    }
  }
  return false;
}

// NOTE: Not implemented because file URLs are currently unsupported.
void ParseFileURL(const char* url, int url_len, Parsed* parsed) {}

}  // namespace openscreen
