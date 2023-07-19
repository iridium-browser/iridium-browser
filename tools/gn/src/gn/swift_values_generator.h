// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_SWIFT_TARGET_VALUES_GENERATOR_H_
#define TOOLS_GN_SWIFT_TARGET_VALUES_GENERATOR_H_

#include <string>

class Err;
class FunctionCallNode;
class Scope;
class Target;

class SwiftValuesGenerator {
 public:
  SwiftValuesGenerator(Target* target, Scope* scope, Err* err);
  ~SwiftValuesGenerator();

  SwiftValuesGenerator(const SwiftValuesGenerator&) = delete;
  SwiftValuesGenerator& operator=(const SwiftValuesGenerator&) = delete;

  void Run();

 private:
  bool FillBridgeHeader();
  bool FillModuleName();

  Target* target_;
  Scope* scope_;
  Err* err_;
};

#endif  // TOOLS_GN_SWIFT_TARGET_VALUES_GENERATOR_H_
