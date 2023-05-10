// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/standalone_receiver/sdl_audio_player.h"

#include <chrono>
#include <sstream>
#include <utility>

#include "absl/types/span.h"
#include "cast/standalone_receiver/avcodec_glue.h"
#include "util/big_endian.h"
#include "util/chrono_helpers.h"
#include "util/osp_logging.h"
#include "util/trace_logging.h"

namespace openscreen {
namespace cast {

namespace {

constexpr char kAudioMediaType[] = "audio";
constexpr SDL_AudioFormat kSDLAudioFormatUnknown = 0;

bool SDLAudioSpecsAreDifferent(const SDL_AudioSpec& a, const SDL_AudioSpec& b) {
  return a.freq != b.freq || a.format != b.format || a.channels != b.channels ||
         a.samples != b.samples;
}

// Convert |num_channels| separate |planes| of audio, each containing
// |num_samples| samples, into a single array of |interleaved| samples. The
// memory backing all of the input arrays and the output array is assumed to be
// suitably aligned.
template <typename Element>
void InterleaveAudioSamples(const uint8_t* const planes[],
                            int num_channels,
                            int num_samples,
                            uint8_t* interleaved) {
  TRACE_DEFAULT_SCOPED(TraceCategory::kStandaloneReceiver);
  // Note: This could be optimized with SIMD intrinsics for much better
  // performance.
  auto* dest = reinterpret_cast<Element*>(interleaved);
  for (int ch = 0; ch < num_channels; ++ch) {
    auto* const src = reinterpret_cast<const Element*>(planes[ch]);
    for (int i = 0; i < num_samples; ++i) {
      dest[i * num_channels] = src[i];
    }
    ++dest;
  }
}

}  // namespace

SDLAudioPlayer::SDLAudioPlayer(ClockNowFunctionPtr now_function,
                               TaskRunner* task_runner,
                               Receiver* receiver,
                               AudioCodec codec,
                               std::function<void()> error_callback)
    : SDLPlayerBase(now_function,
                    task_runner,
                    receiver,
                    CodecToString(codec),
                    std::move(error_callback),
                    kAudioMediaType) {}

SDLAudioPlayer::~SDLAudioPlayer() {
  if (device_ > 0) {
    SDL_CloseAudioDevice(device_);
  }
}

ErrorOr<Clock::time_point> SDLAudioPlayer::RenderNextFrame(
    const SDLPlayerBase::PresentableFrame& next_frame) {
  TRACE_DEFAULT_SCOPED(TraceCategory::kStandaloneReceiver);
  OSP_DCHECK(next_frame.decoded_frame);
  const AVFrame& frame = *next_frame.decoded_frame;

  pending_audio_spec_ = device_spec_;
  pending_audio_spec_.freq = frame.sample_rate;

  // Punt if the AVFrame's format is not compatible with those supported by SDL.
  const auto frame_format = static_cast<AVSampleFormat>(frame.format);
  pending_audio_spec_.format = GetSDLAudioFormat(frame_format);
  if (pending_audio_spec_.format == kSDLAudioFormatUnknown) {
    std::ostringstream error;
    error << "SDL does not support AVSampleFormat " << frame_format;
    return Error(Error::Code::kUnknownError, error.str());
  }

  // Punt if the number of channels is not supported by SDL.
  constexpr int kSdlSupportedChannelCounts[] = {1, 2, 4, 6};
  int frame_channels =
#if _LIBAVUTIL_OLD_CHANNEL_LAYOUT
      frame.channels;
#else
      frame.ch_layout.nb_channels;
#endif  // _LIBAVUTIL_OLD_CHANNEL_LAYOUT

  if (std::find(std::begin(kSdlSupportedChannelCounts),
                std::end(kSdlSupportedChannelCounts),
                frame_channels) == std::end(kSdlSupportedChannelCounts)) {
    std::ostringstream error;
    error << "SDL does not support " << frame_channels << " audio channels.";
    return Error(Error::Code::kUnknownError, error.str());
  }
  pending_audio_spec_.channels = frame_channels;

  // If |device_spec_| is different from what is required, re-compute the sample
  // buffer size and the amount of time that represents. The |device_spec_| will
  // be updated to match |pending_audio_spec_| later, in Present().
  if (SDLAudioSpecsAreDifferent(device_spec_, pending_audio_spec_)) {
    // Find the smallest power-of-two number of samples that represents at least
    // 20ms of audio.
    constexpr auto kMinBufferDuration = milliseconds(20);
    constexpr auto kOneSecond = seconds(1);
    const auto required_samples = static_cast<int>(
        pending_audio_spec_.freq * kMinBufferDuration / kOneSecond);
    OSP_DCHECK_GE(required_samples, 1);
    pending_audio_spec_.samples = 1 << av_log2(required_samples);
    if (pending_audio_spec_.samples < required_samples) {
      pending_audio_spec_.samples *= 2;
    }

    approximate_lead_time_ =
        (pending_audio_spec_.samples * Clock::to_duration(kOneSecond)) /
        pending_audio_spec_.freq;
  }

  // If the decoded audio is in planar format, interleave it for SDL.
  const int bytes_per_sample = av_get_bytes_per_sample(frame_format);
  const int byte_count = frame.nb_samples * frame_channels * bytes_per_sample;
  if (av_sample_fmt_is_planar(frame_format)) {
    interleaved_audio_buffer_.resize(byte_count);
    switch (bytes_per_sample) {
      case 1:
        InterleaveAudioSamples<uint8_t>(frame.data, frame_channels,
                                        frame.nb_samples,
                                        &interleaved_audio_buffer_[0]);
        break;
      case 2:
        InterleaveAudioSamples<uint16_t>(frame.data, frame_channels,
                                         frame.nb_samples,
                                         &interleaved_audio_buffer_[0]);
        break;
      case 4:
        InterleaveAudioSamples<uint32_t>(frame.data, frame_channels,
                                         frame.nb_samples,
                                         &interleaved_audio_buffer_[0]);
        break;
      default:
        OSP_NOTREACHED();
    }
    pending_audio_ = absl::Span<const uint8_t>(interleaved_audio_buffer_);
  } else {
    if (!interleaved_audio_buffer_.empty()) {
      interleaved_audio_buffer_.clear();
      interleaved_audio_buffer_.shrink_to_fit();
    }
    pending_audio_ = absl::Span<const uint8_t>(frame.data[0], byte_count);
  }

  // SDL provides no way to query the actual lead time before audio samples will
  // be output by the sound hardware. The only advice seems to be a quick
  // comment about "the intent is double buffered audio." Thus, schedule the
  // "push" of this data to happen such that the audio will be playing out of
  // the hardware at the intended moment in time.
  return next_frame.presentation_time - approximate_lead_time_;
}

bool SDLAudioPlayer::RenderWhileIdle(const PresentableFrame* frame) {
  // Do nothing. The SDL audio buffer will underrun and result in silence.
  return false;
}

void SDLAudioPlayer::Present() {
  TRACE_DEFAULT_SCOPED(TraceCategory::kStandaloneReceiver);
  if (state() != kScheduledToPresent) {
    // In all other states, just do nothing. The SDL audio buffer will underrun
    // and result in silence.
    return;
  }

  // Re-open audio device, if the audio format has changed.
  if (SDLAudioSpecsAreDifferent(pending_audio_spec_, device_spec_)) {
    if (device_ > 0) {
      SDL_CloseAudioDevice(device_);
      device_spec_ = SDL_AudioSpec{};
    }

    device_ = SDL_OpenAudioDevice(nullptr,  // Pick default device.
                                  0,        // For playback, not recording.
                                  &pending_audio_spec_,  // Desired format.
                                  &device_spec_,  // [output] Obtained format.
                                  0  // Disallow formats other than desired.
    );
    if (device_ <= 0) {
      std::ostringstream error;
      error << "SDL_OpenAudioDevice failed: " << SDL_GetError();
      OnFatalError(error.str());
      return;
    }
    OSP_DCHECK(!SDLAudioSpecsAreDifferent(pending_audio_spec_, device_spec_));

    constexpr int kSdlResumePlaybackCommand = 0;
    SDL_PauseAudioDevice(device_, kSdlResumePlaybackCommand);
  }

  SDL_QueueAudio(device_, pending_audio_.data(), pending_audio_.size());
}

// static
SDL_AudioFormat SDLAudioPlayer::GetSDLAudioFormat(AVSampleFormat format) {
  switch (format) {
    case AV_SAMPLE_FMT_U8P:
    case AV_SAMPLE_FMT_U8:
      return AUDIO_U8;

    case AV_SAMPLE_FMT_S16P:
    case AV_SAMPLE_FMT_S16:
      return IsBigEndianArchitecture() ? AUDIO_S16MSB : AUDIO_S16LSB;

    case AV_SAMPLE_FMT_S32P:
    case AV_SAMPLE_FMT_S32:
      return IsBigEndianArchitecture() ? AUDIO_S32MSB : AUDIO_S32LSB;

    case AV_SAMPLE_FMT_FLTP:
    case AV_SAMPLE_FMT_FLT:
      return IsBigEndianArchitecture() ? AUDIO_F32MSB : AUDIO_F32LSB;

    default:
      // Either NONE, or the 64-bit formats are unsupported.
      break;
  }

  return kSDLAudioFormatUnknown;
}

}  // namespace cast
}  // namespace openscreen
