// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/msg_loop.h"

#include "base/logging.h"

namespace {

#if !defined(OS_ZOS)
thread_local MsgLoop* g_current;
#else
// TODO(gabylb) - zos: thread_local not yet supported, use zoslib's impl'n:
__tlssim<MsgLoop*> __g_current_impl(nullptr);
#define g_current (*__g_current_impl.access())
#endif
}

MsgLoop::MsgLoop() {
  DCHECK(g_current == nullptr);
  g_current = this;
}

MsgLoop::~MsgLoop() {
  DCHECK(g_current == this);
  g_current = nullptr;
}

void MsgLoop::Run() {
  while (!should_quit_) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> queue_lock(queue_mutex_);
      notifier_.wait(queue_lock, [this]() {
        return (!task_queue_.empty()) || should_quit_;
      });

      if (should_quit_)
        return;

      task = std::move(task_queue_.front());
      task_queue_.pop();
    }

    task();
  }
}

void MsgLoop::PostQuit() {
  PostTask([this]() { should_quit_ = true; });
}

void MsgLoop::PostTask(std::function<void()> work) {
  {
    std::unique_lock<std::mutex> queue_lock(queue_mutex_);
    task_queue_.emplace(std::move(work));
  }

  notifier_.notify_one();
}

void MsgLoop::RunUntilIdleForTesting() {
  for (bool done = false; !done;) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> queue_lock(queue_mutex_);
      task = std::move(task_queue_.front());
      task_queue_.pop();

      if (task_queue_.empty())
        done = true;
    }

    task();
  }
}

MsgLoop* MsgLoop::Current() {
  return g_current;
}
