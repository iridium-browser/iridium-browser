// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_GENERAL_TOOL_H_
#define TOOLS_GN_GENERAL_TOOL_H_

#include <string>

#include "base/logging.h"
#include "gn/label.h"
#include "gn/label_ptr.h"
#include "gn/substitution_list.h"
#include "gn/substitution_pattern.h"
#include "gn/tool.h"

class GeneralTool : public Tool {
 public:
  // General tools
  static const char* kGeneralToolStamp;
  static const char* kGeneralToolCopy;
  static const char* kGeneralToolAction;

  // Platform-specific tools
  static const char* kGeneralToolCopyBundleData;
  static const char* kGeneralToolCompileXCAssets;

  GeneralTool(const char* n);
  ~GeneralTool();

  // Manual RTTI and required functions ---------------------------------------

  bool InitTool(Scope* block_scope, Toolchain* toolchain, Err* err);
  bool ValidateName(const char* name) const override;
  void SetComplete() override;
  bool ValidateSubstitution(const Substitution* sub_type) const override;

  GeneralTool* AsGeneral() override;
  const GeneralTool* AsGeneral() const override;

 private:
  GeneralTool(const GeneralTool&) = delete;
  GeneralTool& operator=(const GeneralTool&) = delete;
};

#endif  // TOOLS_GN_GENERAL_TOOL_H_
