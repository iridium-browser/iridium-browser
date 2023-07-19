// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_TARGET_H_
#define TOOLS_GN_TARGET_H_

#include <set>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/logging.h"
#include "gn/action_values.h"
#include "gn/bundle_data.h"
#include "gn/config_values.h"
#include "gn/item.h"
#include "gn/label_pattern.h"
#include "gn/label_ptr.h"
#include "gn/lib_file.h"
#include "gn/metadata.h"
#include "gn/output_file.h"
#include "gn/pointer_set.h"
#include "gn/rust_values.h"
#include "gn/source_file.h"
#include "gn/swift_values.h"
#include "gn/toolchain.h"
#include "gn/unique_vector.h"

class DepsIteratorRange;
class Settings;
class Target;
class Toolchain;

using TargetSet = PointerSet<const Target>;

class Target : public Item {
 public:
  enum OutputType {
    UNKNOWN,
    GROUP,
    EXECUTABLE,
    SHARED_LIBRARY,
    LOADABLE_MODULE,
    STATIC_LIBRARY,
    SOURCE_SET,
    COPY_FILES,
    ACTION,
    ACTION_FOREACH,
    BUNDLE_DATA,
    CREATE_BUNDLE,
    GENERATED_FILE,
    RUST_LIBRARY,
    RUST_PROC_MACRO,
  };

  enum DepsIterationType {
    DEPS_ALL,     // Iterates through all public, private, and data deps.
    DEPS_LINKED,  // Iterates through all non-data dependencies.
  };

  using FileList = std::vector<SourceFile>;
  using StringVector = std::vector<std::string>;

  // We track the set of build files that may affect this target, please refer
  // to Scope for how this is determined.
  Target(const Settings* settings,
         const Label& label,
         const SourceFileSet& build_dependency_files = {});
  ~Target() override;

  // Returns a string naming the output type.
  static const char* GetStringForOutputType(OutputType type);

  // Item overrides.
  Target* AsTarget() override;
  const Target* AsTarget() const override;
  bool OnResolved(Err* err) override;

  OutputType output_type() const { return output_type_; }
  void set_output_type(OutputType t) { output_type_ = t; }

  // True for targets that compile source code (all types of libraries and
  // executables).
  bool IsBinary() const;

  // Can be linked into other targets.
  bool IsLinkable() const;

  // True if the target links dependencies rather than propagated up the graph.
  // This is also true of action and copy steps even though they don't link
  // dependencies, because they also don't propagate libraries up.
  bool IsFinal() const;

  // Set when the target should normally be treated as a data dependency. These
  // do not need to be treated as inputs or hard dependencies for normal build
  // steps, but have to be kept in the dependency tree to be properly
  // propagated. Treating these as data only decreases superfluous rebuilds and
  // increases parallelism.
  bool IsDataOnly() const;

  // Will be the empty string to use the target label as the output name.
  // See GetComputedOutputName().
  const std::string& output_name() const { return output_name_; }
  void set_output_name(const std::string& name) { output_name_ = name; }

  // Returns the output name for this target, which is the output_name if
  // specified, or the target label if not.
  //
  // Because this depends on the tool for this target, the toolchain must
  // have been set before calling.
  std::string GetComputedOutputName() const;

  bool output_prefix_override() const { return output_prefix_override_; }
  void set_output_prefix_override(bool prefix_override) {
    output_prefix_override_ = prefix_override;
  }

  // Desired output directory for the final output. This will be used for
  // the {{output_dir}} substitution in the tool if it is specified. If
  // is_null, the tool default will be used.
  const SourceDir& output_dir() const { return output_dir_; }
  void set_output_dir(const SourceDir& dir) { output_dir_ = dir; }

  // The output extension is really a tri-state: unset (output_extension_set
  // is false and the string is empty, meaning the default extension should be
  // used), the output extension is set but empty (output should have no
  // extension) and the output extension is set but nonempty (use the given
  // extension).
  const std::string& output_extension() const { return output_extension_; }
  void set_output_extension(const std::string& extension) {
    output_extension_ = extension;
    output_extension_set_ = true;
  }
  bool output_extension_set() const { return output_extension_set_; }

  const FileList& sources() const { return sources_; }
  FileList& sources() { return sources_; }

  const SourceFileTypeSet& source_types_used() const {
    return source_types_used_;
  }
  SourceFileTypeSet& source_types_used() { return source_types_used_; }

  // Set to true when all sources are public. This is the default. In this case
  // the public headers list should be empty.
  bool all_headers_public() const { return all_headers_public_; }
  void set_all_headers_public(bool p) { all_headers_public_ = p; }

  // When all_headers_public is false, this is the list of public headers. It
  // could be empty which would mean no headers are public.
  const FileList& public_headers() const { return public_headers_; }
  FileList& public_headers() { return public_headers_; }

  // Whether this target's includes should be checked by "gn check".
  bool check_includes() const { return check_includes_; }
  void set_check_includes(bool ci) { check_includes_ = ci; }

  // Whether this static_library target should have code linked in.
  bool complete_static_lib() const { return complete_static_lib_; }
  void set_complete_static_lib(bool complete) {
    DCHECK_EQ(STATIC_LIBRARY, output_type_);
    complete_static_lib_ = complete;
  }

  // Metadata. Target takes ownership of the resulting scope.
  const Metadata& metadata() const;
  Metadata& metadata();
  bool has_metadata() const { return metadata_.get(); }

  // Get metadata from this target and its dependencies. This is intended to
  // be called after the target is resolved.
  bool GetMetadata(const std::vector<std::string>& keys_to_extract,
                   const std::vector<std::string>& keys_to_walk,
                   const SourceDir& rebase_dir,
                   bool deps_only,
                   std::vector<Value>* result,
                   TargetSet* targets_walked,
                   Err* err) const;

  // GeneratedFile-related methods.
  bool GenerateFile(Err* err) const;

  // Metadata collection methods for GeneratedFile targets.
  struct GeneratedFile {
    Value output_conversion_;
    Value contents_;  // Value::NONE if metadata collection should occur.
    SourceDir rebase_;
    std::vector<std::string> data_keys_;
    std::vector<std::string> walk_keys_;
  };
  const GeneratedFile& generated_file() const;
  GeneratedFile& generated_file();
  bool has_generated_file() const { return generated_file_.get(); }

  const Value& contents() const { return generated_file().contents_; }
  void set_contents(const Value& value) { generated_file().contents_ = value; }
  const Value& output_conversion() const {
    return generated_file().output_conversion_;
  }
  void set_output_conversion(const Value& value) {
    generated_file().output_conversion_ = value;
  }

  const SourceDir& rebase() const { return generated_file().rebase_; }
  void set_rebase(const SourceDir& value) { generated_file().rebase_ = value; }
  const std::vector<std::string>& data_keys() const {
    return generated_file().data_keys_;
  }
  std::vector<std::string>& data_keys() { return generated_file().data_keys_; }
  const std::vector<std::string>& walk_keys() const {
    return generated_file().walk_keys_;
  }
  std::vector<std::string>& walk_keys() { return generated_file().walk_keys_; }

  OutputFile write_runtime_deps_output() const {
    return write_runtime_deps_output_;
  }
  void set_write_runtime_deps_output(const OutputFile& value) {
    write_runtime_deps_output_ = value;
  }

  // Runtime dependencies. These are "file-like things" that can either be
  // directories or files. They do not need to exist, these are just passed as
  // runtime dependencies to external test systems as necessary.
  const std::vector<std::string>& data() const { return data_; }
  std::vector<std::string>& data() { return data_; }

  // Information about the bundle. Only valid for CREATE_BUNDLE target after
  // they have been resolved.
  const BundleData& bundle_data() const;
  BundleData& bundle_data();
  bool has_bundle_data() const { return bundle_data_.get(); }

  // Returns true if targets depending on this one should have an order
  // dependency.
  bool hard_dep() const {
    return output_type_ == ACTION || output_type_ == ACTION_FOREACH ||
           output_type_ == COPY_FILES || output_type_ == CREATE_BUNDLE ||
           output_type_ == BUNDLE_DATA || output_type_ == GENERATED_FILE ||
           builds_swift_module();
  }

  // Returns the iterator range which can be used in range-based for loops
  // to iterate over multiple types of deps in one loop:
  //   for (const auto& pair : target->GetDeps(Target::DEPS_ALL)) ...
  DepsIteratorRange GetDeps(DepsIterationType type) const;

  // Linked private dependencies.
  const LabelTargetVector& private_deps() const { return private_deps_; }
  LabelTargetVector& private_deps() { return private_deps_; }

  // Linked public dependencies.
  const LabelTargetVector& public_deps() const { return public_deps_; }
  LabelTargetVector& public_deps() { return public_deps_; }

  // Non-linked dependencies.
  const LabelTargetVector& data_deps() const { return data_deps_; }
  LabelTargetVector& data_deps() { return data_deps_; }

  // gen_deps only propagate the "should_generate" flag. These dependencies can
  // have cycles so care should be taken if iterating over them recursively.
  const LabelTargetVector& gen_deps() const { return gen_deps_; }
  LabelTargetVector& gen_deps() { return gen_deps_; }

  // List of configs that this class inherits settings from. Once a target is
  // resolved, this will also list all-dependent and public configs.
  const UniqueVector<LabelConfigPair>& configs() const { return configs_; }
  UniqueVector<LabelConfigPair>& configs() { return configs_; }

  // List of configs that all dependencies (direct and indirect) of this
  // target get. These configs are not added to this target. Note that due
  // to the way this is computed, there may be duplicates in this list.
  const UniqueVector<LabelConfigPair>& all_dependent_configs() const {
    return all_dependent_configs_;
  }
  UniqueVector<LabelConfigPair>& all_dependent_configs() {
    return all_dependent_configs_;
  }

  // List of configs that targets depending directly on this one get. These
  // configs are also added to this target.
  const UniqueVector<LabelConfigPair>& public_configs() const {
    return public_configs_;
  }
  UniqueVector<LabelConfigPair>& public_configs() { return public_configs_; }

  // Dependencies that can include files from this target.
  const std::set<Label>& allow_circular_includes_from() const {
    return allow_circular_includes_from_;
  }
  std::set<Label>& allow_circular_includes_from() {
    return allow_circular_includes_from_;
  }

  // Pool option
  const LabelPtrPair<Pool>& pool() const { return pool_; }
  void set_pool(LabelPtrPair<Pool> pool) { pool_ = std::move(pool); }

  // This config represents the configuration set directly on this target.
  ConfigValues& config_values();
  const ConfigValues& config_values() const;
  bool has_config_values() const { return config_values_.get(); }

  ActionValues& action_values();
  const ActionValues& action_values() const;
  bool has_action_values() const { return action_values_.get(); }

  SwiftValues& swift_values();
  const SwiftValues& swift_values() const;
  bool has_swift_values() const { return swift_values_.get(); }

  // Return true if this targets builds a SwiftModule
  bool builds_swift_module() const {
    return IsBinary() && source_types_used().SwiftSourceUsed();
  }

  RustValues& rust_values();
  const RustValues& rust_values() const;
  bool has_rust_values() const { return rust_values_.get(); }

  std::vector<LabelPattern>& friends() { return friends_; }
  const std::vector<LabelPattern>& friends() const { return friends_; }

  std::vector<LabelPattern>& assert_no_deps() { return assert_no_deps_; }
  const std::vector<LabelPattern>& assert_no_deps() const {
    return assert_no_deps_;
  }

  // The toolchain is only known once this target is resolved (all if its
  // dependencies are known). They will be null until then. Generally, this can
  // only be used during target writing.
  const Toolchain* toolchain() const { return toolchain_; }

  // Sets the toolchain. The toolchain must include a tool for this target
  // or the error will be set and the function will return false. Unusually,
  // this function's "err" output is optional since this is commonly used
  // frequently by unit tests which become needlessly verbose.
  bool SetToolchain(const Toolchain* toolchain, Err* err = nullptr);

  // Once this target has been resolved, all outputs from the target will be
  // listed here. This will include things listed in the "outputs" for an
  // action or a copy step, and the output library or executable file(s) from
  // binary targets.
  //
  // It will NOT include stamp files and object files.
  const std::vector<OutputFile>& computed_outputs() const {
    return computed_outputs_;
  }

  // Returns outputs from this target. The link output file is the one that
  // other targets link to when they depend on this target. This will only be
  // valid for libraries and will be empty for all other target types.
  //
  // The dependency output file is the file that should be used to express
  // a dependency on this one. It could be the same as the link output file
  // (this will be the case for static libraries). For shared libraries it
  // could be the same or different than the link output file, depending on the
  // system. For actions this will be the stamp file.
  //
  // These are only known once the target is resolved and will be empty before
  // that. This is a cache of the files to prevent every target that depends on
  // a given library from recomputing the same pattern.
  const OutputFile& link_output_file() const { return link_output_file_; }
  const OutputFile& dependency_output_file() const {
    return dependency_output_file_;
  }

  // The subset of computed_outputs that are considered runtime outputs.
  const std::vector<OutputFile>& runtime_outputs() const {
    return runtime_outputs_;
  }

  // Computes and returns the outputs of this target expressed as SourceFiles.
  //
  // For binary target this depends on the tool for this target so the toolchain
  // must have been loaded beforehand. This will happen asynchronously so
  // calling this on a binary target before the build is complete will produce a
  // race condition.
  //
  // To resolve this, the caller passes in whether the entire build is complete
  // (this is used for the introspection commands which run after everything
  // else).
  //
  // If the build is complete, the toolchain will be used for binary targets to
  // compute the outputs. If the build is not complete, calling this function
  // for binary targets will produce an error.
  //
  // The |loc_for_error| is used to blame a location for any errors produced. It
  // can be empty if there is no range (like this is being called based on the
  // command-line.
  bool GetOutputsAsSourceFiles(const LocationRange& loc_for_error,
                               bool build_complete,
                               std::vector<SourceFile>* outputs,
                               Err* err) const;

  // Computes the set of output files resulting from compiling the given source
  // file.
  //
  // For binary targets, if the file can be compiled and the tool exists, fills
  // the outputs in and writes the tool type to computed_tool_type. If the file
  // is not compilable, returns false.
  //
  // For action_foreach and copy targets, applies the output pattern to the
  // given file name to compute the outputs.
  //
  // For all other target types, just returns the target outputs because such
  // targets conceptually process all of their inputs as one step.
  //
  // The function can succeed with a "NONE" tool type for object files which
  // are just passed to the output. The output will always be overwritten, not
  // appended to.
  bool GetOutputFilesForSource(const SourceFile& source,
                               const char** computed_tool_type,
                               std::vector<OutputFile>* outputs) const;

 private:
  FRIEND_TEST_ALL_PREFIXES(TargetTest, ResolvePrecompiledHeaders);

  // Pulls necessary information from dependencies to this one when all
  // dependencies have been resolved.
  void PullDependentTargetConfigs();
  void PullDependentTargetLibsFrom(const Target* dep, bool is_public);
  void PullDependentTargetLibs();
  void PullRecursiveHardDeps();
  void PullRecursiveBundleData();

  // Fills the link and dependency output files when a target is resolved.
  bool FillOutputFiles(Err* err);

  // Checks precompiled headers from configs and makes sure the resulting
  // values are in config_values_.
  bool ResolvePrecompiledHeaders(Err* err);

  // Validates the given thing when a target is resolved.
  bool CheckVisibility(Err* err) const;
  bool CheckConfigVisibility(Err* err) const;
  bool CheckTestonly(Err* err) const;
  bool CheckAssertNoDeps(Err* err) const;
  void CheckSourcesGenerated() const;
  void CheckSourceGenerated(const SourceFile& source) const;
  bool CheckSourceSetLanguages(Err* err) const;

  OutputType output_type_ = UNKNOWN;
  std::string output_name_;
  bool output_prefix_override_ = false;
  SourceDir output_dir_;
  std::string output_extension_;
  bool output_extension_set_ = false;

  FileList sources_;
  SourceFileTypeSet source_types_used_;
  bool all_headers_public_ = true;
  FileList public_headers_;
  bool check_includes_ = true;
  bool complete_static_lib_ = false;
  std::vector<std::string> data_;
  std::unique_ptr<BundleData> bundle_data_;
  OutputFile write_runtime_deps_output_;

  LabelTargetVector private_deps_;
  LabelTargetVector public_deps_;
  LabelTargetVector data_deps_;
  LabelTargetVector gen_deps_;

  // See getters for more info.
  UniqueVector<LabelConfigPair> configs_;
  UniqueVector<LabelConfigPair> all_dependent_configs_;
  UniqueVector<LabelConfigPair> public_configs_;

  std::set<Label> allow_circular_includes_from_;

  LabelPtrPair<Pool> pool_;

  std::vector<LabelPattern> friends_;
  std::vector<LabelPattern> assert_no_deps_;

  // Used for all binary targets, and for inputs in regular targets. The
  // precompiled header values in this struct will be resolved to the ones to
  // use for this target, if precompiled headers are used.
  std::unique_ptr<ConfigValues> config_values_;

  // Used for action[_foreach] targets.
  std::unique_ptr<ActionValues> action_values_;

  // Used for Rust targets.
  std::unique_ptr<RustValues> rust_values_;

  // User for Swift targets.
  std::unique_ptr<SwiftValues> swift_values_;

  // Toolchain used by this target. Null until target is resolved.
  const Toolchain* toolchain_ = nullptr;

  // Output files. Empty until the target is resolved.
  std::vector<OutputFile> computed_outputs_;
  OutputFile link_output_file_;
  OutputFile dependency_output_file_;
  std::vector<OutputFile> runtime_outputs_;

  std::unique_ptr<Metadata> metadata_;

  // GeneratedFile as metadata collection values.
  std::unique_ptr<GeneratedFile> generated_file_;

  Target(const Target&) = delete;
  Target& operator=(const Target&) = delete;
};

extern const char kExecution_Help[];

#endif  // TOOLS_GN_TARGET_H_
