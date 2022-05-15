// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "gn/err.h"
#include "gn/filesystem_utils.h"
#include "gn/functions.h"
#include "gn/parse_tree.h"
#include "gn/scope.h"
#include "gn/value.h"

namespace functions {

const char kFilterExclude[] = "filter_exclude";
const char kFilterExclude_HelpShort[] =
    "filter_exclude: Remove values that match a set of patterns.";
const char kFilterExclude_Help[] =
    R"(filter_exclude: Remove values that match a set of patterns.

  filter_exclude(values, exclude_patterns)

  The argument values must be a list of strings.

  The argument exclude_patterns must be a list of file patterns (see
  "gn help file_pattern"). Any elements in values matching at least one
  of those patterns will be excluded.

Examples
  values = [ "foo.cc", "foo.h", "foo.proto" ]
  result = filter_exclude(values, [ "*.proto" ])
  # result will be [ "foo.cc", "foo.h" ]
)";

const char kFilterInclude[] = "filter_include";
const char kFilterInclude_HelpShort[] =
    "filter_include: Remove values that do not match a set of patterns.";
const char kFilterInclude_Help[] =
    R"(filter_include: Remove values that do not match a set of patterns.

  filter_include(values, include_patterns)

  The argument values must be a list of strings.

  The argument include_patterns must be a list of file patterns (see
  "gn help file_pattern"). Only elements from values matching at least
  one of the pattern will be included.

Examples
  values = [ "foo.cc", "foo.h", "foo.proto" ]
  result = filter_include(values, [ "*.proto" ])
  # result will be [ "foo.proto" ]
)";

namespace {

enum FilterSelection {
  kExcludeFilter,
  kIncludeFilter,
};

Value RunFilter(Scope* scope,
                const FunctionCallNode* function,
                const std::vector<Value>& args,
                FilterSelection selection,
                Err* err) {
  if (args.size() != 2) {
    *err = Err(function, "Expecting exactly two arguments.");
    return Value();
  }

  // Extract "values".
  if (args[0].type() != Value::LIST) {
    *err = Err(args[0], "First argument must be a list of strings.");
    return Value();
  }

  // Extract "patterns".
  PatternList patterns;
  patterns.SetFromValue(args[1], err);
  if (err->has_error())
    return Value();

  Value result(function, Value::LIST);
  for (const auto& value : args[0].list_value()) {
    if (value.type() != Value::STRING) {
      *err = Err(args[0], "First argument must be a list of strings.");
      return Value();
    }

    const bool matches_pattern = patterns.MatchesValue(value);
    switch (selection) {
      case kIncludeFilter:
        if (matches_pattern)
          result.list_value().push_back(value);
        break;

      case kExcludeFilter:
        if (!matches_pattern)
          result.list_value().push_back(value);
        break;
    }
  }
  return result;
}

}  // anonymous namespace

Value RunFilterExclude(Scope* scope,
                       const FunctionCallNode* function,
                       const std::vector<Value>& args,
                       Err* err) {
  return RunFilter(scope, function, args, kExcludeFilter, err);
}

Value RunFilterInclude(Scope* scope,
                       const FunctionCallNode* function,
                       const std::vector<Value>& args,
                       Err* err) {
  return RunFilter(scope, function, args, kIncludeFilter, err);
}

}  // namespace functions
