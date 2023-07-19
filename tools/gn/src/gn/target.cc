// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/target.h"

#include <stddef.h>

#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "gn/c_tool.h"
#include "gn/config_values_extractors.h"
#include "gn/deps_iterator.h"
#include "gn/filesystem_utils.h"
#include "gn/functions.h"
#include "gn/scheduler.h"
#include "gn/substitution_writer.h"
#include "gn/tool.h"
#include "gn/toolchain.h"
#include "gn/trace.h"

namespace {

using ConfigSet = std::set<const Config*>;

// Merges the public configs from the given target to the given config list.
void MergePublicConfigsFrom(const Target* from_target,
                            UniqueVector<LabelConfigPair>* dest) {
  const UniqueVector<LabelConfigPair>& pub = from_target->public_configs();
  dest->Append(pub.begin(), pub.end());
}

// Like MergePublicConfigsFrom above except does the "all dependent" ones. This
// additionally adds all configs to the all_dependent_configs_ of the dest
// target given in *all_dest.
void MergeAllDependentConfigsFrom(const Target* from_target,
                                  UniqueVector<LabelConfigPair>* dest,
                                  UniqueVector<LabelConfigPair>* all_dest) {
  for (const auto& pair : from_target->all_dependent_configs()) {
    all_dest->push_back(pair);
    dest->push_back(pair);
  }
}

Err MakeTestOnlyError(const Item* from, const Item* to) {
  bool with_toolchain = from->settings()->ShouldShowToolchain({
    &from->label(),
    &to->label(),
  });
  return Err(
      from->defined_from(), "Test-only dependency not allowed.",
      from->label().GetUserVisibleName(with_toolchain) +
          "\n"
          "which is NOT marked testonly can't depend on\n" +
          to->label().GetUserVisibleName(with_toolchain) +
          "\n"
          "which is marked testonly. Only targets with \"testonly = true\"\n"
          "can depend on other test-only targets.\n"
          "\n"
          "Either mark it test-only or don't do this dependency.");
}

// Set check_private_deps to true for the first invocation since a target
// can see all of its dependencies. For recursive invocations this will be set
// to false to follow only public dependency paths.
//
// Pass a pointer to an empty set for the first invocation. This will be used
// to avoid duplicate checking.
//
// Checking of object files is optional because it is much slower. This allows
// us to check targets for normal outputs, and then as a second pass check
// object files (since we know it will be an error otherwise). This allows
// us to avoid computing all object file names in the common case.
bool EnsureFileIsGeneratedByDependency(const Target* target,
                                       const OutputFile& file,
                                       bool check_private_deps,
                                       bool consider_object_files,
                                       bool check_data_deps,
                                       TargetSet* seen_targets) {
  if (!seen_targets->add(target))
    return false;  // Already checked this one and it's not found.

  // Assume that we have relatively few generated inputs so brute-force
  // searching here is OK. If this becomes a bottleneck, consider storing
  // computed_outputs as a hash set.
  for (const OutputFile& cur : target->computed_outputs()) {
    if (file == cur)
      return true;
  }

  if (file == target->write_runtime_deps_output())
    return true;

  // Check binary target intermediate files if requested.
  if (consider_object_files && target->IsBinary()) {
    std::vector<OutputFile> source_outputs;
    for (const SourceFile& source : target->sources()) {
      const char* tool_name;
      if (!target->GetOutputFilesForSource(source, &tool_name, &source_outputs))
        continue;
      if (base::ContainsValue(source_outputs, file))
        return true;
    }
  }

  if (check_data_deps) {
    check_data_deps = false;  // Consider only direct data_deps.
    for (const auto& pair : target->data_deps()) {
      if (EnsureFileIsGeneratedByDependency(pair.ptr, file, false,
                                            consider_object_files,
                                            check_data_deps, seen_targets))
        return true;  // Found a path.
    }
  }

  // Check all public dependencies (don't do data ones since those are
  // runtime-only).
  for (const auto& pair : target->public_deps()) {
    if (EnsureFileIsGeneratedByDependency(pair.ptr, file, false,
                                          consider_object_files,
                                          check_data_deps, seen_targets))
      return true;  // Found a path.
  }

  // Only check private deps if requested.
  if (check_private_deps) {
    for (const auto& pair : target->private_deps()) {
      if (EnsureFileIsGeneratedByDependency(pair.ptr, file, false,
                                            consider_object_files,
                                            check_data_deps, seen_targets))
        return true;  // Found a path.
    }
    if (target->output_type() == Target::CREATE_BUNDLE) {
      for (const auto* dep : target->bundle_data().bundle_deps()) {
        if (EnsureFileIsGeneratedByDependency(dep, file, false,
                                              consider_object_files,
                                              check_data_deps, seen_targets))
          return true;  // Found a path.
      }
    }
  }
  return false;
}

// check_this indicates if the given target should be matched against the
// patterns. It should be set to false for the first call since assert_no_deps
// shouldn't match the target itself.
//
// visited should point to an empty set, this will be used to prevent
// multiple visits.
//
// *failure_path_str will be filled with a string describing the path of the
// dependency failure, and failure_pattern will indicate the pattern in
// assert_no that matched the target.
//
// Returns true if everything is OK. failure_path_str and failure_pattern_index
// will be unchanged in this case.
bool RecursiveCheckAssertNoDeps(const Target* target,
                                bool check_this,
                                const std::vector<LabelPattern>& assert_no,
                                TargetSet* visited,
                                std::string* failure_path_str,
                                const LabelPattern** failure_pattern) {
  static const char kIndentPath[] = "  ";

  if (!visited->add(target))
    return true;  // Already checked this target.

  if (check_this) {
    // Check this target against the given list of patterns.
    for (const LabelPattern& pattern : assert_no) {
      if (pattern.Matches(target->label())) {
        // Found a match.
        *failure_pattern = &pattern;
        *failure_path_str =
            kIndentPath + target->label().GetUserVisibleName(false);
        return false;
      }
    }
  }

  // Recursively check dependencies.
  for (const auto& pair : target->GetDeps(Target::DEPS_ALL)) {
    if (pair.ptr->output_type() == Target::EXECUTABLE)
      continue;
    if (!RecursiveCheckAssertNoDeps(pair.ptr, true, assert_no, visited,
                                    failure_path_str, failure_pattern)) {
      // To reconstruct the path, prepend the current target to the error.
      std::string prepend_path =
          kIndentPath + target->label().GetUserVisibleName(false) + " ->\n";
      failure_path_str->insert(0, prepend_path);
      return false;
    }
  }

  return true;
}

}  // namespace

const char kExecution_Help[] =
    R"(Build graph and execution overview

Overall build flow

  1. Look for ".gn" file (see "gn help dotfile") in the current directory and
     walk up the directory tree until one is found. Set this directory to be
     the "source root" and interpret this file to find the name of the build
     config file.

  2. Execute the build config file identified by .gn to set up the global
     variables and default toolchain name. Any arguments, variables, defaults,
     etc. set up in this file will be visible to all files in the build.

  3. Load the //BUILD.gn (in the source root directory).

  4. Recursively evaluate rules and load BUILD.gn in other directories as
     necessary to resolve dependencies. If a BUILD file isn't found in the
     specified location, GN will look in the corresponding location inside
     the secondary_source defined in the dotfile (see "gn help dotfile").

  5. When a target's dependencies are resolved, write out the `.ninja`
     file to disk.

  6. When all targets are resolved, write out the root build.ninja file.

  Note that the BUILD.gn file name may be modulated by .gn arguments such as
  build_file_extension.

Executing target definitions and templates

  Build files are loaded in parallel. This means it is impossible to
  interrogate a target from GN code for any information not derivable from its
  label (see "gn help label"). The exception is the get_target_outputs()
  function which requires the target being interrogated to have been defined
  previously in the same file.

  Targets are declared by their type and given a name:

    static_library("my_static_library") {
      ... target parameter definitions ...
    }

  There is also a generic "target" function for programmatically defined types
  (see "gn help target"). You can define new types using templates (see "gn
  help template"). A template defines some custom code that expands to one or
  more other targets.

  Before executing the code inside the target's { }, the target defaults are
  applied (see "gn help set_defaults"). It will inject implicit variable
  definitions that can be overridden by the target code as necessary. Typically
  this mechanism is used to inject a default set of configs that define the
  global compiler and linker flags.

Which targets are built

  All targets encountered in the default toolchain (see "gn help toolchain")
  will have build rules generated for them, even if no other targets reference
  them. Their dependencies must resolve and they will be added to the implicit
  "all" rule (see "gn help ninja_rules").

  Targets in non-default toolchains will only be generated when they are
  required (directly or transitively) to build a target in the default
  toolchain.

  Some targets might be associated but without a formal build dependency (for
  example, related tools or optional variants). A target that is marked as
  "generated" can propagate its generated state to an associated target using
  "gen_deps". This will make the referenced dependency have Ninja rules
  generated in the same cases the source target has but without a build-time
  dependency and even in non-default toolchains.

  See also "gn help ninja_rules".

Dependencies

  The only difference between "public_deps" and "deps" except for pushing
  configs around the build tree and allowing includes for the purposes of "gn
  check".

  A target's "data_deps" are guaranteed to be built whenever the target is
  built, but the ordering is not defined. The meaning of this is dependencies
  required at runtime. Currently data deps will be complete before the target
  is linked, but this is not semantically guaranteed and this is undesirable
  from a build performance perspective. Since we hope to change this in the
  future, do not rely on this behavior.
)";

Target::Target(const Settings* settings,
               const Label& label,
               const SourceFileSet& build_dependency_files)
    : Item(settings, label, build_dependency_files) {}

Target::~Target() = default;

// A technical note on accessors defined below: Using a static global
// constant is much faster at runtime than using a static local one.
//
// In other words:
//
//   static const Foo kEmptyFoo;
//
//   const Foo& Target::foo() const {
//     return foo_ ? *foo_ : kEmptyFoo;
//   }
//
// Is considerably faster than:
//
//   const Foo& Target::foo() const {
//     if (foo_) {
//       return *foo_;
//     } else {
//       static const Foo kEmptyFoo;
//       return kEmptyFoo;
//     }
//   }
//
// Because the latter requires relatively expensive atomic operations
// in the second branch.
//

static const BundleData kEmptyBundleData;

const BundleData& Target::bundle_data() const {
  return bundle_data_ ? *bundle_data_ : kEmptyBundleData;
}

BundleData& Target::bundle_data() {
  if (!bundle_data_)
    bundle_data_ = std::make_unique<BundleData>();
  return *bundle_data_;
}

static ConfigValues kEmptyConfigValues;

const ConfigValues& Target::config_values() const {
  return config_values_ ? *config_values_ : kEmptyConfigValues;
}

ConfigValues& Target::config_values() {
  if (!config_values_)
    config_values_ = std::make_unique<ConfigValues>();
  return *config_values_;
}

static const ActionValues kEmptyActionValues;

const ActionValues& Target::action_values() const {
  return action_values_ ? *action_values_ : kEmptyActionValues;
}

ActionValues& Target::action_values() {
  if (!action_values_)
    action_values_ = std::make_unique<ActionValues>();
  return *action_values_;
}

static const RustValues kEmptyRustValues;

const RustValues& Target::rust_values() const {
  return rust_values_ ? *rust_values_ : kEmptyRustValues;
}

RustValues& Target::rust_values() {
  if (!rust_values_)
    rust_values_ = std::make_unique<RustValues>();
  return *rust_values_;
}

static const SwiftValues kEmptySwiftValues;

const SwiftValues& Target::swift_values() const {
  return swift_values_ ? *swift_values_ : kEmptySwiftValues;
}

SwiftValues& Target::swift_values() {
  if (!swift_values_)
    swift_values_ = std::make_unique<SwiftValues>();
  return *swift_values_;
}

static const Metadata kEmptyMetadata;

const Metadata& Target::metadata() const {
  return metadata_ ? *metadata_ : kEmptyMetadata;
}

Metadata& Target::metadata() {
  if (!metadata_)
    metadata_ = std::make_unique<Metadata>();
  return *metadata_;
}

static const Target::GeneratedFile kEmptyGeneratedFile;

const Target::GeneratedFile& Target::generated_file() const {
  return generated_file_ ? *generated_file_ : kEmptyGeneratedFile;
}

Target::GeneratedFile& Target::generated_file() {
  if (!generated_file_)
    generated_file_ = std::make_unique<Target::GeneratedFile>();
  return *generated_file_;
}

// static
const char* Target::GetStringForOutputType(OutputType type) {
  switch (type) {
    case UNKNOWN:
      return "unknown";
    case GROUP:
      return functions::kGroup;
    case EXECUTABLE:
      return functions::kExecutable;
    case LOADABLE_MODULE:
      return functions::kLoadableModule;
    case SHARED_LIBRARY:
      return functions::kSharedLibrary;
    case STATIC_LIBRARY:
      return functions::kStaticLibrary;
    case SOURCE_SET:
      return functions::kSourceSet;
    case COPY_FILES:
      return functions::kCopy;
    case ACTION:
      return functions::kAction;
    case ACTION_FOREACH:
      return functions::kActionForEach;
    case BUNDLE_DATA:
      return functions::kBundleData;
    case CREATE_BUNDLE:
      return functions::kCreateBundle;
    case GENERATED_FILE:
      return functions::kGeneratedFile;
    case RUST_LIBRARY:
      return functions::kRustLibrary;
    case RUST_PROC_MACRO:
      return functions::kRustProcMacro;
    default:
      return "";
  }
}

Target* Target::AsTarget() {
  return this;
}

const Target* Target::AsTarget() const {
  return this;
}

bool Target::OnResolved(Err* err) {
  DCHECK(output_type_ != UNKNOWN);
  DCHECK(toolchain_) << "Toolchain should have been set before resolving.";

  ScopedTrace trace(TraceItem::TRACE_ON_RESOLVED, label());
  trace.SetToolchain(settings()->toolchain_label());

  // Copy this target's own dependent and public configs to the list of configs
  // applying to it.
  configs_.Append(all_dependent_configs_.begin(), all_dependent_configs_.end());
  MergePublicConfigsFrom(this, &configs_);

  // Check visibility for just this target's own configs, before dependents are
  // added, but after public_configs and all_dependent_configs are merged.
  if (!CheckConfigVisibility(err))
    return false;

  // Copy public configs from all dependencies into the list of configs
  // applying to this target (configs_).
  PullDependentTargetConfigs();

  // Copies public dependencies' public configs to this target's public
  // configs. These configs have already been applied to this target by
  // PullDependentTargetConfigs above, along with the public configs from
  // private deps. This step re-exports them as public configs for targets that
  // depend on this one.
  for (const auto& dep : public_deps_) {
    if (dep.ptr->toolchain() == toolchain() ||
        dep.ptr->toolchain()->propagates_configs())
      public_configs_.Append(dep.ptr->public_configs().begin(),
                             dep.ptr->public_configs().end());
  }

  PullRecursiveBundleData();
  if (!ResolvePrecompiledHeaders(err))
    return false;

  if (!FillOutputFiles(err))
    return false;

  if (!SwiftValues::OnTargetResolved(this, err))
    return false;

  if (!CheckSourceSetLanguages(err))
    return false;
  if (!CheckVisibility(err))
    return false;
  if (!CheckTestonly(err))
    return false;
  if (!CheckAssertNoDeps(err))
    return false;
  CheckSourcesGenerated();

  if (!write_runtime_deps_output_.value().empty())
    g_scheduler->AddWriteRuntimeDepsTarget(this);

  if (output_type_ == GENERATED_FILE) {
    DCHECK(!computed_outputs_.empty());
    g_scheduler->AddGeneratedFile(
        computed_outputs_[0].AsSourceFile(settings()->build_settings()));
  }

  return true;
}

bool Target::IsBinary() const {
  return output_type_ == EXECUTABLE || output_type_ == SHARED_LIBRARY ||
         output_type_ == LOADABLE_MODULE || output_type_ == STATIC_LIBRARY ||
         output_type_ == SOURCE_SET || output_type_ == RUST_LIBRARY ||
         output_type_ == RUST_PROC_MACRO;
}

bool Target::IsLinkable() const {
  return output_type_ == STATIC_LIBRARY || output_type_ == SHARED_LIBRARY ||
         output_type_ == RUST_LIBRARY || output_type_ == RUST_PROC_MACRO;
}

bool Target::IsFinal() const {
  return output_type_ == EXECUTABLE || output_type_ == SHARED_LIBRARY ||
         output_type_ == LOADABLE_MODULE || output_type_ == ACTION ||
         output_type_ == ACTION_FOREACH || output_type_ == COPY_FILES ||
         output_type_ == CREATE_BUNDLE || output_type_ == RUST_PROC_MACRO ||
         (output_type_ == STATIC_LIBRARY && complete_static_lib_);
}

bool Target::IsDataOnly() const {
  // BUNDLE_DATA exists only to declare inputs to subsequent CREATE_BUNDLE
  // targets. Changing only contents of the bundle data target should not cause
  // a binary to be re-linked. It should affect only the CREATE_BUNDLE steps
  // instead. As a result, normal targets should treat this as a data
  // dependency.
  return output_type_ == BUNDLE_DATA;
}

DepsIteratorRange Target::GetDeps(DepsIterationType type) const {
  if (type == DEPS_LINKED) {
    return DepsIteratorRange(
        DepsIterator(&public_deps_, &private_deps_, nullptr));
  }
  // All deps.
  return DepsIteratorRange(
      DepsIterator(&public_deps_, &private_deps_, &data_deps_));
}

std::string Target::GetComputedOutputName() const {
  DCHECK(toolchain_)
      << "Toolchain must be specified before getting the computed output name.";

  const std::string& name =
      output_name_.empty() ? label().name() : output_name_;

  std::string result;
  const Tool* tool = toolchain_->GetToolForTargetFinalOutput(this);
  if (tool) {
    // Only add the prefix if the name doesn't already have it and it's not
    // being overridden.
    if (!output_prefix_override_ &&
        !base::StartsWith(name, tool->output_prefix(),
                          base::CompareCase::SENSITIVE))
      result = tool->output_prefix();
  }
  result.append(name);
  return result;
}

bool Target::SetToolchain(const Toolchain* toolchain, Err* err) {
  DCHECK(!toolchain_);
  DCHECK_NE(UNKNOWN, output_type_);
  toolchain_ = toolchain;

  const Tool* tool = toolchain->GetToolForTargetFinalOutput(this);
  if (tool)
    return true;

  // Tool not specified for this target type.
  if (err) {
    *err =
        Err(defined_from(), "This target uses an undefined tool.",
            base::StringPrintf(
                "The target %s\n"
                "of type \"%s\"\n"
                "uses toolchain %s\n"
                "which doesn't have the tool \"%s\" defined.\n\n"
                "Alas, I can not continue.",
                label().GetUserVisibleName(false).c_str(),
                GetStringForOutputType(output_type_),
                label().GetToolchainLabel().GetUserVisibleName(false).c_str(),
                Tool::GetToolTypeForTargetFinalOutput(this)));
  }
  return false;
}

bool Target::GetOutputsAsSourceFiles(const LocationRange& loc_for_error,
                                     bool build_complete,
                                     std::vector<SourceFile>* outputs,
                                     Err* err) const {
  const static char kBuildIncompleteMsg[] =
      "This target is a binary target which can't be queried for its "
      "outputs\nduring the build. It will work for action, action_foreach, "
      "generated_file,\nand copy targets.";

  outputs->clear();

  std::vector<SourceFile> files;
  if (output_type() == Target::ACTION || output_type() == Target::COPY_FILES ||
      output_type() == Target::ACTION_FOREACH ||
      output_type() == Target::GENERATED_FILE) {
    action_values().GetOutputsAsSourceFiles(this, outputs);
  } else if (output_type() == Target::CREATE_BUNDLE) {
    if (!bundle_data().GetOutputsAsSourceFiles(settings(), this, outputs, err))
      return false;
  } else if (IsBinary() && output_type() != Target::SOURCE_SET) {
    // Binary target with normal outputs (source sets have stamp outputs like
    // groups).
    DCHECK(IsBinary()) << static_cast<int>(output_type());
    if (!build_complete) {
      // Can't access the toolchain for a target before the build is complete.
      // Otherwise it will race with loading and setting the toolchain
      // definition.
      *err = Err(loc_for_error, kBuildIncompleteMsg);
      return false;
    }

    const Tool* tool = toolchain()->GetToolForTargetFinalOutput(this);

    std::vector<OutputFile> output_files;
    SubstitutionWriter::ApplyListToLinkerAsOutputFile(
        this, tool, tool->outputs(), &output_files);
    for (const OutputFile& output_file : output_files) {
      outputs->push_back(
          output_file.AsSourceFile(settings()->build_settings()));
    }
  } else {
    // Everything else (like a group or bundle_data) has a stamp output. The
    // dependency output file should have computed what this is. This won't be
    // valid unless the build is complete.
    if (!build_complete) {
      *err = Err(loc_for_error, kBuildIncompleteMsg);
      return false;
    }
    outputs->push_back(
        dependency_output_file().AsSourceFile(settings()->build_settings()));
  }
  return true;
}

bool Target::GetOutputFilesForSource(const SourceFile& source,
                                     const char** computed_tool_type,
                                     std::vector<OutputFile>* outputs) const {
  DCHECK(toolchain());  // Should be resolved before calling.

  outputs->clear();
  *computed_tool_type = Tool::kToolNone;

  if (output_type() == Target::COPY_FILES ||
      output_type() == Target::ACTION_FOREACH) {
    // These target types apply the output pattern to the input.
    std::vector<SourceFile> output_files;
    SubstitutionWriter::ApplyListToSourceAsOutputFile(
        this, settings(), action_values().outputs(), source, outputs);
  } else if (!IsBinary()) {
    // All other non-binary target types just return the target outputs. We
    // don't know if the build is complete and it doesn't matter for non-binary
    // targets, so just assume it's not and pass "false".
    std::vector<SourceFile> outputs_as_source_files;
    Err err;  // We can ignore the error and return empty for failure.
    GetOutputsAsSourceFiles(LocationRange(), false, &outputs_as_source_files,
                            &err);

    // Convert to output files.
    for (const auto& cur : outputs_as_source_files)
      outputs->emplace_back(OutputFile(settings()->build_settings(), cur));
  } else {
    // All binary targets do a tool lookup.
    DCHECK(IsBinary());

    const SourceFile::Type file_type = source.GetType();
    if (file_type == SourceFile::SOURCE_UNKNOWN)
      return false;
    if (file_type == SourceFile::SOURCE_O) {
      // Object files just get passed to the output and not compiled.
      outputs->emplace_back(OutputFile(settings()->build_settings(), source));
      return true;
    }

    // Rust generates on a module level, not source.
    if (file_type == SourceFile::SOURCE_RS)
      return false;

    *computed_tool_type = Tool::GetToolTypeForSourceType(file_type);
    if (*computed_tool_type == Tool::kToolNone)
      return false;  // No tool for this file (it's a header file or something).
    const Tool* tool = toolchain_->GetTool(*computed_tool_type);
    if (!tool)
      return false;  // Tool does not apply for this toolchain.file.

    // Swift may generate on a module or source level.
    if (file_type == SourceFile::SOURCE_SWIFT) {
      if (tool->partial_outputs().list().empty())
        return false;
    }

    const SubstitutionList& substitution_list =
        file_type == SourceFile::SOURCE_SWIFT ? tool->partial_outputs()
                                              : tool->outputs();

    // Figure out what output(s) this compiler produces.
    SubstitutionWriter::ApplyListToCompilerAsOutputFile(
        this, source, substitution_list, outputs);
  }
  return !outputs->empty();
}

void Target::PullDependentTargetConfigs() {
  for (const auto& pair : GetDeps(DEPS_LINKED)) {
    if (pair.ptr->toolchain() == toolchain() ||
        pair.ptr->toolchain()->propagates_configs())
      MergeAllDependentConfigsFrom(pair.ptr, &configs_,
                                   &all_dependent_configs_);
  }
  for (const auto& pair : GetDeps(DEPS_LINKED)) {
    if (pair.ptr->toolchain() == toolchain() ||
        pair.ptr->toolchain()->propagates_configs())
      MergePublicConfigsFrom(pair.ptr, &configs_);
  }
}

void Target::PullRecursiveBundleData() {
  const bool is_create_bundle = output_type_ == CREATE_BUNDLE;
  for (const auto& pair : GetDeps(DEPS_LINKED)) {
    // Don't propagate across toolchain.
    if (pair.ptr->toolchain() != toolchain())
      continue;

    // Don't propagete through create_bundle, unless it is transparent.
    if (pair.ptr->output_type() == CREATE_BUNDLE &&
        !pair.ptr->bundle_data().transparent()) {
      continue;
    }

    // Direct dependency on a bundle_data target.
    if (pair.ptr->output_type() == BUNDLE_DATA) {
      bundle_data().AddBundleData(pair.ptr, is_create_bundle);
    }

    // Recursive bundle_data informations from all dependencies.
    if (pair.ptr->has_bundle_data()) {
      for (const auto* target : pair.ptr->bundle_data().forwarded_bundle_deps())
        bundle_data().AddBundleData(target, is_create_bundle);
    }
  }

  if (has_bundle_data())
    bundle_data().OnTargetResolved(this);
}

bool Target::FillOutputFiles(Err* err) {
  const Tool* tool = toolchain_->GetToolForTargetFinalOutput(this);
  bool check_tool_outputs = false;
  switch (output_type_) {
    case GROUP:
    case BUNDLE_DATA:
    case CREATE_BUNDLE:
    case SOURCE_SET:
    case COPY_FILES:
    case ACTION:
    case ACTION_FOREACH:
    case GENERATED_FILE: {
      // These don't get linked to and use stamps which should be the first
      // entry in the outputs. These stamps are named
      // "<target_out_dir>/<targetname>.stamp". Setting "output_name" does not
      // affect the stamp file name: it is always based on the original target
      // name.
      dependency_output_file_ =
          GetBuildDirForTargetAsOutputFile(this, BuildDirType::OBJ);
      dependency_output_file_.value().append(label().name());
      dependency_output_file_.value().append(".stamp");
      break;
    }
    case EXECUTABLE:
    case LOADABLE_MODULE:
      // Executables and loadable modules don't get linked to, but the first
      // output is used for dependency management.
      CHECK_GE(tool->outputs().list().size(), 1u);
      check_tool_outputs = true;
      dependency_output_file_ =
          SubstitutionWriter::ApplyPatternToLinkerAsOutputFile(
              this, tool, tool->outputs().list()[0]);

      if (tool->runtime_outputs().list().empty()) {
        // Default to the first output for the runtime output.
        runtime_outputs_.push_back(dependency_output_file_);
      } else {
        SubstitutionWriter::ApplyListToLinkerAsOutputFile(
            this, tool, tool->runtime_outputs(), &runtime_outputs_);
      }
      break;
    case RUST_LIBRARY:
    case STATIC_LIBRARY:
      // Static libraries both have dependencies and linking going off of the
      // first output.
      CHECK(tool->outputs().list().size() >= 1);
      check_tool_outputs = true;
      link_output_file_ = dependency_output_file_ =
          SubstitutionWriter::ApplyPatternToLinkerAsOutputFile(
              this, tool, tool->outputs().list()[0]);
      break;
    case RUST_PROC_MACRO:
    case SHARED_LIBRARY:
      CHECK(tool->outputs().list().size() >= 1);
      check_tool_outputs = true;
      if (const CTool* ctool = tool->AsC()) {
        if (ctool->link_output().empty() && ctool->depend_output().empty()) {
          // Default behavior, use the first output file for both.
          link_output_file_ = dependency_output_file_ =
              SubstitutionWriter::ApplyPatternToLinkerAsOutputFile(
                  this, tool, tool->outputs().list()[0]);
        } else {
          // Use the tool-specified ones.
          if (!ctool->link_output().empty()) {
            link_output_file_ =
                SubstitutionWriter::ApplyPatternToLinkerAsOutputFile(
                    this, tool, ctool->link_output());
          }
          if (!ctool->depend_output().empty()) {
            dependency_output_file_ =
                SubstitutionWriter::ApplyPatternToLinkerAsOutputFile(
                    this, tool, ctool->depend_output());
          }
        }
        if (tool->runtime_outputs().list().empty()) {
          // Default to the link output for the runtime output.
          runtime_outputs_.push_back(link_output_file_);
        } else {
          SubstitutionWriter::ApplyListToLinkerAsOutputFile(
              this, tool, tool->runtime_outputs(), &runtime_outputs_);
        }
      } else if (tool->AsRust()) {
        // Default behavior, use the first output file for both.
        link_output_file_ = dependency_output_file_ =
            SubstitutionWriter::ApplyPatternToLinkerAsOutputFile(
                this, tool, tool->outputs().list()[0]);
      }
      break;
    case UNKNOWN:
    default:
      NOTREACHED();
  }

  // Count anything generated from bundle_data dependencies.
  if (output_type_ == CREATE_BUNDLE) {
    if (!bundle_data().GetOutputFiles(settings(), this, &computed_outputs_,
                                      err))
      return false;
  }

  // Count all outputs from this tool as something generated by this target.
  if (check_tool_outputs) {
    SubstitutionWriter::ApplyListToLinkerAsOutputFile(
        this, tool, tool->outputs(), &computed_outputs_);

    // Output names aren't canonicalized in the same way that source files
    // are. For example, the tool outputs often use
    // {{some_var}}/{{output_name}} which expands to "./foo", but this won't
    // match "foo" which is what we'll compute when converting a SourceFile to
    // an OutputFile.
    for (auto& out : computed_outputs_)
      NormalizePath(&out.value());
  }

  // Also count anything the target has declared to be an output.
  if (action_values_.get()) {
    std::vector<SourceFile> outputs_as_sources;
    action_values_->GetOutputsAsSourceFiles(this, &outputs_as_sources);
    for (const SourceFile& out : outputs_as_sources)
      computed_outputs_.push_back(
          OutputFile(settings()->build_settings(), out));
  }

  return true;
}

bool Target::ResolvePrecompiledHeaders(Err* err) {
  // Precompiled headers are stored on a ConfigValues struct. This way, the
  // build can set all the precompiled header settings in a config and apply
  // it to many targets. Likewise, the precompiled header values may be
  // specified directly on a target.
  //
  // Unlike other values on configs which are lists that just get concatenated,
  // the precompiled header settings are unique values. We allow them to be
  // specified anywhere, but if they are specified in more than one place all
  // places must match.

  // Track where the current settings came from for issuing errors.
  bool has_precompiled_headers =
      config_values_.get() && config_values_->has_precompiled_headers();
  const Label* pch_header_settings_from = NULL;
  if (has_precompiled_headers)
    pch_header_settings_from = &label();

  for (ConfigValuesIterator iter(this); !iter.done(); iter.Next()) {
    if (!iter.GetCurrentConfig())
      continue;  // Skip the one on the target itself.

    const Config* config = iter.GetCurrentConfig();
    const ConfigValues& cur = config->resolved_values();
    if (!cur.has_precompiled_headers())
      continue;  // This one has no precompiled header info, skip.

    if (has_precompiled_headers) {
      // Already have a precompiled header values, the settings must match.
      if (config_values_->precompiled_header() != cur.precompiled_header() ||
          config_values_->precompiled_source() != cur.precompiled_source()) {
        bool with_toolchain = settings()->ShouldShowToolchain({
          &label(),
          pch_header_settings_from,
          &config->label(),
        });
        *err = Err(
            defined_from(), "Precompiled header setting conflict.",
            "The target " + label().GetUserVisibleName(with_toolchain) +
                "\n"
                "has conflicting precompiled header settings.\n"
                "\n"
                "From " +
                pch_header_settings_from->GetUserVisibleName(with_toolchain) +
                "\n  header: " + config_values_->precompiled_header() +
                "\n  source: " + config_values_->precompiled_source().value() +
                "\n\n"
                "From " +
                config->label().GetUserVisibleName(with_toolchain) +
                "\n  header: " + cur.precompiled_header() +
                "\n  source: " + cur.precompiled_source().value());
        return false;
      }
    } else {
      // Have settings from a config, apply them to ourselves.
      pch_header_settings_from = &config->label();
      config_values().set_precompiled_header(cur.precompiled_header());
      config_values().set_precompiled_source(cur.precompiled_source());
    }
  }

  return true;
}

bool Target::CheckVisibility(Err* err) const {
  for (const auto& pair : GetDeps(DEPS_ALL)) {
    if (!Visibility::CheckItemVisibility(this, pair.ptr, err))
      return false;
  }
  return true;
}

bool Target::CheckConfigVisibility(Err* err) const {
  for (ConfigValuesIterator iter(this); !iter.done(); iter.Next()) {
    if (const Config* config = iter.GetCurrentConfig())
      if (!Visibility::CheckItemVisibility(this, config, err))
        return false;
  }
  return true;
}

bool Target::CheckSourceSetLanguages(Err* err) const {
  if (output_type() == Target::SOURCE_SET &&
      source_types_used().RustSourceUsed()) {
    *err = Err(defined_from(), "source_set contained Rust code.",
               label().GetUserVisibleName(!settings()->is_default()) +
                   " has Rust code. Only C/C++ source_sets are supported.");
    return false;
  }
  return true;
}

bool Target::CheckTestonly(Err* err) const {
  // If the current target is marked testonly, it can include both testonly
  // and non-testonly targets, so there's nothing to check.
  if (testonly())
    return true;

  // Verify no deps have "testonly" set.
  for (const auto& pair : GetDeps(DEPS_ALL)) {
    if (pair.ptr->testonly()) {
      *err = MakeTestOnlyError(this, pair.ptr);
      return false;
    }
  }

  // Verify no configs have "testonly" set.
  for (ConfigValuesIterator iter(this); !iter.done(); iter.Next()) {
    if (const Config* config = iter.GetCurrentConfig()) {
      if (config->testonly()) {
        *err = MakeTestOnlyError(this, config);
        return false;
      }
    }
  }

  return true;
}

bool Target::CheckAssertNoDeps(Err* err) const {
  if (assert_no_deps_.empty())
    return true;

  TargetSet visited;
  std::string failure_path_str;
  const LabelPattern* failure_pattern = nullptr;

  if (!RecursiveCheckAssertNoDeps(this, false, assert_no_deps_, &visited,
                                  &failure_path_str, &failure_pattern)) {
    *err = Err(
        defined_from(), "assert_no_deps failed.",
        label().GetUserVisibleName(!settings()->is_default()) +
            " has an assert_no_deps entry:\n  " + failure_pattern->Describe() +
            "\nwhich fails for the dependency path:\n" + failure_path_str);
    return false;
  }
  return true;
}

void Target::CheckSourcesGenerated() const {
  // Checks that any inputs or sources to this target that are in the build
  // directory are generated by a target that this one transitively depends on
  // in some way. We already guarantee that all generated files are written
  // to the build dir.
  //
  // See Scheduler::AddUnknownGeneratedInput's declaration for more.
  for (const SourceFile& file : sources_)
    CheckSourceGenerated(file);
  for (ConfigValuesIterator iter(this); !iter.done(); iter.Next()) {
    for (const SourceFile& file : iter.cur().inputs())
      CheckSourceGenerated(file);
  }
  // TODO(agrieve): Check all_libs_ here as well (those that are source files).
  // http://crbug.com/571731
}

void Target::CheckSourceGenerated(const SourceFile& source) const {
  if (!IsStringInOutputDir(settings()->build_settings()->build_dir(),
                           source.value()))
    return;  // Not in output dir, this is OK.

  // Tell the scheduler about unknown files. This will be noted for later so
  // the list of files written by the GN build itself (often response files)
  // can be filtered out of this list.
  OutputFile out_file(settings()->build_settings(), source);
  TargetSet seen_targets;
  bool check_data_deps = false;
  bool consider_object_files = false;
  if (!EnsureFileIsGeneratedByDependency(this, out_file, true,
                                         consider_object_files, check_data_deps,
                                         &seen_targets)) {
    seen_targets.clear();
    // Allow dependency to be through data_deps for files generated by gn.
    check_data_deps =
        g_scheduler->IsFileGeneratedByWriteRuntimeDeps(out_file) ||
        g_scheduler->IsFileGeneratedByTarget(source);
    // Check object files (much slower and very rare) only if the "normal"
    // output check failed.
    consider_object_files = !check_data_deps;
    if (!EnsureFileIsGeneratedByDependency(this, out_file, true,
                                           consider_object_files,
                                           check_data_deps, &seen_targets))
      g_scheduler->AddUnknownGeneratedInput(this, source);
  }
}

bool Target::GetMetadata(const std::vector<std::string>& keys_to_extract,
                         const std::vector<std::string>& keys_to_walk,
                         const SourceDir& rebase_dir,
                         bool deps_only,
                         std::vector<Value>* result,
                         TargetSet* targets_walked,
                         Err* err) const {
  std::vector<Value> next_walk_keys;
  std::vector<Value> current_result;
  // If deps_only, this is the top-level target and thus we don't want to
  // collect its metadata, only that of its deps and data_deps.
  if (deps_only) {
    // Empty string will be converted below to mean all deps and data_deps.
    // Origin is null because this isn't declared anywhere, and should never
    // trigger any errors.
    next_walk_keys.push_back(Value(nullptr, ""));
  } else {
    // Otherwise, we walk this target and collect the appropriate data.
    // NOTE: Always call WalkStep() even when have_metadata() is false,
    // because WalkStep() will append to 'next_walk_keys' in this case.
    // See https://crbug.com/1273069.
    if (!metadata().WalkStep(settings()->build_settings(), keys_to_extract,
                             keys_to_walk, rebase_dir, &next_walk_keys,
                             &current_result, err))
      return false;
  }

  // Gather walk keys and find the appropriate target. Targets identified in
  // the walk key set must be deps or data_deps of the declaring target.
  const DepsIteratorRange& all_deps = GetDeps(Target::DEPS_ALL);
  const SourceDir& current_dir = label().dir();
  for (const auto& next : next_walk_keys) {
    DCHECK(next.type() == Value::STRING);

    // If we hit an empty string in this list, add all deps and data_deps. The
    // ordering in the resulting list of values as a result will be the data
    // from each explicitly listed dep prior to this, followed by all data in
    // walk order of the remaining deps.
    if (next.string_value().empty()) {
      for (const auto& dep : all_deps) {
        // If we haven't walked this dep yet, go down into it.
        if (targets_walked->add(dep.ptr)) {
          if (!dep.ptr->GetMetadata(keys_to_extract, keys_to_walk, rebase_dir,
                                    false, result, targets_walked, err))
            return false;
        }
      }

      // Any other walk keys are superfluous, as they can only be a subset of
      // all deps.
      break;
    }

    // Otherwise, look through the target's deps for the specified one.
    // Canonicalize the label if possible.
    Label next_label = Label::Resolve(
        current_dir, settings()->build_settings()->root_path_utf8(),
        settings()->toolchain_label(), next, err);
    if (next_label.is_null()) {
      *err = Err(next.origin(), std::string("Failed to canonicalize ") +
                                    next.string_value() + std::string("."));
    }
    std::string canonicalize_next_label = next_label.GetUserVisibleName(true);

    bool found_next = false;
    for (const auto& dep : all_deps) {
      // Match against the label with the toolchain.
      if (dep.label.GetUserVisibleName(true) == canonicalize_next_label) {
        // If we haven't walked this dep yet, go down into it.
        if (targets_walked->add(dep.ptr)) {
          if (!dep.ptr->GetMetadata(keys_to_extract, keys_to_walk, rebase_dir,
                                    false, result, targets_walked, err))
            return false;
        }
        // We found it, so we can exit this search now.
        found_next = true;
        break;
      }
    }
    // If we didn't find the specified dep in the target, that's an error.
    // Propagate it back to the user.
    if (!found_next) {
      *err = Err(next.origin(),
                 std::string("I was expecting ") + canonicalize_next_label +
                     std::string(" to be a dependency of ") +
                     label().GetUserVisibleName(true) +
                     ". Make sure it's included in the deps or data_deps, and "
                     "that you've specified the appropriate toolchain.");
      return false;
    }
  }
  result->insert(result->end(), std::make_move_iterator(current_result.begin()),
                 std::make_move_iterator(current_result.end()));
  return true;
}
