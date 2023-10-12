// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_COMMANDS_H_
#define TOOLS_GN_COMMANDS_H_

#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "base/values.h"
#include "gn/target.h"
#include "gn/unique_vector.h"

class BuildSettings;
class Config;
class LabelPattern;
class Setup;
class SourceFile;
class Target;
class Toolchain;

namespace base {
class CommandLine;
}  // namespace base

// Each "Run" command returns the value we should return from main().

namespace commands {

using CommandRunner = int (*)(const std::vector<std::string>&);

extern const char kAnalyze[];
extern const char kAnalyze_HelpShort[];
extern const char kAnalyze_Help[];
int RunAnalyze(const std::vector<std::string>& args);

extern const char kArgs[];
extern const char kArgs_HelpShort[];
extern const char kArgs_Help[];
int RunArgs(const std::vector<std::string>& args);

extern const char kCheck[];
extern const char kCheck_HelpShort[];
extern const char kCheck_Help[];
int RunCheck(const std::vector<std::string>& args);

extern const char kClean[];
extern const char kClean_HelpShort[];
extern const char kClean_Help[];
int RunClean(const std::vector<std::string>& args);

extern const char kDesc[];
extern const char kDesc_HelpShort[];
extern const char kDesc_Help[];
int RunDesc(const std::vector<std::string>& args);

extern const char kGen[];
extern const char kGen_HelpShort[];
extern const char kGen_Help[];
int RunGen(const std::vector<std::string>& args);

extern const char kFormat[];
extern const char kFormat_HelpShort[];
extern const char kFormat_Help[];
int RunFormat(const std::vector<std::string>& args);

extern const char kHelp[];
extern const char kHelp_HelpShort[];
extern const char kHelp_Help[];
int RunHelp(const std::vector<std::string>& args);

extern const char kMeta[];
extern const char kMeta_HelpShort[];
extern const char kMeta_Help[];
int RunMeta(const std::vector<std::string>& args);

extern const char kLs[];
extern const char kLs_HelpShort[];
extern const char kLs_Help[];
int RunLs(const std::vector<std::string>& args);

extern const char kOutputs[];
extern const char kOutputs_HelpShort[];
extern const char kOutputs_Help[];
int RunOutputs(const std::vector<std::string>& args);

extern const char kPath[];
extern const char kPath_HelpShort[];
extern const char kPath_Help[];
int RunPath(const std::vector<std::string>& args);

extern const char kRefs[];
extern const char kRefs_HelpShort[];
extern const char kRefs_Help[];
int RunRefs(const std::vector<std::string>& args);

extern const char kCleanStale[];
extern const char kCleanStale_HelpShort[];
extern const char kCleanStale_Help[];
int RunCleanStale(const std::vector<std::string>& args);

// -----------------------------------------------------------------------------

struct CommandInfo {
  CommandInfo();
  CommandInfo(const char* in_help_short,
              const char* in_help,
              CommandRunner in_runner);

  const char* help_short;
  const char* help;
  CommandRunner runner;
};

using CommandInfoMap = std::map<std::string_view, CommandInfo>;

const CommandInfoMap& GetCommands();

// Command switches as flags and enums -----------------------------------------

// A class that models a set of command-line flags and values that
// can affect the output of various GN commands. For example --tree
// can be used with `gn desc <out_dir> <label> deps --tree`.
//
// Each flag or value is checked by an accessor method which returns
// a boolean or an enum.
//
// Use CommandSwitches::Get() to get a reference to the current
// global set of switches for the process.
//
// Use CommandSwitches::Set() to update its value. This may be
// useful when implementing a REPL in GN, where each evaluation
// might need a different set of command switches.
class CommandSwitches {
 public:
  // Default constructor.
  CommandSwitches() = default;

  // For --quiet, used by `refs`.
  bool has_quiet() const { return has_quiet_; }

  // For --force, used by `check`.
  bool has_force() const { return has_force_; }

  // For --all, used by `desc` and `refs`.
  bool has_all() const { return has_all_; }

  // For --blame used by `desc`.
  bool has_blame() const { return has_blame_; }

  // For --tree used by `desc` and `refs`.
  bool has_tree() const { return has_tree_; }

  // For --format=json used by `desc`.
  bool has_format_json() const { return has_format_json_; }

  // For --default-toolchain used by `desc`, `refs`.
  bool has_default_toolchain() const { return has_default_toolchain_; }

  // For --check-generated
  bool has_check_generated() const { return has_check_generated_; }

  // For --check-system
  bool has_check_system() const { return has_check_system_; }

  // For --public
  bool has_public() const { return has_public_; }

  // For --with-data
  bool has_with_data() const { return has_with_data_; }

  // For --as=(buildtype|label|output).
  enum TargetPrintMode {
    TARGET_PRINT_BUILDFILE,
    TARGET_PRINT_LABEL,
    TARGET_PRINT_OUTPUT,
  };
  TargetPrintMode target_print_mode() const { return target_print_mode_; }

  // For --type=TARGET_TYPE
  Target::OutputType target_type() const { return target_type_; }

  enum TestonlyMode {
    TESTONLY_NONE,   // no --testonly used.
    TESTONLY_FALSE,  // --testonly=false
    TESTONLY_TRUE,   // --testonly=true
  };
  TestonlyMode testonly_mode() const { return testonly_mode_; }

  // For --rebase, --walk and --data in `gn meta`
  std::string meta_rebase_dir() const { return meta_rebase_dir_; }
  std::string meta_data_keys() const { return meta_data_keys_; }
  std::string meta_walk_keys() const { return meta_walk_keys_; }

  // Initialize the global set from a given command line.
  // Must be called early from main(). On success return true,
  // on failure return false after printing an error message.
  static bool Init(const base::CommandLine& cmdline);

  // Retrieve a reference to the current global set of command switches.
  static const CommandSwitches& Get();

  // Change the current global set of command switches, and return
  // the previous value.
  static CommandSwitches Set(CommandSwitches new_switches);

 private:
  bool is_initialized() const { return initialized_; }

  bool InitFrom(const base::CommandLine&);

  static CommandSwitches s_global_switches_;

  bool initialized_ = false;
  bool has_quiet_ = false;
  bool has_force_ = false;
  bool has_all_ = false;
  bool has_blame_ = false;
  bool has_tree_ = false;
  bool has_format_json_ = false;
  bool has_default_toolchain_ = false;
  bool has_check_generated_ = false;
  bool has_check_system_ = false;
  bool has_public_ = false;
  bool has_with_data_ = false;

  TargetPrintMode target_print_mode_ = TARGET_PRINT_LABEL;
  Target::OutputType target_type_ = Target::UNKNOWN;
  TestonlyMode testonly_mode_ = TESTONLY_NONE;

  std::string meta_rebase_dir_;
  std::string meta_data_keys_;
  std::string meta_walk_keys_;
};

// Helper functions for some commands ------------------------------------------

// Modifies the existing build.ninja to only contain the commands necessary to
// run GN and regenerate and build.ninja.d such that build.ninja will be
// treated as dirty and regenerated.
//
// This is used by commands like gen and clean before they modify or delete
// other ninja files, and ensures that ninja can still call GN if the commands
// are interrupted before completion.
//
// On error, returns false.
bool PrepareForRegeneration(const BuildSettings* settings);

// Given a setup that has already been run and some command-line input,
// resolves that input as a target label and returns the corresponding target.
// On failure, returns null and prints the error to the standard output.
const Target* ResolveTargetFromCommandLineString(
    Setup* setup,
    const std::string& label_string);

// Resolves a vector of command line inputs and figures out the full set of
// things they resolve to.
//
// On success, returns true and populates the vectors. On failure, prints the
// error and returns false.
//
// Patterns with wildcards will only match targets. The file_matches aren't
// validated that they are real files or referenced by any targets. They're just
// the set of things that didn't match anything else.
bool ResolveFromCommandLineInput(
    Setup* setup,
    const std::vector<std::string>& input,
    bool default_toolchain_only,
    UniqueVector<const Target*>* target_matches,
    UniqueVector<const Config*>* config_matches,
    UniqueVector<const Toolchain*>* toolchain_matches,
    UniqueVector<SourceFile>* file_matches);

// Runs the header checker. All targets in the build should be given in
// all_targets, and the specific targets to check should be in to_check.
//
// force_check, if true, will override targets opting out of header checking
// with "check_includes = false" and will check them anyway.
//
// Generated files are normally not checked since they do not exist
// unless a build has been run, but passing true for |check_generated|
// will attempt to check them anyway, assuming they exist.
//
// On success, returns true. If the check fails, the error(s) will be printed
// to stdout and false will be returned.
bool CheckPublicHeaders(const BuildSettings* build_settings,
                        const std::vector<const Target*>& all_targets,
                        const std::vector<const Target*>& to_check,
                        bool force_check,
                        bool check_generated,
                        bool check_system);

// Filters the given list of targets by the given pattern list.
void FilterTargetsByPatterns(const std::vector<const Target*>& input,
                             const std::vector<LabelPattern>& filter,
                             std::vector<const Target*>* output);
void FilterTargetsByPatterns(const std::vector<const Target*>& input,
                             const std::vector<LabelPattern>& filter,
                             UniqueVector<const Target*>* output);

// Removes targets from the input that match the given pattern list.
void FilterOutTargetsByPatterns(const std::vector<const Target*>& input,
                                const std::vector<LabelPattern>& filter,
                                std::vector<const Target*>* output);

// Builds a list of pattern from a semicolon-separated list of labels.
bool FilterPatternsFromString(const BuildSettings* build_settings,
                              const std::string& label_list_string,
                              std::vector<LabelPattern>* filters,
                              Err* err);

// These are the documentation strings for the command-line flags used by
// FilterAndPrintTargets. Commands that call that function should incorporate
// these into their help.
#define TARGET_PRINTING_MODE_COMMAND_LINE_HELP                                \
  "  --as=(buildfile|label|output)\n"                                         \
  "      How to print targets.\n"                                             \
  "\n"                                                                        \
  "      buildfile\n"                                                         \
  "          Prints the build files where the given target was declared as\n" \
  "          file names.\n"                                                   \
  "      label  (default)\n"                                                  \
  "          Prints the label of the target.\n"                               \
  "      output\n"                                                            \
  "          Prints the first output file for the target relative to the\n"   \
  "          root build directory.\n"
#define TARGET_TYPE_FILTER_COMMAND_LINE_HELP                                 \
  "  --type=(action|copy|executable|group|loadable_module|shared_library|\n" \
  "          source_set|static_library)\n"                                   \
  "      Restrict outputs to targets matching the given type. If\n"          \
  "      unspecified, no filtering will be performed.\n"
#define TARGET_TESTONLY_FILTER_COMMAND_LINE_HELP                           \
  "  --testonly=(true|false)\n"                                            \
  "      Restrict outputs to targets with the testonly flag set\n"         \
  "      accordingly. When unspecified, the target's testonly flags are\n" \
  "      ignored.\n"

// Applies any testonly and type filters specified on the command line,
// and prints the targets as specified by the --as command line flag.
//
// If indent is true, the results will be indented two spaces.
//
// The vector will be modified so that only the printed targets will remain.
void FilterAndPrintTargets(bool indent, std::vector<const Target*>* targets);
void FilterAndPrintTargets(std::vector<const Target*>* targets,
                           base::ListValue* out);

void FilterAndPrintTargetSet(bool indent, const TargetSet& targets);
void FilterAndPrintTargetSet(const TargetSet& targets, base::ListValue* out);

// Computes which targets reference the given file and also stores how the
// target references the file.
enum class HowTargetContainsFile {
  kSources,
  kPublic,
  kInputs,
  kData,
  kScript,
  kOutput,
};
using TargetContainingFile = std::pair<const Target*, HowTargetContainsFile>;
void GetTargetsContainingFile(Setup* setup,
                              const std::vector<const Target*>& all_targets,
                              const SourceFile& file,
                              bool default_toolchain_only,
                              std::vector<TargetContainingFile>* matches);

// Extra help from command_check.cc
extern const char kNoGnCheck_Help[];

}  // namespace commands

#endif  // TOOLS_GN_COMMANDS_H_
