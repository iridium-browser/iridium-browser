// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_C_INCLUDE_ITERATOR_H_
#define TOOLS_GN_C_INCLUDE_ITERATOR_H_

#include <stddef.h>

#include <string_view>

#include "gn/location.h"

class InputFile;

struct IncludeStringWithLocation {
  std::string_view contents;
  LocationRange location;
  bool system_style_include = false;
};

// Iterates through #includes in C source and header files.
class CIncludeIterator {
 public:
  // The InputFile pointed to must outlive this class.
  explicit CIncludeIterator(const InputFile* input);
  ~CIncludeIterator();

  // Fills in the string with the contents of the next include, and the
  // location with where it came from, and returns true, or returns false if
  // there are no more includes.
  bool GetNextIncludeString(IncludeStringWithLocation* include);

  // Maximum numbef of non-includes we'll tolerate before giving up. This does
  // not count comments or preprocessor.
  static const int kMaxNonIncludeLines;

 private:
  // Returns false on EOF, otherwise fills in the given line and the one-based
  // line number into *line_number;
  bool GetNextLine(std::string_view* line, int* line_number);

  const InputFile* input_file_;

  // This just points into input_file_.contents() for convenience.
  std::string_view file_;

  // 0-based offset into the file.
  size_t offset_ = 0;

  int line_number_ = 0;  // One-based. Indicates the last line we read.

  // Number of lines we've processed since seeing the last include (or the
  // beginning of the file) with some exceptions.
  int lines_since_last_include_ = 0;

  CIncludeIterator(const CIncludeIterator&) = delete;
  CIncludeIterator& operator=(const CIncludeIterator&) = delete;
};

#endif  // TOOLS_GN_C_INCLUDE_ITERATOR_H_
