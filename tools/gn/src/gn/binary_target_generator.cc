// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/binary_target_generator.h"

#include "gn/config_values_generator.h"
#include "gn/deps_iterator.h"
#include "gn/err.h"
#include "gn/filesystem_utils.h"
#include "gn/functions.h"
#include "gn/parse_tree.h"
#include "gn/rust_values_generator.h"
#include "gn/rust_variables.h"
#include "gn/scope.h"
#include "gn/settings.h"
#include "gn/swift_values_generator.h"
#include "gn/value_extractors.h"
#include "gn/variables.h"

BinaryTargetGenerator::BinaryTargetGenerator(
    Target* target,
    Scope* scope,
    const FunctionCallNode* function_call,
    Target::OutputType type,
    Err* err)
    : TargetGenerator(target, scope, function_call, err), output_type_(type) {}

BinaryTargetGenerator::~BinaryTargetGenerator() = default;

void BinaryTargetGenerator::DoRun() {
  target_->set_output_type(output_type_);

  if (!FillOutputName())
    return;

  if (!FillOutputPrefixOverride())
    return;

  if (!FillOutputDir())
    return;

  if (!FillOutputExtension())
    return;

  if (!FillSources())
    return;

  if (!FillPublic())
    return;

  if (!FillFriends())
    return;

  if (!FillCheckIncludes())
    return;

  if (!FillConfigs())
    return;

  if (!FillAllowCircularIncludesFrom())
    return;

  if (!FillCompleteStaticLib())
    return;

  if (!FillPool())
    return;

  if (!ValidateSources())
    return;

  if (target_->source_types_used().RustSourceUsed()) {
    RustValuesGenerator rustgen(target_, scope_, function_call_, err_);
    rustgen.Run();
    if (err_->has_error())
      return;
  }

  if (target_->source_types_used().SwiftSourceUsed()) {
    SwiftValuesGenerator swiftgen(target_, scope_, err_);
    swiftgen.Run();
    if (err_->has_error())
      return;
  }

  // Config values (compiler flags, etc.) set directly on this target.
  ConfigValuesGenerator gen(&target_->config_values(), scope_,
                            scope_->GetSourceDir(), err_);
  gen.Run();
  if (err_->has_error())
    return;
}

bool BinaryTargetGenerator::FillSources() {
  bool ret = TargetGenerator::FillSources();
  for (std::size_t i = 0; i < target_->sources().size(); ++i) {
    const auto& source = target_->sources()[i];
    const SourceFile::Type source_type = source.GetType();
    switch (source_type) {
      case SourceFile::SOURCE_CPP:
      case SourceFile::SOURCE_MODULEMAP:
      case SourceFile::SOURCE_H:
      case SourceFile::SOURCE_C:
      case SourceFile::SOURCE_M:
      case SourceFile::SOURCE_MM:
      case SourceFile::SOURCE_S:
      case SourceFile::SOURCE_ASM:
      case SourceFile::SOURCE_O:
      case SourceFile::SOURCE_DEF:
      case SourceFile::SOURCE_GO:
      case SourceFile::SOURCE_RS:
      case SourceFile::SOURCE_RC:
      case SourceFile::SOURCE_SWIFT:
        // These are allowed.
        break;
      case SourceFile::SOURCE_UNKNOWN:
      case SourceFile::SOURCE_SWIFTMODULE:
      case SourceFile::SOURCE_NUMTYPES:
        *err_ =
            Err(scope_->GetValue(variables::kSources, true)->list_value()[i],
                std::string("Only source, header, and object files belong in "
                            "the sources of a ") +
                    Target::GetStringForOutputType(target_->output_type()) +
                    ". " + source.value() + " is not one of the valid types.");
    }

    target_->source_types_used().Set(source_type);
  }
  return ret;
}

bool BinaryTargetGenerator::FillCompleteStaticLib() {
  if (target_->output_type() == Target::STATIC_LIBRARY) {
    const Value* value = scope_->GetValue(variables::kCompleteStaticLib, true);
    if (!value)
      return true;
    if (!value->VerifyTypeIs(Value::BOOLEAN, err_))
      return false;
    target_->set_complete_static_lib(value->boolean_value());
  }
  return true;
}

bool BinaryTargetGenerator::FillFriends() {
  const Value* value = scope_->GetValue(variables::kFriend, true);
  if (value) {
    return ExtractListOfLabelPatterns(scope_->settings()->build_settings(),
                                      *value, scope_->GetSourceDir(),
                                      &target_->friends(), err_);
  }
  return true;
}

bool BinaryTargetGenerator::FillOutputName() {
  const Value* value = scope_->GetValue(variables::kOutputName, true);
  if (!value)
    return true;
  if (!value->VerifyTypeIs(Value::STRING, err_))
    return false;
  target_->set_output_name(value->string_value());
  return true;
}

bool BinaryTargetGenerator::FillOutputPrefixOverride() {
  const Value* value = scope_->GetValue(variables::kOutputPrefixOverride, true);
  if (!value)
    return true;
  if (!value->VerifyTypeIs(Value::BOOLEAN, err_))
    return false;
  target_->set_output_prefix_override(value->boolean_value());
  return true;
}

bool BinaryTargetGenerator::FillOutputDir() {
  const Value* value = scope_->GetValue(variables::kOutputDir, true);
  if (!value)
    return true;
  if (!value->VerifyTypeIs(Value::STRING, err_))
    return false;

  if (value->string_value().empty())
    return true;  // Treat empty string as the default and do nothing.

  const BuildSettings* build_settings = scope_->settings()->build_settings();
  SourceDir dir = scope_->GetSourceDir().ResolveRelativeDir(
      *value, err_, build_settings->root_path_utf8());
  if (err_->has_error())
    return false;

  if (!EnsureStringIsInOutputDir(build_settings->build_dir(), dir.value(),
                                 value->origin(), err_))
    return false;
  target_->set_output_dir(dir);
  return true;
}

bool BinaryTargetGenerator::FillAllowCircularIncludesFrom() {
  const Value* value =
      scope_->GetValue(variables::kAllowCircularIncludesFrom, true);
  if (!value)
    return true;

  UniqueVector<Label> circular;
  ExtractListOfUniqueLabels(scope_->settings()->build_settings(), *value,
                            scope_->GetSourceDir(),
                            ToolchainLabelForScope(scope_), &circular, err_);
  if (err_->has_error())
    return false;

  // Validate that all circular includes entries are in the deps.
  for (const auto& cur : circular) {
    bool found_dep = false;
    for (const auto& dep_pair : target_->GetDeps(Target::DEPS_LINKED)) {
      if (dep_pair.label == cur) {
        found_dep = true;
        break;
      }
    }
    if (!found_dep) {
      bool with_toolchain = scope_->settings()->ShouldShowToolchain({
        &target_->label(),
        &cur
      });
      *err_ = Err(*value, "Label not in deps.",
                  "The label \"" + cur.GetUserVisibleName(with_toolchain) +
                      "\"\nwas not in the deps of this target. "
                      "allow_circular_includes_from only allows\ntargets "
                      "present in the "
                      "deps.");
      return false;
    }
  }

  // Add to the set.
  for (const auto& cur : circular)
    target_->allow_circular_includes_from().insert(cur);
  return true;
}

bool BinaryTargetGenerator::FillPool() {
  const Value* value = scope_->GetValue(variables::kPool, true);
  if (!value)
    return true;

  Label label =
      Label::Resolve(scope_->GetSourceDir(),
                     scope_->settings()->build_settings()->root_path_utf8(),
                     ToolchainLabelForScope(scope_), *value, err_);
  if (err_->has_error())
    return false;

  LabelPtrPair<Pool> pair(label);
  pair.origin = target_->defined_from();

  target_->set_pool(std::move(pair));
  return true;
}

bool BinaryTargetGenerator::ValidateSources() {
  // For Rust targets, if the only source file is the root `sources` can be
  // omitted/empty.
  if (scope_->GetValue(variables::kRustCrateRoot, false)) {
    target_->source_types_used().Set(SourceFile::SOURCE_RS);
  }

  if (target_->source_types_used().MixedSourceUsed()) {
    *err_ =
        Err(function_call_, "More than one language used in target sources.",
            "Mixed sources are not allowed, unless they are "
            "compilation-compatible (e.g. Objective C and C++).");
    return false;
  }
  return true;
}
