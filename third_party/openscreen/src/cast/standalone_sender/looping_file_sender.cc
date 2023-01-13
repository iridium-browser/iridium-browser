// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/standalone_sender/looping_file_sender.h"

#include <utility>

#if defined(CAST_STANDALONE_SENDER_HAVE_LIBAOM)
#include "cast/standalone_sender/streaming_av1_encoder.h"
#endif
#include "cast/standalone_sender/streaming_vpx_encoder.h"
#include "platform/base/trivial_clock_traits.h"
#include "util/osp_logging.h"
#include "util/trace_logging.h"

namespace openscreen {
namespace cast {

LoopingFileSender::LoopingFileSender(Environment* environment,
                                     ConnectionSettings settings,
                                     const SenderSession* session,
                                     SenderSession::ConfiguredSenders senders,
                                     ShutdownCallback shutdown_callback)
    : env_(environment),
      settings_(std::move(settings)),
      session_(session),
      shutdown_callback_(std::move(shutdown_callback)),
      audio_encoder_(senders.audio_sender->config().channels,
                     StreamingOpusEncoder::kDefaultCastAudioFramesPerSecond,
                     std::move(senders.audio_sender)),
      video_encoder_(CreateVideoEncoder(
          StreamingVideoEncoder::Parameters{.codec = settings.codec},
          env_->task_runner(),
          std::move(senders.video_sender))),
      next_task_(env_->now_function(), env_->task_runner()),
      console_update_task_(env_->now_function(), env_->task_runner()) {
  // Opus and Vp8 are the default values for the config, and if these are set
  // to a different value that means we offered a codec that we do not
  // support, which is a developer error.
  OSP_CHECK(senders.audio_config.codec == AudioCodec::kOpus);
  OSP_CHECK(senders.video_config.codec == VideoCodec::kVp8 ||
            senders.video_config.codec == VideoCodec::kVp9 ||
            senders.video_config.codec == VideoCodec::kAv1);
  OSP_LOG_INFO << "Max allowed media bitrate (audio + video) will be "
               << settings_.max_bitrate;
  bandwidth_being_utilized_ = settings_.max_bitrate / 2;
  UpdateEncoderBitrates();

  next_task_.Schedule([this] { SendFileAgain(); }, Alarm::kImmediately);
}

LoopingFileSender::~LoopingFileSender() = default;

void LoopingFileSender::SetPlaybackRate(double rate) {
  video_capturer_->SetPlaybackRate(rate);
  audio_capturer_->SetPlaybackRate(rate);
}

void LoopingFileSender::UpdateEncoderBitrates() {
  if (bandwidth_being_utilized_ >= kHighBandwidthThreshold) {
    audio_encoder_.UseHighQuality();
  } else {
    audio_encoder_.UseStandardQuality();
  }
  video_encoder_->SetTargetBitrate(bandwidth_being_utilized_ -
                                   audio_encoder_.GetBitrate());
}

void LoopingFileSender::ControlForNetworkCongestion() {
  bandwidth_estimate_ = session_->GetEstimatedNetworkBandwidth();
  if (bandwidth_estimate_ > 0) {
    // Don't ever try to use *all* of the network bandwidth! However, don't go
    // below the absolute minimum requirement either.
    constexpr double kGoodNetworkCitizenFactor = 0.8;
    const int usable_bandwidth = std::max<int>(
        kGoodNetworkCitizenFactor * bandwidth_estimate_, kMinRequiredBitrate);

    // See "congestion control" discussion in the class header comments for
    // BandwidthEstimator.
    if (usable_bandwidth > bandwidth_being_utilized_) {
      constexpr double kConservativeIncrease = 1.1;
      bandwidth_being_utilized_ = std::min<int>(
          bandwidth_being_utilized_ * kConservativeIncrease, usable_bandwidth);
    } else {
      bandwidth_being_utilized_ = usable_bandwidth;
    }

    // Repsect the user's maximum bitrate setting.
    bandwidth_being_utilized_ =
        std::min(bandwidth_being_utilized_, settings_.max_bitrate);

    UpdateEncoderBitrates();
  } else {
    // There is no current bandwidth estimate. So, nothing should be adjusted.
  }

  next_task_.ScheduleFromNow([this] { ControlForNetworkCongestion(); },
                             kCongestionCheckInterval);
}

void LoopingFileSender::SendFileAgain() {
  OSP_LOG_INFO << "Sending " << settings_.path_to_file
               << " (starts in one second)...";
  TRACE_DEFAULT_SCOPED(TraceCategory::kStandaloneSender);

  OSP_DCHECK_EQ(num_capturers_running_, 0);
  num_capturers_running_ = 2;
  capture_start_time_ = latest_frame_time_ = env_->now() + seconds(1);
  audio_capturer_.emplace(
      env_, settings_.path_to_file.c_str(), audio_encoder_.num_channels(),
      audio_encoder_.sample_rate(), capture_start_time_, this);
  video_capturer_.emplace(env_, settings_.path_to_file.c_str(),
                          capture_start_time_, this);

  next_task_.ScheduleFromNow([this] { ControlForNetworkCongestion(); },
                             kCongestionCheckInterval);
  console_update_task_.Schedule([this] { UpdateStatusOnConsole(); },
                                capture_start_time_);
}

void LoopingFileSender::OnAudioData(const float* interleaved_samples,
                                    int num_samples,
                                    Clock::time_point capture_time) {
  TRACE_SCOPED2(TraceCategory::kStandaloneSender, "OnAudioData", "num_samples",
                std::to_string(num_samples), "capture_time",
                ToString(capture_time));
  latest_frame_time_ = std::max(capture_time, latest_frame_time_);
  audio_encoder_.EncodeAndSend(interleaved_samples, num_samples, capture_time);
}

void LoopingFileSender::OnVideoFrame(const AVFrame& av_frame,
                                     Clock::time_point capture_time) {
  TRACE_SCOPED1(TraceCategory::kStandaloneSender, "OnVideoFrame",
                "capture_time", ToString(capture_time));
  latest_frame_time_ = std::max(capture_time, latest_frame_time_);
  StreamingVideoEncoder::VideoFrame frame{};
  frame.width = av_frame.width - av_frame.crop_left - av_frame.crop_right;
  frame.height = av_frame.height - av_frame.crop_top - av_frame.crop_bottom;
  frame.yuv_planes[0] = av_frame.data[0] + av_frame.crop_left +
                        av_frame.linesize[0] * av_frame.crop_top;
  frame.yuv_planes[1] = av_frame.data[1] + av_frame.crop_left / 2 +
                        av_frame.linesize[1] * av_frame.crop_top / 2;
  frame.yuv_planes[2] = av_frame.data[2] + av_frame.crop_left / 2 +
                        av_frame.linesize[2] * av_frame.crop_top / 2;
  for (int i = 0; i < 3; ++i) {
    frame.yuv_strides[i] = av_frame.linesize[i];
  }
  // TODO(jophba): Add performance metrics visual overlay (based on Stats
  // callback).
  video_encoder_->EncodeAndSend(frame, capture_time, {});
}

void LoopingFileSender::UpdateStatusOnConsole() {
  const Clock::duration elapsed = latest_frame_time_ - capture_start_time_;
  const auto seconds_part = to_seconds(elapsed);
  const auto millis_part = to_milliseconds(elapsed - seconds_part);
  // The control codes here attempt to erase the current line the cursor is
  // on, and then print out the updated status text. If the terminal does not
  // support simple ANSI escape codes, the following will still work, but
  // there might sometimes be old status lines not getting erased (i.e., just
  // partially overwritten).
  fprintf(stdout,
          "\r\x1b[2K\rLoopingFileSender: At %01" PRId64
          ".%03ds in file (est. network bandwidth: %d kbps). \n",
          static_cast<int64_t>(seconds_part.count()),
          static_cast<int>(millis_part.count()), bandwidth_estimate_ / 1024);
  fflush(stdout);

  console_update_task_.ScheduleFromNow([this] { UpdateStatusOnConsole(); },
                                       kConsoleUpdateInterval);
}

void LoopingFileSender::OnEndOfFile(SimulatedCapturer* capturer) {
  OSP_LOG_INFO << "The " << ToTrackName(capturer)
               << " capturer has reached the end of the media stream.";
  --num_capturers_running_;
  if (num_capturers_running_ == 0) {
    console_update_task_.Cancel();

    if (settings_.should_loop_video) {
      OSP_DLOG_INFO << "Starting the media stream over again.";
      next_task_.Schedule([this] { SendFileAgain(); }, Alarm::kImmediately);
    } else {
      OSP_DLOG_INFO << "Video complete. Exiting...";
      shutdown_callback_();
    }
  }
}

void LoopingFileSender::OnError(SimulatedCapturer* capturer,
                                std::string message) {
  OSP_LOG_ERROR << "The " << ToTrackName(capturer)
                << " has failed: " << message;
  --num_capturers_running_;
  // If both fail, the application just pauses. This accounts for things like
  // "file not found" errors. However, if only one track fails, then keep
  // going.
}

const char* LoopingFileSender::ToTrackName(SimulatedCapturer* capturer) const {
  if (capturer == &*audio_capturer_) {
    return "audio";
  } else if (capturer == &*video_capturer_) {
    return "video";
  } else {
    OSP_NOTREACHED();
  }
}

std::unique_ptr<StreamingVideoEncoder> LoopingFileSender::CreateVideoEncoder(
    const StreamingVideoEncoder::Parameters& params,
    TaskRunner* task_runner,
    std::unique_ptr<Sender> sender) {
  switch (params.codec) {
    case VideoCodec::kVp8:
    case VideoCodec::kVp9:
      return std::make_unique<StreamingVpxEncoder>(params, task_runner,
                                                   std::move(sender));
    case VideoCodec::kAv1:
#if defined(CAST_STANDALONE_SENDER_HAVE_LIBAOM)
      return std::make_unique<StreamingAv1Encoder>(params, task_runner,
                                                   std::move(sender));
#else
      OSP_LOG_FATAL << "AV1 codec selected, but could not be used because "
                       "LibAOM not installed.";
#endif
    default:
      // Since we only support VP8, VP9, and AV1, any other codec value here
      // should be due only to developer error.
      OSP_LOG_ERROR << "Unsupported codec " << CodecToString(params.codec);
      OSP_NOTREACHED();
  }
}

}  // namespace cast
}  // namespace openscreen
