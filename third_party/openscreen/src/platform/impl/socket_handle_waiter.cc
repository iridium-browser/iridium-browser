// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/socket_handle_waiter.h"

#include <algorithm>
#include <atomic>

#include "absl/algorithm/container.h"
#include "util/osp_logging.h"
#include "util/std_util.h"

namespace openscreen {

SocketHandleWaiter::SocketHandleWaiter(ClockNowFunctionPtr now_function)
    : now_function_(now_function) {}

void SocketHandleWaiter::Subscribe(Subscriber* subscriber,
                                   SocketHandleRef handle) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (handle_mappings_.find(handle) == handle_mappings_.end()) {
    handle_mappings_.emplace(handle, SocketSubscription{subscriber});
  }
}

void SocketHandleWaiter::Unsubscribe(Subscriber* subscriber,
                                     SocketHandleRef handle) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto iterator = handle_mappings_.find(handle);
  if (handle_mappings_.find(handle) != handle_mappings_.end()) {
    handle_mappings_.erase(iterator);
  }
}

void SocketHandleWaiter::UnsubscribeAll(Subscriber* subscriber) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto it = handle_mappings_.begin(); it != handle_mappings_.end();) {
    if (it->second.subscriber == subscriber) {
      it = handle_mappings_.erase(it);
    } else {
      it++;
    }
  }
}

void SocketHandleWaiter::OnHandleDeletion(Subscriber* subscriber,
                                          SocketHandleRef handle,
                                          bool disable_locking_for_testing) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto it = handle_mappings_.find(handle);
  if (it != handle_mappings_.end()) {
    handle_mappings_.erase(it);
    if (!disable_locking_for_testing) {
      handles_being_deleted_.push_back(handle);

      OSP_DVLOG << "Starting to block for handle deletion";
      // This code will allow us to block completion of the socket destructor
      // (and subsequent invalidation of pointers to this socket) until we no
      // longer are waiting on a SELECT(...) call to it, since we only signal
      // this condition variable's wait(...) to proceed outside of SELECT(...).
      handle_deletion_block_.wait(lock, [this, handle]() {
        return !Contains(handles_being_deleted_, handle);
      });
      OSP_DVLOG << "\tDone blocking for handle deletion!";
    }
  }
}

void SocketHandleWaiter::ProcessReadyHandles(
    std::vector<HandleWithSubscription>* handles,
    Clock::duration timeout) {
  if (handles->empty()) {
    return;
  }

  Clock::time_point start_time = now_function_();
  // Process the stalest handles one by one until we hit our timeout.
  do {
    Clock::time_point oldest_time = Clock::time_point::max();
    HandleWithSubscription& oldest_handle = handles->at(0);
    for (HandleWithSubscription& handle : *handles) {
      // Skip already processed handles.
      if (handle.subscription->last_updated >= start_time) {
        continue;
      }

      // Select the oldest handle.
      if (handle.subscription->last_updated < oldest_time) {
        oldest_time = handle.subscription->last_updated;
        oldest_handle = handle;
      }
    }

    // Already processed all handles.
    if (oldest_time == Clock::time_point::max()) {
      return;
    }

    // Process the oldest handle.
    oldest_handle.subscription->last_updated = now_function_();
    oldest_handle.subscription->subscriber->ProcessReadyHandle(
        oldest_handle.ready_handle.handle, oldest_handle.ready_handle.flags);
  } while (now_function_() - start_time <= timeout);
}

Error SocketHandleWaiter::ProcessHandles(Clock::duration timeout) {
  Clock::time_point start_time = now_function_();
  std::vector<SocketHandleRef> handles;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    handles_being_deleted_.clear();
    handle_deletion_block_.notify_all();
    handles.reserve(handle_mappings_.size());
    for (const auto& pair : handle_mappings_) {
      handles.push_back(pair.first);
    }
  }

  Clock::time_point current_time = now_function_();
  Clock::duration remaining_timeout = timeout - (current_time - start_time);
  ErrorOr<std::vector<ReadyHandle>> changed_handles =
      AwaitSocketsReadable(handles, remaining_timeout);

  std::vector<HandleWithSubscription> ready_handles;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    handles_being_deleted_.clear();
    handle_deletion_block_.notify_all();
    if (changed_handles) {
      auto& ch = changed_handles.value();
      ready_handles.reserve(ch.size());
      for (const auto& handle : ch) {
        auto mapping_it = handle_mappings_.find(handle.handle);
        if (mapping_it != handle_mappings_.end()) {
          ready_handles.push_back(
              HandleWithSubscription{handle, &(mapping_it->second)});
        }
      }
    }

    if (changed_handles.is_error()) {
      return changed_handles.error();
    }

    current_time = now_function_();
    remaining_timeout = timeout - (current_time - start_time);
    ProcessReadyHandles(&ready_handles, remaining_timeout);
  }
  return Error::None();
}

}  // namespace openscreen
