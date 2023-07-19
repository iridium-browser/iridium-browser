// Copyright 2023 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/resolved_target_data.h"

#include "gn/config_values_extractors.h"

ResolvedTargetData::TargetInfo* ResolvedTargetData::GetTargetInfo(
    const Target* target) const {
  auto ret = targets_.PushBackWithIndex(target);
  if (ret.first) {
    infos_.push_back(std::make_unique<TargetInfo>(target));
  }
  return infos_[ret.second].get();
}

void ResolvedTargetData::ComputeLibInfo(TargetInfo* info) const {
  UniqueVector<SourceDir> all_lib_dirs;
  UniqueVector<LibFile> all_libs;

  for (ConfigValuesIterator iter(info->target); !iter.done(); iter.Next()) {
    const ConfigValues& cur = iter.cur();
    all_lib_dirs.Append(cur.lib_dirs());
    all_libs.Append(cur.libs());
  }
  for (const Target* dep : info->deps.linked_deps()) {
    if (!dep->IsFinal() || dep->output_type() == Target::STATIC_LIBRARY) {
      const TargetInfo* dep_info = GetTargetLibInfo(dep);
      all_lib_dirs.Append(dep_info->lib_dirs);
      all_libs.Append(dep_info->libs);
    }
  }

  info->lib_dirs = all_lib_dirs.release();
  info->libs = all_libs.release();
  info->has_lib_info = true;
}

void ResolvedTargetData::ComputeFrameworkInfo(TargetInfo* info) const {
  UniqueVector<SourceDir> all_framework_dirs;
  UniqueVector<std::string> all_frameworks;
  UniqueVector<std::string> all_weak_frameworks;

  for (ConfigValuesIterator iter(info->target); !iter.done(); iter.Next()) {
    const ConfigValues& cur = iter.cur();
    all_framework_dirs.Append(cur.framework_dirs());
    all_frameworks.Append(cur.frameworks());
    all_weak_frameworks.Append(cur.weak_frameworks());
  }
  for (const Target* dep : info->deps.linked_deps()) {
    if (!dep->IsFinal() || dep->output_type() == Target::STATIC_LIBRARY) {
      const TargetInfo* dep_info = GetTargetFrameworkInfo(dep);
      all_framework_dirs.Append(dep_info->framework_dirs);
      all_frameworks.Append(dep_info->frameworks);
      all_weak_frameworks.Append(dep_info->weak_frameworks);
    }
  }

  info->framework_dirs = all_framework_dirs.release();
  info->frameworks = all_frameworks.release();
  info->weak_frameworks = all_weak_frameworks.release();
  info->has_framework_info = true;
}

void ResolvedTargetData::ComputeHardDeps(TargetInfo* info) const {
  TargetSet all_hard_deps;
  for (const Target* dep : info->deps.linked_deps()) {
    // Direct hard dependencies
    if (info->target->hard_dep() || dep->hard_dep()) {
      all_hard_deps.insert(dep);
      continue;
    }
    // If |dep| is binary target and |dep| has no public header,
    // |this| target does not need to have |dep|'s hard_deps as its
    // hard_deps to start compiles earlier. Unless the target compiles a
    // Swift module (since they also generate a header that can be used
    // by the current target).
    if (dep->IsBinary() && !dep->all_headers_public() &&
        dep->public_headers().empty() && !dep->builds_swift_module()) {
      continue;
    }

    // Recursive hard dependencies of all dependencies.
    const TargetInfo* dep_info = GetTargetHardDeps(dep);
    all_hard_deps.insert(dep_info->hard_deps);
  }
  info->hard_deps = std::move(all_hard_deps);
  info->has_hard_deps = true;
}

void ResolvedTargetData::ComputeInheritedLibs(TargetInfo* info) const {
  TargetPublicPairListBuilder inherited_libraries;

  ComputeInheritedLibsFor(info->deps.public_deps(), true, &inherited_libraries);
  ComputeInheritedLibsFor(info->deps.private_deps(), false,
                          &inherited_libraries);

  info->inherited_libs = inherited_libraries.Build();
  info->has_inherited_libs = true;
}

void ResolvedTargetData::ComputeInheritedLibsFor(
    base::span<const Target*> deps,
    bool is_public,
    TargetPublicPairListBuilder* inherited_libraries) const {
  for (const Target* dep : deps) {
    // Direct dependent libraries.
    if (dep->output_type() == Target::STATIC_LIBRARY ||
        dep->output_type() == Target::SHARED_LIBRARY ||
        dep->output_type() == Target::RUST_LIBRARY ||
        dep->output_type() == Target::SOURCE_SET ||
        (dep->output_type() == Target::CREATE_BUNDLE &&
         dep->bundle_data().is_framework())) {
      inherited_libraries->Append(dep, is_public);
    }
    if (dep->output_type() == Target::SHARED_LIBRARY) {
      // Shared library dependendencies are inherited across public shared
      // library boundaries.
      //
      // In this case:
      //   EXE -> INTERMEDIATE_SHLIB --[public]--> FINAL_SHLIB
      // The EXE will also link to to FINAL_SHLIB. The public dependency means
      // that the EXE can use the headers in FINAL_SHLIB so the FINAL_SHLIB
      // will need to appear on EXE's link line.
      //
      // However, if the dependency is private:
      //   EXE -> INTERMEDIATE_SHLIB --[private]--> FINAL_SHLIB
      // the dependency will not be propagated because INTERMEDIATE_SHLIB is
      // not granting permission to call functions from FINAL_SHLIB. If EXE
      // wants to use functions (and link to) FINAL_SHLIB, it will need to do
      // so explicitly.
      //
      // Static libraries and source sets aren't inherited across shared
      // library boundaries because they will be linked into the shared
      // library. Rust dylib deps are handled above and transitive deps are
      // resolved by the compiler.
      const TargetInfo* dep_info = GetTargetInheritedLibs(dep);
      for (const auto& pair : dep_info->inherited_libs) {
        if (pair.target()->output_type() == Target::SHARED_LIBRARY &&
            pair.is_public()) {
          inherited_libraries->Append(pair.target(), is_public);
        }
      }
    } else if (!dep->IsFinal()) {
      // The current target isn't linked, so propagate linked deps and
      // libraries up the dependency tree.
      const TargetInfo* dep_info = GetTargetInheritedLibs(dep);
      for (const auto& pair : dep_info->inherited_libs) {
        // Proc macros are not linked into targets that depend on them, so do
        // not get inherited; they are consumed by the Rust compiler and only
        // need to be specified in --extern.
        if (pair.target()->output_type() != Target::RUST_PROC_MACRO)
          inherited_libraries->Append(pair.target(),
                                      is_public && pair.is_public());
      }
    } else if (dep->complete_static_lib()) {
      // Inherit only final targets through _complete_ static libraries.
      //
      // Inherited final libraries aren't linked into complete static
      // libraries. They are forwarded here so that targets that depend on
      // complete static libraries can link them in. Conversely, since
      // complete static libraries link in non-final targets, they shouldn't be
      // inherited.
      const TargetInfo* dep_info = GetTargetInheritedLibs(dep);
      for (const auto& pair : dep_info->inherited_libs) {
        if (pair.target()->IsFinal())
          inherited_libraries->Append(pair.target(),
                                      is_public && pair.is_public());
      }
    }
  }
}

void ResolvedTargetData::ComputeRustLibs(TargetInfo* info) const {
  RustLibsBuilder rust_libs;

  ComputeRustLibsFor(info->deps.public_deps(), true, &rust_libs);
  ComputeRustLibsFor(info->deps.private_deps(), false, &rust_libs);

  info->rust_inherited_libs = rust_libs.inherited.Build();
  info->rust_inheritable_libs = rust_libs.inheritable.Build();
  info->has_rust_libs = true;
}

void ResolvedTargetData::ComputeRustLibsFor(base::span<const Target*> deps,
                                            bool is_public,
                                            RustLibsBuilder* rust_libs) const {
  for (const Target* dep : deps) {
    // Collect Rust libraries that are accessible from the current target, or
    // transitively part of the current target.
    if (dep->output_type() == Target::STATIC_LIBRARY ||
        dep->output_type() == Target::SHARED_LIBRARY ||
        dep->output_type() == Target::SOURCE_SET ||
        dep->output_type() == Target::RUST_LIBRARY ||
        dep->output_type() == Target::GROUP) {
      // Here we have: `this` --[depends-on]--> `dep`
      //
      // The `this` target has direct access to `dep` since its a direct
      // dependency, regardless of the edge being a public_dep or not, so we
      // pass true for public-ness. Whereas, anything depending on `this` can
      // only gain direct access to `dep` if the edge between `this` and `dep`
      // is public, so we pass `is_public`.
      //
      // TODO(danakj): We should only need to track Rust rlibs or dylibs here,
      // as it's used for passing to rustc with --extern. We currently track
      // everything then drop non-Rust libs in
      // ninja_rust_binary_target_writer.cc.
      rust_libs->inherited.Append(dep, true);
      rust_libs->inheritable.Append(dep, is_public);

      const TargetInfo* dep_info = GetTargetRustLibs(dep);
      rust_libs->inherited.AppendInherited(dep_info->rust_inheritable_libs,
                                           true);
      rust_libs->inheritable.AppendInherited(dep_info->rust_inheritable_libs,
                                             is_public);
    } else if (dep->output_type() == Target::RUST_PROC_MACRO) {
      // Proc-macros are inherited as a transitive dependency, but the things
      // they depend on can't be used elsewhere, as the proc macro is not
      // linked into the target (as it's only used during compilation).
      rust_libs->inherited.Append(dep, true);
      rust_libs->inheritable.Append(dep, is_public);
    }
  }
}

void ResolvedTargetData::ComputeSwiftValues(TargetInfo* info) const {
  UniqueVector<const Target*> modules;
  UniqueVector<const Target*> public_modules;
  const Target* target = info->target;

  for (const Target* dep : info->deps.public_deps()) {
    if (dep->toolchain() != target->toolchain() &&
        !dep->toolchain()->propagates_configs()) {
      continue;
    }

    const TargetInfo* dep_info = GetTargetSwiftValues(dep);
    if (dep_info->swift_values.get()) {
      const auto& public_deps = dep_info->swift_values->public_modules;
      modules.Append(public_deps);
      public_modules.Append(public_deps);
    }
  }

  for (const Target* dep : info->deps.private_deps()) {
    if (dep->toolchain() != target->toolchain() &&
        !dep->toolchain()->propagates_configs()) {
      continue;
    }
    const TargetInfo* dep_info = GetTargetSwiftValues(dep);
    if (dep_info->swift_values.get()) {
      modules.Append(dep_info->swift_values->public_modules);
    }
  }

  if (target->builds_swift_module())
    public_modules.push_back(target);

  if (!modules.empty() || !public_modules.empty()) {
    info->swift_values = std::make_unique<TargetInfo::SwiftValues>(
        modules.release(), public_modules.release());
  }
  info->has_swift_values = true;
}
