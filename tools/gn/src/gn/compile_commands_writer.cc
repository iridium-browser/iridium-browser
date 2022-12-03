// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/compile_commands_writer.h"

#include <sstream>

#include "base/json/string_escape.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "gn/builder.h"
#include "gn/c_substitution_type.h"
#include "gn/c_tool.h"
#include "gn/config_values_extractors.h"
#include "gn/deps_iterator.h"
#include "gn/escape.h"
#include "gn/ninja_target_command_util.h"
#include "gn/path_output.h"
#include "gn/string_output_buffer.h"
#include "gn/substitution_writer.h"

// Structure of JSON output file
// [
//   {
//      "directory": "The build directory."
//      "file": "The main source file processed by this compilation step.
//               Must be absolute or relative to the above build directory."
//      "command": "The compile command executed."
//   }
//   ...
// ]

namespace {

#if defined(OS_WIN)
const char kPrettyPrintLineEnding[] = "\r\n";
#else
const char kPrettyPrintLineEnding[] = "\n";
#endif

struct CompileFlags {
  std::string includes;
  std::string defines;
  std::string cflags;
  std::string cflags_c;
  std::string cflags_cc;
  std::string cflags_objc;
  std::string cflags_objcc;
  std::string framework_dirs;
  std::string frameworks;
};

// Helper template function to call RecursiveTargetConfigToStream<std::string>
// and return the JSON-escaped resulting string.
//
// NOTE: The Windows compiler cannot properly deduce the first parameter type
// so pass it at each call site to ensure proper builds for this platform.
template <typename T, typename Writer>
std::string FlagsGetter(RecursiveWriterConfig config,
                        const Target* target,
                        const std::vector<T>& (ConfigValues::*getter)() const,
                        const Writer& writer) {
  std::string result;
  std::ostringstream out;
  RecursiveTargetConfigToStream<T>(config, target, getter, writer, out);
  base::EscapeJSONString(out.str(), false, &result);
  return result;
}

void SetupCompileFlags(const Target* target,
                       PathOutput& path_output,
                       EscapeOptions opts,
                       CompileFlags& flags) {
  bool has_precompiled_headers =
      target->config_values().has_precompiled_headers();

  flags.defines = FlagsGetter<std::string>(
      kRecursiveWriterSkipDuplicates, target, &ConfigValues::defines,
      DefineWriter(ESCAPE_COMPILATION_DATABASE));

  flags.framework_dirs = FlagsGetter<SourceDir>(
      kRecursiveWriterSkipDuplicates, target, &ConfigValues::framework_dirs,
      FrameworkDirsWriter(path_output, "-F"));

  flags.frameworks = FlagsGetter<std::string>(
      kRecursiveWriterSkipDuplicates, target, &ConfigValues::frameworks,
      FrameworksWriter(ESCAPE_COMPILATION_DATABASE, "-framework"));
  flags.frameworks += FlagsGetter<std::string>(
      kRecursiveWriterSkipDuplicates, target, &ConfigValues::weak_frameworks,
      FrameworksWriter(ESCAPE_COMPILATION_DATABASE, "-weak_framework"));

  flags.includes = FlagsGetter<SourceDir>(kRecursiveWriterSkipDuplicates,
                                          target, &ConfigValues::include_dirs,
                                          IncludeWriter(path_output));

  // Helper lambda to call WriteOneFlag() and return the resulting
  // escaped JSON string.
  auto one_flag = [&](RecursiveWriterConfig config,
                      const Substitution* substitution,
                      bool has_precompiled_headers, const char* tool_name,
                      const std::vector<std::string>& (ConfigValues::*getter)()
                          const) -> std::string {
    std::string result;
    std::ostringstream out;
    WriteOneFlag(config, target, substitution, has_precompiled_headers,
                 tool_name, getter, opts, path_output, out,
                 /*write_substitution=*/false, /*indent=*/false);
    base::EscapeJSONString(out.str(), false, &result);
    return result;
  };

  flags.cflags = one_flag(kRecursiveWriterKeepDuplicates, &CSubstitutionCFlags,
                          false, Tool::kToolNone, &ConfigValues::cflags);

  flags.cflags_c = one_flag(kRecursiveWriterKeepDuplicates,
                            &CSubstitutionCFlagsC, has_precompiled_headers,
                            CTool::kCToolCc, &ConfigValues::cflags_c);

  flags.cflags_cc = one_flag(kRecursiveWriterKeepDuplicates,
                             &CSubstitutionCFlagsCc, has_precompiled_headers,
                             CTool::kCToolCxx, &ConfigValues::cflags_cc);

  flags.cflags_objc = one_flag(
      kRecursiveWriterKeepDuplicates, &CSubstitutionCFlagsObjC,
      has_precompiled_headers, CTool::kCToolObjC, &ConfigValues::cflags_objc);

  flags.cflags_objcc =
      one_flag(kRecursiveWriterKeepDuplicates, &CSubstitutionCFlagsObjCc,
               has_precompiled_headers, CTool::kCToolObjCxx,
               &ConfigValues::cflags_objcc);
}

void WriteFile(const SourceFile& source,
               PathOutput& path_output,
               std::ostream& out) {
  std::ostringstream rel_source_path;
  out << "    \"file\": \"";
  path_output.WriteFile(out, source);
}

void WriteDirectory(std::string build_dir, std::ostream& out) {
  out << "\",";
  out << kPrettyPrintLineEnding;
  out << "    \"directory\": \"";
  out << build_dir;
  out << "\",";
}

void WriteCommand(const Target* target,
                  const SourceFile& source,
                  const CompileFlags& flags,
                  std::vector<OutputFile>& tool_outputs,
                  PathOutput& path_output,
                  SourceFile::Type source_type,
                  const char* tool_name,
                  EscapeOptions opts,
                  std::ostream& out) {
  EscapeOptions no_quoting(opts);
  no_quoting.inhibit_quoting = true;
  const Tool* tool = target->toolchain()->GetTool(tool_name);

  out << kPrettyPrintLineEnding;
  out << "    \"command\": \"";

  for (const auto& range : tool->command().ranges()) {
    // TODO: this is emitting a bonus space prior to each substitution.
    if (range.type == &SubstitutionLiteral) {
      EscapeJSONStringToStream(out, range.literal, no_quoting);
    } else if (range.type == &SubstitutionOutput) {
      path_output.WriteFiles(out, tool_outputs);
    } else if (range.type == &CSubstitutionDefines) {
      out << flags.defines;
    } else if (range.type == &CSubstitutionFrameworkDirs) {
      out << flags.framework_dirs;
    } else if (range.type == &CSubstitutionFrameworks) {
      out << flags.frameworks;
    } else if (range.type == &CSubstitutionIncludeDirs) {
      out << flags.includes;
    } else if (range.type == &CSubstitutionCFlags) {
      out << flags.cflags;
    } else if (range.type == &CSubstitutionCFlagsC) {
      if (source_type == SourceFile::SOURCE_C)
        out << flags.cflags_c;
    } else if (range.type == &CSubstitutionCFlagsCc) {
      if (source_type == SourceFile::SOURCE_CPP)
        out << flags.cflags_cc;
    } else if (range.type == &CSubstitutionCFlagsObjC) {
      if (source_type == SourceFile::SOURCE_M)
        out << flags.cflags_objc;
    } else if (range.type == &CSubstitutionCFlagsObjCc) {
      if (source_type == SourceFile::SOURCE_MM)
        out << flags.cflags_objcc;
    } else if (range.type == &SubstitutionLabel ||
               range.type == &SubstitutionLabelName ||
               range.type == &SubstitutionLabelNoToolchain ||
               range.type == &SubstitutionRootGenDir ||
               range.type == &SubstitutionRootOutDir ||
               range.type == &SubstitutionTargetGenDir ||
               range.type == &SubstitutionTargetOutDir ||
               range.type == &SubstitutionTargetOutputName ||
               range.type == &SubstitutionSource ||
               range.type == &SubstitutionSourceNamePart ||
               range.type == &SubstitutionSourceFilePart ||
               range.type == &SubstitutionSourceDir ||
               range.type == &SubstitutionSourceRootRelativeDir ||
               range.type == &SubstitutionSourceGenDir ||
               range.type == &SubstitutionSourceOutDir ||
               range.type == &SubstitutionSourceTargetRelative) {
      EscapeStringToStream(out,
                           SubstitutionWriter::GetCompilerSubstitution(
                               target, source, range.type),
                           opts);
    } else {
      // Other flags shouldn't be relevant to compiling C/C++/ObjC/ObjC++
      // source files.
      NOTREACHED() << "Unsupported substitution for this type of target : "
                   << range.type->name;
      continue;
    }
  }
}

void OutputJSON(const BuildSettings* build_settings,
                std::vector<const Target*>& all_targets,
                std::ostream& out) {
  out << '[';
  out << kPrettyPrintLineEnding;
  bool first = true;
  auto build_dir = build_settings->GetFullPath(build_settings->build_dir())
                       .StripTrailingSeparators();
  std::vector<OutputFile> tool_outputs;  // Prevent reallocation in loop.

  EscapeOptions opts;
  opts.mode = ESCAPE_NINJA_PREFORMATTED_COMMAND;

  for (const auto* target : all_targets) {
    if (!target->IsBinary())
      continue;

    // Precompute values that are the same for all sources in a target to avoid
    // computing for every source.

    PathOutput path_output(
        target->settings()->build_settings()->build_dir(),
        target->settings()->build_settings()->root_path_utf8(),
        ESCAPE_NINJA_COMMAND);

    CompileFlags flags;
    SetupCompileFlags(target, path_output, opts, flags);

    for (const auto& source : target->sources()) {
      // If this source is not a C/C++/ObjC/ObjC++ source (not header) file,
      // continue as it does not belong in the compilation database.
      const SourceFile::Type source_type = source.GetType();
      if (source_type != SourceFile::SOURCE_CPP &&
          source_type != SourceFile::SOURCE_C &&
          source_type != SourceFile::SOURCE_M &&
          source_type != SourceFile::SOURCE_MM)
        continue;

      const char* tool_name = Tool::kToolNone;
      if (!target->GetOutputFilesForSource(source, &tool_name, &tool_outputs))
        continue;

      if (!first) {
        out << ',';
        out << kPrettyPrintLineEnding;
      }
      first = false;
      out << "  {";
      out << kPrettyPrintLineEnding;

      WriteFile(source, path_output, out);
      WriteDirectory(base::StringPrintf("%" PRIsFP, PATH_CSTR(build_dir)), out);
      WriteCommand(target, source, flags, tool_outputs, path_output,
                   source_type, tool_name, opts, out);
      out << "\"";
      out << kPrettyPrintLineEnding;
      out << "  }";
    }
  }

  out << kPrettyPrintLineEnding;
  out << "]";
  out << kPrettyPrintLineEnding;
}

}  // namespace

std::string CompileCommandsWriter::RenderJSON(
    const BuildSettings* build_settings,
    std::vector<const Target*>& all_targets) {
  StringOutputBuffer json;
  std::ostream out(&json);
  OutputJSON(build_settings, all_targets, out);
  return json.str();
}

bool CompileCommandsWriter::RunAndWriteFiles(
    const BuildSettings* build_settings,
    const std::vector<const Target*>& all_targets,
    const std::vector<LabelPattern>& patterns,
    const std::optional<std::string>& legacy_target_filters,
    const base::FilePath& output_path,
    Err* err) {
  std::vector<const Target*> to_write = CollectTargets(
      build_settings, all_targets, patterns, legacy_target_filters, err);
  if (err->has_error())
    return false;

  StringOutputBuffer json;
  std::ostream output_to_json(&json);
  OutputJSON(build_settings, to_write, output_to_json);

  return json.WriteToFileIfChanged(output_path, err);
}

std::vector<const Target*> CompileCommandsWriter::CollectTargets(
    const BuildSettings* build_setting,
    const std::vector<const Target*>& all_targets,
    const std::vector<LabelPattern>& patterns,
    const std::optional<std::string>& legacy_target_filters,
    Err* err) {
  if (legacy_target_filters && legacy_target_filters->empty()) {
    // The legacy filter was specified but has no parameter. This matches
    // everything and we can skip any other kinds of matching.
    return all_targets;
  }

  // Collect the first level of target matches. These are the ones that the
  // patterns match directly.
  std::vector<const Target*> input_targets;
  for (const Target* target : all_targets) {
    if (LabelPattern::VectorMatches(patterns, target->label()))
      input_targets.push_back(target);
  }

  // Add in any legacy filter matches.
  if (legacy_target_filters) {
    std::vector<const Target*> legacy_matches =
        FilterLegacyTargets(all_targets, *legacy_target_filters);

    // This can produce some duplicates with the patterns but the "collect
    // deps" phase will eliminate them.
    input_targets.insert(input_targets.end(), legacy_matches.begin(),
                         legacy_matches.end());
  }

  return CollectDepsOfMatches(input_targets);
}

std::vector<const Target*> CompileCommandsWriter::CollectDepsOfMatches(
    const std::vector<const Target*>& input_targets) {
  // The current set of matched targets.
  TargetSet collected;

  // Represents the next layer of the breadth-first seach. These are all targets
  // that we haven't checked so far.
  std::vector<const Target*> frontier;

  // Collect the first level of target matches specified in the input. There may
  // be duplicates so we still need to do the set checking.
  // patterns match directly.
  for (const Target* target : input_targets) {
    if (!collected.contains(target)) {
      collected.add(target);
      frontier.push_back(target);
    }
  }

  // Collects the dependencies for the next level of iteration. This could be
  // inside the loop but is kept outside to avoid reallocating in every
  // iteration.
  std::vector<const Target*> next_frontier;

  // Loop for each level of the search.
  while (!frontier.empty()) {
    for (const Target* target : frontier) {
      // Check the target's dependencies.
      for (const auto& pair : target->GetDeps(Target::DEPS_ALL)) {
        if (!collected.contains(pair.ptr)) {
          // New dependency found.
          collected.add(pair.ptr);
          next_frontier.push_back(pair.ptr);
        }
      }
    }

    // Swap to the new level and clear out the next one without deallocating the
    // buffer (in most STL implementations, clear() doesn't free the existing
    // buffer).
    std::swap(frontier, next_frontier);
    next_frontier.clear();
  }

  // Convert to vector for output.
  std::vector<const Target*> output;
  output.reserve(collected.size());
  for (const Target* target : collected) {
    output.push_back(target);
  }
  return output;
}

std::vector<const Target*> CompileCommandsWriter::FilterLegacyTargets(
    const std::vector<const Target*>& all_targets,
    const std::string& target_filter_string) {
  std::set<std::string> target_filters_set;
  for (auto& target :
       base::SplitString(target_filter_string, ",", base::TRIM_WHITESPACE,
                         base::SPLIT_WANT_NONEMPTY)) {
    target_filters_set.insert(target);
  }

  std::vector<const Target*> result;
  for (auto& target : all_targets) {
    if (target_filters_set.count(target->label().name()))
      result.push_back(target);
  }

  return result;
}
