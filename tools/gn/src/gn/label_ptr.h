// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_LABEL_PTR_H_
#define TOOLS_GN_LABEL_PTR_H_

#include <stddef.h>

#include <functional>

#include "gn/label.h"

class Config;
class ParseNode;
class Target;

// Structure that holds a labeled "thing". This is used for various places
// where we need to store lists of targets or configs. We sometimes populate
// the pointers on another thread from where we compute the labels, so this
// structure lets us save them separately. This also allows us to store the
// location of the thing that added this dependency.
template <typename T>
struct LabelPtrPair {
  using DestType = T;

  LabelPtrPair() = default;

  explicit LabelPtrPair(const Label& l) : label(l) {}

  // This constructor is typically used in unit tests, it extracts the label
  // automatically from a given pointer.
  explicit LabelPtrPair(const T* p) : label(p->label()), ptr(p) {}

  ~LabelPtrPair() = default;

  Label label;
  const T* ptr = nullptr;

  // The origin of this dependency. This will be null for internally generated
  // dependencies. This happens when a group is automatically expanded and that
  // group's members are added to the target that depends on that group.
  const ParseNode* origin = nullptr;
};

using LabelConfigPair = LabelPtrPair<Config>;
using LabelTargetPair = LabelPtrPair<Target>;

using LabelConfigVector = std::vector<LabelConfigPair>;
using LabelTargetVector = std::vector<LabelTargetPair>;

// Default comparison operators -----------------------------------------------
//
// The default hash and comparison operators operate on the label, which should
// always be valid, whereas the pointer is sometimes null.

template <typename T>
inline bool operator==(const LabelPtrPair<T>& a, const LabelPtrPair<T>& b) {
  return a.label == b.label;
}

template <typename T>
inline bool operator<(const LabelPtrPair<T>& a, const LabelPtrPair<T>& b) {
  return a.label < b.label;
}

namespace std {

template <typename T>
struct hash<LabelPtrPair<T>> {
  std::size_t operator()(const LabelPtrPair<T>& v) const {
    hash<Label> h;
    return h(v.label);
  }
};

}  // namespace std

#endif  // TOOLS_GN_LABEL_PTR_H_
