// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_SUBSTITUTION_TYPE_H_
#define TOOLS_GN_SUBSTITUTION_TYPE_H_

#include <vector>

#include "base/containers/flat_set.h"

class Err;
class ParseNode;

// Each pair here represents the string representation of the substitution in GN
// and in Ninja.
struct Substitution {
  const char* name;
  const char* ninja_name;
  Substitution(const Substitution&) = delete;
  Substitution& operator=(const Substitution&) = delete;
};

using SubstitutionTypes = const std::vector<const Substitution*>;

// All possible substitutions, organized into logical sets.
extern const std::vector<SubstitutionTypes*> AllSubstitutions;

// The set of substitutions available to all tools.
extern const SubstitutionTypes GeneralSubstitutions;

// Types of substitutions.
extern const Substitution SubstitutionLiteral;

// Valid for all tools. These depend on the target and
// do not vary on a per-file basis.
extern const Substitution SubstitutionOutput;
extern const Substitution SubstitutionLabel;
extern const Substitution SubstitutionLabelName;
extern const Substitution SubstitutionLabelNoToolchain;
extern const Substitution SubstitutionRootGenDir;
extern const Substitution SubstitutionRootOutDir;
extern const Substitution SubstitutionOutputDir;
extern const Substitution SubstitutionOutputExtension;
extern const Substitution SubstitutionTargetGenDir;
extern const Substitution SubstitutionTargetOutDir;
extern const Substitution SubstitutionTargetOutputName;

// Valid for all compiler tools.
extern const Substitution SubstitutionSource;
extern const Substitution SubstitutionSourceNamePart;
extern const Substitution SubstitutionSourceFilePart;
extern const Substitution SubstitutionSourceDir;
extern const Substitution SubstitutionSourceRootRelativeDir;
extern const Substitution SubstitutionSourceGenDir;
extern const Substitution SubstitutionSourceOutDir;
extern const Substitution SubstitutionSourceTargetRelative;

// Valid for bundle_data targets.
extern const Substitution SubstitutionBundleRootDir;
extern const Substitution SubstitutionBundleContentsDir;
extern const Substitution SubstitutionBundleResourcesDir;
extern const Substitution SubstitutionBundleExecutableDir;

// Valid for compile_xcassets tool.
extern const Substitution SubstitutionBundleProductType;
extern const Substitution SubstitutionBundlePartialInfoPlist;
extern const Substitution SubstitutionXcassetsCompilerFlags;

// Used only for the args of actions.
extern const Substitution SubstitutionRspFileName;

// A wrapper around an array if flags indicating whether a given substitution
// type is required in some context. By convention, the LITERAL type bit is
// not set.
struct SubstitutionBits {
  SubstitutionBits();

  // Merges any bits set in the given "other" to this one. This object will
  // then be the union of all bits in the two lists.
  void MergeFrom(const SubstitutionBits& other);

  // Converts the substitution type set to a vector of the types listed. Does
  // not include SubstitutionLiteral.
  void FillVector(std::vector<const Substitution*>* vect) const;

  // This set depends on global uniqueness of pointers, and so all points in
  // this set should be the Substitution* constants.
  base::flat_set<const Substitution*> used;
};

// Returns true if the given substitution pattern references the output
// directory. This is used to check strings that begin with a substitution to
// verify that they produce a file in the output directory.
bool SubstitutionIsInOutputDir(const Substitution* type);

// Returns true if the given substitution pattern references the bundle
// directory. This is used to check strings that begin with a substitution to
// verify that they produce a file in the bundle directory.
bool SubstitutionIsInBundleDir(const Substitution* type);

// Returns true if the given substitution is valid for the named purpose.
bool IsValidBundleDataSubstitution(const Substitution* type);
bool IsValidSourceSubstitution(const Substitution* type);
bool IsValidScriptArgsSubstitution(const Substitution* type);

// Both compiler and linker tools.
bool IsValidToolSubstitution(const Substitution* type);
bool IsValidCopySubstitution(const Substitution* type);
bool IsValidCompileXCassetsSubstitution(const Substitution* type);

// Validates that each substitution type in the vector passes the given
// is_valid_subst predicate. Returns true on success. On failure, fills in the
// error object with an appropriate message and returns false.
bool EnsureValidSubstitutions(const std::vector<const Substitution*>& types,
                              bool (*is_valid_subst)(const Substitution*),
                              const ParseNode* origin,
                              Err* err);

#endif  // TOOLS_GN_SUBSTITUTION_TYPE_H_
