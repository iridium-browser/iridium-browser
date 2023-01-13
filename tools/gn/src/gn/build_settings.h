// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_BUILD_SETTINGS_H_
#define TOOLS_GN_BUILD_SETTINGS_H_

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <utility>

#include "base/files/file_path.h"
#include "gn/args.h"
#include "gn/label.h"
#include "gn/scope.h"
#include "gn/source_dir.h"
#include "gn/source_file.h"
#include "gn/version.h"

class Item;

// Settings for one build, which is one toplevel output directory. There
// may be multiple Settings objects that refer to this, one for each toolchain.
class BuildSettings {
 public:
  using ItemDefinedCallback = std::function<void(std::unique_ptr<Item>)>;
  using PrintCallback = std::function<void(const std::string&)>;

  BuildSettings();
  BuildSettings(const BuildSettings& other);

  // Root target label.
  const Label& root_target_label() const { return root_target_label_; }
  void SetRootTargetLabel(const Label& r);

  // Absolute path of the source root on the local system. Everything is
  // relative to this. Does not end in a [back]slash.
  const base::FilePath& root_path() const { return root_path_; }
  const base::FilePath& dotfile_name() const { return dotfile_name_; }
  const std::string& root_path_utf8() const { return root_path_utf8_; }
  void SetRootPath(const base::FilePath& r);
  void set_dotfile_name(const base::FilePath& d) { dotfile_name_ = d; }

  // When nonempty, specifies a parallel directory higherarchy in which to
  // search for buildfiles if they're not found in the root higherarchy. This
  // allows us to keep buildfiles in a separate tree during development.
  const base::FilePath& secondary_source_path() const {
    return secondary_source_path_;
  }
  void SetSecondarySourcePath(const SourceDir& d);

  // Path of the python executable to run scripts with.
  base::FilePath python_path() const { return python_path_; }
  void set_python_path(const base::FilePath& p) { python_path_ = p; }

  // Required Ninja version.
  const Version& ninja_required_version() const {
    return ninja_required_version_;
  }
  void set_ninja_required_version(Version v) { ninja_required_version_ = v; }

  // The 'no_stamp_files' boolean flag can be set to generate Ninja files
  // that use phony rules instead of stamp files in most cases. This reduces
  // the size of the generated Ninja build plans, but requires Ninja 1.11
  // or greater to properly process them.
  bool no_stamp_files() const { return no_stamp_files_; }
  void set_no_stamp_files(bool no_stamp_files) {
    no_stamp_files_ = no_stamp_files;
  }

  const SourceFile& build_config_file() const { return build_config_file_; }
  void set_build_config_file(const SourceFile& f) { build_config_file_ = f; }

  // Path to a file containing the default text to use when running `gn args`.
  const SourceFile& arg_file_template_path() const {
    return arg_file_template_path_;
  }
  void set_arg_file_template_path(const SourceFile& f) {
    arg_file_template_path_ = f;
  }

  // The build directory is the root of all output files. The default toolchain
  // files go into here, and non-default toolchains will have separate
  // toolchain-specific root directories inside this.
  const SourceDir& build_dir() const { return build_dir_; }
  void SetBuildDir(const SourceDir& dir);

  // The build args are normally specified on the command-line.
  Args& build_args() { return build_args_; }
  const Args& build_args() const { return build_args_; }

  // Returns the full absolute OS path cooresponding to the given file in the
  // root source tree.
  base::FilePath GetFullPath(const SourceFile& file) const;
  base::FilePath GetFullPath(const SourceDir& dir) const;
  // Works the same way as other GetFullPath.
  // Parameter as_file defines whether path should be treated as a
  // SourceFile or SourceDir value.
  base::FilePath GetFullPath(const std::string& path, bool as_file) const;

  // Returns the absolute OS path inside the secondary source path. Will return
  // an empty FilePath if the secondary source path is empty. When loading a
  // buildfile, the GetFullPath should always be consulted first.
  base::FilePath GetFullPathSecondary(const SourceFile& file) const;
  base::FilePath GetFullPathSecondary(const SourceDir& dir) const;
  // Works the same way as other GetFullPathSecondary.
  // Parameter as_file defines whether path should be treated as a
  // SourceFile or SourceDir value.
  base::FilePath GetFullPathSecondary(const std::string& path,
                                      bool as_file) const;

  // Called when an item is defined from a background thread.
  void ItemDefined(std::unique_ptr<Item> item) const;
  void set_item_defined_callback(ItemDefinedCallback cb) {
    item_defined_callback_ = cb;
  }

  // Defines a callback that will be used to override the behavior of the
  // print function. This is used in tests to collect print output. If the
  // callback is is_null() (the default) the output will be printed to the
  // console.
  const PrintCallback& print_callback() const { return print_callback_; }
  void set_print_callback(const PrintCallback& cb) { print_callback_ = cb; }

  // A list of files that can call exec_script(). If the returned pointer is
  // null, exec_script may be called from anywhere.
  const SourceFileSet* exec_script_whitelist() const {
    return exec_script_whitelist_.get();
  }
  void set_exec_script_whitelist(std::unique_ptr<SourceFileSet> list) {
    exec_script_whitelist_ = std::move(list);
  }

 private:
  Label root_target_label_;
  base::FilePath dotfile_name_;
  base::FilePath root_path_;
  std::string root_path_utf8_;
  base::FilePath secondary_source_path_;
  base::FilePath python_path_;

  // See 40045b9 for the reason behind using 1.7.2 as the default version.
  Version ninja_required_version_{1, 7, 2};
  bool no_stamp_files_ = false;

  SourceFile build_config_file_;
  SourceFile arg_file_template_path_;
  SourceDir build_dir_;
  Args build_args_;

  ItemDefinedCallback item_defined_callback_;
  PrintCallback print_callback_;

  std::unique_ptr<SourceFileSet> exec_script_whitelist_;

  BuildSettings& operator=(const BuildSettings&) = delete;
};

#endif  // TOOLS_GN_BUILD_SETTINGS_H_
