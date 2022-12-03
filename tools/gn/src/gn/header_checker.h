// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_HEADER_CHECKER_H_
#define TOOLS_GN_HEADER_CHECKER_H_

#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <string_view>
#include <vector>

#include "base/atomic_ref_count.h"
#include "base/gtest_prod_util.h"
#include "base/memory/ref_counted.h"
#include "gn/c_include_iterator.h"
#include "gn/err.h"
#include "gn/source_dir.h"

class BuildSettings;
class InputFile;
class SourceFile;
class Target;

namespace base {
class FilePath;
}

class HeaderChecker : public base::RefCountedThreadSafe<HeaderChecker> {
 public:
  // Represents a dependency chain.
  struct ChainLink {
    ChainLink() : target(nullptr), is_public(false) {}
    ChainLink(const Target* t, bool p) : target(t), is_public(p) {}

    const Target* target;

    // True when the dependency on this target is public.
    bool is_public;

    // Used for testing.
    bool operator==(const ChainLink& other) const {
      return target == other.target && is_public == other.is_public;
    }
  };
  using Chain = std::vector<ChainLink>;

  // check_generated, if true, will also check generated
  // files. Something that can only be done after running a build that
  // has generated them.
  HeaderChecker(const BuildSettings* build_settings,
                const std::vector<const Target*>& targets,
                bool check_generated,
                bool check_system);

  // Runs the check. The targets in to_check will be checked.
  //
  // This assumes that the current thread already has a message loop. On
  // error, fills the given vector with the errors and returns false. Returns
  // true on success.
  //
  // force_check, if true, will override targets opting out of header checking
  // with "check_includes = false" and will check them anyway.
  bool Run(const std::vector<const Target*>& to_check,
           bool force_check,
           std::vector<Err>* errors);

 private:
  friend class base::RefCountedThreadSafe<HeaderChecker>;
  FRIEND_TEST_ALL_PREFIXES(HeaderCheckerTest, IsDependencyOf);
  FRIEND_TEST_ALL_PREFIXES(HeaderCheckerTest, CheckInclude);
  FRIEND_TEST_ALL_PREFIXES(HeaderCheckerTest, PublicFirst);
  FRIEND_TEST_ALL_PREFIXES(HeaderCheckerTest, CheckIncludeAllowCircular);
  FRIEND_TEST_ALL_PREFIXES(HeaderCheckerTest, SourceFileForInclude);
  FRIEND_TEST_ALL_PREFIXES(HeaderCheckerTest,
                           SourceFileForInclude_FileNotFound);
  FRIEND_TEST_ALL_PREFIXES(HeaderCheckerTest,
                           SourceFileForInclude_SwiftBridgeHeader);
  FRIEND_TEST_ALL_PREFIXES(HeaderCheckerTest, Friend);

  ~HeaderChecker();

  struct TargetInfo {
    TargetInfo() : target(nullptr), is_public(false), is_generated(false) {}
    TargetInfo(const Target* t, bool is_pub, bool is_gen)
        : target(t), is_public(is_pub), is_generated(is_gen) {}

    const Target* target;

    // True if the file is public in the given target.
    bool is_public;

    // True if this file is generated and won't actually exist on disk.
    bool is_generated;
  };

  using TargetVector = std::vector<TargetInfo>;
  using FileMap = std::map<SourceFile, TargetVector>;
  using PathExistsCallback = std::function<bool(const base::FilePath& path)>;

  // Backend for Run() that takes the list of files to check. The errors_ list
  // will be populate on failure.
  void RunCheckOverFiles(const FileMap& flies, bool force_check);

  void DoWork(const Target* target, const SourceFile& file);

  // Adds the sources and public files from the given target to the given map.
  static void AddTargetToFileMap(const Target* target, FileMap* dest);

  // Returns true if the given file is in the output directory.
  bool IsFileInOuputDir(const SourceFile& file) const;

  // Resolves the contents of an include to a SourceFile.
  SourceFile SourceFileForInclude(const IncludeStringWithLocation& include,
                                  const std::vector<SourceDir>& include_dirs,
                                  const InputFile& source_file,
                                  Err* err) const;

  // from_target is the target the file was defined from. It will be used in
  // error messages.
  bool CheckFile(const Target* from_target,
                 const SourceFile& file,
                 std::vector<Err>* err) const;

  // Checks that the given file in the given target can include the
  // given include file. If disallowed, adds the error or errors to
  // the errors array.  The range indicates the location of the
  // include in the file for error reporting.
  // |no_depeency_cache| is used to cache or check whether there is no
  // dependency from |from_target| to target having |include_file|.
  void CheckInclude(
      const Target* from_target,
      const InputFile& source_file,
      const SourceFile& include_file,
      const LocationRange& range,
      std::set<std::pair<const Target*, const Target*>>* no_dependency_cache,
      std::vector<Err>* errors) const;

  // Returns true if the given search_for target is a dependency of
  // search_from.
  //
  // If found, the vector given in "chain" will be filled with the reverse
  // dependency chain from the dest target (chain[0] = search_for) to the src
  // target (chain[chain.size() - 1] = search_from).
  //
  // Chains with permitted dependencies will be considered first. If a
  // permitted match is found, *is_permitted will be set to true. A chain with
  // indirect, non-public dependencies will only be considered if there are no
  // public or direct chains. In this case, *is_permitted will be false.
  //
  // A permitted dependency is a sequence of public dependencies. The first
  // one may be private, since a direct dependency always allows headers to be
  // included.
  bool IsDependencyOf(const Target* search_for,
                      const Target* search_from,
                      Chain* chain,
                      bool* is_permitted) const;

  // For internal use by the previous override of IsDependencyOf.  If
  // require_public is true, only public dependency chains are searched.
  bool IsDependencyOf(const Target* search_for,
                      const Target* search_from,
                      bool require_permitted,
                      Chain* chain) const;

  // Makes a very descriptive error message for when an include is disallowed
  // from a given from_target, with a missing dependency to one of the given
  // targets.
  static Err MakeUnreachableError(const InputFile& source_file,
                                  const LocationRange& range,
                                  const Target* from_target,
                                  const TargetVector& targets);

  // Non-locked variables ------------------------------------------------------
  //
  // These are initialized during construction (which happens on one thread)
  // and are not modified after, so any thread can read these without locking.

  const BuildSettings* build_settings_;

  bool check_generated_;

  bool check_system_;

  // Maps source files to targets it appears in (usually just one target).
  FileMap file_map_;

  // Number of tasks posted by RunCheckOverFiles() that haven't completed their
  // execution.
  base::AtomicRefCount task_count_;

  // Locked variables ----------------------------------------------------------
  //
  // These are mutable during runtime and require locking.

  std::mutex lock_;

  std::vector<Err> errors_;

  // Signaled when |task_count_| becomes zero.
  std::condition_variable task_count_cv_;

  HeaderChecker(const HeaderChecker&) = delete;
  HeaderChecker& operator=(const HeaderChecker&) = delete;
};

#endif  // TOOLS_GN_HEADER_CHECKER_H_
