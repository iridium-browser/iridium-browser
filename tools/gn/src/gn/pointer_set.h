// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_GN_POINTER_SET_H_
#define SRC_GN_POINTER_SET_H_

#include <functional>
#include <vector>

#include "gn/hash_table_base.h"

// PointerSet<T> is a fast implemention of a set of non-owning and non-null
// typed pointer values (of type T*).
//
// Note that this intentionally does not support a find() method
// for performance reasons, instead callers must use contains(), add()
// or erase() directly to perform lookups or conditional insertion/removal.
//
// Only constant iterators are provided. Moreover, the slightly faster
// iteration form can be used for performance as in:
//
//   for (auto it = my_set.begin(); it.valid(); ++it) {
//     T* ptr = *it;
//     ...
//   }
//
// Instead of:
//
//   for (T* ptr : my_set) {
//     ...
//   }
//
// The PointerSetNode type implements the methods required by HashTableBase<>
// to store and hash a pointer directly in the buckets array. The special
// address 1 is used as the tombstone value to support removal.
//
// Null nodes are marked with an empty pointer, which means that nullptr
// itself cannot be stored in the set.
struct PointerSetNode {
  const void* ptr_;
  bool is_null() const { return !ptr_; }
  bool is_tombstone() const { return ptr_ == MakeTombstone(); }
  bool is_valid() const { return !is_null() && !is_tombstone(); }
  size_t hash_value() const { return MakeHash(ptr_); }

  // Return the tomebstone value. This could be a static constexpr value
  // if C++ didn't complain about reinterpret_cast<> here.
  static const void* MakeTombstone() {
    return reinterpret_cast<const void*>(1u);
  }

  // Return the hash corresponding to a given pointer.
  static size_t MakeHash(const void* ptr) {
    return std::hash<const void*>()(ptr);
  }
};

template <typename T>
class PointerSet : public HashTableBase<PointerSetNode> {
 public:
  using NodeType = PointerSetNode;
  using BaseType = HashTableBase<NodeType>;

  PointerSet() = default;

  // Allow copying pointer sets.
  PointerSet(const PointerSet& other) : BaseType() { insert(other); }
  PointerSet& operator=(const PointerSet& other) {
    if (this != &other) {
      this->~PointerSet();
      new (this) PointerSet(other);
    }
    return *this;
  }

  // Redefine move operators since the copy constructor above
  // masks the base ones by default.
  PointerSet(PointerSet&& other) noexcept : BaseType(std::move(other)) {}
  PointerSet& operator=(PointerSet&& other) noexcept {
    if (this != &other) {
      this->~PointerSet();
      new (this) PointerSet(std::move(other));
    }
    return *this;
  }

  // Range constructor.
  template <typename InputIter>
  PointerSet(InputIter first, InputIter last) {
    for (; first != last; ++first)
      add(*first);
  }

  // Clear the set.
  void clear() { NodeClear(); }

  // Add one pointer to the set. Return true if the pointer was
  // added, or false if its was already in the set.
  bool add(T* ptr) {
    NodeType* node = Lookup(ptr);
    if (node->is_valid())
      return false;

    node->ptr_ = ptr;
    UpdateAfterInsert();
    return true;
  }

  // Return true if a given pointer is already in the set.
  bool contains(T* ptr) const { return Lookup(ptr)->is_valid(); }

  // Try to remove a pointer from the set. Return true in case of
  // removal, or false if the pointer was not in the set.
  bool erase(T* ptr) {
    NodeType* node = Lookup(ptr);
    if (!node->is_valid())
      return false;
    node->ptr_ = node->MakeTombstone();
    UpdateAfterRemoval();
    return true;
  }

  // Same as add() but does not return a boolean.
  // This minimizes code changes when PointerSet<> replaces
  // other standard set types in the code.
  void insert(T* ptr) { add(ptr); }

  // Range insertion.
  template <typename InputIter>
  void insert(InputIter first, InputIter last) {
    for (; first != last; ++first)
      add(*first);
  }

  // Insert of aitems of |other| into the current set. This
  // is slightly more efficient than using range insertion
  // with insert(other.begin(), other.end()).
  void insert(const PointerSet& other) {
    for (const_iterator iter = other.begin(); iter.valid(); ++iter)
      add(*iter);
  }

  // Return a new set that is the intersection of the current one
  // and |other|.
  PointerSet intersection_with(const PointerSet& other) const {
    PointerSet result;
    for (const_iterator iter = other.begin(); iter.valid(); ++iter) {
      if (contains(*iter))
        result.add(*iter);
    }
    return result;
  }

  // Only provide const interators for pointer sets.
  struct const_iterator : public NodeIterator {
    T* const* operator->() const {
      return &const_cast<T*>(static_cast<const T*>(node_->ptr_));
    }
    T* operator*() const {
      return const_cast<T*>(static_cast<const T*>(node_->ptr_));
    }

    // The following allows:
    // std::vector<T*> vector(set.begin(), set.end());
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T*;
    using pointer = T**;
    using reference = T*&;
  };

  const_iterator begin() const { return {NodeBegin()}; }
  const_iterator end() const { return {NodeEnd()}; }

  // Only used for unit-tests so performance is not critical.
  bool operator==(const PointerSet& other) const {
    if (size() != other.size())
      return false;

    for (const_iterator iter = begin(); iter.valid(); ++iter)
      if (!other.contains(*iter))
        return false;

    for (const_iterator iter = other.begin(); iter.valid(); ++iter)
      if (!contains(*iter))
        return false;

    return true;
  }

  // Convert this to a vector, more convenient and slightly faster than using
  // std::vector<T*>(set.begin(), set.end()).
  std::vector<T*> ToVector() const {
    std::vector<T*> result(this->size());
    auto it_result = result.begin();
    for (auto it = this->begin(); it.valid(); ++it)
      *it_result++ = *it;
    return result;
  }

 private:
  // Lookup node matching a given pointer. If result->valid() is true
  // then the pointer was found, otherwise, this is the location of
  // the best place to insert it.
  NodeType* Lookup(T* ptr) const {
    size_t hash = NodeType::MakeHash(ptr);
    return NodeLookup(hash, [&](NodeType* node) { return node->ptr_ == ptr; });
  }
};

#endif  // SRC_GN_POINTER_SET_H_
