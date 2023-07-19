// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/ninja_generated_file_target_writer.h"

#include "base/strings/string_util.h"
#include "gn/deps_iterator.h"
#include "gn/output_conversion.h"
#include "gn/output_file.h"
#include "gn/scheduler.h"
#include "gn/settings.h"
#include "gn/string_output_buffer.h"
#include "gn/string_utils.h"
#include "gn/target.h"
#include "gn/trace.h"

NinjaGeneratedFileTargetWriter::NinjaGeneratedFileTargetWriter(
    const Target* target,
    std::ostream& out)
    : NinjaTargetWriter(target, out) {}

NinjaGeneratedFileTargetWriter::~NinjaGeneratedFileTargetWriter() = default;

void NinjaGeneratedFileTargetWriter::Run() {
  // Write the file.
  GenerateFile();

  // A generated_file target should generate a stamp file with dependencies
  // on each of the deps and data_deps in the target. The actual collection is
  // done at gen time, and so ninja doesn't need to know about it.
  std::vector<OutputFile> output_files;
  std::vector<OutputFile> data_output_files;
  const auto& target_deps = resolved().GetTargetDeps(target_);
  for (const Target* dep : target_deps.linked_deps()) {
    if (dep->IsDataOnly()) {
      data_output_files.push_back(dep->dependency_output_file());
    } else {
      output_files.push_back(dep->dependency_output_file());
    }
  }

  for (const Target* data_dep : target_deps.data_deps())
    data_output_files.push_back(data_dep->dependency_output_file());

  WriteStampForTarget(output_files, data_output_files);
}

void NinjaGeneratedFileTargetWriter::GenerateFile() {
  Err err;

  std::vector<SourceFile> outputs_as_sources;
  target_->action_values().GetOutputsAsSourceFiles(target_,
                                                   &outputs_as_sources);
  CHECK(outputs_as_sources.size() == 1);

  base::FilePath output =
      settings_->build_settings()->GetFullPath(outputs_as_sources[0]);
  ScopedTrace trace(TraceItem::TRACE_FILE_WRITE_GENERATED, outputs_as_sources[0].value());
  trace.SetToolchain(target_->settings()->toolchain_label());

  // If this is a metadata target, populate the write value with the appropriate
  // data.
  Value contents;
  if (target_->contents().type() == Value::NONE) {
    // Origin is set to the outputs location, so that errors with this value
    // get flagged on the right target.
    CHECK(target_->action_values().outputs().list().size() == 1U);
    contents = Value(target_->action_values().outputs().list()[0].origin(),
                     Value::LIST);
    TargetSet targets_walked;
    ScopedTrace metadata_walk_trace(TraceItem::TRACE_WALK_METADATA, target_->label());
    trace.SetToolchain(target_->settings()->toolchain_label());
    if (!target_->GetMetadata(target_->data_keys(), target_->walk_keys(),
                              target_->rebase(), /*deps_only = */ true,
                              &contents.list_value(), &targets_walked, &err)) {
      g_scheduler->FailWithError(err);
      return;
    }
  } else {
    contents = target_->contents();
  }

  // Compute output.
  StringOutputBuffer storage;
  std::ostream out(&storage);
  ConvertValueToOutput(settings_, contents, target_->output_conversion(), out,
                       &err);

  if (err.has_error()) {
    g_scheduler->FailWithError(err);
    return;
  }

  storage.WriteToFileIfChanged(output, &err);

  if (err.has_error()) {
    g_scheduler->FailWithError(err);
    return;
  }
}
