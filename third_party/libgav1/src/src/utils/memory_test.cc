// Copyright 2021 The libgav1 Authors
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

#include "src/utils/memory.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>

#include "absl/base/config.h"
#include "gtest/gtest.h"

#ifdef ABSL_HAVE_EXCEPTIONS
#include <exception>
#endif

namespace libgav1 {
namespace {

constexpr size_t kMaxAllocableSize = 0x40000000;

struct Small : public Allocable {
  uint8_t x;
};

struct Huge : public Allocable {
  uint8_t x[kMaxAllocableSize + 1];
};

struct SmallMaxAligned : public MaxAlignedAllocable {
  alignas(kMaxAlignment) uint8_t x;
};

struct HugeMaxAligned : public MaxAlignedAllocable {
  alignas(kMaxAlignment) uint8_t x[kMaxAllocableSize + 1];
};

#ifdef ABSL_HAVE_EXCEPTIONS
struct ThrowingConstructor : public Allocable {
  ThrowingConstructor() { throw std::exception(); }

  uint8_t x;
};

struct MaxAlignedThrowingConstructor : public MaxAlignedAllocable {
  MaxAlignedThrowingConstructor() { throw std::exception(); }

  uint8_t x;
};
#endif

TEST(MemoryTest, TestAlignedAllocFree) {
  for (size_t alignment = 1; alignment <= 1 << 20; alignment <<= 1) {
    void* p = AlignedAlloc(alignment, 1);
    // Note this additional check is to avoid an incorrect static-analysis
    // warning for leaked memory with a plain ASSERT_NE().
    if (p == nullptr) {
      FAIL() << "AlignedAlloc(" << alignment << ", 1)";
    }
    const auto p_value = reinterpret_cast<uintptr_t>(p);
    EXPECT_EQ(p_value % alignment, 0)
        << "AlignedAlloc(" << alignment << ", 1) = " << p;
    AlignedFree(p);
  }
}

TEST(MemoryTest, TestAlignedUniquePtrAlloc) {
  for (size_t alignment = 1; alignment <= 1 << 20; alignment <<= 1) {
    auto p = MakeAlignedUniquePtr<uint8_t>(alignment, 1);
    ASSERT_NE(p, nullptr) << "MakeAlignedUniquePtr(" << alignment << ", 1)";
    const auto p_value = reinterpret_cast<uintptr_t>(p.get());
    EXPECT_EQ(p_value % alignment, 0)
        << "MakeAlignedUniquePtr(" << alignment << ", 1) = " << p.get();
  }
}

TEST(MemoryTest, TestAllocable) {
  // Allocable::operator new (std::nothrow) is called.
  std::unique_ptr<Small> small(new (std::nothrow) Small);
  EXPECT_NE(small, nullptr);
  // Allocable::operator delete is called.
  small = nullptr;

  // Allocable::operator new[] (std::nothrow) is called.
  std::unique_ptr<Small[]> small_array_of_smalls(new (std::nothrow) Small[10]);
  EXPECT_NE(small_array_of_smalls, nullptr);
  // Allocable::operator delete[] is called.
  small_array_of_smalls = nullptr;

  // Allocable::operator new (std::nothrow) is called.
  std::unique_ptr<Huge> huge(new (std::nothrow) Huge);
  EXPECT_EQ(huge, nullptr);

  // Allocable::operator new[] (std::nothrow) is called.
  std::unique_ptr<Small[]> huge_array_of_smalls(
      new (std::nothrow) Small[kMaxAllocableSize / sizeof(Small) + 1]);
  EXPECT_EQ(huge_array_of_smalls, nullptr);

#ifdef ABSL_HAVE_EXCEPTIONS
  try {
    // Allocable::operator new (std::nothrow) is called.
    // The constructor throws an exception.
    // Allocable::operator delete (std::nothrow) is called.
    ThrowingConstructor* always = new (std::nothrow) ThrowingConstructor;
    static_cast<void>(always);
  } catch (...) {
  }

  try {
    // Allocable::operator new[] (std::nothrow) is called.
    // The constructor throws an exception.
    // Allocable::operator delete[] (std::nothrow) is called.
    ThrowingConstructor* always = new (std::nothrow) ThrowingConstructor[2];
    static_cast<void>(always);
  } catch (...) {
  }
#endif  // ABSL_HAVE_EXCEPTIONS
}

TEST(MemoryTest, TestMaxAlignedAllocable) {
  // MaxAlignedAllocable::operator new (std::nothrow) is called.
  std::unique_ptr<SmallMaxAligned> small(new (std::nothrow) SmallMaxAligned);
  EXPECT_NE(small, nullptr);
  // Note this check doesn't guarantee conformance as a suitably aligned
  // address may be returned from any allocator.
  EXPECT_EQ(reinterpret_cast<uintptr_t>(small.get()) & (kMaxAlignment - 1), 0);
  // MaxAlignedAllocable::operator delete is called.
  small = nullptr;

  // MaxAlignedAllocable::operator new[] (std::nothrow) is called.
  std::unique_ptr<SmallMaxAligned[]> small_array_of_smalls(
      new (std::nothrow) SmallMaxAligned[10]);
  EXPECT_NE(small_array_of_smalls, nullptr);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(small_array_of_smalls.get()) &
                (kMaxAlignment - 1),
            0);
  // MaxAlignedAllocable::operator delete[] is called.
  small_array_of_smalls = nullptr;

  // MaxAlignedAllocable::operator new (std::nothrow) is called.
  std::unique_ptr<HugeMaxAligned> huge(new (std::nothrow) HugeMaxAligned);
  EXPECT_EQ(huge, nullptr);

  // MaxAlignedAllocable::operator new[] (std::nothrow) is called.
  std::unique_ptr<SmallMaxAligned[]> huge_array_of_smalls(
      new (std::nothrow)
          SmallMaxAligned[kMaxAllocableSize / sizeof(SmallMaxAligned) + 1]);
  EXPECT_EQ(huge_array_of_smalls, nullptr);

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
    // MaxAlignedAllocable::operator new[] (std::nothrow) is called.
    // The constructor throws an exception.
    // MaxAlignedAllocable::operator delete[] (std::nothrow) is called.
    auto* always = new (std::nothrow) MaxAlignedThrowingConstructor[2];
    static_cast<void>(always);
  } catch (...) {
  }
#endif  // ABSL_HAVE_EXCEPTIONS
}

}  // namespace
}  // namespace libgav1
