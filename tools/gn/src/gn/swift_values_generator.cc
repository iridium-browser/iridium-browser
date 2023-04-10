// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/swift_values_generator.h"

#include "gn/label.h"
#include "gn/scope.h"
#include "gn/settings.h"
#include "gn/swift_values.h"
#include "gn/swift_variables.h"
#include "gn/target.h"
#include "gn/value_extractors.h"

SwiftValuesGenerator::SwiftValuesGenerator(Target* target,
                                           Scope* scope,
                                           Err* err)
    : target_(target), scope_(scope), err_(err) {}

SwiftValuesGenerator::~SwiftValuesGenerator() = default;

void SwiftValuesGenerator::Run() {
  if (!FillBridgeHeader())
    return;

  if (!FillModuleName())
    return;
}

bool SwiftValuesGenerator::FillBridgeHeader() {
  const Value* value = scope_->GetValue(variables::kSwiftBridgeHeader, true);
  if (!value)
    return true;

  if (!value->VerifyTypeIs(Value::STRING, err_))
    return false;

  SourceFile dest;
  if (!ExtractRelativeFile(scope_->settings()->build_settings(), *value,
                           scope_->GetSourceDir(), &dest, err_))
    return false;

  target_->swift_values().bridge_header() = std::move(dest);
  return true;
}

bool SwiftValuesGenerator::FillModuleName() {
  const Value* value = scope_->GetValue(variables::kSwiftModuleName, true);
  if (!value) {
    // The target name will be used.
    target_->swift_values().module_name() = target_->label().name();
    return true;
  }

  if (!value->VerifyTypeIs(Value::STRING, err_))
    return false;

  target_->swift_values().module_name() = std::move(value->string_value());
  return true;
}
