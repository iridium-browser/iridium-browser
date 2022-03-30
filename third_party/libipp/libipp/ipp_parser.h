// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBIPP_IPP_PARSER_H_
#define LIBIPP_IPP_PARSER_H_

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ipp_attribute.h"  // NOLINT(build/include)
#include "ipp_base.h"  // NOLINT(build/include)
#include "ipp_frame.h"  // NOLINT(build/include)

namespace ipp {

// forward declarations
class Collection;
class Package;
struct RawAttribute;
struct RawCollection;

class Parser {
 public:
  // Constructor, both parameters must not be nullptr. |frame| is used as
  // internal buffer to store intermediate form of parsed data. All spotted
  // issues are logged to |log| (by push_back()).
  Parser(Frame* frame, std::vector<Log>* log) : frame_(frame), errors_(log) {}

  // Parses data from given buffer and saves it to internal buffer.
  bool ReadFrameFromBuffer(const uint8_t* ptr, const uint8_t* const buf_end);

  // Interpret the content from internal buffer and store it in |package|.
  // If |log_unknown_values| is set then all attributes unknown in |package|
  // are reported to the log.
  bool SaveFrameToPackage(bool log_unknown_values, Package* package);

  // Resets the state of the object to initial state. It does not modify
  // objects provided in the constructor (|frame| and |log|).
  void ResetContent();

 private:
  // Copy/move/assign constructors/operators are forbidden.
  Parser(const Parser&) = delete;
  Parser(Parser&&) = delete;
  Parser& operator=(const Parser&) = delete;
  Parser& operator=(Parser&&) = delete;

  // Methods for adding entries to the log.
  void LogScannerError(const std::string& message, const uint8_t* position);
  void LogParserError(
      const std::string& message,
      std::string action = "This is critical error, parsing was cancelled.");
  void LogParserWarning(const std::string& message);
  void LogParserNewElement();

  // Reads single tag_name_value from the buffer (ptr is updated) and append it
  // to the parameter. Returns false <=> error occurred.
  bool ReadTNVsFromBuffer(const uint8_t** ptr,
                          const uint8_t* const end_buf,
                          std::list<TagNameValue>*);

  // Parser helpers.
  bool ParseRawValue(const TagNameValue& tnv,
                     std::list<TagNameValue>* data_chunks,
                     RawAttribute* attr);
  bool ParseRawCollection(std::list<TagNameValue>* data_chunks,
                          RawCollection* coll);
  bool ParseRawGroup(std::list<TagNameValue>* data_chunks, RawCollection* coll);
  bool DecodeCollection(RawCollection* raw, Collection* coll);

  // Helper for parsing individual values.
  void LoadAttrValue(Attribute* attr,
                     size_t index,
                     const std::vector<uint8_t>& buf,
                     uint8_t tag);

  // Internal buffer.
  Frame* frame_;

  // Internal log: all errors & warnings are logged here.
  std::vector<Log>* errors_;

  // Temporary copies of begin/end pointers of the current buffer.
  const uint8_t* buffer_begin_ = nullptr;
  const uint8_t* buffer_end_ = nullptr;

  // This is a path to group/attribute being processed currently by the parser.
  // Each segment contains a name and a flag (true/false). When the  flag is
  // set to true then notices about direct offspring unknown attributes are
  // added to the log with LogParserNewElement(...) method. When false, this
  // method has no effect for offspring attributes.
  std::vector<std::pair<std::string, bool>> parser_context_;
};

}  // namespace ipp

#endif  //  LIBIPP_IPP_PARSER_H_
