// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_GN_BUILDER_RECORD_MAP_H_
#define SRC_GN_BUILDER_RECORD_MAP_H_

#include "gn/builder_record.h"
#include "gn/hash_table_base.h"

// This header implements a custom Label -> BuilderRecord map that is critical
// for performance of the Builder class.
//
// Measurements show it performs better than
// std::unordered_map<Label, BuilderRecord>, which itself performs much better
// than base::flat_map<Label, BuilderRecord> or std::map<Label, BuilderRecord>
//
// NOTE: This only implements the features required by the Builder class.
//

struct BuilderRecordNode {
  BuilderRecord* record;
  bool is_null() const { return !record; }
  static constexpr bool is_tombstone() { return false; }
  bool is_valid() const { return !is_null() && !is_tombstone(); }
  size_t hash_value() const { return record->label().hash(); }
};

class BuilderRecordMap : public HashTableBase<BuilderRecordNode> {
 public:
  using NodeType = BuilderRecordNode;

  ~BuilderRecordMap() {
    for (auto it = NodeBegin(); it.valid(); ++it) {
      delete it->record;
    }
  }

  // Find BuilderRecord matching |label| or return nullptr.
  BuilderRecord* find(const Label& label) const {
    return Lookup(label)->record;
  }

  // Try to find BuilderRecord matching |label|, and create one if
  // none is found. result.first will be true to indicate that a new
  // record was created.
  std::pair<bool, BuilderRecord*> try_emplace(const Label& label,
                                              const ParseNode* request_from,
                                              BuilderRecord::ItemType type) {
    NodeType* node = Lookup(label);
    if (node->is_valid()) {
      return {false, node->record};
    }
    BuilderRecord* record = new BuilderRecord(type, label, request_from);
    node->record = record;
    UpdateAfterInsert();
    return {true, record};
  }

  // Iteration support
  struct const_iterator : public NodeIterator {
    const BuilderRecord& operator*() const { return *node_->record; }
    BuilderRecord& operator*() { return *node_->record; }

    const BuilderRecord* operator->() const { return node_->record; }
    BuilderRecord* operator->() { return node_->record; }
  };

  const_iterator begin() const { return {NodeBegin()}; }
  const_iterator end() const { return {NodeEnd()}; }

 private:
  NodeType* Lookup(const Label& label) const {
    return NodeLookup(label.hash(), [&label](const NodeType* node) {
      return node->record->label() == label;
    });
  }
};

#endif  // SRC_GN_BUILDER_RECORD_MAP_H_
