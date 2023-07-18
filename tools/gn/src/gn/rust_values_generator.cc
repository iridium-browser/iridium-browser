// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/rust_values_generator.h"

#include "gn/config_values_generator.h"
#include "gn/err.h"
#include "gn/filesystem_utils.h"
#include "gn/functions.h"
#include "gn/parse_tree.h"
#include "gn/rust_variables.h"
#include "gn/scope.h"
#include "gn/target.h"
#include "gn/value_extractors.h"

static const char* kRustSupportedCrateTypesError =
    "\"crate_type\" must be one of \"bin\", \"cdylib\", \"dylib\", or "
    "\"proc-macro\", \"rlib\", \"staticlib\".";

RustValuesGenerator::RustValuesGenerator(Target* target,
                                         Scope* scope,
                                         const FunctionCallNode* function_call,
                                         Err* err)
    : target_(target),
      scope_(scope),
      function_call_(function_call),
      err_(err) {}

RustValuesGenerator::~RustValuesGenerator() = default;

void RustValuesGenerator::Run() {
  // source_set targets don't need any special Rust handling.
  if (target_->output_type() == Target::SOURCE_SET)
    return;

  // Check that this type of target is Rust-supported.
  if (target_->output_type() != Target::EXECUTABLE &&
      target_->output_type() != Target::SHARED_LIBRARY &&
      target_->output_type() != Target::RUST_LIBRARY &&
      target_->output_type() != Target::RUST_PROC_MACRO &&
      target_->output_type() != Target::STATIC_LIBRARY &&
      target_->output_type() != Target::LOADABLE_MODULE) {
    // Only valid rust output types.
    *err_ = Err(function_call_,
                "Target type \"" +
                    std::string(Target::GetStringForOutputType(
                        target_->output_type())) +
                    "\" is not supported for Rust compilation.",
                "Supported target types are \"executable\", \"loadable_module\""
                "\"shared_library\", \"static_library\", or \"source_set\".");
    return;
  }

  if (!FillCrateName())
    return;

  if (!FillCrateType())
    return;

  if (!FillCrateRoot())
    return;

  if (!FillAliasedDeps())
    return;
}

bool RustValuesGenerator::FillCrateName() {
  const Value* value = scope_->GetValue(variables::kRustCrateName, true);
  if (!value) {
    // The target name will be used.
    target_->rust_values().crate_name() = target_->label().name();
    return true;
  }
  if (!value->VerifyTypeIs(Value::STRING, err_))
    return false;

  target_->rust_values().crate_name() = std::move(value->string_value());
  return true;
}

bool RustValuesGenerator::FillCrateType() {
  const Value* value = scope_->GetValue(variables::kRustCrateType, true);
  if (!value) {
    // Require shared_library and loadable_module targets to tell us what
    // they want.
    if (target_->output_type() == Target::SHARED_LIBRARY ||
        target_->output_type() == Target::LOADABLE_MODULE) {
      *err_ = Err(function_call_,
                  "Must set \"crate_type\" on a Rust \"shared_library\".",
                  kRustSupportedCrateTypesError);
      return false;
    }

    return true;
  }

  if (!value->VerifyTypeIs(Value::STRING, err_))
    return false;

  if (value->string_value() == "bin") {
    target_->rust_values().set_crate_type(RustValues::CRATE_BIN);
    return true;
  }
  if (value->string_value() == "cdylib") {
    target_->rust_values().set_crate_type(RustValues::CRATE_CDYLIB);
    return true;
  }
  if (value->string_value() == "dylib") {
    target_->rust_values().set_crate_type(RustValues::CRATE_DYLIB);
    return true;
  }
  if (value->string_value() == "proc-macro") {
    target_->rust_values().set_crate_type(RustValues::CRATE_PROC_MACRO);
    return true;
  }
  if (value->string_value() == "rlib") {
    target_->rust_values().set_crate_type(RustValues::CRATE_RLIB);
    return true;
  }
  if (value->string_value() == "staticlib") {
    target_->rust_values().set_crate_type(RustValues::CRATE_STATICLIB);
    return true;
  }

  *err_ = Err(value->origin(),
              "Inadmissible crate type \"" + value->string_value() + "\".",
              kRustSupportedCrateTypesError);
  return false;
}

bool RustValuesGenerator::FillCrateRoot() {
  const Value* value = scope_->GetValue(variables::kRustCrateRoot, true);
  if (!value) {
    // If there's only one source, use that.
    if (target_->sources().size() == 1) {
      target_->rust_values().set_crate_root(target_->sources()[0]);
      return true;
    }
    // Otherwise, see if "lib.rs" or "main.rs" (as relevant) are in sources.
    std::string to_find =
        target_->output_type() == Target::EXECUTABLE ? "main.rs" : "lib.rs";
    for (auto& source : target_->sources()) {
      if (source.GetName() == to_find) {
        target_->rust_values().set_crate_root(source);
        return true;
      }
    }
    *err_ = Err(function_call_, "Missing \"crate_root\" and missing \"" +
                                    to_find + "\" in sources.");
    return false;
  }

  if (!value->VerifyTypeIs(Value::STRING, err_))
    return false;

  SourceFile dest;
  if (!ExtractRelativeFile(scope_->settings()->build_settings(), *value,
                           scope_->GetSourceDir(), &dest, err_))
    return false;

  target_->rust_values().set_crate_root(dest);
  return true;
}

bool RustValuesGenerator::FillAliasedDeps() {
  const Value* value = scope_->GetValue(variables::kRustAliasedDeps, true);
  if (!value)
    return true;

  if (!value->VerifyTypeIs(Value::SCOPE, err_))
    return false;

  Scope::KeyValueMap aliased_deps;
  value->scope_value()->GetCurrentScopeValues(&aliased_deps);
  for (const auto& pair : aliased_deps) {
    Label dep_label =
        Label::Resolve(scope_->GetSourceDir(),
                       scope_->settings()->build_settings()->root_path_utf8(),
                       ToolchainLabelForScope(scope_), pair.second, err_);

    if (err_->has_error())
      return false;

    // Insert into the aliased_deps map.
    target_->rust_values().aliased_deps().emplace(std::move(dep_label),
                                                  pair.first);
  }

  return true;
}
