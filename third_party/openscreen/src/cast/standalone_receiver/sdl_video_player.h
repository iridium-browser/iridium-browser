// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STANDALONE_RECEIVER_SDL_VIDEO_PLAYER_H_
#define CAST_STANDALONE_RECEIVER_SDL_VIDEO_PLAYER_H_

#include <string>

#include "cast/standalone_receiver/sdl_player_base.h"
#include "cast/streaming/constants.h"

namespace openscreen {
namespace cast {

// Consumes frames from a Receiver, decodes them, and renders them to a
// SDL_Renderer.
class SDLVideoPlayer final : public SDLPlayerBase {
 public:
  // |error_callback| is run only if a fatal error occurs, at which point the
  // player has halted and set |error_status()|.
  SDLVideoPlayer(ClockNowFunctionPtr now_function,
                 TaskRunner* task_runner,
                 Receiver* receiver,
                 VideoCodec codec_name,
                 SDL_Renderer* renderer,
                 std::function<void()> error_callback);

  ~SDLVideoPlayer() final;

 private:
  // Renders the "blue splash" (if waiting) or "red splash" (on error), or
  // otherwise re-renders |frame|; scheduling presentation at an "idle FPS"
  // rate.
  bool RenderWhileIdle(const SDLPlayerBase::PresentableFrame* frame) final;

  // Uploads the decoded picture in |frame| to a SDL texture and draws it using
  // the SDL |renderer_|.
  ErrorOr<Clock::time_point> RenderNextFrame(
      const SDLPlayerBase::PresentableFrame& frame) final;

  // Makes whatever is currently drawn to the SDL |renderer_| be presented
  // on-screen.
  void Present() final;

  // Maps an AVFrame format enum to the SDL equivalent.
  static uint32_t GetSDLPixelFormat(const AVFrame& picture);

  // The SDL renderer drawn to.
  SDL_Renderer* const renderer_;

  // The SDL texture to which the current frame's image is uploaded for
  // accelerated 2D rendering.
  SDLTextureUniquePtr texture_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STANDALONE_RECEIVER_SDL_VIDEO_PLAYER_H_
