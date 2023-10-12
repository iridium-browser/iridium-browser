// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_NINJA_C_BINARY_TARGET_WRITER_H_
#define TOOLS_GN_NINJA_C_BINARY_TARGET_WRITER_H_

#include "gn/config_values.h"
#include "gn/ninja_binary_target_writer.h"
#include "gn/toolchain.h"
#include "gn/unique_vector.h"

struct EscapeOptions;
struct ModuleDep;

// Writes a .ninja file for a binary target type (an executable, a shared
// library, or a static library).
class NinjaCBinaryTargetWriter : public NinjaBinaryTargetWriter {
 public:
  NinjaCBinaryTargetWriter(const Target* target, std::ostream& out);
  ~NinjaCBinaryTargetWriter() override;

  void Run() override;

 private:
  using OutputFileSet = std::set<OutputFile>;

  // Writes all flags for the compiler: includes, defines, cflags, etc.
  void WriteCompilerVars(const std::vector<ModuleDep>& module_dep_info);

  // Write module_deps or module_deps_no_self flags for clang modulemaps.
  void WriteModuleDepsSubstitution(
      const Substitution* substitution,
      const std::vector<ModuleDep>& module_dep_info,
      bool include_self);

  // Writes build lines required for precompiled headers. Any generated
  // object files will be appended to the |object_files|. Any generated
  // non-object files (for instance, .gch files from a GCC toolchain, are
  // appended to |other_files|).
  //
  // input_deps is the stamp file collecting the dependencies required before
  // compiling this target. It will be empty if there are no input deps.
  void WritePCHCommands(const std::vector<OutputFile>& input_deps,
                        const std::vector<OutputFile>& order_only_deps,
                        std::vector<OutputFile>* object_files,
                        std::vector<OutputFile>* other_files);

  // Writes a .pch compile build line for a language type.
  void WritePCHCommand(const Substitution* flag_type,
                       const char* tool_name,
                       CTool::PrecompiledHeaderType header_type,
                       const std::vector<OutputFile>& input_deps,
                       const std::vector<OutputFile>& order_only_deps,
                       std::vector<OutputFile>* object_files,
                       std::vector<OutputFile>* other_files);

  void WriteGCCPCHCommand(const Substitution* flag_type,
                          const char* tool_name,
                          const std::vector<OutputFile>& input_deps,
                          const std::vector<OutputFile>& order_only_deps,
                          std::vector<OutputFile>* gch_files);

  void WriteWindowsPCHCommand(const Substitution* flag_type,
                              const char* tool_name,
                              const std::vector<OutputFile>& input_deps,
                              const std::vector<OutputFile>& order_only_deps,
                              std::vector<OutputFile>* object_files);

  // pch_deps are additional dependencies to run before the rule. They are
  // expected to abide by the naming conventions specified by GetPCHOutputFiles.
  //
  // order_only_dep are the dependencies that must be run before doing any
  // compiles.
  //
  // The files produced by the compiler will be added to two output vectors.
  void WriteSources(const std::vector<OutputFile>& pch_deps,
                    const std::vector<OutputFile>& input_deps,
                    const std::vector<OutputFile>& order_only_deps,
                    const std::vector<ModuleDep>& module_dep_info,
                    std::vector<OutputFile>* object_files,
                    std::vector<SourceFile>* other_files);
  void WriteSwiftSources(const std::vector<OutputFile>& input_deps,
                         const std::vector<OutputFile>& order_only_deps,
                         std::vector<OutputFile>* object_files);

  // Writes the stamp line for a source set. These are not linked.
  void WriteSourceSetStamp(const std::vector<OutputFile>& object_files);

  void WriteLinkerStuff(const std::vector<OutputFile>& object_files,
                        const std::vector<SourceFile>& other_files,
                        const std::vector<OutputFile>& input_deps);
  void WriteOutputSubstitutions();
  void WriteLibsList(const std::string& label,
                     const std::vector<OutputFile>& libs);

  // Writes the implicit dependencies for the link or stamp line. This is
  // the "||" and everything following it on the ninja line.
  //
  // The order-only dependencies are the non-linkable deps passed in as an
  // argument, plus the data file dependencies in the target.
  void WriteOrderOnlyDependencies(
      const UniqueVector<const Target*>& non_linkable_deps);

  // Checks for duplicates in the given list of output files. If any duplicates
  // are found, throws an error and return false.
  bool CheckForDuplicateObjectFiles(const std::vector<OutputFile>& files) const;

  const CTool* tool_;

  NinjaCBinaryTargetWriter(const NinjaCBinaryTargetWriter&) = delete;
  NinjaCBinaryTargetWriter& operator=(const NinjaCBinaryTargetWriter&) = delete;
};

#endif  // TOOLS_GN_NINJA_C_BINARY_TARGET_WRITER_H_
