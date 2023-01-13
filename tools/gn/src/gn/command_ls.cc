// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <set>

#include "base/command_line.h"
#include "gn/commands.h"
#include "gn/label_pattern.h"
#include "gn/setup.h"
#include "gn/standard_out.h"
#include "gn/switches.h"
#include "gn/target.h"

namespace commands {

const char kLs[] = "ls";
const char kLs_HelpShort[] = "ls: List matching targets.";
const char kLs_Help[] =
    R"(gn ls <out_dir> [<label_pattern>] [--default-toolchain] [--as=...]
      [--type=...] [--testonly=...]

  Lists all targets matching the given pattern for the given build directory.
  By default, only targets in the default toolchain will be matched unless a
  toolchain is explicitly supplied.

  If the label pattern is unspecified, list all targets. The label pattern is
  not a general regular expression (see "gn help label_pattern"). If you need
  more complex expressions, pipe the result through grep.

Options

)" TARGET_PRINTING_MODE_COMMAND_LINE_HELP "\n" DEFAULT_TOOLCHAIN_SWITCH_HELP
    "\n" TARGET_TESTONLY_FILTER_COMMAND_LINE_HELP
    "\n" TARGET_TYPE_FILTER_COMMAND_LINE_HELP
    R"(
Examples

  gn ls out/Debug
      Lists all targets in the default toolchain.

  gn ls out/Debug "//base/*"
      Lists all targets in the directory base and all subdirectories.

  gn ls out/Debug "//base:*"
      Lists all targets defined in //base/BUILD.gn.

  gn ls out/Debug //base --as=output
      Lists the build output file for //base:base

  gn ls out/Debug --type=executable
      Lists all executables produced by the build.

  gn ls out/Debug "//base/*" --as=output | xargs ninja -C out/Debug
      Builds all targets in //base and all subdirectories.
)";

int RunLs(const std::vector<std::string>& args) {
  if (args.size() == 0) {
    Err(Location(), "Unknown command format. See \"gn help ls\"",
        "Usage: \"gn ls <build dir> [<label_pattern>]*\"")
        .PrintToStdout();
    return 1;
  }

  // Deliberately leaked to avoid expensive process teardown.
  Setup* setup = new Setup;
  if (!setup->DoSetup(args[0], false) || !setup->Run())
    return 1;

  const base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();
  bool default_toolchain_only = cmdline->HasSwitch(switches::kDefaultToolchain);

  std::vector<const Target*> matches;
  if (args.size() > 1) {
    // Some patterns or explicit labels were specified.
    std::vector<std::string> inputs(args.begin() + 1, args.end());

    UniqueVector<const Target*> target_matches;
    UniqueVector<const Config*> config_matches;
    UniqueVector<const Toolchain*> toolchain_matches;
    UniqueVector<SourceFile> file_matches;
    if (!ResolveFromCommandLineInput(setup, inputs, default_toolchain_only,
                                     &target_matches, &config_matches,
                                     &toolchain_matches, &file_matches))
      return 1;
    matches.insert(matches.begin(), target_matches.begin(),
                   target_matches.end());
  } else if (default_toolchain_only) {
    // List all resolved targets in the default toolchain.
    for (auto* target : setup->builder().GetAllResolvedTargets()) {
      if (target->settings()->is_default())
        matches.push_back(target);
    }
  } else {
    // List all resolved targets.
    matches = setup->builder().GetAllResolvedTargets();
  }
  FilterAndPrintTargets(false, &matches);
  return 0;
}

}  // namespace commands
