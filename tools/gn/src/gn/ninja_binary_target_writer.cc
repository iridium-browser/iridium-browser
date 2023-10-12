// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/ninja_binary_target_writer.h"

#include <sstream>

#include "base/strings/string_util.h"
#include "gn/config_values_extractors.h"
#include "gn/deps_iterator.h"
#include "gn/filesystem_utils.h"
#include "gn/general_tool.h"
#include "gn/ninja_c_binary_target_writer.h"
#include "gn/ninja_rust_binary_target_writer.h"
#include "gn/ninja_target_command_util.h"
#include "gn/ninja_utils.h"
#include "gn/pool.h"
#include "gn/settings.h"
#include "gn/string_utils.h"
#include "gn/substitution_writer.h"
#include "gn/target.h"
#include "gn/variables.h"

namespace {

// Returns the proper escape options for writing compiler and linker flags.
EscapeOptions GetFlagOptions() {
  EscapeOptions opts;
  opts.mode = ESCAPE_NINJA_COMMAND;
  return opts;
}

}  // namespace

NinjaBinaryTargetWriter::NinjaBinaryTargetWriter(const Target* target,
                                                 std::ostream& out)
    : NinjaTargetWriter(target, out),
      rule_prefix_(GetNinjaRulePrefixForToolchain(settings_)) {}

NinjaBinaryTargetWriter::~NinjaBinaryTargetWriter() = default;

void NinjaBinaryTargetWriter::Run() {
  if (target_->source_types_used().RustSourceUsed()) {
    NinjaRustBinaryTargetWriter writer(target_, out_);
    writer.SetResolvedTargetData(GetResolvedTargetData());
    writer.Run();
    return;
  }

  NinjaCBinaryTargetWriter writer(target_, out_);
  writer.SetResolvedTargetData(GetResolvedTargetData());
  writer.Run();
}

std::vector<OutputFile> NinjaBinaryTargetWriter::WriteInputsStampAndGetDep(
    size_t num_stamp_uses) const {
  CHECK(target_->toolchain()) << "Toolchain not set on target "
                              << target_->label().GetUserVisibleName(true);

  UniqueVector<const SourceFile*> inputs;
  for (ConfigValuesIterator iter(target_); !iter.done(); iter.Next()) {
    for (const auto& input : iter.cur().inputs()) {
      inputs.push_back(&input);
    }
  }

  if (inputs.size() == 0)
    return std::vector<OutputFile>();  // No inputs

  // If we only have one input, return it directly instead of writing a stamp
  // file for it.
  if (inputs.size() == 1) {
    return std::vector<OutputFile>{
      OutputFile(settings_->build_settings(), *inputs[0])};
  }

  std::vector<OutputFile> outs;
  for (const SourceFile* source : inputs)
    outs.push_back(OutputFile(settings_->build_settings(), *source));

  // If there are multiple inputs, but the stamp file would be referenced only
  // once, don't write it but depend on the inputs directly.
  if (num_stamp_uses == 1u)
    return outs;

  // Make a stamp file.
  OutputFile stamp_file =
      GetBuildDirForTargetAsOutputFile(target_, BuildDirType::OBJ);
  stamp_file.value().append(target_->label().name());
  stamp_file.value().append(".inputs.stamp");

  out_ << "build ";
  path_output_.WriteFile(out_, stamp_file);
  out_ << ": " << GetNinjaRulePrefixForToolchain(settings_)
       << GeneralTool::kGeneralToolStamp;

  // File inputs.
  for (const auto* input : inputs) {
    out_ << " ";
    path_output_.WriteFile(out_, *input);
  }

  out_ << std::endl;
  return {stamp_file};
}

NinjaBinaryTargetWriter::ClassifiedDeps
NinjaBinaryTargetWriter::GetClassifiedDeps() const {
  ClassifiedDeps classified_deps;

  const auto& target_deps = resolved().GetTargetDeps(target_);

  // Normal public/private deps.
  for (const Target* dep : target_deps.linked_deps()) {
    ClassifyDependency(dep, &classified_deps);
  }

  // Inherited libraries.
  for (const auto& inherited : resolved().GetInheritedLibraries(target_)) {
    ClassifyDependency(inherited.target(), &classified_deps);
  }

  // Data deps.
  for (const Target* data_dep : target_deps.data_deps())
    classified_deps.non_linkable_deps.push_back(data_dep);

  return classified_deps;
}

void NinjaBinaryTargetWriter::ClassifyDependency(
    const Target* dep,
    ClassifiedDeps* classified_deps) const {
  // Only the following types of outputs have libraries linked into them:
  //  EXECUTABLE
  //  SHARED_LIBRARY
  //  _complete_ STATIC_LIBRARY
  //
  // Child deps of intermediate static libraries get pushed up the
  // dependency tree until one of these is reached, and source sets
  // don't link at all.
  bool can_link_libs = target_->IsFinal();

  if (can_link_libs && dep->builds_swift_module())
    classified_deps->swiftmodule_deps.push_back(dep);

  if (target_->source_types_used().RustSourceUsed() &&
      (target_->output_type() == Target::RUST_LIBRARY ||
       target_->output_type() == Target::STATIC_LIBRARY) &&
      dep->IsLinkable()) {
    // Rust libraries and static libraries aren't final, but need to have the
    // link lines of all transitive deps specified.
    classified_deps->linkable_deps.push_back(dep);
  } else if (dep->output_type() == Target::SOURCE_SET ||
             // If a complete static library depends on an incomplete static
             // library, manually link in the object files of the dependent
             // library as if it were a source set. This avoids problems with
             // braindead tools such as ar which don't properly link dependent
             // static libraries.
             (target_->complete_static_lib() &&
              (dep->output_type() == Target::STATIC_LIBRARY &&
               !dep->complete_static_lib()))) {
    // Source sets have their object files linked into final targets
    // (shared libraries, executables, loadable modules, and complete static
    // libraries). Intermediate static libraries and other source sets
    // just forward the dependency, otherwise the files in the source
    // set can easily get linked more than once which will cause
    // multiple definition errors.
    if (can_link_libs)
      AddSourceSetFiles(dep, &classified_deps->extra_object_files);

    // Add the source set itself as a non-linkable dependency on the current
    // target. This will make sure that anything the source set's stamp file
    // depends on (like data deps) are also built before the current target
    // can be complete. Otherwise, these will be skipped since this target
    // will depend only on the source set's object files.
    classified_deps->non_linkable_deps.push_back(dep);
  } else if (target_->complete_static_lib() && dep->IsFinal()) {
    classified_deps->non_linkable_deps.push_back(dep);
  } else if (can_link_libs && dep->IsLinkable()) {
    classified_deps->linkable_deps.push_back(dep);
  } else if (dep->output_type() == Target::CREATE_BUNDLE &&
             dep->bundle_data().is_framework()) {
    classified_deps->framework_deps.push_back(dep);
  } else {
    classified_deps->non_linkable_deps.push_back(dep);
  }
}

void NinjaBinaryTargetWriter::AddSourceSetFiles(
    const Target* source_set,
    UniqueVector<OutputFile>* obj_files) const {
  std::vector<OutputFile> tool_outputs;  // Prevent allocation in loop.

  // Compute object files for all sources. Only link the first output from
  // the tool if there are more than one.
  for (const auto& source : source_set->sources()) {
    const char* tool_name = Tool::kToolNone;
    if (source_set->GetOutputFilesForSource(source, &tool_name, &tool_outputs))
      obj_files->push_back(tool_outputs[0]);
  }

  // Swift files may generate one object file per module or one per source file
  // depending on how the compiler is invoked (whole module optimization).
  if (source_set->source_types_used().SwiftSourceUsed()) {
    const Tool* tool = source_set->toolchain()->GetToolForSourceTypeAsC(
        SourceFile::SOURCE_SWIFT);

    std::vector<OutputFile> outputs;
    SubstitutionWriter::ApplyListToLinkerAsOutputFile(
        source_set, tool, tool->outputs(), &outputs);

    for (const OutputFile& output : outputs) {
      SourceFile output_as_source =
          output.AsSourceFile(source_set->settings()->build_settings());
      if (output_as_source.IsObjectType()) {
        obj_files->push_back(output);
      }
    }
  }

  // Add MSVC precompiled header object files. GCC .gch files are not object
  // files so they are omitted.
  if (source_set->config_values().has_precompiled_headers()) {
    if (source_set->source_types_used().Get(SourceFile::SOURCE_C)) {
      const CTool* tool = source_set->toolchain()->GetToolAsC(CTool::kCToolCc);
      if (tool && tool->precompiled_header_type() == CTool::PCH_MSVC) {
        GetPCHOutputFiles(source_set, CTool::kCToolCc, &tool_outputs);
        obj_files->Append(tool_outputs.begin(), tool_outputs.end());
      }
    }
    if (source_set->source_types_used().Get(SourceFile::SOURCE_CPP)) {
      const CTool* tool = source_set->toolchain()->GetToolAsC(CTool::kCToolCxx);
      if (tool && tool->precompiled_header_type() == CTool::PCH_MSVC) {
        GetPCHOutputFiles(source_set, CTool::kCToolCxx, &tool_outputs);
        obj_files->Append(tool_outputs.begin(), tool_outputs.end());
      }
    }
    if (source_set->source_types_used().Get(SourceFile::SOURCE_M)) {
      const CTool* tool =
          source_set->toolchain()->GetToolAsC(CTool::kCToolObjC);
      if (tool && tool->precompiled_header_type() == CTool::PCH_MSVC) {
        GetPCHOutputFiles(source_set, CTool::kCToolObjC, &tool_outputs);
        obj_files->Append(tool_outputs.begin(), tool_outputs.end());
      }
    }
    if (source_set->source_types_used().Get(SourceFile::SOURCE_MM)) {
      const CTool* tool =
          source_set->toolchain()->GetToolAsC(CTool::kCToolObjCxx);
      if (tool && tool->precompiled_header_type() == CTool::PCH_MSVC) {
        GetPCHOutputFiles(source_set, CTool::kCToolObjCxx, &tool_outputs);
        obj_files->Append(tool_outputs.begin(), tool_outputs.end());
      }
    }
  }
}

void NinjaBinaryTargetWriter::WriteCompilerBuildLine(
    const std::vector<SourceFile>& sources,
    const std::vector<OutputFile>& extra_deps,
    const std::vector<OutputFile>& order_only_deps,
    const char* tool_name,
    const std::vector<OutputFile>& outputs,
    bool can_write_source_info) {
  out_ << "build";
  path_output_.WriteFiles(out_, outputs);

  out_ << ": " << rule_prefix_ << tool_name;
  path_output_.WriteFiles(out_, sources);

  if (!extra_deps.empty()) {
    out_ << " |";
    path_output_.WriteFiles(out_, extra_deps);
  }

  if (!order_only_deps.empty()) {
    out_ << " ||";
    path_output_.WriteFiles(out_, order_only_deps);
  }
  out_ << std::endl;

  if (!sources.empty() && can_write_source_info) {
    out_ << "  "
         << "source_file_part = " << sources[0].GetName();
    out_ << std::endl;
    out_ << "  "
         << "source_name_part = "
         << FindFilenameNoExtension(&sources[0].value());
    out_ << std::endl;
  }
}

void NinjaBinaryTargetWriter::WriteCustomLinkerFlags(
    std::ostream& out,
    const Tool* tool) {

  if (tool->AsC() || (tool->AsRust() && tool->AsRust()->MayLink())) {
    // First the ldflags from the target and its config.
    RecursiveTargetConfigStringsToStream(kRecursiveWriterKeepDuplicates,
                                         target_, &ConfigValues::ldflags,
                                         GetFlagOptions(), out);
  }
}

void NinjaBinaryTargetWriter::WriteLibrarySearchPath(
    std::ostream& out,
    const Tool* tool) {
  // Write library search paths that have been recursively pushed
  // through the dependency tree.
  const auto& all_lib_dirs = resolved().GetLinkedLibraryDirs(target_);
  if (!all_lib_dirs.empty()) {
    // Since we're passing these on the command line to the linker and not
    // to Ninja, we need to do shell escaping.
    PathOutput lib_path_output(path_output_.current_dir(),
                               settings_->build_settings()->root_path_utf8(),
                               ESCAPE_NINJA_COMMAND);
    for (size_t i = 0; i < all_lib_dirs.size(); i++) {
      out << " " << tool->lib_dir_switch();
      lib_path_output.WriteDir(out, all_lib_dirs[i],
                               PathOutput::DIR_NO_LAST_SLASH);
    }
  }

  const auto& all_framework_dirs = resolved().GetLinkedFrameworkDirs(target_);
  if (!all_framework_dirs.empty()) {
    // Since we're passing these on the command line to the linker and not
    // to Ninja, we need to do shell escaping.
    PathOutput framework_path_output(
        path_output_.current_dir(),
        settings_->build_settings()->root_path_utf8(), ESCAPE_NINJA_COMMAND);
    for (size_t i = 0; i < all_framework_dirs.size(); i++) {
      out << " " << tool->framework_dir_switch();
      framework_path_output.WriteDir(out, all_framework_dirs[i],
                                     PathOutput::DIR_NO_LAST_SLASH);
    }
  }
}

void NinjaBinaryTargetWriter::WriteLinkerFlags(
    std::ostream& out,
    const Tool* tool,
    const SourceFile* optional_def_file) {
  // First any ldflags
  WriteCustomLinkerFlags(out, tool);
  // Then the library search path
  WriteLibrarySearchPath(out, tool);

  if (optional_def_file) {
    out_ << " /DEF:";
    path_output_.WriteFile(out, *optional_def_file);
  }
}

void NinjaBinaryTargetWriter::WriteLibs(std::ostream& out, const Tool* tool) {
  // Libraries that have been recursively pushed through the dependency tree.
  // Since we're passing these on the command line to the linker and not
  // to Ninja, we need to do shell escaping.
  PathOutput lib_path_output(
      path_output_.current_dir(), settings_->build_settings()->root_path_utf8(),
      ESCAPE_NINJA_COMMAND);
  EscapeOptions lib_escape_opts;
  lib_escape_opts.mode = ESCAPE_NINJA_COMMAND;
  const auto& all_libs = resolved().GetLinkedLibraries(target_);
  for (size_t i = 0; i < all_libs.size(); i++) {
    const LibFile& lib_file = all_libs[i];
    const std::string& lib_value = lib_file.value();
    if (lib_file.is_source_file()) {
      out << " " << tool->linker_arg();
      lib_path_output.WriteFile(out, lib_file.source_file());
    } else {
      out << " " << tool->lib_switch();
      EscapeStringToStream(out, lib_value, lib_escape_opts);
    }
  }
}

void NinjaBinaryTargetWriter::WriteFrameworks(std::ostream& out,
                                              const Tool* tool) {
  // Frameworks that have been recursively pushed through the dependency tree.
  FrameworksWriter writer(tool->framework_switch());
  const auto& all_frameworks = resolved().GetLinkedFrameworks(target_);
  for (size_t i = 0; i < all_frameworks.size(); i++) {
    writer(all_frameworks[i], out);
  }

  FrameworksWriter weak_writer(tool->weak_framework_switch());
  const auto& all_weak_frameworks = resolved().GetLinkedWeakFrameworks(target_);
  for (size_t i = 0; i < all_weak_frameworks.size(); i++) {
    weak_writer(all_weak_frameworks[i], out);
  }
}

void NinjaBinaryTargetWriter::WriteSwiftModules(
    std::ostream& out,
    const Tool* tool,
    const std::vector<OutputFile>& swiftmodules) {
  // Since we're passing these on the command line to the linker and not
  // to Ninja, we need to do shell escaping.
  PathOutput swiftmodule_path_output(
      path_output_.current_dir(), settings_->build_settings()->root_path_utf8(),
      ESCAPE_NINJA_COMMAND);

  for (const OutputFile& swiftmodule : swiftmodules) {
    out << " " << tool->swiftmodule_switch();
    swiftmodule_path_output.WriteFile(out, swiftmodule);
  }
}

void NinjaBinaryTargetWriter::WritePool(std::ostream& out) {
  if (target_->pool().ptr) {
    out << "  pool = ";
    out << target_->pool().ptr->GetNinjaName(
        settings_->default_toolchain_label());
    out << std::endl;
  }
}
