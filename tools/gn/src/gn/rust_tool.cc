// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/rust_tool.h"

#include "gn/rust_substitution_type.h"
#include "gn/target.h"

const char* RustTool::kRsToolBin = "rust_bin";
const char* RustTool::kRsToolCDylib = "rust_cdylib";
const char* RustTool::kRsToolDylib = "rust_dylib";
const char* RustTool::kRsToolMacro = "rust_macro";
const char* RustTool::kRsToolRlib = "rust_rlib";
const char* RustTool::kRsToolStaticlib = "rust_staticlib";

RustTool::RustTool(const char* n) : Tool(n) {
  CHECK(ValidateName(n));
  // TODO: should these be settable in toolchain definition?
  set_framework_switch("-lframework=");
  set_lib_dir_switch("-Lnative=");
  set_lib_switch("-l");
  set_linker_arg("-Clink-arg=");
}

RustTool::~RustTool() = default;

RustTool* RustTool::AsRust() {
  return this;
}
const RustTool* RustTool::AsRust() const {
  return this;
}

bool RustTool::ValidateName(const char* name) const {
  return name == kRsToolBin || name == kRsToolCDylib || name == kRsToolDylib ||
         name == kRsToolMacro || name == kRsToolRlib ||
         name == kRsToolStaticlib;
}

bool RustTool::MayLink() const {
  return name_ == kRsToolBin || name_ == kRsToolCDylib || name_ == kRsToolDylib ||
         name_ == kRsToolMacro;
}

void RustTool::SetComplete() {
  SetToolComplete();
}

std::string_view RustTool::GetSysroot() const {
  return rust_sysroot_;
}

bool RustTool::SetOutputExtension(const Value* value,
                                  std::string* var,
                                  Err* err) {
  DCHECK(!complete_);
  if (!value)
    return true;  // Not present is fine.
  if (!value->VerifyTypeIs(Value::STRING, err))
    return false;
  if (value->string_value().empty())
    return true;

  *var = std::move(value->string_value());
  return true;
}

bool RustTool::ReadOutputsPatternList(Scope* scope,
                                      const char* var,
                                      SubstitutionList* field,
                                      Err* err) {
  DCHECK(!complete_);
  const Value* value = scope->GetValue(var, true);
  if (!value)
    return true;  // Not present is fine.
  if (!value->VerifyTypeIs(Value::LIST, err))
    return false;

  SubstitutionList list;
  if (!list.Parse(*value, err))
    return false;

  // Validate the right kinds of patterns are used.
  if (list.list().empty()) {
    *err = Err(defined_from(), "\"outputs\" must be specified for this tool.");
    return false;
  }

  for (const auto& cur_type : list.required_types()) {
    if (!IsValidRustSubstitution(cur_type)) {
      *err = Err(*value, "Pattern not valid here.",
                 "You used the pattern " + std::string(cur_type->name) +
                     " which is not valid\nfor this variable.");
      return false;
    }
  }

  *field = std::move(list);
  return true;
}

bool RustTool::InitTool(Scope* scope, Toolchain* toolchain, Err* err) {
  // Initialize default vars.
  if (!Tool::InitTool(scope, toolchain, err)) {
    return false;
  }

  // All Rust tools should have outputs.
  if (!ReadOutputsPatternList(scope, "outputs", &outputs_, err)) {
    return false;
  }

  // Check for a sysroot. Sets an empty string when not explicitly set.
  if (!ReadString(scope, "rust_sysroot", &rust_sysroot_, err)) {
    return false;
  }
  return true;
}

bool RustTool::ValidateSubstitution(const Substitution* sub_type) const {
  if (MayLink())
    return IsValidRustLinkerSubstitution(sub_type);
  if (ValidateName(name_))
    return IsValidRustSubstitution(sub_type);
  NOTREACHED();
  return false;
}
