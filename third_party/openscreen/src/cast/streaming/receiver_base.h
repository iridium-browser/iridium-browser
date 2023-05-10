// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_RECEIVER_BASE_H_
#define CAST_STREAMING_RECEIVER_BASE_H_

#include <chrono>

#include "absl/types/span.h"
#include "cast/streaming/encoded_frame.h"
#include "cast/streaming/session_config.h"
#include "cast/streaming/ssrc.h"
#include "platform/api/time.h"
#include "platform/base/span.h"

namespace openscreen {
namespace cast {

// The Cast Streaming Receiver, a peer corresponding to some Cast Streaming
// Sender at the other end of a network link.
//
// Cast Streaming is a transport protocol which divides up the frames for one
// media stream (e.g., audio or video) into multiple RTP packets containing an
// encrypted payload. The Receiver is the peer responsible for collecting the
// RTP packets, decrypting the payload, and re-assembling a frame that can be
// passed to a decoder and played out.
//
// A Sender ↔ Receiver pair is used to transport each media stream. Typically,
// there are two pairs in a normal system, one for the audio stream and one for
// video stream. A local player is responsible for synchronizing the playout of
// the frames of each stream to achieve lip-sync. See the discussion in
// encoded_frame.h for how the |reference_time| and |rtp_timestamp| of the
// EncodedFrames are used to achieve this.
class ReceiverBase {
 public:
  class Consumer {
   public:
    virtual ~Consumer();

    // Called whenever one or more frames have become ready for consumption. The
    // |next_frame_buffer_size| argument is identical to the result of calling
    // AdvanceToNextFrame(), and so the Consumer only needs to prepare a buffer
    // and call ConsumeNextFrame(). It may then call AdvanceToNextFrame() to
    // check whether there are any more frames ready, but this is not mandatory.
    // See usage example in SDLPlayerBase::OnFramesReady.
    virtual void OnFramesReady(int next_frame_buffer_size) = 0;
  };

  ReceiverBase();
  virtual ~ReceiverBase();

  virtual const SessionConfig& config() const = 0;
  virtual int rtp_timebase() const = 0;
  virtual Ssrc ssrc() const = 0;

  // Set the Consumer receiving notifications when new frames are ready for
  // consumption. Frames received before this method is called will remain in
  // the queue indefinitely.
  virtual void SetConsumer(Consumer* consumer) = 0;

  // Sets how much time the consumer will need to decode/buffer/render/etc., and
  // otherwise fully process a frame for on-time playback. This information is
  // used by the Receiver to decide whether to skip past frames that have
  // arrived too late. This method can be called repeatedly to make adjustments
  // based on changing environmental conditions. It is HIGHLY recommended that
  // consumers of this API provide a proper processing time, otherwise there
  // may be significantly larger playout delays.
  //
  // Default setting: kDefaultPlayerProcessingTime
  virtual void SetPlayerProcessingTime(Clock::duration needed_time) = 0;

  // Propagates a "picture loss indicator" notification to the Sender,
  // requesting a key frame so that decode/playout can recover. It is safe to
  // call this redundantly. The Receiver will clear the picture loss condition
  // automatically, once a key frame is received (i.e., before
  // ConsumeNextFrame() is called to access it).
  virtual void RequestKeyFrame() = 0;

  // Advances to the next frame ready for consumption. This may skip-over
  // incomplete frames that will not play out on-time; but only if there are
  // completed frames further down the queue that have no dependency
  // relationship with them (e.g., key frames).
  //
  // This method returns kNoFramesReady if there is not currently a frame ready
  // for consumption. The caller should wait for a Consumer::OnFramesReady()
  // notification before trying again. Otherwise, the number of bytes of encoded
  // data is returned, and the caller should use this to ensure the buffer it
  // passes to ConsumeNextFrame() is large enough.
  virtual int AdvanceToNextFrame() = 0;

  // Returns the next frame, both metadata and payload data. The Consumer calls
  // this method after being notified via OnFramesReady(), and it can also call
  // this whenever AdvanceToNextFrame() indicates another frame is ready.
  // |buffer| must point to a sufficiently-sized buffer that will be populated
  // with the frame's payload data. Upon return |frame->data| will be set to the
  // portion of the buffer that was populated.
  virtual EncodedFrame ConsumeNextFrame(ByteBuffer buffer) = 0;

  // The default "player processing time" amount. See SetPlayerProcessingTime().
  // This value is based on real world experimentation, however may vary
  // widely depending on the platform of the receiver and what type of
  // decoder is available.
  static constexpr std::chrono::milliseconds kDefaultPlayerProcessingTime{50};

  // Returned by AdvanceToNextFrame() when there are no frames currently ready
  // for consumption.
  static constexpr int kNoFramesReady = -1;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_RECEIVER_BASE_H_
