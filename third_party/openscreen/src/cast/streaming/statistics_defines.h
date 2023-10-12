// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_STATISTICS_DEFINES_H_
#define CAST_STREAMING_STATISTICS_DEFINES_H_

#include <stddef.h>
#include <stdint.h>

#include "cast/streaming/constants.h"
#include "cast/streaming/frame_id.h"
#include "cast/streaming/rtp_time.h"
#include "platform/api/time.h"
#include "util/enum_name_table.h"

namespace openscreen {
namespace cast {

enum class StatisticsEventType : int {
  kUnknown = 0,

  // Sender side frame events.
  kFrameCaptureBegin = 1,
  kFrameCaptureEnd = 2,
  kFrameEncoded = 3,
  kFrameAckReceived = 4,

  // Receiver side frame events.
  kFrameAckSent = 5,
  kFrameDecoded = 6,
  kFramePlayedOut = 7,

  // Sender side packet events.
  kPacketSentToNetwork = 8,
  kPacketRetransmitted = 9,
  kPacketRtxRejected = 10,

  // Receiver side packet events.
  kPacketReceived = 11,

  kNumOfEvents = kPacketReceived + 1
};

enum class StatisticsEventMediaType : int {
  kUnknown = 0,
  kAudio = 1,
  kVideo = 2
};

StatisticsEventMediaType ToMediaType(StreamType type);

extern const EnumNameTable<StatisticsEventType,
                           static_cast<size_t>(
                               StatisticsEventType::kNumOfEvents)>
    kStatisticEventTypeNames;

struct FrameEvent {
  constexpr FrameEvent(FrameId frame_id,
                       StatisticsEventType type,
                       StatisticsEventMediaType media_type,
                       RtpTimeTicks rtp_timestamp,
                       int width,
                       int height,
                       uint32_t size,
                       Clock::time_point timestamp,
                       Clock::duration delay_delta,
                       bool key_frame,
                       int target_bitrate)
      : frame_id(frame_id),
        type(type),
        media_type(media_type),
        rtp_timestamp(rtp_timestamp),
        width(width),
        height(height),
        size(size),
        timestamp(timestamp),
        delay_delta(delay_delta),
        key_frame(key_frame),
        target_bitrate(target_bitrate) {}

  FrameEvent();
  FrameEvent(const FrameEvent& other);
  FrameEvent(FrameEvent&& other) noexcept;
  FrameEvent& operator=(const FrameEvent& other);
  FrameEvent& operator=(FrameEvent&& other);
  ~FrameEvent();

  bool operator==(const FrameEvent& other) const;

  // The frame this event is associated with.
  FrameId frame_id;

  // The type of this frame event.
  StatisticsEventType type = StatisticsEventType::kUnknown;

  // Whether this was audio or video (or unknown).
  StatisticsEventMediaType media_type = StatisticsEventMediaType::kUnknown;

  // The RTP timestamp of the frame this event is associated with.
  RtpTimeTicks rtp_timestamp;

  // Resolution of the frame. Only set for video FRAME_CAPTURE_END events.
  int width = 0;
  int height = 0;

  // Size of encoded frame in bytes. Only set for FRAME_ENCODED event.
  // Note: we use uint32_t instead of size_t for byte count because this struct
  // is sent over IPC which could span 32 & 64 bit processes.
  uint32_t size = 0;

  // Time of event logged.
  Clock::time_point timestamp;

  // Only set for FRAME_PLAYOUT events.
  // If this value is zero the frame is rendered on time.
  // If this value is positive it means the frame is rendered late.
  // If this value is negative it means the frame is rendered early.
  Clock::duration delay_delta;

  // Whether the frame is a key frame. Only set for video FRAME_ENCODED event.
  bool key_frame = false;

  // The requested target bitrate of the encoder at the time the frame is
  // encoded. Only set for video FRAME_ENCODED event.
  int target_bitrate = 0;
};

struct PacketEvent {
  constexpr PacketEvent(uint16_t packet_id,
                        uint16_t max_packet_id,
                        RtpTimeTicks rtp_timestamp,
                        FrameId frame_id,
                        uint32_t size,
                        Clock::time_point timestamp,
                        StatisticsEventType type,
                        StatisticsEventMediaType media_type)
      : packet_id(packet_id),
        max_packet_id(max_packet_id),
        rtp_timestamp(rtp_timestamp),
        frame_id(frame_id),
        size(size),
        timestamp(timestamp),
        type(type),
        media_type(media_type) {}

  PacketEvent();
  PacketEvent(const PacketEvent& other);
  PacketEvent(PacketEvent&& other) noexcept;
  PacketEvent& operator=(const PacketEvent& other);
  PacketEvent& operator=(PacketEvent&& other);
  ~PacketEvent();

  bool operator==(const PacketEvent& other) const;

  // The packet this event is associated with.
  uint16_t packet_id = 0;

  // The highest packet ID seen so far at time of event.
  uint16_t max_packet_id = 0;

  // The RTP timestamp of the frame this event is associated with.
  RtpTimeTicks rtp_timestamp;

  // The frame this event is associated with.
  FrameId frame_id;

  // The size of this packet.
  uint32_t size = 0;

  // Time of event logged.
  Clock::time_point timestamp;

  // The type of this packet event.
  StatisticsEventType type = StatisticsEventType::kUnknown;

  // Whether this was audio or video (or unknown).
  StatisticsEventMediaType media_type = StatisticsEventMediaType::kUnknown;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_STATISTICS_DEFINES_H_
