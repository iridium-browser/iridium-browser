// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_VECTOR_UTILS_H_
#define TOOLS_GN_VECTOR_UTILS_H_

#include <algorithm>
#include <utility>
#include <vector>

// A VectorSetSorter is a convenience class used to efficiently sort and
// de-duplicate one or more sets of items of type T, then iterate over the
// result, or get it as a simple vector. Usage is the following:
//
// For performance reasons, this implementation only stores pointers to the
// input items in order to minimize memory usage. Callers should ensure the
// items added to this sorter do not change until the instance is destroyed.
//
//    1) Create instance, passing an optional initial capacity.
//
//    2) Add items using one of the Add() methods, as many times as
//       necessary. Note that this records only pointers to said items
//       so their content should not change until the instance is destroyed.
//
//    3) Call IteratorOver() to iterate over all sorted and de-duplicated
//       items.
//
//    4) Alternatively, call AsVector() to return a new vector that contains
//       copies of the original sorted / deduplicated items.
//
template <typename T>
class VectorSetSorter {
 public:
  // Constructor. |initial_capacity| might be provided to minimize the number
  // of allocations performed by this instance, if the maximum number of
  // input items is known in advance.
  VectorSetSorter(size_t initial_capacity = 0) {
    ptrs_.reserve(initial_capacity);
  }

  // Add one single item to the sorter.
  void Add(const T& item) {
    ptrs_.push_back(&item);
    sorted_ = false;
  }

  // Add one range of items to the sorter.
  void Add(typename std::vector<T>::const_iterator begin,
           typename std::vector<T>::const_iterator end) {
    for (; begin != end; ++begin)
      ptrs_.push_back(&(*begin));
    sorted_ = false;
  }

  // Add one range of items to the sorter.
  void Add(const T* start, const T* end) {
    for (; start != end; ++start)
      ptrs_.push_back(start);
    sorted_ = false;
  }

  // Iterate over all sorted items, removing duplicates from the loop.
  // |item_callback| is a callable that will be invoked for each item in the
  // result.
  template <typename ITEM_CALLBACK>
  void IterateOver(ITEM_CALLBACK item_callback) {
    if (!sorted_) {
      Sort();
    }
    const T* prev_item = nullptr;
    for (const T* item : ptrs_) {
      if (!prev_item || *prev_item != *item) {
        item_callback(*item);
        prev_item = item;
      }
    }
  }

  // Return the sorted and de-duplicated resulting set as a vector of items.
  // Note that this copies the input items.
  std::vector<T> AsVector() {
    std::vector<T> result;
    result.reserve(ptrs_.size());
    IterateOver([&result](const T& item) { result.push_back(item); });
    return result;
  }

 private:
  // Sort all items previously added to this instance.
  // Must be called after adding all desired items, and before
  // calling IterateOver() or AsVector().
  void Sort() {
    std::sort(ptrs_.begin(), ptrs_.end(),
              [](const T* a, const T* b) { return *a < *b; });
    sorted_ = true;
  }

  std::vector<const T*> ptrs_;
  bool sorted_ = false;
};

#endif  // TOOLS_GN_VECTOR_UTILS_H_
