// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/inherited_libraries.h"

#include "gn/target.h"

InheritedLibraries::InheritedLibraries() = default;

InheritedLibraries::~InheritedLibraries() = default;

std::vector<std::pair<const Target*, bool>>
InheritedLibraries::GetOrderedAndPublicFlag() const {
  std::vector<std::pair<const Target*, bool>> result;
  result.reserve(targets_.size());
  for (size_t i = 0; i < targets_.size(); ++i)
    result.emplace_back(targets_[i], public_flags_[i]);
  return result;
}

void InheritedLibraries::Append(const Target* target, bool is_public) {
  // Try to insert a new node.
  auto ret = targets_.PushBackWithIndex(target);
  if (ret.first) {
    public_flags_.push_back(is_public);
  } else if (is_public) {
    // Target already present, if |is_public|, set its flag.
    public_flags_[ret.second] = true;
  }
}

void InheritedLibraries::AppendInherited(const InheritedLibraries& other,
                                         bool is_public) {
  // Append all items in order, mark them public only if the're already public
  // and we're adding them publicly.
  for (size_t i = 0; i < other.targets_.size(); ++i) {
    Append(other.targets_[i], is_public && other.public_flags_[i]);
  }
}

void InheritedLibraries::AppendPublicSharedLibraries(
    const InheritedLibraries& other,
    bool is_public) {
  for (size_t i = 0; i < other.targets_.size(); ++i) {
    const Target* target = other.targets_[i];
    if (target->output_type() == Target::SHARED_LIBRARY &&
        other.public_flags_[i]) {
      Append(target, is_public);
    }
  }
}
