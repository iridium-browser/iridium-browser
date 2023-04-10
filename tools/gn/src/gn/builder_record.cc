// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/builder_record.h"

#include "gn/item.h"

BuilderRecord::BuilderRecord(ItemType type,
                             const Label& label,
                             const ParseNode* originally_referenced_from)
    : type_(type),
      label_(label),
      originally_referenced_from_(originally_referenced_from) {}

// static
const char* BuilderRecord::GetNameForType(ItemType type) {
  switch (type) {
    case ITEM_TARGET:
      return "target";
    case ITEM_CONFIG:
      return "config";
    case ITEM_TOOLCHAIN:
      return "toolchain";
    case ITEM_POOL:
      return "pool";
    case ITEM_UNKNOWN:
    default:
      return "unknown";
  }
}

// static
bool BuilderRecord::IsItemOfType(const Item* item, ItemType type) {
  switch (type) {
    case ITEM_TARGET:
      return !!item->AsTarget();
    case ITEM_CONFIG:
      return !!item->AsConfig();
    case ITEM_TOOLCHAIN:
      return !!item->AsToolchain();
    case ITEM_POOL:
      return !!item->AsPool();
    case ITEM_UNKNOWN:
    default:
      return false;
  }
}

// static
BuilderRecord::ItemType BuilderRecord::TypeOfItem(const Item* item) {
  if (item->AsTarget())
    return ITEM_TARGET;
  if (item->AsConfig())
    return ITEM_CONFIG;
  if (item->AsToolchain())
    return ITEM_TOOLCHAIN;
  if (item->AsPool())
    return ITEM_POOL;

  NOTREACHED();
  return ITEM_UNKNOWN;
}

void BuilderRecord::AddGenDep(BuilderRecord* record) {
  // Records don't have to wait on resolution of their gen deps, since all they
  // need to do is propagate should_generate to them.
  all_deps_.insert(record);
}

bool BuilderRecord::OnResolvedDep(const BuilderRecord* dep) {
  DCHECK(all_deps_.contains(const_cast<BuilderRecord*>(dep)));
  DCHECK(unresolved_count_ > 0);
  return --unresolved_count_ == 0;
}

std::vector<const BuilderRecord*> BuilderRecord::GetSortedUnresolvedDeps()
    const {
  std::vector<const BuilderRecord*> result;
  for (auto it = all_deps_.begin(); it.valid(); ++it) {
    BuilderRecord* dep = *it;
    if (dep->waiting_on_resolution_.contains(const_cast<BuilderRecord*>(this)))
      result.push_back(dep);
  }
  std::sort(result.begin(), result.end(), LabelCompare);
  return result;
}

void BuilderRecord::AddDep(BuilderRecord* record) {
  if (all_deps_.add(record) && !record->resolved()) {
    unresolved_count_ += 1;
    record->waiting_on_resolution_.add(this);
  }
}
