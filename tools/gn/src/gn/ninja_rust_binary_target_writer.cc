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
#include "gn/rust_values.h"
#include "gn/substitution_writer.h"
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

  std::vector<OutputFile> input_deps =
      WriteInputsStampAndGetDep(num_stamp_uses);

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

  // Collect the full transitive set of rust libraries that this target depends
  // on, and the public flag represents if the target has direct access to the
  // dependency through a chain of public_deps.
  std::vector<ExternCrate> transitive_crates;
  for (const auto& [dep, has_direct_access] :
       target_->rust_transitive_inherited_libs().GetOrderedAndPublicFlag()) {
    // We will tell rustc to look for crate metadata for any rust crate
    // dependencies except cdylibs, as they have no metadata present.
    if (dep->source_types_used().RustSourceUsed() &&
        RustValues::IsRustLibrary(dep)) {
      transitive_crates.push_back({dep, has_direct_access});
      // If the current crate can directly acccess the `dep` crate, then the
      // current crate needs an implicit dependency on `dep` so it will be
      // rebuilt if `dep` changes.
      if (has_direct_access) {
        implicit_deps.push_back(dep->dependency_output_file());
      }
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
  WriteExternsAndDeps(extern_deps, transitive_crates, rustdeps, nonrustdeps);
  WriteSourcesAndInputs();
  WritePool(out_);
}

void NinjaRustBinaryTargetWriter::WriteCompilerVars() {
  const SubstitutionBits& subst = target_->toolchain()->substitution_bits();

  EscapeOptions opts = GetFlagOptions();
  WriteCrateVars(target_, tool_, opts, out_);

  WriteRustCompilerVars(subst, /*indent=*/false, /*always_write=*/true);

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
    path_output_.WriteFile(out_,
                           OutputFile(settings_->build_settings(), source));
  }
  for (const auto& data : target_->config_values().inputs()) {
    out_ << " ";
    path_output_.WriteFile(out_, OutputFile(settings_->build_settings(), data));
  }
  out_ << std::endl;
}

void NinjaRustBinaryTargetWriter::WriteExternsAndDeps(
    const std::vector<const Target*>& deps,
    const std::vector<ExternCrate>& transitive_rust_deps,
    const std::vector<OutputFile>& rustdeps,
    const std::vector<OutputFile>& nonrustdeps) {
  // Writes a external LibFile which comes from user-specified externs, and may
  // be either a string or a SourceFile.
  auto write_extern_lib_file = [this](std::string_view crate_name,
                                      LibFile lib_file) {
    out_ << " --extern ";
    out_ << crate_name;
    out_ << "=";
    if (lib_file.is_source_file()) {
      path_output_.WriteFile(out_, lib_file.source_file());
    } else {
      EscapeOptions escape_opts_command;
      escape_opts_command.mode = ESCAPE_NINJA_COMMAND;
      EscapeStringToStream(out_, lib_file.value(), escape_opts_command);
    }
  };
  // Writes an external OutputFile which comes from a dependency of the current
  // target.
  auto write_extern_target = [this](const Target& dep) {
    std::string_view crate_name;
    const auto& aliased_deps = target_->rust_values().aliased_deps();
    if (auto it = aliased_deps.find(dep.label()); it != aliased_deps.end()) {
      crate_name = it->second;
    } else {
      crate_name = dep.rust_values().crate_name();
    }

    out_ << " --extern ";
    out_ << crate_name;
    out_ << "=";
    path_output_.WriteFile(out_, dep.dependency_output_file());
  };

  // Write accessible crates with `--extern` to add them to the extern prelude.
  out_ << "  externs =";

  // Tracking to avoid emitted the same lib twice. We track it instead of
  // pre-emptively constructing a UniqueVector since we would have to also store
  // the crate name, and in the future the public-ness.
  std::unordered_set<OutputFile> emitted_rust_libs;
  // TODO: We defer private dependencies to -Ldependency until --extern priv is
  // stabilized.
  UniqueVector<SourceDir> private_extern_dirs;

  // Walk the transitive closure of all rust dependencies.
  //
  // For dependencies that are meant to be accessible we pass them to --extern
  // in order to add them to the crate's extern prelude.
  //
  // For all transitive dependencies, we add them to `private_extern_dirs` in
  // order to generate a -Ldependency switch that points to them. This ensures
  // that rustc can find them if they are used by other dependencies. For
  // example:
  //
  //   A -> C --public--> D
  //     -> B --private-> D
  //
  // Here A has direct access to D, but B and C also make use of D, and they
  // will only search the paths specified to -Ldependency, thus D needs to
  // appear as both a --extern (for A) and -Ldependency (for B and C).
  for (const auto& crate : transitive_rust_deps) {
    const OutputFile& rust_lib = crate.target->dependency_output_file();
    if (emitted_rust_libs.count(rust_lib) == 0) {
      if (crate.has_direct_access) {
        write_extern_target(*crate.target);
      }
      emitted_rust_libs.insert(rust_lib);
    }
    private_extern_dirs.push_back(
        rust_lib.AsSourceFile(settings_->build_settings()).GetDir());
  }

  // Add explicitly specified externs from the GN target.
  for (ConfigValuesIterator iter(target_); !iter.done(); iter.Next()) {
    const ConfigValues& cur = iter.cur();
    for (const auto& [crate_name, lib_file] : cur.externs()) {
      write_extern_lib_file(crate_name, lib_file);
    }
  }

  out_ << std::endl;
  out_ << "  rustdeps =";

  for (const SourceDir& dir : private_extern_dirs) {
    // TODO: switch to using `--extern priv:name` after stabilization.
    out_ << " -Ldependency=";
    path_output_.WriteDir(out_, dir, PathOutput::DIR_NO_LAST_SLASH);
  }

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
