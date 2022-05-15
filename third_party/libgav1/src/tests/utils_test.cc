// Copyright 2020 The libgav1 Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "tests/utils.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>

#include "absl/base/config.h"
#include "gtest/gtest.h"
#include "src/utils/memory.h"

#ifdef ABSL_HAVE_EXCEPTIONS
#include <exception>
#endif

namespace libgav1 {
namespace test_utils {
namespace {

constexpr size_t kMaxAllocableSize = 0x40000000;

// Has a trivial default constructor that performs no action.
struct SmallMaxAligned : public MaxAlignedAllocable {
  alignas(kMaxAlignment) uint8_t x;
};

// Has a nontrivial default constructor that initializes the data member.
struct SmallMaxAlignedNontrivialConstructor : public MaxAlignedAllocable {
  alignas(kMaxAlignment) uint8_t x = 0;
};

// Has a trivial default constructor that performs no action.
struct HugeMaxAligned : public MaxAlignedAllocable {
  alignas(kMaxAlignment) uint8_t x[kMaxAllocableSize + 1];
};

// Has a nontrivial default constructor that initializes the data member.
struct HugeMaxAlignedNontrivialConstructor : public MaxAlignedAllocable {
  alignas(kMaxAlignment) uint8_t x[kMaxAllocableSize + 1] = {};
};

#ifdef ABSL_HAVE_EXCEPTIONS
struct MaxAlignedThrowingConstructor : public MaxAlignedAllocable {
  MaxAlignedThrowingConstructor() { throw std::exception(); }

  uint8_t x;
};
#endif

TEST(TestUtilsTest, TestMaxAlignedAllocable) {
  {
    // MaxAlignedAllocable::operator new (std::nothrow) is called.
    std::unique_ptr<SmallMaxAligned> small(new (std::nothrow) SmallMaxAligned);
    EXPECT_NE(small, nullptr);
    // Note this check doesn't guarantee conformance as a suitably aligned
    // address may be returned from any allocator.
    EXPECT_EQ(reinterpret_cast<uintptr_t>(small.get()) & (kMaxAlignment - 1),
              0);
    // MaxAlignedAllocable::operator delete is called.
  }

  {
    // MaxAlignedAllocable::operator new is called.
    std::unique_ptr<SmallMaxAligned> small(new SmallMaxAligned);
    EXPECT_NE(small, nullptr);
    // Note this check doesn't guarantee conformance as a suitably aligned
    // address may be returned from any allocator.
    EXPECT_EQ(reinterpret_cast<uintptr_t>(small.get()) & (kMaxAlignment - 1),
              0);
    // MaxAlignedAllocable::operator delete is called.
  }

  {
    // MaxAlignedAllocable::operator new[] (std::nothrow) is called.
    std::unique_ptr<SmallMaxAligned[]> small_array_of_smalls(
        new (std::nothrow) SmallMaxAligned[10]);
    EXPECT_NE(small_array_of_smalls, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(small_array_of_smalls.get()) &
                  (kMaxAlignment - 1),
              0);
    // MaxAlignedAllocable::operator delete[] is called.
  }

  {
    // MaxAlignedAllocable::operator new[] is called.
    std::unique_ptr<SmallMaxAligned[]> small_array_of_smalls(
        new SmallMaxAligned[10]);
    EXPECT_NE(small_array_of_smalls, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(small_array_of_smalls.get()) &
                  (kMaxAlignment - 1),
              0);
    // MaxAlignedAllocable::operator delete[] is called.
  }

  {
    // MaxAlignedAllocable::operator new (std::nothrow) is called.
    std::unique_ptr<HugeMaxAligned> huge(new (std::nothrow) HugeMaxAligned);
    EXPECT_EQ(huge, nullptr);
  }

  {
    // MaxAlignedAllocable::operator new[] (std::nothrow) is called.
    std::unique_ptr<SmallMaxAligned[]> huge_array_of_smalls(
        new (std::nothrow)
            SmallMaxAligned[kMaxAllocableSize / sizeof(SmallMaxAligned) + 1]);
    EXPECT_EQ(huge_array_of_smalls, nullptr);
  }

#ifdef ABSL_HAVE_EXCEPTIONS
  try {
    // MaxAlignedAllocable::operator new (std::nothrow) is called.
    // The constructor throws an exception.
    // MaxAlignedAllocable::operator delete (std::nothrow) is called.
    auto* always = new (std::nothrow) MaxAlignedThrowingConstructor;
    static_cast<void>(always);
  } catch (...) {
  }

  try {
    // MaxAlignedAllocable::operator new is called.
    // The constructor throws an exception.
    // MaxAlignedAllocable::operator delete is called.
    auto* always = new MaxAlignedThrowingConstructor;
    static_cast<void>(always);
  } catch (...) {
  }

  try {
    // MaxAlignedAllocable::operator new[] (std::nothrow) is called.
    // The constructor throws an exception.
    // MaxAlignedAllocable::operator delete[] (std::nothrow) is called.
    auto* always = new (std::nothrow) MaxAlignedThrowingConstructor[2];
    static_cast<void>(always);
  } catch (...) {
  }

  try {
    // MaxAlignedAllocable::operator new[] is called.
    // The constructor throws an exception.
    // MaxAlignedAllocable::operator delete[] is called.
    auto* always = new MaxAlignedThrowingConstructor[2];
    static_cast<void>(always);
  } catch (...) {
  }

  // Note these calls are only safe with exceptions enabled as if the throwing
  // operator new returns the object is expected to be valid. In this case an
  // attempt to invoke the object's constructor on a nullptr may be made which
  // is undefined behavior.
  try {
    // MaxAlignedAllocable::operator new is called.
    std::unique_ptr<HugeMaxAlignedNontrivialConstructor> huge(
        new HugeMaxAlignedNontrivialConstructor);
    ADD_FAILURE() << "huge allocation should fail.";
  } catch (...) {
    SUCCEED();
  }

  try {
    // MaxAlignedAllocable::operator new[] is called.
    std::unique_ptr<SmallMaxAlignedNontrivialConstructor[]>
        huge_array_of_smalls(
            new SmallMaxAlignedNontrivialConstructor
                [kMaxAllocableSize /
                     sizeof(SmallMaxAlignedNontrivialConstructor) +
                 1]);
    ADD_FAILURE() << "huge_array_of_smalls allocation should fail.";
  } catch (...) {
    SUCCEED();
  }
#endif  // ABSL_HAVE_EXCEPTIONS
}

}  // namespace
}  // namespace test_utils
}  // namespace libgav1
