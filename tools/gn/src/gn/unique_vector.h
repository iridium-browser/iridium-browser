// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_UNIQUE_VECTOR_H_
#define TOOLS_GN_UNIQUE_VECTOR_H_

#include <stddef.h>

#include <algorithm>
#include <vector>

#include "hash_table_base.h"

// A HashTableBase node type used by all UniqueVector<> instantiations.
// The node stores the item's hash value and its index plus 1, in order
// to follow the HashTableBase requirements (i.e. zero-initializable null
// value).
struct UniqueVectorNode {
  size_t hash;
  size_t index_plus1;

  size_t hash_value() const { return hash; }

  bool is_valid() const { return !is_null(); }

  bool is_null() const { return index_plus1 == 0; }

  // Do not support deletion, making lookup faster.
  static constexpr bool is_tombstone() { return false; }

  // Return vector index.
  size_t index() const { return index_plus1 - 1u; }

  // Create new instance from hash value and vector index.
  static UniqueVectorNode Make(size_t hash, size_t index) {
    return {hash, index + 1u};
  }
};

using UniqueVectorHashTableBase = HashTableBase<UniqueVectorNode>;

// A common HashSet implementation used by all UniqueVector instantiations.
class UniqueVectorHashSet : public UniqueVectorHashTableBase {
 public:
  using BaseType = UniqueVectorHashTableBase;
  using Node = BaseType::Node;

  // Specialized Lookup() template function.
  // |hash| is the hash value for |item|.
  // |item| is the item search key being looked up.
  // |vector| is containing vector for existing items.
  //
  // Returns a non-null mutable Node pointer.
  template <typename T>
  Node* Lookup(size_t hash, const T& item, const std::vector<T>& vector) const {
    return BaseType::NodeLookup(hash, [&](const Node* node) {
      return node->hash == hash && vector[node->index()] == item;
    });
  }

  // Specialized Insert() function that converts |index| into the proper
  // UniqueVectorKey type.
  void Insert(Node* node, size_t hash, size_t index) {
    *node = Node::Make(hash, index);
    BaseType::UpdateAfterInsert();
  }

  void Clear() { NodeClear(); }
};

// An ordered set optimized for GN's usage. Such sets are used to store lists
// of configs and libraries, and are appended to but not randomly inserted
// into.
template <typename T>
class UniqueVector {
 public:
  using Vector = std::vector<T>;
  using iterator = typename Vector::iterator;
  using const_iterator = typename Vector::const_iterator;

  const Vector& vector() const { return vector_; }
  size_t size() const { return vector_.size(); }
  bool empty() const { return vector_.empty(); }
  void clear() {
    vector_.clear();
    set_.Clear();
  }
  void reserve(size_t s) { vector_.reserve(s); }

  const T& operator[](size_t index) const { return vector_[index]; }

  const_iterator begin() const { return vector_.begin(); }
  const_iterator end() const { return vector_.end(); }

  // Returns true if the item was appended, false if it already existed (and
  // thus the vector was not modified).
  bool push_back(const T& t) {
    size_t hash = std::hash<T>()(t);
    auto* node = set_.Lookup(hash, t, vector_);
    if (node->is_valid()) {
      return false;  // Already have this one.
    }

    vector_.push_back(t);
    set_.Insert(node, hash, vector_.size() - 1);
    return true;
  }

  bool push_back(T&& t) {
    size_t hash = std::hash<T>()(t);
    auto* node = set_.Lookup(hash, t, vector_);
    if (node->is_valid()) {
      return false;  // Already have this one.
    }

    vector_.push_back(std::move(t));
    set_.Insert(node, hash, vector_.size() - 1);
    return true;
  }

  // Appends a range of items from an iterator.
  template <typename iter>
  void Append(const iter& begin, const iter& end) {
    for (iter i = begin; i != end; ++i)
      push_back(*i);
  }

  void Append(const UniqueVector& other) {
    Append(other.begin(), other.end());
  }

  // Returns the index of the item matching the given value in the list, or
  // (size_t)(-1) if it's not found.
  size_t IndexOf(const T& t) const {
    size_t hash = std::hash<T>()(t);
    auto* node = set_.Lookup(hash, t, vector_);
    return node->index();
  }

 private:
  Vector vector_;
  UniqueVectorHashSet set_;
};

#endif  // TOOLS_GN_UNIQUE_VECTOR_H_
