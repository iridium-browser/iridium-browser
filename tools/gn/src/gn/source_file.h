// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_SOURCE_FILE_H_
#define TOOLS_GN_SOURCE_FILE_H_

#include <stddef.h>

#include <algorithm>
#include <string>
#include <string_view>

#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/logging.h"

#include "gn/string_atom.h"

class SourceDir;

// Represents a file within the source tree. Always begins in a slash, never
// ends in one.
class SourceFile {
 public:
  // This should be sequential integers starting from 0 so they can be used as
  // array indices.
  enum Type {
    SOURCE_UNKNOWN = 0,
    SOURCE_ASM,
    SOURCE_C,
    SOURCE_CPP,
    SOURCE_H,
    SOURCE_M,
    SOURCE_MM,
    SOURCE_MODULEMAP,
    SOURCE_S,
    SOURCE_RC,
    SOURCE_O,  // Object files can be inputs, too. Also counts .obj.
    SOURCE_DEF,

    SOURCE_RS,
    SOURCE_GO,
    SOURCE_SWIFT,
    SOURCE_SWIFTMODULE,

    // Must be last.
    SOURCE_NUMTYPES,
  };

  SourceFile() = default;

  // Takes a known absolute source file. Always begins in a slash.
  explicit SourceFile(const std::string& value);
  explicit SourceFile(std::string&& value);
  explicit SourceFile(StringAtom value);

  ~SourceFile() = default;

  bool is_null() const { return value_.empty(); }
  const std::string& value() const { return value_.str(); }
  Type GetType() const;

  // Optimized implementation of GetType() == SOURCE_XXX
  bool IsDefType() const;          // SOURCE_DEF
  bool IsModuleMapType() const;    // SOURCE_MODULEMAP
  bool IsObjectType() const;       // SOURCE_O
  bool IsSwiftType() const;        // SOURCE_SWIFT
  bool IsSwiftModuleType() const;  // SOURCE_SWIFTMODULE

  // Returns everything after the last slash.
  std::string GetName() const;
  SourceDir GetDir() const;

  // Resolves this source file relative to some given source root. Returns
  // an empty file path on error.
  base::FilePath Resolve(const base::FilePath& source_root) const;

  // Returns true if this file starts with a "//" which indicates a path
  // from the source root.
  bool is_source_absolute() const {
    return value().size() >= 2 && value()[0] == '/' && value()[1] == '/';
  }

  // Returns true if this file starts with a single slash which indicates a
  // system-absolute path.
  bool is_system_absolute() const { return !is_source_absolute(); }

  // Returns a source-absolute path starting with only one slash at the
  // beginning (normally source-absolute paths start with two slashes to mark
  // them as such). This is normally used when concatenating names together.
  //
  // This function asserts that the file is actually source-absolute. The
  // return value points into our buffer.
  std::string_view SourceAbsoluteWithOneSlash() const {
    CHECK(is_source_absolute());
    return std::string_view(&value()[1], value().size() - 1);
  }

  bool operator==(const SourceFile& other) const {
    return value_.SameAs(other.value_);
  }
  bool operator!=(const SourceFile& other) const { return !operator==(other); }
  bool operator<(const SourceFile& other) const {
    return value_ < other.value_;
  }

  struct PtrCompare {
    bool operator()(const SourceFile& a, const SourceFile& b) const noexcept {
      return StringAtom::PtrCompare()(a.value_, b.value_);
    }
  };
  struct PtrHash {
    size_t operator()(const SourceFile& s) const noexcept {
      return StringAtom::PtrHash()(s.value_);
    }
  };

  struct PtrEqual {
    bool operator()(const SourceFile& a, const SourceFile& b) const noexcept {
      return StringAtom::PtrEqual()(a.value_, b.value_);
    }
  };

 private:
  friend class SourceDir;

  void SetValue(const std::string& value);

  StringAtom value_;
};

namespace std {

template <>
struct hash<SourceFile> {
  std::size_t operator()(const SourceFile& v) const {
    return SourceFile::PtrHash()(v);
  }
};

}  // namespace std

// Represents a set of source files.
// NOTE: In practice, this is much faster than using an std::set<> or
// std::unordered_set<> container. E.g. for the Fuchsia Zircon build, the
// overall difference in "gn gen" time is about 10%.
using SourceFileSet = base::flat_set<SourceFile, SourceFile::PtrCompare>;

// Represents a set of tool types.
class SourceFileTypeSet {
 public:
  SourceFileTypeSet();

  void Set(SourceFile::Type type) {
    flags_[static_cast<int>(type)] = true;
    empty_ = false;
  }
  bool Get(SourceFile::Type type) const {
    return flags_[static_cast<int>(type)];
  }

  bool empty() const { return empty_; }

  bool CSourceUsed() const;
  bool RustSourceUsed() const;
  bool GoSourceUsed() const;
  bool SwiftSourceUsed() const;

  bool MixedSourceUsed() const;

 private:
  bool empty_;
  bool flags_[static_cast<int>(SourceFile::SOURCE_NUMTYPES)];
};

#endif  // TOOLS_GN_SOURCE_FILE_H_
