// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/visibility.h"

#include <memory>
#include <string_view>

#include "base/strings/string_util.h"
#include "base/values.h"
#include "gn/err.h"
#include "gn/filesystem_utils.h"
#include "gn/item.h"
#include "gn/label.h"
#include "gn/scope.h"
#include "gn/value.h"
#include "gn/variables.h"

Visibility::Visibility() = default;

Visibility::~Visibility() = default;

bool Visibility::Set(const SourceDir& current_dir,
                     std::string_view source_root,
                     const Value& value,
                     Err* err) {
  patterns_.clear();

  if (!value.VerifyTypeIs(Value::LIST, err)) {
    CHECK(err->has_error());
    return false;
  }

  for (const auto& item : value.list_value()) {
    patterns_.push_back(
        LabelPattern::GetPattern(current_dir, source_root, item, err));
    if (err->has_error())
      return false;
  }
  return true;
}

void Visibility::SetPublic() {
  patterns_.clear();
  patterns_.push_back(LabelPattern(LabelPattern::RECURSIVE_DIRECTORY,
                                   SourceDir(), std::string(), Label()));
}

void Visibility::SetPrivate(const SourceDir& current_dir) {
  patterns_.clear();
  patterns_.push_back(LabelPattern(LabelPattern::DIRECTORY, current_dir,
                                   std::string(), Label()));
}

bool Visibility::CanSeeMe(const Label& label) const {
  return LabelPattern::VectorMatches(patterns_, label);
}

std::string Visibility::Describe(int indent, bool include_brackets) const {
  std::string outer_indent_string(indent, ' ');

  if (patterns_.empty())
    return outer_indent_string + "[] (no visibility)\n";

  std::string result;

  std::string inner_indent_string = outer_indent_string;
  if (include_brackets) {
    result += outer_indent_string + "[\n";
    // Indent the insides more if brackets are requested.
    inner_indent_string += "  ";
  }

  for (const auto& pattern : patterns_)
    result += inner_indent_string + pattern.Describe() + "\n";

  if (include_brackets)
    result += outer_indent_string + "]\n";
  return result;
}

std::unique_ptr<base::Value> Visibility::AsValue() const {
  auto res = std::make_unique<base::ListValue>();
  for (const auto& pattern : patterns_)
    res->AppendString(pattern.Describe());
  return res;
}

// static
bool Visibility::CheckItemVisibility(const Item* from,
                                     const Item* to,
                                     Err* err) {
  if (!to->visibility().CanSeeMe(from->label())) {
    bool with_toolchain = from->settings()->ShouldShowToolchain({
        &to->label(),
        &from->label(),
    });
    std::string to_label = to->label().GetUserVisibleName(with_toolchain);
    std::string from_label = from->label().GetUserVisibleName(with_toolchain);
    *err = Err(from->defined_from(), "Dependency not allowed.",
               "The item " + from_label +
                   "\n"
                   "can not depend on " +
                   to_label +
                   "\n"
                   "because it is not in " +
                   to_label +
                   "'s visibility list: " + to->visibility().Describe(0, true));
    return false;
  }
  return true;
}

// static
bool Visibility::FillItemVisibility(Item* item, Scope* scope, Err* err) {
  const Value* vis_value = scope->GetValue(variables::kVisibility, true);
  if (vis_value)
    item->visibility().Set(
        scope->GetSourceDir(),
        scope->settings()->build_settings()->root_path_utf8(), *vis_value, err);
  else  // Default to public.
    item->visibility().SetPublic();
  return !err->has_error();
}
