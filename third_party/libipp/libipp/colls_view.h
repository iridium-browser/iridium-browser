// Copyright 2023 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBIPP_COLLS_VIEW_H_
#define LIBIPP_COLLS_VIEW_H_

#include <iterator>
#include <vector>

#include "ipp_export.h"

namespace ipp {

class Attribute;
class Collection;
class ConstCollsView;
class Frame;

// This class represents a range of Collections inside Frame or Attribute. It
// provides const and non-const access to underlying Collections with
// bidirectional iterators, operator[], as well as, methods size() and empty().
//
class LIBIPP_EXPORT CollsView {
 public:
  class const_iterator;
  class iterator {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = Collection;
    using difference_type = int;
    using pointer = Collection*;
    using reference = Collection&;

    iterator() = default;
    iterator(const iterator&) = default;
    iterator& operator=(const iterator&) = default;
    iterator& operator++() {
      ++iter_;
      return *this;
    }
    iterator& operator--() {
      --iter_;
      return *this;
    }
    iterator operator++(int) { return iterator(iter_++); }
    iterator operator--(int) { return iterator(iter_--); }
    Collection& operator*() { return **iter_; }
    Collection* operator->() { return *iter_; }
    bool operator==(const iterator& i) const { return iter_ == i.iter_; }
    bool operator!=(const iterator& i) const { return iter_ != i.iter_; }
    bool operator==(const const_iterator& i) const { return iter_ == i.iter_; }
    bool operator!=(const const_iterator& i) const { return iter_ != i.iter_; }

   private:
    friend class CollsView;
    friend class Frame;
    explicit iterator(std::vector<Collection*>::iterator iter) : iter_(iter) {}
    std::vector<Collection*>::iterator iter_;
  };

  class const_iterator {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = const Collection;
    using difference_type = int;
    using pointer = const Collection*;
    using reference = const Collection&;

    const_iterator() = default;
    const_iterator(const const_iterator&) = default;
    const_iterator& operator=(const const_iterator&) = default;
    explicit const_iterator(iterator it) : iter_(it.iter_) {}
    const_iterator& operator=(iterator it) {
      iter_ = it.iter_;
      return *this;
    }
    const_iterator& operator++() {
      ++iter_;
      return *this;
    }
    const_iterator& operator--() {
      --iter_;
      return *this;
    }
    const_iterator operator++(int) { return const_iterator(iter_++); }
    const_iterator operator--(int) { return const_iterator(iter_--); }
    const Collection& operator*() { return **iter_; }
    const Collection* operator->() { return *iter_; }
    bool operator==(const iterator& i) const { return iter_ == i.iter_; }
    bool operator!=(const iterator& i) const { return iter_ != i.iter_; }
    bool operator==(const const_iterator& i) const { return iter_ == i.iter_; }
    bool operator!=(const const_iterator& i) const { return iter_ != i.iter_; }

   private:
    friend class CollsView;
    friend class ConstCollsView;
    friend class Frame;
    explicit const_iterator(std::vector<Collection*>::const_iterator iter)
        : iter_(iter) {}
    std::vector<Collection*>::const_iterator iter_;
  };

  // Default constructor returns always empty range.
  CollsView();
  CollsView(const CollsView& cv) = default;
  CollsView& operator=(const CollsView& cv) {
    colls_ = cv.colls_;
    return *this;
  }
  iterator begin() { return iterator(colls_->begin()); }
  iterator end() { return iterator(colls_->end()); }
  const_iterator cbegin() const { return const_iterator(colls_->cbegin()); }
  const_iterator cend() const { return const_iterator(colls_->cend()); }
  const_iterator begin() const { return cbegin(); }
  const_iterator end() const { return cend(); }
  size_t size() const { return colls_->size(); }
  bool empty() const { return colls_->empty(); }
  Collection& operator[](size_t index) { return *(*colls_)[index]; }
  const Collection& operator[](size_t index) const { return *(*colls_)[index]; }

 private:
  friend class Attribute;
  friend class ConstCollsView;
  friend class Frame;
  explicit CollsView(std::vector<Collection*>& colls) : colls_(&colls) {}
  std::vector<Collection*>* colls_;
};

// Const version of CollsView.
class LIBIPP_EXPORT ConstCollsView {
 public:
  using const_iterator = CollsView::const_iterator;

  // Default constructor returns always empty range.
  ConstCollsView();
  ConstCollsView(const ConstCollsView& cv) = default;
  explicit ConstCollsView(const CollsView& cv) : colls_(cv.colls_) {}
  ConstCollsView& operator=(const ConstCollsView& cv) {
    colls_ = cv.colls_;
    return *this;
  }
  ConstCollsView& operator=(const CollsView& cv) {
    colls_ = cv.colls_;
    return *this;
  }
  const_iterator cbegin() const { return const_iterator(colls_->cbegin()); }
  const_iterator cend() const { return const_iterator(colls_->cend()); }
  const_iterator begin() const { return cbegin(); }
  const_iterator end() const { return cend(); }
  size_t size() const { return colls_->size(); }
  bool empty() const { return colls_->empty(); }
  const Collection& operator[](size_t index) const { return *(*colls_)[index]; }

 private:
  friend class Attribute;
  friend class Frame;
  explicit ConstCollsView(const std::vector<Collection*>& colls)
      : colls_(&colls) {}
  const std::vector<Collection*>* colls_;
};

}  // namespace ipp

#endif  //  LIBIPP_COLLS_VIEW_H_
