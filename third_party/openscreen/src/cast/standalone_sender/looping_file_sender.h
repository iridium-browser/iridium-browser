// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STANDALONE_SENDER_LOOPING_FILE_SENDER_H_
#define CAST_STANDALONE_SENDER_LOOPING_FILE_SENDER_H_

#include <algorithm>
#include <memory>
#include <string>

#include "cast/standalone_sender/connection_settings.h"
#include "cast/standalone_sender/constants.h"
#include "cast/standalone_sender/simulated_capturer.h"
#include "cast/standalone_sender/streaming_opus_encoder.h"
#include "cast/standalone_sender/streaming_video_encoder.h"
#include "cast/streaming/sender_session.h"

namespace openscreen {
namespace cast {

// Plays the media file at a given path over and over again, transcoding and
// streaming its audio/video.
class LoopingFileSender final : public SimulatedAudioCapturer::Client,
                                public SimulatedVideoCapturer::Client {
 public:
  using ShutdownCallback = std::function<void()>;

  LoopingFileSender(Environment* environment,
                    ConnectionSettings settings,
                    const SenderSession* session,
                    SenderSession::ConfiguredSenders senders,
                    ShutdownCallback shutdown_callback);

  ~LoopingFileSender() final;

  void SetPlaybackRate(double rate);

 private:
  void UpdateEncoderBitrates();
  void ControlForNetworkCongestion();
  void SendFileAgain();

  // SimulatedAudioCapturer overrides.
  void OnAudioData(const float* interleaved_samples,
                   int num_samples,
                   Clock::time_point capture_time) final;

  // SimulatedVideoCapturer overrides;
  void OnVideoFrame(const AVFrame& av_frame,
                    Clock::time_point capture_time) final;

  void UpdateStatusOnConsole();

  // SimulatedCapturer::Client overrides.
  void OnEndOfFile(SimulatedCapturer* capturer) final;
  void OnError(SimulatedCapturer* capturer, std::string message) final;

  const char* ToTrackName(SimulatedCapturer* capturer) const;

  std::unique_ptr<StreamingVideoEncoder> CreateVideoEncoder(
      const StreamingVideoEncoder::Parameters& params,
      TaskRunner* task_runner,
      std::unique_ptr<Sender> sender);

  // Holds the required injected dependencies (clock, task runner) used for Cast
  // Streaming, and owns the UDP socket over which all communications occur with
  // the remote's Receivers.
  Environment* const env_;

  // The connection settings used for this session.
  const ConnectionSettings settings_;

  // Session to query for bandwidth information.
  const SenderSession* session_;

  // Callback for tearing down the sender process.
  ShutdownCallback shutdown_callback_;

  int bandwidth_estimate_ = 0;
  int bandwidth_being_utilized_;

  StreamingOpusEncoder audio_encoder_;
  std::unique_ptr<StreamingVideoEncoder> video_encoder_;

  int num_capturers_running_ = 0;
  Clock::time_point capture_start_time_{};
  Clock::time_point latest_frame_time_{};
  absl::optional<SimulatedAudioCapturer> audio_capturer_;
  absl::optional<SimulatedVideoCapturer> video_capturer_;

  Alarm next_task_;
  Alarm console_update_task_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STANDALONE_SENDER_LOOPING_FILE_SENDER_H_
