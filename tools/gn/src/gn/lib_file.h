// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_LIB_FILE_H_
#define TOOLS_GN_LIB_FILE_H_

#include <stddef.h>

#include <algorithm>
#include <string>
#include <string_view>

#include "gn/source_file.h"

// Represents an entry in "libs" list. Can be either a path (a SourceFile) or
// a library name (a string).
class LibFile {
 public:
  LibFile() = default;

  explicit LibFile(std::string_view lib_name);
  explicit LibFile(const SourceFile& source_file);

  bool is_source_file() const { return name_.empty(); }

  // Returns name, or source_file().value() (whichever is set).
  const std::string& value() const;
  const SourceFile& source_file() const;

  bool operator==(const LibFile& other) const {
    return value() == other.value();
  }
  bool operator!=(const LibFile& other) const { return !operator==(other); }
  bool operator<(const LibFile& other) const { return value() < other.value(); }

 private:
  std::string name_;
  SourceFile source_file_;
};

namespace std {

template <>
struct hash<LibFile> {
  std::size_t operator()(const LibFile& v) const {
    hash<std::string> h;
    return h(v.value());
  }
};

}  // namespace std

#endif  // TOOLS_GN_LIB_FILE_H_
