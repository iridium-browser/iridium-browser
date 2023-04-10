// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_BASE_SPAN_H_
#define PLATFORM_BASE_SPAN_H_

#include <stddef.h>
#include <stdint.h>

#include <cassert>
#include <type_traits>
#include <vector>

namespace openscreen {

template <typename T>
class Span;

// In Open Screen code, use these aliases for the most common types of Spans.
// These can be converted to use std::span once the library supports C++20.
using ByteView = Span<const uint8_t>;
using ByteBuffer = Span<uint8_t>;

namespace internal {

template <typename From, typename To>
using EnableIfConvertible = std::enable_if_t<
    std::is_convertible<From (*)[], To (*)[]>::value>;  // NOLINT

}  // namespace internal

// Contains a pointer and length to a span of contiguous data.
//
// The API is a slimmed-down version of a C++20 std::span<T> and is intended to
// be forwards-compatible with very slight modifications.  We don't intend to
// add support for static extents.
//
// NOTES:
// - Although other span implementations allow passing zero to last(), we do
//   not, as the behavior is undefined.  Callers should explicitly create an
//   empty Span instead.
//
// - operator== is not implemented to align with std::span.  For more
//   discussion, this blog post has considerations when implementing operators
//   on types that don't own the data they depend upon:
//   https://abseil.io/blog/20180531-regular-types
//
// - Unit tests that want to compare the bytes behind two ByteViews can use
//   ExpectByteViewsHaveSameBytes().
template <typename T>
class Span {
 public:
  constexpr Span() noexcept = default;
  constexpr Span(const Span&) noexcept = default;
  Span(Span&& other) noexcept = default;
  constexpr Span& operator=(const Span&) noexcept = default;
  Span& operator=(Span&& other) noexcept = default;

  constexpr Span(T* data, size_t count) : data_(data), count_(count) {}

  template <typename U, typename = internal::EnableIfConvertible<U, T>>
  Span(std::vector<U>& v) : data_(v.data()), count_(v.size()) {}  // NOLINT

  template <typename U, typename = internal::EnableIfConvertible<U, T>>
  constexpr Span(const Span<U>& other) noexcept
      : data_(other.data()), count_(other.size()) {}  // NOLINT

  template <typename U, typename = internal::EnableIfConvertible<U, T>>
  constexpr Span& operator=(const Span<U>& other) noexcept {
    data_ = other.data();
    count_ = other.size();
    return *this;
  }

  ~Span() = default;

  constexpr T* data() const noexcept { return data_; }

  constexpr T& operator[](size_t idx) const {
    assert(idx < count_);
    return *(data_ + idx);
  }

  constexpr size_t size() const { return count_; }

  [[nodiscard]] constexpr bool empty() const { return count_ == 0; }

  constexpr Span first(size_t count) const {
    assert(count <= count_);
    return Span(data_, count);
  }

  constexpr Span last(size_t count) const {
    assert(count <= count_);
    assert(count != 0);
    return Span(data_ + (count_ - count), count);
  }

  constexpr T* begin() const noexcept { return data_; }
  constexpr T* end() const noexcept { return data_ + count_; }
  constexpr const T* cbegin() const noexcept { return data_; }
  constexpr const T* cend() const noexcept { return data_ + count_; }

  void remove_prefix(size_t count) noexcept {
    assert(count_ >= count);
    data_ += count;
    count_ -= count;
  }

  void remove_suffix(size_t count) noexcept {
    assert(count_ >= count);
    count_ -= count;
  }

  constexpr Span subspan(size_t offset, size_t count) const {
    assert(offset + count < count_);
    return Span(data_ + offset, count);
  }

 private:
  T* data_{nullptr};
  size_t count_{0};
};

}  // namespace openscreen

#endif  // PLATFORM_BASE_SPAN_H_
