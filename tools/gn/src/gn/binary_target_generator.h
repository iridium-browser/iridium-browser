// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_BINARY_TARGET_GENERATOR_H_
#define TOOLS_GN_BINARY_TARGET_GENERATOR_H_

#include "gn/target.h"
#include "gn/target_generator.h"

// Populates a Target with the values from a binary rule (executable, shared
// library, or static library).
class BinaryTargetGenerator : public TargetGenerator {
 public:
  BinaryTargetGenerator(Target* target,
                        Scope* scope,
                        const FunctionCallNode* function_call,
                        Target::OutputType type,
                        Err* err);
  ~BinaryTargetGenerator() override;

 protected:
  void DoRun() override;
  bool FillSources() override;

 private:
  bool FillCompleteStaticLib();
  bool FillFriends();
  bool FillOutputName();
  bool FillOutputPrefixOverride();
  bool FillOutputDir();
  bool FillAllowCircularIncludesFrom();
  bool FillPool();
  bool ValidateSources();

  Target::OutputType output_type_;

  BinaryTargetGenerator(const BinaryTargetGenerator&) = delete;
  BinaryTargetGenerator& operator=(const BinaryTargetGenerator&) = delete;
};

#endif  // TOOLS_GN_BINARY_TARGET_GENERATOR_H_
