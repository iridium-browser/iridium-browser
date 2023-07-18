// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

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

bool EndsWithExtension(std::string_view str, std::string_view ext) {
  return str.size() > ext.size() && str[str.size() - ext.size() - 1] == '.' &&
         !::memcmp(str.data() + str.size() - ext.size(), ext.data(),
                   ext.size());
}

SourceFile::Type GetSourceFileType(const std::string& file) {
  size_t size = file.size();
  const char* str = file.data();

  // First, single-char extensions.
  if (size > 2 && str[size - 2] == '.') {
    switch (str[size - 1]) {
      case 'c':
        return SourceFile::SOURCE_C;  // .c
      case 'h':
        return SourceFile::SOURCE_H;  // .h
      case 'm':
        return SourceFile::SOURCE_M;  // .m
      case 'o':
        return SourceFile::SOURCE_O;  // .o
      case 'S':
      case 's':
        return SourceFile::SOURCE_S;  // .S and .s
      default:
        return SourceFile::SOURCE_UNKNOWN;
    }
  }

  // Second, two-char extensions
  if (size > 3 && str[size - 3] == '.') {
#define TAG2(c1, c2) ((unsigned)(c1) | ((unsigned)(c2) << 8))
    switch (TAG2(str[size - 2], str[size - 1])) {
      case TAG2('c', 'c'):
        return SourceFile::SOURCE_CPP;  // .cc
      case TAG2('g', 'o'):
        return SourceFile::SOURCE_GO;  // .go
      case TAG2('h', 'h'):
        return SourceFile::SOURCE_H;   // .hh
      case TAG2('m', 'm'):
        return SourceFile::SOURCE_MM;  // .mm
      case TAG2('r', 'c'):
        return SourceFile::SOURCE_RC;  // .rc
      case TAG2('r', 's'):
        return SourceFile::SOURCE_RS;  // .rs
      default:
        return SourceFile::SOURCE_UNKNOWN;
    }
#undef TAG2
  }

  if (size > 4 && str[size - 4] == '.') {
#define TAG3(c1, c2, c3) \
  ((unsigned)(c1) | ((unsigned)(c2) << 8) | ((unsigned)(c3) << 16))
    switch (TAG3(str[size - 3], str[size - 2], str[size - 1])) {
      case TAG3('c', 'p', 'p'):
      case TAG3('c', 'x', 'x'):
      case TAG3('c', '+', '+'):
        return SourceFile::SOURCE_CPP;
      case TAG3('h', 'p', 'p'):
      case TAG3('h', 'x', 'x'):
      case TAG3('i', 'n', 'c'):
      case TAG3('i', 'p', 'p'):
      case TAG3('i', 'n', 'l'):
        return SourceFile::SOURCE_H;
      case TAG3('a', 's', 'm'):
        return SourceFile::SOURCE_S;
      case TAG3('d', 'e', 'f'):
        return SourceFile::SOURCE_DEF;
      case TAG3('o', 'b', 'j'):
        return SourceFile::SOURCE_O;
      default:
        return SourceFile::SOURCE_UNKNOWN;
    }
#undef TAG3
  }

  // Other cases
  if (EndsWithExtension(file, "swift"))
    return SourceFile::SOURCE_SWIFT;

  if (EndsWithExtension(file, "swiftmodule"))
    return SourceFile::SOURCE_SWIFTMODULE;

  if (EndsWithExtension(file, "modulemap"))
    return SourceFile::SOURCE_MODULEMAP;

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

SourceFile::SourceFile(StringAtom value) : value_(value) {}

SourceFile::Type SourceFile::GetType() const {
  return GetSourceFileType(value_.str());
}

bool SourceFile::IsDefType() const {
  std::string_view v = value_.str();
  return EndsWithExtension(v, "def");
}

bool SourceFile::IsObjectType() const {
  std::string_view v = value_.str();
  return EndsWithExtension(v, "o") || EndsWithExtension(v, "obj");
}

bool SourceFile::IsModuleMapType() const {
  std::string_view v = value_.str();
  return EndsWithExtension(v, "modulemap");
}

bool SourceFile::IsSwiftType() const {
  std::string_view v = value_.str();
  return EndsWithExtension(v, "swift");
}

bool SourceFile::IsSwiftModuleType() const {
  std::string_view v = value_.str();
  return EndsWithExtension(v, "swiftmodule");
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
