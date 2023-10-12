// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_CREATE_BUNDLE_TARGET_GENERATOR_H_
#define TOOLS_GN_CREATE_BUNDLE_TARGET_GENERATOR_H_

#include <string_view>

#include "gn/target_generator.h"

class SourceDir;

// Populates a Target with the values from a create_bundle rule.
class CreateBundleTargetGenerator : public TargetGenerator {
 public:
  CreateBundleTargetGenerator(Target* target,
                              Scope* scope,
                              const FunctionCallNode* function_call,
                              Err* err);
  ~CreateBundleTargetGenerator() override;

 protected:
  void DoRun() override;

 private:
  bool FillBundleDir(const SourceDir& bundle_root_dir,
                     std::string_view name,
                     SourceDir* bundle_dir);

  bool FillXcodeExtraAttributes();

  bool FillProductType();
  bool FillPartialInfoPlist();
  bool FillXcodeTestApplicationName();

  bool FillCodeSigningScript();
  bool FillCodeSigningSources();
  bool FillCodeSigningOutputs();
  bool FillCodeSigningArgs();
  bool FillBundleDepsFilter();
  bool FillXcassetCompilerFlags();
  bool FillTransparent();

  CreateBundleTargetGenerator(const CreateBundleTargetGenerator&) = delete;
  CreateBundleTargetGenerator& operator=(const CreateBundleTargetGenerator&) =
      delete;
};

#endif  // TOOLS_GN_CREATE_BUNDLE_TARGET_GENERATOR_H_
