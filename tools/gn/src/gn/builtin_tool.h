// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_BUILTIN_TOOL_H_
#define TOOLS_GN_BUILTIN_TOOL_H_

#include <string>

#include "gn/substitution_list.h"
#include "gn/substitution_pattern.h"
#include "gn/tool.h"

// A built-in tool that is always available regardless of toolchain. So far, the
// only example of this is the phony rule that ninja provides.
class BuiltinTool : public Tool {
 public:
  // Builtin tools
  static const char kBuiltinToolPhony[];

  // Explicit constructor. Note that |name| must be one of the kBuiltinToolXXX
  // constant pointers defined above.
  explicit BuiltinTool(const char* name);
  ~BuiltinTool();

  // Manual RTTI and required functions ---------------------------------------

  bool InitTool(Scope* block_scope, Toolchain* toolchain, Err* err);
  bool ValidateName(const char* name) const override;
  void SetComplete() override;
  bool ValidateSubstitution(const Substitution* sub_type) const override;

  BuiltinTool* AsBuiltin() override;
  const BuiltinTool* AsBuiltin() const override;

 private:
  BuiltinTool(const BuiltinTool&) = delete;
  BuiltinTool& operator=(const BuiltinTool&) = delete;
};

#endif  // TOOLS_GN_BUILTIN_TOOL_H_
