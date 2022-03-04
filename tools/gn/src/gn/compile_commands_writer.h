// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_COMPILE_COMMANDS_WRITER_H_
#define TOOLS_GN_COMPILE_COMMANDS_WRITER_H_

#include "gn/err.h"
#include "gn/target.h"

class Builder;
class BuildSettings;

class CompileCommandsWriter {
 public:
  // Write compile commands into a json file located by parameter file_name.
  //
  // Parameter target_filters should be in "target_name1,target_name2..."
  // format. If it is not empty, only targets that are reachable from targets
  // in target_filters are used to generate compile commands.
  //
  // Parameter quiet is not used.
  static bool RunAndWriteFiles(const BuildSettings* build_setting,
                               const Builder& builder,
                               const std::string& file_name,
                               const std::string& target_filters,
                               bool quiet,
                               Err* err);

  static std::string RenderJSON(const BuildSettings* build_settings,
                                std::vector<const Target*>& all_targets);

  static std::vector<const Target*> FilterTargets(
      const std::vector<const Target*>& all_targets,
      const std::set<std::string>& target_filters_set);

 private:
  // This function visits the deps graph of a target in a DFS fashion.
  static void VisitDeps(const Target* target, TargetSet* visited);
};

#endif  // TOOLS_GN_COMPILE_COMMANDS_WRITER_H_
