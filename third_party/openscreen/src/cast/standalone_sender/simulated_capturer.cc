// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/standalone_sender/simulated_capturer.h"

#include <libavformat/version.h>

#include <algorithm>
#include <chrono>
#include <ratio>
#include <sstream>
#include <thread>

#include "cast/standalone_sender/ffmpeg_glue.h"
#include "cast/streaming/environment.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {

using clock_operators::operator<<;

namespace {
// Threshold at which a warning about media pausing should be logged.
constexpr std::chrono::seconds kPauseWarningThreshold{3};
}  // namespace

SimulatedCapturer::Observer::~Observer() = default;

SimulatedCapturer::SimulatedCapturer(Environment* environment,
                                     const char* path,
                                     AVMediaType media_type,
                                     Clock::time_point start_time,
                                     Observer* observer)
    : format_context_(MakeUniqueAVFormatContext(path)),
      media_type_(media_type),
      start_time_(start_time),
      observer_(observer),
      packet_(MakeUniqueAVPacket()),
      decoded_frame_(MakeUniqueAVFrame()),
      next_task_(environment->now_function(), environment->task_runner()) {
  OSP_DCHECK(observer_);

  if (!format_context_) {
    OnError("MakeUniqueAVFormatContext", AVERROR_UNKNOWN);
    return;  // Capturer is halted (unable to start).
  }

#if LIBAVFORMAT_VERSION_MAJOR < 59
  AVCodec* codec;
#else
  const AVCodec* codec;
#endif  // LIBAVFORMAT_VERSION_MAJOR < 59

  const int stream_result = av_find_best_stream(format_context_.get(),
                                                media_type_, -1, -1, &codec, 0);
  if (stream_result < 0) {
    OnError("av_find_best_stream", stream_result);
    return;  // Capturer is halted (unable to start).
  }

  stream_index_ = stream_result;
  decoder_context_ = MakeUniqueAVCodecContext(codec);
  if (!decoder_context_) {
    OnError("MakeUniqueAVCodecContext", AVERROR_BUG);
    return;  // Capturer is halted (unable to start).
  }
  // This should also be 16 or less, since the encoder implementations emit
  // warnings about too many encode threads. FFMPEG's VP8 implementation
  // actually silently freezes if this is 10 or more. Thus, 8 is used for the
  // max here, just to be safe.
  decoder_context_->thread_count =
      std::min(std::max<int>(std::thread::hardware_concurrency(), 1), 8);
  const int params_result = avcodec_parameters_to_context(
      decoder_context_.get(),
      format_context_->streams[stream_index_]->codecpar);
  if (params_result < 0) {
    OnError("avcodec_parameters_to_context", params_result);
    return;  // Capturer is halted (unable to start).
  }
  SetAdditionalDecoderParameters(decoder_context_.get());

  const int open_result = avcodec_open2(decoder_context_.get(), codec, nullptr);
  if (open_result < 0) {
    OnError("avcodec_open2", open_result);
    return;  // Capturer is halted (unable to start).
  }

  next_task_.Schedule([this] { StartDecodingNextFrame(); },
                      Alarm::kImmediately);
}

SimulatedCapturer::~SimulatedCapturer() = default;

void SimulatedCapturer::SetPlaybackRate(double rate) {
  playback_rate_is_non_zero_ = rate > 0;
  if (playback_rate_is_non_zero_) {
    // Restart playback now that playback rate is nonzero.
    StartDecodingNextFrame();
  }
}

void SimulatedCapturer::SetAdditionalDecoderParameters(
    AVCodecContext* decoder_context) {}

absl::optional<Clock::duration> SimulatedCapturer::ProcessDecodedFrame(
    const AVFrame& frame) {
  return Clock::duration::zero();
}

void SimulatedCapturer::OnError(const char* function_name, int av_errnum) {
  // Make a human-readable string from the libavcodec error.
  std::ostringstream error;
  error << "For " << av_get_media_type_string(media_type_) << ", "
        << function_name << " returned error: " << AvErrorToString(av_errnum);

  // Deliver the error notification in a separate task since this method might
  // have been called from the constructor.
  next_task_.Schedule(
      [this, error_string = error.str()] {
        observer_->OnError(this, error_string);
        // Capturer is now halted.
      },
      Alarm::kImmediately);
}

// static
Clock::duration SimulatedCapturer::ToApproximateClockDuration(
    int64_t ticks,
    const AVRational& time_base) {
  return Clock::duration(av_rescale_q(
      ticks, time_base,
      AVRational{Clock::duration::period::num, Clock::duration::period::den}));
}

void SimulatedCapturer::StartDecodingNextFrame() {
  if (!playback_rate_is_non_zero_) {
    return;
  }
  const int read_frame_result =
      av_read_frame(format_context_.get(), packet_.get());
  if (read_frame_result < 0) {
    if (read_frame_result == AVERROR_EOF) {
      // Insert a "flush request" into the decoder's pipeline, which will
      // signal an EOF in ConsumeNextDecodedFrame() later.
      avcodec_send_packet(decoder_context_.get(), nullptr);
      next_task_.Schedule([this] { ConsumeNextDecodedFrame(); },
                          Alarm::kImmediately);
    } else {
      // All other error codes are fatal.
      OnError("av_read_frame", read_frame_result);
      // Capturer is now halted.
    }
    return;
  }

  if (packet_->stream_index != stream_index_) {
    av_packet_unref(packet_.get());
    next_task_.Schedule([this] { StartDecodingNextFrame(); },
                        Alarm::kImmediately);
    return;
  }

  const int send_packet_result =
      avcodec_send_packet(decoder_context_.get(), packet_.get());
  av_packet_unref(packet_.get());
  if (send_packet_result < 0) {
    // Note: AVERROR(EAGAIN) is also treated as fatal here because
    // avcodec_receive_frame() will be called repeatedly until its result code
    // indicates avcodec_send_packet() must be called again.
    OnError("avcodec_send_packet", send_packet_result);
    return;  // Capturer is now halted.
  }

  next_task_.Schedule([this] { ConsumeNextDecodedFrame(); },
                      Alarm::kImmediately);
}

void SimulatedCapturer::ConsumeNextDecodedFrame() {
  const int receive_frame_result =
      avcodec_receive_frame(decoder_context_.get(), decoded_frame_.get());
  if (receive_frame_result < 0) {
    switch (receive_frame_result) {
      case AVERROR(EAGAIN):
        // This result code, according to libavcodec documentation, means more
        // data should be fed into the decoder (e.g., interframe dependencies).
        next_task_.Schedule([this] { StartDecodingNextFrame(); },
                            Alarm::kImmediately);
        return;
      case AVERROR_EOF:
        observer_->OnEndOfFile(this);
        return;  // Capturer is now halted.
      default:
        OnError("avcodec_receive_frame", receive_frame_result);
        return;  // Capturer is now halted.
    }
  }

  const Clock::duration frame_timestamp = ToApproximateClockDuration(
      decoded_frame_->best_effort_timestamp,
      format_context_->streams[stream_index_]->time_base);
  if (last_frame_timestamp_) {
    const Clock::duration delta = frame_timestamp - *last_frame_timestamp_;
    if (delta <= Clock::duration::zero()) {
      OSP_LOG_WARN << "Dropping " << av_get_media_type_string(media_type_)
                   << " frame with illegal timestamp (delta from last frame: "
                   << delta << "). Bad media file!";
      av_frame_unref(decoded_frame_.get());
      next_task_.Schedule([this] { ConsumeNextDecodedFrame(); },
                          Alarm::kImmediately);
      return;
    } else if (delta >= kPauseWarningThreshold) {
      OSP_LOG_INFO << "For " << av_get_media_type_string(media_type_)
                   << ", encountered a media pause (" << delta
                   << ") in the file.";
    }
  }
  last_frame_timestamp_ = frame_timestamp;

  Clock::time_point capture_time = start_time_ + frame_timestamp;
  const auto delay_adjustment_or_null = ProcessDecodedFrame(*decoded_frame_);
  if (!delay_adjustment_or_null) {
    av_frame_unref(decoded_frame_.get());
    return;  // Stop. Fatal error occurred.
  }
  capture_time += *delay_adjustment_or_null;

  next_task_.Schedule(
      [this, capture_time] {
        DeliverDataToClient(*decoded_frame_, capture_time);
        av_frame_unref(decoded_frame_.get());
        ConsumeNextDecodedFrame();
      },
      capture_time);
}

SimulatedAudioCapturer::Client::~Client() = default;

SimulatedAudioCapturer::SimulatedAudioCapturer(Environment* environment,
                                               const char* path,
                                               int num_channels,
                                               int sample_rate,
                                               Clock::time_point start_time,
                                               Client* client)
    : SimulatedCapturer(environment,
                        path,
                        AVMEDIA_TYPE_AUDIO,
                        start_time,
                        client),
      num_channels_(num_channels),
      sample_rate_(sample_rate),
      client_(client),
      resampler_(MakeUniqueSwrContext()) {
  OSP_DCHECK_GT(num_channels_, 0);
  OSP_DCHECK_GT(sample_rate_, 0);
}

SimulatedAudioCapturer::~SimulatedAudioCapturer() {
  if (swr_is_initialized(resampler_.get())) {
    swr_close(resampler_.get());
  }
}

bool SimulatedAudioCapturer::EnsureResamplerIsInitializedFor(
    const AVFrame& frame) {
  if (swr_is_initialized(resampler_.get())) {
    if (input_sample_format_ == static_cast<AVSampleFormat>(frame.format) &&
        input_sample_rate_ == frame.sample_rate &&
#if _LIBAVUTIL_OLD_CHANNEL_LAYOUT
        input_channel_layout_ == frame.channel_layout
#else
        input_channel_layout_.nb_channels == frame.ch_layout.nb_channels
#endif  // _LIBAVUTIL_OLD_CHANNEL_LAYOUT
    ) {
      return true;
    }

    // Note: Usually, the resampler should be flushed before being destroyed.
    // However, because of the way SimulatedAudioCapturer uses the API, only one
    // audio sample should be dropped in the worst case. Log what's being
    // dropped, just in case libswresample is behaving differently than
    // expected.
    const std::chrono::microseconds amount(
        swr_get_delay(resampler_.get(), std::micro::den));
    OSP_LOG_INFO << "Discarding " << amount
                 << " of audio from the resampler before re-init.";
  }

  input_sample_format_ = AV_SAMPLE_FMT_NONE;

  // Create a fake output frame to hold the output audio parameters, because the
  // resampler API is weird that way.
  const auto fake_output_frame = MakeUniqueAVFrame();
#if _LIBAVUTIL_OLD_CHANNEL_LAYOUT
  fake_output_frame->channel_layout =
      av_get_default_channel_layout(num_channels_);
#else
  av_channel_layout_default(&fake_output_frame->ch_layout, num_channels_);
#endif  // _LIBAVUTIL_OLD_CHANNEL_LAYOUT

  fake_output_frame->format = AV_SAMPLE_FMT_FLT;
  fake_output_frame->sample_rate = sample_rate_;
  const int config_result =
      swr_config_frame(resampler_.get(), fake_output_frame.get(), &frame);
  if (config_result < 0) {
    OnError("swr_config_frame", config_result);
    return false;  // Capturer is now halted.
  }

  const int init_result = swr_init(resampler_.get());
  if (init_result < 0) {
    OnError("swr_init", init_result);
    return false;  // Capturer is now halted.
  }

  input_sample_format_ = static_cast<AVSampleFormat>(frame.format);
  input_sample_rate_ = frame.sample_rate;
  input_channel_layout_ =
#if _LIBAVUTIL_OLD_CHANNEL_LAYOUT
      frame.channel_layout;
#else
      frame.ch_layout;
#endif  // _LIBAVUTIL_OLD_CHANNEL_LAYOUT
  return true;
}

absl::optional<Clock::duration> SimulatedAudioCapturer::ProcessDecodedFrame(
    const AVFrame& frame) {
  if (!EnsureResamplerIsInitializedFor(frame)) {
    return absl::nullopt;
  }

  const int64_t num_leftover_input_samples =
      swr_get_delay(resampler_.get(), input_sample_rate_);
  OSP_DCHECK_GE(num_leftover_input_samples, 0);
  const Clock::duration capture_time_adjustment = -ToApproximateClockDuration(
      num_leftover_input_samples, AVRational{1, input_sample_rate_});

  const int64_t num_output_samples_desired =
      av_rescale_rnd(num_leftover_input_samples + frame.nb_samples,
                     sample_rate_, input_sample_rate_, AV_ROUND_ZERO);
  OSP_DCHECK_GE(num_output_samples_desired, 0);
  resampled_audio_.resize(num_channels_ * num_output_samples_desired);
  uint8_t* output_argument[1] = {
      reinterpret_cast<uint8_t*>(resampled_audio_.data())};
  const int num_samples_converted_or_error = swr_convert(
      resampler_.get(), output_argument, num_output_samples_desired,
      const_cast<const uint8_t**>(frame.extended_data), frame.nb_samples);
  if (num_samples_converted_or_error < 0) {
    resampled_audio_.clear();
    swr_close(resampler_.get());
    OnError("swr_convert", num_samples_converted_or_error);
    return absl::nullopt;  // Capturer is now halted.
  }
  resampled_audio_.resize(num_channels_ * num_samples_converted_or_error);

  return capture_time_adjustment;
}

void SimulatedAudioCapturer::DeliverDataToClient(
    const AVFrame& unused,
    Clock::time_point capture_time) {
  if (resampled_audio_.empty()) {
    return;
  }
  client_->OnAudioData(resampled_audio_.data(),
                       resampled_audio_.size() / num_channels_, capture_time);
  resampled_audio_.clear();
}

SimulatedVideoCapturer::Client::~Client() = default;

SimulatedVideoCapturer::SimulatedVideoCapturer(Environment* environment,
                                               const char* path,
                                               Clock::time_point start_time,
                                               Client* client)
    : SimulatedCapturer(environment,
                        path,
                        AVMEDIA_TYPE_VIDEO,
                        start_time,
                        client),
      client_(client) {}

SimulatedVideoCapturer::~SimulatedVideoCapturer() = default;

void SimulatedVideoCapturer::SetAdditionalDecoderParameters(
    AVCodecContext* decoder_context) {
  // Require the I420 planar format for video.
  decoder_context->get_format = [](struct AVCodecContext* s,
                                   const enum AVPixelFormat* formats) {
    // Return AV_PIX_FMT_YUV420P if it's in the provided list of supported
    // formats. Otherwise, return AV_PIX_FMT_NONE.
    //
    // |formats| is a NONE-terminated array.
    for (; *formats != AV_PIX_FMT_NONE; ++formats) {
      if (*formats == AV_PIX_FMT_YUV420P) {
        break;
      }
    }
    return *formats;
  };
}

void SimulatedVideoCapturer::DeliverDataToClient(
    const AVFrame& frame,
    Clock::time_point capture_time) {
  client_->OnVideoFrame(frame, capture_time);
}

}  // namespace cast
}  // namespace openscreen
