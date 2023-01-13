// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_TARGET_PUBLIC_PAIR_H_
#define TOOLS_GN_TARGET_PUBLIC_PAIR_H_

#include "gn/unique_vector.h"

class Target;

// C++ and Rust target resolution requires computing uniquified and
// ordered lists of static/shared libraries that are collected through
// the target's dependency tree.
//
// Maintaining the order is important to ensure the libraries are linked
// in the correct order in the final link command line.
//
// Also each library must only appear once in the final list, even though
// it may appear multiple times during the dependency tree walk, either as
// a "private" or "public" dependency.
//
// The TargetPublicPair class below encodes a (target_ptr, is_public_flag)
// pair, with convenience accessors and utility structs.
//
// The TargetPublicPairListBuilder is a builder-pattern class that generates
// a unique vector of TargetPublicPair values (i.e. the final list described
// above), and supporting the special logic required to build these lists
// (see the comments for its Append() and AppendInherited() methods).
//
// A convenience encoding for a (target_ptr, is_public_flag) pair.
class TargetPublicPair {
 public:
  TargetPublicPair() = default;
  TargetPublicPair(const Target* target, bool is_public)
      : target_(target), is_public_(is_public) {}
  TargetPublicPair(std::pair<const Target*, bool> pair)
      : target_(pair.first), is_public_(pair.second) {}

  const Target* target() const { return target_; }
  void set_target(const Target* target) { target_ = target; }

  bool is_public() const { return is_public_; }
  void set_is_public(bool is_public) { is_public_ = is_public; }

  // Utility structs that can be used to instantiante containers
  // that only use the target for lookups / comparisons. E.g.
  //
  //   std::unordered_set<TargetPublicPair,
  //                      TargetPublicPair::TargetHash,
  //                      TargetPublicPair::TargetEqualTo>
  //
  //   std::set<TargetPublicPair, TargetPublicPair::TargetLess>
  //
  struct TargetHash {
    size_t operator()(TargetPublicPair p) const noexcept {
      return std::hash<const Target*>()(p.target());
    }
  };

  struct TargetEqualTo {
    bool operator()(TargetPublicPair a, TargetPublicPair b) const noexcept {
      return a.target() == b.target();
    }
  };

  struct TargetLess {
    bool operator()(TargetPublicPair a, TargetPublicPair b) const noexcept {
      return a.target() < b.target();
    }
  };

 private:
  const Target* target_ = nullptr;
  bool is_public_ = false;
};

// A helper type to build a uniquified ordered vector of TargetPublicPair
// instances. Usage is:
//
//   1) Create builder instance.
//
//   2) Call Append() to add a direct dependency, or AppendInherited() to add
//      transitive ones, as many times as necessary.
//
//   3) Call Build() to retrieve final list as a vector.
//
class TargetPublicPairListBuilder
    : public UniqueVector<TargetPublicPair,
                          TargetPublicPair::TargetHash,
                          TargetPublicPair::TargetEqualTo> {
 public:
  // Add (target, is_public) to the list being constructed. If the target
  // was not already in the list, record the |is_public| flag as is,
  // otherwise, set the recorded flag to true only if |is_public| is true, or
  // don't do anything otherwise.
  void Append(const Target* target, bool is_public) {
    auto ret = EmplaceBackWithIndex(target, is_public);
    if (!ret.first && is_public) {
      // UniqueVector<T>::operator[]() always returns a const reference
      // because the returned values are lookup keys in its set-like data
      // structure (thus modifying them would break its internal consistency).
      // However, because TargetHash and TargetEqualTo are being used to
      // instantiate this template, only the target() part of the value must
      // remain constant, and it is possible to modify the is_public() part
      // in-place safely.
      auto* pair = const_cast<TargetPublicPair*>(&(*this)[ret.second]);
      pair->set_is_public(true);
    }
  }

  // Append all pairs from any container with begin() and end() iterators
  // that dereference to values that convert to a TargetPublicPair value.
  // If |is_public| is false, the input pair will be appended with the
  // value of the public flag to false.
  template <
      typename C,
      typename = std::void_t<
          decltype(static_cast<TargetPublicPair>(*std::declval<C>().begin())),
          decltype(static_cast<TargetPublicPair>(*std::declval<C>().end()))>>
  void AppendInherited(const C& other, bool is_public) {
    for (const auto& pair : other) {
      Append(pair.target(), is_public && pair.is_public());
    }
  }

  // Build and return the final list to the caller.
  std::vector<TargetPublicPair> Build() { return release(); }
};

#endif  // TOOLS_GN_TARGET_PUBLIC_PAIR_H_
