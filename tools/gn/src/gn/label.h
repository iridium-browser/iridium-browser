// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_LABEL_H_
#define TOOLS_GN_LABEL_H_

#include <string_view>
#include <tuple>

#include <stddef.h>

#include "gn/source_dir.h"
#include "gn/string_atom.h"

class Err;
class Value;

// A label represents the name of a target or some other named thing in
// the source path. The label is always absolute and always includes a name
// part, so it starts with a slash, and has one colon.
class Label {
 public:
  Label();

  // Makes a label given an already-separated out path and name.
  // See also Resolve().
  Label(const SourceDir& dir,
        std::string_view name,
        const SourceDir& toolchain_dir,
        std::string_view toolchain_name);

  // Makes a label with an empty toolchain.
  Label(const SourceDir& dir, std::string_view name);

  // Resolves a string from a build file that may be relative to the
  // current directory into a fully qualified label. On failure returns an
  // is_null() label and sets the error.
  static Label Resolve(const SourceDir& current_dir,
                       std::string_view source_root,
                       const Label& current_toolchain,
                       const Value& input,
                       Err* err);

  bool is_null() const { return dir_.is_null(); }

  const SourceDir& dir() const { return dir_; }
  const std::string& name() const { return name_.str(); }
  StringAtom name_atom() const { return name_; }

  const SourceDir& toolchain_dir() const { return toolchain_dir_; }
  const std::string& toolchain_name() const { return toolchain_name_.str(); }
  StringAtom toolchain_name_atom() const { return toolchain_name_; }

  // Returns the current label's toolchain as its own Label.
  Label GetToolchainLabel() const;

  // Returns a copy of this label but with an empty toolchain.
  Label GetWithNoToolchain() const;

  // Formats this label in a way that we can present to the user or expose to
  // other parts of the system. SourceDirs end in slashes, but the user
  // expects names like "//chrome/renderer:renderer_config" when printed. The
  // toolchain is optionally included.
  std::string GetUserVisibleName(bool include_toolchain) const;

  // Like the above version, but automatically includes the toolchain if it's
  // not the default one. Normally the user only cares about the toolchain for
  // non-default ones, so this can make certain output more clear.
  std::string GetUserVisibleName(const Label& default_toolchain) const;

  bool operator==(const Label& other) const {
    return hash_ == other.hash_ && name_.SameAs(other.name_) &&
           dir_ == other.dir_ && toolchain_dir_ == other.toolchain_dir_ &&
           toolchain_name_.SameAs(other.toolchain_name_);
  }
  bool operator!=(const Label& other) const { return !operator==(other); }
  bool operator<(const Label& other) const {
    // This custom comparison function uses the fact that SourceDir and
    // StringAtom values have very fast equality comparison to avoid
    // un-necessary string comparisons when components are equal.
    if (dir_ != other.dir_)
      return dir_ < other.dir_;

    if (!name_.SameAs(other.name_))
      return name_ < other.name_;

    if (toolchain_dir_ != other.toolchain_dir_)
      return toolchain_dir_ < other.toolchain_dir_;

    return toolchain_name_ < other.toolchain_name_;
  }

  // Returns true if the toolchain dir/name of this object matches some
  // other object.
  bool ToolchainsEqual(const Label& other) const {
    return toolchain_dir_ == other.toolchain_dir_ &&
           toolchain_name_.SameAs(other.toolchain_name_);
  }

  size_t hash() const { return hash_; }

 private:
  Label(SourceDir dir, StringAtom name)
      : dir_(dir), name_(name), hash_(ComputeHash()) {}

  Label(SourceDir dir,
        StringAtom name,
        SourceDir toolchain_dir,
        StringAtom toolchain_name)
      : dir_(dir),
        name_(name),
        toolchain_dir_(toolchain_dir),
        toolchain_name_(toolchain_name),
        hash_(ComputeHash()) {}

  size_t ComputeHash() const {
    size_t h0 = dir_.hash();
    size_t h1 = name_.ptr_hash();
    size_t h2 = toolchain_dir_.hash();
    size_t h3 = toolchain_name_.ptr_hash();
    return ((h3 * 131 + h2) * 131 + h1) * 131 + h0;
  }

  SourceDir dir_;
  StringAtom name_;

  SourceDir toolchain_dir_;
  StringAtom toolchain_name_;

  size_t hash_;
  // NOTE: Must be initialized by constructors with ComputeHash() value.
};

namespace std {

template <>
struct hash<Label> {
  std::size_t operator()(const Label& v) const { return v.hash(); }
};

}  // namespace std

extern const char kLabels_Help[];

#endif  // TOOLS_GN_LABEL_H_
