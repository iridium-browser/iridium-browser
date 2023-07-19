// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/config_values_generator.h"

#include "base/strings/string_util.h"
#include "gn/build_settings.h"
#include "gn/config_values.h"
#include "gn/frameworks_utils.h"
#include "gn/scope.h"
#include "gn/settings.h"
#include "gn/value.h"
#include "gn/value_extractors.h"
#include "gn/variables.h"

namespace {

void GetStringList(Scope* scope,
                   const char* var_name,
                   ConfigValues* config_values,
                   std::vector<std::string>& (ConfigValues::*accessor)(),
                   Err* err) {
  const Value* value = scope->GetValue(var_name, true);
  if (!value)
    return;  // No value, empty input and succeed.

  ExtractListOfStringValues(*value, &(config_values->*accessor)(), err);
}

void GetDirList(Scope* scope,
                const char* var_name,
                ConfigValues* config_values,
                const SourceDir input_dir,
                std::vector<SourceDir>& (ConfigValues::*accessor)(),
                Err* err) {
  const Value* value = scope->GetValue(var_name, true);
  if (!value)
    return;  // No value, empty input and succeed.

  std::vector<SourceDir> result;
  ExtractListOfRelativeDirs(scope->settings()->build_settings(), *value,
                            input_dir, &result, err);
  (config_values->*accessor)().swap(result);
}

void GetFrameworksList(Scope* scope,
                       const char* var_name,
                       ConfigValues* config_values,
                       std::vector<std::string>& (ConfigValues::*accessor)(),
                       Err* err) {
  const Value* value = scope->GetValue(var_name, true);
  if (!value)
    return;

  std::vector<std::string> frameworks;
  if (!ExtractListOfStringValues(*value, &frameworks, err))
    return;

  // All strings must end with ".frameworks".
  for (const std::string& framework : frameworks) {
    std::string_view framework_name = GetFrameworkName(framework);
    if (framework_name.empty()) {
      *err = Err(*value,
                 "This frameworks value is wrong. "
                 "All listed frameworks names must not include any\n"
                 "path component and have \".framework\" extension.");
      return;
    }
  }

  (config_values->*accessor)().swap(frameworks);
}

}  // namespace

ConfigValuesGenerator::ConfigValuesGenerator(ConfigValues* dest_values,
                                             Scope* scope,
                                             const SourceDir& input_dir,
                                             Err* err)
    : config_values_(dest_values),
      scope_(scope),
      input_dir_(input_dir),
      err_(err) {}

ConfigValuesGenerator::~ConfigValuesGenerator() = default;

void ConfigValuesGenerator::Run() {
#define FILL_STRING_CONFIG_VALUE(name) \
  GetStringList(scope_, #name, config_values_, &ConfigValues::name, err_);
#define FILL_DIR_CONFIG_VALUE(name)                                          \
  GetDirList(scope_, #name, config_values_, input_dir_, &ConfigValues::name, \
             err_);

  FILL_STRING_CONFIG_VALUE(arflags)
  FILL_STRING_CONFIG_VALUE(asmflags)
  FILL_STRING_CONFIG_VALUE(cflags)
  FILL_STRING_CONFIG_VALUE(cflags_c)
  FILL_STRING_CONFIG_VALUE(cflags_cc)
  FILL_STRING_CONFIG_VALUE(cflags_objc)
  FILL_STRING_CONFIG_VALUE(cflags_objcc)
  FILL_STRING_CONFIG_VALUE(defines)
  FILL_DIR_CONFIG_VALUE(framework_dirs)
  FILL_DIR_CONFIG_VALUE(include_dirs)
  FILL_STRING_CONFIG_VALUE(ldflags)
  FILL_DIR_CONFIG_VALUE(lib_dirs)
  FILL_STRING_CONFIG_VALUE(rustflags)
  FILL_STRING_CONFIG_VALUE(rustenv)
  FILL_STRING_CONFIG_VALUE(swiftflags)

#undef FILL_STRING_CONFIG_VALUE
#undef FILL_DIR_CONFIG_VALUE

  // Inputs
  const Value* inputs_value = scope_->GetValue(variables::kInputs, true);
  if (inputs_value) {
    ExtractListOfRelativeFiles(scope_->settings()->build_settings(),
                               *inputs_value, input_dir_,
                               &config_values_->inputs(), err_);
  }

  // Libs
  const Value* libs_value = scope_->GetValue(variables::kLibs, true);
  if (libs_value) {
    ExtractListOfLibs(scope_->settings()->build_settings(), *libs_value,
                      input_dir_, &config_values_->libs(), err_);
  }

  // Externs
  const Value* externs_value = scope_->GetValue(variables::kExterns, true);
  if (externs_value) {
    ExtractListOfExterns(scope_->settings()->build_settings(), *externs_value,
                         input_dir_, &config_values_->externs(), err_);
  }

  // Frameworks
  GetFrameworksList(scope_, variables::kFrameworks, config_values_,
                    &ConfigValues::frameworks, err_);
  GetFrameworksList(scope_, variables::kWeakFrameworks, config_values_,
                    &ConfigValues::weak_frameworks, err_);

  // Precompiled headers.
  const Value* precompiled_header_value =
      scope_->GetValue(variables::kPrecompiledHeader, true);
  if (precompiled_header_value) {
    if (!precompiled_header_value->VerifyTypeIs(Value::STRING, err_))
      return;

    // Check for common errors. This is a string and not a file.
    const std::string& pch_string = precompiled_header_value->string_value();
    if (base::StartsWith(pch_string, "//", base::CompareCase::SENSITIVE)) {
      *err_ = Err(
          *precompiled_header_value, "This precompiled_header value is wrong. ",
          "You need to specify a string that the compiler will match against\n"
          "the #include lines rather than a GN-style file name.\n");
      return;
    }
    config_values_->set_precompiled_header(pch_string);
  }

  const Value* precompiled_source_value =
      scope_->GetValue(variables::kPrecompiledSource, true);
  if (precompiled_source_value) {
    config_values_->set_precompiled_source(input_dir_.ResolveRelativeFile(
        *precompiled_source_value, err_,
        scope_->settings()->build_settings()->root_path_utf8()));
    if (err_->has_error())
      return;
  }
}
