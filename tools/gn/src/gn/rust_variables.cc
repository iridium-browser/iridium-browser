// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/rust_variables.h"

namespace variables {

// Rust target variables ------------------------------------------------------

const char kRustAliasedDeps[] = "aliased_deps";
const char kRustAliasedDeps_HelpShort[] =
    "aliased_deps: [scope] Set of crate-dependency pairs.";
const char kRustAliasedDeps_Help[] =
    R"(aliased_deps: [scope] Set of crate-dependency pairs.

  Valid for `rust_library` targets and `executable`, `static_library`, and
  `shared_library` targets that contain Rust sources.

  A scope, each key indicating the renamed crate and the corresponding value
  specifying the label of the dependency producing the relevant binary.

  All dependencies listed in this field *must* be listed as deps of the target.

    executable("foo") {
      sources = [ "main.rs" ]
      deps = [ "//bar" ]
    }

  This target would compile the `foo` crate with the following `extern` flag:
  `rustc ...command... --extern bar=<build_out_dir>/obj/bar`

    executable("foo") {
      sources = [ "main.rs" ]
      deps = [ ":bar" ]
      aliased_deps = {
        bar_renamed = ":bar"
      }
    }

  With the addition of `aliased_deps`, above target would instead compile with:
  `rustc ...command... --extern bar_renamed=<build_out_dir>/obj/bar`
)";

const char kRustCrateName[] = "crate_name";
const char kRustCrateName_HelpShort[] =
    "crate_name: [string] The name for the compiled crate.";
const char kRustCrateName_Help[] =
    R"(crate_name: [string] The name for the compiled crate.

  Valid for `rust_library` targets and `executable`, `static_library`,
  `shared_library`, and `source_set` targets that contain Rust sources.

  If crate_name is not set, then this rule will use the target name.
)";

const char kRustCrateType[] = "crate_type";
const char kRustCrateType_HelpShort[] =
    "crate_type: [string] The type of linkage to use on a shared_library.";
const char kRustCrateType_Help[] =
    R"(crate_type: [string] The type of linkage to use on a shared_library.

  Valid for `rust_library` targets and `executable`, `static_library`,
  `shared_library`, and `source_set` targets that contain Rust sources.

  Options for this field are "cdylib", "staticlib", "proc-macro", and "dylib".
  This field sets the `crate-type` attribute for the `rustc` tool on static
  libraries, as well as the appropriate output extension in the
  `rust_output_extension` attribute. Since outputs must be explicit, the `lib`
  crate type (where the Rust compiler produces what it thinks is the
  appropriate library type) is not supported.

  It should be noted that the "dylib" crate type in Rust is unstable in the set
  of symbols it exposes, and most usages today are potentially wrong and will
  be broken in the future.

  Static libraries, rust libraries, and executables have this field set
  automatically.
)";

const char kRustCrateRoot[] = "crate_root";
const char kRustCrateRoot_HelpShort[] =
    "crate_root: [string] The root source file for a binary or library.";
const char kRustCrateRoot_Help[] =
    R"(crate_root: [string] The root source file for a binary or library.

  Valid for `rust_library` targets and `executable`, `static_library`,
  `shared_library`, and `source_set` targets that contain Rust sources.

  This file is usually the `main.rs` or `lib.rs` for binaries and libraries,
  respectively.

  If crate_root is not set, then this rule will look for a lib.rs file (or
  main.rs for executable) or a single file in sources, if sources contains
  only one file.
)";

void InsertRustVariables(VariableInfoMap* info_map) {
  info_map->insert(std::make_pair(
      kRustAliasedDeps,
      VariableInfo(kRustAliasedDeps_HelpShort, kRustAliasedDeps_Help)));
  info_map->insert(std::make_pair(
      kRustCrateName,
      VariableInfo(kRustCrateName_HelpShort, kRustCrateName_Help)));
  info_map->insert(std::make_pair(
      kRustCrateType,
      VariableInfo(kRustCrateType_HelpShort, kRustCrateType_Help)));
  info_map->insert(std::make_pair(
      kRustCrateRoot,
      VariableInfo(kRustCrateRoot_HelpShort, kRustCrateRoot_Help)));
}

}  // namespace variables
