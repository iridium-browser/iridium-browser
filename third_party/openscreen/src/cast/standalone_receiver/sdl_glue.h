// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STANDALONE_RECEIVER_SDL_GLUE_H_
#define CAST_STANDALONE_RECEIVER_SDL_GLUE_H_

#include <stdint.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#include <SDL2/SDL.h>
#pragma GCC diagnostic pop

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "util/alarm.h"

namespace openscreen {

class TaskRunner;

namespace cast {

template <uint32_t subsystem>
class ScopedSDLSubSystem {
 public:
  ScopedSDLSubSystem() { SDL_InitSubSystem(subsystem); }
  ~ScopedSDLSubSystem() { SDL_QuitSubSystem(subsystem); }
};

// Macro that, for an SDL_Foo, generates code for:
//
//  using SDLFooUniquePtr = std::unique_ptr<SDL_Foo, SDLFooDestroyer>;
//  SDLFooUniquePtr MakeUniqueSDLFoo(...args...);
#define DEFINE_SDL_UNIQUE_PTR(name)                          \
  struct SDL##name##Destroyer {                              \
    void operator()(SDL_##name* obj) const {                 \
      if (obj) {                                             \
        SDL_Destroy##name(obj);                              \
      }                                                      \
    }                                                        \
  };                                                         \
  using SDL##name##UniquePtr =                               \
      std::unique_ptr<SDL_##name, SDL##name##Destroyer>;     \
  template <typename... Args>                                \
  SDL##name##UniquePtr MakeUniqueSDL##name(Args&&... args) { \
    return SDL##name##UniquePtr(                             \
        SDL_Create##name(std::forward<Args>(args)...));      \
  }

DEFINE_SDL_UNIQUE_PTR(Window);
DEFINE_SDL_UNIQUE_PTR(Renderer);
DEFINE_SDL_UNIQUE_PTR(Texture);

#undef DEFINE_SDL_UNIQUE_PTR

// A looping mechanism that runs the SDL event loop by scheduling periodic tasks
// to the given TaskRunner. Looping continues indefinitely, until the instance
// is destroyed. A client-provided quit callback is invoked whenever a SDL_QUIT
// event is received.
class SDLEventLoopProcessor {
 public:
  SDLEventLoopProcessor(TaskRunner& task_runner,
                        std::function<void()> quit_callback);
  ~SDLEventLoopProcessor();

  using KeyboardEventCallback = std::function<void(const SDL_KeyboardEvent&)>;
  void RegisterForKeyboardEvent(KeyboardEventCallback cb);

 private:
  void ProcessPendingEvents();

  Alarm alarm_;
  std::function<void()> quit_callback_;
  std::vector<KeyboardEventCallback> keyboard_callbacks_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STANDALONE_RECEIVER_SDL_GLUE_H_
