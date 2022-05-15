// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_RECEIVER_PACKET_ROUTER_H_
#define CAST_STREAMING_RECEIVER_PACKET_ROUTER_H_

#include <stdint.h>

#include <utility>
#include <vector>

#include "absl/types/span.h"
#include "cast/streaming/environment.h"
#include "cast/streaming/ssrc.h"
#include "util/flat_map.h"

namespace openscreen {
namespace cast {

class Receiver;

// Handles all network I/O among multiple Receivers meant for synchronized
// play-out (e.g., one Receiver for audio, one Receiver for video). Incoming
// traffic is dispatched to the appropriate Receiver, based on its corresponding
// sender's SSRC. Also, all traffic not coming from the same source is
// filtered-out.
class ReceiverPacketRouter final : public Environment::PacketConsumer {
 public:
  explicit ReceiverPacketRouter(Environment* environment);
  ~ReceiverPacketRouter() final;

 protected:
  friend class Receiver;

  // Called from a Receiver constructor/destructor to register/deregister a
  // Receiver instance that processes RTP/RTCP packets from a Sender having the
  // given SSRC.
  void OnReceiverCreated(Ssrc sender_ssrc, Receiver* receiver);
  void OnReceiverDestroyed(Ssrc sender_ssrc);

  // Called by a Receiver to send a RTCP packet back to the source from which
  // earlier packets were received, or does nothing if OnReceivedPacket() has
  // not been called yet.
  void SendRtcpPacket(absl::Span<const uint8_t> packet);

 private:
  // Environment::PacketConsumer implementation.
  void OnReceivedPacket(const IPEndpoint& source,
                        Clock::time_point arrival_time,
                        std::vector<uint8_t> packet) final;

  Environment* const environment_;

  FlatMap<Ssrc, Receiver*> receivers_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_RECEIVER_PACKET_ROUTER_H_
