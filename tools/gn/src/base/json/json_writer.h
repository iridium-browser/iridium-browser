// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_JSON_JSON_WRITER_H_
#define BASE_JSON_JSON_WRITER_H_

#include <stddef.h>

#include <string>

namespace base {

class Value;

class JSONWriter {
 public:
  enum Options {
    // This option instructs the writer that if a Binary value is encountered,
    // the value (and key if within a dictionary) will be omitted from the
    // output, and success will be returned. Otherwise, if a binary value is
    // encountered, failure will be returned.
    OPTIONS_OMIT_BINARY_VALUES = 1 << 0,

    // Return a slightly nicer formatted json string (pads with whitespace to
    // help with readability).
    OPTIONS_PRETTY_PRINT = 1 << 2,
  };

  // Given a root node, generates a JSON string and puts it into |json|.
  // The output string is overwritten and not appended.
  //
  // TODO(tc): Should we generate json if it would be invalid json (e.g.,
  // |node| is not a DictionaryValue/ListValue or if there are inf/-inf float
  // values)? Return true on success and false on failure.
  static bool Write(const Value& node, std::string* json);

  // Same as above but with |options| which is a bunch of JSONWriter::Options
  // bitwise ORed together. Return true on success and false on failure.
  static bool WriteWithOptions(const Value& node,
                               int options,
                               std::string* json);

 private:
  JSONWriter(int options, std::string* json);

  // Called recursively to build the JSON string. When completed,
  // |json_string_| will contain the JSON.
  bool BuildJSONString(const Value& node, size_t depth);

  // Adds space to json_string_ for the indent level.
  void IndentLine(size_t depth);

  bool omit_binary_values_;
  bool pretty_print_;

  // Where we write JSON data as we generate it.
  std::string* json_string_;

  JSONWriter(const JSONWriter&) = delete;
  JSONWriter& operator=(const JSONWriter&) = delete;
};

}  // namespace base

#endif  // BASE_JSON_JSON_WRITER_H_
