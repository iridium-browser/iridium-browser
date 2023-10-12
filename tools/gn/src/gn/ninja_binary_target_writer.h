// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_NINJA_BINARY_TARGET_WRITER_H_
#define TOOLS_GN_NINJA_BINARY_TARGET_WRITER_H_

#include "gn/c_tool.h"
#include "gn/config_values.h"
#include "gn/ninja_target_writer.h"
#include "gn/toolchain.h"
#include "gn/unique_vector.h"

struct EscapeOptions;

// Writes a .ninja file for a binary target type (an executable, a shared
// library, or a static library).
class NinjaBinaryTargetWriter : public NinjaTargetWriter {
 public:
  NinjaBinaryTargetWriter(const Target* target, std::ostream& out);
  ~NinjaBinaryTargetWriter() override;

  void Run() override;

 protected:
  // Structure used to return the classified deps from |GetDeps| method.
  struct ClassifiedDeps {
    UniqueVector<OutputFile> extra_object_files;
    UniqueVector<const Target*> linkable_deps;
    UniqueVector<const Target*> non_linkable_deps;
    UniqueVector<const Target*> framework_deps;
    UniqueVector<const Target*> swiftmodule_deps;
  };

  // Writes to the output stream a stamp rule for inputs, and
  // returns the file to be appended to source rules that encodes the
  // implicit dependencies for the current target.
  // If num_stamp_uses is small, this might return all input dependencies
  // directly, without writing a stamp file.
  // If there are no implicit dependencies and no extra target dependencies
  // are passed in, this returns an empty vector.
  std::vector<OutputFile> WriteInputsStampAndGetDep(
      size_t num_stamp_uses) const;

  // Gets all target dependencies and classifies them, as well as accumulates
  // object files from source sets we need to link.
  ClassifiedDeps GetClassifiedDeps() const;

  // Classifies the dependency as linkable or nonlinkable with the current
  // target, adding it to the appropriate vector of |classified_deps|. If the
  // dependency is a source set we should link in, the source set's object
  // files will be appended to |classified_deps.extra_object_files|.
  void ClassifyDependency(const Target* dep,
                          ClassifiedDeps* classified_deps) const;

  void WriteCompilerBuildLine(const std::vector<SourceFile>& sources,
                              const std::vector<OutputFile>& extra_deps,
                              const std::vector<OutputFile>& order_only_deps,
                              const char* tool_name,
                              const std::vector<OutputFile>& outputs,
                              bool can_write_source_info = true);

  void WriteLinkerFlags(std::ostream& out,
                        const Tool* tool,
                        const SourceFile* optional_def_file);
  void WriteCustomLinkerFlags(std::ostream& out, const Tool* tool);
  void WriteLibrarySearchPath(std::ostream& out, const Tool* tool);
  void WriteLibs(std::ostream& out, const Tool* tool);
  void WriteFrameworks(std::ostream& out, const Tool* tool);
  void WriteSwiftModules(std::ostream& out,
                         const Tool* tool,
                         const std::vector<OutputFile>& swiftmodules);
  void WritePool(std::ostream& out);

  void AddSourceSetFiles(const Target* source_set,
                         UniqueVector<OutputFile>* obj_files) const;

  // Cached version of the prefix used for rule types for this toolchain.
  std::string rule_prefix_;

 private:
  NinjaBinaryTargetWriter(const NinjaBinaryTargetWriter&) = delete;
  NinjaBinaryTargetWriter& operator=(const NinjaBinaryTargetWriter&) = delete;
};

#endif  // TOOLS_GN_NINJA_BINARY_TARGET_WRITER_H_
