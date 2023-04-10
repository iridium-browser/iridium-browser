// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_RUST_VALUES_GENERATOR_H_
#define TOOLS_GN_RUST_VALUES_GENERATOR_H_

#include "gn/target.h"

class FunctionCallNode;

// Collects and writes specified data.
class RustValuesGenerator {
 public:
  RustValuesGenerator(Target* target,
                      Scope* scope,
                      const FunctionCallNode* function_call,
                      Err* err);
  ~RustValuesGenerator();

  void Run();

 private:
  bool FillCrateName();
  bool FillCrateRoot();
  bool FillCrateType();
  bool FillEdition();
  bool FillAliasedDeps();

  Target* target_;
  Scope* scope_;
  const FunctionCallNode* function_call_;
  Err* err_;

  RustValuesGenerator(const RustValuesGenerator&) = delete;
  RustValuesGenerator& operator=(const RustValuesGenerator&) = delete;
};

#endif  // TOOLS_GN_RUST_VALUES_GENERATOR_H_
