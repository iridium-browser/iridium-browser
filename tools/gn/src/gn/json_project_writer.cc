// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/json_project_writer.h"

#include <algorithm>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <vector>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_writer.h"
#include "base/json/string_escape.h"
#include "base/strings/string_number_conversions.h"
#include "gn/builder.h"
#include "gn/commands.h"
#include "gn/deps_iterator.h"
#include "gn/desc_builder.h"
#include "gn/exec_process.h"
#include "gn/filesystem_utils.h"
#include "gn/scheduler.h"
#include "gn/settings.h"
#include "gn/string_output_buffer.h"

// Structure of JSON output file
// {
//   "build_settings" : {
//     "root_path" : "absolute path of project root",
//     "build_dir" : "build directory (project relative)",
//     "default_toolchain" : "name of default toolchain"
//   }
//   "targets" : {
//      "target x full label" : { target x properties },
//      "target y full label" : { target y properties },
//      ...
//    }
// }
// See desc_builder.cc for overview of target properties

namespace {

void AddTargetDependencies(const Target* target, TargetSet* deps) {
  for (const auto& pair : target->GetDeps(Target::DEPS_LINKED)) {
    if (deps->add(pair.ptr)) {
      AddTargetDependencies(pair.ptr, deps);
    }
  }
}

// Filters targets according to filter string; Will also recursively
// add dependent targets.
bool FilterTargets(const BuildSettings* build_settings,
                   std::vector<const Target*>& all_targets,
                   std::vector<const Target*>* targets,
                   const std::string& dir_filter_string,
                   Err* err) {
  if (dir_filter_string.empty()) {
    *targets = all_targets;
  } else {
    targets->reserve(all_targets.size());
    std::vector<LabelPattern> filters;
    if (!commands::FilterPatternsFromString(build_settings, dir_filter_string,
                                            &filters, err)) {
      return false;
    }
    commands::FilterTargetsByPatterns(all_targets, filters, targets);

    TargetSet target_set(targets->begin(), targets->end());
    for (const auto* target : *targets)
      AddTargetDependencies(target, &target_set);

    targets->clear();
    targets->insert(targets->end(), target_set.begin(), target_set.end());
  }

  // Sort the list of targets per-label to get a consistent ordering of them
  // in the generated project (and thus stability of the file generated).
  std::sort(targets->begin(), targets->end(),
            [](const Target* a, const Target* b) {
              return a->label().name() < b->label().name();
            });

  return true;
}

bool InvokePython(const BuildSettings* build_settings,
                  const base::FilePath& python_script_path,
                  const std::string& python_script_extra_args,
                  const base::FilePath& output_path,
                  bool quiet,
                  Err* err) {
  const base::FilePath& python_path = build_settings->python_path();
  base::CommandLine cmdline(python_path);
  cmdline.AppendArg("--");
  cmdline.AppendArgPath(python_script_path);
  cmdline.AppendArgPath(output_path);
  if (!python_script_extra_args.empty()) {
    cmdline.AppendArg(python_script_extra_args);
  }
  base::FilePath startup_dir =
      build_settings->GetFullPath(build_settings->build_dir());

  std::string output;
  std::string stderr_output;

  int exit_code = 0;
  if (!internal::ExecProcess(cmdline, startup_dir, &output, &stderr_output,
                             &exit_code)) {
    *err =
        Err(Location(), "Could not execute python.",
            "I was trying to execute \"" + FilePathToUTF8(python_path) + "\".");
    return false;
  }

  if (!quiet) {
    printf("%s", output.c_str());
    fprintf(stderr, "%s", stderr_output.c_str());
  }

  if (exit_code != 0) {
    *err = Err(Location(), "Python has quit with exit code " +
                               base::IntToString(exit_code) + ".");
    return false;
  }

  return true;
}

}  // namespace

bool JSONProjectWriter::RunAndWriteFiles(
    const BuildSettings* build_settings,
    const Builder& builder,
    const std::string& file_name,
    const std::string& exec_script,
    const std::string& exec_script_extra_args,
    const std::string& dir_filter_string,
    bool quiet,
    Err* err) {
  SourceFile output_file = build_settings->build_dir().ResolveRelativeFile(
      Value(nullptr, file_name), err);
  if (output_file.is_null()) {
    return false;
  }

  base::FilePath output_path = build_settings->GetFullPath(output_file);

  std::vector<const Target*> all_targets = builder.GetAllResolvedTargets();
  std::vector<const Target*> targets;
  if (!FilterTargets(build_settings, all_targets, &targets, dir_filter_string,
                     err)) {
    return false;
  }

  StringOutputBuffer json = GenerateJSON(build_settings, targets);
  if (!json.ContentsEqual(output_path)) {
    if (!json.WriteToFile(output_path, err)) {
      return false;
    }

    if (!exec_script.empty()) {
      SourceFile script_file;
      if (exec_script[0] != '/') {
        // Relative path, assume the base is in build_dir.
        script_file = build_settings->build_dir().ResolveRelativeFile(
            Value(nullptr, exec_script), err);
        if (script_file.is_null()) {
          return false;
        }
      } else {
        script_file = SourceFile(exec_script);
      }
      base::FilePath script_path = build_settings->GetFullPath(script_file);
      return InvokePython(build_settings, script_path, exec_script_extra_args,
                          output_path, quiet, err);
    }
  }

  return true;
}

namespace {

// NOTE: Intentional macro definition allows compile-time string concatenation.
// (see usage below).
#if defined(OS_WINDOWS)
#define LINE_ENDING "\r\n"
#else
#define LINE_ENDING "\n"
#endif

// Helper class to output a, potentially very large, JSON file to a
// StringOutputBuffer. Note that sorting the keys, if desired, is left to
// the user (unlike base::JSONWriter). This allows rendering to be performed
// in series of incremental steps. Usage is:
//
//   1) Create instance, passing a StringOutputBuffer reference as the
//      destination.
//
//   2) Add keys and values using one of the following:
//
//       a) AddString(key, string_value) to add one string value.
//
//       b) BeginList(key), AddListItem(), EndList() to add a string list.
//          NOTE: Only lists of strings are supported here.
//
//       c) BeginDict(key), ... add other keys, followed by EndDict() to add
//          a dictionary key.
//
//   3) Call Close() or destroy the instance to finalize the output.
//
class SimpleJSONWriter {
 public:
  // Constructor.
  SimpleJSONWriter(StringOutputBuffer& out) : out_(out) {
    out_ << "{" LINE_ENDING;
    SetIndentation(1u);
  }

  // Destructor.
  ~SimpleJSONWriter() { Close(); }

  // Closing finalizes the output.
  void Close() {
    if (indentation_ > 0) {
      DCHECK(indentation_ == 1u);
      if (comma_.size())
        out_ << LINE_ENDING;

      out_ << "}" LINE_ENDING;
      SetIndentation(0);
    }
  }

  // Add new string-valued key.
  void AddString(std::string_view key, std::string_view value) {
    if (comma_.size()) {
      out_ << comma_;
    }
    AddMargin() << Escape(key) << ": " << Escape(value);
    comma_ = "," LINE_ENDING;
  }

  // Begin a new list. Must be followed by zero or more AddListItem() calls,
  // then by EndList().
  void BeginList(std::string_view key) {
    if (comma_.size())
      out_ << comma_;
    AddMargin() << Escape(key) << ": [ ";
    comma_ = {};
  }

  // Add a new list item. For now only string values are supported.
  void AddListItem(std::string_view item) {
    if (comma_.size())
      out_ << comma_;
    out_ << Escape(item);
    comma_ = ", ";
  }

  // End current list.
  void EndList() {
    out_ << " ]";
    comma_ = "," LINE_ENDING;
  }

  // Begin new dictionaary. Must be followed by zero or more other key
  // additions, then a call to EndDict().
  void BeginDict(std::string_view key) {
    if (comma_.size())
      out_ << comma_;

    AddMargin() << Escape(key) << ": {";
    SetIndentation(indentation_ + 1);
    comma_ = LINE_ENDING;
  }

  // End current dictionary.
  void EndDict() {
    if (comma_.size())
      out_ << LINE_ENDING;

    SetIndentation(indentation_ - 1);
    AddMargin() << "}";
    comma_ = "," LINE_ENDING;
  }

  // Add a dictionary-valued key, whose value is already formatted as a valid
  // JSON string. Useful to insert the output of base::JSONWriter::Write()
  // into the target buffer.
  void AddJSONDict(std::string_view key, std::string_view json) {
    if (comma_.size())
      out_ << comma_;
    AddMargin() << Escape(key) << ": ";
    if (json.empty()) {
      out_ << "{ }";
    } else {
      DCHECK(json[0] == '{');
      bool first_line = true;
      do {
        size_t line_end = json.find('\n');

        // NOTE: Do not add margin if original input line is empty.
        // This needs to deal with CR/LF which are part of |json| on Windows
        // only, due to the way base::JSONWriter::Write() is implemented.
        bool line_empty = (line_end == 0 || (line_end == 1 && json[0] == '\r'));
        if (!first_line && !line_empty)
          AddMargin();

        if (line_end == std::string_view::npos) {
          out_ << json;
          comma_ = {};
          return;
        }
        // Important: do not add the final newline.
        out_ << json.substr(
            0, (line_end == json.size() - 1) ? line_end : line_end + 1);
        json.remove_prefix(line_end + 1);
        first_line = false;
      } while (!json.empty());
    }
    comma_ = "," LINE_ENDING;
  }

 private:
  // Return the JSON-escape version of |str|.
  static std::string Escape(std::string_view str) {
    std::string result;
    base::EscapeJSONString(str, true, &result);
    return result;
  }

  // Adjust indentation level.
  void SetIndentation(size_t indentation) { indentation_ = indentation; }

  // Append margin, and return reference to output buffer.
  StringOutputBuffer& AddMargin() const {
    static const char kMargin[17] = "                ";
    size_t margin_len = indentation_ * 3;
    while (margin_len > 0) {
      size_t span = (margin_len > 16u) ? 16u : margin_len;
      out_.Append(kMargin, span);
      margin_len -= span;
    }
    return out_;
  }

  size_t indentation_ = 0;
  std::string_view comma_;
  StringOutputBuffer& out_;
};

}  // namespace

StringOutputBuffer JSONProjectWriter::GenerateJSON(
    const BuildSettings* build_settings,
    std::vector<const Target*>& all_targets) {
  Label default_toolchain_label;
  if (!all_targets.empty())
    default_toolchain_label =
        all_targets[0]->settings()->default_toolchain_label();

  StringOutputBuffer out;

  // Sort the targets according to their human visible labels first.
  std::unordered_map<const Target*, std::string> target_labels;
  for (const Target* target : all_targets) {
    target_labels[target] =
        target->label().GetUserVisibleName(default_toolchain_label);
  }

  std::vector<const Target*> sorted_targets(all_targets.begin(),
                                            all_targets.end());
  std::sort(sorted_targets.begin(), sorted_targets.end(),
            [&target_labels](const Target* a, const Target* b) {
              return target_labels[a] < target_labels[b];
            });

  SimpleJSONWriter json_writer(out);

  // IMPORTANT: Keep the keys sorted when adding them to |json_writer|.

  json_writer.BeginDict("build_settings");
  {
    json_writer.AddString("build_dir", build_settings->build_dir().value());

    json_writer.AddString("default_toolchain",
                          default_toolchain_label.GetUserVisibleName(false));

    json_writer.BeginList("gen_input_files");

    // Other files read by the build.
    std::vector<base::FilePath> other_files = g_scheduler->GetGenDependencies();

    const InputFileManager* input_file_manager =
        g_scheduler->input_file_manager();

    VectorSetSorter<base::FilePath> sorter(
        input_file_manager->GetInputFileCount() + other_files.size());

    input_file_manager->AddAllPhysicalInputFileNamesToVectorSetSorter(&sorter);

    sorter.Add(other_files.begin(), other_files.end());

    std::string build_path = FilePathToUTF8(build_settings->root_path());
    auto item_callback = [&json_writer,
                          &build_path](const base::FilePath& input_file) {
      std::string file;
      if (MakeAbsolutePathRelativeIfPossible(
              build_path, FilePathToUTF8(input_file), &file)) {
        json_writer.AddListItem(file);
      }
    };
    sorter.IterateOver(item_callback);

    json_writer.EndList();  // gen_input_files

    json_writer.AddString("root_path", build_settings->root_path_utf8());
  }
  json_writer.EndDict();  // build_settings

  std::map<Label, const Toolchain*> toolchains;
  json_writer.BeginDict("targets");
  {
    for (const auto* target : sorted_targets) {
      auto description =
          DescBuilder::DescriptionForTarget(target, "", false, false, false);
      // Outputs need to be asked for separately.
      auto outputs = DescBuilder::DescriptionForTarget(target, "source_outputs",
                                                       false, false, false);
      base::DictionaryValue* outputs_value = nullptr;
      if (outputs->GetDictionary("source_outputs", &outputs_value) &&
          !outputs_value->empty()) {
        description->MergeDictionary(outputs.get());
      }

      std::string json_dict;
      base::JSONWriter::WriteWithOptions(*description.get(),
                                         base::JSONWriter::OPTIONS_PRETTY_PRINT,
                                         &json_dict);
      json_writer.AddJSONDict(target_labels[target], json_dict);
      toolchains[target->toolchain()->label()] = target->toolchain();
    }
  }
  json_writer.EndDict();  // targets

  json_writer.BeginDict("toolchains");
  {
    for (const auto& tool_chain_kv : toolchains) {
      base::Value toolchain{base::Value::Type::DICTIONARY};
      const auto& tools = tool_chain_kv.second->tools();
      for (const auto& tool_kv : tools) {
        // Do not list builtin tools
        if (tool_kv.second->AsBuiltin())
          continue;
        base::Value tool_info{base::Value::Type::DICTIONARY};
        auto setIfNotEmptry = [&](const auto& key, const auto& value) {
          if (value.size())
            tool_info.SetKey(key, base::Value{value});
        };
        auto setSubstitutionList = [&](const auto& key,
                                       const SubstitutionList& list) {
          if (list.list().empty())
            return;
          base::Value values{base::Value::Type::LIST};
          for (const auto& v : list.list())
            values.GetList().emplace_back(base::Value{v.AsString()});
          tool_info.SetKey(key, std::move(values));
        };
        const auto& tool = tool_kv.second;
        setIfNotEmptry("command", tool->command().AsString());
        setIfNotEmptry("command_launcher", tool->command_launcher());
        setIfNotEmptry("default_output_extension",
                       tool->default_output_extension());
        setIfNotEmptry("default_output_dir",
                       tool->default_output_dir().AsString());
        setIfNotEmptry("depfile", tool->depfile().AsString());
        setIfNotEmptry("description", tool->description().AsString());
        setIfNotEmptry("framework_switch", tool->framework_switch());
        setIfNotEmptry("weak_framework_switch", tool->weak_framework_switch());
        setIfNotEmptry("framework_dir_switch", tool->framework_dir_switch());
        setIfNotEmptry("lib_switch", tool->lib_switch());
        setIfNotEmptry("lib_dir_switch", tool->lib_dir_switch());
        setIfNotEmptry("linker_arg", tool->linker_arg());
        setSubstitutionList("outputs", tool->outputs());
        setSubstitutionList("partial_outputs", tool->partial_outputs());
        setSubstitutionList("runtime_outputs", tool->runtime_outputs());
        setIfNotEmptry("output_prefix", tool->output_prefix());

        toolchain.SetKey(tool_kv.first, std::move(tool_info));
      }
      std::string json_dict;
      base::JSONWriter::WriteWithOptions(
          toolchain, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json_dict);
      json_writer.AddJSONDict(tool_chain_kv.first.GetUserVisibleName(false), json_dict);
    }
  }
  json_writer.EndDict();  // toolchains

  json_writer.Close();

  return out;
}

std::string JSONProjectWriter::RenderJSON(
    const BuildSettings* build_settings,
    std::vector<const Target*>& all_targets) {
  StringOutputBuffer storage = GenerateJSON(build_settings, all_targets);
  return storage.str();
}
