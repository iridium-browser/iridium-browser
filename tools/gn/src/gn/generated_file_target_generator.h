// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_GENERATED_FILE_TARGET_GENERATOR_H_
#define TOOLS_GN_GENERATED_FILE_TARGET_GENERATOR_H_

#include <string_view>

#include "gn/target.h"
#include "gn/target_generator.h"

// Collects and writes specified data.
class GeneratedFileTargetGenerator : public TargetGenerator {
 public:
  GeneratedFileTargetGenerator(Target* target,
                               Scope* scope,
                               const FunctionCallNode* function_call,
                               Target::OutputType type,
                               Err* err);
  ~GeneratedFileTargetGenerator() override;

 protected:
  void DoRun() override;

 private:
  bool FillGeneratedFileOutput();
  bool FillOutputConversion();
  bool FillContents();
  bool FillDataKeys();
  bool FillWalkKeys();
  bool FillRebase();

  // Returns false if `contents` is defined (i.e. if this target was provided
  // with explicit contents to write). Returns false otherwise, indicating that
  // it is okay to set metadata collection variables on this target.
  //
  // Should be called before FillContents().
  bool IsMetadataCollectionTarget(std::string_view variable,
                                  const ParseNode* origin);

  bool contents_defined_ = false;
  bool data_keys_defined_ = false;

  Target::OutputType output_type_;

  GeneratedFileTargetGenerator(const GeneratedFileTargetGenerator&) = delete;
  GeneratedFileTargetGenerator& operator=(const GeneratedFileTargetGenerator&) =
      delete;
};

#endif  // TOOLS_GN_GENERATED_FILE_TARGET_GENERATOR_H_
