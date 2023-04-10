// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_GROUP_TARGET_GENERATOR_H_
#define TOOLS_GN_GROUP_TARGET_GENERATOR_H_

#include "gn/target_generator.h"

// Populates a Target with the values for a group rule.
class GroupTargetGenerator : public TargetGenerator {
 public:
  GroupTargetGenerator(Target* target,
                       Scope* scope,
                       const FunctionCallNode* function_call,
                       Err* err);
  ~GroupTargetGenerator() override;

 protected:
  void DoRun() override;

 private:
  GroupTargetGenerator(const GroupTargetGenerator&) = delete;
  GroupTargetGenerator& operator=(const GroupTargetGenerator&) = delete;
};

#endif  // TOOLS_GN_GROUP_TARGET_GENERATOR_H_
