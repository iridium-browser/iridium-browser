// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_RUST_VARIABLES_H_
#define TOOLS_GN_RUST_VARIABLES_H_

#include "gn/variables.h"

namespace variables {

// Rust target vars ------------------------------------------------------

extern const char kRustAliasedDeps[];
extern const char kRustAliasedDeps_HelpShort[];
extern const char kRustAliasedDeps_Help[];

extern const char kRustCrateName[];
extern const char kRustCrateName_HelpShort[];
extern const char kRustCrateName_Help[];

extern const char kRustCrateType[];
extern const char kRustCrateType_HelpShort[];
extern const char kRustCrateType_Help[];

extern const char kRustCrateRoot[];
extern const char kRustCrateRoot_HelpShort[];
extern const char kRustCrateRoot_Help[];

extern const char kRustEdition[];
extern const char kRustEdition_HelpShort[];
extern const char kRustEdition_Help[];

void InsertRustVariables(VariableInfoMap* info_map);

}  // namespace variables

#endif  // TOOLS_GN_RUST_VARIABLES_H_