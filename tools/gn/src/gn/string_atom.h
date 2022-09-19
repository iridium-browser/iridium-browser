// Copyright (c) 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_STRING_ATOM_H_
#define TOOLS_GN_STRING_ATOM_H_

#include <functional>
#include <string>
#include <string_view>

// A StringAtom models a pointer to a globally unique constant string.
//
// They are useful as key types for sets and map container types, especially
// when a program uses multiple instances that tend to use the same strings
// (as happen very frequently in GN).
//
// Note that default equality and comparison functions will compare the
// string content, not the pointers, ensuring that the behaviour of
// standard containers using StringAtom key types is the same as if
// std::string was used.
//
// In addition, _ordered_ containers support heterogeneous lookups (i.e.
// using an std::string_view, and by automatic conversion, a const char*
// of const char[] literal) as a key type.
//
// Additionally, it is also possible to implement very fast _unordered_
// containers by using the StringAtom::Fast{Hash,Equal,Compare} structs,
// which will force containers to hash/compare pointer values instead,
// for example:
//
//     // A fast unordered set of unique strings.
//     //
//     // Implementation uses a hash table so performance will be bounded
//     // by the string hash function. Does not support heterogeneous lookups.
//     //
//     using FastStringSet = std::unordered_set<StringAtom,
//                                              StringAtom::PtrHash,
//                                              StringAtom::PtrEqual>;
//
//     // A fast unordered set of unique strings.
//     //
//     // Implementation uses a balanced binary tree so performance will
//     // be bounded by string comparisons. Does support heterogeneous lookups,
//     // but not that this does not extend to the [] operator, only to the
//     // find() method.
//     //
//     using FastStringSet = std::set<StringAtom, StringAtom::PtrCompare>
//
//     // A fast unordered { string -> VALUE } map.
//     //
//     // Implementation uses a balanced binary tree. Supports heterogeneous
//     // lookups.
//     template <typename VALUE>
//     using FastStringMap = std::map<StringAtom, VALUE, StringAtom::PtrCompare>
//
class StringAtom {
 public:
  // Default constructor. Value points to a globally unique empty string.
  StringAtom();

  // Destructor should do nothing at all.
  ~StringAtom() = default;

  // Non-explicit constructors.
  StringAtom(std::string_view str) noexcept;

  // Copy and move operations.
  StringAtom(const StringAtom& other) noexcept : value_(other.value_) {}
  StringAtom& operator=(const StringAtom& other) noexcept {
    if (this != &other) {
      this->~StringAtom();  // really a no-op
      new (this) StringAtom(other);
    }
    return *this;
  }

  StringAtom(StringAtom&& other) noexcept : value_(other.value_) {}
  StringAtom& operator=(const StringAtom&& other) noexcept {
    if (this != &other) {
      this->~StringAtom();  // really a no-op
      new (this) StringAtom(std::move(other));
    }
    return *this;
  }

  bool empty() const { return value_.empty(); }

  // Explicit conversions.
  const std::string& str() const { return value_; }

  // Implicit conversions.
  operator std::string_view() const { return {value_}; }

  // Returns true iff this is the same key.
  // Note that the default comparison functions compare the value instead
  // in order to use them in standard containers without surprises by
  // default.
  bool SameAs(const StringAtom& other) const {
    return &value_ == &other.value_;
  }

  // Default comparison functions.
  bool operator==(const StringAtom& other) const {
    return value_ == other.value_;
  }

  bool operator!=(const StringAtom& other) const {
    return value_ != other.value_;
  }

  bool operator<(const StringAtom& other) const {
    // Avoid one un-necessary string comparison if values are equal.
    if (SameAs(other))
      return false;

    return value_ < other.value_;
  }

  size_t hash() const { return std::hash<std::string>()(value_); }

  // Use the following method and structs to implement containers that
  // use StringAtom values as keys, but only compare/hash the pointer
  // values for speed.
  //
  // E.g.:
  //    using FastSet = std::unordered_set<StringAtom, PtrHash, PtrEqual>;
  //    using FastMap = std::map<StringAtom, Value, PtrCompare>;
  //
  // IMPORTANT: Note that FastMap above is ordered based in the StringAtom
  //            pointer value, not the string content.
  //
  size_t ptr_hash() const { return std::hash<const std::string*>()(&value_); }

  struct PtrHash {
    size_t operator()(const StringAtom& key) const noexcept {
      return key.ptr_hash();
    }
  };

  struct PtrEqual {
    bool operator()(const StringAtom& a, const StringAtom& b) const noexcept {
      return &a.value_ == &b.value_;
    }
  };

  struct PtrCompare {
    bool operator()(const StringAtom& a, const StringAtom& b) const noexcept {
      return &a.value_ < &b.value_;
    }
  };

 protected:
  const std::string& value_;
};

namespace std {

// Ensure default heterogeneous lookups with other types like std::string_view.
template <>
struct less<StringAtom> {
  using is_transparent = int;

  bool operator()(const StringAtom& a, const StringAtom& b) const noexcept {
    return a.str() < b.str();
  }
  template <typename U>
  bool operator()(const StringAtom& a, const U& b) const noexcept {
    return a.str() < b;
  }
  template <typename U>
  bool operator()(const U& a, const StringAtom& b) const noexcept {
    return a < b.str();
  }
};

template <>
struct hash<StringAtom> {
  size_t operator()(const StringAtom& key) const noexcept { return key.hash(); }
};

}  // namespace std

#endif  // TOOLS_GN_STRING_ATOM_H_
