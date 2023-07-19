// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_RESOLVED_TARGET_DEPS_H_
#define TOOLS_GN_RESOLVED_TARGET_DEPS_H_

#include "base/containers/span.h"
#include "gn/label_ptr.h"

#include <memory>

// A class used to record the dependencies of a given Target in
// a way that is much more efficient to iterate over than having three
// separate LabelTargetVector instances. Technically equivalent to
// DepsIterator, but profiling shows that this class is much faster
// to use during graph-traversal heavy operations.
//
// Usage is:
//   1) Create instance, passing const references to the LabelTargetVector
//      instances for the private, public and data deps for the target.
//
//   2) Use private_deps(), public_deps(), data_deps(), linked_deps()
//      and all_deps() to retrieve spans that cover various subsets of
//      interests. These can be used directly in for-range loops as in:
//
//       for (const Target* target : resolved.linked_deps()) {
//         ..
//       }
//
class ResolvedTargetDeps {
 public:
  ResolvedTargetDeps() = default;

  ResolvedTargetDeps(const LabelTargetVector& public_deps,
                     const LabelTargetVector& private_deps,
                     const LabelTargetVector& data_deps)
      : public_count_(static_cast<uint32_t>(public_deps.size())),
        private_count_(static_cast<uint32_t>(private_deps.size())),
        data_count_(static_cast<uint32_t>(data_deps.size())),
        deps_(Allocate(public_deps, private_deps, data_deps)) {}

  size_t size() const { return private_count_ + public_count_ + data_count_; }

  base::span<const Target*> public_deps() const {
    return {deps_.get(), public_count_};
  }

  base::span<const Target*> private_deps() const {
    return {deps_.get() + public_count_, private_count_};
  }

  base::span<const Target*> data_deps() const {
    return {deps_.get() + private_count_ + public_count_, data_count_};
  }

  base::span<const Target*> linked_deps() const {
    return {deps_.get(), private_count_ + public_count_};
  }

  base::span<const Target*> all_deps() const {
    return {deps_.get(), private_count_ + public_count_ + data_count_};
  }

  static std::unique_ptr<const Target*[]> Allocate(
      const LabelTargetVector& public_deps,
      const LabelTargetVector& private_deps,
      const LabelTargetVector& data_deps) {
    size_t total_size =
        private_deps.size() + public_deps.size() + data_deps.size();
    auto result = std::make_unique<const Target*[]>(total_size);
    const Target** ptr = result.get();
    for (const auto& pair : public_deps)
      *ptr++ = pair.ptr;
    for (const auto& pair : private_deps)
      *ptr++ = pair.ptr;
    for (const auto& pair : data_deps)
      *ptr++ = pair.ptr;
    return result;
  }

 private:
  uint32_t public_count_ = 0;
  uint32_t private_count_ = 0;
  uint32_t data_count_ = 0;
  // Store the pointers in the following order: public, private, data.
  std::unique_ptr<const Target*[]> deps_;
};

#endif  // TOOLS_GN_RESOLVED_TARGET_DEPS_H_
