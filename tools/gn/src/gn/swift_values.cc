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

// static
bool SwiftValues::OnTargetResolved(Target* target, Err* err) {
  return FillModuleOutputFile(target, err);
}

// static
bool SwiftValues::FillModuleOutputFile(Target* target, Err* err) {
  if (!target->builds_swift_module())
    return true;

  const Tool* tool =
      target->toolchain()->GetToolForSourceType(SourceFile::SOURCE_SWIFT);
  CHECK(tool->outputs().list().size() >= 1);

  OutputFile module_output_file =
      SubstitutionWriter::ApplyPatternToLinkerAsOutputFile(
          target, tool, tool->outputs().list()[0]);

  const SourceFile module_output_file_as_source =
      module_output_file.AsSourceFile(target->settings()->build_settings());
  if (!module_output_file_as_source.IsSwiftModuleType()) {
    *err = Err(tool->defined_from(), "Incorrect outputs for tool",
               "The first output of tool " + std::string(tool->name()) +
                   " must be a .swiftmodule file.");
    return false;
  }

  SwiftValues& swift_values = target->swift_values();
  swift_values.module_output_file_ = std::move(module_output_file);
  swift_values.module_output_dir_ = module_output_file_as_source.GetDir();

  return true;
}
