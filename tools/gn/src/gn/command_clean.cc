// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "gn/commands.h"
#include "gn/err.h"
#include "gn/filesystem_utils.h"
#include "gn/ninja_build_writer.h"
#include "gn/setup.h"

namespace {

bool CleanOneDir(const std::string& dir) {
  // Deliberately leaked to avoid expensive process teardown.
  Setup* setup = new Setup;
  if (!setup->DoSetup(dir, false))
    return false;

  base::FilePath build_dir(setup->build_settings().GetFullPath(
      SourceDir(setup->build_settings().build_dir().value())));

  // NOTE: Not all GN builds have args.gn file hence we also check here
  // if a build.ninja.d files exists instead.
  base::FilePath args_gn_file = build_dir.AppendASCII("args.gn");
  base::FilePath build_ninja_d_file = build_dir.AppendASCII("build.ninja.d");
  if (!base::PathExists(args_gn_file) &&
      !base::PathExists(build_ninja_d_file)) {
    Err(Location(),
        base::StringPrintf(
            "%s does not look like a build directory.\n",
            FilePathToUTF8(build_ninja_d_file.DirName().value()).c_str()))
        .PrintToStdout();
    return false;
  }

  // Replace existing build.ninja with just enough for ninja to call GN and
  // regenerate ninja files.
  if (!commands::PrepareForRegeneration(&setup->build_settings())) {
    return false;
  }

  // Erase everything but (user-created) args.gn and the build.ninja files we
  // just wrote.
  const base::FilePath::CharType* remaining[]{
      FILE_PATH_LITERAL("args.gn"),
      FILE_PATH_LITERAL("build.ninja"),
      FILE_PATH_LITERAL("build.ninja.d"),
  };
  base::FileEnumerator traversal(
      build_dir, false,
      base::FileEnumerator::FILES | base::FileEnumerator::DIRECTORIES);
  for (base::FilePath current = traversal.Next(); !current.empty();
       current = traversal.Next()) {
    base::FilePath::StringType basename =
        base::ToLowerASCII(current.BaseName().value());
    if (std::none_of(std::begin(remaining), std::end(remaining),
                     [&](auto rem) { return basename == rem; })) {
      base::DeleteFile(current, true);
    }
  }

  return true;
}

}  // namespace

namespace commands {

const char kClean[] = "clean";
const char kClean_HelpShort[] = "clean: Cleans the output directory.";
const char kClean_Help[] =
    "gn clean <out_dir>...\n"
    "\n"
    "  Deletes the contents of the output directory except for args.gn and\n"
    "  creates a Ninja build environment sufficient to regenerate the build.\n";

int RunClean(const std::vector<std::string>& args) {
  if (args.empty()) {
    Err(Location(), "Missing argument.", "Usage: \"gn clean <out_dir>...\"")
        .PrintToStdout();
    return 1;
  }

  for (const auto& dir : args) {
    if (!CleanOneDir(dir))
      return 1;
  }

  return 0;
}

}  // namespace commands
