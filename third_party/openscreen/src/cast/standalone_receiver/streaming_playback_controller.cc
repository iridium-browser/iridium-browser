// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/standalone_receiver/streaming_playback_controller.h"

#include <string>

#if defined(CAST_STANDALONE_RECEIVER_HAVE_EXTERNAL_LIBS)
#include "cast/standalone_receiver/sdl_audio_player.h"  // nogncheck
#include "cast/standalone_receiver/sdl_glue.h"          // nogncheck
#include "cast/standalone_receiver/sdl_video_player.h"  // nogncheck
#else
#include "cast/standalone_receiver/dummy_player.h"  // nogncheck
#endif  // defined(CAST_STANDALONE_RECEIVER_HAVE_EXTERNAL_LIBS)

#include "util/trace_logging.h"

namespace openscreen {
namespace cast {

StreamingPlaybackController::Client::~Client() = default;

#if defined(CAST_STANDALONE_RECEIVER_HAVE_EXTERNAL_LIBS)
StreamingPlaybackController::StreamingPlaybackController(
    TaskRunner* task_runner,
    StreamingPlaybackController::Client* client)
    : task_runner_(task_runner),
      client_(client),
      sdl_event_loop_(task_runner_, [this] {
        client_->OnPlaybackError(this,
                                 Error{Error::Code::kOperationCancelled,
                                       std::string("SDL event loop closed.")});
      }) {
  OSP_DCHECK(task_runner_ != nullptr);
  OSP_DCHECK(client_ != nullptr);
  constexpr int kDefaultWindowWidth = 1280;
  constexpr int kDefaultWindowHeight = 720;
  window_ = MakeUniqueSDLWindow(
      "Cast Streaming Receiver Demo",
      SDL_WINDOWPOS_UNDEFINED /* initial X position */,
      SDL_WINDOWPOS_UNDEFINED /* initial Y position */, kDefaultWindowWidth,
      kDefaultWindowHeight, SDL_WINDOW_RESIZABLE);
  OSP_CHECK(window_) << "Failed to create SDL window: " << SDL_GetError();
  renderer_ = MakeUniqueSDLRenderer(window_.get(), -1, 0);
  OSP_CHECK(renderer_) << "Failed to create SDL renderer: " << SDL_GetError();

  sdl_event_loop_.RegisterForKeyboardEvent(
      [this](const SDL_KeyboardEvent& event) {
        this->HandleKeyboardEvent(event);
      });
}
#else
StreamingPlaybackController::StreamingPlaybackController(
    TaskRunner* task_runner,
    StreamingPlaybackController::Client* client)
    : task_runner_(task_runner), client_(client) {
  OSP_DCHECK(task_runner_ != nullptr);
  OSP_DCHECK(client_ != nullptr);
}
#endif  // defined(CAST_STANDALONE_RECEIVER_HAVE_EXTERNAL_LIBS)

void StreamingPlaybackController::OnNegotiated(
    const ReceiverSession* session,
    ReceiverSession::ConfiguredReceivers receivers) {
  TRACE_DEFAULT_SCOPED(TraceCategory::kStandaloneReceiver);
  Initialize(receivers);
}

void StreamingPlaybackController::OnRemotingNegotiated(
    const ReceiverSession* session,
    ReceiverSession::RemotingNegotiation negotiation) {
  remoting_receiver_ =
      std::make_unique<SimpleRemotingReceiver>(negotiation.messenger);
  remoting_receiver_->SendInitializeMessage(
      [this, receivers = negotiation.receivers](AudioCodec audio_codec,
                                                VideoCodec video_codec) {
        // The configurations in |negotiation| do not have the actual codecs,
        // only REMOTE_AUDIO and REMOTE_VIDEO. Once we receive the
        // initialization callback method, we can override with the actual
        // codecs here.
        auto mutable_receivers = receivers;
        mutable_receivers.audio_config.codec = audio_codec;
        mutable_receivers.video_config.codec = video_codec;
        Initialize(mutable_receivers);
      });
}

void StreamingPlaybackController::OnReceiversDestroying(
    const ReceiverSession* session,
    ReceiversDestroyingReason reason) {
  OSP_LOG_INFO << "Receivers are currently destroying, resetting SDL players.";
  audio_player_.reset();
  video_player_.reset();
}

void StreamingPlaybackController::OnError(const ReceiverSession* session,
                                          Error error) {
  client_->OnPlaybackError(this, error);
}

void StreamingPlaybackController::Initialize(
    ReceiverSession::ConfiguredReceivers receivers) {
#if defined(CAST_STANDALONE_RECEIVER_HAVE_EXTERNAL_LIBS)
  OSP_LOG_INFO << "Successfully negotiated a session, creating SDL players.";
  if (receivers.audio_receiver) {
    audio_player_ = std::make_unique<SDLAudioPlayer>(
        &Clock::now, task_runner_, receivers.audio_receiver,
        receivers.audio_config.codec, [this] {
          client_->OnPlaybackError(this, audio_player_->error_status());
        });
  }
  if (receivers.video_receiver) {
    video_player_ = std::make_unique<SDLVideoPlayer>(
        &Clock::now, task_runner_, receivers.video_receiver,
        receivers.video_config.codec, renderer_.get(), [this] {
          client_->OnPlaybackError(this, video_player_->error_status());
        });
  }
#else
  if (receivers.audio_receiver) {
    audio_player_ = std::make_unique<DummyPlayer>(receivers.audio_receiver);
  }

  if (receivers.video_receiver) {
    video_player_ = std::make_unique<DummyPlayer>(receivers.video_receiver);
  }
#endif  // defined(CAST_STANDALONE_RECEIVER_HAVE_EXTERNAL_LIBS)
}

#if defined(CAST_STANDALONE_RECEIVER_HAVE_EXTERNAL_LIBS)
void StreamingPlaybackController::HandleKeyboardEvent(
    const SDL_KeyboardEvent& event) {
  // We only handle keyboard events if we are remoting.
  if (!remoting_receiver_) {
    return;
  }

  switch (event.keysym.sym) {
    // See codes here: https://wiki.libsdl.org/SDL_Scancode
    case SDLK_KP_SPACE:  // fallthrough, "Keypad Space"
    case SDLK_SPACE:     // "Space"
      is_playing_ = !is_playing_;
      remoting_receiver_->SendPlaybackRateMessage(is_playing_ ? 1.0 : 0.0);
      break;
  }
}
#endif

}  // namespace cast
}  // namespace openscreen
