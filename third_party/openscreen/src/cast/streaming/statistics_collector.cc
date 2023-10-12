// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/statistics_collector.h"

#include <stdint.h>

#include <limits>
#include <utility>

#include "cast/streaming/environment.h"
#include "util/big_endian.h"

namespace openscreen {
namespace cast {

StatisticsCollector::StatisticsCollector(ClockNowFunctionPtr now) : now_(now) {}
StatisticsCollector::~StatisticsCollector() = default;

void StatisticsCollector::CollectPacketSentEvent(ByteView packet,
                                                 PacketMetadata metadata) {
  PacketEvent event;

  // Populate the new PacketEvent by parsing the wire-format |packet|.
  event.timestamp = now_();
  event.type = StatisticsEventType::kPacketSentToNetwork;

  BigEndianReader reader(packet.data(), packet.size());
  bool success = reader.Skip(4);
  uint32_t truncated_rtp_timestamp = 0;
  success &= reader.Read<uint32_t>(&truncated_rtp_timestamp);
  success &= reader.Skip(4);

  event.rtp_timestamp = metadata.rtp_timestamp.Expand(truncated_rtp_timestamp);
  event.media_type = ToMediaType(metadata.stream_type);

  success &= reader.Skip(2);
  success &= reader.Read<uint16_t>(&event.packet_id);
  success &= reader.Read<uint16_t>(&event.max_packet_id);

  // Check that the cast is safe.
  // TODO(issuetracker.google.com/3576782): move to checked casts when ready.
  static_assert(static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) <=
                    static_cast<uint64_t>(std::numeric_limits<size_t>::max()),
                "invalid type cast assumption");
  OSP_DCHECK_LE(packet.size(),
                static_cast<size_t>(std::numeric_limits<uint32_t>::max()));
  event.size = static_cast<uint32_t>(packet.size());
  OSP_DCHECK(success);

  recent_packet_events_.emplace_back(event);
}

void StatisticsCollector::CollectPacketEvent(PacketEvent event) {
  recent_packet_events_.emplace_back(event);
}

void StatisticsCollector::CollectFrameEvent(FrameEvent event) {
  recent_frame_events_.emplace_back(event);
}

std::vector<PacketEvent> StatisticsCollector::TakeRecentPacketEvents() {
  std::vector<PacketEvent> out;
  recent_packet_events_.swap(out);
  return out;
}

std::vector<FrameEvent> StatisticsCollector::TakeRecentFrameEvents() {
  std::vector<FrameEvent> out;
  recent_frame_events_.swap(out);
  return out;
}

}  // namespace cast
}  // namespace openscreen
