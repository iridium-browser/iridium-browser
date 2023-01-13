// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_WORKER_POOL_H_
#define UTIL_WORKER_POOL_H_

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

#include "base/logging.h"

class WorkerPool {
 public:
  WorkerPool();
  WorkerPool(size_t thread_count);
  ~WorkerPool();

  void PostTask(std::function<void()> work);

 private:
  void Worker();

  std::vector<std::thread> threads_;
  std::queue<std::function<void()>> task_queue_;
  std::mutex queue_mutex_;
  std::condition_variable_any pool_notifier_;
  bool should_stop_processing_;

  WorkerPool(const WorkerPool&) = delete;
  WorkerPool& operator=(const WorkerPool&) = delete;
};

#endif  // UTIL_WORKER_POOL_H_
