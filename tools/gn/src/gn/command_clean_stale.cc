// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <optional>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "gn/commands.h"
#include "gn/err.h"
#include "gn/ninja_tools.h"
#include "gn/setup.h"
#include "gn/source_dir.h"
#include "gn/switches.h"
#include "gn/version.h"

namespace commands {

namespace {

bool CleanStaleOneDir(const base::FilePath& ninja_executable,
                      const std::string& dir) {
  // Deliberately leaked to avoid expensive process teardown.
  Setup* setup = new Setup;
  if (!setup->DoSetup(dir, false))
    return false;

  base::FilePath build_dir(setup->build_settings().GetFullPath(
      SourceDir(setup->build_settings().build_dir().value())));

  // The order of operations for these tools is:
  // 1. cleandead - This eliminates old files from the build directory.
  // 2. recompact - This prunes old entries from the ninja log and deps files.
  //
  // This order is ideal because the files removed by cleandead will no longer
  // be found during the recompact, so ninja can prune their entries.
  Err err;
  if (!InvokeNinjaCleanDeadTool(ninja_executable, build_dir, &err)) {
    err.PrintToStdout();
    return false;
  }

  if (!InvokeNinjaRecompactTool(ninja_executable, build_dir, &err)) {
    err.PrintToStdout();
    return false;
  }
  return true;
}

}  // namespace

const char kCleanStale[] = "clean_stale";
const char kCleanStale_HelpShort[] =
    "clean_stale: Cleans the stale output files from the output directory.";
const char kCleanStale_Help[] =
    R"(gn clean_stale [--ninja-executable=...] <out_dir>...

  Removes the no longer needed output files from the build directory and prunes
  their records from the ninja build log and dependency database. These are
  output files that were generated from previous builds, but the current build
  graph no longer references them.

  This command requires a ninja executable of at least version 1.10.0. The
  executable must be provided by the --ninja-executable switch.

Options

  --ninja-executable=<string>
      Can be used to specify the ninja executable to use.
)";

int RunCleanStale(const std::vector<std::string>& args) {
  if (args.empty()) {
    Err(Location(), "Missing argument.",
        "Usage: \"gn clean_stale <out_dir>...\"")
        .PrintToStdout();
    return 1;
  }

  const base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();
  base::FilePath ninja_executable = cmdline->GetSwitchValuePath(switches::kNinjaExecutable);
  if (ninja_executable.empty()) {
    Err(Location(), "No --ninja-executable provided.",
        "--clean-stale requires a ninja executable to run. You can "
        "provide one on the command line via --ninja-executable.")
        .PrintToStdout();
    return 1;
  }

  for (const std::string& dir : args) {
    if (!CleanStaleOneDir(ninja_executable, dir))
      return 1;
  }

  return 0;
}

}  // namespace commands
