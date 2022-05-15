// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_REMOTING_CAPABILITIES_H_
#define CAST_STREAMING_REMOTING_CAPABILITIES_H_

#include <string>
#include <vector>

namespace openscreen {
namespace cast {

// Audio capabilities are how receivers indicate support for remoting codecs--
// as remoting does not include the actual codec in the OFFER message.
enum class AudioCapability {
  // The "baseline set" is used in Chrome to check support for a wide
  // variety of audio codecs in media/remoting/renderer_controller.cc, including
  // but not limited to MP3, PCM, Ogg Vorbis, and FLAC.
  kBaselineSet,
  kAac,
  kOpus,
};

// Similar to audio capabilities, video capabilities are how the receiver
// indicates support for certain video codecs, as well as support for streaming
// 4k content. It is assumed by the sender that the receiver can support 4k
// on all supported codecs.
enum class VideoCapability {
  // |kSupports4k| indicates that the receiver wants and can support 4k remoting
  // content--both decoding/rendering and either a native 4k display or
  // downscaling to the display's native resolution.
  // TODO(issuetracker.google.com/184429130): |kSupports4k| is not super helpful
  // for enabling 4k support, as receivers may not support 4k for all types of
  // content.
  kSupports4k,
  kH264,
  kVp8,
  kVp9,
  kHevc,
  kAv1
};

// This class is similar to the RemotingSinkMetadata in Chrome, however
// it is focused around our needs and is not mojom-based. This contains
// a rough set of capabilities of the receiver to give the sender an idea of
// what features are suppported for remoting.
// TODO(issuetracker.google.com/184189100): this object should be expanded to
// allow more specific constraint tracking.
struct RemotingCapabilities {
  // Receiver audio-specific capabilities.
  std::vector<AudioCapability> audio;

  // Receiver video-specific capabilities.
  std::vector<VideoCapability> video;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_REMOTING_CAPABILITIES_H_
