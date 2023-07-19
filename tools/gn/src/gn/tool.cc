// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/tool.h"

#include "gn/builtin_tool.h"
#include "gn/c_tool.h"
#include "gn/general_tool.h"
#include "gn/rust_tool.h"
#include "gn/settings.h"
#include "gn/target.h"

const char* Tool::kToolNone = "";

Tool::Tool(const char* n) : name_(n) {}

Tool::~Tool() = default;

void Tool::SetToolComplete() {
  DCHECK(!complete_);
  complete_ = true;

  command_.FillRequiredTypes(&substitution_bits_);
  depfile_.FillRequiredTypes(&substitution_bits_);
  description_.FillRequiredTypes(&substitution_bits_);
  outputs_.FillRequiredTypes(&substitution_bits_);
  rspfile_.FillRequiredTypes(&substitution_bits_);
  rspfile_content_.FillRequiredTypes(&substitution_bits_);
  partial_outputs_.FillRequiredTypes(&substitution_bits_);
}

GeneralTool* Tool::AsGeneral() {
  return nullptr;
}

const GeneralTool* Tool::AsGeneral() const {
  return nullptr;
}

CTool* Tool::AsC() {
  return nullptr;
}

const CTool* Tool::AsC() const {
  return nullptr;
}

RustTool* Tool::AsRust() {
  return nullptr;
}
const RustTool* Tool::AsRust() const {
  return nullptr;
}

BuiltinTool* Tool::AsBuiltin() {
  return nullptr;
}
const BuiltinTool* Tool::AsBuiltin() const {
  return nullptr;
}

bool Tool::IsPatternInOutputList(const SubstitutionList& output_list,
                                 const SubstitutionPattern& pattern) const {
  for (const auto& cur : output_list.list()) {
    if (pattern.ranges().size() == cur.ranges().size() &&
        std::equal(pattern.ranges().begin(), pattern.ranges().end(),
                   cur.ranges().begin()))
      return true;
  }
  return false;
}

bool Tool::ValidateSubstitutionList(
    const std::vector<const Substitution*>& list,
    const Value* origin,
    Err* err) const {
  for (const auto& cur_type : list) {
    if (!ValidateSubstitution(cur_type)) {
      *err = Err(*origin, "Pattern not valid here.",
                 "You used the pattern " + std::string(cur_type->name) +
                     " which is not valid\nfor this variable.");
      return false;
    }
  }
  return true;
}

bool Tool::ReadBool(Scope* scope, const char* var, bool* field, Err* err) {
  DCHECK(!complete_);
  const Value* v = scope->GetValue(var, true);
  if (!v)
    return true;  // Not present is fine.
  if (!v->VerifyTypeIs(Value::BOOLEAN, err))
    return false;
  *field = v->boolean_value();
  return true;
}

bool Tool::ReadString(Scope* scope,
                      const char* var,
                      std::string* field,
                      Err* err) {
  DCHECK(!complete_);
  const Value* v = scope->GetValue(var, true);
  if (!v)
    return true;  // Not present is fine.
  if (!v->VerifyTypeIs(Value::STRING, err))
    return false;
  *field = v->string_value();
  return true;
}

bool Tool::ReadPattern(Scope* scope,
                       const char* var,
                       SubstitutionPattern* field,
                       Err* err) {
  DCHECK(!complete_);
  const Value* value = scope->GetValue(var, true);
  if (!value)
    return true;  // Not present is fine.
  if (!value->VerifyTypeIs(Value::STRING, err))
    return false;

  SubstitutionPattern pattern;
  if (!pattern.Parse(*value, err))
    return false;
  if (!ValidateSubstitutionList(pattern.required_types(), value, err))
    return false;

  *field = std::move(pattern);
  return true;
}

bool Tool::ReadPatternList(Scope* scope,
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
  if (!ValidateSubstitutionList(list.required_types(), value, err))
    return false;

  *field = std::move(list);
  return true;
}

bool Tool::ReadLabel(Scope* scope,
                     const char* var,
                     const Label& current_toolchain,
                     LabelPtrPair<Pool>* field,
                     Err* err) {
  DCHECK(!complete_);
  const Value* v = scope->GetValue(var, true);
  if (!v)
    return true;  // Not present is fine.

  Label label =
      Label::Resolve(scope->GetSourceDir(),
                     scope->settings()->build_settings()->root_path_utf8(),
                     current_toolchain, *v, err);
  if (err->has_error())
    return false;

  LabelPtrPair<Pool> pair(label);
  pair.origin = defined_from();

  *field = std::move(pair);
  return true;
}

bool Tool::ReadOutputExtension(Scope* scope, Err* err) {
  DCHECK(!complete_);
  const Value* value = scope->GetValue("default_output_extension", true);
  if (!value)
    return true;  // Not present is fine.
  if (!value->VerifyTypeIs(Value::STRING, err))
    return false;

  if (value->string_value().empty())
    return true;  // Accept empty string.

  if (value->string_value()[0] != '.') {
    *err = Err(*value, "default_output_extension must begin with a '.'");
    return false;
  }

  set_default_output_extension(value->string_value());
  return true;
}

bool Tool::InitTool(Scope* scope, Toolchain* toolchain, Err* err) {
  if (!ReadPattern(scope, "command", &command_, err) ||
      !ReadString(scope, "command_launcher", &command_launcher_, err) ||
      !ReadOutputExtension(scope, err) ||
      !ReadPattern(scope, "depfile", &depfile_, err) ||
      !ReadPattern(scope, "description", &description_, err) ||
      !ReadPatternList(scope, "runtime_outputs", &runtime_outputs_, err) ||
      !ReadString(scope, "output_prefix", &output_prefix_, err) ||
      !ReadPattern(scope, "default_output_dir", &default_output_dir_, err) ||
      !ReadBool(scope, "restat", &restat_, err) ||
      !ReadPattern(scope, "rspfile", &rspfile_, err) ||
      !ReadPattern(scope, "rspfile_content", &rspfile_content_, err) ||
      !ReadLabel(scope, "pool", toolchain->label(), &pool_, err)) {
    return false;
  }
  const bool command_is_required = name_ != GeneralTool::kGeneralToolAction;
  if (command_.empty() == command_is_required) {
    *err = Err(defined_from(), "This tool's command is bad.",
               command_is_required
                   ? "This tool requires \"command\" to be defined."
                   : "This tool doesn't support \"command\".");
    return false;
  }
  return true;
}

std::unique_ptr<Tool> Tool::CreateTool(const ParseNode* function,
                                       const std::string& name,
                                       Scope* scope,
                                       Toolchain* toolchain,
                                       Err* err) {
  std::unique_ptr<Tool> tool = CreateTool(name);
  if (!tool) {
    *err = Err(function, "Unknown tool type.");
    return nullptr;
  }
  if (CTool* c_tool = tool->AsC()) {
    if (c_tool->InitTool(scope, toolchain, err))
      return tool;
    return nullptr;
  }
  if (GeneralTool* general_tool = tool->AsGeneral()) {
    if (general_tool->InitTool(scope, toolchain, err))
      return tool;
    return nullptr;
  }
  if (RustTool* rust_tool = tool->AsRust()) {
    if (rust_tool->InitTool(scope, toolchain, err))
      return tool;
    return nullptr;
  }
  NOTREACHED();
  *err = Err(function, "Unknown tool type.");
  return nullptr;
}

// static
std::unique_ptr<Tool> Tool::CreateTool(const std::string& name) {
  // C tools
  if (name == CTool::kCToolCc)
    return std::make_unique<CTool>(CTool::kCToolCc);
  else if (name == CTool::kCToolCxx)
    return std::make_unique<CTool>(CTool::kCToolCxx);
  else if (name == CTool::kCToolCxxModule)
    return std::make_unique<CTool>(CTool::kCToolCxxModule);
  else if (name == CTool::kCToolObjC)
    return std::make_unique<CTool>(CTool::kCToolObjC);
  else if (name == CTool::kCToolObjCxx)
    return std::make_unique<CTool>(CTool::kCToolObjCxx);
  else if (name == CTool::kCToolRc)
    return std::make_unique<CTool>(CTool::kCToolRc);
  else if (name == CTool::kCToolAsm)
    return std::make_unique<CTool>(CTool::kCToolAsm);
  else if (name == CTool::kCToolSwift)
    return std::make_unique<CTool>(CTool::kCToolSwift);
  else if (name == CTool::kCToolAlink)
    return std::make_unique<CTool>(CTool::kCToolAlink);
  else if (name == CTool::kCToolSolink)
    return std::make_unique<CTool>(CTool::kCToolSolink);
  else if (name == CTool::kCToolSolinkModule)
    return std::make_unique<CTool>(CTool::kCToolSolinkModule);
  else if (name == CTool::kCToolLink)
    return std::make_unique<CTool>(CTool::kCToolLink);

  // General tools
  else if (name == GeneralTool::kGeneralToolAction)
    return std::make_unique<GeneralTool>(GeneralTool::kGeneralToolAction);
  else if (name == GeneralTool::kGeneralToolStamp)
    return std::make_unique<GeneralTool>(GeneralTool::kGeneralToolStamp);
  else if (name == GeneralTool::kGeneralToolCopy)
    return std::make_unique<GeneralTool>(GeneralTool::kGeneralToolCopy);
  else if (name == GeneralTool::kGeneralToolCopyBundleData)
    return std::make_unique<GeneralTool>(
        GeneralTool::kGeneralToolCopyBundleData);
  else if (name == GeneralTool::kGeneralToolCompileXCAssets)
    return std::make_unique<GeneralTool>(
        GeneralTool::kGeneralToolCompileXCAssets);

  // Rust tool
  else if (name == RustTool::kRsToolBin)
    return std::make_unique<RustTool>(RustTool::kRsToolBin);
  else if (name == RustTool::kRsToolCDylib)
    return std::make_unique<RustTool>(RustTool::kRsToolCDylib);
  else if (name == RustTool::kRsToolDylib)
    return std::make_unique<RustTool>(RustTool::kRsToolDylib);
  else if (name == RustTool::kRsToolMacro)
    return std::make_unique<RustTool>(RustTool::kRsToolMacro);
  else if (name == RustTool::kRsToolRlib)
    return std::make_unique<RustTool>(RustTool::kRsToolRlib);
  else if (name == RustTool::kRsToolStaticlib)
    return std::make_unique<RustTool>(RustTool::kRsToolStaticlib);

  return nullptr;
}

// static
const char* Tool::GetToolTypeForSourceType(SourceFile::Type type) {
  switch (type) {
    case SourceFile::SOURCE_C:
      return CTool::kCToolCc;
    case SourceFile::SOURCE_CPP:
      return CTool::kCToolCxx;
    case SourceFile::SOURCE_MODULEMAP:
      return CTool::kCToolCxxModule;
    case SourceFile::SOURCE_M:
      return CTool::kCToolObjC;
    case SourceFile::SOURCE_MM:
      return CTool::kCToolObjCxx;
    case SourceFile::SOURCE_ASM:
    case SourceFile::SOURCE_S:
      return CTool::kCToolAsm;
    case SourceFile::SOURCE_RC:
      return CTool::kCToolRc;
    case SourceFile::SOURCE_RS:
      return RustTool::kRsToolBin;
    case SourceFile::SOURCE_SWIFT:
      return CTool::kCToolSwift;
    case SourceFile::SOURCE_UNKNOWN:
    case SourceFile::SOURCE_H:
    case SourceFile::SOURCE_O:
    case SourceFile::SOURCE_DEF:
    case SourceFile::SOURCE_GO:
      return kToolNone;
    default:
      NOTREACHED();
      return kToolNone;
  }
}

// static
const char* Tool::GetToolTypeForTargetFinalOutput(const Target* target) {
  // The contents of this list might be surprising (i.e. phony tool for copy
  // rules). See the header for why.
  // TODO(crbug.com/gn/39): Don't emit stamp files for single-output targets.
  if (target->source_types_used().RustSourceUsed()) {
    switch (target->rust_values().InferredCrateType(target)) {
      case RustValues::CRATE_BIN:
        return RustTool::kRsToolBin;
      case RustValues::CRATE_CDYLIB:
        return RustTool::kRsToolCDylib;
      case RustValues::CRATE_DYLIB:
        return RustTool::kRsToolDylib;
      case RustValues::CRATE_PROC_MACRO:
        return RustTool::kRsToolMacro;
      case RustValues::CRATE_RLIB:
        return RustTool::kRsToolRlib;
      case RustValues::CRATE_STATICLIB:
        return RustTool::kRsToolStaticlib;
      case RustValues::CRATE_AUTO:
        break;
      default:
        NOTREACHED();
    }
  }
  switch (target->output_type()) {
    case Target::EXECUTABLE:
      return CTool::kCToolLink;
    case Target::SHARED_LIBRARY:
      return CTool::kCToolSolink;
    case Target::LOADABLE_MODULE:
      return CTool::kCToolSolinkModule;
    case Target::STATIC_LIBRARY:
      return CTool::kCToolAlink;
    case Target::ACTION:
    case Target::ACTION_FOREACH:
    case Target::BUNDLE_DATA:
    case Target::COPY_FILES:
    case Target::CREATE_BUNDLE:
    case Target::GENERATED_FILE:
    case Target::GROUP:
    case Target::SOURCE_SET:
      if (target->settings()->build_settings()->no_stamp_files()) {
        return BuiltinTool::kBuiltinToolPhony;
      } else {
        return GeneralTool::kGeneralToolStamp;
      }
    default:
      NOTREACHED();
      return kToolNone;
  }
}
