// Copyright (c) 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "hash_table_base.h"

#include "util/test/test.h"

#include <algorithm>
#include <vector>

// This unit-test is also used to illustrate how to use HashTableBase<>
// in a concrete way. Here, each node is a simple pointer to a Int class
// that wraps a simple integer, but also keeps tracks of
// construction/destruction steps in global counters. This is used by the
// test to verify that operations like copies or moves do not miss or
// create allocations/deallocations.
//
// Because the derived table HashTableTest owns all pointer objects, it
// needs to manually create/deallocate them in its destructor, copy/move
// constructors and operators, as well as insert()/erase()/clear() methods.
//
// Finally, iteration support is provided through const_iterator and
// begin(), end() methods, which are enough for range-based for loops.
//

// A simple int wrapper class that can also count creation/destruction.
class Int {
 public:
  explicit Int(int x) : x_(x) { creation_counter++; }

  Int(const Int& other) : x_(other.x_) { creation_counter++; }

  ~Int() { destruction_counter++; }

  int& x() { return x_; }
  const int& x() const { return x_; }

  size_t hash() const { return static_cast<size_t>(x_); }

  static void ResetCounters() {
    creation_counter = 0;
    destruction_counter = 0;
  }

  int x_;

  static size_t creation_counter;
  static size_t destruction_counter;
};

size_t Int::creation_counter;
size_t Int::destruction_counter;

// A HashTableBase<> node type that contains a simple pointer to a Int value.
struct TestHashNode {
  Int* int_ptr;

  bool is_null() const { return !int_ptr; }
  bool is_tombstone() const { return int_ptr == &kTombstone; }
  bool is_valid() const { return !is_null() && !is_tombstone(); }
  size_t hash_value() const { return int_ptr->hash(); }

  static Int kTombstone;
};

// Value is irrelevant since only its address is used.
Int TestHashNode::kTombstone(-1);

// TestHashTable derives a HashTableBase<> instantiation to demonstrate
// full uses of the template. This includes:
//
//  - Storing a pointer in each node, and managing ownership of pointed
//    objects explicitly in the destructor, copy/move constructor and
//    operator, as well as insert() and erase() methods.
//
//  - The internal pointed objects are Int instance, but the TestHashTable
//    API masks that entirely, instead implementing a simple set of integers,
//    including iteration support.
//
//  Note that placing the integers directly in the nodes would be much easier,
//  but would not allow demonstrating how to manage ownership in thedestructor
class TestHashTable : public HashTableBase<TestHashNode> {
 public:
  using BaseType = HashTableBase<TestHashNode>;
  using Node = BaseType::Node;

  static_assert(std::is_same<Node, TestHashNode>::value,
                "HashTableBase<>::Node is not defined properly!");

  BaseType& asBaseType() { return *this; }
  const BaseType& asBaseType() const { return *this; }

  TestHashTable() = default;

  // IMPORTANT NOTE: Because the table contains bare owning pointers, we
  // have to use explicit copy and move constructor/operators for things
  // to work as expected. This is yet another reason why HashTableBase<>
  // should only be used with care (preferably with non-owning pointers).
  //
  TestHashTable(const TestHashTable& other) : BaseType(other) {
    // Only node (i.e. pointers) are copied by the base type.
    for (Node& node : ValidNodesRange()) {
      node.int_ptr = new Int(*node.int_ptr);
    }
  }

  TestHashTable& operator=(const TestHashTable& other) {
    if (this != &other) {
      this->~TestHashTable();
      new (this) TestHashTable(other);
    }
    return *this;
  }

  TestHashTable(TestHashTable&& other) noexcept : BaseType(std::move(other)) {}
  TestHashTable& operator=(TestHashTable&& other) noexcept {
    if (this != &other) {
      this->~TestHashTable();
      new (this) TestHashTable(std::move(other));
    }
    return *this;
  }

  ~TestHashTable() {
    // Discard all valid Int pointers in the hash table.
    for (Node& node : ValidNodesRange())
      delete node.int_ptr;
  }

  // Return true iff the table contains |x|.
  bool contains(int x) const {
    size_t hash = static_cast<size_t>(x);
    Node* node = NodeLookup(
        hash, [&](const Node* node) { return node->int_ptr->x() == x; });
    return node->is_valid();
  }

  // Try to insert |x| in the table. Returns true on success, or false if
  // the value was already in it.
  bool insert(int x) {
    size_t hash = static_cast<size_t>(x);
    Node* node = NodeLookup(
        hash, [&](const Node* node) { return node->int_ptr->x() == x; });
    if (node->is_valid())
      return false;

    node->int_ptr = new Int(x);
    UpdateAfterInsert();
    return true;
  }

  // Try to remove |x| from the table. Return true on success, or false
  // if the value was not in it.
  bool erase(int x) {
    size_t hash = static_cast<size_t>(x);
    Node* node = NodeLookup(
        hash, [&](const Node* node) { return node->int_ptr->x() == x; });
    if (!node->is_valid())
      return false;

    delete node->int_ptr;
    node->int_ptr = &TestHashNode::kTombstone;
    UpdateAfterRemoval();
    return true;
  }

  // Remove all items
  void clear() {
    // Remove all pointed objects, since NodeClear() will not do it.
    for (Node& node : ValidNodesRange())
      delete node.int_ptr;

    NodeClear();
  }

  // Define const_iterator that point to the integer value instead of the node
  // of the Int instance to completely hide these from this class' API.
  struct const_iterator : public BaseType::NodeIterator {
    const int& operator*() const {
      return (this->BaseType::NodeIterator::operator*()).int_ptr->x();
    }
    const int* operator->() const { return &(this->operator*()); }
  };

  const_iterator begin() const { return {BaseType::NodeBegin()}; }

  const_iterator end() const { return {BaseType::NodeEnd()}; }
};

TEST(HashTableBaseTest, Construction) {
  Int::ResetCounters();
  {
    TestHashTable table;
    EXPECT_TRUE(table.empty());
    EXPECT_EQ(table.size(), 0u);
    EXPECT_EQ(table.begin(), table.end());
  }

  // No item was created or destroyed.
  EXPECT_EQ(Int::creation_counter, 0u);
  EXPECT_EQ(Int::destruction_counter, 0u);
}

TEST(HashTableBaseTest, InsertionsAndLookups) {
  Int::ResetCounters();
  {
    TestHashTable table;
    table.insert(1);
    table.insert(5);
    table.insert(7);

    EXPECT_FALSE(table.empty());
    EXPECT_EQ(table.size(), 3u);
    EXPECT_NE(table.begin(), table.end());

    EXPECT_EQ(Int::creation_counter, 3u);
    EXPECT_EQ(Int::destruction_counter, 0u);

    EXPECT_FALSE(table.contains(0));
    EXPECT_TRUE(table.contains(1));
    EXPECT_FALSE(table.contains(2));
    EXPECT_FALSE(table.contains(3));
    EXPECT_TRUE(table.contains(5));
    EXPECT_FALSE(table.contains(6));
    EXPECT_TRUE(table.contains(7));
    EXPECT_FALSE(table.contains(8));
  }

  EXPECT_EQ(Int::creation_counter, 3u);
  EXPECT_EQ(Int::destruction_counter, 3u);
}

TEST(HashTableBaseTest, CopyAssignment) {
  Int::ResetCounters();
  {
    TestHashTable table;
    table.insert(1);
    table.insert(5);
    table.insert(7);

    EXPECT_FALSE(table.empty());
    EXPECT_EQ(table.size(), 3u);

    TestHashTable table2;
    EXPECT_TRUE(table2.empty());
    table2 = table;
    EXPECT_FALSE(table2.empty());
    EXPECT_EQ(table2.size(), 3u);
    EXPECT_FALSE(table.empty());
    EXPECT_EQ(table.size(), 3u);

    EXPECT_EQ(Int::creation_counter, 6u);
    EXPECT_EQ(Int::destruction_counter, 0u);

    EXPECT_FALSE(table.contains(0));
    EXPECT_TRUE(table.contains(1));
    EXPECT_FALSE(table.contains(2));
    EXPECT_FALSE(table.contains(3));
    EXPECT_TRUE(table.contains(5));
    EXPECT_FALSE(table.contains(6));
    EXPECT_TRUE(table.contains(7));
    EXPECT_FALSE(table.contains(8));

    EXPECT_FALSE(table2.contains(0));
    EXPECT_TRUE(table2.contains(1));
    EXPECT_FALSE(table2.contains(2));
    EXPECT_FALSE(table2.contains(3));
    EXPECT_TRUE(table2.contains(5));
    EXPECT_FALSE(table2.contains(6));
    EXPECT_TRUE(table2.contains(7));
    EXPECT_FALSE(table2.contains(8));
  }

  EXPECT_EQ(Int::creation_counter, 6u);
  EXPECT_EQ(Int::destruction_counter, 6u);
}

TEST(HashTableBaseTest, MoveAssignment) {
  Int::ResetCounters();
  {
    TestHashTable table;
    table.insert(1);
    table.insert(5);
    table.insert(7);

    EXPECT_FALSE(table.empty());
    EXPECT_EQ(table.size(), 3u);

    TestHashTable table2;
    EXPECT_TRUE(table2.empty());
    table2 = std::move(table);
    EXPECT_FALSE(table2.empty());
    EXPECT_EQ(table2.size(), 3u);
    EXPECT_TRUE(table.empty());
    EXPECT_EQ(table.size(), 0u);

    EXPECT_EQ(Int::creation_counter, 3u);
    EXPECT_EQ(Int::destruction_counter, 0u);

    EXPECT_FALSE(table2.contains(0));
    EXPECT_TRUE(table2.contains(1));
    EXPECT_FALSE(table2.contains(2));
    EXPECT_FALSE(table2.contains(3));
    EXPECT_TRUE(table2.contains(5));
    EXPECT_FALSE(table2.contains(6));
    EXPECT_TRUE(table2.contains(7));
    EXPECT_FALSE(table2.contains(8));
  }

  EXPECT_EQ(Int::creation_counter, 3u);
  EXPECT_EQ(Int::destruction_counter, 3u);
}

TEST(HashTableBaseTest, Clear) {
  Int::ResetCounters();
  {
    TestHashTable table;
    table.insert(1);
    table.insert(5);
    table.insert(7);

    EXPECT_FALSE(table.empty());
    EXPECT_EQ(table.size(), 3u);

    table.clear();
    EXPECT_TRUE(table.empty());

    EXPECT_EQ(Int::creation_counter, 3u);
    EXPECT_EQ(Int::destruction_counter, 3u);
  }

  EXPECT_EQ(Int::creation_counter, 3u);
  EXPECT_EQ(Int::destruction_counter, 3u);
}

TEST(HashTableBaseTest, Iteration) {
  TestHashTable table;
  table.insert(1);
  table.insert(5);
  table.insert(7);

  EXPECT_FALSE(table.empty());
  EXPECT_EQ(table.size(), 3u);

  std::vector<int> values;

  for (const int& x : table)
    values.push_back(x);

  std::sort(values.begin(), values.end());
  EXPECT_EQ(values.size(), 3u);
  EXPECT_EQ(values[0], 1);
  EXPECT_EQ(values[1], 5);
  EXPECT_EQ(values[2], 7);
}
