// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_ACTION_TARGET_GENERATOR_H_
#define TOOLS_GN_ACTION_TARGET_GENERATOR_H_

#include "gn/target.h"
#include "gn/target_generator.h"

// Populates a Target with the values from an action[_foreach] rule.
class ActionTargetGenerator : public TargetGenerator {
 public:
  ActionTargetGenerator(Target* target,
                        Scope* scope,
                        const FunctionCallNode* function_call,
                        Target::OutputType type,
                        Err* err);
  ~ActionTargetGenerator() override;

 protected:
  void DoRun() override;

 private:
  bool FillScript();
  bool FillScriptArgs();
  bool FillResponseFileContents();
  bool FillDepfile();
  bool FillMnemonic();
  bool FillPool();
  bool FillInputs();

  // Checks for errors in the outputs variable.
  bool CheckOutputs();

  Target::OutputType output_type_;

  ActionTargetGenerator(const ActionTargetGenerator&) = delete;
  ActionTargetGenerator& operator=(const ActionTargetGenerator&) = delete;
};

#endif  // TOOLS_GN_ACTION_TARGET_GENERATOR_H_
