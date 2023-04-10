// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/rust_project_writer.h"

#include <fstream>
#include <optional>
#include <sstream>
#include <tuple>

#include "base/json/string_escape.h"
#include "base/strings/string_split.h"
#include "gn/builder.h"
#include "gn/deps_iterator.h"
#include "gn/ninja_target_command_util.h"
#include "gn/rust_project_writer_helpers.h"
#include "gn/rust_tool.h"
#include "gn/source_file.h"
#include "gn/string_output_buffer.h"
#include "gn/tool.h"

#if defined(OS_WINDOWS)
#define NEWLINE "\r\n"
#else
#define NEWLINE "\n"
#endif

// Current structure of rust-project.json output file
//
// {
//    "sysroot": "path/to/rust/sysroot",
//    "crates": [
//        {
//            "deps": [
//                {
//                    "crate": 1, // index into crate array
//                    "name": "alloc" // extern name of dependency
//                },
//            ],
//            "source": [
//                "include_dirs": [
//                     "some/source/root",
//                     "some/gen/dir",
//                ],
//                "exclude_dirs": []
//            },
//            "edition": "2021", // edition of crate
//            "cfg": [
//              "unix", // "atomic" value config options
//              "rust_panic=\"abort\""", // key="value" config options
//            ]
//            "root_module": "absolute path to crate",
//            "label": "//path/target:value", // GN target for the crate
//            "target": "x86_64-unknown-linux" // optional rustc target
//        },
// }
//

bool RustProjectWriter::RunAndWriteFiles(const BuildSettings* build_settings,
                                         const Builder& builder,
                                         const std::string& file_name,
                                         bool quiet,
                                         Err* err) {
  SourceFile output_file = build_settings->build_dir().ResolveRelativeFile(
      Value(nullptr, file_name), err);
  if (output_file.is_null())
    return false;

  base::FilePath output_path = build_settings->GetFullPath(output_file);

  std::vector<const Target*> all_targets = builder.GetAllResolvedTargets();

  StringOutputBuffer out_buffer;
  std::ostream out(&out_buffer);

  RenderJSON(build_settings, all_targets, out);
  return out_buffer.WriteToFileIfChanged(output_path, err);
}

// Map of Targets to their index in the crates list (for linking dependencies to
// their indexes).
using TargetIndexMap = std::unordered_map<const Target*, uint32_t>;

// A collection of Targets.
using TargetsVector = UniqueVector<const Target*>;

// Get the Rust deps for a target, recursively expanding OutputType::GROUPS
// that are present in the GN structure.  This will return a flattened list of
// deps from the groups, but will not expand a Rust lib dependency to find any
// transitive Rust dependencies.
void GetRustDeps(const Target* target, TargetsVector* rust_deps) {
  for (const auto& pair : target->GetDeps(Target::DEPS_LINKED)) {
    const Target* dep = pair.ptr;

    if (dep->source_types_used().RustSourceUsed()) {
      // Include any Rust dep.
      rust_deps->push_back(dep);
    } else if (dep->output_type() == Target::OutputType::GROUP) {
      // Inspect (recursively) any group to see if it contains Rust deps.
      GetRustDeps(dep, rust_deps);
    }
  }
}
TargetsVector GetRustDeps(const Target* target) {
  TargetsVector deps;
  GetRustDeps(target, &deps);
  return deps;
}

std::vector<std::string> ExtractCompilerArgs(const Target* target) {
  std::vector<std::string> args;
  for (ConfigValuesIterator iter(target); !iter.done(); iter.Next()) {
    auto rustflags = iter.cur().rustflags();
    for (auto flag_iter = rustflags.begin(); flag_iter != rustflags.end();
         flag_iter++) {
      args.push_back(*flag_iter);
    }
  }
  return args;
}

std::optional<std::string> FindArgValue(const char* arg,
                                        const std::vector<std::string>& args) {
  for (auto current = args.begin(); current != args.end();) {
    // capture the current value
    auto previous = *current;
    // and increment
    current++;

    // if the previous argument matches `arg`, and after the above increment the
    // end hasn't been reached, this current argument is the desired value.
    if (previous == arg && current != args.end()) {
      return std::make_optional(*current);
    }
  }
  return std::nullopt;
}

std::optional<std::string> FindArgValueAfterPrefix(
    const std::string& prefix,
    const std::vector<std::string>& args) {
  for (auto arg : args) {
    if (!arg.compare(0, prefix.size(), prefix)) {
      auto value = arg.substr(prefix.size());
      return std::make_optional(value);
    }
  }
  return std::nullopt;
}

std::vector<std::string> FindAllArgValuesAfterPrefix(
    const std::string& prefix,
    const std::vector<std::string>& args) {
  std::vector<std::string> values;
  for (auto arg : args) {
    if (!arg.compare(0, prefix.size(), prefix)) {
      auto value = arg.substr(prefix.size());
      values.push_back(value);
    }
  }
  return values;
}

void AddTarget(const BuildSettings* build_settings,
               const Target* target,
               TargetIndexMap& lookup,
               CrateList& crate_list) {
  if (lookup.find(target) != lookup.end()) {
    // If target is already in the lookup, we don't add it again.
    return;
  }

  auto compiler_args = ExtractCompilerArgs(target);
  auto compiler_target = FindArgValue("--target", compiler_args);
  auto crate_deps = GetRustDeps(target);

  // Add all dependencies of this crate, before this crate.
  for (const auto& dep : crate_deps) {
    AddTarget(build_settings, dep, lookup, crate_list);
  }

  // The index of a crate is its position (0-based) in the list of crates.
  size_t crate_id = crate_list.size();

  // Add the target to the crate lookup.
  lookup.insert(std::make_pair(target, crate_id));

  SourceFile crate_root = target->rust_values().crate_root();
  std::string crate_label = target->label().GetUserVisibleName(false);

  auto edition =
      FindArgValueAfterPrefix(std::string("--edition="), compiler_args);
  if (!edition.has_value()) {
    edition = FindArgValue("--edition", compiler_args);
  }

  auto gen_dir = GetBuildDirForTargetAsOutputFile(target, BuildDirType::GEN);

  Crate crate = Crate(crate_root, gen_dir, crate_id, crate_label,
                      edition.value_or("2015"));

  crate.SetCompilerArgs(compiler_args);
  if (compiler_target.has_value())
    crate.SetCompilerTarget(compiler_target.value());

  ConfigList cfgs =
      FindAllArgValuesAfterPrefix(std::string("--cfg="), compiler_args);

  crate.AddConfigItem("test");
  crate.AddConfigItem("debug_assertions");

  for (auto& cfg : cfgs) {
    crate.AddConfigItem(cfg);
  }

  // If it's a proc macro, record its output location so IDEs can invoke it.
  auto rust_tool =
      target->toolchain()->GetToolForTargetFinalOutputAsRust(target);
  if (std::string_view(rust_tool->name()) ==
      std::string_view(RustTool::kRsToolMacro)) {
    auto outputs = target->computed_outputs();
    if (outputs.size() > 0) {
      crate.SetIsProcMacro(outputs[0]);
    }
  }

  // Note any environment variables. These may be used by proc macros
  // invoked by the current crate (so we want to record these for all crates,
  // not just proc macro crates)
  for (const auto& env_var : target->config_values().rustenv()) {
    std::vector<std::string> parts = base::SplitString(
        env_var, "=", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    if (parts.size() >= 2) {
      crate.AddRustenv(parts[0], parts[1]);
    }
  }

  // Add the rest of the crate dependencies.
  for (const auto& dep : crate_deps) {
    auto idx = lookup[dep];
    crate.AddDependency(idx, dep->rust_values().crate_name());
  }

  crate_list.push_back(crate);
}

void WriteCrates(const BuildSettings* build_settings,
                 CrateList& crate_list,
                 std::optional<std::string>& sysroot,
                 std::ostream& rust_project) {
  rust_project << "{" NEWLINE;

  // If a sysroot was found, then that can be used to tell rust-analyzer where
  // to find the sysroot (and associated tools like the
  // 'rust-analyzer-proc-macro-srv` proc-macro server that matches the abi used
  // by 'rustc'
  if (sysroot.has_value()) {
    base::FilePath rebased_out_dir =
        build_settings->GetFullPath(build_settings->build_dir());
    auto sysroot_path = FilePathToUTF8(rebased_out_dir) + sysroot.value();
    rust_project << "  \"sysroot\": \"" << sysroot_path << "\"," NEWLINE;
  }

  rust_project << "  \"crates\": [";
  bool first_crate = true;
  for (auto& crate : crate_list) {
    if (!first_crate)
      rust_project << ",";
    first_crate = false;

    auto crate_module =
        FilePathToUTF8(build_settings->GetFullPath(crate.root()));

    rust_project << NEWLINE << "    {" NEWLINE
                 << "      \"crate_id\": " << crate.index() << "," NEWLINE
                 << "      \"root_module\": \"" << crate_module << "\"," NEWLINE
                 << "      \"label\": \"" << crate.label() << "\"," NEWLINE
                 << "      \"source\": {" NEWLINE
                 << "          \"include_dirs\": [" NEWLINE
                 << "               \""
                 << FilePathToUTF8(
                        build_settings->GetFullPath(crate.root().GetDir()))
                 << "\"";
    auto gen_dir = crate.gen_dir();
    if (gen_dir.has_value()) {
      auto gen_dir_path = FilePathToUTF8(
          build_settings->GetFullPath(gen_dir->AsSourceDir(build_settings)));
      rust_project << "," NEWLINE << "               \"" << gen_dir_path
                   << "\"" NEWLINE;
    } else {
      rust_project << NEWLINE;
    }
    rust_project << "          ]," NEWLINE
                 << "          \"exclude_dirs\": []" NEWLINE
                 << "      }," NEWLINE;

    auto compiler_target = crate.CompilerTarget();
    if (compiler_target.has_value()) {
      rust_project << "      \"target\": \"" << compiler_target.value()
                   << "\"," NEWLINE;
    }

    auto compiler_args = crate.CompilerArgs();
    if (!compiler_args.empty()) {
      rust_project << "      \"compiler_args\": [";
      bool first_arg = true;
      for (auto& arg : crate.CompilerArgs()) {
        if (!first_arg)
          rust_project << ", ";
        first_arg = false;

        std::string escaped_arg;
        base::EscapeJSONString(arg, false, &escaped_arg);

        rust_project << "\"" << escaped_arg << "\"";
      }
      rust_project << "]," << NEWLINE;
    }

    rust_project << "      \"deps\": [";
    bool first_dep = true;
    for (auto& dep : crate.dependencies()) {
      if (!first_dep)
        rust_project << ",";
      first_dep = false;

      rust_project << NEWLINE << "        {" NEWLINE
                   << "          \"crate\": " << dep.first << "," NEWLINE
                   << "          \"name\": \"" << dep.second << "\"" NEWLINE
                   << "        }";
    }
    rust_project << NEWLINE "      ]," NEWLINE;  // end dep list

    rust_project << "      \"edition\": \"" << crate.edition() << "\"," NEWLINE;

    auto proc_macro_target = crate.proc_macro_path();
    if (proc_macro_target.has_value()) {
      rust_project << "      \"is_proc_macro\": true," NEWLINE;
      auto so_location = FilePathToUTF8(build_settings->GetFullPath(
          proc_macro_target->AsSourceFile(build_settings)));
      rust_project << "      \"proc_macro_dylib_path\": \"" << so_location
                   << "\"," NEWLINE;
    }

    rust_project << "      \"cfg\": [";
    bool first_cfg = true;
    for (const auto& cfg : crate.configs()) {
      if (!first_cfg)
        rust_project << ",";
      first_cfg = false;

      std::string escaped_config;
      base::EscapeJSONString(cfg, false, &escaped_config);

      rust_project << NEWLINE;
      rust_project << "        \"" << escaped_config << "\"";
    }
    rust_project << NEWLINE;
    rust_project << "      ]";  // end cfgs

    if (!crate.rustenv().empty()) {
      rust_project << "," NEWLINE;
      rust_project << "      \"env\": {";
      bool first_env = true;
      for (const auto& env : crate.rustenv()) {
        if (!first_env)
          rust_project << ",";
        first_env = false;
        std::string escaped_key, escaped_val;
        base::EscapeJSONString(env.first, false, &escaped_key);
        base::EscapeJSONString(env.second, false, &escaped_val);
        rust_project << NEWLINE;
        rust_project << "        \"" << escaped_key << "\": \"" << escaped_val
                     << "\"";
      }

      rust_project << NEWLINE;
      rust_project << "      }" NEWLINE;  // end env vars
    } else {
      rust_project << NEWLINE;
    }
    rust_project << "    }";  // end crate
  }
  rust_project << NEWLINE "  ]" NEWLINE;  // end crate list
  rust_project << "}" NEWLINE;
}

void RustProjectWriter::RenderJSON(const BuildSettings* build_settings,
                                   std::vector<const Target*>& all_targets,
                                   std::ostream& rust_project) {
  TargetIndexMap lookup;
  CrateList crate_list;
  std::optional<std::string> rust_sysroot;

  // All the crates defined in the project.
  for (const auto* target : all_targets) {
    if (!target->IsBinary() || !target->source_types_used().RustSourceUsed())
      continue;

    AddTarget(build_settings, target, lookup, crate_list);

    // If a sysroot hasn't been found, see if we can find one using this target.
    if (!rust_sysroot.has_value()) {
      auto rust_tool =
          target->toolchain()->GetToolForTargetFinalOutputAsRust(target);
      auto sysroot = rust_tool->GetSysroot();
      if (sysroot != "")
        rust_sysroot = sysroot;
    }
  }

  WriteCrates(build_settings, crate_list, rust_sysroot, rust_project);
}
