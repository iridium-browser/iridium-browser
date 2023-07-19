// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/encoded_frame.h"

namespace openscreen {
namespace cast {

EncodedFrame::EncodedFrame(Dependency dependency,
                           FrameId frame_id,
                           FrameId referenced_frame_id,
                           RtpTimeTicks rtp_timestamp,
                           Clock::time_point reference_time,
                           std::chrono::milliseconds new_playout_delay,
                           ByteView data)
    : dependency(dependency),
      frame_id(frame_id),
      referenced_frame_id(referenced_frame_id),
      rtp_timestamp(rtp_timestamp),
      reference_time(reference_time),
      new_playout_delay(new_playout_delay),
      data(data) {}

EncodedFrame::EncodedFrame() = default;
EncodedFrame::~EncodedFrame() = default;

EncodedFrame::EncodedFrame(EncodedFrame&&) noexcept = default;
EncodedFrame& EncodedFrame::operator=(EncodedFrame&&) = default;

void EncodedFrame::CopyMetadataTo(EncodedFrame* dest) const {
  dest->dependency = this->dependency;
  dest->frame_id = this->frame_id;
  dest->referenced_frame_id = this->referenced_frame_id;
  dest->rtp_timestamp = this->rtp_timestamp;
  dest->reference_time = this->reference_time;
  dest->new_playout_delay = this->new_playout_delay;
}

}  // namespace cast
}  // namespace openscreen
