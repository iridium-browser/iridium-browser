// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/builtin_tool.h"
#include "base/logging.h"
#include "gn/target.h"

// static
const char BuiltinTool::kBuiltinToolPhony[] = "phony";

BuiltinTool::BuiltinTool(const char* n) : Tool(n) {
  CHECK(ValidateName(n));
  // Unlike regular tools, which are read from a file,
  // builtin-tools are always ready to go and do not need
  // phased construction.
  SetToolComplete();
}

BuiltinTool::~BuiltinTool() = default;

BuiltinTool* BuiltinTool::AsBuiltin() {
  return this;
}
const BuiltinTool* BuiltinTool::AsBuiltin() const {
  return this;
}

bool BuiltinTool::ValidateName(const char* name) const {
  return name == kBuiltinToolPhony;
}

void BuiltinTool::SetComplete() {
  // Already performed in constructor
}

bool BuiltinTool::InitTool(Scope* scope, Toolchain* toolchain, Err* err) {
  // Initialize default vars.
  return Tool::InitTool(scope, toolchain, err);
}

bool BuiltinTool::ValidateSubstitution(const Substitution* sub_type) const {
  if (name_ == kBuiltinToolPhony)
    return IsValidToolSubstitution(sub_type);
  NOTREACHED();
  return false;
}
