// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/standalone_sender/streaming_opus_encoder.h"

#include <opus/opus.h>

#include <algorithm>
#include <chrono>
#include <utility>

#include "platform/base/span.h"
#include "util/chrono_helpers.h"

namespace openscreen {
namespace cast {

using clock_operators::operator<<;

namespace {

// The bitrate at which virtually all stereo audio can be encoded and decoded
// without human-perceivable artifacts. Source:
// https://wiki.hydrogenaud.io/index.php?title=Opus#Bitrate_performance
constexpr opus_int32 kTransparentBitrate = 160000;

// The maximum number of Cast audio frames the encoder may fall behind by before
// skipping-ahead the RTP timestamps to compensate.
constexpr int kMaxCastFramesBeforeSkip = 3;

}  // namespace

StreamingOpusEncoder::StreamingOpusEncoder(int num_channels,
                                           int cast_frames_per_second,
                                           std::unique_ptr<Sender> sender)
    : num_channels_(num_channels),
      sender_(std::move(sender)),
      samples_per_cast_frame_(sample_rate() / cast_frames_per_second),
      approximate_cast_frame_duration_(Clock::to_duration(seconds(1)) /
                                       cast_frames_per_second),
      encoder_storage_(new uint8_t[opus_encoder_get_size(num_channels_)]),
      input_(new float[num_channels_ * samples_per_cast_frame_]),
      output_(new uint8_t[kOpusMaxPayloadSize]) {
  OSP_CHECK_GT(cast_frames_per_second, 0);
  OSP_DCHECK(sender_);
  OSP_CHECK_GT(samples_per_cast_frame_, 0);
  OSP_CHECK_EQ(sample_rate() % cast_frames_per_second, 0);
  OSP_CHECK(approximate_cast_frame_duration_ > Clock::duration::zero());

  frame_.dependency = EncodedFrame::Dependency::kKeyFrame;

  const auto init_result = opus_encoder_init(
      encoder(), sample_rate(), num_channels_, OPUS_APPLICATION_AUDIO);
  OSP_CHECK_EQ(init_result, OPUS_OK);

  UseStandardQuality();
}

StreamingOpusEncoder::~StreamingOpusEncoder() = default;

int StreamingOpusEncoder::GetBitrate() const {
  opus_int32 bitrate;
  const auto ctl_result =
      opus_encoder_ctl(encoder(), OPUS_GET_BITRATE(&bitrate));
  OSP_CHECK_EQ(ctl_result, OPUS_OK);
  return bitrate;
}

void StreamingOpusEncoder::UseStandardQuality() {
  const auto ctl_result =
      opus_encoder_ctl(encoder(), OPUS_SET_BITRATE(OPUS_AUTO));
  OSP_CHECK_EQ(ctl_result, OPUS_OK);
  UpdateCodecDelay();
}

void StreamingOpusEncoder::UseHighQuality() {
  // kTransparentBitrate assumes stereo audio. Scale it by the actual number of
  // channels.
  const opus_int32 bitrate = kTransparentBitrate * num_channels_ / 2;
  const auto ctl_result =
      opus_encoder_ctl(encoder(), OPUS_SET_BITRATE(bitrate));
  OSP_CHECK_EQ(ctl_result, OPUS_OK);
  UpdateCodecDelay();
}

void StreamingOpusEncoder::EncodeAndSend(const float* interleaved_samples,
                                         int num_samples,
                                         Clock::time_point reference_time) {
  OSP_DCHECK(interleaved_samples);
  OSP_DCHECK_GT(num_samples, 0);

  ResolveTimestampsAndMaybeSkip(reference_time);

  while (num_samples > 0) {
    const int samples_copied =
        FillInputBuffer(interleaved_samples, num_samples);
    num_samples -= samples_copied;
    interleaved_samples += num_channels_ * samples_copied;

    if (num_samples_queued_ < samples_per_cast_frame_) {
      return;  // Not enough yet for a full Cast audio frame.
    }

    const opus_int32 packet_size_or_error =
        opus_encode_float(encoder(), input_.get(), num_samples_queued_,
                          output_.get(), kOpusMaxPayloadSize);
    num_samples_queued_ = 0;
    if (packet_size_or_error < 0) {
      OSP_LOG_FATAL << "AUDIO[" << sender_->ssrc()
                    << "] Error code from opus_encode_float(): "
                    << packet_size_or_error;
      return;
    }

    frame_.frame_id = sender_->GetNextFrameId();
    frame_.referenced_frame_id = frame_.frame_id;
    // Note: It's possible for Opus to encode a zero byte packet. Send a Cast
    // audio frame anyway, to represent the passage of silence and to send other
    // stream metadata.
    frame_.data = ByteView(output_.get(), packet_size_or_error);
    last_sent_frame_reference_time_ = frame_.reference_time;
    switch (sender_->EnqueueFrame(frame_)) {
      case Sender::OK:
        break;
      case Sender::REACHED_ID_SPAN_LIMIT:
        OSP_LOG_WARN << "AUDIO[" << sender_->ssrc()
                     << "] Dropping: FrameId span limit reached.";
        break;
      case Sender::MAX_DURATION_IN_FLIGHT:
        OSP_LOG_INFO << "AUDIO[" << sender_->ssrc()
                     << "] Dropping: In-flight duration would be too high.";
        break;
      case Sender::PAYLOAD_TOO_LARGE:
        OSP_NOTREACHED();  // The Opus packet cannot possibly be too large.
    }

    frame_.rtp_timestamp += RtpTimeDelta::FromTicks(samples_per_cast_frame_);
    frame_.reference_time += approximate_cast_frame_duration_;
  }
}

void StreamingOpusEncoder::UpdateCodecDelay() {
  opus_int32 lookahead = 0;
  const auto ctl_result =
      opus_encoder_ctl(encoder(), OPUS_GET_LOOKAHEAD(&lookahead));
  OSP_CHECK_EQ(ctl_result, OPUS_OK);
  codec_delay_ = RtpTimeDelta::FromTicks(lookahead).ToDuration<Clock::duration>(
      sample_rate());
}

void StreamingOpusEncoder::ResolveTimestampsAndMaybeSkip(
    Clock::time_point reference_time) {
  // Back-track the reference time to account for the audio delay introduced by
  // the codec.
  reference_time -= codec_delay_;

  // Special case: Nothing special for the first frame's timestamps.
  if (start_time_ == Clock::time_point::min()) {
    frame_.rtp_timestamp = RtpTimeTicks();
    frame_.reference_time = start_time_ = reference_time;
    last_sent_frame_reference_time_ =
        reference_time - approximate_cast_frame_duration_;
    return;
  }

  const RtpTimeTicks current_position =
      frame_.rtp_timestamp + RtpTimeDelta::FromTicks(num_samples_queued_);
  const RtpTimeTicks reference_position = RtpTimeTicks::FromTimeSinceOrigin(
      reference_time - start_time_, sample_rate());
  const RtpTimeDelta rtp_advancement = reference_position - current_position;
  const RtpTimeDelta skip_threshold =
      RtpTimeDelta::FromTicks(samples_per_cast_frame_) *
      kMaxCastFramesBeforeSkip;
  if (rtp_advancement > skip_threshold) {
    OSP_LOG_WARN << "Detected audio gap "
                 << rtp_advancement.ToDuration<microseconds>(sample_rate())
                 << ", skipping ahead...";
    num_samples_queued_ = 0;
    frame_.rtp_timestamp = reference_position;
  }

  // Further back-track the reference time to account for the already-queued
  // samples.
  reference_time -= RtpTimeDelta::FromTicks(num_samples_queued_)
                        .ToDuration<Clock::duration>(sample_rate());

  // Frame reference times must be monotonically increasing. A little noise in
  // the negative direction is okay to cap-off. Log a warning if there's a
  // bigger problem (at the source).
  const Clock::time_point lower_bound =
      last_sent_frame_reference_time_ +
      RtpTimeDelta::FromTicks(1).ToDuration<Clock::duration>(sample_rate());
  if (reference_time < lower_bound) {
    const Clock::duration backwards_amount =
        last_sent_frame_reference_time_ - reference_time;
    OSP_LOG_IF(WARN, backwards_amount >= approximate_cast_frame_duration_)
        << "Reference time went *backwards* too much (" << backwards_amount
        << " in wrong direction). A/V sync may suffer at the Receiver!";
    reference_time = lower_bound;
  }

  frame_.reference_time = reference_time;
}

int StreamingOpusEncoder::FillInputBuffer(const float* interleaved_samples,
                                          int num_samples) {
  const int samples_needed = samples_per_cast_frame_ - num_samples_queued_;
  const int samples_to_copy = std::min(num_samples, samples_needed);
  std::copy(interleaved_samples,
            interleaved_samples + num_channels_ * samples_to_copy,
            input_.get() + num_channels_ * num_samples_queued_);
  num_samples_queued_ += samples_to_copy;
  return samples_to_copy;
}

// static
constexpr int StreamingOpusEncoder::kDefaultCastAudioFramesPerSecond;
// static
constexpr int StreamingOpusEncoder::kOpusMaxPayloadSize;

}  // namespace cast
}  // namespace openscreen
