// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/c_tool.h"

#include "base/strings/stringprintf.h"
#include "gn/c_substitution_type.h"
#include "gn/target.h"

const char* CTool::kCToolCc = "cc";
const char* CTool::kCToolCxx = "cxx";
const char* CTool::kCToolCxxModule = "cxx_module";
const char* CTool::kCToolObjC = "objc";
const char* CTool::kCToolObjCxx = "objcxx";
const char* CTool::kCToolRc = "rc";
const char* CTool::kCToolAsm = "asm";
const char* CTool::kCToolSwift = "swift";
const char* CTool::kCToolAlink = "alink";
const char* CTool::kCToolSolink = "solink";
const char* CTool::kCToolSolinkModule = "solink_module";
const char* CTool::kCToolLink = "link";

CTool::CTool(const char* n)
    : Tool(n), depsformat_(DEPS_GCC), precompiled_header_type_(PCH_NONE) {
  CHECK(ValidateName(n));
  set_framework_switch("-framework ");
  set_weak_framework_switch("-weak_framework ");
  set_framework_dir_switch("-F");
  set_lib_dir_switch("-L");
  set_lib_switch("-l");
  set_linker_arg("");
}

CTool::~CTool() = default;

CTool* CTool::AsC() {
  return this;
}
const CTool* CTool::AsC() const {
  return this;
}

bool CTool::ValidateName(const char* name) const {
  return name == kCToolCc || name == kCToolCxx || name == kCToolCxxModule ||
         name == kCToolObjC || name == kCToolObjCxx || name == kCToolRc ||
         name == kCToolSwift || name == kCToolAsm || name == kCToolAlink ||
         name == kCToolSolink || name == kCToolSolinkModule ||
         name == kCToolLink;
}

void CTool::SetComplete() {
  SetToolComplete();
  link_output_.FillRequiredTypes(&substitution_bits_);
  depend_output_.FillRequiredTypes(&substitution_bits_);
}

bool CTool::ValidateRuntimeOutputs(Err* err) {
  if (runtime_outputs().list().empty())
    return true;  // Empty is always OK.

  if (name_ != kCToolSolink && name_ != kCToolSolinkModule &&
      name_ != kCToolLink) {
    *err = Err(defined_from(), "This tool specifies runtime_outputs.",
               "This is only valid for linker tools (alink doesn't count).");
    return false;
  }

  for (const SubstitutionPattern& pattern : runtime_outputs().list()) {
    if (!IsPatternInOutputList(outputs(), pattern)) {
      *err = Err(defined_from(), "This tool's runtime_outputs is bad.",
                 "It must be a subset of the outputs. The bad one is:\n  " +
                     pattern.AsString());
      return false;
    }
  }
  return true;
}

// Validates either link_output or depend_output. To generalize to either, pass
// the associated pattern, and the variable name that should appear in error
// messages.
bool CTool::ValidateLinkAndDependOutput(const SubstitutionPattern& pattern,
                                        const char* variable_name,
                                        Err* err) {
  if (pattern.empty())
    return true;  // Empty is always OK.

  // It should only be specified for certain tool types.
  if (name_ != kCToolSolink && name_ != kCToolSolinkModule) {
    *err = Err(defined_from(),
               "This tool specifies a " + std::string(variable_name) + ".",
               "This is only valid for solink and solink_module tools.");
    return false;
  }

  if (!IsPatternInOutputList(outputs(), pattern)) {
    *err = Err(defined_from(), "This tool's link_output is bad.",
               "It must match one of the outputs.");
    return false;
  }

  return true;
}

bool CTool::ReadPrecompiledHeaderType(Scope* scope, Err* err) {
  const Value* value = scope->GetValue("precompiled_header_type", true);
  if (!value)
    return true;  // Not present is fine.
  if (!value->VerifyTypeIs(Value::STRING, err))
    return false;

  if (value->string_value().empty())
    return true;  // Accept empty string, do nothing (default is "no PCH").

  if (value->string_value() == "gcc") {
    set_precompiled_header_type(PCH_GCC);
    return true;
  } else if (value->string_value() == "msvc") {
    set_precompiled_header_type(PCH_MSVC);
    return true;
  }
  *err = Err(*value, "Invalid precompiled_header_type",
             "Must either be empty, \"gcc\", or \"msvc\".");
  return false;
}

bool CTool::ReadDepsFormat(Scope* scope, Err* err) {
  const Value* value = scope->GetValue("depsformat", true);
  if (!value)
    return true;  // Not present is fine.
  if (!value->VerifyTypeIs(Value::STRING, err))
    return false;

  if (value->string_value() == "gcc") {
    set_depsformat(DEPS_GCC);
  } else if (value->string_value() == "msvc") {
    set_depsformat(DEPS_MSVC);
  } else {
    *err = Err(*value, "Deps format must be \"gcc\" or \"msvc\".");
    return false;
  }
  return true;
}

bool CTool::ReadOutputsPatternList(Scope* scope,
                                   const char* var,
                                   bool required,
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
  if (list.list().empty() && required) {
    *err =
        Err(defined_from(),
            base::StringPrintf("\"%s\" must be specified for this tool.", var));
    return false;
  }

  for (const auto& cur_type : list.required_types()) {
    if (!ValidateOutputSubstitution(cur_type)) {
      *err = Err(*value, "Pattern not valid here.",
                 "You used the pattern " + std::string(cur_type->name) +
                     " which is not valid\nfor this variable.");
      return false;
    }
  }

  *field = std::move(list);
  return true;
}

bool CTool::InitTool(Scope* scope, Toolchain* toolchain, Err* err) {
  // Initialize default vars.
  if (!Tool::InitTool(scope, toolchain, err)) {
    return false;
  }

  // All C tools should have outputs.
  if (!ReadOutputsPatternList(scope, "outputs", /*required=*/true, &outputs_,
                              err)) {
    return false;
  }

  if (!ReadDepsFormat(scope, err) || !ReadPrecompiledHeaderType(scope, err) ||
      !ReadString(scope, "framework_switch", &framework_switch_, err) ||
      !ReadString(scope, "weak_framework_switch", &weak_framework_switch_,
                  err) ||
      !ReadString(scope, "framework_dir_switch", &framework_dir_switch_, err) ||
      !ReadString(scope, "lib_switch", &lib_switch_, err) ||
      !ReadString(scope, "lib_dir_switch", &lib_dir_switch_, err) ||
      !ReadPattern(scope, "link_output", &link_output_, err) ||
      !ReadString(scope, "swiftmodule_switch", &swiftmodule_switch_, err) ||
      !ReadPattern(scope, "depend_output", &depend_output_, err)) {
    return false;
  }

  // Swift tool can optionally specify partial_outputs.
  if (name_ == kCToolSwift) {
    if (!ReadOutputsPatternList(scope, "partial_outputs", /*required=*/false,
                                &partial_outputs_, err)) {
      return false;
    }
  }

  // Validate link_output and depend_output.
  if (!ValidateLinkAndDependOutput(link_output(), "link_output", err)) {
    return false;
  }
  if (!ValidateLinkAndDependOutput(depend_output(), "depend_output", err)) {
    return false;
  }
  if ((!link_output().empty() && depend_output().empty()) ||
      (link_output().empty() && !depend_output().empty())) {
    *err = Err(defined_from(),
               "Both link_output and depend_output should either "
               "be specified or they should both be empty.");
    return false;
  }

  if (!ValidateRuntimeOutputs(err)) {
    return false;
  }
  return true;
}

bool CTool::ValidateSubstitution(const Substitution* sub_type) const {
  if (name_ == kCToolCc || name_ == kCToolCxx || name_ == kCToolCxxModule ||
      name_ == kCToolObjC || name_ == kCToolObjCxx || name_ == kCToolRc ||
      name_ == kCToolAsm)
    return IsValidCompilerSubstitution(sub_type);
  if (name_ == kCToolSwift)
    return IsValidSwiftCompilerSubstitution(sub_type);
  else if (name_ == kCToolAlink)
    return IsValidALinkSubstitution(sub_type);
  else if (name_ == kCToolSolink || name_ == kCToolSolinkModule ||
           name_ == kCToolLink)
    return IsValidLinkerSubstitution(sub_type);
  NOTREACHED();
  return false;
}

bool CTool::ValidateOutputSubstitution(const Substitution* sub_type) const {
  if (name_ == kCToolCc || name_ == kCToolCxx || name_ == kCToolCxxModule ||
      name_ == kCToolObjC || name_ == kCToolObjCxx || name_ == kCToolRc ||
      name_ == kCToolAsm)
    return IsValidCompilerOutputsSubstitution(sub_type);
  if (name_ == kCToolSwift)
    return IsValidSwiftCompilerOutputsSubstitution(sub_type);
  // ALink uses the standard output file patterns as other linker tools.
  else if (name_ == kCToolAlink || name_ == kCToolSolink ||
           name_ == kCToolSolinkModule || name_ == kCToolLink)
    return IsValidLinkerOutputsSubstitution(sub_type);
  NOTREACHED();
  return false;
}
