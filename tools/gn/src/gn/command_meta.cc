// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <set>

#include "base/command_line.h"
#include "base/strings/string_split.h"
#include "gn/commands.h"
#include "gn/metadata_walk.h"
#include "gn/setup.h"
#include "gn/standard_out.h"
#include "gn/switches.h"
#include "gn/target.h"

namespace commands {

const char kMeta[] = "meta";
const char kMeta_HelpShort[] = "meta: List target metadata collection results.";
const char kMeta_Help[] =
    R"(gn meta

  gn meta <out_dir> <target>* --data=<key>[,<key>*]* [--walk=<key>[,<key>*]*]
          [--rebase=<dest dir>]

  Lists collected metaresults of all given targets for the given data key(s),
  collecting metadata dependencies as specified by the given walk key(s).

  See `gn help generated_file` for more information on the walk.

Arguments

  <target(s)>
    A list of target labels from which to initiate the walk.

  --data
    A comma-separated list of keys from which to extract data. In each target
    walked, its metadata scope is checked for the presence of these keys. If
    present, the contents of those variable in the scope are appended to the
    results list.

  --walk (optional)
    A comma-separated list of keys from which to control the walk. In each
    target walked, its metadata scope is checked for the presence of any of
    these keys. If present, the contents of those variables is checked to ensure
    that it is a label of a valid dependency of the target and then added to the
    set of targets to walk. If the empty string ("") is present in any of these
    keys, all deps and data_deps are added to the walk set.

  --rebase (optional)
    A destination directory onto which to rebase any paths found. If set, all
    collected metadata will be rebased onto this path. This option will throw errors
    if collected metadata is not a list of strings.

Examples

  gn meta out/Debug "//base/foo" --data=files
      Lists collected metaresults for the `files` key in the //base/foo:foo
      target and all of its dependency tree.

  gn meta out/Debug "//base/foo" --data=files,other
      Lists collected metaresults for the `files` and `other` keys in the
      //base/foo:foo target and all of its dependency tree.

  gn meta out/Debug "//base/foo" --data=files --walk=stop
      Lists collected metaresults for the `files` key in the //base/foo:foo
      target and all of the dependencies listed in the `stop` key (and so on).

  gn meta out/Debug "//base/foo" --data=files --rebase="/"
      Lists collected metaresults for the `files` key in the //base/foo:foo
      target and all of its dependency tree, rebasing the strings in the `files`
      key onto the source directory of the target's declaration relative to "/".
)";

int RunMeta(const std::vector<std::string>& args) {
  if (args.size() == 0) {
    Err(Location(), "Unknown command format. See \"gn help meta\"",
        "Usage: \"gn meta <out_dir> <target>* --data=<key>[,<key>*] "
        "[--walk=<key>[,<key>*]*] [--rebase=<dest dir>]\"")
        .PrintToStdout();
    return 1;
  }

  Setup* setup = new Setup;
  if (!setup->DoSetup(args[0], false) || !setup->Run())
    return 1;

  const base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();
  std::string rebase_dir = cmdline->GetSwitchValueString("rebase");
  std::string data_keys_str = cmdline->GetSwitchValueString("data");
  std::string walk_keys_str = cmdline->GetSwitchValueString("walk");

  std::vector<std::string> inputs(args.begin() + 1, args.end());

  UniqueVector<const Target*> targets;
  for (const auto& input : inputs) {
    const Target* target = ResolveTargetFromCommandLineString(setup, input);
    if (!target) {
      Err(Location(), "Unknown target " + input).PrintToStdout();
      return 1;
    }
    targets.push_back(target);
  }

  std::vector<std::string> data_keys = base::SplitString(
      data_keys_str, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  if (data_keys.empty()) {
    return 1;
  }
  std::vector<std::string> walk_keys = base::SplitString(
      walk_keys_str, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  Err err;
  TargetSet targets_walked;
  SourceDir rebase_source_dir(rebase_dir);
  // When SourceDir constructor is supplied with an empty string,
  // a trailing slash will be added. This prevent SourceDir::is_null()
  // from returning true. Explicitly remove this traling slash here.
  if (rebase_dir.empty()) {
    rebase_source_dir = SourceDir();
  }
  std::vector<Value> result = WalkMetadata(
      targets, data_keys, walk_keys, rebase_source_dir, &targets_walked, &err);
  if (err.has_error()) {
    err.PrintToStdout();
    return 1;
  }

  OutputString("Metadata values\n", DECORATION_DIM);
  for (const auto& value : result)
    OutputString("\n" + value.ToString(false) + "\n");

  // TODO(juliehockett): We should have better dep tracing and error support for
  // this. Also possibly data about where different values came from.
  OutputString("\nExtracted from:\n", DECORATION_DIM);
  bool first = true;
  for (const auto* target : targets_walked) {
    if (!first) {
      first = false;
      OutputString(", ", DECORATION_DIM);
    }
    OutputString(target->label().GetUserVisibleName(true) + "\n");
  }
  OutputString("\nusing data keys:\n", DECORATION_DIM);
  first = true;
  for (const auto& key : data_keys) {
    if (!first) {
      first = false;
      OutputString(", ");
    }
    OutputString(key + "\n");
  }
  if (!walk_keys.empty()) {
    OutputString("\nand using walk keys:\n", DECORATION_DIM);
    first = true;
    for (const auto& key : walk_keys) {
      if (!first) {
        first = false;
        OutputString(", ");
      }
      OutputString(key + "\n");
    }
  }
  return 0;
}

}  // namespace commands
