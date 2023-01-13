// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_BASE_BYTE_VIEW_H_
#define PLATFORM_BASE_BYTE_VIEW_H_

#include <stddef.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "platform/base/macros.h"

namespace openscreen {

// Contains a pointer and length to a span of continguous and unowned bytes.
// The underlying data cannot be modified.
//
// The API is a slimmed-down version of a C++20 std::span<const uint8_t> and is
// intended to be forwards-compatible.  Support for iterators and front/back can
// be added as needed; we don't intend to add support for static extents.
//
// NOTE: Although other span implementations allow passing zero to last(), we do
// not, as the behavior is undefined.  Callers should explicitly create an empty
// ByteView instead.
//
// TODO(https://issuetracker.google.com/254502854): Replace assert() expressions
// with OSP_CHECK* macros once those are available to files in platform/.
class ByteView {
 public:
  constexpr ByteView() noexcept = default;
  constexpr ByteView(const uint8_t* data, size_t count)
      : data_(data), count_(count) {}
  explicit ByteView(const std::vector<uint8_t>& v)
      : data_(v.data()), count_(v.size()) {}

  constexpr ByteView(const ByteView&) noexcept = default;
  constexpr ByteView& operator=(const ByteView&) noexcept = default;
  ByteView(ByteView&&) noexcept = default;
  ByteView& operator=(ByteView&&) noexcept = default;
  ~ByteView() = default;

  constexpr const uint8_t* data() const noexcept { return data_; }

  constexpr const uint8_t& operator[](size_t idx) const {
    assert(idx < count_);
    return *(data_ + idx);
  }

  constexpr size_t size() const { return count_; }

  [[nodiscard]] constexpr bool empty() const { return count_ == 0; }

  constexpr ByteView first(size_t count) const {
    assert(count <= count_);
    return ByteView(data_, count);
  }

  constexpr ByteView last(size_t count) const {
    assert(count <= count_);
    assert(count != 0);
    return ByteView(data_ + (count_ - count), count);
  }

  constexpr const uint8_t* begin() const noexcept { return data_; }
  constexpr const uint8_t* end() const noexcept { return data_ + count_; }

  void remove_prefix(size_t count) noexcept {
    assert(count_ >= count);
    data_ += count;
    count_ -= count;
  }

  void remove_suffix(size_t count) noexcept {
    assert(count_ >= count);
    count_ -= count;
  }

  constexpr ByteView subspan(size_t offset, size_t count) const {
    assert(offset + count < count_);
    return ByteView(data_ + offset, count);
  }

 private:
  const uint8_t* data_{nullptr};
  size_t count_{0};
};

}  // namespace openscreen

#endif  // PLATFORM_BASE_BYTE_VIEW_H_
