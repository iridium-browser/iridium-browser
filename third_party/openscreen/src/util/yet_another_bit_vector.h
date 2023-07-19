// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_YET_ANOTHER_BIT_VECTOR_H_
#define UTIL_YET_ANOTHER_BIT_VECTOR_H_

#include <stdint.h>

#include <limits>

namespace openscreen {

// This is yet another bit vector implementation. Unlike the ones found in the
// standard library, this one provides useful functionality (find first bit set,
// count number of bits set, shift right) as well as efficiency.
//
// Storage details: The vector must be explicitly sized (when constructed or
// Resize()'ed). There is no dynamic resizing via a push_back()-like operation.
// Storage is determined based on the size specified by the user: either one
// 64-bit integer stored within the class structure (for all sizes <= 64), or a
// heap-allocated array of multiple 64-bit integers.
class YetAnotherBitVector {
 public:
  enum Fill : bool { SET = true, CLEARED = false };

  // Constructs an empty bit vector.
  YetAnotherBitVector();

  // Constructs a bit vector having the given |size| and all bits set/cleared.
  YetAnotherBitVector(int size, Fill fill);

  ~YetAnotherBitVector();

  YetAnotherBitVector(YetAnotherBitVector&& other);
  YetAnotherBitVector& operator=(YetAnotherBitVector&& other);

  // Not Implemented: Conceptually, there's no reason to prohibit copying these
  // objects. Implement it if you need it. :)
  YetAnotherBitVector(const YetAnotherBitVector& other) = delete;
  YetAnotherBitVector& operator=(const YetAnotherBitVector& other) = delete;

  int size() const { return size_; }

  // Query/Set/Clear a single bit at the given |pos|.
  bool IsSet(int pos) const;
  void Set(int pos);
  void Clear(int pos);

  // Resize the bit vector and set/clear all the bits according to |fill|.
  void Resize(int new_size, Fill fill);

  // Sets/Clears all bits.
  void SetAll();
  void ClearAll();

  // Shift all bits right by some number of |steps|, zero-padding the leftmost
  // bits. |steps| must be between zero and |size()|.
  void ShiftRight(int steps);

  // Returns the position of the first bit set, or |size()| if no bits are set.
  int FindFirstSet() const;

  // Returns how many of the bits are set in the range [begin, end).
  int CountBitsSet(int begin, int end) const;

 private:
  bool using_array_storage() const { return size_ > kBitsPerInteger; }

  // Returns the number of integers required to store |size_| bits.
  int array_size() const {
    // The math here is: CEIL(size_ ÷ kBitsPerInteger).
    return (size_ + kBitsPerInteger - 1) / kBitsPerInteger;
  }

  // Helper to create array storage (only if necessary) and initialize all the
  // bits based on the given |fill|. Precondition: Any prior heap-allocated
  // array storage has already been deallocated.
  void InitializeForNewSize(int new_size, Fill fill);

  // Helper to find the integer that contains the bit at the given position, and
  // updates |pos| to the offset of the bit within the integer.
  const uint64_t* Select(int* pos) const;

  // Total number of bits.
  int size_;

  // Either store one integer's worth of bits inline, or all are stored in a
  // separate heap-allocated array. The using_array_storage() method is used to
  // determine which.
  union {
    uint64_t as_integer;
    uint64_t* as_array;
  } bits_;

  static constexpr int kBitsPerInteger = std::numeric_limits<uint64_t>::digits;
  static constexpr uint64_t kAllBitsSet = std::numeric_limits<uint64_t>::max();
  static constexpr uint64_t kNoBitsSet = 0;
};

}  // namespace openscreen

#endif  // UTIL_YET_ANOTHER_BIT_VECTOR_H_
