// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdlib.h>
#include <iostream>
#include <string>

namespace {

// Returns the current user username.
std::string Username() {
  const char* username = getenv("USER");
  return username ? std::string(username) : std::string();
}

// Writes |string| to |stream| while escaping all C escape sequences.
void EscapeString(std::ostream* stream, const std::string& string) {
  for (char c : string) {
    switch (c) {
      case 0:
        *stream << "\\0";
        break;
      case '\a':
        *stream << "\\a";
        break;
      case '\b':
        *stream << "\\b";
        break;
      case '\e':
        *stream << "\\e";
        break;
      case '\f':
        *stream << "\\f";
        break;
      case '\n':
        *stream << "\\n";
        break;
      case '\r':
        *stream << "\\r";
        break;
      case '\t':
        *stream << "\\t";
        break;
      case '\v':
        *stream << "\\v";
        break;
      case '\\':
        *stream << "\\\\";
        break;
      case '\"':
        *stream << "\\\"";
        break;
      default:
        *stream << c;
        break;
    }
  }
}

}  // namespace

int main(int argc, char** argv) {
  std::string username = Username();

  std::cout << "{\"username\": \"";
  EscapeString(&std::cout, username);
  std::cout << "\"}" << std::endl;

  return 0;
}
