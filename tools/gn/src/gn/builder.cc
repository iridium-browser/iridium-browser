// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/builder.h"

#include <stddef.h>
#include <algorithm>
#include <utility>

#include "gn/action_values.h"
#include "gn/config.h"
#include "gn/deps_iterator.h"
#include "gn/err.h"
#include "gn/loader.h"
#include "gn/pool.h"
#include "gn/scheduler.h"
#include "gn/settings.h"
#include "gn/target.h"
#include "gn/trace.h"

namespace {

using BuilderRecordSet = BuilderRecord::BuilderRecordSet;

// Recursively looks in the tree for a given node, returning true if it
// was found in the dependency graph. This is used to see if a given node
// participates in a cycle.
//
// If this returns true, the cycle will be in *path. This should point to an
// empty vector for the first call. During computation, the path will contain
// the full dependency path to the current node.
//
// Return false means no cycle was found.
bool RecursiveFindCycle(const BuilderRecord* search_in,
                        std::vector<const BuilderRecord*>* path) {
  path->push_back(search_in);
  for (const auto& cur : search_in->GetSortedUnresolvedDeps()) {
    std::vector<const BuilderRecord*>::iterator found =
        std::find(path->begin(), path->end(), cur);
    if (found != path->end()) {
      // This item is already in the set, we found the cycle. Everything before
      // the first definition of cur is irrelevant to the cycle.
      path->erase(path->begin(), found);
      path->push_back(cur);
      return true;
    }

    if (RecursiveFindCycle(cur, path))
      return true;  // Found cycle.
  }
  path->pop_back();
  return false;
}

}  // namespace

Builder::Builder(Loader* loader) : loader_(loader) {}

Builder::~Builder() = default;

void Builder::ItemDefined(std::unique_ptr<Item> item) {
  ScopedTrace trace(TraceItem::TRACE_DEFINE_TARGET, item->label());
  trace.SetToolchain(item->settings()->toolchain_label());

  BuilderRecord::ItemType type = BuilderRecord::TypeOfItem(item.get());

  Err err;
  BuilderRecord* record =
      GetOrCreateRecordOfType(item->label(), item->defined_from(), type, &err);
  if (!record) {
    g_scheduler->FailWithError(err);
    return;
  }

  // Check that it's not been already defined.
  if (record->item()) {
    bool with_toolchain = item->settings()->ShouldShowToolchain({&item->label()});
    err = Err(item->defined_from(), "Duplicate definition.",
              "The item\n  " + item->label().GetUserVisibleName(with_toolchain) +
                  "\nwas already defined.");
    err.AppendSubErr(
        Err(record->item()->defined_from(), "Previous definition:"));
    g_scheduler->FailWithError(err);
    return;
  }

  record->set_item(std::move(item));

  // Do target-specific dependency setup. This will also schedule dependency
  // loads for targets that are required.
  switch (type) {
    case BuilderRecord::ITEM_TARGET:
      TargetDefined(record, &err);
      break;
    case BuilderRecord::ITEM_CONFIG:
      ConfigDefined(record, &err);
      break;
    case BuilderRecord::ITEM_TOOLCHAIN:
      ToolchainDefined(record, &err);
      break;
    default:
      break;
  }
  if (err.has_error()) {
    g_scheduler->FailWithError(err);
    return;
  }

  if (record->can_resolve()) {
    if (!ResolveItem(record, &err)) {
      g_scheduler->FailWithError(err);
      return;
    }
  }
}

const Item* Builder::GetItem(const Label& label) const {
  const BuilderRecord* record = GetRecord(label);
  if (!record)
    return nullptr;
  return record->item();
}

const Toolchain* Builder::GetToolchain(const Label& label) const {
  const BuilderRecord* record = GetRecord(label);
  if (!record)
    return nullptr;
  if (!record->item())
    return nullptr;
  return record->item()->AsToolchain();
}

std::vector<const BuilderRecord*> Builder::GetAllRecords() const {
  std::vector<const BuilderRecord*> result;
  result.reserve(records_.size());
  for (const auto& record : records_)
    result.push_back(&record);
  // Ensure deterministic outputs.
  std::sort(result.begin(), result.end(), BuilderRecord::LabelCompare);
  return result;
}

std::vector<const Item*> Builder::GetAllResolvedItems() const {
  std::vector<const Item*> result;
  result.reserve(records_.size());
  for (const auto& record : records_) {
    if (record.type() != BuilderRecord::ITEM_UNKNOWN &&
        record.should_generate() && record.item()) {
      result.push_back(record.item());
    }
  }
  // Ensure deterministic outputs.
  std::sort(result.begin(), result.end(), [](const Item* a, const Item* b) {
    return a->label() < b->label();
  });
  return result;
}

std::vector<const Target*> Builder::GetAllResolvedTargets() const {
  std::vector<const Target*> result;
  result.reserve(records_.size());
  for (const auto& record : records_) {
    if (record.type() == BuilderRecord::ITEM_TARGET &&
        record.should_generate() && record.item())
      result.push_back(record.item()->AsTarget());
  }
  // Ensure deterministic outputs.
  std::sort(result.begin(), result.end(), [](const Target* a, const Target* b) {
    return a->label() < b->label();
  });
  return result;
}

const BuilderRecord* Builder::GetRecord(const Label& label) const {
  // Forward to the non-const version.
  return const_cast<Builder*>(this)->GetRecord(label);
}

BuilderRecord* Builder::GetRecord(const Label& label) {
  return records_.find(label);
}

bool Builder::CheckForBadItems(Err* err) const {
  // Look for errors where we find a defined node with an item that refers to
  // an undefined one with no item. There may be other nodes in turn depending
  // on our defined one, but listing those isn't helpful: we want to find the
  // broken link.
  //
  // This finds normal "missing dependency" errors but does not find circular
  // dependencies because in this case all items in the cycle will be GENERATED
  // but none will be resolved. If this happens, we'll check explicitly for
  // that below.
  std::vector<const BuilderRecord*> bad_records;
  for (const auto& src : records_) {
    if (!src.should_generate())
      continue;  // Skip ungenerated nodes.

    if (!src.resolved())
      bad_records.push_back(&src);
  }
  if (bad_records.empty())
    return true;

  // Sort by label to ensure deterministic outputs.
  std::sort(bad_records.begin(), bad_records.end(),
            BuilderRecord::LabelCompare);

  std::string depstring;
  for (const auto& src : bad_records) {
    // Check dependencies.
    for (const auto* dest : src->GetSortedUnresolvedDeps()) {
      if (!dest->item()) {
        depstring += src->label().GetUserVisibleName(true) + "\n  needs " +
                     dest->label().GetUserVisibleName(true) + "\n";
      }
    }
  }

  if (!depstring.empty()) {
    *err = Err(Location(), "Unresolved dependencies.", depstring);
    return false;
  }

  if (!bad_records.empty()) {
    // Our logic above found a bad node but didn't identify the problem. This
    // normally means a circular dependency.
    depstring = CheckForCircularDependencies(bad_records);
    if (depstring.empty()) {
      // Something's very wrong, just dump out the bad nodes.
      depstring =
          "I have no idea what went wrong, but these are unresolved, "
          "possibly due to an\ninternal error:";
      for (auto* bad_record : bad_records) {
        depstring +=
            "\n\"" + bad_record->label().GetUserVisibleName(true) + "\"";
      }
      *err = Err(Location(), "", depstring);
    } else {
      *err = Err(Location(), "Dependency cycle:", depstring);
    }
    return false;
  }

  return true;
}

bool Builder::TargetDefined(BuilderRecord* record, Err* err) {
  Target* target = record->item()->AsTarget();

  if (!AddDeps(record, target->public_deps(), err) ||
      !AddDeps(record, target->private_deps(), err) ||
      !AddDeps(record, target->data_deps(), err) ||
      !AddDeps(record, target->configs().vector(), err) ||
      !AddDeps(record, target->all_dependent_configs(), err) ||
      !AddDeps(record, target->public_configs(), err) ||
      !AddGenDeps(record, target->gen_deps(), err) ||
      !AddPoolDep(record, target, err) ||
      !AddToolchainDep(record, target, err))
    return false;

  // All targets in the default toolchain get generated by default. We also
  // check if this target was previously marked as "required" and force setting
  // the bit again so the target's dependencies (which we now know) get the
  // required bit pushed to them.
  if (record->should_generate() || target->settings()->is_default())
    RecursiveSetShouldGenerate(record, true);

  return true;
}

bool Builder::ConfigDefined(BuilderRecord* record, Err* err) {
  Config* config = record->item()->AsConfig();
  if (!AddDeps(record, config->configs(), err))
    return false;

  // Make sure all deps of this config are scheduled to be loaded. For other
  // item types like targets, the "should generate" flag is propagated around
  // to mark whether this should happen. We could call
  // RecursiveSetShouldGenerate to do this step here, but since configs nor
  // anything they depend on is actually written, the "generate" flag isn't
  // relevant and means extra book keeping. Just force load any deps of this
  // config.
  for (auto it = record->all_deps().begin(); it.valid(); ++it) {
    ScheduleItemLoadIfNecessary(*it);
  }

  return true;
}

bool Builder::ToolchainDefined(BuilderRecord* record, Err* err) {
  Toolchain* toolchain = record->item()->AsToolchain();

  if (!AddDeps(record, toolchain->deps(), err))
    return false;

  for (const auto& tool : toolchain->tools()) {
    if (tool.second->pool().label.is_null())
      continue;

    BuilderRecord* dep_record = GetOrCreateRecordOfType(
        tool.second->pool().label, tool.second->pool().origin,
        BuilderRecord::ITEM_POOL, err);
    if (!dep_record)
      return false;
    record->AddDep(dep_record);
  }

  // The default toolchain gets generated by default. Also propagate the
  // generate flag if it depends on items in a non-default toolchain.
  if (record->should_generate() ||
      toolchain->settings()->default_toolchain_label() == toolchain->label())
    RecursiveSetShouldGenerate(record, true);

  loader_->ToolchainLoaded(toolchain);
  return true;
}

BuilderRecord* Builder::GetOrCreateRecordForTesting(const Label& label) {
  Err err;
  return GetOrCreateRecordOfType(label, nullptr, BuilderRecord::ITEM_UNKNOWN, &err);
}

BuilderRecord* Builder::GetOrCreateRecordOfType(const Label& label,
                                                const ParseNode* request_from,
                                                BuilderRecord::ItemType type,
                                                Err* err) {
  auto pair = records_.try_emplace(label, request_from, type);
  BuilderRecord* record = pair.second;

  // Check types, if the record was not just created.
  if (!pair.first && record->type() != type) {
    std::string msg =
        "The type of " + label.GetUserVisibleName(true) + "\nhere is a " +
        BuilderRecord::GetNameForType(type) + " but was previously seen as a " +
        BuilderRecord::GetNameForType(record->type()) +
        ".\n\n"
        "The most common cause is that the label of a config was put in the\n"
        "in the deps section of a target (or vice-versa).";
    *err = Err(request_from, "Item type does not match.", msg);
    if (record->originally_referenced_from()) {
      err->AppendSubErr(
          Err(record->originally_referenced_from(), std::string()));
    }
    return nullptr;
  }

  return record;
}

BuilderRecord* Builder::GetResolvedRecordOfType(const Label& label,
                                                const ParseNode* origin,
                                                BuilderRecord::ItemType type,
                                                Err* err) {
  BuilderRecord* record = GetRecord(label);
  if (!record) {
    *err = Err(origin, "Item not found",
               "\"" + label.GetUserVisibleName(true) +
                   "\" doesn't\n"
                   "refer to an existent thing.");
    return nullptr;
  }

  const Item* item = record->item();
  if (!item) {
    *err = Err(
        origin, "Item not resolved.",
        "\"" + label.GetUserVisibleName(true) + "\" hasn't been resolved.\n");
    return nullptr;
  }

  if (!BuilderRecord::IsItemOfType(item, type)) {
    *err =
        Err(origin,
            std::string("This is not a ") + BuilderRecord::GetNameForType(type),
            "\"" + label.GetUserVisibleName(true) + "\" refers to a " +
                item->GetItemTypeName() + " instead of a " +
                BuilderRecord::GetNameForType(type) + ".");
    return nullptr;
  }
  return record;
}

bool Builder::AddDeps(BuilderRecord* record,
                      const LabelConfigVector& configs,
                      Err* err) {
  for (const auto& config : configs) {
    BuilderRecord* dep_record = GetOrCreateRecordOfType(
        config.label, config.origin, BuilderRecord::ITEM_CONFIG, err);
    if (!dep_record)
      return false;
    record->AddDep(dep_record);
  }
  return true;
}

bool Builder::AddDeps(BuilderRecord* record,
                      const UniqueVector<LabelConfigPair>& configs,
                      Err* err) {
  for (const auto& config : configs) {
    BuilderRecord* dep_record = GetOrCreateRecordOfType(
        config.label, config.origin, BuilderRecord::ITEM_CONFIG, err);
    if (!dep_record)
      return false;
    record->AddDep(dep_record);
  }
  return true;
}

bool Builder::AddDeps(BuilderRecord* record,
                      const LabelTargetVector& targets,
                      Err* err) {
  for (const auto& target : targets) {
    BuilderRecord* dep_record = GetOrCreateRecordOfType(
        target.label, target.origin, BuilderRecord::ITEM_TARGET, err);
    if (!dep_record)
      return false;
    record->AddDep(dep_record);
  }
  return true;
}

bool Builder::AddGenDeps(BuilderRecord* record,
                         const LabelTargetVector& targets,
                         Err* err) {
  for (const auto& target : targets) {
    BuilderRecord* dep_record = GetOrCreateRecordOfType(
        target.label, target.origin, BuilderRecord::ITEM_TARGET, err);
    if (!dep_record)
      return false;
    record->AddGenDep(dep_record);
  }
  return true;
}

bool Builder::AddPoolDep(BuilderRecord* record,
                         const Target* target,
                         Err* err) {
  if (target->pool().label.is_null())
    return true;

  BuilderRecord* pool_record = GetOrCreateRecordOfType(
      target->pool().label, target->pool().origin,
      BuilderRecord::ITEM_POOL, err);
  if (!pool_record)
    return false;
  record->AddDep(pool_record);

  return true;
}

bool Builder::AddToolchainDep(BuilderRecord* record,
                              const Target* target,
                              Err* err) {
  BuilderRecord* toolchain_record = GetOrCreateRecordOfType(
      target->settings()->toolchain_label(), target->defined_from(),
      BuilderRecord::ITEM_TOOLCHAIN, err);
  if (!toolchain_record)
    return false;
  record->AddDep(toolchain_record);

  return true;
}

void Builder::RecursiveSetShouldGenerate(BuilderRecord* record, bool force) {
  if (!record->should_generate()) {
    // This function can encounter cycles because gen_deps aren't a DAG. Setting
    // the should_generate flag before iterating avoids infinite recursion in
    // that case.
    record->set_should_generate(true);

    // This may have caused the item to go into "resolved and generated" state.
    if (record->resolved() && resolved_and_generated_callback_)
      resolved_and_generated_callback_(record);
  } else if (!force) {
    return;  // Already set and we're not required to iterate dependencies.
  }

  for (auto it = record->all_deps().begin(); it.valid(); ++it) {
    BuilderRecord* cur = *it;
    if (!cur->should_generate()) {
      ScheduleItemLoadIfNecessary(cur);
      RecursiveSetShouldGenerate(cur, false);
    }
  }
}

void Builder::ScheduleItemLoadIfNecessary(BuilderRecord* record) {
  const ParseNode* origin = record->originally_referenced_from();
  loader_->Load(record->label(), origin ? origin->GetRange() : LocationRange());
}

bool Builder::ResolveItem(BuilderRecord* record, Err* err) {
  DCHECK(record->can_resolve() && !record->resolved());

  if (record->type() == BuilderRecord::ITEM_TARGET) {
    Target* target = record->item()->AsTarget();
    if (!ResolveDeps(&target->public_deps(), err) ||
        !ResolveDeps(&target->private_deps(), err) ||
        !ResolveDeps(&target->data_deps(), err) ||
        !ResolveConfigs(&target->configs(), err) ||
        !ResolveConfigs(&target->all_dependent_configs(), err) ||
        !ResolveConfigs(&target->public_configs(), err) ||
        !ResolvePool(target, err) ||
        !ResolveToolchain(target, err))
      return false;
  } else if (record->type() == BuilderRecord::ITEM_CONFIG) {
    Config* config = record->item()->AsConfig();
    if (!ResolveConfigs(&config->configs(), err))
      return false;
  } else if (record->type() == BuilderRecord::ITEM_TOOLCHAIN) {
    Toolchain* toolchain = record->item()->AsToolchain();
    if (!ResolveDeps(&toolchain->deps(), err))
      return false;
    if (!ResolvePools(toolchain, err))
      return false;
  }

  record->set_resolved(true);

  if (!record->item()->OnResolved(err))
    return false;
  if (record->should_generate() && resolved_and_generated_callback_)
    resolved_and_generated_callback_(record);

  // Recursively update everybody waiting on this item to be resolved.
  const BuilderRecordSet waiting_deps = record->waiting_on_resolution();
  for (auto it = waiting_deps.begin(); it.valid(); ++it) {
    BuilderRecord* waiting = *it;
    if (waiting->OnResolvedDep(record)) {
      if (!ResolveItem(waiting, err))
        return false;
    }
  }
  record->waiting_on_resolution().clear();
  return true;
}

bool Builder::ResolveDeps(LabelTargetVector* deps, Err* err) {
  for (LabelTargetPair& cur : *deps) {
    DCHECK(!cur.ptr);

    BuilderRecord* record = GetResolvedRecordOfType(
        cur.label, cur.origin, BuilderRecord::ITEM_TARGET, err);
    if (!record)
      return false;
    cur.ptr = record->item()->AsTarget();
  }
  return true;
}

bool Builder::ResolveConfigs(UniqueVector<LabelConfigPair>* configs, Err* err) {
  for (const auto& cur : *configs) {
    DCHECK(!cur.ptr);

    BuilderRecord* record = GetResolvedRecordOfType(
        cur.label, cur.origin, BuilderRecord::ITEM_CONFIG, err);
    if (!record)
      return false;
    const_cast<LabelConfigPair&>(cur).ptr = record->item()->AsConfig();
  }
  return true;
}

bool Builder::ResolveToolchain(Target* target, Err* err) {
  BuilderRecord* record = GetResolvedRecordOfType(
      target->settings()->toolchain_label(), target->defined_from(),
      BuilderRecord::ITEM_TOOLCHAIN, err);
  if (!record) {
    *err = Err(
        target->defined_from(), "Toolchain for target not defined.",
        "I was hoping to find a toolchain " +
            target->settings()->toolchain_label().GetUserVisibleName(false));
    return false;
  }

  if (!target->SetToolchain(record->item()->AsToolchain(), err))
    return false;

  return true;
}

bool Builder::ResolvePool(Target* target, Err* err) {
  if (target->pool().label.is_null())
    return true;

  BuilderRecord* record = GetResolvedRecordOfType(
      target->pool().label, target->pool().origin,
      BuilderRecord::ITEM_POOL, err);
  if (!record)
    return false;
  target->set_pool(LabelPtrPair<Pool>(record->item()->AsPool()));

  return true;
}

bool Builder::ResolvePools(Toolchain* toolchain, Err* err) {
  for (const auto& tool : toolchain->tools()) {
    if (tool.second->pool().label.is_null())
      continue;

    BuilderRecord* record = GetResolvedRecordOfType(
        tool.second->pool().label, toolchain->defined_from(),
        BuilderRecord::ITEM_POOL, err);
    if (!record) {
      *err = Err(tool.second->pool().origin, "Pool for tool not defined.",
                 "I was hoping to find a pool " +
                     tool.second->pool().label.GetUserVisibleName(false));
      return false;
    }

    tool.second->set_pool(LabelPtrPair<Pool>(record->item()->AsPool()));
  }

  return true;
}

std::string Builder::CheckForCircularDependencies(
    const std::vector<const BuilderRecord*>& bad_records) const {
  std::vector<const BuilderRecord*> cycle;
  if (!RecursiveFindCycle(bad_records[0], &cycle))
    return std::string();  // Didn't find a cycle, something else is wrong.

  std::string ret;
  for (size_t i = 0; i < cycle.size(); i++) {
    ret += "  " + cycle[i]->label().GetUserVisibleName(loader_->GetDefaultToolchain());
    if (i != cycle.size() - 1)
      ret += " ->";
    ret += "\n";
  }

  return ret;
}
