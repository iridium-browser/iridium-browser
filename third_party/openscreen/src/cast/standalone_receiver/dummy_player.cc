// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/standalone_receiver/dummy_player.h"

#include <chrono>

#include "absl/types/span.h"
#include "cast/streaming/encoded_frame.h"
#include "util/chrono_helpers.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {

DummyPlayer::DummyPlayer(Receiver* receiver) : receiver_(receiver) {
  OSP_DCHECK(receiver_);
  receiver_->SetConsumer(this);
}

DummyPlayer::~DummyPlayer() {
  receiver_->SetConsumer(nullptr);
}

void DummyPlayer::OnFramesReady(int buffer_size) {
  // Consume the next frame.
  buffer_.resize(buffer_size);
  const EncodedFrame frame =
      receiver_->ConsumeNextFrame(absl::Span<uint8_t>(buffer_));

  // Convert the RTP timestamp to a human-readable timestamp (in µs) and log
  // some short information about the frame.
  const auto media_timestamp =
      frame.rtp_timestamp.ToTimeSinceOrigin<microseconds>(
          receiver_->rtp_timebase());
  OSP_LOG_INFO << "[SSRC " << receiver_->ssrc() << "] "
               << (frame.dependency == EncodedFrame::KEY_FRAME ? "KEY " : "")
               << frame.frame_id << " at " << media_timestamp.count() << "µs, "
               << buffer_size << " bytes";
}

}  // namespace cast
}  // namespace openscreen
