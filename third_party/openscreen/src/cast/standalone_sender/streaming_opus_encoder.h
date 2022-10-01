// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STANDALONE_SENDER_STREAMING_OPUS_ENCODER_H_
#define CAST_STANDALONE_SENDER_STREAMING_OPUS_ENCODER_H_

#include <stdint.h>

#include <memory>

#include "cast/streaming/encoded_frame.h"
#include "cast/streaming/sender.h"
#include "platform/api/time.h"

extern "C" {
struct OpusEncoder;
}

namespace openscreen {
namespace cast {

// Wraps the libopus encoder so that the application can stream
// interleaved-floats audio samples to a Sender. Either mono or stereo sound is
// supported.
class StreamingOpusEncoder {
 public:
  // Constructs the encoder for mono or stereo sound, dividing the stream of
  // audio samples up into chunks as determined by the given
  // |cast_frames_per_second|, and for EncodedFrame output to the given
  // |sender|. The sample rate of the audio is assumed to be the Sender's fixed
  // |rtp_timebase()|.
  StreamingOpusEncoder(int num_channels,
                       int cast_frames_per_second,
                       Sender* sender);

  ~StreamingOpusEncoder();

  int num_channels() const { return num_channels_; }
  int sample_rate() const { return sender_->rtp_timebase(); }

  int GetBitrate() const;

  // Sets the encoder back to its "AUTO" bitrate setting, for standard quality.
  // This and UseHighQuality() may be called as often as needed as conditions
  // change.
  //
  // Note: As of 2020-01-21, the encoder in "auto bitrate" mode would use a
  // bitrate of 102kbps for 2-channel, 48 kHz audio and a 10 ms frame size.
  void UseStandardQuality();

  // Sets the encoder to use a high bitrate (virtually no artifacts), when
  // plenty of network bandwidth is available. This and UseStandardQuality() may
  // be called as often as needed as conditions change.
  void UseHighQuality();

  // Encode and send the given |interleaved_samples|, which contains
  // |num_samples| tuples (i.e., multiply by the number of channels to determine
  // the number of array elements). The audio is assumed to have been captured
  // at the required |sample_rate()|. |reference_time| refers to the first
  // sample.
  void EncodeAndSend(const float* interleaved_samples,
                     int num_samples,
                     Clock::time_point reference_time);

  static constexpr int kDefaultCastAudioFramesPerSecond =
      100;  // 10 ms frame duration.

 private:
  OpusEncoder* encoder() const {
    return reinterpret_cast<OpusEncoder*>(encoder_storage_.get());
  }

  // Updates the |codec_delay_| based on the current encoder settings.
  void UpdateCodecDelay();

  // Sets the next frame's reference time, accounting for codec buffering delay.
  // Also, checks whether the reference time has drifted too far forwards, and
  // skips if necessary.
  void ResolveTimestampsAndMaybeSkip(Clock::time_point reference_time);

  // Fills the input buffer as much as possible from the given source data, and
  // returns the number of samples copied into the buffer.
  int FillInputBuffer(const float* interleaved_samples, int num_samples);

  const int num_channels_;
  Sender* const sender_;
  const int samples_per_cast_frame_;
  const Clock::duration approximate_cast_frame_duration_;
  const std::unique_ptr<uint8_t[]> encoder_storage_;
  const std::unique_ptr<float[]> input_;     // Interleaved audio samples.
  const std::unique_ptr<uint8_t[]> output_;  // Opus-encoded packet.

  // The audio delay introduced by the codec.
  Clock::duration codec_delay_{};

  // The number of mono/stereo tuples currently queued in the |input_| buffer.
  // Multiply by |num_channels_| to get the number of array elements.
  int num_samples_queued_ = 0;

  // The reference time of the first frame passed to EncodeAndSend(), offset by
  // the codec delay.
  Clock::time_point start_time_ = Clock::time_point::min();

  // Initialized and used by EncodeAndSend() to hold the metadata and data
  // pointer for each frame being sent.
  EncodedFrame frame_;

  // The |reference_time| for the last sent frame. This is used to check that
  // the reference times are monotonically increasing. If they have [illegally]
  // gone backwards too much, warnings will be logged.
  Clock::time_point last_sent_frame_reference_time_;

  // This is the recommended value, according to documentation in
  // src/include/opus.h in libopus, so that the Opus encoder does not degrade
  // the audio due to memory constraints.
  static constexpr int kOpusMaxPayloadSize = 4000;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STANDALONE_SENDER_STREAMING_OPUS_ENCODER_H_
