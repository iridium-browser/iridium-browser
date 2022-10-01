// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_XCODE_WRITER_H_
#define TOOLS_GN_XCODE_WRITER_H_

#include <iosfwd>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"

class Builder;
class BuildSettings;
class Err;

enum class XcodeBuildSystem {
  kLegacy,
  kNew,
};

// Writes an Xcode workspace to build and debug code.
class XcodeWriter {
 public:
  // Controls some parameters and behaviour of the RunAndWriteFiles().
  struct Options {
    // Name of the generated project file. Defaults to "all" is empty.
    std::string project_name;

    // Name of the ninja target to use for the "All" target in the generated
    // project. If empty, no target will be passed to ninja which will thus
    // try to build all defined targets.
    std::string root_target_name;

    // Name of the ninja executable. Defaults to "ninja" if empty.
    std::string ninja_executable;

    // If specified, should be a semicolon-separated list of label patterns.
    // It will be used to filter the list of targets generated in the project
    // (in the same way that the other filtering is done, source and header
    // files for those target will still be listed in the generated project).
    std::string dir_filters_string;

    // If specified, should be a semicolon-separated list of configuration
    // names. It will be used to generate all the configuration variations
    // in the project. If empty, the project is assumed to only use a single
    // configuration "Release".
    std::string configurations;

    // If specified, should be the path for the configuration's build
    // directory. It can use Xcode variables such as ${CONFIGURATION} or
    // ${EFFECTIVE_PLATFORM_NAME}. If empty, it is assumed to be the same
    // as the project directory.
    base::FilePath configuration_build_dir;

    // If specified, should be a semicolon-separated list of file patterns.
    // It will be used to add files to the project that are not referenced
    // from the BUILD.gn files. This is usually used to add documentation
    // files.
    base::FilePath::StringType additional_files_patterns;

    // If specified, should be a semicolon-separated list of file roots.
    // It will be used to add files to the project that are not referenced
    // from the BUILD.gn files. This is usually used to add documentation
    // files.
    base::FilePath::StringType additional_files_roots;

    // Control which version of the build system should be used for the
    // generated Xcode project.
    XcodeBuildSystem build_system = XcodeBuildSystem::kLegacy;
  };

  // Writes an Xcode workspace with a single project file.
  //
  // The project will lists all files referenced for the build (including the
  // sources, headers and some supporting files). The project can be used to
  // build, develop and debug from Xcode (though adding files, changing build
  // settings, etc. still needs to be done via BUILD.gn files).
  //
  // The list of targets is filtered to only include relevant targets for
  // debugging (mostly binaries and bundles) so it is not possible to build
  // individuals targets (i.e. source_set) via Xcode. This filtering is done
  // to improve the performances when loading the solution in Xcode (project
  // like Chromium cannot be opened if all targets are generated).
  //
  // The source and header files are still listed in the generated generated
  // Xcode project, even if the target they are defined in are filtered (not
  // doing so would make it less pleasant to use Xcode to debug without any
  // significant performance improvement).
  //
  // Extra behaviour is controlled by the |options| parameter. See comments
  // of the Options type for more informations.
  //
  // Returns true on success, fails on failure. |err| is set in that case.
  static bool RunAndWriteFiles(const BuildSettings* build_settings,
                               const Builder& builder,
                               Options options,
                               Err* err);

 private:
  XcodeWriter(const XcodeWriter&) = delete;
  XcodeWriter& operator=(const XcodeWriter&) = delete;
};

#endif  // TOOLS_GN_XCODE_WRITER_H_
