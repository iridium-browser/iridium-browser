// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_NINJA_BUILD_WRITER_H_
#define TOOLS_GN_NINJA_BUILD_WRITER_H_

#include <iosfwd>
#include <map>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "gn/path_output.h"

class Builder;
class BuildSettings;
class Err;
class Settings;
class Target;
class Toolchain;

namespace base {
class CommandLine;
}  // namespace base

// Generates the toplevel "build.ninja" file. This references the individual
// toolchain files and lists all input .gn files as dependencies of the
// build itself.
class NinjaBuildWriter {
 public:
  NinjaBuildWriter(const BuildSettings* settings,
                   const std::unordered_map<const Settings*, const Toolchain*>&
                       used_toolchains,
                   const std::vector<const Target*>& all_targets,
                   const Toolchain* default_toolchain,
                   const std::vector<const Target*>& default_toolchain_targets,
                   std::ostream& out,
                   std::ostream& dep_out);
  ~NinjaBuildWriter();

  // The design of this class is that this static factory function takes the
  // Builder, extracts the relevant information, and passes it to the class
  // constructor. The class itself doesn't depend on the Builder at all which
  // makes testing much easier (tests integrating various functions along with
  // the Builder get very complicated).
  static bool RunAndWriteFile(const BuildSettings* settings,
                              const Builder& builder,
                              Err* err);

  // Extracts from an existing build.ninja file's contents the commands
  // necessary to run GN and regenerate build.ninja.
  //
  // The regeneration rules live at the top of the build.ninja file and their
  // specific contents are an internal detail of NinjaBuildWriter. Used by
  // commands::PrepareForRegeneration.
  //
  // On error, returns an empty string.
  static std::string ExtractRegenerationCommands(std::istream& build_ninja_in);

  bool Run(Err* err);

 private:
  // WriteNinjaRules writes the rules that ninja uses to regenerate its own
  // build files, used whenever a build input file has changed.
  //
  // Ninja file regeneration is accomplished by two separate build statements.
  // This is necessary to work around ninja's behavior of deleting all output
  // files of a build edge if the edge uses a depfile and is interrupted before
  // it can complete. Previously, interrupting regeneration would cause ninja to
  // delete build.ninja, losing any flags/build settings passed to gen
  // previously and requiring the user to manually 'gen' again.
  //
  // The workaround involves misleading ninja about when the build.ninja file is
  // actually written. The first build statement runs the actual 'gen
  // --regeneration' command, writing "build.ninja" (and .d and .stamp) and
  // lists the "build.ninja.d" depfile to automatically trigger regeneration as
  // needed, but does not list "build.ninja" as an output. The second
  // statement's stated output is "build.ninja", but it simply uses the phony
  // rule to refer to the first statement.
  void WriteNinjaRules();
  void WriteAllPools();
  bool WriteSubninjas(Err* err);
  bool WritePhonyAndAllRules(Err* err);

  void WritePhonyRule(const Target* target, std::string_view phony_name);

  const BuildSettings* build_settings_;

  const std::unordered_map<const Settings*, const Toolchain*>& used_toolchains_;
  const std::vector<const Target*>& all_targets_;
  const Toolchain* default_toolchain_;
  const std::vector<const Target*>& default_toolchain_targets_;

  std::ostream& out_;
  std::ostream& dep_out_;
  PathOutput path_output_;

  NinjaBuildWriter(const NinjaBuildWriter&) = delete;
  NinjaBuildWriter& operator=(const NinjaBuildWriter&) = delete;
};

extern const char kNinjaRules_Help[];

// Exposed for testing.
base::CommandLine GetSelfInvocationCommandLine(
    const BuildSettings* build_settings);

#endif  // TOOLS_GN_NINJA_BUILD_WRITER_H_
