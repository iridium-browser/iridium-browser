// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/statistics_defines.h"

namespace openscreen {
namespace cast {

const EnumNameTable<StatisticsEventType,
                    static_cast<size_t>(StatisticsEventType::kNumOfEvents)>
    kStatisticEventTypeNames = {
        {{"Unknown", StatisticsEventType::kUnknown},
         {"FrameCaptureBegin", StatisticsEventType::kFrameCaptureBegin},
         {"FrameCaptureEnd", StatisticsEventType::kFrameCaptureEnd},
         {"FrameEncoded", StatisticsEventType::kFrameEncoded},
         {"FrameAckReceived", StatisticsEventType::kFrameAckReceived},
         {"FrameAckSent", StatisticsEventType::kFrameAckSent},
         {"FrameDecoded", StatisticsEventType::kFrameDecoded},
         {"FramePlayedOut", StatisticsEventType::kFramePlayedOut},
         {"PacketSentToNetwork", StatisticsEventType::kPacketSentToNetwork},
         {"PacketRetransmitted", StatisticsEventType::kPacketRetransmitted},
         {"PacketRtxRejected", StatisticsEventType::kPacketRtxRejected},
         {"PacketReceived", StatisticsEventType::kPacketReceived}}};

StatisticsEventMediaType ToMediaType(StreamType type) {
  switch (type) {
    case StreamType::kUnknown:
      return StatisticsEventMediaType::kUnknown;
    case StreamType::kAudio:
      return StatisticsEventMediaType::kAudio;
    case StreamType::kVideo:
      return StatisticsEventMediaType::kVideo;
  }

  OSP_NOTREACHED();
}

FrameEvent::FrameEvent() = default;
FrameEvent::FrameEvent(const FrameEvent& other) = default;
FrameEvent::FrameEvent(FrameEvent&& other) noexcept = default;
FrameEvent& FrameEvent::operator=(const FrameEvent& other) = default;
FrameEvent& FrameEvent::operator=(FrameEvent&& other) = default;
FrameEvent::~FrameEvent() = default;

bool FrameEvent::operator==(const FrameEvent& other) const {
  return frame_id == other.frame_id && type == other.type &&
         media_type == other.media_type &&
         rtp_timestamp == other.rtp_timestamp && width == other.width &&
         height == other.height && size == other.size &&
         timestamp == other.timestamp && delay_delta == other.delay_delta &&
         key_frame == other.key_frame && target_bitrate == other.target_bitrate;
}

PacketEvent::PacketEvent() = default;
PacketEvent::PacketEvent(const PacketEvent& other) = default;
PacketEvent::PacketEvent(PacketEvent&& other) noexcept = default;
PacketEvent& PacketEvent::operator=(const PacketEvent& other) = default;
PacketEvent& PacketEvent::operator=(PacketEvent&& other) = default;
PacketEvent::~PacketEvent() = default;

bool PacketEvent::operator==(const PacketEvent& other) const {
  return packet_id == other.packet_id && max_packet_id == other.max_packet_id &&
         rtp_timestamp == other.rtp_timestamp && frame_id == other.frame_id &&
         size == other.size && timestamp == other.timestamp &&
         type == other.type && media_type == other.media_type;
}

}  // namespace cast
}  // namespace openscreen
