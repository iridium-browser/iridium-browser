// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/value_extractors.h"

#include <stddef.h>

#include "gn/build_settings.h"
#include "gn/err.h"
#include "gn/frameworks_utils.h"
#include "gn/label.h"
#include "gn/source_dir.h"
#include "gn/source_file.h"
#include "gn/target.h"
#include "gn/value.h"

namespace {

// Sets the error and returns false on failure.
template <typename T, class Converter>
bool ListValueExtractor(const Value& value,
                        std::vector<T>* dest,
                        Err* err,
                        const Converter& converter) {
  if (!value.VerifyTypeIs(Value::LIST, err))
    return false;
  const std::vector<Value>& input_list = value.list_value();
  dest->resize(input_list.size());
  for (size_t i = 0; i < input_list.size(); i++) {
    if (!converter(input_list[i], &(*dest)[i], err))
      return false;
  }
  return true;
}

// Like the above version but extracts to a UniqueVector and sets the error if
// there are duplicates.
template <typename T, class Converter>
bool ListValueUniqueExtractor(const Value& value,
                              UniqueVector<T>* dest,
                              Err* err,
                              const Converter& converter) {
  if (!value.VerifyTypeIs(Value::LIST, err))
    return false;
  const std::vector<Value>& input_list = value.list_value();

  for (const auto& item : input_list) {
    T new_one;
    if (!converter(item, &new_one, err))
      return false;
    if (!dest->push_back(new_one)) {
      // Already in the list, throw error.
      *err = Err(item, "Duplicate item in list");
      size_t previous_index = dest->IndexOf(new_one);
      err->AppendSubErr(
          Err(input_list[previous_index], "This was the previous definition."));
      return false;
    }
  }
  return true;
}

struct RelativeFileConverter {
  RelativeFileConverter(const BuildSettings* build_settings_in,
                        const SourceDir& current_dir_in)
      : build_settings(build_settings_in), current_dir(current_dir_in) {}
  bool operator()(const Value& v, SourceFile* out, Err* err) const {
    *out = current_dir.ResolveRelativeFile(v, err,
                                           build_settings->root_path_utf8());
    return !err->has_error();
  }
  const BuildSettings* build_settings;
  const SourceDir& current_dir;
};

struct LibFileConverter {
  LibFileConverter(const BuildSettings* build_settings_in,
                   const SourceDir& current_dir_in)
      : build_settings(build_settings_in), current_dir(current_dir_in) {}
  bool operator()(const Value& v, LibFile* out, Err* err) const {
    if (!v.VerifyTypeIs(Value::STRING, err))
      return false;
    if (!GetFrameworkName(v.string_value()).empty()) {
      *err = Err(v, "Unsupported value in libs.",
                 "Use frameworks to list framework dependencies.");
      return false;
    }
    if (v.string_value().find('/') == std::string::npos) {
      *out = LibFile(v.string_value());
    } else {
      *out = LibFile(current_dir.ResolveRelativeFile(
          v, err, build_settings->root_path_utf8()));
    }
    return !err->has_error();
  }
  const BuildSettings* build_settings;
  const SourceDir& current_dir;
};

struct RelativeDirConverter {
  RelativeDirConverter(const BuildSettings* build_settings_in,
                       const SourceDir& current_dir_in)
      : build_settings(build_settings_in), current_dir(current_dir_in) {}
  bool operator()(const Value& v, SourceDir* out, Err* err) const {
    *out = current_dir.ResolveRelativeDir(v, err,
                                          build_settings->root_path_utf8());
    return true;
  }
  const BuildSettings* build_settings;
  const SourceDir& current_dir;
};

struct ExternConverter {
  ExternConverter(const BuildSettings* build_settings_in,
                  const SourceDir& current_dir_in)
      : build_settings(build_settings_in), current_dir(current_dir_in) {}
  bool operator()(const Value& v,
                  std::pair<std::string, LibFile>* out,
                  Err* err) const {
    if (!v.VerifyTypeIs(Value::SCOPE, err))
      return false;
    Scope::KeyValueMap scope;
    v.scope_value()->GetCurrentScopeValues(&scope);
    std::string cratename;
    if (auto it = scope.find("crate_name"); it != scope.end()) {
      if (!it->second.VerifyTypeIs(Value::STRING, err))
        return false;
      cratename = it->second.string_value();
    } else {
      return false;
    }
    LibFile path;
    if (auto it = scope.find("path"); it != scope.end()) {
      if (!it->second.VerifyTypeIs(Value::STRING, err))
        return false;
      if (it->second.string_value().find('/') == std::string::npos) {
        path = LibFile(it->second.string_value());
      } else {
        path = LibFile(current_dir.ResolveRelativeFile(
            it->second, err, build_settings->root_path_utf8()));
      }
    } else {
      return false;
    }
    *out = std::pair(cratename, path);
    return !err->has_error();
  }
  const BuildSettings* build_settings;
  const SourceDir& current_dir;
};

// Fills in a label.
template <typename T>
struct LabelResolver {
  LabelResolver(const BuildSettings* build_settings_in,
                const SourceDir& current_dir_in,
                const Label& current_toolchain_in)
      : build_settings(build_settings_in),
        current_dir(current_dir_in),
        current_toolchain(current_toolchain_in) {}
  bool operator()(const Value& v, Label* out, Err* err) const {
    if (!v.VerifyTypeIs(Value::STRING, err))
      return false;
    *out = Label::Resolve(current_dir, build_settings->root_path_utf8(),
                          current_toolchain, v, err);
    return !err->has_error();
  }
  const BuildSettings* build_settings;
  const SourceDir& current_dir;
  const Label& current_toolchain;
};

// Fills the label part of a LabelPtrPair, leaving the pointer null.
template <typename T>
struct LabelPtrResolver {
  LabelPtrResolver(const BuildSettings* build_settings_in,
                   const SourceDir& current_dir_in,
                   const Label& current_toolchain_in)
      : build_settings(build_settings_in),
        current_dir(current_dir_in),
        current_toolchain(current_toolchain_in) {}
  bool operator()(const Value& v, LabelPtrPair<T>* out, Err* err) const {
    if (!v.VerifyTypeIs(Value::STRING, err))
      return false;
    out->label = Label::Resolve(current_dir, build_settings->root_path_utf8(),
                                current_toolchain, v, err);
    out->origin = v.origin();
    return !err->has_error();
  }
  const BuildSettings* build_settings;
  const SourceDir& current_dir;
  const Label& current_toolchain;
};

struct LabelPatternResolver {
  LabelPatternResolver(const BuildSettings* build_settings_in,
                       const SourceDir& current_dir_in)
      : build_settings(build_settings_in), current_dir(current_dir_in) {}
  bool operator()(const Value& v, LabelPattern* out, Err* err) const {
    *out = LabelPattern::GetPattern(current_dir,
                                    build_settings->root_path_utf8(), v, err);
    return !err->has_error();
  }

  const BuildSettings* build_settings;
  const SourceDir& current_dir;
};

}  // namespace

bool ExtractListOfStringValues(const Value& value,
                               std::vector<std::string>* dest,
                               Err* err) {
  if (!value.VerifyTypeIs(Value::LIST, err))
    return false;
  const std::vector<Value>& input_list = value.list_value();
  dest->reserve(input_list.size());
  for (const auto& item : input_list) {
    if (!item.VerifyTypeIs(Value::STRING, err))
      return false;
    dest->push_back(item.string_value());
  }
  return true;
}

bool ExtractListOfRelativeFiles(const BuildSettings* build_settings,
                                const Value& value,
                                const SourceDir& current_dir,
                                std::vector<SourceFile>* files,
                                Err* err) {
  return ListValueExtractor(value, files, err,
                            RelativeFileConverter(build_settings, current_dir));
}

bool ExtractListOfLibs(const BuildSettings* build_settings,
                       const Value& value,
                       const SourceDir& current_dir,
                       std::vector<LibFile>* libs,
                       Err* err) {
  return ListValueExtractor(value, libs, err,
                            LibFileConverter(build_settings, current_dir));
}

bool ExtractListOfRelativeDirs(const BuildSettings* build_settings,
                               const Value& value,
                               const SourceDir& current_dir,
                               std::vector<SourceDir>* dest,
                               Err* err) {
  return ListValueExtractor(value, dest, err,
                            RelativeDirConverter(build_settings, current_dir));
}

bool ExtractListOfLabels(const BuildSettings* build_settings,
                         const Value& value,
                         const SourceDir& current_dir,
                         const Label& current_toolchain,
                         LabelTargetVector* dest,
                         Err* err) {
  return ListValueExtractor(
      value, dest, err,
      LabelPtrResolver<Target>(build_settings, current_dir, current_toolchain));
}

bool ExtractListOfUniqueLabels(const BuildSettings* build_settings,
                               const Value& value,
                               const SourceDir& current_dir,
                               const Label& current_toolchain,
                               UniqueVector<Label>* dest,
                               Err* err) {
  return ListValueUniqueExtractor(
      value, dest, err,
      LabelResolver<Config>(build_settings, current_dir, current_toolchain));
}

bool ExtractListOfUniqueLabels(const BuildSettings* build_settings,
                               const Value& value,
                               const SourceDir& current_dir,
                               const Label& current_toolchain,
                               UniqueVector<LabelConfigPair>* dest,
                               Err* err) {
  return ListValueUniqueExtractor(
      value, dest, err,
      LabelPtrResolver<Config>(build_settings, current_dir, current_toolchain));
}

bool ExtractListOfUniqueLabels(const BuildSettings* build_settings,
                               const Value& value,
                               const SourceDir& current_dir,
                               const Label& current_toolchain,
                               UniqueVector<LabelTargetPair>* dest,
                               Err* err) {
  return ListValueUniqueExtractor(
      value, dest, err,
      LabelPtrResolver<Target>(build_settings, current_dir, current_toolchain));
}

bool ExtractRelativeFile(const BuildSettings* build_settings,
                         const Value& value,
                         const SourceDir& current_dir,
                         SourceFile* file,
                         Err* err) {
  RelativeFileConverter converter(build_settings, current_dir);
  return converter(value, file, err);
}

bool ExtractListOfLabelPatterns(const BuildSettings* build_settings,
                                const Value& value,
                                const SourceDir& current_dir,
                                std::vector<LabelPattern>* patterns,
                                Err* err) {
  return ListValueExtractor(value, patterns, err,
                            LabelPatternResolver(build_settings, current_dir));
}

bool ExtractListOfExterns(const BuildSettings* build_settings,
                          const Value& value,
                          const SourceDir& current_dir,
                          std::vector<std::pair<std::string, LibFile>>* externs,
                          Err* err) {
  return ListValueExtractor(value, externs, err,
                            ExternConverter(build_settings, current_dir));
}
