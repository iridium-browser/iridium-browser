// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/ninja_tools.h"

#include <vector>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/strings/string_number_conversions.h"
#include "gn/err.h"
#include "gn/exec_process.h"
#include "gn/filesystem_utils.h"

namespace {

base::CommandLine CreateNinjaToolCommandLine(const base::FilePath& ninja_executable,
                                             const std::string& tool) {
  base::CommandLine cmdline(ninja_executable);
  cmdline.SetParseSwitches(false);
  cmdline.AppendArg("-t");
  cmdline.AppendArg(tool);
  return cmdline;
}

bool RunNinja(const base::CommandLine& cmdline,
              const base::FilePath& startup_dir,
              std::string* output,
              Err* err) {
  std::string stderr_output;

  int exit_code = 0;
  if (!internal::ExecProcess(cmdline, startup_dir, output, &stderr_output,
                             &exit_code)) {
    *err = Err(Location(), "Could not execute Ninja.",
               "I was trying to execute \"" +
                   FilePathToUTF8(cmdline.GetProgram()) + "\".");
    return false;
  }

  if (exit_code != 0) {
    *err = Err(Location(), "Ninja has quit with exit code " +
                               base::IntToString(exit_code) + ".");
    return false;
  }

  return true;
}

}  // namespace

bool InvokeNinjaRestatTool(const base::FilePath& ninja_executable,
                           const base::FilePath& build_dir,
                           const std::vector<base::FilePath>& files_to_restat,
                           Err* err) {
  base::CommandLine cmdline =
      CreateNinjaToolCommandLine(ninja_executable, "restat");
  for (const base::FilePath& file : files_to_restat) {
    cmdline.AppendArgPath(file);
  }
  std::string output;
  return RunNinja(cmdline, build_dir, &output, err);
}

bool InvokeNinjaCleanDeadTool(const base::FilePath& ninja_executable,
                              const base::FilePath& build_dir,
                              Err* err) {
  base::CommandLine cmdline =
      CreateNinjaToolCommandLine(ninja_executable, "cleandead");
  std::string output;
  return RunNinja(cmdline, build_dir, &output, err);
}

bool InvokeNinjaRecompactTool(const base::FilePath& ninja_executable,
                              const base::FilePath& build_dir,
                              Err* err) {
  base::CommandLine cmdline =
      CreateNinjaToolCommandLine(ninja_executable, "recompact");
  std::string output;
  return RunNinja(cmdline, build_dir, &output, err);
}
