// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STANDALONE_RECEIVER_SDL_AUDIO_PLAYER_H_
#define CAST_STANDALONE_RECEIVER_SDL_AUDIO_PLAYER_H_

#include <string>
#include <vector>

#include "cast/standalone_receiver/sdl_player_base.h"

namespace openscreen {
namespace cast {

// Consumes frames from a Receiver, decodes them, and renders them to an
// internally-owned SDL audio device.
class SDLAudioPlayer final : public SDLPlayerBase {
 public:
  // |error_callback| is run only if a fatal error occurs, at which point the
  // player has halted and set |error_status()|.
  SDLAudioPlayer(ClockNowFunctionPtr now_function,
                 TaskRunner* task_runner,
                 Receiver* receiver,
                 AudioCodec codec,
                 std::function<void()> error_callback);

  ~SDLAudioPlayer() final;

 private:
  // SDLPlayerBase implementation.
  ErrorOr<Clock::time_point> RenderNextFrame(
      const SDLPlayerBase::PresentableFrame& frame) final;
  bool RenderWhileIdle(const SDLPlayerBase::PresentableFrame* frame) final;
  void Present() final;

  // Maps an AVSampleFormat enum to the SDL_AudioFormat equivalent.
  static SDL_AudioFormat GetSDLAudioFormat(AVSampleFormat format);

  // The audio format determined by the last call to RenderCurrentFrame().
  SDL_AudioSpec pending_audio_spec_{};

  // The amount of time before a target presentation time to call Present(), to
  // account for audio buffering (the latency until samples reach the hardware).
  Clock::duration approximate_lead_time_{};

  // When the decoder provides planar data, this buffer is used for storing the
  // interleaved conversion.
  std::vector<uint8_t> interleaved_audio_buffer_;

  // Points to the memory containing the next chunk of interleaved audio.
  absl::Span<const uint8_t> pending_audio_;

  // The currently-open SDL audio device (or zero, if not open).
  SDL_AudioDeviceID device_ = 0;

  // The audio format being used by the currently-open SDL audio device.
  SDL_AudioSpec device_spec_{};
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STANDALONE_RECEIVER_SDL_AUDIO_PLAYER_H_
