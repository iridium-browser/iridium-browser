// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_RUST_TOOL_H_
#define TOOLS_GN_RUST_TOOL_H_

#include <string>
#include <string_view>

#include "base/logging.h"
#include "gn/label.h"
#include "gn/label_ptr.h"
#include "gn/rust_values.h"
#include "gn/source_file.h"
#include "gn/substitution_list.h"
#include "gn/substitution_pattern.h"
#include "gn/target.h"
#include "gn/tool.h"

class RustTool : public Tool {
 public:
  // Rust tools
  static const char* kRsToolBin;
  static const char* kRsToolCDylib;
  static const char* kRsToolDylib;
  static const char* kRsToolMacro;
  static const char* kRsToolRlib;
  static const char* kRsToolStaticlib;

  explicit RustTool(const char* n);
  ~RustTool();

  // Manual RTTI and required functions ---------------------------------------

  bool InitTool(Scope* block_scope, Toolchain* toolchain, Err* err);
  bool ValidateName(const char* name) const override;
  void SetComplete() override;
  bool ValidateSubstitution(const Substitution* sub_type) const override;
  bool MayLink() const;

  RustTool* AsRust() override;
  const RustTool* AsRust() const override;

  std::string_view GetSysroot() const;

  const std::string& dynamic_link_switch() const { return dynamic_link_switch_; }
  void set_dynamic_link_switch(std::string s) {
    DCHECK(!complete_);
    dynamic_link_switch_ = std::move(s);
  }

 private:
  std::string rust_sysroot_;
  std::string dynamic_link_switch_;

  bool SetOutputExtension(const Value* value, std::string* var, Err* err);
  bool ReadOutputsPatternList(Scope* scope,
                              const char* var,
                              SubstitutionList* field,
                              Err* err);

  RustTool(const RustTool&) = delete;
  RustTool& operator=(const RustTool&) = delete;
};

#endif  // TOOLS_GN_RUST_TOOL_H_
