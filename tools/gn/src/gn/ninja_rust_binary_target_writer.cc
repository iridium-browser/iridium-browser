// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/ninja_rust_binary_target_writer.h"

#include <sstream>

#include "base/strings/string_util.h"
#include "gn/deps_iterator.h"
#include "gn/filesystem_utils.h"
#include "gn/general_tool.h"
#include "gn/ninja_target_command_util.h"
#include "gn/ninja_utils.h"
#include "gn/rust_substitution_type.h"
#include "gn/substitution_writer.h"
#include "gn/rust_values.h"
#include "gn/target.h"

namespace {

// Returns the proper escape options for writing compiler and linker flags.
EscapeOptions GetFlagOptions() {
  EscapeOptions opts;
  opts.mode = ESCAPE_NINJA_COMMAND;
  return opts;
}

void WriteVar(const char* name,
              const std::string& value,
              EscapeOptions opts,
              std::ostream& out) {
  out << name << " = ";
  EscapeStringToStream(out, value, opts);
  out << std::endl;
}

void WriteCrateVars(const Target* target,
                    const Tool* tool,
                    EscapeOptions opts,
                    std::ostream& out) {
  WriteVar(kRustSubstitutionCrateName.ninja_name,
           target->rust_values().crate_name(), opts, out);

  std::string crate_type;
  switch (target->rust_values().crate_type()) {
    // Auto-select the crate type for executables, static libraries, and rlibs.
    case RustValues::CRATE_AUTO: {
      switch (target->output_type()) {
        case Target::EXECUTABLE:
          crate_type = "bin";
          break;
        case Target::STATIC_LIBRARY:
          crate_type = "staticlib";
          break;
        case Target::RUST_LIBRARY:
          crate_type = "rlib";
          break;
        case Target::RUST_PROC_MACRO:
          crate_type = "proc-macro";
          break;
        default:
          NOTREACHED();
      }
      break;
    }
    case RustValues::CRATE_BIN:
      crate_type = "bin";
      break;
    case RustValues::CRATE_CDYLIB:
      crate_type = "cdylib";
      break;
    case RustValues::CRATE_DYLIB:
      crate_type = "dylib";
      break;
    case RustValues::CRATE_PROC_MACRO:
      crate_type = "proc-macro";
      break;
    case RustValues::CRATE_RLIB:
      crate_type = "rlib";
      break;
    case RustValues::CRATE_STATICLIB:
      crate_type = "staticlib";
      break;
    default:
      NOTREACHED();
  }
  WriteVar(kRustSubstitutionCrateType.ninja_name, crate_type, opts, out);

  WriteVar(SubstitutionOutputExtension.ninja_name,
           SubstitutionWriter::GetLinkerSubstitution(
               target, tool, &SubstitutionOutputExtension),
           opts, out);
  WriteVar(SubstitutionOutputDir.ninja_name,
           SubstitutionWriter::GetLinkerSubstitution(target, tool,
                                                     &SubstitutionOutputDir),
           opts, out);
}

}  // namespace

NinjaRustBinaryTargetWriter::NinjaRustBinaryTargetWriter(const Target* target,
                                                         std::ostream& out)
    : NinjaBinaryTargetWriter(target, out),
      tool_(target->toolchain()->GetToolForTargetFinalOutputAsRust(target)) {}

NinjaRustBinaryTargetWriter::~NinjaRustBinaryTargetWriter() = default;

// TODO(juliehockett): add inherited library support? and IsLinkable support?
// for c-cross-compat
void NinjaRustBinaryTargetWriter::Run() {
  DCHECK(target_->output_type() != Target::SOURCE_SET);

  size_t num_stamp_uses = target_->sources().size();

  std::vector<OutputFile> input_deps = WriteInputsStampAndGetDep(
      num_stamp_uses);

  WriteCompilerVars();

  // Classify our dependencies.
  ClassifiedDeps classified_deps = GetClassifiedDeps();

  // The input dependencies will be an order-only dependency. This will cause
  // Ninja to make sure the inputs are up to date before compiling this source,
  // but changes in the inputs deps won't cause the file to be recompiled. See
  // the comment on NinjaCBinaryTargetWriter::Run for more detailed explanation.
  std::vector<OutputFile> order_only_deps = WriteInputDepsStampAndGetDep(
      std::vector<const Target*>(), num_stamp_uses);
  std::copy(input_deps.begin(), input_deps.end(),
            std::back_inserter(order_only_deps));

  // Build lists which will go into different bits of the rustc command line.
  // Public rust_library deps go in a --extern rlibs, public non-rust deps go in
  // -Ldependency. Also assemble a list of extra (i.e. implicit) deps
  // for ninja dependency tracking.
  UniqueVector<OutputFile> implicit_deps;
  AppendSourcesAndInputsToImplicitDeps(&implicit_deps);
  implicit_deps.Append(classified_deps.extra_object_files.begin(),
                       classified_deps.extra_object_files.end());

  std::vector<OutputFile> rustdeps;
  std::vector<OutputFile> nonrustdeps;
  nonrustdeps.insert(nonrustdeps.end(),
                     classified_deps.extra_object_files.begin(),
                     classified_deps.extra_object_files.end());
  for (const auto* framework_dep : classified_deps.framework_deps) {
    order_only_deps.push_back(framework_dep->dependency_output_file());
  }
  for (const auto* non_linkable_dep : classified_deps.non_linkable_deps) {
    if (non_linkable_dep->source_types_used().RustSourceUsed() &&
        non_linkable_dep->output_type() != Target::SOURCE_SET) {
      rustdeps.push_back(non_linkable_dep->dependency_output_file());
    }
    order_only_deps.push_back(non_linkable_dep->dependency_output_file());
  }
  for (const auto* linkable_dep : classified_deps.linkable_deps) {
    // Rust cdylibs are treated as non-Rust dependencies for linking purposes.
    if (linkable_dep->source_types_used().RustSourceUsed() &&
        linkable_dep->rust_values().crate_type() != RustValues::CRATE_CDYLIB) {
      rustdeps.push_back(linkable_dep->link_output_file());
    } else {
      nonrustdeps.push_back(linkable_dep->link_output_file());
    }
    implicit_deps.push_back(linkable_dep->dependency_output_file());
  }

  // Rust libraries specified by paths.
  for (ConfigValuesIterator iter(target_); !iter.done(); iter.Next()) {
    const ConfigValues& cur = iter.cur();
    for (const auto& e : cur.externs()) {
      if (e.second.is_source_file()) {
        implicit_deps.push_back(
            OutputFile(settings_->build_settings(), e.second.source_file()));
      }
    }
  }

  // Bubble up the full list of transitive rlib dependencies.
  std::vector<OutputFile> transitive_rustlibs;
  for (const auto* dep : target_->rust_transitive_libs().GetOrdered()) {
    if (dep->source_types_used().RustSourceUsed()) {
      transitive_rustlibs.push_back(dep->dependency_output_file());
    }
  }

  std::vector<OutputFile> tool_outputs;
  SubstitutionWriter::ApplyListToLinkerAsOutputFile(
      target_, tool_, tool_->outputs(), &tool_outputs);
  WriteCompilerBuildLine({target_->rust_values().crate_root()},
                         implicit_deps.vector(), order_only_deps, tool_->name(),
                         tool_outputs);

  std::vector<const Target*> extern_deps(
      classified_deps.linkable_deps.vector());
  std::copy(classified_deps.non_linkable_deps.begin(),
            classified_deps.non_linkable_deps.end(),
            std::back_inserter(extern_deps));
  WriteExterns(extern_deps);
  WriteRustdeps(transitive_rustlibs, rustdeps, nonrustdeps);
  WriteSourcesAndInputs();
}

void NinjaRustBinaryTargetWriter::WriteCompilerVars() {
  const SubstitutionBits& subst = target_->toolchain()->substitution_bits();

  EscapeOptions opts = GetFlagOptions();
  WriteCrateVars(target_, tool_, opts, out_);

  WriteOneFlag(kRecursiveWriterKeepDuplicates, target_,
               &kRustSubstitutionRustFlags, false, Tool::kToolNone,
               &ConfigValues::rustflags, opts, path_output_, out_);

  WriteOneFlag(kRecursiveWriterKeepDuplicates, target_,
               &kRustSubstitutionRustEnv, false, Tool::kToolNone,
               &ConfigValues::rustenv, opts, path_output_, out_);

  WriteSharedVars(subst);
}

void NinjaRustBinaryTargetWriter::AppendSourcesAndInputsToImplicitDeps(
    UniqueVector<OutputFile>* deps) const {
  // Only the crate_root file needs to be given to rustc as input.
  // Any other 'sources' are just implicit deps.
  // Most Rust targets won't bother specifying the "sources =" line
  // because it is handled sufficiently by crate_root and the generation
  // of depfiles by rustc. But for those which do...
  for (const auto& source : target_->sources()) {
    deps->push_back(OutputFile(settings_->build_settings(), source));
  }
  for (const auto& data : target_->config_values().inputs()) {
    deps->push_back(OutputFile(settings_->build_settings(), data));
  }
}

void NinjaRustBinaryTargetWriter::WriteSourcesAndInputs() {
  out_ << "  sources =";
  for (const auto& source : target_->sources()) {
    out_ << " ";
    path_output_.WriteFile(out_, OutputFile(settings_->build_settings(), source));
  }
  for (const auto& data : target_->config_values().inputs()) {
    out_ << " ";
    path_output_.WriteFile(out_, OutputFile(settings_->build_settings(), data));
  }
  out_ << std::endl;
}

void NinjaRustBinaryTargetWriter::WriteExterns(
    const std::vector<const Target*>& deps) {
  out_ << "  externs =";

  for (const Target* target : deps) {
    if (target->output_type() == Target::RUST_LIBRARY ||
        target->output_type() == Target::RUST_PROC_MACRO ||
        (target->source_types_used().RustSourceUsed() &&
         target->rust_values().InferredCrateType(target) ==
             RustValues::CRATE_DYLIB)) {
      out_ << " --extern ";
      const auto& renamed_dep =
          target_->rust_values().aliased_deps().find(target->label());
      if (renamed_dep != target_->rust_values().aliased_deps().end()) {
        out_ << renamed_dep->second << "=";
      } else {
        out_ << std::string(target->rust_values().crate_name()) << "=";
      }
      path_output_.WriteFile(out_, target->dependency_output_file());
    }
  }

  EscapeOptions extern_escape_opts;
  extern_escape_opts.mode = ESCAPE_NINJA_COMMAND;

  for (ConfigValuesIterator iter(target_); !iter.done(); iter.Next()) {
    const ConfigValues& cur = iter.cur();
    for (const auto& e : cur.externs()) {
      out_ << " --extern " << std::string(e.first) << "=";
      if (e.second.is_source_file()) {
        path_output_.WriteFile(out_, e.second.source_file());
      } else {
        EscapeStringToStream(out_, e.second.value(), extern_escape_opts);
      }
    }
  }

  out_ << std::endl;
}

void NinjaRustBinaryTargetWriter::WriteRustdeps(
    const std::vector<OutputFile>& transitive_rustdeps,
    const std::vector<OutputFile>& rustdeps,
    const std::vector<OutputFile>& nonrustdeps) {
  out_ << "  rustdeps =";

  // Rust dependencies.
  UniqueVector<SourceDir> transitive_rustdep_dirs;
  for (const auto& rustdep : transitive_rustdeps) {
    // TODO switch to using --extern priv: after stabilization
    transitive_rustdep_dirs.push_back(
        rustdep.AsSourceFile(settings_->build_settings()).GetDir());
  }
  for (const auto& rustdepdir : transitive_rustdep_dirs) {
    out_ << " -Ldependency=";
    path_output_.WriteDir(out_, rustdepdir, PathOutput::DIR_NO_LAST_SLASH);
  }

  EscapeOptions lib_escape_opts;
  lib_escape_opts.mode = ESCAPE_NINJA_COMMAND;

  // Non-Rust native dependencies.
  UniqueVector<SourceDir> nonrustdep_dirs;
  for (const auto& nonrustdep : nonrustdeps) {
    nonrustdep_dirs.push_back(
        nonrustdep.AsSourceFile(settings_->build_settings()).GetDir());
  }
  // First -Lnative to specify the search directories.
  // This is necessary for #[link(...)] directives to work properly.
  for (const auto& nonrustdep_dir : nonrustdep_dirs) {
    out_ << " -Lnative=";
    path_output_.WriteDir(out_, nonrustdep_dir, PathOutput::DIR_NO_LAST_SLASH);
  }
  // Before outputting any libraries to link, ensure the linker is in a mode
  // that allows dynamic linking, as rustc may have previously put it into
  // static-only mode.
  if (nonrustdeps.size() > 0) {
    out_ << " -Clink-arg=-Bdynamic";
  }
  for (const auto& nonrustdep : nonrustdeps) {
    out_ << " -Clink-arg=";
    path_output_.WriteFile(out_, nonrustdep);
  }
  WriteLibrarySearchPath(out_, tool_);
  WriteLibs(out_, tool_);
  out_ << std::endl;
  out_ << "  ldflags =";
  WriteCustomLinkerFlags(out_, tool_);
  out_ << std::endl;
}
