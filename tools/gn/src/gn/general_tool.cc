// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/general_tool.h"
#include "gn/target.h"

const char* GeneralTool::kGeneralToolStamp = "stamp";
const char* GeneralTool::kGeneralToolCopy = "copy";
const char* GeneralTool::kGeneralToolCopyBundleData = "copy_bundle_data";
const char* GeneralTool::kGeneralToolCompileXCAssets = "compile_xcassets";
const char* GeneralTool::kGeneralToolAction = "action";

GeneralTool::GeneralTool(const char* n) : Tool(n) {
  CHECK(ValidateName(n));
}

GeneralTool::~GeneralTool() = default;

GeneralTool* GeneralTool::AsGeneral() {
  return this;
}
const GeneralTool* GeneralTool::AsGeneral() const {
  return this;
}

bool GeneralTool::ValidateName(const char* name) const {
  return name == kGeneralToolStamp || name == kGeneralToolCopy ||
         name == kGeneralToolCopyBundleData ||
         name == kGeneralToolCompileXCAssets || name == kGeneralToolAction;
}

void GeneralTool::SetComplete() {
  SetToolComplete();
}

bool GeneralTool::InitTool(Scope* scope, Toolchain* toolchain, Err* err) {
  // Initialize default vars.
  return Tool::InitTool(scope, toolchain, err);
}

bool GeneralTool::ValidateSubstitution(const Substitution* sub_type) const {
  if (name_ == kGeneralToolStamp || name_ == kGeneralToolAction)
    return IsValidToolSubstitution(sub_type);
  else if (name_ == kGeneralToolCopy || name_ == kGeneralToolCopyBundleData)
    return IsValidCopySubstitution(sub_type);
  else if (name_ == kGeneralToolCompileXCAssets)
    return IsValidCompileXCassetsSubstitution(sub_type);
  NOTREACHED();
  return false;
}
