// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/commands.h"

#include <fstream>
#include <optional>

#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "gn/builder.h"
#include "gn/config_values_extractors.h"
#include "gn/filesystem_utils.h"
#include "gn/item.h"
#include "gn/label.h"
#include "gn/label_pattern.h"
#include "gn/ninja_build_writer.h"
#include "gn/setup.h"
#include "gn/standard_out.h"
#include "gn/switches.h"
#include "gn/target.h"
#include "util/atomic_write.h"
#include "util/build_config.h"

namespace commands {

namespace {

// Like above but the input string can be a pattern that matches multiple
// targets. If the input does not parse as a pattern, prints and error and
// returns false. If the pattern is valid, fills the vector (which might be
// empty if there are no matches) and returns true.
//
// If default_toolchain_only is true, a pattern with an unspecified toolchain
// will match the default toolchain only. If true, all toolchains will be
// matched.
bool ResolveTargetsFromCommandLinePattern(Setup* setup,
                                          const std::string& label_pattern,
                                          bool default_toolchain_only,
                                          std::vector<const Target*>* matches) {
  Value pattern_value(nullptr, label_pattern);

  Err err;
  LabelPattern pattern = LabelPattern::GetPattern(
      SourceDirForCurrentDirectory(setup->build_settings().root_path()),
      setup->build_settings().root_path_utf8(), pattern_value, &err);
  if (err.has_error()) {
    err.PrintToStdout();
    return false;
  }

  if (default_toolchain_only) {
    // By default a pattern with an empty toolchain will match all toolchains.
    // If the caller wants to default to the main toolchain only, set it
    // explicitly.
    if (pattern.toolchain().is_null()) {
      // No explicit toolchain set.
      pattern.set_toolchain(setup->loader()->default_toolchain_label());
    }
  }

  std::vector<LabelPattern> pattern_vector;
  pattern_vector.push_back(pattern);
  FilterTargetsByPatterns(setup->builder().GetAllResolvedTargets(),
                          pattern_vector, matches);
  return true;
}

// If there's an error, it will be printed and false will be returned.
bool ResolveStringFromCommandLineInput(
    Setup* setup,
    const SourceDir& current_dir,
    const std::string& input,
    bool default_toolchain_only,
    UniqueVector<const Target*>* target_matches,
    UniqueVector<const Config*>* config_matches,
    UniqueVector<const Toolchain*>* toolchain_matches,
    UniqueVector<SourceFile>* file_matches) {
  if (LabelPattern::HasWildcard(input)) {
    // For now, only match patterns against targets. It might be nice in the
    // future to allow the user to specify which types of things they want to
    // match, but it should probably only match targets by default.
    std::vector<const Target*> target_match_vector;
    if (!ResolveTargetsFromCommandLinePattern(
            setup, input, default_toolchain_only, &target_match_vector))
      return false;
    for (const Target* target : target_match_vector)
      target_matches->push_back(target);
    return true;
  }

  // Try to figure out what this thing is.
  Err err;
  Label label = Label::Resolve(
      current_dir, setup->build_settings().root_path_utf8(),
      setup->loader()->default_toolchain_label(), Value(nullptr, input), &err);
  if (err.has_error()) {
    // Not a valid label, assume this must be a file.
    err = Err();
    file_matches->push_back(current_dir.ResolveRelativeFile(
        Value(nullptr, input), &err, setup->build_settings().root_path_utf8()));
    if (err.has_error()) {
      err.PrintToStdout();
      return false;
    }
    return true;
  }

  const Item* item = setup->builder().GetItem(label);
  if (item) {
    if (const Config* as_config = item->AsConfig())
      config_matches->push_back(as_config);
    else if (const Target* as_target = item->AsTarget())
      target_matches->push_back(as_target);
    else if (const Toolchain* as_toolchain = item->AsToolchain())
      toolchain_matches->push_back(as_toolchain);
  } else {
    // Not an item, assume this must be a file.
    file_matches->push_back(current_dir.ResolveRelativeFile(
        Value(nullptr, input), &err, setup->build_settings().root_path_utf8()));
    if (err.has_error()) {
      err.PrintToStdout();
      return false;
    }
  }

  return true;
}

// Retrieves the target printing mode based on the command line flags for the
// current process. Returns true on success. On error, prints a message to the
// console and returns false.
bool GetTargetPrintingMode(CommandSwitches::TargetPrintMode* mode) {
  *mode = CommandSwitches::Get().target_print_mode();
  return true;
}

// Returns the target type filter based on the command line flags for the
// current process. Returns true on success. On error, prints a message to the
// console and returns false.
//
// Target::UNKNOWN will be set if there is no filter. Target::ACTION_FOREACH
// will never be returned. Code applying the filters should apply Target::ACTION
// to both ACTION and ACTION_FOREACH.
bool GetTargetTypeFilter(Target::OutputType* type) {
  *type = CommandSwitches::Get().target_type();
  return true;
}

// Applies any testonly filtering specified on the command line to the given
// target set. On failure, prints an error and returns false.
bool ApplyTestonlyFilter(std::vector<const Target*>* targets) {
  CommandSwitches::TestonlyMode testonly_mode =
      CommandSwitches::Get().testonly_mode();

  if (targets->empty() || testonly_mode == CommandSwitches::TESTONLY_NONE)
    return true;

  bool testonly = (testonly_mode == CommandSwitches::TESTONLY_TRUE);

  // Filter into a copy of the vector, then replace the output.
  std::vector<const Target*> result;
  result.reserve(targets->size());

  for (const Target* target : *targets) {
    if (target->testonly() == testonly)
      result.push_back(target);
  }

  *targets = std::move(result);
  return true;
}

// Applies any target type filtering specified on the command line to the given
// target set. On failure, prints an error and returns false.
bool ApplyTypeFilter(std::vector<const Target*>* targets) {
  Target::OutputType type = Target::UNKNOWN;
  if (!GetTargetTypeFilter(&type))
    return false;
  if (targets->empty() || type == Target::UNKNOWN)
    return true;  // Nothing to filter out.

  // Filter into a copy of the vector, then replace the output.
  std::vector<const Target*> result;
  result.reserve(targets->size());

  for (const Target* target : *targets) {
    // Make "action" also apply to ACTION_FOREACH.
    if (target->output_type() == type ||
        (type == Target::ACTION &&
         target->output_type() == Target::ACTION_FOREACH))
      result.push_back(target);
  }

  *targets = std::move(result);
  return true;
}

// Returns the file path generating this item.
base::FilePath BuildFileForItem(const Item* item) {
  return item->defined_from()->GetRange().begin().file()->physical_name();
}

void PrintTargetsAsBuildfiles(const std::vector<const Target*>& targets,
                              base::ListValue* out) {
  // Output the set of unique source files.
  std::set<std::string> unique_files;
  for (const Target* target : targets)
    unique_files.insert(FilePathToUTF8(BuildFileForItem(target)));

  for (const std::string& file : unique_files) {
    out->AppendString(file);
  }
}

void PrintTargetsAsLabels(const std::vector<const Target*>& targets,
                          base::ListValue* out) {
  // Putting the labels into a set automatically sorts them for us.
  std::set<Label> unique_labels;
  for (auto* target : targets)
    unique_labels.insert(target->label());

  // Grab the label of the default toolchain from the first target.
  Label default_tc_label = targets[0]->settings()->default_toolchain_label();

  for (const Label& label : unique_labels) {
    // Print toolchain only for ones not in the default toolchain.
    out->AppendString(label.GetUserVisibleName(label.GetToolchainLabel() !=
                                               default_tc_label));
  }
}

void PrintTargetsAsOutputs(const std::vector<const Target*>& targets,
                           base::ListValue* out) {
  if (targets.empty())
    return;

  // Grab the build settings from a random target.
  const BuildSettings* build_settings =
      targets[0]->settings()->build_settings();

  for (const Target* target : targets) {
    // Use the link output file if there is one, otherwise fall back to the
    // dependency output file (for actions, for example).
    OutputFile output_file = target->link_output_file();
    if (output_file.value().empty())
      output_file = target->dependency_output_file();

    SourceFile output_as_source = output_file.AsSourceFile(build_settings);
    std::string result =
        RebasePath(output_as_source.value(), build_settings->build_dir(),
                   build_settings->root_path_utf8());
    out->AppendString(result);
  }
}

#if defined(OS_WIN)
// Git bash will remove the first "/" in "//" paths
// This also happens for labels assigned to command line parameters, e.g.
// --filters
// Fix "//" paths, but not absolute and relative paths
inline std::string FixGitBashLabelEdit(const std::string& label) {
  static std::unique_ptr<base::Environment> git_bash_env;
  if (!git_bash_env)
    git_bash_env = base::Environment::Create();

  std::string temp_label(label);

  if (git_bash_env->HasVar(
          "MSYSTEM") &&        // Only for MinGW based shells like Git Bash
      temp_label[0] == '/' &&  // Only fix for //foo paths, not /f:oo paths
      (temp_label.length() < 2 ||
       (temp_label[1] != '/' &&
        (temp_label.length() < 3 || temp_label[1] != ':'))))
    temp_label.insert(0, "/");
  return temp_label;
}
#else
// Only repair on Windows
inline std::string FixGitBashLabelEdit(const std::string& label) {
  return label;
}
#endif

std::optional<HowTargetContainsFile> TargetContainsFile(
    const Target* target,
    const SourceFile& file) {
  for (const auto& cur_file : target->sources()) {
    if (cur_file == file)
      return HowTargetContainsFile::kSources;
  }
  for (const auto& cur_file : target->public_headers()) {
    if (cur_file == file)
      return HowTargetContainsFile::kPublic;
  }
  for (ConfigValuesIterator iter(target); !iter.done(); iter.Next()) {
    for (const auto& cur_file : iter.cur().inputs()) {
      if (cur_file == file)
        return HowTargetContainsFile::kInputs;
    }
  }
  for (const auto& cur_file : target->data()) {
    if (cur_file == file.value())
      return HowTargetContainsFile::kData;
    if (cur_file.back() == '/' &&
        base::StartsWith(file.value(), cur_file, base::CompareCase::SENSITIVE))
      return HowTargetContainsFile::kData;
  }

  if (target->action_values().script().value() == file.value())
    return HowTargetContainsFile::kScript;

  std::vector<SourceFile> output_sources;
  target->action_values().GetOutputsAsSourceFiles(target, &output_sources);
  for (const auto& cur_file : output_sources) {
    if (cur_file == file)
      return HowTargetContainsFile::kOutput;
  }

  for (const auto& cur_file : target->computed_outputs()) {
    if (cur_file.AsSourceFile(target->settings()->build_settings()) == file)
      return HowTargetContainsFile::kOutput;
  }
  return std::nullopt;
}

std::string ToUTF8(base::FilePath::StringType in) {
#if defined(OS_WIN)
  return base::UTF16ToUTF8(in);
#else
  return in;
#endif
}

}  // namespace

CommandInfo::CommandInfo()
    : help_short(nullptr), help(nullptr), runner(nullptr) {}

CommandInfo::CommandInfo(const char* in_help_short,
                         const char* in_help,
                         CommandRunner in_runner)
    : help_short(in_help_short), help(in_help), runner(in_runner) {}

const CommandInfoMap& GetCommands() {
  static CommandInfoMap info_map;
  if (info_map.empty()) {
#define INSERT_COMMAND(cmd) \
  info_map[k##cmd] = CommandInfo(k##cmd##_HelpShort, k##cmd##_Help, &Run##cmd);

    INSERT_COMMAND(Analyze)
    INSERT_COMMAND(Args)
    INSERT_COMMAND(Check)
    INSERT_COMMAND(Clean)
    INSERT_COMMAND(Desc)
    INSERT_COMMAND(Gen)
    INSERT_COMMAND(Format)
    INSERT_COMMAND(Help)
    INSERT_COMMAND(Meta)
    INSERT_COMMAND(Ls)
    INSERT_COMMAND(Outputs)
    INSERT_COMMAND(Path)
    INSERT_COMMAND(Refs)
    INSERT_COMMAND(CleanStale)

#undef INSERT_COMMAND
  }
  return info_map;
}

// static
CommandSwitches CommandSwitches::s_global_switches_ = {};

// static
bool CommandSwitches::Init(const base::CommandLine& cmdline) {
  CHECK(!s_global_switches_.is_initialized())
      << "Only call this once from main()";
  return s_global_switches_.InitFrom(cmdline);
}

// static
const CommandSwitches& CommandSwitches::Get() {
  CHECK(s_global_switches_.is_initialized())
      << "Missing previous succesful call to CommandSwitches::Init()";
  return s_global_switches_;
}

// static
CommandSwitches CommandSwitches::Set(CommandSwitches new_switches) {
  CHECK(s_global_switches_.is_initialized())
      << "Missing previous succesful call to CommandSwitches::Init()";
  CommandSwitches result = std::move(s_global_switches_);
  s_global_switches_ = std::move(new_switches);
  return result;
}

bool CommandSwitches::InitFrom(const base::CommandLine& cmdline) {
  CommandSwitches result;
  result.initialized_ = true;
  result.has_quiet_ = cmdline.HasSwitch("a");
  result.has_force_ = cmdline.HasSwitch("force");
  result.has_all_ = cmdline.HasSwitch("all");
  result.has_blame_ = cmdline.HasSwitch("blame");
  result.has_tree_ = cmdline.HasSwitch("tree");
  result.has_format_json_ = cmdline.GetSwitchValueString("format") == "json";
  result.has_default_toolchain_ =
      cmdline.HasSwitch(switches::kDefaultToolchain);

  result.has_check_generated_ = cmdline.HasSwitch("check-generated");
  result.has_check_system_ = cmdline.HasSwitch("check-system");
  result.has_public_ = cmdline.HasSwitch("public");
  result.has_with_data_ = cmdline.HasSwitch("with-data");

  std::string_view target_print_switch = "as";
  if (cmdline.HasSwitch(target_print_switch)) {
    std::string value = cmdline.GetSwitchValueString(target_print_switch);
    if (value == "buildfile") {
      result.target_print_mode_ = TARGET_PRINT_BUILDFILE;
    } else if (value == "label") {
      result.target_print_mode_ = TARGET_PRINT_LABEL;
    } else if (value == "output") {
      result.target_print_mode_ = TARGET_PRINT_OUTPUT;
    } else {
      Err(Location(), "Invalid value for \"--as\".",
          "I was expecting \"buildfile\", \"label\", or \"output\" but you\n"
          "said \"" +
              value + "\".")
          .PrintToStdout();
      return false;
    }
  }

  std::string_view target_type_switch = "type";
  if (cmdline.HasSwitch(target_type_switch)) {
    std::string value = cmdline.GetSwitchValueString(target_type_switch);
    static const struct {
      const char* name;
      Target::OutputType type;
    } kTypes[] = {
        {"group", Target::GROUP},
        {"executable", Target::EXECUTABLE},
        {"shared_library", Target::SHARED_LIBRARY},
        {"loadable_module", Target::LOADABLE_MODULE},
        {"static_library", Target::STATIC_LIBRARY},
        {"source_set", Target::SOURCE_SET},
        {"copy", Target::COPY_FILES},
        {"action", Target::ACTION},
    };
    bool found = false;
    for (const auto& type : kTypes) {
      if (value == type.name) {
        result.target_type_ = type.type;
        found = true;
        break;
      }
    }
    if (!found) {
      Err(Location(), "Invalid value for \"--type\".").PrintToStdout();
      return false;
    }
  }
  std::string_view testonly_switch = "testonly";
  if (cmdline.HasSwitch(testonly_switch)) {
    std::string value = cmdline.GetSwitchValueString(testonly_switch);
    if (value == "true") {
      result.testonly_mode_ = TESTONLY_TRUE;
    } else if (value == "false") {
      result.testonly_mode_ = TESTONLY_FALSE;
    } else {
      Err(Location(), "Bad value for --testonly.",
          "I was expecting --testonly=true or --testonly=false.")
          .PrintToStdout();
      return false;
    }
  }

  result.meta_rebase_dir_ = cmdline.GetSwitchValueString("rebase");
  result.meta_data_keys_ = cmdline.GetSwitchValueString("data");
  result.meta_walk_keys_ = cmdline.GetSwitchValueString("walk");
  *this = result;
  return true;
}

bool PrepareForRegeneration(const BuildSettings* settings) {
  // Write a .d file for the build which references a nonexistent file.
  // This will make Ninja always mark the build as dirty.
  base::FilePath build_ninja_d_file(settings->GetFullPath(
      SourceFile(settings->build_dir().value() + "build.ninja.d")));
  std::string dummy_depfile("build.ninja.stamp: nonexistent_file.gn\n");
  if (util::WriteFileAtomically(build_ninja_d_file, dummy_depfile.data(),
                                static_cast<int>(dummy_depfile.size())) == -1) {
    Err(Location(), std::string("Failed to write build.ninja.d."))
        .PrintToStdout();
    return false;
  }

  // Write a stripped down build.ninja file with just the commands needed
  // for ninja to call GN and regenerate ninja files.
  base::FilePath build_ninja_path(settings->GetFullPath(
      SourceFile(settings->build_dir().value() + "build.ninja")));
  std::ifstream build_ninja_file(ToUTF8(build_ninja_path.value()));
  if (!build_ninja_file) {
    // Couldn't open the build.ninja file.
    Err(Location(), "Couldn't open build.ninja in this directory.",
        "Try running \"gn gen\" on it and then re-running \"gn clean\".")
        .PrintToStdout();
    return false;
  }
  std::string build_commands =
      NinjaBuildWriter::ExtractRegenerationCommands(build_ninja_file);
  if (build_commands.empty()) {
    Err(Location(), "Unexpected build.ninja contents in this directory.",
        "Try running \"gn gen\" on it and then re-running \"gn clean\".")
        .PrintToStdout();
    return false;
  }
  // Close build.ninja or else WriteFileAtomically will fail on Windows.
  build_ninja_file.close();
  if (util::WriteFileAtomically(build_ninja_path, build_commands.data(),
                                static_cast<int>(build_commands.size())) ==
      -1) {
    Err(Location(), std::string("Failed to write build.ninja."))
        .PrintToStdout();
    return false;
  }

  return true;
}

const Target* ResolveTargetFromCommandLineString(
    Setup* setup,
    const std::string& label_string) {
  // Need to resolve the label after we know the default toolchain.
  Label default_toolchain = setup->loader()->default_toolchain_label();
  Value arg_value(nullptr, FixGitBashLabelEdit(label_string));
  Err err;
  Label label = Label::Resolve(
      SourceDirForCurrentDirectory(setup->build_settings().root_path()),
      setup->build_settings().root_path_utf8(), default_toolchain, arg_value,
      &err);
  if (err.has_error()) {
    err.PrintToStdout();
    return nullptr;
  }

  const Item* item = setup->builder().GetItem(label);
  if (!item) {
    Err(Location(), "Label not found.",
        label.GetUserVisibleName(false) + " not found.")
        .PrintToStdout();
    return nullptr;
  }

  const Target* target = item->AsTarget();
  if (!target) {
    Err(Location(), "Not a target.",
        "The \"" + label.GetUserVisibleName(false) +
            "\" thing\n"
            "is not a target. Somebody should probably implement this command "
            "for "
            "other\nitem types.")
        .PrintToStdout();
    return nullptr;
  }

  return target;
}

bool ResolveFromCommandLineInput(
    Setup* setup,
    const std::vector<std::string>& input,
    bool default_toolchain_only,
    UniqueVector<const Target*>* target_matches,
    UniqueVector<const Config*>* config_matches,
    UniqueVector<const Toolchain*>* toolchain_matches,
    UniqueVector<SourceFile>* file_matches) {
  if (input.empty()) {
    Err(Location(), "You need to specify a label, file, or pattern.")
        .PrintToStdout();
    return false;
  }

  SourceDir cur_dir =
      SourceDirForCurrentDirectory(setup->build_settings().root_path());
  for (const auto& cur : input) {
    if (!ResolveStringFromCommandLineInput(
            setup, cur_dir, cur, default_toolchain_only, target_matches,
            config_matches, toolchain_matches, file_matches))
      return false;
  }
  return true;
}

void FilterTargetsByPatterns(const std::vector<const Target*>& input,
                             const std::vector<LabelPattern>& filter,
                             std::vector<const Target*>* output) {
  for (auto* target : input) {
    for (const auto& pattern : filter) {
      if (pattern.Matches(target->label())) {
        output->push_back(target);
        break;
      }
    }
  }
}

void FilterTargetsByPatterns(const std::vector<const Target*>& input,
                             const std::vector<LabelPattern>& filter,
                             UniqueVector<const Target*>* output) {
  for (auto* target : input) {
    for (const auto& pattern : filter) {
      if (pattern.Matches(target->label())) {
        output->push_back(target);
        break;
      }
    }
  }
}

void FilterOutTargetsByPatterns(const std::vector<const Target*>& input,
                                const std::vector<LabelPattern>& filter,
                                std::vector<const Target*>* output) {
  for (auto* target : input) {
    bool match = false;
    for (const auto& pattern : filter) {
      if (pattern.Matches(target->label())) {
        match = true;
        break;
      }
    }
    if (!match) {
      output->push_back(target);
    }
  }
}

bool FilterPatternsFromString(const BuildSettings* build_settings,
                              const std::string& label_list_string,
                              std::vector<LabelPattern>* filters,
                              Err* err) {
  std::vector<std::string> tokens = base::SplitString(
      label_list_string, ";", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  SourceDir root_dir("//");

  filters->reserve(tokens.size());
  for (const std::string& token : tokens) {
    LabelPattern pattern = LabelPattern::GetPattern(
        root_dir, build_settings->root_path_utf8(),
        Value(nullptr, FixGitBashLabelEdit(token)), err);
    if (err->has_error())
      return false;
    filters->push_back(pattern);
  }

  return true;
}

void FilterAndPrintTargets(std::vector<const Target*>* targets,
                           base::ListValue* out) {
  if (targets->empty())
    return;

  if (!ApplyTestonlyFilter(targets))
    return;
  if (!ApplyTypeFilter(targets))
    return;

  CommandSwitches::TargetPrintMode printing_mode =
      CommandSwitches::TARGET_PRINT_LABEL;
  if (targets->empty() || !GetTargetPrintingMode(&printing_mode))
    return;
  switch (printing_mode) {
    case CommandSwitches::TARGET_PRINT_BUILDFILE:
      PrintTargetsAsBuildfiles(*targets, out);
      break;
    case CommandSwitches::TARGET_PRINT_LABEL:
      PrintTargetsAsLabels(*targets, out);
      break;
    case CommandSwitches::TARGET_PRINT_OUTPUT:
      PrintTargetsAsOutputs(*targets, out);
      break;
  }
}

void FilterAndPrintTargets(bool indent, std::vector<const Target*>* targets) {
  base::ListValue tmp;
  FilterAndPrintTargets(targets, &tmp);
  for (const auto& value : tmp) {
    std::string string;
    value.GetAsString(&string);
    if (indent)
      OutputString("  ");
    OutputString(string);
    OutputString("\n");
  }
}

void FilterAndPrintTargetSet(bool indent, const TargetSet& targets) {
  std::vector<const Target*> target_vector(targets.begin(), targets.end());
  FilterAndPrintTargets(indent, &target_vector);
}

void FilterAndPrintTargetSet(const TargetSet& targets, base::ListValue* out) {
  std::vector<const Target*> target_vector(targets.begin(), targets.end());
  FilterAndPrintTargets(&target_vector, out);
}

void GetTargetsContainingFile(Setup* setup,
                              const std::vector<const Target*>& all_targets,
                              const SourceFile& file,
                              bool default_toolchain_only,
                              std::vector<TargetContainingFile>* matches) {
  Label default_toolchain = setup->loader()->default_toolchain_label();
  for (auto* target : all_targets) {
    if (default_toolchain_only) {
      // Only check targets in the default toolchain.
      if (target->label().GetToolchainLabel() != default_toolchain)
        continue;
    }
    if (auto how = TargetContainsFile(target, file))
      matches->emplace_back(target, *how);
  }
}

}  // namespace commands
