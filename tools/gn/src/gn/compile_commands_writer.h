// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_COMPILE_COMMANDS_WRITER_H_
#define TOOLS_GN_COMPILE_COMMANDS_WRITER_H_

#include <optional>
#include <vector>

#include "gn/err.h"
#include "gn/label_pattern.h"
#include "gn/target.h"

class Builder;
class BuildSettings;

class CompileCommandsWriter {
 public:
  // Writes a compilation database to the given file name consisting of the
  // recursive dependencies of all targets that match or are dependencies of
  // targets that match any given pattern.
  //
  // The legacy target filters takes a deprecated list of comma-separated target
  // names ("target_name1,target_name2...") which are matched against targets in
  // any directory. This is passed as an optional to encapsulate the legacy
  // behavior that providing the switch with no patterns matches everything, but
  // not passing the flag (nullopt for the function parameter) matches nothing.
  //
  // The union of the legacy matches and the target patterns are used.
  //
  // TODO(https://bugs.chromium.org/p/gn/issues/detail?id=302):
  // Remove this legacy target filters behavior.
  static bool RunAndWriteFiles(
      const BuildSettings* build_setting,
      const std::vector<const Target*>& all_targets,
      const std::vector<LabelPattern>& patterns,
      const std::optional<std::string>& legacy_target_filters,
      const base::FilePath& output_path,
      Err* err);

  // Collects all the targets whose commands should get written as part of
  // RunAndWriteFiles() (separated out for unit testing).
  static std::vector<const Target*> CollectTargets(
      const BuildSettings* build_setting,
      const std::vector<const Target*>& all_targets,
      const std::vector<LabelPattern>& patterns,
      const std::optional<std::string>& legacy_target_filters,
      Err* err);

  static std::string RenderJSON(const BuildSettings* build_settings,
                                std::vector<const Target*>& all_targets);

  // Does a depth-first search of the graph starting at the input target and
  // collects all recursive dependencies of those targets.
  static std::vector<const Target*> CollectDepsOfMatches(
      const std::vector<const Target*>& input_targets);

  // Performs the legacy target_name filtering.
  static std::vector<const Target*> FilterLegacyTargets(
      const std::vector<const Target*>& all_targets,
      const std::string& target_filter_string);
};

#endif  // TOOLS_GN_COMPILE_COMMANDS_WRITER_H_
