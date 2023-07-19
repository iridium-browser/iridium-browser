// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_ENCODED_FRAME_H_
#define CAST_STREAMING_ENCODED_FRAME_H_

#include <stdint.h>

#include <chrono>
#include <vector>

#include "cast/streaming/frame_id.h"
#include "cast/streaming/rtp_time.h"
#include "platform/api/time.h"
#include "platform/base/macros.h"
#include "platform/base/span.h"

namespace openscreen {
namespace cast {

// A combination of metadata and data for one encoded frame.  This can contain
// audio data or video data or other.
struct EncodedFrame {
  enum class Dependency : int8_t {
    // "null" value, used to indicate whether |dependency| has been set.
    kUnknown,

    // Not decodable without the reference frame indicated by
    // |referenced_frame_id|.
    kDependent,

    // Independently decodable.
    kIndependent,

    // Independently decodable, and no future frames will depend on any frames
    // before this one.
    kKeyFrame,
  };

  EncodedFrame(Dependency dependency,
               FrameId frame_id,
               FrameId referenced_frame_id,
               RtpTimeTicks rtp_timestamp,
               Clock::time_point reference_time,
               std::chrono::milliseconds new_playout_delay,
               ByteView data);
  EncodedFrame();
  ~EncodedFrame();

  EncodedFrame(EncodedFrame&&) noexcept;
  EncodedFrame& operator=(EncodedFrame&&);

  // Copies all members except |data| to |dest|. Does not modify |dest->data|.
  void CopyMetadataTo(EncodedFrame* dest) const;

  // This frame's dependency relationship with respect to other frames.
  Dependency dependency = Dependency::kUnknown;

  // The label associated with this frame.  Implies an ordering relative to
  // other frames in the same stream.
  FrameId frame_id;

  // The label associated with the frame upon which this frame depends.  If
  // this frame does not require any other frame in order to become decodable
  // (e.g., key frames), |referenced_frame_id| must equal |frame_id|.
  FrameId referenced_frame_id;

  // The stream timestamp, on the timeline of the signal data. For example, RTP
  // timestamps for audio are usually defined as the total number of audio
  // samples encoded in all prior frames. A playback system uses this value to
  // detect gaps in the stream, and otherwise stretch the signal to gradually
  // re-align towards playout targets when too much drift has occurred (see
  // |reference_time|, below).
  RtpTimeTicks rtp_timestamp;

  // The common reference clock timestamp for this frame. Over a sequence of
  // frames, this time value is expected to drift with respect to the elapsed
  // time implied by the RTP timestamps; and this may not necessarily increment
  // with precise regularity.
  //
  // This value originates from a sender, and is the time at which the frame was
  // captured/recorded. In the receiver context, this value is the computed
  // target playout time, which is used for guiding the timing of presentation
  // (see |rtp_timestamp|, above). It is also meant to be used to synchronize
  // the presentation of multiple streams (e.g., audio and video), commonly
  // known as "lip-sync." It is NOT meant to be a mandatory/exact playout time.
  Clock::time_point reference_time;

  // Playout delay for this and all future frames. Used by the Adaptive
  // Playout delay extension. Non-positive values means no change.
  std::chrono::milliseconds new_playout_delay{};

  // A buffer containing the encoded signal data for the frame. In the sender
  // context, this points to the data to be sent. In the receiver context, this
  // is set to the region of a client-provided buffer that was populated.
  ByteView data;

  OSP_DISALLOW_COPY_AND_ASSIGN(EncodedFrame);
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_ENCODED_FRAME_H_
