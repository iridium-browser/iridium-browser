// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_UNIQUE_VECTOR_H_
#define TOOLS_GN_UNIQUE_VECTOR_H_

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <functional>
#include <vector>

#include "hash_table_base.h"

// A HashTableBase node type used by all UniqueVector<> instantiations.
// The node stores the item's hash value and its index plus 1, in order
// to follow the HashTableBase requirements (i.e. zero-initializable null
// value).
struct UniqueVectorNode {
  uint32_t hash32;
  uint32_t index_plus1;

  size_t hash_value() const { return hash32; }

  bool is_valid() const { return !is_null(); }

  bool is_null() const { return index_plus1 == 0; }

  // Do not support deletion, making lookup faster.
  static constexpr bool is_tombstone() { return false; }

  // Return vector index.
  size_t index() const { return index_plus1 - 1u; }

  static uint32_t ToHash32(size_t hash) { return static_cast<uint32_t>(hash); }

  // Create new instance from hash value and vector index.
  static UniqueVectorNode Make(size_t hash, size_t index) {
    return {ToHash32(hash), static_cast<uint32_t>(index + 1u)};
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
  template <typename T, typename EqualTo = std::equal_to<T>>
  Node* Lookup(size_t hash, const T& item, const std::vector<T>& vector) const {
    uint32_t hash32 = Node::ToHash32(hash);
    return BaseType::NodeLookup(hash32, [&](const Node* node) {
      return hash32 == node->hash32 && EqualTo()(vector[node->index()], item);
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
template <typename T,
          typename Hash = std::hash<T>,
          typename EqualTo = std::equal_to<T>>
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

  // Extract the vector out of the instance, clearing it at the same time.
  Vector release() {
    Vector result = std::move(vector_);
    clear();
    return result;
  }

  // Returns true if the item was appended, false if it already existed (and
  // thus the vector was not modified).
  bool push_back(const T& t) {
    size_t hash;
    auto* node = Lookup(t, &hash);
    if (node->is_valid()) {
      return false;  // Already have this one.
    }
    vector_.push_back(t);
    set_.Insert(node, hash, vector_.size() - 1);
    return true;
  }

  // Same as above, but moves the item into the vector if possible.
  bool push_back(T&& t) {
    size_t hash = Hash()(t);
    auto* node = Lookup(t, &hash);
    if (node->is_valid()) {
      return false;  // Already have this one.
    }
    vector_.push_back(std::move(t));
    set_.Insert(node, hash, vector_.size() - 1);
    return true;
  }

  // Construct an item in-place if possible. Return true if it was
  // appended, false otherwise.
  template <typename... ARGS>
  bool emplace_back(ARGS... args) {
    return push_back(T{std::forward<ARGS>(args)...});
  }

  // Try to add an item to the vector. Return (true, index) in
  // case of insertion, or (false, index) otherwise. In both cases
  // |index| will be the item's index in the set and will not be
  // kIndexNone. This can be used to implement a map using a
  // UniqueVector<> for keys, and a parallel array for values.
  std::pair<bool, size_t> PushBackWithIndex(const T& t) {
    size_t hash;
    auto* node = Lookup(t, &hash);
    if (node->is_valid()) {
      return {false, node->index()};
    }
    size_t result = vector_.size();
    vector_.push_back(t);
    set_.Insert(node, hash, result);
    return {true, result};
  }

  // Same as above, but moves the item into the set on success.
  std::pair<bool, size_t> PushBackWithIndex(T&& t) {
    size_t hash;
    auto* node = Lookup(t, &hash);
    if (node->is_valid()) {
      return {false, node->index()};
    }
    size_t result = vector_.size();
    vector_.push_back(std::move(t));
    set_.Insert(node, hash, result);
    return {true, result};
  }

  // Construct an item in-place if possible. If a corresponding
  // item is already in the vector, return (false, index), otherwise
  // perform the insertion and return (true, index). In both cases
  // |index| will be the item's index in the vector and will not be
  // kIndexNone.
  template <typename... ARGS>
  std::pair<bool, size_t> EmplaceBackWithIndex(ARGS... args) {
    return PushBackWithIndex(T{std::forward<ARGS>(args)...});
  }

  // Appends a range of items from an iterator.
  template <typename iter>
  void Append(const iter& begin, const iter& end) {
    for (iter i = begin; i != end; ++i)
      push_back(*i);
  }

  // Append from any iterable container with begin() and end()
  // methods, whose iterators derefence to values convertible to T.
  template <typename C,
            typename = std::void_t<
                decltype(static_cast<const T>(*std::declval<C>().begin())),
                decltype(static_cast<const T>(*std::declval<C>().end()))>>
  void Append(const C& other) {
    Append(other.begin(), other.end());
  }

  // Append from any moveable iterable container with begin() and
  // end() methods. This variant moves items from the container
  // into the UniqueVector instance.
  template <typename C,
            typename = std::void_t<
                decltype(static_cast<T>(*std::declval<C>().begin())),
                decltype(static_cast<T>(*std::declval<C>().end()))>>
  void Append(C&& other) {
    for (auto it = other.begin(); it != other.end(); ++it)
      push_back(std::move(*it));
  }

  // Returns true if the item is already in the vector.
  bool Contains(const T& t) const {
    size_t hash;
    return Lookup(t, &hash)->is_valid();
  }

  // Returns the index of the item matching the given value in the list, or
  // kIndexNone if it's not found.
  size_t IndexOf(const T& t) const {
    size_t hash;
    return Lookup(t, &hash)->index();
  }

  static constexpr size_t kIndexNone = 0xffffffffu;

 private:
  // Lookup hash set node for item |t|, also sets |*hash| to the hash value.
  UniqueVectorNode* Lookup(const T& t, size_t* hash) const {
    *hash = Hash()(t);
    return set_.Lookup<T, EqualTo>(*hash, t, vector_);
  }

  Vector vector_;
  UniqueVectorHashSet set_;
};

#endif  // TOOLS_GN_UNIQUE_VECTOR_H_
