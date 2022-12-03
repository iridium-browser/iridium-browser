// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/yet_another_bit_vector.h"

#include <algorithm>
#include <utility>

#include "util/osp_logging.h"

namespace openscreen {

namespace {

// Returns a bitmask where all the bits whose positions are in the range
// [begin,begin+count) are set, and all other bits are cleared.
constexpr uint64_t MakeBitmask(int begin, int count) {
  // Form a contiguous sequence of bits by subtracting one from the appropriate
  // power of 2. Set all the bits if count >= 64.
  const uint64_t bits_in_wrong_position =
      (count >= std::numeric_limits<uint64_t>::digits)
          ? std::numeric_limits<uint64_t>::max()
          : ((uint64_t{1} << count) - 1);

  // Now shift the contiguous sequence of bits into the correct position.
  return bits_in_wrong_position << begin;
}

}  // namespace

YetAnotherBitVector::YetAnotherBitVector() : size_(0), bits_{.as_integer = 0} {}

YetAnotherBitVector::YetAnotherBitVector(int size,
                                         YetAnotherBitVector::Fill fill) {
  InitializeForNewSize(size, fill);
}

YetAnotherBitVector::~YetAnotherBitVector() {
  if (using_array_storage()) {
    delete[] bits_.as_array;
  }
}

YetAnotherBitVector::YetAnotherBitVector(YetAnotherBitVector&& other)
    : size_(other.size_), bits_(other.bits_) {
  other.size_ = 0;
  other.bits_.as_integer = 0;
}

YetAnotherBitVector& YetAnotherBitVector::operator=(
    YetAnotherBitVector&& other) {
  if (this == &other) {
    return *this;
  }
  if (using_array_storage()) {
    delete[] bits_.as_array;
  }
  size_ = other.size_;
  bits_ = other.bits_;
  other.size_ = 0;
  other.bits_.as_integer = 0;
  return *this;
}

bool YetAnotherBitVector::IsSet(int pos) const {
  OSP_DCHECK_LT(pos, size_);
  const uint64_t* const elem = Select(&pos);
  return (*elem & (uint64_t{1} << pos)) != 0;
}

void YetAnotherBitVector::Set(int pos) {
  OSP_DCHECK_LT(pos, size_);
  uint64_t* const elem = const_cast<uint64_t*>(Select(&pos));
  *elem |= (uint64_t{1} << pos);
}

void YetAnotherBitVector::Clear(int pos) {
  OSP_DCHECK_LT(pos, size_);
  uint64_t* const elem = const_cast<uint64_t*>(Select(&pos));
  *elem &= ~(uint64_t{1} << pos);
}

void YetAnotherBitVector::Resize(int new_size, YetAnotherBitVector::Fill fill) {
  if (using_array_storage()) {
    delete[] bits_.as_array;
  }
  InitializeForNewSize(new_size, fill);
}

void YetAnotherBitVector::SetAll() {
  // Implementation note: Set all bits except those in the last integer that are
  // outside the defined range. This allows the other operations to become
  // simpler and more efficient because they can assume the bits moving into the
  // valid range are not set.

  if (using_array_storage()) {
    const int last_index = array_size() - 1;
    uint64_t* const last = &bits_.as_array[last_index];
    std::fill(&bits_.as_array[0], last, kAllBitsSet);
    *last = MakeBitmask(0, size_ - (last_index * kBitsPerInteger));
  } else {
    bits_.as_integer = MakeBitmask(0, size_);
  }
}

void YetAnotherBitVector::ClearAll() {
  if (using_array_storage()) {
    std::fill(&bits_.as_array[0], &bits_.as_array[array_size()], kNoBitsSet);
  } else {
    bits_.as_integer = kNoBitsSet;
  }
}

void YetAnotherBitVector::ShiftRight(int steps) {
  // Negative |steps| should probably mean "shift left," but this is not
  // implemented.
  OSP_DCHECK_GE(steps, 0);
  OSP_DCHECK_LE(steps, size_);

  if (using_array_storage()) {
    // If |steps| is greater than one integer's worth of bits, first shift the
    // array elements right. This is effectively shifting all the bits right by
    // some multiple of 64.
    const int num_integers = array_size();
    if (steps >= kBitsPerInteger) {
      const int integer_steps = steps / kBitsPerInteger;
      for (int i = integer_steps; i < num_integers; ++i) {
        bits_.as_array[i - integer_steps] = bits_.as_array[i];
      }
      std::fill(&bits_.as_array[num_integers - integer_steps],
                &bits_.as_array[num_integers], kNoBitsSet);
      steps %= kBitsPerInteger;
    }

    // With |steps| now less than 64, shift the bits right within each array
    // element. Start from the back of the array, working towards the front, and
    // propagating any bits that are moving across array elements.
    uint64_t incoming_carry_bits = 0;
    const uint64_t outgoing_mask = MakeBitmask(0, steps);
    for (int i = num_integers; i-- > 0;) {
      const uint64_t outgoing_carry_bits = bits_.as_array[i] & outgoing_mask;
      bits_.as_array[i] >>= steps;
      bits_.as_array[i] |= (incoming_carry_bits << (kBitsPerInteger - steps));
      incoming_carry_bits = outgoing_carry_bits;
    }
  } else {
    if (steps < kBitsPerInteger) {
      bits_.as_integer >>= steps;
    } else {
      bits_.as_integer = 0;
    }
  }
}

int YetAnotherBitVector::FindFirstSet() const {
  // Almost all processors provide a single instruction to "count trailing
  // zeros" in an integer, which is great because this is the same as the
  // 0-based index of the first set bit. So, have the compiler use that
  // whenever it's available. However, note that the intrinsic (and the CPU
  // instruction used) provides undefined results when operating on zero; and
  // so that special case is checked and handled.
#if defined(__clang__) || defined(__GNUC__)
#define CountTrailingZeros(bits) __builtin_ctzll(bits)
#else
  const auto CountTrailingZeros = [](uint64_t bits) -> int {
    // Based on one of the public domain "Bit Twiddling Hacks" heuristics:
    // https://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightParallel
    // clang-format off
    bits &= ~bits + 1;
    int count = kBitsPerInteger;
    if (bits) --count;
    if (bits & UINT64_C(0x00000000ffffffff)) count -= 32;
    if (bits & UINT64_C(0x0000ffff0000ffff)) count -= 16;
    if (bits & UINT64_C(0x00ff00ff00ff00ff)) count -= 8;
    if (bits & UINT64_C(0x0f0f0f0f0f0f0f0f)) count -= 4;
    if (bits & UINT64_C(0x3333333333333333)) count -= 2;
    if (bits & UINT64_C(0x5555555555555555)) count -= 1;
    return count;
    // clang-format on
  };
#endif

  if (using_array_storage()) {
    for (int i = 0, end = array_size(); i < end; ++i) {
      if (bits_.as_array[i] != 0) {
        return (i * kBitsPerInteger) + CountTrailingZeros(bits_.as_array[i]);
      }
    }
    return size_;  // All bits are not set.
  }
  return (bits_.as_integer != 0) ? CountTrailingZeros(bits_.as_integer) : size_;
}

int YetAnotherBitVector::CountBitsSet(int begin, int end) const {
  OSP_DCHECK_LE(0, begin);
  OSP_DCHECK_LE(begin, end);
  OSP_DCHECK_LE(end, size_);

  // Almost all processors provide a single instruction to "count the number of
  // bits set" in an integer. So, have the compiler use that whenever it's
  // available.
#if defined(__clang__) || defined(__GNUC__)
#define PopCount(bits) __builtin_popcountll(bits)
#else
  const auto PopCount = [](uint64_t bits) -> int {
    // Based on one of the public domain "Bit Twiddling Hacks" heuristics:
    // https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
    constexpr int kBitsPerByte = 8;
    bits = bits - ((bits >> 1) & (kAllBitsSet / 3));
    bits = (bits & (kAllBitsSet / 15 * 3)) +
           ((bits >> 2) & (kAllBitsSet / 15 * 3));
    bits = (bits + (bits >> 4)) & (kAllBitsSet / 255 * 15);
    const uint64_t count =
        (bits * (kAllBitsSet / 255)) >> ((sizeof(uint64_t) - 1) * kBitsPerByte);
    return static_cast<int>(count);
  };
#endif

  int count;
  if (using_array_storage()) {
    const int first = begin / kBitsPerInteger;
    const int last = (end - 1) / kBitsPerInteger;
    if (first == last) {
      count = PopCount(bits_.as_array[first] &
                       MakeBitmask(begin % kBitsPerInteger, end - begin));
    } else if (first < last) {
      // Count a subset of the bits in the first and last integers (according to
      // |begin| and |end|), and all of the bits in the integers in-between.
      const uint64_t* p = &bits_.as_array[first];
      count = PopCount((*p) &
                       MakeBitmask(begin % kBitsPerInteger, kBitsPerInteger));
      for (++p; p != &bits_.as_array[last]; ++p) {
        count += PopCount(*p);
      }
      count += PopCount((*p) & MakeBitmask(0, end - (last * kBitsPerInteger)));
    } else {
      count = 0;
    }
  } else {
    count = PopCount(bits_.as_integer & MakeBitmask(begin, end - begin));
  }
  return count;
}

void YetAnotherBitVector::InitializeForNewSize(int new_size, Fill fill) {
  OSP_DCHECK_GE(new_size, 0);
  size_ = new_size;
  if (using_array_storage()) {
    bits_.as_array = new uint64_t[array_size()];
  }
  if (fill == SET) {
    SetAll();
  } else {
    ClearAll();
  }
}

const uint64_t* YetAnotherBitVector::Select(int* pos) const {
  if (using_array_storage()) {
    const int index = *pos / kBitsPerInteger;
    *pos %= kBitsPerInteger;
    return &bits_.as_array[index];
  }
  return &bits_.as_integer;
}

// NOTE: These declarations can be removed when C++17 compliance is mandatory
// for all embedders, as static constexpr members can be declared inline.
constexpr int YetAnotherBitVector::kBitsPerInteger;
constexpr uint64_t YetAnotherBitVector::kAllBitsSet;
constexpr uint64_t YetAnotherBitVector::kNoBitsSet;

}  // namespace openscreen
