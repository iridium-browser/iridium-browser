// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_STATISTICS_COLLECTOR_H_
#define CAST_STREAMING_STATISTICS_COLLECTOR_H_

#include <vector>

#include "cast/streaming/statistics_defines.h"
#include "platform/api/time.h"
#include "platform/base/span.h"

namespace openscreen {
namespace cast {

// This POD struct contains helpful information about a given packet that is
// not stored directly on the packet itself.
struct PacketMetadata {
  // The stream type (audio, video, unknown) of this packet.
  StreamType stream_type;

  // The RTP timestamp associated with this packet.
  RtpTimeTicks rtp_timestamp;
};

// This class is responsible for gathering packet and frame statistics using its
// Collect*() methods, that can then be taken by consumers using the Take*()
// methods.
class StatisticsCollector {
 public:
  explicit StatisticsCollector(ClockNowFunctionPtr now);
  ~StatisticsCollector();

  // Informs the collector that a packet has been sent. The collector will then
  // generate a packet event that is then  added to `recent_packet_events_`.
  void CollectPacketSentEvent(ByteView packet, PacketMetadata metadata);

  // Informs the collector that a packet event has occurred. This event is then
  // added to `recent_packet_events_`.
  void CollectPacketEvent(PacketEvent event);

  // Informs the collector that a frame event has occurred. This event is then
  // added to `recent_frame_events_`.
  void CollectFrameEvent(FrameEvent event);

  // Returns the current collection of packet events stored in
  // `recent_packet_events_`. After calling this method, `recent_packet_events_`
  // is reset to an empty vector.
  std::vector<PacketEvent> TakeRecentPacketEvents();

  // Returns the current collection of frame events stored in
  // `recent_frame_events_`. After calling this method, `recent_frame_events_`
  // is reset to an empty vector.
  std::vector<FrameEvent> TakeRecentFrameEvents();

 private:
  ClockNowFunctionPtr now_;
  std::vector<PacketEvent> recent_packet_events_;
  std::vector<FrameEvent> recent_frame_events_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_STATISTICS_COLLECTOR_H_
