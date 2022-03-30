// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_ERR_H_
#define TOOLS_GN_ERR_H_

#include <memory>
#include <string>
#include <vector>

#include "gn/label.h"
#include "gn/location.h"
#include "gn/token.h"

class ParseNode;
class Value;

// Result of doing some operation. Check has_error() to see if an error
// occurred.
//
// An error has a location and a message. Below that, is some optional help
// text to go with the annotation of the location.
//
// An error can also have sub-errors which are additionally printed out
// below. They can provide additional context.
class Err {
 private:
  struct ErrInfo {
    ErrInfo(const Location& loc, const std::string& msg, const std::string& help)
        : location(loc), message(msg), help_text(help) {}

    Location location;
    Label toolchain_label;

    std::vector<LocationRange> ranges;

    std::string message;
    std::string help_text;

    std::vector<Err> sub_errs;
  };

 public:
  using RangeList = std::vector<LocationRange>;

  // Indicates no error.
  Err() = default;

  // Error at a single point.
  Err(const Location& location,
      const std::string& msg,
      const std::string& help = std::string());

  // Error at a given range.
  Err(const LocationRange& range,
      const std::string& msg,
      const std::string& help = std::string());

  // Error at a given token.
  Err(const Token& token,
      const std::string& msg,
      const std::string& help_text = std::string());

  // Error at a given node.
  Err(const ParseNode* node,
      const std::string& msg,
      const std::string& help_text = std::string());

  // Error at a given value.
  Err(const Value& value,
      const std::string& msg,
      const std::string& help_text = std::string());

  Err(const Err& other);
  Err(Err&& other) = default;

  Err& operator=(const Err&);
  Err& operator=(Err&&) = default;

  bool has_error() const { return !!info_; }

  // All getters and setters below require has_error() returns true.
  const Location& location() const { return info_->location; }
  const std::string& message() const { return info_->message; }
  const std::string& help_text() const { return info_->help_text; }

  void AppendRange(const LocationRange& range) { info_->ranges.push_back(range); }
  const RangeList& ranges() const { return info_->ranges; }

  void set_toolchain_label(const Label& toolchain_label) {
    info_->toolchain_label = toolchain_label;
  }

  void AppendSubErr(const Err& err);

  void PrintToStdout() const;

  // Prints to standard out but uses a "WARNING" messaging instead of the
  // normal "ERROR" messaging. This is a property of the printing system rather
  // than of the Err class because there is no expectation that code calling a
  // function that take an Err check that the error is nonfatal and continue.
  // Generally all Err objects with has_error() set are fatal.
  //
  // In some very specific cases code will detect a condition and print a
  // nonfatal error to the screen instead of returning it. In these cases, that
  // code can decide at printing time whether it will continue (and use this
  // method) or not (and use PrintToStdout()).
  void PrintNonfatalToStdout() const;

 private:
  void InternalPrintToStdout(bool is_sub_err, bool is_fatal) const;

  std::unique_ptr<ErrInfo> info_;  // Non-null indicates error.
};

#endif  // TOOLS_GN_ERR_H_
