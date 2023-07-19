// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/analyzer.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <set>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "gn/builder.h"
#include "gn/config.h"
#include "gn/config_values_extractors.h"
#include "gn/deps_iterator.h"
#include "gn/err.h"
#include "gn/filesystem_utils.h"
#include "gn/loader.h"
#include "gn/location.h"
#include "gn/pool.h"
#include "gn/source_file.h"
#include "gn/target.h"

namespace {

struct Inputs {
  std::vector<SourceFile> source_vec;
  std::vector<Label> compile_vec;
  std::vector<Label> test_vec;
  bool compile_included_all = false;
  std::set<const SourceFile*> source_files;
  std::set<Label> compile_labels;
  std::set<Label> test_labels;
};

struct Outputs {
  std::string status;
  std::string error;
  bool compile_includes_all = false;
  std::set<Label> compile_labels;
  std::set<Label> test_labels;
  std::set<Label> invalid_labels;
};

std::set<Label> LabelsFor(const TargetSet& targets) {
  std::set<Label> labels;
  for (auto* target : targets)
    labels.insert(target->label());
  return labels;
}

TargetSet Intersect(const TargetSet& l, const TargetSet& r) {
  return l.intersection_with(r);
}

std::vector<std::string> GetStringVector(const base::DictionaryValue& dict,
                                         const std::string& key,
                                         Err* err) {
  std::vector<std::string> strings;
  const base::ListValue* lst;
  bool ret = dict.GetList(key, &lst);
  if (!ret) {
    *err = Err(Location(), "Input does not have a key named \"" + key +
                               "\" with a list value.");
    return strings;
  }

  for (size_t i = 0; i < lst->GetSize(); i++) {
    std::string s;
    ret = lst->GetString(i, &s);
    if (!ret) {
      *err = Err(Location(), "Item " + std::to_string(i) + " of \"" + key +
                                 "\" is not a string.");
      strings.clear();
      return strings;
    }
    strings.push_back(std::move(s));
  }
  *err = Err();
  return strings;
}

void WriteString(base::DictionaryValue& dict,
                 const std::string& key,
                 const std::string& value) {
  dict.SetKey(key, base::Value(value));
}

void WriteLabels(const Label& default_toolchain,
                 base::DictionaryValue& dict,
                 const std::string& key,
                 const std::set<Label>& labels) {
  std::vector<std::string> strings;
  auto value = std::make_unique<base::ListValue>();
  for (const auto& l : labels)
    strings.push_back(l.GetUserVisibleName(default_toolchain));
  std::sort(strings.begin(), strings.end());
  value->AppendStrings(strings);
  dict.SetWithoutPathExpansion(key, std::move(value));
}

Label AbsoluteOrSourceAbsoluteStringToLabel(const Label& default_toolchain,
                                            const std::string& s,
                                            Err* err) {
  if (!IsPathSourceAbsolute(s) && !IsPathAbsolute(s)) {
    *err = Err(Location(),
               "\"" + s + "\" is not a source-absolute or absolute path.");
    return Label();
  }
  return Label::Resolve(SourceDir("//"), std::string_view(), default_toolchain,
                        Value(nullptr, s), err);
}

Err JSONToInputs(const Label& default_toolchain,
                 const std::string input,
                 Inputs* inputs) {
  int error_code_out;
  std::string error_msg_out;
  int error_line_out;
  int error_column_out;
  std::unique_ptr<base::Value> value = base::JSONReader::ReadAndReturnError(
      input, base::JSONParserOptions::JSON_PARSE_RFC, &error_code_out,
      &error_msg_out, &error_line_out, &error_column_out);
  if (!value)
    return Err(Location(), "Input is not valid JSON:" + error_msg_out);

  const base::DictionaryValue* dict;
  if (!value->GetAsDictionary(&dict))
    return Err(Location(), "Input is not a dictionary.");

  Err err;

  const char kFilesKey[] = "files";
  {
    std::vector<std::string> files = GetStringVector(*dict, kFilesKey, &err);
    if (err.has_error())
      return err;
    for (auto& s : files) {
      if (!IsPathSourceAbsolute(s) && !IsPathAbsolute(s)) {
        return Err(Location(),
                   "\"" + s + "\" is not a source-absolute or absolute path.");
      }
      inputs->source_vec.emplace_back(std::move(s));
    }
  }

  inputs->compile_included_all = false;
  const char kAdditonalCompileTargetsKey[] = "additional_compile_targets";
  if (dict->HasKey(kAdditonalCompileTargetsKey)) {
    std::vector<std::string> additional_compile_targets =
        GetStringVector(*dict, kAdditonalCompileTargetsKey, &err);
    if (err.has_error())
      return err;

    for (auto& s : additional_compile_targets) {
      if (s == "all") {
        inputs->compile_included_all = true;
      } else {
        inputs->compile_vec.push_back(
            AbsoluteOrSourceAbsoluteStringToLabel(default_toolchain, s, &err));
        if (err.has_error())
          return err;
      }
    }
  }

  const char kTestTargetsKey[] = "test_targets";
  {
    std::vector<std::string> test_targets =
        GetStringVector(*dict, kTestTargetsKey, &err);
    if (err.has_error())
      return err;
    for (auto& s : test_targets) {
      inputs->test_vec.push_back(
          AbsoluteOrSourceAbsoluteStringToLabel(default_toolchain, s, &err));
      if (err.has_error())
        return err;
    }
  }

  for (const auto kv : dict->DictItems()) {
    if (kv.first == kFilesKey || kv.first == kAdditonalCompileTargetsKey ||
        kv.first == kTestTargetsKey) {
      continue;
    }
    return Err(Location(), "Unknown analyze input key \"" + kv.first + "\".");
  }

  for (auto& s : inputs->source_vec)
    inputs->source_files.insert(&s);
  for (auto& l : inputs->compile_vec)
    inputs->compile_labels.insert(l);
  for (auto& l : inputs->test_vec)
    inputs->test_labels.insert(l);
  return Err();
}

std::string OutputsToJSON(const Outputs& outputs,
                          const Label& default_toolchain,
                          Err* err) {
  std::string output;
  auto value = std::make_unique<base::DictionaryValue>();

  if (outputs.error.size()) {
    WriteString(*value, "error", outputs.error);
    WriteLabels(default_toolchain, *value, "invalid_targets",
                outputs.invalid_labels);
  } else {
    WriteString(*value, "status", outputs.status);
    if (outputs.compile_includes_all) {
      auto compile_targets = std::make_unique<base::ListValue>();
      compile_targets->AppendString("all");
      value->SetWithoutPathExpansion("compile_targets",
                                     std::move(compile_targets));
    } else {
      WriteLabels(default_toolchain, *value, "compile_targets",
                  outputs.compile_labels);
    }
    WriteLabels(default_toolchain, *value, "test_targets", outputs.test_labels);
  }

  if (!base::JSONWriter::Write(*value.get(), &output))
    *err = Err(Location(), "Failed to marshal JSON value for output");
  return output;
}

}  // namespace

Analyzer::Analyzer(const Builder& builder,
                   const SourceFile& build_config_file,
                   const SourceFile& dot_file,
                   const SourceFileSet& build_args_dependency_files)
    : all_items_(builder.GetAllResolvedItems()),
      default_toolchain_(builder.loader()->GetDefaultToolchain()),
      build_config_file_(build_config_file),
      dot_file_(dot_file),
      build_args_dependency_files_(build_args_dependency_files) {
  for (const auto* item : all_items_) {
    labels_to_items_[item->label()] = item;

    // Fill dep_map_.
    if (item->AsTarget()) {
      for (const auto& dep_target_pair :
           item->AsTarget()->GetDeps(Target::DEPS_ALL))
        dep_map_.insert(std::make_pair(dep_target_pair.ptr, item));

      for (const auto& dep_config_pair : item->AsTarget()->configs())
        dep_map_.insert(std::make_pair(dep_config_pair.ptr, item));

      dep_map_.insert(std::make_pair(item->AsTarget()->toolchain(), item));

      if (item->AsTarget()->IsBinary() ||
          item->AsTarget()->output_type() == Target::ACTION ||
          item->AsTarget()->output_type() == Target::ACTION_FOREACH) {
        const LabelPtrPair<Pool>& pool =
            item->AsTarget()->pool();
        if (pool.ptr)
          dep_map_.insert(std::make_pair(pool.ptr, item));
      }
    } else if (item->AsConfig()) {
      for (const auto& dep_config_pair : item->AsConfig()->configs())
        dep_map_.insert(std::make_pair(dep_config_pair.ptr, item));
    } else if (item->AsToolchain()) {
      for (const auto& dep_pair : item->AsToolchain()->deps())
        dep_map_.insert(std::make_pair(dep_pair.ptr, item));
    } else {
      DCHECK(item->AsPool());
    }
  }
}

Analyzer::~Analyzer() = default;

std::string Analyzer::Analyze(const std::string& input, Err* err) const {
  Inputs inputs;
  Outputs outputs;

  Err local_err = JSONToInputs(default_toolchain_, input, &inputs);
  if (local_err.has_error()) {
    outputs.error = local_err.message();
    return OutputsToJSON(outputs, default_toolchain_, err);
  }

  std::set<Label> invalid_labels;
  for (const auto& label : InvalidLabels(inputs.compile_labels))
    invalid_labels.insert(label);
  for (const auto& label : InvalidLabels(inputs.test_labels))
    invalid_labels.insert(label);
  if (!invalid_labels.empty()) {
    outputs.error = "Invalid targets";
    outputs.invalid_labels = invalid_labels;
    return OutputsToJSON(outputs, default_toolchain_, err);
  }

  if (WereMainGNFilesModified(inputs.source_files)) {
    outputs.status = "Found dependency (all)";
    if (inputs.compile_included_all) {
      outputs.compile_includes_all = true;
    } else {
      outputs.compile_labels.insert(inputs.compile_labels.begin(),
                                    inputs.compile_labels.end());
      outputs.compile_labels.insert(inputs.test_labels.begin(),
                                    inputs.test_labels.end());
    }
    outputs.test_labels = inputs.test_labels;
    return OutputsToJSON(outputs, default_toolchain_, err);
  }

  std::set<const Item*> affected_items =
      GetAllAffectedItems(inputs.source_files);
  TargetSet affected_targets;
  for (const Item* affected_item : affected_items) {
    if (affected_item->AsTarget())
      affected_targets.insert(affected_item->AsTarget());
  }

  if (affected_targets.empty()) {
    outputs.status = "No dependency";
    return OutputsToJSON(outputs, default_toolchain_, err);
  }

  TargetSet root_targets;
  for (const auto* item : all_items_) {
    if (item->AsTarget() && dep_map_.find(item) == dep_map_.end())
      root_targets.insert(item->AsTarget());
  }

  TargetSet compile_targets = TargetsFor(inputs.compile_labels);
  if (inputs.compile_included_all) {
    for (auto* root_target : root_targets)
      compile_targets.insert(root_target);
  }
  TargetSet filtered_targets = Filter(compile_targets);
  outputs.compile_labels =
      LabelsFor(Intersect(filtered_targets, affected_targets));

  // If every target is affected, simply compile All instead of listing all
  // the targets to make the output easier to read.
  if (inputs.compile_included_all &&
      outputs.compile_labels.size() == filtered_targets.size())
    outputs.compile_includes_all = true;

  TargetSet test_targets = TargetsFor(inputs.test_labels);
  outputs.test_labels = LabelsFor(Intersect(test_targets, affected_targets));

  if (outputs.compile_labels.empty() && outputs.test_labels.empty())
    outputs.status = "No dependency";
  else
    outputs.status = "Found dependency";
  return OutputsToJSON(outputs, default_toolchain_, err);
}

std::set<const Item*> Analyzer::GetAllAffectedItems(
    const std::set<const SourceFile*>& source_files) const {
  std::set<const Item*> directly_affected_items;
  for (auto* source_file : source_files)
    AddItemsDirectlyReferringToFile(source_file, &directly_affected_items);

  std::set<const Item*> all_affected_items;
  for (auto* affected_item : directly_affected_items)
    AddAllItemsReferringToItem(affected_item, &all_affected_items);

  return all_affected_items;
}

std::set<Label> Analyzer::InvalidLabels(const std::set<Label>& labels) const {
  std::set<Label> invalid_labels;
  for (const Label& label : labels) {
    if (labels_to_items_.find(label) == labels_to_items_.end())
      invalid_labels.insert(label);
  }
  return invalid_labels;
}

TargetSet Analyzer::TargetsFor(const std::set<Label>& labels) const {
  TargetSet targets;
  for (const auto& label : labels) {
    auto it = labels_to_items_.find(label);
    if (it != labels_to_items_.end()) {
      DCHECK(it->second->AsTarget());
      targets.insert(it->second->AsTarget());
    }
  }
  return targets;
}

TargetSet Analyzer::Filter(const TargetSet& targets) const {
  TargetSet seen;
  TargetSet filtered;
  for (const auto* target : targets)
    FilterTarget(target, &seen, &filtered);
  return filtered;
}

void Analyzer::FilterTarget(const Target* target,
                            TargetSet* seen,
                            TargetSet* filtered) const {
  if (seen->add(target)) {
    if (target->output_type() != Target::GROUP) {
      filtered->insert(target);
    } else {
      for (const auto& pair : target->GetDeps(Target::DEPS_ALL))
        FilterTarget(pair.ptr, seen, filtered);
    }
  }
}

bool Analyzer::ItemRefersToFile(const Item* item,
                                const SourceFile* file) const {
  for (const auto& cur_file : item->build_dependency_files()) {
    if (cur_file == *file)
      return true;
  }

  if (const Config* config = item->AsConfig()) {
    for (const auto& config_pair: config->configs()) {
      if (ItemRefersToFile(config_pair.ptr, file))
        return true;
    }
  }

  if (!item->AsTarget())
    return false;

  const Target* target = item->AsTarget();
  for (const auto& cur_file : target->sources()) {
    if (cur_file == *file)
      return true;
  }
  for (const auto& cur_file : target->public_headers()) {
    if (cur_file == *file)
      return true;
  }
  for (ConfigValuesIterator iter(target); !iter.done(); iter.Next()) {
    for (const auto& cur_file : iter.cur().inputs()) {
      if (cur_file == *file)
        return true;
    }
  }
  for (const auto& cur_file : target->data()) {
    if (cur_file == file->value())
      return true;
    if (cur_file.back() == '/' &&
        base::StartsWith(file->value(), cur_file, base::CompareCase::SENSITIVE))
      return true;
  }

  if (target->action_values().script().value() == file->value())
    return true;

  std::vector<SourceFile> outputs;
  target->action_values().GetOutputsAsSourceFiles(target, &outputs);
  for (const auto& cur_file : outputs) {
    if (cur_file == *file)
      return true;
  }
  return false;
}

void Analyzer::AddItemsDirectlyReferringToFile(
    const SourceFile* file,
    std::set<const Item*>* directly_affected_items) const {
  for (const auto* item : all_items_) {
    if (ItemRefersToFile(item, file))
      directly_affected_items->insert(item);
  }
}

void Analyzer::AddAllItemsReferringToItem(
    const Item* item,
    std::set<const Item*>* all_affected_items) const {
  if (all_affected_items->find(item) != all_affected_items->end())
    return;  // Already found this item.

  all_affected_items->insert(item);

  auto dep_begin = dep_map_.lower_bound(item);
  auto dep_end = dep_map_.upper_bound(item);
  for (auto cur_dep = dep_begin; cur_dep != dep_end; ++cur_dep)
    AddAllItemsReferringToItem(cur_dep->second, all_affected_items);
}

bool Analyzer::WereMainGNFilesModified(
    const std::set<const SourceFile*>& modified_files) const {
  for (const auto* file : modified_files) {
    if (*file == dot_file_)
      return true;

    if (*file == build_config_file_)
      return true;

    for (const auto& build_args_dependency_file :
         build_args_dependency_files_) {
      if (*file == build_args_dependency_file)
        return true;
    }
  }

  return false;
}
