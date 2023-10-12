// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_C_TOOL_H_
#define TOOLS_GN_C_TOOL_H_

#include <string>

#include "base/logging.h"
#include "gn/label.h"
#include "gn/label_ptr.h"
#include "gn/scope.h"
#include "gn/substitution_list.h"
#include "gn/substitution_pattern.h"
#include "gn/tool.h"
#include "gn/toolchain.h"

class CTool : public Tool {
 public:
  // C compiler tools
  static const char* kCToolCc;
  static const char* kCToolCxx;
  static const char* kCToolCxxModule;
  static const char* kCToolObjC;
  static const char* kCToolObjCxx;
  static const char* kCToolRc;
  static const char* kCToolAsm;
  static const char* kCToolSwift;

  // C linker tools
  static const char* kCToolAlink;
  static const char* kCToolSolink;
  static const char* kCToolSolinkModule;
  static const char* kCToolLink;

  enum DepsFormat { DEPS_GCC = 0, DEPS_MSVC = 1 };

  enum PrecompiledHeaderType { PCH_NONE = 0, PCH_GCC = 1, PCH_MSVC = 2 };

  CTool(const char* n);
  ~CTool();

  // Manual RTTI and required functions ---------------------------------------

  bool InitTool(Scope* block_scope, Toolchain* toolchain, Err* err);
  bool ValidateName(const char* name) const override;
  void SetComplete() override;
  bool ValidateSubstitution(const Substitution* sub_type) const override;

  CTool* AsC() override;
  const CTool* AsC() const override;

  // Getters/setters ----------------------------------------------------------
  //
  // After the tool has had its attributes set, the caller must call
  // SetComplete(), at which point no other changes can be made.

  DepsFormat depsformat() const { return depsformat_; }
  void set_depsformat(DepsFormat f) {
    DCHECK(!complete_);
    depsformat_ = f;
  }

  PrecompiledHeaderType precompiled_header_type() const {
    return precompiled_header_type_;
  }
  void set_precompiled_header_type(PrecompiledHeaderType pch_type) {
    DCHECK(!complete_);
    precompiled_header_type_ = pch_type;
  }

  // Should match files in the outputs() if nonempty.
  const SubstitutionPattern& link_output() const { return link_output_; }
  void set_link_output(SubstitutionPattern link_out) {
    DCHECK(!complete_);
    link_output_ = std::move(link_out);
  }

  const SubstitutionPattern& depend_output() const { return depend_output_; }
  void set_depend_output(SubstitutionPattern dep_out) {
    DCHECK(!complete_);
    depend_output_ = std::move(dep_out);
  }

  // Other functions ----------------------------------------------------------

  // Returns true if this tool has separate outputs for dependency tracking
  // and linking.
  bool has_separate_solink_files() const {
    return !link_output_.empty() || !depend_output_.empty();
  }

 private:
  // Initialization functions -------------------------------------------------
  //
  // Initialization methods used by InitTool(). If successful, will set the
  // field and return true, otherwise will return false. Must be called before
  // SetComplete().
  bool ValidateOutputSubstitution(const Substitution* sub_type) const;
  bool ValidateRuntimeOutputs(Err* err);
  // Validates either link_output or depend_output. To generalize to either,
  // pass
  // the associated pattern, and the variable name that should appear in error
  // messages.
  bool ValidateLinkAndDependOutput(const SubstitutionPattern& pattern,
                                   const char* variable_name,
                                   Err* err);
  bool ReadOutputsPatternList(Scope* scope,
                              const char* var,
                              bool required,
                              SubstitutionList* field,
                              Err* err);
  bool ReadPrecompiledHeaderType(Scope* scope, Err* err);
  bool ReadDepsFormat(Scope* scope, Err* err);

  DepsFormat depsformat_;
  PrecompiledHeaderType precompiled_header_type_;
  SubstitutionPattern link_output_;
  SubstitutionPattern depend_output_;

  CTool(const CTool&) = delete;
  CTool& operator=(const CTool&) = delete;
};

#endif  // TOOLS_GN_C_TOOL_H_
