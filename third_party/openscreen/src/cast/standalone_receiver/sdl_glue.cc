// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/standalone_receiver/sdl_glue.h"

#include <utility>

#include "platform/api/task_runner.h"
#include "platform/api/time.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {

SDLEventLoopProcessor::SDLEventLoopProcessor(
    TaskRunner* task_runner,
    std::function<void()> quit_callback)
    : alarm_(&Clock::now, task_runner),
      quit_callback_(std::move(quit_callback)) {
  alarm_.Schedule([this] { ProcessPendingEvents(); }, Alarm::kImmediately);
}

SDLEventLoopProcessor::~SDLEventLoopProcessor() = default;

void SDLEventLoopProcessor::RegisterForKeyboardEvent(
    SDLEventLoopProcessor::KeyboardEventCallback cb) {
  keyboard_callbacks_.push_back(std::move(cb));
}

void SDLEventLoopProcessor::ProcessPendingEvents() {
  // Process all pending events.
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      OSP_VLOG << "SDL_QUIT received, invoking quit callback...";
      if (quit_callback_) {
        quit_callback_();
      }
    } else if (event.type == SDL_KEYUP) {
      for (auto& cb : keyboard_callbacks_) {
        cb(event.key);
      }
    }
  }

  // Schedule a task to come back and process more pending events.
  constexpr auto kEventPollPeriod = std::chrono::milliseconds(10);
  alarm_.ScheduleFromNow([this] { ProcessPendingEvents(); }, kEventPollPeriod);
}

}  // namespace cast
}  // namespace openscreen
