// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_WEAK_PTR_H_
#define UTIL_WEAK_PTR_H_

#include <memory>
#include <utility>

#include "util/osp_logging.h"

namespace openscreen {

// Weak pointers are pointers to an object that do not affect its lifetime,
// and which may be invalidated (i.e. reset to nullptr) by the object, or its
// owner, at any time; most commonly when the object is about to be deleted.
//
// Weak pointers are useful when an object needs to be accessed safely by one
// or more objects other than its owner, and those callers can cope with the
// object vanishing and e.g. tasks posted to it being silently dropped.
// Reference-counting such an object would complicate the ownership graph and
// make it harder to reason about the object's lifetime.
//
// EXAMPLE:
//
//  class Controller {
//   public:
//    void SpawnWorker() { new Worker(weak_factory_.GetWeakPtr()); }
//    void WorkComplete(const Result& result) { ... }
//   private:
//    // Member variables should appear before the WeakPtrFactory, to ensure
//    // that any WeakPtrs to Controller are invalidated before its members
//    // variable's destructors are executed, rendering them invalid.
//    WeakPtrFactory<Controller> weak_factory_{this};
//  };
//
//  class Worker {
//   public:
//    explicit Worker(WeakPtr<Controller> controller)
//        : controller_(std::move(controller)) {}
//   private:
//    void DidCompleteAsynchronousProcessing(const Result& result) {
//      if (controller_)
//        controller_->WorkComplete(result);
//      delete this;
//    }
//    const WeakPtr<Controller> controller_;
//  };
//
// With this implementation a caller may use SpawnWorker() to dispatch multiple
// Workers and subsequently delete the Controller, without waiting for all
// Workers to have completed.
//
// ------------------------- IMPORTANT: Thread-safety -------------------------
//
// Generally, Open Screen code is meant to be single-threaded. For the few
// exceptional cases, the following is relevant:
//
// WeakPtrs may be created from WeakPtrFactory, and also duplicated/moved on any
// thread/sequence. However, they may only be dereferenced on the same
// thread/sequence that will ultimately execute the WeakPtrFactory destructor or
// call InvalidateWeakPtrs(). Otherwise, use-during-free or use-after-free is
// possible.
//
// openscreen::WeakPtr and WeakPtrFactory are similar, but not identical, to
// Chromium's base::WeakPtrFactory. Open Screen WeakPtrs may be safely created
// from WeakPtrFactory on any thread/sequence, since they are backed by the
// thread-safe bookkeeping of std::shared_ptr<>.

template <typename T>
class WeakPtrFactory;

template <typename T>
class WeakPtr {
 public:
  WeakPtr() = default;
  ~WeakPtr() = default;

  // Copy/Move constructors and assignment operators.
  WeakPtr(const WeakPtr& other) : impl_(other.impl_) {}

  WeakPtr(WeakPtr&& other) noexcept : impl_(std::move(other.impl_)) {}

  WeakPtr& operator=(const WeakPtr& other) {
    impl_ = other.impl_;
    return *this;
  }

  WeakPtr& operator=(WeakPtr&& other) noexcept {
    impl_ = std::move(other.impl_);
    return *this;
  }

  // Create/Assign from nullptr.
  WeakPtr(std::nullptr_t) {}  // NOLINT

  WeakPtr& operator=(std::nullptr_t) {
    impl_.reset();
    return *this;
  }

  // Copy/Move constructors and assignment operators with upcast conversion.
  template <typename U>
  WeakPtr(const WeakPtr<U>& other) : impl_(other.as_std_weak_ptr()) {}

  template <typename U>
  WeakPtr(WeakPtr<U>&& other) noexcept
      : impl_(std::move(other).as_std_weak_ptr()) {}

  template <typename U>
  WeakPtr& operator=(const WeakPtr<U>& other) {
    impl_ = other.as_std_weak_ptr();
    return *this;
  }

  template <typename U>
  WeakPtr& operator=(WeakPtr<U>&& other) noexcept {
    impl_ = std::move(other).as_std_weak_ptr();
    return *this;
  }

  // Accessors.
  T* get() const { return impl_.lock().get(); }

  T& operator*() const {
    T* const pointer = get();
    OSP_DCHECK(pointer);
    return *pointer;
  }

  T* operator->() const {
    T* const pointer = get();
    OSP_DCHECK(pointer);
    return pointer;
  }

  // Allow conditionals to test validity, e.g. if (weak_ptr) {...}
  explicit operator bool() const { return get() != nullptr; }

  // Conversion to std::weak_ptr<T>. It is unsafe to convert in the other
  // direction. See comments for private constructors, below.
  const std::weak_ptr<T>& as_std_weak_ptr() const& { return impl_; }
  std::weak_ptr<T> as_std_weak_ptr() && { return std::move(impl_); }

 private:
  friend class WeakPtrFactory<T>;

  // Called by WeakPtrFactory<T> and the WeakPtr<T> upcast conversion
  // constructors and assigners. These are purposely not being exposed publicly
  // because that would allow a WeakPtr<T> to be valid/invalid by a different
  // ownership/threading model than the intended one (see top-level comments).
  template <typename U>
  explicit WeakPtr(const std::weak_ptr<U>& other) : impl_(other) {}

  template <typename U>
  explicit WeakPtr(std::weak_ptr<U>&& other) noexcept
      : impl_(std::move(other)) {}

  std::weak_ptr<T> impl_;
};

// Allow callers to compare WeakPtrs against nullptr to test validity.
template <typename T>
bool operator!=(const WeakPtr<T>& weak_ptr, std::nullptr_t) {
  return weak_ptr.get() != nullptr;
}
template <typename T>
bool operator!=(std::nullptr_t, const WeakPtr<T>& weak_ptr) {
  return weak_ptr.get() != nullptr;
}
template <typename T>
bool operator==(const WeakPtr<T>& weak_ptr, std::nullptr_t) {
  return weak_ptr.get() == nullptr;
}
template <typename T>
bool operator==(std::nullptr_t, const WeakPtr<T>& weak_ptr) {
  return weak_ptr == nullptr;
}

template <typename T>
class WeakPtrFactory {
 public:
  explicit WeakPtrFactory(T* instance) { Reset(instance); }
  WeakPtrFactory(WeakPtrFactory&& other) noexcept = default;
  WeakPtrFactory& operator=(WeakPtrFactory&& other) noexcept = default;

  // Thread-safe: WeakPtrs may be created on any thread/seuence. They may also
  // be copied and moved on any thread/sequence. However, they MUST only be
  // dereferenced on the same thread/sequence that calls the destructor or
  // InvalidateWeakPtrs().
  WeakPtr<T> GetWeakPtr() const {
    return WeakPtr<T>(std::weak_ptr<T>(bookkeeper_));
  }

  // Destruction and Invalidation: These must be called on the same
  // thread/sequence that dereferences any WeakPtrs to avoid use-after-free
  // bugs.
  ~WeakPtrFactory() = default;
  void InvalidateWeakPtrs() { Reset(bookkeeper_.get()); }

 private:
  WeakPtrFactory(const WeakPtrFactory& other) = delete;
  WeakPtrFactory& operator=(const WeakPtrFactory& other) = delete;

  void Reset(T* instance) {
    // T is owned externally to WeakPtrFactory. Thus, provide a no-op Deleter.
    bookkeeper_ = {instance, [](T*) {}};
  }

  // Manages the std::weak_ptr's referring to T. Does not own T.
  std::shared_ptr<T> bookkeeper_;
};

}  // namespace openscreen

#endif  // UTIL_WEAK_PTR_H_
