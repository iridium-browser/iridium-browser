// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/weak_ptr.h"

#include "gtest/gtest.h"

namespace openscreen {
namespace {

class SomeClass {
 public:
  virtual ~SomeClass() = default;
  virtual int GetValue() const { return 42; }
};

struct SomeSubclass final : public SomeClass {
 public:
  ~SomeSubclass() final = default;
  int GetValue() const override { return 999; }
};

TEST(WeakPtrTest, InteractsWithNullptr) {
  WeakPtr<const int> default_constructed;  // Invoke default constructor.
  EXPECT_TRUE(default_constructed == nullptr);
  EXPECT_TRUE(nullptr == default_constructed);
  EXPECT_FALSE(default_constructed != nullptr);
  EXPECT_FALSE(nullptr != default_constructed);

  WeakPtr<const int> null_constructed = nullptr;  // Invoke construct-from-null.
  EXPECT_TRUE(null_constructed == nullptr);
  EXPECT_TRUE(nullptr == null_constructed);
  EXPECT_FALSE(null_constructed != nullptr);
  EXPECT_FALSE(nullptr != null_constructed);

  const int foo = 42;
  WeakPtrFactory<const int> factory(&foo);
  WeakPtr<const int> not_null = factory.GetWeakPtr();
  EXPECT_TRUE(not_null != nullptr);
  EXPECT_TRUE(nullptr != not_null);
  EXPECT_FALSE(not_null == nullptr);
  EXPECT_FALSE(nullptr == not_null);
}

TEST(WeakPtrTest, CopyConstructsAndAssigns) {
  SomeSubclass foo;
  WeakPtrFactory<SomeSubclass> factory(&foo);

  WeakPtr<SomeSubclass> weak_ptr = factory.GetWeakPtr();
  EXPECT_TRUE(weak_ptr);
  EXPECT_EQ(&foo, weak_ptr.get());

  // Normal copy constructor.
  WeakPtr<SomeSubclass> copied0 = weak_ptr;
  EXPECT_EQ(&foo, weak_ptr.get());  // Did not mutate original.
  EXPECT_TRUE(copied0);
  EXPECT_EQ(&foo, copied0.get());

  // Copy constructor, adding const qualifier.
  WeakPtr<const SomeSubclass> copied1 = weak_ptr;
  EXPECT_EQ(&foo, weak_ptr.get());  // Did not mutate original.
  EXPECT_TRUE(copied1);
  EXPECT_EQ(&foo, copied1.get());

  // Normal copy assignment.
  WeakPtr<SomeSubclass> assigned0;
  EXPECT_FALSE(assigned0);
  assigned0 = copied0;
  EXPECT_EQ(&foo, copied0.get());  // Did not mutate original.
  EXPECT_TRUE(assigned0);
  EXPECT_EQ(&foo, assigned0.get());

  // Copy assignment, adding const qualifier.
  WeakPtr<const SomeSubclass> assigned1;
  EXPECT_FALSE(assigned1);
  assigned1 = copied0;
  EXPECT_EQ(&foo, copied0.get());  // Did not mutate original.
  EXPECT_TRUE(assigned1);
  EXPECT_EQ(&foo, assigned1.get());

  // Upcast copy constructor.
  WeakPtr<SomeClass> copied2 = weak_ptr;
  EXPECT_EQ(&foo, weak_ptr.get());  // Did not mutate original.
  EXPECT_TRUE(copied2);
  EXPECT_EQ(&foo, copied2.get());
  EXPECT_EQ(999, (*copied2).GetValue());
  EXPECT_EQ(999, copied2->GetValue());

  // Upcast copy assignment.
  WeakPtr<SomeClass> assigned2;
  EXPECT_FALSE(assigned2);
  assigned2 = weak_ptr;
  EXPECT_EQ(&foo, weak_ptr.get());  // Did not mutate original.
  EXPECT_TRUE(assigned2);
  EXPECT_EQ(&foo, assigned2.get());
  EXPECT_EQ(999, (*assigned2).GetValue());
  EXPECT_EQ(999, assigned2->GetValue());
}

TEST(WeakPtrTest, MoveConstructsAndAssigns) {
  SomeSubclass foo;
  WeakPtrFactory<SomeSubclass> factory(&foo);

  // Normal move constructor.
  WeakPtr<SomeSubclass> weak_ptr = factory.GetWeakPtr();
  WeakPtr<SomeSubclass> moved0 = std::move(weak_ptr);
  EXPECT_FALSE(weak_ptr);  // Original becomes null.
  EXPECT_TRUE(moved0);
  EXPECT_EQ(&foo, moved0.get());

  // Move constructor, adding const qualifier.
  weak_ptr = factory.GetWeakPtr();
  WeakPtr<const SomeSubclass> moved1 = std::move(weak_ptr);
  EXPECT_FALSE(weak_ptr);  // Original becomes null.
  EXPECT_TRUE(moved1);
  EXPECT_EQ(&foo, moved1.get());

  // Normal move assignment.
  weak_ptr = factory.GetWeakPtr();
  WeakPtr<SomeSubclass> assigned0;
  EXPECT_FALSE(assigned0);
  assigned0 = std::move(weak_ptr);
  EXPECT_FALSE(weak_ptr);  // Original becomes null.
  EXPECT_TRUE(assigned0);
  EXPECT_EQ(&foo, assigned0.get());

  // Move assignment, adding const qualifier.
  weak_ptr = factory.GetWeakPtr();
  WeakPtr<const SomeSubclass> assigned1;
  EXPECT_FALSE(assigned1);
  assigned1 = std::move(weak_ptr);
  EXPECT_FALSE(weak_ptr);  // Original becomes null.
  EXPECT_TRUE(assigned1);
  EXPECT_EQ(&foo, assigned1.get());

  // Upcast move constructor.
  weak_ptr = factory.GetWeakPtr();
  WeakPtr<SomeClass> moved2 = std::move(weak_ptr);
  EXPECT_FALSE(weak_ptr);  // Original becomes null.
  EXPECT_TRUE(moved2);
  EXPECT_EQ(&foo, moved2.get());
  EXPECT_EQ(999, (*moved2).GetValue());  // Result from subclass's GetValue().
  EXPECT_EQ(999, moved2->GetValue());    // Result from subclass's GetValue().

  // Upcast move assignment.
  weak_ptr = factory.GetWeakPtr();
  WeakPtr<SomeClass> assigned2;
  EXPECT_FALSE(assigned2);
  assigned2 = std::move(weak_ptr);
  EXPECT_FALSE(weak_ptr);  // Original becomes null.
  EXPECT_TRUE(assigned2);
  EXPECT_EQ(&foo, assigned2.get());
  EXPECT_EQ(999,
            (*assigned2).GetValue());     // Result from subclass's GetValue().
  EXPECT_EQ(999, assigned2->GetValue());  // Result from subclass's GetValue().
}

TEST(WeakPtrTest, InvalidatesWeakPtrs) {
  const int foo = 1337;
  WeakPtrFactory<const int> factory(&foo);

  // Thrice: Create weak pointers and invalidate them. This is done more than
  // once to confirm the factory can create valid WeakPtrs again after each
  // InvalidateWeakPtrs() call.
  for (int i = 0; i < 3; ++i) {
    // Create three WeakPtrs, two from the factory, one as a copy of another
    // WeakPtr.
    WeakPtr<const int> ptr0 = factory.GetWeakPtr();
    WeakPtr<const int> ptr1 = factory.GetWeakPtr();
    WeakPtr<const int> ptr2 = ptr1;
    EXPECT_EQ(&foo, ptr0.get());
    EXPECT_EQ(&foo, ptr1.get());
    EXPECT_EQ(&foo, ptr2.get());

    // Invalidate all outstanding WeakPtrs from the factory, and confirm all
    // outstanding WeakPtrs become null.
    factory.InvalidateWeakPtrs();
    EXPECT_FALSE(ptr0);
    EXPECT_FALSE(ptr1);
    EXPECT_FALSE(ptr2);
  }
}

}  // namespace
}  // namespace openscreen
