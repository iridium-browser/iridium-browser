// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/swift_values.h"

#include "gn/deps_iterator.h"
#include "gn/err.h"
#include "gn/settings.h"
#include "gn/substitution_writer.h"
#include "gn/target.h"

SwiftValues::SwiftValues() = default;

SwiftValues::~SwiftValues() = default;

bool SwiftValues::OnTargetResolved(const Target* target, Err* err) {
  if (!FillModuleOuputFile(target, err))
    return false;

  FillModuleDependencies(target);
  return true;
}

void SwiftValues::FillModuleDependencies(const Target* target) {
  for (const auto& pair : target->GetDeps(Target::DEPS_LINKED)) {
    if (pair.ptr->toolchain() == target->toolchain() ||
        pair.ptr->toolchain()->propagates_configs()) {
      modules_.Append(pair.ptr->swift_values().public_modules().begin(),
                      pair.ptr->swift_values().public_modules().end());
    }
  }

  for (const auto& pair : target->public_deps()) {
    if (pair.ptr->toolchain() == target->toolchain() ||
        pair.ptr->toolchain()->propagates_configs())
      public_modules_.Append(pair.ptr->swift_values().public_modules().begin(),
                             pair.ptr->swift_values().public_modules().end());
  }

  if (builds_module())
    public_modules_.push_back(target);
}

bool SwiftValues::FillModuleOuputFile(const Target* target, Err* err) {
  if (!target->IsBinary() || !target->source_types_used().SwiftSourceUsed())
    return true;

  const Tool* tool =
      target->toolchain()->GetToolForSourceType(SourceFile::SOURCE_SWIFT);
  CHECK(tool->outputs().list().size() >= 1);

  OutputFile module_output_file =
      SubstitutionWriter::ApplyPatternToLinkerAsOutputFile(
          target, tool, tool->outputs().list()[0]);

  const SourceFile module_output_file_as_source =
      module_output_file.AsSourceFile(target->settings()->build_settings());
  if (module_output_file_as_source.type() != SourceFile::SOURCE_SWIFTMODULE) {
    *err = Err(tool->defined_from(), "Incorrect outputs for tool",
               "The first output of tool " + std::string(tool->name()) +
                   " must be a .swiftmodule file.");
    return false;
  }

  module_output_file_ = std::move(module_output_file);
  module_output_dir_ = module_output_file_as_source.GetDir();

  return true;
}
