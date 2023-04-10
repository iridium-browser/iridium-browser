// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_METADATA_H_
#define TOOLS_GN_METADATA_H_

#include <memory>

#include "gn/build_settings.h"
#include "gn/scope.h"
#include "gn/source_dir.h"

extern const char kMetadata_Help[];

// Metadata about a particular target.
//
// Metadata is a collection of keys and values relating to a particular target.
// Generally, these keys will include three categories of strings: ordinary
// strings, filenames intended to be rebased according to their particular
// source directory, and target labels intended to be used as barriers to the
// walk. Verification of these categories occurs at walk time, not creation
// time (since it is not clear until the walk which values are intended for
// which purpose).
//
// Represented as a scope in the expression language, here it is reduced to just
// the KeyValueMap (since it doesn't need the logical overhead of a full scope).
// Values must be lists of strings, as the walking collection logic contatenates
// their values across targets.
class Metadata {
 public:
  using Contents = Scope::KeyValueMap;

  Metadata() = default;

  const ParseNode* origin() const { return origin_; }
  void set_origin(const ParseNode* origin) { origin_ = origin; }

  // The contents of this metadata variable.
  const Contents& contents() const { return contents_; }
  Contents& contents() { return contents_; }
  void set_contents(Contents&& contents) { contents_ = std::move(contents); }

  // The relative source directory to use when rebasing.
  const SourceDir& source_dir() const { return source_dir_; }
  SourceDir& source_dir() { return source_dir_; }
  void set_source_dir(const SourceDir& d) { source_dir_ = d; }

  // Collect the specified metadata from this instance.
  //
  // Calling this will populate `next_walk_keys` with the values of targets to
  // be walked next (with the empty string "" indicating that the target should
  // walk all of its deps and data_deps).
  bool WalkStep(const BuildSettings* settings,
                const std::vector<std::string>& keys_to_extract,
                const std::vector<std::string>& keys_to_walk,
                const SourceDir& rebase_dir,
                std::vector<Value>* next_walk_keys,
                std::vector<Value>* result,
                Err* err) const;

 private:
  const ParseNode* origin_ = nullptr;
  Contents contents_;
  SourceDir source_dir_;

  std::pair<Value, bool> RebaseValue(const BuildSettings* settings,
                                     const SourceDir& rebase_dir,
                                     const Value& value,
                                     Err* err) const;

  std::pair<Value, bool> RebaseStringValue(const BuildSettings* settings,
                                           const SourceDir& rebase_dir,
                                           const Value& value,
                                           Err* err) const;

  std::pair<Value, bool> RebaseListValue(const BuildSettings* settings,
                                         const SourceDir& rebase_dir,
                                         const Value& value,
                                         Err* err) const;

  std::pair<Value, bool> RebaseScopeValue(const BuildSettings* settings,
                                          const SourceDir& rebase_dir,
                                          const Value& value,
                                          Err* err) const;

  Metadata(const Metadata&) = delete;
  Metadata& operator=(const Metadata&) = delete;
};

#endif  // TOOLS_GN_METADATA_H_
