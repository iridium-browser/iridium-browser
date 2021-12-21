// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/source_file.h"

#include "base/logging.h"
#include "gn/filesystem_utils.h"
#include "gn/source_dir.h"
#include "util/build_config.h"

namespace {

void AssertValueSourceFileString(const std::string& s) {
#if defined(OS_WIN)
  DCHECK(s[0] == '/' ||
         (s.size() > 2 && s[0] != '/' && s[1] == ':' && IsSlash(s[2])));
#else
  DCHECK(s[0] == '/');
#endif
  DCHECK(!EndsWithSlash(s)) << s;
}

SourceFile::Type GetSourceFileType(const std::string& file) {
  std::string_view extension = FindExtension(&file);
  if (extension == "cc" || extension == "cpp" || extension == "cxx" ||
      extension == "c++")
    return SourceFile::SOURCE_CPP;
  if (extension == "h" || extension == "hpp" || extension == "hxx" ||
      extension == "hh" || extension == "inc" || extension == "ipp" ||
      extension == "inl")
    return SourceFile::SOURCE_H;
  if (extension == "c")
    return SourceFile::SOURCE_C;
  if (extension == "m")
    return SourceFile::SOURCE_M;
  if (extension == "mm")
    return SourceFile::SOURCE_MM;
  if (extension == "modulemap")
    return SourceFile::SOURCE_MODULEMAP;
  if (extension == "rc")
    return SourceFile::SOURCE_RC;
  if (extension == "S" || extension == "s" || extension == "asm")
    return SourceFile::SOURCE_S;
  if (extension == "o" || extension == "obj")
    return SourceFile::SOURCE_O;
  if (extension == "def")
    return SourceFile::SOURCE_DEF;
  if (extension == "rs")
    return SourceFile::SOURCE_RS;
  if (extension == "go")
    return SourceFile::SOURCE_GO;
  if (extension == "swift")
    return SourceFile::SOURCE_SWIFT;
  if (extension == "swiftmodule")
    return SourceFile::SOURCE_SWIFTMODULE;

  return SourceFile::SOURCE_UNKNOWN;
}

std::string Normalized(std::string value) {
  DCHECK(!value.empty());
  AssertValueSourceFileString(value);
  NormalizePath(&value);
  return value;
}

}  // namespace

SourceFile::SourceFile(const std::string& value)
    : SourceFile(StringAtom(Normalized(value))) {}

SourceFile::SourceFile(std::string&& value)
    : SourceFile(StringAtom(Normalized(std::move(value)))) {}

SourceFile::SourceFile(StringAtom value) : value_(value) {
  type_ = GetSourceFileType(value_.str());
}

std::string SourceFile::GetName() const {
  if (is_null())
    return std::string();

  const std::string& value = value_.str();
  DCHECK(value.find('/') != std::string::npos);
  size_t last_slash = value.rfind('/');
  return std::string(&value[last_slash + 1], value.size() - last_slash - 1);
}

SourceDir SourceFile::GetDir() const {
  if (is_null())
    return SourceDir();

  const std::string& value = value_.str();
  DCHECK(value.find('/') != std::string::npos);
  size_t last_slash = value.rfind('/');
  return SourceDir(value.substr(0, last_slash + 1));
}

base::FilePath SourceFile::Resolve(const base::FilePath& source_root) const {
  return ResolvePath(value_.str(), true, source_root);
}

void SourceFile::SetValue(const std::string& value) {
  value_ = StringAtom(value);
  type_ = GetSourceFileType(value);
}

SourceFileTypeSet::SourceFileTypeSet() : empty_(true) {
  memset(flags_, 0,
         sizeof(bool) * static_cast<int>(SourceFile::SOURCE_NUMTYPES));
}

bool SourceFileTypeSet::CSourceUsed() const {
  return empty_ || Get(SourceFile::SOURCE_CPP) ||
         Get(SourceFile::SOURCE_MODULEMAP) || Get(SourceFile::SOURCE_H) ||
         Get(SourceFile::SOURCE_C) || Get(SourceFile::SOURCE_M) ||
         Get(SourceFile::SOURCE_MM) || Get(SourceFile::SOURCE_RC) ||
         Get(SourceFile::SOURCE_S) || Get(SourceFile::SOURCE_O) ||
         Get(SourceFile::SOURCE_DEF);
}

bool SourceFileTypeSet::RustSourceUsed() const {
  return Get(SourceFile::SOURCE_RS);
}

bool SourceFileTypeSet::GoSourceUsed() const {
  return Get(SourceFile::SOURCE_GO);
}

bool SourceFileTypeSet::SwiftSourceUsed() const {
  return Get(SourceFile::SOURCE_SWIFT);
}

bool SourceFileTypeSet::MixedSourceUsed() const {
  return (1 << static_cast<int>(CSourceUsed())
            << static_cast<int>(RustSourceUsed())
            << static_cast<int>(GoSourceUsed())
            << static_cast<int>(SwiftSourceUsed())) > 2;
}
