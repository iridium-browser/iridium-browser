// Copyright (c) 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_HASH_TABLE_BASE_H_
#define TOOLS_GN_HASH_TABLE_BASE_H_

#include "base/compiler_specific.h"

#include <stdlib.h>
#include <type_traits>
#include <utility>

// IMPORTANT DISCLAIMER:
//
// THIS IS *NOT* A GENERAL PURPOSE HASH TABLE TEMPLATE. INSTEAD, IT CAN
// CAN BE USED AS THE BASIS FOR VERY HIGH SPEED AND COMPACT HASH TABLES
// THAT OBEY VERY STRICT CONDITIONS DESCRIBED BELOW.
//
// DO NOT USE THIS UNLESS YOU HAVE A GOOD REASON, I.E. THAT PROFILING
// SHOWS THERE *IS* AN OVERALL BENEFIT TO DO SO. FOR MOST CASES,
// std::set<>, std::unordered_set<> and base::flat_set<> ARE PERFECTLY
// FINE AND SHOULD BE PREFERRED.
//
//
// That being said, this implementation uses a completely typical
// open-addressing scheme with a buckets array size which is always
// a power of 2, instead of a prime number. Experience shows this is
// not detrimental to performance as long as you have a sufficiently
// good hash function (which is the case of all C++ standard libraries
// these days for strings and pointers).
//
// The reason it is so fast is due to its compactness and the very
// simple but tight code for a typical lookup / insert / deletion
// operation.
//
// The |buckets_| field holds a pointer to an array of NODE_TYPE
// instances, called nodes. Each node represents one of the following:
// a free entry in the table, an inserted item, or a tombstone marking
// the location of a previously deleted item. Tombstones are only
// used if the hash table instantiation requires deletion support
// (see the is_tombstone() description below).
//
// The template requires that Node be a type with the following traits:
//
//  - It *must* be a trivial type, that is zero-initialized.
//
//  - It provides an is_null() const method, which should return true
//    if the corresponding node matches a 'free' entry in the hash
//    table, i.e. one that has not been assigned to an item, or
//    to a deletion tombstone (see below).
//
//    Of course, a default (zeroed) value, should always return true.
//
//  - It provides an is_tombstone() const method, which should return
//    return true if the corresponding node indicates a previously
//    deleted item.
//
//    Note that if your hash table does not need deletion support,
//    it is highly recommended to make this a static constexpr method
//    that always return false. Doing so will optimize the lookup loop
//    automatically!
//
//  - It must provide an is_valid() method, that simply returns
//    (!is_null() && !is_tombstone()). This is a convenience for this
//    template, but also for the derived class that will extend it
//    (more on this below, in the usage description).
//
// Note that because Node instances are trivial, std::unique_ptr<>
// cannot be used in them. Item lifecycle must this be managed
// explicitly by a derived class of the template instantiation
// instead.
//
// Lookup, insertion and deletion are performed in ways that
// are *very* different from standard containers, and the reason
// is, unsurprisingly, performance.
//
// Use NodeLookup() to look for an existing item in the hash table.
// This takes the item's hash value, and a templated callable to
// compare a node against the search key. This scheme allows
// heterogeneous lookups, and keeps the node type details
// out of this header. Any recent C++ optimizer will generate
// very tight machine code for this call.
//
// The NodeLookup() function always returns a non-null and
// mutable |node| pointer. If |node->is_valid()| is true,
// then the item was found, and |node| points to it.
//
// Otherwise, |node| corresponds to a location that may be
// used for insertion. To do so, the caller should update the
// content of |node| appropriately (e.g. writing a pointer and/or
// hash value to it), then call UpdateAfterInsertion(), which
// may eventually grow the table and rehash nodes in it.
//
// To delete an item, call NodeLookup() first to verify that
// the item is present, then write a tombstone value to |node|,
// then call UpdateAfterDeletion().
//
// Note that what the tombstone value is doesn't matter to this
// header, as long as |node->is_tombstone()| returns true for
// it.
//
// Here's an example of a concrete implementation that stores
// a hash value and an owning pointer in each node:
//
//     struct MyFooNode {
//       size_t hash;
//       Foo*   foo;
//
//       bool is_null() const { return !foo; }
//       bool is_tombstone() const { return foo == &kTombstone; }
//       bool is_valid() const { return !is_null() && !is_tombstone(); }
//
//       static const Foo kTombstone;
//     };
//
//    class MyFooSet : public HashTableBase<MyFoodNode> {
//    public:
//      using BaseType = HashTableBase<MyFooNode>;
//      using Node = BaseType::Node;
//
//      ~MyFooSet() {
//        // Destroy all items in the set.
//        // Note that the iterator only parses over valid items.
//        for (Node* node : *this) {
//          delete node->foo;
//        }
//      }
//
//      // Returns true if this set contains |key|.
//      bool contains(const Foo& key) const {
//        Node* node = BaseType::NodeLookup(
//            MakeHash(key),
//            [&](const Node* node) { return node->foo == key; });
//
//        return node->is_valid();
//      }
//
//      // Try to add |key| to the set. Return true if the insertion
//      // was successful, or false if the item was already in the set.
//      bool add(const Foo& key) {
//        size_t hash = MakeHash(key);
//        Node* node = NodeLookup(
//            hash,
//            [&](const Node* node) { return node->foo == key; });
//
//        if (node->is_valid()) {
//          // Already in the set.
//          return false;
//        }
//
//        // Add it.
//        node->hash = hash;
//        node->foo  = new Foo(key);
//        UpdateAfterInsert();
//        return true;
//      }
//
//      // Try to remove |key| from the set. Return true if the
//      // item was already in it, false otherwise.
//      bool erase(const Foo& key) {
//        Node* node = BaseType::NodeLookup(
//            MakeHash(key),
//            [&](const Node* node) { return node->foo == key; });
//
//        if (!node->is_valid()) {
//          // Not in the set.
//          return false;
//        }
//
//        delete node->foo;
//        node->foo = Node::kTombstone;
//        UpdateAfterDeletion().
//        return true;
//      }
//
//      static size_t MakeHash(const Foo& foo) {
//        return std::hash<Foo>()();
//      }
//    };
//
// For more concrete examples, see the implementation of
// StringAtom or UniqueVector<>
//
template <typename NODE_TYPE>
class HashTableBase {
 public:
  using Node = NODE_TYPE;

  static_assert(std::is_trivial<NODE_TYPE>::value,
                "KEY_TYPE should be a trivial type!");

  static_assert(std::is_standard_layout<NODE_TYPE>::value,
                "KEY_TYPE should be a standard layout type!");

  // Default constructor.
  HashTableBase() = default;

  // Destructor. This only deals with the |buckets_| array itself.
  ~HashTableBase() {
    if (buckets_ != buckets0_)
      free(buckets_);
  }

  // Copy and move operations. These only operate on the |buckets_| array
  // so any owned pointer inside nodes should be handled by custom
  // constructors and operators in the derived class, if needed.
  HashTableBase(const HashTableBase& other)
      : count_(other.count_), size_(other.size_) {
    if (other.buckets_ != other.buckets0_) {
      // NOTE: using malloc() here to clarify that no object construction
      // should occur here.
      buckets_ = reinterpret_cast<Node*>(malloc(other.size_ * sizeof(Node)));
    }
    memcpy(buckets_, other.buckets_, other.size_ * sizeof(Node));
  }

  HashTableBase& operator=(const HashTableBase& other) {
    if (this != &other) {
      this->~HashTableBase();
      new (this) HashTableBase(other);
    }
    return *this;
  }

  HashTableBase(HashTableBase&& other) noexcept
      : count_(other.count_), size_(other.size_), buckets_(other.buckets_) {
    if (buckets_ == other.buckets0_) {
      buckets0_[0] = other.buckets0_[0];
      buckets_ = buckets0_;
    } else {
      other.buckets_ = other.buckets0_;
    }
    other.NodeClear();
  }

  HashTableBase& operator=(HashTableBase&& other) noexcept {
    if (this != &other) {
      this->~HashTableBase();
      new (this) HashTableBase(std::move(other));
    }
    return *this;
  }

  // Return true if the table is empty.
  bool empty() const { return count_ == 0; }

  // Return the number of keys in the set.
  size_t size() const { return count_; }

 protected:
  // The following should only be called by derived classes that
  // extend this template class, and are not available to their
  // clients. This forces the derived class to implement lookup
  // insertion and deletion with sane APIs instead.

  // Iteration support note:
  //
  // Derived classes, if they wish to support iteration, should provide their
  // own iterator/const_iterator/begin()/end() types and methods, possibly as
  // simple wrappers around the NodeIterator/NodeBegin/NodeEnd below.
  //
  // Defining a custom iterator is as easy as deriving from NodeIterator
  // and overloading operator*() and operator->(), as in:
  //
  //  struct FooNode {
  //     size_t hash;
  //     Foo*   foo_ptr;
  //     ...
  //  };
  //
  //  class FooTable : public HashTableBase<FooNode> {
  //  public:
  //     ...
  //
  //     // Iterators point to Foo instances, not table nodes.
  //     struct ConstIterator : NodeIterator {
  //       const Foo* operator->() { return node_->foo_ptr; }
  //       const Foo& operator*)) { return *(node_->foo_ptr); }
  //     };
  //
  //     ConstIterator begin() const { return { NodeBegin() }; }
  //     ConstIterator end() const { return { NodeEnd() }; }
  //
  // The NodeIterator type has a valid() method that can be used to perform
  // faster iteration though at the cost of using a slightly more annoying
  // syntax:
  //
  //    for (auto it = my_table.begin(); it.valid(); ++it) {
  //      const Foo& foo = *it;
  //      ...
  //    }
  //
  // Instead of:
  //
  //    for (const Foo& foo : my_table) {
  //      ...
  //    }
  //
  // The ValidNodesRange() method also returns a object that has begin() and
  // end() methods to be used in for-range loops over Node values as in:
  //
  //    for (Node& node : my_table.ValidNodesRange()) { ... }
  //
  struct NodeIterator {
    Node& operator*() { return *node_; }
    const Node& operator*() const { return *node_; }

    Node* operator->() { return node_; }
    const Node* operator->() const { return node_; }

    bool operator==(const NodeIterator& other) const {
      return node_ == other.node_;
    }

    bool operator!=(const NodeIterator& other) const {
      return node_ != other.node_;
    }

    // pre-increment
    NodeIterator& operator++() {
      node_++;
      while (node_ < node_limit_ && !node_->is_valid())
        node_++;

      return *this;
    }

    // post-increment
    NodeIterator operator++(int) {
      NodeIterator result = *this;
      ++(*this);
      return result;
    }

    // Returns true if the iterator points to a valid node.
    bool valid() const { return node_ != node_limit_; }

    Node* node_ = nullptr;
    Node* node_limit_ = nullptr;
  };

  // NOTE: This is intentionally not public to avoid exposing
  // them in derived classes by mistake. If a derived class
  // wants to support iteration, it provide its own begin() and end()
  // methods, possibly using NodeBegin() and NodeEnd() below to
  // implement them.
  NodeIterator begin() { return NodeBegin(); }
  NodeIterator end() { return NodeEnd(); }

  // Providing methods named NodeBegin() and NodeEnd(), instead of
  // just using begin() and end() is a convenience to derived classes
  // that need to provide their own begin() and end() method, e.g.
  // consider this class:
  //
  //  struct FooNode {
  //     size_t hash;
  //     Foo*   foo_ptr;
  //     ...
  //  };
  //
  //  class FooTable : public HashTableBase<FooNode> {
  //  public:
  //     ...
  //
  //     // Iterators point to Foo instances, not table nodes.
  //     struct ConstIterator : NodeIterator {
  //       const Foo* operator->() { return node_->foo_ptr; }
  //       const Foo& operator*)) { return *(node_->foo_ptr); }
  //     };
  //
  // and compare two ways to implement its begin() method:
  //
  //    Foo::ConstIterator Foo::begin() const {
  //      return {
  //        reinterpret_cast<const HashTableBase<FooNode> *>(this)->begin()
  //      };
  //    };
  //
  // with:
  //
  //    Foo::ConstIterator Foo::begin() const {
  //      return { NodeBegin(); }
  //    }
  //
  NodeIterator NodeBegin() const {
    Node* node = buckets_;
    Node* limit = node + size_;
    while (node < limit && !node->is_valid())
      node++;

    return {node, limit};
  }

  NodeIterator NodeEnd() const {
    Node* limit = buckets_ + size_;
    return {limit, limit};
  }

  // ValidNodeRange() allows a derived-class to use range-based  loops
  // over valid nodes, even if they have defined their own begin() and
  // end() methods, e.g. following the same class design as in the
  // above comment:
  //
  //   FooTable::~FooTable() {
  //     for (const FooNode& node : ValidNodesRange()) {
  //       delete node->foo_ptr;
  //     }
  //   }
  //
  // which is a little bit clearer than the (valid) alternative:
  //
  //   FooTable::~FooTable() {
  //     for (Foo& foo : *this) {
  //       delete &foo;  // WHAT!?
  //     }
  //   }
  //
  struct NodeIteratorPair {
    NodeIterator begin() { return begin_; }
    NodeIterator end() { return end_; }

    NodeIterator begin_;
    NodeIterator end_;
  };

  NodeIteratorPair ValidNodesRange() const { return {NodeBegin(), NodeEnd()}; }

  // Clear the nodes table completely.
  void NodeClear() {
    if (buckets_ != buckets0_)
      free(buckets_);

    count_ = 0;
    size_ = 1;
    buckets_ = buckets0_;
    buckets0_[0] = Node{};
  }

  // Lookup for a node in the hash table that matches the |node_equal|
  // predicate, which takes a const Node* pointer argument, and returns
  // true if this corresponds to a lookup match.
  //
  // IMPORTANT: |node_equal| may or may not check the node hash value,
  // the choice is left to the implementation.
  //
  // Returns a non-null *mutable* node pointer. |node->is_valid()| will
  // be true for matches, and false for misses.
  //
  // NOTE: In case of a miss, this will return the location of the first
  // tombstone encountered during probing, if any, or the first free entry
  // otherwise. This keeps the table consistent in case the node is overwritten
  // by the caller in a following insert operation.
  template <typename NODE_EQUAL>
  Node* NodeLookup(size_t hash, NODE_EQUAL node_equal) const {
    size_t mask = size_ - 1;
    size_t index = hash & mask;
    Node* tombstone = nullptr;  // First tombstone node found, if any.
    for (;;) {
      Node* node = buckets_ + index;
      if (node->is_null()) {
        return tombstone ? tombstone : node;
      }
      if (node->is_tombstone()) {
        if (!tombstone)
          tombstone = node;
      } else if (node_equal(node)) {
        return node;
      }
      index = (index + 1) & mask;
    }
  }

  // Call this method after updating the content of the |node| pointer
  // returned by an unsuccessful NodeLookup(). Return true to indicate that
  // the table size changed, and that existing iterators were invalidated.
  bool UpdateAfterInsert() {
    count_ += 1;
    if (UNLIKELY(count_ * 4 >= size_ * 3)) {
      GrowBuckets();
      return true;
    }
    return false;
  }

  // Call this method after updating the content of the |node| value
  // returned a by successful NodeLookup, to the tombstone value, if any.
  // Return true to indicate a table size change, ie. that existing
  // iterators where invalidated.
  bool UpdateAfterRemoval() {
    count_ -= 1;
    // For now don't support shrinking since this is not useful for GN.
    return false;
  }

 private:
#if defined(__GNUC__) || defined(__clang__)
  [[gnu::noinline]]
#endif
  void
  GrowBuckets() {
    size_t size = size_;
    size_t new_size = (size == 1) ? 8 : size * 2;
    size_t new_mask = new_size - 1;

    // NOTE: Using calloc() since no object constructiopn can or should take
    // place here.
    Node* new_buckets = reinterpret_cast<Node*>(calloc(new_size, sizeof(Node)));

    for (size_t src_index = 0; src_index < size; ++src_index) {
      const Node* node = &buckets_[src_index];
      if (!node->is_valid())
        continue;
      size_t dst_index = node->hash_value() & new_mask;
      for (;;) {
        Node* node2 = new_buckets + dst_index;
        if (node2->is_null()) {
          *node2 = *node;
          break;
        }
        dst_index = (dst_index + 1) & new_mask;
      }
    }

    if (buckets_ != buckets0_)
      free(buckets_);

    buckets_ = new_buckets;
    buckets0_[0] = Node{};
    size_ = new_size;
  }

  // NOTE: The reason for default-initializing |buckets_| to a storage slot
  // inside the object is to ensure the value is never null. This removes one
  // nullptr check from each NodeLookup() instantiation.
  size_t count_ = 0;
  size_t size_ = 1;
  Node* buckets_ = buckets0_;
  Node buckets0_[1] = {{}};
};

#endif  // TOOLS_GN_HASH_TABLE_BASE_H_
