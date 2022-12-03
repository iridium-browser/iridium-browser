// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_RECEIVER_CONSTRAINTS_H_
#define CAST_STREAMING_RECEIVER_CONSTRAINTS_H_

#include <chrono>
#include <memory>
#include <vector>

#include "cast/streaming/constants.h"
#include "cast/streaming/resolution.h"

namespace openscreen {
namespace cast {

// Information about the display the receiver is attached to.
struct Display {
  // Returns true if all configurations supported by |other| are also
  // supported by this instance.
  bool IsSupersetOf(const Display& other) const;

  // The display limitations of the actual screen, used to provide upper
  // bounds on streams. For example, we will never
  // send 60FPS if it is going to be displayed on a 30FPS screen.
  // Note that we may exceed the display width and height for standard
  // content sizes like 720p or 1080p.
  Dimensions dimensions;

  // Whether the embedder is capable of scaling content. If set to false,
  // the sender will manage the aspect ratio scaling.
  bool can_scale_content = false;
};

// Codec-specific audio limits for playback.
struct AudioLimits {
  // Returns true if all configurations supported by |other| are also
  // supported by this instance.
  bool IsSupersetOf(const AudioLimits& other) const;

  // Whether or not these limits apply to all codecs.
  bool applies_to_all_codecs = false;

  // Audio codec these limits apply to. Note that if |applies_to_all_codecs|
  // is true this field is ignored.
  AudioCodec codec;

  // Maximum audio sample rate.
  int max_sample_rate = kDefaultAudioSampleRate;

  // Maximum audio channels, default is currently stereo.
  int max_channels = kDefaultAudioChannels;

  // Minimum and maximum bitrates. Generally capture is done at the maximum
  // bit rate, since audio bandwidth is much lower than video for most
  // content.
  int min_bit_rate = kDefaultAudioMinBitRate;
  int max_bit_rate = kDefaultAudioMaxBitRate;

  // Max playout delay in milliseconds.
  std::chrono::milliseconds max_delay = kDefaultMaxDelayMs;
};

// Codec-specific video limits for playback.
struct VideoLimits {
  // Returns true if all configurations supported by |other| are also
  // supported by this instance.
  bool IsSupersetOf(const VideoLimits& other) const;

  // Whether or not these limits apply to all codecs.
  bool applies_to_all_codecs = false;

  // Video codec these limits apply to. Note that if |applies_to_all_codecs|
  // is true this field is ignored.
  VideoCodec codec;

  // Maximum pixels per second. Value is the standard amount of pixels
  // for 1080P at 30FPS.
  int max_pixels_per_second = 1920 * 1080 * 30;

  // Maximum dimensions. Minimum dimensions try to use the same aspect
  // ratio and are generated from the spec.
  Dimensions max_dimensions = {1920, 1080, {kDefaultFrameRate, 1}};

  // Minimum and maximum bitrates. Default values are based on default min and
  // max dimensions, embedders that support different display dimensions
  // should strongly consider setting these fields.
  int min_bit_rate = kDefaultVideoMinBitRate;
  int max_bit_rate = kDefaultVideoMaxBitRate;

  // Max playout delay in milliseconds.
  std::chrono::milliseconds max_delay = kDefaultMaxDelayMs;
};

// This struct is used to provide constraints for setting up and running
// remoting streams. These properties are based on the current control
// protocol and allow remoting with current senders.
struct RemotingConstraints {
  // Returns true if all configurations supported by |other| are also
  // supported by this instance.
  bool IsSupersetOf(const RemotingConstraints& other) const;

  // Current remoting senders take an "all or nothing" support for audio
  // codec support. While Opus and AAC support is handled in our Constraints'
  // |audio_codecs| property, support for the following codecs must be
  // enabled or disabled all together:
  // MP3
  // PCM, including Mu-Law, S16BE, S24BE, and ALAW variants
  // Ogg Vorbis
  // FLAC
  // AMR, including narrow band (NB) and wide band (WB) variants
  // GSM Mobile Station (MS)
  // EAC3 (Dolby Digital Plus)
  // ALAC (Apple Lossless)
  // AC-3 (Dolby Digital)
  // These properties are tied directly to what Chrome supports. See:
  // https://source.chromium.org/chromium/chromium/src/+/master:media/base/audio_codecs.h
  bool supports_chrome_audio_codecs = false;

  // Current remoting senders assume that the receiver supports 4K for all
  // video codecs supplied in |video_codecs|, or none of them.
  bool supports_4k = false;
};

// Note: embedders are required to implement the following
// codecs to be Cast V2 compliant: H264, VP8, AAC, Opus.
class ReceiverConstraints {
 public:
  ReceiverConstraints();
  ReceiverConstraints(std::vector<VideoCodec> video_codecs,
                      std::vector<AudioCodec> audio_codecs);
  ReceiverConstraints(std::vector<VideoCodec> video_codecs,
                      std::vector<AudioCodec> audio_codecs,
                      std::vector<AudioLimits> audio_limits,
                      std::vector<VideoLimits> video_limits,
                      std::unique_ptr<Display> description);

  ReceiverConstraints(ReceiverConstraints&&) noexcept;
  ReceiverConstraints(const ReceiverConstraints&);
  ReceiverConstraints& operator=(ReceiverConstraints&&) noexcept;
  ReceiverConstraints& operator=(const ReceiverConstraints&);

  // Returns true if all configurations supported by |other| are also
  // supported by this instance.
  //
  // TODO(crbug.com/1356762): Implement receiver-side session renegotation
  // so we can eliminate complicated logic for constraints compatiblity.
  bool IsSupersetOf(const ReceiverConstraints& other) const;

  // Audio and video codec constraints. Should be supplied in order of
  // preference, e.g. in this example if we get both VP8 and H264 we will
  // generally select the VP8 offer. If a codec is omitted from these fields
  // it will never be selected in the OFFER/ANSWER negotiation.
  std::vector<VideoCodec> video_codecs{VideoCodec::kVp8, VideoCodec::kH264};
  std::vector<AudioCodec> audio_codecs{AudioCodec::kOpus, AudioCodec::kAac};

  // Optional limitation fields that help the sender provide a delightful
  // cast experience. Although optional, highly recommended.
  // NOTE: embedders that wish to apply the same limits for all codecs can
  // pass a vector of size 1 with the |applies_to_all_codecs| field set to
  // true.
  std::vector<AudioLimits> audio_limits;
  std::vector<VideoLimits> video_limits;
  std::unique_ptr<Display> display_description;

  // Libcast remoting support is opt-in: embedders wishing to field remoting
  // offers may provide a set of remoting constraints, or leave nullptr for
  // all remoting OFFERs to be rejected in favor of continuing streaming.
  std::unique_ptr<RemotingConstraints> remoting;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_RECEIVER_CONSTRAINTS_H_
