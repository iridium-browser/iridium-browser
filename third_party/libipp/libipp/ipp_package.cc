// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libipp/ipp_package.h"

#include "libipp/ipp_attribute.h"

namespace ipp {

// Methods for Package

Group* Package::GetGroup(GroupTag gn) {
  const std::vector<Group*> groups = GetKnownGroups();
  for (auto g : groups)
    if (g->GetName() == gn)
      return g;
  for (auto g : unknown_groups_)
    if (g->GetName() == gn)
      return g;
  return nullptr;
}

Group* Package::AddUnknownGroup(GroupTag gn, bool is_a_set) {
  if (GetGroup(gn) != nullptr)
    return nullptr;
  unknown_groups_.push_back(new UnknownGroup(gn, is_a_set));
  return unknown_groups_.back();
}

std::vector<Group*> Package::GetAllGroups() {
  std::vector<Group*> groups = GetKnownGroups();
  groups.insert(groups.end(), unknown_groups_.begin(), unknown_groups_.end());
  return groups;
}

Package::~Package() {
  for (auto g : unknown_groups_)
    delete g;
}

// Methods for UnknownGroup

UnknownGroup::UnknownGroup(GroupTag name, bool is_a_set)
    : Group(name, is_a_set), groups_(is_a_set ? 0 : 1) {}

UnknownGroup::~UnknownGroup() {
  for (auto g : groups_)
    delete g;
}

void UnknownGroup::Resize(size_t new_size) {
  if (!IsASet())
    return;
  while (groups_.size() > new_size) {
    delete groups_.back();
    groups_.pop_back();
  }
  while (groups_.size() < new_size)
    groups_.push_back(new Collection);
}

// Methods for Collection

Collection::~Collection() {
  for (const auto& kv : unknown_attrs_) {
    delete kv.second;
  }
}

Attribute* Collection::GetAttribute(AttrName an) {
  const std::vector<Attribute*> known_attr = GetKnownAttributes();
  for (auto a : known_attr)
    if (a->GetNameAsEnum() == an)
      return a;
  auto it2 = unknown_attrs_.find(ToString(an));
  if (it2 != unknown_attrs_.end())
    return it2->second;
  return nullptr;
}

Attribute* Collection::GetAttribute(const std::string& name) {
  auto it = unknown_attrs_.find(name);
  if (it != unknown_attrs_.end())
    return it->second;
  AttrName an;
  if (FromString(name, &an)) {
    const std::vector<Attribute*> known_attr = GetKnownAttributes();
    for (auto a : known_attr)
      if (a->GetNameAsEnum() == an)
        return a;
  }
  return nullptr;
}

Attribute* Collection::AddUnknownAttribute(const std::string& name,
                                           bool is_a_set,
                                           AttrType type) {
  // name cannot be empty
  if (name.empty())
    return nullptr;
  // type must be correct
  if (ToString(type) == "")
    return nullptr;
  // check, if given name is not used
  if (unknown_attrs_.count(name))
    return nullptr;
  AttrName an;
  if (FromString(name, &an)) {
    const std::vector<Attribute*> known_attr = GetKnownAttributes();
    for (auto a : known_attr)
      if (a->GetNameAsEnum() == an)
        return nullptr;
  }
  // add new attribute
  Attribute* attr = nullptr;
  if (type == AttrType::collection)
    attr = new UnknownCollectionAttribute(name, is_a_set);
  else
    attr = new UnknownValueAttribute(name, is_a_set, type);
  unknown_attrs_names_.push_back(name);
  unknown_attrs_[name] = attr;
  return attr;
}

void Collection::ResetAllAttributes() {
  const std::vector<Attribute*> attrs = GetAllAttributes();
  for (auto a : attrs) {
    a->Resize(0);
    a->SetState(AttrState::unset);
    if (a->GetCollection() != nullptr)
      a->GetCollection()->ResetAllAttributes();
  }
}

std::vector<Attribute*> Collection::GetAllAttributes() {
  std::vector<Attribute*> attrs = GetKnownAttributes();
  attrs.reserve(attrs.size() + unknown_attrs_names_.size());
  for (auto an : unknown_attrs_names_)
    attrs.push_back(unknown_attrs_[an]);
  return attrs;
}

}  // namespace ipp
