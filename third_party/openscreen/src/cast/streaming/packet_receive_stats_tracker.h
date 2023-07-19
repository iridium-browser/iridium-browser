// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_PACKET_RECEIVE_STATS_TRACKER_H_
#define CAST_STREAMING_PACKET_RECEIVE_STATS_TRACKER_H_

#include <stdint.h>

#include "cast/streaming/expanded_value_base.h"
#include "cast/streaming/rtcp_common.h"
#include "cast/streaming/rtp_time.h"
#include "platform/api/time.h"

namespace openscreen {
namespace cast {

// Maintains statistics for RTP packet arrival timing, jitter, and loss rates;
// and then uses these to compute and set the related fields in a RTCP Receiver
// Report block.
class PacketReceiveStatsTracker {
 public:
  explicit PacketReceiveStatsTracker(int rtp_timebase);
  ~PacketReceiveStatsTracker();

  // This should be called each time a RTP packet is successfully parsed,
  // whether the packet is a duplicate or not. The |sequence_number| and
  // |rtp_timestamp| arguments should be the values from the
  // RtpPacketParser::ParseResult. |arrival_time| is when the packet was
  // received (i.e., right-off the network socket, before any
  // processing/parsing).
  void OnReceivedValidRtpPacket(uint16_t sequence_number,
                                RtpTimeTicks rtp_timestamp,
                                Clock::time_point arrival_time);

  // Populates *only* those fields in the given |report| that pertain to packet
  // loss, jitter, and the latest-known RTP packet sequence number.
  void PopulateNextReport(RtcpReportBlock* report);

 private:
  // Expands the 16-bit raw packet sequence counter values into full-form,
  // initially constructed from a "first" value.
  class PacketSequenceNumber
      : public ExpandedValueBase<int64_t, PacketSequenceNumber> {
   public:
    constexpr PacketSequenceNumber()
        : ExpandedValueBase(std::numeric_limits<int64_t>::min()) {}
    constexpr explicit PacketSequenceNumber(uint16_t first_raw_counter_value)
        : ExpandedValueBase(static_cast<int64_t>(first_raw_counter_value)) {}

    constexpr bool is_null() const { return *this == PacketSequenceNumber(); }

    constexpr PacketSequenceNumber previous() const {
      return PacketSequenceNumber(value_ - 1);
    }

    // Distance operator.
    constexpr int64_t operator-(PacketSequenceNumber rhs) const {
      return value_ - rhs.value_;
    }

   private:
    friend class ExpandedValueBase<int64_t, PacketSequenceNumber>;

    constexpr explicit PacketSequenceNumber(int64_t value)
        : ExpandedValueBase(value) {}
  };

  const int rtp_timebase_;  // RTP timestamp ticks per second.

  // Until |num_rtp_packets_received_| is greater than zero, the rest of these
  // fields contain invalid values.
  int64_t num_rtp_packets_received_ = 0;
  int64_t num_rtp_packets_received_at_last_report_;

  // The greatest packet sequence number seen in any RTP packet.
  PacketSequenceNumber greatest_sequence_number_;

  // One before the packet sequence number contained in the very first RTP
  // packet seen. This is "one before" to simplify the packet count
  // calculations.
  PacketSequenceNumber base_sequence_number_;

  // The value of |greatest_sequence_number_| when the last call to
  // PopulateNextReport() was made. This is used in the computation of the
  // packet loss rate between reports.
  PacketSequenceNumber greatest_sequence_number_at_last_report_;

  // The time the last RTP packet was received. This is used in the computation
  // that updates |jitter_|.
  Clock::time_point last_rtp_packet_arrival_time_;

  // The RTP timestamp of the last RTP packet received. This is used in the
  // computation that updates |jitter_|.
  RtpTimeTicks last_rtp_packet_timestamp_;

  // The interarrival jitter. See RFC 3550 spec, section 6.4.1. The Cast
  // Streaming spec diverges from the algorithm in the RFC spec in that it uses
  // different pieces of timing data to calculate this metric.
  Clock::duration jitter_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_PACKET_RECEIVE_STATS_TRACKER_H_
