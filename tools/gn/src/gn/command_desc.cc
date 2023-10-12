// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <algorithm>
#include <memory>
#include <set>
#include <sstream>

#include "base/command_line.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "gn/commands.h"
#include "gn/config.h"
#include "gn/desc_builder.h"
#include "gn/rust_variables.h"
#include "gn/setup.h"
#include "gn/standard_out.h"
#include "gn/swift_variables.h"
#include "gn/switches.h"
#include "gn/target.h"
#include "gn/variables.h"

namespace commands {

namespace {

// Desc-specific command line switches.
const char kBlame[] = "blame";
const char kTree[] = "tree";
const char kAll[] = "all";

void PrintDictValue(const base::Value* value,
                    int indentLevel,
                    bool use_first_indent) {
  std::string indent(indentLevel * 2, ' ');
  const base::ListValue* list_value = nullptr;
  const base::DictionaryValue* dict_value = nullptr;
  std::string string_value;
  bool bool_value = false;
  int int_value = 0;
  if (use_first_indent)
    OutputString(indent);
  if (value->GetAsList(&list_value)) {
    OutputString("[\n");
    bool first = true;
    for (const auto& v : *list_value) {
      if (!first)
        OutputString(",\n");
      PrintDictValue(&v, indentLevel + 1, true);
      first = false;
    }
    OutputString("\n" + indent + "]");
  } else if (value->GetAsString(&string_value)) {
    OutputString("\"" + string_value + "\"");
  } else if (value->GetAsBoolean(&bool_value)) {
    OutputString(bool_value ? "true" : "false");
  } else if (value->GetAsDictionary(&dict_value)) {
    OutputString("{\n");
    std::string indent_plus_one((indentLevel + 1) * 2, ' ');
    base::DictionaryValue::Iterator iter(*dict_value);
    bool first = true;
    while (!iter.IsAtEnd()) {
      if (!first)
        OutputString(",\n");
      OutputString(indent_plus_one + iter.key() + " = ");
      PrintDictValue(&iter.value(), indentLevel + 1, false);
      iter.Advance();
      first = false;
    }
    OutputString("\n" + indent + "}");
  } else if (value->GetAsInteger(&int_value)) {
    OutputString(base::IntToString(int_value));
  } else if (value->is_none()) {
    OutputString("<null>");
  }
}

// Prints value with specified indentation level
void PrintValue(const base::Value* value, int indentLevel) {
  std::string indent(indentLevel * 2, ' ');
  const base::ListValue* list_value = nullptr;
  const base::DictionaryValue* dict_value = nullptr;
  std::string string_value;
  bool bool_value = false;
  int int_value = 0;
  if (value->GetAsList(&list_value)) {
    for (const auto& v : *list_value) {
      PrintValue(&v, indentLevel);
    }
  } else if (value->GetAsString(&string_value)) {
    OutputString(indent);
    OutputString(string_value);
    OutputString("\n");
  } else if (value->GetAsBoolean(&bool_value)) {
    OutputString(indent);
    OutputString(bool_value ? "true" : "false");
    OutputString("\n");
  } else if (value->GetAsDictionary(&dict_value)) {
    base::DictionaryValue::Iterator iter(*dict_value);
    while (!iter.IsAtEnd()) {
      OutputString(indent + iter.key() + "\n");
      PrintValue(&iter.value(), indentLevel + 1);
      iter.Advance();
    }
  } else if (value->GetAsInteger(&int_value)) {
    OutputString(indent);
    OutputString(base::IntToString(int_value));
    OutputString("\n");
  } else if (value->is_none()) {
    OutputString(indent + "<null>\n");
  }
}

// Default handler for property
void DefaultHandler(const std::string& name,
                    const base::Value* value,
                    bool value_only) {
  if (value_only) {
    PrintValue(value, 0);
    return;
  }
  OutputString("\n");
  OutputString(name);
  OutputString("\n");
  PrintValue(value, 1);
}

// Specific handler for properties that need different treatment

// Prints the dict in GN scope-sytle.
void MetadataHandler(const std::string& name,
                     const base::Value* value,
                     bool value_only) {
  if (value_only) {
    PrintDictValue(value, 0, true);
    OutputString("\n");
    return;
  }
  OutputString("\n");
  OutputString(name);
  OutputString("\n");
  PrintDictValue(value, 1, true);
  OutputString("\n");
}

// Prints label and property value on one line, capitalizing the label.
void LabelHandler(const std::string& name,
                  const base::Value* value,
                  bool value_only) {
  if (value_only) {
    PrintValue(value, 0);
    return;
  }
  std::string label = name;
  label[0] = base::ToUpperASCII(label[0]);
  std::string string_value;
  if (value->GetAsString(&string_value)) {
    OutputString(name + ": ", DECORATION_YELLOW);
    OutputString(string_value + "\n");
  }
}

void VisibilityHandler(const std::string& name,
                       const base::Value* value,
                       bool value_only) {
  if (value_only) {
    PrintValue(value, 0);
    return;
  }
  const base::ListValue* list;
  if (value->GetAsList(&list)) {
    if (list->empty()) {
      base::Value str("(no visibility)");
      DefaultHandler(name, &str, value_only);
    } else {
      DefaultHandler(name, value, value_only);
    }
  }
}

void PublicHandler(const std::string& name,
                   const base::Value* value,
                   bool value_only) {
  if (value_only) {
    PrintValue(value, 0);
    return;
  }
  std::string p;
  if (value->GetAsString(&p)) {
    if (p == "*") {
      base::Value str("[All headers listed in the sources are public.]");
      DefaultHandler(name, &str, value_only);
      return;
    }
  }
  DefaultHandler(name, value, value_only);
}

void ConfigsHandler(const std::string& name,
                    const base::Value* value,
                    bool value_only) {
  bool tree = base::CommandLine::ForCurrentProcess()->HasSwitch(kTree);
  if (tree)
    DefaultHandler(name + " tree (in order applying)", value, value_only);
  else
    DefaultHandler(name + " (in order applying, try also --tree)", value,
                   value_only);
}

void DepsHandler(const std::string& name,
                 const base::Value* value,
                 bool value_only) {
  bool tree = base::CommandLine::ForCurrentProcess()->HasSwitch(kTree);
  bool all = base::CommandLine::ForCurrentProcess()->HasSwitch(kTree);
  if (tree) {
    DefaultHandler("Dependency tree", value, value_only);
  } else {
    if (!all) {
      DefaultHandler(
          "Direct dependencies "
          "(try also \"--all\", \"--tree\", or even \"--all --tree\")",
          value, value_only);
    } else {
      DefaultHandler("All recursive dependencies", value, value_only);
    }
  }
}

// Outputs need special processing when output patterns are present.
void ProcessOutputs(base::DictionaryValue* target, bool files_only) {
  base::ListValue* patterns = nullptr;
  base::ListValue* outputs = nullptr;
  target->GetList("output_patterns", &patterns);
  target->GetList(variables::kOutputs, &outputs);

  int indent = 0;
  if (outputs || patterns) {
    if (!files_only) {
      OutputString("\noutputs\n");
      indent = 1;
    }
    if (patterns) {
      if (!files_only) {
        OutputString("  Output patterns\n");
        indent = 2;
      }
      PrintValue(patterns, indent);
      if (!files_only)
        OutputString("\n  Resolved output file list\n");
    }
    if (outputs)
      PrintValue(outputs, indent);

    target->Remove("output_patterns", nullptr);
    target->Remove(variables::kOutputs, nullptr);
  }
}

using DescHandlerFunc = void (*)(const std::string& name,
                                 const base::Value* value,
                                 bool value_only);
std::map<std::string, DescHandlerFunc> GetHandlers() {
  return {{"type", LabelHandler},
          {"toolchain", LabelHandler},
          {variables::kVisibility, VisibilityHandler},
          {variables::kMetadata, MetadataHandler},
          {variables::kTestonly, DefaultHandler},
          {variables::kCheckIncludes, DefaultHandler},
          {variables::kAllowCircularIncludesFrom, DefaultHandler},
          {variables::kSources, DefaultHandler},
          {variables::kPublic, PublicHandler},
          {variables::kInputs, DefaultHandler},
          {variables::kConfigs, ConfigsHandler},
          {variables::kPublicConfigs, ConfigsHandler},
          {variables::kAllDependentConfigs, ConfigsHandler},
          {variables::kScript, DefaultHandler},
          {variables::kArgs, DefaultHandler},
          {variables::kDepfile, DefaultHandler},
          {"bundle_data", DefaultHandler},
          {variables::kArflags, DefaultHandler},
          {variables::kAsmflags, DefaultHandler},
          {variables::kCflags, DefaultHandler},
          {variables::kCflagsC, DefaultHandler},
          {variables::kCflagsCC, DefaultHandler},
          {variables::kCflagsObjC, DefaultHandler},
          {variables::kCflagsObjCC, DefaultHandler},
          {variables::kDefines, DefaultHandler},
          {variables::kFrameworkDirs, DefaultHandler},
          {variables::kFrameworks, DefaultHandler},
          {variables::kIncludeDirs, DefaultHandler},
          {variables::kLdflags, DefaultHandler},
          {variables::kPrecompiledHeader, DefaultHandler},
          {variables::kPrecompiledSource, DefaultHandler},
          {variables::kDeps, DepsHandler},
          {variables::kGenDeps, DefaultHandler},
          {variables::kLibs, DefaultHandler},
          {variables::kLibDirs, DefaultHandler},
          {variables::kDataKeys, DefaultHandler},
          {variables::kRebase, DefaultHandler},
          {variables::kWalkKeys, DefaultHandler},
          {variables::kWeakFrameworks, DefaultHandler},
          {variables::kWriteOutputConversion, DefaultHandler},
          {variables::kRustCrateName, DefaultHandler},
          {variables::kRustCrateRoot, DefaultHandler},
          {variables::kSwiftModuleName, DefaultHandler},
          {variables::kSwiftBridgeHeader, DefaultHandler},
          {variables::kMnemonic, DefaultHandler},
          {"runtime_deps", DefaultHandler}};
}

void HandleProperty(const std::string& what,
                    const std::map<std::string, DescHandlerFunc>& handler_map,
                    std::unique_ptr<base::Value>& v,
                    std::unique_ptr<base::DictionaryValue>& dict) {
  if (dict->Remove(what, &v)) {
    auto pair = handler_map.find(what);
    if (pair != handler_map.end())
      pair->second(what, v.get(), false);
  }
}

bool PrintTarget(const Target* target,
                 const std::string& what,
                 bool single_target,
                 const std::map<std::string, DescHandlerFunc>& handler_map,
                 bool all,
                 bool tree,
                 bool blame) {
  std::unique_ptr<base::DictionaryValue> dict =
      DescBuilder::DescriptionForTarget(target, what, all, tree, blame);
  if (!what.empty() && dict->empty()) {
    OutputString("Don't know how to display \"" + what + "\" for \"" +
                 Target::GetStringForOutputType(target->output_type()) +
                 "\".\n");
    return false;
  }
  // Print single value
  if (!what.empty() && dict->size() == 1 && single_target) {
    if (what == variables::kOutputs) {
      ProcessOutputs(dict.get(), true);
      return true;
    }
    base::DictionaryValue::Iterator iter(*dict);
    auto pair = handler_map.find(what);
    if (pair != handler_map.end())
      pair->second(what, &iter.value(), true);
    return true;
  }

  OutputString("Target ", DECORATION_YELLOW);
  OutputString(target->label().GetUserVisibleName(false));
  OutputString("\n");

  std::unique_ptr<base::Value> v;
  // Entries with DefaultHandler are present to enforce order
  HandleProperty("type", handler_map, v, dict);
  HandleProperty("toolchain", handler_map, v, dict);
  HandleProperty(variables::kSwiftModuleName, handler_map, v, dict);
  HandleProperty(variables::kRustCrateName, handler_map, v, dict);
  HandleProperty(variables::kRustCrateRoot, handler_map, v, dict);
  HandleProperty(variables::kVisibility, handler_map, v, dict);
  HandleProperty(variables::kMetadata, handler_map, v, dict);
  HandleProperty(variables::kTestonly, handler_map, v, dict);
  HandleProperty(variables::kCheckIncludes, handler_map, v, dict);
  HandleProperty(variables::kAllowCircularIncludesFrom, handler_map, v, dict);
  HandleProperty(variables::kSources, handler_map, v, dict);
  HandleProperty(variables::kSwiftBridgeHeader, handler_map, v, dict);
  HandleProperty(variables::kPublic, handler_map, v, dict);
  HandleProperty(variables::kInputs, handler_map, v, dict);
  HandleProperty(variables::kConfigs, handler_map, v, dict);
  HandleProperty(variables::kPublicConfigs, handler_map, v, dict);
  HandleProperty(variables::kAllDependentConfigs, handler_map, v, dict);
  HandleProperty(variables::kScript, handler_map, v, dict);
  HandleProperty(variables::kArgs, handler_map, v, dict);
  HandleProperty(variables::kDepfile, handler_map, v, dict);
  ProcessOutputs(dict.get(), false);
  HandleProperty("bundle_data", handler_map, v, dict);
  HandleProperty(variables::kArflags, handler_map, v, dict);
  HandleProperty(variables::kAsmflags, handler_map, v, dict);
  HandleProperty(variables::kCflags, handler_map, v, dict);
  HandleProperty(variables::kCflagsC, handler_map, v, dict);
  HandleProperty(variables::kCflagsCC, handler_map, v, dict);
  HandleProperty(variables::kCflagsObjC, handler_map, v, dict);
  HandleProperty(variables::kCflagsObjCC, handler_map, v, dict);
  HandleProperty(variables::kSwiftflags, handler_map, v, dict);
  HandleProperty(variables::kDefines, handler_map, v, dict);
  HandleProperty(variables::kFrameworkDirs, handler_map, v, dict);
  HandleProperty(variables::kFrameworks, handler_map, v, dict);
  HandleProperty(variables::kIncludeDirs, handler_map, v, dict);
  HandleProperty(variables::kLdflags, handler_map, v, dict);
  HandleProperty(variables::kPrecompiledHeader, handler_map, v, dict);
  HandleProperty(variables::kPrecompiledSource, handler_map, v, dict);
  HandleProperty(variables::kDeps, handler_map, v, dict);
  HandleProperty(variables::kLibs, handler_map, v, dict);
  HandleProperty(variables::kLibDirs, handler_map, v, dict);
  HandleProperty(variables::kDataKeys, handler_map, v, dict);
  HandleProperty(variables::kRebase, handler_map, v, dict);
  HandleProperty(variables::kWalkKeys, handler_map, v, dict);
  HandleProperty(variables::kWeakFrameworks, handler_map, v, dict);
  HandleProperty(variables::kWriteOutputConversion, handler_map, v, dict);

#undef HandleProperty

  // Process the rest (if any)
  base::DictionaryValue::Iterator iter(*dict);
  while (!iter.IsAtEnd()) {
    DefaultHandler(iter.key(), &iter.value(), false);
    iter.Advance();
  }

  return true;
}

bool PrintConfig(const Config* config,
                 const std::string& what,
                 bool single_config,
                 const std::map<std::string, DescHandlerFunc>& handler_map) {
  std::unique_ptr<base::DictionaryValue> dict =
      DescBuilder::DescriptionForConfig(config, what);
  if (!what.empty() && dict->empty()) {
    OutputString("Don't know how to display \"" + what + "\" for a config.\n");
    return false;
  }
  // Print single value
  if (!what.empty() && dict->size() == 1 && single_config) {
    base::DictionaryValue::Iterator iter(*dict);
    auto pair = handler_map.find(what);
    if (pair != handler_map.end())
      pair->second(what, &iter.value(), true);
    return true;
  }

  OutputString("Config: ", DECORATION_YELLOW);
  OutputString(config->label().GetUserVisibleName(false));
  OutputString("\n");

  std::unique_ptr<base::Value> v;
  HandleProperty("toolchain", handler_map, v, dict);
  if (!config->configs().empty()) {
    OutputString(
        "(This is a composite config, the values below are after the\n"
        "expansion of the child configs.)\n");
  }
  HandleProperty(variables::kArflags, handler_map, v, dict);
  HandleProperty(variables::kAsmflags, handler_map, v, dict);
  HandleProperty(variables::kCflags, handler_map, v, dict);
  HandleProperty(variables::kCflagsC, handler_map, v, dict);
  HandleProperty(variables::kCflagsCC, handler_map, v, dict);
  HandleProperty(variables::kCflagsObjC, handler_map, v, dict);
  HandleProperty(variables::kCflagsObjCC, handler_map, v, dict);
  HandleProperty(variables::kSwiftflags, handler_map, v, dict);
  HandleProperty(variables::kDefines, handler_map, v, dict);
  HandleProperty(variables::kFrameworkDirs, handler_map, v, dict);
  HandleProperty(variables::kFrameworks, handler_map, v, dict);
  HandleProperty(variables::kIncludeDirs, handler_map, v, dict);
  HandleProperty(variables::kInputs, handler_map, v, dict);
  HandleProperty(variables::kLdflags, handler_map, v, dict);
  HandleProperty(variables::kLibs, handler_map, v, dict);
  HandleProperty(variables::kLibDirs, handler_map, v, dict);
  HandleProperty(variables::kPrecompiledHeader, handler_map, v, dict);
  HandleProperty(variables::kPrecompiledSource, handler_map, v, dict);
  HandleProperty(variables::kWeakFrameworks, handler_map, v, dict);

#undef HandleProperty

  return true;
}

}  // namespace

// desc ------------------------------------------------------------------------

const char kDesc[] = "desc";
const char kDesc_HelpShort[] =
    "desc: Show lots of insightful information about a target or config.";
const char kDesc_Help[] =
    R"(gn desc

  gn desc <out_dir> <label or pattern> [<what to show>] [--blame]
          [--format=json]

  Displays information about a given target or config. The build parameters
  will be taken for the build in the given <out_dir>.

  The <label or pattern> can be a target label, a config label, or a label
  pattern (see "gn help label_pattern"). A label pattern will only match
  targets.

Possibilities for <what to show>

  (If unspecified an overall summary will be displayed.)

  all_dependent_configs
  allow_circular_includes_from
  arflags [--blame]
  args
  cflags [--blame]
  cflags_c [--blame]
  cflags_cc [--blame]
  check_includes
  configs [--tree] (see below)
  data_keys
  defines [--blame]
  depfile
  deps [--all] [--tree] (see below)
  framework_dirs
  frameworks
  include_dirs [--blame]
  inputs
  ldflags [--blame]
  lib_dirs
  libs
  metadata
  output_conversion
  outputs
  public_configs
  public
  rebase
  script
  sources
  testonly
  visibility
  walk_keys
  weak_frameworks

  runtime_deps
      Compute all runtime deps for the given target. This is a computed list
      and does not correspond to any GN variable, unlike most other values
      here.

      The output is a list of file names relative to the build directory. See
      "gn help runtime_deps" for how this is computed. This also works with
      "--blame" to see the source of the dependency.

Shared flags

)"

    DEFAULT_TOOLCHAIN_SWITCH_HELP

    R"(
  --format=json
      Format the output as JSON instead of text.

Target flags

  --blame
      Used with any value specified on a config, this will name the config that
      causes that target to get the flag. This doesn't currently work for libs,
      lib_dirs, frameworks, weak_frameworks and framework_dirs because those are
      inherited and are more complicated to figure out the blame (patches
      welcome).

Configs

  The "configs" section will list all configs that apply. For targets this will
  include configs specified in the "configs" variable of the target, and also
  configs pushed onto this target via public or "all dependent" configs.

  Configs can have child configs. Specifying --tree will show the hierarchy.

Printing outputs

  The "outputs" section will list all outputs that apply, including the outputs
  computed from the tool definition (eg for "executable", "static_library", ...
  targets).

Printing deps

  Deps will include all public, private, and data deps (TODO this could be
  clarified and enhanced) sorted in order applying. The following may be used:

  --all
      Collects all recursive dependencies and prints a sorted flat list. Also
      usable with --tree (see below).

)"

    TARGET_PRINTING_MODE_COMMAND_LINE_HELP
    "\n" TARGET_TESTONLY_FILTER_COMMAND_LINE_HELP

    R"(
  --tree
      Print a dependency tree. By default, duplicates will be elided with "..."
      but when --all and -tree are used together, no eliding will be performed.

      The "deps", "public_deps", and "data_deps" will all be included in the
      tree.

      Tree output can not be used with the filtering or output flags: --as,
      --type, --testonly.

)"

    TARGET_TYPE_FILTER_COMMAND_LINE_HELP

    R"(
Note

  This command will show the full name of directories and source files, but
  when directories and source paths are written to the build file, they will be
  adjusted to be relative to the build directory. So the values for paths
  displayed by this command won't match (but should mean the same thing).

Examples

  gn desc out/Debug //base:base
      Summarizes the given target.

  gn desc out/Foo :base_unittests deps --tree
      Shows a dependency tree of the "base_unittests" project in
      the current directory.

  gn desc out/Debug //base defines --blame
      Shows defines set for the //base:base target, annotated by where
      each one was set from.
)";

int RunDesc(const std::vector<std::string>& args) {
  if (args.size() != 2 && args.size() != 3) {
    Err(Location(), "Unknown command format. See \"gn help desc\"",
        "Usage: \"gn desc <out_dir> <target_name> [<what to display>]\"")
        .PrintToStdout();
    return 1;
  }
  const base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();

  // Deliberately leaked to avoid expensive process teardown.
  Setup* setup = new Setup;
  if (!setup->DoSetup(args[0], false))
    return 1;
  if (!setup->Run())
    return 1;

  // Resolve target(s) and config from inputs.
  UniqueVector<const Target*> target_matches;
  UniqueVector<const Config*> config_matches;
  UniqueVector<const Toolchain*> toolchain_matches;
  UniqueVector<SourceFile> file_matches;

  std::vector<std::string> target_list;
  target_list.push_back(args[1]);

  if (!ResolveFromCommandLineInput(
          setup, target_list, cmdline->HasSwitch(switches::kDefaultToolchain),
          &target_matches, &config_matches, &toolchain_matches, &file_matches))
    return 1;

  std::string what_to_print;
  if (args.size() == 3)
    what_to_print = args[2];

  bool json = cmdline->GetSwitchValueString("format") == "json";

  if (target_matches.empty() && config_matches.empty()) {
    OutputString(
        "The input " + args[1] + " matches no targets, configs or files.\n",
        DECORATION_YELLOW);
    return 1;
  }

  if (json) {
    // Convert all targets/configs to JSON, serialize and print them
    auto res = std::make_unique<base::DictionaryValue>();
    if (!target_matches.empty()) {
      for (const auto* target : target_matches) {
        res->SetWithoutPathExpansion(
            target->label().GetUserVisibleName(
                target->settings()->default_toolchain_label()),
            DescBuilder::DescriptionForTarget(
                target, what_to_print, cmdline->HasSwitch(kAll),
                cmdline->HasSwitch(kTree), cmdline->HasSwitch(kBlame)));
      }
    } else if (!config_matches.empty()) {
      for (const auto* config : config_matches) {
        res->SetWithoutPathExpansion(
            config->label().GetUserVisibleName(false),
            DescBuilder::DescriptionForConfig(config, what_to_print));
      }
    }
    std::string s;
    base::JSONWriter::WriteWithOptions(
        *res.get(), base::JSONWriter::OPTIONS_PRETTY_PRINT, &s);
    OutputString(s);
  } else {
    // Regular (non-json) formatted output
    bool multiple_outputs = (target_matches.size() + config_matches.size()) > 1;
    std::map<std::string, DescHandlerFunc> handlers = GetHandlers();

    bool printed_output = false;
    for (const Target* target : target_matches) {
      if (printed_output)
        OutputString("\n\n");
      printed_output = true;

      if (!PrintTarget(target, what_to_print, !multiple_outputs, handlers,
                       cmdline->HasSwitch(kAll), cmdline->HasSwitch(kTree),
                       cmdline->HasSwitch(kBlame)))
        return 1;
    }
    for (const Config* config : config_matches) {
      if (printed_output)
        OutputString("\n\n");
      printed_output = true;

      if (!PrintConfig(config, what_to_print, !multiple_outputs, handlers))
        return 1;
    }
  }

  return 0;
}

}  // namespace commands
