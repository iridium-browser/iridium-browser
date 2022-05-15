// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/worker_pool.h"

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "gn/switches.h"
#include "util/build_config.h"
#include "util/sys_info.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

namespace {

#if defined(OS_WIN)
class ProcessorGroupSetter {
 public:
  void SetProcessorGroup(std::thread* thread);

 private:
  int group_ = 0;
  GROUP_AFFINITY group_affinity_;
  int num_available_cores_in_group_ = ::GetActiveProcessorCount(group_) / 2;
  const int num_groups_ = ::GetActiveProcessorGroupCount();
};

void ProcessorGroupSetter::SetProcessorGroup(std::thread* thread) {
  if (num_groups_ <= 1)
    return;

  const HANDLE thread_handle = HANDLE(thread->native_handle());
  ::GetThreadGroupAffinity(thread_handle, &group_affinity_);
  group_affinity_.Group = group_;
  const bool success =
      ::SetThreadGroupAffinity(thread_handle, &group_affinity_, nullptr);
  DCHECK(success);

  // Move to next group once one thread has been assigned per core in |group_|.
  num_available_cores_in_group_--;
  if (num_available_cores_in_group_ <= 0) {
    group_++;
    if (group_ >= num_groups_) {
      group_ = 0;
    }
    num_available_cores_in_group_ = ::GetActiveProcessorCount(group_) / 2;
  }
}
#endif

int GetThreadCount() {
  std::string thread_count =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kThreads);

  // See if an override was specified on the command line.
  int result;
  if (!thread_count.empty() && base::StringToInt(thread_count, &result) &&
      result >= 1) {
    return result;
  }

  // Base the default number of worker threads on number of cores in the
  // system. When building large projects, the speed can be limited by how fast
  // the main thread can dispatch work and connect the dependency graph. If
  // there are too many worker threads, the main thread can be starved and it
  // will run slower overall.
  //
  // One less worker thread than the number of physical CPUs seems to be a
  // good value, both theoretically and experimentally. But always use at
  // least some workers to prevent us from being too sensitive to I/O latency
  // on low-end systems.
  //
  // The minimum thread count is based on measuring the optimal threads for the
  // Chrome build on a several-year-old 4-core MacBook.
  // Almost all CPUs now are hyperthreaded.
  int num_cores = NumberOfProcessors() / 2;
  return std::max(num_cores - 1, 8);
}

}  // namespace

WorkerPool::WorkerPool() : WorkerPool(GetThreadCount()) {}

WorkerPool::WorkerPool(size_t thread_count) : should_stop_processing_(false) {
#if defined(OS_WIN)
  ProcessorGroupSetter processor_group_setter;
#endif

  threads_.reserve(thread_count);
  for (size_t i = 0; i < thread_count; ++i) {
    threads_.emplace_back([this]() { Worker(); });

#if defined(OS_WIN)
    // Set thread processor group. This is needed for systems with more than 64
    // logical processors, wherein available processors are divided into groups,
    // and applications that need to use more than one group's processors must
    // manually assign their threads to groups.
    processor_group_setter.SetProcessorGroup(&threads_.back());
#endif
  }
}

WorkerPool::~WorkerPool() {
  {
    std::unique_lock<std::mutex> queue_lock(queue_mutex_);
    should_stop_processing_ = true;
  }

  pool_notifier_.notify_all();

  for (auto& task_thread : threads_) {
    task_thread.join();
  }
}

void WorkerPool::PostTask(std::function<void()> work) {
  {
    std::unique_lock<std::mutex> queue_lock(queue_mutex_);
    CHECK(!should_stop_processing_);
    task_queue_.emplace(std::move(work));
  }

  pool_notifier_.notify_one();
}

void WorkerPool::Worker() {
  for (;;) {
    std::function<void()> task;

    {
      std::unique_lock<std::mutex> queue_lock(queue_mutex_);

      pool_notifier_.wait(queue_lock, [this]() {
        return (!task_queue_.empty()) || should_stop_processing_;
      });

      if (should_stop_processing_ && task_queue_.empty())
        return;

      task = std::move(task_queue_.front());
      task_queue_.pop();
    }

    task();
  }
}
