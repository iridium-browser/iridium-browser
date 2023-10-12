// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/setup.h"

#include <stdlib.h>

#include <algorithm>
#include <memory>
#include <sstream>
#include <utility>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "gn/command_format.h"
#include "gn/commands.h"
#include "gn/exec_process.h"
#include "gn/filesystem_utils.h"
#include "gn/input_file.h"
#include "gn/label_pattern.h"
#include "gn/parse_tree.h"
#include "gn/parser.h"
#include "gn/source_dir.h"
#include "gn/source_file.h"
#include "gn/standard_out.h"
#include "gn/switches.h"
#include "gn/tokenizer.h"
#include "gn/trace.h"
#include "gn/value.h"
#include "gn/value_extractors.h"
#include "util/build_config.h"

#if defined(OS_WIN)
#include <windows.h>

#include "base/win/scoped_process_information.h"
#include "base/win/win_util.h"
#endif

const char kDotfile_Help[] =
    R"(.gn file

  When gn starts, it will search the current directory and parent directories
  for a file called ".gn". This indicates the source root. You can override
  this detection by using the --root command-line argument

  The .gn file in the source root will be executed. The syntax is the same as a
  buildfile, but with very limited build setup-specific meaning.

  If you specify --root, by default GN will look for the file .gn in that
  directory. If you want to specify a different file, you can additionally pass
  --dotfile:

    gn gen out/Debug --root=/home/build --dotfile=/home/my_gn_file.gn

Variables

  arg_file_template [optional]
      Path to a file containing the text that should be used as the default
      args.gn content when you run `gn args`.

  buildconfig [required]
      Path to the build config file. This file will be used to set up the
      build file execution environment for each toolchain.

  check_targets [optional]
      A list of labels and label patterns that should be checked when running
      "gn check" or "gn gen --check". If neither check_targets or
      no_check_targets (see below) is specified, all targets will be checked.
      It is an error to specify both check_targets and no_check_targets. If it
      is the empty list, no targets will be checked. To bypass this list,
      request an explicit check of targets, like "//*".

      The format of this list is identical to that of "visibility" so see "gn
      help visibility" for examples.

  no_check_targets [optional]
      A list of labels and label patterns that should *not* be checked when
      running "gn check" or "gn gen --check". All other targets will be checked.
      If neither check_targets (see above) or no_check_targets is specified, all
      targets will be checked. It is an error to specify both check_targets and
      no_check_targets.

      The format of this list is identical to that of "visibility" so see "gn
      help visibility" for examples.

  check_system_includes [optional]
      Boolean to control whether system style includes are checked by default
      when running "gn check" or "gn gen --check".  System style includes are
      includes that use angle brackets <> instead of double quotes "". If this
      setting is omitted or set to false, these includes will be ignored by
      default. They can be checked explicitly by running
      "gn check --check-system" or "gn gen --check=system"

  exec_script_whitelist [optional]
      A list of .gn/.gni files (not labels) that have permission to call the
      exec_script function. If this list is defined, calls to exec_script will
      be checked against this list and GN will fail if the current file isn't
      in the list.

      This is to allow the use of exec_script to be restricted since is easy to
      use inappropriately. Wildcards are not supported. Files in the
      secondary_source tree (if defined) should be referenced by ignoring the
      secondary tree and naming them as if they are in the main tree.

      If unspecified, the ability to call exec_script is unrestricted.

      Example:
        exec_script_whitelist = [
          "//base/BUILD.gn",
          "//build/my_config.gni",
        ]

  export_compile_commands [optional]
      A list of label patterns for which to generate a Clang compilation
      database (see "gn help label_pattern" for the string format).

      When specified, GN will generate a compile_commands.json file in the root
      of the build directory containing information on how to compile each
      source file reachable from any label matching any pattern in the list.
      This is used for Clang-based tooling and some editor integration. See
      https://clang.llvm.org/docs/JSONCompilationDatabase.html

      The switch --add-export-compile-commands to "gn gen" (see "gn help gen")
      appends to this value which provides a per-user way to customize it.

      The deprecated switch --export-compile-commands to "gn gen" (see "gn help
      gen") adds to the export target list using a different format.

      Example:
        export_compile_commands = [
          "//base/*",
          "//tools:doom_melon",
        ]

  root [optional]
      Label of the root build target. The GN build will start by loading the
      build file containing this target name. This defaults to "//:" which will
      cause the file //BUILD.gn to be loaded. Note that build_file_extension
      applies to the default case as well.

      The command-line switch --root-target will override this value (see "gn
      help --root-target").

  script_executable [optional]
      By default, GN runs the scripts used in action targets and exec_script
      calls using the Python interpreter found in PATH. This value specifies the
      Python executable or other interpreter to use instead.

      If set to the empty string, the scripts will be executed directly.

      The command-line switch --script-executable will override this value (see
      "gn help --script-executable")

  secondary_source [optional]
      Label of an alternate directory tree to find input files. When searching
      for a BUILD.gn file (or the build config file discussed above), the file
      will first be looked for in the source root. If it's not found, the
      secondary source root will be checked (which would contain a parallel
      directory hierarchy).

      This behavior is intended to be used when BUILD.gn files can't be checked
      in to certain source directories for whatever reason.

      The secondary source root must be inside the main source tree.

  default_args [optional]
      Scope containing the default overrides for declared arguments. These
      overrides take precedence over the default values specified in the
      declare_args() block, but can be overridden using --args or the
      args.gn file.

      This is intended to be used when subprojects declare arguments with
      default values that need to be changed for whatever reason.

  build_file_extension [optional]
      If set to a non-empty string, this is added to the name of all build files
      to load.
      GN will look for build files named "BUILD.$build_file_extension.gn".
      This is intended to be used during migrations or other situations where
      there are two independent GN builds in the same directories.

  ninja_required_version [optional]
      When set specifies the minimum required version of Ninja. The default
      required version is 1.7.2. Specifying a higher version might enable the
      use of some of newer features that can make the build more efficient.


Example .gn file contents

  buildconfig = "//build/config/BUILDCONFIG.gn"

  check_targets = [
    "//doom_melon/*",  # Check everything in this subtree.
    "//tools:mind_controlling_ant",  # Check this specific target.
  ]

  root = "//:root"

  secondary_source = "//build/config/temporary_buildfiles/"

  default_args = {
    # Default to release builds for this project.
    is_debug = false
    is_component_build = false
  }
)";

namespace {

const base::FilePath::CharType kGnFile[] = FILE_PATH_LITERAL(".gn");
const char kDefaultArgsGn[] =
    "# Set build arguments here. See `gn help buildargs`.";

base::FilePath FindDotFile(const base::FilePath& current_dir) {
  base::FilePath try_this_file = current_dir.Append(kGnFile);
  if (base::PathExists(try_this_file))
    return try_this_file;

  base::FilePath with_no_slash = current_dir.StripTrailingSeparators();
  base::FilePath up_one_dir = with_no_slash.DirName();
  if (up_one_dir == current_dir)
    return base::FilePath();  // Got to the top.

  return FindDotFile(up_one_dir);
}

// Called on any thread. Post the item to the builder on the main thread.
void ItemDefinedCallback(MsgLoop* task_runner,
                         Builder* builder_call_on_main_thread_only,
                         std::unique_ptr<Item> item) {
  DCHECK(item);

  // Increment the work count for the duration of defining the item with the
  // builder. Otherwise finishing this callback will race finishing loading
  // files. If there is no other pending work at any point in the middle of
  // this call completing on the main thread, the 'Complete' function will
  // be signaled and we'll stop running with an incomplete build.
  g_scheduler->IncrementWorkCount();

  // Work around issue binding a unique_ptr with std::function by moving into a
  // shared_ptr.
  auto item_shared = std::make_shared<std::unique_ptr<Item>>(std::move(item));
  task_runner->PostTask(
      [builder_call_on_main_thread_only, item_shared]() mutable {
        builder_call_on_main_thread_only->ItemDefined(std::move(*item_shared));
        g_scheduler->DecrementWorkCount();
      });
}

void DecrementWorkCount() {
  g_scheduler->DecrementWorkCount();
}

#if defined(OS_WIN)

std::u16string SysMultiByteTo16(std::string_view mb) {
  if (mb.empty())
    return std::u16string();

  int mb_length = static_cast<int>(mb.length());
  // Compute the length of the buffer.
  int charcount = MultiByteToWideChar(CP_ACP, 0, mb.data(), mb_length, NULL, 0);
  if (charcount == 0)
    return std::u16string();

  std::u16string wide;
  wide.resize(charcount);
  MultiByteToWideChar(CP_ACP, 0, mb.data(), mb_length, base::ToWCharT(&wide[0]),
                      charcount);

  return wide;
}

// Given the path to a batch file that runs Python, extracts the name of the
// executable actually implementing Python. Generally people write a batch file
// to put something named "python" on the path, which then just redirects to
// a python.exe somewhere else. This step decodes that setup. On failure,
// returns empty path.
base::FilePath PythonBatToExe(const base::FilePath& bat_path) {
  // Note exciting double-quoting to allow spaces. The /c switch seems to check
  // for quotes around the whole thing and then deletes them. If you want to
  // quote the first argument in addition (to allow for spaces in the Python
  // path, you need *another* set of quotes around that, likewise, we need
  // two quotes at the end.
  std::u16string command = u"cmd.exe /c \"\"";
  command.append(bat_path.value());
  command.append(u"\" -c \"import sys; print(sys.executable)\"\"");

  std::string python_path;
  std::string std_err;
  int exit_code;
  base::FilePath cwd;
  GetCurrentDirectory(&cwd);
  if (internal::ExecProcess(command, cwd, &python_path, &std_err, &exit_code) &&
      exit_code == 0 && std_err.empty()) {
    base::TrimWhitespaceASCII(python_path, base::TRIM_ALL, &python_path);

    // Python uses the system multibyte code page for sys.executable.
    base::FilePath exe_path(SysMultiByteTo16(python_path));

    // Check for reasonable output, cmd may have output an error message.
    if (base::PathExists(exe_path))
      return exe_path;
  }
  return base::FilePath();
}

// python_exe_name and python_bat_name can be empty but cannot be absolute
// paths. They should be "python.exe" or "", etc., and "python.bat" or "", etc.
base::FilePath FindWindowsPython(const base::FilePath& python_exe_name,
                                 const base::FilePath& python_bat_name) {
  char16_t current_directory[MAX_PATH];
  ::GetCurrentDirectory(MAX_PATH, reinterpret_cast<LPWSTR>(current_directory));

  // First search for python.exe in the current directory.
  if (!python_exe_name.empty()) {
    CHECK(python_exe_name.FinalExtension() == u".exe");
    CHECK_EQ(python_exe_name.IsAbsolute(), false);
    base::FilePath cur_dir_candidate_exe =
        base::FilePath(current_directory).Append(python_exe_name);
    if (base::PathExists(cur_dir_candidate_exe))
      return cur_dir_candidate_exe;
  }

  // Get the path.
  const char16_t kPathEnvVarName[] = u"Path";
  DWORD path_length = ::GetEnvironmentVariable(
      reinterpret_cast<LPCWSTR>(kPathEnvVarName), nullptr, 0);
  if (path_length == 0)
    return base::FilePath();
  std::unique_ptr<char16_t[]> full_path(new char16_t[path_length]);
  DWORD actual_path_length = ::GetEnvironmentVariable(
      reinterpret_cast<LPCWSTR>(kPathEnvVarName),
      reinterpret_cast<LPWSTR>(full_path.get()), path_length);
  CHECK_EQ(path_length, actual_path_length + 1);

  // Search for python.exe in the path.
  for (const auto& component : base::SplitStringPiece(
           std::u16string_view(full_path.get(), path_length), u";",
           base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY)) {
    if (!python_exe_name.empty()) {
      base::FilePath candidate_exe =
          base::FilePath(component).Append(python_exe_name);
      if (base::PathExists(candidate_exe))
        return candidate_exe;
    }

    // Also allow python.bat, but convert into the .exe.
    if (!python_bat_name.empty()) {
      CHECK(python_bat_name.FinalExtension() == u".bat");
      CHECK_EQ(python_bat_name.IsAbsolute(), false);
      base::FilePath candidate_bat =
          base::FilePath(component).Append(python_bat_name);
      if (base::PathExists(candidate_bat)) {
        base::FilePath python_exe = PythonBatToExe(candidate_bat);
        if (!python_exe.empty())
          return python_exe;
      }
    }
  }
  return base::FilePath();
}
#endif

}  // namespace

const char Setup::kBuildArgFileName[] = "args.gn";

Setup::Setup()
    : build_settings_(),
      loader_(new LoaderImpl(&build_settings_)),
      builder_(loader_.get()),
      dotfile_settings_(&build_settings_, std::string()),
      dotfile_scope_(&dotfile_settings_) {
  dotfile_settings_.set_toolchain_label(Label());

  build_settings_.set_item_defined_callback(
      [task_runner = scheduler_.task_runner(),
       builder = &builder_](std::unique_ptr<Item> item) {
        ItemDefinedCallback(task_runner, builder, std::move(item));
      });

  loader_->set_complete_callback(&DecrementWorkCount);
  // The scheduler's task runner wasn't created when the Loader was created, so
  // we need to set it now.
  loader_->set_task_runner(scheduler_.task_runner());
}

bool Setup::DoSetup(const std::string& build_dir, bool force_create) {
  return DoSetup(build_dir, force_create,
                 *base::CommandLine::ForCurrentProcess());
}

bool Setup::DoSetup(const std::string& build_dir,
                    bool force_create,
                    const base::CommandLine& cmdline) {
  Err err;
  if (!DoSetupWithErr(build_dir, force_create, cmdline, &err)) {
    err.PrintToStdout();
    return false;
  }
  DCHECK(!err.has_error());
  return true;
}

bool Setup::DoSetupWithErr(const std::string& build_dir,
                           bool force_create,
                           const base::CommandLine& cmdline,
                           Err* err) {
  scheduler_.set_verbose_logging(cmdline.HasSwitch(switches::kVerbose));
  if (cmdline.HasSwitch(switches::kTime) ||
      cmdline.HasSwitch(switches::kTracelog))
    EnableTracing();

  ScopedTrace setup_trace(TraceItem::TRACE_SETUP, "DoSetup");

  if (!FillSourceDir(cmdline, err))
    return false;
  if (!RunConfigFile(err))
    return false;
  if (!FillOtherConfig(cmdline, err))
    return false;

  // Must be after FillSourceDir to resolve.
  if (!FillBuildDir(build_dir, !force_create, err))
    return false;

  // Apply project-specific default (if specified).
  // Must happen before FillArguments().
  if (default_args_) {
    Scope::KeyValueMap overrides;
    default_args_->GetCurrentScopeValues(&overrides);
    build_settings_.build_args().AddDefaultArgOverrides(overrides);
  }

  if (fill_arguments_) {
    if (!FillArguments(cmdline, err))
      return false;
  }
  if (!FillPythonPath(cmdline, err))
    return false;

  // Check for unused variables in the .gn file.
  if (!dotfile_scope_.CheckForUnusedVars(err)) {
    return false;
  }

  return true;
}

bool Setup::Run() {
  return Run(*base::CommandLine::ForCurrentProcess());
}

bool Setup::Run(const base::CommandLine& cmdline) {
  RunPreMessageLoop();
  if (!scheduler_.Run())
    return false;
  return RunPostMessageLoop(cmdline);
}

SourceFile Setup::GetBuildArgFile() const {
  return SourceFile(build_settings_.build_dir().value() + kBuildArgFileName);
}

void Setup::RunPreMessageLoop() {
  // Will be decremented with the loader is drained.
  g_scheduler->IncrementWorkCount();

  // Load the root build file.
  loader_->Load(root_build_file_, LocationRange(), Label());
}

bool Setup::RunPostMessageLoop(const base::CommandLine& cmdline) {
  Err err;
  if (!builder_.CheckForBadItems(&err)) {
    err.PrintToStdout();
    return false;
  }

  if (!build_settings_.build_args().VerifyAllOverridesUsed(&err)) {
    if (cmdline.HasSwitch(switches::kFailOnUnusedArgs)) {
      err.PrintToStdout();
      return false;
    }
    err.PrintNonfatalToStdout();
    OutputString(
        "\nThe build continued as if that argument was "
        "unspecified.\n\n");
    // Nonfatal error.
  }

  if (check_public_headers_) {
    std::vector<const Target*> all_targets = builder_.GetAllResolvedTargets();
    std::vector<const Target*> to_check;
    if (check_patterns()) {
      commands::FilterTargetsByPatterns(all_targets, *check_patterns(),
                                        &to_check);
    } else if (no_check_patterns()) {
      commands::FilterOutTargetsByPatterns(all_targets, *no_check_patterns(),
                                           &to_check);
    } else {
      to_check = all_targets;
    }

    if (!commands::CheckPublicHeaders(&build_settings_, all_targets, to_check,
                                      false, false, check_system_includes_)) {
      return false;
    }
  }

  // Write out tracing and timing if requested.
  if (cmdline.HasSwitch(switches::kTime))
    PrintLongHelp(SummarizeTraces());
  if (cmdline.HasSwitch(switches::kTracelog))
    SaveTraces(cmdline.GetSwitchValuePath(switches::kTracelog));

  return true;
}

bool Setup::FillArguments(const base::CommandLine& cmdline, Err* err) {
  // Use the args on the command line if specified, and save them. Do this even
  // if the list is empty (this means clear any defaults).
  // If --args is not set, args.gn file does not exist and gen_empty_args
  // is set, generate an empty args.gn file with default comments.

  base::FilePath build_arg_file =
      build_settings_.GetFullPath(GetBuildArgFile());
  auto switch_value = cmdline.GetSwitchValueString(switches::kArgs);
  if (cmdline.HasSwitch(switches::kArgs) ||
      (gen_empty_args_ && !PathExists(build_arg_file))) {
    if (!FillArgsFromCommandLine(
            switch_value.empty() ? kDefaultArgsGn : switch_value, err)) {
      return false;
    }
    SaveArgsToFile();
    return true;
  }

  // No command line args given, use the arguments from the build dir (if any).
  return FillArgsFromFile(err);
}

bool Setup::FillArgsFromCommandLine(const std::string& args, Err* err) {
  args_input_file_ = std::make_unique<InputFile>(SourceFile());
  args_input_file_->SetContents(args);
  args_input_file_->set_friendly_name("the command-line \"--args\"");
  return FillArgsFromArgsInputFile(err);
}

bool Setup::FillArgsFromFile(Err* err) {
  ScopedTrace setup_trace(TraceItem::TRACE_SETUP, "Load args file");

  SourceFile build_arg_source_file = GetBuildArgFile();
  base::FilePath build_arg_file =
      build_settings_.GetFullPath(build_arg_source_file);

  std::string contents;
  if (!base::ReadFileToString(build_arg_file, &contents))
    return true;  // File doesn't exist, continue with default args.

  // Add a dependency on the build arguments file. If this changes, we want
  // to re-generate the build.
  g_scheduler->AddGenDependency(build_arg_file);

  if (contents.empty())
    return true;  // Empty file, do nothing.

  args_input_file_ = std::make_unique<InputFile>(build_arg_source_file);
  args_input_file_->SetContents(contents);
  args_input_file_->set_friendly_name(
      "build arg file (use \"gn args <out_dir>\" to edit)");

  setup_trace.Done();  // Only want to count the load as part of the trace.
  return FillArgsFromArgsInputFile(err);
}

bool Setup::FillArgsFromArgsInputFile(Err* err) {
  ScopedTrace setup_trace(TraceItem::TRACE_SETUP, "Parse args");

  args_tokens_ = Tokenizer::Tokenize(args_input_file_.get(), err);
  if (err->has_error()) {
    return false;
  }

  args_root_ = Parser::Parse(args_tokens_, err);
  if (err->has_error()) {
    return false;
  }

  Scope arg_scope(&dotfile_settings_);
  // Set soure dir so relative imports in args work.
  SourceDir root_source_dir =
      SourceDirForCurrentDirectory(build_settings_.root_path());
  arg_scope.set_source_dir(root_source_dir);
  args_root_->Execute(&arg_scope, err);
  if (err->has_error()) {
    return false;
  }

  // Save the result of the command args.
  Scope::KeyValueMap overrides;
  arg_scope.GetCurrentScopeValues(&overrides);
  build_settings_.build_args().AddArgOverrides(overrides);
  build_settings_.build_args().set_build_args_dependency_files(
      arg_scope.build_dependency_files());
  return true;
}

bool Setup::SaveArgsToFile() {
  ScopedTrace setup_trace(TraceItem::TRACE_SETUP, "Save args file");

  // For the first run, the build output dir might not be created yet, so do
  // that so we can write a file into it. Ignore errors, we'll catch the error
  // when we try to write a file to it below.
  base::FilePath build_arg_file =
      build_settings_.GetFullPath(GetBuildArgFile());
  base::CreateDirectory(build_arg_file.DirName());

  std::string contents = args_input_file_->contents();
  commands::FormatStringToString(contents, commands::TreeDumpMode::kInactive,
                                 &contents, nullptr);
#if defined(OS_WIN)
  // Use Windows lineendings for this file since it will often open in
  // Notepad which can't handle Unix ones.
  base::ReplaceSubstringsAfterOffset(&contents, 0, "\n", "\r\n");
#endif
  if (base::WriteFile(build_arg_file, contents.c_str(),
                      static_cast<int>(contents.size())) == -1) {
    Err(Location(), "Args file could not be written.",
        "The file is \"" + FilePathToUTF8(build_arg_file) + "\"")
        .PrintToStdout();
    return false;
  }

  // Add a dependency on the build arguments file. If this changes, we want
  // to re-generate the build.
  g_scheduler->AddGenDependency(build_arg_file);

  return true;
}

bool Setup::FillSourceDir(const base::CommandLine& cmdline, Err* err) {
  // Find the .gn file.
  base::FilePath root_path;

  // Prefer the command line args to the config file.
  base::FilePath relative_root_path =
      cmdline.GetSwitchValuePath(switches::kRoot);
  if (!relative_root_path.empty()) {
    root_path = base::MakeAbsoluteFilePath(relative_root_path);
    if (root_path.empty()) {
      *err = Err(Location(), "Root source path not found.",
                 "The path \"" + FilePathToUTF8(relative_root_path) +
                     "\" doesn't exist.");
      return false;
    }

    // When --root is specified, an alternate --dotfile can also be set.
    // --dotfile should be a real file path and not a "//foo" source-relative
    // path.
    base::FilePath dotfile_path =
        cmdline.GetSwitchValuePath(switches::kDotfile);
    if (dotfile_path.empty()) {
      dotfile_name_ = root_path.Append(kGnFile);
    } else {
      dotfile_name_ = base::MakeAbsoluteFilePath(dotfile_path);
      if (dotfile_name_.empty()) {
        *err = Err(Location(), "Could not load dotfile.",
                   "The file \"" + FilePathToUTF8(dotfile_path) +
                       "\" couldn't be loaded.");
        return false;
      }
      // Only set dotfile_name if it was passed explicitly.
      build_settings_.set_dotfile_name(dotfile_name_);
    }
  } else {
    // In the default case, look for a dotfile and that also tells us where the
    // source root is.
    base::FilePath cur_dir;
    base::GetCurrentDirectory(&cur_dir);
    dotfile_name_ = FindDotFile(cur_dir);
    if (dotfile_name_.empty()) {
      *err = Err(
          Location(), "Can't find source root.",
          "I could not find a \".gn\" file in the current directory or any "
          "parent,\nand the --root command-line argument was not specified.");
      return false;
    }
    root_path = dotfile_name_.DirName();
  }

  base::FilePath root_realpath = base::MakeAbsoluteFilePath(root_path);
  if (root_realpath.empty()) {
    *err = Err(Location(), "Can't get the real root path.",
               "I could not get the real path of \"" +
                   FilePathToUTF8(root_path) + "\".");
    return false;
  }
  if (scheduler_.verbose_logging())
    scheduler_.Log("Using source root", FilePathToUTF8(root_realpath));
  build_settings_.SetRootPath(root_realpath);

  return true;
}

bool Setup::FillBuildDir(const std::string& build_dir,
                         bool require_exists,
                         Err* err) {
  SourceDir resolved =
      SourceDirForCurrentDirectory(build_settings_.root_path())
          .ResolveRelativeDir(Value(nullptr, build_dir), err,
                              build_settings_.root_path_utf8());
  if (err->has_error()) {
    return false;
  }

  base::FilePath build_dir_path = build_settings_.GetFullPath(resolved);
  if (!base::CreateDirectory(build_dir_path)) {
    *err = Err(Location(), "Can't create the build dir.",
               "I could not create the build dir \"" +
                   FilePathToUTF8(build_dir_path) + "\".");
    return false;
  }
  base::FilePath build_dir_realpath =
      base::MakeAbsoluteFilePath(build_dir_path);
  if (build_dir_realpath.empty()) {
    *err = Err(Location(), "Can't get the real build dir path.",
               "I could not get the real path of \"" +
                   FilePathToUTF8(build_dir_path) + "\".");
    return false;
  }
  resolved = SourceDirForPath(build_settings_.root_path(), build_dir_realpath);

  if (scheduler_.verbose_logging())
    scheduler_.Log("Using build dir", resolved.value());

  if (require_exists) {
    if (!base::PathExists(
            build_dir_path.Append(FILE_PATH_LITERAL("build.ninja")))) {
      *err = Err(
          Location(), "Not a build directory.",
          "This command requires an existing build directory. I interpreted "
          "your input\n\"" +
              build_dir + "\" as:\n  " + FilePathToUTF8(build_dir_path) +
              "\nwhich doesn't seem to contain a previously-generated build.");
      return false;
    }
  }

  build_settings_.SetBuildDir(resolved);
  return true;
}

// On Chromium repositories on Windows the Python executable can be specified as
// python, python.bat, or python.exe (ditto for python3, and with or without a
// full path specification). This handles all of these cases and returns a fully
// specified path to a .exe file.
// This is currently a NOP on other platforms.
base::FilePath ProcessFileExtensions(base::FilePath script_executable) {
#if defined(OS_WIN)
  // If we have a relative path with no extension such as "python" or
  // "python3" then do a path search on the name with .exe and .bat appended.
  auto extension = script_executable.FinalExtension();
  if (script_executable.IsAbsolute()) {
    // Do translation from .bat to .exe but otherwise just pass through.
    if (extension == u".bat")
      script_executable = PythonBatToExe(script_executable);
  } else {
    if (extension == u"") {
      // If no extension is specified then search the path for .exe and .bat
      // variants.
      script_executable =
          FindWindowsPython(script_executable.ReplaceExtension(u".exe"),
                            script_executable.ReplaceExtension(u".bat"));
    } else if (extension == u".bat") {
      // Search the path just for the specified .bat.
      script_executable =
          FindWindowsPython(base::FilePath(), script_executable);
    } else if (extension == u".exe") {
      // Search the path just for the specified .exe.
      script_executable =
          FindWindowsPython(script_executable, base::FilePath());
    }
  }
  script_executable = script_executable.NormalizePathSeparatorsTo('/');
#endif
  return script_executable;
}

bool Setup::FillPythonPath(const base::CommandLine& cmdline, Err* err) {
  // Trace this since it tends to be a bit slow on Windows.
  ScopedTrace setup_trace(TraceItem::TRACE_SETUP, "Fill Python Path");
  const Value* value = dotfile_scope_.GetValue("script_executable", true);
  if (cmdline.HasSwitch(switches::kScriptExecutable)) {
    auto script_executable =
        cmdline.GetSwitchValuePath(switches::kScriptExecutable);
    build_settings_.set_python_path(ProcessFileExtensions(script_executable));
  } else if (value) {
    if (!value->VerifyTypeIs(Value::STRING, err)) {
      return false;
    }
    // Note that an empty string value is valid, and means that the scripts
    // invoked by actions will be run directly.
    base::FilePath python_path;
    if (!value->string_value().empty()) {
      python_path =
          ProcessFileExtensions(UTF8ToFilePath(value->string_value()));
      if (python_path.empty()) {
        *err = Err(Location(), "Could not find \"" + value->string_value() +
                                   "\" from dotfile in PATH.");
        return false;
      }
    }
    build_settings_.set_python_path(python_path);
  } else {
#if defined(OS_WIN)
    base::FilePath python_path =
        ProcessFileExtensions(base::FilePath(u"python"));
    if (!python_path.IsAbsolute()) {
      scheduler_.Log("WARNING",
                     "Could not find python on path, using "
                     "just \"python.exe\"");
      python_path = base::FilePath(u"python.exe");
    }
    build_settings_.set_python_path(python_path);
#else
    build_settings_.set_python_path(base::FilePath("python"));
#endif
  }
  return true;
}

bool Setup::RunConfigFile(Err* err) {
  if (scheduler_.verbose_logging())
    scheduler_.Log("Got dotfile", FilePathToUTF8(dotfile_name_));

  dotfile_input_file_ = std::make_unique<InputFile>(SourceFile("//.gn"));
  if (!dotfile_input_file_->Load(dotfile_name_)) {
    *err = Err(Location(), "Could not load dotfile.",
               "The file \"" + FilePathToUTF8(dotfile_name_) +
                   "\" couldn't be loaded");
    return false;
  }

  dotfile_tokens_ = Tokenizer::Tokenize(dotfile_input_file_.get(), err);
  if (err->has_error()) {
    return false;
  }

  dotfile_root_ = Parser::Parse(dotfile_tokens_, err);
  if (err->has_error()) {
    return false;
  }

  // Add a dependency on the build arguments file. If this changes, we want
  // to re-generate the build. This causes the dotfile to make it into
  // build.ninja.d.
  g_scheduler->AddGenDependency(dotfile_name_);

  // Also add a build dependency to the scope, which is used by `gn analyze`.
  dotfile_scope_.AddBuildDependencyFile(SourceFile("//.gn"));
  dotfile_root_->Execute(&dotfile_scope_, err);
  if (err->has_error()) {
    return false;
  }

  return true;
}

bool Setup::FillOtherConfig(const base::CommandLine& cmdline, Err* err) {
  SourceDir current_dir("//");
  Label root_target_label(current_dir, "");

  // Secondary source path, read from the config file if present.
  // Read from the config file if present.
  const Value* secondary_value =
      dotfile_scope_.GetValue("secondary_source", true);
  if (secondary_value) {
    if (!secondary_value->VerifyTypeIs(Value::STRING, err)) {
      return false;
    }
    build_settings_.SetSecondarySourcePath(
        SourceDir(secondary_value->string_value()));
  }

  // Build file names.
  const Value* build_file_extension_value =
      dotfile_scope_.GetValue("build_file_extension", true);
  if (build_file_extension_value) {
    if (!build_file_extension_value->VerifyTypeIs(Value::STRING, err)) {
      return false;
    }

    std::string extension = build_file_extension_value->string_value();
    auto normalized_extension = UTF8ToFilePath(extension).value();
    if (normalized_extension.find_first_of(base::FilePath::kSeparators) !=
        base::FilePath::StringType::npos) {
      *err = Err(Location(), "Build file extension '" + extension +
                                 "' cannot " + "contain a path separator");
      return false;
    }
    loader_->set_build_file_extension(extension);
  }

  // Ninja required version.
  const Value* ninja_required_version_value =
      dotfile_scope_.GetValue("ninja_required_version", true);
  if (ninja_required_version_value) {
    if (!ninja_required_version_value->VerifyTypeIs(Value::STRING, err)) {
      return false;
    }
    std::optional<Version> version =
        Version::FromString(ninja_required_version_value->string_value());
    if (!version) {
      Err(Location(), "Invalid Ninja version '" +
                          ninja_required_version_value->string_value() + "'")
          .PrintToStdout();
      return false;
    }
    build_settings_.set_ninja_required_version(*version);
  }

  // Root build file.
  if (cmdline.HasSwitch(switches::kRootTarget)) {
    auto switch_value = cmdline.GetSwitchValueString(switches::kRootTarget);
    Value root_value(nullptr, switch_value);
    root_target_label = Label::Resolve(current_dir, std::string_view(), Label(),
                                       root_value, err);
    if (err->has_error()) {
      return false;
    }
    if (dotfile_scope_.GetValue("root", true)) {
      // The "kRootTarget" switch overwrites the "root" variable in ".gn".
      dotfile_scope_.MarkUsed("root");
    }
  } else {
    const Value* root_value = dotfile_scope_.GetValue("root", true);
    if (root_value) {
      if (!root_value->VerifyTypeIs(Value::STRING, err)) {
        return false;
      }

      root_target_label = Label::Resolve(current_dir, std::string_view(),
                                         Label(), *root_value, err);
      if (err->has_error()) {
        return false;
      }
    }
  }
  // Set the root build file here in order to take into account the values of
  // "build_file_extension" and "root".
  root_build_file_ = loader_->BuildFileForLabel(root_target_label);
  build_settings_.SetRootTargetLabel(root_target_label);

  // Build config file.
  const Value* build_config_value =
      dotfile_scope_.GetValue("buildconfig", true);
  if (!build_config_value) {
    Err(Location(), "No build config file.",
        "Your .gn file (\"" + FilePathToUTF8(dotfile_name_) +
            "\")\n"
            "didn't specify a \"buildconfig\" value.")
        .PrintToStdout();
    return false;
  } else if (!build_config_value->VerifyTypeIs(Value::STRING, err)) {
    return false;
  }
  build_settings_.set_build_config_file(
      SourceFile(build_config_value->string_value()));

  // Targets to check.
  const Value* check_targets_value =
      dotfile_scope_.GetValue("check_targets", true);
  if (check_targets_value) {
    check_patterns_ = std::make_unique<std::vector<LabelPattern>>();
    ExtractListOfLabelPatterns(&build_settings_, *check_targets_value,
                               current_dir, check_patterns_.get(), err);
    if (err->has_error()) {
      return false;
    }
  }

  // Targets not to check.
  const Value* no_check_targets_value =
      dotfile_scope_.GetValue("no_check_targets", true);
  if (no_check_targets_value) {
    if (check_targets_value) {
      Err(Location(), "Conflicting check settings.",
          "Your .gn file (\"" + FilePathToUTF8(dotfile_name_) +
              "\")\n"
              "specified both check_targets and no_check_targets and at most "
              "one is allowed.")
          .PrintToStdout();
      return false;
    }
    no_check_patterns_ = std::make_unique<std::vector<LabelPattern>>();
    ExtractListOfLabelPatterns(&build_settings_, *no_check_targets_value,
                               current_dir, no_check_patterns_.get(), err);
    if (err->has_error()) {
      return false;
    }
  }

  const Value* check_system_includes_value =
      dotfile_scope_.GetValue("check_system_includes", true);
  if (check_system_includes_value) {
    if (!check_system_includes_value->VerifyTypeIs(Value::BOOLEAN, err)) {
      return false;
    }
    check_system_includes_ = check_system_includes_value->boolean_value();
  }

  // Fill exec_script_whitelist.
  const Value* exec_script_whitelist_value =
      dotfile_scope_.GetValue("exec_script_whitelist", true);
  if (exec_script_whitelist_value) {
    // Fill the list of targets to check.
    if (!exec_script_whitelist_value->VerifyTypeIs(Value::LIST, err)) {
      return false;
    }
    std::unique_ptr<SourceFileSet> whitelist =
        std::make_unique<SourceFileSet>();
    for (const auto& item : exec_script_whitelist_value->list_value()) {
      if (!item.VerifyTypeIs(Value::STRING, err)) {
        return false;
      }
      whitelist->insert(current_dir.ResolveRelativeFile(item, err));
      if (err->has_error()) {
        return false;
      }
    }
    build_settings_.set_exec_script_whitelist(std::move(whitelist));
  }

  // Fill optional default_args.
  const Value* default_args_value =
      dotfile_scope_.GetValue("default_args", true);
  if (default_args_value) {
    if (!default_args_value->VerifyTypeIs(Value::SCOPE, err)) {
      return false;
    }

    default_args_ = default_args_value->scope_value();
  }

  const Value* arg_file_template_value =
      dotfile_scope_.GetValue("arg_file_template", true);
  if (arg_file_template_value) {
    if (!arg_file_template_value->VerifyTypeIs(Value::STRING, err)) {
      return false;
    }
    SourceFile path(arg_file_template_value->string_value());
    build_settings_.set_arg_file_template_path(path);
  }

  // No stamp files.
  const Value* no_stamp_files_value =
      dotfile_scope_.GetValue("no_stamp_files", true);
  if (no_stamp_files_value) {
    if (!no_stamp_files_value->VerifyTypeIs(Value::BOOLEAN, err)) {
      return false;
    }
    build_settings_.set_no_stamp_files(no_stamp_files_value->boolean_value());
    CHECK(!build_settings_.no_stamp_files())
        << "no_stamp_files does not work yet!";
  }

  // Export compile commands.
  const Value* export_cc_value =
      dotfile_scope_.GetValue("export_compile_commands", true);
  if (export_cc_value) {
    if (!ExtractListOfLabelPatterns(&build_settings_, *export_cc_value,
                                    SourceDir("//"), &export_compile_commands_,
                                    err)) {
      return false;
    }
  }

  // Append any additional export compile command patterns from the cmdline.
  for (const std::string& cur :
       cmdline.GetSwitchValueStrings(switches::kAddExportCompileCommands)) {
    LabelPattern pat = LabelPattern::GetPattern(
        SourceDir("//"), build_settings_.root_path_utf8(), Value(nullptr, cur),
        err);
    if (err->has_error()) {
      err->AppendSubErr(Err(
          Location(),
          "for the command-line switch --add-export-compile-commands=" + cur));
      return false;
    }
    export_compile_commands_.push_back(std::move(pat));
  }

  return true;
}
