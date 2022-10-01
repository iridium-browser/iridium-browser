// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_CONFIG_VALUES_GENERATOR_H_
#define TOOLS_GN_CONFIG_VALUES_GENERATOR_H_

#include "gn/source_dir.h"

class ConfigValues;
class Err;
class Scope;

// This class fills in the config values from a given scope. It's shared
// between the "config" function call and all the different binary target types
// (shared library, static library, etc.) since all of these support the
// various flags stored in the ConfigValues class.
class ConfigValuesGenerator {
 public:
  ConfigValuesGenerator(ConfigValues* dest_values,
                        Scope* scope,
                        const SourceDir& input_dir,
                        Err* err);
  ~ConfigValuesGenerator();

  // Sets the error passed to the constructor on failure.
  void Run();

 private:
  ConfigValues* config_values_;
  Scope* scope_;
  const SourceDir input_dir_;
  Err* err_;

  ConfigValuesGenerator(const ConfigValuesGenerator&) = delete;
  ConfigValuesGenerator& operator=(const ConfigValuesGenerator&) = delete;
};

// For using in documentation for functions which use this.
#define CONFIG_VALUES_VARS_HELP                                        \
  "  Flags: asmflags, cflags, cflags_c, cflags_cc, cflags_objc,\n"     \
  "         cflags_objcc, defines, include_dirs, inputs, ldflags,\n"   \
  "         lib_dirs, libs, precompiled_header, precompiled_source,\n" \
  "         rustenv, rustflags, swiftflags, testonly\n"

#endif  // TOOLS_GN_CONFIG_VALUES_GENERATOR_H_
