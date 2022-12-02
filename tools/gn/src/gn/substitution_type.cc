// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/substitution_type.h"

#include <stddef.h>
#include <stdlib.h>

#include "gn/c_substitution_type.h"
#include "gn/err.h"
#include "gn/rust_substitution_type.h"

const std::vector<SubstitutionTypes*> AllSubstitutions = {
    &GeneralSubstitutions, &CSubstitutions, &RustSubstitutions};

const SubstitutionTypes GeneralSubstitutions = {
    &SubstitutionLiteral,

    &SubstitutionOutput,
    &SubstitutionLabel,
    &SubstitutionLabelName,
    &SubstitutionLabelNoToolchain,
    &SubstitutionRootGenDir,
    &SubstitutionRootOutDir,
    &SubstitutionOutputDir,
    &SubstitutionOutputExtension,
    &SubstitutionTargetGenDir,
    &SubstitutionTargetOutDir,
    &SubstitutionTargetOutputName,

    &SubstitutionSource,
    &SubstitutionSourceNamePart,
    &SubstitutionSourceFilePart,
    &SubstitutionSourceDir,
    &SubstitutionSourceRootRelativeDir,
    &SubstitutionSourceGenDir,
    &SubstitutionSourceOutDir,
    &SubstitutionSourceTargetRelative,

    &SubstitutionBundleRootDir,
    &SubstitutionBundleContentsDir,
    &SubstitutionBundleResourcesDir,
    &SubstitutionBundleExecutableDir,

    &SubstitutionBundleProductType,
    &SubstitutionBundlePartialInfoPlist,
    &SubstitutionXcassetsCompilerFlags,

    &SubstitutionRspFileName,
};

const Substitution SubstitutionLiteral = {"<<literal>>", nullptr};

const Substitution SubstitutionSource = {"{{source}}", "in"};
const Substitution SubstitutionOutput = {"{{output}}", "out"};

const Substitution SubstitutionSourceNamePart = {"{{source_name_part}}",
                                                 "source_name_part"};
const Substitution SubstitutionSourceFilePart = {"{{source_file_part}}",
                                                 "source_file_part"};
const Substitution SubstitutionSourceDir = {"{{source_dir}}", "source_dir"};
const Substitution SubstitutionSourceRootRelativeDir = {
    "{{source_root_relative_dir}}", "source_root_relative_dir"};
const Substitution SubstitutionSourceGenDir = {"{{source_gen_dir}}",
                                               "source_gen_dir"};
const Substitution SubstitutionSourceOutDir = {"{{source_out_dir}}",
                                               "source_out_dir"};
const Substitution SubstitutionSourceTargetRelative = {
    "{{source_target_relative}}", "source_target_relative"};

// Valid for all compiler and linker tools. These depend on the target and
// do not vary on a per-file basis.
const Substitution SubstitutionLabel = {"{{label}}", "label"};
const Substitution SubstitutionLabelName = {"{{label_name}}", "label_name"};
const Substitution SubstitutionLabelNoToolchain = {"{{label_no_toolchain}}",
                                                   "label_no_toolchain"};
const Substitution SubstitutionRootGenDir = {"{{root_gen_dir}}",
                                             "root_gen_dir"};
const Substitution SubstitutionRootOutDir = {"{{root_out_dir}}",
                                             "root_out_dir"};
const Substitution SubstitutionOutputDir = {"{{output_dir}}", "output_dir"};
const Substitution SubstitutionOutputExtension = {"{{output_extension}}",
                                                  "output_extension"};
const Substitution SubstitutionTargetGenDir = {"{{target_gen_dir}}",
                                               "target_gen_dir"};
const Substitution SubstitutionTargetOutDir = {"{{target_out_dir}}",
                                               "target_out_dir"};
const Substitution SubstitutionTargetOutputName = {"{{target_output_name}}",
                                                   "target_output_name"};

// Valid for bundle_data targets.
const Substitution SubstitutionBundleRootDir = {"{{bundle_root_dir}}",
                                                "bundle_root_dir"};
const Substitution SubstitutionBundleContentsDir = {"{{bundle_contents_dir}}",
                                                    "bundle_contents_dir"};
const Substitution SubstitutionBundleResourcesDir = {"{{bundle_resources_dir}}",
                                                     "bundle_resources_dir"};
const Substitution SubstitutionBundleExecutableDir = {
    "{{bundle_executable_dir}}", "bundle_executable_dir"};

// Valid for compile_xcassets tool.
const Substitution SubstitutionBundleProductType = {"{{bundle_product_type}}",
                                                    "product_type"};
const Substitution SubstitutionBundlePartialInfoPlist = {
    "{{bundle_partial_info_plist}}", "partial_info_plist"};
const Substitution SubstitutionXcassetsCompilerFlags = {
    "{{xcasset_compiler_flags}}", "xcasset_compiler_flags"};

// Used only for the args of actions.
const Substitution SubstitutionRspFileName = {"{{response_file_name}}",
                                              "rspfile"};

SubstitutionBits::SubstitutionBits() = default;

void SubstitutionBits::MergeFrom(const SubstitutionBits& other) {
  for (const Substitution* s : other.used)
    used.insert(s);
}

void SubstitutionBits::FillVector(
    std::vector<const Substitution*>* vect) const {
  for (const Substitution* s : used) {
    vect->push_back(s);
  }
}

bool SubstitutionIsInOutputDir(const Substitution* type) {
  return type == &SubstitutionSourceGenDir ||
         type == &SubstitutionSourceOutDir || type == &SubstitutionRootGenDir ||
         type == &SubstitutionRootOutDir || type == &SubstitutionTargetGenDir ||
         type == &SubstitutionTargetOutDir;
}

bool SubstitutionIsInBundleDir(const Substitution* type) {
  return type == &SubstitutionBundleRootDir ||
         type == &SubstitutionBundleContentsDir ||
         type == &SubstitutionBundleResourcesDir ||
         type == &SubstitutionBundleExecutableDir;
}

bool IsValidBundleDataSubstitution(const Substitution* type) {
  return type == &SubstitutionLiteral ||
         type == &SubstitutionSourceTargetRelative ||
         type == &SubstitutionSourceNamePart ||
         type == &SubstitutionSourceFilePart ||
         type == &SubstitutionSourceRootRelativeDir ||
         type == &SubstitutionBundleRootDir ||
         type == &SubstitutionBundleContentsDir ||
         type == &SubstitutionBundleResourcesDir ||
         type == &SubstitutionBundleExecutableDir;
}

bool IsValidSourceSubstitution(const Substitution* type) {
  return type == &SubstitutionLiteral || type == &SubstitutionSource ||
         type == &SubstitutionSourceNamePart ||
         type == &SubstitutionSourceFilePart ||
         type == &SubstitutionSourceDir ||
         type == &SubstitutionSourceRootRelativeDir ||
         type == &SubstitutionSourceGenDir ||
         type == &SubstitutionSourceOutDir ||
         type == &SubstitutionSourceTargetRelative;
}

bool IsValidScriptArgsSubstitution(const Substitution* type) {
  return IsValidSourceSubstitution(type) || type == &SubstitutionRspFileName ||
         IsValidCompilerScriptArgsSubstitution(type) ||
         IsValidRustScriptArgsSubstitution(type);
}

bool IsValidToolSubstitution(const Substitution* type) {
  return type == &SubstitutionLiteral || type == &SubstitutionOutput ||
         type == &SubstitutionLabel || type == &SubstitutionLabelName ||
         type == &SubstitutionLabelNoToolchain ||
         type == &SubstitutionRootGenDir || type == &SubstitutionRootOutDir ||
         type == &SubstitutionTargetGenDir ||
         type == &SubstitutionTargetOutDir ||
         type == &SubstitutionTargetOutputName;
}

bool IsValidCopySubstitution(const Substitution* type) {
  return IsValidToolSubstitution(type) || type == &SubstitutionSource;
}

bool IsValidCompileXCassetsSubstitution(const Substitution* type) {
  return IsValidToolSubstitution(type) || type == &CSubstitutionLinkerInputs ||
         type == &SubstitutionBundleProductType ||
         type == &SubstitutionBundlePartialInfoPlist ||
         type == &SubstitutionXcassetsCompilerFlags;
}

bool EnsureValidSubstitutions(const std::vector<const Substitution*>& types,
                              bool (*is_valid_subst)(const Substitution*),
                              const ParseNode* origin,
                              Err* err) {
  for (const Substitution* type : types) {
    if (!is_valid_subst(type)) {
      *err = Err(origin, "Invalid substitution type.",
                 "The substitution " + std::string(type->name) +
                     " isn't valid for something\n"
                     "operating on a source file such as this.");
      return false;
    }
  }
  return true;
}
