// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_RUST_TARGET_VALUES_H_
#define TOOLS_GN_RUST_TARGET_VALUES_H_

#include <map>

#include "base/containers/flat_map.h"
#include "gn/inherited_libraries.h"
#include "gn/label.h"
#include "gn/source_file.h"

// Holds the values (outputs, args, script name, etc.) for either an action or
// an action_foreach target.
class RustValues {
 public:
  RustValues();
  ~RustValues();

  // Library crate types.
  //
  // The default value CRATE_AUTO means the type should be deduced from the
  // target type (see InferredCrateType() below).
  //
  // Shared library crate types must be specified explicitly, all other target
  // types can be deduced.
  enum CrateType {
    CRATE_AUTO = 0,
    CRATE_BIN,
    CRATE_CDYLIB,
    CRATE_DYLIB,
    CRATE_PROC_MACRO,
    CRATE_RLIB,
    CRATE_STATICLIB,
  };

  // Name of this crate.
  std::string& crate_name() { return crate_name_; }
  const std::string& crate_name() const { return crate_name_; }

  // Main source file for this crate.
  const SourceFile& crate_root() const { return crate_root_; }
  void set_crate_root(SourceFile& s) { crate_root_ = s; }

  // Crate type for compilation.
  CrateType crate_type() const { return crate_type_; }
  void set_crate_type(CrateType s) { crate_type_ = s; }

  // Same as crate_type(), except attempt to resolve CRATE_AUTO based on the
  // target type.
  //
  // Dylib and cdylib targets should call set_crate_type(CRATE_[C]DYLIB)
  // explicitly to resolve ambiguity. For shared libraries, this assumes
  // CRATE_DYLIB by default.
  //
  // For unsupported target types and targets without Rust sources,
  // returns CRATE_AUTO.
  static CrateType InferredCrateType(const Target* target);

  // Returns whether this target is a Rust rlib, dylib, or proc macro.
  //
  // Notably, this does not include staticlib or cdylib targets that have Rust
  // source, because they look like native libraries to the Rust compiler.
  //
  // It does include proc_macro targets, which are sometimes a special case.
  // (TODO: Should it?)
  static bool IsRustLibrary(const Target* target);

  // Any renamed dependencies for the `extern` flags.
  const std::map<Label, std::string>& aliased_deps() const {
    return aliased_deps_;
  }
  std::map<Label, std::string>& aliased_deps() { return aliased_deps_; }

 private:
  std::string crate_name_;
  SourceFile crate_root_;
  CrateType crate_type_ = CRATE_AUTO;
  std::map<Label, std::string> aliased_deps_;

  RustValues(const RustValues&) = delete;
  RustValues& operator=(const RustValues&) = delete;
};

#endif  // TOOLS_GN_RUST_TARGET_VALUES_H_
