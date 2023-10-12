// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_RUST_PROJECT_WRITER_HELPERS_H_
#define TOOLS_GN_RUST_PROJECT_WRITER_HELPERS_H_

#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "base/containers/flat_map.h"
#include "build_settings.h"
#include "gn/source_file.h"
#include "gn/target.h"

// These are internal types and helper functions for RustProjectWriter that have
// been extracted for easier testability.

// Crate Index in the generated file
using CrateIndex = size_t;

using ConfigList = std::vector<std::string>;
using Dependency = std::pair<CrateIndex, std::string>;
using DependencyList = std::vector<Dependency>;

// This class represents a crate to be serialized out as part of the
// rust-project.json file.  This is used to separate the generating
// of the data that needs to be in the file, from the file itself.
class Crate {
 public:
  Crate(SourceFile root,
        std::optional<OutputFile> gen_dir,
        CrateIndex index,
        std::string label,
        std::string edition)
      : root_(root),
        gen_dir_(gen_dir),
        index_(index),
        label_(label),
        edition_(edition) {}

  ~Crate() = default;

  // Add a config item to the crate.
  void AddConfigItem(std::string cfg_item) { configs_.push_back(cfg_item); }

  // Add a key-value environment variable pair used when building this crate.
  void AddRustenv(std::string key, std::string value) {
    rustenv_.emplace(key, value);
  }

  // Add another crate as a dependency of this one.
  void AddDependency(CrateIndex index, std::string name) {
    deps_.push_back(std::make_pair(index, name));
  }

  // Set the compiler arguments used to invoke the compilation of this crate
  void SetCompilerArgs(std::vector<std::string> args) { compiler_args_ = args; }

  // Set the compiler target ("e.g. x86_64-linux-kernel")
  void SetCompilerTarget(std::string target) { compiler_target_ = target; }

  // Set that this is a proc macro with the path to the output .so/dylib/dll
  void SetIsProcMacro(OutputFile proc_macro_dynamic_library) {
    proc_macro_dynamic_library_ = proc_macro_dynamic_library;
  }

  // Returns the root file for the crate.
  SourceFile& root() { return root_; }

  // Returns the root file for the crate.
  std::optional<OutputFile>& gen_dir() { return gen_dir_; }

  // Returns the crate index.
  CrateIndex index() { return index_; }

  // Returns the displayable crate label.
  const std::string& label() { return label_; }

  // Returns the Rust Edition this crate uses.
  const std::string& edition() { return edition_; }

  // Return the set of config items for this crate.
  ConfigList& configs() { return configs_; }

  // Return the set of dependencies for this crate.
  DependencyList& dependencies() { return deps_; }

  // Return the compiler arguments used to invoke the compilation of this crate
  const std::vector<std::string>& CompilerArgs() { return compiler_args_; }

  // Return the compiler target "triple" from the compiler args
  const std::optional<std::string>& CompilerTarget() {
    return compiler_target_;
  }

  // Returns whether this crate builds a proc macro .so
  const std::optional<OutputFile>& proc_macro_path() {
    return proc_macro_dynamic_library_;
  }

  // Returns environment variables applied to this, which may be necessary
  // for correct functioning of environment variables
  const base::flat_map<std::string, std::string>& rustenv() { return rustenv_; }

 private:
  SourceFile root_;
  std::optional<OutputFile> gen_dir_;
  CrateIndex index_;
  std::string label_;
  std::string edition_;
  ConfigList configs_;
  DependencyList deps_;
  std::optional<std::string> compiler_target_;
  std::vector<std::string> compiler_args_;
  std::optional<OutputFile> proc_macro_dynamic_library_;
  base::flat_map<std::string, std::string> rustenv_;
};

using CrateList = std::vector<Crate>;

// Write the entire rust-project.json file contents into the given stream, based
// on the the given crates list.
void WriteCrates(const BuildSettings* build_settings,
                 CrateList& crate_list,
                 std::optional<std::string>& sysroot,
                 std::ostream& rust_project);

// Assemble the compiler arguments for the given GN Target.
std::vector<std::string> ExtractCompilerArgs(const Target* target);

// Find the value of an argument that's passed to the compiler as two
// consecutive strings in the list of arguments:  ["arg", "value"]
std::optional<std::string> FindArgValue(const char* arg,
                                        const std::vector<std::string>& args);

// Find the first argument that matches the prefix, returning the value after
// the prefix.  e.g. ˝--arg=value", is returned as "value" if the prefix
// "--arg=" is used.
std::optional<std::string> FindArgValueAfterPrefix(
    const std::string& prefix,
    const std::vector<std::string>& args);

// Find all arguments that match the given prefix, returning the value after
// the prefix for each one.  e.g. "--cfg=value" is returned as "value" if the
// prefix "--cfg=" is used.
std::vector<std::string> FindAllArgValuesAfterPrefix(
    const std::string& prefix,
    const std::vector<std::string>& args);


#endif  // TOOLS_GN_RUST_PROJECT_WRITER_HELPERS_H_
