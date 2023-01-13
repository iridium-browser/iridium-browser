// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_TOOL_H_
#define TOOLS_GN_TOOL_H_

#include <string>

#include "base/logging.h"
#include "gn/label.h"
#include "gn/label_ptr.h"
#include "gn/scope.h"
#include "gn/source_file.h"
#include "gn/substitution_list.h"
#include "gn/substitution_pattern.h"

class ParseNode;
class Pool;
class Target;
class Toolchain;

class CTool;
class GeneralTool;
class RustTool;
class BuiltinTool;

// To add a new Tool category, create a subclass implementing SetComplete()
// Add a new category to ToolCategories
// Add a GetAs* function
class Tool {
 public:
  static const char* kToolNone;

  virtual ~Tool();

  // Manual RTTI and required functions ---------------------------------------
  //
  // To implement a new tool category to compile binaries in a different way,
  // inherit this class, implement the following functions, and add the
  // appropriate ToolTypes and RTTI getters. New tools will also need to
  // implement a corresponding class inheriting NinjaBinaryTargetWriter that
  // does the actual rule writing.

  // Initialize tool from a scope. Child classes should override this function
  // and call the parent.
  bool InitTool(Scope* block_scope, Toolchain* toolchain, Err* err);

  // Validate the char* passed to the creation.
  virtual bool ValidateName(const char* name) const = 0;

  // Called when the toolchain is saving this tool, after everything is filled
  // in.
  virtual void SetComplete() = 0;

  // Validate substitutions in this tool.
  virtual bool ValidateSubstitution(const Substitution* sub_type) const = 0;

  // Manual RTTI
  virtual CTool* AsC();
  virtual const CTool* AsC() const;
  virtual GeneralTool* AsGeneral();
  virtual const GeneralTool* AsGeneral() const;
  virtual RustTool* AsRust();
  virtual const RustTool* AsRust() const;
  virtual BuiltinTool* AsBuiltin();
  virtual const BuiltinTool* AsBuiltin() const;

  // Basic information ---------------------------------------------------------

  const ParseNode* defined_from() const { return defined_from_; }
  void set_defined_from(const ParseNode* df) { defined_from_ = df; }

  const char* name() const { return name_; }

  // Getters/setters ----------------------------------------------------------
  //
  // After the tool has had its attributes set, the caller must call
  // SetComplete(), at which point no other changes can be made.

  // Command to run.
  const SubstitutionPattern& command() const { return command_; }
  void set_command(SubstitutionPattern cmd) {
    DCHECK(!complete_);
    command_ = std::move(cmd);
  }

  // Launcher for the command (e.g. goma)
  const std::string& command_launcher() const { return command_launcher_; }
  void set_command_launcher(std::string l) {
    DCHECK(!complete_);
    command_launcher_ = std::move(l);
  }

  // Should include a leading "." if nonempty.
  const std::string& default_output_extension() const {
    return default_output_extension_;
  }
  void set_default_output_extension(std::string ext) {
    DCHECK(!complete_);
    DCHECK(ext.empty() || ext[0] == '.');
    default_output_extension_ = std::move(ext);
  }

  const SubstitutionPattern& default_output_dir() const {
    return default_output_dir_;
  }
  void set_default_output_dir(SubstitutionPattern dir) {
    DCHECK(!complete_);
    default_output_dir_ = std::move(dir);
  }

  // Dependency file (if supported).
  const SubstitutionPattern& depfile() const { return depfile_; }
  void set_depfile(SubstitutionPattern df) {
    DCHECK(!complete_);
    depfile_ = std::move(df);
  }

  const SubstitutionPattern& description() const { return description_; }
  void set_description(SubstitutionPattern desc) {
    DCHECK(!complete_);
    description_ = std::move(desc);
  }

  const std::string& framework_switch() const { return framework_switch_; }
  void set_framework_switch(std::string s) {
    DCHECK(!complete_);
    framework_switch_ = std::move(s);
  }

  const std::string& weak_framework_switch() const {
    return weak_framework_switch_;
  }
  void set_weak_framework_switch(std::string s) {
    DCHECK(!complete_);
    weak_framework_switch_ = std::move(s);
  }

  const std::string& framework_dir_switch() const {
    return framework_dir_switch_;
  }
  void set_framework_dir_switch(std::string s) {
    DCHECK(!complete_);
    framework_dir_switch_ = std::move(s);
  }

  const std::string& lib_switch() const { return lib_switch_; }
  void set_lib_switch(std::string s) {
    DCHECK(!complete_);
    lib_switch_ = std::move(s);
  }

  const std::string& lib_dir_switch() const { return lib_dir_switch_; }
  void set_lib_dir_switch(std::string s) {
    DCHECK(!complete_);
    lib_dir_switch_ = std::move(s);
  }

  const std::string& swiftmodule_switch() const { return swiftmodule_switch_; }
  void set_swiftmodule_switch(std::string s) {
    DCHECK(!complete_);
    swiftmodule_switch_ = std::move(s);
  }

  const std::string& linker_arg() const { return linker_arg_; }
  void set_linker_arg(std::string s) {
    DCHECK(!complete_);
    linker_arg_ = std::move(s);
  }

  const SubstitutionList& outputs() const { return outputs_; }
  void set_outputs(SubstitutionList out) {
    DCHECK(!complete_);
    outputs_ = std::move(out);
  }

  const SubstitutionList& partial_outputs() const { return partial_outputs_; }
  void set_partial_outputs(SubstitutionList partial_out) {
    DCHECK(!complete_);
    partial_outputs_ = std::move(partial_out);
  }

  const SubstitutionList& runtime_outputs() const { return runtime_outputs_; }
  void set_runtime_outputs(SubstitutionList run_out) {
    DCHECK(!complete_);
    runtime_outputs_ = std::move(run_out);
  }

  const std::string& output_prefix() const { return output_prefix_; }
  void set_output_prefix(std::string s) {
    DCHECK(!complete_);
    output_prefix_ = std::move(s);
  }

  bool restat() const { return restat_; }
  void set_restat(bool r) {
    DCHECK(!complete_);
    restat_ = r;
  }

  const SubstitutionPattern& rspfile() const { return rspfile_; }
  void set_rspfile(SubstitutionPattern rsp) {
    DCHECK(!complete_);
    rspfile_ = std::move(rsp);
  }

  const SubstitutionPattern& rspfile_content() const {
    return rspfile_content_;
  }
  void set_rspfile_content(SubstitutionPattern content) {
    DCHECK(!complete_);
    rspfile_content_ = std::move(content);
  }

  const LabelPtrPair<Pool>& pool() const { return pool_; }
  void set_pool(LabelPtrPair<Pool> pool) { pool_ = std::move(pool); }

  // Other functions ----------------------------------------------------------

  // Function for the above override to call to complete the tool.
  void SetToolComplete();

  // Substitutions required by this tool.
  const SubstitutionBits& substitution_bits() const {
    DCHECK(complete_);
    return substitution_bits_;
  }

  // Create a tool based on given features.
  static std::unique_ptr<Tool> CreateTool(const std::string& name);
  static std::unique_ptr<Tool> CreateTool(const ParseNode* function,
                                          const std::string& name,
                                          Scope* scope,
                                          Toolchain* toolchain,
                                          Err* err);

  static const char* GetToolTypeForSourceType(SourceFile::Type type);

  static const char* GetToolTypeForTargetFinalOutput(const Target* target);

 protected:
  explicit Tool(const char* t);

  // Initialization functions -------------------------------------------------
  //
  // Initialization methods used by InitTool(). If successful, will set the
  // field and return true, otherwise will return false. Must be called before
  // SetComplete().
  bool IsPatternInOutputList(const SubstitutionList& output_list,
                             const SubstitutionPattern& pattern) const;
  bool ValidateSubstitutionList(const std::vector<const Substitution*>& list,
                                const Value* origin,
                                Err* err) const;
  bool ValidateOutputs(Err* err) const;
  bool ReadBool(Scope* scope, const char* var, bool* field, Err* err);
  bool ReadString(Scope* scope, const char* var, std::string* field, Err* err);
  bool ReadPattern(Scope* scope,
                   const char* var,
                   SubstitutionPattern* field,
                   Err* err);
  bool ReadPatternList(Scope* scope,
                       const char* var,
                       SubstitutionList* field,
                       Err* err);
  bool ReadLabel(Scope* scope,
                 const char* var,
                 const Label& current_toolchain,
                 LabelPtrPair<Pool>* field,
                 Err* err);
  bool ReadOutputExtension(Scope* scope, Err* err);

  const ParseNode* defined_from_ = nullptr;
  const char* name_ = nullptr;

  SubstitutionPattern command_;
  std::string command_launcher_;
  std::string default_output_extension_;
  SubstitutionPattern default_output_dir_;
  SubstitutionPattern depfile_;
  SubstitutionPattern description_;
  std::string framework_switch_;
  std::string weak_framework_switch_;
  std::string framework_dir_switch_;
  std::string lib_switch_;
  std::string lib_dir_switch_;
  std::string swiftmodule_switch_;
  std::string linker_arg_;
  SubstitutionList outputs_;
  SubstitutionList partial_outputs_;
  SubstitutionList runtime_outputs_;
  std::string output_prefix_;
  bool restat_ = false;
  SubstitutionPattern rspfile_;
  SubstitutionPattern rspfile_content_;
  LabelPtrPair<Pool> pool_;

  bool complete_ = false;

  SubstitutionBits substitution_bits_;

  Tool(const Tool&) = delete;
  Tool& operator=(const Tool&) = delete;
};

#endif  // TOOLS_GN_TOOL_H_
