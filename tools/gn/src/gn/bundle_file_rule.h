// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_BUNDLE_FILE_RULE_H_
#define TOOLS_GN_BUNDLE_FILE_RULE_H_

#include <vector>

#include "gn/source_file.h"
#include "gn/substitution_pattern.h"

class BundleData;
class Settings;
class SourceFile;
class Target;
class OutputFile;

// BundleFileRule contains the information found in a "bundle_data" target.
class BundleFileRule {
 public:
  BundleFileRule(const Target* bundle_data_target,
                 const std::vector<SourceFile> sources,
                 const SubstitutionPattern& pattern);
  BundleFileRule(const BundleFileRule& other);
  ~BundleFileRule();

  // Applies the substitution pattern to a source file, returning the result
  // as either a SourceFile or an OutputFile.
  bool ApplyPatternToSource(const Settings* settings,
                            const Target* target,
                            const BundleData& bundle_data,
                            const SourceFile& source_file,
                            SourceFile* expanded_source_file,
                            Err* err) const;
  bool ApplyPatternToSourceAsOutputFile(const Settings* settings,
                                        const Target* target,
                                        const BundleData& bundle_data,
                                        const SourceFile& source_file,
                                        OutputFile* expanded_output_file,
                                        Err* err) const;

  // Returns the associated target (of type Target::BUNDLE_DATA). May be
  // null during testing.
  const Target* target() const { return target_; }

  // Returns the list of SourceFiles.
  const std::vector<SourceFile>& sources() const { return sources_; }

 private:
  const Target* target_;
  std::vector<SourceFile> sources_;
  SubstitutionPattern pattern_;
};

#endif  // TOOLS_GN_BUNDLE_FILE_RULE_H_
