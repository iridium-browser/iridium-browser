/* Copyright 2019 Google LLC. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "ruy/thread_pool.h"

#include <atomic>
#include <chrono>              // NOLINT(build/c++11)
#include <condition_variable>  // NOLINT(build/c++11)
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mutex>   // NOLINT(build/c++11)
#include <thread>  // NOLINT(build/c++11)

#include "ruy/check_macros.h"
#include "ruy/denormal.h"
#include "ruy/trace.h"
#include "ruy/wait.h"

namespace ruy {

// A worker thread.
class Thread {
 public:
  explicit Thread(BlockingCounter* count_busy_threads, Duration spin_duration)
      : state_(State::Startup),
        count_busy_threads_(count_busy_threads),
        spin_duration_(spin_duration) {
    thread_.reset(new std::thread(ThreadFunc, this));
  }

  void RequestExitAsSoonAsPossible() {
    ChangeStateFromOutsideThread(State::ExitAsSoonAsPossible);
  }

  ~Thread() {
    RUY_DCHECK_EQ(state_.load(), State::ExitAsSoonAsPossible);
    thread_->join();
  }

  // Called by an outside thead to give work to the worker thread.
  void StartWork(Task* task) {
    ChangeStateFromOutsideThread(State::HasWork, task);
  }

 private:
  enum class State {
    Startup,  // The initial state before the thread loop runs.
    Ready,    // Is not working, has not yet received new work to do.
    HasWork,  // Has work to do.
    ExitAsSoonAsPossible  // Should exit at earliest convenience.
  };

  // Implements the state_ change to State::Ready, which is where we consume
  // task_. Only called on the worker thread.
  // Reads task_, so assumes ordering past any prior writes to task_.
  void RevertToReadyState() {
    RUY_TRACE_SCOPE_NAME("Worker thread task");
    // See task_ member comment for the ordering of accesses.
    if (task_) {
      task_->Run();
      task_ = nullptr;
    }
    // No need to notify state_cond_, since only the worker thread ever waits
    // on it, and we are that thread.
    // Relaxed order because ordering is already provided by the
    // count_busy_threads_->DecrementCount() at the next line, since the next
    // state_ mutation will be to give new work and that won't happen before
    // the outside thread has finished the current batch with a
    // count_busy_threads_->Wait().
    state_.store(State::Ready, std::memory_order_relaxed);
    count_busy_threads_->DecrementCount();
  }

  // Changes State, from outside thread.
  //
  // The Task argument is to be used only with new_state==HasWork.
  // It specifies the Task being handed to this Thread.
  //
  // new_task is only used with State::HasWork.
  void ChangeStateFromOutsideThread(State new_state, Task* new_task = nullptr) {
    RUY_DCHECK(new_state == State::ExitAsSoonAsPossible ||
               new_state == State::HasWork);
    RUY_DCHECK((new_task != nullptr) == (new_state == State::HasWork));

#ifndef NDEBUG
    // Debug-only sanity checks based on old_state.
    State old_state = state_.load();
    RUY_DCHECK_NE(old_state, new_state);
    RUY_DCHECK(old_state == State::Ready || old_state == State::HasWork);
    RUY_DCHECK_NE(old_state, new_state);
#endif

    switch (new_state) {
      case State::HasWork:
        // See task_ member comment for the ordering of accesses.
        RUY_DCHECK(!task_);
        task_ = new_task;
        break;
      case State::ExitAsSoonAsPossible:
        break;
      default:
        abort();
    }
    // Release order because the worker thread will read this with acquire
    // order.
    state_.store(new_state, std::memory_order_release);
    state_cond_mutex_.lock();
    state_cond_.notify_one();  // Only this one worker thread cares.
    state_cond_mutex_.unlock();
  }

  static void ThreadFunc(Thread* arg) { arg->ThreadFuncImpl(); }

  // Waits for state_ to be different from State::Ready, and returns that
  // new value.
  State GetNewStateOtherThanReady() {
    State new_state;
    const auto& new_state_not_ready = [this, &new_state]() {
      new_state = state_.load(std::memory_order_acquire);
      return new_state != State::Ready;
    };
    RUY_TRACE_INFO(THREAD_FUNC_IMPL_WAITING);
    Wait(new_state_not_ready, spin_duration_, &state_cond_, &state_cond_mutex_);
    return new_state;
  }

  // Thread entry point.
  void ThreadFuncImpl() {
    RUY_TRACE_SCOPE_NAME("Ruy worker thread function");
    RevertToReadyState();

    // Suppress denormals to avoid computation inefficiency.
    ScopedSuppressDenormals suppress_denormals;

    // Thread loop
    while (GetNewStateOtherThanReady() == State::HasWork) {
      RevertToReadyState();
    }

    // Thread end. We should only get here if we were told to exit.
    RUY_DCHECK(state_.load() == State::ExitAsSoonAsPossible);
  }

  // The underlying thread. Used to join on destruction.
  std::unique_ptr<std::thread> thread_;

  // The task to be worked on.
  //
  // The ordering of reads and writes to task_ is as follows.
  //
  // 1. The outside thread gives new work by calling
  //      ChangeStateFromOutsideThread(State::HasWork, new_task);
  //    That does:
  //    - a. Write task_ = new_task (non-atomic).
  //    - b. Store state_ = State::HasWork (memory_order_release).
  // 2. The worker thread picks up the new state by calling
  //      GetNewStateOtherThanReady()
  //    That does:
  //    - c. Load state (memory_order_acquire).
  //    The worker thread then reads the new task in RevertToReadyState().
  //    That does:
  //    - d. Read task_ (non-atomic).
  // 3. The worker thread, still in RevertToReadyState(), consumes the task_ and
  //    does:
  //    - e. Write task_ = nullptr (non-atomic).
  //    And then calls Call count_busy_threads_->DecrementCount()
  //    which does
  //    - f. Store count_busy_threads_ (memory_order_release).
  // 4. The outside thread, in ThreadPool::ExecuteImpl, finally waits for worker
  //    threads by calling count_busy_threads_->Wait(), which does:
  //    - g. Load count_busy_threads_ (memory_order_acquire).
  //
  // Thus the non-atomic write-then-read accesses to task_ (a. -> d.) are
  // ordered by the release-acquire relationship of accesses to state_ (b. ->
  // c.), and the non-atomic write accesses to task_ (e. -> a.) are ordered by
  // the release-acquire relationship of accesses to count_busy_threads_ (f. ->
  // g.).
  Task* task_ = nullptr;

  // Condition variable used by the outside thread to notify the worker thread
  // of a state change.
  std::condition_variable state_cond_;

  // Mutex used to guard state_cond_
  std::mutex state_cond_mutex_;

  // The state enum tells if we're currently working, waiting for work, etc.
  // It is written to from either the outside thread or the worker thread,
  // in the ChangeState method.
  // It is only ever read by the worker thread.
  std::atomic<State> state_;

  // pointer to the master's thread BlockingCounter object, to notify the
  // master thread of when this thread switches to the 'Ready' state.
  BlockingCounter* const count_busy_threads_;

  // See ThreadPool::spin_duration_.
  const Duration spin_duration_;
};

void ThreadPool::ExecuteImpl(int task_count, int stride, Task* tasks) {
  RUY_TRACE_SCOPE_NAME("ThreadPool::Execute");
  RUY_DCHECK_GE(task_count, 1);

  // Case of 1 thread: just run the single task on the current thread.
  if (task_count == 1) {
    (tasks + 0)->Run();
    return;
  }

  // Task #0 will be run on the current thread.
  CreateThreads(task_count - 1);
  count_busy_threads_.Reset(task_count - 1);
  for (int i = 1; i < task_count; i++) {
    RUY_TRACE_INFO(THREADPOOL_EXECUTE_STARTING_TASK);
    auto task_address = reinterpret_cast<std::uintptr_t>(tasks) + i * stride;
    threads_[i - 1]->StartWork(reinterpret_cast<Task*>(task_address));
  }

  RUY_TRACE_INFO(THREADPOOL_EXECUTE_STARTING_TASK_ZERO_ON_CUR_THREAD);
  // Execute task #0 immediately on the current thread.
  (tasks + 0)->Run();

  RUY_TRACE_INFO(THREADPOOL_EXECUTE_WAITING_FOR_THREADS);
  // Wait for the threads submitted above to finish.
  count_busy_threads_.Wait(spin_duration_);
}

// Ensures that the pool has at least the given count of threads.
// If any new thread has to be created, this function waits for it to
// be ready.
void ThreadPool::CreateThreads(int threads_count) {
  RUY_DCHECK_GE(threads_count, 0);
  unsigned int unsigned_threads_count = threads_count;
  if (threads_.size() >= unsigned_threads_count) {
    return;
  }
  count_busy_threads_.Reset(threads_count - threads_.size());
  while (threads_.size() < unsigned_threads_count) {
    threads_.push_back(new Thread(&count_busy_threads_, spin_duration_));
  }
  count_busy_threads_.Wait(spin_duration_);
}

ThreadPool::~ThreadPool() {
  // Send all exit requests upfront so threads can work on them in parallel.
  for (auto w : threads_) {
    w->RequestExitAsSoonAsPossible();
  }
  for (auto w : threads_) {
    delete w;
  }
}

}  // end namespace ruy
